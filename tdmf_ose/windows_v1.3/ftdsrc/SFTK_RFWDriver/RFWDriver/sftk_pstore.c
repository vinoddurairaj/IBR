/**************************************************************************************

Module Name: sftk_pstore.C   
Author Name: Parag sanghvi 
Description: Pstore I/O and related APIS
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

// NT Undocumented calls, TODO : We should not need this.  ???
NTSYSAPI NTSTATUS NTAPI ZwWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );


// To Check PSTORE File Existence
NTSTATUS
sftk_is_file_exist( PUNICODE_STRING   FileName)
{
    NTSTATUS				status = STATUS_SUCCESS;
    IO_STATUS_BLOCK         ioStatus;
    PFILE_OBJECT            fileObject;
    OBJECT_ATTRIBUTES       objAttr;
    HANDLE                  fileHandle = NULL;

    try 
    {
        // Get target file handle for PStore, create it if not exist
#ifdef NTFOUR
        InitializeObjectAttributes(&objAttr, 
                                   FileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL, 
                                   NULL);
#else
        InitializeObjectAttributes(&objAttr, 
                                   FileName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, //sg
                                   NULL, 
                                   NULL);
#endif

        status = ZwCreateFile(	&fileHandle,
								GENERIC_READ|SYNCHRONIZE|GENERIC_WRITE,   
								&objAttr,
								&ioStatus,
								0, 
								FILE_ATTRIBUTE_NORMAL,
								FILE_SHARE_READ|FILE_SHARE_WRITE | FILE_SHARE_DELETE,
								FILE_OPEN, 
								FILE_SYNCHRONOUS_IO_NONALERT,
								NULL, 
								0);

        if (!NT_SUCCESS(status)) 
        {
            DebugPrint((DBG_ERROR, "sftk_is_file_exist: ZwCreateFile(FileName %S) Failed to open with status 0x%08x \n", 
							FileName->Buffer, status));
            try_return(status);
        }

        // Close PStore file for proper operation. 
        status = ZwClose(fileHandle);

        if (!NT_SUCCESS(status)) 
		{
            DebugPrint((DBG_ERROR, "sftk_is_file_exist: ZWClose(FileName %S) Failed to open with status 0x%08x \n", 
							FileName->Buffer, status));

			status = STATUS_SUCCESS; // Igonre this error....
		}

        try_exit:   NOTHING;
    } 
    finally 
    {
    }

    return(status);
} // sftk_is_file_exist()

// 
// Make sure all open Refrence for this Pstore File has been closed before making this API calls.
//
NTSTATUS
sftk_delete_pstorefile( IN PSFTK_LG Sftk_Lg)
{
	NTSTATUS						status = STATUS_SUCCESS;
	HANDLE							fileHandle	= NULL;
	IO_STATUS_BLOCK					ioStatusBlock;
	FILE_DISPOSITION_INFORMATION	file_disposition_info;
	OBJECT_ATTRIBUTES				objAttr;
	ULONG							attribute = OBJ_CASE_INSENSITIVE;
	ULONG							createDisposition;

	file_disposition_info.DeleteFile = TRUE;
	
#ifndef NTFOUR
	attribute |= OBJ_KERNEL_HANDLE; 
#endif

    // Initialize attributes
	InitializeObjectAttributes(	&objAttr, &Sftk_Lg->PStoreFileName, attribute,NULL,NULL);
	status = ZwCreateFile(	&fileHandle,
							DELETE, // | GENERIC_READ | SYNCHRONIZE | GENERIC_WRITE,   
							&objAttr,
							&ioStatusBlock,
							0, 
							FILE_ATTRIBUTE_NORMAL,
							FILE_SHARE_READ|FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							FILE_OPEN,	
							FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,	// FILE_DELETE_ON_CLOSE 
							NULL, 
							0);

    if ( !NT_SUCCESS(status) ) 
    { // failed to open or create pstore file...
        DebugPrint((DBG_ERROR, "sftk_delete_pstorefile: ZwCreateFile(LG 0x%08x PstoreFile Name %S) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, status));
		goto done;
    }

	status = ZwSetInformationFile(	fileHandle, 
									&ioStatusBlock,
									&file_disposition_info,
									sizeof(file_disposition_info),
									FileDispositionInformation);
	if (!NT_SUCCESS(status)) 
	{

		DebugPrint((DBG_ERROR, "sftk_delete_pstorefile: ZwSetInformationFile(LG 0x%08x PstoreFile Name %S) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, status));

		goto done;
	}

done:
	if (fileHandle)
		ZwClose(fileHandle);

	return status;
} // sftk_delete_pstorefile()


NTSTATUS
sftk_open_pstore( IN PUNICODE_STRING  FileName, OUT PHANDLE PtrFileHandle, BOOLEAN bCreate)
{
	NTSTATUS			status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES   objAttr;
	IO_STATUS_BLOCK     ioStatus;
	ULONG				attribute = OBJ_CASE_INSENSITIVE;
	ULONG				createDisposition;

#ifndef NTFOUR
	attribute |= OBJ_KERNEL_HANDLE; 
#endif
    // Initialize attributes
	InitializeObjectAttributes(	&objAttr,
								FileName,
								attribute,
                                NULL,
                                NULL);
	if (bCreate == TRUE)
	{
	   // If the file already exists, open it and overwrite it. 
	   // If it does not, create the given file
	   createDisposition	= FILE_OVERWRITE_IF; 
	}
	else
	{
		// If the file already exists, open it instead of creating a new file. 
		// If it does not, fail the request and do not create a new file
		createDisposition	= FILE_OPEN;
	}

    // By adding FILE_SYNCHR... We make sure that we only 
    // return after a writefile when the write file is complete!
    status = ZwCreateFile(	PtrFileHandle,
							GENERIC_READ|SYNCHRONIZE|GENERIC_WRITE,   
							&objAttr,
							&ioStatus,
							0, 
							FILE_ATTRIBUTE_NORMAL,
							FILE_SHARE_READ|FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							createDisposition, 
							FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
							NULL, 
							0);

    if ( !NT_SUCCESS(status) ) 
    { // failed to open or create pstore file...
        DebugPrint((DBG_ERROR, "sftk_open_pstore:: ZwCreateFile(%S) Failed with status 0x%08x !\n", 
								FileName->Buffer, status ));

        // FTD_ERR(FTD_WRNLVL, "Unable to open the pstore, RC = 0x%x.", RC);
    }

	return status;
} // sftk_open_pstore()


NTSTATUS
sftk_write_pstore(	HANDLE			FileHandle, 
					PLARGE_INTEGER	Offset,
					PVOID			Buffer, 
					ULONG			Size )
{
    NTSTATUS			status = STATUS_SUCCESS;
    LARGE_INTEGER       largeIntWait;
    IO_STATUS_BLOCK     ioStatus;
    ULONG               dwSize_p;

    // Count the number of pstore accesses 
    largeIntWait.QuadPart = -(10*1000*1000);  // relative 1 second
   
    status = ZwWriteFile(	FileHandle,	NULL,	NULL,	NULL, &ioStatus,
							Buffer,		Size,	Offset,	NULL);

	if (status == STATUS_PENDING)
    {
		DebugPrint((DBG_ERROR, "sftk_write_pstore: BUG FIXME FIXME ZwWriteFile(O : %I64d S : %d ) returns STATUS_PENDING, status=0x%08x!\n",
									Offset->QuadPart, Size, status));
		OS_ASSERT(FALSE);
        // If status is pending, wait till the write has finished or 1 second timeout occurs!
        ZwWaitForSingleObject(FileHandle, FALSE, &largeIntWait);
    }

    if ( !NT_SUCCESS(status) )
    {
        DebugPrint((DBG_ERROR, "sftk_write_pstore: ZwWriteFile(O %I64d S : %d) Unable to write to the pstore status=0x%08x!\n",
									 Offset->QuadPart, Size, status));
        // TODO : Log Event here     // FTD_ERR(FTD_WRNLVL, "Unable to write to the pstore, RC = 0x%x.", RC);
    }
    return status;
} // sftk_write_pstore()

NTSTATUS
sftk_read_pstore(	HANDLE			FileHandle, 
					PLARGE_INTEGER	Offset,
					PVOID			Buffer, 
					ULONG			Size )
{
    NTSTATUS			status = STATUS_SUCCESS;
    LARGE_INTEGER       largeIntWait;
    IO_STATUS_BLOCK     ioStatus;
    ULONG               dwSize_p;

    // Count the number of pstore accesses 
    largeIntWait.QuadPart = -(10*1000*1000);  // relative 1 second
   
    status = ZwReadFile(	FileHandle,	NULL,	NULL,	NULL, &ioStatus,
							Buffer,		Size,	Offset,	NULL);

	if (status == STATUS_PENDING)
    {
		DebugPrint((DBG_ERROR, "sftk_read_pstore: BUG FIXME FIXME ZwReadFile(O : %I64d S : %d ) returns STATUS_PENDING, status=0x%08x!\n",
									Offset->QuadPart, Size, status));
		OS_ASSERT(FALSE);
        // If status is pending, wait till the write has finished or 1 second timeout occurs!
        ZwWaitForSingleObject(FileHandle, FALSE, &largeIntWait);
    }

    if ( !NT_SUCCESS(status) )
    {
        DebugPrint((DBG_ERROR, "sftk_read_pstore: ZwWriteFile(O %I64d S : %d) Unable to write to the pstore status=0x%08x!\n",
									 Offset->QuadPart, Size, status));
        // TODO : Log Event here     // FTD_ERR(FTD_WRNLVL, "Unable to write to the pstore, RC = 0x%x.", RC);
    }
    return status;
} // sftk_read_pstore()

//
// From Buffer it calculates checksum and returns checksu value
// NOTE: If buffer has checksum field, make it zero before calling this API
//
NTSTATUS
sftk_get_checksum( PUCHAR Buffer, ULONG Size, PLARGE_INTEGER	Checksum)
{
	ULONG	i, j, onebitCount;

	Checksum->QuadPart = 0;

	// TODO : Enhance this logic for checksum count
	// using number of 1 bits in whole buffer
	for (i=0; i < Size; i++)
	{
		for (j=0, onebitCount = 0; j < (sizeof(UCHAR) * 8); j++)
		{
			onebitCount += ( (Buffer[i] >> j) & 0x1);
		}
		Checksum->QuadPart += onebitCount;
	}
	
	return STATUS_SUCCESS;
} // sftk_get_checksum()

NTSTATUS
sftk_do_validation_PS_HDR( IN PSFTK_PS_HDR	PsHdr, IN ULONG Size)
{
	NTSTATUS		status	= STATUS_IMAGE_CHECKSUM_MISMATCH;
	LARGE_INTEGER	newChecksum, oldChecksum;

	// -- Do Checksum and all fields validation on Read SFTK_PS_HDR data 
	if (PsHdr->MagicNum != SFTK_PS_VERSION_1_MAGIC)	
	{
		DebugPrint((DBG_ERROR, "sftk_do_validation_PS_HDR:: PSHDR (PsHdr->MagicNum 0x%08x != SFTK_PS_VERSION_1_MAGIC 0x%08x ) returning error 0x%08x!\n", 
															PsHdr->MagicNum,  SFTK_PS_VERSION_1_MAGIC, status ));
		goto done;
	}
	if (PsHdr->MajorVer != SFTK_PS_MAJOR_VER)	
	{
		DebugPrint((DBG_ERROR, "sftk_do_validation_PS_HDR:: PSHDR (PsHdr->MajorVer 0x%08x != SFTK_PS_MAJOR_VER 0x%08x ) returning error 0x%08x!\n", 
															PsHdr->MajorVer,  SFTK_PS_MAJOR_VER, status ));
		goto done;
	}
	if (PsHdr->MinorVer != SFTK_PS_MINOR_VER)	
	{
		DebugPrint((DBG_ERROR, "sftk_do_validation_PS_HDR:: PSHDR (PsHdr->MinorVer 0x%08x != SFTK_PS_MINOR_VER 0x%08x ) returning error 0x%08x!\n", 
															PsHdr->MinorVer,  SFTK_PS_MINOR_VER, status ));
		goto done;
	}
	if (PsHdr->ExtraVer != SFTK_PS_EXTRA_VER)	
	{
		DebugPrint((DBG_ERROR, "sftk_do_validation_PS_HDR:: PSHDR (PsHdr->ExtraVer 0x%08x != SFTK_PS_EXTRA_VER 0x%08x ) returning error 0x%08x!\n", 
															PsHdr->ExtraVer,  SFTK_PS_EXTRA_VER, status ));
		goto done;
	}
	if (PsHdr->MagicNum != SFTK_PS_VERSION_1_MAGIC)	
	{
		DebugPrint((DBG_ERROR, "sftk_do_validation_PS_HDR:: PSHDR (PsHdr->MagicNum 0x%08x != SFTK_PS_VERSION_1_MAGIC 0x%08x ) returning error 0x%08x!\n", 
															PsHdr->MagicNum,  SFTK_PS_VERSION_1_MAGIC, status ));
		goto done;
	}
	if (PsHdr->MagicNum != SFTK_PS_VERSION_1_MAGIC)	
	{
		DebugPrint((DBG_ERROR, "sftk_do_validation_PS_HDR:: PSHDR (PsHdr->MagicNum 0x%08x != SFTK_PS_VERSION_1_MAGIC 0x%08x ) returning error 0x%08x!\n", 
															PsHdr->MagicNum,  SFTK_PS_VERSION_1_MAGIC, status ));
		goto done;
	}

	if (PsHdr->Checksum == 0)	
	{
		DebugPrint((DBG_ERROR, "sftk_do_validation_PS_HDR:: PSHDR (PsHdr->Checksum 0x%08x == 0 ) returning error 0x%08x!\n", 
															PsHdr->Checksum,  status ));
		goto done;
	}
	
	// Validate Checksum of PsHdr struct 
	oldChecksum.QuadPart = PsHdr->Checksum;
	newChecksum.QuadPart = 0; 

	PsHdr->Checksum = 0;
	sftk_get_checksum( (PUCHAR) PsHdr, sizeof(SFTK_PS_HDR), &newChecksum);

	if (oldChecksum.QuadPart != newChecksum.QuadPart)	
	{
		DebugPrint((DBG_ERROR, "sftk_do_validation_PS_HDR:: PSHDR (PsHdr oldChecksum.QuadPart 0x%I64x != newChecksum.QuadPart 0x%I64x ) returning error 0x%08x!\n", 
															oldChecksum.QuadPart, newChecksum.QuadPart, status ));
		goto done;
	}
	
	status = STATUS_SUCCESS;
done:
	return status;
} // sftk_do_validation_PS_HDR()

NTSTATUS
sftk_do_validation_PS_DEV( IN PSFTK_PS_DEV	PsDev, IN ULONG Size)
{
	NTSTATUS		status	= STATUS_IMAGE_CHECKSUM_MISMATCH;
	LARGE_INTEGER	newChecksum, oldChecksum;

	// Validate Checksum of PsHdr struct 
	oldChecksum.QuadPart = PsDev->Checksum;
	newChecksum.QuadPart = 0; 

	PsDev->Checksum = 0;
	sftk_get_checksum( (PUCHAR) PsDev, sizeof(SFTK_PS_DEV), &newChecksum);

	if (oldChecksum.QuadPart != newChecksum.QuadPart)	
	{
		DebugPrint((DBG_ERROR, "sftk_do_validation_PS_DEV:: PSDEV (oldChecksum.QuadPart 0x%I64x != newChecksum.QuadPart 0x%I64x ) returning error 0x%08x!\n", 
															oldChecksum.QuadPart, newChecksum.QuadPart, status ));
		goto done;
	}
	
	status = STATUS_SUCCESS;
done:
	return status;
} // sftk_do_validation_PS_DEV()

#if PS_BITMAP_CHECKSUM_ON
NTSTATUS
sftk_do_checksum_validation_of_PS_Bitmap( IN PSFTK_PS_BITMAP	PsBitmap, IN PUCHAR BitmapMem,  IN ULONG BitmapSize)
{
	NTSTATUS		status	= STATUS_IMAGE_CHECKSUM_MISMATCH;
	LARGE_INTEGER	newChecksum, oldChecksum;

	// Validate Checksum of PsHdr struct 
	oldChecksum.QuadPart = PsBitmap->Checksum;
	newChecksum.QuadPart = 0; 

	sftk_get_checksum( (PUCHAR) BitmapMem, BitmapSize, &newChecksum);

	if (oldChecksum.QuadPart != newChecksum.QuadPart)	
	{
		DebugPrint((DBG_ERROR, "sftk_do_checksum_validation_of_PS_Bitmap:: Bitmap (oldChecksum.QuadPart 0x%I64x != newChecksum.QuadPart 0x%I64x ) returning error 0x%08x!\n", 
															oldChecksum.QuadPart, newChecksum.QuadPart, status ));
		goto done;
	}
	
	status = STATUS_SUCCESS;
done:
	return status;
} // sftk_do_checksum_validation_of_PS_Bitmap()
#endif

NTSTATUS
sftk_flush_psHdr_to_pstore(	IN			PSFTK_LG	Sftk_Lg, 
							IN			BOOLEAN		HandleValid,  // TRUE means FileHandle is valid used it
							IN OPTIONAL HANDLE		FileHandle,	  
							IN OPTIONAL BOOLEAN		UseCheckSum) // TRUE means cal. Checksum and write it to pstore file
{
	NTSTATUS		status		= STATUS_SUCCESS;
	LARGE_INTEGER	offset, checksum;
	HANDLE			fileHandle = FileHandle;	

	OS_ASSERT(Sftk_Lg != NULL);
	OS_ASSERT(Sftk_Lg->PsHeader!= NULL);
	
	OS_ASSERT(UseCheckSum == TRUE);

	Sftk_Lg->PsHeader->LgInfo.state = Sftk_Lg->state;
	Sftk_Lg->PsHeader->LgInfo.PrevState = Sftk_Lg->PrevState;
	Sftk_Lg->PsHeader->LgInfo.bInconsistantData = Sftk_Lg->bInconsistantData;
	Sftk_Lg->PsHeader->LgInfo.UserChangedToTrackingMode = Sftk_Lg->UserChangedToTrackingMode;

	if (HandleValid == FALSE) 
	{
		OS_ASSERT(FileHandle == NULL);

		status = sftk_open_pstore( &Sftk_Lg->PStoreFileName , &fileHandle, FALSE);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			DebugPrint((DBG_ERROR, "sftk_flush_psHdr_to_pstore:: sftk_open_pstore(%S, LGNum 0x%08x ) Failed with status 0x%08x !\n", 
									Sftk_Lg->PStoreFileName.Buffer, 
									Sftk_Lg->LGroupNumber, status ));
			// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
			return status;
		}
	}


	if (UseCheckSum == TRUE)
	{
		// calculate total Checksum of Psedv struct 
		Sftk_Lg->PsHeader->Checksum = 0;
		sftk_get_checksum( (PUCHAR) Sftk_Lg->PsHeader, sizeof(SFTK_PS_HDR), &checksum);
		Sftk_Lg->PsHeader->Checksum = checksum.QuadPart;
	}
		
#if TARGET_SIDE
	// copyt Role Mode and Secondary info from LG to Pstore LG
	RtlCopyMemory( &Sftk_Lg->PsHeader->LgInfo.Role, &Sftk_Lg->Role, sizeof(Sftk_Lg->Role));
	RtlCopyMemory( &Sftk_Lg->PsHeader->LgInfo.Secondary, &Sftk_Lg->Secondary, sizeof(Sftk_Lg->Secondary));
#endif

	// Write Psdev bitmaps to Pstore file.
	offset.QuadPart = OFFSET_SFTK_PS_HDR;
	status = sftk_write_pstore( fileHandle, 
								&offset, 
								Sftk_Lg->PsHeader,
								Sftk_Lg->SizeBAlignPsHeader );
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_flush_psHdr_to_pstore:: PSHDR sftk_write_pstore(%S, LG 0x%08x for O %I64d S : %d ) Failed with status 0x%08x !\n", 
															Sftk_Lg->PStoreFileName.Buffer, Sftk_Lg->LGroupNumber, 
															offset.QuadPart, Sftk_Lg->SizeBAlignPsHeader, status ));
		goto done;
	}

done:	
	if ( (HandleValid == FALSE) && (fileHandle) )
	{
		ZwClose(fileHandle);
	}

	return status;
} // sftk_flush_psHdr_to_pstore()

NTSTATUS
sftk_flush_psDev_to_pstore(	IN			PSFTK_DEV	Sftk_Dev, 
							IN			BOOLEAN		HandleValid,  // TRUE means FileHandle is valid used it
							IN OPTIONAL HANDLE		FileHandle,	  
							IN OPTIONAL BOOLEAN		UseCheckSum) // TRUE means cal. Checksum and write it to pstore file
{
	NTSTATUS		status		= STATUS_SUCCESS;
	LARGE_INTEGER	offset, checksum;
	HANDLE			fileHandle = FileHandle;	

	OS_ASSERT(Sftk_Dev->SftkLg != NULL);
	OS_ASSERT(Sftk_Dev->PsDev != NULL);
	
	OS_ASSERT(UseCheckSum == TRUE);

	if (HandleValid == FALSE) 
	{
		OS_ASSERT(FileHandle == NULL);

		status = sftk_open_pstore( &Sftk_Dev->SftkLg->PStoreFileName , &fileHandle, FALSE);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			DebugPrint((DBG_ERROR, "sftk_flush_psDev_to_pstore:: sftk_open_pstore(%S, Sftk_Dev 0x%08x for Device %s ) Failed with status 0x%08x !\n", 
									Sftk_Dev->SftkLg->PStoreFileName.Buffer, Sftk_Dev, Sftk_Dev->Vdevname, status ));
			DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just continue the operations...\n"));
			return status;
		}
	}


	if (UseCheckSum == TRUE)
	{
		// calculate total Checksum of Psedv struct 
		Sftk_Dev->PsDev->Checksum = 0;
		sftk_get_checksum( (PUCHAR) Sftk_Dev->PsDev, sizeof(SFTK_PS_DEV), &checksum);
		Sftk_Dev->PsDev->Checksum = checksum.QuadPart;
	}
		
	// Write Psdev bitmaps to Pstore file.
	offset.QuadPart = Sftk_Dev->OffsetOfPsDev;
	status = sftk_write_pstore( fileHandle, 
								&offset, 
								Sftk_Dev->PsDev,
								Sftk_Dev->SizeBAlignPsDev);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_flush_psDev_to_pstore:: PsDev : sftk_write_pstore(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
															Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
															Sftk_Dev->SftkLg->LGroupNumber, 
															Sftk_Dev->Vdevname,
															offset.QuadPart, 
															Sftk_Dev->SizeBAlignPsDev, status ));
		goto done;
	}
	
done:	
	if ( (HandleValid == FALSE) && (fileHandle) )
	{
		ZwClose(fileHandle);
	}
	return status;
} // sftk_flush_psDev_to_pstore()

NTSTATUS
sftk_flush_all_bitmaps_to_pstore(	IN			PSFTK_DEV	Sftk_Dev, 
									IN			BOOLEAN		HandleValid,  // TRUE means FileHandle is valid used it
									IN OPTIONAL HANDLE		FileHandle,	  
									IN OPTIONAL BOOLEAN		UseCheckSum, // TRUE means cal. Checksum and write it to pstore file
									IN OPTIONAL BOOLEAN		UpdatePsDevWithChecksum) // TRUE means Flush Psdev with new checksum
{
	NTSTATUS		status		= STATUS_SUCCESS;
	LARGE_INTEGER	offset, checksum;
	HANDLE			fileHandle = FileHandle;	

	OS_ASSERT(Sftk_Dev->SftkLg != NULL);
	OS_ASSERT(Sftk_Dev->PsDev != NULL);
	
	if (HandleValid == FALSE) 
	{
		OS_ASSERT(FileHandle == NULL);
		status = sftk_open_pstore( &Sftk_Dev->SftkLg->PStoreFileName , &fileHandle, FALSE);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			DebugPrint((DBG_ERROR, "sftk_flush_all_bitmaps_to_pstore:: sftk_open_pstore(%S, Sftk_Dev 0x%08x for Device %s ) Failed with status 0x%08x !\n", 
									Sftk_Dev->SftkLg->PStoreFileName.Buffer, Sftk_Dev, Sftk_Dev->Vdevname, status ));
			DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just continue the operations...\n"));
			return status;
		}
	}

	// Flush LRDB bitmap 
#if PS_BITMAP_CHECKSUM_ON
	if (UseCheckSum == TRUE)
	{ // calculate LRDB bitmap memory checksum and stored it into psDev
		Sftk_Dev->PsDev->Lrdb.Checksum = 0;
		sftk_get_checksum( (PUCHAR) Sftk_Dev->Lrdb.pBits, Sftk_Dev->Lrdb.BitmapSize, &checksum);
		Sftk_Dev->PsDev->Lrdb.Checksum = checksum.QuadPart;
	}
#endif

	// Write Lrdb bitmaps to Pstore file.
	offset.QuadPart = Sftk_Dev->PsDev->LrdbOffset;

#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
	// Update SFTK_PS_BITMAP_REGION_HDR
	OS_ASSERT( Sftk_Dev->Lrdb.pPsBitmapHdr->MagicNum == SFTK_PS_BITMAP_REGION_MAGIC);
	OS_PerfGetClock( &Sftk_Dev->Lrdb.pPsBitmapHdr->data.TimeStamp, NULL);
	status = sftk_write_pstore( fileHandle, 
								&offset, 
								Sftk_Dev->Lrdb.pPsBitmapHdr,
								Sftk_Dev->Lrdb.BitmapSizeBlockAlign);
#else
	status = sftk_write_pstore( fileHandle, 
								&offset, 
								Sftk_Dev->Lrdb.pBits,
								Sftk_Dev->Lrdb.BitmapSizeBlockAlign);
#endif
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_flush_all_bitmaps_to_pstore:: LRDB : sftk_write_pstore(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
															Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
															Sftk_Dev->SftkLg->LGroupNumber, 
															Sftk_Dev->Vdevname,
															offset.QuadPart, 
															Sftk_Dev->Lrdb.BitmapSizeBlockAlign, status ));
		goto done;
	}

	// Flush HRDB bitmap 
#if PS_BITMAP_CHECKSUM_ON
	if (UseCheckSum == TRUE)
	{ // calculate HRDB bitmap memory checksum and stored it into psDev
		Sftk_Dev->PsDev->Hrdb.Checksum = 0;
		sftk_get_checksum( (PUCHAR) Sftk_Dev->Hrdb.pBits, Sftk_Dev->Hrdb.BitmapSize, &checksum);
		Sftk_Dev->PsDev->Hrdb.Checksum = checksum.QuadPart;
	}
#endif

	// Write Hrdb bitmaps to Pstore file.
	offset.QuadPart = Sftk_Dev->PsDev->HrdbOffset;
	
#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
	// Update SFTK_PS_BITMAP_REGION_HDR
	OS_ASSERT( Sftk_Dev->Hrdb.pPsBitmapHdr->MagicNum == SFTK_PS_BITMAP_REGION_MAGIC);
	OS_PerfGetClock( &Sftk_Dev->Hrdb.pPsBitmapHdr->data.TimeStamp, NULL);
	HRDB_BITMAP_TIMESTAMP(Sftk_Dev->Hrdb.pPsBitmapHdr->data.TimeStamp.QuadPart);

	status = sftk_write_pstore( fileHandle, 
								&offset, 
								Sftk_Dev->Hrdb.pPsBitmapHdr,
								Sftk_Dev->Hrdb.BitmapSizeBlockAlign);
#else
	status = sftk_write_pstore( fileHandle, 
								&offset, 
								Sftk_Dev->Hrdb.pBits,
								Sftk_Dev->Hrdb.BitmapSizeBlockAlign);
#endif
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_flush_all_bitmaps_to_pstore:: HRDB : sftk_write_pstore(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
															Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
															Sftk_Dev->SftkLg->LGroupNumber, 
															Sftk_Dev->Vdevname,
															offset.QuadPart, 
															Sftk_Dev->Hrdb.BitmapSizeBlockAlign, status ));
		goto done;
	}

	if (UpdatePsDevWithChecksum == TRUE)
	{ // Flush PsDev with new calculated Checksum info
#if PS_BITMAP_CHECKSUM_ON
		OS_ASSERT(UseCheckSum == TRUE);
#endif
		status = sftk_flush_psDev_to_pstore( Sftk_Dev, TRUE, fileHandle, UseCheckSum);
		
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_flush_all_bitmaps_to_pstore:: PsDev : sftk_flush_psDev_to_pstore(%S, LG 0x%08x for Device %s) Failed with status 0x%08x !\n", 
																Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
																Sftk_Dev->SftkLg->LGroupNumber, 
																Sftk_Dev->Vdevname, 
																status ));
			goto done;
		}
	} // if (UpdatePsDevWithChecksum == TRUE)

done:	
	if ( (HandleValid == FALSE) && (fileHandle) )
	{
		ZwClose(fileHandle);
	}
	return status;
} // sftk_flush_Lrdb_to_pstore()

// This API Not used anymore
NTSTATUS
sftk_init_Lg_and_devs_from_Pstore(	IN PUNICODE_STRING	FileName,
									IN HANDLE			FileHandle ) 
{
	NTSTATUS		status		= STATUS_SUCCESS;
	PSFTK_LG		pSftk_Lg	= NULL;
	PSFTK_DEV		pSftk_Dev	= NULL;
	LARGE_INTEGER	offset, offset1, checksum;
	PSFTK_PS_HDR	pPsHdr		= NULL;
	PSFTK_PS_DEV	pPsDev		= NULL;
	ULONG			i, size;
	ANSI_STRING		ansiString;
	ftd_lg_info_t	lg_info;
	ftd_dev_info_t  dev_info;
	BOOLEAN			bMarkLG_FullRefresh = FALSE;
	BOOLEAN			bLrdbIsValid		= TRUE;

	// - Allocate Sector Align strcuture SFTK_PS_HDR memory, initialized and write it to pstore file
	size = BLOCK_ALIGN_UP(sizeof(SFTK_PS_HDR));

	pPsHdr = OS_AllocMemory(NonPagedPool, size);
	if (pPsHdr == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore: OS_AllocMemory(PstoreFile %S, sizeof Mem %d ) Failed with status 0x%08x \n", 
											FileName->Buffer, size, status));
		goto done;
	}

	OS_ZeroMemory(pPsHdr, size);

	// Read PsHdr from Pstore file.
	offset.QuadPart = OFFSET_SFTK_PS_HDR;
	status = sftk_read_pstore( FileHandle, &offset, pPsHdr,size );
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: PSHDR sftk_read_pstore(%S, O %I64d S : %d ) Failed with status 0x%08x !\n", 
															FileName->Buffer,  offset.QuadPart, size, status ));
		// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
		goto done;
	}

	status = sftk_do_validation_PS_HDR(	pPsHdr, size );
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: PSHDR sftk_do_validation_PS_HDR(%S, O %I64d S : %d ) Failed with status 0x%08x !\n", 
															FileName->Buffer,  offset.QuadPart, size, status ));
		// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
		goto done;
	}

	// -- Create LG group from this PsHdr
	lg_info.lgdev = pPsHdr->LgInfo.LGroupNumber;

	status = RtlUnicodeStringToAnsiString( &ansiString, FileName, TRUE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: RtlUnicodeStringToAnsiString(UnicodeString %S ) Failed with status 0x%08x !\n", 
															FileName->Buffer,  status ));
		// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
		goto done;
	}
	OS_ZeroMemory( lg_info.vdevname, sizeof(lg_info.vdevname));
	OS_RtlCopyMemory( lg_info.vdevname, ansiString.Buffer, ansiString.Length);
	lg_info.statsize = pPsHdr->LgInfo.statsize;
#if TARGET_SIDE
	lg_info.lgCreationRole = pPsHdr->LgInfo.Role.CreationRole;
	RtlCopyMemory( lg_info.JournalPath, pPsHdr->LgInfo.Role.JPath, sizeof(pPsHdr->LgInfo.Role.JPath));
#endif
	// -- create a LG device and its relative threads 
	OS_ASSERT(GSftk_Config.DriverObject != NULL);
	status = sftk_Create_InitializeLGDevice( GSftk_Config.DriverObject, 
											 &lg_info,
											 &pSftk_Lg,
											 FALSE,
											 FALSE);

	RtlFreeAnsiString( &ansiString );
    if ( !NT_SUCCESS(status) )
    {
		DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore: sftk_Create_InitializeLGDevice (Pstore File %S Lgdev number = %d) Failed to create!!! returning error 0x%08x \n",
							FileName->Buffer, lg_info.lgdev));
        goto done;
    }

	// Now Update Created LG structure from Pstore structure

	pSftk_Lg->PsHeader				= pPsHdr;
	pSftk_Lg->SizeBAlignPsHeader	= size;
	
	OS_ASSERT( pPsHdr->LgInfo.LGroupNumber	== pSftk_Lg->LGroupNumber);

	pPsHdr->LgInfo.TotalNumDevices	= 0; // Sftk_Lg->LgDev_List.NumOfNodes;	

	pSftk_Lg->statsize		= pPsHdr->LgInfo.statsize;
	pSftk_Lg->sync_depth	= pPsHdr->LgInfo.sync_depth;
	pSftk_Lg->sync_timeout	= pPsHdr->LgInfo.sync_timeout;
	pSftk_Lg->iodelay 		= pPsHdr->LgInfo.iodelay;

	// Do not change state here, first read and configured all devices than update state.
	// for default we changed the state to tracking in case 2nd or other device config fails. 
	pSftk_Lg->state = SFTK_MODE_TRACKING;

	// -- Prepare all devices in loop
	offset.QuadPart = OFFSET_FIRST_SFTK_PS_DEV(pSftk_Lg->SizeBAlignPsHeader);

	for (i=0; i < pPsHdr->LgInfo.TotalNumDevices; i++)
	{ // for each and every pstore device create and configure device.
		bLrdbIsValid = TRUE;

		if (offset.QuadPart > pPsHdr->LastOffsetOfPstoreFile)
		{
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore: FIXME FIXME Lgdev number %d, PstoreFile %S (offset.QuadPart %I64d > pPsHdr->LastOffsetOfPstoreFile %I64d )  Failed Corrupted Pstore File!!! Continuing LG prepare operations !!\n",
											pSftk_Lg->LGroupNumber, FileName->Buffer, 
											offset.QuadPart, pPsHdr->LastOffsetOfPstoreFile));
			break;
		}

		// Allocate next PS_DEV info
		size = BLOCK_ALIGN_UP(sizeof(SFTK_PS_DEV));

		pPsDev = OS_AllocMemory(NonPagedPool, size);
		if (pPsDev == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore: OS_AllocMemory(PS_DEV PstoreFile %S, sizeof Mem %d ) Failed with status 0x%08x \n", 
												FileName->Buffer, size, status));
			if (i == 0)
				goto done;
			else
				goto done_igonre_error;
		}

		OS_ZeroMemory(pPsDev, size);

		// Read PsDev from Pstore file.
		status = sftk_read_pstore( FileHandle, &offset, pPsDev, size );
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: PSDEV sftk_read_pstore(%S, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																FileName->Buffer,  offset.QuadPart, size, status ));
			// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
			if (i == 0)
				goto done;
			else
				goto done_igonre_error;
		}

		// Do validation and checksum on read Psdev info
		status = sftk_do_validation_PS_DEV(	pPsDev, size );
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: PSDEV sftk_do_validation_PS_DEV(%S, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																FileName->Buffer,  offset.QuadPart, size, status ));
			// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
			if (i == 0)
				goto done;
			else
				goto done_igonre_error;
		}

		if (pPsDev->Deleted == TRUE)
		{
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: PSDEV (pPsDev->Deleted == TRUE) (psDev->VDevname %s, psDev->Devnum: %d ) skipping this Dev continuing to next one !\n", 
																pPsDev->Vdevname, pPsDev->cdev ));
			// update next offset to read
			offset.QuadPart = OFFSET_NEXT_SFTK_PS_DEV(	offset.QuadPart, 
														pPsDev->TotalSizeOfDevRegion);
			OS_FreeMemory(pPsDev);
			pPsDev = NULL;
			continue;
		}
		
		OS_ASSERT( pPsDev->TotalSizeOfDevRegion = TOTAL_PS_DEV_REGION_SIZE( pPsDev->SizeInbAllign, 
																			pPsDev->Lrdb.BitmapSizeBlockAlign,
																			pPsDev->Hrdb.BitmapSizeBlockAlign));
		OS_ASSERT( pPsDev->LrdbOffset == OFFSET_SFTK_LRDB( offset.QuadPart, size) );
		OS_ASSERT( pPsDev->HrdbOffset == OFFSET_SFTK_LRDB( pPsDev->LrdbOffset, pPsDev->Lrdb.BitmapSizeBlockAlign) );


		// -- create SFTK_DEV from pstore Dev info
		// Prepare dev_info struct which is used to create SFTK_DEV 

		// 
		// TODO : FIXME FIXME : Improve Bitmap logic build up operation during boot time
		// Current logic is based on pld service and driver IOCTL !!!
		//
		OS_ZeroMemory( &dev_info, sizeof(dev_info));

		dev_info.lgnum			= pSftk_Lg->LGroupNumber;
		dev_info.localcdev		= pPsDev->cdev;	// Not Used
		dev_info.cdev			= pPsDev->cdev;
		dev_info.bdev			= pPsDev->bdev;
		dev_info.disksize		= pPsDev->Disksize;

		dev_info.lrdbsize32		= pPsDev->Lrdb.len32;	// FIXME FIXME : To support Old Code !!
		dev_info.hrdbsize32		= pPsDev->Hrdb.len32;
		dev_info.lrdb_res		= 0;							// Not used TODO remove this
		dev_info.hrdb_res		= 0;							// Not used TODO remove this
		dev_info.lrdb_numbits	= pPsDev->Lrdb.TotalNumOfBits;	// TODO : USed efficiently
		dev_info.hrdb_numbits	= pPsDev->Hrdb.TotalNumOfBits;

		dev_info.statsize		= pPsDev->statesize;
		dev_info.lrdb_offset	= 0;						// no use anymore

		strcpy( dev_info.devname, pPsDev->Devname);
		strcpy( dev_info.vdevname, pPsDev->Vdevname);

		dev_info.bUniqueVolumeIdValid	= pPsDev->bUniqueVolumeIdValid;
		dev_info.UniqueIdLength			= pPsDev->UniqueIdLength;
		RtlCopyMemory( dev_info.UniqueId, pPsDev->UniqueId, sizeof(dev_info.UniqueId));

		dev_info.bSignatureUniqueVolumeIdValid	= pPsDev->bSignatureUniqueVolumeIdValid;
		dev_info.SignatureUniqueIdLength		= pPsDev->SignatureUniqueIdLength;
		RtlCopyMemory( dev_info.SignatureUniqueId, pPsDev->SignatureUniqueId, sizeof(dev_info.SignatureUniqueId));

		dev_info.bSuggestedDriveLetterLinkValid	= pPsDev->bSuggestedDriveLetterLinkValid;
		dev_info.SuggestedDriveLetterLinkLength	= pPsDev->SuggestedDriveLetterLinkLength;
		RtlCopyMemory( dev_info.SuggestedDriveLetterLink, pPsDev->SuggestedDriveLetterLink, sizeof(dev_info.SuggestedDriveLetterLink));

#if TARGET_SIDE
		dev_info.lgCreationRole = pPsHdr->LgInfo.Role.CreationRole;
#endif


		// Create SFTK_DEV
		pSftk_Dev = NULL;
		status = sftk_CreateInitialize_SftekDev( &dev_info, &pSftk_Dev, FALSE, FALSE, TRUE);
		if (!NT_SUCCESS(status)) 
		{ // failed
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore: sftk_CreateInitialize_SftekDev(Device - %s) Failed with status 0x%08x !!! \n",
									dev_info.vdevname, status));
			// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
			if (i == 0)
				goto done;
			else
				goto done_igonre_error;
		}
		// Now update pSFTK_Dev from pstore file
		OS_ASSERT( pSftk_Dev->SftkLg != NULL);

		OS_ASSERT( pPsDev->Disksize	== pSftk_Dev->Disksize);

		OS_ASSERT( pPsDev->Lrdb.Sectors_per_bit			== pSftk_Dev->Lrdb.Sectors_per_bit); 
		OS_ASSERT( pPsDev->Lrdb.TotalNumOfBits			== pSftk_Dev->Lrdb.TotalNumOfBits);
		OS_ASSERT( pPsDev->Lrdb.BitmapSize				== pSftk_Dev->Lrdb.BitmapSize);
		OS_ASSERT( pPsDev->Lrdb.BitmapSizeBlockAlign	== pSftk_Dev->Lrdb.BitmapSizeBlockAlign);
		OS_ASSERT( pPsDev->Lrdb.len32					== pSftk_Dev->Lrdb.len32);

		OS_ASSERT( pPsDev->Hrdb.Sectors_per_bit			== pSftk_Dev->Hrdb.Sectors_per_bit); 
		OS_ASSERT( pPsDev->Hrdb.TotalNumOfBits			== pSftk_Dev->Hrdb.TotalNumOfBits);
		OS_ASSERT( pPsDev->Hrdb.BitmapSize				== pSftk_Dev->Hrdb.BitmapSize);
		OS_ASSERT( pPsDev->Hrdb.BitmapSizeBlockAlign	== pSftk_Dev->Hrdb.BitmapSizeBlockAlign);
		OS_ASSERT( pPsDev->Hrdb.len32					== pSftk_Dev->Hrdb.len32);

		pSftk_Dev->PsDev				= pPsDev;
		pSftk_Dev->SizeBAlignPsDev		= size;
		pSftk_Dev->OffsetOfPsDev		= (ULONG) offset.QuadPart;
		
		pSftk_Dev->RefreshLastBitIndex = pPsDev->RefreshLastBitIndex; // TODO FIXME FIXME

		// Read LRDB Bitmap in memory 
		offset1.QuadPart = pPsDev->LrdbOffset;
		
#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
		OS_ASSERT( pSftk_Dev->Lrdb.pPsBitmapHdr != NULL);
		status = sftk_read_pstore( FileHandle, 
									&offset1, 
									pSftk_Dev->Lrdb.pPsBitmapHdr,
									pSftk_Dev->Lrdb.BitmapSizeBlockAlign);
#else
		status = sftk_read_pstore( FileHandle, 
									&offset1, 
									pSftk_Dev->Lrdb.pBits,
									pSftk_Dev->Lrdb.BitmapSizeBlockAlign);
#endif
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: LRDB : sftk_read_pstore(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																pSftk_Dev->SftkLg->PStoreFileName.Buffer, 
																pSftk_Dev->SftkLg->LGroupNumber, 
																pSftk_Dev->Vdevname,
																offset1.QuadPart, 
																pSftk_Dev->Lrdb.BitmapSizeBlockAlign, status ));
			// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
			if (i == 0)
				goto done;
			else
				goto done_igonre_error;
		}

#if PS_BITMAP_CHECKSUM_ON
		// Double check Checksum of LRDB bitmap Memory !!
		status = sftk_do_checksum_validation_of_PS_Bitmap( &pPsDev->Lrdb, (PUCHAR) pSftk_Dev->Lrdb.pBits, pSftk_Dev->Lrdb.BitmapSize);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: LRDB : sftk_do_checksum_validation_of_PS_Bitmap(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																pSftk_Dev->SftkLg->PStoreFileName.Buffer, 
																pSftk_Dev->SftkLg->LGroupNumber, 
																pSftk_Dev->Vdevname,
																offset1.QuadPart, 
																pSftk_Dev->Lrdb.BitmapSize, status ));
			// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
			// TODO FIXME FIXME : We must ignore this error, and mark LG as full refresh requires !!!
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: Pstore LRDB Bitmap memory is corrupted checksum failed!!: TODO FIXME FIXME : We must ignore this error, and mark LG as full refresh requires !!! \n"));
			bMarkLG_FullRefresh = TRUE;
			bLrdbIsValid = FALSE;
		}
#endif

#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
		// No need to verify LRDB since LRDB bitmap always must valid
		OS_ASSERT( pSftk_Dev->Lrdb.pPsBitmapHdr->MagicNum == SFTK_PS_BITMAP_REGION_MAGIC);
		OS_ASSERT( pSftk_Dev->Lrdb.pPsBitmapHdr->data.TimeStamp.QuadPart != 0);
#endif
		// Read HRDB Bitmap in memory 
		offset1.QuadPart = pPsDev->HrdbOffset;

#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
		OS_ASSERT( pSftk_Dev->Lrdb.pPsBitmapHdr != NULL);

		status = sftk_read_pstore(  FileHandle, 
									&offset1, 
									pSftk_Dev->Hrdb.pPsBitmapHdr,
									pSftk_Dev->Hrdb.BitmapSizeBlockAlign);
#else
		status = sftk_read_pstore(  FileHandle, 
									&offset1, 
									pSftk_Dev->Hrdb.pBits,
									pSftk_Dev->Hrdb.BitmapSizeBlockAlign);
#endif
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: LRDB : sftk_read_pstore(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																pSftk_Dev->SftkLg->PStoreFileName.Buffer, 
																pSftk_Dev->SftkLg->LGroupNumber, 
																pSftk_Dev->Vdevname,
																offset1.QuadPart, 
																pSftk_Dev->Hrdb.BitmapSizeBlockAlign, status ));
			// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
			if (i == 0)
				goto done;
			else
				goto done_igonre_error;
		}

#if PS_BITMAP_CHECKSUM_ON
		// Double check Checksum of HRDB bitmap Memory !!
		status = sftk_do_checksum_validation_of_PS_Bitmap( &pPsDev->Hrdb, (PUCHAR) pSftk_Dev->Hrdb.pBits, pSftk_Dev->Hrdb.BitmapSize);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: HRDB : sftk_do_checksum_validation_of_PS_Bitmap(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																pSftk_Dev->SftkLg->PStoreFileName.Buffer, 
																pSftk_Dev->SftkLg->LGroupNumber, 
																pSftk_Dev->Vdevname,
																offset1.QuadPart, 
																pSftk_Dev->Hrdb.BitmapSize, status ));
			// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
			// TODO FIXME FIXME : We must ignore this error, and mark LG as full refresh requires or just use LRDB !!!
			DebugPrint((DBG_ERROR, "sftk_init_Lg_and_devs_from_Pstore:: Pstore LRDB Bitmap memory is corrupted checksum failed!!: TODO FIXME FIXME : We must ignore this error, and mark LG as full refresh requires or just use LRDB !!! \n"));

			// HRDB bitmap is invalid, so prepare from valid LRDB bitmap
			if (bLrdbIsValid == TRUE)	// this gets true at successful shutdown time
			{ // Call Routine to prepare Valid HRDB from LRDB, since we will be running Tracking mode.
				sftk_Prepare_bitmapA_to_bitmapB( &pSftk_Dev->Hrdb, &pSftk_Dev->Lrdb, TRUE);
			}
		}
		else
		{ // success, So use HRDB, if needed make LRDB from HRDB
			// HRDB bitmap is valid, use it
			/* TODO : enable following code if needed
			// Call Routine to prepare Valid LRDB from HRDB, since we will be running Tracking mode.
			// Do we need to ? this is for safety, to make consistence both bitmap
			sftk_Prepare_bitmapA_to_bitmapB( &pSftk_Dev->Lrdb, &pSftk_Dev->Hrdb, TRUE);
			*/
		}
