// ViewPairBottom.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "ViewPairBottom.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ViewPairBottom

IMPLEMENT_DYNCREATE(ViewPairBottom, CPageView)

ViewPairBottom::ViewPairBottom() : CPageView(IDD)
{
	//{{AFX_DATA_INIT(ViewPairBottom)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

ViewPairBottom::~ViewPairBottom()
{
}

void ViewPairBottom::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ViewPairBottom)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ViewPairBottom, CPageView)
	//{{AFX_MSG_MAP(ViewPairBottom)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ViewPairBottom diagnostics

#ifdef _DEBUG
void ViewPairBottom::AssertValid() const
{
	CPageView::AssertValid();
}

void ViewPairBottom::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// ViewPairBottom message handlers
