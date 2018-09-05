// ServerPerformanceView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "ServerPerformanceView.h"
#include "resource.h"
#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerPerformanceView

IMPLEMENT_DYNCREATE(CServerPerformanceView, CPageView)

CServerPerformanceView::CServerPerformanceView()
	: CPageView(CServerPerformanceView::IDD)
{
	//{{AFX_DATA_INIT(CServerPerformanceView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	graphComplete = false;

}

CServerPerformanceView::~CServerPerformanceView()
{
}

void CServerPerformanceView::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerPerformanceView)
	DDX_Control(pDX, IDC_CBO_GRAPHTYPE, m_CBO_GraphTypeCtrl);
	DDX_Control(pDX, IDC_GRAPH_PERFORMANCE, m_GraphCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerPerformanceView, CPageView)
	//{{AFX_MSG_MAP(CServerPerformanceView)
	ON_WM_CREATE()
	ON_COMMAND(ID_DRAW_BAR, OnDrawBar)
	ON_COMMAND(ID_DRAW_LINE, OnDrawLine)
	ON_COMMAND(ID_DRAW_PIE, OnDrawPie)
	ON_COMMAND(ID_DRAW_SCATTER, OnDrawScatter)
	ON_COMMAND(ID_DRAW_WHISKER, OnDrawWhisker)
	ON_COMMAND(ID_DRAW_3D_BAR, OnDraw3dBar)
	ON_COMMAND(ID_DRAW_3D_LINE, OnDraw3dLine)
	ON_COMMAND(ID_DRAW_STACKED_BAR, OnDrawStackedBar)
	ON_COMMAND(ID_DRAW_XY_LINE, OnDrawXyLine)
	ON_COMMAND(ID_DRAW_3D_PIE, OnDraw3dPie)
	ON_COMMAND(ID_DRAW_3D_STACKED_BAR, OnDraw3dStackedBar)
	ON_CBN_SELCHANGE(IDC_CBO_GRAPHTYPE, OnSelchangeCboGraphtype)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CServerPerformanceView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo) 
{
	// TODO: Add your specialized code here and/or call the base class
	
//	CView::OnBeginPrinting(pDC, pInfo);
}

void CServerPerformanceView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo) 
{
	// TODO: Add your specialized code here and/or call the base class
	
//	CView::OnEndPrinting(pDC, pInfo);
}

BOOL CServerPerformanceView::OnPreparePrinting(CPrintInfo* pInfo) 
{
	// TODO: call DoPreparePrinting to invoke the Print dialog box
	pInfo->SetMaxPage(1);
	return DoPreparePrinting(pInfo);
//	return CView::OnPreparePrinting(pInfo);
}

void CServerPerformanceView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
	// TODO: Add your specialized code here and/or call the base class
	int	nPos;
	nPos = -720;  // 1/2 inch top margin
	if(graphComplete)
	{
		m_GraphCtrl.SetMargins(-720, -15120, 720, 10800, nPos); //portrait page
		nPos = m_GraphCtrl.PrintGraph(pDC, pInfo);
	}
//	CView::OnPrint(pDC, pInfo);
}

