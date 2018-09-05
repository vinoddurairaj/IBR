// RightBottomToolbarView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "RightBottomToolbarView.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRightBottomToolbarView

IMPLEMENT_DYNCREATE(CRightBottomToolbarView, CPageView)

CRightBottomToolbarView::CRightBottomToolbarView()
	: CPageView(CRightBottomToolbarView::IDD)
{
	//{{AFX_DATA_INIT(CRightBottomToolbarView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CRightBottomToolbarView::~CRightBottomToolbarView()
{
}

void CRightBottomToolbarView::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRightBottomToolbarView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRightBottomToolbarView, CPageView)
	//{{AFX_MSG_MAP(CRightBottomToolbarView)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRightBottomToolbarView diagnostics

#ifdef _DEBUG
void CRightBottomToolbarView::AssertValid() const
{
	CPageView::AssertValid();
}

void CRightBottomToolbarView::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRightBottomToolbarView message handlers

int CRightBottomToolbarView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPageView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//add the tool bar to the dialog
	m_ToolBar.Create(this);
	m_ToolBar.LoadToolBar(IDR_LIST_TOOLBAR);
	m_ToolBar.ShowWindow(SW_SHOW);
	m_ToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
	
	return 0;
}
