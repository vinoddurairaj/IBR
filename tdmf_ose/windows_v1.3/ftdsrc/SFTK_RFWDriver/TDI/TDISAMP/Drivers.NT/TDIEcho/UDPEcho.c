/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include "TDI.H"
#include "TDIKRNL.H"
#include "KSUtil.h"

#include "UDPEcho.H"
#include "TDIEcho.h"

// Copyright And Configuration Management ----------------------------------
//
//               TDI UDP Echo Server Implementation - UDPEcho.c
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


#define UDPS_PACKET_SIGN   (UINT)0x45434845    /* ECHO */
#define UDPS_BUFFER_SIZE   4096

typedef
struct _UDPS_PACKET_RESERVED
{
   UINT              m_Signature;      // UDPS_PACKET_SIGN

   LIST_ENTRY        m_ListElement;

   UCHAR             m_DataBuffer[ UDPS_BUFFER_SIZE ];

   IO_STATUS_BLOCK   m_PacketIoStatus;

   PIRP              m_pReceiveIrp;

   TDI_CONNECTION_INFORMATION m_RemoteConnectionInfo;
   TA_IP_ADDRESS              m_RemoteAddress;

   ULONG             m_nPackingInsurance;   // Spare. Do Not Use.
}
   UDPS_PACKET_RESERVED, *PUDPS_PACKET_RESERVED;


/* The UDPS_PACKET Structure
----------------------------
 * This is an NDIS_PACKET structure which has been extended to include
 * additional fields (i.e., UDPS_PACKET_RESERVED) specific to this
 * implementation of the ECHO UDP server.
 */
typedef
struct _UDPS_PACKET
{
   NDIS_PACKET          Packet;
   UDPS_PACKET_RESERVED   Reserved;
}
   UDPS_PACKET, *PUDPS_PACKET;

#define   UDPS_PACKET_POOL_SIZE      32
#define   UDPS_BUFFER_POOL_SIZE      UDPS_PACKET_POOL_SIZE

//
// Notes On Receiving
// ------------------
// The TDIECHO sample illustrates two different methods for receiving UDP
// data:
//
//   TdiReceive Requests
//   TdiReceive Events
//
// The method used for receiving UDP data is determined at compile time by
// the USE_RECEIVE_EVENT_HANDLER preprocessor directive.
//
//   USE_RECEIVE_EVENT_HANDLER not defined
//      Receive UDP data by making explicit calls to TdiReceive.
//
//   USE_RECEIVE_EVENT_HANDLER defined
//      Receive UDP data as it is indicated to the TDI_EVENT_RECEIVE event
//      handler.
//
//#define USE_RECEIVE_EVENT_HANDLER

//
// UDPS Global Variables
//
static BOOLEAN       g_bPacketPoolAllocated = FALSE;
static NDIS_HANDLE   g_hPacketPool = NULL;   // NDIS Packet Pool

static BOOLEAN       g_bBufferPoolAllocated = FALSE;
static NDIS_HANDLE   g_hBufferPool = NULL;   // NDIS Buffer Pool

LIST_ENTRY           g_ReceivedUdpPacketList;

static BOOLEAN       g_bAddressOpen = FALSE;
static KS_ADDRESS    g_KS_Address;
static TA_IP_ADDRESS g_LocalAddress;

//
// Local Procedure Prototypes
//
VOID
UDPS_SendDatagramCompletion(
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
    );

#ifdef USE_RECEIVE_EVENT_HANDLER

NTSTATUS
UDPS_TransferDataCallback(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp,
   IN PVOID pContext
   );

#else

VOID
UDPS_ReceiveDatagramCompletion(
   PVOID UserCompletionContext,
   PIO_STATUS_BLOCK IoStatusBlock,
   ULONG Reserved
   );

#endif // USE_RECEIVE_EVENT_HANDLER

/////////////////////////////////////////////////////////////////////////////
//                       U T I L I T Y  R O U T I N E S                    //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// UDPS_FreePacketAndBuffers
//
// Purpose
// Free the NDIS_PACKET and associated NDIS_BUFFER's associated with the
// specified UDPS_PACKET.
//
// Parameters
//
// Return Value
// 
// Remarks
//

