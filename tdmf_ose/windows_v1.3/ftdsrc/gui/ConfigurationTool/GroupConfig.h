// GroupConfig.h: interface for the CGroupConfig class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GROUPCONFIG_H__336CEF0C_2367_45D4_BEF5_9FC3157282CB__INCLUDED_)
#define AFX_GROUPCONFIG_H__336CEF0C_2367_45D4_BEF5_9FC3157282CB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>


class CDevicePairConfig
{
public:	
	CString m_cstrDTCDev;
	CString m_cstrRemarks;
	CString m_cstrDataDev;
	CString m_cstrMirrorDev;
	int     m_nNumDTCDevices;
};


class CGroupConfig
{
	// Tmp saved values
protected:
	CString m_cstrNoteSaved;

protected:
	UINT    m_nGroupNb;
	CString m_cstrFilename;
	
	CString m_cstrPrimaryHost;
	CString m_cstrPStore;
	CString m_cstrPrimaryPort;
	CString m_cstrSecondaryHost;
	CString m_cstrJournal;
	CString m_cstrSecondaryPort;
	bool    m_bChaining;
	CString m_cstrNote;

	std::vector<CDevicePairConfig> m_vecDevicePair;

	bool    m_bSyncMode;
	CString m_cstrSyncDepth;
	CString m_cstrSyncTimeout;
	bool    m_bCompression;
	bool    m_bRefreshNeverTimeout;
	CString m_cstrRefreshTimeout;
	bool    m_bJournalLess;

protected:
	void ParseFile(char *pdest, CString& cstrReadValue);
	void ReadNote();
	void ReadSysValues(CFile *file, int iFileSize);
	void ReadDTCDevices(CFile *file, int iFileSize);
	bool ReadTunablesFromBatFile();
	bool SaveTunablesToBatFile();

public:
	CGroupConfig() : m_nGroupNb(0), m_bChaining(false) {}
	CGroupConfig(UINT nGroupNb, LPCSTR lpcstrFilename) :
		m_nGroupNb(nGroupNb), m_cstrFilename(lpcstrFilename), m_bChaining(false),
		m_bSyncMode(false), m_cstrSyncDepth("1"), m_cstrSyncTimeout("30"), m_bCompression(false),
		m_bRefreshNeverTimeout(false), m_cstrRefreshTimeout("0"), m_bJournalLess(false)
	{
		// Read note
		ReadNote();
	}
	virtual ~CGroupConfig() {}

	void operator=(const CGroupConfig& GroupConfig)
	{
		m_cstrNoteSaved     = GroupConfig.m_cstrNoteSaved;
		m_nGroupNb          = GroupConfig.m_nGroupNb;
		m_cstrFilename      = GroupConfig.m_cstrFilename;		
		m_cstrPrimaryHost   = GroupConfig.m_cstrPrimaryHost;
		m_cstrPStore        = GroupConfig.m_cstrPStore;
		m_cstrPrimaryPort   = GroupConfig.m_cstrPrimaryPort;
		m_cstrSecondaryHost = GroupConfig.m_cstrSecondaryHost;
		m_cstrJournal       = GroupConfig.m_cstrJournal;
		m_cstrSecondaryPort = GroupConfig.m_cstrSecondaryPort;
		m_bChaining         = GroupConfig.m_bChaining;
		m_cstrNote          = GroupConfig.m_cstrNote;
		m_vecDevicePair     = GroupConfig.m_vecDevicePair;
		m_bSyncMode         = GroupConfig.m_bSyncMode;
		m_cstrSyncDepth     = GroupConfig.m_cstrSyncDepth;
		m_cstrSyncTimeout   = GroupConfig.m_cstrSyncTimeout;
		m_bCompression      = GroupConfig.m_bCompression;
		m_bRefreshNeverTimeout = GroupConfig.m_bRefreshNeverTimeout;
		m_cstrRefreshTimeout   = GroupConfig.m_cstrRefreshTimeout;
		m_bJournalLess      = GroupConfig.m_bJournalLess;
	}

	int ReadConfigFile();
	void SaveInitialValues();
	void RestoreInitialValues();

	UINT GetGroupNb() {return m_nGroupNb;}

	CString GetPrimaryHost()   {return m_cstrPrimaryHost;}
	CString GetPStore()        {return m_cstrPStore;}
	CString GetPrimaryPort()   {return m_cstrPrimaryPort;}
	CString GetSecondaryHost() {return m_cstrSecondaryHost;}
	CString GetJournal()       {return m_cstrJournal;}
	CString GetSecondaryPort() {return m_cstrSecondaryPort;}
	bool    GetChaining()      {return m_bChaining;}
	CString GetNote()          {return m_cstrNote;}
	bool    GetCompression()   {return m_bCompression;}
	bool    GetSyncMode()      {return m_bSyncMode;}
	CString GetSyncDepth()     {return m_cstrSyncDepth;}
	CString GetSyncTimeout()   {return m_cstrSyncTimeout;}
	bool    GetRefreshNeverTimeout()  {return m_bRefreshNeverTimeout;}
	CString GetRefreshInterval(){return m_cstrRefreshTimeout;}
	bool    GetJournalLess()   {return m_bJournalLess;}


	void SetPrimaryHost(CString cstrPrimaryHost)     {m_cstrPrimaryHost = cstrPrimaryHost;}
	void SetPStore(CString cstrPStore)               {m_cstrPStore = cstrPStore;}
	void SetPrimaryPort(CString cstrPrimaryPort)     {m_cstrPrimaryPort = cstrPrimaryPort;}
	void SetNote(CString cstrNote)                   {m_cstrNote = cstrNote;}
	void SetSecondaryHost(CString cstrSecondaryHost) {m_cstrSecondaryHost = cstrSecondaryHost;}
	void SetSecondaryPort(CString cstrSecondaryPort) {m_cstrSecondaryPort = cstrSecondaryPort;}
	void SetJournal(CString cstrJournal)             {m_cstrJournal = cstrJournal;}
	void SetChaining(bool bChaining)                 {m_bChaining = bChaining;}
	void SetCompression(bool bCompression)           {m_bCompression = bCompression;}
	void SetSyncMode(bool bSyncMode)                 {m_bSyncMode = bSyncMode;}
	void SetSyncDepth(CString cstrSyncDepth)         {m_cstrSyncDepth = cstrSyncDepth;}
	void SetSyncTimeout(CString cstrSyncTimeout)     {m_cstrSyncTimeout = cstrSyncTimeout;}
	void SetRefreshNeverTimeout(bool bRefreshNeverTimeout) {m_bRefreshNeverTimeout = bRefreshNeverTimeout;}
	void SetRefreshInterval(CString cstrRefreshTimeout)    {m_cstrRefreshTimeout = cstrRefreshTimeout;}
	void SetJournalLess(bool bJournalLess)           {m_bJournalLess = bJournalLess;}

	int  GetPairCount() {return m_vecDevicePair.size();}
	CDevicePairConfig* GetPair(int nIndex) {return &m_vecDevicePair[nIndex];}
	bool RemovePair(int nIndex);
	int  AddPair(LPCSTR lpcstrDTCDev, LPCSTR lpcstrRemarks, LPCSTR lpcstrDataDev, LPCSTR lpcstrMirrorDev, int nNumDTCDevices);

	void SaveToFile();
	bool IsStarted();
	bool IsValid();

	bool ReadTunables();
	void SaveTunables();
};

#endif // !defined(AFX_GROUPCONFIG_H__336CEF0C_2367_45D4_BEF5_9FC3157282CB__INCLUDED_)
