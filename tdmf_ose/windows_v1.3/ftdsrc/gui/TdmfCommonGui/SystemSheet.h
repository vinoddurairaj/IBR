#if !defined(AFX_SYSTEMSHEET_H__10AD6BAB_EA04_4645_8563_839F357DABC4__INCLUDED_)
#define AFX_SYSTEMSHEET_H__10AD6BAB_EA04_4645_8563_839F357DABC4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemSheet.h : header file
//

#include "EmptyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CSystemSheet

class CSystemSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CSystemSheet)

// Construction
public:
	CSystemSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CSystemSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
protected:
	HACCEL     m_hAccelTable;

public:
	CEmptyPage m_EmptyPage;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSystemSheet)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL
	virtual void RemovePage(int nPage);

// Implementation
public:
	CSystemSheet(CWnd* pWndParent);
	virtual ~CSystemSheet();

	void Resize(int x, int y, int cx, int cy);

	// Generated message map functions
protected:
	//{{AFX_MSG(CSystemSheet)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMSHEET_H__10AD6BAB_EA04_4645_8563_839F357DABC4__INCLUDED_)
