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
#include "ftd_port.h"
#include "ftd_error.h"
//#include "license.h"
#include "licprod.h"

#if !defined(_OCTLIC)
#include "license.h"
#include "ftd_lic.h"
#else
#include "LicAPI.h"
#endif

#if !defined(_OCTLIC)
/* DANGER: The following arrays assume that RMDFLAG is 0 and PMDFLAG is 1 */
/* We try to catch this at compile time the best we can. */

#define REG_LICENSE_PATH "HKEY_LOCAL_MACHINE\\Software\\" OEMNAME "\\" PRODUCTNAME "\\CurrentVersion\\License"

#define KEY_PATH "%ETCDIR%/" CAPQ ".lic"

unsigned short *keys[] = {
    np_crypt_key_rmd,
    np_crypt_key_pmd
};

#define KEYWORD CAPQ "_LICENSE:"

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
//    char** lickey, path[MAXPATHLEN];
//	char** path[MAXPATHLEN];
	char	*lickey;
    int dummy;

    exit_value = 0;

/*
#if defined(_WINDOWS)
	sprintf(path, "%s\\%s.lic", PATH_CONFIG, CAPQ);
#else
	sprintf(path, "%s/%s.lic", PATH_CONFIG, CAPQ);
#endif    
	if (LICKEY_OK != (result = get_license(path , &lickey))) {
        switch (result) {
        case LICKEY_FILEOPENERR:
            if (!qflag)
#if defined(_WINDOWS)                
				printf("\tLicense file missing: %s\\" CAPQ ".lic\n",
					PATH_CONFIG );
#else
				printf("\tLicense file missing: %s/" CAPQ ".lic\n",
					PATH_CONFIG );
#endif
			exit_value = 1;
            return;
        case LICKEY_BADKEYWORD:
            if (!qflag)
                printf("\tLicense file has bad keyword, should be " CAPQ "_LICENSE: \"\n");
            exit_value = 1;
            return;
        }
    }
*/
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
#if defined(_WINDOWS)                
            printf("\tBad " CAPQ " License key in %s", REG_LICENSE_PATH);
#else
            printf("\tBad " CAPQ " License key in %s/" CAPQ ".lic\n",
				PATH_CONFIG);
#endif
        exit_value = 1;
        return;
    case LICKEY_BADCHECKSUM:
        if (!qflag)
#if defined(_WINDOWS)                
            printf("\tMalformed " CAPQ " License key in %s (transcription error?)\n", REG_LICENSE_PATH);
#else
            printf("\tMalformed " CAPQ " License key in %s/" CAPQ ".lic (transcription error?)\n");
#endif
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
                printf("\tValid " CAPQ " Demo License -- expires: %s\n", ctime(&when));
            }
        } else {
            if (!qflag) {
                printf("\tExpired " CAPQ "Demo License -- expired: %s\n", ctime(&when));
            }
            exit_value = 1;
        }            
        return;
    } else if (lic.hostid == 0) {		/* Site license */
        cs = lic.customer_name_checksum;
        if (cs != 0 && get_customer_name_checksum() != cs) {
            if (!qflag)
                printf("\t" CAPQ " Site license with bad checksum\n");
            exit_value = 1;
        } else {
            exit_value = 0;
            if (!qflag) 
                printf("\tValid permanent " CAPQ " site license.\n");
        }
        return;
    }
    /* Otherwise we're a host license */
    if (lic.hostid != my_gethostid()) {
        if (!qflag)
            printf("\tPermanent " CAPQ " license is _NOT_ for this system\n");
        exit_value = 1;
    } else {
        exit_value = 0;
        if (!qflag)
            printf("\tPermanent " CAPQ " license is valid for this system\n");
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
	long			lRc;
	unsigned long	ulWeeks;
	FEATURE_TYPE	feature_flags_ret;
	unsigned long	ulVersion;
	struct tm		*gmt;
	char			szLicenseKey[100];
#endif

#if !defined(_OCTLIC)
    argv0 = argv[0];

    if (ftd_init_errfac(CAPQ, argv[0], NULL, NULL, 0, 1) == NULL) {
        goto errexit;
    }

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
    prefix = CAPQ;

    print_license(PMDFLAG);

errexit:
    
#if defined(_WINDOWS) && defined(_DEBUG)
	
	printf("\nPress any key to continue....\n");
	while(!_kbhit())
		Sleep(1);
#endif

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
	
#if defined(_WINDOWS) && defined(_DEBUG)
	
	printf("\nPress any key to continue....\n");
	while(!_kbhit())
		Sleep(1);
#endif

#endif
}

