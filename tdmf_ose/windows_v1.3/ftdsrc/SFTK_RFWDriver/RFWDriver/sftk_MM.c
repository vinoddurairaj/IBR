/**************************************************************************************

Module Name: sftk_MM.C   
Author Name: Parag sanghvi 
Description: Memory Manager APIS are deinfed here.
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

//
//	Function:	Sftk_Ctl_MM_Get_DB_Size()
//
//	Parameters: 
//		IN OUT	PIRP 		Gets and Return Infromation in PIRP SystemBuffer
//
//	Description:
//		Gets PMM_DATABASE_SIZE_ENTRIES as input/Output buffer from IRP,
//		Returns MM each and evry MM_TYPE based its nodesize and other information in 
//		structure PMM_DATABASE_SIZE_ENTRIES.
//		If AWE is used than it does not pass information for MM_Type : MM_TYPE_CHUNK_ENTRY
//		else it passes Information for MM_Type : MM_TYPE_CHUNK_ENTRY 
//
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
Sftk_Ctl_MM_Start_And_Get_DB_Size( PIRP Irp )
{
	PIO_STACK_LOCATION			pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	ULONG						sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	PMM_DATABASE_SIZE_ENTRIES	pMMDbSizeEntries	= Irp->AssociatedIrp.SystemBuffer;
	NTSTATUS					status				= STATUS_SUCCESS;

	if (sizeOfBuffer <	sizeof(MM_DATABASE_SIZE_ENTRIES))
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Get_DB_Size: sizeOfBuffer %d < sizeof(MM_DATABASE_SIZE_ENTRIES) %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeof(MM_DATABASE_SIZE_ENTRIES), status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}
	
	// Initialized if Memmory manager is not intialized before and 
	// Returns MM each and evry MM_TYPE based its nodesize and other information in 
	// structure PMM_DATABASE_SIZE_ENTRIES.
	status = mm_start( pMMDbSizeEntries, &GSftk_Config.Mmgr);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Get_DB_Size: mm_start() Failed with status 0x%08x !!! \n",
										 status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	return status;
} // Sftk_Ctl_MM_Get_DB_Size()

//
//	Function:	Sftk_Ctl_MM_Set_DB_Size()
//	Parameters: 
//		IN OUT	PIRP 		Gets Infromation in PIRP SystemBuffer
//
//	Description:
//		Input: PSET_MM_DATABASE_MEMORY, This API process it 
//		It Creates MDL and locked down memory, and based on input MM_Type Defined
//		it formats allocated number of nodes and put it in MM_ANCHOR->FreeList of specified MM_TYPE
//		It Returns only status.
//
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
Sftk_Ctl_MM_Set_DB_Size( PIRP Irp )
{
	PIO_STACK_LOCATION			pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	ULONG						sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	PSET_MM_DATABASE_MEMORY		pMMDb				= Irp->AssociatedIrp.SystemBuffer;
	NTSTATUS					status				= STATUS_SUCCESS;
	ULONG						sizeOfmem;

	if (sizeOfBuffer <	sizeof(SET_MM_DATABASE_MEMORY))
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Set_DB_Size: sizeOfBuffer %d < sizeof(SET_MM_DATABASE_MEMORY) %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeof(SET_MM_DATABASE_MEMORY), status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	sizeOfmem = sizeof(SET_MM_DATABASE_MEMORY) + (pMMDb->NumberOfArray * sizeof(VIRTUAL_MM_INFO));

	if (sizeOfBuffer < sizeOfmem)
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Set_DB_Size: sizeOfBuffer %d < sizeOfmem Expected %d, NumberOfArray %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeOfmem, pMMDb->NumberOfArray, status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	if ( (pMMDb->MMIndex > MM_TYPE_MAX) || (pMMDb->MMIndex == MM_TYPE_4K_NPAGED_MEM) )
	{
		status = STATUS_INVALID_PARAMETER;
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Set_DB_Size: Invalid MM_Type %d,  Failed with status 0x%08x !!! \n",
										pMMDb->MMIndex, status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	status = mm_type_alloc_init(	&GSftk_Config.Mmgr,
									pMMDb);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Set_DB_Size: mm_type_alloc_init() Failed with status 0x%08x !!! \n",
										 status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	return status;
} // Sftk_Ctl_MM_Set_DB_Size()

//
//	Function:	Sftk_Ctl_MM_Init_Raw_Memory()
//
//	Parameters: 
//		IN OUT	PIRP 		Gets Infromation in PIRP SystemBuffer
//
//	Description:
//		Input: PSET_MM_RAW_MEMORYY, This API process it 
//		It creates MDL for one paged at a time, Locks down and retrievs Physical Page and stores it into MM_ENTRY_LIST.
//		If AWE is not used than it allocates MM_CHUNK_ENTRY and stores baseVaddr into it. Puts all of its 
//		Physical pages into MM_ENTRY structure linked into MM_CHUNK_ENTRY->MMEntryList Anchor.
//		It Returns only status.
//
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
Sftk_Ctl_MM_Init_Raw_Memory( PIRP Irp )
{
	PIO_STACK_LOCATION			pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	ULONG						sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	PSET_MM_RAW_MEMORY			pMMRawMem			= Irp->AssociatedIrp.SystemBuffer;
	NTSTATUS					status				= STATUS_SUCCESS;
	ULONG						sizeOfmem;

	if (sizeOfBuffer <	sizeof(SET_MM_RAW_MEMORY))
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Init_Raw_Memory: sizeOfBuffer %d < sizeof(SET_MM_RAW_MEMORY) %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeof(SET_MM_RAW_MEMORY), status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	sizeOfmem = sizeof(SET_MM_RAW_MEMORY) + (pMMRawMem->NumberOfArray * sizeof(ULONG));

	if (sizeOfBuffer < sizeOfmem)
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Init_Raw_Memory: sizeOfBuffer %d < sizeOfmem Expected %d, NumberOfArray %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeOfmem, pMMRawMem->NumberOfArray, status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	status = mm_type_pages_add(	&GSftk_Config.Mmgr,
								pMMRawMem);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Init_Raw_Memory: mm_type_pages_add() Failed with status 0x%08x !!! \n",
										 status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	return status;
} // Sftk_Ctl_MM_Init_Raw_Memory()

//
//	Function:	Sftk_Ctl_MM_Stop()
//
//	Parameters: 
//		IN OUT	PIRP 		Gets Infromation in PIRP SystemBuffer
//
//	Description:
//		It has to free all MM memories. It does following activities in sequence
//		-	Go Thru each and every LG and change their status to TRACKING_MODE
//			This in turn free each and every BAB used memory back to MM
//		-	Now, No any memory from MM is in used.
//		-	Send MM Worker Thread Signal to Free All RAW memory back to the service thru Shared Memory Map IPC
//		-	Termainate MM Worker thread 
//		-	Free All Allocated and Locked MDL used in MM_ANCHOR
//		-	Make MM Disabled, since there is no any memory left in MM
//		-	return status back to caller.
//	
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
Sftk_Ctl_MM_Stop( PIRP Irp )
{
	PIO_STACK_LOCATION			pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS					status				= STATUS_SUCCESS;
	// ULONG						sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	// PSET_MM_DATABASE_MEMORY		pMMDb				= Irp->AssociatedIrp.SystemBuffer;

	status = mm_stop( &GSftk_Config.Mmgr);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Get_DB_Size: mm_stop() Failed with status 0x%08x !!! \n",
										 status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	return status;
} // Sftk_Ctl_MM_Stop()

//
//	Function:	mm_start()
//
//	Parameters: 
//		IN OUT PMM_DATABASE_SIZE_ENTRIES MMDbSizeEntries 		
//			It gets requires information from this IOCTL and 
//			It returns MM each and every types required information.
//		IN OUT PMM_MANAGER					Mmgr
//			Initialized Mmgr structures
//
//	Description:
//			It gets requires information from this IOCTL and 
//			It Initialized Kernel MM, Create Thread for SM, creante NAmed Event, etc.
//			It returns MM each and every types required information.
//	
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
mm_start(IN OUT PMM_DATABASE_SIZE_ENTRIES	MMDbSizeEntries,
		 IN OUT PMM_MANAGER					Mmgr)
{
	NTSTATUS	status	= STATUS_SUCCESS;
	ULONG		i;	
	
	if (Mmgr->MM_Initialized == TRUE)
	{ // Already Started
		status = STATUS_UNSUCCESSFUL;
		DebugPrint((DBG_ERROR, "mm_start: Mmgr->MM_Initialized %d == TRUE, returning error 0x%08x !!! \n",
										Mmgr->MM_Initialized, status));

		OS_ASSERT(Mmgr->MaxAllocatePhysMemInMB != 0);
		// Just fill information back to MMDbSizeEntries structures and return
		goto done; 
	} // Already Started

	if(MMDbSizeEntries->AWEUsed == FALSE)
	{ // If AWE is not used, We used Chunk based locking, 
		// max chunk size = (MMDbSizeEntries->VChunkSize / MMDbSizeEntries->PageSize) <= 32
		if ( (MMDbSizeEntries->VChunkSize % MMDbSizeEntries->PageSize) != 0)
		{ // MM : limitation validaty check
			status = STATUS_UNSUCCESSFUL;
			DebugPrint((DBG_ERROR, "FIXME FIXME :: mm_start: AWEUsed == FALSE, MMDbSizeEntries->VChunkSize %d Modulo MMDbSizeEntries->PageSize %d != 0, returning error 0x%08x !!! TODO :: \n",
										MMDbSizeEntries->VChunkSize, MMDbSizeEntries->PageSize, status));
			goto done;
		}
		
		/*
		if ( (MMDbSizeEntries->VChunkSize / MMDbSizeEntries->PageSize) > 32)
		{ // MM : limitation validaty check
			status = STATUS_UNSUCCESSFUL;
			DebugPrint((DBG_ERROR, "FIXME FIXME :: mm_start: AWEUsed == FALSE, MMDbSizeEntries->VChunkSize %d DIV MMDbSizeEntries->PageSize %d > 32, returning error 0x%08x !!! TODO :: \n",
										MMDbSizeEntries->VChunkSize, MMDbSizeEntries->PageSize, status));
			goto done;
		}
		*/
	}
	
	// Init each and every fields of Mmgr
	Mmgr->AWEUsed					= MMDbSizeEntries->AWEUsed;
	Mmgr->PageSize					= MMDbSizeEntries->PageSize;
	Mmgr->VChunkSize				= MMDbSizeEntries->VChunkSize;

	// OS_INITIALIZE_LOCK( &Mmgr->Lock, OS_ERESOURCE_LOCK, NULL);
	OS_INITIALIZE_LOCK( &Mmgr->Lock, OS_SPIN_LOCK, NULL);
	OS_INITIALIZE_LOCK( &Mmgr->ReservePoolLock, OS_SPIN_LOCK, NULL);

	Mmgr->MaxAllocatePhysMemInMB	= MMDbSizeEntries->MaxAllocatePhysMemInMB;
	Mmgr->TotalMemAllocated			= 0;
	Mmgr->IncrementAllocationChunkSizeInMB = MMDbSizeEntries->IncrementAllocationChunkSizeInMB;

	Mmgr->AllocThreshold			= MMDbSizeEntries->AllocThreshold;
	Mmgr->AllocIncrement			= MMDbSizeEntries->AllocIncrement;
	Mmgr->AllocThresholdTimeout		= MMDbSizeEntries->AllocThresholdTimeout;	
	Mmgr->AllocThresholdCount		= MMDbSizeEntries->AllocThresholdCount;
	Mmgr->TotalAllocHit				= 0;

	Mmgr->FreeThreshold				= MMDbSizeEntries->FreeThreshold;
	Mmgr->FreeIncrement				= MMDbSizeEntries->FreeIncrement;
	Mmgr->FreeThresholdTimeout		= MMDbSizeEntries->FreeThresholdTimeout;
	Mmgr->FreeThresholdCount		= MMDbSizeEntries->FreeThresholdCount;
	Mmgr->TotalFreeHit				= 0;

	// Statistics
	Mmgr->MM_TotalOSMemUsed					= 0;
	Mmgr->MM_OSMemUsed						= 0;
	Mmgr->MM_TotalNumOfMdlLocked			= 0;
	Mmgr->MM_TotalSizeOfMdlLocked			= 0;
	Mmgr->MM_TotalNumOfMdlLockedAtPresent	= 0;
	Mmgr->MM_TotalSizeOfMdlLockedAtPresent	= 0;

#if 0 // SM_IPC_SUPPORT	
	/*
	// Create SM_Thread
	status = SM_Thread_Create(Mmgr);
	if (!NT_SUCCESS(status)) 
    {
		DebugPrint((DBG_ERROR, "mm_start:: SM_Thread_Create(Mmgr 0x%08x) Failed with Error 0x%08x !\n", 
								Mmgr, status ));
        goto done;
    }
	*/
#endif

	// Initialize IOCTL IPC Communication fields
	KeInitializeEvent( &Mmgr->CmdEvent, SynchronizationEvent, FALSE);
	KeInitializeEvent( &Mmgr->EventFreeAllCompleted, SynchronizationEvent, FALSE);
	OS_INITIALIZE_LOCK( &Mmgr->CmdQueueLock, OS_SPIN_LOCK, NULL);
	ANCHOR_InitializeListHead( Mmgr->CmdList );
	Mmgr->FreeAllCmd = FALSE;
		
	OS_ZeroMemory( MMDbSizeEntries->MmDb, sizeof(MMDbSizeEntries->MmDb));

	// Now Initialized each and every MM_Type and its info.
	for (i=0; i < MM_TYPE_MAX; i++)
	{ // For : Init all slabs

		ANCHOR_InitializeListHead( Mmgr->MmSlab[i].FreeList );
		ANCHOR_InitializeListHead( Mmgr->MmSlab[i].UsedList );
		ANCHOR_InitializeListHead( Mmgr->MmSlab[i].MdlInfoList );

		OS_INITIALIZE_LOCK( &Mmgr->MmSlab[i].Lock, OS_SPIN_LOCK, NULL);

		Mmgr->MmSlab[i].Type						= (UCHAR) i;
		Mmgr->MmSlab[i].TotalMemSize				= 0;
		Mmgr->MmSlab[i].TotalNumberOfPagesInUse		= 0;
		Mmgr->MmSlab[i].NumOfNodestoKeep			= MM_DEFAULT_MAX_NODES_TO_ALLOCATE;
		Mmgr->MmSlab[i].TotalNumberOfNodes			= 0;
		Mmgr->MmSlab[i].TotalNumberOfRawWFreeNodes	= 0;
		Mmgr->MmSlab[i].NumOfMMEntryPerChunk		= 0;	
#if DBG
		Mmgr->MmSlab[i].UnUsedSize					= 0;		// Total size specifies Unused space in pages 
#endif
		// Initialize information in Out buffer with default first
		MMDbSizeEntries->MmDb[i].MMIndex				= (UCHAR) i;
		MMDbSizeEntries->MmDb[i].NodeSize				= 0;
		MMDbSizeEntries->MmDb[i].PageSizeRepresenting	= 0;
		MMDbSizeEntries->MmDb[i].RawMemory				= FALSE;

		switch(i)
		{
			case MM_TYPE_CHUNK_ENTRY:	Mmgr->MmSlab[i].NodeSize = sizeof(MM_CHUNK_ENTRY);
										Mmgr->MmSlab[i].NumOfMMEntryPerChunk= (Mmgr->VChunkSize / Mmgr->PageSize);

										if (Mmgr->AWEUsed == FALSE)
										{ // We do not need this type, since 4k Page from AWE directly available....
											MMDbSizeEntries->MmDb[i].NodeSize				= Mmgr->MmSlab[i].NodeSize;
											MMDbSizeEntries->MmDb[i].PageSizeRepresenting	= Mmgr->VChunkSize;
										}
										break;

			case MM_TYPE_MM_ENTRY	:	Mmgr->MmSlab[i].NodeSize = sizeof(MM_ENTRY);

										MMDbSizeEntries->MmDb[i].NodeSize				= Mmgr->MmSlab[i].NodeSize;
										MMDbSizeEntries->MmDb[i].PageSizeRepresenting	= Mmgr->PageSize;
										break;

			case MM_TYPE_MM_HOLDER:		Mmgr->MmSlab[i].NodeSize = sizeof(MM_HOLDER);

										MMDbSizeEntries->MmDb[i].NodeSize				= Mmgr->MmSlab[i].NodeSize;
										MMDbSizeEntries->MmDb[i].PageSizeRepresenting	= Mmgr->PageSize;
										break;

			case MM_TYPE_4K_NPAGED_MEM:	Mmgr->MmSlab[i].NodeSize = Mmgr->PageSize;
										if (Mmgr->AWEUsed == FALSE)
											Mmgr->MmSlab[i].NumOfMMEntryPerChunk= (Mmgr->VChunkSize / Mmgr->PageSize);
										else
											Mmgr->MmSlab[i].NumOfMMEntryPerChunk= 1;

										break;

			case MM_TYPE_SOFT_HEADER:	Mmgr->MmSlab[i].NodeSize = sizeof(MM_SOFT_HDR);

										MMDbSizeEntries->MmDb[i].NodeSize				= Mmgr->MmSlab[i].NodeSize;
										MMDbSizeEntries->MmDb[i].PageSizeRepresenting	= Mmgr->PageSize;
										break;

			case MM_TYPE_PROTOCOL_HEADER:	
										Mmgr->MmSlab[i].NodeSize = sizeof(MM_PROTO_HDR);

										MMDbSizeEntries->MmDb[i].NodeSize				= Mmgr->MmSlab[i].NodeSize;
										MMDbSizeEntries->MmDb[i].PageSizeRepresenting	= Mmgr->PageSize;
										break;

			default:					
										DebugPrint((DBG_ERROR, "FIXME FIXME :: mm_start: switch(MM_TYPE %d) invalid Type !! FIXME FIXME BUG !!\n",i));
										break;
		} // switch(i)
	} // for (i=0; i < MM_TYPE_MAX; i++)

	MMDbSizeEntries->NumberOfEntries = i;

	Mmgr->MM_Initialized = TRUE;	// success
	status = STATUS_SUCCESS;

done:
	return status;
} // mm_start()

//
//	Function:	mm_stop()
//
//	Parameters: 
//		IN OUT PMM_DATABASE_SIZE_ENTRIES MMDbSizeEntries 		
//			It gets requires information from this IOCTL and 
//			It returns MM each and every types required information.
//		IN OUT PMM_MANAGER					Mmgr
//			Initialized Mmgr structures
//
//	Description:
//		It has to free all MM memories. It does following activities in sequence
//		-	Go Thru each and every LG and change their status to TRACKING_MODE
//			This in turn free each and every BAB used memory back to MM
//		-	Now, No any memory from MM is in used.
//		-	Send MM Worker Thread Signal to Free All RAW memory back to the service thru Shared Memory Map IPC
//		-	Termainate MM Worker thread 
//		-	Free All Allocated and Locked MDL used in MM_ANCHOR
//		-	Make MM Disabled, since there is no any memory left in MM
//		-	return status back to caller.
//	
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
mm_stop( IN OUT PMM_MANAGER		Mmgr)
{
	NTSTATUS		status		= STATUS_SUCCESS;
	PSFTK_LG		pLg			= NULL;
	PLIST_ENTRY		plistEntry	= NULL;
	LONG			PrevState;
	LARGE_INTEGER	waitTimeout;
	PMM_CMD_PACKET	pMmCmdPkt;
	LONG			usedEventsCount = 0;	// always use index 0 atleast for termination and change event signalled.
	PVOID			arrayOfEvents[THREAD_WAIT_OBJECTS];	// MAXIMUM_WAIT_OBJECTS = 64

	if (Mmgr->MM_Initialized == FALSE)
	{ // Already stopped
		status = STATUS_SUCCESS;
		DebugPrint((DBG_ERROR, "mm_stop: Mmgr->MM_Initialized %d == FALSE, returning success 0x%08x !!! \n",
										Mmgr->MM_Initialized, status));
		goto done; 
	} // Already Started

	Mmgr->MM_UnInitializedInProgress = TRUE;

	waitTimeout.QuadPart = DEFAULT_TIMEOUT_FOR_LG_TO_FREE_MEM_TO_MM_WAIT_100NS;

	//		-	Go Thru each and every LG and change their status to TRACKING_MODE
	//			This in turn free each and every BAB used memory back to MM
	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
	for( plistEntry = GSftk_Config.Lg_GroupList.ListEntry.Flink;
		 plistEntry != &GSftk_Config.Lg_GroupList.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every logical group list 
		pLg = CONTAINING_RECORD( plistEntry, SFTK_LG, Lg_GroupLink);

		PrevState = sftk_lg_get_state(pLg);

		if (PrevState != SFTK_MODE_PASSTHRU)
		{
			DebugPrint((DBG_ERROR, "mm_stop: Changing Lg Num %d, Old State 0x%08x to new state 0x%08x Tracking mode !\n",
										pLg->LGroupNumber, pLg->state, SFTK_MODE_TRACKING));

			if (sftk_lg_get_state(pLg) == SFTK_MODE_FULL_REFRESH)
				sftk_lg_change_State( pLg, pLg->state, SFTK_MODE_PASSTHRU, FALSE);
			else
			{
				pLg->DoNotSendOutBandCommandForTracking = TRUE; // so not need to send Tracking outband FTDCHUP command
				sftk_lg_change_State( pLg, pLg->state, SFTK_MODE_TRACKING, FALSE);
			}

			if ( (PrevState == SFTK_MODE_SMART_REFRESH) || (PrevState == SFTK_MODE_NORMAL) || (PrevState == SFTK_MODE_FULL_REFRESH))
			{ // BAB used, Requires to free memory and wait for it
				usedEventsCount = 0;	// always use index 0 atleast for termination and change event signalled.

				arrayOfEvents[usedEventsCount] = &pLg->Event_LGFreeAllMemOfMM; 
				usedEventsCount ++;

				if ( (PrevState == SFTK_MODE_SMART_REFRESH) || (PrevState == SFTK_MODE_FULL_REFRESH) )
				{
					if (pLg->ReleaseIsWaiting == TRUE)
					{ // if Release pool pending then only go for wait...
						arrayOfEvents[usedEventsCount] = &pLg->ReleasePoolDoneEvent; 
						usedEventsCount ++;
					}
				}
				else
				{
					if (PrevState == SFTK_MODE_NORMAL)
					{
						// arrayOfEvents[usedEventsCount] = &pLg->ReleasePoolDoneEvent; 
						// usedEventsCount ++;
					}

				}

				do 
				{
					waitTimeout.QuadPart = DEFAULT_TIMEOUT_FOR_LG_TO_FREE_MEM_TO_MM_WAIT_100NS;
					status = KeWaitForMultipleObjects(	usedEventsCount,
														arrayOfEvents,
														WaitAll,
														Executive,
														KernelMode,
														FALSE,
														&waitTimeout,
														NULL );
					switch(status)
					{
						// case STATUS_SUCCESS:// A time out occurred before the specified set of wait conditions was met. 
						case STATUS_WAIT_0:	// Index 0 got signaled for Sftk_Lg->Event_LGFreeAllMemOfMM
						case STATUS_WAIT_1:		// Index 1 got signaled for Sftk_Lg->ReleasePoolDoneEvent
						case STATUS_WAIT_2:		// Index 1 got signaled for Sftk_Lg->ReleasePoolDoneEvent
										DebugPrint((DBG_ERROR, "mm_stop: Lg %d KeWaitForMultipleObjects(Count %d, All Satisfied, Timeout 0x%I64x)  returned 0x%08x !!\n", 
															pLg->LGroupNumber, usedEventsCount, waitTimeout.QuadPart, status ));
										status = STATUS_SUCCESS;
										break;	
						case STATUS_TIMEOUT:// A time out occurred before the specified set of wait conditions was met. 
										DebugPrint((DBG_ERROR, "mm_stop: Lg %d KeWaitForMultipleObjects(Count %d, Timeout occurred 0x%I64x)  returned 0x%08x !!\n", 
															pLg->LGroupNumber, usedEventsCount, waitTimeout.QuadPart, status ));
										break;
						default:
										DebugPrint((DBG_ERROR, "mm_stop: Lg %d KeWaitForMultipleObjects(Count %d, default 0x%I64x)  returned 0x%08x !!\n", 
															pLg->LGroupNumber, usedEventsCount, waitTimeout.QuadPart, status ));
										OS_ASSERT(FALSE);
										break;
					} // switch(status)
				}while(status != STATUS_SUCCESS);

				waitTimeout.QuadPart = DEFAULT_TIMEOUT_FOR_LG_TO_FREE_MEM_TO_MM_WAIT_100NS;

				while(  (pLg->QueueMgr.PendingList.NumOfNodes > 0)	||
						(pLg->QueueMgr.CommitList.NumOfNodes > 0)	||
						(pLg->QueueMgr.RefreshList.NumOfNodes > 0)	||
						(pLg->QueueMgr.RefreshPendingList.NumOfNodes > 0)	||
						(pLg->QueueMgr.MigrateList.NumOfNodes > 0) )
				{
					DebugPrint((DBG_ERROR, "mm_stop: Lg %d, While(QM Pkts): PendingList:%d,CommitList:%d,RefreshList:%d,MigrateList:%d, going for sleep %I64x !!\n", 
															pLg->LGroupNumber, pLg->QueueMgr.PendingList.NumOfNodes,
															pLg->QueueMgr.CommitList.NumOfNodes, 
															pLg->QueueMgr.RefreshList.NumOfNodes,
															pLg->QueueMgr.MigrateList.NumOfNodes,
															waitTimeout.QuadPart));
					// Sleep for a while and thech again....this only we can do...
					KeDelayExecutionThread( KernelMode, FALSE, &waitTimeout);
				}
			} // BAB used, Requires to free memory and wait for it
		} // if (PrevState != SFTK_MODE_PASSTHRU)

		OS_ASSERT( pLg->QueueMgr.CommitList.NumOfNodes == 0);
		OS_ASSERT( IsListEmpty(&pLg->QueueMgr.CommitList.ListEntry) == TRUE);

		OS_ASSERT( pLg->QueueMgr.PendingList.NumOfNodes == 0);
		OS_ASSERT( IsListEmpty(&pLg->QueueMgr.PendingList.ListEntry) == TRUE);

		OS_ASSERT( pLg->QueueMgr.RefreshList.NumOfNodes == 0);
		OS_ASSERT( IsListEmpty(&pLg->QueueMgr.RefreshList.ListEntry) == TRUE);

		OS_ASSERT( pLg->QueueMgr.MigrateList.NumOfNodes == 0);
		OS_ASSERT( IsListEmpty(&pLg->QueueMgr.MigrateList.ListEntry) == TRUE);
	} // for : scan thru each and every logical group list 
	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);

