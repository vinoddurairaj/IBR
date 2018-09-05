/*
 * ftd_devio.h - FTD device i/o interface
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

#ifndef _FTD_DEVIO_H
#define _FTD_DEVIO_H

#if defined(_WINDOWS)


#define F_TLOCK 0
#define F_ULOCK 1

#define BLOCK_SIZE(a)  ( ((a >> DEV_BSHIFT) + 1) << DEV_BSHIFT )

/*
 * I/O parameter information.  A uio structure describes the I/O which
 * is to be performed by an operation.  Typically the data movement will
 * be performed by a routine such as uiomove(), which updates the uio
 * structure to reflect what was done.
 */

#include "iov_t.h"

typedef signed int ssize_t;

extern HANDLE ftd_open(char *device, int AccessFlags, int Permissions);
extern int ftd_ioctl(HANDLE hndFile, int ioctl, void *buf, int size);
extern ftd_uint64_t ftd_llseek(HANDLE hndFile, ftd_uint64_t offset, int origin);
extern int ftd_lockf(HANDLE hndFile, int ioctl, ftd_uint64_t offset, ftd_uint64_t length);
extern int ftd_fsync(HANDLE hndFile);
extern int ftd_write(HANDLE hndFile, void *buffer, unsigned int count);
extern int ftd_read(HANDLE hndFile, void *buffer, unsigned int count);
#ifdef TDMF_TRACE
	extern int ftd_close(char *mod,int line,HANDLE hndFile);
	#define FTD_CLOSE_FUNC(a,b,c) 	ftd_close(a,b,c)
#else
	extern int ftd_close(HANDLE hndFile);
	#define FTD_CLOSE_FUNC(a,b,c) 	ftd_close(c)
#endif


extern ssize_t ftd_writev(HANDLE hndFile, const struct iovec *iov, int size);
extern void force_dsk_or_rdsk (char* pathout, char* pathin, int tordskflag);

#else

extern int ftd_open(char *name, int mode, int permis);
extern ftd_uint64_t ftd_llseek(int fd, ftd_uint64_t offset, int whence);
extern int ftd_read(int fd, void *buf, int len);
extern int ftd_write(int fd, void *buf, int len);
extern ssize_t ftd_writev(int fd, struct iovec *iov, int iovcnt);
extern int ftd_close(int fd);
extern int ftd_fsync(int fd);
extern int ftd_lockf(int fd, int ioctl, ftd_uint64_t offset, ftd_uint64_t len);
extern int ftd_ioctl(int fd, int ioctl, void *buf, int len);

#endif /* _WINDOWS */

#if !defined(SOCKET)
#define	SOCKET	int
#endif

#if defined(HPUX)
extern int dev_is_logical_vol(ftd_dev_t *devp);
extern int dev_info(char *devname, int *inuseflag, unsigned long *sectors, char *mntpnt);
extern int capture_logical_volumes (int fd);
#endif /* HPUX */

extern void capture_devs_in_dir (int fd, char* cdevpath, char* bdevpath);
extern void walk_rdsk_subdirs_for_devs (int fd, char* rootdir);
extern void walk_dirs_for_devs (int fd, char* rootdir);
extern void capture_all_devs (SOCKET fd);
extern int dev_proc_info_request (int fd, char* cmd);

#endif
