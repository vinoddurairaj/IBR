/*========================================================================
 * licprod.c
 *
 * This file contains product-specific internal utilities for the 
 * FullTime Data license management library.  
 *
 * This implementation is for FullTime Data v4.0
 *
 * Copyright (c) 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved.
 *========================================================================*/

#ifndef _LICPROD_C_
#define _LICPROD_C_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "license.h"
#include "licprod.h"

/* -- This implementation is for FullTime Data 4.0 -- to translate to other
      products, define your own CRYPT_KEY definitions (use integer arrays 
      so strings in executable don't reveal the encryption key */

unsigned short np_crypt_key_pmd[10] = 
    { 103, 111, 111, 102, 121, 102, 111, 111, 116, 0 };
unsigned short np_crypt_key_rmd[10] =
    { 103, 111, 111, 102, 121, 102, 111, 111, 116, 0 };

static char liccustname[80];
static char licmachinemodel[80];



/*-----------------------------------------------------------------------*/
void
set_customer_name (char* name)
{
    int i;
    
    for (i=0; i<80; i++)
        liccustname[i] = '\0';
    if (strlen(name) > 79)
        name[79] = '\0';
    (void) strcpy (liccustname, name);
}
/*-----------------------------------------------------------------------*/
void
set_machine_model (char* model)
{
  int i;
  
  for (i=0; i<80; i++)
      licmachinemodel[i] = '\0';
  if (strlen(model) > 79)
      model[79] = '\0';
  (void) strcpy (licmachinemodel, model);
}

/*-----------------------------------------------------------------------*/
char
get_customer_name_checksum(void)
{
    return license_calculate_checksum(liccustname);
}

/*-----------------------------------------------------------------------*/
char*
get_customer_name(void)
{
    return liccustname;
}

/*-----------------------------------------------------------------------*/
char*
get_machine_model(void)
{
    return licmachinemodel;
}

/*-----------------------------------------------------------------------*/
char
get_machine_model_checksum(void)
{
    char           *ptr;
    
    ptr = licmachinemodel;
    if (ptr == (char *) NULL)
        return 0;
    else
        return license_calculate_checksum(ptr);
}

/*-----------------------------------------------------------------------*/
/*                              END OF FILE                              */
/*-----------------------------------------------------------------------*/

#endif /* _LICPROD_C_ */
