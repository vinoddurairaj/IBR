/**************************************************************************************

Module Name: sftk_tdiconnect.c   
Author Name: Veera Arja
Description: Describes Modules: Protocol and CommunicationFunctions
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#include "sftk_main.h"



//#define _USE_RECEIVE_EVENT_

NTSTATUS
TDI_ConnectedCallback(
					IN PDEVICE_OBJECT pDeviceObject,
					IN PIRP pIrp,
					IN PVOID Context
					)
{
	PTCP_SESSION pSession		= NULL;
	NTSTATUS status				= STATUS_SUCCESS;
	KIRQL oldIrql;
	LONG nLiveSessions			= 0;
	ULONG uRemoteIP				= 0;
	USHORT uRemotePort			= 0;
//	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	pSession = (PTCP_SESSION)Context;

	OS_ASSERT(pSession != NULL);
	OS_ASSERT(pSession->pServer != NULL);
	OS_ASSERT(pSession->pServer->pSessionManager != NULL);
	OS_ASSERT(pSession->pServer->pSessionManager->pLogicalGroupPtr != NULL);

#if DBG
	   DEBUG_DumpTransportAddress(
         (PTRANSPORT_ADDRESS )&pSession->RemoteAddress
         );
#endif
	

	try
	{
		uRemoteIP	= pSession->RemoteAddress.Address[0].Address[0].in_addr;
		uRemotePort = pSession->RemoteAddress.Address[0].Address[0].sin_port;

		if(!NT_SUCCESS(pIrp->IoStatus.Status))
		{
//			OS_ASSERT(pSession->pServer->pSessionManager->pLogicalGroupPtr != NULL);
//			swprintf(wchar1,L"%d",pSession->pServer->pSessionManager->pLogicalGroupPtr->LGroupNumber);
//			swprintf(wchar2,L"0X%08X",pSession->pServer);

//			SM_CONVERT_IP_PORT_TO_STRING_W(wchar3,uRemoteIP,uRemotePort);	// Converts the IP inaddr to String (100.0.0.21:324)

//			swprintf(wchar4,L"0X%08X",pIrp->IoStatus.Status);
//			sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSION_CONNECTION_FAILED_ERROR,pIrp->IoStatus.Status,0,wchar1,wchar2,wchar3,wchar4);

			DebugPrint((DBG_CONNECT, "TDI_ConnectedCallback(): Failed pt Connect to the Secondry with Session = %lx and Id = %ld and status = %lx\n",pSession,pSession->sessionID,pIrp->IoStatus.Status));
			pSession->bSessionEstablished = SFTK_DISCONNECTED;

		}
		else
		{


			DebugPrint((DBG_CONNECT, "TDI_ConnectedCallback(): Successfully Connected to the Secondry with Session = %lx and Id = %ld\n",pSession,pSession->sessionID));

			//pSession->pServer->pSessionManager->nLiveSessions++;
			//Using the Interlocked Increment
			nLiveSessions = InterlockedIncrement(&pSession->pServer->pSessionManager->nLiveSessions);

			pSession->bSessionEstablished = SFTK_CONNECTED;
	
			if(nLiveSessions == 1)
			{
				KeSetEvent(&pSession->pServer->pSessionManager->GroupConnectedEvent,0,FALSE);
				KeClearEvent(&pSession->pServer->pSessionManager->GroupDisconnectedEvent);
			}

//			OS_ASSERT(pSession->pServer->pSessionManager->pLogicalGroupPtr != NULL);
//			swprintf(wchar1,L"%d",pSession->pServer->pSessionManager->pLogicalGroupPtr->LGroupNumber);
//			swprintf(wchar2,L"0X%08X",pSession->pServer);

//			SM_CONVERT_IP_PORT_TO_STRING_W(wchar3,uRemoteIP,uRemotePort);	// Converts the IP inaddr to String (100.0.0.21:324)

//			swprintf(wchar4,L"%d",nLiveSessions);
//			sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSION_CONNECTION_SUCCESSFULL,pIrp->IoStatus.Status,0,wchar1,wchar2,wchar3,wchar4);

		}

		// Free all the Connect and Listen IRP's
		if(pSession->pConnectIrp != NULL)
		{
			IoFreeIrp(pSession->pConnectIrp);
			pSession->pConnectIrp = NULL;
		}
		if(pSession->pListenIrp != NULL)
		{
			IoFreeIrp(pSession->pListenIrp);
			pSession->pListenIrp = NULL;
		}
	}
	finally
	{
	}
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS 
TDI_ErrorEventHandler(
						IN PVOID TdiEventContext,  // The endpoint's file object.
						IN NTSTATUS status         // status code indicating error type.
						)
{
   DebugPrint((DBG_CONNECT, "TDI_ErrorEventHandler():  status: 0x%8.8X\n", status) );

   return( STATUS_SUCCESS );
}

NTSTATUS 
TDI_ReceiveExpeditedEventHandler(
						IN PVOID TdiEventContext,     // Context From SetEventHandler
						IN CONNECTION_CONTEXT ConnectionContext,
						IN ULONG ReceiveFlags,          //
						IN ULONG BytesIndicated,        // number of bytes in this indication
						IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
						OUT ULONG *BytesTaken,          // number of bytes used by indication routine
						IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
						OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
						)
{
   DebugPrint((DBG_RECV, "TDI_ReceiveExpeditedEventHandler(): Entry...\n") );

   DebugPrint((DBG_RECV, "TDI_ReceiveExpeditedEventHandler(): Bytes Indicated: %d; BytesAvailable: %d; Flags: 0x%8.8x\n",
      BytesIndicated, BytesAvailable, ReceiveFlags));

   return( STATUS_SUCCESS );
}

NTSTATUS 
TDI_DisconnectEventHandler(
							IN PVOID TdiEventContext,     // Context From SetEventHandler
							IN CONNECTION_CONTEXT ConnectionContext,
							IN LONG DisconnectDataLength,
							IN PVOID DisconnectData,
							IN LONG DisconnectInformationLength,
							IN PVOID DisconnectInformation,
							IN ULONG DisconnectFlags
							)
{
	PTCP_SESSION pSession		= NULL;
	KIRQL oldIrql;
	LONG nLiveSessions			= 0;
	ULONG uRemoteIP				= 0;
	USHORT uRemotePort			= 0;
//	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	pSession = (PTCP_SESSION)TdiEventContext;

	OS_ASSERT(pSession != NULL);
	OS_ASSERT(pSession->pServer != NULL);
	OS_ASSERT(pSession->pServer->pSessionManager != NULL);
	OS_ASSERT(pSession->pServer->pSessionManager->pLogicalGroupPtr != NULL);

	DebugPrint((DBG_CONNECT, "TDI_DisconnectEventHandler(): Entry; EventContext: 0x%8.8X; ConnectionContext: 0x%8.8X; Flags: 0x%8.8X\n",
      (ULONG )TdiEventContext, (ULONG)ConnectionContext, DisconnectFlags)
      );

	try
	{
		uRemoteIP	= pSession->RemoteAddress.Address[0].Address[0].in_addr;
		uRemotePort = pSession->RemoteAddress.Address[0].Address[0].sin_port;

		DebugPrint((DBG_ERROR, "TDI_DisconnectEventHandler(): The Disconnected Session is = %d\n",pSession->sessionID));

		//pSession->pServer->pSessionManager->nLiveSessions--;
		//Doing InterlockedDecrement
		nLiveSessions = InterlockedExchange(&pSession->pServer->pSessionManager->nLiveSessions, 
											pSession->pServer->pSessionManager->nLiveSessions);
		if (nLiveSessions > 0)
		{
			nLiveSessions = InterlockedDecrement(&pSession->pServer->pSessionManager->nLiveSessions);
			pSession->bSessionEstablished = SFTK_DISCONNECTED;

			// Reset the Server
			pSession->pServer->bReset = TRUE;

			if(nLiveSessions ==0)
			{
				DebugPrint((DBG_CONNECT, "TDI_DisconnectEventHandler(): All the Connections for this SESSION_MANAGER are Down\n"));
				KeSetEvent(&pSession->pServer->pSessionManager->GroupDisconnectedEvent,0,FALSE);
				KeClearEvent(&pSession->pServer->pSessionManager->GroupConnectedEvent);
			}
		}
		
//		OS_ASSERT(pSession->pServer->pSessionManager->pLogicalGroupPtr != NULL);
//		swprintf(wchar1,L"%d",pSession->pServer->pSessionManager->pLogicalGroupPtr->LGroupNumber);
//		swprintf(wchar2,L"0X%08X",pSession->pServer);

//		SM_CONVERT_IP_PORT_TO_STRING_W(wchar3,uRemoteIP,uRemotePort);	// Converts the IP inaddr to String (100.0.0.21:324)

//		swprintf(wchar4,L"%d",nLiveSessions);
//		sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSION_GOT_DISCONNECTED,DisconnectFlags,0,wchar1,wchar2,wchar3,wchar4);
	}
	finally
	{

	}

	return( STATUS_SUCCESS );
}// TDI_DisconnectEventHandler


//This command gets the Command that caused the ACK, since multiple Commands 
//can cause a comman ACK, we have to be careful about this. 
//A Hint is provided that will be useful in determining which Command could have 
//caused the ACK.
LONG 
PROTO_GetCommandOfAck(
			IN ftd_header_t	*AckProtoHdr
			)
{
	LONG returnCommand = -1;
	enum protocol ReceivedProtocolCommand = AckProtoHdr->msgtype;
	ULONG Hint = 1;

	switch(ReceivedProtocolCommand)
	{
	case FTDACKHUP:
		returnCommand =  FTDCHUP;
		break;
	case FTDACKRSYNC:
		//This ACK could have been received both for REFRESH and BACKREFRESH
		//So Could be both of them depending on the State.
		//On the Primary Side its FTDCRFBLK
		if(Hint == 1)
		{
			returnCommand = FTDCRFBLK;
		}
		else
		{
			returnCommand = FTDCBFBLK;
		}
		break;
	case FTDACKCLI:
		//The Only place where the Primary is getting this ACK is when 
		//the secondary is set in FTDCBFSTART Phase.
		returnCommand = FTDCBFSTART;
		break;
	case FTDACKNOOP:
		//This is not used anywhere. We need not have to handle this case
		//Nobody is waiting on this ACK.
		//So We assume that the Command sent is either FTDCRSYNCDEVS or FTDCRSYNCDEVE
		//But the FTDCRSYNCDEVE is never sent, so only command that can get this ACK is
		//FTDCRSYNCDEVS
		returnCommand = FTDCRSYNCDEVS;
		break;
	case FTDACKCHKSUM:
		returnCommand = FTDCCHKSUM;	// Recieved payload 
		break;
	case FTDACKCHUNK:
		// The return header contains the ACK for the MSG_CO , MSG_INCO Sentinals.
		returnCommand = FTDCCHUNK;
		break;
	case FTDACKVERSION1:
		returnCommand = FTDCVERSION;
		break;
	case FTDACKHANDSHAKE:
		returnCommand = FTDCHANDSHAKE;
		break;
	case FTDACKCONFIG:
		returnCommand = FTDCCHKCONFIG;
		break;
	case FTDACKERR:
		//In this case the first command on the Protocol Queue 
		break;
	case FTDACKRFSTART:
		//In the existing Code nothing is done when we got this ACK.
		//The Refresh Start and Completion is based on the FTDACKCHUNK flags
		//The Return Header Contains the RFSTART, RFDONE flags.
		//This can be used but right now we dont use it.
//		if(Hint == 1)
//		{
//			returnCommand = FTDMSGINCO;
//		}
//		else
//		{
//			returnCommand = FTDMSGAVOIDJOURNALS;
//		}
		break;
	case FTDACKCPON:
		returnCommand = FTDMSGCPON;
		break;
	case FTDACKCPOFF:
		returnCommand = FTDMSGCPOFF;
		break;
	default:
		break;
	}
	return returnCommand;
}

//This function returns the respecive ACK that will be received from the Secondary Side
//As response to the Primary Side Command's

LONG 
PROTO_GetAckOfCommand(
			IN enum protocol SentProtocolCommand
			)
{
	LONG returnAck = -1;
	switch(SentProtocolCommand)
	{
		case FTDCHUP:
			returnAck = FTDACKHUP;
			break;
		case FTDCRFBLK:
			returnAck = FTDACKRSYNC;
			break;
		case FTDCBFSTART:
			returnAck = FTDACKCLI;
			break;
		case FTDCRSYNCDEVS:
			returnAck = FTDACKNOOP;
			break;
		case FTDCCHKSUM:
			returnAck = FTDACKCHKSUM;
			break;
		case FTDCBFBLK:
			returnAck = FTDACKRSYNC;
			break;
		case FTDCCHUNK:			// For all the Sentinals and the Bab Data the Same Ack is expected
		case FTDMSGINCO:		// But the Falgs within the header.msg.lg.flags will contain various
		case FTDMSGCO:			// conditions like Smart Refersh Started or Completed etc..
		case FTDMSGAVOIDJOURNALS:
			returnAck = FTDACKCHUNK;
			break;
		case FTDCVERSION:
			returnAck = FTDACKVERSION1;
			break;
		case FTDCHANDSHAKE:
			returnAck = FTDACKHANDSHAKE;
			break;
		case FTDCCHKCONFIG:
			returnAck = FTDACKCONFIG;
			break;
			//These Commands have no ACK's
		case FTDCRFFSTART:
		case FTDCRFFEND:
		case FTDCBFEND:
		case FTDCNOOP:
			break;
		default:
			break;

	}
	return returnAck;
}

LONG 
PROTO_GetAckSentinal(
			IN enum sentinals   eSentinal
			)
{
	ULONG returnAck = -1;

	switch(eSentinal)
	{
	case MSGINCO:
		returnAck = FTDACKRFSTART;
		//Along with this ACK, for the Data Block the ACK FTDACKCHUNK will have the
		//flag "ack.msg.lg.flags" set with the SET_LG_RFSTART()
		break;
	case MSGCO:
		//There is no ACK, But the Data Block that is ACKed back with FTDACKCHUNK
		//will have the flag "ack.msg.lg.flags" set to SET_LG_RFDONE()
		break;
	case MSGCPON:
		returnAck = FTDACKCPON;
		break;
	case MSGCPOFF:
		returnAck = FTDACKCPOFF;
		break;
	case MSGAVOIDJOURNALS:
		returnAck = FTDACKRFSTART;
		break;
	default:
		break;
	}
	return returnAck;
}


//All these functions handle the Received Packets
//Only the ACK packets and the Back-Refresh Packets are used

NTSTATUS 
PROTO_ReceiveHUP(
			IN ftd_header_t* pHeader , 
			IN PTCP_SESSION pSession
			)
{
	//This will be handled on the secondary side, 
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKHUP(
				IN ftd_header_t* pHeader,
				IN PTCP_SESSION pSession
				)
{
	NTSTATUS status = STATUS_SUCCESS;

	return status;
}
//Full Refresh Start, Dealt on the secondary side
NTSTATUS 
PROTO_ReceiveRFFSTART(
				IN ftd_header_t* pHeader,
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

//This is received on the Primary Side as part of the Full Refresh or the Smart Refresh
//Packets.
NTSTATUS 
PROTO_ReceiveACKRSYNC(
				IN ftd_header_t* pHeader,
				IN PTCP_SESSION pSession
				)
{
	NTSTATUS status = STATUS_SUCCESS;

	return status;
}

//This is dealt on the Secondary Side
NTSTATUS 
PROTO_ReceiveRFFEND(
				IN ftd_header_t* pHeader,
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

//This is dealt on the Secondary Side
NTSTATUS 
PROTO_ReceiveBFSTART(
				 IN ftd_header_t* pHeader,
				 IN PTCP_SESSION pSession
				 )
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKCLI(
				IN ftd_header_t* pHeader,
				IN PTCP_SESSION pSession
				)
{
	NTSTATUS status = STATUS_SUCCESS;

	return status;
}

//Dealt on the secondary Side
NTSTATUS 
PROTO_ReceiveRSYNCDEVS(
					IN ftd_header_t* pHeader,
					IN PTCP_SESSION pSession
					)
{
	return STATUS_SUCCESS;
}
//Dealt on the secondary side
NTSTATUS 
PROTO_ReceiveBFEND(
				IN ftd_header_t* pHeader,
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

//This is in resonse to the MSG_INCO OR MSG_AVOID_JOURNALS
NTSTATUS 
PROTO_ReceiveACKRFSTART(
					 IN ftd_header_t* pHeader,
					 IN PTCP_SESSION pSession
					 )
{
	//Do Nothing Just return
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKCPSTART(
					 IN ftd_header_t* pHeader,
					 IN PTCP_SESSION pSession
					 )
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKCPSTOP(
					IN ftd_header_t* pHeader,
					IN PTCP_SESSION pSession
					)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKCPON(
				  IN ftd_header_t* pHeader,
				  IN PTCP_SESSION pSession
				  )
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKCPOFF(
				IN ftd_header_t* pHeader,
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveCPONERR(
				 IN ftd_header_t* pHeader,
				 IN PTCP_SESSION pSession
				 )
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveCPOFFERR(
				IN ftd_header_t* pHeader,
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKCHUNK(
				IN ftd_header_t* pHeader,
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveNOOP(
			IN ftd_header_t* pHeader,
			IN PTCP_SESSION pSession
			)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKHANDSHAKE(
					IN ftd_header_t* pHeader,
					IN PTCP_SESSION pSession
					)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKCONFIG(
					IN ftd_header_t* pHeader,
					IN PTCP_SESSION pSession
					)
{
	return STATUS_SUCCESS;
}

//These are commands with Payload

NTSTATUS 
PROTO_ReceiveRFBLK(
				IN ftd_header_t* pHeader , 
				IN PUCHAR pDataBuffer , 
				IN ULONG nDataLength , 
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

//Got the Error Message So write to EventLog
NTSTATUS 
PROTO_ReceiveACKERR(
				IN ftd_header_t* pHeader , 
				IN PUCHAR pDataBuffer , 
				IN ULONG nDataLength , 
				IN PTCP_SESSION pSession
				)
{
	//We have to log this error to the eventlog, this is for TODO::
	//For the timebeing just print the message
	DebugPrint((DBG_COM, "PROTO_ReceiveACKERR(): The Error from the Secondary is %s\n",(PCHAR)pDataBuffer));
	return STATUS_SUCCESS;
}


NTSTATUS 
PROTO_ReceiveCHKSUM(
				IN ftd_header_t* pHeader , 
				IN PUCHAR pDataBuffer , 
				IN ULONG nDataLength , 
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveBFBLK(
				IN ftd_header_t* pHeader , 
				IN PUCHAR pDataBuffer , 
				IN ULONG nDataLength , 
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKCHKSUM(
					IN ftd_header_t* pHeader , 
					IN PUCHAR pDataBuffer , 
					IN ULONG nDataLength , 
					IN PTCP_SESSION pSession
					)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveCHUNK(
				IN ftd_header_t* pHeader , 
				IN PUCHAR pDataBuffer , 
				IN ULONG nDataLength , 
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveVERSION(
				IN ftd_header_t* pHeader , 
				IN PUCHAR pDataBuffer , 
				IN ULONG nDataLength , 
				IN PTCP_SESSION pSession
				)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveHANDSHAKE(
					IN ftd_header_t* pHeader , 
					IN PUCHAR pDataBuffer , 
					IN ULONG nDataLength , 
					IN PTCP_SESSION pSession
					)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveCHKCONFIG(
					IN ftd_header_t* pHeader , 
					IN PUCHAR pDataBuffer , 
					IN ULONG nDataLength , 
					IN PTCP_SESSION pSession
					)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
PROTO_ReceiveACKVERSION1(
					IN ftd_header_t* pHeader , 
					IN PUCHAR pDataBuffer , 
					IN ULONG nDataLength , 
					IN PTCP_SESSION pSession
					)
{
	return STATUS_SUCCESS;
}

//Here are the Types of algorithms supported

//#define PRED			1	// Prediction Based Algorithm
//#define LZHL			2	// LZH-Light algorithm implementation v 1.0

NTSTATUS 
PROTO_CompressBuffer(
				 IN PUCHAR pInputBuffer ,	// The input Buffer
				 IN ULONG nInputLength ,	// The Length of the input Buffer
				 OUT PUCHAR* pOutputBuffer ,	// The OutputBuffer Allocated  in the Function
				 OUT PULONG pOutputLength , // The Length of the Compressed Data
				 IN LONG algorithm			// The Algorithm used 
				 )
{
	NTSTATUS        status = STATUS_SUCCESS;
	comp_t          *p_compression = NULL;
	WCHAR			wchar1[64];

//	This is a Comment from my friend Jerome as part of his Compression Code
//	RtlCopyMemory(src, "Il fait beau aujourd'hui Il fait beau aujourd'hui Il fait beau aujourd'hui", 74);

	OS_ASSERT(pInputBuffer != NULL);
	OS_ASSERT(nInputLength > 0);
	OS_ASSERT(pOutputBuffer != NULL);
	OS_ASSERT(pOutputLength != NULL);

	//Check if the length is less than or equal to the Max IO Size allowed
	//#define SFTK_MAX_IO_SIZE	262144

	OS_ASSERT(nInputLength <= SFTK_MAX_IO_SIZE);

	try
	{
		switch(algorithm)
		{
		case LZHL:
		case PRED:
			p_compression = comp_create(algorithm);
			if(p_compression == NULL)
			{
				swprintf(wchar1,L"%d",algorithm);
				sftk_LogEventString1(GSftk_Config.DriverObject,MSG_LG_COMPRESSION_OBJECT_ALLOCATION_FAILED_ERROR,STATUS_MEMORY_NOT_ALLOCATED,0,wchar1);

				DebugPrint((DBG_ERROR, "PROTO_CompressBuffer(): Unable to Allocate Memory for comp_t structure for compression\n"));
				status = STATUS_MEMORY_NOT_ALLOCATED;
				leave;
			}
			// The Output Buffer can only be as big as the input Buffer
			*pOutputLength = nInputLength;
			//Allocate memory from the Paged Pool
			*pOutputBuffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool,*pOutputLength,'SSIK');

			if(*pOutputBuffer == NULL)
			{
				swprintf(wchar1,L"%d",algorithm);
				sftk_LogEventString1(GSftk_Config.DriverObject,MSG_LG_COMPRESSION_OBJECT_ALLOCATION_FAILED_ERROR,STATUS_MEMORY_NOT_ALLOCATED,0,wchar1);

				DebugPrint((DBG_ERROR, "PROTO_CompressBuffer(): Unable to Allocate Memory for the Target Buffer for Compression\n"));
				status = STATUS_MEMORY_NOT_ALLOCATED;
				leave;
			}//if
			*pOutputLength = comp_compress(*pOutputBuffer, pOutputLength, pInputBuffer, nInputLength, p_compression);
			break;
		default:
			swprintf(wchar1,L"%d",algorithm);
			sftk_LogEventString1(GSftk_Config.DriverObject,MSG_LG_COMPRESSION_ALGORITHM_NOT_SUPPORTED_ERROR,STATUS_NOT_SUPPORTED,0,wchar1);

			DebugPrint((DBG_ERROR, "PROTO_CompressBuffer(): This Algorithm = %d ,is not supported hence ERROR\n",algorithm));
			status = STATUS_NOT_SUPPORTED;
	//			STATUS_NOT_IMPLEMENTED;
			break;
		}//switch
	}//try
	finally
	{
		if(p_compression != NULL)
		{
			comp_delete(p_compression);
			p_compression = NULL;
		}//if
	}//finally
	return status;
}//PROTO_CompressBuffer


// This function is used to decompress the Buffer.

//#define PRED			1	// Prediction Based Algorithm
//#define LZHL			2	// LZH-Light algorithm implementation v 1.0

NTSTATUS 
PROTO_DecompressBuffer(
					IN PUCHAR pInputBuffer , 
					IN ULONG nInputLength , 
					OUT PUCHAR* pOutputBuffer , 
					OUT PULONG pOutputLength , 
					IN LONG algorithm
					)
{
	NTSTATUS        status = STATUS_SUCCESS;
	comp_t          *p_compression = NULL;
	WCHAR			wchar1[64];

//	This is a Comment from my friend Jerome as part of his Compression Code
//	RtlCopyMemory(src, "Il fait beau aujourd'hui Il fait beau aujourd'hui Il fait beau aujourd'hui", 74);


	OS_ASSERT(pInputBuffer != NULL);
	OS_ASSERT(nInputLength > 0);
	OS_ASSERT(pOutputBuffer != NULL);
	OS_ASSERT(pOutputLength != NULL);

	//The output Buffer size should be a maximum of SFTK_MAX_IO_SIZE
	//#define SFTK_MAX_IO_SIZE	262144

	try
	{
		switch(algorithm)
		{
		case LZHL:
		case PRED:
			p_compression = comp_create(algorithm);
			if(p_compression == NULL)
			{
				swprintf(wchar1,L"%d",algorithm);
				sftk_LogEventString1(GSftk_Config.DriverObject,MSG_LG_COMPRESSION_OBJECT_ALLOCATION_FAILED_ERROR,STATUS_MEMORY_NOT_ALLOCATED,0,wchar1);

				DebugPrint((DBG_ERROR, "PROTO_DecompressBuffer(): Unable to Allocate Memory for comp_t structure for compression\n"));
				status = STATUS_MEMORY_NOT_ALLOCATED;
				leave;
			}//if

			// The Output Buffer can not be bigger than the Mamimum IO Size
			*pOutputLength = SFTK_MAX_IO_SIZE;
			//Allocate memory from the Paged Pool
			*pOutputBuffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool,*pOutputLength,'SSIK');

			if(*pOutputBuffer == NULL)
			{
				swprintf(wchar1,L"%d",algorithm);
				sftk_LogEventString1(GSftk_Config.DriverObject,MSG_LG_COMPRESSION_OBJECT_ALLOCATION_FAILED_ERROR,STATUS_MEMORY_NOT_ALLOCATED,0,wchar1);

				DebugPrint((DBG_ERROR, "PROTO_DecompressBuffer(): Unable to Allocate Memory for the Target Buffer for Compression\n"));
				status = STATUS_MEMORY_NOT_ALLOCATED;
				leave;
			}//if
			*pOutputLength = comp_decompress(*pOutputBuffer, pOutputLength, pInputBuffer, &nInputLength, p_compression);
			break;
		default:
			swprintf(wchar1,L"%d",algorithm);
			sftk_LogEventString1(GSftk_Config.DriverObject,MSG_LG_COMPRESSION_ALGORITHM_NOT_SUPPORTED_ERROR,STATUS_NOT_SUPPORTED,0,wchar1);

			DebugPrint((DBG_ERROR, "PROTO_DecompressBuffer(): This Algorithm = %d ,is not supported hence ERROR\n",algorithm));
			status = STATUS_NOT_SUPPORTED;
	//			STATUS_NOT_IMPLEMENTED;
			break;
		}//switch
	}//try
	finally
	{
		if(p_compression != NULL)
		{
			comp_delete(p_compression);
			p_compression = NULL;
		}//if
	}
	return status;
}//PROTO_DecompressBuffer

NTSTATUS 
PROTO_CreateChecksum(
				IN PUCHAR pInputBuffer , 
				IN ULONG nInputLength , 
				OUT PUCHAR* pOutputBuffer , 
				OUT PULONG pOutputLength , 
				IN LONG algorithm
				)
{
	NTSTATUS status = STATUS_SUCCESS;
	MD5_CTX *pMd5Ctx = NULL;
	WCHAR			wchar1[64];

	OS_ASSERT(pInputBuffer != NULL);
	OS_ASSERT(nInputLength > 0);
	OS_ASSERT(pOutputBuffer != NULL);
	OS_ASSERT(pOutputLength != NULL);

	try
	{
		switch(algorithm)
		{
		case A_MD5:
			pMd5Ctx = MD5Create();
			if(pMd5Ctx == NULL)
			{
				swprintf(wchar1,L"%d",algorithm);
				sftk_LogEventString1(GSftk_Config.DriverObject,MSG_LG_CHECKSUM_OBJECT_ALLOCATION_FAILED_ERROR,STATUS_MEMORY_NOT_ALLOCATED,0,wchar1);

				DebugPrint((DBG_ERROR, "PROTO_CreateChecksum(): Unable to allocate memory for the MD5_CTX\n"));
				status = STATUS_MEMORY_NOT_ALLOCATED;
				leave;
			}//if

			*pOutputLength = DIGESTSIZE;
			*pOutputBuffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool,*pOutputLength,'SSIK');

			if(pOutputBuffer == NULL)
			{
				swprintf(wchar1,L"%d",algorithm);
				sftk_LogEventString1(GSftk_Config.DriverObject,MSG_LG_CHECKSUM_OBJECT_ALLOCATION_FAILED_ERROR,STATUS_MEMORY_NOT_ALLOCATED,0,wchar1);

				DebugPrint((DBG_ERROR, "PROTO_CreateChecksum(): Unable to Allocate Memory for the Target Buffer for Checksum for MD5\n"));
				status = STATUS_MEMORY_NOT_ALLOCATED;
				leave;
			}//if

			MD5Init(pMd5Ctx);
			MD5Update(pMd5Ctx, pInputBuffer , nInputLength );
			MD5Final(*pOutputBuffer,pMd5Ctx);
			break;
		case A_RLE:
		case A_CRC:
		default:
			swprintf(wchar1,L"%d",algorithm);
			sftk_LogEventString1(GSftk_Config.DriverObject,MSG_LG_CHECKSUM_ALGORITHM_NOT_SUPPORTED_ERROR,STATUS_NOT_SUPPORTED,0,wchar1);

			DebugPrint((DBG_ERROR, "PROTO_CreateChecksum(): This Algorithm = %d ,is not supported hence ERROR\n",algorithm));
			status = STATUS_NOT_SUPPORTED;
			break;
		}//switch
	}//try
	finally
	{//Free up the MD5_CTX
		if(pMd5Ctx != NULL)
		{
			MD5Delete(pMd5Ctx);
			pMd5Ctx = NULL;
		}//if
	}//finally
	return status;
}//PROTO_CreateChecksum

BOOLEAN 
PROTO_CompareChecksum(
				IN PUCHAR pInputBuffer1 , 
				IN PUCHAR pInputBuffer2 , 
				IN ULONG nDataLength , 
				IN LONG algorithm
				)
{
	return ( ( OS_EqualMemory(pInputBuffer1 , pInputBuffer2 , nDataLength) == 1 ) ? TRUE : FALSE );
}//PROTO_CompareChecksum


BOOLEAN 
PROTO_IsAckCheckRequired(
						 IN ftd_header_t* pHeader
						 )
{
	BOOLEAN bRequired = TRUE;

	switch(pHeader->msgtype)
	{
	case FTDCNOOP:
		DebugPrint((DBG_PROTO,"PROTO_IsAckCheckRequired(): The Packet Received is of type FTDCNOOP Ignoring ACK Check\n"));
		bRequired = FALSE;
		break;
	default:
		break;
	}
	return bRequired;
}// PROTO_IsAckCheckRequired

//This function validates the header and returns the length of the data
//that needs to be read after the header.
NTSTATUS 
PROTO_ProcessReceiveHeader(
						IN ftd_header_t* pHeader, 
						OUT PULONG pLen, 
						IN PTCP_SESSION pSession
						)
{
	NTSTATUS status = STATUS_SUCCESS;

#if 0	// Verification is removed for ACK because the Retuned ACK never has the MagicValue
	if(pHeader->magicvalue != MAGICHDR)
	{
		*pLen = 0;
		DebugPrint((DBG_ERROR, "PROTO_ProcessReceiveHeader(): The header Received is Invalid \n"));
		status = STATUS_INVALID_USER_BUFFER;
	}
	else
#endif	// Verification is removed for ACK because the Retuned ACK never has the MagicValue
	{
		*pLen = 0;
		DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The header Received is Valid the Length = %d \n",pHeader->len));

		//Process different kinds of Commands
		switch(pHeader->msgtype)
		{
		case FTDCHUP:		//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCHUP\n"));
			status = PROTO_ReceiveHUP(pHeader,pSession);
			break;
		case FTDACKHUP:		//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKHUP\n"));
			status = PROTO_ReceiveACKHUP(pHeader,pSession);
			break;
		case FTDCRFFSTART:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCRFFSTART\n"));
			status = PROTO_ReceiveRFFSTART(pHeader, pSession);
			break;
		case FTDCRFBLK:		//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCRFBLK\n"));
			if(pHeader->msg.lg.data == FTDZERO)
			{
				//Contains Data as all Zeros
				DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Data contains only Zero's as Data and the length = %ld\n",pHeader->len));
				*pLen = 0;
			}
			else if(pHeader->compress)
			{
				//The Compression is enabled so will have to be decompressed before writing to
				//the Disk
				DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): Compression is Enabled and compressed len = %ld , uncomplen = %ld\n" , pHeader->len , pHeader->uncomplen));
				*pLen = pHeader->len;
				status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			else
			{
				//Neither Compression or FTDZERO is Enabled, so Just get the buffer
				*pLen = pHeader->len;
				status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			break;
		case FTDACKRSYNC:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKRSYNC\n"));
			status = PROTO_ReceiveACKRSYNC(pHeader,pSession);
			break;
		case FTDCRFFEND:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCRFFEND\n"));
			status = PROTO_ReceiveRFFEND(pHeader, pSession);
			break;
		case FTDCBFSTART:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCBFSTART\n"));
			status = PROTO_ReceiveBFSTART(pHeader, pSession);
			break;
		case FTDACKCLI:		//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKCLI\n"));
			status = PROTO_ReceiveACKCLI(pHeader, pSession);
			break;
		case FTDACKERR:		//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKERR\n"));
			*pLen = sizeof(ftd_err_t);
			status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDCRSYNCDEVS:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCRSYNCDEVS\n"));
			status = PROTO_ReceiveRSYNCDEVS(pHeader, pSession);
			break;
		case FTDCCHKSUM:	//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCCHKSUM\n"));
			*pLen = sizeof(ftd_dev_t);
			//After getting the Device info, the fields in the ftd_dev_t are used to
			//get the CheckSum Payload
			status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDCBFBLK:		//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCBFBLK\n"));
			if(pHeader->msg.lg.data == FTDZERO)
			{
				//Contains Data as all Zeros
				DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Data contains only Zero's as Data and the length = %ld\n",pHeader->len));
				*pLen = 0;
			}
			else if(pHeader->compress)
			{
				//The Compression is enabled so will have to be decompressed before writing to
				//the Disk
				DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): Compression is Enabled and compressed len = %ld , uncomplen = %ld\n" , pHeader->len , pHeader->uncomplen));
				*pLen = pHeader->len;
				status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			else
			{
				//Neither Compression or FTDZERO is Enabled, so Just get the buffer
				*pLen = pHeader->len;
				status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			break;
		case FTDCBFEND:		//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCBFEND\n"));
			status = PROTO_ReceiveBFEND(pHeader, pSession);
			break;
		case FTDACKRFSTART:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKRFSTART\n"));
			status = PROTO_ReceiveACKRFSTART(pHeader, pSession);
			break;
		case FTDACKCHKSUM:	//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKCHKSUM\n"));
			//The Length of Payload is the Sizeof the Device and the Length of the
			// ftd_dev_delta_t structures
			*pLen = sizeof(ftd_dev_t)+ pHeader->len;
			status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDACKCPSTART:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKCPSTART\n"));
			status = PROTO_ReceiveACKCPSTART(pHeader, pSession);
			break;
		case FTDACKCPSTOP:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKCPSTOP\n"));
			status = PROTO_ReceiveACKCPSTOP(pHeader, pSession);
			break;
		case FTDCCPONERR:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCCPONERR\n"));
			status = PROTO_ReceiveCPONERR(pHeader, pSession);
			break;
		case FTDCCPOFFERR:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCCPOFFERR\n"));
			status = PROTO_ReceiveCPOFFERR(pHeader, pSession);
			break;
		case FTDACKCPON:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKCPON\n"));
			status = PROTO_ReceiveACKCPON(pHeader , pSession);
			break;
		case FTDACKCPOFF:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKCPOFF\n"));
			status = PROTO_ReceiveACKCPOFF(pHeader , pSession);
			break;
		case FTDCCHUNK:		//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCCHUNK\n"));
			//Compression could be Enabled
			if(pHeader->compress)
			{
				//The Compression is enabled so will have to be decompressed before writing to
				//the Disk
				DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): Compression is Enabled and compressed len = %ld , uncomplen = %ld\n" , pHeader->len , pHeader->uncomplen));
				*pLen = pHeader->len;
				status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			else
			{
				//Compression is not Enabled so get the Length
				*pLen = pHeader->len;
				status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			break;
		case FTDACKCHUNK:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKCHUNK\n"));
			//Doesnt have any payload, but the pHeader->msg.lg.flags can contain RFDONE
			//Or RFSTART
			status = PROTO_ReceiveACKCHUNK(pHeader , pSession);
			break;
		case FTDCNOOP:		//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCNOOP\n"));
			status = PROTO_ReceiveNOOP(pHeader , pSession);
			break;
		case FTDCVERSION:	//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCVERSION\n"));
			*pLen = sizeof(ftd_version_t);
			//Get the Primary Version Information
			status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDCHANDSHAKE:	//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCHANDSHAKE\n"));
			*pLen = sizeof(ftd_auth_t);
			//Get the Authentication Code 
			status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDACKHANDSHAKE:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKHANDSHAKE\n"));
			status = PROTO_ReceiveACKHANDSHAKE(pHeader , pSession);
			break;
		case FTDCCHKCONFIG:		//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDCCHKCONFIG\n"));
			//Add the length of the ftd_rdev_t structure that will be sent along with
			//header
			*pLen += sizeof(ftd_rdev_t);
			status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDACKCONFIG:	//No Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): The Packet Received is of type FTDACKCONFIG\n"));
			//The Size of the secondary Device and the Device id are returned
			status = PROTO_ReceiveACKCONFIG(pHeader , pSession);
			break;
		case FTDACKVERSION1:			//Payload This is the ACK for FTDCVERSION
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): Unknown Type of Packet is received\n"));
			//The Secondary Version String Length is in pHeader->msg.lg.data
			*pLen = pHeader->msg.lg.data;
			status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		default:
			status = STATUS_INVALID_USER_BUFFER;
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveHeader(): Packet is received of type %ld\n", pHeader->msgtype ));
			break;
		}
	}
return status;
}// PROTO_ProcessReceiveHeader

//This Function will have the data along with the Header.
//Upon receiving this data and header the respective 
//function is called to process the data.
//If there is still more data that needs to be read from the payload.
//this function will return STATUS_MORE_PROCESSING_REQUIRED and return the
//length of the buffer that still needs to be read in pLen
NTSTATUS 
PROTO_ProcessReceiveData(
					IN ftd_header_t* pHeader , 
					IN PUCHAR pDataBuffer , 
					IN ULONG nDataLength , 
					OUT PULONG pLen , 
					IN PTCP_SESSION pSession
					)
{
	NTSTATUS status = STATUS_SUCCESS;
	ftd_dev_t	*rdevp= NULL;

	// Dont check for the MagicValue because the returned ACK buffer doesnt contain the
	// Proper Value
#if 0	// Verification is removed for ACK because the Retuned ACK never has the MagicValue
	if(pHeader->magicvalue != MAGICHDR)
	{
		*pLen = 0;
		DebugPrint((DBG_ERROR, "PROTO_ProcessReceiveData(): The header Received is Invalid \n"));
		status = STATUS_INVALID_USER_BUFFER;
	}
	else
#endif	// Verification is removed for ACK because the Retuned ACK never has the MagicValue
	{
		*pLen = 0;
		//These are all the commands with Payloads
		//The payloads needs to be processed accordingly
		//For Comamnds that dont have a payload the processing is done in the 
		//function PROTO_ProcessReceiveHeader()
		
		switch(pHeader->msgtype)
		{
		case FTDCRFBLK:		//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): The Data Packet Received is of type FTDCRFBLK\n"));
			//This Function will takecare of the Processing of the Received Refresh Block
			status = PROTO_ReceiveRFBLK(pHeader, pDataBuffer , nDataLength , pSession);
			break;
		case FTDACKERR:		//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): The Data Packet Received is of type FTDACKERR\n"));
			status = PROTO_ReceiveACKERR(pHeader, pDataBuffer , nDataLength , pSession);
			break;
		case FTDCCHKSUM:
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): The Data Packet Received is of type FTDCCHKSUM ftd_dev_t structure is read\n"));

			if(nDataLength > sizeof(ftd_header_t)+sizeof(ftd_dev_t))
			{
				status = PROTO_ReceiveCHKSUM(pHeader , pDataBuffer , nDataLength , pSession);
			}
			else if(nDataLength == sizeof(ftd_header_t)+sizeof(ftd_dev_t))
			{
				//We have to read the Device ftd_dev_t, So read it and get the 
				//CheckSum Payload
				rdevp = (ftd_dev_t*)pDataBuffer;
				*pLen = rdevp->sumnum*DIGESTSIZE;
				//Each DIGESTSIZE represents one CHKSEGSIZE
				status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			break;
		case FTDCBFBLK:		//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): The Data Packet Received is of type FTDCBFBLK\n"));
			status = PROTO_ReceiveBFBLK(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDACKCHKSUM:	//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): The Data Packet Received is of type FTDACKCHKSUM\n"));
			status = PROTO_ReceiveACKCHKSUM(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDCCHUNK:		//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): The Packet Received is of type FTDCCHUNK\n"));
			status = PROTO_ReceiveCHUNK(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDCVERSION:	//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): The Packet Received is of type FTDCVERSION\n"));
			status = PROTO_ReceiveVERSION(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDCHANDSHAKE:	//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): The Packet Received is of type FTDCHANDSHAKE\n"));
			status = PROTO_ReceiveHANDSHAKE(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDCCHKCONFIG:		//Payload
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): The Packet Received is of type FTDCCHKCONFIG\n"));
			status = PROTO_ReceiveCHKCONFIG(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDACKVERSION1:			//Payload This is the ACK for FTDCVERSION
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): Unknown Type of Packet is received\n"));
			status = PROTO_ReceiveACKVERSION1(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		default:
			DebugPrint((DBG_PROTO, "PROTO_ProcessReceiveData(): Packet is received of type %ld\n", pHeader->msgtype ));
			status = STATUS_INVALID_USER_BUFFER;
			break;
		}
	}
return status;
}// PROTO_ProcessReceiveData




VOID 
COM_CleanUpServer(
			IN PSERVER_ELEMENT pServer
			)
{
	LONG lErrorCode =0;
	NTSTATUS status = STATUS_SUCCESS;

	OS_ASSERT(pServer);

	if(pServer->pSession != NULL)
	{
		COM_CleanUpSession(pServer->pSession);
		pServer->pSession = NULL;
	}
	NdisFreeMemory(pServer,sizeof(SERVER_ELEMENT),0);
	pServer = NULL;
}// COM_CleanUpServer

VOID 
COM_CleanUpSession(
			IN PTCP_SESSION pSession
			)
{
	OS_ASSERT(pSession);

	if(pSession != NULL)
	{
		if(pSession->pListenIrp != NULL)
		{
	        IoFreeIrp( pSession->pListenIrp );
			pSession->pListenIrp = NULL;
		}

		if(pSession->pReceiveHeader != NULL)
		{
			COM_ClearReceiveBuffer(pSession->pReceiveHeader);
			pSession->pReceiveHeader = NULL;
		}

		if(pSession->pReceiveBuffer != NULL)
		{
			COM_ClearReceiveBuffer(pSession->pReceiveBuffer);
			pSession->pReceiveBuffer = NULL;
		}
		pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
		pSession->pServer = NULL;
		NdisFreeMemory(pSession,sizeof(TCP_SESSION),0);
	}
	pSession = NULL;
}// COM_CleanUpSession

VOID 
COM_ClearSendBuffer(
			IN PSEND_BUFFER pSendBuffer
			)
{
	if(pSendBuffer != NULL)
	{
		if(pSendBuffer->pSendMdl != NULL)
		{
			TDI_UnlockAndFreeMdl( pSendBuffer->pSendMdl );
			pSendBuffer->pSendMdl = NULL;
		}
		if(pSendBuffer->pSendBuffer != NULL)
		{
			NdisFreeMemory(pSendBuffer->pSendBuffer,0,0);
			pSendBuffer->pSendBuffer = NULL;
		}
		if(pSendBuffer->pSendIrp != NULL)
		{
			IoFreeIrp(pSendBuffer->pSendIrp);
			pSendBuffer->pSendIrp = NULL;
		}
		DebugPrint((DBG_COM, "COM_ClearSendBuffer(): Freed the Send Buffer for buffer %lx and index = %d\n",pSendBuffer,pSendBuffer->index));
		pSendBuffer->pSendList = NULL;
		NdisFreeMemory(pSendBuffer,sizeof(SEND_BUFFER),0);
		pSendBuffer = NULL;
	}
}// COM_ClearSendBuffer

VOID 
COM_ClearReceiveBuffer(
				IN PRECEIVE_BUFFER pReceiveBuffer
				)
{
	if(pReceiveBuffer != NULL)
	{
		if(pReceiveBuffer->pReceiveMdl != NULL)
		{
			TDI_UnlockAndFreeMdl( pReceiveBuffer->pReceiveMdl );
			pReceiveBuffer->pReceiveMdl = NULL;
		}
		if(pReceiveBuffer->pReceiveBuffer != NULL)
		{
			NdisFreeMemory(pReceiveBuffer->pReceiveBuffer,0,0);
			pReceiveBuffer->pReceiveBuffer = NULL;
		}
		if(pReceiveBuffer->pReceiveIrp != NULL)
		{
			IoFreeIrp(pReceiveBuffer->pReceiveIrp);
			pReceiveBuffer->pReceiveIrp = NULL;
		}
		DebugPrint((DBG_COM, "COM_ClearReceiveBuffer(): Freed the Receive Buffer for buffer %lx and index %d\n",pReceiveBuffer,pReceiveBuffer->index));
		pReceiveBuffer->pReceiveList = NULL;
		NdisFreeMemory(pReceiveBuffer,sizeof(RECEIVE_BUFFER),0);
		pReceiveBuffer = NULL;
	}
}// COM_ClearReceiveBuffer

VOID 
COM_ClearReceiveBufferList(
					IN PRECEIVE_BUFFER_LIST pReceiveList
					)
{
	LONG lErrorCode =0;
	PLIST_ENTRY pTempEntry;
	PRECEIVE_BUFFER pReceiveBuffer = NULL;

	DebugPrint((DBG_COM, "COM_ClearReceiveBufferList(): Enter COM_ClearReceiveBufferList()\n"));

	while(!IsListEmpty(&pReceiveList->ReceiveBufferList))
	{
		pTempEntry = RemoveTailList(&pReceiveList->ReceiveBufferList);
		pReceiveBuffer = CONTAINING_RECORD(pTempEntry,RECEIVE_BUFFER,ListElement);

		DebugPrint((DBG_COM, "COM_ClearReceiveBufferList(): Calling COM_ClearReceiveBuffer() for Free buffer %lx\n",pReceiveBuffer));
		COM_ClearReceiveBuffer(pReceiveBuffer);
		pReceiveBuffer = NULL;
	}
	pReceiveList->pSessionManager = NULL;
	pReceiveList->NumberOfReceiveBuffers =0;
	DebugPrint((DBG_COM, "COM_ClearReceiveBufferList(): leaving COM_ClearReceiveBufferList()\n"));
}// COM_ClearReceiveBufferList


VOID 
COM_ClearSendBufferList(
				IN PSEND_BUFFER_LIST pSendList
				)
{
	LONG lErrorCode =0;
	PLIST_ENTRY pTempEntry;
	PSEND_BUFFER pSendBuffer = NULL;

	DebugPrint((DBG_COM, "COM_ClearSendBufferList(): Enter COM_ClearSendBufferList()\n"));
	while(!IsListEmpty(&pSendList->SendBufferList))
	{
		pTempEntry = RemoveTailList(&pSendList->SendBufferList);
		pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,ListElement);
		DebugPrint((DBG_COM, "COM_ClearSendBufferList(): calling COM_ClearSendBuffer for Free buffer %lx\n",pSendBuffer));
		COM_ClearSendBuffer(pSendBuffer);
		pSendBuffer = NULL;
	}
	pSendList->pSessionManager = NULL;
	pSendList->NumberOfSendBuffers =0;
	DebugPrint((DBG_COM, "COM_ClearSendBufferList(): Leaving COM_ClearSendBufferList()\n"));
}// COM_ClearSendBufferList

LONG 
COM_CreateNewSendBuffer(
				OUT PSEND_BUFFER* ppSendBuffer,
				IN ULONG nSendWindow
				)
{
	LONG lErrorCode=0;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PSEND_BUFFER pSendBuffer=NULL;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];


	OS_ASSERT(ppSendBuffer != NULL);
	OS_ASSERT(nSendWindow > 0);
	DebugPrint((DBG_COM, "COM_CreateNewSendBuffer(): Entering the COM_CreateNewSendBuffer()\n"));

	try
	{
		*ppSendBuffer = NULL;

		//Allocating the Session object

		nNdisStatus = NdisAllocateMemoryWithTag(
								&pSendBuffer,
								sizeof(SEND_BUFFER),
								MEM_TAG
								);

		if(!NT_SUCCESS(nNdisStatus))
		{
			swprintf(wchar1,L"%S",L"COM_CreateNewSendBuffer::SEND_BUFFER");
			swprintf(wchar2,L"%d",sizeof(SEND_BUFFER));
			swprintf(wchar3,L"0X%08X",nNdisStatus);
			sftk_LogEventString3(GSftk_Config.DriverObject,MSG_MEMORY_ALLOCATION_FAILED_ERROR,nNdisStatus,0,wchar1,wchar2,wchar3);

			DebugPrint((DBG_ERROR, "COM_CreateNewSendBuffer(): Unable to Accocate Memory for SEND_BUFFER status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pSendBuffer,sizeof(SEND_BUFFER));

	//Allocating the Send Window

		nNdisStatus = NdisAllocateMemoryWithTag(
									&pSendBuffer->pSendBuffer,
									nSendWindow,
									MEM_TAG
									);

		if(!NT_SUCCESS(nNdisStatus))
		{
			swprintf(wchar1,L"%S",L"COM_CreateNewSendBuffer::pSendBuffer->pSendBuffer");
			swprintf(wchar2,L"%d",nSendWindow);
			swprintf(wchar3,L"0X%08X",nNdisStatus);
			sftk_LogEventString3(GSftk_Config.DriverObject,MSG_MEMORY_ALLOCATION_FAILED_ERROR,nNdisStatus,0,wchar1,wchar2,wchar3);

			DebugPrint((DBG_ERROR, "COM_CreateNewSendBuffer(): Unable to Accocate Memory for SendBuffer status = %lx\n",nNdisStatus));
			leave;
		}

		//Initializing the Memory
		NdisZeroMemory(pSendBuffer->pSendBuffer,nSendWindow);
		pSendBuffer->SendWindow = nSendWindow;
		InitializeListHead(&pSendBuffer->MmHolderList);
		pSendBuffer->state = SFTK_BUFFER_FREE;
	}
	finally
	{
		if(!NT_SUCCESS(nNdisStatus))
		{
			if(pSendBuffer != NULL)
			{
				COM_ClearSendBuffer(pSendBuffer);
				pSendBuffer = NULL;
			}
			lErrorCode = 1;
		}
	}
	*ppSendBuffer = pSendBuffer;
	DebugPrint((DBG_COM, "COM_CreateNewSendBuffer(): Leaving COM_CreateNewSendBuffer()\n"));
	return lErrorCode;
}// COM_CreateNewSendBuffer

LONG 
COM_CreateNewReceiveBuffer(
					OUT PRECEIVE_BUFFER* ppReceiveBuffer,
					IN ULONG nReceiveWindow
					)
{
	LONG lErrorCode=0;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PRECEIVE_BUFFER pReceiveBuffer=NULL;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];


	OS_ASSERT(ppReceiveBuffer != NULL);
	OS_ASSERT(nReceiveWindow > 0);
	DebugPrint((DBG_COM, "COM_CreateNewReceiveBuffer(): Enter COM_CreateNewReceiveBuffer()\n"));

	try
	{
		*ppReceiveBuffer = NULL;

		//Allocating the Session object

		nNdisStatus = NdisAllocateMemoryWithTag(
								&pReceiveBuffer,
								sizeof(RECEIVE_BUFFER),
								MEM_TAG
								);

		if(!NT_SUCCESS(nNdisStatus))
		{
			swprintf(wchar1,L"%S",L"COM_CreateNewReceiveBuffer::RECEIVE_BUFFER");
			swprintf(wchar2,L"%d",sizeof(RECEIVE_BUFFER));
			swprintf(wchar3,L"0X%08X",nNdisStatus);
			sftk_LogEventString3(GSftk_Config.DriverObject,MSG_MEMORY_ALLOCATION_FAILED_ERROR,nNdisStatus,0,wchar1,wchar2,wchar3);

			DebugPrint((DBG_ERROR, "COM_CreateNewReceiveBuffer(): Unable to Accocate Memory for RECEIVE_BUFFER status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pReceiveBuffer,sizeof(RECEIVE_BUFFER));

	//Allocating the Send Window

		nNdisStatus = NdisAllocateMemoryWithTag(
									&pReceiveBuffer->pReceiveBuffer,
									nReceiveWindow,
									MEM_TAG
									);

		if(!NT_SUCCESS(nNdisStatus))
		{
			swprintf(wchar1,L"%S",L"COM_CreateNewReceiveBuffer::pReceiveBuffer->pReceiveBuffer");
			swprintf(wchar2,L"%d",nReceiveWindow);
			swprintf(wchar3,L"0X%08X",nNdisStatus);
			sftk_LogEventString3(GSftk_Config.DriverObject,MSG_MEMORY_ALLOCATION_FAILED_ERROR,nNdisStatus,0,wchar1,wchar2,wchar3);

			DebugPrint((DBG_ERROR, "COM_CreateNewReceiveBuffer(): Unable to Accocate Memory for ReceiveBuffer status = %lx\n",nNdisStatus));
			leave;
		}

		//Initializing the Memory
		NdisZeroMemory(pReceiveBuffer->pReceiveBuffer,nReceiveWindow);
		pReceiveBuffer->ReceiveWindow = nReceiveWindow;
		pReceiveBuffer->state = SFTK_BUFFER_FREE;
	}
	finally
	{
		if(!NT_SUCCESS(nNdisStatus))
		{
			if(pReceiveBuffer != NULL)
			{
				COM_ClearReceiveBuffer(pReceiveBuffer);
				pReceiveBuffer = NULL;
			}
			lErrorCode = 1;
		}
	}
	*ppReceiveBuffer = pReceiveBuffer;
	DebugPrint((DBG_COM, "COM_CreateNewReceiveBuffer(): Leaving COM_CreateNewReceiveBuffer()\n"));
	return lErrorCode;
}// COM_CreateNewReceiveBuffer


PSEND_BUFFER 
COM_AddBufferToSendList(
				IN PSEND_BUFFER_LIST pSendList
				)
{
	LONG lErrorCode = 0;
	PSEND_BUFFER pSendBuffer = NULL;
	PSFTK_LG pSftk_Lg = NULL;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSendList != NULL);
	OS_ASSERT(pSendList->pSessionManager != NULL);
	OS_ASSERT(pSendList->pSessionManager->pLogicalGroupPtr != NULL);

	pSftk_Lg  = pSendList->pSessionManager->pLogicalGroupPtr;

	DebugPrint((DBG_COM, "COM_AddBufferToSendList(): Entering COM_AddBufferToSendList()\n"));

	try
	{
		if(pSendList->NumberOfSendBuffers == pSendList->MaxNumberOfSendBuffers)
		{
			return NULL;
		}

		pSendList->SendWindow = CONFIGURABLE_MAX_SEND_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsSendAtaTime);
		lErrorCode = COM_CreateNewSendBuffer(&pSendBuffer,pSendList->SendWindow);
		if(lErrorCode == 0)
		{
			pSendBuffer->pSendList = pSendList;
			pSendList->NumberOfSendBuffers++;
			pSendBuffer->index = pSendList->NumberOfSendBuffers;
			InsertHeadList(&pSendList->SendBufferList,&pSendBuffer->ListElement);
		}
	}
	finally
	{
		DebugPrint((DBG_COM, "COM_AddBufferToSendList(): Leaving COM_AddBufferToSendList()\n"));
	}
	return pSendBuffer;
}// COM_AddBufferToSendList

PSEND_BUFFER 
COM_InsertBufferIntoSendListTail(
				IN PSEND_BUFFER_LIST pSendList,
				IN PSEND_BUFFER pSendBuffer
				)
{
	KIRQL oldIrql;

	OS_ASSERT(pSendList != NULL);
	OS_ASSERT(pSendBuffer != NULL);

	DebugPrint((DBG_COM, "COM_InsertBufferIntoSendListTail(): Entering COM_InsertBufferIntoSendListTail()\n"));

	try
	{
		// Acquire the Send Buffer List Lock
		KeAcquireSpinLock(&pSendList->SendBufferListLock , &oldIrql);

		pSendList->LastBufferIndex++;
		pSendBuffer->pSendList = pSendList;
		pSendBuffer->index = pSendList->LastBufferIndex;
		InsertTailList(&pSendList->SendBufferList,&pSendBuffer->ListElement);

		// Relese the Send Buffer List Lock
		KeReleaseSpinLock(&pSendList->SendBufferListLock , oldIrql);
	}
	finally
	{
		DebugPrint((DBG_COM, "COM_InsertBufferIntoSendListTail(): Leaving COM_InsertBufferIntoSendListTail()\n"));
	}
	return pSendBuffer;
}// COM_InsertBufferIntoSendListTail()

PRECEIVE_BUFFER
COM_InsertBufferIntoReceiveListTail(
							IN PRECEIVE_BUFFER_LIST pReceiveList,
							IN PRECEIVE_BUFFER pReceiveBuffer
							)
{
	KIRQL oldIrql;

	OS_ASSERT(pReceiveList != NULL);
	OS_ASSERT(pReceiveBuffer != NULL);

	DebugPrint((DBG_COM, "COM_InsertBufferIntoReceiveListTail(): Entering COM_InsertBufferIntoReceiveListTail()\n"));

	try
	{
		// Acquire the Send Buffer List Lock
		KeAcquireSpinLock(&pReceiveList->ReceiveBufferListLock , &oldIrql);

		pReceiveBuffer->pReceiveList = pReceiveList;
		pReceiveBuffer->index = 0;
		InsertTailList(&pReceiveList->ReceiveBufferList,&pReceiveBuffer->ListElement);

		// Relese the Send Buffer List Lock
		KeReleaseSpinLock(&pReceiveList->ReceiveBufferListLock , oldIrql);

	}
	finally
	{
		DebugPrint((DBG_COM, "COM_InsertBufferIntoReceiveListTail(): Leaving COM_InsertBufferIntoReceiveListTail()\n"));
	}
	return pReceiveBuffer;
}//COM_InsertBufferIntoReceiveListTail()


PRECEIVE_BUFFER 
COM_AddBufferToReceiveList(
					IN PRECEIVE_BUFFER_LIST pReceiveList
					)
{
	LONG lErrorCode = 0;
	PSFTK_LG pSftk_Lg = NULL;
	PRECEIVE_BUFFER pReceiveBuffer = NULL;

	OS_ASSERT(pReceiveList != NULL);
	OS_ASSERT(pReceiveList->pSessionManager != NULL);
	OS_ASSERT(pReceiveList->pSessionManager->pLogicalGroupPtr != NULL);

	DebugPrint((DBG_COM, "COM_AddBufferToReceiveList(): Enter COM_AddBufferToReceiveList()\n"));

	pSftk_Lg  = pReceiveList->pSessionManager->pLogicalGroupPtr;
	try
	{
		if(pReceiveList->NumberOfReceiveBuffers == pReceiveList->MaxNumberOfReceiveBuffers)
		{
			return NULL;
		}

		pReceiveList->ReceiveWindow = CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsRecvAtaTime);
		lErrorCode = COM_CreateNewReceiveBuffer(&pReceiveBuffer,pReceiveList->ReceiveWindow);
		if(lErrorCode == 0)
		{
			pReceiveBuffer->pReceiveList = pReceiveList;
			pReceiveList->NumberOfReceiveBuffers++;
			pReceiveBuffer->index = pReceiveList->NumberOfReceiveBuffers;
			InsertHeadList(&pReceiveList->ReceiveBufferList,&pReceiveBuffer->ListElement);
		}
	}
	finally
	{
		DebugPrint((DBG_COM, "COM_AddBufferToReceiveList(): Leave COM_AddBufferToReceiveList()\n"));
	}
	return pReceiveBuffer;
}// COM_AddBufferToReceiveList

PSEND_BUFFER 
COM_GetNextStaleSendBuffer(
					IN PSEND_BUFFER_LIST pSendList
					)
{
	PSEND_BUFFER pSendBuffer = NULL;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;

	OS_ASSERT(pSendList != NULL);

//	DebugPrint((DBG_COM, "COM_GetNextStaleSendBuffer(): Entering COM_GetNextStaleSendBuffer()\n"));

	try
	{
		// Acquire the Send Buffer List Lock
		KeAcquireSpinLock(&pSendList->SendBufferListLock , &oldIrql);

		pTempEntry = pSendList->SendBufferList.Flink;
		while(pTempEntry != &pSendList->SendBufferList)
		{
			pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,ListElement);
			pTempEntry = pTempEntry->Flink;

			if(pSendBuffer->state == SFTK_BUFFER_USED)
			{
				pSendBuffer->state = SFTK_BUFFER_INUSE;
				// Relese the Send Buffer List Lock
				KeReleaseSpinLock(&pSendList->SendBufferListLock , oldIrql);
				return pSendBuffer;
			}
		} // while(pTempEntry != &pSendList->SendBufferList)
		// Relese the Send Buffer List Lock
		KeReleaseSpinLock(&pSendList->SendBufferListLock , oldIrql);
	}
	finally
	{
//		DebugPrint((DBG_COM, "COM_GetNextStaleSendBuffer(): Leaving COM_GetNextStaleSendBuffer()\n"));
	}
return NULL;
}// COM_GetNextStaleSendBuffer

PRECEIVE_BUFFER 
COM_GetNextStaleReceiveBuffer(
						IN PRECEIVE_BUFFER_LIST pReceiveList
						)
{
	PRECEIVE_BUFFER pReceiveBuffer = NULL;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;

	OS_ASSERT(pReceiveList != NULL);
	
	DebugPrint((DBG_COM, "COM_GetNextStaleReceiveBuffer(): Entering COM_GetNextStaleReceiveBuffer()\n"));
	try
	{
		// Acquire the Receive Buffer List Lock
		KeAcquireSpinLock(&pReceiveList->ReceiveBufferListLock , &oldIrql);

		pTempEntry = pReceiveList->ReceiveBufferList.Flink;
		while(pTempEntry != &pReceiveList->ReceiveBufferList)
		{
			pReceiveBuffer = CONTAINING_RECORD(pTempEntry,RECEIVE_BUFFER,ListElement);
			pTempEntry = pTempEntry->Flink;
			if(pReceiveBuffer->state == SFTK_BUFFER_USED)
			{
				pReceiveBuffer->state = SFTK_BUFFER_INUSE;
				// Relese the Receive Buffer List Lock
				KeReleaseSpinLock(&pReceiveList->ReceiveBufferListLock , oldIrql);
				return pReceiveBuffer;
			}
		}
		// Relese the Receive Buffer List Lock
		KeReleaseSpinLock(&pReceiveList->ReceiveBufferListLock , oldIrql);
	}
	finally
	{
		DebugPrint((DBG_COM, "COM_GetNextStaleReceiveBuffer(): Leaving COM_GetNextStaleReceiveBuffer()\n"));
	}
return NULL;
}// COM_GetNextStaleReceiveBuffer

PSEND_BUFFER 
COM_GetNextSendBufferWithMM(
				 IN PSEND_BUFFER_LIST pSendList
				 )
 {
	PSEND_BUFFER pSendBuffer = NULL;
	PSESSION_MANAGER pSessionManager = NULL;
	PSFTK_LG pSftk_Lg = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSendList != NULL);
	OS_ASSERT(pSendList->pSessionManager != NULL);
	OS_ASSERT(pSendList->pSessionManager->pLogicalGroupPtr != NULL);

	pSessionManager = pSendList->pSessionManager;
	pSftk_Lg = pSessionManager->pLogicalGroupPtr;

//	DebugPrint((DBG_COM, "COM_GetNextSendBufferWithMM(): Entering COM_GetNextSendBufferWithMM()\n"));

	try{
		status = NdisAllocateMemoryWithTag(&pSendBuffer,sizeof(SEND_BUFFER),'AREV');

		if(!NT_SUCCESS(status)){
			swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
			swprintf(wchar2,L"%S",L"COM_GetNextSendBufferWithMM::SEND_BUFFER");
			swprintf(wchar3,L"%d",sizeof(SEND_BUFFER));
			swprintf(wchar4,L"0X%08X",status);
			sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_COM_MEMORY_ALLOCATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

			DebugPrint((DBG_ERROR, "COM_GetNextSendBufferWithMM(): Unable to Allocte Memory for SEND_BUFFER \n"));
			leave;
		}

		if(pSendBuffer != NULL){
			pSendList->LastBufferIndex++;
			NdisZeroMemory(pSendBuffer,sizeof(SEND_BUFFER));
			pSendBuffer->ActualSendLength = 0;
			pSendBuffer->TotalSendLength =0;
			pSendBuffer->SendWindow = pSendList->SendWindow;
			pSendBuffer->bPhysicalMemory = TRUE;
			pSendBuffer->nCancelFlag = 0;
			pSendBuffer->state = SFTK_BUFFER_INUSE;
			pSendBuffer->index = pSendList->LastBufferIndex;
			// Initialize the MM_HOLDER List and also the other List Elements
			InitializeListHead(&pSendBuffer->MmHolderList);
			InitializeListHead(&pSendBuffer->SessionListElemet);
			InitializeListHead(&pSendBuffer->ListElement);
		}else{
			KeClearEvent(&pSendList->pSessionManager->IOSendPacketsAvailableEvent);
		}
	}
	finally
	{
//		DebugPrint((DBG_COM, "COM_GetNextSendBufferWithMM(): Leaving COM_GetNextSendBufferWithMM()\n"));
	}
	return pSendBuffer;
}// COM_GetNextSendBufferWithMM

// This function tries to get the SEND_BUFFER from alist of SEND_BUFFER_LIST if it cannot find a
// Free Entry. It Creates a new SEND_BUFFER and add it to the SEND_BUFFER_LIST.
PSEND_BUFFER 
COM_GetNextSendBuffer(
				 IN PSEND_BUFFER_LIST pSendList
				 )
 {
	PSEND_BUFFER pSendBuffer = NULL;
	PSEND_BUFFER pSendBuffer1 = NULL;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;
	PSFTK_LG pSftk_Lg = NULL;
	NDIS_STATUS nNdisStatus = NDIS_STATUS_SUCCESS;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSendList != NULL);
	OS_ASSERT(pSendList->pSessionManager != NULL);
	OS_ASSERT(pSendList->pSessionManager->pLogicalGroupPtr != NULL);

	pSftk_Lg = pSendList->pSessionManager->pLogicalGroupPtr;

//	DebugPrint((DBG_COM, "COM_GetNextSendBuffer(): Entering COM_GetNextSendBuffer()\n"));

	try
	{
		// Acquire the Send Buffer List Lock
		KeAcquireSpinLock(&pSendList->SendBufferListLock , &oldIrql);

		pTempEntry = pSendList->SendBufferList.Flink;

		while(pTempEntry != &pSendList->SendBufferList)
		{
			pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,ListElement);
			pTempEntry = pTempEntry->Flink;
			if(pSendBuffer->state == SFTK_BUFFER_FREE)
			{
				pSendBuffer1 = pSendBuffer;
				break;
			}
		}

		if(pSendBuffer1 == NULL)
		{
			pSendBuffer1 = COM_AddBufferToSendList(pSendList);
		} // if(pSendBuffer1 == NULL)
		else
		{
			// We can use an existing Buffer (SEND_BUFFER) all we ahve to check is the Size of the SendWindow
			pSendList->SendWindow = CONFIGURABLE_MAX_SEND_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsSendAtaTime);

			if(pSendBuffer1->SendWindow  != pSendList->SendWindow)
			{	// The Existing Send Window of the Buffer is less than the new value so
				// let reallocate the new Buffer and release the existing Buffer
				// Free the Previous Memory

				OS_ASSERT(pSendBuffer1->pSendBuffer != NULL);
				NdisFreeMemory(pSendBuffer1->pSendBuffer,pSendBuffer1->SendWindow,0);
				pSendBuffer1->pSendBuffer = NULL;
				pSendBuffer1->SendWindow = pSendList->SendWindow;
				// Reallocating the Send Window

				nNdisStatus = NdisAllocateMemoryWithTag(
											&pSendBuffer1->pSendBuffer,
											pSendBuffer1->SendWindow,
											MEM_TAG
											);

				if(!NT_SUCCESS(nNdisStatus))
				{
					swprintf(wchar1,L"%S",L"COM_GetNextSendBuffer::pSendBuffer->pSendBuffer");
					swprintf(wchar2,L"%d",pSendBuffer1->SendWindow);
					swprintf(wchar3,L"0X%08X",nNdisStatus);
					sftk_LogEventString3(GSftk_Config.DriverObject,MSG_MEMORY_ALLOCATION_FAILED_ERROR,nNdisStatus,0,wchar1,wchar2,wchar3);

					DebugPrint((DBG_ERROR, "COM_GetNextSendBuffer(): Unable to Accocate Memory for SendBuffer status = %lx\n",nNdisStatus));

					if(pSendBuffer1 != NULL)
					{
						RemoveEntryList(&pSendBuffer1->ListElement);
						pSendList->NumberOfSendBuffers--;
						COM_ClearSendBuffer(pSendBuffer1);
						pSendBuffer1 = NULL;
					}
					leave;
				}
			}
		}// else // if(pSendBuffer1 == NULL)

		if(pSendBuffer1 != NULL)
		{
			if(pSendBuffer1->pSendBuffer != NULL)
			{
				NdisZeroMemory(pSendBuffer1->pSendBuffer, pSendBuffer1->SendWindow);
			}
			pSendList->LastBufferIndex++;

			pSendBuffer1->ActualSendLength = 0;
			pSendBuffer1->bPhysicalMemory = FALSE;
			pSendBuffer1->nCancelFlag = 0;
			pSendBuffer1->state = SFTK_BUFFER_INUSE;
			pSendBuffer1->index = pSendList->LastBufferIndex;
		}
		else
		{
			KeClearEvent(&pSendList->pSessionManager->IOSendPacketsAvailableEvent);
		}
	}
	finally
	{
		// Relese the Send Buffer List Lock
		KeReleaseSpinLock(&pSendList->SendBufferListLock , oldIrql);
//		DebugPrint((DBG_COM, "COM_GetNextSendBuffer(): Leaving COM_GetNextSendBuffer()\n"));
	}
	return pSendBuffer1;
}// COM_GetNextSendBuffer

PRECEIVE_BUFFER 
COM_GetNextReceiveBuffer(
					IN PRECEIVE_BUFFER_LIST pReceiveList
					)
{
	PRECEIVE_BUFFER pReceiveBuffer = NULL;
	PRECEIVE_BUFFER pReceiveBuffer1 = NULL;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;
	PSFTK_LG pSftk_Lg = NULL;
	NDIS_STATUS nNdisStatus = NDIS_STATUS_SUCCESS;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];


	DebugPrint((DBG_COM, "COM_GetNextReceiveBuffer(): Enter COM_GetNextReceiveBuffer()\n"));

	OS_ASSERT(pReceiveList != NULL);
	OS_ASSERT(pReceiveList->pSessionManager != NULL);
	OS_ASSERT(pReceiveList->pSessionManager->pLogicalGroupPtr != NULL);

	pSftk_Lg  = pReceiveList->pSessionManager->pLogicalGroupPtr;
	try
	{
		// Acquire the Receive Buffer List Lock
		KeAcquireSpinLock(&pReceiveList->ReceiveBufferListLock, &oldIrql);
		pTempEntry = pReceiveList->ReceiveBufferList.Flink;
		while(pTempEntry != &pReceiveList->ReceiveBufferList)
		{
			pReceiveBuffer = CONTAINING_RECORD(pTempEntry,RECEIVE_BUFFER,ListElement);
			pTempEntry = pTempEntry->Flink;
			if(pReceiveBuffer->state == SFTK_BUFFER_FREE)
			{
				pReceiveBuffer1 = pReceiveBuffer;
				break;
			}
		}

		if(pReceiveBuffer1 == NULL)
		{
			pReceiveBuffer1 = COM_AddBufferToReceiveList(pReceiveList);
		} // if(pReceiveBuffer1 == NULL)
		else
		{
			// We found the Buffer, just Check if the ReceveWindow Size is the Same or not
			pReceiveList->ReceiveWindow = CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsRecvAtaTime);

			if(pReceiveList->ReceiveWindow != pReceiveBuffer1->ReceiveWindow)
			{
				// The Receive Buffer Window Size is not the Same so Free it up and allocate
				// New Memory

				OS_ASSERT(pReceiveBuffer1->pReceiveBuffer != NULL);
				NdisFreeMemory(pReceiveBuffer1->pReceiveBuffer,pReceiveBuffer1->ReceiveWindow,0);
				pReceiveBuffer1->pReceiveBuffer = NULL;
				pReceiveBuffer1->ReceiveWindow = pReceiveList->ReceiveWindow;
				// Reallocating the Send Window

				nNdisStatus = NdisAllocateMemoryWithTag(
											&pReceiveBuffer1->pReceiveBuffer,
											pReceiveBuffer1->ReceiveWindow,
											MEM_TAG
											);

				if(!NT_SUCCESS(nNdisStatus))
				{
					swprintf(wchar1,L"%S",L"COM_GetNextReceiveBuffer::pReceiveBuffer->pReceiveBuffer");
					swprintf(wchar2,L"%d",pReceiveBuffer1->ReceiveWindow);
					swprintf(wchar3,L"0X%08X",nNdisStatus);
					sftk_LogEventString3(GSftk_Config.DriverObject,MSG_MEMORY_ALLOCATION_FAILED_ERROR,nNdisStatus,0,wchar1,wchar2,wchar3);

					DebugPrint((DBG_ERROR, "COM_GetNextReceiveBuffer(): Unable to Accocate Memory for ReceiveBuffer status = %lx\n",nNdisStatus));

					if(pReceiveBuffer1 != NULL)
					{
						RemoveEntryList(&pReceiveBuffer1->ListElement);
						pReceiveList->NumberOfReceiveBuffers--;
						COM_ClearReceiveBuffer(pReceiveBuffer1);
						pReceiveBuffer1 = NULL;
					}
					leave;
				}
			}
		}// else if(pReceiveBuffer1 == NULL)

		if(pReceiveBuffer1 != NULL)
		{
			if(pReceiveBuffer1->pReceiveBuffer != NULL)
			{
				NdisZeroMemory(pReceiveBuffer1->pReceiveBuffer, pReceiveBuffer1->ReceiveWindow);
			}
			pReceiveBuffer1->state = SFTK_BUFFER_INUSE;
		}
	}
	finally
	{
		// Relese the Receive Buffer List Lock
		KeReleaseSpinLock(&pReceiveList->ReceiveBufferListLock, oldIrql);
		DebugPrint((DBG_COM, "COM_GetNextReceiveBuffer(): Leave COM_GetNextReceiveBuffer()\n"));
	}
	
	return pReceiveBuffer1;
}// COM_GetNextReceiveBuffer


VOID 
COM_InitializeSendBufferList(
					 IN PSEND_BUFFER_LIST pSendList,
					 IN PSM_INIT_PARAMS pSessionManagerParameters
					 )
{
	PSFTK_LG pSftk_Lg = NULL;

	OS_ASSERT(pSendList != NULL);
	OS_ASSERT(pSendList->pSessionManager != NULL);
	OS_ASSERT(pSendList->pSessionManager->pLogicalGroupPtr != NULL);

	pSftk_Lg = pSendList->pSessionManager->pLogicalGroupPtr;

	// Initialize the Send Buffer List Head
	InitializeListHead(&pSendList->SendBufferList);

	// Initialize the Send Buffer List Lock
	KeInitializeSpinLock(&pSendList->SendBufferListLock);
	pSendList->NumberOfSendBuffers =0;
	pSendList->LastBufferIndex = 0;

	if(pSessionManagerParameters->nMaxNumberOfSendBuffers > 0)
	{
		pSendList->MaxNumberOfSendBuffers = pSessionManagerParameters->nMaxNumberOfSendBuffers;
	}
	else
	{
		pSendList->MaxNumberOfSendBuffers = DEFAULT_MAX_SEND_BUFFERS;
	}
	pSendList->SendWindow = CONFIGURABLE_MAX_SEND_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsSendAtaTime);;
	
}// COM_InitializeSendBufferList

VOID 
COM_InitializeReceiveBufferList(
						IN PRECEIVE_BUFFER_LIST pReceiveList,
						IN PSM_INIT_PARAMS pSessionManagerParameters
						)
{
	PSFTK_LG pSftk_Lg = NULL;

	OS_ASSERT(pReceiveList != NULL);
	OS_ASSERT(pReceiveList->pSessionManager != NULL);
	OS_ASSERT(pReceiveList->pSessionManager->pLogicalGroupPtr != NULL);

	pSftk_Lg = pReceiveList->pSessionManager->pLogicalGroupPtr;
	// Initialize the Receive Buffer List 
	InitializeListHead(&pReceiveList->ReceiveBufferList);

	// Initialize the Receive Buffer List Lock
	KeInitializeSpinLock(&pReceiveList->ReceiveBufferListLock);

	pReceiveList->NumberOfReceiveBuffers =0;

	if(pSessionManagerParameters->nMaxNumberOfReceiveBuffers > 0)
	{
		pReceiveList->MaxNumberOfReceiveBuffers = pSessionManagerParameters->nMaxNumberOfReceiveBuffers;
	}
	else
	{
		pReceiveList->MaxNumberOfReceiveBuffers = DEFAULT_MAX_RECEIVE_BUFFERS;
	}
	pReceiveList->ReceiveWindow = CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsRecvAtaTime);
}// COM_InitializeReceiveBufferList


NDIS_STATUS 
COM_GetSessionReceiveBuffer(
					IN PTCP_SESSION pSession
					)
{
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];
	PSFTK_LG pSftk_Lg = NULL;

	OS_ASSERT(pSession != NULL);
	OS_ASSERT(pSession->pReceiveBuffer != NULL);
	OS_ASSERT(pSession->pServer != NULL);
	OS_ASSERT(pSession->pServer->pSessionManager != NULL);
	OS_ASSERT(pSession->pServer->pSessionManager->pLogicalGroupPtr != NULL);

	pSftk_Lg = pSession->pServer->pSessionManager->pLogicalGroupPtr;

	DebugPrint((DBG_COM, "COM_GetSessionReceiveBuffer(): Entering COM_GetSessionReceiveBuffer()\n"));
	try
	{
		// Calculate the new Window Size that will be used
		pSession->pReceiveBuffer->ActualReceiveWindow = CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsRecvAtaTime);

		if(pSession->pReceiveBuffer->pReceiveBuffer == NULL)
		{
			pSession->pReceiveBuffer->TotalReceiveLength = pSession->pReceiveBuffer->ActualReceiveWindow;
			pSession->pReceiveBuffer->ReceiveWindow = pSession->pReceiveBuffer->ActualReceiveWindow;

			nNdisStatus = NdisAllocateMemoryWithTag(
							&pSession->pReceiveBuffer->pReceiveBuffer,
							pSession->pReceiveBuffer->ReceiveWindow,
							MEM_TAG
							);

			if(!NT_SUCCESS(nNdisStatus))
			{
				swprintf(wchar1,L"%S",L"COM_GetSessionReceiveBuffer::pSession->pReceiveBuffer->pReceiveBuffer");
				swprintf(wchar2,L"%d",pSession->pReceiveBuffer->ReceiveWindow);
				swprintf(wchar3,L"0X%08X",nNdisStatus);
				sftk_LogEventString3(GSftk_Config.DriverObject,MSG_MEMORY_ALLOCATION_FAILED_ERROR,nNdisStatus,0,wchar1,wchar2,wchar3);

				DebugPrint((DBG_ERROR, "COM_GetSessionReceiveBuffer(): Unable to Accocate Memory for Receive Window status = %lx\n",nNdisStatus));
				leave;
			}

			NdisZeroMemory(pSession->pReceiveBuffer->pReceiveBuffer,pSession->pReceiveBuffer->ReceiveWindow);
			pSession->pReceiveBuffer->index = 1;
		}
		else
		{
			// Buffer was perviously Allocated so check for the Receive Window Size's
			if(pSession->pReceiveBuffer->ActualReceiveWindow != pSession->pReceiveBuffer->ReceiveWindow)
			{
				// We found that the Buffer at the Receive Spot is not of the same size as of the Chnaged Buffer
				// So will free up the old Buffer and Reallocate the new Buffer

				OS_ASSERT(pSession->pReceiveBuffer->pReceiveBuffer != NULL);
				NdisFreeMemory(pSession->pReceiveBuffer->pReceiveBuffer,pSession->pReceiveBuffer->ReceiveWindow,0);
				pSession->pReceiveBuffer->pReceiveBuffer = NULL;
				pSession->pReceiveBuffer->TotalReceiveLength = pSession->pReceiveBuffer->ActualReceiveWindow;
				pSession->pReceiveBuffer->ReceiveWindow = pSession->pReceiveBuffer->ActualReceiveWindow;
				
				// Reallocating the Send Window

				nNdisStatus = NdisAllocateMemoryWithTag(
											&pSession->pReceiveBuffer->pReceiveBuffer,
											pSession->pReceiveBuffer->ReceiveWindow,
											MEM_TAG
											);

				if(!NT_SUCCESS(nNdisStatus))
				{
					swprintf(wchar1,L"%S",L"COM_GetSessionReceiveBuffer::pReceiveBuffer->pReceiveBuffer");
					swprintf(wchar2,L"%d",pSession->pReceiveBuffer->ReceiveWindow);
					swprintf(wchar3,L"0X%08X",nNdisStatus);
					sftk_LogEventString3(GSftk_Config.DriverObject,MSG_MEMORY_ALLOCATION_FAILED_ERROR,nNdisStatus,0,wchar1,wchar2,wchar3);

					DebugPrint((DBG_ERROR, "COM_GetSessionReceiveBuffer(): Unable to Accocate Memory for ReceiveBuffer status = %lx\n",nNdisStatus));
					leave;
				}

			}
			NdisZeroMemory(pSession->pReceiveBuffer->pReceiveBuffer,pSession->pReceiveBuffer->ReceiveWindow);
			pSession->pReceiveBuffer->index = 1;
		}
	}
	finally
	{
		DebugPrint((DBG_COM, "COM_GetSessionReceiveBuffer(): Leaving COM_GetSessionReceiveBuffer()\n"));
	}
	return nNdisStatus;
}



NTSTATUS 
COM_StartPMD(
			IN PSESSION_MANAGER pSessionManager,
			IN PSM_INIT_PARAMS pSessionManagerParameters
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE threadHandle= NULL;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	DebugPrint((DBG_COM, "COM_StartPMD(): Enter COM_StartPMD()\n"));

	OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
	OS_ASSERT( pSessionManager != NULL);
	OS_ASSERT( pSessionManagerParameters != NULL);
	OS_ASSERT( pSessionManager->pLogicalGroupPtr != NULL);

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_STARTING_PMD,STATUS_SUCCESS,0,wchar1,wchar2);

	try
	{
		status = COM_InitializeSessionManager(pSessionManager, pSessionManagerParameters);

		if(!NT_SUCCESS(status))
		{
			swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
			swprintf(wchar2,L"0X%08X",pSessionManager);
			swprintf(wchar3,L"0X%08X",status);
			sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_INITIALIZATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3);

			DebugPrint((DBG_ERROR, "COM_StartPMD(): SESSION_MANAGER initialization Failed \n"));
			leave;
		}

		pSessionManager->ConnectionType = CONNECT;

		// Enable all the Sessions before starting the Connection Thread
		COM_EnableAllSessionManager(pSessionManager);

		if(pSessionManager->pConnectListenThread == NULL)
		{
			OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

			//Creating the Connect Thread
			status = PsCreateSystemThread(
				&threadHandle,   // thread handle
			0L,                        // desired access
			NULL,                      // object attributes
			NULL,                      // process handle
			NULL,                      // client id
			COM_ConnectThread2,				//This thread has all the enhancements that are suggested
										//for the Send Thread
			(PVOID )pSessionManager            // start context
			);

			if(!NT_SUCCESS(status))
			{
				swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
				swprintf(wchar2,L"0X%08X",pSessionManager);
				swprintf(wchar3,L"%S",L"COM_StartPMD::COM_ConnectThread2");
				swprintf(wchar4,L"0X%08X",status);
				sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_CREATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

				DebugPrint((DBG_ERROR, "COM_StartPMD(): Unable to Create Connect Thread for session manager %lx\n",pSessionManager));
				leave;
			}

			status = ObReferenceObjectByHandle (
							threadHandle,      // Object Handle
							THREAD_ALL_ACCESS,                     // Desired Access
							NULL,                                // Object Type
							KernelMode,                          // Processor mode
							(PVOID *)&pSessionManager->pConnectListenThread,// Object pointer
							NULL);                               // Object Handle information

			if( !NT_SUCCESS(status) )
			{
				swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
				swprintf(wchar2,L"0X%08X",pSessionManager);
				swprintf(wchar3,L"%S",L"COM_StartPMD::COM_ConnectThread2");
				swprintf(wchar4,L"0X%08X",status);
				sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_OBJECT_REFERENCE_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

				DebugPrint((DBG_ERROR, "COM_StartPMD(): Unable to take ObReferenceObjectByHandle for Connect Thread for session manager %lx\n",pSessionManager));
				pSessionManager->pConnectListenThread = NULL;
				leave;
			}

			ZwClose(threadHandle );
			threadHandle = NULL;
		}
	}
	finally
	{
		if(threadHandle != NULL)
		{
			ZwClose(threadHandle);
			threadHandle = NULL;
		}
	}
	DebugPrint((DBG_COM, "COM_StartPMD(): Leaving COM_StartPMD()\n"));
	return status;
}// COM_StartPMD

VOID 
COM_StopPMD(
		  IN PSESSION_MANAGER pSessionManager
		  )
{
    NTSTATUS status = STATUS_SUCCESS;

	DebugPrint((DBG_COM, "COM_StopPMD(): Enter COM_StopPMD()\n"));
	COM_StopSessionManager(pSessionManager);
	DebugPrint((DBG_COM, "COM_StopPMD(): Leaving COM_StopPMD()\n"));
}// COM_StopPMD


NTSTATUS 
COM_StartRMD(
			IN PSESSION_MANAGER pSessionManager,
			IN PSM_INIT_PARAMS pSessionManagerParameters
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE threadHandle= NULL;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	DebugPrint((DBG_COM, "COM_StartRMD(): Enter COM_StartRMD()\n"));

	OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
	OS_ASSERT( pSessionManager != NULL);
	OS_ASSERT( pSessionManagerParameters != NULL);
	OS_ASSERT( pSessionManager->pLogicalGroupPtr != NULL);

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_STARTING_RMD,STATUS_SUCCESS,0,wchar1,wchar2);

	try
	{
		status = COM_InitializeSessionManager(pSessionManager, pSessionManagerParameters);

		if(!NT_SUCCESS(status))
		{
			swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
			swprintf(wchar2,L"0X%08X",pSessionManager);
			swprintf(wchar3,L"0X%08X",status);
			sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_INITIALIZATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3);

			DebugPrint((DBG_ERROR, "COM_StartRMD(): SESSION_MANAGER initialization Failed \n"));
			leave;
		}

		pSessionManager->ConnectionType = ACCEPT;

		// Enable all the Sessions before starting the Connection Thread
		COM_EnableAllSessionManager(pSessionManager);

		if(pSessionManager->pConnectListenThread == NULL)
		{
			OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

			//Creating the Connect Thread
			status = PsCreateSystemThread(
				&threadHandle,   // thread handle
			0L,                        // desired access
			NULL,                      // object attributes
			NULL,                      // process handle
			NULL,                      // client id
			COM_ListenThread2,				//This thread has all the enhancements that are suggested
										//for the Send Thread
			(PVOID )pSessionManager            // start context
			);

			if(!NT_SUCCESS(status))
			{
				swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
				swprintf(wchar2,L"0X%08X",pSessionManager);
				swprintf(wchar3,L"%S",L"COM_StartPMD::COM_ListenThread2");
				swprintf(wchar4,L"0X%08X",status);
				sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_CREATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

				DebugPrint((DBG_ERROR, "COM_StartRMD(): Unable to Create Listen Thread for session manager %lx\n",pSessionManager));
				leave;
			}

			status = ObReferenceObjectByHandle (
							threadHandle,      // Object Handle
							THREAD_ALL_ACCESS,                     // Desired Access
							NULL,                                // Object Type
							KernelMode,                          // Processor mode
							(PVOID *)&pSessionManager->pConnectListenThread,// Object pointer
							NULL);                               // Object Handle information

			if( !NT_SUCCESS(status) )
			{
				swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
				swprintf(wchar2,L"0X%08X",pSessionManager);
				swprintf(wchar3,L"%S",L"COM_StartPMD::COM_ListenThread2");
				swprintf(wchar4,L"0X%08X",status);
				sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_OBJECT_REFERENCE_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

				DebugPrint((DBG_ERROR, "COM_StartRMD(): Unable to take ObReferenceObjectByHandle for Listen Thread for session manager %lx\n",pSessionManager));
				pSessionManager->pConnectListenThread = NULL;
				leave;
			}

			ZwClose(threadHandle );
			threadHandle = NULL;
		}
	}
	finally
	{
		if(threadHandle != NULL)
		{
			ZwClose(threadHandle);
			threadHandle = NULL;
		}
	}
	DebugPrint((DBG_COM, "COM_StartRMD(): Leaving COM_StartRMD()\n"));
	return status;
}// COM_StartRMD


VOID 
COM_StopRMD(
		  IN PSESSION_MANAGER pSessionManager
		  )
{
    NTSTATUS status = STATUS_SUCCESS;

	DebugPrint((DBG_COM, "COM_StopRMD(): Enter COM_StopRMD()\n"));
	COM_StopSessionManager(pSessionManager);
	DebugPrint((DBG_COM, "COM_StopRMD(): Leaving COM_StopRMD()\n"));
}// COM_StopRMD


NTSTATUS 
COM_CreateSendReceiveThreads(
						IN PSESSION_MANAGER pSessionManager
						)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE threadHandle= NULL;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	DebugPrint((DBG_COM, "COM_CreateSendReceiveThreads(): Enter COM_CreateSendReceiveThreads()\n"));

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);
	try
	{
#define _SEND_THREAD_

#ifdef _SEND_THREAD_

		if(pSessionManager->pSendThreadObject == NULL)
		{
			OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

			//Creating the Send Thread
			status = PsCreateSystemThread(
				&threadHandle,   // thread handle
			0L,                        // desired access
			NULL,                      // object attributes
			NULL,                      // process handle
			NULL,                      // client id
			COM_SendThread2,				//This thread has all the enhancements that are suggested
										//for the Send Thread
			(PVOID )pSessionManager            // start context
			);

			if(!NT_SUCCESS(status))
			{
				swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
				swprintf(wchar2,L"0X%08X",pSessionManager);
				swprintf(wchar3,L"%S",L"COM_StartPMD::COM_SendThread2");
				swprintf(wchar4,L"0X%08X",status);
				sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_CREATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

				DebugPrint((DBG_ERROR, "COM_CreateSendReceiveThreads(): Unable to Create Send Thread for session manager %lx\n",pSessionManager));
				leave;
			}

			status = ObReferenceObjectByHandle (
							threadHandle,      // Object Handle
							THREAD_ALL_ACCESS,                     // Desired Access
							NULL,                                // Object Type
							KernelMode,                          // Processor mode
							(PVOID *)&pSessionManager->pSendThreadObject,// Object pointer
							NULL);                               // Object Handle information

			if( !NT_SUCCESS(status) )
			{
				swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
				swprintf(wchar2,L"0X%08X",pSessionManager);
				swprintf(wchar3,L"%S",L"COM_StartPMD::COM_SendThread2");
				swprintf(wchar4,L"0X%08X",status);
				sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_OBJECT_REFERENCE_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

				OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
				pSessionManager->pSendThreadObject = NULL;
				leave;
			}

			ZwClose(threadHandle );
			threadHandle = NULL;
		}

#endif //#ifdef_SEND_THREAD_
//Use the receive thread only if the receive Event is not registered

//#define _USE_RECEIVE_EVENT_
#ifndef _USE_RECEIVE_EVENT_

		if(pSessionManager->pReceiveThreadObject == NULL)
		{
			OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
			//Creating the Receive Thread
			status = PsCreateSystemThread(
				&threadHandle,   // thread handle
				0L,                        // desired access
				NULL,                      // object attributes
				NULL,                      // process handle
				NULL,                      // client id
				COM_ReceiveThread2,				//This thread has all the changes that are requested
				(PVOID )pSessionManager            // start context
				);

			if(!NT_SUCCESS(status))
			{
				swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
				swprintf(wchar2,L"0X%08X",pSessionManager);
				swprintf(wchar3,L"%S",L"COM_StartPMD::COM_ReceiveThread2");
				swprintf(wchar4,L"0X%08X",status);
				sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_CREATION_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

				DebugPrint((DBG_ERROR, "COM_CreateSendReceiveThreads(): Unable to Create Receive Thread for sessionmanager %lx\n",pSessionManager));
				leave;
			}

			status = ObReferenceObjectByHandle (
							threadHandle,      // Object Handle
							THREAD_ALL_ACCESS,                     // Desired Access
							NULL,                                // Object Type
							KernelMode,                          // Processor mode
							(PVOID *)&pSessionManager->pReceiveThreadObject,// Object pointer
							NULL);                               // Object Handle information

			if( !NT_SUCCESS(status) )
			{
				swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
				swprintf(wchar2,L"0X%08X",pSessionManager);
				swprintf(wchar3,L"%S",L"COM_StartPMD::COM_ReceiveThread2");
				swprintf(wchar4,L"0X%08X",status);
				sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_OBJECT_REFERENCE_FAILED_ERROR,status,0,wchar1,wchar2,wchar3,wchar4);

				OS_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
				pSessionManager->pReceiveThreadObject = NULL;
				leave;
			}
			ZwClose(threadHandle);
			threadHandle = NULL;

		}
#endif //_USE_RECEIVE_EVENT_
//#undef _USE_RECEIVE_EVENT_
	}
	finally
	{
		if(!NT_SUCCESS(status))
		{
			COM_StopSendReceiveThreads(pSessionManager);
		}
		if(threadHandle != NULL)
		{
			ZwClose(threadHandle);
			threadHandle = NULL;
		}
	}
	DebugPrint((DBG_COM, "COM_CreateSendReceiveThreads(): Leveing COM_CreateSendReceiveThreads()\n"));
	return status;
}// COM_CreateSendReceiveThreads


VOID 
COM_StopSendReceiveThreads(
					  IN PSESSION_MANAGER pSessionManager
					  )
{
    NTSTATUS status = STATUS_SUCCESS;

	DebugPrint((DBG_COM, "COM_StopSendReceiveThreads(): Enter COM_StopSendReceiveThreads()\n"));
	KeSetEvent(&pSessionManager->IOExitEvent,0,FALSE);
	if(pSessionManager->pSendThreadObject != NULL)
	{
		status = KeWaitForSingleObject(pSessionManager->pSendThreadObject,Executive,KernelMode,FALSE,NULL);
		if(status == STATUS_SUCCESS)
		{
			DebugPrint((DBG_COM, "COM_StopSendReceiveThreads(): The SendThread() Successfully Exited\n"));
			ObDereferenceObject(pSessionManager->pSendThreadObject);
			pSessionManager->pSendThreadObject = NULL;
		}
	}
	if(pSessionManager->pReceiveThreadObject != NULL)
	{
		status = KeWaitForSingleObject(pSessionManager->pReceiveThreadObject,Executive,KernelMode,FALSE,NULL);
		if(status == STATUS_SUCCESS)
		{
			DebugPrint((DBG_COM, "COM_StopSendReceiveThreads(): The ReceiveThread() Successfully Exited\n"));
			ObDereferenceObject(pSessionManager->pReceiveThreadObject);
			pSessionManager->pReceiveThreadObject = NULL;
		}
	}
	KeResetEvent(&pSessionManager->IOExitEvent);
	DebugPrint((DBG_COM, "COM_StopSendReceiveThreads(): Leaving COM_StopSendReceiveThreads()\n"));
}// COM_StopSendReceiveThreads


//Registering Handlers for Plug and Play for the Network Address and Binding Info.
//Only works in Win2000 and above.
#if _WIN32_WINNT >= 0x0500
NTSTATUS 
TDI_RegisterPnpHandlers(
				)
{
	return STATUS_SUCCESS;
}
#endif

//Code with Optimizations
//This code will dynamically add remove sessions, the lock used is the exclusive lock
//ERESOURCE, Whenever a session needs to be added, the lock is obtained and then a link
//is either added or deleted from the sessions.
//A session is defined as { hostname , hostport , remotename , remoteport }
//Optionally N number of sessions might be added and removed as required.
#define __NEW_CONNECT__
#ifdef __NEW_CONNECT__

//This function will return the List of Connections that meet the Criteria mentioned in the
//ConnectionDetails, Also It uses the bEnabled flag to specify if the Connections Specified 
//in the ConnectionDetails are Enabled or Disabled
//The Connections that match the specified properties like same 
//{SourceIP,SourcePort,TargetIP,TargetPort} will be returned in the pServerList
//The status of the Operation is returned.
//If no Connections meet the specified requirements then we return STATUS_INVALID_PARAMETER
//This is Purely internal, and any caller who is calling this function has to get the
//Exclusive Lock Before Calling thsi function

NTSTATUS 
COM_FindConnectionsCount(
					IN PSESSION_MANAGER pSessionManager ,
					IN PCONNECTION_DETAILS pConnectionDetails , 
					IN BOOLEAN bEnabled, 
					OUT PULONG pServerCount, 
					OUT PLIST_ENTRY pServerList
					)
{
	NTSTATUS lErrorCode=STATUS_SUCCESS;
	NDIS_STATUS ndisStatus = STATUS_SUCCESS;
	LONG i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	PLIST_ENTRY pTempEntry1;
	ULONG nServers = 0;
	LONG *pFoundIndexs = NULL;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];
	

	OS_ASSERT(pSessionManager);

	if(!pSessionManager->bInitialized)
	{
		DebugPrint((DBG_ERROR, "COM_FindConnectionsCount(): The SESSION_MANAGER is not initialized so cannot Find connections\n"));
		return STATUS_INVALID_PARAMETER;
	}

	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);
	DebugPrint((DBG_COM, "COM_FindConnectionsCount(): Entering COM_FindConnectionsCount()\n"));

	if(pConnectionDetails == NULL)
	{
		DebugPrint((DBG_ERROR, "COM_FindConnectionsCount(): The Connection Details Parameter is NULL\n"));
		return STATUS_INVALID_PARAMETER;
	}
	OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	try
	{
		ndisStatus = NdisAllocateMemoryWithTag(
						&pFoundIndexs,
						pConnectionDetails->nConnections * sizeof(LONG),
						MEM_TAG
						);

		if(!NT_SUCCESS(ndisStatus))
		{
			swprintf(wchar1,L"%S",L"COM_FindConnectionsCount::pFoundIndexs");
			swprintf(wchar2,L"%d",pConnectionDetails->nConnections * sizeof(LONG));
			swprintf(wchar3,L"0X%08X",ndisStatus);
			sftk_LogEventString3(GSftk_Config.DriverObject,MSG_MEMORY_ALLOCATION_FAILED_ERROR,ndisStatus,0,wchar1,wchar2,wchar3);

			DebugPrint((DBG_ERROR, "COM_FindConnectionsCount(): Unable to allocate Memory for the Long Array\n"));
			lErrorCode = STATUS_MEMORY_NOT_ALLOCATED;
			leave;
		}

		NdisZeroMemory(pFoundIndexs,pConnectionDetails->nConnections * sizeof(LONG));

		*pServerCount = 0;

		pTempEntry1 = pSessionManager->ServerList.Flink;
		while(pTempEntry1 != &pSessionManager->ServerList)
		{
			pConnectionInfo = pConnectionDetails->ConnectionDetails;
			pServer = CONTAINING_RECORD(pTempEntry1,SERVER_ELEMENT,ListElement);
			pTempEntry1 = pTempEntry1->Flink;

			OS_ASSERT(pServer);
			pSession = pServer->pSession;
			OS_ASSERT(pSession);
			
			for(i = 0 ; i < pConnectionDetails->nConnections && 
						nServers < pConnectionDetails->nConnections ; i++)
			{
				if(pFoundIndexs[i] == 1)
				{
					pConnectionInfo+=1;
					continue;
				}
				//Find the Session That Meets all the Criteria Specified
				if((pServer->LocalAddress.Address[0].Address[0].in_addr == pConnectionInfo->ipLocalAddress.in_addr) &&
					(pServer->LocalAddress.Address[0].Address[0].sin_port == pConnectionInfo->ipLocalAddress.sin_port) &&
					(pSession->RemoteAddress.Address[0].Address[0].in_addr == pConnectionInfo->ipRemoteAddress.in_addr) &&
					(pSession->RemoteAddress.Address[0].Address[0].sin_port == pConnectionInfo->ipRemoteAddress.sin_port) &&
					(pServer->bIsEnabled == bEnabled))
				{
					//Increment the Server Count
					InsertTailList(pServerList,&pServer->ServerListElement);
					pFoundIndexs[i] = 1;
					nServers++;
					break;
				}//if
				pConnectionInfo+=1;
			}//for

			if(nServers == pConnectionDetails->nConnections)
			{
				DebugPrint((DBG_COM, "COM_FindConnectionsCount(): Found the Required Number of Servers So leaving %d \n",nServers));
				break;
			}
			//Goto the next Element
		}//while

		*pServerCount = nServers;
		//If we cannot find the number of servers that meet our criteria
		//We return Error
		if(nServers != pConnectionDetails->nConnections)
		{
			DebugPrint((DBG_ERROR, "COM_FindConnectionsCount(): The Number of Connections %d didnt match the Number of Servers %d\n",
				pConnectionDetails->nConnections,
				nServers));
			lErrorCode = STATUS_INVALID_PARAMETER;
			leave;
		}
	}//try
	finally
	{
		if(pFoundIndexs != NULL)
		{
			NdisFreeMemory(pFoundIndexs , pConnectionDetails->nConnections * sizeof(LONG) , 0);
			pFoundIndexs = NULL;
		}
	}
	DebugPrint((DBG_COM, "COM_FindConnectionsCount(): Leaving COM_FindConnectionsCount()"));
	return lErrorCode;
}// COM_FindConnectionsCount

NTSTATUS 
COM_AddConnections(
			IN PSESSION_MANAGER pSessionManager ,
			IN PCONNECTION_DETAILS pConnectionDetails
			)
{
	NTSTATUS lErrorCode=STATUS_SUCCESS;
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	LIST_ENTRY tempServerList;
	PLIST_ENTRY pTempEntry1;
	ULONG uRemoteIP = 0;
	USHORT uRemotePort = 0;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pConnectionDetails != NULL);

	if(!pSessionManager->bInitialized)
	{
		DebugPrint((DBG_ERROR, "COM_AddConnections(): The SESSION_MANAGER is not initialized so cannot Add connections\n"));
		return STATUS_INVALID_PARAMETER;
	}
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	uRemoteIP = pConnectionDetails->ConnectionDetails[0].ipRemoteAddress.in_addr;
	uRemotePort = pConnectionDetails->ConnectionDetails[0].ipRemoteAddress.sin_port;

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	SM_CONVERT_IP_PORT_TO_STRING_W(wchar3,uRemoteIP,uRemotePort);
	sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_ADD_CONNECTIONS,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);

	DebugPrint((DBG_COM, "COM_AddConnections(): Entering COM_AddConnections()\n"));


	OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	try
	{
		//This will have the Temperory Created Server List, if anything 
		//fails here we will just release the resources and dont change 
		//anything in the existing Server List

		InitializeListHead(&tempServerList);
		pConnectionInfo = pConnectionDetails->ConnectionDetails;

		for(i=0;i<pConnectionDetails->nConnections;i++)
		{
		
			pServer = NULL;

			if(pServer == NULL)
			{
				lErrorCode = COM_AllocateServer(&pServer,pConnectionInfo);
				if(!NT_SUCCESS(lErrorCode))
				{
					DebugPrint((DBG_ERROR, "COM_AddConnections(): Unable to Allocate Server\n"));
					leave;
				}

				InsertTailList(&tempServerList,&pServer->ListElement);
//				InsertTailList(&pSessionManager->ServerList,&pServer->ListElement);
				pServer->pSessionManager = pSessionManager;
			}

			OS_ASSERT(pServer);

			pSession = NULL;

			//Just Ignore the NumberOfSessions Parameter and Set it to 1
			//Let only one session be there per connection
//			if(pConnectionInfo->nNumberOfSessions > 1)
//			{
				pConnectionInfo->nNumberOfSessions = 1;
//			}
			for(j=0;j<pConnectionInfo->nNumberOfSessions;j++)
			{
				pSession = NULL;

				lErrorCode = COM_AllocateSession(&pSession,pServer,pConnectionInfo);

				if(!NT_SUCCESS(lErrorCode))
				{
					DebugPrint((DBG_ERROR, "COM_AddConnections(): Unable to Allocate the Session Pointer\n"));
					leave;
				}
				pServer->pSession = pSession;
			}
			pConnectionInfo+=1;
		}

		//Check if the new Server List is Empty or not
		if(tempServerList.Flink == &tempServerList)
		{
			DebugPrint((DBG_COM, "COM_AddConnections(): There are no Elements in the Server List So Leaving\n"));
			leave;
		}

		// Acquire the resource as shared Lock
		// This Resource will not be acquired if there is a call for Exclusive Lock
		KeEnterCriticalRegion();	//Disable the Kernel APC's
		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		//Insert this New List at the end of the Server List

		pTempEntry1 = pSessionManager->ServerList.Blink;
		pSessionManager->ServerList.Blink = tempServerList.Blink;
		tempServerList.Blink->Flink = &pSessionManager->ServerList;
		tempServerList.Flink->Blink = pTempEntry1;
		pTempEntry1->Flink = tempServerList.Flink;

		pSessionManager->nTotalSessions += pConnectionDetails->nConnections;
		//Release the Resource 
		ExReleaseResourceLite(&pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	//Enable the Kernel APC's

	}
	finally
	{
		if(!NT_SUCCESS(lErrorCode))
		{
			swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
			swprintf(wchar2,L"0X%08X",pSessionManager);
			SM_CONVERT_IP_PORT_TO_STRING_W(wchar3,uRemoteIP,uRemotePort);
			swprintf(wchar4,L"0X%08X",lErrorCode);
			sftk_LogEventWchar4(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_ADD_CONNECTIONS_ERROR,lErrorCode,0,wchar1,wchar2,wchar3,wchar4);

			COM_FreeConnections(&tempServerList);
		}
	}
	DebugPrint((DBG_COM, "COM_AddConnections(): Leaving COM_AddConnections()"));
	return lErrorCode;
}// COM_AddConnections


//This function will enable OR disable sessions based on the Connection Details sent by the 
//User
NTSTATUS 
COM_EnableConnections(
				IN PSESSION_MANAGER pSessionManager ,
				IN PCONNECTION_DETAILS pConnectionDetails, 
				IN BOOLEAN bEnable
				)
{
	NTSTATUS lErrorCode=STATUS_SUCCESS;
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	LIST_ENTRY serverList;
	PLIST_ENTRY pTempEntry1;
	ULONG nServers =0;
	ULONG uRemoteIP = 0;
	USHORT uRemotePort = 0;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];


	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pConnectionDetails != NULL);

	if(!pSessionManager->bInitialized)
	{
		DebugPrint((DBG_ERROR, "COM_EnableConnections(): The SESSION_MANAGER is not initialized so cannot enable connections\n"));
		return STATUS_INVALID_PARAMETER;
	}
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	uRemoteIP = pConnectionDetails->ConnectionDetails[0].ipRemoteAddress.in_addr;
	uRemotePort = pConnectionDetails->ConnectionDetails[0].ipRemoteAddress.sin_port;

	if(bEnable)
	{
		swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
		swprintf(wchar2,L"0X%08X",pSessionManager);
		SM_CONVERT_IP_PORT_TO_STRING_W(wchar3,uRemoteIP,uRemotePort);
		sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_ENABLE_CONNECTIONS,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);
	}
	else
	{
		swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
		swprintf(wchar2,L"0X%08X",pSessionManager);
		SM_CONVERT_IP_PORT_TO_STRING_W(wchar3,uRemoteIP,uRemotePort);
		sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_DISABLE_CONNECTIONS,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);
	}


	DebugPrint((DBG_COM, "COM_EnableConnections(): Entering COM_EnableConnections()\n"));


	OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	InitializeListHead(&serverList);

	try
	{
		// Acquire the resource as shared Lock
		// This Resource will not be acquired if there is a call for Exclusive Lock
		KeEnterCriticalRegion();	//Disable the Kernel APC's
		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		//Find all the Servers that are not yet initialized
		lErrorCode = COM_FindConnectionsCount(pSessionManager,pConnectionDetails,!bEnable,&nServers,&serverList);
		if(!NT_SUCCESS(lErrorCode))
		{
			DebugPrint((DBG_ERROR, "COM_EnableConnections(): Cannot find the Required Servers so leaving\n"));
			leave;
		}
		pTempEntry1 = serverList.Flink;
		while(pTempEntry1 != &serverList)
		{
			//Get the Server Item and set the Enable to TRUE|FALSE
			pServer = CONTAINING_RECORD(pTempEntry1,SERVER_ELEMENT,ServerListElement);
			pTempEntry1 = pTempEntry1->Flink;
			OS_ASSERT(pServer);
			//Enable the Entry This will cause the Connect thread to Initialize the Server and
			//Connect to the Remote Side or Listen on this Server
			pServer->bEnable = bEnable;
		}
	}
	finally
	{
		//Release the Resource 
		ExReleaseResourceLite(&pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	//Enable the Kernel APC's
	}
	DebugPrint((DBG_COM, "COM_EnableConnections(): Leaving COM_EnableConnections()"));
	return lErrorCode;
}// COM_EnableConnections

NTSTATUS 
COM_RemoveConnections(
				IN PSESSION_MANAGER pSessionManager ,
				IN PCONNECTION_DETAILS pConnectionDetails
				)
{
	NTSTATUS lErrorCode=STATUS_SUCCESS;
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	LIST_ENTRY serverList;
	PLIST_ENTRY pTempEntry1;
	ULONG nServers =0;
	ULONG uRemoteIP = 0;
	USHORT uRemotePort = 0;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pConnectionDetails != NULL);

	if(!pSessionManager->bInitialized)
	{
		DebugPrint((DBG_ERROR, "COM_RemoveConnections(): The SESSION_MANAGER is not initialized so cannot Remove connections\n"));
		return STATUS_INVALID_PARAMETER;
	}
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	uRemoteIP = pConnectionDetails->ConnectionDetails[0].ipRemoteAddress.in_addr;
	uRemotePort = pConnectionDetails->ConnectionDetails[0].ipRemoteAddress.sin_port;

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	SM_CONVERT_IP_PORT_TO_STRING_W(wchar3,uRemoteIP,uRemotePort);
	sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_REMOVE_CONNECTIONS,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);


	DebugPrint((DBG_COM, "COM_RemoveConnections(): Entering COM_RemoveConnections()\n"));


	OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	InitializeListHead(&serverList);

	try
	{
		// Acquire the resource as shared Lock
		// This Resource will not be acquired if there is a call for Exclusive Lock
		KeEnterCriticalRegion();	//Disable the Kernel APC's
		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		//Find all the Servers that are not yet initialized
		lErrorCode = COM_FindConnectionsCount(pSessionManager,pConnectionDetails,FALSE,&nServers,&serverList);
		if(!NT_SUCCESS(lErrorCode))
		{
			DebugPrint((DBG_ERROR, "COM_RemoveConnections(): Cannot find the Required Servers so leaving\n"));
			leave;
		}//if

		//Loop through the ServerList That is Disabled and remove the SERVER_ELEMENT
		//Elements from the Server List in the SESSION_MANAGER
		pTempEntry1 = serverList.Flink;
		while(pTempEntry1 != &serverList)
		{
			// Get the Server Item and set the Enable to TRUE|FALSE
			pServer = CONTAINING_RECORD(pTempEntry1,SERVER_ELEMENT,ServerListElement);
			pTempEntry1 = pTempEntry1->Flink;
			OS_ASSERT(pServer);
			// Check if all the Cleanup happened correctly
			OS_ASSERT(pServer->pSession->bSessionEstablished == SFTK_UNINITIALIZED);
			// Get the Next Link Before you delete the
			// existing one
			// Remove this element from the ServerList.
			RemoveEntryList(&pServer->ListElement);
			// Decrement the TotalSessions in the SESSION_MANAGER
			pSessionManager->nTotalSessions--;
			COM_FreeServer(pServer);
			pServer = NULL;
		}//while
	}//try
	finally
	{
		//Release the Resource 
		ExReleaseResourceLite(&pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	//Enable the Kernel APC's
	}//finally
	DebugPrint((DBG_COM, "COM_RemoveConnections(): Leaving COM_RemoveConnections()"));
	return lErrorCode;
}// COM_RemoveConnections

//This function Frees the Sessions from SESSION_MANAGER
VOID 
COM_FreeConnections(
			IN PLIST_ENTRY pServerList
			)
{
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	PLIST_ENTRY pTempEntry;

	DebugPrint((DBG_COM, "COM_FreeConnections(): Entering COM_FreeConnections()\n"));

	OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	try
	{
		pTempEntry = pServerList->Flink;

		while(!IsListEmpty(pServerList))
		{
			pTempEntry = RemoveHeadList(pServerList);
			pServer = CONTAINING_RECORD(pTempEntry,SERVER_ELEMENT,ListElement);
			OS_ASSERT(pServer);
			COM_FreeServer(pServer);
			pServer = NULL;
		}
	}
	finally
	{
	}
	DebugPrint((DBG_COM, "COM_FreeConnections(): Leaving COM_FreeConnections()"));
}// COM_FreeConnections


NTSTATUS 
COM_AllocateServer(
			OUT PSERVER_ELEMENT *ppServer,
			IN PCONNECTION_INFO pConnectionInfo
			)
{

	NTSTATUS lErrorCode = STATUS_SUCCESS;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PSERVER_ELEMENT pServer = NULL;

	OS_ASSERT(ppServer);
	DebugPrint((DBG_COM, "COM_AllocateServer(): Entering COM_AllocateServer()\n"));
	try
	{
		*ppServer = NULL;
		nNdisStatus = NdisAllocateMemoryWithTag(
								&pServer,
								sizeof(SERVER_ELEMENT),
								MEM_TAG
								);
		if(!NT_SUCCESS(nNdisStatus))
		{
			DebugPrint((DBG_ERROR, "COM_AllocateServer(): Unable to Accocate Memory for SERVER_ELEMENT status = %lx\n",nNdisStatus));
			leave;
		}
		NdisZeroMemory(pServer,sizeof(SERVER_ELEMENT));
		TDI_InitIPAddress(&pServer->LocalAddress,pConnectionInfo->ipLocalAddress.in_addr,pConnectionInfo->ipLocalAddress.sin_port);

		// Initialize the Session IO Exit Event
		KeInitializeEvent(&pServer->SessionIOExitEvent,NotificationEvent,FALSE);
		// Initialize the Send Lock Fast Mutex
//		ExInitializeFastMutex(&pServer->SessionSendLock);
		// Initialze the Receive Fast Mutex
//		ExInitializeFastMutex(&pServer->SessionReceiveLock);
	}
	finally
	{
		if(!NT_SUCCESS(nNdisStatus))
		{
			COM_FreeServer(pServer);
			pServer =NULL;
			lErrorCode = 1;
		}
	}
	*ppServer = pServer;

	DebugPrint((DBG_COM, "COM_AllocateServer(): Leaving COM_AllocateServer()"));
	return lErrorCode;
}// COM_AllocateServer

//Free up the Server and the Session Associated with it
VOID 
COM_FreeServer(
		IN PSERVER_ELEMENT pServer
		)
{
	if(pServer != NULL)
	{
		if(pServer->pSession != NULL)
		{
			COM_FreeSession(pServer->pSession);
			pServer->pSession = NULL;
		}
		NdisFreeMemory(pServer,sizeof(SERVER_ELEMENT),0);
	}
}// COM_FreeServer

//Free up the Allocated Session Memory
VOID 
COM_FreeSession(
		IN PTCP_SESSION pSession
		)
{
	if(pSession != NULL)
	{
		NdisFreeMemory(pSession,sizeof(TCP_SESSION),0);
	}
}// COM_FreeSession

NTSTATUS 
COM_AllocateSession(
			OUT PTCP_SESSION *ppSession, 
			IN PSERVER_ELEMENT pServer, 
			IN PCONNECTION_INFO pConnectionInfo
			)
{
	LONG lErrorCode=STATUS_SUCCESS;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PTCP_SESSION pSession = NULL;

	OS_ASSERT(ppSession);
	OS_ASSERT(pServer);

	DebugPrint((DBG_COM, "COM_AllocateSession(): Enetring COM_AllocateSession()\n"));
	try
	{
		*ppSession = NULL;
		//Allocating the Session object
		nNdisStatus = NdisAllocateMemoryWithTag(
								&pSession,
								sizeof(TCP_SESSION),
								MEM_TAG
								);
		if(!NT_SUCCESS(nNdisStatus))
		{
			DebugPrint((DBG_ERROR, "COM_AllocateSession(): Unable to Accocate Memory for TCP_SESSION status = %lx\n",nNdisStatus));
			leave;
		}
		NdisZeroMemory(pSession,sizeof(TCP_SESSION));

		TDI_InitIPAddress(
			&pSession->RemoteAddress,
			pConnectionInfo->ipRemoteAddress.in_addr,
			pConnectionInfo->ipRemoteAddress.sin_port
				);
		//Initialize all other parameters in the Initialize Session
		pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
		pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
		pSession->bSessionEstablished = SFTK_UNINITIALIZED;
		//Assign the Server Pointer
		pSession->pServer = pServer;

		InitializeListHead(&pSession->sendIrpList);

		KeInitializeSpinLock(&pSession->sendIrpListLock);

		//Initialize the receive Event, This will be trigered whenever 
		//there is data on the wire to be received
		KeInitializeEvent(&pSession->IOReceiveEvent,SynchronizationEvent,FALSE);

	}
	finally
	{
		if(!NT_SUCCESS(nNdisStatus))
		{
			COM_FreeSession(pSession);
			lErrorCode = 1;
			pSession = NULL;
		}
	}

	*ppSession = pSession;

	DebugPrint((DBG_COM, "COM_AllocateSession(): Leaving COM_AllocateSession()\n"));
	return lErrorCode;
}// COM_AllocateSession

LONG 
COM_StartSession(
				IN PTCP_SESSION pSession, 
				IN eSessionType type
				)
{
	LONG lErrorCode=0;
	NTSTATUS status = STATUS_SUCCESS;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT pDeviceObject = NULL;
	PSFTK_LG pSftk_Lg = NULL;

	OS_ASSERT(pSession != NULL);
	OS_ASSERT(pSession->pServer != NULL);
	OS_ASSERT(pSession->pServer->pSessionManager != NULL);
	OS_ASSERT(pSession->pServer->pSessionManager->pLogicalGroupPtr != NULL);

	pSftk_Lg = pSession->pServer->pSessionManager->pLogicalGroupPtr;

	try
	{

		

		//Initialize the Connection End Point
		status = TDI_OpenConnectionEndpoint(
						TCP_DEVICE_NAME_W,
						&pSession->pServer->KSAddress,
						&pSession->KSEndpoint,
						&pSession->KSEndpoint    // Context
						);

		if( !NT_SUCCESS( status ) )
		{
			//
			// Connection Endpoint Could Not Be Created
			//
			DebugPrint((DBG_COM, "COM_StartSession(): Open Connection EndPoint Failed for session %lx\n",pSession));
			leave;
		}

		DebugPrint((DBG_COM, "COM_StartSession(): Created Local TDI Connection Endpoint\n") );
		DebugPrint((DBG_COM, "COM_StartSession(): pSession: 0x%8.8X; pAddress: 0x%8.8X; pConnection: 0x%8.8X\n",
			(ULONG )pSession,
			(ULONG )&pSession->pServer->KSAddress,
			(ULONG )&pSession->KSEndpoint
			));

#define _USE_RECEIVE_EVENT_
#ifndef _USE_RECEIVE_EVENT_
		//
		// Setup Event Handlers On The Address Object
		//
		status = TDI_SetEventHandlers(
						&pSession->pServer->KSAddress,
						pSession,      // Event Context
						NULL,          // ConnectEventHandler
						TDI_DisconnectEventHandler,
						TDI_ErrorEventHandler,
						//TCPC_ReceiveEventHandler,
						NULL,
						NULL,          // ReceiveDatagramEventHandler
						TDI_ReceiveExpeditedEventHandler
						);

		if( !NT_SUCCESS( status ) )
		{
			DebugPrint((DBG_ERROR, "COM_StartSession(): Failed To Set the Event Handlers\n"));
			leave;
			//
			// Event Handlers Could Not Be Set
			//
		}
#else
		//
		// Setup Event Handlers On The Address Object
		//
		status = TDI_SetEventHandlers(
						&pSession->pServer->KSAddress,
						pSession,      // Event Context
						NULL,          // ConnectEventHandler
						TDI_DisconnectEventHandler,
						TDI_ErrorEventHandler,	
						TDI_ReceiveEventHandler,	//Receive Handler is enabled
						NULL,          // ReceiveDatagramEventHandler
						TDI_ReceiveExpeditedEventHandler
						);

		if( !NT_SUCCESS( status ) )
		{
			DebugPrint((DBG_ERROR, "COM_StartSession(): Failed To Set the Event Handlers\n"));
			leave;
			//
			// Event Handlers Could Not Be Set
			//
		}

#endif //_USE_RECEIVE_EVENT_

		DebugPrint((DBG_COM, "COM_StartSession(): Set Event Handlers On The Address Object\n") );
		pDeviceObject = IoGetRelatedDeviceObject(
							pSession->KSEndpoint.m_pFileObject
							);


		if(type == CONNECT)
		{
			DebugPrint((DBG_COM, "COM_StartSession(): The CONNECT Initialzation\n"));

			pSession->pConnectIrp = IoAllocateIrp(
										pDeviceObject->StackSize,
										FALSE
										);

			if( !pSession->pConnectIrp )
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				leave;
			}
		}
		if(type == ACCEPT)
		{
			//
			// Allocate Irp For Use In Listening For A Connection
			//
			pSession->pListenIrp = IoAllocateIrp(
										pDeviceObject->StackSize,
										FALSE
										);

			if( !pSession->pListenIrp )
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				leave;
			}

		//
			// Build The Listen Request
		//
			pSession->RemoteConnectionInfo.UserDataLength = 0;
			pSession->RemoteConnectionInfo.UserData = NULL;

			pSession->RemoteConnectionInfo.OptionsLength = 0;
			pSession->RemoteConnectionInfo.Options = NULL;

			pSession->RemoteConnectionInfo.RemoteAddressLength = sizeof( TA_IP_ADDRESS );
			pSession->RemoteConnectionInfo.RemoteAddress = &pSession->RemoteAddress;

		}

		//Allocating the Receive Header object

		nNdisStatus = NdisAllocateMemoryWithTag(
								&pSession->pReceiveHeader,
								sizeof(RECEIVE_BUFFER),
								MEM_TAG
								);

		if(!NT_SUCCESS(nNdisStatus))
		{
			DebugPrint((DBG_ERROR, "COM_StartSession(): Unable to Accocate Memory for RECEIVE_BUFFER status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pSession->pReceiveHeader,sizeof(RECEIVE_BUFFER));


		nNdisStatus = NdisAllocateMemoryWithTag(
								&pSession->pReceiveHeader->pReceiveBuffer,
								sizeof(ftd_header_t),
								MEM_TAG
								);

		if(!NT_SUCCESS(nNdisStatus))
		{
			DebugPrint((DBG_ERROR, "COM_StartSession(): Unable to Accocate Memory for ftd_header_t status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pSession->pReceiveHeader->pReceiveBuffer,sizeof(ftd_header_t));

		pSession->pReceiveHeader->ActualReceiveLength	= 0;
		pSession->pReceiveHeader->ActualReceiveWindow	= sizeof(ftd_header_t);
		pSession->pReceiveHeader->ReceiveWindow			= sizeof(ftd_header_t);
		pSession->pReceiveHeader->TotalReceiveLength	= sizeof(ftd_header_t);
		pSession->pReceiveHeader->state					= SFTK_BUFFER_FREE;
		pSession->pReceiveHeader->pSession				= pSession;

		//Allocating the Receive Data Buffer

		nNdisStatus = NdisAllocateMemoryWithTag(
								&pSession->pReceiveBuffer,
								sizeof(RECEIVE_BUFFER),
								MEM_TAG
								);

		if(!NT_SUCCESS(nNdisStatus))
		{
			DebugPrint((DBG_ERROR, "COM_StartSession(): Unable to Accocate Memory for RECEIVE_BUFFER status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pSession->pReceiveBuffer,sizeof(RECEIVE_BUFFER));

		//Allocate pSession->pReceiveBuffer->pReceiveBuffer Later When it is actually Reqd

		pSession->pReceiveBuffer->ActualReceiveLength	= 0;
		pSession->pReceiveBuffer->ActualReceiveWindow	= CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsRecvAtaTime);
		pSession->pReceiveBuffer->TotalReceiveLength	= pSession->pReceiveBuffer->ActualReceiveWindow;
		pSession->pReceiveBuffer->ReceiveWindow			= pSession->pReceiveBuffer->ActualReceiveWindow;
		pSession->pReceiveBuffer->state					= SFTK_BUFFER_FREE;
		pSession->pReceiveBuffer->pSession				= pSession;

		pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
		pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
		pSession->bSessionEstablished = SFTK_DISCONNECTED;
	}//try
	finally
	{
		if(!NT_SUCCESS(nNdisStatus)|| !NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "COM_StartSession(): The Memory Allocation for function COM_StartSession() So Cleaning up\n"));

			if(pSession != NULL)
			{
				if(pSession->pListenIrp != NULL)
				{
					IoFreeIrp( pSession->pListenIrp );
					pSession->pListenIrp = NULL;
				}//if

				if(pSession->pConnectIrp != NULL)
				{
					IoFreeIrp( pSession->pConnectIrp );
					pSession->pConnectIrp = NULL;
				}//if

				if(pSession->pReceiveHeader != NULL)
				{
					COM_ClearReceiveBuffer(pSession->pReceiveHeader);
					pSession->pReceiveHeader = NULL;
				}//if
				if(pSession->pReceiveBuffer != NULL)
				{
					COM_ClearReceiveBuffer(pSession->pReceiveBuffer);
					pSession->pReceiveBuffer = NULL;
				}//if
				pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
				pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
			}//if
		}//if
	}//finally
	//Return the Error status
	return nNdisStatus | status;
}// COM_StartSession

NTSTATUS 
COM_StartServer(
				IN PSERVER_ELEMENT pServer, 
				IN eSessionType type
				)
{
	NTSTATUS status = STATUS_SUCCESS;

	OS_ASSERT(pServer != NULL);

	// Check if the IPAddress and the Port information is initialized or not
	OS_ASSERT(pServer->LocalAddress.TAAddressCount > 0); 


	try
	{
		KeEnterCriticalRegion();	//Disable the APC's
		// Acquire the resource as exclusive Lock
		ExAcquireResourceExclusiveLite(&pServer->pSessionManager->ServerListLock,TRUE);

		//
		// Open Transport Address
		//
		status = TDI_OpenTransportAddress(
				TCP_DEVICE_NAME_W,
				(PTRANSPORT_ADDRESS )&pServer->LocalAddress,
				&pServer->KSAddress
				);

		if( !NT_SUCCESS( status ) )
		{
			DebugPrint((DBG_ERROR, "COM_StartServer(): Transport Address Couldnt be opened  for IPAddr %d and Port %d So Quitting\n",pServer->LocalAddress.Address[0].Address[0].in_addr, pServer->LocalAddress.Address[0].Address[0].sin_port));
			//
			// Address Object Could Not Be Created
			//
			leave;
		}
		status = COM_StartSession(pServer->pSession, type);
	}
	finally
	{
		if(!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "COM_StartServer(): The OpenTransportAddress() Failed\n"));
			if(pServer != NULL)
			{
				TDI_CloseTransportAddress( &pServer->KSAddress);
			}
		}

		//Release the Resource 
		ExReleaseResourceLite(&pServer->pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	// Enable the APC's
	}
	return status;
}// COM_StartServer

NTSTATUS 
COM_StartServerExclusive(
				IN PSERVER_ELEMENT pServer, 
				IN eSessionType type
				)
{
	NTSTATUS status = STATUS_SUCCESS;

	OS_ASSERT(pServer != NULL);

	// Check if the IPAddress and the Port information is initialized or not
	OS_ASSERT(pServer->LocalAddress.TAAddressCount > 0); 


	try
	{
		KeEnterCriticalRegion();	//Disable the APC's
		// Acquire the resource as exclusive Lock
		ExAcquireResourceExclusiveLite(&pServer->pSessionManager->ServerListLock,TRUE);

		//
		// Open Transport Address
		//
		status = TDI_OpenTransportAddress(
				TCP_DEVICE_NAME_W,
				(PTRANSPORT_ADDRESS )&pServer->LocalAddress,
				&pServer->KSAddress
				);

		if( !NT_SUCCESS( status ) )
		{
			DebugPrint((DBG_ERROR, "COM_StartServerExclusive(): Transport Address Couldnt be opened  for IPAddr %d and Port %d So Quitting\n",pServer->LocalAddress.Address[0].Address[0].in_addr, pServer->LocalAddress.Address[0].Address[0].sin_port));
			//
			// Address Object Could Not Be Created
			//
			leave;
		}
		status = COM_StartSession(pServer->pSession, type);
	}
	finally
	{
		if(!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "COM_StartServerExclusive(): The OpenTransportAddress() Failed\n"));
			if(pServer != NULL)
			{
				TDI_CloseTransportAddress( &pServer->KSAddress);
			}
		}

		//Release the Resource 
		ExReleaseResourceLite(&pServer->pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	// Enable the APC's
	}
	return status;
}// COM_StartServerExclusive



// Use this function only if you are Resetting the whole SESSION_MANAGER ie.. when the Send receive
// Connect Listen Threads are down, so that there is no need for additional locking

VOID 
COM_UninitializeSession1(
					IN PTCP_SESSION pSession
					)
{
	LONG lErrorCode =0;
	NTSTATUS status = STATUS_SUCCESS;
	LARGE_INTEGER DelayTime;
	LONG nLiveSessions = 0;

	// Check to be sure this is part of the Uninitialization code
	// as all the Connect | Listen , Send , Receive Threads should be stopped
	// before we call this.

	OS_ASSERT(pSession != NULL);
	OS_ASSERT(pSession->pServer->pSessionManager->pConnectListenThread == NULL);
	OS_ASSERT(pSession->pServer->pSessionManager->pSendThreadObject == NULL);
	OS_ASSERT(pSession->pServer->pSessionManager->pReceiveThreadObject == NULL);

	try
	{

		DebugPrint((DBG_COM, "COM_UninitializeSession1(): pSession->bSessionEstablished for session %lx\n",pSession));

		if(pSession->pListenIrp != NULL)
		{
			IoCancelIrp(pSession->pListenIrp);
		}

		if(pSession->pConnectIrp != NULL)
		{
			IoCancelIrp(pSession->pConnectIrp);
		}

		if(pSession->pReceiveHeader != NULL)
		{
			if(pSession->pReceiveHeader->pReceiveIrp != NULL)
			{
				IoCancelIrp(pSession->pReceiveHeader->pReceiveIrp);
			}
		}

		if(pSession->bSessionEstablished == SFTK_CONNECTED)
		{
			DebugPrint((DBG_COM, "COM_UninitializeSession1(): Before calling the TDI_Disconnect for session %lx\n",pSession));
			status = TDI_Disconnect(
				&pSession->KSEndpoint,
				NULL,    // UserCompletionEvent
				NULL,    // UserCompletionRoutine
				NULL,    // UserCompletionContext
				NULL,    // pIoStatusBlock
				0        // Disconnect Flags
				);

			nLiveSessions = InterlockedExchange(	&pSession->pServer->pSessionManager->nLiveSessions, 
													pSession->pServer->pSessionManager->nLiveSessions);
			if (nLiveSessions > 0)
			{
				nLiveSessions = InterlockedDecrement(&pSession->pServer->pSessionManager->nLiveSessions);
				pSession->bSessionEstablished = SFTK_DISCONNECTED;

				if(nLiveSessions ==0)
				{
					DebugPrint((DBG_CONNECT, "TDI_DisconnectEventHandler(): All the Connections for this SESSION_MANAGER are Down\n"));
					KeSetEvent(&pSession->pServer->pSessionManager->GroupDisconnectedEvent,0,FALSE);
					KeClearEvent(&pSession->pServer->pSessionManager->GroupConnectedEvent);
				}
			}
		}

		DebugPrint((DBG_COM, "COM_UninitializeSession1(): Waiting After Disconnect...\n" ));

		DelayTime.QuadPart = 10*1000*50;   // 50 MilliSeconds

		KeDelayExecutionThread( KernelMode, FALSE, &DelayTime );

//		pSession->bSessionEstablished = SFTK_DISCONNECTED;
		TDI_CloseConnectionEndpoint( &pSession->KSEndpoint );

//		if(pSession->pListenIrp != NULL)
//		{
//			IoFreeIrp( pSession->pListenIrp );
//			pSession->pListenIrp = NULL;
//		}//if

		if(pSession->pReceiveHeader != NULL)
		{
			COM_ClearReceiveBuffer(pSession->pReceiveHeader);
			pSession->pReceiveHeader = NULL;
		}//if
		if(pSession->pReceiveBuffer != NULL)
		{
			COM_ClearReceiveBuffer(pSession->pReceiveBuffer);
			pSession->pReceiveBuffer = NULL;
		}//if

		pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
		pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
		pSession->bSessionEstablished = SFTK_UNINITIALIZED;

		pSession->pServer->bEnable = FALSE;
		pSession->pServer->bIsEnabled = FALSE;
	}//try
	finally
	{

	}//finally
}// COM_UninitializeSession1

VOID COM_UninitializeServer1(
						IN PSERVER_ELEMENT pServer
						)
{
	LONG lErrorCode =0;
	NTSTATUS status = STATUS_SUCCESS;

	OS_ASSERT(pServer != NULL);

	if(pServer->pSession != NULL)
	{
		DebugPrint((DBG_COM, "COM_UninitializeServer1(): Calling pSession %lx\n",pServer->pSession));
		COM_UninitializeSession1(pServer->pSession);
	}
	TDI_CloseTransportAddress( &pServer->KSAddress);
}// COM_UninitializeServer1

// Resets the Particular connection, this function will first grabs an excluse lock and then
// it tries to cancel all the IRPs that are pending on this Connection, and then the disconnect 
// the Connection and free up session Receive Buffers


VOID COM_StopServer(
					IN PSERVER_ELEMENT pServer
					)
{
	NTSTATUS status = STATUS_SUCCESS;
	PLIST_ENTRY pTempEntry;
	LARGE_INTEGER DelayTime;
	KIRQL oldIrql;
	PSEND_BUFFER pSendBuffer = NULL;
	LONG nLiveSessions = 0;

	OS_ASSERT(pServer != NULL);
	OS_ASSERT(pServer->pSession != NULL);

	try
	{
		DebugPrint((DBG_COM, "COM_StopServer(): Entering COM_StopServer()\n"));

		KeEnterCriticalRegion();	//Disable the APC's
		// Acquire the resource as exclusive Lock
		ExAcquireResourceExclusiveLite(&pServer->pSessionManager->ServerListLock,TRUE);

		DebugPrint((DBG_COM, "COM_StopServer(): pSession->bSessionEstablished for session %lx\n",pServer->pSession));

		if(pServer->pSession->pListenIrp != NULL)
		{
			IoCancelIrp(pServer->pSession->pListenIrp);
		}

		if(pServer->pSession->pConnectIrp != NULL)
		{
			IoCancelIrp(pServer->pSession->pConnectIrp);
		}

		// Cancel All the Send Buffer IRP's and Free up the IRP's
		// To avoid a deadlock just set the CancelFalg in the Buffer to 1 
		// and then Cancel that irp, so that the CancleRoutinue doesnt try to get the 

		KeAcquireSpinLock(&pServer->pSession->sendIrpListLock , &oldIrql);
		while(!IsListEmpty(&pServer->pSession->sendIrpList))
		{
			pTempEntry = RemoveHeadList(&pServer->pSession->sendIrpList);
			pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,SessionListElemet);

			if(pSendBuffer->pSendIrp != NULL)
			{
				// Set the Cancel Falg so that we dont acquire the spinlock in IoCancelIrp
				pSendBuffer->nCancelFlag = 1;
				IoCancelIrp(pSendBuffer->pSendIrp);
			}
		}
		KeReleaseSpinLock(&pServer->pSession->sendIrpListLock , oldIrql);

		if(pServer->pSession->pReceiveHeader != NULL && pServer->pSession->pReceiveHeader->pReceiveIrp != NULL)
		{
			IoCancelIrp(pServer->pSession->pReceiveHeader->pReceiveIrp);
		}

		if(pServer->pSession->bSessionEstablished == SFTK_CONNECTED)
		{
			DebugPrint((DBG_COM, "COM_StopServer(): Before calling the TDI_Disconnect for session %lx\n",pServer->pSession));
			status = TDI_Disconnect(
				&pServer->pSession->KSEndpoint,
				NULL,    // UserCompletionEvent
				NULL,    // UserCompletionRoutine
				NULL,    // UserCompletionContext
				NULL,    // pIoStatusBlock
				0        // Disconnect Flags
				);

			nLiveSessions = InterlockedExchange(	&pServer->pSessionManager->nLiveSessions, 
													pServer->pSessionManager->nLiveSessions);
			if (nLiveSessions > 0)
			{
				nLiveSessions = InterlockedDecrement(&pServer->pSessionManager->nLiveSessions);
				pServer->pSession->bSessionEstablished = SFTK_DISCONNECTED;

				if(nLiveSessions ==0)
				{
					DebugPrint((DBG_CONNECT, "TDI_DisconnectEventHandler(): All the Connections for this SESSION_MANAGER are Down\n"));
					KeSetEvent(&pServer->pSessionManager->GroupDisconnectedEvent,0,FALSE);
					KeClearEvent(&pServer->pSessionManager->GroupConnectedEvent);
				}
			}

		}

		DebugPrint((DBG_COM, "COM_UninitializeSession1(): Disconnect status: 0x%8.8x for Session %d\n", status, pServer->pSession->sessionID ));

		DebugPrint((DBG_COM, "COM_StopServer(): Waiting After Disconnect...\n" ));

		DelayTime.QuadPart = 10*1000*50;   // 50 MilliSeconds

		KeDelayExecutionThread( KernelMode, FALSE, &DelayTime );

	//		pSession->bSessionEstablished = SFTK_DISCONNECTED;
		TDI_CloseConnectionEndpoint( &pServer->pSession->KSEndpoint );

		if(pServer->pSession->pReceiveHeader != NULL)
		{
			COM_ClearReceiveBuffer(pServer->pSession->pReceiveHeader);
			pServer->pSession->pReceiveHeader = NULL;
		}//if
		if(pServer->pSession->pReceiveBuffer != NULL)
		{
			COM_ClearReceiveBuffer(pServer->pSession->pReceiveBuffer);
			pServer->pSession->pReceiveBuffer = NULL;
		}//if
		pServer->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
		pServer->pSession->bSessionEstablished = SFTK_UNINITIALIZED;

		pServer->bEnable = FALSE;
		pServer->bIsEnabled = FALSE;
		pServer->bReset = FALSE;

		TDI_CloseTransportAddress( &pServer->KSAddress);
	}
	finally
	{
		//Release the Resource 
		ExReleaseResourceLite(&pServer->pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	// Enable the APC's

		// Stop the Send and Receive Thread for this Server if they exist Wait until they exit
		// This is applicable if the Server has independent Send and Receive Threads.
		COM_StopSendReceiveThreadForServer(pServer);
	}


	DebugPrint((DBG_COM, "COM_StopServer(): Leaving COM_StopServer()\n"));
}// COM_StopServer		//APC's are disabled in this function call


VOID COM_StopServerExclusive(
					IN PSERVER_ELEMENT pServer
					)
{
	NTSTATUS status = STATUS_SUCCESS;
	PLIST_ENTRY pTempEntry;
	LARGE_INTEGER DelayTime;
	KIRQL oldIrql;
	PSEND_BUFFER pSendBuffer = NULL;
	LONG nLiveSessions = 0;

	OS_ASSERT(pServer != NULL);
	OS_ASSERT(pServer->pSession != NULL);

	try
	{
		DebugPrint((DBG_COM, "COM_StopServerExclusive(): Entering COM_StopServer()\n"));

		KeEnterCriticalRegion();	//Disable the APC's
		// Acquire the resource as exclusive Lock
		ExAcquireResourceExclusiveLite(&pServer->pSessionManager->ServerListLock,TRUE);

		DebugPrint((DBG_COM, "COM_StopServerExclusive(): pSession->bSessionEstablished for session %lx\n",pServer->pSession));


		if(pServer->pSession->pListenIrp != NULL)
		{
			IoCancelIrp(pServer->pSession->pListenIrp);
		}

		if(pServer->pSession->pConnectIrp != NULL)
		{
			IoCancelIrp(pServer->pSession->pConnectIrp);
		}

		// Cancel All the Send Buffer IRP's and Free up the IRP's
		// To avoid a deadlock just set the CancelFalg in the Buffer to 1 
		// and then Cancel that irp, so that the CancleRoutinue doesnt try to get the 

		KeAcquireSpinLock(&pServer->pSession->sendIrpListLock , &oldIrql);
		while(!IsListEmpty(&pServer->pSession->sendIrpList))
		{
			pTempEntry = RemoveHeadList(&pServer->pSession->sendIrpList);
			pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,SessionListElemet);

			if(pSendBuffer->pSendIrp != NULL)
			{
				// Set the Cancel Falg so that we dont acquire the spinlock in IoCancelIrp
				pSendBuffer->nCancelFlag = 1;
				IoCancelIrp(pSendBuffer->pSendIrp);
			}
		}
		KeReleaseSpinLock(&pServer->pSession->sendIrpListLock , oldIrql);

		if(pServer->pSession->pReceiveHeader != NULL && pServer->pSession->pReceiveHeader->pReceiveIrp != NULL)
		{
			IoCancelIrp(pServer->pSession->pReceiveHeader->pReceiveIrp);
		}

		if(pServer->pSession->bSessionEstablished == SFTK_CONNECTED)
		{
			DebugPrint((DBG_COM, "COM_StopServerExclusive(): Before calling the TDI_Disconnect for session %lx\n",pServer->pSession));
			status = TDI_Disconnect(
				&pServer->pSession->KSEndpoint,
				NULL,    // UserCompletionEvent
				NULL,    // UserCompletionRoutine
				NULL,    // UserCompletionContext
				NULL,    // pIoStatusBlock
				0        // Disconnect Flags
				);

			nLiveSessions = InterlockedExchange(	&pServer->pSessionManager->nLiveSessions, 
													pServer->pSessionManager->nLiveSessions);
			if (nLiveSessions > 0)
			{
				nLiveSessions = InterlockedDecrement(&pServer->pSessionManager->nLiveSessions);
				pServer->pSession->bSessionEstablished = SFTK_DISCONNECTED;

				if(nLiveSessions ==0)
				{
					DebugPrint((DBG_CONNECT, "TDI_DisconnectEventHandler(): All the Connections for this SESSION_MANAGER are Down\n"));
					KeSetEvent(&pServer->pSessionManager->GroupDisconnectedEvent,0,FALSE);
					KeClearEvent(&pServer->pSessionManager->GroupConnectedEvent);
				}
			}
		}

		DebugPrint((DBG_COM, "COM_StopServerExclusive(): Disconnect status: 0x%8.8x for Session %d\n", status, pServer->pSession->sessionID ));

		DebugPrint((DBG_COM, "COM_StopServerExclusive(): Waiting After Disconnect...\n" ));

		DelayTime.QuadPart = 10*1000*50;   // 50 MilliSeconds

		KeDelayExecutionThread( KernelMode, FALSE, &DelayTime );

	//		pSession->bSessionEstablished = SFTK_DISCONNECTED;
		TDI_CloseConnectionEndpoint( &pServer->pSession->KSEndpoint );

		if(pServer->pSession->pReceiveHeader != NULL)
		{
			COM_ClearReceiveBuffer(pServer->pSession->pReceiveHeader);
			pServer->pSession->pReceiveHeader = NULL;
		}//if
		if(pServer->pSession->pReceiveBuffer != NULL)
		{
			COM_ClearReceiveBuffer(pServer->pSession->pReceiveBuffer);
			pServer->pSession->pReceiveBuffer = NULL;
		}//if
		pServer->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
		pServer->pSession->bSessionEstablished = SFTK_UNINITIALIZED;

		pServer->bEnable = FALSE;
		pServer->bIsEnabled = FALSE;

		TDI_CloseTransportAddress( &pServer->KSAddress);
	}
	finally
	{
		//Release the Resource 
		ExReleaseResourceLite(&pServer->pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	// Enable the APC's

		// Stop the Send and Receive Thread for this Server if they exist Wait until they exit
		// This is applicable if the Server has independent Send and Receive Threads.
		COM_StopSendReceiveThreadForServer(pServer);
	}


	DebugPrint((DBG_COM, "COM_StopServerExclusive(): Leaving COM_StopServer()\n"));
}// COM_StopServerExclusive		//APC's are disabled in this function call



//This function Just Initializes the Session Manager
NTSTATUS 
COM_InitializeSessionManager(
						IN PSESSION_MANAGER pSessionManager,		
						IN PSM_INIT_PARAMS pSessionManagerParameters
						)
{
	NTSTATUS lErrorCode = STATUS_SUCCESS;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager);
	OS_ASSERT(pSessionManagerParameters);

	// The pSessionManagerParameters->nChunkSize and the pSessionManagerParameters->nChunkDelay
	// can be 0, if not supported

	DebugPrint((DBG_COM, "COM_InitializeSessionManager(): Entering COM_InitializeSessionManager()\n"));

	if(pSessionManager->bInitialized)
	{
		OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);
		DebugPrint((DBG_ERROR, "COM_InitializeSessionManager(): Session Manager already initialized\n"));
		return lErrorCode;
	}

	try
	{
		OS_ZeroMemory(pSessionManager,sizeof(SESSION_MANAGER));
		// Retrieve SFTK_LG from LgNum
#if TARGET_SIDE
		pSessionManager->pLogicalGroupPtr = sftk_lookup_lg_by_lgnum( pSessionManagerParameters->lgnum, pSessionManagerParameters->lgCreationRole );
#else
		pSessionManager->pLogicalGroupPtr = sftk_lookup_lg_by_lgnum( pSessionManagerParameters->lgnum );
#endif
		if (pSessionManager->pLogicalGroupPtr == NULL) 
		{ // LG Already exist
			lErrorCode = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "COM_InitializeSessionManager : sftk_lookup_lg_by_lgnum(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pSessionManagerParameters->lgnum, lErrorCode)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			return lErrorCode;
		}

		swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
		swprintf(wchar2,L"0X%08X",pSessionManager);
		sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_INITIALIZING,STATUS_SUCCESS,0,wchar1,wchar2);

		InitializeListHead(&pSessionManager->ServerList);

		pSessionManager->sendBufferList.pSessionManager = pSessionManager;
		COM_InitializeSendBufferList(&pSessionManager->sendBufferList,pSessionManagerParameters);
		
		pSessionManager->receiveBufferList.pSessionManager = pSessionManager;
		COM_InitializeReceiveBufferList(&pSessionManager->receiveBufferList,pSessionManagerParameters);

		pSessionManager->nTotalSessions = 0;
		pSessionManager->nLiveSessions =0;

		//Initialize the Group uninitialize Event
		KeInitializeEvent(&pSessionManager->GroupUninitializedEvent,NotificationEvent,FALSE);
		//Initializes the Connection Event
		KeInitializeEvent(&pSessionManager->GroupConnectedEvent,NotificationEvent,FALSE);
		// Initialize the Disconneced Event
		KeInitializeEvent(&pSessionManager->GroupDisconnectedEvent,NotificationEvent,TRUE);
		//Initialize the Send Receive Exit Event
		KeInitializeEvent(&pSessionManager->IOExitEvent,NotificationEvent,FALSE);
		//Initialize the Send Receive Exit Event
		KeInitializeEvent(&pSessionManager->HandshakeEvent,NotificationEvent,FALSE);

		KeInitializeEvent(&pSessionManager->IOSendPacketsAvailableEvent,NotificationEvent,FALSE);

		//Initialize the Lock that is used to provide Shared Access 
		ExInitializeResourceLite(&pSessionManager->ServerListLock);

		InitializeListHead(&pSessionManager->ProtocolQueue);

		KeInitializeSpinLock(&pSessionManager->StatisticsLock);

		pSessionManager->pSyncReceivedEvent = NULL;
		pSessionManager->LastPacketSentTime = 0;
		pSessionManager->LastPacketReceiveTime = 0;

		pSessionManager->bSendHandshakeInformation = TRUE;
		pSessionManager->Reset = FALSE;
		pSessionManager->Stop = FALSE; 

//		KeInitializeSpinLock(&pSessionManager->ProtocolQueueLock);

		//Initialize the Send Lock
//		ExInitializeFastMutex(&pSessionManager->SendLock);
		//Initialize the Receive Lock
//		ExInitializeFastMutex(&pSessionManager->ReceiveLock);

		pSessionManager->nChunkSize = pSessionManagerParameters->nChunkSize;
		pSessionManager->nChunkDelay = pSessionManagerParameters->nChunkDelay;
		// This indicates if the Send and the Receive Buffers will be allocated directly from NonPagedPool
		// Or the Memory Manager is used to get all the Memory Allocations.
		pSessionManager->UseMemoryManager = FALSE;
	}
	finally
	{
	}
	pSessionManager->bInitialized = 1;
	DebugPrint((DBG_COM, "COM_InitializeSessionManager(): Leaving COM_InitializeSessionManager()\n"));
	return lErrorCode;
}// COM_InitializeSessionManager

VOID 
COM_ClearReceiveBufferList1(
					IN PRECEIVE_BUFFER_LIST pReceiveList
					)
{
	PLIST_ENTRY pTempEntry;
	PRECEIVE_BUFFER pReceiveBuffer = NULL;
	KIRQL oldIrql;

	DebugPrint((DBG_COM, "COM_ClearReceiveBufferList1(): Enter COM_ClearReceiveBufferList1()\n"));

	KeAcquireSpinLock(&pReceiveList->ReceiveBufferListLock , &oldIrql);
	while(!IsListEmpty(&pReceiveList->ReceiveBufferList))
	{
		pTempEntry = RemoveTailList(&pReceiveList->ReceiveBufferList);
		pReceiveBuffer = CONTAINING_RECORD(pTempEntry,RECEIVE_BUFFER,ListElement);
		DebugPrint((DBG_COM, "COM_ClearReceiveBufferList1(): Calling COM_ClearReceiveBuffer1() for Free buffer %lx\n",pReceiveBuffer));
		COM_ClearReceiveBuffer(pReceiveBuffer);
		pReceiveBuffer = NULL;
	}
	pReceiveList->NumberOfReceiveBuffers =0;
	KeReleaseSpinLock(&pReceiveList->ReceiveBufferListLock , oldIrql);
	DebugPrint((DBG_COM, "COM_ClearReceiveBufferList1(): leaving COM_ClearReceiveBufferList1()\n"));
}// COM_ClearReceiveBufferList1

VOID 
COM_ClearSendBufferList1(
				IN PSEND_BUFFER_LIST pSendList
				)
{
	PLIST_ENTRY pTempEntry;
	PSEND_BUFFER pSendBuffer = NULL;
	KIRQL oldIrql;

	DebugPrint((DBG_COM, "COM_ClearSendBufferList1(): Enter COM_ClearSendBufferList1()\n"));

	KeAcquireSpinLock(&pSendList->SendBufferListLock , &oldIrql);
	while(!IsListEmpty(&pSendList->SendBufferList))
	{
		pTempEntry = RemoveTailList(&pSendList->SendBufferList);
		pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,ListElement);
		DebugPrint((DBG_COM, "COM_ClearSendBufferList1(): calling COM_ClearSendBuffer1 for Free buffer %lx\n",pSendBuffer));
		COM_ClearSendBuffer(pSendBuffer);
		pSendBuffer = NULL;
	}
	pSendList->NumberOfSendBuffers =0;
	KeReleaseSpinLock(&pSendList->SendBufferListLock , oldIrql);
	DebugPrint((DBG_COM, "COM_ClearSendBufferList1(): Leaving COM_ClearSendBufferList1()\n"));
}// COM_ClearSendBufferList1


VOID 
COM_CancelSendIRPs(
				IN PSEND_BUFFER_LIST pSendList
				)
{
	PLIST_ENTRY pTempEntry;
	PSEND_BUFFER pSendBuffer = NULL;

	DebugPrint((DBG_COM, "COM_CancelSendIRPs(): Enter COM_CancelSendIRPs()\n"));

	pTempEntry = pSendList->SendBufferList.Flink;
	while(pTempEntry != &pSendList->SendBufferList)
	{
		pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,ListElement);
		pTempEntry = pTempEntry->Flink;
		// Check if the Send IRP is Not Null
		//If the Previous Value of pSendBuffer->pSendIrp is not NULL
		if(pSendBuffer->pSendIrp)
		{
			IoCancelIrp(pSendBuffer->pSendIrp);
		}//if
	}//while
	DebugPrint((DBG_COM, "COM_CancelSendIRPs(): Leaving COM_CancelSendIRPs()\n"));
}// COM_CancelSendIRPs


VOID 
COM_CancelReceiveIRPs(
				IN PRECEIVE_BUFFER_LIST pReceiveList
				)
{
	PLIST_ENTRY pTempEntry;
	PRECEIVE_BUFFER pReceiveBuffer = NULL;

	DebugPrint((DBG_COM, "COM_CancelReceiveIRPs(): Enter COM_CancelReceiveIRPs()\n"));

	pTempEntry = pReceiveList->ReceiveBufferList.Flink;
	while(pTempEntry != &pReceiveList->ReceiveBufferList)
	{
		pReceiveBuffer = CONTAINING_RECORD(pTempEntry,RECEIVE_BUFFER,ListElement);
		pTempEntry = pTempEntry->Flink;
		// Check if the Receive IRP is Not Null
		//If the Previous Value of pReceiveBuffer->pReceiveIrp is not NULL
		if(pReceiveBuffer->pReceiveIrp)
		{
			IoCancelIrp(pReceiveBuffer->pReceiveIrp);
		}//if
	}//while
	DebugPrint((DBG_COM, "COM_CancelReceiveIRPs(): Leaving COM_CancelReceiveIRPs()\n"));
}// COM_CancelReceiveIRPs

VOID COM_ResetServer(
					IN PSERVER_ELEMENT pServer
					)
{
	NTSTATUS status					= STATUS_SUCCESS;
	PDEVICE_OBJECT pDeviceObject	= NULL;
	PLIST_ENTRY pTempEntry;
	LARGE_INTEGER DelayTime;
	KIRQL oldIrql;
	PSEND_BUFFER pSendBuffer		= NULL;
	LONG nLiveSessions = 0;

	OS_ASSERT(pServer != NULL);
	OS_ASSERT(pServer->pSession != NULL);

	try
	{
		DebugPrint((DBG_COM, "COM_ResetServer(): Entering COM_ResetServer()\n"));

		KeEnterCriticalRegion();	//Disable the APC's
		// Acquire the resource as exclusive Lock
		ExAcquireResourceExclusiveLite(&pServer->pSessionManager->ServerListLock,TRUE);

		DebugPrint((DBG_COM, "COM_ResetServer(): pSession->bSessionEstablished for session %lx\n",pServer->pSession));

		pServer->bReset = FALSE;

		if(pServer->pSession->pListenIrp != NULL)
		{
			IoCancelIrp(pServer->pSession->pListenIrp);
		}

		if(pServer->pSession->pConnectIrp != NULL)
		{
			IoCancelIrp(pServer->pSession->pConnectIrp);
		}

		// Cancel All the Send Buffer IRP's and Free up the IRP's
		// To avoid a deadlock just set the CancelFalg in the Buffer to 1 
		// and then Cancel that irp, so that the CancleRoutinue doesnt try to get the 

		KeAcquireSpinLock(&pServer->pSession->sendIrpListLock , &oldIrql);
		while(!IsListEmpty(&pServer->pSession->sendIrpList))
		{
			pTempEntry = RemoveHeadList(&pServer->pSession->sendIrpList);
			pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,SessionListElemet);

			if(pSendBuffer->pSendIrp != NULL)
			{
				// Set the Cancel Falg so that we dont acquire the spinlock in IoCancelIrp
				pSendBuffer->nCancelFlag = 1;
				IoCancelIrp(pSendBuffer->pSendIrp);
			}
		}
		KeReleaseSpinLock(&pServer->pSession->sendIrpListLock , oldIrql);

		if(pServer->pSession->pReceiveHeader != NULL && pServer->pSession->pReceiveHeader->pReceiveIrp != NULL)
		{
			IoCancelIrp(pServer->pSession->pReceiveHeader->pReceiveIrp);
		}

		if(pServer->pSession->bSessionEstablished == SFTK_CONNECTED)
		{
			DebugPrint((DBG_COM, "COM_ResetServer(): Before calling the TDI_Disconnect for session %lx\n",pServer->pSession));
			status = TDI_Disconnect(
				&pServer->pSession->KSEndpoint,
				NULL,    // UserCompletionEvent
				NULL,    // UserCompletionRoutine
				NULL,    // UserCompletionContext
				NULL,    // pIoStatusBlock
				0        // Disconnect Flags
				);

			nLiveSessions = InterlockedExchange(	&pServer->pSessionManager->nLiveSessions, 
													pServer->pSessionManager->nLiveSessions);
			if (nLiveSessions > 0)
			{
				nLiveSessions = InterlockedDecrement(&pServer->pSessionManager->nLiveSessions);
				pServer->pSession->bSessionEstablished = SFTK_DISCONNECTED;

				if(nLiveSessions ==0)
				{
					DebugPrint((DBG_CONNECT, "COM_ResetServer(): All the Connections for this SESSION_MANAGER are Down\n"));
					KeSetEvent(&pServer->pSessionManager->GroupDisconnectedEvent,0,FALSE);
					KeClearEvent(&pServer->pSessionManager->GroupConnectedEvent);
				}
			}
		}

		DebugPrint((DBG_COM, "COM_ResetServer(): Disconnect status: 0x%8.8x for Session %d\n", status, pServer->pSession->sessionID ));
		DebugPrint((DBG_COM, "COM_ResetServer(): Waiting After Disconnect...\n" ));
		DelayTime.QuadPart = 10*1000*50;   // 50 MilliSeconds
		KeDelayExecutionThread( KernelMode, FALSE, &DelayTime );

		TDI_CloseConnectionEndpoint( &pServer->pSession->KSEndpoint );
		TDI_CloseTransportAddress( &pServer->KSAddress);

		//
		// Open Transport Address
		//
		status = TDI_OpenTransportAddress(
				TCP_DEVICE_NAME_W,
				(PTRANSPORT_ADDRESS )&pServer->LocalAddress,
				&pServer->KSAddress
				);

		if( !NT_SUCCESS( status ) )
		{
			DebugPrint((DBG_ERROR, "COM_ResetServer(): Transport Address Couldnt be opened  for IPAddr %d and Port %d So Quitting\n",pServer->LocalAddress.Address[0].Address[0].in_addr, pServer->LocalAddress.Address[0].Address[0].sin_port));
			//
			// Address Object Could Not Be Created
			//
			leave;
		}



		//Initialize the Connection End Point
		status = TDI_OpenConnectionEndpoint(
						TCP_DEVICE_NAME_W,
						&pServer->KSAddress,
						&pServer->pSession->KSEndpoint,
						&pServer->pSession->KSEndpoint    // Context
						);

		if( !NT_SUCCESS( status ) )
		{
			//
			// Connection Endpoint Could Not Be Created
			//
			DebugPrint((DBG_COM, "COM_ResetServer(): Open Connection EndPoint Failed for session %lx\n",pServer->pSession));
			leave;
		}

		DebugPrint((DBG_COM, "COM_ResetServer(): Created Local TDI Connection Endpoint\n") );
		DebugPrint((DBG_COM, "COM_ResetServer(): pSession: 0x%8.8X; pAddress: 0x%8.8X; pConnection: 0x%8.8X\n",
			(ULONG )pServer->pSession,
			(ULONG )&pServer->KSAddress,
			(ULONG )&pServer->pSession->KSEndpoint
			));

#define _USE_RECEIVE_EVENT_
#ifndef _USE_RECEIVE_EVENT_
		//
		// Setup Event Handlers On The Address Object
		//
		status = TDI_SetEventHandlers(
						&pServer->KSAddress,
						pServer->pSession,      // Event Context
						NULL,          // ConnectEventHandler
						TDI_DisconnectEventHandler,
						TDI_ErrorEventHandler,
						//TCPC_ReceiveEventHandler,
						NULL,
						NULL,          // ReceiveDatagramEventHandler
						TDI_ReceiveExpeditedEventHandler
						);

		if( !NT_SUCCESS( status ) )
		{
			DebugPrint((DBG_ERROR, "COM_ResetServer(): Failed To Set the Event Handlers\n"));
			leave;
			//
			// Event Handlers Could Not Be Set
			//
		}
#else
		//
		// Setup Event Handlers On The Address Object
		//
		status = TDI_SetEventHandlers(
						&pServer->KSAddress,
						pServer->pSession,      // Event Context
						NULL,          // ConnectEventHandler
						TDI_DisconnectEventHandler,
						TDI_ErrorEventHandler,	
						TDI_ReceiveEventHandler,	//Receive Handler is enabled
						NULL,          // ReceiveDatagramEventHandler
						TDI_ReceiveExpeditedEventHandler
						);

		if( !NT_SUCCESS( status ) )
		{
			DebugPrint((DBG_ERROR, "COM_ResetServer(): Failed To Set the Event Handlers\n"));
			leave;
			//
			// Event Handlers Could Not Be Set
			//
		}

#endif //_USE_RECEIVE_EVENT_

		DebugPrint((DBG_COM, "COM_ResetServer(): Set Event Handlers On The Address Object\n") );
		pDeviceObject = IoGetRelatedDeviceObject(
							pServer->pSession->KSEndpoint.m_pFileObject
							);


		if(pServer->pSessionManager->ConnectionType == CONNECT)
		{
			DebugPrint((DBG_COM, "COM_ResetServer(): The CONNECT Initialzation\n"));

			pServer->pSession->pConnectIrp = IoAllocateIrp(
										pDeviceObject->StackSize,
										FALSE
										);

			if( !pServer->pSession->pConnectIrp )
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				leave;
			}
		}
		if(pServer->pSessionManager->ConnectionType == ACCEPT)
		{
			//
			// Allocate Irp For Use In Listening For A Connection
			//
			pServer->pSession->pListenIrp = IoAllocateIrp(
										pDeviceObject->StackSize,
										FALSE
										);

			if( !pServer->pSession->pListenIrp )
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				leave;
			}

		//
			// Build The Listen Request
		//
			pServer->pSession->RemoteConnectionInfo.UserDataLength = 0;
			pServer->pSession->RemoteConnectionInfo.UserData = NULL;

			pServer->pSession->RemoteConnectionInfo.OptionsLength = 0;
			pServer->pSession->RemoteConnectionInfo.Options = NULL;

			pServer->pSession->RemoteConnectionInfo.RemoteAddressLength = sizeof( TA_IP_ADDRESS );
			pServer->pSession->RemoteConnectionInfo.RemoteAddress = &pServer->pSession->RemoteAddress;

		}


		if(pServer->pSession->pReceiveHeader != NULL)
		{
			pServer->pSession->pReceiveHeader->ActualReceiveLength =0;
			pServer->pSession->pReceiveHeader->TotalReceiveLength = sizeof(ftd_header_t);
			pServer->pSession->pReceiveHeader->state = SFTK_BUFFER_FREE;
			pServer->pSession->pReceiveHeader->pSession = pServer->pSession;
		}

		if(pServer->pSession->pReceiveBuffer != NULL)
		{
			pServer->pSession->pReceiveBuffer->ActualReceiveLength = 0;
			pServer->pSession->pReceiveBuffer->TotalReceiveLength = pServer->pSession->pReceiveBuffer->ReceiveWindow;
			pServer->pSession->pReceiveBuffer->state = SFTK_BUFFER_FREE;
			pServer->pSession->pReceiveBuffer->pSession = pServer->pSession;
		}

		pServer->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
		pServer->pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
		pServer->pSession->bSessionEstablished = SFTK_DISCONNECTED;
	}
	finally
	{
		//Release the Resource 
		ExReleaseResourceLite(&pServer->pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	// Enable the APC's
	}
}// COM_ResetServer()

NTSTATUS
COM_EnableAllSessionManager(
						IN PSESSION_MANAGER pSessionManager
						)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSERVER_ELEMENT pServer = NULL;
	PLIST_ENTRY pTempEntry;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager);

	if(!pSessionManager->bInitialized)
	{
		DebugPrint((DBG_ERROR, "COM_EnableAllSessionManager(): The Session Manager is not Initialized\n"));
		return STATUS_INVALID_PARAMETER;
	}
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_ENABLE_ALL_CONNECTIONS,STATUS_SUCCESS,0,wchar1,wchar2);


	try
	{
		// Acquire the resource as shared Lock
		// This Resource will not be acquired if there is a call for Exclusive Lock

		KeEnterCriticalRegion();	//Disable the Kernel APC's
		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		// This is if there is a Send and Receive Thread per SERVER_ELEMENT
		pTempEntry = pSessionManager->ServerList.Flink;

		while(pTempEntry != &pSessionManager->ServerList)
		{
			pServer = CONTAINING_RECORD(pTempEntry,SERVER_ELEMENT,ListElement);
			pTempEntry = pTempEntry->Flink;

			DebugPrint((DBG_COM, "COM_EnableAllSessionManager(): pSession->bSessionEstablished for session %lx\n",pServer->pSession));

			if(pServer->bIsEnabled == FALSE)
			{
				pServer->bEnable = TRUE;
			}
		}// while Iterate Server List
	}
	finally
	{
		//Release the Resource 
		ExReleaseResourceLite(&pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	//Enable the Kernel APC's
	}

return status;
}// COM_EnableAllSessionManager

NTSTATUS
COM_DisableAllSessionManager(
						IN PSESSION_MANAGER pSessionManager
						)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSERVER_ELEMENT pServer = NULL;
	PLIST_ENTRY pTempEntry;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager);

	if(!pSessionManager->bInitialized)
	{
		DebugPrint((DBG_ERROR, "COM_DisableAllSessionManager(): The Session Manager is not Initialized\n"));
		return STATUS_INVALID_PARAMETER;
	}
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_DISABLE_ALL_CONNECTIONS,STATUS_SUCCESS,0,wchar1,wchar2);


	try
	{
		// Acquire the resource as shared Lock
		// This Resource will not be acquired if there is a call for Exclusive Lock

		KeEnterCriticalRegion();	//Disable the Kernel APC's
		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		// This is if there is a Send and Receive Thread per SERVER_ELEMENT
		pTempEntry = pSessionManager->ServerList.Flink;

		while(pTempEntry != &pSessionManager->ServerList)
		{
			pServer = CONTAINING_RECORD(pTempEntry,SERVER_ELEMENT,ListElement);
			pTempEntry = pTempEntry->Flink;
			DebugPrint((DBG_COM, "COM_DisableAllSessionManager(): pSession->bSessionEstablished for session %lx\n",pServer->pSession));

			if(pServer->bIsEnabled == TRUE)
			{
				pServer->bEnable = FALSE;
			}
		}// while Iterate Server List
	}
	finally
	{
		//Release the Resource 
		ExReleaseResourceLite(&pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	//Enable the Kernel APC's
	}

return status;
}// COM_DisableAllSessionManager

// Reset All Connections after first disconnecting and then reconnect
// TODO :: API need to code by veera
NTSTATUS
COM_ResetAllConnections( 
						IN PSESSION_MANAGER pSessionManager
						)
{
	NTSTATUS status = STATUS_SUCCESS;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager);

	if(!pSessionManager->bInitialized)
	{
		DebugPrint((DBG_ERROR, "COM_ResetAllConnections(): The Session Manager is not Initialized\n"));
		return STATUS_INVALID_PARAMETER;
	}
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	try
	{
		swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
		swprintf(wchar2,L"0X%08X",pSessionManager);
		sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_RESET_CONNECTIONS,STATUS_SUCCESS,0,wchar1,wchar2);
		pSessionManager->Reset = TRUE;
	}
	finally
	{
	}
	return status;
} // COM_ResetAllConnections()


// Reset All Connections after first disconnecting and then reconnect
// TODO :: API need to code by veera
NTSTATUS
COM_ResetAllSessionManager( 
						IN PSESSION_MANAGER pSessionManager
						)
{
	NTSTATUS status					= STATUS_SUCCESS;
	PSERVER_ELEMENT pServer			= NULL;
	PDEVICE_OBJECT pDeviceObject	= NULL;
	LARGE_INTEGER iWait;
	PLIST_ENTRY pTempEntry, pSendIrpEntry;
	PSEND_BUFFER pSendBuffer		= NULL;
	KIRQL oldIrql;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];
	LONG nLiveSessions = 0;

	OS_ASSERT(pSessionManager != NULL);

	if(!pSessionManager->bInitialized)
	{
		DebugPrint((DBG_ERROR, "COM_ResetAllSessionManager(): The Session Manager is not Initialized\n"));
		return STATUS_INVALID_PARAMETER;
	}

	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	try
	{
		swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
		swprintf(wchar2,L"0X%08X",pSessionManager);
		sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_RESET_CONNECTIONS,STATUS_SUCCESS,0,wchar1,wchar2);

		// Stop the Send and Receive Threads First
		COM_StopSendReceiveThreads(pSessionManager);

		OS_ASSERT(pSessionManager->pSendThreadObject == NULL);
		OS_ASSERT(pSessionManager->pReceiveThreadObject == NULL);

		// Acquire the resource as shared Lock
		// This Resource will not be acquired if there is a call for Exclusive Lock

//		KeEnterCriticalRegion();	//Disable the Kernel APC's
//		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		// This is if there is a Send and Receive Thread per SERVER_ELEMENT

		pTempEntry = pSessionManager->ServerList.Flink;

		while(pTempEntry != &pSessionManager->ServerList)
		{
			pServer = CONTAINING_RECORD(pTempEntry,SERVER_ELEMENT,ListElement);
			pTempEntry = pTempEntry->Flink;

			DebugPrint((DBG_COM, "COM_ResetAllSessionManager(): pSession->bSessionEstablished for session %lx\n",pServer->pSession));

			// Stop the Send and Receive Threads incase we are using them
			COM_StopSendReceiveThreadForServer(pServer);

			OS_ASSERT(pServer->pSessionSendThreadObject == NULL);
			OS_ASSERT(pServer->pSessionReceiveThreadObject == NULL);


			if(pServer->pSession->pListenIrp != NULL)
			{
				IoCancelIrp(pServer->pSession->pListenIrp);
			}

			if(pServer->pSession->pConnectIrp != NULL)
			{
				IoCancelIrp(pServer->pSession->pConnectIrp);
			}

			// Cancel All the Send Buffer IRP's and Free up the IRP's
			// To avoid a deadlock just set the CancelFalg in the Buffer to 1 
			// and then Cancel that irp, so that the CancleRoutinue doesnt try to get the 

			KeAcquireSpinLock(&pServer->pSession->sendIrpListLock , &oldIrql);
			while(!IsListEmpty(&pServer->pSession->sendIrpList))
			{
				pSendIrpEntry = RemoveHeadList(&pServer->pSession->sendIrpList);
				pSendBuffer = CONTAINING_RECORD(pSendIrpEntry,SEND_BUFFER,SessionListElemet);

				// Set the Cancel Falg so that we dont acquire the spinlock in IoCancelIrp
				if(pSendBuffer->pSendIrp != NULL)
				{
					pSendBuffer->nCancelFlag = 1;
					IoCancelIrp(pSendBuffer->pSendIrp);
				}
			}
			KeReleaseSpinLock(&pServer->pSession->sendIrpListLock , oldIrql);


			if(pServer->pSession->pReceiveHeader != NULL && pServer->pSession->pReceiveHeader->pReceiveIrp != NULL)
			{
				IoCancelIrp(pServer->pSession->pReceiveHeader->pReceiveIrp);
			}

			if(pServer->pSession->bSessionEstablished == SFTK_CONNECTED)
			{
				DebugPrint((DBG_COM, "COM_ResetAllSessionManager(): Before calling the TDI_Disconnect for session %lx\n",pServer->pSession));
				status = TDI_Disconnect(
					&pServer->pSession->KSEndpoint,
					NULL,    // UserCompletionEvent
					NULL,    // UserCompletionRoutine
					NULL,    // UserCompletionContext
					NULL,    // pIoStatusBlock
					0        // Disconnect Flags
					);

				nLiveSessions = InterlockedExchange(	&pServer->pSessionManager->nLiveSessions, 
														pServer->pSessionManager->nLiveSessions);
				if (nLiveSessions > 0)
				{
					nLiveSessions = InterlockedDecrement(&pServer->pSessionManager->nLiveSessions);
					pServer->pSession->bSessionEstablished = SFTK_DISCONNECTED;

					if(nLiveSessions ==0)
					{
						DebugPrint((DBG_CONNECT, "COM_ResetAllSessionManager(): All the Connections for this SESSION_MANAGER are Down\n"));
						KeSetEvent(&pServer->pSessionManager->GroupDisconnectedEvent,0,FALSE);
						KeClearEvent(&pServer->pSessionManager->GroupConnectedEvent);
					}
				}
			}

			DebugPrint((DBG_COM, "COM_ResetAllSessionManager(): Disconnect status: 0x%8.8x for Session %d\n", status, pServer->pSession->sessionID ));
			DebugPrint((DBG_COM, "COM_ResetAllSessionManager(): Waiting After Disconnect...\n" ));

			iWait.QuadPart = 10*1000*50;   // 50 MilliSeconds
			KeDelayExecutionThread( KernelMode, FALSE, &iWait );

			// TODO:: Research Required to figure out how to reuse the Endpoint
			// But for the timebeing we will Close and Reopen the Handles

			TDI_CloseConnectionEndpoint( &pServer->pSession->KSEndpoint );
			TDI_CloseTransportAddress( &pServer->KSAddress);

			//
			// Open Transport Address
			//
			status = TDI_OpenTransportAddress(
					TCP_DEVICE_NAME_W,
					(PTRANSPORT_ADDRESS )&pServer->LocalAddress,
					&pServer->KSAddress
					);

			if( !NT_SUCCESS( status ) )
			{
				DebugPrint((DBG_ERROR, "COM_ResetAllSessionManager(): Transport Address Couldnt be opened  for IPAddr %d and Port %d So Quitting\n",pServer->LocalAddress.Address[0].Address[0].in_addr, pServer->LocalAddress.Address[0].Address[0].sin_port));
				//
				// Address Object Could Not Be Created
				//
				leave;
			}



			//Initialize the Connection End Point
			status = TDI_OpenConnectionEndpoint(
							TCP_DEVICE_NAME_W,
							&pServer->KSAddress,
							&pServer->pSession->KSEndpoint,
							&pServer->pSession->KSEndpoint    // Context
							);

			if( !NT_SUCCESS( status ) )
			{
				//
				// Connection Endpoint Could Not Be Created
				//
				DebugPrint((DBG_COM, "COM_ResetAllSessionManager(): Open Connection EndPoint Failed for session %lx\n",pServer->pSession));
				leave;
			}

			DebugPrint((DBG_COM, "COM_ResetAllSessionManager(): Created Local TDI Connection Endpoint\n") );
			DebugPrint((DBG_COM, "COM_ResetAllSessionManager(): pSession: 0x%8.8X; pAddress: 0x%8.8X; pConnection: 0x%8.8X\n",
				(ULONG )pServer->pSession,
				(ULONG )&pServer->KSAddress,
				(ULONG )&pServer->pSession->KSEndpoint
				));

	#define _USE_RECEIVE_EVENT_
	#ifndef _USE_RECEIVE_EVENT_
			//
			// Setup Event Handlers On The Address Object
			//
			status = TDI_SetEventHandlers(
							&pServer->KSAddress,
							pServer->pSession,      // Event Context
							NULL,          // ConnectEventHandler
							TDI_DisconnectEventHandler,
							TDI_ErrorEventHandler,
							//TCPC_ReceiveEventHandler,
							NULL,
							NULL,          // ReceiveDatagramEventHandler
							TDI_ReceiveExpeditedEventHandler
							);

			if( !NT_SUCCESS( status ) )
			{
				DebugPrint((DBG_ERROR, "COM_ResetAllSessionManager(): Failed To Set the Event Handlers\n"));
				leave;
				//
				// Event Handlers Could Not Be Set
				//
			}
	#else
			//
			// Setup Event Handlers On The Address Object
			//
			status = TDI_SetEventHandlers(
							&pServer->KSAddress,
							pServer->pSession,      // Event Context
							NULL,          // ConnectEventHandler
							TDI_DisconnectEventHandler,
							TDI_ErrorEventHandler,	
							TDI_ReceiveEventHandler,	//Receive Handler is enabled
							NULL,          // ReceiveDatagramEventHandler
							TDI_ReceiveExpeditedEventHandler
							);

			if( !NT_SUCCESS( status ) )
			{
				DebugPrint((DBG_ERROR, "COM_ResetAllSessionManager(): Failed To Set the Event Handlers\n"));
				leave;
				//
				// Event Handlers Could Not Be Set
				//
			}

	#endif //_USE_RECEIVE_EVENT_

			DebugPrint((DBG_COM, "COM_ResetAllSessionManager(): Set Event Handlers On The Address Object\n") );
			pDeviceObject = IoGetRelatedDeviceObject(
								pServer->pSession->KSEndpoint.m_pFileObject
								);


			if(pSessionManager->ConnectionType == CONNECT)
			{
				DebugPrint((DBG_COM, "COM_ResetAllSessionManager(): The CONNECT Initialzation\n"));

				pServer->pSession->pConnectIrp = IoAllocateIrp(
											pDeviceObject->StackSize,
											FALSE
											);

				if( !pServer->pSession->pConnectIrp )
				{
					status = STATUS_INSUFFICIENT_RESOURCES;
					leave;
				}
			}
			if(pSessionManager->ConnectionType == ACCEPT)
			{
				//
				// Allocate Irp For Use In Listening For A Connection
				//
				pServer->pSession->pListenIrp = IoAllocateIrp(
											pDeviceObject->StackSize,
											FALSE
											);

				if( !pServer->pSession->pListenIrp )
				{
					status = STATUS_INSUFFICIENT_RESOURCES;
					leave;
				}

			//
				// Build The Listen Request
			//
				pServer->pSession->RemoteConnectionInfo.UserDataLength = 0;
				pServer->pSession->RemoteConnectionInfo.UserData = NULL;

				pServer->pSession->RemoteConnectionInfo.OptionsLength = 0;
				pServer->pSession->RemoteConnectionInfo.Options = NULL;

				pServer->pSession->RemoteConnectionInfo.RemoteAddressLength = sizeof( TA_IP_ADDRESS );
				pServer->pSession->RemoteConnectionInfo.RemoteAddress = &pServer->pSession->RemoteAddress;

			}

			if(pServer->pSession->pReceiveHeader != NULL)
			{
				pServer->pSession->pReceiveHeader->ActualReceiveLength =0;
				pServer->pSession->pReceiveHeader->TotalReceiveLength = sizeof(ftd_header_t);
				pServer->pSession->pReceiveHeader->state = SFTK_BUFFER_FREE;
				pServer->pSession->pReceiveHeader->pSession = pServer->pSession;
			}

			if(pServer->pSession->pReceiveBuffer != NULL)
			{
				pServer->pSession->pReceiveBuffer->ActualReceiveLength = 0;
				pServer->pSession->pReceiveBuffer->TotalReceiveLength = pServer->pSession->pReceiveBuffer->ReceiveWindow;
				pServer->pSession->pReceiveBuffer->state = SFTK_BUFFER_FREE;
				pServer->pSession->pReceiveBuffer->pSession = pServer->pSession;
			}

			pServer->pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
			pServer->pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
			pServer->pSession->bSessionEstablished = SFTK_DISCONNECTED;
		}// while(pTempEntry != &pSessionManager->ServerList)

		// Make the Send Buffer List All Buffers Free
		KeAcquireSpinLock(&pSessionManager->sendBufferList.SendBufferListLock , &oldIrql);
		pTempEntry = pSessionManager->sendBufferList.SendBufferList.Flink;
		while(pTempEntry != &pSessionManager->sendBufferList.SendBufferList)
		{
			pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,ListElement);
			pTempEntry = pTempEntry->Flink;
			if(pSendBuffer->bPhysicalMemory == TRUE)
			{
				// Clean up the Physical Memory Allocated for Out Of Band Packets
				RemoveEntryList(&pSendBuffer->ListElement);

				if(pSendBuffer->pSendIrp != NULL)
				{
					IoFreeIrp(pSendBuffer->pSendIrp);
					pSendBuffer->pSendIrp = NULL;
				}

				if(pSendBuffer->pSendBuffer != NULL)
				{
					NdisFreeMemory(pSendBuffer->pSendBuffer,pSendBuffer->TotalSendLength,0);
					pSendBuffer->pSendBuffer = NULL;
				}

				NdisFreeMemory(pSendBuffer,sizeof(SEND_BUFFER),0);
				pSendBuffer = NULL;
			}// Freeing up Physical Memory
			else
			{
				// Set the Send Buffer as Free
				//Initializing the Memory
				if (pSendBuffer->pSendBuffer != NULL)
				{
					NdisZeroMemory(pSendBuffer->pSendBuffer,pSendBuffer->SendWindow);
				}
				pSendBuffer->state = SFTK_BUFFER_FREE;
			}// Reinitializing the Sned Buffers
		} // while(pTempEntry != &pSessionManager->sendBufferList.SendBufferList)
		KeReleaseSpinLock(&pSessionManager->sendBufferList.SendBufferListLock , oldIrql);

		pSessionManager->sendBufferList.LastBufferIndex = 0;
		pSessionManager->pSyncReceivedEvent = NULL;
		pSessionManager->LastPacketSentTime = 0;
		pSessionManager->LastPacketReceiveTime = 0;
		pSessionManager->bSendHandshakeInformation = TRUE;
		KeResetEvent(&pSessionManager->HandshakeEvent);
		pSessionManager->Reset = FALSE;
	}
	finally
	{
		//Release the Resource 
//		ExReleaseResourceLite(&pSessionManager->ServerListLock);
//		KeLeaveCriticalRegion();	//Enable the Kernel APC's
	}
	return status;
} // COM_ResetAllSessionManager()

// Stops the Connect/Listen Send Receive Threads, Uninitializes the Server List and 
// Resets the Events
VOID 
COM_StopSessionManager(
					IN PSESSION_MANAGER pSessionManager
					)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSERVER_ELEMENT pServer = NULL;
	// LARGE_INTEGER iWait;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager != NULL);

	if(!pSessionManager->bInitialized)
	{
		DebugPrint((DBG_ERROR, "COM_StopSessionManager(): The Session Manager is not Initialized\n"));
		return;
	}
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	DebugPrint((DBG_COM, "COM_StopSessionManager(): Entering COM_StopSessionManager()\n"));

	try
	{
		swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
		swprintf(wchar2,L"0X%08X",pSessionManager);
		sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_STOP_CONNECTIONS,STATUS_SUCCESS,0,wchar1,wchar2);

		try
		{
			pSessionManager->Stop = TRUE; 
			// iWait.QuadPart = -5*1000*10*1000; //100 Milli Seconds.

			// This is if there are only one send and one receive threads per SESSION_MANAGER
			COM_StopSendReceiveThreads(pSessionManager);
			DebugPrint((DBG_COM, "COM_StopSessionManager(): Successfully Stopped the Send and Receive Threads\n"));

			OS_ASSERT(pSessionManager->pSendThreadObject == NULL);
			OS_ASSERT(pSessionManager->pReceiveThreadObject == NULL);


			try
			{
				// Acquire the resource as shared Lock
				// This Resource will not be acquired if there is a call for Exclusive Lock

				KeEnterCriticalRegion();	//Disable the Kernel APC's
				ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

				// This is if there is a Send and Receive Thread per SERVER_ELEMENT
				pTempEntry = pSessionManager->ServerList.Flink;

				while(pTempEntry != &pSessionManager->ServerList)
				{
					pServer = CONTAINING_RECORD(pTempEntry,SERVER_ELEMENT,ListElement);
					pTempEntry = pTempEntry->Flink;

					DebugPrint((DBG_COM, "COM_StopSessionManager(): Disabling the Server %lx and Session %lx\n",pServer, pServer->pSession));
					
					// Disable all the Sessions
					if(pServer->bIsEnabled == TRUE)
					{
						pServer->bEnable = FALSE;
					}
				} // while(pTempEntry != &pSessionManager->ServerList)
			}
			finally
			{
				//Release the Resource 
				ExReleaseResourceLite(&pServer->pSessionManager->ServerListLock);
				KeLeaveCriticalRegion();	// Enable the APC's
			}

			status = KeWaitForSingleObject(&pSessionManager->GroupDisconnectedEvent,Executive,KernelMode,FALSE,NULL);
			if(status == STATUS_SUCCESS)
			{
				DebugPrint((DBG_COM, "COM_StopSessionManager(): Successfully Disabled all the Connections\n"));
			}

			KeSetEvent(&pSessionManager->GroupUninitializedEvent,0,FALSE);
			//Free up the Connect Listen Thread
			if(pSessionManager->pConnectListenThread != NULL)
			{
				status = KeWaitForSingleObject(pSessionManager->pConnectListenThread,Executive,KernelMode,FALSE,NULL);
				if(status == STATUS_SUCCESS)
				{
					DebugPrint((DBG_COM, "COM_StopSessionManager(): The COM_ConnectThread2() Successfully Exited\n"));
					ObDereferenceObject(pSessionManager->pConnectListenThread);
					pSessionManager->pConnectListenThread = NULL;
				}
			}
			// Reset the Uninitialization Event
			KeResetEvent(&pSessionManager->GroupUninitializedEvent);

			OS_ASSERT(pSessionManager->pConnectListenThread == NULL);

				pTempEntry = pSessionManager->ServerList.Flink;

			while(pTempEntry != &pSessionManager->ServerList)
			{
				pServer = CONTAINING_RECORD(pTempEntry,SERVER_ELEMENT,ListElement);
				pTempEntry = pTempEntry->Flink;

				DebugPrint((DBG_COM, "COM_StopSessionManager(): Disabling the Server %lx and Session %lx\n",pServer, pServer->pSession));
				
				// Disable all the Sessions
				if(pServer->bReset == TRUE)
				{
					pServer->bReset = FALSE;
				}
			} // while(pTempEntry != &pSessionManager->ServerList)


			DebugPrint((DBG_COM, "COM_StopSessionManager(): Successfully Stopped all the Sessions\n"));
			// KeDelayExecutionThread( KernelMode, FALSE, &iWait );
			
			COM_ClearSendBufferList1(&pSessionManager->sendBufferList);
			DebugPrint((DBG_COM, "COM_StopSessionManager(): Successfully Cleared the Send BufferList\n"));

			COM_ClearReceiveBufferList1(&pSessionManager->receiveBufferList);
			DebugPrint((DBG_COM, "COM_StopSessionManager(): Successfully Cleared the Receive BufferList\n"));
			
			// Ressign the Number of Send Buffers incase it is changed
			// This will be a great tool for testing the behaviour of the TDI
			if(pSessionManager->pLogicalGroupPtr->NumOfSendBuffers > 0)
			{
				pSessionManager->sendBufferList.MaxNumberOfSendBuffers = (USHORT)pSessionManager->pLogicalGroupPtr->NumOfSendBuffers;
			}
			pSessionManager->sendBufferList.LastBufferIndex = 0;
			pSessionManager->pSyncReceivedEvent = NULL;
			pSessionManager->LastPacketSentTime = 0;
			pSessionManager->LastPacketReceiveTime = 0;
			pSessionManager->bSendHandshakeInformation = TRUE;
			KeResetEvent(&pSessionManager->HandshakeEvent);
			pSessionManager->Reset = FALSE;
			pSessionManager->Stop = FALSE;

		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			DebugPrint((DBG_COM, "COM_StopSessionManager(): An Exception Occured Error Code is %lx\n",GetExceptionCode()));
			status = GetExceptionCode();
		}
	}
	finally
	{
	}
	DebugPrint((DBG_COM, "COM_StopSessionManager(): Leaving COM_StopSessionManager()\n"));
}// COM_StopSessionManager


VOID 
COM_UninitializeSessionManager(
							IN PSESSION_MANAGER pSessionManager
							)
{
	LONG lErrorCode =0;
	NTSTATUS status = STATUS_SUCCESS;
	PSERVER_ELEMENT pServer = NULL;
	LARGE_INTEGER iWait;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager != NULL);

	DebugPrint((DBG_COM, "COM_UninitializeSessionManager(): Entering COM_UninitializeSessionManager()\n"));
	if(!pSessionManager->bInitialized)
	{
		DebugPrint((DBG_ERROR, "COM_UninitializeSessionManager(): The Session Manager is not Initialized\n"));
		return;
	}
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	try
	{
		swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
		swprintf(wchar2,L"0X%08X",pSessionManager);
		sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_UNINITIALIZE,STATUS_SUCCESS,0,wchar1,wchar2);

		COM_StopSessionManager(pSessionManager);
		ExDeleteResourceLite(&pSessionManager->ServerListLock);
		COM_FreeConnections(&pSessionManager->ServerList);
		pSessionManager->bInitialized = FALSE;

		OS_ZeroMemory(pSessionManager, sizeof(SESSION_MANAGER));
	}
	finally
	{
		
	}
	DebugPrint((DBG_COM, "COM_UninitializeSessionManager(): Leaving COM_UninitializeSessionManager()\n"));
}// COM_UninitializeSessionManager


NTSTATUS 
COM_ConnectThread2(
			IN PSESSION_MANAGER pSessionManager
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PDEVICE_OBJECT pDeviceObject = NULL;
	BOOLEAN bSendReceiveInitialized = FALSE;
	TDI_CONNECTION_INFORMATION RequestConnectionInfo;
	LARGE_INTEGER iCurrentTime;
	ULONG uCurTime = 0;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): Enter COM_ConnectThread2()\n"));

	pSessionManager->bSendHandshakeInformation = TRUE;
	KeResetEvent(&pSessionManager->HandshakeEvent);

	try
	{
		swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
		swprintf(wchar2,L"0X%08X",pSessionManager);
		swprintf(wchar3,L"%S",L"COM_ConnectThread2");
		sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_STARTED,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);

		while(!bExit)
		{
			try
			{
				// Acquire the resource as shared Lock
				// This Resource will not be acquired if there is a call for Exclusive Lock

				KeEnterCriticalRegion();	//Disable the Kernel APC's
				ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

				// 100 Milli-Seconds in 100*Nano Seconds
				// 1 seconds
				iWait.QuadPart = -(1 * 1000*10*1000);
				iZeroWait.QuadPart = 0;

				status = KeWaitForSingleObject(&pSessionManager->GroupConnectedEvent,Executive,KernelMode,FALSE,&iZeroWait);
				if((status == STATUS_SUCCESS) && !bSendReceiveInitialized)
				{
					OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

					if(pSessionManager->bSendHandshakeInformation == TRUE)
					{
						PROTO_Send_NOOP(pSessionManager, pSessionManager->pLogicalGroupPtr->LGroupNumber , 0);
					}

					status = COM_CreateSendReceiveThreads(pSessionManager);
					if(NT_SUCCESS(status))
					{
						bSendReceiveInitialized = TRUE;

						// This is the First Time we got connected so lets send the Handshake Information
						// Get out of the Resource Lock and then Get the exclusive lock 

						try
						{
							if(pSessionManager->bSendHandshakeInformation == TRUE)
							{
								swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
								swprintf(wchar2,L"0X%08X",pSessionManager);
								sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_PERFORMING_HANDSHAKE,STATUS_SUCCESS,0,wchar1,wchar2);

								status = PROTO_Perform_Handshake(pSessionManager);

								// Just check for the STATUS_SUCCESS flag
								// Caution STATUS_TIMEOUT might also happen

								if( status != STATUS_SUCCESS )
								{
									swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
									swprintf(wchar2,L"0X%08X",pSessionManager);
									swprintf(wchar2,L"0X%08X",status);
									sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_HANDSHAKE_FAILED_ERROR,status,0,wchar1,wchar2,wchar3);

									DebugPrint((DBG_ERROR, "COM_ConnectThread2(): PROTO_Perform_Handshake for SessionManager %lx Failed\n",pSessionManager));
									leave;
								}

								pSessionManager->bSendHandshakeInformation = FALSE;
								// Handshake is done so signal the event
								KeSetEvent(&pSessionManager->HandshakeEvent,0,FALSE);
								}
						}
						finally
						{
							// Just check for the STATUS_SUCCESS flag
							// Caution STATUS_TIMEOUT might also happen
							if( status != STATUS_SUCCESS )
							{
								DebugPrint((DBG_ERROR, "COM_ConnectThread2(): COM_ResetAllConnections() Resetting Connections for SessionManager %lx\n",pSessionManager));
								// Set the Reset Falg and continue
								COM_ResetAllConnections(pSessionManager);
							}
						}
					}//if
//					DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): Enter CreateSendReceive\n"));
				}//if
				else if(bSendReceiveInitialized && (status != STATUS_SUCCESS))
				{
					COM_StopSendReceiveThreads(pSessionManager);
					bSendReceiveInitialized = FALSE;

					// Set the Mode to PassThru/Tarcking to clean up all Queue.
					if (sftk_lg_get_state(pSessionManager->pLogicalGroupPtr) == SFTK_MODE_FULL_REFRESH)
					{
						// This happened during normal operations not because of the Pause/Resume Command issued 
						// by the User.

						if(sftk_lg_get_refresh_thread_state(pSessionManager->pLogicalGroupPtr) == TRUE)
						{ // Check to see if user specified the Pause { Normal = TRUE Paused = FALSE }
							sftk_lg_change_State(	pSessionManager->pLogicalGroupPtr, pSessionManager->pLogicalGroupPtr->state, 
												SFTK_MODE_PASSTHRU, FALSE);
						}
					}
					else
					{
						if( pSessionManager->bSendHandshakeInformation == FALSE )
						{
							// Make Sure we must empty all Queue here explictly, Causes Ack thread may be waiting indifinte for 
							// FTCHUP Ack, which will never get back due to reset connections.
							// So we explicitly clear Queue here which will also satisfy FTDCHUP Outband pkts waiting ACK thread 
							// So deadlock condition will not happen.
							if ( (pSessionManager->pLogicalGroupPtr->DoNotSendOutBandCommandForTracking == FALSE) &&
								(pSessionManager->pLogicalGroupPtr->state == SFTK_MODE_TRACKING) )
							{
								QM_ScanAllQList( pSessionManager->pLogicalGroupPtr, TRUE, TRUE);
							}
							pSessionManager->pLogicalGroupPtr->DoNotSendOutBandCommandForTracking = TRUE;
							sftk_lg_change_State(	pSessionManager->pLogicalGroupPtr, pSessionManager->pLogicalGroupPtr->state, 
													SFTK_MODE_TRACKING, FALSE);
						}
					}

					pSessionManager->bSendHandshakeInformation = TRUE;

					DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): Enter LeaveSendReceive\n"));
				}//else if

				if(pSessionManager->Reset == TRUE)
				{
					DebugPrint((DBG_ERROR, "COM_ConnectThread2(): COM_ResetAllSessionManager() Resetting Connections for SessionManager %lx\n",pSessionManager));

					// Kill the Send and Receive Threads and then Reset the Connections
					COM_ResetAllSessionManager(pSessionManager);
					pSessionManager->Reset = FALSE;
					bSendReceiveInitialized = FALSE;
				}
				if(pSessionManager->nLiveSessions > 0)
				{
					KeQuerySystemTime(&iCurrentTime);
					uCurTime = (ULONG)(iCurrentTime.QuadPart/(10*1000*1000));

					// Send a NOOP Packet if the time since the last send was sent is more than FTD_NET_SEND_NOOP_TIME (20 Sec).
					if((uCurTime - pSessionManager->LastPacketSentTime) >= FTD_NET_SEND_NOOP_TIME)
					{
						// If No Data is available to sent for a longtime we should send a NOOP Packet.
						// To Keep the Connection Alive

						PROTO_Send_NOOP(pSessionManager, pSessionManager->pLogicalGroupPtr->LGroupNumber,1);
						pSessionManager->LastPacketSentTime = uCurTime;
					}
				}
				pTemp = pSessionManager->ServerList.Flink;

				while(pTemp != &pSessionManager->ServerList && !bExit)
				{
					pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);
					pTemp = pTemp->Flink;
					//Check if the Server Element and the Session Element are not NULL
					OS_ASSERT(pServerElement);
					OS_ASSERT(pServerElement->pSession);

					//ENABLE THE SESSION
					//Check if the Server Element is Initialized or not
					if( ( pServerElement->pSession->bSessionEstablished == SFTK_UNINITIALIZED ) &&
						( pServerElement->bEnable == TRUE ) &&
						( pServerElement->bIsEnabled == FALSE ) )
					{
						DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): The Session is not Enabled and not Initialized\n"));
						DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): This is the First Time it is called\n"));

						// Release the Resource 
						ExReleaseResourceLite(&pSessionManager->ServerListLock);
						KeLeaveCriticalRegion();	//Enable the Kernel APC's

						status = COM_StartServer(pServerElement,CONNECT);

						if(!NT_SUCCESS(status))
						{
							DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): COM_StartServer() Failed So Just setting to UNINITIALIZED\n"));
							//Initialization of Server Element Failed and hence just set the
							//Session as Disconnected and then set the Server as disabled
							pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
							pServerElement->bEnable = FALSE;
							pServerElement->bIsEnabled = FALSE;
						}//if
						else
						{
							//Set the Session status to DISCONNECTED and the Enabled to TRUE
							//So that we can go ahead and establish connection.
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
							pServerElement->bIsEnabled = TRUE;
						}//else

						// Reaquire the resource as shared
						KeEnterCriticalRegion();	//Disable the Kernel APC's
						ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

					}//if ENABLE

					// DISABLE THE SERVER_ELEMENT
					// Check to see if they want the Sessions to be Disabled
					if( ( pServerElement->pSession->bSessionEstablished != SFTK_UNINITIALIZED ) &&
						( pServerElement->bEnable == FALSE )&&
						( pServerElement->bIsEnabled == TRUE ) )
					{
						DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): We got the Command to Disable the Server %lx\n",pServerElement));

						// Release the Resource 
						ExReleaseResourceLite(&pSessionManager->ServerListLock);
						KeLeaveCriticalRegion();	//Enable the Kernel APC's

						// We will get an exclusive lock here as we dont want Send and Receive Threads
						// to work on the same ServerElement while we are changing it.

						COM_StopServer(pServerElement);

						pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
						pServerElement->bEnable = FALSE;
						pServerElement->bIsEnabled = FALSE;

						// Reaquire the resource as shared
						KeEnterCriticalRegion();	//Disable the Kernel APC's
						ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

					}//if DISABLE

					// RESET the Server Element, Stop the Server and Restart it.
					if( ( pServerElement->pSession->bSessionEstablished != SFTK_UNINITIALIZED ) &&
						( pServerElement->bReset == TRUE ) &&
						( pServerElement->bIsEnabled == TRUE ) )
					{
						pServerElement->bReset = FALSE;
						DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): We got the Command to Reset the Server %lx\n",pServerElement));

						// Release the Resource 
						ExReleaseResourceLite(&pSessionManager->ServerListLock);
						KeLeaveCriticalRegion();	//Enable the Kernel APC's

						// Reset the Server, so that the Connection gets reestablished and we can connect
						// again
						COM_ResetServer(pServerElement);

						// Reaquire the resource as shared
						KeEnterCriticalRegion();	//Disable the Kernel APC's
						ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

					}//if RESET


					// PROCESS the NORMAL CONNECTIONS
					// Check if the Session Needs to be established
					if((pServerElement->pSession != NULL) && 
						(pServerElement->pSession->bSessionEstablished == SFTK_DISCONNECTED) &&
						pServerElement->bIsEnabled)
					{

						OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
						status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
						if(status == STATUS_SUCCESS)
						{
							swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
							swprintf(wchar2,L"0X%08X",pSessionManager);
							sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_CONNECT_THREAD_EXITING,status,0,wchar1,wchar2);

							DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}

//						pSession = pServerElement->pSession;
						pDeviceObject = IoGetRelatedDeviceObject( pServerElement->pSession->KSEndpoint.m_pFileObject);

						try
						{
							pServerElement->pSession->bSessionEstablished = SFTK_INPROGRESS;

							//
							// Setup Request Connection Info
							//

							pServerElement->pSession->RemoteConnectionInfo.UserDataLength = 0;
							pServerElement->pSession->RemoteConnectionInfo.UserData = NULL;
							pServerElement->pSession->RemoteConnectionInfo.OptionsLength = 0;
							pServerElement->pSession->RemoteConnectionInfo.Options = NULL;
							pServerElement->pSession->RemoteConnectionInfo.RemoteAddressLength = sizeof(TA_IP_ADDRESS);
							pServerElement->pSession->RemoteConnectionInfo.RemoteAddress = &pServerElement->pSession->RemoteAddress;

							if(pServerElement->pSession->pConnectIrp == NULL)
							{
								pServerElement->pSession->pConnectIrp = IoAllocateIrp(
															pDeviceObject->StackSize,
															FALSE
															);

								if( !pServerElement->pSession->pConnectIrp )
								{
									status = STATUS_INSUFFICIENT_RESOURCES;
									leave;
								}
							}

							TdiBuildConnect(
								pServerElement->pSession->pConnectIrp,
								pDeviceObject,
								pServerElement->pSession->KSEndpoint.m_pFileObject,
								TDI_ConnectedCallback, // Completion Routine
								pServerElement->pSession,                         // Completion Context
								NULL,                         // Timeout Information
								&pServerElement->pSession->RemoteConnectionInfo,
								NULL
								);
							status = IoCallDriver( pDeviceObject, pServerElement->pSession->pConnectIrp );

							if( !NT_SUCCESS(status) )
							{
								DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): IoCallDriver(pDeviceObject = %lx) returned %lx\n",pDeviceObject,status));
							}

						}//try
						except(EXCEPTION_EXECUTE_HANDLER)
						{
							DebugPrint((DBG_ERROR, "COM_ConnectThread2(): An Exception Occured in COM_ConnectThread2() Error Code is %lx\n",GetExceptionCode()));
							status = GetExceptionCode();
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
						}//except

					}// if((pServerElement->pSession != NULL) 
				}//while(!IsListEmpty(lpListIndex))

				if(!bExit)
				{
					OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
					if(status == STATUS_SUCCESS)
					{
						DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
						bExit = TRUE;
						break;
					}//if
				}//if
			}//try
			finally
			{
				//Release the Resource 
				ExReleaseResourceLite(&pSessionManager->ServerListLock);
				KeLeaveCriticalRegion();	//Enable the Kernel APC's
			}//finally
		}	//while(!bExit)
	}//try
	finally
	{
		DebugPrint((DBG_CONNECT, "COM_ConnectThread2(): Exiting the COM_ConnectThread2() status = %lx\n",status));
	}//finally

	swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
	swprintf(wchar2,L"0X%08X",pSessionManager);
	swprintf(wchar3,L"%S",L"COM_ConnectThread2");
	sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_THREAD_STOPPED,STATUS_SUCCESS,0,wchar1,wchar2,wchar3);

	return PsTerminateSystemThread(status);
}// COM_ConnectThread2

NTSTATUS
COM_StartSendThreadForServer(
						  IN PSERVER_ELEMENT pServerElement
						  )
{
	HANDLE threadHandle = NULL;
	NTSTATUS status = STATUS_SUCCESS;

	OS_ASSERT(pServerElement);

	DebugPrint((DBG_COM, "COM_StartSendThreadForServer(): Entering COM_StartSendThreadforServer() for Server %lx\n",pServerElement));
	try
	{
		// See if the Send Therad is present or not
		if(pServerElement->pSessionSendThreadObject == NULL)
		{
			OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);


			//Creating the Send Thread
			status = PsCreateSystemThread(
			&threadHandle,				// thread handle
			0L,							// desired access
			NULL,						// object attributes
			NULL,						// process handle
			NULL,						// client id
			COM_SendThreadForServer,	//This thread has all the enhancements that are suggested
										//for the Send Thread
			(PVOID )pServerElement      // start context
			);

			if(!NT_SUCCESS(status))
			{
				DebugPrint((DBG_ERROR, "COM_StartSendThreadForServer(): Unable to Create Send Thread for Server %lx\n",pServerElement));
				threadHandle = NULL;
				leave;
			}

			status = ObReferenceObjectByHandle (
							threadHandle,										// Object Handle
							THREAD_ALL_ACCESS,									// Desired Access
							NULL,												// Object Type
							KernelMode,											// Processor mode
							(PVOID *)&pServerElement->pSessionSendThreadObject,	// Object pointer
							NULL);												// Object Handle information

			if( !NT_SUCCESS(status) )
			{
				DebugPrint((DBG_ERROR, "COM_StartSendThreadForServer(): Unable to get the Object reference for Send Thread for Server %lx\n",pServerElement));
				pServerElement->pSessionSendThreadObject = NULL;
				leave;
			}

			ZwClose(threadHandle );
			threadHandle = NULL;
		}
	}
	finally
	{
		if(threadHandle != NULL)
		{
			ZwClose(threadHandle );
			threadHandle = NULL;
		}
		DebugPrint((DBG_COM, "COM_StartSendThreadForServer(): Leaving COM_StartSendThreadforServer()\n"));
	}
	return status;
}//COM_StartSendThreadForServer

NTSTATUS
COM_StartReceiveThreadForServer(
						  IN PSERVER_ELEMENT pServerElement
						  )
{
	HANDLE threadHandle = NULL;
	NTSTATUS status = STATUS_SUCCESS;

	OS_ASSERT(pServerElement);

	DebugPrint((DBG_COM, "COM_StartReceiveThreadForServer(): Entering COM_StartReceiveThreadForServer() for Server %lx\n",pServerElement));
	try
	{
		// See if the Send Therad is present or not
		if(pServerElement->pSessionReceiveThreadObject == NULL)
		{
			OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);


			//Creating the Send Thread
			status = PsCreateSystemThread(
			&threadHandle,				// thread handle
			0L,							// desired access
			NULL,						// object attributes
			NULL,						// process handle
			NULL,						// client id
			COM_ReceiveThreadForServer,	//This thread has all the enhancements that are suggested
										//for the Send Thread
			(PVOID )pServerElement      // start context
			);

			if(!NT_SUCCESS(status))
			{
				DebugPrint((DBG_ERROR, "COM_StartReceiveThreadForServer(): Unable to Create Receive Thread for Server %lx\n",pServerElement));
				threadHandle = NULL;
				leave;
			}

			status = ObReferenceObjectByHandle (
							threadHandle,										// Object Handle
							THREAD_ALL_ACCESS,									// Desired Access
							NULL,												// Object Type
							KernelMode,											// Processor mode
							(PVOID *)&pServerElement->pSessionReceiveThreadObject,	// Object pointer
							NULL);												// Object Handle information

			if( !NT_SUCCESS(status) )
			{
				DebugPrint((DBG_ERROR, "COM_StartReceiveThreadForServer(): Unable to get the Object reference for Receive Thread for Server %lx\n",pServerElement));
				pServerElement->pSessionReceiveThreadObject = NULL;
				leave;
			}

			ZwClose(threadHandle );
			threadHandle = NULL;
		}
	}
	finally
	{
		if(threadHandle != NULL)
		{
			ZwClose(threadHandle );
			threadHandle = NULL;
		}
		DebugPrint((DBG_COM, "COM_StartReceiveThreadForServer(): Leaving COM_StartReceiveThreadForServer()\n"));
	}
	return status;
}//COM_StartReceiveThreadForServer()

NTSTATUS
COM_StopSendReceiveThreadForServer(
						  IN PSERVER_ELEMENT pServerElement
						  )
{
    NTSTATUS status = STATUS_SUCCESS;

	OS_ASSERT(pServerElement);

	DebugPrint((DBG_COM, "COM_StopSendReceiveThreadForServer(): Enter COM_StopSendReceiveThreadForServer() for Server %lx\n",pServerElement));

	KeSetEvent(&pServerElement->SessionIOExitEvent,0,FALSE);
	if(pServerElement->pSessionSendThreadObject != NULL)
	{
		status = KeWaitForSingleObject(pServerElement->pSessionSendThreadObject,Executive,KernelMode,FALSE,NULL);
		if(status == STATUS_SUCCESS)
		{
			DebugPrint((DBG_COM, "COM_StopSendReceiveThreadForServer(): The SendThread() Successfully Exited for Server %lx\n",pServerElement));
			ObDereferenceObject(pServerElement->pSessionSendThreadObject);
			pServerElement->pSessionSendThreadObject = NULL;
		}
	}

	if(pServerElement->pSessionReceiveThreadObject != NULL)
	{
		status = KeWaitForSingleObject(pServerElement->pSessionReceiveThreadObject,Executive,KernelMode,FALSE,NULL);
		if(status == STATUS_SUCCESS)
		{
			DebugPrint((DBG_COM, "COM_StopSendReceiveThreadForServer(): The ReceiveThread() Successfully Exited for Server %lx\n",pServerElement));
			ObDereferenceObject(pServerElement->pSessionReceiveThreadObject);
			pServerElement->pSessionReceiveThreadObject = NULL;
		}
	}
	KeResetEvent(&pServerElement->SessionIOExitEvent);
	DebugPrint((DBG_COM, "COM_StopSendReceiveThreadForServer(): Leaving COM_StopSendReceiveThreadForServer()\n"));
	return status;
}//COM_StopSendReceiveThreadForServer()


NTSTATUS 
COM_ConnectThreadForServer(
			IN PSESSION_MANAGER pSessionManager
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PDEVICE_OBJECT             pDeviceObject = NULL;
	TDI_CONNECTION_INFORMATION RequestConnectionInfo;
	LARGE_INTEGER iCurrentTime;
	ULONG uCurTime = 0;
	WCHAR wchar1[64] , wchar2[64] , wchar3[64] , wchar4[128];

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	pSessionManager->bSendHandshakeInformation = TRUE;
	KeResetEvent(&pSessionManager->HandshakeEvent);

	DebugPrint((DBG_CONNECT, "COM_ConnectThreadForServer(): Enter COM_ConnectThreadForServer()\n"));
	try
	{
		while(!bExit)
		{
			try
			{
				// Acquire the resource as shared Lock
				// This Resource will not be acquired if there is a call for Exclusive Lock

				KeEnterCriticalRegion();	//Disable the Kernel APC's
				ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

				// 100 Milli-Seconds in 100*Nano Seconds
				iWait.QuadPart = -(100*10*1000);
				iZeroWait.QuadPart = 0;

				pTemp = pSessionManager->ServerList.Flink;

				while(pTemp != &pSessionManager->ServerList && !bExit)
				{
					pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);
					pTemp = pTemp->Flink;
					//Check if the Server Element and the Session Element are not NULL
					OS_ASSERT(pServerElement);
					OS_ASSERT(pServerElement->pSession);

					//ENABLE THE SESSION
					//Check if the Server Element is Initialized or not
					if((pServerElement->pSession->bSessionEstablished == SFTK_UNINITIALIZED) &&
						pServerElement->bEnable)
					{
						DebugPrint((DBG_CONNECT, "COM_ConnectThreadForServer(): The Session is not Enabled and not Initialized\n"));
						DebugPrint((DBG_CONNECT, "COM_ConnectThreadForServer(): This is the First Time it is called\n"));

						status = COM_StartServer(pServerElement,CONNECT);

						if(!NT_SUCCESS(status))
						{
							DebugPrint((DBG_CONNECT, "COM_ConnectThreadForServer(): COM_StartServer() Failed So Just setting to UNINITIALIZED\n"));
							//Initialization of Server Element Failed and hence just set the
							//Session as Disconnected and then set the Server as disabled
							pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
							pServerElement->bEnable = FALSE;
							pServerElement->bIsEnabled = FALSE;
						}//if
						else
						{
							//Set the Session status to DISCONNECTED and the Enabled to TRUE
							//So that we can go ahead and establish connection.
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
							pServerElement->bIsEnabled = TRUE;
						}//else
					}//if ENABLE

					// DISABLE THE SERVER_ELEMENT
					// Check to see if they want the Sessions to be Disabled
					if((pServerElement->pSession->bSessionEstablished != SFTK_UNINITIALIZED) &&
						!pServerElement->bEnable &&
						pServerElement->bIsEnabled == TRUE)
					{
						DebugPrint((DBG_CONNECT, "COM_ConnectThreadForServer(): We got the Command to Disable the Server %lx\n",pServerElement));

						COM_StopSendReceiveThreadForServer(pServerElement);
						COM_StopServer(pServerElement);

						pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
						pServerElement->bEnable = FALSE;
						pServerElement->bIsEnabled = FALSE;

					}//if DISABLE

					// RESET the Server Element, Stop the Server and Restart it.
					if((pServerElement->pSession->bSessionEstablished != SFTK_UNINITIALIZED) &&
						pServerElement->bReset &&
						pServerElement->bIsEnabled == TRUE)
					{
						pServerElement->bReset = FALSE;
						DebugPrint((DBG_CONNECT, "COM_ConnectThreadForServer(): We got the Command to Disable the Server %lx\n",pServerElement));

						// Reset the Server, so that the Connection gets reestablished and we can connect
						// again
						COM_StopSendReceiveThreadForServer(pServerElement);
						COM_ResetServer(pServerElement);

					}//if RESET
					
					if(pServerElement->pSession->bSessionEstablished == SFTK_CONNECTED)
					{
						// If the Session is Connected then we have to verify if the send and 
						// Receive threads for this Session are created or not, if they are not created
						// the threads will be created

						if(pSessionManager->bSendHandshakeInformation == TRUE)
						{
							PROTO_Send_NOOP(pSessionManager, pSessionManager->pLogicalGroupPtr->LGroupNumber, 0);
						}
						// Create the Send Thread
						status = COM_StartSendThreadForServer(pServerElement);
						// Create the Receive Thread
						status = COM_StartReceiveThreadForServer(pServerElement);

						try
						{
							if(pSessionManager->bSendHandshakeInformation == TRUE)
							{
								swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
								swprintf(wchar2,L"0X%08X",pSessionManager);
								sftk_LogEventString2(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_PERFORMING_HANDSHAKE,STATUS_SUCCESS,0,wchar1,wchar2);

								status = PROTO_Perform_Handshake(pSessionManager);

								if(!NT_SUCCESS(status))
								{
									swprintf(wchar1,L"%d",pSessionManager->pLogicalGroupPtr->LGroupNumber);
									swprintf(wchar2,L"0X%08X",pSessionManager);
									swprintf(wchar2,L"0X%08X",status);
									sftk_LogEventString3(GSftk_Config.DriverObject,MSG_LG_SESSIONMANAGER_HANDSHAKE_FAILED_ERROR,status,0,wchar1,wchar2,wchar3);

									DebugPrint((DBG_ERROR, "COM_ConnectThread2(): PROTO_Perform_Handshake for SessionManager %lx Failed\n",pSessionManager));
									leave;
								}

								pSessionManager->bSendHandshakeInformation = FALSE;
								KeSetEvent(&pSessionManager->HandshakeEvent,0,FALSE);
							}
						}
						finally
						{
							if(!NT_SUCCESS(status))
							{
								// Set the Reset Falg and continue
								COM_ResetAllConnections(pSessionManager);
							}
						}
					}//if SFTK_CONNECTED
					else if(pServerElement->pSession->bSessionEstablished == SFTK_DISCONNECTED)
					{
						// If the Session is Disconnected then we have to verify if the Send and Receive threads
						// are Terminited or not and wait until we get out of them.

						// Stop the Send receive Threads
						status = COM_StopSendReceiveThreadForServer(pServerElement);
					}//else if SFTK_DISCONNECTED

					if(pSessionManager->Reset == TRUE)
					{
						// Kill the Send and Receive Threads and then Reset the Connections
						COM_ResetAllSessionManager(pSessionManager);
						pSessionManager->Reset = FALSE;
					}
					if(pSessionManager->nLiveSessions > 0)
					{
						KeQuerySystemTime(&iCurrentTime);
						uCurTime = (ULONG)(iCurrentTime.QuadPart/(10*1000*1000));

						// Send a NOOP Packet if the time since the last send was sent is more than FTD_NET_SEND_NOOP_TIME (20 Sec).
						if((uCurTime - pSessionManager->LastPacketSentTime) >= FTD_NET_SEND_NOOP_TIME)
						{
							// If No Data is available to sent for a longtime we should send a NOOP Packet.
							// To Keep the Connection Alive

							PROTO_Send_NOOP(pSessionManager, pSessionManager->pLogicalGroupPtr->LGroupNumber, 1);
							pSessionManager->LastPacketSentTime = uCurTime;
						}
					}


					// PROCESS the NORMAL CONNECTIONS
					// Check if the Session Needs to be established
					if((pServerElement->pSession != NULL) && 
						(pServerElement->pSession->bSessionEstablished == SFTK_DISCONNECTED) &&
						pServerElement->bIsEnabled)
					{

						OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
						status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
						if(status == STATUS_SUCCESS)
						{
							DebugPrint((DBG_CONNECT, "COM_ConnectThreadForServer(): The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}

//						pSession = pServerElement->pSession;
						pDeviceObject = IoGetRelatedDeviceObject( pServerElement->pSession->KSEndpoint.m_pFileObject);

						try
						{
							pServerElement->pSession->bSessionEstablished = SFTK_INPROGRESS;

							//
							// Setup Request Connection Info
							//

							pServerElement->pSession->RemoteConnectionInfo.UserDataLength = 0;
							pServerElement->pSession->RemoteConnectionInfo.UserData = NULL;
							pServerElement->pSession->RemoteConnectionInfo.OptionsLength = 0;
							pServerElement->pSession->RemoteConnectionInfo.Options = NULL;
							pServerElement->pSession->RemoteConnectionInfo.RemoteAddressLength = sizeof(TA_IP_ADDRESS);
							pServerElement->pSession->RemoteConnectionInfo.RemoteAddress = &pServerElement->pSession->RemoteAddress;

							if(pServerElement->pSession->pConnectIrp == NULL)
							{
								pServerElement->pSession->pConnectIrp = IoAllocateIrp(
															pDeviceObject->StackSize,
															FALSE
															);

								if( !pServerElement->pSession->pConnectIrp )
								{
									status = STATUS_INSUFFICIENT_RESOURCES;
									leave;
								}
							}

							TdiBuildConnect(
								pServerElement->pSession->pConnectIrp,
								pDeviceObject,
								pServerElement->pSession->KSEndpoint.m_pFileObject,
								TDI_ConnectedCallback, // Completion Routine
								pServerElement->pSession,                         // Completion Context
								NULL,                         // Timeout Information
								&pServerElement->pSession->RemoteConnectionInfo,
								NULL
								);
							status = IoCallDriver( pDeviceObject, pServerElement->pSession->pConnectIrp );

							if( !NT_SUCCESS(status) )
							{
								DebugPrint((DBG_CONNECT, "COM_ConnectThreadForServer(): IoCallDriver(pDeviceObject = %lx) returned %lx\n",pDeviceObject,status));
							}

						}//try
						except(EXCEPTION_EXECUTE_HANDLER)
						{
							DebugPrint((DBG_ERROR, "COM_ConnectThreadForServer(): An Exception Occured in COM_ConnectThreadForServer() Error Code is %lx\n",GetExceptionCode()));
							status = GetExceptionCode();
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
						}//except

					} // if((pServerElement->pSession != NULL)
				}	// while(pTemp != &pSessionManager->ServerList && !bExit)

				if(!bExit)
				{
					OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
					if(status == STATUS_SUCCESS)
					{
						DebugPrint((DBG_CONNECT, "COM_ConnectThreadForServer(): The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
						bExit = TRUE;
						break;
					}//if
				}//if
			}//try
			finally
			{
				//Release the Resource 
				ExReleaseResourceLite(&pSessionManager->ServerListLock);
				KeLeaveCriticalRegion();	//Enable the Kernel APC's
			}//finally
		}	//while(!bExit)
	}//try
	finally
	{
		DebugPrint((DBG_CONNECT, "COM_ConnectThreadForServer(): Exiting the COM_ConnectThreadForServer() status = %lx\n",status));
	}//finally

	return PsTerminateSystemThread(status);
}// COM_ConnectThreadForServer


// This function returns the Performance Metrice for all the Connections in the SESSION_MANAGER
// The Statistics are returned as SM_PERFORMANCE_INFO structure with all the connections information
// This information is gathered from the underlying TDI RealTime.
// The Memory will be allocated by this function for the Performance Metrics and returned, the Memory 
// allocated will be Paged Memory with OS_AllocatePool, so the user of this function has to releae 
// this memory.

NTSTATUS
COM_QueryPerformanceStatistics(
							IN PSESSION_MANAGER pSessionManager,
							OUT PVOID *ppPerformanceMetrics 
							)
{
	NTSTATUS status = STATUS_SUCCESS;
	LONG nTotalSize = 0;
	PSM_PERFORMANCE_INFO pSmPerformanceInfo = NULL;
	PLIST_ENTRY pTempEntry = NULL;
	PSERVER_ELEMENT pServerElement = NULL;
	PCONNECTION_PERFORMANCE_INFO pConnectionPerformanceInfo = NULL;
	ULONG nBufferLength = 0;

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(ppPerformanceMetrics != NULL);

	*ppPerformanceMetrics = NULL;
	if(pSessionManager->nTotalSessions == 0)
	{
		DebugPrint((DBG_COM, "COM_QueryPerformanceStatistics(): There are no sessions for this SessionManager %lx\n",pSessionManager));
		return status;
	}
	DebugPrint((DBG_COM, "COM_QueryPerformanceStatistics(): Entering COM_QueryPerformanceStatistics() for SessionManager %lx\n",pSessionManager));
	try
	{
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		nTotalSize = sizeof(SM_PERFORMANCE_INFO) + (pSessionManager->nTotalSessions-1)*sizeof(CONNECTION_PERFORMANCE_INFO);
		*ppPerformanceMetrics = OS_AllocMemory(NonPagedPool,nTotalSize);

		if(*ppPerformanceMetrics == NULL)
		{
			DebugPrint((DBG_ERROR, "COM_QueryPerformanceStatistics(): Unable to allocate Memory for the Performance Metrics\n"));
			status = STATUS_MEMORY_NOT_ALLOCATED;
			leave;
		}

		OS_ZeroMemory(*ppPerformanceMetrics,nTotalSize);

		pSmPerformanceInfo = (PSM_PERFORMANCE_INFO)(*ppPerformanceMetrics);

		status = TDI_QueryProviderInfo(TCP_DEVICE_NAME_W,&pSmPerformanceInfo->providerInfo);

		if(!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "COM_QueryPerformanceStatistics(): Unable to Obtain TDI_PROVIDER_INFO Information for SessionManager %lx\n",pSessionManager));
			leave;
		}

		DEBUG_DumpProviderInfo(TCP_DEVICE_NAME_W , &pSmPerformanceInfo->providerInfo);

		status = TDI_QueryProviderStatistics(TCP_DEVICE_NAME_W,&pSmPerformanceInfo->providerStatistics);

		if(!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "COM_QueryPerformanceStatistics(): Unable to Obtain TDI_PROVIDER_STATISTICS Information for SessionManager %lx\n",pSessionManager));
			leave;
		}

		DEBUG_DumpProviderStatistics(TCP_DEVICE_NAME_W , &pSmPerformanceInfo->providerStatistics);

		pConnectionPerformanceInfo = &pSmPerformanceInfo->conPerformanceInfo[0];

		pSmPerformanceInfo->nConnections = pSessionManager->nTotalSessions;

		pTempEntry = pSessionManager->ServerList.Flink;

		while(pTempEntry != &pSessionManager->ServerList)
		{
			pServerElement = CONTAINING_RECORD( pTempEntry,SERVER_ELEMENT,ListElement);
			pTempEntry = pTempEntry->Flink;

			pConnectionPerformanceInfo->ipLocalAddress.in_addr = pServerElement->LocalAddress.Address[0].Address[0].in_addr;
			pConnectionPerformanceInfo->ipLocalAddress.sin_port = pServerElement->LocalAddress.Address[0].Address[0].sin_port;

			pConnectionPerformanceInfo->ipRemoteAddress.in_addr = pServerElement->pSession->RemoteAddress.Address[0].Address[0].in_addr;
			pConnectionPerformanceInfo->ipRemoteAddress.sin_port = pServerElement->pSession->RemoteAddress.Address[0].Address[0].sin_port;

			if(pServerElement->pSession->KSEndpoint.m_pFileObject != NULL)
			{
				nBufferLength = sizeof(TDI_CONNECTION_INFO);
				status = TDI_QueryConnectionInfo(&pServerElement->pSession->KSEndpoint,&pConnectionPerformanceInfo->connectionInfo, &nBufferLength);

				if(NT_SUCCESS(status))
				{
					DEBUG_DumpConnectionInfo(&pConnectionPerformanceInfo->connectionInfo);
				}
			}
			pConnectionPerformanceInfo += 1;
		}	// while(pTempEntry != &pSessionManager->ServerList)
	}//try
	finally
	{
		if(!NT_SUCCESS(status) && *ppPerformanceMetrics != NULL)
		{
			OS_FreeMemory(*ppPerformanceMetrics);
			*ppPerformanceMetrics = NULL;
		}
		//Release the Resource 
		ExReleaseResourceLite(&pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	//Enable the Kernel APC's

		DebugPrint((DBG_COM, "COM_QueryPerformanceStatistics(): Leaving COM_QueryPerformanceStatistics()\n"));
	}
	return status;
}//COM_QueryPerformanceStatistics()

#endif	//__NEW_CONNECT__



