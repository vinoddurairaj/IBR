#if !defined(AFX_VIEWOVERVIEWTOP_H__B411C572_68FE_465C_A9B2_EFAEDFA9861B__INCLUDED_)
#define AFX_VIEWOVERVIEWTOP_H__B411C572_68FE_465C_A9B2_EFAEDFA9861B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewOverviewTop.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ViewOverviewTop form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "PageView.h"

class ViewOverviewTop : public CPageView
{
protected:
	ViewOverviewTop();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(ViewOverviewTop)

// Form Data
public:
	//{{AFX_DATA(ViewOverviewTop)
	enum { IDD = IDD_VIEW_OVVW_TOP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ViewOverviewTop)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~ViewOverviewTop();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(ViewOverviewTop)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWOVERVIEWTOP_H__B411C572_68FE_465C_A9B2_EFAEDFA9861B__INCLUDED_)
