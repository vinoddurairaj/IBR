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
#include <crtdbg.h>

#pragma warning(disable : 4786) //get rid of annoying STL warning
extern "C" 
{
#include "sock.h"
}
#include "libmngtnep.h"
#include "libmngtmsg.h"

using namespace std;

// ***********************************************************
// ***********************************************************
TDMFNotificationMessage::TDMFNotificationMessage(enum NotificationMsgTypes eType, int iLength , const char * pData )
{
    setType(eType);
    setData(iLength,pData);
}

TDMFNotificationMessage::TDMFNotificationMessage(const TDMFNotificationMessage & objToCopy)
{
    setType(objToCopy.m_eType);
    setData(objToCopy.m_iLength,objToCopy.m_pData);
}

TDMFNotificationMessage::~TDMFNotificationMessage()
{
    if ( !bExternalDataBuffer )
        delete [] m_pData;
}

TDMFNotificationMessage& 
TDMFNotificationMessage::operator=(const TDMFNotificationMessage & objToCopy)
{
    setType(objToCopy.m_eType);
    if ( objToCopy.bExternalDataBuffer )
        setDataExt(objToCopy.m_iLength,objToCopy.m_pData);
    else
        setData(objToCopy.m_iLength,objToCopy.m_pData);
    return *this;
}

bool                     
TDMFNotificationMessage::operator==(const TDMFNotificationMessage & objToTest) const
{
    if ( m_pData == 0 && objToTest.m_pData == 0 ||
         m_pData != 0 && objToTest.m_pData != 0 )
    {
        return  objToTest.m_eType == m_eType        &&
                objToTest.m_iLength == m_iLength    &&
                memcmp(objToTest.m_pData,m_pData,m_iLength) == 0;
    }
    return false;
}

void    
TDMFNotificationMessage::setData(int iLength, const char* pData)  
{   
    if ( iLength > 0 )
    {
        m_pData = new char[ iLength ];
        memcpy(m_pData, pData, iLength);
        m_iLength = iLength;                
    }
    else
    {
        m_pData   = 0;
        m_iLength = 0;                
    }
    bExternalDataBuffer = false;
}

void    
TDMFNotificationMessage::setDataExt(int iLength, const char* pData)  
{   
    m_pData = (char*)pData;
    m_iLength = iLength;                
    bExternalDataBuffer = true;
}

bool            
TDMFNotificationMessage::IsValid()
{
    return isKnownType( m_eType );
}

bool    
TDMFNotificationMessage::isKnownType()//check if m_eType is known by this object version
{
    return isKnownType( m_eType );
}

bool    
TDMFNotificationMessage::isKnownType(enum NotificationMsgTypes eType)//check if iType is known object version
{
    return isKnownType( (int)eType );
}

bool    
TDMFNotificationMessage::isKnownType(int iType)//check if iType is known object version
{
    return iType > NOTIF_MSG_TYPE_FIRST && iType < NOTIF_MSG_TYPE_LAST;
}

void 
TDMFNotificationMessage::assign(const TDMFNotificationMessage & objToCopy)
{
    setType(objToCopy.m_eType);
    if ( objToCopy.bExternalDataBuffer )
        setDataExt(objToCopy.m_iLength,objToCopy.m_pData);
    else
        setData(objToCopy.m_iLength,objToCopy.m_pData);
}

bool
TDMFNotificationMessage::Send( sock_t * s )
{
    unsigned int data;
    //send monitoring data over socket
    data = htonl(NOTIFICATION_MSG_HDR_MAGIC_NUMBER);
    if( 4 != sock_send(s, (char *)&data, 4) )
        return false;

    data = htonl((u_long)m_eType);
    if( 4 != sock_send(s, (char *)&data, 4) )
        return false;

    data = htonl((u_long)m_iLength);
    if( 4 != sock_send(s, (char *)&data, 4) )
        return false;

    if ( m_iLength > 0 )
    {
        if( m_iLength != sock_send(s, m_pData, m_iLength) )
            return false;
    }

    return true;
}

bool            
TDMFNotificationMessage::Recv( sock_t * s, int inactivityTimeout /*millisecs*/ )
{
    unsigned int data;
    int r; 

    r = sock_check_recv(s, inactivityTimeout*1000); //msec to microsecs
    if ( r != 1 )
        return false;
    //first 4 bytes = Magic number
    sock_recv(s,(char*)&data,4);
    if ( ntohl(data) != NOTIFICATION_MSG_HDR_MAGIC_NUMBER )
        return false;

    r = sock_check_recv(s, inactivityTimeout*1000); //msec to microsecs
    if ( r != 1 )
        return false;
    //next 4 bytes = Type
    sock_recv(s,(char*)&data,4);
    data = ntohl(data);
    if ( data <= NOTIF_MSG_TYPE_FIRST || data >= NOTIF_MSG_TYPE_LAST )
        return false;
    m_eType = (NotificationMsgTypes) data;

    r = sock_check_recv(s, inactivityTimeout*1000); //msec to microsecs
    if ( r != 1 )
        return false;
    //next 4 bytes = Length
    data = 0xffffffff;
    sock_recv(s,(char*)&data,4);
    m_iLength  = ntohl(data);//length to receive

    int rcvd = 0;
    if ( m_iLength > 0 )
    {
        //next bytes : msg data
        if ( !bExternalDataBuffer && m_pData != 0 )
            delete [] m_pData;

        m_pData = new char [m_iLength];
        bExternalDataBuffer = false;
        int r;
        do
        {   
            r = sock_check_recv(s, inactivityTimeout*1000);
            if ( r == 1 )
            {
                r = sock_recv(s,m_pData+rcvd,m_iLength-rcvd);
                if ( r > 0 )
                {
                    rcvd += r;
                }
            }
            else
            {   //to exit loop
                r = -1;
            }
        } while( r > 0 && rcvd < m_iLength );
    }

    if ( m_eType == NOTIF_MSG_RG_RAW_PERF_DATA              ||
         m_eType == NOTIF_MSG_RG_STATUS_AND_MODE            || 
         m_eType == NOTIF_MSG_SERVER_ACTIVITY               || 
         m_eType == NOTIF_MSG_SERVER_NO_ACTIVITY            || 
         m_eType == NOTIF_MSG_SERVER_COMMUNICATION_PROBLEM  )
    {
        _ASSERT(rcvd == m_iLength);

        if ( rcvd == m_iLength )
            NtoH();
    }

    return rcvd == m_iLength;
}

