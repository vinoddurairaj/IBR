// CreateTdmfTables.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
//#include "createTdmfTables.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

using namespace std;

class UserInfo
{
public:
	UserInfo() {}
	UserInfo(CString cszPW, UINT nAccesFlag) : m_cszPW(cszPW), m_nAccesFlag(nAccesFlag) {}
	
	CString m_cszPW;
	UINT    m_nAccesFlag;
};

class Config
{
public:
    Config(const char* szUser,const char* szPW,const char* szHostName)
    {
		cszDbCreator    = szUser;
        mapUser[szUser] = UserInfo(szPW, DB_ROLE_OWNER);
        cszDBServer     = szHostName;
    };

	std::map<CString, UserInfo> mapUser;
    CString                     cszDBServer;
	CString                     cszDbCreator;

	void AddUser(const char* szUser,const char* szPW, UINT nAccesFlag)
	{
		mapUser[szUser] = UserInfo(szPW, nAccesFlag);
	}
};

/////////////////////////////////////////////////////////////////////////////
/*
#include "ftd_pathnames.h"  //for FTD_SOFTWARE_KEY
CString GetTdmfInstallDirectory()
{
    HKEY	happ;
    char    value[MAX_PATH];
 	DWORD	dwType = REG_SZ;
    DWORD	dwSize = MAX_PATH;
    CString path;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     FTD_SOFTWARE_KEY,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                             "InstallPath",
                             NULL,
                             &dwType,
                             (BYTE*)value,
                             &dwSize) == ERROR_SUCCESS) {
            path = value;
		}

	    RegCloseKey(happ);
	}
	return path;
}
*/

