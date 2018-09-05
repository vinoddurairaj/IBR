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
#define LICKEY_FILEEMPTY   -103

#define TDMFIP_280_FREEFORALL_LICKEY "0HFW8G8HCW3HCWAGFO7F8@7F" // Real: G8A9G8PGHBXDI4XDI9H9XGI1
#define LENGTH_TDMFIP_280_FREEFORALL_LICKEY 24

extern void     set_customer_name (char* name);
extern void     set_machine_model (char* model);
extern char     get_customer_name_checksum(void);
extern char*    get_customer_name(void);
extern char*    get_machine_model(void);
extern char     get_machine_model_checksum(void);
extern int      get_license (int pmdflag, char*** keyl);


extern unsigned short np_crypt_key_pmd[];
extern unsigned short np_crypt_key_rmd[];

#endif /* _LICPROD_H_ */

