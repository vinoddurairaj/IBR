/**************************************************************************************

Module Name:	sftk_WMI.C   
Author Name:	Parag sanghvi
Description:	All Device related API and their initialization/Deinitialization 
				Defined over here.
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#include <sftk_main.h>

#ifdef WMI_IMPLEMENTED	

NTSTATUS
SwrQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve information about
    the guids being registered.

    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose registration information is needed

    *RegFlags returns with a set of flags that describe all of the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device. These flags are ORed into the flags specified
        by the GUIDREGINFO for each guid.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call OS_FreeMemory with the buffer returned.

    *RegistryPath returns with the registry path of the driver. This is
        required

    MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned unmodified. If a value is returned then
        it is NOT freed.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in
        *RegFlags.

Return Value:

    status

--*/
{
    USHORT size;
    NTSTATUS status;
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    size = deviceExtension->DeviceInfo.PhysicalDeviceName.Length + sizeof(UNICODE_NULL);

    InstanceName->Buffer = OS_AllocMemory(PagedPool,
                                         size);
    if (InstanceName->Buffer != NULL)
    {
        *RegistryPath = &GDriverRegistryPath;

        *RegFlags = WMIREG_FLAG_INSTANCE_PDO | WMIREG_FLAG_EXPENSIVE;
        *Pdo = deviceExtension->PhysicalDeviceObject;
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);
}


NTSTATUS
SwrQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. When the driver has finished filling the
    data block it must call WmiCompleteRequest to complete the irp. The
    driver can return STATUS_PENDING if the irp cannot be completed
    immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension;
    ULONG sizeNeeded;
    KIRQL        currentIrql;
    ULONG deviceNameSize;
    PWCHAR diskNamePtr;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

#if 0	// TODO : Implement Our Structure and fill that information as per request so WMU caller 
		// can have this information.
/* ********************************************************
    if (GuidIndex == 0)
    {
		PDISK_PERFORMANCE totalCounters;
		PDISK_PERFORMANCE diskCounters;
		PWMI_DISK_PERFORMANCE Swrormance;

        deviceNameSize = deviceExtension->DeviceInfo.PhysicalDeviceName.Length +
                         sizeof(USHORT);
        sizeNeeded = ((sizeof(WMI_DISK_PERFORMANCE) + 1) & ~1) +
                                         deviceNameSize;
        diskCounters = deviceExtension->DiskCounters;
        if (diskCounters == NULL)
        {
            status = STATUS_UNSUCCESSFUL;
        }
        else if (BufferAvail >= sizeNeeded)
        {
            //
            // Update idle time if disk has been idle
            //
            ULONG i;
            LARGE_INTEGER perfctr, frequency;

            OS_ZeroMemory(Buffer, sizeof(WMI_DISK_PERFORMANCE));
            Swrormance = (PWMI_DISK_PERFORMANCE)Buffer;

            totalCounters = (PDISK_PERFORMANCE)Swrormance;
            KeQuerySystemTime(&totalCounters->QueryTime);

#ifdef USE_PERF_CTR
            perfctr = KeQueryPerformanceCounter(&frequency);
#endif
            for (i=0; i<deviceExtension->Processors; i++) {
                SwrAddCounters( totalCounters, diskCounters, frequency);
                DebugPrint((11,
                    "SwrQueryWmiDataBlock: R%d %I64u W%d%I64u ", i,
                    diskCounters->ReadTime, diskCounters->WriteTime));
                diskCounters = (PDISK_PERFORMANCE)
                               ((PCHAR)diskCounters + PROCESSOR_COUNTERS_SIZE);
            }
            DebugPrint((11, "\n"));
            totalCounters->QueueDepth = deviceExtension->QueueDepth;

            DebugPrint((9,
                "QueryWmiDataBlock: Dev %X RT %I64u WT %I64u Rds %d Wts %d freq %I64u\n",
                totalCounters,
                totalCounters->ReadTime, totalCounters->WriteTime,
                totalCounters->ReadCount, totalCounters->WriteCount,
                frequency));

            if (totalCounters->QueueDepth == 0) {
                LARGE_INTEGER difference;

                difference.QuadPart
#ifdef USE_PERF_CTR
                    = perfctr.QuadPart -
#else
                    = totalCounters->QueryTime.QuadPart -
#endif
                           deviceExtension->LastIdleClock.QuadPart;
                if (frequency.QuadPart > 0) {
                    totalCounters->IdleTime.QuadPart +=
#ifdef USE_PERF_CTR
                        10000000 * difference.QuadPart / frequency.QuadPart;
#else
                        difference.QuadPart;
#endif
                }
            }

            totalCounters->StorageDeviceNumber
                = deviceExtension->DiskNumber;
            RtlCopyMemory(
                &totalCounters->StorageManagerName[0],
                &deviceExtension->StorageManagerName[0],
                8 * sizeof(WCHAR));

            diskNamePtr = (PWCHAR)(Buffer +
                          ((sizeof(DISK_PERFORMANCE) + 1) & ~1));
            *diskNamePtr++ = deviceExtension->DeviceInfo.PhysicalDeviceName.Length;
            RtlCopyMemory(diskNamePtr,
                          deviceExtension->DeviceInfo.PhysicalDeviceName.Buffer,
                          deviceExtension->DeviceInfo.PhysicalDeviceName.Length);
            *InstanceLengthArray = sizeNeeded;

            status = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }

    } 
	else 
****************************************************** */
#endif // 0
	{
        status = STATUS_WMI_GUID_NOT_FOUND;
        sizeNeeded = 0;
    }

    status = WmiCompleteRequest(
                                 DeviceObject,
                                 Irp,
                                 status,
                                 sizeNeeded,
                                 IO_NO_INCREMENT);
    return(status);
}


