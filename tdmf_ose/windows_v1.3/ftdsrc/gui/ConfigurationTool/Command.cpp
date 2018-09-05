// Command.cpp: implementation of the CCommand class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ConfigurationTool.h"
#include "Command.h"

// Mike Pollett
#include "../../tdmf.inc"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern "C"
{
#include "sock.h"
}


CString CCommand::m_cstrInstallPath;


//////////////////////////////////////////////////////////////////////
//
void CCommand::KillPMD(UINT nGroupNb)
{
	CWaitCursor         WaitCursor;
	CString             cstrCmdLine;
	PROCESS_INFORMATION ProcessInfo;

	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

	// KillPmd
	cstrCmdLine.Format("\"%s%skillpmd.exe\" -g %d", m_cstrInstallPath, CMDPREFIX, nGroupNb);
	LPTSTR lpstr = cstrCmdLine.GetBuffer(256);
	CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
	cstrCmdLine.ReleaseBuffer();
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
	// Close process and thread handles. 
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);
}

void CCommand::LaunchPMD(UINT nGroupNb)
{
	CWaitCursor         WaitCursor;
	CString             cstrCmdLine;
	PROCESS_INFORMATION ProcessInfo;

	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

	// KillPmd
	cstrCmdLine.Format("\"%s%slaunchpmd.exe\" -g %d", m_cstrInstallPath, CMDPREFIX, nGroupNb);
	LPTSTR lpstr = cstrCmdLine.GetBuffer(256);
	CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
	cstrCmdLine.ReleaseBuffer();
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
	// Close process and thread handles. 
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);
}

void CCommand::StopGroup(UINT nGroupNb)
{
	CWaitCursor         WaitCursor;
	CString             cstrCmdLine;
	PROCESS_INFORMATION ProcessInfo;

	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

	// KillPmd
	cstrCmdLine.Format("\"%s%skillpmd.exe\" -g %d", m_cstrInstallPath, CMDPREFIX, nGroupNb);
	LPTSTR lpstr = cstrCmdLine.GetBuffer(256);
	CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
	cstrCmdLine.ReleaseBuffer();
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
	// Close process and thread handles. 
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);

	// Stop
	cstrCmdLine.Format("\"%s%sstop.exe\" -g %d", m_cstrInstallPath, CMDPREFIX, nGroupNb);
	lpstr = cstrCmdLine.GetBuffer(256);
	CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
	cstrCmdLine.ReleaseBuffer();
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
	// Close process and thread handles. 
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);
}

void CCommand::StartGroup(UINT nGroupNb)
{
	CWaitCursor         WaitCursor;
	CString             cstrCmdLine;
	PROCESS_INFORMATION ProcessInfo;

	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

	// Start
	cstrCmdLine.Format("\"%s%sstart.exe\" -g %d", m_cstrInstallPath, CMDPREFIX, nGroupNb);
	LPTSTR lpstr = cstrCmdLine.GetBuffer(256);
	BOOL b = CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
	cstrCmdLine.ReleaseBuffer();
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    // Close process and thread handles. 
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);
}

CString CCommand::DumpTunablesToFile(UINT nGroupNb)
{
	CWaitCursor         WaitCursor;
	CString             cstrCmdLine;
	PROCESS_INFORMATION ProcessInfo;

	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

	cstrCmdLine.Format("\"%s%sset.exe\" -o -g %d", m_cstrInstallPath, CMDPREFIX, nGroupNb);
	LPTSTR lpstr = cstrCmdLine.GetBuffer(256);
	//dtcset -o -g <number> 
	CreateProcess(NULL, lpstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
	cstrCmdLine.ReleaseBuffer();
	WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    // Close process and thread handles. 
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);

	CString cstrTmpFile = m_cstrInstallPath;
	cstrTmpFile += "PStore.txt";

	return cstrTmpFile;
}

void CCommand::GetRemoteDevicesRawInfo(LPCSTR lpcstrProductName, LPSTR lpcstrHostnameLocal, LPSTR lpcstrHostnameRemote, UINT nPort, UINT nGroupId, char *szReceiveLine, int iRecBufferSize)
{
	CWaitCursor WaitCursor;
	char szSendLine[25];
	sprintf(szSendLine, "ftd get all devices %d\n", nGroupId);
	
	char buf[256];

	sock_t * sockp = sock_create();

	if(sock_init(sockp, lpcstrHostnameLocal, lpcstrHostnameRemote, 0, 0, SOCK_STREAM, AF_INET, 1, 0) < 0)
	{
		sprintf(buf, "Unable to initialize socket [%s -> %s:%d]: %s.",
				sockp->lhostname,
				sockp->rhostname,
				sockp->port,
				sock_strerror(sock_errno()));
		::MessageBox(NULL, buf, lpcstrProductName, MB_OK);
	}		

	if(sock_bind(sockp, nPort) < 0)
	{
		sprintf(buf, "Socket bind failure [%s:%d]: %s.",
				sockp->lhostname,
				nPort,
				sock_strerror(sock_errno()));
		::MessageBox(NULL, buf, lpcstrProductName, MB_OK);
	}

	sockp->port = nPort;
	sockp->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	int	sockerr = 0;
	if(sock_connect_nonb(sockp, nPort, 3, 0, &sockerr) < 0)
	{
		sprintf(buf, "Unable to establish a connection with the remote [%s:%d]: %s.",
				sockp->rhostname,
				sockp->port,
				sock_strerror(sockerr));
		::MessageBox(NULL, buf, lpcstrProductName, MB_OK);
	}

	sock_set_b(sockp);

	int iBufferLen = strlen(szSendLine);

	sock_send(sockp, szSendLine, iBufferLen);

	memset(szReceiveLine, 0, sizeof(szReceiveLine));
		
	sock_recv(sockp, szReceiveLine, iRecBufferSize);

	if(sockp)
	{
		sock_disconnect(sockp);
		sock_delete(&sockp);
	}
}

CString CCommand::FormatDriveName(LPCSTR lpcstrDrive)
{
	CString cstrFormattedName;

	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	_splitpath(lpcstrDrive, drive, dir, fname, ext);
	
	if (strlen(drive) > 0)
	{
		cstrFormattedName.Format("%s%s", drive, dir);
	}
	else
	{
		cstrFormattedName = lpcstrDrive;
		cstrFormattedName = cstrFormattedName.SpanExcluding(" ");
		if (cstrFormattedName.GetAt(cstrFormattedName.GetLength()-1) != ':')
		{
			cstrFormattedName += ":";
		}
	}

	return cstrFormattedName;
}
