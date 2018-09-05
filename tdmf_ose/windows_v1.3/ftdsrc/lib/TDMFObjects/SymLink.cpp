// SymLink.cpp
//
#include "stdafx.h"
#include <Windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <malloc.h>
#include <lmerr.h>			// for GetErrorText()
#include "SymLink.h"


HANDLE			handle = INVALID_HANDLE_VALUE;

DEVICE_INFO SymbolicLinkInfo( char DriveLetter )
{
	int				IoctlResult;
	DWORD			dwNumberOfBytesReturned;
	int				size;
	char			temp[128];
	DEVICE_INFO		devinfo;
	char			drvLetter[2];
//	FILE		*file;

	drvLetter[0] = DriveLetter;
	drvLetter[1] = NULL;
	InitializeHandle( drvLetter[0] );
	memset(&devinfo, 0, sizeof(devinfo));

	if (handle == INVALID_HANDLE_VALUE) 
	{
		printf("Failed Createfile(%s) With GetlastError %d\n", DRIVE_LETTER_STR, GetLastError());
		DisplayErrorText();
		goto done;
	}
	else
	{
		size = sizeof(devinfo.StorageDeviceNumber);
		memset(&devinfo.StorageDeviceNumber, 0, size);
		// IOCTL_STORAGE_GET_DEVICE_NUMBER to retrieve this info, 
		// Example : \\Device\HardDisk(n)\\Partition(n)
		IoctlResult = DeviceIoControl(
							handle,    // Handle to device
							IOCTL_STORAGE_GET_DEVICE_NUMBER,      // IO Control code for Read
							NULL,        // Input Buffer to driver.
							0,       // Length of buffer in bytes.
							&devinfo.StorageDeviceNumber,        // Output Buffer from driver.
							size,       // Length of buffer in bytes.
							&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
							NULL        // NULL means wait till op. completes.
							);

		if( IoctlResult )
		{
			// succeeded
			// Create device name for each partition
			swprintf(	devinfo.DiskPartitionName,	
						DEVICE_HARDDISK_PARTITION,	// L"\\Device\\Harddisk%d\\Partition%d"
						devinfo.StorageDeviceNumber.DeviceNumber, 
						devinfo.StorageDeviceNumber.PartitionNumber);

			// Set default name for physical disk
			memcpy(	&(devinfo.StorageManagerName[0]),
					L"PhysDisk",
					8 * sizeof(WCHAR));

			devinfo.bStorage_device_Number = TRUE;

			WideCharToMultiByte( CP_ACP, 0, devinfo.DiskPartitionName, -1, temp, 256, NULL, NULL );
			printf("\nDevice link in \\Device\\Harddisk\\Partition format: \n");
			printf("\t%S\n", devinfo.DiskPartitionName );
			//SymLink1 = temp;

		}
		else
		{
			swprintf(	devinfo.DiskPartitionName, 
						JUNK	);

			printf("\nIoctl IOCTL_STORAGE_GET_DEVICE_NUMBER failed. This may be a dynamic volume\n");
		}

		// IOCTL_VOLUME_QUERY_VOLUME_NUMBER to retrieve HarddiskVolume number and its volumename like logdisk, etc..
		// if disk is LogiDisk type then we get following info successfully.
		size = sizeof(devinfo.VolumeNumber);
		memset(&devinfo.VolumeNumber, 0, size);
		// Now, get the VOLUME_NUMBER information
		IoctlResult = DeviceIoControl(
							handle,    // Handle to device
							IOCTL_VOLUME_QUERY_VOLUME_NUMBER,      // IO Control code for Read
							NULL,        // Input Buffer to driver.
							0,       // Length of buffer in bytes.
							&devinfo.VolumeNumber,        // Output Buffer from driver.
							size,       // Length of buffer in bytes.
							&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
							NULL        // NULL means wait till op. completes.
							);
		//RtlZeroMemory(&devinfo.StorageManagerName[0], sizeof(devinfo.StorageManagerName));
		if( IoctlResult )
		{
				memcpy(	&devinfo.StorageManagerName[0],
						&devinfo.VolumeNumber.VolumeManagerName[0],
						8 * sizeof(WCHAR));
				devinfo.bVolumeNumber = TRUE;
		}
		else
		{
			if (devinfo.VolumeNumber.VolumeManagerName[0] == (WCHAR) UNICODE_NULL ) 
			{
				memcpy(	&devinfo.StorageManagerName[0],
						L"LogiDisk",
						8 * sizeof(WCHAR));
			}
			printf("\nIoctl IOCTL_VOLUME_QUERY_VOLUME_NUMBER failed. This may be a dynamic volume\n");
		}

		{  // Printf statement
			ULONG i;
			printf("\nStorageManagerName: \n\t");
			for (i=0;i<(sizeof(devinfo.StorageManagerName)/2);i++)
				printf("%C",devinfo.StorageManagerName[i]);

		}
		// IOCTL_MOUNTDEV_QUERY_DEVICE_NAME used to retrieve \Device\HarddiskVolume1 into DiskVolumeName...
		{	// Get DiskVolume name information 
			PMOUNTDEV_NAME	pOutput = NULL;
			MOUNTDEV_NAME	output;
			ULONG			outputSize = sizeof(MOUNTDEV_NAME);
			memset(&output, 0, outputSize);

			// Now, get the VOLUME_NUMBER information
			IoctlResult = DeviceIoControl(
								handle,    // Handle to device
								IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,      // IO Control code for Read
								NULL,        // Input Buffer to driver.
								0,       // Length of buffer in bytes.
								&output,        // Output Buffer from driver.
								outputSize,       // Length of buffer in bytes.
								&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
								NULL        // NULL means wait till op. completes.
								);

			if (!IoctlResult) 
			{ // now retrieve actual DiskVolume Name with passing proper asking size.
				outputSize = sizeof(MOUNTDEV_NAME) + output.NameLength;
				pOutput = (PMOUNTDEV_NAME) malloc(outputSize);
				if (pOutput == NULL)
				{ // Failed
					printf("\n Error: malloc( Size %d): failed \n", outputSize);
					goto done;
				}
				memset(pOutput, 0, outputSize);
				
				IoctlResult = DeviceIoControl(
								handle,    // Handle to device
								IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,      // IO Control code for Read
								NULL,        // Input Buffer to driver.
								0,       // Length of buffer in bytes.
								pOutput,        // Output Buffer from driver.
								outputSize,       // Length of buffer in bytes.
								&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
								NULL        // NULL means wait till op. completes.
								);
			} // if (status == STATUS_BUFFER_OVERFLOW) 

			if (IoctlResult) 
			{	//succeeded

				// DiskVolumeName will have a value like \Device\HardDiskVolume(nnn) where nnn stands for volume number...
				memset(	devinfo.DiskVolumeName, 0, sizeof(devinfo.DiskVolumeName) );

				memcpy(	devinfo.DiskVolumeName,
						pOutput->Name,
						pOutput->NameLength);

				devinfo.bDiskVolumeName = TRUE;
				printf("\n\nDevice link in \\Device\\HarddiskVolume format: \n");
				printf("\t%S\n", devinfo.DiskVolumeName );
				WideCharToMultiByte( CP_ACP, 0, devinfo.DiskVolumeName, -1, temp, 256, NULL, NULL );
				//SymLink2 = temp;

			}
			else
			{
				printf("\nIoctl IOCTL_MOUNTDEV_QUERY_DEVICE_NAME failed. This may be a dynamic volume\\n");
			}
			if (pOutput)
			{
				free(pOutput);
				pOutput = NULL;
			}

		} // Get DiskVolume name information 

		// following fields are supported only >= Win2k OS 

		// IOCTL_MOUNTDEV_QUERY_UNIQUE_ID used to retrieve Volume/Disk Unique Persistence ID, 
		// It Contains the unique volume ID. The format for unique volume names is "\??\Volume{GUID}\", 
		// where GUID is a globally unique identifier that identifies the volume.	
		{
			PMOUNTDEV_UNIQUE_ID		pOutput = NULL;
			MOUNTDEV_UNIQUE_ID		uniqueIdInfo;
			ULONG					outputSize = sizeof(MOUNTDEV_UNIQUE_ID);

			memset(	&uniqueIdInfo, 0, outputSize );

			IoctlResult = DeviceIoControl(
								handle,    // Handle to device
								IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,      // IO Control code for Read
								NULL,        // Input Buffer to driver.
								0,       // Length of buffer in bytes.
								&uniqueIdInfo,        // Output Buffer from driver.
								outputSize,       // Length of buffer in bytes.
								&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
								NULL        // NULL means wait till op. completes.
								);

			if (!IoctlResult) 
			{	// now retrieve actual DiskVolume Name with passing proper asking size.
				outputSize = sizeof(MOUNTDEV_UNIQUE_ID) + uniqueIdInfo.UniqueIdLength;
				pOutput = (PMOUNTDEV_UNIQUE_ID) malloc(outputSize);
				if (pOutput == NULL)
				{ // Failed
					printf("\n Error: malloc( Size %d): failed \n", outputSize);
					goto done;
				}

				memset(	pOutput, 0, outputSize );
				
				IoctlResult = DeviceIoControl(
								handle,    // Handle to device
								IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,      // IO Control code for Read
								NULL,        // Input Buffer to driver.
								0,       // Length of buffer in bytes.
								pOutput,        // Output Buffer from driver.
								outputSize,       // Length of buffer in bytes.
								&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
								NULL        // NULL means wait till op. completes.
								);

			} // if (status == STATUS_BUFFER_OVERFLOW) 

			if (IoctlResult) 
			{	//succeeded
				devinfo.UniqueIdInfo = pOutput;
				devinfo.bUniqueVolumeId = TRUE;

				{  // Printf statement
					ULONG i;
					printf("\nUniqueId: \n\t");
					for (i=0;i<(devinfo.UniqueIdInfo->UniqueIdLength);i++)
						printf("%x",devinfo.UniqueIdInfo->UniqueId[i]);
						printf("\n");

				}

				// DiskVolumeName will have a value like \Device\HardDiskVolume(nnn) where nnn stands for volume number...
				//printf("IOCTL_MOUNTDEV_QUERY_UNIQUE_ID : Length : %d, UniqueId %s \n", devinfo.UniqueIdInfo->UniqueIdLength, devinfo.UniqueIdInfo->UniqueId);
			}
			else
			{
				printf("\nIoctl IOCTL_MOUNTDEV_QUERY_UNIQUE_ID failed.\n");
			}

			if (pOutput)
			{
				free(pOutput);
				pOutput = NULL;
			}
		}

		// following fields are supported only >= Win2k OS 
		// IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME is Optional returns Drive letter (if Drive Letter is persistent across boot) or
		// suggest Drive Letter Dos Sym,bolic Name.	
		{
			PMOUNTDEV_SUGGESTED_LINK_NAME	pOutput = NULL;
			MOUNTDEV_SUGGESTED_LINK_NAME	suggestedDriveLinkName;
			ULONG							outputSize = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);

			// First Retrieve Total size of Unique Id name
			memset(	&suggestedDriveLinkName, 0, outputSize );

			IoctlResult = DeviceIoControl(
								handle,    // Handle to device
								IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME,      // IO Control code for Read
								NULL,        // Input Buffer to driver.
								0,       // Length of buffer in bytes.
								&suggestedDriveLinkName,        // Output Buffer from driver.
								outputSize,       // Length of buffer in bytes.
								&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
								NULL        // NULL means wait till op. completes.
								);

			if (!IoctlResult) 
			{ // now retrieve actual DiskVolume Name with passing proper asking size.
				outputSize = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME) + suggestedDriveLinkName.NameLength;
				pOutput = (PMOUNTDEV_SUGGESTED_LINK_NAME) malloc(outputSize);
				if (pOutput == NULL)
				{ // Failed
					printf("\n Error: malloc( Size %d): failed \n", outputSize);
					goto done;
				}

				memset(	pOutput, 0, outputSize );
				
				IoctlResult = DeviceIoControl(
								handle,    // Handle to device
								IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME,      // IO Control code for Read
								NULL,        // Input Buffer to driver.
								0,       // Length of buffer in bytes.
								pOutput,        // Output Buffer from driver.
								outputSize,       // Length of buffer in bytes.
								&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
								NULL        // NULL means wait till op. completes.
								);
			} // if (status == STATUS_BUFFER_OVERFLOW) 

			if (IoctlResult)
			{ //succeeded
				devinfo.SuggestedDriveLinkName = pOutput;
				devinfo.bSuggestedDriveLetter = TRUE;

				{  // Printf statement
					printf("\nSuggestedDriveLinkName: \n\t");
					printf("%S",devinfo.SuggestedDriveLinkName->Name);
				}
			}
			else
			{
				printf("\nIoctl IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME failed. This fails for Basic Partitions\n");
			}

			if (pOutput)
			{
				free(pOutput);
				pOutput = NULL;
			}

		}

		// Generate Customize Unique Signature Volume id , this specially is used for Windows NT 4.0 systems
		if(!sftk_GenerateSignatureGuid(&devinfo, handle))
		{ // Failed 
			printf("sftk_GenerateSignatureGuid: Failed!!\n"); 
			// TODO can we do anything else don't think so... so just return error
			goto done;
		}

		printf("\nUnique Signature Volume id: \n");
				printf("\t%s\n", devinfo.SignatureUniqueId );
		//SymLink3 = devinfo.SignatureUniqueId;


		if (	(devinfo.bStorage_device_Number == TRUE) && 
				(devinfo.bVolumeNumber == TRUE) &&
				(devinfo.bDiskVolumeName == TRUE) && 
				(devinfo.bUniqueVolumeId == TRUE) &&
				(devinfo.bSuggestedDriveLetter == TRUE))
		{
			devinfo.bValidName = TRUE;
		}
		
	}

	/*file = fopen( filename, "r+");

	if(file)
	{
		fwrite("Device Information\n", sizeof(char), strlen("Device Information\n"), file);
		fwrite(DiskName, sizeof(char), strlen(DiskName), file);
		fwrite("\nEnd Of Device Information\n\n", sizeof(char), strlen("End Of Device Information\n\n"), file);
		fwrite("End Of Configuration Files\n\n", sizeof(char), strlen("End Of Configuration Files\n\n"), file);
		fclose(file);
	}*/

