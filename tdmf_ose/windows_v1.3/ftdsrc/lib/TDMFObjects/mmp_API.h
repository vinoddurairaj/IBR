/*
 * mmp_API.h -	object providing an interface to the TDMF main functionalities
 * 
 * Copyright (c) 2002 Fujitsu SoftTek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef __MMP_API_H_
#define __MMP_API_H_

#include "System.h" //TDMF Objects model
#include "TDMFObjectsDef.h"
#include <afx.h>


//*****************************************************************************
//*****************************************************************************

class MMPAPI_Error
{
public:
    enum ErrorCodes
    {
         OK      = _TDMF_ERROR_CODE_OK
        ,SUCCESS =  _TDMF_ERROR_CODE_SUCCESS

        ,ERR_INTERNAL_ERROR           = _TDMF_ERROR_CODE_INTERNAL_ERROR
        ,ERR_INVALID_PARAMETER        = _TDMF_ERROR_CODE_INVALID_PARAMETER
        ,ERR_ILLEGAL_IP_OR_PORT_VALUE = _TDMF_ERROR_CODE_ILLEGAL_IP_OR_PORT_VALUE
        ,ERR_SENDING_DATA_TO_COLLECTOR        = _TDMF_ERROR_CODE_SENDING_DATA_TO_COLLECTOR //  error while sending data to TDMF Collector
        ,ERR_RECEIVING_DATA_FROM_COLLECTOR    = _TDMF_ERROR_CODE_RECEIVING_DATA_FROM_COLLECTOR //  error while receiving data from TDMF Collector
        ,ERR_UNABLE_TO_CONNECT_TO_COLLECTOR   = _TDMF_ERROR_CODE_UNABLE_TO_CONNECT_TO_COLLECTOR//  unable to connect to TDMF Collector
        ,ERR_UNABLE_TO_CONNECT_TO_TDMF_AGENT  = _TDMF_ERROR_CODE_UNABLE_TO_CONNECT_TO_TDMF_AGENT//  unable to connect to TDMF Agent
        ,ERR_COMM_RUPTURE_WITH_TDMF_AGENT     = _TDMF_ERROR_CODE_COMM_RUPTURE_WITH_TDMF_AGENT//  unexpected communication rupture while rx/tx data with TDMF Agent
        ,ERR_COMM_RUPTURE_WITH_TDMF_COLLECTOR = _TDMF_ERROR_CODE_COMM_RUPTURE_WITH_TDMF_COLLECTOR//  unexpected communication rupture while rx/tx data with TDMF Collector

        ,ERR_UNKNOWN_TDMF_AGENT    = _TDMF_ERROR_CODE_UNKNOWN_TDMF_AGENT//  unknown TDMF Agent (szAgentId)
        ,ERR_UNKNOWN_DOMAIN_NAME   = _TDMF_ERROR_CODE_UNKNOWN_DOMAIN_NAME//  
        ,ERR_UNKNOWN_HOST_ID       = _TDMF_ERROR_CODE_UNKNOWN_HOST_ID//  
        ,ERR_UNKNOWN_SOURCE_SERVER = _TDMF_ERROR_CODE_UNKNOWN_SOURCE_SERVER//  
        ,ERR_UNKNOWN_TARGET_SERVER = _TDMF_ERROR_CODE_UNKNOWN_TARGET_SERVER//  
        ,ERR_UNKNOWN_REP_GROUP     = _TDMF_ERROR_CODE_UNKNOWN_REP_GROUP

        ,ERR_SAVING_TDMF_AGENT_CONFIGURATION = _TDMF_ERROR_CODE_SAVING_TDMF_AGENT_CONFIGURATION//  Agent could not save the received Agent config.  Continuing using current config.
        ,ERR_ILLEGAL_TDMF_AGENT_CONFIGURATION_PROVIDED = _TDMF_ERROR_CODE_ILLEGAL_TDMF_AGENT_CONFIGURATION_PROVIDED//provided Agent config. contains illegal values and is rejected by Agent.  Continuing using current config.

        ,ERR_DELETING_SOURCE_REP_GROUP = _TDMF_ERROR_CODE_DELETING_SOURCE_REP_GROUP
        ,ERR_DELETING_TARGET_REP_GROUP = _TDMF_ERROR_CODE_DELETING_TARGET_REP_GROUP

        ,ERR_SET_SOURCE_REP_GROUP = _TDMF_ERROR_CODE_SET_SOURCE_REP_GROUP
        ,ERR_SET_TARGET_REP_GROUP = _TDMF_ERROR_CODE_SET_TARGET_REP_GROUP

        ,ERR_CREATING_DB_RECORD = _TDMF_ERROR_CODE_CREATING_DB_RECORD
		,ERR_UPDATING_DB_RECORD = _TDMF_ERROR_CODE_UPDATING_DB_RECORD
        ,ERR_DELETING_DB_RECORD = _TDMF_ERROR_CODE_DELETING_DB_RECORD
		,ERR_FINDING_DB_RECORD  = _TDMF_ERROR_CODE_FINDING_DB_RECORD
        ,ERR_DATABASE_RELATION_ERROR = _TDMF_ERROR_CODE_DATABASE_RELATION_ERROR
		,ERR_DATABASE_TRANSACTION = _TDMF_ERROR_CODE_DATABASE_TRANSACTION

        ,ERR_AGENT_PROCESSING_TDMF_CMD  = _TDMF_ERROR_CODE_AGENT_PROCESSING_TDMF_CMD

        ,ERR_EMPTY_OBJECT_LIST_PROVIDED = _TDMF_ERROR_CODE_EMPTY_OBJECT_LIST_PROVIDED

        ,ERR_BAD_OR_MISSING_REGISTRATION_KEY = _TDMF_ERROR_CODE_BAD_OR_MISSING_REGISTRATION_KEY

		,ERR_NO_CFG_FILE = _TDMF_ERROR_CODE_NO_CFG_FILE
        ,ERR_UNKNOWN_SCRIPT_SERVER_FILE = _TDMF_ERROR_CODE_UNKNOWN_SCRIPT_SERVER_FILE
        ,ERR_UNKNOWN_SENDING_SCRIPT_SERVER_FILE_TO_AGENT = _TDMF_ERROR_CODE_ERROR_SENDING_SCRIPT_SERVER_FILE_TO_AGENT
    };

    /* ctr */
    MMPAPI_Error( ErrorCodes code = SUCCESS );

    /**
     * true = object indicates success, false = object indicates an error
     */
    inline bool IsOK();

    /* operators */
    /**
     * Converts the error code to an integer
     * 
     */
    inline operator int();

    /**
     * Returns a message corresponding to the error code 
     */
    operator CString();

    /* setter */
    inline void setMessage(const char * msg);
    inline void setMessage(const CString & msg);

    void setCode( ErrorCodes code );
    void setFromMMPCode(int r);


