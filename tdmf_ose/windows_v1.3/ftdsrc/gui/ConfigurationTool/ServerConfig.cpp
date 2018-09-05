// ServerConfig.cpp: implementation of the CServerConfig class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ConfigurationTool.h"
#include "ServerConfig.h"
#include "Command.h"

#include "license.h"
#include "ftd_lic.h"

// Mike Pollett
#include "../../tdmf.inc"

extern "C" 
{
#include "sock.h"
}


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define REG_PATH "Software\\" OEMNAME "\\Dtc\\CurrentVersion"
#define REG_KEY_PATH "InstallPath"
#define REG_PARAM_PATH "SYSTEM\\CurrentControlSet\\Services\\" DRIVERNAME "\\Parameters"
#define REG_DRIVER_PATH "SYSTEM\\CurrentControlSet\\Services\\" DRIVERNAME
#define REG_KEY_BAB "num_chunks"
#define REG_KEY_CHUNK_SIZE "chunk_size"
#define REG_KEY_START "start"
#define REG_KEY_MAXMEM "maxmem"
#define REG_KEY_TCP "tcp_window_size"
#define REG_KEY_PORT "port"


//////////////////////////////////////////////////////////////////////
//

CServerConfig::CServerConfig(): m_bReboot(false), m_nBABSize(-1), m_nChunkSize(0), m_bGroupRead(false)
{
	// Read values from registry
	GetRegConfigPath();
	GetRegBabSize();
	GetRegTCPWinSize();
	GetRegPort();

	// Init Command path
	CCommand::m_cstrInstallPath = m_szInstallPath;
}

CServerConfig::~CServerConfig()
{
}

UINT CServerConfig::GetMaxBABSize()
{
	UINT nMaxBabSize = 192;

	int nMemoryLimit = INT_MAX;
	MEMORYSTATUS stat;
	GlobalMemoryStatus(&stat);
	if (stat.dwTotalPhys > 0)
	{
		nMemoryLimit = (stat.dwTotalPhys / (1024 * 1024)/*MB*/);
	}

	#define BAB_GRANULARITY 32
	nMaxBabSize = ((nMemoryLimit * 6 / 10) / BAB_GRANULARITY) * BAB_GRANULARITY; 

	// if NT 
	if (_winver <= 0x400)
	{
		nMaxBabSize = __min(192, nMaxBabSize);
	}
	else
	{
		nMaxBabSize = __min(224, nMaxBabSize);
	}

	return nMaxBabSize;
}

void CServerConfig::GetLicenseKey(CString& cstrLicenseKey, CString& cstrExpDate)
{
	char	szLicenseKey[_MAX_PATH];
	char	szDate[100];

	// init the license key
	memset(szLicenseKey, 0, sizeof(szLicenseKey));
	memset(szDate, 0, sizeof(szDate));

	getLicenseKeyFromReg(szLicenseKey);
	cstrLicenseKey = szLicenseKey;
	
	InitLicenseKey(cstrExpDate);
}

void CServerConfig::WriteLicenseKey(LPCSTR lpcstrLicenseKey)
{
	writeLicenseKeyToReg((char*)lpcstrLicenseKey);
}

void CServerConfig::GetRegConfigPath()
{
	HKEY	happ;
	DWORD	dwType = 0;
    DWORD	dwSize = _MAX_PATH;

    // Read the current state from the registry
    // Try opening the registry key:
    // HKEY_CURRENT_USER\Software\FullTime Software\<AppName>
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_PATH,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                        REG_KEY_PATH,
                        NULL,
                        &dwType,
                        (BYTE*)m_szInstallPath,
                        &dwSize) != ERROR_SUCCESS)
		{	
		}
		strcat(m_szInstallPath, "\\");

		RegCloseKey(happ);
	}
}

void CServerConfig::GetRegBabSize()
{
	HKEY	happ;
    DWORD	dwSize = sizeof(DWORD);
	DWORD	dwReadValue = 0;
	BOOLEAN	bMaxMem = FALSE;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_PARAM_PATH,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{

		if ( RegQueryValueEx(happ,
                        REG_KEY_MAXMEM,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) == ERROR_SUCCESS)
		{
			if (dwReadValue != 0)
			{
				bMaxMem = TRUE;
				dwReadValue = -1;
			}
		}

		if (!bMaxMem)
		{
			if (RegQueryValueEx(happ,
								REG_KEY_CHUNK_SIZE,
								NULL,
								0,
								(PBYTE)&dwReadValue,
								&dwSize) != ERROR_SUCCESS)
			{
				dwReadValue = 0;
			}
			else
			{
				m_nChunkSize = dwReadValue;

				if (RegQueryValueEx(happ,
							REG_KEY_BAB,
							NULL,
							0,
							(PBYTE)&dwReadValue,
							&dwSize) != ERROR_SUCCESS)
			{
				dwReadValue = 0;
			}
				else
				{
					dwReadValue *= m_nChunkSize/(1024*1024);
				}
			}
		}
	
		RegCloseKey(happ);
	}

	m_nBABSize = dwReadValue;
}

void CServerConfig::GetRegTCPWinSize()
{
	HKEY	happ;
    DWORD	dwSize = sizeof(DWORD);
	DWORD	dwReadValue = 0;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_PATH,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                        REG_KEY_TCP,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) != ERROR_SUCCESS)
		{
			dwReadValue = 0;
		}

		RegCloseKey(happ);
	}
	
	m_nTcpWindowSize = dwReadValue/1024;
}

