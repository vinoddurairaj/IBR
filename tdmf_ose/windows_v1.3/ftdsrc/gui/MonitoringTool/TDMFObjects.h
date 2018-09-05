/******************************************************************************************
FileName             : DataObjects.h
Author               : Mario Parent
Purpose		         : Classes to hold differents informations
Date Of Creation     : 2002-04-12
Modification History :
Date:                  Modifications:
******************************************************************************************/

// DataObjects.h
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TDMFOBJECTS_H__46E16461_12EA_11D3_AAB2_14BE04C10E27__INCLUDED_)
#define AFX_TDMFOBJECTS_H__46E16461_12EA_11D3_AAB2_14BE04C10E27__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
 
#include <afxtempl.h>
#include <afxcoll.h>

#define	SEC_PER_MIN				60
#define	SEC_PER_HOUR			60*SEC_PER_MIN
#define	SEC_PER_DAY				24*SEC_PER_HOUR
#define	MIN_PER_HOUR			60
#define	MIN_PER_DAY				24*MIN_PER_HOUR
#define	INTERVAL_SUBJECT_1	"Interval*(Subjects-1)"


class CTDMFObject;
class CTDMFLogicalGroup;
class	CTDMFTDMFAgentInfo;
class CTDMFReplicationPair;
class CTDMFVolume;
class CTDMFServer;
class CTDMFThrottles;

//////////////////////////////////////////////////////////////////////
// CTDMFObject Class

class CMessage : public CObject  
{
	public:
	DECLARE_DYNCREATE(CMessage)
	CMessage();
	CMessage(CString strMessage);

		
//attribut		
	CString m_strMessage;
	
};

class CTDMFObject : public CObject  
{
public:
	int			m_nID;
	BOOL			m_bNew;
	BOOL			m_bModified;

	CTDMFObject();
	DECLARE_DYNCREATE(CTDMFObject)

	CTDMFObject(int nID,BOOL bNew = true);
	virtual ~CTDMFObject();

	
	CString GetStringState(int nState);
	CString GetStringMode(int nMode);
	CString GetStringAction(int nAction);

};

//////////////////////////////////////////////////////////////////////
// CTDMFDomaine Class

class CTDMFDomaine : public CTDMFObject  
{
public:

	CTDMFDomaine();
	DECLARE_DYNCREATE(CTDMFDomaine)
	CTDMFDomaine(int nID, CString strName, BOOL bNew = true);
	virtual ~CTDMFDomaine();


//operation
	BOOL LoadAllServers();


// Attributes

	CString		m_strDomaineName;			//On primary Machine
	CTypedPtrArray<CObArray, CTDMFServer*>		m_arServers;

  
};

//////////////////////////////////////////////////////////////////////
// CTDMFServer Class

class CTDMFServer : public CTDMFObject  
{
public:

	CTDMFServer();
	DECLARE_DYNCREATE(CTDMFServer)
	CTDMFServer(int nID, CString strName, BOOL bNew = true);
	virtual ~CTDMFServer();

	BOOL LoadAllVolumes();
	BOOL LoadAllLogicalGroups();
   int GetNumberOfActiveLogicalGroup();

// Attributes

	CString		m_strServerName;			//On primary Machine
	CString		m_strNote;	
	CString		m_strServerIPAdress1;	//On primary Machine
	CString		m_strServerIPAdress2;	//On primary Machine
	CString		m_strServerIPAdress3;	//On primary Machine
	CString		m_strServerIPAdress4;	//On primary Machine
	int			m_nIPPort;
	CString		m_strJournalVolume;
	int			m_BABSize;
	int			m_TCPWinSize;
	CString		m_strOS_Type;
	CString		m_strOS_Version;
	CString		m_strFileSystem;
	CString		m_strTDMFVersion;
	int			m_nNumberOfCPU;
	int			m_nRamSize;

	int			m_nState;
	bool			m_bConfigured;

	CString GetStringState(int nState);

