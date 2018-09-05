/*
 * ftd_dev.h - FTD device interface
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

#ifndef _FTD_DEV_H
#define _FTD_DEV_H

#include "llist.h"
#include "ftd_config.h"
#include "ftd_port.h"
#include "ftdio.h"
#if defined(_WINDOWS)
#include "ftd_perf.h"
#endif

/* this assumes 32-bits per int */ 
#define WORD_BOUNDARY(x)  ((x) >> 5) 
#define SINGLE_BIT(x)     (1 << ((x) & 31))
#define TEST_BIT(ptr,bit) (SINGLE_BIT(bit) & *(ptr + WORD_BOUNDARY(bit)))
#define SET_BIT(ptr,bit)  (*(ptr + WORD_BOUNDARY(bit)) |= SINGLE_BIT(bit))

/* use natural integer bit ordering (bit 0 = LSB, bit 31 = MSB) */
#define START_MASK(x)     (((unsigned int)0xffffffff) << ((x) & 31))
#define END_MASK(x)       (((unsigned int)0xffffffff) >> (31 - ((x) & 31)))

/* device level statistics structure */
typedef struct ftd_dev_stat_s {
	int devid;					/* device id                             */
#if !defined(_WINDOWS)
	int				actual;		/* device actual (bytes)                 */
	int				effective;	/* device effective (bytes)              */
#else
	int				connection;
	ftd_uint64_t	actual;
	ftd_uint64_t	effective;
#endif
	int entries;				/* device entries                        */
	int entage;					/* time between BAB write and RMD recv   */
	int jrnentage;				/* time between jrnl write and mir write */
	off_t rsyncoff;				/* rsync sectors done                    */
	int rsyncdelta;				/* rsync sectors changed                 */
	float actualkbps;			/* kbps transfer rate                    */
	float effectkbps;			/* effective transfer rate w/compression */
	float pctdone;				/* % of refresh or backfresh complete    */
	int sectors;				/* # of sectors in bab                   */
	int pctbab;					/* pct of bab in use                     */
	double local_kbps_read;		/* ftd device kbps read                  */
	double local_kbps_written;	/* ftd device kbps written               */
	ftd_uint64_t	ctotread_sectors;
	ftd_uint64_t	ctotwrite_sectors;
} ftd_dev_stat_t;
				
/* rsync delta structure */
typedef struct ftd_dev_delta_s {
	int offset;					/* delta block offset          */
	int length;					/* delta block length                 */
} ftd_dev_delta_t;

/* ftd volume structure */
typedef struct ftd_dev_s {
	int devid;					/* device ID insofar as PMD/RMD com      */
	HANDLE devfd;				/* local device handle                */
	HANDLE ftddevfd;			/* ftd device handle                */
    int devsize;				/* device size (sectors)                 */
	int devbsize;				/* device block size (bytes)             */
	int devbshift;				/* device block size bit shift           */
	int no0write;				/* meta-dev flag - don't write sector 0  */
	dev_t ftdnum;				/* ftd device number - primary only      */
	dev_t num;					/* local device number              */
	int dbvalid;				/* number valid dirty bits in map        */
	int dbres;					/* dirty bit map resolution              */
	int dblen;					/* dirty bit map length                  */
	unsigned char *db;			/* high-res dirty bit map address		*/
	off_t dirtyoff;				/* start of dirty segment offset         */
	int dirtylen;				/* dirty segment length            */
	off_t rsyncoff;				/* sector read offset                    */
	off_t rsyncackoff;			/* ACKd sector offset                    */
	int rsynclen;				/* data read length - sectors            */
	int rsyncbytelen;			/* data read length - bytes              */
	int rsyncdelta;				/* total sectors transferred/written     */
	int rsyncdone;				/* rsync - done flag                     */
	char *rsyncbuf;				/* device data buffer address            */
	int sumcnt;					/* number of checksum lists              */
	int sumnum;					/* number of checksums in each list      */
	int sumoff;					/* device offset digest represents       */
	int sumlen;					/* device length digest represents       */
	int sumbuflen;				/* checksum digest buffer length         */
	char *sumbuf;				/* checksum digest data buffer  address    */
	ftd_dev_stat_t *statp;		/* device-level statistics               */
	LList *deltamap;			/* offsets/lengths for deltas            */
} ftd_dev_t;

extern ftd_dev_t *ftd_dev_create(void);
extern int ftd_dev_delete(ftd_dev_t *devp);
extern int ftd_dev_open(ftd_dev_cfg_t *cfgp, int lgnum, ftd_dev_t *devp,
	int modebits, int permisbits, int role);
extern int ftd_dev_ftd_open(ftd_dev_cfg_t *cfgp, ftd_dev_t *devp,
	int modebits, int permisbits, int role);
extern int ftd_dev_init(ftd_dev_t *devp, int devid,
	char *rdevname, char *devname, int size, int num);
extern int ftd_dev_meta(ftd_dev_cfg_t *devcfgp, ftd_dev_t *devp);
extern int ftd_dev_close(ftd_dev_t *devp);
extern int ftd_dev_ftd_close(ftd_dev_t *devp);

extern int ftd_dev_write(ftd_dev_t *devp, char *buf, 
	ftd_uint64_t offset, int length, 
	char *devname);

extern int ftd_dev_add(HANDLE ctlfd, HANDLE devfd, char *ps_name, char *group_name,
	int lgnum, char *rdevname, char *devname, int grpstarted,
	int regen_hrdb, int *devidx, unsigned int size, ftd_dev_info_t *devp);

extern int ftd_dev_rem(HANDLE ctlfd, HANDLE lgfd, int lgnum,
	u_char *lrdb, u_char *hrdb,
	char *ps_name, char *groupname, char *devname, int dev_num,
	int state, int ndevs, ftd_dev_info_t *dip, int silent);

#endif
