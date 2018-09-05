//tdiConnect.c
//Worker Thread that goes through all the SERVER_ELEMENT's and Connects to various Sessions.

//#include "common.h"
#include "tdiutil.h"
//#include <wchar.h>      // for time_t, dev_t
//#include <windef.h>
//#include "mmgr_ntkrnl.h"

//#include "slab.h"
//#include "mmg.h"
//#include "dtb.h"
//#include "sftkprotocol.h"

MMG_PUBLIC ULONG MmgDebugLevel = MMGDBG_HIGH;




//#define _USE_RECEIVE_EVENT_

NTSTATUS
SftkTCPCConnectedCallback(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp,
   IN PVOID Context
   )
{
	PTCP_SESSION pSession = (PTCP_SESSION)Context;
	NTSTATUS Status = STATUS_SUCCESS;
	KIRQL oldIrql;
	LONG nLiveSessions = 0;

#if DBG
	   DEBUG_DumpTransportAddress(
         (PTRANSPORT_ADDRESS )&pSession->RemoteAddress
         );
#endif


	try
	{
		if(!NT_SUCCESS(pIrp->IoStatus.Status))
		{
			KdPrint(("Failed pt Connect to the Secondry with Session = %lx and Id = %ld and Status = %lx\n",pSession,pSession->sessionID,pIrp->IoStatus.Status));
			pSession->bSessionEstablished = SFTK_DISCONNECTED;

		}
		else
		{

			KdPrint(("Successfully Connected to the Secondry with Session = %lx and Id = %ld\n",pSession,pSession->sessionID));

			//pSession->pServer->pSessionManager->nLiveSessions++;
			//Using the Interlocked Increment

			nLiveSessions = InterlockedIncrement(&pSession->pServer->pSessionManager->nLiveSessions);

			pSession->bSessionEstablished = SFTK_CONNECTED;
	
			if(nLiveSessions == 1)
			{
				KeSetEvent(&pSession->pServer->pSessionManager->GroupConnectedEvent,0,FALSE);
			}
		}
	}
	finally
	{
	}
	return STATUS_MORE_PROCESSING_REQUIRED;
}

