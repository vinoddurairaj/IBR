#include "DebugCapture.h"
//#include "licapi.h"
//#include "misc.h"
#include <Windows.h>
#include <stdio.h>
#include <time.h>

//#include "ftd_lic.h"
#include "ftd_devlock.h"
#include "license.h"
//#include "misc.h"

//from misc.h
extern void getDiskSigAndInfo(char *szDir, char *szDiskInfo, int iGroupID);


int	getInstallPath(char *szPath)
{
	int	iRc = 0;

	HKEY	happ;
	DWORD	dwType = 0;
    DWORD	dwSize = _MAX_PATH;

    // Read the current state from the registry
    // Try opening the registry key:
    // HKEY_CURRENT_USER\Software\Legato Systems\<AppName>
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0, KEY_QUERY_VALUE, &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ, REG_KEY, NULL, &dwType, (BYTE*)szPath, &dwSize) != ERROR_SUCCESS)
		{
			iRc = -1;	
		}
		else
		{
			strcat(szPath, "\\");
			RegCloseKey(happ);
		}
	}
	else
	{
		iRc = -1;
	}

	return iRc;
}

int getConfigFileList(int *iNumCfgs)
{
	int		iRc = 0, i = 0, n = 0;
	char	szFileName[_MAX_PATH];
	FILE	*file;
	char	szPath[_MAX_PATH];

	memset(szCfgFileList, 0, sizeof(szCfgFileList));
	memset(szPath, 0, sizeof(szPath));
	
	iRc = getInstallPath(szPath);

	// get primary cfgs
	for(i = 0; i <= 999; i++)
	{
		memset(szFileName, 0, sizeof(szFileName));
		sprintf(szFileName, "%sp%03d.cfg", szPath, i);
		file = fopen(szFileName, "r");
		if(file)
		{
			strcpy(szCfgFileList[n], (const char *)szFileName);
			n++;
			fclose(file);
		}
	}

	// get secondary cfgs
	for(i = 0; i <= 999; i++)
	{
		memset(szFileName, 0, sizeof(szFileName));
		sprintf(szFileName, "%ss%03d.cfg", szPath, i);
		file = fopen(szFileName, "r");
		if(file)
		{
			strcpy(szCfgFileList[n], (const char *)szFileName);
			n++;
			fclose(file);
		}
	}

	*iNumCfgs = n;

	return iRc;
}

