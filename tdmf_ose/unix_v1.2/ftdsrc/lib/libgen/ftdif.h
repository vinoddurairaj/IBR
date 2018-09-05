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
/***************************************************************************
 * ftdif.h - Common driver code interface
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 ***************************************************************************
 */

#ifndef _FTDIF_H
#define _FTDIF_H 1

extern int get_dirtybits(int, int, int, dirtybit_array_t*, ftd_dev_info_t **);
extern int flush_bab(int ctlfd, int lgnum);
extern int set_sync_depth(int ctlfd, int lgnum, int sync_depth);
extern int set_sync_timeout(int ctlfd, int lgnum, int sync_timeout);
extern int get_bab_size(int ctlfd);
extern int set_lrt_bits(int ctlfd, int lgnum);
extern int set_lrt_mode(int ctlfd, int lgnum);

#endif /* _FTDIF_H */

