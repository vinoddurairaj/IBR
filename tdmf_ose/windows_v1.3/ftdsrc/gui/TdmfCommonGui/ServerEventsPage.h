#if !defined(AFX_SERVEREVENTSPAGE_H__FA973A77_0682_438F_B2FA_A95627FFB500__INCLUDED_)
#define AFX_SERVEREVENTSPAGE_H__FA973A77_0682_438F_B2FA_A95627FFB500__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerEventsPage.h : header file
//

#include "SystemEventsPage.h"


/////////////////////////////////////////////////////////////////////////////
// CServerEventsPage dialog

class CServerEventsPage : public CSystemEventsPage
{
	DECLARE_DYNCREATE(CServerEventsPage)

// Construction
public:
	CServerEventsPage();
	~CServerEventsPage();

	void SaveColumnDef();
	void LoadColumnDef();

// Dialog Data
	//{{AFX_DATA(CServerEventsPage)
	enum { IDD = IDD_SERVER_EVENTS};
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerEventsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerEventsPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVEREVENTSPAGE_H__FA973A77_0682_438F_B2FA_A95627FFB500__INCLUDED_)
