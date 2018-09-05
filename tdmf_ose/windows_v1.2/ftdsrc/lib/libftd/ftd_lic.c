/*
 * ftd_lic.c - verify license
 *
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#include "ftd_port.h"
#include "ftd_pathnames.h"
#include "ftd_error.h"
#if !defined(_OCTLIC)
#include "license.h"
#else
#include "LicAPI.h"
#endif
/*
 * ftd_lic_verify -- verify license in license file 
 */
int
ftd_lic_verify(char *facility)
{
#if !defined(_OCTLIC)
//	char **lickey, licfilname[MAXPATHLEN];
	char			*szLic;
	int rc = LICKEY_OK;
#else
	unsigned long	ulWeeks;
	FEATURE_TYPE	feature_flags_ret;
	unsigned long	ulVersion;
	long			lRc;
	int				iOEM = LICENSE_VERSION_REPLICATION_43;
#endif

#if !defined(_OCTLIC)

#if defined(_WINDOWS)
//	sprintf(licfilname, "%s\\%s.lic", PATH_CONFIG, CAPQ);
#else	
	sprintf(licfilname, "%s/%s.lic", PATH_CONFIG, CAPQ);
#endif
	/* perform license checking */
/*
	if (LICKEY_OK != (rc = get_license(licfilname, &lickey))) {
		switch (rc) {
        case LICKEY_FILEOPENERR:
            reporterr(ERRFAC, M_LICFILMIS, ERRCRIT, licfilname);
            break;
        case LICKEY_BADKEYWORD:
            reporterr(ERRFAC, M_LICBADWORD, ERRCRIT, facility);
            break;
        default:
            reporterr(ERRFAC, M_LICFILE, ERRCRIT, licfilname);
        }
        return -1;
    }
    if (lickey == NULL) {
        reporterr(ERRFAC, M_LICFILFMT, ERRCRIT, licfilname);
        return -1;
    }
    rc = check_key(lickey, np_crypt_key_pmd, NP_CRYPT_KEY_LEN);
    if (LICKEY_OK != rc) { 
        switch (rc) {
        case LICKEY_NULL:
        case LICKEY_EMPTY:
            reporterr(ERRFAC, M_LICFILE, ERRCRIT, licfilname);
            break;
        default:
            reporterr(ERRFAC, M_LICGENERR, ERRCRIT);
        }
		free(*lickey);
        return -1;
    }

	free(*lickey);
*/

	szLic = (char *)malloc(_MAX_PATH);
	
	memset(szLic, 0, _MAX_PATH);

//	getLicenseKeyFromReg(szLic);
//	rc = check_key(&szLic, np_crypt_key_pmd, NP_CRYPT_KEY_LEN);

//	if (LICKEY_OK != rc)
//	{
//		reporterr(ERRFAC, M_LICGENERR, ERRCRIT);
//	}

	free(szLic);

    return rc;

#else	
	// OctLic
#ifdef STK 
	iOEM = LICENSE_VERSION_STK_50;
#elif MTI
	iOEM = LICENSE_VERSION_MDS_50;
#else
	iOEM = LICENSE_VERSION_REPLICATION_43;
#endif

	lRc = validateLicense(&ulWeeks, &feature_flags_ret, &ulVersion);

	if(ulVersion != iOEM)
		lRc = -3;

	if(lRc < 0)
	{
		reporterr(ERRFAC, M_LICEXPIRE, ERRCRIT);
	}

	return lRc;

#endif
}

