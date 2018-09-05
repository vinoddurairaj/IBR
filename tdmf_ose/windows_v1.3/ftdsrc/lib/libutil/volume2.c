/*
 * volume2.c - TDMF Management Notification Event Protocol
 *
 * This file was created to extract function getDiskSigAndInfo()
 * from volume.c , to avoid Link-time libraires cascade problems
 * in projects not linking with libftd.lib and liblst.lib
 *
 * Copyright (c) 2002 Fujitsu Softek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#include <windows.h>
#include <stdio.h>
#include <winioctl.h>

#include "list.h"
#include "misc.h"
#include "volmntpt.h"
#include "ftd_devlock.h"

//volume.c
unsigned long getDriveid(HANDLE  hVolume);

//
// This debug buffer is used in both _DEBUG and _TRACE_VOLUMES
//
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
static char DbgMsgBuf[256];
#endif


//
//  Return the first part of Volume GUID as Disk ID , WIN2K ONLY
//
//
// This function accepts either a GUID or the mount point directory
// as the IN parameter...
//
//
// If szDrive is a mountpoint directory, we get the GUID before 
// returning the partial GUID...
//
unsigned long getDiskIdFromGuid( char *szDrive )
{
    char    szVolumeName[MAX_PATH];
    char    szDiskId[11];
    char    *szGuidPart;

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"getDiskIdFromGuid for %s\n",szDrive);
        OutputDebugString(DbgMsgBuf);
#endif

    if ( !IS_VALID_VOLUME_NAME( szDrive ) )
    {
        memset(szVolumeName, 0, MAX_PATH);

        if (!getVolumeNameForVolMntPt(  szDrive,   /* ex H:\MountPoint1\ or C:\ */
                                        szVolumeName,  
                                        MAX_PATH ))
        {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
            sprintf(DbgMsgBuf,"getDiskIdFromGuid failed (getVolumeNameForVolMntPt)\n");
            OutputDebugString(DbgMsgBuf);
#endif
            return 0;
        }
    
        szGuidPart = strrchr(szVolumeName, '{');
    }
    else
    {
        szGuidPart = strrchr(szDrive, '{');
    }

    memset(szDiskId, 0, 11);
    strncpy( szDiskId, "0x", 2);
    strncat( szDiskId, ++szGuidPart, 8 );

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"getDiskIdFromGuid returned [0x%08x]\n", strtoul(szDiskId, NULL,0));
    OutputDebugString(DbgMsgBuf);
#endif


    return ( strtoul( szDiskId , NULL, 0 ) );
}

