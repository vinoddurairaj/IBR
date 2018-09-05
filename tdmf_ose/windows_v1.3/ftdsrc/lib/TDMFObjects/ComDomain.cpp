// Domain.cpp : Implementation of CComDomain
#include "stdafx.h"
#include "TDMFObjects.h"
#include "ComDomain.h"
#include "ComServer.h"
#include "ComSystem.h"
#include "mmp_API.h"


/////////////////////////////////////////////////////////////////////////////
// CComDomain

STDMETHODIMP CComDomain::get_Name(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pDomain->m_strName.c_str()).Copy();
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComDomain::put_Name(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pDomain->m_strName = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComDomain::get_ServerCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pDomain->m_listServer.size();
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComDomain::GetServer(long nIndex, IServer **ppServer)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	*ppServer = NULL;

	if ((nIndex < 0) || (ppServer == NULL) || ((UINT)nIndex >= m_pDomain->m_listServer.size()))
	{
		hr = E_INVALIDARG;
	}
	else
	{
		std::list<CServer>::iterator it;
		long nCount;
		for (it = m_pDomain->m_listServer.begin(), nCount = 0;
			 (nCount < nIndex) && (it != m_pDomain->m_listServer.end()); it++, nCount++);
		if (nCount == nIndex)
		{
			CComObject<CComServer>* pComServer = NULL;
			hr = CComObject<CComServer>::CreateInstance(&pComServer);
			if (SUCCEEDED(hr))
			{
				CServer& Server = *it;
				pComServer->m_pServer = &Server;
				pComServer->AddRef();
				hr = pComServer->QueryInterface(ppServer);
				pComServer->Release();
			}
		}
		else
		{
			hr = E_INVALIDARG;
		}
	}

	return hr;
}

STDMETHODIMP CComDomain::CreateNewServer(IServer **ppNewServer)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (ppNewServer == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		// Create a new server
		CServer& Server = m_pDomain->AddNewServer();

		CComObject<CComServer>* pComServer = NULL;
		hr = CComObject<CComServer>::CreateInstance(&pComServer);
		if (SUCCEEDED(hr))
		{
			pComServer->m_pServer = &Server;
			pComServer->AddRef();
			hr = pComServer->QueryInterface(ppNewServer);
			pComServer->Release();
		}
	}

	return hr;
}

STDMETHODIMP CComDomain::get_Parent(ISystem **ppVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;
	
	if (ppVal != NULL)
	{
		*ppVal = NULL;

		CComObject<CComSystem>* pComSystem = NULL;
		hr = CComObject<CComSystem>::CreateInstance(&pComSystem);
		if (SUCCEEDED(hr))
		{
			pComSystem->m_pSystem = m_pDomain->m_pParent;
			pComSystem->AddRef();
			hr = pComSystem->QueryInterface(ppVal);
			pComSystem->Release();
		}
	}

	return hr;
}

STDMETHODIMP CComDomain::RemoveServer(IServer *pServer, long *pRetVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pRetVal = MMPAPI_Error::OK;

	long nKey;
	if (SUCCEEDED(pServer->get_Key(&nKey)))
	{
		std::list<CServer>::iterator it = m_pDomain->m_listServer.begin();
		while(it != m_pDomain->m_listServer.end())
		{
			if (it->GetKey() == nKey)
			{
				// Remove it from DB
				*pRetVal = it->RemoveFromDB();

				if (*pRetVal == MMPAPI_Error::OK)
				{
					m_pDomain->m_listServer.erase(it);
				}

				m_pDomain->UpdateStatus();

				break;
			}
			it++;
		}
	}

	return S_OK;
}

STDMETHODIMP CComDomain::get_Description(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pDomain->m_strDescription.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComDomain::put_Description(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pDomain->m_strDescription = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComDomain::get_State(ElementState *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = (ElementState)m_pDomain->m_eState;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComDomain::get_Key(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pDomain->GetKey();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComDomain::IsEqual(IDomain *pDomain, BOOL *pbRetVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	long nKey;
	HRESULT hr = pDomain->get_Key(&nKey);
	if (SUCCEEDED(hr))
	{
		*pbRetVal = (m_pDomain->GetKey() == nKey);
	}

	return hr;
}

STDMETHODIMP CComDomain::SaveToDB(TdmfErrorCode* pRetVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pRetVal != NULL)
	{
		*pRetVal = (TdmfErrorCode)m_pDomain->SaveToDB();
		hr = S_OK;
	}

	return hr;
}
