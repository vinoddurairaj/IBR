// DebugMonitor.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "DebugMonitor.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDebugMonitor dialog


CDebugMonitor::CDebugMonitor(CWnd* pParent /*=NULL*/)
	: CDialog(CDebugMonitor::IDD, pParent), m_nTraceLevel(0), m_pDoc(NULL)
{

	//{{AFX_DATA_INIT(CDebugMonitor)
	m_AliveAgents                   = _T("N/A");
	m_AliveMsgPerHour               = _T("N/A");
	m_AliveMsgPerMin                = _T("N/A");
	m_AgentAliveSocket              = _T("N/A");
	m_AgentInfo                     = _T("N/A");
	m_AgentInfoRequest              = _T("N/A");
	m_AgentState                    = _T("N/A");
	m_AlertData                     = _T("N/A");
	m_Default                       = _T("N/A");
	m_GetAgentGenConfig             = _T("N/A");
	m_GetAllDevice                  = _T("N/A");
	m_GetDbParams                   = _T("N/A");
	m_GetLgConfig                   = _T("N/A");
	m_GroupMonitoring               = _T("N/A");
	m_GroupState                    = _T("N/A");
	m_GuiMsg                        = _T("N/A");
	m_MonitoringDateRegistration    = _T("N/A");
	m_PerfConfig                    = _T("N/A");
	m_PerfMsg                       = _T("N/A");
	m_Registration_Key              = _T("N/A");
	m_SetAgentGenConfig             = _T("N/A");
	m_SetAllDevices                 = _T("N/A");
	m_SetDbParams                   = _T("N/A");
	m_SetLgConfig                   = _T("N/A");
	m_StatusMsg                     = _T("N/A");
	m_TdmfCommand                   = _T("N/A");
	m_TdmfCommonGuiRegistration     = _T("N/A");
	//}}AFX_DATA_INIT
}

void CDebugMonitor::AddTrace(const CString rgcstrMsg[3])
{
	m_ListCtrlMsg.InsertItem(0, rgcstrMsg[0]);
	m_ListCtrlMsg.SetItemText(0, 1, rgcstrMsg[1]);
	m_ListCtrlMsg.SetItemText(0, 2, rgcstrMsg[2]);
    m_ListCtrlMsg.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
}

void CDebugMonitor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDebugMonitor)
	DDX_Control(pDX, IDC_SAVE, m_ButtonSave);
	DDX_Control(pDX, IDC_LIST_MSG, m_ListCtrlMsg);
    DDX_Text(pDX, IDC_ALIVE_AGENTS, m_AliveAgents);
	DDX_Text(pDX, IDC_ALIVE_MSG_PER_HOUR, m_AliveMsgPerHour);
	DDX_Text(pDX, IDC_ALIVE_MSG_PER_MIN, m_AliveMsgPerMin);
	DDX_Text(pDX, IDC_MESG_AGENT_ALIVE_SOCKET, m_AgentAliveSocket);
	DDX_Text(pDX, IDC_MESG_AGENT_INFO, m_AgentInfo);
	DDX_Text(pDX, IDC_MESG_AGENT_INFO_REQUEST, m_AgentInfoRequest);
	DDX_Text(pDX, IDC_MESG_AGENT_STATE, m_AgentState);
	DDX_Text(pDX, IDC_MESG_ALERT_DATA, m_AlertData);
	DDX_Text(pDX, IDC_MESG_DEFAULT, m_Default);
	DDX_Text(pDX, IDC_MESG_GET_AGENT_GEN_CONFIG, m_GetAgentGenConfig);
	DDX_Text(pDX, IDC_MESG_GET_ALL_DEVICES, m_GetAllDevice);
	DDX_Text(pDX, IDC_MESG_GET_DB_PARAMS, m_GetDbParams);
	DDX_Text(pDX, IDC_MESG_GET_LG_CONFIG, m_GetLgConfig);
	DDX_Text(pDX, IDC_MESG_GROUP_MONITORING, m_GroupMonitoring);
	DDX_Text(pDX, IDC_MESG_GROUP_STATE, m_GroupState);
	DDX_Text(pDX, IDC_MESG_GUI_MSG, m_GuiMsg);
	DDX_Text(pDX, IDC_MESG_MONITORING_DATA_REGISTRATION, m_MonitoringDateRegistration);
	DDX_Text(pDX, IDC_MESG_PERF_CFG_MSG, m_PerfConfig);
	DDX_Text(pDX, IDC_MESG_PERF_MSG, m_PerfMsg);
	DDX_Text(pDX, IDC_MESG_REGISTRATION_KEY, m_Registration_Key);
	DDX_Text(pDX, IDC_MESG_SET_AGENT_GEN_CONFIG, m_SetAgentGenConfig);
	DDX_Text(pDX, IDC_MESG_SET_ALL_DEVICES, m_SetAllDevices);
	DDX_Text(pDX, IDC_MESG_SET_DB_PARAMS, m_SetDbParams);
	DDX_Text(pDX, IDC_MESG_SET_LG_CONFIG, m_SetLgConfig);
	DDX_Text(pDX, IDC_MESG_STATUS_MSG, m_StatusMsg);
	DDX_Text(pDX, IDC_MESG_TDMF_CMD, m_TdmfCommand);
	DDX_Text(pDX, IDC_MESG_TDMFCOMMONGGUI_REGISTRATION, m_TdmfCommonGuiRegistration);
	DDX_Radio(pDX, IDC_RADIO1, m_nTraceLevel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDebugMonitor, CDialog)
	//{{AFX_MSG_MAP(CDebugMonitor)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_SAVE, OnSave)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_RADIO1, OnRadio1)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio1)
	ON_BN_CLICKED(IDC_RADIO3, OnRadio1)
	ON_BN_CLICKED(IDC_RADIO4, OnRadio1)
	ON_BN_CLICKED(IDC_RADIO5, OnRadio1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDebugMonitor message handlers

BOOL CDebugMonitor::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_ListCtrlMsg.InsertColumn(0, "Time");
	m_ListCtrlMsg.SetColumnWidth(0, 130);
	m_ListCtrlMsg.InsertColumn(1, "Level");
	m_ListCtrlMsg.SetColumnWidth(1, 40);
	m_ListCtrlMsg.InsertColumn(2, "Description");

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDebugMonitor::OnSize(UINT nType, int cx, int cy) 
{
   
    CDialog::OnSize(nType, cx, cy);

	if (m_ListCtrlMsg.m_hWnd != NULL)
	{
		CRect Rect;
		m_ListCtrlMsg.GetWindowRect(&Rect);
		ScreenToClient(&Rect);

        m_ListCtrlMsg.MoveWindow(Rect.left, Rect.top, cx - (2 * Rect.left), cy - 250);
		m_ListCtrlMsg.SetColumnWidth(2, LVSCW_AUTOSIZE);

    }
    

}

void CDebugMonitor::OnSave() 
{
	CFileDialog FileDlg(FALSE,
						"log",
						NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						"Log Files (*.log)|*.log|All Files (*.*)|*.*||",
						this);

	if (FileDlg.DoModal() == IDOK)
	{
		CString cstrFile = FileDlg.GetPathName();

		std::ofstream ofs(cstrFile);

		for (int nIndex = 0; nIndex < m_ListCtrlMsg.GetItemCount(); nIndex++)
		{
			ofs << (LPCSTR)m_ListCtrlMsg.GetItemText(nIndex, 0) << " "
				<< (LPCSTR)m_ListCtrlMsg.GetItemText(nIndex, 1) << " "
				<< (LPCSTR)m_ListCtrlMsg.GetItemText(nIndex, 2) << "\n";
		}
	}
}

void CDebugMonitor::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	
	if (bShow)
	{
		if (m_pDoc != NULL)
		{
			m_nTraceLevel = m_pDoc->m_pSystem->TraceLevel;

			UpdateData(FALSE);
		}
        m_ListCtrlMsg.SetColumnWidth(2, LVSCW_AUTOSIZE);
	}
}