#if 0 // SM_IPC_SUPPORT
	/*
	waitTimeout.QuadPart = DEFAULT_TIMEOUT_FREEALL_RAW_MEM_CMD_WAIT_100NS;
	//		-	Now, No any memory from MM is in used.
	//		-	Send MM Worker Thread Signal to Free All RAW memory back to the service thru Shared Memory Map IPC
	KeClearEvent( &Mmgr->SM_EventCmdExecuted );
	Mmgr->SM_EventCmd = SM_Event_FreeAll;
	KeSetEvent( &Mmgr->SM_Event, 0, FALSE);
	status = KeWaitForSingleObject(	&Mmgr->SM_EventCmdExecuted, 
									Executive,
									KernelMode,
									FALSE,
									&waitTimeout );
	switch(status)
	{
		case STATUS_SUCCESS:// A time out occurred before the specified set of wait conditions was met. 
						DebugPrint((DBG_ERROR, "mm_stop: KeWaitForSingleObject(FreeAll : SM_EventCmdExecuted)  returned STATUS_SUCCESS 0x%08x !!\n", 
											status ));
						break;	
		case STATUS_TIMEOUT:// A time out occurred before the specified set of wait conditions was met. 
						DebugPrint((DBG_ERROR, "mm_stop: KeWaitForSingleObject(FreeAll : SM_EventCmdExecuted, Timeout 0x%I64x)  returned STATUS_TIMEOUT 0x%08x !!\n", 
											waitTimeout.QuadPart, status ));
						OS_ASSERT(FALSE);
						break;
		default:
						DebugPrint((DBG_ERROR, "mm_stop: KeWaitForSingleObject(FreeAll : SM_EventCmdExecuted, Timeout 0x%I64x)  returned error 0x%08x !!\n", 
											waitTimeout.QuadPart, status ));
						OS_ASSERT(FALSE);
						break;
	}
	*/
#endif
	// Used IOCTL IPC mechanism to communicates between Kernel and Service for MM

	// First make Wotk pkts Queue list empty since we are terminating
	Mmgr->FreeAllCmd = TRUE;

	OS_ACQUIRE_LOCK( &Mmgr->CmdQueueLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	while( !(IsListEmpty(&Mmgr->CmdList.ListEntry)) )
	{ // While : Unlock & Free All pkts
		plistEntry = RemoveHeadList(&Mmgr->CmdList.ListEntry);
		Mmgr->CmdList.NumOfNodes --;
		pMmCmdPkt = CONTAINING_RECORD( plistEntry, MM_CMD_PACKET, cmdLink);
		OS_FreeMemory(pMmCmdPkt);
	}
	OS_RELEASE_LOCK( &Mmgr->CmdQueueLock, NULL);

	KeSetEvent( &Mmgr->CmdEvent, 0, FALSE);
	status = KeWaitForSingleObject(	&Mmgr->EventFreeAllCompleted, 
									Executive,
									KernelMode,
									FALSE,
									NULL); // &waitTimeout
	switch(status)
	{
		case STATUS_SUCCESS:// A time out occurred before the specified set of wait conditions was met. 
						DebugPrint((DBG_ERROR, "mm_stop: KeWaitForSingleObject(Event_LGFreeAllMemOfMM)  returned STATUS_SUCCESS 0x%08x !!\n", 
											status ));
						break;	
		case STATUS_TIMEOUT:// A time out occurred before the specified set of wait conditions was met. 
						DebugPrint((DBG_ERROR, "mm_stop: KeWaitForSingleObject(Event_LGFreeAllMemOfMM, Timeout 0x%I64x)  returned STATUS_TIMEOUT 0x%08x !!\n", 
											waitTimeout.QuadPart, status ));
						break;
		default:
						DebugPrint((DBG_ERROR, "mm_stop: KeWaitForSingleObject(Event_LGFreeAllMemOfMM, Timeout 0x%I64x)  returned error 0x%08x !!\n", 
											waitTimeout.QuadPart, status ));
						break;
	}

	//		-	Termainate MM SM Worker thread 
	// Not need to terminate SM Worker thread, since its ok, it won't do any work

	//		-	Free All Allocated and Locked MDL used in MM_ANCHOR
	status = mm_type_alloc_deinit_all( Mmgr );
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "mm_stop: mm_type_alloc_deinit_all() Failed with status 0x%08x !!! \n",
										 status));
		OS_ASSERT(FALSE);
		// return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	//		-	Make MM Disabled, since there is no any memory left in MM
	//		-	return status back to caller.
	status = mm_deinit( Mmgr, FALSE );
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "mm_stop: mm_deinit() Failed with status 0x%08x !!! \n",
										 status));
		OS_ASSERT(FALSE);
		// return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	Mmgr->MM_UnInitializedInProgress = FALSE;
done:
	return status;
} // mm_stop()

//
//	Function:	mm_deinit()
//
//	Parameters: 
//	Description:
//			It Deinit all MM and if specify it terminates SM thread for MM
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
mm_deinit(IN OUT PMM_MANAGER	Mmgr, IN BOOLEAN TerminateSMThread)
{
	NTSTATUS	status	= STATUS_SUCCESS;
	ULONG		i;	
	
	if (Mmgr->MM_Initialized == FALSE)
	{ // Already Started
		DebugPrint((DBG_ERROR, "mm_deinit: Mmgr->MM_Initialized %d == FALSE, returning success 0x%08x !!! \n",
										Mmgr->MM_Initialized, status));

		// Just fill information back to MMDbSizeEntries structures and return
		goto done; 
	} // Already Started

	Mmgr->MM_Initialized = FALSE;
	// Init each and every fields of Mmgr
	Mmgr->AWEUsed					= FALSE;
	Mmgr->PageSize					= 0;
	Mmgr->VChunkSize				= 0;

	OS_DEINITIALIZE_LOCK( &Mmgr->Lock, NULL);
	OS_DEINITIALIZE_LOCK( &Mmgr->ReservePoolLock, NULL);

	Mmgr->MaxAllocatePhysMemInMB	= 0;
	Mmgr->TotalMemAllocated			= 0;
	Mmgr->IncrementAllocationChunkSizeInMB = 0;

	Mmgr->AllocThreshold			= 0;
	Mmgr->AllocIncrement			= 0;
	Mmgr->AllocThresholdTimeout		= 0;
	Mmgr->AllocThresholdCount		= 0;
	Mmgr->TotalAllocHit				= 0;

	Mmgr->FreeThreshold				= 0;
	Mmgr->FreeIncrement				= 0;
	Mmgr->FreeThresholdTimeout		= 0;
	Mmgr->FreeThresholdCount		= 0;
	Mmgr->TotalFreeHit				= 0;

	if (TerminateSMThread == TRUE)
	{ // Remove SM_Thread	 
#if 0 // SM_IPC_SUPPORT
		status = SM_Thread_Terminate( Mmgr );
		if (!NT_SUCCESS(status)) 
		{
			DebugPrint((DBG_ERROR, "mm_start:: SM_Thread_Create(Mmgr 0x%08x) Failed with Error 0x%08x !\n", 
									Mmgr, status ));
			OS_ASSERT(FALSE);
		}
#endif
	}
	// Now Initialized each and every MM_Type and its info.
	for (i=0; i < MM_TYPE_MAX; i++)
	{ // For : Init all slabs

		ANCHOR_InitializeListHead( Mmgr->MmSlab[i].FreeList );
		ANCHOR_InitializeListHead( Mmgr->MmSlab[i].UsedList );
		ANCHOR_InitializeListHead( Mmgr->MmSlab[i].MdlInfoList );

		OS_DEINITIALIZE_LOCK( &Mmgr->MmSlab[i].Lock, NULL);
	} // for (i=0; i < MM_TYPE_MAX; i++)

done:
	return STATUS_SUCCESS;
} // mm_deinit()

//
//	Function:	mm_type_alloc_init()
//
//	Parameters: 
//		IN OUT PMM_MANAGER					Mmgr
//			Initialized Mmgr structures
//		IN		PSET_MM_DATABASE_MEMORY		MMDb
//			It gets Allocated memory for specified MM_TYPE
//
//	Description:
//			It gets requires information from this IOCTL and 
//			It Initialized/Allocate specified MM_TYPE slab, 
//			It lockes down memory and mapped it to use in kernel
//	
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
mm_type_alloc_init(	IN	OUT PMM_MANAGER					Mmgr,
					IN		PSET_MM_DATABASE_MEMORY		MMDb)
{
	NTSTATUS				status		= STATUS_SUCCESS;
	PMM_ANCHOR				pMM_anchor	= &Mmgr->MmSlab[MMDb->MMIndex];
	PMM_ANCHOR_MDL_INFO		pMM_mdlInfo;
	ULONG					i, j;	
	ULONG					sizeOfMdlInfo, numOfnodes;
	PUCHAR					pMemory;
	PMM_CHUNK_ENTRY			pMmChunkEntry;
	PMM_ENTRY				pMmEntry;
	PMM_HOLDER				pMmHolder;
	PMM_SOFT_HDR			pMmSoftHdr;
	PMM_PROTO_HDR			pMmProtoHdr;
	PLIST_ENTRY				pListEntry;

	i = j = 0;

	if (Mmgr->MM_Initialized == FALSE)
	{ // MMMgr not started 
		status = STATUS_UNSUCCESSFUL;
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_alloc_init: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										Mmgr->MM_Initialized, status));
		return status;
	} // MMMgr not started 

	if ( !(	(pMM_anchor->Type == MM_TYPE_MM_ENTRY)			|| 
			(pMM_anchor->Type == MM_TYPE_MM_HOLDER)			||
			(pMM_anchor->Type == MM_TYPE_CHUNK_ENTRY)		||
			(pMM_anchor->Type == MM_TYPE_SOFT_HEADER)		||
			(pMM_anchor->Type == MM_TYPE_PROTOCOL_HEADER)	))
	{
		status = STATUS_INVALID_PARAMETER;
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_alloc_init: pMM_acnhor->Type %d is invalid with this IOCTL !!! returning error 0x%08x !!! \n",
										pMM_anchor->Type, status));
		return status;
	}

	// Allocate memory for MDL information to hold in Anchor
	sizeOfMdlInfo	= SIZE_OF_MM_ANCHOR_MDL_INFO(MMDb->NumberOfArray);
	pMM_mdlInfo		= OS_AllocMemory(NonPagedPool, sizeOfMdlInfo);
	if (pMM_mdlInfo == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_alloc_init: For PMM_ANCHOR_MDL_INFO : OS_AllocMemory(size %d) Failed!!! returning error 0x%08x !!! \n",
										sizeOfMdlInfo, status));
		return status;
	}
	OS_ZeroMemory( pMM_mdlInfo, sizeOfMdlInfo);

	InitializeListHead( &pMM_mdlInfo->MdlInfoLink );
	pMM_mdlInfo->ChunkSize		= MMDb->ChunkSize;
	pMM_mdlInfo->NumberOfArray	= MMDb->NumberOfArray;
	pMM_mdlInfo->TotalMemorySize= MMDb->TotalMemorySize;
	
	for (i=0; i < MMDb->NumberOfArray; i++)
	{ // for : Locked down each and and every user supplied Vaddr using MDL into kernel
		// Create MDL and Locked Memory in this MDl
		// .
		status = mm_locked_mdlInfo(	&MMDb->VMem[i],
									&pMM_mdlInfo->ArrayMdl[i] );
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "mm_type_alloc_init: mm_locked_mdlInfo() Failed with status 0x%08x !!! \n",
										 status));
			goto done;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
		}
	} // for : Locked down each and and every user supplied Vaddr using MDL into kernel

	// Increment total memory allocation in Mmgr
	OS_ACQUIRE_LOCK( &Mmgr->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	Mmgr->TotalMemAllocated	+= MMDb->TotalMemorySize;
	OS_RELEASE_LOCK( &Mmgr->Lock, NULL);

	// Add Mdl Info into MMAnchor's Anchor list
	OS_ACQUIRE_LOCK( &pMM_anchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	InsertTailList( &pMM_anchor->MdlInfoList.ListEntry, &pMM_mdlInfo->MdlInfoLink );
	pMM_anchor->MdlInfoList.NumOfNodes ++;
	
	for (i=0; i < MMDb->NumberOfArray; i++)
	{ // for : scan thru Mdl kernel space memory and mapped it to fixed size node, add into FreeList
		// Now we got kernel memory, so now make nodes from this memory and add it into freeList
		pMemory	= (PUCHAR) pMM_mdlInfo->ArrayMdl[i].SystemVAddr;
		OS_ZeroMemory( pMemory, pMM_mdlInfo->ArrayMdl[i].Size);

		numOfnodes = pMM_mdlInfo->ArrayMdl[i].Size / pMM_anchor->NodeSize;

		for (j=0; j < numOfnodes ; j++)
		{ // For : extract nodes from allocated memory and add it into Freelist
			if ( (ULONG) pMemory >= (ULONG) ((ULONG) pMM_mdlInfo->ArrayMdl[i].SystemVAddr + pMM_mdlInfo->ArrayMdl[i].Size) )
			{
				DebugPrint((DBG_ERROR, "BUG FIXME FIXME :: pMemory 0x%08x >= 0x%08x ((ULONG) pMM_mdlInfo->ArrayMdl[i].SystemVAddr 0x%08x + pMM_mdlInfo->ArrayMdl[i].Size %d) !!! \n",
										 pMemory, ((ULONG) pMM_mdlInfo->ArrayMdl[i].SystemVAddr + pMM_mdlInfo->ArrayMdl[i].Size),
										 pMM_mdlInfo->ArrayMdl[i].SystemVAddr,
										 pMM_mdlInfo->ArrayMdl[i].Size));
				break;
			}

			switch(pMM_anchor->Type)
			{
				case MM_TYPE_CHUNK_ENTRY:	

							pMmChunkEntry = (PMM_CHUNK_ENTRY) pMemory;

							InitializeListHead( &pMmChunkEntry->MmChunkLink );
							ANCHOR_InitializeListHead( pMmChunkEntry->MmEntryList );
							//pMmChunkEntry->FreeMap	= 0;
							pMmChunkEntry->Flag = 0;
							
							pListEntry = &pMmChunkEntry->MmChunkLink;
							break;

				case MM_TYPE_MM_ENTRY	:	
							pMmEntry = (PMM_ENTRY) pMemory;

							InitializeListHead( &pMmEntry->MmEntryLink );
							pMmEntry->PageEntry		= 0;
							pMmEntry->ChunkEntry	= NULL;
			
							pListEntry = &pMmEntry->MmEntryLink;
							break;

				case MM_TYPE_MM_HOLDER:		
							pMmHolder = (PMM_HOLDER) pMemory;

							InitializeListHead( &pMmHolder->MmHolderLink );
							ANCHOR_InitializeListHead( pMmHolder->MmEntryList );
							pMmHolder->FlagLink		= MM_HOLDER_FLAG_LINK_FREE_LIST;
							pMmHolder->Mdl			= NULL;
							pMmHolder->Size			= 0;
							pMmHolder->SystemVAddr	= NULL;
							
							pListEntry = &pMmHolder->MmHolderLink;
							break;

				case MM_TYPE_SOFT_HEADER:		
							pMmSoftHdr = (PMM_SOFT_HDR) pMemory;

							InitializeListHead( &pMmSoftHdr->MmSoftHdrLink );
							pMmSoftHdr->Flag	= 0;
							OS_SetFlag( pMmSoftHdr->Flag, MM_FLAG_SOFT_HDR);
							OS_ZeroMemory( &pMmSoftHdr->Hdr, sizeof(pMmSoftHdr->Hdr));

							pListEntry = &pMmSoftHdr->MmSoftHdrLink;
							break;

				case MM_TYPE_PROTOCOL_HEADER:		
							pMmProtoHdr = (PMM_PROTO_HDR) pMemory;

							InitializeListHead( &pMmProtoHdr->MmProtoHdrLink );
							pMmProtoHdr->Flag	= 0;
							OS_SetFlag( pMmProtoHdr->Flag, MM_FLAG_PROTO_HDR);
							OS_ZeroMemory( &pMmProtoHdr->Hdr, sizeof(pMmSoftHdr->Hdr));
							ANCHOR_InitializeListHead( pMmProtoHdr->MmSoftHdrList );
						
							pListEntry = &pMmProtoHdr->MmProtoHdrLink;
							break;

				case MM_TYPE_4K_NPAGED_MEM:	
				default:
							DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_alloc_init: inside Switch() pMM_acnhor->Type %d is invalid with this IOCTL !!! FIXME FIXME !!! \n",
										pMM_anchor->Type, status));
							OS_ASSERT(FALSE);
							break;
			} // switch(pMM_anchor->Type)
			
			// insert this node into free list anchor
			InsertTailList( &pMM_anchor->FreeList.ListEntry, pListEntry );
			pMM_anchor->FreeList.NumOfNodes ++;

			pMM_anchor->TotalNumberOfNodes ++;
			// increment Memory
			pMemory = (PUCHAR) ((ULONG) pMemory + pMM_anchor->NodeSize);	
			
		} // For : extract nodes from allocated memory and add it into Freelist

		// increments total memory size used for this MM_TYPE
		pMM_anchor->TotalMemSize += pMM_mdlInfo->ArrayMdl[i].Size;

	} // for : scan thru Mdl kernel space memory and mapped it to fixed size node, add into FreeList

	OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);
	
	status = STATUS_SUCCESS;

done:

	if (!NT_SUCCESS(status))
	{ // Failed, Do cleanup here
		if (pMM_mdlInfo)
		{
			for (j=0; j < i; j++)
			{
				mm_unlocked_mdlInfo( &pMM_mdlInfo->ArrayMdl[j] );
			}
			OS_FreeMemory(pMM_mdlInfo);
		}
	}

	return status;
} // mm_type_alloc_init()

