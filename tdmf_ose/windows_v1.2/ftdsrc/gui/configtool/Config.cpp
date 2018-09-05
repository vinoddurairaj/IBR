// Config.cpp : implementation file
//

#include "stdafx.h"
#include "DTCConfigTool.h"
#include "Config.h"
#include <conio.h>
#include <winioctl.h>

#include "misc.h"

#include "sock.h"

#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CConfig *lpConfig;
/////////////////////////////////////////////////////////////////////////////
// CConfig

CConfig::CConfig()
{
	lpConfig = this;	
}

CConfig::~CConfig()
{
}


BEGIN_MESSAGE_MAP(CConfig, CWnd)
	//{{AFX_MSG_MAP(CConfig)
	ON_WM_CREATE()
	ON_WM_ACTIVATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CConfig message handlers

void CConfig::getConfigPath(char *strPath)
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
                        REG_KEY,
                        NULL,
                        &dwType,
                        (BYTE*)strPath,
                        &dwSize) != ERROR_SUCCESS) {
			
		}
		strcat(strPath, "\\");
	}
	
	RegCloseKey(happ);
}

int CConfig::getRegBabSize(DWORD *iBabSize)
{
	HKEY	happ;
    DWORD	dwSize = sizeof(DWORD);
	DWORD	dwReadValue = 0;
	BOOLEAN	bMaxMem = TRUE;

	*iBabSize = 0;

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
                        &dwSize) != ERROR_SUCCESS) {
			bMaxMem = FALSE;
		}
		if (dwReadValue == 0)
			bMaxMem = FALSE;

		if ( RegQueryValueEx(happ,
						REG_KEY_BAB,
						NULL,
						0,
						(PBYTE)&dwReadValue,
						&dwSize) != ERROR_SUCCESS) {
			dwReadValue = 0;
		}
		*iBabSize = dwReadValue;
	}
	
	RegCloseKey(happ);

	if (bMaxMem)
		return -1;
	
	return 0;
}

void CConfig::getRegTCPWinSize(DWORD *iTCPWinSize)
{
	HKEY	happ;
    DWORD	dwSize = sizeof(DWORD);
	DWORD	dwReadValue;


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
                        &dwSize) != ERROR_SUCCESS) {
			
		}
	}
	
	*iTCPWinSize = dwReadValue;
	RegCloseKey(happ);
}

void CConfig::getRegPort(DWORD *iPort)
{
	HKEY	happ;
    DWORD	dwSize = sizeof(DWORD);
	DWORD	dwReadValue;


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
                        &dwSize) != ERROR_SUCCESS) {
			
		}
	}
	
	*iPort = dwReadValue;
	RegCloseKey(happ);
}

void CConfig::setDevCfg(int iBABSize, int iTCPWinSize, int iPortNum,
						int	iGroupID, char strGroupNote[_MAX_PATH],
						char strPath[_MAX_PATH], char strCfgFileInUse[_MAX_PATH])
{
	// m_structDevCfg structure contains all
	// important info from DTCConfigTool's initial dialog
	m_structDevCfg.iBABSize = iBABSize;
	m_structDevCfg.iTCPWinSize = iTCPWinSize;
	m_structDevCfg.iPortNum = iPortNum;
	m_structDevCfg.iGroupID = iGroupID;

	memset(m_structDevCfg.strGroupNote, 0, sizeof(m_structDevCfg.strGroupNote));
	memset(m_structDevCfg.strPath, 0, sizeof(m_structDevCfg.strPath));
	memset(m_structDevCfg.strCfgFileInUse, 0, sizeof(m_structDevCfg.strCfgFileInUse));

	strcpy(m_structDevCfg.strGroupNote, strGroupNote);
	strcpy(m_structDevCfg.strPath, strPath);
	strcpy(m_structDevCfg.strCfgFileInUse, strCfgFileInUse);

	m_structDTCDevValues.m_iNumDTCDevices = 1;

	// Write info to the Reg
	HKEY	happ;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PARAM_PATH, 0,
                     KEY_SET_VALUE, &happ) == ERROR_SUCCESS) 
	{
		RegSetValueEx(happ, REG_KEY_BAB, 0, REG_DWORD, (const BYTE *)&iBABSize,
						sizeof(DWORD));
		
		RegCloseKey(happ);
	}

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0,
                     KEY_SET_VALUE, &happ) == ERROR_SUCCESS) 
	{
		RegSetValueEx(happ, REG_KEY_TCP, 0, REG_DWORD, (const BYTE *)&iTCPWinSize,
						sizeof(DWORD));

		RegSetValueEx(happ, REG_KEY_PORT, 0, REG_DWORD, (const BYTE *)&iPortNum,
						sizeof(DWORD));

		RegCloseKey(happ);
	}


	// End write to Reg
}

void CConfig::setSysDlgValues()
{
	m_structSystemValues.m_iPortNum = m_sheetConfig->m_structSystemValues.m_iPortNum;
	m_structSystemValues.m_bChaining = m_sheetConfig->m_structSystemValues.m_bChaining;
	strcpy(m_structSystemValues.m_strPrimaryHostName, m_sheetConfig->m_structSystemValues.m_strPrimaryHostName);
	strcpy(m_structSystemValues.m_strPStoreDev, m_sheetConfig->m_structSystemValues.m_strPStoreDev);
	strcpy(m_structSystemValues.m_strSecondHostName, m_sheetConfig->m_structSystemValues.m_strSecondHostName);
	strcpy(m_structSystemValues.m_strJournalDir, m_sheetConfig->m_structSystemValues.m_strJournalDir);
	strcpy(m_structSystemValues.m_strNote, m_sheetConfig->m_structSystemValues.m_strNote);

/* JRL, Not used ????
	HKEY	happ;

	char	strPstore[_MAX_PATH];
	memset(strPstore, 0, sizeof(strPstore));

	sprintf(strPstore, "\\\\.\\%s", m_structSystemValues.m_strPStoreDev);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0,
                     KEY_SET_VALUE, &happ) == ERROR_SUCCESS) 
	{
		RegSetValueEx(happ, REG_KEY_PSTORE, 0, REG_SZ, (const BYTE *)strPstore,
						strlen(strPstore));
		
		RegCloseKey(happ);
	}
*/
}

void CConfig::setDTCDeviceValues()
{
	strcpy(m_structDTCDevValues.m_strDTCDev, m_sheetConfig->m_structDTCDevValues.m_strDTCDev);
	strcpy(m_structDTCDevValues.m_strRemarks, m_sheetConfig->m_structDTCDevValues.m_strRemarks);
	strcpy(m_structDTCDevValues.m_strDataDev, m_sheetConfig->m_structDTCDevValues.m_strDataDev);
	strcpy(m_structDTCDevValues.m_strMirrorDev, m_sheetConfig->m_structDTCDevValues.m_strMirrorDev);
	m_structDTCDevValues.m_iNumDTCDevices = m_sheetConfig->m_structDTCDevValues.m_iNumDTCDevices;
}

void CConfig::setThrottleValues()
{
	strcpy(m_structThrottleValues.m_strThrottle, m_sheetConfig->m_structThrottleValues.m_strThrottle);
	m_structThrottleValues.m_bThrottleTrace = m_sheetConfig->m_structThrottleValues.m_bThrottleTrace;
}

