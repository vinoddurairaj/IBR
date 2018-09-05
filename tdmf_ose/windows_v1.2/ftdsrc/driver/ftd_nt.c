/*
 * ftd_nt.c - FullTime Data driver for NT
 *
 * Copyright (c) 1996, 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * $Id: ftd_nt.c,v 1.9 2004/02/20 23:42:04 szg00 Exp $
 *
 */

/*
 * Includes, Declarations and Local Data
 */
#include            "NTDDK.h"
#include            "ftd_def.h"
#include            "ftd_nt.h"
#include            "ftd_bab.h"
#include            "ftd_ddi.h"
#include            "ftd_all.h"
#include            "ftd_bits.h"
#include            "ftd_klog.h"

#ifndef NTFOUR    
#pragma message(__LOC__ "COMPILING W2K/W2K3/XP\n")
#else
#pragma message(__LOC__ "COMPILING NT4\n")
#endif

#ifndef _ERESOURCE_SPINS_
#pragma message(__LOC__ "NORMAL SPIN LOCKS ARE IN USE\n")
#else
#pragma message(__LOC__ "SPIN LOCKS ARE ERESOURCES\n")
#endif
#if DBG
#pragma message(__LOC__ "DEBUG DRIVER")
#else
#pragma message(__LOC__ "NON DEBUG DRIVER")
#endif

#undef DRIVERNAME
#define DRIVERNAME  "dtcblock"          // compile hac

typedef struct ftd_ext {
    ftd_node_id_t       NodeIdentifier;
} ftd_ext_t;

typedef struct ftd_lg_io
{
    PIRP            Irp;
    ftd_dev_t       *softp;
    wlheader_t      *hp;
    LIST_ENTRY      ListEntry;
} ftd_lg_io_t;

//
//
// The one and only status structure! GLOBAL! 
// 
//
ftd_driver_status_t CUR_STATUS;



/*
 * Local Function Prototypes
 */
static NTSTATUS ftd_detach_target(PDEVICE_OBJECT SourceDeviceObject, ftd_dev_t *softp);
static FTD_STATUS ftd_create_dir(PWCHAR DirectoryNameStr, PVOID PtrReturnedObject);
static FTD_STATUS ftd_del_dir(PWCHAR DirectoryNameStr, PVOID PtrObject);
static NTSTATUS ftd_get_registry(PUNICODE_STRING RegistryPath);
static VOID ftd_init_ptrs(PDRIVER_OBJECT DriverObject);
static NTSTATUS ftd_shutdown_flush(PDEVICE_OBJECT DeviceObject, PIRP Irp);
static VOID ftd_unload(PDRIVER_OBJECT DriverObject);
static FTD_STATUS ftd_dev_dispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);

static void ftd_init_dev_ext(ftd_dev_t *softp, DEVICE_OBJECT *AssociatedDeviceObject, unsigned int NodeType);
static void ftd_init_ext_ext(ftd_ext_t *extension, unsigned int NodeType);
static void ftd_init_lg_ext(ftd_lg_t *lgp, unsigned int NodeType);

static FTD_STATUS ftd_dev_ioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
static FTD_STATUS ftd_ioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
static FTD_STATUS ftd_open(PDEVICE_OBJECT DeviceObject, PIRP Irp);
static FTD_STATUS ftd_close(PDEVICE_OBJECT DeviceObject, PIRP Irp);
static FTD_STATUS ftd_do_read(PDEVICE_OBJECT DeviceObject, PIRP Irp);
static FTD_STATUS ftd_do_write(PDEVICE_OBJECT DeviceObject, PIRP Irp);
static FTD_STATUS ftd_find_device(char *devname, PDEVICE_OBJECT *diskDeviceObject, PDEVICE_OBJECT *targeDeviceObject);
static FTD_STATUS ftd_find_file(char *devname, ftd_lg_t *extension);

/* one big ass buffer */
ftd_int64_t *big_ass_buffer;
int         big_ass_buffer_size;

#if DBG
ULONG   FTDDebugLevel = 0;
#endif

static BOOLEAN 
ExceptionFilter(PEXCEPTION_POINTERS ExceptionInformation,
                               ULONG ExceptionCode)
{
    IN_FCT(ExceptionFilter)
    DbgPrint("*************************************************************\n");
    DbgPrint("** Exception Caught in TDMFBlock driver!                   **\n");
    DbgPrint("**	                                                     **\n");
    DbgPrint("** Execute the following windbg commands:                  **\n");
    DbgPrint("**     !exr %x ; !cxr %x ; !kb                 **\n",
            ExceptionInformation->ExceptionRecord, ExceptionInformation->ContextRecord);
    DbgPrint("**	                                                     **\n");
    DbgPrint("*************************************************************\n");


    __try 
    {

            FTDBreakPoint();
    } 
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        // nothing
    }

    DbgPrint("*************************************************************\n");
    DbgPrint("** Continuing past break point.                            **\n");
    DbgPrint("*************************************************************\n");

    OUT_FCT(ExceptionFilter)
    return EXCEPTION_EXECUTE_HANDLER;
}

static BOOLEAN 
ExceptionFilterDontStop(PEXCEPTION_POINTERS ExceptionInformation,
                        ULONG ExceptionCode)
{
    IN_FCT(ExceptionFilterDontStop)
    DbgPrint("*************************************************************\n");
    DbgPrint("** Exception Caught in TDMFBlock driver!                   **\n");
    DbgPrint("**                                                         **\n");
    DbgPrint("** Execute the following windbg commands:                  **\n");
    DbgPrint("**     !exr %x ; !cxr %x ; !kb                 **\n",
            ExceptionInformation->ExceptionRecord, ExceptionInformation->ContextRecord);
    DbgPrint("**                                                         **\n");
    DbgPrint("*************************************************************\n");

    OUT_FCT(ExceptionFilterDontStop)
    return EXCEPTION_EXECUTE_HANDLER;
}




PIRP
FindQueuedRequest(PLIST_ENTRY ListHead)
{
    PLIST_ENTRY ListEntry, Next;
    PIRP        Irp = NULL;

    ftd_ctl_t   *ctlp = ftd_global_state;

    IN_FCT(FindQueuedRequest)

    ASSERT(ctlp);

    for (ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = Next) 
    {

        ftd_lg_io_t     *iop;
        
        iop = CONTAINING_RECORD( ListEntry, ftd_lg_io_t, ListEntry );

        Next = ListEntry->Flink;
        
        if ( iop->Irp && iop->Irp->Cancel) 
        {
            RemoveEntryList(ListEntry);

            Irp = iop->Irp;

            ExFreeToNPagedLookasideList(&ctlp->LGIoLookasideListHead, iop);

            OUT_FCT(FindQueuedRequest)
            return Irp;
        }
    }

    OUT_FCT(FindQueuedRequest)
    return Irp;
}

VOID
SyncCancelRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel any sync request in the driver.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{
    FTD_CONTEXT context;

    u_int               bytes;
    ftd_uint64_t        *temp;
    wlheader_t          *wl;
    bab_buffer_t        *buf;

    ftd_dev_t*  softp = (ftd_dev_t*)(DeviceObject->DeviceExtension);
    PRE_ACQUIRE_MUTEX

    IN_FCT(SyncCancelRequest)

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // Pull it off the queue and complete it as canceled.
    //
    ACQUIRE_MUTEX(softp->lgp->mutex, context);
    if ( (softp->lgp->mgr == NULL) ||
        (softp->lgp->mgr->in_use_head == NULL) )
    {
        RELEASE_MUTEX(softp->lgp->mutex, context);

        OUT_FCT(SyncCancelRequest)
        return;
    }

    bytes = 0x7fffffff;
    BAB_MGR_FIRST_BLOCK(softp->lgp->mgr, buf, temp);
    while (temp && bytes > 0) {
        if (softp->lgp->mgr->flags & FTD_BAB_PHYSICAL) 
        {
            wl = ftd_bab_map_memory(temp, sizeof(wlheader_t));
        } 
        else 
        {
            wl = (wlheader_t *) temp;
        }

        if (wl->majicnum != DATASTAR_MAJIC_NUM) 
        {
            FTD_ERR(FTD_WRNLVL, "LG %d WriteLog header corrupt, (SyncCancelRequest).", softp->lgp->dev & ~FTD_LGFLAG);

            if (softp->lgp->mgr->flags & FTD_BAB_PHYSICAL)
                ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
        
            break;
        }

        if (wl->bp && wl->bp->Cancel) 
        {
            FTD_ERR(FTD_WRNLVL, "LG %d Sync Cancel, device = %d.", softp->lgp->dev & ~FTD_LGFLAG, wl->dev);

            IoSetCancelRoutine(wl->bp, NULL);

            wl->bp->IoStatus.Status = STATUS_CANCELLED;
            wl->bp->IoStatus.Information = 0;

            IoCompleteRequest(wl->bp, IO_NO_INCREMENT);

            wl->bp = NULL;
        }

        bytes -= FTD_WLH_BYTES(wl);

        BAB_MGR_NEXT_BLOCK(temp, wl, buf);
        
        if (softp->lgp->mgr->flags & FTD_BAB_PHYSICAL)
            ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
    }
    RELEASE_MUTEX(softp->lgp->mutex, context);

    OUT_FCT(SyncCancelRequest)

}

static VOID
ftd_sync_thread(PVOID ext)
{
    FTD_STATUS          RC = STATUS_SUCCESS;
    FTD_CONTEXT         context;

    LARGE_INTEGER       largeInt, CurrentTime;
    time_t              seconds;

    u_int               bytes;
    ftd_uint64_t        *temp;
    wlheader_t          *wl;
    bab_buffer_t        *buf;
    PRE_ACQUIRE_MUTEX

    ftd_lg_t            *lgp = (ftd_lg_t*) ext;

    IN_FCT(ftd_sync_thread)

    ASSERT(lgp);
    
    DbgPrint("Starting thread ftd_sync_thread , tid = %04x \n" , PsGetCurrentThreadId() );
    
    largeInt.QuadPart = -(10*1000*1000);  // relative 1 second

    //
    // Set thread priority to lowest realtime level.
    //

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while (TRUE) 
    {
        //
        // KeWaitForSingleObject won't return error here
        KeWaitForSingleObject(
            (PVOID) &lgp->SyncThreadSemaphore,
            Executive,
            KernelMode,
            FALSE,
            &largeInt );
        ACQUIRE_MUTEX(lgp->mutex, context);

        if ( (lgp->sync_depth == (unsigned int) -1) ||
            (lgp->mgr == NULL) ||
            (lgp->mgr->in_use_head == NULL) )
        {
            RELEASE_MUTEX(lgp->mutex, context);

            if ( lgp->ThreadShouldStop )
            {
                //
                // When calling psterminatesystemthread, nothing
                // else gets executed!! make sure we decrement 
                // ourselves first
                //
                OUT_FCT(ftd_sync_thread)
                PsTerminateSystemThread( STATUS_SUCCESS );
            }
            else
            {
                continue;
            }
        }

        KeQuerySystemTime(&CurrentTime);
        seconds = (LONG) (CurrentTime.QuadPart / (ULONG)(10*1000*1000));

        bytes = 0x7fffffff;
        BAB_MGR_FIRST_BLOCK(lgp->mgr, buf, temp);
        while (temp && bytes > 0) 
        {
            if (lgp->mgr->flags & FTD_BAB_PHYSICAL) 
            {
                wl = ftd_bab_map_memory(temp, sizeof(wlheader_t));
            } 
            else 
            {
                wl = (wlheader_t *) temp;
            }
            if (wl->majicnum != DATASTAR_MAJIC_NUM) 
            {
                FTD_ERR(FTD_WRNLVL, "LG %d WriteLog header corrupt, (ftd_sync_thread).", lgp->dev & ~FTD_LGFLAG);
    
                if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                    ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
        
                break;
            }
            if (wl->complete && wl->bp) 
            {
                if ( lgp->ThreadShouldStop ||
                    (seconds > ((time_t)lgp->sync_timeout + wl->timestamp)) ) 
                {
                    IoSetCancelRoutine(wl->bp, NULL);

                    IoCompleteRequest(wl->bp, IO_NO_INCREMENT);

                    FTD_ERR(FTD_WRNLVL, "LG %d Syncmode timeout, device = %d.", lgp->dev & ~FTD_LGFLAG, wl->dev);
            
                    wl->bp = 0;
                }
                else
                {
                    if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                        ftd_bab_unmap_memory(wl, sizeof(wlheader_t));

                    break;
                }
            }

            bytes -= FTD_WLH_BYTES(wl);
        
            BAB_MGR_NEXT_BLOCK(temp, wl, buf);

            if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
        }
        RELEASE_MUTEX(lgp->mutex, context);
    
        if ( lgp->ThreadShouldStop ) 
        {
            //
            // When calling psterminatesystemthread, nothing
            // else gets executed!! make sure we decrement 
            // ourselves first
            //
            OUT_FCT(ftd_sync_thread)
            PsTerminateSystemThread( STATUS_SUCCESS );
        }
    }
    OUT_FCT(ftd_sync_thread)
}

