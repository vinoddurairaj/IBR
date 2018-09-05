/* #ident "@(#)$Id: tdmfAgent.h,v 1.28 2003/11/13 11:05:51 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _TDMFAGENT_H_
#define _TDMFAGENT_H_

#include <stropts.h>

#if defined(_AIX)

#define DTCVAROPTDIR	"/var/dtc"
#define AGTVAROPTDIR	"/var/dua"
#define DTCCFGDIR	"/etc/dtc/lib"
#define TDMFFILEDIR	DTCCFGDIR
#if defined(AGN_TEST)
#define AGTCFGDIR	"."
#else
#define AGTCFGDIR	"/etc/dua/lib"
#endif
#define DTCCMDDIR	"/usr/dtc/bin"

#else /* defined(SOLARIS) or defined(HPUX) */

#define DTCVAROPTDIR	"/var/opt/SFTKdtc"
#define AGTVAROPTDIR	"/var/opt/SFTKdua"
#define DTCCFGDIR	"/etc/opt/SFTKdtc"
#define TDMFFILEDIR	DTCCFGDIR
#if defined(AGN_TEST)
#define AGTCFGDIR	"."
#else
#define AGTCFGDIR	"/etc/opt/SFTKdua"
#endif
#define DTCCMDDIR	"/opt/SFTKdtc/bin"

#endif

#if defined(linux)
#define PATH_DRIVER_FILES   "/opt/SFTKdtc/driver"
#define MODULES_CONFIG_PATH "/etc/modules.conf"
#define MODULES_NAME        "sftkdtc"
#endif

#define AGTTMPDIR	""AGTVAROPTDIR"/Agn_tmp"

#define FTD_AGENT 		"dtcAgent"
#define FTD_LISTNER 		"dtcListener"
#define TDMF_TOOLS_PREFIX      	"dtc" /* to confirm */
#define EVENT_FILE      	""DTCVAROPTDIR"/dtcerror.log"
#define LIC_FILE		""DTCCFGDIR"/DTC.lic"
#define DEFAULT_TDMF_DOMAIN_NAME "(not specified)"
#define AGN_CFGFILE		""AGTCFGDIR"/dtcAgent.cfg"
#define PRE_AGN_CFGFILE		""AGTCFGDIR"/.dtcAgent.cfg"
#define SERVICES_FILE          	"/etc/services"
#define PRE_SERVICES_FILE      	"/etc/services.predua"
#define AGENTTMP		""AGTTMPDIR"/agent.tmp"
#define CHECKPOINT_FIFO		""AGTTMPDIR"/agent.fifo"
#define PATH_CONFIG                "DTCCFGDIR"

#if defined(SOLARIS)
#define AGN_STOP_SH		"/etc/init.d/SFTKdua stop > /dev/null 2>&1"
#elif defined(HPUX)
#define AGN_STOP_SH		"/opt/SFTKdua/bin/SFTKdua stop > /dev/null 2>&1"
#elif defined(_AIX)
#define AGN_STOP_SH		"/etc/dua/init.d/SFTKdua stop > /dev/null 2>&1"
#elif defined(linux)
#define AGN_STOP_SH		"/etc/init.d/SFTKdua stop > /dev/null 2>&1"
#endif


#define TDMF_MSGLOCK			((key_t) 51341L)
#define BIGCOUNT			10000
#define FTD_DEF_LISTNERPORT  		10112
#define FTD_DEF_AGENTPORT    		577
#define FTD_DEF_COLLECTORPORT   	576
#define MAXHOST  			17
#define FTDCMANAGEMENT			65
#define LOCALHOSTIP                	0x100007f
#define LOOPBACKIP                	0x7f000001
#define MNGT_MSG_MAGICNUMBER        	0xe8ad4239
#define SENDERTYPE_TDMF_SERVER      	0
#define SENDERTYPE_TDMF_AGENT       	2
#define RETRY_COUNT                     3

#define FALSE 0
#define TRUE 1
#define false 0
#define true 1
#define CFG_IS_NOT_STRINGVAL   0
#define CFG_IS_STRINGVAL       1

#define MMP_MNGT_MAX_FILENAME_SZ        256
#define MMP_MNGT_MAX_DRIVE_SZ           256
#define MMP_MNGT_MAX_VOLUME_SZ          256
#define MMP_MNGT_MAX_TDMF_PATH_SZ       256
#define MMP_MNGT_MAX_MACHINE_NAME_SZ    80
#define MMP_MNGT_MAX_HOST_ID_NAME_SZ    16
#define MMP_MNGT_MAX_DOMAIN_NAME_SZ     80
#define MMP_MNGT_MAX_IP_STR_SZ          20
#define MMP_MNGT_MAX_REGIST_KEY_SZ      40
#define MMP_MNGT_MAX_OS_TYPE_SZ         30
#define MMP_MNGT_MAX_OS_VERSION_SZ      50
#define MMP_MNGT_MAX_FILE_SYSTEM_SZ     16
#define MMP_MNGT_MAX_TDMF_VERSION_SZ    40
#define N_MAX_IP                        4
#define NULL_IP_STR                     "0.0.0.0"
#define MMP_MNGT_DEFAULT_INTERVAL		4
#define FILE_PATH_LEN                    256

