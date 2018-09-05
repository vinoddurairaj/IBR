#ifndef _PCATDIH_H_
#define _PCATDIH_H_

/**********************************************************************************
*
* The following macros make life a little easier
*
**********************************************************************************/

// try-finally simulation
#define try_return(S)   { S; goto try_exit; }
#define try_return1(S)  { S; goto try_exit1; }
#define try_return2(S)  { S; goto try_exit2; }

// the following macro allows us to increment a large integer value atomically.
// we expect an unsigned long to be supplied as the increment value.
// a spin lock should be passed-in to synchronize operations
#define  UTIL_IncrementLargeInteger(LargeIntegerOp, ULongIncrement, pSpinLock)   {         \
   KIRQL            OldIrql;                                                                     \
   KeAcquireSpinLock(pSpinLock, &OldIrql);                                                   \
   RtlLargeIntegerAdd((LargeIntegerOp),(RtlConvertUlongToLargeInteger((ULongIncrement))));   \
   KeReleaseSpinLock(pSpinLock, OldIrql);                                                   \
}

// the following macro allows us to decrement a large integer value atomically.
// we expect an unsigned long to be supplied as the decrement value.
// a spin lock should be passed-in to synchronize operations
#define UTIL_DecrementLargeInteger(LargeIntegerOp, ULongIncrement, pSpinLock)   {         \
   KIRQL            OldIrql;                                                                     \
   KeAcquireSpinLock(pSpinLock, &OldIrql);                                                   \
   RtlLargeIntegerSubtract((LargeIntegerOp),(RtlConvertUlongToLargeInteger((ULongIncrement))));   \
   KeReleaseSpinLock(pSpinLock, OldIrql);                                                   \
}

// the following macro allows us to check if the large integer value is zero,
// atomically. Note that I have added (for convenience) a check to ensure that
// the value is non-negative.
#define UTIL_IsLargeIntegerZero(ReturnValue, LargeIntegerOp, pSpinLock)   {               \
   KIRQL            OldIrql;                                                                     \
   KeAcquireSpinLock(pSpinLock, &OldIrql);                                                   \
   ASSERT(RtlLargeIntegerGreaterOrEqualToZero((LargeIntegerOp)));                              \
   ReturnValue = RtlLargeIntegerEqualToZero((LargeIntegerOp));                                 \
   KeReleaseSpinLock(pSpinLock, OldIrql);                                                   \
}

#if DBG
#define   UTIL_BreakPoint()   DbgBreakPoint()
#else
#define   UTIL_BreakPoint()
#endif


//
// "Borrow" Windows 2000 DDK IoCopyCurrentIrpStackLocationToNext MACRO
//
#ifndef IoCopyCurrentIrpStackLocationToNext
#define IoCopyCurrentIrpStackLocationToNext( Irp ) { \
    PIO_STACK_LOCATION irpSp; \
    PIO_STACK_LOCATION nextIrpSp; \
    irpSp = IoGetCurrentIrpStackLocation( (Irp) ); \
    nextIrpSp = IoGetNextIrpStackLocation( (Irp) ); \
    RtlCopyMemory( nextIrpSp, irpSp, FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine)); \
    nextIrpSp->Control = 0; }
#endif // IoCopyCurrentIrpStackLocationToNext


//
// TCP/UDP/RawIP Device Names
//
#define DD_IP_DEVICE_NAME       L"\\Device\\Ip"
#define DD_TCP_DEVICE_NAME      L"\\Device\\Tcp"
#define DD_UDP_DEVICE_NAME      L"\\Device\\Udp"
#define DD_RAW_IP_DEVICE_NAME   L"\\Device\\RawIp"
#define DD_AFD_DEVICE_NAME      L"\\Device\\Afd"
#define DD_W32API_DEVICE_NAME   L"\\Device\\PCATDIH"

//
// TCP/UDP/RawIP Filter Device Names
//
#define TDIH_IP_DEVICE_NAME         L"\\Device\\PcaIpFilter"
#define TDIH_TCP_DEVICE_NAME        L"\\Device\\PcaTcpFilter"
#define TDIH_UDP_DEVICE_NAME        L"\\Device\\PcaUdpFilter"
#define TDIH_RAW_IP_DEVICE_NAME     L"\\Device\\PcaRawIpFilter"
#define TDIH_AFD_DEVICE_NAME        L"\\Device\\PcaAfdFilter"
#define TDIH_W32API_DEVICE_NAME     L"\\Device\\PCATDIH"

