/*
 * iputil.h - IP utilities
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

/* external prototypes */
extern int name_is_ipstring(char *name);
extern int getnetconfcount(void);
extern void getnetconfs(unsigned long *iplist);
extern int name_to_ip(char *name, unsigned long *ip);
extern int ipstring_to_ip(char *ipstring, unsigned long *ip);
extern int ip_to_name(unsigned long ip, char *name);
extern int ip_to_ipstring(unsigned long ip, char *ipstring);

