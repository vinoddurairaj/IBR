/********************************************************* {COPYRIGHT-TOP} ***
* IBM Confidential
* OCO Source Materials
* 6949-32F - Softek Replicator for Unix and 6949-32K - Softek TDMF (IP) for Unix
*
*
* (C) Copyright IBM Corp. 2006, 2011  All Rights Reserved.
* The source code for this program is not published or otherwise  
* divested of its trade secrets, irrespective of what has been 
* deposited with the U.S. Copyright Office.
********************************************************* {COPYRIGHT-END} **/
/* #ident "@(#)$Id: Agent_sock.c,v 1.11 2012/02/13 22:12:23 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>
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
#include <sys/stat.h>
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
#include "common.h"
#include "licplat.h"

#define SOCK_CONNECT               0x0001
#define GET_SOCK_CONNECT(x)        ((x) & SOCK_CONNECT)
#define UNSET_SOCK_CONNECT(x)      ((x) &= 0xfffe)

extern ipAddress_t giTDMFCollectorIP;
extern ipAddress_t giTDMFAgentIP;

void abort_Agn();
	int     dbg;
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
	int     rlength = 0;
	rcode = 0;
	datalen = 0;
	dataptr = readpacket;
	errno = 0;
	Alarm = 300;

	sprintf(logmsg, "start. len = %d addr = %p\n", len,readpacket);
	logout(17,F_ftd_sock_recv,logmsg);

	/* read packet area clear */
	memset(readpacket,0x00,len);

	/* timer set */
	signal(SIGALRM, abort_Agn);

	/* data length read */
	while( datalen < len){
		alarm(Alarm);
		if((rlength = read(sockt->sockID,dataptr,len)) < 0){
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
					int len)            /* data length */
{
	int   rcode = 0;                        /* return code */
	int   written;

	rcode = 0;
	errno = 0;
	/*packet write */
	do
	{
	    if(( written = write(sockt->sockID,
				 writepacket + rcode,
				 len - rcode)) == -1){
		rcode = -1; /* packet write error */
	    } else {
		rcode += written;
	    }
	} while (rcode > 0 && rcode != len);
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


	if ((sockp = (sock_t*)calloc(1, sizeof(sock_t))) == NULL) {
		logoutx(4, F_sock_create, "calloc faied", "errno", errno);
		return NULL;
	}
	sockp->sockID = -1;
	return sockp;
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
	if(retval < 0)
	{	/* error */
		*level = (pre_err == 0) ? 4 : 12;
		pre_err = 1;
		return -1;
	} 
	else
	{	/* not error */
		if(pre_err == 0)
		{
			*level = 12;
			return 0;
		} 
		else 
		{
			*level = 9;
			pre_err = 0;
			return 1;
		}
	}
}

/*
 * sock_sendtox -- read a UDP message from bound port
 */
int
sock_sendtox(ipAddress_t rip,int rport,char * data,int datalen,int a,int b)
{  
	int rc;  
	int s = 0;  
	int n,len;  
	int level;
	struct sockaddr_in addr;
#if !defined(FTD_IPV4)
	struct sockaddr_in6 addr6;
#endif
	char tmpipaddr[20];
	char tmpipaddr6[48];					  
	char logmsg[100];
	int i ; 

	logoutx(17,F_sock_sendtox, logmsg, "code", s);
	logout(17,F_sock_sendtox,logmsg);

	memset(&addr, 0, sizeof(struct sockaddr_in));
#if !defined(FTD_IPV4)	
	memset(&addr6, 0, sizeof(struct sockaddr_in6));
#endif
	 
if(rip.Version == IPVER_4)
{
#if defined(linux)
	addr.sin_port = htons(rport);
#else
	addr.sin_port = rport;
#endif
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = rip.Addr.V4;

}
else
{
#if !defined(FTD_IPV4) 
#if defined(linux)
	addr6.sin6_port = htons(rport);
#else
	addr6.sin6_port = rport;
#endif
	   
	 memcpy(&addr6.sin6_addr.s6_addr, rip.Addr.V6.Word,sizeof(rip.Addr.V6.Word)) ;
	 addr6.sin6_family = AF_INET6;
   //	  	addr6.sin6_scope_id = 1;//rip.Addr.V6.ScopeID  - 1;

#endif
}


#if defined(FTD_IPV4)
	s = socket(AF_INET ,SOCK_STREAM, 0);
#else
	s = socket((rip.Version == IPVER_4) ? AF_INET : AF_INET6,SOCK_STREAM, 0);
#endif

	logoutx(17,F_sock_sendtox, "socket return", "code", s);
	if (s != 0)
	{
		setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&n,sizeof(int));
		
		if(rip.Version == IPVER_4)
		{
			rc = connect_st(s, (struct sockaddr *) &addr,sizeof(addr), &level);
		}
		else
		{
#if !defined(FTD_IPV4)
			rc = connect_st(s, (struct sockaddr *) &addr6,sizeof(addr6), &level);
#endif
		}

		if (rc >= 0) 
		{
			if (rc == 1)
				logout(level, F_sock_sendtox, "recovery from connect error.\n");
			/*  packet write */
			rc = write(s,data,datalen);
			if (rc != datalen) 
			{
				logoutx(4, F_sock_sendtox, "packet write error", "errno", errno);
			}
		} 
		else 
		{
			logoutx(level, F_sock_sendtox, "connect error agent_sock", "errno", errno);
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
	addr.sin_addr.s_addr = sockp->rip.Addr.V4;
#else
	addr.sin_addr.s_addr = sockp->rip.Addr.V4;
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
#if defined(SOLARIS) || defined(HPUX)
	int length;  
#else
	socklen_t length;
#endif

	struct sockaddr_in addr;
#if !defined(FTD_IPV4)  
	struct sockaddr_in6 addr6; 
#endif
	sprintf(logmsg,"start. len = %d\n", len);
	logout(17,F_sock_recvfrom,logmsg);
			
	if(giTDMFCollectorIP.Version == IPVER_4) 
	{
		memset(&addr, 0, sizeof(struct sockaddr_in));
		length = sizeof(struct sockaddr);

		rc = recvfrom( sockp->sockID, buf, len,0, (struct sockaddr *)&addr ,&length);
		sockp->rip.Addr.V4 = addr.sin_addr.s_addr;
    }
	else
	{
#if !defined(FTD_IPV4)
		memset(&addr6, 0, sizeof(struct sockaddr_in6));
		length = sizeof(struct sockaddr);
	   	//addr6.sin6_scope_id = 1;//sockp->rip.Addr.V6.ScopeID - 1;

		rc = recvfrom( sockp->sockID, buf, len,0, (struct sockaddr *)&addr6 ,&length);

		memcpy(sockp->rip.Addr.V6.Word, addr6.sin6_addr.s6_addr,sizeof(sockp->rip.Addr.V6.Word)) ;
#endif
	}

													
	return rc;			  
}
																	 
#if 1	/* hostid */

int
ftd_mngt_getServerId()
{
	sprintf(gszServerUID, "%08x", my_gethostid(HOSTID_IDENTIFICATION));
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

	if (uname(&utsname) == -1) 
	{
		return -1;
	} 
	else 
	{
		if ((hp = gethostbyname(utsname.nodename)) == NULL) 
		{
			return -1;
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr, sizeof (sin->sin_addr));
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
	{
		return -1;
	}
	if (ioctl(sockfd, SIOCGARP, &myarp) == -1) 
	{
		return -1;
	}
	close(sockfd);

	ptr = (unsigned char *)&myarp.arp_ha.sa_data[0];
	sprintf(gszServerUID, "%x%x%x", *(ptr+3), *(ptr+4), *(ptr+5));
	return 0;
}

#endif
