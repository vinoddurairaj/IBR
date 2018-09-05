/* #ident "@(#)$Id: license.c,v 1.4 2003/11/13 02:48:23 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "license.h"
#include "licprod.c"

/*-----------------------------------------------------------------------*/
#define STR_MAX 80
#define cryptpos(__c, __len) \
    ((__c%2) ? __c : (__len - __c - ((__len%2) ? 1 : 2)))


/*-----------------------------------------------------------------------*/
license_checksum_t
x_license_calculate_checksum(license_data_t * key)
{
  license_data_t  x;
  license_checksum_t rv;
  char           *ptr;
  int             i, n;
  unsigned int    sum = 0;

  rv.x = 0;
  rv.checksum = 0;
  x = *key;
  x.checksum = 0;
  ptr = (char *) &x;
  n = sizeof(license_data_t);
  for (i = 0; i < n; i++) {
    sum += ((unsigned int) *ptr++) % 10;
  }
  rv.checksum = sum;
  return rv;
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
    checksum = x_license_calculate_checksum(&lic_out);
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
      if (lic_out.hostid != my_gethostid()) {
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
        if (lic_out.hostid != my_gethostid()) {
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

