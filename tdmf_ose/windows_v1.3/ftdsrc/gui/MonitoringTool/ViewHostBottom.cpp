// ViewHostBottom.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "ViewHostBottom.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ViewHostBottom

IMPLEMENT_DYNCREATE(ViewHostBottom, CPageView)

ViewHostBottom::ViewHostBottom() : CPageView(IDD)
{
	//{{AFX_DATA_INIT(ViewHostBottom)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

ViewHostBottom::~ViewHostBottom()
{
}

void ViewHostBottom::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ViewHostBottom)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ViewHostBottom, CPageView)
	//{{AFX_MSG_MAP(ViewHostBottom)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ViewHostBottom diagnostics

#ifdef _DEBUG
void ViewHostBottom::AssertValid() const
{
	CPageView::AssertValid();
}

void ViewHostBottom::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// ViewHostBottom message handlers
