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
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <sys/types.h>

#define STDIN STDIN_FILENO
#define STDOUT STDOUT_FILENO

#if defined(HPUX)
/*
 * 
 */
#include "bigints.h"

offset_t llseek(int filedes, offset_t offset, int whence);

int ftd_make_devices(char *raw, char *block, dev_t raw_major_num, 
                     dev_t block_major_num, int minor_num, dev_t *raw_dev_out, 
                     dev_t *block_dev_out);
int ftd_make_group(int lgnum, dev_t major_num, dev_t *dev_out);

#elif defined(SOLARIS) || defined(_AIX)
#include <sys/types.h>

/* same names, different args */
void ftd_make_devices(char *raw, char *block, dev_t dev);
void ftd_make_group(int lgnum, dev_t dev);

#endif

void ftd_delete_group(int lgnum);

#endif /* _PLATFORM_H_ */

