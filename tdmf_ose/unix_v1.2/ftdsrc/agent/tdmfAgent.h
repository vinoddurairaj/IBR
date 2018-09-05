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
/* #ident "@(#)$Id: tdmfAgent.h,v 1.27 2018/02/28 00:25:45 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _TDMFAGENT_H_
#define _TDMFAGENT_H_


#include <inttypes.h>
#include <dirent.h>

#if defined(_AIX)

#define DTCVAROPTDIR               "/var/"PKGNAME
#define AGNVAROPTDIR               "/var/"PKGNAME
#define DTCCFGDIR                  "/etc/"PKGNAME"/lib"
#define TDMFFILEDIR	               DTCCFGDIR
#if defined(AGN_TEST)
#define AGTCFGDIR                  "."
#else
#define AGTCFGDIR                  "/etc/"PKGNAME"/lib"
#endif
#define DTCCMDDIR                  "/usr/"PKGNAME"/bin"

#else /* defined(SOLARIS) or defined(HPUX) */

#define DTCVAROPTDIR               "/var/opt/"PKGNAME
#define AGNVAROPTDIR               "/var/opt/"PKGNAME
#define DTCCFGDIR                  "/etc/opt/"PKGNAME
#define TDMFFILEDIR                DTCCFGDIR
#if defined(AGN_TEST)
#define AGTCFGDIR                  "."
#else
#define AGTCFGDIR                  "/etc/opt/"PKGNAME
#endif
#define DTCCMDDIR                  "/opt/"PKGNAME"/bin"

#endif

#if defined(linux)
// We used to explicitely include <linux/version.h> here.
// Look for LINUX_VERSION_CODE comments within Makefile.inc.Linux for other details.
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#define PATH_DRIVER_FILES          "/etc/opt/"PKGNAME"/driver"   /* pc070816 */
#define MODULES_CONFIG_PATH        "/etc/modprobe.d/sftkdtc.conf"

#define MODULES_NAME               "sftk"QNM
#endif

#define AGTTMPDIR	AGNVAROPTDIR"/Agn_tmp"

#define FTD_AGENT                  "in."QAGN
#define FTD_LISTNER                QNM"Listener"
#define EVENT_FILE                 DTCVAROPTDIR"/"QNM"error.log"
#define LIC_FILE                   DTCCFGDIR"/"CAPQ".lic"
#define DEFAULT_TDMF_DOMAIN_NAME   "(not specified)"
#define AGN_CFGFILE                AGTCFGDIR"/"QNM"Agent.cfg"
#define PRE_AGN_CFGFILE            AGTCFGDIR"/."QNM"Agent.cfg"
#define SERVICES_FILE              "/etc/services"
#define PRE_SERVICES_FILE          "/etc/services.pre"QAGN
#define AGENTTMP                   AGTTMPDIR"/agent.tmp"
#define CHECKPOINT_FIFO            AGTTMPDIR"/agent.fifo"
#define PATH_CONFIG                DTCCFGDIR
#define AGN_ERRLOGMAX              (1024*1024)

#if defined(SOLARIS)
#define AGN_STOP_SH		"/etc/init.d/"PKGNAME"-startagent stop > /dev/null 2>&1"
#elif defined(HPUX)
#define AGN_STOP_SH		"/sbin/init.d/"PKGNAME"-startagent stop > /dev/null 2>&1"
#elif defined(_AIX)
#define AGN_STOP_SH		"/etc/"PKGNAME"/init.d/"PKGNAME"-startagent stop > /dev/null 2>&1"
#elif defined(linux)
#define AGN_STOP_SH		"/etc/init.d/"PKGNAME"-startagent stop > /dev/null 2>&1"
#endif