#endif

#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
		// Do Validation of HRDB Bitmap Region.
		OS_ASSERT( pSftk_Dev->Hrdb.pPsBitmapHdr->MagicNum == SFTK_PS_BITMAP_REGION_MAGIC);

		if ( IS_PS_HRDB_BITMAP_VALID(	pSftk_Dev->Hrdb.pPsBitmapHdr, 
										pSftk_Dev->Lrdb.pPsBitmapHdr) )
		{ // HRDB bitmap is valid, use it
			/* TODO : enable following code if needed
			// Call Routine to prepare Valid LRDB from HRDB, since we will be running Tracking mode.
			// Do we need to ? this is for safety, to make consistence both bitmap
			sftk_Prepare_bitmapA_to_bitmapB( &pSftk_Dev->Lrdb, &pSftk_Dev->Hrdb, TRUE);
			*/
		}
		else
		{ // HRDB bitmap is invalid, so prepare from valid LRDB bitmap
			if (bLrdbIsValid == TRUE)	// this gets true at successful shutdown time
			{ // Call Routine to prepare Valid HRDB from LRDB, since we will be running Tracking mode.
				sftk_Prepare_bitmapA_to_bitmapB( &pSftk_Dev->Hrdb, &pSftk_Dev->Lrdb, TRUE);
			}
		}
