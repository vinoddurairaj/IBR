// ComDevice.cpp : Implementation of CComDevice
#include "stdafx.h"
#include "TDMFObjects.h"
#include "ComDevice.h"
#include ".\comdevice.h"


/////////////////////////////////////////////////////////////////////////////
// CComDevice

STDMETHODIMP CComDevice::get_Name(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_strPath.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComDevice::get_Size(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_strLength.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComDevice::get_FileSystem(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_strFileSystem.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComDevice::get_DriveId(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_strDriveId.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComDevice::get_StartOff(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_strStartOff.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComDevice::get_CanBeSource(VARIANT_BOOL* pVal)  // TargetVolumeTag
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_bUseAsSource;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComDevice::get_CanBeTarget(VARIANT_BOOL* pVal)  // TargetVolumeTag
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_bUseAsTarget;
		hr = S_OK;
	}

	return hr;
}
