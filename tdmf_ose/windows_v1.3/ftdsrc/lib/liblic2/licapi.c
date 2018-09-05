/*=============================================================================
 *  Replication Block License Key Utilities
 *
 *  K. Jute  -- March, 23 2000
 *
 *===========================================================================*/

// Mike Pollett
#include "../../tdmf.inc"

#include "windows.h"
#include "LicAPI.h"

void getLicenseKeyFromReg(char *szLicenseKey)
{
	HKEY	happ;
	DWORD	dwType = 0;
    DWORD	dwSize = _MAX_PATH;

    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_PATH,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                        REG_LICENSE_KEY,
                        NULL,
                        &dwType,
                        (BYTE*)szLicenseKey,
                        &dwSize) != ERROR_SUCCESS) {
			
		}
	}
	
	RegCloseKey(happ);
}

void writeLicenseKeyToReg(char *szLicenseKey)
{
	// Write info to the Reg
	HKEY	happ;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0,
                     KEY_SET_VALUE, &happ) == ERROR_SUCCESS) 
	{
		RegSetValueEx(happ, REG_LICENSE_KEY, 0, REG_SZ, szLicenseKey,
						strlen(szLicenseKey));
		
		RegCloseKey(happ);
	}
}

void parseLicenseKey(const char *szLicenseKey, char *szAccesscode, char *szLicstring)
{
	int	i;

	// parse key to access code and license
	for(i = 0; i != 8; i++)
		szAccesscode[i] = szLicenseKey[i];
	for(i = 9; i != 25; i++)
		szLicstring[i - 9] = szLicenseKey[i];
}

long validateLicense(unsigned long *lWeeks,
					FEATURE_TYPE *feature_flags_ret, unsigned long *lVersion)
{
	long			lRc = -4;
	char			szLicstring[17];
	char			szAccesscode[9];
	char			szLicenseKey[_MAX_PATH];

	memset(szLicstring, 0, sizeof(szLicstring));
	memset(szAccesscode, 0, sizeof(szAccesscode));
	memset(szLicenseKey, 0, sizeof(szLicenseKey));

	getLicenseKeyFromReg(szLicenseKey);

	parseLicenseKey(szLicenseKey, szAccesscode, szLicstring);

	lRc = checklicok(szLicstring, szAccesscode, lWeeks, feature_flags_ret, lVersion);

	return lRc;
}

int check_key(char **keyl, unsigned short* np_crypt_key, int crypt_key_len)
{
	unsigned long	ulWeeks;
	FEATURE_TYPE	feature_flags_ret;
	unsigned long	ulVersion;
	long			lRc;

	lRc = validateLicense(&ulWeeks, &feature_flags_ret, &ulVersion);

	return lRc;
}
