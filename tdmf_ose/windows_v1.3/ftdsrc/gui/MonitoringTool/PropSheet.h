// PropSheet.h : header file
//
// PropSheet is a modeless property sheet that is 
// created once and not destroyed until the application
// closes.  It is initialized and controlled from
// PropSheetFrame.
 
#ifndef __PROPERTYSHEET_H__
#define __PROPERTYSHEET_H__

#include "GenericPropPage.h"

/////////////////////////////////////////////////////////////////////////////
// PropSheet

class LogoWnd : public CWnd
{
      BITMAP  m_StructLogoBitmap;
      CBitmap m_BmpLogo;
		
      class PropSheet* m_pPropSheet;
   public:
      BOOL Init(PropSheet* pPropSheet, UINT nID);
      void Resize(int nWidth);
      int GetHeight();
      void DoPaint(CTabCtrl* pTabCtrl);
   protected:
      afx_msg BOOL OnEraseBkgnd(CDC* pDC);
      afx_msg void OnPaint();
      DECLARE_MESSAGE_MAP()
}; 

class PropSheet : public CPropertySheet
{
      DECLARE_DYNAMIC(PropSheet)

	public:
		LogoWnd m_WndLogo;
		BOOL m_bBackgroundLogo;
   protected:
      
      CRect   m_rectPages;

   public:
      PropSheet(CWnd* pWndParent = NULL);
      virtual ~PropSheet();
      virtual void PostNcDestroy();
      
      void GetPageRectangle(CRect& rect)
      {
         rect = m_rectPages;
      }
      
      GenericPropPage* GetActivePage( ) const
      {
         return (GenericPropPage*)CPropertySheet::GetActivePage();
      }



   public:
      void Resize(int cx, int cy);
      virtual BOOL Create(CWnd* pParentWnd = NULL, DWORD dwStyle = (DWORD)-1, DWORD dwExStyle = 0);

      // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(PropSheet)
   public:
      //}}AFX_VIRTUAL

   // Generated message map functions
   protected:
      //{{AFX_MSG(PropSheet)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	//}}AFX_MSG
      DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif   // __PROPERTYSHEET_H__
