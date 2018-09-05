/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include "TDI.H"
#include "TDIKRNL.H"
//#include "KSTcpEx.h"
#include "KSUtil.h"

#include "TCPEcho.H"
#include "TDIEcho.h"

// Copyright And Configuration Management ----------------------------------
//
//               TDI TCP Echo Server Implementation - TCPEcho.c
//
//                     Network Development Framework (NDF)
//                                    For
//                          Windows 95 And Windows NT
//
//      Copyright (c) 1997-2001, Printing Communications Associates, Inc.
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------

typedef
struct _TCPS_Connection
{
   LIST_ENTRY                    m_ListElement;

   BOOLEAN                       m_bIsConnectionObjectValid;
   KS_ENDPOINT                 m_KS_Endpoint;

   BOOLEAN                       m_bIsConnected;
   BOOLEAN                       m_bIsDisconnecting;

   LIST_ENTRY                    m_ReceivedTcpPacketList;

   TA_IP_ADDRESS                 m_RemoteAddress;
   TDI_CONNECTION_INFORMATION    m_RemoteConnectionInfo;

   PIRP                          m_pAcceptIrp;
   PIRP                          m_pDisconnectIrp;
   PKEVENT                       m_pWaitEvent;
}
   TCPS_Connection, *PTCPS_Connection;

#define   MAX_CONNECTIONS   8

#define TCPS_PACKET_SIGN   (UINT)0x45434845    /* ECHO */
#define TCPS_BUFFER_SIZE   4096

typedef
struct _TCPS_PACKET_RESERVED
{
   UINT               m_Signature;      // TCPS_PACKET_SIGN

   LIST_ENTRY         m_ListElement;

   PTCPS_Connection   m_pConnection;

   UCHAR               m_DataBuffer[ TCPS_BUFFER_SIZE ];

   IO_STATUS_BLOCK   m_PacketIoStatus;

   PIRP              m_pReceiveIrp;

   TA_IP_ADDRESS                  m_RemoteAddress;
   TDI_CONNECTION_INFORMATION      m_RemoteConnectionInfo;

   ULONG               m_nPackingInsurance;   // Spare. Do Not Use.
}
   TCPS_PACKET_RESERVED, *PTCPS_PACKET_RESERVED;


/* The TCPS_PACKET Structure
----------------------------
 * This is an NDIS_PACKET structure which has been extended to include
 * additional fields (i.e., TCPS_PACKET_RESERVED) specific to this
 * implementation of the ECHO TCP server.
 */
typedef
struct _TCPS_PACKET
{
   NDIS_PACKET          Packet;
   TCPS_PACKET_RESERVED   Reserved;
}
   TCPS_PACKET, *PTCPS_PACKET;

#define   TCPS_PACKET_POOL_SIZE      32
#define   TCPS_BUFFER_POOL_SIZE      TCPS_PACKET_POOL_SIZE

//
// Notes On Receiving
// ------------------
// The TDIECHO sample illustrates two different methods for receiving TCP
// data:
//
//   TdiReceive Requests
//   TdiReceive Events
//
// The method used for receiving TCP data is determined at compile time by
// the USE_RECEIVE_EVENT_HANDLER preprocessor directive.
//
//   USE_RECEIVE_EVENT_HANDLER not defined
//      Receive TCP data by making explicit calls to TdiReceive.
//
//   USE_RECEIVE_EVENT_HANDLER defined
//      Receive TCP data as it is indicated to the TDI_EVENT_RECEIVE event
//      handler.
//
//#define USE_RECEIVE_EVENT_HANDLER

//
// TCPS Global Variables
//
int                  g_nTCPS_UseCount = 0;

TCPS_Connection      g_ConnectionSpace[MAX_CONNECTIONS];

LIST_ENTRY           g_FreeConnectionList;
LIST_ENTRY           g_OpenConnectionList;

static BOOLEAN       g_bPacketPoolAllocated = FALSE;
static NDIS_HANDLE   g_hPacketPool = NULL;   // NDIS Packet Pool

static BOOLEAN       g_bBufferPoolAllocated = FALSE;
static NDIS_HANDLE   g_hBufferPool = NULL;   // NDIS Buffer Pool

static BOOLEAN         g_bAddressOpen = FALSE;
static KS_ADDRESS    g_KS_Address;
static TA_IP_ADDRESS g_LocalAddress;

//
// Local Procedure Prototypes
//
VOID
TCPS_SendCompletion(
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
    );

#ifdef USE_RECEIVE_EVENT_HANDLER

NTSTATUS
TCPS_TransferDataCallback(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp,
   IN PVOID pContext
   );

#else

VOID
TCPS_ReceiveCompletion(
   PVOID UserCompletionContext,
   PIO_STATUS_BLOCK IoStatusBlock,
   ULONG Reserved
   );

#endif // USE_RECEIVE_EVENT_HANDLER

/////////////////////////////////////////////////////////////////////////////
//                       U T I L I T Y  R O U T I N E S                    //
/////////////////////////////////////////////////////////////////////////////

PTCPS_Connection
TCPS_AllocConnection( void )
{
   PTCPS_Connection pConnection;

   //
   // Fetch A TCPS_Connection Structure From The Free List
   //
   if( IsListEmpty( &g_FreeConnectionList ) )
   {
      return( (PTCPS_Connection )NULL );
   }

   pConnection = (PTCPS_Connection )RemoveHeadList( &g_FreeConnectionList );

   if( !pConnection )
   {
      return( (PTCPS_Connection )NULL );
   }

   //
   // Sanity Checks
   //
   ASSERT( pConnection->m_bIsConnectionObjectValid );

   if( !pConnection->m_bIsConnectionObjectValid )
   {
      return( (PTCPS_Connection )NULL );
   }

   //
   // Set Connection State
   //
   pConnection->m_bIsConnected = FALSE;
   pConnection->m_bIsDisconnecting = FALSE;

   ASSERT( IsListEmpty( &pConnection->m_ReceivedTcpPacketList ) );

   return( pConnection );
}


