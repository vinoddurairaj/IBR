// MainFilterView.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "MainFilterView.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFilterView

IMPLEMENT_DYNCREATE(CMainFilterView, CFormView)

CMainFilterView::CMainFilterView()
	: CFormView(CMainFilterView::IDD)
{
	//{{AFX_DATA_INIT(CMainFilterView)
	//}}AFX_DATA_INIT
}

CMainFilterView::~CMainFilterView()
{
}

void CMainFilterView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMainFilterView)
	DDX_Control(pDX, IDC_CBO_OBJECT, m_CBO_ObjectCtrl);
	DDX_Control(pDX, IDC_CBO_SORT_TYPE, m_CBO_SortTypeCtrl);
	DDX_Control(pDX, IDC_CBO_SORTBY, m_CBO_SortByCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMainFilterView, CFormView)
	//{{AFX_MSG_MAP(CMainFilterView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFilterView diagnostics

#ifdef _DEBUG
void CMainFilterView::AssertValid() const
{
	CFormView::AssertValid();
}

void CMainFilterView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFilterView message handlers


