#if !defined(AFX_DETAILSSERVERPAGE_H__72440D95_B468_4C72_A992_4896C74E3791__INCLUDED_)
#define AFX_DETAILSSERVERPAGE_H__72440D95_B468_4C72_A992_4896C74E3791__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DetailsServerPage.h : header file
//

#include "PageView.h"
/////////////////////////////////////////////////////////////////////////////
// CDetailsServerView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CDetailsServerView : public CPageView
{
protected:
	CDetailsServerView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CDetailsServerView)

// Form Data
public:

	//{{AFX_DATA(CDetailsServerView)
	enum { IDD = IDD_PAGE_DETAILS_SERVER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDetailsServerView)
	public:
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnDraw(CDC* pDC);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDetailsServerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CDetailsServerView)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DETAILSSERVERPAGE_H__72440D95_B468_4C72_A992_4896C74E3791__INCLUDED_)
