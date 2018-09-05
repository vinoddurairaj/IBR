// SVRegKey.cpp: implementation of the SVRegKey class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SVRegKey.h"
#include "MonitorRes.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#define BUFFSIZE 2000
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SVRegKey::SVRegKey()
{
	m_hKey = 0;
	m_pBigbuff = 0;
}


SVRegKey::~SVRegKey()
{
	CloseRegKey();
	delete [] m_pBigbuff;
}

long SVRegKey::OpenRegKey(CString csKey, HKEY hKey)
{
	long rc;
	rc =  RegCreateKeyEx(hKey, 
		(LPCTSTR)csKey,
//		_T("Software\\Softek\\Storage Manager\\Space Manager"),
		0,
		_T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 
		0, &m_hKey, &m_dwDisp);
	if (rc != ERROR_SUCCESS)
		AfxMessageBox(FormatSysMessage(rc, _T("OpenRegKey")));
	return rc; 

}


long SVRegKey::SetValue(CString csSubKey, int nValue)
{
	CString cs;
	cs.Format(_T("%d"), nValue);
	return SetValue(csSubKey, cs);
}
long SVRegKey::SetValue(CString csSubKey, CString csValue)
{
	ASSERT(m_hKey != 0);
	long rc = 0;	
	if (csValue.IsEmpty())
	{
		rc = RegDeleteValue(m_hKey, csSubKey);
		if (rc == ERROR_FILE_NOT_FOUND)	// key not there (this is ok, trying to delete a key that is not there)
			rc = ERROR_SUCCESS;
	}  
	else
	{
		rc = RegSetValueEx(m_hKey, csSubKey, 0, REG_SZ,
			(UCHAR*)(LPCTSTR)csValue, (csValue.GetLength()+1) * sizeof(TCHAR)); // must be as ANSI, will be converted to UNICODE by windows
	}
	if (rc == ERROR_SUCCESS)
		return rc;
	AfxMessageBox(FormatSysMessage(rc, "SetValue"));
	return rc; 
}

long SVRegKey::SetValueBinary(CString csSubKey, CString csValue)
{
	ASSERT(m_hKey != 0);
	long rc = 0;	
	if (csValue.IsEmpty())
	{
		rc = RegDeleteValue(m_hKey, (LPCTSTR)csSubKey);
		if (rc == ERROR_FILE_NOT_FOUND)	// key not there (this is ok, trying to delete a key that is not there)
			rc = ERROR_SUCCESS;
	}  
	else
	{
		rc = RegSetValueEx(m_hKey, (LPCTSTR)csSubKey, 0, REG_BINARY,
			(UCHAR*)(LPCTSTR)csValue, csValue.GetLength());
	}
	if (rc == ERROR_SUCCESS)
		return rc;
	AfxMessageBox(FormatSysMessage(rc, _T("SetValueBinary")));
	return rc; 
}

long SVRegKey::SetValueBinary(CString csSubKey, long lValue)
{
	CString csValue;
	csValue.Format(_T("%u"),lValue);
	return SetValueBinary(csSubKey, csValue);
}


CString SVRegKey::GetValue(CString csSubKey, CString csDefault)
{
	ASSERT(m_hKey != 0);
	long rc;

	unsigned long lType;
	DWORD dwBuffsize = BUFFSIZE;
	TCHAR buff[BUFFSIZE+1];
	rc = RegQueryValueEx(m_hKey, (LPCTSTR)csSubKey, 0, &lType,
		(UCHAR*)buff, &dwBuffsize);
	if (rc == ERROR_SUCCESS)
	{
		buff[dwBuffsize] = '\0';
		return buff;
	}

	if (rc == ERROR_MORE_DATA)
	{
		m_pBigbuff = new char[dwBuffsize+1];

		rc = RegQueryValueEx(m_hKey, (LPCTSTR)csSubKey, 0, &lType,
			(UCHAR*)m_pBigbuff, &dwBuffsize);
		if (rc == ERROR_SUCCESS)
		{
			m_pBigbuff[dwBuffsize] = '\0';
			return m_pBigbuff;
		}
	}
	return csDefault;

}

