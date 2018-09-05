/*
 *  LZH-Light algorithm implementation v 1.0
 *
 *  Copyright (C) Sergey Ignatchenko 1998
 *  This  software  is  provided  "as is"  without  express  or implied 
 *  warranty.
 *
 *  Permission to use, copy, modify, distribute and sell this  software 
 *  for any purpose is hereby  granted  without  fee,  subject  to  the 
 *  following restrictions:
 *
 *  1. this notice may not be removed or altered;
 *  2. altered source versions must be plainly marked as such, and must 
 *     not be misrepresented as being the original software.
 *
 *  T.Mills - 10/98 - converted from C++ to C
 *
 */

#define LZHLINTERNAL

#include "sftk_lzhl.h"

#ifdef LZSLOWHASH
#define LZHASHSHIFT 5
#define UPDATE_HASH( hash, c )          \
    {                                   \
    (hash) ^= (c);                        \
    (hash) = ROTL( (hash), LZHASHSHIFT );   \
    }
#define UPDATE_HASH_EX( hash, src )                     \
    {                                                   \
    (hash) ^= ROTL( (src)[ 0 ], LZHASHSHIFT * LZMATCH );  \
    (hash) ^= (src)[ LZMATCH ];                           \
    (hash) = ROTL( (hash), LZHASHSHIFT );                   \
    }

#define HASH_POS(hash) ((( (hash) * 214013 + 2531011) >> (32-LZTABLEBITS)) )

#else

#define LZHASHSHIFT (((LZTABLEBITS)+(LZMATCH)-1)/(LZMATCH))

#define UPDATE_HASH( hash, c ) { (hash) = ( (hash) << LZHASHSHIFT ) ^ (c); }
#define UPDATE_HASH_EX( hash, src )  { \
    (hash) = ( (hash) << LZHASHSHIFT ) ^ (src)[ LZMATCH ]; }

#define LZHASHMASK ((LZTABLESIZE)-1)
#define HASH_POS( hash ) ( (hash) & LZHASHMASK )
#endif

static symbol_t encode_symbolTable0[] = {
    #include "Henc.tbl"
};

static symbol_t decode_symbolTable0[] = {
    #include "Hdec_s.tbl"
};

static grp_t groupTable0[] = {
    #include "Hdec_g.tbl"
};

#define _calcHash(src, hash) \
    for (p=(src);p<(src)+LZMATCH; ) { \
        UPDATE_HASH((hash),*p++); \
    };

int i, j, k;
BYTE *p;

#define LZBUF__NMATCH(lzbuf, pos, p, nLimit) \
    if (LZBUFSIZE-(pos) >= (nLimit)) { \
        for (i=0,j=(pos);i<(nLimit);++i) { \
            if ((lzbuf)->buf[j+i] != (p)[i]) { \
                (nLimit)=i; \
                break; \
            } \
        } \
    } else { \
        for (i=j=(pos);i<LZBUFSIZE;++i) { \
            if ((lzbuf)->buf[i] != p[i-j]) { \
                (nLimit)=i-j; \
                break; \
            } \
        } \
        for (i=0,j=LZBUFSIZE-(pos),k=(nLimit)-(LZBUFSIZE-(pos));i<k;++i) { \
            if ((lzbuf)->buf[i] != p[j+i]) { \
                (nLimit)=j+i; \
                break; \
            } \
        } \
    };

#define _UPDATETABLE(table, hash, src, pos, len) \
    if ((len)>0) { \
        if ((len)>LZSKIPHASH) { \
            ++(src); \
            for (p=((src)+(len));p<((src)+(len)+LZMATCH); ) { \
                UPDATE_HASH((hash),*p++); \
            } \
        } else { \
            UPDATE_HASH_EX((hash),(src)); \
            ++(src); \
            for (i=0;i<(len);++i) { \
                table[HASH_POS((hash))]=(LZTABLEINT)(((pos)+i)&LZBUFMASK); \
                UPDATE_HASH_EX((hash),(src)+i); \
            } \
        } \
    };

#define CODER__PUTBITS(coder, codeBits, code) \
    (coder)->bits |= ((code)<<(32-(coder)->nBits-(codeBits))); \
    (coder)->nBits += (codeBits); \
    if ((coder)->nBits >= 16) { \
        *(coder)->dst++ = (BYTE)((coder)->bits>>24); \
        *(coder)->dst++ = (BYTE)((coder)->bits>>16); \
        (coder)->nBits -= 16; \
        (coder)->bits <<= 16; \
    }; \

#define CODER__PUT_0(coder, symbol) \
    if (--(coder)->stat->nextStat <= 0) { \
        (void)coder__callStat((coder)); \
    } \
    ++(coder)->sstat[(UINT16)(symbol)]; \
    CODER__PUTBITS((coder), \
        (&(coder)->stat->symbolTable[(UINT16)(symbol)])->nBits, \
        (&(coder)->stat->symbolTable[(UINT16)(symbol)])->code);