static VOID
ftd_commit_thread(PVOID ext)
{
    FTD_STATUS          RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION  PtrCurrentStackLocation = NULL;
    PLIST_ENTRY         ListEntry;
    
    FTD_CONTEXT         context;
    FTD_IRQL            oldIrql;

    LARGE_INTEGER       CurrentTime;

    int                 length;
    BOOLEAN             dowakeup = FALSE;
    BOOLEAN             dodone = FALSE;

    ftd_uint64_t        *buf;
    ftd_lg_io_t         *iop;
    PIRP                Irp;
    ftd_dev_t           *softp;
    wlheader_t          *hp;
    bab_mgr_t           *mgr;
    ftd_ctl_t           *ctlp;
    PRE_ACQUIRE_MUTEX

    PREACQUIRE_LOCK

    ftd_lg_t            *lgp = (ftd_lg_t*) ext;

    IN_FCT(ftd_commit_thread)

    ASSERT(lgp);


    ctlp = ftd_global_state;
    ASSERT(ctlp);

    mgr = lgp->mgr;
    ASSERT(mgr);

    DbgPrint("Starting thread ftd_commit_thread , tid = %04x \n" , PsGetCurrentThreadId() );
    //
    // Set thread priority to lowest realtime level.
    //

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while (TRUE) 
    {

        //
        // KeWaitForSingleObject won't return error here - this thread
        // isn't alertable and won't take APCs, and we're not passing in
        // a timeout.
        //
        KeWaitForSingleObject(
            (PVOID) &lgp->CommitQueueSemaphore,
            Executive,
            KernelMode,
            FALSE,
            NULL );

        //
        // While we are manipulating the queue we capture the
        // spin lock.
        //
        ACQUIRE_LOCK(lgp->lock, oldIrql);

        while ( !IsListEmpty(&lgp->CommitQueueListHead) ) 
        {
            ListEntry = RemoveHeadList(&lgp->CommitQueueListHead);
            iop = CONTAINING_RECORD( ListEntry, ftd_lg_io_t, ListEntry );

            RELEASE_LOCK(lgp->lock, oldIrql);

            Irp = iop->Irp;
            softp = iop->softp;
            if (lgp->mgr->flags & FTD_BAB_PHYSICAL) 
            {
                buf = (ftd_uint64_t *)iop->hp;
                hp = ftd_bab_map_memory(iop->hp, sizeof(wlheader_t));
            } 
            else 
            {
                //
                // The buffer is in the list of irps that we have here
                //
                // This buffer is actually the pointer to the memory where this IO 
                // is located
                //
                hp = (wlheader_t *)(buf = (ftd_uint64_t *)iop->hp);
            }

            ExFreeToNPagedLookasideList(&ctlp->LGIoLookasideListHead, iop);

            if ( lgp->ThreadShouldStop ) 
            {
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            else
            {
                int wlsectors; 

                // Get the current I/O stack location.
                PtrCurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
                ASSERT(PtrCurrentStackLocation);
                ACQUIRE_MUTEX(lgp->mutex, context);
                wlsectors = hp->length; 

                dowakeup = FALSE;
        
                /* update disk stats */
                ACQUIRE_LOCK(softp->lock, oldIrql);
                softp->writeiocnt++;
                softp->sectorswritten += (PtrCurrentStackLocation->Parameters.Write.Length >> DEV_BSHIFT);
                softp->wlentries++;
                softp->wlsectors += wlsectors;
                RELEASE_LOCK(softp->lock, oldIrql);

                /* update group stats */
                lgp->wlentries++;
                lgp->wlsectors += hp->length;

                if (    (lgp->flags & FTD_LG_OPEN) == 0 
                     || 
                        lgp->wlentries < lgp->sync_depth) 
                {    /* if less than the depth complete the IRP */
                    hp->bp = NULL;
                    dodone = TRUE;
                } 
                else 
                {
                    IoSetCancelRoutine(Irp, SyncCancelRequest);
            
                    hp->bp = Irp;

                    KeQuerySystemTime(&CurrentTime);
                    hp->timestamp = (LONG) (CurrentTime.QuadPart / (ULONG)(10*1000*1000));
                    
                    dodone = FALSE;
                }

                hp->complete = 1;

                length = FTD_WLH_QUADS(hp);

                if (mgr->flags & FTD_BAB_PHYSICAL)
                    ftd_bab_unmap_memory(hp, sizeof(wlheader_t));

                //
                // If the pending buffer is pointing to this buffer
                //
                if (ftd_bab_get_pending(mgr) == buf) 
                {
                    //
                    // commit this buffer!
                    //
                    ftd_bab_commit_memory(mgr, length);
                    
                    ASSERT(hp->group_ptr == lgp);

                    dowakeup = TRUE;
        
                    //
                    // The following code is only done in case:  
                    // - a "message"inserted while this IO and other IO's 
                    // were pending and this message could not complete itself.
                    //
                    // - IO's completed out of order, and were not commited
                    //
                    // because in any other case, hp->complete will never be 1
                    // and you cannot commit an IO that has not completed!
                    //

                    //
                    // if there is still pending memory
                    //
                    /* commit any completed requests past this one! */
                    while ((hp = (wlheader_t *)ftd_bab_get_pending(mgr)) != NULL) 
                    {
                        if (mgr->flags & FTD_BAB_PHYSICAL)
                            hp = ftd_bab_map_memory(hp, sizeof(wlheader_t));

                        if (hp->complete == 1) 
                        {
                            length = FTD_WLH_QUADS(hp);
                            ftd_bab_commit_memory(mgr, length); 

                            ASSERT(hp->group_ptr == lgp);
                        
                            if (mgr->flags & FTD_BAB_PHYSICAL)
                                ftd_bab_unmap_memory(hp, sizeof(wlheader_t));
                        } 
                        else 
                        {
                            if (mgr->flags & FTD_BAB_PHYSICAL)
                                ftd_bab_unmap_memory(hp, sizeof(wlheader_t));

                            break;
                        }
                    }
                }
                RELEASE_MUTEX(lgp->mutex, context);

                if (dowakeup)
                    ftd_wakeup(lgp);

                if (dodone)
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);

            }

            ACQUIRE_LOCK(lgp->lock, oldIrql);
        }
        RELEASE_LOCK(lgp->lock, oldIrql);
    
        if ( lgp->ThreadShouldStop ) 
        {
            //
            // When calling psterminatesystemthread, nothing
            // else gets executed!! make sure we decrement 
            // ourselves first
            //
            OUT_FCT(ftd_commit_thread)
            PsTerminateSystemThread( STATUS_SUCCESS );
        }
    }
    OUT_FCT(ftd_commit_thread)
}


#ifdef OLD_PSTORE
static FTD_STATUS
ftd_write_pstore(ftd_dev_t *softp, unsigned int *map)
{
    FTD_STATUS          RC = STATUS_SUCCESS;
    LARGE_INTEGER       largeInt;
    PIRP                newIrp;
    KEVENT              event;
    IO_STATUS_BLOCK     ioStatusBlock;

    IN_FCT(ftd_write_pstore)

    largeInt.QuadPart = ((ULONG)softp->lrdb_offset) << DEV_BSHIFT;  // to bytes

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Build IRP to write buffer
    //

    newIrp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                       softp->lgp->PStoreDeviceObject,
                                       map,
                                       softp->lrdb.len32 * sizeof(int),
                                       &largeInt,
                                       &event,
                                       &ioStatusBlock );

    if (newIrp) 
    {
        //
        // Pass request to port driver and wait for request to complete.
        //

        RC = IoCallDriver(softp->lgp->PStoreDeviceObject, newIrp);

        if (RC == STATUS_PENDING) 
        {
            KeWaitForSingleObject(&event,
                                  Suspended,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            RC = ioStatusBlock.Status;
        }
    }
    else
    {
        RC = STATUS_INSUFFICIENT_RESOURCES;
    }


    // if something failed log it, but go on
    if ( !NT_SUCCESS(RC) )
        FTD_ERR(FTD_WRNLVL, "Unable to write to the pstore, RC = %d.", RC);

    OUT_FCT(ftd_write_pstore)
    return RC;
}

#else

#pragma code_seg()



NTSYSAPI NTSTATUS NTAPI ZwWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

#if DBG
static unsigned int NO_PSTORE = 0;
static unsigned int count_pstore_writes = 0;
#endif

static FTD_STATUS
ftd_write_pstore(ftd_dev_t *softp, unsigned int *map)
{
    FTD_STATUS          RC = STATUS_SUCCESS;
    LARGE_INTEGER       largeInt, largeIntWait;
    OBJECT_ATTRIBUTES   objAttr;
    IO_STATUS_BLOCK     ioStatus;
    LPSTR               pBuffer_p;
    ULONG               dwSize_p;
    HANDLE              FileHandle = NULL;

    IN_FCT(ftd_write_pstore)

#if DBG
    //
    // Count the number of pstore accesses
    //
    count_pstore_writes++;

    //
    // Disable pstore access for tests (in debugger)
    //
    if (NO_PSTORE)
    {
        OUT_FCT(ftd_write_pstore)
        return RC;
    }
#endif

    largeInt.QuadPart = ((ftd_uint64_t)(softp->lrdb_offset)) << DEV_BSHIFT;  // to bytes

    largeIntWait.QuadPart = -(10*1000*1000);  // relative 1 second

    pBuffer_p = (LPSTR)map;
    dwSize_p = (ULONG)(softp->lrdb.len32 * sizeof(int));

    //
    // Initialize attributes
    //
#ifdef NTFOUR
    InitializeObjectAttributes(&objAttr, 
                               &softp->lgp->PStoreFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL, 
                                NULL);
#else
    InitializeObjectAttributes(&objAttr, 
                               &softp->lgp->PStoreFileName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, //sg
                                NULL, 
                                NULL);
#endif

    //
    // By adding FILE_SYNCHR... We make sure that we only 
    // return after a writefile when the write file is complete!
    //
    RC = ZwCreateFile( &FileHandle,
                        GENERIC_READ|SYNCHRONIZE|GENERIC_WRITE,   
                        &objAttr,
                        &ioStatus,
                        0, 
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        FILE_OPEN_IF, 
                        FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL, 
                        0);

    if ( !NT_SUCCESS(RC) ) 
    {
        DbgPrint("ftd_write_pstore : Unable to open the pstore!\n");
        FTD_ERR(FTD_WRNLVL, "Unable to open the pstore, RC = 0x%x.", RC);
        OUT_FCT(ftd_write_pstore)
        return RC;
    }

    RC = ZwWriteFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &ioStatus,
                        pBuffer_p,
                        dwSize_p,
                        &largeInt,
                        NULL
                        );

    if ( !NT_SUCCESS(RC) )
    {
        DbgPrint("ftp_write_pstore : Unable to write to the pstore RC=0x%lx!\n",RC);
            FTD_ERR(FTD_WRNLVL, "Unable to write to the pstore, RC = 0x%x.", RC);
    }
    else if (STATUS_PENDING == RC)
    {
        //
        // If status is pending, wait till the write has finished!
        //
        ZwWaitForSingleObject(FileHandle, FALSE, &largeIntWait);
    }

    RC = ZwClose(FileHandle);

    // if something failed log it, but go on
    if ( !NT_SUCCESS(RC) )
    {
        DbgPrint("ftd_write_pstore : Unable to close filehandle!\n");
        FTD_ERR(FTD_WRNLVL, "Unable to close to the pstore, RC = 0x%x.", RC);
    }

    OUT_FCT(ftd_write_pstore)
    return RC;
}

#endif

static VOID
ftd_chk_pstore_list(ftd_dev_t *softp)
{
    PLIST_ENTRY     QueueHead, ListEntry, Next;
    FTD_IRQL        oldIrql;
    FTD_CONTEXT     context;

    PREACQUIRE_LOCK

    ftd_ctl_t       *ctlp = ftd_global_state;

    IN_FCT(ftd_chk_pstore_list)

    ASSERT(ctlp);

    QueueHead = &softp->PStoreQueueListHead;

    ACQUIRE_LOCK(softp->lock, oldIrql);
    for (ListEntry = QueueHead->Flink;
         ListEntry != QueueHead;
         ListEntry = Next) 
    {

        ftd_lg_io_t     *iop;
        PIRP            Irp;
        
        iop = CONTAINING_RECORD( ListEntry, ftd_lg_io_t, ListEntry );
        Irp = iop->Irp;

        Next = ListEntry->Flink;
        
        if ( (Irp != NULL) && !ftd_check_lrdb(softp, Irp)) 
        {

            RemoveEntryList(ListEntry);

            //
            // Free the spin lock in case IoCompleteRequest doesn't
            // like us to hold it
            //

            RELEASE_LOCK(softp->lock, oldIrql);

            ExFreeToNPagedLookasideList(&ctlp->LGIoLookasideListHead, iop);
            IoCallDriver(softp->TargetDeviceObject, Irp);

            ACQUIRE_LOCK(softp->lock, oldIrql);

            //
            // The world may have changed so restart
            //

            Next = QueueHead->Flink;
        }
    }
    RELEASE_LOCK(softp->lock, oldIrql);
    OUT_FCT(ftd_chk_pstore_list)
}

#pragma code_seg()