#if defined(SOLARIS) || defined(HPUX)
#define INT_MAX       2147483647    /* maximum (signed) int value */
#endif

/* File System values */
#define MMP_MNGT_FS_IGNORE	-1
#define MMP_MNGT_FS_UNKNOWN	0  /* unknown or unformatted partition/volume */
#define MMP_MNGT_FS_VXFS	30 /* HP File Systems : 30,... */
#define MMP_MNGT_FS_HFS		31
#define MMP_MNGT_FS_UFS		40 /* Solaris File Systems : 40,... */
#define MMP_MNGT_FS_JFS		50 /* AIX File Systems : 50,... */

#define ftd_int64          long long
#define ftd_uint64         unsigned long long

enum mngt_status
{
 MMP_MNGT_STATUS_OK                          =  0    /* success */
,MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR   =  100  /* could not connect to TDM F Collector socket */
,MMP_MNGT_STATUS_ERR_UNKNOWN_TDMFAGENT               /* specified TDMF Agent (sz ServerUID) is unknown to TDMF Collector */
,MMP_MNGT_STATUS_ERR_CONNECT_TDMFAGENT               /* could not connect to TDM F Agent. */
,MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFAGENT              /* unexpected communication rupture while rx/tx data with TDMF Agent */
,MMP_MNGT_STATUS_ERR_MSG_RXTX_TDMFCOLLECTOR          /* unexpected communication rupture while rx/tx data with TDMF Collector */

,MMP_MNGT_STATUS_ERR_SET_AGENT_GEN_CONFIG            /* Agent could not save the received Agent config.  Continuing using current config. */
,MMP_MNGT_STATUS_ERR_INVALID_SET_AGENT_GEN_CONFIG    /* provided Agent config. contains illegal values and is rejected by Agent.  Continuing using current confi g. */

,MMP_MNGT_STATUS_ERR_COLLECTOR_INTERNAL_ERROR        /* basic functionality failed ... */

,MMP_MNGT_STATUS_ERR_BAD_OR_MISSING_REGISTRATION_KEY

,MMP_MNGT_STATUS_ERR_ERROR_ACCESSING_TDMF_DB         /* 109 */
};

/* available Tdmf commands.  to be used when calling mmp_mngt_sendTdmfCommand() */
enum tdmf_commands
{
 FIRST_TDMF_CMD         = 0x2000

,MMP_MNGT_TDMF_CMD_START     = FIRST_TDMF_CMD
,MMP_MNGT_TDMF_CMD_STOP
,MMP_MNGT_TDMF_CMD_INIT
,MMP_MNGT_TDMF_CMD_OVERRIDE
,MMP_MNGT_TDMF_CMD_INFO
,MMP_MNGT_TDMF_CMD_HOSTINFO
,MMP_MNGT_TDMF_CMD_LICINFO
,MMP_MNGT_TDMF_CMD_RECO
,MMP_MNGT_TDMF_CMD_SET
,MMP_MNGT_TDMF_CMD_LAUNCH_PMD
,MMP_MNGT_TDMF_CMD_LAUNCH_REFRESH
,MMP_MNGT_TDMF_CMD_LAUNCH_BACKFRESH
,MMP_MNGT_TDMF_CMD_KILL_PMD
,MMP_MNGT_TDMF_CMD_KILL_RMD
,MMP_MNGT_TDMF_CMD_KILL_REFRESH
,MMP_MNGT_TDMF_CMD_KILL_BACKFRESH
,MMP_MNGT_TDMF_CMD_CHECKPOINT
,MMP_MNGT_TDMF_CMD_OS_CMD_EXE
,MMP_MNGT_TDMF_CMD_TESTTDMF
,MMP_MNGT_TDMF_CMD_TRACE

,LAST_TDMF_CMD          = MMP_MNGT_TDMF_CMD_TRACE
,INVALID_TDMF_CMD
};

enum agent_command
{
 FIRST_AGN_CMD         = 0x9000

,MMP_AGN_TDMF_CMD_START     = FIRST_AGN_CMD
,MMP_AGN_TDMF_CMD_RMDCHK
,MMP_AGN_TDMF_CMD_END

,LAST_AGN_CMD          = MMP_AGN_TDMF_CMD_END
,INVALID_AGN_CMD
};

