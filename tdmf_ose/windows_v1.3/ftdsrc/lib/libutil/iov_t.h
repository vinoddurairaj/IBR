/*
 * iov_t.h - struct iovec definition for NT  
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

#ifndef _IOV_T_H_
#define _IOV_T_H_

typedef	char	*caddr_t;
typedef struct iovec {
	caddr_t	iov_base;
	int	iov_len;
} iovec_t;

#endif
