// ViewHostTop.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfgui.h"
#include "Doc.h"
#include "MainFrm.h"
#include "ViewHostTop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ViewHostTop

IMPLEMENT_DYNCREATE(ViewHostTop, CPageView)

ViewHostTop::ViewHostTop() : CPageView(IDD)
{
	//{{AFX_DATA_INIT(ViewHostTop)
	m_strBarFilled = _T("");
	m_strServerName = _T("");
	m_strDomain = _T("");
	m_bIsSource = TRUE;
	m_bIsTarget = FALSE;
	//}}AFX_DATA_INIT
}

ViewHostTop::~ViewHostTop()
{
}

void ViewHostTop::DoDataExchange(CDataExchange* pDX)
{
	CPageView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ViewHostTop)
	DDX_Control(pDX, IDC_IPADDRESS, m_IPAddress);
	DDX_Control(pDX, IDC_PROGRESS_BAB, m_PrgrBABFilled);
	DDX_Text(pDX, IDC_STATIC_BAB_FILLED, m_strBarFilled);
	DDX_Text(pDX, IDC_EDT_NAME, m_strServerName);
	DDX_Text(pDX, IDC_EDT_DOMAIN, m_strDomain);
	DDX_Check(pDX, IDC_CHECK_ISSOURCE, m_bIsSource);
	DDX_Check(pDX, IDC_CHECK_ISTARGET, m_bIsTarget);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ViewHostTop, CPageView)
	//{{AFX_MSG_MAP(ViewHostTop)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ViewHostTop diagnostics

#ifdef _DEBUG
void ViewHostTop::AssertValid() const
{
	CPageView::AssertValid();
}

void ViewHostTop::Dump(CDumpContext& dc) const
{
	CPageView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// ViewHostTop message handlers

void ViewHostTop::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   Doc* pDoc = ((CMainFrame*)AfxGetMainWnd())->GetDocument();
   const DocServerInfo& dsi = pDoc->GetServerInfo();
	if(dsi.bValid)
   {
      m_strServerName = dsi.strName;
      m_strDomain = dsi.strDomain;
      m_IPAddress.SetAddress(dsi.IPAddress.nField0,
                             dsi.IPAddress.nField1,
                             dsi.IPAddress.nField2,
                             dsi.IPAddress.nField3);
      
      COLORREF clrPrgr = (dsi.nBABUsed >= 90) ? RGB(255, 0, 0) : RGB(0, 0, 255);
      m_PrgrBABFilled.SendMessage(PBM_SETBARCOLOR, 0, clrPrgr);     
      m_PrgrBABFilled.SetPos(dsi.nBABUsed);
      m_strBarFilled.FormatMessage(_T("BAB is filled to\n%1!d!%% of capacity"), dsi.nBABUsed);
      UpdateData(FALSE);
   }
   else
   {
      m_IPAddress.ClearAddress();
   }
}

void ViewHostTop::OnInitialUpdate() 
{
	CPageView::OnInitialUpdate();
   m_PrgrBABFilled.SetRange(0, 100);
}

void ViewHostTop::Update()
{
	OnUpdate(NULL, 0, NULL);
}


HBRUSH ViewHostTop::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPageView::OnCtlColor(pDC, pWnd, nCtlColor);
	
	// TODO: Change any attributes of the DC here
	
	// TODO: Return a different brush if the default is not desired

   if (pWnd->GetDlgCtrlID() == IDC_PROGRESS_BAB)
   {
   	return hbr;
   }   


	return hbr;
}
