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
/***************************************************************************
 * licinfo.c - Prints information about licenses on this system.
 *
 * (c) Copyright 1996, 1997 FullTime Software, Inc. All Rights Reserved
 *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#if defined(linux)
#include <unistd.h>
#endif /* defined(linux) */
#include "errors.h"
#include "license.h"
#include "licprod.h"
#include "pathnames.h"

#ifdef TDMF_TRACE
FILE *dbgfd;
#endif

/* DANGER: The following arrays assume that RMDFLAG is 0 and PMDFLAG is 1 */
/* We try to catch this at compile time the best we can. */

#define KEY_PATH "%ETCDIR%/%CAPQ%.lic"

unsigned short *keys[] = {
    np_crypt_key_rmd,
    np_crypt_key_pmd
};

#define KEYWORD "%CAPQ%_LICENSE:"


char *argv0;
char *prefix;
int exit_value = 0;
int qflag = 0;						/*wr15023*/


void
print_license(int flag)
{
    license_data_t lic;
    int result;
    char** lickey;
    char* license_type = "";
    
    exit_value = 0;
    if (LICKEY_OK != (result = get_license (0, &lickey))) {
        switch (result) {
        case LICKEY_FILEOPENERR:
            if (!qflag)
                printf("\tLicense file missing: %s/%CAPQ%.lic\n", PATH_CONFIG );
            exit_value = 1;
            return;
        case LICKEY_BADKEYWORD:
            if (!qflag)
                printf("\tLicense file has bad keyword, should be \"%CAPQ%_LICENSE: \"\n");
            exit_value = 1;
            return;
        case LICKEY_FILEEMPTY:
            if (!qflag)
                printf("\tLicense file empty: %s/%CAPQ%.lic\n", PATH_CONFIG );
            exit_value = 1;
            return;
	    case LICKEY_NOT_ALLOWED:
            if (!qflag)
                printf("\tUnrestricted any-host any-site permanent license is not allowed.\n", PATH_CONFIG );
            exit_value = 1;
            return;
        default:
            if (!qflag)
                printf("\tLicense file unknown error: %d: %s/%CAPQ%.lic\n",
                        result, PATH_CONFIG );
            exit_value = 1;
            return;
        }
    }

#if 0
    printf("*lickey=%s, *np_crypt_key_pmd=0x%x, key_len=%d\n",
                *lickey, *np_crypt_key_pmd, NP_CRYPT_KEY_LEN);
#endif

    result = check_key_r (lickey, np_crypt_key_pmd, NP_CRYPT_KEY_LEN, &lic);

#if 0
    printf("*lickey=%s, *np_crypt_key_pmd=0x%x, key_len=%d, chksum=%d, sizeof(license_data_t)=%d\n",
                *lickey, *np_crypt_key_pmd, NP_CRYPT_KEY_LEN, 
                lic.checksum, sizeof(license_data_t));
#endif
    license_type = (lic.hostid == 0 ? "site" : "host");

    if (result == LICKEY_OK)
    {
        exit_value = 0;
    }
    else
    {
        exit_value = 1;
    }
    
    switch (result) {
    case LICKEY_OK:
    {
        if (lic.license_expires) {
            if (!qflag) {
                time_t when = lic.expiration_date << 16;
                printf("\tValid %CAPQ% %s Expiry License -- expires: %s", license_type, ctime(&when));
            }
        } else {
            if (!qflag)
                printf("\tPermanent %CAPQ% %s license is valid for this system.\n", license_type);
        }
        break;
    }
    case LICKEY_EXPIRED:
    {
        if (!qflag) {
            time_t when = lic.expiration_date << 16;        
            printf("\tDefunct %CAPQ% %s Expiry License -- expired: %s", license_type, ctime(&when));
        }
        break;
    }
    case LICKEY_WRONGHOST:
    {
        if (!qflag)
            printf("\t%CAPQ% %s license is _NOT_ for this system.\n", license_type);
        break;
    }
    case LICKEY_BADSITELIC:
    {
        if (!qflag)
            printf("\t%CAPQ% license with bad customer checksum.\n");
        break;
    }
    case LICKEY_WRONGMACHINETYPE:
    {
        if (!qflag)
            printf("\t%CAPQ% license with wrong machine type.\n");
        break;
    }
    case LICKEY_BADCHECKSUM:
    {
        if (!qflag)
            printf("\tMalformed %CAPQ% License key in %s/%CAPQ%.lic - may be a typing error\n", PATH_CONFIG);
        break;
    }
    case LICKEY_NULL:
    case LICKEY_EMPTY:
    case LICKEY_GENERICERR:
    case LICKEY_BADFEATUREMASK:
    default:
    {
        if (!qflag)
            printf("\tBad %CAPQ% License key in %s/%CAPQ%.lic. Error code: %d\n", PATH_CONFIG, result);
        break;
    }
    }
}

void
usage(void)
{
    fprintf(stderr, "Usage: %s [-hq]\n", argv0);		/*wr15023*/
    fprintf(stderr, "\
\t-h\t\t\tThis usage message\n\
\t-q\t\t\tRun quietly, only complaining about errors\n");
    exit(1);
}

int
main (int argc, char **argv)
{
    int ch;

    putenv("LANG=C");

    /* Make sure we are root */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }
    
    argv0 = argv[0];

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    opterr = 0;
    while ((ch = getopt(argc, argv, "hq")) != -1) {		/*wr15023*/
        switch (ch) {
        case 'q':
            qflag = 1;
            break;
        default:
            usage();
        }
    }
    prefix = "%CAPQ%";
    print_license(PMDFLAG);
    exit(exit_value);
}

