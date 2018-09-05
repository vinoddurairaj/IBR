/*
 * ftd_hostinfo.c - dump host info to stdout 
 *
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 *
 */
#if defined(_WINDOWS) && defined(_DEBUG)
#include <conio.h>
#endif

#include "ftd_port.h"
#include "licplat.h"
#include "sock.h"

int
main(int argc, const char **argv)
{
    u_long addr;
    struct hostent *hp;
    char **p;
    char hostname[256];
    long hostid;
    int local, errnum;
    int exitcode = 1;//assume error

////------------------------------------------
#if defined(_WINDOWS)
	void ftd_util_force_to_use_only_one_processor();
	ftd_util_force_to_use_only_one_processor();
#endif
//------------------------------------------

    local = 0;
    if (argc > 3 || (argc == 2 && 0 == strncmp("-help", argv[1], 2))) {
        (void) printf("usage: %s\n", argv[0]);
        (void) printf("usage: %s hostname\n", argv[0]);
        (void) printf("usage: %s -i ipaddr\n", argv[0]);
        goto errexit;
    }
#if defined(_WINDOWS)
    (void)tcp_startup();
#endif

	if (argc == 1) {
      (void) gethostname (hostname, 256);
      hp = gethostbyname(hostname);
    } else if (argc == 2) {
      hp = gethostbyname(argv[1]);
      if (hp == NULL) {
#if defined(_WINDOWS)
		errnum = WSAGetLastError();
#else
		errnum = errno;
#endif
        (void) printf("host information for %s not found: (errno = %d)\n",
			argv[1], errnum);
        goto errexit;
      }
    } else {
      if ((int)(addr = inet_addr(argv[2])) == -1) {
        (void) printf("IP-address must be of the form a.b.c.d\n");
        goto errexit;
      }
      hp = gethostbyaddr((char *)&addr, sizeof (addr), AF_INET);
    }

    if (hp == NULL) {
      fprintf (stderr, "no host information found\n");
      goto errexit;
    }
    (void) gethostname (hostname, 256);
    hostid = my_gethostid ();
    for (p = hp->h_addr_list; *p != 0; p++) {
        struct in_addr in;
        char **q;

        (void) memcpy(&in.s_addr, *p, sizeof (in.s_addr));
        (void) printf("%s\t%s", inet_ntoa(in), hp->h_name);
        if (0 == strncmp(hp->h_name, hostname, strlen(hostname))) local = 1;
        for (q = hp->h_aliases; *q != 0; q++) {
            if (0 == strcmp(*q, hostname)) local = 1;
            (void) printf(" %s", *q);
        }
        (void) putchar('\n');
    }
    if (local) fprintf (stdout, "hostid: 0x%08lx\n", hostid);

    exitcode = 0;//success

errexit:
    
#if defined(_WINDOWS)
	(void)tcp_cleanup();
#endif

	exit (exitcode);
}
