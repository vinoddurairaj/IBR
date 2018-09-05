/*****************************************************************************
 *                                                                           *
 *  This software  is the licensed software of Fujitsu Software              *
 *  Technology Corporation                                                   *
 *                                                                           *
 *  Copyright (c) 2002 by Fujitsu Software Technology Corporation            *
 *                                                                           *
 *  THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF            *
 *  FUJITSU SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED             *
 *  UNDER LICENSE FROM FUJITSU SOFTWARE TECHNOLOGY CORPORATION               *
 *                                                                           *
 *****************************************************************************

 *****************************************************************************
 *                                                                           *
 *  Revision History:                                                        *
 *                                                                           *
 *  Date        By              Change                                       *
 *  ----------- --------------  -------------------------------------------  *
 *  08-01-2002  Bob Hudson      Initial version.                             *
 *                                                                           *
 *                                                                           *
 *****************************************************************************/
#include "CLI.h"

#define SCSI_PORT_DEVICE_NAME			"\\\\.\\scsi%d:"
#define PHYSICAL_DRIVE_DEVICE_NAME		"\\\\.\\PhysicalDrive%d"

typedef struct _SCSI_BUS_DATA {
    UCHAR NumberOfLogicalUnits;
    UCHAR InitiatorBusId;
    ULONG InquiryDataOffset;
}SCSI_BUS_DATA, *PSCSI_BUS_DATA;


typedef struct _SCSI_ADAPTER_BUS_INFO {
    UCHAR NumberOfBuses;
    SCSI_BUS_DATA BusData[1];
} SCSI_ADAPTER_BUS_INFO, *PSCSI_ADAPTER_BUS_INFO;

typedef struct _SCSI_INQUIRY_DATA {
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    BOOLEAN DeviceClaimed;
    ULONG InquiryDataLength;
    ULONG NextInquiryDataOffset;
    UCHAR InquiryData[1];
}SCSI_INQUIRY_DATA, *PSCSI_INQUIRY_DATA;


#define IOCTL_SCSI_BASE                 FILE_DEVICE_CONTROLLER
#define IOCTL_SCSI_GET_INQUIRY_DATA     CTL_CODE(IOCTL_SCSI_BASE, 0x0403, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_RESCAN_BUS           CTL_CODE(IOCTL_SCSI_BASE, 0x0407, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define DISK_ADMINISTRATOR_EVENT "Disk Administrator Is Running"


/*
 * Function: 		
 *		VOID		CPUInfo(VOID);
 *
 * Command Line:	/cpuinfo
 *			
 * Arguments: 	none
 *	
 * Returns: It retrieves CPU Architecture and displays CPU Count
 *
 * Conclusion:	It displays information about the current computer system. 
 *				This includes the architecture and type of the processor, 
 *				the number of processors in the system, the page size, and
 *				other such information. 
 *
 * Description:
 *			This Function Calls GetSystemInfo() and displays informations.
 */
