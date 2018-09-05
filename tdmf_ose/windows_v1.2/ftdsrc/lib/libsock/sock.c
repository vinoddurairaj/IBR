/*
 * sock.c - generic socket interface
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

#if defined(_WINDOWS)
#include <winsock2.h>
#include <errno.h>
#endif

#include <stdio.h> 
#include "iputil.h"
#include "sock.h"

#if defined(_WINDOWS)
#include "sockerrnum.h"
#include "sockerr.h"
#endif

#if defined(_WINDOWS)
static int check_ip_addr_from_ipconfig(int ip);
#endif


#if defined(_WINDOWS)

/*
 * sock_errno --
 *	This routine converts a WinSock error into an errno value.
 *
 * Results:
 *	Returns the errno.
 */
#ifdef TDMF_TRACE
int  sock_errno()
{
	int errCode2,errCode = WSAGetLastError();

	if ((errCode >= WSAEINTR) && (errCode <= WSAEREMOTE)) 
		{
	 	errCode2 = wsaErrorTable[errCode - WSAEINTR];
		return errCode;
		}
	else
		return errCode;
}
#else
int  sock_errno()
{
	int errCode = WSAGetLastError();

	if ((errCode >= WSAEINTR) && (errCode <= WSAEREMOTE)) 
		{
	 	errCode = wsaErrorTable[errCode - WSAEINTR];
		}	 
	else 
		{
		errCode = EINVAL;
		}	
	return errCode;
}
#endif

char *sock_strerror(int errnum)
{
	#ifdef TDMF_TRACE
	if ((errnum >= WSAEINTR) && (errnum <= WSAEREMOTE)) 
		return sock_errstr[errnum - WSAEINTR];
	else if (errnum && errnum <= ELOOP)
	  	return sock_errstr[errnum];
	else
		return sock_errstr[EINVAL];
	#else
	return sock_errstr[errnum];
	#endif
}

extern unsigned long gethostid(void);

int
sock_startup(void)
{
	return tcp_startup();
}

int
sock_cleanup(void)
{
	return tcp_cleanup();
}
#else

int
sock_errno(void)
{

	return errno;
}

char *
sock_strerror(int errnum)
{
	return (char*)strerror(errnum);
}

#endif

/*
 * sock_create -- create a socket object
 */
sock_t *
sock_create(void) 
{
	sock_t *sockp;

	if ((sockp = (sock_t*)calloc(1, sizeof(sock_t))) == NULL) {
		return NULL;
	}

	sockp->sock = (SOCKET)-1;

#if defined(_WINDOWS)
	sockp->bBlocking = TRUE;
#endif

	return sockp;
}

/*
 * sock_delete -- destroy a socket object
 */
int
sock_delete(sock_t **sockpp) 
{
	sock_t	*sockp;

	if (sockpp == NULL || *sockpp == NULL) {
		return 0;
	}
	
	sockp = *sockpp;

	if (GET_SOCK_CONNECT(sockp->flags)) {
		// disconnect before destroying 
		sock_disconnect(sockp);
	}

	free(sockp);

	*sockpp = NULL;

	return 0;
}

/*
 * sock_init -- initialize the socket object
 */
int
sock_init
(sock_t *sockp, char *lhostname, char *rhostname,
	unsigned long lip, unsigned long rip, int type, int family,
	int create, int verifylocal)
{
	int rc;

	if (lhostname && strlen(lhostname)) {
		strcpy(sockp->lhostname, lhostname);
	}
	if (rhostname && strlen(rhostname)) {
	    strcpy(sockp->rhostname, rhostname);
	}
		
	sockp->lip = lip;
	sockp->rip = rip;
	sockp->type = type;
	sockp->family = family;
	sockp->lhostid = gethostid();

	// fill in any unspecified fields
	if (lip > 0) {
		if (verifylocal) {
			if ((rc = sock_is_me(sockp, 1)) == 0) {
				return -2;
			} else if (rc < 0) {
				return -1;
			}
		}
		
		if (lhostname == NULL || strlen(lhostname) == 0) {
			ip_to_name(lip, sockp->lhostname);	
		}
	
	} else {
		if (lhostname && strlen(lhostname)) {
			if (name_is_ipstring(lhostname)) {
				ipstring_to_ip(lhostname, &sockp->lip);
			} else {
				name_to_ip(lhostname, &sockp->lip);
			}
			if (verifylocal) {
				if ((rc = sock_is_me(sockp, 1)) == 0) {
					return -2;
				} else if (rc < 0) {
					return -1;
				}
			}
		}
	}

	if (rip > 0) {
		if (rhostname == NULL || strlen(rhostname) == 0) {
			ip_to_name(rip, sockp->rhostname);	
		}
	} else {
		if (rhostname && strlen(rhostname)) {
			if (name_is_ipstring(rhostname)) {
				ipstring_to_ip(rhostname, &sockp->rip);
			} else {
				name_to_ip(rhostname, &sockp->rip);
			}
		}
	}

	if (create) {
		if ((rc = tcp_socket(family)) < 0) {
			return rc;
		}
		sockp->sock = rc;
#if defined(_WINDOWS)
		sockp->bBlocking = TRUE;
#endif
	}

	return 0;
}

