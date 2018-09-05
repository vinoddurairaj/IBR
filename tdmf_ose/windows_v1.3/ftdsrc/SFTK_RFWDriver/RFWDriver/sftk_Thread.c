/**************************************************************************************

Module Name: sftk_Thread.C   
Author Name: Parag sanghvi 
Description: thread APIS and its related APIS
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

NTSTATUS
sftk_Create_DevThread( IN OUT 	PSFTK_DEV		Sftk_Dev, 
					   IN		ftd_dev_info_t  *In_dev_info )
{
	NTSTATUS			status = STATUS_SUCCESS;
	HANDLE				threadHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;

	// create Master Thread per device for its own Pstore LRDB bitmap to flush 
	// This thread also allocates/initalize bab memory for new write data if LG is running
	// in SFTK_MODE_NORMAL or SFTK_MODE_SMART_REFRESH. (if SFTK_MODE_BAB_UPDATE is set)
    
	OS_INITIALIZE_LOCK( &Sftk_Dev->MasterQueueLock, OS_SPIN_LOCK, NULL);

    try 
    {
		Sftk_Dev->MasterThreadObjectPointer = NULL;

        Sftk_Dev->ThreadShouldStop = FALSE;

		if (Sftk_Dev->MasterIrpProcessingThreadWakeupTimeout.QuadPart == 0)
		{
			Sftk_Dev->MasterIrpProcessingThreadWakeupTimeout.QuadPart = DEFAULT_TIMEOUT_FOR_MASTERIRP_PROCESSING_THREAD;
		}
       
		KeInitializeEvent( &Sftk_Dev->MasterQueueEvent, SynchronizationEvent , FALSE);
		// KeInitializeSemaphore(&Sftk_Dev->MasterQueueSemaphore, 0, MAXLONG);

		ANCHOR_InitializeListHead( Sftk_Dev->MasterQueueList )

#ifdef NTFOUR
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
#else
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
#endif
        status = PsCreateSystemThread(	&threadHandle,
										(ACCESS_MASK) 0L,
										&ObjectAttributes,
										(HANDLE) 0L,
										NULL,
										sftk_dev_master_thread,
										Sftk_Dev);

        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "sftk_Create_DevThread:: PsCreateSystemThread( for sftk_dev_master_thread Device %s) Failed, returning 0x%08x !", 
								In_dev_info->vdevname, status ));
            try_return(status);
        }
		
        Sftk_Dev->MasterThreadObjectPointer = NULL;
        status = ObReferenceObjectByHandle(threadHandle,
										   THREAD_ALL_ACCESS,
										   NULL,
										   KernelMode,
										   &Sftk_Dev->MasterThreadObjectPointer,
										   NULL);

        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "sftk_Create_DevThread:: ObReferenceObjectByHandle( for sftk_dev_master_thread Device %s) Failed, returning 0x%08x !", 
								In_dev_info->vdevname, status ));
            try_return(status);
        }

        ZwClose(threadHandle);

        try_exit:   NOTHING;

    } 
    finally 
    {
        if (!NT_SUCCESS(status)) 
        { // failed Terminate thread if its stareted
            Sftk_Dev->ThreadShouldStop = TRUE;

            if (Sftk_Dev->MasterThreadObjectPointer) 
            {
                // KeReleaseSemaphore(&Sftk_Dev->MasterQueueSemaphore, 0, 1, FALSE);
				KeSetEvent( &Sftk_Dev->MasterQueueEvent, 0, FALSE);
                KeWaitForSingleObject(	(PVOID) Sftk_Dev->MasterThreadObjectPointer,
										Executive,
										KernelMode,
										FALSE,
										NULL );
                ObDereferenceObject( Sftk_Dev->MasterThreadObjectPointer );

				OS_DEINITIALIZE_LOCK( &Sftk_Dev->MasterQueueLock, NULL);
				Sftk_Dev->MasterThreadObjectPointer = NULL;
            }
        } 
    }

	return status;
} // sftk_Create_DevThread()

NTSTATUS
sftk_Terminate_DevThread( IN OUT 	PSFTK_DEV	Sftk_Dev )
{
	NTSTATUS status = STATUS_SUCCESS;

	Sftk_Dev->ThreadShouldStop = TRUE;

    if (Sftk_Dev->MasterThreadObjectPointer) 
    {
        // KeReleaseSemaphore(&Sftk_Dev->MasterQueueSemaphore, 0, 1, FALSE);
		KeSetEvent( &Sftk_Dev->MasterQueueEvent, 0, FALSE);
        KeWaitForSingleObject(	(PVOID) Sftk_Dev->MasterThreadObjectPointer,
								Executive,
								KernelMode,
								FALSE,
								NULL );
        ObDereferenceObject( Sftk_Dev->MasterThreadObjectPointer );

		OS_DEINITIALIZE_LOCK( &Sftk_Dev->MasterQueueLock, NULL);
    }

	return status;
} // sftk_Terminate_DevThread()

NTSTATUS
sftk_Create_LGThread(	IN OUT 	PSFTK_LG		Sftk_Lg, 
						IN		ftd_lg_info_t	*LG_Info )
{
	NTSTATUS			status = STATUS_SUCCESS;
	HANDLE				threadHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;

	// create Refresh Thread, Acknowledgement Thread, TDI thread and other thread per Logical Group. 

	// Refresh Thread 
	// repsonbilities is to wait on state for change event and 
	// starts doing Full-refresh Or Smart Refresh operations per Logical Group (every devices).

	// Acknowledgement Thread:
	// repsonbilities is to wait on timeout/signal event 
	// starts traversing all Cache manager (BAB for incoming I/O) Queues and prepare new ALRDB bitmap
	// Merge ALRDB with LRDB and make this LRDB as new LRDB. Update it onto  Pstore file.
	// If current mode is Tracking Mode or smart Refresh mode, it also builds AHRDB from Queue list.
	// Merge AHRDB with HRDB and make this HRDB as new HRDB. 

	// TDI thread:
	// Veera's Opaque thread module, responsible for Socket connections and other relative works.

    try 
    {
		Sftk_Lg->RefreshThreadShouldStop	= FALSE;
		Sftk_Lg->AckThreadShouldStop		= FALSE;
       
		Sftk_Lg->RefreshThreadObjectPointer	= NULL;
		Sftk_Lg->AckThreadObjectPointer		= NULL;

		KeInitializeSemaphore( &Sftk_Lg->RefreshStateChangeSemaphore, 0, MAXLONG);
		KeInitializeSemaphore( &Sftk_Lg->AckStateChangeSemaphore, 0, MAXLONG);
			
		// TODO : Get this value from Service in ftd_lg_info_t structure....
		if (Sftk_Lg->RefreshThreadWakeupTimeout.QuadPart == 0)
			Sftk_Lg->RefreshThreadWakeupTimeout.QuadPart	= DEFAULT_TIMEOUT_FOR_REFRESH_THREAD;

		if (Sftk_Lg->AckWakeupTimeout.QuadPart == 0)
			Sftk_Lg->AckWakeupTimeout.QuadPart	= DEFAULT_TIMEOUT_FOR_ACK_THREAD;

#if TARGET_SIDE
		// initialize secondary side threads
		Sftk_Lg->Secondary.TWriteThreadShouldStop	= FALSE;
		Sftk_Lg->Secondary.JApplyThreadShouldStop	= FALSE;
		Sftk_Lg->Secondary.TWriteThreadObjPtr		= NULL;
		Sftk_Lg->Secondary.JApplyThreadObjPtr		= NULL;

		KeInitializeEvent( &Sftk_Lg->Secondary.TWriteWorkEvent, SynchronizationEvent, FALSE);
		KeInitializeEvent( &Sftk_Lg->Secondary.JApplyWorkEvent, SynchronizationEvent, FALSE);

		if (Sftk_Lg->Secondary.TWriteWakeupTimeout.QuadPart == 0)
			Sftk_Lg->Secondary.TWriteWakeupTimeout.QuadPart = DEFAULT_TIMEOUT_FOR_TARGET_WRITE_THREAD;

		if (Sftk_Lg->Secondary.JApplyWakeupTimeout.QuadPart == 0)
			Sftk_Lg->Secondary.JApplyWakeupTimeout.QuadPart = DEFAULT_TIMEOUT_FOR_JAPPLY_THREAD;
#endif
		
		// Create Refresh Thread
#ifdef NTFOUR
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
#else
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
#endif
        status = PsCreateSystemThread(	&threadHandle,
										(ACCESS_MASK) 0L,
										&ObjectAttributes,
										(HANDLE) 0L,
										NULL,
										sftk_refresh_lg_thread,
										Sftk_Lg);

        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "sftk_Create_LGThread:: PsCreateSystemThread( for sftk_refresh_lg_thread  LG Number %d) Failed, returning 0x%08x !", 
								Sftk_Lg->LGroupNumber, status ));
            try_return(status);
        }

        status = ObReferenceObjectByHandle(threadHandle,
										   THREAD_ALL_ACCESS,
										   NULL,
										   KernelMode,
										   &Sftk_Lg->RefreshThreadObjectPointer,
										   NULL);

        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "sftk_Create_LGThread:: ObReferenceObjectByHandle( for sftk_refresh_lg_thread  LG Number %d) Failed, returning 0x%08x !", 
								Sftk_Lg->LGroupNumber, status ));
            try_return(status);
        }

        ZwClose(threadHandle);

		// Create Acknowledgement thread
#ifdef NTFOUR
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
#else
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
#endif
        status = PsCreateSystemThread(	&threadHandle,
										(ACCESS_MASK) 0L,
										&ObjectAttributes,
										(HANDLE) 0L,
										NULL,
										sftk_acknowledge_lg_thread,
										Sftk_Lg);

        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "sftk_Create_LGThread:: PsCreateSystemThread( sftk_acknowledge_lg_thread LG Number %d) Failed, returning 0x%08x !", 
								Sftk_Lg->LGroupNumber, status ));
            try_return(status);
        }

        status = ObReferenceObjectByHandle(threadHandle,
										   THREAD_ALL_ACCESS,
										   NULL,
										   KernelMode,
										   &Sftk_Lg->AckThreadObjectPointer,
										   NULL);

        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "sftk_Create_LGThread:: ObReferenceObjectByHandle( sftk_acknowledge_lg_thread  LG Number %d) Failed, returning 0x%08x !", 
								Sftk_Lg->LGroupNumber, status ));
            try_return(status);
        }

        ZwClose(threadHandle);

#if TARGET_SIDE
		// --- Create Secondary side thread
		// Create Target Write thread
#ifdef NTFOUR
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
#else
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
#endif
        status = PsCreateSystemThread(	&threadHandle,
										(ACCESS_MASK) 0L,
										&ObjectAttributes,
										(HANDLE) 0L,
										NULL,
										sftk_Target_write_Thread,
										Sftk_Lg);
        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "sftk_Create_LGThread:: PsCreateSystemThread( sftk_Target_write_Thread LG Number %d) Failed, returning 0x%08x !", 
								Sftk_Lg->LGroupNumber, status ));
            try_return(status);
        }
        status = ObReferenceObjectByHandle(threadHandle,
										   THREAD_ALL_ACCESS,
										   NULL,
										   KernelMode,
										   &Sftk_Lg->Secondary.TWriteThreadObjPtr,
										   NULL);
        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "sftk_Create_LGThread:: ObReferenceObjectByHandle( sftk_Target_write_Thread LG Number %d) Failed, returning 0x%08x !", 
								Sftk_Lg->LGroupNumber, status ));
            try_return(status);
        }
        ZwClose(threadHandle);

		// Create JApply thread
#ifdef NTFOUR
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
#else
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
#endif
        status = PsCreateSystemThread(	&threadHandle,
										(ACCESS_MASK) 0L,
										&ObjectAttributes,
										(HANDLE) 0L,
										NULL,
										sftk_JApply_Thread,
										Sftk_Lg);
        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "sftk_Create_LGThread:: PsCreateSystemThread( sftk_JApply_Thread LG Number %d) Failed, returning 0x%08x !", 
								Sftk_Lg->LGroupNumber, status ));
            try_return(status);
        }
        status = ObReferenceObjectByHandle(threadHandle,
										   THREAD_ALL_ACCESS,
										   NULL,
										   KernelMode,
										   &Sftk_Lg->Secondary.JApplyThreadObjPtr,
										   NULL);
        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "sftk_Create_LGThread:: ObReferenceObjectByHandle( sftk_JApply_Thread LG Number %d) Failed, returning 0x%08x !", 
								Sftk_Lg->LGroupNumber, status ));
            try_return(status);
        }
        ZwClose(threadHandle);
#endif // #if TARGET_SIDE

        try_exit:   NOTHING;

    } 
    finally 
    {
        if (!NT_SUCCESS(status)) 
        { // failed Terminate All threads if its stareted
            Sftk_Lg->RefreshThreadShouldStop	= TRUE;
			Sftk_Lg->AckThreadShouldStop		= TRUE;
       
            if (Sftk_Lg->RefreshThreadObjectPointer) 
            {
                KeReleaseSemaphore( &Sftk_Lg->RefreshStateChangeSemaphore, 0, 1, FALSE);
                KeWaitForSingleObject(	(PVOID) Sftk_Lg->RefreshThreadObjectPointer,
										Executive,
										KernelMode,
										FALSE,
										NULL );
                ObDereferenceObject( Sftk_Lg->RefreshThreadObjectPointer );
				Sftk_Lg->RefreshThreadObjectPointer = NULL;
            }

			if (Sftk_Lg->AckThreadObjectPointer) 
            {
                KeReleaseSemaphore( &Sftk_Lg->AckStateChangeSemaphore, 0, 1, FALSE);
                KeWaitForSingleObject(	(PVOID) Sftk_Lg->AckThreadObjectPointer,
										Executive,
										KernelMode,
										FALSE,
										NULL );
                ObDereferenceObject( Sftk_Lg->AckThreadObjectPointer );
				Sftk_Lg->AckThreadObjectPointer = NULL;
            }

#if TARGET_SIDE
			Sftk_Lg->Secondary.TWriteThreadShouldStop = TRUE;
			Sftk_Lg->Secondary.JApplyThreadShouldStop = TRUE;

			if (Sftk_Lg->Secondary.TWriteThreadObjPtr) 
            {
                KeSetEvent( &Sftk_Lg->Secondary.TWriteWorkEvent, 0, FALSE);
                KeWaitForSingleObject(	(PVOID) Sftk_Lg->Secondary.TWriteThreadObjPtr,
										Executive,
										KernelMode,
										FALSE,
										NULL );
                ObDereferenceObject( Sftk_Lg->Secondary.TWriteThreadObjPtr );
				Sftk_Lg->Secondary.TWriteThreadObjPtr = NULL;
            }

			if (Sftk_Lg->Secondary.JApplyThreadObjPtr) 
            {
                KeSetEvent( &Sftk_Lg->Secondary.TWriteWorkEvent, 0, FALSE);
                KeWaitForSingleObject(	(PVOID) Sftk_Lg->Secondary.JApplyThreadObjPtr,
										Executive,
										KernelMode,
										FALSE,
										NULL );
                ObDereferenceObject( Sftk_Lg->Secondary.JApplyThreadObjPtr );
				Sftk_Lg->Secondary.JApplyThreadObjPtr = NULL;
            }
#endif
        } 
    }

	return status;
} // sftk_Create_LGThread()

NTSTATUS
sftk_Terminate_LGThread( IN OUT PSFTK_LG	Sftk_Lg)
{
	NTSTATUS status = STATUS_SUCCESS;

	Sftk_Lg->RefreshThreadShouldStop	= TRUE;
	Sftk_Lg->AckThreadShouldStop		= TRUE;
    
    if (Sftk_Lg->RefreshThreadObjectPointer) 
    {
        KeReleaseSemaphore( &Sftk_Lg->RefreshStateChangeSemaphore, 0, 1, FALSE);
        KeWaitForSingleObject(	(PVOID) Sftk_Lg->RefreshThreadObjectPointer,
								Executive,
								KernelMode,
								FALSE,
								NULL );
        ObDereferenceObject( Sftk_Lg->RefreshThreadObjectPointer );
		Sftk_Lg->RefreshThreadObjectPointer = NULL;
    }

	if (Sftk_Lg->AckThreadObjectPointer) 
    {
        KeReleaseSemaphore( &Sftk_Lg->AckStateChangeSemaphore, 0, 1, FALSE);
        KeWaitForSingleObject(	(PVOID) Sftk_Lg->AckThreadObjectPointer,
								Executive,
								KernelMode,
								FALSE,
								NULL );
        ObDereferenceObject( Sftk_Lg->AckThreadObjectPointer );
		Sftk_Lg->AckThreadObjectPointer = NULL;
    }

#if TARGET_SIDE
	Sftk_Lg->Secondary.TWriteThreadShouldStop = TRUE;
	Sftk_Lg->Secondary.JApplyThreadShouldStop = TRUE;

	if (Sftk_Lg->Secondary.TWriteThreadObjPtr) 
    {
        KeSetEvent( &Sftk_Lg->Secondary.TWriteWorkEvent, 0, FALSE);
        KeWaitForSingleObject(	(PVOID) Sftk_Lg->Secondary.TWriteThreadObjPtr,
								Executive,
								KernelMode,
								FALSE,
								NULL );
        ObDereferenceObject( Sftk_Lg->Secondary.TWriteThreadObjPtr );
		Sftk_Lg->Secondary.TWriteThreadObjPtr = NULL;
    }
	if (Sftk_Lg->Secondary.JApplyThreadObjPtr) 
    {
        KeSetEvent( &Sftk_Lg->Secondary.TWriteWorkEvent, 0, FALSE);
        KeWaitForSingleObject(	(PVOID) Sftk_Lg->Secondary.JApplyThreadObjPtr,
								Executive,
								KernelMode,
								FALSE,
								NULL );
        ObDereferenceObject( Sftk_Lg->Secondary.JApplyThreadObjPtr );
		Sftk_Lg->Secondary.JApplyThreadObjPtr = NULL;
    }
#endif
	return status;
} // sftk_Terminate_LGThread()

// *********************** Thread APIS ********************************************** 

// Device Master Thread for its own Pstore LRDB bitmap to flush 
// This thread also allocates/initalize bab memory for new write data if LG is running
// in SFTK_MODE_NORMAL or SFTK_MODE_SMART_REFRESH. (if SFTK_MODE_BAB_UPDATE is set)
    
VOID
sftk_dev_master_thread( PSFTK_DEV	Sftk_Dev)
{
	NTSTATUS			status			= STATUS_SUCCESS;
	PSFTK_LG			pSftk_Lg		= Sftk_Dev->SftkLg;
	BOOLEAN				bPstoreFlush	= FALSE;	
	ANCHOR_LINKLIST		localQueueList;
	LARGE_INTEGER		lrdb_offset;
	PIRP_CONTEXT		pIrpContext;
    PLIST_ENTRY			plistEntry;
    PIRP				pIrp;
	PIO_STACK_LOCATION	pIrpStack;
	LONG				currentState;
	HANDLE				fileHandle = NULL;
	SM_INIT_PARAMS		sm_Params;
	
    
	DebugPrint((DBG_THREAD, "sftk_dev_master_thread:: Starting Dev Master Thread: Sftk_Dev 0x%08x for Device %s ! \n", 
								Sftk_Dev, Sftk_Dev->Vdevname));

	if (OS_IsFlagSet( Sftk_Dev->SftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == FALSE)
	{
		status = sftk_open_pstore( &Sftk_Dev->SftkLg->PStoreFileName, &fileHandle, FALSE);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			WCHAR				wszStringchar1[64];

			DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: sftk_open_pstore(%S, Sftk_Dev 0x%08x for Device %s ) Failed with status 0x%08x !\n", 
									Sftk_Dev->SftkLg->PStoreFileName.Buffer, Sftk_Dev, Sftk_Dev->Vdevname, status ));
			DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just continue the operations...\n"));

			swprintf(  wszStringchar1, L"Dev Master IRP Processing Thread");
			sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_OPEN_ERROR, status, 
								  0, Sftk_Dev->SftkLg->PStoreFileName.Buffer, wszStringchar1);
		}

		// old code was : lrdb_offset.QuadPart = ((LONGLONG) (Sftk_Dev->lrdb_offset)) << DEV_BSHIFT;  // to bytes
		lrdb_offset.QuadPart = (LONGLONG) Sftk_Dev->PsDev->LrdbOffset;
	}

    // Set thread priority to lowest realtime level.
    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while (Sftk_Dev->ThreadShouldStop == FALSE) 
    {
        // Wait for a request from the dispatch routines.
        // KeWaitForSingleObject won't return error here - this thread
        // isn't alertable and won't take APCs, and we're not passing in
        // a timeout.
		if (Sftk_Dev->MasterIrpProcessingThreadWakeupTimeout.QuadPart == 0)
		{
			Sftk_Dev->MasterIrpProcessingThreadWakeupTimeout.QuadPart	= DEFAULT_TIMEOUT_FOR_MASTERIRP_PROCESSING_THREAD;
		}

        try 
        {
            KeWaitForSingleObject(	(PVOID) &Sftk_Dev->MasterQueueEvent, // &Sftk_Dev->MasterQueueSemaphore,
									Executive,
									KernelMode,
									FALSE,
									&Sftk_Dev->MasterIrpProcessingThreadWakeupTimeout );

        } 
        except (sftk_ExceptionFilterDontStop(GetExceptionInformation(), GetExceptionCode()) ) 
        {
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

            // We encountered an exception somewhere, eat it up.
            DebugPrint((DBG_ERROR, "sftk_dev_master_thread::KeWaitForSingleObject() crashed: EXCEPTION_EXECUTE_HANDLER, Exception code = 0x%08x.\n", GetExceptionCode() ));
        }

        // if lrdb bits are set, complete the IRPs
		ANCHOR_InitializeListHead( localQueueList );

		OS_ACQUIRE_LOCK( &Sftk_Dev->MasterQueueLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
        ANCHOR_MOVE_LIST( localQueueList, Sftk_Dev->MasterQueueList );
		OS_RELEASE_LOCK( &Sftk_Dev->MasterQueueLock, NULL);

		// Walk thru Local link list
		for(	plistEntry = localQueueList.ListEntry.Flink;
				plistEntry != &localQueueList.ListEntry;)
		{ // for :scan thru each and every IRP list 

			// During addition Irp into queue, we use IRP fields to add it into link list
			// Following code is used for this operations:

			// pIrpContext = (PIRP_CONTEXT) &pIrp->Tail.Overlay.DriverContext[0];
			// InitializeListHead( &pIrpContext->ListEntry );
			// pIrpContext->Irp = pIrp;
			// 
			// OS_ACQUIRE_LOCK( &Sftk_Dev->MasterQueueLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
			// InsertTailList( &Sftk_Dev->MasterQueueList.ListEntry, &pIrpContext->ListEntry);
			// Sftk_Dev->MasterQueueList.NumOfNodes ++;
			// OS_RELEASE_LOCK( &Sftk_Dev->MasterQueueLock, NULL);

			pIrpContext = (PIRP_CONTEXT) plistEntry;
			plistEntry = plistEntry->Flink;	// get next lstentry 

			if (plistEntry == NULL)
			{
				DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: BUG FIXME FIXME (Sftk_Dev 0x%08x for Device %s ) next IRP list is NULL !!! IRP may have cancelled.... PANIC, Can not continue, few IRP may get lost permenently ....!\n", 
											Sftk_Dev, Sftk_Dev->Vdevname));
				OS_ASSERT(FALSE);
				break;
			}

			pIrp = pIrpContext->Irp;
			if (pIrp == NULL)
			{
				DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: BUG FIXME FIXME :(Sftk_Dev 0x%08x for Device %s ) IRP is NULL !!! IRP may have cancelled.... PANIC!\n", 
											Sftk_Dev, Sftk_Dev->Vdevname));
				OS_ASSERT(FALSE);
				continue;
			}

			// ReInitlaize IRP fields with NULL, causes no more 
			pIrpContext->ListEntry.Flink = pIrpContext->ListEntry.Blink = NULL;
			pIrpContext->Irp = NULL;

			pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

#if TARGET_SIDE
			if (LG_IS_SECONDARY_MODE(pSftk_Lg))
			{
				IoSetCompletionRoutine(pIrp, NULL, NULL, FALSE, FALSE, FALSE);	

				if ( (LG_IS_SECONDARY_WITH_TRACKING(pSftk_Lg)) )
				{ // set bit here
					// Update Bitmaps
					bPstoreFlush = sftk_Update_bitmap(	Sftk_Dev, 
														pIrpStack->Parameters.Write.ByteOffset.QuadPart,
														pIrpStack->Parameters.Write.Length);

					if ( (OS_IsFlagSet( Sftk_Dev->SftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == TRUE) &&
						 (Sftk_Dev->SftkLg->LastShutDownUpdated == FALSE) )
					{ // After Boot time code
						Sftk_Dev->SftkLg->LastShutDownUpdated = TRUE;
						sftk_lg_update_lastshutdownKey( Sftk_Dev->SftkLg, FALSE);
					}

					if ( (bPstoreFlush == TRUE) 
							&&
						 (OS_IsFlagSet( Sftk_Dev->SftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == FALSE) )
					{ // Flush LRDB to Pstore File synchronously...
						OS_ASSERT( Sftk_Dev->PsDev != NULL);

						if (fileHandle == NULL)
						{
							OS_ASSERT(OS_IsFlagSet( Sftk_Dev->SftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == FALSE);

							status = sftk_open_pstore( &Sftk_Dev->SftkLg->PStoreFileName, &fileHandle, FALSE);
							if ( !NT_SUCCESS(status) ) 
							{ // failed to open or create pstore file...
								WCHAR				wszStringchar1[64];

								DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: sftk_open_pstore(%S, Sftk_Dev 0x%08x for Device %s ) Failed with status 0x%08x !\n", 
															Sftk_Dev->SftkLg->PStoreFileName.Buffer, Sftk_Dev, Sftk_Dev->Vdevname, status ));
								DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just continue the operations...\n"));

								swprintf(  wszStringchar1, L"Dev Master IRP Processing Thread");
								sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_OPEN_ERROR, status, 
													  0, Sftk_Dev->SftkLg->PStoreFileName.Buffer, wszStringchar1);
							}

							// old code was : lrdb_offset.QuadPart = ((LONGLONG) (Sftk_Dev->lrdb_offset)) << DEV_BSHIFT;  // to bytes
							lrdb_offset.QuadPart = (LONGLONG) Sftk_Dev->PsDev->LrdbOffset;
						}

						Sftk_Dev->Statistics.PstoreLrdbUpdateCounts++;
						Sftk_Dev->SftkLg->Statistics.PstoreLrdbUpdateCounts++;

#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
								// Update TimeStamp in SFTK_PS_BITMAP_REGION_HDR
						OS_PerfGetClock( &Sftk_Dev->Lrdb.pPsBitmapHdr->data.TimeStamp, NULL);

						status = sftk_write_pstore( fileHandle, &lrdb_offset, 
													Sftk_Dev->Lrdb.pPsBitmapHdr,
													Sftk_Dev->Lrdb.BitmapSizeBlockAlign );
#else
						status = sftk_write_pstore( fileHandle, &lrdb_offset, 
													Sftk_Dev->Lrdb.pBits,
													Sftk_Dev->Lrdb.BitmapSizeBlockAlign );
#endif
						if ( !NT_SUCCESS(status) ) 
						{ // failed to open or create pstore file...
							WCHAR	wstr[64];
							DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: sftk_write_pstore(%S, Sftk_Dev 0x%08x for Device %s O %I64d S %d ) Failed with status 0x%08x !\n", 
													Sftk_Dev->SftkLg->PStoreFileName.Buffer, Sftk_Dev, Sftk_Dev->Vdevname, 
													lrdb_offset.QuadPart, Sftk_Dev->Lrdb.BitmapSizeBlockAlign, status ));
							DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just continue the operations...\n"));
							// Log Event
							swprintf( wstr, L"Updating Bitmaps");
							sftk_LogEventNum2Wchar2(	GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
														0, Sftk_Dev->cdev, Sftk_Dev->SftkLg->LGroupNumber, 
														Sftk_Dev->SftkLg->PStoreFileName.Buffer, wstr);

						}
					} // Flush LRDB to Pstore File synchronously...
				} // if ( (LG_IS_SECONDARY_WITH_TRACKING(pSftk_Lg)) )
				else
				{
					DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: BUG FIXME FIXME : LG %d In Secondary mode without tracking, (Sftk_Dev 0x%08x for Device %s ) IRP must not come here !!! \n", 
											pSftk_Lg->LGroupNumber, Sftk_Dev, Sftk_Dev->Vdevname));
					OS_ASSERT(FALSE);
				}
				goto callDriver;
			} // if (LG_IS_SECONDARY_MODE(pSftk_Lg))
#endif // #if TARGET_SIDE

			currentState = sftk_lg_get_state(pSftk_Lg);
			switch( currentState )
			{
				case SFTK_MODE_SMART_REFRESH:
				case SFTK_MODE_NORMAL:		// Use BAB

							// Allocate MM Pkt and Queue it and set Irp Completion routine with proper context
							sftk_dev_alloc_mem_for_new_writes(	Sftk_Dev, pIrpStack, pIrp );

				case SFTK_MODE_FULL_REFRESH:
				case SFTK_MODE_TRACKING:	// No BAB usage 
									
							if ( (currentState == SFTK_MODE_FULL_REFRESH) || (currentState == SFTK_MODE_TRACKING) || 
								 (currentState == SFTK_MODE_PASSTHRU) )
							{	// Do not register Completion routine as we do not need it for this operation
								IoSetCompletionRoutine(pIrp, NULL, NULL, FALSE, FALSE, FALSE);	
							}
					
							// Update Bitmaps
							bPstoreFlush = sftk_Update_bitmap(	Sftk_Dev, 
																pIrpStack->Parameters.Write.ByteOffset.QuadPart,
																pIrpStack->Parameters.Write.Length);

							if ( (OS_IsFlagSet( Sftk_Dev->SftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == TRUE) &&
								 (Sftk_Dev->SftkLg->LastShutDownUpdated == FALSE) )
							{
								Sftk_Dev->SftkLg->LastShutDownUpdated = TRUE;
								sftk_lg_update_lastshutdownKey( Sftk_Dev->SftkLg, FALSE);
							}

							if ( (bPstoreFlush == TRUE) 
									&&
								 (OS_IsFlagSet( Sftk_Dev->SftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == FALSE) )
							{ // Flush LRDB to Pstore File synchronously...
								OS_ASSERT( Sftk_Dev->PsDev != NULL);

								if (fileHandle == NULL)
								{
									OS_ASSERT(OS_IsFlagSet( Sftk_Dev->SftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == FALSE);

									status = sftk_open_pstore( &Sftk_Dev->SftkLg->PStoreFileName, &fileHandle, FALSE);
									if ( !NT_SUCCESS(status) ) 
									{ // failed to open or create pstore file...
										WCHAR				wszStringchar1[64];

										DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: sftk_open_pstore(%S, Sftk_Dev 0x%08x for Device %s ) Failed with status 0x%08x !\n", 
																	Sftk_Dev->SftkLg->PStoreFileName.Buffer, Sftk_Dev, Sftk_Dev->Vdevname, status ));
										DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just continue the operations...\n"));

										swprintf(  wszStringchar1, L"Dev Master IRP Processing Thread");
										sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_OPEN_ERROR, status, 
															  0, Sftk_Dev->SftkLg->PStoreFileName.Buffer, wszStringchar1);
									}

									// old code was : lrdb_offset.QuadPart = ((LONGLONG) (Sftk_Dev->lrdb_offset)) << DEV_BSHIFT;  // to bytes
									lrdb_offset.QuadPart = (LONGLONG) Sftk_Dev->PsDev->LrdbOffset;
								}

								Sftk_Dev->Statistics.PstoreLrdbUpdateCounts++;
								Sftk_Dev->SftkLg->Statistics.PstoreLrdbUpdateCounts++;

#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
								// Update TimeStamp in SFTK_PS_BITMAP_REGION_HDR
								OS_PerfGetClock( &Sftk_Dev->Lrdb.pPsBitmapHdr->data.TimeStamp, NULL);

								status = sftk_write_pstore( fileHandle, &lrdb_offset, 
															Sftk_Dev->Lrdb.pPsBitmapHdr,
															Sftk_Dev->Lrdb.BitmapSizeBlockAlign );
#else
								status = sftk_write_pstore( fileHandle, &lrdb_offset, 
															Sftk_Dev->Lrdb.pBits,
															Sftk_Dev->Lrdb.BitmapSizeBlockAlign );
#endif
								if ( !NT_SUCCESS(status) ) 
								{ // failed to open or create pstore file...
									WCHAR	wstr[64];
									DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: sftk_write_pstore(%S, Sftk_Dev 0x%08x for Device %s O %I64d S %d ) Failed with status 0x%08x !\n", 
															Sftk_Dev->SftkLg->PStoreFileName.Buffer, Sftk_Dev, Sftk_Dev->Vdevname, 
															lrdb_offset.QuadPart, Sftk_Dev->Lrdb.BitmapSizeBlockAlign, status ));
									DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just continue the operations...\n"));
									// Log Event
									swprintf( wstr, L"Updating Bitmaps");
									sftk_LogEventNum2Wchar2(	GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
																0, Sftk_Dev->cdev, Sftk_Dev->SftkLg->LGroupNumber, 
																Sftk_Dev->SftkLg->PStoreFileName.Buffer, wstr);

								}
							} // Flush LRDB to Pstore File synchronously...

							if (currentState == SFTK_MODE_TRACKING)
							{ // TODO : Revisit this and put smart logic to change state from tracking to Smart Refresh like time out
								pSftk_Lg->TrackingIoCount ++;	// Incremenrs incoming IO count for tracking mode

								// TODO : call Veera's API to make sure we have atleast one socket 
								// open before we change state otherwise we should not change the state
								if ( (GSftk_Config.Mmgr.MM_Initialized == TRUE)	&&
									 (pSftk_Lg->TrackingIoCount >= MAX_IO_FOR_TRACKING_TO_SMART_REFRESH) && 
									 (pSftk_Lg->UserChangedToTrackingMode == FALSE)  && 
									 (pSftk_Lg->DualLrdbMode	== FALSE) )
								{
									if (sftk_lg_is_socket_alive(pSftk_Lg) == TRUE)
									{ // connection is alive
										// if after boot time, if we were running porev state = Full refresh run full refresh else
										// do smart refresh 
										if (OS_IsFlagSet( pSftk_Lg->flags, SFTK_LG_FLAG_AFTER_BOOT_RESUME_FULL_REFRESH) == TRUE) 
										{ // After reboot, resume previous running full refresh 
											OS_ASSERT(pSftk_Lg->PsHeader != NULL);
											OS_ASSERT(pSftk_Lg->PsHeader->LgInfo.state == SFTK_MODE_FULL_REFRESH);

											sftk_lg_change_State( pSftk_Lg, pSftk_Lg->state, SFTK_MODE_FULL_REFRESH, FALSE);

											OS_ClearFlag( pSftk_Lg->flags, SFTK_LG_FLAG_AFTER_BOOT_RESUME_FULL_REFRESH);
										}
										else
										{
											sftk_lg_change_State( pSftk_Lg, pSftk_Lg->state, SFTK_MODE_SMART_REFRESH, FALSE);
										}
									}
									else
									{ // try to start connections, next visit will change state ...
										RtlZeroMemory( &sm_Params, sizeof(sm_Params));

										// set all values as default....TODO: Service must have passed this default values during LG create !!
										sm_Params.lgnum						= pSftk_Lg->LGroupNumber;
										sm_Params.nSendWindowSize			= CONFIGURABLE_MAX_SEND_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsSendAtaTime);	
										sm_Params.nMaxNumberOfSendBuffers	= DEFAULT_MAX_SEND_BUFFERS;	// 5 defined in ftdio.h
										sm_Params.nReceiveWindowSize		= CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit,pSftk_Lg->NumOfPktsRecvAtaTime);
										sm_Params.nMaxNumberOfReceiveBuffers= DEFAULT_MAX_RECEIVE_BUFFERS;	// 5 defined in ftdio.h
										sm_Params.nChunkSize				= 0;	 
										sm_Params.nChunkDelay				= 0;	 

										COM_StartPMD( &pSftk_Lg->SessionMgr, &sm_Params);
										pSftk_Lg->TrackingIoCount = 0;	// reset counter, so after a while we try to change state
									}
								}
							}
							break;
				
				default:	// IRP not suppose to be here anymore.
							DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: BUG FIXME FIXME (Sftk_Dev 0x%08x for Device %s ) state is 0x%08x !!! IRP never should come here in this state !!!\n", 
										Sftk_Dev, Sftk_Dev->Vdevname, currentState));
							OS_ASSERT(FALSE);	
							break;
			} // switch( currentState )
#if TARGET_SIDE
callDriver:
#endif
			IoCallDriver( Sftk_Dev->DevExtension->TargetDeviceObject, pIrp);
			
		} // for : scan thru each and every IRP list 
    
	} // while (Sftk_Dev->ThreadShouldStop) 

	// its always safe to flush LRDB and HRDB bitmaps on to Pstore file before we terminates
	// TODO Later we do that update successfull shutdown bit information on persistenent storage, registry or
	// binary file..... We may do this at service level or at shutdown notification time of driver level....
	Sftk_Dev->Statistics.PstoreLrdbUpdateCounts++;
	Sftk_Dev->SftkLg->Statistics.PstoreLrdbUpdateCounts++;

	if (OS_IsFlagSet( Sftk_Dev->SftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == FALSE)
	{
		#if PS_HRDB_VALIDATION_USE_TIMESTAMP // USe TimeStamp For HRDB Validation on pstore file
		status = sftk_flush_all_bitmaps_to_pstore( Sftk_Dev, TRUE, fileHandle, FALSE, FALSE);
		#else
		status = sftk_flush_all_bitmaps_to_pstore( Sftk_Dev, TRUE, fileHandle, TRUE, TRUE);
		#endif

		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			WCHAR	wstr[64];

			DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: sftk_flush_all_bitmaps_to_pstore(%S, Sftk_Dev 0x%08x for Device %s ) Failed with status 0x%08x !\n", 
																Sftk_Dev->SftkLg->PStoreFileName.Buffer, 
																Sftk_Dev, 
																Sftk_Dev->Vdevname, 
																status ));
			DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just continue the operations...\n"));

			// Log Event Message
			swprintf( wstr, L"Flushing All Bitmaps");

			sftk_LogEventNum2Wchar2(GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
									0, Sftk_Dev->cdev, Sftk_Dev->SftkLg->LGroupNumber, 
									Sftk_Dev->SftkLg->PStoreFileName.Buffer, wstr);
		}
	}

	if (fileHandle)
	{
		status = ZwClose(fileHandle);

		if ( !NT_SUCCESS(status) )
		{
			DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: ZwClose(%S, Sftk_Dev 0x%08x for Device %s ) Failed with status 0x%08x !\n", 
											Sftk_Dev->SftkLg->PStoreFileName.Buffer, Sftk_Dev, Sftk_Dev->Vdevname, status ));
			// FTD_ERR(FTD_WRNLVL, "Unable to close to the pstore, RC = 0x%x.", RC);
		}
	}

	DebugPrint((DBG_THREAD, "sftk_dev_master_thread:: Terminating Dev Master Thread: Sftk_Dev 0x%08x for Device %s ! \n", 
								Sftk_Dev, Sftk_Dev->Vdevname ));

	PsTerminateSystemThread( STATUS_SUCCESS );

} // sftk_dev_master_thread()    

// Acknowledgement Thread:
// repsonbilities is to wait on timeout/signal event 
// starts traversing all Cache manager (BAB for incoming I/O) Queues and prepare new ALRDB bitmap
// Merge ALRDB with LRDB and make this LRDB as new LRDB. Update it onto  Pstore file.
// If current mode is Tracking Mode or smart Refresh mode, it also builds AHRDB from Queue list.
// Merge AHRDB with HRDB and make this HRDB as new HRDB. 
NTSTATUS
sftk_dev_alloc_mem_for_new_writes(	PSFTK_DEV			Sftk_Dev, 
									PIO_STACK_LOCATION	IrpStack,
									PIRP				Irp)
{
	NTSTATUS			status		= STATUS_SUCCESS;
	PSFTK_LG			pSftk_Lg	= Sftk_Dev->SftkLg;
	BOOLEAN				bUnMapPages = FALSE;
	BOOLEAN				firstAlloc  = TRUE;
	PLIST_ENTRY			pAnchorList = NULL;
	PLIST_ENTRY			plistEntry;
	PVOID				systemBuffer, pMM_Buffer;
	PUCHAR				inDataBuffer;
	ULONG				totalSizeDone, sizeToAllocate;
	LARGE_INTEGER		byteOffset;
	
	// Get a system-space pointer to the user's buffer.  A system
	// address must be used because we may already have left the
	// original caller's address space.
	if (Irp->MdlAddress == NULL) 
	{
		DebugPrint((DBG_ERROR, "sftk_dev_alloc_mem_for_new_writes:: IRP has MdlAddress NULL using Irp->UserBuffer 0x%08x !! (LG %d Sftk_Dev 0x%08x - %s, Offset %I64d Size %d) FIXME FIXME **\n", 
									Irp->UserBuffer, pSftk_Lg->LGroupNumber, Sftk_Dev, Sftk_Dev->Vdevname,
									IrpStack->Parameters.Write.ByteOffset.QuadPart,
									IrpStack->Parameters.Write.Length));
		systemBuffer = Irp->UserBuffer;
	}
	else 
	{ // Requires to get system buffer from MDL
		bUnMapPages = (Irp->MdlAddress->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL))? FALSE : TRUE;
		#ifdef NTFOUR
		systemBuffer = MmGetSystemAddressForMdl( Irp->MdlAddress );
		#else		// MUST use this fct for WIN2K, otherwise BSOD exposure
		systemBuffer = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, HighPagePriority );
		// systemBuffer = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );
		#endif
	}

	if (systemBuffer == NULL) 
	{ // Error !! 
		DebugPrint((DBG_ERROR, "** BUG FIXME FIXME sftk_dev_alloc_mem_for_new_writes:: IRP has NULL systemBuffer !! (LG %d Sftk_Dev 0x%08x - %s, Offset %I64d Size %d) FIXME FIXME **\n", 
									pSftk_Lg->LGroupNumber, Sftk_Dev, Sftk_Dev->Vdevname,
									IrpStack->Parameters.Write.ByteOffset.QuadPart,
									IrpStack->Parameters.Write.Length));
		OS_ASSERT(FALSE);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	OS_ASSERT( systemBuffer != NULL);
	OS_ASSERT( pSftk_Lg != NULL);

	byteOffset.QuadPart	= IrpStack->Parameters.Write.ByteOffset.QuadPart;
	inDataBuffer		= systemBuffer;
	totalSizeDone		= 0;
	do 
	{
		sizeToAllocate	= pSftk_Lg->MaxTransferUnit;	// Max buffer size per LG
		if ( (totalSizeDone + sizeToAllocate) > IrpStack->Parameters.Write.Length)
		{ // Last boundary condition for allocation, truncate allocation if needed
			sizeToAllocate = IrpStack->Parameters.Write.Length - totalSizeDone;
		}
		OS_ASSERT( (sizeToAllocate % SECTOR_SIZE) == 0);

		// If LG has more than 1 device, than Grab Lock to synchronize BAB Overflow condition detect 
		// against other Device thread 
		if (pSftk_Lg->LgDev_List.NumOfNodes > 1)
			OS_ACQUIRE_LOCK( &pSftk_Lg->AckLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

		// Call Memory manger to allocate Hard-Header + soft-header + Data Buffer
		pMM_Buffer = mm_alloc_buffer( pSftk_Lg, sizeToAllocate, SOFT_HDR, TRUE); 

		// Release lock if acquired before
		if (pSftk_Lg->LgDev_List.NumOfNodes > 1)
			OS_RELEASE_LOCK( &pSftk_Lg->AckLock, NULL);

		if (pMM_Buffer == NULL)
		{ // MM/BAB OverFlow !!!
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		// copy memory to BAB data buffer.
		status = MM_COPY_BUFFER( pMM_Buffer, inDataBuffer, sizeToAllocate, TRUE);
		if (status != STATUS_SUCCESS)
		{ // MM/BAB OverFlow !!!
			DebugPrint((DBG_ERROR, "sftk_dev_alloc_mem_for_new_writes: MM_COPY_BUFFER() Failed with status 0x%08x for Size %d, Considering BAB OverFlow !!! \n",
											status, sizeToAllocate));
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		// MM/BAB Successfully allocated !!! Initialize Soft hdr 
		Proto_MMInitSoftHDr( pMM_Buffer,  byteOffset.QuadPart, sizeToAllocate, Sftk_Dev);

		InitializeListHead(MM_GetContextList(pMM_Buffer));

		if (firstAlloc == TRUE)
		{
			pAnchorList = MM_GetContextList(pMM_Buffer);
			firstAlloc = FALSE;
		}
		else
		{
			InsertTailList( pAnchorList, MM_GetContextList(pMM_Buffer));
		}

		inDataBuffer		= (PUCHAR) ((ULONG) inDataBuffer + sizeToAllocate);
		byteOffset.QuadPart += sizeToAllocate;
		totalSizeDone		+= sizeToAllocate;

	} while(totalSizeDone != IrpStack->Parameters.Write.Length); // do-While : Do Segmentation of Pkts if needed

	// If we have locked IRP buffer than unlocked it here, since not needed now
	if (bUnMapPages == TRUE)
		MmUnmapLockedPages(systemBuffer, Irp->MdlAddress);

	if (!NT_SUCCESS(status))
	{ // Failed Allocation, BAB OVERFLOW Change state
		if(pAnchorList != NULL)
		{ // Free all Allocated pkts from Listentry 
			while( !IsListEmpty(pAnchorList) )
			{
				plistEntry = RemoveHeadList(pAnchorList);
				pMM_Buffer = MM_GetMMHolderFromContextList(plistEntry);

				InitializeListHead(MM_GetContextList(pMM_Buffer));
				mm_free_buffer( pSftk_Lg, pMM_Buffer);
			}

			// now free first node
			pMM_Buffer = MM_GetMMHolderFromContextList(pAnchorList);
			InitializeListHead(pAnchorList);
			mm_free_buffer( pSftk_Lg, pMM_Buffer);
		}

		DebugPrint((DBG_ERROR, "sftk_dev_alloc_mem_for_new_writes:: mm_alloc_buffer(LG %d Sftk_Dev 0x%08x - %s, Offset %I64d Size %d) return NULL ** BAB OVERFLOW ** Changing state to Tracking!\n", 
										pSftk_Lg->LGroupNumber, Sftk_Dev, Sftk_Dev->Vdevname,
										IrpStack->Parameters.Write.ByteOffset.QuadPart,
										IrpStack->Parameters.Write.Length));

		// This API will realese lock when it come back. if it has acquired !!
		sftk_lg_change_State( pSftk_Lg, pSftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
		return status;
	} // Failed Allocation, BAB OVERFLOW Change state

	OS_ASSERT(pAnchorList != NULL);

	// first insert first node into PendingList
	pMM_Buffer = MM_GetMMHolderFromContextList( pAnchorList );
	QM_Insert( pSftk_Lg, pMM_Buffer, PENDING_QUEUE, TRUE);

	// insert all other Pkts in PendingQueue, if exist
	for(	plistEntry = pAnchorList->Flink; 
			plistEntry != pAnchorList; 
			plistEntry = plistEntry->Flink )
	{
		pMM_Buffer = MM_GetMMHolderFromContextList(plistEntry);
		// Call API to Put this Hard-Header into Pending Link List
		QM_Insert( pSftk_Lg, pMM_Buffer, PENDING_QUEUE, TRUE);
	}
	
	// set this Hard-Header pointer as completion context of completion routine
	IoSetCompletionRoutine(Irp, sftk_write_completion, pAnchorList, TRUE, TRUE, TRUE);

	return STATUS_SUCCESS;
} // sftk_dev_alloc_mem_for_new_writes()

NTSTATUS
sftk_write_completion(	IN PDEVICE_OBJECT       DeviceObject,
						IN PIRP                 Irp,
						IN PVOID				Context)
{
	PIO_STACK_LOCATION	currentIrpStack	= IoGetCurrentIrpStackLocation(Irp);
	PLIST_ENTRY			pAnchorList		= Context;
	PLIST_ENTRY			plistEntry;
	PVOID				pMM_Buffer;
	PSFTK_DEV			pSftk_Dev;

	OS_ASSERT(Context != NULL);

	// PIRP_CONTEXT pIrpContext = (PIRP_CONTEXT) &Irp->Tail.Overlay.DriverContext[0];

	DebugPrint( ( DBG_COMPLETION, "sftk_write_completion: WRITE: DeviceObject %X Irp %X Context %X, Offset %I64d, Size %d, IRP complteted with status 0x%08x Information %d \n",
										DeviceObject, Irp, Context,
										currentIrpStack->Parameters.Write.ByteOffset.QuadPart,
										currentIrpStack->Parameters.Write.Length,
										Irp->IoStatus.Status,
										Irp->IoStatus.Information));

	if (Context)
	{ // TODO : Call MM API to move this Soft Header from Pending to Commit queue
		if ( NT_SUCCESS( Irp->IoStatus.Status) )
		{ // success
			// Move Pkts from Pending Queue To commit Queue
			// Move first pkts
			pMM_Buffer = MM_GetMMHolderFromContextList( pAnchorList );

			pSftk_Dev = ((PMM_SOFT_HDR) (MM_GetHdr(pMM_Buffer)))->SftkDev;
			OS_ASSERT(pSftk_Dev != NULL);
			OS_ASSERT(pSftk_Dev->SftkLg != NULL);

			QM_Move(pSftk_Dev->SftkLg, pMM_Buffer, PENDING_QUEUE, COMMIT_QUEUE, TRUE);

			// Move rest of pkts if exist
			while( !IsListEmpty(pAnchorList) )
			{
				plistEntry = RemoveHeadList(pAnchorList);
				pMM_Buffer = MM_GetMMHolderFromContextList(plistEntry);

				InitializeListHead(MM_GetContextList(pMM_Buffer));

				QM_Move(pSftk_Dev->SftkLg, pMM_Buffer, PENDING_QUEUE, COMMIT_QUEUE, TRUE);
			}
			InitializeListHead( pAnchorList );
		}
		else
		{ // Failed
			WCHAR	wstr1[40], wstr2[40], wstr3[40], wstr4[40];

			DebugPrint( ( DBG_ERROR, "sftk_write_completion: Failed WRITE: DeviceObject %X Irp %X Context %X, Offset %I64d, Size %d, IRP complteted with status 0x%08x Information %d \n",
										DeviceObject, Irp, Context,
										currentIrpStack->Parameters.Write.ByteOffset.QuadPart,
										currentIrpStack->Parameters.Write.Length,
										Irp->IoStatus.Status,
										Irp->IoStatus.Information));
			// DebugPrint((DBG_ERROR, "sftk_write_completion:: Removing Failed Completed IRP's Soft header from Pending Queue and freeing up !!!\n"));
			// DebugPrint((DBG_ERROR, "sftk_write_completion:: Freeing BAB MM Buffer since IRP got Failed !!\n"));

			// Free all Allocated pkts from Listentry 
			// Move Pkts from Pending Queue To commit Queue
			// Move first pkts
			pMM_Buffer = MM_GetMMHolderFromContextList( pAnchorList );

			pSftk_Dev = ((PMM_SOFT_HDR) (MM_GetHdr(pMM_Buffer)))->SftkDev;
			OS_ASSERT(pSftk_Dev != NULL);
			OS_ASSERT(pSftk_Dev->SftkLg != NULL);

			while( !IsListEmpty(pAnchorList) )
			{
				plistEntry = RemoveHeadList(pAnchorList);
				pMM_Buffer = MM_GetMMHolderFromContextList(plistEntry);

				QM_Remove(pSftk_Dev->SftkLg, pMM_Buffer, PENDING_QUEUE, TRUE);

				InitializeListHead(MM_GetContextList(pMM_Buffer));
				mm_free_buffer( pSftk_Dev->SftkLg, pMM_Buffer);
			}
			// now free first node
			pMM_Buffer = MM_GetMMHolderFromContextList(pAnchorList);
			QM_Remove(pSftk_Dev->SftkLg, pMM_Buffer, PENDING_QUEUE, TRUE);

			InitializeListHead( pAnchorList );
			mm_free_buffer( pSftk_Dev->SftkLg, pMM_Buffer);

			// mm_free_buffer(pSftk_Dev->SftkLg, Context);

			/* Dispatch level problems
			// Log Event
			// TODO : Test this for Dispatch Level, Does it works !!
			swprintf( wstr1, L"%d", pSftk_Dev->cdev);
			swprintf( wstr2, L"%d", pSftk_Dev->SftkLg->LGroupNumber);
			swprintf( wstr3, L"%I64d",currentIrpStack->Parameters.Write.ByteOffset.QuadPart );
			swprintf( wstr4, L"%d", currentIrpStack->Parameters.Write.Length);
			sftk_LogEventWchar4(GSftk_Config.DriverObject, MSG_REPL_SRC_DEVICE_WRITE_FAILED, Irp->IoStatus.Status, 
									0, wstr1, wstr2, wstr3, wstr4);
			*/

		}
	}
	else
	{
		DebugPrint((DBG_ERROR, "** BUG FIXME FIXME : sftk_write_completion:: Completion Routine Context was set to NULL !!! ** PANIC FIXME FIXME ...!\n"));
		OS_ASSERT(FALSE);
	}

	// Propogate IRP Pending states
	if (Irp->PendingReturned) 
	{
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;	// since we are done with this IRP, we must return status success
							// if we need to call explicitly IoCompleteRequest for this IRP than only we must return
							// STATUS_MORE_PROCESSING_REQUIRED
} // sftk_write_completion()

