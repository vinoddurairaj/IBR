/*
 * version.c - FTD version number
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

#ifdef VERSION
#if defined(_AIX) || defined(_WINDOWS)
volatile char qdsreleasenumber[] = PRODUCTNAME" Release Version " VERSION BLDSEQNUM;
#else /* defined(_AIX) */
char qdsreleasenumber[] = PRODUCTNAME" Release Version " VERSION BLDSEQNUM;
#endif /* defined(_AIX) */
#endif
