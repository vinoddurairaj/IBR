// PropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PropSheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropSheet

IMPLEMENT_DYNAMIC(PropSheet, CPropertySheet)

PropSheet::PropSheet(CWnd* pWndParent)
:  CPropertySheet(IDS_PROPSHT_CAPTION, pWndParent)
{
   m_rectPages.SetRectEmpty();
	m_bBackgroundLogo = false;
}

PropSheet::~PropSheet()
{
}


BEGIN_MESSAGE_MAP(PropSheet, CPropertySheet)
   //{{AFX_MSG_MAP(PropSheet)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// PropSheet message handlers

BOOL PropSheet::Create(CWnd* pParentWnd, DWORD dwStyle, DWORD dwExStyle	)
{
	
	BOOL bRet = CPropertySheet::Create(pParentWnd, dwStyle, dwExStyle);
//   if(bRet)
//   {
//		if(m_bBackgroundLogo)
//		{
//			bRet = m_WndLogo.Init(this, 1 /* IDD_LOGOWINDOW */);
//			m_WndLogo.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
//		}
//   }

   return bRet;
}

void PropSheet::PostNcDestroy()
{
   CPropertySheet::PostNcDestroy();
   delete this;
}

void PropSheet::Resize(int cx, int cy)
{
	ASSERT(IsWindow(m_hWnd));

   // resize the property sheet:
   MoveWindow(0, 0, cx, cy);


	  // resize the logo window:
//	   m_WndLogo.Resize(cx);

   // move and resize the tab control, making room for the background logo:
   CRect rectTabs;
   CTabCtrl* pTabCtrl = GetTabControl();
   ASSERT(pTabCtrl->GetItemCount());
   pTabCtrl->GetItemRect(0, rectTabs);
 
	int nTopOffset = 0;
 
/*	if(m_bBackgroundLogo)
	{
		nTopOffset = m_WndLogo.GetHeight() - rectTabs.bottom;
	   pTabCtrl->MoveWindow(0, nTopOffset, cx, cy - nTopOffset);
	}
*/  
   pTabCtrl->MoveWindow(0, nTopOffset, cx, cy - nTopOffset);

   // calculate and save the dimensions and position of each page:
   pTabCtrl->GetWindowRect(&m_rectPages);
	pTabCtrl->AdjustRect(FALSE, &m_rectPages);
   ScreenToClient(&m_rectPages);

   // resize the active page:
   GenericPropPage* pPage = (GenericPropPage*)GetActivePage();
   ASSERT(pPage);
   pPage->Resize(m_rectPages);
}

BOOL PropSheet::OnEraseBkgnd(CDC* pDC) 
{
   return TRUE;
}

void PropSheet::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
/*	if(m_bBackgroundLogo)
	{
		m_WndLogo.InvalidateRect(NULL);
	}
*/
	// Do not call CPropertySheet::OnPaint() for painting messages
}

/******************************************************************************
**
**   CLogoWnd implementation
**
******************************************************************************/

BEGIN_MESSAGE_MAP(LogoWnd, CWnd)
   //{{AFX_MSG_MAP(LogoWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL LogoWnd::Init(PropSheet* pPropSheet, UINT nID)
{
   m_BmpLogo.LoadBitmap(IDB_BITMAP_LOGO);
   m_BmpLogo.GetBitmap(&m_StructLogoBitmap);
   m_pPropSheet = pPropSheet;
   CRect rect;
   pPropSheet->GetClientRect(&rect);
   rect.bottom = m_StructLogoBitmap.bmHeight;
   return Create(NULL, _T(""), WS_VISIBLE | WS_CHILD,
                 rect, pPropSheet, nID, NULL);
}

void LogoWnd::Resize(int nWidth)
{
   MoveWindow(0, 0, nWidth, m_StructLogoBitmap.bmHeight);
}

int LogoWnd::GetHeight()
{
   return m_StructLogoBitmap.bmHeight;
}

BOOL LogoWnd::OnEraseBkgnd(CDC* pDC) 
{
   return TRUE;
}

void LogoWnd::OnPaint() 
{
	// Do not call CWnd::OnPaint() for painting messages
	CPaintDC dc(this); // device context for painting
   DoPaint(m_pPropSheet->GetTabControl());
}