protected:

    /* members */
    ErrorCodes      m_code;
    CString         m_cszMsg; 
};

inline MMPAPI_Error::operator int()
{
    return (int)m_code;
}

inline bool MMPAPI_Error::IsOK()
{
    return m_code == SUCCESS;
}
  
inline void MMPAPI_Error::setMessage(const char * msg)
{
    m_cszMsg += msg;
}

inline void MMPAPI_Error::setMessage(const CString & msg)
{
    m_cszMsg += msg;
}

//*****************************************************************************
//*****************************************************************************
#include "libmngt.h" //for MMP_HANDLE defn.
#include "libmngtdef.h" //for enum tdmf_commands and mmp_... structures definitions
#include "TDMFNotificationSubscriptionMessage.h"


class FsTdmfDb;
class TDMFNotificationMessage;
class NotifSubscriptionItemSet;
class MMP_API
{
public:
    MMP_API();
    MMP_API(int iCollectorIPAddr, int iCollectorIPPort, FsTdmfDb *db);
    ~MMP_API();

    /* types */

    /* setters */
    void initCollectorIPAddr(int iCollectorIPAddr);
    void initCollectorIPAddr(const char * szCollectorIPAddr);//dotted decimal format expected, "a.b.c.d"
    void initCollectorIPPort(int iCollectorIPPort);
    void initDB(FsTdmfDb *db);
	void initTrace(CSystem* pSystemLog);

	void Trace(unsigned short thislevel, const char* message, ...);

    /* ****************************** */
    /* Actions on Collector           */
    /* ****************************** */
	/**
	 * Request/Verify if this app has ownership over the TDMF system
	 */
    MMPAPI_Error    requestTDMFOwnership(HWND hUID, /*out*/bool * pbOwnershipGranted);
    MMPAPI_Error  	requestTDMFOwnership(const char * szClientUID, /*out*/bool * pbOwnershipGranted);
    /**
     * Release ownership over the TDMF system
     */ 
	MMPAPI_Error  	releaseTDMFOwnership(HWND hUID, /*out*/bool * pbOwnershipGranted);
    MMPAPI_Error  	releaseTDMFOwnership(const char * szClientUID, /*out*/bool * pbOwnershipGranted);

