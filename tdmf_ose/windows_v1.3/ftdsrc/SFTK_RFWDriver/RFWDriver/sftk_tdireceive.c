/**************************************************************************************

Module Name: sftk_tdireceive.c   
Author Name: Veera Arja
Description: Describes Modules: Protocol and CommunicationFunctions
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#include "sftk_main.h"


//This new version of receive will do all the Receive in this function and
//Not in the IOCompletion Callback
//For 

#define __NEW_RECEIVE__
#ifdef __NEW_RECEIVE__

//Set the IO Receive Event, Which is triggered whenever there is data on the Wire.
//The Receive Thread will wait for all the sessions in the Group.

NTSTATUS 
TDI_ReceiveEventHandler(
						IN PVOID TdiEventContext,     // Context From SetEventHandler
						IN CONNECTION_CONTEXT ConnectionContext,
						IN ULONG ReceiveFlags,
						IN ULONG BytesIndicated,
						IN ULONG BytesAvailable,
						OUT ULONG *BytesTaken,
						IN PVOID Tsdu,				// pointer describing this TSDU, typically a lump of bytes
						OUT PIRP *IoRequestPacket	// TdiReceive IRP if MORE_PROCESSING_REQUIRED.
						)
{
	PTCP_SESSION pSession = NULL;

	pSession = (PTCP_SESSION)TdiEventContext;

    DebugPrint((DBG_RECV, "TDI_ReceiveEventHandler(): Entry; EventContext: 0x%8.8X; ConnectionContext: 0x%8.8X; Flags: 0x%8.8X\n",
      (ULONG )TdiEventContext, (ULONG)ConnectionContext, ReceiveFlags)
      );

	DebugPrint((DBG_RECV, "TDI_ReceiveEventHandler(): Bytes Indicated = %d , Bytes Available = %d for session %lx\n",
						BytesIndicated,
						BytesAvailable,
						pSession));
	//Just set the Event and do nothing else
	*BytesTaken = 0;
	KeSetEvent(&pSession->IOReceiveEvent,0,FALSE);
	return STATUS_SUCCESS;
}// TDI_ReceiveEventHandler



NTSTATUS
TDI_EndpointReceiveRequestComplete2(
								IN PDEVICE_OBJECT pDeviceObject,
								IN PIRP pIrp,
								IN PVOID Context
								)
{
	PRECEIVE_BUFFER pReceiveBuffer		= NULL;
	PSESSION_MANAGER pSessionManager	= NULL;
	PTCP_SESSION pSession				= NULL;
	PSFTK_LG pSftkLg = NULL;
	NTSTATUS status						= STATUS_SUCCESS;
	ULONG Length						= 0;
	PKEVENT	pSyncEvent					= NULL;
	LARGE_INTEGER iCurrentTime;
	ULONG uCurTime						= 0;
//	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	status = pIrp->IoStatus.Status;
	Length = pIrp->IoStatus.Information;
	pReceiveBuffer = (PRECEIVE_BUFFER)Context;

	// Just to make sure
	OS_ASSERT(pReceiveBuffer != NULL);
	OS_ASSERT(pReceiveBuffer->pSession != NULL);
	OS_ASSERT(pReceiveBuffer->pSession->pServer != NULL);
	OS_ASSERT(pReceiveBuffer->pSession->pServer->pSessionManager != NULL);
	OS_ASSERT(pReceiveBuffer->pSession->pServer->pSessionManager->pLogicalGroupPtr != NULL);

	pSessionManager = pReceiveBuffer->pSession->pServer->pSessionManager;
	pSession = pReceiveBuffer->pSession;
	pSftkLg = pSessionManager->pLogicalGroupPtr;

	if(NT_SUCCESS(status))
	{
		DebugPrint((DBG_RECV, "TDI_EndpointReceiveRequestComplete2(): Receive %d Bytes and Index = %d for ReceiveBuffer = %lx\n", Length, pReceiveBuffer->index , pReceiveBuffer));
		pReceiveBuffer->ActualReceiveLength += Length;  //Increment the Actual Length

		// Initialize the Last Packet Received Time
		KeQuerySystemTime(&iCurrentTime);
		uCurTime = (ULONG)(iCurrentTime.QuadPart/(10*1000*1000));
		pSessionManager->LastPacketReceiveTime = uCurTime;
	}
	else
	{
//		swprintf(wchar1,L"%d",pSftkLg->LGroupNumber);
//		swprintf(wchar2,L"0X%08X",pSession->pServer);
//		swprintf(wchar3,L"%d",pReceiveBuffer->index);
//		swprintf(wchar4,L"0X%08X",status);
//		sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_RECEIVE_COMPLETION_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

		DebugPrint((DBG_RECV, "TDI_EndpointReceiveRequestComplete2(): status 0x%8.8X and Index = %d for ReceiveBuffer = %lx\n", status , pReceiveBuffer->index , pReceiveBuffer));

		// Lets do the Sync Part, if the Receive IRP Fails for some reason, then its better to set 
		// the error code and then trigger the event so that no other thread gets hung 
		pSyncEvent = InterlockedExchangePointer(&pSessionManager->pSyncReceivedEvent, NULL);
		if(pSyncEvent != NULL)
		{
			// Set the Error Code and then Trigger the Event so that the Receive Call will not Hang
			pSessionManager->ReceivedProtocolHeader.msgtype = FTDRECEIVEERROR;
			KeSetEvent(pSyncEvent,0,FALSE);
		}

		if(status != STATUS_IO_TIMEOUT)
		{
			// Reset the Particular Server that is giving you error.
			pSession->pServer->bReset = TRUE;
		}
		else
		{
			DebugPrint((DBG_SEND, "TDI_EndpointReceiveRequestComplete2(): Ignoring TIMEOUT status 0x%08x\n", status ));
		}
	}

	TDI_UnlockAndFreeMdl(pReceiveBuffer->pReceiveMdl);	//Free the MDL

	// Free up the IRP
	if(pSession->pReceiveHeader->pReceiveIrp != NULL)
	{
		OS_ASSERT(pSession->pReceiveHeader->pReceiveIrp == pIrp);
		IoFreeIrp(pSession->pReceiveHeader->pReceiveIrp);
		pSession->pReceiveHeader->pReceiveIrp = NULL;
	}

	pReceiveBuffer->pReceiveMdl = NULL;

	return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS 
COM_ReceiveThread2(
			PSESSION_MANAGER pSessionManager
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	ULONG nDataLength = 0;
	PSFTK_LG pSftkLg = NULL;
	KIRQL oldIrql;
	PKEVENT pSyncEvent	= NULL;
	LARGE_INTEGER iCurrentTime;
	ULONG uCurTime = 0;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager);
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	DebugPrint((DBG_RECV, "COM_ReceiveThread2(): Enter COM_ReceiveThread2()\n"));

	// Initialize the Last Packet Received Time
	KeQuerySystemTime(&iCurrentTime);
	uCurTime = (ULONG)(iCurrentTime.QuadPart/(10*1000*1000));
	pSessionManager->LastPacketReceiveTime = uCurTime;

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	swprintf(wchar3,L"%S",L"COM_ReceiveThread2");
	sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_STARTED,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);


	try
	{
		try
		{

//			iZeroWait.QuadPart = -(10*10*1000);
			iZeroWait.QuadPart = 0;
			while(!bExit)
			{
				try
				{
					//Acquire the resource as shared Lock
					//This Resource will not be acquired if there is a call for Exclusive Lock

					KeEnterCriticalRegion();	//Disable the Kernel APC's
					ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

					//We already disabled the APC's so use the UnSafe method to acquire the
					//Receive Lock
//					ExAcquireFastMutexUnsafe(&pSessionManager->ReceiveLock);


					//30 Milli-Seconds in 100*Nano Seconds
					iWait.QuadPart = -(10*10*1000);
					pTemp = pSessionManager->ServerList.Flink;

					while(pTemp != &pSessionManager->ServerList && !bExit)
					{
//						DebugPrint((DBG_RECV, "COM_ReceiveThread2(): Going for the Server List Also in COM_ReceiveThread2()\n"));
						pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);
						pTemp = pTemp->Flink;

						if(pServerElement == NULL)
						{
							DebugPrint((DBG_RECV, "COM_ReceiveThread2(): The SERVER_ELEMENT is Null and hence cannot Proceed\n"));
							status = STATUS_MEMORY_NOT_ALLOCATED;
							break;
						}
						
						OS_ASSERT(pServerElement);
						OS_ASSERT(pServerElement->pSession);

						if(pServerElement->pSession != NULL && !bExit)
						{
							OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

							status = KeWaitForSingleObject(&pSessionManager->IOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
							if(status == STATUS_SUCCESS)
							{
								swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
								swprintf(wchar2,L"0X%08X",pSessionManager);
								sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXITING,status,0,wchar1,wchar2);
								DebugPrint((DBG_RECV, "COM_ReceiveThread2(): The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
								bExit = TRUE;
								break;
							}

							if(pServerElement->pSession->bSessionEstablished == SFTK_CONNECTED) 
							{
								if(pServerElement->pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_NOTINIT)
								{
									pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_HEADER;

									DebugPrint((DBG_RECV, "COM_ReceiveThread2(): The Session that is used to receive Data is %lx\n",pServerElement->pSession));

									pServerElement->pSession->pReceiveHeader->TotalReceiveLength = PROTOCOL_HEADER_SIZE;

									if(pServerElement->pSession->pReceiveHeader->pReceiveMdl == NULL)
									{
										pServerElement->pSession->pReceiveHeader->pReceiveMdl = TDI_AllocateAndProbeMdl(pServerElement->pSession->pReceiveHeader->pReceiveBuffer , pServerElement->pSession->pReceiveHeader->TotalReceiveLength ,FALSE,FALSE,NULL);
									}

									OS_ASSERT(pServerElement->pSession->pReceiveHeader->pReceiveMdl);

									(pServerElement->pSession->pReceiveHeader->pReceiveMdl)->Next = NULL;   // IMPORTANT!!!


									pServerElement->pSession->pReceiveHeader->ActualReceiveLength = 0;

									try
									{
										// Lets try without Reuse IRP
//										if(pServerElement->pSession->pReceiveHeader->pReceiveIrp != NULL)
//										{
//											IoReuseIrp(pServerElement->pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
//										}
										status = TDI_ReceiveOnEndpoint1(
													&pServerElement->pSession->KSEndpoint,
													NULL,       // User Completion Event
													TDI_EndpointReceiveRequestComplete2,// User Completion Routine
													pServerElement->pSession->pReceiveHeader,   // User Completion Context
													&pServerElement->pSession->pReceiveHeader->IoReceiveStatus,
													pServerElement->pSession->pReceiveHeader->pReceiveMdl,
													0,           // Flags
													&pServerElement->pSession->pReceiveHeader->pReceiveIrp
													);

									if(	(status == STATUS_INSUFFICIENT_RESOURCES) && 
										(pServerElement->pSession->pReceiveHeader->pReceiveIrp == NULL))
										{
											swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
											swprintf(wchar2,L"0X%08X",pServerElement);
											swprintf(wchar3,L"0X%08X",status);
											sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,status,0,wchar1,wchar2,wchar3);

											DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): An Exception Occured in COM_ReceiveThread2() Error Code is %lx\n",status));
											// Failed to Allocate IRP hence just set the Buffer to Free 
											TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveHeader->pReceiveMdl);	//Free the MDL
											pServerElement->pSession->pReceiveHeader->pReceiveMdl = NULL;
											pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
										}
									}
									except(EXCEPTION_EXECUTE_HANDLER)
									{
										swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
										swprintf(wchar2,L"0X%08X",pServerElement);
										swprintf(wchar3,L"0X%08X",GetExceptionCode());
										sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,GetExceptionCode(),0,wchar1,wchar2,wchar3);

										DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): An Exception Occured in COM_ReceiveThread2() Error Code is %lx\n",GetExceptionCode()));
										status = GetExceptionCode();

										TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveHeader->pReceiveMdl);	//Free the MDL
										pServerElement->pSession->pReceiveHeader->pReceiveMdl = NULL;
										pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
									}
								}
								else if(pServerElement->pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_HEADER)
								{
									//This will assure that the Previous CompletionRoutine completed
									//Successfully, before calling the IoCallDriver() again
									//We can use an Event here, but it does the samething
									if(pServerElement->pSession->pReceiveHeader->pReceiveMdl == NULL)
									{
										if(pServerElement->pSession->pReceiveHeader->ActualReceiveLength < pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
										{
											//We still Need to read somemore buffer
											pServerElement->pSession->pReceiveHeader->pReceiveMdl = TDI_AllocateAndProbeMdl(pServerElement->pSession->pReceiveHeader->pReceiveBuffer + pServerElement->pSession->pReceiveHeader->ActualReceiveLength,pServerElement->pSession->pReceiveHeader->TotalReceiveLength-pServerElement->pSession->pReceiveHeader->ActualReceiveLength,FALSE,FALSE,NULL);
											//Call the TDI_RECEIVE with the new MDL
											try
											{
												//Lets try without Reuse IRP
//												if(pServerElement->pSession->pReceiveHeader->pReceiveIrp != NULL)
//												{
//													IoReuseIrp(pServerElement->pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
//												}
												status = TDI_ReceiveOnEndpoint1(
															&pServerElement->pSession->KSEndpoint,
															NULL,       // User Completion Event
															TDI_EndpointReceiveRequestComplete2,// User Completion Routine
															pServerElement->pSession->pReceiveHeader,   // User Completion Context
															&pServerElement->pSession->pReceiveHeader->IoReceiveStatus,
															pServerElement->pSession->pReceiveHeader->pReceiveMdl,
															0,           // Flags
															&pServerElement->pSession->pReceiveHeader->pReceiveIrp
															);

													if(	(status == STATUS_INSUFFICIENT_RESOURCES) && 
														(pServerElement->pSession->pReceiveHeader->pReceiveIrp == NULL))
												{
													swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
													swprintf(wchar2,L"0X%08X",pServerElement);
													swprintf(wchar3,L"0X%08X",status);
													sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,status,0,wchar1,wchar2,wchar3);

													DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): An Exception Occured in COM_ReceiveThread2() Error Code is %lx\n",status));
													// Failed to Allocate IRP hence just set the Buffer to Free 
													TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveHeader->pReceiveMdl);	//Free the MDL
													pServerElement->pSession->pReceiveHeader->pReceiveMdl = NULL;
													pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
												}
											}
											except(EXCEPTION_EXECUTE_HANDLER)
											{
												swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
												swprintf(wchar2,L"0X%08X",pServerElement);
												swprintf(wchar3,L"0X%08X",GetExceptionCode());
												sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,GetExceptionCode(),0,wchar1,wchar2,wchar3);

												DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): An Exception Occured in COM_ReceiveThread2() Error Code is %lx\n",GetExceptionCode()));
												status = GetExceptionCode();

												TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveHeader->pReceiveMdl);	//Free the MDL
												pServerElement->pSession->pReceiveHeader->pReceiveMdl = NULL;
												pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
											}


										}
										else
										{
											nDataLength = 0;
											status = PROTO_ProcessReceiveHeader(((ftd_header_t*)pServerElement->pSession->pReceiveHeader->pReceiveBuffer),&nDataLength,pServerElement->pSession);
											//Received the Total header so process it and read the Data if 
											//there is any

											if( status == STATUS_SUCCESS || status == STATUS_MORE_PROCESSING_REQUIRED)
											{
												if(nDataLength > 0)
												{
													// Check if the Size of the header to be received is less than the Size of the Avalibale Receive Window.
													OS_ASSERT(nDataLength <= pServerElement->pSession->pReceiveBuffer->ActualReceiveWindow);

													status = COM_GetSessionReceiveBuffer(pServerElement->pSession);

													if(!NT_SUCCESS(status))
													{
														swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
														swprintf(wchar2,L"0X%08X",pSessionManager);
														swprintf(wchar3,L"0X%08X",pServerElement->pSession->pReceiveBuffer->ActualReceiveWindow);
														sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_BUFFER_NOT_AVAILABLE,status,0,wchar1,wchar2,wchar3);

														//There is no Free Buffer available so returning
														DebugPrint((DBG_RECV, "COM_ReceiveThread2(): Unable to get the Free Buffer to Receive Data for Session %lx\n",pServerElement->pSession));
														leave;
														// return STATUS_MORE_PROCESSING_REQUIRED;
													}

													OS_ASSERT(pServerElement->pSession->pReceiveBuffer);
													OS_ASSERT(pServerElement->pSession->pReceiveBuffer->pReceiveBuffer);

													//This is the new Buffer to call TDI_RECEIVE
													pServerElement->pSession->pReceiveBuffer->state = SFTK_BUFFER_INUSE;
													pServerElement->pSession->pReceiveBuffer->ActualReceiveLength = 0;
													pServerElement->pSession->pReceiveBuffer->TotalReceiveLength = nDataLength;
													pServerElement->pSession->pReceiveBuffer->pReceiveMdl = TDI_AllocateAndProbeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveBuffer,pServerElement->pSession->pReceiveBuffer->TotalReceiveLength ,FALSE,FALSE,NULL);
													pServerElement->pSession->pReceiveBuffer->pReceiveMdl->Next = NULL;
													pServerElement->pSession->pReceiveBuffer->pSession = pServerElement->pSession;

													OS_ASSERT(pServerElement->pSession->pReceiveBuffer->pReceiveMdl);

													// There is still more data to receive so set the Status to SFTK_PROCESSING_RECEIVE_DATA.
													pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_DATA;

													//Lets try without Reuse IRP
//													if(pServerElement->pSession->pReceiveHeader->pReceiveIrp != NULL)
//													{
//														IoReuseIrp(pServerElement->pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
//													}
													try
													{
														status = TDI_ReceiveOnEndpoint1(
																	&pServerElement->pSession->KSEndpoint,
																	NULL,       // User Completion Event
																	TDI_EndpointReceiveRequestComplete2,// User Completion Routine
																	pServerElement->pSession->pReceiveBuffer,   // User Completion Context
																	&pServerElement->pSession->pReceiveBuffer->IoReceiveStatus,
																	pServerElement->pSession->pReceiveBuffer->pReceiveMdl,
																	0,           // Flags
																	&pServerElement->pSession->pReceiveHeader->pReceiveIrp
																	);
													if(	(status == STATUS_INSUFFICIENT_RESOURCES) && 
														(pServerElement->pSession->pReceiveHeader->pReceiveIrp == NULL))
														{
															swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
															swprintf(wchar2,L"0X%08X",pServerElement);
															swprintf(wchar3,L"0X%08X",status);
															sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,status,0,wchar1,wchar2,wchar3);

															DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): An Exception Occured in COM_ReceiveThread2() Error Code is %lx\n",status));
															// Failed to Allocate IRP hence just set the Buffer to Free 
															TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveMdl);	//Free the MDL
															pServerElement->pSession->pReceiveBuffer->pReceiveMdl = NULL;
															pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
														}
													}
													except(EXCEPTION_EXECUTE_HANDLER)
													{
														DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): An Exception Occured in COM_ReceiveThread2() Error Code is %lx\n",GetExceptionCode()));
														status = GetExceptionCode();

														TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveMdl);	//Free the MDL
														pServerElement->pSession->pReceiveBuffer->pReceiveMdl = NULL;
														pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
													}
												}
												else
												{
													/*
													swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
													swprintf(wchar2,L"0X%08X",pServerElement);
													swprintf(wchar3,L"%d",((ftd_header_t*)pServerElement->pSession->pReceiveHeader->pReceiveBuffer)->msgtype);
													swprintf(wchar4,L"%d",pServerElement->pSession->pReceiveHeader->TotalReceiveLength);
													sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_REPL_DRIVER_PROTOCOL_RECEIVED_PACKET,STATUS_SUCCESS,0,wchar1,wchar2,wchar3,wchar4);
													*/

													//Data is Read completely
													//So can release the Buffer and set the Session Receive status to unused
													DebugPrint((DBG_RECV, "COM_ReceiveThread2(): Completed Reading the Whole Header for Session %lx and Length %ld and No PayLoad\n", pServerElement->pSession, pServerElement->pSession->pReceiveHeader->TotalReceiveLength));

													if(LG_IS_PRIMARY_MODE(pSessionManager->pLogicalGroupPtr))
													{ // The Current Role is PRIMARY So just do the primary Processing
														pSyncEvent = InterlockedExchangePointer(&pSessionManager->pSyncReceivedEvent, NULL);
														if(pSyncEvent != NULL)
														{
															OS_RtlCopyMemory(&pSessionManager->ReceivedProtocolHeader, pServerElement->pSession->pReceiveHeader->pReceiveBuffer , PROTOCOL_HEADER_SIZE);
															pSessionManager->pOutputBuffer = NULL;
															pSessionManager->OutputBufferLenget = 0;
															KeSetEvent(pSyncEvent,0,FALSE);
														}
														else if(pSessionManager->bSendHandshakeInformation == FALSE)
														{
															if( PROTO_IsAckCheckRequired( ( ftd_header_t* ) pServerElement->pSession->pReceiveHeader->pReceiveBuffer ) == TRUE )
															{
																// Removing the Packet from the Queue Without Payload
																status = QM_RemovePktsFromMigrateQueue(pSessionManager->pLogicalGroupPtr,
																					pServerElement->pSession->pReceiveHeader->pReceiveBuffer,
																					NULL,
																					0);
																if (!NT_SUCCESS(status))
																{ // nothing to do. ignore error
																	DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): QM_RemovePktsFromMigrateQueue() failed with error %lx Terminating and Resetting Connection\n",status));
																	// We need to reset connection and terminate this thread
																	COM_ResetAllConnections(pSessionManager);
																	bExit = TRUE;
																	leave;
																}
															}
															else
															{ // The Packet Received is not for ACK Check so Just Need to Process it.

															}
														}
													}
													else
													{ // The Current Role is SECONDARY so do the Secondary Stuff
														PMM_HOLDER pMM_Buffer = NULL;
														// Put this Packet into the Receive Queue. 
														// There is no Data Buffer Only the Protocol Header So Size = 0.
														pMM_Buffer = mm_alloc_buffer( pSessionManager->pLogicalGroupPtr, 0, PROTO_HDR, TRUE); 
														if(pMM_Buffer == NULL)
														{
															DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): mm_alloc_buffer() failed to allocate the Memory for MM_HOLDER with MM_PROTO_HDR for Size Zero for LG %d\n",pSessionManager->pLogicalGroupPtr->LGroupNumber));
															// TODO Do the Error Recovery Seems like we are out of Resources so should we wait or reset the connections and exit.
															COM_ResetAllConnections(pSessionManager);
															bExit = TRUE;
															leave;
														}
														OS_ASSERT(pMM_Buffer->pProtocolHdr != NULL);
														// Copy the Header 
														OS_RtlCopyMemory(pMM_Buffer->pProtocolHdr, pServerElement->pSession->pReceiveHeader->pReceiveBuffer , PROTOCOL_HEADER_SIZE);
														// Insert the MM_HOLDER into the TReceiveQueue.
														QM_Insert(pSessionManager->pLogicalGroupPtr,pMM_Buffer, TRECEIVE_QUEUE, TRUE);
														// This Data will be available for the Target Write Thread.
													}

													// Increment the Receive Performance Counter
													pSftkLg = pSessionManager->pLogicalGroupPtr;

													if(pSftkLg != NULL)
													{
														KeAcquireSpinLock(&pSessionManager->StatisticsLock , &oldIrql);

														pServerElement->SessionPerformanceInformation.nPacketsReceived++;
														pServerElement->SessionPerformanceInformation.nBytesReceived += pServerElement->pSession->pReceiveHeader->TotalReceiveLength;

														if(pServerElement->SessionPerformanceInformation.nMinimumReceivedPacketSize >  pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
														{
															pServerElement->SessionPerformanceInformation.nMinimumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
														}

														if(pServerElement->SessionPerformanceInformation.nMaximumReceivedPacketSize <  pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
														{
															pServerElement->SessionPerformanceInformation.nMaximumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
														}
														pServerElement->SessionPerformanceInformation.nAverageReceivedPacketSize = (ULONG)(pServerElement->SessionPerformanceInformation.nBytesReceived / pServerElement->SessionPerformanceInformation.nPacketsReceived);

														pSftkLg->Statistics.SM_PacketsReceived++;
														pSftkLg->Statistics.SM_BytesReceived += pServerElement->pSession->pReceiveHeader->TotalReceiveLength;

														if(pSftkLg->Statistics.SM_MinimumReceivedPacketSize > pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
														{
															pSftkLg->Statistics.SM_MinimumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
														}

														if(pSftkLg->Statistics.SM_MaximumReceivedPacketSize < pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
														{
															pSftkLg->Statistics.SM_MaximumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
														}
														pSftkLg->Statistics.SM_AverageReceivedPacketSize = (ULONG)(pSftkLg->Statistics.SM_BytesReceived / pSftkLg->Statistics.SM_PacketsReceived);

														KeReleaseSpinLock(&pSessionManager->StatisticsLock , oldIrql);
													}

													// Release the Spin Lock after the increment operation


													pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
												}//else
											}//if( status == STATUS_SUCCESS || status == STATUS_MORE_PROCESSING_REQUIRED)
											else
											{
												swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
												swprintf(wchar2,L"0X%08X",pServerElement);
												sftk_LogEventString2(GSftk_Config.DriverObject,MSG_REPL_DRIVER_PROTOCOL_INVALID_RECEIVE_PACKET_ERROR,status,0,wchar1,wchar2);

												// Got an error while Processing the returned ACK
												// Cannot do much just leave the thread
												DebugPrint((DBG_RECV, "COM_ReceiveThread2(): The received data is not recognized for Session %lx so exiting \n", pServerElement->pSession));
												bExit = TRUE;
												leave;
											}
										}//else
									}//if(pServerElement->pSession->pReceiveHeader->pReceiveMdl == NULL)
									else
									{
										iWait.QuadPart = -(10*10*1000);
										KeDelayExecutionThread(KernelMode,FALSE,&iWait);
									}
								}//else if(pServerElement->pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_HEADER)
								else	//This is pServerElement->pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_DATA
								{
									//This will assure that the Previous CompletionRoutine completed
									//Successfully, before calling the IoCallDriver() again
									//We can use an Event here, but it does the samething
									OS_ASSERT(pServerElement->pSession->pReceiveBuffer);
									if(pServerElement->pSession->pReceiveBuffer->pReceiveMdl == NULL)
									{
										if(pServerElement->pSession->pReceiveBuffer->ActualReceiveLength < pServerElement->pSession->pReceiveBuffer->TotalReceiveLength)
										{
											//Still Need to read sommore data
											//We still Need to read somemore buffer
											pServerElement->pSession->pReceiveBuffer->pReceiveMdl = TDI_AllocateAndProbeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveBuffer + pServerElement->pSession->pReceiveBuffer->ActualReceiveLength,pServerElement->pSession->pReceiveBuffer->TotalReceiveLength-pServerElement->pSession->pReceiveBuffer->ActualReceiveLength,FALSE,FALSE,NULL);
											//Call the TDI_RECEIVE with the new MDL
											try
											{
												//Lets Try without Reuse IRP
//												if(pServerElement->pSession->pReceiveHeader->pReceiveIrp != NULL)
//												{
//													IoReuseIrp(pServerElement->pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
//												}
												status = TDI_ReceiveOnEndpoint1(
															&pServerElement->pSession->KSEndpoint,
															NULL,       // User Completion Event
															TDI_EndpointReceiveRequestComplete2,// User Completion Routine
															pServerElement->pSession->pReceiveBuffer,   // User Completion Context
															&pServerElement->pSession->pReceiveBuffer->IoReceiveStatus,
															pServerElement->pSession->pReceiveBuffer->pReceiveMdl,
															0,           // Flags
															&pServerElement->pSession->pReceiveHeader->pReceiveIrp
															);
													if(	(status == STATUS_INSUFFICIENT_RESOURCES) && 
														(pServerElement->pSession->pReceiveHeader->pReceiveIrp == NULL))
												{
													swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
													swprintf(wchar2,L"0X%08X",pServerElement);
													swprintf(wchar3,L"0X%08X",status);
													sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,status,0,wchar1,wchar2,wchar3);

													DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): An Exception Occured in COM_ReceiveThread2() Error Code is %lx\n",status));
													// Failed to Allocate IRP hence just set the Buffer to Free 
													TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveMdl);	//Free the MDL
													pServerElement->pSession->pReceiveBuffer->pReceiveMdl = NULL;
													pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
												}
											}
											except(EXCEPTION_EXECUTE_HANDLER)
											{
												swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
												swprintf(wchar2,L"0X%08X",pServerElement);
												swprintf(wchar3,L"0X%08X",GetExceptionCode());
												sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,GetExceptionCode(),0,wchar1,wchar2,wchar3);

												DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): An Exception Occured in COM_ReceiveThread2() Error Code is %lx\n",GetExceptionCode()));
												status = GetExceptionCode();

												TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveMdl);	//Free the MDL
												pServerElement->pSession->pReceiveBuffer->pReceiveMdl = NULL;
												pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
											}
										}
										else
										{
											//Data is Read completely
											//So can release the Buffer and set the Session Receive status to unused

											nDataLength = 0;
											status = PROTO_ProcessReceiveData(((ftd_header_t*)pServerElement->pSession->pReceiveHeader->pReceiveBuffer),pServerElement->pSession->pReceiveBuffer->pReceiveBuffer,pServerElement->pSession->pReceiveBuffer->TotalReceiveLength,&nDataLength,pServerElement->pSession);

																								// Check if the Size of the header to be received is less than the Size of the Avalibale Receive Window.

											if(status == STATUS_MORE_PROCESSING_REQUIRED)
											{
												//Still More Data to be Read so Increase the TotalSize 
												//and try again
												pServerElement->pSession->pReceiveBuffer->TotalReceiveLength += nDataLength;

												// Check if the Total Length that we have to read excceds the Receive Window Size
												OS_ASSERT(pServerElement->pSession->pReceiveBuffer->TotalReceiveLength <= pServerElement->pSession->pReceiveBuffer->ReceiveWindow);
											}
											else if(status == STATUS_SUCCESS)
											{
												/*
												swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
												swprintf(wchar2,L"0X%08X",pServerElement);
												swprintf(wchar3,L"%d",((ftd_header_t*)pServerElement->pSession->pReceiveHeader->pReceiveBuffer)->msgtype);
												swprintf(wchar4,L"%d",pServerElement->pSession->pReceiveBuffer->TotalReceiveLength);
												sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_REPL_DRIVER_PROTOCOL_RECEIVED_PACKET_WITH_PAYLOAD,STATUS_SUCCESS,0,wchar1,wchar2,wchar3,wchar4);
												*/

												//All the Data is Read, so fine
												DebugPrint((DBG_RECV, "COM_ReceiveThread2(): Completed Reading the Whole Data for Session %lx and Length %ld\n", pServerElement->pSession, pServerElement->pSession->pReceiveBuffer->TotalReceiveLength));

												if(LG_IS_PRIMARY_MODE(pSessionManager->pLogicalGroupPtr))
												{
													pSyncEvent = InterlockedExchangePointer(&pSessionManager->pSyncReceivedEvent, NULL);

													if(pSyncEvent != NULL)
													{
														try
														{
															OS_RtlCopyMemory(&pSessionManager->ReceivedProtocolHeader, pServerElement->pSession->pReceiveHeader->pReceiveBuffer , PROTOCOL_HEADER_SIZE);

															status = NdisAllocateMemoryWithTag(
																							&pSessionManager->pOutputBuffer,
																							pServerElement->pSession->pReceiveBuffer->TotalReceiveLength,
																							MEM_TAG
																							);

															if(!NT_SUCCESS(status))
															{
																swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
																swprintf(wchar2,L"%S",L"COM_ReceiveThread2::pSessionManager->pOutputBuffer");
																swprintf(wchar3,L"%d",pServerElement->pSession->pReceiveBuffer->TotalReceiveLength);
																swprintf(wchar4,L"0X%08X",status);
																sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_COM_MEMORY_ALLOCATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

																DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): Unable to Accocate Memory for Received Data status = %lx\n",status));
																leave;
															}

															NdisZeroMemory(pSessionManager->pOutputBuffer,pServerElement->pSession->pReceiveBuffer->TotalReceiveLength);

															OS_RtlCopyMemory(pSessionManager->pOutputBuffer, pServerElement->pSession->pReceiveBuffer->pReceiveBuffer , pServerElement->pSession->pReceiveBuffer->TotalReceiveLength);
															pSessionManager->OutputBufferLenget = pServerElement->pSession->pReceiveBuffer->TotalReceiveLength;
														}// try
														finally
														{
															if(!NT_SUCCESS(status))
															{
																pSessionManager->ReceivedProtocolHeader.msgtype = FTDRECEIVEERROR;
															}
															KeSetEvent(pSyncEvent,0,FALSE);
														}
													}// if(pSyncEvent != NULL)
													else if(pSessionManager->bSendHandshakeInformation == FALSE)
													{

														if( PROTO_IsAckCheckRequired( ( ftd_header_t* ) pServerElement->pSession->pReceiveHeader->pReceiveBuffer ) == TRUE )
														{
															// Removing the Packet from the Queue With Payload
															status = QM_RemovePktsFromMigrateQueue(pSessionManager->pLogicalGroupPtr,
																				pServerElement->pSession->pReceiveHeader->pReceiveBuffer,
																				pServerElement->pSession->pReceiveBuffer->pReceiveBuffer,
																				pServerElement->pSession->pReceiveBuffer->TotalReceiveLength);
															if (!NT_SUCCESS(status))
															{ // nothing to do. ignore error
																DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): QM_RemovePktsFromMigrateQueue() with Payload failed with error %lx Terminating and Resetting Connection\n",status));
																// We need to reset connection and terminate this thread
																COM_ResetAllConnections(pSessionManager);
																bExit = TRUE;
																leave;
															}
														}
														else
														{
															// Received a Packet that is not an ACK so we might need to process that
															// Like a NOOP or some ASynchronour Protocol Command that needs to be handled
														}
													} // else if(pSessionManager->bSendHandshakeInformation == FALSE)
												}// if(LG_IS_PRIMARY_MODE(pSessionManager->pLogicalGroupPtr))
												else
												{ // The Current Role is SECONDARY so do the Secondary Stuff
													PMM_HOLDER pMM_Buffer = NULL;
													// Put this Packet into the Receive Queue. 
													// There is no Data Buffer Only the Protocol Header So Size = 0.
													pMM_Buffer = mm_alloc_buffer( pSessionManager->pLogicalGroupPtr, pServerElement->pSession->pReceiveBuffer->TotalReceiveLength, PROTO_HDR, TRUE); 
													if(pMM_Buffer == NULL)
													{
														DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): mm_alloc_buffer() failed to allocate the Memory for MM_HOLDER with MM_PROTO_HDR for Size Zero for LG %d\n",pSessionManager->pLogicalGroupPtr->LGroupNumber));
														// TODO Do the Error Recovery Seems like we are out of Resources so should we wait or reset the connections and exit.
														COM_ResetAllConnections(pSessionManager);
														bExit = TRUE;
														leave;
													}
													OS_ASSERT(pMM_Buffer->pProtocolHdr != NULL);
													// Copy the Header 
													OS_RtlCopyMemory(pMM_Buffer->pProtocolHdr, pServerElement->pSession->pReceiveHeader->pReceiveBuffer , PROTOCOL_HEADER_SIZE);
													// Copy the Received Data.
													MM_COPY_BUFFER(pMM_Buffer, pServerElement->pSession->pReceiveBuffer->pReceiveBuffer , pServerElement->pSession->pReceiveBuffer->TotalReceiveLength, TRUE);
													// Insert the MM_HOLDER to the TReceiveQueue
													QM_Insert(pSessionManager->pLogicalGroupPtr,pMM_Buffer, TRECEIVE_QUEUE, TRUE);
													// This Data will be available for the Target Write Thread.
												}



												// Increment the Receive Performance Counter
												pSftkLg = pSessionManager->pLogicalGroupPtr;

												if(pSftkLg != NULL)
												{
													KeAcquireSpinLock(&pSessionManager->StatisticsLock , &oldIrql);

													pServerElement->SessionPerformanceInformation.nPacketsReceived++;
													pServerElement->SessionPerformanceInformation.nBytesReceived += pServerElement->pSession->pReceiveHeader->TotalReceiveLength;

													if(pServerElement->SessionPerformanceInformation.nMinimumReceivedPacketSize >  pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
													{
														pServerElement->SessionPerformanceInformation.nMinimumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
													}

													if(pServerElement->SessionPerformanceInformation.nMaximumReceivedPacketSize <  pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
													{
														pServerElement->SessionPerformanceInformation.nMaximumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
													}
													pServerElement->SessionPerformanceInformation.nAverageReceivedPacketSize = (ULONG)(pServerElement->SessionPerformanceInformation.nBytesReceived / pServerElement->SessionPerformanceInformation.nPacketsReceived);

													pSftkLg->Statistics.SM_PacketsReceived++;
													pSftkLg->Statistics.SM_BytesReceived += pServerElement->pSession->pReceiveHeader->TotalReceiveLength;

													if(pSftkLg->Statistics.SM_MinimumReceivedPacketSize > pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
													{
														pSftkLg->Statistics.SM_MinimumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
													}

													if(pSftkLg->Statistics.SM_MaximumReceivedPacketSize < pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
													{
														pSftkLg->Statistics.SM_MaximumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
													}
													pSftkLg->Statistics.SM_AverageReceivedPacketSize = (ULONG)(pSftkLg->Statistics.SM_BytesReceived / pSftkLg->Statistics.SM_PacketsReceived);

													KeReleaseSpinLock(&pSessionManager->StatisticsLock , oldIrql);
												}

												// Release the Spin Lock after the increment operation

												pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
											}
											else
											{
												swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
												swprintf(wchar2,L"0X%08X",pServerElement);
												sftk_LogEventString2(GSftk_Config.DriverObject,MSG_REPL_DRIVER_PROTOCOL_INVALID_RECEIVE_PACKET_ERROR,status,0,wchar1,wchar2);

												// Got an error while Processing the returned ACK
												// Cannot do much just leave the thread
												DebugPrint((DBG_RECV, "COM_ReceiveThread2(): The received data is not recognized for Session %lx so exiting \n", pServerElement->pSession));
												bExit = TRUE;
												leave;
											}
										}
									}//if(pServerElement->pSession->pReceiveBuffer->pReceiveMdl == NULL)
									else
									{
										iWait.QuadPart = -(10*10*1000);
										KeDelayExecutionThread(KernelMode,FALSE,&iWait);
									}
								}//This is pServerElement->pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_DATA
							}
							else
							{
								if(pSessionManager->nLiveSessions == 0)
								{
									swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
									swprintf(wchar2,L"0X%08X",pSessionManager);
									sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_NO_SESSION_IS_ALIVE,STATUS_SUCCESS,0,wchar1,wchar2);

									DebugPrint((DBG_RECV, "COM_ReceiveThread2(): There are no live sessions so exiting\n"));
									bExit = TRUE;
									break;
								}
								//30 Milli-Seconds in 100*Nano Seconds
								iWait.QuadPart = -(10*10*1000);
								KeDelayExecutionThread(KernelMode,FALSE,&iWait);
							}
						}
					}
					if(!bExit)
					{
						OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

						status = KeWaitForSingleObject(&pSessionManager->IOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
						if(status == STATUS_SUCCESS)
						{
							swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
							swprintf(wchar2,L"0X%08X",pSessionManager);
							sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXITING,status,0,wchar1,wchar2);

							DebugPrint((DBG_RECV, "COM_ReceiveThread2(): The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}
					}
				}//try
				finally
				{
					//Release the Receive Lock
//					ExReleaseFastMutexUnsafe(&pSessionManager->ReceiveLock);
					//Release the Resource 
					ExReleaseResourceLite(&pSessionManager->ServerListLock);
					KeLeaveCriticalRegion();	//Enable the Kernel APC's
				}
			}//while(!bExit)
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): An Exception Occured in COM_ReceiveThread2() Error Code is %lx\n",GetExceptionCode()));
			status = GetExceptionCode();
		}
	}
	finally
	{
		DebugPrint((DBG_RECV, "COM_ReceiveThread2(): Exiting the COM_ReceiveThread2() status = %lx\n",status));
	}

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	swprintf(wchar3,L"%S",L"COM_ReceiveThread2");
	sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_STOPPED,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);

	return PsTerminateSystemThread(status);
}


