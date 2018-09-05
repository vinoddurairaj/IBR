#include <windows.h>
#include <stdio.h>
#include <winioctl.h>

#include "list.h"
#include "misc.h"
#include "volmntpt.h"

//#include <string.h>
//#include "c:\ntddk\inc\mountmgr.h"


//#define _TRACE_VOLUMES

//
// This debug buffer is used in both _DEBUG and _TRACE_VOLUMES
//
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
static char DbgMsgBuf[256];
#endif


#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning Msg: "

#ifdef _TRACE_VOLUMES
#pragma message(__LOC__ "Including TRACE_VOLUMES")
#endif

unsigned long getDriveid(HANDLE  hVolume)
{
    unsigned long serial = -1;
    DWORD dwRes, dwBytesRead;
    DWORD dwSize = (sizeof(DWORD) * 2) + (128 * sizeof(PARTITION_INFORMATION));
    DRIVE_LAYOUT_INFORMATION *drive = (struct _DRIVE_LAYOUT_INFORMATION *)malloc(dwSize);
    
    dwRes = DeviceIoControl(  hVolume,  // handle to a device
        IOCTL_DISK_GET_DRIVE_LAYOUT, // dwIoControlCode operation to perform
        NULL,                        // lpInBuffer; must be NULL
        0,                           // nInBufferSize; must be zero
        drive,                      // pointer to output buffer
        dwSize,      // size of output buffer
        &dwBytesRead,               // receives number of bytes returned
        NULL                        // pointer to OVERLAPPED structure); 
    );

    if (dwRes)
        serial = drive->Signature;

    free(drive);

#ifdef _DEBUG
    sprintf(DbgMsgBuf, "getDriveid Handle = [0x%08x]\n", hVolume);
    OutputDebugString(DbgMsgBuf);
#endif

    return serial;
}



BOOL GetVolumePartitionInfo(HANDLE hVolume, PPARTITION_INFORMATION ppi)
{
    DWORD dwBytesReturned=0;

    if (!DeviceIoControl(hVolume,
                IOCTL_DISK_GET_PARTITION_INFO, NULL, 0,
                ppi, sizeof(PARTITION_INFORMATION), &dwBytesReturned, 
                (LPOVERLAPPED) NULL )) 
    {
        DWORD err = GetLastError();

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"GetVolumePartitionInfo for [0x%08x] returned [0x%08x]\n", hVolume, err);
        OutputDebugString(DbgMsgBuf);
#endif
        
        return FALSE;
    }

#ifdef _DEBUG
    sprintf(DbgMsgBuf, "GetVolumePartitionInfo Handle = [0x%08x]: high =%ld low=%ld sectors hidden %ld\n", hVolume, ppi->PartitionLength.HighPart, ppi->PartitionLength.LowPart, ppi->HiddenSectors);
    OutputDebugString(DbgMsgBuf);
#endif

    return TRUE;
}

BOOL GetVolumeDiskGeometry(HANDLE hVolume, DISK_GEOMETRY *geo)
{
    DWORD dwBytesReturned=0;

    if (!DeviceIoControl(hVolume,
                IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                geo, sizeof(DISK_GEOMETRY), &dwBytesReturned, 
                (LPOVERLAPPED) NULL )) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"GetVolumeDiskGeometry failed\n");
        OutputDebugString(DbgMsgBuf);
#endif

        return FALSE;
    }

#ifdef _DEBUG
    sprintf(DbgMsgBuf, "GetVolumeDiskGeometry Handle = [0x%08x]: %ld size\n", hVolume, geo->BytesPerSector);
    OutputDebugString(DbgMsgBuf);
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

#ifdef _DEBUG
    sprintf(DbgMsgBuf, "OpenAVolume DriveLetter [%c] Volume [%s] Handle = [0x%08x]\n", cDriveLetter[0],szVolumeName, hVolume);
    OutputDebugString(DbgMsgBuf);
#endif

    return hVolume;   
}

