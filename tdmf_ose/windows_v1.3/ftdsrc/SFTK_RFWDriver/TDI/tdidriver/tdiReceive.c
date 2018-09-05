//tdiReceive.c
//This File Defines the Receive Worker Thread that will be Responsible for Receiving the
//Infor from the Secondary.

#include "tdiutil.h"


VOID
SftkTCPSReceiveCompletion(
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
    )
{
   PRECEIVE_BUFFER  pReceiveBuffer = (PRECEIVE_BUFFER )UserCompletionContext;

   if( NT_SUCCESS( IoStatusBlock->Status ) )
   {
	   KdPrint(( "SftkTCPSReceiveBuffer: Receive %d Bytes and Index = %d for ReceiveBuffer = %lx\n", IoStatusBlock->Information, pReceiveBuffer->index , pReceiveBuffer));
   }
   else
   {
      KdPrint(( "SftkTCPSreceiveBuffer: Status 0x%8.8X and Index = %d for ReceiveBuffer = %lx\n", IoStatusBlock->Status , pReceiveBuffer->index , pReceiveBuffer));
   }
   pReceiveBuffer->state = SFTK_BUFFER_FREE;
return;
}

NTSTATUS CreateReceiveThread1(PSESSION_MANAGER pSessionManager)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PTCP_SESSION pSession = NULL;
	PLIST_ENTRY pTempEntry;
