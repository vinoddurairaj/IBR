// ResourceManager.h: interface for the CResourceManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESOURCEMANAGER_H__3C098639_EB42_489C_BCAB_0393EB9D1169__INCLUDED_)
#define AFX_RESOURCEMANAGER_H__3C098639_EB42_489C_BCAB_0393EB9D1169__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Resource.h"


class CResourceManager
{
protected:
	HINSTANCE m_hResource;
	CString   m_cstrResourceDllName;

	HINSTANCE GetExtendedResourceHandle();
	UINT      InternalLoadString(UINT nID, LPTSTR lpszBuf, UINT nMaxBuf);
	void      LoadVersionBuildInfo();

protected:
	HICON     LoadIcon(UINT nID);
	HBITMAP   LoadBitmap(UINT nID);
	CString   LoadString(UINT nID);

	enum Resources
	{
		strProductName     = SFTK_IDS_PRODUCTNAME,
		strFullProductName = SFTK_IDS_FULLPRODUCTNAME,
		strCompanyName     = SFTK_IDS_COMPANYNAME,
		strCopyrightNotice = SFTK_IDS_COPYRIGHTNOTICE,
		bmpAboutBox        = SFTK_IDB_ABOUTBOX,
		bmpSplashScreen    = SFTK_IDB_SPLASHSCREEN,
		bmpTreeBkgrdLogo   = SFTK_IDB_TREEBKLOGO,
		icoAppLogo         = SFTK_IDI_APPLOGO
	};

protected:
	CString   m_cstrProductName;
	CString   m_cstrFullProductName;
	CString   m_cstrCompanyName;
	CString   m_cstrCopyrightNotice;

	HICON     m_hIcon;
	HBITMAP   m_hBitmapSplash;
	HBITMAP   m_hBitmapTreeViewLogo;
	HBITMAP   m_hBitmapAboutBoxLogo;

	CString   m_cstrProductVersion;
	CString   m_cstrProductBuild;

public:
	CResourceManager(LPSTR lpcstrResDllName = NULL);
	~CResourceManager();

	void      SetResourceDllName(LPCSTR lpcstrResDllName);

	LPCSTR    GetProductName();
	LPCSTR    GetFullProductName();
	LPCSTR    GetCompanyName();
	LPCSTR    GetCopyrightNotice();

	HICON     GetApplicationIcon();
	HBITMAP   GetSplashScreen();
	HBITMAP   GetTreeViewLogo();
	HBITMAP   GetAboutBoxLogo();

	LPCSTR    GetProductVersion();
	LPCSTR    GetProductBuild();
};


#endif // !defined(AFX_RESOURCEMANAGER_H__3C098639_EB42_489C_BCAB_0393EB9D1169__INCLUDED_)