HANDLE OpenAVolumeMountPoint(char *szVolumeMntPt, DWORD dwAccessFlags)
{       
    //
    // szVolumeName format : \\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    // or           format : H:\MountPoint1\
    //
    HANDLE hVolume;
    char szVolumeName[MAX_PATH];

    memset(szVolumeName, 0, MAX_PATH);

    if ( !IS_VALID_VOLUME_NAME(szVolumeMntPt) )
    {
        if (!getVolumeNameForVolMntPt( szVolumeMntPt,    // input volume mount point or directory
                                       szVolumeName,     // output volume name buffer
                                       MAX_PATH ))       // size of volume name buffer
              {
#ifdef _DEBUG
                  sprintf(DbgMsgBuf, "OpenAVolumeMountPoint Failed for [%s]\nAccessflags=[0x%08x]\n", szVolumeMntPt, dwAccessFlags);
                  OutputDebugString(DbgMsgBuf);
#endif
                  return INVALID_HANDLE_VALUE;
               }

              szVolumeName[48] = 0;
    }
    else
          {
              strcpy(szVolumeName, szVolumeMntPt);
          }

    hVolume = CreateFile( szVolumeName,
                          dwAccessFlags,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING, 0,
                          NULL );
#ifdef _DEBUG
    sprintf(DbgMsgBuf, "OpenAVolumeMountPoint for [%s]\nVolName [%s]\nAccessFlags=[0x%08x]\nHandle = [0x%08x]\n", szVolumeMntPt, szVolumeName, dwAccessFlags , hVolume);
    OutputDebugString(DbgMsgBuf);
#endif

    return hVolume;
}

BOOL CloseVolume(HANDLE hVolume)   
{       
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"CloseVolume Handle = [0x%08x]\n",hVolume);
    OutputDebugString(DbgMsgBuf);
#endif
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
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"LockVolume Handle = [0x%08x] LOCKED\n",hVolume);
        OutputDebugString(DbgMsgBuf);
#endif
        return TRUE;
    }
    else
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"LockVolume Handle = [0x%08x] NOT LOCKED [0x%08x]\n",hVolume,GetLastError());
        OutputDebugString(DbgMsgBuf);
#endif
        return FALSE;
    }
}

//
// SVG2004: ADDED:
// Flush disk before unlocking
// Check for valid handle
//
BOOL UnLockVolume(HANDLE hVolume)
{
    DWORD dwBytesReturned=0;

    if ((!hVolume) || (hVolume == INVALID_HANDLE_VALUE))
    {
        return FALSE;
    }

    FlushFileBuffers(hVolume);

    if (DeviceIoControl(hVolume,
            FSCTL_UNLOCK_VOLUME,
            NULL, 0,
            NULL, 0,
            &dwBytesReturned,
            NULL))
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"UnLockVolume Handle = [0x%08x] UNLOCKED\n",hVolume);
        OutputDebugString(DbgMsgBuf);
#endif
        return TRUE;
    }
    else
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"UnLockVolume Handle = [0x%08x] NOT UNLOCKED\n",hVolume);
        OutputDebugString(DbgMsgBuf);
#endif
        return FALSE;
    }
}

BOOL DismountVolume(HANDLE hVolume)
{
    DWORD dwBytesReturned=0;

    if (DeviceIoControl( hVolume,
        FSCTL_DISMOUNT_VOLUME,
        NULL, 0,
        NULL, 0,
        &dwBytesReturned,
        NULL))
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DismountVolume Handle = [0x%08x] DISMOUNTED\n",hVolume);
        OutputDebugString(DbgMsgBuf);
#endif
        return TRUE;
    }
    else
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DismountVolume Handle = [0x%08x] NOT DISMOUNTED\n",hVolume);
        OutputDebugString(DbgMsgBuf);
#endif
        return FALSE;
    }
}


BOOL MountVolume(HANDLE hVolume)
{
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"MountVolume Handle = [0x%08x]\n",hVolume);
    OutputDebugString(DbgMsgBuf);
#endif

    if ( !UnLockVolume(hVolume) ) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"MountVolume Handle = [0x%08x] Unable to Unlock Volume!\n",hVolume);
        OutputDebugString(DbgMsgBuf);
#endif

        CloseVolume(hVolume);
        return FALSE;
    }

    return TRUE;
}

//
//  Remount the volume... Make sure volume is actually mounted...
//                        Try reading a few chars for a while...
//
BOOL CheckVolumeMounted(HANDLE hVolume)
{
    BYTE    bReadBuffer[2048];
    DWORD   dwReturnedBytes = 0;
    DWORD   dwRetry         = 50;

    //
    // Mount file system
    //
    GetLogicalDrives();

    //
    // Set & Read
    // 
    SetFilePointer(hVolume, 0, NULL, FILE_BEGIN);

    while(!ReadFile(hVolume,bReadBuffer,2048,&dwReturnedBytes, NULL) && --dwRetry)
    {
        SetFilePointer(hVolume, 0, NULL, FILE_BEGIN);
        Sleep(100);
    }
    
    if (!dwRetry)
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"MountVolume Read did not work [0x%08x]\n",GetLastError());
        OutputDebugString(DbgMsgBuf);
