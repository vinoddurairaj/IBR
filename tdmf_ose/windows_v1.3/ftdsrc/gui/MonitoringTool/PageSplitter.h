#if !defined(AFX_PAGESPLITTER_H__44737E17_A02F_46F8_B50E_181FC8A3AB71__INCLUDED_)
#define AFX_PAGESPLITTER_H__44737E17_A02F_46F8_B50E_181FC8A3AB71__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PageSplitter.h : header file
//
#include "MainFrm.h"
#include "SplitterFrame.h"

struct SPanelData
{
   CRuntimeClass* pCRuntimeClass;
};

class PageSplitter : public CSplitterWnd
{
   protected:
      SplitterFrame* m_pSplitterFrame;
      class GenericPropPage* m_pParentPage;
      SPanelData* m_pPanelArray;
      int m_nRows;
      int m_nCols;
      CMainFrame* m_pMainFrame;

   public:
      PageSplitter();
      virtual ~PageSplitter();
      BOOL Construct(GenericPropPage* pParentPage,
                     SPanelData* pPanelArray, int nRows, int nCols);

      SplitterFrame* GetFrame()
      {
         return m_pSplitterFrame;
      }

      void UpdateAllViews();
		void UpdateAllViews(UINT Message);

      // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(PageSplitter)
      //}}AFX_VIRTUAL

      // Generated message map functions
   protected:
      //{{AFX_MSG(PageSplitter)
         // NOTE - the ClassWizard will add and remove member functions here.
      //}}AFX_MSG
      DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGESPLITTER_H__44737E17_A02F_46F8_B50E_181FC8A3AB71__INCLUDED_)