void LogoWnd::DoPaint(CTabCtrl* pTabCtrl)
{
   CDC* pDC = GetDC();
   
   // build a bitmap of the desired background:
   CRect rectAll;
   GetClientRect(&rectAll);

   CDC dcAll;
   CBitmap bitmapAll;
   bitmapAll.CreateCompatibleBitmap(pDC, rectAll.Width(), rectAll.Height());
   dcAll.CreateCompatibleDC(pDC);
   dcAll.SelectObject(&bitmapAll);

   // paint the logo on the right-hand side of the rectangle:
   CDC dcLogo;
   dcLogo.CreateCompatibleDC(pDC);
   dcLogo.SelectObject(&m_BmpLogo);
   dcAll.BitBlt(rectAll.right - m_StructLogoBitmap.bmWidth, rectAll.top,
                m_StructLogoBitmap.bmWidth, m_StructLogoBitmap.bmHeight,
                &dcLogo, 0, 0, SRCCOPY);

   // paint the rest of the window with a solid color:
   CBrush brush;
   brush.CreateSolidBrush(RGB(255, 0, 0));
   dcAll.SelectObject(&brush);
   dcAll.PatBlt(0, 0, rectAll.Width() - m_StructLogoBitmap.bmWidth, rectAll.Height(), PATCOPY);

   // calculate the area covered by the tabs, to avoid painting them:
   int nTotPages = pTabCtrl->GetItemCount();
   if(!nTotPages)
      return;
   int nSelPage = pTabCtrl->GetCurSel();

   CRect* pTabRect = new CRect[nTotPages];

   CRect rectTabs(10000, 10000, 0, 0);
   for(int nPage = 0; nPage < nTotPages; ++nPage)
   {
      pTabCtrl->GetItemRect(nPage, &pTabRect[nPage]);
      
      if(nPage == nSelPage)
         pTabRect[nPage].InflateRect(2, 2);
      
      pTabCtrl->ClientToScreen(&pTabRect[nPage]);
      ScreenToClient(&pTabRect[nPage]);

      if(rectTabs.left > pTabRect[nPage].left)
         rectTabs.left = pTabRect[nPage].left;
      
      if(rectTabs.right < pTabRect[nPage].right)
         rectTabs.right = pTabRect[nPage].right;
      
      if(rectTabs.top > pTabRect[nPage].top)
         rectTabs.top = pTabRect[nPage].top;
      
      if(rectTabs.bottom < pTabRect[nPage].bottom)
         rectTabs.bottom = pTabRect[nPage].bottom;
   }

   // bitmap and DC to save the image of the tabs:
   CDC dcTabs;
   CBitmap bitmapTabs;
   bitmapTabs.CreateCompatibleBitmap(pDC, rectTabs.right, rectTabs.bottom);
   dcTabs.CreateCompatibleDC(pDC);
   dcTabs.SelectObject(&bitmapTabs);

   // copy the background to the tabs bitmap:
   dcTabs.BitBlt(0, 0, rectTabs.right, rectTabs.bottom,
                 &dcAll, 0, 0, SRCCOPY);

   // save the image of each tab:
   struct SCornerPixel
   {
      int nX;
      int nY;
      COLORREF clr;
   };
   
   static SCornerPixel aPxlLeft[3] =
   {
      { 0, 0, 0 },
      { 1, 0, 0 },
      { 0, 1, 0 },
   };

   static SCornerPixel aPxlRight[3] =
   {
      { 1, 0, 0 },
      { 2, 0, 0 },
      { 1, 1, 0 },
   };
   
   for(nPage = 0; nPage < nTotPages; ++nPage)
   {
      int p;
      for(p = 0; p < 3; ++p)
      {
         aPxlLeft[p].clr = dcTabs.GetPixel(pTabRect[nPage].left + aPxlLeft[p].nX,
                                           pTabRect[nPage].top + aPxlLeft[p].nY);
         aPxlRight[p].clr = dcTabs.GetPixel(pTabRect[nPage].right - aPxlRight[p].nX,
                                           pTabRect[nPage].top + aPxlRight[p].nY);
      }

      dcTabs.BitBlt(pTabRect[nPage].left, pTabRect[nPage].top,
                    pTabRect[nPage].Width(), pTabRect[nPage].Height(), 
                    pDC, pTabRect[nPage].left, pTabRect[nPage].top, SRCCOPY);

      for(p = 0; p < 3; ++p)
      {
         dcTabs.SetPixel(pTabRect[nPage].left + aPxlLeft[p].nX,
                         pTabRect[nPage].top + aPxlLeft[p].nY,
                         aPxlLeft[p].clr);
         dcTabs.SetPixel(pTabRect[nPage].right - aPxlRight[p].nX,
                         pTabRect[nPage].top + aPxlRight[p].nY,
                         aPxlRight[p].clr);
      }
   }

   // copy the saved tabs to the whole image:
   dcAll.BitBlt(rectTabs.left, rectTabs.top, rectTabs.Width(), rectTabs.Height(),
                &dcTabs, rectTabs.left, rectTabs.top, SRCCOPY);

   // finally blit the whole thing to the screen:
   pDC->BitBlt(0, 0, rectAll.Width(), rectAll.Height(),
               &dcAll, 0, 0, SRCCOPY);
   delete [] pTabRect;
}

