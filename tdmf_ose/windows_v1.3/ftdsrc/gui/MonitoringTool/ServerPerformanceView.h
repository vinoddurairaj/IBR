#if !defined(AFX_SERVERPERFORMANCEVIEW_H__F5CB2D06_FB33_4B34_811B_EE73A4E33160__INCLUDED_)
#define AFX_SERVERPERFORMANCEVIEW_H__F5CB2D06_FB33_4B34_811B_EE73A4E33160__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerPerformanceView.h : header file
//
#include "Doc.h"
#include "PageView.h"
#include "Graph.h"
/////////////////////////////////////////////////////////////////////////////
// CServerPerformanceView 

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CServerPerformanceView : public CPageView
{
protected:
	CServerPerformanceView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CServerPerformanceView)

// Form Data
public:
	//{{AFX_DATA(CServerPerformanceView)
	enum { IDD = IDD_PAGE_SERVER_PERFORMANCE };
	CComboBox	m_CBO_GraphTypeCtrl;
	CGraph	m_GraphCtrl;
	//}}AFX_DATA

// Attributes
public:
   BOOL graphComplete;
// Operations
public:
 
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerPerformanceView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnDraw(CDC* pDC);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CServerPerformanceView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CServerPerformanceView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDrawBar();
	afx_msg void OnDrawLine();
	afx_msg void OnDrawPie();
	afx_msg void OnDrawScatter();
	afx_msg void OnDrawWhisker();
	afx_msg void OnDraw3dBar();
	afx_msg void OnDraw3dLine();
	afx_msg void OnDrawStackedBar();
	afx_msg void OnDrawXyLine();
	afx_msg void OnDraw3dPie();
	afx_msg void OnDraw3dStackedBar();
	afx_msg void OnSelchangeCboGraphtype();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERPERFORMANCEVIEW_H__F5CB2D06_FB33_4B34_811B_EE73A4E33160__INCLUDED_)
