/*
 * ftd_sock_t.h - FTD socket object structure
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
#ifndef _FTD_SOCK_T_H
#define _FTD_SOCK_T_H

#include "sock.h"

#define FTDSOCKMAGIC	0xBADF00D3

#define	FTD_SOCK_GENERIC		1
#define	FTD_SOCK_INET			2

typedef struct ftd_sock_s {
	int magicvalue;					/* so we know it's been initialized */
	int keepalive;					/* don't destroy if TRUE */
	int contries;					/* peer connect tries */
	int type;						/* connect type: GENERIC, LG */
	sock_t *sockp;					/* socket object address */
} ftd_sock_t;

#endif