#define LZBUF_TOBUF_BYTE(lzbuf, c) \
    (lzbuf)->buf[((lzbuf)->bufPos&LZBUFMASK)] = (c); \
    (lzbuf)->bufPos++;

#define LZBUF_TOBUF_BUF(lzbuf, src, sz) \
    if ((((lzbuf)->bufPos&LZBUFMASK)+sz) > LZBUFSIZE) { \
        memcpy((lzbuf)->buf+((lzbuf)->bufPos&LZBUFMASK), src, \
            LZBUFSIZE-((lzbuf)->bufPos&LZBUFMASK)); \
        memcpy((lzbuf)->buf, src+(LZBUFSIZE-((lzbuf)->bufPos&LZBUFMASK)), \
            sz-(LZBUFSIZE-((lzbuf)->bufPos&LZBUFMASK))); \
    } else { \
        memcpy((lzbuf)->buf+((lzbuf)->bufPos&LZBUFMASK), src, sz); \
    } \
    (lzbuf)->bufPos+=sz;

#define LZBUF__BUFCPY(lzbuf, dst, pos, sz) \
    if ((((pos)&LZBUFMASK)+sz) > LZBUFSIZE) { \
        memcpy((dst), (lzbuf)->buf+((pos)&LZBUFMASK), \
            LZBUFSIZE-((pos)&LZBUFMASK)); \
        memcpy((dst)+(LZBUFSIZE-((pos)&LZBUFMASK)), (lzbuf)->buf, \
            sz-(LZBUFSIZE-((pos)&LZBUFMASK))); \
    } else { \
        memcpy((dst), (lzbuf)->buf+((pos)&LZBUFMASK), sz); \
    } \
    
#define CMPHUFFSTATTMPSTRUCT(a, b) \
    ( ((b)->n-(a)->n) ? ((b)->n-(a)->n): ((b)->i-(a)->i) )
    
#define _ADDGROUP(groups, group, nBits) \
    for (j=(group);j>0 && (nBits)<(groups)[j-1];--j) { \
        (groups)[j]=(groups)[j-1]; \
    } \
    (groups)[j]=(nBits);

#define DECODER__GET(decoder, src, srcEnd, n, ret) \
    if ((decoder)->nBits < (n)) { \
        if (*(src) >= (srcEnd)) { \
            (decoder)->nBits = 0; \
            *(ret) = -1; \
        } else { \
            (decoder)->bits |= (*(*(src))<<(24-(decoder)->nBits)); \
            (*(src))++; \
            (decoder)->nBits+=8; \
            *(ret) = (decoder)->bits>>(32-(n)); \
            (decoder)->bits<<=(n); \
            (decoder)->nBits-=(n); \
        } \
    } else { \
        *(ret) = (decoder)->bits>>(32-(n)); \
        (decoder)->bits<<=(n); \
        (decoder)->nBits-=(n); \
    } \
    
static void
shellSort(huffstattmpstruct_t *a, int N)
{
    int i, j;
    int h;
    huffstattmpstruct_t v;

    ASSERT(13 <= N / 9);
    ASSERT(40 > N / 9); 

    h = 40;

    for ( ; h > 0; h /= 3 ) {         /* h = 40, 13, 4, 1 */
        for (i = h + 1; i <= N; ++i) {
            v = a[i];
            j = i;
            while ((j > h)
            && (CMPHUFFSTATTMPSTRUCT(&v,&a[j-h]) < 0)) {
                a[j] = a[j-h];
                j -= h;
            }
            a[j] = v;
        }
    }
} /* shellSort */

static int
coderstat_makeSortedTmp
(lzhl_encoderstat_t *stat, huffstattmpstruct_t* s)
{
    int total = 0;
    int j;

    for (j = 0; j < NHUFFSYMBOLS ; ++j) {
        s[j].i = (short)j;
        s[j].n = stat->stat[j];
        total += stat->stat[j];
        stat->stat[j] = HUFFRECALCSTAT(stat->stat[j]);
    }
    shellSort(s-1, NHUFFSYMBOLS);
    return total;
} /* coderstat_makeSortedTmp */

static int
decoderstat_makeSortedTmp
(lzhl_decoderstat_t *stat, huffstattmpstruct_t* s)
{
    int total = 0;
    int j;

    for (j = 0; j < NHUFFSYMBOLS ; ++j) {
        s[j].i = (short)j;
        s[j].n = stat->stat[j];
        total += stat->stat[j];
        stat->stat[j] = HUFFRECALCSTAT(stat->stat[j]);
    }
    shellSort(s-1, NHUFFSYMBOLS);
    return total;
} /* decoderstat_makeSortedTmp */