/* Agent config file key parameter */
#define AGENT_CFG_IP               "CollectorIP"
#define AGENT_CFG_AGENTIP          "AgentIP"
#define AGENT_CFG_PORT             "CollectorPORT"
#define AGENT_CFG_BAB              "BabSize"
#define AGENT_CFG_EMULATOR         "AgentEmulator"
#define AGENT_CFG_DOMAIN           "domain"
#define TRACELEVEL                 "tracelevel"
#define MSGFILEPATH                "MsgFilePath"
#define REVERSE                    "reverse"
#define PERFUPLOADPERIOD           "PerfUploadPeriod"
#define REPLGROUPMONITUPLOADPERIOD "ReplGroupMonitUploadPeriod"
#define EMULATORRANGEMIN           "EmulatorRangeMin"
#define EMULATORRANGEMAX           "EmulatorRangeMax"
#define MASK                       "mask"
#define INSTALLPATH                "InstallPath"
#define TRANSMITINTERVAL           "TransmitInterval"
#define LISTENERPORT			   "ListenerPort"

#define TDMF_MSGLOCK                ((key_t) 51341L)
#define BIGCOUNT                    10000
#define FTD_DEF_LISTNERPORT         10112
#define FTD_DEF_AGENTPORT           577
#define FTD_DEF_COLLECTORPORT       576
#define MAXHOST                     128
#define FTDCMANAGEMENT              65
#define LOCALHOSTIP                	0x100007f
#define LOOPBACKIP                	0x7f000001
#define MNGT_MSG_MAGICNUMBER       	0xe8ad4239
#define SENDERTYPE_TDMF_SERVER     	0						
#define SENDERTYPE_TDMF_AGENT      	2
#define RETRY_COUNT                 3

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
#define	MMP_MNGT_MAX_IP_STR_SZ_V6		48
#define MMP_MNGT_MAX_REGIST_KEY_SZ      40
#define MMP_MNGT_MAX_OS_TYPE_SZ         30
#define MMP_MNGT_MAX_OS_VERSION_SZ      50
#define MMP_MNGT_MAX_FILE_SYSTEM_SZ     16
#define MMP_MNGT_MAX_TDMF_VERSION_SZ    40
#define N_MAX_IP                        4
#define NULL_IP_STR                     "0.0.0.0"
#define MMP_MNGT_DEFAULT_INTERVAL       4
#define MMP_MNGT_MAX_INTERVAL           3600
#define IP_ADDR_LEN                     15
#define MAX_IP_ADDRESS_VALUE            255
#define AGN_CFG_PARM_SIZE               256
#define CMDLINE_SIZE                    512
#define LOGMSG_SIZE                     1024
#define FILE_PATH_LEN                   256

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
#define MMP_MNGT_FS_ZFS     60 /* Zfs file system. Denotes a physical virtual device that is member of a zpool. */

#define ftd_int64          long long
#define ftd_uint64         unsigned long long

// WI_338550 December 2017, implementing RPO / RTT
#undef  ENABLE_RPO_DEBUGGING

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
,MMP_MNGT_TDMF_CMD_HANDLE
,MMP_MNGT_TDMF_CMD_PANALYZE
,MMP_MNGT_TDMF_CMD_EXPAND
,MMP_MNGT_TDMF_CMD_LIMITSIZE
,LAST_TDMF_CMD          = MMP_MNGT_TDMF_CMD_LIMITSIZE
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
,AGENT_CFG
};

/*
STRATION_KEY msg
*/

