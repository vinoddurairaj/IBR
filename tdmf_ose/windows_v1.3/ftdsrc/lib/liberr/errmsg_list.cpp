/*
 * errmsg_list.cpp - error message list object
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

#include "errors.h"
#include "errmsg_list.h"


//THE one and only Status msg list instance of a TDMF Agent/Client
StatusMsgList    gStatusMsgList;

/*
 *
 *
 */
char StatusMsg::TAG_MESSAGE         =   (char)0;
char StatusMsg::TAG_SUBLIST_BEGIN   =   (char)11;
char StatusMsg::TAG_SUBLIST_END     =   (char)22;


//******************************************
StatusMsg::StatusMsg()
{
    m_cPriority     = -1;  
    m_cTag          = -1;    
    m_iThreadId     = -1;   
    m_TimeStamp     = -1;
    m_iTdmfCmd      = -1;
    m_bProcessed    = false;
}

//******************************************
StatusMsg::StatusMsg(const char* szMessage, int iPriority)
{
    if ( szMessage != 0 )
        m_strMessage    = szMessage;
    m_cPriority     = (char)iPriority;  //can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
    m_cTag          = TAG_MESSAGE;    
    m_iThreadId     = 0;                //thread id.  useful only when m_cIsTag is TAG_SUBLIST_BEGIN or TAG_SUBLIST_END
    m_TimeStamp     = time(0);
    m_iTdmfCmd      = 0;
    m_bProcessed    = true;            //considered processed when not identified to a precise thread
}

//******************************************
StatusMsg::StatusMsg(const char* szMessage, int iPriority, char cTag, int iTdmfCmd, int iThreadId)
{
    if ( szMessage != 0 )
        m_strMessage = szMessage;
    m_cPriority     = (char)iPriority;//can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
    m_cTag          = cTag;//0 = not a tag msg, 1=Begin sub-list tag, 2=End sub-list tag.
    m_iThreadId     = iThreadId;//thread id.  useful when m_cIsTag is TAG_SUBLIST_BEGIN or TAG_SUBLIST_END
    m_TimeStamp     = time(0);
    m_iTdmfCmd      = iTdmfCmd;
    m_bProcessed    = m_iThreadId == 0;//considered processed if not identified to a precise thread
}

//******************************************
//copy ctr
StatusMsg::StatusMsg(const StatusMsg & aMsg)
{
    m_strMessage    = aMsg.m_strMessage;
    m_cPriority     = aMsg.m_cPriority;//can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
    m_cTag          = aMsg.m_cTag;//0 = not a tag msg, 1=Begin sub-list tag, 2=End sub-list tag.
    m_iThreadId     = aMsg.m_iThreadId;//thread id.  useful when m_cIsTag is TAG_SUBLIST_BEGIN or TAG_SUBLIST_END
    m_TimeStamp     = aMsg.m_TimeStamp;
    m_bProcessed    = aMsg.m_bProcessed;
    m_iTdmfCmd      = aMsg.m_iTdmfCmd;
}

//******************************************
StatusMsg::~StatusMsg()
{   //empty by design
}


//******************************************
//assignment operator 
StatusMsg& StatusMsg::operator=(const StatusMsg & aMsg)
{
    m_strMessage    = aMsg.m_strMessage;
    m_cPriority     = aMsg.m_cPriority;//can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
    m_cTag          = aMsg.m_cTag;//0 = not a tag msg, 1=Begin sub-list tag, 2=End sub-list tag.
    m_iThreadId     = aMsg.m_iThreadId;//thread id.  useful when m_cIsTag is TAG_SUBLIST_BEGIN or TAG_SUBLIST_END
    m_TimeStamp     = aMsg.m_TimeStamp;
    m_bProcessed    = aMsg.m_bProcessed;
    m_iTdmfCmd      = aMsg.m_iTdmfCmd;

    return *this;
}

//*****************************************************************************
//equality operator
bool    StatusMsg::operator==(StatusMsg & aMsg) const 
{
    return  m_strMessage    == aMsg.m_strMessage    &&
            m_cPriority     == aMsg.m_cPriority     &&
            m_cTag          == aMsg.m_cTag          &&
            m_iThreadId     == aMsg.m_iThreadId     &&
            //m_TimeStamp   == aMsg.m_TimeStamp;
            //m_bProcessed  == aMsg.m_bProcessed;
            m_iTdmfCmd      == aMsg.m_iTdmfCmd  ;
}
//*****************************************************************************
//inequality operator
bool    StatusMsg::operator!=(StatusMsg & aMsg) const
{
    return ! (*this == aMsg);
}


//*****************************************************************************
//
//*****************************************************************************
StatusMsgList::StatusMsgList()
{
    m_hLock = CreateMutex(0,0,0);
    m_hevMsgInList = CreateEvent(0,0,0,0);
    m_hevDestructionTime = CreateEvent(0,0,0,0);
}

//******************************************
StatusMsgList::~StatusMsgList()
{   
    SetEvent(m_hevDestructionTime);
    Sleep(1000);
    CloseHandle(m_hLock);
    CloseHandle(m_hevMsgInList);
    CloseHandle(m_hevDestructionTime);
}