static void
coderstat__calcStat(lzhl_encoderstat_t *stat, int *groups)
{
    static huffstattmpstruct_t s[NHUFFSYMBOLS];
    int pos = 0;
    int nTotal = 0;
    int group;
    int avgGroup;
    int i, j, k, n, nn, left;
    int nBits;
    int nBits15;
    int nItems;
    int nItems15;
    int over;
    int bestNBits;
    int bestNBits15;
    int best;
    int pos15;
    int maxK;
    int symbol;
    int total;

    total = coderstat_makeSortedTmp(stat, s);

    stat->nextStat = HUFFRECALCLEN;

    for (group = 0; group < 14; ++group) {
        avgGroup = (total-nTotal)/(16-group);
        i = n = nn = 0;
        for (nBits = 0;; ++nBits) {
            over = 0;
            nItems = 1 << nBits;

            if (pos + i + nItems > NHUFFSYMBOLS) {
                nItems = NHUFFSYMBOLS - pos;
                over = 1;
            }
            for ( ; i < nItems ; ++i) {
                nn += s[pos+i].n;
            }
            if (over || nBits >= 8 || nn > avgGroup) {
                if (nBits == 0 || abs(n-avgGroup) > abs(nn-avgGroup)) {
                    n = nn;
                } else {
                    --nBits;
                }
                _ADDGROUP(groups, group, nBits);
                nTotal += n;
                pos += 1 << nBits;
                break;
            } else {
                n = nn;
            }
        }
    }
    bestNBits = 0;
    bestNBits15 = 0;
    best = 0x7FFFFFFF;
    i = nn = left = 0;
    for (j = pos; j < NHUFFSYMBOLS ; ++j) {
        left += s[j].n;
    }
    for (nBits = 0 ;; ++nBits) {
        nItems = 1 << nBits;
        if (pos + i + nItems > NHUFFSYMBOLS) {
            break;
        }
        for ( ; i < nItems ; ++i) {
            nn += s[pos+i].n;
        }
        nItems15 = NHUFFSYMBOLS - (pos+i);
        for (nBits15 = 0 ;; ++nBits15) {
            if (1 << nBits15 >= nItems15) {
                break;
            }
        } 

        ASSERT(left >= nn);

        if (nBits <= 8 && nBits15 <= 8) {
            n = nn*nBits+(left-nn )*nBits15;
            if (n < best) {
                best = n;
                bestNBits = nBits;
                bestNBits15 = nBits15;
            } else {
                break; /* PERF optimization */
            }
        }
    }
    pos15 = pos+(1<<bestNBits);
    _ADDGROUP(groups, 14, bestNBits);
    _ADDGROUP(groups, 15, bestNBits15);

    pos = 0;
    for(j = 0; j < 16 ; ++j) {
        nBits = groups[j];
        nItems = 1 << nBits;
        maxK = min(nItems, NHUFFSYMBOLS-pos);
        for (k = 0; k < maxK ; ++k) {
            symbol = s[pos+k].i;
            stat->symbolTable[symbol].nBits = nBits + 4;
            stat->symbolTable[symbol].code = (j<<nBits) | k;
        }
        pos += 1 << nBits;
    }
} /* coderstat__calcStat */

static void
coder__callStat(lzhl_encoder_t *coder)
{
    static int groups[16];
    int nBits;
    int delta;
    int lastNBits = 0;
    int i;

    coder->stat->nextStat = 2; /* to avoid recursion, >=2 */
    CODER__PUT_0(coder, NHUFFSYMBOLS-2);
    coderstat__calcStat(coder->stat, groups);

    for (i = 0; i < 16 ; ++i) {
        nBits = groups[i];

        ASSERT(nBits >= lastNBits && nBits <= 8);

        delta = nBits - lastNBits;
        lastNBits = nBits;
        CODER__PUTBITS(coder, delta + 1, 1);
    }
    return;
} /* coder__callStat */

static void
coder_putRaw(lzhl_encoder_t *coder, BYTE* src, size_t sz)
{
    BYTE *srcEnd;

    for (srcEnd = src+sz; src < srcEnd ; ++src) {
        CODER__PUT_0(coder, *src);
    }
    return;
} /* coder_putRaw */

