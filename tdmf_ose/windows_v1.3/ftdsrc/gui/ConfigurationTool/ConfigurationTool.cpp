// ConfigurationTool.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ConfigurationTool.h"
#include "ConfigurationToolDlg.h"

extern "C"
{
#include "sock.h"
#include "ftd_error.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigurationToolApp

BEGIN_MESSAGE_MAP(CConfigurationToolApp, CWinApp)
	//{{AFX_MSG_MAP(CConfigurationToolApp)
	//}}AFX_MSG
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigurationToolApp construction

CConfigurationToolApp::CConfigurationToolApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CConfigurationToolApp object

CConfigurationToolApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CConfigurationToolApp initialization

BOOL CConfigurationToolApp::InitInstance()
{
	if (sock_startup())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	if (ftd_init_errfac( "Replicator", "configuration Tool", NULL, NULL, 0, 1) == NULL) 
	{
		error_tracef( TRACEERR, "Start:main()", "Calling ftd_init_errfac()" );
		return FALSE;
	}

	AfxEnableControlContainer();

	// Standard initialization
#if (_MSC_VER < 1300)
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
#endif

	CConfigurationToolDlg dlg;

	CString cstrWndTitle = dlg.m_ResourceManager.GetProductName();
	cstrWndTitle += " Configuration Tool";
	HWND hWnd = ::FindWindow(NULL, cstrWndTitle);
	if (hWnd != NULL)
	{
		::SetForegroundWindow(hWnd);
		return FALSE;
	}
	else
	{
		m_pMainWnd = &dlg;
		int nResponse = dlg.DoModal();
	}

	ftd_delete_errfac();

	sock_cleanup();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