NTSTATUS
SwrWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for enabling or
    disabling events and data collection.  When the driver has finished it
    must call WmiCompleteRequest to complete the irp. The driver can return
    STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose events or data collection are being
        enabled or disabled

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    Function differentiates between event and data collection operations

    Enable indicates whether to enable or disable


Return Value:

    status

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension;
    ULONG i;

    deviceExtension = DeviceObject->DeviceExtension;

#if 0	// TODO : Implement Our Structure and fill that information as per request so WMU caller 
		// can have this information.
/* ********************************************************
    if (GuidIndex == 0)
    {
        if (Function == WmiDataBlockControl) {
          if (Enable) {
             if (InterlockedIncrement(&deviceExtension->CountersEnabled) == 1) {
                //
                // Reset per processor counters to 0
                //
                if (deviceExtension->DiskCounters != NULL) {
                    OS_ZeroMemory(
                        deviceExtension->DiskCounters,
                        PROCESSOR_COUNTERS_SIZE * deviceExtension->Processors);
                }
                SwrGetClock(deviceExtension->LastIdleClock, NULL);
                DebugPrint((10,
                    "SwrWmiFunctionControl: LIC=%I64u\n",
                    deviceExtension->LastIdleClock));
                deviceExtension->QueueDepth = 0;
                DebugPrint((3, "SwrWmi: Counters enabled %d\n",
                                deviceExtension->CountersEnabled));
             }
          } else {
             if (InterlockedDecrement(&deviceExtension->CountersEnabled)
                  <= 0) {
                deviceExtension->CountersEnabled = 0;
                deviceExtension->QueueDepth = 0;
                DebugPrint((3, "SwrWmi: Counters disabled %d\n",
                                deviceExtension->CountersEnabled));
            }
          }
        }
        status = STATUS_SUCCESS;
    } 
	else 
******************************************************** */
#endif // 0
	{
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(
                                 DeviceObject,
                                 Irp,
                                 status,
                                 0,
                                 IO_NO_INCREMENT);
    return(status);
}

NTSTATUS SwrWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles any WMI requests for information. Since the disk
    information is read-only, is always collected and does not have any
    events only QueryAllData, QuerySingleInstance and GetRegInfo requests
    are supported.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION      irpSp;
    NTSTATUS status;
    PWMILIB_CONTEXT wmilibContext;
    SYSCTL_IRP_DISPOSITION disposition;
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    DebugPrint((2, "SwrWmi: DeviceObject %X Irp %X\n",
                    DeviceObject, Irp));
    wmilibContext = &deviceExtension->WmilibContext;
    if (wmilibContext->GuidCount == 0)  // wmilibContext is not valid
    {
        DebugPrint((3, "SwrWmi: WmilibContext invalid"));
        return SwrSendToNextDriver(DeviceObject, Irp);
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    DebugPrint((3, "SwrWmi: Calling WmiSystemControl\n"));
    status = WmiSystemControl(	wmilibContext,
                                DeviceObject,
                                Irp,
                                &disposition);
    switch (disposition)
    {
        case IrpProcessed:
        {
            break;
        }

        case IrpNotCompleted:
        {
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }

//      case IrpForward:
//      case IrpNotWmi:
        default:
        {
            status = SwrSendToNextDriver(DeviceObject, Irp);
            break;
        }
    }
    return(status);
}