VOID
UDPS_FreePacketAndBuffers(
   PUDPS_PACKET pUDPS_Packet
   )
{
   ULONG          nDataSize, nBufferCount;
   PNDIS_BUFFER   pNdisBuffer;

   //
   // Sanity Check On Arguments
   //
   ASSERT( pUDPS_Packet );

   if( !pUDPS_Packet )
   {
      return;
   }

   //
   // Verify The Packet Signature
   //
   ASSERT( pUDPS_Packet->Reserved.m_Signature == UDPS_PACKET_SIGN );

   if( pUDPS_Packet->Reserved.m_Signature != UDPS_PACKET_SIGN )
   {
#if DBG
      DbgPrint( "UDPS_FreePacketAndBuffers: Invalid Packet Signature\n" );
#endif

      return;
   }

   //
   // Invalidate The Signature
   //
   pUDPS_Packet->Reserved.m_Signature = 0;      // Zap The Signature

   //
   // Recycle The Packet
   //
   KS_FreePacketAndBuffers( (PNDIS_PACKET )pUDPS_Packet );
}



/////////////////////////////////////////////////////////////////////////////
//                          S E N D  R O U T I N E S                       //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// UDPS_EchoUdpPackets
//
// Purpose
// Call to handle packets queued on the ReceivedUdpPacketList.
//
// Parameters
//
// Return Value
// 
// Remarks
//

VOID
UDPS_EchoUdpPackets( BOOLEAN bShutdown )
{
   //
   // Echo UDP Packets
   //
   while( !IsListEmpty( &g_ReceivedUdpPacketList ) )
   {
      PLIST_ENTRY    linkage;
      PUDPS_PACKET   pUDPS_Packet;
      PNDIS_BUFFER   pNdisBuffer = NULL;
      ULONG          nDataSize, nByteCount, nBufferCount;
      TDI_STATUS     nTdiStatus = STATUS_SUCCESS;

      //
      // Find The Oldest Packet
      //
      linkage = RemoveHeadList( &g_ReceivedUdpPacketList );

      pUDPS_Packet = CONTAINING_RECORD(
                     linkage,
                     UDPS_PACKET,
                     Reserved.m_ListElement
                     );

      if( bShutdown )
      {
         //
         // Recycle The Packet And Buffers
         //
         UDPS_FreePacketAndBuffers( pUDPS_Packet );

         continue;
      }

      //
      // Query The NDIS_PACKET
      // ---------------------
      // NdisQueryPacket is called to locate the NDIS_BUFFER to be passed to
      // TdiSend().
      //
      NdisQueryPacket(
         (PNDIS_PACKET )pUDPS_Packet,
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
         UDPS_FreePacketAndBuffers( pUDPS_Packet );

         return;
      }

      nTdiStatus = KS_SendDatagramOnAddress(
                        &g_KS_Address,
                        NULL,          // User Completion Event
                        UDPS_SendDatagramCompletion,  // User Completion Routine
                        pUDPS_Packet,   // User Completion Context
                        &pUDPS_Packet->Reserved.m_PacketIoStatus,
                        pNdisBuffer,   // MdlAddress
                        &pUDPS_Packet->Reserved.m_RemoteConnectionInfo
                        );

      if( !NT_SUCCESS( nTdiStatus ) )
      {
         break;
      }
   }
}


