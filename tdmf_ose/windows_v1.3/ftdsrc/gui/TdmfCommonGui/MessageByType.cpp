// MessageByType.cpp : implementation file
//

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "MessageByType.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMessageByTypeDlg dialog


CMessageByTypeDlg::CMessageByTypeDlg(TDMFOBJECTSLib::ISystem* pSystem,CWnd* pParent /*=NULL*/)
	: CDialog(CMessageByTypeDlg::IDD, pParent), m_pSystem(pSystem)
{
	//{{AFX_DATA_INIT(CMessageByTypeDlg)
	m_AliveAgents                   = _T("");
	m_AliveMsgPerHour               = _T("");
	m_AliveMsgPerMin                = _T("");
	m_AgentAliveSocket              = _T("");
	m_AgentInfo                     = _T("");
	m_AgentInfoRequest              = _T("");
	m_AgentState                    = _T("");
	m_AlertData                     = _T("");
	m_Default                       = _T("");
	m_GetAgentGenConfig             = _T("");
	m_GetAllDevice                  = _T("");
	m_GetDbParams                   = _T("");
	m_GetLgConfig                   = _T("");
	m_GroupMonitoring               = _T("");
	m_GroupState                    = _T("");
	m_GuiMsg                        = _T("");
	m_MonitoringDateRegistration    = _T("");
	m_PerfConfig                    = _T("");
	m_PerfMsg                       = _T("");
	m_Registration_Key              = _T("");
	m_SetAgentGenConfig             = _T("");
	m_SetAllDevices                 = _T("");
	m_SetDbParams                   = _T("");
	m_SetLgConfig                   = _T("");
	m_StatusMsg                     = _T("");
	m_TdmfCommand                   = _T("");
	m_TdmfCommonGuiRegistration     = _T("");
	//}}AFX_DATA_INIT
}


void CMessageByTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMessageByTypeDlg)
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
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMessageByTypeDlg, CDialog)
	//{{AFX_MSG_MAP(CMessageByTypeDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMessageByTypeDlg message handlers

void CMessageByTypeDlg::UpdateTheMsgFromCollectorStats(TDMFOBJECTSLib::ICollectorStatsPtr pICollectorStats)
{

    if(m_pSystem != NULL)
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

}

