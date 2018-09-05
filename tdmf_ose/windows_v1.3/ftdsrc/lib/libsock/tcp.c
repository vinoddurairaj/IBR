/*
 * tcp.c - tcp socket interface
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

#if defined (_WINDOWS)
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#include "iputil.h"
#include "tcp.h"

static int tcp_check(SOCKET sock, int timeo, int type);

#if defined (_WINDOWS) 
/*
 * tcp_startup
 */
int
tcp_startup(void)
{
	WSADATA Data;
	int status;

	/* initialize the Windows Socket DLL */
	status=WSAStartup(MAKEWORD(2, 2), &Data);
	if (status != 0)
		return -1;

	return 0;
}

/*
 * tcp_cleanup
 */
int
tcp_cleanup(void)
{
	/* cleanup the Windows Socket DLL */
	int status = WSACleanup();
	if (status == SOCKET_ERROR)
		return -1;
 
	return 0;
}

#else

#define FAR 

int
tcp_errno()
{
	return errno;
}

#endif

SOCKET
tcp_socket(int family)
{
	SOCKET	sock;
	int		timeout = 60000;

	if ((sock = socket(family, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		return (SOCKET)-1;
	}

	return sock;
}

/*
 * tcp_bind -- bind given protocol family/ip/port to socket 
 */
int
tcp_bind(SOCKET sock, int family, char *host, unsigned long ip, int port)
{
	SOCKADDR_IN addr; 
#if !defined(_WINDOWS)
	SOCKADDR_UN addr1; 
#endif
	u_long	lip;
	int	len, rc;
	
	u_short sport = port;

	switch(family) {
	case AF_INET:
		if (ip > 0) {
			lip = ip;
		} else {
			if (host == NULL || strlen(host) == 0) {
				lip = INADDR_ANY; // any NIC 
			} else {
				if (name_is_ipstring(host)) {
					rc = ipstring_to_ip(host, &lip);
				} else {
					rc = name_to_ip(host, &lip);
				}
				if (rc < 0) {
					return rc;
				}     
			}
		}
		memset(&addr, 0, sizeof(SOCKADDR_IN));
		
		addr.sin_family = family;
		memcpy(&addr.sin_addr, &lip, sizeof(addr.sin_addr));
		
		addr.sin_port = sport;
		len = sizeof(SOCKADDR_IN);

		rc = bind(sock, (struct sockaddr*)&addr, len);
		
		if (rc == SOCKET_ERROR) {
			return -1;
		}
		
		break;
#if !defined(_WINDOWS)
	case AF_UNIX:
		unlink(host); /* get rid of pipe file and re-create */
		
		memset(&addr1, 0, sizeof(SOCKADDR_UN));
		
		addr1.sun_family = family;
		
		strcpy(addr1.sun_path, host);

#if defined(_AIX)
		len = SUN_LEN((SOCKADDR_UN*) &addr1);
#else  /* defined(_AIX) */
		len = strlen(host) + sizeof(addr1.sun_family);
#endif /* defined(_AIX) */
		
		rc = bind(sock, (struct sockaddr*)&addr1, len);
		
		if (rc == SOCKET_ERROR) {
			return -1;
		}
		
		break;
#endif
	default:
		return -1;
	}

	return 0;
}

/*
 * tcp_connect -- connect a stream socket with a remote server 
 */
int
tcp_connect
(SOCKET sock, int family, char *lhost, char *rhost,
	unsigned long lip, unsigned long rip, int port)
{
	SOCKADDR_IN sinaddr;
#if !defined(_WINDOWS)
	SOCKADDR_UN	sunaddr;
#endif
	u_long		ip; 
	u_short		sport = port;
	int			len, rc,  n;

	switch(family) {
	case AF_INET:
		memset((char*)&sinaddr, 0, sizeof(SOCKADDR_IN));
		
		sinaddr.sin_family = family;
		sinaddr.sin_port = sport;

		if (rip <= 0) {
			if (name_is_ipstring(rhost)) {
				rc = ipstring_to_ip(rhost, &ip);
			} else {
				rc = name_to_ip(rhost, &ip);
			}
		} else {
			ip = rip;
		}
		
		memcpy(&sinaddr.sin_addr, &ip, sizeof(sinaddr.sin_addr));
		len = sizeof(SOCKADDR_IN);
		
		rc = connect(sock, (struct sockaddr*)&sinaddr, len);

		break;
#if !defined(_WINDOWS)
	case AF_UNIX:
		memset(&sunaddr, 0, sizeof(SOCKADDR_UN));
		
		sunaddr.sun_family = family;
		strcpy(sunaddr.sun_path, rhost);

#if defined(_AIX)
		len = SUN_LEN((SOCKADDR_UN*) &sunaddr);
#else  /* defined(_AIX) */
		len = strlen(rhost) + sizeof(sunaddr.sun_family);
#endif /* defined(_AIX) */
		
		rc = connect(sock, (struct sockaddr*)&sunaddr, len);
		
		break;
#endif
	default:
		return -1;
	}

	if (rc == SOCKET_ERROR) {
		return -1;
	}
	n = 1;

	rc = tcp_setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
		(char*)&n, sizeof(int));

	if (rc != 0) {
		return -1;
	}

	return 0;
}

/*
 * tcp_listen -- listen for connections on given port
 */
int
tcp_listen(SOCKET sock, int qsize)
{
	if (listen(sock, qsize) == SOCKET_ERROR) {
		return -1;
	}

	return 0;
}

/*
 * tcp_accept -- accepts a stream socket coonection from peer 
 */
SOCKET
tcp_accept(SOCKET sock, SOCKADDR_IN *addr)
{
	SOCKET rsock;
	//SOCKADDR_IN addr;
#if defined(_AIX)
	u_long n;
#else
	int n;
#endif

#if defined(_AIX)
	n = SUN_LEN((SOCKADDR_UN*) addr);
#else
	n = sizeof(SOCKADDR_IN);
#endif

	rsock = accept(sock, (struct sockaddr*)addr, &n);
	if (rsock == INVALID_SOCKET) {
		return (SOCKET)-1;
	}

	return rsock;
}

/*
 * tcp_set_nonb - Set the socket as non-blocking 
 */
int tcp_set_nonb(SOCKET sock)
{

#if defined (_WINDOWS)
	int rc;
	int non_block = 1;

	rc = ioctlsocket(sock, FIONBIO, &non_block);
	
	if (rc == SOCKET_ERROR) {
		closesocket(sock);
		return -1;
	}
#else
	int val;

	val = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, val | O_NONBLOCK);

#endif

	return 0;
}

