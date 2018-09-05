// RightView.h : interface of the RightView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RIGHTVIEW_H__16FF0FD7_F6EA_4993_8858_2146A137A4EE__INCLUDED_)
#define AFX_RIGHTVIEW_H__16FF0FD7_F6EA_4993_8858_2146A137A4EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropSheetFrame.h"
#include "PropPage.h"
#include "PageView.h"

class CRightBottomView : public CPageView
{
   protected:
      PropSheet*						m_pPropSheet;
      PropSheetFrame*				m_pPropFrame;
		PropPg_ServerVolumes			m_Volumes;
 		PropPg_ServerEvents			m_ServerEvents;
		PropPg_Performance			m_Performance;
		PropPg_ServerCommand			m_ServerCommand;
   // Attributes
   public:

   // Operations
   public:

   // Implementation
   protected: // create from serialization only
      CRightBottomView();

   public:
      void OnProperties();
      virtual ~CRightBottomView();

   #ifdef _DEBUG
      virtual void AssertValid() const;
      virtual void Dump(CDumpContext& dc) const;
   #endif

   // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(CRightBottomView)
	public:
   virtual void OnInitialUpdate();
	protected:
   virtual void OnDraw(CDC* pDC);
	virtual void OnUpdate(CPageView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

   // Generated message map functions
   protected:
      //{{AFX_MSG(CRightBottomView)
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnDestroy();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   //}}AFX_MSG
      DECLARE_MESSAGE_MAP()
      DECLARE_DYNCREATE(CRightBottomView)
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIGHTVIEW_H__16FF0FD7_F6EA_4993_8858_2146A137A4EE__INCLUDED_)
