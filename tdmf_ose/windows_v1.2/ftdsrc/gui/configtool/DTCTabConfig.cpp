// DTCTabConfig.cpp : implementation file
//

#include "stdafx.h"
#include "DTCConfigTool.h"
#include "DTCTabConfig.h"
#include "Systems.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDTCTabConfig dialog


CDTCTabConfig::CDTCTabConfig(CWnd* pParent /*=NULL*/)
	: CDialog(CDTCTabConfig::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDTCTabConfig)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDTCTabConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDTCTabConfig)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDTCTabConfig, CDialog)
	//{{AFX_MSG_MAP(CDTCTabConfig)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDTCTabConfig message handlers

BOOL CDTCTabConfig::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