void CServerPerformanceView::OnDrawBar() 
{
	m_GraphCtrl.SetGraphType(BAR_GRAPH);
	m_GraphCtrl.SetGraphTitle("Bar Chart");
//	m_GraphCtrl.SetGraphAlignment(HORIZONTAL_ALIGN);

	m_GraphCtrl.SetXAxisAlignment(0);
//	m_GraphCtrl.SetXAxisAlignment(90);
//	m_GraphCtrl.SetXAxisAlignment(270);
//	m_GraphCtrl.SetXAxisAlignment(45);
//	m_GraphCtrl.SetXAxisAlignment(315);
	m_GraphCtrl.SetXAxisLabel("Games");
	m_GraphCtrl.SetYAxisLabel("Scores");
//	m_GraphCtrl.SetGraphQuadType(2);
	m_GraphCtrl.SetTickLimits(0, 300, 50);
//	m_GraphCtrl.SetGraphQuadType(3);
//	m_GraphCtrl.SetGraphQuadType(4);
//	m_GraphCtrl.SetTickLimits(-300, 300, 100);

	//set up some series
	m_GraphCtrl.RemoveAllSeries();
	CGraphSeries* series1 = new CGraphSeries();
	CGraphSeries* series2 = new CGraphSeries();
	CGraphSeries* series3 = new CGraphSeries();
	series1->SetLabel("day 1");
	series2->SetLabel("day 2");
	series3->SetLabel("day 3");
	series1->SetData(0, 150);
	series1->SetData(1, 202);
	series1->SetData(2, 230);
	series2->SetData(0, 199);
	series2->SetData(1, 140);
	series2->SetData(2, 279);
	series3->SetData(0, 204);
	series3->SetData(1, 221);
	series3->SetData(2, 208);
	m_GraphCtrl.AddSeries(series1);
	m_GraphCtrl.AddSeries(series2);
	m_GraphCtrl.AddSeries(series3);

	//set the colors of my bars
	m_GraphCtrl.SetColor(0, FOREST_GREEN);
	m_GraphCtrl.SetColor(1, SKY_BLUE);
	m_GraphCtrl.SetColor(2, DUSK);

	//set up legend

	m_GraphCtrl.SetLegend(0, "game 1");
	m_GraphCtrl.SetLegend(1, "game 2");
	m_GraphCtrl.SetLegend(2, "game 3");
		
	graphComplete = TRUE;
	Invalidate(TRUE);
	
}

void CServerPerformanceView::OnDrawLine() 
{
	m_GraphCtrl.SetGraphType(LINE_GRAPH);
	m_GraphCtrl.SetGraphTitle("Line Graph");
//	m_GraphCtrl.SetGraphAlignment(HORIZONTAL_ALIGN);

	//	testGraph.SetGraphType(1);
	m_GraphCtrl.SetXAxisAlignment(0);
//	m_GraphCtrl.SetXAxisAlignment(90);
//	m_GraphCtrl.SetXAxisAlignment(270);
//	m_GraphCtrl.SetXAxisAlignment(45);
//	m_GraphCtrl.SetXAxisAlignment(315);
	m_GraphCtrl.SetXAxisLabel("Games");
	m_GraphCtrl.SetYAxisLabel("Scores");
//	m_GraphCtrl.SetTickLimits(0, 300, 50);
	m_GraphCtrl.SetTickLimits(63, 74, 1);

	//set up some series
	m_GraphCtrl.RemoveAllSeries();
	CGraphSeries* series1 = new CGraphSeries();
	CGraphSeries* series2 = new CGraphSeries();
	CGraphSeries* series3 = new CGraphSeries();
	series1->SetLabel("day 1");
	series2->SetLabel("day 2");
	series3->SetLabel("day 3");
//	series1->SetData(0, 150);
//	series1->SetData(1, 202);
//	series1->SetData(2, 230);
//	series2->SetData(0, 199);
//	series2->SetData(1, 140);
//	series2->SetData(2, 279);
//	series3->SetData(0, 204);
//	series3->SetData(1, 221);
//	series3->SetData(2, 208);
	series1->SetData(0, 64);
	series1->SetData(1, 72);
	series1->SetData(2, 70);
	series2->SetData(0, 63);
	series2->SetData(1, 68);
	series2->SetData(2, 71);
	series3->SetData(0, 74);
	series3->SetData(1, 69);
	series3->SetData(2, 66);

	m_GraphCtrl.AddSeries(series1);
	m_GraphCtrl.AddSeries(series2);
	m_GraphCtrl.AddSeries(series3);

	m_GraphCtrl.SetColor(0, FOREST_GREEN);
	m_GraphCtrl.SetColor(1, SKY_BLUE);
	m_GraphCtrl.SetColor(2, DUSK);

	//set up legend
	
	m_GraphCtrl.SetLegend(0, "game 1");
	m_GraphCtrl.SetLegend(1, "game 2");
	m_GraphCtrl.SetLegend(2, "game 3");
		
	graphComplete = TRUE;
	Invalidate(TRUE);
	
}