#define MNGT_VERSIONED_TYPE(version, mngtKey)  (((version) << 16) + (mngtKey))
enum management
{
/* management msg types */      /* 0x1000 = 4096 */
 MMP_MNGT_SET_LG_CONFIG      = 0x1000 /* receiving one logical group configura tion data
                                           (.cfg file) */
,MMP_MNGT_SET_CONFIG_STATUS           /* receiving configuration data */
,MMP_MNGT_GET_LG_CONFIG               /* request to send one logical group configuration data
                                           (.cfg file) */
,MMP_MNGT_AGENT_INFO_REQUEST          /* request for host information:
                                           IP, listener socket port, ... */
,MMP_MNGT_AGENT_INFO                  /* TDMF Agent response to a MMP_ MNGT_AGENT_INFO_REQUEST
                                           message OR SET TDMF Agent general information
                                           (S erver info)   */
,MMP_MNGT_REGISTRATION_KEY            /* SET/GET registration key */
,MMP_MNGT_TDMF_CMD                    /* management cmd, specified by a tdmf_c ommands sub-cmd */
,MMP_MNGT_TDMF_CMD_STATUS             /* response to management cmd */
,MMP_MNGT_SET_AGENT_GEN_CONFIG        /* SET TDMF Agent general config. parame ters */
,MMP_MNGT_GET_AGENT_GEN_CONFIG        /* GET TDMF Agent general config. parame ters */
,MMP_MNGT_GET_ALL_DEVICES             /* GET list of devices (disks/vo lumes)
                                           present on TDMF Agent system */
,MMP_MNGT_SET_ALL_DEVICES             /* response to a MMP_MNGT_GET_ALL_DEVICES
                                           request message. */
,MMP_MNGT_SET_ALL_DEVICES_1=MNGT_VERSIONED_TYPE(1, MMP_MNGT_SET_ALL_DEVICES)
				      /* response to a MMP_MNGT_GET_ALL_DEVICES
                                           request message with devno values. */
,MMP_MNGT_ALERT_DATA=MMP_MNGT_SET_ALL_DEVICES+1 /* TDMF Agent sends this message to TDMF Collector
                                           to report an alert condition */
,MMP_MNGT_STATUS_MSG
,MMP_MNGT_PERF_MSG                    /* TDMF Agent periodically sends perform ance data
                                           to TDMF Collector */
,MMP_MNGT_PERF_CFG_MSG                /* various performance counters and paci ng values */
,MMP_MNGT_MONITORING_DATA_REGISTRATION/* Request the TDMF Collector to be noti fied for any 
                                           TDMF Agents status/mode/performance data changes. */
,MMP_MNGT_AGENT_ALIVE_SOCKET          /* Agent opens a socket on Collector and sends this msg.
                                           Both peers keep socket connected at all times.
                                           Used by Collec tor for Agent detection. */
,MMP_MNGT_AGENT_STATE                 /* Agent sends this msg to report about its various
                                           states to Collector. */
,MMP_MNGT_GROUP_STATE                 /* TDMF Agent periodical ly sends group states to
                                           TDMF Collector */
,MMP_MNGT_TDMFCOMMONGUI_REGISTRATION  /* TDMF Common GUI sends this msg period ically as
                                           a watchdog to keep or claim ownership over
                                           the Collector and TDMF DB. */
,MMP_MNGT_GROUP_MONITORING            /* TDMF Agent sends various rela-time gr oup information
                                           to Collector */
,MMP_MNGT_SET_DB_PARAMS               /* TDMF Common GUI sends this msg to set various
                                           parameters related to the Tdmf DB */
,MMP_MNGT_GET_DB_PARAMS               /* TDMF Common GUI sends this msg to set various
                                           parameters related to the Tdmf DB */
,MMP_MNGT_AGENT_TERMINATE             /* TDMF Collector requests that an Repl.
                                           Server Agent stops running. */
,MMP_MNGT_GUI_MSG                     /* Inter-GUI message */
,MMP_MNGT_COLLECTOR_STATE             /* TDMF collector sends this msg to GUI to report
                                           its state */
,MMP_MNGT_TDMF_SENDFILE               /* receiving a TDMF files (script or zip) */
,MMP_MNGT_TDMF_SENDFILE_STATUS        /* receiving TDMF files (script or zip) */
,MMP_MNGT_TDMF_GETFILE                /* request to send TDMF files (tdmf*.bat) */
    // TDMF Common GUI sends this msg to delete the specified agent from the db
,MMP_MNGT_REQUEST_AGENT_REMOVAL
// Enhanced execute command request
,MMP_MNGT_CMD_EXECUTE_EX
// Enhanced execute commend result
,MMP_MNGT_CMD_RESULT_EX
// Message to upload the Product usage statistics of a server to the DMC
,MMP_MNGT_GET_PRODUCT_USAGE_DATA
    // The following messages are using the new extended header (mmp_MessageHeaderEx)
    // Extension of serverinfo structure for IPv6
,MMP_MNGT_EX_BASE = 0x2000
	// New Server Info message w/IPv6 support
,MMP_MNGT_EX_AGENT_INFO2
 // New response message to a MMP_MNGT_GET_ALL_DEVICES request
,MMP_MNGT_EX_DEVICE_INFO
};



 /* CHANGES FOR IPV6 SUPPORT */
/***************************************************************************/

