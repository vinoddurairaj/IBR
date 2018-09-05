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
 * $Id: ftd_klog.c,v 1.5 2001/10/01 17:36:42 bouchard Exp $
 *
 */

/* ftd common kernel error logging interface */

#if defined(MAIN)
#include <stdio.h>
#endif /* defined(MAIN) */

#include "ftd_kern_ctypes.h"
#include "ftd_klog.h"



/* ported to handle long long args */
#define LL_SPRINTF
/* use bsd code for sprintf */
#define BSDKSPRINTF 

#if defined(FTD_PRIVATE)
#undef FTD_PRIVATE
#endif /* defined(FTD_PRIVATE) */

#if defined(FTDDEBUG)
#define FTD_PRIVATE
#else /* defined(FTDDEBUG) */
#define FTD_PRIVATE static
#endif /* defined(FTDDEBUG) */

/* for FTD_DPRINTF */
int ftddprintf=0;

/* ftd_klog.c */
FTD_PRIVATE ftd_int32_t __sprintf (ftd_char_t * buf, const ftd_char_t * cfmt,...);
FTD_PRIVATE ftd_char_t *ksprintn (ftd_uint64_t ul, ftd_int32_t base, ftd_int32_t * lenp);
FTD_PRIVATE ftd_int32_t kvprintf (ftd_char_t const *fmt, ftd_void_t (*func) (int, ftd_void_t *), ftd_void_t * arg, ftd_int32_t radix, va_list ap);


ftd_void_t
ftd_err (ftd_int32_t s, ftd_char_t * m, ftd_int32_t l, ftd_char_t * f,...)
{
  va_list ap;
  ftd_char_t *bp;
  ftd_int32_t rtnval;
  ftd_char_t *llvl;

  struct error_log_def_s error_log_s;

  ftd_char_t _vmsg[FTDERRMSGBUFSZ];	/* varargs format scratch */

  bzero ((void*)&error_log_s,sizeof(error_log_s));

  /* 
   * format message buffer to look like:
   * QNM: <level>: <file name>[<line number>]: formatted error message
   */
  bp = &error_log_s._errmsg[0];
#if defined(_AIX)
  switch (s)
    {
    case DBGLVL:
      llvl = "DEBUG";
      break;
    case NTCLVL:
      llvl = "NOTICE";
      break;
    case WRNLVL:
      llvl = "WARNING";
      break;
    default:
      llvl = "UNKNOWN";
      break;
    }
  __sprintf (bp, "%s: %s: %s[%d]: ", QNM, llvl, m, l);
#else 
	#if defined(BSDKSPRINTF) 
		#if defined(HPUX)
		   sprintf(bp, FTDERRMSGBUFSZ, "%s: %s[%d]: ", QNM, m, l);
		#else
		  __sprintf (bp, "%s: %s[%d]: ", QNM, m, l);
		#endif
	#else  
  		sprintf (bp, "%s: %s[%d]: ", QNM, m, l);
	#endif /* defined(BSDKSPRINTF) */
#endif 
  bp += strlen (bp);
  va_start (ap, f);
#if defined(BSDKSPRINTF) 
  rtnval = kvprintf (f, NULL, (ftd_void_t *) bp, 10, ap);
#else  /* defined(BSDKSPRINTF) */
  rtnval = vsprintf (bp, f, ap);
#endif /* defined(BSDKSPRINTF) */
  va_end (ap);
#if defined(notdef)
  bp[rtnval] = '\0';
#endif /* defined(notdef) */

#if defined(SOLARIS) || defined(HPUX)
	#if defined(MAIN)
		printf ("(LEVEL) %d ", s);
		printf ("%s", &error_log_s._errmsg[0]);
	#else 
	  	cmn_err (s, "%s\n", &error_log_s._errmsg[0]); 
#endif /* defined(MAIN) */
#elif  defined(_AIX)
  /* for now, error_id is undefined */
  error_log_s.errhead.error_id = ERRID_SYSLOG;
  __sprintf (&error_log_s.errhead.resource_name[0], "%s", QNM);
  errsave ((ftd_char_t *) & error_log_s,
	   ERR_REC_SIZE + (strlen (error_log_s._errmsg) + 1));
#endif

}

