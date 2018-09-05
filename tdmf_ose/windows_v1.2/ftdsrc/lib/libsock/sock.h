/*
 * sock.h - generic socket interface	
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

#ifndef _SOCK_H
#define _SOCK_H 

#if !defined(_WINDOWS) 
#include <sys/types.h>
#include <sys/uio.h>
#endif

#ifdef __cplusplus
extern "C"{ 
#endif

#include "tcp.h"

#define MAXHOST			128
#define LOCALHOSTIP		0x100007f	// 127.0.0.1	

#define SOCK_CONNECT		0x0001
#define SOCK_READABLE		0x0010
#define SOCK_WRITABLE		0x0100
#define SOCK_HUP			0x1000

#define SET_SOCK_CONNECT(x)		((x) |= SOCK_CONNECT)
#define GET_SOCK_CONNECT(x)		((x) & SOCK_CONNECT)
#define UNSET_SOCK_CONNECT(x)	((x) &= 0xfffe)

#define SET_SOCK_READABLE(x)	((x) |= SOCK_READABLE)
#define GET_SOCK_READABLE(x)	((x) & SOCK_READABLE)
#define UNSET_SOCK_READABLE(x)	((x) &= 0xffef)

#define SET_SOCK_WRITABLE(x)	((x) |= SOCK_WRITABLE)
#define GET_SOCK_WRITABLE(x)	((x) & SOCK_WRITABLE)
#define UNSET_SOCK_WRITABLE(x)	((x) &= 0xfeff)

#define SET_SOCK_HUP(x)			((x) |= SOCK_HUP)
#define GET_SOCK_HUP(x)			((x) & SOCK_HUP)
#define UNSET_SOCK_HUP(x)		((x) &= 0xefff)

/* socket structure */
typedef struct sock_s {
	SOCKET sock;				/* socket for remote communications		*/
#if defined(_WINDOWS)
	HANDLE	hEvent;
	int	bBlocking;
#endif
	int type;					/* protocol type - AF_INET, AF_UNIX		*/
	int family;					/* protocol family - STREAM, DGRAM		*/
	int tsbias;					/* time diff between local and remote	*/
	char lhostname[MAXHOST];	/* local host name						*/
	char rhostname[MAXHOST];	/* remote host name						*/
	unsigned long lip;			/* local ip address						*/
	unsigned long rip;			/* remote ip address					*/
	unsigned long lhostid;		/* local hostid							*/
	unsigned long rhostid;		/* remote hostid						*/
	int port;					/* connection port number				*/
	int readcnt;				/* number of bytes read					*/
	int writecnt;				/* number of bytes written				*/
	int flags;					/* state bits							*/
} sock_t;

/* prototypes */
extern sock_t *sock_create(void);
extern int sock_delete(sock_t **sockpp);
extern int sock_init(sock_t *sockp, char *lhostname, char *rhostname, unsigned long lip,
	 unsigned long rip, int type, int family, int create, int verifylocal);
extern SOCKET sock_socket(int family);
extern int sock_bind(sock_t *sockp, int port);
extern int sock_connect(sock_t *sockp, int port);
extern int sock_connect_nonb(sock_t *sockp, int port, int sec, int usec,
	int *sockerr);
extern int sock_disconnect(sock_t *sockp);
extern int sock_listen(sock_t *sockp, int port);
extern int sock_accept(sock_t *listener, sock_t *sockp);
extern int sock_accept_nonb(sock_t *listener, sock_t *sockp,
	int sec, int usec);
extern int sock_set_opt(sock_t *sockp, int level, int optname, char *optval, int optlen);
extern int sock_get_opt(sock_t *sockp, int level, int optname, char *optval, int *optlen);
extern int sock_send(sock_t *sockp, char *buf, int len);
extern int sock_send_vector(sock_t *sockp, struct iovec *iov, int iovcnt);
extern int sock_recv(sock_t *sockp, char *buf, int len);
extern int sock_check_connect(sock_t *sockp);
extern int sock_check_recv(sock_t *sockp, int timeo);
extern int sock_check_send(sock_t *sockp, int timeo);
extern int sock_set_nonb(sock_t *sockp);
extern int sock_set_b(sock_t *sockp);
extern int sock_is_me(sock_t *sockp, int local);
extern int sock_ipstring_to_ip(char *ipstring, unsigned long *ip);
extern int sock_name_to_ip(char *name, unsigned long *ip);
extern int sock_ip_to_ipstring(unsigned long ip, char *ipstring);
extern int sock_ip_to_name(unsigned long ip, char *name);
extern int sock_get_local_ip_list(unsigned long *iplist);
extern int sock_errno(void);
extern char *sock_strerror(int errnum);
extern struct hostent *sock_gethostbyname(char *hostname);

#if defined(_WINDOWS)
extern int sock_startup(void);
extern int sock_cleanup(void);
extern char *sock_strerror(int errnum);
#endif

#ifdef __cplusplus 
}
#endif

#endif /* _SOCK_H */
