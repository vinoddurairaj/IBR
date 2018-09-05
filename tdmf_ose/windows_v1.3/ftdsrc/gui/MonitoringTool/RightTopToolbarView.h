#if !defined(AFX_RIGHTTOPTOOLBARVIEW_H__D6A6A477_A5F7_4FBD_9A8F_791922184892__INCLUDED_)
#define AFX_RIGHTTOPTOOLBARVIEW_H__D6A6A477_A5F7_4FBD_9A8F_791922184892__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RightTopToolbarView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRightTopToolbarView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "PageView.h"

class CRightTopToolbarView : public CPageView
{
protected:
	CRightTopToolbarView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CRightTopToolbarView)

// Form Data
public:
	//{{AFX_DATA(CRightTopToolbarView)
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
	//{{AFX_VIRTUAL(CRightTopToolbarView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CRightTopToolbarView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CRightTopToolbarView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIGHTTOPTOOLBARVIEW_H__D6A6A477_A5F7_4FBD_9A8F_791922184892__INCLUDED_)
