// Server.h : Declaration of the CComServer

#ifndef __SERVER_H_
#define __SERVER_H_

#include "ReplicationGroup.h"
#include "ScriptServer.h"
#include "Event.h"
#include "States.h"


/////////////////////////////////////////////////////////////////////////////
//

class CDomain;
class CDeviceList;
class FsTdmfDb;
class FsTdmfRecSrvInf;
class MMP_API;


/////////////////////////////////////////////////////////////////////////////
//

class CJournalDrive
{
public:
	__int64 m_liJournalTotalSize;
	__int64 m_liDiskTotalSize;
    __int64 m_liDiskFreeSize;

	CJournalDrive() : m_liJournalTotalSize(0), m_liDiskTotalSize(0), m_liDiskFreeSize(0) {}
};


/////////////////////////////////////////////////////////////////////////////
// CServer

class CServer
{
public:
	BOOL FindTheScriptFileInTheList(CString strFilename);

	enum Stats {
		eBABEntries = 0,
		ePctBABFull = 1,
		ePctDone    = 2,
		eReadBytes  = 3,
		eWriteBytes = 4,
		eBABSectors = 5,
	};

	std::string m_strName;
	std::vector<std::string> m_vecstrIPAddress;
 	std::string m_strOSType;
	std::string m_strOSVersion;
	std::string m_strAgentVersion;
	std::string m_strKeyExpirationDate;
	std::string m_strPStoreFile;
	std::string m_strJournalDirectory;
	std::string m_strKey;
	std::string m_strDescription;
	enum ElementStates m_eState;
	BOOL        m_bConnected;
	long        m_nBABSizeReq;
	long        m_nBABSizeAct;
	long        m_nTCPWndSize;
	long        m_nPort;
	long        m_nReplicationPort;
	long        m_nHostID;
	long        m_nRAMSizeKB;
	long        m_nNbrCPU;
	long        m_nPctBAB;
	long        m_nEntries;
	__int64     m_liPStoreSize;
	CDomain*    m_pParent;
	//TDMF DB : index of this Server in the t_ServerInfo table.  
    //          Allows fast access to DB record.
    int         m_iDbSrvId;
	int         m_iDbDomainFk;

	std::string m_strCmdHistory;
	std::string m_strLastCmdOutput;

	CEventList  m_EventList;
	//EventListIterator m_EventListIterator;

	std::map<std::string, CJournalDrive> m_mapJournalDrive;

	bool        m_bLockCmds;

	CServer() : m_eState(STATE_UNDEF), m_bConnected(FALSE), m_nBABSizeReq(0), m_nBABSizeAct(0), m_nTCPWndSize(0), 
        m_nPort(0), m_nReplicationPort(0), m_nHostID(0), m_nRAMSizeKB(0), m_pParent(NULL), m_nPctBAB(0), 
        m_nEntries(0), m_liPStoreSize(0), m_bLockCmds(false)
	{
 	}

    std::list <CScriptServer> m_listScriptServerFile;
 	std::list <CReplicationGroup> m_listReplicationGroup;
 
	CReplicationGroup& AddNewReplicationGroup();
    CScriptServer& AddNewScriptServerFile();

    bool RemoveScriptServerFile(long nDbScriptSrvId);
    long ImportAllScriptServerFiles(BOOL bOverwriteExistingFile,char* strFileExtension);
    long ImportOneScriptServerFile(char* strFilename);
    BOOL IsValidScriptServerFileName(const char* strScriptServerFilename, long& lFileType);
    BOOL FindTheScriptFilesMissingOnServer(CStringArray& arrStrFilename, std::list<CScriptServer>& listImportScriptServer);

    /**
     * Initialize object from provided TDMF database record
     *
     * @param bRecurse : indicate if the list of replication groups is to be filled or not.
     */
    bool Initialize(FsTdmfRecSrvInf *pRecord, bool bRecurse);

    /**
     * Fill the list of replication groups according to the TDMF DB
     * Object members must be already initialized (prior call to Initialize()).
     */
    bool InitializeGroups(bool bRecurseForPairs, FsTdmfRecSrvInf *pRecSrv = 0);

   /**
     * Fill the list of Script Server Object according to the TDMF DB
     */
    bool InitializeScriptServers(FsTdmfRecSrvInf *pRecSrv = 0);


    /**
     * Register the current server to the collector (for the perfomance notifications)
     */
	bool EnablePerformanceNotifications(BOOL bEnable = TRUE);

