// Util.cpp : implementation file
//

#include "stdafx.h"
#include "DriveChangeTracker.h"
#include "DriveChangeTrackerDlg.h"
#include ".\drivechangetrackerdlg.h"
#include "util.h"
#include <winioctl.h>
#include "..\..\..\..\ntddk\inc\mountmgr.h"


#define NO_MNT_PT_INFO      0
#define VALID_MNT_PT_INFO	1
#define ERROR_MNT_PT_INFO  -1

enum  MntPtMngrData { 
	DEVICE_NAME = 1,
	DOS_DEVICE,
	VOLUME_GUID
}; 

int QueryVolMntPtInfoFromDevName    (   
                                    char *pszDeviceName, 
                                    int piDevStrSize, 
                                    char *pszMntPtInfo, 
                                    int piGuidStrSize, 
					                unsigned int piMntPtType
                                    );

int UtilGetGUIDforVolumeMountPoint  (
                                    IN  char *          szVolumeMountPoint, 
                                    OUT char *          szVolumeGUID, 
                                    IN  unsigned long   ulBufSize            
                                    );

int UtilGetNextVolumeMountPoint     (   
                                    IN  HANDLE          hVolumeHandle, 
                                    OUT char *          szVolumeMountPoint, 
                                    IN  unsigned long   ulBufSize            
                                    );

HANDLE UtilGetVolumeHandleAndFirstMountPoint(   
                                    IN  char *          szVolumeGUID, 
                                    OUT char *          szVolumeMountPoint, 
                                    IN  unsigned long   ulBufSize           
                                    );

void UtilCloseVolumeHandle          (         
                                    IN  HANDLE          hVolume 
                                    );

//
// Will create a vector containing the list of all devices and their corresponding GUID value
//
unsigned int GetAllVolumes(CString * cspArray, unsigned int uiMaxVolumes)
{
    char            szDriveString   [_MAX_PATH];
    char            szDosDevice     [_MAX_PATH];
    char            szCurDosDevice  [_MAX_PATH];
    char            szGUIDName      [_MAX_PATH];
    char            szMountPt       [_MAX_PATH];
    char            szDrive         [4];

    int             i                   =   0,
                    iDrive              =   0;
    unsigned int    uiNumVolumes        =   0;

    //
    // Get all drives on this system
    //
	GetLogicalDriveStrings(sizeof(szDriveString), szDriveString);
	
    //
    // Get drive letter of each drive...
    //
	while(szDriveString[i] != 0 && szDriveString[i+1] != 0)
	{
        szDrive[0] = szDriveString[i];    //Drive Letter is here !
        szDrive[1] = szDriveString[i+1];
        szDrive[2] = szDriveString[i+2];
        szDrive[3] = 0;
		i = i + 4;

		memset(szDosDevice, 0, sizeof(szDosDevice));
        memset(szCurDosDevice, 0, sizeof(szCurDosDevice));

		strcpy(szDosDevice , "\\DosDevices\\");
		strncat(szDosDevice , &szDrive[0], 1);
		strcat(szDosDevice , ":");

        //
        // validate drivetype (Don't care about CD's, ramdisks, remotes, and removables!)
        //
		iDrive = GetDriveType(szDrive);

		if(     iDrive != DRIVE_REMOVABLE 
            &&  iDrive != DRIVE_CDROM 
            &&  iDrive != DRIVE_RAMDISK 
            &&  iDrive != DRIVE_REMOTE      )
		{   
            HANDLE  hDrive                  = NULL;                  
            bool    bDriveNotFinished       = true;
            bool    bGUIDName               = true;
            bool    bAreMountPointsPresent  = false;

            strcpy(szCurDosDevice,szDosDevice);

            //
            // Process drive and any mount points on this drive
            //
            while (bDriveNotFinished && bGUIDName)
            {
                memset(szMountPt,0,sizeof(szMountPt));
                memset(szGUIDName,0,sizeof(szGUIDName));
                //
                // Get Name and GUID of current device!
                //
                if (    
                        (QueryVolMntPtInfoFromDevName( szCurDosDevice, sizeof(szCurDosDevice), szGUIDName, sizeof(szGUIDName), VOLUME_GUID ) == VALID_MNT_PT_INFO) 
                     ||
                        (UtilGetGUIDforVolumeMountPoint( szCurDosDevice, szGUIDName, sizeof(szGUIDName) ) )
                   )
                {
                    //
                    // Some GUID may come back as \\?\ instead of \??\
                    // and finish with a \ instead of }
                    //
                    // so change it back to \??\ and no \ behind the }!!!
                    //
                    szGUIDName[1] = '?';  
					szGUIDName[48] = 0;
					
                    //
                    // Store drivename and guid!
                    //
                    if (bAreMountPointsPresent)
                    {
                        if (uiNumVolumes<uiMaxVolumes)
                        {
                            cspArray[uiNumVolumes] = szCurDosDevice;
                        }
                    }
                    else
                    {
                        if (uiNumVolumes<uiMaxVolumes)
                        {
                            cspArray[uiNumVolumes] = szDrive;
                        }
                    }

                    uiNumVolumes++;

                    //
                    // Check for any more volume mount points
                    //
                    if (!hDrive)
                    {
                        hDrive = UtilGetVolumeHandleAndFirstMountPoint( szDrive, szMountPt, _MAX_PATH );
                        if (hDrive == INVALID_HANDLE_VALUE)
                        {
                            bDriveNotFinished = false;
                        }
                    }
                    else
                    {
                        if (bAreMountPointsPresent)
                        {
                            bDriveNotFinished = (UtilGetNextVolumeMountPoint(hDrive,szMountPt,_MAX_PATH ) >0);
                        }
                    }

    			    if (hDrive && !bDriveNotFinished) 
	    		    {
        			    UtilCloseVolumeHandle(hDrive);
                    }
                    else
                    {
                        //
                        // Initialize szCurDosDevice with drivename followed by mountpoint!
                        //
                        bAreMountPointsPresent = true;
                		strcpy(szCurDosDevice,szDrive);
		                strcat(szCurDosDevice,szMountPt);
                    }
                }
                else
                {
                    bDriveNotFinished = false;
                }
            }
        }
    }

    return uiNumVolumes;
}

