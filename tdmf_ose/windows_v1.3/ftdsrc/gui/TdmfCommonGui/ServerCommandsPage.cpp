// ServerCommandsPage.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "tdmfcommonguiDoc.h"
#include "ServerCommandsPage.h"
#include "ViewNotification.h"
#include "ToolsView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CServerCommandsPage property page

IMPLEMENT_DYNCREATE(CServerCommandsPage, CPropertyPage)

CServerCommandsPage::CServerCommandsPage () : 
	CPropertyPage(CServerCommandsPage::IDD), m_bServer(true)
{
	//{{AFX_DATA_INIT(CServerCommandsPage)
	m_szCmdDesc = _T("");
	m_szCmd = _T("");
	m_szResult = _T("");
	//}}AFX_DATA_INIT
} // CServerCommandsPage::CServerCommandsPage ()

CServerCommandsPage::~CServerCommandsPage ()
{

} // CServerCommandsPage::~CServerCommandsPage ()

void CServerCommandsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerCommandsPage)
	DDX_Control(pDX, IDC_EDIT_CMD_RESULT, m_CtrlLast);
	DDX_Control(pDX, IDC_COMBO_CMD_HST, m_CboCmdHst);
	DDX_Control(pDX, IDC_COMBO_CMD_LST, m_CboCmdLst);
	DDX_Text(pDX, IDC_EDIT_CMD_DESC, m_szCmdDesc);
	DDX_CBString(pDX, IDC_COMBO_CMD_HST, m_szCmd);
	DDX_Text(pDX, IDC_EDIT_CMD_RESULT, m_szResult);
	//}}AFX_DATA_MAP
}

void CServerCommandsPage::PushBackCommand(char* pszCmd, char* pszArg,
										  TDMFOBJECTSLib::tagTdmfCommand eCmd)
{
	struct StCmd  Cmd;

	Cmd.strCmd = pszCmd;
	Cmd.strArg = pszArg;
	Cmd.eCmd   = eCmd;
	m_vecStCmd.push_back(Cmd);
}

void CServerCommandsPage::FillCommandVector()
{
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
	BOOL bWindows = (strstr(pDoc->GetSelectedServer()->OSType, "Windows") != 0 ||
		    		 strstr(pDoc->GetSelectedServer()->OSType, "windows") != 0 ||
				     strstr(pDoc->GetSelectedServer()->OSType, "WINDOWS") != 0);

	m_vecStCmd.clear();

	if (bWindows)
	{
		PushBackCommand("analyzer",      "[-a] [-f [volumename]] [-h] [-l] [-p pid] [-v]", TDMFOBJECTSLib::CMD_HANDLE);
	}
	PushBackCommand("checkpoint",    "[-g+#{group}|-a]+[-p|-s]+[-on|-off]",TDMFOBJECTSLib::CMD_CHECKPOINT);
	PushBackCommand("hostinfo",      "[${Host Name}]",            TDMFOBJECTSLib::CMD_HOSTINFO);
	PushBackCommand("info",          "[-g+#{group}|-a|-v]",       TDMFOBJECTSLib::CMD_INFO);
	PushBackCommand("init",          "[-b+#{BabSizeMb]",          TDMFOBJECTSLib::CMD_INIT);
	PushBackCommand("killbackfresh", "[-g+#{group}|-a|-h]",       TDMFOBJECTSLib::CMD_KILL_BACKFRESH);
	PushBackCommand("killpmd",       "[-g+#{group}|-a|-h]",       TDMFOBJECTSLib::CMD_KILL_PMD);
	PushBackCommand("killrefresh",   "[-g+#{group}|-a|-h]",       TDMFOBJECTSLib::CMD_KILL_REFRESH);
	PushBackCommand("killrmd",           "[-g+#{group}|-a]",      TDMFOBJECTSLib::CMD_KILL_RMD);
	PushBackCommand("launchbackfresh", "[-g+#{group}|-a]",        TDMFOBJECTSLib::CMD_LAUNCH_BACKFRESH);
	PushBackCommand("launchpmd",     "[-g+#{group}|-a]",          TDMFOBJECTSLib::CMD_LAUNCH_PMD);
	PushBackCommand("launchrefresh", "[-g+#{group}&[-f|-c]|-a]&[-f|-c]",TDMFOBJECTSLib::CMD_LAUNCH_REFRESH);
	PushBackCommand("licinfo",       "",                          TDMFOBJECTSLib::CMD_LICINFO);
	PushBackCommand("override",      "[-g+#{group}|-a]+[clear BAB]|[state+passthru|normal|tracking|refresh|backfresh]",TDMFOBJECTSLib::CMD_OVERRIDE);
	PushBackCommand("panalyze",      "[-g+#{group}|-v]",          TDMFOBJECTSLib::CMD_PANALYZE);
	PushBackCommand("reco",          "[-g+#{group}|-a]+[-d]",     TDMFOBJECTSLib::CMD_RECO);
	PushBackCommand("set",           "[-g+#{group}]+[{parameter_name}={value}]",TDMFOBJECTSLib::CMD_SET);
	PushBackCommand("start",         "[-g+#{group}|-a][-b][-f]",  TDMFOBJECTSLib::CMD_START);
	PushBackCommand("stop",          "[-g+#{group}|-a|-s]",       TDMFOBJECTSLib::CMD_STOP);
	if (bWindows)
	{
		PushBackCommand("trace",         "[-lx|-off|-on] [-b]",       TDMFOBJECTSLib::CMD_TRACE);
	}
}

