// TdmfCommonGui.h : main header file for the TDMFCOMMONGUI application
//

#if !defined(AFX_TDMFCOMMONGUI_H__03771F0E_DE9A_46B3_BD37_F50A7539E1E6__INCLUDED_)
#define AFX_TDMFCOMMONGUI_H__03771F0E_DE9A_46B3_BD37_F50A7539E1E6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#include "..\..\lib\libResMgr\ResourceManager.h"


/////////////////////////////////////////////////////////////////////////////
//

extern bool g_bStaticLogo;
extern bool g_bStatusbar;
extern bool g_bRedBar;

/////////////////////////////////////////////////////////////////////////////
//

#define CATCH_ALL_LOG_ERROR(MSG) \
	catch(...)                   \
	{                            \
		std::ostringstream oss;  \
		oss << "Unexpected Error: " << MSG; \
		AfxMessageBox(oss.str().c_str(), MB_OK | MB_ICONINFORMATION);   \
	}


/////////////////////////////////////////////////////////////////////////////
// CTdmfCommonGuiApp:
// See TdmfCommonGui.cpp for the implementation of this class
//

class CTdmfCommonGuiApp : public CWinApp
{
public:

	virtual BOOL PreTranslateMessage(MSG* pMsg);

	CTdmfCommonGuiApp();
	~CTdmfCommonGuiApp();

	CString m_cstrHelpFile;
  
	CResourceManager m_ResourceManager;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTdmfCommonGuiApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CTdmfCommonGuiApp)
	afx_msg void OnAppAbout();
	afx_msg void OnHelpContents();
	afx_msg void OnHelpIndex();
	afx_msg void OnHelpSearch();
    afx_msg void OnFileOpen();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CTdmfCommonGuiApp theApp;


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDMFCOMMONGUI_H__03771F0E_DE9A_46B3_BD37_F50A7539E1E6__INCLUDED_)
