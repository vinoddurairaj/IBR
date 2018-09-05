/*
 * libmngtmsg.h -   declarations of messages exchanged between the TDMF clients applications,
 *                  (libmngt.lib library), the TDMF Collector and TDMF Agents.
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
#ifndef __LIBMNGTMSG_H_
#define __LIBMNGTMSG_H_

#include "libmngtdef.h"

/*
 * MMP_PROTOCOL_VERSION value is used to identify a precise version of the 
 * management protocol. The Collector will detect a version difference 
 * with any Replication Server (refer to Alive socket code).  
 * When this occurs, it report this state to its log file file and 
 * can also send a TERMINATE msg to the Repl. Server. when required.
 * 
 */

#define MMP_PROTOCOL_VERSION    4   
// v4 , 2003-05-28 : modif. to MMP_MNGT_TDMF_SENDFILE, _SENDFILE_STATUS and _GETFILE (...)
// v4 , 2003-04-17 : modif. to MMP_MNGT_COLLECTOR_STATE (...)
// v3 , 2002-11-15 : modif. to MMP_MNGT_AGENT_INFO msg (mmp_TdmfServerInfo structure)
// v2 , 2002-10-28 : modif. to MMP_MNGT_REGISTRATION_KEY msg
    

/*
 * GUI_COLLECTOR_PROTOCOL_VERSION value is used to identify a precise version of the 
 * gui collector protocol.
 */

#define GUI_COLLECTOR_PROTOCOL_VERSION    2
// v2 , 2003-06-26 : modif. to mmp_TdmfCollectorParams to add TImeOut attributs

    
enum management  // UPDATE  gszMMPMsgTypeText[] strings when adding/modifying PROTOCOL
{ 
/* management msg types */  /* 0x1000 = 4096 */
 MMP_MNGT_SET_LG_CONFIG      = 0x1000   // receiving one logical group configuration data (.cfg file)
,MMP_MNGT_SET_CONFIG_STATUS             // receiving configuration data
,MMP_MNGT_GET_LG_CONFIG                 // request to send one logical group configuration data (.cfg file)
,MMP_MNGT_AGENT_INFO_REQUEST            // request for host information: IP, listener socket port, ...
,MMP_MNGT_AGENT_INFO                    // TDMF Agent response to a MMP_MNGT_AGENT_INFO_REQUEST message 
                                        // OR
                                        // SET TDMF Agent general information (Server info)
,MMP_MNGT_REGISTRATION_KEY              // SET/GET registration key
,MMP_MNGT_TDMF_CMD                      // management cmd, specified by a tdmf_commands sub-cmd
,MMP_MNGT_TDMF_CMD_STATUS               // response to management cmd
,MMP_MNGT_SET_AGENT_GEN_CONFIG          // SET TDMF Agent general config. parameters
,MMP_MNGT_GET_AGENT_GEN_CONFIG          // GET TDMF Agent general config. parameters
,MMP_MNGT_GET_ALL_DEVICES               // GET list of devices (disks/volumes) present on TDMF Agent system
,MMP_MNGT_SET_ALL_DEVICES               // response to a MMP_MNGT_GET_ALL_DEVICES request message.
,MMP_MNGT_ALERT_DATA                    // TDMF Agent sends this message to TDMF Collector to report an alert condition
,MMP_MNGT_STATUS_MSG
,MMP_MNGT_PERF_MSG                      // TDMF Agent periodically sends performance data to TDMF Collector
,MMP_MNGT_PERF_CFG_MSG                  // various performance counters and pacing values
,MMP_MNGT_MONITORING_DATA_REGISTRATION  // Request the TDMF Collector to be notified for any TDMF Agents status/mode/performance data changes.
,MMP_MNGT_AGENT_ALIVE_SOCKET            // Agent opens a socket on Collector and sends this msg.  Both peers keep socket connected at all times.  Used by Collector for Agent detection.
,MMP_MNGT_AGENT_STATE                   // Agent sends this msg to report about its various states to Collector.
,MMP_MNGT_GROUP_STATE                   // TDMF Agent periodically sends group states to TDMF Collector
,MMP_MNGT_TDMFCOMMONGUI_REGISTRATION    // TDMF Common GUI sends this msg periodically as a watchdog to keep or claim ownership over the Collector and TDMF DB.
,MMP_MNGT_GROUP_MONITORING              // TDMF Agent sends various rela-time group information to Collector
,MMP_MNGT_SET_DB_PARAMS                 // TDMF Common GUI sends this msg to set various parameters related to the Tdmf DB
,MMP_MNGT_GET_DB_PARAMS                 // TDMF Common GUI sends this msg to set various parameters related to the Tdmf DB
,MMP_MNGT_AGENT_TERMINATE               // TDMF Collector requests that an Repl. Server Agent stops running.

,MMP_MNGT_GUI_MSG                       // Inter-GUI message
,MMP_MNGT_COLLECTOR_STATE               // TDMF collector sends this msg to GUI to report its state
,MMP_MNGT_TDMF_SENDFILE                 // receiving a TDMF files (script or zip)
,MMP_MNGT_TDMF_SENDFILE_STATUS          // receiving TDMF files (script or zip)
,MMP_MNGT_TDMF_GETFILE                  // request to send TDMF files (tdmf*.bat)
};

