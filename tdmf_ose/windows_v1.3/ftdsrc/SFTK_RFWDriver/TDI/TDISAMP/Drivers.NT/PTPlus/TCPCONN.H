/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __TCPCONN_H__
#define __TCPCONN_H__

// Copyright And Configuration Management ----------------------------------
//
//        Header For TCP Connection Function Filters - TCPConn.h
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

typedef
struct _TCPConn
{
   LIST_ENTRY     tc_q;       // Linkage

   PTDIH_DeviceExtension   tc_DeviceExtension;

   PFILE_OBJECT            tc_FileObject;    // File Object Pointer

	AddrObj        *tc_ao;     // Back pointer to AddrObj
   PVOID          tc_context; // As Passed To TdiOpenConnection

   //
   // Remote Peer Address
   // -------------------
   // Captured at:
   //    TDIH_TdiConnect
   //
   //
   TA_IP_ADDRESS           tc_RemoteAddress;

//   RequestCompleteRoutine tc_rtn;        // Close Completion routine.
   PVOID          tc_rtncontext; // User context for close completion routine.

   LIST_ENTRY     tc_send_q;     // Pending Sends

   NDIS_SPIN_LOCK tc_rcv_q_lock; // Pending Receives
   LIST_ENTRY     tc_rcv_q;

   uint           TotalSize;     // Total amount of data remaining
                                 // In All Chained TDIH_PACKETs.
   LIST_ENTRY     tc_rcv_pkt_q;  // Pending Receive Requests

   NDIS_TIMER     tc_rx_timer;

   PTDI_IND_RECEIVE           ao_rcv;           // Receive event handler
   PVOID                      ao_rcvcontext;    // Receive context
   PTDI_IND_RECEIVE_EXPEDITED ao_exprcv;        // Expedited receive event handler
   PVOID                      ao_exprcvcontext; // Expedited receive context
}
   TCPConn;


//
// The Connection Context Lists
//
extern LIST_ENTRY FreeTCPConnList;
extern LIST_ENTRY OpenTCPConnList;


TCPConn *
TDIH_GetConnFromFileObject(
   PFILE_OBJECT   FileObject
   );

TCPConn *
TDIH_GetConnFromConnectionContext(
	PVOID ConnectionContext
   );

NTSTATUS
TDIH_TdiOpenConnection(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp,
   PVOID                   ConnectionContext
   );

NTSTATUS
TDIH_TdiCloseConnection(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

NTSTATUS
TDIH_TdiConnectEventHandler(
   IN PVOID TdiEventContext,     // Context From SetEventHandler
   IN LONG RemoteAddressLength,
   IN PVOID RemoteAddress,
   IN LONG UserDataLength,       // Unused for MSTCP
   IN PVOID UserData,            // Unused for MSTCP
   IN LONG OptionsLength,
   IN PVOID Options,
   OUT CONNECTION_CONTEXT *ConnectionContext,
   OUT PIRP *hAcceptIrp
   );

NTSTATUS
TDIH_TdiConnect(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

NTSTATUS
TDIH_TdiListen(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

NTSTATUS
TDIH_TdiDisconnect(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

NTSTATUS
TDIH_TdiAssociateAddress(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

NTSTATUS
TDIH_TdiDisAssociateAddress(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

#endif // __TCPCONN_H__

