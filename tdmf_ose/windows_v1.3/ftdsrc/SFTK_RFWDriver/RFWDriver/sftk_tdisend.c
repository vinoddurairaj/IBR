/**************************************************************************************

Module Name: sftk_tdisend.c   
Author Name: Veera Arja
Description: Describes Modules: Protocol and CommunicationFunctions
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#include "sftk_main.h"


NTSTATUS
TDI_EndpointSendRequestComplete2(
							IN PDEVICE_OBJECT pDeviceObject,
							IN PIRP pIrp,
							IN PVOID Context
							)
{
	PSEND_BUFFER pSendBuffer			= NULL;
	PSESSION_MANAGER pSessionManager	= NULL;
	PTCP_SESSION pSession				= NULL;
	NTSTATUS status						= STATUS_SUCCESS;
	ULONG Length						= 0;
	KIRQL oldIrql;
	PMDL pMdl							= NULL;
	PSFTK_LG pSftkLg					= NULL;
//	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];
	
	status = pIrp->IoStatus.Status;
	Length = pIrp->IoStatus.Information;
	pSendBuffer = (PSEND_BUFFER)Context;

	// Just to make sure
	OS_ASSERT(pSendBuffer != NULL);
	OS_ASSERT(pSendBuffer->pSession != NULL);
	OS_ASSERT(pSendBuffer->pSession->pServer != NULL);
	OS_ASSERT(pSendBuffer->pSession->pServer->pSessionManager != NULL);
	OS_ASSERT(pSendBuffer->pSession->pServer->pSessionManager->pLogicalGroupPtr != NULL);

	pSessionManager = pSendBuffer->pSession->pServer->pSessionManager;
	pSession = pSendBuffer->pSession;
	pSftkLg = pSessionManager->pLogicalGroupPtr;


	TDI_UnlockAndFreeMdl(pSendBuffer->pSendMdl);
	pSendBuffer->pSendMdl = NULL;

	if(NT_SUCCESS(status)){
		//This is successfull, so Just increment the Length
		pSendBuffer->ActualSendLength += Length;

		DebugPrint((DBG_SEND, "TDI_EndpointSendRequestComplete2(): Send %d Bytes and Index = %d for SendBuffer = %lx \
				  TotalLength = %d , ActualLength = %d , and Session = %lx\n", 
				  Length, pSendBuffer->index , pSendBuffer,pSendBuffer->TotalSendLength, 
				  pSendBuffer->ActualSendLength, pSession));

		if(pSendBuffer->ActualSendLength == pSendBuffer->TotalSendLength){
#if 0 // DBG
			{
				ULONG i;
				ftd_header_t * pProtoHdr = (ftd_header_t *) pSendBuffer->pSendBuffer;

				for (i=0; i < pSendBuffer->TotalSendLength;)
				{
					DebugPrint((DBG_PROTO, "### TDI_EndpointSendRequestComplete2(): %d SendComplete:MsgId %d,devid:0x%08x,O:%d,len:%d, Length:%d, Buffer:0x%08x ###\n",
								i, pProtoHdr->msgtype, pProtoHdr->msg.lg.devid, pProtoHdr->msg.lg.offset, 
								pProtoHdr->msg.lg.len,pSendBuffer->TotalSendLength, pSendBuffer->pSendBuffer));

					i += (sizeof(ftd_header_t) + (pProtoHdr->msg.lg.len << DEV_BSHIFT));
					pProtoHdr = (ftd_header_t *) ((ULONG) pProtoHdr + (sizeof(ftd_header_t) + (pProtoHdr->msg.lg.len << DEV_BSHIFT)));
				}
			}
#endif

			//pSendBuffer->pSession->pSendBuffer = NULL;

			// Check the Cancel Flag before we Acquire the Spin Lock because we dont want to acquire 
			// the Spinlock if we are cancelling the IRP, with IoCancelIrp(), This completion routinue 
			// will be called by the IoCancelIrp() function and should not acquire the spcinlock again
			// because in that case will have a deadlock.

			// Increment all the counters of the Session
			// Increment Send Performance Metrics

			// Acquire the Spin Lock before incrementing the Statistics.


			KeAcquireSpinLock(&pSessionManager->StatisticsLock , &oldIrql);

			pSession->pServer->SessionPerformanceInformation.nPacketsSent++;
			pSession->pServer->SessionPerformanceInformation.nBytesSent += pSendBuffer->ActualSendLength;
			// Set the Minimum Send Packet Size
			if(pSession->pServer->SessionPerformanceInformation.nMinimumSendPacketSize  > pSendBuffer->ActualSendLength){
				pSession->pServer->SessionPerformanceInformation.nMinimumSendPacketSize = pSendBuffer->ActualSendLength;
			}
			// Set the Maximum Send Packet Size
			if(pSession->pServer->SessionPerformanceInformation.nMaximumSendPacketSize  < pSendBuffer->ActualSendLength){
				pSession->pServer->SessionPerformanceInformation.nMaximumSendPacketSize = pSendBuffer->ActualSendLength;
			}
			// Calculate the Average Send Packet Size
			pSession->pServer->SessionPerformanceInformation.nAverageSendPacketSize = (ULONG)(pSession->pServer->SessionPerformanceInformation.nBytesSent / pSession->pServer->SessionPerformanceInformation.nPacketsSent);
			// Update all the counters of the SESSION_MANAGER
			pSftkLg->Statistics.SM_PacketsSent++;
			pSftkLg->Statistics.SM_BytesSent += pSendBuffer->ActualSendLength;
			if(pSftkLg->Statistics.SM_MinimumSendPacketSize > pSendBuffer->ActualSendLength){
				pSftkLg->Statistics.SM_MinimumSendPacketSize = pSendBuffer->ActualSendLength; 
			}
			if(pSftkLg->Statistics.SM_MaximumSendPacketSize < pSendBuffer->ActualSendLength){
				pSftkLg->Statistics.SM_MaximumSendPacketSize = pSendBuffer->ActualSendLength;
			}
			pSftkLg->Statistics.SM_AverageSendPacketSize = (ULONG)(pSftkLg->Statistics.SM_BytesSent/pSftkLg->Statistics.SM_PacketsSent);
			KeReleaseSpinLock(&pSessionManager->StatisticsLock , oldIrql);
			// Release the Spin Lock after the increment operation

			if(pSendBuffer->nCancelFlag == 0){
				// Remove this Element from the Send List once it is done processing
				KeAcquireSpinLock(&pSession->sendIrpListLock , &oldIrql);
				if(pSendBuffer->nCancelFlag == 0){
					RemoveEntryList(&pSendBuffer->SessionListElemet);

					//Free up the Resource if the Memory is allocated from the System
					if(pSendBuffer->bPhysicalMemory){
						// Remove this SEND_BUFFER from the List Because this is allocated
						// from the System Memory and is not part of the SEND_BUFFER_LIST
						KeAcquireSpinLockAtDpcLevel(&pSendBuffer->pSendList->SendBufferListLock);
						RemoveEntryList(&pSendBuffer->ListElement);
						KeReleaseSpinLockFromDpcLevel(&pSendBuffer->pSendList->SendBufferListLock);

						// Release the IRP 
						if(pSendBuffer->pSendIrp != NULL){
							OS_ASSERT(pSendBuffer->pSendIrp == pIrp);
							IoFreeIrp(pSendBuffer->pSendIrp);
							pSendBuffer->pSendIrp = NULL;
						}
						if(pSendBuffer->pSendBuffer != NULL){
							NdisFreeMemory(pSendBuffer->pSendBuffer,pSendBuffer->ActualSendLength,0);
							pSendBuffer->pSendBuffer = NULL;
						}
						NdisFreeMemory(pSendBuffer,sizeof(SEND_BUFFER),0);
						pSendBuffer = NULL;
					}
				}
				KeReleaseSpinLock(&pSession->sendIrpListLock , oldIrql);
			}
			if(pSendBuffer != NULL && !pSendBuffer->bPhysicalMemory){
				// Reset the Cancel Falg
				pSendBuffer->nCancelFlag = 0;
				if(pSendBuffer->pSendIrp != NULL){
					// Free up The IRP.
					OS_ASSERT(pSendBuffer->pSendIrp == pIrp);
					IoFreeIrp(pSendBuffer->pSendIrp);
					pSendBuffer->pSendIrp = NULL;
				}
				pSendBuffer->state = SFTK_BUFFER_FREE;

				// Set this Event so that any threads that are waiting for the Buffers will Wakeup and 
				// Send Data.
				KeSetEvent(&pSessionManager->IOSendPacketsAvailableEvent,0, FALSE);
			}
		}else{
			DebugPrint((DBG_ERROR, "TDI_EndpointSendRequestComplete2(): Shouldnt Have come Here doesnt support NonBlocking Send %d Bytes and Index = %d for SendBuffer = %lx \
					TotalLength = %d , ActualLength = %d , and Session = %lx\n", 
					Length, pSendBuffer->index , pSendBuffer,pSendBuffer->TotalSendLength, 
					pSendBuffer->ActualSendLength, pSession));
			OS_ASSERT(FALSE);
		}
	}else{
		pSendBuffer->ActualSendLength = 0;

		DebugPrint((DBG_SEND, "TDI_EndpointSendRequestComplete2(): status 0x%8.8X and Index = %d for SendBuffer = %lx\
				  TotalLength = %d , Session = %lx\n", 
				  status , pSendBuffer->index , pSendBuffer, pSendBuffer->TotalSendLength,
				  pSession));

		// Remove this Element from the Send List once it is done processing
		// Check the Cancel Flag before we Acquire the Spin Lock because we dont want to acquire 
		// the Spinlock if we are cancelling the IRP, with IoCancelIrp(), This completion routinue 
		// will be called by the IoCancelIrp() function and should not acquire the spcinlock again
		// because in that case will have a deadlock.

		if(pSendBuffer->nCancelFlag == 0){
			KeAcquireSpinLock(&pSession->sendIrpListLock , &oldIrql);
			if(pSendBuffer->nCancelFlag == 0){
				RemoveEntryList(&pSendBuffer->SessionListElemet);
			}
			KeReleaseSpinLock(&pSession->sendIrpListLock , oldIrql);
		}

		if(pSendBuffer != NULL){
			// Reset the Cancel Falg
			pSendBuffer->nCancelFlag = 0;
			if(pSendBuffer->pSendIrp != NULL){
				OS_ASSERT(pSendBuffer->pSendIrp == pIrp);
				// Free up The IRP.
				IoFreeIrp(pSendBuffer->pSendIrp);
				pSendBuffer->pSendIrp = NULL;
			}
			pSendBuffer->state = SFTK_BUFFER_USED;
			// Set this Event so that any threads that are waiting for the Buffers will Wakeup and 
			// Send Data.
			KeSetEvent(&pSessionManager->IOSendPacketsAvailableEvent,0, FALSE);
		}
		// Reset the Particular Session
		pSession->pServer->bReset = TRUE;
	}
	return STATUS_MORE_PROCESSING_REQUIRED;
}//TDI_EndpointSendRequestComplete2()


VOID 
COM_FillRawBufferRandom(
				PSEND_BUFFER pSendBuffer, 
				PULONG pLen
				)
{

	ftd_header_t	*pProtoHdr;
	LARGE_INTEGER	currentTime;

	NdisZeroMemory(pSendBuffer->pSendBuffer,pSendBuffer->SendWindow);
	*pLen = pSendBuffer->SendWindow - 512;	// Get one block Less
	
	pProtoHdr =	((ftd_header_t*)pSendBuffer->pSendBuffer);
	pProtoHdr->magicvalue			= MAGICHDR;
	OS_PerfGetClock(&currentTime, NULL);
	pProtoHdr->ts					= (LONG) (currentTime.QuadPart / (ULONG)(10*1000*1000));
	pProtoHdr->msgtype				= FTDCRFBLK;
	pProtoHdr->cli					= 0;
	pProtoHdr->compress				= 0;
	pProtoHdr->len					= (ULONG) (*pLen);
	pProtoHdr->uncomplen			= *pLen;
	pProtoHdr->ackwanted			= 1;
	pProtoHdr->msg.lg.lgnum			= 0;
	pProtoHdr->msg.lg.devid			= 0;
	pProtoHdr->msg.lg.bsize			= 0;
	pProtoHdr->msg.lg.offset		= (ULONG) (5120 >> DEV_BSHIFT);
	pProtoHdr->msg.lg.len			= (ULONG) (*pLen >> DEV_BSHIFT);
	pProtoHdr->msg.lg.data			= 0;
	pProtoHdr->msg.lg.flags			= 0;
	pProtoHdr->msg.lg.reserved		= 0;

	*pLen = *pLen + sizeof(ftd_header_t);
}
//Sends Header and Data 
//Will stop the Send Thread, and then induce the Protocol Packet.
NTSTATUS 
COM_SendVector(
			IN SFTK_IO_VECTOR vectorArray[] , 
			IN LONG len , 
			IN PSESSION_MANAGER pSessionManager
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	int index =0;
	PSEND_BUFFER pSendBuffer = NULL;
	int retrycount = 10;
	LARGE_INTEGER iWait;

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);
	OS_ASSERT(vectorArray != NULL);

	DebugPrint((DBG_SEND, "COM_SendVector(): Entering COM_SendVector() the number of elements = %d\n",len));

	//Check if the Length is wrong
	if(len <= 0)
	{
		DebugPrint((DBG_ERROR, "COM_SendVector(): Invalid Parameter the length is Zero\n"));
		return STATUS_INVALID_PARAMETER;
	}

	// Check if the SessionManager is initialized and have some active connections
	if( ( !pSessionManager->bInitialized ) || 
		( pSessionManager->nLiveSessions <= 0 ) || 
		( pSessionManager->Reset == TRUE ) ||
		( pSessionManager->Stop == TRUE) )
	{
		DebugPrint((DBG_ERROR, "COM_SendBufferVector(): Cannot send any Packets if there are no Active Connections\n"));
		return STATUS_INVALID_PARAMETER;
	}

	try
	{
		iWait.QuadPart = -(30*10*1000);
//		KeEnterCriticalRegion();	//Disable the Kernel APC's
//		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		//Get the Send Buffer from the Queue

		while(retrycount >0)
		{
			pSendBuffer = COM_GetNextSendBuffer(&pSessionManager->sendBufferList);

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
			status =  STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		pSendBuffer->ActualSendLength = 0;
		pSendBuffer->TotalSendLength =0;

		for(index =0 ; index < len ; index++)
		{
			NdisMoveMemory(pSendBuffer->pSendBuffer+pSendBuffer->TotalSendLength,vectorArray[index].pBuffer,vectorArray[index].uLength);
			pSendBuffer->TotalSendLength += vectorArray[index].uLength;
		}

		// Start  the Protocol Part Here  TODO VEERA
		// End the Protocol Part Here  TODO VEERA

		pSendBuffer->bPhysicalMemory = FALSE;
		pSendBuffer->state = SFTK_BUFFER_USED;

	}
	finally
	{
		if(!NT_SUCCESS(status))
		{
			if(pSendBuffer != NULL)
			{
				pSendBuffer->state = SFTK_BUFFER_FREE;
			}
		}

		//Release the Resource 
//		ExReleaseResourceLite(&pSessionManager->ServerListLock);
//		KeLeaveCriticalRegion();	//Enable the Kernel APC's
		DebugPrint((DBG_SEND, "COM_SendVector(): Exiting the COM_SendVector() status = %lx\n",status));
	}
return status;
}//COM_SendVector()

NTSTATUS 
COM_SendBufferVector(
			IN SFTK_IO_VECTOR vectorArray[] , 
			IN LONG				len , 
			IN PSESSION_MANAGER pSessionManager
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	int index =0;
	PSEND_BUFFER pSendBuffer = NULL;
	LONG nTempLength = 0;
//	KIRQL oldIrql;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);
	OS_ASSERT(vectorArray != NULL);

	DebugPrint((DBG_SEND, "COM_SendBufferVector(): Entering COM_SendBufferVector() the number of elements = %d\n",len));

	//Check if the Length is wrong
	if(len <= 0)
	{
		DebugPrint((DBG_ERROR, "COM_SendBufferVector(): Invalid Parameter the length is Zero\n"));
		return STATUS_INVALID_PARAMETER;
	}

	// Check if the SessionManager is initialized and have some active connections
	if( ( !pSessionManager->bInitialized ) || 
		( pSessionManager->nLiveSessions <= 0 ) || 
		( pSessionManager->Reset == TRUE ) || 
		( pSessionManager->Stop == TRUE ) )
	{
		DebugPrint((DBG_ERROR, "COM_SendBufferVector(): Cannot send any Packets if there are no Active Connections\n"));
		return STATUS_INVALID_PARAMETER;
	}

	try
	{
//		KeEnterCriticalRegion();	//Disable the Kernel APC's
//		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		status = NdisAllocateMemoryWithTag(&pSendBuffer,sizeof(SEND_BUFFER),'AREV');

		if(!NT_SUCCESS(status))
		{
			swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
			swprintf(wchar2,L"%S",L"COM_SendBufferVector::SEND_BUFFER");
			swprintf(wchar3,L"%d",sizeof(SEND_BUFFER));
			swprintf(wchar4,L"0X%08X",status);
			sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_COM_MEMORY_ALLOCATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

			DebugPrint((DBG_ERROR, "COM_SendBufferVector(): Unable to Allocte Memory for SEND_BUFFER \n"));
			leave;
		}

		NdisZeroMemory(pSendBuffer,sizeof(SEND_BUFFER));

		pSendBuffer->ActualSendLength = 0;
		pSendBuffer->TotalSendLength =0;

		for(index =0 ; index < len ; index++)
		{
			pSendBuffer->TotalSendLength += vectorArray[index].uLength;
		}

		status = NdisAllocateMemoryWithTag(&pSendBuffer->pSendBuffer, pSendBuffer->TotalSendLength, 'AREV');

		if(!NT_SUCCESS(status))
		{
			swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
			swprintf(wchar2,L"%S",L"COM_SendBufferVector::pSendBuffer->pSendBuffer");
			swprintf(wchar3,L"%d",pSendBuffer->TotalSendLength);
			swprintf(wchar4,L"0X%08X",status);
			sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_COM_MEMORY_ALLOCATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

			DebugPrint((DBG_ERROR, "COM_SendBufferVector(): Unable to Allocte Memory for Send Buffer \n"));
			leave;
		}

		NdisZeroMemory(pSendBuffer->pSendBuffer, pSendBuffer->TotalSendLength);

		nTempLength = 0;
		for(index =0 ; index < len ; index++)
		{
			NdisMoveMemory(pSendBuffer->pSendBuffer+nTempLength,vectorArray[index].pBuffer,vectorArray[index].uLength);
			nTempLength +=vectorArray[index].uLength;
		}

		// Start  the Protocol Part Here  TODO VEERA
		// End the Protocol Part Here  TODO VEERA

		// Set the Flag indicating that the Memory is allocated from the system, and hence needs to be 
		// Freed in the Completion Routinue.

		pSendBuffer->bPhysicalMemory = TRUE;
		pSendBuffer->state = SFTK_BUFFER_USED;

		COM_InsertBufferIntoSendListTail(&pSessionManager->sendBufferList , pSendBuffer);
		//TODO Has to takecare of the protocol Part
	}
	finally
	{
		if(!NT_SUCCESS(status))
		{
			if(pSendBuffer != NULL)
			{
				COM_ClearSendBuffer(pSendBuffer);
				pSendBuffer = NULL;
			}
		}
		//Release the Resource 
//		ExReleaseResourceLite(&pSessionManager->ServerListLock);
//		KeLeaveCriticalRegion();	//Enable the Kernel APC's
		DebugPrint((DBG_SEND, "COM_SendBufferVector(): Exiting the COM_SendBufferVector() status = %lx\n",status));
	}
	return status;
}//COM_SendBufferVector()

NTSTATUS 
COM_SendBuffer(
			IN PUCHAR			lpBuffer , 
			IN LONG				len , 
			IN PSESSION_MANAGER pSessionManager
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSEND_BUFFER pSendBuffer = NULL;
//	KIRQL oldIrql;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);
	OS_ASSERT(lpBuffer != NULL);

	DebugPrint((DBG_SEND, "COM_SendBuffer(): Entering COM_SendBuffer() the number of elements = %d\n",len));

	//Check if the Length is wrong
	if(len <= 0)
	{
		DebugPrint((DBG_ERROR, "COM_SendBuffer(): Invalid Parameter the length is Zero\n"));
		return STATUS_INVALID_PARAMETER;
	}

	if( ( !pSessionManager->bInitialized ) || 
		( pSessionManager->nLiveSessions <= 0 ) || 
		( pSessionManager->Reset == TRUE ) || 
		( pSessionManager->Stop == TRUE ) )
	{
		DebugPrint((DBG_ERROR, "COM_SendBuffer(): Cannot send any Packets if there are no Active Connections\n"));
		return STATUS_INVALID_PARAMETER;
	}
	
	try
	{
//		KeEnterCriticalRegion();	//Disable the Kernel APC's
//		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		status = NdisAllocateMemoryWithTag(&pSendBuffer,sizeof(SEND_BUFFER),'AREV');

		if(!NT_SUCCESS(status))
		{
			swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
			swprintf(wchar2,L"%S",L"COM_SendBuffer::SEND_BUFFER");
			swprintf(wchar3,L"%d",sizeof(SEND_BUFFER));
			swprintf(wchar4,L"0X%08X",status);
			sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_COM_MEMORY_ALLOCATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

			DebugPrint((DBG_ERROR, "COM_SendBuffer(): Unable to Allocte Memory for SEND_BUFFER \n"));
			leave;
		}

		NdisZeroMemory(pSendBuffer,sizeof(SEND_BUFFER));

		pSendBuffer->ActualSendLength = 0;
		pSendBuffer->TotalSendLength = len;


		status = NdisAllocateMemoryWithTag(&pSendBuffer->pSendBuffer, pSendBuffer->TotalSendLength, 'AREV');

		if(!NT_SUCCESS(status))
		{
			swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
			swprintf(wchar2,L"%S",L"COM_SendBuffer::pSendBuffer->pSendBuffer");
			swprintf(wchar3,L"%d",pSendBuffer->TotalSendLength);
			swprintf(wchar4,L"0X%08X",status);
			sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_COM_MEMORY_ALLOCATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

			DebugPrint((DBG_ERROR, "COM_SendBuffer(): Unable to Allocte Memory for Send Buffer \n"));
			leave;
		}

		NdisZeroMemory(pSendBuffer->pSendBuffer, pSendBuffer->TotalSendLength);
		NdisMoveMemory(pSendBuffer->pSendBuffer , lpBuffer,pSendBuffer->TotalSendLength);

		// Start  the Protocol Part Here  TODO VEERA
		// End the Protocol Part Here  TODO VEERA

		// Set the Flag indicating that the Memory is allocated from the system, and hence needs to be 
		// Freed in the Completion Routinue.

		pSendBuffer->bPhysicalMemory = TRUE;
		pSendBuffer->state = SFTK_BUFFER_USED;

		COM_InsertBufferIntoSendListTail(&pSessionManager->sendBufferList , pSendBuffer);
		//TODO Has to takecare of the protocol Part
	}
	finally
	{
		if(!NT_SUCCESS(status))
		{
			if(pSendBuffer != NULL)
			{
				COM_ClearSendBuffer(pSendBuffer);
				pSendBuffer = NULL;
			}
		}
		//Release the Resource 
//		ExReleaseResourceLite(&pSessionManager->ServerListLock);
//		KeLeaveCriticalRegion();	//Enable the Kernel APC's
		DebugPrint((DBG_SEND, "COM_SendBuffer(): Exiting the COM_SendBuffer() status = %lx\n",status));
	}
return status;
}//COM_SendBuffer()


//Code with Optimizations
#define __NEW_SEND__
#ifdef __NEW_SEND__



NTSTATUS 
COM_SendThread2(
		PSESSION_MANAGER pSessionManager
		)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PSEND_BUFFER pSendBuffer = NULL;
	ULONG LenBuf =0;
	KIRQL oldIrql;
	LARGE_INTEGER iCurrentTime;
	ULONG uCurTime = 0;
	PKSEMAPHORE pSendBufferWaitEvent = NULL;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];
	PSFTK_LG pSftkLg = NULL;
	PMM_HOLDER pRootProtoMmHolderPtr = NULL;	// This will be used for direct Memory Access while sending the data

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	DebugPrint((DBG_SEND, "COM_SendThread2(): Enter COM_SendThread2()\n"));

	pSftkLg = pSessionManager->pLogicalGroupPtr;

	KeQuerySystemTime(&iCurrentTime);
	uCurTime = (ULONG)(iCurrentTime.QuadPart/(10*1000*1000));
	pSessionManager->LastPacketSentTime = uCurTime;

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	swprintf(wchar3,L"%S",L"COM_SendThread2");
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

					pTemp = pSessionManager->ServerList.Flink;

					while(pTemp != &pSessionManager->ServerList && !bExit)
					{
//						DebugPrint((DBG_SEND, "COM_SendThread2(): Going for the Server List Also in COM_SendThread2()\n"));
						pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);
						pTemp = pTemp->Flink;

						OS_ASSERT(pServerElement != NULL);
						OS_ASSERT(pServerElement->pSession != NULL);

						//Cahnging the while to if
						if(pServerElement->pSession != NULL && !bExit)
						{
							OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

							status = KeWaitForSingleObject(&pSessionManager->IOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
							if(status == STATUS_SUCCESS)
							{
								swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
								swprintf(wchar2,L"0X%08X",pSessionManager);
								sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SEND_THREAD_EXITING,status,0,wchar1,wchar2);

								DebugPrint((DBG_SEND, "COM_SendThread2(): The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
								bExit = TRUE;
								break;
							}

							if(pServerElement->pSession->bSessionEstablished == SFTK_CONNECTED)
							{
								pSendBuffer = NULL;

								//The pServerElement->pSession->pSendBuffer is completely Processed so 
								//Check if there are any Stale Buffers on the Wire
								if(pSendBuffer == NULL)
								{
									pSendBuffer = COM_GetNextStaleSendBuffer(&pSessionManager->sendBufferList);
								}

								if(pSendBuffer == NULL)
								{
									// If we didnt send the handshake Information dont go for the Refresh IO
									// or Commit IO
									if(pSessionManager->bSendHandshakeInformation == TRUE)
									{
//										DebugPrint((DBG_SEND, "COM_SendThread2(): The Handshake information is not sent so we cannot send the other Data\n"));
										iWait.QuadPart = -(1000*10*1000);
										KeDelayExecutionThread(KernelMode,FALSE,&iWait);
										leave;
									}
//									KdPrint(("Coundnt Find a Stale Send Buffer So Calling COM_GetNextSendBuffer\n"));
									pSendBuffer = COM_GetNextSendBuffer(&pSessionManager->sendBufferList);
								
									if(pSendBuffer != NULL)
									{
#define _PARAGS_CODE_
#ifdef _PARAGS_CODE_
										// Get the Lenght from the SendWindow
										pSendBuffer->TotalSendLength = pSendBuffer->SendWindow;

										if(pSessionManager->UseMemoryManager == TRUE){
											// Use the Memory Manager Data Directly for sending over the wire.
											if(!QM_RetrievePkts(
												pSessionManager->pLogicalGroupPtr, 
												pSendBuffer, 
												&pSendBuffer->TotalSendLength,	// In Buffer Max size, OUT actual buffer data filled size
												TRUE,
												&pRootProtoMmHolderPtr,
												&pSendBufferWaitEvent)){
												/*
												swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
												swprintf(wchar2,L"0X%08X",pSessionManager);
												sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SEND_DISPATCH_QUEUE_IS_EMPTY,STATUS_SUCCESS,0,wchar1,wchar2);
												*/

												DebugPrint((DBG_SEND, "COM_SendThread2(): The QM_RetrievePkts() returned FALSE, So NO Packets available to send\n"));
												// If there is no data to be read then just Sleep for sometime and then try again.
												pSendBuffer->state = SFTK_BUFFER_FREE;

												iWait.QuadPart = DEFAULT_TIMEOUT_FOR_PACKETRETRIVAL_WAIT; // -(10*1000*1000)
												status = KeWaitForSingleObject(&pSftkLg->EventPacketsAvailableForRetrival,Executive,KernelMode,FALSE,&iWait);
												leave;
											}// while() loop until we get the Packets
										}else{
											// We need to get the Packets from the Migrate Queue and fill up the buffer
											if(!QM_RetrievePkts(
												pSessionManager->pLogicalGroupPtr, 
												(PVOID)pSendBuffer->pSendBuffer, 
												&pSendBuffer->TotalSendLength,	// In Buffer Max size, OUT actual buffer data filled size
												FALSE,
												NULL,
												&pSendBufferWaitEvent)){
												/*
												swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
												swprintf(wchar2,L"0X%08X",pSessionManager);
												sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SEND_DISPATCH_QUEUE_IS_EMPTY,STATUS_SUCCESS,0,wchar1,wchar2);
												*/

												DebugPrint((DBG_SEND, "COM_SendThread2(): The QM_RetrievePkts() returned FALSE, So NO Packets available to send\n"));
												// If there is no data to be read then just Sleep for sometime and then try again.
												pSendBuffer->state = SFTK_BUFFER_FREE;

												iWait.QuadPart = DEFAULT_TIMEOUT_FOR_PACKETRETRIVAL_WAIT; // -(10*1000*1000)
												status = KeWaitForSingleObject(&pSftkLg->EventPacketsAvailableForRetrival,Executive,KernelMode,FALSE,&iWait);
												leave;
											}// while() loop until we get the Packets
										}