static void coder_putMatch
(lzhl_encoder_t *coder, BYTE* src, size_t nRaw, size_t matchOver, size_t disp)
{
    encode_matchoveritem_t *mitem;
    encode_dispitem_t *ditem;
    int nBits;
    UINT32 bits;

    symbol_t *item;

    static encode_matchoveritem_t _matchOverTable[] = {
    { 264, 1, 0x00 },

    { 265, 2, 0x00 },
    { 265, 2, 0x02 },

    { 266, 3, 0x00 },
    { 266, 3, 0x02 },
    { 266, 3, 0x04 },
    { 266, 3, 0x06 },

    { 267, 4, 0x00 },
    { 267, 4, 0x02 },
    { 267, 4, 0x04 },
    { 267, 4, 0x06 },
    { 267, 4, 0x08 },
    { 267, 4, 0x0A },
    { 267, 4, 0x0C },
    { 267, 4, 0x0E },
    };

    static encode_dispitem_t _dispTable[] = {
        #include "Hdisp.tbl"
    };

    ASSERT((int)nRaw <= coder->maxRaw);
    ASSERT((int)matchOver <= coder->maxMatchOver);
    ASSERT(disp >= 0 && disp < LZBUFSIZE);

    coder_putRaw(coder, src, nRaw);

    if (matchOver < 8) {
        CODER__PUT_0(coder, 256+matchOver);
    } else if (matchOver < 38) {
        matchOver -= 8;
        mitem = &_matchOverTable[matchOver>>1];
        if (--coder->stat->nextStat <= 0) { 
            (void)coder__callStat((coder));
        }
        ++coder->sstat[mitem->symbol];
        item = &coder->stat->symbolTable[mitem->symbol];
        CODER__PUTBITS(coder, item->nBits+mitem->nBits,
            (item->code << mitem->nBits) | (mitem->bits | (matchOver&0x01)));
    } else {
        matchOver -= 38;
        mitem = &_matchOverTable[matchOver >> 5];
        CODER__PUT_0(coder, mitem->symbol+4);
        CODER__PUTBITS(coder, mitem->nBits+4,
            (mitem->bits<<4) | (matchOver&0x1F));
    }
#if LZBUFBITS < 8
    #error
#endif
    ditem = &_dispTable[disp >> (LZBUFBITS-7)];
    nBits = ditem->nBits + (LZBUFBITS-7);
    bits = (((UINT32)ditem->bits) << (LZBUFBITS-7))
        | (disp & ((1 << (LZBUFBITS-7))-1));
#if LZBUFBITS >= 15
    if (nBits > 16) {

        ASSERT(nBits <= 32);

        CODER__PUTBITS(coder, nBits-16, bits >> 16);
        CODER__PUTBITS(coder, 16, bits & 0xFFFF);
    } else
#endif
    {

        ASSERT(nBits <= 16);

        CODER__PUTBITS(coder, nBits, bits);
    }
} /* coder_putMatch */

static size_t
coder_flush(lzhl_encoder_t *coder)
{
    CODER__PUT_0(coder, NHUFFSYMBOLS - 1);
    while (coder->nBits > 0) {
        *coder->dst++ = (BYTE)(coder->bits >> 24);
        coder->nBits -= 8;
        coder->bits <<= 8;
    }
    return coder->dst - coder->dstBegin;
} /* coder_flush */

static size_t
calcMaxBuf(size_t rawSz)
{
    return rawSz + (rawSz >> 1) + 32;
} /* calcMaxBuf */