/*
 * tcp_set_b - Set the socket as non-blocking 
 */
int tcp_set_b(SOCKET sock)
{

#if defined (_WINDOWS)
	int	rc, non_block = 0;

	rc = ioctlsocket(sock, FIONBIO, &non_block);
	
	if (rc == SOCKET_ERROR) {
		closesocket(sock);
		return -1;
	}
#else
	int	val;

	val = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, val & ~O_NONBLOCK);

#endif

	return 0;
}

/*
 * tcp_disconnect -- close connection between client/server 
 */
int 
tcp_disconnect(SOCKET sock)
{

	if (sock != (SOCKET)-1) {
		closesocket(sock);
	}

	return 0;
}

/*
 * tcp_send -- send a msg to the net
 */

int
tcp_send(SOCKET sock, const char *buf, int len)
{
#if !defined (_WINDOWS)
	return write(sock, buf, len);
#else
	return send(sock, buf, len, 0);
#endif
}

/*
 * tcp_sendto -- send a msg to the net
 */

int
tcp_sendto(SOCKET sock, SOCKADDR* addr, const char *buf, int len)
{
#if !defined (_WINDOWS)
#error	
#else
	return sendto(sock, buf, len, 0, addr, sizeof(SOCKADDR));
#endif
}

/*
 * tcp_send_vector -- send a msg to the net
 */
