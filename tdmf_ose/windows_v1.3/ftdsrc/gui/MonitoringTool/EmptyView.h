#if !defined(AFX_EMPTYVIEW_H__E584D19B_72EE_4DA5_90E5_B01D4D327577__INCLUDED_)
#define AFX_EMPTYVIEW_H__E584D19B_72EE_4DA5_90E5_B01D4D327577__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EmptyView.h : header file
//
#include "PageView.h"
/////////////////////////////////////////////////////////////////////////////
// CEmptyView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CEmptyView : public CPageView
{
protected:
	CEmptyView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CEmptyView)

// Form Data
public:
	//{{AFX_DATA(CEmptyView)
	enum { IDD = IDD_PAGE_TOOLBAR };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:
   CToolBar		m_ToolBar;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEmptyView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CEmptyView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CEmptyView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMPTYVIEW_H__E584D19B_72EE_4DA5_90E5_B01D4D327577__INCLUDED_)
