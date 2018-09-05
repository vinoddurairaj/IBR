//tdiSend.c
//This File has the Send Worker Thread Function that will be responsible for Sending 
//The Data in the BufferWindow.

//#include "common.h"
#include "tdiutil.h"
//#include <wchar.h>      // for time_t, dev_t
//#include <windef.h>
//#include "mmgr_ntkrnl.h"

//#include "slab.h"
//#include "mmg.h"
//#include "dtb.h"
//#include "sftkprotocol.h"

void FillRawBufferRandom(PSEND_BUFFER pSendBuffer, PULONG pLen);

#ifdef _SEND_IN_CHUNKS

SftkEndpointSendRequestComplete(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp,
	IN PVOID Context
	)
{
	PSEND_BUFFER pSendBuffer= NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG Length = 0;
	ULONG remainingLength = 0;
//	PPROTOCOL_PACKET pProtocolPacket;
//	KIRQL oldIrql;
	
	Status = pIrp->IoStatus.Status;
	Length = pIrp->IoStatus.Information;
	pSendBuffer = (PSEND_BUFFER)Context;

	if(NT_SUCCESS(Status))
	{
		KdPrint(( "SftkEndpointSendRequestComplete: Send %d Bytes and Index = %d for SendBuffer = %lx\n", Length, pSendBuffer->index , pSendBuffer));

		pSendBuffer->ActualSendLength += Length;
		SFTK_TDI_UnlockAndFreeMdl(pSendBuffer->pSendMdl);
		pSendBuffer->pSendMdl = NULL;

		//Sent the Whole Message
		//So Reuse this Buffer
		if(pSendBuffer->ActualSendLength == pSendBuffer->TotalSendLength)
		{
			pSendBuffer->ActualSendLength = 0;
			pSendBuffer->state = SFTK_BUFFER_FREE;
			pSendBuffer->pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
		}
		else
		{
			remainingLength = pSendBuffer->TotalSendLength - pSendBuffer->ActualSendLength;
			if(remainingLength <= pSendBuffer->pSendList->ChunkSize)
			{
				//Send whatever is the remaining buffer
				pSendBuffer->pSendMdl = SFTK_TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer + pSendBuffer->ActualSendLength,remainingLength,FALSE,FALSE,NULL);	
			}
			else
			{
				//Send Only a Chunk Size
				pSendBuffer->pSendMdl = SFTK_TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer + pSendBuffer->ActualSendLength,pSendBuffer->pSendList->ChunkSize,FALSE,FALSE,NULL);	
			}
			try
			{
				if(pSendBuffer->pSendIrp != NULL)
				{
					IoReuseIrp(pSendBuffer->pSendIrp,STATUS_SUCCESS);
				}
				Status = SFTK_TDI_SendOnEndpoint1(
								&pSendBuffer->pSession->KSEndpoint,
								NULL,       // User Completion Event
								//SftkTCPCSendCompletion,       // User Completion Routine
								SftkEndpointSendRequestComplete,
								pSendBuffer,       // User Completion Context
								&pSendBuffer->IoSendStatus,
								pSendBuffer->pSendMdl,
								0,           // Send Flags
								&pSendBuffer->pSendIrp
								);
			}
			except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("An Exception Occured in CreateSendThread1() Error Code is %lx\n",GetExceptionCode()));
				Status = GetExceptionCode();
				pSendBuffer->state = SFTK_BUFFER_USED;
				pSendBuffer->pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
			}
		}
	}
	else
	{
		pSendBuffer->state = SFTK_BUFFER_USED;
		pSendBuffer->pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
		KdPrint(( "SftkTCPSSendBuffer: Status 0x%8.8X and Index = %d for SendBuffer = %lx\n", Status , pSendBuffer->index , pSendBuffer));
	}

//	IoFreeIrp( pIrp );

//	pSendBuffer->state = SFTK_BUFFER_FREE;
	return STATUS_MORE_PROCESSING_REQUIRED;
}

#else  //_SEND_IN_CHUNKS

