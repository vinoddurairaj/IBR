/******************************************************************************************
FileName             : DataObjects.cpp
Author               : Mario Parent
Purpose		         : To create differents objects to hold info in the listviews
Date Of Creation     : 2002-04-12
Modification History :
Date:                  Modifications:
******************************************************************************************/

// CObjects.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CMessage Class
IMPLEMENT_DYNCREATE(CMessage, CObject)


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CMessage::CMessage
	Description :	Contain base class informations
	Return :			constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CMessage::CMessage()
{
	m_strMessage		= _T("");
}


CMessage::CMessage(CString strMessage)
	: m_strMessage(strMessage)
{

}


//////////////////////////////////////////////////////////////////////
// CObject Class
IMPLEMENT_DYNCREATE(CTDMFObject, CObject)


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFObject::CTDMFObject
	Description :	Contain base class informations
	Return :			constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFObject::CTDMFObject()
{
	m_nID				= 0;
	m_bNew			= TRUE;
	m_bModified		= FALSE;
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFObject::CTDMFObject
	Description :	Contain base class informations
	Return :			constructor	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			CString strName	-	a name to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFObject::CTDMFObject(int nID, BOOL bNew  /*= true*/)
	: m_nID(nID),m_bNew(bNew)
{

}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFObject::~CTDMFObject
	Description :	
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFObject::~CTDMFObject()
{

}

CString CTDMFObject::GetStringState(int nState)
{
	CString strValue;

	switch (nState)
	{	
		case StateInactive:
			strValue = _T("Inactive");
			break;
		case StateActive:
			strValue = _T("Inactive");
			break;
		case StateStarted:
			strValue = _T("Started");	
			break;
		case StateRefresh:
			strValue = _T("Refresh");
			break;
		case StateConnectionLost:
			strValue = _T("Connection lost");	
			break;
		case StateCheckpoint:
			strValue = _T("Checkpoint");	
			break;
		case StateBackfresh:
			strValue = _T("Backfresh");	
			break;
		case StateTracking:
			strValue = _T("Tracking");	
			break;
		case StatePassthru:
			strValue = _T("Passthru");	
			break;
		case StateConnect:
			strValue = _T("Connect");	
			break;
		case StateNormal:
			strValue = _T("Normal");	
			break;
		case StateUndefined:
			strValue = _T("Undefined");	
			break;
		default:
			strValue = _T("Undefined");	
			break;
	}

	return strValue;

}	

CString CTDMFObject::GetStringMode(int nMode)
{
	CString strValue;
	
	switch (nMode)
	{	
		case ModeNormal:
			strValue = _T("Inactive");
			break;
		case ModeTracking:
			strValue = _T("Inactive");
			break;
		case ModeUndefined:
			strValue = _T("Undefined");
			break;
		default:
			strValue = _T("Undefined");	
			break;
	}

	return strValue;

}	

CString CTDMFObject::GetStringAction(int nAction)
{
	CString strValue;
	
	switch (nAction)
	{	
		case ActionStart:
			strValue = _T("Start");
			break;
		case ActionStop:
			strValue = _T("Stop");
			break;
		default:
			strValue = _T("Stop");	
			break;
	}

	return strValue;

}	
//////////////////////////////////////////////////////////////////////
// CTDMFReplicationPair Class
IMPLEMENT_DYNCREATE(CTDMFReplicationPair, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFReplicationPair::CTDMFReplicationPair
	Description :	Contains replication pair informations
	Return :			constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFReplicationPair::CTDMFReplicationPair()
{

	m_nDtcID					= -1;		
	m_nLogicalGroupID		= -1;	
	m_nSourceServerID		= -1;
	m_strNotes				= _T("");

	m_strDeviceName		= _T("");
	m_strSourceDisk		= _T("");		
	m_strSourceDisk1		= _T("");
	m_strSourceDisk2		= _T("");
	m_strSourceDisk3		= _T("");

	m_strTargetDisk		= _T("");
	m_strTargetDisk1		= _T("");
	m_strTargetDisk2		= _T("");
	m_strTargetDisk3		= _T("");


}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFReplicationPair::CTDMFReplicationPair
	Description :	Contains replication pair informations
	Return :			constructor	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFReplicationPair::CTDMFReplicationPair(int nID,BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew)
{

	m_nDtcID					= -1;		
	m_nLogicalGroupID		= -1;	
	m_nSourceServerID		= -1;
	m_strNotes				= _T("");

	m_strDeviceName		= _T("");
	m_strSourceDisk		= _T("");		
	m_strSourceDisk1		= _T("");
	m_strSourceDisk2		= _T("");
	m_strSourceDisk3		= _T("");

	m_strTargetDisk		= _T("");
	m_strTargetDisk1		= _T("");
	m_strTargetDisk2		= _T("");
	m_strTargetDisk3		= _T("");
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFReplicationPair::~CTDMFReplicationPair
	Description :	
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFReplicationPair::~CTDMFReplicationPair()
{

}


//////////////////////////////////////////////////////////////////////
// CTDMFThrottles Class
IMPLEMENT_DYNCREATE(CTDMFThrottles, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFThrottles::CTDMFThrottles
	Description :	Contains replication pair informations
	Return :			constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFThrottles::CTDMFThrottles()
{
	m_strExpression	= _T("");
	m_bAlways			= true;
	m_strTimeRange		= _T("");
   m_strActionList	= _T("");
}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFThrottles::CTDMFThrottles
	Description :
	Return :			constructor	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFThrottles::CTDMFThrottles(int nID,BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew)
{

	m_strExpression	= _T("");
	m_bAlways			= true;
	m_strTimeRange		= _T("");
   m_strActionList	= _T("");

}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFThrottles::~CTDMFThrottles
	Description :	
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFThrottles::~CTDMFThrottles()
{

}


//////////////////////////////////////////////////////////////////////
// CTDMFVolume Class
IMPLEMENT_DYNCREATE(CTDMFVolume, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFVolume::CTDMFVolume
	Description :	Default constructor
	Return :			constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFVolume::CTDMFVolume()
{
	m_bAvailable	= false;
	m_nSize			=	0;
}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFVolume::CTDMFVolume	
	Description :	
	Return :			int (by default)	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			CString strName	-	a name to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/

CTDMFVolume::CTDMFVolume(int nID, CString strName, BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew),m_strName(strName)
{
	m_bAvailable	= false;
	m_nSize			=	0;
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFVolume::~CTDMFVolume
	Description :	
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFVolume::~CTDMFVolume()
{
  
}

//////////////////////////////////////////////////////////////////////
// CTDMFAllServers Class
IMPLEMENT_DYNCREATE(CTDMFAllServers, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFAllServers::CTDMFAllServers
	Description :	Contain all The servers object 
	Return :			Default constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFAllServers::CTDMFAllServers()
{
	m_arServers.RemoveAll();
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFAllServers::CTDMFAllServers
	Description :	Contain all Server informations
	Return :			constructor	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFAllServers::CTDMFAllServers(int nID, BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew)
{
	m_arServers.RemoveAll();
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFAllServers::~CTDMFAllServers
	Description :	Destroy all the objects Server Info 
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFAllServers::~CTDMFAllServers()
{
	//clean the Volumes array
  	int nSize =  m_arServers.GetSize();
	for (int i=0; i<nSize; i++)
		delete m_arServers[i];

	m_arServers.RemoveAll();

}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-16			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFAllServers::LoadAllServers
	Description :	Load all the objects server info
	Return :			BOOL	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
BOOL CTDMFAllServers::LoadAllServers()
{
	//call the api to refresh the database with server info


	//	for Collection 
	int nID;
	CString strName;

	CTDMFServer* pServerInfo = new CTDMFServer(nID, strName, FALSE);
	if (!pServerInfo)
		return FALSE;

	pServerInfo->m_strServerName			= _T("");			
	pServerInfo->m_strNote					= _T("");	
	pServerInfo->m_strServerIPAdress1	= _T("");	
	pServerInfo->m_strServerIPAdress2	= _T("");	
	pServerInfo->m_strServerIPAdress3	= _T("");	
	pServerInfo->m_strServerIPAdress4	= _T("");	
	pServerInfo->m_nIPPort					=	0;
	pServerInfo->m_strJournalVolume		= _T("");
	pServerInfo->m_BABSize					=	0;
	pServerInfo->m_TCPWinSize				=	0;
	pServerInfo->m_strOS_Type				= _T("");
	pServerInfo->m_strOS_Version			= _T("");
	pServerInfo->m_strFileSystem			= _T("");
	pServerInfo->m_strTDMFVersion			= _T("");
	pServerInfo->m_nNumberOfCPU			=	1;
	pServerInfo->m_nRamSize					=	0;
	pServerInfo->m_nState					= StateNormal;
	pServerInfo->m_bConfigured				= false;

	m_arServers.SetAtGrow(pServerInfo->m_nID, pServerInfo);
	return true;
}

//////////////////////////////////////////////////////////////////////
// CTDMFServer Class
IMPLEMENT_DYNCREATE(CTDMFServer, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFServer::CTDMFServer
	Description :	Contain all host informations
	Return :			Default constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFServer::CTDMFServer()
{

	m_strServerName			= _T("");			
	m_strNote					= _T("");	
	m_strServerIPAdress1		= _T("");	
	m_strServerIPAdress2		= _T("");	
	m_strServerIPAdress3		= _T("");	
	m_strServerIPAdress4		= _T("");	
	m_nIPPort					=	0;
	m_strJournalVolume		= _T("");
	m_BABSize					=	0;
	m_TCPWinSize				=	0;
	m_strOS_Type				= _T("");
	m_strOS_Version			= _T("");
	m_strFileSystem			= _T("");
	m_strTDMFVersion			= _T("");
	m_nNumberOfCPU				=	1;
	m_nRamSize					=	0;

	m_nState						= StateNormal;
	m_bConfigured				= false;

	m_arVolumes.RemoveAll();
	m_mapLogicalGroups.RemoveAll();

}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFServer::CTDMFServer
	Description :	Contain all Server informations
	Return :			constructor	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			CString strName	-	a name to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFServer::CTDMFServer(int nID, CString strName, BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew),m_strServerName(strName)
{
	m_strNote					= _T("");	
	m_strServerIPAdress1		= _T("");	
	m_strServerIPAdress2		= _T("");	
	m_strServerIPAdress3		= _T("");	
	m_strServerIPAdress4		= _T("");	
	m_nIPPort					=	0;
	m_strJournalVolume		= _T("");
	m_BABSize					=	0;
	m_TCPWinSize				=	0;
	m_strOS_Type				= _T("");
	m_strOS_Version			= _T("");
	m_strFileSystem			= _T("");
	m_strTDMFVersion			= _T("");
	m_nNumberOfCPU				=	1;
	m_nRamSize					=	0;

	m_nState						= StateNormal;
	m_bConfigured				= false;
	
	m_mapLogicalGroups.RemoveAll();
	m_arVolumes.RemoveAll();
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFServer::~CTDMFServer
	Description :	Destroy all the objects Volumes 
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFServer::~CTDMFServer()
{
	//clean the Volumes array
  	int nSize = m_arVolumes.GetSize();
	for (int i=0; i<nSize; i++)
		delete m_arVolumes[i];

	m_arVolumes.RemoveAll();

	//clean the Logical Groups array
	POSITION pos = m_mapLogicalGroups.GetStartPosition();
	while(pos != NULL)
	{
		 int nItem;
		 CTDMFLogicalGroup* pLogicalGroup;
		 
		 m_mapLogicalGroups.GetNextAssoc(pos, nItem, pLogicalGroup);
		 delete pLogicalGroup;
	}

	m_mapLogicalGroups.RemoveAll();

}

BOOL CTDMFServer::LoadAllVolumes()
{
  	//call the api to refresh the database with server info


	//	for Collection do a loop
	int nID;
	CString strName;

	CTDMFVolume* pVolume = new CTDMFVolume(nID, strName, FALSE);
	if (!pVolume)
		return FALSE;

	pVolume->m_nSize  		= 0;	
	pVolume->m_bAvailable	= false;

	m_arVolumes.Add(pVolume);

	return true;
}

BOOL CTDMFServer::LoadAllLogicalGroups()
{

	//call the api to refresh the database with server info

	m_mapLogicalGroups.RemoveAll();

	//	for Collection do a loop
	int nID;
	CString strName;

	CTDMFLogicalGroup* pLogicalGroup = new CTDMFLogicalGroup(nID, FALSE);
	if (!pLogicalGroup)
		return FALSE;

/*to finish to fill the data in the object
	
	pLogicalGroup->m_nSize  		= 0;	
	pLogicalGroup->m_bAvailable	= false;
*/
	m_mapLogicalGroups.SetAt(pLogicalGroup->m_nID,pLogicalGroup);

	return true;

}

int CTDMFServer::GetNumberOfActiveLogicalGroup()
{
	int nItem, nCount;

	nCount = 0;
	CTDMFLogicalGroup* pLogicalGroup;

	POSITION pos = m_mapLogicalGroups.GetStartPosition();
	while(pos != NULL)
	{
		 m_mapLogicalGroups.GetNextAssoc(pos, nItem, pLogicalGroup);
		 if(pLogicalGroup->m_nState == StateActive)
		 {
			nCount += 1;	 
		 }
	}

	return nCount;
}

//////////////////////////////////////////////////////////////////////
// CTDMFSystemConfiguration Class
IMPLEMENT_DYNCREATE(CTDMFSystemConfiguration, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFSystemConfiguration::CTDMFSystemConfiguration
	Description :	Contain all System Configuration
	Return :			Default constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFSystemConfiguration::CTDMFSystemConfiguration()
{

	m_mapLogicalGroups.RemoveAll();
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFSystemConfiguration::CTDMFSystemConfiguration
	Description :	Contain all Server informations
	Return :			constructor	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFSystemConfiguration::CTDMFSystemConfiguration(int nID, BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew)
{

	m_mapLogicalGroups.RemoveAll();
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFSystemConfiguration::~CTDMFSystemConfiguration
	Description :	Destroy all the objects Volumes 
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFSystemConfiguration::~CTDMFSystemConfiguration()
{
	//clean the Logical Groups array
	POSITION pos = m_mapLogicalGroups.GetStartPosition();
	while(pos != NULL)
	{
		 int nItem;
		 CTDMFLogicalGroup* pLogicalGroup;
		 
		 m_mapLogicalGroups.GetNextAssoc(pos, nItem, pLogicalGroup);
		 delete pLogicalGroup;
	}

	m_mapLogicalGroups.RemoveAll();
}

BOOL CTDMFSystemConfiguration::LoadAllLogicalGroups()
{

	//call the api to refresh the database with server info

	m_mapLogicalGroups.RemoveAll();

	int nID;
	CString strName;
	CTDMFLogicalGroup* pLogicalGroup;

	for (int i =0;i<3;i++)
	{

		pLogicalGroup = new CTDMFLogicalGroup(nID, FALSE);
		if (!pLogicalGroup)
			continue;

	
		pLogicalGroup->m_nID		= i+1;	
		pLogicalGroup->m_nState = StateActive;

		m_mapLogicalGroups.SetAt(pLogicalGroup->m_nID,pLogicalGroup);
	}
	return true;

}

//////////////////////////////////////////////////////////////////////
// CTDMFLogicalGroup Class
IMPLEMENT_DYNCREATE(CTDMFLogicalGroup, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFLogicalGroup::CTDMFLogicalGroup
	Description :	Contains information on Logical Group
	Return :			constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFLogicalGroup::CTDMFLogicalGroup()
{

	m_nSourceServerID			= -1;			
	m_nTargetServerID			= -1;	
	m_strSourceServerName	= _T("");
	m_strTargetServerName	= _T("");

	m_strNote					= _T("");

//details
	m_strGroupName				= _T("");				
	m_nLocalReads				= 0;			
	m_nLocalWrites				= 0;	
	m_dPourcentBABUsed		= 0;
	m_nBABEntries				= 0;
	m_strCompressionRatio	= _T("");
	m_nJournalMegaByte		= 0;
	m_nState						=	StateInactive;
	m_nMode						= ModeNormal;
	m_nAction					= ActionStart;


	//Tunables parameters	

	m_bChaining						= false;
	m_nChunkDelay					= 0;
	m_nChunkSize					= 1024;
	m_bSyncMode						= false; // default asynchronous
	m_nSyncModeDepth				= 1;		// 1 entry
	m_nSyncModeTimeOut			= 30;		// 30 seconds
	m_bNetworkUsageThreshold	= false;
	m_nUpdateInterval				= 10;		//10 seconds
	m_nMaxStatFileSize			= 64;		// 64 KB
	m_nNextMaxKBPS					= -1;		//-1 indicates off
	m_bTraceThrottle				= false; // false = off
	m_bCompression					= false; // false = off

	m_mapReplicationPairs.RemoveAll();
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFLogicalGroup::CTDMFLogicalGroup
	Description :	
	Return :			constructor	-	
	Parameters :
		int nID				- 	a ID number to give to the object
		BOOL bNew			-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFLogicalGroup::CTDMFLogicalGroup(int nID, BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew)
{

	m_nSourceServerID			= -1;			
	m_nTargetServerID			= -1;	
	m_strSourceServerName	= _T("");
	m_strTargetServerName	= _T("");
	m_strNote					= _T("");

//details
	m_strGroupName				= _T("");				
	m_nLocalReads				= 0;			
	m_nLocalWrites				= 0;	
	m_dPourcentBABUsed		= 0;
	m_nBABEntries				= 0;
	m_strCompressionRatio	= _T("");
	m_nJournalMegaByte		= 0;
	m_nState						=	StateInactive;
	m_nMode						= ModeNormal;
	m_nAction					= ActionStart;

	//Tunables parameters	

	m_bChaining						= false;
	m_nChunkDelay					= 0;
	m_nChunkSize					= 1024;
	m_bSyncMode						= false; // default asynchronous
	m_nSyncModeDepth				= 1;		// 1 entry
	m_nSyncModeTimeOut			= 30;		// 30 seconds
	m_bNetworkUsageThreshold	= false;
	m_nUpdateInterval				= 10;		//10 seconds
	m_nMaxStatFileSize			= 64;		// 64 KB
	m_nNextMaxKBPS					= -1;		//-1 indicates off
	m_bTraceThrottle				= false; // false = off
	m_bCompression					= false; // false = off


	m_mapReplicationPairs.RemoveAll();
}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFLogicalGroup::~CTDMFLogicalGroup
	Description :	Destructor of the logical group
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFLogicalGroup::~CTDMFLogicalGroup()
{
  	POSITION pos = m_mapReplicationPairs.GetStartPosition();
	while(pos != NULL)
	{
		 int nItem;
		 CTDMFReplicationPair* pReplicationPair;
		 
		 m_mapReplicationPairs.GetNextAssoc(pos, nItem, pReplicationPair);
		 delete pReplicationPair;
	}

	m_mapReplicationPairs.RemoveAll();

}

BOOL CTDMFLogicalGroup::LoadAllReplicationPair()
{
	//call the api to refresh the database with server info

	m_mapReplicationPairs.RemoveAll();

	//	for Collection do a loop
	int nID;
	CString strName;

	CTDMFReplicationPair* pReplicationPair = new CTDMFReplicationPair(nID, FALSE);
	if (!pReplicationPair)
		return FALSE;

/*to finish to fill the data in the object
	
	pReplicationPair-> 		= 0;	
	pReplicationPair->	= false;
*/
	m_mapReplicationPairs.SetAt(pReplicationPair->m_nID,pReplicationPair);

	return true;

}



//////////////////////////////////////////////////////////////////////
// CTDMFAgentInfo Class
IMPLEMENT_DYNCREATE(CTDMFAgentInfo, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFAgentInfo::CTDMFAgentInfo
	Description :	
	Return :			constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFAgentInfo::CTDMFAgentInfo()
{
	m_strMachine			= _T("");			
	m_strNumberOfValidIP	= _T("");	
	m_strIPAgent			= _T("");			
	m_strPortNumber		= -1;			
	m_strIPRouter			= _T("");			 
	m_strTDMFDomain		= _T("");		
}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFAgentInfo::CTDMFAgentInfo
	Description :	Contain all Server informations
	Return :			constructor	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			CString strName	-	a name to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFAgentInfo::CTDMFAgentInfo(int nID, CString strMachine, BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew),m_strMachine(strMachine)
{

	m_strNumberOfValidIP	= _T("");	
	m_strIPAgent			= _T("");			
	m_strPortNumber		= -1;			
	m_strIPRouter			= _T("");			 
	m_strTDMFDomain		= _T("");		
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFAgentInfo::~CTDMFAgentInfo
	Description :	
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFAgentInfo::~CTDMFAgentInfo()
{

}

//////////////////////////////////////////////////////////////////////
// CTDMFRegistrationKey Class
IMPLEMENT_DYNCREATE(CTDMFRegistrationKey, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFRegistrationKey::CTDMFRegistrationKey
	Description :	
	Return :			constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFRegistrationKey::CTDMFRegistrationKey()
{
	m_strMachine			= _T("");			
	m_strRegistrationKey	= _T("");	

}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFRegistrationKey::CTDMFRegistrationKey
	Description :	Contain all Server informations
	Return :			constructor	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			CString strName	-	a name to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFRegistrationKey::CTDMFRegistrationKey(int nID, CString strMachine,CString strRegistrationKey, BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew),m_strMachine(strMachine),m_strRegistrationKey(strRegistrationKey)
{

}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFRegistrationKey::~CTDMFRegistrationKey
	Description :	
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFRegistrationKey::~CTDMFRegistrationKey()
{

}


//////////////////////////////////////////////////////////////////////
// CTDMFMonitorInfo Class
IMPLEMENT_DYNCREATE(CTDMFMonitorInfo, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFMonitorInfo::CTDMFMonitorInfo
	Description :	
	Return :			constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFMonitorInfo::CTDMFMonitorInfo()
{
	m_strServer				= _T("");			
	m_strLogicalGroup		= _T("");	
	m_strConnection		= _T("");		
	m_strReplicationPair	= _T("");		
	m_strMode				= _T("");		
	m_dLocalRead			= 0; //in Kbs		
	m_dLocalWrite			= 0; //in Kbs		
	m_dNetActual			= 0; //in Kbs		
	m_dNetEffect			= 0; //in Kbs		
	m_nEntries				= 0;
	m_dPercentBABUsed		= 0;	
}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFMonitorInfo::CTDMFMonitorInfo
	Description :	Contain all Server informations
	Return :			constructor	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			CString strName	-	a name to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFMonitorInfo::CTDMFMonitorInfo(int nID, BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew)
{

	m_strServer				= _T("");			
	m_strLogicalGroup		= _T("");	
	m_strConnection		= _T("");		
	m_strReplicationPair	= _T("");		
	m_strMode				= _T("");		
	m_dLocalRead			= 0; //in Kbs		
	m_dLocalWrite			= 0; //in Kbs		
	m_dNetActual			= 0; //in Kbs		
	m_dNetEffect			= 0; //in Kbs		
	m_nEntries				= 0;
	m_dPercentBABUsed		= 0;	
}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFMonitorInfo::~CTDMFMonitorInfo
	Description :	
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFMonitorInfo::~CTDMFMonitorInfo()
{

}

//////////////////////////////////////////////////////////////////////
// CTDMFEventInfo Class
IMPLEMENT_DYNCREATE(CTDMFEventInfo, CTDMFObject )


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFEventInfo::CTDMFEventInfo
	Description :	
	Return :			constructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFEventInfo::CTDMFEventInfo()
{
	m_dtEventTime.GetCurrentTime();	
	m_strMessageType	= _T("");	
	m_strMessage		= _T("");
	m_nSeverity			= 1;

}

/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFEventInfo::CTDMFEventInfo
	Description :	Contain all Server informations
	Return :			constructor	-	
	Parameters :
			int nID	- 		a ID number to give to the object
			CString strName	-	a name to give to the object
			BOOL bNew	-	true if new
	Note :
\*---------------------------------------------------------------------------*/
CTDMFEventInfo::CTDMFEventInfo(int nID, BOOL bNew /*= true*/)
	: CTDMFObject(nID, bNew)
{

	m_dtEventTime.GetCurrentTime();	
	m_strMessageType	= _T("");	
	m_strMessage		= _T("");
	m_nSeverity			= 1;

}


/*---------------------------------------------------------------------------*\
	Author :Mario Parent			Date : 2002-04-12			version 1.0
  ---------------------------------------------------------------------------
	Function :		CTDMFEventInfo::~CTDMFEventInfo
	Description :	
	Return :			destructor	-	
	Parameters :
	Note :
\*---------------------------------------------------------------------------*/
CTDMFEventInfo::~CTDMFEventInfo()
{

}





