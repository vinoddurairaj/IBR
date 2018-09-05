// GroupConfig.cpp: implementation of the CGroupConfig class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "../../tdmf.inc"

#include "ConfigurationTool.h"
#include "GroupConfig.h"
#include "Command.h"

#include "sockerrnum.h"
#include "ftdio.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
//

bool CGroupConfig::RemovePair(int nIndex)
{
	std::vector<CDevicePairConfig>::iterator it = m_vecDevicePair.begin();
	for (int i = 0; (i < nIndex) && (it != m_vecDevicePair.end()); i++)
	{
		it++;
	}

	if (it != m_vecDevicePair.end())
	{
		m_vecDevicePair.erase(it);
	}

	return (it != m_vecDevicePair.end()) ? true : false;
}

int CGroupConfig::AddPair(LPCSTR lpcstrDTCDev,
						  LPCSTR lpcstrRemarks,
						  LPCSTR lpcstrDataDev,
						  LPCSTR lpcstrMirrorDev,
						  int    nNumDTCDevices)
{
	CDevicePairConfig DevicePairConfig;
	DevicePairConfig.m_cstrDTCDev     = lpcstrDTCDev;
	DevicePairConfig.m_cstrRemarks    = lpcstrRemarks;
	DevicePairConfig.m_cstrDataDev    = lpcstrDataDev;
	DevicePairConfig.m_cstrMirrorDev  = lpcstrMirrorDev;
	DevicePairConfig.m_nNumDTCDevices = nNumDTCDevices;

	m_vecDevicePair.push_back(DevicePairConfig);

	return m_vecDevicePair.size() - 1; // zero based index
}

void CGroupConfig::SaveInitialValues()
{
	m_cstrNoteSaved = m_cstrNote;
}

void CGroupConfig::RestoreInitialValues()
{
	m_cstrNote = m_cstrNoteSaved;
	
	// TODO: to save memory: clear uneccesary info (info that will be reread from file)
}

void CGroupConfig::ParseFile(char *pdest, CString& cstrReadValue)
{
	cstrReadValue = "";

	char	chRead = 0;
	int		i = 0;

	while(chRead != 13)
	{
		chRead = *pdest;
		if (chRead != 13)
		{
			cstrReadValue += chRead;
		
			i++;
			pdest++;
		}
	}

	// REmove leading spaces
	cstrReadValue.TrimLeft();
}

void CGroupConfig::ReadNote()
{
	char  strNotes[] = "NOTES:  ";

	CFile file;
	if (file.Open(m_cstrFilename, CFile::modeReadWrite, NULL))
	{
		int nFileSize = (int)file.GetLength();

		char* pszFileReadBuffer = new char[nFileSize];
		file.Read(pszFileReadBuffer, nFileSize);

		// NOTES
		char* pdest = strstr(pszFileReadBuffer, strNotes);
		if(pdest)
		{
			pdest = pdest + sizeof(strNotes) - 1;
			ParseFile(pdest, m_cstrNote);
		}

		file.Close();
		delete pszFileReadBuffer;
	}
}

int CGroupConfig::ReadConfigFile()
{
	int   iRc = 1;
	CFile file;
	
	if (file.Open(m_cstrFilename, CFile::modeReadWrite, NULL))
	{
		int iFileSize = (int)file.GetLength();
		if(iFileSize > 0)
		{
			ReadSysValues(&file, iFileSize);
			file.SeekToBegin( );
			ReadDTCDevices(&file, iFileSize);
		}
		file.Close();
	}
	else
	{
		iRc = 0;
	}
	
	return iRc;
}

