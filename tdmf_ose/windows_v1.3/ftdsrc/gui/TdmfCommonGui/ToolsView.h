#if !defined(AFX_TOOLSVIEW_H__BD7C397A_8574_4A58_8B90_84A0B417CFD6__INCLUDED_)
#define AFX_TOOLSVIEW_H__BD7C397A_8574_4A58_8B90_84A0B417CFD6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ToolsView.h : header file
//

#include "SystemSheet.h"
#include "SystemDetailsPage.h"
#include "SystemEventsPage.h"
#include "DomainDetailsPage.h"
#include "ServerDetailsPage.h"
#include "ServereventsPage.h"
#include "ServerPerformanceMonitorPage.h"
#include "ServerPerformanceReporterPage.h"
#include "ServerCommandsPage.h"
#include "RGDetailsPage.h"
#include "RGCommandsPage.h"
#include "RGReplicationPairsPage.h"
#include "RGEventsPage.h"


/////////////////////////////////////////////////////////////////////////////
// CToolsView view

class CToolsView : public CScrollView
{
protected:
	CToolsView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CToolsView)

	void RemoveAllPages();

// Attributes
protected:
	CSystemSheet*           m_pSystemSheet;

	CSystemEventsPage       m_SystemEventsPage;
	CSystemDetailsPage      m_SystemDetailsPage;

	CDomainDetailsPage      m_DomainDetailsPage;

	CServerDetailsPage              m_ServerDetailsPage;
	CServerEventsPage               m_ServerEventsPage;
	CServerPerformanceMonitorPage   m_ServerPerformanceMonitorPage;
//	CServerPerformanceReporterPage  m_ServerPerformanceReporterPage;
	CServerCommandsPage             m_ServerCommandsPage;

	CRGDetailsPage          m_RGDetailsPage;
	CRGEventsPage           m_RGEventsPage;
	CRGReplicationPairsPage m_RGReplicationPairsPage;
	CRGCommandsPage         m_RGCommandsPage;

 	int m_nPageSet;
	int m_nSystemActivePage;
	int m_nServerActivePage;
	int m_nRGActivePage;

	CSize m_Size;
	bool  m_bScrollbarsEnabled;

public:

	void EnableScrollBars(bool bEnable = true);
	bool IsScrollBarsEnabled() {return m_bScrollbarsEnabled;}

// Operations
public:


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CToolsView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnDraw(CDC* pDC);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CToolsView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CToolsView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TOOLSVIEW_H__BD7C397A_8574_4A58_8B90_84A0B417CFD6__INCLUDED_)
