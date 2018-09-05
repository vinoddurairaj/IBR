/*
 * md5const.c - MD5 construction/destruction	
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
#include	<stdlib.h>
#include	"md5.h"
#include	"md5const.h"

MD5_CTX *MD5Create (void)
{
	MD5_CTX *context;

	if ((context = (MD5_CTX*)calloc(1, sizeof(MD5_CTX))) == NULL) {
		return NULL;
	}

	return context;
}

int MD5Delete (MD5_CTX *context)
{

	free(context);

	return 0;
}