/*
 * sock_socket -- create the socket and return a handle to it
 */
SOCKET
sock_socket(int family)
{

	return tcp_socket(family);
}

/*
 * sock_bind -- bind sockp with its address
 */
int
sock_bind(sock_t *sockp, int port)
{
	int rc;

	rc = tcp_bind(sockp->sock, sockp->family, sockp->lhostname, sockp->lip, 0);

	return rc;
}

/*
 * sock_connect -- connect sockp with its peer 
 */
int
sock_connect(sock_t *sockp, int port)
{
	int rc;

	if (sockp == NULL) {
		return 0;
	}

	if (GET_SOCK_CONNECT(sockp->flags)) {
		// already connected 
		return -1;
	}

	switch(sockp->type) {
	case SOCK_STREAM:
		rc = tcp_connect(sockp->sock, sockp->family, sockp->lhostname,
			sockp->rhostname, sockp->lip, sockp->rip, port);
		if (rc < 0) {
			return rc;
		}
		
		sockp->port = port;
		SET_SOCK_CONNECT(sockp->flags);

		return rc;
	case SOCK_DGRAM:
	default:
		return -2;
	}

}

/*
 * sock_connect_nonb -- connect w/timeout sockp with its peer 
 */
int
sock_connect_nonb(sock_t *sockp,
	int port, int sec, int usec, int *sockerr)
{
	fd_set			rset, wset;
	struct timeval	tval;
	int				rc, ret = 0, errnum;

#if defined(_AIX)
	size_t			len; 
#endif /* defined(_AIX) */
	unsigned long	flags = 0;
	
	// save socket attributes before changing
#if !defined(_WINDOWS)
	fcntl(sockp->sock, F_GETFL, &flags);
#else
	int	bBlocking = sockp->bBlocking;
	
	if ((ret = sock_set_nonb(sockp)) < 0) {
		goto done;
	}
#endif
	
	sockp->port = port;

	rc = sock_connect(sockp, port);

	if (rc < 0) {
		*sockerr = errnum = sock_errno();
#if defined(_WINDOWS)
		if (errnum != EWOULDBLOCK
			&& errnum != EALREADY)
			// Winsock returns EALREADY on subsequent 
			// connect calls instead of EWOULDBLOCK
			// on non-blocking sockets
		{
#else
		if (errnum != EINPROGRESS) {
#endif
			ret = -1;
			goto done;
		}
	} else if (rc == 0) {
		goto done;	// connect completed immediately 
	}

	FD_ZERO(&wset);
	FD_SET(sockp->sock, &wset);

	tval.tv_sec = sec;
	tval.tv_usec = usec;

	rset = wset;

#if defined(_WINDOWS)
	// Winsock - connection established if socket is writable only
	rc = select(sockp->sock+1, NULL, &wset, NULL, (sec || usec) ? &tval : NULL);
#else
	// Berkley - connection established if socket is readable or writable
	rc = select(sockp->sock+1, &rset, &wset, NULL, (sec || usec) ? &tval : NULL);
#endif

	if (rc == 0) {
		*sockerr = ETIMEDOUT; // ETIMEDOUT 
		ret = -1;
		goto done;
	}

	if (FD_ISSET(sockp->sock, &rset) || FD_ISSET(sockp->sock, &wset)) {
#if !defined(_WINDOWS)
		{
			char	error[256];
			int		len = sizeof(error);

			if ((rc = sock_get_opt(sockp, SOL_SOCKET, SO_ERROR, error, &len)) < 0) {
				ret = rc;
				goto done;
			}
		}
#endif
	} else {
		ret = -1;
		goto done;
	}

done:

#if defined(_WINDOWS)
	if (bBlocking) {
		if ((rc = sock_set_b(sockp)) < 0) {
			ret = rc;
		}
	}
#else
	fcntl(sockp->sock, F_SETFL, &flags);
#endif

	if (ret < 0) {
		// close socket
		sock_disconnect(sockp);
#if defined(_WINDOWS)
		if (sockp->hEvent) {
			CloseHandle(sockp->hEvent);
		}
#endif		
	}

	return ret;
}

/*
 * sock_disconnect -- disconnect sockp object from its peer
 */
int
sock_disconnect(sock_t *sockp)
{
	int rc;

	switch(sockp->type) {
	case SOCK_STREAM:
		rc = tcp_disconnect(sockp->sock);
	case SOCK_DGRAM:
	default:
		rc = -1;
	}

	UNSET_SOCK_CONNECT(sockp->flags);
	sockp->sock = (SOCKET)-1;

	return rc;
}

/*
 * sock_recv -- read a message from peer
 */
int
sock_recv(sock_t *sockp, char *buf, int len)
{
	int rc;

	rc = tcp_recv(sockp->sock, buf, len);

	return rc;
}
    
/*
 * sock_send -- send a message to peer  
 */
int
sock_send(sock_t *sockp, char *buf, int len)
{
	int rc;

	rc = tcp_send(sockp->sock, buf, len);

	return rc;
}

/*
 * sock_send_vector -- send vector to peer
 */
int
sock_send_vector(sock_t *sockp, struct iovec *iov, int iovcnt)
{
	return tcp_send_vector(sockp->sock, iov, iovcnt);
}

/*
 * sock_check_connect -- are we connected
 */
int
sock_check_connect(sock_t *sockp)
{

	switch(sockp->type) {
	default:
		return tcp_check_connect(sockp->sock);
	}

}

/*
 * sock_check_recv -- would net recv block ?
 */
int
sock_check_recv(sock_t *sockp, int timeo)
{

	if (sockp->sock <= 0) {
		return 0;
	}

	switch(sockp->type) {
	default:
		return tcp_check_recv(sockp->sock, timeo);
	}

}

/*
 * sock_check_send -- would send block ?
 */
int
sock_check_send(sock_t *sockp, int timeo)
{

	if (sockp->sock <= 0) {
		return 0;
	}

	switch(sockp->type) {
	default:
		return tcp_check_send(sockp->sock, timeo);
	}

}

/*
 * sock_listen -- set sock object to listen
 */
int
sock_listen(sock_t *sockp, int port)
{
	int	rc, n;

	n = 1;
	rc = tcp_setsockopt(sockp->sock, SOL_SOCKET, SO_REUSEADDR,
		(char*)&n, sizeof(int));
	
	if (rc == -1) {
		return -1;
	}
	
	rc = tcp_bind(sockp->sock, sockp->family, sockp->lhostname, sockp->lip, port);
	
	if (rc == -1) {
		return -2;
	}

	rc = tcp_listen(sockp->sock, 5);
	
	if (rc == -1) {
		return -3;
	}
	
	sockp->port = port;

	return 0;
}

/*
 * sock_accept -- accept a connection from peer
 */
int
sock_accept(sock_t *listener, sock_t *sockp)
{

	/* copy listener state to target sock_t */
	memcpy(sockp, listener, sizeof(sock_t));

	if ((sockp->sock = tcp_accept(listener->sock)) < 0) {
		return sockp->sock;
	}
	SET_SOCK_CONNECT(sockp->flags);

	return 0;
}

/*
 * sock_accept_nonb -- accept w/timeout
 */
int
sock_accept_nonb(sock_t *listener, sock_t *sockp, int sec, int usec)
{

	return -1;
}

/*
 * sock_set_opt -- set socket option 
 */
int
sock_set_opt
(sock_t *sockp, int level, int optname, char *optval, int optlen)
{
	return tcp_setsockopt(sockp->sock, level, optname, optval, optlen);
}

/*
 * sock_get_opt -- get socket option 
 */
int
sock_get_opt
(sock_t *sockp, int level, int optname, char *optval, int *optlen)
{
	return tcp_getsockopt(sockp->sock, level, optname, optval, optlen);
}

/*
 * sock_set_nonb -- set socket to non-blocking
 */
int
sock_set_nonb(sock_t *sockp)
{
	int	rc;

#if defined(_WINDOWS)
	if (WSAEventSelect(sockp->sock, sockp->hEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR) {
		rc = -1;
	} else {
		rc = 0;
	}
	
	sockp->bBlocking = FALSE;
#endif

	return tcp_set_nonb(sockp->sock);
}

/*
 * sock_set_b -- set socket to blocking
 */
int
sock_set_b(sock_t *sockp)
{
	int	rc;

#if defined(_WINDOWS)
	if (WSAEventSelect(sockp->sock, NULL, 0) == SOCKET_ERROR) {
		rc = -1;
	} else {
		rc = 0;
	}
	
	sockp->bBlocking = TRUE;
#endif
	
	return tcp_set_b(sockp->sock);
}

/*
 * sock_gethostbyname --
 * return hostent struct containing hostname with truncated domain name 
 */
struct hostent *
sock_gethostbyname(char *hostname)
{
	struct hostent	*hp;

	if ((hp = gethostbyname(hostname)) == NULL) {
		return NULL;
	}

	hp->h_name = (char*)strtok(hp->h_name, ".");

	return hp;
}

/*
 * sock_is_me - figures out if the target IP address contained in the
 *              sock_t object is this machine
 *        1 = the ip addr pertains to this machine
 *        0 = the ip addr does not pertain to this machine
 *       -1 = invalid IP addr
 */
int
sock_is_me(sock_t *sockp, int local)
{
	u_long			*ips = NULL, ip;
	int				ipcount, ipretval;
    
	ipretval = -1;

	ipcount = 0; 
	ipcount = getnetconfcount();
    
	if (ipcount > 0) {
		ips = (u_long*)malloc(ipcount*sizeof(u_long));
		(void)getnetconfs(ips);
	}

	if (local) {
		ip = sockp->lip;
	} else {
		ip = sockp->rip;
	}

	if (ip <= 0) {
		return ipretval;
	}
	
	if (ip == LOCALHOSTIP) {
		return 1;
	}

	{
		int i = 0;
		
		ipretval = 0;

		while(ips[i]) {
			if (ips[i] == ip) {
				ipretval = 1;
				break;
			}
			i++;
		}
	}

#if defined(_WINDOWS)
    if ( ipretval == 0 ) {
        //returns 1 if ip is found in the list output by ipconfig.exe
        ipretval = check_ip_addr_from_ipconfig(ip);
    }
#endif

/*
	// check out the ip number
	if (ip > 0L) {
		hp = gethostbyaddr((char*)&ip, sizeof(ip), AF_INET);
		if (hp == NULL) {
			if (ips)
				free(ips);
            
			return -2;
		} else {
			ipretval = 0;
			for (j=0; j<ipcount; j++) {
				if (ips[j] == ip) {
					ipretval = 1;
					break;
				}
			}
		}
	}
*/
	free(ips);
    
	return ipretval;
}

/*
 * sock_ipstring_to_ip -- convert ipstring to ip address
 */
int
sock_ipstring_to_ip(char *ipstring, unsigned long *ip) {
	
	ipstring_to_ip(ipstring, ip);
	
	return 0;
}

/*
 * sock_name_to_ip -- convert hostname to ip address
 */
int
sock_name_to_ip(char *name, unsigned long *ip) {
	
	name_to_ip(name, ip);
	
	return 0;
}

/*
 * sock_ip_to_ipstring -- convert ip to ipstring in dot notation
 */
int
sock_ip_to_ipstring(unsigned long ip, char *ipstring) {
	
	ip_to_ipstring(ip, ipstring);
	
	return 0;
}

/*
 * sock_ip_to_name -- convert ip to hostname
 */
int
sock_ip_to_name(unsigned long ip, char *name) {
	
	ip_to_name(ip, name);
	
	return 0;
}

/*
 * sock_get_local_ip_list -- get all IP addresses for all available local NICs
 */
int
sock_get_local_ip_list(unsigned long *iplist) {
	
	getnetconfs(iplist);
	
	return 0;
}


/*
 * The section below uses the Windows ipconfig.exe tool to get 
 * the list of IP addresses in use by the host.
 * check_ip_addr_from_ipconfig() is the new function added within sock_is_me()
 */
#if defined(_WINDOWS)
///////////////////////////////////////////////////////////////////////////////
//begin
///////////////////////////////////////////////////////////////////////////////
typedef struct __ConsoleOutputCtrl
{
    HANDLE          hOutputRead,    //provide this handle to child process
                    hOutputWrite,   //provide this handle to child process
                    hErrorWrite;    //provide this handle to child process
    char            *pData;
    unsigned int    iSize, 
                    iTotalDataRead;

} ConsoleOutputCtrl;

static BOOL consoleOutput_init(ConsoleOutputCtrl *ctrl)
{
    HANDLE hOutputReadTmp;
    SECURITY_ATTRIBUTES sa;
    // Set up the security attributes struct.
    sa.nLength= sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    ctrl->iSize = 1024;
    ctrl->pData = (char*)malloc( ctrl->iSize );
    ctrl->iTotalDataRead = 0;

    // Create the child process output pipe.
    if (!CreatePipe(&hOutputReadTmp,&ctrl->hOutputWrite,&sa,0))
    {
        //assert(0);
        return FALSE;
    }
    // Create a duplicate of the output write handle for the std error
    // write handle. This is necessary in case the child application
    // closes one of its std output handles.
    if (!DuplicateHandle(GetCurrentProcess(), ctrl->hOutputWrite,
                         GetCurrentProcess(),&ctrl->hErrorWrite,
                         0,TRUE,
                         DUPLICATE_SAME_ACCESS))
    {
        //assert(0);
        return FALSE;
    }

    // Create new output read handle and the input write handles. Set
    // the Properties to FALSE. Otherwise, the child inherits the
    // properties and, as a result, non-closeable handles to the pipes
    // are created.
    if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
                         GetCurrentProcess(),&ctrl->hOutputRead, // Address of new handle.
                         0,FALSE, // Make it uninheritable.
                         DUPLICATE_SAME_ACCESS))
    {
        //assert(0);
        return FALSE;
    }

    // Close inheritable copies of the handles you do not want to be
    // inherited.
    if (!CloseHandle(hOutputReadTmp)) 
    {
        //assert(0);
        return FALSE;
    }

    return TRUE;
}