	CMap<int, int, CTDMFLogicalGroup *, CTDMFLogicalGroup *> m_mapLogicalGroups;	
	CTypedPtrArray<CObArray, CTDMFVolume*>		m_arVolumes;
  
};

//////////////////////////////////////////////////////////////////////
// CTDMFLogicalGroup Class
// Its a logical group 
class CTDMFLogicalGroup : public CTDMFObject  
{
public:

	BOOL LoadAllReplicationPair();


	CTDMFLogicalGroup();
	DECLARE_DYNCREATE(CTDMFLogicalGroup)
	CTDMFLogicalGroup(int nID, BOOL bNew = true);
	virtual ~CTDMFLogicalGroup();

// Attributes
 	int			m_nSourceServerID;	
	CString		m_strSourceServerName;
	int			m_nTargetServerID;	
	CString		m_strTargetServerName;
	CString		m_strNote;

//details
	CString		m_strGroupName;				
	int			m_nLocalReads;			
	int			m_nLocalWrites;	
	double		m_dPourcentBABUsed;
	int			m_nBABEntries;
	CString		m_strCompressionRatio;
   int			m_nJournalMegaByte;
   int			m_nState;
	int			m_nMode;
	int			m_nAction;

	//Tunables parameters	

	bool			m_bChaining;
	int			m_nChunkDelay;
	int			m_nChunkSize;
	bool			m_bSyncMode;
	int			m_nSyncModeDepth;	
	int			m_nSyncModeTimeOut;
	bool			m_bNetworkUsageThreshold;
	double		m_dNetworkUtilisation;
	int			m_nUpdateInterval;
	int			m_nMaxStatFileSize;
	int			m_nNextMaxKBPS;
	bool			m_bTraceThrottle;
	bool			m_bCompression;

	CMap<int, int, CTDMFReplicationPair *, CTDMFReplicationPair *> m_mapReplicationPairs;


	// Array of throttles to be added
   CArray<CTDMFThrottles , CTDMFThrottles > m_arThrottles;

};

//////////////////////////////////////////////////////////////////////
// CTDMFReplicationPair Class

class CTDMFReplicationPair : public CTDMFObject  
{
public:


	CTDMFReplicationPair();
	DECLARE_DYNCREATE(CTDMFReplicationPair)
	CTDMFReplicationPair(int nID, BOOL bNew = true);
	virtual ~CTDMFReplicationPair();

	int		m_nDtcID;	
	int		m_nLogicalGroupID;	
	int		m_nSourceServerID;
	CString	m_strNotes;

	CString  m_strDeviceName;
	CString  m_strSourceDisk;		
	CString  m_strSourceDisk1;
	CString  m_strSourceDisk2;
	CString  m_strSourceDisk3;

	CString  m_strTargetDisk;
	CString  m_strTargetDisk1;
	CString  m_strTargetDisk2;
	CString  m_strTargetDisk3;

};

//////////////////////////////////////////////////////////////////////
// CTDMFVolume Class

class CTDMFVolume : public CTDMFObject  
{
public:


	CTDMFVolume();
	DECLARE_DYNCREATE(CTDMFVolume)
	CTDMFVolume(int nID, CString strName, BOOL bNew = true);
	virtual ~CTDMFVolume();

// Attributes

	CString	m_strName;
	bool		m_bAvailable;
	int		m_nSize;

};


//////////////////////////////////////////////////////////////////////
// CTDMFAgentInfo Class

class CTDMFAgentInfo : public CTDMFObject  
{
public:

	CTDMFAgentInfo();
	DECLARE_DYNCREATE(CTDMFAgentInfo)
	CTDMFAgentInfo(int nID, CString strMachine, BOOL bNew = true);
	virtual ~CTDMFAgentInfo();

// Attributes
	CString		m_strMachine;				//name of machine where this TDMF Agent runs.
	CString		m_strNumberOfValidIP;	//nbr of valid szIP strings
	CString		m_strIPAgent;				//dotted decimal format
	int			m_strPortNumber;			//port nbr exposed by this TDMF Agent
	CString		m_strIPRouter;				//dotted decimal format, can be "0.0.0.0" if no router 
	CString		m_strTDMFDomain;			//name of TDMF domain 
	
};

