#if !defined(AFX_FILTERDLG_H__637561F6_F78C_46A7_AE4C_CB4566B8FCC7__INCLUDED_)
#define AFX_FILTERDLG_H__637561F6_F78C_46A7_AE4C_CB4566B8FCC7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilterDlg.h : header file
//
#include "SHyperlinkComboBox.h"
/////////////////////////////////////////////////////////////////////////////
// CMonitorFilterView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CMonitorFilterView : public CFormView
{
protected:
	CMonitorFilterView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CMonitorFilterView)

// Form Data
public:
	//{{AFX_DATA(CMonitorFilterView)
	enum { IDD = IDD_PAGE_MONITOR };
	CEdit	m_Edit_ServerCtrl;
	CEdit	m_Edit_PairCtrl;
	CEdit	m_Edit_LGroupCtrl;
	CButton	m_Btn_ApplyCtrl;
	CButton	m_Btn_PickServerCtrl;
	CButton	m_Btn_PickLgroupCtrl;
	CButton	m_BTN_PickPairCtrl;
	SHyperlinkComboBox	m_Cbo_SortbyCtrl;
	SHyperlinkComboBox	m_Cbo_ServerCtrl;
	SHyperlinkComboBox	m_Cbo_PickServerCtrl;
	SHyperlinkComboBox	m_Cbo_PickPairCtrl;
	SHyperlinkComboBox	m_Cbo_PickLGroupCtrl;
	SHyperlinkComboBox	m_Cbo_PairCtrl;
	SHyperlinkComboBox	m_Cbo_LGroupCtrl;
	//}}AFX_DATA

// Attributes
public:

// Operations
public:
	void LoadTheServerPickCombo();
	void LoadTheLogicalGroupPickCombo();
	void LoadTheReplicationPairPickCombo();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMonitorFilterView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMonitorFilterView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CMonitorFilterView)
	afx_msg void OnSelchangeCboServer();
	afx_msg void OnBtnPickServer();
	afx_msg void OnSelchangeCboLgroup();
	afx_msg void OnSelchangeCboPair();
	afx_msg void OnBtnPickLgroup();
	afx_msg void OnBtnPickPair();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILTERDLG_H__637561F6_F78C_46A7_AE4C_CB4566B8FCC7__INCLUDED_)