#endif // #ifdef WMI_IMPLEMENTED	

#ifdef OLD_EVENT_LOG_SUPPORT

#include "ftd_klog.h"
#include "ftd_nt.h"

#if defined(FTD_PRIVATE)
#undef FTD_PRIVATE
#endif /* defined(FTD_PRIVATE) */

#if defined(FTDDEBUG)
#define FTD_PRIVATE
#else  /* defined(FTDDEBUG) */
#define FTD_PRIVATE static
#endif /* defined(FTDDEBUG) */

/* ftd_klog.c */
FTD_PRIVATE int _sprintf(char *buf, const char *cfmt, ...);
FTD_PRIVATE char *ksprintn(u_long ul, int base, int *lenp);
FTD_PRIVATE int kvprintf(char const *fmt, void (*func)(int, void *), void *arg, int radix, va_list ap);

static struct error_log_def_s error_log_s;

extern ftd_ctl_t *ftd_global_state;

VOID
cmn_err(IN PUCHAR str)
{
    PIO_ERROR_LOG_PACKET    errorLogEntry;
    UCHAR                   EntrySize;
    ULONG                   MessageSize;
    PUCHAR                  StringLoc;
    STRING                  ntString;
    UNICODE_STRING          Message;
    UCHAR                   szMessage[ERROR_LOG_MAXIMUM_SIZE];
    ftd_ctl_t               *ctlp = NULL;

    IN_FCT(cmn_err)

#if FTD_DEBUG
    FTDDebug(FTDERRORS, ("%s.\n", str));
#endif

    // Get a pointer to the global data.
    if ((ctlp = ftd_global_state) == NULL) 
    {
        ASSERT(ctlp);
    
        OUT_FCT(cmn_err)
        return;         /* invalid minor number */
    }

    strcpy(szMessage, str);
    EntrySize = sizeof(IO_ERROR_LOG_PACKET) - sizeof(ULONG) + (strlen(szMessage) * sizeof(WCHAR))+ sizeof(UNICODE_NULL);
    if (EntrySize > ERROR_LOG_MAXIMUM_SIZE) 
    {
        EntrySize -= ERROR_LOG_MAXIMUM_SIZE;
        EntrySize = strlen(szMessage) - EntrySize;

        szMessage[EntrySize] = '\0';
    }

    RtlInitAnsiString(&ntString, szMessage);
    RtlAnsiStringToUnicodeString(&Message, &ntString, TRUE);

    MessageSize = (wcslen(Message.Buffer)*sizeof(WCHAR)) + sizeof(UNICODE_NULL);
    EntrySize = sizeof(IO_ERROR_LOG_PACKET) - sizeof(ULONG) + MessageSize;
    if (EntrySize > ERROR_LOG_MAXIMUM_SIZE) 
    {
        EntrySize = ERROR_LOG_MAXIMUM_SIZE;
    }

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        ctlp->DriverObject,
        EntrySize
        );

    if (errorLogEntry != NULL) 
    {
    
        errorLogEntry->MajorFunctionCode = (UCHAR)0;
        errorLogEntry->RetryCount = (UCHAR)-1;
        errorLogEntry->DumpDataSize = 0;
        errorLogEntry->NumberOfStrings = 1;
        errorLogEntry->StringOffset = sizeof(IO_ERROR_LOG_PACKET) - sizeof(ULONG);
        errorLogEntry->EventCategory = 0;
        errorLogEntry->ErrorCode = FTD_SIMPLE_KERNEL_MESSAGE;
        errorLogEntry->UniqueErrorValue = 0;
        errorLogEntry->FinalStatus = STATUS_SUCCESS;
        errorLogEntry->SequenceNumber = (ULONG)-1;
        errorLogEntry->IoControlCode = 0;
    
        StringLoc = ((PUCHAR)errorLogEntry) + errorLogEntry->StringOffset;
        RtlCopyMemory (StringLoc,Message.Buffer,MessageSize);
    
        IoWriteErrorLogEntry(errorLogEntry);
    
    }

    RtlFreeUnicodeString(&Message);

    OUT_FCT(cmn_err)
}

