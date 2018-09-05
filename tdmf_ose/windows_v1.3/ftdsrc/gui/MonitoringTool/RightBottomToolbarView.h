#if !defined(AFX_RIGHTBOTTOMTOOLBARVIEW_H__AD657636_B92B_4FEA_86CE_2ED9DEE9CECE__INCLUDED_)
#define AFX_RIGHTBOTTOMTOOLBARVIEW_H__AD657636_B92B_4FEA_86CE_2ED9DEE9CECE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RightBottomToolbarView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRightBottomToolbarView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "PageView.h"

class CRightBottomToolbarView : public CPageView
{
protected:
	CRightBottomToolbarView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CRightBottomToolbarView)

// Form Data
public:
	//{{AFX_DATA(CRightBottomToolbarView)
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
	//{{AFX_VIRTUAL(CRightBottomToolbarView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CRightBottomToolbarView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CRightBottomToolbarView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIGHTBOTTOMTOOLBARVIEW_H__AD657636_B92B_4FEA_86CE_2ED9DEE9CECE__INCLUDED_)
