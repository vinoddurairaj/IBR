// PageView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "PageView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPageView

IMPLEMENT_DYNCREATE(CPageView, CFormView)

CPageView::CPageView() : CFormView(IDD_PPG_EMPTY)
{
 	m_crBackground = GetSysColor(COLOR_3DFACE);
	m_wndbkBrush.CreateSolidBrush(m_crBackground); 

}

CPageView::CPageView(UINT nIDTemplate) : CFormView(nIDTemplate)
{
	m_crBackground = GetSysColor(COLOR_3DFACE);
	m_wndbkBrush.CreateSolidBrush(m_crBackground); 

}

CPageView::~CPageView()
{
	if(m_wndbkBrush.GetSafeHandle())
		m_wndbkBrush.DeleteObject();
}

void CPageView::Update()
{
	CFormView::OnUpdate(NULL, 0, NULL);
}

void CPageView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPageView, CFormView)
	//{{AFX_MSG_MAP(CPageView)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPageView::SetBackgroundColor(COLORREF crBackground)
{
	m_crBackground = crBackground;
	
	if(m_wndbkBrush.GetSafeHandle())
		m_wndbkBrush.DeleteObject();

	m_wndbkBrush.CreateSolidBrush(m_crBackground); 
}

BOOL CPageView::OnEraseBkgnd(CDC* pDC) 
{
	CFormView::OnEraseBkgnd(pDC);

	CRect rect;
	GetClientRect(rect);

	pDC->FillRect(&rect, &m_wndbkBrush);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPageView diagnostics

#ifdef _DEBUG
void CPageView::AssertValid() const
{
	CFormView::AssertValid();
}

void CPageView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

Doc* CPageView::GetDocument() // non-debug version is inline
{
	Doc* pDoc = ((CMainFrame*)AfxGetApp()->m_pMainWnd)->GetDocument();
	return pDoc;
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPageView message handlers

void CPageView::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
	
}



int CPageView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFormView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	
	return 0;
}
