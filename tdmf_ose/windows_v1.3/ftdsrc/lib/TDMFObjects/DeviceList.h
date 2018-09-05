// DeviceList.h : Declaration of the CDeviceList

#ifndef __DEVICELIST_H_
#define __DEVICELIST_H_

#include "resource.h"       // main symbols
#include "Device.h"


/////////////////////////////////////////////////////////////////////////////
// CDeviceList
class CDeviceList 
{
public:
	CDeviceList() {}

	std::list<CDevice> m_listDevice;
};

#endif //__DEVICELIST_H_
