#ifndef FTD_NT_H
#define FTD_NT_H

// some constant definitions
#define     FTD_PANIC_IDENTIFIER            (0x86427532)

#define     FTD_DRV_DIR                     L"\\Device\\DTC"
#define     FTD_DOS_DRV_DIR_XP              L"\\Global??\\DTC"
#define     FTD_DOS_DRV_DIR                 L"\\DosDevices\\DTC"
#define     FTD_CTL_NAME                    L"ctl"
#define     FTD_LG_NAME                     L"lg"
#define     FTD_LG_EVENT_NAME               L"bab"

#define     DEFAULT_CHUNK_SIZE              0x100000
#define     DEFAULT_NUM_CHUNKS              0x40

#define     REGISTRY_NAME_BREAKONENTRY      L"BreakOnEntry"
#define     REGISTRY_NAME_DEBUGLEVEL        L"DebugLevel"
#define     REGISTRY_NAME_CHUNK_SIZE        L"chunk_size"
#define     REGISTRY_NAME_NUM_CHUNKS        L"num_chunks"

#define     DEFAULT_MAXMEM                  0x00000000

#define     REGISTRY_NAME_MAXMEM            L"maxmem"

/**************************************************************************
    some useful definitions
**************************************************************************/
typedef char                int8;
typedef short               int16;
typedef int                 int32;

typedef unsigned char       u_int8;
typedef unsigned short      u_int16;
typedef unsigned int        u_int32;
typedef unsigned int        u_int;
typedef unsigned long       u_long;

typedef unsigned char       u_char;

// global variables - minimize these
#if _DEBUG
extern ULONG                FTDDebugLevel;
#define TDMF_DEBUG_PRINT  DbgPrint 
#else
#define TDMF_DEBUG_PRINT 
#endif

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'DTFZ')
#endif

#if _DEBUG
#define FTDBreakPoint() DbgBreakPoint()

extern ULONG   FTDDebugLevel;

#define FTDBUGCHECK             ((ULONG)0x80000000)
#define FTDERRORS               ((ULONG)0x00000001)
#define FTDINIT                 ((ULONG)0x00000002)

#define FTDDebug(LEVEL, STRING) \
        do { \
            if (FTDDebugLevel & LEVEL) { \
                DbgPrint STRING; \
            } \
            if (LEVEL == FTDBUGCHECK) { \
                ASSERT(FALSE); \
            } \
        } while (0)
#else
#define FTDBreakPoint()

#define FTDDebug(LEVEL,STRING) do {NOTHING;} while (0)
#endif

/**********************************************************************************
*
* The following macros make life a little easier
*
**********************************************************************************/

// try-finally simulation
#define try_return(S)   { S; goto try_exit; }
#define try_return1(S)  { S; goto try_exit1; }
#define try_return2(S)  { S; goto try_exit2; }

// Flag (bit-field) manipulation
#define FTDSetFlag(Flag, Value) ((Flag) |= (Value))
#define FTDClearFlag(Flag, Value)   ((Flag) &= ~(Value))

// Align a value along a 4-byte boundary.
#define FTDQuadAlign(Value)         ((((uint32)(Value)) + 7) & 0xfffffff8)

// to perform a bug-check (panic), the following macro is used
// it allows us to print out the file identifier, line #, and upto 3
// additional DWORD arguments.
#define FTDPanic(arg1, arg2, arg3)                  \
    (KeBugCheckEx(FTD_PANIC_IDENTIFIER, FTD_BUG_CHECK_ID | __LINE__, (uint32)(arg1), (uint32)(arg2), (uint32)(arg3)))

// the following macro allows us to increment a large integer value atomically.
// we expect an unsigned long to be supplied as the increment value.
// a spin lock should be passed-in to synchronize operations
#define FTDIncrementLargeInteger(LargeIntegerOp, ULongIncrement, PtrSpinLock)   {           \
    KIRQL               OldIrql;                                                                                            \
    KeAcquireSpinLock(PtrSpinLock, &OldIrql);                                                                   \
    RtlLargeIntegerAdd((LargeIntegerOp),(RtlConvertUlongToLargeInteger((ULongIncrement)))); \
    KeReleaseSpinLock(PtrSpinLock, OldIrql);                                                                    \
}

// the following macro allows us to decrement a large integer value atomically.
// we expect an unsigned long to be supplied as the decrement value.
// a spin lock should be passed-in to synchronize operations
#define FTDDecrementLargeInteger(LargeIntegerOp, ULongIncrement, PtrSpinLock)   {           \
    KIRQL               OldIrql;                                                                                            \
    KeAcquireSpinLock(PtrSpinLock, &OldIrql);                                                                   \
    RtlLargeIntegerSubtract((LargeIntegerOp),(RtlConvertUlongToLargeInteger((ULongIncrement))));    \
    KeReleaseSpinLock(PtrSpinLock, OldIrql);                                                                    \
}

// the following macro allows us to check if the large integer value is zero,
// atomically. Note that I have added (for convenience) a check to ensure that
// the value is non-negative.
#define FTDIsLargeIntegerZero(ReturnValue, LargeIntegerOp, PtrSpinLock) {                   \
    KIRQL               OldIrql;                                                                                            \
    KeAcquireSpinLock(PtrSpinLock, &OldIrql);                                                                   \
    ASSERT(RtlLargeIntegerGreaterOrEqualToZero((LargeIntegerOp)));                                      \
    ReturnValue = RtlLargeIntegerEqualToZero((LargeIntegerOp));                                         \
    KeReleaseSpinLock(PtrSpinLock, OldIrql);                                                                    \
}

/**********************************************************************************
*
* End of macro definitions
*
**********************************************************************************/
extern FTD_STATUS ftd_nt_add_device(PDRIVER_OBJECT DriverObject, char *devname, char *vdevname, ftd_dev_t **softp);
extern FTD_STATUS ftd_nt_add_lg(PDRIVER_OBJECT DriverObject, int lgnum, char *vdevname, ftd_lg_t **lgp);
extern FTD_STATUS ftd_nt_detach_device(ftd_dev_t *softp);
extern FTD_STATUS ftd_nt_del_device(ftd_dev_t *softp);
extern FTD_STATUS ftd_nt_del_lg(ftd_lg_t *lgp);

extern FTD_STATUS ftd_nt_lg_close(ftd_lg_t *lgp);
extern FTD_STATUS ftd_nt_lg_open(ftd_lg_t *lgp, int minor);

#endif /* FTD_NT_H */