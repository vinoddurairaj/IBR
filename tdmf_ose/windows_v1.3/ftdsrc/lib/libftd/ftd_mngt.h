/*
 * ftd_mngt.h - FTD management messages 
 * 
 * Copyright (c) 2002 Fujitsu SotfTek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef _FTD_MNGT_H
#define _FTD_MNGT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ftd_sock.h"
#include "..\..\lib\libmngt\libmngtdef.h"

//instance of errfac_t * used by several FTD MNGT threads
extern errfac_t * gMngtErrfac;

/* external prototypes */

//ftd_mngt.cpp
void    ftd_mngt_initialize();
void    ftd_mngt_shutdown();
int     ftd_mngt_recv_msg(ftd_sock_t *fsockp);
int     ftd_mngt_send_agentinfo_msg(int rip, int rport);
void    ftd_mngt_send_alert_msg(mmp_TdmfAlertHdr *alertData, char* szModuleName, char* szAlertMessage);
char*   ftd_mngt_getServerId(/*out*/ char *szHostId );
void    ftd_mngt_GetPerfConfig(mmp_TdmfPerfConfig *perfCfg);
void    ftd_mngt_GetCollectorInfo(int *collectorIP, int *collectorPort);
int     ftd_mngt_get_perf_upload_period();
time_t  ftd_mngt_get_repl_grp_monit_upload_period();
void    ftd_mngt_initialize_collector_values(void);
void    SendAllPendingMessagesToCollector(void);


void ftd_mngt_msgs_send_status_msg_to_collector(const char *szMessage, int iTimestamp);


int     ftd_mngt_get_local_ip_for_collector_routing(void);
int		ftd_mngt_IfCollectorPresent();


#ifdef _FTD_MNGT_LG_H_
#ifdef __cplusplus
int     ftd_mngt_send_lg_monit(int lgnum, bool isPrimary, ftd_mngt_lg_monit_t* monitp);
#endif
#endif

void    ftd_mngt_UpdateAliveMsgTime(void);

//ftd_mngt_perf.cpp
int     ftd_mngt_performance_init();//return 0 = error, otherwise ok
void    ftd_mngt_performance_end();
#ifdef _FTD_PERF_H_
void    ftd_mngt_performance_send_data(const ftd_perf_instance_t *pDeviceInstanceData, int iNPerfData);
#endif
void    ftd_mngt_performance_set_group_cp(short sGrpNumber, int IsCheckpoint /*0 or 1*/);
void    ftd_mngt_performance_send_all_to_Collector();
void    ftd_mngt_performance_force_acq();
HANDLE  ftd_mngt_performance_get_force_acq_event();
int     ftd_mngt_performance_get_wakeup_period();
//request that the perf upload period be 1 second for the next N seconds
//after that period, it falls back to the configured gTdmfPerfConfig.iPerfUploadPeriod.
void    ftd_mngt_performance_reduce_stat_upload_period(int iFastPerfPeriodSeconds, int bWakeUpStatThreadNow);


void    ftd_mngt_sys_initialize();
int     ftd_mngt_sys_need_to_restart_system();//0 = no need to restart, otherwise need to restart TDMF Agent FTD Master Thread
//called when critical values like BAB size, TCP Port or TCP Window size have been modified
void    ftd_mngt_sys_request_system_restart();
void    ftd_mngt_sys_restart_system();//initiate a system shutdown and restart.

int     ftd_mngt_set_config(sock_t *sockp);
int     ftd_mngt_get_config(sock_t *sockp);
int     ftd_mngt_set_file(sock_t *sockp);
int     ftd_mngt_get_file(sock_t *sockp);
int     ftd_mngt_registration_key_req(sock_t *sockp);
int     ftd_mngt_tdmf_cmd(sock_t *sockp);
int     ftd_mngt_agent_general_config(sock_t *sockp, int iMngtType);
int     ftd_mngt_get_all_devices(sock_t *sockp);
int     ftd_mngt_get_perf_cfg(sock_t *sockp);
void    ftd_mngt_tracef( unsigned char pucLevel, char* pcpMsg, ... );
void    ftd_mngt_send_registration_key();

void    ftd_mngt_emulatorGetMachineName(int iEmulatorIdx, /*in*/const char *szMachineName, /*out*/char *szEmulatedMachineName, int size);
void    ftd_mngt_emulatorGetHostID(const char* szOriginHostId, int iEmulatorIdx, /*out*/char * szAgentUniqueID , int size);

#ifdef __cplusplus
}
#endif


//
// Declarations related to Agent Emulation mode
//
#define TDMF_AGENT_EMULATOR     1   //allow emulator code to be compiled or not

typedef struct __TDMFAgentEmulator
{
    int             bEmulatorEnabled;// 0 = false, otherwise true.
    int             iAgentRangeMin,
                    iAgentRangeMax;

} TDMFAgentEmulator;

//defined in libftd\ftd_mngt.cpp
extern  TDMFAgentEmulator   gTdmfAgentEmulator;

#if TDMF_AGENT_EMULATOR  
#define EMULATOR_MODE           (gTdmfAgentEmulator.bEmulatorEnabled != 0)

#define BEGIN_EMULATOR_LOOP     int iEmulatorIdx = gTdmfAgentEmulator.iAgentRangeMin; do {
#define END_EMULATOR_LOOP       }while( EMULATOR_MODE && ++iEmulatorIdx <= gTdmfAgentEmulator.iAgentRangeMax )

#else   //TDMF_AGENT_EMULATOR  

#define EMULATOR_MODE           false
#define BEGIN_EMULATOR_LOOP     
#define END_EMULATOR_LOOP       

#endif  //TDMF_AGENT_EMULATOR  


#endif	//_FTD_MNGT_H

