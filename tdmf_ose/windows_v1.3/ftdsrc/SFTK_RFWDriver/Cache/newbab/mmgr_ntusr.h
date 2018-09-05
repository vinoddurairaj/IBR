/*HDR************************************************************************
 *                                                                         
 * Softek - Fujitsu                                    
 *
 *===========================================================================
 *
 * N mmg_ntusr.h
 * P Replicator 
 * S OS definitions, prototypes
 * V Windows Kernel specific
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 03.05.2004
 * O User space emulations
 * T Cache requirements and design specifications - v 1.0.0.
 * C DBG - 
 * H 03.05.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: mmgr_ntusr.h,v 1.4 2004/06/17 22:03:27 jcq40 Exp $"
 *
 *HDR************************************************************************/

#ifndef _MMG_NTUSR_
#define _MMG_NTUSR_

// useless ddk beautifiers 

#define IN
#define OUT
#define OPTIONAL

// some types
typedef void *        PVOID;
typedef unsigned long ULONG;

#define try     __try
#define finally __finally
#define leave   __leave

#ifdef _DEBUG
extern  MMG_PUBLIC ULONG MmgDebugLevel;
#define DBGSTATIC static
#undef  ASSERTMSG
#define ASSERTMSG(msg,exp)         if (!(exp)) {print msg}
#define ASSERT(exp)                if (!(exp)) {printf("%s",__FILE__),printf("%d",__LINE__), printf("%s", #exp); DebugBreak();} 
#define MMGDEBUG(LEVEL, STRING)    if (LEVEL <= MmgDebugLevel)   \
                                   {                             \
								   if (LEVEL == MMGDBG_FATAL) {\
								   printf("***");                \
								   }                             \
								   printf STRING;                \
								   }
#define CHECK_IRQL(msg, irql)
#endif /* _DEBUG */



#ifdef NOP
//
//  excerpt from ddk/ntdef.h
//
//  Doubly linked list structure.  Can be used as either a list head, or
//  as link words.
//

typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY, *PRLIST_ENTRY;

// begin_winnt begin_ntminiport
//
// Calculate the byte offset of a field in a structure of type type.
//

#define FIELD_OFFSET(type, field)    ((LONG)(LONG_PTR)&(((type *)0)->field))


//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (ULONG_PTR)(&((type *)0)->field)))
#endif
//
//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.
//

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
//
//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }


//
//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)

// end_wdm end_nthal end_ntifs end_ntndis

/* 
 * Emulate ftd_config from services
 */
typedef struct 
{
	int dummy;

} ftd_lg_info_t;

//
// Pool Allocation routines (in pool.c)
//

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType
    } POOL_TYPE;

typedef
PVOID
(*PALLOCATE_FUNCTION) (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

typedef
VOID
(*PFREE_FUNCTION) (
    IN PVOID Buffer
    );

typedef struct _npl_
{
	SIZE_T size;

} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

typedef enum 
{
	SynchronizationEvent = 1,
	NotificationEvent    = 2

} EVENT_TYPE;

typedef enum 
{

	KernelMode,
	UserMode

} KPROCESSOR_MODE;

typedef enum 
{
	Executive,
	UserRequest

} KWAIT_REASON;

typedef enum 
{
	IO_NO_INCREMENT,

} KPRIORITY;

#define NTSTATUS                              DWORD // ugly

//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//

#define OS_NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

//
// Generic test for information on any status value.
//

#define NT_INFORMATION(Status) ((ULONG)(Status) >> 30 == 1)

//
// Generic test for warning on any status value.
//

#define NT_WARNING(Status) ((ULONG)(Status) >> 30 == 2)

//
// Generic test for error on any status value.
//

#define NT_ERROR(Status) ((ULONG)(Status) >> 30 == 3)

#define STATUS_SUCCESS                        (0x00000000L) 
//
// MessageId: STATUS_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Insufficient system resources exist to complete the API.
//
#define STATUS_INSUFFICIENT_RESOURCES    (0xC000009AL)     // ntsubauth

//
// MessageId: STATUS_BUFFER_TOO_SMALL
//
// MessageText:
//
//  {Buffer Too Small}
//  The buffer is too small to contain the entry. No information has been written to the buffer.
//
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)

//
// MessageId: STATUS_INVALID_PARAMETER
//
// MessageText:
//
//  An invalid parameter was passed to a service or function.
//
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)

//
// MessageId: STATUS_PIPE_EMPTY
//
// MessageText:
//
//  Used to indicate that a read operation was done on an empty pipe.
//
#define STATUS_PIPE_EMPTY                ((NTSTATUS)0xC00000D9L)

//
// MessageId: STATUS_UNSUCCESSFUL
//
// MessageText:
//
//  {Operation Failed}
//  The requested operation was unsuccessful.
//
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)


