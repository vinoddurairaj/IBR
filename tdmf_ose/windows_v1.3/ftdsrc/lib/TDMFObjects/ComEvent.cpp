// ComEvent.cpp : Implementation of CComEvent
#include "stdafx.h"
#include "TDMFObjects.h"
#include "ComEvent.h"

/////////////////////////////////////////////////////////////////////////////
// CEvent


STDMETHODIMP CComEvent::get_Date(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pEvent->m_strDate.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComEvent::get_Time(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pEvent->m_strTime.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComEvent::get_Source(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pEvent->m_strSource.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComEvent::get_Description(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pEvent->m_strDescription.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComEvent::get_Severity(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pEvent->m_nSeverity;
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComEvent::get_Type(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pEvent->m_strType.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComEvent::get_GroupID(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pEvent->m_nGroupID;
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComEvent::get_PairID(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pEvent->m_nPairID;
		hr =S_OK;
	}

	return hr;
}
