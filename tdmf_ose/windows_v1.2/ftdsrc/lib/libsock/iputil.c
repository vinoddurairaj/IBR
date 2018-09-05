/*
 * iputil.c - 
 * 
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 *
 * utilities for determining all IP addresses associated with
 * the current system that the program is running on.  This is
 * used for determining identity in with regards to
 * configuration files and not use hostids.
 *            
 * This module was borrowed from in.named and contains a
 * Berkeley style copyright notice.
 *
 * The external function prototypes live in config.h
 *
 * int getnetconfcount(); returns the number of IP addresses
 *                        associated with the current system
 *
 * void getnetconfs(u_long* ipaddrs);
 *                        fills in an array of unsigned longs
 *                        with the IP addresses of the current
 *                        system (malloc with getnetconfcount)
 */

#include <stdio.h>
#ifdef _WINDOWS
#include <windows.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(SOLARIS) || defined(_AIX)
#include <netdb.h>
#endif
#endif

/*
 * name_is_ipstring -- determine if the hostname is an ip address string
 */
int
name_is_ipstring(char *name)
{
	int	i;

	int len = strlen(name);

	for (i=0;i<len;i++) {
		if (isalpha(name[i])) {
			return 0;
		}
	}

	return 1; 
}

/*
 * name_to_ip -- convert hostname to ip address
 */
int
name_to_ip(char *name, unsigned long *ip)
{
	struct hostent *host;
	struct in_addr in;
	char **p;

	if (name == NULL || strlen(name) == 0) {
		return -1;
	} 
    
	host = gethostbyname(name);
    
	if (host == NULL) {
		return -1;
	}
    
	p = host->h_addr_list;

	memcpy(&in.s_addr, *p, sizeof(in.s_addr));
	*ip = in.s_addr; 

	return 0; 
}

/*
 * ipstring_to_ip -- convert ip address string to ip address
 */
int
ipstring_to_ip(char *name, unsigned long *ip)
{

	if (name == NULL || strlen(name) == 0) {
		return -1;
	} 

	{
		int		n1, n2, n3, n4;
		
		char	*s, *lname = strdup(name);
		
		if ((s = strtok(lname, ".\n")))
			n1 = atoi(s);
		if ((s = strtok(NULL, ".\n")))
			n2 = atoi(s);
		if ((s = strtok(NULL, ".\n")))
			n3 = atoi(s);
		if ((s = strtok(NULL, ".\n")))
			n4 = atoi(s);

		free ( lname );

		*ip = n1 + (n2 << 8) + (n3 << 16) + (n4 << 24); 

	}

	return 0; 
}

/*
 * ip_to_ipstring -- convert ip address to ipstring in dot notation
 */
int
ip_to_ipstring(unsigned long ip, char *ipstring)
{
	int a1, a2, a3, a4;

	a1 = 0x000000ff & (ip);
	a2 = 0x000000ff & (ip >> 8);
	a3 = 0x000000ff & (ip >> 16);
	a4 = 0x000000ff & (ip >> 24);
	
	sprintf(ipstring, "%d.%d.%d.%d", a1, a2, a3, a4);

	return 0; 
}

/*
 * ip_to_name -- convert ip address to hostname
 */
int
ip_to_name(unsigned long ip, char *name)
{
	struct hostent	*host;
	unsigned long	addr;
	char			ipstring[32], **p;

	ip_to_ipstring(ip, ipstring);

	addr = inet_addr(ipstring);
	host = gethostbyaddr((const char *)&addr, sizeof(addr), AF_INET);

	if (host == NULL) {
		return -1;
	}
	p = host->h_addr_list;
	strcpy(name, host->h_name);

	return 0; 
}


#if defined(HPUX)


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
getnetconfs(unsigned long *iplist)
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
            iplist[count] = inet_addr(++ptr);
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
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
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
getnetconfs(u_long* iplist)
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
        iplist[++count] = ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr;
		
        /* use  ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr; */
    }
    close(vs);
}

#elif defined(_WINDOWS)

