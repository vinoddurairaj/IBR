// ComDevice.h : Declaration of the CComDevice

#ifndef __COMDEVICE_H_
#define __COMDEVICE_H_

#include "resource.h"       // main symbols
#include "Device.h"         // CDevice

/////////////////////////////////////////////////////////////////////////////
// CComDevice
class ATL_NO_VTABLE CComDevice : 
	public CDevice,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CComDevice, &CLSID_Device>,
	public IDevice
{
public:
	CComDevice() {}

DECLARE_REGISTRY_RESOURCEID(IDR_DEVICE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComDevice)
	COM_INTERFACE_ENTRY(IDevice)
END_COM_MAP()

// IDevice
public:
	STDMETHOD(get_StartOff)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_DriveId)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_FileSystem)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Size)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_CanBeSource)(VARIANT_BOOL* pVal);
	STDMETHOD(get_CanBeTarget)(VARIANT_BOOL* pVal);
};

#endif //__COMDEVICE_H_
