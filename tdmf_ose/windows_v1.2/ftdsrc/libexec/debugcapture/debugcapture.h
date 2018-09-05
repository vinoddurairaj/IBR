/*
 * DebugCapture.h - Replication Debug Capture
 * 
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef _FTD_DEBUGCAPTURE_H_
#define _FTD_DEBUGCAPTURE_H_

#include <Windows.h>

#define REG_PATH "Software\\" OEMNAME "\\" PRODUCTNAME "\\CurrentVersion"
#define REG_KEY "InstallPath"

#define REG_PARAM_PATH "SYSTEM\\CurrentControlSet\\Services\\" DRIVERNAME "\\Parameters"
#define REG_KEY_BAB "num_chunks"
#define REG_KEY_MAXMEM "maxmem"
#define REG_KEY_CHUNK_SIZE "CHUNK_SIZE"

#define EVENT_SYSTEM 0
#define EVENT_APPLICATION 1

char	szCfgFileList[1000][_MAX_PATH];
HANDLE	m_hLog;

int main(int argc, char *argv[]);
int getConfigFileList(int *iNumCfgs);
int	getInstallPath(char *szPath);
int writeDebugFile(char *szFileName);
int getLocalSystemInfo(char *szSystemInfo);
int getEventLogInfo(char *szEventLog, int iEventNumber, int iEventType);
int getNumberOfEventsInLog(char *szType);
int getLicenseInfo(char *szLicenseInfo);
int getDeviceInfo(char *szDeviceInfo);
int	getRegistryInfo(char *szRegInfo);
int	getReplicationDirectoryList(char *szDirList);
void waitForProcessEnd(PROCESS_INFORMATION *ProcessInfo);

#endif