//	KIRQL oldIrql;
//	PRECEIVE_BUFFER pReceiveBuffer=NULL;

	KdPrint(("Enter CreateReceiveThread1()\n"));
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
					ExAcquireFastMutexUnsafe(&pSessionManager->ReceiveLock);


					//30 Milli-Seconds in 100*Nano Seconds
					iWait.QuadPart = -(10*10*1000);
					pTemp = pSessionManager->ServerList.Flink;

					while(pTemp != &pSessionManager->ServerList && !bExit)
					{
						KdPrint(("Going for the Server List Also in CreateReceiveThread1()\n"));
						pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);

						if(pServerElement == NULL)
						{
							KdPrint(("The SERVER_ELEMENT is Null and hence cannot Proceed\n"));
							Status = STATUS_MEMORY_NOT_ALLOCATED;
							break;
				//			goto End;
						}
						
						ASSERT(pServerElement);

						if(pServerElement->pSession != NULL && !bExit)
						{
							ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

							Status = KeWaitForSingleObject(&pSessionManager->IOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
							if(Status == STATUS_SUCCESS)
							{
								KdPrint(("The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
								bExit = TRUE;
								break;
							}

							pSession = pServerElement->pSession;
							if(pSession == NULL)
							{
								KdPrint(("The TCP_SESSION is Null and hence cannot Proceed"));
							}
							ASSERT(pSession);

							if((pSession->bSessionEstablished == SFTK_CONNECTED) &&
								(pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_NOTINIT))
							{
								pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_HEADER;

								KdPrint(("The Session that is used to receive Data is %lx\n",pSession));

								if(pSession->pReceiveHeader->pReceiveMdl == NULL)
								{
									pSession->pReceiveHeader->pReceiveMdl = SFTK_TDI_AllocateAndProbeMdl(pSession->pReceiveHeader->pReceiveBuffer , pSession->pReceiveHeader->TotalReceiveLength ,FALSE,FALSE,NULL);
								}

								ASSERT(pSession->pReceiveHeader->pReceiveMdl);

								(pSession->pReceiveHeader->pReceiveMdl)->Next = NULL;   // IMPORTANT!!!

								pSession->pReceiveHeader->ActualReceiveLength = 0;

								try
								{
									Status = SFTK_TDI_ReceiveOnEndpoint1(
												&pSession->KSEndpoint,
												NULL,       // User Completion Event
												SftkEndpointReceiveRequestComplete,// User Completion Routine
												pSession->pReceiveHeader,   // User Completion Context
												&pSession->pReceiveHeader->IoReceiveStatus,
												pSession->pReceiveHeader->pReceiveMdl,
												0,           // Flags
												&pSession->pReceiveHeader->pReceiveIrp
												);
								}
								except(EXCEPTION_EXECUTE_HANDLER)
								{
									KdPrint(("An Exception Occured in CreateReceiveThread1() Error Code is %lx\n",GetExceptionCode()));
									Status = GetExceptionCode();
									pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
								}
							}
							else
							{
								if(pSession->pServer->pSessionManager->nLiveSessions == 0)
								{
									KdPrint(("There are no live sessions so exiting\n"));
									bExit = TRUE;
									break;
								}
								//30 Milli-Seconds in 100*Nano Seconds
								iWait.QuadPart = -(50*10*1000);
								KeDelayExecutionThread(KernelMode,FALSE,&iWait);
							}

	//						KeDelayExecutionThread(KernelMode,FALSE,&iWait);
						}	//while(!IsListEmpty(pServerElement->UsedSessionList))
						pTemp = pTemp->Flink;
					}
					if(!bExit)
					{
						ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

						Status = KeWaitForSingleObject(&pSessionManager->IOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
						if(Status == STATUS_SUCCESS)
						{
							KdPrint(("The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}
					}
				}//try
				finally
				{
					//Release the Receive Lock
					ExReleaseFastMutexUnsafe(&pSessionManager->ReceiveLock);
					//Release the Resource 
					ExReleaseResourceLite(&pSessionManager->ServerListLock);
					KeLeaveCriticalRegion();	//Enable the Kernel APC's
				}
			}//while(!bExit)
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint(("An Exception Occured in CreateReceiveThread1() Error Code is %lx\n",GetExceptionCode()));
			Status = GetExceptionCode();
		}
	}
	finally
	{
		//Check if the Resource is still acquired for Shared Access
		//If So release the resource
		if(ExIsResourceAcquiredSharedLite(&pSessionManager->ServerListLock) >0)
		{
			ExReleaseResourceLite(&pSessionManager->ServerListLock);
		//Check if the Kernel APC's are Disabled 
		//if so Leave the Critical Section
	//		if(KeAreApcsDisabled())
			{
				KeLeaveCriticalRegion();
			}
		}

		KdPrint(("Exiting the CreateReceiveThread1() Status = %lx\n",Status));
	}

	return PsTerminateSystemThread(Status);
}

//This new version of receive will do all the Receive in this function and
//Not in the IOCompletion Callback
//For 

#define __NEW_RECEIVE__
#ifdef __NEW_RECEIVE__

SftkEndpointReceiveRequestComplete2(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp,
	IN PVOID Context
	)
{
	PRECEIVE_BUFFER pReceiveBuffer= NULL;
	PTCP_SESSION pSession = NULL;
	NTSTATUS Status;
	ULONG Length;
	
	Status = pIrp->IoStatus.Status;
	Length = pIrp->IoStatus.Information;
	pReceiveBuffer = (PRECEIVE_BUFFER)Context;

	pSession = pReceiveBuffer->pSession;	//Get the Session Pointer

	if(NT_SUCCESS(Status))
	{
		KdPrint(( "SftkEndpointReceiveRequestComplete: Receive %d Bytes and Index = %d for ReceiveBuffer = %lx\n", Length, pReceiveBuffer->index , pReceiveBuffer));
		pReceiveBuffer->ActualReceiveLength += Length;  //Increment the Actual Length
		SFTK_TDI_UnlockAndFreeMdl(pReceiveBuffer->pReceiveMdl);	//Free the MDL
		pReceiveBuffer->pReceiveMdl = NULL;
	}
	else
	{
		KdPrint(( "SftkTCPSreceiveBuffer: Status 0x%8.8X and Index = %d for ReceiveBuffer = %lx\n", Status , pReceiveBuffer->index , pReceiveBuffer));
		pReceiveBuffer->state = SFTK_BUFFER_FREE;
	}
	return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS ReceiveThread2(PSESSION_MANAGER pSessionManager)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PTCP_SESSION pSession = NULL;
	PLIST_ENTRY pTempEntry;
	ULONG nDataLength = 0;
//	KIRQL oldIrql;
//	PRECEIVE_BUFFER pReceiveBuffer=NULL;

	KdPrint(("Enter CreateReceiveThread1()\n"));
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
					ExAcquireFastMutexUnsafe(&pSessionManager->ReceiveLock);


					//30 Milli-Seconds in 100*Nano Seconds
					iWait.QuadPart = -(10*10*1000);
					pTemp = pSessionManager->ServerList.Flink;

					while(pTemp != &pSessionManager->ServerList && !bExit)
					{
						KdPrint(("Going for the Server List Also in CreateReceiveThread1()\n"));
						pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);

						if(pServerElement == NULL)
						{
							KdPrint(("The SERVER_ELEMENT is Null and hence cannot Proceed\n"));
							Status = STATUS_MEMORY_NOT_ALLOCATED;
							break;
				//			goto End;
						}
						
						ASSERT(pServerElement);

						if(pServerElement->pSession != NULL && !bExit)
						{
							ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

							Status = KeWaitForSingleObject(&pSessionManager->IOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
							if(Status == STATUS_SUCCESS)
							{
								KdPrint(("The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
								bExit = TRUE;
								break;
							}

							pSession = pServerElement->pSession;
							if(pSession == NULL)
							{
								KdPrint(("The TCP_SESSION is Null and hence cannot Proceed"));
							}
							ASSERT(pSession);

							if(pSession->bSessionEstablished == SFTK_CONNECTED) 
							{
								if(pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_NOTINIT)
								{
									pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_HEADER;

									KdPrint(("The Session that is used to receive Data is %lx\n",pSession));

									pSession->pReceiveHeader->TotalReceiveLength = PROTOCOL_HEADER_SIZE;

									if(pSession->pReceiveHeader->pReceiveMdl == NULL)
									{
										pSession->pReceiveHeader->pReceiveMdl = SFTK_TDI_AllocateAndProbeMdl(pSession->pReceiveHeader->pReceiveBuffer , pSession->pReceiveHeader->TotalReceiveLength ,FALSE,FALSE,NULL);
									}

									ASSERT(pSession->pReceiveHeader->pReceiveMdl);

									(pSession->pReceiveHeader->pReceiveMdl)->Next = NULL;   // IMPORTANT!!!


									pSession->pReceiveHeader->ActualReceiveLength = 0;

									try
									{
										if(pSession->pReceiveHeader->pReceiveIrp != NULL)
										{
											IoReuseIrp(pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
										}
										Status = SFTK_TDI_ReceiveOnEndpoint1(
													&pSession->KSEndpoint,
													NULL,       // User Completion Event
													SftkEndpointReceiveRequestComplete2,// User Completion Routine
													pSession->pReceiveHeader,   // User Completion Context
													&pSession->pReceiveHeader->IoReceiveStatus,
													pSession->pReceiveHeader->pReceiveMdl,
													0,           // Flags
													&pSession->pReceiveHeader->pReceiveIrp
													);
									}
									except(EXCEPTION_EXECUTE_HANDLER)
									{
										KdPrint(("An Exception Occured in CreateReceiveThread1() Error Code is %lx\n",GetExceptionCode()));
										Status = GetExceptionCode();
										pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
									}
								}
								else if(pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_HEADER)
								{
									//This will assure that the Previous CompletionRoutine completed
									//Successfully, before calling the IoCallDriver() again
									//We can use an Event here, but it does the samething
									if(pSession->pReceiveHeader->pReceiveMdl == NULL)
									{
										if(pSession->pReceiveHeader->ActualReceiveLength < pSession->pReceiveHeader->TotalReceiveLength)
										{
											//We still Need to read somemore buffer
											pSession->pReceiveHeader->pReceiveMdl = SFTK_TDI_AllocateAndProbeMdl(pSession->pReceiveHeader->pReceiveBuffer + pSession->pReceiveHeader->ActualReceiveLength,pSession->pReceiveHeader->TotalReceiveLength-pSession->pReceiveHeader->ActualReceiveLength,FALSE,FALSE,NULL);
											//Call the TDI_RECEIVE with the new MDL
											try
											{
												if(pSession->pReceiveHeader->pReceiveIrp != NULL)
												{
													IoReuseIrp(pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
												}
												Status = SFTK_TDI_ReceiveOnEndpoint1(
															&pSession->KSEndpoint,
															NULL,       // User Completion Event
															SftkEndpointReceiveRequestComplete2,// User Completion Routine
															pSession->pReceiveHeader,   // User Completion Context
															&pSession->pReceiveHeader->IoReceiveStatus,
															pSession->pReceiveHeader->pReceiveMdl,
															0,           // Flags
															&pSession->pReceiveHeader->pReceiveIrp
															);
											}
											except(EXCEPTION_EXECUTE_HANDLER)
											{
												KdPrint(("An Exception Occured in SftkEndpointReceiveRequestComplete() Error Code is %lx\n",GetExceptionCode()));
												Status = GetExceptionCode();
												pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
											}


										}
										else
										{
											nDataLength = 0;
											Status = SftkProcessReceiveHeader(((ftd_header_t*)pSession->pReceiveHeader->pReceiveBuffer),&nDataLength,pSession);
											//Received the Total header so process it and read the Data if 
											//there is any

											if( Status == STATUS_SUCCESS || Status == STATUS_MORE_PROCESSING_REQUIRED)
											{
												if(nDataLength > 0)
												{
													Status = GetSessionReceiveBuffer(pSession);

													if(!NT_SUCCESS(Status))
													{
														//There is no Free Buffer available so returning
														KdPrint(("Unable to get the Free Buffer to Receive Data for Session %lx\n",pSession));
														return STATUS_MORE_PROCESSING_REQUIRED;
													}

													ASSERT(pSession->pReceiveBuffer);
													ASSERT(pSession->pReceiveBuffer->pReceiveBuffer);

													//This is the new Buffer to call TDI_RECEIVE
													pSession->pReceiveBuffer->state = SFTK_BUFFER_INUSE;
													pSession->pReceiveBuffer->ActualReceiveLength = 0;
													pSession->pReceiveBuffer->TotalReceiveLength = nDataLength;
													pSession->pReceiveBuffer->pReceiveMdl = SFTK_TDI_AllocateAndProbeMdl(pSession->pReceiveBuffer->pReceiveBuffer,pSession->pReceiveBuffer->TotalReceiveLength ,FALSE,FALSE,NULL);
													pSession->pReceiveBuffer->pReceiveMdl->Next = NULL;
													pSession->pReceiveBuffer->pSession = pSession;

													ASSERT(pSession->pReceiveBuffer->pReceiveMdl);

													if(pSession->pReceiveHeader->pReceiveIrp != NULL)
													{
														IoReuseIrp(pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
													}
													try
													{
														Status = SFTK_TDI_ReceiveOnEndpoint1(
																	&pSession->KSEndpoint,
																	NULL,       // User Completion Event
																	SftkEndpointReceiveRequestComplete2,// User Completion Routine
																	pSession->pReceiveBuffer,   // User Completion Context
																	&pSession->pReceiveBuffer->IoReceiveStatus,
																	pSession->pReceiveBuffer->pReceiveMdl,
																	0,           // Flags
																	&pSession->pReceiveHeader->pReceiveIrp
																	);
													}
													except(EXCEPTION_EXECUTE_HANDLER)
													{
														KdPrint(("An Exception Occured in SftkEndpointReceiveRequestComplete() Error Code is %lx\n",GetExceptionCode()));
														Status = GetExceptionCode();
														pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
													}
												}
												else
												{
													//Data is Read completely
													//So can release the Buffer and set the Session Receive status to unused
													KdPrint(("Completed Reading the Whole Header for Session %lx and Length %ld and No PayLoad\n", pSession, pSession->pReceiveHeader->TotalReceiveLength));
													pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
												}
											}
										}
									}
								}
								else	//This is pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_DATA
								{
									//This will assure that the Previous CompletionRoutine completed
									//Successfully, before calling the IoCallDriver() again
									//We can use an Event here, but it does the samething
									ASSERT(pSession->pReceiveBuffer);
									if(pSession->pReceiveBuffer->pReceiveMdl == NULL)
									{
										if(pSession->pReceiveBuffer->ActualReceiveLength < pSession->pReceiveBuffer->TotalReceiveLength)
										{
											//Still Need to read sommore data
											//We still Need to read somemore buffer
											pSession->pReceiveBuffer->pReceiveMdl = SFTK_TDI_AllocateAndProbeMdl(pSession->pReceiveBuffer->pReceiveBuffer + pSession->pReceiveBuffer->ActualReceiveLength,pSession->pReceiveBuffer->TotalReceiveLength-pSession->pReceiveBuffer->ActualReceiveLength,FALSE,FALSE,NULL);
											//Call the TDI_RECEIVE with the new MDL
											try
											{
												if(pSession->pReceiveHeader->pReceiveIrp != NULL)
												{
													IoReuseIrp(pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
												}
												Status = SFTK_TDI_ReceiveOnEndpoint1(
															&pSession->KSEndpoint,
															NULL,       // User Completion Event
															SftkEndpointReceiveRequestComplete2,// User Completion Routine
															pSession->pReceiveBuffer,   // User Completion Context
															&pSession->pReceiveBuffer->IoReceiveStatus,
															pSession->pReceiveBuffer->pReceiveMdl,
															0,           // Flags
															&pSession->pReceiveHeader->pReceiveIrp
															);
											}
											except(EXCEPTION_EXECUTE_HANDLER)
											{
												KdPrint(("An Exception Occured in SftkEndpointReceiveRequestComplete() Error Code is %lx\n",GetExceptionCode()));
												Status = GetExceptionCode();
												pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
											}

										}
										else
										{
											//Data is Read completely
											//So can release the Buffer and set the Session Receive status to unused

											nDataLength = 0;
											Status = SftkProcessReceiveData(((ftd_header_t*)pSession->pReceiveHeader->pReceiveBuffer),pSession->pReceiveBuffer->pReceiveBuffer,pSession->pReceiveBuffer->TotalReceiveLength,&nDataLength,pSession);

											if(Status == STATUS_MORE_PROCESSING_REQUIRED)
											{
												//Still More Data to be Read so Increase the TotalSize 
												//and try again
												pSession->pReceiveBuffer->TotalReceiveLength += nDataLength;
											}
											else
											{
												//All the Data is Read, so fine
												KdPrint(("Completed Reading the Whole Data for Session %lx and Length %ld\n", pSession, pSession->pReceiveBuffer->TotalReceiveLength));
												pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
											}
										}
									}//if(pSession->pReceiveBuffer->pReceiveMdl == NULL)
								}//This is pSession->bReceiveStatus == SFTK_PROCESSING_RECEIVE_DATA
							}
							else
							{
								if(pSession->pServer->pSessionManager->nLiveSessions == 0)
								{
									KdPrint(("There are no live sessions so exiting\n"));
									bExit = TRUE;
									break;
								}
								//30 Milli-Seconds in 100*Nano Seconds
								iWait.QuadPart = -(10*10*1000);
								KeDelayExecutionThread(KernelMode,FALSE,&iWait);
							}
						}
						pTemp = pTemp->Flink;
					}
					if(!bExit)
					{
						ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

						Status = KeWaitForSingleObject(&pSessionManager->IOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
						if(Status == STATUS_SUCCESS)
						{
							KdPrint(("The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}
					}
				}//try
				finally
				{
					//Release the Receive Lock
					ExReleaseFastMutexUnsafe(&pSessionManager->ReceiveLock);
					//Release the Resource 
					ExReleaseResourceLite(&pSessionManager->ServerListLock);
					KeLeaveCriticalRegion();	//Enable the Kernel APC's
				}
			}//while(!bExit)
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint(("An Exception Occured in CreateReceiveThread1() Error Code is %lx\n",GetExceptionCode()));
			Status = GetExceptionCode();
		}
	}
	finally
	{
		KdPrint(("Exiting the CreateReceiveThread1() Status = %lx\n",Status));
	}

	return PsTerminateSystemThread(Status);
}

#endif	//__NEW_RECEIVE__
