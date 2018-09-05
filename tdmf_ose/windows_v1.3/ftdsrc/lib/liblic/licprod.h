/*========================================================================
 * licprod.h
 *
 * This file contains product-specific internal utilities for the 
 * FullTime Data license management library.  
 *
 * Copyright (c) 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved.
 *========================================================================*/

#ifndef _LICPROD_H_
#define _LICPROD_H_


/* -- This implementation is for FullTime Data v4.0 -- to translate to other
      products, define your own CRYPT_KEY definitions (use integer arrays 
      so strings in executable don't reveal the encryption key */

#define NP_CRYPT_KEY_LEN 10

#define PMDFLAG 1
#define RMDFLAG 1

#define LICKEY_FILEOPENERR -101
#define LICKEY_BADKEYWORD  -102

#ifdef __cplusplus
extern "C"{ 
#endif

extern void     set_customer_name (char* name);
extern void     set_machine_model (char* model);
extern char     get_customer_name_checksum(void);
extern char*    get_customer_name(void);
extern char*    get_machine_model(void);
extern char     get_machine_model_checksum(void);


extern unsigned short np_crypt_key_pmd[];
extern unsigned short np_crypt_key_rmd[];

#ifdef __cplusplus 
}
#endif

#endif /* _LICPROD_H_ */

