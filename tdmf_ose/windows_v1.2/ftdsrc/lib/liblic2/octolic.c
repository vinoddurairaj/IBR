/*=============================================================================
 *  Octopus 3.0 License Key Utilities
 *
 *  S. B. Wahl  -- May 12, 1997
 *
 *===========================================================================*/
#define PROTOTYPES 1
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#include "md5c.h"

#define OCTOLIC_MAIN

#include "octolic.h"

#define OCTOPUS_SIGNATURE 0

#define XOR_MASK 0x8a8a9090

typedef struct BIT_BUFFER_STRUCT
{
	unsigned char bitbuf[128];
	size_t bit_index;
	size_t bit_mask;
} BIT_BUFFER;

extern char bytetob26 (unsigned char in);

/*----------------------------------------------------------------------*/
/* biwkts -- return the current timestamp as an integral value since 1997
             in two-week integral values */
unsigned short
biwkts ()
{
  time_t ts;
  time_t ts1997;
  struct tm baseline;
  long val;
  unsigned short retval;

  baseline.tm_sec = 0;
  baseline.tm_min = 0;
  baseline.tm_hour = 0;
  baseline.tm_mday = 0;
  baseline.tm_mon = 0;
  baseline.tm_year = 97;
  baseline.tm_isdst = 0;
  ts1997 = mktime (&baseline);
  (void) time (&ts);
  val = ((long) ts - (long) ts1997) + LICENSE_INTERVAL-1;
  retval = (unsigned short) (val / LICENSE_INTERVAL);
  return (retval);
}

static const char xlate_table[] = "BZC2D3E4F5G6H78J9KALMNPQRSTUVWXY";

void
reset_bits(int clear_flag, BIT_BUFFER *pBits)
{
	pBits->bit_index = 0;
	pBits->bit_mask = 1;
	if (clear_flag)
		memset(pBits->bitbuf, '\0', sizeof(pBits->bitbuf));
}

static size_t
get_bitbuf_len(BIT_BUFFER *pBits)
{
	if (pBits->bit_mask == 1)
		return(pBits->bit_index);
	return(pBits->bit_index + 1);
}

static
void acc_bit(int value, BIT_BUFFER *pBits)
{
	if (value == 0)
		pBits->bitbuf[pBits->bit_index] &= (char)(~pBits->bit_mask);
	else
		pBits->bitbuf[pBits->bit_index] |= (char)pBits->bit_mask;
	pBits->bit_mask <<= 1;
	if (pBits->bit_mask > 0x80)
	{
		pBits->bit_mask = 1;
		pBits->bit_index++;
	}
}

void
accumulate_bits(unsigned long value, size_t num_bits, BIT_BUFFER *pBits)
{
	size_t i;

	for (i=0 ; i<num_bits ; i++)
	{
		acc_bit(value & 1, pBits);
		value >>= 1;
	}
}

static unsigned long
get_bit(BIT_BUFFER *pBits)
{
	unsigned long value;

	value = (pBits->bitbuf[pBits->bit_index] & pBits->bit_mask) != 0;
	pBits->bit_mask <<= 1;
	if (pBits->bit_mask > 0x80)
	{
		pBits->bit_mask = 1;
		pBits->bit_index++;
	}
	return(value);
}


static unsigned long
extract_bits(size_t num_bits, BIT_BUFFER *pBits)
{
	unsigned long value = 0;
	size_t i;
	unsigned long mask = 1;

	for (i=0 ; i<num_bits ; i++)
	{
		if (get_bit(pBits))
			value |= mask;
		mask <<= 1;
	}
	return(value);
}

// bintoascii
// value must be < 32
static char
bintoascii(unsigned value)
{

	if (value < (sizeof(xlate_table)/sizeof(xlate_table[0])))
		return(xlate_table[value]);
	return(xlate_table[0]);
}

// asciitobin
// returned value will always be < 32
static unsigned long
asciitobin(char ch)
{
	char *ptr;

	ptr = strchr(xlate_table, (int)ch);
	if (ptr != NULL)
		return((unsigned long)(ptr - xlate_table));
	return(0);
}