SftkEndpointSendRequestComplete(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp,
	IN PVOID Context
	)
{
	PSEND_BUFFER pSendBuffer= NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG Length = 0;
	
	Status = pIrp->IoStatus.Status;
	Length = pIrp->IoStatus.Information;
	pSendBuffer = (PSEND_BUFFER)Context;

	if(NT_SUCCESS(Status))
	{
		KdPrint(( "SftkEndpointSendRequestComplete: Send %d Bytes and Index = %d for SendBuffer = %lx\n", Length, pSendBuffer->index , pSendBuffer));

		SFTK_TDI_UnlockAndFreeMdl(pSendBuffer->pSendMdl);
		pSendBuffer->pSendMdl = NULL;
		pSendBuffer->state = SFTK_BUFFER_FREE;
	}
	else
	{
		KdPrint(( "SftkTCPSSendBuffer: Status 0x%8.8X and Index = %d for SendBuffer = %lx\n", Status , pSendBuffer->index , pSendBuffer));
		pSendBuffer->state = SFTK_BUFFER_USED;
	}
	return STATUS_MORE_PROCESSING_REQUIRED;
}
#endif  //_SEND_IN_CHUNKS

VOID
SftkTCPCSendCompletion(
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
    )
{
   PSEND_BUFFER  pSendBuffer = (PSEND_BUFFER )UserCompletionContext;

   if( NT_SUCCESS( IoStatusBlock->Status ) )
   {
	   // pSendBuffer->ActualSendLength = 
	  KdPrint(( "SftkTCPCSendBuffer: Sent %d Bytes and index = %d for SendBuffer = %lx\n", IoStatusBlock->Information, pSendBuffer->index,pSendBuffer));
//	  SFTK_TDI_UnlockAndFreeMdl(pSendBuffer->pSendMdl);
//	  pSendBuffer->pSendMdl = NULL;
	  pSendBuffer->state = SFTK_BUFFER_FREE;

   }
   else
   {
	  KdPrint(( "SftkTCPCSendBuffer: Status 0x%8.8X and index = %d for SendBuffer = %lx\n", IoStatusBlock->Status, pSendBuffer->index , pSendBuffer));
	  pSendBuffer->state = SFTK_BUFFER_USED;
   }
return;
}

