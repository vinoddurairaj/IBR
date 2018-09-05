// TdmfDbUpgrade.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "TdmfDbUpgrade.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CTdmfDbUpgradeApp

BEGIN_MESSAGE_MAP(CTdmfDbUpgradeApp, CWinApp)
	//{{AFX_MSG_MAP(CTdmfDbUpgradeApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTdmfDbUpgradeApp construction

CTdmfDbUpgradeApp::CTdmfDbUpgradeApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTdmfDbUpgradeApp object

CTdmfDbUpgradeApp theApp;



bool UpgradeDbFrom1_3_2To1_3_3(const FsTdmfDb* db)
{
//	CString cszSql = "ADD " + FsTdmfRecAlert::smszFldTest + " SMALLINT NULL";

	// Remove ChunkSize from t_ServerInfo
	CString cszSql = "DROP COLUMN ChunkSize";
	if(!db->mpRecSrvInf->FtdAlter(cszSql))
		return false;

	// Add ChunkSize to t_LogicalGroup
	 cszSql = "ADD " + FsTdmfRecLgGrp::smszFldChunkSize + " INT NULL";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;

	// Add EnableCompression to t_LogicalGroup
	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldEnableCompression + " BIT NULL";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;

	return true;
}

bool UpgradeDbFrom1_3_3To1_3_4(FsTdmfDb* db)
{
	if (!db->FtdCreateTbl(db->mpRecSysLog))
		return false;

	if (!db->FtdCreateTbl(db->mpRecKeyLog))
		return false;

    if (!db->FtdCreateTbl(db->mpRecScriptSrv))
		return false;

	CString cszSql;
	
	// Add TgtDHCPNameUsed to t_LogicalGroup
	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldTgtDHCPNameUsed + " INT NOT NULL DEFAULT 0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;

    // Add PrimaryDHCPNameUsed to t_LogicalGroup
	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldPrimaryDHCPNameUsed + " INT NOT NULL DEFAULT 0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;

    // Add PrimaryEditedIPUsed to t_LogicalGroup
	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldPrimaryEditedIPUsed + " INT NOT NULL DEFAULT 0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;

	   // Add TgtEditedIPUsed to t_LogicalGroup
	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldTgtEditedIPUsed + " INT NOT NULL DEFAULT 0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;
 

	return true;
}

bool UpgradeDbFrom1_3_4To1_3_5(FsTdmfDb* db)
{
   // Add Agent Port to t_ServerInfo
	CString cszSql = "ADD " + FsTdmfRecSrvInf::smszFldReplicationPort + " SMALLINT NOT NULL DEFAULT 575";
	if(!db->mpRecSrvInf->FtdAlter(cszSql))
		return false;	

	return true;
}

bool UpgradeDbFrom1_3_5To1_3_6(FsTdmfDb* db)
{
   // Add Throttle to t_ LogicalGroup
	CString cszSql = "ADD " + FsTdmfRecLgGrp::smszFldThrottles + " VARCHAR(7000)";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;	

	// Add Rep Group Edited IPs
	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldPrimaryEditedIP + " DECIMAL NOT NULL DEFAULT   0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;	

	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldTgtEditedIP + " DECIMAL NOT NULL DEFAULT   0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;	
      
	// Remove ChunkSize from t_ServerInfo
	cszSql = "DROP COLUMN ClusterIP";
	db->mpRecSrvInf->FtdAlter(cszSql);

	return true;
}

bool UpgradeDbFrom1_3_6To1_3_7(FsTdmfDb* db)
{
	// Remove Perforance table constraint foreign key
	CString cszSql = "DROP CONSTRAINT PerfFk";
	if(!db->mpRecPerf->FtdAlter(cszSql))
		return false;

	// Add JournalLess to t_ LogicalGroup
	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldJournalLess + " BIT DEFAULT 0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;	

	return true;
}

