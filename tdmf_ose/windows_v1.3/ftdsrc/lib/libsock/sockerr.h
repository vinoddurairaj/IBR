/*
 * sockerr.h - generic socket interface errors	
 * 
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#if !defined(_SOCKERR_H_)
#define		_SOCKERR_H_

#if defined(_WINDOWS)

static int wsaErrorTable[] = {
    EINTR,			/* WSAEINTR		- 4 */
    EINVAL,			/* - 5 */
    EINVAL,			/* - 6 */
    EINVAL,			/* - 7 */
    EINVAL,			/* - 8 */
    EINVAL,			/* - 9 */
    EINVAL,			/* - 10 */
    EINVAL,			/* - 11 */
    EINVAL,			/* - 12 */
    EACCES,			/* WSAEACCES	- 13 */
    EFAULT,			/* WSAEFAULT	- 14 */
    EINVAL,			/* - 15 */
    EINVAL,			/* - 16 */
    EINVAL,			/* - 17 */
    EINVAL,			/* - 18 */
    EINVAL,			/* - 19 */
    EINVAL,			/* - 20 */
    EINVAL,			/* - 21 */
    EINVAL,			/* - 22 */
    EINVAL,			/* - 23 */
    EMFILE,			/* WSAEMFILE	- 24 */
    EINVAL,			/* - 25 */
    EINVAL,			/* - 26 */
    EINVAL,			/* - 27 */
    EINVAL,			/* - 28 */
    EINVAL,			/* - 29 */
    EINVAL,			/* - 30 */
    EINVAL,			/* - 31 */
    EINVAL,			/* - 32 */
    EINVAL,			/* - 33 */
    EINVAL,			/* - 34 */
    EWOULDBLOCK,	/* WSAEWOULDBLOCK	- 35 */
    EINPROGRESS,	/* WSAEINPROGRESS	- 36 */
    EALREADY,		/* WSAEALREADY		- 37 */
    ENOTSOCK,		/* WSAENOTSOCK		- 38 */
    EDESTADDRREQ,	/* WSAEDESTADDRREQ	- 39 */
    EMSGSIZE,		/* WSAEMSGSIZE		- 40 */
    EPROTOTYPE,		/* WSAEPROTOTYPE	- 41 */
    ENOPROTOOPT,	/* WSAENOPROTOOPT	- 42 */
    EPROTONOSUPPORT,/* WSAEPROTONOSUPPORT	- 43 */
    ESOCKTNOSUPPORT,/* WSAESOCKTNOSUPPORT	- 44 */
    EOPNOTSUPP,		/* WSAEOPNOTSUPP	- 45 */
    EPFNOSUPPORT,	/* WSAEPFNOSUPPORT	- 46 */
    EAFNOSUPPORT,	/* WSAEAFNOSUPPORT	- 47 */
    EADDRINUSE,		/* WSAEADDRINUSE	- 48 */
    EADDRNOTAVAIL,	/* WSAEADDRNOTAVAIL	- 49 */
    ENETDOWN,		/* WSAENETDOWN		- 50 */
    ENETUNREACH,	/* WSAENETUNREACH	- 51 */
    ENETRESET,		/* WSAENETRESET		- 52 */
    ECONNABORTED,	/* WSAECONNABORTED	- 53 */
    ECONNRESET,		/* WSAECONNRESET	- 54 */
    ENOBUFS,		/* WSAENOBUFS		- 55 */
    EISCONN,		/* WSAEISCONN		- 56 */
    ENOTCONN,		/* WSAENOTCONN		- 57 */
    ESHUTDOWN,		/* WSAESHUTDOWN		- 58 */
    ETOOMANYREFS,	/* WSAETOOMANYREFS	- 59 */
    ETIMEDOUT,		/* WSAETIMEDOUT		- 60 */
    ECONNREFUSED,	/* WSAECONNREFUSED	- 61 */
    ELOOP,			/* WSAELOOP			- 62 */
    ENAMETOOLONG,	/* WSAENAMETOOLONG	- 63 */
    EHOSTDOWN,		/* WSAEHOSTDOWN		- 64 */
    EHOSTUNREACH,	/* WSAEHOSTUNREACH	- 65 */
    ENOTEMPTY,		/* WSAENOTEMPTY		- 66 */
    EWOULDBLOCK,	/* WSAEPROCLIM		- 67 */
    EUSERS,			/* WSAEUSERS		- 68 */
    EDQUOT,			/* WSAEDQUOT		- 69 */
    ESTALE,			/* WSAESTALE		- 70 */
    EREMOTE,		/* WSAEREMOTE		- 71 */
};