void CConfig::setTunableParams()
{
	m_structTunableParamsValues.m_bSyncMode = m_sheetConfig->m_structTunableParamsValues.m_bSyncMode;
	m_structTunableParamsValues.m_bCompression = m_sheetConfig->m_structTunableParamsValues.m_bCompression;
	m_structTunableParamsValues.m_bStatGen = m_sheetConfig->m_structTunableParamsValues.m_bStatGen;
	m_structTunableParamsValues.m_bNetThresh = m_sheetConfig->m_structTunableParamsValues.m_bNetThresh;
	m_structTunableParamsValues.m_iDepth = m_sheetConfig->m_structTunableParamsValues.m_iDepth;
	m_structTunableParamsValues.m_iTimeout = m_sheetConfig->m_structTunableParamsValues.m_iTimeout;
	m_structTunableParamsValues.m_iUpdateInterval = m_sheetConfig->m_structTunableParamsValues.m_iUpdateInterval;
	m_structTunableParamsValues.m_iMaxStatFileSize = m_sheetConfig->m_structTunableParamsValues.m_iMaxStatFileSize;
	m_structTunableParamsValues.m_iMaxTranRate = m_sheetConfig->m_structTunableParamsValues.m_iMaxTranRate;
	m_structTunableParamsValues.m_iDelayWrites = m_sheetConfig->m_structTunableParamsValues.m_iDelayWrites;
	m_structTunableParamsValues.m_liRefreshInterval = m_sheetConfig->m_structTunableParamsValues.m_liRefreshInterval;
}

void CConfig::setPropSheetValues(CDTCConfigPropSheet *sheetConfig)
{
	m_sheetConfig = sheetConfig;
}

unsigned long CConfig::gethostid(char *szDir)
{
	HANDLE hVolume;
	unsigned long serial = -1;
	DWORD dwRes, dwBytesRead;
	DWORD dwSize = (sizeof(DWORD) * 2) + (128 * sizeof(PARTITION_INFORMATION));
	DRIVE_LAYOUT_INFORMATION *drive = (struct _DRIVE_LAYOUT_INFORMATION *)malloc(dwSize);
	
	hVolume = OpenAVolume(szDir, GENERIC_READ);

	dwRes = DeviceIoControl(  hVolume,  // handle to a device
		IOCTL_DISK_GET_DRIVE_LAYOUT, // dwIoControlCode operation to perform
		NULL,                        // lpInBuffer; must be NULL
		0,                           // nInBufferSize; must be zero
		drive,						// pointer to output buffer
		dwSize,      // size of output buffer
		&dwBytesRead,				// receives number of bytes returned
		NULL						// pointer to OVERLAPPED structure); 
	);


	CloseVolume(hVolume);

	if (dwRes)
		serial = drive->Signature;

	free(drive);

	return serial;
}

void CConfig::writeConfigFile ()
{
	FILE		*file;
	CString		strReadString;
	time_t		ltime;
	char		strTime[128];
	struct tm	*gmt;
	char		strTempCount[10];
	char		szDiskInfo[256];

	time( &ltime );
	gmt = localtime( &ltime );
    sprintf(strTime, "%s", asctime( gmt ));

	// ardeb 020912 v
	CString lszFileInUse = m_structDevCfg.strCfgFileInUse;
	lszFileInUse += "Tmp";

	//file = fopen( m_structDevCfg.strCfgFileInUse, "w+");
	file = fopen( lszFileInUse, "w+");
	// ardeb 020912 ^

	strReadString.LoadString(IDS_STRING_HEADER_BREAK);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_OEM_FILE);
	strReadString = "#  " PRODUCTNAME " "+  strReadString;
	strReadString = strReadString + m_structDevCfg.strCfgFileInUse;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_OEM_VERSION);
	strReadString = "#  " PRODUCTNAME " "+  strReadString;
	strReadString = strReadString + VERSION" \n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_UPDATE);
	strReadString = strReadString + strTime;
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_HEADER_BREAK);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_NOTE);
	if(strcmp(m_structSystemValues.m_strNote, "") == 0)
	{
		strReadString = strReadString + m_structDevCfg.strGroupNote;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	}
	else
	{
		strReadString = strReadString + m_structSystemValues.m_strNote;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	}
	
	// Begin System Values
	strReadString.LoadString(IDS_STRING_PRIMARY_DEF);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_HOST);
	strReadString = strReadString + m_structSystemValues.m_strPrimaryHostName;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_PSTORE);
	char	strPStore[_MAX_PATH];
	memset(strPStore, 0, sizeof(strPStore));
	// [020712] AlRo v
	//sprintf(strPStore, "\\\\.\\%s", m_structSystemValues.m_strPStoreDev);
	sprintf(strPStore, "%.*s", _MAX_PATH-1, m_structSystemValues.m_strPStoreDev);
	// [020712] AlRo ^
	strReadString = strReadString + strPStore;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_SECONDARY);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_HOST);
	strReadString = strReadString + m_structSystemValues.m_strSecondHostName;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_JOURNAL);
	int iJournalDirLength;
	iJournalDirLength = strlen(m_structSystemValues.m_strJournalDir);
	if(iJournalDirLength == 1)
		strcat(m_structSystemValues.m_strJournalDir, ":\\");
	else if(m_structSystemValues.m_strJournalDir[iJournalDirLength - 1] == ':')
		m_structSystemValues.m_strJournalDir[iJournalDirLength] = '\\';
	strReadString = strReadString + m_structSystemValues.m_strJournalDir;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_SECOND_PORT);
	char	strTempPort[10];
	memset(strTempPort, 0, sizeof(strTempPort));
	itoa(m_structSystemValues.m_iPortNum, strTempPort, 10);
	strReadString = strReadString + strTempPort;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

	if(m_structSystemValues.m_bChaining)
	{
		strReadString.LoadString(IDS_STRING_CHAIN_ON);
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	}
	else
	{
		strReadString.LoadString(IDS_STRING_CHAIN_OFF);
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	}
	// End System Values