NTSTATUS CreateSendThread1(PSESSION_MANAGER pSessionManager)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PTCP_SESSION pSession = NULL;
	PLIST_ENTRY pTempEntry;
	PSEND_BUFFER pSendBuffer = NULL;
	ULONG LenBuf =0;
	PPROTOCOL_PACKET pProtocolPacket = NULL;

	KdPrint(("Enter CreateSendThread1()\n"));
	try
	{
		try
		{
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
					//Send Lock
					ExAcquireFastMutexUnsafe(&pSessionManager->SendLock);

					pTemp = pSessionManager->ServerList.Flink;

					while(pTemp != &pSessionManager->ServerList && !bExit)
					{
						KdPrint(("Going for the Server List Also in CreateSendThread1()\n"));
						pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);

						if(pServerElement == NULL)
						{
							KdPrint(("The SERVER_ELEMENT is Null and hence cannot Proceed\n"));
							Status = STATUS_MEMORY_NOT_ALLOCATED;
							break;
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
								pSendBuffer = GetNextStaleSendBuffer(&pSessionManager->sendBufferList);

								if(pSendBuffer == NULL)
								{
//									KdPrint(("Coundnt Find a Stale Send Buffer So Calling GetNextSendBuffer\n"));
									pSendBuffer = GetNextSendBuffer(&pSessionManager->sendBufferList);
								
									if(pSendBuffer != NULL)
									{
										//Fill in Some Data or Do Something
										FillRawBufferRandom(pSendBuffer,&LenBuf);

										//Create the MDL
										pSendBuffer->pSendMdl = SFTK_TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer,LenBuf,FALSE,FALSE,NULL);

										//Think About this if the Lock Failed
										ASSERT(pSendBuffer->pSendMdl);
										
										//Add the Protocol Object to the Queue
										//////////////////////////////////////////////////////////

										pProtocolPacket = (PPROTOCOL_PACKET)ExAllocateFromNPagedLookasideList(&pSessionManager->ProtocolList);

										//If there is no Memory for the Protocol header
										//Just Clean up and exit the Send Thread
										if(pProtocolPacket == NULL && pSendBuffer != NULL && pSendBuffer->pSendMdl != NULL)
										{
											KdPrint(("Unable to Allocate PROTOCOL_PACKET So Exiting the Send Thread\n"));
											SFTK_TDI_UnlockAndFreeMdl( pSendBuffer->pSendMdl );
											pSendBuffer->pSendMdl = NULL;
											pSendBuffer->state = SFTK_BUFFER_FREE;
											bExit = TRUE;
											leave;
										}
										else
										{
											//Get the Protocol Packet Object, initialize the IO Header and the ftd_header_t
											//then initialize the time this packet is added to the protocol Queue
											//Add the Packet to the ProtocolQueue
											NdisZeroMemory(pProtocolPacket,PROTOCOL_PACKET_SIZE);
											pProtocolPacket->pIoPacket = pSendBuffer->pIoPacket;
											NdisMoveMemory(&pProtocolPacket->ProtocolHeader,pSendBuffer->pSendBuffer,PROTOCOL_HEADER_SIZE);
											KeQuerySystemTime(&pProtocolPacket->tStartTime);

											KeInitializeEvent(&pProtocolPacket->ReceiveAckEvent,NotificationEvent,FALSE);

//											KeAcquireSpinLock(&pSessionManager->ProtocolQueueLock,&oldIrql);
											InsertHeadList(&pSessionManager->ProtocolQueue,&pProtocolPacket->ListElement);
//											KeReleaseSpinLock(&pSessionManager->ProtocolQueueLock,oldIrql);
										}

										//End of Add Protocol Header to the protocol Queue
										//////////////////////////////////////////////////////////
									}
									else
									{
										//30 Milli-Seconds in 100*Nano Seconds
										iWait.QuadPart = -(30*10*1000);
										KdPrint(("There are no more Buffers available to Send Data so Wait\n"));
										KeDelayExecutionThread(KernelMode,FALSE,&iWait);
										leave;
									}
								}
		                        
								if(pSendBuffer != NULL && pSendBuffer->pSendMdl != NULL)
								{

									pSendBuffer->pSendMdl->Next = NULL;
									pSendBuffer->pSession = pSession;

									KdPrint(("The Session that is used to Send is %lx\n",pSession));

									try
									{
										//Reinitialize the IRP, incase of resuing it.
										if(pSendBuffer->pSendIrp != NULL)
										{
											IoReuseIrp(pSendBuffer->pSendIrp,STATUS_SUCCESS);
										}
										Status = SFTK_TDI_SendOnEndpoint1(
														&pSession->KSEndpoint,
														NULL,       // User Completion Event
														//SftkTCPCSendCompletion,       // User Completion Routine
														SftkEndpointSendRequestComplete,
														pSendBuffer,       // User Completion Context
														&pSendBuffer->IoSendStatus,
														pSendBuffer->pSendMdl,
														0,           // Send Flags
														&pSendBuffer->pSendIrp
														);
									}
									except(EXCEPTION_EXECUTE_HANDLER)
									{
										KdPrint(("An Exception Occured in CreateSendThread1() Error Code is %lx\n",GetExceptionCode()));
										pSendBuffer->state = SFTK_BUFFER_USED;
									}
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
								iWait.QuadPart = -(10*10*1000);
								KeDelayExecutionThread(KernelMode,FALSE,&iWait);
							}

							iWait.QuadPart = -(10*10*1000);
							KeDelayExecutionThread(KernelMode,FALSE,&iWait);
						}	//if(pServerElement->pSession != NULL && !bExit)
						pTemp = pTemp->Flink;
					}	//while(pTemp != &pSessionManager->ServerList && !bExit)
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
					//Release the Send Lock
					ExReleaseFastMutexUnsafe(&pSessionManager->SendLock);
					//Release the Resource 
					ExReleaseResourceLite(&pSessionManager->ServerListLock);
					KeLeaveCriticalRegion();	//Enable the Kernel APC's
				}
			}//while(!bExit)
		}//try
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint(("An Exception Occured in CreateSendThread1() Error Code is %lx\n",GetExceptionCode()));
		}
	}//try
	finally
	{
		KdPrint(("Exiting the CreateSendThread1() Status = %lx\n",Status));
	}

	return PsTerminateSystemThread(Status);
}