//network to host byte order convertion on known structures
void TDMFNotificationMessage::NtoH()
{
    const char *data    = m_pData;
    const char *dataend = m_pData + m_iLength;
    while ( data < dataend )
    {
        if ( m_eType == NOTIF_MSG_RG_RAW_PERF_DATA )
        {
            if ( data + sizeof(mnep_NotifMsgDataRepGrpRawPerf) <= dataend )
                mnep_convert_ntoh((mnep_NotifMsgDataRepGrpRawPerf *)data);

            data += sizeof(mnep_NotifMsgDataRepGrpRawPerf);
        }
        else if ( m_eType == NOTIF_MSG_RG_STATUS_AND_MODE )
        {
            if ( data + sizeof(mnep_NotifMsgDataRepGrpStatusMode) <= dataend )
                mnep_convert_ntoh((mnep_NotifMsgDataRepGrpStatusMode *)data);

            data += sizeof(mnep_NotifMsgDataRepGrpStatusMode);
        }
        else if (m_eType == NOTIF_MSG_SERVER_COMMUNICATION_PROBLEM )
        {
            if ( data + sizeof(mnep_NotifMsgDataAgentUID) <= dataend )
                mnep_convert_ntoh((mnep_NotifMsgDataAgentUID *)data);

            data += sizeof(mnep_NotifMsgDataAgentUID);
        }
        else if ( m_eType == NOTIF_MSG_SERVER_ACTIVITY || 
                  m_eType == NOTIF_MSG_SERVER_NO_ACTIVITY )
        {
            if ( data + sizeof(mnep_NotifMsgDataServerInfo) <= dataend )
                mnep_convert_ntoh((mnep_NotifMsgDataServerInfo*)data);

            data += sizeof(mnep_NotifMsgDataServerInfo);
        }
        else 
        {
            //_ASSERT(0);
            return;
        }
    }
}

// ***********************************************************
// ***********************************************************
void mnep_convert_hton(mnep_NotifMsgDataRepGrpStatusMode *    data)
{
    mnep_convert_hton( &data->agentUID );

    data->sConnection       =   htons(data->sConnection);
    data->sPctDone          =   htons(data->sPctDone);
    data->sRepGroupNumber   =   htons(data->sRepGroupNumber);
    data->sState            =   htons(data->sState);

    data->liJournalDiskFreeSz   = htonI64(data->liJournalDiskFreeSz);
    data->liJournalDiskTotalSz  = htonI64(data->liJournalDiskTotalSz);
    data->liJournalSz           = htonI64(data->liJournalSz);
    data->liPStoreDiskFreeSz    = htonI64(data->liPStoreDiskFreeSz);
    data->liPStoreDiskTotalSz   = htonI64(data->liPStoreDiskTotalSz);
    data->liPStoreSz            = htonI64(data->liPStoreSz);

}

void mnep_convert_ntoh(mnep_NotifMsgDataRepGrpStatusMode *    data)
{
    mnep_convert_ntoh( &data->agentUID );

    data->sConnection       =   ntohs(data->sConnection);
    data->sPctDone          =   ntohs(data->sPctDone);
    data->sRepGroupNumber   =   ntohs(data->sRepGroupNumber);
    data->sState            =   ntohs(data->sState);

    data->liJournalDiskFreeSz   = ntohI64(data->liJournalDiskFreeSz);
    data->liJournalDiskTotalSz  = ntohI64(data->liJournalDiskTotalSz);
    data->liJournalSz           = ntohI64(data->liJournalSz);
    data->liPStoreDiskFreeSz    = ntohI64(data->liPStoreDiskFreeSz);
    data->liPStoreDiskTotalSz   = ntohI64(data->liPStoreDiskTotalSz);
    data->liPStoreSz            = ntohI64(data->liPStoreSz);
}

// ***********************************************************
void mnep_convert_hton(mnep_NotifMsgDataAgentUID *            data)
{
    data->iHostID    =  htonl(data->iHostID);   
    data->tStimeTamp =  htonl(data->tStimeTamp);   
}

void mnep_convert_ntoh(mnep_NotifMsgDataAgentUID *            data)
{
    data->iHostID    =  ntohl(data->iHostID);   
    data->tStimeTamp =  ntohl(data->tStimeTamp);   
}

// ***********************************************************
void mnep_convert_hton(mnep_NotifMsgDataRepGrpRawPerf *       data)
{
    mnep_convert_hton( &data->agentUID );
    mmp_convert_hton( &data->perf );
}

void mnep_convert_ntoh(mnep_NotifMsgDataRepGrpRawPerf *       data)
{
    mnep_convert_ntoh( &data->agentUID );
    mmp_convert_ntoh( &data->perf );
}

// ***********************************************************
void mnep_convert_ntoh(mnep_NotifMsgDataServerInfo*  data)
{
	mnep_convert_ntoh( &data->agentUID );
	data->nBABAllocated = ntohl(data->nBABAllocated);
	data->nBABRequested = ntohl(data->nBABRequested);
}