enum tdmf_commands_status
{
 MMP_MNGT_TDMF_CMD_STATUS_OK      =   0
,MMP_MNGT_TDMF_CMD_STATUS_ERROR
,MMP_MNGT_TDMF_CMD_STATUS_ERR_COULD_NOT_REACH_TDMF_AGENT
};

/* cfg_file_type */
enum cfg_file_type
{
 ALL_CFG     =  0
,PRIMARY_CFG
,SECONDARY_CFG
};

/*
STRATION_KEY msg
*/

enum management
{
/* management msg types */      /* 0x1000 = 4096 */
 MMP_MNGT_SET_LG_CONFIG      = 0x1000 /* receiving one logical group configura
							 tion data (.cfg file) */
,MMP_MNGT_SET_CONFIG_STATUS           /* receiving configuration data */
,MMP_MNGT_GET_LG_CONFIG               /* request to send one logical group
					      configuration data (.cfg file) */
,MMP_MNGT_AGENT_INFO_REQUEST          /* request for host information:
				            IP, listener socket port, ... */
,MMP_MNGT_AGENT_INFO                  /* TDMF Agent response to a 
					 MMP_ MNGT_AGENT_INFO_REQUEST message */
                                      /* OR  */
                                      /* SET TDMF Agent general information
						 (S erver info)   */
,MMP_MNGT_REGISTRATION_KEY            /* SET/GET registration key */
,MMP_MNGT_TDMF_CMD                    /* management cmd, specified by a
						 tdmf_c ommands sub-cmd */
,MMP_MNGT_TDMF_CMD_STATUS             /* response to management cmd */
,MMP_MNGT_SET_AGENT_GEN_CONFIG        /* SET TDMF Agent general config.
							 parame ters */
,MMP_MNGT_GET_AGENT_GEN_CONFIG        /* GET TDMF Agent general config.
							 parame ters */
,MMP_MNGT_GET_ALL_DEVICES             /* GET list of devices (disks/vo lumes)
						present on TDMF Agent system */
,MMP_MNGT_SET_ALL_DEVICES             /* response to a MMP_MNGT_GET_AL L_DEVICES request message. */
,MMP_MNGT_ALERT_DATA                  /* TDMF Agent sends this message to TDMF Collector to report an alert condition */
,MMP_MNGT_STATUS_MSG
,MMP_MNGT_PERF_MSG                    /* TDMF Agent periodically sends perform ance data to TDMF Collector */
,MMP_MNGT_PERF_CFG_MSG                /* various performance counters and paci ng values */
,MMP_MNGT_MONITORING_DATA_REGISTRATION/* Request the TDMF Collector to be noti fied for any TDMF Agents status/mode/performance data changes. */
,MMP_MNGT_AGENT_ALIVE_SOCKET          /* Agent opens a socket on Collector and sends this msg.  Both peers keep socket connected at all times.  Used by Collec tor for Agent detection. */
,MMP_MNGT_AGENT_STATE                 /* Agent sends this msg to report about its various states to Collector. */
,MMP_MNGT_GROUP_STATE                 /* TDMF Agent periodical ly sends group states to TDMF Collector */
,MMP_MNGT_TDMFCOMMONGUI_REGISTRATION  /* TDMF Common GUI sends this msg period ically as a watchdog to keep or claim ownership over the Collector and TDMF DB. */
,MMP_MNGT_GROUP_MONITORING            /* TDMF Agent sends various rela-time gr oup information to Collector */
,MMP_MNGT_SET_DB_PARAMS               /* TDMF Common GUI sends this msg to set various parameters related to the Tdmf DB */
,MMP_MNGT_GET_DB_PARAMS               /* TDMF Common GUI sends this msg to set various parameters related to the Tdmf DB */
,MMP_MNGT_AGENT_TERMINATE             /* TDMF Collector requests that an Repl.  Server Agent stops running. */
,MMP_MNGT_GUI_MSG                     /* Inter-GUI message */
,MMP_MNGT_COLLECTOR_STATE             /* TDMF collector sends this msg to GUI to report its state */
,MMP_MNGT_TDMF_SENDFILE               /* receiving a TDMF files (script or zip) */
,MMP_MNGT_TDMF_SENDFILE_STATUS        /* receiving TDMF files (script or zip) */
,MMP_MNGT_TDMF_GETFILE                /* request to send TDMF files (tdmf*.bat) */

};

typedef struct socks_s {
        int sockID;               /* socket for request recv     */
        int type;                 /* protocol type - AF_IN ET, AF_UNIX    */
        int family;               /* protocol family - STR EAM, DGRAM     */
        int tsbias;               /* time diff between loc al and remote  */
        char lhostname[MAXHOST];  /* local host name */
        char rhostname[MAXHOST];  /* remote host name */
        unsigned long lip;        /* local ip address */
        unsigned long rip;        /* remote ip address */
        unsigned long lhostid;    /* local hostid */
        unsigned long rhostid;    /* remote hostid */
        int port;                 /* connection port numbe r        */
        int readcnt;              /* number of bytes read */
        int writecnt;             /* number of bytes written */
        int flags;                /* state bits */
} sock_t;


