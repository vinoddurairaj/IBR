/**************************************************************************************

Module Name: sftk_protocol.c   
Author Name: Veera Arja , Saumya Tripathy 
Description: Describes Modules: Protocol Functions
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#include "sftk_main.h"

#define MAX_PROTOCOL_STRING_LENEGTH		40

/* peer <-> peer protocol */
CHAR gProtocolStrings[][MAX_PROTOCOL_STRING_LENEGTH] = {

	"FTDCHANDSHAKE",
	"FTDCCHKCONFIG",
	"FTDCNOOP",
	"FTDCVERSION",
	"FTDCCHUNK",
	"FTDCHUP",
	"FTDCCPONERR",
	"FTDCCPOFFERR",
	"FTDCEXIT",
	"FTDCBFBLK",
	"FTDCBFSTART",
	"FTDCBFREAD",
	"FTDCBFEND",
	"FTDCCHKSUM",
	"FTDCRSYNCDEVS",
	"FTDCRSYNCDEVE",
	"FTDCRFBLK",
	"FTDCRFEND",
	"FTDCRFFEND",
	"FTDCKILL",
	"FTDCRFSTART",
	"FTDCRFFSTART",
	"FTDCCPSTARTP",
	"FTDCCPSTARTS",
	"FTDCCPSTOPP",
	"FTDCCPSTOPS",
	"FTDCCPON",
	"FTDCCPOFF",
	"FTDCSTARTAPPLY",
	"FTDCSTOPAPPLY",
	"FTDCSTARTPMD",
	"FTDCAPPLYDONECPON",
	"FTDCREFOFLOW",
	"FTDCSIGNALPMD",
	"FTDCSIGNALRMD",
	"FTDCSTARTRECO",

	/* acks */
	"FTDACKERR" ,
	"FTDACKRSYNC",
	"FTDACKCHKSUM",
	"FTDACKCHUNK",
	"FTDACKHUP",
	"FTDACKCPSTART",
	"FTDACKCPSTOP",
	"FTDACKCPON",
	"FTDACKCPOFF",
	"FTDACKCPOFFERR",
	"FTDACKCPONERR",
	"FTDACKNOOP",
	"FTDACKCONFIG",
	"FTDACKKILL" ,
	"FTDACKHANDSHAKE",
	"FTDACKVERSION",
	"FTDACKCLI",
	"FTDACKRFSTART",
	"FTDACKNOPMD",
	"FTDACKNORMD",
	"FTDACKDOCPON",
	"FTDACKDOCPOFF",
	"FTDACKCLD",
	"FTDACKCPERR",

	/* signals */
	"FTDCSIGUSR1",
	"FTDCSIGTERM",
	"FTDCSIGPIPE",

	/* management msg type */
	"FTDCMANAGEMENT",

	/* new debug level msg */
	"FTDCSETTRACELEVEL",
	"FTDCSETTRACELEVELACK",

	/* REMOTE WAKEUP msg */
	"FTDCREMWAKEUP",

	/* REMOTE DRIVE ERROR */
	"FTDCREMDRIVEERR",

	/* SMART REFRESH RELAUNCH FROM RMD,RMDA */
	"FTDCREFRSRELAUNCH",

	//This is for Sentinal

	//These will be used to set as commands for the Sentinals.
	//Care will be taken so that they are inserted into the Data Pool
	//Only after the Refresh and Migration Queue's are empty.
	"FTDSENTINAL",
	"FTDMSGINCO",
	"FTDMSGCO",
	"FTDMSGAVOIDJOURNALS",
	"FTDMSGCPON",
	"FTDMSGCPOFF",

	"FTDRECEIVEERROR"
};

LONG gMaxProtocolCommands = sizeof(gProtocolStrings)/MAX_PROTOCOL_STRING_LENEGTH;


CHAR* 
PROTO_GetProtocolString(
					LONG protocol
					)
{
	if(protocol <= 0 || protocol > gMaxProtocolCommands)
	{
		DebugPrint((DBG_PROTO, "PROTO_GetProtocolString(): OutOfBounds for the protocol Command %d\n",protocol));
		return "Invalid Protocol";
	}
	return gProtocolStrings[protocol-1];

}// PROTO_GetProtocolString()

/*
 * PROTO_PrepareProtocolSentinal - 
 *
 */
