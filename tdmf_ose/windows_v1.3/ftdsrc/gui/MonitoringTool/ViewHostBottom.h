#if !defined(AFX_VIEWHOSTBOTTOM_H__9CC663DE_96FC_4673_AF15_88F564891AF7__INCLUDED_)
#define AFX_VIEWHOSTBOTTOM_H__9CC663DE_96FC_4673_AF15_88F564891AF7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewHostBottom.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ViewHostBottom form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "PageView.h"

class ViewHostBottom : public CPageView
{
protected:
	ViewHostBottom();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(ViewHostBottom)

// Form Data
public:
	//{{AFX_DATA(ViewHostBottom)
	enum { IDD = IDD_VIEW_HOST_BOTTOM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ViewHostBottom)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~ViewHostBottom();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(ViewHostBottom)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWHOSTBOTTOM_H__9CC663DE_96FC_4673_AF15_88F564891AF7__INCLUDED_)