CString CServerCommandsPage::GetDefaultArg(int nItemIndex)
{
	CString strArg;

	// If we are in the server command then
	// by default, use the option : -a (if exist)
	if (strstr(m_vecStCmd[nItemIndex].strArg.c_str(), "-a"))
	{
		strArg = "-a";
	}
	else if (m_vecStCmd[nItemIndex].strCmd == "hostinfo")
	{
		CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

		strArg = BSTR(pDoc->GetSelectedServer()->Name);
	}

	return strArg;
}


BEGIN_MESSAGE_MAP(CServerCommandsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CServerCommandsPage)
	ON_CBN_SELCHANGE(IDC_COMBO_CMD_LST, OnSelchangeComboCmdLst)
	ON_BN_CLICKED(IDC_BUT_GO, OnButGo)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerCommandsPage message handlers

BOOL CServerCommandsPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog ();

	FillCommandVector();

	CEdit* lpEdt = (CEdit*)(m_CboCmdLst.GetWindow(GW_CHILD));
	lpEdt->SetReadOnly ();

	for (unsigned int nIndex = 0; nIndex < m_vecStCmd.size(); nIndex++)
	{
		m_CboCmdLst.AddString(m_vecStCmd[nIndex].strCmd.c_str());
	}

	m_CboCmdLst.SetCurSel(0);

	OnSelchangeComboCmdLst();

	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
	CString cstrHistory;
	// Display last command output for this server or group
	if (m_bServer)
	{
		m_szResult = (BSTR)pDoc->GetSelectedServer()->LastCmdOutput;
		cstrHistory = (BSTR)pDoc->GetSelectedServer()->CmdHistory;
	}
	else
	{
		m_szResult = (BSTR)pDoc->GetSelectedReplicationGroup()->LastCmdOutput;
		cstrHistory = (BSTR)pDoc->GetSelectedReplicationGroup()->CmdHistory;
	}

    m_szResult.Replace("\n", "\r\n");
	// Rebuild command history for this server or group
	while (!cstrHistory.IsEmpty())
	{
		CString cstrCmd;
		int nIndex = cstrHistory.Find(';');

		if (nIndex > 0)
		{
			cstrCmd = cstrHistory.Left(nIndex);
			cstrHistory.TrimLeft(cstrCmd+";");
		}
		else
		{
			cstrCmd = cstrHistory;
			cstrHistory.Empty();
		}

		m_CboCmdHst.InsertString(0, cstrCmd);
	}


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE

} // CServerCommandsPage::OnInitDialog()

void CServerCommandsPage::OnSelchangeComboCmdLst()
{
	try
	{
		int nCurSel = m_CboCmdLst.GetCurSel();

		// Set Cmd description field (args)
		m_szCmdDesc = m_vecStCmd[nCurSel].strArg.c_str();

		// Build command		
		m_szCmd  = m_vecStCmd[nCurSel].strCmd.c_str();
		m_szCmd += " ";
		m_szCmd += GetDefaultArg(nCurSel);
		
		UpdateData(FALSE);
		
		// Positon cursor at the end of the Command combo box
		m_CboCmdHst.SetFocus();
		CEdit* lpEdt = (CEdit*)(m_CboCmdHst.GetWindow(GW_CHILD));
		int    liLen = m_szCmd.GetLength();
		lpEdt->SetSel(liLen, liLen); // Position at the end
	}
	CATCH_ALL_LOG_ERROR(1039);

} // CServerCommandsPage::OnSelchangeComboCmdLst ()

