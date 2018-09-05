/**************************************************************************************

Module Name: sftk_Bitmap.C   
Author Name: Parag sanghvi 
Description: Bitmap APIS
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

NTSTATUS
sftk_Create_DevBitmaps( IN OUT 	PSFTK_DEV		Sftk_Dev, 
					    IN		ftd_dev_info_t  *In_dev_info )
{
	NTSTATUS status = STATUS_SUCCESS;
	
	// We have Input : Size of Bitmap, size of Disk, calculate Chunksize means number of sectors per bit.

	// ----- LRDB Initialize 
	status = sftk_Init_Bitmaps( &Sftk_Dev->Lrdb, &In_dev_info->disksize, &In_dev_info->lrdbsize32);

	if (status != STATUS_SUCCESS)
	{
		DebugPrint((DBG_ERROR, "sftk_Create_DevBitmaps:: sftk_Init_Bitmaps( for LRDB ) Failed, returning status 0x%08x !\n",Sftk_Dev->Lrdb.BitmapSize, status));
		goto done;
	}

	DebugPrint((DBG_CONFIG, "sftk_Create_DevBitmaps:: LRDB: BitmapSize %d, Sectors_per_volume %d, Sectors_per_bit %d, TotalNumOfBits %d! \n", 
								Sftk_Dev->Lrdb.BitmapSize, Sftk_Dev->Lrdb.Sectors_per_volume, 
								Sftk_Dev->Lrdb.Sectors_per_bit, Sftk_Dev->Lrdb.TotalNumOfBits));
	
	// ----- HRDB Initialize 
	status = sftk_Init_Bitmaps( &Sftk_Dev->Hrdb, &In_dev_info->disksize, &In_dev_info->hrdbsize32);

	if (status != STATUS_SUCCESS)
	{
		DebugPrint((DBG_ERROR, "sftk_Create_DevBitmaps:: sftk_Init_Bitmaps( for HRDB ) Failed, returning status 0x%08x !\n",Sftk_Dev->Lrdb.BitmapSize, status));
		goto done;
	}

	DebugPrint((DBG_CONFIG, "sftk_Create_DevBitmaps:: Hrdb: BitmapSize %d, Sectors_per_volume %d, Sectors_per_bit %d, TotalNumOfBits %d! \n", 
								Sftk_Dev->Hrdb.BitmapSize, Sftk_Dev->Hrdb.Sectors_per_volume, 
								Sftk_Dev->Hrdb.Sectors_per_bit, Sftk_Dev->Hrdb.TotalNumOfBits));

#if TARGET_SIDE
	status = sftk_Init_Bitmaps( &Sftk_Dev->Srdb, &In_dev_info->disksize, &In_dev_info->hrdbsize32);

	if (status != STATUS_SUCCESS)
	{
		DebugPrint((DBG_ERROR, "sftk_Create_DevBitmaps:: sftk_Init_Bitmaps( for SRDB ) Failed, returning status 0x%08x !\n",Sftk_Dev->Lrdb.BitmapSize, status));
		goto done;
	}

	DebugPrint((DBG_CONFIG, "sftk_Create_DevBitmaps:: Srdb: BitmapSize %d, Sectors_per_volume %d, Sectors_per_bit %d, TotalNumOfBits %d! \n", 
								Sftk_Dev->Srdb.BitmapSize, Sftk_Dev->Srdb.Sectors_per_volume, 
								Sftk_Dev->Srdb.Sectors_per_bit, Sftk_Dev->Srdb.TotalNumOfBits));
#endif

	// ----- ALrdb Initialize - Only Acknowledge Thread uses (builds) this bitmaps and merge it to Lrdb and Hrdb 
	status = sftk_Init_Bitmaps( &Sftk_Dev->ALrdb, &In_dev_info->disksize, &In_dev_info->lrdbsize32);

	if (status != STATUS_SUCCESS)
	{
		DebugPrint((DBG_ERROR, "sftk_Create_DevBitmaps:: sftk_Init_Bitmaps( for ALrdb ) Failed, returning status 0x%08x !\n",Sftk_Dev->Lrdb.BitmapSize, status));
		goto done;
	}

#if 1 // For Backward Compatibility with service
	{ // initalize Bitmap's shift, len32, numbits
	LONG	diskbits, lrdbbits, hrdbbits, i;

	diskbits = sftk_get_msb(In_dev_info->disksize) + 1 + DEV_BSHIFT;
    lrdbbits = sftk_get_msb(In_dev_info->lrdbsize32 * 4);
    hrdbbits = sftk_get_msb(In_dev_info->hrdbsize32 * 4);

	// LRDB
    Sftk_Dev->Lrdb.bitsize = diskbits - lrdbbits;

    if (Sftk_Dev->Lrdb.bitsize < MINIMUM_LRDB_BITSIZE)
        Sftk_Dev->Lrdb.bitsize = MINIMUM_LRDB_BITSIZE;

    Sftk_Dev->Lrdb.shift	= Sftk_Dev->Lrdb.bitsize - DEV_BSHIFT;
    // Sftk_Dev->Lrdb.len32	= In_dev_info->lrdbsize32;
    Sftk_Dev->Lrdb.numbits	= (In_dev_info->disksize + 
			((1 << Sftk_Dev->Lrdb.shift) - 1)) >> Sftk_Dev->Lrdb.shift;

	// HRDB
	Sftk_Dev->Hrdb.bitsize = diskbits - hrdbbits;
    if (Sftk_Dev->Hrdb.bitsize < MINIMUM_HRDB_BITSIZE)
        Sftk_Dev->Hrdb.bitsize = MINIMUM_HRDB_BITSIZE;

    Sftk_Dev->Hrdb.shift = Sftk_Dev->Hrdb.bitsize - DEV_BSHIFT;

    // Sftk_Dev->Hrdb.len32 = info->hrdbsize32;
    Sftk_Dev->Hrdb.numbits = (In_dev_info->disksize + 
        ((1 << Sftk_Dev->Hrdb.shift) - 1)) >> Sftk_Dev->Hrdb.shift;

	}
#endif

	Sftk_Dev->noOfHrdbBitsPerLrdbBit = 0;
	Sftk_Dev->noOfHrdbBitsPerLrdbBit = Sftk_Dev->Lrdb.Sectors_per_bit / Sftk_Dev->Hrdb.Sectors_per_bit;

	OS_ASSERT( Sftk_Dev->Lrdb.Sectors_per_bit > Sftk_Dev->Hrdb.Sectors_per_bit);

	if ((Sftk_Dev->Lrdb.Sectors_per_bit % Sftk_Dev->Hrdb.Sectors_per_bit) != 0)
	{
		DebugPrint((DBG_ERROR, "sftk_Create_DevBitmaps:: (DestBitmap->Sectors_per_bit 0x%08x Modulo SrcBitmap->Sectors_per_bit 0x%08x) != 0!\n",
												Sftk_Dev->Lrdb.Sectors_per_bit, Sftk_Dev->Hrdb.Sectors_per_bit));

		DebugPrint((DBG_ERROR, "sftk_Create_DevBitmaps:: TODO FIXME FIXME Mapping of Bitmap will be partial !!!! !\n"));
		Sftk_Dev->noOfHrdbBitsPerLrdbBit ++;
	}

	Sftk_Dev->Statistics.NumOfBlksPerBit = Sftk_Dev->Hrdb.Sectors_per_bit;

	DebugPrint((DBG_CONFIG, "sftk_Create_DevBitmaps:: ALrdb: BitmapSize %d, Sectors_per_volume %d, Sectors_per_bit %d, TotalNumOfBits %d! \n", 
								Sftk_Dev->ALrdb.BitmapSize, Sftk_Dev->ALrdb.Sectors_per_volume, 
								Sftk_Dev->ALrdb.Sectors_per_bit, Sftk_Dev->ALrdb.TotalNumOfBits));

done:
	if (!NT_SUCCESS(status))
	{ // failed, do cleanup
		sftk_Delete_DevBitmaps( Sftk_Dev );
	}

	return status;
} // sftk_Create_DevBitmaps()

NTSTATUS
sftk_Init_Bitmaps(	IN OUT 	PSFTK_BITMAP	Bitmap, 
					IN		PULONG			DiskSize,		// in sectors, Total Disk size = 2 Tera Bytes size limitation
					IN		PULONG			BitmapMemSize)	// total size of memory in bytes used to allocate Bitmap
{
	NTSTATUS status = STATUS_SUCCESS;

	OS_ASSERT(Bitmap != NULL);	OS_ASSERT(DiskSize != NULL);	OS_ASSERT(BitmapMemSize != NULL);

	// set the disk size in sectors
	Bitmap->Sectors_per_volume	= *DiskSize;

	// following fields is nothing but backward compatiblitiy, BitmapMemSize value is from service thru IOCTL
	Bitmap->len32				= *BitmapMemSize;	// * 4 = Actual Bytes allocated for bitmap.

	// Get Actual Bitmaps Bytes allovation value.
	Bitmap->BitmapSize			= Bitmap->len32 * 4;	// Lrdb = 8k, Pstore.	Format uses this much

	// - now calculate Sectors_per_bit = chunk_size of Bitmap
	if ( (Bitmap->BitmapSize * 8) >= Bitmap->Sectors_per_volume)
	{	// 1 bit = 1 Sector, since we have enough bitmap size.
		Bitmap->Sectors_per_bit = 1;
	}
	else
	{
		Bitmap->Sectors_per_bit = (ULONG) (Bitmap->Sectors_per_volume / (Bitmap->BitmapSize * 8) );

		if ( (ULONG) (Bitmap->Sectors_per_volume % (Bitmap->BitmapSize * 8) ) )
			Bitmap->Sectors_per_bit ++;	// increment one more block per bit.
	}

	// get Total number of bits used.
	Bitmap->TotalNumOfBits = (Bitmap->Sectors_per_volume / Bitmap->Sectors_per_bit);

	if ( (ULONG) (Bitmap->Sectors_per_volume % Bitmap->Sectors_per_bit))
		Bitmap->TotalNumOfBits ++;

	OS_ASSERT( (Bitmap->TotalNumOfBits / 8) <= Bitmap->BitmapSize);


	Bitmap->DirtyMap = 0;

	// Allocate Bitmap buffer memory Sector Align since we used it to flush to pstore file 
	Bitmap->BitmapSizeBlockAlign = BLOCK_ALIGN_UP(Bitmap->BitmapSize);
	
#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
	OS_ASSERT((sizeof(SFTK_PS_BITMAP_REGION_HDR) % SECTOR_SIZE) == 0);
	Bitmap->BitmapSizeBlockAlign += sizeof(SFTK_PS_BITMAP_REGION_HDR);
#endif

	OS_ASSERT((Bitmap->BitmapSizeBlockAlign % SECTOR_SIZE) == 0);

	// Allocate Bitmap Buffer
	Bitmap->pBits	= OS_AllocMemory( NonPagedPool, Bitmap->BitmapSizeBlockAlign); // Bitmap->BitmapSize;
	if (!Bitmap->pBits)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_Init_Bitmaps:: OS_AllocMemory( BitmapSize %d) Failed, returning status 0x%08x !\n",
												Bitmap->BitmapSizeBlockAlign, status));
		goto done;
	}

	OS_ZeroMemory(Bitmap->pBits, Bitmap->BitmapSizeBlockAlign);

#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
	Bitmap->pPsBitmapHdr	= (PSFTK_PS_BITMAP_REGION_HDR) Bitmap->pBits;
	Bitmap->pBits			= (PULONG) ((ULONG) Bitmap->pPsBitmapHdr + sizeof(SFTK_PS_BITMAP_REGION_HDR));
	Bitmap->pPsBitmapHdr->MagicNum = SFTK_PS_BITMAP_REGION_MAGIC;
#else
	Bitmap->pPsBitmapHdr = (PSFTK_PS_BITMAP_REGION_HDR) Bitmap->pBits;
#endif

	// Allocate RTL Bitmap Header Buffer
	Bitmap->pBitmapHdr = OS_AllocMemory( NonPagedPool, sizeof(RTL_BITMAP));
	if (!Bitmap->pBitmapHdr)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_Init_Bitmaps:: OS_AllocMemory( sizeof(RTL_BITMAP) %d) Failed, returning status 0x%08x !\n",
									sizeof(RTL_BITMAP), status));
		goto done;
	}

	// Initialize Bitmap Lock, TODO : Do we need this !! ? 
	OS_INITIALIZE_LOCK( &Bitmap->Lock, OS_ERESOURCE_LOCK, NULL);

	// Initialize Rtl Bitmap Routines.
	// Total Number of bits = SrcBitmap->Blocks_per_volume
	RtlInitializeBitMap( Bitmap->pBitmapHdr, Bitmap->pBits, Bitmap->TotalNumOfBits); 

	// Always clear bitmap first before usage...
	RtlClearAllBits( Bitmap->pBitmapHdr); 

done:
	if (status != STATUS_SUCCESS)
	{ // Failure
		sftk_DeInit_Bitmaps( Bitmap );
	}

	return status;
} // sftk_Init_Bitmaps()

NTSTATUS
sftk_DeInit_Bitmaps(IN OUT 	PSFTK_BITMAP	Bitmap)
{
	if (Bitmap->pBitmapHdr)
	{
		OS_DEINITIALIZE_LOCK( &Bitmap->Lock, NULL);	// we may have allocate Lock for this
		OS_FreeMemory(Bitmap->pBitmapHdr);
	}

	if (Bitmap->pPsBitmapHdr)
		OS_FreeMemory(Bitmap->pPsBitmapHdr);

	OS_ZeroMemory( Bitmap, sizeof(SFTK_BITMAP));

	return STATUS_SUCCESS;
} // sftk_DeInit_Bitmaps()

NTSTATUS
sftk_Delete_DevBitmaps( IN OUT 	PSFTK_DEV	Sftk_Dev )
{
	// Delete all Bitmaps.
	sftk_DeInit_Bitmaps( &Sftk_Dev->Lrdb );
	sftk_DeInit_Bitmaps( &Sftk_Dev->Hrdb );
#if TARGET_SIDE
	sftk_DeInit_Bitmaps( &Sftk_Dev->Srdb );
#endif
	sftk_DeInit_Bitmaps( &Sftk_Dev->ALrdb );

	return STATUS_SUCCESS;
} // sftk_Delete_DevBitmaps()

NTSTATUS
sftk_bit_range_from_offset( IN		PSFTK_BITMAP Bitmap, 
							IN		UINT64		 Offset,	// In bytes
							IN		ULONG		 Size,		// In bytes
							IN	OUT	PULONG		 StartBit,  // Returninus Starting Bit
							IN	OUT	PULONG		 EndBit)    // Ending Bit, include this bit number too.
{
	ULONG	blkOffset, blkSize;
	ULONG	numOfBits;

	OS_ASSERT( (Offset % SECTOR_SIZE) == 0);	// must be divisible of 512, Sector Operations
	OS_ASSERT( (Size % SECTOR_SIZE) == 0);		// must be divisible of 512, Sector Operations
	
	blkOffset	= (ULONG) (Offset / (UINT64) SECTOR_SIZE);
	blkSize		= Size / SECTOR_SIZE;
	
	// Determine where/how much we're writing from the IRP
	*StartBit	= blkOffset / Bitmap->Sectors_per_bit; 

	// if(blkSize	<= Bitmap->Sectors_per_bit) 
    //		numOfBits = 1; 
	// else
	numOfBits = blkSize / Bitmap->Sectors_per_bit; 

	// If we cross a block boundary, then we need to bump the block count to ensure proper bits set
	if(blkSize % Bitmap->Sectors_per_bit)
		numOfBits ++;
	
	*EndBit	= *StartBit + (numOfBits-1); 

	return STATUS_SUCCESS;

} // sftk_bit_range_from_offset()


NTSTATUS
sftk_offset_from_bit_range( IN		PSFTK_BITMAP Bitmap, 
							IN	OUT	PUINT64		 Offset,	// In bytes
							IN	OUT	PULONG		 Size,		// In bytes
							IN		ULONG		 StartBit,  // Returninus Starting Bit
							IN		ULONG		 EndBit)    // Ending Bit, include this bit number number too.
{
	ULONG	blkOffset, blkSize;
	ULONG	numOfBits;

	blkOffset	= (ULONG) ( StartBit * Bitmap->Sectors_per_bit );		// in sectors
	*Offset		= (UINT64) ((UINT64) blkOffset * (UINT64) SECTOR_SIZE);	// in bytes

	numOfBits = EndBit - StartBit + 1;

	blkSize		= (ULONG) ( numOfBits * Bitmap->Sectors_per_bit );		// in sectors
	*Size		= (ULONG) ((ULONG) blkSize * (ULONG) SECTOR_SIZE);		// in Bytes

	OS_ASSERT( (*Offset % SECTOR_SIZE) == 0);	// must be divisible of 512, Sector Operations
	OS_ASSERT( (*Size % SECTOR_SIZE) == 0);		// must be divisible of 512, Sector Operations
	
	return STATUS_SUCCESS;

} // sftk_offset_from_bit_range()


//
// Returns TRUE :	If it updates bitmap for specified range, 
//					(makes any or all bit(s) dirty (set to 1) in the range)
// Returns FALSE :	If it does not updates any bit(s) in specified range of bitmap, 
//					(All bits are already marked as dirty (set to 1) for specified range)
//
BOOLEAN		
sftk_Update_bitmap(		IN		PSFTK_DEV	 Sftk_Dev, 
						IN		UINT64		 Offset,	// In bytes
						IN		ULONG		 Size)		// In bytes    
{
	BOOLEAN	bret	 = FALSE;	// Defualt return
	BOOLEAN	bBitsSet = FALSE;
	ULONG	startBit, endBit, numBits;
	LONG	currentState;
	

	OS_ASSERT(Sftk_Dev->SftkLg != NULL);
	OS_ASSERT( Sftk_Dev->Lrdb.pBitmapHdr != NULL);	// check Memory validation 
	OS_ASSERT( Sftk_Dev->Lrdb.pBits		 != NULL);	// check Memory validation 
	OS_ASSERT( Size		 != 0);	// check Memory validation 

	if (Size == 0)
		return FALSE;

	startBit = endBit = numBits = 0;

	sftk_bit_range_from_offset( &Sftk_Dev->Lrdb, Offset, Size, &startBit, &endBit );

	if ( sftk_Check_bits_to_update(Sftk_Dev, TRUE, &startBit, &endBit) == TRUE )
	{ // Update Bitrange
		numBits = (endBit-startBit+1);

		OS_ASSERT( (startBit + numBits) <= Sftk_Dev->Lrdb.TotalNumOfBits);	// check boundary conditions

		bBitsSet = RtlAreBitsSet( Sftk_Dev->Lrdb.pBitmapHdr, startBit, numBits );
		if (bBitsSet == TRUE)
		{	// if all bits are set to 1, nothing to do, just return FALSE
			bret = FALSE;
		}
		else
		{	// if all or any one bit in range is not set to 1, update all Bits range to 1.
			bret = TRUE;
			RtlSetBits( Sftk_Dev->Lrdb.pBitmapHdr, startBit, numBits );
			// TODO : Now Update Dirty Mapping to optimize Pstore Updates
		}
	} // Update Bitrange

	// Any how go ahead and update ALrdb since no harm doing this
	if (Sftk_Dev->SftkLg->DualLrdbMode == TRUE)
	{ // Also Update ALrdb bitmap here
		RtlSetBits( Sftk_Dev->ALrdb.pBitmapHdr, startBit, numBits );
	}

	if (Sftk_Dev->SftkLg->CopyLrdb == TRUE)
	{ // copy ALrdb to LRdb and reset respective fileds to FALSE
		Sftk_Dev->SftkLg->CopyLrdb		= FALSE;
		Sftk_Dev->SftkLg->DualLrdbMode	= FALSE;
		RtlCopyMemory( Sftk_Dev->Lrdb.pBits, Sftk_Dev->ALrdb.pBits, Sftk_Dev->Lrdb.BitmapSize );
		bret = TRUE;	// we have to flush new LRDB bitmaps
	}

	currentState = sftk_lg_get_state( Sftk_Dev->SftkLg);
	if ( ( currentState == SFTK_MODE_FULL_REFRESH) || ( currentState == SFTK_MODE_TRACKING) )
	{	// check and update HRDB bitmap
		startBit = endBit = numBits = 0;
		sftk_bit_range_from_offset( &Sftk_Dev->Hrdb, Offset, Size, &startBit, &endBit );

		if ( sftk_Check_bits_to_update(Sftk_Dev, FALSE, &startBit, &endBit) == TRUE )
		{ // Update Bitrange
			numBits = (endBit-startBit+1);

			OS_ASSERT( (startBit + numBits) <= Sftk_Dev->Hrdb.TotalNumOfBits);	// check boundary conditions

			OS_ASSERT( Sftk_Dev->Hrdb.pBitmapHdr != NULL);	// check Memory validation 
			OS_ASSERT( Sftk_Dev->Hrdb.pBits		 != NULL);	// check Memory validation 

			bBitsSet = RtlAreBitsSet( Sftk_Dev->Hrdb.pBitmapHdr, startBit, numBits );
			if (bBitsSet == FALSE)
			{ // if all or any one bit in range is not set to 1
				// update all Bits range to 1.
				RtlSetBits( Sftk_Dev->Hrdb.pBitmapHdr, startBit, numBits );
			}
		} // Update Bitrange
	}
#if TARGET_SIDE 
	if (Sftk_Dev->SftkLg->UseSRDB == TRUE) 
	{ // update SRDB bitmaps
		startBit = endBit = numBits = 0;
		sftk_bit_range_from_offset( &Sftk_Dev->Srdb, Offset, Size, &startBit, &endBit );

		numBits = (endBit-startBit+1);

		OS_ASSERT( (startBit + numBits) <= Sftk_Dev->Srdb.TotalNumOfBits);	// check boundary conditions

		OS_ASSERT( Sftk_Dev->Srdb.pBitmapHdr != NULL);	// check Memory validation 
		OS_ASSERT( Sftk_Dev->Srdb.pBits		 != NULL);	// check Memory validation 

		bBitsSet = RtlAreBitsSet( Sftk_Dev->Srdb.pBitmapHdr, startBit, numBits );
		if (bBitsSet == FALSE)
		{ // if all or any one bit in range is not set to 1
			// update all Bits range to 1.
			RtlSetBits( Sftk_Dev->Srdb.pBitmapHdr, startBit, numBits );
		}
	} // update SRDB bitmaps
#endif

	return bret;	// TRUE means bit(s) is updated else FALSE no change in bitmap...
} // sftk_Update_bitmap()

LONG
sftk_get_msb(LONG num)
{
    LONG	i;
    ULONG	temp = (ULONG)num;

    for (i = 0;i < 32;i++, temp >>= 1) 
    {
        if (temp == 0) break;
    }

    return (i-1);
} // sftk_get_msb()

NTSTATUS
sftk_Prepare_bitmapA_to_bitmapB(IN		PSFTK_BITMAP	DestBitmap, 
								IN		PSFTK_BITMAP	SrcBitmap,
								IN		BOOLEAN			CleanDestBitmapToStart)
{
	ULONG	noOfBitsDiff	= 0;
	ULONG	nextSrcSetBitIndex, nextDestBitToSet;
	BOOLEAN bret;
	BOOLEAN	bReachLastBoundary	= FALSE;
	ULONG	firstBitNumSet	= 0;
	BOOLEAN	bFirsttime = TRUE;


	nextSrcSetBitIndex = 0;

	// first make dest bit all zeros
	if (CleanDestBitmapToStart == TRUE)
	{
		RtlClearAllBits( DestBitmap->pBitmapHdr );
	}

	if (SrcBitmap->Sectors_per_bit > DestBitmap->Sectors_per_bit)
	{ // Src Bitmap has bigger chunksize Example: Src = LRDB, Dest = HRDB, LRdb to HRdb
		noOfBitsDiff = SrcBitmap->Sectors_per_bit / DestBitmap->Sectors_per_bit;

		if ((SrcBitmap->Sectors_per_bit % DestBitmap->Sectors_per_bit) != 0)
		{
			DebugPrint((DBG_ERROR, "sftk_Prepare_bitmapA_to_bitmapB:: (SrcBitmap->Sectors_per_bit 0x%08x Modulo DestBitmap->Sectors_per_bit 0x%08x) != 0!\n",
												SrcBitmap->Sectors_per_bit, DestBitmap->Sectors_per_bit));

			DebugPrint((DBG_ERROR, "sftk_Prepare_bitmapA_to_bitmapB:: TODO FIXME FIXME Mapping of Bitmap will be partial !!!! !\n"));
			noOfBitsDiff ++;
		}

		firstBitNumSet = RtlFindSetBits(	SrcBitmap->pBitmapHdr, 
											1, 
											nextSrcSetBitIndex);
		bFirsttime = TRUE;
		while( ( (nextSrcSetBitIndex = RtlFindSetBits(SrcBitmap->pBitmapHdr, 1, nextSrcSetBitIndex)) != firstBitNumSet) 
					|| 
				 (bFirsttime == TRUE) )
		{ // While: Found next set bit in SrcBitmap
			if (nextSrcSetBitIndex == DEV_LASTBIT_CLEAN_ALL)
				break;

			bFirsttime = FALSE;
			nextDestBitToSet = nextSrcSetBitIndex * noOfBitsDiff;

			if ( (nextDestBitToSet + noOfBitsDiff) > DestBitmap->TotalNumOfBits)
			{
				noOfBitsDiff = DestBitmap->TotalNumOfBits - nextDestBitToSet;
				bReachLastBoundary = TRUE;

			}
			OS_ASSERT( (nextDestBitToSet + noOfBitsDiff) <= DestBitmap->TotalNumOfBits);	// check boundary conditions

			RtlSetBits( DestBitmap->pBitmapHdr, nextDestBitToSet,  noOfBitsDiff);

			if (bReachLastBoundary == TRUE)
				break;

			nextSrcSetBitIndex ++; // bump up next Src bit index to check

			if (nextSrcSetBitIndex >= SrcBitmap->TotalNumOfBits)
			{ // We are done checking all Src bitmap
				break; // we are done
			} 
		} // While: Found next set bit in SrcBitmap

	} // Src Bitmap has bigger chunksize Example: Src = LRDB, Dest = HRDB, LRdb to HRdb  
	else
	{ // Dst Bitmap has bigger chunksize, Example: Src = HRDB, Dest = LRDB, HRdb to LRdb
		noOfBitsDiff = DestBitmap->Sectors_per_bit / SrcBitmap->Sectors_per_bit;

		if ((DestBitmap->Sectors_per_bit % SrcBitmap->Sectors_per_bit) != 0)
		{
			DebugPrint((DBG_ERROR, "sftk_Prepare_bitmapA_to_bitmapB:: (DestBitmap->Sectors_per_bit 0x%08x Modulo SrcBitmap->Sectors_per_bit 0x%08x) != 0!\n",
												DestBitmap->Sectors_per_bit, SrcBitmap->Sectors_per_bit));

			DebugPrint((DBG_ERROR, "sftk_Prepare_bitmapA_to_bitmapB:: TODO FIXME FIXME Mapping of Bitmap will be partial !!!! !\n"));
			noOfBitsDiff ++;
		}

		for( nextSrcSetBitIndex = 0, nextDestBitToSet =0;; )
		{
			// check Src Bitmap Boundary condition 
			if ( (nextSrcSetBitIndex + noOfBitsDiff) > SrcBitmap->TotalNumOfBits)
			{ // readjust noOfBitsDiff to check, since its last Bit indexs
				noOfBitsDiff = SrcBitmap->TotalNumOfBits - nextSrcSetBitIndex;
				bReachLastBoundary = TRUE;
			} 
			OS_ASSERT( (nextSrcSetBitIndex + noOfBitsDiff) <= SrcBitmap->TotalNumOfBits);	// check boundary conditions

			// Check if any bits are set in calculate Bit Runs (noOfBitsDiff)
			bret = RtlAreBitsClear( SrcBitmap->pBitmapHdr, nextSrcSetBitIndex, noOfBitsDiff);
			if (bret == FALSE)
			{ // Any bit in given range is set !! So update Destination bit
				RtlSetBits( DestBitmap->pBitmapHdr, nextDestBitToSet,  1);
			}

			// bump up bit index for src and dst
			nextSrcSetBitIndex += noOfBitsDiff,
			nextDestBitToSet ++;

			if (nextDestBitToSet >= DestBitmap->TotalNumOfBits)
			{ // We are done with all bitmaps in DestBitmap
				// OS_ASSERT(bReachLastBoundary == TRUE);	// FIXME FIXME
				break;
			} 

			if (bReachLastBoundary == TRUE)
			{
				//OS_ASSERT(nextDestBitToSet >= DestBitmap->TotalNumOfBits); // FIXME FIXME 
				break;	// FIXME FIXME
			}
		} // for( nextSrcSetBitIndex = 0, nextDestBitToSet =0;; )

	} // Dst Bitmap has bigger chunksize, Example: Src = HRDB, Dest = LRDB, HRdb to LRdb
	
	return STATUS_SUCCESS;
} // sftk_Prepare_bitmapA_to_bitmapB

//
// TRUE means update this range else do not update this range
//
BOOLEAN
sftk_Check_bits_to_update(	IN		PSFTK_DEV	Sftk_Dev, 
							IN		BOOLEAN		LrdbCheck,	// TRUE means Lrdb else HRDB check
							IN		PULONG		StartBit, 
							IN		PULONG		EndBit)
{
	if (Sftk_Dev->SftkLg->state != SFTK_MODE_FULL_REFRESH)
		return TRUE;	// Update this range, we check only for Full refresh mode.

	if ( (Sftk_Dev->RefreshLastBitIndex == DEV_LASTBIT_CLEAN_ALL)  
					|| 
		 (Sftk_Dev->RefreshLastBitIndex == DEV_LASTBIT_NO_CLEAN) )
		 return TRUE;	// Update this range, Nothing to check

	// Remember : RefreshLastBitIndex is always based on HRDB bitmap resolutions
	if (LrdbCheck == TRUE)
	{ // Lrdb Check
		ULONG currentLrdbBitIndex = Sftk_Dev->RefreshLastBitIndex / Sftk_Dev->noOfHrdbBitsPerLrdbBit;

		// check here bit number with pSftk_Dev->RefreshLastBitIndex 
		// if startBit to set is <= pSftk_Dev->RefreshLastBitIndex + 2) than do update bitmaps.
		if (*StartBit <= FULL_REFRESH_LAST_INDEX(currentLrdbBitIndex))
			return TRUE;	// Update this range
		else
			return FALSE;	// Do Not update this range

	} // if (LrdbCheck == TRUE)
	else
	{ // Hrdb Check
		// check here bit number with pSftk_Dev->RefreshLastBitIndex 
		// if startBit to set is <= pSftk_Dev->RefreshLastBitIndex + 2) than do update bitmaps.
		if (*StartBit <= FULL_REFRESH_LAST_INDEX(Sftk_Dev->RefreshLastBitIndex))
			return TRUE;	// Update this range
		else
			return FALSE;	// Do Not update this range
	}
		
	return TRUE;	// Update this range
} // sftk_Check_bits_to_update()