typedef struct mmp_mngt_header_s {
    int magicnumber;         /* indicates the following is a management message */
    int mngttype;            /* enum management, the management msg type. */

    int sendertype;          /* indicates this message sender :
					 TDMF_AGENT , TDMF_SERVER, CLIENT_APP */
                             /* only TDMF Collector checks this flag */
    int mngtstatus;          /* enum mngt_status.  
				this field is used to return a response status.
			        used when the message is a response to a m ngt request. */
} mmp_mngt_header_t;


/* sent in response to a MMP_MNGT_SET_LG_CONFIG request. */
/* indicates if configuration was updated succesfully */
typedef struct mmp_mngt_ConfigurationStatusMsg_s {
    mmp_mngt_header_t   hdr;
    char                szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ]; 
		      /* name of machine identifying the TDMF Agent. */
    unsigned short      usLgId;
                     /*logical group for whic h the config. set was requested*/
    int                 iStatus;
                            /* 0 = OK, 1 = rx/tx error with TDMF Agent, */
			    /* 2 = err writing cfg file */
} mmp_mngt_ConfigurationStatusMsg_t;


/* broadcasted by TDMF Collector (MMP_MNGT_AGENT_INFO_REQUEST) */
typedef struct __mmp_TDMFDBServerInfo
{
        int                         iBrdcstResponsePort; 
       /* port temporary exposed by the TDMF Collector strictly to respond to this request */

} mmp_TdmfDBServerInfo;

typedef struct __mmp_TDMFServerInfo /* __mmp_TDMFAgentInfo */
{
        char                    szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];
/* tdmf unique host id */
        char                    szIPAgent[ N_MAX_IP ][ MMP_MNGT_MAX_IP_STR_SZ ];
/* dotted decimal format */
        char                    szIPRouter[ MMP_MNGT_MAX_IP_STR_SZ ];
/* dotted decimal format, can be "0.0.0.0" if no router */
        char                    szTDMFDomain[ MMP_MNGT_MAX_DOMAIN_NAME_SZ ];
/* name of TDMF domain */
        char                    szMachineName[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];
/* name of machine where this TDMF Agent runs. */
        int             iPort;   /* port nbr exposed by this TDMF Agent */
    int             iTCPWindowSize;         /* KiloBytes */
    int             iBABSizeReq;            /* MegaBytes : requested BAB size */
    int             iBABSizeAct;            /* MegaBytes : actual BAB size */
    int             iNbrCPU;
    int             iRAMSize;               /* KiloBytes */
    int             iAvailableRAMSize;      /* KiloBytes */
    char            szOsType[MMP_MNGT_MAX_OS_TYPE_SZ];
    char            szOsVersion[MMP_MNGT_MAX_OS_VERSION_SZ];
    char            szTdmfVersion[MMP_MNGT_MAX_TDMF_VERSION_SZ];
    char            szTdmfPath[MMP_MNGT_MAX_TDMF_PATH_SZ];
    char            szTdmfPStorePath[MMP_MNGT_MAX_TDMF_PATH_SZ];
    char            szTdmfJournalPath[MMP_MNGT_MAX_TDMF_PATH_SZ];
    unsigned char   ucNbrIP;                /* nbr of valid szIPAgent strings */

} mmp_TdmfServerInfo;

typedef struct mmp_mngt_BroadcastReq_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfDBServerInfo    data;
} mmp_mngt_BroadcastReqMsg_t;

/* MMP_MNGT_AGENT_INFO */
/*  Sent by TDMF Agent to TDMF Collector to provide TDMF parameters */
typedef struct mmp_mngt_TdmfAgentConfigMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfServerInfo      data;
} mmp_mngt_TdmfAgentConfigMsg_t ;

/* MMP_MNGT_GET_ALL_DEVICES */
typedef struct mmp_mngt_TdmfAgentDevicesMsg_s {
    mmp_mngt_header_t	hdr;
    char     		szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ]; 
			/* name of machine identifying the TDMF Agent. */
    int                 iNbrDevices;
			/* number of mmp_TdmfDeviceInfo contiguous to this structure */
} mmp_mngt_TdmfAgentDevicesMsg_t ;