//
//	Function:	mm_type_alloc_deinit_all()
//
//	Parameters: 
//		IN OUT PMM_MANAGER					Mmgr
//			Initialized Mmgr structures
//		IN		PSET_MM_DATABASE_MEMORY		MMDb
//			It gets Allocated memory for specified MM_TYPE
//
//	Description:
//			It Release All memory allocated in all MM_TYPE except Raw 4K Page memory 
//			which must be released before calling this API
//			It DeInitialized/Free specified MM_TYPE slab, 
//			It unlock memory and unmapped it from kernel.
//	
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
mm_type_alloc_deinit_all(	IN	OUT PMM_MANAGER	Mmgr)
{
	NTSTATUS				status		= STATUS_SUCCESS;
	PMM_ANCHOR				pMM_anchor;
	PMM_ANCHOR_MDL_INFO		pMM_mdlInfo;
	ULONG					i, j;	
	PLIST_ENTRY				pListEntry;

	if (Mmgr->MM_Initialized == FALSE)
	{ // MMMgr not started 
		status = STATUS_UNSUCCESSFUL;
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_alloc_deinit_all: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										Mmgr->MM_Initialized, status));
		return status;
	} // MMMgr not started 

	OS_ACQUIRE_LOCK( &Mmgr->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	for (i=0; i < MM_TYPE_MAX; i++)
	{ // for : deinit each and every MM_TYPE
		pMM_anchor = &Mmgr->MmSlab[i];

		if (pMM_anchor->Type == MM_TYPE_4K_NPAGED_MEM)
		{ // Non Paged Raw memory 
			// Nothing to free anything
			OS_ASSERT(pMM_anchor->FreeList.NumOfNodes == 0);
			OS_ASSERT(pMM_anchor->TotalMemSize == 0);
			OS_ASSERT(pMM_anchor->TotalNumberOfNodes == 0);
			continue;
		}

		OS_ACQUIRE_LOCK( &pMM_anchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

		// First free all nodes
		ANCHOR_InitializeListHead( pMM_anchor->FreeList );
		ANCHOR_InitializeListHead( pMM_anchor->UsedList );
		pMM_anchor->Flag				= 0;
		pMM_anchor->TotalMemSize		= 0;
		pMM_anchor->TotalNumberOfNodes	= 0;
		
		while( !(IsListEmpty(&pMM_anchor->MdlInfoList.ListEntry)) )
		{ // While : Unlock & Free All mdl 
			pListEntry = RemoveHeadList(&pMM_anchor->MdlInfoList.ListEntry);
			pMM_anchor->MdlInfoList.NumOfNodes --;	// Decrement Counter.

			pMM_mdlInfo = CONTAINING_RECORD( pListEntry, MM_ANCHOR_MDL_INFO, MdlInfoLink);

			for (j=0; j < pMM_mdlInfo->NumberOfArray; j++)
			{ // for : UnLocked and free MDL 
				status = mm_unlocked_mdlInfo(	&pMM_mdlInfo->ArrayMdl[j] );
				if (!NT_SUCCESS(status))
				{
					DebugPrint((DBG_ERROR, "mm_type_alloc_deinit_all: mm_unlocked_mdlInfo() Failed with status 0x%08x !!! \n",
												 status));
				}
			} // for : UnLocked and free MDL 

			OS_FreeMemory( pMM_mdlInfo );
		} // While : Unlock & Free All mdl 

		ANCHOR_InitializeListHead( pMM_anchor->MdlInfoList );

		OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);
	} // for : deinit each and every MM_TYPE

	Mmgr->TotalMemAllocated	= 0;

	OS_RELEASE_LOCK( &Mmgr->Lock, NULL);
	
	status = STATUS_SUCCESS;

	return status;
} // mm_type_alloc_deinit_all()

PVOID
mm_type_alloc( UCHAR	MM_Type)
{
	PMM_ANCHOR				pMM_anchor	= &GSftk_Config.Mmgr.MmSlab[MM_Type];
	PVOID					pMemory		= NULL;
	PMM_CHUNK_ENTRY			pMmChunkEntry;
	PMM_ENTRY				pMmEntry;
	PMM_HOLDER				pMmHolder;
	PMM_SOFT_HDR			pMmSoftHdr;
	PMM_PROTO_HDR			pMmProtoHdr;
	PLIST_ENTRY				pListEntry;

	if (GSftk_Config.Mmgr.MM_Initialized == FALSE)
	{ // MMMgr not started 
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_alloc: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										GSftk_Config.Mmgr.MM_Initialized, NULL));
		return NULL;
	} // MMMgr not started 

	OS_ACQUIRE_LOCK( &pMM_anchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	if ( IsListEmpty( &pMM_anchor->FreeList.ListEntry) ) 
	{ // No Free node presence !!
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_alloc: MM_Type %d does not have and free node, Number of Free nodes  %d!!! \n",
										MM_Type, pMM_anchor->FreeList.NumOfNodes));
		// Allocate From OS
		switch(MM_Type)
		{
			case MM_TYPE_SOFT_HEADER:
				// Allocate Memory from OS
					OS_ASSERT( pMM_anchor->NodeSize == sizeof(MM_SOFT_HDR));

					pMemory = pMmSoftHdr = OS_AllocMemory(NonPagedPool, pMM_anchor->NodeSize);
					if (pMmSoftHdr == NULL)
					{ // Failed :should not happened
						DebugPrint((DBG_ERROR, "mm_type_alloc: OS_AllocMemory(MM_TYPE_SOFT_HEADER size %d) Failed !! returning error!! \n",
														pMM_anchor->NodeSize));
						goto done;
					}
					InitializeListHead( &pMmSoftHdr->MmSoftHdrLink );
					pMmSoftHdr->Flag	= 0;
					OS_SetFlag( pMmSoftHdr->Flag, MM_FLAG_SOFT_HDR);
					OS_ZeroMemory( &pMmSoftHdr->Hdr, sizeof(pMmSoftHdr->Hdr));
					OS_SetFlag( pMmSoftHdr->Flag, MM_FLAG_ALLOCATED_FROM_OS);

					// Lets Probe and Lock the wlheader_t this will be used in transfering the data
					// through TDI. This is NonPaged Buffer so Locking no Problem
					pMmSoftHdr->Mdl = TDI_AllocateAndProbeMdl(&pMmSoftHdr->Hdr,sizeof(pMmSoftHdr->Hdr),FALSE,FALSE,NULL);
					pMmSoftHdr->Size = sizeof(pMmSoftHdr->Hdr);

					GSftk_Config.Mmgr.MM_TotalOSMemUsed += pMM_anchor->NodeSize;
					GSftk_Config.Mmgr.MM_OSMemUsed		+= pMM_anchor->NodeSize;

					break;

			case MM_TYPE_PROTOCOL_HEADER:
					// Allocate Memory from OS
					pMemory = pMmProtoHdr = OS_AllocMemory(NonPagedPool, pMM_anchor->NodeSize);
					if (pMmProtoHdr == NULL)
					{ // Failed :should not happened
						DebugPrint((DBG_ERROR, "mm_type_alloc: OS_AllocMemory(MM_TYPE_PROTOCOL_HEADER size %d) Failed !! returning error!! \n",
														pMM_anchor->NodeSize));
						goto done;
					}
					InitializeListHead( &pMmProtoHdr->MmProtoHdrLink );
					pMmProtoHdr->Flag		= 0;
					pMmProtoHdr->RawDataSize= 0;
					pMmProtoHdr->Event		= NULL;
					pMmProtoHdr->SftkDev	= NULL;	
					pMmProtoHdr->RetProtoHDr= NULL;
					pMmProtoHdr->RetBuffer	= NULL;
					pMmProtoHdr->RetBufferSize= 0;
					pMmProtoHdr->Status		= STATUS_SUCCESS;

					OS_SetFlag( pMmProtoHdr->Flag, MM_FLAG_PROTO_HDR);
					OS_ZeroMemory( &pMmProtoHdr->Hdr, sizeof(pMmSoftHdr->Hdr));
					ANCHOR_InitializeListHead( pMmProtoHdr->MmSoftHdrList );
					OS_SetFlag( pMmProtoHdr->Flag, MM_FLAG_ALLOCATED_FROM_OS);

					// Lets Probe and Lock the ftd_header_t this will be used in transfering the data
					// through TDI. This is NonPaged Buffer so Locking no Problem
					pMmProtoHdr->Mdl = TDI_AllocateAndProbeMdl(&pMmProtoHdr->Hdr,sizeof(pMmProtoHdr->Hdr),FALSE,FALSE,NULL);
					pMmProtoHdr->Size = sizeof(pMmProtoHdr->Hdr);

					GSftk_Config.Mmgr.MM_TotalOSMemUsed += pMM_anchor->NodeSize;
					GSftk_Config.Mmgr.MM_OSMemUsed		+= pMM_anchor->NodeSize;

					break;
		} // switch(MM_Type)
		goto done;
	}

	pListEntry = RemoveHeadList( &pMM_anchor->FreeList.ListEntry );
	pMM_anchor->FreeList.NumOfNodes --;	// Decrement Counter.

	switch(MM_Type)
	{
		case MM_TYPE_CHUNK_ENTRY:	pMemory = pMmChunkEntry = CONTAINING_RECORD( pListEntry, MM_CHUNK_ENTRY, MmChunkLink); 
									InitializeListHead( &pMmChunkEntry->MmChunkLink );
									ANCHOR_InitializeListHead( pMmChunkEntry->MmEntryList );
									//pMmChunkEntry->FreeMap	= 0;
									pMmChunkEntry->Flag		= 0;
									break;


		case MM_TYPE_MM_ENTRY:		pMemory = pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink); 
									InitializeListHead( &pMmEntry->MmEntryLink );
									pMmEntry->PageEntry		= NULL;
									pMmEntry->ChunkEntry	= NULL;
									break;

		case MM_TYPE_MM_HOLDER:		pMemory = pMmHolder= CONTAINING_RECORD( pListEntry, MM_HOLDER, MmHolderLink); 
									InitializeListHead( &pMmHolder->IrpContextList );
									InitializeListHead( &pMmHolder->MmHolderLink );
									pMmHolder->FlagLink		= 0;
									ANCHOR_InitializeListHead( pMmHolder->MmEntryList );
									pMmHolder->Mdl			= NULL;
									pMmHolder->Size			= 0;
									pMmHolder->SystemVAddr	= NULL;
									pMmHolder->Proto_type	= 0;
									pMmHolder->pProtocolHdr = NULL;
									break;

		case MM_TYPE_SOFT_HEADER:		
									pMemory = pMmSoftHdr= CONTAINING_RECORD( pListEntry, MM_SOFT_HDR, MmSoftHdrLink); 

									InitializeListHead( &pMmSoftHdr->MmSoftHdrLink );
									pMmSoftHdr->Flag	= 0;
									OS_SetFlag( pMmSoftHdr->Flag, MM_FLAG_SOFT_HDR);
									OS_ZeroMemory( &pMmSoftHdr->Hdr, sizeof(pMmSoftHdr->Hdr));
									break;

		case MM_TYPE_PROTOCOL_HEADER:		
									pMemory = pMmProtoHdr = CONTAINING_RECORD( pListEntry, MM_PROTO_HDR, MmProtoHdrLink); 

									InitializeListHead( &pMmProtoHdr->MmProtoHdrLink );
									pMmProtoHdr->Flag		= 0;
									pMmProtoHdr->RawDataSize= 0;
									pMmProtoHdr->Event		= NULL;
									pMmProtoHdr->SftkDev	= NULL;	
									pMmProtoHdr->RetProtoHDr= NULL;
									pMmProtoHdr->RetBuffer	= NULL;
									pMmProtoHdr->RetBufferSize= 0;
									pMmProtoHdr->Status		= STATUS_SUCCESS;

									OS_SetFlag( pMmProtoHdr->Flag, MM_FLAG_PROTO_HDR);
									OS_ZeroMemory( &pMmProtoHdr->Hdr, sizeof(pMmSoftHdr->Hdr));
									ANCHOR_InitializeListHead( pMmProtoHdr->MmSoftHdrList );
									break;

		case MM_TYPE_4K_NPAGED_MEM:		
		default :					DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_alloc: inside Switch() pMM_acnhor->Type %d is invalid with this API !!! FIXME FIXME !!! \n",
													pMM_anchor->Type));
									OS_ASSERT(FALSE);
									break; // ERROR
	} // switch(MM_Type)

done:
	OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);
	return pMemory;
} // mm_type_alloc()

VOID
mm_type_free( PVOID	Memory, UCHAR	MM_Type)
{
	PMM_ANCHOR				pMM_anchor	= &GSftk_Config.Mmgr.MmSlab[MM_Type];
	PMM_CHUNK_ENTRY			pMmChunkEntry;
	PMM_ENTRY				pMmEntry;
	PMM_HOLDER				pMmHolder;
	PMM_SOFT_HDR			pMmSoftHdr;
	PMM_PROTO_HDR			pMmProtoHdr;
	PLIST_ENTRY				pListEntry;

	if (GSftk_Config.Mmgr.MM_Initialized == FALSE)
	{ // MMMgr not started 
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_free: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										GSftk_Config.Mmgr.MM_Initialized, NULL));
		OS_ASSERT(FALSE);
		return;
	} // MMMgr not started 

	switch(MM_Type)
	{
		case MM_TYPE_CHUNK_ENTRY:	pMmChunkEntry = Memory;
									InitializeListHead( &pMmChunkEntry->MmChunkLink );
									ANCHOR_InitializeListHead( pMmChunkEntry->MmEntryList );
									//pMmChunkEntry->FreeMap	= 0;
									pMmChunkEntry->Flag		= 0;
									pMmChunkEntry->VAddr	= NULL;

									pListEntry = &pMmChunkEntry->MmChunkLink;
									break;


		case MM_TYPE_MM_ENTRY:		pMmEntry = Memory; 
									InitializeListHead( &pMmEntry->MmEntryLink );
									pMmEntry->PageEntry		= NULL;
									pMmEntry->ChunkEntry	= NULL;

									pListEntry = &pMmEntry->MmEntryLink;
									break;

		case MM_TYPE_MM_HOLDER:		pMmHolder = Memory;
									InitializeListHead( &pMmHolder->IrpContextList );
									InitializeListHead( &pMmHolder->MmHolderLink );
									pMmHolder->FlagLink		= 0;
									ANCHOR_InitializeListHead( pMmHolder->MmEntryList );
									pMmHolder->Mdl			= NULL;
									pMmHolder->Size			= 0;
									pMmHolder->SystemVAddr	= NULL;
									pMmHolder->Proto_type	= 0;
									pMmHolder->pProtocolHdr = NULL;
									
									pListEntry = &pMmHolder->MmHolderLink;
									break;

		case MM_TYPE_SOFT_HEADER:	pMmSoftHdr = Memory;

									if ( OS_IsFlagSet( pMmSoftHdr->Flag, MM_FLAG_ALLOCATED_FROM_OS) == TRUE )
									{
										GSftk_Config.Mmgr.MM_OSMemUsed -= pMM_anchor->NodeSize;
										// Check if the MDL is not Null then free the MDL.
										if(pMmSoftHdr->Mdl != NULL){
											TDI_UnlockAndFreeMdl(pMmSoftHdr->Mdl);
											pMmSoftHdr->Mdl = NULL;
											pMmSoftHdr->Size = 0;
										}
										OS_FreeMemory(pMmSoftHdr);
										return;
									}

									InitializeListHead( &pMmSoftHdr->MmSoftHdrLink );
									pMmSoftHdr->Flag	= 0;
									pMmSoftHdr->SftkDev	= NULL;	
									OS_SetFlag( pMmSoftHdr->Flag, MM_FLAG_SOFT_HDR);
									OS_ZeroMemory( &pMmSoftHdr->Hdr, sizeof(pMmSoftHdr->Hdr));

									pListEntry = &pMmSoftHdr->MmSoftHdrLink;
									break;

		case MM_TYPE_PROTOCOL_HEADER:		
									pMmProtoHdr = Memory;

									if ( OS_IsFlagSet( pMmProtoHdr->Flag, MM_FLAG_ALLOCATED_FROM_OS) == TRUE )
									{
										GSftk_Config.Mmgr.MM_OSMemUsed -= pMM_anchor->NodeSize;
										// Check if the MDL is not Null then free the MDL.
										if(pMmProtoHdr->Mdl != NULL){
											TDI_UnlockAndFreeMdl(pMmProtoHdr->Mdl);
											pMmProtoHdr->Mdl = NULL;
											pMmProtoHdr->Size = 0;
										}
										OS_FreeMemory(pMmProtoHdr);
										return;
									}
									InitializeListHead( &pMmProtoHdr->MmProtoHdrLink );
									pMmProtoHdr->Flag		= 0;
									pMmProtoHdr->RawDataSize= 0;
									pMmProtoHdr->Event		= NULL;
									pMmProtoHdr->SftkDev	= NULL;	
									pMmProtoHdr->RetProtoHDr= NULL;
									pMmProtoHdr->RetBuffer	= NULL;
									pMmProtoHdr->RetBufferSize= 0;
									pMmProtoHdr->Status		= STATUS_SUCCESS;

									OS_SetFlag( pMmProtoHdr->Flag, MM_FLAG_PROTO_HDR);
									OS_ZeroMemory( &pMmProtoHdr->Hdr, sizeof(pMmSoftHdr->Hdr));
									ANCHOR_InitializeListHead( pMmProtoHdr->MmSoftHdrList );

									pListEntry = &pMmProtoHdr->MmProtoHdrLink;
									break;

		case MM_TYPE_4K_NPAGED_MEM:		
		default :					DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_free: inside Switch() pMM_acnhor->Type %d is invalid with this API !!! FIXME FIXME !!! \n",
													pMM_anchor->Type));
									OS_ASSERT(FALSE);
									return;	// EROR !!!
									break; // ERROR
	} // switch(MM_Type)

	OS_ACQUIRE_LOCK( &pMM_anchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	InsertTailList( &pMM_anchor->FreeList.ListEntry, pListEntry );
	pMM_anchor->FreeList.NumOfNodes ++;
	OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);

	return;
} // mm_type_free()

//
//	Function:	mm_type_pages_add()
//
//	Parameters: 
//		IN OUT PMM_MANAGER					Mmgr
//			Initialized Mmgr structures
//		IN		PSET_MM_RAW_MEMORY		MMRawMem
//			Information about new pages 
//
//	Description:
//			It retrieves new pages from MMRawMem and allocates it into MMSlab[MM_TYPE_4K_NPAGED_MEM]
//			If Mmgr->AWEUsed == TRUE 
//					It directly retrieves system physical pages and uses MM_ENTRY
//					to add into MMSlab[MM_TYPE_4K_NPAGED_MEM].FreeList.
//				
//			If Mmgr->AWEUsed == FALSE 
//					It uses MDL to Locked users Vaddr to get system physical pages 
//					It uses MM_CHUNK_ENTRY with MM_ENTRY to store this physical pages
//					It adds MM_CHUNK_ENTRY to add free pages into MMSlab[MM_TYPE_4K_NPAGED_MEM].FreeList.
//	
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
mm_type_pages_add(	IN	OUT PMM_MANAGER				Mmgr,
					IN		PSET_MM_RAW_MEMORY		MMRawMem)
{
	NTSTATUS				status		= STATUS_SUCCESS;
	PMM_ANCHOR				pMM_anchor	= &Mmgr->MmSlab[MM_TYPE_4K_NPAGED_MEM];
	ULONG					i, j;	
	PMM_CHUNK_ENTRY			pMmChunkEntry;
	PMM_ENTRY				pMmEntry;
	PLIST_ENTRY				pListEntry;	// used specially for error handling
	PLIST_ENTRY				pLocalList = NULL;	// used specially for error handling
	PULONG					arrayOfPfn = (PULONG) &MMRawMem->ArrayOfMem;

	if (Mmgr->MM_Initialized == FALSE)
	{ // MMMgr not started 
		status = STATUS_UNSUCCESSFUL;
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_pages_add: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										Mmgr->MM_Initialized, status));
		return status;
	} // MMMgr not started 

	if (Mmgr->AWEUsed == TRUE)
	{ // if: AWE is used 
		for (i=0; i < MMRawMem->NumberOfArray; i++)
		{ // for: Use MM_ENTRY to Add each and every pages

			pMmEntry = mm_type_alloc(MM_TYPE_MM_ENTRY);
			if (pMmEntry == NULL)
			{ // Failed :should not happened
				status = STATUS_INSUFFICIENT_RESOURCES;
				DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_pages_add: mm_type_alloc(MM_TYPE_MM_ENTRY %d) Failed returning status 0x%08x !! BUG : FIXME FIXME \n",
													MM_TYPE_MM_ENTRY, status));
				OS_ASSERT(FALSE);
				goto done;
			}
			// initialized it
			pMmEntry->PageEntry		= (PULONG) arrayOfPfn[i]; // MMRawMem->ArrayOfMem[i];
			pMmEntry->ChunkEntry	= NULL;
			
			if (i == 0)
				pLocalList = &pMmEntry->MmEntryLink;
			else
				InsertTailList( pLocalList, &pMmEntry->MmEntryLink );	// Insert it into Local Anchor List
		} // for: Use MM_ENTRY to Add each and every pages

	} // if: AWE is used 
	else
	{ // else : AWE is NOT used, Virtual Alloc is used 
		OS_ASSERT( Mmgr->VChunkSize == MMRawMem->ChunkSize );
		OS_ASSERT( (Mmgr->VChunkSize % Mmgr->PageSize) == 0);
		OS_ASSERT( (Mmgr->VChunkSize / Mmgr->PageSize) < 32);	// Chunk based Bitmap is ULONG value

		OS_ASSERT( pMM_anchor->NumOfMMEntryPerChunk >= 1);	// Chunk based Bitmap is ULONG value
	
		for (i=0; i < MMRawMem->NumberOfArray; i++)
		{ // for : Allocate MM_CHUNK_ENTRY and Add each and every pages in it using MM_ENTRY struct
			pMmChunkEntry = mm_type_alloc(MM_TYPE_CHUNK_ENTRY);	// Retrieve Free MM_CHUNK_ENTRY 
			if (pMmChunkEntry == NULL)
			{ // Failed :should not happened
				status = STATUS_INSUFFICIENT_RESOURCES;
				DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_pages_add: mm_type_alloc(MM_TYPE_CHUNK_ENTRY %d) Failed returning status 0x%08x !! BUG : FIXME FIXME \n",
													MM_TYPE_CHUNK_ENTRY, status));
				OS_ASSERT(FALSE);
				goto done;
			}

			//pMmChunkEntry->FreeMap	= 0;	// Everything is free...
			pMmChunkEntry->VAddr	= (PVOID) arrayOfPfn[i];  // MMRawMem->ArrayOfMem[i];
			pMmChunkEntry->Flag		= 0;
			OS_SetFlag(pMmChunkEntry->Flag, MM_CHUNK_FLAG_FREE_LIST);

			for (j=0; j < pMM_anchor->NumOfMMEntryPerChunk; j++)
			{ // for : Allocate, initialized and add MM_ENTRY into MM_CHUNK_ENTRY
				// Retrieve Free MM_ENTRY structure from MMSlab[]
				pMmEntry = mm_type_alloc(MM_TYPE_MM_ENTRY);
				if (pMmEntry == NULL)
				{ // Failed :should not happened
					status = STATUS_INSUFFICIENT_RESOURCES;
					DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_pages_add: mm_type_alloc(MM_TYPE_MM_ENTRY %d) Failed returning status 0x%08x !! BUG : FIXME FIXME \n",
														MM_TYPE_MM_ENTRY, status));
					OS_ASSERT(FALSE);
					// Free here current MM_CHUNK_ENTRY and its relative MM_ENTRY structure
					while( !(IsListEmpty(&pMmChunkEntry->MmEntryList.ListEntry)) )
					{ // While : Scan thru local list and free back MM slab
						pListEntry = RemoveHeadList(&pMmChunkEntry->MmEntryList.ListEntry);
						pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink);
						mm_type_free(pMmEntry, MM_TYPE_MM_ENTRY);
					}
					mm_type_free(pMmChunkEntry, MM_TYPE_CHUNK_ENTRY);
					pMmChunkEntry = NULL;
					goto done;
				}

				// initialized it
				pMmEntry->PageEntry		= (PULONG) NULL;
				pMmEntry->ChunkEntry			=  pMmChunkEntry;

				// insert this MM_ENTRY into MM_CHUNK_ENTRY Anchor
				InsertTailList( &pMmChunkEntry->MmEntryList.ListEntry, &pMmEntry->MmEntryLink );
				pMmChunkEntry->MmEntryList.NumOfNodes ++;
			} // for : Allocate, initialized and add MM_ENTRY into MM_CHUNK_ENTRY

			OS_ASSERT(pMmChunkEntry->VAddr != NULL);

			// Now retrieve and store physical pages in MM_CHUNK's list of MM_ENTRY
			status = mm_locked_and_init_Chunk( pMmChunkEntry->VAddr, // (PVOID)	&MMRawMem->ArrayOfMem[i],
												MMRawMem->ChunkSize,
												pMM_anchor->NumOfMMEntryPerChunk,
												pMmChunkEntry);
			if (!NT_SUCCESS(status))
			{ // Failed
				DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_pages_add: mm_locked_and_init_Chunk(Index %d Vaddr 0x%08x, Size %d) Failed returning status 0x%08x !! BUG : FIXME FIXME \n",
													i, pMmChunkEntry->VAddr, MMRawMem->ChunkSize, status));

				// Free here current MM_CHUNK_ENTRY and its relative MM_ENTRY structure
				while( !(IsListEmpty(&pMmChunkEntry->MmEntryList.ListEntry)) )
				{ // While : Scan thru local list and free back MM slab
					pListEntry = RemoveHeadList(&pMmChunkEntry->MmEntryList.ListEntry);
					pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink);
					mm_type_free(pMmEntry, MM_TYPE_MM_ENTRY);
				}
				mm_type_free(pMmChunkEntry, MM_TYPE_CHUNK_ENTRY);
				pMmChunkEntry = NULL;
				goto done;
			}
			// Insert it into Local Anchor List
			if (i == 0)
				pLocalList = &pMmChunkEntry->MmChunkLink;
			else
				InsertTailList( pLocalList, &pMmChunkEntry->MmChunkLink);	// Insert it into Local Anchor List
		} // for : Allocate MM_CHUNK_ENTRY and Add each and every pages in it using MM_ENTRY struct
	} // else : AWE is NOT used, Virtual Alloc is used 

	// Add this list into MM_TYPE_4K_NPAGED_MEM
	OS_ACQUIRE_LOCK( &pMM_anchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	// InsertTailList( &pMM_anchor->FreeList.ListEntry, &localList );
	// adding group of link list to Tail of Anchor
	InsertGroupListToTailList(&pMM_anchor->FreeList.ListEntry, pLocalList);

	pMM_anchor->FreeList.NumOfNodes			+= MMRawMem->NumberOfArray;
	pMM_anchor->TotalNumberOfNodes			+= MMRawMem->NumberOfArray;
	pMM_anchor->TotalNumberOfRawWFreeNodes	+= MMRawMem->NumberOfArray;
	pMM_anchor->TotalMemSize				+= MMRawMem->TotalMemorySize;

	Mmgr->TotalMemAllocated	+= MMRawMem->TotalMemorySize;

	OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);

	status = STATUS_SUCCESS;

