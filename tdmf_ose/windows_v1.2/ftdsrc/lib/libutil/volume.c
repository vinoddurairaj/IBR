#include <windows.h>
#include <stdio.h>
#include <winioctl.h>

#include "list.h"
#include "misc.h"

#include "ftd_devlock.h"

unsigned long getDriveid(HANDLE	 hVolume)
{
	unsigned long serial = -1;
	DWORD dwRes, dwBytesRead;
	DWORD dwSize = (sizeof(DWORD) * 2) + (128 * sizeof(PARTITION_INFORMATION));
	DRIVE_LAYOUT_INFORMATION *drive = (struct _DRIVE_LAYOUT_INFORMATION *)malloc(dwSize);
	
	dwRes = DeviceIoControl(  hVolume,  // handle to a device
		IOCTL_DISK_GET_DRIVE_LAYOUT, // dwIoControlCode operation to perform
		NULL,                        // lpInBuffer; must be NULL
		0,                           // nInBufferSize; must be zero
		drive,						// pointer to output buffer
		dwSize,      // size of output buffer
		&dwBytesRead,				// receives number of bytes returned
		NULL						// pointer to OVERLAPPED structure); 
	);

	if (dwRes)
		serial = drive->Signature;

	free(drive);

	return serial;
}

void getDiskSigAndInfo(char *szDir, char *szDiskInfo, int iGroupID)
{
	HANDLE			hVolume;
	unsigned long	ulDriveID = 0;
	PARTITION_INFORMATION ppi;
	char			szStartingOffset[100];
	char			szPartitionLength[100];
	BOOL			bLocked = FALSE;
	char			szDirParse[10];
	
	memset(szDirParse, 0, sizeof(szDirParse));
	memset(szDiskInfo, 0, sizeof(szDiskInfo));
	memset(szStartingOffset, 0, sizeof(szStartingOffset));
	memset(szPartitionLength, 0, sizeof(szPartitionLength));

	strcpy(szDirParse, szDir);

	if ( (hVolume = OpenAVolume(szDir, GENERIC_READ)) == INVALID_HANDLE_VALUE )
	{
		szDirParse[strlen(szDirParse) - 1] = 0;
		// Get the right logical group
		if((hVolume = ftd_dev_lock(szDirParse, iGroupID)) == INVALID_HANDLE_VALUE )
		{
			return;
		}
		else
		{
			bLocked = TRUE;
		}
	}

	if ( (ulDriveID = getDriveid(hVolume)) == (unsigned long)-1)
	{
		 // ardeb 020917 v
		ulDriveID = 0000000000;
		/*
		if(!bLocked)
		{
			CloseVolume(hVolume);
		}
		else
		{
			ftd_dev_unlock(hVolume);
		}

		return;
		*/
		 // ardeb 020917 ^
	}

	if ( !GetVolumePartitionInfo(hVolume, &ppi) )
	{
		 // ardeb 020917 v
		ppi.StartingOffset.QuadPart  = 00000000000;
		ppi.PartitionLength.QuadPart = 000000000;

		/*
		if(!bLocked)
		{
			CloseVolume(hVolume);
		}
		else
		{
			ftd_dev_unlock(hVolume);
		}

		return;
		*/
		 // ardeb 020917 ^
	}

//
// SVG 30-05-03
// 
#ifdef NEW_DISK_SIZE_METHOD
#pragma message ("NEW disk size method")    
    //
    // Patch to always get the actual size returned by windows explorer instead
    // of "real" disk extents...
    //
    {
        ULARGE_INTEGER      uliFreeBytesAvail,
                            uliTotalBytes,
                            uliTotalFree;

        if (GetDiskFreeSpaceEx(szDir,&uliFreeBytesAvail,&uliTotalBytes,&uliTotalFree))
        {
            ppi.PartitionLength.QuadPart = uliTotalBytes.QuadPart;
        }
    }
#endif

	if(!bLocked)
	{
		CloseVolume(hVolume);
	}
	else
	{
		ftd_dev_unlock(hVolume);
	}

	_i64toa(ppi.StartingOffset.QuadPart, szStartingOffset, 10);
	_i64toa(ppi.PartitionLength.QuadPart, szPartitionLength, 10);
	
	sprintf(szDiskInfo, " %u %s %s", ulDriveID, szStartingOffset, szPartitionLength);
}

