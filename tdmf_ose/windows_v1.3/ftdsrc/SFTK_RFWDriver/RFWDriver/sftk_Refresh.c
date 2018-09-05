/**************************************************************************************

Module Name: sftk_Refresh.C   
Author Name: Parag sanghvi 
Description: thread APIS and its related APIS
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

// WaitForMultipleObjects() limit for max wait is MAXIMUM_WAIT_OBJECTS = 64.
#define		MAX_DEV_PER_LG		(MAXIMUM_WAIT_OBJECTS-2)	// since we can go above MAXIMUM_QAIT_OBJECTS limit
#define		MAX_ARRAY_EVENTS	MAX_DEV_PER_LG + 2			// 2 is for other events wait

// Device Master Thread for its own Pstore LRDB bitmap to flush 
// This thread also allocates/initalize bab memory for new write data if LG is running
// in SFTK_MODE_NORMAL or SFTK_MODE_SMART_REFRESH. (if SFTK_MODE_BAB_UPDATE is set)
    
// We always use HRDB bitmap for Full/Smart Refresh, 
// Caller Who change this state event to refresh mode, has a responsibity
// to make or prepare this Hrdb bitmap.

// When Complete Bitmap parsing I is over, means all data with bit set 1 in HRDB has been 
// sent to Secondary. This refresh thread will go for wait for RefreshEmptyAckQueueEvent which gets signalled
// when in refresh mode migration queue is empty.

// We should not need lock over here, since HRDB is usage is based on state, if Refresh state gets changed
// this thread will stop using this HRDB and will go for wait again. next time it will start from lastindex which
// gets reset all the time.
VOID
sftk_refresh_lg_thread( PSFTK_LG	Sftk_Lg)
{
	NTSTATUS			status					= STATUS_SUCCESS;
	PSFTK_DEV			pSftk_Dev				= NULL;
	PKWAIT_BLOCK		pWaitBlockArray			= NULL;
	BOOLEAN				bUsePnpEvents			= FALSE;
	BOOLEAN				bStateChanged			= FALSE;
	BOOLEAN				bFullRefresh, bDevRefreshDone;
	ULONG				numOfArrayEvents, usedEventsCount;
	PVOID				arrayOfEvents[MAXIMUM_WAIT_OBJECTS];	// MAXIMUM_WAIT_OBJECTS = 64
	ULONG				nextBit, readSize;
	LONG				currentState;
	PLIST_ENTRY			plistEntry;
	LARGE_INTEGER		byteOffset, endOffset, offset, getnextpktTimeout;
	PVOID				pDataBuffer;
	PMM_HOLDER			pMM_Buffer;
	RCONTEXT			rContext;
	ULONG				noOfEvents;		// Cache wait event count
	PVOID				eventsList[2];	// Cache wait event array
	ULONG				readLength, totalReadDone, numOfSetBits;
	BOOLEAN				bErrorInSegmentationLoop = FALSE;
	WCHAR				wchar1[64], wchar2[64], wchar3[128];	// for message
	BOOLEAN				bExitWaitloop = FALSE;
	ULONG				countWaitloop = 0;
	BOOLEAN				bNoDataSend = TRUE;
	LARGE_INTEGER		delayTime;
	    
	DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: Starting Thread: Sftk_Lg 0x%08x for LG Num %d ! \n", 
								Sftk_Lg, Sftk_Lg->LGroupNumber));

	rContext.Sftk_Lg = Sftk_Lg;
	// KeInitializeEvent( &rContext.Event, SynchronizationEvent, FALSE);

    // Set thread priority to lowest realtime level /2 means medium Low realtime level = 8.
    KeSetPriorityThread(KeGetCurrentThread(), (LOW_REALTIME_PRIORITY/2));	// LOW_REALTIME_PRIORITY = 16, so we setting to 8

	// If Count <= THREAD_WAIT_OBJECTS, then WaitBlockArray can be NULL. 
	// Otherwise this parameter must point to a memory buffer of 
	// sizeof(KWAIT_BLOCK) * Count bytes. The routine uses this buffer 
	// for record-keeping while performing the wait operation. 
	pWaitBlockArray = OS_AllocMemory(PagedPool, (sizeof(KWAIT_BLOCK) * MAXIMUM_WAIT_OBJECTS));

	OS_ZeroMemory( arrayOfEvents, sizeof(arrayOfEvents) );
	numOfArrayEvents = 0;
	// Following Events may use to wait for different state of signals
	// Index 0 -	RefreshStateChangeSemaphore for changes of LG state or termination signals.
	// 
	// Index 1 -	RefreshEmptyAckQueueEvent this is used only if RefreshFinishedParseI == TRUE 
	//				to check Migration Queue is empty and now its safe to transit the state from 
	//				Smart refresh mode to Normal mode Or Full Refresh to Smart Refresh mode 
	// Index 2 -	Per device SFTK_DEV under LG, PnpEventDiskArrived event
	// Index 3 -	Per device SFTK_DEV under LG, PnpEventDiskArrived event
	//	.
	//	.
	// Index n -	total number of Disconnected devices....	
	//				If during Refresh operations any device under LG gets disconnected, we wait
	//				on that respective device pnp event to resume refresh work, so we wait for this events.

	arrayOfEvents[numOfArrayEvents] = &Sftk_Lg->RefreshStateChangeSemaphore;	// Index 0	
	numOfArrayEvents ++;
	
	arrayOfEvents[numOfArrayEvents] = &Sftk_Lg->RefreshEmptyAckQueueEvent;		// Index 1
	numOfArrayEvents ++;

	usedEventsCount = 1;	// always use index 0 atleast for termination and change event signalled.
	numOfArrayEvents = usedEventsCount;

    while (Sftk_Lg->RefreshThreadShouldStop == FALSE) 
    {
		if (Sftk_Lg->RefreshThreadWakeupTimeout.QuadPart == 0)
		{
			Sftk_Lg->RefreshThreadWakeupTimeout.QuadPart	= DEFAULT_TIMEOUT_FOR_REFRESH_THREAD;
		}

		if ( (numOfArrayEvents > THREAD_WAIT_OBJECTS) && (pWaitBlockArray == NULL) )
		{
			pWaitBlockArray = OS_AllocMemory(PagedPool, (sizeof(KWAIT_BLOCK) * MAXIMUM_WAIT_OBJECTS));
			if (pWaitBlockArray == NULL)
			{
				DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: For LG %d : OS_AllocMemory(PagedPool, size %d) for KeWaitForMultipleObjectsfor failed...FIXME FIXME...\n", 
											Sftk_Lg->LGroupNumber, (sizeof(KWAIT_BLOCK) * numOfArrayEvents) ));
				DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just wait for 3 events...\n"));

				numOfArrayEvents = THREAD_WAIT_OBJECTS;
			}
		}
			
		// not need to make zero as per DDK help, just being safe side...
		if (pWaitBlockArray)
			OS_ZeroMemory(pWaitBlockArray, (sizeof(KWAIT_BLOCK) * MAXIMUM_WAIT_OBJECTS));

		try 
		{
			status = KeWaitForMultipleObjects(	numOfArrayEvents,
												arrayOfEvents,
												WaitAny,
												Executive,
												KernelMode,
												FALSE,
												&Sftk_Lg->RefreshThreadWakeupTimeout,
												pWaitBlockArray );

		} 
		except (sftk_ExceptionFilterDontStop(GetExceptionInformation(), GetExceptionCode()) ) 
		{
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

			// We encountered an exception somewhere, eat it up.
			DebugPrint((DBG_ERROR,"sftk_refresh_lg_thread:: KeWaitForMultipleObjects() crashed: EXCEPTION_EXECUTE_HANDLER, Exception code = 0x%08x.\n", GetExceptionCode() ));
		}

		if (Sftk_Lg->RefreshThreadShouldStop == TRUE) 
			break;	// terminate thread

#if TARGET_SIDE
		if (LG_IS_SECONDARY_MODE(Sftk_Lg))
		{ // nothing to do
			continue;
		}
#endif

		switch(status)
		{ 
			case STATUS_ALERTED: // The wait is completed because of an alert to the thread. 
								DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d,KeWaitForMultipleObjects(NumOfEvents %d): returned STATUS_ALERTED (0x%08x) !!\n", 
										Sftk_Lg->LGroupNumber, numOfArrayEvents, status ));
								OS_ASSERT(FALSE);	// Never get this error, since No Alert and Kernemode
								break;
			case STATUS_USER_APC: // A user APC was delivered to the current thread before the specified Timeout 
								  // interval expired. 
								DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d,KeWaitForMultipleObjects(NumOfEvents %d): returned STATUS_USER_APC (0x%08x) !!\n", 
										Sftk_Lg->LGroupNumber, numOfArrayEvents, status ));
								OS_ASSERT(FALSE);	// Never get this error, since No Alert and Kernemode
								break;
			case STATUS_TIMEOUT:	// A time out occurred before the specified set of wait conditions was met. 
								// This value can be returned when an explicit time-out value of zero is 
								// specified, but the specified set of wait conditions cannot be met immediately
								DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d,KeWaitForMultipleObjects(NumOfEvents %d): returned STATUS_TIMEOUT (0x%08x) !!\n", 
													Sftk_Lg->LGroupNumber, numOfArrayEvents, status ));
								break;

			// case STATUS_SUCCESS:	// Depending on the specified WaitType, one or all of the dispatcher 
									// objects in the Object array satisfied the wait. 
									// STATUS_SUCCESS and STATUS_WAIT_0 both is same value
			case STATUS_WAIT_0:		// Index 0 got signaled for Sftk_Lg->RefreshStateChangeSemaphore
								DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d,Signalled RefreshStateChangeSemaphore (NumOfEvents %d): returned (0x%08x) !!\n", 
													Sftk_Lg->LGroupNumber, numOfArrayEvents, status ));
								break;
			case STATUS_WAIT_1:		// Index 1 got signaled for Sftk_Lg->RefreshEmptyAckQueueEvent

								OS_ASSERT(Sftk_Lg->RefreshFinishedParseI == TRUE);
								Sftk_Lg->RefreshFinishedParseI = FALSE; // reset values.

								// Release all Acuired buffer, This API will wait Remove pkts from Queue till it finished 
								status = mm_release_buffer_for_refresh( Sftk_Lg );
								if (!NT_SUCCESS(status))
								{
									DebugPrint((DBG_ERROR, "BUG FIXME FIXME sftk_refresh_lg_thread:: mm_release_buffer_for_refresh() LG %d : Failed to Release pkts BUG FIXME FIXME !! ..\n", 
															Sftk_Lg->LGroupNumber));
									OS_ASSERT(FALSE);
								}

								// we are done with refresh so switch the state here.
								currentState = sftk_lg_get_state(Sftk_Lg);
								switch(currentState)
								{
								case SFTK_MODE_FULL_REFRESH:
										DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d, Signalled RefreshEmptyAckQueueEvent: Changing state from SFTK_MODE_FULL_REFRESH to SFTK_MODE_SMART_REFRESH !!\n", 
													Sftk_Lg->LGroupNumber));

										// Send OutBand for Full refresh Complete Procotol Command 
										status = QM_SendOutBandPkt( Sftk_Lg, FALSE, FALSE, FTDCRFFEND);
										if (!NT_SUCCESS(status))
										{
											DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d : Failed Otband Proto command %d FTDCRFFEND!! ..\n", 
																	Sftk_Lg->LGroupNumber, FTDCRFFEND));
											swprintf(  wchar1, L"%d", Sftk_Lg->LGroupNumber);
											swprintf(  wchar3, L"Full Refresh Complete FTDCRFFEND");
											sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_DRIVER_PROTOCOL_MSG_ERROR, status, 
																	0, wchar1, wchar3 );
											// We can not do anything, so just continue
											// TDI will change the state automatically
											// Log Event Message here
											numOfArrayEvents = usedEventsCount;
											Sftk_Lg->DoNotSendOutBandCommandForTracking = TRUE; // so not need to send Tracking outband FTDCHUP command
											sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
											status = STATUS_TIMEOUT; // go for sleep, change state event will signalled...
											break;
										}
										Sftk_Lg->SRefreshWasNotDone = FALSE;
										if (GSftk_Config.Mmgr.MM_Initialized == TRUE)
										{
											sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_SMART_REFRESH, FALSE);
										}
										else
										{
											Sftk_Lg->DoNotSendOutBandCommandForTracking = TRUE; // so not need to send Tracking outband FTDCHUP command
											sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
										}

										status = STATUS_TIMEOUT; // go for sleep, change state event will signalled...
										break;

								case SFTK_MODE_SMART_REFRESH:
										DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d, Signalled RefreshEmptyAckQueueEvent: Changing state from SFTK_MODE_SMART_REFRESH to SFTK_MODE_NORMAL !!\n", 
													Sftk_Lg->LGroupNumber));

										// Send OutBand for Smart refresh Complete Procotol Command 
										status = QM_SendOutBandPkt( Sftk_Lg, TRUE, TRUE, FTDMSGCO);
										if (!NT_SUCCESS(status))
										{
											DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d : Failed Otband Proto command %d FTDMSGCO!! ..\n", 
																	Sftk_Lg->LGroupNumber, FTDMSGCO));
											swprintf(  wchar1, L"%d", Sftk_Lg->LGroupNumber);
											swprintf(  wchar3, L"Smart Refresh Complete FTDMSGCO");
											sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_DRIVER_PROTOCOL_MSG_ERROR, status, 
																	0, wchar1, wchar3 );
											// We can not do anything, so just continue
											// TDI will change the state automatically
											// Log Event Message here
											numOfArrayEvents = usedEventsCount;
											Sftk_Lg->DoNotSendOutBandCommandForTracking = TRUE; // so not need to send Tracking outband FTDCHUP command
											sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
											status = STATUS_TIMEOUT; // go for sleep, change state event will signalled...
											break;
										}

										Sftk_Lg->SRefreshWasNotDone = FALSE;
										// sftk_update_lg_dev_lastBitIndex(pSftk_Lg, DEV_LASTBIT_CLEAN_ALL);
										if (GSftk_Config.Mmgr.MM_Initialized == TRUE)
											sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_NORMAL, FALSE);
										else
										{
											Sftk_Lg->DoNotSendOutBandCommandForTracking = TRUE; // so not need to send Tracking outband FTDCHUP command
											sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
										}

										status = STATUS_TIMEOUT; // go for sleep, change state event will signalled...
										break;

								default:
										DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d, Signalled RefreshEmptyAckQueueEvent: state is 0x%08x !!\n", 
													Sftk_Lg->LGroupNumber, currentState));
										// Nothing TO DO,
										break;
								}
								break;

			case STATUS_WAIT_2:		// Index 2 got signaled for one of Device Sftk_Dev->pSftk_Dev->PnpEventDiskArrived
			case STATUS_WAIT_3:		// Index 3 got signaled for one of Device Sftk_Dev->pSftk_Dev->PnpEventDiskArrived
									//	.......................
								DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d,KeWaitForMultipleObjects(NumOfEvents %d): Pnp Event got signaled, returned (0x%08x) !!\n", 
												Sftk_Lg->LGroupNumber, numOfArrayEvents, status ));
								break;
			
			default:
								DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d,KeWaitForMultipleObjects(NumOfEvents %d): returned Default (0x%08x) !!\n", 
										Sftk_Lg->LGroupNumber, numOfArrayEvents, status ));
								break;

		} // switch(status)

		if (status == STATUS_TIMEOUT)
			continue;

		// check if we need to do refresh for LG if yes than do work else continue..
		if ( sftk_lg_State_is_not_refresh(Sftk_Lg) )
		{
			numOfArrayEvents = usedEventsCount;
			continue;	// nothing to do. 
		}

		if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
			bFullRefresh = TRUE;	// Full resfresh
		else
			bFullRefresh = FALSE;	// smart refresh

		bNoDataSend = TRUE;

		DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d,  Starting %s NewState 0x%x!!\n", 
							Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));

		// update stats for refresh 
		Sftk_Lg->Statistics.TotalBitsDirty = 0;
		Sftk_Lg->Statistics.TotalBitsDone  = 0;
		Sftk_Lg->Statistics.NumOfBlksPerBit=0;

		// Calculate all Dirty Bit index for each and every devices
		for(	plistEntry = Sftk_Lg->LgDev_List.ListEntry.Flink;
				plistEntry != &Sftk_Lg->LgDev_List.ListEntry;
				plistEntry = plistEntry->Flink )
		{ // for :scan thru each and every Devices to update stats
			pSftk_Dev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

			if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
			{ // Full Refresh
				if ( (pSftk_Dev->RefreshLastBitIndex != DEV_LASTBIT_NO_CLEAN) &&
					 (pSftk_Dev->RefreshLastBitIndex != DEV_LASTBIT_CLEAN_ALL) )
				{
					pSftk_Dev->Statistics.TotalBitsDirty = 
						(pSftk_Dev->Hrdb.TotalNumOfBits - pSftk_Dev->RefreshLastBitIndex);
				}
				else
				{
					pSftk_Dev->Statistics.TotalBitsDirty = pSftk_Dev->Hrdb.TotalNumOfBits;
				}
				
			} // Full Refresh
			else
			{ // Smart Refresh
				pSftk_Dev->Statistics.TotalBitsDirty =
					RtlNumberOfSetBits( pSftk_Dev->Hrdb.pBitmapHdr);
			} // Smart Refresh

			pSftk_Dev->Statistics.NumOfBlksPerBit = pSftk_Dev->Hrdb.Sectors_per_bit;
			pSftk_Dev->Statistics.TotalBitsDone   = 0;
			Sftk_Lg->Statistics.TotalBitsDirty	  += pSftk_Dev->Statistics.TotalBitsDirty;
		} // for :scan thru each and every Devices to update stats

		// Call Memory Manager to reserve number of pre allocate ProtoHdr + fixed size Data Buffer (chunk size) packets
		// use Sftk_Lg->MaxTransferUnit for fixed size allocation of Refresh pool
		// Use Sftk_Lg->NumOfAsyncRefreshIO to reserve n number of data buffer for refresh reads
		status = mm_reserve_buffer_for_refresh( Sftk_Lg );
		if (!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "BUG FIXME FIXME sftk_refresh_lg_thread:: mm_reserve_buffer_for_refresh() LG %d (pSftk_Dev->DevExtension == NULL): Failed to allocate %d number of pkts BUG FIXME FIXME !! ..\n", 
									Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname));
			OS_ASSERT(FALSE);

			// Log Event Message here
			swprintf(  wchar1, L"%d", Sftk_Lg->LGroupNumber);
			sftk_get_stateString(Sftk_Lg->state, wchar2);
			swprintf(  wchar3, L"Insufficient Memory");
			sftk_LogEventString3( GSftk_Config.DriverObject, MSG_REPL_LG_REFRESH_FAILED, status, 
								  0, wchar1, wchar2, wchar3 );

			// may be changing State is only the option, so change state to tracking mode, consider it
			DebugPrint((DBG_ERROR, "CHECKT TEST !! sftk_refresh_lg_thread:: may be changing State is only the option, so change state to tracking mode, consider it CHECKT TEST !!\n", 
									Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname));
			// as BAB Overflow
			if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
				sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
			else
				sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);

			numOfArrayEvents = usedEventsCount;
			continue;	// FIXME FIXME Error Handling here
		}

		// Send Protocol command to secondary
		if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
		{ // if : Full Refresh
			// Send OutBand for Full refresh start 
			status = QM_SendOutBandPkt( Sftk_Lg, TRUE, TRUE, FTDCHUP);
			if (!NT_SUCCESS(status))
			{
				DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d : Failed Outband Start Full Refresh Proto command %d FTDCHUP!! ..\n", 
										Sftk_Lg->LGroupNumber, FTDCHUP));

				swprintf(  wchar1, L"%d", Sftk_Lg->LGroupNumber);
				swprintf(  wchar3, L"FRefresh FTDCHUP");
				sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_DRIVER_PROTOCOL_MSG_ERROR, status, 
								  0, wchar1, wchar3 );
				// We can not do anything, so just go for sleep than later we come back here again 
				// Log Event Message here
				mm_release_buffer_for_refresh(Sftk_Lg);
				numOfArrayEvents = usedEventsCount;
				// TODO : Veera, Must complete this.....Add Event which get used to signal whenever connection arrives
				// So i can wait here...in case error ocure.
				if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
					sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
				else
				{
					Sftk_Lg->DoNotSendOutBandCommandForTracking = TRUE; // so not need to send Tracking outband FTDCHUP command
					sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
				}
				numOfArrayEvents = usedEventsCount;
				continue;
			}

			status = QM_SendOutBandPkt( Sftk_Lg, FALSE, FALSE, FTDCRFFSTART);
			if (!NT_SUCCESS(status))
			{
				DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d : Failed Outband Start Full Refresh Proto command %d FTDCRFFSTART!! ..\n", 
										Sftk_Lg->LGroupNumber, FTDCRFFSTART));

				swprintf(  wchar1, L"%d", Sftk_Lg->LGroupNumber);
				swprintf(  wchar3, L"FRefresh FTDCRFFSTART");
				sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_DRIVER_PROTOCOL_MSG_ERROR, status, 
								  0, wchar1, wchar3 );
				// We can not do anything, so just go for sleep than later we come back here again 
				// Log Event Message here
				mm_release_buffer_for_refresh(Sftk_Lg);
				numOfArrayEvents = usedEventsCount;
				// TODO : Veera, Must complete this.....Add Event which get used to signal whenever connection arrives
				// So i can wait here...in case error ocure.
				if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
					sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
				else
				{
					Sftk_Lg->DoNotSendOutBandCommandForTracking = TRUE; // so not need to send Tracking outband FTDCHUP command
					sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
				}
				numOfArrayEvents = usedEventsCount;
				continue;
			}
		} // if: Full Refresh
		else
		{
			if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_SMART_REFRESH)
			{ // else: smart Refresh
				// Send OutBand for smart refresh start 
				if (Sftk_Lg->PrevState == SFTK_MODE_FULL_REFRESH)
					status = QM_SendOutBandPkt( Sftk_Lg, TRUE, TRUE, FTDMSGAVOIDJOURNALS);
				else
				{
					if (Sftk_Lg->SRefreshWasNotDone == FALSE)
					{
						status = QM_SendOutBandPkt( Sftk_Lg, TRUE, TRUE, FTDMSGINCO);
					}
					else
						status = STATUS_SUCCESS;
				}

				if (!NT_SUCCESS(status))
				{
					DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d : Failed Outband Start Full Refresh Proto command %d FTDMSGAVOIDJOURNALS %d, FTDMSGINCO %d!! ..\n", 
											Sftk_Lg->LGroupNumber, FTDMSGINCO, FTDMSGAVOIDJOURNALS, FTDMSGINCO));
					swprintf(  wchar1, L"%d", Sftk_Lg->LGroupNumber);
					swprintf(  wchar3, L"SRefresh FTDMSGINCO");
					sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_DRIVER_PROTOCOL_MSG_ERROR, status, 
									0, wchar1, wchar3 );
					// We can not do anything, so just go for sleep than later we come back here again 
					// Log Event Message here
					mm_release_buffer_for_refresh(Sftk_Lg);
					numOfArrayEvents = usedEventsCount;

					if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
						sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
					else
					{
						Sftk_Lg->DoNotSendOutBandCommandForTracking = TRUE; // so not need to send Tracking outband FTDCHUP command
						sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
					}

					numOfArrayEvents = usedEventsCount;
					continue;
				}
			} // else: smart Refresh
		}

		// Start Log Event Message here
		swprintf(  wchar1, L"%d", Sftk_Lg->LGroupNumber);
		sftk_get_stateString(Sftk_Lg->state, wchar2);
		sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_LG_REFRESH_STARTED, status, 
							  0, wchar1, wchar2);

		// TODO: Do we need lock to protect Device link list under Logical Group,
		// May be shared lock will be useful, check this and add it later if needed.
		// OS_ACQUIRE_LOCK( &Sftk_Lg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
		Sftk_Lg->RefreshFinishedParseI = FALSE;	// since we are starting Refresh ParseI 
		KeClearEvent( &Sftk_Lg->RefreshEmptyAckQueueEvent );
		bUsePnpEvents	= FALSE;	// by default do not use Number Of pnp Events
		bStateChanged	= FALSE;	// by default We are running refresh mode. no state change.

		numOfArrayEvents = usedEventsCount + 1;	// 1 Only wait for next state change

		if (bFullRefresh == FALSE)	
		{ // Smart resfresh
			if (Sftk_Lg->SRefreshWasNotDone == FALSE)
				Sftk_Lg->SRefreshWasNotDone = TRUE;
		}
	
		// Walk thru Local link list
		for(	plistEntry = Sftk_Lg->LgDev_List.ListEntry.Flink;
				plistEntry != &Sftk_Lg->LgDev_List.ListEntry;
				plistEntry = plistEntry->Flink )
		{ // for :scan thru each and every Device under LG 
			pSftk_Dev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

			if (pSftk_Dev->DevExtension == NULL)
			{
				// Ops, Source device is null means Src Disk Device has Removed thru PNP...
				DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d (pSftk_Dev->DevExtension == NULL): Can not do Refresh on this device %s its not connected..\n", 
										Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname));

				arrayOfEvents[numOfArrayEvents] = &pSftk_Dev->PnpEventDiskArrived;
				numOfArrayEvents ++;

				if (bUsePnpEvents == FALSE)
					bUsePnpEvents = TRUE;
				continue;
				// we must wait for event to get pnp disk arrival notifications
			}

			// At Current LG is doing refresh for this device, so store NumOfBlksPerBit value from current device
			Sftk_Lg->Statistics.NumOfBlksPerBit = pSftk_Dev->Hrdb.Sectors_per_bit;

			if ( sftk_lg_State_is_not_refresh(Sftk_Lg) )
			{
				bStateChanged = TRUE;
				break;
			}
			
			if (bFullRefresh == FALSE)	
			{ // Smart resfresh
				pSftk_Dev->RefreshLastBitIndex = 0;	// start 0 index
				pSftk_Dev->FirstDirtyBitNum = 0;
				
			}
			// For Full Refresh: Do not Reset Start Bit index we already have done this when we change state
			pSftk_Dev->UpdatePS_RefreshLastBitCounter = 0;

			endOffset.QuadPart	= pSftk_Dev->Hrdb.Sectors_per_volume * SECTOR_SIZE;  // Fixed Data size in bytes
			
			while( (sftk_get_next_refresh_dev_offset(	pSftk_Dev,		bFullRefresh, 
														&byteOffset,	&readSize,
														&endOffset)) == TRUE) 
			{ // While : scan thru each and every bit in bitmaps starting from 0

				OS_ASSERT( (readSize % SECTOR_SIZE) == 0);
				OS_ASSERT( (byteOffset.QuadPart % SECTOR_SIZE) == 0);

				bErrorInSegmentationLoop	= FALSE;
				offset.QuadPart	= byteOffset.QuadPart;
				totalReadDone	= 0;
				readLength		= Sftk_Lg->MaxTransferUnit;	// each cache buffer size

				// Check to see if the Full Refresh is Paused if so just sleep and continue
				// The Next Time since RefreshLastBitIndex is not incremented so we will get the
				// Same Offset and Size values.

				if( ( bFullRefresh == TRUE ) &&
					( sftk_lg_get_refresh_thread_state( Sftk_Lg ) == FALSE ) )
				{
					// Paused
					// Refresh is Paused so Delay for a few MilliSeconds and loopback.
					delayTime.QuadPart = DEFAULT_TIMEOUT_FOR_FULLREFRESH_PAUSE_WAIT;   // DEFAULT_TIMEOUT_FOR_FULLREFRESH_PAUSE_WAIT = 1 Sec					KeDelayExecutionThread( KernelMode, FALSE, &delayTime ); -(10*1000*1000)
					KeDelayExecutionThread( KernelMode, FALSE, &delayTime );
					continue;
				}

				do 
				{ // do-While : Do Segmentation of read if needed
					if ( (totalReadDone + readLength) > readSize)
					{ // Last boundary condition to read so truncate readLength if needed
						readLength = readSize - totalReadDone;	
					}

					OS_ASSERT( (readLength % SECTOR_SIZE) == 0);
					
					// Cache Manager : get next avaialble soft-header + data packet
					Sftk_Lg->RefreshNextBuffer	= NULL;
					pMM_Buffer					= NULL;
					// get next buffer, its Returns Buffer Pointer into Sftk_Lg->RefreshNextBuffer
					// status = mm_get_next_buffer_from_refresh_pool( Sftk_Lg );
					status = mm_get_buffer_from_refresh_pool( Sftk_Lg,  &pMM_Buffer);
					if (status != STATUS_SUCCESS)
					{
						DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d,  mm_get_buffer_from_refresh_pool() return UNSUCCESSFUL, Must have change state... PrevState %s NewState 0x%x!!\n", 
							Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));

						if (! (sftk_lg_State_is_not_refresh(Sftk_Lg)) )
						{
							DebugPrint((DBG_ERROR, "BUG FIXME FIXME sftk_refresh_lg_thread:: LG %d,  State must not be Refresh state !! PrevState %s NewState 0x%x BUG FIXME FIXME !!\n", 
										Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));
							OS_ASSERT(FALSE);

							// can't do anything change the state forcebly...
							if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
								sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
							else
								sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
						}
						bStateChanged = TRUE;
						bErrorInSegmentationLoop = TRUE;
						break;
					}
#if 0
					if (status == STATUS_PENDING)
					{ // wait for next buffer to be available
						bExitWaitloop = FALSE;
						countWaitloop = 0;
						do 
						{ // get pkts from refresh Queue
							/*
							// get next buffer, its Returns Buffer Pointer into Sftk_Lg->RefreshNextBuffer
							status = mm_get_next_buffer_from_refresh_pool( Sftk_Lg );
							if (status == STATUS_UNSUCCESSFUL)
							{
								DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d,  mm_get_next_buffer_from_refresh_pool() return UNSUCCESSFUL, Must have change state... PrevState %s NewState 0x%x!!\n", 
									Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));

								if (! (sftk_lg_State_is_not_refresh(Sftk_Lg)) )
								{
									DebugPrint((DBG_ERROR, "BUG FIXME FIXME sftk_refresh_lg_thread:: LG %d,  State must not be Refresh state !! PrevState %s NewState 0x%x BUG FIXME FIXME !!\n", 
												Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));
									OS_ASSERT(FALSE);

									// can't do anything change the state forcebly...
									if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
										sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
									else
										sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
								}
								bStateChanged = TRUE;
								bErrorInSegmentationLoop = TRUE;
								bExitWaitloop = TRUE;
								break;
							}
							*/
							// if (status == STATUS_PENDING)
							{ // wait for next buffer to be available
								eventsList[0] = &Sftk_Lg->RefreshStateChangeSemaphore;	// Stat change event Index 0	
								eventsList[1] = &Sftk_Lg->RefreshPktsWaitEvent;	// Cache Bufgfer ready event
								getnextpktTimeout.QuadPart = DEFAULT_TIMEOUT_FOR_REFRESHPOOL_GET_NEXT_PKT;

								noOfEvents = 2;
								// Wait For Data Buffer to be ready else state change
								// TRUE means data buffer is ready else state got changed
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
														break;
									case STATUS_WAIT_1:	// Index 1 got signaled for Sftk_Lg->RefreshPktsWaitEvent
														OS_ASSERT(Sftk_Lg->RefreshNextBuffer != NULL);	
														KeClearEvent( &Sftk_Lg->RefreshPktsWaitEvent); // not needed its Synchronize event 
														bExitWaitloop = TRUE;
														break;
									case STATUS_TIMEOUT:	// A time out occurred before the specified set of wait conditions was met. 
														DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d,GetNext Pkt : KeWaitForMultipleObjects(): returned STATUS_TIMEOUT, count %d !!\n", 
																		Sftk_Lg->LGroupNumber, countWaitloop));
									default:			break;
								}
							} // if (status == STATUS_PENDING)
							/*
							else
							{
								bExitWaitloop = TRUE;	// we got pkt
							}
							*/

							if (Sftk_Lg->RefreshNextBuffer != NULL)
							{
								KeClearEvent( &Sftk_Lg->RefreshPktsWaitEvent); // not needed its Synchronize event 
								break;
							}

							countWaitloop ++;
						} while(bExitWaitloop == FALSE);

						if (bErrorInSegmentationLoop == TRUE)
							break;
					} // wait for next buffer to be available
					pMM_Buffer = Sftk_Lg->RefreshNextBuffer;

					Sftk_Lg->RefreshNextBuffer = NULL;
