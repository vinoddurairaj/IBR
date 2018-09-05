/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __UDPSEND_H__
#define __UDPSEND_H__

// Copyright And Configuration Management ----------------------------------
//
//           Header For UDP Send Function Filters - UDPSend.h
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
struct _UDPSendReq
{
   LIST_ENTRY        tsr_q;      // Linkage

   ULONG             tsr_connid;
//   TDI_CALLBACK      tsr_rtn;     // Completion routine.
   PVOID             tsr_context; // User context.

   USHORT            tsr_flags;  // Flags for this send.
   ULONG             tsr_size;   // Total size of buffer chain.
   PNDIS_BUFFER      tsr_buffer; // Pointer to useable NDIS buffer chain.
}
   UDPSendReq;


extern LIST_ENTRY FreeDGSendReqList;

NTSTATUS
TDIH_TdiSendDatagram(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

#endif // __UDPSEND_H__

