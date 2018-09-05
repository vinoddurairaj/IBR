// TimeRangeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "TimeRangeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimeRangeDlg dialog


CTimeRangeDlg::CTimeRangeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTimeRangeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTimeRangeDlg)
	m_value = 10;
	//}}AFX_DATA_INIT
}


void CTimeRangeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTimeRangeDlg)
	DDX_Control(pDX, IDC_SPIN_TIMERANGE, m_Spin_CTRL);
	DDX_Text(pDX, IDC_TXT_TIMERANGE, m_value);
	DDV_MinMaxUInt(pDX, m_value, 10, 60);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTimeRangeDlg, CDialog)
	//{{AFX_MSG_MAP(CTimeRangeDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimeRangeDlg message handlers

BOOL CTimeRangeDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_Spin_CTRL.SetRange(10, 60 );	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
