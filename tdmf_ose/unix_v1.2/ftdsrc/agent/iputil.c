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
/* #ident "@(#)$Id: iputil.c,v 1.8 2010/12/20 20:12:25 dkodjo Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

/****************************************************************************
 * iputil.c -- utilities for determining all IP addresses associated with
 *             the current system that the program is running on.  This is
 *             used for determining identity in with regards to
 *             configuration files and not use hostids.
 *            
 *             This module was borrowed from in.named and contains a
 *             Berkeley style copyright notice.
 *
 *             The external function prototypes live in config.h
 *
 *             int getnetconfcount(); returns the number of IP addresses
 *                                    associated with the current system
 *
 *             void getnetconfs(unsigned int* ipaddrs);
 *                                    fills in an array of unsigned longs
 *                                    with the IP addresses of the current
 *                                    system (malloc with getnetconfcount)
 ***************************************************************************/

#include "errors.h"

/*
 * ++Copyright++ 1986, 1989, 1990
 * -
 * Copyright (c) 1986, 1989, 1990
 *    The Regents of the University of California.  All rights reserved.
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
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
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
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

#ifdef NOT_USE_LIBGEN
#if !defined(lint) && !defined(SABER)
char copyright[] =
"@(#) Copyright (c) 1986, 1989, 1990 The Regents of the University of California.\n\
portions Copyright (c) 1993 Digital Equipment Corporation\n\
portions Copyright (c) 1995 Internet Software Consortium\n\
All rights reserved.\n";
#endif /* not lint */
#endif /* NOT_USE_LIBGEN */
extern char copyright[];

/*
 * Internet Name server (see RCF1035 & others).
 */
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#if !defined(SYSV) && defined(XXX)
#include <sys/wait.h>
#endif /* !SYSV */
#if defined(__osf__)
# define _SOCKADDR_LEN		/* XXX - should be in portability.h but that
				 * would need to be included before socket.h
				 */
#endif
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#if defined(__osf__)
# include <sys/mbuf.h>
# include <net/route.h>
#endif
#if defined(_AIX)
# include <sys/time.h>
# define TIME_H_INCLUDED
#endif
#include <net/if.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <resolv.h>
#if defined(SVR4) || defined(__SVR4)
# include <sys/sockio.h>
#endif
#include "common.h" 
#include "tdmfAgent.h"
#include "tdmfAgent_trace.h"

#if defined(HPUX) && !defined(FTD_IPV4)
#include <net/if6.h>
#endif

static int
siocgifconf(int sd, struct ifconf *ifc)
{
    return FTD_IOCTL_CALL(sd, SIOCGIFCONF, (char *)ifc);
}

struct ifreq64 {
    struct ifreq ifr;
#if defined(linux)
    char pad[4 + 4];
#endif
};

#if defined(HPUX)
int
agent_getnetconfcount6(void)
{
  #if !defined(FTD_IPV4)
    
    int count;
    struct if_laddrconf ifc;
    struct if_laddrreq *ifr, *ifend;
	struct if_laddrreq ifreq;
	struct if_laddrreq ifs[64];
    int vs, i;
  
									   
    count = 0;

	i = 0;
	while (i++ < 10) {
	    if ((vs = socket(AF_INET6, SOCK_STREAM, 0)) == 0) {
			if (errno == EINTR) {
				continue;
			}
			logoutx(7, F_getnetconfcount, "socket error", "errno", errno);
			return 0;
   		}
		break;
    }
	if (i == 10) {
		return 0;
	}

    ifc.iflc_len = sizeof(ifs);
    ifc.iflc_req = ifs;
    if (FTD_IOCTL_CALL(vs, SIOCGLIFCONF, (char *)&ifc) < 0) {
		logoutx(7, F_getnetconfcount, "ioctl error - get interface configuration", "errno", errno);
		return 0;
    }
  
    ifend = ifs + (ifc.iflc_len / sizeof(struct ifreq));
    for (ifr = ifc.iflc_req ; ifr < ifend;
         ifr++) {
    
        if (ifr->iflr_addr.sa_family != AF_INET6 ||
            ((struct sockaddr_in6 *)
                &ifr->iflr_addr)->sin6_addr.s6_addr == 0) {
            continue;
        }
												 
  
  		strcpy(ifreq.iflr_name, ifr->iflr_name);
#if !defined(BSD) || (BSD < 199103)
        if (FTD_IOCTL_CALL(vs, SIOCGLIFADDR, (char *)&ifreq) < 0) {
			logoutx(7, F_getnetconfcount, "ioctl error - get interface addr", "errno", errno);
            continue;
        }
#endif
        /*
         * Skip over address 0.0.0.0 since this will conflict
         * with binding to wildcard address later.  Interfaces
         * which are not completely configured can have this addr.
         */
        if (((struct sockaddr_in6 *)&ifreq.iflr_addr)->sin6_addr.s6_addr
            == 0x00000000) {
			logoutx(4, F_getnetconfs, "address = zero", "errno", 4);

            continue;
        }
        count++;
    }
    close(vs);
    return (count);
	#endif
}

