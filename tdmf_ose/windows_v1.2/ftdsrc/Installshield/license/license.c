#include <windows.h>
#include <time.h>

#if defined(_OCTLIC)
#include "LicAPI.h"
#else
#include "license.h"
#endif

/*----------------------------------------------------------------------------*\
 * Function: DllMain()
 *
 *  Purpose: DLL Entry Point.
 *
 *    Input: hinstDLL    - Handle of DLL module
 *           fdwReason   - Reason for Calling function
 *                         Can be either of:
 *                            DLL_PROCESS_ATTACH
 *                            DLL_THREAD_ATTACH
 *                            DLL_PROCESS_DETACH
 *                            DLL_THREAD_DETACH
 *           lpvReserved - Reserved
 *
 *   Returns: 1 to the Window Application if function is successful
 *            0 otherwise.
 *
 *  Comments: MUST not be defined as _export function.  IF any initializations
 *            for the DLL needs to be carried out, carry out in this routine.
 *
\*----------------------------------------------------------------------------*/
BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason,
                           LPVOID lpvReserved)
{
   return 1;    // For our purpose, return back 1.
}

/*----------------------------------------------------------------------------*\
 *
 * Function: verify()
 *
 *  Purpose: 
 *    Input: 
 *  Returns: VOID (returns nothing)
 *
 * Comments:
 *
\*---------------------------------------------------------------------------*/
void WINAPI verify (int FAR* lpIValue, LPSTR szLicenseKey)
{
#if defined(_OCTLIC)
	unsigned long	ulWeeks;
	FEATURE_TYPE	feature_flags_ret;
	unsigned long	ulVersion;
	char			szAccesscode[9];
	char			szLicstring[17];
	struct tm		*gmt;

	parseLicenseKey(szLicenseKey, szAccesscode, szLicstring);

	*lpIValue = checklicok(szLicstring, szAccesscode, &ulWeeks, &feature_flags_ret, &ulVersion);
#else // no octlic
	char			*szLic;

	szLic = (char *)malloc(_MAX_PATH);
	
	memset(szLic, 0, _MAX_PATH);

	strcpy(szLic, szLicenseKey);

	*lpIValue = check_key(&szLic, np_crypt_key_pmd, NP_CRYPT_KEY_LEN);

	free(szLic);

#endif // octlic
}
