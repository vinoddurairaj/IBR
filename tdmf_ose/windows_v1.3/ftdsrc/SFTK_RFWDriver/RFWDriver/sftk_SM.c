/**************************************************************************************

Module Name: sftk_SM.C   
Author Name: Parag sanghvi 
Description: Shared Memory IPC APIs are deinfed here.
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

#if 1 // SM_IPC_SUPPORT	
//
// Kernel API PreDeclaration
//
NTSTATUS
NTAPI
ZwCreateSection(
	OUT PHANDLE           SectionHandle,
	IN ACCESS_MASK        DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN PLARGE_INTEGER     MaximumSize OPTIONAL,
	IN ULONG              SectionPageProtection,
	IN ULONG              AllocationAttributes,
	IN HANDLE             FileHandle OPTIONAL);

// Win32: SetSecurityDescriptorOwner
NTSTATUS 
NTAPI 
RtlSetOwnerSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor, 
    PSID Owner, 
    BOOLEAN OwnerDefaulted);

// Win32: SetSecurityDescriptorGroup
NTSTATUS 
NTAPI 
RtlSetGroupSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor, 
    PSID Group, 
    BOOLEAN GroupDefaulted);

//
// Functions Definations starts here
//

NTSTATUS
SM_Thread_Create( IN OUT PMM_MANAGER	Mmgr)
{
	NTSTATUS			status = STATUS_SUCCESS;
	HANDLE				threadHandle;
    OBJECT_ATTRIBUTES   objAttribes;

	if (Mmgr->SM_ThreadObjPtr)
	{ // Thread is already created and running, so just return STATUS_SUCCESS
		DebugPrint((DBG_ERROR, "SM_Thread_Create:: SM_Thread is already running nothing to do, Contex 0x%08x, returning 0x%08x !\n", 
								Mmgr, status ));
		OS_ASSERT(Mmgr->SM_Cmd != NULL);		
		return status;
	}

	try 
    {
		Mmgr->SM_ThreadObjPtr				= NULL;
        Mmgr->SM_ThreadStop					= FALSE;
		Mmgr->SM_WakeupTimeout.QuadPart		= DEFAULT_TIMEOUT_FOR_SM_THREAD_WAIT_100NS;
		Mmgr->SM_CmdExecWaitTimeout.QuadPart= DEFAULT_TIMEOUT_FOR_SM_CMD_EXECUTE_WAIT_100NS;
		Mmgr->SM_Cmd						= NULL;

		// Initialized SM Local Event 
		KeInitializeEvent( &Mmgr->SM_Event, SynchronizationEvent, FALSE);
		KeInitializeEvent( &Mmgr->SM_EventCmdExecuted, SynchronizationEvent, FALSE);
        // ANCHOR_InitializeListHead( Sftk_Dev->MasterQueueList )

#ifdef NTFOUR
        InitializeObjectAttributes(&objAttribes, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
#else
        InitializeObjectAttributes(&objAttribes, NULL, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
#endif
        status = PsCreateSystemThread(	&threadHandle,
										(ACCESS_MASK) 0L,
										&objAttribes,
										(HANDLE) 0L,
										NULL,
										SM_Thread,
										Mmgr);
        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "SM_Thread_Create:: PsCreateSystemThread( for SM_Thread, Contex 0x%08x) Failed, returning 0x%08x !\n", 
								Mmgr, status ));
            try_return(status);
        }

        status = ObReferenceObjectByHandle(threadHandle,
										   THREAD_ALL_ACCESS,
										   NULL,
										   KernelMode,
										   &Mmgr->SM_ThreadObjPtr,
										   NULL);
        if (!NT_SUCCESS(status)) 
        {
			DebugPrint((DBG_ERROR, "SM_Thread_Create:: ObReferenceObjectByHandle( for SM_Thread, Contex 0x%08x) Failed, returning 0x%08x !\n", 
								Mmgr, status ));
			try_return(status);
        }

		ZwClose(threadHandle);

		// Wait for signal object, thread notification of created Shared memory section successfully
        KeWaitForSingleObject(	(PVOID) &Mmgr->SM_Event,
								Executive,
								KernelMode,
								FALSE,
								NULL );

		KeClearEvent(&Mmgr->SM_Event);

		if (Mmgr->SM_Cmd == NULL)
		{ // Failed : Thread failed to create Shared memory map section !! 
			// Terminate Thread
			status = STATUS_UNSUCCESSFUL;
			DebugPrint((DBG_ERROR, "SM_Thread_Create:: KeWaitForSingleObject( for SM_Thread to create Shared Memory map, Contex 0x%08x) Failed, returning 0x%08x !\n", 
								Mmgr, status ));
            try_return(status);
		}
       
        try_exit:   NOTHING;
    } 
    finally 
    {
        if (!NT_SUCCESS(status)) 
        { // failed Terminate thread if its stareted
            Mmgr->SM_ThreadStop = TRUE;

            if (Mmgr->SM_ThreadObjPtr) 
            {
                KeSetEvent(&Mmgr->SM_Event, 0, FALSE);
                KeWaitForSingleObject(	(PVOID) Mmgr->SM_ThreadObjPtr,
										Executive,
										KernelMode,
										FALSE,
										NULL );
                ObDereferenceObject( Mmgr->SM_ThreadObjPtr );
				Mmgr->SM_ThreadObjPtr = NULL;
            }
        } 
    }

	return status;
} // SM_Thread_Create()

NTSTATUS
SM_Thread_Terminate( IN OUT PMM_MANAGER	Mmgr)
{
	NTSTATUS status = STATUS_SUCCESS;

	Mmgr->SM_ThreadStop = TRUE;

    if (Mmgr->SM_ThreadObjPtr) 
    {
		KeSetEvent(&Mmgr->SM_Event, 0, FALSE);
		KeWaitForSingleObject(	(PVOID) Mmgr->SM_ThreadObjPtr,
								Executive,
								KernelMode,
								FALSE,
								NULL );
		ObDereferenceObject( Mmgr->SM_ThreadObjPtr );
		Mmgr->SM_ThreadObjPtr = NULL;
     }

	return status;
} // SM_Thread_Terminate()

NTSTATUS
SM_IPC_Open(PMM_MANAGER	Mmgr)
{
	NTSTATUS				status		= STATUS_SUCCESS;
	PSECURITY_DESCRIPTOR	pSD			= NULL;
	ULONG					viewsize	= 0;
	LARGE_INTEGER			sharedMemorySize;
	UNICODE_STRING			sectionUniString, namedEvent;
	OBJECT_ATTRIBUTES		objAttribes;
	
	Mmgr->SM_Size				= SFTK_SHAREDMEMORY_SIZE;
	Mmgr->SM_Vaddr				= NULL;
	Mmgr->SM_Cmd				= NULL;
	Mmgr->SM_Mdl				= NULL;
	Mmgr->SM_HSection			= NULL;
	Mmgr->SM_HCreateSection		= NULL;
	Mmgr->SM_NamedEvent			= NULL;
	Mmgr->SM_NamedEventHandle	= NULL;

	RtlInitUnicodeString( &sectionUniString, SFTK_SHAREDMEMORY_KERNEL_PATHNAME);

	InitializeObjectAttributes (&objAttribes,
								&sectionUniString,
								OBJ_CASE_INSENSITIVE,
								(HANDLE) NULL,
								(PSECURITY_DESCRIPTOR) NULL);

	status = ZwOpenSection (	&Mmgr->SM_HSection,
								SECTION_ALL_ACCESS,
								&objAttribes);

	if (!NT_SUCCESS(status))
	{ // Failed : ZwOpenSection()
		Mmgr->SM_HSection = NULL;
		DebugPrint((DBG_ERROR, "SM_IPC_Open:: ZwOpenSection( %S) Failed, Returned 0x%08x, Now Trying to CreateSection !\n", 
								sectionUniString.Buffer, status ));

		goto try_to_createSection;	// Test try without SD !!! FIXME FIXME !! TODO TODO : PARAG
		
		// Create shared memory section object  
		// create a NULL DACL so users can access shared memory
		// note a NULL DACL is not the same as a NULL, which is used
		// to specify the default security descriptor.
		// Initialize buffer pointers
		pSD = (PSECURITY_DESCRIPTOR) OS_AllocMemory( NonPagedPool, 1024 );
		if (pSD == NULL)
		{
			DebugPrint((DBG_ERROR, "SM_IPC_Open:: RtlCreateSecurityDescriptor( %S) Failed, Error 0x%08x, Trying to CreateSection using NULL SD!\n", 
								sectionUniString.Buffer, status ));
			goto try_to_createSection;
		}
		// Create an absolute-form security descriptor for manipulation.
		// The one on the security descriptor is in self-relative form.
		status = RtlCreateSecurityDescriptor(	pSD,
												SECURITY_DESCRIPTOR_REVISION );
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "SM_IPC_Open:: RtlCreateSecurityDescriptor( %S) Failed, Error 0x%08x, Trying to CreateSection using NULL SD!\n", 
								sectionUniString.Buffer, status ));
			OS_FreeMemory( pSD );
			pSD = NULL;
			goto try_to_createSection;
		}
		// set NULL DACL to the security descriptor..
		status = RtlSetDaclSecurityDescriptor( pSD, TRUE, NULL, FALSE );

  		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "SM_IPC_Open:: RtlSetDaclSecurityDescriptor( %S) Failed, Error 0x%08x, Trying to CreateSection using NULL SD!\n", 
								sectionUniString.Buffer, status ));
			OS_FreeMemory( pSD );
			pSD = NULL;
			goto try_to_createSection;
		}
		// mark the security descriptor as having no owner
		status = RtlSetOwnerSecurityDescriptor( pSD, NULL, FALSE );
      
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "SM_IPC_Open:: RtlSetOwnerSecurityDescriptor( %S) Failed, Error 0x%08x, Trying to CreateSection using NULL SD!\n", 
								sectionUniString.Buffer, status ));
			OS_FreeMemory( pSD );
			pSD = NULL;
			goto try_to_createSection;
		}
		//mark the security descriptor as having no primary group
		status = RtlSetGroupSecurityDescriptor( pSD, NULL, FALSE );
		if (!NT_SUCCESS(status))
		{
			 DebugPrint((DBG_ERROR, "SM_IPC_Open:: RtlSetGroupSecurityDescriptor( %S) Failed, Error 0x%08x, Trying to CreateSection using NULL SD!\n", 
								sectionUniString.Buffer, status ));
			OS_FreeMemory( pSD );
			pSD = NULL;
			goto try_to_createSection;
		}
		// Finally, make sure that what we made is valid
		if( !RtlValidSecurityDescriptor( pSD ))
		{
			DebugPrint((DBG_ERROR, "SM_IPC_Open:: RtlValidSecurityDescriptor( %S) Failed, Error 0x%08x, Trying to CreateSection using NULL SD!\n", 
								sectionUniString.Buffer, status ));
			OS_FreeMemory( pSD );
			pSD = NULL;
			goto try_to_createSection;
		}

try_to_createSection:

		// use NULL DACL in object attributes used by create section object
		InitializeObjectAttributes (&objAttribes,
									  &sectionUniString,
									  OBJ_CASE_INSENSITIVE,
									  (HANDLE) NULL,
									  pSD);
		
		sharedMemorySize.LowPart = Mmgr->SM_Size;
		sharedMemorySize.HighPart = 0;

		status = ZwCreateSection(	&Mmgr->SM_HCreateSection,
									SECTION_MAP_READ | SECTION_MAP_WRITE,
									&objAttribes,
									&sharedMemorySize,
									PAGE_READWRITE,
									SEC_RESERVE,
									NULL);

		if (!NT_SUCCESS(status))
		{ // could not create shared memory
			Mmgr->SM_HCreateSection = NULL;
			DebugPrint((DBG_ERROR, "SM_IPC_Open:: ZwCreateSection( %S, Size %d) Failed, Error 0x%08x!\n", 
								sectionUniString.Buffer, sharedMemorySize.LowPart, status ));
			goto done;
		}	

		// open just created shared memory section object
		status = ZwOpenSection (	&Mmgr->SM_HSection,
									SECTION_ALL_ACCESS,
									&objAttribes);

		if (!NT_SUCCESS(status))
		{
			Mmgr->SM_HSection = NULL;
			DebugPrint((DBG_ERROR, "SM_IPC_Open:: ZwOpenSection( %S, Size %d) Failed, Error 0x%08x !\n", 
								sectionUniString.Buffer, sharedMemorySize.LowPart, status ));
			goto done;
		}
	} // Failed : ZwOpenSection()

	// If the value of this parameter is zero, a view of the section will  
	// be mapped starting at the specified section offset and continuing  
	// to the end of the section.Map the section
	status = ZwMapViewOfSection (	Mmgr->SM_HSection,
									(HANDLE) -1,
									&Mmgr->SM_Vaddr,
									0L,				// zero bits
									Mmgr->SM_Size,	// PAGE_SIZE,  // commit size
									NULL,			// section offset
									&viewsize, 
									ViewShare,		// specify 0?
									0,
									PAGE_READWRITE);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "SM_IPC_Open:: ZwMapViewOfSection( %S, Size %d) Failed, Error 0x%08x!\n", 
								sectionUniString.Buffer, sharedMemorySize.LowPart, status ));
		goto done;
	}

	Mmgr->SM_Cmd = (PSM_CMD) Mmgr->SM_Vaddr;

#if 0	// Not Needed, May be in future, 
		// In future if shared memory needs access from any context in driver, 
		// enable this code !!
	// use base address to create MDL. If the Mmgr->SM_Mdl is not not null when passed
	// in to this function it assumes that it points to a valid MDL;
	Mmgr->SM_Mdl =  MmCreateMdl(	Mmgr->SM_Mdl,
									Mmgr->SM_Vaddr,
									Mmgr->SM_Size);
	if (Mmgr->SM_Mdl == NULL)
	{
		status = STATUS_UNSUCCESSFUL;
		DebugPrint((DBG_ERROR, "SM_IPC_Open:: MmCreateMdl( %S, Size %d) Failed, Error 0x%08x!\n", 
								sectionUniString.Buffer, Mmgr->SM_Size, status ));
		goto done;
	} 

	try 
	{ // Validate and lock down page.
		MmProbeAndLockPages(	Mmgr->SM_Mdl, KernelMode, IoModifyAccess);
	}
	except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
    {
		// Log event message here
		sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

        // We encountered an exception somewhere, eat it up.
		status = GetExceptionCode(); // STATUS_UNSUCCESSFUL;
        DebugPrint((DBG_ERROR, "SM_IPC_Open:: MmProbeAndLockPages( %S, Size %d) Failed, Error 0x%08x, Exception code = 0x%08x.\n",
								sectionUniString.Buffer, Mmgr->SM_Size, status, GetExceptionCode() ));
		IoFreeMdl(Mmgr->SM_Mdl); // Free MDL
		Mmgr->SM_Mdl = NULL;
		goto done;
    }

	Mmgr->SM_Cmd = (PSM_CMD) MmGetSystemAddressForMdl(Mmgr->SM_Mdl);
	if (Mmgr->SM_Cmd == NULL)
	{
		MmUnlockPages(Mmgr->SM_Mdl);
		IoFreeMdl(Mmgr->SM_Mdl); // Free MDL
		Mmgr->SM_Mdl = NULL;
		status = STATUS_UNSUCCESSFUL;
		DebugPrint((DBG_ERROR, "SM_IPC_Open:: MmCreateMdl( %S, Size %d) Failed, Error 0x%08x!\n", 
								sectionUniString.Buffer, Mmgr->SM_Size, status ));
		goto done;
	} 
#endif // #if 0

	// zero shared memory since we are starting over....
	RtlZeroMemory(Mmgr->SM_Cmd, Mmgr->SM_Size);

	// Now Create Named Event to Syncronize access to Shared Memory section
	RtlInitUnicodeString(&namedEvent, SFTK_SHAREDMEMORY_KERNEL_EVENT);

	Mmgr->SM_NamedEvent = IoCreateSynchronizationEvent(	&namedEvent, 
														&Mmgr->SM_NamedEventHandle);
	if (! Mmgr->SM_NamedEvent)
	{ // Failed
#if 0 
		if (Mmgr->SM_Mdl)
		{
			MmUnlockPages(Mmgr->SM_Mdl);
			IoFreeMdl(Mmgr->SM_Mdl); // Free MDL
			Mmgr->SM_Mdl = NULL;
		}
#endif
		status = STATUS_UNSUCCESSFUL;
		DebugPrint((DBG_ERROR, "SM_IPC_Open:: IoCreateSynchronizationEvent( %S for SM %S, Size %d) Failed, Error 0x%08x!\n", 
								namedEvent.Buffer, sectionUniString.Buffer, Mmgr->SM_Size, status ));
		goto done;
	}

	KeResetEvent(Mmgr->SM_NamedEvent);  // Set event to non-signaled

	status = STATUS_SUCCESS;

done:
	if (pSD)
	{
		OS_FreeMemory( pSD );
		pSD = NULL;
	}

	if (!NT_SUCCESS(status))
	{ // Failed
		if (Mmgr->SM_Vaddr)
			ZwUnmapViewOfSection( Mmgr->SM_HSection, Mmgr->SM_Vaddr);
    
		if (Mmgr->SM_HSection)
			ZwClose (Mmgr->SM_HSection);

		if (Mmgr->SM_HCreateSection)
  			ZwClose(Mmgr->SM_HCreateSection);

		if (Mmgr->SM_NamedEvent)
			ZwClose(Mmgr->SM_NamedEventHandle);
		
		// Mmgr->SM_Size		= SFTK_SHAREDMEMORY_SIZE;
		Mmgr->SM_Vaddr				= NULL;
		Mmgr->SM_Cmd				= NULL;
		Mmgr->SM_Mdl				= NULL;
		Mmgr->SM_HSection			= NULL;
		Mmgr->SM_HCreateSection		= NULL;
		Mmgr->SM_NamedEvent			= NULL;
		Mmgr->SM_NamedEventHandle	= NULL;
	} // Failed

	return status;
} // SM_IPC_Open()


NTSTATUS
SM_IPC_Close(PMM_MANAGER	Mmgr)
{
	NTSTATUS		status				= STATUS_SUCCESS;

#if 0 
	if (Mmgr->SM_Mdl)
	{
		MmUnlockPages(Mmgr->SM_Mdl);
		IoFreeMdl(Mmgr->SM_Mdl); // Free MDL
		Mmgr->SM_Mdl = NULL;
	}
#endif

	if (Mmgr->SM_Vaddr)
		ZwUnmapViewOfSection( Mmgr->SM_HSection, Mmgr->SM_Vaddr);
    
	if (Mmgr->SM_HSection)
		ZwClose (Mmgr->SM_HSection);

	if (Mmgr->SM_HCreateSection)
  		ZwClose(Mmgr->SM_HCreateSection);

	if (Mmgr->SM_NamedEvent)
		ZwClose(Mmgr->SM_NamedEventHandle);
		
	// Mmgr->SM_Size		= SFTK_SHAREDMEMORY_SIZE;
	Mmgr->SM_Vaddr				= NULL;
	Mmgr->SM_Cmd				= NULL;
	Mmgr->SM_Mdl				= NULL;

	Mmgr->SM_HSection			= NULL;
	Mmgr->SM_HCreateSection		= NULL;

	Mmgr->SM_NamedEvent			= NULL;
	Mmgr->SM_NamedEventHandle	= NULL;

	return status;
} // SM_IPC_Close()

VOID
SM_Thread( PMM_MANAGER	Mmgr)
{
	NTSTATUS			status			= STATUS_SUCCESS;
	ULONG				numOfEvents		= 2;
	BOOLEAN				bCmdResultWait	= FALSE;
	BOOLEAN				bJmpForWaiting, bTimeOut, bNamedEventSignalled, bWorkEventSignalled;
	BOOLEAN				bContinueExecCommand, bProcessLocalWorkEventSignalled;
	PVOID				arrayOfEvents[2];	
	PSM_CMD				pSM_Cmd;
	PSET_MM_RAW_MEMORY	pmm_raw_memory;
	ULONG				maxNodesToProcess;
		
	// OS_PERF;
	
	DebugPrint((DBG_MM, "SM_Thread:: Starting Thread! \n"));

	// Create Shared Memory map Section, and Named Event 
	status = SM_IPC_Open(Mmgr);
	if (!NT_SUCCESS(status)) 
    { // failed 
		DebugPrint((DBG_ERROR, "SM_Thread:: SM_IPC_Open(Contex 0x%08x) Failed, status 0x%08x, Terminating Thread!\n", 
								Mmgr, status ));
		Mmgr->SM_ThreadStop = TRUE;
	}
	pSM_Cmd	= Mmgr->SM_Cmd;
	KeSetEvent(&Mmgr->SM_Event, 0, FALSE);	// Signalled event to pass SM_Open status 
	
	arrayOfEvents[0] = &Mmgr->SM_Event;		// Index 0 : Local Event to get work
	arrayOfEvents[1] = Mmgr->SM_NamedEvent;	// Index 1 : Named Synchronize Event for Shared Memory map section

	bContinueExecCommand	= FALSE;
	bCmdResultWait			= FALSE;
	bProcessLocalWorkEventSignalled	= FALSE;
		
    // Set thread priority to lowest realtime level /2 means medium Low realtime level = 8.
    // KeSetPriorityThread(KeGetCurrentThread(), (LOW_REALTIME_PRIORITY));	// LOW_REALTIME_PRIORITY = 16, so we setting to 8

    while (Mmgr->SM_ThreadStop == FALSE) 
    {
        try 
		{
			status = KeWaitForMultipleObjects(	numOfEvents,
												arrayOfEvents,
												WaitAny,
												Executive,
												KernelMode,
												FALSE,
												&Mmgr->SM_WakeupTimeout,
												NULL );

		} 
		except (sftk_ExceptionFilterDontStop(GetExceptionInformation(), GetExceptionCode()) ) 
		{
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

			// We encountered an exception somewhere, eat it up.
			DebugPrint((DBG_ERROR,"SM_Thread:: KeWaitForMultipleObjects() crashed: EXCEPTION_EXECUTE_HANDLER, Exception code = 0x%08x.\n", GetExceptionCode() ));
		}

		if (Mmgr->MM_Initialized == FALSE)
			continue;	// nothing to do !!
	
		// if (Mmgr->SM_ThreadStop == TRUE) 
		//	break;	// terminate thread
		bJmpForWaiting		= FALSE;
		bTimeOut			= FALSE; 
		bNamedEventSignalled= FALSE;
		bWorkEventSignalled	= FALSE;

		switch(status)
		{ 
			case STATUS_ALERTED: // The wait is completed because of an alert to the thread. 
								DebugPrint((DBG_MM, "SM_Thread: KeWaitForMultipleObjects(NumOfEvents %d): returned STATUS_ALERTED (0x%08x) !!\n", 
												numOfEvents, status ));
								bJmpForWaiting = TRUE;;
								OS_ASSERT(FALSE);	// Never get this error, since No Alert and Kernemode
								break;
			case STATUS_USER_APC: // A user APC was delivered to the current thread before the specified Timeout 
								  // interval expired. 
								DebugPrint((DBG_MM, "SM_Thread: KeWaitForMultipleObjects(NumOfEvents %d): returned STATUS_USER_APC (0x%08x) !!\n", 
												numOfEvents, status ));
								bJmpForWaiting = TRUE;
								OS_ASSERT(FALSE);	// Never get this error, since No Alert and Kernemode
								break;
			case STATUS_TIMEOUT:// A time out occurred before the specified set of wait conditions was met. 
								DebugPrint((DBG_MM, "SM_Thread: KeWaitForMultipleObjects(NumOfEvents %d): returned STATUS_TIMEOUT (0x%08x) !!\n", 
													numOfEvents, status ));
								bJmpForWaiting		= FALSE;
								break;

			// case STATUS_SUCCESS:	// Depending on the specified WaitType, one or all of the dispatcher 
									// objects in the Object array satisfied the wait. 
									// STATUS_SUCCESS and STATUS_WAIT_0 both is same value
			case STATUS_WAIT_0:		// Index 0 got signaled for Mmgr->SM_Event
								DebugPrint((DBG_MM, "SM_Thread: Signalled 'Local Event to get work' (NumOfEvents %d): returned (0x%08x) !!\n", 
													numOfEvents, status ));
								bWorkEventSignalled	= TRUE;
								break;
			case STATUS_WAIT_1:		// Index 1 got signaled for Mmgr->SM_NamedEvent
								DebugPrint((DBG_MM, "SM_Thread: Signalled 'Named Synchronize Event' (NumOfEvents %d): returned (0x%08x) !!\n", 
													numOfEvents, status ));
								bNamedEventSignalled= TRUE;
								break;
			default:
								DebugPrint((DBG_MM, "SM_Thread:: FIXME FIXME Default: KeWaitForMultipleObjects(NumOfEvents %d): returned Default (0x%08x) !!\n", 
												numOfEvents, status ));
								bJmpForWaiting = TRUE;
								OS_ASSERT(FALSE);	// Never get this error, since No Alert and Kernemode
								break;

		} // switch(status)

		if (Mmgr->SM_ThreadStop == TRUE) 
			break;

		if (bProcessLocalWorkEventSignalled == TRUE)
		{
			bWorkEventSignalled	= TRUE;
			bJmpForWaiting		= FALSE;
		}

		if (bJmpForWaiting == TRUE)
			continue;

		if (bNamedEventSignalled == TRUE)
		{ // Check Command Return status from Shared Memory map 
			if (bCmdResultWait == FALSE)
			{
				DebugPrint((DBG_MM, "SM_Thread:: FIXME FIXME CmdResult is not expected and Named Event got signalled !! \n"));
				OS_ZeroMemory(pSM_Cmd, Mmgr->SM_Size);
				continue;	// nothing to do
			}

			if (pSM_Cmd->Executed	!= CMD_EXECUTE_COMPLETE)	
			{ // command is not executed
				DebugPrint((DBG_MM, "SM_Thread:: FIXME FIXME We picked up our own set signal !!! Command didn't executed, waiting for command to execute !!! \n"));
				// signalled it again since its sychronization event, it got unsignalled
				KeSetEvent( Mmgr->SM_NamedEvent, 0, FALSE);
				KeDelayExecutionThread(KernelMode, FALSE, &Mmgr->SM_CmdExecWaitTimeout);	
				continue;	// nothing to do
			}

			// Since Executed command completed, change this local variable to FALSE
			bCmdResultWait = FALSE;

			// Now check here what command needs to execute
			switch(pSM_Cmd->Command)
			{
				case CMD_MM_ALLOC: // Always we allocate in Mmgr->IncrementAllocationChunkSizeInMB

						DebugPrint((DBG_MM, "SM_Thread:: CMD_MM_ALLOC: Result Receieved !!! \n"));

						pmm_raw_memory	= (PSET_MM_RAW_MEMORY) ( (ULONG) pSM_Cmd + sizeof(SM_CMD));

						if ( (pSM_Cmd->Status != SM_STATUS_SUCCESS) || (pmm_raw_memory->NumberOfArray <= 0) )
						{
							DebugPrint((DBG_ERROR, "SM_Thread: CMD_MM_ALLOC: Service returned NumberOfArray %d or Failed this command with error %d (0x%08x) !! \n",
																		pmm_raw_memory->NumberOfArray, pSM_Cmd->Status, pSM_Cmd->Status));
							if (Mmgr->SM_EventCmd != SM_Event_None)
								Mmgr->SM_EventCmd = SM_Event_None;	// change command 

							OS_ZeroMemory(pSM_Cmd, Mmgr->SM_Size);
							goto do_wait;
						}

						// Add This memory into Kernel
						status = mm_type_pages_add( Mmgr, pmm_raw_memory);
						if (!NT_SUCCESS(status))
						{ // Adding Memory failed, free this pages back to service !!! 
							DebugPrint((DBG_ERROR, "SM_Thread: mm_type_pages_add(NumberOfNodes %d, ChunkSize %d) Failed with status 0x%08x !!\n",
																		pmm_raw_memory->NumberOfArray, pmm_raw_memory->ChunkSize, status));

							DebugPrint((DBG_ERROR, "SM_Thread: Error Handling, Freeing Pages back to system since mm_type_pages_add() Failed !!\n"));
							// OS_ASSERT(FALSE);

							// Adding Memory failed, free this pages back to service !!! 
							pSM_Cmd->Command		= CMD_MM_FREE;
							pSM_Cmd->Executed		= CMD_NOT_EXECUTED;	// command Not executed
							pSM_Cmd->Status			= SM_STATUS_SUCCESS;

							pSM_Cmd->InBufferSize	= sizeof(SM_CMD) + sizeof(SET_MM_RAW_MEMORY) + 
													 (sizeof(ULONG) * (pmm_raw_memory->NumberOfArray-1));
							pSM_Cmd->InBuffer		= pSM_Cmd;
							pSM_Cmd->OutBufferSize	= pSM_Cmd->InBufferSize;
							pSM_Cmd->OutBuffer		= pSM_Cmd;
							pSM_Cmd->RetBytes		= 0;
							goto exec_cmd;
						} // Adding Memory failed, free this pages back to service !!! 

						// successfully executed command, 
						if (Mmgr->SM_EventCmd == SM_Event_Alloc)
							Mmgr->SM_EventCmd = SM_Event_None;	// change command 

						OS_ZeroMemory(pSM_Cmd, Mmgr->SM_Size);
						goto do_wait;

						break;

			case CMD_MM_FREE: // Always we allocate in Mmgr->IncrementAllocationChunkSizeInMB

						DebugPrint((DBG_MM, "SM_Thread:: CMD_MM_FREE: Result Receieved !!! \n"));

						pmm_raw_memory	= (PSET_MM_RAW_MEMORY) ( (ULONG) pSM_Cmd + sizeof(SM_CMD));

						if ( pSM_Cmd->Status != SM_STATUS_SUCCESS )
						{
							DebugPrint((DBG_ERROR, "BUG FIXME FIXME : SM_Thread: CMD_MM_FREE: Service returned Failed this command with error %d (0x%08x) !! \n",
																		pSM_Cmd->Status, pSM_Cmd->Status));
						}

						// successfully executed command, 
						if (Mmgr->SM_EventCmd == SM_Event_Free)
						{
							Mmgr->SM_EventCmd = SM_Event_None;	// change command 
							OS_ZeroMemory(pSM_Cmd, Mmgr->SM_Size);
							goto do_wait;
						}

						if (Mmgr->SM_EventCmd == SM_Event_FreeAll)
							bContinueExecCommand = TRUE;

						break;
			default:
						DebugPrint((DBG_MM, "FIXME FIXME SM_Thread:: pSM_Cmd->Command %d INVALID Command,  Result Receieved, ignoring this!!! \n",pSM_Cmd->Command));
						OS_ZeroMemory(pSM_Cmd, Mmgr->SM_Size);
						break;
			} //  switch(pSM_Cmd->Command)
		} // Check Command Return status from Shared Memory map 

		if (bWorkEventSignalled == TRUE) 
		{
			if (bCmdResultWait == TRUE)
			{
				DebugPrint((DBG_MM, "SM_Thread:: Local Event Got Signalled, while we are waiting previouse command to complete execution !!\n"));
				bProcessLocalWorkEventSignalled = TRUE;
				goto do_wait;
			}
		}

		bProcessLocalWorkEventSignalled = FALSE;

		if ((bWorkEventSignalled == TRUE) || (bContinueExecCommand == TRUE))
		{ // Ask Cmd To execute: if ((bWorkEventSignalled == TRUE) || (bContinueExecCommand == TRUE))

			// Now check here what command needs to execute
			switch(Mmgr->SM_EventCmd)
			{
				case SM_Event_Alloc: // Always we allocate in Mmgr->IncrementAllocationChunkSizeInMB

						DebugPrint((DBG_MM, "SM_Thread:: SM_Event_Alloc: Executing !!! \n"));

						OS_ZeroMemory(pSM_Cmd, Mmgr->SM_Size);
						pSM_Cmd->Command		= CMD_MM_ALLOC;
						pSM_Cmd->Executed		= CMD_NOT_EXECUTED;	// command Not executed
						pSM_Cmd->Status			= SM_STATUS_SUCCESS;

						pmm_raw_memory	= (PSET_MM_RAW_MEMORY) ( (ULONG) pSM_Cmd + sizeof(SM_CMD));
						
						if (Mmgr->AWEUsed == TRUE)
						{ // if AWE is used
							maxNodesToProcess = (ULONG)
								((ULONGLONG) (Mmgr->IncrementAllocationChunkSizeInMB * ONEMILLION) / (ULONGLONG) Mmgr->PageSize);

							pmm_raw_memory->ChunkSize		= Mmgr->PageSize;
							pmm_raw_memory->NumberOfArray	= maxNodesToProcess;
							pmm_raw_memory->TotalMemorySize = (maxNodesToProcess * Mmgr->PageSize);
						}
						else
						{ // else: AWE is Not used
							maxNodesToProcess = (ULONG)
								((ULONGLONG) (Mmgr->IncrementAllocationChunkSizeInMB * ONEMILLION) / (ULONGLONG) Mmgr->VChunkSize);

							pmm_raw_memory->ChunkSize		= Mmgr->VChunkSize;
							pmm_raw_memory->NumberOfArray	= maxNodesToProcess;
							pmm_raw_memory->TotalMemorySize = (maxNodesToProcess * Mmgr->VChunkSize);
						}

						
						
						pSM_Cmd->InBufferSize	= sizeof(SM_CMD) + sizeof(SET_MM_RAW_MEMORY) + 
													 (sizeof(ULONG) * (pmm_raw_memory->NumberOfArray-1));
						pSM_Cmd->InBuffer		= pSM_Cmd;
						pSM_Cmd->OutBufferSize	= pSM_Cmd->InBufferSize;
						pSM_Cmd->OutBuffer		= pSM_Cmd;
						pSM_Cmd->RetBytes		= 0;
						
						break;

				case SM_Event_Free:	// Always we Free in Mmgr->IncrementAllocationChunkSizeInMB 
				case SM_Event_FreeAll:	// We free all pages memory

						if (Mmgr->SM_EventCmd == SM_Event_Free)
						{
							DebugPrint((DBG_MM, "SM_Thread:: SM_Event_Free: Executing !!! \n"));
						}
						else
						{
							DebugPrint((DBG_MM, "SM_Thread:: SM_Event_FreeAll: Executing !!! \n"));
						}

						OS_ZeroMemory(pSM_Cmd, Mmgr->SM_Size);
						pSM_Cmd->Command		= CMD_MM_FREE;
						pSM_Cmd->Executed		= CMD_NOT_EXECUTED;	// command Not executed
						pSM_Cmd->Status			= SM_STATUS_SUCCESS;

						pmm_raw_memory	= (PSET_MM_RAW_MEMORY) ( (ULONG) pSM_Cmd + sizeof(SM_CMD));
						
						if (Mmgr->AWEUsed == TRUE)
						{ // if AWE is used
							maxNodesToProcess = (ULONG)
								((ULONGLONG) (Mmgr->IncrementAllocationChunkSizeInMB * ONEMILLION) / (ULONGLONG) Mmgr->PageSize);

							pmm_raw_memory->ChunkSize		= Mmgr->PageSize;
							pmm_raw_memory->NumberOfArray	= maxNodesToProcess;
							pmm_raw_memory->TotalMemorySize = (maxNodesToProcess * Mmgr->PageSize);
						}
						else
						{ // else: AWE is Not used
							maxNodesToProcess = (ULONG)
								((ULONGLONG) (Mmgr->IncrementAllocationChunkSizeInMB * ONEMILLION) / (ULONGLONG) Mmgr->VChunkSize);

							pmm_raw_memory->ChunkSize		= Mmgr->VChunkSize;
							pmm_raw_memory->NumberOfArray	= maxNodesToProcess;
							pmm_raw_memory->TotalMemorySize = (maxNodesToProcess * Mmgr->VChunkSize);
						}
						
						// Now fill array by free RAW pages/Vaddr from kernel MM.
						status = mm_type_pages_remove( Mmgr, pmm_raw_memory);
						if (!NT_SUCCESS(status))
						{ // Failed
							DebugPrint((DBG_ERROR, "FIXE FIXME :: SM_Thread: mm_type_pages_remove(NumberOfNodes %d, ChunkSize %d) Failed with status 0x%08x !! BUG : FIXME FIXME \n",
																		maxNodesToProcess,pmm_raw_memory->ChunkSize, status));
							// status = STATUS_SUCCESS;
							OS_ASSERT(FALSE);
							// Do not execute command,  
							Mmgr->SM_EventCmd = SM_Event_None;	// change command 
							goto do_wait;
						}

						if (pmm_raw_memory->NumberOfArray <= 0)
						{
							DebugPrint((DBG_ERROR, "SM_Thread: mm_type_pages_remove(Asking NumberOfNodes %d, ChunkSize %d) but nothing to free return numberOfNodes to free %d, Ignoring free and continuing....\n",
														maxNodesToProcess, pmm_raw_memory->ChunkSize, pmm_raw_memory->NumberOfArray));
							// Do not execute command,  

							if (Mmgr->SM_EventCmd == SM_Event_FreeAll)
							{
								Mmgr->SM_EventCmd = SM_Event_None;	// change command 
								KeSetEvent( &Mmgr->SM_EventCmdExecuted, 0, FALSE);
							}
							else
								Mmgr->SM_EventCmd = SM_Event_None;	// change command 

							goto do_wait;
						}

						pSM_Cmd->InBufferSize	= sizeof(SM_CMD) + sizeof(SET_MM_RAW_MEMORY) + 
													 (sizeof(ULONG) * (pmm_raw_memory->NumberOfArray-1));
						pSM_Cmd->InBuffer		= pSM_Cmd;
						pSM_Cmd->OutBufferSize	= pSM_Cmd->InBufferSize;
						pSM_Cmd->OutBuffer		= pSM_Cmd;
						pSM_Cmd->RetBytes		= 0;
						
						break;

				case SM_Event_None:
						DebugPrint((DBG_MM, "FIXME FIXME : SM_Thread: Mmgr->SM_EventCmd = SM_Event_None, %d :: Nothing to do FIXME FIXME !!! \n", Mmgr->SM_EventCmd));
						OS_ZeroMemory(pSM_Cmd, Mmgr->SM_Size);
						goto do_wait;
						break;
				default:	
						DebugPrint((DBG_MM, "FIXME FIXME : SM_Thread: Mmgr->SM_EventCmd = default %d :: INVALID COMMAND, Nothing to do FIXME FIXME !!! \n", Mmgr->SM_EventCmd));
						OS_ZeroMemory(pSM_Cmd, Mmgr->SM_Size);
						Mmgr->SM_EventCmd = SM_Event_None;	// change command 
						goto do_wait;
						break;
			} // switch(Mmgr->SM_EventCmd)
exec_cmd:		
			// Pass Command To Service thru Shared Memory map section
			pSM_Cmd->Executed	= CMD_NOT_EXECUTED;	// command Asked to Execute

			// Signals Event
			bCmdResultWait = TRUE;	// Command ask to execute so result is expecting
			KeSetEvent( Mmgr->SM_NamedEvent, 0, FALSE);

			// go for sleep so event signalled picked up by service not use !!
			KeDelayExecutionThread(KernelMode, FALSE, &Mmgr->SM_CmdExecWaitTimeout);	
		} // Ask Cmd To execute: if ((bWorkEventSignalled == TRUE) || (bContinueExecCommand == TRUE))

do_wait:
		if (Mmgr->SM_ThreadStop == TRUE) 
			break;
	} // while (Mmgr->SM_ThreadStop == FALSE) 

	status = SM_IPC_Close(Mmgr);
	if (!NT_SUCCESS(status)) 
    { // failed 
		DebugPrint((DBG_ERROR, "SM_Thread:: SM_IPC_Close(Contex 0x%08x) Failed, status 0x%08x, Terminating Thread!\n", 
								Mmgr, status ));
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
	return;
} // SM_Thread()

#endif // #if 1 // SM_IPC_SUPPORT	


//
//	Function:	Sftk_Ctl_MM_Cmd()
//
//	Parameters: 
//		IN OUT	PIRP 		Gets Infromation in PIRP SystemBuffer
//
//	Description:
//
// Returns: NTSTATUS values, STATUS_SUCCESS means success else failed.
//
NTSTATUS
Sftk_Ctl_MM_Cmd( PIRP Irp )
{
	NTSTATUS					status				= STATUS_SUCCESS;
	PIO_STACK_LOCATION			pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	ULONG						sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	PSM_CMD						pSM_Cmd				= Irp->AssociatedIrp.SystemBuffer;
	
	if (sizeOfBuffer < SFTK_SHAREDMEMORY_SIZE)
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Cmd: sizeOfBuffer %d < SFTK_SHAREDMEMORY_SIZE %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, SFTK_SHAREDMEMORY_SIZE, status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	if(GSftk_Config.Mmgr.MM_Initialized == FALSE)
	{
		pSM_Cmd->Command		= CMD_TERMINATE;	// Terminate the user mode IOCTL thread
		pSM_Cmd->Executed		= CMD_NOT_EXECUTED;	// command Not executed
		DebugPrint((DBG_ERROR, "Sftk_Ctl_MM_Cmd: pMmgr->MM_Initialized %d == FALSE, returning Command to terminate with status 0x%08x !!! \n",
										GSftk_Config.Mmgr.MM_Initialized, status));
		goto done; 
	}

	// Check Ioctl Command which is executed and get new command to execute
	status = MM_GetCmd( &GSftk_Config.Mmgr, pSM_Cmd );
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "MM_GetCmd() failed with error 0x%08x !!! \n", status));
		goto done; 
	}
	
done:
	return status;
} // Sftk_Ctl_MM_Cmd()

NTSTATUS
MM_GetCmd( PMM_MANAGER	Mmgr, IN PSM_CMD	SM_Cmd )
{ // Wait for command
	NTSTATUS			status			= STATUS_SUCCESS;
	BOOLEAN				bCmdRecieved	= FALSE;
	LARGE_INTEGER		waitTimeout;			// Timeout used to wait in MM_GetCmd() 
	PMM_CMD_PACKET		pMmCmdPkt;
	PSET_MM_RAW_MEMORY	pmm_raw_memory;
	SM_EVENT_COMMAND	cmd;
	PLIST_ENTRY			pListEntry;
	ULONG				maxNodesToProcess;

	pmm_raw_memory	= (PSET_MM_RAW_MEMORY) ( (ULONG) SM_Cmd + sizeof(SM_CMD));

	OS_ASSERT(SM_Cmd->Executed	== CMD_EXECUTE_COMPLETE);	// command Not executed)

	// Now check here what command got executed
	switch(SM_Cmd->Command)
	{
		case CMD_MM_ALLOC: // Always we allocate in Mmgr->IncrementAllocationChunkSizeInMB

				DebugPrint((DBG_MM, "MM_GetCmd:: CMD_MM_ALLOC: Result Receieved !!! \n"));

				GSftk_Config.Mmgr.SM_AllocExecuting = FALSE;

				if ( (SM_Cmd->Status != SM_STATUS_SUCCESS) || (pmm_raw_memory->NumberOfArray <= 0) )
				{
					DebugPrint((DBG_ERROR, "MM_GetCmd: CMD_MM_ALLOC: Service returned NumberOfArray %d or Failed this command with error %d (0x%08x) !! \n",
																pmm_raw_memory->NumberOfArray, SM_Cmd->Status, SM_Cmd->Status));
					break;	// nothing to do, since alloc failed !!!
				}

				// Add This memory into the Kernel
				status = mm_type_pages_add( Mmgr, pmm_raw_memory);
				if (!NT_SUCCESS(status))
				{ // Adding Memory failed, free this pages back to service !!! 
					DebugPrint((DBG_ERROR, "MM_GetCmd: mm_type_pages_add(NumberOfNodes %d, ChunkSize %d) Failed with status 0x%08x !!\n",
																pmm_raw_memory->NumberOfArray, pmm_raw_memory->ChunkSize, status));

					DebugPrint((DBG_ERROR, "MM_GetCmd: Error Handling, Freeing Pages back to system since mm_type_pages_add() Failed !!\n"));
					// OS_ASSERT(FALSE);

					// Adding Memory failed, free this pages back to service !!! 
					SM_Cmd->Command			= CMD_MM_FREE;
					SM_Cmd->Executed		= CMD_NOT_EXECUTED;	// command Not executed
					SM_Cmd->Status			= SM_STATUS_SUCCESS;

					SM_Cmd->InBufferSize	= sizeof(SM_CMD) + sizeof(SET_MM_RAW_MEMORY) + 
											 (sizeof(ULONG) * (pmm_raw_memory->NumberOfArray-1));
					SM_Cmd->InBuffer		= SM_Cmd;
					SM_Cmd->OutBufferSize	= SM_Cmd->InBufferSize;
					SM_Cmd->OutBuffer		= SM_Cmd;
					SM_Cmd->RetBytes		= 0;

					return STATUS_SUCCESS;
				} // Adding Memory failed, free this pages back to service !!! 
				break;

	case CMD_MM_FREE: // Always we allocate in Mmgr->IncrementAllocationChunkSizeInMB

				DebugPrint((DBG_MM, "MM_GetCmd:: CMD_MM_FREE: Result Receieved !!! \n"));

				GSftk_Config.Mmgr.SM_FreeExecuting = FALSE;

				if ( SM_Cmd->Status != SM_STATUS_SUCCESS )
				{
					DebugPrint((DBG_ERROR, "BUG FIXME FIXME : MM_GetCmd: CMD_MM_FREE: Service returned Failed this command with error %d (0x%08x) !! \n",
																SM_Cmd->Status, SM_Cmd->Status));
				}
				break;

	case CMD_NONE:
				DebugPrint((DBG_MM, "MM_GetCmd:: CMD_NONE,  Nothing got executed!!! \n"));
				break;
	default:
				DebugPrint((DBG_MM, "FIXME FIXME MM_GetCmd:: SM_Cmd->Command %d INVALID Command,  Result Receieved, ignoring this!!! \n",SM_Cmd->Command));
				break;
	} //  switch(SM_Cmd->Command)

	// Now Get next command to execute
	OS_ZeroMemory(SM_Cmd, SM_Cmd->InBufferSize);

	SM_Cmd->Executed		= CMD_NOT_EXECUTED;	// command Not executed

	waitTimeout.QuadPart= DEFAULT_TIMEOUT_FOR_SM_THREAD_WAIT_100NS;

	while( (Mmgr->MM_Initialized == TRUE) && (bCmdRecieved == FALSE) )
	{ // While : MM_iniitialized == TRUE and Cmd is not recieved to execute 
		if (Mmgr->FreeAllCmd == FALSE)
		{
			status = KeWaitForSingleObject(	&Mmgr->CmdEvent,
											Executive,
											KernelMode,
											FALSE,
											&waitTimeout );
		
			if (status != STATUS_SUCCESS)
				continue;

			if (IsListEmpty(&Mmgr->CmdList.ListEntry))
				continue;

			// Get command from queue
			OS_ACQUIRE_LOCK( &Mmgr->CmdQueueLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
			pListEntry = RemoveHeadList(&Mmgr->CmdList.ListEntry);
			Mmgr->CmdList.NumOfNodes --;
			OS_RELEASE_LOCK( &Mmgr->CmdQueueLock, NULL);
			
			pMmCmdPkt = CONTAINING_RECORD( pListEntry, MM_CMD_PACKET, cmdLink);
			cmd = pMmCmdPkt->Cmd;

			OS_FreeMemory(pMmCmdPkt);
		}
		else
			cmd = SM_Event_FreeAll;

		// Execute Command
		bCmdRecieved = TRUE;
		SM_Cmd->Executed		= CMD_NOT_EXECUTED;	// command Not executed
		SM_Cmd->Status			= SM_STATUS_SUCCESS;

		switch(cmd)
		{
			case SM_Event_Alloc: // Always we allocate in Mmgr->IncrementAllocationChunkSizeInMB

					DebugPrint((DBG_MM, "MM_GetCmd:: SM_Event_Alloc: Executing !!! \n"));

					/*
					if ( (Mmgr->TotalMemAllocated / ONEMILLION) > Mmgr->MaxAllocatePhysMemInMB)
					{ // no need to ask more memory causes we already reach our limit
						DebugPrint((DBG_ERROR, "MM_GetCmd: SM_Event_Alloc, Alloc reach limit %d MB:: Nothing to do, going for sleep again!!! \n", 
										Mmgr->MaxAllocatePhysMemInMB));
						OS_ZeroMemory(SM_Cmd, Mmgr->SM_Size);
						bCmdRecieved = FALSE;
						break;
					}
					*/
						
					SM_Cmd->Command			= CMD_MM_ALLOC;

					OS_ASSERT( GSftk_Config.Mmgr.SM_AllocExecuting == TRUE);
					
					if (Mmgr->AWEUsed == TRUE)
					{ // if AWE is used
						maxNodesToProcess = (ULONG)
							((ULONGLONG) (Mmgr->IncrementAllocationChunkSizeInMB * ONEMILLION) / (ULONGLONG) Mmgr->PageSize);

						pmm_raw_memory->ChunkSize		= Mmgr->PageSize;
						pmm_raw_memory->NumberOfArray	= maxNodesToProcess;
						pmm_raw_memory->TotalMemorySize = (maxNodesToProcess * Mmgr->PageSize);
					}
					else
					{ // else: AWE is Not used
						maxNodesToProcess = (ULONG)
							((ULONGLONG) (Mmgr->IncrementAllocationChunkSizeInMB * ONEMILLION) / (ULONGLONG) Mmgr->VChunkSize);

						pmm_raw_memory->ChunkSize		= Mmgr->VChunkSize;
						pmm_raw_memory->NumberOfArray	= maxNodesToProcess;
						pmm_raw_memory->TotalMemorySize = (maxNodesToProcess * Mmgr->VChunkSize);
					}
					
					SM_Cmd->InBufferSize	= sizeof(SM_CMD) + sizeof(SET_MM_RAW_MEMORY) + 
												 (sizeof(ULONG) * (pmm_raw_memory->NumberOfArray-1));
					SM_Cmd->InBuffer		= SM_Cmd;
					SM_Cmd->OutBufferSize	= SM_Cmd->InBufferSize;
					SM_Cmd->OutBuffer		= SM_Cmd;
					SM_Cmd->RetBytes		= 0;
					break;
		
			case SM_Event_Free:	// Always we Free in Mmgr->IncrementAllocationChunkSizeInMB 
			case SM_Event_FreeAll:	// We free all pages memory

					if (cmd == SM_Event_Free)
					{
						OS_ASSERT( GSftk_Config.Mmgr.SM_FreeExecuting == TRUE);
						DebugPrint((DBG_MM, "MM_GetCmd:: SM_Event_Free: Executing !!! \n"));
					}
					else
					{
						DebugPrint((DBG_MM, "MM_GetCmd:: SM_Event_FreeAll: Executing !!! \n"));
					}

					OS_ZeroMemory(SM_Cmd, Mmgr->SM_Size);
					SM_Cmd->Command		= CMD_MM_FREE; 
					
					if (Mmgr->AWEUsed == TRUE)
					{ // if AWE is used
						maxNodesToProcess = (ULONG)
							((ULONGLONG) (Mmgr->IncrementAllocationChunkSizeInMB * ONEMILLION) / (ULONGLONG) Mmgr->PageSize);

						pmm_raw_memory->ChunkSize		= Mmgr->PageSize;
						pmm_raw_memory->NumberOfArray	= maxNodesToProcess;
						pmm_raw_memory->TotalMemorySize = (maxNodesToProcess * Mmgr->PageSize);
					}
					else
					{ // else: AWE is Not used
						maxNodesToProcess = (ULONG)
							((ULONGLONG) (Mmgr->IncrementAllocationChunkSizeInMB * ONEMILLION) / (ULONGLONG) Mmgr->VChunkSize);

						pmm_raw_memory->ChunkSize		= Mmgr->VChunkSize;
						pmm_raw_memory->NumberOfArray	= maxNodesToProcess;
						pmm_raw_memory->TotalMemorySize = (maxNodesToProcess * Mmgr->VChunkSize);
					}
					
					// Now fill array by free RAW pages/Vaddr from kernel MM.
					status = mm_type_pages_remove( Mmgr, pmm_raw_memory);
					if (!NT_SUCCESS(status))
					{ // Failed
						DebugPrint((DBG_ERROR, "FIXE FIXME :: MM_GetCmd: mm_type_pages_remove(NumberOfNodes %d, ChunkSize %d) Failed with status 0x%08x !! BUG : FIXME FIXME \n",
																	maxNodesToProcess,pmm_raw_memory->ChunkSize, status));
						// Do not execute command,  
						OS_ASSERT(FALSE);
					}

					if ( (pmm_raw_memory->NumberOfArray <= 0) && (cmd == SM_Event_FreeAll) )
					{
						DebugPrint((DBG_ERROR, "MM_GetCmd: mm_type_pages_remove(Asking NumberOfNodes %d, ChunkSize %d) but nothing to free return numberOfNodes to free %d, Ignoring free and continuing....\n",
													maxNodesToProcess, pmm_raw_memory->ChunkSize, pmm_raw_memory->NumberOfArray));
						// Do not execute command,  
						Mmgr->FreeAllCmd = FALSE;
						KeSetEvent( &Mmgr->EventFreeAllCompleted, 0, FALSE);
						SM_Cmd->Command = CMD_TERMINATE;
						break;
					}

					if (pmm_raw_memory->NumberOfArray == 0)
					{ // nothing to free, so just ignore it and go for sleep again....
						DebugPrint((DBG_ERROR, "MM_GetCmd: SM_Event_Free, Free_NumberOfArray %d :: Nothing to do, going for sleep again!!! \n", 
										pmm_raw_memory->NumberOfArray));
						OS_ZeroMemory(SM_Cmd, Mmgr->SM_Size);
						bCmdRecieved = FALSE;
						break;
					}

					pmm_raw_memory->TotalMemorySize = (pmm_raw_memory->NumberOfArray *  Mmgr->VChunkSize);

					SM_Cmd->InBufferSize	= sizeof(SM_CMD) + sizeof(SET_MM_RAW_MEMORY) + 
												 (sizeof(ULONG) * (pmm_raw_memory->NumberOfArray-1));
					SM_Cmd->InBuffer		= SM_Cmd;
					SM_Cmd->OutBufferSize	= SM_Cmd->InBufferSize;
					SM_Cmd->OutBuffer		= SM_Cmd;
					SM_Cmd->RetBytes		= 0;
					break;

			case SM_Event_None:
					DebugPrint((DBG_MM, "FIXME FIXME : MM_GetCmd: Mmgr->SM_EventCmd = SM_Event_None, %d :: Nothing to do FIXME FIXME !!! \n", Mmgr->SM_EventCmd));
					bCmdRecieved = FALSE;
					break;
			default:	
					DebugPrint((DBG_MM, "FIXME FIXME : MM_GetCmd: Mmgr->SM_EventCmd = default %d :: INVALID COMMAND, Nothing to do FIXME FIXME !!! \n", Mmgr->SM_EventCmd));
					bCmdRecieved = FALSE;
					break;
		} // switch(Mmgr->SM_EventCmd)

	} // While : MM_iniitialized == TRUE and Cmd is not recieved to execute 

	if(bCmdRecieved == FALSE)
	{
		if (Mmgr->MM_Initialized == FALSE)
			SM_Cmd->Command = CMD_TERMINATE;	// Terminate the user mode IOCTL thread
	}

	return STATUS_SUCCESS;
} // MM_GetCmd()