static VOID
ftd_pstore_thread(PVOID ext)
{
    FTD_STATUS      RC = STATUS_SUCCESS;
    LIST_ENTRY      ListHead, *ListEntry;
    FTD_IRQL        oldIrql;

    ftd_lg_io_t     *iop;
    PIRP            Irp;
    
    unsigned int    *map;
    ftd_ctl_t       *ctlp;

    ftd_dev_t       *softp = (ftd_dev_t*) ext;

    PREACQUIRE_LOCK

    IN_FCT(ftd_pstore_thread)

    ctlp = ftd_global_state;
    ASSERT(ctlp);

    DbgPrint("Starting PSTORE thread , tid  = %04x  IRQL = %d\n" , PsGetCurrentThreadId() ,KeGetCurrentIrql());

    map = ExAllocatePool(NonPagedPool, softp->lrdb.len32 * sizeof(int));

    //
    // Prepare the work list
    //
    InitializeListHead( &ListHead );

    //
    // Set thread priority to lowest realtime level.
    //

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while (TRUE) 
    {

        //
        // Wait for a request from the dispatch routines.
        // KeWaitForSingleObject won't return error here - this thread
        // isn't alertable and won't take APCs, and we're not passing in
        // a timeout.
        //
        try 
        {
            KeWaitForSingleObject(
                (PVOID) &softp->PStoreQueueSemaphore,
                Executive,
                KernelMode,
                FALSE,
                NULL );

        } 
        except (ExceptionFilterDontStop(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            DbgPrint( "KeWaitForSingleObject() crashed in ftd_pstore_thread() : EXCEPTION_EXECUTE_HANDLER, RC = 0x%lx.\n", GetExceptionCode() );
        }

        // if lrdb bits are set, complete the IRPs
        ftd_chk_pstore_list(softp);

        // do we want to quit?
        if ( softp->ThreadShouldStop ) 
        {
        
            ACQUIRE_LOCK(softp->lock, oldIrql);

            while ( !IsListEmpty(&softp->PStoreQueueListHead) ) 
            {
                ListEntry = RemoveHeadList(&softp->PStoreQueueListHead);

                iop = CONTAINING_RECORD( ListEntry, ftd_lg_io_t, ListEntry );
                Irp = iop->Irp;

                if (Irp) 
                {
                    ftd_update_lrdb(softp->lrdb.map, softp->lrdb.shift, Irp);
                    RELEASE_LOCK(softp->lock, oldIrql);
                
                    ftd_write_pstore(softp, softp->lrdb.map);

                    IoCallDriver(softp->TargetDeviceObject, Irp);
                }
                else 
                {
                    RELEASE_LOCK(softp->lock, oldIrql);
                    
                    // special case, just flush to the pstore
                    ftd_write_pstore(softp, softp->lrdb.map);
                }

                ExFreeToNPagedLookasideList(&ctlp->LGIoLookasideListHead, iop);

                ACQUIRE_LOCK(softp->lock, oldIrql);
            }
            RELEASE_LOCK(softp->lock, oldIrql);
            ExFreePool(map);

            //
            // When calling psterminatesystemthread, nothing
            // else gets executed!! make sure we decrement 
            // ourselves first
            //
            OUT_FCT(ftd_pstore_thread)
            PsTerminateSystemThread( STATUS_SUCCESS );
            break;
        } // if(threadshouldstop)
        
        // If there is anything left on the list, create a PStore I/O
        ACQUIRE_LOCK(softp->lock, oldIrql);
        if ( !IsListEmpty(&softp->PStoreQueueListHead) ) 
        {
            ListEntry = RemoveHeadList(&softp->PStoreQueueListHead);

            // get the ptrs and put back on the list
            iop = CONTAINING_RECORD( ListEntry, ftd_lg_io_t, ListEntry );
            Irp = iop->Irp;
            
            // special case, flush
            if (Irp == NULL) 
            {
                RELEASE_LOCK(softp->lock, oldIrql);
    
                ExFreeToNPagedLookasideList(&ctlp->LGIoLookasideListHead, iop);

                ftd_write_pstore(softp, softp->lrdb.map);
            }
            else 
            {
                RtlCopyMemory(map, softp->lrdb.map, softp->lrdb.len32);
                RELEASE_LOCK(softp->lock, oldIrql);

                ExFreeToNPagedLookasideList(&ctlp->LGIoLookasideListHead, iop);

                ftd_update_lrdb(map, softp->lrdb.shift, Irp);
                if ( NT_SUCCESS(ftd_write_pstore(softp, map)) ) 
                {
                    ACQUIRE_LOCK(softp->lock, oldIrql);
                    ftd_update_lrdb(softp->lrdb.map, softp->lrdb.shift, Irp);
                    RELEASE_LOCK(softp->lock, oldIrql);
                }
                else
// XXXX FIXME, if the write to the pstore fails, we should do something????
                {
                    ACQUIRE_LOCK(softp->lock, oldIrql);
                    ftd_update_lrdb(softp->lrdb.map, softp->lrdb.shift, Irp);
                    RELEASE_LOCK(softp->lock, oldIrql);
                }

                // Complete this guy
                IoCallDriver(softp->TargetDeviceObject, Irp);
                
                // check for some more
                ftd_chk_pstore_list(softp);
            }
                
            ACQUIRE_LOCK(softp->lock, oldIrql);
        }
  
        RELEASE_LOCK(softp->lock, oldIrql);
    }
    OUT_FCT(ftd_pstore_thread)
}


/*************************************************************************
*
* Function: DriverEntry()
*
* Description:
*   This routine is the standard entry point for most kernel mode drivers.
*   The routine is invoked at IRQL PASSIVE_LEVEL in the context of a
*   system worker thread.
*   All FTD driver specific data structures etc. are initialized here.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error (will cause driver to be unloaded).
*
*************************************************************************/
NTSTATUS DriverEntry(
PDRIVER_OBJECT      DriverObject,       // created by the I/O sub-system
PUNICODE_STRING     RegistryPath)       // path to the registry key
{
    NTSTATUS                        RC = STATUS_SUCCESS;
    BOOLEAN                         RegisteredShutdown = FALSE;
    UNICODE_STRING                  DriverDeviceName;
    HANDLE                          threadHandle;

    ftd_ctl_t                       *ctlp = NULL;
    ftd_ext_t                       *extension = NULL;

    IN_FCT(DriverEntry)

    //DbgPrint("Driver was created at:%s\n",__TIMESTAMP__);  

    try 
    {
        try 
        {
            // init some string space
            RtlInitUnicodeString(&DriverDeviceName, NULL);
            
            ctlp = ftd_global_state = kmem_zalloc(sizeof(ftd_ctl_t), KM_NOSLEEP);
            if (ctlp == NULL) 
            {
                try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
            }

            FTD_ERR(FTD_WRNLVL, "DriverEntry(), Cv:%s \n" , __TIMESTAMP__ );
            // initialize some required fields
            ctlp->NodeIdentifier.NodeType = FTD_NODE_TYPE_CTL_DATA;
            ctlp->NodeIdentifier.NodeSize = sizeof(ftd_ctl_t);

            // keep a ptr to the driver object sent to us by the I/O Mgr
            ctlp->DriverObject = DriverObject;

            ALLOC_LOCK(ctlp->lock, "Driver");

            // initialize the IRP major function table
            ftd_init_ptrs(DriverObject);

            // Get boot info
            if (!NT_SUCCESS(RC = ftd_get_registry(RegistryPath)) ) 
            {
                FTD_ERR(FTD_WRNLVL, "FATAL ERROR: Unable to read the registry information, RC = %d.", RC);

                try_return(RC);
            }

            // ftd_bab_init wants chunksize and numchunks as args.
            ctlp->bab_size = ftd_bab_init(ctlp->chunk_size, ctlp->num_chunks, ctlp->maxmem);
            if (ctlp->bab_size == 0) 
            {
                FTD_ERR(FTD_WRNLVL, "FATAL ERROR: Unable to allocate BAB memory\n");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
            }

            FTDSetFlag(ctlp->flags, FTD_CTL_FLAGS_BAB_CREATED);

            if (ddi_soft_state_init(&ftd_dev_state, sizeof(ftd_dev_t), 16) != 0) 
            {
                FTD_ERR(FTD_WRNLVL, "FATAL ERROR: Unable to init the device's state.");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
            }

            if (ddi_soft_state_init(&ftd_lg_state, sizeof(ftd_lg_t), 16) != 0) 
            {
                FTD_ERR(FTD_WRNLVL, "FATAL ERROR: Unable to init the logical group's state.");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
            }

            ExInitializeNPagedLookasideList(&ctlp->LGIoLookasideListHead,
                                        NULL,
                                        NULL,
                                        NonPagedPoolMustSucceed,
                                        sizeof(ftd_lg_io_t),
                                        'dtfZ',
                                        (USHORT)300);
            FTDSetFlag(ctlp->flags, FTD_CTL_FLAGS_LGIO_CREATED);

            // It would be nice to create directories   in which to create
            // named (temporary) ftd driver objects.

            // Start with a directory in the NT Object Name Space.
            if (!NT_SUCCESS(RC = ftd_create_dir(FTD_DRV_DIR,
                                                &(ctlp->DirectoryObject)))) 
            {
                FTD_ERR(FTD_WRNLVL, "FATAL ERROR: Unable to create the device directory structure, RC = %d.", RC);

                try_return(RC);
            }

            // create a device object representing the driver itself
            //  so that requests can be targeted to the driver ...
            DriverDeviceName.MaximumLength = sizeof(FTD_DRV_DIR) + sizeof(L"\\") + sizeof(FTD_CTL_NAME);
            DriverDeviceName.Buffer = ExAllocatePool(PagedPool,DriverDeviceName.MaximumLength + sizeof(UNICODE_NULL));
            if (!DriverDeviceName.Buffer) 
            {
                FTD_ERR(FTD_WRNLVL, "FATAL ERROR: Unable to allocate memory for the ctl device name.");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES); 
            }
            RtlZeroMemory(DriverDeviceName.Buffer, DriverDeviceName.MaximumLength + sizeof(UNICODE_NULL));
            RtlAppendUnicodeToString(&DriverDeviceName, FTD_DRV_DIR);
            RtlAppendUnicodeToString(&DriverDeviceName, L"\\");
            RtlAppendUnicodeToString(&DriverDeviceName, FTD_CTL_NAME);

            // Create the ctl kernel device
            if (!NT_SUCCESS(RC = IoCreateDevice(
                    ctlp->DriverObject,
                    sizeof(ftd_ext_t),
                    &DriverDeviceName,
                    FILE_DEVICE_UNKNOWN,    // For lack of anything better ?
                    0,
                    FALSE,  // Not exclusive.
                    &(ctlp->DeviceObject)))) 
            {
                // failed to create a device object, leave.
                FTD_ERR(FTD_WRNLVL, "FATAL ERROR: Unable to create the ctl kernel device, RC = %d.", RC);

                try_return(RC);
            }

            // Initialize the extension for the device object.
            extension = (ftd_ext_t *)(ctlp->DeviceObject->DeviceExtension);
            ftd_init_ext_ext(extension, FTD_NODE_TYPE_EXT_DEVICE);

            // In order to allow user-space helper applications to access our
            // device object for the filter driver, create a symbolic link to
            // the object.
            RtlInitUnicodeString(&ctlp->UserVisibleName, NULL);
            ctlp->UserVisibleName.MaximumLength = sizeof(FTD_DOS_DRV_DIR) + sizeof(L"\\") + sizeof(FTD_CTL_NAME);
            ctlp->UserVisibleName.Buffer = ExAllocatePool(NonPagedPool,ctlp->UserVisibleName.MaximumLength + sizeof(UNICODE_NULL));
            if (!ctlp->UserVisibleName.Buffer) 
            {
                FTD_ERR(FTD_WRNLVL, "FATAL ERROR: Unable to allocate memory for the Win32 ctl device name.");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES); 
            }
            RtlZeroMemory(ctlp->UserVisibleName.Buffer, ctlp->UserVisibleName.MaximumLength + sizeof(UNICODE_NULL));
            RtlAppendUnicodeToString(&ctlp->UserVisibleName, FTD_DOS_DRV_DIR);
            RtlAppendUnicodeToString(&ctlp->UserVisibleName, L"\\");
            RtlAppendUnicodeToString(&ctlp->UserVisibleName, FTD_CTL_NAME);

            // Now create a directory in the DOS (Win32) visible name space.
            if (!NT_SUCCESS(RC = ftd_create_dir(FTD_DOS_DRV_DIR,
                                                &(ctlp->DosDirectoryObject)))) 
            {
                FTD_ERR(FTD_WRNLVL, "FATAL ERROR: Unable to create the Win32 device directory, RC = %d.", RC);

                try_return(RC);
            }

            if (!NT_SUCCESS(RC = IoCreateSymbolicLink(&(ctlp->UserVisibleName), &DriverDeviceName))) 
            {
                FTD_ERR(FTD_WRNLVL, "FATAL ERROR: Unable to create the Win32 device symbolic name, RC = %d.", RC);

                try_return(RC);
            }
            FTDSetFlag(ctlp->flags, FTD_CTL_FLAGS_SYMLINK_CREATED);

        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "DriverEntry() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }

        try_exit:   NOTHING;
    } 
    finally 
    {
        if (NT_SUCCESS(RC)) 
        {
            FTD_ERR(FTD_WRNLVL, DRIVERNAME " loaded Cv:[%s]", __TIMESTAMP__);
        }
        else 
        {
            // Start unwinding if we were unsuccessful.

            if ( ctlp != NULL) 
            {

                if (ctlp->flags & FTD_CTL_FLAGS_SYMLINK_CREATED) 
                {
                    IoDeleteSymbolicLink(&ctlp->UserVisibleName);
                }

                if (ctlp->UserVisibleName.Buffer != NULL) 
                {
                    ExFreePool( ctlp->UserVisibleName.Buffer );
                }

                // Now, delete any device objects, etc. we may have created
                if (ctlp->DeviceObject) 
                {
                    // nothing (no-one) should be attached to our device
                    // object at this time.
                    IoDeleteDevice(ctlp->DeviceObject);
                }

                // Delete the directories we may have created.
                if (ctlp->DosDirectoryObject) 
                {
                    ftd_del_dir(FTD_DOS_DRV_DIR, ctlp->DosDirectoryObject);
                }

                if (ctlp->DirectoryObject) 
                {
                    ftd_del_dir(FTD_DRV_DIR, ctlp->DirectoryObject);
                }

                if (ctlp->flags & FTD_CTL_FLAGS_BAB_CREATED) 
                {
                    ftd_bab_fini(ctlp->maxmem);
                }

                if (ctlp->flags & FTD_CTL_FLAGS_LGIO_CREATED)
                    ExDeleteNPagedLookasideList(&ctlp->LGIoLookasideListHead);

                if (ftd_lg_state) 
                {
                    ddi_soft_state_fini(&ftd_lg_state);
                }

                if (ftd_dev_state) 
                {
                    ddi_soft_state_fini(&ftd_dev_state);
                }
                
                DEALLOC_LOCK(ctlp->lock);
    
                kmem_free(ctlp, sizeof(ftd_ctl_t));
            }
        } 

        if (DriverDeviceName.Buffer != NULL) 
        {
            ExFreePool( DriverDeviceName.Buffer );
        }
    }

    OUT_FCT(DriverEntry)
    return(RC);
}

#if DBG
/*
 *----------------------------------------------------------------------
 * ftd_unload
 *
 *  This function releases any resources allocated during driver initialization.
 *
 * Arguments:
 *  A pointer to the Driver object
 *
 * Results:
 *  None
 *
 *----------------------------------------------------------------------
 */
