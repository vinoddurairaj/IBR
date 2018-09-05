/*
 * Copyright (c) 1996, 1998 by FullTime Software, Inc.  All Rights Reserved.
 *
 * Defines and structures used only within the driver.
 *
 * $Id: ftd_def.h,v 1.12 2004/03/02 15:24:08 szg00 Exp $
 */

#ifndef _FTD_DEF_H
#define _FTD_DEF_H

//
// #DEFINES FOR PRAGMA MESSAGES TO KNOW WHERE WE ARE IN THE CODE
//
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning Msg: "
#define __LOC2__ __FILE__ "("__STR1__(__LINE__)") warning Msg: "

#include <ntddk.h>

#include <stdio.h>
#include <sys\types.h>
#include "sysmacros.h"

#define _TEXT(quote) L##quote      // r_winnt

#define DEV_BSHIFT 9
#define DEV_BSIZE   512

typedef dev_t           minor_t;
typedef char *          caddr_t;
typedef long            daddr_t;

#define FTD_CONTEXT     ERESOURCE_THREAD
#ifndef _ERESOURCE_SPINS_
#define FTD_IRQL        KIRQL
#else
#define FTD_IRQL        ERESOURCE_THREAD
#endif

#define FTD_STATUS      NTSTATUS
#define FTD_SUCCESS     STATUS_SUCCESS

#define ENOENT          FTD_DRIVER_ERROR_CODE + 2
#define ENXIO           FTD_DRIVER_ERROR_CODE + 6
#define EAGAIN          FTD_DRIVER_ERROR_CODE + 11
#define EACCES          FTD_DRIVER_ERROR_CODE + 13
#define EFAULT          FTD_DRIVER_ERROR_CODE + 14
#define EBUSY           FTD_DRIVER_ERROR_CODE + 16
#define EINVAL          FTD_DRIVER_ERROR_CODE + 22
#define ENOTTY          FTD_DRIVER_ERROR_CODE + 25
#define EADDRINUSE      FTD_DRIVER_ERROR_CODE + 98  /* Address already in use */

/**************************************************************************
    each structure has a unique "node type" or signature associated with it
**************************************************************************/
#define FTD_NODE_TYPE_CTL_DATA          (0xfdecba10)
#define FTD_NODE_TYPE_LG_DEVICE         (0xfdecba11)
#define FTD_NODE_TYPE_ATTACHED_DEVICE   (0xfdecba12)
#define FTD_NODE_TYPE_EXT_DEVICE        (0xfdecba13)

/**************************************************************************
    every structure has a node type, and a node size associated with it.
    The node type serves as a signature field. The size is used for
    consistency checking ...
**************************************************************************/
typedef struct ftd_node_id {
    unsigned int    NodeType;           // a 32 bit identifier for the structure
    unsigned int    NodeSize;           // computed as sizeof(structure)
} ftd_node_id_t;

// the following check asserts that the passed-in device extension
// pointer is valid
#define FTDAssertCtlPtrValid(Ptr)           {                                                   \
        ASSERT((Ptr));                                                                                      \
        ASSERT((((Ptr)->NodeIdentifier.NodeType == FTD_NODE_TYPE_CTL_DATA)) &&\
                 ((Ptr)->NodeIdentifier.NodeSize == sizeof(ftd_ctl_t)));    \
}

#define FTDAssertExtPtrValid(PtrExtension)          {                                                   \
        ASSERT((PtrExtension));                                                                                     \
        ASSERT((((PtrExtension)->NodeIdentifier.NodeType == FTD_NODE_TYPE_EXT_DEVICE)) &&\
                 ((PtrExtension)->NodeIdentifier.NodeSize == sizeof(ftd_ext_t)));   \
}

#define FTDAssertDevExtPtrValid(PtrExtension)           {                                                   \
        ASSERT((PtrExtension));                                                                                     \
        ASSERT((((PtrExtension)->NodeIdentifier.NodeType == FTD_NODE_TYPE_ATTACHED_DEVICE)) &&\
                 ((PtrExtension)->NodeIdentifier.NodeSize == sizeof(ftd_dev_t)));   \
}

#define FTDAssertLGExtPtrValid(PtrExtension)            {                                                   \
        ASSERT((PtrExtension));                                                                                     \
        ASSERT((((PtrExtension)->NodeIdentifier.NodeType == FTD_NODE_TYPE_LG_DEVICE)) &&\
                 ((PtrExtension)->NodeIdentifier.NodeSize == sizeof(ftd_lg_t)));    \
}