/*----------------------------------------------------------------------*/
/* md5todec -- extract 3 hex digits from an md5 signature and make decimal */
unsigned short
md5todec (const char* md5string)
{
  char tbuf[7];
  unsigned short tval;
  int t;

  tval = 0;
  tbuf[0] = '0';
  tbuf[1] = 'x';
  tbuf[2] = md5string[9];
  tbuf[3] = md5string[27];
  tbuf[4] = md5string[3];
  tbuf[5] = '\0';
  sscanf (tbuf, "%x", &t);
  tval = (unsigned short) t;
  return (tval);
}

/*----------------------------------------------------------------------*/
/* checklicok -- given a license key and access code, check if it is ok to 
                 run (both valid, unexpired)
		 return codes are:
		 >0 = timestamp of when demo license expires (cast to time_t)
		 0 = permanent license
		 -1 = expired demo license
		 -2 = invalid access code
		 -3 = invalid license key
		 -4 = argument error (wrong size?)
		 */
long
checklicok (
	const char* licstring_param,
	const char* accesscode_param,
	unsigned long *wks_ret,
	FEATURE_TYPE *feature_flags_ret,
	unsigned long *version_ret)
{
  char accesscode[9];
  char licstring[17];
  MD5_CTX context;
  unsigned char digest[16];
  int i;
  char acchk;
  time_t ts1997;
  struct tm baseline;
  unsigned long wks;
  unsigned long featuremask0;
  unsigned long featuremask1;
  unsigned long version;
  unsigned char save_digest_3, save_digest_9;
  BIT_BUFFER Bits;

  strncpy(accesscode, accesscode_param, sizeof(accesscode));
  accesscode[sizeof(accesscode)-1] = '\0';
  strncpy(licstring, licstring_param, sizeof(licstring));
  licstring[sizeof(licstring)-1] = '\0';
  /* -- verify the access code checksum */
  acchk = bytetob26 ((char) ((toupper(accesscode[2]) + toupper(accesscode[3]) + toupper(accesscode[4]) + 
			      toupper(accesscode[5]) + toupper(accesscode[6])) % 256));
  if (acchk != toupper(accesscode[7])) {
    return (-2L);
  }
  /* -- take the access code and the expiration timestamp and generate a
        md5 signature */
  reset_bits(1, &Bits);
  for (i=0 ; i<16 ; i++)
	  accumulate_bits(asciitobin((char)toupper(licstring[i])), 5, &Bits);
  // the last 2 bytes are from the md5 digest, save them for later
  save_digest_3 = Bits.bitbuf[8];
  save_digest_9 = Bits.bitbuf[9];
  // now append access code and signature
  Bits.bit_mask = 1;
  Bits.bit_index = 8;
  // now append the access code and signature
  for (i=0 ; i<8 ; i++)
	  accumulate_bits(accesscode[i], 8, &Bits);
  accumulate_bits('0' + OCTOPUS_SIGNATURE, 8, &Bits);
  MD5Init (&context);
  MD5Update (&context, Bits.bitbuf, get_bitbuf_len(&Bits));
  MD5Final (digest, &context);
  // validate saved digest with what was generated
  if ((save_digest_3 != digest[3])
	  || (save_digest_9 != digest[9]))
    return (-3L);
  /* -- signature is ok, now check expiration date -- */
  reset_bits(0, &Bits);
  wks = extract_bits(10, &Bits);
  featuremask0 = extract_bits(32, &Bits) ^ XOR_MASK;
  featuremask1 = (extract_bits(14, &Bits) ^ XOR_MASK) & 0x3fff;
  version = extract_bits(8, &Bits);
  if (feature_flags_ret != NULL)
  {
	  feature_flags_ret->flags[0] = featuremask0;
	  feature_flags_ret->flags[1] = featuremask1;
  }
  if (version_ret != NULL)
	  *version_ret = version;
  if (wks_ret != NULL)
	  *wks_ret = wks;
  if (wks < biwkts()) {
    return (-1L);
  }
  if (wks != 999) {
    /* -- return timestamp of expiration -- */
    baseline.tm_sec = 0;
    baseline.tm_min = 0;
    baseline.tm_hour = 0;
    baseline.tm_mday = 0;
    baseline.tm_mon = 0;
    baseline.tm_year = 97;
    baseline.tm_isdst = 0;
    ts1997 = mktime (&baseline);

    return ((long) ts1997 + (long)(wks * LICENSE_INTERVAL));
  }
  /* -- license is permanent */
  return (0);
}