NTSTATUS 
PROTO_PrepareProtocolSentinal(	IN enum protocol	eProtocol, 
								IN OUT ftd_header_t *protocolHeader,
								IN OUT wlheader_t	*softHeader,
								IN PVOID			RawBuffer,			// Optional, for Payload, Copy this to new allocated buffer
								IN ULONG			RawBufferSize,
								IN ULONGLONG		Offset,
								IN ULONG			Size,
								IN LONG				Lg_Number,
								IN LONG				Dev_Number)	// optional : Size of RetBuffer
{
	NTSTATUS status = STATUS_SUCCESS;

	DebugPrint((DBG_PROTO, "PROTO_PrepareProtocolSentinal(): Enter PROTO_PrepareProtocolSentinal()\n"));

	
	switch(eProtocol)
	{
	case FTDMSGINCO:
	case FTDMSGCO:
	case FTDMSGCPON:
	case FTDMSGCPOFF:
	case FTDMSGAVOIDJOURNALS:

		OS_ASSERT(RawBuffer != NULL);
		OS_ASSERT(RawBufferSize == 512);

		if(eProtocol == FTDMSGINCO)
			OS_RtlCopyMemory(RawBuffer, MSG_INCO, sizeof(MSG_INCO));

		if(eProtocol == FTDMSGCO)
			OS_RtlCopyMemory(RawBuffer, MSG_CO, sizeof(MSG_CO));

		if(eProtocol == FTDMSGCPON)
			OS_RtlCopyMemory(RawBuffer, MSG_CPON, sizeof(MSG_CPON));

		if(eProtocol == FTDMSGCPOFF)
			OS_RtlCopyMemory(RawBuffer, MSG_CPOFF,sizeof(MSG_CPOFF));

		if(eProtocol == FTDMSGAVOIDJOURNALS)
			OS_RtlCopyMemory(RawBuffer, MSG_AVOID_JOURNALS, sizeof(MSG_AVOID_JOURNALS));

		// Initialize the Protocol Header
		protocolHeader->magicvalue		= MAGICHDR;
		protocolHeader->msgtype			= FTDCCHUNK;
		protocolHeader->msg.lg.devid	= 0;
		protocolHeader->ts				= 0; //lgp->ts;	//This is UnKnown
		protocolHeader->ackwanted		= 1;
		protocolHeader->compress		= 0;	//for Now Compression is Disabled
		protocolHeader->uncomplen		= sizeof(wlheader_t) + RawBufferSize;	//This is length in Bytes
		protocolHeader->len				= protocolHeader->msg.lg.len = sizeof(wlheader_t) + RawBufferSize;

		// Initialize the Soft Header for the Sentinals:
		softHeader->majicnum	= DATASTAR_MAJIC_NUM;
		softHeader->offset		= -1;
		softHeader->length		= RawBufferSize >> DEV_BSHIFT;	
							//The Length that is passed is in Bytes
		softHeader->dev			= -1;
		softHeader->diskdev		= -1;
		softHeader->group_ptr	= NULL; //lgp;	//For now this Unknown
		softHeader->complete	= 1;
		softHeader->flags		= 0;
		softHeader->bp			= 0;
		break;

	case FTDCRFFEND:
		protocolHeader->magicvalue		= MAGICHDR;
		protocolHeader->msgtype			= FTDCRFFEND;
		protocolHeader->msg.lg.devid	= Lg_Number;
		protocolHeader->ackwanted		= 0;
		break;

	case FTDCHUP:
		protocolHeader->magicvalue		= MAGICHDR;
		protocolHeader->msgtype			= FTDCHUP;
		protocolHeader->ackwanted		= 1;
		break;

	case FTDACKHUP:
		protocolHeader->msgtype			= FTDACKHUP;
		protocolHeader->msg.lg.devid	= Dev_Number;
		protocolHeader->msg.lg.data		= FTDACKHUP;
		break;

	case FTDCRFFSTART:
		protocolHeader->msgtype			= FTDCRFFSTART;
		break;

	case FTDACKRFSTART:
		protocolHeader->msgtype			= FTDACKRFSTART;
		break;

	case FTDCCHKSUM:
		// TODO:: Better to call as a sperate API, Because it has Payload
		break;
	case FTDACKCHKSUM:
		// TODO:: Better to call as a sperate API, Because it has Payload
		break;

	case FTDACKCPSTART:
		protocolHeader->msgtype			= FTDACKCPSTART;
		break;

	case FTDACKCPSTOP:
		protocolHeader->msgtype			= FTDACKCPSTOP;
		break;

	case FTDCCPONERR:
		protocolHeader->msgtype			= FTDCCPONERR;
		break;

	case FTDCCPOFFERR:
		protocolHeader->msgtype			= FTDCCPOFFERR;
		break;

	case FTDCNOOP:
		protocolHeader->msgtype			= FTDCNOOP;
		break;
	
	case FTDCRFBLK:
		protocolHeader->magicvalue		= MAGICHDR;
		protocolHeader->msgtype			= FTDCRFBLK;
		protocolHeader->msg.lg.devid	= Dev_Number;
		protocolHeader->msg.lg.offset	= (int) Get_Values_InSectorsFromBytes(Offset); // Offset >> DEV_BSHIFT;	//In Blocks
		protocolHeader->msg.lg.len		= (int) Get_Values_InSectorsFromBytes(Size); // Offset >> DEV_BSHIFT;	//In Blocks

		protocolHeader->ackwanted		= 1;
		protocolHeader->compress		= 0;	//for Now Compression is Disabled
		protocolHeader->uncomplen		= Size;	//This is length in Bytes
		protocolHeader->len				= 
		protocolHeader->msg.lg.len		= (int) Get_Values_InSectorsFromBytes(Size); // Size >> DEV_BSHIFT;

		break;

	case FTDCBFBLK:

		// TODO:: Has to add Some Initialization
		break;

	case FTDCCHUNK:
		protocolHeader->magicvalue		= MAGICHDR;
		protocolHeader->msgtype			= FTDCCHUNK;
		protocolHeader->msg.lg.devid	= 0;
		protocolHeader->ts				= 0; //lgp->ts;	//This is UnKnown
		protocolHeader->ackwanted		= 1;
		protocolHeader->compress		= 0;	//for Now Compression is Disabled
		protocolHeader->uncomplen		= Size;	//This is length in Bytes
		protocolHeader->len				= protocolHeader->msg.lg.len = Size;
		break;

	default:

		DebugPrint((DBG_PROTO, "PROTO_PrepareProtocolSentinal(): Default %d FIXME FIXME\n", eProtocol));
		OS_ASSERT(FALSE);
		status = STATUS_UNSUCCESSFUL;
		break;
	}

	DebugPrint((DBG_PROTO, "PROTO_PrepareProtocolSentinal(): Leave PROTO_PrepareProtocolSentinal()\n"));
	return status;

} /* PROTO_PrepareProtocolSentinal */ 

/*
 * PROTO_PrepareSentinal - 
 *
 */
NTSTATUS 
PROTO_PrepareSentinal( IN enum sentinals eSentinal, 
					 IN OUT wlheader_t *softHeader, 
					 IN rplcc_iorp_t   *inputData,
					 OUT PVOID outputData)

/**/
{
	NTSTATUS status = STATUS_SUCCESS;

	DebugPrint((DBG_PROTO, "PROTO_PrepareSentinal(): Enter PROTO_PrepareSentinal()\n"));

	switch(eSentinal)
	{
	case MSGINCO:
	case MSGCO:
	case MSGCPON:
	case MSGCPOFF:
	case MSGAVOIDJOURNALS:
		softHeader->majicnum	= DATASTAR_MAJIC_NUM;
		softHeader->offset		= -1;
		softHeader->length		= inputData->Size >> DEV_BSHIFT;	
		//The Length that is passed is in Bytes
		softHeader->dev			= -1;
		softHeader->diskdev		= -1;
		softHeader->group_ptr	= NULL; //lgp;	//For now this Unknown
		softHeader->complete	= 1;
		softHeader->flags		= 0;
		softHeader->bp			= 0;
		//These are to be decided
//		lgp->wlentries++;
//		lgp->wlsectors += softHeader->length;
		//These are to be decided
		break;
	default:
		status = STATUS_UNSUCCESSFUL;
		break;
	}

	if(!NT_SUCCESS(status))
	{
		return status;
	}

	switch(eSentinal)
	{
	case MSGINCO:
		RtlCopyMemory(inputData->pBuffer, MSG_INCO, sizeof(MSG_INCO));
		break;
	case MSGCO:
		RtlCopyMemory(inputData->pBuffer, MSG_CO, sizeof(MSG_CO));
		break;
	case MSGCPON:
		RtlCopyMemory(inputData->pBuffer, MSG_CPON, sizeof(MSG_CPON));
		break;
	case MSGCPOFF:
		RtlCopyMemory(inputData->pBuffer, MSG_CPOFF,sizeof(MSG_CPOFF));
		break;
	case MSGAVOIDJOURNALS:
		RtlCopyMemory(inputData->pBuffer, MSG_AVOID_JOURNALS, sizeof(MSG_AVOID_JOURNALS));
		break;
	default:
		status = STATUS_UNSUCCESSFUL;
		break;
	}

	DebugPrint((DBG_PROTO, "PROTO_PrepareSentinal(): Leave PROTO_PrepareSentinal()\n"));
	return status;

} /* PROTO_PrepareSentinal */ 

