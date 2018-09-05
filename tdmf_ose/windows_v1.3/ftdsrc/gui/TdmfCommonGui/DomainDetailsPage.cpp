// DomainDetailsPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "DomainDetailsPage.h"
#include "ToolsView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDomainDetailsPage property page

IMPLEMENT_DYNCREATE(CDomainDetailsPage, CPropertyPage)

CDomainDetailsPage::CDomainDetailsPage()
	: CPropertyPage(CDomainDetailsPage::IDD)
{
	//{{AFX_DATA_INIT(CDomainDetailsPage)
	m_strDescription = _T("");
	m_strTitle = _T("");
	//}}AFX_DATA_INIT
}

CDomainDetailsPage::~CDomainDetailsPage()
{
}

void CDomainDetailsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDomainDetailsPage)
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_EditDescription);
	DDX_Control(pDX, IDC_RICHEDIT_TITLE, m_RichEditTitle);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDX_Text(pDX, IDC_RICHEDIT_TITLE, m_strTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDomainDetailsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDomainDetailsPage)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDomainDetailsPage message handlers

BOOL CDomainDetailsPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_RichEditTitle.SetBackgroundColor(FALSE, GetSysColor(COLOR_BTNFACE));

	CHARFORMAT cf;
	cf.dwMask = CFM_BOLD | CFM_SIZE;
	m_RichEditTitle.GetDefaultCharFormat(cf);
	cf.dwEffects = CFE_BOLD;
	cf.yHeight = (LONG)(cf.yHeight * 1.5);
	m_RichEditTitle.SetDefaultCharFormat(cf);

	RefreshValues();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDomainDetailsPage::RefreshValues()
{
	try
	{
		CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

		if (pDoc->GetSelectedDomain() != NULL)
		{
			m_strTitle = (BSTR)pDoc->GetSelectedDomain()->Name;
			m_strDescription = (BSTR)pDoc->GetSelectedDomain()->Description;

			UpdateData(FALSE);
		}
	}
	CATCH_ALL_LOG_ERROR(1002);
}


BOOL CDomainDetailsPage::OnSetActive() 
{
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars();
	
	return CPropertyPage::OnSetActive();
}

void CDomainDetailsPage::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);
	
	CRect Rect;

	if (m_RichEditTitle.m_hWnd != NULL)
	{
		m_RichEditTitle.GetWindowRect(&Rect);
		ScreenToClient(&Rect);
		Rect.right = cx - 10;

		m_RichEditTitle.MoveWindow(&Rect);
	}

	if (m_EditDescription.m_hWnd != NULL)
	{
		m_EditDescription.GetWindowRect(&Rect);
		ScreenToClient(&Rect);
		Rect.right = cx - 10;

		m_EditDescription.MoveWindow(&Rect);
	}
}
