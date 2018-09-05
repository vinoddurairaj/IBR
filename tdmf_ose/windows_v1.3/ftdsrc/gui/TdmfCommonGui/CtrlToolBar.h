#if !defined(AFX_CTRLTOOLBAR_H__A9A08989_231F_45DC_8E4A_77B8D2EFA8D4__INCLUDED_)
#define AFX_CTRLTOOLBAR_H__A9A08989_231F_45DC_8E4A_77B8D2EFA8D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CtrlToolBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCtrlToolBar window

class CCtrlToolBar : public CToolBar
{
// Construction
public:
	CCtrlToolBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCtrlToolBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCtrlToolBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCtrlToolBar)
	afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CTRLTOOLBAR_H__A9A08989_231F_45DC_8E4A_77B8D2EFA8D4__INCLUDED_)