//
// the ioctl IOCTL_STORAGE_GET_DEVICE_NUMBER
// only works on basic volumes
// 
// So if the call succeeds, 
// this is a basic volume
//
// However, because in the future this ioctl can succeed,
// we also add another 2nd test to check for the first
// partition on a volume. If that partition is type 0x42,
// all partitions on that drive are DYNAMIC
//
unsigned int IsDiskBasic(char *szDrive)
{
    //
    // return 0xFFFFFFFF on errors!
    //
    unsigned int                uiBytesReturned     = 0;    
    char                        cInOutBuffer[2048];          
    char                        cDeviceName[512];   
                                                            
    HANDLE                      hVolume;
    HANDLE                      hDrive;

    PDRIVE_LAYOUT_INFORMATION   pDriveLayout        = NULL;
    STORAGE_DEVICE_NUMBER       StorageNumber;
    unsigned int                uiDriveNumber       = 0xFFFFFFFF;


#if DBG
    char                        DbgChar[255];

    //
    // Dump the name of the drive we are checking
    //
    sprintf(DbgChar,"\nIsDiskBasic\nAnalyzing Drive %s\n",szDrive);
    OutputDebugString(DbgChar);
#endif

    //
    // Clear devioctl buffer
    //
    memset(cInOutBuffer, 0, sizeof(cInOutBuffer));

    if ( (hVolume = OpenAVolumeMountPoint(szDrive, GENERIC_READ)) == INVALID_HANDLE_VALUE )
    {
        //
        // Unable to open the volume! Return an error!
        //

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"IsDiskBasic Failed (OpenAVolumeMountPoint)\n");
        OutputDebugString(DbgMsgBuf);
#endif

        return 0xFFFFFFFF;
    }

    //
    // Try to get the device number using the ioctl, and the correct size of the structure
    //
    if ( DeviceIoControl(   hVolume,
                            IOCTL_STORAGE_GET_DEVICE_NUMBER,
                            NULL, 
                            0,
                            &StorageNumber, 
                            sizeof(STORAGE_DEVICE_NUMBER), 
                            &uiBytesReturned, 
                            (LPOVERLAPPED) NULL )               ) 
    {
        uiDriveNumber = StorageNumber.DeviceNumber;
#if DBG
        //
        // Dump Device Number
        //
        sprintf(DbgChar,"Device Number %ld\n",
                uiDriveNumber   );
        OutputDebugString(DbgChar);
#endif
    }
    else
    {
        //
        // If this call does not succeed, this is a DYNAMIC volume!
        //
#if DBG
        sprintf(DbgChar,"Unable to complete IOCTL_STORAGE_GET_DEVICE_NUMBER - error [%ld]\n",GetLastError());
        OutputDebugString(DbgChar);

        sprintf(DbgChar,"[%s] is a Dynamic drive\n",szDrive);
        OutputDebugString(DbgChar);
#endif
        
        CloseVolume(hVolume);
        return 0;
    }

    //
    // Try to get the drive layout of the drive number returned above...
    //
    if (uiDriveNumber!=0xFFFFFFFF)
    {
        //
        // The name of the drive must be of type : \\.\PHYSICALDRIVEx
        //                  
        sprintf(cDeviceName,"\\\\.\\Physicaldrive%ld",uiDriveNumber);

        //
        // Open the drive in share mode, we only want to check it's attributes
        //
        hDrive = CreateFile(cDeviceName, // drive to open
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (hDrive != INVALID_HANDLE_VALUE)
        {
            //
            // Clear devioctl buffer
            //
            memset(cInOutBuffer, 0, sizeof(cInOutBuffer));

            if ( DeviceIoControl(   hDrive,
                                    IOCTL_DISK_GET_DRIVE_LAYOUT,
                                    NULL, 
                                    0,
                                    &cInOutBuffer, 
                                    sizeof(cInOutBuffer), 
                                    &uiBytesReturned, 
                                    (LPOVERLAPPED) NULL )               ) 
            {
                pDriveLayout = (PDRIVE_LAYOUT_INFORMATION) cInOutBuffer;
#if DBG
                sprintf(DbgChar,"[%s] IOCTL_DISK_GET_DRIVE LAYOUT successful\n",cDeviceName);
                OutputDebugString(DbgChar);

                sprintf(DbgChar,"Partition type [0x%08X]\n",
                        pDriveLayout->PartitionEntry[0].PartitionType );
                OutputDebugString(DbgChar);
#endif
                //
                // Verify for DYNAMIC drive!
                //
                // Validate all the pointers! 
                // We don't want to access invalid areas...
                //
                if (    (uiBytesReturned)
                    &&  (pDriveLayout)
                    &&  (pDriveLayout->PartitionCount)
                    &&  (0x42 == pDriveLayout->PartitionEntry[0].PartitionType) )
                {
                    // 
                    // 0x42 on first partition means DYNAMIC!
                    // 
                    CloseHandle(hDrive);
                    CloseVolume(hVolume);
           
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
                    sprintf(DbgMsgBuf,"IsDiskBasic read 0x42 on first partition!\n");
                    OutputDebugString(DbgMsgBuf);
#endif
                    return 0;
                    
                }

            }

            CloseHandle(hDrive);

        }
        else
        {
#if DBG
            sprintf(DbgChar,"Unable to open the handle to the physical drive %s\n",cDeviceName);
            OutputDebugString(DbgChar);

            sprintf(DbgChar,"Unable to complete CreateFile - error [%ld]\n",GetLastError());
            OutputDebugString(DbgChar);
#endif
        }
    }

#if DBG
    sprintf(DbgChar,"[%s] is a Basic drive\n",szDrive);
    OutputDebugString(DbgChar);
#endif

    //
    // This is a basic drive!
    //
    CloseVolume(hVolume);   
    return 1;
}


