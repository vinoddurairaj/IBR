// ComCollectorStats.cpp : Implementation of CComCollectorStats
#include "stdafx.h"
#include "TDMFObjects.h"
#include "ComCollectorStats.h"

/////////////////////////////////////////////////////////////////////////////
// CComCollectorStats


STDMETHODIMP CComCollectorStats::GetDBMsgPerHour(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->NbPDBMsgPerHr;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComCollectorStats::GetDBMsgPerMin(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->NbPDBMsgPerMn;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComCollectorStats::GetDBMsgPending(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->NbPDBMsg;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetThrdPerHour(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->NbThrdRngHr;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetThrdPerMin(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->NbThrdRngPerMn;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetThrdPending(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->NbThrdRng;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetAgentsAlive(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->NbAgentsAlive;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetAliveMsgPerMin(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->NbAliveMsgPerMn;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetAliveMsgPerHour(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->NbAliveMsgPerHr;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetTimeCollector(time_t *pTime)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pTime != NULL)
	{
		*pTime = m_pTdmfCollectorState->CollectorTime;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_GetLGConfig(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GET_LG_CONFIG;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_SetLGConfig(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_SET_LG_CONFIG;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_AgentInfoRequest(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_INFO_REQUEST;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_AgentInfo(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_INFO;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_RegistrationKey(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_REGISTRATION_KEY;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_TDMFCmd(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_TDMF_CMD;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_SetAgenGenConfig(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	
    HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_SET_AGENT_GEN_CONFIG;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_GetAgentGenConfig(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GET_AGENT_GEN_CONFIG;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_SetAllDevices(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_SET_ALL_DEVICES;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_GetAllDevices(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GET_ALL_DEVICES;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_AlertData(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_ALERT_DATA;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_StatusMsg(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_STATUS_MSG;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_PerfMsg(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_PERF_MSG;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_PerfCfgMsg(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_PERF_CFG_MSG;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_MonitoringDataReg(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_MONITORING_DATA_REGISTRATION;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_AgentAliveSocket(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_ALIVE_SOCKET;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_GroupState(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GROUP_STATE;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_GroupMonitoring(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GROUP_MONITORING;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_TDMFCommonGuiReg(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_TDMFCOMMONGUI_REGISTRATION;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_SetDBParams(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_SET_DB_PARAMS;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_GetDBParams(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GET_DB_PARAMS;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_AgentState(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_AGENT_STATE;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_GuiMsg(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_MMP_MNGT_GUI_MSG;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComCollectorStats::GetMsg_Default(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pTdmfCollectorState->TdmfMessagesStates.Nb_default;
		hr = S_OK;
	}

	return S_OK;
}