int getLocalSystemInfo(char *szSystemInfo)
{
	int			iRc = 0;
	SYSTEM_INFO SystemInfo;
	char		szTemp[15];

	GetSystemInfo(&SystemInfo);

	// processor architecture
	strcpy(szSystemInfo, "\tProcessor architecture: ");
	if(SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_MIPS)
	{
		strcat(szSystemInfo, "PROCESSOR_ARCHITECTURE_MIPS\n");
	}
	else if(SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA)
	{
		strcat(szSystemInfo, "PROCESSOR_ARCHITECTURE_ALPHA\n");
	}
	else if(SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_PPC)
	{
		strcat(szSystemInfo, "PROCESSOR_ARCHITECTURE_PPC\n");
	}
	else if(SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
	{
		strcat(szSystemInfo, "PROCESSOR_ARCHITECTURE_INTEL\n");
	}
	else
	{
		strcat(szSystemInfo, "PROCESSOR_ARCHITECTURE_UNKNOWN\n");
	}

	// page size
	memset(szTemp, 0, sizeof(szTemp));
	strcat(szSystemInfo, "\tPage size: ");
	itoa(SystemInfo.dwPageSize, szTemp, 10);
	strcat(szSystemInfo, szTemp);
	strcat(szSystemInfo, "\n");

	// Number of processors
	memset(szTemp, 0, sizeof(szTemp));
	strcat(szSystemInfo, "\tNumber of processors: ");
	itoa(SystemInfo.dwNumberOfProcessors, szTemp, 10);
	strcat(szSystemInfo, szTemp);
	strcat(szSystemInfo, "\n");

	// Processor Level
	strcat(szSystemInfo, "\tProcessor level: ");
	if(SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
	{
		if(SystemInfo.wProcessorLevel == 3)
			strcat(szSystemInfo, "Intel 80386\n");
		else if(SystemInfo.wProcessorLevel == 4 )
			strcat(szSystemInfo, "Intel 80486\n");
		else if(SystemInfo.wProcessorLevel == 5)
			strcat(szSystemInfo, "Intel Pentium\n");
		else if(SystemInfo.wProcessorLevel == 6)
			strcat(szSystemInfo, "Intel Pentium Pro or Pentium II\n");

		else if(SystemInfo.wProcessorLevel == 6)
			strcat(szSystemInfo, "Intel Pentium Pro or Pentium II\n");
	}
	else if(SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_MIPS)
	{
		strcat(szSystemInfo, "MIPS\n");
	}
	else if(SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA)
	{
		strcat(szSystemInfo, "Alpha\n");
	}
	else if(SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_PPC)
	{
		strcat(szSystemInfo, "PPC\n");
	}

	return iRc;
}

int getEventLogInfo(char *szEventLog, int iEventNumber, int iEventType)
{
	int				iRc = 0;
	EVENTLOGRECORD	*pevlr;
	LPTSTR			StringData = NULL, SourceName;
    DWORD			dwRead, dwNeeded, dwNext;
	BYTE			bBuffer[256];
	BOOL			bRc;


	pevlr = (EVENTLOGRECORD*)bBuffer;

	bRc = ReadEventLog(m_hLog,			// event log handle 
		EVENTLOG_SEQUENTIAL_READ |	// reads forward 
		EVENTLOG_BACKWARDS_READ,  
		iEventNumber,							// record number  
		pevlr,						// pointer to buffer 
		0,							// size of read 
		&dwRead,					// number of bytes read 
		&dwNext);					// bytes in next record 

	dwNeeded = dwNext;

	pevlr = (EVENTLOGRECORD*)malloc(dwNeeded);

	if (!(bRc = ReadEventLog(m_hLog,// event log handle 
		EVENTLOG_SEQUENTIAL_READ |	// reads forward 
		EVENTLOG_BACKWARDS_READ,  
		iEventNumber,							// record number  
		pevlr,						// pointer to buffer 
		dwNeeded,					// size of read 
		&dwRead,					// number of bytes read 
		&dwNext)))					// bytes in next record 
	{
		free(pevlr);
		iRc = -1;
	}

	SourceName = (LPSTR)((LPBYTE)pevlr + sizeof(EVENTLOGRECORD));

	if(iEventType == EVENT_APPLICATION)
	{
		if (!stricmp(SourceName, PRODUCTNAME))
		{
			StringData = (LPSTR)((LPBYTE)pevlr + pevlr->StringOffset);
			strcpy(szEventLog, StringData);
		}
	}
	else if(iEventType == EVENT_SYSTEM)
	{
		if (!stricmp(SourceName, QNM "block"))
		{
			StringData = (LPSTR)((LPBYTE)pevlr + pevlr->StringOffset + (pevlr->NumStrings - 1));
			strcpy(szEventLog, StringData);
		}
	}

	if(iRc == 0)
		free(pevlr);

	return iRc;
}

int getNumberOfEventsInLog(char *szType)
{
	int		iRc = 0;
	DWORD	dwNumEvents;

	m_hLog = OpenEventLog(NULL, szType);

	GetNumberOfEventLogRecords(m_hLog, &dwNumEvents);

	iRc = dwNumEvents;

	return iRc;
}

int getLicenseInfo(char *szLicenseInfo)
{
	int				iRc = 0;
	char			*szLic;
	char			szDate[_MAX_PATH];
#if defined(_OCTLIC)
	char			szLicenseKey[_MAX_PATH];
	unsigned long	lWeeks;
	FEATURE_TYPE	feature_flags_ret;
	unsigned long	lVersion;
	int				iOEM;
	struct tm		*gmt;
	char			szDate[_MAX_PATH];

#ifdef STK 
	iOEM = LICENSE_VERSION_STK_50;
#elif MTI
	iOEM = LICENSE_VERSION_MDS_50;
#else
	iOEM = LICENSE_VERSION_REPLICATION_43;
#endif

	memset(szDate, 0, sizeof(szDate));
	memset(szLicenseKey, 0, sizeof(szLicenseKey));

	getLicenseKeyFromReg(szLicenseKey);
	validateLicense(&lWeeks, &feature_flags_ret, &lVersion);

	if(lVersion != (unsigned int)iOEM)
		iRc = -3;

	if(iRc > 0)
	{
		// Time left in license key
		gmt = gmtime( (time_t *)&iRc );
		sprintf(szDate, "%s", asctime( gmt ));
	}
	else if(iRc == 0)
	{
		sprintf(szDate, "%s", "Permanent License");
	}
	else if(iRc == -1)
	{
		sprintf(szDate, "%s", "Expired Demo License");
	}
	else if(iRc == -2)
	{
		sprintf(szDate, "%s", "Invalid Access Code");
	}
	else if(iRc == -3)
	{
		sprintf(szDate, "%s", "Invalid License Key");
	}
	
	sprintf(szLicenseInfo, "%s: %s\n", szLicenseKey, szDate);
#else // no octlic
	memset(szDate, 0, sizeof(szDate));
	szLic = (char *)malloc(_MAX_PATH);
	
	memset(szLic, 0, _MAX_PATH);

	getLicenseKeyFromReg(szLic);
	iRc = check_key(&szLic, np_crypt_key_pmd, NP_CRYPT_KEY_LEN);

	if(iRc == 1)
	{
		sprintf(szDate, "%s", "Valid License");
	}
	else if(iRc == -2)
	{
		sprintf(szDate, "%s", "License Key Is Empty");
	}
	else if(iRc == -3)
	{
		sprintf(szDate, "%s", "Bad Checksum");
	}
	else if(iRc == -4)
	{
		sprintf(szDate, "%s", "Expired License");
	}
	else if(iRc == -5)
	{
		sprintf(szDate, "%s", "Wrong Host");
	}
	else if(iRc == -7)
	{
		sprintf(szDate, "%s", "Wrong Machine Type");
	}
	else if(iRc == -8)
	{
		sprintf(szDate, "%s", "Bad Feature Mask");
	}

	sprintf(szLicenseInfo, "%s: %s\n", szLic, szDate);

	free(szLic);
#endif // octlic
	return iRc;
}

int getDeviceInfo(char *szDebugDeviceInfo)
{
	int		iRc = 0;
	char	szDriveString[_MAX_PATH];
	char	szDrive[4];
	char	szDeviceInfo[100];
	char	szDeviceList[1024];
	char	szDiskSigInfo[_MAX_PATH];
	int		i = 0, iDrive;
	int		iTotal;
	ULARGE_INTEGER FreeBytesAvailableToCaller, TotalNumberOfBytes, TotalNumberOfFreeBytes;

	memset(szDriveString, 0, sizeof(szDriveString));
	memset(szDeviceList, 0, sizeof(szDeviceList));
	
	GetLogicalDriveStrings(sizeof(szDriveString), szDriveString);

	while(szDriveString[i] != 0 || szDriveString[i+1] != 0)
	{
		sprintf(szDrive, "%c%c%c", szDriveString[i], szDriveString[i+1], szDriveString[i+2]);
		i = i + 4;

		iDrive = GetDriveType(szDrive);
		
		if(iDrive != DRIVE_REMOVABLE && iDrive != DRIVE_CDROM && iDrive != DRIVE_RAMDISK &&
			iDrive != DRIVE_REMOTE)
		{
			memset(szDiskSigInfo, 0, sizeof(szDiskSigInfo));
			getDiskSigAndInfo(szDrive, szDiskSigInfo, -1);

			if (GetDiskFreeSpaceEx(szDrive, &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)) {
				iTotal = TotalNumberOfBytes.QuadPart/1024/1024;
				sprintf(szDeviceInfo, "\t[-%c-]\t%d MB\t\t%s\n",szDrive[0], iTotal, szDiskSigInfo);
			} else {
				sprintf(szDeviceInfo, "\t[-%c-]\tUnknown\t\t%s\n",szDrive[0], szDiskSigInfo);
			}
			
			strcat(szDeviceList, szDeviceInfo);
			memset(szDeviceInfo, 0, sizeof(szDeviceInfo));
		}
	}
	
	strcpy(szDebugDeviceInfo, szDeviceList);

	return iRc;
}

int	getRegistryInfo(char *szRegInfo)
{
	int		iRc = 0;
	HKEY	happ;
    DWORD	dwSize = sizeof(DWORD);
	DWORD	dwReadValue = 0;
	DWORD	dwMaxMem, dwNumChunks, dwChunkSize;

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
			dwReadValue = 0;
		}
		dwMaxMem = dwReadValue;
		dwReadValue = 0;

		if ( RegQueryValueEx(happ,
						REG_KEY_BAB,
						NULL,
						0,
						(PBYTE)&dwReadValue,
						&dwSize) != ERROR_SUCCESS) {
			dwReadValue = 0;
		}
		dwNumChunks = dwReadValue;
		dwReadValue = 0;

		if ( RegQueryValueEx(happ,
						REG_KEY_CHUNK_SIZE,
						NULL,
						0,
						(PBYTE)&dwReadValue,
						&dwSize) != ERROR_SUCCESS) {
			dwReadValue = 0;
		}
		dwChunkSize = dwReadValue;
	}
	RegCloseKey(happ);

	sprintf(szRegInfo, "\tMAX_MEM = %d\n\tNUM_CHUNKS = %d\n\tCHUNK_SIZE = %d\n", dwMaxMem, dwNumChunks, dwChunkSize);

	return iRc;
}