void StatusMsgList::Lock()
{
    if ( WAIT_TIMEOUT == WaitForSingleObject(m_hLock,10000) )
    {
        _ASSERT(0);//A TIMEOUT = A BUG 
    }
}

void StatusMsgList::Unlock()
{
    BOOL b = ReleaseMutex(m_hLock); _ASSERT(b);
}

void StatusMsgList::Append(const StatusMsg & aStatusMsg) 
{ 
    //protection against consecutive posting of the same message 
    if ( aStatusMsg != m_lastMsg )
    {
        m_list.push_back(aStatusMsg); 
        SignalNewMsgAvailable();//new msg in list
        m_lastMsg = aStatusMsg;
    }
}

bool StatusMsgList::WaitForNewMessages(DWORD timeout)
{
    HANDLE  handles[2];
    handles[0]  = m_hevMsgInList;
    handles[1]  = m_hevDestructionTime;
    //wait for either event to be signaled
    DWORD status = WaitForMultipleObjects(2,handles,0,timeout);
    return status != WAIT_TIMEOUT;
}

void StatusMsgList::SignalNewMsgAvailable() 
{ 
    SetEvent(m_hevMsgInList);//new msg in list
}

    
/*
 * Look for a status msg with TAG_SUBLIST_BEGIN and a precise thread id in the list.
 *
 */
IterStatusMsg
StatusMsgList::FindBeginTag(int iThreadId)
{
    Lock();
    IterStatusMsg it = m_list.begin();
    while( it != m_list.end() ) 
    {
        if ( (*it).isThreadBeginTag(iThreadId) )
            break;//found it !
        else 
            it++;
    }
    Unlock();
    return it;
}

/*
 * Add a regular Message to the list
 */
void    error_msg_list_addMessage(int priority, const char *statusmsg)
{
    gStatusMsgList.Append( StatusMsg(statusmsg, priority) );
}

void    error_msg_list_addMessageThread(int priority, const char *statusmsg, int iThreadId)
{
    gStatusMsgList.Append( StatusMsg(statusmsg, priority, StatusMsg::TAG_MESSAGE, 0, iThreadId) );
}
/*
 * Add a Start Tag Message to the list
 */
void    error_msg_list_addBeginTag(int iThreadId)
{
    gStatusMsgList.Append( StatusMsg(0, LOG_INFO, StatusMsg::TAG_SUBLIST_BEGIN, 0, iThreadId) );
}
/*
 * Add an End Tag Message to the list
 */
void    error_msg_list_addEndTag(int iThreadId)
{
    gStatusMsgList.Append( StatusMsg(0, LOG_INFO, StatusMsg::TAG_SUBLIST_END, 0, iThreadId) );
}

int     error_msg_list_getAllThreadMessages(int iThreadId)
{
    int iError = 0;
    gStatusMsgList.Lock();

    IterStatusMsg it = gStatusMsgList.FindBeginTag(iThreadId);
    while ( it != gStatusMsgList.m_list.end() && !(*it).isThreadEndTag(iThreadId) )
    {
        if( (*it).isThreadMessage(iThreadId) )
        {
            char p = (*it).getPriority();
            //if message contains LOG_ERR, 
            if (p == LOG_CRIT || p == LOG_ERR)
            {
                iError = 1;
                break;
            }
        }
        it++;
    }

    gStatusMsgList.Unlock();

    return iError;
}

void    error_msg_list_markAllThreadMessagesAsProcessed(int iThreadId)
{
    bool    bFlag = false;

    gStatusMsgList.Lock();

    IterStatusMsg it = gStatusMsgList.FindBeginTag(iThreadId);
    while ( it != gStatusMsgList.m_list.end() )
    {
        if( iThreadId == (*it).getThreadId() )
        {
            (*it).setMsgProcessed();
            bFlag = true;
            if ( StatusMsg::TAG_SUBLIST_END == (*it).getTag() )
            {
                break;
            }
        }
        it++;
    }

    //if at least one msg marked as processed, signal this to Status Thread
    if ( bFlag )
        gStatusMsgList.SignalNewMsgAvailable();

    gStatusMsgList.Unlock();
}

/* ac : not used 
void    error_msg_list_clearAllThreadMessages(int iThreadId)
{
    gStatusMsgList.Lock();

    IterStatusMsg it;
    IterStatusMsg itBegin;
    IterStatusMsg itEnd;

    it = gStatusMsgList.FindBeginTag(iThreadId);
    while ( it != gStatusMsgList.m_list.end() )
    {
        _ASSERT((*it).getThreadId() == iThreadId);

        if( (*it).isThreadBeginTag(iThreadId) )
        {
            itBegin = it;
        }
        else if( (*it).isThreadEndTag(iThreadId) )
        {
            itEnd = it;
            break;
        }
        it++;
    }

    gStatusMsgList.m_list.erase(itBegin,++itEnd);


    gStatusMsgList.Unlock();
}
*/
