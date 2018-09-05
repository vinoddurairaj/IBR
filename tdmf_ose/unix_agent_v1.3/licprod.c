/* #ident "@(#)$Id: licprod.c,v 1.5 2003/11/13 02:48:23 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "license.h"

#define PATH_CONFIG    "DTCCFGDIR"
#define LICKEY_FILEOPENERR -101

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
int
get_license (int pmdflag, char ***lickeyaddr)
{
    FILE* fd;
    struct stat statbuf[1];
    char line[256];
    char* ptr;
    char* key;
    char **lickey;
    int keycnt;
    char fname[256];
    
    sprintf (fname, "%s/%CAPQ%.lic", PATH_CONFIG);
    if ((FILE*)NULL == (fd = fopen (fname, "r"))) {
        return LICKEY_FILEOPENERR;
    } else {
        stat(fname, statbuf);
    }
    if (statbuf->st_size <= 0) {
        return -1;
    }
    keycnt = 0;
    while (!feof(fd) && NULL != fgets (line, 255, fd)) {
        if (0 < strlen(line)) {
            if ('#' != line[0] && 
                ' ' != line[0] && 
                '\t' != line[0] && 
                '\n' != line[0]) {
                if (0 == strncmp(line, "%CAPQ%_LICENSE:", 12)) {
                    ptr = &line[12];
                } else {
                    ptr = line;
                }
                while (ptr[0] != '\0' && ptr[0] != '\n' && 
                       (ptr[0] == ' ' || ptr[0] == '\t')) ptr++;
                key = (char*)malloc(256);
                if (!keycnt) {
                    lickey = (char**)malloc(sizeof(char*));
                } else {
                    lickey = (char**)realloc(lickey, (keycnt+1)*sizeof(char*));
                }
                lickey[keycnt] = key;
                strcpy (key, ptr);
                keycnt++;
            }
        }
        memset(line, 0, sizeof(line));
    }
    /* null-terminate the key list */
    lickey = (char**)realloc(lickey, (keycnt+1)*sizeof(char*));
    lickey[keycnt] = NULL;
    *lickeyaddr = lickey;
    fclose(fd);
    return LICKEY_OK;
}

/*-----------------------------------------------------------------------*/
/*                              END OF FILE                              */
/*-----------------------------------------------------------------------*/

#endif /* _LICPROD_C_ */