unsigned long gethostid(void)
{
	HANDLE hVolume;
	unsigned long serial = -1;
	char szWindowsDir[_MAX_PATH];
	DWORD dwRes, dwBytesRead;
	DWORD dwSize = (sizeof(DWORD) * 2) + (128 * sizeof(PARTITION_INFORMATION));
	DRIVE_LAYOUT_INFORMATION *drive = malloc(dwSize);
	
	if ( !GetWindowsDirectory(szWindowsDir, sizeof(szWindowsDir)) )
		return -1;

	hVolume = OpenAVolume(szWindowsDir, GENERIC_READ);

	dwRes = DeviceIoControl(  hVolume,  // handle to a device
		IOCTL_DISK_GET_DRIVE_LAYOUT, // dwIoControlCode operation to perform
		NULL,                        // lpInBuffer; must be NULL
		0,                           // nInBufferSize; must be zero
		drive,						// pointer to output buffer
		dwSize,      // size of output buffer
		&dwBytesRead,				// receives number of bytes returned
		NULL						// pointer to OVERLAPPED structure); 
	);


	CloseVolume(hVolume);

	if (dwRes)
		serial = drive->Signature;

	free(drive);

	return serial;
}

BOOL GetVolumePartitionInfo(HANDLE hVolume, PPARTITION_INFORMATION ppi)
{
	DWORD dwBytesReturned=0;

	if (!DeviceIoControl(hVolume,
				IOCTL_DISK_GET_PARTITION_INFO, NULL, 0,
				ppi, sizeof(PARTITION_INFORMATION), &dwBytesReturned, 
				(LPOVERLAPPED) NULL )) {
		DWORD err = GetLastError();
		
		return FALSE;
	}

#ifdef _DEBUG
    {
        char dbgbuf[256];
        sprintf(dbgbuf, "GetVolumePartitionInfo(0x%x): high =%ld low=%ld sectors hidden %ld\n", hVolume, ppi->PartitionLength.HighPart, ppi->PartitionLength.LowPart, ppi->HiddenSectors);
        OutputDebugString(dbgbuf);
    }
#endif

	return TRUE;
}

BOOL GetVolumeDiskGeometry(HANDLE hVolume, DISK_GEOMETRY *geo)
{
	DWORD dwBytesReturned=0;

	if (!DeviceIoControl(hVolume,
				IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
				geo, sizeof(DISK_GEOMETRY), &dwBytesReturned, 
				(LPOVERLAPPED) NULL )) {
		return FALSE;
	}

#ifdef _DEBUG
    {
        char dbgbuf[256];
        sprintf(dbgbuf, "ftd_get_dev_bsize(0x%x): %ld size\n", hVolume, geo->BytesPerSector);
        OutputDebugString(dbgbuf);
    }
#endif
	return TRUE;
}

HANDLE OpenAVolume(char *cDriveLetter, DWORD dwAccessFlags)
{       
	HANDLE hVolume;
	char szVolumeName[8];       

	sprintf(szVolumeName, TEXT("\\\\.\\%c:"), cDriveLetter[0]);
	hVolume = CreateFile(   szVolumeName,
		dwAccessFlags,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING, 0,
		NULL );

	return hVolume;   
}

BOOL CloseVolume(HANDLE hVolume)   
{       
	return CloseHandle(hVolume);
}

BOOL LockVolume(HANDLE hVolume)
{
	DWORD dwBytesReturned=0;
	if (DeviceIoControl(hVolume,
			FSCTL_LOCK_VOLUME,
			NULL, 0,
			NULL, 0,
			&dwBytesReturned,
			NULL))
		return TRUE;
	
	return FALSE;
}

