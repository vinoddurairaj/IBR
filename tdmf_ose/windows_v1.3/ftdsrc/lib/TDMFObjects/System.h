// System.h : Declaration of the CSystem

#ifndef __SYSTEM_H_
#define __SYSTEM_H_

#include "Domain.h"
#include "Event.h"
#include "../libmngt/libmngtnep.h"
#include "../libmngt/libmngtmsg.h"
#include "FsTdmfDb.h"


class FsTdmfDb;
class MMP_API;

//#define _TESTER_WND
#ifdef _TESTER_WND
#pragma message ( "WARNING: Test Wnd is enabled")
#pragma warning ( disable : 4355 )
#include "SystemTestWnd.h"
#endif


/////////////////////////////////////////////////////////////////////////////
//

#define WM_REPLICATION_GROUP_STATE_CHANGE (WM_USER)     // wParam = DomainName, lParam = HostId + RepGroupNumber
#define WM_SERVER_STATE_CHANGE			  (WM_USER + 1) // wParam = DomainName, lParam = HostId
#define WM_SERVER_CONNECTION_CHANGE		  (WM_USER + 3) // wParam = DomainName, lParam = HostId
#define WM_REPLICATION_GROUP_PERF_CHANGE  (WM_USER + 4) // wParam = DomainName, lParam = HostId + RepGroupNumber
#define WM_SERVER_PERF_CHANGE			  (WM_USER + 5) // wParam = DomainName, lParam = HostId
#define WM_COLLECTOR_COMMUNICATION_STATUS (WM_USER + 6) // wParam = Notif Msg
#define WM_DOMAIN_ADD			          (WM_USER + 7)
#define WM_SERVER_ADD			          (WM_USER + 8)
#define WM_DEBUG_TRACE                    (WM_USER + 9)
#define WM_REPLICATION_GROUP_ADD		  (WM_USER + 10)
#define WM_DOMAIN_REMOVE		          (WM_USER + 11)
#define WM_DOMAIN_MODIFY		          (WM_USER + 12)
#define WM_SERVER_REMOVE		          (WM_USER + 13)
#define WM_SERVER_MODIFY		          (WM_USER + 14)
#define WM_REPLICATION_GROUP_REMOVE		  (WM_USER + 15)
#define WM_REPLICATION_GROUP_MODIFY		  (WM_USER + 16)
#define WM_TEXT_MESSAGE		              (WM_USER + 17)
#define WM_RECEIVED_STATISTICS_DATA       (WM_USER + 18)
#define WM_IPADRESS_UNKNOWN			      (WM_USER + 19)
#define WM_SERVER_BAB_NOT_OPTIMAL	      (WM_USER + 20)

//////////////////////////////////////////////////////////////////////////////
// Logging levels.

#define ONLOG_NOLOGGING         0
#define ONLOG_BASIC             1
#define ONLOG_EXTENDED          2
#define ONLOG_DEBUG             3
#define ONLOG_MORE_DEBUG        4


//////////////////////////////////////////////////////////////////////
//

class CConditionElement
{
private:

public:
	virtual ~CConditionElement() {}
	virtual BOOL Evaluate(long nMode, long nConnectionStatus, long nPlatform) = 0;
	virtual CConditionElement* Copy() = 0;
};


class CConditionElementOr : public CConditionElement
{
private:
	CConditionElement* m_pConditionElementLeft;
	CConditionElement* m_pConditionElementRight;

public:
	CConditionElementOr(CConditionElement* pLeft, CConditionElement* pRight)
		: m_pConditionElementLeft(pLeft), m_pConditionElementRight(pRight) {}

	~CConditionElementOr() {delete m_pConditionElementLeft; delete m_pConditionElementRight;}

	BOOL Evaluate(long nMode, long nConnectionStatus, long nPlatform)
	{
		return (m_pConditionElementLeft->Evaluate(nMode, nConnectionStatus, nPlatform) ||
				m_pConditionElementRight->Evaluate(nMode, nConnectionStatus, nPlatform));
	}

	CConditionElement* Copy()
	{
		CConditionElementOr* pRet = new CConditionElementOr(NULL, NULL);
		pRet->m_pConditionElementLeft  = m_pConditionElementLeft->Copy();
		pRet->m_pConditionElementRight = m_pConditionElementRight->Copy();

		return pRet;
	}
};


class CConditionElementAnd : public CConditionElement
{
private:
	CConditionElement* m_pConditionElementLeft;
	CConditionElement* m_pConditionElementRight;

public:
	CConditionElementAnd(CConditionElement* pLeft, CConditionElement* pRight)
		: m_pConditionElementLeft(pLeft), m_pConditionElementRight(pRight) {}

