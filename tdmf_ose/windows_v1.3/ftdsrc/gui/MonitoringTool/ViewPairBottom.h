#if !defined(AFX_VIEWPAIRBOTTOM_H__D353ECD9_D589_4785_AB3D_F5F5182BC9F4__INCLUDED_)
#define AFX_VIEWPAIRBOTTOM_H__D353ECD9_D589_4785_AB3D_F5F5182BC9F4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewPairBottom.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ViewPairBottom form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "PageView.h"

class ViewPairBottom : public CPageView
{
protected:
	ViewPairBottom();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(ViewPairBottom)

// Form Data
public:
	//{{AFX_DATA(ViewPairBottom)
	enum { IDD = IDD_VIEW_PAIR_BOTTOM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ViewPairBottom)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~ViewPairBottom();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(ViewPairBottom)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWPAIRBOTTOM_H__D353ECD9_D589_4785_AB3D_F5F5182BC9F4__INCLUDED_)
