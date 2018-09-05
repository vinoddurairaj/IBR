/*
 * volmntpt.c -- Volume Mount Points functions
 *
 * (c) Copyright Fujitsu-Softek 2003, Inc. All Rights Reserved
 *
 */


//Mike Pollett
#include "../../tdmf.inc"

/* Those HEADER files are in conflict with misc.h , NTFS_VOLUME_DATA_BUFFER definition */

//#if !defined(NTFOUR) // Not supported in NT4 OS 
//#define _WIN32_WINNT 0x0501
//#endif

#include <windows.h>
#include <stdio.h>
#include <winioctl.h>
#include <string.h>

#include "volmntpt.h"
#include "mountmgr.h"

//
// This debug buffer is used in both _DEBUG and _TRACE_VOLUMES
//
#if (defined(_DEBUG) || defined(_TRACE_VOLUMES))
static char DbgMsgBuf[256];
#endif


#if !defined(NTFOUR) // Not supported in NT4 OS 
int QueryVolMntPtInfoFromDevName(char *pszDeviceName, int piDevStrSize, char *pszMntPtInfo, int piGuidStrSize, 
                                 unsigned int piMntPtType)
{
    HANDLE hMntManager;
    int IoctlResult = 0;
    char DeviceBuf[ BUFSIZE ];
    unsigned char InputBuffer[ 4096 ]; 
    unsigned char OutputBuffer[ 16384 ]; 

    PMOUNTMGR_MOUNT_POINT pMountMgrPointBuffer = ( PMOUNTMGR_MOUNT_POINT )( &InputBuffer ); 
    PMOUNTMGR_MOUNT_POINTS pMountMgrPointsBuffer = ( PMOUNTMGR_MOUNT_POINTS )( &OutputBuffer );
 
    unsigned long sReturned = 0; 
    unsigned long Index = 0;
    char MountPointInfo[ MAX_PATH ];

    RtlZeroMemory( &InputBuffer, sizeof( InputBuffer ));
    RtlZeroMemory( &OutputBuffer, sizeof( OutputBuffer ));
    RtlZeroMemory( &DeviceBuf, BUFSIZE);

#ifdef _DEBUG
    sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName for DevName=%s\n",pszDeviceName);
    OutputDebugString(DbgMsgBuf);
#endif


    if ( ( piGuidStrSize > 0 ) && ( piGuidStrSize <=  BUFSIZE) ) 
    {
        RtlZeroMemory( pszMntPtInfo, piGuidStrSize );
    }
    else
    {
#ifdef _DEBUG
        sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName Failed (size of output string to small)\n");
        OutputDebugString(DbgMsgBuf);
#endif

        return ERROR_MNT_PT_INFO;
    }
    
    strncpy( DeviceBuf , pszDeviceName, piDevStrSize);

    //
    // Initialize the non persistent device name element of 
    // MOUNTMGR_MOUNT_POINT structure, MOUNTMGR.H
    // 
    pMountMgrPointBuffer->DeviceNameOffset = sizeof( MOUNTMGR_MOUNT_POINT );
    pMountMgrPointBuffer->DeviceNameLength = ( 2 * strlen( DeviceBuf ) ); 

    //
    // Open an handle to the Mount Point Manager
    // 
    hMntManager = CreateFile("\\\\.\\MountPointManager", 
                            (GENERIC_READ | GENERIC_WRITE),
                            (FILE_SHARE_READ | FILE_SHARE_WRITE), 
                             NULL, 
                             OPEN_EXISTING, 
                             0, 
                             NULL );

    if ( hMntManager == INVALID_HANDLE_VALUE )
    {
#ifdef _DEBUG
        sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName Failed (CreateFile)\n");
        OutputDebugString(DbgMsgBuf);
#endif

        return ERROR_MNT_PT_INFO;
    }

    // Convert from regular string to UNICODE string
    if ( MultiByteToWideChar(CP_ACP, 
                             0, 
                            DeviceBuf, 
                            ( -1 ), 
                            ( PUSHORT )( &InputBuffer[ sizeof(MOUNTMGR_MOUNT_POINT ) ] ), 
                            ( ( sizeof( InputBuffer ) - sizeof( MOUNTMGR_MOUNT_POINT ) ) / 2 ) 
                            ) == 0)
    {
#ifdef _DEBUG
        sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName Failed (MultByteToWideChar)\n");
        OutputDebugString(DbgMsgBuf);
#endif
        CloseHandle( hMntManager );
        return ERROR_MNT_PT_INFO;
    }

    pMountMgrPointsBuffer->Size = sizeof( OutputBuffer );
                     
    //
    // Query the GUID info to the Mount Point Manager
    //
    IoctlResult = DeviceIoControl(
                    hMntManager,
                    IOCTL_MOUNTMGR_QUERY_POINTS,
                    pMountMgrPointBuffer,
                    sizeof( InputBuffer ),
                    pMountMgrPointsBuffer,
                    sizeof( OutputBuffer ),
                    &sReturned,
                    NULL
                    );

    CloseHandle( hMntManager );

    //
    // Verify the number of Volume Mount Points of DOS Device
    //

    if( ( pMountMgrPointsBuffer->NumberOfMountPoints > 0 ) && ( IoctlResult ) ) 
    {
        //
        //Scan the SymbolicLinkName array for first occurence of VALID "\\?\Volume{GUID}}\"
        //   Usually, first  = "\DosDevices\_:" string
        //   Usually, second = ""\\?\Volume{GUID}}\" string, Unique Volume GUID
        //   Usually, third  = ""\\?\Volume{GUID}}\" string, persistent symbolic link

        for( Index = 0 ; Index < pMountMgrPointsBuffer->NumberOfMountPoints; Index ++ ) 
        {
            RtlZeroMemory( &MountPointInfo, sizeof( MountPointInfo ) );
            switch ( piMntPtType ) 
            {
                case DEVICE_NAME:
                    if ( WideCharToMultiByte( CP_ACP, 0, 
                            ( PWCHAR )( &OutputBuffer[ pMountMgrPointsBuffer->MountPoints[ Index ].DeviceNameOffset ] ),
                            ( pMountMgrPointsBuffer->MountPoints[ Index ].DeviceNameLength / sizeof( WCHAR ) ),
                            ( char * )( MountPointInfo ), sizeof( MountPointInfo ),
                            NULL, // Default character to be used 
                            NULL ) == 0 )
                    {
#ifdef _DEBUG
                        sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName Failed (WideCharToMultiByte)\n");
                        OutputDebugString(DbgMsgBuf);
#endif

                        return ERROR_MNT_PT_INFO;
                    }
                    else if ( IS_VALID_DEVICE_NAME( MountPointInfo ) )
                    {
                        strcpy( pszMntPtInfo, MountPointInfo );
#ifdef _DEBUG
                        sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName Success DEVICE_NAME:%s\n",pszMntPtInfo);
                        OutputDebugString(DbgMsgBuf);
#endif
                        return VALID_MNT_PT_INFO;
                    }
                    break;

                case DOS_DEVICE:
                    if ( WideCharToMultiByte( CP_ACP, 0, 
                    ( PWCHAR )( &OutputBuffer[ pMountMgrPointsBuffer->MountPoints[ Index ].SymbolicLinkNameOffset ] ),
                    ( pMountMgrPointsBuffer->MountPoints[ Index ].SymbolicLinkNameLength / sizeof( WCHAR ) ),
                    ( char * )( MountPointInfo ), sizeof( MountPointInfo ),
                    NULL, // Default character to be used 
                    NULL ) == 0 )
                    {
#ifdef _DEBUG
                        sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName Failed (WideCharToMultiByte2)\n");
                        OutputDebugString(DbgMsgBuf);
#endif
                        return ERROR_MNT_PT_INFO;
                    }
                    else if ( IS_VALID_DRIVE_LETTER( MountPointInfo ) )
                    {
                        strcpy( pszMntPtInfo, MountPointInfo );
#ifdef _DEBUG
                        sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName Success DOS_DEVICE:%s\n",pszMntPtInfo);
                        OutputDebugString(DbgMsgBuf);
#endif
                        return VALID_MNT_PT_INFO;
                    }
                    break;

                case VOLUME_GUID:
                    if ( WideCharToMultiByte( CP_ACP, 0, 
                    ( PWCHAR )( &OutputBuffer[ pMountMgrPointsBuffer->MountPoints[ Index ].SymbolicLinkNameOffset ] ),
                    ( pMountMgrPointsBuffer->MountPoints[ Index ].SymbolicLinkNameLength / sizeof( WCHAR ) ),
                    ( char * )( MountPointInfo ), sizeof( MountPointInfo ),
                    NULL, // Default character to be used 
                    NULL ) == 0 )
                    {
#ifdef _DEBUG
                        sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName Failed (WideCharToMultiByte3)\n");
                        OutputDebugString(DbgMsgBuf);
#endif
                        return ERROR_MNT_PT_INFO;
                    }
                    else if ( IS_VALID_VOLUME_NAME( MountPointInfo ) )
                    {
                        strcpy( pszMntPtInfo, MountPointInfo );
#ifdef _DEBUG
                        sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName Success VOLUME_GUID:%s\n",pszMntPtInfo);
                        OutputDebugString(DbgMsgBuf);
#endif
                        return VALID_MNT_PT_INFO;
                    }
                    break;
                default:
#ifdef _DEBUG
                        sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName Unknown type\n");
                        OutputDebugString(DbgMsgBuf);
#endif

                    break;
            }
        }
    }

#ifdef _DEBUG
    sprintf(DbgMsgBuf, "QueryVolMntPtInfoFromDevName No mount point info\n");
    OutputDebugString(DbgMsgBuf);
#endif

    return NO_MNT_PT_INFO;
}