typedef struct __mmp_TdmfDeviceInfo {
    char            szDrivePath[MMP_MNGT_MAX_DRIVE_SZ];
					 /* drive / path identifying the drive/volume. */
    short           sFileSystem;         /* MMP_MNGT_FS_... values */
    char            szDriveId[24];       /* volume id, 64 bit integer value in text format. eg: "1234567890987654" */
    char            szStartOffset[24];   /* volume starting offset, 64 bit integer value. eg: "1234567890987654" */
    char            szLength[24];        /* volume length, 64 bit integer value. eg: "1234567890987654" */
} mmp_TdmfDeviceInfo;

/* MMP_MNGT_PERF_CFG_MSG */
typedef struct __mmp_TdmfPerfConfig {
    /* all periods specified in tenth of a second (0.1 sec) units */
    /* any values <= 0 will be considered as non-initialized and 
		will not be refreshed by TDMF Agents */
    int     iPerfUploadPeriod;	
	/* period at which the TDMF Agent acquires the Performence data */
    int     iReplGrpMonitPeriod;
	/* period at which the TDMF Agent acquires all Replication Groups 
		Moinitoring Data */
    int     notused[12];            /* for future use */
} mmp_TdmfPerfConfig;

/* MMP_MNGT_PERF_CFG_MSG */
/*  Sent by TDMF Collector to all TDMF Agents. */
/*  Collector saves into the TDMF DB NVP table and sends to all Agents in DB */
typedef struct mmp_mngt_TdmfPerfCfgMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfPerfConfig      data;
} mmp_mngt_TdmfPerfCfgMsg_t;

sock_t * sock_create(void);

enum tdmf_filetype
{
 MMP_MNGT_FILETYPE_TDMF_CFG      =   0
,MMP_MNGT_FILETYPE_TDMF_EXE
,MMP_MNGT_FILETYPE_TDMF_BAT
,MMP_MNGT_FILETYPE_TDMF_TAR
,MMP_MNGT_FILETYPE_TDMF_ZIP
};

typedef struct __mmp_TdmfFileTransferData {
    char            szFilename[ MMP_MNGT_MAX_FILENAME_SZ ];	
			/* name of file at destination, WITHOUT path. */
    int             iType;		/* enum tdmf_filetype */
    unsigned int    uiSize;		/* size of following data. */
    /* the file binary data must be contiguous to this structure. */
} mmp_TdmfFileTransferData;

/*
 * MMP_MNGT_GET_LG_CONFIG :
 *  request a TDMF Agent for an existing logical group configuration 
 *  file (.cfg).
 * MMP_MNGT_SET_LG_CONFIG :
 *  request a TDMF Agent to use the supplied logical group configuration 
 *  file (.cfg).
 *
 * Name of cfg file identifies the logical group.
 *
 * MMP_MNGT_GET_LG_CONFIG : 
 *  Name of the requested file must be provided within the 'data' structure.  
 *  The 'data.uiFileSize' member MUST be set to 0.
 *  Other 'data' members are ignored.
 * MMP_MNGT_SET_LG_CONFIG : 
 *  All members of the 'data' structure must be filled properly.
 *  This message will serve as response to a MMP_MNGT_GET_LG_CONFIG request.
 */
typedef struct mmp_mngt_ConfigurationMsg_s {
    mmp_mngt_header_t	hdr;
    char		szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];
			    /* name of machine identifying the TDMF Agent. */
    mmp_TdmfFileTransferData    data;	/* cfg file is transfered */
} mmp_mngt_ConfigurationMsg_t;

/* MMP_MNGT_TDMF_SENDFILE : */
/*   All members of the 'data' structure must be filled properly. */
typedef struct mmp_mngt_FileMsg_s {
	mmp_mngt_header_t           hdr;
	char                        szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];
                                        /* name of machine identifying the TDMF Agent. */
	mmp_TdmfFileTransferData    data;                   /* TDMF file is transfered */
} mmp_mngt_FileMsg_t;

/* sent in response to a MMP_MNGT_TDMF_SENDFILE request. */
/* indicates if TDMF script was updated succesfully with a MMP_MNGT_TDMF_SENDFILE_STATUS */
typedef struct mmp_mngt_FileStatusMsg_s {
	mmp_mngt_header_t           hdr;
	char                        szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];
                                        /* name of machine identifying the TDMF Agent. */
	unsigned short              usLgId;
                                        /* logical group for which the config. set was requested */
	int                         iStatus;
                                        /* 0 = OK, 1 = rx/tx error with TDMF Agent,2 = err writing cfg file */
} mmp_mngt_FileStatusMsg_t;