done:
	if (handle != INVALID_HANDLE_VALUE)
		CloseHandle(handle);
	return devinfo;
}

int
InitializeHandle(char DriveLetter)
{
	char			driveLetter[6];
//	char*			filename;
	ULONG			dwDesiredAccess, dwShareMode, dwCreationDisposition;

	strcpy(driveLetter, DRIVE_LETTER_STR);
	strcat( driveLetter, &DriveLetter);
	//strcat( driveLetter, "X");
	driveLetter[5] = NULL;
	strcat( driveLetter, ":");

//	filename = FILENAME;

	dwDesiredAccess			= GENERIC_READ;
	dwShareMode				= FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	dwCreationDisposition	= OPEN_EXISTING;

	
	handle = CreateFile(	driveLetter, dwDesiredAccess, dwShareMode, 
							NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);

	return 1;
}

// This API will create Signature based unique ID. This API supports Disk/Disk Partition/ Disk Dynamic Spanned Partition
int	
sftk_GenerateSignatureGuid( PDEVICE_INFO	devinfo, HANDLE handle )
{
	int							status = 0;
	PDRIVE_LAYOUT_INFORMATION	pDriveLayout = NULL;
	BOOLEAN						bRawDisk	= FALSE;
	BOOLEAN						bIsVolume	= FALSE;
	int							IoctlResult;
	DWORD						dwNumberOfBytesReturned;
	int							outputSize;
	char						physicalDrive[128];
	HANDLE						physicalDriveHandle;

	// Retrieve Signature + Partition Starting Offset + Size of Partition.
	// If its Raw disk than Starting Offset may be  0 and size of partition may be complete size of disk.
	if ( devinfo->bStorage_device_Number == TRUE ) 
	{ // we have valid disk and Partition number 
		if ( (devinfo->StorageDeviceNumber.DeviceNumber	!= -1) &&
			 (devinfo->StorageDeviceNumber.PartitionNumber == 0) )
		{	// its RAW Disk device 
			// Current Device is RAW Device.so do not need to retrieve Pointer for it
			devinfo->StartingOffset.QuadPart  = 0; // its RAW Disk 
			devinfo->PartitionLength.QuadPart = 0; // its RAW Disk 
			bRawDisk = TRUE;

		} 
		else
		{	
			printf("\n\ncurrent device is either Partition\\Dynamic partition\\ or FTDisk object.\n\n");
		}
	} // we have valid disk and Partition number 
	else
	{ // else get valid disk number 
		// - Get Volume Or partition Raw Disk number, if its Dynamic or FT Partition than we use 
		// first Disk of partition for signature.
		status = sftk_GetVolumeDiskExtents( devinfo, handle );

		if(!status)
		{ // Failed 
			printf("sftk_GenerateSignatureGuid: sftk_GetVolumeDiskExtents() Failed!!\n"); 
			// TODO can we do anything else don't think so... so just return error
			goto done;
		}
		bIsVolume = TRUE;
	} // else get valid disk number 

	sprintf(	physicalDrive,	
				PHYSICALDRIVE,
				devinfo->StorageDeviceNumber.DeviceNumber);

	physicalDriveHandle = CreateFile(	physicalDrive, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
							NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (physicalDriveHandle == INVALID_HANDLE_VALUE) 
	{
		printf("Failed Createfile(%s) With GetlastError %d\n", physicalDrive, GetLastError());
		DisplayErrorText();
		goto done;
	}

	outputSize = 8192;
	pDriveLayout = (PDRIVE_LAYOUT_INFORMATION) malloc(outputSize);
	if (!pDriveLayout)
	{
		// Failed
		printf("\n Error: malloc( Size %d): failed \n", outputSize);
		goto done;
	}
	memset(	pDriveLayout, 0, outputSize );

	IoctlResult = DeviceIoControl(
						physicalDriveHandle,    // Handle to device
						IOCTL_DISK_GET_DRIVE_LAYOUT,      // IO Control code for Read
						NULL,        // Input Buffer to driver.
						0,       // Length of buffer in bytes.
						pDriveLayout,        // Output Buffer from driver.
						outputSize,       // Length of buffer in bytes.
						&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
						NULL        // NULL means wait till op. completes.
						);

	if (!IoctlResult)
	{ // failed
		printf("sftk_GenerateSignatureGuid() for IOCTL_DISK_GET_DRIVE_LAYOUT failed With GetlastError %d\n", GetLastError());

		if (pDriveLayout)
		{
			free(pDriveLayout);
			pDriveLayout = NULL;
		}

		goto done;
	}

	// store signature
	devinfo->Signature = pDriveLayout->Signature;	// store signature

	if (pDriveLayout)
	{
		free(pDriveLayout);
		pDriveLayout = NULL;
	}

	// - Retrieve Partition starting Offset and Size in bytes.
	if ( (bRawDisk == FALSE) && (bIsVolume == FALSE) )
	{ // Its basic Partition, Retrieve Partition starting Offset and Size in bytes.
		PPARTITION_INFORMATION pPartitionInfo;
		outputSize = sizeof(PARTITION_INFORMATION);
		pPartitionInfo = (PPARTITION_INFORMATION) malloc(outputSize);

		if (!pPartitionInfo)
		{
			// Failed
			printf("\n Error: malloc( Size %d): failed \n", outputSize);
			goto done;
		}

		memset(	pPartitionInfo, 0, outputSize );

		IoctlResult = DeviceIoControl(
						handle,    // Handle to device
						IOCTL_DISK_GET_PARTITION_INFO,      // IO Control code for Read
						NULL,        // Input Buffer to driver.
						0,       // Length of buffer in bytes.
						pPartitionInfo,        // Output Buffer from driver.
						outputSize,       // Length of buffer in bytes.
						&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
						NULL        // NULL means wait till op. completes.
						);

		if (!IoctlResult)
		{ // failed
			printf("sftk_GenerateSignatureGuid() for IOCTL_DISK_GET_PARTITION_INFO failed.\n");
			
			if (pPartitionInfo)
			{
				free(pPartitionInfo);
				pPartitionInfo = NULL;
			}
			
			goto done;
		}

		devinfo->StartingOffset	= pPartitionInfo->StartingOffset;
		devinfo->PartitionLength	= pPartitionInfo->PartitionLength;

		if (pPartitionInfo)
		{
			free(pPartitionInfo);
			pPartitionInfo = NULL;
		}

	} // Its basic Partition, Retrieve Partition starting Offset and Size in bytes.

	// - Genarate & store Unique Signature based Id - 
	sprintf(	(char*)devinfo->SignatureUniqueId,	
				SIGNATURE_UNIQUE_ID,	// "volume(%08x-%I64x-%I64x)"	// total 50 bytes
				devinfo->Signature,
				devinfo->StartingOffset,
				devinfo->PartitionLength);
				

	devinfo->SignatureUniqueIdLength = strlen( (char*)(devinfo->SignatureUniqueId) );
	devinfo->bSignatureUniqueVolumeId = TRUE;	// we genareted signature unique id successfuly
	status = 1;

done:

	if (physicalDriveHandle != INVALID_HANDLE_VALUE)
		CloseHandle(physicalDriveHandle);
	return status;
} // sftk_GenerateSignatureGuid()

//
//
int 
sftk_GetVolumeDiskExtents( PDEVICE_INFO	devinfo, HANDLE handle )
{
	int						status				= 0;
	PVOLUME_DISK_EXTENTS	pVolumeDiskExtents	= NULL;
	VOLUME_DISK_EXTENTS		VolumeDiskExtents;
	ULONG					outputSize			= sizeof(VOLUME_DISK_EXTENTS);
	int						IoctlResult;
	DWORD					dwNumberOfBytesReturned;

	memset(	&VolumeDiskExtents, 0, outputSize );

	IoctlResult = DeviceIoControl(
					handle,    // Handle to device
					IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,      // IO Control code for Read
					NULL,        // Input Buffer to driver.
					0,       // Length of buffer in bytes.
					&VolumeDiskExtents,        // Output Buffer from driver.
					outputSize,       // Length of buffer in bytes.
					&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
					NULL        // NULL means wait till op. completes.
					);

	if(!IoctlResult)
	{
		outputSize= sizeof(VOLUME_DISK_EXTENTS) + VolumeDiskExtents.NumberOfDiskExtents * sizeof(DISK_EXTENT);

		pVolumeDiskExtents = (PVOLUME_DISK_EXTENTS)calloc(1, outputSize);
		if (!pVolumeDiskExtents)
		{
			// Failed
			printf("\n Error: malloc( Size %d): failed \n", outputSize);
			goto done;
		}

		memset(	&VolumeDiskExtents, 0, outputSize );

		IoctlResult = DeviceIoControl(
					handle,    // Handle to device
					IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,      // IO Control code for Read
					NULL,        // Input Buffer to driver.
					0,       // Length of buffer in bytes.
					&VolumeDiskExtents,        // Output Buffer from driver.
					outputSize,       // Length of buffer in bytes.
					&dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
					NULL        // NULL means wait till op. completes.
					);
	} // if(status==STATUS_BUFFER_OVERFLOW) 

	if(IoctlResult)
	{
		if( (VolumeDiskExtents.NumberOfDiskExtents > 0) &&
			(wcsncmp(devinfo->StorageManagerName, L"FTDISK", 6) ==0) )
		{ // current device is FTDISK !!!
			printf("sftk_GetVolumeDiskExtents():: This is FTDISK Device !!\n"); 
			devinfo->IsVolumeFtVolume = TRUE;
		}

		devinfo->StorageDeviceNumber.DeviceNumber	= VolumeDiskExtents.Extents[0].DiskNumber;
		devinfo->StartingOffset						= VolumeDiskExtents.Extents[0].StartingOffset;
		devinfo->PartitionLength					= VolumeDiskExtents.Extents[0].ExtentLength;
		devinfo->bStorage_device_Number				= TRUE;
		status = 1;
	}

done:
	if (pVolumeDiskExtents)
	{
		free(pVolumeDiskExtents);
		pVolumeDiskExtents = NULL;
	}
	return status;
} // sftk_GetVolumeDiskExtents()

// internal function used to convert SDK API GetLastError() into string format and returns error text string to caller
VOID	DisplayErrorText()
{
    HMODULE hModule = NULL; // default to system source
    LPSTR MessageBuffer;
    DWORD dwBufferLength;
	DWORD dwLastError	= GetLastError();
	DWORD dwFormatFlags =	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_IGNORE_INSERTS |
							FORMAT_MESSAGE_FROM_SYSTEM ;

	// display error details
	printf("\tError Detailed Information: \n");	
	printf("\tGetLastError() = %d (0x%08x) \n", dwLastError, dwLastError);	

    
    // If dwLastError is in the network range, 
    //  load the message source.

    if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) {
        hModule = LoadLibraryEx(
            TEXT("netmsg.dll"),
            NULL,
            LOAD_LIBRARY_AS_DATAFILE
            );

        if(hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    // Call FormatMessage() to allow for message 
    //  text to be acquired from the system 
    //  or from the supplied module handle.

    if(dwBufferLength = FormatMessageA(
        dwFormatFlags,
        hModule, // module to get message from (NULL == system)
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
        (LPSTR) &MessageBuffer,
        0,
        NULL
        ))
    {
        // Output message string on stderr.
		printf("\tErrorString    = %s \n",MessageBuffer);	

        // Free the buffer allocated by the system.
        LocalFree(MessageBuffer);
    } 
	else
	{
		printf("\tErrorString    = Unknown Error \n");	
	}

    // If we loaded a message source, unload it.
    if(hModule != NULL)
        FreeLibrary(hModule);

} // DisplayErrorText()