/*
 * PROTO_PrepareProtocolHeader - 
 *
 */
NTSTATUS 
PROTO_PrepareProtocolHeader( IN enum protocol     eProtocol,
 					       IN OUT ftd_header_t  *protocolHeader,
						   IN rplcc_iorp_t      *inputData,
						   OUT PVOID            outputData,
						   LONG                 ackwanted )

/**/
{
	NTSTATUS status = STATUS_SUCCESS;

	DebugPrint((DBG_PROTO, "PROTO_PrepareProtocolHeader(): Enter PROTO_PrepareProtocolHeader()\n"));

	switch(eProtocol)
	{
	case FTDCRFBLK:
		protocolHeader->magicvalue = MAGICHDR;
		protocolHeader->msgtype    = FTDCRFBLK;
		protocolHeader->msg.lg.devid  = inputData->DeviceId;
		protocolHeader->msg.lg.offset = (int)inputData->Blk_Start;	//In Blocks
		protocolHeader->msg.lg.len    = inputData->Size;	    //In Blocks

		protocolHeader->ackwanted = ackwanted;
		protocolHeader->compress = 0;	//for Now Compression is Disabled
		protocolHeader->uncomplen = inputData->Size<< DEV_BSHIFT;	//This is length in Bytes
		protocolHeader->len = protocolHeader->msg.lg.len = inputData->Size;

		break;
	case FTDCBFBLK:
		break;
	case FTDCCHUNK:
	case FTDSENTINAL:
		protocolHeader->magicvalue = MAGICHDR;
		protocolHeader->msgtype = FTDCCHUNK;
		protocolHeader->msg.lg.devid = 0;
		protocolHeader->ts = 0; //lgp->ts;	//This is UnKnown
		protocolHeader->ackwanted = ackwanted;
		protocolHeader->compress = 0;	//for Now Compression is Disabled
		protocolHeader->uncomplen = inputData->Size;	//This is length in Bytes
		protocolHeader->len = protocolHeader->msg.lg.len = inputData->Size;

		break;
	default:
		status = STATUS_UNSUCCESSFUL;
		break;
	}

	DebugPrint((DBG_PROTO, "PROTO_PrepareProtocolHeader(): Leave PROTO_PrepareProtocolHeader()\n"));

return status;
} /* PROTO_PrepareProtocolHeader */

/*
 * PROTO_PrepareProtocolHeader - 
 *
 */
NTSTATUS
PROTO_PrepareSoftHeader(IN PVOID           *p_data,
					  IN IN rplcc_iorp_t *p_iorequest)

/**/
{
	NTSTATUS ret = STATUS_SUCCESS;
	wlheader_t *hp = NULL;

	DebugPrint((DBG_PROTO, "PROTO_PrepareSoftHeader(): Entering PROTO_PrepareSoftHeader\n"));

	hp = (wlheader_t *)p_data;
	hp->complete = 0;
	hp->bp = NULL;
	hp->majicnum = DATASTAR_MAJIC_NUM;

	/*
	* Original Code 
	* hp->offset = (u_int)(PtrCurrentStackLocation->Parameters.Write.ByteOffset.QuadPart >> DEV_BSHIFT);
	* hp->length = PtrCurrentStackLocation->Parameters.Write.Length >> DEV_BSHIFT;
	* hp->group_ptr = lgp
	*/
	hp->offset   = (int)p_iorequest->Blk_Start >> DEV_BSHIFT;
	hp->length   = p_iorequest->Size      >> DEV_BSHIFT;
	hp->diskdev  = p_iorequest->DeviceId;  //softp->bdev;
	hp->dev      = p_iorequest->DeviceId;  //softp->bdev;

	hp->flags = 0;

	DebugPrint((DBG_PROTO, "PROTO_PrepareSoftHeader(): Leaving PROTO_PrepareSoftHeader\n"));

return(ret);
} /* PROTO_PrepareSoftHeader */