void getDiskSigAndInfo(char *szDir, char *szDiskInfo, int iGroupID)
{
    HANDLE          hVolume;
    unsigned long   ulDriveID = 0;
    PARTITION_INFORMATION ppi;
    char            szStartingOffset[100];
    char            szPartitionLength[100];
    BOOL            bLocked = FALSE;
    char            szDirParse[10];
    char            szVolumeName[8];       

    memset(szDirParse, 0, sizeof(szDirParse));
    memset(szVolumeName, 0, sizeof(szVolumeName));
    memset(szDiskInfo, 0, sizeof(szDiskInfo));
    memset(szStartingOffset, 0, sizeof(szStartingOffset));
    memset(szPartitionLength, 0, sizeof(szPartitionLength));

    strcpy(szDirParse, szDir);

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"getDiskSigAndInfo for %s\n", szDir);
    OutputDebugString(DbgMsgBuf);
#endif

    if ( (hVolume = OpenAVolume(szDir, GENERIC_READ)) == INVALID_HANDLE_VALUE )
    {
        // Get the right logical group
        if((hVolume = ftd_dev_lock(szDirParse, iGroupID)) == INVALID_HANDLE_VALUE )
        {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
            sprintf(DbgMsgBuf,"GetDiskSigAndInfo Failed (ftd_dev_lock)\n");
            OutputDebugString(DbgMsgBuf);
#endif
            return;
        }
        else
        {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
            sprintf(DbgMsgBuf,"GetDiskSigAndInfo worked(ftd_dev_lock)\n");
            OutputDebugString(DbgMsgBuf);
#endif
            bLocked = TRUE;
        }
    }

    if ( (ulDriveID = getDriveid(hVolume)) == (unsigned long)-1)
    {
#if !defined(NTFOUR) // Not supported in NT4 OS 
        sprintf(szVolumeName, TEXT("%c:\\"), szDir[0]);
        ulDriveID = getDiskIdFromGuid( szVolumeName );
#else
         // ardeb 020917 v
        ulDriveID = 0000000000;
#endif
    }

    if ( !GetVolumePartitionInfo(hVolume, &ppi) )
    {
         // ardeb 020917 v
        ppi.StartingOffset.QuadPart  = 00000000000;
        ppi.PartitionLength.QuadPart = 000000000;

    }

//
// SVG 30-05-03
//+++ 
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
//---

    if(bLocked)
    {
        ftd_dev_unlock(hVolume);
    }
    else
    {
        CloseVolume(hVolume);
    }

    _i64toa(ppi.StartingOffset.QuadPart, szStartingOffset, 10);
    _i64toa(ppi.PartitionLength.QuadPart, szPartitionLength, 10);

    sprintf(szDiskInfo, " %u %s %s", ulDriveID, szStartingOffset, szPartitionLength);

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"getDiskSigAndInfo for %s returned %s\n", szDir, szDiskInfo);
    OutputDebugString(DbgMsgBuf);
#endif

}

BOOL getMntPtSigAndInfo(char *szDir, char *szDiskInfo, int iGroupID)
{
    HANDLE          hVolume;
    unsigned long   ulDriveID = 0;
    PARTITION_INFORMATION ppi;
    char            szStartingOffset[100];
    char            szPartitionLength[100];
    BOOL            bLocked = FALSE;
    char            szDirParse[10];
    char            szGUID[MAX_PATH];
    
    memset(szDirParse,          0, sizeof(szDirParse));
    memset(szDiskInfo,          0, sizeof(szDiskInfo));
    memset(szStartingOffset,    0, sizeof(szStartingOffset));
    memset(szPartitionLength,   0, sizeof(szPartitionLength));
    memset(szGUID,              0, MAX_PATH);

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"getMntPtSigAndInfo for %s\n", szDir);
        OutputDebugString(DbgMsgBuf);
