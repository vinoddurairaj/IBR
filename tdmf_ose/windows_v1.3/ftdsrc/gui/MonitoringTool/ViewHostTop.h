#if !defined(AFX_VIEWHOSTTOP_H__A9985D9E_ADDA_406C_A69A_8CB7CFF76355__INCLUDED_)
#define AFX_VIEWHOSTTOP_H__A9985D9E_ADDA_406C_A69A_8CB7CFF76355__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewHostTop.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ViewHostTop form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "PageView.h"

class ViewHostTop : public CPageView
{
   protected:
	   ViewHostTop();           // protected constructor used by dynamic creation
	   DECLARE_DYNCREATE(ViewHostTop)

   // Form Data
   public:
	   //{{AFX_DATA(ViewHostTop)
	enum { IDD = IDD_VIEW_HOST_TOP };
	   CIPAddressCtrl	m_IPAddress;
	   CProgressCtrl	m_PrgrBABFilled;
	CString	m_strBarFilled;
	CString	m_strServerName;
	CString	m_strDomain;
	BOOL	m_bIsSource;
	BOOL	m_bIsTarget;
	//}}AFX_DATA

   // Attributes
   public:

   // Operations
   public:
      virtual void Update();

   // Overrides
	   // ClassWizard generated virtual function overrides
	   //{{AFX_VIRTUAL(ViewHostTop)
	   public:
	   virtual void OnInitialUpdate();
	   protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   virtual ~ViewHostTop();
   #ifdef _DEBUG
	   virtual void AssertValid() const;
	   virtual void Dump(CDumpContext& dc) const;
   #endif

	   // Generated message map functions
	   //{{AFX_MSG(ViewHostTop)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWHOSTTOP_H__A9985D9E_ADDA_406C_A69A_8CB7CFF76355__INCLUDED_)
