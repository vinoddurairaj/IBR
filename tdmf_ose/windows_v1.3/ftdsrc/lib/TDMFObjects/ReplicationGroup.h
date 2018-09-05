// ReplicationGroup.h : Declaration of the CReplicationGroup

#ifndef __REPLICATIONGROUP_H_
#define __REPLICATIONGROUP_H_

#include "ReplicationPair.h"
#include "Event.h"
#include "States.h"


class CServer;
class FsTdmfDb;
class FsTdmfRecLgGrp;
class MMP_API;

/////////////////////////////////////////////////////////////////////////////
// CReplicationGroup

//return VALUES for CReplicationGroup::IsValid()
#define REPGRP_OK                                   0
#define REPGRP_INVALID_PSTORE_FILENAME              ((int)1<<0)
#define REPGRP_INVALID_JOURNAL_DIR                  ((int)1<<1)
#define REPGRP_ERROR_NO_REPPAIR                     ((int)1<<2)
#define REPGRP_GROUP_NUMBER_ALREADY_EXIST_SOURCE    ((int)1<<3)
#define REPGRP_GROUP_NUMBER_ALREADY_EXIST_TARGET    ((int)1<<4)
#define REPGRP_NO_TARGET_SERVER_SPECIFIED           ((int)1<<5)


class CReplicationGroup
{
public:

	enum ObjectState
	{
		RPO_SAVED    = 0,
		RPO_DELETED  = 1,
		RPO_MODIFIED = 2,
		RPO_NEW      = 3,
	};

	enum ReplicationGroupType
	{
		GT_SOURCE   = 0x01,
		GT_TARGET   = 0x02,
		GT_LOOPBACK = 0x04,
	};

	enum ElementStates m_eState;
	long        m_nConnectionState;
	long        m_nMode;
	BOOL        m_bSync;
	BOOL        m_bSymmetric;
	long        m_nSymmetricGroupNumber;
	BOOL        m_bSymmetricNormallyStarted;
	long        m_nFailoverInitialState;
	long        m_nSymmetricConnectionState;
	long        m_nSymmetricMode;
	std::string m_strSymmetricJournalDirectory;
	std::string m_strSymmetricPStoreFile;
	BOOL        m_bChaining;
	std::string m_strStateTimeStamp;
	std::string m_strDescription;
	long        m_nGroupNumber;
	std::string m_strJournalDirectory;
	std::string m_strPStoreFile;
	long        m_nSyncDepth;
	long        m_nSyncTimeout;
	BOOL        m_bEnableCompression;
	BOOL        m_bRefreshNeverTimeout;
	long        m_nRefreshTimeoutInterval;
	long        m_nChunkDelay;
	long        m_nChunkSizeKB;
	long        m_nMaxFileStatSize;
	BOOL        m_bNetThreshold;
	long        m_nNetMaxKbps;
	long        m_nStatInterval;
	BOOL        m_bJournalLess;
	long        m_nPctDone;
    std::string m_strThrottles;

	CServer*    m_pParent;      // Source
	BOOL        m_bPrimaryDHCPAdressUsed;
 	BOOL        m_bPrimaryEditedIPUsed;
	std::string m_strPrimaryEditedIP;
     
	CServer*    m_pServerTarget;// Target  
	BOOL        m_bTargetDHCPAdressUsed;
 	BOOL        m_bTargetEditedIPUsed;
	std::string m_strTargetEditedIP;


	__int64     m_nReadKbps;
	__int64     m_nWriteKbps;
	__int64     m_nActualNet;
	__int64     m_nEffectiveNet;
	long        m_nBABEntries;
	long        m_nPctBAB;

	__int64     m_liPStoreSize;
	__int64     m_liJournalSize;
	__int64     m_liDiskTotalSize;
	__int64     m_liDiskFreeSize;

	//TDMF DB : index of this ReplGroup Source and Target Servers in the t_ServerInfo table.  
    //          Allows fast access to DB record and fast access to their corresponding CServer object 
    int         m_iDbSrcFk;
    int         m_iDbTgtFk;

	std::string m_strCmdHistory;
	std::string m_strLastCmdOutput;

	int			m_nType;

	enum ObjectState m_eObjectState;

	bool        m_bLockCmds;

	BOOL        m_bForcePMDRestart;