// Our Emulation functions
#define OS_NTSTATUS                           NTSTATUS
#define OS_LIST_ENTRY                         LIST_ENTRY
#define OS_MUTEX_LOCK                         DWORD  //KMUTEX
#define OS_ERESOURCE                          HANDLE //ERESOURCE
#define OS_KSPINLOCK                          HANDLE //KSPIN_LOCK
#define OS_SYNC_EVENT                         HANDLE //EVENT
#define OS_LOOKASIDE					      NPAGED_LOOKASIDE_LIST
#define OS_MEMSECTION                         HANDLE // ?????????
#define OS_ExInitializeNPagedLookasideList    RplUsrInitNPLookAsideList
#define OS_ExDeleteNPagedLookasideList        RplUsrDelNPLookAsideList
#define OS_ExFreeToNPagedLookasideList        RplUsrFreeToNPagedLookasideList
#define OS_ExAllocatePoolWithTag              RplUsrAllocatePoolWithTag
#define OS_ExFreePool                         RplUsrFreePool
#define OS_AllocateSection                    RplUsrAllocateSection
#define OS_FreeSection					      RplUsrFreeSection
#define OS_IsValidSection				      RplUsrIsValidSection
#define OS_ExAllocateFromNPagedLookasideList  RplUsrAllocateFromNPagedLookasideList
#define OS_KeInitializeEvent                  RplUsrKeInitializeEvent
#define OS_ExInitializeResourceLite           RplUsrInitializeResourceLite
#define OS_ExAcquireResourceExclusiveLite     RplUsrAcquireResourceExclusiveLite
#define OS_ExReleaseResourceLite              RplUsrReleaseResourceLite
#define OS_KeDelayExecutionThread             RplUsrDelayExecutionThread
#define OS_KeWaitForSingleObject              RplUsrWaitForSingleObject
#define OS_KeSetEvent                         RplUsrSetEvent
#define OS_MapViewOfSection                   RplUsrMapViewOfSection
#define OS_InterlockedCompareExchange         RplInterlockedCompareExchange
#define OS_InterlockedExchange                RplInterlockedExchange
#define OS_KeGetCurrentThread                 RplGetCurrentThread

    

typedef OS_SYNC_EVENT *PRKEVENT; 

// Emulation prototypes

OS_NTSTATUS
RplUsrAllocateSection(IN OS_MEMSECTION *memSection, LARGE_INTEGER memSize);

OS_NTSTATUS
RplUsrFreeSection(IN OS_MEMSECTION *memSection);

BOOLEAN
RplUsrIsValidSection(IN OS_MEMSECTION *memSection);

OS_NTSTATUS
RplUsrInitNPLookAsideList(  IN PNPAGED_LOOKASIDE_LIST  Lookaside,
							IN PALLOCATE_FUNCTION  Allocate  OPTIONAL,
							IN PFREE_FUNCTION      Free      OPTIONAL,
							IN ULONG   Flags,
							IN SIZE_T  Size,
							IN ULONG   Tag,
							IN USHORT  Depth );

OS_NTSTATUS
RplUsrDelNPLookAsideList(IN PNPAGED_LOOKASIDE_LIST  Lookaside);

PVOID
RplUsrAllocateFromNPagedLookasideList(IN PNPAGED_LOOKASIDE_LIST  Lookaside);

VOID
RplUsrFreeToNPagedLookasideList(IN PNPAGED_LOOKASIDE_LIST  Lookaside, IN PVOID  Entry);

VOID
RplUsrKeInitializeEvent(IN PRKEVENT  Event, IN EVENT_TYPE  Type, IN BOOLEAN  State);

OS_NTSTATUS
RplUsrInitializeResourceLite(IN OS_ERESOURCE *Eresource);

BOOLEAN
RplUsrAcquireResourceExclusiveLite(IN OS_ERESOURCE *Eresource, IN BOOLEAN Wait);

VOID 
RplUsrReleaseResourceLite(IN OS_ERESOURCE  *Eresource);

OS_NTSTATUS 
RplUsrDelayExecutionThread(IN KPROCESSOR_MODE  WaitMode,
						   IN BOOLEAN  Alertable,
						   IN PLARGE_INTEGER  Interval   );

OS_NTSTATUS 
RplUsrWaitForSingleObject(IN PVOID  Object,
	  				      IN KWAIT_REASON  WaitReason,
						  IN KPROCESSOR_MODE  WaitMode,
						  IN BOOLEAN          Alertable,
						  IN PLARGE_INTEGER   Timeout  OPTIONAL    );

LONG
RplUsrSetEvent(IN PRKEVENT  Event,
               IN KPRIORITY Increment,
               IN BOOLEAN   Wait );
PVOID
RplUsrAllocatePoolWithTag(IN POOL_TYPE  PoolType,
						IN SIZE_T  NumberOfBytes,
						IN ULONG  Tag		);

VOID
RplUsrFreePool(IN PVOID  p_elem);

OS_NTSTATUS
RplUsrMapViewOfSection(OS_MEMSECTION *p_memsection,
					   vaddr_t       size,
					   vaddr_t       offset,
					   PVOID         *p_mapaddr);

LONG
RplInterlockedCompareExchange(
    IN OUT PLONG  Destination,
    IN LONG  Exchange,
    IN LONG  Comparand
    );

LONG 
RplInterlockedExchange(IN OUT PLONG Target,
                       IN     LONG  Value    );
PVOID
RplGetCurrentThread();

#endif _MMG_NTUSR_

/* EOF */
