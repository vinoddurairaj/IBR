#if !defined(AFX_OPTIONGENERALPAGE_H__C33643E7_D1D4_45E1_B038_927505E5EF89__INCLUDED_)
#define AFX_OPTIONGENERALPAGE_H__C33643E7_D1D4_45E1_B038_927505E5EF89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionGeneralPage.h : header file
//
#include "tdmfcommonguiDoc.h"
/////////////////////////////////////////////////////////////////////////////
// COptionGeneralPage dialog

class COptionGeneralPage : public CPropertyPage
{
	DECLARE_DYNCREATE(COptionGeneralPage)
    CTdmfCommonGuiDoc* m_pDoc;

// Construction
public:
	COptionGeneralPage(CTdmfCommonGuiDoc* pDoc = NULL);
	~COptionGeneralPage();

// Dialog Data
	//{{AFX_DATA(COptionGeneralPage)
	enum { IDD = IDD_OPTIONS_GENERAL };
	CSpinButtonCtrl	m_SpinT10;
	CSpinButtonCtrl	m_SpinT3;
	CSpinButtonCtrl	m_SpinT4;
	CSpinButtonCtrl	m_SpinT5;
	CSpinButtonCtrl	m_SpinT6;
	CSpinButtonCtrl	m_SpinT7;
	CSpinButtonCtrl	m_SpinT8;
	CSpinButtonCtrl	m_SpinT9;
	CSpinButtonCtrl	m_SpinT2;
	CSpinButtonCtrl	m_SpinT1;
	CEdit	m_EditT10;
    CEdit	m_EditT9;
    CEdit	m_EditT8;
    CEdit	m_EditT7;
    CEdit	m_EditT6;
    CEdit	m_EditT5;
    CEdit	m_EditT4;
    CEdit	m_EditT3;
	CEdit	m_EditT2;
	CEdit	m_EditT1;
	UINT	m_nT1;
	UINT	m_nT2;
	UINT	m_nT10;
    UINT	m_nT9;
    UINT	m_nT8;
    UINT	m_nT7;
    UINT	m_nT6;
    UINT	m_nT5;
    UINT	m_nT4;
    UINT	m_nT3;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COptionGeneralPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COptionGeneralPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONGENERALPAGE_H__C33643E7_D1D4_45E1_B038_927505E5EF89__INCLUDED_)
