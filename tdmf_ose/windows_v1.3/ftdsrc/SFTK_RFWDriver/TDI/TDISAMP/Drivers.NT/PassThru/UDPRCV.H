/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __UDPRCV_H__
#define __UDPRCV_H__

// Copyright And Configuration Management ----------------------------------
//
//          Header For UDP Receive Function Filters - UDPRcv.h
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

NTSTATUS
TDIH_TdiReceiveDGEventHandler(
   PVOID TdiEventContext,        // Context From SetEventHandler
   LONG SourceAddressLength,     // length of the originator of the datagram
   PVOID SourceAddress,          // string describing the originator of the datagram
   LONG OptionsLength,           // options for the receive
   PVOID Options,                //
   ULONG ReceiveDatagramFlags,   //
   ULONG BytesIndicated,         // number of bytes in this indication
   ULONG BytesAvailable,         // number of bytes in complete Tsdu
   ULONG *BytesTaken,            // number of bytes used by indication routine
   PVOID Tsdu,                   // pointer describing this TSDU, typically a lump of bytes
   PIRP *IoRequestPacket         // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   );

NTSTATUS
TDIH_TdiChainedReceiveDGEventHandler(
   PVOID TdiEventContext,        // Context From SetEventHandler
   LONG SourceAddressLength,     // length of the originator of the datagram
   PVOID SourceAddress,          // string describing the originator of the datagram
   LONG OptionsLength,           // options for the receive
   PVOID Options,                //
   ULONG ReceiveDatagramFlags,   //
   ULONG ReceiveDatagramLength,  // length of client data in TSDU
   ULONG StartingOffset,         // offset of start of client data in TSDU
   PMDL  Tsdu,                   // TSDU data chain
   PVOID TsduDescriptor          // for call to TdiReturnChainedReceives
   );

NTSTATUS
TDIH_TdiReceiveDatagram(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

#endif // __UDPRCV_H__