    /**
     * Move the current server to the specified domain
     */
	int MoveToDomain(long nKeyDomain, CServer**);

	/**
	 * Stats computation functions
	 */
	long GetTargetReplicationGroupCount();
	long GetReplicationPairCount();
	long GetTargetReplicationPairCount();
    long GetScriptServerFileCount();
	/**
     * Add/Update this server information in the DB Server table.
	 * Also send the appropriate TDMF commands to the agent
     */
    int SaveToDB();
    
	int RemoveFromDB();

	long LaunchCommand(enum tdmf_commands eCmd, char* pszOptions, char* pszLog, CString& Message);

	int GetDeviceList(CDeviceList**, long nHostIDTarget = 0, CDeviceList** ppDeviceListTarget = NULL);

    FsTdmfDb* GetDB();
	MMP_API*  GetMMP();

	// Events (Alerts)
	CEvent* GetEventFirst();
	CEvent* GetEventAt     ( long nIndex ); // ardev 021128
	long    GetEventCount  ();              // ardev 021128
	BOOL    IsEventAt      ( long nIndex ); // ardev 021128

	// Import a group config from server
	long ImportReplGroup(std::string& strErrMsg);
	BOOL ImportOneReplGroup(CReplicationGroup& RepGrp, std::string& strPrimaryHostName, std::string& strTargetHostName, std::map<UINT, CDeviceList*> mappDeviceList);

 
	long GetKey() const
	{
		return m_iDbSrvId;
	}

	bool operator==(const CServer& Server) const
	{
		return (GetKey() == Server.GetKey());
	}

	void operator=(const CServer& Server)
	{
		m_strName              = Server.m_strName;
		m_vecstrIPAddress      = Server.m_vecstrIPAddress;
		m_strOSType            = Server.m_strOSType;
		m_strOSVersion         = Server.m_strOSVersion;
		m_strAgentVersion      = Server.m_strAgentVersion;
		m_strKeyExpirationDate = Server.m_strKeyExpirationDate;
		m_strPStoreFile        = Server.m_strPStoreFile;
		m_strJournalDirectory  = Server.m_strJournalDirectory;
		m_strKey               = Server.m_strKey;
		m_strDescription       = Server.m_strDescription;
		m_eState               = Server.m_eState;
		m_bConnected           = Server.m_bConnected;
		m_nBABSizeReq          = Server.m_nBABSizeReq;
		m_nBABSizeAct          = Server.m_nBABSizeAct;
		m_nTCPWndSize          = Server.m_nTCPWndSize;
		m_nPort                = Server.m_nPort;
		m_nReplicationPort     = Server.m_nReplicationPort;
		m_nHostID              = Server.m_nHostID;
		m_nRAMSizeKB           = Server.m_nRAMSizeKB;
		m_nNbrCPU              = Server.m_nNbrCPU;
		m_nPctBAB              = Server.m_nPctBAB;
		m_nEntries             = Server.m_nEntries;
		m_liPStoreSize         = Server.m_liPStoreSize;
		//m_pParent              = Server.m_pParent;
		m_iDbSrvId             = Server.m_iDbSrvId;
		m_iDbDomainFk          = Server.m_iDbDomainFk;
		m_strCmdHistory        = Server.m_strCmdHistory;
		m_strLastCmdOutput     = Server.m_strLastCmdOutput;
		m_EventList            = Server.m_EventList;
		m_mapJournalDrive      = Server.m_mapJournalDrive;
		m_listReplicationGroup = Server.m_listReplicationGroup;
    
        m_listScriptServerFile = Server.m_listScriptServerFile;
    	std::list<CScriptServer>::iterator it;
		for(it = m_listScriptServerFile.begin(); it != m_listScriptServerFile.end(); it++)
		{
			it->m_pServer = this;
		}

	}

	class CStat
	{
	public:
		int m_nStatId;
		std::vector<CPoint> m_vecValues;
	};

	class CStatsInfo
	{
	public:
		int m_nGroupId;
		std::list<CStat> m_listStat;
	};

	void ParseXMLQuery(CString cstrXML, std::list<CStatsInfo>& listStats);
	void FillStats(std::list<CStatsInfo>& listStats, DATE dateBegin, DATE dateEnd);
	void CreateXMLResult(std::list<CStatsInfo>& listStats, CString& cstrRetVal);

	CString GetPerformanceValues(CString Stats, DATE dateBegin, DATE dateEnd);

	void LockCmds();
	void UnlockCmds();
};

#endif //__SERVER_H_