int GetAdapterIpAddress(char *szAdapter, char *szIpAddress)
{
    char tcpip[1024];
    HKEY hKey;
    int  ret = 0;

    strcpy(tcpip, "System\\CurrentControlSet\\Services\\");
    strcat(tcpip, szAdapter);
    strcat(tcpip, "\\Parameters\\Tcpip");

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, tcpip, 0L, KEY_READ, &hKey) == ERROR_SUCCESS) 
    {
        //DHCP or static address ?
        char	pszIpAddr[24],*pszValueName;
		LONG	size = sizeof(pszIpAddr);
		DWORD	dwType,dwDHCP;

        dwType  = REG_DWORD;
        size    = sizeof(dwDHCP);
		if ( RegQueryValueEx(hKey,
                        "EnableDHCP",
						NULL,
						&dwType,
                        (BYTE*)&dwDHCP,
                        &size) != ERROR_SUCCESS)
        {
            dwDHCP = 0;//"EnableDHCP" value not found.
        }

        if ( dwDHCP )
            pszValueName = "DhcpIPAddress";
        else
            pszValueName = "IPAddress";

        dwType  = REG_SZ;
        size    = sizeof(pszIpAddr);
		if ( RegQueryValueEx(hKey,
                        pszValueName,
						NULL,
						&dwType,
                        pszIpAddr,
                        &size) == ERROR_SUCCESS)
        {
            strcpy(szIpAddress, pszIpAddr);
#ifdef _DEBUG
            OutputDebugString("GetAdapterIpAddress() returns ip <");
            OutputDebugString(szIpAddress);
            OutputDebugString(">\n");
#endif
            ret = 1;
        }

		RegCloseKey(hKey);
    }

	return ret;//0 = error, 1 = success
}

//
// GetNicList
// fill NIC/IP list and return to client
//
void GetNicList(char szNicList[99][256])
{
	HKEY            hKey, hKeyCard;
	DWORD           iValue, iList = 0;
    LONG            Status;
	FILETIME        ftLastWrite;
	DWORD           NameLen;
	char            NameBuf[MAX_PATH];
	char			szKey[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey,
						  0L, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
		char    lpszClass[MAX_PATH];	// address of buffer for class string 
		DWORD   lpcchClass = sizeof(lpszClass);	// address of size of class string buffer 
		DWORD   lpcSubKeys;	// address of buffer for number of subkeys 
		DWORD   lpcchMaxSubkey;	// address of buffer for longest subkey name length  
		DWORD   lpcchMaxClass;	// address of buffer for longest class string length 
		DWORD   lpcValues;	// address of buffer for number of value entries 
		DWORD   lpcchMaxValueName;	// address of buffer for longest value name length 
		DWORD   lpcbMaxValueData;	// address of buffer for longest value data length 
		DWORD   lpcbSecurityDescriptor;	// address of buffer for security descriptor length 
		FILETIME  lpftLastWriteTime; 	// address of buffer for last write time 
	
        Status = RegQueryInfoKey (
			hKey,	// handle of key to query 
			lpszClass,	// address of buffer for class string 
			&lpcchClass,	// address of size of class string buffer 
			NULL,	// reserved 
			&lpcSubKeys,	// address of buffer for number of subkeys 
			&lpcchMaxSubkey,	// address of buffer for longest subkey name length  
			&lpcchMaxClass,	// address of buffer for longest class string length 
			&lpcValues,	// address of buffer for number of value entries 
			&lpcchMaxValueName,	// address of buffer for longest value name length 
			&lpcbMaxValueData,	// address of buffer for longest value data length 
			&lpcbSecurityDescriptor,	// address of buffer for security descriptor length 
			&lpftLastWriteTime 	// address of buffer for last write time 
		   );	

		for (iValue = 0; Status == ERROR_SUCCESS; iValue++)
		{
			NameLen = sizeof(NameBuf);
			if ((Status = RegEnumKeyEx(
				hKey,	// handle of key to enumerate 
				iValue,	// index of subkey to enumerate 
				NameBuf,	// address of buffer for subkey name 
				&NameLen,	// address for size of subkey buffer 
				NULL,	// reserved 
				NULL,	// address of buffer for class string 
				NULL,	// address for size of class buffer 
				&ftLastWrite 	// address for time key last written to 
			   )) == ERROR_SUCCESS)
            {
                if ((Status = RegOpenKeyEx(hKey, NameBuf,
									  0L, KEY_READ, &hKeyCard)) == ERROR_SUCCESS)
                {
					char pszSrvName[256];
					LONG size = sizeof(pszSrvName);
					DWORD dwType;

					if ((Status = RegQueryValueEx(hKeyCard,	// handle of key to query 
						"ServiceName",	            // address of name of value to query 
						NULL,
						&dwType,
						pszSrvName,       // pointer to put buffer in
						&size)) == ERROR_SUCCESS)
                    {
                        strcpy(szNicList[iList++], pszSrvName);
                    } /* if RegQueryValue */
                    RegCloseKey(hKeyCard);
                } /* if RegOpenKeyEx */
            } /* if RegEnumKeyEx */
        } /* for iValue */

        RegCloseKey(hKey);
    } /* if RegOpenKeyEx */

	szNicList[iList][0] = '\0';
}

