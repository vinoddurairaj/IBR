#if !defined(AFX_TESTVIEW_H__592CE199_7FDF_45E0_BD42_4FA8C3283123__INCLUDED_)
#define AFX_TESTVIEW_H__592CE199_7FDF_45E0_BD42_4FA8C3283123__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TestView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// TestView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "PageView.h"

class TestView : public CPageView
{
protected:
	TestView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(TestView)

// Form Data
public:
	//{{AFX_DATA(TestView)
	enum { IDD = IDD_PPG_EMPTY };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TestView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~TestView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(TestView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTVIEW_H__592CE199_7FDF_45E0_BD42_4FA8C3283123__INCLUDED_)