    /**
     * Control over parameters that affect some aspects of the Tdmf Collector
     */
    MMPAPI_Error  	getSystemParameters(mmp_TdmfCollectorParams* params);
    MMPAPI_Error  	setSystemParameters(const mmp_TdmfCollectorParams* params);

   
     /* ****************************** */
    /* Actions on a CScriptServerFile */
    /* ****************************** */
    /**
     * Add or Modify the specified CScriptServerFile from the TDMF system (DB)
     */
     /**
     * Remove the specified CScriptServerFile from the TDMF system (DB)
     */
     MMPAPI_Error    delScriptServerFileFromDb(CScriptServer* pScriptServer );

    /**
     * Add or Modifiy the specified CScriptServerFile from the TDMF system (DB)
     */
     MMPAPI_Error    addScriptServerFileToDb(CScriptServer & ScriptServer );

     /**
     * Send the specified CScriptServerFile to the server
     */
     MMPAPI_Error    SendScriptServerFileToAgent( CScriptServer* pScriptServer );

      /**
     * Import group configuration from Server
     * Upon succes, output params repgrp, strOtherHostName and bIsThisHostSource 
     * are updated according to information received (.cfg file).
     */
     MMPAPI_Error    ImportAllScriptServerFiles( CServer & server,
                                                char* strFileExension,
                                                long lFileType,
                                                std::list<CScriptServer> & listScriptServer);

     MMPAPI_Error    ImportOneScriptServerFile( CServer & server,
                                                char* strFilename,
                                                long lFileType,
                                                CScriptServer & ScriptServer);

  

    /* ****************************** */
    /* Actions on a Replication Group */
    /* ****************************** */
    /**
     * Add or Modifiy the specified RepGroup from the TDMF system (Agent and DB)
     */
    MMPAPI_Error    addRepGroup(CReplicationGroup & repGroup /*group def*/, long nOldGroupNumber, long nOldTargetHostId, std::string* pstrWarning, bool bUseTransaction, bool bDistributeCfgFiles = true);

    /**
     * Remove the specified RepGroup from the TDMF system (Agent and DB)
     */
    MMPAPI_Error    delRepGroup(CReplicationGroup & repGroup /*group def*/, bool bDeleteCfgfiles = true);

	MMPAPI_Error    delSymmetricGroup(CReplicationGroup & repGroup);

    /**
     * Import group configuration from Server
     * Upon succes, output params repgrp, strOtherHostName and bIsThisHostSource 
     * are updated according to information received (.cfg file).
     */
    MMPAPI_Error    getRepGroupCfg( const CServer & server, 
                                    short sGrpNumber, 
                                    CReplicationGroup & repgrp, 
                                    std::string & strHostName, 
                                    std::string & strOtherHostName, 
                                    bool & bIsThisHostSource );

    MMPAPI_Error    getRepGroupCfg( const CServer & server, 
                                    std::list<CReplicationGroup> & listGrp, 
									std::list<std::string> & listPrimaryHostName ,
                                    std::list<std::string> & listTgtHostName );

	/**
	 * Tunables management
	 */
	BOOL            IsTunableModified( CReplicationGroup& repgrp );
	MMPAPI_Error    GetTunables( CReplicationGroup& repGroup, std::string& strOutput );
	MMPAPI_Error    SetTunables( CReplicationGroup& repgrp );
	MMPAPI_Error    SaveTunables( CReplicationGroup& repGroup, bool bUseTransaction = true );
	typedef struct TdmfTunables
	{
		int  nChunkDelay;
		int  nChunkSize;
		bool bCompression;
		bool bSync;
		int  nSyncDepth;
		int  nSyncTimeout;
		int  nRefreshTimeout;
		int  nNetMaxKbps;
		int  nStatInterval;
		int  nMaxStatFileSize;
		bool bJournalLess;
	} TdmfTunables;
	BOOL ParseTunables(std::string& strTunableBuffer, TdmfTunables* pTunables);