#else
										if(0)
										{
											// This is the regular testing code that will be used by the sample driver
											// Fill in Some Data or Do Something
											COM_FillRawBufferRandom(pSendBuffer,&LenBuf);

											pSendBuffer->ActualSendLength =0;
											pSendBuffer->TotalSendLength = LenBuf;
										}
										else
										{
											// For the timebeing just set the Buffer to Free, 
											// Sleep for a few seconds and continue;

											iWait.QuadPart = -(100*10*1000);
											KeDelayExecutionThread(KernelMode,FALSE,&iWait);
											pSendBuffer->state = SFTK_BUFFER_FREE;

											leave;
										}
#endif

										//Add the Protocol Object to the Queue TODO VEERA
										//End of Add Protocol Header to the protocol Queue TODO VEERA
									}
									else
									{
										/*
										swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
										swprintf(wchar2,L"0X%08X",pSessionManager);
										sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SEND_BUFFER_LIST_IS_EMPTY,STATUS_SUCCESS,0,wchar1,wchar2);
										*/

										DebugPrint((DBG_SEND, "COM_SendThread2(): The pSendBuffer is NULL, so NO SendBuffers are available to send Packets\n"));
										iWait.QuadPart = DEFAULT_TIMEOUT_FOR_SENDBUFFER_AVAILABILITY_WAIT; // -(10*1000*1000)
										status = KeWaitForSingleObject(&pSessionManager->IOSendPacketsAvailableEvent,Executive,KernelMode,FALSE,&iWait);
										leave;
									}
								}

								if(pSendBuffer != NULL)
								{
									// Set the pServerElement->pSession->pSendBuffer, to the Current Buffer, either if
									pServerElement->pSession->pSendBuffer = pSendBuffer;

									// Insert the newly sent Buffer to the sendList. this will keep tarck of 
									// All the Buffers that are either sent or in pending on this session.
									// We can use this list to cancel any IRP's that are not yet completed 
									// on this session.

									KeAcquireSpinLock(&pServerElement->pSession->sendIrpListLock , &oldIrql);
									InsertHeadList(&pServerElement->pSession->sendIrpList , &pSendBuffer->SessionListElemet);
									KeReleaseSpinLock(&pServerElement->pSession->sendIrpListLock , oldIrql);

									LenBuf = pSendBuffer->TotalSendLength - pSendBuffer->ActualSendLength;
									//Lock the Mdl
									pSendBuffer->pSendMdl = TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer+pSendBuffer->ActualSendLength,LenBuf,FALSE,FALSE,NULL);
	
									OS_ASSERT(pSendBuffer->pSendMdl);

									// Dont Know wht
									// pSendBuffer->state = SFTK_BUFFER_USED;
									pSendBuffer->pSendMdl->Next = NULL;
									pSendBuffer->pSession = pServerElement->pSession;

