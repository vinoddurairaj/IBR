#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <winioctl.h>

#include "list.h"
#include "misc.h"
#include "error.h"
#include "bitmap.h"


//--------------------------------------------------------------------
//                         D E F I N E S
//--------------------------------------------------------------------

//
// Size of the buffer we read file mapping information into.
// The buffer is big enough to hold the 16 bytes that 
// come back at the head of the buffer (the number of entries 
// and the starting virtual cluster), as well as 512 pairs
// of [virtual cluster, logical cluster] pairs.
//
#define	FILEMAPSIZE		(512+2)

//
// Size of the bitmap buffer we pass in. Its large enough to
// hold information for the 16-byte header that's returned
// plus the indicated number of bytes, each of which has 8 bits 
// (imagine that!)
//
#define BITMAPBYTES		4096
#define BITMAPSIZE		(BITMAPBYTES+2*sizeof(ULONGLONG))

//
// Invalid longlong number
//
#define LLINVALID		((ULONGLONG) -1)

//--------------------------------------------------------------------
//                        G L O B A L S
//--------------------------------------------------------------------

BYTE		BitShift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };


//--------------------------------------------------------------------
//                      F U N C T I O N S
//--------------------------------------------------------------------

//--------------------------------------------------------------------
//
// LocateNDLLCalls
//
// Loads function entry points in NTDLL
//
//--------------------------------------------------------------------
static BOOLEAN LocateNTDLLCalls()
{

 	if( !(NtFsControlFile = (void *) GetProcAddress( GetModuleHandle("ntdll.dll"),
			"NtFsControlFile" )) ) {

		return FALSE;
	}

    return TRUE;
}

//--------------------------------------------------------------------
//
// DumpBitmap
//
// Start at the offset specified (if any) and dumps all the free
// clusters on the volume to the end of the volume or until
// the user stops the dump with a 'q'.
//
//--------------------------------------------------------------------
BOOL GetVolumeUsedClusters( HANDLE hVolume, PLIST_ENTRY pUsedBitmapList, 
                            char *szError )
{
    LIST_ENTRY                  FreeBitmapList;
    PLIST_ENTRY                 pFreehead;
    PCLUSTER_GROUP              pFreeClusters, pUsedClusters;
	ULONGLONG					currentCluster = 0;

	InitializeListHead( pUsedBitmapList );

    if ( !GetVolumeFreeClusters( hVolume, &FreeBitmapList, szError ) ) {
        return FALSE;
    }
    
    while(!IsListEmpty(&FreeBitmapList)) 
    {
        pFreehead = RemoveHeadList(&FreeBitmapList);
        pFreeClusters = CONTAINING_RECORD(pFreehead, CLUSTER_GROUP, ListEntry); 

		if (pFreeClusters->cluster != currentCluster) {
            pUsedClusters = (PCLUSTER_GROUP)malloc(sizeof(CLUSTER_GROUP));

            pUsedClusters->cluster = currentCluster;
			pUsedClusters->num = pFreeClusters->cluster - currentCluster - 1;

            InsertTailList(pUsedBitmapList,&pUsedClusters->ListEntry);
		}
        
		currentCluster = pFreeClusters->cluster + pFreeClusters->num;

		free(pFreeClusters);
    }

    return TRUE;
}

