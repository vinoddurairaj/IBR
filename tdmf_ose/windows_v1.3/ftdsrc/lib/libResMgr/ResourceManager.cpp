// ResourceManager.cpp: implementation of the CResourceManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResourceManager.h"


//////////////////////////////////////////////////////////////////////
//

HINSTANCE CResourceManager::GetExtendedResourceHandle()
{
	if (m_hResource == NULL)
	{
		m_hResource = LoadLibrary(m_cstrResourceDllName);
	}
	
	return m_hResource;
}

UINT CResourceManager::InternalLoadString(UINT nID, LPTSTR lpszBuf, UINT nMaxBuf)
{
	HINSTANCE hResource = GetExtendedResourceHandle();
	if (hResource == NULL)
	{
		hResource = AfxGetResourceHandle();
	}
	
	int nLen = ::LoadString(hResource, nID, lpszBuf, nMaxBuf);
	if ((nLen == 0)  && (hResource != AfxGetResourceHandle()))
	{
		nLen = ::LoadString(AfxGetResourceHandle(), nID, lpszBuf, nMaxBuf);
	}
	
	if (nLen == 0)
	{
		lpszBuf[0] = '\0';
	}
	
	return nLen;
}

void CResourceManager::LoadVersionBuildInfo()
{
    // Get values from resource for the version

	// First try to find info in resource dll
	if (m_cstrResourceDllName.GetLength() > 0)
	{
		DWORD dw;
		DWORD size = GetFileVersionInfoSize((LPTSTR)(LPCTSTR)m_cstrResourceDllName, &dw);
		char* pVer = new char[size];

		if (GetFileVersionInfo((LPTSTR)(LPCTSTR)m_cstrResourceDllName, 0, size, pVer))
		{
			LPVOID pBuff;
			UINT cbBuff;

			//assuming English as default res language ...
			VerQueryValue(pVer, "\\StringFileInfo\\040904B0\\ProductVersion", &pBuff, &cbBuff); 
			m_cstrProductVersion = (LPTSTR)pBuff;

			VerQueryValue(pVer, "\\StringFileInfo\\040904B0\\SpecialBuild", &pBuff, &cbBuff); 
		    m_cstrProductBuild = (LPTSTR)pBuff;
		}

		delete [] pVer;
	}

	// Second, if no Product Version was found, use the main application's product version
	if (m_cstrProductVersion.GetLength() == 0)
	{
		char szFileName[_MAX_PATH];
		GetModuleFileName(NULL, szFileName, _MAX_PATH);

	    DWORD dw;
		DWORD size = GetFileVersionInfoSize(szFileName, &dw);
		char* pVer = new char[size];

	    if (GetFileVersionInfo(szFileName, 0, size, pVer))
		{
			LPVOID pBuff;
			UINT cbBuff;

			//assuming English as default res language ...
			VerQueryValue(pVer, "\\StringFileInfo\\040904B0\\ProductVersion", &pBuff, &cbBuff); 
			m_cstrProductVersion = (LPTSTR)pBuff;

			VerQueryValue(pVer, "\\StringFileInfo\\040904B0\\SpecialBuild", &pBuff, &cbBuff); 
			m_cstrProductBuild = (LPTSTR)pBuff;
		}

		delete [] pVer;
	}
}

HICON CResourceManager::LoadIcon(UINT nID)
{
	HINSTANCE hResource = GetExtendedResourceHandle();
	if (hResource == NULL)
	{
		hResource = AfxGetResourceHandle();
	}
	
	HICON hIcon = ::LoadIcon(hResource, MAKEINTRESOURCE(nID));
	if ((hIcon == NULL) && (hResource != AfxGetResourceHandle()))
	{
		hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(nID));
	}
	
	return hIcon;
}

HBITMAP CResourceManager::LoadBitmap(UINT nID)
{
	HINSTANCE hResource = GetExtendedResourceHandle();
	if (hResource == NULL)
	{
		hResource = AfxGetResourceHandle();
	}
	
	HBITMAP hBitmap = ::LoadBitmap(hResource, MAKEINTRESOURCE(nID));
	if ((hBitmap == NULL) && (hResource != AfxGetResourceHandle()))
	{
		hBitmap = ::LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(nID));
	}
	
	return hBitmap;
}