VOID	CPUInfo(VOID)
{
	SYSTEM_INFO	SysInfo;
	CHAR		string[256], string1[256];

	GetSystemInfo(&SysInfo);

	printf("\n Command '/CpuInfo' executed successfully using GetSystemInfo() SDK API: \n");
	printf(" System Information: \n");

	printf(" NumberOfProcessors       : %d (0x%08x)\n", SysInfo.dwNumberOfProcessors, SysInfo.dwNumberOfProcessors);

	switch(SysInfo.wProcessorArchitecture)
	{
		case	PROCESSOR_ARCHITECTURE_INTEL:	
					sprintf(string, "PROCESSOR_ARCHITECTURE_INTEL"); 

					switch(SysInfo.wProcessorLevel)
					{
						case 3: strcat(string, " (Intel 80386)"); break;
						case 4: strcat(string, " (Intel 80486)"); break;
						case 5: strcat(string, " (Intel Pentium)"); break;
						case 6: strcat(string, " (Intel Pentium Pro or Pentium II)"); break;
						default:	sprintf(string1," (Intel %d)", SysInfo.dwProcessorType);
									strcat(string, string1);
					}
					break;

		case	PROCESSOR_ARCHITECTURE_MIPS:	sprintf(string, "PROCESSOR_ARCHITECTURE_MIPS"); 
												if (SysInfo.wProcessorLevel == 0004)
													strcat(string, " (MIPS R4000)");
												else
													{
													sprintf(string1, " (MIPS R%4x)", SysInfo.wProcessorLevel);
													strcat(string, string1);
													}
												break;
		case	PROCESSOR_ARCHITECTURE_ALPHA:	sprintf(string, "PROCESSOR_ARCHITECTURE_ALPHA"); 
												sprintf(string1, " (Alpha %d)", SysInfo.wProcessorLevel);
												strcat(string, string1);
												break;
		case	PROCESSOR_ARCHITECTURE_PPC:		sprintf(string, "PROCESSOR_ARCHITECTURE_PPC"); 
												sprintf(string1, " (PPC 60%d)", SysInfo.wProcessorLevel);
												strcat(string, string1);
												break;

		case	PROCESSOR_ARCHITECTURE_SHX:		sprintf(string, "PROCESSOR_ARCHITECTURE_SHX"); break;
		case	PROCESSOR_ARCHITECTURE_ARM:		sprintf(string, "PROCESSOR_ARCHITECTURE_ARM"); break;

		case	PROCESSOR_ARCHITECTURE_IA64:	sprintf(string, "PROCESSOR_ARCHITECTURE_IA64"); break;

		case	PROCESSOR_ARCHITECTURE_ALPHA64:	sprintf(string, "PROCESSOR_ARCHITECTURE_ALPHA64"); break;
		case	PROCESSOR_ARCHITECTURE_MSIL:	sprintf(string, "PROCESSOR_ARCHITECTURE_MSIL"); break;
		case	PROCESSOR_ARCHITECTURE_UNKNOWN:	sprintf(string, "PROCESSOR_ARCHITECTURE_UNKNOWN"); break;
		default:								sprintf(string, "unknown Processor Architecture"); break;
	}
	printf(" Processor Architecture   : %d, (%s) \n", SysInfo.dwOemId, string);
	printf(" ProcessorType            : %d (0x%08x)      \n", SysInfo.dwProcessorType, SysInfo.dwProcessorType);
	printf(" ProcessorLevel           : %d (0x%08x)      \n", SysInfo.wProcessorLevel, SysInfo.wProcessorLevel);
	printf(" ProcessorRevision        : %d (0x%08x)      \n", SysInfo.wProcessorRevision, SysInfo.wProcessorRevision);
	printf(" PageSize                 : %d (0x%08x)\n", SysInfo.dwPageSize, SysInfo.dwPageSize);
	printf(" MinimumApplicationAddress: %d (0x%08x)\n", SysInfo.lpMinimumApplicationAddress, SysInfo.lpMinimumApplicationAddress);
	printf(" MaximumApplicationAddress: %d (0x%08x) \n", 
					SysInfo.lpMaximumApplicationAddress, SysInfo.lpMaximumApplicationAddress,
					( (ULONG) (SysInfo.lpMaximumApplicationAddress)/ (ULONG) (1024*1024*1024)) );
					
	printf(" ActiveProcessorMask      : %d (0x%08x)\n", SysInfo.dwActiveProcessorMask, SysInfo.dwActiveProcessorMask);
	printf(" AllocationGranularity    : %d (0x%08x)\n", SysInfo.dwAllocationGranularity, SysInfo.dwAllocationGranularity);

	printf("\n");
	return;
} // CPUInfo()

/*
 * Function: 		
 *		VOID		MemoryInfo(VOID);
 *
 * Command Line:	/meminfo
 *			
 * Arguments: 	none
 *	
 * Returns: It retrieves information about the system's current usage of 
 *			both physical and virtual memory. 
 *
 * Conclusion:	It displays information about the system's current usage of 
 *				both physical and virtual memory. 
 *
 * Description:
 *			This Function Calls GlobalMemoryStatus() and displays informations.
 */
VOID	MemoryInfo(VOID)
{
	MEMORYSTATUS	Status;

	GlobalMemoryStatus(&Status);

	printf("\n Command '/memInfo' executed successfully using GlobalMemoryStatus() SDK API: \n");
	printf(" Current Memory Information: \n");

	printf(" Total Physical Memory (in Bytes)              : %d (0x%08x) (%d in MB)\n", Status.dwTotalPhys, Status.dwTotalPhys,
																			(Status.dwTotalPhys/(1024*1024)) );
	printf(" Total Physical Memory Available (in Bytes)    : %d (0x%08x) (%d in MB)\n", Status.dwAvailPhys, Status.dwAvailPhys,
																			(Status.dwAvailPhys/(1024*1024)) );
	printf(" MemoryLoad (in % Total Physical Memory in-Use): %d (0x%08x)\n", Status.dwMemoryLoad, Status.dwMemoryLoad);


	printf("\n Total Paging File (Possible) Size in Bytes    : %d (0x%08x) (%d in MB)\n", Status.dwTotalPageFile, Status.dwTotalPageFile,
																			(Status.dwTotalPageFile/(1024*1024)) );
	printf(" Total Paging File Space Available in Bytes    : %d (0x%08x) (%d in MB)\n", Status.dwAvailPageFile, Status.dwAvailPageFile,
																			(Status.dwAvailPageFile/(1024*1024)) );
	
	printf("\n Calling Process's Total Virual address space size      : %d (0x%08x) in Bytes\n", Status.dwTotalVirtual, Status.dwTotalVirtual);
	printf(" Calling Process's unreserved/uncommitted VA memory size: %d (0x%08x) in Bytes\n", Status.dwAvailVirtual, Status.dwAvailVirtual);
	return;	
} // MemoryInfo()