/*
	// Begin Throttles
	strReadString.LoadString(IDS_STRING_THROTTLE);
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

	// get all throttle commands
	CString	strAction;
	int	i = 0, iIndex = 0;
	char	chCompare = '0';
	char	strActionItem[_MAX_PATH];

	strReadString.LoadString(IDS_STRING_ACTION_LIST);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	while(chCompare != NULL)
	{
		memset(strActionItem, 0, sizeof(strActionItem));
		iIndex = 0;
		while(chCompare != '\n' && chCompare != NULL)
		{
			chCompare = m_structThrottleValues.m_strThrottle[i];
			strActionItem[iIndex] = m_structThrottleValues.m_strThrottle[i];
			i++;
			iIndex++;
		}
		strAction.LoadString(IDS_STRING_ACTION);
		strAction = strAction + strActionItem;
		fwrite(strAction.operator LPCTSTR (), sizeof(char), strAction.GetLength(), file); 
		chCompare = m_structThrottleValues.m_strThrottle[i];
		strAction.Empty();
	}
	

	strReadString.LoadString(IDS_STRING_END_ACTION_LIST);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	// End Throttles
*/
	// Begin DTC devices
	strReadString.LoadString(IDS_STRING_DEVICE_DEF);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

	// Profile section.  This has to work for mult DTC devices
	//
	int iDTCListCount = m_listStructDTCDevValues.GetCount();
	POSITION head = m_listStructDTCDevValues.GetHeadPosition();

	BOOL lbValid3Val = TRUE;
	for(int i = 1; i <= iDTCListCount; i++)
	{
		m_structDTCDevValues = m_listStructDTCDevValues.GetNext(head);

		strReadString.LoadString(IDS_STRING_PROFILE);
		memset(strTempCount, 0, sizeof(strTempCount));
		itoa(i, strTempCount, 10);

		strReadString = strReadString + strTempCount;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

		strReadString.LoadString(IDS_STRING_REMARK);
		strReadString = strReadString + m_structDTCDevValues.m_strRemarks;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

		strReadString.LoadString(IDS_STRING_PRIMARY_SYSA);
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

		itoa(i - 1, strTempCount, 10);

		strReadString = "  "CAPPRODUCTNAME "-DEVICE:  ";
		strReadString = strReadString + m_structDTCDevValues.m_strDataDev;
		strReadString = strReadString + strTempCount;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
		
		strReadString.LoadString(IDS_STRING_DATADISK);
		getDiskSigAndInfo(m_structDTCDevValues.m_strDataDev, szDiskInfo, -1);
		strReadString = strReadString + m_structDTCDevValues.m_strDataDev;
		strReadString = strReadString + szDiskInfo;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
		
		strReadString.LoadString(IDS_STRING_SEC_SYSB);
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
		
		
		strReadString.LoadString(IDS_STRING_MIRROR);
		strReadString = strReadString + m_structDTCDevValues.m_strMirrorDev;

		// ardeb 020912 v
		if ( m_szDeviceListInfo[m_iMirrorIndexArray[i - 1]][0] < 48 )
		{
			lbValid3Val = FALSE;
		}
		// ardeb 020912 ^
		strReadString = strReadString + " ";
		strReadString = strReadString + m_szDeviceListInfo[m_iMirrorIndexArray[i - 1]];
		m_iMirrorDevIndex++;

		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

	}
	//
	// End Profile Section

	// End DTC devices

	strReadString.LoadString(IDS_STRING_END_CONFIG);
	strReadString = strReadString + QNM " Configuration File: "; 
	strReadString = strReadString + m_structDevCfg.strCfgFileInUse;
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

	// Make call to set the tunable params "DTCSet"

	fclose(file);

	// ardeb 020912 v
	if ( lbValid3Val )
	{
		CopyFile ( lszFileInUse, m_structDevCfg.strCfgFileInUse, FALSE );
	}
	else
	{
		CString lszMsg;
		lszMsg.Format (
			"Can not save file %.*s because\n"
			"the mirror disk informations are unaccessible.\n", 
			1024, m_structDevCfg.strCfgFileInUse
		);

		AfxMessageBox ( lszMsg );
	}

	DeleteFile ( lszFileInUse );
	// ardeb 020912 ^

} // CConfig::writeConfigFile ()


void CConfig::parseFile(char *pdest, char strReadValue[_MAX_PATH])
{
	char	chRead = 0;
	int		i = 0;

	memset(strReadValue, 0, _MAX_PATH);

	while(chRead != 13)
	{
		chRead = *pdest;
		strReadValue[i] = chRead;
		i++;
		pdest++;
	}
	strReadValue[i - 1] = 0;
}

void CConfig::readSysValues(CFile *file, int iFileSize)
{
	char	strReadValue[_MAX_PATH];
	char	*strFileReadBuffer;
	char	*pdest;
	char	strNotes[] = "NOTES:  ";
	char	strHost[] = "HOST:                ";
	char	strPStore[] = "PSTORE:              ";
	char	strJournal[] = "JOURNAL:             ";
	char	strChaining[] = "CHAINING:            ";
	char	strSecondPort[] = "SECONDARY-PORT:      ";

	strFileReadBuffer = new char[iFileSize];

	file->Read(strFileReadBuffer, iFileSize);

	// NOTES
	pdest = strstr(strFileReadBuffer, strNotes);
	if(pdest)
	{
		pdest = pdest + sizeof(strNotes) - 1;
		parseFile(pdest, strReadValue);
		memset(m_structSystemValues.m_strNote, 0, sizeof(m_structSystemValues.m_strNote));
		strcpy(m_structSystemValues.m_strNote, strReadValue);
	}
	// END NOTES

	// PRIMARY HOST
	pdest = strstr(strFileReadBuffer, strHost);
	if(pdest)
	{
		pdest = pdest + sizeof(strHost) - 1;
		parseFile(pdest, strReadValue);
		memset(m_structSystemValues.m_strPrimaryHostName, 0, sizeof(m_structSystemValues.m_strPrimaryHostName));
		strcpy(m_structSystemValues.m_strPrimaryHostName, strReadValue);
	}
	// END PRIMARY HOST

	// PSTORE
	pdest = strstr(strFileReadBuffer, strPStore);
	if(pdest)
	{
		char	strPStoreTemp[_MAX_PATH];
		memset(strPStoreTemp, 0, sizeof(strPStoreTemp));

		pdest = pdest + sizeof(strPStore) - 1;
		parseFile(pdest, strReadValue);
		// [020712] AlRo v
		// lala chupusur la
		//sprintf(strPStoreTemp, "%c:", strReadValue[4]);
		sprintf(strPStoreTemp, "%.*s", _MAX_PATH-1, strReadValue );
		// [020712] AlRo ^
		// ardeb
		memset(m_structSystemValues.m_strPStoreDev, 0, sizeof(m_structSystemValues.m_strPStoreDev));
		strcpy(m_structSystemValues.m_strPStoreDev, strPStoreTemp);
	}
	// END PSTORE

	// SECONDARY HOST
	pdest = strstr(strFileReadBuffer + 500, strHost);
	if(pdest)
	{
		pdest = pdest + sizeof(strHost) - 1;
		parseFile(pdest, strReadValue);
		memset(m_structSystemValues.m_strSecondHostName, 0, sizeof(m_structSystemValues.m_strSecondHostName));
		strcpy(m_structSystemValues.m_strSecondHostName, strReadValue);
	}
	// END SECONDARY HOST

	// JOURNAL
	pdest = strstr(strFileReadBuffer, strJournal);
	if(pdest)
	{
		pdest = pdest + sizeof(strJournal) - 1;
		parseFile(pdest, strReadValue);
		memset(m_structSystemValues.m_strJournalDir, 0, sizeof(m_structSystemValues.m_strJournalDir));
		strcpy(m_structSystemValues.m_strJournalDir, strReadValue);
	}
	// END JOURNAL

	// CHAINING
	pdest = strstr(strFileReadBuffer, strChaining);
	if(pdest)
	{
		pdest = pdest + sizeof(strChaining) - 1;
		parseFile(pdest, strReadValue);
		if(strcmp(strReadValue, "on") == 0)
		{
			m_structSystemValues.m_bChaining = TRUE;
		}
		else
		{
			m_structSystemValues.m_bChaining = FALSE;
		}
	}
	// END CHAINING

	// SECOND PORT
	pdest = strstr(strFileReadBuffer, strSecondPort);
	if(pdest)
	{
		pdest = pdest + sizeof(strSecondPort) - 1;
		parseFile(pdest, strReadValue);
		m_structSystemValues.m_iPortNum = atoi(strReadValue);
		if(m_structSystemValues.m_iPortNum == 0)
		{
			getRegPort((DWORD *)&m_structSystemValues.m_iPortNum);
		}
	}
	// SECOND PORT

	// [020712] AlRo v
	//delete strFileReadBuffer;
	delete[] strFileReadBuffer;
	// [020712] AlRo ^
}