//									DebugPrint((DBG_SEND, "COM_SendThread2(): The Session that is used to Send is %lx\n",pServerElement->pSession));

									try
									{
										KeQuerySystemTime(&iCurrentTime);
										uCurTime = (ULONG)(iCurrentTime.QuadPart/(10*1000*1000));
										pSessionManager->LastPacketSentTime = uCurTime;
										//Reinitialize the IRP, incase of resuing it.
										//Lets try without Setting the Reuse IRP
//										if(pSendBuffer->pSendIrp != NULL)
//										{
//											IoReuseIrp(pSendBuffer->pSendIrp,STATUS_SUCCESS);
//										}
#if 0 // DBG
			{
				ULONG i;
				ftd_header_t * pProtoHdr = (ftd_header_t *) pSendBuffer->pSendBuffer;

				for (i=0; i < LenBuf;)
				{
					DebugPrint((DBG_PROTO, "---- TDI_Sending: %d SendComplete:MsgId %d,devid:0x%08x,O:%d,len:%d, Length:%d, Buffer:0x%08x ---\n",
								i, pProtoHdr->msgtype, pProtoHdr->msg.lg.devid, pProtoHdr->msg.lg.offset, 
								pProtoHdr->msg.lg.len,LenBuf, pSendBuffer->pSendBuffer));

					i += (sizeof(ftd_header_t) + (pProtoHdr->msg.lg.len << DEV_BSHIFT));
					pProtoHdr = (ftd_header_t *) ((ULONG) pProtoHdr + (sizeof(ftd_header_t) + (pProtoHdr->msg.lg.len << DEV_BSHIFT)));
				}
			}
#endif
										DebugPrint((DBG_SEND, "COM_SendThread2(): Calling TDI_SendOnEndpoint1() to send packet of length %d with index %d and Session %lx\n", LenBuf , pSendBuffer->index , pSendBuffer->pSession));

										status = TDI_SendOnEndpoint1(
														&pServerElement->pSession->KSEndpoint,
														NULL,       // User Completion Event
														TDI_EndpointSendRequestComplete2,
														pSendBuffer,       // User Completion Context
														&pSendBuffer->IoSendStatus,
														pSendBuffer->pSendMdl,
														TDI_SEND_NO_RESPONSE_EXPECTED | TDI_SEND_NON_BLOCKING,           // Send Flags
														&pSendBuffer->pSendIrp
														);

										if(	(status == STATUS_INSUFFICIENT_RESOURCES) && 
											(pSendBuffer->pSendIrp == NULL))
										{
											swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
											swprintf(wchar2,L"0X%08X",pServerElement);
											swprintf(wchar3,L"0X%08X",status);
											sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SEND_THREAD_EXCEPTION,status,0,wchar1,wchar2,wchar3);

											DebugPrint((DBG_ERROR, "COM_SendThread2(): An Exception Occured in COM_SendThread2() Error Code is %lx\n",status));
											// Failed to Allocate IRP hence just set the Buffer to Free 
											pSendBuffer->state = SFTK_BUFFER_USED;
											pSendBuffer->ActualSendLength =0;
										}
									}
									except(EXCEPTION_EXECUTE_HANDLER)
									{
										swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
										swprintf(wchar2,L"0X%08X",pServerElement);
										swprintf(wchar3,L"0X%08X",GetExceptionCode());
										sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SEND_THREAD_EXCEPTION,GetExceptionCode(),0,wchar1,wchar2,wchar3);

										DebugPrint((DBG_ERROR, "COM_SendThread2(): An Exception Occured in COM_SendThread2() Error Code is %lx\n",GetExceptionCode()));
										pSendBuffer->state = SFTK_BUFFER_USED;
										pSendBuffer->ActualSendLength =0;
									}
								}
							}
							else
							{
								if(pSessionManager->nLiveSessions <= 0)
								{
									swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
									swprintf(wchar2,L"0X%08X",pSessionManager);
									sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_NO_SESSION_IS_ALIVE,STATUS_SUCCESS,0,wchar1,wchar2);

									DebugPrint((DBG_SEND, "COM_SendThread2(): There are no live sessions so exiting\n"));
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
					}	//while(pTemp != &pSessionManager->ServerList && !bExit)

					if(!bExit)
					{
						OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

						status = KeWaitForSingleObject(&pSessionManager->IOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
						if(status == STATUS_SUCCESS)
						{
							swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
							swprintf(wchar2,L"0X%08X",pSessionManager);
							sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SEND_THREAD_EXITING,status,0,wchar1,wchar2);

							DebugPrint((DBG_SEND, "COM_SendThread2(): The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}
					}
				}//try
				finally
				{
					//Release the Send Lock
					ExReleaseResourceLite(&pSessionManager->ServerListLock);
					KeLeaveCriticalRegion();	//Enable the Kernel APC's
				}
			}//while(!bExit)
		}//try
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			DebugPrint((DBG_ERROR, "COM_SendThread2(): An Exception Occured in COM_SendThread2() Error Code is %lx\n",GetExceptionCode()));
		}
	}//try
	finally
	{
		DebugPrint((DBG_SEND, "COM_SendThread2(): Exiting the COM_SendThread2() status = %lx\n",status));
	}

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	swprintf(wchar3,L"%S",L"COM_SendThread2");
	sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_STOPPED,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);


	return PsTerminateSystemThread(status);
}