/*
 * Function: 		
 *		VOID RescanDisks(VOID)
 *
 * Command Line:	/RescanDisks
 *			
 * Arguments: 	none
 *	
 * Returns: It rescan Scsi Ports to discovers any new disk devices.
 *
 * Conclusion:	It Rescan each and every Scsi Port and issues 
 *				IOCTL_DISK_FIND_NEW_DEVICES for each and every disk 
 *				device exist on the local system
 *
 * Description:
 *			This Function Opens all "\\\\.\\scsi%d:" devices and sends
 *			IOCTL_SCSI_RESCAN_BUS and for each & every disk devices: 
 *			"\\\\.\\PhysicalDrive%d" it sends IOCTL_DISK_FIND_NEW_DEVICES.
 */
VOID RescanDisks(VOID)
{
	printf("\n Rescanning the SCSI buses, please wait ....\n");

#if 0 // Only for Windows NT, anyhow it won't harm 
	if(IsDiskAdministratorRunning())
	{
		printf("Disk administrator is running. Can't rescan SCSI disks now!\n");
		return;
	}
#endif

	RescanScsiBus();
	DiskIoctlNewDisk();

	printf("\n Command '/RescanDisks' executed successfully. \n");
} // VOID RescanDisks(VOID)

LONG RescanScsiBus ()
{
	HANDLE fileHandle;
    DWORD dwError = 0;
	ULONG returned=0;
	INT status;
	CHAR devicePath[256];
	LONG count=0;
	ULONG port;
    
	for(port=0;;port++)
	{
		// Open the device
		sprintf(devicePath, SCSI_PORT_DEVICE_NAME, port);
		fileHandle = CreateFile(devicePath,
								GENERIC_READ | GENERIC_WRITE,
								(FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE),
    							NULL,
    							OPEN_EXISTING,
    							FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
    							NULL);

		if(fileHandle==INVALID_HANDLE_VALUE)
			break;

		status = DeviceIoControl(fileHandle,
								 IOCTL_SCSI_RESCAN_BUS,
								 NULL,
								 0,
								 NULL,
								 0,
								 &returned,
								 FALSE);
		if (!status) 
		{
			CloseHandle(fileHandle);
			continue;
		}
#if 0
		UCHAR buffer[2048];
		ULONG	i;
		PSCSI_ADAPTER_BUS_INFO  sabi;
		PSCSI_INQUIRY_DATA sid;
	
		status = DeviceIoControl(fileHandle,
								 IOCTL_SCSI_GET_INQUIRY_DATA,
								 NULL,
								 0,
								 buffer,
								 sizeof(buffer),
								 &returned,
								 FALSE);
	

		if(status)
		{
			sabi = (SCSI_ADAPTER_BUS_INFO *)buffer;
	
			for (i = 0; i < sabi->NumberOfBuses; i++) 
			{
				sid = (PSCSI_INQUIRY_DATA) (buffer + sabi->BusData[i].InquiryDataOffset);

				while (sabi->BusData[i].InquiryDataOffset) 
				{
					if(!sid->DeviceClaimed)
						count++;
				
					if (sid->NextInquiryDataOffset == 0) 
						break;
          
					sid = (PSCSI_INQUIRY_DATA) (buffer +
								sid->NextInquiryDataOffset);
				}
			}
		}
#endif
		CloseHandle(fileHandle);
	} // for(ULONG port=0;;port++)
	
	return count;
} // long RescanScsiBus ()

BOOLEAN DiskIoctlNewDisk()
{
	CHAR	devicePath[256];
	ULONG	diskNumber;
	HANDLE	fileHandle;
	ULONG	returned=0;
	INT		status;
	
	for(diskNumber=0;;diskNumber++)
	{
		// Open the device
		sprintf(devicePath, PHYSICAL_DRIVE_DEVICE_NAME, diskNumber);
			
		fileHandle = CreateFile(devicePath,
								GENERIC_READ | GENERIC_WRITE,
								(FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE),
    							NULL,
    							OPEN_EXISTING,
    							FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
    							NULL);
		
		if(fileHandle==INVALID_HANDLE_VALUE)
			break;

		status = DeviceIoControl(fileHandle,
								 IOCTL_DISK_FIND_NEW_DEVICES,
								 NULL,
								 0,
								 NULL,
								 0,
								 &returned,
								 FALSE);
		CloseHandle(fileHandle);
	} // for(diskNumber=0;;diskNumber++)

	return TRUE;
} // DiskIoctlNewDisk()

BOOLEAN IsDiskAdministratorRunning()
{
	HANDLE handle=CreateMutex(NULL,TRUE, DISK_ADMINISTRATOR_EVENT);
	
	if(handle==NULL)
		return TRUE;

	if(GetLastError()==ERROR_ALREADY_EXISTS)
	{
		CloseHandle(handle);
		SetLastError(ERROR_ALREADY_EXISTS);
		return TRUE;
	}

	CloseHandle(handle);
	return FALSE;
} // IsDiskAdministratorRunning()

