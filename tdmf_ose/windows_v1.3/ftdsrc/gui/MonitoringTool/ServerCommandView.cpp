// ServerCommandView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "ServerCommandView.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerCommandView

IMPLEMENT_DYNCREATE(CServerCommandView, CPageView)

CServerCommandView::CServerCommandView()
	: CPageView(CServerCommandView::IDD)
{
	//{{AFX_DATA_INIT(CServerCommandView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CServerCommandView::~CServerCommandView()
{
}

void CServerCommandView::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerCommandView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerCommandView, CPageView)
	//{{AFX_MSG_MAP(CServerCommandView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerCommandView diagnostics

#ifdef _DEBUG
void CServerCommandView::AssertValid() const
{
	CPageView::AssertValid();
}

void CServerCommandView::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CServerCommandView message handlers
void CServerCommandView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{

	switch (lHint)
	{
		case UVH_CHANGE_GRAPH_TYPE:
			{
			
			};
  		break;
	
	}
	
}