// ProgressInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ProgressInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CProgressInfoDlg dialog

CProgressInfoDlg::CProgressInfoDlg(CString strMsg, UINT nAviID, CWnd* pParent)
	: CDialog(CProgressInfoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProgressInfoDlg)
	m_strMsg = strMsg;
	//}}AFX_DATA_INIT

	Create(CProgressInfoDlg::IDD, pParent);
	m_AnimateCtrl.Open(nAviID);
	m_AnimateCtrl.Play(0, -1, -1);
}

CProgressInfoDlg::~CProgressInfoDlg()
{
	GetParent()->SetFocus();
}

void CProgressInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgressInfoDlg)
	DDX_Control(pDX, IDC_ANIMATE1, m_AnimateCtrl);
	DDX_Text(pDX, IDC_STATIC_MSG, m_strMsg);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProgressInfoDlg, CDialog)
	//{{AFX_MSG_MAP(CProgressInfoDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressInfoDlg message handlers
