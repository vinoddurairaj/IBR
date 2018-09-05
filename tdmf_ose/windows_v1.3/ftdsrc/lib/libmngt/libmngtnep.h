/*
 * libmngtnep.h - TDMF Management Notification Event Protocol
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
#ifndef _LIBMNGTNEP_H_
#define _LIBMNGTNEP_H_

#pragma warning(disable : 4786) //get rid of annoying STL warning
#include <list>
/*extern "C" 
{
#include "sock.h"
}
*/
#ifndef _SOCK_H
struct sock_t;
#endif

// ***********************************************************
#pragma pack(push, 1)    //ensure 1 byte packing on structures exchanged via socket communication

#ifdef __AFX_H__
#include <afx.h>    //MFC in use
#else
#include <windows.h>
#endif
#ifndef _FTD_PERF_H_
//#include "ftd_perf.h"
#include "libmngtdef.h"
#endif

//header for many notification data messages
typedef struct {
  	unsigned int    iHostID;		//numerical HostID value identifying the TDMF Agent.
    time_t          tStimeTamp;     
} mnep_NotifMsgDataAgentUID;

//data portion of NOTIF_MSG_SERVER_ACTIVITY msg 
typedef struct {
  	mnep_NotifMsgDataAgentUID    agentUID;		//HostID identifying the TDMF Agent.

	int nBABAllocated;
	int nBABRequested;
} mnep_NotifMsgDataServerInfo;

//data portion of NOTIF_MSG_RG_STATUS_AND_MODE msg 
typedef struct {
  	mnep_NotifMsgDataAgentUID    agentUID;		//HostID identifying the TDMF Agent.

    short           sRepGroupNumber;//normally : 0 to 500
    short           sConnection;    /* 0 = pmd only, 1 = connected, -1 = accumulate */
    short           sState;         /* 
                                    #define FTD_M_JNLUPDATE     0x01
                                    #define FTD_M_BITUPDATE     0x02
                                    #define FTD_MODE_PASSTHRU   0x10
                                    #define FTD_MODE_NORMAL     FTD_M_JNLUPDATE
                                    #define FTD_MODE_TRACKING   FTD_M_BITUPDATE
                                    #define FTD_MODE_REFRESH    (FTD_M_JNLUPDATE | FTD_M_BITUPDATE)
                                    #define FTD_MODE_BACKFRESH  0x20
									//*** new define reserved for State (group Started or Stopped) ***
                                    #define FTD_STATE_START  	    0x100	//bit set = group started.
                                    #define FTD_STATE_CHECKPOINT 	0x200	//bit set = group in checkpoint
                                    */
    short           sPctDone;       //percent done of refresh,backfresh and other operations

    //PStore and Journal related values
    __int64         liPStoreDiskTotalSz,
                    liPStoreDiskFreeSz,
                    liPStoreSz;
    __int64         liJournalDiskTotalSz,
                    liJournalDiskFreeSz,
                    liJournalSz;

    char            notused[124];   /* allows code compatibility with future upgrade */

} mnep_NotifMsgDataRepGrpStatusMode;

//data portion of NOTIF_MSG_RG_RAW_PERF_DATA msg 
typedef struct {
  	mnep_NotifMsgDataAgentUID    agentUID;		//HostID identifying the TDMF Agent.

    ftd_perf_instance_t     perf;

} mnep_NotifMsgDataRepGrpRawPerf;


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

#pragma pack(pop)    
// ***********************************************************

// ***********************************************************
enum NotificationMsgTypes
{   //all enum . members must have contiguous values and be in the range ] FIRST , LAST [
     NOTIF_MSG_TYPE_INVALID = 5000
    ,NOTIF_MSG_TYPE_FIRST   = 5001

    ,NOTIF_MSG_SUBSCRIPTION                 //msg data = see TDMFSubscriptionMessage

    ,NOTIF_MSG_RG_RAW_PERF_DATA             //msg data = NotifMsgDataRepGrpRawPerf
    ,NOTIF_MSG_RG_STATUS_AND_MODE           //msg data = NotifMsgDataRepGrpStatusMode                    