#define TDMF_COLLECTOR_DEF_PORT     576
#define TDMF_BROADCAST_PORT         10112
#define MNGT_MSG_MAGICNUMBER        0xe8ad4239
#define SENDERTYPE_TDMF_AGENT       2
#define SENDERTYPE_TDMF_SERVER      0
#define SENDERTYPE_TDMF_CLIENTAPP   1
#define INVALID_HOST_ID             0

enum mngt_status
{
 MMP_MNGT_STATUS_OK                          =  0    // success
,MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR   =  100  // could not connect to TDMF Collector socket
,MMP_MNGT_STATUS_ERR_UNKNOWN_TDMFAGENT               // specified TDMF Agent (szServerUID) is unknown to TDMF Collector
,MMP_MNGT_STATUS_ERR_CONNECT_TDMFAGENT               // could not connect to TDMF Agent.
,MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT              // unexpected communication rupture while rx/tx data with TDMF Agent
,MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR          // unexpected communication rupture while rx/tx data with TDMF Collector

,MMP_MNGT_STATUS_ERR_SET_AGENT_GEN_CONFIG            // Agent could not save the received Agent config.  Continuing using current config.
,MMP_MNGT_STATUS_ERR_INVALID_SET_AGENT_GEN_CONFIG    // provided Agent config. contains illegal values and is rejected by Agent.  Continuing using current config.

,MMP_MNGT_STATUS_ERR_COLLECTOR_INTERNAL_ERROR        // basic functionality failed ...

,MMP_MNGT_STATUS_ERR_BAD_OR_MISSING_REGISTRATION_KEY 

,MMP_MNGT_STATUS_ERR_ERROR_ACCESSING_TDMF_DB         //109
};

//*****************************************************************************
// Message Structures definitions
//*****************************************************************************
#pragma pack(push, 1)    


/* MMP management header message structure */
/* this is the management header used in all mngt messages */
typedef struct mmp_mngt_header_s {
    int magicnumber;                /* indicates the following is a management message */
    int mngttype;                   /* enum management, the management msg type. */

    int sendertype;                 /* indicates this message sender : TDMF_AGENT, TDMF_SERVER, CLIENT_APP */
                                    /* only TDMF Collector checks this flag */

    int mngtstatus;                 /* enum mngt_status.
                                       this field is used to return a response status.  
                                       used when the message is a response to a mngt request. */
                                        
} mmp_mngt_header_t;


//Broadcast request message, broadcasted by TDMF Collector to TDMF Agents
//or
//send by a TDMF client application to TDMF Collector to request a DB refresh of TDMF Agents info.
typedef struct mmp_mngt_BroadcastReq_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfDBServerInfo    data; 
} mmp_mngt_BroadcastReqMsg_t;

//MMP_MNGT_SET_AGENT_GEN_CONFIG
//MMP_MNGT_GET_AGENT_GEN_CONFIG
//  Sent by TDMF Collector to one TDMF Agent to force general TDMF parameters

//MMP_MNGT_AGENT_INFO
//  Sent by TDMF Agent to TDMF Collector to provide TDMF parameters
typedef struct mmp_mngt_TdmfAgentConfigMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfServerInfo      data; 
} mmp_mngt_TdmfAgentConfigMsg_t ;


//MMP_MNGT_GET_ALL_DEVICES
typedef struct mmp_mngt_TdmfAgentDevicesMsg_s {
    mmp_mngt_header_t       hdr;
    char                    szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];            //name of machine identifying the TDMF Agent.
    int                     iNbrDevices;                //number of mmp_TdmfDeviceInfo contiguous to this structure
} mmp_mngt_TdmfAgentDevicesMsg_t ;


