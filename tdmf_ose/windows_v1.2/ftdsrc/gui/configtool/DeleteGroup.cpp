// DeleteGroup.cpp : implementation file
//

#include "stdafx.h"
#include "DTCConfigTool.h"
#include "DeleteGroup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDeleteGroup dialog


CDeleteGroup::CDeleteGroup(CWnd* pParent /*=NULL*/)
	: CDialog(CDeleteGroup::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDeleteGroup)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDeleteGroup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeleteGroup)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeleteGroup, CDialog)
	//{{AFX_MSG_MAP(CDeleteGroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeleteGroup message handlers

void CDeleteGroup::OnOK() 
{
	m_bDeleteGroup = TRUE;
	
	CDialog::OnOK();
}

void CDeleteGroup::OnCancel() 
{
	m_bDeleteGroup = FALSE;
	
	CDialog::OnCancel();
}

BOOL CDeleteGroup::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_bDeleteGroup = FALSE;	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