#endif
		// update next offset to read
		offset.QuadPart = OFFSET_NEXT_SFTK_PS_DEV(	pSftk_Dev->OffsetOfPsDev, 
													pPsDev->TotalSizeOfDevRegion);
		pPsDev = NULL;
	} // for each and every pstore device create and configure device.

	// TODO : FIXME FIXME : What state we changed here??.... do according actions ....
	if (pPsHdr->ValidLastShutdown == TRUE)	// this gets true at successful shutdown time
	{
		if (bMarkLG_FullRefresh == FALSE)
			pSftk_Lg->state = SFTK_MODE_TRACKING;
		else
			pSftk_Lg->state = SFTK_MODE_PASSTHRU;
	}
	else
	{
		pSftk_Lg->state = SFTK_MODE_PASSTHRU;
	}

	// Make it valid shutdown to FALSE since we are just boot up
	pSftk_Lg->PsHeader->ValidLastShutdown = FALSE;	// Later when we change the state we will update Pstore file., TODO Verify this
	
	status = STATUS_SUCCESS;

done:
	if (status != STATUS_SUCCESS)
	{ // Failed
		if (pSftk_Lg)
		{
			sftk_delete_lg(pSftk_Lg, TRUE);
		}
		else
		{
			if (pPsHdr)
				OS_FreeMemory(pPsHdr);

			if (pPsDev)
				OS_FreeMemory(pPsDev);
		}
	} // Failed

