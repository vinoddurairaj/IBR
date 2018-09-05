/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "TDIINFO.h"
#include "smpletcp.h"

#ifndef __KSTCPEX_H__
#define __KSTCPEX_H__

// Copyright And Configuration Management ----------------------------------
//
//                Header For TCP Extension IOCTLs - KSTcpEx.h
//
//                  PCAUSA TDI Client Samples For Windows NT
//
//      Copyright (c) 2000-2001 Printing Communications Associates, Inc.
//                                - PCAUSA -
//
// Adapted from SMPLETCP.H header file in WSHSMPLE sample the Windows NT 4.0
// DDK. The original SMPLETCP.H header is:
//
//                 Copyright(c) Microsoft Corp., 1990-1996
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
//
// End ---------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
//// TcpBuildSetInfoEx
//
// A MACRO modeled after similar marcos defined in TdiInfo.h. It sets up
// the IRP and IO_STACK_LOCATION for making the IOCTL_TCP_SET_INFORMATION_EX
// call to the TCP device driver.
//
// IOCTL_TCP_SET_INFORMATION_EX uses METHOD_BUFFERED, and this defines which
// IRP and stack pointer fields must be initialized to pass the 
// TCP_REQUEST_SET_INFORMATION_EX buffer to the TCP device driver.
//
// The TCP_REQUEST_SET_INFORMATION_EX structure is passed as the
// InputBuffer (to the driver). The space allocated for the structure must
// be large enough to contain the structure and the set data buffer, which
// begins at the Buffer field.
//
// The OutputBuffer parameter in the DeviceIoControl is not used.
//
#define TcpBuildSetInfoEx(Irp, DevObj, FileObj, CompRoutine, Contxt, Buffer, BufferLen)\
    {                                                                        \
        PIO_STACK_LOCATION _IRPSP;                                           \
        if ( CompRoutine != NULL) {                                          \
            IoSetCompletionRoutine( Irp, CompRoutine, Contxt, TRUE, TRUE, TRUE);\
        } else {                                                             \
            IoSetCompletionRoutine( Irp, NULL, NULL, FALSE, FALSE, FALSE);   \
        }                                                                    \
        _IRPSP = IoGetNextIrpStackLocation (Irp);                            \
        _IRPSP->MajorFunction = IRP_MJ_DEVICE_CONTROL;                       \
        _IRPSP->DeviceObject = DevObj;                                       \
        _IRPSP->FileObject = FileObj;                                        \
        _IRPSP->Parameters.DeviceIoControl.OutputBufferLength = 0;           \
        _IRPSP->Parameters.DeviceIoControl.InputBufferLength = BufferLen;    \
        _IRPSP->Parameters.DeviceIoControl.IoControlCode = IOCTL_TCP_SET_INFORMATION_EX;  \
        Irp->AssociatedIrp.SystemBuffer = Buffer;                            \
    }


/////////////////////////////////////////////////////////////////////////////
//// TcpBuildQueryInfoEx
//
// A MACRO modeled after similar marcos defined in TdiInfo.h. It sets up
// the IRP and IO_STACK_LOCATION for making the IOCTL_TCP_QUERY_INFORMATION_EX
// call to the TCP device driver.
//
// IOCTL_TCP_QUERY_INFORMATION_EX uses METHOD_NEITHER, and this defines which
// IRP and stack pointer fields must be initialized to pass the 
// TCP_REQUEST_QUERY_INFORMATION_EX buffer to the TCP device driver.
//
// The TCP_REQUEST_QUERY_INFORMATION_EX structure is passed as the
// InputBuffer (to the driver).
//
// The return buffer is passed as the OutputBuffer in the DeviceIoControl
// request. This structure is passed as the InputBuffer.
//
// The OutputBuffer parameter in the DeviceIoControl is not used.
//
// AFAIK IOCTL_TCP_QUERY_INFORMATION_EX is _NOT_ the inverse of
// IOCTL_TCP_SET_INFORMATION_EX. That is, you cannot query information
// using the same basic methods used to set information. For example,
// you can use IOCTL_TCP_SET_INFORMATION_EX to set TCP_NODELAY but you
// cannot turn around and use IOCTL_TCP_QUERY_INFORMATION_EX to determine
// the TCP_NODELAY setting. Winsock support of getsockopt() doesn't actually
// call the TDI driver. Instead, it maintains the state of socket options
// in the Winsock Helper DLL and returns the value from the WSH DLL when
// getsockopt() is called.
//
// This macro is NOT exercised in the PCAUSA sample code. It is probable
// that the FileObj is from a Control Channel. It is probable that the
// GetTcpipInterfaceList() function in the NT DDK WSHSMPLE sample can
// provide guidance in exploring the use of IOCTL_TCP_QUERY_INFORMATION_EX.
//

#define TcpBuildQueryInfoEx(Irp, DevObj, FileObj, CompRoutine, Contxt, InPtr, InLen, OutPtr, OutLen)\
    {                                                                        \
        PIO_STACK_LOCATION _IRPSP;                                           \
        if ( CompRoutine != NULL) {                                          \
            IoSetCompletionRoutine( Irp, CompRoutine, Contxt, TRUE, TRUE, TRUE);\
        } else {                                                             \
            IoSetCompletionRoutine( Irp, NULL, NULL, FALSE, FALSE, FALSE);   \
        }                                                                    \
        _IRPSP = IoGetNextIrpStackLocation (Irp);                            \
        _IRPSP->MajorFunction = IRP_MJ_DEVICE_CONTROL;                       \
        _IRPSP->DeviceObject = DevObj;                                       \
        _IRPSP->FileObject = FileObj;                                        \
        _IRPSP->Parameters.DeviceIoControl.OutputBufferLength = OutLen;           \
        _IRPSP->Parameters.DeviceIoControl.InputBufferLength = InLen;    \
        _IRPSP->Parameters.DeviceIoControl.IoControlCode = IOCTL_TCP_QUERY_INFORMATION_EX;  \
        _IRPSP->Parameters.DeviceIoControl.Type3InputBuffer = InPtr;    \
        Irp->UserBuffer = OutPtr;                            \
    }

#endif // __KSTCPEX_H__

