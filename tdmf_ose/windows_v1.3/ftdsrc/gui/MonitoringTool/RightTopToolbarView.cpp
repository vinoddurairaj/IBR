// RightTopToolbarView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "RightTopToolbarView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRightTopToolbarView

IMPLEMENT_DYNCREATE(CRightTopToolbarView, CPageView)

CRightTopToolbarView::CRightTopToolbarView()
	: CPageView(CRightTopToolbarView::IDD)
{
	//{{AFX_DATA_INIT(CRightTopToolbarView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CRightTopToolbarView::~CRightTopToolbarView()
{
}

void CRightTopToolbarView::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRightTopToolbarView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRightTopToolbarView, CPageView)
	//{{AFX_MSG_MAP(CRightTopToolbarView)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()




/////////////////////////////////////////////////////////////////////////////
// CRightTopToolbarView diagnostics

#ifdef _DEBUG
void CRightTopToolbarView::AssertValid() const
{
	CPageView::AssertValid();
}

void CRightTopToolbarView::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRightTopToolbarView message handlers

int CRightTopToolbarView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
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



void CRightTopToolbarView::OnInitialUpdate() 
{
	CPageView::OnInitialUpdate();
	
}