#ifdef ZNEVER
#define TDIH_IP_DOS_DEVICE_NAME     L"\\DosDevices\\PcaIpFilter"
#define TDIH_TCP_DOS_DEVICE_NAME    L"\\DosDevices\\PcaTcpFilter"
#define TDIH_UDP_DOS_DEVICE_NAME    L"\\DosDevices\\PcaUdpFilter"
#define TDIH_AFD_DOS_DEVICE_NAME    L"\\DosDevices\\PcaAfdFilter"
#define TDIH_RAW_IP_DOS_DEVICE_NAME L"\\DosDevices\\PcaRawIpFilter"
#endif

#define TDIH_W32API_DOS_DEVICE_NAME L"\\DosDevices\\PCATDIH"

//
// TCP/UDP/RawIP IOCTL code definitions
//
#define FSCTL_TCP_BASE     FILE_DEVICE_NETWORK

#define _TCP_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_TCP_BASE, function, method, access)

#define IOCTL_TCP_QUERY_INFORMATION_EX  \
            _TCP_CTL_CODE(0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_TCP_SET_INFORMATION_EX  \
            _TCP_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// Control Use Of Experimental Filters
// -----------------------------------
// The \Device\Tcp and \Device\Udp filters are fairly well defined by
// Microsoft. However, other TCP/IP related devices are Microsoft
// propritary and are largely undocumented. Since these experimental
// filters are basically useless, their use is controlled by the following
// preprocessor variables:
//
#define USE_AFD_FILTER
#define USE_RAW_IP_FILTER
#define USE_IP_FILTER

/**********************************************************************************
*
* End of macro definitions
*
**********************************************************************************/

/**************************************************************************
   each structure has a unique "node type" or signature associated with it
**************************************************************************/
#define   TDIH_NODE_TYPE_GLOBAL_DATA            (0xfdecba10)
#define   TDIH_NODE_TYPE_IP_FILTER_DEVICE       (0xfdecba11)
#define   TDIH_NODE_TYPE_TCP_FILTER_DEVICE      (0xfdecba12)
#define   TDIH_NODE_TYPE_UDP_FILTER_DEVICE      (0xfdecba13)
#define   TDIH_NODE_TYPE_RAW_IP_FILTER_DEVICE   (0xfdecba14)
#define   TDIH_NODE_TYPE_AFD_FILTER_DEVICE      (0xfdecba15)
#define   TDIH_NODE_TYPE_W32API_DEVICE          (0xfdecba16)

/**************************************************************************
   every structure has a node type, and a node size associated with it.
   The node type serves as a signature field. The size is used for
   consistency checking ...
**************************************************************************/
typedef
struct _TDIH_NodeIdentifier
{
   ULONG    NodeType;         // a 32 bit identifier for the structure
   ULONG    NodeSize;         // computed as sizeof(structure)
}
   TDIH_NodeIdentifier, *PTDIH_NodeIdentifier;

typedef
struct _IP_Extension
{
   ULONG    Reserved;   // Not Yet Defined...
}
   IP_Extension, *PIP_Extension;

typedef
struct _TCP_Extension
{
   ULONG    Reserved;   // Not Yet Defined...
}
   TCP_Extension, *PTCP_Extension;

typedef
struct _UDP_Extension
{
   ULONG    Reserved;   // Not Yet Defined...
}
   UDP_Extension, *PUDP_Extension;

typedef
struct _RawIP_Extension
{
   ULONG    Reserved;   // Not Yet Defined...
}
   RawIP_Extension, *PRawIP_Extension;

typedef
struct _Afd_Extension
{
   ULONG    Reserved;   // Not Yet Defined...
}
   Afd_Extension, *PAfd_Extension;

typedef
struct _W32API_Extension
{
   ULONG    Reserved;   // Not Yet Defined...
}
   W32API_Extension, *PW32API_Extension;

/**************************************************************************
   the device extension for each device object created by the filter driver
**************************************************************************/
typedef
struct _TDIH_DeviceExtension
{
   // A signature (including device extension size).
   TDIH_NodeIdentifier     NodeIdentifier;

   // For convenience, a back ptr to the device object that contains this
   // extension.
   PDEVICE_OBJECT          pFilterDeviceObject; // Our Device Object

   // The sample filter driver keeps a private doubly-linked list of all
   // device objects created by the driver.
   LIST_ENTRY              NextDeviceObject;

   // See Flag definitions below.
   ULONG                   DeviceExtensionFlags;

   // The target (lowest level) device object we are attached to.
   PDEVICE_OBJECT          TargetDeviceObject;

   // The file object of the device we are attached to.
   PFILE_OBJECT            TargetFileObject;

   // The device object immediately below us.
   PDEVICE_OBJECT          LowerDeviceObject;

   // A count of outstanding I/O requests for which we have specified a
   // completion routine.
   LARGE_INTEGER           OutstandingIoRequests;

   // The OutstandingIoRequests field is protected by an Executive spin lock.
   KSPIN_LOCK              IoRequestsSpinLock;

   // The event object is used to synchronize detach requests with pending
   // I/O operations and similar stuff.
   KEVENT                  IoInProgressEvent;

   //
   // Protocol Filter Specific Data Area
   //
   union
   {
      IP_Extension      ip;
      TCP_Extension     tcp;
      UDP_Extension     udp;
      RawIP_Extension   rawip;
      Afd_Extension     afd;
      W32API_Extension  w32api;
   }
      u;

   ULONG                   PackingInsurance; // Do Not Touch!!!
}
   TDIH_DeviceExtension, *PTDIH_DeviceExtension;

#define   TDIH_DEV_EXT_ATTACHED   (0x00000001)


/////////////////////////////////////////////////////////////////////////////
//                       I P  D E V I C E  F I L T E R                     //
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_IP_FILTER

NTSTATUS
IPFilter_Attach(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   );

#ifdef DBG

VOID
IPFilter_Detach(
   PDEVICE_OBJECT pDeviceObject
   );

#endif // DBG

NTSTATUS
IPFilter_Dispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

#endif // USE_IP_FILTER


/////////////////////////////////////////////////////////////////////////////
//                      T C P  D E V I C E  F I L T E R                    //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
TCPFilter_Attach(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   );

#ifdef DBG

VOID
TCPFilter_Detach(
   PDEVICE_OBJECT pDeviceObject
   );

#endif // DBG


/////////////////////////////////////////////////////////////////////////////
//                      U D P  D E V I C E  F I L T E R                    //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
UDPFilter_Attach(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   );

#ifdef DBG

VOID
UDPFilter_Detach(
   PDEVICE_OBJECT pDeviceObject
   );

#endif // DBG


/////////////////////////////////////////////////////////////////////////////
//                    R A W  I P  D E V I C E  F I L T E R                 //
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_RAW_IP_FILTER

NTSTATUS
RawIPFilter_Attach(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   );

#ifdef DBG

VOID
RawIPFilter_Detach(
   PDEVICE_OBJECT pDeviceObject
   );

#endif // DBG

#endif // USE_RAW_IP_FILTER


/////////////////////////////////////////////////////////////////////////////
//                      A F D  D E V I C E  F I L T E R                    //
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_AFD_FILTER

NTSTATUS
AfdFilter_Attach(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   );

#ifdef DBG

VOID
AfdFilter_Detach(
   PDEVICE_OBJECT pDeviceObject
   );

#endif // DBG

NTSTATUS
AfdFilter_Dispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

#endif USE_AFD_FILTER


/////////////////////////////////////////////////////////////////////////////
//                      W I N 3 2  A P I  D E V I C E                      //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
W32API_Initialize(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   );

#ifdef DBG

VOID
W32API_Unload(
   PDEVICE_OBJECT pDeviceObject
   );

#endif // DBG

NTSTATUS
W32API_Dispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

#define TDIH_BUFFER_POOL_SIZE      64
extern NDIS_HANDLE g_hTDIH_BufferPool;

#endif  // _PCATDIH_H_

