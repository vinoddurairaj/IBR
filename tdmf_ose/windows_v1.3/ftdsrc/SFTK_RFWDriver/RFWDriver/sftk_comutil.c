/**************************************************************************************

Module Name: sftk_comutil.c   
Author Name: Veera Arja
Description: Describes Modules: Protocol and CommunicationFunctions
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#include "sftk_main.h"

#define MAX_ADDRESS_BUFFER_SIZE	512

VOID
TDI_DoQueryAddressInfoTest(
						PTCP_SESSION  pSession
						)
{
//	PTCPC_SESSION pCliSession = pSession->clientSession;
	NTSTATUS    status = STATUS_SUCCESS;
	PUCHAR	pInfoBuffer= NULL;
	ULONG nInfoBufferSize = MAX_ADDRESS_BUFFER_SIZE;
	
	NDIS_STATUS NdisStatus = NdisAllocateMemoryWithTag((PVOID)pInfoBuffer,nInfoBufferSize,MEM_TAG);

	if(NdisStatus != NDIS_STATUS_SUCCESS)
	{
		DebugPrint((DBG_ERROR, "TDI_DoQueryAddressInfoTest(): failed to allocate Memory for MAX_ADDRESS_BUFFER_SIZE = %d\n",MAX_ADDRESS_BUFFER_SIZE)); 
//		KdPrint(("Failed To Allocate Memory\n"));
		return;
	}


   //
   // Query TDI Address Info On Transport Address
   //
   status = TDI_QueryAddressInfo(
				pSession->pServer->KSAddress.m_pFileObject,  // Address Object
               pInfoBuffer,
               &nInfoBufferSize
               );

   DebugPrint((DBG_TDI_QUERY, "TDI_DoQueryAddressInfoTest(): Query Address Info status: 0x%8.8x\n",status)); 
//   KdPrint(( "Query Address Info status: 0x%8.8x\n", status ));

   if( NT_SUCCESS( status ) )
   {
      DEBUG_DumpAddressInfo( (PTDI_ADDRESS_INFO )pInfoBuffer );
   }

   //
   // Query TDI Address Info On Connection Endpoint
   //
   status = TDI_QueryAddressInfo(
               pSession->KSEndpoint.m_pFileObject,  // Connection Object
               pInfoBuffer,
               &nInfoBufferSize
               );

   DebugPrint((DBG_TDI_QUERY, "TDI_DoQueryAddressInfoTest(): Query Address Info status: 0x%8.8x\n",status)); 
//   KdPrint(( "Query Address Info status: 0x%8.8x\n", status ));

   if( NT_SUCCESS( status ) )
   {
      DEBUG_DumpAddressInfo( (PTDI_ADDRESS_INFO )pInfoBuffer );
   }

   NdisFreeMemory((PVOID)pInfoBuffer,nInfoBufferSize,0);
}

VOID
TDI_DoQueryConnectionInfoTest(
						PTCP_SESSION  pSession
						)
{
//	PTCPC_SESSION pCliSession = pSession->clientSession;
	NTSTATUS    status = STATUS_SUCCESS;
//   ULONG       nInfoBufferSize = sizeof( pSession->m_InfoBuffer );

	PUCHAR	pInfoBuffer= NULL;
	ULONG nInfoBufferSize = MAX_ADDRESS_BUFFER_SIZE;
	
	NDIS_STATUS NdisStatus = NdisAllocateMemoryWithTag((PVOID)pInfoBuffer,nInfoBufferSize,MEM_TAG);

	if(NdisStatus != NDIS_STATUS_SUCCESS)
	{
		DebugPrint((DBG_ERROR, "TDI_DoQueryConnectionInfoTest(): Failed To Allocate Memory for MAX_ADDRESS_BUFFER_SIZE = %d\n",MAX_ADDRESS_BUFFER_SIZE)); 
//		KdPrint(("Failed To Allocate Memory\n"));
		return;
	}

   //
   // Query TDI Connection Info
   //
   status = TDI_QueryConnectionInfo(
				&pSession->KSEndpoint,  // Connection Endpoint
				pInfoBuffer,
				&nInfoBufferSize
               );

   DebugPrint((DBG_TDI_QUERY, "TDI_DoQueryConnectionInfoTest(): Query Connection Info status: 0x%8.8x\n", status )); 
//   KdPrint(( "Query Connection Info status: 0x%8.8x\n", status ));

   if( NT_SUCCESS( status ) )
   {
      DEBUG_DumpConnectionInfo( (PTDI_CONNECTION_INFO )pInfoBuffer );
   }
   NdisFreeMemory((PVOID)pInfoBuffer,nInfoBufferSize,0);
}

VOID 
TDI_DUMP_ConnectionInfo(
					PCONNECTION_DETAILS lpConnectionDetails
					)
{
	PCONNECTION_INFO pConnectionInfo = NULL;
	int i =0;

	DebugPrint((DBG_TDI_QUERY, "TDI_DUMP_ConnectionInfo(): Number of Connections = %d\n" , lpConnectionDetails->nConnections )); 
//	KdPrint( ( "Number of Connections = %d\n" , lpConnectionDetails->nConnections ) );

	pConnectionInfo = lpConnectionDetails->ConnectionDetails;
	for(i=0;i<lpConnectionDetails->nConnections;i++)
	{
		DebugPrint((DBG_TDI_QUERY, "TDI_DUMP_ConnectionInfo(): The Connection Local Address = %lx , Local Port = %lx , \n RemoteAddres = %lx , Remote Port = %lx , \n NumberOfSession = %lx\n",
			   pConnectionInfo->ipLocalAddress.in_addr, pConnectionInfo->ipLocalAddress.sin_port,
			   pConnectionInfo->ipRemoteAddress.in_addr, pConnectionInfo->ipRemoteAddress.sin_port,
			   pConnectionInfo->nNumberOfSessions));
		pConnectionInfo += 1;
	}
}
// End File sftk_comutil.c