// ViewPairTop.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "ViewPairTop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ViewPairTop

IMPLEMENT_DYNCREATE(ViewPairTop, CPageView)

ViewPairTop::ViewPairTop() : CPageView(IDD)
{
	//{{AFX_DATA_INIT(ViewPairTop)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

ViewPairTop::~ViewPairTop()
{
}

void ViewPairTop::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ViewPairTop)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ViewPairTop, CPageView)
	//{{AFX_MSG_MAP(ViewPairTop)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ViewPairTop diagnostics

#ifdef _DEBUG
void ViewPairTop::AssertValid() const
{
	CPageView::AssertValid();
}

void ViewPairTop::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// ViewPairTop message handlers