    ,NOTIF_MSG_SERVER_ACTIVITY              //msg data = NotifMsgDataAgentUID
    ,NOTIF_MSG_SERVER_NO_ACTIVITY           //msg data = NotifMsgDataAgentUID
    ,NOTIF_MSG_SERVER_COMMUNICATION_PROBLEM //msg data = NotifMsgDataAgentUID

    ,NOTIF_MSG_COLLECTOR_COMMUNICATION_PROBLEM  //no data
    ,NOTIF_MSG_COLLECTOR_UNABLE_TO_CONNECT      //no data.  cannot establish socket connection with Collector
    ,NOTIF_MSG_COLLECTOR_CONNECTED              //no data.  socket connection established with Collector

    ,NOTIF_MSG_GUI_MSG
    ,NOTIF_MSG_COLLECTOR_STAT				    //msg data = TdmfCollectorState struct  RDDEV030328
    ,NOTIF_MSG_TYPE_LAST    
};

#define NOTIFICATION_MSG_HDR_MAGIC_NUMBER   0xfaceface
/*
 * This class encapsulates the logic to build, send and/or receive a TDMF Notification Message.
 * It enforces the TDMF Notification protocol.
 */
class TDMFNotificationMessage
{
public:
    TDMFNotificationMessage(enum NotificationMsgTypes eType = NOTIF_MSG_TYPE_INVALID, int iLength = 0, const char * pData = 0);
    TDMFNotificationMessage(const TDMFNotificationMessage & objToCopy);
    ~TDMFNotificationMessage();

    TDMFNotificationMessage& operator=(const TDMFNotificationMessage & objToCopy);
    bool                     operator==(const TDMFNotificationMessage & objToTest) const;

    inline enum NotificationMsgTypes    getType() const;
    inline int          getLength() const;
    inline const char*  getData() const;

    inline void     setType   (enum NotificationMsgTypes iType);
    void            setData   (int iLength, const char* pData);
    void            setDataExt(int iLength, const char* pData);

    bool            IsValid();
    bool            isKnownType();//check if m_eType is known by this object version
    bool            isKnownType(enum NotificationMsgTypes eType);//check if eType is known object version
    bool            isKnownType(int iType);//check if iType is known object version

    bool            Send( sock_t * s );
    bool            Recv( sock_t * s, int inactivityTimeout /*millisecs*/ );

protected:
    void            assign(const TDMFNotificationMessage & objToCopy);
    void            NtoH();

    enum NotificationMsgTypes   m_eType;  //Notification Message Type
    unsigned int     m_iLength;//Notification Message Data Length
    char    *m_pData; //Notification Message Data 
    bool    bExternalDataBuffer;
};

inline enum NotificationMsgTypes     
TDMFNotificationMessage::getType()   const 
{ 
    return m_eType;   
};

inline int     
TDMFNotificationMessage::getLength() const 
{ 
    return m_iLength; 
};

inline const char*   
TDMFNotificationMessage::getData()   const 
{ 
    return m_pData;   
};

inline void    
TDMFNotificationMessage::setType(enum NotificationMsgTypes eType)          
{ 
    m_eType = eType;    
};


//*****************************************************************************
//*****************************************************************************
//#include "TDMFNotificationSubscriptionMessage.h" //for various enum values


//*****************************************************************************
//*****************************************************************************
void mnep_convert_hton(mnep_NotifMsgDataRepGrpStatusMode *    data);
void mnep_convert_hton(mnep_NotifMsgDataAgentUID *            data);
void mnep_convert_hton(mnep_NotifMsgDataRepGrpRawPerf *       data);


void mnep_convert_ntoh(mnep_NotifMsgDataRepGrpStatusMode *    data);
void mnep_convert_ntoh(mnep_NotifMsgDataAgentUID *            data);
void mnep_convert_ntoh(mnep_NotifMsgDataRepGrpRawPerf *       data);
void mnep_convert_ntoh(mnep_NotifMsgDataServerInfo*           data);


#endif  //_LIBMNGTNEP_H_