void CServerPerformanceView::OnDrawPie() 
{
	m_GraphCtrl.SetGraphType(PIE_GRAPH);
	m_GraphCtrl.SetGraphTitle("Pie Chart");
	
	//set up legend
	
	m_GraphCtrl.SetLegend(0, "game 1");
	m_GraphCtrl.SetLegend(1, "game 2");
	m_GraphCtrl.SetLegend(2, "game 3");
	m_GraphCtrl.SetLegend(3, "game 4");
	m_GraphCtrl.SetLegend(4, "game 5");
	m_GraphCtrl.SetLegend(5, "game 6");
	m_GraphCtrl.SetLegend(6, "game 7");
	m_GraphCtrl.SetLegend(7, "game 8");
	m_GraphCtrl.SetLegend(8, "game 9");
	m_GraphCtrl.SetLegend(9, "game 10");

	//set up some series
	m_GraphCtrl.RemoveAllSeries();
	CGraphSeries* series1 = new CGraphSeries();
//	CGraphSeries* series2 = new CGraphSeries();
//	CGraphSeries* series3 = new CGraphSeries();
	series1->SetLabel("day 1");
//	series2->SetLabel("day 2");
//	series3->SetLabel("day 3");
	series1->SetData(0, 15);
	series1->SetData(1, 5);
	series1->SetData(2, 2);
	series1->SetData(3, 8);
	series1->SetData(4, 30);
	series1->SetData(5, 6);
	series1->SetData(6, 7);
	series1->SetData(7, 9);
	series1->SetData(8, 8);
	series1->SetData(9, 10);
//	series2->SetData(0, 15);
//	series2->SetData(1, 15);
//	series2->SetData(2, 15);
//	series3->SetData(0, 10);
//	series3->SetData(1, 20);
//	series3->SetData(2, 30);
//	series3->SetData(3, 40);
//	series3->SetData(4, 50);

	m_GraphCtrl.AddSeries(series1);
//	m_GraphCtrl.AddSeries(series2);
//	m_GraphCtrl.AddSeries(series3);

	//set the colors of my bars
	m_GraphCtrl.SetColor(0, FOREST_GREEN);
	m_GraphCtrl.SetColor(1, SKY_BLUE);
	m_GraphCtrl.SetColor(2, DUSK);
	m_GraphCtrl.SetColor(3, HOT_PINK);
	m_GraphCtrl.SetColor(4, LAVENDER);
	m_GraphCtrl.SetColor(5, ROYAL_BLUE);
	m_GraphCtrl.SetColor(6, BROWN);
	m_GraphCtrl.SetColor(7, MAROON);
	m_GraphCtrl.SetColor(8, GREY);
	m_GraphCtrl.SetColor(9, TAN);

	graphComplete = TRUE;
	Invalidate(TRUE);
}

void CServerPerformanceView::OnDrawScatter() 
{
	m_GraphCtrl.SetGraphType(SCATTER_GRAPH);
	m_GraphCtrl.SetGraphTitle("Scatter Graph");
//	m_GraphCtrl.SetGraphAlignment(HORIZONTAL_ALIGN);
	
//	m_GraphCtrl.SetGraphQuadType(2);
	m_GraphCtrl.SetTickLimits(0,100,10);
//	m_GraphCtrl.SetGraphQuadType(3);
//	m_GraphCtrl.SetGraphQuadType(4);
//	m_GraphCtrl.SetTickLimits(-100, 100, 10);

	m_GraphCtrl.SetXAxisLabel("Test 1");
	m_GraphCtrl.SetYAxisLabel("Test 2");

	//set up some series
	CGraphSeries* student1 = new CGraphSeries();
	CGraphSeries* student2 = new CGraphSeries();
	CGraphSeries* student3 = new CGraphSeries();
	CGraphSeries* student4 = new CGraphSeries();
	CGraphSeries* student5 = new CGraphSeries();
	CGraphSeries* student6 = new CGraphSeries();
	CGraphSeries* student7 = new CGraphSeries();
	CGraphSeries* student8 = new CGraphSeries();
	CGraphSeries* student9 = new CGraphSeries();
	CGraphSeries* student10 = new CGraphSeries();
	CGraphSeries* student11 = new CGraphSeries();
	CGraphSeries* student12 = new CGraphSeries();

	student1->SetData(67,98);
	student2->SetData(94,95);
	student3->SetData(58,45);
	student4->SetData(27,35);
	student5->SetData(84,79);
	student6->SetData(72,91);
	student7->SetData(75,78);
	student8->SetData(91,88);
	student9->SetData(100,94);
	student10->SetData(48,63);
	student11->SetData(63,92);
	student12->SetData(81,89);

	m_GraphCtrl.AddSeries(student1);
	m_GraphCtrl.AddSeries(student2);
	m_GraphCtrl.AddSeries(student3);
	m_GraphCtrl.AddSeries(student4);
	m_GraphCtrl.AddSeries(student5);
	m_GraphCtrl.AddSeries(student6);
	m_GraphCtrl.AddSeries(student7);
	m_GraphCtrl.AddSeries(student8);
	m_GraphCtrl.AddSeries(student9);
	m_GraphCtrl.AddSeries(student10);
	m_GraphCtrl.AddSeries(student11);
	m_GraphCtrl.AddSeries(student12);

	graphComplete = TRUE;
	Invalidate(TRUE);	
}

