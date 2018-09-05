// ServerConfig.h: interface for the CServerConfig class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERCONFIG_H__A6F66733_66D9_4827_B791_B01A8D2A50FA__INCLUDED_)
#define AFX_SERVERCONFIG_H__A6F66733_66D9_4827_B791_B01A8D2A50FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GroupConfig.h"
#include <map>


class CServerConfig  
{
protected:
	bool m_bReboot;
	char m_szInstallPath[_MAX_PATH];
	int  m_nBABSize;
	int  m_nChunkSize;
	UINT m_nTcpWindowSize;
	UINT m_nPrimaryPortNumber;
	std::map<UINT, CGroupConfig> m_mapGroup;
	std::map<UINT, CGroupConfig>::iterator m_itGroup;
	bool m_bGroupRead;

protected:
	void GetRegConfigPath();
	void GetRegBabSize();
	void GetRegTCPWinSize();
	void GetRegPort();

public:
	CServerConfig();
	virtual ~CServerConfig();

	UINT GetMaxBABSize();

	void GetLicenseKey(CString& cstrLicenseKey, CString& cstrExpDate);
	void WriteLicenseKey(LPCSTR lpcstrLicenseKey);

	LPCSTR GetInstallPath()
	{
		return m_szInstallPath;
	}

	bool NeedToReboot()
	{
		return m_bReboot;
	}

	int GetBABSize()
	{
		return m_nBABSize;
	}

	void SetBABSize(int nBABSize)
	{
		if (nBABSize != m_nBABSize)
		{
			m_nBABSize = nBABSize;
			m_bReboot  = true;
		}
	}

	UINT GetTcpWindowSize()
	{
		return m_nTcpWindowSize;
	}

	void SetTcpWindowSize(UINT nTcpWindowSize)
	{
		m_nTcpWindowSize = nTcpWindowSize;
	}

	UINT GetPrimaryPortNumber()
	{
		return m_nPrimaryPortNumber;
	}

	void SetPrimaryPortNumber(UINT nPort)
	{
		m_nPrimaryPortNumber = nPort;
	}

	bool Save();

	int InitLicenseKey(CString& cstrDate);

	CGroupConfig* GetGroup(UINT nKey);
	CGroupConfig* GetFirstGroup();
	CGroupConfig* GetNextGroup();
	CGroupConfig* AddGroup(UINT nKey);
	bool          DeleteGroup(UINT nKey);

};

#endif // !defined(AFX_SERVERCONFIG_H__A6F66733_66D9_4827_B791_B01A8D2A50FA__INCLUDED_)
