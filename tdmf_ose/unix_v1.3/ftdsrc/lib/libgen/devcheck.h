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

#ifndef _DEVCHECK_H_
#define _DEVCHECK_H_

void force_dsk_or_rdsk (char* pathout, char* pathin, int tordskflag);

#if defined(HPUX)
void convert_lv_name (char *pathout, char *pathin, int to_char);
int is_logical_volume (char *devname);
int dev_info(char *devname, int *inuseflag, u_long *sectors, char *mntpnt);
int capture_logical_volumes (int fd);
#endif /* HPUX */

void capture_devs_in_dir (int fd, char* cdevpath, char* bdevpath);
void walk_rdsk_subdirs_for_devs (int fd, char* rootdir);
void walk_dirs_for_devs (int fd, char* rootdir);
void capture_all_devs (int fd);
int process_dev_info_request (int fd, char* cmd);

#endif /* _DEVCHECK_H_ */