void CConfig::readDTCDevices(CFile *file, int iFileSize)
{
	char	strReadValue[_MAX_PATH];
	char	*ptr, *strFileReadBuffer;
	char	*pdest;
	char	strRemark[] = "REMARK:  ";
	char	strDataCastDevDev[] = CAPPRODUCTNAME "-DEVICE:  ";
	char	strDataDisk[] = "DATA-DISK:        ";
	char	strMirror[] = "MIRROR-DISK:      ";

	
	ptr = new char[iFileSize];
	strFileReadBuffer = ptr;

	file->Read(strFileReadBuffer, iFileSize);

	m_listStructDTCDevValues.RemoveAll();

	while(strFileReadBuffer != NULL)
	{
		// REMARKS
		pdest = strstr(strFileReadBuffer, strRemark);
		if(pdest)
		{
			pdest = pdest + sizeof(strRemark) - 1;
			parseFile(pdest, strReadValue);
			memset(m_structDTCDevValues.m_strRemarks, 0, sizeof(m_structDTCDevValues.m_strRemarks));
			strcpy(m_structDTCDevValues.m_strRemarks, strReadValue);
		}
		// END REMARKS	

		// OPEN STORAGE DEVICE
		pdest = strstr(strFileReadBuffer, strDataCastDevDev);
		if(pdest)
		{
			pdest = pdest + sizeof(strDataCastDevDev) - 1;
			parseFile(pdest, strReadValue);
			memset(m_structDTCDevValues.m_strDTCDev, 0, sizeof(m_structDTCDevValues.m_strDTCDev));
			strcpy(m_structDTCDevValues.m_strDTCDev, strReadValue);
		}
		// END OPEN STORAGE DEVICE	

		// DATA DISK
		pdest = strstr(strFileReadBuffer, strDataDisk);
		if(pdest)
		{
			pdest = pdest + sizeof(strDataDisk) - 1;
			parseFile(pdest, strReadValue);
			memset(m_structDTCDevValues.m_strDataDev, 0, sizeof(m_structDTCDevValues.m_strDataDev));
			sprintf(m_structDTCDevValues.m_strDataDev, "%c%c", strReadValue[0], strReadValue[1]);
			_snprintf ( m_structDTCDevValues.m_strPri3Val, _MAX_PATH, &(strReadValue[3]) ); // ardeb 020913
		}
		// END DATA DISK

		// MIRROR DISK
		pdest = strstr(strFileReadBuffer, strMirror);
		if(pdest)
		{
			pdest = pdest + sizeof(strMirror) - 1;
			parseFile(pdest, strReadValue);
			memset(m_structDTCDevValues.m_strMirrorDev, 0, sizeof(m_structDTCDevValues.m_strMirrorDev));
			sprintf(m_structDTCDevValues.m_strMirrorDev, "%c%c", strReadValue[0], strReadValue[1]);
			_snprintf ( m_structDTCDevValues.m_strSec3Val, _MAX_PATH, &(strReadValue[3]) ); // ardeb 020913
		}
		// END MIRROR DISK
		
		// Add to linked list
		m_listStructDTCDevValues.AddTail(m_structDTCDevValues);

		// move ptr
		strFileReadBuffer = pdest;

	} // while
	m_listStructDTCDevValues.RemoveTail();
	delete ptr;
}

void CConfig::readThrottleInfo(CFile *file, int iFileSize)
{
	char	strReadValue[_MAX_PATH];
	char	*strFileReadBuffer;
	char	*strStart;
	char	*pdest;
	char	strAction[] = "ACTION: ";

	strFileReadBuffer = new char[iFileSize];
	strStart = strFileReadBuffer;

	file->Read(strFileReadBuffer, iFileSize);
	
	memset(m_structThrottleValues.m_strThrottle, 0, sizeof(m_structThrottleValues.m_strThrottle));
	pdest = strstr(strFileReadBuffer, strAction);
	while(pdest != NULL)
	{
		pdest = pdest + sizeof(strAction) - 1;
		parseFile(pdest, strReadValue);
		strcat(strReadValue, "\r\n");
		strcat(m_structThrottleValues.m_strThrottle, strReadValue);

		strFileReadBuffer = pdest;
		pdest = strstr(strFileReadBuffer, strAction);
	}

	strFileReadBuffer = strStart;
	delete strFileReadBuffer;
}

int CConfig::readConfigFile()
{
	CFile	file;
	int		iFileSize = 0;
	BOOL	bFileOpen;
	int		iRc = 1;

	bFileOpen = file.Open(m_structDevCfg.strCfgFileInUse, CFile::modeReadWrite, NULL);
	if(bFileOpen)
		iFileSize = file.GetLength();
	else
		iRc = 0;
	
	if(iFileSize > 0)
	{
		readSysValues(&file, iFileSize);
		file.SeekToBegin( );
		// this will have to work for mult profiles
		readDTCDevices(&file, iFileSize);
		file.SeekToBegin( );
//		readThrottleInfo(&file, iFileSize);
		file.SeekToBegin( );

		// Make a backup of the initial values to know if they change before
		// rewriting the file.  ardeb 020913
		BakInitVal();
	}

	if(bFileOpen)
		file.Close();

	return iRc;
}

void CConfig::readNote(char *strPath, char *strGroupID, char *strNote)
{
	char	strFileName[_MAX_PATH];
	char	strReadValue[_MAX_PATH];
	CFile	file;
	int		iFileSize = 0;
	char	*strFileReadBuffer;
	char	*pdest;
	char	strNotes[] = "NOTES:  ";

	memset(strFileName, 0, sizeof(strFileName));
	sprintf(strFileName, "%sp%s.cfg", strPath, strGroupID);

	file.Open(strFileName, CFile::modeReadWrite, NULL);
	iFileSize = file.GetLength();

	strFileReadBuffer = new char[iFileSize];

	file.Read(strFileReadBuffer, iFileSize);

	// NOTES
	pdest = strstr(strFileReadBuffer, strNotes);
	if(pdest)
	{
		pdest = pdest + sizeof(strNotes) - 1;
		parseFile(pdest, strReadValue);
		memset(strNote, 0, _MAX_PATH);
		strcpy(strNote, strReadValue);
	}
	// END NOTES

	file.Close();
	delete strFileReadBuffer;
}