#define IPSTRING_LEN   48
#define IPV6_SHORT_LEN  8
#define IPV4_BYTE_LEN 4

#define ABORT_IF(cond) if (cond) goto Abort;
#define ABORT_TRY
#define ABORT_CATCH Abort:

// This is a improved verion of above
// It allows a block to be called no matter the status (success or failure) of the try block
// to do some cleanup.
//
//        ABORT_TRY
//        {
//          //...
//        }
//        ABORT_CATCH_FINALLY
//        {
//          //...
//        }
//        ABORT_FINALLY
#define ABORT_CATCH_FINALLY goto AbortEnd; Abort:
#define ABORT_FINALLY   AbortEnd:

typedef enum ip_address_version_e
{
    IPVER_UNDEFINED = 0,
    IPVER_4,
    IPVER_6
} ip_address_version_t;

typedef struct ipAddressV6_s
{
	unsigned short Word[IPV6_SHORT_LEN];
	unsigned int ScopeID;
} ipAddressV6_t;

typedef struct ipAddress_s
{
    ip_address_version_t Version;
    union
    {
        unsigned int V4;
		ipAddressV6_t V6;
    } Addr;
} ipAddress_t;





typedef struct socks_s {
        int sockID;               /* socket for request recv     */
        int type;                 /* protocol type - AF_IN ET, AF_UNIX    */
        int family;               /* protocol family - STR EAM, DGRAM     */
        int tsbias;               /* time diff between loc al and remote  */
        char lhostname[MAXHOST];  /* local host name */
        char rhostname[MAXHOST];  /* remote host name */
        ipAddress_t lip;        /* local ip address */
        ipAddress_t rip;        /* remote ip address */
        unsigned int lhostid;    /* local hostid */
        unsigned int rhostid;    /* remote hostid */
        int port;                 /* connection port numbe r        */
        int readcnt;              /* number of bytes read */
        int writecnt;             /* number of bytes written */
        int flags;                /* state bits */
} sock_t;


sock_t * sock_create(void);


#if	defined(HPUX)
#pragma pack 1 
#else
#pragma pack(1)
#endif 


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
    mmp_mngt_header_t hdr;
    char              szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ]; 
                               /* name of machine identifying the TDMF Agent. */
    unsigned short    usLgId;  /*logical group for whic h the config. set was requested*/
    int               iStatus; /* 0 = OK, 1 = rx/tx error with TDMF Agent, */
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

    /* Next fields imported from Windows header file lib\libmngt\libmngtdef.h for new definitions related to Windows clusters, 
       for compatibility in messages size in communications with the Collector (pc070816) */
    /* Next fields were added in MMP_PROTOCOL_VERSION 5.  IMPORTANT =>  Note: Always add new fields at the end of the
                           structure to preserve backward compatibility. */
    char            szClusterVirtualName[N_MAX_IP][MMP_MNGT_MAX_MACHINE_NAME_SZ];   /* V5 - Cluster Virtual name    */
    char            szClusterVirtualIpAddress[N_MAX_IP][MMP_MNGT_MAX_IP_STR_SZ];    /* V5 - Cluster Virtual ip address */
    int             iClusterNodeState[N_MAX_IP];                                    /* V5 - Cluster current node state */
    char            szReserved[256];                                                /* V5 - reserved  */

} mmp_TdmfServerInfo;


/*

  Support for  new PROTOCOL 

*/

enum FeatureState
    {
    MMP_MNGT_FEATURE_NOT_SUPPORTED = 0,
    MMP_MNGT_FEATURE_DISABLED = -1,
    MMP_MNGT_FEATURE_ENABLED = 1
    };

  /***************************************************************************************\

Struct:         __mmp_TdmfServerInfo2

Description:    Second generation server info message.  It supports easy upgrade of content.
                RFX263: Synchronized with RFW definitions related ServerInfo2 Version 2

\***************************************************************************************/
#define SERVERINFO2_VERSION 4

