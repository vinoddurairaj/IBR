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
extern long fdisksize(char * szDir, HANDLE fd);
extern daddr_t disksize(char *fn);
#endif

