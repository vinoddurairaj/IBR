#if !defined(AFX_VIEWOVERVIEWBOTTOM_H__654B1943_5EFE_45D1_B8BB_98B6589CB087__INCLUDED_)
#define AFX_VIEWOVERVIEWBOTTOM_H__654B1943_5EFE_45D1_B8BB_98B6589CB087__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewOverviewBottom.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ViewOverviewBottom form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "PageView.h"

class ViewOverviewBottom : public CPageView
{
protected:
	ViewOverviewBottom();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(ViewOverviewBottom)

// Form Data
public:
	//{{AFX_DATA(ViewOverviewBottom)
	enum { IDD = IDD_VIEW_OVVW_BOTTOM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ViewOverviewBottom)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~ViewOverviewBottom();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(ViewOverviewBottom)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWOVERVIEWBOTTOM_H__654B1943_5EFE_45D1_B8BB_98B6589CB087__INCLUDED_)
