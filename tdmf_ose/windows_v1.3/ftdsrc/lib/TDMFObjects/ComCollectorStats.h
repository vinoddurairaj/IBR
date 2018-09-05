// ComCollectorStats.h : Declaration of the CComCollectorStats

#ifndef __COMCOLLECTORSTATS_H_
#define __COMCOLLECTORSTATS_H_

#include "resource.h"       // main symbols
#include "libmngtdef.h"

/////////////////////////////////////////////////////////////////////////////
// CComCollectorStats
class ATL_NO_VTABLE CComCollectorStats : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CComCollectorStats, &CLSID_ComCollectorStats>,
	public ICollectorStats
{

public:
	mmp_TdmfCollectorState* m_pTdmfCollectorState;

public:
	CComCollectorStats() : m_pTdmfCollectorState(NULL) 
    {
      
    }
	

DECLARE_REGISTRY_RESOURCEID(IDR_COMCOLLECTORSTATS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComCollectorStats)
	COM_INTERFACE_ENTRY(ICollectorStats)
END_COM_MAP()

// ICollectorStats
public:
	STDMETHOD(GetMsg_Default)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_GuiMsg)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_AgentState)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_GetDBParams)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_SetDBParams)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_TDMFCommonGuiReg)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_GroupMonitoring)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_GroupState)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_AgentAliveSocket)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_MonitoringDataReg)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_PerfCfgMsg)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_PerfMsg)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_StatusMsg)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_AlertData)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_GetAllDevices)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_SetAllDevices)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_GetAgentGenConfig)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_SetAgenGenConfig)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_TDMFCmd)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_RegistrationKey)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_AgentInfo)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_AgentInfoRequest)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_SetLGConfig)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetMsg_GetLGConfig)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetTimeCollector)(/*[out,retval]*/ time_t* pTime);
	STDMETHOD(GetAliveMsgPerHour)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetAliveMsgPerMin)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetAgentsAlive)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetThrdPending)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetThrdPerMin)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetThrdPerHour)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetDBMsgPending)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetDBMsgPerMin)(/*[out,retval]*/ unsigned long* pVal);
	STDMETHOD(GetDBMsgPerHour)(/*[out, retval]*/ unsigned long * pVal);


};

#endif //__COMCOLLECTORSTATS_H_
