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
/*
 * Copyright (c) 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "license.h"
#include "licprod.c"

/*-----------------------------------------------------------------------*/
#define STR_MAX 80
#define cryptpos(__c, __len) \
    ((__c%2) ? __c : (__len - __c - ((__len%2) ? 1 : 2)))

/*-----------------------------------------------------------------------*/
unsigned int
x_license_calculate_checksum(license_data_t * key)
{
  license_data_t  x;
  license_checksum_t rv;
  char           *ptr;
  int             i, n;
  unsigned int    sum = 0;

  rv.x = 0;
  rv.checksum = 0;																  
  memcpy((void *) &x, (void *) key, sizeof(license_data_t));
  x.checksum = 0;
  ptr = (char *) &x;
  n = sizeof(license_data_t);
  for (i = 0; i < n; i++) {
    sum += ((unsigned int) *ptr++) % 10;
  }
  rv.checksum = sum;
  return  rv.checksum;
}

/*-----------------------------------------------------------------------*/
char
license_calculate_checksum(char *ptr)
{
  int i;

  i = 0;
  while (*ptr != '\0') 
    i += *ptr++;
  return (char) i;
}

/*-----------------------------------------------------------------------*/
int
license_is_site_wide(char *key, unsigned short* crypt_key, int crypt_key_len)
{
  license_data_t  lic_out, lic_crypt;
  int             i;
  char            str[STR_MAX];

  for (i = 0; i < STR_MAX; i++)
    str[i] = '\0';
  strncpy(str, key, STR_MAX - 1);
  char_decode((unsigned char *) str, (unsigned char *) &lic_crypt,
	      2 * sizeof(license_data_t));
  np_decrypt((unsigned char *) &lic_crypt, (unsigned char *) &lic_out,
	     sizeof(license_data_t), crypt_key, crypt_key_len);
  if (lic_out.license_expires == 1) 
    return 0;
  if (lic_out.customer_name_checksum != 0)
    return 0;
  return 1;
}

/*-----------------------------------------------------------------------*/
void
np_encrypt(unsigned char *src, unsigned char *dest, int len,
	   unsigned short *crypt_key, int key_len)
{
  int i;
  for (i = 0; i < len; i++)
    dest[i] = (unsigned char) src[cryptpos(i, len)] + 
      crypt_key[(i % key_len)] - ((i > 0) ? dest[i - 1] : 0);

#if 0
  printf("len=%d, key_len=%d\n", len, key_len);
  for (i = 0; i < len; i++) {
    printf("0x%2x ", src[i]);
  }
  printf("\n");
  for (i = 0; i < len; i++) {
    printf("0x%2x ", dest[i]);
  }
  printf("\n");
#endif
}

/*-----------------------------------------------------------------------*/
void
np_decrypt(unsigned char *src, unsigned char *dest, int len,
	   unsigned short *crypt_key, int key_len)
{
  int i;
  for (i = 0; i < len; i++)
    dest[cryptpos(i, len)] = (unsigned char) src[i] - 
      crypt_key[(i % key_len)] + ((i > 0) ? src[i - 1] : 0);

#if 0
  printf("len=%d, key_len=%d\n", len, key_len);
  for (i = 0; i < len; i++) {
    printf("0x%2x ", src[i]);
  }
  printf("\n");
  for (i = 0; i < len; i++) {
    printf("0x%2x ", dest[i]);
  }
  printf("\n");
#endif
}

/*-----------------------------------------------------------------------*/
void
char_encode(unsigned char *src, unsigned char *dest, int src_len)
{
  int             i;
  int             j;
  char            str[STR_MAX + 1];
  unsigned char  *ptr = dest;

  for (i = 0; i < src_len; i++) {
    dest[2 * i] = ((src[i] >> 4) & 0xf) + 'A';
    if (dest[2 * i] == 'O')
      dest[2 * i] = 'X';
    dest[(2 * i) + 1] = ((src[i]) & 0xf);
    dest[(2 * i) + 1] += (dest[(2 * i) + 1] >= 9) ? ('A' - 9) : '1';
  }
  /* add spaces for readability */
  i = 0;
  j = 0;
  for (; *ptr != '\0'; ptr++) {
    if (i == STR_MAX) {
        break;
    }
    str[i++] = *ptr;
    j++;
    if ((j != 0) && ((j % NP_CRYPT_GROUP_SIZE) == 0))
      str[i++] = ' ';
  }
  str[i++] = '\0';
  strcpy((char*)dest, str);
}

/*-----------------------------------------------------------------------*/
void
char_decode(unsigned char *src, unsigned char *dest, int src_len)
{
  int             i;
  char            str[STR_MAX + 1];
  unsigned char  *ptr = src;

  /* strip out the spaces */
  str[0] = '\0';
  str[STR_MAX] = '\0';
  i = 0;
  for (; *ptr != '\0'; ptr++) {
    if (i == STR_MAX) {
        break;
    }
    if (*ptr == ' ')
      continue;
    str[i++] = *ptr;
  }
  str[i++] = '\0';
  strcpy((char*)src, str);
  for (i = 0; i < src_len; i += 2) {
    dest[i / 2] = (((src[i] == 'X') ? 'O' : src[i]) - 'A') << 4;
    dest[i / 2] |= (src[i + 1] - ((src[i + 1] >= 'A') ? ('A' - 9) : '1'));
  }
}

