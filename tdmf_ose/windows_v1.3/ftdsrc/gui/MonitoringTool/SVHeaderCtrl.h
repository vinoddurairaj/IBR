#if !defined(AFX_SVHEADERCTRL_H__A4E624D9_E098_43C9_8304_8F7BF2A87A0B__INCLUDED_)
#define AFX_SVHEADERCTRL_H__A4E624D9_E098_43C9_8304_8F7BF2A87A0B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SVHeaderCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// SVHeaderCtrl window

class SVHeaderCtrl : public CHeaderCtrl
{
// Construction
public:
	SVHeaderCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SVHeaderCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~SVHeaderCtrl();

	// Generated message map functions
protected:

	//{{AFX_MSG(SVHeaderCtrl)
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVHEADERCTRL_H__A4E624D9_E098_43C9_8304_8F7BF2A87A0B__INCLUDED_)
