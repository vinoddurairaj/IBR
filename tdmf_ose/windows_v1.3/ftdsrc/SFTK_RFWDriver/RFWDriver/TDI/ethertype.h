/*
 * Copyright (c) 1993, 1994, 1996
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#) $Header: /cvs2/sunnyvale/tdmf_ose/windows_v1.3/ftdsrc/SFTK_RFWDriver/RFWDriver/TDI/ethertype.h,v 1.1 2004/07/16 04:15:42 pqsj0 Exp $ (LBL)
 */

/* Types missing from some systems */

#ifndef ETHERTYPE_PUP
#define	ETHERTYPE_PUP		0x0200	/* PUP protocol */
#endif
#ifndef ETHERTYPE_IP
#define	ETHERTYPE_IP		0x0800	/* IP protocol */
#endif
#ifndef ETHERTYPE_ARP
#define ETHERTYPE_ARP		0x0806	/* Addr. resolution protocol */
#endif
#ifndef ETHERTYPE_NS
#define ETHERTYPE_NS		0x0600
#endif
#ifndef	ETHERTYPE_SPRITE
#define	ETHERTYPE_SPRITE	0x0500
#endif
#ifndef ETHERTYPE_TRAIL
#define ETHERTYPE_TRAIL		0x1000
#endif
#ifndef	ETHERTYPE_DECEXP
#define	ETHERTYPE_DECEXP		0x6000
#endif
#ifndef	ETHERTYPE_MOPDL
#define	ETHERTYPE_MOPDL		0x6001
#endif
#ifndef	ETHERTYPE_MOPRC
#define	ETHERTYPE_MOPRC		0x6002
#endif
#ifndef	ETHERTYPE_DN
#define	ETHERTYPE_DN		0x6003
#endif
#ifndef	ETHERTYPE_LAT
#define	ETHERTYPE_LAT		0x6004
#endif
#ifndef ETHERTYPE_SCA
#define ETHERTYPE_SCA		0x6007
#endif
#ifndef ETHERTYPE_REVARP
#define ETHERTYPE_REVARP	0x8035
#endif
#ifndef	ETHERTYPE_LANBRIDGE
#define	ETHERTYPE_LANBRIDGE	0x8038
#endif
#ifndef	ETHERTYPE_DECDNS
#define	ETHERTYPE_DECDNS	0x803c
#endif
#ifndef	ETHERTYPE_DECDTS
#define	ETHERTYPE_DECDTS	0x803e
#endif
#ifndef	ETHERTYPE_VEXP
#define	ETHERTYPE_VEXP		0x805b
#endif
#ifndef	ETHERTYPE_VPROD
#define	ETHERTYPE_VPROD		0x805c
#endif
#ifndef ETHERTYPE_ATALK
#define ETHERTYPE_ATALK		0x809b
#endif
#ifndef ETHERTYPE_AARP
#define ETHERTYPE_AARP		0x80f3
#endif
#ifndef	ETHERTYPE_LOOPBACK
#define	ETHERTYPE_LOOPBACK	0x9000
#endif

