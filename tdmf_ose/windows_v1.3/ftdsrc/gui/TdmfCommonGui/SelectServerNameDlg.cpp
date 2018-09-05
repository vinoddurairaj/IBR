// SelectServerNameDlg.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "SelectServerNameDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectServerNameDlg dialog


CSelectServerNameDlg::CSelectServerNameDlg(TDMFOBJECTSLib::IDomain* pDomain, CTdmfCommonGuiDoc* pDoc, std::string* pstrTargetHostName , CWnd* pParent /*=NULL*/)
	: CDialog(CSelectServerNameDlg::IDD, pParent), m_pDomain(pDomain), m_pstrTargetHostName(pstrTargetHostName),  m_pDoc(pDoc)
{
	//{{AFX_DATA_INIT(CSelectServerNameDlg)
	m_StrMessage = _T("");
	//}}AFX_DATA_INIT
}


void CSelectServerNameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectServerNameDlg)
	DDX_Control(pDX, IDC_LIST_SERVER, m_List_Server);
	DDX_Text(pDX, IDC_EDIT_MESSAGE, m_StrMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectServerNameDlg, CDialog)
	//{{AFX_MSG_MAP(CSelectServerNameDlg)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectServerNameDlg message handlers

BOOL CSelectServerNameDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    m_StrMessage.Format("The system cannot find the Target Server with this IP adress: \"%s\".\r\n\r\nPlease choose a server to associate with this address from the following list:",m_pstrTargetHostName->c_str());
    UpdateData(FALSE);

	try
	{
		long nNbServer = m_pDomain->ServerCount;
		for (long i = 0; i < nNbServer; i++)
		{
			TDMFOBJECTSLib::IServerPtr pServer = m_pDomain->GetServer(i);
			
			int nIndex = m_List_Server.AddString(pServer->Name);
			
			m_List_Server.SetItemDataPtr(nIndex, pServer);
			pServer->AddRef();
		}
		m_List_Server.SetCurSel(0);
	}
	CATCH_ALL_LOG_ERROR(1038);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectServerNameDlg::OnOK() 
{
	m_nServerId = ((TDMFOBJECTSLib::IServer*)m_List_Server.GetItemDataPtr(m_List_Server.GetCurSel()))->GetKey();

	CDialog::OnOK();
}

void CSelectServerNameDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	for (int nIndex = 0; nIndex < m_List_Server.GetCount(); nIndex++)
	{
		TDMFOBJECTSLib::IServer* pServer = (TDMFOBJECTSLib::IServer*)m_List_Server.GetItemDataPtr(nIndex);
		pServer->Release();
	}
}