#endif	// # if 0
					OS_ASSERT( pMM_Buffer != NULL);
					if ( sftk_lg_State_is_not_refresh(Sftk_Lg) )
					{
						if (pMM_Buffer)
						{ // free Refresh Buffer Pkt back to Refresh pool
							DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d Dev %s,LastBit %d, After getnext Pkts, State change, PrevState %s, NewState 0x%x!!\n", 
											Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, pSftk_Dev->RefreshLastBitIndex, 
											(bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));
							mm_free_buffer_to_refresh_pool( pSftk_Dev->SftkLg, pMM_Buffer );
						}
						else
						{
							DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d Dev %s,LastBit %d, While wait for getnextPkts,State change,PrevState %s, NewState 0x%x!!\n", 
											Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, pSftk_Dev->RefreshLastBitIndex, 
											(bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));

						}
						bStateChanged = TRUE;
						bErrorInSegmentationLoop = TRUE;
						break;
					}
				
					OS_ASSERT(pMM_Buffer != NULL);
					//  initialized Proto Hdr this 
					Proto_MMInitProtoHdrForRefresh( pMM_Buffer, offset.QuadPart, readLength, pSftk_Dev,0);

					// Zero Out Read Buffer
					pDataBuffer = MM_GetDataBufferPointer(pMM_Buffer);

					OS_ASSERT(pDataBuffer != NULL);
					OS_ZeroMemory( pDataBuffer, readLength);
			
					if ( sftk_lg_State_is_not_refresh(Sftk_Lg) )
					{ // free Refresh Buffer Pkt back to Refresh pool
						DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d Dev %s,LastBit %d, After ProtoInit,State change,PrevState %s, NewState 0x%x!!\n", 
											Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, pSftk_Dev->RefreshLastBitIndex, 
											(bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));

						mm_free_buffer_to_refresh_pool( pSftk_Dev->SftkLg, pMM_Buffer );

						bStateChanged = TRUE;
						bErrorInSegmentationLoop = TRUE;
						break;
					}
					// Read data from disk Asynchronously, This also registers read completion callback routine.
					status = sftk_async_read_data(	pSftk_Dev, 
													(PLARGE_INTEGER) &offset, 
													&readLength, 
													pDataBuffer, 
													pMM_Buffer); 

					if( (status != STATUS_SUCCESS) && (status != STATUS_PENDING) )
					{ 
						DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d Dev %s, sftk_async_read_data() Read Offset %I64d size %d, Failed with Error 0x%08x !!\n", 
											Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, offset.QuadPart, readLength, status ));
						// TODO : FIXME : Error Handling requires here
						DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: Fixme Fixme Filename %s, Line Number %d, Error Handling requires for sftk_async_read_data() failure ..\n",
										__FILE__, __LINE__));
						OS_ASSERT(FALSE);
						// Log Event
						swprintf(  wchar1, L"%d", Sftk_Lg->LGroupNumber);
						sftk_get_stateString(Sftk_Lg->state, wchar2);
						swprintf(  wchar3, L"Rd Src Device %d Offset %I64d size %d Failed", 
										pSftk_Dev->cdev, offset.QuadPart, readLength);
						sftk_LogEventString3( GSftk_Config.DriverObject, MSG_REPL_LG_REFRESH_FAILED, status, 
											  0, wchar1, wchar2, wchar3 );

						DebugPrint( ( DBG_ERROR, ": sftk_refresh_lg_thread: Refresh READ: Failed, Freeing Buffer and doing log event !!! \n"));
						// free Refresh Buffer Pkt back to Refresh pool
						mm_free_buffer_to_refresh_pool( pSftk_Dev->SftkLg, pMM_Buffer );

						// In Middle of operation disk got removed ....
						if (pSftk_Dev->DevExtension == NULL)
						{
							DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d Disk Dev %s got removed in middle of Read I/O, sftk_async_read_data() Igonring this and moving to next device... !!\n", 
											Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname));
							// Ops, Source device is null means Src Disk Device has Removed thru PNP...
							DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d (pSftk_Dev->DevExtension == NULL): Can not do Refresh on this device %s its not connected..\n", 
													Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname));

							arrayOfEvents[numOfArrayEvents] = &pSftk_Dev->PnpEventDiskArrived;
							numOfArrayEvents ++;

							if (bUsePnpEvents == FALSE)
								bUsePnpEvents = TRUE;
						}
						else
						{ // can't do anything change the state forcebly...
							if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
								sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);
							else
								sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);

							bStateChanged = TRUE;
						}
						bErrorInSegmentationLoop = TRUE;
						break;	// anyhow just break it here, we will try this device later....
					} // if( (status != STATUS_SUCCESS) && (status != STATUS_PENDING) )

					if (bNoDataSend == TRUE)
						bNoDataSend = FALSE;	// atleast one data has been send

					// Bump up the counters
					totalReadDone	+= readLength;
					offset.QuadPart += readLength;

				} while(totalReadDone < readSize); // do-While : Do Segmentation of read if needed

				if (bErrorInSegmentationLoop == TRUE)
					break;	// Error occurred 

				// Read completion callback will put this in commit queue, 
				// later Ack from secondary will clear this bit from bitmap from TDI worker thread.

				// go for next bit to read
				pSftk_Dev->RefreshLastBitIndex ++; // bump up next bit to start looking for next set bit
				// update stats
				pSftk_Dev->Statistics.TotalBitsDone ++;
				Sftk_Lg->Statistics.TotalBitsDone ++;
				pSftk_Dev->Statistics.CurrentRefreshBitIndex = pSftk_Dev->RefreshLastBitIndex;

				DebugPrint((DBG_RTHREAD, "@@@ sftk_refresh_lg_thread:: LG %d, %s: RefreshLastBitIndex %d, TotalBitsDone %I64d (LG %I64d) (TotalBitDirty %I64d)!!\n", 
							Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh",
							pSftk_Dev->RefreshLastBitIndex, pSftk_Dev->Statistics.TotalBitsDone, 
							Sftk_Lg->Statistics.TotalBitsDone,Sftk_Lg->Statistics.TotalBitsDirty));

				pSftk_Dev->UpdatePS_RefreshLastBitCounter ++;

				if ( ((pSftk_Dev->UpdatePS_RefreshLastBitCounter % DEFAULT_UPDATE_LASTBIT_TO_PS) == 0)
						||
					 (pSftk_Dev->RefreshLastBitIndex >= pSftk_Dev->Hrdb.TotalNumOfBits) )
				{
					pSftk_Dev->PsDev->RefreshLastBitIndex = pSftk_Dev->RefreshLastBitIndex;

					status = sftk_flush_psDev_to_pstore(pSftk_Dev, FALSE, NULL, TRUE);
					if (!NT_SUCCESS(status))
					{
						DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d Dev %s, sftk_flush_psDev_to_pstore() LastBitIndex %d, Failed with Error 0x%08x, ignoring error !!\n", 
											Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, pSftk_Dev->PsDev->RefreshLastBitIndex, status ));

						swprintf( wchar3, L"Flushing Dev LastBitIndex");

						sftk_LogEventNum2Wchar2(GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
									0, pSftk_Dev->cdev, pSftk_Dev->SftkLg->LGroupNumber, pSftk_Dev->SftkLg->PStoreFileName.Buffer, wchar3);

						status = STATUS_SUCCESS; // ignore error
					}
				}

				// nextBit is always zero based
				if (pSftk_Dev->RefreshLastBitIndex >= pSftk_Dev->Hrdb.TotalNumOfBits)
				{
					DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d Dev %s,LastBit %d, We are done with all bit for this device, PrevState %s, NewState 0x%x!!\n", 
											Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, pSftk_Dev->RefreshLastBitIndex, 
											(bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));
					pSftk_Dev->RefreshLastBitIndex = DEV_LASTBIT_CLEAN_ALL;	// we are done with complete bitmaps
					break;	// we are done with all bits
				}

				if ( sftk_lg_State_is_not_refresh(Sftk_Lg) )
				{
					DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d Dev %s,LastBit %d, After Async Read State change, PrevState %s, NewState 0x%x!!\n", 
											Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, pSftk_Dev->RefreshLastBitIndex, 
											(bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));
					bStateChanged = TRUE;
					break;
				}

			} // While : scan thru each and every bit in bitmaps starting from 0

			DebugPrint((DBG_ERROR, "!!! sftk_refresh_lg_thread:: Outside while LG %d, %s: RefreshLastBitIndex %d, TotalBitsDone %I64d (LG %I64d) (TotalBitDirty %I64d)!!\n", 
							Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh",
							pSftk_Dev->RefreshLastBitIndex, pSftk_Dev->Statistics.TotalBitsDone, 
							Sftk_Lg->Statistics.TotalBitsDone,Sftk_Lg->Statistics.TotalBitsDirty));


			if ( sftk_lg_State_is_not_refresh(Sftk_Lg) )
			{
				DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d Dev %s,LastBit %d, After While loop all bits state change, PrevState %s, NewState 0x%x!!\n", 
											Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, pSftk_Dev->RefreshLastBitIndex, 
											(bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));
				bStateChanged = TRUE;
				break;
			}

			// update last bit counter on pstore file for current device
			pSftk_Dev->PsDev->RefreshLastBitIndex = pSftk_Dev->RefreshLastBitIndex;

			status = sftk_flush_psDev_to_pstore(pSftk_Dev, FALSE, NULL, TRUE);
			if (!NT_SUCCESS(status))
			{
				DebugPrint((DBG_ERROR, "sftk_refresh_lg_thread:: LG %d Dev %s, sftk_flush_psDev_to_pstore() LastBitIndex %d, Failed with Error 0x%08x, ignoring error !!\n", 
									Sftk_Lg->LGroupNumber, pSftk_Dev->Vdevname, pSftk_Dev->PsDev->RefreshLastBitIndex, status ));
				// Log message
				swprintf( wchar3, L"Flushing Dev LastBitIndex");
				sftk_LogEventNum2Wchar2(GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
										0, pSftk_Dev->cdev, pSftk_Dev->SftkLg->LGroupNumber, 
										pSftk_Dev->SftkLg->PStoreFileName.Buffer, wchar3);
				status = STATUS_SUCCESS; // ignore error
			}
		} // for :scan thru each and every Device under LG 

		DebugPrint((DBG_ERROR, "--!!! sftk_refresh_lg_thread:: Outside FOR LG %d, %s: RefreshLastBitIndex %d, TotalBitsDone %I64d (LG %I64d) (TotalBitDirty %I64d)!!\n", 
							Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh",
							pSftk_Dev->RefreshLastBitIndex, pSftk_Dev->Statistics.TotalBitsDone, 
							Sftk_Lg->Statistics.TotalBitsDone,Sftk_Lg->Statistics.TotalBitsDirty));

		// trigger event mentioning that we have been terminated from smart refresh work due to state change.

		// since we either complete work successfully or stop in middle of smart refresh due
		// to change of events or due to Pnp Removal of disk (disk read failed !!).
		KeSetEvent( &Sftk_Lg->EventRefreshWorkStop, 0, FALSE);

		// OS_RELEASE_LOCK( &Sftk_Lg->Lock, NULL);

		if (bStateChanged == TRUE)
		{ // in middle of Refresh operations, state got changed....
			DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d, In middle of Refresh, State got change, PrevState %s, NewState 0x%x!!\n", 
											Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));
			numOfArrayEvents = usedEventsCount;	// 1 Only wait for next state change
			continue;
		}

		if (bUsePnpEvents == TRUE)
		{ // Refresh is not done, Log Event PNP is OFF, Pause Refresh
			DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d, Refresh paused due to PNP OFF, PrevState %s, NewState 0x%x!!\n", 
											Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));

			swprintf(  wchar1, L"%d", Sftk_Lg->LGroupNumber);
			sftk_get_stateString(Sftk_Lg->state, wchar2);
			swprintf(  wchar3, L"One of the Src Device Is Offline");
			sftk_LogEventString3( GSftk_Config.DriverObject, MSG_REPL_LG_REFRESH_PAUSED, status, 
						0, wchar1, wchar2, wchar3 );

			continue;
		}
		
		// Refresh has finished parseI, we just wqait till ACK arrives and trhan we change state
		if (bNoDataSend == TRUE)
		{ // we should signal here only for empty Ack wait event..
			KeSetEvent( &Sftk_Lg->RefreshEmptyAckQueueEvent, 0, FALSE);
		}

		Sftk_Lg->Statistics.NumOfBlksPerBit = 0;	// reset values 

		DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: LG %d, Refresh finished, PrevState %s, NewState 0x%x!!\n", 
											Sftk_Lg->LGroupNumber, (bFullRefresh == TRUE)?"FullRefresh":"Smart Refresh", Sftk_Lg->state ));
		numOfArrayEvents = usedEventsCount + 1;	// 1 for wait for empty signals
		Sftk_Lg->RefreshFinishedParseI = TRUE;
    } // while (Sftk_Lg->RefreshThreadShouldStop == FALSe) 

	if (pWaitBlockArray)
		OS_FreeMemory(pWaitBlockArray);

	DebugPrint((DBG_RTHREAD, "sftk_refresh_lg_thread:: Terminating Thread: Sftk_Lg 0x%08x for LG Num %d ! \n", 
								Sftk_Lg, Sftk_Lg->LGroupNumber));

	PsTerminateSystemThread( STATUS_SUCCESS );

} // sftk_refresh_lg_thread()    

