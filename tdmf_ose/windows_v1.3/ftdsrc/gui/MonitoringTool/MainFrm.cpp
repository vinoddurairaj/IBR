// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "TDMFGUI.h"

#include "MainFrm.h"
#include "RightBottomView.h"
#include "RightTopView.h"
#include "MainFilterView.h"
#include "RightBottomViewConfiguration.h"
#include "TDMFServersView.h"
#include "TDMFLogicalGroupsView.h"
#include "LeftTreeView.h"
#include "ReplicationPairView.h"
#include "MainView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFont*	gpAppFont = NULL;
CFont*	gpTabFont = NULL;

COLORREF clrWhite = RGB(255, 255, 255);
COLORREF clrBlack = RGB(0, 0, 0);
COLORREF clrLGray = RGB(192, 192, 192);
COLORREF clrDGray = RGB(128, 128, 128);
COLORREF clrDBlue = RGB(0, 0, 128);
COLORREF clrTurquoise = RGB(0, 128, 128);
COLORREF clrRed	= RGB(255, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
   //{{AFX_MSG_MAP(CMainFrame)
   ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
   ID_SEPARATOR,           // status line indicator
   // ID_INDICATOR_CAPS,
   // ID_INDICATOR_NUM,
   // ID_INDICATOR_SCRL,
   IDS_DOMAIN,
   IDS_SERVER,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame() : m_bInitialized(false)
{
   // TODO: add member initialization code here
   
}

CMainFrame::~CMainFrame()
{
	if ( gpAppFont != NULL )
	delete gpAppFont;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   if(CFrameWnd::OnCreate(lpCreateStruct) == -1)
      return -1;
   
   if(!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
      | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
      !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
   {
      TRACE0("Failed to create toolbar\n");
      return -1;      // fail to create
   }

   if(!m_wndStatusBar.Create(this) ||
      !m_wndStatusBar.SetIndicators(indicators,
        sizeof(indicators)/sizeof(UINT)))
   {
      TRACE0("Failed to create status bar\n");
      return -1;      // fail to create
   }

   // TODO: Delete these three lines if you don't want the toolbar to
   //  be dockable
   m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
   EnableDocking(CBRS_ALIGN_ANY);
   DockControlBar(&m_wndToolBar);
   m_bInitialized = true;

	gpAppFont = new CFont();
	gpAppFont->CreatePointFont ( 90, _T("MS Sans Serif") );

	m_wndToolBar.SetFont ( gpAppFont );
	m_wndStatusBar.SetFont ( gpAppFont );

		/// INFO BAR CODE //////////////////////////
	m_InfoBar.Create(NULL, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | CBRS_TOP,
	         CRect(0,0,0,0), this, AFX_IDW_STATUS_BAR);
	m_InfoBar.SetBarStyle(CBRS_ALIGN_TOP);

 // setup font specifics
    LOGFONT LogFont;
    LogFont.lfHeight = 26;
    LogFont.lfWidth = 0;
    LogFont.lfEscapement = 0;
    LogFont.lfOrientation = 0;
    LogFont.lfWeight = FW_BOLD;
    LogFont.lfItalic = TRUE;
    LogFont.lfUnderline = 0;
    LogFont.lfStrikeOut = 0;
    LogFont.lfCharSet = ANSI_CHARSET;
    LogFont.lfOutPrecision = OUT_TT_PRECIS;
    LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    LogFont.lfQuality = DEFAULT_QUALITY;
    LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    lstrcpy (LogFont.lfFaceName, "Arial");

	m_InfoBar.SetFont(LogFont);
	m_InfoBar.SetTextColor(clrWhite);
	m_InfoBar.SetBackgroundColor(clrRed);
	m_InfoBar.SetBitmap(IDB_BITMAP_LOGO);
	m_InfoBar.SetText(_T("TDMF"));

   return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /* lpcs */,
                                CCreateContext* pContext)
{
 /*  // create splitter window
   if(!m_wndSplitter.CreateStatic(this, 1, 2))
      return FALSE;

   if(!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(LeftView), CSize(100, 100), pContext) ||
      !m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(RightView), CSize(100, 100), pContext))
   {
      m_wndSplitter.DestroyWindow();
      return FALSE;
   }
*/

	m_pCreateContext = pContext;

	m_aryPaneViews[0] = RUNTIME_CLASS(CLeftTreeView);
	m_aryPaneViews[1] = RUNTIME_CLASS(CRightTopView);
	m_aryPaneViews[2] = RUNTIME_CLASS(CTDMFLogicalGroupsView);

	if (!m_aryPaneViews[0] || !m_aryPaneViews[1] || !m_aryPaneViews[2] || !m_aryPaneViews[3])
		return FALSE;

	// create 1st splitter
	if (!m_wndSplitter1.CreateStatic(this, 1, 2))
	{
		TRACE0("Failed to CreateStaticSplitter\n");
		return FALSE;
	}

/*	// add the 2nd splitter pane - which is a nested splitter with 2 rows
	if (!m_wndSplitter2.CreateStatic(
		&m_wndSplitter1,     // our parent window is the first splitter
		3, 1,               // the new splitter is 3 rows, 1 column
		WS_CHILD | WS_VISIBLE | WS_BORDER,  // style, WS_BORDER is needed
		m_wndSplitter1.IdFromRowCol(0, 1)
			// new splitter is in the first row, 2nd column of first splitter
	   ))
	{
		TRACE0("Failed to create nested splitter\n");
		return FALSE;
	}
*/
	// add the first splitter pane
	if (!m_wndSplitter1.CreateView(0, 0, 
		m_aryPaneViews[0], CSize(m_nSplitterVpos, 100), pContext))
	{
		TRACE0("Failed to create first pane\n");
		return FALSE;
	}

		// add the second splitter pane
	if (!m_wndSplitter1.CreateView(0, 1, 
		m_aryPaneViews[1], CSize(m_nSplitterVpos, 100), pContext))
	{
		TRACE0("Failed to create first pane\n");
		return FALSE;
	}

/*	// now create the 3 views inside the nested splitter
	if (!m_wndSplitter2.CreateView(0, 0,
		m_aryPaneViews[1], CSize(100, m_nSplitterHpos), pContext))
	{
		TRACE0("Failed to create first pane\n");
		return FALSE;
	}
	
	if (!m_wndSplitter2.CreateView(1, 0,
		m_aryPaneViews[2], CSize(100, 100), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

	if (!m_wndSplitter2.CreateView(2, 0,
		m_aryPaneViews[3], CSize(100, 100), pContext))
	{
		TRACE0("Failed to create third pane\n");
		return FALSE;
	}
*/

   return TRUE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
   if( !CFrameWnd::PreCreateWindow(cs) )
      return FALSE;
   // TODO: Modify the Window class or styles here by modifying
   //  the CREATESTRUCT cs

   return TRUE;
}

Doc* CMainFrame::GetDocument()
{
   return (Doc*)GetLeftView()->GetDocument();
}

CLeftTreeView* CMainFrame::GetLeftView()
{
   CWnd* pWnd = m_wndSplitter1.GetPane(0, 0);
   return DYNAMIC_DOWNCAST(CLeftTreeView, pWnd);
}

CRightBottomView* CMainFrame::GetRightView()
{
   CWnd* pWnd = m_wndSplitter2.GetPane(0, 1);
   return DYNAMIC_DOWNCAST(CRightBottomView, pWnd);
}



/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
   CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
   CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
	
	if(m_bInitialized)
   {
      UINT nID, nStyle;
      int cxWidth;

      m_wndStatusBar.GetPaneInfo(0, nID, nStyle, cxWidth);
      m_wndStatusBar.SetPaneInfo(0, nID, SBPS_NORMAL, cx / 3);

      m_wndStatusBar.GetPaneInfo(1, nID, nStyle, cxWidth);
      m_wndStatusBar.SetPaneInfo(1, nID, SBPS_NORMAL, cx / 3);

      m_wndStatusBar.GetPaneInfo(2, nID, nStyle, cxWidth);
      m_wndStatusBar.SetPaneInfo(2, nID, SBPS_STRETCH, cx / 3);

		m_wndSplitter1.RecalcLayout();
		
   }
}

void CMainFrame::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CFrameWnd::OnShowWindow(bShow, nStatus);
	if(bShow)
	{
		m_wndSplitter1.SetColumnInfo(0,175,20);
		m_wndSplitter1.SetColumnInfo(1,175,20);
		m_wndSplitter1.RecalcLayout( );	
//		m_wndSplitter2.SetRowInfo(0,300,0);
//		m_wndSplitter2.SetRowInfo(1,200,0);
//		m_wndSplitter2.SetRowInfo(2,100,0);
//		m_wndSplitter2.RecalcLayout( );	
	}
}
