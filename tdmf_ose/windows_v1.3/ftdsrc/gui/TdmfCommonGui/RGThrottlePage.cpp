// RGThrottlePage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "RGThrottlePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRGThrottlePage property page

IMPLEMENT_DYNCREATE(CRGThrottlePage, CPropertyPage)

CRGThrottlePage::CRGThrottlePage(TDMFOBJECTSLib::IReplicationGroup *pRG, bool bReadOnly) 
: CPropertyPage(CRGThrottlePage::IDD) , m_pRG(pRG), m_bReadOnly(bReadOnly)
{
	//{{AFX_DATA_INIT(CRGThrottlePage)
	m_strThrottles = _T("");
	//}}AFX_DATA_INIT
}

CRGThrottlePage::~CRGThrottlePage()
{
}

void CRGThrottlePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGThrottlePage)
	DDX_Control(pDX, IDC_EDIT_THROTTLE, m_Edit_Throttle_Ctrl);
	DDX_Text(pDX, IDC_EDIT_THROTTLE, m_strThrottles);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGThrottlePage, CPropertyPage)
	//{{AFX_MSG_MAP(CRGThrottlePage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGThrottlePage message handlers

BOOL CRGThrottlePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	

	if (m_pRG != NULL)
	{
		m_strThrottles = (BSTR)m_pRG->Throttles;
		m_strThrottles.Replace("\n", "\r\n");
	}
	
	m_Edit_Throttle_Ctrl.SetReadOnly();
	
	UpdateData(FALSE);
	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