/////////////////////////////////////////////////////////////////////////////
//// UDPS_SendDatagramCompletion
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
UDPS_SendDatagramCompletion(
   PVOID UserCompletionContext,
   PIO_STATUS_BLOCK IoStatusBlock,
   ULONG Reserved
   )
{
   PUDPS_PACKET      pUDPS_Packet;
   TDI_STATUS        nTdiStatus;
   PNDIS_BUFFER      pNdisBuffer;
   ULONG             nDataSize, nBufferCount;
   NTSTATUS          nFinalStatus = IoStatusBlock->Status;
   ULONG             nByteCount = IoStatusBlock->Information;

#if DBG
   DbgPrint( "UDPS_SendDatagramCompletion: FinalStatus: %d; BytesSent: %d\n", nFinalStatus, nByteCount );
#endif

   pUDPS_Packet = (PUDPS_PACKET )UserCompletionContext;

   //
   // Sanity Check On Passed Parameters
   //
   ASSERT( pUDPS_Packet );

   if( !pUDPS_Packet )
   {
      return;
   }

   //
   // Verify The Packet Signature
   //
   ASSERT( pUDPS_Packet->Reserved.m_Signature == UDPS_PACKET_SIGN );

   if( pUDPS_Packet->Reserved.m_Signature != UDPS_PACKET_SIGN )
   {
#if DBG
      DbgPrint( "UDPS_SendDatagramCompletion: Invalid Packet Signature\n" );
#endif

      return;
   }

   ASSERT( NT_SUCCESS( nFinalStatus ) );

   if( !NT_SUCCESS( nFinalStatus ) )
   {
      // ATTENTION!!! Update Statistics???
   }

   //
   // Possibly Send Another Packet
   //
   UDPS_EchoUdpPackets( FALSE );

   //
   // Query The NDIS_PACKET
   //
   NdisQueryPacket(
      (PNDIS_PACKET )pUDPS_Packet,
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
      UDPS_FreePacketAndBuffers( pUDPS_Packet );

      return;
   }

   //
   // Reset pNdisBuffer->Length
   // -------------------------
   // When the packet was received on this NDIS_BUFFER, the Length field
   // was adjusted to the amount of data copied into the buffer. Here
   // the Length field is reset to indicate the size of the buffer.
   //
   NdisAdjustBufferLength( pNdisBuffer, UDPS_BUFFER_SIZE );

#ifdef USE_RECEIVE_EVENT_HANDLER

   //
   // Recycle The Packet And Buffers
   //
   UDPS_FreePacketAndBuffers( pUDPS_Packet );

#else

   //
   // Start Another Receive On Same Buffer
   //
   KS_ReceiveDatagramOnAddress(
      &g_KS_Address,
      NULL,       // User Completion Event
      UDPS_ReceiveDatagramCompletion,// User Completion Routine
      pUDPS_Packet,   // User Completion Context
      &pUDPS_Packet->Reserved.m_PacketIoStatus,
      pNdisBuffer,   // MdlAddress
      NULL,          // ReceiveDatagramInfo
      &pUDPS_Packet->Reserved.m_RemoteConnectionInfo, // ReturnInfo
      TDI_RECEIVE_NORMAL   // InFlags
      );

#endif // USE_RECEIVE_EVENT_HANDLER

   return;
}


/////////////////////////////////////////////////////////////////////////////
//                      R E C E I V E  R O U T I N E S                     //
/////////////////////////////////////////////////////////////////////////////


#ifdef USE_RECEIVE_EVENT_HANDLER

/////////////////////////////////////////////////////////////////////////////
//// UDPS_TransferDataCallback
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
// has copied into the buffer and then queues the UDPS_Packet in the 
// ReceivedUdpPacketList. It then calls a routine which can send the packet
// back to the originator.
//