NTSTATUS 
PROTO_Perform_Handshake(
					IN PSESSION_MANAGER pSessionManager
					)
{
	NTSTATUS status = STATUS_SUCCESS;
	CHAR strConfigFileName[10];
	PCHAR strSecondaryVersion = NULL;
	LARGE_INTEGER iWait;

	CHAR strConfigFilePath[512];
	CHAR strLocalHostName[32];
	ULONG ulLocalIP		= 0;
	int CP				= 0;

	CHAR strRemoteDeviceName[512];
	ULONG ulRemoteDevSize	= 0;
	ULONG ulDevId			= 0;
	KEVENT syncEvent;
	PLIST_ENTRY	listEntry1;
	PSFTK_DEV	pSftkDev;

	OS_ASSERT(pSessionManager != NULL);
	OS_ASSERT(pSessionManager->pLogicalGroupPtr != NULL);

	try
	{
		//FTD_NET_ACK_RECEIVE_TIMEOUT = 30 Seconds
		iWait.QuadPart = -(FTD_NET_ACK_RECEIVE_TIMEOUT*10*1000*1000);
		// Initialize the Wait Time for Receiving the ACK
		KeInitializeEvent(&syncEvent,SynchronizationEvent,FALSE);

		// Set the Sync Event so that we will get signalled when we got an ack back
		pSessionManager->pSyncReceivedEvent = &syncEvent;

		sprintf(strConfigFileName, STRING_CONFIG_FILENAME,	// "%s%03d.%s"
						SECONDARY_CFG_PREFIX,				// "s"
						pSessionManager->pLogicalGroupPtr->LGroupNumber,	
						PATH_CFG_SUFFIX);	// "cfg"

		status = PROTO_Send_VERSION(pSessionManager->pLogicalGroupPtr->LGroupNumber,
									strConfigFileName,
									&strSecondaryVersion,
									pSessionManager,
									0);

		if(!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Error in sending the VERSION\n"));
//			InterlockedExchangePointer(&pSessionManager->pSyncReceivedEvent, NULL);
			leave;
		}

		// Wait for the ACK
		status = KeWaitForSingleObject(&syncEvent,Executive,KernelMode,FALSE,&iWait);

		if(status != STATUS_SUCCESS)
		{
			DebugPrint((DBG_PROTO, "PROTO_Perform_Handshake(): Error in Receiving VERSION ACK\n"));
			// Wait failed or timeout so we should just remove the event
//			InterlockedExchangePointer(&pSessionManager->pSyncReceivedEvent, NULL);
			leave;
		}

		if( pSessionManager->ReceivedProtocolHeader.msgtype != PROTO_GetAckOfCommand(FTDCVERSION) )
		{
			DebugPrint( ( DBG_ERROR , "PROTO_Perform_Handshake(): Invalid ACK %s received for FTDCVERSION \n",PROTO_GetProtocolString(pSessionManager->ReceivedProtocolHeader.msgtype) ) );

			if(pSessionManager->pOutputBuffer != NULL)
			{
				DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Received the Output Buffer Information\n"));
				NdisFreeMemory(pSessionManager->pOutputBuffer,pSessionManager->OutputBufferLenget,0);
				pSessionManager->OutputBufferLenget = 0;
				pSessionManager->pOutputBuffer = NULL;
			}
			status = STATUS_INVALID_NETWORK_RESPONSE;
			leave;
		}
		else
		{
			DebugPrint( ( DBG_PROTO , "PROTO_Perform_Handshake(): Received Proper ACK %s for FTDCVERSION \n",PROTO_GetProtocolString(pSessionManager->ReceivedProtocolHeader.msgtype) ) );

			if(pSessionManager->pOutputBuffer != NULL)
			{
				DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Received the Output Buffer Information\n"));
				NdisFreeMemory(pSessionManager->pOutputBuffer,pSessionManager->OutputBufferLenget,0);
				pSessionManager->OutputBufferLenget = 0;
				pSessionManager->pOutputBuffer = NULL;
			}
		}
		// Do the processing after you successfully received the ACK

		// Set the Sync Event so that we will get signalled when we got an ack back
		pSessionManager->pSyncReceivedEvent = &syncEvent;

		sprintf(strConfigFilePath , "%s","Hi, How are you Baby");
		
		// TODO, atleast one Socket Information should be present, for this Local Host Name
		OS_ASSERT(pSessionManager->ServerList.Flink != &pSessionManager->ServerList);
		ulLocalIP = CONTAINING_RECORD(	pSessionManager->ServerList.Flink , 
										SERVER_ELEMENT,
										ListElement)->LocalAddress.Address[0].Address[0].in_addr;

		SM_CONVERT_IP_TO_STRING( strLocalHostName , ulLocalIP );

		status = PROTO_Send_HANDSHAKE(0, // pSessionManager->pLogicalGroupPtr->flags
									GSftk_Config.HostId,
									strConfigFilePath,
									strLocalHostName,
									ulLocalIP,
									&CP,
									pSessionManager,
									0);

		if(!NT_SUCCESS(status))
		{
			DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Error in sending the HANDSHAKE\n"));
//			InterlockedExchangePointer(&pSessionManager->pSyncReceivedEvent, NULL);
			leave;
		}

		// Wait for the ACK
		status = KeWaitForSingleObject(&syncEvent,Executive,KernelMode,FALSE,&iWait);

		if(status != STATUS_SUCCESS)
		{
			DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Error in Receiving HANDSHAKE ACK\n"));
			// Wait failed or timeout so we should just remove the event
//			InterlockedExchangePointer(&pSessionManager->pSyncReceivedEvent, NULL);
			leave;
		}

		if( pSessionManager->ReceivedProtocolHeader.msgtype != PROTO_GetAckOfCommand(FTDCHANDSHAKE) )
		{
			DebugPrint( ( DBG_ERROR , "PROTO_Perform_Handshake(): Invalid ACK %s received for HANDSHAKE \n",PROTO_GetProtocolString(pSessionManager->ReceivedProtocolHeader.msgtype) ) );

			if(pSessionManager->pOutputBuffer != NULL)
			{
				DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Received the Output Buffer Information\n"));
				NdisFreeMemory(pSessionManager->pOutputBuffer,pSessionManager->OutputBufferLenget,0);
				pSessionManager->OutputBufferLenget = 0;
				pSessionManager->pOutputBuffer = NULL;
			}
			status = STATUS_INVALID_NETWORK_RESPONSE;
			leave;
		}
		else
		{
			DebugPrint( ( DBG_PROTO , "PROTO_Perform_Handshake(): Received Proper ACK %s for HANDSHAKE \n",PROTO_GetProtocolString(pSessionManager->ReceivedProtocolHeader.msgtype) ) );

			if(pSessionManager->pOutputBuffer != NULL)
			{
				DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Received the Output Buffer Information\n"));
				NdisFreeMemory(pSessionManager->pOutputBuffer,pSessionManager->OutputBufferLenget,0);
				pSessionManager->OutputBufferLenget = 0;
				pSessionManager->pOutputBuffer = NULL;
			}
		}


		// Do the processing after you successfully received the ACK

		try
		{

			OS_ACQUIRE_LOCK(	&pSessionManager->pLogicalGroupPtr->Lock, 
								OS_ACQUIRE_SHARED, TRUE, NULL);

			for(listEntry1 = pSessionManager->pLogicalGroupPtr->LgDev_List.ListEntry.Flink; 
				listEntry1 != &pSessionManager->pLogicalGroupPtr->LgDev_List.ListEntry;
				listEntry1 = listEntry1->Flink)
			{ // for : scan each and every Devices under current LG existing entries
				pSftkDev = CONTAINING_RECORD( listEntry1, SFTK_DEV, LgDev_Link);

				// Set the Sync Event so that we will get signalled when we got an ack back
				pSessionManager->pSyncReceivedEvent = &syncEvent;


				OS_ASSERT(pSftkDev != NULL);
				OS_ASSERT(pSftkDev->NodeId.NodeType == NODE_TYPE_SFTK_DEV);

				status =  PROTO_Send_CHKCONFIG(pSftkDev->cdev,
											pSftkDev->cdev,
											pSftkDev->cdev,
											pSftkDev->strRemoteDeviceName,
											&ulRemoteDevSize,
											&ulDevId,
											pSessionManager,
											0);

				if(!NT_SUCCESS(status))
				{
					DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Error in sending the CHKCONFIG\n"));
//					InterlockedExchangePointer(&pSessionManager->pSyncReceivedEvent, NULL);
					leave;
				}

				// Wait for the Ack
				status = KeWaitForSingleObject(&syncEvent,Executive,KernelMode,FALSE,&iWait);

				if(status != STATUS_SUCCESS)
				{
					DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Error in Receiving CHKCONFIG ACK\n"));
					// Wait failed or timeout so we should just remove the event
//					InterlockedExchangePointer(&pSessionManager->pSyncReceivedEvent, NULL);
					leave;
				}

				if( pSessionManager->ReceivedProtocolHeader.msgtype != PROTO_GetAckOfCommand(FTDCCHKCONFIG) )
				{
					DebugPrint( ( DBG_ERROR , "PROTO_Perform_Handshake(): Invalid ACK %s received for FTDCCHKCONFIG \n",PROTO_GetProtocolString(pSessionManager->ReceivedProtocolHeader.msgtype) ) );

					if(pSessionManager->pOutputBuffer != NULL)
					{
						DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Received the Output Buffer Information\n"));
						NdisFreeMemory(pSessionManager->pOutputBuffer,pSessionManager->OutputBufferLenget,0);
						pSessionManager->OutputBufferLenget = 0;
						pSessionManager->pOutputBuffer = NULL;
					}
					status = STATUS_INVALID_NETWORK_RESPONSE;
					leave;
				}
				else
				{
					DebugPrint( ( DBG_PROTO , "PROTO_Perform_Handshake(): Received Proper ACK %s for FTDCCHKCONFIG \n",PROTO_GetProtocolString(pSessionManager->ReceivedProtocolHeader.msgtype) ) );

					if(pSessionManager->pOutputBuffer != NULL)
					{
						DebugPrint((DBG_ERROR, "PROTO_Perform_Handshake(): Received the Output Buffer Information\n"));
						NdisFreeMemory(pSessionManager->pOutputBuffer,pSessionManager->OutputBufferLenget,0);
						pSessionManager->OutputBufferLenget = 0;
						pSessionManager->pOutputBuffer = NULL;
					}
				}
				// Do the processing after you successfully received the ACK
			}// Loop the Device List and send the CHKCONFIG for each device
		}// try for the send config loop
		finally
		{
			// Release the lock
			OS_RELEASE_LOCK( &pSessionManager->pLogicalGroupPtr->Lock, NULL);
		}
		
	}
	finally
	{
		InterlockedExchangePointer(&pSessionManager->pSyncReceivedEvent, NULL);
	}

	return status;
}


