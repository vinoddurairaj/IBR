/*
 * DBServer_NotificationEvents.cpp - Management of all TDMF Agent status 
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

#include "stdafx.h"
#pragma warning(disable : 4786) //get rid of annoying STL warning
#include <afx.h>
#include "libmngtnep.h"
#include "DBServer_NotificationEvents.h"

using namespace std;

#if 0
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

inline enum NotificationMsgTypes     
TDMFNotificationMessage::getType()   const 
{ 
    return m_eType;   
}

inline int     
TDMFNotificationMessage::getLength() const 
{ 
    return m_iLength; 
}

inline const char*   
TDMFNotificationMessage::getData()   const 
{ 
    return m_pData;   
}

inline void    
TDMFNotificationMessage::setType(enum NotificationMsgTypes eType)          
{ 
    m_eType = eType;    
}

void    
TDMFNotificationMessage::setData(int iLength, const char* pData)  
{   
    m_pData = new char[ iLength ];
    memcpy(m_pData, pData, iLength);
    m_iLength = iLength;                
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
        m_pData = new char [m_iLength];
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

    return rcvd == m_iLength;
}
#endif

// ***********************************************************
// ***********************************************************
NotificationEventItem::NotificationEventItem() :
    TDMFNotificationMessage()
{
    //m_hMutex = CreateMutex(0,0,0);

    //set as many consumer bits as there are consumers registered
    m_Consumers = 0;
}

NotificationEventItem::NotificationEventItem(DWORD dwConsumers, enum NotificationMsgTypes eType, int iLength, const char * pData ) :
    TDMFNotificationMessage(eType, iLength, pData)
{
    //set as many consumer bits as there are consumers registered
    m_Consumers = dwConsumers;
}

NotificationEventItem::NotificationEventItem(DWORD dwConsumers, const TDMFNotificationMessage & msg) :
    TDMFNotificationMessage(msg)
{
    //m_hMutex = CreateMutex(0,0,0);

    //set as many consumer bits as there are consumers registered
    m_Consumers = dwConsumers;
}

//copy ctr
NotificationEventItem::NotificationEventItem(const NotificationEventItem & aNotifEvItem)
{
    /*BOOL b = DuplicateHandle(
      GetCurrentProcess(),  // handle to source process
      aRGMData.m_hMutex,         // handle to duplicate
      GetCurrentProcess(),  // handle to target process
      &m_hMutex,      // duplicate handle
      0,        // requested access
      0,          // handle inheritance option
      DUPLICATE_SAME_ACCESS               // optional actions
    );_ASSERT(b);*/

    m_Consumers = aNotifEvItem.m_Consumers;
    //init the base class TDMFNotificationMessage obj
    assign(aNotifEvItem);
}

NotificationEventItem::~NotificationEventItem()
{
    /*if ( m_hMutex != NULL ) 
        CloseHandle(m_hMutex);
    m_hMutex = NULL;*/
}

NotificationEventItem& NotificationEventItem::operator=(const NotificationEventItem & aNotifEvItem)
{
    /*BOOL b = DuplicateHandle(
      GetCurrentProcess(),  // handle to source process
      aRGMData.m_hMutex,         // handle to duplicate
      GetCurrentProcess(),  // handle to target process
      &m_hMutex,      // duplicate handle
      0,        // requested access
      0,          // handle inheritance option
      DUPLICATE_SAME_ACCESS               // optional actions
    );_ASSERT(b);*/

    m_Consumers = aNotifEvItem.m_Consumers;
    //init the base class TDMFNotificationMessage obj
    assign(aNotifEvItem);

    return *this;
}

bool
NotificationEventItem::operator==(const NotificationEventItem & aNotifEvItem) const
{
    const TDMFNotificationMessage *base1 = &aNotifEvItem;
    const TDMFNotificationMessage *base2 = this;

    return m_Consumers == aNotifEvItem.m_Consumers &&
           *base1 == *base2;
}

bool  NotificationEventItem::Lock()
{
    /*if ( m_hMutex != NULL )
        return WAIT_OBJECT_0 == WaitForSingleObject( m_hMutex, 60*1000 );*/
    return true;
}