done:
	if (!NT_SUCCESS(status))
	{ // Failed : Do Cleanup
		if (Mmgr->AWEUsed == TRUE)
		{ // AWE is used 
			if (pLocalList != NULL)
			{
				while(pLocalList->Flink != pLocalList)
				{ // While : Scan thru local list and free back MM slab
					pListEntry = RemoveHeadList(pLocalList);
					pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink);
					mm_type_free(pMmEntry, MM_TYPE_MM_ENTRY);
				}
				// Free itself now
				pMmEntry = CONTAINING_RECORD( pLocalList, MM_ENTRY, MmEntryLink);
				mm_type_free(pMmEntry, MM_TYPE_MM_ENTRY);
			}
		}
		else
		{ // AWE is NOT used, Virtual Alloc is used 
			if (pLocalList != NULL)
			{
				PLIST_ENTRY	plistEntry;
				// Free here all MM_CHUNK_ENTRY and its relative MM_ENTRY structure
				while(pLocalList->Flink != pLocalList)
				{ // While : Scan thru local list and free back MM_CHUNK to its MM slab
					pListEntry = RemoveHeadList(pLocalList);
					pMmChunkEntry = CONTAINING_RECORD( pListEntry, MM_CHUNK_ENTRY, MmChunkLink);

					// unlocked it pages
					status = mm_unlocked_and_deInit_Chunk(	MMRawMem->ChunkSize,
															pMM_anchor->NumOfMMEntryPerChunk,
															pMmChunkEntry);
					if (!NT_SUCCESS(status))
					{ // Failed
						DebugPrint((DBG_ERROR, "FIXE FIXME :: AWE NOT USED mm_type_pages_add: mm_unlocked_and_deInit_Chunk(Vaddr 0x%08x, Size %d) Failed returning status 0x%08x !! BUG : FIXME FIXME \n",
																pMmChunkEntry->VAddr, MMRawMem->ChunkSize, status));
					}

					while( !(IsListEmpty(&pMmChunkEntry->MmEntryList.ListEntry)) )
					{ // While : Scan thru pMmChunkEntry list and free back MM_ENTRY to its MM slab
						plistEntry = RemoveHeadList(&pMmChunkEntry->MmEntryList.ListEntry);
						pMmEntry = CONTAINING_RECORD( plistEntry, MM_ENTRY, MmEntryLink);
						
						mm_type_free(pMmEntry, MM_TYPE_MM_ENTRY);
					}
					mm_type_free(pMmChunkEntry, MM_TYPE_CHUNK_ENTRY);
				} // While : Scan thru local list and free back MM_CHUNK to its MM slab

				// Free itself now
				pMmChunkEntry = CONTAINING_RECORD( pLocalList, MM_CHUNK_ENTRY, MmChunkLink);
				// unlocked it pages
				status = mm_unlocked_and_deInit_Chunk(	MMRawMem->ChunkSize,
														pMM_anchor->NumOfMMEntryPerChunk,
														pMmChunkEntry);
				if (!NT_SUCCESS(status))
				{ // Failed
					DebugPrint((DBG_ERROR, "FIXE FIXME :: AWE NOT USED : mm_type_pages_add: mm_unlocked_and_deInit_Chunk(Vaddr 0x%08x, Size %d) Failed returning status 0x%08x !! BUG : FIXME FIXME \n",
																pMmChunkEntry->VAddr, MMRawMem->ChunkSize, status));
				}

				while( !(IsListEmpty(&pMmChunkEntry->MmEntryList.ListEntry)) )
				{ // While : Scan thru pMmChunkEntry list and free back MM_ENTRY to its MM slab
					plistEntry = RemoveHeadList(&pMmChunkEntry->MmEntryList.ListEntry);
					pMmEntry = CONTAINING_RECORD( plistEntry, MM_ENTRY, MmEntryLink);
					
					mm_type_free(pMmEntry, MM_TYPE_MM_ENTRY);
				}
				mm_type_free(pMmChunkEntry, MM_TYPE_CHUNK_ENTRY);
			}
		} // AWE is NOT used, Virtual Alloc is used 
	} // Failed : Do Cleanup

	return status;
} // mm_type_pages_add()

//
//	Function:	mm_type_pages_remove()
//
//	Parameters: 
//		IN OUT PMM_MANAGER					Mmgr
//			Initialized Mmgr structures
//		IN		PSET_MM_RAW_MEMORY		MMRawMem
//			Information about new pages 
//
//	Description:
//			It retrieves Free pages and returns into MMRawMem 
//			If Mmgr->AWEUsed == TRUE 
//					It directly retrieves system physical pages and uses MM_ENTRY
//					to remove from MMSlab[MM_TYPE_4K_NPAGED_MEM].FreeList.
//				
//			If Mmgr->AWEUsed == FALSE 
//					It uses MDL to UnLocked users Vaddr 
//					It uses MM_CHUNK_ENTRY with MM_ENTRY to unlocked MDL Vaddr
//					It removes free MM_CHUNK_ENTRY and returns Vaddr into MMRawMem
//	
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
mm_type_pages_remove(	IN	OUT PMM_MANAGER				Mmgr,
						IN		PSET_MM_RAW_MEMORY		MMRawMem)
{
	NTSTATUS				status		= STATUS_SUCCESS;
	PMM_ANCHOR				pMM_anchor	= &Mmgr->MmSlab[MM_TYPE_4K_NPAGED_MEM];
	ULONG					i, j;	
	PMM_CHUNK_ENTRY			pMmChunkEntry;
	PMM_ENTRY				pMmEntry;
	PLIST_ENTRY				pListEntry, plistEntry;	// used specially for error handling
	PULONG					arrayOfPfn = (PULONG) &MMRawMem->ArrayOfMem;

	if (Mmgr->MM_Initialized == FALSE)
	{ // MMMgr not started 
		status = STATUS_UNSUCCESSFUL;
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_pages_remove: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										Mmgr->MM_Initialized, status));
		return status;
	} // MMMgr not started 

	i=0;
	OS_ACQUIRE_LOCK( &pMM_anchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	if (Mmgr->AWEUsed == TRUE)
	{ // if: AWE is used 
		while( !(IsListEmpty(&pMM_anchor->FreeList.ListEntry)) )
		{ // While : Scan thru local list and free back MM slab
			if (i >= MMRawMem->NumberOfArray)
				break;

			pListEntry = RemoveHeadList(&pMM_anchor->FreeList.ListEntry);
			pMM_anchor->FreeList.NumOfNodes			--;
			pMM_anchor->TotalNumberOfNodes			--;
			pMM_anchor->TotalNumberOfRawWFreeNodes	--;
			pMM_anchor->TotalMemSize				-= (ULONGLONG) Mmgr->PageSize;

			Mmgr->TotalMemAllocated					-= (ULONGLONG) Mmgr->PageSize;

			pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink);

			arrayOfPfn[i] = (ULONG) pMmEntry->PageEntry;
			i ++;

			pMmEntry->PageEntry = NULL;

			mm_type_free(pMmEntry, MM_TYPE_MM_ENTRY);
		} // While : Scan thru local list and free back MM slab
	} // if: AWE is used 
	else
	{ // else : AWE is NOT used, Virtual Alloc is used 
		OS_ASSERT( Mmgr->VChunkSize == MMRawMem->ChunkSize );
		OS_ASSERT( (Mmgr->VChunkSize % Mmgr->PageSize) == 0);
		OS_ASSERT( (Mmgr->VChunkSize / Mmgr->PageSize) < 32);	// Chunk based Bitmap is ULONG value
	
		// REMEMBER: use BLink to scan from Tail, Since Tail will have more free Complete ChunkEntrys
		for(plistEntry = pMM_anchor->FreeList.ListEntry.Blink;
			plistEntry != &pMM_anchor->FreeList.ListEntry;)
		{ // for :Scan thru Chunk entry freelist from tail
			if (i >= MMRawMem->NumberOfArray)
				break;	// we are done

			pMmChunkEntry = CONTAINING_RECORD( plistEntry, MM_CHUNK_ENTRY, MmChunkLink); 
			plistEntry = plistEntry->Blink;

			if (pMmChunkEntry->MmEntryList.NumOfNodes < pMM_anchor->NumOfMMEntryPerChunk)
			{ // we found first chunk Which does not have whole Chunk free !!! 
				DebugPrint((DBG_ERROR, "mm_type_pages_remove: Found First chunk from Tail Which Is not completely free!!! pMmChunkEntry->MmEntryList.NumOfNodes %d, Terminating free search!!! \n",
										pMmChunkEntry->MmEntryList.NumOfNodes, status));
				break;
			}

			OS_ASSERT( pMmChunkEntry->MmEntryList.NumOfNodes == pMM_anchor->NumOfMMEntryPerChunk);
			// List must not be empty if it present in FreeList
			OS_ASSERT( OS_IsFlagSet( pMmChunkEntry->Flag, MM_CHUNK_FLAG_FREE_LIST) == TRUE);
			OS_ASSERT( IsListEmpty( &pMmChunkEntry->MmEntryList.ListEntry) == FALSE);

			// Remove it from Free List Anchor
			RemoveEntryList( &pMmChunkEntry->MmChunkLink );
			pMM_anchor->FreeList.NumOfNodes			--;
			pMM_anchor->TotalNumberOfNodes			--;
			pMM_anchor->TotalNumberOfRawWFreeNodes	--;
			pMM_anchor->TotalMemSize				-= (ULONGLONG) Mmgr->VChunkSize;

			Mmgr->TotalMemAllocated					-= (ULONGLONG) Mmgr->VChunkSize;
			// OS_ClearFlag( pMmChunkEntry->Flag, MM_CHUNK_FLAG_FREE_LIST);
			
			// Unlock VAddr Physical pages in kernel
			status = mm_unlocked_and_deInit_Chunk(	Mmgr->VChunkSize,
													pMM_anchor->NumOfMMEntryPerChunk,
													pMmChunkEntry);
			if (!NT_SUCCESS(status))
			{ // Failed
				DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_pages_remove: mm_locked_and_init_Chunk(Vaddr 0x%08x, Size %d) Failed returning status 0x%08x !! BUG : FIXME FIXME \n",
															pMmChunkEntry->VAddr ,Mmgr->VChunkSize, status));
				status = STATUS_SUCCESS;
				OS_ASSERT(FALSE);
			}

			while( !IsListEmpty( &pMmChunkEntry->MmEntryList.ListEntry) ) 
			{ // While : list is not empty
				pListEntry = RemoveHeadList( &pMmChunkEntry->MmEntryList.ListEntry);
				pMmChunkEntry->MmEntryList.NumOfNodes --;	// Decrement Counter.

				pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink); 

				OS_ASSERT(pMmEntry->ChunkEntry == pMmChunkEntry);

				// InitializeListHead( &pMmEntry->MmEntryLink );
				// pMmEntry->ChunkEntry = NULL;
				mm_type_free(pMmEntry, MM_TYPE_MM_ENTRY);
			} // While : list is not empty

			arrayOfPfn[i] = (ULONG) pMmChunkEntry->VAddr;
			i ++;
			// pMmChunkEntry->VAddr = NULL;
			mm_type_free(pMmChunkEntry, MM_TYPE_CHUNK_ENTRY);

			if (i >= MMRawMem->NumberOfArray)
				break;	// we are done

		} // for :Scan thru Chunk entry freelist from tail
	} // else : AWE is NOT used, Virtual Alloc is used 

	OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);

	MMRawMem->NumberOfArray = i;	// store final values

	return STATUS_SUCCESS;
} // mm_type_pages_remove()

NTSTATUS
mm_locked_mdlInfo(	IN		PVIRTUAL_MM_INFO	VaddrInfo,
					IN OUT	PMM_MDL_INFO		MdlInfo )
{
	NTSTATUS	status = STATUS_SUCCESS;

	MdlInfo->Mdl			= NULL;
	MdlInfo->SystemVAddr	= NULL;
	
	MdlInfo->Mdl = MmCreateMdl( MdlInfo->Mdl, VaddrInfo->Vaddr, VaddrInfo->SizeOfMem);

	if(MdlInfo->Mdl == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "mm_locked_mdlInfo: MmCreateMdl( baseVaddr 0x%08x, size %d) Failed!!! returning error 0x%08x !!! \n",
										VaddrInfo->Vaddr, VaddrInfo->SizeOfMem, status));
		return status;
	}

	try 
    {
		MmProbeAndLockPages( MdlInfo->Mdl, KernelMode, IoModifyAccess );
	}
	except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
	{
		status = GetExceptionCode();	
		// Log event message here
		sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

		DebugPrint((DBG_ERROR, "mm_locked_mdlInfo: MmProbeAndLockPages( Mdl 0x%08x, baseVaddr 0x%08x, size %d) Failed with exception code 0x%08x!!! returning error 0x%08x !!! \n",
										MdlInfo->Mdl, VaddrInfo->Vaddr, VaddrInfo->SizeOfMem, status, status));
		IoFreeMdl( MdlInfo->Mdl );	// free mdl
		MdlInfo->Mdl = NULL;
		return status;
	}

	MdlInfo->SystemVAddr = MmGetSystemAddressForMdl( MdlInfo->Mdl );
	if(MdlInfo->SystemVAddr == NULL)	
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "mm_locked_mdlInfo: MmGetSystemAddressForMdl( Mdl 0x%08x baseVaddr 0x%08x, size %d) Failed!!! returning error 0x%08x !!! \n",
										MdlInfo->Mdl, VaddrInfo->Vaddr, VaddrInfo->SizeOfMem, status));
		MmUnlockPages( MdlInfo->Mdl );	// unlock pages
		IoFreeMdl( MdlInfo->Mdl );		// free mdl
		MdlInfo->Mdl = NULL;
		return status;
	}

	MdlInfo->OrgVAddr	= VaddrInfo->Vaddr;
	MdlInfo->Size		= VaddrInfo->SizeOfMem;

	return STATUS_SUCCESS;
} // mm_locked_mdlInfo()

NTSTATUS
mm_unlocked_mdlInfo(	IN OUT	PMM_MDL_INFO	MdlInfo )
{
	if (MdlInfo->Mdl)
	{
		OS_ASSERT(MdlInfo->SystemVAddr != NULL);
		
		MmUnlockPages( MdlInfo->Mdl);	// unlock pages
		IoFreeMdl( MdlInfo->Mdl );	// Free mdl
	}

	MdlInfo->Mdl			= NULL;
	MdlInfo->SystemVAddr	= NULL;

	return STATUS_SUCCESS;
} // mm_unlocked_mdlInfo()

NTSTATUS
mm_locked_and_init_Chunk(	IN		PVOID			Vaddr,
							IN		ULONG			ChunkSize,
							IN		ULONG			NumberOfPages,
							IN	OUT	PMM_CHUNK_ENTRY	MmChunkEntry)
{
	NTSTATUS		status	= STATUS_SUCCESS;
	PMDL			pMdl	=	NULL;
	PLIST_ENTRY		plistEntry; 
	PMM_ENTRY		pMmEntry;
	PULONG			phyPages;
	ULONG			i;	
		
	pMdl = MmCreateMdl( pMdl, Vaddr, ChunkSize);

	if(pMdl == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "mm_locked_and_init_Chunk: MmCreateMdl( baseVaddr 0x%08x, size %d) Failed!!! returning error 0x%08x !!! \n",
										Vaddr, ChunkSize, status));
		return status;
	}

	try 
    {
		MmProbeAndLockPages( pMdl, KernelMode, IoModifyAccess );
	}
	except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
	{
		status = GetExceptionCode();	
		// Log event message here
		sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

		DebugPrint((DBG_ERROR, "FIXME FIXME mm_locked_and_init_Chunk: MmProbeAndLockPages( Mdl 0x%08x, baseVaddr 0x%08x, size %d) Failed with exception code 0x%08x!!! returning error 0x%08x !!! \n",
										pMdl, Vaddr, ChunkSize, status, status));
		IoFreeMdl( pMdl );	// free mdl
		return status;
	}

	phyPages = (PULONG) (pMdl + 1);;
	for( plistEntry = MmChunkEntry->MmEntryList.ListEntry.Flink, i=0 ;
		 plistEntry != &MmChunkEntry->MmEntryList.ListEntry;
		 plistEntry = plistEntry->Flink, i++)
	{ // for :Set page in each and every MM_ENTRY struct
		if ( i >= NumberOfPages)
		{
			DebugPrint((DBG_ERROR, "FIXME FIXME mm_locked_and_init_Chunk: i %d >= NumberOfPages %d NOT POSSIBLE !! BUG FIXME FIXME !!! \n",
										i, NumberOfPages));
			OS_ASSERT(FALSE);
			break;
		}
		pMmEntry = CONTAINING_RECORD( plistEntry, MM_ENTRY, MmEntryLink);
		pMmEntry->PageEntry = (PULONG) phyPages[i];
	} // for :Set page in each and every MM_ENTRY struct

	if ( i != NumberOfPages)
	{
		DebugPrint((DBG_ERROR, "FIXME FIXME mm_locked_and_init_Chunk: i %d != NumberOfPages %d NOT POSSIBLE !! BUG FIXME FIXME !!! \n",
										i, NumberOfPages));
		OS_ASSERT(FALSE);
	}

	GSftk_Config.Mmgr.PSystemProcess = (PVOID) pMdl->Process;

	// MdlInfo->SystemVAddr = MmGetSystemAddressForMdl( MdlInfo->Mdl );

	// Free MDL structure
	IoFreeMdl( pMdl );

	return STATUS_SUCCESS;
} // mm_locked_and_init_Chunk()

