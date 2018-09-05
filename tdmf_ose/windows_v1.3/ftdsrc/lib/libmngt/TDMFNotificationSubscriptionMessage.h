/*
 * TDMFNotificationSubscriptionMessage.h - Manages the data related to the Subscription to TDMF Notifications.
 *
 * Copyright (c) 2002 Fujitsu Softek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef _TDMFNOTIFICATIONSUBSCRIPTIONMESSAGE_H_
#define _TDMFNOTIFICATIONSUBSCRIPTIONMESSAGE_H_

enum eSubscriptionAction
{
    ACT_ADD,    
    ACT_DELETE  
};

enum eSubscriptionDataType
{
    TYPE_STATE,      
    TYPE_FULL_STATS  
};

/*
    NOTIF_MSG_SUBSCRIPTION message data portion:

    The data received will describe Server and/or Group subscription details:

    'S','R','V',':' followed by the 32-bit unique Host ID : identifies the Agent/Server

    '+','S',' ',' '     : request for State of current Agent/Server
    '-','S',' ',' '     : request to remove State of current Agent/Server
    
    'G','R','P',':'                : start of Repl.Group related flags (Repl.Group of current Agent/Server)
    '+','F', group number (16-bit) : request for Repl.Group FULL Statistics (ftd_perf_instance_t)
    '+','S', group number (16-bit) : request for Repl.Group State 

    'E','N','D','.'     : end of subscription instructions. (end of message data)

 */

//*****************************************************************************
class TDMFNotificationSubscriptionMessage
{
public:

    TDMFNotificationSubscriptionMessage();
    TDMFNotificationSubscriptionMessage(const char *pData, int nDataLen, bool bCopyData = true );
    ~TDMFNotificationSubscriptionMessage();

    //setters
    setData(const char *pData, int nDataLen, bool bCopyData = true );
    //once data is set, analyse it.
    bool    parseData();


    //used to build a message data buiffer for eventual transmission.
    void    requestInit();
    bool    requestServer( int iHostID,  enum eSubscriptionAction eAct, enum eSubscriptionDataType eType );
    bool    requestServer( int iHostID );
    bool    requestGroup ( int iGroupNumber, enum eSubscriptionAction eAct, enum eSubscriptionDataType eType );
    void    requestComplete();
    //retreive buffer 
    const char* getSubscriptionRequestData();
    int         getSubscriptionRequestDataLen() { return m_CurDataLen; };


    /*
     *  Override these methods to be notified by parseData()
     */
    virtual bool serverSubscription     ( int iHostID, enum eSubscriptionAction eAct, enum eSubscriptionDataType eType) {return true;} ;
    virtual bool replgroupSubscription  ( int iHostID, int iGroupNumber, enum eSubscriptionAction eAct, enum eSubscriptionDataType eType) {return true;} ;

protected:
    bool    parseServerSection (char ** ppData, int iCurrentHostID);
    bool    parseGroupSection  (char ** ppData, int iCurrentHostID);
    void    appendToMsgData( const char *pData, int size );

    bool    m_bLocalCopy;
    char    *m_pData;
    int     m_nMaxDataLen;

    //members used by request...() methods
    int     m_CurHostId,
            m_CurGroupNbr,
            m_CurDataLen;
    bool    m_CurContextServer;//true for Server, false for Group
};


#endif  //_TDMFNOTIFICATIONSUBSCRIPTIONMESSAGE_H_