// Acknowledgement Thread:
// repsonbilities is to wait on timeout/signal event 
// starts traversing all Cache manager (BAB for incoming I/O) Queues and prepare new ALRDB bitmap
// Merge ALRDB with LRDB and make this LRDB as new LRDB. Update it onto  Pstore file.
// If current mode is Tracking Mode or smart Refresh mode, it also builds AHRDB from Queue list.
// Merge AHRDB with HRDB and make this HRDB as new HRDB. 
VOID
sftk_acknowledge_lg_thread( PSFTK_LG	Sftk_Lg)
{
	NTSTATUS		status				= STATUS_SUCCESS;
	PSFTK_DEV		pSftk_Dev			= NULL;		
	LONG			currentState;
	BOOLEAN			bJmpForWaiting, bTimeOut;

	OS_PERF;
	
	DebugPrint((DBG_THREAD, "sftk_acknowledge_lg_thread:: Starting Thread: Sftk_Lg 0x%08x for LG Num %d ! \n", 
								Sftk_Lg, Sftk_Lg->LGroupNumber));
	
    // Set thread priority to lowest realtime level /2 means medium Low realtime level = 8.
    KeSetPriorityThread(KeGetCurrentThread(), (LOW_REALTIME_PRIORITY));	// LOW_REALTIME_PRIORITY = 16, so we setting to 8

    while (Sftk_Lg->AckThreadShouldStop == FALSE) 
    {
        // Wait for a request from the dispatch routines.
        // KeWaitForSingleObject won't return error here - this thread
        // isn't alertable and won't take APCs, and we're not passing in
        // a timeout.
        try 
        {
            status = KeWaitForSingleObject(	(PVOID) &Sftk_Lg->AckStateChangeSemaphore,
													Executive,
													KernelMode,
													FALSE,
													&Sftk_Lg->AckWakeupTimeout );
        } 
        except (sftk_ExceptionFilterDontStop(GetExceptionInformation(), GetExceptionCode()) ) 
        {	// We encountered an exception somewhere, eat it up.
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

            DebugPrint((DBG_ERROR,"sftk_acknowledge_lg_thread:: KeWaitForSingleObject() crashed: EXCEPTION_EXECUTE_HANDLER, Exception code = 0x%08x.\n", GetExceptionCode() ));
        }

		if (Sftk_Lg->AckThreadShouldStop == TRUE)
			break;	// terminate thread

#if TARGET_SIDE
		if (LG_IS_SECONDARY_MODE(Sftk_Lg))
		{
			continue;
		}
#endif

		bTimeOut = FALSE;
		switch(status)
		{ 
			case STATUS_ALERTED: // The wait is completed because of an alert to the thread. 
								DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread:: LG %d,KeWaitForSingleObject(): returned STATUS_ALERTED (0x%08x) !!\n", 
														Sftk_Lg->LGroupNumber, status ));
								OS_ASSERT(FALSE);	// Never get this error, since No Alert and Kernemode
								break;
			case STATUS_USER_APC: // A user APC was delivered to the current thread before the specified Timeout interval expired. 
								DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread:: LG %d,KeWaitForSingleObject(): returned STATUS_USER_APC (0x%08x) !!\n", 
														Sftk_Lg->LGroupNumber, status ));
								OS_ASSERT(FALSE);	// Never get this error, since No Alert and Kernemode
								break;
			case STATUS_TIMEOUT:	// A time out occurred before the specified set of wait conditions was met. 
								// This value can be returned when an explicit time-out value of zero is 
								// specified, but the specified set of wait conditions cannot be met immediately
								DebugPrint((DBG_THREAD, "sftk_acknowledge_lg_thread:: LG %d,KeWaitForSingleObject(): returned STATUS_TIMEOUT Timeout %I64d (status 0x%08x) !!\n", 
														Sftk_Lg->LGroupNumber, Sftk_Lg->AckWakeupTimeout.QuadPart, status ));
								bTimeOut = TRUE;
								break;

			case STATUS_SUCCESS:	// Event got signalled
								DebugPrint((DBG_THREAD, "sftk_acknowledge_lg_thread:: LG %d,Signalled AckStateChangeSemaphore(): status 0x%08x !!\n", 
													Sftk_Lg->LGroupNumber, status ));
								break;
			default:
								DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread:: LG %d,KeWaitForSingleObject(): Default status 0x%08x !!\n", 
										Sftk_Lg->LGroupNumber, status ));
								OS_ASSERT(FALSE);
								break;

		} // switch(status)

		// we either got signaled or got time out, Check State before doing anything
		bJmpForWaiting = FALSE; // default don't jump for sleep again

		currentState = sftk_lg_get_state(Sftk_Lg);
		switch (currentState)
		{
			case SFTK_MODE_PASSTHRU: 
			case SFTK_MODE_BACKFRESH: 
							bJmpForWaiting = TRUE;	// nothing to do
							DebugPrint((DBG_THREAD, "sftk_acknowledge_lg_thread: LgNum %d:state %s:nothing to do, going for sleep again \n",
											Sftk_Lg->LGroupNumber, 
											(currentState==SFTK_MODE_PASSTHRU) ?"SFTK_MODE_PASSTHRU":"SFTK_MODE_BACKREFRESH" ));
							break;

			case SFTK_MODE_FULL_REFRESH:
			case SFTK_MODE_SMART_REFRESH:	
							bJmpForWaiting = TRUE;	// nothing to do			
							DebugPrint((DBG_THREAD, "sftk_acknowledge_lg_thread: LgNum %d:state %s:nothing to do, going for sleep again \n",
											Sftk_Lg->LGroupNumber, 
											(currentState==SFTK_MODE_FULL_REFRESH) ?"SFTK_MODE_FULL_REFRESH":"SFTK_MODE_SMART_REFRESH" ));
							break;

			case SFTK_MODE_NORMAL:	// must be timeout case only
							DebugPrint((DBG_THREAD, "sftk_acknowledge_lg_thread: LgNum %d:state %s:Must be timeout case only\n",
											Sftk_Lg->LGroupNumber, 
											"SFTK_MODE_NORMAL"));
							//OS_ASSERT(bTimeOut == TRUE); 	
							break;

			case SFTK_MODE_TRACKING: // must be event signal case only	
							if (bTimeOut == TRUE)
							{
								bJmpForWaiting = TRUE; // don't do any work
							}
#if DBG
							else
							{
							DebugPrint((DBG_THREAD, "sftk_acknowledge_lg_thread: LgNum %d:state %s:Must be event Signal case only\n",
												Sftk_Lg->LGroupNumber, "SFTK_MODE_TRACKING"));
							}
#endif
							
							break;
			default:	
							bJmpForWaiting = TRUE;	// can't do anything
							DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread: BUG FIXME FIXME :LgNum %d:unknown state 0x%08x: FIXME FIXME\n",
											Sftk_Lg->LGroupNumber, 
											currentState));
							OS_ASSERT(FALSE);
							break;
		} // switch (currentState)

		if (bJmpForWaiting == TRUE) 
		{
			continue;	// go for sleep again
		}
		
		Sftk_Lg->UpdateHRDB = FALSE;	// Alweys initlaize as FALSE means not to update HRDB

		KeClearEvent( &Sftk_Lg->EventAckFinishBitmapPrep);

		// ACK Lock: this lock will stop all incoming IO processing till this ACK thread
		// Prepares bitmap and done with its work
		OS_ACQUIRE_LOCK( &Sftk_Lg->AckLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

		if (currentState == SFTK_MODE_TRACKING)
		{	// for tracking mode only we nedd Hrdb Bitmaps
			Sftk_Lg->UpdateHRDB = TRUE;
		}

		// Prepare ALRDB and HRDB Bitmap so we can update it with queue packets.
		OS_PERF_STARTTIME;

		status = sftk_ack_prepare_bitmaps( Sftk_Lg);

		OS_PERF_ENDTIME(ACK_THREAD_PREPARE_BITMAPS, 0);

		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread:: LG %d, sftk_ack_prepare_bitmaps(): Failed with status 0x%08x !!\n", 
										Sftk_Lg->LGroupNumber, status ));
			OS_ASSERT(FALSE);
		}