/*-----------------------------------------------------------------------*/
int
check_key(char **keyl, unsigned short* np_crypt_key, int crypt_key_len)
{
  license_data_t  t;

  return (check_key_r (keyl, np_crypt_key, crypt_key_len, &t));
}


/*-----------------------------------------------------------------------*/
int
check_key_r (char **keyl, unsigned short* np_crypt_key, int crypt_key_len,
             license_data_t* ret_lic)
{
  license_data_t  lic_out, lic_crypt;
  char            buffer[BUFSIZ + 1];
  time_t          exp_date;
  time_t          now;
  license_checksum_t checksum;
  int i;
  int rc;
  char *key;
	char TDMFIP280_free_for_all_license[LENGTH_TDMFIP_280_FREEFORALL_LICKEY+1];
	int found_free_for_all_license;
	
	strcpy( TDMFIP280_free_for_all_license, TDMFIP_280_FREEFORALL_LICKEY );
	TDMFIP280_free_for_all_license[LENGTH_TDMFIP_280_FREEFORALL_LICKEY] = NULL;

	found_free_for_all_license = compare_license_keys( *keyl, TDMFIP280_free_for_all_license, LENGTH_TDMFIP_280_FREEFORALL_LICKEY );
	if( found_free_for_all_license )
    {
	    return LICKEY_NOT_ALLOWED;
    }
 
  memset ((void*)ret_lic, 0, sizeof(license_data_t));
  if (keyl == (char**)NULL)
    return LICKEY_NULL;
  if (*keyl == NULL)
    return LICKEY_EMPTY;

  for (i = 0; keyl[i]; i++) {
    rc = LICKEY_OK;
    key = keyl[i];
    strncpy(buffer, key, BUFSIZ);
    buffer[BUFSIZ] = '\0';
    char_decode((unsigned char *) buffer, (unsigned char *) &lic_crypt,
	      2 * sizeof(license_data_t));
    np_decrypt((unsigned char *) &lic_crypt, (unsigned char *) &lic_out,
	     sizeof(license_data_t), np_crypt_key, crypt_key_len);
    checksum.checksum = x_license_calculate_checksum(&lic_out);

#if 0
    printf("i=%d, key=%s, crypt_key_len=%d, lic_out.checksum=0x%x, checksum.checksum=0x%x\n",
	i, key, crypt_key_len,
	lic_out.checksum, checksum.checksum);
#endif
    if (lic_out.checksum != 0 && checksum.checksum != lic_out.checksum) {
      rc = LICKEY_BADCHECKSUM;
      continue;
    }
    ret_lic->license_expires = lic_out.license_expires;
    ret_lic->expiration_date = lic_out.expiration_date;
    ret_lic->hostid = lic_out.hostid;
    ret_lic->customer_name_checksum = lic_out.customer_name_checksum;
    if (lic_out.license_expires) {	
      /* demo key */
      /* First check to see if license has expired */
      exp_date = lic_out.expiration_date << 16;
      now = time((time_t *) NULL);
      if (now > exp_date) {
        rc = LICKEY_EXPIRED;
        continue;
      }
      if (lic_out.hostid == 0) {
        break;
      }
#ifdef TDMFUNIXGENUS  // Code borrowed by TDMF UNIX, to fix compilation problem here
      if (lic_out.hostid != my_gethostid()) {
#else
      if (lic_out.hostid != my_gethostid(HOSTID_LICENSE)) {
#endif
        /* WRONGHOST */
        rc = LICKEY_WRONGHOST;
        continue;
      }
      return (LICKEY_OK);
    } else {		/* Permanent license */
      if (lic_out.hostid == 0) {	/* site license */
        if ((lic_out.customer_name_checksum != 0)
        && (get_customer_name_checksum() != lic_out.customer_name_checksum)) {
          rc = LICKEY_BADSITELIC;
          continue;
        }
      } else {	/* hostid-based license */
#ifdef TDMFUNIXGENUS  // Code borrowed by TDMF UNIX, to fix compilation problem here
         if (lic_out.hostid != my_gethostid()) {
#else
         if (lic_out.hostid != my_gethostid(HOSTID_LICENSE)) {
#endif
          rc = LICKEY_WRONGHOST;
          continue;
        }
      }
      if ((lic_out.machine_model_checksum != 0) && 
        (get_machine_model_checksum() != 0)) {
        if (get_machine_model_checksum() != lic_out.machine_model_checksum) {
          rc = LICKEY_WRONGMACHINETYPE;
          continue;
        }
      }
      if (lic_out.license_level != 0) {	/* sg limitation */
      }
      return (LICKEY_OK);
    }
  }
  return rc;
}

/*-----------------------------------------------------------------------*/
/* END OF FILE                                                           */
/*-----------------------------------------------------------------------*/