/* MMP_MNGT_ALERT_DATA */
typedef struct __mmp_TdmfAlertHdr {
    char	szServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];
		/* name of machine identifying the TDMF Agent. */
    short	sLGid;                  
		/* number identifying the logical group (0 to 999) */
    short	sDeviceId;              /* DTC number */
    unsigned int	uiTimeStamp;            
		/* GMT date-time , nbr of seconds since 01-01-1970 */
    char	cSeverity;              
		/* 1= highest severity, 5 = lowest severity */
    char	cType;                  
		/* message Type : refer to enum tdmf_alert_type */
    /* 
     * two zero-terminated strings follow this structure
     * the first is a string identifying the Module generating the message.
     * the second is a string containing the text message.
     */ 
} mmp_TdmfAlertHdr;

enum tdmf_alert_type
{
 MMP_MNGT_MONITOR_TYPE_INFORMATION  = 0
,MMP_MNGT_MONITOR_TYPE_WARNING
,MMP_MNGT_MONITOR_TYPE_ERROR
};


/* MMP_MNGT_TDMF_CMD */
/* ask a TDMF Agent to perform a specific action */
typedef struct mmp_mngt_TdmfCommandMsg_s {
    mmp_mngt_header_t      hdr;
    char     szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];
                  /* name of machine identifying the TDMF Agent. */
    int                    iSubCmd;
                  /* value from enum tdmf_commands */
    char                   szCmdOptions[ 256 ];
                  /* list of options for sub-cmd, similar to a Tdmf exe cmd line */
} mmp_mngt_TdmfCommandMsg_t;


/* MMP_MNGT_TDMF_CMD_STATUS, */
/* message sent by a TDMF Agent in response to a MMP_MNGT_TDMF_CMD request */
typedef struct mmp_mngt_TdmfCommandMsgStatus_s {
    mmp_mngt_header_t  hdr;
    char               szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];
                                /* name of machine identifying the TDMF Agent.*/
    int                iSubCmd; /* value from enum tdmf_commands */
    int                iStatus; /* value from enum tdmf_commands_status */
    int                iLength; /* length of string (including \0) */
                                /*                following this structure */
                                /* the string is the console output */
                                /*     of the tdmf command tool performing */
                                /*      the command. */
} mmp_mngt_TdmfCommandStatusMsg_t;

/*
 * MMP_MNGT_PERF_MSG
 * Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
 */
typedef struct __mmp_TdmfPerfData {
    char        szServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];  
		/* name of machine identifying the TDMF Agent. */
    int         iPerfDataSize;
                /* size of performance data (in bytes) following this structure.
                 * the performance data is in the form of a number of
                 * ftd_perf_instance_t structures
                 * (iPerfDataSize / sizeof(ftd_perf_instance_t))
                 * If, for the reception peer, the result of
                 * iPerfDataSize / sizeof(ftd_perf_instance_t) has a
                 * remainder (iPerfDataSize % sizeof(ftd_perf_instance_t) != 0),
                 * it could mean that both peers are not synchronized
                 * on the ftd_perf_instance_t struct. definition.
                 */
} mmp_TdmfPerfData;

/*
 * MMP_MNGT_PERF_MSG
 * Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
 */
typedef struct mmp_mngt_TdmfPerfMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfPerfData        data;
    /*
     * this message structure is followed by a variable number of
     * ftd_perf_instance_t structures. refer to mmp_TdmfPerfData members.
     */
} mmp_mngt_TdmfPerfMsg_t;

/*
 * MMP_MNGT_GROUP_STATE
 * Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
 */
typedef struct __mmp_TdmfGroupState {
   short        sRepGrpNbr;  /* valid number have values >= 0. */
   short        sState;      /* bit0 : 0 = group stopped, 1 = group started. */
} mmp_TdmfGroupState;

/*
 * MMP_MNGT_GROUP_STATE
 * Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
 */
typedef struct mmp_mngt_TdmfGroupStateMsg_s {
    mmp_mngt_header_t       hdr;
    char                    szServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];
    /*
     * name of machine identifying the TDMF Agent.
     * this message structure is followed by a variable number of
     * mmp_TdmfGroupState structures
     * the last mmp_TdmfGroupState structure will show an invalid
     * group number value.
     */
} mmp_mngt_TdmfGroupStateMsg_t;


/*
 * MMP_MNGT_GROUP_MONITORING
 */
typedef struct __mmp_TdmfReplGroupMonitor {
        char         szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];
        /* unique Host ID (in hexadecimal text format) of the TDMF Agent. */
        /* 64-bit integer value */
        ftd_int64    liActualSz;
        /* Total size (in bytes) of PStore file or Journal file for this group. */
        ftd_int64    liDiskTotalSz;
        /* Total size (in MB) of disk on which PStore file or Journal file is maintained. */
        ftd_int64    liDiskFreeSz;
        /* Total free space size (in MB) of disk on which PStore file or Journal file is maintained. */
        int          iReplGrpSourceIP;  /* IP address of the Source */
        short        sReplGrpNbr;       /* replication group number, >= 0 */
        char         isSource;
         /* indicates if this server is the SOURCE */
         /* (primary) or TARGET (secondary) server of this replication group */
        char         notused1[1];  /*for future use, back to a 4-byte boundary*/
        char         notused2[256];/*for future use */
} mmp_TdmfReplGroupMonitor;

