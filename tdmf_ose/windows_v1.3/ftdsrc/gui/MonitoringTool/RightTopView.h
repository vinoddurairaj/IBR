// RightTopView.h : interface of the RightTopView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RIGHTTOPVIEW_H__16FF0FD7_F6EA_4993_8858_2146A137A4EE__INCLUDED_)
#define AFX_RIGHTTOPVIEW_H__16FF0FD7_F6EA_4993_8858_2146A137A4EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropSheetFrame.h"
#include "PropPage.h"
#include "PageView.h"

class CRightTopView : public CPageView
{
   protected:
      PropSheet*					m_pPropSheet;
      PropSheetFrame*			m_pPropFrame;
      PropPg_Monitoring			m_PPageMonitoring;
      PropPg_Administration	m_PPageAdministration;
	

   // Attributes
   public:

   // Operations
   public:

   // Implementation
   protected: // create from serialization only
      CRightTopView();

   public:
      void OnProperties();
      virtual ~CRightTopView();

   #ifdef _DEBUG
      virtual void AssertValid() const;
      virtual void Dump(CDumpContext& dc) const;
   #endif

   // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(CRightTopView)
	public:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
   virtual void OnInitialUpdate();
	protected:
   virtual void OnDraw(CDC* pDC);
	virtual void OnUpdate(CPageView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

   // Generated message map functions
   protected:
      //{{AFX_MSG(CRightTopView)
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnDestroy();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
      DECLARE_MESSAGE_MAP()
      DECLARE_DYNCREATE(CRightTopView)
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIGHTTOPVIEW_H__16FF0FD7_F6EA_4993_8858_2146A137A4EE__INCLUDED_)