#if TARGET_SIDE 
		if (Sftk_Lg->UseSRDB == FALSE) 
		{ // only in non-srdb mode, Dual LRDB mode can be on, causes We do not clean LRDB and SRDB in UseSRDB mode.
#endif
			Sftk_Lg->DualLrdbMode	= TRUE;		// so Other thread continue its work
			Sftk_Lg->CopyLrdb		= FALSE;	// so Other thread will do right things when work is done
#if TARGET_SIDE 
		} // only in non-srdb mode, Dual LRDB mode can be on, causes We do not clean LRDB and SRDB in UseSRDB mode.
#endif

		OS_RELEASE_LOCK( &Sftk_Lg->AckLock, NULL);

		// since we either complete work successfully or stop in middle of smart refresh due
		// to change of events or due to Pnp Removal of disk (disk read failed !!).
		KeSetEvent( &Sftk_Lg->EventAckFinishBitmapPrep, 0, FALSE);

		OS_PERF_STARTTIME;
		// This API will return once all Packets in LG's Cache Manager Queue will get processed and updated 
		// respective ALRDB and HRDB bitmaps . NOTE: We update HRDB only if asked to do so thru LG state.
		if (Sftk_Lg->CacheOverFlow == TRUE)
		{ // call special CACHE manager routine for cleanup BAB along with parsing BAB
			Sftk_Lg->CacheOverFlow = FALSE;	// reset flag value.

			if (Sftk_Lg->DoNotSendOutBandCommandForTracking == FALSE)
			{ // Send OutBand for Full refresh Complete Procotol Command 
				DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread:: Sending LG %d : FTDCHUP %d !! ..\n", 
											Sftk_Lg->LGroupNumber, FTDCHUP));

				status = QM_SendOutBandPkt( Sftk_Lg, TRUE, TRUE, FTDCHUP);

				if (!NT_SUCCESS(status))
				{
					DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread:: LG %d : Failed Otband Proto command %d FTDCHUP !! ..\n", 
											Sftk_Lg->LGroupNumber, FTDCHUP));
				}
				DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread:: LG %d : Returned FTDCHUP %d with status %x !! ..\n", 
											Sftk_Lg->LGroupNumber, FTDCHUP, status));

			}
			Sftk_Lg->DoNotSendOutBandCommandForTracking = FALSE;
			// KeClearEvent( &Sftk_Lg->Event_LGFreeAllMemOfMM );
			status = QM_ScanAllQList( Sftk_Lg, TRUE, TRUE);
			if (!NT_SUCCESS(status))
			{
				DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread:: QM_ScanAllQList( LG %d, FreeAllEntries : TRUE) Failed with status 0x%08x !!\n", 
											Sftk_Lg->LGroupNumber, status ));
				OS_ASSERT(FALSE);
			}
			OS_PERF_ENDTIME(ACK_THREAD_PARSE_AND_DELTE_QUEUE_LIST, 0);

			DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread:: LG %d : QM_SCanAllQList() done !! ..\n", 
											Sftk_Lg->LGroupNumber));

			// Signal the event here to tell MM has free all its memory for current LG
			KeSetEvent( &Sftk_Lg->Event_LGFreeAllMemOfMM, 0, FALSE);
		}
		else
		{ // Call normal Routine, which only parse BAB but not cleanup BAB
			status = QM_ScanAllQList( Sftk_Lg, FALSE, TRUE);
			if (!NT_SUCCESS(status))
			{
				DebugPrint((DBG_ERROR, "sftk_acknowledge_lg_thread:: QM_ScanAllQList( LG %d, FreeAllEntries : FALSE) Failed with status 0x%08x !!\n", 
											Sftk_Lg->LGroupNumber, status ));
				OS_ASSERT(FALSE);
			}
			OS_PERF_ENDTIME(ACK_THREAD_ONLY_PARSE_QUEUE_LIST, 0);
		}