static BOOL consoleOutput_read(ConsoleOutputCtrl *ctrl)
{
    DWORD read = 0;

    if ( ctrl->pData == 0 )
    {
        //assert(0);
        return FALSE;
    }
    // Close pipe handles (do not continue to modify the parent).
    // You need to make sure that no handles to the write end of the
    // output pipe are maintained in this process or else the pipe will
    // not close when the child process exits and the ReadFile will hang.
    if (!CloseHandle(ctrl->hOutputWrite)) 
    {
        //assert(0);
        return FALSE;
    }
    if (!CloseHandle(ctrl->hErrorWrite)) 
    {
        //assert(0);
        return FALSE;
    }

    read = 0;
    ctrl->iTotalDataRead = 0;
    while ( ReadFile(ctrl->hOutputRead, 
                     ctrl->pData + ctrl->iTotalDataRead, 
                     ctrl->iSize - ctrl->iTotalDataRead, 
                     &read,0) )
    {
        ctrl->iTotalDataRead += read;
        if ( ctrl->iTotalDataRead == ctrl->iSize )
        {   //enlarge buffer by 1KB and continue reading from child output stream Pipe
            char *pNewData = (char*)malloc( ctrl->iSize + 1024 );
            memmove(pNewData, ctrl->pData, ctrl->iSize);
            ctrl->iSize += 1024;
            free( ctrl->pData );
            ctrl->pData = pNewData;
        }
    }
    //No more data received from process.

    //because console output data is text-based, append '\0' at end of data received
    if ( ctrl->iTotalDataRead == ctrl->iSize )
    {   //enlarge buffer by 1 byte to ensure EOS
        char *pNewData = (char*)malloc( ctrl->iSize + 1 );
        memmove(pNewData, ctrl->pData, ctrl->iSize);
        ctrl->iSize += 1;
        free( ctrl->pData );
        ctrl->pData = pNewData;
    }
    ctrl->pData[ ctrl->iTotalDataRead ] = 0;//ensure end of text string
    ctrl->iTotalDataRead++;

    return TRUE;
}

