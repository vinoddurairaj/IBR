/*
 * ftd_ioctl.h - FTD driver interface
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

#ifndef _FTD_IOCTL_H_
#define _FTD_IOCTL_H_

#include "ftdio.h"

/* external prototypes */
extern int ftd_ioctl_get_bab_size(HANDLE ctlfd, int *size);
extern int ftd_ioctl_update_lrdbs(HANDLE ctlfd, int lgnum, int silent);
extern int ftd_ioctl_update_hrdbs(HANDLE ctlfd, int lgnum, int silent);
extern int ftd_ioctl_set_group_state(HANDLE ctlfd, int lgnum, int state);
extern int ftd_ioctl_get_group_state(HANDLE ctlfd, int lgnum);
extern int ftd_ioctl_get_group_stats(HANDLE ctlfd, int lgnum, ftd_stat_t *lgp, int silent);
extern int ftd_ioctl_set_lrdbs(HANDLE ctlfd, HANDLE fd, int lgnum, dirtybit_array_t *db, ftd_dev_info_t *dip, int ndevs);
extern int ftd_ioctl_get_lrdbs(HANDLE ctlfd, HANDLE fd, int lgnum, dirtybit_array_t *db, ftd_dev_info_t *dip);
extern int ftd_ioctl_set_hrdbs(HANDLE ctlfd, HANDLE fd, int lgnum, dirtybit_array_t *db, ftd_dev_info_t *dip, int ndevs);
extern int ftd_ioctl_get_hrdbs(HANDLE ctlfd, HANDLE fd, int lgnum, dirtybit_array_t *db, ftd_dev_info_t *dip);
extern int ftd_ioctl_set_lrdbs_from_buf(HANDLE ctlfd, HANDLE fd, int lgnum, dev_t *dev, int *lrdb, int len);
extern int ftd_ioctl_set_hrdbs_from_buf(HANDLE ctlfd, HANDLE fd, int lgnum, dev_t *dev, int *hrdb, int len);
extern int ftd_ioctl_clear_bab(HANDLE ctlfd, int lgnum);
extern int ftd_ioctl_clear_lrdbs(HANDLE ctlfd, int lgnum, int silent);
extern int ftd_ioctl_clear_hrdbs(HANDLE ctlfd, int lgnum, int silent);
extern int ftd_ioctl_set_iodelay(HANDLE ctlfd, int lgnum, int delay);
extern int ftd_ioctl_set_sync_depth(HANDLE ctlfd, int lgnum, int sync_depth);
extern int ftd_ioctl_set_sync_timeout(HANDLE ctlfd, int lgnum, int sync_timeout);
extern int ftd_ioctl_set_lg_state_buffer(HANDLE ctlfd, int lgnum, char *buf);
extern int ftd_ioctl_get_lg_state_buffer(HANDLE ctlfd, int lgnum,
	char *buf, int silent);
extern int ftd_ioctl_set_dev_state_buffer(HANDLE ctlfd, int lgnum, int devnum, int buflen, char *buf, int silent);
extern int ftd_ioctl_get_dev_state_buffer(HANDLE ctlfd, int lgnum, int devnum, int buflen, char *buf, int silent);
extern int ftd_ioctl_get_groups_info(HANDLE ctlfd, int lgnum, ftd_lg_info_t *lgp, int silent);
extern int ftd_ioctl_get_devices_info(HANDLE ctlfd, int lgnum, ftd_dev_info_t *dip);
extern int ftd_ioctl_oldest_entries(HANDLE devfd, oldest_entries_t *oe);
extern int ftd_ioctl_send_lg_message(HANDLE devfd, int lgnum, char *sentinel);
extern int ftd_ioctl_del_lg(HANDLE ctlfd, int lgdev, int silent);
extern int ftd_ioctl_del_device(HANDLE ctlfd, int devnum);
extern int ftd_ioctl_get_device_nums(HANDLE ctlfd, ftd_devnum_t *devnum);
extern int ftd_ioctl_new_lg(HANDLE ctlfd, ftd_lg_info_t *info, int silent);
extern int ftd_ioctl_new_device(HANDLE ctlfd, ftd_dev_info_t *info,
	int silent);
extern int ftd_ioctl_migrate(HANDLE ctlfd, int lgnum, int bytes);
extern int ftd_ioctl_get_dev_stats(HANDLE ctlfd, int lgnum, int devnum, disk_stats_t *DiskInfo, int nDiskStats);
extern int ftd_ioctl_init_stop(HANDLE devfd, int lgnum, int silent);
extern int ftd_ioctl_set_trace_level(HANDLE ctlfd, int level);

// Added By Saumya Tripathi 08/23/04
extern ULONG Sftk_Get_TotalLgCount(HANDLE ctlfd);
extern ULONG Sftk_Get_TotalDevCount(HANDLE ctlfd);
extern ULONG Sftk_Get_TotalLgDevCount(HANDLE ctlfd, ULONG LgNum);
extern DWORD Sftk_Get_AllStatsInfo( PALL_LG_STATISTICS All_LgStats, ULONG Size );
DWORD sftk_DeviceIoControl(	IN	HANDLE	HandleDevice,
						IN	PCHAR	DeviceName,
						IN	DWORD	IoControlCode,
						IN	LPVOID	InBuffer,
						IN	DWORD	InBufferSize,
						OUT LPVOID	OutBuffer,
						IN	DWORD	OutBufferSize,
						OUT LPDWORD	BytesReturned);

// SAUMYA_FIX_INITIALIZATION_MODULE
#if 0
extern int sftk_ioctl_config_begin(); 
extern int sftk_ioctl_config_end();
extern int sftk_ioctl_add_connections();
extern int sftk_ioctl_remove_connections();
extern int sftk_ioctl_enable_connections();
extern int sftk_ioctl_disable_connections();
extern int sftk_ioctl_launch_pmd();
extern int sftk_ioctl_stop_all_connections();
extern int sftk_ioctl_query_connections();
#endif// SAUMYA_FIX_INITIALIZATION_MODULE

#endif