//MMP_MNGT_REGISTRATION_KEY 
//When sender provides a non-empty keydata.szRegKey , it is a SET key message. 
//When sender provides an empty keydata.szRegKey ("" or "\0"), it is a GET key message. 
//For a SET key, the same message is used as response. Upon success, the szRegKey member 
//will contain the same key value.  On failure the szRegKey member returned will be an empty string.
//For a GET key, the same message is used as response. Upon success, the szRegKey member 
//will contain the key value.  On failure the szRegKey member returned will be an empty string.
typedef struct mmp_mngt_RegistrationKey_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfRegistrationKey keydata;
} mmp_mngt_RegistrationKeyMsg_t;

//MMP_MNGT_GET_LG_CONFIG 
//request a TDMF Agent for an existing logical group configuration file ( .cfg ).
//MMP_MNGT_SET_LG_CONFIG 
//request a TDMF Agent to use the supplied logical group configuration file ( .cfg ).
//
//Name of cfg file identifies the logical group.
//
//MMP_MNGT_GET_LG_CONFIG : 
//  Name of the requested file must be provided within the 'data' structure.  
//  The 'data.uiFileSize' member MUST be set to 0.
//  Other 'data' members are ignored.
//MMP_MNGT_SET_LG_CONFIG : 
//  All members of the 'data' structure must be filled properly.
//  This message will serve as response to a MMP_MNGT_GET_LG_CONFIG request.
typedef struct mmp_mngt_ConfigurationMsg_s {
    mmp_mngt_header_t           hdr;
    char                        szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];        //name of machine identifying the TDMF Agent.
    mmp_TdmfFileTransferData    data;                   //cfg file is transfered 
} mmp_mngt_ConfigurationMsg_t;

//sent in response to a MMP_MNGT_SET_LG_CONFIG request.
//indicates if configuration was updated succesfully
typedef struct mmp_mngt_ConfigurationStatusMsg_s {
    mmp_mngt_header_t           hdr;                    
    char                        szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];        //name of machine identifying the TDMF Agent.
    unsigned short              usLgId;                 //logical group for which the config. set was requested
    int                         iStatus;                //0 = OK, 1 = rx/tx error with TDMF Agent, 2 = err writing cfg file
} mmp_mngt_ConfigurationStatusMsg_t;

//MMP_MNGT_TDMF_SENDFILE : 
//  All members of the 'data' structure must be filled properly.
typedef struct mmp_mngt_FileMsg_s {
    mmp_mngt_header_t           hdr;
    char                        szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];        //name of machine identifying the TDMF Agent.
    mmp_TdmfFileTransferData    data;                   //TDMF file is transfered 
} mmp_mngt_FileMsg_t;

//sent in response to a MMP_MNGT_TDMF_SENDFILE request.
//indicates if TDMF script was updated succesfully with a MMP_MNGT_TDMF_SENDFILE_STATUS
typedef struct mmp_mngt_FileStatusMsg_s {
    mmp_mngt_header_t           hdr;                    
    char                        szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];        //name of machine identifying the TDMF Agent.
    unsigned short              usLgId;                 //logical group for which the config. set was requested
    int                         iStatus;                //0 = OK, 1 = rx/tx error with TDMF Agent, 2 = err writing cfg file
} mmp_mngt_FileStatusMsg_t;

//MMP_MNGT_TDMF_CMD 
//ask a TDMF Agent to perform a specific action
typedef struct mmp_mngt_TdmfCommandMsg_s {
    mmp_mngt_header_t       hdr;
    char                    szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];        //name of machine identifying the TDMF Agent.
    int                     iSubCmd;                //value from enum tdmf_commands
    char                    szCmdOptions[ 256 ];     //list of options for sub-cmd, similar to a Tdmf exe cmd line
} mmp_mngt_TdmfCommandMsg_t;

//MMP_MNGT_TDMF_CMD_STATUS, 
//message sent by a TDMF Agent in response to a MMP_MNGT_TDMF_CMD request
typedef struct mmp_mngt_TdmfCommandMsgStatus_s {
    mmp_mngt_header_t       hdr;
    char                    szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];        //name of machine identifying the TDMF Agent.
    int                     iSubCmd;                //value from enum tdmf_commands
    int                     iStatus;                //value from enum tdmf_commands_status
    int                     iLength;                //length of string (including \0) following this structure
                                                    //the string is the console output of the tdmf command tool performing the command.
} mmp_mngt_TdmfCommandStatusMsg_t;