int lzhl_compress_init(lzhl_t *lzhlp)
{
    int i;

    /* init table */
    for (i = 0; i < LZTABLESIZE; ++i) {
        lzhlp->table[i] = (LZTABLEINT)-1;
    }

	if (!lzhlp->coder)
	{
        lzhlp->coder = ExAllocatePoolWithTag(PagedPool, sizeof(lzhl_encoder_t), 'kjhg');
		if (!lzhlp->coder)
		{
			return 0;
		}
	}

	if (!lzhlp->decoder) 
	{
        lzhlp->decoder = ExAllocatePoolWithTag(PagedPool, sizeof(lzhl_decoder_t), 'kjhg');
		if (!lzhlp->decoder)
		{
			return 0;
		}
	}
   
    lzhlp->coder->maxMatchOver = 517;
    lzhlp->coder->maxRaw = 64;
    if (!lzhlp->coder->stat) {
        lzhlp->coder->stat = ExAllocatePoolWithTag(PagedPool, sizeof(lzhl_encoderstat_t), 'kjhg');
    } else {
        memset(lzhlp->coder->stat, 0, sizeof(lzhl_encoderstat_t));
    }
    if (!lzhlp->coder->stat->symbolTable) {
        lzhlp->coder->stat->symbolTable =
            ExAllocatePoolWithTag(PagedPool, NHUFFSYMBOLS*sizeof(symbol_t), 'kjhg');
		if (!lzhlp->coder->stat->symbolTable) {
			return 0;
		}
    } else {
        memset(lzhlp->coder->stat->symbolTable, 0, 
            NHUFFSYMBOLS*sizeof(symbol_t));
    }
    memcpy(lzhlp->coder->stat->symbolTable, encode_symbolTable0,
        NHUFFSYMBOLS*sizeof(symbol_t));
    if (!lzhlp->coder->stat->stat) {
        lzhlp->coder->stat->stat = ExAllocatePoolWithTag(PagedPool, sizeof(HUFFINT) * NHUFFSYMBOLS, 'kjhg');
    } else {
        memset(lzhlp->coder->stat->stat, 0, NHUFFSYMBOLS*sizeof(HUFFINT));
    }

    /* init decoder->stat */
    if (!lzhlp->decoder->stat) {
        lzhlp->decoder->stat = ExAllocatePoolWithTag(PagedPool, sizeof(lzhl_decoderstat_t) , 'kjhg');
		lzhlp->decoder->stat->symbolTable = NULL;
    } else {
        memset(lzhlp->decoder->stat, 0, sizeof(lzhl_decoderstat_t));
    }
    if (!lzhlp->decoder->stat->symbolTable) {
        lzhlp->decoder->stat->symbolTable =
            ExAllocatePoolWithTag(PagedPool, NHUFFSYMBOLS*sizeof(symbol_t), 'kjhg');
    } else {
        memset(lzhlp->decoder->stat->symbolTable, 0, 
            NHUFFSYMBOLS*sizeof(symbol_t));
    }
    memcpy(lzhlp->decoder->stat->symbolTable, decode_symbolTable0,
        NHUFFSYMBOLS*sizeof(HUFFINT));
    memcpy(lzhlp->decoder->stat->groupTable, groupTable0, 16*sizeof(grp_t));
    
    if (!lzhlp->decoder->stat->stat) {
        lzhlp->decoder->stat->stat = ExAllocatePoolWithTag(PagedPool, NHUFFSYMBOLS*sizeof(HUFFINT), 'kjhg');
    } else {
        memset(lzhlp->decoder->stat->stat, 0, NHUFFSYMBOLS*sizeof(HUFFINT));
    }
    /* init buffer */
	if (!lzhlp->lzbuf)
	{
		lzhlp->lzbuf = ExAllocatePoolWithTag(PagedPool, sizeof(lzbuffer_t), 'kjhg');
		if (!lzhlp->lzbuf)
			return 0;
	} else {
		memset(lzhlp->lzbuf->buf, 0, sizeof(lzbuffer_t));
	}
    if (!lzhlp->lzbuf->buf) {
        lzhlp->lzbuf->buf = ExAllocatePoolWithTag(PagedPool, LZBUFSIZE, 'kjhg');
    } else {
        memset(lzhlp->lzbuf->buf, 0, LZBUFSIZE);
    }
    lzhlp->lzbuf->bufPos = 0;

    return 0;
} /* lzhl_compress_init */

void lzhl_compress_free(lzhl_t *lzhlp)
{
    if (lzhlp->lzbuf->buf)
		ExFreePool(lzhlp->lzbuf->buf);

	if (lzhlp->coder->stat->symbolTable)
		ExFreePool(lzhlp->coder->stat->symbolTable);

	if (lzhlp->coder->stat->stat)
		ExFreePool(lzhlp->coder->stat->stat);

    if (lzhlp->coder->stat)
		ExFreePool(lzhlp->coder->stat);

	if (lzhlp->decoder->stat->symbolTable)
		ExFreePool(lzhlp->decoder->stat->symbolTable);

	if (lzhlp->decoder->stat->stat)
		ExFreePool(lzhlp->decoder->stat->stat);

    if (lzhlp->decoder->stat)
		ExFreePool(lzhlp->decoder->stat);
}

