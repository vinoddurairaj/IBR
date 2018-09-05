/*
 * libmngtdef.h -   declarations of structures shared by the library and its clients
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
#ifndef __LIBMNGTDEF_H_
#define __LIBMNGTDEF_H_



//avoid C++ name mangling problems when imported/used by C client program
#ifdef __cplusplus 
extern "C" {
#endif

#define MMP_MNGT_TIMEOUT_GETDEVICES     20*60   // 20 min. // ardev 030117
#define MMP_MNGT_TIMEOUT_DMF_CMD        20*60

#define MMP_MNGT_MAX_FILENAME_SZ        256
#define MMP_MNGT_MAX_DRIVE_SZ           256
#define MMP_MNGT_MAX_VOLUME_SZ          256
#define MMP_MNGT_MAX_MACHINE_NAME_SZ    80      //must be a multiple of 8 , some structure alignmenet rely on it.
#define MMP_MNGT_MAX_HOST_ID_NAME_SZ    16
#define MMP_MNGT_MAX_DOMAIN_NAME_SZ     80
#define MMP_MNGT_MAX_IP_STR_SZ          20
#define MMP_MNGT_MAX_REGIST_KEY_SZ      40
#define MMP_MNGT_MAX_OS_TYPE_SZ         30
#define MMP_MNGT_MAX_OS_VERSION_SZ      50
#define MMP_MNGT_MAX_FILE_SYSTEM_SZ     16
#define MMP_MNGT_MAX_TDMF_VERSION_SZ    40
#define MMP_MNGT_MAX_TDMF_PATH_SZ       256
#define N_MAX_IP                        4
#define MMP_MNGT_MAX_GUI_MSG_SZ         64

#define NULL_IP_STR                     "0.0.0.0"
#define MMP_NAT_ENABLED                 "*.*.*.*"

//the ServerUID member is used to identify a TDMF Agent
//if this name begins with the prefix HOST_ID_PREFIX, 
//then the remaining must be interpreted as the Agent HostId
//otherwise, the ServerUID must be interpreted as the Agent Server Host name.
#define HOST_ID_PREFIX                  "HostID="
#define HOST_ID_PREFIX_LEN              7


//File System values
#define MMP_MNGT_FS_UNKNOWN              0  //unknown or unformatted partition/volume
#define MMP_MNGT_FS_FAT                 10  //FAT (Windows) File Systems 10 to 19
#define MMP_MNGT_FS_FAT16               11  
#define MMP_MNGT_FS_FAT32               12
#define MMP_MNGT_FS_NTFS                20  //NTFS (Windows) File Systems 
//
// STEVE add support for dynamic disks
//
#define MMP_MNGT_FS_NTFS_DYN			1010
#define MMP_MNGT_FS_FAT_DYN				1011
#define MMP_MNGT_FS_FAT16_DYN			1012
#define MMP_MNGT_FS_FAT32_DYN			1020
//
#define MMP_MNGT_FS_VXFS                30  //HP File Systems : 30,...
#define MMP_MNGT_FS_HFS                 31
#define MMP_MNGT_FS_UFS                 40  //Solaris File Systems : 40,...
#define MMP_MNGT_FS_JFS                 50  //AIX File Systems : 50,...

//todo : add other file systems

#pragma pack(push, 1)   
    
/**********************************************************************************************/
/** ftd_perf_instance_t must be pack to 1 byte, as all the others structures TX over socket  **/
/**********************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
#include "ftd_perf.h"   
#ifdef __cplusplus
}
#endif
//*****************************************************************************
// Structures definitions
//*****************************************************************************
//MMP_MNGT_SET_GEN_CONFIG           // SET TDMF Agent general config. parameters
//MMP_MNGT_GET_GEN_CONFIG           // GET TDMF Agent general config. parameters
typedef struct __mmp_TDMFServerInfo//__mmp_TDMFAgentInfo
{
    char            szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];    //tdmf unique host id 
    char            szIPAgent[ N_MAX_IP ][ MMP_MNGT_MAX_IP_STR_SZ ];//dotted decimal format
    char            szIPRouter[ MMP_MNGT_MAX_IP_STR_SZ ];           //dotted decimal format, can be "0.0.0.0" if no router 
    char            szTDMFDomain[ MMP_MNGT_MAX_DOMAIN_NAME_SZ ];    //name of TDMF domain 
    char            szMachineName[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];  //name of machine where this TDMF Agent runs.
    int             iPort;                      //port nbr exposed by this TDMF Agent
    int             iTCPWindowSize;             //KiloBytes
    int             iBABSizeReq;                //MegaBytes : requested BAB size
    int             iBABSizeAct;                //MegaBytes : actual BAB size
    int             iNbrCPU;
    int             iRAMSize;                   //KiloBytes
    int             iAvailableRAMSize;          //KiloBytes
    char            szOsType[MMP_MNGT_MAX_OS_TYPE_SZ];
    char            szOsVersion[MMP_MNGT_MAX_OS_VERSION_SZ];
    char            szTdmfVersion[MMP_MNGT_MAX_TDMF_VERSION_SZ];
    char            szTdmfPath[MMP_MNGT_MAX_TDMF_PATH_SZ];
    char            szTdmfPStorePath[MMP_MNGT_MAX_TDMF_PATH_SZ];
    char            szTdmfJournalPath[MMP_MNGT_MAX_TDMF_PATH_SZ];
    unsigned char   ucNbrIP;                    //nbr of valid szIPAgent strings

} mmp_TdmfServerInfo;


//broadcasted by TDMF Collector (MMP_MNGT_AGENT_INFO_REQUEST)
typedef struct __mmp_TDMFDBServerInfo
{
    int             iBrdcstResponsePort;    //port temporary exposed by the TDMF Collector strictly to respond to this request

} mmp_TdmfDBServerInfo;


//MMP_MNGT_AGENT_INFO_REQUEST
//sent by client app to TDMF Collector or by TDMF Collector to TDMF Agent
typedef struct __mmp_TdmfRegistrationKey
{
    char            szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ];        //name of machine identifying the TDMF Agent.
    char            szRegKey[ MMP_MNGT_MAX_REGIST_KEY_SZ ];         
    int             iKeyExpirationTime;                         //returned by a MMP_MNGT_REGISTRATION_KEY GET key cmd. 
                                                                //If > 0, it has to be a time_t value representing the key expiration date/time.
                                                                //If <= 0, decode as follow:
                                                                /* 
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

typedef struct __mmp_TdmfFileTransferData {
    char            szFilename[ MMP_MNGT_MAX_FILENAME_SZ ];    //name of file at destination, WITHOUT path.
    int             iType;              //enum tdmf_filetype
    unsigned int    uiSize;             //size of following data. 
    //the file binary data must be contiguous to this structure.
} mmp_TdmfFileTransferData;


//MMP_MNGT_GET_ALL_DEVICES
typedef struct __mmp_TdmfDeviceInfo {
    char            szDrivePath[MMP_MNGT_MAX_DRIVE_SZ]; //drive / path identifying the drive/volume. 
    short           sFileSystem;            //MMP_MNGT_FS_... values     
    char            szDriveId[24];          //volume id, 64 bit integer value in text format. eg: "1234567890987654"
    char            szStartOffset[24];      //volume starting offset, 64 bit integer value. eg: "1234567890987654"
    char            szLength[24];           //volume length, 64 bit integer value. eg: "1234567890987654"
} mmp_TdmfDeviceInfo;

// By Saumya Tripathi 09/09/2004
// SAUMYA_FIX_DEVICE_UNIQUE_ID
#if 0
typedef struct ATTACHED_DISK_INFO
{
	// Disk number for reference in WMI
    ULONG			DiskNumber;

	// Physical Device name or WMI Instance Name
    WCHAR			PhysicalDeviceNameBuffer[256];

	BOOLEAN			bValidName;	// TRUE means all follwing BOOLEAN values are TRUE

	//		IOCTL_STORAGE_GET_DEVICE_NUMBER to retrieve this info, 
	// -	if disk is PhysDisk type then IOCTL_STORAGE_GET_DEVICE_NUMBER succeeds and 
	//		we store name string \\Device\HardDisk(n)\\Partition(n) in DiskPartitionName
	// -	if disk is not PhyDisk then IOCTL_STORAGE_GET_DEVICE_NUMBER fails
	//		We use IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, to get device name string and that string 
	//		we store in DiskVolumeName.
	//		It also uses IOCTL_VOLUME_QUERY_VOLUME_NUMBER to get VOLUME_NUMBER information

    // Use to keep track of Volume info from ntddvol.h
    WCHAR			StorageManagerName[8];		// L"PhysDisk", L"LogiDisk" else the value from VOLUME_NUMBER
														// PhyDisk means "\\Device\\Harddisk%d\\Partition%d",
														// LogiDisk means "\\Device\HarDiskVolume%d

	// IOCTL_STORAGE_GET_DEVICE_NUMBER to retrieve this info, Example : \\Device\HardDisk(n)\\Partition(n)
	// if disk is PhysDisk type then we get following info successfully.
	BOOLEAN			bStorage_device_Number;		// TRUE means Storage_device_Number is valid
	ULONG			DeviceType;
	ULONG			DeviceNumber;
	ULONG			PartitionNumber;
// 	STORAGE_DEVICE_NUMBER	StorageDeviceNumber;		// values retrieved from IOCTL_STORAGE_GET_DEVICE_NUMBER
	WCHAR			DiskPartitionName[128];		// Stores \\Device\HardDisk(n)\\Partition(n)

	// IOCTL_VOLUME_QUERY_VOLUME_NUMBER to retrieve HarddiskVolume number and its volumename like logdisk, etc..
	// if disk is LogiDisk type then we get following info successfully.
	BOOLEAN			bVolumeNumber;			// TRUE means VolumeNumber is valid
	ULONG			VolumeNumber;
    WCHAR			VolumeManagerName[8];
// 	VOLUME_NUMBER	VolumeNumber;			// IOCTL_VOLUME_QUERY_VOLUME_NUMBER
	
	// IOCTL_MOUNTDEV_QUERY_DEVICE_NAME used to retrieve \Device\HarddiskVolume1 into DiskVolumeName...
	BOOLEAN			bDiskVolumeName;		// TRUE means DiskVolumeName has valid value
	WCHAR			DiskVolumeName[128];	// IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returns string in DiskVolumeName	
													// Example : \Device\HarddiskVolume1 or \DosDevices\D or "\DosDevices\E:\FilesysD\mnt
	// following fields are supported only >= Win2k OS 
	// IOCTL_MOUNTDEV_QUERY_UNIQUE_ID used to retrieve Volume/Disk Unique Persistence ID, 
	// It Contains the unique volume ID. The format for unique volume names is "\??\Volume{GUID}\", 
	// where GUID is a globally unique identifier that identifies the volume.
	BOOLEAN			bUniqueVolumeId;	// TRUE means UniqueIdInfo Fields values are valid
	USHORT			UniqueIdLength;
	UCHAR			UniqueId[256];
	// PMOUNTDEV_UNIQUE_ID				UniqueIdInfo;		// Example: For instance, drive letter "D" must be represented in this manner: "\DosDevices\D:". 
	
	// following fields are supported only >= Win2k OS 
	// IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME is Optional returns Drive letter (if Drive Letter is persistent across boot) or
	// suggest Drive Letter Dos Sym,bolic Name.
	BOOLEAN			bSuggestedDriveLetter;	// TRUE means All Following Fields values are valid
	BOOLEAN			UseOnlyIfThereAreNoOtherLinks;
    USHORT			NameLength;
//	PMOUNTDEV_SUGGESTED_LINK_NAME	SuggestedDriveLinkName;	// Example: For instance, drive letter "D" must be represented in this manner: "\DosDevices\D:". 
	WCHAR			SuggestedDriveName[256];

	// Store Customize Signature GUID which gets used as alternate Volume GUID only if bUniqueVolumeID is not valid
	// Format is "volume(nnnnnnnn-nnnnnnnnnnnnnnnn-nnnnnnnnnnnnnnnn)"	: volume(Disksignature-StartingOffset-SizeInBytes)
	// Our customize alternate Disk Signature based Unique ID for Volume (Raw Disk/ Disk Partition)
	BOOLEAN			bSignatureUniqueVolumeId;	// TRUE means SignatureUniqueIdLength and SignatureUniqueId has valid values
	USHORT			SignatureUniqueIdLength;
    UCHAR			SignatureUniqueId[128];			// 128 is enough, if requires bump up this value.

	BOOLEAN			IsVolumeFtVolume; // TRUE means StorageManagerName == FTDISK && NumberOfDiskExtents > 0
	PVOID			pRawDiskDevice;	// Pointer to RAW Disk device of current partition or disk object

	ULONG			Signature;			// signature
	LARGE_INTEGER   StartingOffset;		// Starting Offset of Partition, if its RAW Disk than value is 0 
    LARGE_INTEGER   PartitionLength;	// Size of partition, if its RAW Disk than value i

	PVOID			SftkDev;	// if attached its non null value
	ULONG			cdev;
	ULONG			LGNum;
	
} ATTACHED_DISK_INFO, *PATTACHED_DISK_INFO;

// DISK_DEVICE_INFO structure will be used per LG 
typedef struct ATTACHED_DISK_INFO_LIST
{
	ULONG				NumOfDisks;
	ATTACHED_DISK_INFO	DiskInfo[1];

} ATTACHED_DISK_INFO_LIST, *PATTACHED_DISK_INFO_LIST;

#endif // SAUMYA_FIX_DEVICE_UNIQUE_ID

//MMP_MNGT_ALERT_DATA
typedef struct __mmp_TdmfAlertHdr {
    char            szServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];          //name of machine identifying the TDMF Agent.
    short           sLGid;                  //number identifying the logical group (0 to 999)
    short           sDeviceId;              //DTC number
    unsigned int    uiTimeStamp;            //GMT date-time , nbr of seconds since 01-01-1970
    char            cSeverity;              //1= highest severity, 5 = lowest severity
    char            cType;                  //message Type : refer to enum tdmf_alert_type
    //two zero-terminated strings follow this structure
    //the first is a string identifying the Module generating the message.
    //the second is a string containing the text message.
} mmp_TdmfAlertHdr;

enum tdmf_alert_type
{
 MMP_MNGT_MONITOR_TYPE_INFORMATION  = 0
,MMP_MNGT_MONITOR_TYPE_WARNING
,MMP_MNGT_MONITOR_TYPE_ERROR
};


//MMP_MNGT_STATUS_MSG
typedef struct __mmp_TdmfStatusMsg {
    char            szServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];          //name of machine identifying the TDMF Agent.
    char            cPriority;      //can be LOG_INFO, LOG_WARNING, LOG_CRIT, LOG_ERR
    char            cTag;           //one of the TAG_... value
    char            pad[2];         //in order so that the sizeof() value be a multiple of 8
    int             iTdmfCmd;       //tdmf command to which this message is related to.  0 if irrelevant.
    int             iTimeStamp; 
    int             iLength;        //length of message string following this structure, including terminating '\0' character.
} mmp_TdmfStatusMsg;


//MMP_MNGT_PERF_MSG
//  Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
typedef struct __mmp_TdmfPerfData {
    char            szServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];  //name of machine identifying the TDMF Agent.
    int             iPerfDataSize;                              //size of performance data (in bytes) following this structure.
    //the performance data is in the form of a number of ftd_perf_instance_t structures (iPerfDataSize / sizeof(ftd_perf_instance_t))
    //If, for the reception peer, the result of iPerfDataSize / sizeof(ftd_perf_instance_t)  has a remainder ( iPerfDataSize % sizeof(ftd_perf_instance_t) != 0 ), 
    //it could mean that both peers are not synchronized on the ftd_perf_instance_t struct. definition.
} mmp_TdmfPerfData;

//MMP_MNGT_GROUP_STATE
//  Sent by TDMF Agents to TDMF Collector.  Collector saves into the TDMF DB.
typedef struct __mmp_TdmfGroupState {
    short       sRepGrpNbr; //valid number have values >= 0.
    short       sState;     //bit0 : 0 = group stopped, 1 = group started.
} mmp_TdmfGroupState;

//MMP_MNGT_PERF_CFG_MSG
typedef struct __mmp_TdmfPerfConfig {
    //all periods specified in tenth of a second (0.1 sec) units
    //any values <= 0 will be considered as non-initialized and will not be refreshed by TDMF Agents
    int     iPerfUploadPeriod;      //period at which the TDMF Agent acquires the Performence data
    int     iReplGrpMonitPeriod;    //period at which the TDMF Agent acquires all Replication Groups Moinitoring Data
    int     notused[12];            //for future use
} mmp_TdmfPerfConfig;


//available Tdmf commands.  to be used when calling mmp_mngt_sendTdmfCommand()
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
,MMP_MNGT_TDMF_CMD_OS_CMD_EXE //allows to launch cmd.exe commands on a TDMF Server/Agent
,MMP_MNGT_TDMF_CMD_TESTTDMF  //a simple test program
,MMP_MNGT_TDMF_CMD_TRACE
,MMP_MNGT_TDMF_CMD_HANDLE
,MMP_MNGT_TDMF_CMD_PANALYZE  //from 2.1.6 Merge
,LAST_TDMF_CMD          = MMP_MNGT_TDMF_CMD_PANALYZE
,INVALID_TDMF_CMD//beyond cmd list
};

enum tdmf_commands_status
{
 MMP_MNGT_TDMF_CMD_STATUS_OK      =   0
,MMP_MNGT_TDMF_CMD_STATUS_ERROR
,MMP_MNGT_TDMF_CMD_STATUS_ERR_COULD_NOT_REACH_TDMF_AGENT
};


enum tdmf_filetype
{
 MMP_MNGT_FILETYPE_TDMF_CFG      =   0
,MMP_MNGT_FILETYPE_TDMF_EXE
,MMP_MNGT_FILETYPE_TDMF_BAT
,MMP_MNGT_FILETYPE_TDMF_ZIP
,MMP_MNGT_FILETYPE_TDMF_TAR
};



//MMP_MNGT_MONITORING_DATA_REGISTRATION
typedef struct __mmp_TdmfMonitoringCfg {

    unsigned int    iMaxDataRefreshPeriod;  //maximum time between consecutive MMP_MNGT_MONITORING_DATA sent to 
                                            //time period expressed in seconds 

} mmp_TdmfMonitoringCfg;


//MMP_MNGT_GROUP_MONITORING
typedef struct __mmp_TdmfReplGroupMonitor {
    char            szServerUID[ MMP_MNGT_MAX_MACHINE_NAME_SZ ]; //unique Host ID (in hexadecimal text format) of the TDMF Agent.

    //64-bit integer value
    __int64         liActualSz;      //Total size (in bytes) of PStore file or Journal file for this group.
    __int64         liDiskTotalSz;   //Total size (in MB) of disk on which PStore file or Journal file is maintained.
    __int64         liDiskFreeSz;    //Total free space size (in MB) of disk on which PStore file or Journal file is maintained.

    int             iReplGrpSourceIP;   //IP address of the Source 
    short           sReplGrpNbr;        //replication group number, >= 0
    char            isSource;           //indicates if this server is the SOURCE (primary) or TARGET (secondary) server of this replication group

    char            notused1[1];        //for future use, back to a 4-byte boundary
    char            notused2[256];      //for future use

} mmp_TdmfReplGroupMonitor;

//MMP_MNGT_SET_DB_PARAMS
//MMP_MNGT_GET_DB_PARAMS
typedef struct __mmp_TdmfCollectorParams {
    //the Collector stores the following values in the Tdmf DB t_SystemParameters table
    int     iCollectorTcpPort;          // Collector listening port nbr

    int     iDBPerformanceTableMaxNbr;  //Limits the number of records in the TDMF DB t_Performance table to this value. A value of 0 disables this restriction.
    int     iDBPerformanceTableMaxDays; //Limit the number of records in the TDMF DB t_Performance table based on record time_stamp. An value of 0 disables this resctriction.
    int     iDBAlertStatusTableMaxNbr;  //Limits the number of records in the TDMF DB t_Alert_Status table to this value. A value of 0 disables this restriction.
    int     iDBAlertStatusTableMaxDays; //Limit the number of records in the TDMF DB t_Alert_Status table based on record time_stamp. An value of 0 disables this resctriction.

    int     iDBCheckPeriodMinutes;      //Period at which TDMF tables restrictions are checked and enforced by Collector. Value units are minutes. 

	BOOL    bValidKey;
    int     iCollectorBroadcastIPMask;  //(feature not implemented) mask used to perform an IP broadcast
    int     iCollectorBroadcastTimeout; //(feature not implemented) min period before closing RX of broadcast responses

    int     iTimeOut1; //(feature not implemented) Timeout setting for the collector
    int     iTimeOut2; //(feature not implemented) Timeout setting for the collector
    int     iTimeOut3; //(feature not implemented) Timeout setting for the collector
    int     iTimeOut4; //(feature not implemented) Timeout setting for the collector
    int     iTimeOut5; //(feature not implemented) Timeout setting for the collector
    int     iTimeOut6; //(feature not implemented) Timeout setting for the collector
    int     iTimeOut7; //(feature not implemented) Timeout setting for the collector
    int     iTimeOut8; //(feature not implemented) Timeout setting for the collector
    int     iTimeOut9; //(feature not implemented) Timeout setting for the collector
    int     iTimeOut10; //(feature not implemented) Timeout setting for the collector

} mmp_TdmfCollectorParams;


enum tdmf_agent_state
{
     AGENT_STATE_ALIVE   =   0      //Tdmf Agent is on-line and connected to Collector
    ,AGENT_STATE_NORMAL  =   0
    ,AGENT_STATE_UNINSTALLED        //Tdmf Agent reports it is about to be uninstalled
    ,AGENT_STATE_NOT_ALIVE          //Tdmf Agent is not detected by Collector has being on-line
};
//MMP_MNGT_AGENT_STATE
typedef struct __mmp_TdmfAgentState {
    char    szServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];  //name of machine identifying the TDMF Agent.
    int     iState;                                     //enum tdmf_agent_state
    char    notusedyet[64];                             //for future use    
} mmp_TdmfAgentState;

typedef struct __mmp_TdmfGuiMsg {
    char szMsg[MMP_MNGT_MAX_GUI_MSG_SZ];
} mmp_TdmfGuiMsg;

typedef struct __mmp_TdmfMessagesStates {
    //
    //      Types of collector messages being treated
    //
    DWORD       Nb_MMP_MNGT_SET_LG_CONFIG;
    DWORD       Nb_MMP_MNGT_GET_LG_CONFIG;
    DWORD       Nb_MMP_MNGT_AGENT_INFO_REQUEST;
    DWORD       Nb_MMP_MNGT_AGENT_INFO;
    DWORD       Nb_MMP_MNGT_REGISTRATION_KEY;
    DWORD       Nb_MMP_MNGT_TDMF_CMD;
    DWORD       Nb_MMP_MNGT_SET_AGENT_GEN_CONFIG;
    DWORD       Nb_MMP_MNGT_GET_AGENT_GEN_CONFIG;
    DWORD       Nb_MMP_MNGT_SET_ALL_DEVICES;
    DWORD       Nb_MMP_MNGT_GET_ALL_DEVICES;
    DWORD       Nb_MMP_MNGT_ALERT_DATA;
    DWORD       Nb_MMP_MNGT_STATUS_MSG;
    DWORD       Nb_MMP_MNGT_PERF_MSG;
    DWORD       Nb_MMP_MNGT_PERF_CFG_MSG;
    DWORD       Nb_MMP_MNGT_MONITORING_DATA_REGISTRATION;
    DWORD       Nb_MMP_MNGT_AGENT_ALIVE_SOCKET;
    DWORD       Nb_MMP_MNGT_GROUP_STATE;
    DWORD       Nb_MMP_MNGT_GROUP_MONITORING;
    DWORD       Nb_MMP_MNGT_TDMFCOMMONGUI_REGISTRATION;
    DWORD       Nb_MMP_MNGT_SET_DB_PARAMS;
    DWORD       Nb_MMP_MNGT_GET_DB_PARAMS;
    DWORD       Nb_MMP_MNGT_AGENT_STATE;
    DWORD       Nb_MMP_MNGT_GUI_MSG;
    DWORD       Nb_default;
} mmp_TdmfMessagesStates;

//MMP_MNGT_COLLECTOR_STATE
typedef struct __mmp_TdmfCollectorState {
    //        
    // Structure values
    // 
    time_t      CollectorTime;                          // Collector time stamp
    //
    // Actual statistics
    //
    //
    //  DB access
    //
    DWORD       NbPDBMsg;                               // Number Of Pending DataBase Messages
    DWORD       NbPDBMsgPerMn;                          // per minute
    DWORD       NbPDBMsgPerHr;                          // per hour
    //
    //  Collector messages received
    //
    DWORD       NbThrdRng;                              // Number Of Messages received by collector executing
    DWORD       NbThrdRngPerMn;                         // per minute
    DWORD       NbThrdRngHr;                            // per hour
    
    mmp_TdmfMessagesStates  TdmfMessagesStates;
    
    //
    //  Alive socket and messages related information
    //
    DWORD       NbAgentsAlive;
    DWORD       NbAliveMsgPerMn;
    DWORD       NbAliveMsgPerHr;

} mmp_TdmfCollectorState;

#pragma pack(pop)    

//avoid C++ name mangling problems when imported/used by C client program
#ifdef __cplusplus 
}
#endif

#endif //__LIBMNGTDEF_H_