//MMP_MNGT_ALERT_DATA
//  Sent by TDMF Agents to TDMF Collector.  Collector saves the data to the TDMF DB.
//
typedef struct mmp_mngt_TdmfAlertMsg_s {
    mmp_mngt_header_t       hdr;
    int                     iLength;                //nbr of bytes following this structures.  used for proper reception on socket.
    mmp_TdmfAlertHdr        data;
    //two zero-terminated strings follow the mmp_TdmfAlertHdr structure:
    //the first is a string identifying the Module generating the message.
    //the second is a string containing the text message 
} mmp_mngt_TdmfAlertMsg_t;

/*
//MMP_MNGT_MONITORING_DATA
//  Sent by TDMF Agents to TDMF Collector.  Collector saves the data to the TDMF DB.
//
typedef struct mmp_mngt_TdmfMonitoringMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfMonitoringData  data;
} mmp_mngt_TdmfMonitoringMsg_t;
*/

//MMP_MNGT_STATUS_MSG
//  Sent by TDMF Agents to TDMF Collector.  Collector saves the messages to the TDMF DB.
typedef struct mmp_mngt_TdmfStatusMsgMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfStatusMsg       data;
} mmp_mngt_TdmfStatusMsgMsg_t;

//MMP_MNGT_PERF_MSG
//  Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
typedef struct mmp_mngt_TdmfPerfMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfPerfData        data;
    //this message structure is followed by a variable number of ftd_perf_instance_t structures
    //refer to mmp_TdmfPerfData members.
} mmp_mngt_TdmfPerfMsg_t;

//MMP_MNGT_GROUP_STATE
//  Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
typedef struct mmp_mngt_TdmfGroupStateMsg_s {
    mmp_mngt_header_t       hdr;
    char                    szServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];  //name of machine identifying the TDMF Agent.
    //this message structure is followed by a variable number of mmp_TdmfGroupState structures
    //the last mmp_TdmfGroupState structure will show an invalid group number value.
} mmp_mngt_TdmfGroupStateMsg_t;



//MMP_MNGT_PERF_CFG_MSG
//  Sent by TDMF Collector to all TDMF Agents.  
//  Collector saves into the TDMF DB NVP table and sends to all Agents in DB
typedef struct mmp_mngt_TdmfPerfCfgMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfPerfConfig      data;
} mmp_mngt_TdmfPerfCfgMsg_t;


//MMP_MNGT_MONITORING_DATA_REGISTRATION
//  Sent by an application to TDMF Collector
//  Registers the application to receive all TDMF Agents monitoring data (status/mode/performance)
typedef struct mmp_mngt_TdmfMonitorRegistrationMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfMonitoringCfg   data;
} mmp_mngt_TdmfMonitorRegistrationMsg_t;

//MMP_MNGT_AGENT_ALIVE_SOCKET
//  Sent once by a TDMF Agent to TDMF Collector
typedef struct mmp_mngt_TdmfAgentAliveMsg_s {
    mmp_mngt_header_t       hdr;
    char                    szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ]; //unique Host ID (in hexadecimal text format) of the TDMF Agent.
    int                     iMsgLength; //number of bytes following this message.  
    //zero-terminated text string containing tags-values pairs can be contiguous to this message.
} mmp_mngt_TdmfAgentAliveMsg_t;
//possible tags included in variable data portion of MMP_MNGT_AGENT_ALIVE_SOCKET msg
#define MMP_MNGT_ALIVE_TAG_MMPVER   "MMPVER="
#define MMP_MNGT_ALIVE_TIMEOUT      10  //max period of Alive msg (in seconds)


#define MMP_MNGT_MAX_CLIENT_UID_SZ  16
//MMP_MNGT_TDMFCOMMONGUI_REGISTRATION
//  Sent by a TDMF common GUI apps to TDMF Collector
typedef struct mmp_mngt_TdmfCommonGuiRegistrationMsg_s {
    mmp_mngt_header_t       hdr;
    char                    szClientUID[ MMP_MNGT_MAX_CLIENT_UID_SZ ]; //unique Client ID : uniquely identify a client among tdmf gui on same machine
    char                    bRequestOwnership;  //0 = release ownership , else request / renew ownership.
    char                    notused[3];  //to align structure
} mmp_mngt_TdmfCommonGuiRegistrationMsg_t;


//MMP_MNGT_GROUP_MONITORING
//  Sent by a TDMF Agent to TDMF Collector
typedef struct mmp_mngt_TdmfReplGroupMonitorMsg_s {
    mmp_mngt_header_t           hdr;
    mmp_TdmfReplGroupMonitor    data;
} mmp_mngt_TdmfReplGroupMonitorMsg_t;

