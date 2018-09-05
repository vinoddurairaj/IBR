//
// Copyright (C) 1994 - 2000 Legato Systems, Inc.
// All rights reserved.
//
//
/****************************************************************************
*    LicAPI.h
*    PURPOSE: main header file for the Replication Block license key utilities
*
*	 K. Jute  -- March, 23 2000
****************************************************************************/

#ifndef BLOCKLIC_H_INCLUDED
#define BLOCKLIC_H_INCLUDED

#include "octolic.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	LICENSE_VERSION_OCTOPUS_3 = 1,
	LICENSE_VERSION_OCTOPUS_32,
	LICENSE_VERSION_FTD_4,
	LICENSE_VERSION_OCTOPUS_40 = 4,
	LICENSE_VERSION_REPLICATION_43 = 5,
	LICENSE_VERSION_MDS_50 = 6,
	LICENSE_VERSION_STK_50 = 7
};


// Registry paths to license key
#define REG_PATH "Software\\" OEMNAME "\\" PRODUCTNAME "\\CurrentVersion"
#define REG_LICENSE_KEY "License"

void getLicenseKeyFromReg(char *szLicenseKey);
void writeLicenseKeyToReg(char *szLicenseKey);
void parseLicenseKey(const char *szLicenseKey, char *szAccesscode, char *szLicstring);

/*----------------------------------------------------------------------*/
/* validateLicense -- check if it is ok to 
                 run (both valid, unexpired)
		 return codes are:
		 >0 = timestamp of when demo license expires (cast to time_t)
		 0 = permanent license
		 -1 = expired demo license
		 -2 = invalid access code
		 -3 = invalid license key
		 -4 = argument error (wrong size?)
		 */
long validateLicense(unsigned long *lWeeks,
					FEATURE_TYPE *feature_flags_ret, unsigned long *lVersion);

// function wrapper to work with existing pmd, rmd code
int check_key(char **keyl, unsigned short* np_crypt_key, int crypt_key_len);


#ifdef __cplusplus
}
#endif


#endif