#if !defined(AFX_SPLITTERFRAME_H__7C2EB29E_2579_4659_8818_974A4119F465__INCLUDED_)
#define AFX_SPLITTERFRAME_H__7C2EB29E_2579_4659_8818_974A4119F465__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SplitterFrame.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// SplitterFrame frame

class SplitterFrame : public CFrameWnd
{
   // Attributes
   public:

   // Operations
   public:

   // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(SplitterFrame)
      //}}AFX_VIRTUAL

   // Implementation
   protected:

      // Generated message map functions
      //{{AFX_MSG(SplitterFrame)
      afx_msg BOOL OnEraseBkgnd(CDC* pDC);
      //}}AFX_MSG
      DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPLITTERFRAME_H__7C2EB29E_2579_4659_8818_974A4119F465__INCLUDED_)