done_igonre_error:

	return status;
} // sftk_init_Lg_and_devs_from_Pstore()

NTSTATUS
sftk_format_pstorefile( IN PSFTK_LG Sftk_Lg )
{
	NTSTATUS		status		= STATUS_SUCCESS;
	HANDLE			fileHandle	= NULL;
	LARGE_INTEGER	offset;
	PSFTK_PS_HDR	pPsHdr;
	LARGE_INTEGER	checksum;

	checksum.QuadPart = 0;

	OS_ASSERT(Sftk_Lg->PsHeader == NULL);
	Sftk_Lg->SizeBAlignPsHeader = 0;

	status = sftk_open_pstore( &Sftk_Lg->PStoreFileName, &fileHandle, TRUE);
	if (!NT_SUCCESS(status)) 
	{
		DebugPrint((DBG_ERROR, "sftk_format_pstorefile: sftk_open_pstore(LG 0x%08x PstoreFile Name %S) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, status));

		goto done;
	}

	// Format This Pstore File

	// - Allocate Sector Align strcuture SFTK_PS_HDR memory, initialized and write it to pstore file
	Sftk_Lg->SizeBAlignPsHeader = BLOCK_ALIGN_UP(sizeof(SFTK_PS_HDR));

	Sftk_Lg->PsHeader = OS_AllocMemory(NonPagedPool, Sftk_Lg->SizeBAlignPsHeader);
	if (Sftk_Lg->PsHeader == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_format_pstorefile: OS_AllocMemory(LG 0x%08x PstoreFile %S, sizeof Mem %d ) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, Sftk_Lg->SizeBAlignPsHeader, status));
		goto done;
	}

	OS_ZeroMemory(Sftk_Lg->PsHeader, Sftk_Lg->SizeBAlignPsHeader);
	// Initialized it
	pPsHdr = Sftk_Lg->PsHeader;

	pPsHdr->MagicNum	= SFTK_PS_VERSION_1_MAGIC;	// 0xBADF00D1
	pPsHdr->MajorVer	= SFTK_PS_MAJOR_VER;			// 0x1
	pPsHdr->MinorVer	= SFTK_PS_MINOR_VER;			// 0x0
	pPsHdr->ExtraVer	= SFTK_PS_EXTRA_VER;			// 0x0
	pPsHdr->SizeInbAllign = Sftk_Lg->SizeBAlignPsHeader;

	pPsHdr->ValidLastShutdown = FALSE;	// Make this to true at shutdown time
	// pPsHdr->Checksum // TODO : Calculate Checksum of current structure.

	pPsHdr->LgInfo.LGroupNumber		= Sftk_Lg->LGroupNumber;
	pPsHdr->LgInfo.state			= Sftk_Lg->state;
	pPsHdr->LgInfo.PrevState		= Sftk_Lg->state;
	pPsHdr->LgInfo.bInconsistantData = Sftk_Lg->bInconsistantData;
	pPsHdr->LgInfo.UserChangedToTrackingMode = Sftk_Lg->UserChangedToTrackingMode;

	pPsHdr->LgInfo.TotalNumDevices	= 0; // Sftk_Lg->LgDev_List.NumOfNodes;	

	pPsHdr->LgInfo.statsize			= Sftk_Lg->statsize;
	pPsHdr->LgInfo.sync_depth		= Sftk_Lg->sync_depth;
	pPsHdr->LgInfo.sync_timeout		= Sftk_Lg->sync_timeout;
	pPsHdr->LgInfo.iodelay			= Sftk_Lg->iodelay;

	pPsHdr->LastOffsetOfPstoreFile = OFFSET_FIRST_SFTK_PS_DEV(Sftk_Lg->SizeBAlignPsHeader);

	// Write PsHdr to Pstore file.
	status = sftk_flush_psHdr_to_pstore( Sftk_Lg, TRUE, fileHandle, TRUE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_format_pstorefile:: PSHDR sftk_flush_psHdr_to_pstore(%S, LG 0x%08x) Failed with status 0x%08x !\n", 
															Sftk_Lg->PStoreFileName.Buffer, Sftk_Lg->LGroupNumber, 
															status ));
		goto done;
	}

	status = STATUS_SUCCESS;
done:
	if (fileHandle)
		ZwClose(fileHandle);

	if (status != STATUS_SUCCESS)
	{
		if (Sftk_Lg->PsHeader)
			OS_FreeMemory(Sftk_Lg->PsHeader);
		Sftk_Lg->PsHeader = NULL;
	}

	return status;
} // sftk_format_pstorefile

