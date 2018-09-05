/*
 * ftd_bits.h 
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#ifndef _FTD_BITS_H_
#define _FTD_BITS_H_

#include "ftd_def.h"

/* this assumes 32-bits per int */
#define WORD_BOUNDARY(x)  ((x) >> 5)
#define SINGLE_BIT(x)     (1 << ((x) & 31))
#define TEST_BIT(ptr,bit) (SINGLE_BIT(bit) & *(ptr + WORD_BOUNDARY(bit)))
#define SET_BIT(ptr,bit)  (*(ptr + WORD_BOUNDARY(bit)) |= SINGLE_BIT(bit))

/* use natural integer bit ordering (bit 0 = LSB, bit 31 = MSB) */
#define START_MASK(x)     (((ftd_uint32_t)0xffffffff) << ((x) & 31))
#define END_MASK(x)       (((ftd_uint32_t)0xffffffff) >> (31 - ((x) & 31)))

/* for a couple of functions that work on both dirtybit arrays */
#define FTD_HIGH_RES_DIRTYBITS 0
#define FTD_LOW_RES_DIRTYBITS  1

#endif /* _FTD_BITS_H_ */