void CConfig::waitForProcessEnd(PROCESS_INFORMATION *ProcessInfo)
{
	DWORD	dwExitCode = STILL_ACTIVE;
	
	while(dwExitCode == STILL_ACTIVE)
	{
		GetExitCodeProcess(ProcessInfo->hProcess, &dwExitCode);
		Sleep(1);
	}
}

void CConfig::setTunablesInPStore()
{
	char	strFtdStartPath[_MAX_PATH];
	char	strCmdLineArgs[_MAX_PATH];
	PROCESS_INFORMATION ProcessInfo;
		
	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);


	// Set CHUNKSIZE
	memset(strFtdStartPath, 0, sizeof(strFtdStartPath));
	memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
	getConfigPath(strFtdStartPath);
	strcat(strFtdStartPath, QNM);
	strcat(strFtdStartPath, "set.exe");
/* Do not do this here
	sprintf(strCmdLineArgs, "%sset -g %d CHUNKSIZE=%d", QNM, m_structDevCfg.iGroupID, 
			m_structDevCfg.iBABSize);
	CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
					CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

	waitForProcessEnd(&ProcessInfo);
*/

	// Set SYNCMODE
	int iSyncMode = 0;
	memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
	if(m_structTunableParamsValues.m_bSyncMode)
		iSyncMode = 1;
	sprintf(strCmdLineArgs, "%sset -g %d SYNCMODE=%d", QNM, m_structDevCfg.iGroupID, iSyncMode);
	CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
					CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

	waitForProcessEnd(&ProcessInfo);

	if(m_structTunableParamsValues.m_bSyncMode)
	{
		// Set SYNCMODEDEPTH
		memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
		sprintf(strCmdLineArgs, "%sset -g %d SYNCMODEDEPTH=%d", QNM, m_structDevCfg.iGroupID,
				m_structTunableParamsValues.m_iDepth);
		CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
						CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

		waitForProcessEnd(&ProcessInfo);

		// Set SYNCMODETIMEOUT
		memset(strCmdLineArgs,  0, sizeof(strCmdLineArgs));
		sprintf(strCmdLineArgs, "%sset -g %d SYNCMODETIMEOUT=%d", QNM, m_structDevCfg.iGroupID,
				m_structTunableParamsValues.m_iTimeout);
		CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
						CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

		waitForProcessEnd(&ProcessInfo);
	}


/*
	int iLogStats = 0;
	if(m_structTunableParamsValues.m_bStatGen)
	{
		// Set LOGSTATS
		iLogStats = 1;
		memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
		sprintf(strCmdLineArgs, "%s -g %d LOGSTATS=%d", strSet, m_structDevCfg.iGroupID, 
				iLogStats);
		CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
						CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

		waitForProcessEnd(&ProcessInfo);

		// Set STATINTERVAL
		memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
		sprintf(strCmdLineArgs, "%s -g %d STATINTERVAL=%d", strSet, m_structDevCfg.iGroupID,
				m_structTunableParamsValues.m_iUpdateInterval);
		CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
						CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

		waitForProcessEnd(&ProcessInfo);

		// Set MAXSTATFILESIZE
		memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
		sprintf(strCmdLineArgs, "%s -g %d MAXSTATFILESIZE=%d", strSet, m_structDevCfg.iGroupID,
				m_structTunableParamsValues.m_iMaxStatFileSize);
		CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
						CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

		waitForProcessEnd(&ProcessInfo);

	}
	else
	{
		// Set LOGSTATS
		memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
		sprintf(strCmdLineArgs, "%s -g %d LOGSTATS=%d", strSet, m_structDevCfg.iGroupID,
				iLogStats);
		CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
						CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

		waitForProcessEnd(&ProcessInfo);

	}
*/
	int	iCompression = 0;
	if(m_structTunableParamsValues.m_bCompression)
	{
		iCompression = 1;
	}
	// Set COMPRESSION
	memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
	sprintf(strCmdLineArgs, "%sset -g %d COMPRESSION=%d", QNM, m_structDevCfg.iGroupID,
			iCompression);
	CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
					CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

	waitForProcessEnd(&ProcessInfo);

	// Set NETMAXKBPS
/*
	if(m_structTunableParamsValues.m_bNetThresh)
	{
		memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
		sprintf(strCmdLineArgs, "%s -g %d NETMAXKBPS=%d", strSet, m_structDevCfg.iGroupID,
				m_structTunableParamsValues.m_iMaxTranRate);
		CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
						CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

		waitForProcessEnd(&ProcessInfo);

	}
*/
	// Set IODelay
/*
	if(m_structTunableParamsValues.m_iDelayWrites)
	{
		memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
		sprintf(strCmdLineArgs, "%s -g %d IODELAY=%d", strSet, m_structDevCfg.iGroupID,
				m_structTunableParamsValues.m_iDelayWrites);
		CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
						CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

		waitForProcessEnd(&ProcessInfo);
	}
*/
	// Set Refresh interval
	if(m_structTunableParamsValues.m_liRefreshInterval)
	{
		memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
		sprintf(strCmdLineArgs, "%sset -g %d REFRESHTIMEOUT=%d", QNM, m_structDevCfg.iGroupID,
				m_structTunableParamsValues.m_liRefreshInterval);
		CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
						CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

		waitForProcessEnd(&ProcessInfo);
	}

}

void CConfig::setStartGroup()
{
	char	strFtdStartPath[_MAX_PATH];
	char	strCmdLineArgs[_MAX_PATH];
	PROCESS_INFORMATION ProcessInfo;
	
	memset(strFtdStartPath, 0, sizeof(strFtdStartPath));
	memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
	getConfigPath(strFtdStartPath);
	strcat(strFtdStartPath, QNM);
	strcat(strFtdStartPath, "start.exe");
	sprintf(strCmdLineArgs, "%sstart -g %d", QNM, m_structDevCfg.iGroupID);

	
	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

	// FtdStart
	CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
					CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

	waitForProcessEnd(&ProcessInfo);
}

void CConfig::stopGroup()
{
	char	strFtdStartPath[_MAX_PATH];
	char	strCmdLineArgs[_MAX_PATH];
	PROCESS_INFORMATION ProcessInfo;
	
	memset(strFtdStartPath, 0, sizeof(strFtdStartPath));
	memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
	getConfigPath(strFtdStartPath);
	strcat(strFtdStartPath, QNM);
	strcat(strFtdStartPath, "killpmd.exe");
	sprintf(strCmdLineArgs, "%skillpmd -g %d", QNM, m_structDevCfg.iGroupID);

	
	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

	// FtdKillPmd
	CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
					CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

	waitForProcessEnd(&ProcessInfo);

	memset(strFtdStartPath, 0, sizeof(strFtdStartPath));
	memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));
	getConfigPath(strFtdStartPath);
	strcat(strFtdStartPath, QNM);
	strcat(strFtdStartPath, "stop.exe");
	sprintf(strCmdLineArgs, "%sstop -g %d", QNM, m_structDevCfg.iGroupID);

	
	// FtdStop
	CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
					CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

	waitForProcessEnd(&ProcessInfo);


}