//MMP_MNGT_SET_DB_PARAMS
//MMP_MNGT_GET_DB_PARAMS
//  Sent by a TDMF common GUI apps to TDMF Collector
typedef struct mmp_mngt_TdmfCollectorParamsMsg_s {
    mmp_mngt_header_t           hdr;
    mmp_TdmfCollectorParams     data;
} mmp_mngt_TdmfCollectorParamsMsg_t;

//MMP_MNGT_AGENT_STATE
typedef struct mmp_mngt_TdmfAgentStateMsg_s {
    mmp_mngt_header_t           hdr;
    mmp_TdmfAgentState          data;
} mmp_mngt_TdmfAgentStateMsg_t;

//MMP_MNGT_GUI_MSG
typedef struct mmp_mngt_TdmfGuiMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfGuiMsg          data;
} mmp_mngt_TdmfGuiMsg_t;

//MMP_MNGT_COLLECTOR_STATE
typedef struct mmp_mngt_TdmfCollectorStateMsg_s {
    mmp_mngt_header_t           hdr;
    mmp_TdmfCollectorState      data;
} mmp_mngt_TdmfCollectorState;

#pragma pack(pop)    
//*****************************************************************************
// utility functions.  not intended to be used outside TDMF product
//*****************************************************************************
//#ifdef __cplusplus
//extern "C"  {
//#endif
////////////////////////////////////////////
//convert fields BEFORE sending on socket
////////////////////////////////////////////
void    mmp_convert_mngt_hdr_hton(mmp_mngt_header_t *hdr);
//void    mmp_convert_TdmfLGConfiguration_hton(mmp_TdmfLGCfgHdr *lgcfghdr);
void    mmp_convert_TdmfServerInfo_hton(mmp_TdmfServerInfo *srvrInfo);
//void    mmp_convert_TdmfAgentConfig_hton(mmp_TdmfAgentConfig *agentConfig);
void    mmp_convert_TdmfAgentDeviceInfo_hton(mmp_TdmfDeviceInfo *devInfo);
void    mmp_convert_TdmfAlert_hton(mmp_TdmfAlertHdr *alertData);
//void    mmp_convert_TdmfMonitoring_hton(mmp_TdmfMonitoringData *monitoringData);
void    mmp_convert_TdmfStatusMsg_hton(mmp_TdmfStatusMsg *statusMsg);
void    mmp_convert_TdmfPerfData_hton(mmp_TdmfPerfData *perfData);
void    mmp_convert_TdmfPerfConfig_hton(mmp_TdmfPerfConfig *perfCfg);
void    mmp_convert_hton(mmp_TdmfPerfConfig *perfCfg);
//void    mmp_convert_hton(mmp_TdmfMonitoringDataHdr *pData);
#ifdef  _FTD_PERF_H_
void    mmp_convert_hton(ftd_perf_instance_t *perf);
#endif//_FTD_PERF_H_
//void    mmp_convert_hton(mmp_TdmfMonitoringDataRepGroupStatusAndMode *pData);
__int64     htonI64( __int64 int64Val );
void    mmp_convert_hton(mmp_TdmfGroupState *grpstate);
void    mmp_convert_hton(mmp_TdmfReplGroupMonitor *grpmonit);
void    mmp_convert_hton(mmp_TdmfCollectorParams *dbparams);
void    mmp_convert_hton(mmp_TdmfAgentState *agentState);
void    mmp_convert_hton(mmp_TdmfFileTransferData *data);