void CGroupConfig::ReadSysValues(CFile *file, int iFileSize)
{
	CString cstrReadValue;
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
		ParseFile(pdest, cstrReadValue);
		m_cstrNote = cstrReadValue;
	}
	// END NOTES

	// PRIMARY HOST
	pdest = strstr(strFileReadBuffer, strHost);
	if(pdest)
	{
		pdest = pdest + sizeof(strHost) - 1;
		ParseFile(pdest, cstrReadValue);
		m_cstrPrimaryHost = cstrReadValue;
	}
	// END PRIMARY HOST

	// PSTORE
	pdest = strstr(strFileReadBuffer, strPStore);
	if(pdest)
	{
		char strPStoreTemp[_MAX_PATH];
		memset(strPStoreTemp, 0, sizeof(strPStoreTemp));

		pdest = pdest + sizeof(strPStore) - 1;
		ParseFile(pdest, cstrReadValue);
		sprintf(strPStoreTemp, "%.*s", _MAX_PATH-1, cstrReadValue);

		m_cstrPStore = strPStoreTemp;
	}
	// END PSTORE

	// SECONDARY HOST
	pdest = strstr(strFileReadBuffer + 500, strHost);
	if(pdest)
	{
		pdest = pdest + sizeof(strHost) - 1;
		ParseFile(pdest, cstrReadValue);
		m_cstrSecondaryHost = cstrReadValue;
	}
	// END SECONDARY HOST

	// JOURNAL
	pdest = strstr(strFileReadBuffer, strJournal);
	if(pdest)
	{
		pdest = pdest + sizeof(strJournal) - 1;
		ParseFile(pdest, cstrReadValue);
		m_cstrJournal = cstrReadValue;
	}
	// END JOURNAL

	// CHAINING
	pdest = strstr(strFileReadBuffer, strChaining);
	if(pdest)
	{
		pdest = pdest + sizeof(strChaining) - 1;
		ParseFile(pdest, cstrReadValue);
		if(strstr(cstrReadValue, "on"))
		{
			m_bChaining = TRUE;
		}
		else
		{
			m_bChaining = FALSE;
		}
	}
	// END CHAINING

	// SECOND PORT
	pdest = strstr(strFileReadBuffer, strSecondPort);
	if(pdest)
	{
		pdest = pdest + sizeof(strSecondPort) - 1;
		ParseFile(pdest, cstrReadValue);
		m_cstrSecondaryPort = cstrReadValue;
		if(atoi(m_cstrSecondaryPort) == 0)
		{
			// TODO: take primary port
			m_cstrSecondaryPort = "575";
		}
	}
	// SECOND PORT

	delete[] strFileReadBuffer;
}

void CGroupConfig::ReadDTCDevices(CFile *file, int iFileSize)
{
	CString cstrReadValue;
	char	*ptr, *strFileReadBuffer;
	char	*pdest;
	char	strRemark[] = "REMARK:  ";
	char	strDataCastDevDev[] = "DTC-DEVICE:  ";
	char	strDataCastDevDevOld[] = "TDMF-DEVICE:  ";
	char	strDataDisk[] = "DATA-DISK:        ";
	char	strMirror[] = "MIRROR-DISK:      ";
	
	ptr = new char[iFileSize];
	strFileReadBuffer = ptr;

	file->Read(strFileReadBuffer, iFileSize);

	m_vecDevicePair.clear();

	while(strFileReadBuffer != NULL)
	{
		bool bNewOneRead = false;
		CDevicePairConfig DevicePairConfig;

		// REMARKS
		pdest = strstr(strFileReadBuffer, strRemark);
		if(pdest)
		{
			bNewOneRead = true;
			pdest = pdest + sizeof(strRemark) - 1;
			ParseFile(pdest, cstrReadValue);
			DevicePairConfig.m_cstrRemarks = cstrReadValue;
		}
		// END REMARKS	

		// OPEN STORAGE DEVICE
		pdest = strstr(strFileReadBuffer, strDataCastDevDev);
		if (pdest == NULL)
		{
			pdest = strstr(strFileReadBuffer, strDataCastDevDevOld);
		}
		if(pdest)
		{
			bNewOneRead = true;
			pdest = pdest + sizeof(strDataCastDevDev) - 1;
			ParseFile(pdest, cstrReadValue);
			DevicePairConfig.m_cstrDTCDev = cstrReadValue;
		}
		// END OPEN STORAGE DEVICE	

		// DATA DISK
		pdest = strstr(strFileReadBuffer, strDataDisk);
		if(pdest)
		{
			bNewOneRead = true;
			pdest = pdest + sizeof(strDataDisk) - 1;
			ParseFile(pdest, cstrReadValue);
			DevicePairConfig.m_cstrDataDev = cstrReadValue;
		}
		// END DATA DISK

		// MIRROR DISK
		pdest = strstr(strFileReadBuffer, strMirror);
		if(pdest)
		{
			bNewOneRead = true;
			pdest = pdest + sizeof(strMirror) - 1;
			ParseFile(pdest, cstrReadValue);
			DevicePairConfig.m_cstrMirrorDev = cstrReadValue;
		}
		// END MIRROR DISK
		
		// Add to linked list
		if (bNewOneRead)
		{
			m_vecDevicePair.push_back(DevicePairConfig);
		}

		// move ptr
		strFileReadBuffer = pdest;
	}

	delete ptr;
}