void 
ftd_err(int s, char *m, int l, char *f, ...)
{ 
    va_list ap;
    char *bp;
    int rtnval;
    char *llvl;

    char _vmsg[FTDERRMSGBUFSZ]; /* varargs format scratch */

    IN_FCT(ftd_err)

    /* 
     * format message buffer to look like:
     * ProductName : <level>: <file name>[<line number>]: formatted error message
     */
    bp = &error_log_s._errmsg[0];
    bp[0] = '\0'; 
    bp += strlen(bp);
        va_start(ap, f);
    rtnval = kvprintf(f, NULL, (void *)bp, 10, ap);
        va_end(ap);
        bp[rtnval] = '\0';

    cmn_err(&error_log_s._errmsg[0]);

    OUT_FCT(ftd_err)
}


/*-
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
char const hex2ascii_data[] = "0123456789abcdefghijklmnopqrstuvwxyz";
#define hex2ascii(hex)  (hex2ascii_data[hex])

/*
 * Scaled down version of _sprintf(3).
 */
FTD_PRIVATE int
_sprintf(char *buf, const char *cfmt, ...)
{
    int retval;
    va_list ap;

    IN_FCT(_sprintf)

    va_start(ap, cfmt);
    retval = kvprintf(cfmt, NULL, (void *)buf, 10, ap);
    buf[retval] = '\0';
    va_end(ap);

    OUT_FCT(_sprintf)
    return retval;
}

/*
 * Put a number (base <= 16) in a buffer in reverse order; return an
 * optional length and a pointer to the NULL terminated (preceded?)
 * buffer.
 */
FTD_PRIVATE char *
ksprintn(ul, base, lenp)
    u_long ul;
    int base, *lenp;
{                   /* A long in base 8, plus NULL. */
    static char buf[sizeof(long) * NBBY / 3 + 2];
    char *p;

    IN_FCT(ksprintn)
    p = buf;
    do 
    {
        *++p = hex2ascii(ul % base);
    } while (ul /= base);
    if (lenp)
        *lenp = p - buf;

    OUT_FCT(ksprintn)
    return (p);
}

/*
 * Scaled down version of printf(3).
 *
 * Two additional formats:
 *
 * The format %b is supported to decode error registers.
 * Its usage is:
 *
 *  printf("reg=%b\n", regval, "<base><arg>*");
 *
 * where <base> is the output base expressed as a control character, e.g.
 * \10 gives octal; \20 gives hex.  Each arg is a sequence of characters,
 * the first of which gives the bit number to be inspected (origin 1), and
 * the next characters (up to a control character, i.e. a character <= 32),
 * give the name of the register.  Thus:
 *
 *  kvprintf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 *
 * would produce output:
 *
 *  reg=3<BITTWO,BITONE>
 *
 * XXX:  %D  -- Hexdump, takes pointer and separator string:
 *      ("%6D", ptr, ":")   -> XX:XX:XX:XX:XX:XX
 *      ("%*D", len, ptr, " " -> XX XX XX XX ...
 */
