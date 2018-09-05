// PropSheetFrame.h : header file
//
// This file contains the mini-frame that controls modeless 
// property sheet PropSheet.

#ifndef __PROPSHEETFRAME_H__
#define __PROPSHEETFRAME_H__

#include "PropSheet.h"

/////////////////////////////////////////////////////////////////////////////
// PropSheetFrame frame

class PropSheetFrame : public CMiniFrameWnd
{
      DECLARE_DYNCREATE(PropSheetFrame)
   //Construction
   public:
      PropSheetFrame();

   // Attributes
   public:
      PropSheet* m_pModelessPropSheet;

   // Operations
   public:

   // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(PropSheetFrame)
      //}}AFX_VIRTUAL

   // Implementation
   public:
      virtual ~PropSheetFrame();

      // Generated message map functions
      //{{AFX_MSG(PropSheetFrame)
      afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
      afx_msg void OnClose();
      afx_msg void OnSetFocus(CWnd* pOldWnd);
      afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
      //}}AFX_MSG
      DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif   // __PROPSHEETFRAME_H__
