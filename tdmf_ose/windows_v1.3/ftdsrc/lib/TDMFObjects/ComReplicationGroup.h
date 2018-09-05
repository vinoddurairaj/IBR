// ComReplicationGroup.h : Declaration of the CComReplicationGroup

#ifndef __COMREPLICATIONGROUP_H_
#define __COMREPLICATIONGROUP_H_

#include "resource.h"       // main symbols

#include "ReplicationGroup.h"


/////////////////////////////////////////////////////////////////////////////
// CComReplicationGroup

class ATL_NO_VTABLE CComReplicationGroup : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CComReplicationGroup, &CLSID_ReplicationGroup>,
	public IReplicationGroup
{
protected:
	CComPtr<IUnknown> m_pUnkMarshaler;

public:
	CComReplicationGroup() : m_pRG(NULL), m_pUnkMarshaler(NULL) {}

	CReplicationGroup* m_pRG;

DECLARE_REGISTRY_RESOURCEID(IDR_REPLICATIONGROUP)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComReplicationGroup)
	COM_INTERFACE_ENTRY(IReplicationGroup)
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

// IReplicationGroup
public:
	STDMETHOD(get_SymmetricJournal)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SymmetricJournal)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SymmetricPStore)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SymmetricPStore)(/*[in]*/ BSTR newVal);
	STDMETHOD(Failover)(/*[out]*/ long* Warning, /*[out, retval]*/ long* Result);
	STDMETHOD(get_SymmetricMode)(/*[out, retval]*/ GroupMode *pVal);
	STDMETHOD(get_SymmetricConnectionStatus)(/*[out, retval]*/ ConnectionStatus *pVal);
	STDMETHOD(get_FailoverInitialState)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_FailoverInitialState)(/*[in]*/ long newVal);
	STDMETHOD(get_SymmetricNormallyStarted)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_SymmetricNormallyStarted)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_SymmetricGroupNumber)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_SymmetricGroupNumber)(/*[in]*/ long newVal);
	STDMETHOD(RemoveSymmetricGroup)();
	STDMETHOD(get_Symmetric)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Symmetric)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_ForcePMDRestart)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_ForcePMDRestart)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_TargetEditedIP)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_TargetEditedIP)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_PrimaryEditedIP)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_PrimaryEditedIP)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Throttles)(/*[out, retval]*/ BSTR* pVal);
	STDMETHOD(get_IsLockCmds)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(UnlockCmds)();
	STDMETHOD(LockCmds)();
	STDMETHOD(SetTargetEditedAddressUsed)(/*[in]*/ BOOL bVal);
	STDMETHOD(SetPrimaryEditedAddressUsed)(/*[in]*/ BOOL bVal);
	STDMETHOD(get_PrimaryName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_PrimaryIPAdress)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_PrimaryIPAdress)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_TargetEditedIPAdress)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_TargetEditedIPAdress)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_PrimaryEditedIPAdress)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_PrimaryEditedIPAdress)(/*[in]*/ BSTR newVal);
	STDMETHOD(SetPrimaryDHCPAddressUsed)(BOOL bVal);
	STDMETHOD(IsTargetEditedIPUsed)(/*[out, retval]*/ BOOL * pVal);
	STDMETHOD(IsPrimaryEditedIPUsed)(/*[out, retval]*/ BOOL* pVal);
	STDMETHOD(IsPrimaryDHCPAdressUsed)(/*[out, retval]*/BOOL *pVal);
	STDMETHOD(SetTargetDHCPAddressUsed)(/*[in]*/ BOOL bVal);
	STDMETHOD(IsTargetDHCPAdressUsed)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_JournalLess)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_JournalLess)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_StatInterval)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_StatInterval)(/*[in]*/ long newVal);
	STDMETHOD(get_NetMaxKbps)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_NetMaxKbps)(/*[in]*/ long newVal);
	STDMETHOD(get_NetThreshold)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_NetThreshold)(/*[in]*/ BOOL newVal);
	STDMETHOD(SetTunables)(/*[out, retval]*/ TdmfErrorCode* RetVal);
	STDMETHOD(SaveTunables)(/*[out, retval]*/ TdmfErrorCode* RetVal);
	STDMETHOD(IsEventAt)(/*[in]*/ long nIndex, /*[out, retval]*/ BOOL* pbLoaded);
	STDMETHOD(GetEventCount)(/*[out, retval]*/ long* pnRow);
	STDMETHOD(GetEventAt)(/*[in]*/ long nIndex, /*[out, retval]*/ IEvent** ppEvent);
	STDMETHOD(get_PctBAB)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_BABEntries)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_EffectiveNet)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ActualNet)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_WriteKbps)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ReadKbps)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(CreateAssociatedTargetGroup)(/*[out, retval]*/ IReplicationGroup** RGTarget);
	STDMETHOD(GetTargetGroup)(/*[out, retval]*/ IReplicationGroup** pVal);
	STDMETHOD(get_IsSource)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(GetEventFirst)(/*[out, retval]*/ IEvent** pEvent);
	STDMETHOD(get_CmdHistory)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_LastCmdOutput)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(GetUniqueGroupNumber)(/*[out, retval]*/ long* NewGroupNumber);
	STDMETHOD(get_JournalSize)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_DiskTotalSize)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_DiskFreeSize)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(LaunchCommand)(/*[in]*/TdmfCommand eCmd, /*[in]*/ BSTR pszOptions, /*[in]*/ BSTR Log, /*[in]*/ BOOL Symmetric, /*[out]*/ BSTR* Message, /*[out, retval]*/ long *pRetCode	);
	STDMETHOD(SaveToDB)(/*[in]*/ long OldGroupNumber, /*[in]*/ long nOldTgtHostId, /*[out]*/ BSTR* WarningsMsg, /*[out, retval]*/ TdmfErrorCode* pRetVal);
	STDMETHOD(get_PctDone)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_ConnectionStatus)(/*[out, retval]*/ ConnectionStatus *pVal);
	STDMETHOD(put_ConnectionStatus)(/*[in]*/ ConnectionStatus newVal);
	STDMETHOD(IsEqual)(/*[in]*/ IReplicationGroup* pRG, /*[out, retval]*/ BOOL* pbRetVal);
	STDMETHOD(get_Key)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_TargetServer)(/*[out, retval]*/ IServer **ppTargetServer);
	STDMETHOD(putref_TargetServer)(/*[in]*/ IServer* newVal);
	STDMETHOD(CreateNewReplicationPair)(/*[out, retval]*/ IReplicationPair **pReplicationPair);
	STDMETHOD(RemoveReplicationPair)(/*[in]*/ IReplicationPair *pReplicationPair);
	STDMETHOD(GetReplicationPair)(/*[in]*/ long Index, /*[out, retval]*/ IReplicationPair** pReplicationPair);
	STDMETHOD(get_ReplicationPairCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_ChunkDelay)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_ChunkDelay)(/*[in]*/ long newVal);
	STDMETHOD(get_ChunkSize)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_ChunkSize)(/*[in]*/ long newVal);
	STDMETHOD(get_EnableCompression)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_EnableCompression)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_Sync)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Sync)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_SyncTimeout)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_SyncTimeout)(/*[in]*/ long newVal);
	STDMETHOD(get_SyncDepth)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_SyncDepth)(/*[in]*/ long newVal);
	STDMETHOD(get_RefreshTimeoutInterval)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_RefreshTimeoutInterval)(/*[in]*/ long newVal);
	STDMETHOD(get_RefreshNeverTimeout)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_RefreshNeverTimeout)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_MaxFileStatSize)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_MaxFileStatSize)(/*[in]*/ long newVal);
	STDMETHOD(get_PStoreDirectory)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_PStoreDirectory)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_JournalDirectory)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_JournalDirectory)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_TargetIPAddress)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_GroupNumber)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_GroupNumber)(/*[in]*/ long newVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_StateTimeStamp)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Chaining)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Chaining)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_Mode)(/*[out, retval]*/ GroupMode *pVal);
	STDMETHOD(put_Mode)(/*[in]*/ GroupMode newVal);
	STDMETHOD(get_State)(/*[out, retval]*/ ElementState *pVal);
	STDMETHOD(get_TargetName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Parent)(/*[out, retval]*/ IServer** ppServer);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
};

#endif //__COMREPLICATIONGROUP_H_