NTSTATUS
sftk_create_dev_in_pstorefile( IN PSFTK_LG Sftk_Lg, IN	PSFTK_DEV Sftk_Dev )
{
	NTSTATUS		status		= STATUS_SUCCESS;
	HANDLE			fileHandle	= NULL;
	PSFTK_PS_HDR	pPsHdr;
	PSFTK_PS_DEV	pPsDev;
	ULONG			savedLastOffsetOfPstoreFile;

	OS_ASSERT(Sftk_Lg->PsHeader != NULL);
	OS_ASSERT(Sftk_Dev->SftkLg != NULL);
	OS_ASSERT(Sftk_Dev->PsDev == NULL);

	Sftk_Dev->SizeBAlignPsDev = 0;
	pPsHdr = Sftk_Lg->PsHeader;

	status = sftk_open_pstore( &Sftk_Lg->PStoreFileName, &fileHandle, FALSE);
	if (!NT_SUCCESS(status)) 
	{
		DebugPrint((DBG_ERROR, "sftk_create_dev_in_pstorefile: sftk_open_pstore(LG 0x%08x PstoreFile Name %S) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, status));

		goto done;
	}

	// Format Device area in Pstore File

	// - Allocate Sector Align strcuture SFTK_PS_DEV memory, initialized and write it to pstore file
	Sftk_Dev->SizeBAlignPsDev = BLOCK_ALIGN_UP(sizeof(SFTK_PS_DEV));

	Sftk_Dev->PsDev = OS_AllocMemory(NonPagedPool, Sftk_Dev->SizeBAlignPsDev);
	if (Sftk_Dev->PsDev == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_create_dev_in_pstorefile: OS_AllocMemory(LG 0x%08x PstoreFile %S, sizeof Mem %d ) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, Sftk_Dev->SizeBAlignPsDev, status));
		goto done;
	}

	OS_ZeroMemory(Sftk_Dev->PsDev, Sftk_Dev->SizeBAlignPsDev);
	// Initialized it
	pPsDev = Sftk_Dev->PsDev;
	pPsDev->SizeInbAllign = Sftk_Dev->SizeBAlignPsDev;

	pPsDev->Deleted		= FALSE;
	pPsDev->cdev		= Sftk_Dev->cdev;
	pPsDev->bdev		= Sftk_Dev->bdev;
	pPsDev->localbdisk	= Sftk_Dev->localbdisk;
	pPsDev->localcdisk	= Sftk_Dev->localcdisk;

	RtlCopyMemory( pPsDev->Devname, Sftk_Dev->Devname, sizeof(pPsDev->Devname) );
	RtlCopyMemory( pPsDev->Vdevname, Sftk_Dev->Vdevname, sizeof(pPsDev->Vdevname) );

	pPsDev->bUniqueVolumeIdValid = Sftk_Dev->bUniqueVolumeIdValid;
	pPsDev->UniqueIdLength		 = Sftk_Dev->UniqueIdLength;
	RtlCopyMemory( pPsDev->UniqueId, Sftk_Dev->UniqueId, sizeof(pPsDev->UniqueId) );
	
	pPsDev->bSignatureUniqueVolumeIdValid = Sftk_Dev->bSignatureUniqueVolumeIdValid;
	pPsDev->SignatureUniqueIdLength		 = Sftk_Dev->SignatureUniqueIdLength;
	RtlCopyMemory( pPsDev->SignatureUniqueId, Sftk_Dev->SignatureUniqueId, sizeof(pPsDev->SignatureUniqueId) );
	
	pPsDev->bSuggestedDriveLetterLinkValid = Sftk_Dev->bSuggestedDriveLetterLinkValid;
	pPsDev->SuggestedDriveLetterLinkLength = Sftk_Dev->SuggestedDriveLetterLinkLength;
	RtlCopyMemory( pPsDev->SuggestedDriveLetterLink, Sftk_Dev->SuggestedDriveLetterLink, sizeof(pPsDev->SuggestedDriveLetterLink) );

	pPsDev->Flags		= Sftk_Dev->Flags;
	pPsDev->Disksize	= Sftk_Dev->Disksize;
	pPsDev->statesize	= Sftk_Dev->statsize;	

	pPsDev->Lrdb.ValidBitmap			= TRUE;
	pPsDev->Lrdb.Sectors_per_bit		= Sftk_Dev->Lrdb.Sectors_per_bit; 
	pPsDev->Lrdb.TotalNumOfBits			= Sftk_Dev->Lrdb.TotalNumOfBits;
	pPsDev->Lrdb.BitmapSize				= Sftk_Dev->Lrdb.BitmapSize;
	pPsDev->Lrdb.BitmapSizeBlockAlign	= Sftk_Dev->Lrdb.BitmapSizeBlockAlign;
	pPsDev->Lrdb.len32					= Sftk_Dev->Lrdb.len32;
	pPsDev->RefreshLastBitIndex			= 0;		// Sftk_Dev->RefreshLastBitIndex; // will be zero at start

	pPsDev->Hrdb.ValidBitmap			= TRUE;
	pPsDev->Hrdb.Sectors_per_bit		= Sftk_Dev->Hrdb.Sectors_per_bit; 
	pPsDev->Hrdb.TotalNumOfBits			= Sftk_Dev->Hrdb.TotalNumOfBits;
	pPsDev->Hrdb.BitmapSize				= Sftk_Dev->Hrdb.BitmapSize;
	pPsDev->Hrdb.BitmapSizeBlockAlign	= Sftk_Dev->Hrdb.BitmapSizeBlockAlign;
	pPsDev->Hrdb.len32					= Sftk_Dev->Hrdb.len32;

	pPsDev->LrdbOffset = OFFSET_SFTK_LRDB(pPsHdr->LastOffsetOfPstoreFile, Sftk_Dev->SizeBAlignPsDev);
	pPsDev->HrdbOffset = OFFSET_SFTK_HRDB(pPsDev->LrdbOffset, pPsDev->Lrdb.BitmapSizeBlockAlign);

	pPsDev->TotalSizeOfDevRegion = TOTAL_PS_DEV_REGION_SIZE(pPsDev->SizeInbAllign, 
															pPsDev->Lrdb.BitmapSizeBlockAlign,
															pPsDev->Hrdb.BitmapSizeBlockAlign);

	Sftk_Dev->OffsetOfPsDev = pPsHdr->LastOffsetOfPstoreFile; // store offset of psdev in SFTK_DEV 

	// Flush Psdev + LRDB + HRDB Bitmaps Memory, all Device Region on to Pstore File
	status = sftk_flush_all_bitmaps_to_pstore( Sftk_Dev, TRUE, fileHandle, TRUE, TRUE);
		
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_create_dev_in_pstorefile:: PsDev : sftk_flush_all_bitmaps_to_pstore(%S, LG 0x%08x for Device %s) Failed with status 0x%08x !\n", 
																Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
																Sftk_Dev->SftkLg->LGroupNumber, 
																Sftk_Dev->Vdevname, 
																status ));
		goto done;
	}

	// update PsHdr and its LG
	// increment next available offset in Pstore file
	savedLastOffsetOfPstoreFile = pPsHdr->LastOffsetOfPstoreFile;
	pPsHdr->LastOffsetOfPstoreFile = OFFSET_NEXT_SFTK_PS_DEV(	pPsHdr->LastOffsetOfPstoreFile, 
																pPsDev->TotalSizeOfDevRegion);
	pPsHdr->LgInfo.TotalNumDevices ++;	// increment it

	// Write PsHdr to Pstore file.
	status = sftk_flush_psHdr_to_pstore( Sftk_Lg, TRUE, fileHandle, TRUE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_create_dev_in_pstorefile:: PSHDR sftk_flush_psHdr_to_pstore(%S, LG 0x%08x) Failed with status 0x%08x !\n", 
															Sftk_Lg->PStoreFileName.Buffer, Sftk_Lg->LGroupNumber, 
															status ));
		pPsHdr->LastOffsetOfPstoreFile = savedLastOffsetOfPstoreFile;
		pPsHdr->LgInfo.TotalNumDevices --;
		goto done;
	}
	
	status = STATUS_SUCCESS;
done:
	if (fileHandle)
		ZwClose(fileHandle);

	if (status != STATUS_SUCCESS)
	{
		if (Sftk_Dev->PsDev)
			OS_FreeMemory(Sftk_Dev->PsDev);
		Sftk_Dev->PsDev = NULL;
	}

	return status;
} // sftk_create_dev_in_pstorefile()

NTSTATUS
sftk_Delete_dev_in_pstorefile( IN	PSFTK_DEV Sftk_Dev )
{
	NTSTATUS	status;

	OS_ASSERT(Sftk_Dev->SftkLg != NULL);

	if (Sftk_Dev->PsDev == NULL)
		return STATUS_SUCCESS;	// nothing to do ....

	Sftk_Dev->PsDev->Deleted = TRUE;

	status = sftk_flush_psDev_to_pstore( Sftk_Dev, FALSE, NULL, TRUE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		WCHAR wchar3[64];

		DebugPrint((DBG_ERROR, "sftk_Delete_dev_in_pstorefile:: sftk_flush_psDev_to_pstore(%S, LG 0x%08x for Device %s) Failed with status 0x%08x !\n", 
															Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
															Sftk_Dev->SftkLg->LGroupNumber, 
															Sftk_Dev->Vdevname, 
															status ));
		swprintf( wchar3, L"Deleting Dev");
		sftk_LogEventNum2Wchar2(GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
						0, Sftk_Dev->cdev, Sftk_Dev->SftkLg->LGroupNumber, Sftk_Dev->SftkLg->PStoreFileName.Buffer, wchar3);

	}
	return status;
} // sftk_Delete_dev_in_pstorefile()

//
// It flush all LG and their devices to Pstore file with marking Successful Shutdown ON, if 
// ShutDownOn == TRUE
//
NTSTATUS
sftk_flush_all_pstore( IN BOOLEAN ShutDownOn, BOOLEAN bGrabLGLock, BOOLEAN bGrabGlobalLock )	// TRUE means Shutdown time
{
	NTSTATUS	status		= STATUS_SUCCESS;
	PSFTK_LG	pSftk_Lg	= NULL;	
	PSFTK_DEV	pSftk_Dev	= NULL;	
	HANDLE		fileHandle	= NULL;
	BOOLEAN		bHandleValid= FALSE;
	PLIST_ENTRY	plistEntry, plistEntry1;
	WCHAR		wstr[64];
	
	// go Thru each and every LG, and flush LG, all its devices to Pstore file:
	if (bGrabGlobalLock == TRUE)
		OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	for( plistEntry = GSftk_Config.Lg_GroupList.ListEntry.Flink;
		 plistEntry != &GSftk_Config.Lg_GroupList.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every logical group list 
		bHandleValid= FALSE;

		pSftk_Lg = CONTAINING_RECORD( plistEntry, SFTK_LG, Lg_GroupLink);

		OS_ASSERT(pSftk_Lg->PsHeader != NULL);

		pSftk_Lg->PsHeader->LgInfo.state = pSftk_Lg->state;
		OS_ASSERT(pSftk_Lg->PsHeader->LgInfo.LGroupNumber == pSftk_Lg->LGroupNumber);
		OS_ASSERT(pSftk_Lg->PsHeader->LgInfo.TotalNumDevices == pSftk_Lg->LgDev_List.NumOfNodes);

		if (ShutDownOn == TRUE)
		{
			// first uninitialize and stop session socket manager for current LG
			COM_UninitializeSessionManager( &pSftk_Lg->SessionMgr );

			if ( OS_IsFlagSet( pSftk_Lg->flags, SFTK_LG_FLAG_STARTED_LG_THREADS) ) 
			{ // Terminate Thread for LG
				sftk_Terminate_LGThread( pSftk_Lg );
			}
			
			pSftk_Lg->PsHeader->ValidLastShutdown = TRUE;
		}

		// Open pstore file so we can flush new state and other required info
		status = sftk_open_pstore( &pSftk_Lg->PStoreFileName , &fileHandle, FALSE);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			DebugPrint((DBG_ERROR, "sftk_flush_all_pstore:: sftk_open_pstore(%S, LG Num 0x%08x ) Failed with status 0x%08x !\n", 
									pSftk_Lg->PStoreFileName.Buffer, pSftk_Lg->LGroupNumber, status ));
			DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just continue the operations...\n"));

			if (ShutDownOn == TRUE)
				swprintf( wstr, L"Shutdown time Flush LG %d Pstore", pSftk_Lg->LGroupNumber);
			else
				swprintf( wstr, L"Flush LG %d Pstore", pSftk_Lg->LGroupNumber);

			sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_OPEN_ERROR, status, 
								  0, pSftk_Lg->PStoreFileName.Buffer, wstr);

			// ignore error
			bHandleValid= FALSE;
			fileHandle	= NULL;
		}
		else
		{ // success
			bHandleValid = TRUE;
		}

		// go thru each and every Devices under LG and flush it to Pstore file!!!! ???
		if (bGrabLGLock == TRUE)
			OS_ACQUIRE_LOCK( &pSftk_Lg->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

		for(	plistEntry1 = pSftk_Lg->LgDev_List.ListEntry.Flink;
				plistEntry1 != &pSftk_Lg->LgDev_List.ListEntry;
				plistEntry1 = plistEntry1->Flink )
		{ // for :scan thru each and every Devices under logical group 
			pSftk_Dev = CONTAINING_RECORD( plistEntry1, SFTK_DEV, LgDev_Link);

			OS_ASSERT(pSftk_Dev->SftkLg == pSftk_Lg);
			OS_ASSERT(pSftk_Dev->LGroupNumber == pSftk_Lg->LGroupNumber);
			OS_ASSERT(pSftk_Dev->PsDev != NULL);

			pSftk_Dev->PsDev->RefreshLastBitIndex = pSftk_Dev->RefreshLastBitIndex; // just to make sure its updated...

			// Write last bit info changed to SFTK_PS_DEV and flush it to Pstore file
			// Also flush all new bitmap to pstore file
			status = sftk_flush_all_bitmaps_to_pstore(pSftk_Dev, bHandleValid, fileHandle, TRUE, TRUE);
			if ( !NT_SUCCESS(status) ) 
			{ // failed to open or create pstore file...
				
				DebugPrint((DBG_ERROR, "sftk_flush_all_pstore:: sftk_flush_all_bitmaps_to_pstore(%S, LG 0x%08x Sftk_Dev 0x%08x for Device %s ) Failed with status 0x%08x Ignoring Error !!!\n", 
							pSftk_Lg->PStoreFileName.Buffer, pSftk_Lg->LGroupNumber, 
							pSftk_Dev, pSftk_Dev->Vdevname, status ));

				if (ShutDownOn == TRUE)
					swprintf( wstr, L"Flushing All Bitmaps At Shutdown time");
				else
					swprintf( wstr, L"Flushing All Bitmaps");

				sftk_LogEventNum2Wchar2(GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
									0, pSftk_Dev->cdev, pSftk_Dev->SftkLg->LGroupNumber, 
									pSftk_Dev->SftkLg->PStoreFileName.Buffer, wstr);
			}
		} // for :scan thru each and every Devices under logical group 

		if (bGrabLGLock == TRUE)
			OS_RELEASE_LOCK( &pSftk_Lg->Lock, NULL);

		// Write state changed to SFTK_PS_LG and flush it to Pstore file
		status = sftk_flush_psHdr_to_pstore( pSftk_Lg, bHandleValid, fileHandle, TRUE);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			DebugPrint((DBG_ERROR, "sftk_flush_all_pstore:: sftk_flush_psHdr_to_pstore(%S, LG 0x%08x)  Failed with status 0x%08x Ignoring Error !!!\n", 
										pSftk_Lg->PStoreFileName.Buffer, pSftk_Lg->LGroupNumber, status ));
			if (ShutDownOn == TRUE)
				swprintf( wstr, L"Flushing LG Hdr At Shutdown time");
			else
				swprintf( wstr, L"Flushing LG Hdr");

			sftk_LogEventNum2Wchar2(GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
									0, pSftk_Lg->LGroupNumber, pSftk_Lg->LGroupNumber, 
									pSftk_Lg->PStoreFileName.Buffer, wstr);
		}

		if ((bHandleValid == TRUE) && (fileHandle))
			ZwClose(fileHandle);

	} // for : scan thru each and every logical group list 

	if (bGrabGlobalLock == TRUE)
		OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
		
	return status;
} // sftk_flush_all_pstore()