void CDebugMonitor::OnRadio1() 
{
	UpdateData();

	if (m_pDoc != NULL)
	{
		m_pDoc->m_pSystem->TraceLevel = m_nTraceLevel;
	}
}


void CDebugMonitor::UpdateTheMsgFromCollectorStats(TDMFOBJECTSLib::ICollectorStatsPtr pICollectorStats)
{

    if(pICollectorStats != NULL)   
    {
        m_AliveAgents.Format(_T("%d"),pICollectorStats->GetAgentsAlive());
	    m_AliveMsgPerHour.Format(_T("%d"), pICollectorStats->GetAliveMsgPerHour());
        m_AliveMsgPerMin.Format(_T("%d"), pICollectorStats->GetAliveMsgPerMin());
	    m_AgentAliveSocket.Format(_T("%d"), pICollectorStats->GetMsg_AgentAliveSocket());
	    m_AgentInfo.Format(_T("%d"), pICollectorStats->GetMsg_AgentInfo());
	    m_AgentInfoRequest.Format(_T("%d"), pICollectorStats->GetMsg_AgentInfoRequest());
	    m_AgentState.Format(_T("%d"), pICollectorStats->GetMsg_AgentState());
	    m_AlertData.Format(_T("%d"), pICollectorStats->GetMsg_AlertData());
	    m_Default.Format(_T("%d"), pICollectorStats->GetMsg_Default());
	    m_GetAgentGenConfig.Format(_T("%d"), pICollectorStats->GetMsg_GetAgentGenConfig());
	    m_GetAllDevice.Format(_T("%d"), pICollectorStats->GetMsg_GetAllDevices());
	    m_GetDbParams.Format(_T("%d"), pICollectorStats->GetMsg_GetDBParams());
	    m_GetLgConfig.Format(_T("%d"), pICollectorStats->GetMsg_GetLGConfig());
	    m_GroupMonitoring.Format(_T("%d"), pICollectorStats->GetMsg_GroupMonitoring());
	    m_GroupState.Format(_T("%d"), pICollectorStats->GetMsg_GroupState());
	    m_GuiMsg.Format(_T("%d"), pICollectorStats->GetMsg_GuiMsg());
	    m_MonitoringDateRegistration.Format(_T("%d"), pICollectorStats->GetMsg_MonitoringDataReg());
	    m_PerfConfig.Format(_T("%d"), pICollectorStats->GetMsg_PerfCfgMsg());
	    m_PerfMsg.Format(_T("%d"), pICollectorStats->GetMsg_PerfMsg());
	    m_Registration_Key.Format(_T("%d"), pICollectorStats->GetMsg_RegistrationKey());
	    m_SetAgentGenConfig.Format(_T("%d"), pICollectorStats->GetMsg_SetAgenGenConfig());
	    m_SetAllDevices.Format(_T("%d"), pICollectorStats->GetMsg_SetAllDevices());
	    m_SetDbParams.Format(_T("%d"), pICollectorStats->GetMsg_SetDBParams());
	    m_SetLgConfig.Format(_T("%d"), pICollectorStats->GetMsg_SetLGConfig());
	    m_StatusMsg.Format(_T("%d"), pICollectorStats->GetMsg_StatusMsg());
	    m_TdmfCommand.Format(_T("%d"), pICollectorStats->GetMsg_TDMFCmd());
	    m_TdmfCommonGuiRegistration.Format(_T("%d"), pICollectorStats->GetMsg_TDMFCommonGuiReg());

        UpdateData(false);

    }

}