#endif
        return FALSE;
    }
    else
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"CheckVolumeMounted read worked B Read=[0x%08x] retry=[0x%08x]\n",dwReturnedBytes, dwRetry);
        OutputDebugString(DbgMsgBuf);
#endif
        return TRUE;
    }

}



HANDLE AttachAVolume(char *cDriveLetter)
{
    HANDLE hVolume;
    BOOLEAN bDismounted=FALSE;

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"AttachAVolume %c\n",*cDriveLetter);
    OutputDebugString(DbgMsgBuf);
#endif

    hVolume = OpenAVolume(cDriveLetter, GENERIC_READ);

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"AttachAVolume Handle = [0x%08x]\n",hVolume);
    OutputDebugString(DbgMsgBuf);
#endif

    return hVolume;
}


HANDLE LockAVolume(char *cDriveLetter)
{
    HANDLE hVolume;
    BOOLEAN bLocked=FALSE;

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"LockAVolume %c\n",*cDriveLetter);
    OutputDebugString(DbgMsgBuf);
#endif


    hVolume = OpenAVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE);

    if (hVolume == INVALID_HANDLE_VALUE) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"LockAVolume (media not opened)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        return hVolume;
    }

    // Lock the volume.
    if (LockVolume(hVolume)) 
    {
        bLocked=TRUE;
    }

    // if the media was dismounted ok, return true
    if (!bLocked)
    {
        CloseVolume(hVolume);

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"LockAVolume Failed (media not locked)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        return INVALID_HANDLE_VALUE;
    }

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"LockAVolume Success Handle = [0x%08x]\n",hVolume);
    OutputDebugString(DbgMsgBuf);
#endif

    return hVolume;
}


HANDLE DismountAVolume(char *cDriveLetter)
{
    HANDLE hVolume;

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"DismountAVolume %c\n", *cDriveLetter);
    OutputDebugString(DbgMsgBuf);
#endif

    hVolume = OpenAVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE);
    if (hVolume == INVALID_HANDLE_VALUE) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DismountAVolume Failed (unable to open volume)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        return hVolume;
    }
    // Lock and dismount the volume.
    if (!DismountVolume(hVolume)) 
    {
        CloseVolume(hVolume);

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DismountAVolume Failed (unable to dismount volume)\n");
        OutputDebugString(DbgMsgBuf);
#endif

        return INVALID_HANDLE_VALUE;
    }

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"DismountAVolume Success Handle = [0x%08x]\n",hVolume);
    OutputDebugString(DbgMsgBuf);
#endif
    return hVolume;
}


BOOL CreateAVolume(char *cDriveLetter, char * NTPath)
{
    char szDosDeviceName[80];

    sprintf(szDosDeviceName, "%c:",cDriveLetter[0]);

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"CreateAVolume DosDeviceName=%s\n",NTPath);
    OutputDebugString(DbgMsgBuf);
#endif

    if (!DefineDosDevice(DDD_RAW_TARGET_PATH, szDosDeviceName, NTPath)) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"CreateAVolume Failed (Unable to DefineDosDevice)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        return FALSE;
    }

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"CreateAVolume Success DosDevName = %s NtPath= %s\n",szDosDeviceName,NTPath);
    OutputDebugString(DbgMsgBuf);
#endif

    return TRUE;
}

BOOL DeleteAVolume(char *cDriveLetter) 
{
    HANDLE hVolume;
    BOOLEAN bDismounted=FALSE;
    char szDosDeviceName[80];

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"DeleteAVolume %c\n",*cDriveLetter);
    OutputDebugString(DbgMsgBuf);
#endif

    hVolume = OpenAVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE);
    if (hVolume == INVALID_HANDLE_VALUE) 
    {
#ifdef _DEBUG
        sprintf(DbgMsgBuf,"DeleteAVolume Failed (Unable to OpenAVolume)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        return FALSE;
    }

    // Lock and dismount the volume.
    if (!LockVolume(hVolume) || !DismountVolume(hVolume)) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DeleteAVolume Failed (Unable to Lock or Dismount the Volume)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        CloseVolume(hVolume);
        return FALSE;
    }

    sprintf(szDosDeviceName, "%c:", cDriveLetter[0]);
    if ( !DefineDosDevice(DDD_REMOVE_DEFINITION, szDosDeviceName, NULL) ) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DeleteAVolume Failed (Unable to DefineDosDevice)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        CloseVolume(hVolume);
        return FALSE;
    }

    if ( !UnLockVolume(hVolume) ) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DeleteAVolume Failed (Unable to UnLockVolume)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        CloseVolume(hVolume);
        return FALSE;
    }

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"DeleteAVolume Success DosDevice(deleted) = %s Handle = [0x%08x]\n",szDosDeviceName,hVolume);
    OutputDebugString(DbgMsgBuf);
