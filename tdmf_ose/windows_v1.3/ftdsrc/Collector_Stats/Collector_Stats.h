// Collector_Stats.h : main header file for the COLLECTOR_STATS application
//

#if !defined(AFX_COLLECTOR_STATS_H__5B8D3D1E_2447_4174_BB33_E8A906C0287C__INCLUDED_)
#define AFX_COLLECTOR_STATS_H__5B8D3D1E_2447_4174_BB33_E8A906C0287C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CCollector_StatsApp:
// See Collector_Stats.cpp for the implementation of this class
//

class CCollector_StatsApp : public CWinApp
{
public:
	CCollector_StatsApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCollector_StatsApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CCollector_StatsApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLLECTOR_STATS_H__5B8D3D1E_2447_4174_BB33_E8A906C0287C__INCLUDED_)
