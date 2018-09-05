/*
 * ftd_all.c - FullTime Data driver code for ALL platforms
 *
 * Copyright (c) 1996, 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * $Id: ftd_klog.c,v 1.4 2003/06/03 14:02:38 szg00 Exp $
 *
 */

/* ftd common kernel error logging interface */

#if defined(MAIN)
#include <stdio.h>
#endif /* defined(MAIN) */

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
     * QNM: <level>: <file name>[<line number>]: formatted error message
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

#if defined(MAIN)

/* standalone test case driver for ftd_err */
main()
{
    char mebuf[256];

    IN_FCT(main)

    FTD_ERR(FTD_DBGLVL,
    "I've fallen down and I can't get up\n");

    FTD_ERR(FTD_DBGLVL,
    "I've fallen down and I can't get up: %c\n", 'c');
    
    FTD_ERR(FTD_DBGLVL,
    "I've fallen down and I can't get up: %d\n", 1);
    
    FTD_ERR(FTD_NTCLVL,
    "I've fallen down and I can't get up: %d\n", -1);
    
    FTD_ERR(FTD_NTCLVL,
    "I've fallen down and I can't get up: 0x%08x\n", 0x0000ffff);
    
    FTD_ERR(FTD_WRNLVL,
    "I've fallen down and I can't get up: 0x%08x\n", 0xffff0000);
    
    FTD_ERR(FTD_WRNLVL,
    "I've fallen down and I can't get up: %s\n", "ow!");
    
    FTD_ERR(FTD_WRNLVL,
    "I've fallen down and I can't get up: %s %s\n", "ow!", "ow!");

    OUT_FCT(main)

}
#endif /* defined(MAIN) */
