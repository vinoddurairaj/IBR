///////////////////////////////////////////////////////////////////////////////
// File        : InfoBar.h
// Description : This class implement a info bar that display a title in a bar
//               
// Author      : Mario Parent (22-May-2001)
//

#if !defined(AFX_INFOBAR_H__C789E26C_DA4B_11D2_BF44_006008085F93__INCLUDED_)
#define AFX_INFOBAR_H__C789E26C_DA4B_11D2_BF44_006008085F93__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InfoBar.h : header file
//
 
/////////////////////////////////////////////////////////////////////////////
// CInfoBar window

class CInfoBar : public CControlBar
{
// Construction
public:
	CInfoBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInfoBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	void		SetTextColor(COLORREF crNew);
	void		SetBackgroundColor(COLORREF cr);
	BOOL		SetTextFont(LPCTSTR lpFontName);
	BOOL		SetFont(LOGFONT lf);
	BOOL		SetBitmap(UINT nResID);
	void		SetText(LPCTSTR lpszNew);
	virtual ~CInfoBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CInfoBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg LRESULT OnSizeParent(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

//attributes
	int		m_cxAvailable;
	CFont		m_font;
	CString	m_caption;
	CBitmap	m_bm;

// Operations
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);

private:
	COLORREF	m_crBackgroundColor;
	COLORREF	m_crTextColor;
	CSize		m_sizeBitmap;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INFOBAR_H__C789E26C_DA4B_11D2_BF44_006008085F93__INCLUDED_)