NTSTATUS 
COM_SendThreadForServer(
		PSERVER_ELEMENT pServerElement
		)
{
	NTSTATUS status = STATUS_SUCCESS;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PSEND_BUFFER pSendBuffer = NULL;
	ULONG LenBuf =0;
	KIRQL oldIrql;
	LARGE_INTEGER iCurrentTime;
	ULONG uCurTime = 0;
	PKSEMAPHORE pSendBufferWaitEvent = NULL;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pServerElement != NULL);
	OS_ASSERT(pServerElement->pSession != NULL);
	OS_ASSERT(pServerElement->pSessionManager != NULL);
	OS_ASSERT(pServerElement->pSessionManager->pLogicalGroupPtr != NULL);

	DebugPrint((DBG_SEND, "COM_SendThreadForServer(): Enter COM_SendThreadForServer()\n"));

	if(pServerElement->pSessionManager->LastPacketSentTime == 0)
	{
		KeQuerySystemTime(&iCurrentTime);
		uCurTime = (ULONG)(iCurrentTime.QuadPart/(10*1000*1000));
		pServerElement->pSessionManager->LastPacketSentTime = uCurTime;
	}

	swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pServerElement->pSessionManager);
	swprintf(wchar3,L"%S",L"COM_SendThreadForServer");
	sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_STARTED,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);

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
					ExAcquireResourceSharedLite(&pServerElement->pSessionManager->ServerListLock,TRUE);

					//Cahnging the while to if
					if(pServerElement->pSession != NULL && !bExit)
					{
						OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

						status = KeWaitForSingleObject(&pServerElement->SessionIOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
						if(status == STATUS_SUCCESS)
						{
							swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
							swprintf(wchar2,L"0X%08X",pServerElement->pSessionManager);
							sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SEND_THREAD_EXITING,status,0,wchar1,wchar2);

							DebugPrint((DBG_SEND, "COM_SendThreadForServer(): The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}

						if(pServerElement->pSession->bSessionEstablished == SFTK_CONNECTED)
						{
							pSendBuffer = NULL;

							// Send to Complete just goto the next Send IRP.

							//The pSession->pSendBuffer is completely Processed so 
							//Check if there are any Stale Buffers on the Wire
							if(pSendBuffer == NULL)
							{
								pSendBuffer = COM_GetNextStaleSendBuffer(&pServerElement->pSessionManager->sendBufferList);
							}

							if(pSendBuffer == NULL)
							{
								// If we didnt send the handshake Information dont go for the Refresh IO
								// or Commit IO
								if(pServerElement->pSessionManager->bSendHandshakeInformation == TRUE)
								{
									DebugPrint((DBG_SEND, "COM_SendThreadForServer(): The Handshake information is not sent so we cannot send the other Data\n"));
									iWait.QuadPart = -(30*10*1000);
									KeDelayExecutionThread(KernelMode,FALSE,&iWait);
									leave;
								}

//									KdPrint(("Coundnt Find a Stale Send Buffer So Calling COM_GetNextSendBuffer\n"));
								pSendBuffer = COM_GetNextSendBuffer(&pServerElement->pSessionManager->sendBufferList);
							
								if(pSendBuffer != NULL)
								{
#define _PARAGS_CODE_
#ifdef _PARAGS_CODE_
									pSendBuffer->TotalSendLength = pSendBuffer->SendWindow;
									// We need to get the Packets from the Migrate Queue and fill up the buffer
									if(!QM_RetrievePkts(
										pServerElement->pSessionManager->pLogicalGroupPtr, 
										(PVOID)pSendBuffer->pSendBuffer, 
										&pSendBuffer->TotalSendLength,	// In Buffer Max size, OUT actual buffer data filled size
										FALSE,
										NULL,
										&pSendBufferWaitEvent))
									{
										/*
										swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
										swprintf(wchar2,L"0X%08X",pServerElement->pSessionManager);
										sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SEND_DISPATCH_QUEUE_IS_EMPTY,STATUS_SUCCESS,0,wchar1,wchar2);
										*/

										// If there is no data to be read then just Sleep for sometime and then try again.
										iWait.QuadPart = -(100*10*1000);
										KeDelayExecutionThread(KernelMode,FALSE,&iWait);
										pSendBuffer->state = SFTK_BUFFER_FREE;

										leave;
									}// while() loop until we get the Packets

#else
									if(0)
									{
										// This is the regular testing code that will be used by the sample driver
										// Fill in Some Data or Do Something
										COM_FillRawBufferRandom(pSendBuffer,&LenBuf);

										pSendBuffer->ActualSendLength =0;
										pSendBuffer->TotalSendLength = LenBuf;
									}
									else
									{
										// For the timebeing just set the Buffer to Free, 
										// Sleep for a few seconds and continue;

										iWait.QuadPart = -(100*10*1000);
										KeDelayExecutionThread(KernelMode,FALSE,&iWait);
										pSendBuffer->state = SFTK_BUFFER_FREE;

										leave;
									}
#endif
								}
								else
								{
									/*
									swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
									swprintf(wchar2,L"0X%08X",pServerElement->pSessionManager);
									sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SEND_BUFFER_LIST_IS_EMPTY,STATUS_SUCCESS,0,wchar1,wchar2);
									*/

									//30 Milli-Seconds in 100*Nano Seconds
									iWait.QuadPart = -(100*10*1000);
									DebugPrint((DBG_SEND, "COM_SendThreadForServer(): There are no more Buffers available to Send Data so Wait\n"));
									KeDelayExecutionThread(KernelMode,FALSE,&iWait);

									leave;
								}
							}

							if(pSendBuffer != NULL)
							{
								// Set the pSession->pSendBuffer, to the Current Buffer, either if
								pServerElement->pSession->pSendBuffer = pSendBuffer;

								// Insert the newly sent Buffer to the sendList. this will keep tarck of 
								// All the Buffers that are either sent or in pending on this session.
								// We can use this list to cancel any IRP's that are not yet completed 
								// on this session.

								KeAcquireSpinLock(&pServerElement->pSession->sendIrpListLock , &oldIrql);
								InsertHeadList(&pServerElement->pSession->sendIrpList , &pSendBuffer->SessionListElemet);
								KeReleaseSpinLock(&pServerElement->pSession->sendIrpListLock , oldIrql);

								LenBuf = pSendBuffer->TotalSendLength - pSendBuffer->ActualSendLength;
								//Lock the Mdl
								pSendBuffer->pSendMdl = TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer+pSendBuffer->ActualSendLength,LenBuf,FALSE,FALSE,NULL);

								OS_ASSERT(pSendBuffer->pSendMdl);

								// Dont Know wht
								// pSendBuffer->state = SFTK_BUFFER_USED;
								pSendBuffer->pSendMdl->Next = NULL;
								pSendBuffer->pSession = pServerElement->pSession;

								DebugPrint((DBG_SEND, "COM_SendThreadForServer(): The Session that is used to Send is %lx\n",pServerElement->pSession));

								try
								{
									KeQuerySystemTime(&iCurrentTime);
									uCurTime = (ULONG)(iCurrentTime.QuadPart/(10*1000*1000));
									pServerElement->pSessionManager->LastPacketSentTime = uCurTime;

									//Reinitialize the IRP, incase of resuing it.
									//Lets Try without Reuse IRP
//									if(pSendBuffer->pSendIrp != NULL)
//									{
//										IoReuseIrp(pSendBuffer->pSendIrp,STATUS_SUCCESS);
//									}
									status = TDI_SendOnEndpoint1(
													&pServerElement->pSession->KSEndpoint,
													NULL,       // User Completion Event
													TDI_EndpointSendRequestComplete2,
													pSendBuffer,       // User Completion Context
													&pSendBuffer->IoSendStatus,
													pSendBuffer->pSendMdl,
													TDI_SEND_NO_RESPONSE_EXPECTED  | TDI_SEND_NON_BLOCKING,           // Send Flags
													&pSendBuffer->pSendIrp
													);

									if(	(status == STATUS_INSUFFICIENT_RESOURCES) && 
										(pSendBuffer->pSendIrp == NULL))
									{
										swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
										swprintf(wchar2,L"0X%08X",pServerElement);
										swprintf(wchar3,L"0X%08X",status);
										sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SEND_THREAD_EXCEPTION,status,0,wchar1,wchar2,wchar3);

										DebugPrint((DBG_ERROR, "COM_SendThreadForServer(): An Exception Occured in COM_SendThread2() Error Code is %lx\n",status));
										// Failed to Allocate IRP hence just set the Buffer to Free 
										pSendBuffer->state = SFTK_BUFFER_USED;
										pSendBuffer->ActualSendLength =0;
									}
								}
								except(EXCEPTION_EXECUTE_HANDLER)
								{
									swprintf(wchar1,L"%d",pServerElement->pSessionManager->pLogicalGroupPtr->LGroupNumber);
									swprintf(wchar2,L"0X%08X",pServerElement);
									swprintf(wchar3,L"0X%08X",GetExceptionCode());
									sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SEND_THREAD_EXCEPTION,GetExceptionCode(),0,wchar1,wchar2,wchar3);

									DebugPrint((DBG_ERROR, "COM_SendThreadForServer(): An Exception Occured in COM_SendThreadForServer() Error Code is %lx\n",GetExceptionCode()));
									pSendBuffer->state = SFTK_BUFFER_USED;
									pSendBuffer->ActualSendLength =0;
								}
							}
						}
						else
						{
							DebugPrint((DBG_SEND, "COM_SendThreadForServer(): The Server %lx is not Connected so exiting the send thread\n", pServerElement));
							bExit = TRUE;
							break;
						}
						iWait.QuadPart = -(10*10*1000);
						KeDelayExecutionThread(KernelMode,FALSE,&iWait);
					}	//if(pServerElement->pSession != NULL && !bExit)
					if(!bExit)
					{
						OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

						status = KeWaitForSingleObject(&pServerElement->SessionIOExitEvent,Executive,KernelMode,FALSE,&iZeroWait);
						if(status == STATUS_SUCCESS)
						{
							DebugPrint((DBG_SEND, "COM_SendThreadForServer(): The pSessionManager->IOExitEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}
					}
				}//try
				finally
				{
					//Release the Send Lock
					ExReleaseResourceLite(&pServerElement->pSessionManager->ServerListLock);
					KeLeaveCriticalRegion();	//Enable the Kernel APC's
				}
			}//while(!bExit)
		}//try
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			DebugPrint((DBG_ERROR, "COM_SendThreadForServer(): An Exception Occured in COM_SendThreadForServer() Error Code is %lx\n",GetExceptionCode()));
		}
	}//try
	finally
	{
		DebugPrint((DBG_SEND, "COM_SendThreadForServer(): Exiting the COM_SendThreadForServer() status = %lx\n",status));
	}

	return PsTerminateSystemThread(status);
}