void CServerPerformanceView::OnDrawWhisker() 
{
	m_GraphCtrl.SetGraphType(BOX_WHISKER_GRAPH);
	m_GraphCtrl.SetGraphTitle("Box and Whisker Graph");
//	m_GraphCtrl.SetGraphAlignment(HORIZONTAL_ALIGN);
	m_GraphCtrl.SetXAxisLabel("Test Scores");
	m_GraphCtrl.SetYAxisLabel("Test Score");

	//set up some series
	CGraphSeries* test1 = new CGraphSeries();
	CGraphSeries* test2 = new CGraphSeries();
	CGraphSeries* test3 = new CGraphSeries();

	test1->SetData(0,14);
	test1->SetData(1,13);
	test1->SetData(2,3);
	test1->SetData(3,7);
	test1->SetData(4,9);
	test1->SetData(5,12);
	test1->SetData(6,17);
	test1->SetData(7,4);
	test1->SetData(8,9);
	test1->SetData(9,10);
	test1->SetData(10,18);
	test1->SetData(11,16);

	test2->SetData(0,14);
	test2->SetData(1,13);
	test2->SetData(2,3);
	test2->SetData(3,7);
	test2->SetData(4,9);
	test2->SetData(5,12);
	test2->SetData(6,17);

	test3->SetData(0,14);
	test3->SetData(1,13);
	test3->SetData(2,3);
	test3->SetData(3,7);
	test3->SetData(4,9);
	
	test1->SetLabel("Test 1");
	test2->SetLabel("Test 2");
	test3->SetLabel("Test 3");

	m_GraphCtrl.AddSeries(test1);
	m_GraphCtrl.AddSeries(test2);
	m_GraphCtrl.AddSeries(test3);

	m_GraphCtrl.SetTickLimits(0,20,1);


	graphComplete = TRUE;
	Invalidate(TRUE);	
}