NTSTATUS
mm_unlocked_and_deInit_Chunk(	IN		ULONG			ChunkSize,
								IN		ULONG			NumberOfPages,
								IN	OUT	PMM_CHUNK_ENTRY	MmChunkEntry)
{
	NTSTATUS		status	= STATUS_SUCCESS;
	PMDL			pMdl	=	NULL;
	PLIST_ENTRY		plistEntry; 
	PMM_ENTRY		pMmEntry;
	PULONG			phyPages;
	ULONG			i;	
		
	pMdl = MmCreateMdl( pMdl, (void *) 0, ChunkSize);

	if(pMdl == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "mm_unlocked_and_deInit_Chunk: MmCreateMdl( baseVaddr 0x%08x, size %d) Failed!!! returning error 0x%08x !!! \n",
										0, ChunkSize, status));
		return status;
	}

	// Initialise this MDL with system process pointer saved in DataAddrVirtToPhys()
	// Need to use the same proc to unlock or else, will not decrement _MmPageLockCount
	// preventing subsequent memroy lock down as it will have exceeded quota
	pMdl->Process = (PEPROCESS) GSftk_Config.Mmgr.PSystemProcess;

	 // store previously acquired physical addresss at end of mdl
	// *(PULONG)(pMdl + 1) = getDataPhysPage();

	phyPages = (PULONG) (pMdl + 1);

	for( plistEntry = MmChunkEntry->MmEntryList.ListEntry.Flink, i=0 ;
		 plistEntry != &MmChunkEntry->MmEntryList.ListEntry;
		 plistEntry = plistEntry->Flink, i++)
	{ // for :Set page in each and every MM_ENTRY struct
		if ( i >= NumberOfPages)
		{
			DebugPrint((DBG_ERROR, "FIXME FIXME mm_unlocked_and_deInit_Chunk: i %d >= NumberOfPages %d NOT POSSIBLE !! BUG FIXME FIXME !!! \n",
										i, NumberOfPages));
			OS_ASSERT(FALSE);
			break;
		}
		pMmEntry = CONTAINING_RECORD( plistEntry, MM_ENTRY, MmEntryLink);

		phyPages[i] = (ULONG) pMmEntry->PageEntry;

		pMmEntry->PageEntry = NULL;
	} // for :Set page in each and every MM_ENTRY struct

	if ( i != NumberOfPages)
	{
		DebugPrint((DBG_ERROR, "FIXME FIXME mm_unlocked_and_deInit_Chunk: i %d != NumberOfPages %d NOT POSSIBLE !! BUG FIXME FIXME !!! \n",
										i, NumberOfPages));
		OS_ASSERT(FALSE);
	}
	 // probe it 
	/* 
		We're using physical pages (via AWE) allocated up in user-space. Because of 
		this, we can't call MmProbeAndLockPages(). However, MmMapLockedPages() wants
		the MDL_PAGES_LOCKED flag set or it will complain (in the checked build). 

		So, we manually set the flag here to let the system know that the pages are locked. 
	*/ 
	pMdl->MdlFlags |= MDL_PAGES_LOCKED ; 

	try 
    {
		// get system virtual address to unlock
		MmMapLockedPages(pMdl, KernelMode);	 
		// unlock pages
		MmUnlockPages( pMdl );
	}
	except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
	{
		status = GetExceptionCode();	
		// Log event message here
		sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

		DebugPrint((DBG_ERROR, "FIXME FIXME mm_unlocked_and_deInit_Chunk: MmProbeAndLockPages( Mdl 0x%08x, baseVaddr 0x%08x, size %d) Failed with exception code 0x%08x!!! returning error 0x%08x !!! \n",
										pMdl, 0, ChunkSize, status, status));
		IoFreeMdl( pMdl );	// free mdl
		return status;
	}

	// MdlInfo->SystemVAddr = MmGetSystemAddressForMdl( MdlInfo->Mdl );

	// Free MDL structure
	IoFreeMdl( pMdl );

	return STATUS_SUCCESS;
} // mm_unlocked_and_deInit_Chunk()

PMM_HOLDER
mm_alloc( ULONG Size, BOOLEAN LockedAndMapedVA)
{
	PMM_ANCHOR				pMM_anchor	= &GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM];
	PVOID					pMemory		= NULL;
	PMM_HOLDER				pMmHolder	= NULL;
	PMM_CHUNK_ENTRY			pMmChunkEntry;
	PMM_ENTRY				pMmEntry;
	PLIST_ENTRY				plistEntry, pListEntry;
	NTSTATUS				status	= STATUS_UNSUCCESSFUL;
	PULONG					phyPages;
	ULONG					i, j;
	ULONG					numOfPages = Size / GSftk_Config.Mmgr.PageSize;
	BOOLEAN					bAllocMemFromService = FALSE;
	
	if (GSftk_Config.Mmgr.MM_Initialized == FALSE)
	{ // MMMgr not started 
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_type_free: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										GSftk_Config.Mmgr.MM_Initialized, NULL));
		return NULL;
	} // MMMgr not started 

	if ( (Size % GSftk_Config.Mmgr.PageSize) != 0 )
	{
#if DBG
		pMM_anchor->UnUsedSize += (Size % GSftk_Config.Mmgr.PageSize);
#endif
		numOfPages ++;	// for partial data, add one page 
	}

	if (GSftk_Config.Mmgr.MM_Initialized == FALSE)
	{ // MMMgr not started 
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_alloc: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										GSftk_Config.Mmgr.MM_Initialized));
		return NULL;
	} // MMMgr not started 

	pMmHolder = mm_type_alloc(MM_TYPE_MM_HOLDER);
	if (pMmHolder == NULL)
	{ // Failed :should not happened
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_alloc: mm_type_alloc(MM_TYPE_MM_HOLDER %d) Failed returning status 0x%08x !! BUG : FIXME FIXME \n",
											MM_TYPE_MM_HOLDER));
		// OS_ASSERT(FALSE);
		return NULL;
	}

	// Check and See if the Size Requested is Zero in this case only Proto Header might be there
	// with no data so we will just return the allocated MM_HOLDER
    if(Size == 0)
	{ // Size of Buffer Requested is Zero so no need to allocate Memory or Lock the Memory
		return pMmHolder; 
	}

	pMmHolder->Mdl = IoAllocateMdl( (void *) 0, Size, FALSE, FALSE, NULL);
	if(pMmHolder->Mdl == NULL)
	{
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_alloc: IoAllocateMdl(Size %d) Failed returning status 0x%08x !! BUG : FIXME FIXME \n",
											Size));
		mm_type_free(pMmHolder, MM_TYPE_MM_HOLDER);
		// OS_ASSERT(FALSE);
		return NULL;
	}
	// Set flags (see ntddk.h:mdlflags) to prevent system from reprobing,
	// relocking and obtaining a system virtual address for adresses we are
	// passing down
	pMmHolder->Mdl->MdlFlags =	MDL_WRITE_OPERATION | MDL_ALLOCATED_FIXED_SIZE | MDL_PAGES_LOCKED;

	// Assign a bogus address for system virtual address in the mdl.
	// A change in this address and mdl flag settings indicates that
	// the system vaddr was mapped and we need to unmap it.
	// pMmHolder->Mdl->MappedSystemVa = (void *)0xDEADFACE;	// we do not need this since we need system virtual address
	phyPages = (PULONG) (pMmHolder->Mdl + 1);
	
	OS_ACQUIRE_LOCK( &pMM_anchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	if (GSftk_Config.Mmgr.AWEUsed == TRUE)
	{ // if: AWE is used 
		for (i=0; i < numOfPages; i++)
		{ // for: Use MM_ENTRY to Add each and every pages
			if ( IsListEmpty( &pMM_anchor->FreeList.ListEntry) ) 
			{ // Failed : No Free node presence !!
				DebugPrint((DBG_ERROR, "mm_alloc: No more Pages available !!! Number of Free nodes  %d!!! \n",
												pMM_anchor->FreeList.NumOfNodes));
				OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);
				goto done;	// failed
			}
			pListEntry = RemoveHeadList( &pMM_anchor->FreeList.ListEntry );
			pMM_anchor->FreeList.NumOfNodes --;	// Decrement Counter.
			
			pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink); 
			InitializeListHead( &pMmEntry->MmEntryLink );
			
			phyPages[i] = (ULONG) pMmEntry->PageEntry;

			// Insert it into Local Anchor List
			InsertTailList( &pMmHolder->MmEntryList.ListEntry, &pMmEntry->MmEntryLink );
			pMmHolder->MmEntryList.NumOfNodes ++;
		} // for: Use MM_ENTRY to Add each and every pages

	} // if: AWE is used 
	else
	{ // else : AWE is NOT used, Virtual Alloc is used 
		
		i = 0;
		for(plistEntry = pMM_anchor->FreeList.ListEntry.Flink;
			plistEntry != &pMM_anchor->FreeList.ListEntry;)
		{ // for :Scan thru Chunk entry freelist
			pMmChunkEntry = CONTAINING_RECORD( plistEntry, MM_CHUNK_ENTRY, MmChunkLink); 
			plistEntry = plistEntry->Flink;

			if (pMmChunkEntry->MmEntryList.NumOfNodes == pMM_anchor->NumOfMMEntryPerChunk)
				pMM_anchor->TotalNumberOfRawWFreeNodes --;	// since we are removing some entry from it

			// List must not be empty if it present in FreeList
			OS_ASSERT( OS_IsFlagSet( pMmChunkEntry->Flag, MM_CHUNK_FLAG_FREE_LIST) == TRUE);
			OS_ASSERT( IsListEmpty( &pMmChunkEntry->MmEntryList.ListEntry) == FALSE);

			while( !IsListEmpty( &pMmChunkEntry->MmEntryList.ListEntry) ) 
			{ // While : list is not empty
				if ( i >= numOfPages)
					break;	// we are done;

				pListEntry = RemoveHeadList( &pMmChunkEntry->MmEntryList.ListEntry);
				pMmChunkEntry->MmEntryList.NumOfNodes --;	// Decrement Counter.

				pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink); 
				InitializeListHead( &pMmEntry->MmEntryLink );

				OS_ASSERT(pMmEntry->ChunkEntry == pMmChunkEntry);
				OS_ASSERT(pMmEntry->PageEntry != NULL);
				
				phyPages[i] = (ULONG) pMmEntry->PageEntry;
				i ++;

				// Insert it into Local Anchor List
				InsertTailList( &pMmHolder->MmEntryList.ListEntry, &pMmEntry->MmEntryLink );
				pMmHolder->MmEntryList.NumOfNodes ++;

			} // While : list is not empty

			if (IsListEmpty( &pMmChunkEntry->MmEntryList.ListEntry))
			{ // Remove this empty chunk entry from free list to used list
				OS_ASSERT(pMmChunkEntry->MmEntryList.NumOfNodes == 0) ;
				OS_ASSERT( OS_IsFlagSet( pMmChunkEntry->Flag, MM_CHUNK_FLAG_FREE_LIST) == TRUE);

				// Remove it from Free List Anchor
				OS_ClearFlag( pMmChunkEntry->Flag, MM_CHUNK_FLAG_FREE_LIST);
				RemoveEntryList( &pMmChunkEntry->MmChunkLink );
				pMM_anchor->FreeList.NumOfNodes --;

				// Add it into Used List Anchor
				OS_SetFlag( pMmChunkEntry->Flag, MM_CHUNK_FLAG_USED_LIST);
				InsertTailList( &pMM_anchor->UsedList.ListEntry, &pMmChunkEntry->MmChunkLink );
				pMM_anchor->UsedList.NumOfNodes ++;
			}

			if ( i >= numOfPages)
				break;	// we are done;
		} // for :Scan thru Chunk entry freelist

		if ( i < numOfPages)
		{ // We have not got enough page, No memory to satisfy request !!!
			DebugPrint((DBG_ERROR, "mm_alloc: Non AWE : No more Pages available !!! Number of Free nodes  %d!!! \n",
												pMM_anchor->FreeList.NumOfNodes));
			OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);
			goto done;	// failed
		}
	} // else : AWE is NOT used, Virtual Alloc is used 

	OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);

	OS_ClearFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED);
	pMmHolder->SystemVAddr = NULL;

	if (LockedAndMapedVA == TRUE)
	{ // Locked MDL And Mapped VA for current Allocation
		try 
		{
			// get system virtual address to unlock
			pMmHolder->SystemVAddr = MmMapLockedPages(pMmHolder->Mdl, KernelMode);		// Alternative calls : MmGetSystemAddressForMdlSafe 
			OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED);

			GSftk_Config.Mmgr.MM_TotalNumOfMdlLocked	+= 1;
			GSftk_Config.Mmgr.MM_TotalSizeOfMdlLocked	+= Size;

			GSftk_Config.Mmgr.MM_TotalNumOfMdlLockedAtPresent  += 1;
			GSftk_Config.Mmgr.MM_TotalSizeOfMdlLockedAtPresent += Size;
		}
		except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
		{
			status = GetExceptionCode();	
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
										0, __FILE__, __LINE__, GetExceptionCode());

			DebugPrint((DBG_ERROR, "FIXME FIXME mm_alloc: MmMapLockedPages( Mdl 0x%08x, Size %d) Failed with exception code 0x%08x!!! error 0x%08x !!! \n",
											pMmHolder->Mdl, Size, status, status));
			goto done;
		}
	} // Locked MDL And Mapped VA for current Allocation

	pMemory = pMmHolder;	// success

	if (pMmHolder->SystemVAddr)
	{
		OS_ZeroMemory( pMmHolder->SystemVAddr, Size);
	}
	pMmHolder->Size = Size;

done:
	if (pMemory == NULL)
	{ // Failed : Do Cleanup
		if (pMmHolder)
		{
			if (pMmHolder->Mdl)
			{
				if ( OS_IsFlagSet( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED) == TRUE )
				{
					MmUnmapLockedPages( pMmHolder->SystemVAddr, pMmHolder->Mdl);
					OS_ClearFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED);
					pMmHolder->SystemVAddr = NULL;
				}
				IoFreeMdl(pMmHolder->Mdl);
				pMmHolder->Mdl = NULL;
			}

			OS_ACQUIRE_LOCK( &pMM_anchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

			if (GSftk_Config.Mmgr.AWEUsed == TRUE)
			{ // if: AWE is used 
				while( !(IsListEmpty(&pMmHolder->MmEntryList.ListEntry)) )
				{ // While : Scan thru local list and free back MM slab
					pListEntry = RemoveHeadList(&pMmHolder->MmEntryList.ListEntry);
					pMmHolder->MmEntryList.NumOfNodes --;

					pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink);

					// insert it back to Free list
					InsertTailList( &pMM_anchor->FreeList.ListEntry, &pMmEntry->MmEntryLink );
					pMM_anchor->FreeList.NumOfNodes ++;
				}
			}
			else
			{ // else : AWE is NOT used, Virtual Alloc is used 
				while( !(IsListEmpty(&pMmHolder->MmEntryList.ListEntry)) )
				{ // While : Scan thru local list and free back MM slab
					pListEntry = RemoveHeadList(&pMmHolder->MmEntryList.ListEntry);
					pMmHolder->MmEntryList.NumOfNodes --;

					pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink);

					// InitializeListHead( &pMmEntry->MmEntryLink );
					OS_ASSERT(pMmEntry->PageEntry != NULL);

					pMmChunkEntry = pMmEntry->ChunkEntry;

					if (OS_IsFlagSet( pMmChunkEntry->Flag, MM_CHUNK_FLAG_USED_LIST) == TRUE)
					{ // First remove its chunk entry from Used list and than add into FreeList
						OS_ClearFlag( pMmChunkEntry->Flag, MM_CHUNK_FLAG_USED_LIST);
						RemoveEntryList( &pMmChunkEntry->MmChunkLink );
						pMM_anchor->UsedList.NumOfNodes --;

						// Add it into Free List Anchor
						OS_SetFlag( pMmChunkEntry->Flag, MM_CHUNK_FLAG_FREE_LIST);
						InsertHeadList( &pMM_anchor->FreeList.ListEntry, &pMmChunkEntry->MmChunkLink );
						pMM_anchor->FreeList.NumOfNodes ++;
					}

					// insert current mm_entry back to its chunkentry anchor
					InsertTailList( &pMmChunkEntry->MmEntryList.ListEntry, &pMmEntry->MmEntryLink );
					pMmChunkEntry->MmEntryList.NumOfNodes ++;

					if (pMmChunkEntry->MmEntryList.NumOfNodes == pMM_anchor->NumOfMMEntryPerChunk)
						pMM_anchor->TotalNumberOfRawWFreeNodes ++;	// since we are removing some entry from it
				}
			} // else : AWE is NOT used, Virtual Alloc is used 

			OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);
				
			mm_type_free(pMmHolder, MM_TYPE_MM_HOLDER);
		}
	} // Failed : Do Cleanup
	else
	{ // successed
		pMM_anchor->TotalNumberOfPagesInUse += numOfPages;
	}
	
	// Now Here calculate Memory dynamic Allocation requirement, if needed post msg to allocate more memory
	bAllocMemFromService = MM_IsAllocMemFromServiceNeeded( pMM_anchor->TotalMemSize, 
									(pMM_anchor->TotalNumberOfPagesInUse * GSftk_Config.Mmgr.PageSize),
									GSftk_Config.Mmgr.AllocThreshold);

	if (bAllocMemFromService == TRUE)
	{ // first check if already allocated max memory
		if ( (GSftk_Config.Mmgr.TotalMemAllocated / ONEMILLION) < GSftk_Config.Mmgr.MaxAllocatePhysMemInMB)
		{ // we can allocate more memory
			SM_AddCmd_ForMM(SM_Event_Alloc);
		}
	}

	return pMemory;
} // mm_alloc()

//
//  VOID
//  MM_COPY_BUFFER(	PMM_HOLDER _pMmHolder_, PVOID	_SrcBuffer_, ULONG _Size_, BOOLEAN CopyFromSrcBuffer );
//	
//	Copy Buffer from SrcBuffer into MM_HOLDER SystemVAddr memory for specified Size
//
NTSTATUS
MM_COPY_BUFFER(	PMM_HOLDER MmHolder, PVOID	SrcBuffer, ULONG Size, BOOLEAN CopyFromSrcBuffer )
{
	NTSTATUS status = STATUS_SUCCESS;

	if ( OS_IsFlagSet( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED) == FALSE )
	{ // Locked and mapped Pages to get system virtual address space
		OS_ASSERT(MmHolder->SystemVAddr == NULL);
		try 
		{ // get system virtual address to unlock
			MmHolder->SystemVAddr = MmMapLockedPages( MmHolder->Mdl, KernelMode);		// Alternative calls : MmGetSystemAddressForMdlSafe 
			OS_SetFlag( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED);

			// update stats
			GSftk_Config.Mmgr.MM_TotalNumOfMdlLocked	+= 1;
			GSftk_Config.Mmgr.MM_TotalSizeOfMdlLocked	+= MmHolder->Size;

			GSftk_Config.Mmgr.MM_TotalNumOfMdlLockedAtPresent  += 1;
			GSftk_Config.Mmgr.MM_TotalSizeOfMdlLockedAtPresent += MmHolder->Size;
		}
		except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
		{
			status = GetExceptionCode();	
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
										0, __FILE__, __LINE__, GetExceptionCode());

			DebugPrint((DBG_ERROR, "FIXME FIXME MM_COPY_BUFFER: MmMapLockedPages( Mdl 0x%08x, Size %d) Failed with exception code 0x%08x!!! error 0x%08x !!! \n",
											MmHolder->Mdl, Size, status, status));
			if (status == STATUS_SUCCESS)
			{
				DebugPrint((DBG_ERROR, "FIXME FIXME MM_COPY_BUFFER: Exception returned with status success !! MmMapLockedPages( Mdl 0x%08x, Size %d) Failed with exception code 0x%08x!!! error 0x%08x !!! \n",
											MmHolder->Mdl, Size, status, status));
				OS_ASSERT(FALSE);
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
			goto done;
		}
	} // Locked and mapped Pages to get system virtual address space

	// copy data now
	if( MmHolder->SystemVAddr)	
	{		
		if (CopyFromSrcBuffer == TRUE)
			OS_RtlCopyMemory( MmHolder->SystemVAddr, SrcBuffer, Size);
		else
			OS_RtlCopyMemory( SrcBuffer, MmHolder->SystemVAddr, Size);	
	}										
	else	
	{	
		OS_ASSERT(OS_IsFlagSet( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED) == FALSE);
		OS_ASSERT(FALSE);
		status = STATUS_INSUFFICIENT_RESOURCES;
	}	
	
done:
	// Always Unlock MDL pages and unmaped VA after use, do not hold this for longer, 
	// causes otherwise system will panic or give STATUS_INSUFFICIENT_RESOURCES for resources...
	if ( OS_IsFlagSet( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED) == TRUE )
	{ 
		OS_ASSERT( MmHolder->SystemVAddr != NULL);

		MmUnmapLockedPages( MmHolder->SystemVAddr, MmHolder->Mdl);
		OS_ClearFlag( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED);
		MmHolder->SystemVAddr = NULL;

		GSftk_Config.Mmgr.MM_TotalNumOfMdlLockedAtPresent  -= 1;
		GSftk_Config.Mmgr.MM_TotalSizeOfMdlLockedAtPresent -= MmHolder->Size;
	}

	return status;	// on failure caller must free MM_Holder...
} // MM_COPY_BUFFER()

// Locked Memory API will lock Mdl and get system virtual address which is stored inside MM_HOLDEr
// Caller can use MM_HOLDER->MDl Directly or MM_Holder->SystemVAddr, use memory size as MM_HOLDER->Size
NTSTATUS
MM_Locked_memory(	PMM_HOLDER MmHolder )
{
	NTSTATUS status = STATUS_SUCCESS;

	if ( OS_IsFlagSet( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED) == FALSE )
	{ // Locked and mapped Pages to get system virtual address space
		OS_ASSERT(MmHolder->SystemVAddr == NULL);
		try 
		{ // get system virtual address to unlock
			MmHolder->SystemVAddr = MmMapLockedPages( MmHolder->Mdl, KernelMode);		// Alternative calls : MmGetSystemAddressForMdlSafe 
			OS_SetFlag( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED);

			// update stats
			GSftk_Config.Mmgr.MM_TotalNumOfMdlLocked	+= 1;
			GSftk_Config.Mmgr.MM_TotalSizeOfMdlLocked	+= MmHolder->Size;

			GSftk_Config.Mmgr.MM_TotalNumOfMdlLockedAtPresent  += 1;
			GSftk_Config.Mmgr.MM_TotalSizeOfMdlLockedAtPresent += MmHolder->Size;
		}
		except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
		{
			status = GetExceptionCode();	
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
										0, __FILE__, __LINE__, GetExceptionCode());

			DebugPrint((DBG_ERROR, "FIXME FIXME MM_COPY_BUFFER: MmMapLockedPages( Mdl 0x%08x, Size %d) Failed with exception code 0x%08x!!! error 0x%08x !!! \n",
											MmHolder->Mdl, MmHolder->Size, status, status));
			if (status == STATUS_SUCCESS)
			{
				DebugPrint((DBG_ERROR, "FIXME FIXME MM_COPY_BUFFER: Exception returned with status success !! MmMapLockedPages( Mdl 0x%08x, Size %d) Failed with exception code 0x%08x!!! error 0x%08x !!! \n",
											MmHolder->Mdl, MmHolder->Size, status, status));
				OS_ASSERT(FALSE);
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
		}
	} // Locked and mapped Pages to get system virtual address space

	return status;
} // MM_Locked_memory()