	CReplicationGroup() : m_eState(STATE_UNDEF), m_nMode(FTD_M_UNDEF), m_bSync(FALSE),
		m_bChaining(FALSE), m_bSymmetric(FALSE), m_nSymmetricGroupNumber(0),
		m_bSymmetricNormallyStarted(FALSE), m_nFailoverInitialState(0),
		m_nSymmetricConnectionState(FTD_M_UNDEF), m_nSymmetricMode(FTD_M_UNDEF), m_nGroupNumber(0),
		m_nSyncDepth(1), m_nSyncTimeout(30), m_bEnableCompression(FALSE), m_bRefreshNeverTimeout(FALSE),
		m_nConnectionState(FTD_UNDEF), m_nRefreshTimeoutInterval(0), m_pParent(NULL),
		m_pServerTarget(NULL), m_nChunkDelay(0), m_nChunkSizeKB(256), m_nMaxFileStatSize(64),
		m_bNetThreshold(FALSE), m_nNetMaxKbps(0), m_nStatInterval(10), m_bJournalLess(FALSE),
		m_nPctDone(0), m_nReadKbps(0), m_nWriteKbps(0), m_nActualNet(0), m_nEffectiveNet(0),
		m_nBABEntries(0), m_nPctBAB(0), m_iDbSrcFk(0), m_iDbTgtFk(0),
		m_liJournalSize(0), m_liPStoreSize(0), m_liDiskTotalSize(0), m_liDiskFreeSize(0),
		m_nType(GT_SOURCE), m_eObjectState(RPO_NEW),m_bLockCmds(false), m_bForcePMDRestart(false)
	{
		m_bPrimaryDHCPAdressUsed		= false; 
		m_bPrimaryEditedIPUsed			= false;
		m_strPrimaryEditedIP			= _T("");
		m_bTargetDHCPAdressUsed			= false;
		m_bTargetEditedIPUsed			= false;
		m_strTargetEditedIP				= _T("");
	}

	CReplicationGroup(const CReplicationGroup& Grp)
	{
		*this = Grp;
		m_pParent = Grp.m_pParent;
	}

	CReplicationGroup& operator=(const CReplicationGroup& Grp)
	{
		m_eState           = Grp.m_eState;
		m_nMode            = Grp.m_nMode;
		m_nConnectionState = Grp.m_nConnectionState;
		m_nPctDone         = Grp.m_nPctDone;
		m_nReadKbps        = Grp.m_nReadKbps;
		m_nWriteKbps       = Grp.m_nWriteKbps;
		m_nActualNet       = Grp.m_nActualNet;
		m_nEffectiveNet    = Grp.m_nEffectiveNet;
		m_nBABEntries      = Grp.m_nBABEntries;
		m_nPctBAB          = Grp.m_nPctBAB;
		m_liJournalSize    = Grp.m_liJournalSize;
		m_liPStoreSize     = Grp.m_liPStoreSize;
		m_liDiskTotalSize  = Grp.m_liDiskTotalSize;
		m_liDiskFreeSize   = Grp.m_liDiskFreeSize;
		m_strStateTimeStamp = Grp.m_strStateTimeStamp;
      
	    m_bPrimaryDHCPAdressUsed		= Grp.m_bPrimaryDHCPAdressUsed;
		m_bPrimaryEditedIPUsed			= Grp.m_bPrimaryEditedIPUsed;
		m_strPrimaryEditedIP			= Grp.m_strPrimaryEditedIP;
	
		m_bTargetDHCPAdressUsed			= Grp.m_bTargetDHCPAdressUsed;
		m_bTargetEditedIPUsed			= Grp.m_bTargetEditedIPUsed;
   		m_strTargetEditedIP				= Grp.m_strTargetEditedIP;

		m_nGroupNumber			= Grp.m_nGroupNumber;
		m_nType					= Grp.m_nType;
		m_bSymmetric                = Grp.m_bSymmetric;
		m_nSymmetricGroupNumber     = Grp.m_nSymmetricGroupNumber;
		m_bSymmetricNormallyStarted = Grp.m_bSymmetricNormallyStarted;
		m_nFailoverInitialState     = Grp.m_nFailoverInitialState;
		m_nSymmetricMode            = Grp.m_nSymmetricMode;
		m_nSymmetricConnectionState = Grp.m_nSymmetricConnectionState;
		m_strSymmetricJournalDirectory = Grp.m_strSymmetricJournalDirectory;
		m_strSymmetricPStoreFile    = Grp.m_strSymmetricPStoreFile;

		m_bChaining				= Grp.m_bChaining;

		m_nChunkDelay				= Grp.m_nChunkDelay;
		m_nChunkSizeKB				= Grp.m_nChunkSizeKB;
		m_bEnableCompression		= Grp.m_bEnableCompression;
		m_bSync						= Grp.m_bSync;
		m_nSyncDepth				= Grp.m_nSyncDepth;
		m_nSyncTimeout				= Grp.m_nSyncTimeout;
		m_bRefreshNeverTimeout    = Grp.m_bRefreshNeverTimeout;
		m_nRefreshTimeoutInterval = Grp.m_nRefreshTimeoutInterval;
		m_nMaxFileStatSize        = Grp.m_nMaxFileStatSize;
		m_bNetThreshold           = Grp.m_bNetThreshold;
		m_nNetMaxKbps             = Grp.m_nNetMaxKbps;
		m_nStatInterval           = Grp.m_nStatInterval;
		m_bJournalLess            = Grp.m_bJournalLess;

		m_strDescription          = Grp.m_strDescription;
		m_strJournalDirectory     = Grp.m_strJournalDirectory;
		m_strPStoreFile           = Grp.m_strPStoreFile;
		m_strCmdHistory           = Grp.m_strCmdHistory;
		m_strLastCmdOutput        = Grp.m_strLastCmdOutput;
        m_strThrottles			  = Grp.m_strThrottles;

		//m_pParent = Grp.m_pParent;
		m_pServerTarget = Grp.m_pServerTarget;

		m_iDbSrcFk = Grp.m_iDbSrcFk;
		m_iDbTgtFk = Grp.m_iDbTgtFk;
		m_eObjectState = Grp.m_eObjectState;

		m_bLockCmds = Grp.m_bLockCmds;

		m_bForcePMDRestart = Grp.m_bForcePMDRestart;

		m_listReplicationPair = Grp.m_listReplicationPair;
		std::list<CReplicationPair>::iterator it;
		for(it = m_listReplicationPair.begin(); it != m_listReplicationPair.end(); it++)
		{
			it->m_pParent = this;
		}

		return *this;
	}

