/*
 * disksize.c -- FTD miscellaneous routines
 *
 * (c) Copyright 1999 Legato Systems, Inc. All Rights Reserved
 *
 */

#if defined (_WINDOWS)
//typedef long daddr_t;
#endif

/* external prototypes */
extern daddr_t disksize(char *fn);

#if defined(_WINDOWS)
extern int ftd_get_dev_bsize(HANDLE fd);
//
// SVG 30-05-03
//
#ifndef NEW_DISK_SIZE_METHOD
#pragma message ("Old disk size method")
extern long fdisksize(HANDLE fd);
#else
extern long fdisksize(char * szDir, HANDLE fd);
#endif
extern daddr_t disksize(char *fn);
#endif

