/*
 * libmngtnep.cpp - TDMF Management Notification Event Protocol
 *
 *  **************************************************************
 *  *** note : this code has not been entirely tested 
 *             and should be considered as a near Beta version ***
 *  **************************************************************
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

#include <windows.h>
#include <crtdbg.h>
#include "TDMFNotificationSubscriptionMessage.h"

#define SUBSCRIPTION_SERVER_TAG      "SRV:"
#define SUBSCRIPTION_GROUP_TAG       "GRP:"
#define SUBSCRIPTION_END_TAG         "END."
#define SUBSCRIPTION_SERVER_TAG_LONG (*((int*)"SRV:"))
#define SUBSCRIPTION_GROUP_TAG_LONG  (*((int*)"GRP:"))
#define SUBSCRIPTION_END_TAG_LONG    (*((int*)"END."))

#define SUBSCRIPTION_ADD_TAG        '+'
#define SUBSCRIPTION_DEL_TAG        '-'
#define SUBSCRIPTION_STATE_TAG      'S'
#define SUBSCRIPTION_FULLSTATS_TAG  'F'

TDMFNotificationSubscriptionMessage::TDMFNotificationSubscriptionMessage()
{
    m_bLocalCopy = false;
    m_pData      = 0;
    m_nMaxDataLen   = 0;
}

TDMFNotificationSubscriptionMessage::TDMFNotificationSubscriptionMessage(const char *pData, int nDataLen, bool bCopyData)
{
    setData(pData, nDataLen, bCopyData );
}

TDMFNotificationSubscriptionMessage::~TDMFNotificationSubscriptionMessage()
{
    if ( m_bLocalCopy )
        delete [] m_pData;
}

TDMFNotificationSubscriptionMessage::setData(const char *pData, int nDataLen, bool bCopyData )
{
    m_bLocalCopy = bCopyData;
    if ( bCopyData )
    {
        m_pData = new char [nDataLen];
        memcpy(m_pData,pData,nDataLen);
    }
    else
    {
        m_pData = (char*)pData;
    }
    m_nMaxDataLen = nDataLen;
}

//once data is set, analyse it.
bool TDMFNotificationSubscriptionMessage::parseData()
{
    bool bParse = true;
    char *pData;
    int  iCurrentContext = 0;
    int  iCurrentHostID  = 0;
    int  iCurrentGrpNbr  = 0;

    pData = m_pData;
    while( bParse )
    {
        int *pTag = (int*)pData;

        if ( *pTag == SUBSCRIPTION_SERVER_TAG_LONG )
        {
            pData += 4;//skip SUBSCRIPTION_SERVER_TAG ( 4 bytes long )
            iCurrentHostID = ntohl( *((int*)pData) );
            pData += 4;
            parseServerSection(&pData,iCurrentHostID);
        }
        else if ( *pTag == SUBSCRIPTION_GROUP_TAG_LONG )
        {
            pData += 4;
            parseGroupSection(&pData,iCurrentHostID);
        }
        else if ( *pTag == SUBSCRIPTION_END_TAG_LONG )
        {
            bParse = false;
        }
        else
        {
            _ASSERT(0);
            return false;
        }
    }
    return true;
}

bool TDMFNotificationSubscriptionMessage::parseServerSection(char ** ppData, int iCurrentHostID)
{
    char *pData     = *ppData;
    enum eSubscriptionAction    eAct;
    enum eSubscriptionDataType  eType;
    bool bParse     = true;
    bool bGoForType;

    while ( bParse )
    {
        bGoForType = false;

        if ( *pData == SUBSCRIPTION_ADD_TAG )
        {
            eAct = ACT_ADD;     pData++;
            bGoForType = true;
        }
        else if ( *pData == SUBSCRIPTION_DEL_TAG )
        {
            eAct = ACT_DELETE;  pData++;
            bGoForType = true;
        }
        else
        {
            bParse = false;
        }

        if ( bGoForType )
        {
            if ( *pData == SUBSCRIPTION_STATE_TAG )
            {
                eType = TYPE_STATE;      pData += 3;
                serverSubscription( iCurrentHostID, eAct, eType );
            }
            else if ( *pData == SUBSCRIPTION_FULLSTATS_TAG )
            {
                eType = TYPE_FULL_STATS; pData += 3;
                serverSubscription( iCurrentHostID, eAct, eType );
            }
            else
            {
                _ASSERT(0);
                return false;
            }
        }
    }

    *ppData = pData;
    return true;
}
bool TDMFNotificationSubscriptionMessage::parseGroupSection(char ** ppData, int iCurrentHostID)
{
    char *pData     = *ppData;
    enum eSubscriptionAction    eAct;
    enum eSubscriptionDataType  eType;
    bool bParse     = true;
    bool bGoForType;
    int  iCurrentGroupNbr;

    while ( bParse )
    {
        bGoForType = false;

        if ( *pData == SUBSCRIPTION_ADD_TAG )
        {
            eAct = ACT_ADD;     pData++;
            bGoForType = true;
        }
        else if ( *pData == SUBSCRIPTION_DEL_TAG )
        {
            eAct = ACT_DELETE;  pData++;
            bGoForType = true;
        }
        else
        {
            bParse = false;
        }

        if ( bGoForType )
        {
            if ( *pData == SUBSCRIPTION_STATE_TAG )
            {
                eType = TYPE_STATE;      pData ++;
                iCurrentGroupNbr = ntohs( *((short*)pData) );  pData += 2;
                replgroupSubscription( iCurrentHostID, iCurrentGroupNbr, eAct, eType );
            }
            else if ( *pData == SUBSCRIPTION_FULLSTATS_TAG )
            {
                eType = TYPE_FULL_STATS; pData ++;
                iCurrentGroupNbr = ntohs( *((short*)pData) );  pData += 2;
                replgroupSubscription( iCurrentHostID, iCurrentGroupNbr, eAct, eType );
            }
            else
            {
                _ASSERT(0);
            }
        }
    }

    *ppData = pData;
    return true;
}


void    TDMFNotificationSubscriptionMessage::requestInit()
{
    m_CurHostId   = -1;
    m_CurGroupNbr = -1;

    if ( m_bLocalCopy )
        delete [] m_pData;

    m_nMaxDataLen   = 1024;
    m_pData         = new char [m_nMaxDataLen];
    m_bLocalCopy    = true;//destructor has to delete m_pData 
    m_CurDataLen    = 0;
}

bool    TDMFNotificationSubscriptionMessage::requestServer( int iHostID )
{
    char data[8];

    if ( (m_CurContextServer && m_CurHostId != iHostID) ||
         !m_CurContextServer )           
    {
        //first 4 bytes = "SRV:"
        strcpy( data, SUBSCRIPTION_SERVER_TAG );
        //next 4 bytes = hostid 
        int netHostID = htonl( iHostID );
        memcpy( &data[4], &netHostID, 4 );//save Host ID as binary data
        appendToMsgData( data, 8 );
        m_CurContextServer = true;
    }

    return true;
}

bool    TDMFNotificationSubscriptionMessage::requestServer( int iHostID, enum eSubscriptionAction eAct, enum eSubscriptionDataType eType)
{
    char data[8];

    if ( (m_CurContextServer && m_CurHostId != iHostID) ||
         !m_CurContextServer )           
    {
        //first 4 bytes = "SRV:"
        strcpy( data, SUBSCRIPTION_SERVER_TAG );
        //next 4 bytes = hostid 
        int netHostID = htonl( iHostID );
        memcpy( &data[4], &netHostID, 4 );//save Host ID as binary data
        appendToMsgData( data, 8 );
        m_CurContextServer = true;
    }

    if ( eAct == ACT_ADD )
        data[0] = SUBSCRIPTION_ADD_TAG;
    else if ( eAct == ACT_DELETE )
        data[0] = SUBSCRIPTION_DEL_TAG;
    else
        _ASSERT(0);

    if ( eType == TYPE_STATE )
        data[1] = SUBSCRIPTION_STATE_TAG;
    else if ( eType == TYPE_FULL_STATS )
        data[1] = SUBSCRIPTION_FULLSTATS_TAG;
    else
        _ASSERT(0);

    data[2] = data[3] = ' ';
    appendToMsgData( data, 4 );

    return true;
}

bool    TDMFNotificationSubscriptionMessage::requestGroup ( int iGroupNumber, enum eSubscriptionAction eAct, enum eSubscriptionDataType eType)
{
    char data[8];

    if ( m_CurContextServer )           
    {   //first 4 bytes = "GRP:"
        appendToMsgData( SUBSCRIPTION_GROUP_TAG, 4 );
        m_CurContextServer = false;
    }

    if ( eAct == ACT_ADD )
        data[0] = SUBSCRIPTION_ADD_TAG;
    else if ( eAct == ACT_DELETE )
        data[0] = SUBSCRIPTION_DEL_TAG;
    else
        _ASSERT(0);

    if ( eType == TYPE_STATE )
        data[1] = SUBSCRIPTION_STATE_TAG;
    else if ( eType == TYPE_FULL_STATS )
        data[1] = SUBSCRIPTION_FULLSTATS_TAG;
    else
        _ASSERT(0);

    //next 2 bytes = group number 
    short netGroupNbr = htons( iGroupNumber );
    memcpy( &data[2], &netGroupNbr, 2 );//save Host ID as binary data

    appendToMsgData( data, 4 );

    return true;
}

void    TDMFNotificationSubscriptionMessage::requestComplete()
{
    appendToMsgData( SUBSCRIPTION_END_TAG, 4 );
}

void    TDMFNotificationSubscriptionMessage::appendToMsgData( const char *pData, int size )
{
    if ( m_CurDataLen + size > m_nMaxDataLen )
    {
        char *tmp = new char [m_nMaxDataLen + size + 1024];//extend buffer
        memcpy( tmp, m_pData, m_nMaxDataLen );//copy existing data
        delete [] m_pData;
        m_pData         = tmp;
        m_nMaxDataLen   = m_nMaxDataLen + size + 1024;
    }
    //append new data
    memcpy( m_pData+m_CurDataLen, pData, size );//append new data
    m_CurDataLen += size;
}

const char*   TDMFNotificationSubscriptionMessage::getSubscriptionRequestData()
{
    return m_pData;
}