//PROTOCOL SEND and RECEIVE Commands
// These are the commands to send and Receive the different Protocol Commands


// sends FTDCHUP command over to the other server
NTSTATUS PROTO_Send_HUP( IN PSESSION_MANAGER pSessionManger , IN LONG AckWanted)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));

	header.magicvalue = MAGICHDR;
	header.msgtype = FTDCHUP;
	header.ackwanted = 1;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_HUP(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}

// sends FTDACKHUP command over to the other server
NTSTATUS PROTO_Send_ACKHUP( IN int iDeviceID, IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));

	ack.msgtype = FTDACKHUP;
	ack.msg.lg.devid = iDeviceID;
	ack.msg.lg.data = FTDACKHUP;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_ACKHUP(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}

// sends FTDCRFFSTART command over to the secondary server
NTSTATUS PROTO_Send_RFFSTART( IN PSESSION_MANAGER pSessionManger , IN LONG AckWanted)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));
	header.msgtype = FTDCRFFSTART;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_RFFSTART(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}

// sends FTDACKRFSTART command over to the PRIMARY server
NTSTATUS PROTO_Send_ACKRFSTART( IN PSESSION_MANAGER pSessionManger , IN LONG AckWanted)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));
	ack.msgtype = FTDACKRFSTART;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_ACKRFSTART(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}


// sends FTDCRFFEND command over to the secondary server
NTSTATUS PROTO_Send_RFFEND(IN int iLgnum, IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));

	header.msgtype = FTDCRFFSTART;
	header.msg.lg.devid = iLgnum;
    header.ackwanted = 0;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_RFFEND(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}
 
// sends FTDCCHKSUM command over to the secondary server
NTSTATUS PROTO_Send_CHKSUM( IN  int iLgnumIN,
					  IN  int iDeviceId, 
					  IN  ftd_dev_t* ptrDeviceStructure,
					  OUT PVOID* ptrDeltaMap,
					  IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[3];
	int             iovcnt, rc, digestlen;
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));

	header.magicvalue = MAGICHDR;
    header.msgtype = FTDCCHKSUM;
    header.ackwanted = 1;
    header.len = 1;
	header.msg.lg.lgnum = iLgnumIN;

	if (ptrDeviceStructure->sumlen == 0) {
            ptrDeviceStructure->sumoff = 0;
			ptrDeviceStructure->sumlen = 0;
			return status;
       }

    digestlen = ptrDeviceStructure->sumnum * DIGESTSIZE;
    header.msg.lg.devid = ptrDeviceStructure->devid;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);
	iovec[1].pBuffer = ptrDeviceStructure;
    iovec[1].uLength = sizeof(ftd_dev_t);
	iovec[2].pBuffer = ptrDeviceStructure->sumbuf;
    iovec[2].uLength = digestlen;

	ptrDeviceStructure->statp->actual += sizeof(ftd_dev_t) + digestlen;
    ptrDeviceStructure->statp->effective += sizeof(ftd_dev_t) + digestlen;

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 3, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_CHKSUM(): COM_SendBufferVector failed"));
		return status;
	}

	if( !NT_SUCCESS( status = PROTO_RecvVector_CheckSum( ptrDeltaMap, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_CHKSUM(): PROTO_RecvVector_CheckSum failed"));
		return status;
	}

	return status;
}

// sends FTDACKCHKSUM command over to the primary server
NTSTATUS PROTO_Send_ACKCHKSUM(IN  ftd_dev_t* ptrDeviceStructure,
						IN	int iDeltamap_len,
						IN	PVOID ptrDeltaMap,
						IN	PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[3];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));

    ack.msgtype = FTDACKCHKSUM;
    ack.msg.lg.devid = ptrDeviceStructure->devid;
    ack.msg.lg.len = (ptrDeviceStructure->sumlen >> DEV_BSHIFT);
    ack.len = iDeltamap_len;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);
	iovec[1].pBuffer = ptrDeviceStructure;
    iovec[1].uLength = sizeof(ftd_dev_t);
	iovec[2].pBuffer = ptrDeltaMap;
    iovec[2].uLength = iDeltamap_len;

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_ACKCHKSUM(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}

