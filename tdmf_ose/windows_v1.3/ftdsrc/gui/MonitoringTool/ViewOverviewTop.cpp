// ViewOverviewTop.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "ViewOverviewTop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ViewOverviewTop

IMPLEMENT_DYNCREATE(ViewOverviewTop, CPageView)

ViewOverviewTop::ViewOverviewTop() : CPageView(IDD)
{
	//{{AFX_DATA_INIT(ViewOverviewTop)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

ViewOverviewTop::~ViewOverviewTop()
{
}

void ViewOverviewTop::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ViewOverviewTop)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ViewOverviewTop, CPageView)
	//{{AFX_MSG_MAP(ViewOverviewTop)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ViewOverviewTop diagnostics

#ifdef _DEBUG
void ViewOverviewTop::AssertValid() const
{
	CPageView::AssertValid();
}

void ViewOverviewTop::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// ViewOverviewTop message handlers
