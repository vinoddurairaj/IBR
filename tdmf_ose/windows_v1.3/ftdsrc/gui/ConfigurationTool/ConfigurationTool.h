// ConfigurationTool.h : main header file for the CONFIGURATIONTOOL application
//

#if !defined(AFX_CONFIGURATIONTOOL_H__E07D2E42_929F_411A_A1C1_5F8613B1BA17__INCLUDED_)
#define AFX_CONFIGURATIONTOOL_H__E07D2E42_929F_411A_A1C1_5F8613B1BA17__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CConfigurationToolApp:
// See ConfigurationTool.cpp for the implementation of this class
//

class CConfigurationToolApp : public CWinApp
{
public:
	CConfigurationToolApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigurationToolApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CConfigurationToolApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGURATIONTOOL_H__E07D2E42_929F_411A_A1C1_5F8613B1BA17__INCLUDED_)
