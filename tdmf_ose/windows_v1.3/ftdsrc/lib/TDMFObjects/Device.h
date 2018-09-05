// Device.h : Declaration of the CDevice

#ifndef __DEVICE_H_
#define __DEVICE_H_


/////////////////////////////////////////////////////////////////////////////
// CDevice
class CDevice
{
public:
	std::string m_strPath;
	std::string m_strFileSystem;

    //The disk related values can be very large numbers, much larger to fit into 32-bits.  
    //Storing and working with floating point types have showed to be problematic: 
    //conversions to/from integer numbers (__int64) introduced errors and imprecisions.
    //These numbers can, for almost all the GUI processing, be treated as text.
    //Therefore, this is the format they will be contained in.
    std::string m_strDriveId; //disk identification nbr
    std::string m_strStartOff;//disk start offset in bytes
    std::string m_strLength;  //disk length/size in bytes

	bool m_bUseAsSource;
	bool m_bUseAsTarget;

	CDevice() : m_strDriveId("0"), m_strStartOff("0"), m_strLength("0"), m_bUseAsSource(true), m_bUseAsTarget(true)
	{
	}
};

#endif //__DEVICE_H_