//////////////////////////////////////////////////////////////////////
// CTDMFRegistrationKey Class

class CTDMFRegistrationKey : public CTDMFObject  
{
public:

	CTDMFRegistrationKey();
	DECLARE_DYNCREATE(CTDMFRegistrationKey)
	CTDMFRegistrationKey(int nID, CString strMachine, CString strRegistrationKey, BOOL bNew = true);
	virtual ~CTDMFRegistrationKey();

// Attributes
	CString		m_strMachine;				//name of machine where this TDMF Agent runs.
	CString		m_strRegistrationKey;	//Registration Key
	
	
};

//////////////////////////////////////////////////////////////////////
// CTDMFAllServers Class

class CTDMFAllServers : public CTDMFObject  
{
public:
	BOOL LoadAllServers();


	CTDMFAllServers();
	DECLARE_DYNCREATE(CTDMFAllServers)
	CTDMFAllServers(int nID, BOOL bNew = true);
	virtual ~CTDMFAllServers();

// Attributes
	CTypedPtrArray<CObArray, CTDMFServer*>	m_arServers;
};

//////////////////////////////////////////////////////////////////////
// CTDMFMonitorInfo Class

class CTDMFMonitorInfo : public CTDMFObject  
{
public:

	CTDMFMonitorInfo();
	DECLARE_DYNCREATE(CTDMFMonitorInfo)
	CTDMFMonitorInfo(int nID, BOOL bNew = true);
	virtual ~CTDMFMonitorInfo();

// Attributes
	CString		m_strServer;		
	CString		m_strLogicalGroup;
	CString		m_strConnection;	
	CString		m_strReplicationPair;	
	CString		m_strMode;	
	double		m_dLocalRead; //in Kbs		
	double		m_dLocalWrite; //in Kbs		
	double		m_dNetActual; //in Kbs		
	double		m_dNetEffect; //in Kbs		
	int			m_nEntries;
 	double		m_dPercentBABUsed;	
	
};

//////////////////////////////////////////////////////////////////////
// CTDMFEventInfo Class

class CTDMFEventInfo : public CTDMFObject  
{
public:

	CTDMFEventInfo();
	DECLARE_DYNCREATE(CTDMFEventInfo)
	CTDMFEventInfo(int nID, BOOL bNew = true);
	virtual ~CTDMFEventInfo();

// Attributes
	COleDateTime 	m_dtEventTime;		
	CString			m_strMessageType;
	CString			m_strMessage;	
	int				m_nSeverity;	
};

//////////////////////////////////////////////////////////////////////
// CTDMFSystemConfiguration Class

class CTDMFSystemConfiguration : public CTDMFObject  
{
public:
	BOOL LoadAllLogicalGroups();

	CTDMFSystemConfiguration();
	DECLARE_DYNCREATE(CTDMFSystemConfiguration)
	CTDMFSystemConfiguration(int nID,BOOL bNew = true);
	virtual ~CTDMFSystemConfiguration();

// Attributes




  	CMap<int, int, CTDMFLogicalGroup *, CTDMFLogicalGroup *> m_mapLogicalGroups;	
};




//////////////////////////////////////////////////////////////////////
// CTDMFThrottles Class

class CTDMFThrottles : public CTDMFObject  
{
public:

	CTDMFThrottles();
	DECLARE_DYNCREATE(CTDMFThrottles)
	CTDMFThrottles(int nID, BOOL bNew = true);
	virtual ~CTDMFThrottles();


	CString	m_strExpression;
	BOOL		m_bAlways;
	CString  m_strTimeRange;
   CString  m_strActionList; 

};
#endif // !defined(AFX_TDMFOBJECTS_H__46E16461_12EA_11D3_AAB2_14BE04C10E27__INCLUDED_)