//--------------------------------------------------------------------
//
// DumpBitmap
//
// Start at the offset specified (if any) and dumps all the free
// clusters on the volume to the end of the volume or until
// the user stops the dump with a 'q'.
//
//--------------------------------------------------------------------
BOOL GetVolumeFreeClusters( HANDLE hVolume, PLIST_ENTRY pBitmapList, 
                            char *szError )
{
	DWORD						status;
	PBITMAP_DESCRIPTOR			bitMappings;
	ULONGLONG					cluster;
	ULONGLONG					numFree;
	ULONGLONG					startLcn;
	ULONGLONG					nextLcn;
	ULONGLONG					lastLcn;
	IO_STATUS_BLOCK				ioStatus;
	ULONGLONG					i;
    PCLUSTER_GROUP              pBitmap;
	BYTE		                BitMap[ BITMAPSIZE ];
    char						error[256];

	InitializeListHead( pBitmapList );

	if( !LocateNTDLLCalls() ) {
		return FALSE;
	}

    //
	// Start scanning at the cluster offset the user specifies
	//
	bitMappings = (PBITMAP_DESCRIPTOR) BitMap;
	cluster = 0;
	nextLcn = 0;
	lastLcn = LLINVALID;
	while( !(status = NtFsControlFile( hVolume, NULL, NULL, 0, &ioStatus,
						FSCTL_GET_VOLUME_BITMAP,
						&nextLcn, sizeof( cluster ),
						bitMappings, BITMAPSIZE )) ||
			 status == STATUS_BUFFER_OVERFLOW ||
			 status == STATUS_PENDING ) {

		// 
		// If the operation is pending, wait for it to finish
		//
		if( status == STATUS_PENDING ) {
			
			WaitForSingleObject( hVolume, INFINITE );
			
			//
			// Get the status from the status block
			//
			if( ioStatus.Status != STATUS_SUCCESS && 
				ioStatus.Status != STATUS_BUFFER_OVERFLOW ) {

				PrintNtError( ioStatus.Status, error );
				sprintf(szError, "Get Volume Bitmap: %s", error);

				return FALSE;
			}
		}

		//
		// Scan through the returned bitmap info, looking for empty clusters
		//
		startLcn = bitMappings->StartLcn;
		numFree = 0;
		cluster = LLINVALID;
		for( i = 0; i < min( bitMappings->ClustersToEndOfVol, 8*BITMAPBYTES); i++ ) 
		{
			if( !(bitMappings->Map[ i/8 ] & BitShift[ i % 8 ])) 
			{

				//
				// Cluster is free
				//
				if( cluster == LLINVALID ) 
				{

					cluster = startLcn + i;
					numFree = 1;

				} 
				else 
				{

					numFree++;
				}
			} 
			else 
			{

				//
				// Cluster is not free
				//
				if( cluster != LLINVALID ) {
					
					if( lastLcn == cluster ) {

						lastLcn = LLINVALID;
					} else {

                        pBitmap = (PCLUSTER_GROUP)malloc(sizeof(CLUSTER_GROUP));

                        pBitmap->cluster = cluster;
						pBitmap->num = numFree;
                        InsertTailList(pBitmapList,&pBitmap->ListEntry);

						numFree = 0;
						lastLcn = cluster;
						cluster = LLINVALID;
					}
				} 
			}
		}

		//
		// Print any remaining
		//
		if( cluster != LLINVALID && lastLcn != cluster ) {

            pBitmap = (PCLUSTER_GROUP)malloc(sizeof(CLUSTER_GROUP));

            pBitmap->cluster = cluster;
			pBitmap->num = numFree;
            InsertTailList(pBitmapList,&pBitmap->ListEntry);

			numFree = 0;
			cluster = LLINVALID;
		}

		//
		// End of volume?
		//
		if( status != STATUS_BUFFER_OVERFLOW ) {

			return TRUE;
		}				

		//
		// Move to the next block
		//
		nextLcn = bitMappings->StartLcn + i;
	}

	//
	// We only get here when there's an error
	//
	PrintNtError( status, error );
	sprintf(szError, "\nGet Volume Bitmap: %s", error);

	return FALSE;
}