/////////////////////////////////////////////////////////////////////////////
static 
bool CreateTdmfDB(FsTdmfDb & tdmfDb, Config & cfg, CString & csztdmfDir, CString & cszError) 
{
    //CString tdmfDir = GetTdmfInstallDirectory();
    //tdmfDir = "c:\\tdmfdb";
    CString cszTdmfDb  = "DtcDb";

    //create DB
    if ( !tdmfDb.FtdCreateDb ( cszTdmfDb , csztdmfDir ) )
    {
        cszError =  "Error while creating database <" ;
        cszError += cszTdmfDb + ">, using path <" + csztdmfDir + ">.  " + tdmfDb.FtdGetErrMsg();
        //return false;
    }

    //create logins in DB
	for(std::map<CString, UserInfo>::iterator it = cfg.mapUser.begin(); it != cfg.mapUser.end(); it++)
	{
		if ( !tdmfDb.FtdCreateLoginsWithAccess ( it->first, it->second.m_cszPW, cszTdmfDb, it->second.m_nAccesFlag ) )
		{
	        cszError =  "Error while creating login <";
		    cszError += it->first + "> for database <" + cszTdmfDb + ">.  " + tdmfDb.FtdGetErrMsg();
	        //return false;
	    }
	}

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//extern "C" TDMFINSTALL_API 
//int CreateTdmfDB(const char* szPathForDBFiles, const char* szCollectorPort)
STDAPI CreateTdmfDB(const char* szPathForDBFiles, const char* szCollectorPort)
{
    /*
	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
    */
    bool bCreateDb = true, bCreateTables = true;
    CString cszError;
    CString cszDBPath = szPathForDBFiles;
	
	char computerName[MAX_COMPUTERNAME_LENGTH+1];
	DWORD size = MAX_COMPUTERNAME_LENGTH+1;
	if (!GetComputerName(computerName, &size))
    {
        printf("\nError, could not get the local computer name.\n");
        return -1;
    }

	CString cszComputerName = computerName;
	cszComputerName += "\\DTC";

    Config  cfg("DtcCollector","dtc2003",(LPCTSTR)cszComputerName);
	cfg.AddUser("DtcAdmin",  "dtc", DB_ROLE_OWNER|DB_ROLE_SECURITYADMIN|DB_ROLE_ACCESSADMIN|DB_SRV_ROLE_SECURITYADMIN);
	cfg.AddUser("DtcUser",   "dtc", DB_ROLE_OWNER);
	cfg.AddUser("DtcViewer", "dtc", DB_ROLE_DATAREADER);

    if ( bCreateDb )
    {
		try
		{
			FsTdmfDb tdmfDb ( "sa", "dtc01", cfg.cszDBServer); //PB=>new password "tdmf01" for sa user, 14032003

			// Limit the amount of memory used by the BD
			CString lszDdlMemory;
			int     nMemoryLimit = 128;  // Default = 128 MB
			MEMORYSTATUS stat;
			GlobalMemoryStatus(&stat);
			if (stat.dwTotalPhys > 0)
			{
				nMemoryLimit = (stat.dwTotalPhys / (1024 * 1024)/*MB*/) / 2;
			}

			// First show advance option
			lszDdlMemory = "EXEC sp_configure 'show advanced option', '1'";
			try
			{
				tdmfDb.ExecuteSQL( lszDdlMemory );
			}
			catch ( CDBException* e )
			{
				e->Delete ();
			}
			lszDdlMemory = "RECONFIGURE WITH OVERRIDE";
			try
			{
				tdmfDb.ExecuteSQL( lszDdlMemory );
			}
			catch ( CDBException* e )
			{
				e->Delete ();
			}


			// Second, set advance option
			lszDdlMemory.Format ( "EXEC sp_configure 'max server memory', '%d'", nMemoryLimit );
			try
			{
				tdmfDb.ExecuteSQL( lszDdlMemory );
			}
			catch ( CDBException* e )
			{
				cszError = e->m_strError;
				e->Delete ();

				printf("\nError, could not change instance size.\n%s",(LPCTSTR)cszError);
			}
			lszDdlMemory = "RECONFIGURE WITH OVERRIDE";
			try
			{
				tdmfDb.ExecuteSQL( lszDdlMemory );
			}
			catch ( CDBException* e )
			{
				e->Delete ();
			}

			// Boost SQLServer process
			lszDdlMemory.Format ( "EXEC sp_configure 'priority boost', '1'" );
			try
			{
				tdmfDb.ExecuteSQL( lszDdlMemory );
			}
			catch ( CDBException* e )
			{
				cszError = e->m_strError;
				e->Delete ();

				printf("\nError, could not change instance priority.\n%s",(LPCTSTR)cszError);
			}
			lszDdlMemory = "RECONFIGURE WITH OVERRIDE";
			try
			{
				tdmfDb.ExecuteSQL( lszDdlMemory );
			}
			catch ( CDBException* e )
			{
				e->Delete ();
			}

			// Create tables
			if ( !CreateTdmfDB(tdmfDb,cfg,cszDBPath,cszError) )
			{
				printf("\nError, could not properly create the database and tables.\n%s",(LPCTSTR)cszError);
				return -1;
			}
		}
		catch(...)
		{
            printf("\nError, could not connect to the DB instance on SQL Server.\n%s",(LPCTSTR)cszError);
			return -1;
		}
    }

    if ( bCreateTables )
    {
        FsTdmfDb tdmfDb ( cfg.cszDbCreator, cfg.mapUser[cfg.cszDbCreator].m_cszPW, cfg.cszDBServer, "DtcDb" );
        //create tables
        if ( !tdmfDb.FtdCreateTbls() )
        {
            cszError =  "Error while creating tables: " + tdmfDb.FtdGetErrMsg();
            //cszError += cfg.cszUser + "> for database <" + cszTdmfDb + ">.  " + tdmfDb.FtdGetErrMsg();
            //return false;
        }
		else
		{
			tdmfDb.mpRecNvp->FtdNew( CString(TDMF_NVP_NAME_COLLECTOR_PORT) , CString(szCollectorPort) );
			tdmfDb.mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("TCP listener Port number opened by Collector.") );

			tdmfDb.mpRecNvp->FtdNew( CString(TDMF_NVP_NAME_DB_VERSION) , CString(FsTdmfDb::mszDbVersion) );
			tdmfDb.mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, CString("Database version installed on the computer.") );
		}
    }

	return 0;
}