static VOID
ftd_unload(PDRIVER_OBJECT DriverObject)
{
    NTSTATUS                        RC = STATUS_SUCCESS;
    ftd_ctl_t                       *ctlp = NULL;

    IN_FCT(ftd_unload)
    
    try 
    {
        try 
        {

            // Get a pointer to the global data.
            if ((ctlp = ftd_global_state) == NULL) 
            {
                ASSERT(ctlp);
                OUT_FCT(ftd_unload)
                return;
            }
            
            /*
             * Kill them all!
             */
            while (ctlp->lghead) {
                if (ftd_del_lg(ctlp->lghead, getminor(ctlp->lghead->dev) & ~FTD_LGFLAG) != FTD_SUCCESS) 
                {
                    FTD_ERR(FTD_WRNLVL, "Unable to unload FTD, logical group(s) still active."); 
                    OUT_FCT(ftd_unload)
                    return;
                }
            }

            if (ctlp->flags & FTD_CTL_FLAGS_SYMLINK_CREATED) 
            {
                IoDeleteSymbolicLink(&ctlp->UserVisibleName);
            }

            if (ctlp->UserVisibleName.Buffer != NULL) 
            {
                ExFreePool( ctlp->UserVisibleName.Buffer );
            }

            // Now, delete any device objects, etc. we may have created
            if (ctlp->DeviceObject) {
                // nothing (no-one) should be attached to our device
                // object at this time.
                IoDeleteDevice(ctlp->DeviceObject);
            }

            // Delete the directories we may have created.
            if (ctlp->DosDirectoryObject) {
                ftd_del_dir(FTD_DOS_DRV_DIR, ctlp->DosDirectoryObject);
            }
            if (ctlp->DirectoryObject) {
                ftd_del_dir(FTD_DRV_DIR, ctlp->DirectoryObject);
            }

            if (ctlp->flags & FTD_CTL_FLAGS_BAB_CREATED) {
                ftd_bab_fini(ctlp->maxmem);
            }

            if (ctlp->flags & FTD_CTL_FLAGS_LGIO_CREATED)
                ExDeleteNPagedLookasideList(&ctlp->LGIoLookasideListHead);

            // need to do this here, before the free
            FTD_ERR(FTD_WRNLVL, DRIVERNAME " unloaded."); 
            
            if (ftd_lg_state)
                ddi_soft_state_fini(&ftd_lg_state);

            if (ftd_dev_state)
                ddi_soft_state_fini(&ftd_dev_state);

            DEALLOC_LOCK(ctlp->lock);

            kmem_free(ctlp, sizeof(ftd_ctl_t));

        } except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_unload() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }
    } 
    finally 
    {
    }

    OUT_FCT(ftd_unload)
}
#endif


static NTSTATUS
ftd_shutdown_flush(PDEVICE_OBJECT DeviceObject, PIRP Irp)

/*++

Routine Description:

    This routine is called for a shutdown and flush IRPs.  These are sent by the
    system before it actually shuts down or when the file system does a flush.

Arguments:

    DriverObject - Pointer to device object to being shutdown by system.
    Irp          - IRP involved.

Return Value:

    NT Status

--*/

{
    FTD_CONTEXT             context;
    FTD_STATUS              RC;
    ftd_dev_t *softp;

    IN_FCT(ftd_shutdown_flush)

    // Get a pointer to the group extension
    softp = (ftd_dev_t *)(DeviceObject->DeviceExtension);
    FTDAssertDevExtPtrValid(softp);

    //
    // Set current stack back one.
    //

    Irp->CurrentLocation++,
    Irp->Tail.Overlay.CurrentStackLocation++;

    RC = IoCallDriver(softp->TargetDeviceObject, Irp);

    OUT_FCT(ftd_shutdown_flush)
    return RC;

} // end DiskPerfShutdownFlush()


/*************************************************************************
*
* Function: ftd_create_dir()
*
* Description:
*   Create a directory in the object name space. Make the directory
*   a temporary object if requested by the caller.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/an appropriate error
*
*************************************************************************/
static FTD_STATUS 
ftd_create_dir(PWCHAR DirectoryNameStr, PVOID PtrReturnedObject)
{
    NTSTATUS                        RC = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES               DirectoryAttributes;
    UNICODE_STRING                  DirectoryName;
    HANDLE                          hnd;
    BOOLEAN                         bHandleCreated = FALSE;

    IN_FCT(ftd_create_dir)

    try {
        // Create a Unicode string.
        RtlInitUnicodeString(&DirectoryName, DirectoryNameStr);

        // Create an object attributes structure.
        InitializeObjectAttributes(&DirectoryAttributes,
                                    &DirectoryName, OBJ_PERMANENT,
                                    NULL, NULL);
    
        // The following call will fail if we do not have appropriate privileges.
        if (!NT_SUCCESS(RC = ZwCreateDirectoryObject(&hnd,
                                        DIRECTORY_ALL_ACCESS,
                                        &DirectoryAttributes)) ) 
        {
            try_return(RC);
        }
        bHandleCreated = TRUE;

        if (!NT_SUCCESS(RC = ObReferenceObjectByHandle (hnd,
                                                        OBJECT_TYPE_ALL_ACCESS,
                                                        (POBJECT_TYPE) NULL,
                                                        KernelMode,
                                                        PtrReturnedObject,
                                                        (POBJECT_HANDLE_INFORMATION) NULL)) ) 
        {
            try_return(RC);
        }

        try_exit: NOTHING;

    } 
    finally 
    {
        // Make the named directory object a temporary object.
        if (bHandleCreated == TRUE) 
        {
            ZwClose(hnd);            
        }
    }

    OUT_FCT(ftd_create_dir)
    return(RC); 
}


/*************************************************************************
*
* Function: ftd_del_dir()
*
* Description:
*   Delete a directory in the object name space.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/an appropriate error
*
*************************************************************************/
static FTD_STATUS 
ftd_del_dir(PWCHAR DirectoryNameStr, PVOID PtrObject)
{
    NTSTATUS                        RC = STATUS_SUCCESS;
    UNICODE_STRING                  DirectoryName;
    OBJECT_ATTRIBUTES               DirectoryAttributes;
    HANDLE                          hnd = NULL;

    IN_FCT(ftd_del_dir)
    
    try 
    {
        ObDereferenceObject(PtrObject);

        // Create a Unicode string.
        RtlInitUnicodeString(&DirectoryName, DirectoryNameStr);

        // Create an object attributes structure.
        InitializeObjectAttributes(&DirectoryAttributes,
                                    &DirectoryName, OBJ_OPENIF,
                                    NULL, NULL);
    
        // The following call will fail if we do not have appropriate privileges.
        if (!NT_SUCCESS(RC = ZwCreateDirectoryObject(&hnd,
                                        DIRECTORY_ALL_ACCESS,
                                        &DirectoryAttributes)) ) 
        {
            try_return(RC);
        }

        ZwMakeTemporaryObject(hnd);
        ZwClose(hnd);            

        try_exit: NOTHING;

    } 
    finally 
    {
    }

    OUT_FCT(ftd_del_dir)
    return(RC); 
}


/*************************************************************************
*
* Function: ftd_init_ptrs()
*
* Description:
*   Initialize the IRP major function pointer array in the driver object
*   structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
static VOID 
ftd_init_ptrs(PDRIVER_OBJECT DriverObject)
{
    int i;

    IN_FCT(ftd_init_ptrs)

#if DBG
    DriverObject->DriverUnload = ftd_unload;
#endif

    // Initialize the ones we care about with unique function pointers.
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = ftd_open;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = ftd_close;
    DriverObject->MajorFunction[IRP_MJ_READ]            = ftd_do_read;
    DriverObject->MajorFunction[IRP_MJ_WRITE]           = ftd_do_write;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = ftd_ioctl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]        = ftd_shutdown_flush;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]   = ftd_shutdown_flush;

    OUT_FCT(ftd_init_ptrs)
    return;
}


/*************************************************************************
*
* Function: ftd_get_registry()
*
* Description:
*   read necessary values from the registry
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: NTSTATUS
*
*************************************************************************/
static NTSTATUS 
ftd_get_registry(PUNICODE_STRING RegistryPath)      // path to the registry key
{
    NTSTATUS                    RC = STATUS_SUCCESS;
    static  WCHAR               SubKeyString[] = L"\\Parameters";
    RTL_QUERY_REGISTRY_TABLE    paramTable[3];
    UNICODE_STRING              paramPath;
    ULONG                       chunk_size, defaultChunkSize = DEFAULT_CHUNK_SIZE;
    ULONG                       num_chunks, defaultNumChunks = DEFAULT_NUM_CHUNKS;
    ULONG                       maxmem, defaultMaxMem = DEFAULT_MAXMEM;

    IN_FCT(ftd_get_registry)

    try 
    {
        try 
        {
            //
            // The registry path parameter points to our key, we will append
            // the Parameters key and look for any additional configuration items
            // there.  We add room for a trailing NUL for those routines which
            // require it.

            paramPath.MaximumLength = RegistryPath->Length + sizeof(SubKeyString);
            paramPath.Buffer = ExAllocatePool(PagedPool, paramPath.MaximumLength);
            if (paramPath.Buffer == NULL) 
            {
                RC =  STATUS_INSUFFICIENT_RESOURCES; 

                try_return(RC);
            }

            RtlCopyMemory(
                paramPath.Buffer, RegistryPath->Buffer, RegistryPath->Length);

            RtlCopyMemory(
                &paramPath.Buffer[RegistryPath->Length / 2], SubKeyString,
                sizeof(SubKeyString));

            paramPath.Length = paramPath.MaximumLength;

#if DBG
            {
                ULONG                       zero = 0;

                ULONG                       debugLevel = 0;
                ULONG                       shouldBreak = 0;
                //
                // We use this to query into the registry as to whether we
                // should break at driver entry.
                //

                RtlZeroMemory(&paramTable[0], sizeof(paramTable));

                paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
                paramTable[0].Name = REGISTRY_NAME_BREAKONENTRY;
                paramTable[0].EntryContext = &shouldBreak;
                paramTable[0].DefaultType = REG_DWORD;
                paramTable[0].DefaultData = &zero;
                paramTable[0].DefaultLength = sizeof(ULONG);
    
                paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
                paramTable[1].Name = REGISTRY_NAME_DEBUGLEVEL;
                paramTable[1].EntryContext = &debugLevel;
                paramTable[1].DefaultType = REG_DWORD;
                paramTable[1].DefaultData = &zero;
                paramTable[1].DefaultLength = sizeof(ULONG);
    
                if (!NT_SUCCESS(RtlQueryRegistryValues(
                    RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                    paramPath.Buffer, &paramTable[0], NULL, NULL)))
                {
                    shouldBreak = 0;
                    debugLevel = 0;
                }
    
                FTDDebugLevel = debugLevel;

                if (shouldBreak)
                {
                    DbgPrint("FTD Breaking...\n");
                    
                    FTDBreakPoint();
/*
                    __try {
                            FTDBreakPoint();
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        // nothing
                    }
*/
                }
            }
#endif
            RtlZeroMemory(&paramTable[0], sizeof(paramTable));

            paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
            paramTable[0].Name = REGISTRY_NAME_CHUNK_SIZE;
            paramTable[0].EntryContext = &chunk_size;
            paramTable[0].DefaultType = REG_DWORD;
            paramTable[0].DefaultData = &defaultChunkSize;
            paramTable[0].DefaultLength = sizeof(ULONG);

            paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
            paramTable[1].Name = REGISTRY_NAME_NUM_CHUNKS;
            paramTable[1].EntryContext = &num_chunks;
            paramTable[1].DefaultType = REG_DWORD;
            paramTable[1].DefaultData = &defaultNumChunks;
            paramTable[1].DefaultLength = sizeof(ULONG);

            if (!NT_SUCCESS(( RC = RtlQueryRegistryValues(
                RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                paramPath.Buffer, &paramTable[0], NULL, NULL)))) 
            {
                try_return(RC);
            }

            // Do some work
            ftd_global_state->chunk_size = chunk_size;
            ftd_global_state->num_chunks = num_chunks;

            RtlZeroMemory(&paramTable[0], sizeof(paramTable));

            paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
            paramTable[0].Name = REGISTRY_NAME_MAXMEM;
            paramTable[0].EntryContext = &maxmem;
            paramTable[0].DefaultType = REG_DWORD;
            paramTable[0].DefaultData = &defaultMaxMem;
            paramTable[0].DefaultLength = sizeof(ULONG);

            if (!NT_SUCCESS(( RC = RtlQueryRegistryValues(
                RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                paramPath.Buffer, &paramTable[0], NULL, NULL)))) 
            {
                try_return(RC);
            }

            // Do some work
            ftd_global_state->maxmem = maxmem;

        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_get_registry() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }

        try_exit:   NOTHING;

    } 
    finally 
    {
        if ( paramPath.Buffer != NULL) 
        {
            ExFreePool( paramPath.Buffer );
        }
    }

    OUT_FCT(ftd_get_registry)
    return(RC);
}

