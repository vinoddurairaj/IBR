// FsTdmfDbApp.h : main header file for the FSTDMFDB application
//

#if !defined(AFX_FSTDMFDB_H__B9E20B8C_57E9_48DC_8202_737A29C16D8D__INCLUDED_)
#define AFX_FSTDMFDB_H__B9E20B8C_57E9_48DC_8202_737A29C16D8D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbApp:
// See FsTdmfDb.cpp for the implementation of this class
//

class CFsTdmfDbApp : public CWinApp
{
public:
	CFsTdmfDbApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFsTdmfDbApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CFsTdmfDbApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FSTDMFDB_H__B9E20B8C_57E9_48DC_8202_737A29C16D8D__INCLUDED_)