FTD_PRIVATE int
kvprintf(char const *fmt, void (*func)(int, void*), void *arg, int radix, va_list ap)
{
#define PCHAR(c) {int cc=(c); if (func) (*func)(cc,arg); else *d++ = (char)cc; retval++; }
    char *p, *q, *d;
    u_char *up;
    int ch, n;
    u_long ul;
    int base, lflag, tmp, width, ladjust, sharpflag, neg, sign, dot;
    int dwidth;
    char padc;
    int retval = 0;

    IN_FCT(kvprintf)

    if (!func)
        d = (char *) arg;
    else
        d = NULL;

    if (fmt == NULL)
        fmt = "(fmt null)\n";

    if (radix < 2 || radix > 36)
        radix = 10;

    for (;;) 
    {
        padc = ' ';
        width = 0;
        while ((ch = (u_char)*fmt++) != '%') 
        {
            if (ch == '\0')
            {
                OUT_FCT(kvprintf)
                return retval;
            }
            PCHAR(ch);
        }
        lflag = 0; ladjust = 0; sharpflag = 0; neg = 0;
        sign = 0; dot = 0; dwidth = 0;
reswitch:   switch (ch = (u_char)*fmt++) {
        case '.':
            dot = 1;
            goto reswitch;
        case '#':
            sharpflag = 1;
            goto reswitch;
        case '+':
            sign = 1;
            goto reswitch;
        case '-':
            ladjust = 1;
            goto reswitch;
        case '%':
            PCHAR(ch);
            break;
        case '*':
            if (!dot) {
                width = va_arg(ap, int);
                if (width < 0) {
                    ladjust = !ladjust;
                    width = -width;
                }
            } else {
                dwidth = va_arg(ap, int);
            }
            goto reswitch;
        case '0':
            if (!dot) {
                padc = '0';
                goto reswitch;
            }
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
                for (n = 0;; ++fmt) {
                    n = n * 10 + ch - '0';
                    ch = *fmt;
                    if (ch < '0' || ch > '9')
                        break;
                }
            if (dot)
                dwidth = n;
            else
                width = n;
            goto reswitch;
        case 'b':
            ul = va_arg(ap, int);
            p = va_arg(ap, char *);
            for (q = ksprintn(ul, *p++, NULL); *q;)
                PCHAR(*q--);

            if (!ul)
                break;

            for (tmp = 0; *p;) {
                n = *p++;
                if (ul & (1 << (n - 1))) {
                    PCHAR(tmp ? ',' : '<');
                    for (; (n = *p) > ' '; ++p)
                        PCHAR(n);
                    tmp = 1;
                } else
                    for (; *p > ' '; ++p)
                        continue;
            }
            if (tmp)
                PCHAR('>');
            break;
        case 'c':
            PCHAR(va_arg(ap, int));
            break;
        case 'D':
            up = va_arg(ap, u_char *);
            p = va_arg(ap, char *);
            if (!width)
                width = 16;
            while(width--) {
                PCHAR(hex2ascii(*up >> 4));
                PCHAR(hex2ascii(*up & 0x0f));
                up++;
                if (width)
                    for (q=p;*q;q++)
                        PCHAR(*q);
            }
            break;
        case 'd':
            ul = lflag ? va_arg(ap, long) : va_arg(ap, int);
            sign = 1;
            base = 10;
            goto number;
        case 'l':
            lflag = 1;
            goto reswitch;
        case 'n':
            ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
            base = radix;
            goto number;
        case 'o':
            ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
            base = 8;
            goto number;
        case 'p':
            ul = (u_long)va_arg(ap, void *);
            base = 16;
            PCHAR('0');
            PCHAR('x');
            goto number;
        case 's':
            p = va_arg(ap, char *);
            if (p == NULL)
                p = "(null)";
            if (!dot)
                n = strlen (p);
            else
                for (n = 0; n < dwidth && p[n]; n++)
                    continue;

            width -= n;

            if (!ladjust && width > 0)
                while (width--)
                    PCHAR(padc);
            while (n--)
                PCHAR(*p++);
            if (ladjust && width > 0)
                while (width--)
                    PCHAR(padc);
            break;
        case 'u':
            ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
            base = 10;
            goto number;
        case 'x':
            ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
            base = 16;
number:         if (sign && (long)ul < 0L) {
                neg = 1;
                ul = -(long)ul;
            }
            p = ksprintn(ul, base, &tmp);
            if (sharpflag && ul != 0) {
                if (base == 8)
                    tmp++;
                else if (base == 16)
                    tmp += 2;
            }
            if (neg)
                tmp++;

            if (!ladjust && width && (width -= tmp) > 0)
                while (width--)
                    PCHAR(padc);
            if (neg)
                PCHAR('-');
            if (sharpflag && ul != 0) {
                if (base == 8) {
                    PCHAR('0');
                } else if (base == 16) {
                    PCHAR('0');
                    PCHAR('x');
                }
            }

            while (*p)
                PCHAR(*p--);

            if (ladjust && width && (width -= tmp) > 0)
                while (width--)
                    PCHAR(padc);

            break;
        default:
            PCHAR('%');
            if (lflag)
                PCHAR('l');
            PCHAR(ch);
            break;
        }
    }
#undef PCHAR

    OUT_FCT(kvprintf)
}


#endif // #ifdef OLD_EVENT_LOG_SUPPORT


