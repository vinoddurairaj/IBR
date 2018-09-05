// ComScriptServerFile.h : Declaration of the CComScriptServerFile

#ifndef __SCRIPTSERVERFILE_H_
#define __SCRIPTSERVERFILE_H_

#include "resource.h"       // main symbols
#include "ScriptServer.h"

/////////////////////////////////////////////////////////////////////////////
// CComScriptServerFile
class ATL_NO_VTABLE CComScriptServerFile : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CComScriptServerFile, &CLSID_ScriptServerFile>,
	public IScriptServerFile
{
public:
	CComScriptServerFile(): m_pScriptServer(NULL)
	{
	}

 	CScriptServer* m_pScriptServer;

DECLARE_REGISTRY_RESOURCEID(IDR_SCRIPTSERVERFILE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComScriptServerFile)
	COM_INTERFACE_ENTRY(IScriptServerFile)
END_COM_MAP()

// IScriptServerFile
public:
	STDMETHOD(get_New)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_New)(/*[in]*/ BOOL newVal);
	STDMETHOD(IsNew)(/*[out, retval ]*/ BOOL* pVal);
	STDMETHOD(SendScriptServerFileToAgent)(/*[out,retval]*/ TdmfErrorCode* pRetVal);
	STDMETHOD(SaveToDB)(/*[out,retval]*/ TdmfErrorCode* pRetVal);
	STDMETHOD(ParentServer)(/*[out,retval]*/IServer** ppServer);
	STDMETHOD(get_ModificationDate)(/*[out, retval]*/ BSTR* pVal);
	STDMETHOD(put_ModificationDate)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_CreationDate)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_CreationDate)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Content)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Content)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Type)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Type)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Extension)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Extension)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FileName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FileName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ParentServerID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_ParentServerID)(/*[in]*/ long newVal);
	STDMETHOD(get_ScriptServerID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_ScriptServerID)(/*[in]*/ long newVal);
};

#endif //__SCRIPTSERVERFILE_H_