//
// Returns TRUE means Valid Offsset and size is returned to do next chunk for refresh 
// FALSE means no more data is remain for refresh, refresh Phase I is completed for specific device
// 
BOOLEAN
sftk_get_next_refresh_dev_offset(	PSFTK_DEV		Sftk_Dev, 
									BOOLEAN			FullRefresh, 
									PLARGE_INTEGER	ByteOffset, 
									PULONG			ReadSize,
									PLARGE_INTEGER	EndOffset)
{
	// TODO : Currently we support only HRDB to do smart refresh
	if (FullRefresh == TRUE) 
	{ // Doing Full Refresh
		// Sftk_Dev->RefreshLastBitIndex ++; // We already have incremented before in caller aPI
		if (Sftk_Dev->RefreshLastBitIndex >= Sftk_Dev->Hrdb.TotalNumOfBits)
			Sftk_Dev->RefreshLastBitIndex = DEV_LASTBIT_CLEAN_ALL;	// we are done with complete bitmaps
	}
	else
	{ // doing smart refresh
		OS_ASSERT(Sftk_Dev->RefreshLastBitIndex <= Sftk_Dev->Hrdb.TotalNumOfBits);
		if (Sftk_Dev->RefreshLastBitIndex == 0)
		{ // this is first time, so store this bit number
			Sftk_Dev->FirstDirtyBitNum = RtlFindSetBits( Sftk_Dev->Hrdb.pBitmapHdr, 
															 1,								// default 1 bit at a time....
															 Sftk_Dev->RefreshLastBitIndex);
			Sftk_Dev->RefreshLastBitIndex = Sftk_Dev->FirstDirtyBitNum;
		}
		else
		{
			Sftk_Dev->RefreshLastBitIndex = RtlFindSetBits( Sftk_Dev->Hrdb.pBitmapHdr, 
															 1,								// default 1 bit at a time....
															 Sftk_Dev->RefreshLastBitIndex);

			if (Sftk_Dev->FirstDirtyBitNum == Sftk_Dev->RefreshLastBitIndex)
			{ // we are done, since RtlFindSetBits() works circularly, we got first bit which are done again....
				Sftk_Dev->RefreshLastBitIndex = DEV_LASTBIT_CLEAN_ALL;
				Sftk_Dev->FirstDirtyBitNum = 0;
			}
		}
	}

	if ( Sftk_Dev->RefreshLastBitIndex == DEV_LASTBIT_CLEAN_ALL )
		return FALSE; // Refresh is completed, we are done with refresh for this device.

	// calculate Offset to Read, since numOfBytesPerBit = chunk size in bytes per bit 
	ByteOffset->QuadPart = (LONGLONG) ( (INT64) Sftk_Dev->RefreshLastBitIndex * 
										(INT64) (Sftk_Dev->Hrdb.Sectors_per_bit * SECTOR_SIZE) );  

	OS_ASSERT( ByteOffset->QuadPart < EndOffset->QuadPart);

	*ReadSize = (ULONG) (Sftk_Dev->Hrdb.Sectors_per_bit * SECTOR_SIZE);

	if( (INT64) (ByteOffset->QuadPart + *ReadSize) > EndOffset->QuadPart)
	{ // Last Boundary offset
		DebugPrint((DBG_RTHREAD, "sftk_get_next_refresh_dev_offset:: Dev %s, Adjusting Disk Boundary Read Offset %I64d size %d to size %d \n", 
							Sftk_Dev->Vdevname, ByteOffset->QuadPart, *ReadSize, 
							(ULONG) (EndOffset->QuadPart - ByteOffset->QuadPart) ));

		*ReadSize = (ULONG) (EndOffset->QuadPart - ByteOffset->QuadPart);  

		if(*ReadSize == 0)		// we're overflowing, just exit nomrally
		{  // zero read size
			DebugPrint((DBG_RTHREAD, "sftk_get_next_refresh_dev_offset:: Dev %s, Adjusting Disk Boundary Read Offset %I64d size 0 !!\n", 
							Sftk_Dev->Vdevname, ByteOffset->QuadPart ));

			OS_ASSERT(FALSE);	// Break into Debugger

			if (FullRefresh == FALSE) 
			{ // Smart Refresh, we need to turn this bit to zero...
				RtlClearBits( Sftk_Dev->Hrdb.pBitmapHdr, Sftk_Dev->RefreshLastBitIndex, 1);
			}

			Sftk_Dev->RefreshLastBitIndex = DEV_LASTBIT_CLEAN_ALL;	// we are done with complete bitmaps
			return FALSE;	
		}  // zero read size
	} // Last Boundary offset
	return TRUE; // refresh is not completed
} // sftk_get_next_refresh_dev_offset()