#if TARGET_SIDE 
		if (Sftk_Lg->UseSRDB == FALSE) 
		{ // only in non-srdb mode, Dual LRDB mode can be on, causes We do not clean LRDB and SRDB in UseSRDB mode.
#endif
			// set CopyLrdb to TRUE so IRP processing Thread will copy their resepective LRDB  
			// and then change DualLrdbMode = FALSE.
			Sftk_Lg->CopyLrdb = TRUE;	// No need to grab lock.to change this field.
#if TARGET_SIDE 
		} // only in non-srdb mode, Dual LRDB mode can be on, causes We do not clean LRDB and SRDB in UseSRDB mode.
#endif

	} // while (Sftk_Lg->AckThreadShouldStop == FALSE) 

	DebugPrint((DBG_THREAD, "sftk_acknowledge_lg_thread:: Terminating Thread: Sftk_Lg 0x%08x for LG Num %d ! \n", 
								Sftk_Lg, Sftk_Lg->LGroupNumber));

	PsTerminateSystemThread( STATUS_SUCCESS );
} // sftk_acknowledge_lg_thread

// LG->AckLock must have acquired before calling this API
NTSTATUS
sftk_ack_prepare_bitmaps( PSFTK_LG Sftk_Lg )
{
	NTSTATUS		status		= STATUS_SUCCESS; 
	PSFTK_DEV		pSftk_Dev;
	PLIST_ENTRY		plistEntry;
	ULONG			refreshLastBitIndex, lrdb_refreshLastBitIndex;
	ULONG			startBit, endBit;
	LARGE_INTEGER	byteOffset; 
	INT64			numOfBytesPerBit;
#if DBG
	LONG			currentState = sftk_lg_get_state(Sftk_Lg);
#endif
	// Now first Initalize LRDB and check if we need to build HRDB if yes than also update it.
	for(	plistEntry = Sftk_Lg->LgDev_List.ListEntry.Flink;
			plistEntry != &Sftk_Lg->LgDev_List.ListEntry;
			plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every Device under LG 
		pSftk_Dev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

		if (pSftk_Dev->DevExtension == NULL)
		{	// Ops, Source device is null means Src Disk Device has Removed thru PNP...
			DebugPrint((DBG_ERROR, "sftk_ack_prepare_bitmaps:: LG %d Dev %s (pSftk_Dev->DevExtension == NULL): No Any Packet suppose to exist .. Anyhow Continuing operations..\n", 
									Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname));
		}

		// TODO : Reconsider this :Logic, still it has condition and bugs in this logic..... // PARAG
		refreshLastBitIndex = pSftk_Dev->RefreshLastBitIndex;
#if DBG
		if (currentState == SFTK_MODE_NORMAL)
		{
			DebugPrint((DBG_THREAD, "sftk_ack_prepare_bitmaps:: LG %d Dev %s (state == SFTK_MODE_NORMAL) refreshLastBitIndex 0x%08x must be == DEV_LASTBIT_CLEAN_ALL 0x%08x), else Assert() and FIXME\n", 
							Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, refreshLastBitIndex, DEV_LASTBIT_CLEAN_ALL));
			OS_ASSERT(refreshLastBitIndex == DEV_LASTBIT_CLEAN_ALL);
		}
#endif
#if TARGET_SIDE 
		if ((Sftk_Lg->UseSRDB == FALSE) && (refreshLastBitIndex == DEV_LASTBIT_NO_CLEAN))
#else
		if (refreshLastBitIndex == DEV_LASTBIT_NO_CLEAN)
#endif
		{
			DebugPrint((DBG_ERROR, "sftk_ack_prepare_bitmaps:: FIXME FIXME LG %d Dev %s refreshLastBitIndex 0x%08x == DEV_LASTBIT_NO_CLEAN 0x%08x), Why ? we are here !!! FIXME\n", 
									Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, refreshLastBitIndex, DEV_LASTBIT_NO_CLEAN));
			// OS_ASSERT(FALSE);
			// copy Lrdb to ALrdb
			RtlCopyMemory( pSftk_Dev->ALrdb.pBits, pSftk_Dev->Lrdb.pBits, pSftk_Dev->Lrdb.BitmapSize );
			continue;	// nothing to do
		}

		if ( (refreshLastBitIndex == DEV_LASTBIT_CLEAN_ALL) ) 
		{	// Make complete ALRDB zeros and same as HRDB
#if TARGET_SIDE 
			if (Sftk_Lg->UseSRDB == FALSE) 
				RtlClearAllBits( pSftk_Dev->ALrdb.pBitmapHdr); 
#else
			RtlClearAllBits( pSftk_Dev->ALrdb.pBitmapHdr); 
#endif

			if (Sftk_Lg->UpdateHRDB == TRUE)
				RtlClearAllBits( pSftk_Dev->Hrdb.pBitmapHdr); 
			continue;
		}

		// Clear bits starting from 0 to last bit Index used in smart refresh

#if TARGET_SIDE 
		// first copy Lrdb to ALrdb
		if (Sftk_Lg->UseSRDB == FALSE) 
			RtlCopyMemory( pSftk_Dev->ALrdb.pBits, pSftk_Dev->Lrdb.pBits, pSftk_Dev->Lrdb.BitmapSize );
#else
		// first copy Lrdb to ALrdb
		RtlCopyMemory( pSftk_Dev->ALrdb.pBits, pSftk_Dev->Lrdb.pBits, pSftk_Dev->Lrdb.BitmapSize );
#endif
		
#if TARGET_SIDE 
		if (Sftk_Lg->UseSRDB == FALSE) 
		{ // update ARLDB for new LRDB only in non-SRDB mode
#endif
			// Convert this index into ALRDB Bit Index
			numOfBytesPerBit 	= pSftk_Dev->Hrdb.Sectors_per_bit * SECTOR_SIZE;  
			byteOffset.QuadPart = (LONGLONG) ( (INT64) refreshLastBitIndex * (INT64) numOfBytesPerBit ); 

			startBit = endBit = 0;
			sftk_bit_range_from_offset( &pSftk_Dev->ALrdb, 
										byteOffset.QuadPart, 
										(ULONG) numOfBytesPerBit, 
										&startBit, 
										&endBit );

			lrdb_refreshLastBitIndex = endBit;	
			
			if (lrdb_refreshLastBitIndex <= pSftk_Dev->ALrdb.TotalNumOfBits)
			{
				// TODO : Why we need to increment its safe to not to do that worst to worst it will skip one bit 
				RtlClearBits( pSftk_Dev->ALrdb.pBitmapHdr, 0, (lrdb_refreshLastBitIndex+1) );	// since endBit is zero based we should increment it to 1
			}
			else
			{ // BUG Check
				DebugPrint((DBG_ERROR, "sftk_ack_prepare_bitmaps:: LG %d Dev %s : Bitmap ALRDB Calculation BUG FIXME FIXME ..\n", 
									Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname));
				OS_ASSERT(FALSE);
				RtlClearAllBits( pSftk_Dev->ALrdb.pBitmapHdr ); 
			}
#if TARGET_SIDE 
		} // update ARLDB for new LRDB only in non-SRDB mode
#endif
		// Clear HRDB till specified last index if its requires
		OS_ASSERT( (refreshLastBitIndex + 1) <= pSftk_Dev->Hrdb.TotalNumOfBits);
		
		if (Sftk_Lg->UpdateHRDB == TRUE)
			RtlClearBits( pSftk_Dev->Hrdb.pBitmapHdr, 0, (refreshLastBitIndex+1) );	// since  startinde is Zero based, its ok to add one

	} // for :scan thru each and every Device under LG 

	return STATUS_SUCCESS;
} // sftk_ack_prepare_bitmaps()