void CServerPerformanceView::OnDraw3dBar() 
{
	m_GraphCtrl.SetGraphType(BAR_GRAPH_3D);

	
	m_GraphCtrl.SetGraphTitle("3D Bar Chart");
//	m_GraphCtrl.SetGraphAlignment(HORIZONTAL_ALIGN);

	m_GraphCtrl.SetXAxisAlignment(0);
//	m_GraphCtrl.SetXAxisAlignment(90);
//	m_GraphCtrl.SetXAxisAlignment(270);
//	m_GraphCtrl.SetXAxisAlignment(45);
//	m_GraphCtrl.SetXAxisAlignment(315);
	m_GraphCtrl.SetXAxisLabel("Games");
	m_GraphCtrl.SetYAxisLabel("Scores");
	m_GraphCtrl.Set3DDepthRatio(.1);

	//set up some series
	m_GraphCtrl.RemoveAllSeries();
	CGraphSeries* series1 = new CGraphSeries();
	series1->SetLabel("day 1");
	series1->SetData(0, 150);
	series1->SetData(1, 200);
	series1->SetData(2, 250);
	series1->SetData(3, 185);
	series1->SetData(4, 198);
	series1->SetData(5, 234);
	series1->SetData(6, 170);
	series1->SetData(7, 190);
	series1->SetData(8, 188);
	series1->SetData(9, 209);
	series1->SetData(10, 158);
	series1->SetData(11, 97);
	m_GraphCtrl.AddSeries(series1);

	m_GraphCtrl.SetTickLimits(0, 300, 50);

	//set the colors of my bars
	m_GraphCtrl.SetColor(0, FOREST_GREEN);
	m_GraphCtrl.SetColor(1, SKY_BLUE);
	m_GraphCtrl.SetColor(2, DUSK);
	m_GraphCtrl.SetColor(3, HOT_PINK);
	m_GraphCtrl.SetColor(4, LAVENDER);
	m_GraphCtrl.SetColor(5, ROYAL_BLUE);
	m_GraphCtrl.SetColor(6, BROWN);
	m_GraphCtrl.SetColor(7, MAROON);
	m_GraphCtrl.SetColor(8, GREY);
	m_GraphCtrl.SetColor(9, TAN);
	m_GraphCtrl.SetColor(10, YELLOW);
	m_GraphCtrl.SetColor(11, ORANGE);

	//set up legend
	
	m_GraphCtrl.SetLegend(0, "game 1");
	m_GraphCtrl.SetLegend(1, "game 2");
	m_GraphCtrl.SetLegend(2, "game 3");
	m_GraphCtrl.SetLegend(3, "game 4");
	m_GraphCtrl.SetLegend(4, "game 5");
	m_GraphCtrl.SetLegend(5, "game 6");
	m_GraphCtrl.SetLegend(6, "game 7");
	m_GraphCtrl.SetLegend(7, "game 8");
	m_GraphCtrl.SetLegend(8, "game 9");
	m_GraphCtrl.SetLegend(9, "game 10");
	m_GraphCtrl.SetLegend(10, "game 11");
	m_GraphCtrl.SetLegend(11, "game 12");
		
	graphComplete = TRUE;
	Invalidate(TRUE);
	
}


void CServerPerformanceView::OnDraw3dLine() 
{
	m_GraphCtrl.SetGraphType(LINE_GRAPH_3D);
	m_GraphCtrl.SetGraphTitle("3D Line Graph");
//	m_GraphCtrl.SetGraphAlignment(HORIZONTAL_ALIGN);

	//	testGraph.SetGraphType(1);
	m_GraphCtrl.SetXAxisAlignment(0);
//	m_GraphCtrl.SetXAxisAlignment(90);
//	m_GraphCtrl.SetXAxisAlignment(270);
//	m_GraphCtrl.SetXAxisAlignment(45);
//	m_GraphCtrl.SetXAxisAlignment(315);
	m_GraphCtrl.SetXAxisLabel("Games");
	m_GraphCtrl.SetYAxisLabel("Scores");
	m_GraphCtrl.Set3DDepthRatio(.1);
	m_GraphCtrl.Set3DLineBase(0,0);

	//set up some series
	m_GraphCtrl.RemoveAllSeries();
	CGraphSeries* series1 = new CGraphSeries();
	CGraphSeries* series2 = new CGraphSeries();
	CGraphSeries* series3 = new CGraphSeries();
	CGraphSeries* series4 = new CGraphSeries();
	CGraphSeries* series5 = new CGraphSeries();
	series1->SetLabel("day 1");
	series2->SetLabel("day 2");
	series3->SetLabel("day 3");
	series4->SetLabel("day 4");
	series5->SetLabel("day 5");
	series1->SetData(0, 240);	
	series1->SetData(1, 50);
	series1->SetData(2, 60);
	series2->SetData(0, 300);
	series2->SetData(1, 140);
	series2->SetData(2, 279);
	series3->SetData(0, 300);
	series3->SetData(1, 221);
	series3->SetData(2, 208);
	series4->SetData(0, 150);
	series4->SetData(1, 250);
	series4->SetData(2, 135);
	series5->SetData(0, 230);
	series5->SetData(1, 130);
	series5->SetData(2, 80);
	m_GraphCtrl.AddSeries(series1);
	m_GraphCtrl.AddSeries(series2);
	m_GraphCtrl.AddSeries(series3);
	m_GraphCtrl.AddSeries(series4);
	m_GraphCtrl.AddSeries(series5);

	m_GraphCtrl.SetTickLimits(0, 300, 50);

	m_GraphCtrl.SetColor(0, RED);
	m_GraphCtrl.SetColor(1, FOREST_GREEN);
	m_GraphCtrl.SetColor(2, SKY_BLUE);

	//set up legend
	
	m_GraphCtrl.SetLegend(0, "game 1");
	m_GraphCtrl.SetLegend(1, "game 2");
	m_GraphCtrl.SetLegend(2, "game 3");
		
	graphComplete = TRUE;
	Invalidate(TRUE);
}