NTSTATUS
sftk_CC_refresh_callback( PVOID Buffer, PVOID Context )
{
	PRCONTEXT pRContext = Context;

	// set buffer in SFTK_LG location
	pRContext->Sftk_Lg->RefreshNextBuffer = Buffer;
	KeSetEvent( &pRContext->Event, 0, FALSE);

	return STATUS_SUCCESS;
} // sftk_CC_refresh_callback()


NTSTATUS
sftk_async_read_data(	PSFTK_DEV		Sftk_Dev, 
						PLARGE_INTEGER	ByteOffset, 
						PULONG			NumBytes, 
						PVOID			Buffer, 
						PVOID			Context)
{
	NTSTATUS        status      = STATUS_SUCCESS;
    PIRP            pIrp        = NULL;
    IO_STATUS_BLOCK ioStatus    = { 0, 0 };

	if(*NumBytes == 0) 
	{ 
		DebugPrint((DBG_ERROR, "sftk_async_read_data:: Sftk_Dev %s : Sftk_Dev->DevExtension = 0x%08x, Zero-byte Read, returning success status!\n",
							Sftk_Dev->Vdevname, Sftk_Dev->DevExtension, STATUS_UNSUCCESSFUL)); 
		return STATUS_SUCCESS; 
	}
	
	if ( (Sftk_Dev->DevExtension == NULL) || (Sftk_Dev->DevExtension->TargetDeviceObject == NULL) )
	{
		DebugPrint((DBG_ERROR, "sftk_async_read_data:: Sftk_Dev %s : Sftk_Dev->DevExtension = 0x%08x, Sftk_Dev->DevExtension->TargetDeviceObject == NULL, returning status 0x%08x !\n",
							Sftk_Dev->Vdevname, Sftk_Dev->DevExtension, STATUS_UNSUCCESSFUL)); 
		return STATUS_UNSUCCESSFUL; 
	}

	// Enable this only for the timebeing we will just test it out and see the BandWidth and how much we can send
#if 0 // For Testing Only
	RtlFillMemory( Buffer, *NumBytes, 0xF1); 
	QM_Insert( Sftk_Dev->SftkLg, Context, REFRESH_QUEUE, TRUE);
	return STATUS_SUCCESS;
#endif

#if 0 // For Testing Only
	{ // synchronouse Read IO
		KEVENT	event;
		KeInitializeEvent( &event, SynchronizationEvent, FALSE);

		pIrp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,								// We're reading the volume!
											Sftk_Dev->DevExtension->TargetDeviceObject,	// Device to read from
											Buffer,										// Buffer to get input from
											*NumBytes,									// Number of bytes to read...
											ByteOffset,									// Offset to read from
											&event,
											&ioStatus );
		if(pIrp == NULL)
		{ 
			DebugPrint((DBG_ERROR, "sftk_async_read_data:: IoBuildSynchronousFsdRequest(Dev %s, O %I64d, S %d) failed to allocate IRP !! returning error 0x%08x !\n",
									Sftk_Dev->Vdevname, ByteOffset->QuadPart, *NumBytes, STATUS_INSUFFICIENT_RESOURCES)); 
			return STATUS_INSUFFICIENT_RESOURCES; 
		} 
		// Now call the driver with this IRP
		status = IoCallDriver(	Sftk_Dev->DevExtension->TargetDeviceObject,	// Device to read from
								pIrp );
		if(status == STATUS_PENDING )
		{
			KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
			status = ioStatus.Status;
		}

		if (status == STATUS_SUCCESS)
		{
			QM_Insert( Sftk_Dev->SftkLg, Context, REFRESH_QUEUE, TRUE);
		}
		else
		{ // Failed 
			WCHAR			wstr1[40], wstr2[40], wstr3[40], wstr4[40];

			DebugPrint( ( DBG_ERROR, "BUG FIXME FAILED: sftk_async_read_completion: Refresh READ: Context %X,O:%I64d,Size:%d, IRP status 0x%08x Information %d\n",
										Context,ByteOffset->QuadPart,*NumBytes, 
										ioStatus.Status, ioStatus.Information));
			// Log Event
			swprintf( wstr1, L"%d", Sftk_Dev->cdev);
			swprintf( wstr2, L"%d", Sftk_Dev->SftkLg->LGroupNumber);
			swprintf( wstr3, L"%I64d",ByteOffset->QuadPart );
			swprintf( wstr4, L"%d", *NumBytes);
			sftk_LogEventWchar4( GSftk_Config.DriverObject, MSG_REPL_SRC_DEVICE_READ_FAILED, status, 
									0, wstr1, wstr2, wstr3, wstr4);
			// mm_free_buffer_to_refresh_pool( Sftk_Dev->SftkLg, Context );
		}

		return status;
	}