void FillRawBufferRandom(PSEND_BUFFER pSendBuffer, PULONG pLen)
{

//	LARGE_INTEGER systemTime;
//	UCHAR pTransmitChar;

	*pLen = 1024*100;

//	pTransmitChar = (UCHAR)((*pLen & 0xFF) ^((*pLen >> 8) & 0xFF)|((*pLen >> 16) & 0xFF));

	NdisZeroMemory(pSendBuffer->pSendBuffer,pSendBuffer->pSendList->SendWindow);
	((ftd_header_t*)pSendBuffer->pSendBuffer)->magicvalue = MAGICHDR;
	((ftd_header_t*)pSendBuffer->pSendBuffer)->len = *pLen;
//	NdisFillMemory(pSendBuffer->pSendBuffer+sizeof(ftd_header_t),*pLen,pTransmitChar);

	*pLen = *pLen + sizeof(ftd_header_t);
}
//Sends Header and Data 
//Will stop the Send Thread, and then induce the Protocol Packet.
NTSTATUS SftkSendVector(SFTK_IOVEC vectorArray[] , LONG len , PSESSION_MANAGER pSessionManager)
{
	NTSTATUS Status = STATUS_SUCCESS;
	int index =0;
	PSEND_BUFFER pSendBuffer = NULL;
	int retrycount = 10;
	LARGE_INTEGER iWait;
	PPROTOCOL_PACKET pProtocolPacket = NULL;

	KdPrint(("Entering SftkSendVector() the number of elements = %d\n",len));

	//Check if the Length is wrong
	if(len <= 0)
	{
		KdPrint(("Invalid Parameter the length is Zero\n"));
		return STATUS_INVALID_PARAMETER;
	}

	//Check if this is Replicator Protocol header or not.
	if(((ftd_header_t*)vectorArray[0].pBuffer)->magicvalue != MAGICHDR)
	{
		KdPrint(("Invalid Header Value Not a Protocol Header\n"));
		return STATUS_INVALID_PARAMETER;
	}

	try
	{
		iWait.QuadPart = -(30*10*1000);
		ExAcquireFastMutex(&pSessionManager->SendLock);

		//Get the Send Buffer from the Queue

		while(retrycount >0)
		{
			pSendBuffer = GetNextSendBuffer(&pSessionManager->sendBufferList);

			if(pSendBuffer != NULL)
			{
				//Got the Buffer Reqd
				break;
			}
			else
			{
				KeDelayExecutionThread(KernelMode,FALSE,&iWait);
				//try one more time
				retrycount--;
			}
		}

		if(pSendBuffer == NULL)
		{
			Status =  STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		pSendBuffer->ActualSendLength = 0;
		pSendBuffer->TotalSendLength =0;

		for(index =0 ; index < len ; index++)
		{
			NdisMoveMemory(pSendBuffer->pSendBuffer+pSendBuffer->TotalSendLength,vectorArray[index].pBuffer,vectorArray[index].nLength);
			pSendBuffer->TotalSendLength += vectorArray[index].nLength;
		}

		pSendBuffer->pSendMdl = SFTK_TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer,pSendBuffer->TotalSendLength,FALSE,FALSE,NULL);

		if(pSendBuffer->pSendMdl == NULL)
		{
			KdPrint(("Unable to Lock Buffer from MDL Usage\n"));
			Status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}
		
		//Adding the Protocol Header to the Protocol Queue
		pProtocolPacket = (PPROTOCOL_PACKET)ExAllocateFromNPagedLookasideList(&pSessionManager->ProtocolList);

		if(pProtocolPacket == NULL)
		{
			KdPrint(("Unable to Allocate PROTOCOL_PACKET so returning Error\n"));
			Status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}
		else
		{
			//Get the Protocol Packet Object, initialize the IO Header and the ftd_header_t
			//then initialize the time this packet is added to the protocol Queue
			//Add the Packet to the ProtocolQueue
			NdisZeroMemory(pProtocolPacket,PROTOCOL_PACKET_SIZE);
			pProtocolPacket->pIoPacket = pSendBuffer->pIoPacket;
			NdisMoveMemory(&pProtocolPacket->ProtocolHeader,pSendBuffer->pSendBuffer,PROTOCOL_HEADER_SIZE);
			KeQuerySystemTime(&pProtocolPacket->tStartTime);

			KeInitializeEvent(&pProtocolPacket->ReceiveAckEvent,NotificationEvent,FALSE);
//			KeAcquireSpinLock(&pSessionManager->ProtocolQueueLock,&oldIrql);
			InsertHeadList(&pSessionManager->ProtocolQueue,&pProtocolPacket->ListElement);
//			KeReleaseSpinLock(&pSessionManager->ProtocolQueueLock,oldIrql);
		}
		//End of Adding the Protocol Header to the Protocol Queue

		pSendBuffer->state = SFTK_BUFFER_USED;

	}
	finally
	{
		if(!NT_SUCCESS(Status) && pSendBuffer != NULL && pSendBuffer->pSendMdl != NULL)
		{
			SFTK_TDI_UnlockAndFreeMdl( pSendBuffer->pSendMdl );
			pSendBuffer->pSendMdl = NULL;

			if(pProtocolPacket != NULL)
			{
				ExFreeToNPagedLookasideList(&pSessionManager->ProtocolList, pProtocolPacket);
			}
			pSendBuffer->state = SFTK_BUFFER_FREE;
		}

		ExReleaseFastMutex(&pSessionManager->SendLock);
		KdPrint(("Exiting the SftkSendVector() Status = %lx\n",Status));
	}
return Status;
}