NTSTATUS
sftk_ack_update_bitmap(PSFTK_DEV Sftk_Dev, LONGLONG Offset, ULONG Length)
{
	ULONG			numBits, startBit, endBit;

	if (Sftk_Dev == NULL)
	{
		OS_ASSERT(FALSE);
		return STATUS_SUCCESS;
	}

#if 0 // DBG
	/*
	NTSTATUS		status					= STATUS_SUCCESS; 
	// Here check state of sftk_Lg
	switch (sftk_lg_get_state(Sftk_Dev->SftkLg))
	{
		case SFTK_MODE_PASSTHRU:	

					status = STATUS_INVALID_DEVICE_REQUEST;
					DebugPrint((DBG_ERROR, "sftk_ack_update_bitmap: BUG FIXME FIXME  LgNum %d:state: SFTK_MODE_PASSTHRU: We are not suppose to get call here with this state...!!! returning error 0x%08x \n",
							Sftk_Dev->SftkLg->LGroupNumber, status));

					OS_ASSERT(FALSE);
					return status;	// nothing to do here

		case SFTK_MODE_BACKFRESH:	

					status = STATUS_INVALID_DEVICE_REQUEST;
					DebugPrint((DBG_ERROR, "sftk_ack_update_bitmap: BUG FIXME FIXME  LgNum %d:state: SFTK_MODE_BACKFRESH: We are not suppose to get call here with this state...!!! returning error 0x%08x \n",
							Sftk_Dev->SftkLg->LGroupNumber, status));

					OS_ASSERT(FALSE);
					return status;	// nothing to do here

		case SFTK_MODE_NORMAL:	
		case SFTK_MODE_TRACKING:	
		case SFTK_MODE_FULL_REFRESH:	
		case SFTK_MODE_SMART_REFRESH:	
					break;
		default:
					status = STATUS_INVALID_DEVICE_REQUEST;
					DebugPrint((DBG_ERROR, "sftk_ack_update_bitmap: BUG FIXME FIXME  LgNum %d:state: default: uknown state 0x%08x. We are not suppose to get call here with this state...!!! returning error 0x%08x \n",
							Sftk_Dev->SftkLg->LGroupNumber, sftk_lg_get_state(Sftk_Dev->SftkLg), status));

					OS_ASSERT(FALSE);
					return status;	// nothing to do here
	} // switch (sftk_lg_get_state(Sftk_Dev->SftkLg))
	*/
#endif		

#if TARGET_SIDE 
	if (Sftk_Dev->SftkLg->UseSRDB == FALSE)
	{ // only in non-srdb mode, Dual LRDB mode can be on, causes We do not clean LRDB and SRDB in UseSRDB mode.
#endif
		// first update ALRDB
		sftk_bit_range_from_offset( &Sftk_Dev->ALrdb, Offset, Length, &startBit, &endBit );
		numBits = (endBit-startBit+1);
		OS_ASSERT( (startBit + numBits) <= Sftk_Dev->ALrdb.TotalNumOfBits);	// check boundary conditions
		RtlSetBits( Sftk_Dev->ALrdb.pBitmapHdr, startBit, numBits );
#if TARGET_SIDE 
	} // only in non-srdb mode, Dual LRDB mode can be on, causes We do not clean LRDB and SRDB in UseSRDB mode.
#endif

	// Now update HRDB if its specified to do so..
	if (Sftk_Dev->SftkLg->UpdateHRDB == TRUE)
	{
		sftk_bit_range_from_offset( &Sftk_Dev->Hrdb, Offset, Length, &startBit, &endBit );
		numBits = (endBit-startBit+1);
		OS_ASSERT( (startBit + numBits) <= Sftk_Dev->Hrdb.TotalNumOfBits);	// check boundary conditions
		RtlSetBits( Sftk_Dev->Hrdb.pBitmapHdr, startBit, numBits );
	}

	return STATUS_SUCCESS;
} // sftk_ack_update_bitmap()

