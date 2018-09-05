// PageSplitter.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "GenericPropPage.h"
#include "PageSplitter.h"
#include "PageView.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PageSplitter

PageSplitter::PageSplitter()
{
}

PageSplitter::~PageSplitter()
{
}

BOOL PageSplitter::Construct(GenericPropPage* pParentPage,
                             SPanelData* pPanelArray, int nRows, int nCols)
{
	// Establish main frame pointer
	m_pMainFrame = (CMainFrame*) AfxGetMainWnd();

   m_pParentPage = pParentPage;
   m_pPanelArray = pPanelArray;
   m_nRows = nRows;
   m_nCols = nCols;
   CRect rc;
   m_pParentPage->GetClientRect(&rc);
   m_pSplitterFrame = new SplitterFrame();
	BOOL b = m_pSplitterFrame->Create(NULL, _T(""), WS_VISIBLE | WS_CHILD, rc, m_pParentPage);
   ASSERT(b);
	
   if(!CreateStatic(m_pSplitterFrame, nRows, nCols))
      return FALSE;
   
	CSize sz(rc.Height() / nRows, rc.Width() / nCols);

   for(int row = 0; row < m_nRows; ++row)
   {
      for(int col = 0; col < m_nCols; ++col)
      {

         if(!CreateView(row, col, pPanelArray[row * m_nCols + col].pCRuntimeClass, sz, NULL))
            return FALSE;
		
			CPageView* pPaneView = (CPageView*)GetPane(row, col);
			pPaneView->m_pParentPage = m_pParentPage;
      }
   }
   return TRUE;
}

void PageSplitter::UpdateAllViews()
{
   for(int row = 0; row < m_nRows; ++row)
   {
      for(int col = 0; col < m_nCols; ++col)
      {
         CPageView* pPaneView = (CPageView*)GetPane(row, col);
			pPaneView->m_pParentPage = m_pParentPage;
			pPaneView->Update();
      }
   }
}

void PageSplitter::UpdateAllViews(UINT Message)
{
   for(int row = 0; row < m_nRows; ++row)
   {
      for(int col = 0; col < m_nCols; ++col)
      {
         CPageView* pPaneView = (CPageView*)GetPane(row, col);
		   pPaneView->SendMessage( Message);
      }
   }
}


BEGIN_MESSAGE_MAP(PageSplitter, CSplitterWnd)
	//{{AFX_MSG_MAP(PageSplitter)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// PageSplitter message handlers