void
agent_getnetconfs6(char* ipv6)
{
   #if !defined(FTD_IPV4)
   
    struct if_laddrconf ifc;
    struct if_laddrreq *ifr, *ifend;
	struct if_laddrreq ifreq;
	struct if_laddrreq ifs[64];
    int vs;
    int count, i;
    char* logmsg;
	char *tmpipaddrs = ipv6;

    count = -1;

	i = 0;
	while (i++ < 10) {
	    if ((vs = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
			if (errno == EINTR) {
				continue;
			}
			logoutx(7, F_getnetconfs, "socket error", "errno", errno);
			return;
   		}
		break;
    }
	if (i == 10) {
		return;
	}

    ifc.iflc_len = sizeof(ifs);
    ifc.iflc_req = ifs;
    if (FTD_IOCTL_CALL(vs, SIOCGLIFCONF, (char *)&ifc) < 0) {
		logoutx(7, F_getnetconfs, "ioctl error - get interface configuration", "errno", errno);
		return;
    }
  
   	ifend = ifs + (ifc.iflc_len / sizeof(struct ifreq)); 
    for (ifr = ifc.iflc_req ; ifr < ifend;
         ifr++) {        
        if (ifr->iflr_addr.sa_family != AF_INET6 ||
            ((struct sockaddr_in6 *)
                &ifr->iflr_addr)->sin6_addr.s6_addr == 0) {
			   logoutx(4, F_getnetconfs, "address family or zero address", "errno", 4);
            continue;															 
        } 
        strcpy(ifreq.iflr_name, ifr->iflr_name);

#if !defined(BSD) || (BSD < 199103)
        if (FTD_IOCTL_CALL(vs, SIOCGLIFADDR, (char *)&ifreq) < 0) {
	 		logoutx(7, F_getnetconfs, "ioctl error - get interface addr", "errno", errno);
            continue;
        }
#endif
        /*
         * Skip over address 0.0.0.0 since this will conflict
         * with binding to wildcard address later.  Interfaces
         * which are not completely configured can have this addr.
         */
        if (((struct sockaddr_in6 *)&ifreq.iflr_addr)->sin6_addr.s6_addr
            == 0x00000000) {
			logoutx(4, F_getnetconfs, "address = zero", "errno", 4);
            continue;
        } 

		inet_ntop(AF_INET6,(struct in6_addr *)&(((struct sockaddr_in6 *)&ifreq.iflr_addr)->sin6_addr.s6_addr),
                   tmpipaddrs,INET6_ADDRSTRLEN);
           tmpipaddrs+=INET6_ADDRSTRLEN;


    }
    close(vs);
	#endif
}
  
#endif 


#if defined(linux) ||  defined(HPUX)
int
agent_getnetconfcount(void)
{
    int count;
    struct ifconf ifc;
    struct ifreq *ifr, *ifend;
	struct ifreq ifreq;
	struct ifreq ifs[64];
    int vs, i;
  
									   
    count = 0;

	i = 0;
	while (i++ < 10) {
	    if ((vs = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
			if (errno == EINTR) {
				continue;
			}
			logoutx(7, F_getnetconfcount, "socket error", "errno", errno);
			return 0;
   		}
		break;
    }
	if (i == 10) {
		return 0;
	}

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;
    if (FTD_IOCTL_CALL(vs, SIOCGIFCONF, (char *)&ifc) < 0) {
		logoutx(7, F_getnetconfcount, "ioctl error - get interface configuration", "errno", errno);
		return 0;
    }
  
    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req ; ifr < ifend;
         ifr++) {
    
        if (ifr->ifr_addr.sa_family != AF_INET ||
            ((struct sockaddr_in *)
                &ifr->ifr_addr)->sin_addr.s_addr == 0) {
            continue;
        }
												 
  
  		strcpy(ifreq.ifr_name, ifr->ifr_name);
#if !defined(BSD) || (BSD < 199103)
        if (FTD_IOCTL_CALL(vs, SIOCGIFADDR, (char *)&ifreq) < 0) {
			logoutx(7, F_getnetconfcount, "ioctl error - get interface addr", "errno", errno);
            continue;
        }
#endif
        /*
         * Skip over address 0.0.0.0 since this will conflict
         * with binding to wildcard address later.  Interfaces
         * which are not completely configured can have this addr.
         */
        if (((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr
            == 0x00000000) {
			logoutx(4, F_getnetconfs, "address = zero", "errno", 4);

            continue;
        }
        count++;
    }
    close(vs);
    return (count);
}

int
agent_getnetconfs(unsigned int* ipaddrs)
{
    struct ifconf ifc;
    struct ifreq *ifr, *ifend;
	struct ifreq ifreq;
	struct ifreq ifs[64];
    int vs;
    int count, i;
    char* logmsg;

    count = -1;

	i = 0;
	while (i++ < 10) {
	    if ((vs = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			if (errno == EINTR) {
				continue;
			}
			logoutx(7, F_getnetconfs, "socket error", "errno", errno);
			return 0;
   		}
		break;
    }
	if (i == 10) {
		return 0;
	}

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;
    if (FTD_IOCTL_CALL(vs, SIOCGIFCONF, (char *)&ifc) < 0) {
		logoutx(7, F_getnetconfs, "ioctl error - get interface configuration", "errno", errno);
		return 0;
    }
  
   	ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq)); 
    for (ifr = ifc.ifc_req ; ifr < ifend;
         ifr++) {        
        if (ifr->ifr_addr.sa_family != AF_INET ||
            ((struct sockaddr_in *)
                &ifr->ifr_addr)->sin_addr.s_addr == 0) {
			   logoutx(4, F_getnetconfs, "address family or zero address", "errno", 4);
            continue;															 
        } 
        strcpy(ifreq.ifr_name, ifr->ifr_name);

#if !defined(BSD) || (BSD < 199103)
        if (FTD_IOCTL_CALL(vs, SIOCGIFADDR, (char *)&ifreq) < 0) {
	 		logoutx(7, F_getnetconfs, "ioctl error - get interface addr", "errno", errno);
            continue;
        }
#endif
        /*
         * Skip over address 0.0.0.0 since this will conflict
         * with binding to wildcard address later.  Interfaces
         * which are not completely configured can have this addr.
         */
        if (((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr
            == 0x00000000) {
			logoutx(4, F_getnetconfs, "address = zero", "errno", 4);
            continue;
        } 
        ipaddrs[++count] = ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr;

#if defined(linux)
		ipaddrs[count] = ntohl(ipaddrs[count]);
#endif

    }
    close(vs);
	return ++count;
}

#else

int
agent_getnetconfcount(void)
{
    int count;
    struct ifconf ifc;
    struct ifreq ifreq, *ifr;
    char buf[32768], *cp, *cplim;
    int vs, i;
       
#ifdef PURE
    memset(buf, 0, sizeof(buf));
#endif
    count = 0;

	i = 0;
	while (i++ < 10) {
	    if ((vs = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
			if (errno == EINTR) {
				continue;
			}
/*
			syslog(LOG_ERR, "socket(SOCK_STREAM): %m");
*/
			logoutx(7, F_getnetconfcount, "socket error", "errno", errno);
			return 0;
   		}
		break;
    }
	if (i == 10) {
		return 0;
	}

    ifc.ifc_len = sizeof buf;
    ifc.ifc_buf = buf;
    if (FTD_IOCTL_CALL(vs, SIOCGIFCONF, (char *)&ifc) < 0) {
/*
        syslog(LOG_ERR, "get interface configuration: %m - exiting");
*/
		logoutx(7, F_getnetconfcount, "ioctl error - get interface configuration", "errno", errno);
		return 0;
    }
#if defined(AF_LINK) && \
    !defined(RISCOS_BSD) && !defined(M_UNIX) && \
	!defined(sgi) && !defined(sun) && !defined(HPUX) && !defined(NO_SA_LEN)
#define my_max(a, b) (a > b ? a : b)
#define my_size(p)	my_max((p).sa_len, sizeof(p))
#else
#define my_size(p) (sizeof (p))
#endif
	cplim = buf + ifc.ifc_len;    /* skip over if's with big ifr_addr's */
    for (cp = buf;
         cp < cplim;
         cp += sizeof (ifr->ifr_name) + my_size(ifr->ifr_addr)) {
#undef my_size
        ifr = (struct ifreq *)cp;
        if (ifr->ifr_addr.sa_family != AF_INET ||
            ((struct sockaddr_in *)
                &ifr->ifr_addr)->sin_addr.s_addr == 0) {
            continue;
        }
        ifreq = *ifr;
        /*
         * Don't test IFF_UP, packets may still be received at this
         * address if any other interface is up.
         */
#if !defined(BSD) || (BSD < 199103)
        if (FTD_IOCTL_CALL(vs, SIOCGIFADDR, (char *)&ifreq) < 0) {
/*
            syslog(LOG_NOTICE, "get interface addr: %m");
*/
			logoutx(7, F_getnetconfcount, "ioctl error - get interface addr", "errno", errno);
            continue;
        }
#endif
        /*
         * Skip over address 0.0.0.0 since this will conflict
         * with binding to wildcard address later.  Interfaces
         * which are not completely configured can have this addr.
         */
        if (((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr
            == 0x00000000) {
            continue;
        }
        count++;
        /* use  ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr; */
    }
    close(vs);
    return (count);
}


int
agent_getnetconfs(unsigned int* ipaddrs)
{
    struct ifconf ifc;
    struct ifreq ifreq, *ifr;
    char buf[32768], *cp, *cplim;
    int vs;
    int count, i;

#ifdef PURE
    memset(buf, 0, sizeof(buf));
#endif
    count = -1;

	i = 0;
	while (i++ < 10) {
	    if ((vs = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			if (errno == EINTR) {
				continue;
			}
/*
			syslog(LOG_ERR, "socket(SOCK_STREAM): %m");
*/
			logoutx(7, F_getnetconfs, "socket error", "errno", errno);
			return 0;
   		}
		break;
    }
	if (i == 10) {
		return 0;
	}

    ifc.ifc_len = sizeof buf;
    ifc.ifc_buf = buf;
    if (FTD_IOCTL_CALL(vs, SIOCGIFCONF, (char *)&ifc) < 0) {
/*
        syslog(LOG_ERR, "get interface configuration: %m - exiting");
*/
		logoutx(7, F_getnetconfs, "ioctl error - get interface configuration", "errno", errno);
		return 0;
    }
#if defined(AF_LINK) && \
    !defined(RISCOS_BSD) && !defined(M_UNIX) && \
	!defined(sgi) && !defined(sun) && !defined(HPUX) && !defined(NO_SA_LEN)
#define my_max(a, b) (a > b ? a : b)
#define my_size(p)	my_max((p).sa_len, sizeof(p))
#else
#define my_size(p) (sizeof (p))
#endif
    cplim = buf + ifc.ifc_len;    /* skip over if's with big ifr_addr's */
    for (cp = buf; cp < cplim;
         cp += sizeof (ifr->ifr_name) + my_size(ifr->ifr_addr)) {
#undef my_size
        ifr = (struct ifreq *)cp;
        if (ifr->ifr_addr.sa_family != AF_INET ||
            ((struct sockaddr_in *)
                &ifr->ifr_addr)->sin_addr.s_addr == 0) {
            continue;
        }
        ifreq = *ifr;
        /*
         * Don't test IFF_UP, packets may still be received at this
         * address if any other interface is up.
         */
#if !defined(BSD) || (BSD < 199103)
        if (FTD_IOCTL_CALL(vs, SIOCGIFADDR, (char *)&ifreq) < 0) {
/*
            syslog(LOG_NOTICE, "get interface addr: %m");
*/
			logoutx(7, F_getnetconfs, "ioctl error - get interface addr", "errno", errno);
            continue;
        }
#endif
        /*
         * Skip over address 0.0.0.0 since this will conflict
         * with binding to wildcard address later.  Interfaces
         * which are not completely configured can have this addr.
         */
        if (((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr
            == 0x00000000) {
            continue;
        }
        ipaddrs[++count] = ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr;
#if defined(linux)
		ipaddrs[count] = ntohl(ipaddrs[count]);
#endif
		
        /* use  ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr; */
    }
    close(vs);
	return ++count;
}

#endif


/* IPV6 Support */


#if defined(SOLARIS)
int
agent_getnetconfcount6(void)
{
    int count = 0;
#if !defined(FTD_IPV4)
    char *buf;
    int vs, i;
    struct lifnum  lifn;
    struct lifconf lifc;
    struct lifreq *ifptr, *end;
 
	i = 0;
	while (i++ < 10) {
	    if ((vs = socket(AF_INET6, SOCK_STREAM, 0)) == 0) {
			if (errno == EINTR) {
				continue;
			}
/*
			syslog(LOG_ERR, "socket(SOCK_STREAM): %m");
*/
			reporterr(ERRFAC, M_SOCKFAIL, ERRWARN, strerror(errno));
			return -1;
   		}
		break;
    }
	if (i == 10) {
		return count;
	}

    lifn.lifn_family = AF_UNSPEC;
    lifn.lifn_flags = 0;
    if (ioctl (vs, SIOCGLIFNUM, &lifn) < 0) {
        return -1;
    }
    buf = (caddr_t) malloc(sizeof(struct lifreq) * lifn.lifn_count);

    lifc.lifc_family = AF_UNSPEC;
    lifc.lifc_flags = 0;
    lifc.lifc_len = lifn.lifn_count * sizeof (struct lifreq);
    lifc.lifc_buf = buf;

    if (ioctl (vs, SIOCGLIFCONF, (char *) &lifc) < 0) {
        return -1;
    }
    end = lifc.lifc_req + lifn.lifn_count;

    for (ifptr = lifc.lifc_req; ifptr < end; ifptr++) {
        if (((struct sockaddr_in6 *)&ifptr->lifr_addr)->sin6_addr.s6_addr == 0x00000000 && ((struct sockaddr_in *)&ifptr->lifr_addr)->sin_addr.s_addr == 0x00000000) {
            continue;
        }

	if (ifptr->lifr_addr.ss_family == AF_INET || ifptr->lifr_addr.ss_family == AF_INET6) {
            count++;
    	}
    }
    close(vs);
#endif 
    return (count);
}

void
agent_getnetconfs6(char *ipv6)
{
#if !defined(FTD_IPV4)
    char *buf;
    int vs;
    int i;
    char *tmpipaddrs = ipv6;
    struct lifnum  lifn;
    struct lifconf lifc;
    struct lifreq *ifptr, *end;

	i = 0;
	while (i++ < 10) {
	    if ((vs = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
			if (errno == EINTR) {
				continue;
			}
/*
			syslog(LOG_ERR, "socket(SOCK_STREAM): %m");
*/
			reporterr(ERRFAC, M_SOCKFAIL, ERRWARN, strerror(errno));
			return;
   		}
		break;
    }
	if (i == 10) {
		return;
	}
    lifn.lifn_family = AF_UNSPEC;
    lifn.lifn_flags = 0;
    if (ioctl (vs, SIOCGLIFNUM, &lifn) < 0) {
        return;
    }
    buf = (caddr_t) malloc(sizeof(struct lifreq) * lifn.lifn_count);

    lifc.lifc_family = AF_UNSPEC;
    lifc.lifc_flags = 0;
    lifc.lifc_len = lifn.lifn_count * sizeof (struct lifreq);
    lifc.lifc_buf = buf;

    if (ioctl (vs, SIOCGLIFCONF, (char *) &lifc) < 0) {
        return;
    }
    end = lifc.lifc_req + lifn.lifn_count;

    for (ifptr = lifc.lifc_req; ifptr < end; ifptr++) {
        if (((struct sockaddr_in6 *)&ifptr->lifr_addr)->sin6_addr.s6_addr == 0x00000000 && ((struct sockaddr_in *)&ifptr->lifr_addr)->sin_addr.s_addr == 0x00000000) {
            continue;
        }

	if (ifptr->lifr_addr.ss_family == AF_INET) {
	    strcpy(tmpipaddrs, inet_ntoa(((struct sockaddr_in *)&ifptr->lifr_addr)->sin_addr));
	    tmpipaddrs+=INET6_ADDRSTRLEN;
	} else if (ifptr->lifr_addr.ss_family == AF_INET6) {
	    inet_ntop(AF_INET6,(struct in6_addr *)&(((struct sockaddr_in6 *)&ifptr->lifr_addr)->sin6_addr),
                tmpipaddrs,INET6_ADDRSTRLEN);
	    tmpipaddrs+=INET6_ADDRSTRLEN;
    	}
#if 0
	if ((ifptr->lifr_addr.ss_family == AF_INET)  || (ifptr->lifr_addr.ss_family == AF_INET6))
        {
           tmpipaddrs = (char*) &(((struct sockaddr_in6 *)&ifptr->lifr_addr)->sin6_addr);
        //    strcpy(tmpipaddrs,(char *) &(((struct sockaddr_in6 *)&ifptr->lifr_addr)->sin6_addr));
            tmpipaddrs+=INET6_ADDRSTRLEN;

        }
#endif 
    }
    close(vs);
#endif 
}
#endif /* defined(SOLARIS) */



#if defined(_AIX)
int
agent_getnetconfcount6(void)
{
#if !defined(FTD_IPV4)
    int count;
    struct ifconf ifc;
    struct ifreq *ifr;
    char buf[32768];
    char *cp, *cplim;
    int vs, i;

/*#ifdef PURE*/
    memset(buf, 0, sizeof(buf));
/*#endif*/
    count = 0;

        i = 0;
        while (i++ < 10) {
            if ((vs = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
                        if (errno == EINTR) {
                                continue;
                        }
/*
                        syslog(LOG_ERR, "socket(SOCK_STREAM): %m");
*/
                        reporterr(ERRFAC, M_SOCKFAIL, ERRWARN, strerror(errno));
                        return -1;
                }
                break;
    }
        if (i == 10) {
                return count;
        }
   ifc.ifc_len = sizeof buf;
    ifc.ifc_buf = buf;
    if (siocgifconf(vs, (struct ifconf *)&ifc) < 0) {
/*
        syslog(LOG_ERR, "get interface configuration: %m - exiting");
*/
                reporterr(ERRFAC, M_IOCTLERR, ERRWARN,
                        "SIOCGIFCONF", strerror(errno));
        close(vs);
      //  EXIT(EXITANDDIE);
    }
#if defined(AF_LINK) && \
    !defined(RISCOS_BSD) && !defined(M_UNIX) && \
        !defined(sgi) && !defined(sun) && !defined(NO_SA_LEN)

#define my_max(a, b) (a > b ? a : b)
#define my_size(p)      my_max((p).sa_len, sizeof(p))
#else
#define my_size(p) (sizeof (p))
#endif
        cplim = buf + ifc.ifc_len;    /* skip over if's with big ifr_addr's */
    for (cp = buf;
         cp < cplim;
         cp += sizeof (ifr->ifr_name) + my_size(ifr->ifr_addr)) {
#undef my_size
        ifr = (struct ifreq *)cp;
#if 0
        if (((struct sockaddr_in6 *)&ifr->ifr_addr)->sin6_addr.s6_addr == 0 &&
            ((struct sockaddr_in *)
                &ifr->ifr_addr)->sin_addr.s_addr == 0) {
            continue;
        }
#endif
        /*
         * Skip over address 0.0.0.0 since this will conflict
         * with binding to wildcard address later.  Interfaces
         * which are not completely configured can have this addr.
         */
        if (((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr.s_addr
            == 0x00000000 && ((struct sockaddr_in6 *)&ifr->ifr_addr)->sin6_addr.s6_addr == 0x00000000) {
            continue;
        }
        if (ifr->ifr_addr.sa_family == AF_INET || ifr->ifr_addr.sa_family == AF_INET6)
            count++;
}
    close(vs);
    return (count);
#endif 
}
void
agent_getnetconfs6(char* ipv6)
{
#if !defined(FTD_IPV4)
    struct ifconf ifc;
    struct ifreq *ifr;
    char buf[32768];
    char *cp, *cplim;
    int vs;
    int count, i;

  struct sockaddr *sa; char *tmpipaddrs; 
#ifdef PURE
    memset(buf, 0, sizeof(buf));
#endif
    count = -1;

        i = 0;
        while (i++ < 10) {
            if ((vs = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        if (errno == EINTR) {
                                continue;
                        }
/*
                        syslog(LOG_ERR, "socket(SOCK_STREAM): %m");
*/
                        reporterr(ERRFAC, M_SOCKFAIL, ERRWARN, strerror(errno));
                        return;
                }
                break;
    }
        if (i == 10) {
                return;
        }

    ifc.ifc_len = sizeof buf;
    ifc.ifc_buf = buf;
    tmpipaddrs = ipv6;
 if (siocgifconf(vs, &ifc) < 0) {
/*
        syslog(LOG_ERR, "get interface configuration: %m - exiting");
*/
                reporterr(ERRFAC, M_IOCTLERR, ERRWARN,
                        "SIOCGIFCONF", strerror(errno));
        close(vs);
        //EXIT(EXITANDDIE);
    }
#if defined(AF_LINK) && \
    !defined(RISCOS_BSD) && !defined(M_UNIX) && \
        !defined(sgi) && !defined(sun) && !defined(NO_SA_LEN)
#define my_max(a, b) (a > b ? a : b)
#define my_size(p)      my_max((p).sa_len, sizeof(p))
#else
#define my_size(p) (sizeof (p))
#endif
    cplim = buf + ifc.ifc_len;    /* skip over if's with big ifr_addr's */
    for (cp = buf; cp < cplim;
         cp += sizeof (ifr->ifr_name) + my_size(ifr->ifr_addr)) {
#undef my_size
        ifr = (struct ifreq *)cp;
#if 0
        if (((struct sockaddr_in6 *)&ifr->ifr_addr)->sin6_addr.s6_addr == 0 &&
            ((struct sockaddr_in *)
                &ifr->ifr_addr)->sin_addr.s_addr == 0) {
            continue;
        }
#endif
        /*
         * Skip over address 0.0.0.0 since this will conflict
         * with binding to wildcard address later.  Interfaces
         * which are not completely configured can have this addr.
         */
        if (((struct sockaddr_in6 *)&ifr->ifr_addr)->sin6_addr.s6_addr == 0x00000000 && ((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr.s_addr
            == 0x00000000) {
            continue;
        }
        sa=(struct sockaddr *)&(ifr->ifr_addr);
        if (ifr->ifr_addr.sa_family == AF_INET) {
            strcpy(tmpipaddrs,inet_ntoa(((struct sockaddr_in *)sa)->sin_addr));
           tmpipaddrs+=INET6_ADDRSTRLEN;
        } else if (ifr->ifr_addr.sa_family == AF_INET6) {
            inet_ntop(AF_INET6,(struct in6_addr *)&(((struct sockaddr_in6 *)sa)->sin6_addr),
                tmpipaddrs,INET6_ADDRSTRLEN);
            tmpipaddrs+=INET6_ADDRSTRLEN;
        }

    }
    close(vs);
#endif 
}

#endif //AIX 


#if defined(linux)
#include <ifaddrs.h>
int agent_getnetconfcount6(void)
{
#if !defined(FTD_IPV4)
     struct ifaddrs *ifap0, *ifap;
     struct sockaddr *sa ;
     int count=0;

   if (getifaddrs(&ifap0)) {
         reporterr(ERRFAC, M_COUNTFAIL,ERRWARN);
         return -1;
   }

   for (ifap = ifap0; ifap; ifap=ifap->ifa_next) {
   if (((ifap->ifa_addr->sa_family == AF_INET6) ||(ifap->ifa_addr->sa_family == AF_INET)) && ifap->ifa_addr != NULL){
              count++;
      }
   }
        return count;
#endif 
}

/*------IP Address fetching function -------*/
void  agent_getnetconfs6(char* ipv6)
{
#if !defined(FTD_IPV4)
     struct ifaddrs *ifap0, *ifap;
     struct sockaddr *sa ;
     int i=0,count=0;
     int ptr=0;
     char *tmpipaddrs = ipv6;
        
   if (getifaddrs(&ifap0)) {
          reporterr(ERRFAC, M_ADDRFAIL,ERRWARN); 
          return;
   }

   for (ifap = ifap0; ifap; ifap=ifap->ifa_next) {
   if (ifap->ifa_addr == NULL)
              continue;

   if ((ifap->ifa_addr->sa_family == AF_INET6)) {
         inet_ntop(AF_INET6,(struct in6_addr *)&(((struct sockaddr_in6 *)ifap->ifa_addr)->sin6_addr),
                      tmpipaddrs,INET6_ADDRSTRLEN);
           tmpipaddrs+=INET6_ADDRSTRLEN;
      }
   if ((ifap->ifa_addr->sa_family == AF_INET)) {
            inet_ntop(AF_INET,(struct in_addr *)&(((struct sockaddr_in *)ifap->ifa_addr)->sin_addr),
                      tmpipaddrs,INET6_ADDRSTRLEN); 
            tmpipaddrs+=INET6_ADDRSTRLEN;
      }
   }
#endif
}

#endif /* defined(linux) */