/* Sent by a TDMF Agent to TDMF Collector */
typedef struct mmp_mngt_TdmfReplGroupMonitorMsg_s {
    mmp_mngt_header_t		hdr;
    mmp_TdmfReplGroupMonitor	data;
} mmp_mngt_TdmfReplGroupMonitorMsg_t;

/*
 * MMP_MNGT_AGENT_ALIVE_SOCKET
 * Sent once by a TDMF Agent to TDMF Collector
 */
typedef struct mmp_mngt_TdmfAgentAliveMsg_s {
    mmp_mngt_header_t    hdr;
    char                 szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];
        /* unique Host ID (in hexadecimal text format) of the TDMF Agent. */
    int                  iMsgLength;
        /*
         * number of bytes following this message.
         * zero-terminated text string containing tags-values pairs can be
         * contiguous to this message.
         */
} mmp_mngt_TdmfAgentAliveMsg_t;

/* MMP_MNGT_STATUS_MSG */
typedef struct __mmp_TdmfStatusMsg {
    char	szServerUID[80];
		/* name of machine identifying the TDMF Agent. */
    char	cPriority;
		/* can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR */
    char	cTag;
		/* one of the TAG_... value */
    char   	pad[2];   /* mtldev */
		/*to align remaining elements of structure */

    int  	iTdmfCmd;
		/* tdmf command to which this message is relatedto.  
			0 if irrelevant. */
    int		iTimeStamp;
    int		iLength; 
    		/* length of message string following this structure, 
			including terminating '\0' character. */
} mmp_TdmfStatusMsg;

/* MMP_MNGT_STATUS_MSG */
/*  Sent by TDMF Agents to TDMF Collector.  
	Collector saves the messages to the TDMF DB. */
typedef struct mmp_mngt_TdmfStatusMsgMsg_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfStatusMsg       data;
} mmp_mngt_TdmfStatusMsgMsg_t;

/* MMP_AGENT_RDMCHECK */
/*  Sent by TDMF Agents to TDMF Agent.   */
typedef struct mmp_agn_CheckRmd_s {
    mmp_mngt_header_t       hdr;
    int       		    lgnum;
} mmp_agn_CheckRmd_t;

#define CFG_IS_NOT_STRINGVAL		0
#define MAX_SIZEOF_INSTANCE_NAME	48

#define SECONDARYPORT			575

#define SECTORSIZE 512
#define ROUNDL( d ) ((long)((d) + ((d) > 0 ? 0.5 : -0.5)))
#define FTD_PS_DEVICE_ATTR_SIZE 4*1024
#define FTDPERFMAGIC	0xBADF00D6

/* lib/libgen/ftd_cmd.h */
#define FTD_MAX_GROUPS		1000
#define	FTD_MAX_DEVICES		256
#define SHARED_MEMORY_ITEM_COUNT	(FTD_MAX_GROUPS * FTD_MAX_DEVICES + 1)
#define SHARED_MEMORY_OBJECT_SIZE	(SHARED_MEMORY_ITEM_COUNT * sizeof(ftd_perf_instance_t))

/* device level statistics structure */
typedef struct ftd_dev_stat_s {
        int	devid;          /* device id */
        int     actual;         /* device actual (bytes) */
        int	effective;      /* device effective (bytes) */
        int entries;            /* device entries */
        int entage;             /* time between BAB write and RMD recv   */
        int jrnentage;          /* time between jrnl write and mir write */
        off_t rsyncoff;         /* rsync sectors done */
        int rsyncdelta;         /* rsync sectors changed */
        float actualkbps;       /* kbps transfer rate */
        float effectkbps;       /* effective transfer rate w/compression */ 
        float pctdone;          /* % of refresh or backfresh complete    */
        int sectors;            /* # of sectors in bab */
        int pctbab;             /* pct of bab in use */
        double local_kbps_read; /* ftd device kbps read */
        double local_kbps_written;  /* ftd device kbps written */
        ftd_uint64	ctotread_sectors;
        ftd_uint64	ctotwrite_sectors;
} ftd_dev_stat_t;