NTSTATUS 
COM_ReceiveThreadForServer(
			PSERVER_ELEMENT pServerElement
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	ULONG nDataLength = 0;
	PSFTK_LG pSftkLg = NULL;
	KIRQL oldIrql;
	PKEVENT pSyncEvent = NULL;
	LARGE_INTEGER iCurrentTime;
	ULONG uCurTime = 0;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	DebugPrint((DBG_RECV, "COM_ReceiveThreadForServer(): Enter COM_ReceiveThreadForServer()\n"));

	OS_ASSERT(pServerElement != NULL);
	OS_ASSERT(pServerElement->pSessionManager != NULL);
	OS_ASSERT(pServerElement->pSessionManager->pLogicalGroupPtr != NULL);

	if(pServerElement->pSessionManager->LastPacketReceiveTime == 0)
	{
		// Initialize the Last Packet Received Time
		KeQuerySystemTime(&iCurrentTime);
		uCurTime = (ULONG)(iCurrentTime.QuadPart/(10*1000*1000));
		pServerElement->pSessionManager->LastPacketReceiveTime = uCurTime;
	}

	swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pServerElement->pSessionManager);
	swprintf(wchar3,L"%S",L"COM_ReceiveThreadForServer");
	sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_STARTED,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);

	try
	{
		try
		{

//			iZeroWait.QuadPart = -(10*10*1000);
			iZeroWait.QuadPart = 0;
			while(!bExit)
			{
				try
				{
					//Acquire the resource as shared Lock
					//This Resource will not be acquired if there is a call for Exclusive Lock

					KeEnterCriticalRegion();	//Disable the Kernel APC's
					ExAcquireResourceSharedLite(&pServerElement->pSessionManager->ServerListLock,TRUE);

					//We already disabled the APC's so use the UnSafe method to acquire the
					//Receive Lock
//					ExAcquireFastMutexUnsafe(&pServerElement->SessionReceiveLock);


					//30 Milli-Seconds in 100*Nano Seconds
					iWait.QuadPart = -(10*10*1000);

					if(pServerElement->pSession != NULL && !bExit)
					{
						OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

						status = KeWaitForSingleObject(&pServerElement->SessionIOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
						if(status == STATUS_SUCCESS)
						{
							swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
							swprintf(wchar2,L"0X%08X",pServerElement->pSessionManager);
							sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXITING,status,0,wchar1,wchar2);

							DebugPrint((DBG_RECV, "COM_ReceiveThreadForServer(): The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}

						if(pServerElement->pSession->bSessionEstablished == SFTK_CONNECTED) 
						{
							if(pServerElement->pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_NOTINIT)
							{
								pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_HEADER;

								DebugPrint((DBG_RECV, "COM_ReceiveThreadForServer(): The Session that is used to receive Data is %lx\n",pServerElement->pSession));

								pServerElement->pSession->pReceiveHeader->TotalReceiveLength = PROTOCOL_HEADER_SIZE;

								if(pServerElement->pSession->pReceiveHeader->pReceiveMdl == NULL)
								{
									pServerElement->pSession->pReceiveHeader->pReceiveMdl = TDI_AllocateAndProbeMdl(pServerElement->pSession->pReceiveHeader->pReceiveBuffer , pServerElement->pSession->pReceiveHeader->TotalReceiveLength ,FALSE,FALSE,NULL);
								}

								OS_ASSERT(pServerElement->pSession->pReceiveHeader->pReceiveMdl);

								(pServerElement->pSession->pReceiveHeader->pReceiveMdl)->Next = NULL;   // IMPORTANT!!!


								pServerElement->pSession->pReceiveHeader->ActualReceiveLength = 0;

								try
								{
									//Lets try Without Reuse IRP
//									if(pServerElement->pSession->pReceiveHeader->pReceiveIrp != NULL)
//									{
//										IoReuseIrp(pServerElement->pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
//									}
									status = TDI_ReceiveOnEndpoint1(
												&pServerElement->pSession->KSEndpoint,
												NULL,       // User Completion Event
												TDI_EndpointReceiveRequestComplete2,// User Completion Routine
												pServerElement->pSession->pReceiveHeader,   // User Completion Context
												&pServerElement->pSession->pReceiveHeader->IoReceiveStatus,
												pServerElement->pSession->pReceiveHeader->pReceiveMdl,
												0,           // Flags
												&pServerElement->pSession->pReceiveHeader->pReceiveIrp
												);
										if(status == STATUS_INSUFFICIENT_RESOURCES && pServerElement->pSession->pReceiveHeader->pReceiveIrp == NULL)
										{
											swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
											swprintf(wchar2,L"0X%08X",pServerElement);
											swprintf(wchar3,L"0X%08X",status);
											sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,status,0,wchar1,wchar2,wchar3);

											DebugPrint((DBG_ERROR, "COM_ReceiveThreadForServer(): An Exception Occured in COM_ReceiveThreadForServer() Error Code is %lx\n",status));
											// Failed to Allocate IRP hence just set the Buffer to Free 
											TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveHeader->pReceiveMdl);	//Free the MDL
											pServerElement->pSession->pReceiveHeader->pReceiveMdl = NULL;
											pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
										}

								}
								except(EXCEPTION_EXECUTE_HANDLER)
								{
									swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
									swprintf(wchar2,L"0X%08X",pServerElement);
									swprintf(wchar3,L"0X%08X",GetExceptionCode());
									sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,GetExceptionCode(),0,wchar1,wchar2,wchar3);

									DebugPrint((DBG_ERROR, "COM_ReceiveThreadForServer(): An Exception Occured in COM_ReceiveThreadForServer() Error Code is %lx\n",GetExceptionCode()));
									status = GetExceptionCode();

									TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveHeader->pReceiveMdl);	//Free the MDL
									pServerElement->pSession->pReceiveHeader->pReceiveMdl = NULL;
									pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
								}
							}
							else if(pServerElement->pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_HEADER)
							{
								//This will assure that the Previous CompletionRoutine completed
								//Successfully, before calling the IoCallDriver() again
								//We can use an Event here, but it does the samething
								if(pServerElement->pSession->pReceiveHeader->pReceiveMdl == NULL)
								{
									if(pServerElement->pSession->pReceiveHeader->ActualReceiveLength < pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
									{
										//We still Need to read somemore buffer
										pServerElement->pSession->pReceiveHeader->pReceiveMdl = TDI_AllocateAndProbeMdl(pServerElement->pSession->pReceiveHeader->pReceiveBuffer + pServerElement->pSession->pReceiveHeader->ActualReceiveLength,pServerElement->pSession->pReceiveHeader->TotalReceiveLength - pServerElement->pSession->pReceiveHeader->ActualReceiveLength,FALSE,FALSE,NULL);
										//Call the TDI_RECEIVE with the new MDL
										try
										{
											//Lets try Without Reuse IRP
//											if(pServerElement->pSession->pReceiveHeader->pReceiveIrp != NULL)
//											{
//												IoReuseIrp(pServerElement->pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
//											}
											status = TDI_ReceiveOnEndpoint1(
														&pServerElement->pSession->KSEndpoint,
														NULL,       // User Completion Event
														TDI_EndpointReceiveRequestComplete2,// User Completion Routine
														pServerElement->pSession->pReceiveHeader,   // User Completion Context
														&pServerElement->pSession->pReceiveHeader->IoReceiveStatus,
														pServerElement->pSession->pReceiveHeader->pReceiveMdl,
														0,           // Flags
														&pServerElement->pSession->pReceiveHeader->pReceiveIrp
														);
											if(status == STATUS_INSUFFICIENT_RESOURCES && pServerElement->pSession->pReceiveHeader->pReceiveIrp == NULL)
											{
												swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
												swprintf(wchar2,L"0X%08X",pServerElement);
												swprintf(wchar3,L"0X%08X",status);
												sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,status,0,wchar1,wchar2,wchar3);

												DebugPrint((DBG_ERROR, "COM_ReceiveThreadForServer(): An Exception Occured in COM_ReceiveThreadForServer() Error Code is %lx\n",status));
												// Failed to Allocate IRP hence just set the Buffer to Free 
												TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveHeader->pReceiveMdl);	//Free the MDL
												pServerElement->pSession->pReceiveHeader->pReceiveMdl = NULL;
												pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
											}
										}
										except(EXCEPTION_EXECUTE_HANDLER)
										{
											swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
											swprintf(wchar2,L"0X%08X",pServerElement);
											swprintf(wchar3,L"0X%08X",GetExceptionCode());
											sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,GetExceptionCode(),0,wchar1,wchar2,wchar3);

											DebugPrint((DBG_ERROR, "COM_ReceiveThreadForServer(): An Exception Occured in COM_ReceiveThreadForServer() Error Code is %lx\n",GetExceptionCode()));
											status = GetExceptionCode();

											TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveHeader->pReceiveMdl);	//Free the MDL
											pServerElement->pSession->pReceiveHeader->pReceiveMdl = NULL;
											pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
										}


									}
									else
									{
										nDataLength = 0;
										status = PROTO_ProcessReceiveHeader(((ftd_header_t*)pServerElement->pSession->pReceiveHeader->pReceiveBuffer),&nDataLength,pServerElement->pSession);
										//Received the Total header so process it and read the Data if 
										//there is any

										if( status == STATUS_SUCCESS || status == STATUS_MORE_PROCESSING_REQUIRED)
										{
											if(nDataLength > 0)
											{
												OS_ASSERT(nDataLength <= pServerElement->pSession->pReceiveBuffer->ActualReceiveWindow);
												status = COM_GetSessionReceiveBuffer(pServerElement->pSession);

												if(!NT_SUCCESS(status))
												{
													swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
													swprintf(wchar2,L"0X%08X",pServerElement->pSessionManager);
													swprintf(wchar3,L"0X%08X",pServerElement->pSession->pReceiveBuffer->ActualReceiveWindow);
													sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_BUFFER_NOT_AVAILABLE,status,0,wchar1,wchar2,wchar3);

													//There is no Free Buffer available so returning
													DebugPrint((DBG_RECV, "COM_ReceiveThreadForServer(): Unable to get the Free Buffer to Receive Data for Session %lx\n",pServerElement->pSession));
													leave;
													// return STATUS_MORE_PROCESSING_REQUIRED;
												}

												OS_ASSERT(pServerElement->pSession->pReceiveBuffer);
												OS_ASSERT(pServerElement->pSession->pReceiveBuffer->pReceiveBuffer);

												//This is the new Buffer to call TDI_RECEIVE
												pServerElement->pSession->pReceiveBuffer->state = SFTK_BUFFER_INUSE;
												pServerElement->pSession->pReceiveBuffer->ActualReceiveLength = 0;
												pServerElement->pSession->pReceiveBuffer->TotalReceiveLength = nDataLength;
												pServerElement->pSession->pReceiveBuffer->pReceiveMdl = TDI_AllocateAndProbeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveBuffer,pServerElement->pSession->pReceiveBuffer->TotalReceiveLength ,FALSE,FALSE,NULL);
												pServerElement->pSession->pReceiveBuffer->pReceiveMdl->Next = NULL;
												pServerElement->pSession->pReceiveBuffer->pSession = pServerElement->pSession;

												OS_ASSERT(pServerElement->pSession->pReceiveBuffer->pReceiveMdl);

												// There is still more data to receive so set the Status to SFTK_PROCESSING_RECEIVE_DATA.
												pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_DATA;


												//Lets try Without Reuse IRP
//												if(pServerElement->pSession->pReceiveHeader->pReceiveIrp != NULL)
//												{
//													IoReuseIrp(pServerElement->pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
//												}
												try
												{
													status = TDI_ReceiveOnEndpoint1(
																&pServerElement->pSession->KSEndpoint,
																NULL,       // User Completion Event
																TDI_EndpointReceiveRequestComplete2,// User Completion Routine
																pServerElement->pSession->pReceiveBuffer,   // User Completion Context
																&pServerElement->pSession->pReceiveBuffer->IoReceiveStatus,
																pServerElement->pSession->pReceiveBuffer->pReceiveMdl,
																0,           // Flags
																&pServerElement->pSession->pReceiveHeader->pReceiveIrp
																);
													if(status == STATUS_INSUFFICIENT_RESOURCES && pServerElement->pSession->pReceiveHeader->pReceiveIrp == NULL)
													{
														swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
														swprintf(wchar2,L"0X%08X",pServerElement);
														swprintf(wchar3,L"0X%08X",status);
														sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,status,0,wchar1,wchar2,wchar3);

														DebugPrint((DBG_ERROR, "COM_ReceiveThreadForServer(): An Exception Occured in COM_ReceiveThreadForServer() Error Code is %lx\n",status));
														// Failed to Allocate IRP hence just set the Buffer to Free 
														TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveMdl);	//Free the MDL
														pServerElement->pSession->pReceiveBuffer->pReceiveMdl = NULL;
														pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
													}
												}
												except(EXCEPTION_EXECUTE_HANDLER)
												{
													DebugPrint((DBG_ERROR, "COM_ReceiveThreadForServer(): An Exception Occured in COM_ReceiveThreadForServer() Error Code is %lx\n",GetExceptionCode()));
													status = GetExceptionCode();

													TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveMdl);	//Free the MDL
													pServerElement->pSession->pReceiveBuffer->pReceiveMdl = NULL;
													pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
												}
											}
											else
											{
												/*
												swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
												swprintf(wchar2,L"0X%08X",pServerElement);
												swprintf(wchar3,L"%d",((ftd_header_t*)pServerElement->pSession->pReceiveHeader->pReceiveBuffer)->msgtype);
												swprintf(wchar4,L"%d",pServerElement->pSession->pReceiveHeader->TotalReceiveLength);
												sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_REPL_DRIVER_PROTOCOL_RECEIVED_PACKET,STATUS_SUCCESS,0,wchar1,wchar2,wchar3,wchar4);
												*/

												//Data is Read completely
												//So can release the Buffer and set the Session Receive status to unused
												DebugPrint((DBG_RECV, "COM_ReceiveThreadForServer(): Completed Reading the Whole Header for Session %lx and Length %ld and No PayLoad\n", pServerElement->pSession, pServerElement->pSession->pReceiveHeader->TotalReceiveLength));

												pSyncEvent = InterlockedExchangePointer(&pServerElement->pSessionManager->pSyncReceivedEvent, NULL);

												if(pSyncEvent != NULL)
												{
													OS_RtlCopyMemory(&pServerElement->pSessionManager->ReceivedProtocolHeader, pServerElement->pSession->pReceiveHeader->pReceiveBuffer , PROTOCOL_HEADER_SIZE);
													pServerElement->pSessionManager->pOutputBuffer = NULL;
													pServerElement->pSessionManager->OutputBufferLenget = 0;
													KeSetEvent(pSyncEvent,0,FALSE);
												}// if
												else if(pServerElement->pSessionManager->bSendHandshakeInformation == FALSE)
												{
													if( PROTO_IsAckCheckRequired( ( ftd_header_t* ) pServerElement->pSession->pReceiveHeader->pReceiveBuffer ) == TRUE )
													{
														// Removing the Packet from the Queue Without Payload
														status = QM_RemovePktsFromMigrateQueue(pServerElement->pSessionManager->pLogicalGroupPtr,
																			pServerElement->pSession->pReceiveHeader->pReceiveBuffer,
																			NULL,
																			0);
														if (!NT_SUCCESS(status))
														{ // nothing to do. ignore error
															DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): QM_RemovePktsFromMigrateQueue() failed with error %lx Terminating and Resetting Connection\n",status));
															// We need to reset connection and terminate this thread
															COM_ResetAllConnections(pServerElement->pSessionManager);
															bExit = TRUE;
															leave;
														}
													}
												}


												// Increment the Receive Performance Counter
												pSftkLg = pServerElement->pSessionManager->pLogicalGroupPtr;

												if(pSftkLg != NULL)
												{
													KeAcquireSpinLock(&pServerElement->pSessionManager->StatisticsLock , &oldIrql);

													pServerElement->SessionPerformanceInformation.nPacketsReceived++;
													pServerElement->SessionPerformanceInformation.nBytesReceived += pServerElement->pSession->pReceiveHeader->TotalReceiveLength;

													if(pServerElement->SessionPerformanceInformation.nMinimumReceivedPacketSize >  pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
													{
														pServerElement->SessionPerformanceInformation.nMinimumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
													}

													if(pServerElement->SessionPerformanceInformation.nMaximumReceivedPacketSize <  pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
													{
														pServerElement->SessionPerformanceInformation.nMaximumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
													}
													pServerElement->SessionPerformanceInformation.nAverageReceivedPacketSize = (ULONG)(pServerElement->SessionPerformanceInformation.nBytesReceived / pServerElement->SessionPerformanceInformation.nPacketsReceived);

													pSftkLg->Statistics.SM_PacketsReceived++;
													pSftkLg->Statistics.SM_BytesReceived += pServerElement->pSession->pReceiveHeader->TotalReceiveLength;

													if(pSftkLg->Statistics.SM_MinimumReceivedPacketSize > pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
													{
														pSftkLg->Statistics.SM_MinimumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
													}

													if(pSftkLg->Statistics.SM_MaximumReceivedPacketSize < pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
													{
														pSftkLg->Statistics.SM_MaximumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
													}
													pSftkLg->Statistics.SM_AverageReceivedPacketSize = (ULONG)(pSftkLg->Statistics.SM_BytesReceived / pSftkLg->Statistics.SM_PacketsReceived);

													KeReleaseSpinLock(&pServerElement->pSessionManager->StatisticsLock , oldIrql);
												}
												// Release the Spin Lock after the increment operation

												pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
											}//else
										}//if( status == STATUS_SUCCESS || status == STATUS_MORE_PROCESSING_REQUIRED)
										else
										{
											swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
											swprintf(wchar2,L"0X%08X",pServerElement);
											sftk_LogEventString2(GSftk_Config.DriverObject,MSG_REPL_DRIVER_PROTOCOL_INVALID_RECEIVE_PACKET_ERROR,status,0,wchar1,wchar2);

											// Got an error while Processing the returned ACK
											// Cannot do much just leave the thread
											DebugPrint((DBG_RECV, "COM_ReceiveThread2(): The received data is not recognized for Session %lx so exiting \n", pServerElement->pSession));
											bExit = TRUE;
											leave;
										}
									}//else
								}//if(pSession->pReceiveHeader->pReceiveMdl == NULL)
								else
								{
									iWait.QuadPart = -(10*10*1000);
									KeDelayExecutionThread(KernelMode,FALSE,&iWait);
								}
							}//else if(pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_HEADER)
							else	//This is pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_DATA
							{
								//This will assure that the Previous CompletionRoutine completed
								//Successfully, before calling the IoCallDriver() again
								//We can use an Event here, but it does the samething
								OS_ASSERT(pServerElement->pSession->pReceiveBuffer);
								if(pServerElement->pSession->pReceiveBuffer->pReceiveMdl == NULL)
								{
									if(pServerElement->pSession->pReceiveBuffer->ActualReceiveLength < pServerElement->pSession->pReceiveBuffer->TotalReceiveLength)
									{
										//Still Need to read sommore data
										//We still Need to read somemore buffer
										pServerElement->pSession->pReceiveBuffer->pReceiveMdl = TDI_AllocateAndProbeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveBuffer + pServerElement->pSession->pReceiveBuffer->ActualReceiveLength,pServerElement->pSession->pReceiveBuffer->TotalReceiveLength - pServerElement->pSession->pReceiveBuffer->ActualReceiveLength,FALSE,FALSE,NULL);
										//Call the TDI_RECEIVE with the new MDL
										try
										{
											//Lets try Without Reuse IRP
//											if(pServerElement->pSession->pReceiveHeader->pReceiveIrp != NULL)
//											{
//												IoReuseIrp(pServerElement->pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
//											}
											status = TDI_ReceiveOnEndpoint1(
														&pServerElement->pSession->KSEndpoint,
														NULL,       // User Completion Event
														TDI_EndpointReceiveRequestComplete2,// User Completion Routine
														pServerElement->pSession->pReceiveBuffer,   // User Completion Context
														&pServerElement->pSession->pReceiveBuffer->IoReceiveStatus,
														pServerElement->pSession->pReceiveBuffer->pReceiveMdl,
														0,           // Flags
														&pServerElement->pSession->pReceiveHeader->pReceiveIrp
														);
												if(status == STATUS_INSUFFICIENT_RESOURCES && pServerElement->pSession->pReceiveHeader->pReceiveIrp == NULL)
												{
													swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
													swprintf(wchar2,L"0X%08X",pServerElement);
													swprintf(wchar3,L"0X%08X",status);
													sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,status,0,wchar1,wchar2,wchar3);

													DebugPrint((DBG_ERROR, "COM_ReceiveThreadForServer(): An Exception Occured in COM_ReceiveThreadForServer() Error Code is %lx\n",status));
													// Failed to Allocate IRP hence just set the Buffer to Free 
													TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveMdl);	//Free the MDL
													pServerElement->pSession->pReceiveBuffer->pReceiveMdl = NULL;
													pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
												}
										}
										except(EXCEPTION_EXECUTE_HANDLER)
										{
											swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
											swprintf(wchar2,L"0X%08X",pServerElement);
											swprintf(wchar3,L"0X%08X",GetExceptionCode());
											sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXCEPTION,GetExceptionCode(),0,wchar1,wchar2,wchar3);

											DebugPrint((DBG_ERROR, "COM_ReceiveThreadForServer(): An Exception Occured in COM_ReceiveThreadForServer() Error Code is %lx\n",GetExceptionCode()));
											status = GetExceptionCode();

											TDI_UnlockAndFreeMdl(pServerElement->pSession->pReceiveBuffer->pReceiveMdl);	//Free the MDL
											pServerElement->pSession->pReceiveBuffer->pReceiveMdl = NULL;
											pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
										}

									}
									else
									{
										//Data is Read completely
										//So can release the Buffer and set the Session Receive status to unused

										nDataLength = 0;
										status = PROTO_ProcessReceiveData(((ftd_header_t*)pServerElement->pSession->pReceiveHeader->pReceiveBuffer),pServerElement->pSession->pReceiveBuffer->pReceiveBuffer,pServerElement->pSession->pReceiveBuffer->TotalReceiveLength,&nDataLength,pServerElement->pSession);

										if(status == STATUS_MORE_PROCESSING_REQUIRED)
										{
											//Still More Data to be Read so Increase the TotalSize 
											//and try again
											pServerElement->pSession->pReceiveBuffer->TotalReceiveLength += nDataLength;

											OS_ASSERT(pServerElement->pSession->pReceiveBuffer->TotalReceiveLength <= pServerElement->pSession->pReceiveBuffer->ReceiveWindow);
										}
										else if(status == STATUS_SUCCESS)
										{
											/*
											swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
											swprintf(wchar2,L"0X%08X",pServerElement);
											swprintf(wchar3,L"%d",((ftd_header_t*)pServerElement->pSession->pReceiveHeader->pReceiveBuffer)->msgtype);
											swprintf(wchar4,L"%d",pServerElement->pSession->pReceiveBuffer->TotalReceiveLength);
											sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_REPL_DRIVER_PROTOCOL_RECEIVED_PACKET_WITH_PAYLOAD,STATUS_SUCCESS,0,wchar1,wchar2,wchar3,wchar4);
											*/

											//All the Data is Read, so fine
											DebugPrint((DBG_RECV, "COM_ReceiveThreadForServer(): Completed Reading the Whole Data for Session %lx and Length %ld\n", pServerElement->pSession, pServerElement->pSession->pReceiveBuffer->TotalReceiveLength));

											pSyncEvent = InterlockedExchangePointer(&pServerElement->pSessionManager->pSyncReceivedEvent, NULL);

											if(pSyncEvent != NULL)
											{
												try
												{
													OS_RtlCopyMemory(&pServerElement->pSessionManager->ReceivedProtocolHeader, pServerElement->pSession->pReceiveHeader->pReceiveBuffer , PROTOCOL_HEADER_SIZE);

													status = NdisAllocateMemoryWithTag(
																					&pServerElement->pSessionManager->pOutputBuffer,
																					pServerElement->pSession->pReceiveBuffer->TotalReceiveLength,
																					MEM_TAG
																					);

													if(!NT_SUCCESS(status))
													{
														DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): Unable to Accocate Memory for Received Data status = %lx\n",status));
														leave;
													}

													NdisZeroMemory(pServerElement->pSessionManager->pOutputBuffer,pServerElement->pSession->pReceiveBuffer->TotalReceiveLength);

													OS_RtlCopyMemory(pServerElement->pSessionManager->pOutputBuffer, pServerElement->pSession->pReceiveBuffer->pReceiveBuffer , pServerElement->pSession->pReceiveBuffer->TotalReceiveLength);
													pServerElement->pSessionManager->OutputBufferLenget = pServerElement->pSession->pReceiveBuffer->TotalReceiveLength;
												}// try
												finally
												{
													if(!NT_SUCCESS(status))
													{
														pServerElement->pSessionManager->ReceivedProtocolHeader.msgtype = FTDRECEIVEERROR;
													}
													KeSetEvent(pSyncEvent,0,FALSE);
												}// finally
											}// if
											else if(pServerElement->pSessionManager->bSendHandshakeInformation == FALSE)
											{
												if( PROTO_IsAckCheckRequired( ( ftd_header_t* ) pServerElement->pSession->pReceiveHeader->pReceiveBuffer ) == TRUE )
												{
													// Removing the Packet from the Queue With Payload
													status = QM_RemovePktsFromMigrateQueue(pServerElement->pSessionManager->pLogicalGroupPtr,
																		pServerElement->pSession->pReceiveHeader->pReceiveBuffer,
																		pServerElement->pSession->pReceiveBuffer->pReceiveBuffer,
																		pServerElement->pSession->pReceiveBuffer->TotalReceiveLength);
													if (!NT_SUCCESS(status))
													{ // nothing to do. ignore error
														DebugPrint((DBG_ERROR, "COM_ReceiveThread2(): QM_RemovePktsFromMigrateQueue() with Payload failed with error %lx Terminating and Resetting Connection\n",status));
														// We need to reset connection and terminate this thread
														COM_ResetAllConnections(pServerElement->pSessionManager);
														bExit = TRUE;
														leave;
													}
												}
											}


											// Increment the Receive Performance Counter
											pSftkLg = pServerElement->pSessionManager->pLogicalGroupPtr;

											if(pSftkLg != NULL)
											{
												KeAcquireSpinLock(&pServerElement->pSessionManager->StatisticsLock , &oldIrql);

												pServerElement->SessionPerformanceInformation.nPacketsReceived++;
												pServerElement->SessionPerformanceInformation.nBytesReceived += pServerElement->pSession->pReceiveHeader->TotalReceiveLength;

												if(pServerElement->SessionPerformanceInformation.nMinimumReceivedPacketSize >  pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
												{
													pServerElement->SessionPerformanceInformation.nMinimumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
												}

												if(pServerElement->SessionPerformanceInformation.nMaximumReceivedPacketSize <  pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
												{
													pServerElement->SessionPerformanceInformation.nMaximumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
												}
												pServerElement->SessionPerformanceInformation.nAverageReceivedPacketSize = (ULONG)(pServerElement->SessionPerformanceInformation.nBytesReceived / pServerElement->SessionPerformanceInformation.nPacketsReceived);

												pSftkLg->Statistics.SM_PacketsReceived++;
												pSftkLg->Statistics.SM_BytesReceived += pServerElement->pSession->pReceiveHeader->TotalReceiveLength;

												if(pSftkLg->Statistics.SM_MinimumReceivedPacketSize > pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
												{
													pSftkLg->Statistics.SM_MinimumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
												}

												if(pSftkLg->Statistics.SM_MaximumReceivedPacketSize < pServerElement->pSession->pReceiveHeader->TotalReceiveLength)
												{
													pSftkLg->Statistics.SM_MaximumReceivedPacketSize = pServerElement->pSession->pReceiveHeader->TotalReceiveLength;
												}
												pSftkLg->Statistics.SM_AverageReceivedPacketSize = (ULONG)(pSftkLg->Statistics.SM_BytesReceived / pSftkLg->Statistics.SM_PacketsReceived);

												KeReleaseSpinLock(&pServerElement->pSessionManager->StatisticsLock , oldIrql);
											}
											// Release the Spin Lock after the increment operation

											pServerElement->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
										}// else
										else
										{
											swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
											swprintf(wchar2,L"0X%08X",pServerElement);
											sftk_LogEventString2(GSftk_Config.DriverObject,MSG_REPL_DRIVER_PROTOCOL_INVALID_RECEIVE_PACKET_ERROR,status,0,wchar1,wchar2);

											// Got an error while Processing the returned ACK
											// Cannot do much just leave the thread
											DebugPrint((DBG_RECV, "COM_ReceiveThread2(): The received data is not recognized for Session %lx so exiting \n", pServerElement->pSession));
											bExit = TRUE;
											leave;
										}
									}// else
								}//if(pSession->pReceiveBuffer->pReceiveMdl == NULL)
								else
								{
									iWait.QuadPart = -(10*10*1000);
									KeDelayExecutionThread(KernelMode,FALSE,&iWait);
								}
							}//This is pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_DATA
						}
						else
						{
							DebugPrint((DBG_RECV, "COM_ReceiveThreadForServer(): There Server %lx is not Connected so exiting\n",pServerElement));
							bExit = TRUE;
							break;
						}
					}
					if(!bExit)
					{
						OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

						status = KeWaitForSingleObject(&pServerElement->SessionIOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
						if(status == STATUS_SUCCESS)
						{
							swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
							swprintf(wchar2,L"0X%08X",pServerElement->pSessionManager);
							sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_RECEIVE_THREAD_EXITING,status,0,wchar1,wchar2);

							DebugPrint((DBG_RECV, "COM_ReceiveThreadForServer(): The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}
					}
				}//try
				finally
				{
					//Release the Receive Lock
//					ExReleaseFastMutexUnsafe(&pServerElement->SessionReceiveLock);
					//Release the Resource 
					ExReleaseResourceLite(&pServerElement->pSessionManager->ServerListLock);
					KeLeaveCriticalRegion();	//Enable the Kernel APC's
				}
			}//while(!bExit)
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			DebugPrint((DBG_ERROR, "COM_ReceiveThreadForServer(): An Exception Occured in COM_ReceiveThreadForServer() Error Code is %lx\n",GetExceptionCode()));
			status = GetExceptionCode();
		}
	}
	finally
	{
		DebugPrint((DBG_RECV, "COM_ReceiveThreadForServer(): Exiting the COM_ReceiveThreadForServer() status = %lx\n",status));
	}

	return PsTerminateSystemThread(status);
}

