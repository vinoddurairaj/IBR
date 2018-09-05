/* #ident "@(#)$Id: Agent_sock.c,v 1.17 2003/11/13 02:48:20 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
#include <stdio.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <sys/ioctl.h>
#if defined(SOLARIS)		/* for HP */
#include <sys/sockio.h>
#elif defined(_AIX)
#include <sys/un.h>
#include <net/if.h>
#endif
#include <net/if_arp.h>
#include <sys/utsname.h>
#include "tdmfAgent.h"
#include "tdmfAgent_trace.h"

#define SOCK_CONNECT               0x0001
#define GET_SOCK_CONNECT(x)        ((x) & SOCK_CONNECT)
#define UNSET_SOCK_CONNECT(x)      ((x) &= 0xfffe)

void abort_Agn();
int     dbg;
FILE	*log;
int  Debug;
int RequestID;
int Alarm;

extern char	logmsg[];
extern char	gszServerUID[];

/*
 * ftd_sock_recv --
 */
int ftd_sock_recv(sock_t *sockt, /* socket descriptor */
            char *readpacket,    /* packet buffer for read() */
            int  len)            /* data length area */
{
        char    *dataptr;        /* carrent offset already read */
        int     datalen;         /* carrent offset already read  */
        int     rcode;           /* return code */
        int     rlimit;          /* read length delemiter */
	int     rlength;
        rcode = 0;
        datalen = 0;
        dataptr = readpacket;
        errno = 0;
	Alarm = 300;

        sprintf(logmsg, "start. len = %d addr = %d\n", len,readpacket);
        logout(17,F_ftd_sock_recv,logmsg);

        /* read packet area clear */
        memset(readpacket,0x00,len);

        /* timer set */
        signal(SIGALRM, abort_Agn);

        /* data length read */
        while( datalen < len){
                alarm(Alarm);
                if((rlength = read(sockt->sockID,dataptr,len)) < 0
 ){
                        /*packet length data read error */
	                sprintf(logmsg,"read error. len = %d, rlen = %d, errno = %d.\n",len,rlength,errno);
	                logout(4,F_ftd_sock_recv,logmsg);
                        rcode = -1;
                        goto proc_end;
                }
                datalen += rlength ;
                dataptr += rlength ;
	        sprintf(logmsg,"read. len = %d, datalen = %d.\n", len,datalen);
	        logout(17,F_ftd_sock_recv,logmsg);
        }

        /* timer reset */
        alarm(0);
        signal(SIGALRM, SIG_DFL);

proc_end:
	logoutx(17,F_ftd_sock_recv,"return","code",rlength);
        return(datalen);
}

int  ftd_sock_send(sock_t *sockt,       /* socket descriptor */
                 char  *writepacket,	/* packet buffer for write() */
                 int len)               /* data length */
{
        int   rcode;                       /* return code */

        rcode = 0 ;
        /*packet write */
        if(( rcode = write(sockt->sockID,writepacket,len)) == -1){
		/* packet write error */
                rcode = -1;
        }
        return(rcode);
}


void abort_Agn()
{
        exit(1);
}

/*
 * sock_create -- create a socket object
 */
sock_t *
sock_create(void)
{
        sock_t *sockp;

        logout(17,F_sock_create,"start\n");

        if ((sockp = (sock_t*)calloc(1, sizeof(sock_t))) == NULL) {
                logoutx(4, F_sock_create, "calloc faied", "errno", errno);
                return NULL;
        }
        sockp->sockID = -1;
        return sockp;
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

        /* fill in any unspecified fields */
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
        	if (sockp->type == SOCK_DGRAM) {
                	if ((rc = socket(family,SOCK_DGRAM,0)) < 0) {
                        	return rc;
                	}
        	} else {
                	if ((rc = tcp_socket(family)) < 0) {
                        	return rc;
                	}
        	}
                sockp->sockID = rc;
        }
        return 0;
}


int tcp_socket(int family)
{
        int  sock;
        int             timeout = 60000;

        if ((sock = socket(family, SOCK_STREAM, 0)) == -1) {
                return -1;
        }
        return sock;
}