NTSTATUS
sftk_Update_LG_from_pstore( PSFTK_LG Sftk_Lg)
{
	NTSTATUS		status		= STATUS_SUCCESS;
	HANDLE			fileHandle	= NULL;	
	PSFTK_DEV		pSftkDev;
	LARGE_INTEGER	offset, checksum;
	WCHAR			wstr[64];
	
	OS_ASSERT( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == TRUE);
	OS_ASSERT( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_PSTORE_FILE_CREATED) == FALSE);

	if (Sftk_Lg->PStoreFileName.Buffer == NULL)
	{ // since pstore file is not configured by LG, we can't do anything, return error
		status = STATUS_NO_SUCH_FILE;
		DebugPrint((DBG_ERROR, "sftk_Update_LG_from_pstore:: LGNum %d, pstore file value is NULL, we can't do anything, return error 0x%08x !\n", 
											Sftk_Lg->LGroupNumber, status ));
		return status;
	}

	OS_ASSERT(Sftk_Lg->PsHeader == NULL);

	status = sftk_open_pstore( &Sftk_Lg->PStoreFileName , &fileHandle, FALSE);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to open pstore file...
		DebugPrint((DBG_ERROR, "sftk_Update_LG_from_pstore:: Open : sftk_open_pstore(LG %d File : %S) Failed with status 0x%08x !\n",
								Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, status ));
		DebugPrint((DBG_ERROR, "sftk_Update_LG_from_pstore:: We will try to format this pstore file...\n"));

		swprintf( wstr, L"Recover LG %d state From Pstore", Sftk_Lg->LGroupNumber);
		sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_OPEN_ERROR, status, 
								  0, Sftk_Lg->PStoreFileName.Buffer, wstr);

		// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
		status = sftk_format_pstorefile(Sftk_Lg);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			DebugPrint((DBG_ERROR, "sftk_Update_LG_from_pstore:: Create: sftk_format_pstorefile(LG %d File : %S) Failed with status 0x%08x, returning error !\n",
									Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, status ));
			
			// pstore file can't even create, so just dail this LG and delete it later
			swprintf( wstr, L"Format New Pstore File for LG %d", Sftk_Lg->LGroupNumber);
			sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_OPEN_ERROR, status, 
								  0, Sftk_Lg->PStoreFileName.Buffer, wstr);

			return status;
		}
		else
		{ // We are done Formating new Pstore file, So just marked the flag in LG so later we know to add new Dev into it
			sftk_LogEventNum1Wchar1(	GSftk_Config.DriverObject, MSG_REPL_PSTORE_FILE_FORMATTED, status, 
										0, Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer);

			// since we formated new pstore, its requires full refresh, old delta lost
			sftk_lg_change_State(Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
			if (Sftk_Lg->state != SFTK_MODE_PASSTHRU)
				Sftk_Lg->state = SFTK_MODE_PASSTHRU; // Forced to do so...
			
			
			OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_NEW_PSTORE_FILE_FORMATTED);
			OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_PSTORE_FILE_CREATED);
			return STATUS_SUCCESS;
		}
	} // failed to open pstore file...

	// We are here means we opened already existed pstore file
	// Allocate required LG structures for pstore file 
	OS_ASSERT(Sftk_Lg->PsHeader == NULL);
	Sftk_Lg->SizeBAlignPsHeader = 0;

	// - Allocate Sector Align strcuture SFTK_PS_HDR memory, initialized and write it to pstore file
	Sftk_Lg->SizeBAlignPsHeader = BLOCK_ALIGN_UP(sizeof(SFTK_PS_HDR));

	Sftk_Lg->PsHeader = OS_AllocMemory(NonPagedPool, Sftk_Lg->SizeBAlignPsHeader);
	if (Sftk_Lg->PsHeader == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_Update_LG_from_pstore: OS_AllocMemory(LG 0x%08x PstoreFile %S, sizeof Mem %d ) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, Sftk_Lg->SizeBAlignPsHeader, status));
		goto done;
	}
	OS_ZeroMemory(Sftk_Lg->PsHeader, Sftk_Lg->SizeBAlignPsHeader);

	// Read PsHdr from Pstore file.
	offset.QuadPart = OFFSET_SFTK_PS_HDR;
	status = sftk_read_pstore( fileHandle, &offset, Sftk_Lg->PsHeader, Sftk_Lg->SizeBAlignPsHeader);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		WCHAR wstr1[64];

		DebugPrint((DBG_ERROR, "sftk_Update_LG_from_pstore:: PSHDR sftk_read_pstore( LG %d, %S, O %I64d S : %d ) Failed with status 0x%08x !\n", 
									Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer,  offset.QuadPart, Sftk_Lg->SizeBAlignPsHeader, status ));
		// Log Event, and error handling, we should terminate here ??? or just continue ... ???
		swprintf( wstr, L"%I64d", offset.QuadPart);
		swprintf( wstr1, L"%d", Sftk_Lg->SizeBAlignPsHeader);
		sftk_LogEventString3( GSftk_Config.DriverObject, MSG_REPL_PSTORE_READ_ERROR, status, 
								  0, Sftk_Lg->PStoreFileName.Buffer, wstr, wstr1);

		goto done;
	}

	status = sftk_do_validation_PS_HDR(	Sftk_Lg->PsHeader, Sftk_Lg->SizeBAlignPsHeader );
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_Update_LG_from_pstore:: PSHDR sftk_do_validation_PS_HDR( LG %d, %S, O %I64d S : %d ) Failed with status 0x%08x !\n", 
								Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer,  offset.QuadPart, Sftk_Lg->SizeBAlignPsHeader, status ));

		swprintf( wstr, L"LG %d Header", Sftk_Lg->LGroupNumber);
		sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_VALIDATION_FAILED, status, 
								  0, Sftk_Lg->PStoreFileName.Buffer, wstr);

		goto done;
	}

	OS_SetFlag( Sftk_Lg->flags, SFTK_LG_FLAG_PSTORE_FILE_CREATED);

	if (Sftk_Lg->PsHeader->LgInfo.state == SFTK_MODE_FULL_REFRESH)
	{ // After reboot, resume previous running full refresh ...
		OS_SetFlag( Sftk_Lg->flags, SFTK_LG_FLAG_AFTER_BOOT_RESUME_FULL_REFRESH);
	}
	 // Restore value from Pstore file. 
	Sftk_Lg->PrevState					= Sftk_Lg->PsHeader->LgInfo.PrevState; 
	Sftk_Lg->UserChangedToTrackingMode	= Sftk_Lg->PsHeader->LgInfo.UserChangedToTrackingMode; 
	Sftk_Lg->bInconsistantData			= Sftk_Lg->PsHeader->LgInfo.bInconsistantData; 

	status = STATUS_SUCCESS;

done:
	if (fileHandle) 
		ZwClose(fileHandle);

	if ( !NT_SUCCESS(status) ) 
	{ // failed, Do Cleanup here....
		if(Sftk_Lg->PsHeader)
			OS_FreeMemory(Sftk_Lg->PsHeader);
		Sftk_Lg->PsHeader			= NULL;
		Sftk_Lg->SizeBAlignPsHeader = 0;
		OS_ClearFlag( Sftk_Lg->flags, SFTK_LG_FLAG_PSTORE_FILE_CREATED);
	}

	return status;
} // sftk_Update_LG_from_pstore()

NTSTATUS
sftk_Update_Dev_from_pstore( PSFTK_LG Sftk_Lg, PSFTK_DEV Sftk_Dev)
{
	NTSTATUS		status		= STATUS_SUCCESS;
	HANDLE			fileHandle	= NULL;	
	LARGE_INTEGER	offset, offset1, checksum;
	PSFTK_PS_HDR	pPsHdr;
	PSFTK_PS_DEV	pPsDev;
	BOOLEAN			bFoundDev, bLrdbIsValid, bHrdbValid;
	ULONG			lrdbSize, hrdbSize, size, i;
	PUCHAR			pLrdbBits,pHrdbBits;
	PSFTK_PS_BITMAP_REGION_HDR	pLrdbPsBitmapHdr, pHrdbPsBitmapHdr;
	SFTK_BITMAP		bitmap;
	WCHAR			wchar[64], wchar1[30];


	pLrdbBits = pHrdbBits = NULL;
	pLrdbPsBitmapHdr = pHrdbPsBitmapHdr = NULL;
	pPsDev	= NULL;
	OS_ZeroMemory(&bitmap, sizeof(bitmap));
	bitmap.pBitmapHdr = NULL;
	
	// OS_ASSERT( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_PSTORE_FILE_CREATED) == TRUE);
	OS_ASSERT( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == TRUE);
	OS_ASSERT( OS_IsFlagSet( Sftk_Dev->Flags, SFTK_DEV_FLAG_PSTORE_FILE_ADDED) == FALSE);

	OS_ASSERT( Sftk_Dev->SftkLg == Sftk_Lg);

	if (OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_PSTORE_FILE_CREATED) == FALSE)
	{ // since pstore file is not configured by LG, we can't do anything, return error
		status = STATUS_NO_SUCH_FILE;
		DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: LGNum %d, VDevName %s,  pstore file is not configured by LG, we can't do anything, return error 0x%08x !\n", 
											Sftk_Dev->LGroupNumber, Sftk_Dev->Vdevname, status ));
		return status;
	}

	if (OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_NEW_PSTORE_FILE_FORMATTED) == TRUE)
	{ // Add new SftkDev into Pstore file, since its new pstore file....
		status = sftk_create_dev_in_pstorefile(Sftk_Lg, Sftk_Dev);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: Format: sftk_create_dev_in_pstorefile(LG %d File : %S, Dev %s) Failed with status 0x%08x, returning error !\n",
									Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, Sftk_Dev->Vdevname, status ));
			
			// Dev can't add to pstore file , so just delete this dev under LG later in config_end
			swprintf( wchar, L"Creating New Device %d", Sftk_Dev->cdev);
			sftk_LogEventNum2Wchar2(GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
						0, Sftk_Dev->cdev, Sftk_Dev->SftkLg->LGroupNumber, 
						Sftk_Dev->SftkLg->PStoreFileName.Buffer, wchar);

			return status;
		}
		else
		{ // success
			OS_SetFlag( Sftk_Dev->Flags, SFTK_DEV_FLAG_PSTORE_FILE_ADDED);

			if (Sftk_Lg->state != SFTK_MODE_PASSTHRU)
			{
				sftk_lg_change_State(Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
				if (Sftk_Lg->state != SFTK_MODE_PASSTHRU)
					Sftk_Lg->state = SFTK_MODE_PASSTHRU; // Forced to do so...
			}
			return STATUS_SUCCESS;
		}
	} // Add new SftkDev into Pstore file, since its new pstore file....

	// We are here means we have opened already existed pstore file
	// Allocate required Dev structures for pstore file 

	OS_ASSERT(Sftk_Dev->SftkLg != NULL);
	OS_ASSERT(Sftk_Lg->PsHeader != NULL);
	OS_ASSERT(Sftk_Dev->PsDev == NULL);

	pPsHdr = Sftk_Lg->PsHeader;
	status = sftk_open_pstore( &Sftk_Lg->PStoreFileName, &fileHandle, FALSE);
	if (!NT_SUCCESS(status)) 
	{
		DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore: sftk_open_pstore(LG 0x%08x PstoreFile Name %S) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, status));

		goto done;
	}
	// Format Device area in Pstore File
	// - Allocate Sector Align strcuture SFTK_PS_DEV memory, initialized and write it to pstore file
	Sftk_Dev->SizeBAlignPsDev = BLOCK_ALIGN_UP(sizeof(SFTK_PS_DEV));

	Sftk_Dev->PsDev = OS_AllocMemory(NonPagedPool, Sftk_Dev->SizeBAlignPsDev);
	if (Sftk_Dev->PsDev == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore: OS_AllocMemory(LG 0x%08x PstoreFile %S, sizeof Mem %d ) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, Sftk_Dev->SizeBAlignPsDev, status));
		goto done;
	}
	OS_ZeroMemory(Sftk_Dev->PsDev, Sftk_Dev->SizeBAlignPsDev);

	// Allocate Extra LRDB Bitmap memory
	lrdbSize = Sftk_Dev->Lrdb.BitmapSizeBlockAlign;
	pLrdbBits = OS_AllocMemory(NonPagedPool, lrdbSize);
	if (pLrdbBits == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore: LRDB : OS_AllocMemory(LG 0x%08x PstoreFile %S, sizeof Mem %d ) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, lrdbSize, status));
		goto done;
	}
	OS_ZeroMemory(pLrdbBits, lrdbSize);

	
	// Allocate Extra HRDB Bitmap memory
	hrdbSize = Sftk_Dev->Hrdb.BitmapSizeBlockAlign;
	pHrdbBits = OS_AllocMemory(NonPagedPool, hrdbSize);
	if (pHrdbBits == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore: HRDB : OS_AllocMemory(LG 0x%08x PstoreFile %S, sizeof Mem %d ) Failed with status 0x%08x \n", 
							Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, hrdbSize, status));
		goto done;
	}
	OS_ZeroMemory(pHrdbBits, hrdbSize);