    /* ************************************************** */
    /* Common TDMF commands applied on Replication Group  */
    /* ************************************************** */
    /**
     *  Start the Repl.Group.  Parent (Source) Server member must be initialized.
     *  ( tdmfstart -gWXY )
     */
    MMPAPI_Error    tdmfStart           (const CReplicationGroup & repGroup);
    /**
     *  Start ALL Repl.Groups of this (Source) Server.
     *  ( tdmfstart -a )
     */
    MMPAPI_Error    tdmfStart           (const CServer & srcServer);
    /**
     *  Stop the Repl.Group.  Parent (Source) Server member must be initialized.
     *  ( tdmfstop -gWXY )
     */
    MMPAPI_Error    tdmfStop            (const CReplicationGroup & repGroup);
    /**
     *  Stop ALL Repl.Groups of this (Source) Server.
     *  ( tdmfstop -a )
     */
    MMPAPI_Error    tdmfStop            (const CServer & srcServer);
    MMPAPI_Error    tdmfLaunchRefresh   (const CReplicationGroup & repGroup, bool bForce);
    /**
     *  Kill the group's PMD.  Parent (Source) Server member must be initialized.
     *  ( tdmfkillpmd -gWXY )
     */
    MMPAPI_Error    tdmfKillPMD         (const CReplicationGroup & repGroup);
    /**
     *  Kill ALL PMDs of this (Source) Server.
     *  ( tdmfkillpmd -a )
     */
    MMPAPI_Error    tdmfKillPMD         (const CServer & srcServer);
    /* TDMF commands */
    // base tdmf tool method
    MMPAPI_Error    tdmfCmd( int iHostID /*agent host id*/, enum tdmf_commands eCmd, const char *szOptions );

	std::string GetCmdName(enum tdmf_commands eCmd);

    /**
     * Retreive the console text output produced by the last tdmf cmd performed ( tdmf...()  method )
     */
    std::string     getTdmfCmdOutput();

    /* ******************************** */
    /* Actions on a TDMF Server - Agent */
    /* ******************************** */
    MMPAPI_Error    assignDomain(CServer *pServer, CDomain *pDomain);

    /**
     * Call to modify the main listener IP Port of a TDMF Server.
     * The Server will be rebooted in order for port change to take effect.
     * Upon succes, TDMF DB is updated, all Repl.Groups cfg files where this server 
     * is a TARGET are modified.
     * All Repl. Groups on Target Server and will be stopped by this operation.
     */
    //MMPAPI_Error    setIPPort(CServer *pServer, int iPort);
    /**
     * Call when critical cfg values of a TDMF Server are modified.
     * Critical values are: IP Port, TCP Window size and BAB size.
     * To be effective a modification to one of these requires the Server machine to reboot.
     *
     * IP Port modification only:
     * Upon succes, TDMF DB is updated, all Repl.Groups cfg files where this Server 
     * is a TARGET are modified.
     * All Repl. Groups on Target Server and will be stopped by this operation.
     */
    MMPAPI_Error    modifyCriticalServerCfgValues(CServer *pServer, bool bStopPrimaryGroups, bool bIPPortChanged );

    /**
     * Provide a new TDMF Key for the Agent.
     * Key is sent to Agent and saved to TDMF DB.
     */
    MMPAPI_Error    setKey(CServer *pServer, const char* szKey);
	MMPAPI_Error    setCollectorKey(const char* szKey);
	BOOL            IsCollectorKeyValid();

    /**
     * Retreive a list of devices (disks/volumes) of the specified TDMF Server (Agent).
     *
     * Argument         : (out) listCDevice : reference on a STL list object to be filled.
     *                                        list is first cleared from its current content,
     *                                        then devices are added to it.
     */
    MMPAPI_Error    getDevices(const long nHostId, /*out*/std::list<CDevice> & listCDevice, bool bInitInfoFull );
    MMPAPI_Error    getDevices(const long nHostId, bool bInitInfoFull, /*out*/std::list<CDevice> & listSourceDevices,
                               const long nHostIdTarget, bool bInitInfoFullTarget,/*out*/std::list<CDevice> & listTargetDevices );

	MMPAPI_Error    delServer(CServer *pServer);


    /* ******************************************************* */
    /* Notification related requests                           */
    /* Reception of State,Stats,Events,... from TDMF Collector */
    /* ******************************************************* */
    /**
     * Call once to register (Collector) for notification 
     */
    MMPAPI_Error    registerForNotifications(int iPeriod /*seconds*/, const HWND hWnd, UINT uiWinMsg);
    /**
     * Request to be notified using a callback function.
     * The third param., pContext, will be passed as first param to callback function.
     * 
     */
    MMPAPI_Error    registerForNotifications(int iPeriod /*seconds*/, void (*f)(void* /*pContext*/, TDMFNotificationMessage* /*pNotifMsg*/) , void* pContext );
    void            unregisterForNotifications();
    /**
     * Add/Remove notification on one Server/Group or a list of Server/Group.
     */
    MMPAPI_Error    requestNotification(const CReplicationGroup & repGroup, 
                                        enum eSubscriptionAction eAction, 
                                        enum eSubscriptionDataType eType 
                                        );
    MMPAPI_Error    requestNotification(std::list<CReplicationGroup> & listGroup, 
                                        enum eSubscriptionAction eAction, 
                                        enum eSubscriptionDataType eType 
                                        );
    /**
     * only TYPE_STATE statistics can be retreived from a Server (TYPE_FULL_STATS is irrelevant).
     * to get TYPE_FULL_STATS, use the Repl.Group versions of requestNotification().
     */
    MMPAPI_Error    requestNotification(const CServer & server, 
                                        enum eSubscriptionAction eAction
                                        );
    MMPAPI_Error    requestNotification(std::list<CServer> & listServer, 
                                        enum eSubscriptionAction eAction
                                        );

protected:

    sock_t*     m_socket; // 020725
    HANDLE      m_hNotifThread;
    HANDLE      m_hevEndNotifThread;
    HANDLE      m_hevNotifDataReady;
    HANDLE      m_hevNotifDataProcessed;
    int         m_iNotifPeriod;//seconds
    CRITICAL_SECTION m_csNotifRequest;
    HWND        m_hDestHwnd;
    UINT        m_uiDestWinMsg;
    char*       m_pNotifBuffer;
    int         m_iNotifBufferLen;
    void        (*m_fnNotifCallback) (void* pContext, TDMFNotificationMessage* obj);
    void*       m_pContext;
    std::string m_strTdmfCmdOutput;

    MMP_HANDLE  m_hMMP;
    int         m_ip,
                m_port;
    char        m_ipstr[ 32 ];

    FsTdmfDb    *m_db;

	CSystem     *m_pSystemLog;
	
    NotifSubscriptionItemSet *m_SubscriptionsRegisterSet;

    /* methods */
    MMPAPI_Error    createMMPHandle();
    /**
     * Sends new Server Information to specified Agent 
     *
     * Task performed by calling mmp_mngt_setTdmfAgentConfig().
     */
    MMPAPI_Error    collectorTXAgentInfo( CServer *pServer );
    /**
     * Send the Repl.Group configuration to the SOURCE and the TARGET Agents.
     *
     * Task performed by calling mmp_setTdmfAgentLGConfig() for each Agent.
     */
	MMPAPI_Error    SendCfgFileToAgent( CReplicationGroup* repgrp, int iSrcHostId, int iTgtHostId );
	MMPAPI_Error    SendEmptyCfgFileToAgent( CReplicationGroup* repgrp, int iSrcHostId, int iTgtHostId );
    MMPAPI_Error    collectorTXRepGroup( CReplicationGroup* repgrp, int iSrcHostId, int iTgtHostId, bool bSymmetric );
	MMPAPI_Error    RemoveSymmetricFiles( CReplicationGroup* repgrp, int iSrcHostId, int iTgtHostId );

    static DWORD WINAPI staticNotificationThread(void *pThis);

    void                NotificationThread();

    static DWORD WINAPI staticGetDeviceThread(void* pContext);


    /**
     * Used to automatically resync. subscription after reconnection to Collector
     */
    void            requestNotification();
    void            requestToNotifThread(const char * pData, int iDataLen );
    static DWORD WINAPI staticRequestToNotifThread(void *pThis);


    /**
	 * Inter-GUI messages
	 */
public:

	MMPAPI_Error SendTdmfGuiMessage(DWORD nType, BYTE* pByte, UINT nLength);
	MMPAPI_Error RetrieveTdmfGuiMessage(mmp_TdmfGuiMsg* pMsg, DWORD* pnID, DWORD* piType, BYTE** ppByte);

	MMPAPI_Error SendTdmfObjectChangeMessage(int iModule, int iOp, int iDbDomain, int iDbSrvId, int iGroupNumber);
	MMPAPI_Error SendTdmfTextMessage(char* pszMsg);
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Inter-GUI messages

enum GUI_MESSAGES
{
	TDMFGUI_OBJECT_CHANGE,
	TDMFGUI_TEXT_MESSAGE,
};

enum OBJECT_CHANGE_MESSAGES  // TODO: define message format (check with collector)
{
	TDMF_DOMAIN = 0x01,
		TDMF_SERVER = 0x02,
		TDMF_GROUP  = 0x04,
		
		TDMF_ADD_NEW = 0x10,
		TDMF_MODIFY  = 0x20,
		TDMF_REMOVE  = 0x40,
		TDMF_MOVE    = 0x80,
};

typedef struct __TdmfGuiObjectChangeMessage
{
	int iModule;
	int iOp;
	int iDbDomain;
	int iDbSrvId;
	int iGroupNumber;
} TdmfGuiObjectChangeMessage;


#endif //__MMP_API_H_