#endif	//__NEW_SEND__


/*************************************************************

#if 0	// This Code need not be exacuted _OBSOLETE_


NTSTATUS
TDI_EndpointSendRequestComplete(
							IN PDEVICE_OBJECT pDeviceObject,
							IN PIRP pIrp,
							IN PVOID Context
							)
{
	PSEND_BUFFER pSendBuffer= NULL;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG Length = 0;
	ULONG remainingLength = 0;
//	PPROTOCOL_PACKET pProtocolPacket;
//	KIRQL oldIrql;
	
	status = pIrp->IoStatus.Status;
	Length = pIrp->IoStatus.Information;
	pSendBuffer = (PSEND_BUFFER)Context;

	if(NT_SUCCESS(status))
	{
		KdPrint(( "TDI_EndpointSendRequestComplete: Send %d Bytes and Index = %d for SendBuffer = %lx\n", Length, pSendBuffer->index , pSendBuffer));

		pSendBuffer->ActualSendLength += Length;
		TDI_UnlockAndFreeMdl(pSendBuffer->pSendMdl);
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
				pSendBuffer->pSendMdl = TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer + pSendBuffer->ActualSendLength,remainingLength,FALSE,FALSE,NULL);	
			}
			else
			{
				//Send Only a Chunk Size
				pSendBuffer->pSendMdl = TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer + pSendBuffer->ActualSendLength,pSendBuffer->pSendList->ChunkSize,FALSE,FALSE,NULL);	
			}
			try
			{
				if(pSendBuffer->pSendIrp != NULL)
				{
					IoReuseIrp(pSendBuffer->pSendIrp,STATUS_SUCCESS);
				}
				status = TDI_SendOnEndpoint1(
								&pSendBuffer->pSession->KSEndpoint,
								NULL,       // User Completion Event
								TDI_EndpointSendRequestComplete,
								pSendBuffer,       // User Completion Context
								&pSendBuffer->IoSendStatus,
								pSendBuffer->pSendMdl,
								0,           // Send Flags
								&pSendBuffer->pSendIrp
								);
			}
			except(EXCEPTION_EXECUTE_HANDLER)
			{
				KdPrint(("An Exception Occured in TDI_EndpointSendRequestComplete() Error Code is %lx\n",GetExceptionCode()));
				status = GetExceptionCode();
				pSendBuffer->state = SFTK_BUFFER_USED;
				pSendBuffer->pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
			}
		}
	}
	else
	{
		pSendBuffer->state = SFTK_BUFFER_USED;
		pSendBuffer->pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
		KdPrint(( "SftkTCPSSendBuffer: status 0x%8.8X and Index = %d for SendBuffer = %lx\n", status , pSendBuffer->index , pSendBuffer));
	}

//	IoFreeIrp( pIrp );

//	pSendBuffer->state = SFTK_BUFFER_FREE;
	return STATUS_MORE_PROCESSING_REQUIRED;
}



#endif 0	// This Code need not be exacuted _OBSOLETE_


************************************************************/