void CServerPerformanceView::OnDrawStackedBar() 
{
	m_GraphCtrl.SetGraphType(STACKED_BAR_GRAPH);
	m_GraphCtrl.SetGraphTitle("Stacked Bar Chart");
//	m_GraphCtrl.SetGraphAlignment(HORIZONTAL_ALIGN);

	m_GraphCtrl.SetXAxisAlignment(0);
//	m_GraphCtrl.SetXAxisAlignment(90);
//	m_GraphCtrl.SetXAxisAlignment(270);
//	m_GraphCtrl.SetXAxisAlignment(45);
//	m_GraphCtrl.SetXAxisAlignment(315);
	m_GraphCtrl.SetXAxisLabel("Games");
	m_GraphCtrl.SetYAxisLabel("Series");

	//set up some series
	m_GraphCtrl.RemoveAllSeries();
	CGraphSeries* series1 = new CGraphSeries();
	CGraphSeries* series2 = new CGraphSeries();
	CGraphSeries* series3 = new CGraphSeries();
	series1->SetLabel("day 1");
	series2->SetLabel("day 2");
	series3->SetLabel("day 3");
	series1->SetData(0, 150);
	series1->SetData(1, 202);
	series1->SetData(2, 230);
	series2->SetData(0, 199);
	series2->SetData(1, 140);
	series2->SetData(2, 279);
	series3->SetData(0, 204);
	series3->SetData(1, 221);
	series3->SetData(2, 208);
	m_GraphCtrl.AddSeries(series1);
	m_GraphCtrl.AddSeries(series2);
	m_GraphCtrl.AddSeries(series3);

	m_GraphCtrl.SetTickLimits(0, 900, 50);

	//set the colors of my bars
	m_GraphCtrl.SetColor(0, FOREST_GREEN);
	m_GraphCtrl.SetColor(1, SKY_BLUE);
	m_GraphCtrl.SetColor(2, DUSK);

	//set up legend
	
	m_GraphCtrl.SetLegend(0, "game 1");
	m_GraphCtrl.SetLegend(1, "game 2");
	m_GraphCtrl.SetLegend(2, "game 3");
		
	graphComplete = TRUE;
	Invalidate(TRUE);
	
	
}

void CServerPerformanceView::OnDrawXyLine() 
{
	m_GraphCtrl.SetGraphType(XY_LINE_GRAPH);

	m_GraphCtrl.SetGraphTitle("X-Y Line Graph");
//	m_GraphCtrl.SetGraphAlignment(HORIZONTAL_ALIGN);

	//	testGraph.SetGraphType(1);
	m_GraphCtrl.SetXAxisAlignment(0);
//	m_GraphCtrl.SetXAxisAlignment(90);
//	m_GraphCtrl.SetXAxisAlignment(270);
//	m_GraphCtrl.SetXAxisAlignment(45);
//	m_GraphCtrl.SetXAxisAlignment(315);
	m_GraphCtrl.SetXAxisLabel("Games");
	m_GraphCtrl.SetYAxisLabel("Scores");

	//set up some series
	m_GraphCtrl.RemoveAllSeries();
	CGraphSeries* series1 = new CGraphSeries();
	CGraphSeries* series2 = new CGraphSeries();
	CGraphSeries* series3 = new CGraphSeries();
	series1->SetLabel("day 1");
	series2->SetLabel("day 2");
	series3->SetLabel("day 3");
	series1->SetData(0, 150);
	series1->SetData(1, 202);
	series1->SetData(2, 230);
	series2->SetData(0, 199);
	series2->SetData(1, 140);
	series2->SetData(2, 279);
	series3->SetData(0, 204);
	series3->SetData(1, 221);
	series3->SetData(2, 208);
	m_GraphCtrl.AddSeries(series1);
	m_GraphCtrl.AddSeries(series2);
	m_GraphCtrl.AddSeries(series3);

	m_GraphCtrl.SetTickLimits(0, 300, 50);

	m_GraphCtrl.SetColor(0, FOREST_GREEN);
	m_GraphCtrl.SetColor(1, SKY_BLUE);
	m_GraphCtrl.SetColor(2, DUSK);

	//set up legend
	
	m_GraphCtrl.SetLegend(0, "game 1");
	m_GraphCtrl.SetLegend(1, "game 2");
	m_GraphCtrl.SetLegend(2, "game 3");
		
	graphComplete = TRUE;
	Invalidate(TRUE);
	
}

