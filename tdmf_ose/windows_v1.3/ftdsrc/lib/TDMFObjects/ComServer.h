// ComServer.h : Declaration of the CComServer

#ifndef __COMSERVER_H_
#define __COMSERVER_H_

#include "resource.h"       // main symbols
#include "Server.h"         // CServer

/////////////////////////////////////////////////////////////////////////////
// CComServer

class ATL_NO_VTABLE CComServer : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CComServer, &CLSID_Server>,
	public IServer
{
protected:
	CComPtr<IUnknown> m_pUnkMarshaler;	

public:
	CComServer() : m_pServer(NULL), m_bEnablePerformanceNotifications(FALSE), m_pUnkMarshaler(NULL) {}

	CServer* m_pServer;
	BOOL     m_bEnablePerformanceNotifications;

DECLARE_REGISTRY_RESOURCEID(IDR_SERVER)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComServer)
	COM_INTERFACE_ENTRY(IServer)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

// IServer
public:
	STDMETHOD(get_ReplicationPort)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_ReplicationPort)(/*[in]*/ long newVal);
	STDMETHOD(get_NbrCPU)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_NbrCPU)(/*[in]*/ long newVal);
	STDMETHOD(get_IsLockCmds)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(UnlockCmds)();
	STDMETHOD(LockCmds)();
	STDMETHOD(ImportOneScriptServerFile)(/*[in]*/ BSTR strFilename);
	STDMETHOD(ImportAllScriptServerFiles)(/*[int]*/ BOOL bOverwriteExistingFile,BSTR strExtension);
	STDMETHOD(RemoveScriptServerFile)(long nScriptServerID, long *pRetVal);
	STDMETHOD(GetScriptServerFile)(/*[in]*/ long Index,/*[out,retval]*/ IScriptServerFile** ppScriptServerFile);
	STDMETHOD(GetScriptServerFileCount)(/*[out,retval]*/long *pVal);
	STDMETHOD(CreateScriptServerFile)(/*[out,retval]*/IScriptServerFile**ppScriptServerFile);
	STDMETHOD(Import)(/*[out]*/ BSTR* Message, /*[out, retval]*/ long* ErrCode);
	STDMETHOD(get_JournalDiskFreeSize)(/*[in]*/ long Index, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_JournalDiskSize)(/*[in]*/ long Index, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_JournalDrive)(/*[in]*/ long Index, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_JournalDriveCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_JournalSize)(/*[in]*/ long Index, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(IsEventAt)(/*[in]*/ long nIndex, /*[out, retval]*/ BOOL* pbLoaded);
	STDMETHOD(GetEventCount)(/*[out, retval]*/ long* pnRow);
	STDMETHOD(GetEventAt)(/*[in]*/ long nIndex, /*[out, retval]*/ IEvent** ppEvent);
	STDMETHOD(get_CmdHistory)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_LastCmdOutput)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_PStoreSize)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(GetPerformanceValues)(/*[in]*/ BSTR Stats, /*[in]*/ DATE DateBegin, /*[in]*/ DATE DateEnd, /*[out, retval]*/ BSTR* RetVal);
	STDMETHOD(get_BABSizeAllocated)(/*[out, retval]*/ long *pVal);
	STDMETHOD(GetDeviceList)(/*[out, retval]*/ IDeviceList** pDeviceList);
	STDMETHOD(GetDeviceLists)(/*[in]*/ IServer* pServerTarget, /*[out]*/ IDeviceList** pDeviceList, /*[out]*/ IDeviceList** pDeviceListTarget);
	STDMETHOD(SaveToDB)(/*[out, retval]*/ TdmfErrorCode* pRetVal);
	STDMETHOD(GetEventFirst)(/*[out, retval]*/ IEvent** pEvent);
	STDMETHOD(get_TargetReplicationPairCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_ReplicationPairCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_JournalDirectory)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_JournalDirectory)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_TargetReplicationGroupCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_BABEntries)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_PctBAB)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_PerformanceNotifications)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_PerformanceNotifications)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_Connected)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(IsEqual)(/*[in]*/ IServer* pServer, /*[out, retval]*/ BOOL* pbRetVal);
	STDMETHOD(get_RegKey)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RegKey)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RAMSize)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_HostID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Key)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Port)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Port)(/*[in]*/ long newVal);
	STDMETHOD(get_TCPWndSize)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_TCPWndSize)(/*[in]*/ long newVal);
	STDMETHOD(get_PStoreDirectory)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_PStoreDirectory)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_BABSize)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_BABSize)(/*[in]*/ long newVal);
	STDMETHOD(get_State)(/*[out, retval]*/ ElementState *pVal);
	STDMETHOD(get_KeyExpirationDate)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_AgentVersion)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_OSVersion)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_OSType)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_IPAddressCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_IPAddress)(/*[in]*/ long Index, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(LaunchCommand)(TdmfCommand eCmd, BSTR pszOptions, BSTR Log, BSTR* Message, long *pRetCode);
	STDMETHOD(RemoveReplicationGroup)(/*[in]*/ IReplicationGroup* pReplicationGroup, /*[out, retval]*/ long *pRetVal);
	STDMETHOD(MoveTo)(/*[in]*/ IDomain* pDomain, /*[out, retval]*/ long *pVal);
	STDMETHOD(CreateNewReplicationGroup)(/*[out, retval]*/ IReplicationGroup** ppReplicationGroup);
	STDMETHOD(get_Parent)(/*[out, retval]*/ IDomain** ppDomain);
	STDMETHOD(GetReplicationGroup)(/*[in]*/ long Index, /*[out, retval]*/ IReplicationGroup** ppReplicationGroup);
	STDMETHOD(get_ReplicationGroupCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
};

#endif //__COMSERVER_H_