char *sock_errstr[] = {
	"Unknown error",								/* 0 */
	"Operation not permitted", 						/* EPERM */
	"No such file or directory",					/* ENOENT */
	"No such process", 								/* ESRCH */
	"Interrupted system call", 						/* EINTR */
	"I/O error", 									/* EIO */
	"No such device or address",					/* ENXIO */
	"Arg list too long", 							/* E2BIG */
	"Exec format error", 							/* ENOEXEC */
	"Bad file number", 								/* EBADF */
	"No child processes", 							/* ECHILD */	/* 10 */
	"Try again", 									/* EAGAIN */
	"Out of memory", 								/* ENOMEM */
	"Permission denied", 							/* EACCES */
	"Bad address", 									/* EFAULT */
	"Block device required",						/* ENOTBLK */
	"Device or resource busy", 						/* EBUSY */
	"File exists", 									/* EEXIST */
	"Cross-device link",			 				/* EXDEV */
	"No such device", 								/* ENODEV */
	"Not a directory", 								/* ENOTDIR */	/* 20 */
	"Is a directory", 								/* EISDIR */
	"Invalid argument",								/* EINVAL */
	"File table overflow", 							/* ENFILE */
	"Too many open files",							/* EMFILE */
	"Not a typewriter", 							/* ENOTTY */
	"Text file busy",								/* ETXTBSY */
	"File too large", 								/* EFBIG */
	"No space left on device",						/* ENOSPC */
	"Illegal seek", 								/* ESPIPE */
	"Read-only file system", 						/* EROFS */		/* 30 */
	"Too many links", 								/* EMLINK */
	"Broken pipe", 									/* EPIPE */
	"Math argument out of domain of func",			/* EDOM */
	"Math result not representable",			 	/* ERANGE */
	"Operation would block", 						/* EWOULDBLOCK */
	"Resource deadlock would occur",				/* EDEADLK */
	"",												/* no win equiv. */
	"File name too long", 							/* ENAMETOOLONG */
	"No record locks available",					/* ENOLCK */
	"Function not implemented", 					/* ENOSYS */	/* 40 */
	"Directory not empty", 							/* ENOTEMPTY */
	"Illegal byte sequence",						/* EILSEQ */
	"Identifier removed", 							/* EIDRM */
	"Channel number out of range", 					/* ECHRNG */
	"Level 2 not synchronized", 					/* EL2NSYNC */
	"Level 3 halted", 								/* EL3HLT */
	"Level 3 reset", 								/* EL3RST */
	"Link number out of range", 					/* ELNRNG */
	"Protocol driver not attached",					/* EUNATCH */
	"No CSI structure available", 					/* ENOCSI */	/* 50 */
	"Level 2 halted", 								/* EL2HLT */
	"Invalid exchange", 							/* EBADE */
	"Invalid request descriptor", 					/* EBADR */
	"Exchange full", 								/* EXFULL */
	"No anode", 									/* ENOANO */
	"Invalid request code",					 		/* EBADRQC */
	"Invalid slot", 								/* EBADSLT */
	"File locking deadlock error", 					/* EDEADLOCK */
	"Bad font file format",							/* EBFONT */
	"Device not a stream", 							/* ENOSTR */	/* 60 */
	"No data available", 							/* ENODATA */
	"Timer expired", 								/* ETIME */
	"Out of streams resources",			 			/* ENOSR */
	"Machine is not on the network", 				/* ENONET */
	"Package not installed", 						/* ENOPKG */
	"Object is remote", 							/* EREMOTE */
	"Link has been severed", 						/* ENOLINK */
	"Advertise error", 								/* EADV */
	"Srmount error", 								/* ESRMNT */
	"Communication error on send",					/* ECOMM */		/* 70 */
	"Protocol error", 								/* EPROTO */
	"Multihop attempted", 							/* EMULTIHOP */
	"RFS specific error", 							/* EDOTDOT */
	"Not a data message", 							/* EBADMSG */
	"Value too large for defined data type",		/* EOVERFLOW */
	"Name not unique on network", 					/* ENOTUNIQ */
	"File descriptor in bad state",					/* EBADFD */
	"Remote address changed", 						/* EREMCHG */
	"Can not access a needed shared library",		/* ELIBACC */
	"Accessing a corrupted shared library",			/* ELIBBAD */	/* 80 */
	".lib section in a.out corrupted",				/* ELIBSCN */
	"Attempting to link in too many shared libraries", 	/* ELIBMAX */
	"Cannot exec a shared library directly",		/* ELIBEXEC */
	"",												/* no msg */
	"Interrupted system call should be restarted", 	/* ERESTART */
	"Streams pipe error", 							/* ESTRPIPE */
	"Too many users", 								/* EUSERS */
	"Socket operation on non-socket", 				/* ENOTSOCK */
	"Destination address required",				 	/* EDESTADDRREQ */
	"Message too long", 							/* EMSGSIZE */	/* 90 */
	"Protocol wrong type for socket",				/* EPROTOTYPE */
	"Protocol not available", 						/* ENOPROTOOPT */
	"Protocol not supported",						/* EPROTONOSUPPORT */
	"Socket type not supported", 					/* ESOCKTNOSUPPORT */
	"Operation not supported on transport endpoint",/* EOPNOTSUPP */
	"Protocol family not supported", 				/* EPFNOSUPPORT */
	"Address family not supported by protocol", 	/* EAFNOSUPPORT */
	"Address already in use", 						/* EADDRINUSE */
	"Cannot assign requested address", 				/* EADDRNOTAVAIL */
	"Network is down", 								/* ENETDOWN */	/* 100 */	
	"Network is unreachable",						/* ENETUNREACH */
	"Network dropped connection because of reset", 	/* ENETRESET */
	"Software caused connection abort", 			/* ECONNABORTED */
	"Connection reset by peer", 					/* ECONNRESET */
	"No buffer space available", 					/* ENOBUFS */
	"Transport endpoint is already connected", 		/* EISCONN */
	"Transport endpoint is not connected", 			/* ENOTCONN */
	"Cannot send after transport endpoint shutdown",/* ESHUTDOWN */
	"Too many references: cannot splice", 			/* ETOOMANYREFS */
	"Connection timed out",							/* ETIMEDOUT */	/* 110 */	
	"Connection refused", 							/* ECONNREFUSED */
	"Host is down",									/* EHOSTDOWN */
	"No route to host", 							/* EHOSTUNREACH */
	"Operation already in progress",				/* EALREADY */
	"Operation now in progress",					/* EINPROGRESS */
	"Stale NFS file handle", 						/* ESTALE */
	"Structure needs cleaning", 					/* EUCLEAN */
	"Not a XENIX named type file",					/* ENOTNAM */
	"No XENIX semaphores available",				/* ENAVAIL */
	"Is a named type file",							/* EISNAM */	/* 120 */
	"Remote I/O error",								/* EREMOTEIO */
	"Quota exceeded", 								/* EDQUOT */
	"Too many levels of symbolic links", 			/* ELOOP */
	NULL
};

#endif

#endif