static void consoleOutput_delete(ConsoleOutputCtrl *ctrl)
{
    if (!CloseHandle(ctrl->hOutputRead)) 
    {
        //assert(0);
    }
  
    free( ctrl->pData );
    ctrl->iSize = 0;
    ctrl->pData = 0;
    ctrl->iTotalDataRead = 0;
}

// pCur must be a zero-terminated string.
static char * 
parse_to_find_ip_addr(char* pCur)
{
    // ' ipconfig /all '  outputs text information , 
    // in which "IP Address" can be found, as many times as the host as IP Addresses.
    //
    // looking for :  "IP Address.....................: aaa.bbb.c.dd "
    //
    pCur = strstr( pCur, "IP Address" );
    if ( pCur == NULL )
        return NULL;

    pCur = strchr( pCur, ':' );
    if ( pCur == NULL )
        return NULL;

    while( !isdigit(*pCur) )
        pCur++;

#ifdef _DEBUG
    if ( pCur )
        printf("\ndebug: parse_to_find_ip_addr returns %.15s ",pCur);
    else
        printf("\ndebug: parse_to_find_ip_addr returns NULL.");
#endif

    //ptr to text IP address.
    return pCur;
}

static int
check_ip_addr_from_ipconfig(int iptofind)
{
    int                     ret = 0;
    BOOL                    b;
    PROCESS_INFORMATION     pInfo;
    STARTUPINFO             sInfo;
    ConsoleOutputCtrl       conctrl;

    consoleOutput_init(&conctrl);

    //launch process command 
    memset(&sInfo,0,sizeof(sInfo));
    sInfo.cb = sizeof(sInfo);

    sInfo.dwFlags    = STARTF_USESTDHANDLES;
    sInfo.hStdError  = conctrl.hErrorWrite;//GetStdHandle(STD_ERROR_HANDLE);
    sInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    sInfo.hStdOutput = conctrl.hOutputWrite;//GetStdHandle(STD_OUTPUT_HANDLE);

    b = CreateProcess(  NULL,       // name of executable module
                        "ipconfig.exe /all ",  // command line string
                        NULL,                 // SD
                        NULL,                 // SD
                        TRUE,                 // handle inheritance option
                        CREATE_NO_WINDOW,     // creation flags
                        NULL,                 // new environment block
                        NULL,                 // current directory name
                        &sInfo,               // startup information
                        &pInfo                // process information
                        );//ASSERT(b);
    if ( b ) {
        char *pCur;
        int  i,ipfound;
        #define MAX_IP_PER_ADAPTER  8
        int  iplist[MAX_IP_PER_ADAPTER];

        //accumulate all console output into pData buffer
        //while waiting for cmd process to end ...
        consoleOutput_read(&conctrl);

        CloseHandle(pInfo.hProcess);
        CloseHandle(pInfo.hThread);

        ipfound = 0;
        pCur = conctrl.pData;//conctrl.pData is a zero-terminated string
        while( (pCur = parse_to_find_ip_addr(pCur)) != NULL     && 
               ipfound < MAX_IP_PER_ADAPTER                     )
            ipstring_to_ip( pCur, (unsigned long*)&iplist[ ipfound++ ] );

        for(i=0; i<ipfound && i<MAX_IP_PER_ADAPTER; i++) {
            if ( iptofind == iplist[ i ] )
                ret = 1;//found it 
        }

    }
    else
    {
        //debug: DWORD gle = GetLastError();
    }

    consoleOutput_delete(&conctrl);

    return ret;
}

//end
///////////////////////////////////////////////////////////////////////////////
#endif