void CConfig::getTunablesFromPStore()
{
	char	strFtdStartPath[_MAX_PATH];
	char	strCmdLineArgs[_MAX_PATH];
	char	strSet[] = QNM "set";
	char	strSetExe[] = QNM "set.exe";
	PROCESS_INFORMATION ProcessInfo;

	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

	memset(strFtdStartPath, 0, sizeof(strFtdStartPath));
	memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));

	getConfigPath(strFtdStartPath);
	strcat(strFtdStartPath, strSetExe);

	sprintf(strCmdLineArgs, "%s -o -g %d", strSet, m_structDevCfg.iGroupID);

	//dtcset -g <number> 
	CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
				  CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

	waitForProcessEnd(&ProcessInfo);
	
	// Have to get the values set and save them in lpConfig->m_structTunableParamsValues

	// OnInit in CTunableParams set all values

	// Get tunables from file
	m_structTunableParamsValues.m_bCompression = TRUE;
	m_structTunableParamsValues.m_bNetThresh = TRUE;
	m_structTunableParamsValues.m_bStatGen = TRUE;
	m_structTunableParamsValues.m_bSyncMode = TRUE;
	m_structTunableParamsValues.m_iDelayWrites = 0;
	m_structTunableParamsValues.m_iDepth = 0;
	m_structTunableParamsValues.m_iMaxStatFileSize = 0;
	m_structTunableParamsValues.m_iMaxTranRate = 0;
	m_structTunableParamsValues.m_iTimeout = 0;
	m_structTunableParamsValues.m_iUpdateInterval = 0;
	m_structTunableParamsValues.m_iUpdateInterval = 0;
	m_structTunableParamsValues.m_liRefreshInterval = 1;

	char	strChunkSize[] = "CHUNKSIZE: ";
	char	strChunkDelay[] = "CHUNKDELAY: ";
	char	strSyncMode[] = "SYNCMODE: ";
	char	strSyncModeDepth[] = "SYNCMODEDEPTH: ";
	char	strSyncModeTimeOut[] = "SYNCMODETIMEOUT: ";
	char	strIODelay[] = "IODELAY: ";
	char	strNetMaxBps[] = "NETMAXKBPS: ";
	char	strStatInterval[] = "STATINTERVAL: ";
	char	strMaxStatFileSize[] = "MAXSTATFILESIZE: ";
	char	strLogStats[] = "LOGSTATS: ";
	char	strTraceThrottle[] = "TRACETHROTTLE: ";
	char	strCompression[] = "COMPRESSION: ";
	char	strRefreshInt[] = "REFRESHTIMEOUT: ";
	CFile	file;
	int		iFileSize;
	char	*strFileReadBuffer, *pdest;
	char	strReadValue[_MAX_PATH];
	char	strPath[_MAX_PATH];

	getConfigPath(strPath);
	strcat(strPath, "pstore.txt");
	BOOL bIsOpen = file.Open(strPath, CFile::modeRead, NULL);
	if(!bIsOpen)
		file.Open(strPath, CFile::modeCreate, NULL);
	iFileSize = file.GetLength();

	if(iFileSize <= 0 && bIsOpen)
	{
		char szError[_MAX_PATH];

		memset(szError, 0, sizeof(szError));
		sprintf(szError, "Failed to read Tunable Parameters for group %03d pstore %s\\.\nUsing default values!",
					m_structDevCfg.iGroupID, m_structSystemValues.m_strPStoreDev);
		::MessageBox(NULL, szError, PRODUCTNAME, MB_OK|MB_ICONWARNING);
	}

	strFileReadBuffer = new char[iFileSize];

	file.Read(strFileReadBuffer, iFileSize);

	// compression
	pdest = strstr(strFileReadBuffer, strCompression);
	if(pdest)
	{
		pdest = pdest + sizeof(strCompression) - 1;
		parseFile(pdest, strReadValue);
		m_structTunableParamsValues.m_bCompression = atoi(strReadValue);
	}
	// end compression

	// Sync Mode
	pdest = strstr(strFileReadBuffer, strSyncMode);
	if(pdest)
	{
		pdest = pdest + sizeof(strSyncMode) - 1;
		parseFile(pdest, strReadValue);
		m_structTunableParamsValues.m_bSyncMode = atoi(strReadValue);
	}
	// end Sync Mode

	// Sync Mode
	pdest = strstr(strFileReadBuffer, strNetMaxBps);
	if(pdest)
	{
		pdest = pdest + sizeof(strNetMaxBps) - 1;
		parseFile(pdest, strReadValue);
		m_structTunableParamsValues.m_bNetThresh = atoi(strReadValue);
	}
	// end Sync Mode

	// Sync Mode depth
	pdest = strstr(strFileReadBuffer, strSyncModeDepth);
	if(pdest)
	{
		pdest = pdest + sizeof(strSyncModeDepth) - 1;
		parseFile(pdest, strReadValue);
		m_structTunableParamsValues.m_iDepth = atoi(strReadValue);
	}
	// end Sync Mode depth

	// Sync Mode timeout
	pdest = strstr(strFileReadBuffer, strSyncModeTimeOut);
	if(pdest)
	{
		pdest = pdest + sizeof(strSyncModeTimeOut) - 1;
		parseFile(pdest, strReadValue);
		m_structTunableParamsValues.m_iTimeout = atoi(strReadValue);
	}
	// end Sync Mode timeout

	// Stat Gen interval
	pdest = strstr(strFileReadBuffer, strStatInterval);
	if(pdest)
	{
		pdest = pdest + sizeof(strStatInterval) - 1;
		parseFile(pdest, strReadValue);
		m_structTunableParamsValues.m_iUpdateInterval = atoi(strReadValue);
	}
	// end Stat Gen interval

	// Stat Gen Max size
	pdest = strstr(strFileReadBuffer, strMaxStatFileSize);
	if(pdest)
	{
		pdest = pdest + sizeof(strMaxStatFileSize) - 1;
		parseFile(pdest, strReadValue);
		m_structTunableParamsValues.m_iMaxStatFileSize = atoi(strReadValue);
	}
	// end Stat Gen max size

	// Net Thresh
	pdest = strstr(strFileReadBuffer, strNetMaxBps);
	if(pdest)
	{
		pdest = pdest + sizeof(strNetMaxBps) - 1;
		parseFile(pdest, strReadValue);
		m_structTunableParamsValues.m_iMaxTranRate = atoi(strReadValue);
	}
	// end Net Thresh

	// Delay Writes
	pdest = strstr(strFileReadBuffer, strIODelay);
	if(pdest)
	{
		pdest = pdest + sizeof(strIODelay) - 1;
		parseFile(pdest, strReadValue);
		m_structTunableParamsValues.m_iDelayWrites = atoi(strReadValue);
	}
	// end Delay Writes

	// Refresh Interval
	pdest = strstr(strFileReadBuffer, strRefreshInt);
	if(pdest)
	{
		pdest = pdest + sizeof(strRefreshInt) - 1;
		parseFile(pdest, strReadValue);
		m_structTunableParamsValues.m_liRefreshInterval = atoi(strReadValue);
	}
	// Refresh Interval

	file.Close();

	delete strFileReadBuffer;
}


int CConfig::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void CConfig::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{

	CWnd::OnActivate(nState, pWndOther, bMinimized);
}
///////////////////////////////////////////////////////////////////////////////////

