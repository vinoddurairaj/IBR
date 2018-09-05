// MonitorTool.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MonitorTool.h"
#include "MonitorToolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMonitorToolApp

BEGIN_MESSAGE_MAP(CMonitorToolApp, CWinApp)
	//{{AFX_MSG_MAP(CMonitorToolApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMonitorToolApp construction

CMonitorToolApp::CMonitorToolApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMonitorToolApp object

CMonitorToolApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMonitorToolApp initialization

BOOL CMonitorToolApp::InitInstance()
{
	AfxEnableControlContainer();

	char szFileName[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	GetModuleFileName(NULL, szFileName, _MAX_PATH);
	_splitpath(szFileName, drive, dir, fname, ext);
	_makepath(szFileName, drive, dir, "RBRes", "dll");
	m_ResourceManager.SetResourceDllName(szFileName);

	// Set application's name
	CString cstrAppName = m_ResourceManager.GetFullProductName();
	cstrAppName += " Monitor Tool";
	free((void*)m_pszAppName);
	m_pszAppName = _tcsdup(cstrAppName);

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CMonitorToolDlg dlg;
	m_pMainWnd = &dlg;

	

//	hIcon = m_ResourceManager.GetApplicationIcon();

	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