// UnLocked Memory API will Unlock Mdl and Make system virtual address to NULL inside MM_HOLDER
// Caller can not use MM_HOLDER->MDl Directly or MM_Holder->SystemVAddr, since its NULL now
// but MM_HOLDER->size field is till have valid value, and data is also valid.
NTSTATUS
MM_UnLocked_memory(	PMM_HOLDER MmHolder )
{
	NTSTATUS status = STATUS_SUCCESS;

	if ( OS_IsFlagSet( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED) == TRUE )
	{ 
		OS_ASSERT( MmHolder->SystemVAddr != NULL);

		MmUnmapLockedPages( MmHolder->SystemVAddr, MmHolder->Mdl);
		OS_ClearFlag( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED);
		MmHolder->SystemVAddr = NULL;

		GSftk_Config.Mmgr.MM_TotalNumOfMdlLockedAtPresent  -= 1;
		GSftk_Config.Mmgr.MM_TotalSizeOfMdlLockedAtPresent -= MmHolder->Size;
	}
	else
	{
		OS_ASSERT(MmHolder->SystemVAddr == NULL);
	}
	return status;
} // MM_UnLocked_memory()

VOID
mm_free( PMM_HOLDER MmHolder)
{
	PMM_ANCHOR			pMM_anchor	= &GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM];
	PMM_CHUNK_ENTRY		pMmChunkEntry;
	PMM_ENTRY			pMmEntry;
	PLIST_ENTRY			pListEntry;
	ULONG				numOfPages = 0;
	BOOLEAN				bFreeMemFromService = FALSE;

	if (MmHolder == NULL)
	{
		DebugPrint((DBG_ERROR, "BUF FIXME FIXME mm_free: Invalid Parameter 0x%08x, BUG Fix Caller Code FIXME FIXME !!! \n",
										MmHolder));
		OS_ASSERT(FALSE);
		return;
	}

	if (GSftk_Config.Mmgr.MM_Initialized == FALSE)
	{ // MMMgr not started 
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_free: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										GSftk_Config.Mmgr.MM_Initialized, NULL));
		OS_ASSERT(FALSE);
		return;
	} // MMMgr not started 

	// OS_ASSERT(MmHolder->Mdl != NULL);
	if (MmHolder->Mdl)
	{
		OS_ASSERT(MmHolder->Size > 0);

		if ( OS_IsFlagSet( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED) == TRUE )
		{
			OS_ASSERT( MmHolder->SystemVAddr != NULL);

			MmUnmapLockedPages( MmHolder->SystemVAddr, MmHolder->Mdl);
			OS_ClearFlag( MmHolder->FlagLink, MM_HOLDER_FLAG_LINK_MDL_LOCKED);
			MmHolder->SystemVAddr = NULL;

			GSftk_Config.Mmgr.MM_TotalNumOfMdlLockedAtPresent  -= 1;
			GSftk_Config.Mmgr.MM_TotalSizeOfMdlLockedAtPresent -= MmHolder->Size;
		}

		IoFreeMdl(MmHolder->Mdl);
		MmHolder->Mdl = NULL;
	}
	
	OS_ACQUIRE_LOCK( &pMM_anchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	if (GSftk_Config.Mmgr.AWEUsed == TRUE)
	{ // if: AWE is used 
		while( !(IsListEmpty(&MmHolder->MmEntryList.ListEntry)) )
		{ // While : Scan thru local list and free back MM slab
			pListEntry = RemoveHeadList(&MmHolder->MmEntryList.ListEntry);
			MmHolder->MmEntryList.NumOfNodes --;

			pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink);

			// insert it back to Free list
			InsertTailList( &pMM_anchor->FreeList.ListEntry, &pMmEntry->MmEntryLink );
			pMM_anchor->FreeList.NumOfNodes ++;
			numOfPages ++;
		}
	}
	else
	{ // else : AWE is NOT used, Virtual Alloc is used 
		while( !(IsListEmpty(&MmHolder->MmEntryList.ListEntry)) )
		{ // While : Scan thru local list and free back MM slab
			pListEntry = RemoveHeadList(&MmHolder->MmEntryList.ListEntry);
			MmHolder->MmEntryList.NumOfNodes --;

			pMmEntry = CONTAINING_RECORD( pListEntry, MM_ENTRY, MmEntryLink);

			// InitializeListHead( &pMmEntry->MmEntryLink );
			OS_ASSERT(pMmEntry->PageEntry != NULL);

			pMmChunkEntry = pMmEntry->ChunkEntry;

			if (OS_IsFlagSet( pMmChunkEntry->Flag, MM_CHUNK_FLAG_USED_LIST) == TRUE)
			{ // First remove its chunk entry from Used list and than add into FreeList
				OS_ClearFlag( pMmChunkEntry->Flag, MM_CHUNK_FLAG_USED_LIST);
				RemoveEntryList( &pMmChunkEntry->MmChunkLink );
				pMM_anchor->UsedList.NumOfNodes --;

				// Add it into Free List Anchor
				OS_SetFlag( pMmChunkEntry->Flag, MM_CHUNK_FLAG_FREE_LIST);
				InsertHeadList( &pMM_anchor->FreeList.ListEntry, &pMmChunkEntry->MmChunkLink );
				pMM_anchor->FreeList.NumOfNodes ++;
			}

			// insert current mm_entry back to its chunkentry anchor
			InsertTailList( &pMmChunkEntry->MmEntryList.ListEntry, &pMmEntry->MmEntryLink );
			pMmChunkEntry->MmEntryList.NumOfNodes ++;
			numOfPages ++;

			if (pMmChunkEntry->MmEntryList.NumOfNodes == pMM_anchor->NumOfMMEntryPerChunk)
				pMM_anchor->TotalNumberOfRawWFreeNodes ++;	// since we are removing some entry from it
		}
	} // else : AWE is NOT used, Virtual Alloc is used 

	OS_RELEASE_LOCK( &pMM_anchor->Lock, NULL);
		
	mm_type_free(MmHolder, MM_TYPE_MM_HOLDER);

	pMM_anchor->TotalNumberOfPagesInUse -= numOfPages;

	// Now Here calculate Memory dynamic Free requirement, if needed post msg to Free more memory
	if (pMM_anchor->TotalMemSize > (GSftk_Config.Mmgr.IncrementAllocationChunkSizeInMB * 2 * ONEMILLION) )
	{ // check for Dynamic free requirement
		ULONGLONG	IncrAllocationSize = (GSftk_Config.Mmgr.IncrementAllocationChunkSizeInMB * ONEMILLION);
		ULONGLONG	TotalMemUsed, LeaveUnusedFreeThresholdMem;
		
		TotalMemUsed = (pMM_anchor->TotalNumberOfPagesInUse * GSftk_Config.Mmgr.PageSize);

		if (TotalMemUsed < (pMM_anchor->TotalMemSize - IncrAllocationSize) )
		{
			LeaveUnusedFreeThresholdMem = ( (IncrAllocationSize * GSftk_Config.Mmgr.FreeThreshold) / 100);

			if (TotalMemUsed < ((pMM_anchor->TotalMemSize - IncrAllocationSize) - LeaveUnusedFreeThresholdMem))
				bFreeMemFromService  = TRUE;
		}
	}

	if (bFreeMemFromService == TRUE)
	{ // first check if already allocated max memory
		SM_AddCmd_ForMM(SM_Event_Free);
	}

	return;
} // mm_free()

VOID
mm_free_ProtoHdr( PSFTK_LG	Sftk_Lg, PMM_PROTO_HDR MmProtoHdr)
{
	PMM_SOFT_HDR	pMmSoftHdr;
	PLIST_ENTRY		plistEntry;

	// TODO: Per LG, add logic to alloc maximum threshold memory, Check here that threshold !! 
	OS_ASSERT(OS_IsFlagSet( MmProtoHdr->Flag, MM_FLAG_PROTO_HDR) == TRUE);

	if ( MM_IsEventValidInMMProtoHdr(MmProtoHdr) )
	{	// Synchronous command so signal this completion
		// pEvent = MM_GetEventFromMMProtoHdr( pMMmigrateProtoHdr );
		MM_SignalEventInMMProtoHdr(MmProtoHdr);
		// No need to free this packet, Outband Caller will get signalled and then process it and free it later....
		return;	// Outband pkt send caller will ree this packet....we can not free it here
	}

	// OS_ASSERT(pMmProtoHdr->MmSoftHdrList.NumOfNodes == 0);
	while( !(IsListEmpty(&MmProtoHdr->MmSoftHdrList.ListEntry)) )
	{ // While : Free All SoftHdr under ProtoHDr if presence 
		plistEntry = RemoveHeadList(&MmProtoHdr->MmSoftHdrList.ListEntry);
		MmProtoHdr->MmSoftHdrList.NumOfNodes --;

		pMmSoftHdr = CONTAINING_RECORD( plistEntry, MM_SOFT_HDR, MmSoftHdrLink);

		OS_ASSERT(OS_IsFlagSet( pMmSoftHdr->Flag, MM_FLAG_SOFT_HDR) == TRUE);

		mm_type_free(pMmSoftHdr, MM_TYPE_SOFT_HEADER);
	}
	
	OS_ASSERT(MmProtoHdr->MmSoftHdrList.NumOfNodes == 0);

	if (MmProtoHdr->RetProtoHDr != NULL)
	{ // Free Ret Proto Hdr 
		PMM_PROTO_HDR pMmRetProtoHdr = MmProtoHdr->RetProtoHDr;
		while( !(IsListEmpty(&pMmRetProtoHdr->MmSoftHdrList.ListEntry)) )
		{ // While : Free All SoftHdr under ProtoHDr if presence 
			plistEntry = RemoveHeadList(&pMmRetProtoHdr->MmSoftHdrList.ListEntry);
			pMmRetProtoHdr->MmSoftHdrList.NumOfNodes --;

			pMmSoftHdr = CONTAINING_RECORD( plistEntry, MM_SOFT_HDR, MmSoftHdrLink);

			OS_ASSERT(OS_IsFlagSet( pMmSoftHdr->Flag, MM_FLAG_SOFT_HDR) == TRUE);

			mm_type_free(pMmSoftHdr, MM_TYPE_SOFT_HEADER);
		}
		OS_ASSERT(pMmRetProtoHdr->MmSoftHdrList.NumOfNodes == 0);
		mm_type_free( pMmRetProtoHdr, MM_TYPE_PROTOCOL_HEADER);
		MmProtoHdr->RetProtoHDr = NULL;
	}
	
	if (MmProtoHdr->RetBuffer != NULL)
	{
		OS_FreeMemory( MmProtoHdr->RetBuffer) ;
		MmProtoHdr->RetBuffer = NULL;
	}

	mm_type_free(MmProtoHdr, MM_TYPE_PROTOCOL_HEADER);
	return;
} // mm_free_ProtoHdr()

PMM_HOLDER
mm_alloc_buffer( PSFTK_LG Sftk_Lg, ULONG Size, HDR_TYPE Hdr_Type, BOOLEAN LockedAndMapedVA)
{
	PVOID					pMemory		= NULL;
	PMM_HOLDER				pMmHolder	= NULL;
	PMM_SOFT_HDR			pMmSoftHdr	= NULL;
	PMM_PROTO_HDR			pMmProtoHdr = NULL;

	if (GSftk_Config.Mmgr.MM_Initialized == FALSE)
	{ // MMMgr not started 
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_alloc_buffer: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										GSftk_Config.Mmgr.MM_Initialized, NULL));
		return NULL;
	} // MMMgr not started 

	// TODO: Per LG, add logic to alloc maximum threshold memory, Check here that threshold !! 
	pMmHolder = mm_alloc(Size, LockedAndMapedVA);
	if (!pMmHolder)
	{
		DebugPrint((DBG_ERROR, "mm_alloc_buffer: mm_alloc(size %d) Failed, returning NULL to caller !!! \n",
										Size));
		goto done;
	}

	// Now Allocate Soft_hdr or Proto_hdr depdns on HDR_TYPE specifies
	switch(Hdr_Type)
	{
		case SOFT_HDR:
				pMmSoftHdr = mm_type_alloc(MM_TYPE_SOFT_HEADER);
				if (pMmSoftHdr == NULL)
				{ // Failed :should not happened
					DebugPrint((DBG_ERROR, "mm_alloc_buffer: mm_type_alloc(MM_TYPE_SOFT_HEADER %d) Failed !! Allocation from OS Also Failed Returning Error!! \n",
														MM_TYPE_SOFT_HEADER));
					goto done;
				}
				// Init MMHolder with protoHdr information
				pMmHolder->pProtocolHdr = pMmSoftHdr;
				OS_SetFlag( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_SOFT_HDR);

				// update stats
				if (Sftk_Lg)
				{
					Sftk_Lg->Statistics.MM_LgTotalMemUsed += sizeof(MM_SOFT_HDR);
					Sftk_Lg->Statistics.MM_LgMemUsed += sizeof(MM_SOFT_HDR);
				}
				break;

		case PROTO_HDR:
				pMmProtoHdr = mm_type_alloc(MM_TYPE_PROTOCOL_HEADER);
				if (pMmProtoHdr == NULL)
				{ // Failed :should not happened
					DebugPrint((DBG_ERROR, "mm_alloc_buffer: mm_type_alloc(MM_TYPE_PROTOCOL_HEADER %d) Failed !! Allocation from OS Also Failed Returning Error !! \n",
														MM_TYPE_PROTOCOL_HEADER));
					goto done;
				}
				// Init MMHolder with protoHdr information
				pMmHolder->pProtocolHdr = pMmProtoHdr;
				OS_SetFlag( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_PROTO_HDR);

				// update stats
				if (Sftk_Lg)
				{
					Sftk_Lg->Statistics.MM_LgTotalMemUsed += sizeof(MM_PROTO_HDR);
					Sftk_Lg->Statistics.MM_LgMemUsed += sizeof(MM_PROTO_HDR);
				}
				break;

		default:
				DebugPrint((DBG_ERROR, "mm_alloc_buffer: HDr_Type %d: is undefined, Returning without any proto Hdr or Soft Hdr !! \n",
														Hdr_Type));
				break;

	}
	// caller will Initialize Hdr with proper information
	// Caller will Insert it into appropriate queue as specified
	pMemory	= pMmHolder;	// success

	// update stats
	if (Sftk_Lg)
	{
		Sftk_Lg->Statistics.MM_LgTotalMemUsed += pMmHolder->Size;
		Sftk_Lg->Statistics.MM_LgMemUsed += pMmHolder->Size;

		Sftk_Lg->Statistics.MM_LgTotalRawMemUsed += pMmHolder->Size;
		Sftk_Lg->Statistics.MM_LgRawMemUsed += pMmHolder->Size;
	}

	OS_ASSERT(pMmHolder->Mdl != NULL);
	OS_ASSERT(pMmHolder->SystemVAddr != NULL);

done:
	if (pMemory == NULL)
	{ // Failed: Do clean up 
		if (pMmHolder)
			mm_free(pMmHolder);

		if (pMmSoftHdr)
			mm_type_free(pMmSoftHdr, MM_TYPE_SOFT_HEADER);

		if (pMmProtoHdr)
			mm_type_free(pMmProtoHdr, MM_TYPE_PROTOCOL_HEADER);
	}
	return pMemory;
} // mm_alloc_buffer()

VOID
mm_free_buffer( PSFTK_LG	Sftk_Lg, PMM_HOLDER MmHolder)
{
	PMM_SOFT_HDR			pMmSoftHdr	= NULL;
	PMM_PROTO_HDR			pMmProtoHdr = NULL;

	// TODO: Per LG, add logic to alloc maximum threshold memory, Check here that threshold !! 
	if (GSftk_Config.Mmgr.MM_Initialized == FALSE)
	{ // MMMgr not started 
		DebugPrint((DBG_ERROR, "FIXE FIXME :: mm_free_buffer: Mmgr->MM_Initialized %d == FALSE, Can't do anything !!! returning error 0x%08x !!! \n",
										GSftk_Config.Mmgr.MM_Initialized, NULL));
		OS_ASSERT(FALSE);
		return;
	} // MMMgr not started 

	
	// Now Free Soft_hdr or Proto_hdr depdns on HDR_TYPE specifies
	if (OS_IsFlagSet( MmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_PROTO_HDR) )
	{
		pMmProtoHdr = MmHolder->pProtocolHdr;
		mm_free_ProtoHdr(Sftk_Lg, pMmProtoHdr);

		Sftk_Lg->Statistics.MM_LgMemUsed -= sizeof(MM_PROTO_HDR);
		goto done;
	}

	if (OS_IsFlagSet( MmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_SOFT_HDR) )
	{ // else its Soft Hdr
		pMmSoftHdr = MmHolder->pProtocolHdr;
		OS_ASSERT(OS_IsFlagSet( pMmSoftHdr->Flag, MM_FLAG_SOFT_HDR) == TRUE);

		Sftk_Lg->Statistics.MM_LgMemUsed -= sizeof(MM_SOFT_HDR);

		mm_type_free(pMmSoftHdr, MM_TYPE_SOFT_HEADER);
		goto done;
	}
	
	// update stats
	if (Sftk_Lg)
	{
		Sftk_Lg->Statistics.MM_LgMemUsed -= MmHolder->Size;
		Sftk_Lg->Statistics.MM_LgRawMemUsed -= MmHolder->Size;
	}
done:
	mm_free(MmHolder);
	return;
} // mm_free_buffer()

NTSTATUS
mm_reserve_buffer_for_refresh( PSFTK_LG Sftk_Lg )
{
	ULONG		i;	
	PMM_HOLDER	pMM_Buffer;
	NTSTATUS	status = STATUS_SUCCESS;

	OS_ACQUIRE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	
	if (Sftk_Lg->ReserveIsActive == TRUE)
	{
		DebugPrint((DBG_ERROR, "mm_reserve_buffer_for_refresh:: Reserve Is Already done before, Lg %d NumOfAllocatedRefreshIOPkts %d RefreshIOPkts.NumOfNodes %d !! FIXME FIXME \n", 
									Sftk_Lg->LGroupNumber, Sftk_Lg->NumOfAllocatedRefreshIOPkts, Sftk_Lg->RefreshIOPkts.NumOfNodes));
		OS_ASSERT(FALSE);
		goto done;
	}

	OS_ASSERT( Sftk_Lg->NumOfAsyncRefreshIO > 0 );
	OS_ASSERT( Sftk_Lg->MaxTransferUnit > 0 );
	OS_ASSERT( Sftk_Lg->NumOfAllocatedRefreshIOPkts == 0);
	OS_ASSERT( Sftk_Lg->RefreshIOPkts.NumOfNodes == 0);
	
	ANCHOR_InitializeListHead( Sftk_Lg->RefreshIOPkts );

	for (i=0; i < Sftk_Lg->NumOfAsyncRefreshIO; i++)
	{ // for : allocate Sftk_Lg->NumOfAsyncRefreshIO number of Data buffer pkts for refresh work for specified LG
		pMM_Buffer = mm_alloc_buffer( Sftk_Lg, Sftk_Lg->MaxTransferUnit, PROTO_HDR, TRUE); 

		if (pMM_Buffer == NULL)
		{ // MM/BAB OverFlow !!!
			DebugPrint((DBG_ERROR, "mm_reserve_buffer_for_refresh:: mm_alloc_buffer(LG %d Size %d) return NULL \n", 
									Sftk_Lg->LGroupNumber, Sftk_Lg->MaxTransferUnit));

			if (i == 0 )
			{
				DebugPrint((DBG_ERROR, "BUG FIXME FIXME mm_reserve_buffer_for_refresh:: (i==0) mm_alloc_buffer(LG %d Size %d) BUG FIXME FIXME  \n", 
									Sftk_Lg->LGroupNumber, Sftk_Lg->MaxTransferUnit));

				// TODO :Allocate Memory from OS for MM_Holder + Data Buffer + Proto Hdr
				DebugPrint((DBG_ERROR, "BUG FIXME FIXME mm_reserve_buffer_for_refresh:: TODO : Allocate atleast one pkt from OS Directly so caller can Do Refresh !!! TODO BUG :: FIXME FIXME  \n"));

				OS_ASSERT(FALSE);

				status = STATUS_INSUFFICIENT_RESOURCES;
				goto done;
			}
			else
			{
				DebugPrint((DBG_ERROR, "mm_reserve_buffer_for_refresh:: Asking Number to allocate Refresh Async Pkts %d, returning only %d pkts to caller, Low Memory Situations \n", Sftk_Lg->NumOfAsyncRefreshIO, i));
				break;
			}
		} // MM/BAB OverFlow !!!

		InsertTailList( &Sftk_Lg->RefreshIOPkts.ListEntry, &pMM_Buffer->MmHolderLink );
		Sftk_Lg->RefreshIOPkts.NumOfNodes ++;
	} // for : allocate Sftk_Lg->NumOfAsyncRefreshIO number of Data buffer pkts for refresh work for specified LG

	Sftk_Lg->NumOfAllocatedRefreshIOPkts= Sftk_Lg->RefreshIOPkts.NumOfNodes;
	Sftk_Lg->WaitingRefreshNextBuffer	= FALSE;
	Sftk_Lg->RefreshNextBuffer			= NULL;

	KeClearEvent( &Sftk_Lg->RefreshPktsWaitEvent );
	KeClearEvent( &Sftk_Lg->ReleaseFreeAllPktsWaitEvent );
	KeClearEvent( &Sftk_Lg->ReleasePoolDoneEvent );

	Sftk_Lg->ReserveIsActive			= TRUE;
	Sftk_Lg->ReleaseIsWaiting			= FALSE;

	status = STATUS_SUCCESS;
done:
	OS_RELEASE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, NULL);
	return status;
} // mm_reserve_buffer_for_refresh()