typedef struct __mmp_TdmfServerInfo2
{
    // Agent host id
    char            ServerHostID[MMP_MNGT_MAX_MACHINE_NAME_SZ];
    // nbr of valid szIPAgent strings
    unsigned char   IPCount;
    // v4 or v6 address string format
    char            IPAgent[N_MAX_IP][MMP_MNGT_MAX_IP_STR_SZ_V6];
    // dotted decimal format, can be "0.0.0.0" if no router
    char            IPRouter[MMP_MNGT_MAX_IP_STR_SZ_V6];
    // name of TDMF domain
    char            DomainName[MMP_MNGT_MAX_DOMAIN_NAME_SZ];
    // name of machine where this TDMF Agent runs.
    char            MachineName[MMP_MNGT_MAX_MACHINE_NAME_SZ];
    // port nbr exposed by this TDMF Agent
    int             AgentListenPort;

    // KiloBytes
    int             TCPWindowSize;
    int             CPUCount;
    // KiloBytes
    int             RAMSize;
    // KiloBytes
    int             AvailableRAMSize;

    char            OsType[MMP_MNGT_MAX_OS_TYPE_SZ];
    char            OsVersion[MMP_MNGT_MAX_OS_VERSION_SZ];
    char            ProductVersion[MMP_MNGT_MAX_TDMF_VERSION_SZ];
    char            InstallPath[MMP_MNGT_MAX_TDMF_PATH_SZ];
    
    struct tagFeatures
    {
        struct tagBAB
	{
            // 0 = not supported, 1 = on, -1 = off.  See FeatureState enum
            char State;
            // Requested BAB size (MegaBytes)
            int RequestedSize;
            // Actual BAB size (MegaBytes)
            int ActualSize;
            // Agent is able to allocate or free BAB dynamically: 0 = not supported, 1 = on, -1 = off.  See FeatureState enum
            char DynamicAllocationState;
            struct tagOptimizedRecording
                {
                // 0 = not supported, 1 = on, -1 = off.  See FeatureState enum
                char State;
                // High threshold, in percentage
                char HighThreshold;
                // Low threshold, in percentage
                char LowThreshold;
                // 0 = off, 1 = on
                char CollisionDetectionState;
                } OptimizedRecording;
	} BAB;

        struct tagPStore
            {
            char Path[MMP_MNGT_MAX_TDMF_PATH_SZ];
            
            struct tagHighResolutionBitmap
                {
                // 0 = not supported, 1 = on, -1 = off.  See FeatureState enum
                char State;
                // HRT can be set to large mode (for large volumes): 0 = not supported, 1 = on, -1 = off.  See FeatureState enum
				// <<< add a new value to indicate Proportional HRDB; MUST CHECK DMC also
                char LargeState;
                // HRT size in bits
                int Size;
                } HighResolutionBitmap;
            struct tagLowResolutionBitmap
                {
                // 0 = not supported, 1 = on, -1 = off.  See FeatureState enum
                char State;
                // LRT persistence can be disabled: 0 = not supported, 1 = supported
                char PersistencyState;
                // LRT size in bits
                int Size;
                } LowResolutionBitmap;
            } PStore;

        struct tagJournal
            {
            // 0 = not supported, 1 = on, -1 = off.  See FeatureState enum
            char State;
            char Path[MMP_MNGT_MAX_TDMF_PATH_SZ];
            } Journal;
        
        struct tagCluster
            {
            // 0 = not supported, 1 = on, -1 = off.  See FeatureState enum
            char State;
            // Cluster Virtual name array
            char VirtualName[MMP_MNGT_MAX_MACHINE_NAME_SZ];
            // Cluster Virtual ip address array
            char VirtualIpAddress[MMP_MNGT_MAX_IP_STR_SZ_V6];
            // Cluster current node state
            char NodeState;
           } Cluster;

        // Attributes supported in group config files
        struct tagGroup
	{
            // Autostart - specifies if the 'AUTOSTART' keyword in the group config files is supported: 0 = not supported, 1 = supported
            char AutoStart;
                
            // Dynamic Driver Activation - specifies if the 'DYNAMIC-ACTIVATION' keyword in the group config files is supported: 0 = not supported, 1 = supported
            char DynamicInsertion;

            struct tagThinCopy
	    {
                // 'THIN-COPY' supported for mirror groups: 0 = not supported, 1 = supported
                char MirrorState;
                // 'THIN-COPY' supported for non-mirror groups: 0 = not supported, 1 = supported
                char NonMirrorState;    
	    } ThinCopy;
	} Group;
    } Features;
} mmp_TdmfServerInfo2;