#if 0
/*
 * sock_bind_to_port -- bind sockp with its address to a specific port number.
 *                      Note : Binding to a specific port number other than 
 *			       port 0 is discouraged for client applications, 
 *			       since there is a danger of conflicting with 
 *			       another socket already using that port number.
 */
int
sock_bind_to_port(sock_t *sockp, int port)
{
        int rc;  

	sockp->port = port;
        rc = tcp_bind(sockp->sockID, sockp->family, sockp->lhostname, sockp->lip,
port );
        return rc;
}


/*
 * tcp_bind -- bind given protocol family/ip/port to socket
 */
int
tcp_bind(int sock, int family, char *host, unsigned long ip, int port)
{
        struct sockaddr_in addr;
        struct sockaddr_un addr1;
        u_long  lip;
        int     len, rc;

        u_short sport = port;

        switch(family) {
        case AF_INET:
                if (ip > 0) {
                        lip = ip;
                } else {
                        if (host == NULL || strlen(host) == 0) {
                                lip = INADDR_ANY; /* any NIC */
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

                memset(&addr, 0, sizeof(struct sockaddr_in));
                addr.sin_family = family;
                memcpy(&addr.sin_addr, &lip, sizeof(addr.sin_addr));
                addr.sin_port = sport;
                len = sizeof(struct sockaddr_in);

                rc = bind(sock, (struct sockaddr*)&addr, len);
                if (rc != 0) {
                        return -1;
                }
                break;
        case AF_UNIX:
                unlink(host); /* get rid of pipe file and re-create */
                memset(&addr1, 0, sizeof(struct sockaddr_un));
                addr1.sun_family = family;
                strcpy(addr1.sun_path, host);
#if defined(_AIX)
                len = SUN_LEN((SOCKADDR_UN*) &addr1);
#else  /* defined(_AIX) */
                len = strlen(host) + sizeof(addr1.sun_family);
#endif /* defined(_AIX) */
                rc = bind(sock, (struct sockaddr*)&addr1, len);

                if (rc != 0) {
                        return -1;
                }
                break;
        default:
                return -1;
        }
        return 0;
}
#endif

/*
 * sock_delete -- destroy a socket object
 */
int
sock_delete(sock_t **sockpp)
{  
        sock_t  *sockp;

        if (sockpp == NULL || *sockpp == NULL) {
                return 0;
        }
        sockp = *sockpp;
        if (GET_SOCK_CONNECT(sockp->flags)) {
                /* disconnect before destroying */
                sock_disconnect(sockp);
        }
        free(sockp);
        *sockpp = NULL;
        return 0;
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
                rc = tcp_disconnect(sockp->sockID);
        case SOCK_DGRAM:
        default:
                rc = -1;
        }

        UNSET_SOCK_CONNECT(sockp->flags);
        sockp->sockID = -1;
        return rc;
}

/*
 * tcp_disconnect -- close connection between client/server
 */
int
tcp_disconnect(int sock)
{
        if (sock != -1) {
                close(sock);
        }
        return 0;
} 

/*
 * sock_sendtox -- read a UDP message from bound port
 */
int
sock_sendtox(int rip,int rport,char * data,int datalen,int a,int b)
{  
    int rc;  
    int s;  
    int n,len;  
	int level;
    struct sockaddr_in addr;
    char tmpipaddr[16];

    logout(17,F_sock_sendtox,"start\n"); 

    ip_to_ipstring(rip,tmpipaddr);
    sprintf(logmsg,"port = %d IP = %s.\n",rport,tmpipaddr);
    logout(17,F_sock_sendtox,logmsg);

    memset(&addr, 0, sizeof(struct sockaddr_in));
#if defined(linux)
    addr.sin_addr.s_addr = inet_addr(tmpipaddr);
    addr.sin_port = htons(rport);
#else
    addr.sin_addr.s_addr = rip;
    addr.sin_port = rport;
#endif
    addr.sin_family = AF_INET;

    n=1;
    s = socket(AF_INET,SOCK_STREAM, 0);
    logoutx(17,F_sock_sendtox, "socket return", "code", s);
    if (s != 0)
    {
        setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&n,sizeof(int));
        len = sizeof(addr);
        rc = connect_st(s, (struct sockaddr *) &addr,sizeof(addr), &level);
        if (rc >= 0) 
        {
		    if (rc == 1)
                logout(level, F_sock_sendtox, "recovery from connect error.\n");
		    /*  packet write */
            rc = write(s,data,datalen);
		    if (rc != datalen) {
                logoutx(4, F_sock_sendtox, "packet write error", "errno", errno);
		    }
#if 0
            rc = ftd_sock_send(s,data,datalen);              /* do it all */
#endif
        } else {
		    logoutx(level, F_sock_sendtox, "connect error", "errno", errno);
		}
        close(s);
        s = 0;
    }
    else 
    {
	rc = -1;
    }
    return rc;
}

