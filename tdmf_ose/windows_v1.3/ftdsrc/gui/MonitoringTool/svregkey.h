// SVRegKey.h: interface for the SVRegKey class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SVRegKey_H__EFA42180_F31C_11D1_8493_1473F9C0E008__INCLUDED_)
#define AFX_SVRegKey_H__EFA42180_F31C_11D1_8493_1473F9C0E008__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#define ENCRYPT_TIMELINE 5
#define ENCRYPT_PASSWORD 3
#define ENCRYPT_CPASSWORD 2
#include "MonitorRes.h"   
#include "svGlobal.h"     

class SVRegKey  
{
public:
	CString GetValueEncrypt(int nMask, CString csSubKey, CString csDefault="");
	long SetValueEncrypt(int nMask, CString csSubKey, CString csValue);
	CString GetValue(CString csSubKey, CString csDefault="");
	long CloseRegKey();
	CString FormatSysMessage(long lMsg, CString csFunc);
	long SetValue(CString csSubKey, CString csValue);
	long SetValue(CString csSubKey, int nValue);
	long SetValueBinary(CString csSubKey, CString csValue);
	long SetValueBinary(CString csSubKey, long lValue);
	SVRegKey();
	virtual ~SVRegKey();
	long OpenRegKey(CString csKey = RESCHAR(IDS_REGISTRYNAME), HKEY hKey = HKEY_CURRENT_USER);
	CString GetSubKeys(int nIndex);
	long DelKey(CString csKeyname);
protected:
	HKEY m_hKey;
	HKEY m_hSubKey;
	DWORD m_dwDisp;
	char* m_pBigbuff;
};

#endif // !defined(AFX_SVRegKey_H__EFA42180_F31C_11D1_8493_1473F9C0E008__INCLUDED_)