void waitForProcessEnd(PROCESS_INFORMATION *ProcessInfo)
{
	DWORD	dwExitCode = STILL_ACTIVE;
	
	while(dwExitCode == STILL_ACTIVE)
	{
		GetExitCodeProcess(ProcessInfo->hProcess, &dwExitCode);
		Sleep(1);
	}
}

int	getReplicationDirectoryList(char *szDirList)
{

	int		iRc = 0;
	FILE	*file;
	char	szPath[_MAX_PATH];
	char	szPathandFile[_MAX_PATH];
	char	szPathShort[_MAX_PATH];
	char	szPathandFileShort[_MAX_PATH];
	char	strFtdStartPath[_MAX_PATH];
	char	strCmdLineArgs[_MAX_PATH];
	PROCESS_INFORMATION ProcessInfo;
		
	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);


	memset(szPathShort, 0, sizeof(szPathShort));
	memset(szPathandFileShort, 0, sizeof(szPathandFileShort));
	memset(szPathandFile, 0, sizeof(szPathandFile));
	memset(strFtdStartPath, 0, sizeof(strFtdStartPath));
	memset(strCmdLineArgs, 0, sizeof(strCmdLineArgs));

	memset(szPath, 0, sizeof(szPath));
	
	getInstallPath(szPath);
	sprintf(szPathandFile, "%sDbgTmp.txt", szPath);

	GetShortPathName(szPath, szPathShort, sizeof(szPathShort));
	GetShortPathName(szPathandFile, szPathandFileShort, sizeof(szPathandFileShort));
	
	sprintf(strFtdStartPath, "c:\\I386\\Cmd.exe");
	sprintf(strCmdLineArgs, "Cmd dir %s > %s", szPathShort, szPathandFileShort);

	// open the temp file
	file = fopen(szPathandFile, "w+");

	if(file)
	{
		fclose(file);

		iRc = CreateProcess(strFtdStartPath, strCmdLineArgs, NULL, NULL, TRUE,
					CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

		if(iRc)
			waitForProcessEnd(&ProcessInfo);

		//fclose(file);
		DeleteFile(szPathandFile);
	}


	return iRc;
}