NTSTATUS
mm_release_buffer_for_refresh( PSFTK_LG Sftk_Lg )
{
	PMM_HOLDER		pMM_Buffer;
	PLIST_ENTRY		plistEntry;
	NTSTATUS		status = STATUS_SUCCESS;
	LARGE_INTEGER	timeoutwait;

	OS_ACQUIRE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	if (Sftk_Lg->ReserveIsActive == FALSE)
	{
		DebugPrint((DBG_ERROR, "mm_release_buffer_for_refresh:: Reserve was not Done before, Lg %d NumOfAllocatedRefreshIOPkts %d RefreshIOPkts.NumOfNodes %d !! FIXME FIXME \n", 
									Sftk_Lg->LGroupNumber, Sftk_Lg->NumOfAllocatedRefreshIOPkts, Sftk_Lg->RefreshIOPkts.NumOfNodes));
		goto done;
	}

	if (Sftk_Lg->ReleaseIsWaiting == TRUE)
	{
		DebugPrint((DBG_ERROR, "mm_release_buffer_for_refresh:: Someone already called release and waiting to complete, Lg %d NumOfAllocatedRefreshIOPkts %d RefreshIOPkts.NumOfNodes %d !! FIXME FIXME \n", 
									Sftk_Lg->LGroupNumber, Sftk_Lg->NumOfAllocatedRefreshIOPkts, Sftk_Lg->RefreshIOPkts.NumOfNodes));
		OS_ASSERT(FALSE);
		goto done;
	}

	OS_ASSERT( Sftk_Lg->NumOfAsyncRefreshIO > 0 );
	OS_ASSERT( Sftk_Lg->MaxTransferUnit > 0 );

	OS_ASSERT( Sftk_Lg->NumOfAllocatedRefreshIOPkts > 0);
	// OS_ASSERT( Sftk_Lg->RefreshIOPkts.NumOfNodes > 0);
	if (Sftk_Lg->NumOfAllocatedRefreshIOPkts != Sftk_Lg->RefreshIOPkts.NumOfNodes)
	{ // we need to wait till all pkts gets free here
		Sftk_Lg->ReleaseIsWaiting = TRUE;
		OS_RELEASE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, NULL);
		DebugPrint((DBG_ERROR, "mm_release_buffer_for_refresh:: Indifinite waiting for ReleaseFreeAllPktsWaitEvent till all pkts gets free, Lg %d NumOfAllocatedRefreshIOPkts %d RefreshIOPkts.NumOfNodes %d !! FIXME FIXME \n", 
									Sftk_Lg->LGroupNumber, Sftk_Lg->NumOfAllocatedRefreshIOPkts, Sftk_Lg->RefreshIOPkts.NumOfNodes));
		do 
		{
			timeoutwait.QuadPart = DEFAULT_TIMEOUT_FOR_RELEASE_POOL_WAIT;

			status = KeWaitForSingleObject(	(PVOID) &Sftk_Lg->ReleaseFreeAllPktsWaitEvent,Executive,
											KernelMode,	FALSE,	&timeoutwait );	// Indifinte waiting..... 
			switch(status)
			{
				case STATUS_SUCCESS:	// state change event
									KeClearEvent( &Sftk_Lg->ReleaseFreeAllPktsWaitEvent); // just be safe clear event
									OS_ASSERT( Sftk_Lg->NumOfAllocatedRefreshIOPkts == Sftk_Lg->RefreshIOPkts.NumOfNodes);
									break;
				case STATUS_TIMEOUT:	// A time out occurred before the specified set of wait conditions was met. 
									DebugPrint((DBG_ERROR, "mm_release_buffer_for_refresh:: LG %d,GetNext Pkt : KeWaitForMultipleObjects(): returned STATUS_TIMEOUT, Wait 0x%I64x !!\n", 
													Sftk_Lg->LGroupNumber, timeoutwait.QuadPart));
				default:			break;
			}

			if( Sftk_Lg->NumOfAllocatedRefreshIOPkts == Sftk_Lg->RefreshIOPkts.NumOfNodes)
			{
				if (status != STATUS_SUCCESS)
				{
					DebugPrint((DBG_ERROR, "FIXME FIXME mm_release_buffer_for_refresh:: LG %d,All Pkts Free ! but we did not hit signal event !!! FIXME FIXME!!\n", 
													Sftk_Lg->LGroupNumber));
					status = STATUS_SUCCESS;
					OS_ASSERT(FALSE);
				}
				break;
			}
		} while (status != STATUS_SUCCESS);

		OS_ACQUIRE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	}
	OS_ASSERT( Sftk_Lg->NumOfAllocatedRefreshIOPkts == Sftk_Lg->RefreshIOPkts.NumOfNodes);

	while( !(IsListEmpty(&Sftk_Lg->RefreshIOPkts.ListEntry)) )
	{ // While : Unlock & Free All pkts
		plistEntry = RemoveHeadList(&Sftk_Lg->RefreshIOPkts.ListEntry);
		Sftk_Lg->RefreshIOPkts.NumOfNodes --;

		pMM_Buffer = CONTAINING_RECORD( plistEntry, MM_HOLDER, MmHolderLink);
		
		mm_free_buffer( Sftk_Lg, pMM_Buffer);
	}

	Sftk_Lg->NumOfAllocatedRefreshIOPkts = 0;
	Sftk_Lg->RefreshIOPkts.NumOfNodes	= 0;
	ANCHOR_InitializeListHead( Sftk_Lg->RefreshIOPkts );

	Sftk_Lg->WaitingRefreshNextBuffer	= FALSE;
	Sftk_Lg->RefreshNextBuffer			= NULL;
	Sftk_Lg->ReserveIsActive			= FALSE;
	Sftk_Lg->ReleaseIsWaiting			= FALSE;
	KeClearEvent( &Sftk_Lg->RefreshPktsWaitEvent );
	KeClearEvent( &Sftk_Lg->ReleaseFreeAllPktsWaitEvent );
	KeSetEvent( &Sftk_Lg->ReleasePoolDoneEvent, 0, FALSE );

	status = STATUS_SUCCESS;
done:
	OS_RELEASE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, NULL);
	return status;
} // mm_release_buffer_for_refresh()

NTSTATUS
mm_get_buffer_from_refresh_pool( PSFTK_LG Sftk_Lg, PMM_HOLDER *MmHolder)
{
	PLIST_ENTRY		plistEntry;
	NTSTATUS		status			= STATUS_SUCCESS;
	BOOLEAN			bExitWaitloop	= FALSE;
	ULONG			noOfEvents;
	PVOID			eventsList[2];	// Cache wait event array
	LARGE_INTEGER	getnextpktTimeout;

	*MmHolder = NULL;
	do 
	{
		OS_ACQUIRE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
		if (Sftk_Lg->ReleaseIsWaiting == TRUE)
		{ // return STATUS_UNSUCCESSFUL if Release is in wait ....
			DebugPrint((DBG_ERROR, "mm_get_next_buffer_from_refresh_pool:: Someone already called release so returning Error, Lg %d NumOfAllocatedRefreshIOPkts %d RefreshIOPkts.NumOfNodes %d !! \n", 
										Sftk_Lg->LGroupNumber, Sftk_Lg->NumOfAllocatedRefreshIOPkts, Sftk_Lg->RefreshIOPkts.NumOfNodes));
			status = STATUS_UNSUCCESSFUL;
			goto done;
		}

		if (!IsListEmpty(&Sftk_Lg->RefreshIOPkts.ListEntry) )
		{ // Empty buffer exist in anchor, retrieved it and return
			plistEntry = RemoveHeadList(&Sftk_Lg->RefreshIOPkts.ListEntry);
			Sftk_Lg->RefreshIOPkts.NumOfNodes --;

			*MmHolder = CONTAINING_RECORD( plistEntry, MM_HOLDER, MmHolderLink);
			// KeClearEvent( &Sftk_Lg->RefreshPktsWaitEvent); // not needed its Synchronize event 
			status = STATUS_SUCCESS;
			goto done;
		}
		OS_RELEASE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, NULL);

		eventsList[0] = &Sftk_Lg->RefreshStateChangeSemaphore;	// Stat change event Index 0	
		eventsList[1] = &Sftk_Lg->RefreshPktsWaitEvent;	// Cache Bufgfer ready event

		getnextpktTimeout.QuadPart = DEFAULT_TIMEOUT_FOR_REFRESHPOOL_GET_NEXT_PKT;

		noOfEvents = 2;
		// Wait For Data Buffer to be ready else state change, TRUE means data buffer is ready else state got changed
		// anyhow will check state again do necessary things
		status = KeWaitForMultipleObjects(	noOfEvents,
											eventsList,
											WaitAny,
											Executive,
											KernelMode,
											FALSE,
											&getnextpktTimeout,
											NULL );
		switch(status)
		{
			case STATUS_WAIT_0:	// state change event
								bExitWaitloop = TRUE;
								status = STATUS_UNSUCCESSFUL;	// we are done
								KeClearEvent( &Sftk_Lg->RefreshPktsWaitEvent); // just be safe clear event
								break;
			case STATUS_WAIT_1:	// Index 1 got signaled for Sftk_Lg->RefreshPktsWaitEvent
								KeClearEvent( &Sftk_Lg->RefreshPktsWaitEvent); // just be safe clear event
								break;
			case STATUS_TIMEOUT:	// A time out occurred before the specified set of wait conditions was met. 
								DebugPrint((DBG_RTHREAD, "mm_get_buffer_from_refresh_pool:: LG %d,GetNext Pkt : KeWaitForMultipleObjects(): returned STATUS_TIMEOUT, Wait 0x%I64x !!\n", 
												Sftk_Lg->LGroupNumber, getnextpktTimeout.QuadPart));
			default:			break;
		}
	} while(bExitWaitloop == FALSE);

	return status;	// return status error here only

done:
	OS_RELEASE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, NULL);
	return status;
} // mm_get_buffer_from_refresh_pool()

NTSTATUS
mm_get_next_buffer_from_refresh_pool( PSFTK_LG Sftk_Lg )
{
	PLIST_ENTRY	plistEntry;
	NTSTATUS	status = STATUS_SUCCESS;

	OS_ACQUIRE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	if (Sftk_Lg->ReleaseIsWaiting == TRUE)
	{ // return STATUS_UNSUCCESSFUL if Release is in wait ....
		DebugPrint((DBG_ERROR, "mm_get_next_buffer_from_refresh_pool:: Someone already called release so returning Error, Lg %d NumOfAllocatedRefreshIOPkts %d RefreshIOPkts.NumOfNodes %d !! \n", 
									Sftk_Lg->LGroupNumber, Sftk_Lg->NumOfAllocatedRefreshIOPkts, Sftk_Lg->RefreshIOPkts.NumOfNodes));
		OS_ASSERT(FALSE);
		status = STATUS_UNSUCCESSFUL;
	
		if (Sftk_Lg->RefreshNextBuffer != NULL)
		{ // this pkt will never get free, so free it explictly...
			InsertTailList( &Sftk_Lg->RefreshIOPkts.ListEntry, &((PMM_HOLDER) Sftk_Lg->RefreshNextBuffer)->MmHolderLink);
			Sftk_Lg->RefreshIOPkts.NumOfNodes ++;
		}
		goto done;
	}

	if (Sftk_Lg->RefreshNextBuffer != NULL)
	{
		status = STATUS_SUCCESS;
		Sftk_Lg->WaitingRefreshNextBuffer	= FALSE;
		KeClearEvent( &Sftk_Lg->RefreshPktsWaitEvent); // not needed its Synchronize event 
		goto done;
	}

	if (!IsListEmpty(&Sftk_Lg->RefreshIOPkts.ListEntry) )
	{ // Empty buffer exist in anchor, retrieved it and return
		plistEntry = RemoveHeadList(&Sftk_Lg->RefreshIOPkts.ListEntry);
		Sftk_Lg->RefreshIOPkts.NumOfNodes --;

		OS_ASSERT(Sftk_Lg->RefreshNextBuffer == NULL);
		OS_ASSERT(Sftk_Lg->WaitingRefreshNextBuffer == FALSE);

		Sftk_Lg->RefreshNextBuffer = CONTAINING_RECORD( plistEntry, MM_HOLDER, MmHolderLink);
		Sftk_Lg->WaitingRefreshNextBuffer	= FALSE;
		KeClearEvent( &Sftk_Lg->RefreshPktsWaitEvent); // not needed its Synchronize event 
		status = STATUS_SUCCESS;
		goto done;
	}
	// Other wise, Do not have empty node, so return status_pending 

	KeClearEvent( &Sftk_Lg->RefreshPktsWaitEvent); // not needed its Synchronize event 
	Sftk_Lg->RefreshNextBuffer			= NULL;
	Sftk_Lg->WaitingRefreshNextBuffer	= TRUE;
	status = STATUS_PENDING;
done:
	OS_RELEASE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, NULL);
	return status;
} // mm_get_next_buffer_from_refresh_pool()

NTSTATUS
mm_free_buffer_to_refresh_pool( PSFTK_LG Sftk_Lg, PMM_HOLDER MmHolder )
{
	NTSTATUS status = STATUS_SUCCESS;

	OS_ACQUIRE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

#if 1
	InsertTailList( &Sftk_Lg->RefreshIOPkts.ListEntry, &MmHolder->MmHolderLink);
	Sftk_Lg->RefreshIOPkts.NumOfNodes ++;

	if ( (Sftk_Lg->RefreshIOPkts.NumOfNodes == 1) && 
		 (Sftk_Lg->ReleaseIsWaiting == FALSE) )
	{
		KeSetEvent( &Sftk_Lg->RefreshPktsWaitEvent, 0, FALSE);
	}

#else
	/*
	if ( (Sftk_Lg->RefreshNextBuffer == NULL) && 
		 (Sftk_Lg->WaitingRefreshNextBuffer == TRUE) &&
		 (Sftk_Lg->ReleaseIsWaiting == FALSE) )
	{ // Some one waiting for next buffer so just give it right away
		Sftk_Lg->RefreshNextBuffer			= MmHolder;
		Sftk_Lg->WaitingRefreshNextBuffer	= FALSE;
		KeSetEvent( &Sftk_Lg->RefreshPktsWaitEvent, 0, FALSE);
		goto done;
	}
	InsertTailList( &Sftk_Lg->RefreshIOPkts.ListEntry, &MmHolder->MmHolderLink);
	Sftk_Lg->RefreshIOPkts.NumOfNodes ++;
	*/
#endif
	
	if (	(Sftk_Lg->ReleaseIsWaiting == TRUE) && 
			(Sftk_Lg->RefreshIOPkts.NumOfNodes == Sftk_Lg->NumOfAllocatedRefreshIOPkts) )
	{ // Signal release event
#if 0
		/*
		// Must have change state event signal in this case, that's why ASSERT needed to double check
		if (Sftk_Lg->WaitingRefreshNextBuffer == TRUE)
		{
			DebugPrint((DBG_ERROR, "BUG FIXME FIXME mm_free_buffer_to_refresh_pool:: Someone already called release, can't satisy getnext... , Lg %d NumOfAllocatedRefreshIOPkts %d RefreshIOPkts.NumOfNodes %d BUG FIXME FIXME !! \n", 
										Sftk_Lg->LGroupNumber, Sftk_Lg->NumOfAllocatedRefreshIOPkts, Sftk_Lg->RefreshIOPkts.NumOfNodes));
			OS_ASSERT(FALSE);
			// KeSetEvent( &Sftk_Lg->RefreshPktsWaitEvent, 0, FALSE);
		}
		*/
#endif
		KeSetEvent( &Sftk_Lg->ReleaseFreeAllPktsWaitEvent, 0, FALSE);
		if (Sftk_Lg->RefreshFinishedParseI == TRUE)
		{
			DebugPrint((DBG_ERROR, "BUG FIXME FIXME mm_free_buffer_to_refresh_pool:: Someone already called release, Signalling RefreshParse Done !! Lg %d NumOfAllocatedRefreshIOPkts %d RefreshIOPkts.NumOfNodes %d BUG FIXME FIXME !! \n", 
										Sftk_Lg->LGroupNumber, Sftk_Lg->NumOfAllocatedRefreshIOPkts, Sftk_Lg->RefreshIOPkts.NumOfNodes));
			OS_ASSERT(FALSE);
			// KeSetEvent( &Sftk_Lg->RefreshEmptyAckQueueEvent, 0, FALSE);
		}
		goto done;
	}
done:
	OS_RELEASE_LOCK( &GSftk_Config.Mmgr.ReservePoolLock, NULL);
	return status;
} // mm_free_buffer_to_refresh_pool()

//
// This will allocate Data Buffer + One Proto Hdr + specified numbers of Soft Hdr
// Structures Allocation : MM_HOLDER + one MM_PROTO_HDR + Specified numbers of MM_SOFT_HDR
//
PMM_HOLDER
mm_alloc_buffer_protoHdr_SoftHdr( PSFTK_LG Sftk_Lg, ULONG Size, ULONG	NumOfSoftHdr, BOOLEAN LockedAndMapedVA)
{
	PVOID					pMemory		= NULL;
	PMM_HOLDER				pMmHolder	= NULL;
	PMM_SOFT_HDR			pMmSoftHdr	= NULL;
	PMM_PROTO_HDR			pMmProtoHdr = NULL;
	ULONG					i;

	// TODO: Per LG, add logic to alloc maximum threshold memory, Check here that threshold !! 
	pMmHolder = mm_alloc(Size, LockedAndMapedVA);
	if (!pMmHolder)
	{
		DebugPrint((DBG_ERROR, "mm_alloc_buffer_protoHdr_SoftHdr: mm_alloc(size %d) Failed, returning NULL to caller !!! \n",
										Size));
		goto done;
	}

	pMmProtoHdr = mm_type_alloc(MM_TYPE_PROTOCOL_HEADER);
	if (pMmProtoHdr == NULL)
	{ // Failed :should not happened
		DebugPrint((DBG_ERROR, "mm_alloc_buffer_protoHdr_SoftHdr: mm_type_alloc(MM_TYPE_PROTOCOL_HEADER %d) Failed !! Allocation from OS Also Failed Returning Error !! \n",
											MM_TYPE_PROTOCOL_HEADER));
		goto done;
	}
	// Init MMHolder with protoHdr information
	pMmHolder->pProtocolHdr = pMmProtoHdr;
	OS_SetFlag( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_PROTO_HDR);

	for (i=0; i < NumOfSoftHdr; i++)
	{
		pMmSoftHdr = mm_type_alloc(MM_TYPE_SOFT_HEADER);
		if (pMmSoftHdr == NULL)
		{ // Failed :should not happened
			DebugPrint((DBG_ERROR, "mm_alloc_buffer_protoHdr_SoftHdr: i %d, mm_type_alloc(MM_TYPE_SOFT_HEADER %d) Failed !! Allocation from OS Also Failed Returning Error!! \n",
												i, MM_TYPE_SOFT_HEADER));
			goto done;
		}
		// Init MMHolder with protoHdr information
		MM_InsertSoftHdrIntoProtoHdr(pMmProtoHdr, pMmSoftHdr);
	}

	OS_ASSERT(pMmProtoHdr->MmSoftHdrList.NumOfNodes == NumOfSoftHdr);

	// caller will Initialize Hdr with proper information
	// Caller will Insert it into appropriate queue as specified
	pMemory	= pMmHolder;	// success

	OS_ASSERT(pMmHolder->Mdl != NULL);
	OS_ASSERT(pMmHolder->SystemVAddr != NULL);

done:
	if (pMemory == NULL)
	{ // Failed: Do clean up 
		if (pMmHolder)
			mm_free(pMmHolder);

		if (pMmProtoHdr)
			mm_free_ProtoHdr(Sftk_Lg, pMmProtoHdr);
	}
	return pMemory;
} // mm_alloc_buffer_protoHdr_SoftHdr()

