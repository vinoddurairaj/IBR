/*
 * ftd_port_win.h --
 *
 *
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 *	This header file handles porting issues that occur because of
 *	differences between Windows and Unix. It should be the only
 *	file that contains #ifdefs to handle different flavors of OS.
 */

#ifndef _FTD_PORT_WIN_H
#define _FTD_PORT_WIN_H

//// Mike Pollett need for windows .h files
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
//#endif


/* DEV_BSHIFT is set for 512 bytes */
#define DEV_BSHIFT  9
#define DEV_BSIZE   512

#include <malloc.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <process.h>
#include <signal.h>
#include <winsock2.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <winioctl.h>
#include <limits.h>
#include "values.h" /* MAXINT usage */

#include "un.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include "getopt.h"
#include "misc.h"
#include "values.h" /* MAXINT usage */

#include "sockerrnum.h"		/* libsock */
#include "iov_t.h"			/* struct iovec */

#ifdef __cplusplus
#   define EXTERN extern "C"
#else
#   define EXTERN extern
#endif

#ifdef NO_PROTOTYPES
#define _ANSI_ARGS_(x) ()
#else 
#define _ANSI_ARGS_(x) x
#endif

typedef __int64					ftd_int64_t;
typedef unsigned __int64		ftd_uint64_t;
typedef __int64					offset_t;

#define NBPB					8

/*
 * Supply definitions for macros to query wait status, if not already
 * defined in header files above.
 */

#if FTD_UNION_WAIT
#   define WAIT_STATUS_TYPE union wait
#else
#   define WAIT_STATUS_TYPE int
#endif

#ifndef WIFEXITED
#   define WIFEXITED(stat)  (((*((int *) &(stat))) & 0xff) == 0)
#endif

#ifndef WEXITSTATUS
#   define WEXITSTATUS(stat) (((*((int *) &(stat))) >> 8) & 0xff)
#endif

#ifndef WIFSIGNALED
#   define WIFSIGNALED(stat) (((*((int *) &(stat)))) && ((*((int *) &(stat))) == ((*((int *) &(stat))) & 0x00ff)))
#endif

#ifndef WTERMSIG
#   define WTERMSIG(stat)    ((*((int *) &(stat))) & 0x7f)
#endif

#ifndef WIFSTOPPED
#   define WIFSTOPPED(stat)  (((*((int *) &(stat))) & 0xff) == 0177)
#endif

#ifndef WSTOPSIG
#   define WSTOPSIG(stat)    (((*((int *) &(stat))) >> 8) & 0xff)
#endif

/*
 * Define constants for waitpid() system call if they aren't defined
 * by a system header file.
 */

#ifndef WNOHANG
#   define WNOHANG 1
#endif
#ifndef WUNTRACED
#   define WUNTRACED 2
#endif

/*
 * Define MAXPATHLEN in terms of MAXPATH if available
 */

#ifndef MAXPATH
#define MAXPATH MAX_PATH
#endif /* MAXPATH */

#ifndef MAXPATHLEN
#define MAXPATHLEN MAXPATH
#endif /* MAXPATHLEN */

#ifndef F_OK
#    define F_OK 00
#endif
#ifndef X_OK
#    define X_OK 01
#endif
#ifndef W_OK
#    define W_OK 02
#endif
#ifndef R_OK
#    define R_OK 04
#endif

/*
 * On systems without symbolic links (i.e. S_IFLNK isn't defined)
 * define "lstat" to use "stat" instead.
 */

#ifndef S_IFLNK
#   define lstat stat
#endif

/*
 * Define macros to query file type bits, if they're not already
 * defined.
 */

#ifndef S_ISREG
#   ifdef S_IFREG
#       define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#   else
#       define S_ISREG(m) 0
#   endif
# endif
#ifndef S_ISDIR
#   ifdef S_IFDIR
#       define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#   else
#       define S_ISDIR(m) 0
#   endif
# endif
#ifndef S_ISCHR
#   ifdef S_IFCHR
#       define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#   else
#       define S_ISCHR(m) 0
#   endif
# endif
#ifndef S_ISBLK
#   ifdef S_IFBLK
#       define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#   else
#       define S_ISBLK(m) 0
#   endif
# endif
#ifndef S_ISFIFO
#   ifdef S_IFIFO
#       define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#   else
#       define S_ISFIFO(m) 0
#   endif
# endif
#ifndef S_ISLNK
#   ifdef S_IFLNK
#       define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#   else
#       define S_ISLNK(m) 0
#   endif
# endif
#ifndef S_ISSOCK
#   ifdef S_IFSOCK
#       define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#   else
#       define S_ISSOCK(m) 0
#   endif
# endif

#define _O_SEQUENTIAL   0x0020  /* file access is primarily sequential */
#define O_SYNC _O_SEQUENTIAL

#define S_IRUSR     000000400
#define S_IWUSR     000000200

/*
 * Define pid_t and uid_t if they're not already defined.
 */

#if ! FTD_PID_T
#   define pid_t HANDLE
#endif
#if ! FTD_UID_T
#   define uid_t int
#endif
#if ! FTD_DADDR_T
#   define daddr_t long
#endif
#if ! FTD_CADDR_T
#   define caddr_t char *
#endif

#define usleep(x)	Sleep((x)/1000)
#define sleep(x)	Sleep((x)*1000)

/*
 * Declarations for Windows specific functions.
 */

EXTERN void		FtdWinConvertError _ANSI_ARGS_((DWORD errCode));
EXTERN void		FtdWinConvertWSAError _ANSI_ARGS_((DWORD errCode));

#endif /* _FTD_PORT_WIN_H */