#ifdef NO_PROTOTYPES
#define _ANSI_ARGS(x) ()
#else 
#define _ANSI_ARGS(x) x
#endif

#include "ftd_bab.h"
#include "ftdio.h"
#include "ftd_klog.h"

/*
 * Local definitions, for clarity of code
 */

/* a bitmap descriptor */
typedef struct ftd_bitmap {             /* SIZEOF == 20 */
    int          bitsize; /* a bit represents this many bytes */
    int          shift;   /* converts sectors to bit values   */
    int          len32;   /* length of "map" in 32-bit words  */
    int          numbits; /* number of valid bits */
    unsigned int *map;
} ftd_bitmap_t;

#define FTD_WAIT_EVENTS             3
#define FTD_WAIT_PSTORE_COMPLETE    0
#define FTD_WAIT_PSTORE_IRP         1
#define FTD_WAIT_TERMINATE          2

#define FTD_DEFAULT_LG          (8)
#define FTD_DEFAULT_DEVS        (32)
#define FTD_DEFAULT_MEM         (24)

#define FTD_CTL_FLAGS_SYMLINK_CREATED           (0x00000001)
#define FTD_CTL_FLAGS_BAB_CREATED               (0x00000002)
#define FTD_CTL_FLAGS_LGIO_CREATED              (0x00000004)

typedef struct ftd_ctl 
{
    ftd_node_id_t       NodeIdentifier;
    ULONG               maxmem;
    int                 chunk_size;     /* 00 */
    int                 num_chunks;     /* 04 */
    caddr_t             bab;            /* 08 */
    unsigned int        bab_size;       /* 0c */
    int                 flags;
    int                 lg_open;        /* 14 */
    int                 dev_open;       /* 18 */
#ifndef _ERESOURCE_SPINS_
    KSPIN_LOCK          lock;
#else
    ERESOURCE           lock;
#endif
    struct ftd_lg       *lghead;        /* 28 */
    PDRIVER_OBJECT          DriverObject;
    PDEVICE_OBJECT          DeviceObject;

    UNICODE_STRING          UserVisibleName;

    PVOID                   DirectoryObject;
    PVOID                   DosDirectoryObject;

    NPAGED_LOOKASIDE_LIST   LGIoLookasideListHead;
} ftd_ctl_t;

#define FTD_LG_OPEN             (0x00000001)
#define FTD_LG_COLLISION        (0x00000002)
#define FTD_LG_SYMLINK_CREATED  (0x00000004)
#define FTD_LG_EVENT_CREATED    (0x00000008)
#define FTD_DIRTY_LAG 1024
/* we won't attempt to reset dirtybits when bab is more than this
   number of sectors full. (Walking the bab is expensive) */
#define FTD_RESET_MAX  (1024 * 20)

typedef struct ftd_lg
{
    ftd_node_id_t       NodeIdentifier;
    int                 state;
    int                 flags;
    dev_t               dev;       /* our device number */
    ftd_ctl_t           *ctlp;
    struct ftd_dev      *devhead;
    struct ftd_lg       *next;
    struct _bab_mgr_    *mgr;
/* NB: "lock" appears to protect the ftd_lg struct as well as the
 * memory allocated by the "mgr" and the mgr itself.
 */
    ERESOURCE           mutex;
#ifndef _ERESOURCE_SPINS_
    KSPIN_LOCK          lock;
#else
    ERESOURCE           lock;
#endif
    char                *statbuf;
    int                 statsize;

    unsigned int        wlentries; /* number of entries in writelog */
    unsigned int        wlsectors; /* number of sectors in writelog */
    unsigned int        dirtymaplag; /* number of entries since last 
                                        reset of dirtybits */

    int                 ndevs;

    unsigned int        sync_depth;
    unsigned int        sync_timeout;
    unsigned int        iodelay;
    PDRIVER_OBJECT      DriverObject;
    PDEVICE_OBJECT      DeviceObject;

    UNICODE_STRING      UserVisibleName;

    HANDLE              hEvent;
    PKEVENT             Event;

    // PStore
    PDEVICE_OBJECT      PStoreDeviceObject;
#ifndef OLD_PSTORE
    UNICODE_STRING      PStoreFileName;
#endif
    BOOLEAN             ThreadShouldStop;

    PVOID               CommitThreadObjectPointer;
    LIST_ENTRY          CommitQueueListHead;
    KSEMAPHORE          CommitQueueSemaphore;

    PVOID               SyncThreadObjectPointer;
    KSEMAPHORE          SyncThreadSemaphore;
} ftd_lg_t;