#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
	pLrdbPsBitmapHdr	= (PSFTK_PS_BITMAP_REGION_HDR) pLrdbBits;
	pLrdbBits			= (PUCHAR) ((ULONG) pLrdbPsBitmapHdr + sizeof(SFTK_PS_BITMAP_REGION_HDR));

	pHrdbPsBitmapHdr	= (PSFTK_PS_BITMAP_REGION_HDR) pHrdbBits;
	pHrdbBits			= (PUCHAR) ((ULONG) pHrdbPsBitmapHdr + sizeof(SFTK_PS_BITMAP_REGION_HDR));
#else
	pLrdbPsBitmapHdr = pLrdbBits;
	pHrdbPsBitmapHdr = pHrdbBits;
#endif

	// bitmap.pBitmapHdr			= pSftk_Dev->Lrdb.pBitmapHdr;
	// Allocate RTL Bitmap Header Buffer
	bitmap.pBitmapHdr = OS_AllocMemory( NonPagedPool, sizeof(RTL_BITMAP));
	if (!bitmap.pBitmapHdr)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: OS_AllocMemory( sizeof(RTL_BITMAP) %d) Failed, returning status 0x%08x !\n",
											sizeof(RTL_BITMAP), status));
		goto done;
	}
	// now read pstore file, and find current sftk_Dev in Pstore file
	// in not found than add this sftk_dev into pstore file and change LG state to pass thru since full refresh is needed for 
	// newly added src device

	// -- Prepare all devices in loop
	bFoundDev = FALSE;
	bLrdbIsValid = TRUE;
	bHrdbValid	 = TRUE;

	offset.QuadPart = OFFSET_FIRST_SFTK_PS_DEV(Sftk_Lg->SizeBAlignPsHeader);

	for (i=0; i < pPsHdr->LgInfo.TotalNumDevices; i++)
	{ // for each and every pstore device create and configure device.
		if (offset.QuadPart > pPsHdr->LastOffsetOfPstoreFile)
		{
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore: FIXME FIXME Lgdev number %d, PstoreFile %S (offset.QuadPart %I64d > pPsHdr->LastOffsetOfPstoreFile %I64d )  Failed Corrupted Pstore File!!! Continuing LG prepare operations !!\n",
											Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, 
											offset.QuadPart, pPsHdr->LastOffsetOfPstoreFile));
			break;
		}

		size = BLOCK_ALIGN_UP(sizeof(SFTK_PS_DEV));
		if (pPsDev == NULL)
		{ // Allocate PS_DEV info one time only
			pPsDev = OS_AllocMemory(NonPagedPool, size);
			if (pPsDev == NULL)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore: OS_AllocMemory(PS_DEV PstoreFile %S, sizeof Mem %d ) Failed with status 0x%08x \n", 
													Sftk_Lg->PStoreFileName.Buffer, size, status));
				goto done;
			}
		}
		OS_ZeroMemory(pPsDev, size);

		// Read PsDev from Pstore file.
		status = sftk_read_pstore( fileHandle, &offset, pPsDev, size );
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file

			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: PSDEV sftk_read_pstore(%S, O %I64d S : %d ) Failed with status 0x%08x !\n", 
											Sftk_Lg->PStoreFileName.Buffer,  offset.QuadPart, size, status ));
			// Log Event, and error handling, we should terminate here ??? or just continue ... ???
			swprintf( wchar, L"%I64d", offset.QuadPart);
			swprintf( wchar1, L"%d", size);
			sftk_LogEventString3( GSftk_Config.DriverObject, MSG_REPL_PSTORE_READ_ERROR, status, 
									  0, Sftk_Lg->PStoreFileName.Buffer, wchar, wchar1);
			goto done;
		}

		// Do validation and checksum on read Psdev info
		status = sftk_do_validation_PS_DEV(	pPsDev, size );
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: PSDEV sftk_do_validation_PS_DEV(%S, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																Sftk_Lg->PStoreFileName.Buffer,  offset.QuadPart, size, status ));
			// Log Event, and error handling, we should terminate here ??? or just continue ... ???
			swprintf( wchar, L"Dev %d Header", pPsDev->cdev);
			sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_VALIDATION_FAILED, status, 
								  0, Sftk_Lg->PStoreFileName.Buffer, wchar);
		}
		if (pPsDev->Deleted == TRUE)
		{
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: PSDEV (pPsDev->Deleted == TRUE) (psDev->VDevname %s, psDev->Devnum: %d ) skipping this Dev continuing to next one !\n", 
																pPsDev->Vdevname, pPsDev->cdev ));
			// update next offset to read
			offset.QuadPart = OFFSET_NEXT_SFTK_PS_DEV(	offset.QuadPart, 
														pPsDev->TotalSizeOfDevRegion);
			continue;
		}
		
		OS_ASSERT( pPsDev->TotalSizeOfDevRegion = TOTAL_PS_DEV_REGION_SIZE( pPsDev->SizeInbAllign, 
																			pPsDev->Lrdb.BitmapSizeBlockAlign,
																			pPsDev->Hrdb.BitmapSizeBlockAlign));
		OS_ASSERT( pPsDev->LrdbOffset == OFFSET_SFTK_LRDB( offset.QuadPart, size) );
		OS_ASSERT( pPsDev->HrdbOffset == OFFSET_SFTK_LRDB( pPsDev->LrdbOffset, pPsDev->Lrdb.BitmapSizeBlockAlign) );

		// -- Compare SFTK_DEV from pstore Dev info
		if (pPsDev->cdev == Sftk_Dev->cdev) 
		{ // compare ..
			bFoundDev = TRUE;
			if (pPsDev->bUniqueVolumeIdValid == TRUE) 
			{
				if(!((Sftk_Dev->bUniqueVolumeIdValid == TRUE) &&
					(Sftk_Dev->UniqueIdLength == pPsDev->UniqueIdLength) &&
					(RtlCompareMemory( pPsDev->UniqueId, Sftk_Dev->UniqueId, Sftk_Dev->UniqueIdLength) 
							== Sftk_Dev->UniqueIdLength) ))
				{ // not matched !!
					bFoundDev = FALSE;
				}
			}
			if (pPsDev->bSignatureUniqueVolumeIdValid == TRUE) 
			{
				if(!((Sftk_Dev->bSignatureUniqueVolumeIdValid == TRUE) &&
					(Sftk_Dev->SignatureUniqueIdLength == pPsDev->SignatureUniqueIdLength) &&
					(RtlCompareMemory( pPsDev->SignatureUniqueId, Sftk_Dev->SignatureUniqueId, Sftk_Dev->SignatureUniqueIdLength) 
							== Sftk_Dev->SignatureUniqueIdLength) ))
				{ // not matched !!
					bFoundDev = FALSE;
				}
			}

			if (bFoundDev == FALSE)
			{ // Not Matched....
				// since Dev Number has matched....delete this entry from pstore file
				DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: PSDEV Matched Cdev %d but Devinfo did not matched, deleting pstore existing entry and continuing next search in pstore file !\n", 
																pPsDev->cdev));
				status = sftk_delete_psdev_entry_in_pstore( pPsDev, fileHandle, &offset, size, TRUE);
				if ( !NT_SUCCESS(status) ) 
				{ // failed to write to pstore file
					DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: PSDEV to Delete cdev %d, our Dev %d sftk_delete_psdev_entry_in_pstore(%S, O %I64d S : %d ) Failed with status 0x%08x !\n", 
													pPsDev->cdev, Sftk_Dev->cdev, Sftk_Lg->PStoreFileName.Buffer,  offset.QuadPart, size, status ));
					// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
				}
				// update next offset to read
				offset.QuadPart = OFFSET_NEXT_SFTK_PS_DEV(	offset.QuadPart, 
														pPsDev->TotalSizeOfDevRegion);
				continue;
			}
		}
		
		if (bFoundDev == FALSE)
		{ // Not Matched.
			// update next offset to read
			offset.QuadPart = OFFSET_NEXT_SFTK_PS_DEV(	offset.QuadPart, 
														pPsDev->TotalSizeOfDevRegion);
			continue;
		}

		// we found our Device, use it from psotre, Merge Bitmap 
		OS_ASSERT( Sftk_Dev->SizeBAlignPsDev == size);
		RtlCopyMemory(Sftk_Dev->PsDev, pPsDev, size);
		Sftk_Dev->OffsetOfPsDev		= (ULONG) offset.QuadPart;
		
		Sftk_Dev->RefreshLastBitIndex = Sftk_Dev->PsDev->RefreshLastBitIndex; // TODO FIXME FIXME

		// ---- Read LRDB Bitmap in memory 
		offset1.QuadPart = pPsDev->LrdbOffset;
		
		#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
		OS_ASSERT( Sftk_Dev->Lrdb.pPsBitmapHdr != NULL);
		status = sftk_read_pstore(  fileHandle, &offset1, pLrdbPsBitmapHdr,
									Sftk_Dev->Lrdb.BitmapSizeBlockAlign);
		#else
		status = sftk_read_pstore( FileHandle, &offset1, pLrdbBits,
									Sftk_Dev->Lrdb.BitmapSizeBlockAlign);
		#endif
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: LRDB : sftk_read_pstore(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
																Sftk_Dev->SftkLg->LGroupNumber, 
																Sftk_Dev->Vdevname,
																offset1.QuadPart, 
																Sftk_Dev->Lrdb.BitmapSizeBlockAlign, status ));

			// Log Event, and error handling, we should terminate here ??? or just continue ... ???
			swprintf( wchar, L"%I64d for LRDB Bitmap Dev %d ", offset.QuadPart, Sftk_Dev->cdev);
			swprintf( wchar1, L"%d", size);
			sftk_LogEventString3( GSftk_Config.DriverObject, MSG_REPL_PSTORE_READ_ERROR, status, 
									  0, Sftk_Lg->PStoreFileName.Buffer, wchar, wchar1);

			// since Dev Number has matched....delete this entry from pstore file
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: PSDEV LRDB is not valid, Deleting Dev Cdev %d entry in pstore file!\n", 
															pPsDev->cdev));
			status = sftk_delete_psdev_entry_in_pstore( pPsDev, fileHandle, &offset, size, TRUE);
			if ( !NT_SUCCESS(status) ) 
			{ // failed to write to pstore file
				DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: PSDEV to Delete cdev %d, our Dev %d sftk_delete_psdev_entry_in_pstore(%S, O %I64d S : %d ) Failed with status 0x%08x !\n", 
												pPsDev->cdev, Sftk_Dev->cdev, Sftk_Lg->PStoreFileName.Buffer,  offset.QuadPart, size, status ));
				// TODO : Log Event, and error handling, we should terminate here ??? or just continue ... ???
			}
			bFoundDev = FALSE;
			break;
		}

		#if PS_BITMAP_CHECKSUM_ON
		// Double check Checksum of LRDB bitmap Memory !!
		status = sftk_do_checksum_validation_of_PS_Bitmap( &pPsDev->Lrdb, (PUCHAR) pLrdbBits, Sftk_Dev->Lrdb.BitmapSize);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: LRDB : sftk_do_checksum_validation_of_PS_Bitmap(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
																Sftk_Dev->SftkLg->LGroupNumber, 
																Sftk_Dev->Vdevname,
																offset1.QuadPart, 
																Sftk_Dev->Lrdb.BitmapSize, status ));
			swprintf( wchar, L"Dev %d LRDB Bitmap", Sftk_Dev->cdev);
			sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_VALIDATION_FAILED, status, 
								  0, Sftk_Lg->PStoreFileName.Buffer, wchar);

			// TODO FIXME FIXME : We must ignore this error, and mark LG as full refresh requires !!!
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: Pstore LRDB Bitmap memory is corrupted checksum failed!!: TODO FIXME FIXME : We must ignore this error, and mark LG as full refresh requires !!! \n"));
			bLrdbIsValid = FALSE;
		}
		#endif

		#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
		// No need to verify LRDB since LRDB bitmap always must valid
		OS_ASSERT( Sftk_Dev->Lrdb.pPsBitmapHdr->MagicNum == SFTK_PS_BITMAP_REGION_MAGIC);
		// OS_ASSERT( Sftk_Dev->Lrdb.pPsBitmapHdr->data.TimeStamp.QuadPart != 0);
		#endif

		// ---- Read LRDB Bitmap in memory 
		offset1.QuadPart = pPsDev->HrdbOffset;
		
		#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
		OS_ASSERT( Sftk_Dev->Lrdb.pPsBitmapHdr != NULL);
		status = sftk_read_pstore(  fileHandle, &offset1, pHrdbPsBitmapHdr,
									Sftk_Dev->Hrdb.BitmapSizeBlockAlign);
		#else
		status = sftk_read_pstore( FileHandle, &offset1, pHrdbBits,
									Sftk_Dev->Hrdb.BitmapSizeBlockAlign);
		#endif
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file

			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: HRDB : sftk_read_pstore(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
																Sftk_Dev->SftkLg->LGroupNumber, 
																Sftk_Dev->Vdevname,
																offset1.QuadPart, 
																Sftk_Dev->Hrdb.BitmapSizeBlockAlign, status ));

			swprintf( wchar, L"%I64d for HRDB Bitmap Dev %d ", offset.QuadPart, Sftk_Dev->cdev);
			swprintf( wchar1, L"%d", size);
			sftk_LogEventString3( GSftk_Config.DriverObject, MSG_REPL_PSTORE_READ_ERROR, status, 
									  0, Sftk_Lg->PStoreFileName.Buffer, wchar, wchar1);

			bHrdbValid = FALSE;
		}

		#if PS_BITMAP_CHECKSUM_ON
		// Double check Checksum of HRDB bitmap Memory !!
		status = sftk_do_checksum_validation_of_PS_Bitmap( &pPsDev->Hrdb, (PUCHAR) pHrdbPsBitmapHdr, Sftk_Dev->Hrdb.BitmapSize);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to write to pstore file
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: HRDB : sftk_do_checksum_validation_of_PS_Bitmap(%S, LG 0x%08x for Device %s, O %I64d S : %d ) Failed with status 0x%08x !\n", 
																Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
																Sftk_Dev->SftkLg->LGroupNumber, 
																Sftk_Dev->Vdevname,
																offset1.QuadPart, 
																Sftk_Dev->Hrdb.BitmapSize, status ));
			swprintf( wchar, L"Dev %d HRDB Bitmap", Sftk_Dev->cdev);
			sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_VALIDATION_FAILED, status, 
								  0, Sftk_Lg->PStoreFileName.Buffer, wchar);

			// TODO FIXME FIXME : We must ignore this error, and mark LG as full refresh requires or just use LRDB !!!
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: Pstore LRDB Bitmap memory is corrupted checksum failed!!: TODO FIXME FIXME : We must ignore this error, and mark LG as full refresh requires or just use LRDB !!! \n"));
			bHrdbValid = FALSE;
		}
		#endif

		#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
		if (bHrdbValid == TRUE)
		{
			// Do Validation of HRDB Bitmap Region.
			OS_ASSERT( Sftk_Dev->Hrdb.pPsBitmapHdr->MagicNum == SFTK_PS_BITMAP_REGION_MAGIC);

			if ( !(IS_PS_HRDB_BITMAP_VALID(	pHrdbPsBitmapHdr, 
											pLrdbPsBitmapHdr)) )
			{ // HRDB bitmap is not valid, use it
				bHrdbValid = FALSE;
			}
		}
		#endif

		if (bHrdbValid == TRUE)
		{
			bitmap.Sectors_per_volume	= Sftk_Dev->Hrdb.Sectors_per_volume;
			bitmap.Sectors_per_bit		= Sftk_Dev->Hrdb.Sectors_per_bit;
			bitmap.TotalNumOfBits		= Sftk_Dev->Hrdb.TotalNumOfBits;
			bitmap.BitmapSize			= Sftk_Dev->Hrdb.BitmapSize;
			bitmap.BitmapSizeBlockAlign = Sftk_Dev->Hrdb.BitmapSizeBlockAlign;
			bitmap.pPsBitmapHdr			= pHrdbPsBitmapHdr;
			bitmap.pBits				= (PULONG) pHrdbBits;

			RtlInitializeBitMap( bitmap.pBitmapHdr, bitmap.pBits, bitmap.TotalNumOfBits); 
			
			// bitmap.DirtyMap				= Sftk_Dev->Lrdb.DirtyMap;
			// bitmap.Ranges_per_map		= Sftk_Dev->Lrdb.Ranges_per_map;
			sftk_Prepare_bitmapA_to_bitmapB( &Sftk_Dev->Lrdb, &bitmap, FALSE);
			sftk_Prepare_bitmapA_to_bitmapB( &Sftk_Dev->Hrdb, &bitmap, FALSE);
		}

		// Now Merge Pstore Lrdb into in memory LRDB 
		if (bLrdbIsValid == TRUE)	// this gets true at successful shutdown time
		{ // Call Routine to Merge Pstore LRDB with in memory LRDB
			bitmap.Sectors_per_volume	= Sftk_Dev->Lrdb.Sectors_per_volume;
			bitmap.Sectors_per_bit		= Sftk_Dev->Lrdb.Sectors_per_bit;
			bitmap.TotalNumOfBits		= Sftk_Dev->Lrdb.TotalNumOfBits;
			bitmap.BitmapSize			= Sftk_Dev->Lrdb.BitmapSize;
			bitmap.BitmapSizeBlockAlign = Sftk_Dev->Lrdb.BitmapSizeBlockAlign;
			bitmap.pPsBitmapHdr			= pLrdbPsBitmapHdr;
			bitmap.pBits				= (PULONG) pLrdbBits;

			RtlInitializeBitMap( bitmap.pBitmapHdr, bitmap.pBits, bitmap.TotalNumOfBits); 
			
			// bitmap.DirtyMap				= Sftk_Dev->Lrdb.DirtyMap;
			// bitmap.Ranges_per_map		= Sftk_Dev->Lrdb.Ranges_per_map;
			// Update LRDB
			sftk_Prepare_bitmapA_to_bitmapB( &Sftk_Dev->Lrdb, &bitmap, FALSE);
			// update HRdb also
			sftk_Prepare_bitmapA_to_bitmapB( &Sftk_Dev->Hrdb, &bitmap, FALSE);


			if (Sftk_Dev->SftkLg->PsHeader->LgInfo.state == SFTK_MODE_FULL_REFRESH)
			{ // Before reboot if we were running Full refresh...
				// Just for safety decrement counter
				Sftk_Dev->RefreshLastBitIndex = Sftk_Dev->PsDev->RefreshLastBitIndex; // TODO FIXME FIXME
				if ( (Sftk_Dev->RefreshLastBitIndex != DEV_LASTBIT_CLEAN_ALL) && (Sftk_Dev->RefreshLastBitIndex > 0) )
				{
					Sftk_Dev->RefreshLastBitIndex = Sftk_Dev->RefreshLastBitIndex - 1; // for safety 
					Sftk_Dev->PsDev->RefreshLastBitIndex = Sftk_Dev->RefreshLastBitIndex;
				}

#if 0 // If we Only allow to do Smart Refresh after boot time, enable this code. 
				// after reboot we always do Smart refresh...
				// Prepare new Hrdb bitmap for current device for full refresh
				// Make It one all Hrdb Bitmap from RefreshLastBitIndex to end of bitmap
				if ( (Sftk_Dev->RefreshLastBitIndex != DEV_LASTBIT_NO_CLEAN) &&
					 (Sftk_Dev->RefreshLastBitIndex != DEV_LASTBIT_CLEAN_ALL) )
				{ // update Lrdb and HRdb for Smart refresh from prev state Full resfresh before boot
					ULONG			numOfBytesPerBit,  startBit, endBit, lrdb_refreshLastBitIndex;
					LARGE_INTEGER	byteOffset;

					// first do for lrdb
					numOfBytesPerBit 	= Sftk_Dev->Hrdb.Sectors_per_bit * SECTOR_SIZE;  
					byteOffset.QuadPart = (LONGLONG) ( (INT64) Sftk_Dev->RefreshLastBitIndex * (INT64) numOfBytesPerBit ); 

					startBit = endBit = 0;
					sftk_bit_range_from_offset( &Sftk_Dev->Lrdb, 
												byteOffset.QuadPart, 
												(ULONG) numOfBytesPerBit, 
												&startBit, 
												&endBit );

					lrdb_refreshLastBitIndex = endbit;
					if (lrdb_refreshLastBitIndex > 0)
						lrdb_refreshLastBitIndex = lrdb_refreshLastBitIndex  - 1;	 // just for one bump down, for safety

					if (lrdb_refreshLastBitIndex <= Sftk_Dev->Lrdb.TotalNumOfBits)
					{
						// TODO : Why we need to increment its safe to not to do that worst to worst it will skip one bit 
						RtlSetBits( Sftk_Dev->Lrdb.pBitmapHdr, lrdb_refreshLastBitIndex, 
									(Sftk_Dev->Lrdb.TotalNumOfBits-1) );	// since endBit is zero based we should increment it to 1
					}

					// Now Do for Hrdb
					lrdb_refreshLastBitIndex = Sftk_Dev->RefreshLastBitIndex;
					if (lrdb_refreshLastBitIndex > 0)
						lrdb_refreshLastBitIndex = lrdb_refreshLastBitIndex  - 1;	 // just for one bump down, for safety
					
					if (lrdb_refreshLastBitIndex <= Sftk_Dev->Hrdb.TotalNumOfBits)
					{
						RtlSetBits( Sftk_Dev->Hrdb.pBitmapHdr, lrdb_refreshLastBitIndex, 
										(Sftk_Dev->Hrdb.TotalNumOfBits-1) );	// since endBit is zero based we should increment it to 1
					}
				} // update Lrdb and HRdb for Smart refresh from prev state Full resfresh before boot
#endif
			}

		}
		else
		{ // Pstore LRDB is not valid, Require Full refresh
			if (Sftk_Lg->state != SFTK_MODE_PASSTHRU)
			{
				sftk_lg_change_State(Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
				if (Sftk_Lg->state != SFTK_MODE_PASSTHRU)
					Sftk_Lg->state = SFTK_MODE_PASSTHRU; // Forced to do so...
			}
		}
		bFoundDev = TRUE;
		break;
	} // for each and every pstore device create and configure device.

	if (bFoundDev == TRUE)
	{
		OS_SetFlag( Sftk_Dev->Flags, SFTK_DEV_FLAG_PSTORE_FILE_ADDED);
		status = STATUS_SUCCESS;
	}
	else
	{ // Format this device into Pstore file since we have not found it
		if(Sftk_Dev->PsDev)
			OS_FreeMemory(Sftk_Dev->PsDev);
		Sftk_Dev->PsDev				= NULL;
		Sftk_Dev->SizeBAlignPsDev	= 0;

		OS_ClearFlag( Sftk_Dev->Flags, SFTK_DEV_FLAG_PSTORE_FILE_ADDED);

		status = sftk_create_dev_in_pstorefile(Sftk_Lg, Sftk_Dev);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			DebugPrint((DBG_ERROR, "sftk_Update_Dev_from_pstore:: Format: sftk_create_dev_in_pstorefile(LG %d File : %S, Dev %s) Failed with status 0x%08x, returning error !\n",
									Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, Sftk_Dev->Vdevname, status ));
			
			// Dev can't add to pstore file , so just delete this dev under LG later in config_end
			swprintf( wchar, L"Creating New Device %d", Sftk_Dev->cdev);
			sftk_LogEventNum2Wchar2(GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
						0, Sftk_Dev->cdev, Sftk_Dev->SftkLg->LGroupNumber, 
						Sftk_Dev->SftkLg->PStoreFileName.Buffer, wchar);

			goto done;
		}
		else
		{ // success
			OS_SetFlag( Sftk_Dev->Flags, SFTK_DEV_FLAG_PSTORE_FILE_ADDED);

			if (Sftk_Lg->state != SFTK_MODE_PASSTHRU)
			{
				sftk_lg_change_State(Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
				if (Sftk_Lg->state != SFTK_MODE_PASSTHRU)
					Sftk_Lg->state = SFTK_MODE_PASSTHRU; // Forced to do so...
			}
			status = STATUS_SUCCESS;
		}
	}