#define IS_VALID_DRIVE_LETTER(s) (\
		s[0] == '\\' &&           \
		s[1] == 'D' &&            \
		s[2] == 'o' &&            \
		s[3] == 's' &&            \
		s[4] == 'D' &&            \
		s[5] == 'e' &&            \
		s[6] == 'v' &&            \
		s[7] == 'i' &&            \
		s[8] == 'c' &&            \
		s[9] == 'e' &&            \
		s[10] == 's' &&           \
		s[11] == '\\' &&          \
		s[12] >= 'A' &&           \
		s[12] <= 'Z' &&           \
		s[13] == ':')

#define IS_VALID_DEVICE_NAME(s) (\
		s[0] == '\\' &&           \
		s[1] == 'D' &&            \
		s[2] == 'e' &&            \
		s[3] == 'v' &&            \
		s[4] == 'i' &&            \
		s[5] == 'c' &&            \
		s[6] == 'e' &&            \
		s[7] == '\\' &&            \
		s[8] == 'H' &&            \
		s[9] == 'a' &&            \
		s[10] == 'r' &&           \
		s[11] == 'd' &&          \
		s[12] == 'd' &&           \
		s[13] == 'i' &&           \
		s[14] <= 's' &&           \
		s[15] == 'k')

//
// Macro that defines what a "volume name" mount point is.  This macro can
// be used to scan the result from QUERY_POINTS to discover which mount points
// are "volume name" mount points.
//
#define IS_VALID_VOLUME_NAME(s) (          \
		s[0] == '\\' &&					   \
	    (s[1] == '?' ||  s[1] == '\\') && \
		s[2] == '?' &&			           \
		s[3] == '\\' &&                    \
		s[4] == 'V' &&                     \
		s[5] == 'o' &&                     \
		s[6] == 'l' &&                     \
		s[7] == 'u' &&                     \
		s[8] == 'm' &&                     \
		s[9] == 'e' &&                     \
		s[10] == '{' &&                    \
		s[19] == '-' &&                    \
		s[24] == '-' &&                    \
		s[29] == '-' &&                    \
		s[34] == '-' &&                    \
		s[47] == '}')

#define NO_MNT_PT_INFO      0
#define VALID_MNT_PT_INFO	1
#define ERROR_MNT_PT_INFO  -1

unsigned    char InputBuffer    [4096]; 
unsigned    char OutputBuffer   [16384]; 
    	    char DeviceBuf      [4096];


