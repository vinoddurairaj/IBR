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
/*
 * aixcmn.c - port AIX system includes -
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#if !defined(AIXCMN_H) && defined(_AIX)
#define  AIXCMN_H


/* these are found on most platforms in <sys/param.h>: */
#if !defined(DEV_BSIZE)
#define DEV_BSIZE       512
#endif /* !defined(DEV_BSIZE) */

#if !defined(DEV_BSHIFT)
#define DEV_BSHIFT      9               /* log2(DEV_BSIZE) */
#endif /* !defined(DEV_BSHIFT) */

#if !defined(SECTORSIZE)
#define SECTORSIZE 512
#endif /* !defined(SECTORSIZE) */

#include <sys/param.h> /* MAXPATHLEN usage */

#include <values.h> /* MAXINT usage */

#endif /* !defined(AIXCMN_H) && defined(_AIX) */