void  NotificationEventItem::Unlock()
{
    //LeaveCriticalSection(&m_hCS);
    /*if ( m_hMutex != NULL ) {
        BOOL b = ReleaseMutex( m_hMutex );_ASSERT(b);
    }*/
}

inline DWORD   
NotificationEventItem::getItemConsumers() const
{
    return m_Consumers;
}

inline bool     
NotificationEventItem::setItemAsRead( DWORD dwConsumerID )
{ 
    m_Consumers &= ~CONSUMER_BIT(dwConsumerID);//clear one bit
    return m_Consumers == 0;
}
  
 
// ***********************************************************
// ***********************************************************
NotificationEventFifo::NotificationEventFifo()
{
    InitializeCriticalSection(&m_csLock);
    Initialize();
}

NotificationEventFifo::~NotificationEventFifo()
{
    m_listEventItem.clear();
    DeleteCriticalSection(&m_csLock);
}

NotificationEventFifo::Initialize()
{
    for ( int i = 0 ; i < MAX_CONSUMERS ; i++ )
        m_ConsumerIndexFromThreadID[i] = INVALID_TID;
}

NotificationEventFifo::AddEvent(enum NotificationMsgTypes iType, int iLength, const char * pData)
{
    m_listEventItem.push_back( NotificationEventItem(m_ActualConsumersMask, iType, iLength, pData) );
}

NotificationEventFifo::AddEvent( const TDMFNotificationMessage & msg )
{
    m_listEventItem.push_back( NotificationEventItem(m_ActualConsumersMask, msg) );
}

//get oldest unread TDMFNotificationMessage for this consumer
NotificationEventItem& 
NotificationEventFifo::getEventItemForConsumer( DWORD consumerId ) /*const*/
{
    //scan list and clear this customer bit from all items
    EventItemList::iterator it  = m_listEventItem.begin();
    EventItemList::iterator end = m_listEventItem.end();
    while( it != end )
    {
        if ( (*it).getItemConsumers() & CONSUMER_BIT(consumerId) )
        {   //found item for this consumer
            EventItemList::reference ref = *it;
            return ref ;
        }
        it++;
    }

    //invalid item
    return *end;
}

void
NotificationEventFifo::removeEventItemForConsumer( NotificationEventItem& item , DWORD consumerId ) 
{
    MarkItemAsRead( consumerId, item );
}

DWORD NotificationEventFifo::AddConsumer( DWORD tid )
{
    for(DWORD consumerId=0; consumerId<MAX_CONSUMERS; consumerId++)
    {
        if ( m_ConsumerIndexFromThreadID[consumerId] == INVALID_TID )
        {   //assign this slot to the consumer
            m_ConsumerIndexFromThreadID[consumerId] = tid;
            m_ActualConsumersMask |= CONSUMER_BIT(consumerId);
            return consumerId;
        }
    }
    return -1;
}

void NotificationEventFifo::DelConsumer( DWORD tid )
{
    for(DWORD consumerId=0; consumerId<MAX_CONSUMERS; consumerId++)
    {
        if ( m_ConsumerIndexFromThreadID[consumerId] == tid )
        {   //free this slot 
            m_ConsumerIndexFromThreadID[consumerId] = INVALID_TID;
            m_ActualConsumersMask &= ~CONSUMER_BIT(consumerId);
            //scan ALL list and clear this customer bit from all items
            EventItemList::iterator it  = m_listEventItem.begin();
            EventItemList::iterator end = m_listEventItem.end();
            while( it != end )
            {
                MarkItemAsRead( consumerId, it );
                it++;
            }
            return;
        }
    }
}

void NotificationEventFifo::MarkItemAsRead( DWORD consumerId,  EventItemList::iterator it )
{
    if ( (*it).setItemAsRead( consumerId ) )
    {   //item read by ALL consumers, remove item from list 
        m_listEventItem.erase(it);
    }
}

void NotificationEventFifo::MarkItemAsRead( DWORD consumerId, NotificationEventItem& item )
{
    if ( item.setItemAsRead( consumerId ) )
    {   //item read by ALL consumers, remove item from list 
        m_listEventItem.remove(item);
    }
}