int QueryVolMntPtInfoFromDevName(   char *          pszDeviceName, 
                                    int             piDevStrSize, 
                                    char *          pszMntPtInfo, 
                                    int             piGuidStrSize, 
					                unsigned int    piMntPtType     )
{
	HANDLE                  hMntManager;
    char                    MountPointInfo[ MAX_PATH ];

    PMOUNTMGR_MOUNT_POINT   pMountMgrPointBuffer    = ( PMOUNTMGR_MOUNT_POINT ) ( &InputBuffer ); 
    PMOUNTMGR_MOUNT_POINTS  pMountMgrPointsBuffer   = ( PMOUNTMGR_MOUNT_POINTS )( &OutputBuffer );
 	int                     IoctlResult             = 0;
    unsigned long           sReturned               = 0; 
    unsigned long           Index                   = 0;

	RtlZeroMemory( &InputBuffer, sizeof( InputBuffer ));
	RtlZeroMemory( &OutputBuffer, sizeof( OutputBuffer ));
	RtlZeroMemory( &DeviceBuf, sizeof(DeviceBuf));

    //
    // Check to see that the string given in input is not too small or too big
    //
	if ( ( piGuidStrSize > 0 ) && ( piGuidStrSize <=  sizeof(DeviceBuf)) ) 
    {
		RtlZeroMemory( pszMntPtInfo, piGuidStrSize );
    }
	else
    {
		return ERROR_MNT_PT_INFO;
    }

    //
    // Copy devicename to DeviceBuf
    //
	strncpy( DeviceBuf , pszDeviceName, piDevStrSize);

	//
	// Initialize the non persistent device name element of 
	// MOUNTMGR_MOUNT_POINT structure, MOUNTMGR.H
	// 
    pMountMgrPointBuffer->DeviceNameOffset = sizeof( MOUNTMGR_MOUNT_POINT );
    pMountMgrPointBuffer->DeviceNameLength = ( 2 * (USHORT)strlen( DeviceBuf ) ); 

	//
	// Open a handle to the Mount Point Manager
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
		return ERROR_MNT_PT_INFO;
    }

    //
	// Convert the devicename from a regular string to UNICODE string
    //
    // Make sure we place the string in the correct area of the input buffer
    //
	if ( MultiByteToWideChar(CP_ACP, 
							 0, 
							DeviceBuf, 
							( -1 ), 
							(LPWSTR)( &InputBuffer[ sizeof(MOUNTMGR_MOUNT_POINT ) ] ),       // destination of converted string
							( ( sizeof( InputBuffer ) - sizeof( MOUNTMGR_MOUNT_POINT ) ) / 2 ) 
							) == 0)
    {
    	CloseHandle( hMntManager );
		return ERROR_MNT_PT_INFO;
    }

    //
    // Setup the output buffer for the ioctl
    //
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
    // If we are unable to send the ioctl, get out of dodge!
    //
    if (!IoctlResult)
    {
#if _SHOW_ALL_OUTPUT_
        printf("Unable to send Ioctl error = %ld\n",GetLastError());
#endif
        return ERROR_MNT_PT_INFO;
    }

	//
	// Verify the number of Volume Mount Points of the current DOS Device
	//
	if( pMountMgrPointsBuffer->NumberOfMountPoints > 0) 
    {
		//
		// Scan the SymbolicLinkName array for first occurence of VALID "\\?\Volume{GUID}}\"
        //
		//   Usually, first  = "\DosDevices\_:" string
		//   Usually, second = ""\\?\Volume{GUID}}\" string, Unique Volume GUID
		//   Usually, third  = ""\\?\Volume{GUID}}\" string, persistent symbolic link

        for( Index = 0 ; Index < pMountMgrPointsBuffer->NumberOfMountPoints; Index ++ ) 
		{
            //
            // Clear the mountpoint info
            //
			RtlZeroMemory( &MountPointInfo, sizeof( MountPointInfo ) );

			switch ( piMntPtType ) 
			{
                //
                // This tupple contains a DEVICE_NAME!!  example: \Device\HardDiskVolume1
                //
				case DEVICE_NAME:
                    //
                    // Convert the name back to non-unicode, and check its validity
                    //
					if ( WideCharToMultiByte(   
                            CP_ACP, 
                            0, 
							( PWCHAR )( &OutputBuffer[ pMountMgrPointsBuffer->MountPoints[ Index ].DeviceNameOffset ] ),
							( pMountMgrPointsBuffer->MountPoints[ Index ].DeviceNameLength / sizeof( WCHAR ) ),
							( char * )( MountPointInfo ), sizeof( MountPointInfo ),
							NULL, // Default character to be used 
							NULL ) 
                        == 0 )
                    {
						return ERROR_MNT_PT_INFO;
                    }
					else if ( IS_VALID_DEVICE_NAME( MountPointInfo ) )
					{
						strcpy( pszMntPtInfo, MountPointInfo );
						return VALID_MNT_PT_INFO;
					}
					break;

                //
                // This tupple contains a DOS_DEVICE!!  example: \DosDevices\C:\
                //
				case DOS_DEVICE:
                    //
                    // Convert the name back to non-unicode, and check it's validity
                    //
					if ( WideCharToMultiByte( 
                            CP_ACP, 
                            0, 
					        ( PWCHAR )( &OutputBuffer[ pMountMgrPointsBuffer->MountPoints[ Index ].SymbolicLinkNameOffset ] ),
					        ( pMountMgrPointsBuffer->MountPoints[ Index ].SymbolicLinkNameLength / sizeof( WCHAR ) ),
					        ( char * )( MountPointInfo ), sizeof( MountPointInfo ),
					        NULL, // Default character to be used 
					        NULL ) 
                        == 0 )
                    {
						return ERROR_MNT_PT_INFO;
                    }
					else if ( IS_VALID_DRIVE_LETTER( MountPointInfo ) )
					{
						strcpy( pszMntPtInfo, MountPointInfo );
						return VALID_MNT_PT_INFO;
					}
					break;
                
                //
                // This tupple contains a VOLUME_GUID!! example: \??\Volume{fef4d314-0604-11d7-90cf-806d6172696f}
                //
				case VOLUME_GUID:
                    //
                    // Convert the name back to non-unicode, and check it's validity
                    //
					if ( WideCharToMultiByte( 
                            CP_ACP, 
                            0, 
					        ( PWCHAR )( &OutputBuffer[ pMountMgrPointsBuffer->MountPoints[ Index ].SymbolicLinkNameOffset ] ),
					        ( pMountMgrPointsBuffer->MountPoints[ Index ].SymbolicLinkNameLength / sizeof( WCHAR ) ),
					        ( char * )( MountPointInfo ), sizeof( MountPointInfo ),
					        NULL, // Default character to be used 
					        NULL ) 
                        == 0 )
                    {
						return ERROR_MNT_PT_INFO;
                    }
					else if ( IS_VALID_VOLUME_NAME( MountPointInfo ) )
					{
						strcpy( pszMntPtInfo, MountPointInfo );
                        //
                        // Some GUID may come back as \\?\ instead of \??\
                        // and finish with a \ instead of }
                        //
                        // so change it back to \??\ and no \ behind the }!!!
                        //
                        pszMntPtInfo[1] = '?';  
					    pszMntPtInfo[48] = 0;
						return VALID_MNT_PT_INFO;
					}
					break;

                //
                // We should never pass here
                //
				default:
					break;
			}
		}
	}
	return NO_MNT_PT_INFO;
}


