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
int pflag = 1;
int qflag = 0;
int rflag = 1;

void
print_license(int flag)
{
    license_data_t lic;
    int result;
    time_t when;
    time_t now;
    int cs;
    char** lickey;
    int dummy;

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
        }
    }
    result = check_key_r (lickey, np_crypt_key_pmd, NP_CRYPT_KEY_LEN, &lic);
    switch (result) {
    case LICKEY_NULL:
    case LICKEY_EMPTY:
    case LICKEY_GENERICERR:
        if (!qflag)
            printf("\tBad %CAPQ% License key in %s/%CAPQ%.lic\n", PATH_CONFIG);
        exit_value = 1;
        return;
    case LICKEY_BADCHECKSUM:
        if (!qflag)
            printf("\tMalformed %CAPQ% License key in %s/%CAPQ%.lic (transcription error?)\n");
        exit_value = 1;
        return;
    case LICKEY_OK:
        exit_value = 0;
        break;
    case LICKEY_EXPIRED:
    case LICKEY_WRONGHOST:
    case LICKEY_BADSITELIC:
    case LICKEY_WRONGMACHINETYPE:
    case LICKEY_BADFEATUREMASK:
        exit_value = 1;
        break;
    default:
        /* drop through */
        dummy = 0;
    }
    if (lic.license_expires) {
        when = lic.expiration_date << 16;
        now = time((time_t *) NULL);
        if (when > now) {
            exit_value = 0;
            if (!qflag) {
                printf("\tValid %CAPQ% Demo License -- expires: %s\n", ctime(&when));
            }
        } else {
            if (!qflag) {
                printf("\tExpired %CAPQ% Demo License -- expired: %s\n", ctime(&when));
            }
            exit_value = 1;
        }            
        return;
    } else if (lic.hostid == 0) {		/* Site license */
        cs = lic.customer_name_checksum;
        if (cs != 0 && get_customer_name_checksum() != cs) {
            if (!qflag)
                printf("\t%CAPQ% Site license with bad checksum\n");
            exit_value = 1;
        } else {
            exit_value = 0;
            if (!qflag) 
                printf("\tValid permanent %CAPQ% site license.\n");
        }
        return;
    }
    /* Otherwise we're a host license */
    if (lic.hostid != my_gethostid()) {
        if (!qflag)
            printf("\tPermanent %CAPQ% license is _NOT_ for this system\n");
        exit_value = 1;
    } else {
        exit_value = 0;
        if (!qflag)
            printf("\tPermanent %CAPQ% license is valid for this system\n");
    }
}

void
usage(void)
{
    fprintf(stderr, "Usage: %s [-hpqr]\n", argv0);
    fprintf(stderr, "\
\t-h\t\t\tThis usage message\n\
\t-p\t\t\tJust check to see if the PMD has a license\n\
\t-q\t\t\tRun quietly, only complaining about errors\n\
\t-r\t\t\tJust check to see if the RMD has a license\n");
    exit(1);
}

int
main (int argc, char **argv)
{
    int ch;

    argv0 = argv[0];

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    opterr = 0;
    while ((ch = getopt(argc, argv, "hpqr")) != -1) {
        switch (ch) {
        case 'p':
            pflag = 1;
            rflag = 0;
            qflag = 0;
            break;
        case 'q':
            qflag = 1;
            break;
        case 'r':
            pflag = 1;
            rflag = 0;
            qflag = 0;
            break;
        default:
            usage();
        }
    }
    prefix = "%CAPQ%";
    print_license(PMDFLAG);
    exit(exit_value);
}

