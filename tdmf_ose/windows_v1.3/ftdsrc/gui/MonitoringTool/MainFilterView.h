#if !defined(AFX_MAINFILTERVIEW_H__E8A9613B_A630_41F0_9BE6_4AE2AD24E763__INCLUDED_)
#define AFX_MAINFILTERVIEW_H__E8A9613B_A630_41F0_9BE6_4AE2AD24E763__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MainFilterView.h : header file
//
#include "SHyperlinkComboBox.h"
/////////////////////////////////////////////////////////////////////////////
// CMainFilterView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CMainFilterView : public CFormView
{
protected:
	CMainFilterView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CMainFilterView)

// Form Data
public:
	//{{AFX_DATA(CMainFilterView)
	enum { IDD = IDD_TITLE_BAR };
	SHyperlinkComboBox	m_CBO_ObjectCtrl;
	SHyperlinkComboBox	m_CBO_SortTypeCtrl;
	SHyperlinkComboBox	m_CBO_SortByCtrl;
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFilterView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMainFilterView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CMainFilterView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFILTERVIEW_H__E8A9613B_A630_41F0_9BE6_4AE2AD24E763__INCLUDED_)
