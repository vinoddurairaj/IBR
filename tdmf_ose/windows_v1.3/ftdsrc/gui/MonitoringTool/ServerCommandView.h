#if !defined(AFX_SERVERCOMMANDVIEW_H__E684973D_FF85_4D49_BEBE_A806C2F7B227__INCLUDED_)
#define AFX_SERVERCOMMANDVIEW_H__E684973D_FF85_4D49_BEBE_A806C2F7B227__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerCommandView.h : header file
//
#include "Doc.h"
#include "PageView.h"
/////////////////////////////////////////////////////////////////////////////
// CServerCommandView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CServerCommandView : public CPageView
{
protected:
	CServerCommandView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CServerCommandView)

// Form Data
public:
	//{{AFX_DATA(CServerCommandView)
	enum { IDD = IDD_SERVER_COMMAND };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerCommandView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CServerCommandView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CServerCommandView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERCOMMANDVIEW_H__E684973D_FF85_4D49_BEBE_A806C2F7B227__INCLUDED_)
