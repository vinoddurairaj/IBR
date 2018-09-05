/*
 * errmsg_list.h - error message list object
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
#ifndef __ERRMSG_LIST_H_
#define __ERRMSG_LIST_H_

#include <windows.h>
#include <crtdbg.h>
#include <time.h>

#ifdef __cplusplus

#pragma warning(disable : 4786)
#include <list>     //STL
#include <string>   //STL
/*
 *
 *
 */
class StatusMsg
{
public:
    //default ctr
    StatusMsg();
    //normal ctrs
    StatusMsg(const char* szMessage, int iPriority);
    StatusMsg(const char* szMessage, int iPriority, char cTag, int iTdmfCmd = 0, int iThreadId = (int)GetCurrentThreadId() );
    //copy ctr
    StatusMsg(const StatusMsg & aMsg);
    ~StatusMsg();

    //assignment operator 
    StatusMsg& operator=(const StatusMsg & statusmsg);
    //equality operator
    bool       operator==(StatusMsg & aMsg) const; 
    bool       operator!=(StatusMsg & aMsg) const;


    static char TAG_MESSAGE;
    static char TAG_SUBLIST_BEGIN;
    static char TAG_SUBLIST_END;

    char    getTag()            { return m_cTag; };
    int     getThreadId()       { return m_iThreadId; };
    const char*   getMessage()  { return m_strMessage.c_str(); };
    int     getMessageLength()  { return m_strMessage.length(); };
    char    getPriority()       { return m_cPriority; };
    int     getTimeStamp()      { return m_TimeStamp; };
    int     getTdmfCmd()        { return m_iTdmfCmd; };

    void    setMsgProcessed()   { m_bProcessed = true; };

    bool    isThreadBeginTag(int iThreadId) { return m_cTag==TAG_SUBLIST_BEGIN && m_iThreadId==iThreadId; };
    bool    isThreadMessage(int iThreadId)  { return m_cTag==TAG_MESSAGE && m_iThreadId==iThreadId; };
    bool    isThreadEndTag(int iThreadId)   { return m_cTag==TAG_SUBLIST_END && m_iThreadId==iThreadId; };
    bool    isMsgProcessed()                { return m_bProcessed; };

private:
    std::string     m_strMessage;
    char            m_cPriority;    //can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
    char            m_cTag;         //one of the TAG_... value
    int             m_iThreadId;    //thread id.  useful when m_cIsTag is TAG_SUBLIST_BEGIN or TAG_SUBLIST_END
    time_t          m_TimeStamp; 
    bool            m_bProcessed;   //internal flag
    int             m_iTdmfCmd;
};

typedef std::list<StatusMsg>::iterator  IterStatusMsg;

/*
 *
 *
 */
class StatusMsgList 
{
public:
    StatusMsgList();
    ~StatusMsgList();

    void    Append(const StatusMsg & aStatusMsg);

    //blocks until one or more messages are added to list
    //return true 
    bool    WaitForNewMessages(DWORD timeout = INFINITE);
    //unblock WaitForNewMessages()
    void    SignalNewMsgAvailable();

    std::list<StatusMsg>::iterator  
            FindBeginTag(int iThreadId = (int)GetCurrentThreadId());

    //remove all messages from list belonging to the specified thread between corresponding Start and End tags.
    bool    RemoveMessages(int iThreadId = (int)GetCurrentThreadId());

    //gain exclusive access on list
    void Lock();
    void Unlock();

    std::list<StatusMsg>    m_list;

private:
    HANDLE                  m_hLock;
    HANDLE                  m_hevMsgInList;//signals the presence of new messages in list
    HANDLE                  m_hevDestructionTime;
    StatusMsg               m_lastMsg;//copy of last message inserted in list
};
#endif  //__cplusplus



// utility functions and declarations to be used in errors.c
#ifdef __cplusplus
extern "C" {
#endif

void    error_msg_list_addMessage(int priority, const char* statusmsg);
void    error_msg_list_addMessageThread(int priority, const char *statusmsg, int iThreadId);
void    error_msg_list_addBeginTag(int iThreadId);
void    error_msg_list_addEndTag  (int iThreadId);
int     error_msg_list_getAllThreadMessages(int iThreadId);
void    error_msg_list_markAllThreadMessagesAsProcessed(int iThreadId);
void    error_msg_list_clearAllThreadMessages(int iThreadId);


#ifdef __cplusplus
}
#endif



#endif  //__ERRMSG_LIST_H_
