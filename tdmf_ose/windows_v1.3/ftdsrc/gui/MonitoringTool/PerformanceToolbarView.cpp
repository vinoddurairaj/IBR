// PerformanceToolbarView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "PerformanceToolbarView.h"
#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPerformanceToolbarView

IMPLEMENT_DYNCREATE(CPerformanceToolbarView, CPageView)

CPerformanceToolbarView::CPerformanceToolbarView()
	: CPageView(CPerformanceToolbarView::IDD)
{
	//{{AFX_DATA_INIT(CPerformanceToolbarView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pPerformanceView = NULL;
}

CPerformanceToolbarView::~CPerformanceToolbarView()
{
}

void CPerformanceToolbarView::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPerformanceToolbarView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPerformanceToolbarView, CPageView)
	//{{AFX_MSG_MAP(CPerformanceToolbarView)
	ON_WM_CREATE()
	ON_COMMAND(ID_DRAW_BAR, OnDrawBar)
	ON_UPDATE_COMMAND_UI(ID_DRAW_BAR, OnUpdateDrawBar)
	ON_COMMAND(ID_DRAW_3D_BAR, OnDraw3dBar)
	ON_UPDATE_COMMAND_UI(ID_DRAW_3D_BAR, OnUpdateDraw3dBar)
	ON_COMMAND(ID_DRAW_3D_LINE, OnDraw3dLine)
	ON_UPDATE_COMMAND_UI(ID_DRAW_3D_LINE, OnUpdateDraw3dLine)
	ON_COMMAND(ID_DRAW_3D_PIE, OnDraw3dPie)
	ON_UPDATE_COMMAND_UI(ID_DRAW_3D_PIE, OnUpdateDraw3dPie)
	ON_COMMAND(ID_DRAW_3D_STACKED_BAR, OnDraw3dStackedBar)
	ON_UPDATE_COMMAND_UI(ID_DRAW_3D_STACKED_BAR, OnUpdateDraw3dStackedBar)
	ON_COMMAND(ID_DRAW_LINE, OnDrawLine)
	ON_UPDATE_COMMAND_UI(ID_DRAW_LINE, OnUpdateDrawLine)
	ON_COMMAND(ID_DRAW_PIE, OnDrawPie)
	ON_UPDATE_COMMAND_UI(ID_DRAW_PIE, OnUpdateDrawPie)
	ON_COMMAND(ID_DRAW_SCATTER, OnDrawScatter)
	ON_UPDATE_COMMAND_UI(ID_DRAW_SCATTER, OnUpdateDrawScatter)
	ON_COMMAND(ID_DRAW_STACKED_BAR, OnDrawStackedBar)
	ON_UPDATE_COMMAND_UI(ID_DRAW_STACKED_BAR, OnUpdateDrawStackedBar)
	ON_COMMAND(ID_DRAW_WHISKER, OnDrawWhisker)
	ON_UPDATE_COMMAND_UI(ID_DRAW_WHISKER, OnUpdateDrawWhisker)
	ON_COMMAND(ID_DRAW_XY_LINE, OnDrawXyLine)
	ON_UPDATE_COMMAND_UI(ID_DRAW_XY_LINE, OnUpdateDrawXyLine)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPerformanceToolbarView diagnostics

#ifdef _DEBUG
void CPerformanceToolbarView::AssertValid() const
{
	CPageView::AssertValid();
}

void CPerformanceToolbarView::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPerformanceToolbarView message handlers
int CPerformanceToolbarView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPageView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//attach the view to the main document
	Doc* p = GetDocument();
	p->AddView(this);


	//add the tool bar to the dialog
	m_ToolBar.Create(this);
	m_ToolBar.LoadToolBar(IDR_GRAPH);
	m_ToolBar.ShowWindow(SW_SHOW);
	m_ToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
	
	return 0;
}

void CPerformanceToolbarView::OnDrawBar() 
{
	CMessage Message(_T("Bar"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);
}

void CPerformanceToolbarView::OnUpdateDrawBar(CCmdUI* pCmdUI) 
{

}

void CPerformanceToolbarView::OnDraw3dBar() 
{
	CMessage Message(_T("Bar"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);
	
}

void CPerformanceToolbarView::OnUpdateDraw3dBar(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CPerformanceToolbarView::OnDraw3dLine() 
{
	CMessage Message(_T("Bar 3D"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);
}

void CPerformanceToolbarView::OnUpdateDraw3dLine(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CPerformanceToolbarView::OnDraw3dPie() 
{
	CMessage Message(_T("Pie 3D"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);
	
}

void CPerformanceToolbarView::OnUpdateDraw3dPie(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CPerformanceToolbarView::OnDraw3dStackedBar() 
{
	CMessage Message(_T("Stacked Bar 3D"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);
	
}

void CPerformanceToolbarView::OnUpdateDraw3dStackedBar(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CPerformanceToolbarView::OnDrawLine() 
{
	CMessage Message(_T("Line"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);
	
}

void CPerformanceToolbarView::OnUpdateDrawLine(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CPerformanceToolbarView::OnDrawPie() 
{
	CMessage Message(_T("Pie"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);
	
}

void CPerformanceToolbarView::OnUpdateDrawPie(CCmdUI* pCmdUI) 
{
}

void CPerformanceToolbarView::OnDrawScatter() 
{
	CMessage Message(_T("Scatter"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);
}

void CPerformanceToolbarView::OnUpdateDrawScatter(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CPerformanceToolbarView::OnDrawStackedBar() 
{
	CMessage Message(_T("Stacked Bar"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);

	
}

void CPerformanceToolbarView::OnUpdateDrawStackedBar(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CPerformanceToolbarView::OnDrawWhisker() 
{
	CMessage Message(_T("Box Whisker"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);

	
}

void CPerformanceToolbarView::OnUpdateDrawWhisker(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CPerformanceToolbarView::OnDrawXyLine() 
{
	CMessage Message(_T("XY Line"));
	GetDocument()->UpdateAllViews(NULL,UVH_CHANGE_GRAPH_TYPE,&Message);

}

void CPerformanceToolbarView::OnUpdateDrawXyLine(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}
