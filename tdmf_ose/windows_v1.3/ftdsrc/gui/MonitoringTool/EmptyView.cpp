// EmptyView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "EmptyView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEmptyView

IMPLEMENT_DYNCREATE(CEmptyView, CPageView)

CEmptyView::CEmptyView()
	: CPageView(CEmptyView::IDD)
{
	//{{AFX_DATA_INIT(CEmptyView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CEmptyView::~CEmptyView()
{
}

void CEmptyView::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEmptyView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEmptyView, CPageView)
	//{{AFX_MSG_MAP(CEmptyView)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEmptyView diagnostics

#ifdef _DEBUG
void CEmptyView::AssertValid() const
{
	CPageView::AssertValid();
}

void CEmptyView::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEmptyView message handlers

int CEmptyView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPageView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//add the tool bar to the dialog
	m_ToolBar.Create(this);
	m_ToolBar.LoadToolBar(IDR_MAINFRAME);
	m_ToolBar.ShowWindow(SW_SHOW);
	m_ToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
	
	return 0;
}