	std::list<CReplicationPair> m_listReplicationPair;
	CReplicationPair& AddNewReplicationPair();

    /**
     * Initialize object from provided TDMF database record
     *
     * @param bRecurse : indicate if the list of replication pairs is to be filled or not.
     */
    bool Initialize(FsTdmfRecLgGrp *pRec, bool bRecurse);

    /**
     * Fill the list of replication pairs according to the TDMF DB
     * Object members must be already initialized (prior call to Initialize()).
     */
    bool InitializePairs();

    /**
     * This method performs verification on members values.
     * Its status indicates if the Replication Group is configured properly
     * to be sent to its TDMF Server/Agent.
     * @return : int , 0 = valid, otherwise a combinaison of REP_GRP_xyz values.
     */
    int IsValid();

	/**
     * Add/Update Replication Group information in the DB Group table.
	 * Also send the appropriate TDMF commands to the agent
     */
    int SaveToDB(long nOldGroupNumber, long nOldTgtHostId, std::string* pstrWarning, bool bUseTransaction = true);

	int RemoveFromDB();

	long LaunchCommand(enum tdmf_commands eCmd, char* pszOptions, char* pszLog, BOOL Symmetric, CString& cstrMsg);

    FsTdmfDb* GetDB();
	MMP_API*  GetMMP();

	// Events (Alerts)
	CEventList  m_EventList;
	//EventListIterator m_EventListIterator;
	CEvent* GetEventFirst();
	CEvent* GetEventAt     ( long nIndex ); // ardev 021129
	long    GetEventCount  ();              // ardev 021129
	BOOL    IsEventAt      ( long nIndex ); // ardev 021129

	std::string GetKey() const
	{
		std::ostringstream oss;
		oss << ((m_nType & GT_SOURCE) ? "P" : "S") << m_nGroupNumber;
		return oss.str();
	}

	bool operator==(const CReplicationGroup& RG) const
	{
		return (GetKey().compare(RG.GetKey()) == 0);
	}

	bool operator<(const CReplicationGroup& RG) const
	{
		return m_nGroupNumber < RG.m_nGroupNumber;
	}

	// Find and set a new valid group number
	long GetUniqueGroupNumber();

    void setTarget(const CServer *pServerTarget);
	bool SetNewTargetServer(char* pszDomainName, long nHostID);

	CReplicationGroup* GetTargetGroup();
	CReplicationGroup* CreateAssociatedTargetGroup();

	bool Copy(CReplicationGroup*);

	// Tunables
	int SaveTunables();
	int SetTunables();

	void LockCmds();
	void UnlockCmds();

	// Symmetric
	bool RemoveSymmetricGroup();
	long Failover(long& nWarning);
};


#endif //__REPLICATIONGROUP_H_
