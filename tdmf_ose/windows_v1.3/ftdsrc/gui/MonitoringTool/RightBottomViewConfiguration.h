#if !defined(AFX_RIGHTBOTTOMVIEWCONFIGURATION_H__6D5E9CC4_F261_4BAD_BF6E_8611A6664409__INCLUDED_)
#define AFX_RIGHTBOTTOMVIEWCONFIGURATION_H__6D5E9CC4_F261_4BAD_BF6E_8611A6664409__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RightBottomViewConfiguration.h : header file
//
#include "PropSheetFrame.h"
#include "PropPage.h"
/////////////////////////////////////////////////////////////////////////////
// CRightBottomViewConfiguration view

class CRightBottomViewConfiguration : public CView
{
protected:
	CRightBottomViewConfiguration();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CRightBottomViewConfiguration)

// Attributes
	PropSheet*						m_pPropSheet;
   PropSheetFrame*				m_pPropFrame;
   PropPg_ReplicationGroups	m_ReplicationGroups;
	PropPg_ServerEvents			m_ServerEvents;
	PropPg_ServerCommand			m_ServerCommand;

// Attributes
public:

// Operations
public:
   Doc* GetDocument();
	void OnProperties();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRightBottomViewConfiguration)
	public:
   virtual void OnInitialUpdate();
	protected:
   virtual void OnDraw(CDC* pDC);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CRightBottomViewConfiguration();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CRightBottomViewConfiguration)
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnDestroy();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in CRightBottomView.cpp
inline Doc* CRightBottomView::GetDocument()
   { return (Doc*)m_pDocument; }
#endif
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIGHTBOTTOMVIEWCONFIGURATION_H__6D5E9CC4_F261_4BAD_BF6E_8611A6664409__INCLUDED_)