NTSTATUS
UDPS_TransferDataCallback(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp,
   IN PVOID pContext
   )
{
   PUDPS_PACKET      pUDPS_Packet;
   PNDIS_BUFFER      pNdisBuffer;
   ULONG               nDataSize, nBufferCount;
   NTSTATUS            nFinalStatus = pIrp->IoStatus.Status;
   ULONG               nByteCount = pIrp->IoStatus.Information;

#if DBG
   DbgPrint( "UDPS_TransferDataCallback: FinalStatus: %d; Bytes Transfered: %d\n",
      nFinalStatus, nByteCount );
#endif

   pUDPS_Packet = (PUDPS_PACKET )pContext;

   //
   // Sanity Checks
   //
   ASSERT( pUDPS_Packet );

   if( !pUDPS_Packet )
   {
      IoFreeIrp( pIrp );

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   //
   // Verify The Packet Signature
   //
   ASSERT( pUDPS_Packet->Reserved.m_Signature == UDPS_PACKET_SIGN );

   if( pUDPS_Packet->Reserved.m_Signature != UDPS_PACKET_SIGN )
   {
#if DBG
      DbgPrint( "UDPS_TransferDataCallback: Invalid Packet Signature\n" );
#endif

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
      UDPS_FreePacketAndBuffers( pUDPS_Packet );

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
      (PNDIS_PACKET )pUDPS_Packet,
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
      UDPS_FreePacketAndBuffers( pUDPS_Packet );

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
   // Put The Packet In The Received UDP PacketList
   //
   InsertTailList(
      &g_ReceivedUdpPacketList,
      &pUDPS_Packet->Reserved.m_ListElement
      );

   //
   // Possibly Send Another Packet
   //
   UDPS_EchoUdpPackets( FALSE );

   return( STATUS_MORE_PROCESSING_REQUIRED );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPS_ReceiveDatagramEventHandler
//
// Purpose
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
UDPS_ReceiveDatagramEventHandler(
   IN PVOID TdiEventContext,       // Context From SetEventHandler
   IN LONG SourceAddressLength,    // length of the originator of the datagram
   IN PVOID SourceAddress,         // string describing the originator of the datagram
   IN LONG OptionsLength,          // options for the receive
   IN PVOID Options,               //
   IN ULONG ReceiveDatagramFlags,  //
   IN ULONG BytesIndicated,        // number of bytes in this indication
   IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
   OUT ULONG *BytesTaken,          // number of bytes used by indication routine
   IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
   OUT PIRP *IoRequestPacket       // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   )
{
   PUDPS_PACKET         pUDPS_Packet;
   NDIS_STATUS          nNdisStatus;
   PNDIS_BUFFER         pNdisBuffer;
   PDEVICE_OBJECT       pUDPDeviceObject;
   PTRANSPORT_ADDRESS   pTransAddr = (PTRANSPORT_ADDRESS )SourceAddress;
   BOOLEAN              bIsCompleteTsdu = FALSE;

#if DBG
   DbgPrint( "UDPS_ReceiveDatagramEventHandler Entry...\n" );
   DbgPrint( "BytesIndicated: %d, BytesAvailable: %d; ReceiveFlags: 0x%X\n",
      BytesIndicated, BytesAvailable, ReceiveDatagramFlags );

   DEBUG_DumpTransportAddress( pTransAddr );
#endif

   //
   // Determine Whether Tsdu Contains A Full TSDU
   // -------------------------------------------
   // We could check (ReceiveDatagramFlags & TDI_RECEIVE_ENTIRE_MESSAGE).
   // However, checking (BytesIndicated == BytesAvailable) seems more
   // reliable.
   //
   if( BytesIndicated == BytesAvailable )
   {
      bIsCompleteTsdu = TRUE;
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
      &(PNDIS_PACKET )pUDPS_Packet,
      g_hPacketPool
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pUDPS_Packet )
   {
      // ATTENTION!!! Update Statistics???

#if DBG
      DbgPrint( "UDPS_ReceiveDatagramEventHandler Could Not Allocate Packet\n" );
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
   pUDPS_Packet->Reserved.m_Signature = UDPS_PACKET_SIGN;

   //
   // Save Remote Address
   //
   NdisMoveMemory(
      &pUDPS_Packet->Reserved.m_RemoteAddress,
      SourceAddress,
      sizeof( TA_IP_ADDRESS )
      );

   //
   // Save Remote Connection Info In Packet Reserved Area
   //
   NdisZeroMemory(
      &pUDPS_Packet->Reserved.m_RemoteConnectionInfo,
      sizeof( TDI_CONNECTION_INFORMATION )
      );

   pUDPS_Packet->Reserved.m_RemoteConnectionInfo.RemoteAddress = &pUDPS_Packet->Reserved.m_RemoteAddress;
   pUDPS_Packet->Reserved.m_RemoteConnectionInfo.RemoteAddressLength = sizeof( TA_IP_ADDRESS );

   //
   // Save Bytes Taken In Internal Buffer
   // -----------------------------------
   // In this sample the data is saved in an internal buffer that is a
   // field in the UDPS_PACKET structure.
   //
   // ATTENTION!!! Note that *BytesTaken is set in the following statement.
   //
   if( BytesIndicated <= UDPS_BUFFER_SIZE )
   {
      *BytesTaken = BytesIndicated;
   }
   else
   {
      *BytesTaken = UDPS_BUFFER_SIZE;
   }

   NdisMoveMemory(
      pUDPS_Packet->Reserved.m_DataBuffer,
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
      pUDPS_Packet->Reserved.m_DataBuffer,   // Private Buffer
      *BytesTaken
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pNdisBuffer )
   {
      // ATTENTION!!! Update Statistics???

      NdisFreePacket( (PNDIS_PACKET )pUDPS_Packet );

#if DBG
      DbgPrint( "UDPS_ReceiveDatagramEventHandler Could Not Allocate Buffer\n" );
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

   NdisChainBufferAtFront( (PNDIS_PACKET )pUDPS_Packet, pNdisBuffer );

   //
   // Put The Packet In The Received UDP PacketList
   //
   InsertTailList(
      &g_ReceivedUdpPacketList,
      &pUDPS_Packet->Reserved.m_ListElement
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
      UDPS_EchoUdpPackets( FALSE );

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
      &(PNDIS_PACKET )pUDPS_Packet,
      g_hPacketPool
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pUDPS_Packet )
   {
      // ATTENTION!!! Update Statistics???

#if DBG
      DbgPrint( "UDPS_ReceiveDatagramEventHandler Could Not Allocate Packet\n" );
#endif

      return( STATUS_SUCCESS );
   }

   //
   // Initialize The Packet Signature
   //
   pUDPS_Packet->Reserved.m_Signature = UDPS_PACKET_SIGN;

   //
   // Save Remote Address
   //
   NdisMoveMemory(
      &pUDPS_Packet->Reserved.m_RemoteAddress,
      SourceAddress,
      sizeof( TA_IP_ADDRESS )
      );

   //
   // Save Remote Connection Info In Packet Reserved Area
   //
   NdisZeroMemory(
      &pUDPS_Packet->Reserved.m_RemoteConnectionInfo,
      sizeof( TDI_CONNECTION_INFORMATION )
      );

   pUDPS_Packet->Reserved.m_RemoteConnectionInfo.RemoteAddress = &pUDPS_Packet->Reserved.m_RemoteAddress;
   pUDPS_Packet->Reserved.m_RemoteConnectionInfo.RemoteAddressLength = sizeof( TA_IP_ADDRESS );

   //
   // Allocate An NDIS Buffer Descriptor For The Receive Data
   //
   NdisAllocateBuffer(
      &nNdisStatus,
      &pNdisBuffer,
      g_hBufferPool,
      pUDPS_Packet->Reserved.m_DataBuffer,   // Private Buffer
      BytesAvailable - *BytesTaken
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pNdisBuffer )
   {
      // ATTENTION!!! Update Statistics???

      NdisFreePacket( (PNDIS_PACKET )pUDPS_Packet );

#if DBG
      DbgPrint( "UDPS_ReceiveDatagramEventHandler Could Not Allocate Buffer\n" );
#endif

      return( STATUS_SUCCESS );
   }

   NdisChainBufferAtFront( (PNDIS_PACKET )pUDPS_Packet, pNdisBuffer );

   //
   // Allocate Resources To Call TDI To Transfer Received Data
   //
   pUDPDeviceObject = IoGetRelatedDeviceObject(
                        g_KS_Address.m_pFileObject
                        );

   pUDPS_Packet->Reserved.m_pReceiveIrp = IoAllocateIrp(
                                 pUDPDeviceObject->StackSize,
                                 FALSE
                                 );

   ASSERT( pUDPS_Packet->Reserved.m_pReceiveIrp );

   if( !pUDPS_Packet->Reserved.m_pReceiveIrp )
   {
      UDPS_FreePacketAndBuffers( pUDPS_Packet );

      return( STATUS_SUCCESS );
   }

   TdiBuildReceiveDatagram(
      pUDPS_Packet->Reserved.m_pReceiveIrp, // IRP
      pUDPDeviceObject,                // Pointer To TDI Device Object
      g_KS_Address.m_pFileObject,      // Address File Object
      UDPS_TransferDataCallback,       // CompletionRoutine
      pUDPS_Packet,                    // Context
      pNdisBuffer,                     // MdlAddress
      BytesAvailable - *BytesTaken,    // ReceiveLen
      &pUDPS_Packet->Reserved.m_RemoteConnectionInfo, // ReceiveDatagramInfo
      NULL,                            // ReturnInfo
      TDI_RECEIVE_NORMAL               // InFlags
      );

   //
   // Make the next stack location current. Normally IoCallDriver would
   // do this, but for this IRP it has been bypassed.
   //
   IoSetNextIrpStackLocation( pUDPS_Packet->Reserved.m_pReceiveIrp );

   //
   // Tell TDI To Transfer The Data
   //
   *IoRequestPacket = pUDPS_Packet->Reserved.m_pReceiveIrp;

   return( STATUS_MORE_PROCESSING_REQUIRED );
}

#else

/////////////////////////////////////////////////////////////////////////////
//// UDPS_ReceiveDatagramCompletion
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
UDPS_ReceiveDatagramCompletion(
   PVOID UserCompletionContext,
   PIO_STATUS_BLOCK IoStatusBlock,
   ULONG Reserved
   )
{
   PUDPS_PACKET      pUDPS_Packet;
   PNDIS_BUFFER      pNdisBuffer;
   ULONG             nDataSize, nBufferCount;
   NTSTATUS          nFinalStatus = IoStatusBlock->Status;
   ULONG             nByteCount = IoStatusBlock->Information;

#if DBG
   DbgPrint( "UDPS_ReceiveDatagramCompletion: FinalStatus: 0x%8.8x; Bytes Transfered: %d\n",
      nFinalStatus, nByteCount );
#endif

   pUDPS_Packet = (PUDPS_PACKET )UserCompletionContext;

   //
   // Sanity Check On Passed Parameters
   //
   ASSERT( pUDPS_Packet );

   if( !pUDPS_Packet )
   {
      return;
   }

   //
   // Verify The Packet Signature
   //
   ASSERT( pUDPS_Packet->Reserved.m_Signature == UDPS_PACKET_SIGN );

   if( pUDPS_Packet->Reserved.m_Signature != UDPS_PACKET_SIGN )
   {
#if DBG
      DbgPrint( "UDPS_ReceiveDatagramCompletion: Invalid Packet Signature\n" );
#endif

      return;
   }

   DEBUG_DumpTransportAddress(
      (PTRANSPORT_ADDRESS )&pUDPS_Packet->Reserved.m_RemoteAddress
      );

   //
   // Handle Transfer Failure
   //
   if( !NT_SUCCESS( nFinalStatus ) )
   {
      //
      // Recycle The Packet And Buffers
      //
      UDPS_FreePacketAndBuffers( pUDPS_Packet );

      // ATTENTION!!! Update Statistics???

      return;
   }

   // ATTENTION!!! Update Statistics???

   //
   // Query The NDIS_PACKET
   //
   NdisQueryPacket(
      (PNDIS_PACKET )pUDPS_Packet,
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
      UDPS_FreePacketAndBuffers( pUDPS_Packet );

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
   // Put The Packet In The Received UDP PacketList
   //
   InsertTailList(
      &g_ReceivedUdpPacketList,
      &pUDPS_Packet->Reserved.m_ListElement
      );

   //
   // Possibly Send Another Packet
   //
   UDPS_EchoUdpPackets( FALSE);

   return;
}

#endif // USE_RECEIVE_EVENT_HANDLER


/////////////////////////////////////////////////////////////////////////////
//// UDPS_Startup
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
UDPS_Startup( PDEVICE_OBJECT pDeviceObject )
{
   NTSTATUS          nTdiStatus;
   NDIS_STATUS       nNdisStatus;
   TA_IP_ADDRESS     ECHOAddress;
   int               i;
   PUDPS_PACKET      pUDPS_Packet;
   PNDIS_BUFFER      pNdisBuffer;

#if DBG
   DbgPrint( "UDPS_Startup Entry...\n" );
   DbgPrint( "pDeviceObject: 0x%X\n", pDeviceObject );
#endif

   g_bAddressOpen = FALSE;

   //
   // Initialize The Received UDP Packet List
   //
   InitializeListHead( &g_ReceivedUdpPacketList );

   //
   // Allocate The Packet Pool
   //
   NdisAllocatePacketPool(
      &nNdisStatus,
      &g_hPacketPool,
      UDPS_PACKET_POOL_SIZE,
      sizeof( UDPS_PACKET_RESERVED )
      );

   ASSERT( NT_SUCCESS( nNdisStatus ) );

   if( !NT_SUCCESS( nNdisStatus ) )
   {
#if DBG
      DbgPrint( "UDPS_Startup: Failed To Allocate Packet Pool\n" );
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
      UDPS_BUFFER_POOL_SIZE
      );

   ASSERT( NT_SUCCESS( nNdisStatus ) );

   if( !NT_SUCCESS( nNdisStatus ) )
   {
#if DBG
      DbgPrint( "UDPS_Startup: Failed To Allocate Buffer Pool\n" );
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
      &g_LocalAddress,
      INADDR_ANY,                   // Any Local Address
      KS_htons( IPPORT_ECHO )       // ECHO Port
      );

   //
   // Open The Local TDI Address Object
   //
   nTdiStatus = STATUS_REQUEST_ABORTED;

   nTdiStatus = KS_OpenTransportAddress(
                  UDP_DEVICE_NAME_W,
                  (PTRANSPORT_ADDRESS )&g_LocalAddress,
                  &g_KS_Address
                  );

   ASSERT( NT_SUCCESS( nTdiStatus ) );

   if ( !NT_SUCCESS( nTdiStatus ) )
   {
      return( nTdiStatus );
   }

   g_bAddressOpen = TRUE;

   //
   // Setup Event Handlers On The Server Address Object
   //
   nTdiStatus = KS_SetEventHandlers(
                  &g_KS_Address,
                  pDeviceObject, // Event Context
                  NULL,          // ConnectEventHandler
                  NULL,          // DisconnectEventHandler
                  NULL,          // ErrorEventHandler,
                  NULL,          // ReceiveEventHandler
#ifdef USE_RECEIVE_EVENT_HANDLER
                  UDPS_ReceiveDatagramEventHandler,  // ReceiveDatagramEventHandler
#else
                  NULL,          // ReceiveDatagramEventHandler
#endif
                  NULL           // ReceiveExpeditedEventHandler
                  );

   ASSERT( NT_SUCCESS( nTdiStatus ) );

   if( !NT_SUCCESS( nTdiStatus ) )
      return( nTdiStatus );


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
      &(PNDIS_PACKET )pUDPS_Packet,
      g_hPacketPool
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pUDPS_Packet )
   {
      // ATTENTION!!! Update Statistics???

#if DBG
      DbgPrint( "UDPS_Startup Could Not Allocate Packet\n" );
#endif

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   //
   // Initialize The Packet Signature
   //
   pUDPS_Packet->Reserved.m_Signature = UDPS_PACKET_SIGN;

   //
   // Initialize The Space For Remote Address To Be Returned
   //
   KS_InitIPAddress(
      &pUDPS_Packet->Reserved.m_RemoteAddress,
      INADDR_ANY,                   // Any Local Address
      0                             // Any Port
      );

   //
   // Setup Return Connection Info In Packet Reserved Area
   //
   NdisZeroMemory(
      &pUDPS_Packet->Reserved.m_RemoteConnectionInfo,
      sizeof( TDI_CONNECTION_INFORMATION )
      );

   pUDPS_Packet->Reserved.m_RemoteConnectionInfo.RemoteAddress = &pUDPS_Packet->Reserved.m_RemoteAddress;
   pUDPS_Packet->Reserved.m_RemoteConnectionInfo.RemoteAddressLength = sizeof( TA_IP_ADDRESS );

   //
   // Allocate An NDIS Buffer Descriptor For The Receive Data
   //
   NdisAllocateBuffer(
      &nNdisStatus,
      &pNdisBuffer,
      g_hBufferPool,
      pUDPS_Packet->Reserved.m_DataBuffer,   // Private Buffer
      UDPS_BUFFER_SIZE                       // Private Buffer's Size
      );

   if( nNdisStatus != NDIS_STATUS_SUCCESS || !pNdisBuffer )
   {
      // ATTENTION!!! Update Statistics???

      NdisFreePacket( (PNDIS_PACKET )pUDPS_Packet );

#if DBG
      DbgPrint( "UDPS_Startup Could Not Allocate Buffer\n" );
#endif

      return( STATUS_MORE_PROCESSING_REQUIRED );
   }

   NdisChainBufferAtFront( (PNDIS_PACKET )pUDPS_Packet, pNdisBuffer );

   //
   // Start A Receive On The Connection
   //
   KS_ReceiveDatagramOnAddress(
      &g_KS_Address,
      NULL,       // User Completion Event
      UDPS_ReceiveDatagramCompletion, // User Completion Routine
      pUDPS_Packet,   // User Completion Context
      &pUDPS_Packet->Reserved.m_PacketIoStatus,
      pNdisBuffer,   // MdlAddress
      NULL,          // ReceiveDatagramInfo
      &pUDPS_Packet->Reserved.m_RemoteConnectionInfo, // ReturnInfo
      TDI_RECEIVE_NORMAL   // InFlags
      );

#endif // USE_RECEIVE_EVENT_HANDLER

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPS_Shutdown
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

VOID UDPS_Shutdown( PDEVICE_OBJECT pDeviceObject )
{
   TDI_STATUS        nTdiStatus;

#if DBG
   DbgPrint( "UDPS_Shutdown Entry...\n" );
#endif

   //
   // Empty the ReceivedUdpPacketList
   // ----------------------------
   // UDPS_EchoUdpPackets will empty the list if bShutdown is TRUE.
   //
   UDPS_EchoUdpPackets( TRUE );

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

   return;
}