size_t
lzhl_compress(lzhl_t *lzhlp)
{
    BYTE *p, *pEnd;
    const BYTE *srcBegin = lzhlp->src;
    const BYTE *srcEnd = lzhlp->src + lzhlp->srclen;
    int srcLeft;
    int nRaw;
    int maxRaw;
    int hashPos;
    int wrapBufPos;
    int matchLen;
    int matchLimit;
    int xtraMatchLimit;
    int xtraMatch;
    int d;
    int i;
#ifdef LZLAZYMATCH
    int lazyMatchHashPos;
    LZPOS lazyMatchBufPos;
    int lazyMatchNRaw;
    LZHASH lazyMatchHash;
    int lazyMatchLen = 0;
    BOOL lazyForceMatch = FALSE;
#endif
    LZHASH hash, hash2;
    BYTE *srcp;
    int rc;

    /* init encoder */
    lzhlp->coder->stat->nextStat = HUFFRECALCLEN;
    lzhlp->coder->sstat = lzhlp->coder->stat->stat;

    lzhlp->coder->dst = lzhlp->coder->dstBegin = lzhlp->target;
    lzhlp->coder->bits = lzhlp->coder->nBits = 0;

    hash = 0;
    if (lzhlp->srclen >= LZMATCH) {
        pEnd = (BYTE*)(lzhlp->src+LZMATCH);
        for (p = lzhlp->src; p < pEnd ; ) {
            UPDATE_HASH(hash, *p++);
        }
    }
    for (;;) {
        srcLeft = srcEnd - lzhlp->src;

        if (srcLeft < LZMATCH) {
            if (srcLeft) {
                LZBUF_TOBUF_BUF(lzhlp->lzbuf, lzhlp->src, srcLeft);
                coder_putRaw(lzhlp->coder, lzhlp->src, srcLeft);
                lzhlp->src += srcLeft;
            }
            break; /* forever */
        }
        nRaw = 0;
        maxRaw = min(srcLeft-LZMATCH, lzhlp->coder->maxRaw);
#ifdef LZLAZYMATCH
        lazyMatchLen = 0;
        lazyForceMatch = FALSE;
#endif
        for (;;) {
            hash2 = HASH_POS(hash);
            hashPos = lzhlp->table[hash2];
            wrapBufPos = lzhlp->lzbuf->bufPos & LZBUFMASK;
            lzhlp->table[hash2] = (lztableitem_t)wrapBufPos;

            matchLen = 0;
            if (hashPos != (LZTABLEINT)(-1) && hashPos != wrapBufPos) {
                matchLimit = min(min(((wrapBufPos-hashPos)&LZBUFMASK),
                    srcLeft-nRaw), LZMIN+lzhlp->coder->maxMatchOver);
                LZBUF__NMATCH(lzhlp->lzbuf, hashPos, (lzhlp->src+nRaw), matchLimit);
                matchLen = matchLimit;

#ifdef LZOVERLAP
                if (((hashPos+matchLen) & LZBUFMASK) == wrapBufPos) {

                    ASSERT(matchLen != 0);

					xtraMatchLimit = min(LZMIN+lzhlp->coder->maxMatchOver-matchLen,
                        srcLeft-nRaw-matchLen);
                    for (xtraMatch=0; xtraMatch < xtraMatchLimit; ++xtraMatch) {
                        if (lzhlp->src[nRaw+xtraMatch] !=
                            lzhlp->src[nRaw+xtraMatch+matchLen]) {
                            break; /* for(xtraMatch) */
                        }
                    }
                    matchLen += xtraMatch;
                }
#endif
#ifdef LZBACKWARDMATCH
                if (matchLen >= LZMIN-1) {
                    /* to ensure that buf will be overwritten */
                    xtraMatchLimit = min(LZMIN+lzhlp->coder->maxMatchOver-matchLen,
                        nRaw);
                    d = (int)((lzhlp->lzbuf->bufPos-hashPos)&LZBUFMASK);
                    xtraMatchLimit = min(min(xtraMatchLimit, d-matchLen),
                        LZBUFSIZE-d);
                    for (xtraMatch=0; xtraMatch < xtraMatchLimit; ++xtraMatch) {
                        if (lzhlp->lzbuf->buf[((hashPos-xtraMatch-1) & LZBUFMASK)] !=
                            lzhlp->src[nRaw-xtraMatch-1]) {
                            break; /* for(xtraMatch) */
                        }
                    }
                    if (xtraMatch > 0) {

/*
                        assert(matchLen+xtraMatch >= LZMIN);
                        assert(matchLen+xtraMatch <=
                            ((lzbuf->bufPos-hashPos)&LZBUFMASK));
*/
                        nRaw -= xtraMatch;
                        lzhlp->lzbuf->bufPos -= xtraMatch;
                        hashPos -= xtraMatch;
                        matchLen += xtraMatch;
                        wrapBufPos = lzhlp->lzbuf->bufPos & LZBUFMASK;
                        hash = 0;
                        _calcHash(lzhlp->src+nRaw, hash);
#ifdef LZLAZYMATCH
                        lazyForceMatch = TRUE;
#endif
                        }
                    }
#endif
                }
#ifdef LZLAZYMATCH
            if (lazyMatchLen >= LZMIN) {
                if (matchLen > lazyMatchLen) {
                    coder_putMatch(lzhlp->coder, lzhlp->src, nRaw,
                        matchLen-LZMIN, ((wrapBufPos-hashPos)&LZBUFMASK));
                    srcp=lzhlp->src+nRaw;
                    _UPDATETABLE(lzhlp->table, hash, srcp, lzhlp->lzbuf->bufPos+1,
                        min(matchLen-1, srcEnd-(lzhlp->src+nRaw+1)));
                    LZBUF_TOBUF_BUF(lzhlp->lzbuf, lzhlp->src+nRaw, matchLen);
                    lzhlp->src += nRaw+matchLen;
                    break; /* for(nRaw) */
                } else {
                    nRaw = lazyMatchNRaw;
                    lzhlp->lzbuf->bufPos = lazyMatchBufPos;
                    hash = lazyMatchHash;
                    UPDATE_HASH_EX(hash, lzhlp->src+nRaw);
                    coder_putMatch(lzhlp->coder, lzhlp->src, nRaw, lazyMatchLen-LZMIN,
                        ((lzhlp->lzbuf->bufPos-lazyMatchHashPos)&LZBUFMASK));
                    srcp=lzhlp->src+nRaw+1;
                    _UPDATETABLE(lzhlp->table, hash, srcp, lzhlp->lzbuf->bufPos+2,
                        min(lazyMatchLen-2, srcEnd-(lzhlp->src+nRaw+2)));
                    LZBUF_TOBUF_BUF(lzhlp->lzbuf, lzhlp->src+nRaw, lazyMatchLen);
                    lzhlp->src += nRaw+lazyMatchLen;
                    break; /* for(nRaw) */
                }
            }
#endif
            if (matchLen >= LZMIN) {
#ifdef LZLAZYMATCH
                if (!lazyForceMatch ) {
                    lazyMatchLen = matchLen;
                    lazyMatchHashPos = hashPos;
                    lazyMatchNRaw = nRaw;
                    lazyMatchBufPos = lzhlp->lzbuf->bufPos;
                    lazyMatchHash = hash;
                } else
#endif
                {
                coder_putMatch(lzhlp->coder, lzhlp->src, nRaw, matchLen-LZMIN,
                    ((wrapBufPos-hashPos)&LZBUFMASK));
                srcp=lzhlp->src+nRaw;
                _UPDATETABLE(lzhlp->table, hash, srcp, lzhlp->lzbuf->bufPos+1,
                    min(matchLen-1, srcEnd-(lzhlp->src+nRaw+1)));
                LZBUF_TOBUF_BUF(lzhlp->lzbuf, lzhlp->src+nRaw, matchLen);
                lzhlp->src += nRaw+matchLen;
                break; /* for(nRaw) */
                }
            }
#ifdef LZLAZYMATCH
            ASSERT(!lazyForceMatch);
#endif
            if (nRaw+1 > maxRaw) {
#ifdef LZLAZYMATCH
                if (lazyMatchLen >= LZMIN) {
                    coder_putMatch(lzhlp->coder, lzhlp->src, nRaw, lazyMatchLen-LZMIN,
                        ((lzhlp->lzbuf->bufPos-lazyMatchHashPos)&LZBUFMASK));
                    srcp=lzhlp->src+nRaw;
                    _UPDATETABLE(lzhlp->table, hash, srcp, lzhlp->lzbuf->bufPos+1,
                        min(lazyMatchLen-1, srcEnd-(lzhlp->src+nRaw+1)));
                    LZBUF_TOBUF_BUF(lzhlp->lzbuf, lzhlp->src+nRaw, lazyMatchLen);
                    lzhlp->src += nRaw+lazyMatchLen;
                    break; /* for(nRaw) */
                }
#endif
                if (nRaw+LZMATCH >= srcLeft && srcLeft <= lzhlp->coder->maxRaw) {
                    LZBUF_TOBUF_BUF(lzhlp->lzbuf, lzhlp->src+nRaw, srcLeft-nRaw);
                    nRaw = srcLeft;
                }
                coder_putRaw(lzhlp->coder, lzhlp->src, nRaw);
                lzhlp->src += nRaw;
                break; /* for( nRaw ) */
            }
            UPDATE_HASH_EX(hash, lzhlp->src+nRaw);
            LZBUF_TOBUF_BYTE(lzhlp->lzbuf, lzhlp->src[nRaw++]);
        } /* for(nRaw) */
    } /* forever */

    rc = coder_flush(lzhlp->coder);

    return rc; 
} /* lzhl_compress */

