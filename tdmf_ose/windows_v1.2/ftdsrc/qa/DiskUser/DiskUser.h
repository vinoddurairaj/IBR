// DiskUser.h : main header file for the DISKUSER application
//

#if !defined(AFX_DISKUSER_H__6DC2FB17_72C9_479E_842C_80CFA4740441__INCLUDED_)
#define AFX_DISKUSER_H__6DC2FB17_72C9_479E_842C_80CFA4740441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDiskUserApp:
// See DiskUser.cpp for the implementation of this class
//

class CDiskUserApp : public CWinApp
{
public:
	CDiskUserApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDiskUserApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDiskUserApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISKUSER_H__6DC2FB17_72C9_479E_842C_80CFA4740441__INCLUDED_)