#define SERVERINFO2_SIZE sizeof(mmp_TdmfServerInfo2)

typedef struct __mmp_MessageHeaderEx
{
    // Server unique identifier
    char    szServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];
    // Size of message following this header
    unsigned int   ulMessageSize;
    // Version of message following this header
    unsigned int   ulMessageVersion;
    // Number of messages following this header
    unsigned int   ulInstanceCount;
} mmp_MessageHeaderEx;




/** Imported from Windows definitions (pc070817)
 * MMP_PROTOCOL_VERSION value is used to identify a precise version of the
 * management protocol. The Collector will detect a version difference with any
 * Replication Server (refer to Alive socket code). When this occurs, it report
 * this state to its log file file and can also send a TERMINATE msg to the Repl.
 * Server. when required.
 */
#define MMP_PROTOCOL_VERSION    6



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


typedef struct mmp_mngt_TdmfAgentConfig2Msg_s {
    mmp_mngt_header_t       hdr;
	mmp_MessageHeaderEx		hdrEx;
    mmp_TdmfServerInfo2      data;
} mmp_mngt_TdmfAgentConfig2Msg_t ;


/***************************************************************************************\

Struct:         __mmp_TdmfServerConfig

Description:    Server configuration parameters sent from the Collector to the Agents.
                RFX263: Synchronized with RFW definitions related ServerInfo2 Version 2

\***************************************************************************************/

typedef struct __mmp_TdmfServerConfig
    {
    char szServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];   // TDMF unique host id
    char szTDMFDomain[MMP_MNGT_MAX_DOMAIN_NAME_SZ];   // Name of the TDMF domain

    int  iPort;
    int  iTCPWindowSize;   // KiloBytes
    int  iBABSizeReq;      // Requested BAB size in MegaBytes
    int  iLargePStoreSupport;

    char cThinBABSupport;                 // 0 = not supported, 1 = on, -1 = off.  See FeatureState enum
    char cThinBABHighThreshold;           // In percentage
    char cThinBABLowThreshold;            // In percentage
    char cThinBABBlockCollisionDetection; // 0 = off, 1 = on

    char szReserved[508];
    } mmp_TdmfServerConfig;

#define SERVERCONFIG_SIZE sizeof(mmp_TdmfServerConfig)


// MMP_MNGT_SET_AGENT_GEN_CONFIG (Protocol Version >= 6)
// Sent by TDMF Collector to TDMF Agent to configure TDMF parameters
typedef struct mmp_mngt_TdmfServerConfigMsg_s
    {
    mmp_mngt_header_t     hdr;
    mmp_TdmfServerConfig  data;
} mmp_mngt_TdmfServerConfigMsg_t;


/* MMP_MNGT_GET_ALL_DEVICES */
typedef struct mmp_mngt_TdmfAgentDevicesMsg_s {
    mmp_mngt_header_t   hdr;
    char                szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ]; 
                          /* name of machine identifying the TDMF Agent. */
    int                 iNbrDevices;
                          /* number of mmp_TdmfDeviceInfo contiguous to this structure */
} mmp_mngt_TdmfAgentDevicesMsg_t ;

/* MMP_MNGT_EX_DEVICE_INFO */
typedef struct mmp_mngt_TdmfAgentExDeviceInfoHeader_s {
    mmp_mngt_header_t   hdr;
    mmp_MessageHeaderEx	hdrEx;
} mmp_mngt_TdmfAgentExDeviceInfoHeader_t ;

