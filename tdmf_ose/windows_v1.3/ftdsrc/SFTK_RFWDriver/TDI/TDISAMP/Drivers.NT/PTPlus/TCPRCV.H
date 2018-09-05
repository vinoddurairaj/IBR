/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __TCPRCV_H__
#define __TCPRCV_H__

// Copyright And Configuration Management ----------------------------------
//
//          Header For TCP Receive Function Filters - TCPRcv.h
//         Transport Data Interface (TDI) Filter For Windows NT
//
//     Copyright (c) 2000-2001 Printing Communications Associates, Inc.
//                               - PCAUSA -
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
//
// End ---------------------------------------------------------------------

#define TDIH_PACKET_BUF_SIZE 8192

typedef
struct _TDIH_PACKET
{
   LIST_ENTRY     ListElement;

   AddrObj        *pAddrObj;
   TCPConn        *pTCPConn;

   NTSTATUS       IoStatus;

   uint           Position;      // Current logical offset into packet.
   void           *Data;         // Current pointer into packet data.
   uint           ContigSize;    // Amount of contiguous data remaining.
   uint           BufferCount;   // Number Of Buffers At Head

   ULONG          Flags;
   USHORT         ShortFlags;

   PNDIS_BUFFER   Head;          // first buffer in the chain
   PNDIS_BUFFER   Tail;          // last buffer in the chain

   UCHAR          PacketBuffer[ TDIH_PACKET_BUF_SIZE ];  // OURS!!!
}
   TDIH_PACKET, *PTDIH_PACKET;

VOID
TDIH_FreePacket( PTDIH_PACKET pTDIH_Packet );

NTSTATUS
TDIH_TdiStartPrivateReceive(
   TCPConn *pTCPConn
   );

#ifdef ZNEVER

NTSTATUS
TDIH_TdiReceiveEventHandler(
   PVOID TdiEventContext,     // Context From SetEventHandler
   CONNECTION_CONTEXT ConnectionContext,
   ULONG ReceiveFlags,
   ULONG BytesIndicated,
   ULONG BytesAvailable,
   ULONG *BytesTaken,
   PVOID Tsdu,				// pointer describing this TSDU, typically a lump of bytes
   PIRP *IoRequestPacket	// TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   );

#endif // ZNEVER

NTSTATUS
TDIH_TdiChainedReceiveEventHandler(
   PVOID TdiEventContext,     // Context From SetEventHandler
   CONNECTION_CONTEXT ConnectionContext,
   ULONG ReceiveFlags,
   ULONG ReceiveLength,        // length of client data in TSDU
   ULONG StartingOffset,       // offset of start of client data in TSDU
   PMDL  Tsdu,                 // TSDU data chain
   PVOID TsduDescriptor        // for call to TdiReturnChainedReceives
   );

NTSTATUS
TDIH_TdiReceive(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

VOID
TDIH_ReceiveTimerProc(
   PVOID SystemSpecific1,
   PVOID FunctionContext,
   PVOID SystemSpecific2,
   PVOID SystemSpecific3
   );

#endif // __TCPRCV_H__