void CServerCommandsPage::OnButGo()
{
	CWaitCursor WaitCursor;

	try
	{
		// Reparse command. It may have been edited by the user
		UpdateData();
		CString strTmp = m_szCmd;
		strTmp.TrimLeft();
		CString strCmd = strTmp.SpanExcluding(" ");

		CString strArg = strTmp.Right(strTmp.GetLength() - strCmd.GetLength());
		strArg.TrimLeft();

		// Check if the command is a defined one
		// and find the command ID
		TDMFOBJECTSLib::tagTdmfCommand eCmd;
		for (unsigned int nIndex = 0; nIndex < m_vecStCmd.size(); nIndex++)
		{
			if (strCmd == m_vecStCmd[nIndex].strCmd.c_str())
			{
				eCmd = m_vecStCmd[nIndex].eCmd;
				break;
			}
		}
		if (nIndex >= m_vecStCmd.size())
		{
			MessageBox("The command entered is undefined.", "Error", MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			// Store command
			m_CboCmdHst.InsertString(0, m_szCmd);

			// Send command
			CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
			BSTR bstrTmp;
			long nRetCode;

			if (m_bServer)
			{
				nRetCode = pDoc->GetSelectedServer()->LaunchCommand(eCmd, _bstr_t(strArg), _bstr_t(m_szCmd), &bstrTmp);
			}
			else
			{
				nRetCode = pDoc->GetSelectedReplicationGroup()->LaunchCommand(eCmd, _bstr_t(strArg), _bstr_t(m_szCmd), FALSE, &bstrTmp);
			}
			_bstr_t bstrMsg(bstrTmp, false);

			// Display an error msg
			if (nRetCode != 0)
			{
				if (bstrMsg.length() > 0)
				{
					m_szResult = (LPCSTR)bstrMsg;
				}
				else
				{
					std::ostringstream oss;
					oss << (LPCSTR)m_szCmd <<": The command has returned the following error code: " << nRetCode;
					m_szResult = oss.str().c_str();
				}
			}
			else
			{
				// Display the command's result
				if (m_bServer)
				{
					m_szResult = (BSTR)pDoc->GetSelectedServer()->LastCmdOutput;
				}
				else
				{
					m_szResult = (BSTR)pDoc->GetSelectedReplicationGroup()->LastCmdOutput;
				}
			}

			m_szResult.Replace("\n", "\r\n");
			UpdateData(FALSE);
		}
	}
	CATCH_ALL_LOG_ERROR(1040);
}

BOOL CServerCommandsPage::PreTranslateMessage(MSG* pMsg) 
{
	if ((pMsg->message == WM_KEYDOWN) &&
		(pMsg->wParam == VK_TAB))
	{
		if ((GetFocus() == &m_CtrlLast) && (!(GetKeyState(VK_SHIFT) & ~1)))
		{
			CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
			CViewNotification  VN;
			
			VN.m_nMessageId = CViewNotification::TAB_TOOLS_NEXT;
			pDoc->UpdateAllViews(NULL, CViewNotification::VIEW_NOTIFICATION, &VN);
			
			return TRUE;
		}
	}

	if ((pMsg->message == WM_KEYDOWN) &&
		(pMsg->wParam == VK_RETURN))
	{
		OnButGo();
		return TRUE;
	}

	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CServerCommandsPage::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);
	
	if (m_CtrlLast.m_hWnd != NULL)
	{
		RECT Rect;
		m_CtrlLast.GetWindowRect(&Rect);
		ScreenToClient(&Rect);
		Rect.bottom = cy - 7;
		Rect.right  = cx - 7;

		if ((Rect.bottom - Rect.top) >= 115)
		{
			m_CtrlLast.MoveWindow(&Rect);
		}
	}
}


BOOL CServerCommandsPage::OnSetActive() 
{
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars();
	
	return CPropertyPage::OnSetActive();
}
