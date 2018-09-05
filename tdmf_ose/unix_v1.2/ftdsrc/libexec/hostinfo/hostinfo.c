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
/***************************************************************************
 * hostinfo.c - Prints information about node, IP addresses, and hostid
 *
 * (c) Copyright 1996, 1997 FullTime Software, Inc. All Rights Reserved
 *
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "licplat.h"
#include "network.h"
#include "common.h"

#ifdef TDMF_TRACE
FILE *dbgfd;
#endif


int
main(int argc, const char **argv)
{
#if defined(FTD_IPV4)
    u_long addr;
    struct hostent *hp;
    char **p;
    char hostname[1024];
    int  hostid;
    int local;
#else
    u_long addr;
    char hostname[1024];
    int hostid;
    int local;
    int err;
    int p4flag = 0, p6flag = 0;
    struct addrinfo hints,*res,*ap; 
    char host[NI_MAXHOST];
    char buf6[INET6_ADDRSTRLEN];
    char buf[INET_ADDRSTRLEN];
#endif 
    

    putenv("LANG=C");

    /* Make sure we are root */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }

    local = 0;
    if (argc > 2 || (argc == 2 && 0 == strncmp("-help", argv[1], 2))) {
        (void) printf("usage: %s\n", argv[0]);
        (void) printf("usage: %s hostname\n", argv[0]);
/*        (void) printf("usage: %s -i ipaddr\n", argv[0]);*/
        exit (1);
    }
    (void) gethostname (hostname, sizeof(hostname));

#if defined(FTD_IPV4)
    hp = gethostbyname(hostname);
    if (hp == NULL) {
      fprintf (stderr, "no host information found\n");
      exit (3);
    }
    strcpy(hostname, hp->h_name);

    if (argc == 1) {
      ;
    } else if (argc == 2) {
      hp = gethostbyname(argv[1]);
      if (hp == NULL) {
        (void) printf("host information for %s not found\n", argv[1]);
        exit (3);
      }
     } else {
      if ((int)(addr = inet_addr(argv[2])) == -1) {
        (void) printf("IP-address must be of the form a.b.c.d\n");
        exit (2);
      }
      hp = gethostbyaddr((char *)&addr, sizeof (addr), AF_INET);
    }

    if (hp == NULL) {
      fprintf (stderr, "no host information found\n");
      exit (3);
    }
    hostid = my_gethostid (HOSTID_LICENSE);
    for (p = hp->h_addr_list; *p != 0; p++) {
        struct in_addr in;
        char **q;

        (void) memcpy(&in.s_addr, *p, sizeof (in.s_addr));
        (void) printf("%s\t%s", inet_ntoa(in), hp->h_name);
        if (0 == strcmp(hp->h_name, hostname)) local = 1;
        for (q = hp->h_aliases; *q != 0; q++) {
            if (0 == strcmp(*q, hostname)) local = 1;
            (void) printf(" %s", *q);
        }
        (void) putchar('\n');
    }
#else
    memset (&hints, 0, sizeof (hints));
    hints.ai_flags = AI_CANONNAME;   
    err = getaddrinfo(hostname, NULL, &hints, &res);

    if (err != 0) {
       fprintf (stderr, " no host information found\n");
       exit (3);
    }
    strcpy(hostname, res->ai_canonname);
    if (argc == 1) {
      ;
    } else if (argc == 2) {
       freeaddrinfo(res);
       memset (&hints, 0, sizeof (hints));
       hints.ai_flags = AI_CANONNAME;   
       err = getaddrinfo(argv[1], NULL, &hints, &res);
       if (err != 0) {
          (void) printf("host information for %s not found\n", argv[1]);
          exit (3);
       }
    }
    hostid = my_gethostid (HOSTID_LICENSE);
    if (0 == strcmp(res->ai_canonname, hostname)) 
      local = 1;
    printf("Hostname           : %s \t\n", res->ai_canonname);

   for (ap=res;ap!=NULL; ap=ap->ai_next)
   {
      if ((ap->ai_family == AF_INET) && (!p4flag)) {
         (void) printf("IPv4 Address       : %s\t\n", \
                  inet_ntop(AF_INET, &((struct sockaddr_in *)ap->ai_addr)->sin_addr,buf,sizeof(buf)));
         p4flag = 1;
      }
      else if ((ap->ai_family == AF_INET6) &&  (!p6flag)) {
         (void) printf("IPv6 Address       : %s\t\n", \
                    inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ap->ai_addr)->sin6_addr,buf6,sizeof(buf6)));
         p6flag = 1;
      }
   }
   freeaddrinfo(res);
#endif /* defined(FTD_IPV4) */
   if (local) fprintf (stdout, "Hostid for license : 0x%08x\n", hostid);
   if (local) fprintf (stdout, "Hostid for agent   : 0x%08x\n", my_gethostid(HOSTID_IDENTIFICATION));
   
   return 0;
}