void CGroupConfig::SaveToFile()
{
	FILE		*file;
	CString		strReadString;
	time_t		ltime;
	char		strTime[128];
	struct tm	*gmt;
	char		strTempCount[10];

	time(&ltime);
	gmt = localtime(&ltime);
    sprintf(strTime, "%s", asctime(gmt));

	file = fopen(m_cstrFilename, "w+");

	strReadString.LoadString(IDS_STRING_HEADER_BREAK);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_OEM_FILE);
	strReadString = "#   " +  strReadString;
	strReadString = strReadString + m_cstrFilename;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_OEM_VERSION);
	strReadString = "#   "+  strReadString;
	strReadString = strReadString + VERSION" \n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_UPDATE);
	strReadString = strReadString + strTime;
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_HEADER_BREAK);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_NOTE);
	strReadString = strReadString + m_cstrNote;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	// Begin System Values
	strReadString.LoadString(IDS_STRING_PRIMARY_DEF);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_HOST);
	strReadString = strReadString + m_cstrPrimaryHost;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_PSTORE);
	char	strPStore[_MAX_PATH];
	memset(strPStore, 0, sizeof(strPStore));
	sprintf(strPStore, "%.*s", _MAX_PATH-1, m_cstrPStore);
	strReadString = strReadString + strPStore;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_SECONDARY);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_HOST);
	strReadString = strReadString + m_cstrSecondaryHost;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_JOURNAL);
	strReadString = strReadString + m_cstrJournal;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	
	strReadString.LoadString(IDS_STRING_SECOND_PORT);
	strReadString = strReadString + m_cstrSecondaryPort;
	strReadString = strReadString + "\n";
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

	if(m_bChaining)
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

	// Begin DTC devices
	strReadString.LoadString(IDS_STRING_DEVICE_DEF);
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

	// Profile section.  This has to work for mult DTC devices
	//
	int iDTCListCount = GetPairCount();

	for(int i = 1; i <= iDTCListCount; i++)
	{
		CDevicePairConfig* pDevicePairConfig = GetPair(i-1);

		strReadString.LoadString(IDS_STRING_PROFILE);
		memset(strTempCount, 0, sizeof(strTempCount));
		itoa(i, strTempCount, 10);

		strReadString = strReadString + strTempCount;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

		strReadString.LoadString(IDS_STRING_REMARK);
		strReadString = strReadString + pDevicePairConfig->m_cstrRemarks;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

		strReadString.LoadString(IDS_STRING_PRIMARY_SYSA);
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

		itoa(i - 1, strTempCount, 10);

		strReadString = "  DTC-DEVICE:  ";
		CString cstrTmp = CCommand::FormatDriveName(pDevicePairConfig->m_cstrDataDev);
		strReadString = strReadString + cstrTmp;
		if (cstrTmp.GetAt(cstrTmp.GetLength()-1) != ':')
		{
			strReadString = strReadString + ":";
		}
		strReadString = strReadString + strTempCount;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
		
		strReadString.LoadString(IDS_STRING_DATADISK);
		strReadString = strReadString + pDevicePairConfig->m_cstrDataDev;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
		
		strReadString.LoadString(IDS_STRING_SEC_SYSB);
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
		
		
		strReadString.LoadString(IDS_STRING_MIRROR);
		strReadString = strReadString + pDevicePairConfig->m_cstrMirrorDev;
		strReadString = strReadString + "\n";
		fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);
	}
	//
	// End Profile Section

	// End DTC devices

	strReadString.LoadString(IDS_STRING_END_CONFIG);
	strReadString = strReadString + "Configuration File: "; 
	strReadString = strReadString + m_cstrFilename;
	fwrite(strReadString.operator LPCTSTR (), sizeof(char), strReadString.GetLength(), file);

	fclose(file);
}