int
getnetconfcount(void)
{
	HKEY            hKey;
	DWORD           iValue, iCount = 0;
    LONG            Status;
	FILETIME        ftLastWrite;
	DWORD           NameLen;
	char            NameBuf[MAX_PATH];
	char			szKey[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey,
						  0L, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
		char    lpszClass[MAX_PATH];	// address of buffer for class string 
		DWORD   lpcchClass = sizeof(lpszClass);	// address of size of class string buffer 
		DWORD   lpcSubKeys;	// address of buffer for number of subkeys 
		DWORD   lpcchMaxSubkey;	// address of buffer for longest subkey name length  
		DWORD   lpcchMaxClass;	// address of buffer for longest class string length 
		DWORD   lpcValues;	// address of buffer for number of value entries 
		DWORD   lpcchMaxValueName;	// address of buffer for longest value name length 
		DWORD   lpcbMaxValueData;	// address of buffer for longest value data length 
		DWORD   lpcbSecurityDescriptor;	// address of buffer for security descriptor length 
		FILETIME  lpftLastWriteTime; 	// address of buffer for last write time 

        Status = RegQueryInfoKey (
			hKey,	// handle of key to query 
			lpszClass,	// address of buffer for class string 
			&lpcchClass,	// address of size of class string buffer 
			NULL,	// reserved 
			&lpcSubKeys,	// address of buffer for number of subkeys 
			&lpcchMaxSubkey,	// address of buffer for longest subkey name length  
			&lpcchMaxClass,	// address of buffer for longest class string length 
			&lpcValues,	// address of buffer for number of value entries 
			&lpcchMaxValueName,	// address of buffer for longest value name length 
			&lpcbMaxValueData,	// address of buffer for longest value data length 
			&lpcbSecurityDescriptor,	// address of buffer for security descriptor length 
			&lpftLastWriteTime 	// address of buffer for last write time 
		   );	

		for (iValue = 0; Status == ERROR_SUCCESS; iValue++)
		{
			NameLen = sizeof(NameBuf);

			if ((Status = RegEnumKeyEx(
				hKey,	// handle of key to enumerate 
				iValue,	// index of subkey to enumerate 
				NameBuf,	// address of buffer for subkey name 
				&NameLen,	// address for size of subkey buffer 
				NULL,	// reserved 
				NULL,	// address of buffer for class string 
				NULL,	// address for size of class buffer 
				&ftLastWrite 	// address for time key last written to 
			   )) == ERROR_SUCCESS)
            {
                iCount++;               
            } /* if RegEnumKeyEx */
        } /* for iValue */

        RegCloseKey(hKey);
    } /* if RegOpenKeyEx */

    return iCount;
}

void
getnetconfs(u_long* iplist)
{
    char szNicList[99][256];
    char szIpAddress[256];
    int i = 0, count = 0;

	GetNicList(szNicList);

    while (szNicList[i][0]) {
        if (GetAdapterIpAddress(szNicList[i], szIpAddress)) {
	        iplist[count++] = inet_addr(szIpAddress);
		}

		i++;
    }
}

#endif
