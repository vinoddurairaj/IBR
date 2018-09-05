/*
 * tcp.h - tcp socket interface
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

#ifndef _TCP_H
#define _TCP_H 

#ifdef __cplusplus
#include <winsock2.h>
extern "C"{ 
#endif

#if !defined(_WINDOWS)

#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#define FAR
#define SOCKADDR_IN struct sockaddr_in
#define SOCKADDR_UN struct sockaddr_un
#define SOCKET int
#define closesocket(x) close(x)
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR -1
#define SOCKET_EOF 0 

#else

#include <windows.h>
extern int tcp_startup(void);
extern int tcp_cleanup(void);

#include "iov_t.h"

#endif

/* prototypes */
extern SOCKET tcp_socket(int family);
extern int tcp_connect(SOCKET sock, int family, char *lhost, char *rhost,
	unsigned long lip, unsigned long rip, int port);
extern SOCKET tcp_accept(SOCKET sock, SOCKADDR_IN *addr);
extern int tcp_bind(SOCKET sock, int family, char *hostname, unsigned long ip, int port);
extern int tcp_listen(SOCKET sock, int qsize);
extern int tcp_disconnect(SOCKET sock);
extern int tcp_disconnect(SOCKET sock);
extern int tcp_send(SOCKET sock, const char *buf, int len);
extern int tcp_send_vector(SOCKET sock, struct iovec *iov, int iovcnt);
extern int tcp_sendto(SOCKET sock, SOCKADDR* addr, const char *buf, int len);
extern int tcp_recv(SOCKET sock, char *buf, int len);
extern int tcp_recvfrom(SOCKET sock, SOCKADDR* addr, const char *buf, int len);
extern int tcp_setsockopt(SOCKET sock, int level, int optname, const char FAR *optval, int optlen);
extern int tcp_getsockopt(SOCKET sock, int level, int optname, const char FAR *optval, int *optlen);
extern int tcp_check_connect(SOCKET sock);
extern int tcp_check_recv(SOCKET sock, int timeo);
extern int tcp_check_send(SOCKET sock, int timeo);
extern int tcp_set_nonb(SOCKET sock);
extern int tcp_set_b(SOCKET sock);
extern unsigned long tcp_get_numbytes(SOCKET sock);

#ifdef __cplusplus 
}
#endif

#endif

