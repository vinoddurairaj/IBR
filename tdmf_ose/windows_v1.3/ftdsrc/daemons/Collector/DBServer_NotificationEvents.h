/*
 * DBServer_NotificationEvents.h - 
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
#pragma warning(disable : 4786) //get rid of annoying STL warning
#include <list>
#include "libmngtnep.h"

// ***********************************************************
class NotificationEventItem : public TDMFNotificationMessage
{
public:
    NotificationEventItem();
    NotificationEventItem(DWORD dwConsumers, const TDMFNotificationMessage & msg);
    NotificationEventItem(DWORD dwConsumers, enum NotificationMsgTypes eType = NOTIF_MSG_TYPE_INVALID, int iLength = 0, const char * pData = 0);
    //copy ctr
    NotificationEventItem(const NotificationEventItem & aRGMData);
    ~NotificationEventItem();

    NotificationEventItem&  operator=(const NotificationEventItem & aNotifEvItem);
    bool                    operator==(const NotificationEventItem & aNotifEvItem) const;

    inline DWORD   getItemConsumers() const;

    //clears the item read status for the specified consumer.
    //returns true if item is marked as read for ALL consumers.
    inline bool     setItemAsRead( DWORD dwConsumerID );

#ifndef _DEBUG
private:
#endif
    inline  bool    Lock();
    inline  void    Unlock();

    //coordintation between reading and writing perf data to this object
    //HANDLE              m_hMutex;

    //one DWORD value corresponds to 32 Consumers (one bit per consumer)
    //each bit of a value is assigned to one Consumer (max of 32 consumers)
    //when a bit is set, it indicates to the targetted consumer that this Notification message 
    //is available for this consumer.
    //when m_Consumers = 0, then msg has been read by all consumers.
    DWORD   m_Consumers;
};

// ***********************************************************
#define MAX_CONSUMERS       32  //32 bits max in a DWORD
#define INVALID_TID         -1

#define CONSUMER_BIT(c)     (1UL << c)
class NotificationEventFifo 
{
public:
    NotificationEventFifo();
    ~NotificationEventFifo();

    Initialize();

    AddEvent(const TDMFNotificationMessage & msg);
    AddEvent(enum NotificationMsgTypes iType, int iLength, const char * pData);

    //get oldest unread TDMFNotificationMessage for this consumer
    NotificationEventItem& getEventItemForConsumer( DWORD consumerId ) /*const*/ ;
    void  removeEventItemForConsumer( NotificationEventItem& item , DWORD consumerId) ;

    //returns the bit reserved for this consumer , the consumerId
    DWORD AddConsumer( DWORD tid = GetCurrentThreadId() );
    void  DelConsumer( DWORD tid = GetCurrentThreadId() );
    //keep track of a maximum of MAX_CONSUMERS consumers binary states 
    //one DWORD value corresponds to one thread id, a thread id identifying one consumer.
    //the index in the vector at which a consumer finds its thread id 
    //indicates which bit is reserved for it in each of the different m_DataFlags[] values.
    DWORD    m_ConsumerIndexFromThreadID[MAX_CONSUMERS];


protected:
    DWORD           m_ActualConsumersMask;

    typedef std::list< NotificationEventItem >  EventItemList;
    EventItemList   m_listEventItem;

    CRITICAL_SECTION  m_csLock;

    void    MarkItemAsRead( DWORD consumerId, EventItemList::iterator it );
    void    MarkItemAsRead( DWORD consumerId, NotificationEventItem& item );
};


