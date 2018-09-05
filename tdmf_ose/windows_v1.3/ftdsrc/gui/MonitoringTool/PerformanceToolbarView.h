#if !defined(AFX_PERFORMANCETOOLBARVIEW_H__B23338BB_FA65_4900_B363_64BFA2BE12EE__INCLUDED_)
#define AFX_PERFORMANCETOOLBARVIEW_H__B23338BB_FA65_4900_B363_64BFA2BE12EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PerformanceToolbarView.h : header file
//
#include "Doc.h"
/////////////////////////////////////////////////////////////////////////////
// CPerformanceToolbarView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "PageView.h"

class CPerformanceToolbarView : public CPageView
{
protected:
	CPerformanceToolbarView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CPerformanceToolbarView)

// Form Data
public:
	//{{AFX_DATA(CPerformanceToolbarView)
	enum { IDD = IDD_PAGE_TOOLBAR };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:
    CToolBar		m_ToolBar;
    CPageView*		m_pPerformanceView;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPerformanceToolbarView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CPerformanceToolbarView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CPerformanceToolbarView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDrawBar();
	afx_msg void OnUpdateDrawBar(CCmdUI* pCmdUI);
	afx_msg void OnDraw3dBar();
	afx_msg void OnUpdateDraw3dBar(CCmdUI* pCmdUI);
	afx_msg void OnDraw3dLine();
	afx_msg void OnUpdateDraw3dLine(CCmdUI* pCmdUI);
	afx_msg void OnDraw3dPie();
	afx_msg void OnUpdateDraw3dPie(CCmdUI* pCmdUI);
	afx_msg void OnDraw3dStackedBar();
	afx_msg void OnUpdateDraw3dStackedBar(CCmdUI* pCmdUI);
	afx_msg void OnDrawLine();
	afx_msg void OnUpdateDrawLine(CCmdUI* pCmdUI);
	afx_msg void OnDrawPie();
	afx_msg void OnUpdateDrawPie(CCmdUI* pCmdUI);
	afx_msg void OnDrawScatter();
	afx_msg void OnUpdateDrawScatter(CCmdUI* pCmdUI);
	afx_msg void OnDrawStackedBar();
	afx_msg void OnUpdateDrawStackedBar(CCmdUI* pCmdUI);
	afx_msg void OnDrawWhisker();
	afx_msg void OnUpdateDrawWhisker(CCmdUI* pCmdUI);
	afx_msg void OnDrawXyLine();
	afx_msg void OnUpdateDrawXyLine(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERFORMANCETOOLBARVIEW_H__B23338BB_FA65_4900_B363_64BFA2BE12EE__INCLUDED_)
