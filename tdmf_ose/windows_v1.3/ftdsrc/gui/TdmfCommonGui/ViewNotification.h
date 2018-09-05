// ViewNotification.h: interface for the CViewNotification class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIEWNOTIFICATION_H__90208170_B9C3_4293_BBA9_21915F5F0069__INCLUDED_)
#define AFX_VIEWNOTIFICATION_H__90208170_B9C3_4293_BBA9_21915F5F0069__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



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
#define WM_EVENT_SELECTION_CHANGE         (WM_USER + 21)


///////////////////////////////////////////////////////////////////////////////
//

class CViewNotification : public CObject  
{
public:

	typedef enum {
		VIEW_NOTIFICATION = 1000,
	} TDMF_NOTIFICATION;

	typedef enum {
		EMPTY = 0,

		SELECTION_CHANGE_SYSTEM = 1,
		SELECTION_CHANGE_DOMAIN,
		SELECTION_CHANGE_SERVER,
		SELECTION_CHANGE_RG,
		SELECTION_CHANGE_RP,

		REFRESH_SYSTEM,

		SYSTEM_CHANGE,

		DOMAIN_ADD,
		DOMAIN_REMOVE,
		DOMAIN_CHANGE,

		SERVER_ADD,
		SERVER_REMOVE,
		SERVER_CHANGE,
		SERVER_BAB_NOT_OPTIMAL,

		REPLICATION_GROUP_ADD,
		REPLICATION_GROUP_REMOVE,
		REPLICATION_GROUP_CHANGE,

		TAB_SYSTEM_PREVIOUS,
		TAB_SYSTEM_NEXT,
		TAB_SERVER_PREVIOUS,
		TAB_SERVER_NEXT,
		TAB_REPLICATION_GROUP_PREVIOUS,
		TAB_REPLICATION_GROUP_NEXT,
		TAB_TOOLS_PREVIOUS,
		TAB_TOOLS_NEXT,

        RECEIVED_COLLECTOR_STATISTICS_DATA,
        SHOW_DETAILS_COLLECTOR_STATS_MSG,

		EVENT_SELECTION_CHANGE_UP,
		EVENT_SELECTION_CHANGE_DOWN,

	} MESSAGES;

	typedef enum {
		NONE        = 0,  // No change
		PROPERTIES  = 1,	 // Some properties have changed (we don't known witch one), param2 = NULL
		NAME        = 2,  // Name has changed, param2 = Old Name
		STATE       = 4,  // Element's state has changed or selection state has changed
		CONNECTION  = 8,  // Element's connection state has changed
		PERFORMANCE = 16, // Element's performance data has changed
		ITEM        = 32, // A new item have been selected
	} CHANGE_PARAMS;

	MESSAGES      m_nMessageId;
	IUnknown*     m_pUnk;
	CHANGE_PARAMS m_eParam;  // May contains a CHANGE_PARAMS
	DWORD         m_dwParam1; // Contains a value associated to eParam
	DWORD         m_dwParam2; // Contains another value associated to eParam
	DWORD         m_dwParam3; // Contains another value associated to eParam

public:
	CViewNotification();
	virtual ~CViewNotification();
};


#endif // !defined(AFX_VIEWNOTIFICATION_H__90208170_B9C3_4293_BBA9_21915F5F0069__INCLUDED_)