#endif

    CloseVolume(hVolume);
    return TRUE;
}

BOOL CreateAMountPointVolume(char *szVolMntPnt, char *szDeviceName)
{
    char szVolumeName[MAX_PATH];

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"CreateAMountPointVolume for Device = %s\n",szDeviceName);
    OutputDebugString(DbgMsgBuf);
#endif

    if( QueryVolMntPtInfoFromDevName( szDeviceName, MAX_PATH, szVolumeName, MAX_PATH, VOLUME_GUID) != VALID_MNT_PT_INFO )
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"CreateAMountPointVolume Failed (QueryVolMntPtInfoFromDevName)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        return FALSE;
    }

    szVolumeName[1] = '\\';
    szVolumeName[48] = '\\';

    if (!setVolMntPnt( szVolMntPnt, szVolumeName )) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"CreateAMountPointVolume Failed (Unable to set volume mountpoint)\n");
        OutputDebugString(DbgMsgBuf);
#endif

        return FALSE;
    }

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"CreateAMountPointVolume Success\n");
    OutputDebugString(DbgMsgBuf);
#endif

    return TRUE;
}

BOOL DeleteAMountPointVolume(char *szMountPoint) 
{
    HANDLE hVolume;

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"DeleteAMountPointVolume MountPoint = %s\n",szMountPoint);
    OutputDebugString(DbgMsgBuf);
#endif

    hVolume = OpenAVolumeMountPoint(szMountPoint, GENERIC_READ | GENERIC_WRITE);
    if (hVolume == INVALID_HANDLE_VALUE) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DeleteAMountPointVolume Failed (OpenAVolumeMountPoint)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        return FALSE;
    }

    // Lock and dismount the volume.
    if (!LockVolume(hVolume) || !DismountVolume(hVolume)) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DeleteAMountPointVolume Failed (Unable to Lock or Dismount volume)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        CloseVolume(hVolume);
        return FALSE;
    }

    if ( !delVolMntPnt( szMountPoint )) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DeleteAMountPointVolume Failed (Unable to delete mountpoint)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        CloseVolume(hVolume);
        return FALSE;
    }

    if ( !UnLockVolume(hVolume) ) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"DeleteAMountPointVolume Failed (Unable to unlock volume)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        CloseVolume(hVolume);
        return FALSE;
    }

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"DeleteAMountPointVolume Success\n");
    OutputDebugString(DbgMsgBuf);
#endif

    CloseVolume(hVolume);
    return TRUE;
}

HANDLE LockAMountPointVolume(char *szMountPoint)
{
    HANDLE hVolume;
    BOOLEAN bLocked=FALSE;

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"LockAMountPointVolume MountPoint = %s\n",szMountPoint);
    OutputDebugString(DbgMsgBuf);
#endif

    hVolume = OpenAVolumeMountPoint(szMountPoint, GENERIC_READ | GENERIC_WRITE);
    if (hVolume == INVALID_HANDLE_VALUE) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"LockAMountPointVolume Failed (OpenAVolumeMountPoint)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        return hVolume;
    }

    // Lock the volume.
    if (LockVolume(hVolume)) 
    {
        bLocked=TRUE;
    }

    // if the media was dismounted ok, return true
    if (!bLocked)
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"LockAMountPointVolume Failed (Volume not locked)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        CloseVolume(hVolume);
        return INVALID_HANDLE_VALUE;
    }

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"LockAMountPointVolume Success\n");
    OutputDebugString(DbgMsgBuf);
#endif

    return hVolume;
}

BOOL sync(char *cDriveLetter)
{
    HANDLE hVolume;

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"sync %c\n",*cDriveLetter);
    OutputDebugString(DbgMsgBuf);
#endif

    hVolume = OpenAVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE);
    if (hVolume == INVALID_HANDLE_VALUE) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"sync Failed (OpenAVolume)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        return FALSE;
    }

    if (FlushFileBuffers( hVolume ) == 0) 
    {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"sync Failed (FlushFileBuffers)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        return FALSE;
    }

    CloseVolume(hVolume);

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"sync Success\n");
    OutputDebugString(DbgMsgBuf);
#endif
    return TRUE;
}