HANDLE getFirstVolMntPtHandle( char *szVolumeGuid, char *PtBuf, unsigned long dwBufSize )
{
    HANDLE RetValue = FindFirstVolumeMountPoint( szVolumeGuid, // root path of volume to be scanned/
                                     PtBuf,   // pointer to output string
                                     dwBufSize // size of output buffer
                                     );

#ifdef _DEBUG
    sprintf(DbgMsgBuf, "getFirstVolMntPtHandle from %s Handle = [0x%08x]\n", szVolumeGuid,RetValue);
    OutputDebugString(DbgMsgBuf);
#endif

    return RetValue;
}

BOOL getVolumeNameForVolMntPt ( char *szVolumeMntPt, char *szVolumePath, unsigned long dwBufSize )
{
    BOOL RetValue = GetVolumeNameForVolumeMountPoint( szVolumeMntPt,    // input volume mount point or directory
                                               szVolumePath,     // output volume name buffer
                                               dwBufSize );        // size of volume name buffer
#ifdef _DEBUG
    sprintf(DbgMsgBuf, "getVolumeNameForVolMntPt from %s RetValue = [0x%08x]\n", szVolumeMntPt, RetValue);
    OutputDebugString(DbgMsgBuf);
#endif

    return RetValue;
}

void closeVolMntPtHandle( HANDLE hPt )
{
#ifdef _DEBUG
    sprintf(DbgMsgBuf, "closeVolMntPtHandle\n");
    OutputDebugString(DbgMsgBuf);
#endif

    FindVolumeMountPointClose(hPt);
}

