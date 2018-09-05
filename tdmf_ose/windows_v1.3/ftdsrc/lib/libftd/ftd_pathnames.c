/*
 * ftd_pathnames.c - FTD path names interface
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

// Mike Pollett
#include "../../tdmf.inc"

#include "ftd_pathnames.h"
#if defined ( _TLS_ERRFAC )

extern DWORD TLSIndexErrorString;
/*
 * Parse the system config file for something like: key=value;
 */
char*
ftd_nt_path(char* key)
{
    HKEY	happ;
	DWORD	dwType = 0;
    DWORD	dwSize = _MAX_PATH;
	char * path;
	path = TlsGetValue( TLSIndexErrorString );

    // Read the current state from the registry
    // Try opening the registry key:
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     FTD_SOFTWARE_KEY,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                        key,
                        NULL,
                        &dwType,
                        (BYTE*)path,
                        &dwSize) != ERROR_SUCCESS) {
			return NULL;
		}
	}
	else {
		return NULL;
	}
	
	RegCloseKey(happ);

	return path;
}



#else
__declspec( thread ) char path[_MAX_PATH];

/*
 * Parse the system config file for something like: key=value;
 */
char*
ftd_nt_path(char* key)
{
    HKEY	happ;
	DWORD	dwType = 0;
    DWORD	dwSize = sizeof(path);

    // Read the current state from the registry
    // Try opening the registry key:
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     FTD_SOFTWARE_KEY,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                        key,
                        NULL,
                        &dwType,
                        (BYTE*)path,
                        &dwSize) != ERROR_SUCCESS) {
			return NULL;
		}
	}
	else {
		return NULL;
	}
	
	RegCloseKey(happ);

	return path;
}

#endif