////////////////////////////////////////////
//convert fields AFTER reception from socket
////////////////////////////////////////////
void    mmp_convert_mngt_hdr_ntoh(mmp_mngt_header_t *hdr);
//void    mmp_convert_TdmfLGConfiguration_ntoh(mmp_TdmfLGCfgHdr *lgcfghdr);
void    mmp_convert_TdmfServerInfo_ntoh(mmp_TdmfServerInfo *srvrInfo);
//void    mmp_convert_TdmfAgentConfig_ntoh(mmp_TdmfAgentConfig *agentConfig);
void    mmp_convert_TdmfAgentDeviceInfo_ntoh(mmp_TdmfDeviceInfo *devInfo);
void    mmp_convert_TdmfAlert_ntoh(mmp_TdmfAlertHdr *alertData);
//void    mmp_convert_TdmfMonitoring_ntoh(mmp_TdmfMonitoringData *monitoringData);
void    mmp_convert_TdmfStatusMsg_ntoh(mmp_TdmfStatusMsg *statusMsg);
void    mmp_convert_TdmfPerfData_ntoh(mmp_TdmfPerfData *perfData);
void    mmp_convert_TdmfPerfConfig_ntoh(mmp_TdmfPerfConfig *perfCfg);
void    mmp_convert_ntoh(mmp_TdmfPerfConfig *perfCfg);
//void    mmp_convert_ntoh(mmp_TdmfMonitoringDataHdr *pData);
#ifdef  _FTD_PERF_H_
void    mmp_convert_ntoh(ftd_perf_instance_t *perf);
#endif//_FTD_PERF_H_
//void    mmp_convert_ntoh(mmp_TdmfMonitoringDataRepGroupStatusAndMode *pData);
__int64     ntohI64( __int64 int64Val );
void    mmp_convert_ntoh(mmp_TdmfGroupState *grpstate);
void    mmp_convert_ntoh(mmp_TdmfReplGroupMonitor *grpmonit);
void    mmp_convert_ntoh(mmp_TdmfCollectorParams *dbparams);
void    mmp_convert_ntoh(mmp_TdmfAgentState *agentState);
void    mmp_convert_ntoh(mmp_TdmfFileTransferData *data);


int     mmp_sendmsg(unsigned long rip, unsigned long rport, const char* senddata, int sendlen, char* rcvdata, int rcvsize);

#ifdef _SOCK_H 
int     mmp_mngt_sock_send(sock_t *s, char *buf, int len);
int     mmp_mngt_sock_recv(sock_t *s, char *buf, int len, int timeout = 30 ); // ardev 030110
void    mmp_mngt_recv_cfg_data(sock_t *s, /*in*/mmp_mngt_header_t *msghdr, /*out*/mmp_mngt_ConfigurationMsg_t ** ppRcvCfgData);
void    mmp_mngt_recv_file_data(sock_t *s, /*in*/mmp_mngt_header_t *msghdr, /*out*/mmp_mngt_FileMsg_t ** ppRcvFileData);
int     mmp_mngt_request_alldevices( int rip, int rport, 
                                     sock_t *request,
                                     int sendertype, //SENDERTYPE_TDMF_....
                                     const char * szAgentId,
                                     bool         bIsHostId,
                                    /*out*/mmp_mngt_TdmfAgentDevicesMsg_t **ppTdmfDevicesMsg );
void    mmp_mngt_recv_alldevices_data(sock_t *s, mmp_mngt_header_t *hdr, mmp_mngt_TdmfAgentDevicesMsg_t **ppRcvDevMsg);
#endif
//to release memory allocated by a succesful call to mmp_mngt_recv_cfg_data()
void    mmp_mngt_free_cfg_data_mem(/*in*/mmp_mngt_ConfigurationMsg_t ** ppRcvCfgData);

//to release memory allocated by a succesful call to mmp_mngt_recv_script_data()
void    mmp_mngt_free_file_data_mem(/*in*/mmp_mngt_FileMsg_t ** ppRcvFileData);

//to release memory allocated by a succesful call to mmp_mngt_recv_cfg_data()
void    mmp_mngt_free_alldevices_data(mmp_mngt_TdmfAgentDevicesMsg_t **ppRcvDevMsg);

void    mmp_mngt_build_SetConfigurationMsg(mmp_mngt_ConfigurationMsg_t **ppCfgMsg, 
                                           const char   *szAgentId, 
                                           bool         bIsHostId,
                                           int          iSenderType,  
                                           const char   *szCfgFileName, 
                                           const char   *pFileData, 
                                           unsigned int uiDataSize );

void    mmp_mngt_release_SetConfigurationMsg(mmp_mngt_ConfigurationMsg_t **ppCfgMsg);


void    mmp_mngt_build_SetTdmfFileMsg(mmp_mngt_FileMsg_t **ppFileMsg, 
								      const char   *szAgentId, 
                                      bool         bIsHostId,
                                      int          iSenderType,  
                                      const char   *szTdmfFileName, 
                                      const char   *pFileData, 
                                      unsigned int uiDataSize,
									  int          iFileType);

void    mmp_mngt_release_SetTdmfFileMsg(mmp_mngt_FileMsg_t **ppFileMsg);

const char*   mmp_mngt_getMsgTypeText(int iMMPType);

//#ifdef __cplusplus
//}
//#endif



#endif //__LIBMNGTMSG_H_