	~CConditionElementAnd() {delete m_pConditionElementLeft; delete m_pConditionElementRight;}

	BOOL Evaluate(long nMode, long nConnectionStatus, long nPlatform)
	{
		return (m_pConditionElementLeft->Evaluate(nMode, nConnectionStatus, nPlatform) &&
				m_pConditionElementRight->Evaluate(nMode, nConnectionStatus, nPlatform));
	}

	CConditionElement* Copy()
	{
		CConditionElementAnd* pRet = new CConditionElementAnd(NULL, NULL);
		pRet->m_pConditionElementLeft  = m_pConditionElementLeft->Copy();
		pRet->m_pConditionElementRight = m_pConditionElementRight->Copy();

		return pRet;
	}
};


class CConditionElementEvaluation : public CConditionElement
{
public:
	enum Variable
	{
		VariableMode = 0,
		VariableConnectionStatus,
		VariablePlatform
	};

private:

	enum Variable m_eVar;
	long          m_nValue;

public:

	CConditionElementEvaluation(enum Variable eVar, long nValue) : m_eVar(eVar), m_nValue(nValue) {}

	BOOL Evaluate(long nMode, long nConnectionStatus, long nPlatform)
	{
		switch (m_eVar)
		{
		case VariableMode:
			return (nMode == m_nValue);
			break;

		case VariableConnectionStatus:
			return (nConnectionStatus == m_nValue);
			break;

		case VariablePlatform:
			return (nPlatform == m_nValue);
			break;
		}

		return FALSE;
	}

	CConditionElement* Copy()
	{
		CConditionElementEvaluation* pRet = new CConditionElementEvaluation(m_eVar, m_nValue);

		return pRet;
	}
};


/////////////////////////////////////////////////////////////////////////////
// 

class CCommandMenuEntry
{
public:
	std::string         m_strEntry;
	int                 m_nCmd;
	CConditionElement*  m_pConditionElement;

	CCommandMenuEntry() : m_nCmd(0), m_pConditionElement(NULL) {}
	~CCommandMenuEntry() {if (m_pConditionElement) delete m_pConditionElement;}

	CCommandMenuEntry(const CCommandMenuEntry& CmdEntry) : m_pConditionElement(NULL)
	{
		m_strEntry = CmdEntry.m_strEntry;
		m_nCmd     = CmdEntry.m_nCmd;
		if (CmdEntry.m_pConditionElement != NULL)
		{
			m_pConditionElement = CmdEntry.m_pConditionElement->Copy();
		}
	}
};


/////////////////////////////////////////////////////////////////////////////
// CSystem
class CSystem
{
protected:
    FsTdmfDb*   m_db;
    MMP_API*    m_mmp;  //interface to TDMF advanced functionalities
    CUIntArray  m_TimeOutArray;

	CEventList  m_EventList;
	//EventListIterator m_EventListItNext;
	//EventListIterator m_EventListItPrev;

	bool           m_bCheckAccess;

public:
	unsigned short m_nTraceLevel;

	CComBSTR       m_bstrUser;
	CComBSTR       m_bstrPwd;
    mmp_TdmfCollectorState m_CollectorStatsMsg;

	UINT           m_nNbLockCmds;

#ifdef _TESTER_WND
	CSystemTestWnd m_SystemTestWnd;
#endif

public:

    
	long GetTimeOutFromDB();
    long SetTimeOutToDB();
    int  GetTimeOut(int nIndex);
    void SetTimeOut(int nIndex, int nValue);

	long GetUserCount();
	void InitializeTheCollectorStatsMember();

	bool SetDeleteRecords(short Table, long days, long NbRecords);
	void GetDeleteRecords(short Table, long* pDays, long* pNbRecords);
	long GetDeleteDelay();
	bool SetDeleteDelay(long nNewValue);

    bool AlreadyExistDomain(char* pszDomainName , long lKey);
	std::string m_strName;

	std::string m_strDatabaseServer;
	std::string m_strCollectorVersion;
	std::string m_strCollectorIP;
	std::string m_strCollectorPort;
	std::string m_strCollectorHostId;

	HWND        m_hWnd;     // The window that will receive the notifications
	std::map<long, std::string> m_mapServerName;

	CSystem();
    ~CSystem();

	void Trace(unsigned short thislevel, const char* message, ...);
	void TraceOut(unsigned short thislevel, const char* message, va_list marker);