//This command gets the Command that caused the ACK, since multiple Commands 
//can cause a comman ACK, we have to be careful about this. 
//A Hint is provided that will be useful in determining which Command could have 
//caused the ACK.
LONG GetCommandOfAck(IN protocol_e ReceivedProtocolCommand, IN ULONG Hint)
{
	LONG returnCommand = -1;

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
		returnCommand = FTDCCHKSUM;
		break;
	case FTDACKCHUNK:
		//The return header contains the ACK for the MSG_CO , MSG_INCO Sentinals.
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

LONG GetAckOfCommand(IN protocol_e SentProtocolCommand)
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
		case FTDCCHUNK:
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

LONG GetAckSentinal(IN enum sentinals   eSentinal)
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


PPROTOCOL_PACKET FindSentProtocolPacket(LONG nCommandType, PSESSION_MANAGER pSessionManager)
{
	PLIST_ENTRY pTempEntry = NULL;
	PPROTOCOL_PACKET pProtocolPacket = NULL;
	PPROTOCOL_PACKET pProtocolPacketFound = NULL;

	ASSERT(nCommandType > 0);


	try
	{
		//Loop and find the Protocol Packet that has this command
		pTempEntry = pSessionManager->ProtocolQueue.Flink;
		while(pTempEntry != &pSessionManager->ProtocolQueue)
		{
			pProtocolPacket = CONTAINING_RECORD(pTempEntry,PROTOCOL_PACKET,ListElement);

			//Check if we have the commandtype and also if it is 
			//already not signalled
			if((pProtocolPacket->ProtocolHeader.msgtype == nCommandType) && 
				(KeReadStateEvent(&pProtocolPacket->ReceiveAckEvent) == 0))
			{
				//Found the Comnmand Packet that belongs to the ACK
				KdPrint(("Found the Protocol Packet = %d of the Command \n", nCommandType));
				pProtocolPacketFound = pProtocolPacket;
				leave;				
			}
		}
	}
	finally
	{
	}
	return pProtocolPacketFound;
} 

//All these functions handle the Received Packets
//Only the ACK packets and the Back-Refresh Packets are used

NTSTATUS SftkReceiveHUP(IN ftd_header_t* pHeader , IN PTCP_SESSION pSession)
{
	//This will be handled on the secondary side, 
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKHUP(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	PPROTOCOL_PACKET pProtocolPacket = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	LONG nCommandType = -1;

	try
	{
		//Find the Command For this ACK
		//The Hint to this function depends on the group State 
		//and other Group Attributes
		nCommandType = GetCommandOfAck(pHeader->msgtype, 1);

		if(nCommandType == -1)
		{
			KdPrint(("There is no command Associated with the MsgType %d\n", pHeader->msgtype));
			leave;
		}

		pProtocolPacket = FindSentProtocolPacket(nCommandType,pSession->pServer->pSessionManager);
		if(pProtocolPacket == NULL)
		{
			KdPrint(("There is no Protocol Packet for the ACKHUP so just ignoring the Packet\n"));
			leave;
		}

		//Copy the Received ACK Header to the Original Header
		NdisMoveMemory(&pProtocolPacket->ReceivedProtocolHeader, pHeader , PROTOCOL_HEADER_SIZE);

		//Set the Other Parameters to the respective Values
		pProtocolPacket->pOutputBuffer = NULL;
		pProtocolPacket->OutputBufferLenget = 0;
		//Set the End Time
		KeQuerySystemTime(&pProtocolPacket->tEndTime);
		//Signal the Event
		KeSetEvent(&pProtocolPacket->ReceiveAckEvent , 0 , FALSE);
	}
	finally
	{
	}
	return Status;
}
//Full Refresh Start, Dealt on the secondary side
NTSTATUS SftkReceiveRFFSTART(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

//This is received on the Primary Side as part of the Full Refresh or the Smart Refresh
//Packets.
NTSTATUS SftkReceiveACKRSYNC(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	PPROTOCOL_PACKET pProtocolPacket = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	LONG nCommandType = -1;

	try
	{
		//Find the Command For this ACK
		//The Hint to this function depends on the group State 
		//and other Group Attributes
		nCommandType = GetCommandOfAck(pHeader->msgtype, 1);

		if(nCommandType == -1)
		{
			KdPrint(("There is no command Associated with the MsgType %d\n", pHeader->msgtype));
			leave;
		}

		pProtocolPacket = FindSentProtocolPacket(nCommandType,pSession->pServer->pSessionManager);
		if(pProtocolPacket == NULL)
		{
			KdPrint(("There is no Protocol Packet for the ACKRSYNC so just ignoring the Packet\n"));
			leave;
		}

		//Copy the Received ACK Header to the Original Header
		NdisMoveMemory(&pProtocolPacket->ReceivedProtocolHeader, pHeader , PROTOCOL_HEADER_SIZE);

		//Set the Other Parameters to the respective Values
		pProtocolPacket->pOutputBuffer = NULL;
		pProtocolPacket->OutputBufferLenget = 0;
		//Set the End Time
		KeQuerySystemTime(&pProtocolPacket->tEndTime);
		//Signal the Event
		KeSetEvent(&pProtocolPacket->ReceiveAckEvent , 0 , FALSE);
	}
	finally
	{
	}
	return Status;
}

//This is dealt on the Secondary Side
NTSTATUS SftkReceiveRFFEND(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

//This is dealt on the Secondary Side
NTSTATUS SftkReceiveBFSTART(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKCLI(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	PPROTOCOL_PACKET pProtocolPacket = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	LONG nCommandType = -1;

	try
	{
		//Find the Command For this ACK
		//The Hint to this function depends on the group State 
		//and other Group Attributes
		nCommandType = GetCommandOfAck(pHeader->msgtype, 1);

		if(nCommandType == -1)
		{
			KdPrint(("There is no command Associated with the MsgType %d\n", pHeader->msgtype));
			leave;
		}

		pProtocolPacket = FindSentProtocolPacket(nCommandType,pSession->pServer->pSessionManager);
		if(pProtocolPacket == NULL)
		{
			KdPrint(("There is no Protocol Packet for the ACKCLI so just ignoring the Packet\n"));
			leave;
		}

		//Copy the Received ACK Header to the Original Header
		NdisMoveMemory(&pProtocolPacket->ReceivedProtocolHeader, pHeader , PROTOCOL_HEADER_SIZE);

		//Set the Other Parameters to the respective Values
		pProtocolPacket->pOutputBuffer = NULL;
		pProtocolPacket->OutputBufferLenget = 0;
		//Set the End Time
		KeQuerySystemTime(&pProtocolPacket->tEndTime);
		//Signal the Event
		KeSetEvent(&pProtocolPacket->ReceiveAckEvent , 0 , FALSE);
	}
	finally
	{
	}
	return Status;
}

//Dealt on the secondary Side
NTSTATUS SftkReceiveRSYNCDEVS(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}
//Dealt on the secondary side
NTSTATUS SftkReceiveBFEND(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

//This is in resonse to the MSG_INCO OR MSG_AVOID_JOURNALS
NTSTATUS SftkReceiveACKRFSTART(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	//Do Nothing Just return
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKCPSTART(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKCPSTOP(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKCPON(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKCPOFF(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveCPONERR(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveCPOFFERR(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKCHUNK(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveNOOP(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKHANDSHAKE(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKCONFIG(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

//These are commands with Payload

NTSTATUS SftkReceiveRFBLK(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

//Got the Error Message So write to EventLog
NTSTATUS SftkReceiveACKERR(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession)
{
	//We have to log this error to the eventlog, this is for TODO::
	//For the timebeing just print the message
	KdPrint(("The Error from the Secondary is %s\n",(PCHAR)pDataBuffer));
	return STATUS_SUCCESS;
}


NTSTATUS SftkReceiveCHKSUM(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveBFBLK(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKCHKSUM(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveCHUNK(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveVERSION(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveHANDSHAKE(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveCHKCONFIG(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkReceiveACKVERSION1(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkCompressBuffer(IN PUCHAR pInputBuffer , IN ULONG nInputLength , OUT PUCHAR* pOutBuffer ,OUT PULONG pOutputLength , IN LONG algorithm)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkDecompressBuffer(IN PUCHAR pInputBuffer , IN ULONG nInputLength , OUT PUCHAR* pOutBuffer , OUT PULONG pOutputLength , IN LONG algorithm)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkExpandBuffer(IN UCHAR cFillChar , OUT PUCHAR* pOutBuffer , IN ULONG nDataLength)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkCreateChecksum(IN PUCHAR pInputBuffer , IN ULONG nInputLength , OUT PUCHAR* pOutBuffer , OUT PULONG pOutputLength , IN LONG algorithm)
{
	return STATUS_SUCCESS;
}

NTSTATUS SftkCompareChecksum(IN PUCHAR pInputBuffer1 , IN PUCHAR pInputBuffer2 , IN ULONG nDataLength , IN LONG algorithm)
{
	return STATUS_SUCCESS;
}


//This function validates the header and returns the length of the data
//that needs to be read after the header.
NTSTATUS SftkProcessReceiveHeader(ftd_header_t* pHeader, PULONG pLen, PTCP_SESSION pSession)
{
	NTSTATUS Status = STATUS_SUCCESS;

	if(pHeader->magicvalue != MAGICHDR)
	{
		*pLen = 0;
		KdPrint(("The header Received is Invalid \n"));
		Status = STATUS_INVALID_USER_BUFFER;
	}
	else
	{
		*pLen = 0;
		KdPrint(("The header Received is Valid the Length = %d \n",pHeader->len));

		//Process different kinds of Commands
		switch(pHeader->msgtype)
		{
		case FTDCHUP:		//No Payload
			KdPrint(("The Packet Received is of type FTDCHUP\n"));
			Status = SftkReceiveHUP(pHeader,pSession);
			break;
		case FTDACKHUP:		//No Payload
			KdPrint(("The Packet Received is of type FTDACKHUP\n"));
			Status = SftkReceiveACKHUP(pHeader,pSession);
			break;
		case FTDCRFFSTART:	//No Payload
			KdPrint(("The Packet Received is of type FTDCRFFSTART\n"));
			Status = SftkReceiveRFFSTART(pHeader, pSession);
			break;
		case FTDCRFBLK:		//Payload
			KdPrint(("The Packet Received is of type FTDCRFBLK\n"));
			if(pHeader->msg.lg.data == FTDZERO)
			{
				//Contains Data as all Zeros
				KdPrint(("The Data contains only Zero's as Data and the length = %ld\n",pHeader->len));
				*pLen = 0;
			}
			else if(pHeader->compress)
			{
				//The Compression is enabled so will have to be decompressed before writing to
				//the Disk
				KdPrint(("Compression is Enabled and compressed len = %ld , uncomplen = %ld\n" , pHeader->len , pHeader->uncomplen));
				*pLen = pHeader->len;
				Status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			else
			{
				//Neither Compression or FTDZERO is Enabled, so Just get the buffer
				*pLen = pHeader->len;
				Status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			break;
		case FTDACKRSYNC:	//No Payload
			KdPrint(("The Packet Received is of type FTDACKRSYNC\n"));
			Status = SftkReceiveACKRSYNC(pHeader,pSession);
			break;
		case FTDCRFFEND:	//No Payload
			KdPrint(("The Packet Received is of type FTDCRFFEND\n"));
			Status = SftkReceiveRFFEND(pHeader, pSession);
			break;
		case FTDCBFSTART:	//No Payload
			KdPrint(("The Packet Received is of type FTDCBFSTART\n"));
			Status = SftkReceiveBFSTART(pHeader, pSession);
			break;
		case FTDACKCLI:		//No Payload
			KdPrint(("The Packet Received is of type FTDACKCLI\n"));
			Status = SftkReceiveACKCLI(pHeader, pSession);
			break;
		case FTDACKERR:		//Payload
			KdPrint(("The Packet Received is of type FTDACKERR\n"));
			*pLen = sizeof(ftd_err_t);
			Status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDCRSYNCDEVS:	//No Payload
			KdPrint(("The Packet Received is of type FTDCRSYNCDEVS\n"));
			Status = SftkReceiveRSYNCDEVS(pHeader, pSession);
			break;
		case FTDCCHKSUM:	//Payload
			KdPrint(("The Packet Received is of type FTDCCHKSUM\n"));
			*pLen = sizeof(ftd_dev_t);
			//After getting the Device info, the fields in the ftd_dev_t are used to
			//get the CheckSum Payload
			Status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDCBFBLK:		//Payload
			KdPrint(("The Packet Received is of type FTDCBFBLK\n"));
			if(pHeader->msg.lg.data == FTDZERO)
			{
				//Contains Data as all Zeros
				KdPrint(("The Data contains only Zero's as Data and the length = %ld\n",pHeader->len));
				*pLen = 0;
			}
			else if(pHeader->compress)
			{
				//The Compression is enabled so will have to be decompressed before writing to
				//the Disk
				KdPrint(("Compression is Enabled and compressed len = %ld , uncomplen = %ld\n" , pHeader->len , pHeader->uncomplen));
				*pLen = pHeader->len;
				Status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			else
			{
				//Neither Compression or FTDZERO is Enabled, so Just get the buffer
				*pLen = pHeader->len;
				Status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			break;
		case FTDCBFEND:		//No Payload
			KdPrint(("The Packet Received is of type FTDCBFEND\n"));
			Status = SftkReceiveBFEND(pHeader, pSession);
			break;
		case FTDACKRFSTART:	//No Payload
			KdPrint(("The Packet Received is of type FTDACKRFSTART\n"));
			Status = SftkReceiveACKRFSTART(pHeader, pSession);
			break;
		case FTDACKCHKSUM:	//Payload
			KdPrint(("The Packet Received is of type FTDACKCHKSUM\n"));
			//The Length of Payload is the Sizeof the Device and the Length of the
			// ftd_dev_delta_t structures
			*pLen = sizeof(ftd_dev_t)+ pHeader->len;
			Status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDACKCPSTART:	//No Payload
			KdPrint(("The Packet Received is of type FTDACKCPSTART\n"));
			Status = SftkReceiveACKCPSTART(pHeader, pSession);
			break;
		case FTDACKCPSTOP:	//No Payload
			KdPrint(("The Packet Received is of type FTDACKCPSTOP\n"));
			Status = SftkReceiveACKCPSTOP(pHeader, pSession);
			break;
		case FTDCCPONERR:	//No Payload
			KdPrint(("The Packet Received is of type FTDCCPONERR\n"));
			Status = SftkReceiveCPONERR(pHeader, pSession);
			break;
		case FTDCCPOFFERR:	//No Payload
			KdPrint(("The Packet Received is of type FTDCCPOFFERR\n"));
			Status = SftkReceiveCPOFFERR(pHeader, pSession);
			break;
		case FTDACKCPON:	//No Payload
			KdPrint(("The Packet Received is of type FTDACKCPON\n"));
			Status = SftkReceiveACKCPON(pHeader , pSession);
			break;
		case FTDACKCPOFF:	//No Payload
			KdPrint(("The Packet Received is of type FTDACKCPOFF\n"));
			Status = SftkReceiveACKCPOFF(pHeader , pSession);
			break;
		case FTDCCHUNK:		//Payload
			KdPrint(("The Packet Received is of type FTDCCHUNK\n"));
			//Compression could be Enabled
			if(pHeader->compress)
			{
				//The Compression is enabled so will have to be decompressed before writing to
				//the Disk
				KdPrint(("Compression is Enabled and compressed len = %ld , uncomplen = %ld\n" , pHeader->len , pHeader->uncomplen));
				*pLen = pHeader->len;
				Status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			else
			{
				//Compression is not Enabled so get the Length
				*pLen = pHeader->len;
				Status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			break;
		case FTDACKCHUNK:	//No Payload
			KdPrint(("The Packet Received is of type FTDACKCHUNK\n"));
			//Doesnt have any payload, but the pHeader->msg.lg.flags can contain RFDONE
			//Or RFSTART
			Status = SftkReceiveACKCHUNK(pHeader , pSession);
			break;
		case FTDCNOOP:		//No Payload
			KdPrint(("The Packet Received is of type FTDCNOOP\n"));
			Status = SftkReceiveNOOP(pHeader , pSession);
			break;
		case FTDCVERSION:	//Payload
			KdPrint(("The Packet Received is of type FTDCVERSION\n"));
			*pLen = sizeof(ftd_version_t);
			//Get the Primary Version Information
			Status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDCHANDSHAKE:	//Payload
			KdPrint(("The Packet Received is of type FTDCHANDSHAKE\n"));
			*pLen = sizeof(ftd_auth_t);
			//Get the Authentication Code 
			Status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDACKHANDSHAKE:	//No Payload
			KdPrint(("The Packet Received is of type FTDACKHANDSHAKE\n"));
			Status = SftkReceiveACKHANDSHAKE(pHeader , pSession);
			break;
		case FTDCCHKCONFIG:		//Payload
			KdPrint(("The Packet Received is of type FTDCCHKCONFIG\n"));
			//Add the length of the ftd_rdev_t structure that will be sent along with
			//header
			*pLen += sizeof(ftd_rdev_t);
			Status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		case FTDACKCONFIG:	//No Payload
			KdPrint(("The Packet Received is of type FTDACKCONFIG\n"));
			//The Size of the secondary Device and the Device id are returned
			Status = SftkReceiveACKCONFIG(pHeader , pSession);
			break;
		case FTDACKVERSION1:			//Payload This is the ACK for FTDCVERSION
			KdPrint(("Unknown Type of Packet is received\n"));
			//The Secondary Version String Length is in pHeader->msg.lg.data
			*pLen = pHeader->msg.lg.data;
			Status = STATUS_MORE_PROCESSING_REQUIRED;
			break;
		default:
			KdPrint(("Packet is received of type %ld\n", pHeader->msgtype ));
			break;
		}
	}
return Status;
}
//This Function will have the data along with the Header.
//Upon receiving this data and header the respective 
//function is called to process the data.
//If there is still more data that needs to be read from the payload.
//this function will return STATUS_MORE_PROCESSING_REQUIRED and return the
//length of the buffer that still needs to be read in pLen
NTSTATUS SftkProcessReceiveData(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , OUT PULONG pLen , IN PTCP_SESSION pSession)
{
	NTSTATUS Status = STATUS_SUCCESS;
	ftd_dev_t	*rdevp= NULL;

	if(pHeader->magicvalue != MAGICHDR)
	{
		*pLen = 0;
		KdPrint(("The header Received is Invalid \n"));
		Status = STATUS_INVALID_USER_BUFFER;
	}
	else
	{
		*pLen = 0;
		//These are all the commands with Payloads
		//The payloads needs to be processed accordingly
		//For Comamnds that dont have a payload the processing is done in the 
		//function SftkProcessReceiveHeader()
		
		switch(pHeader->msgtype)
		{
		case FTDCRFBLK:		//Payload
			KdPrint(("The Data Packet Received is of type FTDCRFBLK\n"));
			//This Function will takecare of the Processing of the Received Refresh Block
			Status = SftkReceiveRFBLK(pHeader, pDataBuffer , nDataLength , pSession);
			break;
		case FTDACKERR:		//Payload
			KdPrint(("The Data Packet Received is of type FTDACKERR\n"));
			Status = SftkReceiveACKERR(pHeader, pDataBuffer , nDataLength , pSession);
			break;
		case FTDCCHKSUM:
			KdPrint(("The Data Packet Received is of type FTDCCHKSUM ftd_dev_t structure is read\n"));

			if(nDataLength > sizeof(ftd_header_t)+sizeof(ftd_dev_t))
			{
				Status = SftkReceiveCHKSUM(pHeader , pDataBuffer , nDataLength , pSession);
			}
			else if(nDataLength == sizeof(ftd_header_t)+sizeof(ftd_dev_t))
			{
				//We have to read the Device ftd_dev_t, So read it and get the 
				//CheckSum Payload
				rdevp = (ftd_dev_t*)pDataBuffer;
				*pLen = rdevp->sumnum*DIGESTSIZE;
				//Each DIGESTSIZE represents one CHKSEGSIZE
				Status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			break;
		case FTDCBFBLK:		//Payload
			KdPrint(("The Data Packet Received is of type FTDCBFBLK\n"));
			Status = SftkReceiveBFBLK(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDACKCHKSUM:	//Payload
			KdPrint(("The Data Packet Received is of type FTDACKCHKSUM\n"));
			Status = SftkReceiveACKCHKSUM(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDCCHUNK:		//Payload
			KdPrint(("The Packet Received is of type FTDCCHUNK\n"));
			Status = SftkReceiveCHUNK(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDCVERSION:	//Payload
			KdPrint(("The Packet Received is of type FTDCVERSION\n"));
			Status = SftkReceiveVERSION(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDCHANDSHAKE:	//Payload
			KdPrint(("The Packet Received is of type FTDCHANDSHAKE\n"));
			Status = SftkReceiveHANDSHAKE(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDCCHKCONFIG:		//Payload
			KdPrint(("The Packet Received is of type FTDCCHKCONFIG\n"));
			Status = SftkReceiveCHKCONFIG(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		case FTDACKVERSION1:			//Payload This is the ACK for FTDCVERSION
			KdPrint(("Unknown Type of Packet is received\n"));
			Status = SftkReceiveACKVERSION1(pHeader , pDataBuffer , nDataLength , pSession);
			break;
		default:
			KdPrint(("Packet is received of type %ld\n", pHeader->msgtype ));
			break;
		}
	}
return Status;
}


SftkEndpointReceiveRequestComplete(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp,
	IN PVOID Context
	)
{
	PRECEIVE_BUFFER pReceiveBuffer= NULL;
	PRECEIVE_BUFFER pNewReceiveBuffer = NULL;
	PTCP_SESSION pSession = NULL;
	NTSTATUS Status;
	ULONG Length;
	ULONG Len=0;
	
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
		

		if(pReceiveBuffer->ActualReceiveLength < pReceiveBuffer->TotalReceiveLength)
		{
			//If Data Received is less than the Total then call the IRP with the new MDL

			pReceiveBuffer->pReceiveMdl = SFTK_TDI_AllocateAndProbeMdl(pReceiveBuffer->pReceiveBuffer + pReceiveBuffer->ActualReceiveLength,pReceiveBuffer->TotalReceiveLength-pReceiveBuffer->ActualReceiveLength,FALSE,FALSE,NULL);


			//Call the TDI_RECEIVE with the new MDL
			try
			{
				IoReuseIrp(pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
				Status = SFTK_TDI_ReceiveOnEndpoint1(
							&pSession->KSEndpoint,
							NULL,       // User Completion Event
							SftkEndpointReceiveRequestComplete,// User Completion Routine
							pReceiveBuffer,   // User Completion Context
							&pReceiveBuffer->IoReceiveStatus,
							pReceiveBuffer->pReceiveMdl,
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
				Status = SftkProcessReceiveHeader(((ftd_header_t*)pReceiveBuffer->pReceiveBuffer),&Len, pSession);

				//If Valid Header and if there is a payload then get the new Buffer and 
				//do TDI_RECEIVE again.

				if( Status == STATUS_SUCCESS || Status == STATUS_MORE_PROCESSING_REQUIRED)
				{
					if(Len > 0)
					{
//						pNewReceiveBuffer = GetNextReceiveBuffer(&pSession->pServer->pSessionManager->receiveBufferList);

//						if(pNewReceiveBuffer == NULL)
//						{
//							//There is no Free Buffer available so returning
//							KdPrint(("Unable to get the Free Buffer to Receive Data for Session %lx\n",pSession));
//							return STATUS_MORE_PROCESSING_REQUIRED;
//						}

						Status = GetSessionReceiveBuffer(pSession);

						if(!NT_SUCCESS(Status))
						{
							//There is no Free Buffer available so returning
							KdPrint(("Unable to get the Free Buffer to Receive Data for Session %lx\n",pSession));
							return STATUS_MORE_PROCESSING_REQUIRED;
						}

						pNewReceiveBuffer = pSession->pReceiveBuffer;

						ASSERT(pNewReceiveBuffer);
						ASSERT(pNewReceiveBuffer->pReceiveBuffer);

						//This is the new Buffer to call TDI_RECEIVE
						pNewReceiveBuffer->state = SFTK_BUFFER_INUSE;
						pNewReceiveBuffer->ActualReceiveLength = 0;
						pNewReceiveBuffer->TotalReceiveLength = Len;
						pNewReceiveBuffer->pReceiveMdl = SFTK_TDI_AllocateAndProbeMdl(pNewReceiveBuffer->pReceiveBuffer,pNewReceiveBuffer->TotalReceiveLength ,FALSE,FALSE,NULL);
						pNewReceiveBuffer->pReceiveMdl->Next = NULL;
						pNewReceiveBuffer->pSession = pSession;

						ASSERT(pNewReceiveBuffer->pReceiveMdl);

						IoReuseIrp(pSession->pReceiveHeader->pReceiveIrp,STATUS_SUCCESS);
						try
						{
							Status = SFTK_TDI_ReceiveOnEndpoint1(
										&pSession->KSEndpoint,
										NULL,       // User Completion Event
										SftkEndpointReceiveRequestComplete,// User Completion Routine
										pNewReceiveBuffer,   // User Completion Context
										&pNewReceiveBuffer->IoReceiveStatus,
										pNewReceiveBuffer->pReceiveMdl,
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
		KdPrint(( "SftkTCPSreceiveBuffer: Status 0x%8.8X and Index = %d for ReceiveBuffer = %lx\n", Status , pReceiveBuffer->index , pReceiveBuffer));
		pReceiveBuffer->state = SFTK_BUFFER_FREE;
	}

//	IoFreeIrp( pIrp );

//	pReceiveBuffer->state = SFTK_BUFFER_FREE;
	return STATUS_MORE_PROCESSING_REQUIRED;
}

//This Receive Event Handler demonestrates the use of the IoRequestPacket
//For getting Data that is more than the Indicated.
NTSTATUS SftkTCPCReceiveEventHandler1(
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
	NTSTATUS Status = STATUS_SUCCESS;
	PTCP_SESSION pSession = NULL;
	PIRP pIrp = NULL;
	PRECEIVE_BUFFER pReceiveBuffer= NULL;
	PDEVICE_OBJECT pDeviceObject;
	ULONG Length;

	pSession = (PTCP_SESSION)TdiEventContext;
	pDeviceObject = IoGetRelatedDeviceObject( pSession->KSEndpoint.m_pFileObject );

    KdPrint(("SftkTCPCReceiveEventHandler: Entry; EventContext: 0x%8.8X; ConnectionContext: 0x%8.8X; Flags: 0x%8.8X\n",
      (ULONG )TdiEventContext, (ULONG)ConnectionContext, ReceiveFlags)
      );

	KdPrint(("SftkTCPCReceiveEventHandler: Bytes Indicated = %d , Bytes Available = %d for session %lx\n",
						BytesIndicated,
						BytesAvailable,
						pSession));

	if(BytesIndicated > 64 )
//	if(BytesIndicated < BytesAvailable)
	{
		KdPrint(("We got a Partial Message\n"));
		pReceiveBuffer = GetNextReceiveBuffer(&pSession->pServer->pSessionManager->receiveBufferList);
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
				SftkEndpointReceiveRequestComplete,   // Completion routine
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

//Set the IO Receive Event, Which is triggered whenever there is data on the Wire.
//The Receive Thread will wait for all the sessions in the Group.

NTSTATUS SftkTCPCReceiveEventHandler(
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

    KdPrint(("SftkTCPCReceiveEventHandler: Entry; EventContext: 0x%8.8X; ConnectionContext: 0x%8.8X; Flags: 0x%8.8X\n",
      (ULONG )TdiEventContext, (ULONG)ConnectionContext, ReceiveFlags)
      );

	KdPrint(("SftkTCPCReceiveEventHandler: Bytes Indicated = %d , Bytes Available = %d for session %lx\n",
						BytesIndicated,
						BytesAvailable,
						pSession));
	//Just set the Event and do nothing else
	*BytesTaken = 0;
	KeSetEvent(&pSession->IOReceiveEvent,0,FALSE);
	return STATUS_SUCCESS;
}


NTSTATUS SftkTCPSDisconnectEventHandler(
   IN PVOID TdiEventContext,     // Context From SetEventHandler
   IN CONNECTION_CONTEXT ConnectionContext,
   IN LONG DisconnectDataLength,
   IN PVOID DisconnectData,
   IN LONG DisconnectInformationLength,
   IN PVOID DisconnectInformation,
   IN ULONG DisconnectFlags
   )
{
	PTCP_SESSION pSession = NULL;
	KIRQL oldIrql;
	LONG nLiveSessions =0;

   KdPrint(("SftkTCPSDisconnectEventHandler: Entry; EventContext: 0x%8.8X; ConnectionContext: 0x%8.8X; Flags: 0x%8.8X\n",
      (ULONG )TdiEventContext, (ULONG)ConnectionContext, DisconnectFlags)
      );

   pSession = (PTCP_SESSION)TdiEventContext;

   KdPrint(("The Disconnected Session is = %d\n",pSession->sessionID));

   //pSession->pServer->pSessionManager->nLiveSessions--;
   //Doing InterlockedDecrement

   nLiveSessions = InterlockedDecrement(&pSession->pServer->pSessionManager->nLiveSessions);
   pSession->bSessionEstablished = SFTK_DISCONNECTED;

   if(nLiveSessions ==0)
   {
	   KdPrint(("All the Connections for this SESSION_MANAGER are Down\n"));
	   KeClearEvent(&pSession->pServer->pSessionManager->GroupConnectedEvent);
   }
   return( STATUS_SUCCESS );
}

void UninitializeConnectListenThread(PSESSION_MANAGER pSessionManager)
{
	LONG lErrorCode =0;
	NTSTATUS Status = STATUS_SUCCESS;
	PSERVER_ELEMENT pServer = NULL;
	LARGE_INTEGER iWait;
	PLIST_ENTRY pTempEntry;

	if(!pSessionManager->bInitialized)
	{
		return;
	}
	try
	{
		try
		{
			iWait.QuadPart = -5*1000*10*1000; //100 Milli Seconds.
			KeSetEvent(&pSessionManager->GroupUninitializedEvent,0,FALSE);
			KeDelayExecutionThread( KernelMode, FALSE, &iWait );

			StopSendReceiveThreads(pSessionManager);
			KeDelayExecutionThread( KernelMode, FALSE, &iWait );

			ExDeleteNPagedLookasideList(&pSessionManager->ProtocolList);

			KdPrint(("Successfully Stopped the Send and Receive Threads\n"));
			pTempEntry = pSessionManager->ServerList.Flink;

			while(pTempEntry != &pSessionManager->ServerList)
			{
				pServer = CONTAINING_RECORD(pTempEntry,SERVER_ELEMENT,ListElement);
				KdPrint(("Uninitializing the Server %lx and Session %lx\n",pServer, pServer->pSession));
				UninitializeServer(pServer);
				pTempEntry = pTempEntry->Flink;
			}

			KeDelayExecutionThread( KernelMode, FALSE, &iWait );

			KdPrint(("Successfully Stopped all the Sessions\n"));

			while(!IsListEmpty(&pSessionManager->ServerList))
			{
				pTempEntry = RemoveHeadList(&pSessionManager->ServerList);
				pServer = CONTAINING_RECORD(pTempEntry,SERVER_ELEMENT,ListElement);
				CleanUpServer(pServer);
				pServer = NULL;
			}
			KdPrint(("Successfully Cleared the Sessions\n"));

			ClearSendBufferList(&pSessionManager->sendBufferList);
			KdPrint(("Successfully Cleared the Send BufferList\n"));
			ClearReceiveBufferList(&pSessionManager->receiveBufferList);
			KdPrint(("Successfully Cleared the Receive BufferList\n"));

		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint(("An Exception Occured in UninitializeConnectListenThread() Error Code is %lx\n",GetExceptionCode()));
			Status = GetExceptionCode();
		}
	}
	finally
	{
		if(pSessionManager->lpConnectionDetails != NULL)
		{
			NdisFreeMemory(pSessionManager->lpConnectionDetails,0,0);
			pSessionManager->lpConnectionDetails = NULL;
		}
	}
}

void CleanUpServer(PSERVER_ELEMENT pServer)
{
	LONG lErrorCode =0;
	NTSTATUS Status = STATUS_SUCCESS;
	PLIST_ENTRY pTempEntry;

	ASSERT(pServer);

	if(pServer->pSession != NULL)
	{
		CleanUpSession(pServer->pSession);
		pServer->pSession = NULL;
	}
	NdisFreeMemory(pServer,sizeof(SERVER_ELEMENT),0);
	pServer = NULL;
}

void CleanUpSession(PTCP_SESSION pSession)
{
	ASSERT(pSession);

	if(pSession != NULL)
	{
		if(pSession->pListenIrp != NULL)
		{
	        IoFreeIrp( pSession->pListenIrp );
			pSession->pListenIrp = NULL;
		}

		if(pSession->pReceiveHeader != NULL)
		{
			ClearReceiveBuffer(pSession->pReceiveHeader);
			pSession->pReceiveHeader = NULL;
		}

		if(pSession->pReceiveBuffer != NULL)
		{
			ClearReceiveBuffer(pSession->pReceiveBuffer);
			pSession->pReceiveBuffer = NULL;
		}
		pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
		pSession->pServer = NULL;
		NdisFreeMemory(pSession,sizeof(TCP_SESSION),0);
	}
	pSession = NULL;
}


void UninitializeServer(PSERVER_ELEMENT pServer)
{
	LONG lErrorCode =0;
	NTSTATUS Status = STATUS_SUCCESS;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;

	ASSERT(pServer);

	if(pServer->pSession != NULL)
	{
		KdPrint(("Calling pSession %lx\n",pServer->pSession));
		UninitializeSession(pServer->pSession);
	}
	SFTK_TDI_CloseTransportAddress( &pServer->KSAddress);
}
void UninitializeSession(PTCP_SESSION pSession)
{
	LONG lErrorCode =0;
	NTSTATUS Status = STATUS_SUCCESS;
	LARGE_INTEGER DelayTime;

	try
	{

		KdPrint(("pSession->bSessionEstablished for session %lx\n",pSession));
		if(pSession->bSessionEstablished == SFTK_CONNECTED)
		{
			KdPrint(("Before calling the SFTK_TDI_Disconnect for session %lx\n",pSession));
			Status = SFTK_TDI_Disconnect(
				&pSession->KSEndpoint,
				NULL,    // UserCompletionEvent
				NULL,    // UserCompletionRoutine
				NULL,    // UserCompletionContext
				NULL,    // pIoStatusBlock
				0        // Disconnect Flags
				);
		}

		KdPrint(( "Disconnect Status: 0x%8.8x for Session %d\n", Status, pSession->sessionID ));

		KdPrint(( "Waiting After Disconnect...\n" ));

		DelayTime.QuadPart = 10*1000*50;   // 50 MilliSeconds

		KeDelayExecutionThread( KernelMode, FALSE, &DelayTime );

		pSession->bSessionEstablished = SFTK_DISCONNECTED;
		SFTK_TDI_CloseConnectionEndpoint( &pSession->KSEndpoint );
		
	}
	finally
	{

	}

}


LONG InitializeServer(PSERVER_ELEMENT *ppServer,PCONNECTION_INFO pConnectionInfo)
{

	LONG lErrorCode=0;
	NTSTATUS Status = STATUS_SUCCESS;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PSERVER_ELEMENT pServer = NULL;

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
			KdPrint(("Unable to Accocate Memory for SERVER_ELEMENT Status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pServer,sizeof(SERVER_ELEMENT));
		
		SFTK_TDI_InitIPAddress(&pServer->LocalAddress,pConnectionInfo->ipLocalAddress.in_addr,pConnectionInfo->ipLocalAddress.sin_port);

		//
		// Open Transport Address
		//
		Status = SFTK_TDI_OpenTransportAddress(
				TCP_DEVICE_NAME_W,
				(PTRANSPORT_ADDRESS )&pServer->LocalAddress,
				&pServer->KSAddress
				);


		if( !NT_SUCCESS( Status ) )
		{
			KdPrint(("Transport Address Couldnt be opened So Quitting\n"));
			//
			// Address Object Could Not Be Created
			//
			leave;
		}


	}
	finally
	{
		if(!NT_SUCCESS(nNdisStatus) || !NT_SUCCESS(Status))
		{
			KdPrint(("The Memory Allocation Or OpenTransportAddress() Failed Getting out of the for Loop\n"));
			if(pServer != NULL)
			{
				SFTK_TDI_CloseTransportAddress( &pServer->KSAddress);
				NdisFreeMemory(pServer,sizeof(SERVER_ELEMENT),0);
			}
			pServer =NULL;
			lErrorCode = 1;
		}
	}
	*ppServer = pServer;
	return lErrorCode;
}

void ClearSendBuffer(PSEND_BUFFER pSendBuffer)
{
	if(pSendBuffer != NULL)
	{
		if(pSendBuffer->pSendMdl != NULL)
		{
			SFTK_TDI_UnlockAndFreeMdl( pSendBuffer->pSendMdl );
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
		KdPrint(("Freed the Send Buffer for buffer %lx and index = %d\n",pSendBuffer,pSendBuffer->index));
		pSendBuffer->pSendList = NULL;
		NdisFreeMemory(pSendBuffer,sizeof(SEND_BUFFER),0);
		pSendBuffer = NULL;
	}
}
void ClearReceiveBuffer(PRECEIVE_BUFFER pReceiveBuffer)
{
	if(pReceiveBuffer != NULL)
	{
		if(pReceiveBuffer->pReceiveMdl != NULL)
		{
			SFTK_TDI_UnlockAndFreeMdl( pReceiveBuffer->pReceiveMdl );
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
		KdPrint(("Freed the Receive Buffer for buffer %lx and index %d\n",pReceiveBuffer,pReceiveBuffer->index));
		pReceiveBuffer->pReceiveList = NULL;
		NdisFreeMemory(pReceiveBuffer,sizeof(RECEIVE_BUFFER),0);
		pReceiveBuffer = NULL;
	}
}
void ClearReceiveBufferList(PRECEIVE_BUFFER_LIST pReceiveList)
{
	LONG lErrorCode =0;
	PLIST_ENTRY pTempEntry;
	PRECEIVE_BUFFER pReceiveBuffer = NULL;

	KdPrint(("Enter ClearReceiveBufferList()\n"));

	while(!IsListEmpty(&pReceiveList->FreeSessionList))
	{
		pTempEntry = RemoveTailList(&pReceiveList->FreeSessionList);
		pReceiveBuffer = CONTAINING_RECORD(pTempEntry,RECEIVE_BUFFER,ListElement);
		KdPrint(("Calling ClearReceiveBuffer() for Free buffer %lx\n",pReceiveBuffer));
		ClearReceiveBuffer(pReceiveBuffer);
		pReceiveBuffer = NULL;
	}
	pReceiveList->pSessionManager = NULL;
	pReceiveList->NumberOfReceiveBuffers =0;
	KdPrint(("leaving ClearReceiveBufferList()\n"));
}
void ClearSendBufferList(PSEND_BUFFER_LIST pSendList)
{
	LONG lErrorCode =0;
	PLIST_ENTRY pTempEntry;
	PSEND_BUFFER pSendBuffer = NULL;

	KdPrint(("Enter ClearSendBufferList()\n"));
	while(!IsListEmpty(&pSendList->FreeSessionList))
	{
		pTempEntry = RemoveTailList(&pSendList->FreeSessionList);
		pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,ListElement);
		KdPrint(("calling ClearSendBuffer for Free buffer %lx\n",pSendBuffer));
		ClearSendBuffer(pSendBuffer);
		pSendBuffer = NULL;
	}
	pSendList->pSessionManager = NULL;
	pSendList->NumberOfSendBuffers =0;
	KdPrint(("Leaving ClearSendBufferList()\n"));
}

LONG CreateNewSendBuffer(PSEND_BUFFER* ppSendBuffer,ULONG nSendWindow)
{
	LONG lErrorCode=0;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PSEND_BUFFER pSendBuffer=NULL;

	KdPrint(("Entering the CreateNewSendBuffer()\n"));

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
			KdPrint(("Unable to Accocate Memory for SEND_BUFFER Status = %lx\n",nNdisStatus));
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
			KdPrint(("Unable to Accocate Memory for SendBuffer Status = %lx\n",nNdisStatus));
			leave;
		}

		//Initializing the Memory
		NdisZeroMemory(pSendBuffer->pSendBuffer,nSendWindow);

		//Probe and Lock the Send Memory
		//Commented and Moved Down
//		pSendBuffer->pSendMdl = SFTK_TDI_AllocateAndProbeMdl(pSendBuffer->pSendBuffer,nSendWindow,FALSE,FALSE,NULL);

//		if(pSendBuffer->pSendMdl == NULL)
//		{
//			KdPrint(("Unable to lock the SendBuffer in Momory\n"));
//			nNdisStatus = STATUS_INSUFFICIENT_RESOURCES;
//			leave;
//		}

		pSendBuffer->state = SFTK_BUFFER_FREE;
	}
	finally
	{
		if(!NT_SUCCESS(nNdisStatus))
		{
			if(pSendBuffer != NULL)
			{
				ClearSendBuffer(pSendBuffer);
				pSendBuffer = NULL;
			}
			lErrorCode = 1;
		}
	}
	*ppSendBuffer = pSendBuffer;
	KdPrint(("Leaving CreateNewSendBuffer()\n"));
	return lErrorCode;
}

LONG CreateNewReceiveBuffer(PRECEIVE_BUFFER* ppReceiveBuffer,ULONG nReceiveWindow)
{
	LONG lErrorCode=0;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PRECEIVE_BUFFER pReceiveBuffer=NULL;

	KdPrint(("Enter CreateNewReceiveBuffer()\n"));

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
			KdPrint(("Unable to Accocate Memory for RECEIVE_BUFFER Status = %lx\n",nNdisStatus));
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
			KdPrint(("Unable to Accocate Memory for ReceiveBuffer Status = %lx\n",nNdisStatus));
			leave;
		}

		//Initializing the Memory
		NdisZeroMemory(pReceiveBuffer->pReceiveBuffer,nReceiveWindow);

		//Probe and Lock the Send Memory
//		pReceiveBuffer->pReceiveMdl = SFTK_TDI_AllocateAndProbeMdl(pReceiveBuffer->pReceiveBuffer,nReceiveWindow,FALSE,FALSE,NULL);

//		if(pReceiveBuffer->pReceiveMdl == NULL)
//		{
//			KdPrint(("Unable to lock the ReceiveBuffer in Momory\n"));
//			nNdisStatus = STATUS_INSUFFICIENT_RESOURCES;
//			leave;
//		}
		pReceiveBuffer->state = SFTK_BUFFER_FREE;
	}
	finally
	{
		if(!NT_SUCCESS(nNdisStatus))
		{
			if(pReceiveBuffer != NULL)
			{
				ClearReceiveBuffer(pReceiveBuffer);
				pReceiveBuffer = NULL;
			}
			lErrorCode = 1;
		}
	}
	*ppReceiveBuffer = pReceiveBuffer;
	KdPrint(("Leaving CreateNewReceiveBuffer()\n"));
	return lErrorCode;
}


PSEND_BUFFER AddBufferToSendList(PSEND_BUFFER_LIST pSendList)
{
	LONG lErrorCode = 0;
	PSEND_BUFFER pSendBuffer = NULL;

//	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	if(pSendList->NumberOfSendBuffers == pSendList->MaxNumberOfSendBuffers)
	{
		return NULL;
	}
	lErrorCode = CreateNewSendBuffer(&pSendBuffer,pSendList->SendWindow);
	if(lErrorCode == 0)
	{
		pSendBuffer->pSendList = pSendList;
		pSendList->NumberOfSendBuffers++;
		pSendBuffer->index = pSendList->NumberOfSendBuffers;
		InsertHeadList(&pSendList->FreeSessionList,&pSendBuffer->ListElement);
	}
	return pSendBuffer;
}

PRECEIVE_BUFFER AddBufferToReceiveList(PRECEIVE_BUFFER_LIST pReceiveList)
{
	LONG lErrorCode = 0;
	PRECEIVE_BUFFER pReceiveBuffer = NULL;

	KdPrint(("Enter AddBufferToReceiveList()\n"));

//	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	if(pReceiveList->NumberOfReceiveBuffers == pReceiveList->MaxNumberOfReceiveBuffers)
	{
		return NULL;
	}
	lErrorCode = CreateNewReceiveBuffer(&pReceiveBuffer,pReceiveList->ReceiveWindow);
	if(lErrorCode == 0)
	{
		pReceiveBuffer->pReceiveList = pReceiveList;
		pReceiveList->NumberOfReceiveBuffers++;
		pReceiveBuffer->index = pReceiveList->NumberOfReceiveBuffers;
		InsertHeadList(&pReceiveList->FreeSessionList,&pReceiveBuffer->ListElement);
	}
	KdPrint(("Leave AddBufferToReceiveList()\n"));
	return pReceiveBuffer;
}

PSEND_BUFFER GetNextStaleSendBuffer(PSEND_BUFFER_LIST pSendList)
{
	PSEND_BUFFER pSendBuffer = NULL;
	PLIST_ENTRY pTempEntry;

	pTempEntry = pSendList->FreeSessionList.Flink;
	while(pTempEntry !=&pSendList->FreeSessionList)
	{
		pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,ListElement);
		if(pSendBuffer->state == SFTK_BUFFER_USED)
		{
			return pSendBuffer;
		}
		pTempEntry = pTempEntry->Flink;
	}
return NULL;
}
PRECEIVE_BUFFER GetNextStaleReceiveBuffer(PRECEIVE_BUFFER_LIST pReceiveList)
{
	PRECEIVE_BUFFER pReceiveBuffer = NULL;
	PLIST_ENTRY pTempEntry;

	pTempEntry = pReceiveList->FreeSessionList.Flink;
	while(pTempEntry != &pReceiveList->FreeSessionList)
	{
		pReceiveBuffer = CONTAINING_RECORD(pTempEntry,RECEIVE_BUFFER,ListElement);
		if(pReceiveBuffer->state == SFTK_BUFFER_USED)
		{
			return pReceiveBuffer;
		}
		pTempEntry = pTempEntry->Flink;
	}
return NULL;
}

PSEND_BUFFER GetNextSendBuffer(PSEND_BUFFER_LIST pSendList)
 {
	PSEND_BUFFER pSendBuffer = NULL;
	PSEND_BUFFER pSendBuffer1 = NULL;
	PLIST_ENTRY pTempEntry;

//	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	pTempEntry = pSendList->FreeSessionList.Flink;

	while(pTempEntry != &pSendList->FreeSessionList)
	{
		pSendBuffer = CONTAINING_RECORD(pTempEntry,SEND_BUFFER,ListElement);
		if(pSendBuffer->state == SFTK_BUFFER_FREE)
		{
			pSendBuffer1 = pSendBuffer;
			break;
		}
		pTempEntry = pTempEntry->Flink;
	}

	if(pSendBuffer1 == NULL)
	{
		pSendBuffer1 = AddBufferToSendList(pSendList);
	}

	if(pSendBuffer1 != NULL)
	{
		NdisZeroMemory(pSendBuffer1,sizeof(SEND_BUFFER));
		pSendBuffer1->state = SFTK_BUFFER_INUSE;
	}
	return pSendBuffer1;
}

PRECEIVE_BUFFER GetNextReceiveBuffer(PRECEIVE_BUFFER_LIST pReceiveList)
{
	PRECEIVE_BUFFER pReceiveBuffer = NULL;
	PRECEIVE_BUFFER pReceiveBuffer1 = NULL;
	PLIST_ENTRY pTempEntry;

	KdPrint(("Enter GetNextReceiveBuffer()\n"));

//	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	pTempEntry = pReceiveList->FreeSessionList.Flink;

	while(pTempEntry != &pReceiveList->FreeSessionList)
	{
		pReceiveBuffer = CONTAINING_RECORD(pTempEntry,RECEIVE_BUFFER,ListElement);
		if(pReceiveBuffer->state == SFTK_BUFFER_FREE)
		{
			pReceiveBuffer1 = pReceiveBuffer;
			break;
		}
		pTempEntry = pTempEntry->Flink;
	}

	if(pReceiveBuffer1 == NULL)
	{
		pReceiveBuffer1 = AddBufferToReceiveList(pReceiveList);
	}

	if(pReceiveBuffer1 != NULL)
	{
		NdisZeroMemory(pReceiveBuffer1,sizeof(RECEIVE_BUFFER));
		pReceiveBuffer1->state = SFTK_BUFFER_INUSE;
	}
	KdPrint(("Leave GetNextReceiveBuffer()\n"));
	return pReceiveBuffer1;
}


LONG InitializeSession(PTCP_SESSION *ppSession, PSERVER_ELEMENT pServer, PCONNECTION_INFO pConnectionInfo,LONG nSendWindowSize, LONG nReceiveWindowSize,eSessionType type)
{
	LONG lErrorCode=0;
	NTSTATUS Status = STATUS_SUCCESS;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PTCP_SESSION pSession = NULL;
	PDEVICE_OBJECT pDeviceObject = NULL;

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
			KdPrint(("Unable to Accocate Memory for TCP_SESSION Status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pSession,sizeof(TCP_SESSION));


/////////////////////////
		//Initialize the Connection End Point
		Status = SFTK_TDI_OpenConnectionEndpoint(
						TCP_DEVICE_NAME_W,
						&pServer->KSAddress,
						&pSession->KSEndpoint,
						&pSession->KSEndpoint    // Context
						);

		if( !NT_SUCCESS( Status ) )
		{
			//
			// Connection Endpoint Could Not Be Created
			//
			KdPrint(("Open Connection EndPoint Failed for session %lx\n",pSession));
			leave;
		}

		KdPrint(("Created Local TDI Connection Endpoint\n") );

		KdPrint(("  pSession: 0x%8.8X; pAddress: 0x%8.8X; pConnection: 0x%8.8X\n",
			(ULONG )pSession,
			(ULONG )&pServer->KSAddress,
			(ULONG )&pSession->KSEndpoint
			));

		pSession->pServer = pServer;

#ifndef _USE_RECEIVE_EVENT_
		//
		// Setup Event Handlers On The Address Object
		//
		Status = SFTK_TDI_SetEventHandlers(
						&pServer->KSAddress,
						pSession,      // Event Context
						NULL,          // ConnectEventHandler
						SftkTCPSDisconnectEventHandler,
						SftkTCPCErrorEventHandler,
						//TCPC_ReceiveEventHandler,
						NULL,
						NULL,          // ReceiveDatagramEventHandler
						SftkTCPCReceiveExpeditedEventHandler
						);

		if( !NT_SUCCESS( Status ) )
		{
			KdPrint(("Failed To Set the Event Handlers\n"));
			leave;
			//
			// Event Handlers Could Not Be Set
			//
		}
#else
		//
		// Setup Event Handlers On The Address Object
		//
		Status = SFTK_TDI_SetEventHandlers(
						&pServer->KSAddress,
						pSession,      // Event Context
						NULL,          // ConnectEventHandler
						SftkTCPSDisconnectEventHandler,
						SftkTCPCErrorEventHandler,	
						SftkTCPCReceiveEventHandler,	//Receive Handler is enabled
						NULL,          // ReceiveDatagramEventHandler
						SftkTCPCReceiveExpeditedEventHandler
						);

		if( !NT_SUCCESS( Status ) )
		{
			KdPrint(("Failed To Set the Event Handlers\n"));
			leave;
			//
			// Event Handlers Could Not Be Set
			//
		}

#endif //_USE_RECEIVE_EVENT_

		KdPrint(("Set Event Handlers On The Address Object\n") );
		pDeviceObject = IoGetRelatedDeviceObject(
							pSession->KSEndpoint.m_pFileObject
							);


		if(type == CONNECT)
		{
			//
			// Setup Remote TDI Address
			//
			SFTK_TDI_InitIPAddress(
				&pSession->RemoteAddress,
				pConnectionInfo->ipRemoteAddress.in_addr,
				pConnectionInfo->ipRemoteAddress.sin_port
				);
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
				Status = STATUS_INSUFFICIENT_RESOURCES;
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
			KdPrint(("Unable to Accocate Memory for RECEIVE_BUFFER Status = %lx\n",nNdisStatus));
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
			KdPrint(("Unable to Accocate Memory for ftd_header_t Status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pSession->pReceiveHeader->pReceiveBuffer,sizeof(ftd_header_t));

		//Probe and Lock the Send Memory
//		pSession->pReceiveHeader->pReceiveMdl = SFTK_TDI_AllocateAndProbeMdl(pSession->pReceiveHeader->pReceiveBuffer,sizeof(ftd_header_t),FALSE,FALSE,NULL);

//		if(pSession->pReceiveHeader->pReceiveMdl == NULL)
//		{
//			KdPrint(("Unable to lock the ReceiveBuffer in Momory\n"));
//			nNdisStatus = STATUS_INSUFFICIENT_RESOURCES;
//			leave;
//		}
		pSession->pReceiveHeader->ActualReceiveLength =0;
		pSession->pReceiveHeader->TotalReceiveLength = sizeof(ftd_header_t);

		pSession->pReceiveHeader->state = SFTK_BUFFER_FREE;

		pSession->pReceiveHeader->pSession = pSession;



		//
		// Allocate Irp For Receiving header
		//
//		pSession->pReceiveHeader->pReceiveIrp = IoAllocateIrp(
//													pDeviceObject->StackSize,
//													FALSE
//													);

//		if( !pSession->pReceiveHeader->pReceiveIrp )
//		{
//			Status = STATUS_INSUFFICIENT_RESOURCES;
//			leave;
//		}

		//Allocating the Receive Data Buffer

		nNdisStatus = NdisAllocateMemoryWithTag(
								&pSession->pReceiveBuffer,
								sizeof(RECEIVE_BUFFER),
								MEM_TAG
								);

		if(!NT_SUCCESS(nNdisStatus))
		{
			KdPrint(("Unable to Accocate Memory for RECEIVE_BUFFER Status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pSession->pReceiveBuffer,sizeof(RECEIVE_BUFFER));

		//Allocate pSession->pReceiveBuffer->pReceiveBuffer Later When it is actually Reqd

		pSession->pReceiveBuffer->ActualReceiveLength = 0;
		pSession->pReceiveBuffer->TotalReceiveLength = nReceiveWindowSize;
		pSession->pReceiveBuffer->state = SFTK_BUFFER_FREE;
		pSession->pReceiveBuffer->pSession = pSession;


		//Initialize the receive Event, This will be trigered whenever 
		//there is data on the wire to be received
		KeInitializeEvent(&pSession->IOReceiveEvent,SynchronizationEvent,FALSE);
		pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;

		pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;

		pSession->bSessionEstablished = SFTK_DISCONNECTED;
	}
	finally
	{
		if(!NT_SUCCESS(nNdisStatus)|| !NT_SUCCESS(Status))
		{
			KdPrint(("The Memory Allocation Failed Getting out of the for Loop\n"));
			if(pSession != NULL)
			{
				CleanUpSession(pSession);
			}
			lErrorCode = 1;
			pSession = NULL;
		}
	}

	*ppSession = pSession;
	return lErrorCode;
}

NDIS_STATUS GetSessionReceiveBuffer(PTCP_SESSION pSession)
{
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;

	ASSERT(pSession);

	KdPrint(("Entering GetSessionReceiveBuffer()\n"));
	try
	{
		if(pSession->pReceiveBuffer->pReceiveBuffer == NULL)
		{
			nNdisStatus = NdisAllocateMemoryWithTag(
							&pSession->pReceiveBuffer->pReceiveBuffer,
							pSession->pReceiveBuffer->TotalReceiveLength,
							MEM_TAG
							);

			if(!NT_SUCCESS(nNdisStatus))
			{
				KdPrint(("Unable to Accocate Memory for Receive Window Status = %lx\n",nNdisStatus));
				leave;
			}

			NdisZeroMemory(pSession->pReceiveBuffer->pReceiveBuffer,pSession->pReceiveBuffer->TotalReceiveLength);
			pSession->pReceiveBuffer->index = 1;
		}
		else
		{
			NdisZeroMemory(pSession->pReceiveBuffer->pReceiveBuffer,pSession->pReceiveBuffer->TotalReceiveLength);
			pSession->pReceiveBuffer->index = 1;
		}
	}
	finally
	{
		KdPrint(("Leaving GetSessionReceiveBuffer()\n"));
	}
	return nNdisStatus;
}
void InitializeSendBufferList(PSEND_BUFFER_LIST pSendList,PCONNECTION_DETAILS pConnectionDetails)
{
	InitializeListHead(&pSendList->FreeSessionList);
	pSendList->NumberOfSendBuffers =0;
	pSendList->MaxNumberOfSendBuffers = 5;
	pSendList->SendWindow = pConnectionDetails->nSendWindowSize;
	pSendList->ChunkSize = 2000*1024;	//This is in Bytes
}
void InitializeReceiveBufferList(PRECEIVE_BUFFER_LIST pReceiveList,PCONNECTION_DETAILS pConnectionDetails)
{
	InitializeListHead(&pReceiveList->FreeSessionList);
	pReceiveList->NumberOfReceiveBuffers =0;
	pReceiveList->MaxNumberOfReceiveBuffers = 10;
	pReceiveList->ReceiveWindow = pConnectionDetails->nReceiveWindowSize;
}

LONG InitializeConnectThread(PSESSION_MANAGER pSessionManager)
{
	LONG lErrorCode=0;
	PCONNECTION_DETAILS pConnectionDetails = pSessionManager->lpConnectionDetails;
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	KIRQL oldIrql;

	try
	{
		ASSERT(pConnectionDetails);

		InitializeListHead(&pSessionManager->ServerList);

		//Added New 
		InitializeSendBufferList(&pSessionManager->sendBufferList,pSessionManager->lpConnectionDetails);
		pSessionManager->sendBufferList.pSessionManager = pSessionManager;
		InitializeReceiveBufferList(&pSessionManager->receiveBufferList,pSessionManager->lpConnectionDetails);
		pSessionManager->receiveBufferList.pSessionManager = pSessionManager;

		//Added New


		pSessionManager->nLiveSessions =0;

		//Initialize the Group uninitialize Event
		KeInitializeEvent(&pSessionManager->GroupUninitializedEvent,NotificationEvent,FALSE);
		//Initializes the Connection Event
		KeInitializeEvent(&pSessionManager->GroupConnectedEvent,NotificationEvent,FALSE);
		//Initialize the Send Receive Exit Event
		KeInitializeEvent(&pSessionManager->IOExitEvent,NotificationEvent,FALSE);

		//Initialize the Lock that is used to provide Shared Access 
		ExInitializeResourceLite(&pSessionManager->ServerListLock);

		ExInitializeNPagedLookasideList(&pSessionManager->ProtocolList,NULL,NULL,0,PROTOCOL_PACKET_SIZE,MEM_TAG,0);

		InitializeListHead(&pSessionManager->ProtocolQueue);

		KeInitializeSpinLock(&pSessionManager->ProtocolQueueLock);

		//Initialize the Send Lock
		ExInitializeFastMutex(&pSessionManager->SendLock);
		//Initialize the Receive Lock
		ExInitializeFastMutex(&pSessionManager->ReceiveLock);

		pConnectionInfo = pConnectionDetails->ConnectionDetails;

		for(i=0;i<pConnectionDetails->nConnections;i++)
		{
		
			pServer = NULL;
			lErrorCode =0;

			if(pServer == NULL)
			{
				lErrorCode = InitializeServer(&pServer,pConnectionInfo);
				if(lErrorCode==1)
				{
					//Memory Allocation Failed No Use Going Any further;
					break;
				}
				InsertTailList(&pSessionManager->ServerList,&pServer->ListElement);
				pServer->pSessionManager = pSessionManager;
			}

			ASSERT(pServer);

			lErrorCode = 0;
			pSession = NULL;

			if(pConnectionInfo->nNumberOfSessions > 1)
			{
				pConnectionInfo->nNumberOfSessions = 1;
			}

			for(j=0;j<pConnectionInfo->nNumberOfSessions;j++)
			{
				pSession = NULL;
				lErrorCode =0;

				lErrorCode = InitializeSession(&pSession,pServer,pConnectionInfo,pConnectionDetails->nSendWindowSize,pConnectionDetails->nReceiveWindowSize,CONNECT);

				if(lErrorCode == 1)
				{
					break;;
				}
				pServer->pSession = pSession;
			}

			if(lErrorCode == 1)
			{
				break;
			}
			pConnectionInfo+=1;
		}
	}
	finally
	{
	}
	pSessionManager->bInitialized = 1;
	return lErrorCode;
}


PSERVER_ELEMENT FindServerElement(PSESSION_MANAGER pSessionManager, PCONNECTION_INFO pConnectionInfo)
{
	PLIST_ENTRY pTempList;
	PSERVER_ELEMENT pTempServer = NULL;
	BOOLEAN bFound = FALSE;

	pTempList = pSessionManager->ServerList.Flink;

	while(pTempList != &pSessionManager->ServerList)
	{
		pTempServer = CONTAINING_RECORD(pTempList,SERVER_ELEMENT,ListElement);

		if((pTempServer->LocalAddress.Address->Address->in_addr == pConnectionInfo->ipLocalAddress.in_addr) &&
			(pTempServer->LocalAddress.Address->Address->sin_port == pConnectionInfo->ipLocalAddress.sin_port))
		{
			bFound = TRUE;
			break;
		}
		pTempList = pTempList->Flink;
	}
	if(bFound)
	{
		return pTempServer;
	}
	else
	{
		return NULL;
	}
}

BOOLEAN IsLGConnected(PSESSION_MANAGER pSessionManager)
{
	return ((pSessionManager->nLiveSessions > 0)?TRUE:FALSE);
}

NTSTATUS CreateSendReceiveThreads(PSESSION_MANAGER pSessionManager)
{
	NTSTATUS Status = STATUS_SUCCESS;
	HANDLE threadHandle= NULL;

	KdPrint(("Enter CreateSendReceiveThreads()\n"));
	try
	{
#define _SEND_THREAD_

#ifdef _SEND_THREAD_

		if(pSessionManager->pSendThreadObject == NULL)
		{
			ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

			//Creating the Send Thread
			Status = PsCreateSystemThread(
				&threadHandle,   // thread handle
			0L,                        // desired access
			NULL,                      // object attributes
			NULL,                      // process handle
			NULL,                      // client id
//			CreateSendThread1,           // start routine
			SendThread2,				//This thread has all the enhancements that are suggested
										//for the Send Thread
			(PVOID )pSessionManager            // start context
			);

			if(!NT_SUCCESS(Status))
			{
				KdPrint(("Unable to Create Send Thread for session manager %lx\n",pSessionManager));
				leave;
			}

			Status = ObReferenceObjectByHandle (
							threadHandle,      // Object Handle
							THREAD_ALL_ACCESS,                     // Desired Access
							NULL,                                // Object Type
							KernelMode,                          // Processor mode
							(PVOID *)&pSessionManager->pSendThreadObject,// Object pointer
							NULL);                               // Object Handle information

			if( !NT_SUCCESS(Status) )
			{
				ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
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
			ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
			//Creating the Receive Thread
			Status = PsCreateSystemThread(
				&threadHandle,   // thread handle
				0L,                        // desired access
				NULL,                      // object attributes
				NULL,                      // process handle
				NULL,                      // client id
//				CreateReceiveThread1,           // start routine
				ReceiveThread2,				//This thread has all the changes that are requested
				(PVOID )pSessionManager            // start context
				);

			if(!NT_SUCCESS(Status))
			{
				KdPrint(("Unable to Create Receive Thread for sessionmanager %lx\n",pSessionManager));
				leave;
			}

			Status = ObReferenceObjectByHandle (
							threadHandle,      // Object Handle
							THREAD_ALL_ACCESS,                     // Desired Access
							NULL,                                // Object Type
							KernelMode,                          // Processor mode
							(PVOID *)&pSessionManager->pReceiveThreadObject,// Object pointer
							NULL);                               // Object Handle information

			if( !NT_SUCCESS(Status) )
			{
				ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
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
		if(!NT_SUCCESS(Status))
		{
			StopSendReceiveThreads(pSessionManager);
		}
		if(threadHandle != NULL)
		{
			ZwClose(threadHandle);
			threadHandle = NULL;
		}
	}
	KdPrint(("Leveing CreateSendReceiveThreads()\n"));
	return Status;
}
VOID StopSendReceiveThreads(PSESSION_MANAGER pSessionManager)
{
    NTSTATUS Status = STATUS_SUCCESS;

	KdPrint(("Enter StopSendReceiveThreads()\n"));
	KeSetEvent(&pSessionManager->IOExitEvent,0,FALSE);
	if(pSessionManager->pSendThreadObject != NULL)
	{
		Status = KeWaitForSingleObject(pSessionManager->pSendThreadObject,Executive,KernelMode,FALSE,NULL);
		if(Status == STATUS_SUCCESS)
		{
			KdPrint(("The SendThread() Successfully Exited\n"));
			ObDereferenceObject(pSessionManager->pSendThreadObject);
			pSessionManager->pSendThreadObject = NULL;
		}
	}
	if(pSessionManager->pReceiveThreadObject != NULL)
	{
		Status = KeWaitForSingleObject(pSessionManager->pReceiveThreadObject,Executive,KernelMode,FALSE,NULL);
		if(Status == STATUS_SUCCESS)
		{
			KdPrint(("The ReceiveThread() Successfully Exited\n"));
			ObDereferenceObject(pSessionManager->pReceiveThreadObject);
			pSessionManager->pReceiveThreadObject = NULL;
		}
	}
	KeResetEvent(&pSessionManager->IOExitEvent);
	KdPrint(("Leaving StopSendReceiveThreads()\n"));
}
NTSTATUS CreateConnectThread(PSESSION_MANAGER pSessionManager)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PTCP_SESSION pSession = NULL;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;
	BOOLEAN bSendReceiveInitialized = FALSE;

	KdPrint(("Enter CreateConnectThread()\n"));
	try
	{
		try
		{
			InitializeConnectThread(pSessionManager);
			
			while(!bExit)
			{
				try
				{
				// Acquire the resource as shared Lock
				// This Resource will not be acquired if there is a call for Exclusive Lock

				KeEnterCriticalRegion();	//Disable the Kernel APC's
				ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

				//30 Milli-Seconds in 100*Nano Seconds
				iWait.QuadPart = -(30*10*1000);
				iZeroWait.QuadPart = 0;

				Status = KeWaitForSingleObject(&pSessionManager->GroupConnectedEvent,Executive,KernelMode,FALSE,&iZeroWait);
				if((Status == STATUS_SUCCESS) && !bSendReceiveInitialized)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					Status = CreateSendReceiveThreads(pSessionManager);
					if(NT_SUCCESS(Status))
					{
						bSendReceiveInitialized = TRUE;
					}
					KdPrint(("Enter CreateSendReceive\n"));
				}
				else if(bSendReceiveInitialized && (Status != STATUS_SUCCESS))
				{
					StopSendReceiveThreads(pSessionManager);
					bSendReceiveInitialized = FALSE;
					KdPrint(("Enter LeaveSendReceive\n"));
				}

				if(!IsListEmpty(&pSessionManager->ServerList))
				{
					pTemp = pSessionManager->ServerList.Flink;

					while(pTemp != &pSessionManager->ServerList && !bExit)
					{
						pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);

						if(pServerElement == NULL)
						{
							KdPrint(("The SERVER_ELEMENT is Null and hence cannot Proceed\n"));
							Status = STATUS_MEMORY_NOT_ALLOCATED;
							break;
				//			goto End;
						}
						
						ASSERT(pServerElement);

						if(pServerElement->pSession != NULL && 
							pServerElement->pSession->bSessionEstablished == SFTK_DISCONNECTED)
						{

							ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
							Status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
							if(Status == STATUS_SUCCESS)
							{
								KdPrint(("The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
								bExit = TRUE;
								break;
							}

							pSession = pServerElement->pSession;

							if(pSession == NULL)
							{
								KdPrint(("The TCP_SESSION is Null and hence cannot Proceed"));
							}

							ASSERT(pSession);

							try
							{
								pSession->bSessionEstablished = SFTK_INPROGRESS;
								Status = SFTK_TDI_NewConnect(&pSession->KSEndpoint, 
												(PTRANSPORT_ADDRESS )&pSession->RemoteAddress,
												SftkTCPCConnectedCallback,
												pSession
												);
							}
							except(EXCEPTION_EXECUTE_HANDLER)
							{
								KdPrint(("An Exception Occured in CreateConnectThread() Error Code is %lx\n",GetExceptionCode()));
								Status = GetExceptionCode();
								pSession->bSessionEstablished = SFTK_DISCONNECTED;
							}

						}	//while(!IsListEmpty(pServerElement->FreeSessionList))
						pTemp = pTemp->Flink;
					}	//while(!IsListEmpty(lpListIndex))
				}	//if(!IsListEmpty(&pDeviceExtension->pSessionManager->ServerList))

				if(!bExit)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					Status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
					if(Status == STATUS_SUCCESS)
					{
						KdPrint(("The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
						bExit = TRUE;
						break;
					}
				}
				}
				finally
				{
					//Release the Resource 
					ExReleaseResourceLite(&pSessionManager->ServerListLock);
					KeLeaveCriticalRegion();	//Enable the Kernel APC's
				}
			}	//while(!bExit)
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint(("An Exception Occured in CreateConnectThread() Error Code is %lx\n",GetExceptionCode()));
			Status = GetExceptionCode();
		}
	}
	finally
	{
		KdPrint(("Exiting the CreateConnectThread() Status = %lx\n",Status));
	}

	return PsTerminateSystemThread(Status);
}

//Registering Handlers for Plug and Play for the Network Address and Binding Info.
//Only works in Win2000 and above.
#if _WIN32_WINNT >= 0x0500
NTSTATUS RegisterPnpHandlers()
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
//The Status of the Operation is returned.
//If no Connections meet the specified requirements then we return STATUS_INVALID_PARAMETER
//This is Purely internal, and any caller who is calling this function has to get the
//Exclusive Lock Before Calling thsi function

NTSTATUS FindConnectionsCount(IN PSESSION_MANAGER pSessionManager ,IN PCONNECTION_DETAILS pConnectionDetails , IN BOOLEAN bEnabled, OUT PULONG pServerCount, OUT PLIST_ENTRY pServerList)
{
	NTSTATUS lErrorCode=STATUS_SUCCESS;
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	PLIST_ENTRY pTempEntry1;
	ULONG nServers = 0;

	ASSERT(pSessionManager);
	KdPrint(("Entering FindConnections()\n"));

	if(pConnectionDetails == NULL)
	{
		KdPrint(("The Connection Details Parameter is NULL\n"));
		return STATUS_INVALID_PARAMETER;
	}
	if(!pSessionManager->bInitialized)
	{
		KdPrint(("The SESSION_MANAGER is not initialized so cannot Allocate connections\n"));
		return STATUS_INVALID_PARAMETER;
	}
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	try
	{
		*pServerCount = 0;

		pTempEntry1 = pSessionManager->ServerList.Flink;
		while(pTempEntry1 != &pSessionManager->ServerList)
		{
			pConnectionInfo = pConnectionDetails->ConnectionDetails;
			pServer = CONTAINING_RECORD(pTempEntry1,SERVER_ELEMENT,ListElement);
			ASSERT(pServer);
			pSession = pServer->pSession;
			ASSERT(pSession);

			for(i=0;i<pConnectionDetails->nConnections;i++)
			{
				//Find the Session That Meets all the Criteria Specified
				if((pServer->LocalAddress.Address[0].Address[0].in_addr == pConnectionInfo->ipLocalAddress.in_addr) &&
					(pServer->LocalAddress.Address[0].Address[0].sin_port == pConnectionInfo->ipLocalAddress.sin_port) &&
					(pSession->RemoteAddress.Address[0].Address[0].in_addr == pConnectionInfo->ipRemoteAddress.in_addr) &&
					(pSession->RemoteAddress.Address[0].Address[0].sin_port == pConnectionInfo->ipRemoteAddress.sin_port) &&
					(pServer->bIsEnabled == bEnabled))
				{
					//Increment the Server Count
					InsertTailList(pServerList,&pServer->ServerListElement);
					nServers++;
					break;
				}//if
				pConnectionInfo+=1;
			}//for
			//Goto the next Element
			pTempEntry1 = pTempEntry1->Flink;
		}//while

		*pServerCount = nServers;
		//If we cannot find the number of servers that meet our criteria
		//We return Error
		if(nServers != pConnectionDetails->nConnections)
		{
			KdPrint(("The Number of Connections %d didnt match the Number of Servers %d\n",
				pConnectionDetails->nConnections,
				nServers));
			lErrorCode = STATUS_INVALID_PARAMETER;
			leave;
		}
	}//try
	finally
	{
	}
	KdPrint(("Leaving FindConnections()"));
	return lErrorCode;
}

NTSTATUS AddConnections(PSESSION_MANAGER pSessionManager ,PCONNECTION_DETAILS pConnectionDetails)
{
	NTSTATUS lErrorCode=STATUS_SUCCESS;
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	LIST_ENTRY tempServerList;
	PLIST_ENTRY pTempEntry1;


	ASSERT(pSessionManager);

	KdPrint(("Entering AllocateConnections()\n"));

	if(pConnectionDetails == NULL)
	{
		KdPrint(("The Connection Details Parameter is NULL\n"));
		return STATUS_INVALID_PARAMETER;
	}
	if(!pSessionManager->bInitialized)
	{
		KdPrint(("The SESSION_MANAGER is not initialized so cannot Allocate connections\n"));
		return STATUS_INVALID_PARAMETER;
	}

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

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
				lErrorCode = AllocateServer(&pServer,pConnectionInfo);
				if(!NT_SUCCESS(lErrorCode))
				{
					KdPrint(("Unable to Allocate Server\n"));
					leave;
				}

				InsertTailList(&tempServerList,&pServer->ListElement);
//				InsertTailList(&pSessionManager->ServerList,&pServer->ListElement);
				pServer->pSessionManager = pSessionManager;
			}

			ASSERT(pServer);

			pSession = NULL;

			//Let only one session be there per connection
			if(pConnectionInfo->nNumberOfSessions > 1)
			{
				pConnectionInfo->nNumberOfSessions = 1;
			}
			for(j=0;j<pConnectionInfo->nNumberOfSessions;j++)
			{
				pSession = NULL;

				lErrorCode = AllocateSession(&pSession,pServer,pConnectionInfo);

				if(!NT_SUCCESS(lErrorCode))
				{
					KdPrint(("Unable to Allocate the Session Pointer\n"));
					leave;
				}
				pServer->pSession = pSession;
			}
			pConnectionInfo+=1;
		}

		//Check if the new Server List is Empty or not
		if(tempServerList.Flink == &tempServerList)
		{
			KdPrint(("There are no Elements in the Server List So Leaving\n"));
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

		//Release the Resource 
		ExReleaseResourceLite(&pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	//Enable the Kernel APC's

	}
	finally
	{
		if(!NT_SUCCESS(lErrorCode))
		{
			FreeConnections(&tempServerList);
		}
	}
	KdPrint(("Leaving AllocateConnections()"));
	return lErrorCode;
}

//This function will enable OR disable sessions based on the Connection Details sent by the 
//User
NTSTATUS EnableConnections(PSESSION_MANAGER pSessionManager ,PCONNECTION_DETAILS pConnectionDetails, BOOLEAN bEnable)
{
	NTSTATUS lErrorCode=STATUS_SUCCESS;
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	LIST_ENTRY serverList;
	PLIST_ENTRY pTempEntry1;
	ULONG nServers =0;


	ASSERT(pSessionManager);

	KdPrint(("Entering AllocateConnections()\n"));

	if(pConnectionDetails == NULL)
	{
		KdPrint(("The Connection Details Parameter is NULL\n"));
		return STATUS_INVALID_PARAMETER;
	}

	if(!pSessionManager->bInitialized)
	{
		KdPrint(("The SESSION_MANAGER is not initialized so cannot Allocate connections\n"));
		return STATUS_INVALID_PARAMETER;
	}

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	InitializeListHead(&serverList);

	try
	{
		// Acquire the resource as shared Lock
		// This Resource will not be acquired if there is a call for Exclusive Lock
		KeEnterCriticalRegion();	//Disable the Kernel APC's
		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		//Find all the Servers that are not yet initialized
		lErrorCode = FindConnectionsCount(pSessionManager,pConnectionDetails,!bEnable,&nServers,&serverList);
		if(!NT_SUCCESS(lErrorCode))
		{
			KdPrint(("Cannot find the Required Servers so leaving\n"));
			leave;
		}
		pTempEntry1 = serverList.Flink;
		while(pTempEntry1 != &serverList)
		{
			//Get the Server Item and set the Enable to TRUE|FALSE
			pServer = CONTAINING_RECORD(pTempEntry1,SERVER_ELEMENT,ServerListElement);
			ASSERT(pServer);
			//Enable the Entry This will cause the Connect thread to Initialize the Server and
			//Connect to the Remote Side or Listen on this Server
			pServer->bEnable = bEnable;
			pTempEntry1 = pTempEntry1->Flink;
		}
	}
	finally
	{
		//Release the Resource 
		ExReleaseResourceLite(&pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	//Enable the Kernel APC's
	}
	KdPrint(("Leaving AllocateConnections()"));
	return lErrorCode;
}

NTSTATUS RemoveConnections(PSESSION_MANAGER pSessionManager ,PCONNECTION_DETAILS pConnectionDetails)
{
	NTSTATUS lErrorCode=STATUS_SUCCESS;
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	LIST_ENTRY serverList;
	PLIST_ENTRY pTempEntry1;
	ULONG nServers =0;

	ASSERT(pSessionManager);

	KdPrint(("Entering AllocateConnections()\n"));

	if(pConnectionDetails == NULL)
	{
		KdPrint(("The Connection Details Parameter is NULL\n"));
		return STATUS_INVALID_PARAMETER;
	}
	if(!pSessionManager->bInitialized)
	{
		KdPrint(("The SESSION_MANAGER is not initialized so cannot Allocate connections\n"));
		return STATUS_INVALID_PARAMETER;
	}

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	InitializeListHead(&serverList);

	try
	{
		// Acquire the resource as shared Lock
		// This Resource will not be acquired if there is a call for Exclusive Lock
		KeEnterCriticalRegion();	//Disable the Kernel APC's
		ExAcquireResourceExclusiveLite(&pSessionManager->ServerListLock,TRUE);

		//Find all the Servers that are not yet initialized
		lErrorCode = FindConnectionsCount(pSessionManager,pConnectionDetails,FALSE,&nServers,&serverList);
		if(!NT_SUCCESS(lErrorCode))
		{
			KdPrint(("Cannot find the Required Servers so leaving\n"));
			leave;
		}//if

		//Loop through the ServerList That is Disabled and remove the SERVER_ELEMENT
		//Elements from the Server List in the SESSION_MANAGER
		pTempEntry1 = serverList.Flink;
		while(pTempEntry1 != &serverList)
		{
			//Get the Server Item and set the Enable to TRUE|FALSE
			pServer = CONTAINING_RECORD(pTempEntry1,SERVER_ELEMENT,ServerListElement);
			ASSERT(pServer);
			//Check if all the Cleanup happened correctly
			ASSERT(pServer->pSession->bSessionEstablished == SFTK_UNINITIALIZED);
			//Get the Next Link Before you delete the
			//existing one
			pTempEntry1 = pTempEntry1->Flink;
			//Remove this element from the ServerList.
			RemoveEntryList(&pServer->ListElement);
			FreeServer(pServer);
			pServer = NULL;
		}//while
	}//try
	finally
	{
		//Release the Resource 
		ExReleaseResourceLite(&pSessionManager->ServerListLock);
		KeLeaveCriticalRegion();	//Enable the Kernel APC's
	}//finally
	KdPrint(("Leaving AllocateConnections()"));
	return lErrorCode;
}

//This function Frees the Sessions from SESSION_MANAGER
VOID FreeConnections(PLIST_ENTRY pServerList)
{
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	PLIST_ENTRY pTempEntry;

	KdPrint(("Entering FreeConnections()\n"));

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	try
	{
		pTempEntry = pServerList->Flink;

		while(!IsListEmpty(pServerList))
		{
			pTempEntry = RemoveHeadList(pServerList);
			pServer = CONTAINING_RECORD(pTempEntry,SERVER_ELEMENT,ListElement);
			ASSERT(pServer);
			FreeServer(pServer);
			pServer = NULL;
		}
	}
	finally
	{
	}
	KdPrint(("Leaving AllocateConnections()"));
}
NTSTATUS AllocateServer(PSERVER_ELEMENT *ppServer,PCONNECTION_INFO pConnectionInfo)
{

	NTSTATUS lErrorCode = STATUS_SUCCESS;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PSERVER_ELEMENT pServer = NULL;

	ASSERT(ppServer);
	KdPrint(("Entering AllocateServer()\n"));
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
			KdPrint(("Unable to Accocate Memory for SERVER_ELEMENT Status = %lx\n",nNdisStatus));
			leave;
		}
		NdisZeroMemory(pServer,sizeof(SERVER_ELEMENT));
		SFTK_TDI_InitIPAddress(&pServer->LocalAddress,pConnectionInfo->ipLocalAddress.in_addr,pConnectionInfo->ipLocalAddress.sin_port);
	}
	finally
	{
		if(!NT_SUCCESS(nNdisStatus))
		{
			KdPrint(("The Memory Allocation \n"));
			FreeServer(pServer);
			pServer =NULL;
			lErrorCode = 1;
		}
	}
	*ppServer = pServer;

	KdPrint(("Leaving AllocateServer()"));
	return lErrorCode;
}

//Free up the Server and the Session Associated with it
VOID FreeServer(PSERVER_ELEMENT pServer)
{
	if(pServer != NULL)
	{
		if(pServer->pSession != NULL)
		{
			FreeSession(pServer->pSession);
			pServer->pSession = NULL;
		}
		NdisFreeMemory(pServer,sizeof(SERVER_ELEMENT),0);
	}
}

//Free up the Allocated Session Memory
VOID FreeSession(PTCP_SESSION pSession)
{
	if(pSession != NULL)
	{
		NdisFreeMemory(pSession,sizeof(TCP_SESSION),0);
	}
}

NTSTATUS AllocateSession(PTCP_SESSION *ppSession, PSERVER_ELEMENT pServer, PCONNECTION_INFO pConnectionInfo)
{
	LONG lErrorCode=STATUS_SUCCESS;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PTCP_SESSION pSession = NULL;

	ASSERT(ppSession);
	ASSERT(pServer);

	KdPrint(("Enetring AllocateSession()\n"));
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
			KdPrint(("Unable to Accocate Memory for TCP_SESSION Status = %lx\n",nNdisStatus));
			leave;
		}
		NdisZeroMemory(pSession,sizeof(TCP_SESSION));

		SFTK_TDI_InitIPAddress(
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
	}
	finally
	{
		if(!NT_SUCCESS(nNdisStatus))
		{
			KdPrint(("The Memory Allocation Failed for TCP_SESSION in AllocateSession()\n"));
			FreeSession(pSession);
			lErrorCode = 1;
			pSession = NULL;
		}
	}

	*ppSession = pSession;

	KdPrint(("Leaving AllocateSession()\n"));
	return lErrorCode;
}

LONG InitializeSession1(PTCP_SESSION pSession, eSessionType type)
{
	LONG lErrorCode=0;
	NTSTATUS Status = STATUS_SUCCESS;
	NDIS_STATUS nNdisStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT pDeviceObject = NULL;

	try
	{
		ASSERT(pSession);
		ASSERT(pSession->pServer);

		//Initialize the Connection End Point
		Status = SFTK_TDI_OpenConnectionEndpoint(
						TCP_DEVICE_NAME_W,
						&pSession->pServer->KSAddress,
						&pSession->KSEndpoint,
						&pSession->KSEndpoint    // Context
						);

		if( !NT_SUCCESS( Status ) )
		{
			//
			// Connection Endpoint Could Not Be Created
			//
			KdPrint(("Open Connection EndPoint Failed for session %lx\n",pSession));
			leave;
		}

		KdPrint(("Created Local TDI Connection Endpoint\n") );
		KdPrint(("  pSession: 0x%8.8X; pAddress: 0x%8.8X; pConnection: 0x%8.8X\n",
			(ULONG )pSession,
			(ULONG )&pSession->pServer->KSAddress,
			(ULONG )&pSession->KSEndpoint
			));

#ifndef _USE_RECEIVE_EVENT_
		//
		// Setup Event Handlers On The Address Object
		//
		Status = SFTK_TDI_SetEventHandlers(
						&pSession->pServer->KSAddress,
						pSession,      // Event Context
						NULL,          // ConnectEventHandler
						SftkTCPSDisconnectEventHandler,
						SftkTCPCErrorEventHandler,
						//TCPC_ReceiveEventHandler,
						NULL,
						NULL,          // ReceiveDatagramEventHandler
						SftkTCPCReceiveExpeditedEventHandler
						);

		if( !NT_SUCCESS( Status ) )
		{
			KdPrint(("Failed To Set the Event Handlers\n"));
			leave;
			//
			// Event Handlers Could Not Be Set
			//
		}
#else
		//
		// Setup Event Handlers On The Address Object
		//
		Status = SFTK_TDI_SetEventHandlers(
						&pSession->pServer->KSAddress,
						pSession,      // Event Context
						NULL,          // ConnectEventHandler
						SftkTCPSDisconnectEventHandler,
						SftkTCPCErrorEventHandler,	
						SftkTCPCReceiveEventHandler,	//Receive Handler is enabled
						NULL,          // ReceiveDatagramEventHandler
						SftkTCPCReceiveExpeditedEventHandler
						);

		if( !NT_SUCCESS( Status ) )
		{
			KdPrint(("Failed To Set the Event Handlers\n"));
			leave;
			//
			// Event Handlers Could Not Be Set
			//
		}

#endif //_USE_RECEIVE_EVENT_

		KdPrint(("Set Event Handlers On The Address Object\n") );
		pDeviceObject = IoGetRelatedDeviceObject(
							pSession->KSEndpoint.m_pFileObject
							);


		if(type == CONNECT)
		{
			KdPrint(("The CONNECT Initialzation is already Done\n"));
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
				Status = STATUS_INSUFFICIENT_RESOURCES;
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
			KdPrint(("Unable to Accocate Memory for RECEIVE_BUFFER Status = %lx\n",nNdisStatus));
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
			KdPrint(("Unable to Accocate Memory for ftd_header_t Status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pSession->pReceiveHeader->pReceiveBuffer,sizeof(ftd_header_t));

		pSession->pReceiveHeader->ActualReceiveLength =0;
		pSession->pReceiveHeader->TotalReceiveLength = sizeof(ftd_header_t);
		pSession->pReceiveHeader->state = SFTK_BUFFER_FREE;
		pSession->pReceiveHeader->pSession = pSession;

		//Allocating the Receive Data Buffer

		nNdisStatus = NdisAllocateMemoryWithTag(
								&pSession->pReceiveBuffer,
								sizeof(RECEIVE_BUFFER),
								MEM_TAG
								);

		if(!NT_SUCCESS(nNdisStatus))
		{
			KdPrint(("Unable to Accocate Memory for RECEIVE_BUFFER Status = %lx\n",nNdisStatus));
			leave;
		}

		NdisZeroMemory(pSession->pReceiveBuffer,sizeof(RECEIVE_BUFFER));

		//Allocate pSession->pReceiveBuffer->pReceiveBuffer Later When it is actually Reqd

		pSession->pReceiveBuffer->ActualReceiveLength = 0;
		pSession->pReceiveBuffer->TotalReceiveLength = pSession->pServer->pSessionManager->nReceiveWindowSize;
		pSession->pReceiveBuffer->state = SFTK_BUFFER_FREE;
		pSession->pReceiveBuffer->pSession = pSession;

		//Initialize the receive Event, This will be trigered whenever 
		//there is data on the wire to be received
		KeInitializeEvent(&pSession->IOReceiveEvent,SynchronizationEvent,FALSE);
		pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
		pSession->bSendStatus = SFTK_PROCESSING_SEND_NOTINIT;
		pSession->bSessionEstablished = SFTK_DISCONNECTED;
	}//try
	finally
	{
		if(!NT_SUCCESS(nNdisStatus)|| !NT_SUCCESS(Status))
		{
			KdPrint(("The Memory Allocation for function InitializeSession1() So Cleaning up\n"));

			if(pSession != NULL)
			{
				if(pSession->pListenIrp != NULL)
				{
					IoFreeIrp( pSession->pListenIrp );
					pSession->pListenIrp = NULL;
				}//if
				if(pSession->pReceiveHeader != NULL)
				{
					ClearReceiveBuffer(pSession->pReceiveHeader);
					pSession->pReceiveHeader = NULL;
				}//if
				if(pSession->pReceiveBuffer != NULL)
				{
					ClearReceiveBuffer(pSession->pReceiveBuffer);
					pSession->pReceiveBuffer = NULL;
				}//if
				pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
				pSession->bSessionEstablished = SFTK_UNINITIALIZED;

				SFTK_TDI_CloseConnectionEndpoint( &pSession->KSEndpoint );
			}//if
		}//if
	}//finally
	//Return the Error Status
	return nNdisStatus | Status;
}

NTSTATUS InitializeServer1(PSERVER_ELEMENT pServer, eSessionType type)
{
	NTSTATUS Status = STATUS_SUCCESS;

	try
	{
		//Check if the IPAddress and the Port information is initialized or not
		ASSERT(pServer->LocalAddress.TAAddressCount > 0); 
		//
		// Open Transport Address
		//
		Status = SFTK_TDI_OpenTransportAddress(
				TCP_DEVICE_NAME_W,
				(PTRANSPORT_ADDRESS )&pServer->LocalAddress,
				&pServer->KSAddress
				);

		if( !NT_SUCCESS( Status ) )
		{
			KdPrint(("Transport Address Couldnt be opened  for IPAddr %d and Port %d So Quitting\n",pServer->LocalAddress.Address[0].Address[0].in_addr, pServer->LocalAddress.Address[0].Address[0].sin_port));
			//
			// Address Object Could Not Be Created
			//
			leave;
		}
		Status = InitializeSession1(pServer->pSession, type);
	}
	finally
	{
		if(!NT_SUCCESS(Status))
		{
			KdPrint(("The OpenTransportAddress() Failed\n"));
			if(pServer != NULL)
			{
				SFTK_TDI_CloseTransportAddress( &pServer->KSAddress);
			}
		}
	}
	return Status;
}


void UninitializeSession1(PTCP_SESSION pSession)
{
	LONG lErrorCode =0;
	NTSTATUS Status = STATUS_SUCCESS;
	LARGE_INTEGER DelayTime;

	try
	{

		KdPrint(("pSession->bSessionEstablished for session %lx\n",pSession));
		if(pSession->bSessionEstablished == SFTK_CONNECTED)
		{
			KdPrint(("Before calling the SFTK_TDI_Disconnect for session %lx\n",pSession));
			Status = SFTK_TDI_Disconnect(
				&pSession->KSEndpoint,
				NULL,    // UserCompletionEvent
				NULL,    // UserCompletionRoutine
				NULL,    // UserCompletionContext
				NULL,    // pIoStatusBlock
				0        // Disconnect Flags
				);
		}

		KdPrint(( "Disconnect Status: 0x%8.8x for Session %d\n", Status, pSession->sessionID ));

		KdPrint(( "Waiting After Disconnect...\n" ));

		DelayTime.QuadPart = 10*1000*50;   // 50 MilliSeconds

		KeDelayExecutionThread( KernelMode, FALSE, &DelayTime );

//		pSession->bSessionEstablished = SFTK_DISCONNECTED;
		SFTK_TDI_CloseConnectionEndpoint( &pSession->KSEndpoint );

		if(pSession->pListenIrp != NULL)
		{
			IoFreeIrp( pSession->pListenIrp );
			pSession->pListenIrp = NULL;
		}//if
		if(pSession->pReceiveHeader != NULL)
		{
			ClearReceiveBuffer(pSession->pReceiveHeader);
			pSession->pReceiveHeader = NULL;
		}//if
		if(pSession->pReceiveBuffer != NULL)
		{
			ClearReceiveBuffer(pSession->pReceiveBuffer);
			pSession->pReceiveBuffer = NULL;
		}//if
		pSession->bReceiveStatus = SFTK_PROCESSING_RECEIVE_NOTINIT;
		pSession->bSessionEstablished = SFTK_UNINITIALIZED;
	}//try
	finally
	{

	}//finally
}

void UninitializeServer1(PSERVER_ELEMENT pServer)
{
	LONG lErrorCode =0;
	NTSTATUS Status = STATUS_SUCCESS;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;

	ASSERT(pServer);

	if(pServer->pSession != NULL)
	{
		KdPrint(("Calling pSession %lx\n",pServer->pSession));
		UninitializeSession1(pServer->pSession);
	}
	SFTK_TDI_CloseTransportAddress( &pServer->KSAddress);
}

//This function Just Initializes the Session Manager
LONG InitializeSessionManager(PSESSION_MANAGER pSessionManager)
{
	LONG lErrorCode=0;

	ASSERT(pSessionManager);

	KdPrint(("Entering InitializeConnectThread1()\n"));

	if(pSessionManager->bInitialized)
	{
		KdPrint(("Session Manager already initialized\n"));
		return lErrorCode;
	}

	try
	{
		NdisZeroMemory(pSessionManager,sizeof(SESSION_MANAGER));
		InitializeListHead(&pSessionManager->ServerList);

		InitializeSendBufferList(&pSessionManager->sendBufferList,pSessionManager->lpConnectionDetails);
		pSessionManager->sendBufferList.pSessionManager = pSessionManager;
		InitializeReceiveBufferList(&pSessionManager->receiveBufferList,pSessionManager->lpConnectionDetails);
		pSessionManager->receiveBufferList.pSessionManager = pSessionManager;

		pSessionManager->nLiveSessions =0;

		//Initialize the Group uninitialize Event
		KeInitializeEvent(&pSessionManager->GroupUninitializedEvent,NotificationEvent,FALSE);
		//Initializes the Connection Event
		KeInitializeEvent(&pSessionManager->GroupConnectedEvent,NotificationEvent,FALSE);
		//Initialize the Send Receive Exit Event
		KeInitializeEvent(&pSessionManager->IOExitEvent,NotificationEvent,FALSE);

		//Initialize the Lock that is used to provide Shared Access 
		ExInitializeResourceLite(&pSessionManager->ServerListLock);

		ExInitializeNPagedLookasideList(&pSessionManager->ProtocolList,NULL,NULL,0,PROTOCOL_PACKET_SIZE,MEM_TAG,0);

		InitializeListHead(&pSessionManager->ProtocolQueue);

		KeInitializeSpinLock(&pSessionManager->ProtocolQueueLock);

		//Initialize the Send Lock
		ExInitializeFastMutex(&pSessionManager->SendLock);
		//Initialize the Receive Lock
		ExInitializeFastMutex(&pSessionManager->ReceiveLock);

	}
	finally
	{
	}
	pSessionManager->bInitialized = 1;
	KdPrint(("Leaving InitializeConnectThread1()\n"));
	return lErrorCode;
}

NTSTATUS ConnectThread2(PSESSION_MANAGER pSessionManager)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PTCP_SESSION pSession = NULL;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;
	BOOLEAN bSendReceiveInitialized = FALSE;

	KdPrint(("Enter CreateConnectThread()\n"));
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

				//30 Milli-Seconds in 100*Nano Seconds
				iWait.QuadPart = -(30*10*1000);
				iZeroWait.QuadPart = 0;

				Status = KeWaitForSingleObject(&pSessionManager->GroupConnectedEvent,Executive,KernelMode,FALSE,&iZeroWait);
				if((Status == STATUS_SUCCESS) && !bSendReceiveInitialized)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					Status = CreateSendReceiveThreads(pSessionManager);
					if(NT_SUCCESS(Status))
					{
						bSendReceiveInitialized = TRUE;
					}//if
					KdPrint(("Enter CreateSendReceive\n"));
				}//if
				else if(bSendReceiveInitialized && (Status != STATUS_SUCCESS))
				{
					StopSendReceiveThreads(pSessionManager);
					bSendReceiveInitialized = FALSE;
					KdPrint(("Enter LeaveSendReceive\n"));
				}//else if
				pTemp = pSessionManager->ServerList.Flink;

				while(pTemp != &pSessionManager->ServerList && !bExit)
				{
					pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);

					//Check if the Server Element and the Session Element are not NULL
					ASSERT(pServerElement);
					ASSERT(pServerElement->pSession);

					//ENABLE THE SESSION
					//Check if the Server Element is Initialized or not
					if((pServerElement->pSession->bSessionEstablished == SFTK_UNINITIALIZED) &&
						pServerElement->bEnable)
					{
						KdPrint(("The Session is not Enabled and not Initialized\n"));
						KdPrint(("This is the First Time it is called\n"));

						Status = InitializeServer1(pServerElement,CONNECT);
						if(!NT_SUCCESS(Status))
						{
							KdPrint(("InitializeServer1() Failed So Just setting to UNINITIALIZED\n"));
							//Initialization of Server Element Failed and hence just set the
							//Session as Disconnected and then set the Server as disabled
							pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
							pServerElement->bEnable = FALSE;
							pServerElement->bIsEnabled = FALSE;
						}//if
						else
						{
							//Set the Session Status to DISCONNECTED and the Enabled to TRUE
							//So that we can go ahead and establish connection.
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
							pServerElement->bIsEnabled = TRUE;
						}//else
					}//if

					//DISABLE THE SERVER_ELEMENT
					//Check to see if they want the Sessions to be Disabled
					if((pServerElement->pSession->bSessionEstablished != SFTK_UNINITIALIZED) &&
						!pServerElement->bEnable &&
						pServerElement->bIsEnabled == TRUE)
					{
						KdPrint(("We got the Command to Disable the Server %lx\n",pServerElement));

						UninitializeServer1(pServerElement);
						pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
						pServerElement->bEnable = FALSE;
						pServerElement->bIsEnabled = FALSE;
					}//if


					//PROCESS the NORMAL CONNECTIONS
					//Check if the Session Needs to be established
					if((pServerElement->pSession != NULL) && 
						(pServerElement->pSession->bSessionEstablished == SFTK_DISCONNECTED) &&
						pServerElement->bIsEnabled)
					{

						ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
						Status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
						if(Status == STATUS_SUCCESS)
						{
							KdPrint(("The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}

						pSession = pServerElement->pSession;

						try
						{
							pSession->bSessionEstablished = SFTK_INPROGRESS;
							Status = SFTK_TDI_NewConnect(&pSession->KSEndpoint, 
											(PTRANSPORT_ADDRESS )&pSession->RemoteAddress,
											SftkTCPCConnectedCallback,
											pSession
											);
						}//try
						except(EXCEPTION_EXECUTE_HANDLER)
						{
							KdPrint(("An Exception Occured in CreateConnectThread() Error Code is %lx\n",GetExceptionCode()));
							Status = GetExceptionCode();
							pSession->bSessionEstablished = SFTK_DISCONNECTED;
						}//except

					}	//while(!IsListEmpty(pServerElement->FreeSessionList))
					pTemp = pTemp->Flink;
				}	//while(!IsListEmpty(lpListIndex))

				if(!bExit)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					Status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
					if(Status == STATUS_SUCCESS)
					{
						KdPrint(("The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
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
		KdPrint(("Exiting the CreateConnectThread() Status = %lx\n",Status));
	}//finally

	return PsTerminateSystemThread(Status);
}

#endif	//__NEW_SEND__