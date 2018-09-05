/********************************************************* {COPYRIGHT-TOP} ***
* IBM Confidential
* OCO Source Materials
* 6949-32F - Softek Replicator for Unix and 6949-32K - Softek TDMF (IP) for Unix
*
*
* (C) Copyright IBM Corp. 2006, 2011  All Rights Reserved.
* The source code for this program is not published or otherwise  
* divested of its trade secrets, irrespective of what has been 
* deposited with the U.S. Copyright Office.
********************************************************* {COPYRIGHT-END} **/
/* #ident "@(#)$Id: tdmfAgent_trace.h,v 1.7 2014/05/06 16:19:16 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _TDMFAGENT_TRACE_H_
#define _TDMFAGENT_TRACE_H_

void	logout(int,int,char *);
void	logoutx(int,int,const char *,const char *,int);
void	logoutdelta(char *);

#define F_main												1
#define F_Agent_accept_proc									2
#define F_program_exit										3
#define F_abnormal_exit										4
#define F_time_out_daemon									5
#define F_Agent_proc										6
#define F_ftd_sock_recv										7
#define F_ftd_sock_send										8
#define F_abort_Agn											9
#define F_sock_create										10
#define F_sock_init											11
#define F_tcp_socket										12
#define F_sock_bind_to_port									13
#define F_tcp_bind											14
#define F_sock_delete										15
#define F_sock_disconnect									16
#define F_tcp_disconnect									17
#define F_sock_sendtox										18
#define F_sock_recvfrom										19
#define F_ftd_mngt_getServerId								20
#define F_cfg_get_software_key_value						21
#define F_cfg_set_software_key_value						22
#define F_ipstring_to_ip									23
#define F_ip_to_name										24
#define F_name_to_ip										25
#define F_sock_is_me										26
#define F_sock_ip_to_ipstring								27
#define F_ip_to_ipstring									28
#define F_name_is_ipstring									29
#define F__reporterr										30
#define F_mmp_convert_mngt_hdr_ntoh							31
#define F_mmp_convert_mngt_hdr_hton							32
#define F_mmp_convert_TdmfServerInfo_hton					33
#define F_mmp_convert_TdmfPerfConfig_ntoh					34
#define F_mmp_convert_TdmfAgentDeviceInfo_hton				35
#define F_mmp_convert_TdmfAlert_hton						36
#define F_mmp_convert_TdmfAlert_ntoh						37
#define F_mmp_convert_TdmfStatusMsg_hton					38
#define F_mmp_convert_TdmfReplGroupMonitor_hton				39
#define F_mmp_convert_TdmfGroupState_hton					40
#define F_htonI64											41
#define F_mmp_convert_perf_instance_hton					42
#define F_mmp_convert_TdmfServerInfo_ntoh					43
#define F_rv2												44
#define F_cfg_get_key_value									45
#define F_cfg_set_key_value									46
#define F_getline											47
#define F_sem_create										48
#define F_sem_rm											49
#define F_sem_close											50
#define F_sem_wait											51
#define F_sem_signal										52
#define F_sem_op											53
#define F_cnvmsg											54
#define F_parmcpy											55
#define F_atollx											56
#define F_lltoax											57
#define F_ftd_agn_get_rmd_stat								58
#define F_ftd_mngt_initialize								59
#define F_ftd_mngt_create_broadcast_listener_socket			60
#define F_ftd_mngt_receive_on_broadcast_listener_socket		61
#define F_ftd_mngt_send_agentinfo_msg						62
#define F_ftd_mngt_alive_socket								63
#define F_ftd_mngt_StatusMsgThread							64
#define F_ftd_mngt_agent_general_config						65
#define F_set_service_port									66
#define F_isValidBABSize									67
#define F_getversion										68
#define F_ftd_mngt_gather_server_info						69
#define F_setvtocinfo										70
#define F_force_dsk_or_rdsk									72
#define F_convert_lv_name									73
#define F_is_logical_volume									74
#define F_get_char_device_info								75
#define F_dev_info											76
#define F_capture_logical_volumes							77
#define F_hd_config_addr									78
#define F_queryvgs											79
#define F_capture_devs_in_dir								80
#define F_walk_rdsk_subdirs_for_devs						81
#define F_walk_dirs_for_devs								82
#define F_capture_all_devs									83
#define F_ftd_mngt_get_all_devices							84
#define F_mmp_mngt_build_SetConfigurationMsg				85
#define F_mmp_mngt_release_SetConfigurationMsg				86
#define F_ftd_mngt_read_file_from_disk						87
#define F_ftd_mngt_get_config								88
#define F_ftd_mngt_get_perf_cfg								89
#define F_ftd_mngt_performance_force_acq					90
#define F_ftd_mngt_reduce_perf_upload_period				91
#define F_ftd_mngt_performance_reduce_stat_upload_period	92
#define F_ftd_mngt_get_force_fast_send_end_time				93
#define F_ftd_mngt_performance_get_wakeup_period			94
#define F_ftd_mngt_performance_is_time_to_upld				95
#define F_getlgnum											97
#define F_freelgnum											98
#define F_ftd_mngt_performance_send_all_to_Collector		99
#define F_setStartState										100
#define F_setGroupNbr										101
#define F_getGroupNbr										102
#define F_isGroupStarted									103
#define F_hasGroupStateChanged								104
#define F_RepGrpState										105
#define F_getGroupState										106
#define F_setRateDeltaTime									107
#define F_getRate											108
#define F_setCheckpointState								109
#define F_ispmd												110
#define F_isrmdx											111
#define F_checkrmd											112
#define F_isrmd												114
#define F_getdevinfo										115
#define F_setInstanceName									116
#define F_StatThread										117
#define F_ftd_mngt_performance_send_data					118
#define F_isRegistrationKeyValid							119
#define F_ftd_mngt_key_get_licence_key_expiration_date		120
#define F_ftd_mngt_registration_key_req						121
#define F_ftd_mngt_send_registration_key					122
#define F_getjournaldirname									123
#define F_getdiskuseinfo									124
#define F_getjournalfilevalue								125
#define F_getReplGrpSourceIP								126
#define F_ftd_mngt_send_lg_state							127
#define F_mmp_mngt_recv_cfg_data							128
#define F_mmp_mngt_free_cfg_data_mem						129
#define F_ftd_mngt_write_file_to_disk						130
#define F_ftd_mngt_set_config								131
#define F_ftd_mngt_tdmf_cmd									132
#define F_send_event										133
#define F_ftd_bab_initialize								134
#define F_ftd_system_restarting								135
#define F_ftd_master_restarting								136
#define F_ftd_mngt_sys_request_system_restart				137
#define F_connect_st										138
#define F_getnetconfcount									139
#define F_getnetconfs										140
#define F_rv4												141
#define F_ftd_mngt_send_all_state							142
#define F_mmp_mngt_recv_file_data							143
#define F_ftd_mngt_set_file									144
#define F_ftd_mngt_get_file									145
#define F_check_cfg_filename								146
#define F_send_lg_journal									147
#define F_time_so_far_usec									148
#define F_make_md_list										149
#define F_close_part_list									150
#define F_ismatchinlist										151
#define F_make_part_list									152
#define F_opened_num										153
#define F_survey_cfg_file									154
#define F_get_migrate_dir									155
#define F_config_file_edit									156
#define F_config_file_delete								157
#define F_config_file_list									158
#define F_migrate_agent_cfg_file							159
#define F_migrate_lg_cfg_file								160
#define F_Agnt_disksize                                     161
#define F_Agnt_brute_force_get_device_size					162
#define F_collect_server_product_usage_stats				163
#define F_ftd_mngt_get_product_usage_data					164
#endif	/* _TDMFAGENT_TRACE_H_ */
