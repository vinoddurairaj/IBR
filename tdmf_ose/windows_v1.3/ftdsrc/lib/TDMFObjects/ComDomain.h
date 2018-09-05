// ComDomain.h : Declaration of the CComDomain

#ifndef __COMDOMAIN_H_
#define __COMDOMAIN_H_

#include "resource.h"       // main symbols
#include "Domain.h"         // CDomain

/////////////////////////////////////////////////////////////////////////////
// CComDomain
class ATL_NO_VTABLE CComDomain : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CComDomain, &CLSID_Domain>,
	public IDomain
{
protected:
	CComPtr<IUnknown> m_pUnkMarshaler;	

public:
	CComDomain() : m_pDomain(NULL), m_pUnkMarshaler(NULL) {}

	CDomain* m_pDomain;

DECLARE_REGISTRY_RESOURCEID(IDR_DOMAIN)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComDomain)
	COM_INTERFACE_ENTRY(IDomain)
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

// IDomain
public:
	STDMETHOD(SaveToDB)(/*[out, retval]*/ TdmfErrorCode* pRetVal);
	STDMETHOD(IsEqual)(/*[in]*/ IDomain* pDomain, /*[out, retval]*/ BOOL* pbRetVal);
	STDMETHOD(get_Key)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_State)(/*[out, retval]*/ ElementState *pVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);
	STDMETHOD(RemoveServer)(/*[in]*/ IServer* pServer, /*[out, retval]*/ long* pRetVal);
	STDMETHOD(get_Parent)(/*[out, retval]*/ ISystem* *pVal);
	STDMETHOD(CreateNewServer)(/*[out, retval]*/ IServer** ppServer);
	STDMETHOD(GetServer)(/*[in]*/ long Index, /*[out, retval]*/ IServer** ppServer);
	STDMETHOD(get_ServerCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Name)(/*[in]*/ BSTR newVal);
};

#endif //__COMDOMAIN_H_
