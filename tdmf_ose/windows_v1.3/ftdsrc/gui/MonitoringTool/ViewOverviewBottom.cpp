// ViewOverviewBottom.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "ViewOverviewBottom.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ViewOverviewBottom

IMPLEMENT_DYNCREATE(ViewOverviewBottom, CPageView)

ViewOverviewBottom::ViewOverviewBottom() : CPageView(IDD)
{
	//{{AFX_DATA_INIT(ViewOverviewBottom)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

ViewOverviewBottom::~ViewOverviewBottom()
{
}

void ViewOverviewBottom::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ViewOverviewBottom)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ViewOverviewBottom, CPageView)
	//{{AFX_MSG_MAP(ViewOverviewBottom)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ViewOverviewBottom diagnostics

#ifdef _DEBUG
void ViewOverviewBottom::AssertValid() const
{
	CPageView::AssertValid();
}

void ViewOverviewBottom::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// ViewOverviewBottom message handlers
