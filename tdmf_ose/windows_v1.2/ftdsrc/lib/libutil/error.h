#ifndef _INC_ERROR
#define _INC_ERROR

//
// Error codes returned by NtFsControlFile (see NTSTATUS.H)
//
#define STATUS_SUCCESS			         ((NTSTATUS)0x00000000L)
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)
#define STATUS_ALREADY_COMMITTED         ((NTSTATUS)0xC0000021L)
#define STATUS_INVALID_DEVICE_REQUEST    ((NTSTATUS)0xC0000010L)

//
// return code type
//
typedef UINT NTSTATUS;

//
// Io Status block (see NTDDK.H)
//
typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;


#ifdef  __cplusplus
extern "C" {
#endif

extern void PrintNtError( NTSTATUS Status, char *error );
extern void PrintWin32Error( DWORD ErrorCode, char *error );

#ifdef __cplusplus
}   /* ... extern "C" */
#endif

#endif // _INC_ERROR