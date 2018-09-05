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
#include "licprod.h"
#include "pathnames.h"

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
compare_license_keys (char *lic1, char *lic2, int max_length)
{
	int j, n, match;

	match = 1;
	for ( j = 0, n = max_length-1; j < max_length; j++, n-- )
	{
		if( lic1[j] != (char)(lic2[n] + 1) )
		{
			match = 0;
			break;
		}
	}

	return match;

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
	char TDMFIP280_free_for_all_license[LENGTH_TDMFIP_280_FREEFORALL_LICKEY+1];
	int found_free_for_all_license;
	
	strcpy( TDMFIP280_free_for_all_license, TDMFIP_280_FREEFORALL_LICKEY );
	TDMFIP280_free_for_all_license[LENGTH_TDMFIP_280_FREEFORALL_LICKEY] = NULL;
    
    sprintf (fname, "%s/%CAPQ%.lic", PATH_CONFIG);
    if ((FILE*)NULL == (fd = fopen (fname, "r"))) {
        return LICKEY_FILEOPENERR;
    } else {
        stat(fname, statbuf);
    }
    if (statbuf->st_size <= 0) {
        return LICKEY_FILEEMPTY;
    }
    keycnt = 0;
    while (!feof(fd) && NULL != fgets (line, 255, fd)) {
        if (0 < strlen(line)) {
            if ('#' != line[0] && 
                ' ' != line[0] && 
                '\t' != line[0] && 
                '\n' != line[0]) {
#if !defined(linux)
                if (0 == strncmp(line, "%CAPQ%_LICENSE:", 12)) {
                    ptr = &line[12];
#else
                ptr = strchr(line,'_');
                if (ptr != NULL)
                {
                    if (0 == strncmp(ptr, "_LICENSE:", 9)) {
                        ptr = ptr + 9;
                    } else {
                        ptr = line;
                    }
#endif /* !defined(linux) */
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
	/* In TDMFIP 2.9.0, reject the free-for-all permanent license that was installed in release 2.8.0 */
    lickey[keycnt] = NULL;
    *lickeyaddr = lickey;
    fclose(fd);
	found_free_for_all_license = compare_license_keys( *lickey, TDMFIP280_free_for_all_license, LENGTH_TDMFIP_280_FREEFORALL_LICKEY );
	if( found_free_for_all_license )
    {
	    return LICKEY_NOT_ALLOWED;
    }
    return LICKEY_OK;
}


/*-----------------------------------------------------------------------*/
/*                              END OF FILE                              */
/*-----------------------------------------------------------------------*/

#endif /* _LICPROD_C_ */