int writeDebugFile(char *szFileName)
{
	int		iRc = 0, i = 0;
	FILE	*file, *fileCfg, *fileBootIni;
	int		iNumCfgs = 0;
	int		iFileLen = 0;
	char	*szReadBuffer;
	char	szSystemInfo[1024];
	char	szEventLog[1024];
	char	szDirList[1024];
	int		iNumEvents = 0;
	char	szLicenseInfo[_MAX_PATH];
	char	szDeviceInfo[1024];
	struct tm		*gmt;
	char	szDate[_MAX_PATH];
	char	szHostName[_MAX_PATH];
	char	szRegInfo[_MAX_PATH];
	long	ltime;
	WORD	wVersionRequested;
	WSADATA wsaData;
	char	szWinDir[_MAX_PATH];
	char	szBootIniPath[_MAX_PATH];

	wVersionRequested = MAKEWORD( 2, 2 );
	iRc = WSAStartup( wVersionRequested, &wsaData );

	// open the debug dumb file
	file = fopen(szFileName, "w+");

	if(file)
	{
		// write debug file header
		memset(szDate, 0, sizeof(szDate));
		memset(szHostName, 0, sizeof(szHostName));
		gethostname(szHostName, sizeof(szHostName));
		iRc = WSAGetLastError();
		time( &ltime );
		gmt = gmtime( (time_t *)&ltime );
		sprintf(szDate, "DebugCapture: %s\nMachine: %s\n\n", asctime( gmt ), szHostName);
		fwrite(szDate, sizeof(char), strlen(szDate), file);
		WSACleanup( );

		// write system info to debug file
		memset(szSystemInfo, 0, sizeof(szSystemInfo));
		getLocalSystemInfo(szSystemInfo);
		fwrite("System Information\n", sizeof(char), strlen("System Information\n"), file);
		fwrite(szSystemInfo, sizeof(char), strlen(szSystemInfo), file);
		fwrite("End Of System Information\n\n", sizeof(char), strlen("End Of System Information\n\n"), file);

		// write license key to debug file
		fwrite("License Key Information\n", sizeof(char), strlen("License Key Information\n"), file);
		memset(szLicenseInfo, 0, sizeof(szLicenseInfo));
		getLicenseInfo(szLicenseInfo);
		fwrite("\t", sizeof(char), strlen("\t"), file);
		fwrite(szLicenseInfo, sizeof(char), strlen(szLicenseInfo), file);
		fwrite("End Of License Key Information\n\n", sizeof(char), strlen("End Of License Key Information\n\n"), file);

		// write device info to debug file
		memset(szDeviceInfo, 0, sizeof(szDeviceInfo));
		getDeviceInfo(szDeviceInfo);
		fwrite("Device Information\n", sizeof(char), strlen("Device Information\n"), file);
		fwrite(szDeviceInfo, sizeof(char), strlen(szDeviceInfo), file);
		fwrite("End Of Device Information\n\n", sizeof(char), strlen("End Of Device Information\n\n"), file);
	
		// write registry info to debug file
		fwrite("Registry Entries\n", sizeof(char), strlen("Registry Entries\n"), file);
		memset(szRegInfo, 0, sizeof(szRegInfo));
		getRegistryInfo(szRegInfo);
		fwrite(szRegInfo, sizeof(char), strlen(szRegInfo), file);
		fwrite("End Of Registry Entries\n\n", sizeof(char), strlen("End Of Registry Entries\n\n"), file);
		
		// write dir list for dtc
		memset(szDirList, 0, sizeof(szDirList));
//		getReplicationDirectoryList(szDirList);
	
		// write event log info to debug file (application)
		fwrite("Event Viewer Information (Application)\n", sizeof(char), strlen("Event Viewer Information (Application)\n"), file);
		iNumEvents = getNumberOfEventsInLog("Application");
		for(i = iNumEvents; i > 0; i--)
		{
			memset(szEventLog, 0, sizeof(szEventLog));
			getEventLogInfo(szEventLog, i, EVENT_APPLICATION);
			if(strlen(szEventLog) > 0)
			{
				strcat(szEventLog, "\n");
				fwrite("\t", sizeof(char), strlen("\t"), file);
				fwrite(szEventLog, sizeof(char), strlen(szEventLog), file);
			}
		}
		fwrite("End Of Event Viewer Information (Application)\n\n", sizeof(char), strlen("End Of Event Viewer Information (Application)\n\n"), file);

		// write event log info to debug file (system)
		fwrite("Event Viewer Information (System)\n", sizeof(char), strlen("Event Viewer Information (System)\n"), file);
		iNumEvents = getNumberOfEventsInLog("System");
		for(i = iNumEvents; i > 0; i--)
		{
			memset(szEventLog, 0, sizeof(szEventLog));
			getEventLogInfo(szEventLog, i, EVENT_SYSTEM);
			if(strlen(szEventLog) > 0)
			{
				strcat(szEventLog, "\n");
				fwrite("\t", sizeof(char), strlen("\t"), file);
				fwrite(szEventLog, sizeof(char), strlen(szEventLog), file);
			}
		}
		fwrite("End Of Event Viewer Information (System)\n\n", sizeof(char), strlen("End Of Event Viewer Information (System)\n\n"), file);


		// write boot.ini

		memset(szWinDir, 0, sizeof(szWinDir));
		GetWindowsDirectory(szWinDir, sizeof(szWinDir));
		memset(szBootIniPath, 0, sizeof(szBootIniPath));
		sprintf(szBootIniPath, "%c:\\Boot.ini", szWinDir[0]);

		fileBootIni = fopen(szBootIniPath, "r");

		//If BOOT.INI file exist
		if ( fileBootIni != NULL)
		{
			fwrite("Boot.ini\n\n", sizeof(char), strlen("Boot.ini\n\n"), file);
			// Get length of cfg file
			fseek(fileBootIni, 0L, SEEK_END);
			iFileLen = ftell(fileBootIni);
			fseek(fileBootIni, 0L, SEEK_SET);

			// malloc buffer of size of the cfg file
			szReadBuffer = malloc(iFileLen);
			memset(szReadBuffer, 0, iFileLen);

			// read the cfg file
			fread(szReadBuffer, sizeof(char), iFileLen, fileBootIni);

			// write contents of cfg to debug file
			fwrite(szReadBuffer, sizeof(char), iFileLen, file);
			fwrite("\n", sizeof(char), sizeof("\n") - 1, file);
		
			// free buffer and close cfg file
			free(szReadBuffer);
			fclose(fileBootIni);

			fwrite("End Of Boot.ini\n\n", sizeof(char), strlen("End Of Boot.ini\n\n"), file);
			// End Write boot.ini to debug file
		}

		// Write cfg's to debug file
		fwrite("Configuration files\n\n", sizeof(char), strlen("Configuration files\n\n"), file);
		iRc = getConfigFileList(&iNumCfgs);
		for(i = 0; i < iNumCfgs; i++)
		{
			fileCfg = fopen(szCfgFileList[i], "r");
			if(fileCfg)
			{
				// Get length of cfg file
				fseek(fileCfg, 0L, SEEK_END);
				iFileLen = ftell(fileCfg);
				fseek(fileCfg, 0L, SEEK_SET);

				// malloc buffer of size of the cfg file
				szReadBuffer = malloc(iFileLen);
				memset(szReadBuffer, 0, iFileLen);

				// read the cfg file
				fread(szReadBuffer, sizeof(char), iFileLen, fileCfg);

				// write contents of cfg to debug file
				fwrite(szReadBuffer, sizeof(char), iFileLen, file);
				fwrite("\n\n\n\n", sizeof(char), sizeof("\n\n\n\n") - 1, file);
				
				// free buffer and close cfg file
				free(szReadBuffer);
				fclose(fileCfg);
			}
		}
		fwrite("End Of Configuration Files\n\n", sizeof(char), strlen("End Of Configuration Files\n\n"), file);
		// End Write cfg's to debug file
		
		// close debug dump file
		fclose(file);
	}
	else
	{
		iRc = -1;
	}

	return iRc;
}

int main(int argc, char *argv[])
{
	int				iRc = 0;
	char			szPath[_MAX_PATH];
	char			szOutputFile[_MAX_PATH];


	if (ftd_dev_lock_create() == -1) {
		return 1;
	}

	memset(szPath, 0, sizeof(szPath));
	memset(szOutputFile, 0, sizeof(szOutputFile));

	getInstallPath(szPath);
	sprintf(szOutputFile, "%sDebug Capture.txt", szPath);

	printf("\nOutput File: %s\n", szOutputFile);

	writeDebugFile(szOutputFile);

	ftd_dev_lock_delete();

	return 0;
}