#define DEVICEINFOEX_VERSION 1
typedef struct __mmp_TdmfDeviceInfoEx {
    char            szDrivePath[MMP_MNGT_MAX_DRIVE_SZ]; /* drive / path identifying the drive/volume. */
					 
    int16_t         sFileSystem;                        /* MMP_MNGT_FS_... values */
    char	        szFileSystemSpecificText[64];       /* Used for the zpool membership name in the case of MMP_MNGT_FS_ZFS */
    
    uint64_t        liDriveId;                          /* volume id. UINT64_MAX if not used. */
    
    uint64_t        liStartOffset;                      /* volume starting offset. */
    uint64_t        liLength;                           /* volume length. */
    
    uint64_t        liDeviceMajor;                      /* AIX only: device major number. UINT64_MAX on all other platforms than AIX. */
    uint64_t        liDeviceMinor;                      /* AIX only: device minor number. UINT64_MAX on all other platforms than AIX. */
} mmp_TdmfDeviceInfoEx;

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
    int             iType;   /* enum tdmf_filetype */
    unsigned int    uiSize;  /* size of following data. */
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
#if	defined(HPUX)
#pragma pack 1 
#else
#pragma pack(1)
#endif 

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
        int          iReplGrpSourceIP;  /* IP address of the Source, for agents <2.6.1 */
        short        sReplGrpNbr;       /* replication group number, >= 0 */
        char         isSource;
         /* indicates if this server is the SOURCE */
         /* (primary) or TARGET (secondary) server of this replication group */
        char    iReplGrpSourceIP_V6[MMP_MNGT_MAX_IP_STR_SZ_V6];     // IP address of the Source, for agents >=2.6.1
        char    notused[209];                                       // for future use
} mmp_TdmfReplGroupMonitor;

#if defined(HPUX)
#pragma pack 0
#elif defined(SOLARIS)
#pragma pack()
#elif defined(_AIX)
#pragma pack(pop)
#else
#pragma pack(0)
#endif

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



#if defined(HPUX)
#pragma pack 0
#elif defined(SOLARIS)
#pragma pack()
#elif defined(_AIX)
#pragma pack(pop)
#else
#pragma pack(0)
#endif



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
#define FTDPERFMAGIC	0xBADF00D6

/* lib/libgen/ftd_cmd.h */
#ifdef NOT_USE_LIBGEN
#define FTD_MAX_GROUPS		1000
#define	FTD_MAX_DEVICES		256
#define FTD_PS_DEVICE_ATTR_SIZE 4*1024
#endif
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

// WI_338550 December 2017, implementing RPO / RTT
#if	defined(HPUX)
#pragma pack 1 
#else
#pragma pack(1)
#endif 
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
    // WI_338550 December 2017, implementing RPO / RTT
    struct
    {
        unsigned char rpoIndicators     : 1;
        unsigned char thinBabIndicators : 1;
        unsigned char                   : 6;
    } features;
    
    int     currentRpo;          //
    int     maxRpo;              //
    int     currentRtt;          // rpoIndicators
    int     timeRemaining;       //
    int     pendingMB;           //
    char	    padding[2 + 12]; /* to align structure on a 8 byte boundary.*/
} ftd_perf_instance_t;
// WI_338550 December 2017, implementing RPO / RTT
#if defined(HPUX)
#pragma pack 0
#elif defined(SOLARIS)
#pragma pack()
#elif defined(_AIX)
#pragma pack(pop)
#else
#pragma pack(0)
#endif


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




typedef struct __TDMFAgentEmulator
{
    int		bEmulatorEnabled;	/* 0 = false, otherwise true. */
    int		iAgentRangeMin;
    int		iAgentRangeMax;
} TDMFAgentEmulator;

#ifdef NOT_USE_LIBGEN

#ifndef FTD_IOCTL_CALL
#define	FTD_IOCTL_CALL(a,b,c)	ioctl(a,b,c)
#endif

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
    ftd_uint64 rfshoffset;      /* rsync sectors done */  
    ftd_uint64 rfshdelta;       /* rsync sectors changed */ 
} devstat_t;
#endif

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

// WI_338550 December 2017, implementing RPO / RTT
extern int debug_RPO;

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

/* Agent_uty.c */

extern int name_is_ipstring(char *strip);
extern int cfg_set_software_key_value(char *key, char *data);
extern int cfg_get_software_key_value(char *key, char *data, int size);
extern int yesorno(char *message);
extern int survey_cfg_file(char *location, int cfg_file_type);
extern int cfg_get_key_value(char *key, char *value, int stringval);
extern int cfg_set_key_value(char *key, char *value, int stringval);
extern int ipstring_to_ip_uint(char *name, unsigned int *ip);
extern int ipstring_to_ip(char* ipStr, ipAddress_t *ip);
extern int ip_to_ipstring(ipAddress_t ip, char* ipstring);
extern int ip_to_ipstring_uint(unsigned int ip, char *ipstring);
extern int ip_to_name(ipAddress_t ip, char *name);

