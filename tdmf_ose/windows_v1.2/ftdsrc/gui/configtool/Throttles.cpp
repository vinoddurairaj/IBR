// Throttles.cpp : implementation file
//

#include "stdafx.h"
#include "DTCConfigTool.h"
#include "Throttles.h"
#include "Config.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CConfig	*lpConfig;
/////////////////////////////////////////////////////////////////////////////
// CThrottles property page

IMPLEMENT_DYNCREATE(CThrottles, CPropertyPage)

CThrottles::CThrottles() : CPropertyPage(CThrottles::IDD)
{
	//{{AFX_DATA_INIT(CThrottles)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CThrottles::~CThrottles()
{
}

void CThrottles::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CThrottles)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CThrottles, CPropertyPage)
	//{{AFX_MSG_MAP(CThrottles)
	ON_BN_CLICKED(IDC_BUTTON_THROTTLE_BUILDER, OnButtonThrottleBuilder)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThrottles message handlers

void CThrottles::OnButtonThrottleBuilder() 
{
	::MessageBox(NULL, "Throttle Builder", "Throttles", MB_OK);	
}

BOOL CThrottles::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	SetDlgItemText( IDC_EDIT_THROTTLE_EDITOR, lpConfig->m_structThrottleValues.m_strThrottle );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
