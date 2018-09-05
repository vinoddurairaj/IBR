/********************************************************* {COPYRIGHT-TOP} ***
* IBM Confidential
* OCO Source Materials
* 6949-32F - Softek Replicator for Unix and 6949-32K - Softek TDMF (IP) for Unix
*
*
* (C) Copyright IBM Corp. 2006, 2011  All Rights Reserved.
* The source code for this program is not published or otherwise  
* divested of its trade secrets, irrespective of what has been 
* deposited with the U.S. Copyright Office.
********************************************************* {COPYRIGHT-END} **/
/*========================================================================
 * license.h
 *
 * This file contains the FullTime license management library interfaces.
 *
 * Copyright (c) 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved.
 *========================================================================*/

#ifndef _LICENSE_H_
#define _LICENSE_H_

#include "licplat.h"

/* -- error codes from check_key -- */
#define LICKEY_OK                  1
#define LICKEY_GENERICERR          0
#define LICKEY_NULL               -1
#define LICKEY_EMPTY              -2
#define LICKEY_BADCHECKSUM        -3
#define LICKEY_EXPIRED            -4
#define LICKEY_WRONGHOST          -5
#define LICKEY_BADSITELIC         -6
#define LICKEY_WRONGMACHINETYPE   -7
#define LICKEY_BADFEATUREMASK     -8
#define LICKEY_NOT_ALLOWED        -9

#define TDMFIP_280_FREEFORALL_LICKEY "0HFW8G8HCW3HCWAGFO7F8@7F" // Real: G8A9G8PGHBXDI4XDI9H9XGI1
#define LENGTH_TDMFIP_280_FREEFORALL_LICKEY 24


/* -- data types -- */
typedef struct license_data {
  short           expiration_date;	/* In 2^16 second units from epoch */
#if defined(_AIX)
  unsigned int   checksum:7;
  unsigned int   license_expires:1;	/* True/False */
#else  /* defined(_AIX) */
  unsigned char   checksum:7;
  unsigned char   license_expires:1;	/* True/False */
#endif /* defined(_AIX) */
  char            license_level;
  int             hostid;
  char            machine_model_checksum;
  char            customer_name_checksum;
} license_data_t;

typedef struct license_checksum {
#if defined(_AIX)
  unsigned int   checksum:7;
  unsigned int   x:1; 
#else  /* defined(_AIX) */
  unsigned char   checksum:7;
  unsigned char   x:1; 
#endif /* defined(_AIX) */
} license_checksum_t;

/* -- license management API definition -- */
extern void     
np_encrypt (unsigned char *src, unsigned char *dest, int len, 
	    unsigned short *crypt_key, int key_len);

extern void     
np_decrypt (unsigned char *src, unsigned char *dest, int len, 
	    unsigned short *crypt_key, int key_len);

extern void     
char_encode (unsigned char *src, unsigned char *dest, int src_len);

extern void     
char_decode (unsigned char *src, unsigned char *dest, int src_len);

extern char     
license_calculate_checksum(char *ptr);

extern int      
license_is_site_wide(char *key, unsigned short* crypt_key, int crypt_key_len);

extern unsigned int
x_license_calculate_checksum(license_data_t *key);

extern int      
check_key(char** keyl, unsigned short* cryptkey, int cryptkeylen);

extern int      
check_key_r(char** keyl, unsigned short* cryptkey, int cryptkeylen,
            license_data_t* ret_lic);

/* -- internal utility inferfaces defined on a per-product basis */
#include "licprod.h"
#define NP_CRYPT_GROUP_SIZE 4

/* ===== end of file ===== */
#endif /* _LICENSE_H_ */
