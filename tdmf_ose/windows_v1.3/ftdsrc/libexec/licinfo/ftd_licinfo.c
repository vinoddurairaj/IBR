/*
 * ftd_licinfo.c - dump license info to stdout 
 *
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

// Include this library to force the linker to link the MFC libraries in the correct order Mike Pollett
#include "../../forcelib.h"
// Mike Pollett
#include "../../tdmf.inc"

#if defined(_WINDOWS) && defined(_DEBUG)
#include <conio.h>
#endif
#include "ftd_port.h"
#include "ftd_error.h"
//#include "license.h"
#include "licprod.h"
#include "sock.h"

#if defined(_WINDOWS) 
//libftd, ftd_mngt_msgs.cpp
void ftd_mngt_msgs_log(const char **argv, int argc);
#endif

#if !defined(_OCTLIC)
#include "license.h"
#include "ftd_lic.h"
#else
#include "LicAPI.h"
#endif

#if !defined(_OCTLIC)
/* DANGER: The following arrays assume that RMDFLAG is 0 and PMDFLAG is 1 */
/* We try to catch this at compile time the best we can. */

#define REG_LICENSE_PATH "HKEY_LOCAL_MACHINE\\Software\\" OEMNAME "\\Dtc\\CurrentVersion\\License"

unsigned short *keys[] = {
    np_crypt_key_rmd,
    np_crypt_key_pmd
};

char *argv0;
int exit_value = 0;
int pflag = 1;
int qflag = 0;
int rflag = 1;

void
print_license(int flag)
{
    license_data_t lic;
    int result;
    time_t exp_time;
    struct tm expires;
    time_t now;
    int cs;
//    char** lickey, path[MAXPATHLEN];
//  char** path[MAXPATHLEN];
    char    *lickey;
    int dummy;

    exit_value = 0;

    lickey = (char *)malloc(_MAX_PATH);
    memset(lickey, 0, _MAX_PATH);
    getLicenseKeyFromReg(lickey);
    if(strlen(lickey) < 1)
    {
        printf("\tLicense missing from registry: %s\n", REG_LICENSE_PATH );
        return;
    }
    //result = check_key(&lickey, np_crypt_key_pmd, NP_CRYPT_KEY_LEN);

    result = check_key_r(&lickey, np_crypt_key_pmd, NP_CRYPT_KEY_LEN, &lic);

    
    switch (result) {
    case LICKEY_NULL:
    case LICKEY_EMPTY:
    case LICKEY_GENERICERR:
        if (!qflag)
            printf("\tBad License key in %s", REG_LICENSE_PATH);
        exit_value = 1;
        return;
    case LICKEY_BADCHECKSUM:
        if (!qflag)
            printf("\tMalformed License key in %s (transcription error?)\n", REG_LICENSE_PATH);
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

        exp_time = lic.expiration_date << 16;

        /* Force expiration hour to 13:00:00 */
        expires = *localtime( &exp_time );  
        expires.tm_sec = 00;
        expires.tm_min = 00;
        expires.tm_hour = 13;
        exp_time = mktime(&expires);

        now = time((time_t *) NULL);
        if (exp_time > now) {
            exit_value = 0;
            if (!qflag) {
                printf("\tValid Non-Permanent License -- expires: %s\n", ctime(&exp_time));
            }
        } else {
            if (!qflag) {
                printf("\tExpired Non-Permanent License -- expired: %s\n", ctime(&exp_time));
            }
            exit_value = 1;
        }            
        return;
    } else if (lic.hostid == 0) {       /* Site license */
        cs = lic.customer_name_checksum;
        if (cs != 0 && get_customer_name_checksum() != cs) {
            if (!qflag)
                printf("\t Site license with bad checksum\n");
            exit_value = 1;
        } else {
            exit_value = 0;
            if (!qflag) 
                printf("\tValid permanent site license.\n");
        }
        return;
    }
    /* Otherwise we're a host license */
    if (lic.hostid != my_gethostid()) {
        if (!qflag)
            printf("\tPermanent license is _NOT_ for this system\n");
        exit_value = 1;
    } else {
        exit_value = 0;
        if (!qflag)
            printf("\tPermanent license is valid for this system\n");
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

    return;
}
#endif

int
main (int argc, char **argv)
{
#if !defined(_OCTLIC)
    int ch;
#else
    long            lRc;
    unsigned long   ulWeeks;
    FEATURE_TYPE    feature_flags_ret;
    unsigned long   ulVersion;
    struct tm       *gmt;
    char            szLicenseKey[100];
#endif

#if !defined(_OCTLIC)
    argv0 = argv[0];

#if defined(_WINDOWS) 
    if (sock_startup() == -1) {
        goto errexit;
    }
#endif

    if (ftd_init_errfac("Replicator", argv[0], NULL, NULL, 0, 1) == NULL) {
        goto errexit;
    }

#if defined(_WINDOWS) 
    //send status msg to System Event Log and TDMF Collector
    ftd_mngt_msgs_log(argv, argc);
#endif

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
            goto errexit;
        }
    }

    print_license(PMDFLAG);

errexit:
    
    exit(exit_value);

#else
    // OctLic
    lRc = validateLicense(&ulWeeks, &feature_flags_ret, &ulVersion);

    memset(szLicenseKey, 0, sizeof(szLicenseKey));
    getLicenseKeyFromReg(szLicenseKey);

    if(lRc > 0)
    {
        // Time left in license key
        gmt = gmtime( (time_t *)&lRc );
        printf("License Key: %s\nExpires: %s\n", szLicenseKey, asctime( gmt ));
    }
    else if(lRc == 0)
    {
        printf("Permanent License: %s\n", szLicenseKey);
    }
    else if(lRc == -1)
    {
        printf("Expired Demo License: %s\n", szLicenseKey);
    }
    else if(lRc == -2)
    {
        printf("Invalid Access Code: %s\n", szLicenseKey);
    }
    else if(lRc == -3)
    {
        printf("Invalid License Key: %s\n", szLicenseKey);
    }
#endif
}

