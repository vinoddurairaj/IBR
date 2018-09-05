// LeftView.cpp : implementation of the CLeftView class
//

#include "stdafx.h"
#include "TDMFGUI.h"

#include "Doc.h"
#include "RightBottomView.h"
#include "LeftView.h"
#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CPageView)

BEGIN_MESSAGE_MAP(CLeftView, CPageView)
	//{{AFX_MSG_MAP(CLeftView)
	ON_WM_SIZE()
	ON_NOTIFY(TVN_SELCHANGING, IDC_LEFTVIEW_TREECTRL, OnSelChanging)
	ON_NOTIFY(TVN_SELCHANGED, IDC_LEFTVIEW_TREECTRL, OnSelChanged)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CPageView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CPageView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CPageView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLeftView construction/destruction

CLeftView::CLeftView()
{
	// TODO: add construction code here

}

CLeftView::~CLeftView()
{
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CPageView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView drawing

void CLeftView::OnDraw(CDC* pDC)
{
	Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}


/////////////////////////////////////////////////////////////////////////////
// CLeftView printing

BOOL CLeftView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CLeftView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CLeftView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CPageView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}

Doc* CLeftView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(Doc)));
	return (Doc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLeftView message handlers

void CLeftView::OnInitialUpdate() 
{
   CPageView::OnInitialUpdate();
   
   m_TreeImageList.Create(16, 16, ILC_COLOR | ILC_MASK, 0, 3);
   m_TreeImageList.SetBkColor(CLR_NONE);
   m_TreeImageList.Add(AfxGetApp()->LoadIcon(IDI_ICON_DOMAIN));
   m_TreeImageList.Add(AfxGetApp()->LoadIcon(IDI_ICON_MACHINE));
   m_TreeImageList.Add(AfxGetApp()->LoadIcon(IDI_ICON_DRIVE));

   CRect rect;
   GetClientRect(&rect);
   m_Tree.Create(0
                 //| WS_VISIBLE
                 | WS_TABSTOP
                 | WS_CHILD
                 //| WS_BORDER
                 | TVS_HASBUTTONS
                 | TVS_LINESATROOT
                 | TVS_HASLINES
                 | TVS_DISABLEDRAGDROP
                 | 0 , rect, this, IDC_LEFTVIEW_TREECTRL);

   m_Tree.SetImageList(&m_TreeImageList, TVSIL_NORMAL);

   Doc* pDoc = GetDocument();
   BOOL bOk = pDoc->GetAllDomainNames(m_Tree);
   HTREEITEM m_hTreeRoot = m_Tree.GetRootItem();  // retrieves h1
}

void CLeftView::OnSize(UINT nType, int cx, int cy) 
{
	CPageView::OnSize(nType, cx, cy);
	
	if(IsWindow(m_Tree.m_hWnd))
   {
      m_Tree.SetWindowPos(0, 0, 0, cx, cy, SWP_SHOWWINDOW | SWP_NOZORDER);
   }
}

void CLeftView::OnSelChanging(LPNMHDR pnmhdr, LRESULT *pLResult)
{
   HTREEITEM hItem = m_Tree.GetSelectedItem();
   TVITEM item;
   m_Tree.GetItem(&item);
}

// struct STempAndDummyServerInfo
// {
//    int nNodeSequence;
//    int nNodeType;
//    SIPAddress ip;
//    int nBABUsed;
// };

void CLeftView::OnSelChanged(LPNMHDR pnmhdr, LRESULT *pLResult)
{
   GetDocument()->GetServerInfo(m_Tree);

	
}

void CLeftView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	m_Tree.SetBkColor( COLOR_LEVEL1 );	
	// Do not call CPageView::OnPaint() for painting messages
}
