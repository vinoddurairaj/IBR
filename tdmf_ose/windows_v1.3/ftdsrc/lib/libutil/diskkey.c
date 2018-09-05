//=====================================================================
//
// Diskkey.c
//
//====================================================================
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <winioctl.h>

#include "error.h"
#include "diskkey.h"

//
// Name of the DISK key and information value
//
static char	DiskKeyName[] = "SYSTEM\\Disk";
static char	DiskValueName[] = "Information";		 

//
// FT-type strings
//
static char PartTypes[][32] = {
	"Mirror",
	"Stripe",
	"ParityStripe",
	"VolumeSet",
	"NonFt",
	"WholeDisk"
};

//
// FT-volume states
//
static char FtStates[][32] = {
	"Healthy",
	"Orphaned",
	"Regenerating",
	"Initializing",
	"SyncWithCopy"
};

//
// FT-partition states
//
static char FtPartStates[][32] = {
    "FtStateOk",      
    "FtHasOrphan",  
    "FtDisabled",    
    "FtRegenerating", 
    "FtInitializing",
    "FtCheckParity",  
    "FtNoCheckData"  
};

//----------------------------------------------------------------------
//
// DumpDiskInformation
//
// Dump information about one disk.
//
//---------------------------------------------------------------------- 
int GetParitionNumber( PDISK_DESCRIPTION DiskDescription, char cDriveLetter, 
                        int DiskNum, int *partition, BOOLEAN *bDone )
{
	PDISK_PARTITION		partInfo;
	PPARTITION_HEADER	partHeader;
	int					i;

	if( DiskDescription->NumberOfPartitions ) {

		partHeader = (PPARTITION_HEADER) (char *) &DiskDescription->PartitionHeader;

		for( i = 0; i < DiskDescription->NumberOfPartitions; i++ ) {

			partInfo = (PDISK_PARTITION) ( (char *) &partHeader->Partitions +
								i * sizeof( DISK_PARTITION ));

            if ( (partInfo->DriveLetter == cDriveLetter) && (partInfo->AssignDriveLetter == 1) ) {
                *partition = i + 1;

                *bDone = TRUE;

                return 0;
            }
        }		  
	}

	return  ( (DiskDescription->NumberOfPartitions ? sizeof( PARTITION_HEADER ): 0) + 
			( DiskDescription->NumberOfPartitions * sizeof( DISK_PARTITION )));
}


//----------------------------------------------------------------------
//
// DumpAllInformation
//
// Starts with the header, and calls sub-dumping functions.
//
//----------------------------------------------------------------------
BOOLEAN GetDiskParitionNumber(PDISK_CONFIG_HEADER DiskInformation, char cDriveLetter, 
                              int *disk, int *partition)
{
	PDISK_REGISTRY		diskInfo;
	char				*diskDescription;
	char                *DiskInfoBuffer = (char *) DiskInformation;
    BOOLEAN             bDone = FALSE;
	int					i;

    diskInfo = (PDISK_REGISTRY) &DiskInfoBuffer[ DiskInformation->DiskInformationOffset ];
	diskDescription = (char *) &diskInfo->Disks[0];

    for( i = 0; i < diskInfo->NumberOfDisks; i++ ) {

		diskDescription += GetParitionNumber( (PDISK_DESCRIPTION) diskDescription, cDriveLetter,
            i, partition, &bDone );

        if (bDone) {
			*disk = i;

            return TRUE;
        }
    }

	return FALSE;
}



//----------------------------------------------------------------------
//
// ReadDiskInformation
//
// Just reads the disk information value.
//
//----------------------------------------------------------------------
PDISK_CONFIG_HEADER ReadDiskInformation( char *szError )
{
	int		            informationSize;
	DWORD	            status, disktype;
	HKEY	            hDiskKey;
    PDISK_CONFIG_HEADER DiskInformation;
    char                error[256];
	//
	// Open the disk key
	//
	if( RegOpenKey( HKEY_LOCAL_MACHINE, DiskKeyName, &hDiskKey ) != ERROR_SUCCESS) {
		PrintWin32Error(GetLastError(), error);
		sprintf(szError, "Error opening DISK key: %s", error);
		return NULL;
	}

 	//
	// Read the value, allocating larger buffers if necessary
 	//
 	informationSize = 0;
 	// Read to null buffer to find value size
 	RegQueryValueEx( hDiskKey, DiskValueName, NULL, &disktype, NULL,
							&informationSize );
 	DiskInformation = (PDISK_CONFIG_HEADER) malloc( informationSize  );
 
 	status = RegQueryValueEx( hDiskKey, DiskValueName, 0, &disktype, 
 		(PBYTE) DiskInformation, &informationSize );
	
	//
	// Did we read the value successfully?
	//
	if( status != ERROR_SUCCESS ) {
        free(DiskInformation);

		PrintWin32Error(status, error);
		sprintf(szError, "Error reading DISK\\Information value: %s", error );
		RegCloseKey( hDiskKey );
		return NULL;
	}

	RegCloseKey( hDiskKey );
	
    return DiskInformation;
}

BOOLEAN GetPartitionStrFromDrive(char *cDriveLetter, char *szPartition, char *szError)
{
    PDISK_CONFIG_HEADER DiskInformation;
    int     disk, partition;

	if( cDriveLetter[0] >= 'a' && cDriveLetter[0] <= 'z' ) {
		cDriveLetter[0] -= 'a';
        cDriveLetter[0] += 'A';
	} 

    //
	// Read the disk key
	//
    if ( (DiskInformation = ReadDiskInformation(szError)) == NULL ) {
        return FALSE;
    }

    if ( !GetDiskParitionNumber(DiskInformation, cDriveLetter[0], &disk, &partition) ) {
        free (DiskInformation);
        return FALSE;
    }

    sprintf(szPartition,
            "\\Device\\Harddisk%d\\Partition%d",
            disk,
            partition);

    free (DiskInformation);

    return TRUE;
}

BOOLEAN GetPartitionNumFromDrive(char *cDriveLetter, int *disk, int *partition, char *szError)
{
    PDISK_CONFIG_HEADER DiskInformation;

	if( cDriveLetter[0] >= 'a' && cDriveLetter[0] <= 'z' ) {
		cDriveLetter[0] -= 'a';
        cDriveLetter[0] += 'A';
	} 


    //
	// Read the disk key
	//
    if ( (DiskInformation = ReadDiskInformation(szError)) == NULL ) {
        return FALSE;
    }

    if ( !GetDiskParitionNumber(DiskInformation, cDriveLetter[0], disk, partition) ) {
        free (DiskInformation);
        return FALSE;
    }

    free (DiskInformation);

    return TRUE;
}