bool CGroupConfig::IsStarted() 
{
	SetLastError(0);

	CString	cstrDtcPath;
	cstrDtcPath.Format("\\\\.\\DTC\\lg%d", m_nGroupNb);

	HANDLE handle = CreateFile(cstrDtcPath, GENERIC_READ, FILE_SHARE_READ, NULL,
								OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(handle == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() != (FTD_DRIVER_ERROR_CODE | EBUSY))
		{
			return false;
		}
	}
	else
	{
		CloseHandle(handle);
	}

	return true;
}

bool CGroupConfig::IsValid()
{
	// Validated in Property Pages
	// IPs, Port, Pstore and Journal dir must not be empty

	// At least one pair must be created
	if (m_vecDevicePair.size() == 0)
	{
		return false;
	}

	return true;
}

bool CGroupConfig::ReadTunablesFromBatFile()
{
	// Try to open group's tunables tmp .bat file (settunables%d.bat)
	CFile file;
	CString cstrTunablesFile;
	cstrTunablesFile.Format("%ssettunables%d.bat", CCommand::m_cstrInstallPath, m_nGroupNb);
	
	if (! file.Open(cstrTunablesFile, CFile::modeRead, NULL))
	{
		return false;
	}

	int iFileSize = (int)file.GetLength();
	if (iFileSize == 0)
	{
		return false;
	}

	char*   strFileReadBuffer;
	char*   pdest;
	CString cstrReadValue;

	strFileReadBuffer = new char[iFileSize];
	file.Read(strFileReadBuffer, iFileSize);

	// Read SYNCMODE
	pdest = strstr(strFileReadBuffer, "SYNCMODE=");
	if(pdest)
	{
		pdest = pdest + sizeof("SYNCMODE=") - 1;
		ParseFile(pdest, cstrReadValue);
		m_bSyncMode = ((cstrReadValue == "1") || (cstrReadValue == "ON"));
	}

	// Read SYNCMODEDEPTH
	pdest = strstr(strFileReadBuffer, "SYNCMODEDEPTH=");
	if(pdest)
	{
		pdest = pdest + sizeof("SYNCMODEDEPTH=") - 1;
		ParseFile(pdest, cstrReadValue);
		m_cstrSyncDepth = cstrReadValue;
	}

	// Read SYNCMODETIMEOUT
	pdest = strstr(strFileReadBuffer, "SYNCMODETIMEOUT=");
	if(pdest)
	{
		pdest = pdest + sizeof("SYNCMODETIMEOUT=") - 1;
		ParseFile(pdest, cstrReadValue);
		m_cstrSyncTimeout = cstrReadValue;
	}

	// Read COMPRESSION
	pdest = strstr(strFileReadBuffer, "COMPRESSION=");
	if(pdest)
	{
		pdest = pdest + sizeof("COMPRESSION=") - 1;
		ParseFile(pdest, cstrReadValue);
		m_bCompression = (cstrReadValue == "1");
	}

	// Read Refresh interval
	pdest = strstr(strFileReadBuffer, "REFRESHTIMEOUT=");
	if(pdest)
	{
		pdest = pdest + sizeof("REFRESHTIMEOUT=") - 1;
		ParseFile(pdest, cstrReadValue);
		if (cstrReadValue == "-1")
		{
			m_bRefreshNeverTimeout = true;
			m_cstrRefreshTimeout = "";
		}
		else
		{
			m_bRefreshNeverTimeout = false;
			m_cstrRefreshTimeout = cstrReadValue;
		}
	}

	// Read JOURNAL
	pdest = strstr(strFileReadBuffer, "JOURNAL=");
	if(pdest)
	{
		pdest = pdest + sizeof("JOURNAL=") - 1;
		ParseFile(pdest, cstrReadValue);
		m_bJournalLess = ((cstrReadValue == "0") || (cstrReadValue == "OFF"));
	}

	delete[] strFileReadBuffer;

	return true;
}