#if TARGET_SIDE
// LG target Write Thread: Use only in Secondary mode, 
// - Responsible to process each and every Protocol Pkt arrived from Primary and send its ACk back.
// - uses 2 Qeueue TSend and TReceieve.
// - Does operation for Disk Write, or Journal Write. It also creates Journal files as needed. 
// - Responsible for Journal Apply threadto start its work
VOID
sftk_Target_write_Thread( PSFTK_LG Sftk_LG)
{
	NTSTATUS			status			= STATUS_SUCCESS;
	ANCHOR_LINKLIST		localQueueList;
	PLIST_ENTRY			plistEntry;
	PMM_PROTO_HDR		pMM_Proto_Hdr;
    
	DebugPrint((DBG_THREAD, "sftk_Target_write_Thread:: Starting Thread: LGnum 0x%08x ! \n", 
								Sftk_LG->LGroupNumber));

    // Set thread priority to lowest realtime level.
    // KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while (Sftk_LG->Secondary.TWriteThreadShouldStop == FALSE) 
    {
        // Wait for a Pkt arrival Event 
		if (Sftk_LG->Secondary.TWriteWakeupTimeout.QuadPart == 0)
		{
			Sftk_LG->Secondary.TWriteWakeupTimeout.QuadPart	= DEFAULT_TIMEOUT_FOR_TARGET_WRITE_THREAD;
		}

        try 
        {
            KeWaitForSingleObject(	(PVOID) &Sftk_LG->Secondary.TWriteWorkEvent, // &Sftk_Dev->MasterQueueSemaphore,
									Executive,
									KernelMode,
									FALSE,
									&Sftk_LG->Secondary.TWriteWakeupTimeout );

        } 
        except (sftk_ExceptionFilterDontStop(GetExceptionInformation(), GetExceptionCode()) ) 
        {
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

            // We encountered an exception somewhere, eat it up.
            DebugPrint((DBG_ERROR, "sftk_Target_write_Thread::KeWaitForSingleObject() crashed: EXCEPTION_EXECUTE_HANDLER, Exception code = 0x%08x.\n", GetExceptionCode() ));
        }

#if TARGET_SIDE
		if (LG_IS_PRIMARY_MODE(Sftk_LG) )
		{ // nothing to do....
			continue;
		}
#endif
        // Read all pkts from TRecieve Queue into Local Anchor List.
		ANCHOR_InitializeListHead( localQueueList );

		OS_ACQUIRE_LOCK( &Sftk_LG->QueueMgr.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
        ANCHOR_MOVE_LIST( localQueueList, Sftk_LG->QueueMgr.TRecieveList );
		OS_RELEASE_LOCK( &Sftk_LG->QueueMgr.Lock, NULL);

		// Walk thru Local link list
		for(	plistEntry = localQueueList.ListEntry.Flink;
				plistEntry != &localQueueList.ListEntry;)
		{ // for :scan thru each and every IRP list 
			pMM_Proto_Hdr = CONTAINING_RECORD( plistEntry, MM_PROTO_HDR, MmProtoHdrLink);
			plistEntry = plistEntry->Flink;	// get next lstentry 

			// now call Protocol processing routines, which will process this and free pMM_Proto_Hdr
			status = sftk_secondary_Recved_Pkt_Process( Sftk_LG, pMM_Proto_Hdr);
		}
	} // while (Sftk_LG->Secondary.TWriteThreadShouldStop == FALSE) 

	DebugPrint((DBG_THREAD, "sftk_Target_write_Thread:: Terminating Thread: Sftk_LG 0x%08x! \n", 
								Sftk_LG->LGroupNumber));

	PsTerminateSystemThread( STATUS_SUCCESS );
} // sftk_Target_write_Thread()

// API get call in secondary mode only.
NTSTATUS
sftk_secondary_Recved_Pkt_Process( PSFTK_LG Sftk_LG, PMM_PROTO_HDR pMM_Proto_Hdr)
{
	NTSTATUS status = STATUS_SUCCESS;

	DebugPrint((DBG_PROTO, "sftk_secondary_Recved_Pkt_Process:: TODO TODO FIXME FIXME Sftk_LG 0x%08x! \n", 
								Sftk_LG->LGroupNumber));

	OS_ASSERT(LG_IS_SECONDARY_MODE(Sftk_LG));


	// TODO : on recieve of I/O from primary, always check first secondary running on failover mode, 
	// if yes then do following
	if (LG_IS_SECONDARY_WITH_TRACKING( Sftk_LG))
	{ // we already running in failover mode.. 
		DebugPrint((DBG_ERROR, "sftk_lg_failover: Secondary lg %d, running in failover mode!!! \n",
							Sftk_LG->LGroupNumber));

		if (LG_IS_JOURNAL_ON(Sftk_LG))
		{ // if : secondary failover with Journal ON
			// TODO : Do IO to journal..
		}
		else
		{ // failover without journal
			// TODO : Return error failover with no journal to primary, ignore data 
			DebugPrint((DBG_ERROR, "sftk_lg_failover: Secondary lg %d, running in failover mode with No Journal, returning proto error!!! \n",
							Sftk_LG->LGroupNumber));
		}
	} // we already running in failover mode.. 

	// free this pMM_Proto_Hdr and its Raw Buffer after processing it.
	//in case of Disk Async I/O, There is multiple soft hdr with data exist inside proto Hdr
	// so once it issued to Disk as multiple IRp, at completion of all IRP, in completion routine,
	// we must free Raw Buffer (MM_HOLDER type) first and then MM_PROTO_HDR free after making RawBuffer zero.

	
	return status;
} // sftk_secondary_Recved_Pkt_Process()

// LG target Write Thread: Use only in Secondary mode, 
// - Responsible to process each and every Protocol Pkt arrived from Primary and send its ACk back.
// - uses 2 Qeueue TSend and TReceieve.
// - Does operation for Disk Write, or Journal Write. It also creates Journal files as needed. 
// - Responsible for Journal Apply threadto start its work
VOID
sftk_JApply_Thread( PSFTK_LG Sftk_LG)
{
	NTSTATUS			status			= STATUS_SUCCESS;
	PLIST_ENTRY			plistEntry;
	PMM_PROTO_HDR		pMM_Proto_Hdr;
    
	DebugPrint((DBG_THREAD, "sftk_JApply_Thread:: Starting Thread: LGnum 0x%08x ! \n", 
								Sftk_LG->LGroupNumber));

    // Set thread priority to lowest realtime level.
    // KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while (Sftk_LG->Secondary.JApplyThreadShouldStop == FALSE) 
    {
        // Wait for a Pkt arrival Event 
		if (Sftk_LG->Secondary.JApplyWakeupTimeout.QuadPart == 0)
		{
			Sftk_LG->Secondary.JApplyWakeupTimeout.QuadPart	= DEFAULT_TIMEOUT_FOR_JAPPLY_THREAD;
		}

        try 
        {
            KeWaitForSingleObject(	(PVOID) &Sftk_LG->Secondary.JApplyWorkEvent, // &Sftk_Dev->MasterQueueSemaphore,
									Executive,
									KernelMode,
									FALSE,
									&Sftk_LG->Secondary.JApplyWakeupTimeout );

        } 
        except (sftk_ExceptionFilterDontStop(GetExceptionInformation(), GetExceptionCode()) ) 
        {
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

            // We encountered an exception somewhere, eat it up.
            DebugPrint((DBG_ERROR, "sftk_JApply_Thread::KeWaitForSingleObject() crashed: EXCEPTION_EXECUTE_HANDLER, Exception code = 0x%08x.\n", GetExceptionCode() ));
        }

#if TARGET_SIDE
		if (LG_IS_PRIMARY_MODE(Sftk_LG) )
		{ // nothing to do....
			continue;
		}
#endif

#if 0
		// Do Journal Apply process Management here
		KeResetEvent( &Sftk_LG->Role.JApplyWorkDoneEvent );
		Sftk_LG->Role.JApplyRunning = TRUE;
		// Do work here.....
		Sftk_LG->Role.JcurSR_FileNo = 0; // TODO : retrieve this value from Pstore file after boot Japply process....
		Sftk_LG->Role.JcurN_Offest	= 0; // TODO : retrieve this value from Pstore file after boot Japply process.... 
		
		do 
		{ // start in do-while loop for Japply process
			// first check sequence number based existing .Src and .Snc file 
			
			Sftk_LG->Role.JcurSR_FileNo ++;
			Sftk_LG->Role.JcurSR_Offest = 0;
			Sftk_LG->Role.JcurSN_Offest = 0;
			// TODO : just check next .Src and .snc exist, if yes then do apply .nc file.

			// When reaches to last file to apply .Nc file check this flag
			if (Sftk_LG->Secondary.DoFastMinimalJApply == TRUE)
			{ // do not apply this last .nc file
				DebugPrint((DBG_ERROR, "sftk_JApply_Thread::DoFastMinimalJApply == TRUE set so skipping last .Nc file to apply and finishing JApply work \n"));
				break; // goto JApplyDone;
			}

		} while(FALSE);

// JApplyDone:
		// When its done set this event
		KeSetEvent( &Sftk_LG->Role.JApplyWorkDoneEvent, 0, FALSE );
#endif
	} // while (Sftk_LG->Secondary.JApplyThreadShouldStop == FALSE) 

	DebugPrint((DBG_THREAD, "sftk_JApply_Thread:: Terminating Thread: Sftk_LG 0x%08x! \n", 
								Sftk_LG->LGroupNumber));

	PsTerminateSystemThread( STATUS_SUCCESS );
} // sftk_JApply_Thread()

#endif
