/*
 *  LZH-Light algorithm implementation v 1.0
 *  Copyright (C) Sergey Ignatchenko 1998
 *  This  software  is  provided  "as is"  without  express  or implied 
 *  warranty.
 *
 *  Permission to use, copy, modify, distribute and sell this  software 
 *  for any purpose is hereby  granted  without  fee,  subject  to  the 
 *  following restrictions:
 *  1. this notice may not be removed or altered;
 *  2. altered source versions must be plainly marked as such, and must 
 *     not be misrepresented as being the original software.
 *
 *  Tom Mills - 10/98 - converted from C++ to C
 */

#ifndef LZHLINTERNAL
#error This file is for LZHL internal use only
#endif

#include "sftk__lzhl.h"

#define HUFFRECALCSTAT( s ) ( (s) >> 1 )

#define HUFFINT   INT16
#define HUFFUINT UINT16
#define NHUFFSYMBOLS ( 256 + 16 + 2 )

typedef struct huffstattmpstruct_s {
    HUFFINT i;
    HUFFINT n;
} huffstattmpstruct_t;

typedef struct symbol_s {
    HUFFINT nBits;
    HUFFUINT code;
} symbol_t;

typedef struct lzhl_encoderstat_s {
    symbol_t *symbolTable;
    HUFFINT *stat;
    int nextStat;
} lzhl_encoderstat_t;

typedef struct lzhl_encoder_s {
    lzhl_encoderstat_t* stat;
    int maxMatchOver;
    int maxRaw;
    BYTE *dst;
    BYTE *dstBegin;
    UINT32 bits;
    int nBits;
    HUFFINT *sstat;
} lzhl_encoder_t;

typedef struct grp_s {
    int nBits;
    int pos;
} grp_t;

typedef struct lzhl_decoderstat_s {
    HUFFINT *stat; 
    grp_t groupTable[16];
    HUFFINT *symbolTable;
} lzhl_decoderstat_t;

typedef struct lzhl_decoder_s {
    lzhl_decoderstat_t *stat;
    UINT32 bits;
    int nBits;
} lzhl_decoder_t;

typedef struct encode_matchoveritem_s {
    int symbol;
    int nBits;
    UINT16 bits;
} encode_matchoveritem_t;

typedef struct decode_matchoveritem_s {
    int nExtraBits;
    int base;
} decode_matchoveritem_t;

typedef struct encode_dispitem_s {
    int nBits;
    UINT16 bits;
} encode_dispitem_t;

typedef struct decode_dispitem_s {
    int nBits;
    int disp;
} decode_dispitem_t;

static int makeSortedTmp(huffstattmpstruct_t*);
static void coder__callStat(lzhl_encoder_t*);
static size_t coder_flush(lzhl_encoder_t*);

static void putRaw(BYTE*, size_t);
static void putMatch(BYTE*, size_t, size_t, size_t);

static size_t calcMaxBuf(size_t);