#endif

    if (!getVolumeNameForVolMntPt( szDir,     
                                   szGUID,     
                                   MAX_PATH ))       
    {
#if DBG
        OutputDebugString("getMntPtSigAndInfo Failed (getVolumeNameForVolMntPt)\n");
#endif
        return FALSE;
    }

    szGUID[1] = '\\';
    szGUID[48] = 0;

    //
    // szGUID format : \\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    //

    if ( (hVolume = OpenAVolumeMountPoint(szGUID, GENERIC_READ)) == INVALID_HANDLE_VALUE ) 
    {
        ulDriveID = 0000000000;
        ppi.StartingOffset.QuadPart  = 00000000000;
        ppi.PartitionLength.QuadPart = 000000000;
    }
    else
    {
        if ( (ulDriveID = getDriveid(hVolume)) == (unsigned long)-1) 
        {
#if !defined(NTFOUR) // Not supported in NT4 OS 
            ulDriveID = getDiskIdFromGuid( szGUID );
#else
            // ardeb 020917 v
            ulDriveID = 0000000000;
#endif
        }

        if ( !GetVolumePartitionInfo(hVolume, &ppi) ) 
        {
             // ardeb 020917 v
            ppi.StartingOffset.QuadPart  = 00000000000;
            ppi.PartitionLength.QuadPart = 000000000;
        }
    }

//
// SVG 30-05-03
// +++
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
// ---

    if (        hVolume 
            && (INVALID_HANDLE_VALUE!=hVolume)  )
    {
        //
        // Only close a valid handle
        //
        CloseVolume(hVolume);
    }

    _i64toa(ppi.StartingOffset.QuadPart, szStartingOffset, 10);
    _i64toa(ppi.PartitionLength.QuadPart, szPartitionLength, 10);
    
    sprintf(szDiskInfo, " %u %s %s", ulDriveID, szStartingOffset, szPartitionLength);

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"getMntPtSigAndInfo for %s returned %s\n",szDir, szDiskInfo);
    OutputDebugString(DbgMsgBuf);
#endif


    return TRUE;
}

unsigned long getDeviceNameSymbolicLink( char *lpDeviceName, char *lpTargetPath, unsigned long ucchMax )
{
    unsigned long result = 0;
    char szVolumePath[MAX_PATH];

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"getDeviceNameSymbolicLink for %s\n", lpDeviceName);
    OutputDebugString(DbgMsgBuf);
#endif


    if ( strlen(lpDeviceName) == 2 )
    {
        result = QueryDosDevice(lpDeviceName, lpTargetPath, ucchMax);
    }
    else 
    {

        // Must be a Mount Point  format:  H:\MyMountPoint1\ //
        if (!getVolumeNameForVolMntPt( lpDeviceName,     // input volume mount point or directory
                                       szVolumePath,     // output volume name buffer
                                       MAX_PATH ))       // size of volume name buffer
        {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
        sprintf(DbgMsgBuf,"getDeviceNameSymbolicLink Failed (getVolumeNameForVolMntPt)\n");
        OutputDebugString(DbgMsgBuf);
#endif
            return 0;
        }
       
        //  Volume GUID format:  \??\Volume{....} //
        szVolumePath[1] = '?';
        szVolumePath[48] = 0;

        if( QueryVolMntPtInfoFromDevName( szVolumePath, MAX_PATH, lpTargetPath, MAX_PATH, DEVICE_NAME) != VALID_MNT_PT_INFO )
        {
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
            sprintf(DbgMsgBuf,"getDeviceNameSymbolicLink Failed (QueryVolMntPtInfoFromDevName)\n");
            OutputDebugString(DbgMsgBuf);
#endif
            return 0;
        }

        result = strlen(lpTargetPath);
    }

#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
    sprintf(DbgMsgBuf,"getDeviceNameSysmbolicLink returned %s [0x%08x] err=[0x%08x]\n",lpTargetPath, result, (result == 0) ? GetLastError() : 0);
    OutputDebugString(DbgMsgBuf);
#endif

    return(result);
}