extern int ftd_mngt_set_config(sock_t *sockID);
extern int ftd_mngt_get_config(sock_t *sockID);
extern int ipstring_to_ipv4(char* ipStr, ipAddress_t *ip);
extern int ipstring_to_ipv6(char* ipStr, ipAddress_t *ip);
extern void atollx(char [],unsigned long long *);
extern int msg_init(char *title);
extern int is_unspecified(ipAddress_t pIp);
extern void mmp_convert_mngt_hdr_ntoh(mmp_mngt_header_t *hdr);
extern void mmp_convert_mngt_hdr_hton(mmp_mngt_header_t *hdr);
extern void mmp_convert_mngt_hdrEx_hton(mmp_MessageHeaderEx* hdrEx);
extern void mmp_convert_mngt_hdrEx_ntoh(mmp_MessageHeaderEx* hdrEx);
extern void mmp_convert_TdmfServerInfo2_hton(mmp_TdmfServerInfo2* srvrInfo2);
extern void mmp_convert_TdmfServerInfo2_ntoh(mmp_TdmfServerInfo2* srvrInfo2);
extern void mmp_convert_TdmfStatusMsg_hton(mmp_TdmfStatusMsg *statusMsg);
extern void mmp_convert_TdmfAgentDeviceInfoEx_hton(mmp_TdmfDeviceInfoEx *devInfo);
extern void mmp_convert_TdmfGroupState_hton(mmp_TdmfGroupState *state);
extern void mmp_convert_FileTransferData_hton(mmp_TdmfFileTransferData *data);
extern void mmp_convert_perf_instance_hton(ftd_perf_instance_t *perf);
extern void mmp_convert_TdmfPerfConfig_ntoh(mmp_TdmfPerfConfig *perfCfg);
extern void mmp_convert_TdmfReplGroupMonitor_hton(mmp_TdmfReplGroupMonitor *GrpMon);
extern void mmp_convert_TdmfServerConfig_ntoh(mmp_TdmfServerConfig* srvrConfig);
extern int agent_getnetconfcount(void);
extern int agent_getnetconfcount6(void);
extern void agent_getnetconfs6(char* ipv6);
extern int  ftd_mngt_get_perf_cfg(sock_t *sockp);
extern int isrmdx(int num);
extern int ismatchinlist( char *inpstr );
extern int make_part_list();
extern int ipReset(ipAddress_t* poIpAddress);
extern int sock_recvfrom(sock_t *sockp, char *buf, int len);
extern int ftd_mngt_send_agentinfo_msg(ipAddress_t rip, int rport);
extern int sock_sendtox(ipAddress_t rip,int rport,char * data,int datalen,int a,int b);
// extern int connect_st(int s, const struct sockaddr *name, int namelen, int *level);
extern void ftd_mngt_send_registration_key();
extern void cnvmsg(char *text,int size);
extern int ftd_mngt_tdmf_cmd(sock_t *sockID);
extern int ftd_mngt_registration_key_req(sock_t *sockID);
extern int check_cfg_filename(struct dirent *dent, int type);
extern int ftd_mngt_agent_general_config(sock_t *sockID, int iMngtType);
extern int ftd_agn_get_rmd_stat(sock_t *sockID);
extern int ftd_mngt_set_file(sock_t *sockID);
extern int ftd_mngt_get_file(sock_t *sockID);
extern int ftd_sock_recv(sock_t *sockt, char *readpacket, int  len);
extern int ftd_sock_send(sock_t *sockt, char  *writepacket, int len);
extern char get_customer_name_checksum(void);
extern void ftd_mngt_gather_server_info(mmp_TdmfServerInfo2 *srvrInfo);
extern void ftd_mngt_sys_request_system_restart(mmp_TdmfServerConfig *sip, int flag);
extern int ftd_mngt_get_product_usage_data(sock_t *sockID);
