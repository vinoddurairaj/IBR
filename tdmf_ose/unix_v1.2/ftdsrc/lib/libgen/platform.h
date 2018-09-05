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
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * $Header: /cvs2/sunnyvale/tdmf_ose/unix_v1.2/ftdsrc/lib/libgen/platform.h,v 1.10 2011/11/21 22:13:23 proulxm Exp $
 *
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <sys/types.h>
#include <sys/stat.h>

#define STDIN STDIN_FILENO
#define STDOUT STDOUT_FILENO

#if defined(HPUX)
/*
 * 
 */
#include "bigints.h"
#include <sys/stat.h>

offset_t llseek(int filedes, offset_t offset, int whence);

int ftd_make_devices(struct stat *cbuf,struct stat *bbuf, char *raw, char *block, dev_t cdev, dev_t bdev); /* SS/IS activity */
int ftd_make_group(int lgnum, dev_t major_num, dev_t *dev_out);

#elif defined(SOLARIS) || defined(_AIX) || defined(linux)
#include <sys/types.h>
#if defined(linux)
#include <sys/stat.h>
#endif

/* same names, different args */
void ftd_make_devices(struct stat *cbuf, struct stat *bbuf, char *raw, char *block, dev_t dev); 
/* SS/IS activity */
void ftd_make_group(int lgnum, dev_t dev);

#endif

void ftd_delete_group(int lgnum);
int ftd_make_aix_links(char *rawpath, char *blkpath, int lgnum, int dtcnum);
void ftd_rm_aix_links(int lgnum, int dtcnum);
int ftd_odm_add(char *blksrcpath, int lgnum, int dtcnum, char * blockshort);
int ftd_odm_delete(int lgnum, int dtcnum, char * blockshort);

#endif /* _PLATFORM_H_ */

