// LeftView.h : interface of the CLeftView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_LEFTVIEW_H__CA55E9C7_ED92_4CA1_97DE_C150992C1229__INCLUDED_)
#define AFX_LEFTVIEW_H__CA55E9C7_ED92_4CA1_97DE_C150992C1229__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "PageView.h"


class Doc;


class CLeftView : public CPageView
{
   protected:
      CTreeCtrl  m_Tree;
      CImageList m_TreeImageList;
      HTREEITEM  m_hTreeRoot;

   protected: // create from serialization only
      CLeftView();
      virtual ~CLeftView();

   // Attributes
   public:
      Doc* GetDocument();

   // Operations
   public:

   // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(CLeftView)
      public:
      virtual void OnDraw(CDC* pDC);  // overridden to draw this view
      virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
      protected:
      virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
      virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
      virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
      virtual void OnInitialUpdate(); // called first time after construct
      //}}AFX_VIRTUAL

   // Implementation
   public:
   #ifdef _DEBUG
      virtual void AssertValid() const;
      virtual void Dump(CDumpContext& dc) const;
   #endif

   // Generated message map functions
   protected:
      //{{AFX_MSG(CLeftView)
      afx_msg void OnSize(UINT nType, int cx, int cy);
   	afx_msg void OnSelChanging(LPNMHDR pnmhdr, LRESULT *pLResult);
   	afx_msg void OnSelChanged(LPNMHDR pnmhdr, LRESULT *pLResult);
	afx_msg void OnPaint();
	//}}AFX_MSG
      DECLARE_MESSAGE_MAP()
      DECLARE_DYNCREATE(CLeftView)
};

#ifndef _DEBUG  // debug version in RightView.cpp
inline Doc* CLeftView::GetDocument()
   { return (Doc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LEFTVIEW_H__CA55E9C7_ED92_4CA1_97DE_C150992C1229__INCLUDED_)
