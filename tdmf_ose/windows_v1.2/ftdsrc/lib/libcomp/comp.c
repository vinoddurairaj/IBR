/*
 * comp.c - data compression interface
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

#include "comp.h"

comp_t *
comp_create(int algorithm)
{
	comp_t *comp;

	if ((comp = (comp_t*)calloc(1, sizeof(comp_t))) == NULL) {
		return NULL;
	}

	switch(algorithm) {
	case LZHL:
		comp->algorithm = LZHL;
		break;
	case PRED:
		comp->algorithm = PRED;
		break;
	default:
		break;
	}
	comp->magicvalue = COMPMAGIC;
	comp_init(comp);

	return comp;
}

int
comp_delete(comp_t *comp)
{
	lzhl_compress_free(&comp->lzhl);

	free(comp);

	return 0;
}

int
comp_compress(unsigned char *target, size_t *targetlen,
	unsigned char *src, size_t srclen, comp_t *comp)
{
	int iCompLen;

	if (comp->magicvalue != COMPMAGIC) {
		return ERRCOMPMAGIC;
	}
	
	switch(comp->algorithm) {
    case PRED:
		comp->pred.src = src;
		comp->pred.srclen = srclen;
		comp->pred.target = target;
		comp->pred.targetlen = *targetlen;

		iCompLen = pred_compress(&comp->pred);

		// predictor compression could change buffer
		// address and length
		target = comp->pred.target;
		*targetlen = comp->pred.targetlen;

		return(iCompLen);
     case LZHL:
		comp->lzhl.src = src;
		comp->lzhl.srclen = srclen;
		comp->lzhl.target = target;
		comp->lzhl.targetlen = *targetlen;

		iCompLen = lzhl_compress(&comp->lzhl);

		return(iCompLen);
    default:
        return 0;
    }
}

int
comp_decompress(unsigned char *target, size_t *targetlen,
	unsigned char *src, size_t *srclen, comp_t *comp)
{
	int iDecompLen;

	if (comp->magicvalue != COMPMAGIC) {
		return ERRCOMPMAGIC;
	}

    switch(comp->algorithm) {
    case PRED:
		comp->pred.src = src;
		comp->pred.srclen = *srclen;
		comp->pred.target = target;
		comp->pred.targetlen = *targetlen;

		iDecompLen = pred_decompress(&comp->pred);

		return(iDecompLen);
    case LZHL:
		comp->lzhl.src = src;
		comp->lzhl.srclen = *srclen;
		comp->lzhl.target = target;
		comp->lzhl.targetlen = *targetlen;

		iDecompLen = lzhl_decompress(&comp->lzhl);

		return(iDecompLen);
    default:
        return 0;
    }
}

int
comp_init(comp_t *comp) 
{

    if (pred_compress_init(&comp->pred) == -1) {
        return -1;
    }
    if (lzhl_compress_init(&comp->lzhl) == -1) {
        return -1;
    }

    return 0;
}