/*-
 * sync mode states
 *
 * a logical group is in sync mode where the sync depth is 
 * set positive indefinite.
 * 
 * sync mode behavior is triggered only when some threshold 
 * value number of unacknowledge write log entries accumulates.
 */
#define LG_IS_SYNCMODE(lgp) ((lgp)->sync_depth != -1)
#define LG_DO_SYNCMODE(lgp) \
    (LG_IS_SYNCMODE((lgp)) && ((lgp)->wlentries >= (lgp)->sync_depth))


typedef struct ftd_dev
{
    ftd_node_id_t       NodeIdentifier;
    struct ftd_dev      *next;          /*   0 */
    int                 flags;          /*   4 */
    ftd_lg_t            *lgp;           /*   8 */
    daddr_t             disksize;
#ifndef _ERESOURCE_SPINS_
    KSPIN_LOCK          lock;
#else
    ERESOURCE           lock;
#endif
    ftd_bitmap_t        lrdb;           /*  20 */
    ftd_bitmap_t        hrdb;           /*  40 */
    struct buf          *qhead;         /*  60 */
    struct buf          *qtail;         /*  64 */
    struct buf          *freelist;      /*  68 */
    struct buf          *pendingbps;
    struct buf          *lrdbbp;
    int                 pendingios;
    int                 highwater;
/* four unique device ids associated with a device */
    dev_t               cdev;           /*  72 */
    dev_t               bdev;           /*  76 */
    dev_t               localbdisk;     /*  80 */
    dev_t               localcdisk;     /*  84 */
    char                *statbuf;       /*  88 */
    int                 statsize;       /*  92 */
    struct dev_ops      *ld_dop; /* handle on local disk */ /* 96 */

    /* BS stats */
    ftd_int64_t           sectorsread;    /* 100 */
    ftd_int64_t           readiocnt;      /* 108 */
    ftd_int64_t           sectorswritten; /* 116 */
    ftd_int64_t           writeiocnt;     /* 124 */
    int                 wlentries; /* number of entries in wl */ /* 132 */
    int                 wlsectors; /* number of sectors in wl */ /* 136 */
    char                    devname[64];
    int                     lrdb_offset; // in blocks
    // For convenience, a back ptr to the device object that contains this
    // extension.
    PDEVICE_OBJECT          DeviceObject;
    // The driver object for use on repartitioning.
    PDRIVER_OBJECT          DriverObject;
    // The device object we are attached to.
    PDEVICE_OBJECT          TargetDeviceObject;
    // Stored for convenience. A pointer to the driver object for the
    // target device object (you can always obtain this information from
    // the target device object).
    PDRIVER_OBJECT          TargetDriverObject;
    // The Raw Disk device objeck
    PDEVICE_OBJECT          DiskDeviceObject;

    BOOLEAN                 ThreadShouldStop;

    PVOID                   PStoreThreadObjectPointer;
    LIST_ENTRY              PStoreQueueListHead;
    KSEMAPHORE              PStoreQueueSemaphore;
#ifndef NTFOUR    
    // ** New Fields for PnP Support
    KEVENT                  PagingPathCountEvent; 
    unsigned int            PagingPathCount; 
#endif
} ftd_dev_t;

#define FTD_DEV_OPEN            (0x1)
#define FTD_STOP_IN_PROGRESS    (0x2)
#define FTD_DEV_ATTACHED        (0x4)

/* A HRDB bit should address no less than a sector */
#define MINIMUM_HRDB_BITSIZE      DEV_BSHIFT

/* A LRDB bit should address no less than 256K */
#define MINIMUM_LRDB_BITSIZE      18

//
// The one and only status structure! GLOBAL! 
// 

