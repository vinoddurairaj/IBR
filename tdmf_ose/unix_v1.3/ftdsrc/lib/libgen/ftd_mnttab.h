/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * /usr/include/sys/mnttab.h from Solaris 2.5. Needed for emulation of
 * getmntany(). See man page on Sun for more info. 
 */
#ifndef _FTD_MNTTAB_H
#define	_FTD_MNTTAB_H

#include <stdio.h>

#ifdef	__cplusplus
extern "C" {
#endif

struct mnttab {
	char	*mnt_special;
	char	*mnt_mountp;
	char	*mnt_fstype;
	char	*mnt_mntopts;
	char	*mnt_time;
};

extern int	getmntany(FILE *, struct mnttab *, struct mnttab *);

#ifdef	__cplusplus
}
#endif

#endif	/* _FTD_MNTTAB_H */