#endif

#if 0
	// Allocate an IRP for this operation...
	pIrp = IoBuildAsynchronousFsdRequest(	IRP_MJ_READ,								// We're reading the volume!
											Sftk_Dev->DevExtension->TargetDeviceObject,	// Device to read from
											Buffer,										// Buffer to get input from
			                                *NumBytes,									// Number of bytes to read...
					                        ByteOffset,									// Offset to read from
							                &ioStatus );

	if(pIrp == NULL)
	{ 
		DebugPrint((DBG_ERROR, "sftk_async_read_data:: IoBuildAsynchronousFsdRequest(Dev %s, O %I64d, S %d) failed to allocate IRP !! returning error 0x%08x !\n",
								Sftk_Dev->Vdevname, ByteOffset->QuadPart, *NumBytes, STATUS_INSUFFICIENT_RESOURCES)); 
		return STATUS_INSUFFICIENT_RESOURCES; 
	} 
#else
	{	
	PIO_STACK_LOCATION pStack;

	pIrp = IoAllocateIrp(	Sftk_Dev->DevExtension->TargetDeviceObject->StackSize + 1, FALSE);

	if(pIrp == NULL)
	{ 
		DebugPrint((DBG_ERROR, "sftk_async_read_data:: IoAllocateIrp(Dev %s, O %I64d, S %d) failed to allocate IRP !! returning error 0x%08x !\n",
								Sftk_Dev->Vdevname, ByteOffset->QuadPart, *NumBytes, STATUS_INSUFFICIENT_RESOURCES)); 
		return STATUS_INSUFFICIENT_RESOURCES; 
	} 
	IoSetNextIrpStackLocation(pIrp);
	pStack = IoGetCurrentIrpStackLocation(pIrp);
	
	// Setup IRP for read or write
	pStack->MajorFunction						= IRP_MJ_READ;
	pStack->Parameters.Read.Length				= *NumBytes;
	pStack->Parameters.Read.ByteOffset.QuadPart = ByteOffset->QuadPart;
	
	// Setup I/O stack location
	IoCopyCurrentIrpStackLocationToNext(pIrp);

	pIrp->MdlAddress = MM_GetMdlPointer(Context);
	pIrp->AssociatedIrp.SystemBuffer = Buffer;	
	QM_Insert( Sftk_Dev->SftkLg, Context, REFRESH_PENDING_QUEUE, TRUE);
	}