typedef struct ftd_perf_instance_s {
    ftd_int64       actual;
    ftd_int64       effective;
    ftd_int64       bytesread;
    ftd_int64       byteswritten;
    int             connection;
    int             drvmode;        /* driver mode */
    int             lgnum;          /* group number */
    int             insert;         /* gui list insert */
    int             devid;          /* device id */
    int             rsyncoff;       /* rsync sectors done */
    int             rsyncdelta;     /* rsync sectors changed */
    int             entries;        /* # of entries in bab */
    int             sectors;        /* # of sectors in bab */
    int             pctdone;
    int             pctbab;
    unsigned short wcszInstanceName[MAX_SIZEOF_INSTANCE_NAME];
    int             Reserved1;              /* unused */
    int             Reserved2;              /* unused */
    char	    role;
    char	    padding[3 + 32]; /* to align structure on a 8 byte boundary.*/
} ftd_perf_instance_t;

typedef struct ftd_perf_s {
    int			magicvalue;     /* so we know it's been initialized */
    int			hSharedMemory;
    int			hMutex;
    int			hGetEvent;
    int			hSetEvent;
    ftd_perf_instance_t	*pData;
} ftd_perf_t;

typedef struct grpstate_s {
	mmp_TdmfGroupState      m_state;
	int                     m_bStateChanged;
} grpstate_t;

/* MMP_MNGT_AGENT_INFO_REQUEST */
/* sent by client app to TDMF Collector or by TDMF Collector to TDMF Agent */
typedef struct __mmp_TdmfRegistrationKey
{
    char            szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];
    /* name of machine identifying the TDMF Agent. */
    char            szRegKey[ MMP_MNGT_MAX_REGIST_KEY_SZ ];
    int             iKeyExpirationTime;
    /* time_t. returned by a MMP_MNGT_REGISTRATION_KEY GET key cmd. if <= 1,
                                                           decode as follow: */
    /*
     KEY_OK                  1  -> indicates a permanent key: it never expires.
     KEY_GENERICERR          0
     KEY_NULL               -1
     KEY_EMPTY              -2
     KEY_BADCHECKSUM        -3
     KEY_EXPIRED            -4
     KEY_WRONGHOST          -5
     KEY_BADSITELIC         -6
     KEY_WRONGMACHINETYPE   -7
     KEY_BADFEATUREMASK     -8
     */
} mmp_TdmfRegistrationKey;


typedef struct mmp_mngt_RegistrationKey_s {
    mmp_mngt_header_t       hdr;
    mmp_TdmfRegistrationKey keydata;
} mmp_mngt_RegistrationKeyMsg_t;

typedef struct __TDMFAgentEmulator
{
    int		bEmulatorEnabled;	/* 0 = false, otherwise true. */
    int		iAgentRangeMin;
    int		iAgentRangeMax;
} TDMFAgentEmulator;

#define	FTD_IOCTL_CALL(a,b,c)	ioctl(a,b,c)

typedef struct devstat_s {
    struct devstat_s *p;	/* previous device stats */
    struct devstat_s *n;	/* next device stats */
    int devid;			/* device id */
    int a_tdatacnt;		/* header+compr bytes from lastts */
    int a_datacnt;		/* compr data bytes sent */
    int e_tdatacnt;		/* header+uncompr bytes sent */
    int e_datacnt;		/* uncompr data bytes sent */
    int entries;		/* entries sent/recv */
    int entage;			/* time between BAB write and recvd by RMD */
    int jrnentage;		/* time between journal write and mirro write */
    off_t rfshoffset;		/* rsync sectors done */
    int rfshdelta;		/* rsync sectors changed */
} devstat_t;

typedef struct cp_fifo_s {
    int		idx;
    int		lgnum;
    int		on_off; /* 0 = off, else = on */
} cp_fifo_t;

struct mnttab {
	char    *mnt_special;
	char    *mnt_mountp;
	char    *mnt_fstype;
	char    *mnt_mntopts;
	char    *mnt_time;
};

extern	int	getmntany(FILE *, struct mnttab *, struct mnttab *);

#if defined(_AIX)

/* these are found on most platforms in <sys/param.h>: */
#if !defined(DEV_BSIZE)
#define DEV_BSIZE       512
#endif /* !defined(DEV_BSIZE) */

#if !defined(DEV_BSHIFT)
#define DEV_BSHIFT      9               /* log2(DEV_BSIZE) */
#endif /* !defined(DEV_BSHIFT) */

#if !defined(SECTORSIZE)
#define SECTORSIZE 512
#endif /* !defined(SECTORSIZE) */

#include <sys/param.h> /* MAXPATHLEN usage */
#include <values.h> /* MAXINT usage */

#endif /* defined(_AIX) */

/* set requestRestart flag */
#define	GENE_CFG_BABSIZE	0x1
#define	GENE_CFG_WINDOWSIZE	0x2
#define	GENE_CFG_PORT		0x4
#define	GENE_CFG_DOMAIN		0x8
#endif	/* _TDMFAGENT_H_ */

/* DLKM check version */
#if defined(HPUX)
#define DLKM_TDMF_VER	10300000
#define DLKM_OS_VER		1100
#endif /* defined(HPUX) */