//--------------------------------------------------------------------
//
// GetFileUsedClusters
//
// Dumps the clusters belonging to the specified file until the
// end of the file.
//
//--------------------------------------------------------------------
BOOL GetFileUsedClusters( int drive, char *argument, PLIST_ENTRY pBitmapList, char *szError )
{
	DWORD						status;
	int							i;
	HANDLE						sourceFile;
	char						fileName[MAX_PATH];
	IO_STATUS_BLOCK				ioStatus;
	ULONGLONG					startVcn;
	PGET_RETRIEVAL_DESCRIPTOR	fileMappings;
    ULONGLONG	                FileMap[ FILEMAPSIZE ];
    PCLUSTER_GROUP              pBitmap;
	char						error[256];

	InitializeListHead( pBitmapList );

    //
	// Make the name into a real pathname
	//
	if( strlen( argument ) > 1 && argument[0] != '\\' &&
		argument[0] != 'A'+drive &&
		argument[0] != 'a'+drive ) 
		sprintf(fileName, "%C:\\%s", drive+'A', argument );
	else if( strlen( argument ) > 1 && argument[0] == '\\') 
		sprintf(fileName, "%C:%s", drive+'A', argument );
	else
		strcpy(fileName, argument );

	//
	// Open the file
	//
	sourceFile = CreateFile( fileName, GENERIC_READ, 
					FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
					FILE_FLAG_NO_BUFFERING, 0 );
	if( sourceFile == INVALID_HANDLE_VALUE ) {
		PrintWin32Error( GetLastError(), error );
		sprintf(szError, "Failed to open file: %s", error);
		return FALSE;
	}

	//
	// Start dumping the mapping information. Go until we hit the end of the
	// file.
	//
	startVcn = 0;
	fileMappings = (PGET_RETRIEVAL_DESCRIPTOR) FileMap;
	while( !(status = NtFsControlFile( sourceFile, NULL, NULL, 0, &ioStatus,
						FSCTL_GET_RETRIEVAL_POINTERS,
						&startVcn, sizeof( startVcn ),
						fileMappings, FILEMAPSIZE * sizeof(LARGE_INTEGER) ) ) ||
			 status == STATUS_BUFFER_OVERFLOW ||
			 status == STATUS_PENDING ) {

		// 
		// If the operation is pending, wait for it to finish
		//
		if( status == STATUS_PENDING ) {
			
			WaitForSingleObject( sourceFile, INFINITE ); 

			//
			// Get the status from the status block
			//
			if( ioStatus.Status != STATUS_SUCCESS && 
				ioStatus.Status != STATUS_BUFFER_OVERFLOW ) {

				PrintNtError( ioStatus.Status, error );
				sprintf("Enumerate file clusters: %s", szError);
				return FALSE;
			}
		}

		//
		// Loop through the buffer of number/cluster pairs, printing them
		// out.
		//
		startVcn = fileMappings->StartVcn;
		for( i = 0; i < (ULONGLONG) fileMappings->NumberOfPairs; i++ ) {

/*
			//
			// On NT 4.0, a compressed virtual run (0-filled) is 
			// identified with a cluster offset of -1
			//
			if( fileMappings->Pair[i].Lcn == LLINVALID ) {
				printf("   VCN: %I64d VIRTUAL LEN: %I64d\n",
							startVcn, fileMappings->Pair[i].Vcn - startVcn ); 
			} else {
				printf("   VCN: %I64d LCN: %I64d LEN: %I64d\n",
							startVcn, fileMappings->Pair[i].Lcn, 
							fileMappings->Pair[i].Vcn - startVcn );
			}
*/
            pBitmap = (PCLUSTER_GROUP)malloc(sizeof(CLUSTER_GROUP));

            pBitmap->cluster = fileMappings->Pair[i].Lcn;
			pBitmap->num = fileMappings->Pair[i].Vcn - startVcn;
            InsertTailList(pBitmapList,&pBitmap->ListEntry);

			startVcn = fileMappings->Pair[i].Vcn;
		}

		//
		// If the buffer wasn't overflowed, then we're done
		//
		if( !status ) break;
	}

	CloseHandle( sourceFile );

	if( status != STATUS_BUFFER_OVERFLOW ) {

		return TRUE;
	}				


	return FALSE;
}

