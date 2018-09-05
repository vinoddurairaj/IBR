#if !defined(AFX_RGEVENTSPAGE_H__FA973A77_0682_438F_B2FA_A95627FFB500__INCLUDED_)
#define AFX_RGEVENTSPAGE_H__FA973A77_0682_438F_B2FA_A95627FFB500__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RGEventsPage.h : header file
//

#include "SystemEventsPage.h"


/////////////////////////////////////////////////////////////////////////////
// CRGEventsPage dialog

class CRGEventsPage : public CSystemEventsPage
{
	DECLARE_DYNCREATE(CRGEventsPage)

// Construction
public:
	CRGEventsPage();
	~CRGEventsPage();

	void SaveColumnDef();
	void LoadColumnDef();

// Dialog Data
	//{{AFX_DATA(CRGEventsPage)
	enum { IDD = IDD_SYSTEM_EVENTS};
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRGEventsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRGEventsPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RGEVENTSPAGE_H__FA973A77_0682_438F_B2FA_A95627FFB500__INCLUDED_)