bool UpgradeDbFrom1_3_7To1_3_8(FsTdmfDb* db)
{
	// Add JournalLess to t_ LogicalGroup
	CString cszSql = "ADD " + FsTdmfRecLgGrp::smszFldSymmetric + " BIT DEFAULT 0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;	

	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldSymmetricGroupNumber + " INT NOT NULL DEFAULT 0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;	

	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldSymmetricNormallyStarted + " BIT NOT NULL DEFAULT 0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;	

	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldFailoverInitialState + " INT NOT NULL DEFAULT 0";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;	

	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldSymmetricPStoreFile + " VARCHAR(300)";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;	

	cszSql = "ADD " + FsTdmfRecLgGrp::smszFldSymmetricJournalDirectory + " VARCHAR(300)";
	if(!db->mpRecGrp->FtdAlter(cszSql))
		return false;	

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Return codes:
// -------------
// 0  = Upgrade Successful
// 1  = DB already upgraded
// 2  = Could not get the local computer name
// 3  = Could not connect to the TDMF DB
// 4  = Could not get the TDMF DB version
// 5  = Invalid TDMF DB version
// 6  = Unable to update DBVersion info in DB
// 10 = Upgrade from version 1.3.2 to 1.3.3 wasn't able to be applied to the TDMF DB
//
//extern "C" __declspec(dllexport)
//int UpgradeTdmfDb(const char* szComputerName/*=NULL*/)
STDAPI UpgradeTdmfDb(const char* szComputerName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString cszComputerName;
	if (szComputerName == NULL)
	{
		char computerName[MAX_COMPUTERNAME_LENGTH+1];
		DWORD size = MAX_COMPUTERNAME_LENGTH+1;
		if (!GetComputerName(computerName, &size))
		{
			printf("\nError, could not get the local computer name.\n");
			return 2;
		}
		cszComputerName = computerName;
	}
	else
	{
		cszComputerName = szComputerName;
	}

	cszComputerName += "\\dtc";

	FsTdmfDb db("DtcAdmin", "dtc", (LPCTSTR)cszComputerName);
	if (!db.FtdIsOpen())
	{
        printf("\nError, could not connect to DB.\n");
        return 3;
	}

	// Get version from DB
	CString cszDbVersion = "";

    if ( db.mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_VERSION ) )
    {
        cszDbVersion = db.mpRecNvp->FtdSel(FsTdmfRecNvp::smszFldVal);
    }
    else
    {
        printf("\nError, could not get DB version.\n");
        return 4;
    }

	// Get version from libdb
	CString cszLibVersion = FsTdmfDb::mszDbVersion;

	if(cszDbVersion == cszLibVersion)
	{
        printf("\nThe DB already has an upgraded version.\n");
		return 1;
	}
	else
	{
		const short v1_3_0 = 0;
		const short v1_3_1 = 1;
		const short v1_3_2 = 2;
		const short v1_3_3 = 3;
		const short v1_3_4 = 4;
		const short v1_3_5 = 5;
		const short v1_3_6 = 6;
		const short v1_3_7 = 7;
		const short v1_3_8 = 8;


		short version;

		// Convert value from string to short, to be able to use a 'switch' statement
		if(strcmp((LPCTSTR)cszDbVersion, "1.3.0") == 0)
			version = v1_3_0;
		else if(strcmp((LPCTSTR)cszDbVersion, "1.3.1") == 0)
			version = v1_3_1;
		else if(strcmp((LPCTSTR)cszDbVersion, "1.3.2") == 0)
			version = v1_3_2;
		else if(strcmp((LPCTSTR)cszDbVersion, "1.3.3") == 0)
			version = v1_3_3;
		else if(strcmp((LPCTSTR)cszDbVersion, "1.3.4") == 0)
			version = v1_3_4;
		else if(strcmp((LPCTSTR)cszDbVersion, "1.3.5") == 0)
			version = v1_3_5;
		else if(strcmp((LPCTSTR)cszDbVersion, "1.3.6") == 0)
			version = v1_3_6;
		else if(strcmp((LPCTSTR)cszDbVersion, "1.3.7") == 0)
			version = v1_3_7;
		else if(strcmp((LPCTSTR)cszDbVersion, "1.3.8") == 0)
			version = v1_3_8;
		else
			version = -1;

		switch(version)
		{
		case v1_3_0:
			// No upgrade available
		case v1_3_1:
			// No upgrade available
		case v1_3_2:
			if(!UpgradeDbFrom1_3_2To1_3_3(&db))
			{
				printf("\nThe upgrade from version 1.3.2 to 1.3.3 wasn't able to be applied to DB.\n");
				return 10;
			}
		case v1_3_3:
			if (!UpgradeDbFrom1_3_3To1_3_4(&db))
				return 10;
		case v1_3_4:
			if (!UpgradeDbFrom1_3_4To1_3_5(&db))
				return 10;
		case v1_3_5:
			if (!UpgradeDbFrom1_3_5To1_3_6(&db))
				return 10;
		case v1_3_6:	
			if (!UpgradeDbFrom1_3_6To1_3_7(&db))
				return 10;
		case v1_3_7:	
			if (!UpgradeDbFrom1_3_7To1_3_8(&db))
				return 10;
			break;

		default:
			printf("\nThe DB version is invalid.\n");
			return 5;
		}
	}

	// Update DBVersion info in DB
    if ( !(db.mpRecNvp->FtdPos( TDMF_NVP_NAME_DB_VERSION ) &&
           db.mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldVal, cszLibVersion ) ) )
	{
		return 6;
	}

	return 0;
}