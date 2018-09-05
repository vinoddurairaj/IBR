// FileObjInfoIoCtl.h    Include file for FileObjInfo driver
//
// Defines the IOCTL codes used.  The IOCTL code contains a command
// identifier, plus other information about the device, the type of access
// with which the file must have been opened, and the type of buffering.
//

// Device type           -- in the "User Defined" range."
#define GPD_TYPE    40000

// Dos-Device name
#define	GPD_DOSDEVICE	"\\\\.\\TDMFANALYZER"

#define MAX_FOBJ_FILE_LEN   256

// The IOCTL function codes from 0x800 to 0xFFF are for customer use.

#define IOCTL_FOI_RESOLVE \
    CTL_CODE( GPD_TYPE, 0xAFF, METHOD_BUFFERED, FILE_ANY_ACCESS )

#include <pshpack4.h>		// allow use in driver and application build!

// "file object" - similar to real file object
typedef struct _FOBJ {
    BOOLEAN isValid;
    USHORT Type;
    USHORT Size;
    ULONG DeviceObject;
    BOOLEAN LockOperation;
    BOOLEAN DeletePending;
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;
    BOOLEAN SharedRead;
    BOOLEAN SharedWrite;
    BOOLEAN SharedDelete;
    ULONG Flags;
    LARGE_INTEGER CurrentByteOffset;
    ULONG Waiters;
    ULONG Busy;
    USHORT FileName[MAX_FOBJ_FILE_LEN];  // wide string file name
} FOBJ;

// "device object" - similar to real device object
// this device object ALWAYS belongs to the corresponding file object
typedef struct _DOBJ {
    BOOLEAN isValid;
    USHORT Type;
    USHORT Size;
    LONG ReferenceCount;
    PVOID DriverObject;
    ULONG Flags;                                // See above:  DO_...
    ULONG Characteristics;                      // See ntioapi:  FILE_...
    DEVICE_TYPE DeviceType;
    USHORT DeviceName[MAX_FOBJ_FILE_LEN];  // wide string file name
} DOBJ;

typedef struct  _FOI_RESOLVE_INPUT {
    ULONG   FObjAddress;        // actually a File object pointer
}   FOI_RESOLVE_INPUT;

typedef struct _FOI_RESOLVE_OUTPUT {
    FOBJ    fobj;
    DOBJ    dobj;
}   FOI_RESOLVE_OUTPUT;

#include <poppack.h>