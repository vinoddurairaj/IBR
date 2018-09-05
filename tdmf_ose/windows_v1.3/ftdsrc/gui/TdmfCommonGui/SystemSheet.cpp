// SystemSheet.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "ViewNotification.h"
#include "ToolsView.h"
#include "SystemSheet.h"
#include "PropertyPageBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSystemSheet

IMPLEMENT_DYNAMIC(CSystemSheet, CPropertySheet)

CSystemSheet::CSystemSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	: CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CSystemSheet::CSystemSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CSystemSheet::~CSystemSheet()
{
}

CSystemSheet::CSystemSheet(CWnd* pWndParent)
	:CPropertySheet(AFX_IDS_APP_TITLE, pWndParent)
{
	AddPage(&m_EmptyPage);
}

void CSystemSheet::Resize(int x, int y, int cx, int cy)
{
	MoveWindow(x, y, cx, cy);
	
	// Get Tab control
	CTabCtrl* pTabCtrl = GetTabControl();
	
	HWND hTabWnd = (HWND)::GetDlgItem(m_hWnd, AFX_IDC_TAB_CONTROL);
	if (hTabWnd == NULL)
	{
		hTabWnd = pTabCtrl->m_hWnd;
	}
	
	if (!::IsWindow(hTabWnd)) return;
	
	CRect tabRect;
	GetClientRect(&tabRect);
	// Resize the tab control
	::SetWindowPos(hTabWnd, NULL, 0, 0, tabRect.Width(), tabRect.Height(), SWP_NOZORDER);
	::InvalidateRect(hTabWnd, NULL, FALSE);
	::SendMessage(hTabWnd, TCM_ADJUSTRECT, FALSE, (LPARAM)&tabRect);
	::MapWindowPoints(hTabWnd, m_hWnd, (LPPOINT)&tabRect, (sizeof(RECT)/sizeof(POINT))/*2*/);
	
	// resize the page
	int nPageIndex = pTabCtrl->GetCurSel();
	CPropertyPage* pPage = GetPage(nPageIndex);
	ASSERT(pPage);

	pPage->MoveWindow(&tabRect);
}


BEGIN_MESSAGE_MAP(CSystemSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CSystemSheet)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSystemSheet message handlers


int CSystemSheet::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	// Set for Scrolling Tabs style
    EnableStackedTabs(FALSE);

	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_hAccelTable = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME)); 

	return 0;
}

BOOL CSystemSheet::PreTranslateMessage(MSG* pMsg) 
{
	if ((pMsg->message == WM_KEYDOWN) &&
		(pMsg->wParam == VK_TAB))
	{
		if (GetFocus() == GetTabControl())
		{
			CViewNotification  VN;
			CToolsView*        pToolsView = (CToolsView*)GetParent();
			CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)pToolsView->GetDocument();

			if (GetKeyState(VK_SHIFT) & ~1)
			{
				VN.m_nMessageId = CViewNotification::TAB_TOOLS_PREVIOUS;
				pDoc->UpdateAllViews(pToolsView, CViewNotification::VIEW_NOTIFICATION, &VN);

				return TRUE;
			}

			CPropertyPageBase* pPage = dynamic_cast<CPropertyPageBase*>(GetActivePage());

			if (pPage && (pPage->CaptureTabKey() == FALSE))
			{
				VN.m_nMessageId = CViewNotification::TAB_TOOLS_NEXT;
				pDoc->UpdateAllViews(pToolsView, CViewNotification::VIEW_NOTIFICATION, &VN);

				return TRUE;
			}
		}
	}

	if (m_hAccelTable)
	{
		if (TranslateAccelerator(m_hWnd, m_hAccelTable, pMsg))
		{
			return(TRUE);
		}
	}

	return CPropertySheet::PreTranslateMessage(pMsg);
}

void CSystemSheet::RemovePage(int nPage)
{
	CPropertyPage* pPropertyPage = GetPage(nPage);

	// Sometimes, before deleting a page it activate it... that can cause an inacceptable delay
	// Tell the page that we're about to delete it, so don't do anything fancy in SetActive()
	CPropertyPageBase* pPropertyPageBase = dynamic_cast<CPropertyPageBase*>(pPropertyPage);
	if (pPropertyPageBase != NULL)
	{
		pPropertyPageBase->DisableSetActive();
	}

	CPropertySheet::RemovePage(nPage);
}

BOOL CSystemSheet::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;	
}
