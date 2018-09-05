// TdmfDbUpgrade.h : main header file for the TDMFDBUPGRADE DLL
//

#if !defined(AFX_TDMFDBUPGRADE_H__E9D9C4CA_C0BB_43F6_BF1C_929FFE99D8E6__INCLUDED_)
#define AFX_TDMFDBUPGRADE_H__E9D9C4CA_C0BB_43F6_BF1C_929FFE99D8E6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTdmfDbUpgradeApp
// See TdmfDbUpgrade.cpp for the implementation of this class
//

class CTdmfDbUpgradeApp : public CWinApp
{
public:
	CTdmfDbUpgradeApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTdmfDbUpgradeApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CTdmfDbUpgradeApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//extern "C" __declspec(dllexport)
//int UpgradeTdmfDb(const char* szComputerName);


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDMFDBUPGRADE_H__E9D9C4CA_C0BB_43F6_BF1C_929FFE99D8E6__INCLUDED_)