// ardeb 020913
BOOL CConfig::Is3ValValid ()
{
	POSITION head          = m_listStructDTCDevValues.GetHeadPosition();
	int      iDTCListCount = m_listStructDTCDevValues.GetCount();

	for(int i = 1; i <= iDTCListCount; i++)
	{
		m_structDTCDevValues    = m_listStructDTCDevValues.GetNext(head);

		if ( m_szDeviceListInfo[m_iMirrorIndexArray[i - 1]] [ 0 ] < 48 )
		{
			return FALSE;
		}
	}

	return TRUE;

} // CConfig::Is3ValValid ()


// ardeb 020913
BOOL CConfig::IsValChanged ()
{
	if ( strncmp( m_structDevCfgCpy.strGroupNote, 
		          m_structDevCfg.strGroupNote, _MAX_PATH ) != 0 
				)
	{
		return TRUE;
	}
	else if ( strncmp( m_structSystemValuesCpy.m_strNote,
					   m_structSystemValues.m_strNote, _MAX_PATH ) != 0 
			)
	{
		return TRUE;
	}
	else if ( strncmp( m_structSystemValuesCpy.m_strPrimaryHostName, 
					   m_structSystemValues.m_strPrimaryHostName, _MAX_PATH ) != 0 
			)
	{
		return TRUE;
	}
	else if ( strncmp( m_structSystemValuesCpy.m_strPStoreDev,
					   m_structSystemValues.m_strPStoreDev, _MAX_PATH ) != 0 
			)
	{
		return TRUE;
	}
	else if ( strncmp( m_structSystemValuesCpy.m_strSecondHostName, 
					   m_structSystemValues.m_strSecondHostName, _MAX_PATH ) != 0 
			)
	{
		return TRUE;
	}
	else if ( strncmp( m_structSystemValuesCpy.m_strJournalDir, 
					   m_structSystemValues.m_strJournalDir, _MAX_PATH ) != 0 
			)
	{
		return TRUE;
	}
	else if ( m_structSystemValuesCpy.m_iPortNum != m_structSystemValues.m_iPortNum )
	{
		return TRUE;
	}

	int iDTCListCountCpy = m_listStructDTCDevValuesCpy.GetCount();
	int iDTCListCount    = m_listStructDTCDevValues.GetCount();

	if ( iDTCListCount != iDTCListCountCpy )
	{
		return TRUE;
	}

	POSITION headCpy = m_listStructDTCDevValuesCpy.GetHeadPosition();
	POSITION head    = m_listStructDTCDevValues.GetHeadPosition();

	struct structDTCDevValues l_structDTCDevValuesCpy;

	for(int i = 1; i <= iDTCListCount; i++)
	{
		l_structDTCDevValuesCpy = m_listStructDTCDevValuesCpy.GetNext(headCpy);
		m_structDTCDevValues    = m_listStructDTCDevValues.GetNext(head);

		if ( strncmp( l_structDTCDevValuesCpy.m_strRemarks, 
					  m_structDTCDevValues.m_strRemarks, _MAX_PATH ) != 0 
					)
		{
			return TRUE;
		}
		else if ( strncmp( l_structDTCDevValuesCpy.m_strDataDev,
						   m_structDTCDevValues.m_strDataDev, _MAX_PATH ) != 0 
				)
		{
			return TRUE;
		}
		else if ( strncmp( l_structDTCDevValuesCpy.m_strMirrorDev, 
						   m_structDTCDevValues.m_strMirrorDev, _MAX_PATH ) != 0 
				)
		{
			return TRUE;
		}
		else if ( strncmp( l_structDTCDevValuesCpy.m_strDTCDev,
						   m_structDTCDevValues.m_strDTCDev, _MAX_PATH ) != 0 
				)
		{
			return TRUE;
		}
		else if ( m_szDeviceListInfo[m_iMirrorIndexArray[i - 1]] [ 0 ] >= 48 )
		{
			if ( strncmp( l_structDTCDevValuesCpy.m_strSec3Val, 
							   m_szDeviceListInfo[m_iMirrorIndexArray[i - 1]], _MAX_PATH ) != 0 
			   )
			{
				return TRUE;
			}
		}		
	}

	return FALSE;

} // CConfig::IsValChanged ()


void CConfig::setSocketConnection(char *szReceiveLine, int iRecBufferSize)
{
	char				lhostname[_MAX_PATH], rhostname[_MAX_PATH];
	int					port;
//	char				*szSendLine = "ftd get all devices\n", buf[256];
	char				szSendLine[25], buf[256];
	int					iBufferLen;
	int					iRc = 0;
    /* [02-10-10] ac V 
	int					iIP1,iIP2,iIP3,iIP4;
    */

	memset(szSendLine, 0, sizeof(szSendLine));
	sprintf(szSendLine, "ftd get all devices %d\n", lpConfig->m_structDevCfg.iGroupID);
	
	// Get secondary machine name
	CPropertyPage *pPropSheet;
	pPropSheet = lpConfig->m_sheetConfig->GetPage(0);

    /* [02-10-10] ac V 
	iIP1 = pPropSheet->GetDlgItemInt( IDC_EDIT_PRIMARY_IP1, NULL, FALSE );
	iIP2 = pPropSheet->GetDlgItemInt( IDC_EDIT_PRIMARY_IP2, NULL, FALSE );
	iIP3 = pPropSheet->GetDlgItemInt( IDC_EDIT_PRIMARY_IP3, NULL, FALSE );
	iIP4 = pPropSheet->GetDlgItemInt( IDC_EDIT_PRIMARY_IP4, NULL, FALSE );

	memset(lhostname, 0, sizeof(lhostname));
	sprintf(lhostname, "%03i.%03i.%03i.%03i", iIP1, iIP2, iIP3, iIP4);
    ac ^ */
    pPropSheet->GetDlgItemText( IDC_EDIT_PRI_HOST_OR_IP, lhostname, sizeof(lhostname) );

    /* [02-10-10] ac V 
	iIP1 = pPropSheet->GetDlgItemInt( IDC_EDIT_SECONDARY_IP1, NULL, FALSE );
	iIP2 = pPropSheet->GetDlgItemInt( IDC_EDIT_SECONDARY_IP2, NULL, FALSE );
	iIP3 = pPropSheet->GetDlgItemInt( IDC_EDIT_SECONDARY_IP3, NULL, FALSE );
	iIP4 = pPropSheet->GetDlgItemInt( IDC_EDIT_SECONDARY_IP4, NULL, FALSE );

	memset(rhostname, 0, sizeof(rhostname));
	sprintf(rhostname, "%03i.%03i.%03i.%03i", iIP1, iIP2, iIP3, iIP4);
    ac ^ */
    pPropSheet->GetDlgItemText( IDC_EDIT_SEC_HOST_OR_IP, rhostname, sizeof(rhostname) );

	// ardeb 020919 v
	//port = lpConfig->m_structSystemValues.m_iPortNum;
	port = pPropSheet->GetDlgItemInt ( IDC_EDIT_SECONDARY_PORT, NULL, TRUE );
	// ardeb 020919 ^

	int temp = strlen(rhostname);
	if(strlen(rhostname) == 0)
	{
		::MessageBox(NULL, "No Secondary Machine Specified",  PRODUCTNAME, MB_OK);
	}
	else
	{
		sock_startup();

		sockp = sock_create();

		if(sock_init(sockp, lhostname, rhostname, 0, 0, SOCK_STREAM, AF_INET, 1, 0) < 0) {
			sprintf(buf, "Unable to initialize socket [%s -> %s:%d]: %s.",
				sockp->lhostname,
				sockp->rhostname,
				sockp->port,
				sock_strerror(sock_errno()));
			::MessageBox(NULL, buf, PRODUCTNAME, MB_OK);
		}		

		if(sock_bind(sockp, port) < 0) {
			sprintf(buf, "Socket bind failure [%s:%d]: %s.",
				sockp->lhostname,
				port,
				sock_strerror(sock_errno()));
			::MessageBox(NULL, buf, PRODUCTNAME, MB_OK);
		}

		sockp->port = port;
		sockp->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		int	sockerr;
		if(sock_connect_nonb(sockp, port, 3, 0, &sockerr) < 0) {
		//if(sock_connect(sockp, port) < 0) {
			sprintf(buf, "Unable to establish a connection with the remote [%s:%d]: %s.",
				sockp->rhostname,
				sockp->port,
				sock_strerror(sockerr));
			::MessageBox(NULL, buf, PRODUCTNAME, MB_OK);
		}

		sock_set_b(sockp);

		iBufferLen = strlen(szSendLine);

		sock_send(sockp, szSendLine, iBufferLen);

		memset(szReceiveLine, 0, sizeof(szReceiveLine));
		
		sock_recv(sockp, szReceiveLine, iRecBufferSize);
	}
}


