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

#endif // __TCPRCV_H__

