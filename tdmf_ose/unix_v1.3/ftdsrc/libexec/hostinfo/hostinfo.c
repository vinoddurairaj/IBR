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

#ifdef TDMF_TRACE
FILE *dbgfd;
#endif

int
main(int argc, const char **argv)
{
    u_long addr;
    struct hostent *hp;
    char **p;
    char hostname[256];
    long hostid;
    int local;

    local = 0;
    if (argc > 3 || (argc == 2 && 0 == strncmp("-help", argv[1], 2))) {
        (void) printf("usage: %s\n", argv[0]);
        (void) printf("usage: %s hostname\n", argv[0]);
        (void) printf("usage: %s -i ipaddr\n", argv[0]);
        exit (1);
    }
    if (argc == 1) {
      (void) gethostname (hostname, 256);
      hp = gethostbyname(hostname);
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
    (void) gethostname (hostname, 256);
    hostid = my_gethostid ();
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
    if (local) fprintf (stdout, "hostid: 0x%08lx\n", hostid);
    exit (0);
}