BOOL UnLockVolume(HANDLE hVolume)
{
	DWORD dwBytesReturned=0;

	if (DeviceIoControl(hVolume,
			FSCTL_UNLOCK_VOLUME,
			NULL, 0,
			NULL, 0,
			&dwBytesReturned,
			NULL))
		return TRUE;

	return FALSE;
}

BOOL DismountVolume(HANDLE hVolume)
{
	DWORD dwBytesReturned=0;

	return DeviceIoControl( hVolume,
		FSCTL_DISMOUNT_VOLUME,
		NULL, 0,
		NULL, 0,
		&dwBytesReturned,
		NULL);
}

BOOL MountVolume(HANDLE hVolume)
{
	if ( !UnLockVolume(hVolume) ) {
		CloseVolume(hVolume);
		return FALSE;
	}

	return TRUE;
}

HANDLE AttachAVolume(char *cDriveLetter)
{
	HANDLE hVolume;
	BOOLEAN bDismounted=FALSE;

	hVolume = OpenAVolume(cDriveLetter, GENERIC_READ);

	return hVolume;
}

HANDLE LockAVolume(char *cDriveLetter)
{
	HANDLE hVolume;
	BOOLEAN bLocked=FALSE;

	hVolume = OpenAVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE);
	if (hVolume == INVALID_HANDLE_VALUE) {
		return hVolume;
	}

	// Lock the volume.
	if (LockVolume(hVolume)) {
		bLocked=TRUE;
	}

	// if the media was dismounted ok, return true
	if (!bLocked)
	{
		CloseVolume(hVolume);

		return INVALID_HANDLE_VALUE;
	}

	return hVolume;
}

HANDLE DismountAVolume(char *cDriveLetter)
{
	HANDLE hVolume;

	hVolume = OpenAVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE);
	if (hVolume == INVALID_HANDLE_VALUE) {
		return hVolume;
	}

	// Lock and dismount the volume.
	if (!DismountVolume(hVolume)) {
		CloseVolume(hVolume);

		return INVALID_HANDLE_VALUE;
	}

	return hVolume;
}

BOOL CreateAVolume(char *cDriveLetter, char * NTPath)
{
	char szDosDeviceName[80];

    sprintf(szDosDeviceName, "%c:", cDriveLetter[0]);

#ifdef _DEBUG
    {
        char dbgbuf[256];
        sprintf(dbgbuf, "CreateAVolume: DosDeviceName[%s] Path[%s]\n", szDosDeviceName, NTPath );
        OutputDebugString(dbgbuf);
    }
#endif

    if (!DefineDosDevice(DDD_RAW_TARGET_PATH, szDosDeviceName, NTPath)) {
        return FALSE;
    }

    return TRUE;
}

BOOL DeleteAVolume(char *cDriveLetter)
{
	HANDLE hVolume;
	BOOLEAN bDismounted=FALSE;
	char szDosDeviceName[80];

	hVolume = OpenAVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE);
	if (hVolume == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	// Lock and dismount the volume.
	if (!LockVolume(hVolume) || !DismountVolume(hVolume)) {
		CloseVolume(hVolume);
		return FALSE;
	}

	sprintf(szDosDeviceName, "%c:", cDriveLetter[0]);
	if ( !DefineDosDevice(DDD_REMOVE_DEFINITION, szDosDeviceName, NULL) ) {
		CloseVolume(hVolume);
		return FALSE;
	}

	if ( !UnLockVolume(hVolume) ) {
		CloseVolume(hVolume);
		return FALSE;
	}

	CloseVolume(hVolume);

	return TRUE;
}

BOOL sync(char *cDriveLetter)
{
	HANDLE hVolume;

	hVolume = OpenAVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE);
	if (hVolume == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	if (FlushFileBuffers( hVolume ) == 0) {
		return FALSE;
	}

	CloseVolume(hVolume);

	return TRUE;
}