VOID
SM_AddCmd_ForMM( SM_EVENT_COMMAND SmCmd)
{
	PMM_CMD_PACKET	pMmCmdPkt;

	switch(SmCmd)
	{
		case SM_Event_Alloc:	if (GSftk_Config.Mmgr.SM_AllocExecuting == TRUE)
									return;
								break;
		case SM_Event_Free:		if (GSftk_Config.Mmgr.SM_FreeExecuting == TRUE)
									return;
								break;
	}
	pMmCmdPkt = (PMM_CMD_PACKET) OS_AllocMemory( NonPagedPool, sizeof(MM_CMD_PACKET) );
	if (pMmCmdPkt == NULL)
	{
		DebugPrint((DBG_ERROR, "SM_AddCmd_ForMM:: OS_AllocMemory( size %d) Failed, doing nothing and returning!\n", 
							sizeof(MM_CMD_PACKET)));
		OS_ASSERT(FALSE);
		return;
	}
	InitializeListHead( &pMmCmdPkt->cmdLink );
	switch(SmCmd)
	{
		case SM_Event_Alloc:	GSftk_Config.Mmgr.SM_AllocExecuting = TRUE; break;
		case SM_Event_Free:		GSftk_Config.Mmgr.SM_FreeExecuting = TRUE; break;
	}
	pMmCmdPkt->Cmd		= SmCmd; // SM_Event_Alloc, SM_Event_Free, SM_Event_FreeAll, SM_Event_None

	OS_ACQUIRE_LOCK( &GSftk_Config.Mmgr.CmdQueueLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	InsertTailList( &GSftk_Config.Mmgr.CmdList.ListEntry, &pMmCmdPkt->cmdLink );
	GSftk_Config.Mmgr.CmdList.NumOfNodes ++;
	OS_RELEASE_LOCK( &GSftk_Config.Mmgr.CmdQueueLock, NULL);

	KeSetEvent( &GSftk_Config.Mmgr.CmdEvent, 0, FALSE);
	return;
} // SM_AddCmd_ForMM()

/***************** Windows OS Provided Slab Allocator Testing **************************/
/*
#if	MM_TEST_WINDOWS_SLAB
NTSTATUS	
MM_LookAsideList_Test()
{
	NTSTATUS status;

	status = MM_Init_LookAsideList();
	if (!NT_SUCCESS(status))
	{ // Failed
		DebugPrint((DBG_ERROR, "MM_LookAsideList_Test: MM_Init_LookAsideList() Failed with error %08x \n", status));
		return status;
	}

	status = MM_TestMaxMemAlloc_LookAsideList();
	if (!NT_SUCCESS(status))
	{ // Failed
		DebugPrint((DBG_ERROR, "MM_LookAsideList_Test: MM_TestMaxMemAlloc_LookAsideList() Failed with error %08x \n", status));
	}

	status = MM_DeInit_LookAsideList();
	if (!NT_SUCCESS(status))
	{ // Failed
		DebugPrint((DBG_ERROR, "MM_LookAsideList_Test: MM_DeInit_LookAsideList() Failed with error %08x \n", status));
	}

	return status;
} // MM_LookAsideList_Test()

NTSTATUS	
MM_Init_LookAsideList()
{
	NTSTATUS	status = STATUS_SUCCESS;
	UCHAR		i, j;	

	DebugPrint((DBG_ERROR, "MM_Init_LookAsideList: Started \n"));

	OS_ZeroMemory( GSftk_Config.MmSlab, sizeof(GSftk_Config.MmSlab) );

	for (i=0; i < MM_TYPE_MAX; i++)
	{ // For : Init all slabs

		ANCHOR_InitializeListHead( GSftk_Config.MmSlab[i].FreeList );
		ANCHOR_InitializeListHead( GSftk_Config.MmSlab[i].MdlInfoList );

		OS_INITIALIZE_LOCK( &GSftk_Config.MmSlab[i].Lock, OS_ERESOURCE_LOCK, NULL);

		GSftk_Config.MmSlab[i].Type = i;

		switch(i)
		{
			case MM_TYPE_MM_ENTRY	:	GSftk_Config.MmSlab[i].NodeSize = sizeof(MM_ENTRY);
										GSftk_Config.MmSlab[i].Allocate = NULL;
										GSftk_Config.MmSlab[i].Free = NULL;
										break;

			case MM_TYPE_4K_PAGED_MEM:	GSftk_Config.MmSlab[i].NodeSize = PAGE_SIZE;		
										GSftk_Config.MmSlab[i].Allocate = NULL;
										GSftk_Config.MmSlab[i].Free = NULL;
										break;

			case MM_TYPE_4K_NPAGED_MEM:	GSftk_Config.MmSlab[i].NodeSize = PAGE_SIZE;		
										GSftk_Config.MmSlab[i].Allocate = NULL;
										GSftk_Config.MmSlab[i].Free = NULL;
										break;

			case MM_TYPE_MM_HOLDER:		GSftk_Config.MmSlab[i].NodeSize = sizeof(MM_HOLDER);
										GSftk_Config.MmSlab[i].Allocate = NULL;
										GSftk_Config.MmSlab[i].Free = NULL;
										break;

			default:					break;
		}

		GSftk_Config.MmSlab[i].TotalNumberOfNodes	= 0;
		GSftk_Config.MmSlab[i].TotalMemSize			= 0;
		GSftk_Config.MmSlab[i].NumOfNodestoKeep		= MM_DEFAULT_MAX_NODES_TO_ALLOCATE;

		if (i == MM_TYPE_4K_PAGED_MEM)
		{
			ExInitializePagedLookasideList (	&GSftk_Config.MmSlab[i].parm.PagedLookaside, // IN PNPAGED_LOOKASIDE_LIST Lookaside,
												GSftk_Config.MmSlab[i].Allocate,			// IN PALLOCATE_FUNCTION Allocate,
												GSftk_Config.MmSlab[i].Free,				// IN PFREE_FUNCTION Free,
												0,											// IN ULONG Flags,
												GSftk_Config.MmSlab[i].NodeSize,		// IN SIZE_T Size,
												MM_TAG,									// IN ULONG Tag,
												0);										//IN USHORT Depth
		}
		else
		{
			ExInitializeNPagedLookasideList (	&GSftk_Config.MmSlab[i].parm.NPagedLookaside, // IN PNPAGED_LOOKASIDE_LIST Lookaside,
												GSftk_Config.MmSlab[i].Allocate,// IN PALLOCATE_FUNCTION Allocate,
												GSftk_Config.MmSlab[i].Free,// IN PFREE_FUNCTION Free,
												0,						// IN ULONG Flags,
												GSftk_Config.MmSlab[i].NodeSize, // IN SIZE_T Size,
												MM_TAG,				// IN ULONG Tag,
												0);					// IN USHORT Depth
		}
		OS_SetFlag(GSftk_Config.MmSlab[i].Flag, MM_ANCHOR_FLAG_LOOKASIDE_INIT_DONE);
	} // For : Init all slabs

	return status;
} // MM_Init_LookAsideList()

NTSTATUS	
MM_TestMaxMemAlloc_LookAsideList( )
{
	NTSTATUS	status;
	ULONG		i, j;

	for (j=0, i=MM_TYPE_4K_NPAGED_MEM; j < 2; j++)
	{ // For : Test Max mem can get allocate per slab

		status = MM_AllocMaxRawMem_LookAsideList( &GSftk_Config.MmSlab[i] );
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "MM_TestMaxMemAlloc_LookAsideList: MM_AllocMaxRawMem_LookAsideList(Type %d) Failed with 0x%08x \n", i,status));
			continue;
		}

		status = MM_FreeAllSlabRawMem_LookAsideList( &GSftk_Config.MmSlab[i] );
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "MM_TestMaxMemAlloc_LookAsideList: MM_FreeAllSlabRawMem_LookAsideList(MM_FreeAllSlabMem %d) Failed with 0x%08x \n", i,status));
			break;
			// continue;
		}
		i = MM_TYPE_4K_NPAGED_MEM;
	} // For : Test Max mem can get allocate per slab

	return STATUS_SUCCESS;
} // MM_TestMaxMemAlloc_LookAsideList()

NTSTATUS	
MM_AllocMaxRawMem_LookAsideList( PMM_ANCHOR MmAnchor)
{
	NTSTATUS	status = STATUS_SUCCESS;
	ULONG		i;	
	PMM_ENTRY	pMemEntry	= NULL;
	PVOID		pBuffer		= NULL;

	DebugPrint((DBG_ERROR, "MM_Init_LookAsideList: Started \n"));

	
	if ((MmAnchor->Type != MM_TYPE_4K_PAGED_MEM) && (MmAnchor->Type != MM_TYPE_4K_NPAGED_MEM))
	{
		status = STATUS_INVALID_PARAMETER;
		DebugPrint((DBG_ERROR, "MM_AllocMaxRawMem_LookAsideList: Invalid type %d, returning error %08x \n", MmAnchor->Type,status));
		return status;
	}
	
	// Diaply States of LookAside List information
	DebugPrint((DBG_ERROR, "******************** BEFORE_S *********************************\n"));
	
	DisplayLookAsideListInternals( &GSftk_Config.MmSlab[MM_TYPE_MM_ENTRY] );
	DisplayLookAsideListInternals( MmAnchor );
	
	DebugPrint((DBG_ERROR, "******************** BEFORE_E *********************************\n"));

	for (i=0;;i++)
	{ // For : Allocate Max mem per slab test peak number what we can allocate max

		// First allocate MM_Entry to hold new Raw memory pointers
		pMemEntry = MM_AllocMem_From_LookAsideList(MM_TYPE_MM_ENTRY);
		if (!pMemEntry)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "MM_AllocMaxRawMem_LookAsideList: MM_AllocMem_From_LookAsideList(%d) Failed !!! \n", MM_TYPE_MM_ENTRY));
			break;
		}
		// Allocate RAW Memory of 4K size 
		pMemEntry->ChunkEntry = (PMM_CHUNK_ENTRY) MM_AllocMem_From_LookAsideList(MmAnchor->Type);
		if (!pMemEntry->ChunkEntry)
		{
			// free mem Entry
			MM_FreeMemToOS_LookAsideList( MM_TYPE_MM_ENTRY, pMemEntry);

			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "MM_AllocMaxRawMem_LookAsideList: MM_AllocMem_From_LookAsideList(%d) Failed !!! \n", MmAnchor->Type));
			break;
		}

		// insert this into FreeList
		OS_ACQUIRE_LOCK( &MmAnchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

		InsertTailList( &MmAnchor->FreeList.ListEntry, &pMemEntry->MmEntryLink );
		MmAnchor->FreeList.NumOfNodes ++;

		OS_RELEASE_LOCK( &MmAnchor->Lock, NULL);
	} // For : Allocate Max mem per slab test peak number what we can allocate max

	// Diaply States of LookAside List information
	DebugPrint((DBG_ERROR, "******************** AFTER_S *********************************\n"));
	
	DisplayLookAsideListInternals( &GSftk_Config.MmSlab[MM_TYPE_MM_ENTRY] );
	DisplayLookAsideListInternals( MmAnchor );
	
	DebugPrint((DBG_ERROR, "******************** AFTER_E *********************************\n"));

	// Diaply MM_ANCHOR information 
	DebugPrint((DBG_ERROR, "-----------------------------------------------------------\n"));
	
	DisplayMMAnchorInfo_LookAsideList( &GSftk_Config.MmSlab[MM_TYPE_MM_ENTRY] );
	DisplayMMAnchorInfo_LookAsideList( MmAnchor );

	DebugPrint((DBG_ERROR, "-----------------------------------------------------------\n"));
	
	return STATUS_SUCCESS;
} // MM_AllocMaxRawMem_LookAsideList();

NTSTATUS	
MM_FreeAllSlabRawMem_LookAsideList( PMM_ANCHOR MmAnchor)
{
	NTSTATUS	status = STATUS_SUCCESS;
	PMM_ENTRY	pMemEntry	= NULL;
	PLIST_ENTRY	plistEntry	= NULL;

	DebugPrint((DBG_ERROR, "MM_FreeAllSlabRawMem_LookAsideList: Started \n"));

	if ((MmAnchor->Type != MM_TYPE_4K_PAGED_MEM) && (MmAnchor->Type != MM_TYPE_4K_NPAGED_MEM))
	{
		status = STATUS_INVALID_PARAMETER;
		DebugPrint((DBG_ERROR, "MM_FreeAllSlabRawMem_LookAsideList: Invalid type %d, returning error %08x \n", MmAnchor->Type,status));
		return status;
	}
	
	OS_ACQUIRE_LOCK( &MmAnchor->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	while( !IsListEmpty( &MmAnchor->FreeList.ListEntry) ) 
	{ // While : Free All memory 
		plistEntry = RemoveHeadList( &MmAnchor->FreeList.ListEntry );
		MmAnchor->FreeList.NumOfNodes --;	// Decrement Counter.

		pMemEntry = CONTAINING_RECORD( plistEntry, MM_ENTRY, MmEntryLink);

		// free Raw memory 
		status = MM_FreeMemToOS_LookAsideList(MmAnchor->Type, pMemEntry->ChunkEntry);
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "MM_FreeAllSlabRawMem_LookAsideList: MM_FreeMemToOS_LookAsideList(%d) failed with error %08x \n", MmAnchor->Type,status));
		}

		// free MM_ENTRY memory 
		status = MM_FreeMemToOS_LookAsideList(MM_TYPE_MM_ENTRY, pMemEntry);
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "MM_FreeAllSlabRawMem_LookAsideList: MM_FreeMemToOS_LookAsideList(%d) failed with error %08x \n", MM_TYPE_MM_ENTRY,status));
		}

	} // While : Free All memory 
	OS_RELEASE_LOCK( &MmAnchor->Lock, NULL);

	return STATUS_SUCCESS;
} // MM_FreeAllSlabRawMem_LookAsideList();

PVOID
MM_AllocMem_From_LookAsideList(ULONG Type)
{
	PVOID		pBuffer		= NULL;
	PMM_ENTRY	pMemEntry	= NULL;
	PMM_HOLDER	pMemHolder	= NULL;
	PLIST_ENTRY	plistEntry	= NULL;
	ULONG		i			= Type;

	OS_PERF;

	#if 0
	if ( !IsListEmpty( &GSftk_Config.MmSlab[i].FreeList.ListEntry) ) 
	{
		plistEntry = RemoveHeadList( &GSftk_Config.MmSlab[i].FreeList.ListEntry );
		GSftk_Config.MmSlab[i].FreeList.NumOfNodes --;	// Decrement Counter.

		switch(i)
		{
			case MM_TYPE_MM_ENTRY:		pBuffer = pMemEntry = CONTAINING_RECORD( plistEntry, MM_ENTRY, MmEntryLink); break;

			case MM_TYPE_4K_PAGED_MEM:	
			case MM_TYPE_4K_NPAGED_MEM:	pMemEntry = CONTAINING_RECORD( plistEntry, MM_ENTRY, MmEntryLink); 
										OS_ASSERT(pMemEntry->ChunkEntry != NULL);
										pBuffer = pMemEntry->ChunkEntry;
										break;
			case MM_TYPE_MM_HOLDER:		pBuffer = pMemHolder = CONTAINING_RECORD( plistEntry, MM_HOLDER, MmHolderLink); break;
			default : break; // ERROR
		}
		goto done;
	}
	#endif

	OS_ASSERT( OS_IsFlagSet(GSftk_Config.MmSlab[i].Flag, MM_ANCHOR_FLAG_LOOKASIDE_INIT_DONE) == TRUE);

	OS_PERF_STARTTIME;

	// allocate new
	if (i == MM_TYPE_4K_PAGED_MEM)
	{
		pBuffer = ExAllocateFromPagedLookasideList( &GSftk_Config.MmSlab[i].parm.PagedLookaside );
		if (pBuffer == NULL)
		{
			DebugPrint((DBG_ERROR, "MM_AllocMem_From_LookAsideList: ExAllocateFromNPagedLookasideList(Type %d) Failed !! \n",i));
		}
	}
	else
	{
		pBuffer = ExAllocateFromNPagedLookasideList( &GSftk_Config.MmSlab[i].parm.NPagedLookaside );
		if (pBuffer == NULL)
		{
			DebugPrint((DBG_ERROR, "MM_AllocMem_From_LookAsideList: ExAllocateFromNPagedLookasideList(Type %d) Failed !! \n",i));
		}
	}

	OS_PERF_ENDTIME(MM_ALLOC_PROCESSING, 1);

	if (pBuffer)
	{ // set variable according
		GSftk_Config.MmSlab[i].TotalNumberOfNodes ++;
		GSftk_Config.MmSlab[i].TotalMemSize += GSftk_Config.MmSlab[i].NodeSize;

		switch(i)
		{
			case MM_TYPE_MM_ENTRY:		pMemEntry = pBuffer; break;
			case MM_TYPE_4K_PAGED_MEM:	
			case MM_TYPE_4K_NPAGED_MEM:	break;
			case MM_TYPE_MM_HOLDER:		pMemHolder = pBuffer; break;
			default:					break; // ERROR
		}
	}
	
// done:
	if (pBuffer)
		OS_ZeroMemory(pBuffer, GSftk_Config.MmSlab[i].NodeSize);

	if (pMemEntry)
	{ // initialize 
		InitializeListHead(&pMemEntry->MmEntryLink);
		// pMemEntry->ChunkEntry		= NULL;
		// pMemEntry->PageEntry	= NULL;
		// Do Not Insert it into free list since it will go to different free list like 4k page free list.
	}

	if (pMemHolder)
	{ // initialize 
		InitializeListHead(&pMemHolder->MmHolderLink);
		ANCHOR_InitializeListHead( pMemHolder->MmEntryList );
		// pMemHolder->VAddr		= NULL;
		// pMemHolder->PageEntry	= NULL;
		// Do Not Insert it into free list since it will go to different free list like 4k page free list.
	}

	return pBuffer;
} // MM_AllocMem_From_LookAsideList()

NTSTATUS	
MM_FreeMemToOS_LookAsideList( ULONG Type, PVOID	Buffer)
{
	OS_PERF;

	if(Buffer == NULL)
	{
		DebugPrint((DBG_ERROR, "MM_FreeMemToOS_LookAsideList: Type %d Buffer == NULL, FIXME FIXME  !! \n", Type));
		return STATUS_INVALID_PARAMETER;
	}

	OS_PERF_STARTTIME;

	if (Type == MM_TYPE_4K_PAGED_MEM)
	{
		ExFreeToPagedLookasideList( &GSftk_Config.MmSlab[Type].parm.PagedLookaside, Buffer );
	}
	else
	{
		ExFreeToNPagedLookasideList( &GSftk_Config.MmSlab[Type].parm.NPagedLookaside, Buffer );
	}

	OS_PERF_ENDTIME(MM_FREE_PROCESSING, 1);

	GSftk_Config.MmSlab[Type].TotalNumberOfNodes --;
	GSftk_Config.MmSlab[Type].TotalMemSize -= GSftk_Config.MmSlab[Type].NodeSize;

	return STATUS_SUCCESS;
} // MM_FreeMemToOS_LookAsideList();


NTSTATUS	
MM_DeInit_LookAsideList()
{
	UCHAR	i;
	// free all memory
	DebugPrint((DBG_ERROR, "MM_DeInit_LookAsideList: Started !! \n"));

	for (i=0; i < MM_TYPE_MAX; i++)
	{
		if (OS_IsFlagSet(GSftk_Config.MmSlab[i].Flag, MM_ANCHOR_FLAG_LOOKASIDE_INIT_DONE))
		{
			if (i == MM_TYPE_4K_PAGED_MEM)
				ExDeletePagedLookasideList( &GSftk_Config.MmSlab[i].parm.PagedLookaside); // IN PNPAGED_LOOKASIDE_LIST Lookaside,
			else
				ExDeleteNPagedLookasideList( &GSftk_Config.MmSlab[i].parm.NPagedLookaside); // IN PNPAGED_LOOKASIDE_LIST Lookaside,
		}

		OS_DEINITIALIZE_LOCK( &GSftk_Config.MmSlab[i].Lock, NULL);
	}

	OS_ZeroMemory( GSftk_Config.MmSlab, sizeof(GSftk_Config.MmSlab) );

	return STATUS_SUCCESS;
} // MM_DeInit_LookAsideList()

VOID
DisplayLookAsideListInternals( PMM_ANCHOR MmAnchor)
{
	if (MmAnchor->Type == MM_TYPE_4K_PAGED_MEM)
	{ // Used Paged LookAside List
		DebugPrint((DBG_ERROR, "PagedLookAsideList Info For MM_Type:      %d\n", MmAnchor->Type));
		DebugPrint((DBG_ERROR, "Depth             : %d\n", MmAnchor->parm.PagedLookaside.L.Depth));
		DebugPrint((DBG_ERROR, "MaximumDepth      : %d\n", MmAnchor->parm.PagedLookaside.L.MaximumDepth ));
		DebugPrint((DBG_ERROR, "TotalAllocates    : %d\n", MmAnchor->parm.PagedLookaside.L.TotalAllocates ));
		DebugPrint((DBG_ERROR, "AllocateHits      : %d\n", MmAnchor->parm.PagedLookaside.L.AllocateHits ));
		DebugPrint((DBG_ERROR, "TotalFrees        : %d\n", MmAnchor->parm.PagedLookaside.L.TotalFrees ));
		DebugPrint((DBG_ERROR, "FreeMisses        : %d\n", MmAnchor->parm.PagedLookaside.L.FreeMisses ));
		DebugPrint((DBG_ERROR, "PoolType          : %d\n", MmAnchor->parm.PagedLookaside.L.Type ));
		DebugPrint((DBG_ERROR, "Tag               : %d\n", MmAnchor->parm.PagedLookaside.L.Tag ));
		DebugPrint((DBG_ERROR, "Size              : %d\n", MmAnchor->parm.PagedLookaside.L.Size ));
		DebugPrint((DBG_ERROR, "AllocateAPI       : 0x%08x\n", MmAnchor->parm.PagedLookaside.L.Allocate ));
		DebugPrint((DBG_ERROR, "FreeAPI           : 0x%08x\n", MmAnchor->parm.PagedLookaside.L.Free ));
		DebugPrint((DBG_ERROR, "LastTotalAllocates: %d\n", MmAnchor->parm.PagedLookaside.L.LastTotalAllocates ));
		DebugPrint((DBG_ERROR, "LastAllocateMisses: %d\n", MmAnchor->parm.PagedLookaside.L.LastAllocateMisses ));

	}
	else
	{ // Used NonPaged LookAside List
		DebugPrint((DBG_ERROR, "NPagedLookAsideList Info For MM_Type:      %d\n", MmAnchor->Type));
		DebugPrint((DBG_ERROR, "Depth             : %d\n", MmAnchor->parm.NPagedLookaside.L.Depth));
		DebugPrint((DBG_ERROR, "MaximumDepth      : %d\n", MmAnchor->parm.NPagedLookaside.L.MaximumDepth ));
		DebugPrint((DBG_ERROR, "TotalAllocates    : %d\n", MmAnchor->parm.NPagedLookaside.L.TotalAllocates ));
		DebugPrint((DBG_ERROR, "AllocateHits      : %d\n", MmAnchor->parm.NPagedLookaside.L.AllocateHits ));
		DebugPrint((DBG_ERROR, "TotalFrees        : %d\n", MmAnchor->parm.NPagedLookaside.L.TotalFrees ));
		DebugPrint((DBG_ERROR, "FreeMisses        : %d\n", MmAnchor->parm.NPagedLookaside.L.FreeMisses ));
		DebugPrint((DBG_ERROR, "PoolType          : %d\n", MmAnchor->parm.NPagedLookaside.L.Type ));
		DebugPrint((DBG_ERROR, "Tag               : %d\n", MmAnchor->parm.NPagedLookaside.L.Tag ));
		DebugPrint((DBG_ERROR, "Size              : %d\n", MmAnchor->parm.NPagedLookaside.L.Size ));
		DebugPrint((DBG_ERROR, "AllocateAPI       : 0x%08x\n", MmAnchor->parm.NPagedLookaside.L.Allocate ));
		DebugPrint((DBG_ERROR, "FreeAPI           : 0x%08x\n", MmAnchor->parm.NPagedLookaside.L.Free ));
		DebugPrint((DBG_ERROR, "LastTotalAllocates: %d\n", MmAnchor->parm.NPagedLookaside.L.LastTotalAllocates ));
		DebugPrint((DBG_ERROR, "LastAllocateMisses: %d\n", MmAnchor->parm.NPagedLookaside.L.LastAllocateMisses ));
	}

} // DisplayLookAsideListInternals()

VOID
DisplayMMAnchorInfo_LookAsideList( PMM_ANCHOR MmAnchor)
{
	DebugPrint((DBG_ERROR, "MM_Type:      %d\n", MmAnchor->Type));
	DebugPrint((DBG_ERROR, "NodeSize:    %d\n", MmAnchor->NodeSize));
	DebugPrint((DBG_ERROR, "TotalMemSize:    %d\n", MmAnchor->TotalMemSize));
	// DebugPrint((DBG_ERROR, "MaximumSize:   %d\n", MmAnchor->MaximumSize));
	// DebugPrint((DBG_ERROR, "MinimumSize:   %d\n", MmAnchor->MinimumSize));
	DebugPrint((DBG_ERROR, "NumberOfNodes: %d\n", MmAnchor->FreeList.NumOfNodes));
	DebugPrint((DBG_ERROR, "TotalNumberOfNodes: %d\n", MmAnchor->TotalNumberOfNodes));
	DebugPrint((DBG_ERROR, "NumOfNodestoKeep: %d\n", MmAnchor->NumOfNodestoKeep));
} // DisplayMMAnchorInfo_LookAsideList()

#endif // #if	MM_TEST_WINDOWS_SLAB
*/