int
lzhl_decompress(lzhl_t *lzhlp)
{
    const BYTE *startDst = lzhlp->target;
    const BYTE *startSrc = lzhlp->src;
	const BYTE *endSrc = lzhlp->src + lzhlp->srclen;
    const BYTE *endDst = lzhlp->target + lzhlp->targetlen;
    int grp;
    int pos0, pos1, pos2;
    int symbol;
    int matchOver;
    int matchLen;
    int nBits = 0;
    int lastNBits;
    int dispPrefix;
    int disp;
    int extra;
    int i;
    int n;
    int ret;
    grp_t *group;
    BOOL shift;
    static int firsttime = 1;

    decode_matchoveritem_t* mitem;
    decode_dispitem_t* ditem;

    static decode_matchoveritem_t _matchOverTable[] = {
            { 1,   8 },
            { 2,  10 },
            { 3,  14 },
            { 4,  22 },
            { 5,  38 },
            { 6,  70 },
            { 7, 134 },
            { 8, 262 }
    };

    static decode_dispitem_t _dispTable[] = {
            { 0,  0 },
            { 0,  1 },
            { 1,  2 },
            { 2,  4 },
            { 3,  8 },
            { 4, 16 },
            { 5, 32 },
            { 6, 64 }
    };

    /* init decoder */
    lzhlp->decoder->bits = 0;
    lzhlp->decoder->nBits = 0;

    for (;;) {
        if (lzhlp->src >= endSrc) {
            return FALSE;
        }
        DECODER__GET(lzhlp->decoder, &lzhlp->src, endSrc, 4, &grp);
        group = &lzhlp->decoder->stat->groupTable[grp];
        nBits = group->nBits;
        if (nBits == 0) {
            symbol = lzhlp->decoder->stat->symbolTable[group->pos];
        } else {
            ASSERT(nBits <= 8);
            DECODER__GET(lzhlp->decoder, &lzhlp->src, endSrc, nBits, &pos0);
            pos0 += group->pos;
            if (pos0 >= NHUFFSYMBOLS) {
                return FALSE;
            }
            symbol = lzhlp->decoder->stat->symbolTable[pos0];
        }

        ASSERT(symbol < NHUFFSYMBOLS);

        ++(lzhlp->decoder->stat->stat[symbol]);
        shift = FALSE;
        if (symbol < 256) {
            if (lzhlp->target >= endDst) {
                return FALSE;
            }
            *lzhlp->target = (BYTE)symbol;
            lzhlp->target++;
            LZBUF_TOBUF_BYTE(lzhlp->lzbuf, (BYTE)symbol);
            continue; /* forever */
        } else if (symbol == NHUFFSYMBOLS - 2) {
            decoderstat_makeSortedTmp(lzhlp->decoder->stat, lzhlp->s);

            for (i = 0; i < NHUFFSYMBOLS ; ++i) {
                lzhlp->decoder->stat->symbolTable[i] = lzhlp->s[i].i;
            }
            lastNBits = 0;
            pos1 = 0;
            for (i = 0; i < 16 ; ++i) {
                for (n = 0 ;; ++n ) {
                    DECODER__GET(lzhlp->decoder, &lzhlp->src, endSrc, 1, &ret);
                    if (ret) {
                        break;
                    }
                }
                lastNBits += n;
                lzhlp->decoder->stat->groupTable[i].nBits = lastNBits;
                lzhlp->decoder->stat->groupTable[i].pos = pos1;
                pos1 += 1 << lastNBits;
            }

            ASSERT(pos1 < NHUFFSYMBOLS + 255);

            continue; /* forever */
        } else if (symbol == NHUFFSYMBOLS - 1) {
            break; /* forever */
        }
        if (symbol < 256 + 8) {
            matchOver = symbol - 256;
        } else {
            mitem = &_matchOverTable[symbol-256-8];
            DECODER__GET(lzhlp->decoder, &lzhlp->src, endSrc, mitem->nExtraBits, &extra);
            matchOver = mitem->base + extra;
        }
        DECODER__GET(lzhlp->decoder, &lzhlp->src, endSrc, 3, &dispPrefix);

        ditem = &_dispTable[dispPrefix];
        nBits = ditem->nBits+LZBUFBITS-7;
        disp = 0;

        ASSERT(nBits <= 16);

        if (nBits > 8) {
            nBits -= 8;
            DECODER__GET(lzhlp->decoder, &lzhlp->src, endSrc, 8, &ret);
            disp |= ret << nBits;
        }

        ASSERT(nBits <= 8);

        DECODER__GET(lzhlp->decoder, &lzhlp->src, endSrc, nBits, &ret);
        disp |= ret;

        disp += ditem->disp << (LZBUFBITS - 7);

        
		ASSERT(disp >=0 && disp < LZBUFSIZE);

        matchLen = matchOver + LZMIN;
        if (lzhlp->target + matchLen > endDst) {
            return FALSE;
        }
        pos2 = lzhlp->lzbuf->bufPos - disp;
        if (matchLen < disp) {
            LZBUF__BUFCPY(lzhlp->lzbuf, lzhlp->target, pos2, matchLen);
        } else {
            LZBUF__BUFCPY(lzhlp->lzbuf, lzhlp->target, pos2, disp);
            for (i = 0; i < matchLen - disp; ++i) {
                lzhlp->target[i + disp] = lzhlp->target[i];
            }
        }
        LZBUF_TOBUF_BUF(lzhlp->lzbuf, lzhlp->target, matchLen);
        lzhlp->target += matchLen;
    }
    if (lzhlp->targetlen)
        lzhlp->targetlen -= (lzhlp->target - startDst);
    if (lzhlp->srclen)
        lzhlp->srclen -= (lzhlp->src - startSrc);

    //return TRUE;

	return (lzhlp->target - startDst);
/*
    return (dst - startDst);
*/
} /* lzhl_decompress */