	bool GetFirstKeyLogMsg(std::string& strDate, std::string& strHostname, long* pnHostId, std::string& strRegKey, std::string& strExpDate);
	bool GetNextKeyLogMsg(std::string& strDate, std::string& strHostname, long* pnHostId, std::string& strRegKey, std::string& strExpDate);
	BOOL LogUsersActions();
	void LogUsersActions(BOOL bLog);
	bool GetFirstLogMsg(std::string& strDate, std::string& strSource, std::string& strUser, std::string& strMsg);
	bool GetNextLogMsg(std::string& strDate, std::string& strSource, std::string& strUser, std::string& strMsg);
	bool DeleteAllLogMsg();
	void LogUserAction(LPCSTR lpcstrMsg);

	std::list<CDomain> m_listDomain;
	CDomain& AddNewDomain();

	// Multi-console
	bool AddDomain(int iDbKa);
	bool RemoveDomain(int iDbKa);
	bool ModifyDomain(int iDbKa);
	bool AddServerToDomain(int iDbKa, int iSrvId);
	bool MoveServerToDomain(int iDbKa, int iSrvId, int iDbKaNew);
	bool RemoveServerFromDomain(int iDbKa, int iSrvId);
	bool ModifyServerInDomain(int iDbKa, int iSrvId);
	bool AddGroupToServerInDomain(int iDbKa, int iSrvId, int iGrpId);
	bool RemoveGroupFromServerInDomain(int iDbKa, int iSrvId, int iGrpId);
	bool ModifyGroupInServerInDomain(int iDbKa, int iSrvId, int iGrpId);
	bool FowardTextMessage(UINT nID, BYTE* pbMsg);

    FsTdmfDb* GetDB() {return m_db;}
	MMP_API*  GetMMP() {return m_mmp;}
    mmp_TdmfCollectorState*  GetTdmfCollectorState() {return &m_CollectorStatsMsg;}
    void SetTdmfCollectorState( mmp_TdmfCollectorState* pTdmfCollectorState);
    
    /**
     * Initialize object from provided TDMF database record
     */
	int  Initialize(HWND hWnd);
	void Uninitialize();
	long Open(char* pszDSN);
	void InitializeModeAndStatus();
	long RequestOwnership(BOOL bRequest);

	bool UpdateModeAndStatus(time_t, long, long, long, long, long, __int64, __int64, __int64, __int64, __int64, __int64);
	bool UpdateServerConnectionState(long nHostID, BOOL bConnected, int nBABSizeAllocated, int nBABSizeRequested);
	bool UpdatePerformanceData(time_t, long, long, __int64, __int64, __int64, __int64, int, int, int, int);
	bool UpdateObjects(BYTE* pMsg);
	void ResetModeAndStatus(long nHostID);

	static void OnNotificationMsg(void* pThis, TDMFNotificationMessage* pNotificationMessage);

	// Events (Alerts)
	CEvent*     GetEventFirst ();
	CEvent*     GetEventAt    ( long nIndex );
	long        GetEventCount ();
	BOOL        IsEventAt     ( long nIndex );
	std::string GetServerName ( long nIndex );
	/**
	 * Get the table sizes from the DB
	 */
	std::string GetTableSize  ( short Table, long* pCount );
	bool DeleteTableRecords   ( short Table );

	// Commands Menus Definition
	typedef std::map< std::string, std::vector<CCommandMenuEntry> > STRING2VEC;


	STRING2VEC  m_mapCommandMenuEntries;

	void        LoadCommandMenuEntries();
	int         GetNbCommandMenuEntries(char* szKey);
	std::string GetCommandMenuEntryName(char* szKey, int nIndex);
	int         GetCommandMenuEntryId(char* szKey, int nIndex);
	BOOL        IsCommandMenuEntryEnabled(char* szKey, int nIndex, int nMode, int nConnectionStatus, int nPlatform);
	std::string GetCommandMenuString(char* szKey, int Id);

	// User role
	FsTdmfDbUserInfo::TdmfRoles  m_eTdmfRole;
	std::list<FsTdmfDbUserInfo> m_listUser;
	std::list<FsTdmfDbUserInfo>::iterator m_itUser;
	std::string          GetUserRole();
	bool                 GetFirstUser(std::string& strLocation, std::string& strUserName, std::string& strType, std::string& strApp);
	bool                 GetNextUser(std::string& strLocation, std::string& strUserName, std::string& strType, std::string& strApp);
};


#endif //__SYSTEM_H_
