// ComDeviceList.cpp : Implementation of CDeviceList
#include "stdafx.h"
#include "TDMFObjects.h"
#include "ComDeviceList.h"
#include "ComDevice.h"


/////////////////////////////////////////////////////////////////////////////
// CComDeviceList


STDMETHODIMP CComDeviceList::get_DeviceCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pDeviceList->m_listDevice.size();
		hr =S_OK;
	}

	return hr;
}

STDMETHODIMP CComDeviceList::GetDevice(long nIndex, IDevice **ppDevice)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	if ((nIndex < 0) || (ppDevice == NULL) || ((UINT)nIndex >= m_pDeviceList->m_listDevice.size()))
	{
		hr = E_INVALIDARG;
	}
	else
	{
		*ppDevice = NULL;

		std::list<CDevice>::iterator it;
		long nCount;
		for (it = m_pDeviceList->m_listDevice.begin(), nCount = 0;
			 (nCount < nIndex) && (it != m_pDeviceList->m_listDevice.end()); it++, nCount++);
		if (nCount == nIndex)
		{
			CComObject<CComDevice>* pComDevice = NULL;
			hr = CComObject<CComDevice>::CreateInstance(&pComDevice);
			if (SUCCEEDED(hr))
			{
				(*(CDevice*)pComDevice) = *it;
				pComDevice->AddRef();
				hr = pComDevice->QueryInterface(ppDevice);
				pComDevice->Release();
			}
		}
		else
		{
			hr = E_INVALIDARG;
		}
	}

	return hr;
}