void CServerConfig::GetRegPort()
{
	HKEY	happ;
    DWORD	dwSize = sizeof(DWORD);
	DWORD	dwReadValue = 0;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_PATH,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                        REG_KEY_PORT,
                        NULL,
                        0,
                        (PBYTE)&dwReadValue,
                        &dwSize) != ERROR_SUCCESS)
		{
			dwReadValue = 0;
		}

		RegCloseKey(happ);
	}
	
	m_nPrimaryPortNumber = dwReadValue;
}

bool CServerConfig::Save()
{
	// Write info to the Reg
	HKEY	happ;
	
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PARAM_PATH, 0,	KEY_SET_VALUE, &happ) == ERROR_SUCCESS) 
	{
		DWORD nNumChunks = m_nBABSize;

		if (m_nChunkSize > 0)
		{
			nNumChunks = m_nBABSize/(m_nChunkSize / (1024 * 1024));
		}

		RegSetValueEx(happ, REG_KEY_BAB, 0, REG_DWORD, (const BYTE *)&nNumChunks, sizeof(DWORD));

		RegCloseKey(happ);
	}

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_DRIVER_PATH, 0,	KEY_SET_VALUE, &happ) == ERROR_SUCCESS) 
	{
		DWORD dwValue;

		if (m_nBABSize == 0)
		{
		    dwValue = 4;  // SERVICE DISABLED (4) else
		}
		else
		{
			dwValue = 0;  // SERVICE BOOT START (0)
		}

		RegSetValueEx(happ, REG_KEY_START, 0, REG_DWORD, (const BYTE *)&dwValue,	sizeof(DWORD));

		RegCloseKey(happ);
	}
	
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0, KEY_SET_VALUE, &happ) == ERROR_SUCCESS) 
	{
		UINT nTcpWndSize = m_nTcpWindowSize * 1024;
		RegSetValueEx(happ, REG_KEY_TCP, 0, REG_DWORD, (const BYTE *)&nTcpWndSize, sizeof(DWORD));
		
		RegSetValueEx(happ, REG_KEY_PORT, 0, REG_DWORD, (const BYTE *)&m_nPrimaryPortNumber, sizeof(DWORD));
		
		RegCloseKey(happ);
	}

	return true;
}

int CServerConfig::InitLicenseKey(CString& cstrDate)
{
	int	iRc = 0;

	iRc = ftd_lic_verify();

	if(iRc == 1)
	{
		cstrDate = "Valid License";
	}
	else if(iRc == -2)
	{
		cstrDate = "License Key Is Empty";
	}
	else if(iRc == -3)
	{
		cstrDate = "Bad Checksum";
	}
	else if(iRc == -4)
	{
		cstrDate = "Expired License";
	}
	else if(iRc == -5)
	{
		cstrDate = "Wrong Host";
	}
	else if(iRc == -7)
	{
		cstrDate = "Wrong Machine Type";
	}
	else if(iRc == -8)
	{
		cstrDate = "Bad Feature Mask";
	}

	return iRc;
}

CGroupConfig* CServerConfig::GetFirstGroup()
{
	if (m_bGroupRead == false)
	{
		for(int i = 0; i <= 999; i++)
		{
			CString cstrFileName;
			cstrFileName.Format("%sp%03d.cfg", m_szInstallPath, i);

			FILE* file = fopen(cstrFileName, "r");
			if (file != NULL)
			{
				fclose(file); // Close file first; GroupConfig constructor will open it to read the Note field

				CGroupConfig GroupCfg(i, cstrFileName);
				CString cstrPort;
				cstrPort.Format("%d", m_nPrimaryPortNumber);
				GroupCfg.SetPrimaryPort(cstrPort);

				m_mapGroup[i] = GroupCfg;
			}
		}

		m_bGroupRead = true;
	}

	m_itGroup = m_mapGroup.begin();
	return (m_itGroup != m_mapGroup.end()) ? &m_itGroup->second : NULL;
}

CGroupConfig* CServerConfig::GetNextGroup()
{
	m_itGroup++;
	return (m_itGroup != m_mapGroup.end()) ? &m_itGroup->second : NULL;
}

CGroupConfig* CServerConfig::GetGroup(UINT nKey)
{
	return &m_mapGroup[nKey];
}

CGroupConfig* CServerConfig::AddGroup(UINT nKey)
{
	CString cstrFileName;
	cstrFileName.Format("%sp%03d.cfg", m_szInstallPath, nKey);

	CGroupConfig GroupConfigNew(nKey, cstrFileName);

	// get the local host ip
	char szLocalHost[64];
	gethostname(szLocalHost, sizeof(szLocalHost));
	struct hostent *host = gethostbyname(szLocalHost);
    if (host != NULL)
	{
	    struct in_addr in;
		memcpy(&in.s_addr, *host->h_addr_list, sizeof(in.s_addr));
		GroupConfigNew.SetPrimaryHost(inet_ntoa(in));
	}

	// set secondary port to primary port # by default
	CString cstrPort;
	cstrPort.Format("%d", m_nPrimaryPortNumber);
	GroupConfigNew.SetPrimaryPort(cstrPort);
	GroupConfigNew.SetSecondaryPort(cstrPort);

	// Give default value
	CString cstrPStore;
	cstrPStore.Format("%sPStore\\PStore%d.Dat", m_szInstallPath, nKey);
	GroupConfigNew.SetPStore(cstrPStore);

	CString cstrJournal;
	cstrJournal.Format("%sJournal", m_szInstallPath);
	GroupConfigNew.SetJournal(cstrJournal);

	// Save group 
	m_mapGroup[nKey] = GroupConfigNew;

	return &m_mapGroup[nKey];
}

bool CServerConfig::DeleteGroup(UINT nKey)
{
	std::map<UINT, CGroupConfig>::iterator it = m_mapGroup.find(nKey);
	if (it != m_mapGroup.end())
	{
		m_mapGroup.erase(it);
		return true;
	}

	return false;
}
