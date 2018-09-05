// TDMFGUI.h : main header file for the TDMFGUI application
//

#if !defined(AFX_TDMFGUI_H__8C76D72B_59B6_4278_9353_E9FD35D1034E__INCLUDED_)
#define AFX_TDMFGUI_H__8C76D72B_59B6_4278_9353_E9FD35D1034E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
   #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// App:
// See TDMFGUI.cpp for the implementation of this class
//

class App : public CWinApp
{
   public:
      App();

   // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(App)
      public:
      virtual BOOL InitInstance();
      //}}AFX_VIRTUAL

   // Implementation
      //{{AFX_MSG(App)
      afx_msg void OnAppAbout();
         // NOTE - the ClassWizard will add and remove member functions here.
         //    DO NOT EDIT what you see in these blocks of generated code !
      //}}AFX_MSG
      DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDMFGUI_H__8C76D72B_59B6_4278_9353_E9FD35D1034E__INCLUDED_)
