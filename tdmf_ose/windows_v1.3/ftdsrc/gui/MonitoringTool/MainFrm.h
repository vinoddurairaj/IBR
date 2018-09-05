// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////
#include "InfoBar.h"

#if !defined(AFX_MAINFRM_H__4C97EC8F_E6C0_4220_B47C_D8BED2DB1EF3__INCLUDED_)
#define AFX_MAINFRM_H__4C97EC8F_E6C0_4220_B47C_D8BED2DB1EF3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CFrameWnd
{
		int				m_nSplitterVpos;
		int				m_nSplitterHpos;
		CSplitterWnd	m_wndSplitter1;
		CSplitterWnd	m_wndSplitter2;
		CRuntimeClass*	m_aryPaneViews[4];
      CStatusBar		m_wndStatusBar;
      CToolBar			m_wndToolBar;
      bool				m_bInitialized;
      CInfoBar			m_InfoBar;

		 
public:
//operation


public:	
		CCreateContext* m_pCreateContext;

   protected: // create from serialization only
      CMainFrame();
   public:
      virtual ~CMainFrame();
      class Doc* GetDocument();
      class CLeftTreeView* GetLeftView();
      class CRightBottomView* GetRightView();

   // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(CMainFrame)
      public:
      virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
      virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
      //}}AFX_VIRTUAL

   // Generated message map functions
   protected:
      //{{AFX_MSG(CMainFrame)
      afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
      afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
      DECLARE_MESSAGE_MAP()
      DECLARE_DYNCREATE(CMainFrame)

   // Debugging
   public:
   #ifdef _DEBUG
      virtual void AssertValid() const;
      virtual void Dump(CDumpContext& dc) const;
   #endif
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__4C97EC8F_E6C0_4220_B47C_D8BED2DB1EF3__INCLUDED_)
