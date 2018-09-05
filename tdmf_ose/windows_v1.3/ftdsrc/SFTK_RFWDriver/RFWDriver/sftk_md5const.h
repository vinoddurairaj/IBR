/*
 * md5const.h - MD5 construction/destruction	
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

#ifndef	_MD5CONST_H
#define	_MD5CONST_H

#include "sftk_md5.h"

/* added Create, Delete prototypes */
extern MD5_CTX *MD5Create PROTO_LIST ((void));
extern int MD5Delete PROTO_LIST ((MD5_CTX *context));

#endif