VOID
TCPS_FreeConnection(
   PTCPS_Connection pConnection
   )
{
   //
   // Set Connection State
   //
   pConnection->m_bIsConnected = FALSE;
   pConnection->m_bIsDisconnecting = FALSE;

   //
   // Return Connection To Free List
   //
   InsertTailList(
      &g_FreeConnectionList,
      &pConnection->m_ListElement
      );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_FreePacketAndBuffers
//
// Purpose
// Free the NDIS_PACKET and associated NDIS_BUFFER's associated with the
// specified TCPS_PACKET.
//
// Parameters
//
// Return Value
// 
// Remarks
//

VOID
TCPS_FreePacketAndBuffers(
   PTCPS_PACKET pTCPS_Packet
   )
{
   ULONG          nDataSize, nBufferCount;
   PNDIS_BUFFER   pNdisBuffer;

   //
   // Sanity Check On Arguments
   //
   ASSERT( pTCPS_Packet );

   if( !pTCPS_Packet )
   {
      return;
   }

   //
   // Verify The Packet Signature
   //
   ASSERT( pTCPS_Packet->Reserved.m_Signature == TCPS_PACKET_SIGN );

   if( pTCPS_Packet->Reserved.m_Signature != TCPS_PACKET_SIGN )
   {
#if DBG
      DbgPrint( "TCPS_FreePacketAndBuffers: Invalid Packet Signature\n" );
#endif

      return;
   }

   //
   // Invalidate The Signature
   //
   pTCPS_Packet->Reserved.m_Signature = 0;      // Zap The Signature

   //
   // Recycle The Packet
   //
   KS_FreePacketAndBuffers( (PNDIS_PACKET )pTCPS_Packet );
}



/////////////////////////////////////////////////////////////////////////////
//                       C O N N E C T  R O U T I N E S                    //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
TCPS_ConnectedCallback(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp,
   IN PVOID Context
   )
{
   PTCPS_Connection   pConnection = (PTCPS_Connection )Context;
   NTSTATUS          nFinalStatus = pIrp->IoStatus.Status;
   PTCPS_PACKET      pTCPS_Packet;
   NDIS_STATUS       nNdisStatus;
   PNDIS_BUFFER      pNdisBuffer;

#if DBG
   DbgPrint( "TCPS_ConnectedCallback: FinalStatus: %d\n", nFinalStatus );
#endif

   if( NT_SUCCESS( nFinalStatus ) )
   {
      pConnection->m_bIsConnected = TRUE;
   }
   else
   {
      pConnection->m_bIsConnected = FALSE;
      pConnection->m_bIsDisconnecting = FALSE;

      //
      // Remove From Open Connection List
      //
      RemoveEntryList( &pConnection->m_ListElement );

      //
      // Return Connection To Free List
      //
      TCPS_FreeConnection( pConnection );
   }

   IoFreeIrp( pIrp );   // Don't Access pIrp Any More...

   if( !NT_SUCCESS( nFinalStatus ) )
   {
      return( STATUS_MORE_PROCESSING_REQUIRED );
   }


#ifndef USE_RECEIVE_EVENT_HANDLER

   //
   // Allocate The Receive Packet Descriptor
   // --------------------------------------
   // Use of this structure is adopted from it's use in lower-level
   // NDIS protocol drivers. It is simply a convienient way to allocate
   // the space needed to handle packet reception.
   //
   NdisAllocatePacket(
      &nNdisStatus,
      &(PNDIS_PACKET )pTCPS_Packet,
      g_hPacketPool
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pTCPS_Packet )
   {
      // ATTENTION!!! Update Statistics???

#if DBG
      DbgPrint( "TCPS_ConnectedCallback Could Not Allocate Packet\n" );
#endif

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   //
   // Initialize The Packet Signature
   //
   pTCPS_Packet->Reserved.m_Signature = TCPS_PACKET_SIGN;

   //
   // Save Connection Pointer For Use In Callback Routine
   //
   pTCPS_Packet->Reserved.m_pConnection = pConnection;

   //
   // Allocate An NDIS Buffer Descriptor For The Receive Data
   //
   NdisAllocateBuffer(
      &nNdisStatus,
      &pNdisBuffer,
      g_hBufferPool,
      pTCPS_Packet->Reserved.m_DataBuffer,   // Private Buffer
      TCPS_BUFFER_SIZE                        // Private Buffer's Size
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pNdisBuffer )
   {
      // ATTENTION!!! Update Statistics???

      NdisFreePacket( (PNDIS_PACKET )pTCPS_Packet );

#if DBG
      DbgPrint( "TCPS_ConnectedCallback Could Not Allocate Buffer\n" );
#endif

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   NdisChainBufferAtFront( (PNDIS_PACKET )pTCPS_Packet, pNdisBuffer );

   //
   // Start A Receive On The Connection
   //
   KS_ReceiveOnEndpoint(
      &pConnection->m_KS_Endpoint,
      NULL,                   // User Completion Event
      TCPS_ReceiveCompletion, // User Completion Routine
      pTCPS_Packet,            // User Completion Context
      &pTCPS_Packet->Reserved.m_PacketIoStatus,
      pNdisBuffer,            // MdlAddress
      0                       // Flags
      );

#endif // USE_RECEIVE_EVENT_HANDLER

   return( STATUS_MORE_PROCESSING_REQUIRED );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_ConnectEventHandler
//
// Purpose
// Called by the TDI to indicate that a connection request has been received
// from a remote client.
//
// Parameters
//   TdiEventContext
//   RemoteAddressLength
//   RemoteAddress
//   UserDataLength
//   UserData
//   OptionsLength
//   Options
//   ConnectionContext
//   hAcceptIrp
//
// Return Value
// 
// Remarks
//

NTSTATUS
TCPS_ConnectEventHandler(
   IN PVOID TdiEventContext,     // Context From SetEventHandler
   IN LONG RemoteAddressLength,
   IN PVOID RemoteAddress,
   IN LONG UserDataLength,       // Unused for MSTCP
   IN PVOID UserData,            // Unused for MSTCP
   IN LONG OptionsLength,
   IN PVOID Options,
   OUT CONNECTION_CONTEXT *ConnectionContext,
   OUT PIRP *hAcceptIrp
   )
{
   PTCPS_Connection   pConnection = NULL;
   TDI_STATUS        nTdiStatus;
   PDEVICE_OBJECT    pTCPDeviceObject;

#if DBG
   DbgPrint( "TCPS_ConnectEventHandler Entry...\n" );
#endif

   //
   // Allocate A Connection
   //
   pConnection = TCPS_AllocConnection();

   ASSERT( pConnection );

   if( !pConnection )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   pTCPDeviceObject = IoGetRelatedDeviceObject(
                        pConnection->m_KS_Endpoint.m_pFileObject
                        );

   //
   // Save Remote Client Address
   //
   NdisMoveMemory(
      &pConnection->m_RemoteAddress,
      RemoteAddress,
      sizeof( TA_IP_ADDRESS )
      );

#if DBG
   DEBUG_DumpTransportAddress(
      (PTRANSPORT_ADDRESS )&pConnection->m_RemoteAddress
      );
#endif

   //
   // Allocate Resources To Call TDI To Accept
   //
   pConnection->m_pAcceptIrp = IoAllocateIrp(
                                 pTCPDeviceObject->StackSize,
                                 FALSE
                                 );

   ASSERT( pConnection->m_pAcceptIrp );

   if( !pConnection->m_pAcceptIrp )
      return( STATUS_INSUFFICIENT_RESOURCES );

   //
   // Build The Accept Request
   //
   pConnection->m_RemoteConnectionInfo.UserDataLength = 0;
   pConnection->m_RemoteConnectionInfo.UserData = NULL;

   pConnection->m_RemoteConnectionInfo.OptionsLength = 0;
   pConnection->m_RemoteConnectionInfo.Options = NULL;

   pConnection->m_RemoteConnectionInfo.RemoteAddressLength = sizeof( TA_IP_ADDRESS );
   pConnection->m_RemoteConnectionInfo.RemoteAddress = &pConnection->m_RemoteAddress;

   TdiBuildAccept(
      pConnection->m_pAcceptIrp,          // IRP
      pTCPDeviceObject,                   // Pointer To TDI Device Object
      pConnection->m_KS_Endpoint.m_pFileObject,   // Connection Endpoint File Object
      TCPS_ConnectedCallback,
      pConnection,
      &pConnection->m_RemoteConnectionInfo,
      &pConnection->m_RemoteConnectionInfo
      );

   //
   // Make the next stack location current.  Normally IoCallDriver would
   // do this, but for this IRP it has been bypassed.
   //
   IoSetNextIrpStackLocation( pConnection->m_pAcceptIrp );

   //
   // Place The New Connection In The Open Connection List
   //
   InsertTailList(
      &g_OpenConnectionList,
      &pConnection->m_ListElement
      );

   *hAcceptIrp = pConnection->m_pAcceptIrp;
   *ConnectionContext = pConnection;

   return( STATUS_MORE_PROCESSING_REQUIRED );   // Accept The Connection
}


/////////////////////////////////////////////////////////////////////////////
//                          S E N D  R O U T I N E S                       //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// TCPS_EchoTcpPackets
//
// Purpose
// Call to handle packets queued on the ReceivedTcpPacketList.
//
// Parameters
//
// Return Value
// 
// Remarks
//

VOID
TCPS_EchoTcpPackets(
   PTCPS_Connection pConnection
   )
{
   //
   // Echo TCP Packets
   //
   while( !IsListEmpty( &pConnection->m_ReceivedTcpPacketList ) )
   {
      PLIST_ENTRY    linkage;
      PTCPS_PACKET   pTCPS_Packet;
      PNDIS_BUFFER   pNdisBuffer = NULL;
      ULONG          nDataSize, nByteCount, nBufferCount;
      TDI_STATUS     nTdiStatus;

      //
      // Find The Oldest Packet
      //
      linkage = RemoveHeadList( &pConnection->m_ReceivedTcpPacketList );

      pTCPS_Packet = CONTAINING_RECORD(
                     linkage,
                     TCPS_PACKET,
                     Reserved.m_ListElement
                     );

      //
      // Check On State
      //
      if( !pConnection->m_bIsConnected || pConnection->m_bIsDisconnecting  )
      {
         //
         // Recycle The Packet And Buffers
         //
         TCPS_FreePacketAndBuffers( pTCPS_Packet );

         continue;
      }

      //
      // Query The NDIS_PACKET
      // ---------------------
      // NdisQueryPacket is called to locate the NDIS_BUFFER to be passed to
      // TdiSend(). The nDataSize will be the unmodified original size of
      // the buffer - *NOT* the amount of data which was transfered to the
      // buffer by TDI.
      //
      NdisQueryPacket(
         (PNDIS_PACKET )pTCPS_Packet,
         (PULONG )NULL,
         (PULONG )&nBufferCount,
         &pNdisBuffer,
         &nDataSize
         );

      if( !pNdisBuffer )
      {
         //
         // Recycle The Packet And Buffers
         //
         TCPS_FreePacketAndBuffers( pTCPS_Packet );

         return;
      }

      nTdiStatus = KS_SendOnEndpoint(
                        &pConnection->m_KS_Endpoint,
                        NULL,          // User Completion Event
                        TCPS_SendCompletion,  // User Completion Routine
                        pTCPS_Packet,   // User Completion Context
                        &pTCPS_Packet->Reserved.m_PacketIoStatus,
                        pNdisBuffer,   // MdlAddress
                        0              // Send Flags
                        );

      if( !NT_SUCCESS( nTdiStatus ) )
      {
         break;
      }
   }
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_SendCompletion
//
// Purpose
// Called by TDI when the send operation completes.
//
// Parameters
//
// Return Value
// 
// Remarks
//

VOID
TCPS_SendCompletion(
   PVOID UserCompletionContext,
   PIO_STATUS_BLOCK IoStatusBlock,
   ULONG Reserved
   )
{
   PTCPS_Connection   pConnection;
   PTCPS_PACKET      pTCPS_Packet;
   TDI_STATUS        nTdiStatus;
   PNDIS_BUFFER      pNdisBuffer;
   ULONG             nDataSize, nBufferCount;
   NTSTATUS          nFinalStatus = IoStatusBlock->Status;
   ULONG             nByteCount = IoStatusBlock->Information;

#if DBG
   DbgPrint( "TCPS_SendCompletion: FinalStatus: %d; BytesSent: %d\n", nFinalStatus, nByteCount );
#endif

   pTCPS_Packet = (PTCPS_PACKET )UserCompletionContext;

   //
   // Sanity Check On Passed Parameters
   //
   ASSERT( pTCPS_Packet );

   if( !pTCPS_Packet )
   {
      return;
   }

   //
   // Verify The Packet Signature
   //
   ASSERT( pTCPS_Packet->Reserved.m_Signature == TCPS_PACKET_SIGN );

   if( pTCPS_Packet->Reserved.m_Signature != TCPS_PACKET_SIGN )
   {
#if DBG
      DbgPrint( "TCPS_SendCompletion: Invalid Packet Signature\n" );
#endif

      return;
   }

   pConnection = pTCPS_Packet->Reserved.m_pConnection;


   if( !NT_SUCCESS( nFinalStatus ) )
   {
      // ATTENTION!!! Update Statistics???
   }

   //
   // Possibly Send Another Packet
   //
   TCPS_EchoTcpPackets( pConnection );

   //
   // Query The NDIS_PACKET
   //
   NdisQueryPacket(
      (PNDIS_PACKET )pTCPS_Packet,
      (PULONG )NULL,
      (PULONG )&nBufferCount,
      &pNdisBuffer,
      &nDataSize
      );

   if( !pNdisBuffer )
   {
      //
      // Recycle The Packet And Buffers
      //
      TCPS_FreePacketAndBuffers( pTCPS_Packet );

      return;
   }

   //
   // Reset pNdisBuffer->Length
   // -------------------------
   // When the packet was received on this NDIS_BUFFER, the Length field
   // was adjusted to the amount of data copied into the buffer. Here
   // the Length field is reset to indicate the size of the buffer.
   //
   NdisAdjustBufferLength( pNdisBuffer, TCPS_BUFFER_SIZE );

#ifdef USE_RECEIVE_EVENT_HANDLER

   //
   // Recycle The Packet And Buffers
   //
   TCPS_FreePacketAndBuffers( pTCPS_Packet );

#else

   //
   // Check On State
   //
   if( !pConnection->m_bIsConnected || pConnection->m_bIsDisconnecting  )
   {
      //
      // Recycle The Packet And Buffers
      //
      TCPS_FreePacketAndBuffers( pTCPS_Packet );
   }
   else
   {
      //
      // Start Another Receive On Same Buffer
      //
      KS_ReceiveOnEndpoint(
         &pConnection->m_KS_Endpoint,
         NULL,       // User Completion Event
         TCPS_ReceiveCompletion,// User Completion Routine
         pTCPS_Packet,   // User Completion Context
         &pTCPS_Packet->Reserved.m_PacketIoStatus,
         pNdisBuffer,   // MdlAddress
         0           // Flags
         );
   }

#endif // USE_RECEIVE_EVENT_HANDLER

   return;
}


/////////////////////////////////////////////////////////////////////////////
//                      R E C E I V E  R O U T I N E S                     //
/////////////////////////////////////////////////////////////////////////////


#ifdef USE_RECEIVE_EVENT_HANDLER

/////////////////////////////////////////////////////////////////////////////
//// TCPS_TransferDataCallback
//
// Purpose
// Called by TDI when the receive data transfer initiated by ClientEventReceive
// or TdiReceive completes.
//
// Parameters
//
// Return Value
// 
// Remarks
// For non-error cases, this function saves the number of bytes which TDI
// has copied into the buffer and then queues the TCPS_Packet in the 
// ReceivedTcpPacketList. It then calls a routine which can send the packet
// back to the originator.
//

NTSTATUS
TCPS_TransferDataCallback(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp,
   IN PVOID pContext
   )
{
   PTCPS_Connection   pConnection;
   PTCPS_PACKET      pTCPS_Packet;
   PNDIS_BUFFER      pNdisBuffer;
   ULONG               nDataSize, nBufferCount;
   NTSTATUS            nFinalStatus = pIrp->IoStatus.Status;
   ULONG               nByteCount = pIrp->IoStatus.Information;

#if DBG
   DbgPrint( "TCPS_TransferDataCallback: FinalStatus: %d; Bytes Transfered: %d\n",
      nFinalStatus, nByteCount );
#endif

   pTCPS_Packet = (PTCPS_PACKET )pContext;

   //
   // Sanity Checks
   //
   ASSERT( pTCPS_Packet );

   if( !pTCPS_Packet )
   {
      IoFreeIrp( pIrp );

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   //
   // Verify The Packet Signature
   //
   ASSERT( pTCPS_Packet->Reserved.m_Signature == TCPS_PACKET_SIGN );

   if( pTCPS_Packet->Reserved.m_Signature != TCPS_PACKET_SIGN )
   {
#if DBG
      DbgPrint( "TCPS_TransferDataCallback: Invalid Packet Signature\n" );
#endif

      IoFreeIrp( pIrp );

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   pConnection = pTCPS_Packet->Reserved.m_pConnection;

   //
   // Check On State
   //
   if( !pConnection->m_bIsConnected || pConnection->m_bIsDisconnecting  )
   {
      //
      // Recycle The Packet And Buffers
      //
      TCPS_FreePacketAndBuffers( pTCPS_Packet );

      IoFreeIrp( pIrp );

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   //
   // Handle Transfer Failure
   //
   if( !NT_SUCCESS( nFinalStatus ) )
   {
      //
      // Recycle The Packet And Buffers
      //
      TCPS_FreePacketAndBuffers( pTCPS_Packet );

      // ATTENTION!!! Update Statistics???

      IoFreeIrp( pIrp );

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   // ATTENTION!!! Update Statistics???

   //
   // Query The NDIS_PACKET
   // ---------------------
   // NdisQueryPacket is called to locate the NDIS_BUFFER to be passed to
   // TdiSend(). The nDataSize will be the unmodified original size of
   // the buffer - *NOT* the amount of data which was transfered to the
   // buffer by TDI.
   //
   NdisQueryPacket(
      (PNDIS_PACKET )pTCPS_Packet,
      (PULONG )NULL,
      (PULONG )&nBufferCount,
      &pNdisBuffer,
      &nDataSize
      );

   ASSERT( pNdisBuffer );

   if( !pNdisBuffer )
   {
      //
      // Recycle The Packet And Buffers
      //
      TCPS_FreePacketAndBuffers( pTCPS_Packet );

      IoFreeIrp( pIrp );

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   //
   // Save The Byte Count
   // -------------------
   // In this implementation, the NDIS_BUFFER Length field is adjusted
   // to the amount of data that was copied into the buffer. This has
   // the added benefit of insuring that when the packet is sent later
   // the NDIS_BUFFER Length field and the length parameter passed to
   // TdiSend are the same.
   //
   NdisAdjustBufferLength( pNdisBuffer, nByteCount );

   // ATTENTION!!! Check Flags???

   IoFreeIrp( pIrp );   // Don't Access pIrp Any More...

   //
   // Put The Packet In The Connection's Received TCP PacketList
   //
   InsertTailList(
      &pConnection->m_ReceivedTcpPacketList,
      &pTCPS_Packet->Reserved.m_ListElement
      );

   //
   // Possibly Send Another Packet
   //
   TCPS_EchoTcpPackets( pConnection );

   return( STATUS_MORE_PROCESSING_REQUIRED );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_ReceiveEventHandler
//
// Purpose
// Called by the TDI when incomming data arrives on the connection.
//
// Parameters
//
// Return Value
// 
// Remarks
// This handler is NOT called if there is an outstanding TdiReceive or
// we have not accepted all previously indicated data.
//
// Although some data (perhaps all) is available at pReceivedData, it is
// best to handle received data as a two-step operation - which actually
// parallels the two-step reception operation of the underlying protocol
// driver.
//
// The first step in reception is when the received data is "indicated"
// here at the TCPS_ReceiveEventHandler. Here the data at Tsdu and
// the associated ReceiveFlags can be examined to determine if the data
// is of further interest.
//
// If the data is of interest, then resources should be allocated and
// the ??? setup to have the underlying TDI driver transfer
// the data. When transfer is complete, the TCPS_ReceiveCompletion
// callback will ba called.
//
// In the TCPS_ReceiveCompletion the data, which we will then own,
// can be processed further. In the case of the ECHO client, it can be
// sent back to the remote network client.
//
//   ***** TDI Client (US) *****    * TDI Protocol Driver (MSTCP) *
//   TCPS_ReceiveEventHandler       ReceiveHandler
//   TCPS_TransferDataCallback      TransferDataComplete
//

NTSTATUS
TCPS_ReceiveEventHandler(
   IN PVOID TdiEventContext,                 // Context From SetEventHandler
   IN CONNECTION_CONTEXT ConnectionContext,  // Contect From Accept
   IN ULONG ReceiveFlags,
   IN ULONG BytesIndicated,
   IN ULONG BytesAvailable,
   OUT ULONG *BytesTaken,
   IN PVOID Tsdu,            // pointer describing this TSDU, typically a lump of bytes
   OUT PIRP *IoRequestPacket   // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   )
{
   PTCPS_Connection   pConnection;
   PTCPS_PACKET      pTCPS_Packet;
   NDIS_STATUS         nNdisStatus;
   PNDIS_BUFFER      pNdisBuffer;
   PDEVICE_OBJECT    pTCPDeviceObject;

#if DBG
   DbgPrint( "TCPS_ReceiveEventHandler Entry...\n" );
   DbgPrint( "BytesIndicated: %d, BytesAvailable: %d; ReceiveFlags: 0x%X\n",
      BytesIndicated, BytesAvailable, ReceiveFlags );
#endif

   pConnection = (PTCPS_Connection )ConnectionContext;

   //
   // Check On State
   //
   if( !pConnection->m_bIsConnected || pConnection->m_bIsDisconnecting  )
   {
      //
      // Tell TDI That All Data Was Taken
      //
      *BytesTaken = BytesIndicated;
      return( STATUS_SUCCESS );
   }

   //
   // Allocate The Receive Packet Descriptor
   // --------------------------------------
   // Use of this structure is adopted from it's use in lower-level
   // NDIS protocol drivers. It is simply a convienient way to allocate
   // the space needed to handle packet reception.
   //
   NdisAllocatePacket(
      &nNdisStatus,
      &(PNDIS_PACKET )pTCPS_Packet,
      g_hPacketPool
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pTCPS_Packet )
   {
      // ATTENTION!!! Update Statistics???

#if DBG
      DbgPrint( "TCPS_ReceiveEventHandler Could Not Allocate Packet\n" );
#endif

      //
      // Tell TDI That No Data Was Taken
      // -------------------------------
      // TDI will attempt to buffer the data for later consumption or
      // the data will be retransmitted. Special steps may be needed to
      // force the TDI driver to indicate the data not taken at a later
      // time when resources become available.
      //
      *BytesTaken = 0;
      return( STATUS_SUCCESS );
   }

   //
   // Initialize The Packet Signature
   //
   pTCPS_Packet->Reserved.m_Signature = TCPS_PACKET_SIGN;

   //
   // Save Connection Pointer For Use In Callback Routine
   //
   pTCPS_Packet->Reserved.m_pConnection = pConnection;

   //
   // Save Bytes Taken In Internal Buffer
   // -----------------------------------
   // In this sample the data is saved in an internal buffer that is a
   // field in the TCPS_PACKET structure.
   //
   // ATTENTION!!! Note that *BytesTaken is set in the following statement.
   //
   if( BytesIndicated <= TCPS_BUFFER_SIZE )
   {
      *BytesTaken = BytesIndicated;
   }
   else
   {
      *BytesTaken = TCPS_BUFFER_SIZE;
   }

   NdisMoveMemory(
      pTCPS_Packet->Reserved.m_DataBuffer,
      Tsdu,
      *BytesTaken
      );

   //
   // Allocate An NDIS Buffer Descriptor For The Receive Data
   //
   NdisAllocateBuffer(
      &nNdisStatus,
      &pNdisBuffer,
      g_hBufferPool,
      pTCPS_Packet->Reserved.m_DataBuffer,   // Private Buffer
      *BytesTaken
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pNdisBuffer )
   {
      // ATTENTION!!! Update Statistics???

      NdisFreePacket( (PNDIS_PACKET )pTCPS_Packet );

#if DBG
      DbgPrint( "TCPS_ReceiveEventHandler Could Not Allocate Buffer\n" );
#endif

      //
      // Tell TDI That No Data Was Taken
      // -------------------------------
      // TDI will attempt to buffer the data for later consumption or
      // the data will be retransmitted. Special steps may be needed to
      // force the TDI driver to indicate the data not taken at a later
      // time when resources become available.
      //
      *BytesTaken = 0;
      return( STATUS_SUCCESS );
   }

   NdisChainBufferAtFront( (PNDIS_PACKET )pTCPS_Packet, pNdisBuffer );

   //
   // Put The Packet In The Connection's Received TCP PacketList
   //
   InsertTailList(
      &pConnection->m_ReceivedTcpPacketList,
      &pTCPS_Packet->Reserved.m_ListElement
      );

   //
   // Determine Whether Tsdu Contains A Full TSDU
   // -------------------------------------------
   // We could check (ReceiveDatagramFlags & TDI_RECEIVE_ENTIRE_MESSAGE).
   // However, checking (BytesIndicated == BytesAvailable) seems more
   // reliable.
   //
   if( *BytesTaken == BytesAvailable )
   {
      //
      // Possibly Send Another Packet
      //
      TCPS_EchoTcpPackets( pConnection );

      return( STATUS_SUCCESS );
   }

   //
   // Allocate Another Receive Packet Descriptor
   // ------------------------------------------
   // Use of this structure is adopted from it's use in lower-level
   // NDIS protocol drivers. It is simply a convienient way to allocate
   // the space needed to handle packet reception.
   //
   NdisAllocatePacket(
      &nNdisStatus,
      &(PNDIS_PACKET )pTCPS_Packet,
      g_hPacketPool
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pTCPS_Packet )
   {
      // ATTENTION!!! Update Statistics???

#if DBG
      DbgPrint( "TCPS_ReceiveEventHandler Could Not Allocate Packet\n" );
#endif

      return( STATUS_SUCCESS );
   }

   //
   // Initialize The Packet Signature
   //
   pTCPS_Packet->Reserved.m_Signature = TCPS_PACKET_SIGN;

   //
   // Save Connection Pointer For Use In Callback Routine
   //
   pTCPS_Packet->Reserved.m_pConnection = pConnection;

   //
   // Process Partial TSDU
   // --------------------
   // One could check (ReceiveDatagramFlags & TDI_RECEIVE_COPY_LOOKAHEAD) to
   // determine whether copying the BytesIndicated lookahead data is
   // required or not. However, since the case where lookahead data must
   // be copied must be dealt with anyway, it seems simpler to just go ahead
   // and always copy the lookahead data here in the handler.

   //
   // Allocate An NDIS Buffer Descriptor For The Receive Data
   //
   NdisAllocateBuffer(
      &nNdisStatus,
      &pNdisBuffer,
      g_hBufferPool,
      pTCPS_Packet->Reserved.m_DataBuffer,   // Private Buffer
      BytesAvailable - *BytesTaken
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pNdisBuffer )
   {
      // ATTENTION!!! Update Statistics???

      NdisFreePacket( (PNDIS_PACKET )pTCPS_Packet );

#if DBG
      DbgPrint( "TCPS_ReceiveEventHandler Could Not Allocate Buffer\n" );
#endif

      return( STATUS_SUCCESS );
   }

   NdisChainBufferAtFront( (PNDIS_PACKET )pTCPS_Packet, pNdisBuffer );

   //
   // Allocate Resources To Call TDI To Transfer Received Data
   //
   pTCPDeviceObject = IoGetRelatedDeviceObject(
                        pConnection->m_KS_Endpoint.m_pFileObject
                        );

   pTCPS_Packet->Reserved.m_pReceiveIrp = IoAllocateIrp(
                                 pTCPDeviceObject->StackSize,
                                 FALSE
                                 );

   ASSERT( pTCPS_Packet->Reserved.m_pReceiveIrp );

   if( !pTCPS_Packet->Reserved.m_pReceiveIrp )
   {
      TCPS_FreePacketAndBuffers( pTCPS_Packet );

      return( STATUS_SUCCESS );
   }

   TdiBuildReceive(
      pTCPS_Packet->Reserved.m_pReceiveIrp, // IRP
      pTCPDeviceObject,                  // Pointer To TDI Device Object
      pConnection->m_KS_Endpoint.m_pFileObject,   // Connection Endpoint File Object
      TCPS_TransferDataCallback,         // CompletionRoutine
      pTCPS_Packet,                     // Context
      pNdisBuffer,                     // MdlAddress
      0,                                 // ReceiveFlags
      BytesAvailable - *BytesTaken     // ReceiveLength
      );

   //
   // Make the next stack location current.  Normally IoCallDriver would
   // do this, but for this IRP it has been bypassed.
   //
   IoSetNextIrpStackLocation( pTCPS_Packet->Reserved.m_pReceiveIrp );

   //
   // Tell TDI To Transfer The Data
   //
   *IoRequestPacket = pTCPS_Packet->Reserved.m_pReceiveIrp;

   return( STATUS_MORE_PROCESSING_REQUIRED );
}

#else

/////////////////////////////////////////////////////////////////////////////
//// TCPS_ReceiveCompletion
//
// Purpose
//
// Parameters
//
// Return Value
// 
// Remarks
//

VOID
TCPS_ReceiveCompletion(
   PVOID UserCompletionContext,
   PIO_STATUS_BLOCK IoStatusBlock,
   ULONG Reserved
   )
{
   PTCPS_Connection   pConnection;
   PTCPS_PACKET      pTCPS_Packet;
   PNDIS_BUFFER      pNdisBuffer;
   ULONG             nDataSize, nBufferCount;
   NTSTATUS          nFinalStatus = IoStatusBlock->Status;
   ULONG             nByteCount = IoStatusBlock->Information;

#if DBG
   DbgPrint( "TCPS_ReceiveCompletion: FinalStatus: 0x%8.8x; Bytes Transfered: %d\n",
      nFinalStatus, nByteCount );
#endif

   pTCPS_Packet = (PTCPS_PACKET )UserCompletionContext;

   //
   // Sanity Check On Passed Parameters
   //
   ASSERT( pTCPS_Packet );

   if( !pTCPS_Packet )
   {
      return;
   }

   //
   // Verify The Packet Signature
   //
   ASSERT( pTCPS_Packet->Reserved.m_Signature == TCPS_PACKET_SIGN );

   if( pTCPS_Packet->Reserved.m_Signature != TCPS_PACKET_SIGN )
   {
#if DBG
      DbgPrint( "TCPS_ReceiveCompletion: Invalid Packet Signature\n" );
#endif

      return;
   }

   pConnection = pTCPS_Packet->Reserved.m_pConnection;

   //
   // Check On State
   //
   if( !pConnection->m_bIsConnected || pConnection->m_bIsDisconnecting  )
   {
      //
      // Recycle The Packet And Buffers
      //
      TCPS_FreePacketAndBuffers( pTCPS_Packet );

      return;
   }

   //
   // Handle Transfer Failure
   //
   if( !NT_SUCCESS( nFinalStatus ) )
   {
      //
      // Recycle The Packet And Buffers
      //
      TCPS_FreePacketAndBuffers( pTCPS_Packet );

      // ATTENTION!!! Update Statistics???

      return;
   }

   // ATTENTION!!! Update Statistics???

   //
   // Query The NDIS_PACKET
   //
   NdisQueryPacket(
      (PNDIS_PACKET )pTCPS_Packet,
      (PULONG )NULL,
      (PULONG )&nBufferCount,
      &pNdisBuffer,
      &nDataSize
      );

   ASSERT( pNdisBuffer );

   if( !pNdisBuffer )
   {
      //
      // Recycle The Packet And Buffers
      //
      TCPS_FreePacketAndBuffers( pTCPS_Packet );

      return;
   }

   //
   // Save The Byte Count
   // -------------------
   // In this implementation, the NDIS_BUFFER Length field is adjusted
   // to the amount of data that was copied into the buffer. This has
   // the added benefit of insuring that when the packet is sent later
   // the NDIS_BUFFER Length field and the length parameter passed to
   // TdiSend are the same.
   //
   NdisAdjustBufferLength( pNdisBuffer, nByteCount );

   // ATTENTION!!! Check Flags???

   //
   // Put The Packet In The Connection's Received TCP PacketList
   //
   InsertTailList(
      &pConnection->m_ReceivedTcpPacketList,
      &pTCPS_Packet->Reserved.m_ListElement
      );

   //
   // Possibly Send Another Packet
   //
   TCPS_EchoTcpPackets( pConnection );

   return;
}

#endif // USE_RECEIVE_EVENT_HANDLER


/////////////////////////////////////////////////////////////////////////////
//                    D I S C O N N E C T  R O U T I N E S                 //
/////////////////////////////////////////////////////////////////////////////


VOID
TCPS_DisconnectCallback(
   PVOID UserCompletionContext,
   PIO_STATUS_BLOCK IoStatusBlock,
   ULONG Reserved
   )
{
   PTCPS_Connection   pConnection = (PTCPS_Connection )UserCompletionContext;
   NTSTATUS          nFinalStatus = IoStatusBlock->Status;

#if DBG
   DbgPrint( "TCPS_DisconnectCallback: FinalStatus: %d\n", nFinalStatus );
#endif

   //
   // Mark As Not Connected
   //
   pConnection->m_bIsConnected = FALSE;

   //
   // Empty the ReceivedTcpPacketList
   // ----------------------------
   // TCPS_EchoTcpPackets will empty the list if pConnection->m_bIsConnected
   // is FALSE.
   //
   TCPS_EchoTcpPackets( pConnection );

   if( pConnection->m_pWaitEvent )
   {
#if DBG
      DbgPrint( "TCPS_DisconnectCallback: Setting Wait Event\n" );
#endif

      KeSetEvent( pConnection->m_pWaitEvent, 0, FALSE );
      pConnection->m_pWaitEvent = NULL;
   }

   //
   // Return Connection To Free List
   //
   TCPS_FreeConnection( pConnection );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_DisconnectEventHandler
//
// Purpose
// Called by the TDI to indicate that a remote client is closing a
// connection.
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TCPS_DisconnectEventHandler(
   IN PVOID TdiEventContext,                  // Context From SetEventHandler
   IN CONNECTION_CONTEXT ConnectionContext,   // As passed to TdiOpenConnection
   IN LONG DisconnectDataLength,
   IN PVOID DisconnectData,
   IN LONG DisconnectInformationLength,
   IN PVOID DisconnectInformation,
   IN ULONG DisconnectFlags
   )
{
   PTCPS_Connection   pConnection;
   TDI_STATUS        nTdiStatus;

#if DBG
   DbgPrint( "TCPS_DisconnectEventHandler Entry...\n" );
#endif

   pConnection = (PTCPS_Connection )ConnectionContext;

   //
   // Remove From Open Connection List
   //
   RemoveEntryList( &pConnection->m_ListElement );

   switch( DisconnectFlags )
   {
      case TDI_DISCONNECT_WAIT:      // Used For Disconnect Notification
#if DBG
         DbgPrint( "Disconnect: Wait\n" );
#endif
         //
         // Return Connection To Free List
         //
         TCPS_FreeConnection( pConnection );

         break;

      case TDI_DISCONNECT_ABORT:      // Immediately Terminate Connection
#if DBG
         DbgPrint( "Disconnect: Abort\n" );
#endif
         //
         // Return Connection To Free List
         //
         TCPS_FreeConnection( pConnection );

         break;

      case TDI_DISCONNECT_RELEASE:   // Initiate Graceful Disconnect
#if DBG
         DbgPrint( "Disconnect: Release\n" );
#endif

         nTdiStatus = KS_Disconnect(
                        &pConnection->m_KS_Endpoint,
                        NULL,       // UserCompletionEvent
                        TCPS_DisconnectCallback,
                        pConnection,
                        NULL,
                        0
                        );

         break;

      default:
#if DBG
         DbgPrint( "Unexpected Flags: 0x%X\n", DisconnectFlags );
#endif
         break;
   }

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_Startup
//
// Purpose
// Startup the ECHO server.
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TCPS_Startup( PDEVICE_OBJECT pDeviceObject )
{
   NTSTATUS          nTdiStatus;
   NDIS_STATUS       nNdisStatus;
   TA_IP_ADDRESS     ECHOAddress;
   int               i;

#if DBG
   DbgPrint( "TCPS_Startup Entry...\n" );
   DbgPrint( "pDeviceObject: 0x%X\n", pDeviceObject );
#endif

   if( g_nTCPS_UseCount > 0 )
   {
      ++g_nTCPS_UseCount;

      return( STATUS_SUCCESS );
   }

   g_bAddressOpen = FALSE;

   //
   // Initialize Connection Lists
   //
   InitializeListHead( &g_FreeConnectionList );
   InitializeListHead( &g_OpenConnectionList );

   //
   // Allocate The Packet Pool
   //
   NdisAllocatePacketPool(
      &nNdisStatus,
      &g_hPacketPool,
      TCPS_PACKET_POOL_SIZE,
      sizeof( TCPS_PACKET_RESERVED )
      );

   ASSERT( NT_SUCCESS( nNdisStatus ) );

   if( !NT_SUCCESS( nNdisStatus ) )
   {
#if DBG
      DbgPrint( "TCPS_Startup: Failed To Allocate Packet Pool\n" );
#endif

      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   g_bPacketPoolAllocated = TRUE;

   //
   // Allocate The Buffer Pool
   //
   NdisAllocateBufferPool(
      &nNdisStatus,
      &g_hBufferPool,
      TCPS_BUFFER_POOL_SIZE
      );

   ASSERT( NT_SUCCESS( nNdisStatus ) );

   if( !NT_SUCCESS( nNdisStatus ) )
   {
#if DBG
      DbgPrint( "TCPS_Startup: Failed To Allocate Buffer Pool\n" );
#endif

      NdisFreePacketPool( g_hPacketPool );

      g_bPacketPoolAllocated = FALSE;
      g_bBufferPoolAllocated = FALSE;

      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   g_bBufferPoolAllocated = TRUE;

   //
   // Initialize The Local IP Address
   //
   KS_InitIPAddress(
      &g_LocalAddress,              // Pointer To A TA_IP_ADDRESS
      INADDR_ANY,                   // Any Local Internet Address
      KS_htons( IPPORT_ECHO )       // ECHO Port
      );

   //
   // Open The Local TDI Address Object
   //
   nTdiStatus = STATUS_REQUEST_ABORTED;

   nTdiStatus = KS_OpenTransportAddress(
                  TCP_DEVICE_NAME_W,
                  (PTRANSPORT_ADDRESS )&g_LocalAddress,
                  &g_KS_Address
                  );

   ASSERT( NT_SUCCESS( nTdiStatus ) );

   if ( !NT_SUCCESS( nTdiStatus ) )
   {
      return( nTdiStatus );
   }

   //
   // Initialize And Link Up The Free Connections Pool
   //
   for( i = 0; i < MAX_CONNECTIONS ; i++ )
   {
      PTCPS_Connection            pConnection = &g_ConnectionSpace[i];
      OBJECT_ATTRIBUTES          TcpObjectAttributes;
      PVOID                      *contextAddress;
      PFILE_FULL_EA_INFORMATION  eaBuffer;
      ULONG                      eaLength;
      UNICODE_STRING             TcpDeviceName;
      IO_STATUS_BLOCK            IoStatus;

      //
      // Zero The Connection Memory
      //
      NdisZeroMemory( pConnection, sizeof( TCPS_Connection ) );

      InitializeListHead( &pConnection->m_ReceivedTcpPacketList );

      //
      // Create The Connection Endpoint
      //
      nTdiStatus = KS_OpenConnectionEndpoint(
                     TCP_DEVICE_NAME_W,
                     &g_KS_Address,
                     &pConnection->m_KS_Endpoint,
                     pConnection    // Context
                     );

      ASSERT( NT_SUCCESS( nTdiStatus ) );

      if ( !NT_SUCCESS( nTdiStatus ) )
      {
         break;
      }

      pConnection->m_bIsConnectionObjectValid = TRUE;

      InsertTailList(
         &g_FreeConnectionList,
         &g_ConnectionSpace[i].m_ListElement
         );
   }

   g_bAddressOpen = TRUE;

   //
   // Setup Event Handlers On The Server Address Object
   //
   nTdiStatus = KS_SetEventHandlers(
                  &g_KS_Address,
                  pDeviceObject,       // Event Context
                  TCPS_ConnectEventHandler,  // ConnectEventHandler
                  TCPS_DisconnectEventHandler,  // DisconnectEventHandler
                  NULL,                // ErrorEventHandler,
#ifdef USE_RECEIVE_EVENT_HANDLER
                  TCPS_ReceiveEventHandler,  // ReceiveEventHandler
#else
                  NULL,                // ReceiveEventHandler
#endif
                  NULL,                // ReceiveDatagramEventHandler
                  NULL                 // ReceiveExpeditedEventHandler
                  );

   ASSERT( NT_SUCCESS( nTdiStatus ) );

   if( !NT_SUCCESS( nTdiStatus ) )
      return( nTdiStatus );

   g_nTCPS_UseCount = 1;

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_Shutdown
//
// Purpose
// Shutdown the ECHO server.
//
// Parameters
//
// Return Value
// 
// Remarks
//

VOID TCPS_Shutdown( PDEVICE_OBJECT pDeviceObject )
{
   TDI_STATUS        nTdiStatus;

#if DBG
   DbgPrint( "TCPS_Shutdown Entry...\n" );
#endif

   if( --g_nTCPS_UseCount <= 0 )
   {
      PTCPS_Connection   pConnection;

      //
      // Destroy All Connections
      //
      while( !IsListEmpty( &g_OpenConnectionList ) )
      {
         pConnection = (PTCPS_Connection )RemoveHeadList( &g_OpenConnectionList );

#if DBG
         DbgPrint( "Destroying Connection; ConnectionContext: 0x%X\n",
            pConnection );
#endif

         if( pConnection->m_bIsConnected )
         {
            //
            // Perform Synchronous Disconnect
            //
            nTdiStatus = KS_Disconnect(
                           &pConnection->m_KS_Endpoint,
                           NULL,    // UserCompletionEvent
                           NULL,    // UserCompletionRoutine
                           NULL,    // UserCompletionContext
                           NULL,    // pIoStatusBlock
                           0        // Disconnect Flags
                           );
         }

         //
         // Mark As Not Connected
         //
         pConnection->m_bIsConnected = FALSE;

         //
         // Empty the ReceivedTcpPacketList
         // ----------------------------
         // TCPS_EchoTcpPackets will empty the list if pConnection->m_bIsConnected
         // is FALSE.
         //
         TCPS_EchoTcpPackets( pConnection );

         //
         // Return Connection To Free List
         //
         TCPS_FreeConnection( pConnection );
      }

      //
      // Release Connections In The Free Connection List
      //
      while( !IsListEmpty( &g_FreeConnectionList ) )
      {
         pConnection = (PTCPS_Connection )RemoveHeadList( &g_FreeConnectionList );

#if DBG
         DbgPrint( "Destroying Connection; ConnectionContext: 0x%X\n",
            pConnection );
#endif

         //
         // Close The TDI Connection Object, If Necessary
         //
         if( pConnection->m_bIsConnectionObjectValid )
         {
            KS_CloseConnectionEndpoint( &pConnection->m_KS_Endpoint );
         }

         pConnection->m_bIsConnectionObjectValid = FALSE;
      }

      //
      // Close The TDI Address Object, If Necessary
      //
      if( g_bAddressOpen )
      {
         KS_CloseTransportAddress( &g_KS_Address );
      }

      g_bAddressOpen = FALSE;

      //
      // Free Packet And Buffer Pools
      //
      if( g_bBufferPoolAllocated )
         NdisFreeBufferPool( g_hBufferPool );

      g_bBufferPoolAllocated = FALSE;

      if( g_bPacketPoolAllocated )
         NdisFreePacketPool( g_hPacketPool );

      g_bPacketPoolAllocated = FALSE;

      g_nTCPS_UseCount = 0;

      return;
   }
}