void CServerPerformanceView::OnDraw3dPie() 
{
	m_GraphCtrl.SetGraphType(PIE_GRAPH_3D);

	m_GraphCtrl.SetGraphTitle("3D Pie Chart");
	m_GraphCtrl.Set3DDepthRatio(.1);
	
	//set up legend
	
	m_GraphCtrl.SetLegend(0, "game 1");
	m_GraphCtrl.SetLegend(1, "game 2");
	m_GraphCtrl.SetLegend(2, "game 3");
	m_GraphCtrl.SetLegend(3, "game 4");
	m_GraphCtrl.SetLegend(4, "game 5");
	m_GraphCtrl.SetLegend(5, "game 6");
	m_GraphCtrl.SetLegend(6, "game 7");
	m_GraphCtrl.SetLegend(7, "game 8");
	m_GraphCtrl.SetLegend(8, "game 9");
	m_GraphCtrl.SetLegend(9, "game 10");

	//set up some series
	m_GraphCtrl.RemoveAllSeries();
	CGraphSeries* series1 = new CGraphSeries();
	series1->SetLabel("day 1");
	series1->SetData(0, 15);
	series1->SetData(1, 5);
	series1->SetData(2, 2);
	series1->SetData(3, 8);
	series1->SetData(4, 30);
	series1->SetData(5, 6);
	series1->SetData(6, 7);
	series1->SetData(7, 9);
	series1->SetData(8, 8);
	series1->SetData(9, 10);

	m_GraphCtrl.AddSeries(series1);

	//set the colors of my bars
	m_GraphCtrl.SetColor(0, FOREST_GREEN);
	m_GraphCtrl.SetColor(1, SKY_BLUE);
	m_GraphCtrl.SetColor(2, DUSK);
	m_GraphCtrl.SetColor(3, HOT_PINK);
	m_GraphCtrl.SetColor(4, LAVENDER);
	m_GraphCtrl.SetColor(5, ROYAL_BLUE);
	m_GraphCtrl.SetColor(6, BROWN);
	m_GraphCtrl.SetColor(7, MAROON);
	m_GraphCtrl.SetColor(8, GREY);
	m_GraphCtrl.SetColor(9, TAN);

	graphComplete = TRUE;
	Invalidate(TRUE);}