int
tcp_send_vector(SOCKET sock, struct iovec *iov, int iovcnt)
{
#if !defined(_WINDOWS)
	return writev(sock, iov, iovcnt);
#else
	int i, totalcnt = 0;

	for (i = 0; i< iovcnt; i++)
	{
		int writecnt = tcp_send(sock, iov[i].iov_base, iov[i].iov_len);
		if (writecnt == -1)
			return -1;

		totalcnt += writecnt;
		if (writecnt != iov[i].iov_len)
			break;

		if ( !tcp_check_send(sock, 1000000) )
			break;
	}

	return totalcnt;
/*
	int	rc, wlen, i;

	for (wlen = 0, i = 0; i < iovcnt; i++) {
		if ((rc = tcp_send(sock, iov[i].iov_base, iov[i].iov_len)) <= 0) {
			return rc;
		} else {
//printf("\n$$$ tcp_send rc = %d\n",rc);
			wlen += rc;
		}
		if (rc < iov[i].iov_len) {
			break;
		}
	}
	return wlen;
*/
#endif
}

/*
 * tcp_recv -- read a msg from the net
 */
int
tcp_recv(SOCKET sock, char *buf, int len)
{
#if !defined (_WINDOWS)
	return read(sock, buf, len);
#else
	return recv(sock, buf, len, 0);
#endif
}
   

/*
 * tcp_recvfrom -- read a UDP msg from the net
 */
int
tcp_recvfrom(SOCKET sock, SOCKADDR* addr, const char *buf, int len)
{
    int size = sizeof(SOCKADDR);
#if !defined (_WINDOWS)
#error	
#else
	return recvfrom(sock, (char*)buf, len, 0, addr, &size);
#endif
}


/*
 * tcp_check_connect -- is connection up ?
 */
int
tcp_check_connect(SOCKET sock)
{
	return 1;
}

/*
 * tcp_check_recv --
 * would connection block ?
 */
static int
tcp_check(SOCKET sock, int timeo, int type)
{
	struct timeval	timeout;
	fd_set			fds, *readset, *writeset;
	int				rc;

	timeout.tv_sec = 0L;
	timeout.tv_usec = timeo;

	FD_ZERO(&fds);
	FD_SET(sock, &fds);

	if (type == 0) {
		// it's a read check
		readset = &fds;
		writeset = NULL;
	} else {
		// it's a write check
		readset = NULL;
		writeset = &fds;
	}

	if (timeo < 0) {
		// see if there is something waiting to be read
		// wait forever
		rc = select(sock+1, readset, writeset, NULL, NULL);
	} else {
		// see if there is something waiting to be read
		// timeout
		rc = select(sock+1, readset, writeset, NULL, &timeout);
	}

	if (rc < 0) {
		return rc;
	} else if (!FD_ISSET(sock, &fds)) {
		return 0;
	}

	return 1;
}

/*
 * tcp_check_recv --
 * would connection read block ?
 */
int
tcp_check_recv(SOCKET sock, int timeo)
{
	
	return tcp_check(sock, timeo, 0);
}

/*
 * tcp_check_send --
 * would connection write block ?
 */
int
tcp_check_send(SOCKET sock, int timeo)
{
	
	return tcp_check(sock, timeo, 1);
}

int 
tcp_setsockopt
(SOCKET sock, int level, int optname, const char *optval, int optlen)
{
	int rc = setsockopt (sock, level, optname, optval, optlen);
	if (rc == SOCKET_ERROR) {
		return -1;
	}

	return 0;
}

int 
tcp_getsockopt
(SOCKET sock, int level, int optname, const char *optval, int *optlen)
{
	int rc = getsockopt(sock, level, optname, (char*)optval, optlen);
	if (rc == SOCKET_ERROR) {
		return -1;
	}

	return 0;
}

unsigned long 
tcp_get_numbytes(SOCKET sock)
{
	int	rc;
	unsigned long	numbytes;

#if defined (_WINDOWS)
	rc = ioctlsocket(sock, FIONREAD, &numbytes);
	if (rc == SOCKET_ERROR) {
		closesocket(sock);
		return -1;
	}
#else

#endif
	return numbytes;
}
