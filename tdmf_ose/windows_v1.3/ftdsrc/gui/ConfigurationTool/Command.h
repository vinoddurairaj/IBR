// Command.h: interface for the CCommand class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMAND_H__BA824C9A_3C70_4CBA_B837_05313DCC2CA2__INCLUDED_)
#define AFX_COMMAND_H__BA824C9A_3C70_4CBA_B837_05313DCC2CA2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CCommand  
{
public:
	static CString m_cstrInstallPath;

public:
	CCommand() {}
	virtual ~CCommand() {}

	static void KillPMD(UINT nGroupNb);
	static void LaunchPMD(UINT nGroupNb);
	static void StopGroup(UINT nGroupNb);
	static void StartGroup(UINT nGroupNb);
	static CString DumpTunablesToFile(UINT nGroupNb);
	static void GetRemoteDevicesRawInfo(LPCSTR lpcstrProductName, LPSTR lpcstrHostnameLocal, LPSTR lpcstrHostnameRemote, UINT nPort, UINT nGroupId, char *szReceiveLine, int iRecBufferSize);
	static CString FormatDriveName(LPCSTR lpcstrDrive);
};

#endif // !defined(AFX_COMMAND_H__BA824C9A_3C70_4CBA_B837_05313DCC2CA2__INCLUDED_)
