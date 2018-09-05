/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __TDIADDR_H__
#define __TDIADDR_H__

// Copyright And Configuration Management ----------------------------------
//
//           Header For TDI Address Function Filters - Addr.h
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

// This is the data structure used to track Address Objects. The VxD
// allocates and initializes one of these in its hook for OpenAddress.

typedef
struct _AddrObj
{
	LIST_ENTRY              ao_q;		         // Linkage

   PTDIH_DeviceExtension   ao_DeviceExtension;

   PFILE_OBJECT            ao_FileObject;    // File Object Pointer

   ULONG                   ao_prot;          // Protocol for this AO

   LIST_ENTRY              ao_tc_q;          // Queue of TCP connections

   char                    ao_ProcessName[ NT_PROCNAMELEN ];

   //
   // Event Handlers
   //
   PTDI_IND_CONNECT           ao_connect;       // Connect event handle
   PVOID                      ao_conncontext;   // Receive DG context
   PTDI_IND_DISCONNECT        ao_disconnect;    // Disconnect event routine
   PVOID                      ao_disconncontext;// Disconnect event context
   PTDI_IND_ERROR             ao_error;         // Error event routine
   PVOID                      ao_errcontext;    // Error event context
   PTDI_IND_RECEIVE           ao_rcv;           // Receive event handler
   PVOID                      ao_rcvcontext;    // Receive context
   PTDI_IND_CHAINED_RECEIVE   ao_chainedrcv;    // Chained receive event handler
   PVOID                      ao_chainedrcvcontext;// Chained receive context
   PTDI_IND_RECEIVE_DATAGRAM  ao_rcvdg;         // Receive DG event handler
   PVOID                      ao_rcvdgcontext;  // Receive DG context
   PTDI_IND_CHAINED_RECEIVE_DATAGRAM  ao_chainedrcvdg;  // Chained receive DG event handler
   PVOID                      ao_chainedrcvdgcontext;  // Chained receive DG context
   PTDI_IND_RECEIVE_EXPEDITED ao_exprcv;        // Expedited receive event handler
   PVOID                      ao_exprcvcontext; // Expedited receive context

   LIST_ENTRY     ao_senddg_q;     // Pending DG Sends
   LIST_ENTRY     ao_rcvdg_q;      // Pending DG Receives

   //
   // Fields For Internally Created Address Objects
   //
   BOOLEAN        aox_valid;     // TRUE if AddrCreateExInfo was provided
   ULONG          aox_data1;
   PVOID          aox_data2;
}
   AddrObj;

typedef
struct _AddrObjExInfo
{
	LIST_ENTRY  aox_q;		      // Linkage
   PETHREAD    aox_thread;
   ULONG       aox_data1;
   PVOID       aox_data2;
   AddrObj     *aox_ao;          // Returned
}
   AddrObjExInfo, *PAddrObjExInfo;

//
// The Address Object Lists
//
extern LIST_ENTRY FreeAddrObjList;
extern LIST_ENTRY OpenAddrObjList;
extern LIST_ENTRY AddrObjExInfoList;


AddrObj *
TDIH_GetAddrObjFromFileObject(
   PFILE_OBJECT   FileObject
   );

NTSTATUS
TDIH_TdiOpenAddress(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

NTSTATUS
TDIH_TdiCloseAddress(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

NTSTATUS
TDIH_TdiSetEvent(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

#endif // __TDIADDR_H__

