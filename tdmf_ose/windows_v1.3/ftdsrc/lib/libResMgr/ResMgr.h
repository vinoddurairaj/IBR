#if !defined(RESMGR_H__INCLUDED_)
#define RESMGR_H__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Resource.h"

#define GET_FULLPRODUCTNAME_FROM_RESMGR(lpstrProductName, nMaxSize)				\
do																				\
{																				\
	char szFileName[_MAX_PATH];													\
	HMODULE hModule;															\
	char drive[_MAX_DRIVE];														\
	char dir[_MAX_DIR];															\
	char fname[_MAX_FNAME];														\
	char ext[_MAX_EXT];															\
																				\
	GetModuleFileName(NULL, szFileName, _MAX_PATH);								\
	_splitpath(szFileName, drive, dir, fname, ext);								\
	_makepath(szFileName, drive, dir, "RBRes", "dll" );							\
																				\
	hModule = LoadLibrary(szFileName);												\
	if (hModule != NULL)															\
	{																				\
		lpstrProductName[0] = '\0';													\
		LoadString(hModule, SFTK_IDS_FULLPRODUCTNAME, lpstrProductName, nMaxSize);	\
																					\
		FreeLibrary(hModule);														\
	}																				\
	else																			\
	{																				\
		lpstrProductName[0] = '\0';													\
		LoadString(GetModuleHandle(NULL), SFTK_IDS_FULLPRODUCTNAME, lpstrProductName, nMaxSize); \
	}																				\
} while(0);


#endif
