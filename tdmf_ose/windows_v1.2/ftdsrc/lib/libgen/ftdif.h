/***************************************************************************
 * ftdif.h - Common driver code interface
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 ***************************************************************************
 */

#ifndef _FTDIF_H
#define _FTDIF_H 1

extern int set_dirtybits(int ctlfd, int fd, int lgnum, dirtybit_array_t *db, 
    ftd_dev_info_t **dip);
extern int get_dirtybits(int, int, int, dirtybit_array_t*, ftd_dev_info_t **);
extern int flush_bab(int ctlfd, int lgnum);
extern int set_iodelay(int ctlfd, int lgnum, int delay);
extern int set_sync_depth(int ctlfd, int lgnum, int sync_depth);
extern int set_sync_timeout(int ctlfd, int lgnum, int sync_timeout);

#endif /* _FTDIF_H */

