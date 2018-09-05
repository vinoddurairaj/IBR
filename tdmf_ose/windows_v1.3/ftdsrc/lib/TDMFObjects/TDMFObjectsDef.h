#ifndef _H_TDMFOBJECTSDEF_H_
#define _H_TDMFOBJECTSDEF_H_


#define _TDMF_DB_ALL_TABLES			0
#define _TDMF_DB_DOMAIN_TABLE		1
#define _TDMF_DB_SERVER_TABLE		2
#define _TDMF_DB_GROUP_TABLE		3
#define _TDMF_DB_PAIR_TABLE			4
#define _TDMF_DB_PERFORMANCE_TABLE	5
#define _TDMF_DB_PEOPLE_TABLE		6
#define _TDMF_DB_ALERT_TABLE		7
#define _TDMF_DB_COMMAND_TABLE		8
#define _TDMF_DB_PARAMETERS_TABLE	9


#define _TDMF_ERROR_CODE_OK                               0
#define _TDMF_ERROR_CODE_SUCCESS                          _TDMF_ERROR_CODE_OK
#define _TDMF_ERROR_CODE_INTERNAL_ERROR                   1
#define _TDMF_ERROR_CODE_INVALID_PARAMETER                2
#define _TDMF_ERROR_CODE_ILLEGAL_IP_OR_PORT_VALUE         3
#define _TDMF_ERROR_CODE_SENDING_DATA_TO_COLLECTOR        4  //  error while sending data to TDMF Collector
#define _TDMF_ERROR_CODE_RECEIVING_DATA_FROM_COLLECTOR    5  //  error while receiving data from TDMF Collector
#define _TDMF_ERROR_CODE_UNABLE_TO_CONNECT_TO_COLLECTOR   6  //  unable to connect to TDMF Collector
#define _TDMF_ERROR_CODE_UNABLE_TO_CONNECT_TO_TDMF_AGENT  7  //  unable to connect to TDMF Agent
#define _TDMF_ERROR_CODE_COMM_RUPTURE_WITH_TDMF_AGENT     8  //  unexpected communication rupture while rx/tx data with TDMF Agent
#define _TDMF_ERROR_CODE_COMM_RUPTURE_WITH_TDMF_COLLECTOR 9  //  unexpected communication rupture while rx/tx data with TDMF Collector
#define _TDMF_ERROR_CODE_UNKNOWN_TDMF_AGENT               10 //  unknown TDMF Agent (szAgentId)
#define _TDMF_ERROR_CODE_UNKNOWN_DOMAIN_NAME              11 //  
#define _TDMF_ERROR_CODE_UNKNOWN_HOST_ID                  12 //  
#define _TDMF_ERROR_CODE_UNKNOWN_SOURCE_SERVER            13 //  
#define _TDMF_ERROR_CODE_UNKNOWN_TARGET_SERVER            14 //  
#define _TDMF_ERROR_CODE_UNKNOWN_REP_GROUP                15 
#define _TDMF_ERROR_CODE_SAVING_TDMF_AGENT_CONFIGURATION  16 //  Agent could not save the received Agent config.  Continuing using current config.
#define _TDMF_ERROR_CODE_ILLEGAL_TDMF_AGENT_CONFIGURATION_PROVIDED 17//provided Agent config. contains illegal values and is rejected by Agent.  Continuing using current config.
#define _TDMF_ERROR_CODE_DELETING_SOURCE_REP_GROUP        18
#define _TDMF_ERROR_CODE_DELETING_TARGET_REP_GROUP        19
#define _TDMF_ERROR_CODE_SET_SOURCE_REP_GROUP             20
#define _TDMF_ERROR_CODE_SET_TARGET_REP_GROUP             21
#define _TDMF_ERROR_CODE_CREATING_DB_RECORD               22
#define _TDMF_ERROR_CODE_UPDATING_DB_RECORD               23
#define _TDMF_ERROR_CODE_DELETING_DB_RECORD               24
#define _TDMF_ERROR_CODE_FINDING_DB_RECORD                25
#define _TDMF_ERROR_CODE_DATABASE_RELATION_ERROR          26
#define _TDMF_ERROR_CODE_DATABASE_TRANSACTION             27
#define _TDMF_ERROR_CODE_AGENT_PROCESSING_TDMF_CMD        28
#define _TDMF_ERROR_CODE_EMPTY_OBJECT_LIST_PROVIDED       29
#define _TDMF_ERROR_CODE_BAD_OR_MISSING_REGISTRATION_KEY  30
#define _TDMF_ERROR_CODE_NO_CFG_FILE                      31
#define _TDMF_ERROR_CODE_UNKNOWN_SCRIPT_SERVER_FILE       32
#define _TDMF_ERROR_CODE_ERROR_SENDING_SCRIPT_SERVER_FILE_TO_AGENT      33

#endif // _H_TDMFOBJECTSDEF_H_