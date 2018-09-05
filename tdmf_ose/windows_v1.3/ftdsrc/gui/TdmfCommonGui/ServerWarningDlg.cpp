// ServerWarningDlg.cpp : implementation file
//

#include "stdafx.h"
#include "../../tdmf.inc"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "ServerWarningDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerWarningDlg dialog


CServerWarningDlg::CServerWarningDlg(CTdmfCommonGuiDoc* pDoc, TDMFOBJECTSLib::IServer* pServer, CWnd* pParent /*=NULL*/)
	: CDialog(CServerWarningDlg::IDD, pParent), m_pServer(pServer), m_pDoc(pDoc)
{
	//{{AFX_DATA_INIT(CServerWarningDlg)
	m_bShowWarnings = FALSE;
	m_nRequested = 0;
	m_nAllocated = 0;
	m_cstrMsg = _T("");
	//}}AFX_DATA_INIT
}


void CServerWarningDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerWarningDlg)
	DDX_Check(pDX, IDC_CHECK_MSG, m_bShowWarnings);
	DDX_Text(pDX, IDC_EDIT_REQ, m_nRequested);
	DDX_Text(pDX, IDC_EDIT_ACT, m_nAllocated);
	DDX_Text(pDX, IDC_EDIT_MSG, m_cstrMsg);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerWarningDlg, CDialog)
	//{{AFX_MSG_MAP(CServerWarningDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerWarningDlg message handlers

BOOL CServerWarningDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CString cstrMsg;
	cstrMsg.Format("Replication Server %S was unable to allocate the requested amount of BAB", (BSTR)m_pServer->Name);
	
	m_cstrMsg = cstrMsg;
	m_nAllocated = m_pServer->BABSizeAllocated;
	m_nRequested = m_pServer->BABSize;

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CServerWarningDlg::OnOK() 
{
	UpdateData();

	m_pDoc->m_bShowServersWarnings = m_bShowWarnings ? FALSE : TRUE;

	#define FTD_SOFTWARE_KEY "Software\\" OEMNAME "\\Dtc\\CurrentVersion"
	HKEY hKey;
	if (RegCreateKeyEx(HKEY_CURRENT_USER,
					   FTD_SOFTWARE_KEY,
					   NULL,
					   NULL,
					   REG_OPTION_NON_VOLATILE,
					   KEY_ALL_ACCESS,
					   NULL,
					   &hKey,
					   NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, "DtcServersWarnings", 0, REG_DWORD, (LPBYTE)&m_pDoc->m_bShowServersWarnings, sizeof(m_pDoc->m_bShowServersWarnings));
		RegFlushKey(hKey);
		RegCloseKey(hKey);
	}

	CDialog::OnOK();
}