#if defined(BSDKSPRINTF)

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
ftd_char_t const hex2ascii_data[] = "0123456789abcdefghijklmnopqrstuvwxyz";
#define hex2ascii(hex)  (hex2ascii_data[hex])

/*
 * Scaled down version of __sprintf(3).
 */
FTD_PRIVATE ftd_int32_t
__sprintf (ftd_char_t * buf, const ftd_char_t * cfmt,...)
{
  ftd_int32_t retval;
  va_list ap;

  va_start (ap, cfmt);
  retval = kvprintf (cfmt, NULL, (ftd_void_t *) buf, 10, ap);
  buf[retval] = '\0';
  va_end (ap);
  return retval;
}

/*
 * Put a number (base <= 16) in a buffer in reverse order; return an
 * optional length and a pointer to the NULL terminated (preceded?)
 * buffer.
 */
FTD_PRIVATE ftd_char_t *
ksprintn (ul, base, lenp)
     ftd_uint64_t ul;
     ftd_int32_t base, *lenp;
{				/* A long in base 8, plus NULL. */
  static ftd_char_t buf[sizeof (ftd_int32_t) * NBBY / 3 + 2];
  ftd_char_t *p;

  p = buf;
  do
    {
      *++p = hex2ascii (ul % base);
    }
  while (ul /= base);
  if (lenp)
    *lenp = p - buf;
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
 *      printf("reg=%b\n", regval, "<base><arg>*");
 *
 * where <base> is the output base expressed as a control character, e.g.
 * \10 gives octal; \20 gives hex.  Each arg is a sequence of characters,
 * the first of which gives the bit number to be inspected (origin 1), and
 * the next characters (up to a control character, i.e. a character <= 32),
 * give the name of the register.  Thus:
 *
 *      kvprintf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 *
 * would produce output:
 *
 *      reg=3<BITTWO,BITONE>
 *
 * XXX:  %D  -- Hexdump, takes pointer and separator string:
 *              ("%6D", ptr, ":")   -> XX:XX:XX:XX:XX:XX
 *              ("%*D", len, ptr, " " -> XX XX XX XX ...
 */
FTD_PRIVATE ftd_int32_t
kvprintf (ftd_char_t const *fmt, ftd_void_t (*func) (int, ftd_void_t *), ftd_void_t * arg, ftd_int32_t radix, va_list ap)
{
#define PCHAR(c) {ftd_int32_t cc=(c); if (func) (*func)(cc,arg); else *d++ = cc; retval++; }
  ftd_char_t *p, *q, *d;
  ftd_uchar_t *up;
  ftd_int32_t ch, n;
  ftd_uint64_t ul;
  ftd_int32_t base, lflag, tmp, width, ladjust, sharpflag, neg, sign, dot;
  ftd_int32_t dwidth;
  ftd_char_t padc;
  ftd_int32_t retval = 0;

  if (!func)
    d = (ftd_char_t *) arg;
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
      while ((ch = (ftd_uchar_t) * fmt++) != '%')
	{
	  if (ch == '\0')
	    return retval;
	  PCHAR (ch);
	}
      lflag = 0;
      ladjust = 0;
      sharpflag = 0;
      neg = 0;
      sign = 0;
      dot = 0;
      dwidth = 0;
    reswitch:switch (ch = (ftd_uchar_t) * fmt++)
	{
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
	  PCHAR (ch);
	  break;
	case '*':
	  if (!dot)
	    {
	      width = va_arg (ap, ftd_int32_t);
	      if (width < 0)
		{
		  ladjust = !ladjust;
		  width = -width;
		}
	    }
	  else
	    {
	      dwidth = va_arg (ap, ftd_int32_t);
	    }
	  goto reswitch;
	case '0':
	  if (!dot)
	    {
	      padc = '0';
	      goto reswitch;
	    }
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  for (n = 0;; ++fmt)
	    {
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
	  ul = va_arg (ap, ftd_int32_t);
	  p = va_arg (ap, ftd_char_t *);
	  for (q = ksprintn (ul, *p++, NULL); *q;)
	    PCHAR (*q--);

	  if (!ul)
	    break;

	  for (tmp = 0; *p;)
	    {
	      n = *p++;
	      if (ul & (1 << (n - 1)))
		{
		  PCHAR (tmp ? ',' : '<');
		  for (; (n = *p) > ' '; ++p)
		    PCHAR (n);
		  tmp = 1;
		}
	      else
		for (; *p > ' '; ++p)
		  continue;
	    }
	  if (tmp)
	    PCHAR ('>');
	  break;
	case 'c':
	  PCHAR (va_arg (ap, ftd_int32_t));
	  break;
	case 'D':
	  up = va_arg (ap, ftd_uchar_t *);
	  p = va_arg (ap, ftd_char_t *);
	  if (!width)
	    width = 16;
	  while (width--)
	    {
	      PCHAR (hex2ascii (*up >> 4));
	      PCHAR (hex2ascii (*up & 0x0f));
	      up++;
	      if (width)
		for (q = p; *q; q++)
		  PCHAR (*q);
	    }
	  break;
	case 'd':
#if defined(LL_SPRINTF)
switch(lflag) {
	case 0:
	  ul = (unsigned) va_arg (ap, ftd_int32_t);
	break;
	case 1:
	  ul = (unsigned) va_arg (ap, ftd_int32_t);
	break;
	case 2:
	  ul = (unsigned) va_arg (ap, ftd_int64_t);
	break;
}
#else
	  ul = lflag ? va_arg (ap, ftd_int32_t) : va_arg (ap, ftd_int32_t);
#endif /* defined(LL_SPRINTF) */
	  sign = 1;
	  base = 10;
	  goto number;
	case 'l':
	  lflag++;
	  goto reswitch;
	case 'n':
#if defined(LL_SPRINTF)
switch(lflag) {
	case 0:
	  ul = (unsigned) va_arg (ap, ftd_int32_t);
	break;
	case 1:
	  ul = (unsigned) va_arg (ap, ftd_int32_t);
	break;
	case 2:
	  ul = (unsigned) va_arg (ap, ftd_int64_t);
	break;
}
#else
	  ul = lflag ? va_arg (ap, ftd_uint64_t) : va_arg (ap, ftd_uint32_t);
#endif /* defined(LL_SPRINTF) */
	  base = radix;
	  goto number;
	case 'o':
#if defined(LL_SPRINTF)
switch(lflag) {
	case 0:
	  ul = va_arg (ap, ftd_int32_t);
	break;
	case 1:
	  ul = va_arg (ap, ftd_int32_t);
	break;
	case 2:
	  ul = va_arg (ap, ftd_int64_t);
	break;
}
#else
	  ul = lflag ? va_arg (ap, ftd_uint64_t) : va_arg (ap, ftd_uint32_t);
#endif /* defined(LL_SPRINTF) */
	  base = 8;
	  goto number;
	case 'p':
	  ul = (ftd_uint64_t) va_arg (ap, ftd_void_t *);
	  base = 16;
	  PCHAR ('0');
	  PCHAR ('x');
	  goto number;
	case 's':
	  p = va_arg (ap, ftd_char_t *);
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
	      PCHAR (padc);
	  while (n--)
	    PCHAR (*p++);
	  if (ladjust && width > 0)
	    while (width--)
	      PCHAR (padc);
	  break;
	case 'u':
#if defined(LL_SPRINTF)
switch(lflag) {
	case 0:
	  ul = (unsigned) va_arg (ap, ftd_int32_t);
	break;
	case 1:
	  ul = (unsigned) va_arg (ap, ftd_int32_t);
	break;
	case 2:
	  ul = (unsigned) va_arg (ap, ftd_int64_t);
	break;
}
#else
	  ul = lflag ? va_arg (ap, ftd_uint64_t) : va_arg (ap, ftd_uint32_t);
#endif /* defined(LL_SPRINTF) */
	  base = 10;
	  goto number;
	case 'x':
#if defined(LL_SPRINTF)
switch(lflag) {
	case 0:
	  ul = (unsigned) va_arg (ap, ftd_int32_t);
	break;
	case 1:
	  ul = (unsigned) va_arg (ap, ftd_int32_t);
	break;
	case 2:
	  ul = (unsigned) va_arg (ap, ftd_int64_t);
	break;
}
#else
	  ul = lflag ? va_arg (ap, ftd_uint64_t) : va_arg (ap, ftd_uint32_t);
#endif
	  base = 16;
	number:if (sign && (ftd_int32_t) ul < 0L)
	    {
	      neg = 1;
	      ul = -(ftd_int32_t) ul;
	    }
	  p = ksprintn (ul, base, &tmp);
	  if (sharpflag && ul != 0)
	    {
	      if (base == 8)
		tmp++;
	      else if (base == 16)
		tmp += 2;
	    }
	  if (neg)
	    tmp++;

	  if (!ladjust && width && (width -= tmp) > 0)
	    while (width--)
	      PCHAR (padc);
	  if (neg)
	    PCHAR ('-');
	  if (sharpflag && ul != 0)
	    {
	      if (base == 8)
		{
		  PCHAR ('0');
		}
	      else if (base == 16)
		{
		  PCHAR ('0');
		  PCHAR ('x');
		}
	    }

	  while (*p)
	    PCHAR (*p--);

	  if (ladjust && width && (width -= tmp) > 0)
	    while (width--)
	      PCHAR (padc);

	  break;
	default:
	  PCHAR ('%');
	  if (lflag)
	    PCHAR ('l');
	  PCHAR (ch);
	  break;
	}
    }
#undef PCHAR
}
#endif /* defined(BSDKSPRINTF) */

#if defined(MAIN)

#if defined(_AIX)
errsave (ftd_char_t * p, ftd_int32_t len)
{
  struct error_log_def_s *elp = (struct error_log_def_s *) p;
  ftd_char_t *em;

  em = elp->_errmsg;

  fprintf (stderr, "%s\n", em);
}
#endif /* defined(_AIX) */



/* standalone test case driver for ftd_err */
main ()
{
  ftd_char_t mebuf[256];

  FTD_ERR (FTD_DBGLVL,
	   "I've fallen down and I can't get up\n");

  FTD_ERR (FTD_DBGLVL,
	   "I've fallen down and I can't get up: %c\n", 'c');

  FTD_ERR (FTD_DBGLVL,
	   "I've fallen down and I can't get up: %d\n", 1);

  FTD_ERR (FTD_NTCLVL,
	   "I've fallen down and I can't get up: %d\n", -1);

  FTD_ERR (FTD_NTCLVL,
	   "I've fallen down and I can't get up: 0x%08x\n", 0x0000ffff);

  FTD_ERR (FTD_WRNLVL,
	   "I've fallen down and I can't get up: 0x%08x\n", 0xffff0000);

  FTD_ERR (FTD_WRNLVL,
	   "I've fallen down and I can't get up: %s\n", "ow!");

  FTD_ERR (FTD_WRNLVL,
	   "I've fallen down and I can't get up: %s %s\n", "ow!", "ow!");

  FTD_ERR (FTD_WRNLVL,
	   "I've fallen down and I can't get up: 0x%08lx\n", 0x12345678l);

  FTD_ERR (FTD_WRNLVL,
	   "I've fallen down and I can't get up: 0x%016lx\n", 0x12345678l);

  FTD_ERR (FTD_WRNLVL,
	   "I've fallen down and I can't get up: 0x%016llx\n", 
	   0x0000000012345678ll);

  FTD_ERR (FTD_WRNLVL,
	   "I've fallen down and I can't get up: 0x%016llx\n", 
	   0x0000000100000001ll);

  FTD_ERR(FTD_WRNLVL,
	   "I've fallen down and I can't get up: 0x%016x\n", 
	   0x87654321);

  FTD_ERR (FTD_WRNLVL,
	   "I've fallen down and I can't get up: 0x%08x 0x%016x 0x%016llx\n", 
	   0x12345678, 0x87654321, 0x12345678abcdef0ll);

}
#endif /* defined(MAIN) */