done:
	if (fileHandle) 
		ZwClose(fileHandle);

	if (bitmap.pBitmapHdr)
		OS_FreeMemory(bitmap.pBitmapHdr);

	if (pPsDev)
		OS_FreeMemory(pPsDev);

	if(pLrdbPsBitmapHdr)
		OS_FreeMemory(pLrdbPsBitmapHdr);

	if(pHrdbPsBitmapHdr)
		OS_FreeMemory(pHrdbPsBitmapHdr);

	if ( !NT_SUCCESS(status) ) 
	{ // failed, Do Cleanup here....
		if(Sftk_Dev->PsDev)
			OS_FreeMemory(Sftk_Dev->PsDev);
	
		Sftk_Dev->PsDev				= NULL;
		Sftk_Dev->SizeBAlignPsDev	= 0;

		OS_ClearFlag( Sftk_Dev->Flags, SFTK_DEV_FLAG_PSTORE_FILE_ADDED);
	}

	return status;
} // sftk_Update_Dev_from_pstore()

NTSTATUS
sftk_delete_psdev_entry_in_pstore(	PSFTK_PS_DEV	PsDev, HANDLE FileHandle, 
									PLARGE_INTEGER Offset, ULONG Size, BOOLEAN UseChecksum)
{
	NTSTATUS		status = STATUS_SUCCESS;
	LARGE_INTEGER	checksum;

	PsDev->Deleted = TRUE;

	if (UseChecksum == TRUE)
	{
		checksum.QuadPart = 0;

		PsDev->Checksum = 0;
		sftk_get_checksum( (PUCHAR) PsDev, sizeof(SFTK_PS_DEV), &checksum);
		PsDev->Checksum = checksum.QuadPart;
	}
				
	// Write Psdev bitmaps to Pstore file.
	status = sftk_write_pstore( FileHandle, Offset, PsDev, Size);
	if ( !NT_SUCCESS(status) ) 
	{ // failed to write to pstore file
		DebugPrint((DBG_ERROR, "sftk_delete_psdev_entry_in_pstore:: PSDEV to Delete cdev %d sftk_write_pstore(O %I64d S : %d ) Failed with status 0x%08x !\n", 
										PsDev->cdev, Offset->QuadPart, Size, status ));
	}

	return status;
} // sftk_delete_psdev_entry_in_pstore