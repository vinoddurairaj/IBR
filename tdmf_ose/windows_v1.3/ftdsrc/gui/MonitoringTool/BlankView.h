#if !defined(AFX_BLANKVIEW_H__BD2E9FAF_607A_4DE2_B0F3_79A2FF8E9B87__INCLUDED_)
#define AFX_BLANKVIEW_H__BD2E9FAF_607A_4DE2_B0F3_79A2FF8E9B87__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BlankView.h : header file
//
#include "PageView.h"
/////////////////////////////////////////////////////////////////////////////
// CBlankView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CBlankView : public CPageView
{
protected:
	CBlankView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBlankView)

// Form Data
public:
	//{{AFX_DATA(CBlankView)
	enum { IDD = IDD_BLANK };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBlankView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBlankView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CBlankView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BLANKVIEW_H__BD2E9FAF_607A_4DE2_B0F3_79A2FF8E9B87__INCLUDED_)