typedef struct ftd_driver_status
{
    unsigned long       ulCurrentOperationsPending;

    //
    // ftd_all.c
    //
    unsigned long       ulftd_biodone;
    unsigned long       ulftd_dev_n_open;
    unsigned long       ulftd_dev_close;
    unsigned long       ulftd_dev_open;
    unsigned long       ulftd_del_device;
    unsigned long       ulftd_do_sync_done;
    unsigned long       ulftd_clear_dirtybits;
    unsigned long       ulftd_set_dirtybits;
    unsigned long       ulftd_clear_hrdb;
    unsigned long       ulftd_lg_close;
    unsigned long       ulftd_lg_open;
    unsigned long       ulftd_del_lg;
    unsigned long       ulftd_ctl_close;
    unsigned long       ulftd_ctl_open;
    //
    // ftd_bab.c
    //
    unsigned long       ulftd_bab_init;
    unsigned long       ulftd_bab_fini;
    unsigned long       ulbab_buffer_free;
    unsigned long       ulbab_buffer_alloc;
    unsigned long       ulftd_bab_alloc_mgr;
    unsigned long       ulftd_bab_free_mgr;
    unsigned long       ulftd_bab_free_memory;
    unsigned long       ulftd_bab_commit_memory;
    unsigned long       ulftd_bab_get_free_length;
    unsigned long       ulftd_bab_get_used_length;
    unsigned long       ulftd_bab_get_committed_length;
    unsigned long       ulftd_bab_copy_out;
    unsigned long       ulftd_bab_get_ptr;
    unsigned long       ulftd_bab_get_pending;
    unsigned long       ulftd_bab_map_memory;
    unsigned long       ulftd_bab_unmap_memory;
    //
    // ftd_bits.c
    //
    unsigned long       ulftd_set_bits;
    unsigned long       ulftd_update_hrdb;
    unsigned long       ulftd_check_lrdb;
    unsigned long       ulftd_update_lrdb;
    unsigned long       ulftd_lg_get_device;
    unsigned long       ulftd_compute_dirtybits;
    //
    // ftd_ddi.c
    //
    unsigned long       ulddi_soft_state_init;
    unsigned long       ulddi_soft_state_fini;
    unsigned long       ulddi_get_soft_state;
    unsigned long       ulddi_soft_state_free;
    unsigned long       ulddi_soft_state_zalloc;
    unsigned long       ulddi_get_free_soft_state;
    unsigned long       ulkmem_alloc;
    unsigned long       ulkmem_zalloc;
    unsigned long       ulkmem_free;
    unsigned long       ulcopyout;
    unsigned long       ulcopyin;
    unsigned long       uldelay;

    //
    // ftd_ioctl.c
    //
    unsigned long       ulftd_clear_bab_and_stats;
    unsigned long       ulftd_get_msb;
    unsigned long       ulftd_ctl_get_config;
    unsigned long       ulftd_ctl_get_num_devices;
    unsigned long       ulftd_ctl_get_num_groups;
    unsigned long       ulftd_ctl_get_devices_info;
    unsigned long       ulftd_ctl_get_groups_info;
    unsigned long       ulftd_ctl_get_device_stats;
    unsigned long       ulftd_ctl_get_group_stats;
    unsigned long       ulftd_ctl_set_group_state;
    unsigned long       ulftd_ctl_get_bab_size;
    unsigned long       ulftd_ctl_update_lrdbs;
    unsigned long       ulftd_ctl_update_hrdbs;
    unsigned long       ulftd_ctl_get_group_state;
    unsigned long       ulftd_ctl_clear_bab;
    unsigned long       ulftd_ctl_clear_dirtybits;
    unsigned long       ulftd_ctl_new_device;
    unsigned long       ulftd_ctl_new_lg;
    unsigned long       ulftd_ctl_init_stop;
    unsigned long       ulftd_ctl_del_device;
    unsigned long       ulftd_ctl_ctl_config;
    unsigned long       ulftd_ctl_get_dev_state_buffer;
    unsigned long       ulftd_ctl_get_lg_state_buffer;
    unsigned long       ulftd_ctl_set_dev_state_buffer;
    unsigned long       ulftd_ctl_set_lg_state_buffer;
    unsigned long       ulftd_ctl_set_iodelay;
    unsigned long       ulftd_ctl_set_sync_depth;
    unsigned long       ulftd_ctl_set_sync_timeout;
    unsigned long       ulftd_ctl_start_lg;
    unsigned long       ulftd_ctl_ioctl;
    unsigned long       ulftd_lg_send_lg_message;
    unsigned long       ulftd_lg_oldest_entries;
    unsigned long       ulftd_lookup_dev;
    unsigned long       ulftd_lg_migrate;
    unsigned long       ulftd_lg_get_dirty_bits;
    unsigned long       ulftd_lg_get_dirty_bit_info;
    unsigned long       ulftd_lg_set_dirty_bits;
    unsigned long       ulftd_lg_update_dirtybits;
    unsigned long       ulftd_lg_ioctl;
    unsigned long       ulftd_ctl_del_lg;

    //
    // ftd_klog.c
    //
    unsigned long       ulmain;
    unsigned long       ulcmn_err;
    unsigned long       ulftd_err;
    unsigned long       ul_sprintf;
    unsigned long       ulksprintn;
    unsigned long       ulkvprintf;

    //
    // ftd_nt.c
    //
    unsigned long       ulExceptionFilter;
    unsigned long       ulExceptionFilterDontStop;
    unsigned long       ulFindQueuedRequest;
    unsigned long       ulSyncCancelRequest;
    unsigned long       ulftd_sync_thread;
    unsigned long       ulftd_commit_thread;
    unsigned long       ulftd_write_pstore;
    unsigned long       ulftd_chk_pstore_list;
    unsigned long       ulftd_pstore_thread;
    unsigned long       ulDriverEntry;
    unsigned long       ulftd_unload;
    unsigned long       ulftd_shutdown_flush;
    unsigned long       ulftd_create_dir;
    unsigned long       ulftd_del_dir;
    unsigned long       ulftd_init_ptrs;
    unsigned long       ulftd_get_registry;
    unsigned long       ulftd_ctl_get_device_nums;
    unsigned long       ulfinish_biodone;
    unsigned long       ulftd_dev_dispatch;
    unsigned long       ulftd_ioctl;
    unsigned long       ulftd_nt_lg_close;
    unsigned long       ulftd_nt_lg_open;
    unsigned long       ulftd_open;
    unsigned long       ulftd_close;
    unsigned long       ulftd_iodone_generic;
    unsigned long       ulftd_wakeup;
    unsigned long       ulftd_iodone_journal;
    unsigned long       ulftd_do_read;
    unsigned long       ulftd_do_write;
    unsigned long       ulftd_flush_lrdb;
    unsigned long       ulftd_nt_del_lg;
    unsigned long       ulftd_nt_start_lg_thread;
    unsigned long       ulftd_nt_add_lg;
    unsigned long       ulftd_nt_detach_device;
    unsigned long       ulftd_nt_del_device;
    unsigned long       ulftd_nt_start_dev_thread;
    unsigned long       ulftd_nt_add_device;
    unsigned long       ulftd_find_device;
    unsigned long       ulftd_find_file;
    unsigned long       ulftd_init_dev_ext;
    unsigned long       ulftd_init_ext_ext;
    unsigned long       ulftd_init_lg_ext;

    //
    // mapmem.c
    //
    unsigned long       ulMapMemMapTheMemory;
    unsigned long       ulMapMemUnMapTheMemory;

    //
    // memset.c
    //
    unsigned long       ulftd_memset;

    /*
    //
    // Locks & Semaphores
    //
    unsigned long       ul;
    unsigned long       ul;
    unsigned long       ul;
    unsigned long       ul;
    unsigned long       ul;
    unsigned long       ul;
    unsigned long       ul;
    unsigned long       ul;
    unsigned long       ul;
    unsigned long       ul;
    */
    unsigned long       ulmutex1;
    unsigned long       ulmutex2;
    unsigned long       ulfalsespinlock;
    unsigned long       ulCurrentReads;
    unsigned long       ulCurrentWrites;

} ftd_driver_status_t;

