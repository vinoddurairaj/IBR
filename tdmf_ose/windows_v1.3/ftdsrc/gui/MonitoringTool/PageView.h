#if !defined(AFX_PAGEVIEW_H__F7F44E9E_D79E_4519_B6AC_536F158EA8F2__INCLUDED_)
#define AFX_PAGEVIEW_H__F7F44E9E_D79E_4519_B6AC_536F158EA8F2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PageView.h : header file
//
#include "resource.h"
#include "GenericPropPage.h"
#include "Doc.h"

/////////////////////////////////////////////////////////////////////////////
// PageView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class Doc;

class CPageView : public CFormView
{

   public:	
	CPageView();           // protected constructor used by dynamic creation
   CPageView(UINT nIDTemplate);

  protected:

   DECLARE_DYNCREATE(CPageView)
	COLORREF m_crBackground;
	CBrush m_wndbkBrush;	// background brush

	GenericPropPage* m_pParentPage;

	void SetBackgroundColor(COLORREF crBackground);

   // Form Data
   public:
     
		//{{AFX_DATA(CPageView)
	   enum { IDD = IDD_PPG_EMPTY };
		   // NOTE: the ClassWizard will add data members here
	   //}}AFX_DATA

   // Attributes
   public:
  
   // Operations
   public:
		Doc* GetDocument();
      virtual void Update();
   // Overrides
	   // ClassWizard generated virtual function overrides
	   //{{AFX_VIRTUAL(CPageView)
	public:
	virtual void OnInitialUpdate();
	protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   // Implementation
   protected:
	   virtual ~CPageView();
   #ifdef _DEBUG
	   virtual void AssertValid() const;
	   virtual void Dump(CDumpContext& dc) const;
   #endif

	   // Generated message map functions
	   //{{AFX_MSG(CPageView)	
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in RightView.cpp
inline Doc* CPageView::GetDocument()
   { 	
		Doc* pDoc = ((CMainFrame*)AfxGetApp()->m_pMainWnd)->GetDocument();
		return pDoc; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGEVIEW_H__F7F44E9E_D79E_4519_B6AC_536F158EA8F2__INCLUDED_)