CString CResourceManager::LoadString(UINT nID)
{
	CString cstrVal;
	
	// try fixed buffer first (to avoid wasting space in the heap)
	TCHAR szTemp[256];
	int nLen = InternalLoadString(nID, szTemp, 256);
	if (nLen < 255) // don't count the endind '\0'
	{
		cstrVal = szTemp;
		return cstrVal;
	}
	
	// try buffer size of 512, then larger size until entire string is retrieved
	int nSize = 256;
	do
	{
		nSize += 256;
		nLen = InternalLoadString(nID, cstrVal.GetBuffer(nSize-1), nSize);
	} while (nLen >= nSize);
	cstrVal.ReleaseBuffer();
	
	return cstrVal;
}

CResourceManager::CResourceManager(LPSTR lpcstrResDllName) :
	m_hResource(NULL), m_cstrResourceDllName(lpcstrResDllName),	m_hIcon(NULL),
	m_hBitmapSplash(NULL), m_hBitmapTreeViewLogo(NULL), m_hBitmapAboutBoxLogo(NULL)
{
	if (lpcstrResDllName != NULL)
	{
		SetResourceDllName(lpcstrResDllName);
	}
}

CResourceManager::~CResourceManager()
{
	if (m_hResource != NULL)
	{
		FreeLibrary(m_hResource);
	}

	if (m_hIcon != NULL)
	{
		DeleteObject(m_hIcon);
	}

	if (m_hBitmapSplash != NULL)
	{
		DeleteObject(m_hBitmapSplash);
	}

	if (m_hBitmapTreeViewLogo != NULL)
	{
		DeleteObject(m_hBitmapTreeViewLogo);
	}

	if (m_hBitmapAboutBoxLogo != NULL)
	{
		DeleteObject(m_hBitmapAboutBoxLogo);
	}
}

void CResourceManager::SetResourceDllName(LPCSTR lpcstrResDllName)
{
	m_cstrResourceDllName = lpcstrResDllName;

	if (m_hResource != NULL)
	{
		FreeLibrary(m_hResource);
	}
	m_hResource = NULL;
}

LPCSTR CResourceManager::GetProductName()
{
	if (m_cstrProductName.GetLength() == 0)
	{
		m_cstrProductName = LoadString(strProductName);
	}

	return m_cstrProductName;
}

LPCSTR CResourceManager::GetFullProductName()
{
	if (m_cstrFullProductName.GetLength() == 0)
	{
		m_cstrFullProductName = LoadString(strFullProductName);
	}

	return m_cstrFullProductName;
}

LPCSTR CResourceManager::GetCompanyName()
{
	if (m_cstrCompanyName.GetLength() == 0)
	{
		m_cstrCompanyName = LoadString(strCompanyName);
	}

	return m_cstrCompanyName;
}

LPCSTR CResourceManager::GetCopyrightNotice()
{
	if (m_cstrCopyrightNotice.GetLength() == 0)
	{
		m_cstrCopyrightNotice = LoadString(strCopyrightNotice);
	}

	return m_cstrCopyrightNotice;
}

HICON CResourceManager::GetApplicationIcon()
{
	if (m_hIcon == NULL)
	{
		m_hIcon = LoadIcon(CResourceManager::icoAppLogo);
	}

	return m_hIcon;
}

HBITMAP CResourceManager::GetSplashScreen()
{
	if (m_hBitmapSplash == NULL)
	{
		m_hBitmapSplash = LoadBitmap(CResourceManager::bmpSplashScreen);
	}

	return m_hBitmapSplash;
}

HBITMAP CResourceManager::GetTreeViewLogo()
{
	if (m_hBitmapTreeViewLogo == NULL)
	{
		m_hBitmapTreeViewLogo = LoadBitmap(CResourceManager::bmpTreeBkgrdLogo);
	}

	return m_hBitmapTreeViewLogo;
}

HBITMAP CResourceManager::GetAboutBoxLogo()
{
	if (m_hBitmapAboutBoxLogo == NULL)
	{
		m_hBitmapAboutBoxLogo = LoadBitmap(CResourceManager::bmpAboutBox);
	}

	return m_hBitmapAboutBoxLogo;
}

LPCSTR CResourceManager::GetProductVersion()
{
	if (m_cstrProductVersion.GetLength() == 0)
	{
		LoadVersionBuildInfo();
	}

	return m_cstrProductVersion;
}

LPCSTR CResourceManager::GetProductBuild()
{
	if (m_cstrProductVersion.GetLength() == 0)
	{
		LoadVersionBuildInfo();
	}

	return m_cstrProductBuild;
}
