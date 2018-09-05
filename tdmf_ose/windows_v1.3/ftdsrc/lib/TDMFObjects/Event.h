// Event.h : Declaration of the CEvent

#ifndef __EVENT_H_
#define __EVENT_H_

#include "resource.h"       // main symbols

class FsTdmfRecAlert;

/////////////////////////////////////////////////////////////////////////////
// CEvent

class CEvent
{
public:
	long        m_nIk;
	std::string m_strDate;
	std::string m_strTime;
	std::string m_strSource;
	std::string m_strServer;
	long        m_nGroupID;
	long        m_nPairID;
	std::string m_strType;
	long        m_nSeverity;
	std::string m_strDescription;

public:
	CEvent() : m_nIk(0), m_nGroupID(0), m_nPairID(0), m_nSeverity(0) {}

	/**
     * Initialize object from provided TDMF database record
     */
	bool Initialize
		(
			FsTdmfRecAlert* pRec,
			std::string     pStrSrvName = ""
		);

}; // class CEvent


/////////////////////////////////////////////////////////////////////////////
// CEventList

// The following is the maximum number 
// of event row that can be fetched
// consecutively.
#define EVENT_PAGE_SIZE 600

//typedef std::list<CEvent>::iterator EventListIterator;

class CEventList
{
public:
	std::map<long, CEvent>  m_mapEvent;

public:
	CEventList() {}

	/**
     * Reset the list of object (CEvent)
     */
	void Reset();

	/**
     * Read the unread events from provided TDMF database record
	 */
	CEvent* ReadRangeFromDb     
			( 
				class CSystem* pSystem,
				long           nIndex,
				long           nServerId   = -1,
				long           nGroupId    = -1
			);                           // ardev 021127

	CEvent* GetAt                ( long nIndex );

}; // class CEventList


#endif //__EVENT_H_