/*
 * sock_sendto -- read a UDP message from bound port
 */
int
sock_sendto(sock_t *sockp, char *buf, int len)
{  
    int rc;  
    struct sockaddr_in addr;
 
    memset(&addr, 0, sizeof(struct sockaddr));
#if defined(linux)
    addr.sin_addr.s_addr = inet_addr(ip_to_ipstring(sockp->rip));
#else
    addr.sin_addr.s_addr = sockp->rip;
#endif

    rc = sendto( sockp->sockID, buf, len,0,(struct sockaddr *)&addr,sizeof(struct sockaddr));
    return rc;
}

/*
 * sock_recvfrom -- read a UDP message from bound port
 */
int
sock_recvfrom(sock_t *sockp, char *buf, int len)
{  
    int rc;  
#if defined(SOLARIS) || defined(HPUX) || defined(linux)
    int length;  
#else
    socklen_t length;
#endif
    struct sockaddr_in addr;
    
    sprintf(logmsg,"start. len = %d\n", len);
    logout(17,F_sock_recvfrom,logmsg);
    memset(&addr, 0, sizeof(struct sockaddr_in));
    length = sizeof(struct sockaddr);

    rc = recvfrom( sockp->sockID, buf, len,0, (struct sockaddr *)&addr ,&length);
    sockp->rip = addr.sin_addr.s_addr;
    return rc;
}

#if 1	/* hostid */

int
ftd_mngt_getServerId()
{
	sprintf(gszServerUID, "%08lx", my_gethostid());
	return 0;
}

#else	/* MAC addr */

int
ftd_mngt_getServerId()
{
	struct sockaddr_in *sin;
	struct arpreq myarp;
	int sockfd;
	unsigned char	*ptr;
	struct utsname	utsname;
	struct hostent	*hp;

	bzero((caddr_t)&myarp, sizeof(myarp));
	myarp.arp_pa.sa_family = AF_INET;
	sin = (struct sockaddr_in *)&myarp.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = -1;

	if (uname(&utsname) == -1) {
		return -1;
	} else {
		if ((hp = gethostbyname(utsname.nodename)) == NULL) {
			return -1;
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr, sizeof (sin->sin_addr));
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return -1;
	}
	if (ioctl(sockfd, SIOCGARP, &myarp) == -1) {
		return -1;
	}
	close(sockfd);

	ptr = (unsigned char *)&myarp.arp_ha.sa_data[0];
	sprintf(gszServerUID, "%x%x%x", *(ptr+3), *(ptr+4), *(ptr+5));
	return 0;
}

#endif

/* for WR16044
	return value:
		 1:connect success (the last failure)
		 0:connect success (the last success)
		-1:connect failure

	The following values are stored in *level by the result of connect.
		connect success(the last success) : 12 (TRACE)
		connect success(the last failure) :  9 (INFO)
		connect failure(the last success) :  4 (WARNING)
		connect failure(the last failure) : 12 (TRACE)
 */
int
connect_st(int s, const struct sockaddr *name, int namelen, int *level)
{
	static int pre_err = 0;	/* 0:not error, 1:error */
	int retval = 0;

	retval = connect(s, name, namelen);
	if(retval < 0){	/* error */
		*level = (pre_err == 0) ? 4 : 12;
		pre_err = 1;
		return -1;
	} else {	/* not error */
		if(pre_err == 0){
			*level = 12;
			return 0;
		} else {
			*level = 9;
			pre_err = 0;
			return 1;
		}
	}
}