//Code with Optimizations
#define __NEW_SEND__
#ifdef __NEW_SEND__

SftkEndpointSendRequestComplete2(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp,
	IN PVOID Context
	)
{
	PSEND_BUFFER pSendBuffer= NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG Length = 0;
	ULONG remainingLength = 0;
	
	Status = pIrp->IoStatus.Status;
	Length = pIrp->IoStatus.Information;
	pSendBuffer = (PSEND_BUFFER)Context;

	if(NT_SUCCESS(Status))
	{
		//This is successfull, so Just increment the Length
		pSendBuffer->ActualSendLength += Length;
		KdPrint(( "SftkEndpointSendRequestComplete: Send %d Bytes and Index = %d for SendBuffer = %lx \
				  TotalLength = %d , ActualLength = %d , and Session = %lx\n", 
				  Length, pSendBuffer->index , pSendBuffer,pSendBuffer->TotalSendLength, 
				  pSendBuffer->ActualSendLength, pSendBuffer->pSession));
	}
	else
	{
		pSendBuffer->state = SFTK_BUFFER_USED;
		pSendBuffer->ActualSendLength = 0;
		KdPrint(( "SftkTCPSSendBuffer: Status 0x%8.8X and Index = %d for SendBuffer = %lx\
				  TotalLength = %d , Session = %lx\n", 
				  Status , pSendBuffer->index , pSendBuffer, pSendBuffer->TotalSendLength,
				  pSendBuffer->pSession));
	}

	SFTK_TDI_UnlockAndFreeMdl(pSendBuffer->pSendMdl);
	pSendBuffer->pSendMdl = NULL;

	return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS SendThread2(PSESSION_MANAGER pSessionManager)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PTCP_SESSION pSession = NULL;
	PLIST_ENTRY pTempEntry;
	PSEND_BUFFER pSendBuffer = NULL;
	ULONG LenBuf =0;
	PPROTOCOL_PACKET pProtocolPacket = NULL;

	KdPrint(("Enter CreateSendThread1()\n"));
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
					//Send Lock
					ExAcquireFastMutexUnsafe(&pSessionManager->SendLock);

					pTemp = pSessionManager->ServerList.Flink;

					while(pTemp != &pSessionManager->ServerList && !bExit)
					{
						KdPrint(("Going for the Server List Also in CreateSendThread1()\n"));
						pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);

						ASSERT(pServerElement);

						//Cahnging the while to if
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
								pSendBuffer = NULL;
								//Check if there is some Buffer already Processing by the Session 
								//If So just Use the Session Buffer
								if(pSession->pSendBuffer != NULL && pSession->pSendBuffer->pSendMdl != NULL)
								{
									//The Session Buffer is Still getting Processed, So 
									//Just Goto the Next Session.
									pTemp = pTemp->Flink;
									continue;
								}
								else if(pSession->pSendBuffer != NULL)
								{
									if(pSession->pSendBuffer->ActualSendLength == pSession->pSendBuffer->TotalSendLength)
									{
										NdisZeroMemory(pSession->pSendBuffer,sizeof(SEND_BUFFER));
										pSession->pSendBuffer->state = SFTK_BUFFER_FREE;
										pSession->pSendBuffer = NULL;
										
									}
									pSendBuffer = pSession->pSendBuffer;
								}

								//The pSession->pSendBuffer is completely Processed so 
								//Check if there are any Stale Buffers on the Wire
								if(pSendBuffer == NULL)
								{
									pSendBuffer = GetNextStaleSendBuffer(&pSessionManager->sendBufferList);
								}

								if(pSendBuffer == NULL)
								{
//									KdPrint(("Coundnt Find a Stale Send Buffer So Calling GetNextSendBuffer\n"));
									pSendBuffer = GetNextSendBuffer(&pSessionManager->sendBufferList);
								
									if(pSendBuffer != NULL)
									{
										//Fill in Some Data or Do Something
										FillRawBufferRandom(pSendBuffer,&LenBuf);

										pSendBuffer->ActualSendLength =0;
										pSendBuffer->TotalSendLength = LenBuf;

										//Add the Protocol Object to the Queue
										//////////////////////////////////////////////////////////

										pProtocolPacket = (PPROTOCOL_PACKET)ExAllocateFromNPagedLookasideList(&pSessionManager->ProtocolList);

										//If there is no Memory for the Protocol header
										//Just Clean up and exit the Send Thread
										if(pProtocolPacket == NULL && pSendBuffer != NULL && pSendBuffer->pSendMdl != NULL)
										{
											KdPrint(("Unable to Allocate PROTOCOL_PACKET So Exiting the Send Thread\n"));
											SFTK_TDI_UnlockAndFreeMdl( pSendBuffer->pSendMdl );
											pSendBuffer->pSendMdl = NULL;
											pSendBuffer->state = SFTK_BUFFER_FREE;
											bExit = TRUE;
											leave;
										}
										else
										{
											//Get the Protocol Packet Object, initialize the IO Header and the ftd_header_t
											//then initialize the time this packet is added to the protocol Queue
											//Add the Packet to the ProtocolQueue
											NdisZeroMemory(pProtocolPacket,PROTOCOL_PACKET_SIZE);
											pProtocolPacket->pIoPacket = pSendBuffer->pIoPacket;
											NdisMoveMemory(&pProtocolPacket->ProtocolHeader,pSendBuffer->pSendBuffer,PROTOCOL_HEADER_SIZE);
											KeQuerySystemTime(&pProtocolPacket->tStartTime);

											KeInitializeEvent(&pProtocolPacket->ReceiveAckEvent,NotificationEvent,FALSE);

//											KeAcquireSpinLock(&pSessionManager->ProtocolQueueLock,&oldIrql);
											InsertHeadList(&pSessionManager->ProtocolQueue,&pProtocolPacket->ListElement);
//											KeReleaseSpinLock(&pSessionManager->ProtocolQueueLock,oldIrql);
										}

										//End of Add Protocol Header to the protocol Queue
										//////////////////////////////////////////////////////////
									}
									else
									{
										//30 Milli-Seconds in 100*Nano Seconds
										iWait.QuadPart = -(30*10*1000);
										KdPrint(("There are no more Buffers available to Send Data so Wait\n"));
										KeDelayExecutionThread(KernelMode,FALSE,&iWait);
										leave;
									}
								}

								if(pSendBuffer != NULL)
								{
									if(pSession->pSendBuffer == NULL)
									{
										pSession->pSendBuffer = pSendBuffer;
									}
									LenBuf = pSendBuffer->TotalSendLength - pSendBuffer->ActualSendLength;
									//Lock the Mdl
									if(LenBuf < pSendBuffer->pSendList->ChunkSize || pSendBuffer->pSendList->ChunkSize == 0)
									{
										pSendBuffer->pSendMdl = SFTK_TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer+pSendBuffer->ActualSendLength,LenBuf,FALSE,FALSE,NULL);
									}
									else
									{
										pSendBuffer->pSendMdl = SFTK_TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer+pSendBuffer->ActualSendLength,pSendBuffer->pSendList->ChunkSize,FALSE,FALSE,NULL);
									}
	
									ASSERT(pSendBuffer->pSendMdl);

									pSendBuffer->state = SFTK_BUFFER_USED;
									pSendBuffer->pSendMdl->Next = NULL;
									pSendBuffer->pSession = pSession;

									KdPrint(("The Session that is used to Send is %lx\n",pSession));

									try
									{
										//Reinitialize the IRP, incase of resuing it.
										if(pSendBuffer->pSendIrp != NULL)
										{
											IoReuseIrp(pSendBuffer->pSendIrp,STATUS_SUCCESS);
										}
										Status = SFTK_TDI_SendOnEndpoint1(
														&pSession->KSEndpoint,
														NULL,       // User Completion Event
														//SftkTCPCSendCompletion,       // User Completion Routine
														SftkEndpointSendRequestComplete2,
														pSendBuffer,       // User Completion Context
														&pSendBuffer->IoSendStatus,
														pSendBuffer->pSendMdl,
														0,           // Send Flags
														&pSendBuffer->pSendIrp
														);
									}
									except(EXCEPTION_EXECUTE_HANDLER)
									{
										KdPrint(("An Exception Occured in CreateSendThread1() Error Code is %lx\n",GetExceptionCode()));
										pSendBuffer->state = SFTK_BUFFER_USED;
										pSendBuffer->ActualSendLength =0;
									}
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
								iWait.QuadPart = -(10*10*1000);
								KeDelayExecutionThread(KernelMode,FALSE,&iWait);
							}

							iWait.QuadPart = -(10*10*1000);
							KeDelayExecutionThread(KernelMode,FALSE,&iWait);
						}	//if(pServerElement->pSession != NULL && !bExit)
						pTemp = pTemp->Flink;
					}	//while(pTemp != &pSessionManager->ServerList && !bExit)
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
					//Release the Send Lock
					ExReleaseFastMutexUnsafe(&pSessionManager->SendLock);
					//Release the Resource 
					ExReleaseResourceLite(&pSessionManager->ServerListLock);
					KeLeaveCriticalRegion();	//Enable the Kernel APC's
				}
			}//while(!bExit)
		}//try
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint(("An Exception Occured in CreateSendThread1() Error Code is %lx\n",GetExceptionCode()));
		}
	}//try
	finally
	{
		KdPrint(("Exiting the CreateSendThread1() Status = %lx\n",Status));
	}

	return PsTerminateSystemThread(Status);
}

#endif	//__NEW_SEND__