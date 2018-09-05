// ReplicationPair.cpp : Implementation of CComReplicationPair
#include "stdafx.h"
#include "TDMFObjects.h"
#include "ComReplicationPair.h"
#include "ComReplicationGroup.h"


/////////////////////////////////////////////////////////////////////////////
//  CComReplicationPair

STDMETHODIMP CComReplicationPair::get_Name(BSTR *pVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		std::ostringstream oss;
		oss << m_pRP->m_DeviceSource.m_strPath << " => " << m_pRP->m_DeviceTarget.m_strPath;
		*pVal = CComBSTR(oss.str().c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::get_Description(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRP->m_strDescription.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::put_Description(BSTR newVal)
{
    USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRP->m_strDescription = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationPair::get_PairNumber(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRP->m_nPairNumber;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::put_PairNumber(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRP->m_nPairNumber = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationPair::get_Parent(IReplicationGroup **pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		CComObject<CComReplicationGroup>* pComRG = NULL;
		hr = CComObject<CComReplicationGroup>::CreateInstance(&pComRG);
		if (SUCCEEDED(hr))
		{
			pComRG->m_pRG = m_pRP->m_pParent;
			pComRG->AddRef();
			hr = pComRG->QueryInterface(pVal);
			pComRG->Release();
		}
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::get_SrcSize(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRP->m_DeviceSource.m_strLength.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::put_SrcSize(BSTR newVal)
{
    USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRP->m_DeviceSource.m_strLength = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationPair::get_TgtSize(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRP->m_DeviceTarget.m_strLength.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::put_TgtSize(BSTR newVal)
{
    USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRP->m_DeviceTarget.m_strLength = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationPair::get_FileSystem(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRP->m_strFileSystem.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::put_FileSystem(BSTR newVal)
{
    USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRP->m_strFileSystem = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationPair::get_SrcName(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRP->m_DeviceSource.m_strPath.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::put_SrcName(BSTR newVal)
{
    USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRP->m_DeviceSource.m_strPath = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationPair::get_TgtName(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRP->m_DeviceTarget.m_strPath.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::put_TgtName(BSTR newVal)
{
    USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRP->m_DeviceTarget.m_strPath = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationPair::get_Key(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRP->GetKey().c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::IsEqual(IReplicationPair* pRP, BOOL *pbRetVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CComBSTR bstrKey;
	HRESULT hr = pRP->get_Key(&bstrKey);
	if (SUCCEEDED(hr))
	{
		*pbRetVal = (m_pRP->GetKey() == OLE2A(bstrKey));
	}

	return hr;
}


STDMETHODIMP CComReplicationPair::get_ObjectState(ReplicationPairObjectState *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = (ReplicationPairObjectState)m_pRP->m_eObjectState;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationPair::put_ObjectState(ReplicationPairObjectState newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRP->m_eObjectState = (CReplicationPair::ObjectState)newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationPair::CopyDevice(BOOL bSourceServer, IDevice *pDevice)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CDevice* pDeviceRP = bSourceServer ? &m_pRP->m_DeviceSource : &m_pRP->m_DeviceTarget;

	CComBSTR bstrVal;

	if (SUCCEEDED(pDevice->get_Name(&bstrVal)))
	{
		pDeviceRP->m_strPath = OLE2A(bstrVal);
		bstrVal.Empty(); // to avoid memory leaks
	}
	if (SUCCEEDED(pDevice->get_FileSystem(&bstrVal)))
	{
		m_pRP->m_strFileSystem = OLE2A(bstrVal);
		// Dup info
		pDeviceRP->m_strFileSystem = OLE2A(bstrVal);
		bstrVal.Empty();
	}
	if (SUCCEEDED(pDevice->get_DriveId(&bstrVal)))
	{
		pDeviceRP->m_strDriveId = OLE2A(bstrVal);
		bstrVal.Empty();
	}
	if (SUCCEEDED(pDevice->get_StartOff(&bstrVal)))
	{
		pDeviceRP->m_strStartOff = OLE2A(bstrVal);
		bstrVal.Empty();
	}
	if (SUCCEEDED(pDevice->get_Size(&bstrVal)))
	{
		pDeviceRP->m_strLength = OLE2A(bstrVal);
		bstrVal.Empty();
	}

	return S_OK;
}
