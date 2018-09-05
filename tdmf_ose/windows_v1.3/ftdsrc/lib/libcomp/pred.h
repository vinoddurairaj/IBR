#ifndef _pred_h
#define _pred_h

#define GUESS_TABLE_SIZE 65536

#define IHASH(x,y) do {(x) = ((x) << 4) ^ (y);} while(0)
#define OHASH(x,y) do {(x) = ((x) << 4) ^ (y);} while(0)

typedef struct pred_s {
	unsigned short iHash;
	unsigned short oHash;
	unsigned char InputGuessTable[GUESS_TABLE_SIZE];
	unsigned char OutputGuessTable[GUESS_TABLE_SIZE];
	int srclen;
	unsigned char *src;
	int targetlen;
	unsigned char *target;
} pred_t;

extern int pred_compress_init(pred_t *predp);
extern int pred_compress(pred_t *predp);
extern int pred_decompress(pred_t *predp);

#endif

