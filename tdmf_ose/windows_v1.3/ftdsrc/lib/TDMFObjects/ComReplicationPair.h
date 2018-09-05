// ComReplicationPair.h : Declaration of the CComReplicationPair

#ifndef __COMREPLICATIONPAIR_H_
#define __COMREPLICATIONPAIR_H_

#include "resource.h"       // main symbols
#include "ReplicationPair.h"

/////////////////////////////////////////////////////////////////////////////
// CComReplicationPair

class ATL_NO_VTABLE CComReplicationPair : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CComReplicationPair, &CLSID_ReplicationPair>,
	public IReplicationPair
{
public:
	CComReplicationPair() : m_pRP(NULL) {}

	CReplicationPair* m_pRP;

DECLARE_REGISTRY_RESOURCEID(IDR_REPLICATIONPAIR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComReplicationPair)
	COM_INTERFACE_ENTRY(IReplicationPair)
END_COM_MAP()

// IReplicationPair
public:
	STDMETHOD(CopyDevice)(/*[in]*/ BOOL bSourceServer, /*[in]*/ IDevice* pDevice );
	STDMETHOD(get_ObjectState)(/*[out, retval]*/ ReplicationPairObjectState *pVal);
	STDMETHOD(put_ObjectState)(/*[in]*/ ReplicationPairObjectState newVal);
	STDMETHOD(get_TgtName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_TgtName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SrcName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SrcName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FileSystem)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FileSystem)(/*[in]*/ BSTR newVal);
	STDMETHOD(IsEqual)(/*[in]*/ IReplicationPair* pRP, /*[out, retval]*/ BOOL* pbRetVal);
	STDMETHOD(get_Key)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_TgtSize)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_TgtSize)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SrcSize)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SrcSize)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Parent)(/*[out, retval]*/ IReplicationGroup* *pVal);
	STDMETHOD(get_PairNumber)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_PairNumber)(/*[in]*/ long newVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
};

#endif //__COMREPLICATIONPAIR_H_
