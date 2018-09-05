// FileObjInfo.h    Include file for File Object info driver
//
#include <ntddk.h>
#include <string.h>
#include <devioctl.h>
#include <warning.h>
#include "../include/FileObjInfoIoCtl.h"           // Get IOCTL interface definitions

// NT device name
#define GPD_DEVICE_NAME L"\\Device\\TDMFANALYZER0"

// File system device name.   When you execute a CreateFile call to open the
// device, use "\\.\GpdDev", or, given C's conversion of \\ to \, use
// "\\\\.\\GpdDev"
#define DOS_DEVICE_NAME L"\\DosDevices\\TDMFANALYZER"

// driver local data structure specific to each device object
typedef struct _LOCAL_DEVICE_INFO {
    ULONG               DeviceType;     // Our private Device Type
    PDEVICE_OBJECT      DeviceObject;   // The Gpd device object.
} LOCAL_DEVICE_INFO, *PLOCAL_DEVICE_INFO;

/********************* function prototypes ***********************************/
//
NTSTATUS    DriverEntry(       IN  PDRIVER_OBJECT DriverObject,
                               IN  PUNICODE_STRING RegistryPath );

NTSTATUS    GpdCreateDevice(   IN  PWSTR szPrototypeName,
                               IN  DEVICE_TYPE DeviceType,
                               IN  PDRIVER_OBJECT DriverObject,
                               OUT PDEVICE_OBJECT *ppDevObj     );

NTSTATUS    GpdDispatch(       IN  PDEVICE_OBJECT pDO,
                               IN  PIRP pIrp                    );

NTSTATUS    GpdIoctlResolve(   IN  PLOCAL_DEVICE_INFO pLDI,
                               IN  PIRP pIrp,
                               IN  PIO_STACK_LOCATION IrpStack,
                               IN  ULONG IoctlCode              );

VOID        GpdUnload(         IN  PDRIVER_OBJECT DriverObject );

