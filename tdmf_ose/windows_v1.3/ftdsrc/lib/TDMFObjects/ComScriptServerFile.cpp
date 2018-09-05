// ComScriptServerFile.cpp : Implementation of CComScriptServerFile
#include "stdafx.h"
#include "TDMFObjects.h"
#include "ComScriptServerFile.h"
#include "ComServer.h"
/////////////////////////////////////////////////////////////////////////////
// CComScriptServerFile


STDMETHODIMP CComScriptServerFile::get_ScriptServerID(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = (long)m_pScriptServer->m_iDbScriptSrvId;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::put_ScriptServerID(long newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pScriptServer->m_iDbScriptSrvId = newVal;

	return S_OK;
}

STDMETHODIMP CComScriptServerFile::get_ParentServerID(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = (long)m_pScriptServer->m_iDbSrvId;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::put_ParentServerID(long newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pScriptServer->m_iDbSrvId = newVal;

	return S_OK;
}

STDMETHODIMP CComScriptServerFile::get_FileName(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pScriptServer->m_strFileName.c_str()).Copy();
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::put_FileName(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pScriptServer->m_strFileName = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComScriptServerFile::get_Extension(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pScriptServer->m_strExtension.c_str()).Copy();
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::put_Extension(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pScriptServer->m_strExtension = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComScriptServerFile::get_Type(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pScriptServer->m_strType.c_str()).Copy();
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::put_Type(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pScriptServer->m_strType = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComScriptServerFile::get_Content(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pScriptServer->m_strContent.c_str()).Copy();
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::put_Content(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pScriptServer->m_strContent = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComScriptServerFile::get_CreationDate(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pScriptServer->m_strCreationDate.c_str()).Copy();
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::put_CreationDate(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pScriptServer->m_strCreationDate = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComScriptServerFile::get_ModificationDate(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pScriptServer->m_strModificationDate.c_str()).Copy();
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::put_ModificationDate(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pScriptServer->m_strModificationDate = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComScriptServerFile::ParentServer(IServer **ppServer)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (ppServer != NULL)
	{
		CComObject<CComServer>* pComServer = NULL;
		hr = CComObject<CComServer>::CreateInstance(&pComServer);
		if (SUCCEEDED(hr))
		{
			pComServer->m_pServer = m_pScriptServer->m_pServer;
			pComServer->AddRef();
			hr = pComServer->QueryInterface(ppServer);
			pComServer->Release();
		}
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::SaveToDB(TdmfErrorCode *pRetVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pRetVal != NULL)
	{
		*pRetVal = (TdmfErrorCode)m_pScriptServer->SaveToDB();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::SendScriptServerFileToAgent(TdmfErrorCode *pRetVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pRetVal != NULL)
	{
		*pRetVal = (TdmfErrorCode)m_pScriptServer->SendToAgent();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::IsNew(BOOL *pVal)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pScriptServer->m_bNew;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::get_New(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pScriptServer->m_bNew;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComScriptServerFile::put_New(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pScriptServer->m_bNew = newVal;

	return S_OK;
}
