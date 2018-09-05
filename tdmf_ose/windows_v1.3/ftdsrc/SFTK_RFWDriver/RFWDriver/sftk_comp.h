/*
 * comp.h - data compression interface
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

#ifndef _COMP_H_
#define _COMP_H_

#include "sftk_lzhl.h"
#include "sftk_pred.h"

#define PRED			1
#define LZHL			2

#define COMPMAGIC		0xBADD00D1

#define ERRCOMPMAGIC	-100

typedef struct comp_s {
	int magicvalue;
	int algorithm;
	pred_t pred;
	lzhl_t lzhl;
} comp_t;

#ifdef __cplusplus
extern "C"{ 
#endif

extern comp_t *comp_create(int algorithm);
extern int comp_delete(comp_t *comp);
extern int comp_compress(unsigned char *target, size_t *targetlen, unsigned char *src, size_t srclen, comp_t *comp);
extern int comp_decompress(unsigned char *target, size_t *targetlen, unsigned char *src, size_t *srclen, comp_t *comp);
extern int comp_init(comp_t *comp);

#ifdef __cplusplus 
}
#endif

#endif
