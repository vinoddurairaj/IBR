#ifndef _lzhl_h
#define _lzhl_h

#define LZHLINTERNAL

#include "_lz.h"
#include "_huff.h"

typedef struct lzhl_s {
	int srclen;
	unsigned char *src;
	int targetlen;
	unsigned char *target;
	lztableitem_t table[LZTABLESIZE];
	lzbuffer_t lzbuf[1];
	huffstattmpstruct_t s[NHUFFSYMBOLS];
	lzhl_encoder_t coder[1];
	lzhl_decoder_t decoder[1];
} lzhl_t;

extern size_t lzhl_compress(lzhl_t *lzhlp);
extern int lzhl_decompress(lzhl_t *lzhlp);
extern int lzhl_compress_init(lzhl_t *lzhlp);
extern void lzhl_compress_free(lzhl_t *lzhlp);

#endif