bool CGroupConfig::SaveTunablesToBatFile()
{
	// Create a settunables%d.bat file
	CFile file;
	CString cstrTunablesFile;
	cstrTunablesFile.Format("%ssettunables%d.bat", CCommand::m_cstrInstallPath, m_nGroupNb);
	
	if (! file.Open(cstrTunablesFile, CFile::modeCreate | CFile::modeWrite, NULL))
	{
		return false;
	}

	// dtcset -g # TUNABLE=v
	CString cstrCmd;

	// Set SYNCMODE
	int iSyncMode = m_bSyncMode ? 1 : 0;
	cstrCmd.Format("\"%s%sset.exe\" -g %d SYNCMODE=%d\r\n", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, iSyncMode);
	file.Write(cstrCmd, cstrCmd.GetLength());

	if(m_bSyncMode)
	{
		// Set SYNCMODEDEPTH
		cstrCmd.Format("\"%s%sset.exe\" -g %d SYNCMODEDEPTH=%s\r\n", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, m_cstrSyncDepth);
		file.Write(cstrCmd, cstrCmd.GetLength());

		// Set SYNCMODETIMEOUT
		cstrCmd.Format("\"%s%sset.exe\" -g %d SYNCMODETIMEOUT=%s\r\n", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, m_cstrSyncTimeout);
		file.Write(cstrCmd, cstrCmd.GetLength());
	}
	
	int	iCompression = m_bCompression ? 1 : 0;
	// Set COMPRESSION
	cstrCmd.Format("\"%s%sset.exe\" -g %d COMPRESSION=%d\r\n", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, iCompression);
	file.Write(cstrCmd, cstrCmd.GetLength());
	
	// Set Refresh interval
	if(m_bRefreshNeverTimeout)
	{
		cstrCmd.Format("\"%s%sset.exe\" -g %d REFRESHTIMEOUT=-1\r\n", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb);
	}
	else
	{
		cstrCmd.Format("\"%s%sset.exe\" -g %d REFRESHTIMEOUT=%s\r\n", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, m_cstrRefreshTimeout);
	}
	file.Write(cstrCmd, cstrCmd.GetLength());

	int	iJournal = m_bJournalLess ? 0 : 1;
	// Set JOURNAL
	cstrCmd.Format("\"%s%sset.exe\" -g %d JOURNAL=%d\r\n", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, iJournal);
	file.Write(cstrCmd, cstrCmd.GetLength());

	file.Close();
	
	return true;
}