//
// This function returns 1 if the given VolumeMountPoint is valid and 
// has a GUID!
//
// It also returns the GUID in szVolumeGUID
//
// ulBufSize is the size of szVolumeGUID you give us
//
int UtilGetGUIDforVolumeMountPoint(IN  char *          szVolumeMountPoint, 
                                   OUT char *          szVolumeGUID, 
                                   IN  unsigned long   ulBufSize            )
{
    int RetValue;
    RetValue = GetVolumeNameForVolumeMountPoint(szVolumeMountPoint, 
											    szVolumeGUID,    
									            ulBufSize           );      
    if (RetValue)
    {
        szVolumeGUID[1] = '?';
        szVolumeGUID[48] = 0;
    }
    return RetValue;
}

//
// This function returns 1 if the volume has another valid mount point
//
// It also returns the next valid mount point in szVolumeMountPoint
//
// ulBufSize is the size of szVolumeMountPoint you give us
//
int UtilGetNextVolumeMountPoint(   IN  HANDLE          hVolumeHandle, 
                                   OUT char *          szVolumeMountPoint, 
                                   IN  unsigned long   ulBufSize            )
{
    return FindNextVolumeMountPoint(hVolumeHandle,         
									szVolumeMountPoint,       
									ulBufSize           );  
}

//
// This functions returns a valid handle on the volume if there is a
// mount point on this volume.
//
// It also returns the first valid mount point in szVolumeMountPoint
//
// ulBufSize is the size of szVolumeMountPoint you give us
//
// The function returns INVALID_HANDLE_VALUE if there are no 
// Mount Points
//
// NOTE: Make sure you close the handle returned by calling:
//       UtilCloseVolumeHandle ( hVolume ) 
//
HANDLE UtilGetVolumeHandleAndFirstMountPoint(   
                                    IN  char *          szVolumeGUID, 
                                    OUT char *          szVolumeMountPoint, 
                                    IN  unsigned long   ulBufSize           )
{
	HANDLE hVolumeHandle = INVALID_HANDLE_VALUE;

	hVolumeHandle = FindFirstVolumeMountPoint(  szVolumeGUID,  
									            szVolumeMountPoint,
									            ulBufSize           );    
	return(hVolumeHandle);
}

//
// This function closes the handle returned by
// UtilGetVolumeHandleAndFirstMountPoint(...)
//
void UtilCloseVolumeHandle(         IN  HANDLE          hVolume )
{
    //
    // Validate the number passed to us (minimum validation)
    //
    if (    (INVALID_HANDLE_VALUE != hVolume)
         &&
            (hVolume)
       )
    {
	    FindVolumeMountPointClose(hVolume);
    }
}
