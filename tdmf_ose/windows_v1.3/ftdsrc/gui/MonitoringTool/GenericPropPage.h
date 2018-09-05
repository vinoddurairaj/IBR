#if !defined(AFX_GENERICPROPPAGE_H__C635E228_BA0A_4811_B15F_BDEBFDE78588__INCLUDED_)
#define AFX_GENERICPROPPAGE_H__C635E228_BA0A_4811_B15F_BDEBFDE78588__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GenericPropPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// GenericPropPage dialog

#include "PageSplitter.h"
#include "resource.h"


class GenericPropPage : public CPropertyPage
{
   DECLARE_DYNCREATE(GenericPropPage)

      BOOL Construct(class PropSheet* pParentSheet)
      {
         return FALSE;
      }

   protected:
      int m_nRows;
      int m_nCols;
      SPanelData* m_pPanelArray;

      PropSheet* m_pParentSheet;
      PageSplitter m_PageSplitter;

      BOOL Construct(PropSheet* pParentSheet,
                     SPanelData* pPanelArray, int nRows, int nCols,
                     UINT nIDCaption = 0, UINT nIDTemplate = IDD_PPG_EMPTY);
      void Resize();

   public:
      GenericPropPage();
      ~GenericPropPage();

      void Resize(const CRect& rect);
      void Update();


      // Dialog Data
      //{{AFX_DATA(GenericPropPage)
         // NOTE - ClassWizard will add data members here.
         //    DO NOT EDIT what you see in these blocks of generated code !
      //}}AFX_DATA


      // Overrides
      // ClassWizard generate virtual function overrides
      //{{AFX_VIRTUAL(GenericPropPage)
      public:
      virtual BOOL OnSetActive();
      protected:
      virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
      //}}AFX_VIRTUAL

      // Implementation
      protected:
         // Generated message map functions
         //{{AFX_MSG(GenericPropPage)
      virtual BOOL OnInitDialog();
      //}}AFX_MSG
         DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENERICPROPPAGE_H__C635E228_BA0A_4811_B15F_BDEBFDE78588__INCLUDED_)

