#if !defined(AFX_VIEWPAIRTOP_H__FD544287_5224_4F3E_9F37_99B3CDD525FB__INCLUDED_)
#define AFX_VIEWPAIRTOP_H__FD544287_5224_4F3E_9F37_99B3CDD525FB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewPairTop.h : header file
//
 
/////////////////////////////////////////////////////////////////////////////
// ViewPairTop form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "PageView.h"

class ViewPairTop : public CPageView
{
protected:
	ViewPairTop();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(ViewPairTop)

// Form Data
public:
	//{{AFX_DATA(ViewPairTop)
	enum { IDD = IDD_VIEW_PAIR_TOP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ViewPairTop)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~ViewPairTop();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(ViewPairTop)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWPAIRTOP_H__FD544287_5224_4F3E_9F37_99B3CDD525FB__INCLUDED_)
