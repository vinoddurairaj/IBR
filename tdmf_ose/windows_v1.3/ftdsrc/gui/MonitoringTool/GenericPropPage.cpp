// GenericPropPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "GenericPropPage.h"
#include "PropSheet.h"
#include "PageView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(GenericPropPage, CPropertyPage)

GenericPropPage::GenericPropPage() : m_pPanelArray(NULL)
{
}

GenericPropPage::~GenericPropPage()
{
}

BOOL GenericPropPage::Construct(PropSheet* pParentSheet,
                                SPanelData* pPanelArray, int nRows, int nCols,
                                UINT nIDCaption /* = 0 */,
                                UINT nIDTemplate /* = IDD_PPG_EMPTY */)
{
	ASSERT(pParentSheet);
   m_pParentSheet = pParentSheet;
   m_nRows = nRows;
   m_nCols = nCols;
   m_pPanelArray = pPanelArray;
   CPropertyPage::Construct(nIDTemplate, nIDCaption);
	m_pParentSheet->AddPage(this);
   return TRUE;
}

void GenericPropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(GenericPropPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(GenericPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(GenericPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL GenericPropPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_PageSplitter.Construct(this, m_pPanelArray, m_nRows, m_nCols);
	
	// return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
	return TRUE;
}

BOOL GenericPropPage::OnSetActive() 
{
   Resize();

   SplitterFrame* pSplitterFrame = m_PageSplitter.GetFrame();
   if(pSplitterFrame != NULL && ::IsWindow(pSplitterFrame->m_hWnd))
		pSplitterFrame->ShowWindow(SW_SHOW);

	 CPageView* pPaneView = (CPageView*)m_PageSplitter.GetActivePane();
    if(pPaneView)
		 pPaneView->Update();


   return CPropertyPage::OnSetActive();
}

void GenericPropPage::Resize(const CRect& rect)
{
   SplitterFrame* pSplitterFrame = m_PageSplitter.GetFrame();
   if(pSplitterFrame)
   {
      MoveWindow(rect);
      pSplitterFrame->MoveWindow(0, 0, rect.Width(), rect.Height());
   }
}

void GenericPropPage::Resize() 
{
   CRect rect;
	m_pParentSheet->GetPageRectangle(rect);
   Resize(rect);
}

void GenericPropPage::Update()
{
   m_PageSplitter.UpdateAllViews();
}