// sends FTDACKCPSTART command over to the other server
NTSTATUS PROTO_Send_ACKCPSTART(IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));

	ack.msgtype = FTDACKCPSTART;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_ACKCPSTART(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}

// sends FTDACKCPSTOP command over to the other server
NTSTATUS PROTO_Send_ACKCPSTOP(IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));

	ack.msgtype = FTDACKCPSTOP;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_ACKCPSTOP(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}

// sends FTDCCPONERR command over to the other server
NTSTATUS PROTO_Send_CPONERR(IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));
	header.msgtype = FTDCCPONERR;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "Send_CPONERR(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}

// sends FTDCCPOFFERR command over to the other server
NTSTATUS PROTO_Send_CPOFFERR(IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));
	header.msgtype = FTDCCPOFFERR;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "Send_CPOFFERR(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}

// Is sent as part of Handshake mechanism when starting the PMD. 
// If RMD is not alive it is spawned. sends FTDCNOOP command over to the other server
// AckWanted will be either 1 or 0
NTSTATUS PROTO_Send_NOOP(IN PSESSION_MANAGER pSessionManger, IN LONG LGnum , IN LONG AckWanted)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));
	header.msgtype = FTDCNOOP;
	header.ackwanted = AckWanted;
	header.msg.lg.lgnum = LGnum;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_NOOP(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}


// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns the Remote Version String. sends FTDCVERSION command over to the other server
NTSTATUS PROTO_Send_VERSION(IN int iLgnumIN,
					  IN PCHAR strConfigFile,
					  OUT PCHAR* strSecondaryVersion,
					  IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	header;
	ftd_version_t	version;
	SFTK_IO_VECTOR	iovec[2];
	LARGE_INTEGER   CurrentTime;
	NTSTATUS		status = STATUS_SUCCESS;
	ANSI_STRING		configFileName;

	memset(&header, 0, sizeof(header));
	header.magicvalue = MAGICHDR;
    header.msgtype = FTDCVERSION;
    header.ackwanted = 1;
    header.msg.lg.lgnum = iLgnumIN;

	KeQuerySystemTime( &CurrentTime );
	header.ts = (ULONG)(CurrentTime.QuadPart/(10*1000*1000));
	version.pmdts = header.ts;

	OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	RtlInitAnsiString(&configFileName, strConfigFile);
	OS_RtlCopyMemory(version.configpath , configFileName.Buffer , configFileName.Length);
	version.configpath[configFileName.Length] = '\0';

//	sprintf(version.configpath, "%s%03d.%s\n",
//        SECONDARY_CFG_PREFIX, iLgnumIN, PATH_CFG_SUFFIX);

	OS_RtlCopyMemory(version.version,VERSIONSTR , sizeof(VERSIONSTR));
	version.version[sizeof(VERSIONSTR)] = '\0';

	/* set default protocol version numbers */
//    strcpy(versionstr, VERSIONSTR);// hard code it here
//    strcpy(version.version, versionstr);

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);
	iovec[1].pBuffer = &version;
    iovec[1].uLength = sizeof(ftd_version_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 2, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_VERSION(): COM_SendBufferVector failed"));
		return status;
	}

	if( !NT_SUCCESS( status = PROTO_RecvVector_Version( strSecondaryVersion, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_VERSION(): PROTO_RecvVector_Version failed"));
		return status;
	}

	return status;
}

// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns STATUS_SUCCESS indicating whether the Secondary is in CP mode or not.
NTSTATUS PROTO_Send_HANDSHAKE(IN unsigned int nFlags,
					  IN ULONG ulHostID,
					  IN CONST PCHAR strConfigFilePath,
					  IN CONST PCHAR strLocalHostName,
					  IN ULONG ulLocalIP,
					  OUT int* CP,
					  IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	header;
	ftd_auth_t		auth;
	SFTK_IO_VECTOR	iovec[2];
	LARGE_INTEGER   CurrentTime;
	ULONG			ts;
	NTSTATUS		status = STATUS_SUCCESS;

	KeQuerySystemTime( &CurrentTime );
	ts = (ULONG)(CurrentTime.QuadPart/(10*1000*1000));

	memset(&auth, 0, sizeof(auth));

	ftd_sock_encode_auth(ts, strLocalHostName,
        ulHostID, ulLocalIP, &auth.len, auth.auth);

	strcpy(auth.configpath, strConfigFilePath);

	memset(&header, 0, sizeof(header));

	header.magicvalue = MAGICHDR;
    header.msgtype = FTDCHANDSHAKE;
	header.ts = ts;
    header.ackwanted = 1;
    header.msg.lg.flags = nFlags;
    header.msg.lg.data = ulHostID;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);
	iovec[1].pBuffer = &auth;
    iovec[1].uLength = sizeof(ftd_auth_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 2, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_HANDSHAKE(): COM_SendBufferVector failed"));
		return status;
	}

	if( !NT_SUCCESS( status = PROTO_RecvVector_Handshake( CP, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_HANDSHAKE(): PROTO_RecvVector_Handshake failed"));
		return status;
	}

	return status;
}

// This ack is prepared and sent from the RMD in response to FTDCHANDSHAKE command
NTSTATUS PROTO_Send_ACKHANDSHAKE(IN unsigned int nFlags, 
						   IN int iDeviceID, 
						   IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));
	ack.msgtype = FTDACKHANDSHAKE;

    if (GET_LG_CPON(nFlags)) {
        // tell primary that we are in checkpoint mode 
        SET_LG_CPON(ack.msg.lg.flags);
    }

    ack.msg.lg.devid = iDeviceID;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_ACKHANDSHAKE(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}

// Is sent as part of Handshake mechanism when starting the PMD. 
// For each device in the group, one command is sent. If the remote side can not find the device 
// specified, an error is returned.
NTSTATUS PROTO_Send_CHKCONFIG(IN int iDeviceID,
					  IN dev_t ulDevNum,
					  IN dev_t ulFtdNum,
					  IN CONST PCHAR strRemoteDeviceName,
					  OUT PULONG ulRemoteDevSize,
					  OUT PULONG ulDevId,
					  IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	header;
	ftd_rdev_t		rdev;
	SFTK_IO_VECTOR	iovec[2];
	LARGE_INTEGER  CurrentTime;
	NTSTATUS		status = STATUS_SUCCESS;
	ANSI_STRING		remDriveLetter;

	memset(&header, 0, sizeof(header));

	// send mirror device name to the remote system for verification 
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCCHKCONFIG;
	KeQuerySystemTime( &CurrentTime );
	header.ts = (ULONG)(CurrentTime.QuadPart/(10*1000*1000));
//	header.ts = CurrentTime.LowPart;
    header.ackwanted = 1;
    header.msg.lg.devid = iDeviceID;

    rdev.devid = iDeviceID;
    rdev.minor = ulDevNum;
    rdev.ftd = ulFtdNum;

	RtlInitAnsiString(&remDriveLetter , strRemoteDeviceName);

	OS_RtlCopyMemory(rdev.path, remDriveLetter.Buffer,remDriveLetter.Length);
	rdev.path[remDriveLetter.Length] = '\0';
	rdev.len = remDriveLetter.Length;

//    strcpy(rdev.path, strRemoteDeviceName);
//    rdev.len = strlen(strRemoteDeviceName);

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);
	iovec[1].pBuffer = &rdev;
    iovec[1].uLength = sizeof(ftd_rdev_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 2, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_CHKCONFIG(): COM_SendBufferVector failed"));
		return status;
	}

	if( !NT_SUCCESS( status = PROTO_RecvVector_CheckConfig( ulRemoteDevSize, ulDevId, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_CHKCONFIG(): PROTO_RecvVector_CheckConfig failed"));
		return status;
	}

	return status;
}

// This ack is prepared and sent from the RMD in response to FTDCCHKCONFIG command
NTSTATUS PROTO_Send_ACKCONFIG(IN int iSize, 
						IN int iDeviceID, 
						IN PSESSION_MANAGER pSessionManger, IN LONG AckWanted)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));

	/* return mirror volume size for a configuration query */
    ack.msgtype = FTDACKCONFIG;
    ack.msg.lg.data = iSize;
    ack.msg.lg.devid = iDeviceID;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = COM_SendBufferVector( iovec, 1, pSessionManger ) ))
	{
		DebugPrint((DBG_ERROR, "PROTO_Send_ACKCONFIG(): COM_SendBufferVector failed"));
		return status;
	}

	return status;
}

