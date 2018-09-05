//tdiutil.h
#ifndef _TDIUTIL_H_
#define _TDIUTIL_H_

#define SFTK_FILE_DEVICE_TDITTCP_BASE 0x00008004  // First User Device Type

#define SFTK_FILE_DEVICE_TCP_SERVER   (SFTK_FILE_DEVICE_TDITTCP_BASE)
#define SFTK_IOCTL_TCP_SERVER_BASE    SFTK_FILE_DEVICE_TCP_SERVER

#define SFTK_FILE_DEVICE_TCP_CLIENT   (SFTK_FILE_DEVICE_TDITTCP_BASE+1)
#define SFTK_IOCTL_TCP_CLIENT_BASE    SFTK_FILE_DEVICE_TCP_CLIENT


#define SFTK_TDI_TCP_SERVER_BASE_NAME_A        "SftkTdiTcpServer"
#define SFTK_TDI_TCP_SERVER_BASE_NAME_W        L"SftkTdiTcpServer"
#define SFTK_TDI_TCP_SERVER_DISPLAY_NAME_A     "Sftk TDI TTCP TCP Server"
#define SFTK_TDI_TCP_SERVER_DISPLAY_NAME_W     L"Sftk TDI TTCP TCP Server"
#define SFTK_TDI_TCP_SERVER_DEVICE_NAME_A      "\\Device\\SftkTdiTcpServer"
#define SFTK_TDI_TCP_SERVER_DEVICE_NAME_W      L"\\Device\\SftkTdiTcpServer"

#ifdef UNICODE

#define SFTK_TDI_TCP_SERVER_BASE_NAME       SFTK_STFK_TDI_TCP_SERVER_BASE_NAME_W
#define SFTK_TDI_TCP_SERVER_DISPLAY_NAME    SFTK_TDI_TCP_SERVER_DISPLAY_NAME_W
#define SFTK_TDI_TCP_SERVER_DEVICE_NAME     SFTK_TDI_TCP_SERVER_DEVICE_NAME_W

#else

#define SFTK_TDI_TCP_SERVER_BASE_NAME       SFTK_TDI_TCP_SERVER_BASE_NAME_A
#define SFTK_TDI_TCP_SERVER_DISPLAY_NAME    SFTK_TDI_TCP_SERVER_DISPLAY_NAME_A
#define SFTK_TDI_TCP_SERVER_DEVICE_NAME     SFTK_TDI_TCP_SERVER_DEVICE_NAME_A

#endif

//
// TDI TCP Client Device Name Strings
//
#define SFTK_TDI_TCP_CLIENT_BASE_NAME_A        "SftkTdiTcpClient"
#define SFTK_TDI_TCP_CLIENT_BASE_NAME_W        L"SftkTdiTcpClient"
#define SFTK_TDI_TCP_CLIENT_DISPLAY_NAME_A     "Sftk TDI TTCP TCP Client"
#define SFTK_TDI_TCP_CLIENT_DISPLAY_NAME_W     L"Sftk TDI TTCP TCP Client"
#define SFTK_TDI_TCP_CLIENT_DEVICE_NAME_A      "\\Device\\SftkTdiTcpClient"
#define SFTK_TDI_TCP_CLIENT_DEVICE_NAME_W      L"\\Device\\SftkTdiTcpClient"

#ifdef UNICODE

#define SFTK_TDI_TCP_CLIENT_BASE_NAME       SFTK_TDI_TCP_CLIENT_BASE_NAME_W
#define SFTK_TDI_TCP_CLIENT_DISPLAY_NAME    SFTK_TDI_TCP_CLIENT_DISPLAY_NAME_W
#define SFTK_TDI_TCP_CLIENT_DEVICE_NAME     SFTK_TDI_TCP_CLIENT_DEVICE_NAME_W

#else

#define SFTK_TDI_TCP_CLIENT_BASE_NAME       SFTK_TDI_TCP_CLIENT_BASE_NAME_A
#define SFTK_TDI_TCP_CLIENT_DISPLAY_NAME    SFTK_TDI_TCP_CLIENT_DISPLAY_NAME_A
#define SFTK_TDI_TCP_CLIENT_DEVICE_NAME     SFTK_TDI_TCP_CLIENT_DEVICE_NAME_A

#endif

#if 0

typedef
VOID
(*PDEVICE_UNLOAD) (
    IN struct _DEVICE_OBJECT *DeviceObject
    );


typedef struct _DEVICE_EXTENSION
{
   PDEVICE_OBJECT       pDeviceObject;
   BOOLEAN				bSymbolicLinkCreated;
   SESSION_MANAGER		SessionManager;
   PDRIVER_DISPATCH     MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
   PDEVICE_UNLOAD       DeviceUnload;

}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#endif //0

extern PDRIVER_OBJECT   g_pTheDriverObject;


/////////////////////////////////
////////Device Creation Functions
/////////////////////////////////

NTSTATUS
SftkDeviceLoadC(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
SftkDeviceLoadS(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//////////////////////////////
///////Common Base Functions
//////////////////////////////

NTSTATUS
SftkTDIDeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTDIDeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTDIDeviceRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTDIDeviceWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTDIDeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTDIDeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
SftkTDIDriverUnload(
    IN PDRIVER_OBJECT pDriverObject
    );

///////////////////////////
//Client Functions
///////////////////////////
NTSTATUS
SftkTCPCDeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTCPCDeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTCPCDeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTCPCDeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
SftkTCPCDeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   );


///////////////////////////////////////
//Adding Server Specific Functions
///////////////////////////////////////

NTSTATUS
SftkTCPSDeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTCPSDeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTCPSDeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SftkTCPSDeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
SftkTCPSDeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   );

#endif //_TDIUTIL_H_