// DetailsServerPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "DetailsServerPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDetailsServerView

IMPLEMENT_DYNCREATE(CDetailsServerView, CPageView)

CDetailsServerView::CDetailsServerView()
	: CPageView(CDetailsServerView::IDD)
{
	//{{AFX_DATA_INIT(CDetailsServerView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CDetailsServerView::~CDetailsServerView()
{
}

void CDetailsServerView::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDetailsServerView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDetailsServerView, CPageView)
	//{{AFX_MSG_MAP(CDetailsServerView)
	ON_WM_CTLCOLOR()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDetailsServerView diagnostics

#ifdef _DEBUG
void CDetailsServerView::AssertValid() const
{
	CPageView::AssertValid();
}

void CDetailsServerView::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDetailsServerView message handlers

void CDetailsServerView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) 
{
	CPageView::OnPrepareDC(pDC, pInfo);
}

HBRUSH CDetailsServerView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPageView::OnCtlColor(pDC, pWnd, nCtlColor);
	
	HBRUSH hBrush = NULL; 
	if(nCtlColor == CTLCOLOR_STATIC) 
	{ 
	// Here if Static or disabled controls 
	pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT)); 
	pDC->SetBkColor(COLOR_LEVEL1); 
	hBrush = CreateSolidBrush(COLOR_LEVEL1); 
	} 
	else 
	hBrush = CFormView::OnCtlColor(pDC, pWnd, nCtlColor); 

	return hBrush; 

}



void CDetailsServerView::OnDraw(CDC* pDC) 
{

}

int CDetailsServerView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPageView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void CDetailsServerView::OnInitialUpdate() 
{
	CPageView::OnInitialUpdate();
	
	SetBackgroundColor(COLOR_LEVEL1);		
}