static void
ftd_sock_encode_auth(ULONG ts, PCHAR hostname, ULONG hostid, ULONG ip,
            int *encodelen, PCHAR encode)
{
    encodeunion *kp, key1, key2;
    int         i, j;
    u_char      k, t;

    key1.ul = ((u_long) ts) ^ ((u_long) hostid);
    key2.ul = ((u_long) ts) ^ ((u_long) ip);
  
    i = j = 0;
    while (i < (int)strlen(hostname)) {
        kp = ((i%8) > 3) ? &key1 : &key2;
        k = kp->uc[i%4];
        t = (u_char) (0x000000ff & ((u_long)k ^ (u_long) hostname[i++]));
        sprintf (&(encode[j]), "%02x", t);
        j += 2;
    }
    encode[j] = '\0';
    *encodelen = j;

    return;
}

NTSTATUS
PROTO_RecvVector_Version( 
OUT PCHAR* strSecondaryVersion,
IN PSESSION_MANAGER pSessionManger)
{
	return STATUS_SUCCESS;
}

NTSTATUS
PROTO_RecvVector_Handshake( 
OUT int* CP,
IN PSESSION_MANAGER pSessionManger)
{
	return STATUS_SUCCESS;
}

NTSTATUS
PROTO_RecvVector_CheckConfig( 
OUT PULONG ulRemoteDevSize,
OUT PULONG ulDevId,
IN PSESSION_MANAGER pSessionManger)
{
	return STATUS_SUCCESS;
}

NTSTATUS
PROTO_RecvVector_CheckSum( 
OUT PVOID* ptrDeltaMap,
IN PSESSION_MANAGER pSessionManger)
{
	return STATUS_SUCCESS;
}

