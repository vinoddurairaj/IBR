// ComSystem.h : Declaration of the CComSystem

#ifndef __COMSYSTEM_H_
#define __COMSYSTEM_H_

#include "resource.h"       // main symbols
#include "System.h"


/////////////////////////////////////////////////////////////////////////////
// CComSystem

class ATL_NO_VTABLE CComSystem : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CComSystem, &CLSID_System>,
	public ISystem
{
protected:
	CComPtr<IUnknown> m_pUnkMarshaler;

public:
	CComSystem() : m_pSystem(NULL), m_pUnkMarshaler(NULL) {}

	CSystem* m_pSystem;

DECLARE_REGISTRY_RESOURCEID(IDR_SYSTEM)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComSystem)
	COM_INTERFACE_ENTRY(ISystem)
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

// ISystem
public:
	STDMETHOD(get_IsLockCmds)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(SetTimeOut)(/*[in]*/ int nIndex, /*[in]*/ int nValue);
	STDMETHOD(GetTimeOut)(/*[in]*/ int nIndex, /*[out, retval]*/ long* pvalue);
	STDMETHOD(GetTimeOutFromDB)();
	STDMETHOD(SetTimeOutToDB)();
	STDMETHOD(GetUserCount)( /*[out]*/ long* pCount);
	STDMETHOD(SetDeleteDelay)(/*[in]*/ long value);
	STDMETHOD(GetDeleteDelay)(/*[out, retval]*/ long* pValue);
	STDMETHOD(GetFirstKeyLogMsg)(/*[out]*/ BSTR* Date, /*[out]*/ BSTR* Hostname, /*[out]*/ long* HostId, /*[out]*/ BSTR* RegKey, /*[out]*/ BSTR* ExpDate);
 	STDMETHOD(GetNextKeyLogMsg)(/*[out]*/ BSTR* Date, /*[out]*/ BSTR* Hostname, /*[out]*/ long* HostId, /*[out]*/ BSTR* RegKey, /*[out]*/ BSTR* ExpDate);
	STDMETHOD(DeleteAllLogMsg)(/*[out, retval]*/ BOOL* bRet);
	STDMETHOD(GetFirstLogMsg)(/*[out]*/ BSTR* Date, /*[out]*/ BSTR* Source, /*[out]*/ BSTR* User, /*[out]*/ BSTR* Msg);
 	STDMETHOD(GetNextLogMsg)(/*[out]*/ BSTR* Date, /*[out]*/ BSTR* Source, /*[out]*/ BSTR* User, /*[out]*/ BSTR* Msg);
	STDMETHOD(get_LogUsersActions)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_LogUsersActions)(/*[in]*/ BOOL newVal);
	STDMETHOD(SetCollectorStats)(/*[in]*/ ICollectorStats * pICollectorStats);
	STDMETHOD(GetCollectorStats)(/*[out, retval]*/ ICollectorStats** pICollectorStats);
	STDMETHOD(SendTextMessage)(/*[in]*/ BSTR Msg);
	STDMETHOD(AlreadyExistDomain)(/*[in]*/ BSTR Name,/*[in]*/ long lKey, /*[out, retval]*/ BOOL *pRet);
    STDMETHOD(GetFirstUser)(/*[out]*/ BSTR* Location,/*[out]*/ BSTR* UserName, /*[out]*/ BSTR* Type, /*[out]*/ BSTR* App);
 	STDMETHOD(GetNextUser)(/*[out]*/ BSTR* Location,/*[out]*/ BSTR* UserName, /*[out]*/ BSTR* Type, /*[out]*/ BSTR* App);
	STDMETHOD(get_UserRole)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(IsEventAt)(/*[in]*/ long nIndex, /*[out, retval]*/ BOOL* pbLoaded);
	STDMETHOD(GetEventCount)(/*[out, retval]*/ long* pnRow);
	STDMETHOD(GetEventAt)(/*[int]*/ long nIndex, /*[out, retval]*/ IEvent** pEvent);
	STDMETHOD(get_Password)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Password)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_UserID)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_UserID)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_TraceLevel)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_TraceLevel)(/*[in]*/ short newVal);
	STDMETHOD(GetCommandMenuString)(/*[in]*/ BSTR Key, /*[in]*/ long Id, /*[out, retval]*/ BSTR* Val);
	STDMETHOD(IsCommandMenuEntryEnabled)(/*[in]*/ BSTR key, /*[in]*/ long Index, /*[in]*/ long Mode, /*[in]*/ long ConnectionStatus, /*[in]*/ long Platform, /*[out, retval]*/ BOOL* Enabled);
	STDMETHOD(GetCommandMenuEntryId)(/*[in]*/ BSTR Key, /*[in]*/ long Index, /*[out, retval]*/ long* Id);
	STDMETHOD(GetCommandMenuEntryName)(/*[in]*/ BSTR Key, /*[in]*/ long Index, /*[out, retval]*/ BSTR* Name);
	STDMETHOD(GetNbCommandMenuEntries)(/*[in]*/ BSTR Key, /*[out, retval]*/ long* Nb);
	STDMETHOD(SetDeleteRecords)(/*[in]*/ TdmfDBTable Table, /*[in]*/ long days, /*[in]*/ long NbRecords);
	STDMETHOD(GetDeleteRecords)(/*[in]*/ TdmfDBTable Table, /*[out]*/ long *pDays, /*[out]*/ long *pNbRecords);
	STDMETHOD(DeleteTableRecords)(/*[in]*/ TdmfDBTable Table);
	STDMETHOD(GetTableSize)(/*[in]*/ TdmfDBTable Table, /*[out]*/ long* pCount, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(Open)(/*[in]*/ BSTR bstrDSN, /*[out, retval]*/ long* pErr);
	STDMETHOD(RequestOwnership)(/*[in]*/ BOOL bRequest);
	STDMETHOD(GetLastCommandOutput)(/*[out, retval]*/ BSTR* pCmdOutput);
	STDMETHOD(GetEventFirst)(/*[out, retval]*/ IEvent ** pEvent);
	STDMETHOD(GetDescription)(/*[out]*/ BSTR *pDatabase, /*[out]*/ BSTR *pVersion, /*[out]*/ BSTR *pIP, /*[out]*/ BSTR *pPort, /*[out]*/ BSTR *pHostId);
	STDMETHOD(Uninitialize)();
	STDMETHOD(Init)(/*[in]*/ long HWND, /*[out, retval]*/ long* pErr);
	STDMETHOD(Refresh)(/*[out, retval]*/ long* pErr);
	STDMETHOD(RemoveDomain)(/*[in]*/ IDomain* pDomain, /*[out, retval]*/ long *pRetVal);
	STDMETHOD(CreateNewDomain)(/*[out, retval]*/ IDomain** ppNewDomain);
	STDMETHOD(GetDomain)(/*[in]*/ long Index, /*[out, retval]*/ IDomain** ppDomain);
	STDMETHOD(get_DomainCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Name)(/*[out, retval]*/ BSTR Val);
};

#endif //__COMSYSTEM_H_