bool CGroupConfig::ReadTunables()
{
	CWaitCursor WaitCursor;
	CString cstrTmpFile = CCommand::DumpTunablesToFile(m_nGroupNb);

	// Get tunables from file
	//char	strChunkSize[] = "CHUNKSIZE: ";
	//char	strChunkDelay[] = "CHUNKDELAY: ";
	char	strSyncMode[] = "SYNCMODE: ";
	char	strSyncModeDepth[] = "SYNCMODEDEPTH: ";
	char	strSyncModeTimeOut[] = "SYNCMODETIMEOUT: ";
	//char	strIODelay[] = "IODELAY: ";
	//char	strNetMaxBps[] = "NETMAXKBPS: ";
	//char	strStatInterval[] = "STATINTERVAL: ";
	//char	strMaxStatFileSize[] = "MAXSTATFILESIZE: ";
	//char	strLogStats[] = "LOGSTATS: ";
	//char	strTraceThrottle[] = "TRACETHROTTLE: ";
	char	strCompression[] = "COMPRESSION: ";
	char	strRefreshInt[] = "REFRESHTIMEOUT: ";
	char	strJournal[] = "JOURNAL: ";
	CFile	file;
	int		iFileSize;
	char	*strFileReadBuffer, *pdest;
	CString cstrReadValue;

	BOOL bIsOpen = file.Open(cstrTmpFile, CFile::modeRead, NULL);
	if(!bIsOpen)
		file.Open(cstrTmpFile, CFile::modeCreate, NULL);
	iFileSize = (int)file.GetLength();

	if(iFileSize == 0)
	{
		file.Close();
		return ReadTunablesFromBatFile();
	}

	iFileSize = (int)file.GetLength();
	strFileReadBuffer = new char[iFileSize];

	file.Read(strFileReadBuffer, iFileSize);

	// compression
	pdest = strstr(strFileReadBuffer, strCompression);
	if(pdest)
	{
		pdest = pdest + sizeof(strCompression) - 1;
		ParseFile(pdest, cstrReadValue);
		m_bCompression = atoi(cstrReadValue) ? true : false;
	}
	// end compression

	// Sync Mode
	pdest = strstr(strFileReadBuffer, strSyncMode);
	if(pdest)
	{
		pdest = pdest + sizeof(strSyncMode) - 1;
		ParseFile(pdest, cstrReadValue);
		cstrReadValue.MakeUpper();
		if (cstrReadValue == "ON")
		{
			m_bSyncMode = TRUE;
		}
		else if (cstrReadValue == "OFF")
		{
			m_bSyncMode = FALSE;
		}
		else
		{
			m_bSyncMode = atoi(cstrReadValue) ? true : false;
		}
	}
	// end Sync Mode

	// Sync Mode depth
	pdest = strstr(strFileReadBuffer, strSyncModeDepth);
	if(pdest)
	{
		pdest = pdest + sizeof(strSyncModeDepth) - 1;
		ParseFile(pdest, cstrReadValue);
		m_cstrSyncDepth = cstrReadValue;
	}
	// end Sync Mode depth

	// Sync Mode timeout
	pdest = strstr(strFileReadBuffer, strSyncModeTimeOut);
	if(pdest)
	{
		pdest = pdest + sizeof(strSyncModeTimeOut) - 1;
		ParseFile(pdest, cstrReadValue);
		m_cstrSyncTimeout = cstrReadValue;
	}
	// end Sync Mode timeout

	// Refresh Interval
	pdest = strstr(strFileReadBuffer, strRefreshInt);
	if(pdest)
	{
		pdest = pdest + sizeof(strRefreshInt) - 1;
		ParseFile(pdest, cstrReadValue);
		
		if (cstrReadValue == "-1")
		{
			m_bRefreshNeverTimeout = true;
			m_cstrRefreshTimeout = "0";
		}
		else
		{
			m_bRefreshNeverTimeout = false;
			m_cstrRefreshTimeout = cstrReadValue;
		}
	}
	// Refresh Interval

	// Journal
	pdest = strstr(strFileReadBuffer, strJournal);
	if(pdest)
	{
		pdest = pdest + sizeof(strJournal) - 1;
		ParseFile(pdest, cstrReadValue);
		cstrReadValue.MakeUpper();
		if (cstrReadValue == "ON")
		{
			m_bJournalLess = FALSE;
		}
		else if (cstrReadValue == "OFF")
		{
			m_bJournalLess = TRUE;
		}
		else
		{
			m_bJournalLess = atoi(cstrReadValue) ? false : true;
		}
	}
	// end Journal

	file.Close();

	delete[] strFileReadBuffer;

	// Remove Tmp file
	DeleteFile(cstrTmpFile);

	return true;
}