extern ftd_driver_status_t CUR_STATUS;

#if _DEBUG
#define IN_FCT(fctname) \
    InterlockedIncrement(&CUR_STATUS.ul##fctname);

#define OUT_FCT(fctname) \
    InterlockedDecrement(&CUR_STATUS.ul##fctname);
#else
#define IN_FCT(fctname) 
#define OUT_FCT(fctname) 
#endif

#define _NEW_OPERATION_         InterlockedIncrement(&CUR_STATUS.ulCurrentOperationsPending);
#define _NEW_OPERATION_ENDS     InterlockedDecrement(&CUR_STATUS.ulCurrentOperationsPending);

#define ALLOC_MUTEX(lock, str)          ExInitializeResourceLite(&lock)

extern unsigned int gDbgTick;


#if _OLD_METHOD_

#define PRE_ACQUIRE_MUTEX               BOOLEAN     bAcquiredMutex  = FALSE;\
                                        KIRQL       MUTEXOldIrql;
#define PRE_ACQUIRE_MUTEX_2             BOOLEAN     bAcquiredMutex2  = FALSE;\
                                        KIRQL       MUTEXOldIrql2;
       
#define ACQUIRE_MUTEX(lock,irql)        \
    {\
        _NEW_OPERATION_\
        IN_FCT(mutex1)\
        if ( FALSE == ExIsResourceAcquiredExclusiveLite(&lock) )\
        {\
            MUTEXOldIrql =  KeGetCurrentIrql();\
            if (PASSIVE_LEVEL == MUTEXOldIrql)\
            {\
                KeRaiseIrql(APC_LEVEL,&MUTEXOldIrql);\
            }\
            ExAcquireResourceExclusiveLite(&lock, TRUE);\
            irql = ExGetCurrentResourceThread();\
            bAcquiredMutex = TRUE;\
        }\
        else\
        {\
            bAcquiredMutex = FALSE;\
        }\
     }

#define RELEASE_MUTEX(lock,irql)        \
    {\
        if ( TRUE == bAcquiredMutex ) \
        { \
            ExReleaseResourceForThreadLite(&lock, irql); \
            if (PASSIVE_LEVEL == MUTEXOldIrql)\
            {\
                KeLowerIrql(MUTEXOldIrql);\
            }\
        } \
        OUT_FCT(mutex1)\
        _NEW_OPERATION_ENDS\
    }

#define ACQUIRE_MUTEX_2(lock,irql)       \
    {\
        _NEW_OPERATION_\
        IN_FCT(mutex2)\
        if ( FALSE == ExIsResourceAcquiredExclusiveLite(&lock) )\
        {\
            MUTEXOldIrql2 =  KeGetCurrentIrql();\
            if (PASSIVE_LEVEL == MUTEXOldIrql2)\
            {\
                KeRaiseIrql(APC_LEVEL,&MUTEXOldIrql);\
            }\
            ExAcquireResourceExclusiveLite(&lock, TRUE);\
            irql = ExGetCurrentResourceThread();\
            bAcquiredMutex2 = TRUE;\
        }\
        else\
        {\
            bAcquiredMutex2 = FALSE;\
        }\
    }

#define RELEASE_MUTEX_2(lock,irql)       \
    {\
        if ( TRUE == bAcquiredMutex2 ) \
        { \
            ExReleaseResourceForThreadLite(&lock, irql); \
            if (PASSIVE_LEVEL == MUTEXOldIrql2)\
            {\
                KeLowerIrql(MUTEXOldIrql2);\
            }\
        } \
        OUT_FCT(mutex2)\
        _NEW_OPERATION_ENDS\
    }

#else

#define PRE_ACQUIRE_MUTEX               BOOLEAN     bAcquiredMutex  = FALSE;
#define PRE_ACQUIRE_MUTEX_2             BOOLEAN     bAcquiredMutex2  = FALSE;

#define PRE_ACQUIRE_MUTEX_FORCE         BOOLEAN     bAcquiredMutex  = TRUE;

#define ACQUIRE_MUTEX(m_Mutex,m_ThreadID)        \
    {\
        _NEW_OPERATION_\
        IN_FCT(mutex1)\
        if ( FALSE == ExIsResourceAcquiredExclusiveLite(&m_Mutex) )\
        {\
            KeEnterCriticalRegion();\
            ExAcquireResourceExclusiveLite(&m_Mutex, TRUE);\
            m_ThreadID = ExGetCurrentResourceThread();\
            bAcquiredMutex = TRUE;\
        }\
        else\
        {\
            bAcquiredMutex = FALSE;\
        }\
     }

#define RELEASE_MUTEX(m_Mutex,m_ThreadID)        \
    {\
        if ( TRUE == bAcquiredMutex ) \
        { \
            ExReleaseResourceForThreadLite(&m_Mutex, m_ThreadID); \
            KeLeaveCriticalRegion(); \
        } \
        OUT_FCT(mutex1)\
        _NEW_OPERATION_ENDS\
    }

#define ACQUIRE_MUTEX_2(m_Mutex,m_ThreadID)       \
    {\
        _NEW_OPERATION_\
        IN_FCT(mutex2)\
        if ( FALSE == ExIsResourceAcquiredExclusiveLite(&m_Mutex) )\
        {\
            KeEnterCriticalRegion();\
            ExAcquireResourceExclusiveLite(&m_Mutex, TRUE);\
            m_ThreadID = ExGetCurrentResourceThread();\
            bAcquiredMutex2 = TRUE;\
        }\
        else\
        {\
            bAcquiredMutex2 = FALSE;\
        }\
    }

#define RELEASE_MUTEX_2(m_Mutex,m_ThreadID)       \
    {\
        if ( TRUE == bAcquiredMutex2 ) \
        { \
            ExReleaseResourceForThreadLite(&m_Mutex, m_ThreadID); \
            KeLeaveCriticalRegion(); \
        } \
        OUT_FCT(mutex2)\
        _NEW_OPERATION_ENDS\
    }
#endif


#define DEALLOC_MUTEX(lock)             ExDeleteResourceLite(&lock)      

#ifdef _ERESOURCE_SPINS_

#define ALLOC_LOCK(lock, str)           

#define PREACQUIRE_LOCK                 

#define ACQUIRE_LOCK( m_Mutex,m_ThreadID)       \
    {\
        _NEW_OPERATION_\
        IN_FCT(falsespinlock)\
        KeEnterCriticalRegion();\
        ExAcquireResourceExclusiveLite(&m_Mutex, TRUE);\
        m_ThreadID = ExGetCurrentResourceThread();\
    }

#define RELEASE_LOCK(m_Mutex,m_ThreadID)       \
    {\
        ExReleaseResourceForThreadLite(&m_Mutex, m_ThreadID); \
        KeLeaveCriticalRegion(); \
        OUT_FCT(falsespinlock)\
        _NEW_OPERATION_ENDS\
    }
#define DEALLOC_LOCK(lock)


#else

#define PREACQUIRE_LOCK

#define ALLOC_LOCK(lock, str)           KeInitializeSpinLock(&lock)

#define ACQUIRE_LOCK(lock,irql)         \
    {\
        _NEW_OPERATION_\
        KeAcquireSpinLock(&lock, &irql);\
    }

#define RELEASE_LOCK(lock,irql)         \
    {\
         KeReleaseSpinLock(&lock, irql);\
        _NEW_OPERATION_ENDS\
    }
#define DEALLOC_LOCK(lock)

#endif

#define NOT_IMPLEMENETED        @@@; ERROR HERE: CODE NOT IMPLEMENTED @@@;


#if !defined(TRUE)
#define TRUE 1
#define FALSE 0
#endif /* !defined(TRUE) */

#if defined(LRDBSYNCH)

/*-
 * test whether LRT map IO is currently pending.
 * 
 * if not: 
 *   o mark LRT map busy.
 *   o initiate LRT map IO.
 * 
 * if so: 
 *   o mark LRT map wanted. 
 *   o return without initiating IO.
 *   o upon pending IO completion, 
 *     this will be sensed in biodone_psdev().
 *     from there, another IO will be initiated.
 */

/* LRT map is available */
#define LRDB_FREE_FREE 0x00000001

/*- 
 * LRT map not available, 
 * there is a previous IO pending.
 * initiate another IO upon 
 * completion of the pending IO.
 */
#define LRDB_STAT_WANT 0x00000001

#define LRDB_FREE_SYNCH(l) {\
    int nval=0; \
    int oval=LRDB_FREE_FREE; \
    if(compare_and_swap(&((l)->lrdb_free), &oval, nval) == FALSE) { \
        /* FTD_ERR(FTD_WRNLVL, "LRDB_FREE_SYNCH: busy"); */ \
        (l)->lrdb_busycnt++; \
        fetch_and_or(&((l)->lrdb_stat), LRDB_STAT_WANT); \
        return(0); \
    } \
    else { \
        /* FTD_ERR(FTD_WRNLVL, "LRDB_FREE_SYNCH: free"); */ \
        (l)->lrdb_freecnt++; \
    } \
}

/* unset LRT map free */
#define LRDB_FREE_ACK(l) (l)->lrdb_free = LRDB_FREE_FREE 

/* unset LRT map want */
#define LRDB_WANT_ACK(l) fetch_and_and(&((l)->lrdb_stat), LRDB_STAT_WANT)

#endif /* defined(LRDBSYNCH) */

/* debugging support */
#if defined(FTD_PRIVATE)
#undef FTD_PRIVATE
#endif /* defined(FTD_PRIVATE) */
#if defined(FTD_DEBUG)
#define FTD_PRIVATE
#else
#define FTD_PRIVATE static
#endif /* defined(FTD_DEBUG) */

#endif  /* _FTD_DEF_H */