BOOL getNextVolMntPt ( HANDLE hPt, char *PtBuf , unsigned long dwBufSize )
{
    BOOL RetValue = FindNextVolumeMountPoint( hPt,    // handle to scan
                                       PtBuf,  // pointer to output string
                                       dwBufSize ); // size of output buffer

#ifdef _DEBUG
    sprintf(DbgMsgBuf, "GetNextVolMntPt for Handle = [0x%08x]\n", hPt);
    OutputDebugString(DbgMsgBuf);
#endif

    return RetValue;
}


BOOL delVolMntPnt( char *szMountPoint  )
{
    BOOL RetValue = DeleteVolumeMountPoint( szMountPoint );

#ifdef _DEBUG
    sprintf(DbgMsgBuf, "delVolMntPnt for %s returned [0x%08x]\n",szMountPoint, RetValue);
    OutputDebugString(DbgMsgBuf);
#endif

    return RetValue;
}


BOOL setVolMntPnt( char *szVolumeMountPoint, char *szVolumeName )
{
    BOOL RetValue = SetVolumeMountPoint( szVolumeMountPoint, szVolumeName );

#ifdef _DEBUG
    sprintf(DbgMsgBuf, "setVolMntPnt to %s for %s returned [0x%08x]\n", szVolumeName, szVolumeMountPoint, RetValue);
    OutputDebugString(DbgMsgBuf);
#endif

    return RetValue;
}

#else  // Function's stub for NT4 version

int QueryVolMntPtInfoFromDevName(char *pszDeviceName, int piDevStrSize, char *pszMntPtInfo, int piGuidStrSize, 
                                 unsigned int piMntPtType)
{
    return 0;
}

HANDLE getFirstVolMntPtHandle( char *szVolumeGuid, char *PtBuf, unsigned long dwBufSize )
{
    return -1;
}

BOOL getVolumeNameForVolMntPt ( char *szVolumeMntPt, char *szVolumePath, unsigned long dwBufSize )
{
    return 0;
}

void closeVolMntPtHandle( HANDLE hPt )
{
}

BOOL getNextVolMntPt ( HANDLE hPt, char *PtBuf , unsigned long dwBufSize )
{
    return 0;
}

BOOL delVolMntPnt( char *szMountPoint  )
{
    return 0;
}


BOOL setVolMntPnt( char *szVolumeMountPoint, char *szVolumeName )
{
    return 0;
}

#endif