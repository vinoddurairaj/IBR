/*
 * comp.h - data compression interface
 * 
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef _COMPRESS_H_
#define _COMPRESS_H_

#include "lzhl.h"
#include "pred.h"

#define PRED	1
#define LZHL	2

typedef struct comp_s {
	int algorithm;			/* type of compression algorithm employed */
	unsigned char *cp;		/* address of compression state table */
} comp_t;

extern comp_t *comp_create(int algorithm);
extern int comp_delete(comp_t *compp);
extern int comp_compress(unsigned char *target, size_t *targetlen, unsigned char *src, size_t srclen, int algorithm);
extern int comp_decompress(unsigned char *, size_t*, BYTE*, size_t*, int);
extern int comp_init(comp_t *compp);

#endif
