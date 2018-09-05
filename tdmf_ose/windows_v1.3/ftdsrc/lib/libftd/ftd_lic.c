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
#include "license.h"
/*
 * ftd_lic_verify -- verify license in license file 
 */
int
ftd_lic_verify()
{
	char    szLic[_MAX_PATH];
	char    **pTable,*pWk[2];
	int     rc;

    pWk[0] = szLic;
    pWk[1] = 0;//to avoid problem caused by assumption in check_key()
    pTable = &pWk[0];

	memset(szLic, 0, _MAX_PATH);

	getLicenseKeyFromReg(szLic);
	rc = check_key(pTable, np_crypt_key_pmd, NP_CRYPT_KEY_LEN);

	if (LICKEY_OK != rc)
	{
		reporterr(ERRFAC, M_LICGENERR, ERRCRIT);
	}

    return rc;
}

