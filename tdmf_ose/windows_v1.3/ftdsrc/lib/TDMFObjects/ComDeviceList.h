// ComDeviceList.h : Declaration of the CDeviceList

#ifndef __COMDEVICELIST_H_
#define __COMDEVICELIST_H_

#include "resource.h"       // main symbols
#include "DeviceList.h"


/////////////////////////////////////////////////////////////////////////////
// CDeviceList
class ATL_NO_VTABLE CComDeviceList : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CComDeviceList, &CLSID_DeviceList>,
	public IDeviceList
{
public:
	CComDeviceList() : m_pDeviceList(NULL)
	{
	}

	~CComDeviceList() 
	{
		if (m_pDeviceList != NULL)
		{
			delete m_pDeviceList;
			m_pDeviceList = NULL;
		}
	}

	CDeviceList* m_pDeviceList;

DECLARE_REGISTRY_RESOURCEID(IDR_DEVICELIST)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComDeviceList)
	COM_INTERFACE_ENTRY(IDeviceList)
END_COM_MAP()

// IDeviceList
public:
	STDMETHOD(GetDevice)(/*[in]*/ long index, /*[out, retval]*/ IDevice** pDevice);
	STDMETHOD(get_DeviceCount)(/*[out, retval]*/ long *pVal);
};

#endif //__COMDEVICELIST_H_