void CServerPerformanceView::OnDraw3dStackedBar() 
{
	m_GraphCtrl.SetGraphType(STACKED_BAR_GRAPH_3D);
	m_GraphCtrl.SetGraphTitle("3D Stacked Bar Chart");
//	m_GraphCtrl.SetGraphAlignment(HORIZONTAL_ALIGN);

	m_GraphCtrl.SetXAxisAlignment(0);
//	m_GraphCtrl.SetXAxisAlignment(90);
//	m_GraphCtrl.SetXAxisAlignment(270);
//	m_GraphCtrl.SetXAxisAlignment(45);
//	m_GraphCtrl.SetXAxisAlignment(315);
	m_GraphCtrl.SetXAxisLabel("Games");
	m_GraphCtrl.SetYAxisLabel("Series");
	m_GraphCtrl.Set3DDepthRatio(.1);

	//set up some series
	m_GraphCtrl.RemoveAllSeries();
	CGraphSeries* series1 = new CGraphSeries();
	CGraphSeries* series2 = new CGraphSeries();
	CGraphSeries* series3 = new CGraphSeries();
	series1->SetLabel("day 1");
	series2->SetLabel("day 2");
	series3->SetLabel("day 3");
	series1->SetData(0, 150);
	series1->SetData(1, 202);
	series1->SetData(2, 230);
	series2->SetData(0, 199);
	series2->SetData(1, 140);
	series2->SetData(2, 279);
	series3->SetData(0, 204);
	series3->SetData(1, 221);
	series3->SetData(2, 208);
	m_GraphCtrl.AddSeries(series1);
	m_GraphCtrl.AddSeries(series2);
	m_GraphCtrl.AddSeries(series3);

	m_GraphCtrl.SetTickLimits(0, 900, 100);

	//set the colors of my bars
	m_GraphCtrl.SetColor(0, FOREST_GREEN);
	m_GraphCtrl.SetColor(1, SKY_BLUE);
	m_GraphCtrl.SetColor(2, DUSK);

	//set up legend
	
	m_GraphCtrl.SetLegend(0, "game 1");
	m_GraphCtrl.SetLegend(1, "game 2");
	m_GraphCtrl.SetLegend(2, "game 3");
		
	graphComplete = TRUE;
	Invalidate(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CServerPerformanceView diagnostics

#ifdef _DEBUG
void CServerPerformanceView::AssertValid() const
{
	CPageView::AssertValid();
}

void CServerPerformanceView::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CServerPerformanceView message handlers

void CServerPerformanceView::OnDraw(CDC* pDC) 
{
  UpdateWindow();

	if(graphComplete)
	{
		CDC* pControlDC = m_GraphCtrl.GetDC();
		m_GraphCtrl.DrawGraph(pControlDC);
		ReleaseDC(pControlDC);
	}
   m_GraphCtrl.UpdateWindow();
 
}

int CServerPerformanceView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPageView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   Doc* p = GetDocument();
	
	p->AddView(this);

	return 0;
}

void CServerPerformanceView::OnInitialUpdate() 
{
	CPageView::OnInitialUpdate();
	
	graphComplete = FALSE;	
	
	m_CBO_GraphTypeCtrl.SetCurSel(m_CBO_GraphTypeCtrl.FindStringExact(0,_T("Bar")));

	OnSelchangeCboGraphtype() ;
	
}


void CServerPerformanceView::OnSelchangeCboGraphtype() 
{
	CString strValue;
   m_CBO_GraphTypeCtrl.GetWindowText(strValue);


	if(strValue.CompareNoCase(_T("Bar")) == 0)
	{
		OnDrawBar();
	}
	else if( strValue.CompareNoCase(_T("Line")) == 0)
	{
		OnDrawLine();
	}
	else if( strValue.CompareNoCase(_T("Pie")) == 0)
	{
		OnDrawPie();
	}
	else if( strValue.CompareNoCase(_T("Scatter")) == 0)
	{
		OnDrawScatter();
	}
	else if( strValue.CompareNoCase(_T("Box Whisker")) == 0)
	{
	  OnDrawWhisker();
	}
	else if( strValue.CompareNoCase(_T("Stacked Bar")) == 0)
	{
	  OnDrawStackedBar();
	}
	else if( strValue.CompareNoCase(_T("XY Line")) == 0)
	{
	  OnDrawXyLine();
	}
	else if( strValue.CompareNoCase(_T("Bar 3D")) == 0)
	{
	  OnDraw3dBar();
	}
	else if( strValue.CompareNoCase(_T("Line 3D")) == 0)
	{
	  OnDraw3dLine();
	}
	else if( strValue.CompareNoCase(_T("Pie 3D")) == 0)
	{
		OnDraw3dPie();
	}
	else if( strValue.CompareNoCase(_T("Stacked Bar 3D")) == 0)
	{
	  OnDraw3dStackedBar();
	}

}

void CServerPerformanceView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{

	switch (lHint)
	{
		case UVH_CHANGE_GRAPH_TYPE:
			{
				CString strValue = ((CMessage*)pHint)->m_strMessage;
				m_CBO_GraphTypeCtrl.SetCurSel(m_CBO_GraphTypeCtrl.FindStringExact(0,strValue));
				OnSelchangeCboGraphtype() ;
			};
  		break;
		case UVH_CHANGE_MODE:
			{
				CString strValue = ((CMessage*)pHint)->m_strMessage;
				AfxMessageBox(strValue);
			};
  		break;

		
	
	}
	
}