long SVRegKey::CloseRegKey()
{
	if (m_hKey == 0)
		return 0;
	long rc;
	rc = RegCloseKey(m_hKey);
	m_hKey = 0;
	if (rc == ERROR_SUCCESS)
		return rc;
	AfxMessageBox(FormatSysMessage(rc, "CloseRegKey"));
	return rc; 
}

CString SVRegKey::FormatSysMessage(long lMsg, CString csFunc)
{
	CString csReturn;
TCHAR pBuff[257];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 
	NULL,
	lMsg,
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language,
	pBuff,
	256, // DWORD nSize, 
	0); //va_list *Arguments  
//	csReturn.Format(MAKEINTRESOURCE(IDS_REGERROR), csFunc);
	csReturn += pBuff;
	return csReturn;
}

long SVRegKey::SetValueEncrypt(int nMask, CString csSubKey, CString csValue)
{
	long rc = 0;	
	if (csValue.IsEmpty())
	{
		rc = RegDeleteValue(m_hKey, (LPCTSTR)csSubKey);
		if (rc == ERROR_FILE_NOT_FOUND)	// key not there
			rc = ERROR_SUCCESS;
	}  
	else
	{
		CString csWork;
		for (int i=0; i<csValue.GetLength(); i++)
		{ // note, this must (reverse) match GetValueEncrypt
			csValue.SetAt(i,csValue[i] ^ nMask);
			csValue.SetAt(i,csValue[i] ^ 1);
			csValue.SetAt(i,csValue[i] ^ 'F');
			csValue.SetAt(i,csValue[i] ^ 3);
		}
		rc = RegSetValueEx(m_hKey, (LPCTSTR)csSubKey, 0, REG_BINARY,
			(UCHAR*)(LPCTSTR)csValue, csValue.GetLength());
	}
	if (rc == ERROR_SUCCESS)
		return rc;
	AfxMessageBox(FormatSysMessage(rc, "SetValueE"));
	return rc; 

}
CString SVRegKey::GetValueEncrypt(int nMask, CString csSubKey, CString csDefault)
{
	ASSERT(m_hKey != 0);
	long rc;

	unsigned long lType;
	DWORD dwBuffsize = BUFFSIZE;
	char buff[BUFFSIZE+1];
	rc = RegQueryValueEx(m_hKey, (LPCTSTR)csSubKey, 0, &lType,
		(UCHAR*)buff, &dwBuffsize);
	if (rc == ERROR_SUCCESS)
	{
		for (int i=0; (UINT)i<dwBuffsize; i++)
		{ // note, this must (reverse) match SetValueEncrypt
			buff[i] =  buff[i] ^ 3;
			buff[i] =  buff[i] ^ 'F';
			buff[i] =  buff[i] ^ 1;
			buff[i] =  buff[i] ^ nMask;
		}
		buff[i] = '\0';
		return buff;
	}
	return csDefault;
}
// obtain subkeys of current opened key
// returns one key per call
// caller must increment nIndex, start with 0
// returns empty string when done
CString SVRegKey::GetSubKeys(int nIndex)
{
	CString cs;
	ASSERT(m_hKey != 0);
	long rc;

	DWORD dwSubkeyNameLen = 255;  // note: must limit entry length 
	TCHAR szSubkeyName[255];
	rc = RegEnumKeyEx(m_hKey,nIndex,szSubkeyName,&dwSubkeyNameLen,
		NULL,NULL,NULL,NULL);
	if (rc == ERROR_SUCCESS)
	{
		szSubkeyName[dwSubkeyNameLen] = '\0';
		return szSubkeyName;
	}
	else if (rc != ERROR_NO_MORE_ITEMS)
		AfxMessageBox(FormatSysMessage(rc, "GetSubKeys: "));

	return "";
}


long SVRegKey::DelKey(CString csKeyname)
{
	ASSERT(m_hKey != 0);
	long rc;
	rc = RegDeleteKey(m_hKey,(LPCTSTR) csKeyname);

	if (rc == ERROR_SUCCESS)
		return rc;
	AfxMessageBox(FormatSysMessage(rc, "DelKey: "));
	return rc; 
}