int
ftd_ctl_get_device_nums(dev_t dev, int arg, int flag)
{
    int         index;
    FTD_IRQL    context;

    ftd_devnum_t *devnum = (ftd_devnum_t *)arg;

    IN_FCT(ftd_ctl_get_device_nums)

    index = ddi_get_free_soft_state(ftd_dev_state);
    if (index == -1)
    {
        OUT_FCT(ftd_ctl_get_device_nums)
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    devnum->c_major = index;
    devnum->b_major = index;

    OUT_FCT(ftd_ctl_get_device_nums)
    return FTD_SUCCESS;
}

void 
finish_biodone(PIRP Irp)
{
    IN_FCT(finish_biodone)
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    OUT_FCT(finish_biodone)
}

/*
 * Any I/Os that we don't understand, we pass down to the next layer 
 * below us.
 */
static FTD_STATUS
ftd_dev_dispatch(PDEVICE_OBJECT DeviceObject, PIRP  Irp)
{
    FTD_STATUS              RC = STATUS_SUCCESS;
    BOOLEAN                 CompleteIrp = FALSE;
    PIO_STACK_LOCATION      PtrNextIoStackLocation = NULL;
    PIO_STACK_LOCATION      PtrCurrentStackLocation = NULL;

    ftd_dev_t               *softp = NULL;

    IN_FCT(ftd_dev_dispatch)

    try 
    {
        try 
        {
            // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
            if (Irp->CurrentLocation == 1) 
            {
                // Bad!! Fudge the error code. Break if we can ...
                FTD_ERR(FTD_WRNLVL, "ftd_dev_dispatch(), Irp->CurrentLocation == 1.");

                CompleteIrp = TRUE;
                try_return(RC = STATUS_INVALID_DEVICE_REQUEST);
            }

            // Get a pointer to the device extension
            softp = (ftd_dev_t*)(DeviceObject->DeviceExtension);
            FTDAssertDevExtPtrValid(softp);

            Irp->CurrentLocation++,
            Irp->Tail.Overlay.CurrentStackLocation++;

            try_return(RC = IoCallDriver(softp->TargetDeviceObject, Irp));

        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_dev_dispatch() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }

        try_exit:   NOTHING;

    } 
    finally 
    {
        if (CompleteIrp == TRUE) 
        {
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    OUT_FCT(ftd_dev_dispatch)
    return(RC);
}

static FTD_STATUS 
ftd_ioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    FTD_STATUS          RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION  PtrCurrentStackLocation = NULL;
    ftd_node_id_t       *idp = (ftd_node_id_t *)(DeviceObject->DeviceExtension);
    BOOLEAN             CompleteIrp = FALSE;
    int                 flag = 0, cmd, err = 0;
    int                 arg;
    dev_t               dev = 0;
    FTD_CONTEXT         context;
//  PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_ioctl)

//sg
//
// Don't acquire the mutex unless it is our code
// that will be executed. We don't want to be at 
// APC level for code that is not ours to handle!
   
    try 
    {
        try 
        {
            // Get the current I/O stack location.
            PtrCurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
            ASSERT(PtrCurrentStackLocation);

            cmd = PtrCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode;
            arg = (int)Irp->AssociatedIrp.SystemBuffer;

            if (idp->NodeType == FTD_NODE_TYPE_EXT_DEVICE) 
            {
                CompleteIrp = TRUE;
                //sg
                // Was try_return (RC = ftd_ctl_ioctl(dev, cmd, arg, flag));
                // but had to undo that macro 
                // to be able to release the mutex 
                // before leaving this place
                //
                RC = ftd_ctl_ioctl(dev, cmd, arg, flag); 
                goto try_exit;
            }
            else if (idp->NodeType == FTD_NODE_TYPE_LG_DEVICE) 
            {
                // Get a pointer to the lg extension
                ftd_lg_t *lgp = (ftd_lg_t*)(DeviceObject->DeviceExtension);
                FTDAssertLGExtPtrValid(lgp);                
                CompleteIrp = TRUE;
                //sg
                // Was try_return (RC = ftd_lg_ioctl(lgp->dev, cmd, arg, flag));
                // but had to undo that macro 
                // to be able to release the mutex 
                // before leaving this place
                //
                RC = ftd_lg_ioctl(lgp->dev, cmd, arg, flag);
                goto try_exit;

            } 
            else 
            {
                //sg
                // Why hold a mutex if we send it to the underlying object?
                //
                // This is especially since we go to APC level because of 
                // the mutex!
                //
                try_return (RC = ftd_dev_dispatch(DeviceObject, Irp));

            }
        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_ioctl() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }

        try_exit:   NOTHING;

    } 
    finally 
    {
        if (CompleteIrp == TRUE) 
        {
            if (RC == FTD_SUCCESS) 
            {
                Irp->IoStatus.Information = PtrCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength;
            } 
            else 
            {
                Irp->IoStatus.Information = 0;
            }

            Irp->IoStatus.Status = RC;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    OUT_FCT(ftd_ioctl)
    return(RC);
}

FTD_STATUS
ftd_nt_lg_close(ftd_lg_t *lgp)
{
    NTSTATUS    RC = STATUS_SUCCESS;
    
    IN_FCT(ftd_nt_lg_close)

    if (lgp->flags & FTD_LG_EVENT_CREATED) 
    {
        ZwClose( lgp->hEvent );
            
        FTDClearFlag(lgp->flags, FTD_LG_EVENT_CREATED);
    }
            
    OUT_FCT(ftd_nt_lg_close)
    return RC;
}

FTD_STATUS
ftd_nt_lg_open(ftd_lg_t *lgp, int minor)
{
    NTSTATUS            RC = STATUS_SUCCESS;
    UNICODE_STRING      LGNumberString;
    UNICODE_STRING      EventName;
    BOOLEAN             bOpened = FALSE;

    IN_FCT(ftd_nt_lg_open)

    try 
    {
        try 
        {
            //
            // initialize the unicode string for the device number
            //

            RtlInitUnicodeString(&LGNumberString, NULL);
            LGNumberString.MaximumLength = sizeof(WCHAR)*4;
            LGNumberString.Buffer = ExAllocatePool(PagedPool,sizeof(WCHAR)*3 + sizeof(UNICODE_NULL));
            if (!LGNumberString.Buffer) 
            {
                FTD_ERR(FTD_WRNLVL, "Unable to allocate memory for the logical group's number string.");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES); 
            }
            RtlIntegerToUnicodeString(minor, 10, &LGNumberString);

            RtlInitUnicodeString(&EventName, NULL);
            EventName.MaximumLength = sizeof(L"\\BaseNamedObjects") + sizeof(L"\\") + sizeof(QNM) + sizeof(FTD_LG_NAME) + LGNumberString.Length + sizeof(FTD_LG_EVENT_NAME);
            EventName.Buffer = ExAllocatePool(PagedPool,EventName.MaximumLength + sizeof(UNICODE_NULL));
            if (!EventName.Buffer) 
            {
                FTD_ERR(FTD_WRNLVL, "Unable to allocate memory for the logical group's event name.");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES); 
            }
            RtlZeroMemory(EventName.Buffer, EventName.MaximumLength + sizeof(UNICODE_NULL));
            RtlAppendUnicodeToString(&EventName, L"\\BaseNamedObjects");
            RtlAppendUnicodeToString(&EventName, L"\\");
            RtlAppendUnicodeToString(&EventName, WQNM);
            RtlAppendUnicodeToString(&EventName, FTD_LG_NAME);
            RtlAppendUnicodeStringToString(&EventName, &LGNumberString);
            RtlAppendUnicodeToString(&EventName, FTD_LG_EVENT_NAME);

            lgp->Event = IoCreateSynchronizationEvent(&EventName, &lgp->hEvent) ;
            if (lgp->Event == NULL) 
            {
                FTD_ERR(FTD_WRNLVL, "Unable to create the logical group's event.");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES); 
            }
            FTDSetFlag(lgp->flags, FTD_LG_EVENT_CREATED);

        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_nt_lg_open() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }

        try_exit:   NOTHING;
    } 
    finally 
    {
        // Start unwinding if we were unsuccessful.
        if (!NT_SUCCESS(RC)) 
        {
            ftd_nt_lg_close(lgp);
        }
        
        if (EventName.Buffer != NULL) 
        {
            ExFreePool( EventName.Buffer );
        }

        if (LGNumberString.Buffer != NULL) 
        {
            ExFreePool( LGNumberString.Buffer );
        }
    }

    OUT_FCT(ftd_nt_lg_open)
    return(RC);
}

static FTD_STATUS
ftd_open(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    FTD_STATUS          RC = STATUS_SUCCESS;
    BOOLEAN             CompleteIrp = FALSE;
    FTD_CONTEXT         context;
//    PRE_ACQUIRE_MUTEX

    ftd_node_id_t       *idp = (ftd_node_id_t *)(DeviceObject->DeviceExtension);

    IN_FCT(ftd_open)

    try 
    {
        try 
        {
            if (idp->NodeType == FTD_NODE_TYPE_EXT_DEVICE) 
            {
                CompleteIrp = TRUE;

                try_return (RC = ftd_ctl_open(0, 0));
            } 
            else if (idp->NodeType == FTD_NODE_TYPE_LG_DEVICE) 
            {
                // Get a pointer to the device extension
                ftd_lg_t *lgp = (ftd_lg_t*)(DeviceObject->DeviceExtension);
                FTDAssertLGExtPtrValid(lgp);

                CompleteIrp = TRUE;

                try_return (RC = ftd_lg_open(lgp->dev, 0));
            } 
            else 
            {
                // Get a pointer to the device extension
                ftd_dev_t *softp = (ftd_dev_t*)(DeviceObject->DeviceExtension);
                FTDAssertDevExtPtrValid(softp);

                if (!NT_SUCCESS(RC = ftd_dev_open(softp->bdev, 0)) ) 
                {
                    CompleteIrp = TRUE;

                    try_return(RC);
                }
                
                try_return (RC = ftd_dev_dispatch(DeviceObject, Irp));
            }
        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_open() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }

        try_exit:   NOTHING;

    } 
    finally 
    {
        if (CompleteIrp == TRUE) 
        {
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    OUT_FCT(ftd_open)
    return(RC);
}

static FTD_STATUS
ftd_close(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{   
    FTD_STATUS      RC = STATUS_SUCCESS;
    BOOLEAN         CompleteIrp = FALSE;
    FTD_CONTEXT     context;
//    PRE_ACQUIRE_MUTEX

    ftd_node_id_t   *idp = (ftd_node_id_t *)(DeviceObject->DeviceExtension);

    IN_FCT(ftd_close)

    try 
    {
        try 
        {
            if (idp->NodeType == FTD_NODE_TYPE_EXT_DEVICE) 
            {
                CompleteIrp = TRUE;

                try_return (RC = ftd_ctl_close(0, 0));
            } 
            else if (idp->NodeType == FTD_NODE_TYPE_LG_DEVICE) 
            {
                // Get a pointer to the device extension
                ftd_lg_t *lgp = (ftd_lg_t*)(DeviceObject->DeviceExtension);
                FTDAssertLGExtPtrValid(lgp);

                CompleteIrp = TRUE;

                try_return (RC = ftd_lg_close(lgp->dev, 0));
            } 
            else 
            {
                // Get a pointer to the device extension
                ftd_dev_t *softp = (ftd_dev_t*)(DeviceObject->DeviceExtension);
                FTDAssertDevExtPtrValid(softp);

                if (!NT_SUCCESS(RC = ftd_dev_close(softp->bdev, 0)) ) 
                {
                    CompleteIrp = TRUE;

                    try_return(RC);
                }
                
                try_return (RC = ftd_dev_dispatch(DeviceObject, Irp));
            }
        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_close() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }

        try_exit:   NOTHING;

    } 
    finally 
    {
        if (CompleteIrp == TRUE) 
        {
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    OUT_FCT(ftd_close)
    return(RC);
}

/*
 * ftd_iodone_generic
 *
 * We have the driver below us call this routine when we're doing
 * simple things.  For all reads we go through here, as well as writes
 * that we're not journalling for some reason.  This routine is
 * supposed to be fairly simple.  We get the pointer to the user's bp,
 * modify some stats, adjust the resid(ual) for the buf and then call
 * ftp_finish_io to wrap things up.
 */
FTD_STATUS
ftd_iodone_generic(
    IN PDEVICE_OBJECT       PtrDeviceObject,
    IN PIRP                 Irp,
    IN void                 *Context)
{
    NTSTATUS                RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION      PtrCurrentStackLocation = NULL;
    PDEVICE_OBJECT          PtrAssociatedDeviceObject = NULL;
    FTD_IRQL                oldIrql;

    ftd_dev_t               *softp;

    PREACQUIRE_LOCK

    IN_FCT(ftd_iodone_generic)

    try 
    {

        // Get the current I/O stack location.
        PtrCurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(PtrCurrentStackLocation);

        // Get a pointer to the device extension
        softp = (ftd_dev_t*)(Context);
        FTDAssertDevExtPtrValid(softp);

        if (Irp->PendingReturned) 
        {
            IoMarkIrpPending(Irp);
        }

        PtrAssociatedDeviceObject = softp->DeviceObject;

        // Ensure that this is a valid device object pointer, else return
        // immediately.
        if (PtrAssociatedDeviceObject != PtrDeviceObject) 
        {
            // Bug exposed
            try_return(RC = STATUS_SUCCESS);
        }
        
        ACQUIRE_LOCK(softp->lock, oldIrql);
        if (PtrCurrentStackLocation->MajorFunction == IRP_MJ_READ) 
        {
            softp->readiocnt++;
            softp->sectorsread += (Irp->IoStatus.Information >> DEV_BSHIFT);
        } 
        else 
        {
            softp->writeiocnt++;
            softp->sectorswritten += (Irp->IoStatus.Information >> DEV_BSHIFT);
        }
        RELEASE_LOCK(softp->lock, oldIrql);

        try_exit:   NOTHING;

    } 
    finally 
    {
    }

    OUT_FCT(ftd_iodone_generic)
    return (RC);
}

void ftd_wakeup(ftd_lg_t *lgp)
{
    IN_FCT(ftd_wakeup)

    if (lgp->flags & FTD_LG_EVENT_CREATED) 
    {
#if DBG
        FTD_CONTEXT currentIrql = KeGetCurrentIrql();

        ASSERT(currentIrql <= DISPATCH_LEVEL);
#endif

        KeSetEvent(lgp->Event, IO_NO_INCREMENT, FALSE);
    }

    OUT_FCT(ftd_wakeup)
}

/*
 * ftd_iodone_journal
 *
 * For all journalled writes, we come through this routine when they
 * finish.  Once they finish, we commit the data in the bab so the pmd
 * can see it.  If we're not in sync mode, then we'll cause the users'
 * bp to be acknowledged to the upper layers when we finally call
 * ftd_finish_io.
 */
NTSTATUS
ftd_iodone_journal(
    IN PDEVICE_OBJECT       PtrDeviceObject,
    IN PIRP                 Irp,
    IN void                 *Context)
{
    NTSTATUS    RC = STATUS_SUCCESS;
    PDEVICE_OBJECT          PtrAssociatedDeviceObject = NULL;
    FTD_IRQL    oldIrql;

    ftd_lg_io_t *iop;
    wlheader_t  *hp;
    ftd_ctl_t   *ctlp;
    ftd_dev_t   *softp;

    PREACQUIRE_LOCK

    IN_FCT(ftd_iodone_journal)

    try 
    {
        ctlp = ftd_global_state;
        ASSERT(ctlp);
    
        softp = (ftd_dev_t*)(PtrDeviceObject->DeviceExtension);
        FTDAssertDevExtPtrValid(softp);

        if (Irp->PendingReturned) 
        {
            IoMarkIrpPending(Irp);
        }

        PtrAssociatedDeviceObject = softp->DeviceObject;

        // Ensure that this is a valid device object pointer, else return
        // immediately.
        if (PtrAssociatedDeviceObject != PtrDeviceObject) 
        {
            // Bug exposed
            try_return(RC = STATUS_SUCCESS);
        }
        
        // Get a pointer to the hp
        hp = (wlheader_t*)(Context);

        iop = ExAllocateFromNPagedLookasideList(&ctlp->LGIoLookasideListHead);
        ASSERT(iop);

        iop->Irp = Irp;
        iop->softp = softp;
        iop->hp = hp;

        ACQUIRE_LOCK(softp->lgp->lock, oldIrql);
        InsertTailList(&softp->lgp->CommitQueueListHead, &iop->ListEntry);
        RELEASE_LOCK(softp->lgp->lock, oldIrql);

        KeReleaseSemaphore(&softp->lgp->CommitQueueSemaphore, 0, 1, FALSE);

        try_return (RC = STATUS_MORE_PROCESSING_REQUIRED);

        try_exit:   NOTHING;

    } 
    finally 
    {
    }

    OUT_FCT(ftd_iodone_journal)
    return (RC);
}

/*
 * ftd_do_read
 *
 * Process a read to the device.  Setup things so that we can call our
 * bdev_strategy with the buf and have it do the right thing.
 */
static FTD_STATUS
ftd_do_read(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    FTD_STATUS              RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION      PtrNextIoStackLocation = NULL;
    PIO_STACK_LOCATION      PtrCurrentStackLocation = NULL;
    BOOLEAN                 AcquiredExtension = FALSE;
    BOOLEAN                 CompleteIrp = FALSE;
    PDEVICE_OBJECT          PtrTargetDeviceObject = NULL;
    ULONG                   ReturnedInformation = 0;
    FTD_CONTEXT             context;

    ftd_dev_t               *softp;

    IN_FCT(ftd_do_read)

    try 
    {

        // Get the current I/O stack location.
        PtrCurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(PtrCurrentStackLocation);

        // Get a pointer to the device extension
        softp = (ftd_dev_t*)(DeviceObject->DeviceExtension);
        FTDAssertDevExtPtrValid(softp);

        //  If the device extension is for a lower device forward the request.
        if ((softp->NodeIdentifier.NodeType == FTD_NODE_TYPE_ATTACHED_DEVICE)
            && (softp->flags & FTD_DEV_ATTACHED)) 
        {

            // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
            if (Irp->CurrentLocation == 1) 
            {
                // Bad!! Fudge the error code. Break if we can ...
                FTD_ERR(FTD_WRNLVL, "ftd_do_read(), Irp->CurrentLocation == 1.");

                CompleteIrp = TRUE;
                try_return(RC = STATUS_INVALID_DEVICE_REQUEST);
            }

            PtrNextIoStackLocation = IoGetNextIrpStackLocation(Irp);

            // So far, so good! Copy over the contents of the current stack
            // location into the next IRP stack location. Be careful about
            // how we do the copy. The following statement is convenient
            // but will end up screwing up any driver above us who has
            // set a completion routine!
            *PtrNextIoStackLocation = *PtrCurrentStackLocation;

            // We will specify a default completion routine. This will
            // prevent any completion routine being invoked twice
            // (set by a driver above us in the calling hierarchy) and also
            // allow us the opportunity to do whatever we like once the
            // function processing has been completed.
            // We will specify that our completion routine be invoked regardless
            // of how the IRP is completed/cancelled.
            IoSetCompletionRoutine(Irp, ftd_iodone_generic, softp, TRUE, TRUE, TRUE);

            // Forward the request. Note that if the target does not
            // wish to service the function, the request will get redirected
            // to IopInvalidDeviceRequest() (a routine that completes the
            // IRP with STATUS_INVALID_DEVICE_REQUEST).
            PtrTargetDeviceObject = softp->TargetDeviceObject;

            RC = IoCallDriver(PtrTargetDeviceObject, Irp);

            try_return(RC);
        }

        // The filter driver does not particularly care to respond to these requests.
        CompleteIrp = TRUE;
        try_return(RC = STATUS_INVALID_DEVICE_REQUEST);

        try_exit:   NOTHING;

    } 
    finally 
    {
        // Complete the IRP only if we must.
        if (CompleteIrp) 
        {
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = ReturnedInformation;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    OUT_FCT(ftd_do_read)
    return(RC);
}

/*
 * ONLY CALLED FOR WRITE!
 *
 * copy the contents of the buffer to our memory log and/or manage the
 * high resolution and low resolution dirty bits.
 */
FTD_STATUS
ftd_do_write(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    FTD_STATUS              RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION      PtrNextIoStackLocation = NULL;
    PIO_STACK_LOCATION      PtrCurrentStackLocation = NULL;
    BOOLEAN                 AcquiredExtension = FALSE;
    BOOLEAN                 CompleteIrp = FALSE;
    BOOLEAN                 PassIrpOn = TRUE;
    PDEVICE_OBJECT          PtrTargetDeviceObject = NULL;
    ULONG                   ReturnedInformation = 0;
    PUCHAR                  SourceAddress = NULL;
    FTD_CONTEXT             context;
    FTD_IRQL                oldIrql;

    ftd_dev_t               *softp = NULL;
    ftd_lg_t                *lgp = NULL;
    wlheader_t              *hp;
    ftd_uint64_t            *buf;
    unsigned int            size64;
    BOOLEAN                 bUnMapPages;
    FTD_CONTEXT             mutexcontext;
    PRE_ACQUIRE_MUTEX

    PREACQUIRE_LOCK
    IN_FCT(ftd_do_write)

    try 
    {

        // Get the current I/O stack location.
        PtrCurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(PtrCurrentStackLocation);

        // Get a pointer to the device extension
        softp = (ftd_dev_t*)(DeviceObject->DeviceExtension);
        FTDAssertDevExtPtrValid(softp);

        // Get a pointer to the LG extension
        lgp = softp->lgp;
        FTDAssertLGExtPtrValid(lgp);

        //  If the device extension is for a lower device forward the request.
        if ((softp->NodeIdentifier.NodeType == FTD_NODE_TYPE_ATTACHED_DEVICE)
            && (softp->flags & FTD_DEV_ATTACHED)) 
        {

            // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
            if (Irp->CurrentLocation == 1) 
            {
                // Bad!! Fudge the error code. Break if we can ...
                FTD_ERR(FTD_WRNLVL, "ftd_do_write(), Irp->CurrentLocation == 1.");

                CompleteIrp = TRUE;
                try_return(RC = STATUS_INVALID_DEVICE_REQUEST);
            }
            PtrNextIoStackLocation = IoGetNextIrpStackLocation(Irp);

            // So far, so good! Copy over the contents of the current stack
            // location into the next IRP stack location. Be careful about
            // how we do the copy. The following statement is convenient
            // but will end up screwing up any driver above us who has
            // set a completion routine!
            *PtrNextIoStackLocation = *PtrCurrentStackLocation;

            // We will specify a default completion routine.
            // We will specify that our completion routine be invoked regardless
            // of how the IRP is completed/cancelled.
            IoSetCompletionRoutine(Irp, ftd_iodone_generic, softp, TRUE, TRUE, TRUE);
    
            /* bab_mgr always sees the world as 64-bit words! */
            /* XXX This code should use the FTD_WLH_QUADS() macro XXX */
            size64 = sizeof_64bit(wlheader_t) + (PtrCurrentStackLocation->Parameters.Write.Length / 8);
            ACQUIRE_MUTEX(lgp->mutex, context);

            /*
             * While journalling, we copy the data to the journal.  We do this so
             * that later we can feed it to the pmd.
             *
             * XXX For HPUX we may need to do a similar copying of data from the
             * XXX buf in the proper context so we can successfully do the I/O
             * XXX to disk.
             */
            if ((lgp->state & FTD_M_JNLUPDATE) &&
                ((buf = ftd_bab_alloc_memory(lgp->mgr, size64)) != NULL)) 
            {
                if (lgp->mgr->flags & FTD_BAB_PHYSICAL) 
                {
                    hp = ftd_bab_map_memory(buf, size64 * 8);
                } 
                else 
                {
                    hp = (wlheader_t *)buf;
                }

                hp->complete = 0;
                hp->bp = NULL;

                /* add a header and some data to the memory buffer */
                hp->majicnum = DATASTAR_MAJIC_NUM;
                hp->offset = (u_int)(PtrCurrentStackLocation->Parameters.Write.ByteOffset.QuadPart >> DEV_BSHIFT);
                hp->length = PtrCurrentStackLocation->Parameters.Write.Length >> DEV_BSHIFT;
                hp->diskdev = softp->bdev;
                hp->dev = softp->bdev;

                hp->group_ptr = lgp;
                hp->flags = 0;

                /* now copy the data */
                //
                // Get a system-space pointer to the user's buffer.  A system
                // address must be used because we may already have left the
                // original caller's address space.
                //

                ASSERT(Irp->MdlAddress != NULL);

                bUnMapPages = ((Irp->MdlAddress->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | 
                    MDL_SOURCE_IS_NONPAGED_POOL)) ?  FALSE : TRUE);

#ifdef NTFOUR
                SourceAddress = MmGetSystemAddressForMdl( Irp->MdlAddress );
#else           // MUST use this fct for WIN2K, otherwise BSOD exposure
                SourceAddress = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority );
#endif

                    //***
                    //*** POTENTIAL PROBLEM!
                    //***
                    //*** If source Addres was not obtained, we don't copy the memory...
                    //*** so that means we just transfered 0's!!! CORRUPTION!!!!!!!!!!!!
                    //***

                if (SourceAddress)
                {

                    RtlCopyMemory(
                        (caddr_t)(((ftd_uint64_t*)hp) + sizeof_64bit(wlheader_t)),
                        SourceAddress, PtrCurrentStackLocation->Parameters.Write.Length);

                    if (bUnMapPages)
                        MmUnmapLockedPages(SourceAddress, Irp->MdlAddress);
                }

                if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                    ftd_bab_unmap_memory(hp, size64 * 8);
                
                RELEASE_MUTEX(lgp->mutex, context);
                /*
                 * We use the iodone function that we set before to figure out
                 * what to do now.
                 */
                IoSetCompletionRoutine(Irp, ftd_iodone_journal, buf, TRUE, TRUE, TRUE);
            } 
            else if (lgp->state == FTD_MODE_NORMAL) 
            {
                /* we just overflowed the journal */
                FTD_ERR (FTD_WRNLVL, "LG %d transitioning to Tracking mode.", lgp->dev & ~FTD_LGFLAG);
                
                ftd_clear_hrdb(lgp);
    
                lgp->state = FTD_MODE_TRACKING;

                RELEASE_MUTEX(lgp->mutex, context);

                ftd_wakeup(lgp);
            } 
            else if (lgp->state == FTD_MODE_REFRESH) 
            {
                /* 
                * we just overflowed the journal again.   Since we've been keeping
                * track of the dirty bits all along, there is little we need to
                * do here except flush the bab.  We can't recompute the dirty bits
                * here because entries may have been migrated out of the bab
                * in the interrum and we'd have no way of knowing.
                */

                /* XXX WARNING XXX Don't we want to clear the bab here because we
                * know it is dirty?  Yes, in theory we do, but we don't want to
                * step on the PMD's world view, so we let it do it when it thinks
                * we've gone down this code path 
                */
                FTD_ERR (FTD_WRNLVL, "LG %d transitioning to Tracking mode.", lgp->dev & ~FTD_LGFLAG);

                //
                // Added to clear bab? svg 28/05/03
                //
                ftd_clear_hrdb(lgp);
                //
    
                lgp->state = FTD_MODE_TRACKING;

                RELEASE_MUTEX(lgp->mutex, context);

                //
                // Added to clear bab? svg 28/05/03
                //
                ftd_wakeup(lgp);
                //
                /* we're just accumulating dirty bits right now */
            } 
            else 
            {
            /* XXX How the heck do we get here? */
                RELEASE_MUTEX(lgp->mutex, context);
            }

            /*
             * Update the hrdb in overflow and recover modes
             */
            if (lgp->state & FTD_M_BITUPDATE)
                ftd_update_hrdb(softp, Irp);

            /* Always see if the low resolution dirty bits have changed and flush
              them when they have */

            ACQUIRE_LOCK(softp->lock, oldIrql);
            if (ftd_check_lrdb(softp, Irp))
            {
                RELEASE_LOCK(softp->lock, oldIrql);
                // we need to update the lrdb....
                if (!NT_SUCCESS(RC = ftd_flush_lrdb(softp, Irp)) )
                {
                    FTD_ERR(FTD_WRNLVL, "Unable to flush the lrdb to PStore.");
                }
                else
                    PassIrpOn = FALSE;
            } 
            else 
            {
                RELEASE_LOCK(softp->lock, oldIrql);
            }

            if (PassIrpOn)
                RC = IoCallDriver(softp->TargetDeviceObject, Irp);
            else
                RC = STATUS_PENDING;
           
           try_return(RC);
        }

        // The driver does not particularly care to respond to these requests.
        CompleteIrp = TRUE;
        try_return(RC = STATUS_INVALID_DEVICE_REQUEST);

        try_exit:   NOTHING;

    } 
    finally 
    {

        // Complete the IRP only if we must.
        if (CompleteIrp) 
        {
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = ReturnedInformation;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    OUT_FCT(ftd_do_write)
    return(RC);
}

FTD_STATUS
ftd_flush_lrdb(ftd_dev_t *softp, PIRP Irp)
{
    FTD_STATUS  RC = STATUS_SUCCESS;
    FTD_IRQL    oldIrql;
    ftd_lg_io_t *iop;

    ftd_ctl_t   *ctlp = ftd_global_state;
    
    PREACQUIRE_LOCK

    IN_FCT(ftd_flush_lrdb)

    ASSERT(ctlp);

    try 
    {
        iop = ExAllocateFromNPagedLookasideList(&ctlp->LGIoLookasideListHead);
        ASSERT(iop);
        if (iop == NULL)
            try_return(RC = STATUS_UNSUCCESSFUL);

        iop->Irp = Irp;

        if (Irp != NULL) 
        {
            IoMarkIrpPending(Irp);
        }

        ACQUIRE_LOCK(softp->lock, oldIrql);
        InsertTailList(&softp->PStoreQueueListHead, &iop->ListEntry);
        RELEASE_LOCK(softp->lock, oldIrql);
        KeReleaseSemaphore(&softp->PStoreQueueSemaphore, 0, 1, FALSE);

        try_exit:   NOTHING;
    } 
    finally 
    {
    }

    OUT_FCT(ftd_flush_lrdb)
    return RC;
}

FTD_STATUS
ftd_nt_del_lg(ftd_lg_t *lgp)
{
    NTSTATUS RC = STATUS_SUCCESS;

    IN_FCT(ftd_nt_del_lg)

    try 
    {
        try 
        {
            if (lgp->flags & FTD_LG_SYMLINK_CREATED) 
            {
                IoDeleteSymbolicLink(&lgp->UserVisibleName);
            }

            if (lgp->UserVisibleName.Buffer != NULL) 
            {
                ExFreePool( lgp->UserVisibleName.Buffer );
            }

            lgp->ThreadShouldStop = TRUE;

            KeReleaseSemaphore(&lgp->CommitQueueSemaphore, 0, 1, FALSE);
            KeWaitForSingleObject(
                (PVOID) lgp->CommitThreadObjectPointer,
                Executive,
                KernelMode,
                FALSE,
                NULL );
            ObDereferenceObject(lgp->CommitThreadObjectPointer);

            KeReleaseSemaphore(&lgp->SyncThreadSemaphore, 0, 1, FALSE);
            KeWaitForSingleObject(
                (PVOID) lgp->SyncThreadObjectPointer,
                Executive,
                KernelMode,
                FALSE,
                NULL );
            ObDereferenceObject(lgp->SyncThreadObjectPointer);

            // Now, delete any device objects, etc. we may have created
            if (lgp->DeviceObject) 
            {
                // nothing (no-one) should be attached to our device
                // object at this time.
                IoDeleteDevice(lgp->DeviceObject);
            }
        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_nt_lg_del() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }
    } 
    finally 
    {
    }

    OUT_FCT(ftd_nt_del_lg)
    return (RC);
}

NTSTATUS
ftd_nt_start_lg_thread(ftd_lg_t *lgp)
{
    NTSTATUS    RC = STATUS_SUCCESS;
    HANDLE      threadHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    IN_FCT(ftd_nt_start_lg_thread)

    try 
    {
        lgp->ThreadShouldStop = FALSE;
       
        KeInitializeSemaphore(&lgp->CommitQueueSemaphore, 0, MAXLONG);
        InitializeListHead( &lgp->CommitQueueListHead );
#ifdef NTFOUR
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
#else
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
#endif
        RC = PsCreateSystemThread(&threadHandle,
                                (ACCESS_MASK) 0L,
                                &ObjectAttributes,
                                (HANDLE) 0L,
                                NULL,
                                ftd_commit_thread,
                                lgp);

        if (!NT_SUCCESS(RC)) 
        {
            try_return(RC);
        }

        lgp->CommitThreadObjectPointer = NULL;
        RC = ObReferenceObjectByHandle(
                           threadHandle,
                           THREAD_ALL_ACCESS,
                           NULL,
                           KernelMode,
                           &lgp->CommitThreadObjectPointer,
                           NULL
                           );

        if (!NT_SUCCESS(RC)) 
        {
            try_return(RC);
        }

        ZwClose(threadHandle);

        KeInitializeSemaphore(&lgp->SyncThreadSemaphore, 0, MAXLONG);

#ifdef NTFOUR
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
#else
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
#endif

        RC = PsCreateSystemThread(&threadHandle,
                                (ACCESS_MASK) 0L,
                                &ObjectAttributes,
                                (HANDLE) 0L,
                                NULL,
                                ftd_sync_thread,
                                lgp);

        if (!NT_SUCCESS(RC)) 
        {
            try_return(RC);
        }

        lgp->SyncThreadObjectPointer = NULL;
        RC = ObReferenceObjectByHandle(
                           threadHandle,
                           THREAD_ALL_ACCESS,
                           NULL,
                           KernelMode,
                           &lgp->SyncThreadObjectPointer,
                           NULL
                           );

        if (!NT_SUCCESS(RC)) 
        {
            try_return(RC);
        }

        ZwClose(threadHandle);

        try_exit:   NOTHING;

    } 
    finally 
    {
        if (!NT_SUCCESS(RC)) 
        {
            lgp->ThreadShouldStop = TRUE;

            if (lgp->CommitThreadObjectPointer) 
            {
                KeReleaseSemaphore(&lgp->CommitQueueSemaphore, 0, 1, FALSE);
                KeWaitForSingleObject(
                    (PVOID) lgp->CommitThreadObjectPointer,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL );
                ObDereferenceObject(lgp->CommitThreadObjectPointer);
            }

            if (lgp->SyncThreadObjectPointer) 
            {
                KeReleaseSemaphore(&lgp->SyncThreadSemaphore, 0, 1, FALSE);
                KeWaitForSingleObject(
                    (PVOID) lgp->SyncThreadObjectPointer,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL );
                ObDereferenceObject(lgp->SyncThreadObjectPointer);
            }
        } 
        else 
        {
        }
    }

    OUT_FCT(ftd_nt_start_lg_thread)
    return(RC);
}

FTD_STATUS
ftd_nt_add_lg(PDRIVER_OBJECT DriverObject, int lgnum, char *vdevname, ftd_lg_t **lgp)
{
    NTSTATUS                        RC = STATUS_SUCCESS;
    PVOID                           DirectoryObject;
    UNICODE_STRING                  DriverDeviceName;
    UNICODE_STRING                  NumberString;
    PDEVICE_OBJECT                  DeviceObject;
    BOOLEAN                         AcquiredExtension = FALSE;
    UCHAR                           ntDeviceName[64];
    PDEVICE_OBJECT                  diskDeviceObject;

    ftd_ctl_t                       *ctlp;
    ftd_lg_t                        *extension = NULL;

    IN_FCT(ftd_nt_add_lg)

    try 
    {
        try 
        {
            ctlp = ftd_global_state;
            FTDAssertCtlPtrValid(ctlp);

            //
            // initialize the temp unicode strings
            //
            RtlInitUnicodeString(&NumberString, NULL);
            RtlInitUnicodeString(&DriverDeviceName, NULL);

            //
            // populate the unicode string for the device number
            //
            NumberString.MaximumLength = sizeof(WCHAR)*4;
            NumberString.Buffer = ExAllocatePool(PagedPool,NumberString.MaximumLength + sizeof(UNICODE_NULL));
            if (!NumberString.Buffer) 
            {
                FTD_ERR(FTD_WRNLVL, "Unable to allocate memory for the logical group's number string.");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES); 
            }
            RtlIntegerToUnicodeString(lgnum, 10, &NumberString);

            // create a device object representing the driver itself
            //  so that requests can be targeted to the driver ...
            RtlInitUnicodeString(&DriverDeviceName, NULL);
            DriverDeviceName.MaximumLength = sizeof(FTD_DRV_DIR) + sizeof(L"\\") + sizeof(FTD_LG_NAME) + NumberString.Length;
            DriverDeviceName.Buffer = ExAllocatePool(PagedPool,DriverDeviceName.MaximumLength + sizeof(UNICODE_NULL));

            if (!DriverDeviceName.Buffer) 
            {
                FTD_ERR(FTD_WRNLVL, "Unable to allocate memory for the logical group's device name.");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES); 
            }
            RtlZeroMemory(DriverDeviceName.Buffer, DriverDeviceName.MaximumLength + sizeof(UNICODE_NULL));
            RtlAppendUnicodeToString(&DriverDeviceName, FTD_DRV_DIR);
            RtlAppendUnicodeToString(&DriverDeviceName, L"\\");
            RtlAppendUnicodeToString(&DriverDeviceName, FTD_LG_NAME);
            RtlAppendUnicodeStringToString(&DriverDeviceName, &NumberString);

            if (!NT_SUCCESS(RC = IoCreateDevice(
                    DriverObject,
                    sizeof(ftd_lg_t),
                    &DriverDeviceName,
                    FILE_DEVICE_UNKNOWN,    // For lack of anything better ?
                    0,
                    FALSE,  // Not exclusive.
                    &DeviceObject))) 
            {
                // failed to create a device object, leave.
                FTD_ERR(FTD_WRNLVL, "Unable to create the kernel device for this logical group, RC = %d.", RC);

                try_return(RC);
            }

            // Initialize the extension for the device object.
            extension = (ftd_lg_t*)(DeviceObject->DeviceExtension);
            ftd_init_lg_ext(extension, FTD_NODE_TYPE_LG_DEVICE);
            extension->DeviceObject = DeviceObject;
 
            // In order to allow user-space helper applications to access our
            // lg object for the ftd driver, create a symbolic link to
            // the object.
            RtlInitUnicodeString(&extension->UserVisibleName, NULL);
            extension->UserVisibleName.MaximumLength = sizeof(FTD_DOS_DRV_DIR) + sizeof(L"\\") + sizeof(FTD_LG_NAME) + NumberString.Length;
            extension->UserVisibleName.Buffer = ExAllocatePool(NonPagedPool,extension->UserVisibleName.MaximumLength + sizeof(UNICODE_NULL));
            if (!extension->UserVisibleName.Buffer) 
            {
                FTD_ERR(FTD_WRNLVL, "Unable to allocate memory for the logical group's Win32 device name.");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES); 
            }
            RtlZeroMemory(extension->UserVisibleName.Buffer, extension->UserVisibleName.MaximumLength + sizeof(UNICODE_NULL));
            RtlAppendUnicodeToString(&extension->UserVisibleName, FTD_DOS_DRV_DIR);
            RtlAppendUnicodeToString(&extension->UserVisibleName, L"\\");
            RtlAppendUnicodeToString(&extension->UserVisibleName, FTD_LG_NAME);
            RtlAppendUnicodeStringToString(&extension->UserVisibleName, &NumberString);

            if (!NT_SUCCESS(RC = IoCreateSymbolicLink(&extension->UserVisibleName, &DriverDeviceName))) {
                FTD_ERR(FTD_WRNLVL, "Unable to create the logical group's symbolic link, RC = %d.", RC);

                try_return(RC);
            }

            FTDSetFlag(extension->flags, FTD_LG_SYMLINK_CREATED);

#ifdef OLD_PSTORE
            if (!NT_SUCCESS(RC = ftd_find_device(vdevname, &diskDeviceObject, &extension->PStoreDeviceObject)) ) 
            {
                // failed to create a device object; cannot do much now.
               FTD_ERR(FTD_WRNLVL, "Unable to find the logical group's pstore's device:  %s, RC = %d.", vdevname, RC);
               try_return(RC);
            }
#else
            // Record PStore name in extension structure
            RtlInitUnicodeString(&extension->PStoreFileName, NULL);
            extension->PStoreFileName.MaximumLength = 2 * sizeof(vdevname);
            extension->PStoreFileName.Buffer = ExAllocatePool(NonPagedPool,extension->PStoreFileName.MaximumLength);
            if (!extension->PStoreFileName.Buffer) 
            {
                FTD_ERR(FTD_WRNLVL, "Unable to allocate memory for PStore extension block");

                try_return(RC = STATUS_INSUFFICIENT_RESOURCES); 
            }

            RtlZeroMemory(extension->PStoreFileName.Buffer, extension->PStoreFileName.MaximumLength);

            if (!NT_SUCCESS(RC = ftd_find_file(vdevname, extension)) ) 
            {
                // failed to create a file handle; cannot do much now.
                FTD_ERR(FTD_WRNLVL, "Unable to find the logical group's pstore's file:  %s, RC = %d.", vdevname, RC);
                try_return(RC);
            }
#endif

            // We do this whenever device objects are create on-the-fly (i.e. not as
            // part of driver initialization).
            DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_nt_add_lg() : EXCEPTION_EXECUTE_HANDLER, RC = %x.", GetExceptionCode());
        }

        try_exit:   NOTHING;
    } 
    finally 
    {
        // Start unwinding if we were unsuccessful.
        if (!NT_SUCCESS(RC)) 
        {
            if (extension)
            {
                if (extension->flags & FTD_LG_SYMLINK_CREATED) 
                {
                    IoDeleteSymbolicLink(&extension->UserVisibleName);
                }

                if (extension->UserVisibleName.Buffer != NULL) 
                {
                    ExFreePool( extension->UserVisibleName.Buffer );
                }
            
                // Now, delete any device objects, etc. we may have created
                if (extension->DeviceObject) 
                {
                    // nothing (no-one) should be attached to our device
                    // object at this time.
                    IoDeleteDevice(extension->DeviceObject);
                }
            }
        } 
        else 
        {
            *lgp = extension;
        }

        if (NumberString.Buffer != NULL) 
        {
            ExFreePool( NumberString.Buffer );
        }

        if (DriverDeviceName.Buffer != NULL) 
        {
            ExFreePool( DriverDeviceName.Buffer );
        }
    }

    OUT_FCT(ftd_nt_add_lg)
    return(RC);
}

/*************************************************************************
*
* Function: ftd_detach_device()
*
* Description:
*   This routine will detach from the target device object. It will delete
*   the locally created device object as well. Note that we must ensure that
*   no I/O requests are outstanding when this occurs (or we should drop them).
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
NTSTATUS 
ftd_nt_detach_device(ftd_dev_t *softp)
{
    NTSTATUS RC = STATUS_SUCCESS;

    IN_FCT(ftd_nt_detach_device)

    ASSERT(softp);

    try 
    {
        try 
        {
            //
            // Check if this device is mounted.
            //
            if ((!softp->DiskDeviceObject->Vpb) || (softp->DiskDeviceObject->Vpb->Flags & VPB_MOUNTED)) 
            {
                FTD_ERR(FTD_WRNLVL, "The device %s, is mounted, unable to unload.", softp->devname);
                try_return(RC = STATUS_UNSUCCESSFUL);
            }
            
            // Detach if attached.
            if (softp->flags & FTD_DEV_ATTACHED) 
            {
                IoDetachDevice(softp->TargetDeviceObject);
                FTDClearFlag(softp->flags, FTD_DEV_ATTACHED);
            }
        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_nt_detach_device() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }

        try_exit:   NOTHING;
    } 
    finally 
    {
    }

    OUT_FCT(ftd_nt_detach_device)
    return(RC);
}

FTD_STATUS
ftd_nt_del_device(ftd_dev_t *softp)
{
    NTSTATUS            RC = STATUS_SUCCESS;

    ftd_dev_t           **link;

    IN_FCT(ftd_nt_del_device)
    
    try 
    {
        try 
        {
            if (softp->flags & FTD_DEV_ATTACHED) 
            {
                if (!NT_SUCCESS(RC = ftd_nt_detach_device(softp))) 
                {
                    try_return(RC);
                }
            }

            softp->ThreadShouldStop = TRUE;
            KeReleaseSemaphore(&softp->PStoreQueueSemaphore, 0, 1, FALSE);
            KeWaitForSingleObject(
                (PVOID) softp->PStoreThreadObjectPointer,
                Executive,
                KernelMode,
                FALSE,
                NULL );
            ObDereferenceObject(softp->PStoreThreadObjectPointer);

            // Note that on 4.0 and later systems, this will result in a recursive fast detach.
            IoDeleteDevice(softp->DeviceObject);
        } 
        except (ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
            // We encountered an exception somewhere, eat it up.
            FTD_ERR(FTD_WRNLVL, "ftd_nt_del_device() : EXCEPTION_EXECUTE_HANDLER, RC = %d.", GetExceptionCode());
        }

        try_exit:   NOTHING;
    } 
    finally 
    {
    }

    OUT_FCT(ftd_nt_del_device)
    return(RC);
}

NTSTATUS
ftd_nt_start_dev_thread(ftd_dev_t *softp)
{
    NTSTATUS    RC = STATUS_SUCCESS;
    HANDLE      threadHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    IN_FCT(ftd_nt_start_dev_thread)

    try 
    {
        softp->ThreadShouldStop = FALSE;
#ifndef _ERESOURCE_SPINS_
        KeInitializeSpinLock(&softp->lock);
#endif
        KeInitializeSemaphore(&softp->PStoreQueueSemaphore, 0, MAXLONG);
        InitializeListHead( &softp->PStoreQueueListHead );

#ifdef NTFOUR
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
#else
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
#endif

        RC = PsCreateSystemThread(&threadHandle,
                                (ACCESS_MASK) 0L,
                                &ObjectAttributes,
                                (HANDLE) 0L,
                                NULL,
                                ftd_pstore_thread,
                                softp);
        if (!NT_SUCCESS(RC)) 
        {
            try_return(RC);
        }

        RC = ObReferenceObjectByHandle(
                           threadHandle,
                           THREAD_ALL_ACCESS,
                           NULL,
                           KernelMode,
                           &softp->PStoreThreadObjectPointer,
                           NULL
                           );
        if (!NT_SUCCESS(RC)) 
        {
            try_return(RC);
        }

        ZwClose(threadHandle);
        
        try_exit:   NOTHING;

    } 
    finally 
    {
        if (!NT_SUCCESS(RC)) 
        {
        } 
        else 
        {
        }
    }

    OUT_FCT(ftd_nt_start_dev_thread)
    return(RC);
}

/*************************************************************************
*
* Function: ftd_nt_add_device()
*
*    Creates and initializes a new filter device object FiDO for the
*    corresponding PDO.  Then it attaches the device object to the device
*    stack of the drivers for the device.
*
* Arguments:
*    DriverObject - Disk performance driver object.
*    PhysicalDeviceObject - Physical Device Object from the underlying layered driver
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
ftd_nt_add_device(PDRIVER_OBJECT DriverObject, char *devname, char *vdevname, ftd_dev_t **softp)
{
    NTSTATUS                RC = STATUS_SUCCESS;
    IO_STATUS_BLOCK         ioStatus;
    BOOLEAN                 InitializedDeviceObject = FALSE;
    PDEVICE_OBJECT          filterDeviceObject;
    PDEVICE_OBJECT          targetDeviceObject;
    PDEVICE_OBJECT          diskDeviceObject;

    ftd_dev_t               *extension = NULL;

    IN_FCT(ftd_nt_add_device)

    try 
    {
        if (!NT_SUCCESS(RC = ftd_find_device(vdevname, &diskDeviceObject, &targetDeviceObject)) ) 
        {
            // failed to create a device object; cannot do much now.
            FTD_ERR(FTD_WRNLVL, "Unable to find the device:  %s, RC = %d.", devname, RC);

            try_return(RC);
        }

        //
        // Check if this device is already mounted.
        //
        if ((!diskDeviceObject->Vpb) || (diskDeviceObject->Vpb->Flags & VPB_MOUNTED)) 
        {

            FTD_ERR(FTD_WRNLVL, "The device %s, is mounted!", devname);

            //
            // Assume this device has already been attached.
            //

            try_return(RC = STATUS_UNSUCCESSFUL);
        }

        //  
        // Create a filter device object for this device (partition).
        //
        if (!NT_SUCCESS(RC = IoCreateDevice(DriverObject,
                            sizeof(ftd_dev_t),
                            NULL,
                            FILE_DEVICE_DISK,
                            0,
                            FALSE,
                            &filterDeviceObject))) 
        {
            // failed to create a device object; cannot do much now.
            FTD_ERR(FTD_WRNLVL, "Unable to create the kernel device for: %s, RC = %d.", devname, RC);

            try_return(RC);
        }

        // Initialize the extension for the device object.
        extension = (ftd_dev_t*)(filterDeviceObject->DeviceExtension);
        ftd_init_dev_ext(extension, filterDeviceObject, FTD_NODE_TYPE_ATTACHED_DEVICE);
        strcpy(extension->devname, devname);
        InitializedDeviceObject = TRUE;
    
        //
        // Attaches the device object to the highest device object in the chain and
        // return the previously highest device object, which is passed to
        // IoCallDriver when pass IRPs down the device stack
        //

        extension->TargetDeviceObject =
            IoAttachDeviceToDeviceStack(filterDeviceObject, targetDeviceObject);

        if (extension->TargetDeviceObject == NULL) 
        {
            FTD_ERR(FTD_WRNLVL, "Unable to attach to the device stack for: %s.", devname);

            try_return(RC = STATUS_NO_SUCH_DEVICE);
        }

        extension->DiskDeviceObject = diskDeviceObject;
        extension->DriverObject = DriverObject;

        // Initialize the TargetDeviceObject field in the extension.
        extension->TargetDriverObject = extension->TargetDeviceObject->DriverObject;
        FTDSetFlag(extension->flags, FTD_DEV_ATTACHED);
        
        // We should set the Flags values correctly to indicate whether
        // direct-IO, buffered-IO, or neither is required. Typically, FSDs
        // (especially native FSD implementations) do not want the I/O
        // Manager to touch the user buffer at all.
        filterDeviceObject->Flags |= (extension->TargetDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO));

        //
        // Clear the DO_DEVICE_INITIALIZING flag
        //
        filterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        // We are there now. All I/O requests will start being redirected to
        // us until we detach ourselves.

        try_exit:   NOTHING;


    } 
    finally 
    {
        if (!NT_SUCCESS(RC) && filterDeviceObject) 
        {
            if (InitializedDeviceObject) 
            {
                IoDeleteDevice(filterDeviceObject);
            }
        } 
        else 
        {
            *softp = extension;
        }
    }

    OUT_FCT(ftd_nt_add_device)
    return(RC);
}

static FTD_STATUS
ftd_find_device(char *vdevname, PDEVICE_OBJECT *diskDeviceObject, PDEVICE_OBJECT *targetDeviceObject)
{
    FTD_STATUS              RC = STATUS_SUCCESS;
    IO_STATUS_BLOCK         ioStatus;
    STRING                  ntString;
    UNICODE_STRING          ntUnicodeString;
    PFILE_OBJECT            fileObject;

    ftd_dev_t               *extension = NULL;

    IN_FCT(ftd_find_device)

    try 
    {
        //
        // Create unicode NT device name.
        //
        RtlInitAnsiString(&ntString,
                          vdevname);

        RC = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                        &ntString,
                                        TRUE);

        if (!NT_SUCCESS(RC)) 
        {
            FTD_ERR(FTD_WRNLVL, "Unable to create the UNICODE device name for %s, RC = %d.", vdevname, RC);

            try_return(RC);
        }
    
        //
        // Get target device object.
        //
        RC = IoGetDeviceObjectPointer(&ntUnicodeString,
                                          FILE_READ_ATTRIBUTES,
                                          &fileObject,
                                          targetDeviceObject);

        *diskDeviceObject = fileObject->DeviceObject;

        //
        // Dereference file object as these are the rules.
        //
        ObDereferenceObject(fileObject);

        //
        // If this fails then it is because there is no such device
        // which signals completion.
        //

        if (!NT_SUCCESS(RC)) 
        {
            FTD_ERR(FTD_WRNLVL, "Unable to get the device pointer for %s, RC = %d.", vdevname, RC);
        
            RtlFreeUnicodeString(&ntUnicodeString);

            try_return(RC);
        }

        RtlFreeUnicodeString(&ntUnicodeString);

        //FTD_ERR(FTD_NTCLVL, "ftd_find_device(): %s found\n", vdevname);

        // We are there now. All I/O requests will start being redirected to
        // us until we detach ourselves.

        try_exit:   NOTHING;


    } 
    finally 
    {
    }

    OUT_FCT(ftd_find_device)
    return(RC);
}

#ifndef OLD_PSTORE
static FTD_STATUS
ftd_find_file(char *vdevname, ftd_lg_t *extension)
{
    FTD_STATUS              RC = STATUS_SUCCESS;
    IO_STATUS_BLOCK         ioStatus;
    STRING                  ntString;
    UNICODE_STRING          ntUnicodeString;
    PFILE_OBJECT            fileObject;
    OBJECT_ATTRIBUTES       objAttr;
    HANDLE                  FileHandle;

    IN_FCT(ftd_find_file)

    try 
    {
        //
        // Create unicode NT device name.
        //
        RtlInitAnsiString(&ntString,
                          vdevname);

        RC = RtlAnsiStringToUnicodeString(&extension->PStoreFileName,
                                          &ntString,
                                          TRUE);

        if (!NT_SUCCESS(RC)) 
        {
            FTD_ERR(FTD_WRNLVL, "Unable to create the UNICODE device name for %s, RC = %d.", vdevname, RC);

            try_return(RC);
        }
    

        //
        // Get target file handle for PStore, create it if not exist
        //
#ifdef NTFOUR
        InitializeObjectAttributes(&objAttr, 
                                   &extension->PStoreFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL, 
                                   NULL);
#else
        InitializeObjectAttributes(&objAttr, 
                                   &extension->PStoreFileName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, //sg
                                   NULL, 
                                   NULL);
#endif

        RC = ZwCreateFile( &FileHandle,
                            GENERIC_READ|SYNCHRONIZE|GENERIC_WRITE,   
                            &objAttr,
                            &ioStatus,
                            0, 
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OPEN, 
                            FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL, 
                            0);

        if (!NT_SUCCESS(RC)) 
        {
            FTD_ERR(FTD_WRNLVL, "Unable to get the file handle for %s, RC = %x.", vdevname, RC);
        
            RtlFreeUnicodeString(&extension->PStoreFileName);

            try_return(RC);
        }

        //
        // Close PStore file for proper operation.
        //
        RC = ZwClose(FileHandle);

        if (!NT_SUCCESS(RC)) 
            FTD_ERR(FTD_WRNLVL, "Unable to close the file handle for %s, RC = %x.", vdevname, RC);

        // FTD_ERR(FTD_NTCLVL, "ftd_find_device(): %s found\n", vdevname);

        // We are there now. All I/O requests will start being redirected to
        // us until we detach ourselves.

        try_exit:   NOTHING;


    } 
    finally 
    {
    }

    OUT_FCT(ftd_find_file)
    return(RC);
}
#endif

/*************************************************************************
*
* Function: ftd_init_dev_ext()
*
* Description:
*   Performs some rudimentary work to initialize the device extension
*   presumably for a newly created device object.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None.
*
*************************************************************************/
void 
ftd_init_dev_ext(ftd_dev_t *softp, PDEVICE_OBJECT AssociatedDeviceObject, unsigned int NodeType)
{   
    IN_FCT(ftd_init_dev_ext)

    RtlZeroMemory(softp, sizeof(ftd_dev_t));

    softp->NodeIdentifier.NodeType = NodeType;
    softp->NodeIdentifier.NodeSize = sizeof(ftd_dev_t);

    softp->DeviceObject = AssociatedDeviceObject;

    OUT_FCT(ftd_init_dev_ext)
    return;
}



/*************************************************************************
*
* Function: ftd_init_ext_ext()
*
* Description:
*   Performs some rudimentary work to initialize the device extension
*   presumably for a newly created device object.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None.
*
*************************************************************************/
void 
ftd_init_ext_ext(ftd_ext_t *extension, unsigned int NodeType)
{
    IN_FCT(ftd_init_ext_ext)

    RtlZeroMemory(extension, sizeof(ftd_ext_t));

    extension->NodeIdentifier.NodeType = NodeType;
    extension->NodeIdentifier.NodeSize = sizeof(ftd_ext_t);

    OUT_FCT(ftd_init_ext_ext)
    return;
}


/*************************************************************************
*
* Function: ftd_init_lg_ext()
*
* Description:
*   Performs some rudimentary work to initialize the LG extension
*   presumably for a newly created group object.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None.
*
*************************************************************************/
void 
ftd_init_lg_ext(ftd_lg_t *lgp, unsigned int NodeType)
{
    IN_FCT(ftd_init_lg_ext)

    RtlZeroMemory(lgp, sizeof(ftd_lg_t));

    lgp->NodeIdentifier.NodeType = NodeType;
    lgp->NodeIdentifier.NodeSize = sizeof(ftd_lg_t);

    OUT_FCT(ftd_init_lg_ext)
    return;
}