#endif	//__NEW_RECEIVE__


/*********************************

#if 0 // This is _OBSOLETE_

NTSTATUS
TDI_EndpointReceiveRequestComplete(
								IN PDEVICE_OBJECT pDeviceObject,
								IN PIRP pIrp,
								IN PVOID Context
								)
{
	PRECEIVE_BUFFER pReceiveBuffer= NULL;
	PRECEIVE_BUFFER pNewReceiveBuffer = NULL;
	PTCP_SESSION pSession = NULL;
	NTSTATUS status;
	ULONG Length;
	ULONG Len=0;
	
	status = pIrp->IoStatus.Status;
	Length = pIrp->IoStatus.Information;
	pReceiveBuffer = (PRECEIVE_BUFFER)Context;

	pSession = pReceiveBuffer->pSession;	//Get the Session Pointer

	if(NT_SUCCESS(status))
	{
		KdPrint(( "TDI_EndpointReceiveRequestComplete: Receive %d Bytes and Index = %d for ReceiveBuffer = %lx\n", Length, pReceiveBuffer->index , pReceiveBuffer));

		pReceiveBuffer->ActualReceiveLength += Length;  //Increment the Actual Length

		TDI_UnlockAndFreeMdl(pReceiveBuffer->pReceiveMdl);	//Free the MDL
		pReceiveBuffer->pReceiveMdl = NULL;
		

		if(pReceiveBuffer->ActualReceiveLength < pReceiveBuffer->TotalReceiveLength)
		{
			//If Data Received is less than the Total then call the IRP with the new MDL

			pReceiveBuffer->pReceiveMdl = TDI_AllocateAndProbeMdl(pReceiveBuffer->pReceiveBuffer + pReceiveBuffer->ActualReceiveLength,pReceiveBuffer->TotalReceiveLength-pReceiveBuffer->ActualReceiveLength,FALSE,FALSE,NULL);


			//Call the TDI_RECEIVE with the new MDL
			try
			{
				IoReuseIrp(pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
				status = TDI_ReceiveOnEndpoint1(
							&pSession->KSEndpoint,
							NULL,       // User Completion Event
							TDI_EndpointReceiveRequestComplete,// User Completion Routine
							pReceiveBuffer,   // User Completion Context
							&pReceiveBuffer->IoReceiveStatus,
							pReceiveBuffer->pReceiveMdl,
							0,           // Flags
							&pSession->pReceiveHeader->pReceiveIrp
							);
			}
			except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("An Exception Occured in TDI_EndpointReceiveRequestComplete() Error Code is %lx\n",GetExceptionCode()));
				status = GetExceptionCode();
				pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
			}
		}
		else if(pReceiveBuffer->ActualReceiveLength == pReceiveBuffer->TotalReceiveLength)
		{
			//Completely Read the Full Packet
			pReceiveBuffer->state = SFTK_BUFFER_FREE;
			pReceiveBuffer->ActualReceiveLength = 0;

			if(pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_HEADER)
			{
				//Header Data is Read
				pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_DATA;
				//Validate the Received Header
				status = PROTO_ProcessReceiveHeader(((ftd_header_t*)pReceiveBuffer->pReceiveBuffer),&Len, pSession);

				//If Valid Header and if there is a payload then get the new Buffer and 
				//do TDI_RECEIVE again.

				if( status == STATUS_SUCCESS || status == STATUS_MORE_PROCESSING_REQUIRED)
				{
					if(Len > 0)
					{
//						pNewReceiveBuffer = COM_GetNextReceiveBuffer(&pSession->pServer->pSessionManager->receiveBufferList);

//						if(pNewReceiveBuffer == NULL)
//						{
//							//There is no Free Buffer available so returning
//							KdPrint(("Unable to get the Free Buffer to Receive Data for Session %lx\n",pSession));
//							return STATUS_MORE_PROCESSING_REQUIRED;
//						}

						status = COM_GetSessionReceiveBuffer(pSession);

						if(!NT_SUCCESS(status))
						{
							//There is no Free Buffer available so returning
							KdPrint(("Unable to get the Free Buffer to Receive Data for Session %lx\n",pSession));
							return STATUS_MORE_PROCESSING_REQUIRED;
						}

						pNewReceiveBuffer = pSession->pReceiveBuffer;

						OS_ASSERT(pNewReceiveBuffer);
						OS_ASSERT(pNewReceiveBuffer->pReceiveBuffer);

						//This is the new Buffer to call TDI_RECEIVE
						pNewReceiveBuffer->state = SFTK_BUFFER_INUSE;
						pNewReceiveBuffer->ActualReceiveLength = 0;
						pNewReceiveBuffer->TotalReceiveLength = Len;
						pNewReceiveBuffer->pReceiveMdl = TDI_AllocateAndProbeMdl(pNewReceiveBuffer->pReceiveBuffer,pNewReceiveBuffer->TotalReceiveLength ,FALSE,FALSE,NULL);
						pNewReceiveBuffer->pReceiveMdl->Next = NULL;
						pNewReceiveBuffer->pSession = pSession;

						OS_ASSERT(pNewReceiveBuffer->pReceiveMdl);

						IoReuseIrp(pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
						try
						{
							status = TDI_ReceiveOnEndpoint1(
										&pSession->KSEndpoint,
										NULL,       // User Completion Event
										TDI_EndpointReceiveRequestComplete,// User Completion Routine
										pNewReceiveBuffer,   // User Completion Context
										&pNewReceiveBuffer->IoReceiveStatus,
										pNewReceiveBuffer->pReceiveMdl,
										0,           // Flags
										&pSession->pReceiveHeader->pReceiveIrp
										);
						}
						except(EXCEPTION_EXECUTE_HANDLER)
						{
							KdPrint(("An Exception Occured in TDI_EndpointReceiveRequestComplete() Error Code is %lx\n",GetExceptionCode()));
							status = GetExceptionCode();
							pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
						}
					}
					else
					{
						//Data is Read completely
						//So can release the Buffer and set the Session Receive status to unused
						KdPrint(("Completed Reading the Whole Header for Session %lx and Length %ld and No PayLoad\n", pSession, pReceiveBuffer->TotalReceiveLength));
						pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
					}
				}
			}
			else if(pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_DATA)
			{
				//Data is Read completely
				//So can release the Buffer and set the Session Receive status to unused
				KdPrint(("Completed Reading the Whole Data for Session %lx and Length %ld\n", pSession, pReceiveBuffer->TotalReceiveLength));
				pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
			}
		}
	}
	else
	{
		KdPrint(( "SftkTCPSreceiveBuffer: status 0x%8.8X and Index = %d for ReceiveBuffer = %lx\n", status , pReceiveBuffer->index , pReceiveBuffer));
		pReceiveBuffer->state = SFTK_BUFFER_FREE;
	}

//	IoFreeIrp( pIrp );

//	pReceiveBuffer->state = SFTK_BUFFER_FREE;
	return STATUS_MORE_PROCESSING_REQUIRED;
}

//This Receive Event Handler demonestrates the use of the IoRequestPacket
//For getting Data that is more than the Indicated.
NTSTATUS 
TDI_ReceiveEventHandler1(
						IN PVOID TdiEventContext,     // Context From SetEventHandler
						IN CONNECTION_CONTEXT ConnectionContext,
						IN ULONG ReceiveFlags,
						IN ULONG BytesIndicated,
						IN ULONG BytesAvailable,
						OUT ULONG *BytesTaken,
						IN PVOID Tsdu,				// pointer describing this TSDU, typically a lump of bytes
						OUT PIRP *IoRequestPacket	// TdiReceive IRP if MORE_PROCESSING_REQUIRED.
						)
{
	NTSTATUS status = STATUS_SUCCESS;
	PTCP_SESSION pSession = NULL;
	PIRP pIrp = NULL;
	PRECEIVE_BUFFER pReceiveBuffer= NULL;
	PDEVICE_OBJECT pDeviceObject;
	ULONG Length;

	pSession = (PTCP_SESSION)TdiEventContext;
	pDeviceObject = IoGetRelatedDeviceObject( pSession->KSEndpoint.m_pFileObject );

    KdPrint(("TDI_ReceiveEventHandler: Entry; EventContext: 0x%8.8X; ConnectionContext: 0x%8.8X; Flags: 0x%8.8X\n",
      (ULONG )TdiEventContext, (ULONG)ConnectionContext, ReceiveFlags)
      );

	KdPrint(("TDI_ReceiveEventHandler: Bytes Indicated = %d , Bytes Available = %d for session %lx\n",
						BytesIndicated,
						BytesAvailable,
						pSession));

	if(BytesIndicated > 64 )
//	if(BytesIndicated < BytesAvailable)
	{
		KdPrint(("We got a Partial Message\n"));
		pReceiveBuffer = COM_GetNextReceiveBuffer(&pSession->pServer->pSessionManager->receiveBufferList);
		if(pReceiveBuffer == NULL)
		{
			KdPrint(("Got Null ReceiveBuffer\n"));
			*BytesTaken = 0;
			return STATUS_SUCCESS;
		}
		else
		{
			pReceiveBuffer->state = SFTK_BUFFER_INUSE;
			*BytesTaken = 0;

			//
			// Allocate IRP
			//
			pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

			if( !pIrp )
			{
				return STATUS_DATA_NOT_ACCEPTED;
			}

			Length = 0;
			pReceiveBuffer->pReceiveMdl->Next = NULL;
			Length = MmGetMdlByteCount( pReceiveBuffer->pReceiveMdl );

			TdiBuildReceive(
				pIrp,
				pDeviceObject,
				pSession->KSEndpoint.m_pFileObject,
				TDI_EndpointReceiveRequestComplete,   // Completion routine
				pReceiveBuffer,              // Completion context
				pReceiveBuffer->pReceiveMdl,                            // the data buffer
				0,                           // receive flags
				Length                           // total length of receive MDLs
				);

			//
			// Make the next stack location current.  Normally IoCallDriver would
			// do this, but for this IRP it has been bypassed.
			//
			IoSetNextIrpStackLocation( pIrp );

			*IoRequestPacket = pIrp;
			KdPrint(("Just before returning\n"));
			return STATUS_MORE_PROCESSING_REQUIRED;
		}
	}
	else
	{
		*BytesTaken = BytesIndicated;
		return STATUS_SUCCESS;
	}
}




#endif // This is _OBSOLETE_

************************************************/