// Make a backup of the initial values to know if they change before
// rewriting the file.  ardeb 020913
void CConfig::BakInitVal ()
{
	// Because MicroSoft strncpy is not standard any more with ANSI
	// we will use _snprintf
	_snprintf ( m_structDevCfgCpy.strGroupNote, _MAX_PATH, m_structDevCfg.strGroupNote );

	_snprintf ( m_structSystemValuesCpy.m_strNote, 
		_MAX_PATH, m_structSystemValues.m_strNote );

	_snprintf ( m_structSystemValuesCpy.m_strPrimaryHostName, 
		_MAX_PATH, m_structSystemValues.m_strPrimaryHostName );

	_snprintf ( m_structSystemValuesCpy.m_strPStoreDev, 
		_MAX_PATH, m_structSystemValues.m_strPStoreDev );

	_snprintf ( m_structSystemValuesCpy.m_strSecondHostName, 
		_MAX_PATH, m_structSystemValues.m_strSecondHostName );

	_snprintf ( m_structSystemValuesCpy.m_strJournalDir, 
		_MAX_PATH, m_structSystemValues.m_strJournalDir );

	m_structSystemValuesCpy.m_iPortNum = m_structSystemValues.m_iPortNum;

	m_listStructDTCDevValuesCpy.RemoveAll();

	int      iDTCListCount = m_listStructDTCDevValues.GetCount();
	POSITION head          = m_listStructDTCDevValues.GetHeadPosition();
	for(int i = 1; i <= iDTCListCount; i++)
	{
		m_structDTCDevValues = m_listStructDTCDevValues.GetNext(head);
		m_listStructDTCDevValuesCpy.AddTail(m_structDTCDevValues);
	}

} // CConfig::BakInitVal ()

	
void CConfig::disconnectSocket()
{
	if(sockp)
	{
		sock_disconnect(sockp);
		sock_delete(&sockp);

		tcp_cleanup();
	}
}

void	CConfig::updateCurrentmirror()
{
	int			n, index = 0;
	char		strDrive[_MAX_PATH];
	char		szReceiveInfo[3 * 1024];
	CString		strLowerWinDir = "";
	int			iDeviceCount = 0;
	int			iDeviceIndexByChar = 0;
	int			iDeviceIndex = 0;

	memset(m_szRemoteDevices, 0, sizeof(m_szRemoteDevices));
	memset(m_iMirrorIndexArray, 0, sizeof(m_iMirrorIndexArray));
	lpConfig->m_iMirrorDevIndex = 0;

	//Socket
	memset(szReceiveInfo, 0, sizeof(szReceiveInfo));
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	setSocketConnection(szReceiveInfo, sizeof(szReceiveInfo));
	disconnectSocket();
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	//End Socket

	if(strlen(szReceiveInfo) > 0)
	{
		// Fill in remote devices
		memset(m_szDeviceListInfo, 0, sizeof(m_szDeviceListInfo));

		memset(strDrive, 0, sizeof(strDrive));
		while(szReceiveInfo[index] != 0)
		{
			n = 0;
			while(szReceiveInfo[index] != '{')
			{
				strDrive[n] = szReceiveInfo[index];
				index++;
				n++;
			}
			
			index = index + 2;
			iDeviceIndexByChar = 0;
			while(szReceiveInfo[index] != '\\')
			{
				m_szDeviceListInfo[iDeviceIndex] [iDeviceIndexByChar] = szReceiveInfo[index];

				iDeviceIndexByChar++;
				index++;
			}
			
			iDeviceIndex++;

			m_szRemoteDevices[iDeviceIndex - 1] = strDrive[2];
			
			index++;
			memset(strDrive, 0, sizeof(strDrive));
		}

		for(int i = 0; (unsigned int)i < strlen(lpConfig->m_szCurrentDevices); i++)
		{
			for(int j = 0; (unsigned int)j < strlen(m_szRemoteDevices); j++)
			{
				if(lpConfig->m_szCurrentDevices[i] == m_szRemoteDevices[j])
				{
					lpConfig->m_iMirrorIndexArray[lpConfig->m_iMirrorDevIndex] = j;
					lpConfig->m_iMirrorDevIndex++;
				}
			}
		}

		lpConfig->m_iMirrorIndexArray[lpConfig->m_iMirrorDevIndex] = lpConfig->m_iMirrorDev;
	}
}

void	CConfig::parseIPString(char *szIP, int *iIP1, int *iIP2, int *iIP3, int *iIP4)
{
	char seps[]   = ".\n";
	char *token;
	char *pdest;

	char szTempIP1[_MAX_PATH], szTempIP2[_MAX_PATH];

	memset(szTempIP1, 0, sizeof(szTempIP1));
	memset(szTempIP2, 0, sizeof(szTempIP2));

	strcpy(szTempIP1, szIP);
	strcpy(szTempIP2, szIP);

	// check if the string is a hostname from an old cfg file
	pdest = strchr( szTempIP1, '.' );

	// if pdest is null we got a hostname not an IP
	if(pdest != NULL)
	{
		token = strtok( szTempIP2, seps );
		*iIP1 = atoi(token);

		token = strtok( NULL, seps );
		*iIP2 = atoi(token);

		token = strtok( NULL, seps );
		*iIP3 = atoi(token);

		token = strtok( NULL, seps );
		*iIP4 = atoi(token);
	}
	else
	{
		// if a hostname was passed in from an old cfg file set the ip to 0
		*iIP1 = *iIP2 = *iIP3 = *iIP4 = 0;
	}
}
