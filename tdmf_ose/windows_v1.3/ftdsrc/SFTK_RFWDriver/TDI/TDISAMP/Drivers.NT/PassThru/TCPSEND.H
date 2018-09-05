/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __TCPSEND_H__
#define __TCPSEND_H__

// Copyright And Configuration Management ----------------------------------
//
//           Header For TCP Send Function Filters - TCPSend.h
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
TDIH_TdiSend(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   );

#endif // __TCPSEND_H__