#endif
	// The completion routine will free the irp so that we don't have to. 
	IoSetCompletionRoutine( pIrp, sftk_async_read_Completion, Context, TRUE, TRUE, TRUE );

	// Now call the driver with this IRP
	// ioStatus.Status may look like local, but we passed a pointer to it into the body of the IRP. We're cool. 
	return IoCallDriver(	Sftk_Dev->DevExtension->TargetDeviceObject,	// Device to read from
							pIrp );
} // sftk_async_read_data()

NTSTATUS	
sftk_async_read_Completion(	IN PDEVICE_OBJECT	DeviceObject, 
							IN PIRP				Irp, 
							IN PVOID			Context )
{ 
	PIO_STACK_LOCATION	currentIrpStack	= IoGetCurrentIrpStackLocation(Irp);
	PSFTK_DEV			pSftk_Dev;
	WCHAR				wstr1[40], wstr2[40], wstr3[40], wstr4[40];
	LONGLONG			offset;
	ULONG				length;
	PMM_PROTO_HDR		pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(Context);

	offset  = MM_GetOffsetFromProtoHdr(pMmProtoHdr);
	length  = MM_GetLengthFromProtoHdr(pMmProtoHdr);
	
	pSftk_Dev = ((PMM_PROTO_HDR) (MM_GetHdr(Context)))->SftkDev;

	// OS_ASSERT( deviceExtension->NodeId.NodeType == NODE_TYPE_FILTER_DEV);
	OS_ASSERT(pSftk_Dev != NULL);
	OS_ASSERT(pSftk_Dev->SftkLg != NULL);

	DebugPrint( ( DBG_COMPLETION, "sftk_async_read_completion: Context %X, Irp:%X,O:%I64d,Size:%d, IRP_Status 0x%08x Information %d\n",
										Context,Irp, offset,length,Irp->IoStatus.Status,Irp->IoStatus.Information));

	
	if( (MM_GetMdlPointer(Context) == NULL) && ( Irp->MdlAddress ) )
	{
		MmUnlockPages( Irp->MdlAddress);
        IoFreeMdl( Irp->MdlAddress);
    }
	

	if (Irp->IoStatus.Status == STATUS_SUCCESS) 
	{
		QM_Move( pSftk_Dev->SftkLg, Context, REFRESH_PENDING_QUEUE, REFRESH_QUEUE, TRUE);
		// QM_Insert( pSftk_Dev->SftkLg, Context, REFRESH_QUEUE, TRUE);
	}
	else
	{ // Failed
		DebugPrint( ( DBG_ERROR, "FAILED: sftk_async_read_completion: Refresh READ: Context %X, Irp:%X,O:%I64d,Size:%d, IRP status 0x%08x Information %d\n",
										Context,Irp, offset,length,
										Irp->IoStatus.Status,Irp->IoStatus.Information));
		/* Dispatch level problems
		// Log Event
		// TODO : Test this for Dispatch Level, Does it works !!
		swprintf( wstr1, L"%d", pSftk_Dev->cdev);
		swprintf( wstr2, L"%d", pSftk_Dev->SftkLg->LGroupNumber);
		swprintf( wstr3, L"%I64d",offset );
		swprintf( wstr4, L"%d", length);
		sftk_LogEventWchar4(GSftk_Config.DriverObject, MSG_REPL_SRC_DEVICE_READ_FAILED, Irp->IoStatus.Status, 
								0, wstr1, wstr2, wstr3, wstr4);
		*/
		QM_Remove( pSftk_Dev->SftkLg, Context, REFRESH_PENDING_QUEUE, TRUE);
		mm_free_buffer_to_refresh_pool( pSftk_Dev->SftkLg, Context );
	} // Failed

	// clean up after ourselves & leave 
	IoFreeIrp( Irp ); 
	
	// We've freed the IRP. If we don't returned "MORE_PROCESSING_REQUIRED" flag, then the I/O Mgr 
	//	may try to do more post-processing on the IRP, leading to a bugcheck. 
	return STATUS_MORE_PROCESSING_REQUIRED; 

} // sftk_async_read_Completion

	