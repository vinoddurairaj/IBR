// DTCConfigTool.h : main header file for the DTCCONFIGTOOL application
//

#if !defined(AFX_DTCCONFIGTOOL_H__E0E9AFF7_557D_11D3_BAF7_00C04F54F512__INCLUDED_)
#define AFX_DTCCONFIGTOOL_H__E0E9AFF7_557D_11D3_BAF7_00C04F54F512__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDTCConfigToolApp:
// See DTCConfigTool.cpp for the implementation of this class
//

class CDTCConfigToolApp : public CWinApp
{
public:
	CDTCConfigToolApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDTCConfigToolApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDTCConfigToolApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DTCCONFIGTOOL_H__E0E9AFF7_557D_11D3_BAF7_00C04F54F512__INCLUDED_)