/*-------------------------------------------------------------------------*/
/* bytetob26 -- transform a byte into two A-Z characters, return least sig */
char
bytetob26 (unsigned char in)
{
  return ((char) ('A' + (in % 26)));
}

/*-------------------------------------------------------------------------*/
/* genaccode -- given an integer sequence number, generate an access code 
                string (8 characters) */
void
genaccode (char* accode, int value)
{
  MD5_CTX context;
  unsigned char digest[16];
  char valbuf[9];
  int len;

  /* -- create a  MD5 checksum of the value -- */
  sprintf (valbuf, "%08x", value);
  valbuf[8] = '\0';
  len = 8;
  MD5Init (&context);
  MD5Update (&context, (unsigned char *)valbuf, len);
  MD5Final (digest, &context);
  /* -- fill-in the access code hash string */
  accode[0] = 'Q';
  accode[1] = 'O';
  accode[2] = bytetob26 (digest[3]);
  accode[3] = bytetob26 (digest[15]);
  accode[4] = bytetob26 (digest[7]);
  accode[5] = bytetob26 (digest[5]);
  accode[6] = bytetob26 (digest[1]);
  accode[7] = 
  bytetob26 ((char) ((accode[2]+accode[3]+accode[4]+accode[5]+accode[6])%256));
  accode[8] = '\0';
}

/*-------------------------------------------------------------------------*/
/* genlic -- generates an Octopus license (deposits in licstring (char*8))
             when given (1) the number of weeks from now (wksfromnow) the
	     license should expire [set to 0 for permanent], a 
             feature mask (four active bits for enabling/disabling features)
	     and an access code. */
void
genlic(
	char* licstring,
	const char* accode,
	int wksfromnow,
	int version,
	FEATURE_TYPE feature_flags)
{
  MD5_CTX context;
  unsigned char digest[16];
  int i;
  size_t save_bit_mask, save_bit_index;
  BIT_BUFFER Bits;

  reset_bits(1, &Bits);
  /* -- calculate expiration date */
  if (wksfromnow == 0 || wksfromnow == 999) {
    wksfromnow = 999;
  } else {
    wksfromnow = (wksfromnow) + biwkts();
    if (wksfromnow > 999) wksfromnow = 999;
  }
  accumulate_bits(wksfromnow, 10, &Bits);
  accumulate_bits(feature_flags.flags[0] ^ XOR_MASK, 32, &Bits);
  accumulate_bits(feature_flags.flags[1] ^ XOR_MASK, 14, &Bits);
  accumulate_bits(version, 8, &Bits);
  // so far we've accumulated 64 bits, save the current position
  save_bit_mask = Bits.bit_mask;
  save_bit_index = Bits.bit_index;
  // now append the access code and signature
  for (i=0 ; i<8 ; i++)
	  accumulate_bits(toupper(accode[i]), 8, &Bits);
  accumulate_bits('0' + OCTOPUS_SIGNATURE, 8, &Bits);
  MD5Init (&context);
  MD5Update (&context, Bits.bitbuf, get_bitbuf_len(&Bits));
  MD5Final (digest, &context);
  // append 16 bits of the digest
  Bits.bit_mask = save_bit_mask;
  Bits.bit_index = save_bit_index;
  accumulate_bits(digest[3], 8, &Bits);
  accumulate_bits(digest[9], 8, &Bits);
  // this should be a total of 80 bits
  // now translate the bit stream to a character string
  reset_bits(0, &Bits);
  for (i=0 ; i<16 ; i++)
    licstring[i] = bintoascii(extract_bits(5, &Bits));
  licstring[i] = '\0';
}

