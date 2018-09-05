// TestView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "TestView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TestView

IMPLEMENT_DYNCREATE(TestView, CPageView)

TestView::TestView() : CPageView(IDD)
{
	//{{AFX_DATA_INIT(TestView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

TestView::~TestView()
{
}

void TestView::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TestView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TestView, CPageView)
	//{{AFX_MSG_MAP(TestView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TestView diagnostics

#ifdef _DEBUG
void TestView::AssertValid() const
{
	CPageView::AssertValid();
}

void TestView::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// TestView message handlers