void CGroupConfig::SaveTunables()
{
	CWaitCursor WaitCursor;

	if (IsStarted() == false) // will not be able to set tunables with dtcset
	{
		SaveTunablesToBatFile();
	}
	else
	{
		CString             cstrCmdLine;
		PROCESS_INFORMATION ProcessInfo;
		
		// Set up the start up info struct.
		STARTUPINFO si;
		ZeroMemory(&si,sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		
		// Set SYNCMODE
		int iSyncMode = 0;
		if(m_bSyncMode)
			iSyncMode = 1;
		
		cstrCmdLine.Format("\"%s%sset.exe\" -g %d SYNCMODE=%d", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, iSyncMode);
		LPTSTR lpstr = cstrCmdLine.GetBuffer(256);
		CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
		cstrCmdLine.ReleaseBuffer();
		WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
		// Close process and thread handles. 
		CloseHandle(ProcessInfo.hProcess);
		CloseHandle(ProcessInfo.hThread);
		
		if(m_bSyncMode)
		{
			// Set SYNCMODEDEPTH
			cstrCmdLine.Format("\"%s%sset.exe\" -g %d SYNCMODEDEPTH=%s", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, m_cstrSyncDepth);
			lpstr = cstrCmdLine.GetBuffer(256);
			CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
			cstrCmdLine.ReleaseBuffer();
			WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
			// Close process and thread handles. 
		    CloseHandle(ProcessInfo.hProcess);
			CloseHandle(ProcessInfo.hThread);
			
			// Set SYNCMODETIMEOUT
			cstrCmdLine.Format("\"%s%sset.exe\" -g %d SYNCMODETIMEOUT=%s", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, m_cstrSyncTimeout);
			lpstr = cstrCmdLine.GetBuffer(256);
			CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
			cstrCmdLine.ReleaseBuffer();
			WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
			// Close process and thread handles. 
			CloseHandle(ProcessInfo.hProcess);
			CloseHandle(ProcessInfo.hThread);
		}
		
		int	iCompression = 0;
		if(m_bCompression)
		{
			iCompression = 1;
		}
		// Set COMPRESSION
		cstrCmdLine.Format("\"%s%sset.exe\" -g %d COMPRESSION=%d", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, iCompression);
		lpstr = cstrCmdLine.GetBuffer(256);
		CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
		cstrCmdLine.ReleaseBuffer();
		WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
		// Close process and thread handles. 
		CloseHandle(ProcessInfo.hProcess);
		CloseHandle(ProcessInfo.hThread);
		
		// Set Refresh interval
		if(m_bRefreshNeverTimeout)
		{
			cstrCmdLine.Format("\"%s%sset.exe\" -g %d REFRESHTIMEOUT=-1", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb);
		}
		else
		{
			cstrCmdLine.Format("\"%s%sset.exe\" -g %d REFRESHTIMEOUT=%s", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, m_cstrRefreshTimeout);
		}
		lpstr = cstrCmdLine.GetBuffer(256);
		CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
		cstrCmdLine.ReleaseBuffer();
		WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
		// Close process and thread handles. 
		CloseHandle(ProcessInfo.hProcess);
		CloseHandle(ProcessInfo.hThread);

		int	iJournal = m_bJournalLess ? 0 : 1;
		// Set JOURNAL
		cstrCmdLine.Format("\"%s%sset.exe\" -g %d JOURNAL=%d", CCommand::m_cstrInstallPath, CMDPREFIX, m_nGroupNb, iJournal);
		lpstr = cstrCmdLine.GetBuffer(256);
		CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
		cstrCmdLine.ReleaseBuffer();
		WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
		// Close process and thread handles. 
		CloseHandle(ProcessInfo.hProcess);
		CloseHandle(ProcessInfo.hThread);
	}
}
