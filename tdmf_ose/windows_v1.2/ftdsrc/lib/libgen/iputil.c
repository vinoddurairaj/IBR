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
 *             void getnetconfs(u_long* ipaddrs);
 *                                    fills in an array of unsigned longs
 *                                    with the IP addresses of the current
 *                                    system (malloc with getnetconfcount)
 ***************************************************************************/

#if defined(HPUX)

#include <unistd.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * parse the /etc/rc.config.d/netconf file for IP_ADDRESS entries
 */
int
getnetconfcount()
{
    FILE *fd;
    char line[256], *ptr;
    int  len, count;

    if ((fd = fopen("/etc/rc.config.d/netconf","r")) == NULL) {
        return 0;
    }
    /* look for IP_ADDRESS[?]=xxx.xxx.xxx.xxx */
    count = 0;
    while (fgets(line, 256, fd) != NULL) {
        len = strlen(line);
        if ((len < 21) || (line[0] == '#'))
            continue;
        /* eat leading white space */
        ptr = line;
        while (*ptr && ((*ptr == ' ') || (*ptr == '\t'))) ptr++;
        if (strncmp(ptr, "IP_ADDRESS", 10) == 0) {
            count++;
        }
    }
    fclose(fd);
    return count;
}

void
getnetconfs(u_long *ipaddrs)
{
    FILE *fd;
    char line[256], *ptr;
    int  len, count;

    if ((fd = fopen("/etc/rc.config.d/netconf","r")) == NULL) {
        return;
    }
    /* look for IP_ADDRESS[?]=xxx.xxx.xxx.xxx */
    count = 0;
    while (fgets(line, 256, fd) != NULL) {
        len = strlen(line);
        if ((len < 21) || (line[0] == '#'))
            continue;
        /* eat leading white space */
        ptr = line;
        while (*ptr && ((*ptr == ' ') || (*ptr == '\t'))) ptr++;
        if (strncmp(ptr, "IP_ADDRESS", 10) == 0) {
            while (*ptr && (*ptr != '=')) ptr++;
            if (*ptr != '=')
                continue;
            ipaddrs[count] = inet_addr(++ptr);
            count++;
        }
    }
    fclose(fd);
}

#elif defined(SOLARIS) || defined(_AIX)

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

#if !defined(lint) && !defined(SABER)
char copyright[] =
"@(#) Copyright (c) 1986, 1989, 1990 The Regents of the University of California.\n\
portions Copyright (c) 1993 Digital Equipment Corporation\n\
portions Copyright (c) 1995 Internet Software Consortium\n\
All rights reserved.\n";
#endif /* not lint */

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

int
getnetconfcount(void)
{
    int count;
    struct ifconf ifc;
    struct ifreq ifreq, *ifr;
    char buf[32768], *cp, *cplim;
    int vs;
       
#ifdef PURE
    memset(buf, 0, sizeof(buf));
#endif
    count = 0;
    if ((vs = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        syslog(LOG_ERR, "socket(SOCK_STREAM): %m");
        exit(1);
    }
    ifc.ifc_len = sizeof buf;
    ifc.ifc_buf = buf;
    if (ioctl(vs, SIOCGIFCONF, (char *)&ifc) < 0) {
        syslog(LOG_ERR, "get interface configuration: %m - exiting");
        exit(1);
    }
#if defined(AF_LINK) && \
    !defined(RISCOS_BSD) && !defined(M_UNIX) && \
	!defined(sgi) && !defined(sun) && !defined(NO_SA_LEN)
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
        if (ioctl(vs, SIOCGIFADDR, (char *)&ifreq) < 0) {
            syslog(LOG_NOTICE, "get interface addr: %m");
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


void
getnetconfs(u_long* ipaddrs)
{
    struct ifconf ifc;
    struct ifreq ifreq, *ifr;
    char buf[32768], *cp, *cplim;
    int vs;
    int count;

#ifdef PURE
    memset(buf, 0, sizeof(buf));
#endif
    count = -1;
    if ((vs = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        syslog(LOG_ERR, "socket(SOCK_STREAM): %m");
        exit(1);
    }
    ifc.ifc_len = sizeof buf;
    ifc.ifc_buf = buf;
    if (ioctl(vs, SIOCGIFCONF, (char *)&ifc) < 0) {
        syslog(LOG_ERR, "get interface configuration: %m - exiting");
        exit(1);
    }
#if defined(AF_LINK) && \
    !defined(RISCOS_BSD) && !defined(M_UNIX) && \
	!defined(sgi) && !defined(sun) && !defined(NO_SA_LEN)
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
        if (ioctl(vs, SIOCGIFADDR, (char *)&ifreq) < 0) {
            syslog(LOG_NOTICE, "get interface addr: %m");
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
		
        /* use  ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr; */
    }
    close(vs);
}

#endif /* defined(SOLARIS) || defined(_AIX) */