#if 1 // VEERA : Add This API
NTSTATUS
Proto_Compare_and_updateAckHdr( PSFTK_LG		Sftk_Lg,
								PMM_PROTO_HDR	DestProtoHdr,		// compare or update
								ftd_header_t	*AckProtoHdr,		// Ack Recieved Proto Hdr
								PVOID			RetBuffer,			// Optional, for Payload, Copy this to new allocated buffer
								ULONG			RetBufferSize )	// optional : Size of RetBuffer
{
	ftd_header_t		*pProtoHdr;
	NTSTATUS			status		= STATUS_SUCCESS;
	LONG				nCommandType= -1;
	WCHAR				wchar1[64],wchar3[128];

	OS_ASSERT(DestProtoHdr != NULL);
	OS_ASSERT(MM_IsProtoHdr(DestProtoHdr) == TRUE);
	OS_ASSERT(DestProtoHdr->RetBuffer == NULL);
	OS_ASSERT(DestProtoHdr->RetProtoHDr == NULL);

	try
	{
		DebugPrint((DBG_PROTO, "+++ Proto_Compare_and_updateAckHdr(): Send:MsgId %d,devid:0x%08x,O:%d,len:%d,  Recv:MsgId %d,devid:0x%08x,O:%d,len:%d +++\n",
					DestProtoHdr->Hdr.msgtype, DestProtoHdr->Hdr.msg.lg.devid, DestProtoHdr->Hdr.msg.lg.offset, DestProtoHdr->Hdr.msg.lg.len,
					AckProtoHdr->msgtype, AckProtoHdr->msg.lg.devid, AckProtoHdr->msg.lg.offset, AckProtoHdr->msg.lg.len));

		if( (AckProtoHdr->msgtype == FTDACKCHUNK) || (AckProtoHdr->msgtype == FTDACKRFSTART) )
		{ // TODO : check for FTDMSGCPON, FTDMSGCPOFF
			switch(DestProtoHdr->Hdr.msgtype)
			{ // sender's pkt
				case FTDCCHUNK:
							status = STATUS_SUCCESS;
							switch (DestProtoHdr->msgtype)
							{ // original msgtype
								case FTDMSGCO:
												if (AckProtoHdr->msgtype != FTDACKCHUNK)
												{
													status = STATUS_OBJECT_TYPE_MISMATCH;
													OS_ASSERT(FALSE);
												}
												break;
								case FTDMSGCPON:
								case FTDMSGCPOFF:
												// TODO : Update this with proper values...
												if (AckProtoHdr->msgtype != FTDACKCHUNK)
												{
													status = STATUS_OBJECT_TYPE_MISMATCH;
													OS_ASSERT(FALSE);
												}
												break;

								case FTDMSGINCO:
								case FTDMSGAVOIDJOURNALS:
											if (AckProtoHdr->msgtype == FTDACKRFSTART)
												status = STATUS_TIMER_RESUME_IGNORED;	// expecting next ack value FTACKCHUNK
											else
												status = STATUS_SUCCESS;	// we are done with this pkt
											break;

								default: // normal IO BAB Data
										// OS_ASSERT(FALSE);
										if (AckProtoHdr->msgtype != FTDACKCHUNK)
										{
											status = STATUS_OBJECT_TYPE_MISMATCH;
											OS_ASSERT(FALSE);
										}
										break;
							} // original msgtype
							break;
				
				
				case FTDMSGINCO:
				case FTDMSGAVOIDJOURNALS:
				case FTDMSGCO:
							OS_ASSERT(FALSE);
							status = STATUS_SUCCESS;
							break;
				default:
							status = STATUS_OBJECT_TYPE_MISMATCH;

							if (nCommandType == FTDACKERR)
							{
								DebugPrint((DBG_PROTO, "Proto_Compare_and_updateAckHdr(): LG %d, Send Msg type %d Recieved Error msg with msg type %d \n", 
												Sftk_Lg->LGroupNumber, DestProtoHdr->Hdr.msgtype, nCommandType));
							}
							// Log event for sending msg recv msg is this ....
							swprintf(  wchar1, L"LG %d Send Msg %d", Sftk_Lg->LGroupNumber, DestProtoHdr->Hdr.msgtype);
							swprintf(  wchar3, L"Recv Msg %d", nCommandType);
							sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_DRIVER_PROTOCOL_ACK_ERROR, status, 
													0, wchar1, wchar3 );
							break;	
			}	
			
			goto done;
		}

		nCommandType = PROTO_GetCommandOfAck(AckProtoHdr);
		if(nCommandType == DestProtoHdr->Hdr.msgtype)
		{
			switch(nCommandType)
			{
				case FTDCCHKSUM:	// Checksum payload or success ack payload msg
					{ // allocate payload memory and returned into caller's protohdr
						PMM_HOLDER		pMM_Holder	= NULL;

						DestProtoHdr->RetProtoHDr = mm_type_alloc(MM_TYPE_PROTOCOL_HEADER);
						if (!DestProtoHdr->RetProtoHDr)
						{ // Error 
							status = STATUS_INSUFFICIENT_RESOURCES;
							DebugPrint((DBG_ERROR, "Proto_Compare_and_updateAckHdr: mm_type_alloc(Alloc for Ret ProtoHDr) failed status 0x%08x, ignoring it !!! FIXME FIXME !! \n",
																	status ));
							leave;
						}
						DebugPrint((DBG_ERROR, "Proto_Compare_and_updateAckHdr: The Ack is perfect for the command in the Migrate Queue so fine !! \n"));

						if(RetBuffer != NULL && RetBufferSize > 0)
						{ // allocate Raw Buffer
							DebugPrint((DBG_ERROR, "Proto_Compare_and_updateAckHdr: There is a payload with this ACK so Copying it to the PMM_PROTO_HDR in the Migration Queue\n"));

							DestProtoHdr->RetBuffer = OS_AllocMemory(NonPagedPool,RetBufferSize);
							if (!DestProtoHdr->RetBuffer)
							{ // Error 
								status = STATUS_INSUFFICIENT_RESOURCES;

								if (DestProtoHdr->RetProtoHDr)
									mm_free_ProtoHdr(Sftk_Lg, DestProtoHdr->RetProtoHDr); // mm_type_free( DestProtoHdr->RetProtoHDr, MM_TYPE_PROTOCOL_HEADER);
									
								DestProtoHdr->RetProtoHDr = NULL;

								DebugPrint((DBG_ERROR, "Proto_Compare_and_updateAckHdr: OS_AllocMemory(%d Alloc for RetBuffer) failed status 0x%08x, ignoring it !!! FIXME FIXME !! \n",
																		RetBufferSize, status ));
								leave;
							}
							OS_ZeroMemory(DestProtoHdr->RetBuffer , RetBufferSize);
							OS_RtlCopyMemory(DestProtoHdr->RetBuffer , RetBuffer , RetBufferSize);
						} // allocate Raw Buffer

						// now copy ack proto hdr
						OS_RtlCopyMemory(	Proto_MMGetProtoHdr( DestProtoHdr->RetProtoHDr), 
											AckProtoHdr, 
											Proto_GetProtoHdrSize(DestProtoHdr->RetProtoHDr));
					}
					break; // Payload case

				default: 
					// just overwrite proto Hdr of sender's
					OS_RtlCopyMemory(	Proto_MMGetProtoHdr( DestProtoHdr ), 
										AckProtoHdr, 
										Proto_GetProtoHdrSize(DestProtoHdr));
					break;
			} // switch(nCommandType)
			status = STATUS_SUCCESS;	// since msg matched always return status success
		}
		else
		{ // Msg did not match, either our protocol is out of order or we got err msg FTDACKERR for command
			status = STATUS_OBJECT_TYPE_MISMATCH; 

			if (nCommandType == FTDACKERR)
			{
				DebugPrint((DBG_PROTO, "Proto_Compare_and_updateAckHdr(): LG %d, Send Msg type %d Recieved Error msg with msg type %d \n", 
								Sftk_Lg->LGroupNumber, DestProtoHdr->Hdr.msgtype, nCommandType));
			}
			// Log event for sending msg recv msg is this ....
			swprintf(  wchar1, L"LG %d Send Msg %d", Sftk_Lg->LGroupNumber, DestProtoHdr->Hdr.msgtype);
			swprintf(  wchar3, L"Recv Msg %d", nCommandType);
			sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_DRIVER_PROTOCOL_ACK_ERROR, status, 
									0, wchar1, wchar3 );
			// no need to allocate any buffer just return error, connection will get reset....
			// DestProtoHdr->RetProtoHDr = OS_AllocMemory(NonPagedPool,Proto_GetProtoHdrSize(DestProtoHdr));
			// OS_ZeroMemory(DestProtoHdr->RetProtoHDr, Proto_GetProtoHdrSize(DestProtoHdr));
			// OS_RtlCopyMemory(DestProtoHdr->RetProtoHDr,AckProtoHdr,Proto_GetProtoHdrSize(DestProtoHdr));
			
		}
	}
	finally
	{
	}
done:
	Proto_MMSetStatusInProtoHdr( DestProtoHdr, status );

	return status;
} // Proto_Compare_and_updateAckHdr
#endif


/*EOF*/
