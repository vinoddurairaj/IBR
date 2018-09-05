/*****************************************************************************
 *                                                                           *
 *  This software  is the licensed software of Fujitsu Software              *
 *  Technology Corporation                                                   *
 *                                                                           *
 *  Copyright (c) 2004 by Softek Storage Technology Corporation              *
 *                                                                           *
 *  THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF            *
 *  FUJITSU SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED             *
 *  UNDER LICENSE FROM FUJITSU SOFTWARE TECHNOLOGY CORPORATION               *
 *                                                                           *
 *****************************************************************************

 *****************************************************************************
 *                                                                           *
 *  Revision History:                                                        *
 *                                                                           *
 *  Date        By              Change                                       *
 *  ----------- --------------  -------------------------------------------  *
 *  04-27-2004  Parag Sanghvi   Initial version.                             *
 *                                                                           *
 *                                                                           *
 *****************************************************************************/
#include <winsock2.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <winioctl.h>
#include <lmerr.h>			// for GetErrorText()

#define TARGET_SIDE 1

#include "..\RFWDriver\ftdio.h"	// for IOCTL Defination to include

//
// -------- Strcuture Definations -------------
//
#define	INT_OPT		0
#define	FLOAT_OPT	1
#define	DOUBLE_OPT	2
#define	STRING_OPT	3


typedef enum COMMAND_ID
{
	CID_CpuInfo		= 0,	
	CID_MemInfo,
	CID_RescanDisks,
	
	CID_ConfigBegin,
	CID_ConfigEnd,
	CID_NewLG,
	CID_NewDev,
	CID_DelLG,
	CID_DelDev,
	CID_LGInfo,
	CID_LGStats,
	CID_GetState,
	CID_SetState,
	CID_DiskList,

	CID_AddSock,		
	CID_StartPmd,		
	CID_StopPmd,		
	CID_DelSock,		
	CID_EnableSock,	
	CID_DisableSock,	
	CID_ParseConfig,	
	CID_GetParms,		
	CID_SetParms,		
	CID_LGMonitor,		
	CID_MMMonitor,		

	CID_ConfigFromReg,

	CID_Help,
	CID_Help1,
	CID_Help2,

	CID_MAX_NULL,	

} COMMAND_ID, *PCOMMAND_ID;

typedef enum OPTION_ID
{
	ID_LGNum = 0,			
	ID_LGCreationRole,	// ROLE_TYPE PRIMARY = 0 , SECONDARY = 1
	ID_LGJournalPath,	// The Journal Path that will be used
	ID_PFile,			

	ID_DevNum,			
	ID_DevId,			
	ID_DevSize,		
	ID_vdevname,		

	ID_UniqueId,		
	ID_SignUniqueId,	

	ID_DriveLetter,	
	ID_lrdbSize,		
	ID_hrdbSize,		

	ID_State,			
	ID_SrcIP,			
	ID_DstIP,			
	ID_SrcPort,		
	ID_DstPort,		

	ID_Version,		
	ID_HostId,			
	ID_SystemName,		

	ID_RemoteDriveName,

	ID_MaxSize,		
	ID_MaxSendPkts,	
	ID_MaxRecvPkts,	

	ID_TrackingIoCount,			
	ID_MaxTransferUnit,			
	ID_NumOfAsyncRefreshIO,		
	ID_AckWakeupTimeout,			
	ID_throtal_refresh_send_pkts,	
	ID_throtal_commit_send_pkts,	
	ID_NumOfPktsSendAtaTime,		
	ID_NumOfPktsRecvAtaTime,		
	ID_NumOfSendBuffers,		// Added this configurable so that the number of send buffers can be adjusted at runtime
	ID_DebugLevel,					

	ID_TimeOut,					
	ID_TimeDelay,					

	ID_MAX_NULL,	// NULL ID 

} OPTION_ID, *POPTION_ID;

typedef	struct	_CMD_LINE_STRINGS
{
	PCHAR	CmdPrefix;
	PCHAR	OptPrefix;
	PCHAR	DelString;

}CMD_LINE_STRINGS, *PCMD_LINE_STRINGS;

#define	OPT_FLAG_NULL			0
#define	OPT_FLAG_FIRST_ENTRY	1
#define	OPT_FLAG_START_SECTION	2
#define	OPT_FLAG_END_SECTION	4
#define	OPT_FLAG_LAST_ENTRY		8

typedef	struct	_CMD_LINE_OPT
{
	UCHAR		Flags;
	ULONG		Index;
	PCHAR		OptStr;
	PCHAR		Desc;
	UCHAR		Type;
	BOOL		Present;
	BOOL		Value;

	union
	{
		DWORD	Int;
		float	Float;
		double	Double;
		PCHAR	String;
	};

}CMD_LINE_OPT, *PCMD_LINE_OPT;

#define	CMD_FLAG_NULL			0
#define	CMD_FLAG_FIRST_ENTRY	1
#define	CMD_FLAG_START_SECTION	2
#define	CMD_FLAG_END_SECTION	4
#define	CMD_FLAG_LAST_ENTRY		8

typedef	struct	_CMD_LINE_CMD
{
	UCHAR	Flags;		// See directly above for defines
	ULONG	CommandId;
	PCHAR	CmdStr;
	PCHAR	Desc;
	void	(*Func)(void);

}CMD_LINE_CMD, *PCMD_LINE_CMD;

extern CMD_LINE_OPT Options[];
extern CMD_LINE_CMD Commands[];

//
// ------ Function ProtoType defination defined in cli.c file
//
int		main(int argc, char *argv[]);

void	PrintDesc(PCHAR String, DWORD Length, DWORD Start, DWORD End);

void	CHAR_Help(void);	// character based help display
void	Display_String( PCHAR String, 
						DWORD DisplaySize, 
						DWORD StartScreen,	// relative to 0
						DWORD ScreenWidth,	// relative to 1
						DWORD *CurrentScreen,	// relative to 0
						PCHAR  Prefix);	
void	Help(void);	// Graphic based help display
VOID	GetErrorText();

//
// ------ Function ProtoType defination defined in ioctl.c file
//
BOOL	OpenDevice(HANDLE *pHandle, CHAR * DeviceName);
VOID	CloseDevice(HANDLE Handle);

BOOL SendIoctl( HANDLE Handle,			DWORD dwIoControlCode, 
				LPVOID lpInBuffer,		DWORD nInBufferSize,
				LPVOID lpOutBuffer,		DWORD nOutBufferSize,
				LPDWORD lpBytesReturned);



//
// ------ Function ProtoType defination defined in cliParser.c file
//
VOID			CLIParseCmdLine(IN PCMD_LINE_STRINGS CmdStrs,
								IN PCMD_LINE_CMD Cmds,
								IN PCMD_LINE_OPT Opts,
								IN DWORD argc,
								IN CHAR **argv);
PCMD_LINE_OPT	CLIGetOpt(	IN PCHAR OptStr);
VOID			CLIGetXY(	OUT PDWORD Col,
							OUT PDWORD Row);
VOID			CLISetXY(	IN DWORD Col,
							IN DWORD Row);
BOOL			CLIGetConsoleInfo( IN PCONSOLE_SCREEN_BUFFER_INFO ConsoleInfo);
VOID			SetOptions(DWORD argc, char **argv, DWORD Index);

//
// ********* Followings are APIs used for CLI command line *************
//

//
// ------ Function ProtoType defined in system.c file
//
/*
 * Function: 		
 *		VOID		CPUInfo(void);
 *
 * Command Line:	/cpuinfo
 *			
 * Arguments: 	none
 *	
 * Returns: It retrieves CPU Architecture and displays CPU Count
 *
 * Conclusion:	It displays information about the current computer system. 
 *				This includes the architecture and type of the processor, 
 *				the number of processors in the system, the page size, and
 *				other such information. 
 *
 * Description:
 *			This Function Calls GetSystemInfo() and displays informations.
 */
VOID CPUInfo(void);


/*
 * Function: 		
 *		VOID		MemoryInfo(void);
 *
 * Command Line:	/meminfo
 *			
 * Arguments: 	none
 *	
 * Returns: It retrieves information about the system's current usage of 
 *			both physical and virtual memory. 
 *
 * Conclusion:	It displays information about the system's current usage of 
 *				both physical and virtual memory. 
 *
 * Description:
 *			This Function Calls GlobalMemoryStatus() and displays informations.
 */
VOID MemoryInfo(void);

/*
 * Function: 		
 *		VOID RescanDisks(VOID)
 *
 * Command Line:	/RescanDisks
 *			
 * Arguments: 	none
 *	
 * Returns: It rescan Scsi Ports to discovers any new disk devices.
 *
 * Conclusion:	It Rescan each and every Scsi Port and issues 
 *				IOCTL_DISK_FIND_NEW_DEVICES for each and every disk 
 *				device exist on the local system
 *
 * Description:
 *			This Function Opens all "\\\\.\\scsi%d:" devices and sends
 *			IOCTL_SCSI_RESCAN_BUS and for each & every disk devices: 
 *			"\\\\.\\PhysicalDrive%d" it sends IOCTL_DISK_FIND_NEW_DEVICES.
 */
VOID RescanDisks(VOID);

// Internal API 
LONG RescanScsiBus();
BOOLEAN DiskIoctlNewDisk();
BOOLEAN IsDiskAdministratorRunning();




//
// ------ Function ProtoType defined in TDMF.c file
//

/*
 *  Function: 		
 *		void	ConfigBegin(void)
 *
 *  Command Line:	/ConfigBegin 
 *		
 *  Arguments: 	
 * 	
 * Returns: Send FTD_CONFIG_BEGIN IOCTL to driver
 * 			
 * Conclusion: 
 *
 * Description:
 *		Send FTD_CONFIG_BEGIN IOCTL to driver.
 *		-Driver uses these as start point for Driver's LG and Dev configurtation updates
 *		-Driver marks all its existing LG and Dev with Config Begin starts
 *		-When Driver gets LG create or DEV Create, it marks repective LG or dev mentioning 
 *		 Updated.
 *		-Finally caller has to call CONFIGEnd IOCTL, At that time Driver goes thru 
 *		 each and every untouched LG and Devices, and deletes that automatically !!
 */
void	ConfigBegin(void);

/*
 *  Function: 		
 *		void	ConfigEnd(void)
 *
 *  Command Line:	/ConfigEnd 
 *		
 *  Arguments: 	
 * 	
 * Returns: Send FTD_CONFIG_END IOCTL to driver to End Configuration Process
 * 			
 * Conclusion: 
 *
 * Description:
 *		Send FTD_CONFIG_END IOCTL to driver.
 *		-Driver uses these as start point for Driver's LG and Dev configurtation updates
 *		-Driver marks all its existing LG and Dev with Config Begin starts
 *		-When Driver gets LG create or DEV Create, it marks repective LG or dev mentioning 
 *		 Updated.
 *		-Finally caller has to call CONFIGEnd IOCTL, At that time Driver goes thru 
 *		 each and every untouched LG and Devices, and deletes that automatically !!
 */
void	ConfigEnd(void);

/*
 *  Function: 		
 *		void	ConfigFromReg(void)
 *
 *  Command Line:	/ConfigFromReg
 *		
 *  Arguments: 	
 * 	
 * Returns: Send IOCTL: FTD_RECONFIGURE_FROM_REGISTRY to driver to ReRead and Reconfigure
 *			Driver from Registries. This will test Driver's Boot time init code.
 * 			
 * Conclusion: 
 *
 * Description:
 *		Send IOCTL: FTD_RECONFIGURE_FROM_REGISTRY to driver.
 *		-Driver Reads registry and creates (if not already created) LG and devices.
 */
void	ConfigFromReg(void);

/*
 *  Function: 		
 *		void	NewLG(void)
 *
 *  Command Line:	/NewLG <-LGNum n> <-PFile s>
 *		
 *  Arguments: 	<-LGNum n>  : n is Logical Group Number
 *				<-PFile s>  : s is Pstore File name including complete path
 * 	
 * Returns: Create Specify LG at driver level.
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function prepares struct ftd_lg_info_t and sends it to driver
 * 		to create LG device. To do this it also has to create Empty Pstore file.
 *
 *		It uses IOCTL : FTD_NEW_LG to send it to CTL_DEVICE
 */
void	NewLG(void);

/*
 *  Function: 		
 *		void	NewDev(void)
 *
*  Command Line:	/NewDev <-LGNum n> <-DevNum n> [<-DevId s>] 
 *							<-DevSize s> <-RemoteDriveName s>
 *							< [-vdevname s] | [-UniqueId s] | [-DriveLetter s] >
 *							[-SignUniqueId s]
 *							[-lrdbSize n] [-hrdbSize n]
 *		
 *  Arguments: 	<-LGNum n>			: n is Logical Group Number
 *				<-DevNum n>			: n is unique Device Number under LG
 *				<-DevId s>			: s is Unique string to identify Device under LG
 *				<-DevSize s>		: s is Device Size in bytes.
 *				<-RemoteDriveName s>: s is Remote secondary Drive Name pair. Example: 'F:' 
 *				<-vdevname s>		: s is '\Device\HardDiskVolume(n)' where n is Partition Volume number
 *				<-UniqueId s>		: s is '\??\Volume{GUID}\' where GUID is numbers
 *				<-SignUniqueId s>	: s is 'volume(nnnnnnnn-nnnnnnnnnnnnnnnn-nnnnnnnnnnnnnnnn)'
 *									  volume(Signature-StartingOffset.QuadPart-PartitionLength.QuadPart)
 *				<-DriveLetter s>	: s is like '\??\D:' where D can be any drive letter
 *				<-lrdbSize n>		: n is in bytes of LRDB Bitmap Size Defualt is 8K
 *				<-hrdbSize n>		: n is in bytes of HRDB Bitmap Size Defualt is 128K
 * 	
 * Returns: Create Specify Device under specify LG.
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function prepares struct ftd_dev_info_t and sends it to driver
 * 		to create Device under LG device. 
 *
 *		It uses IOCTL : FTD_NEW_DEVICE to send it to CTL_DEVICE
 */
void	NewDev(void);


/*
 *  Function: 		
 *		void	DelLG(void)
 *
 *  Command Line:	/DelLG <-LGNum n> 
 *		
 *  Arguments: 	<-LGNum n>  : n is Logical Group Number
 * 	
 * Returns: Delete Specify SFTK_LG at driver level. input: <-LGNum n>
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function sends FTD_DEL_LG IOCTL to driver to delete specify LG 
 */
void	DelLG(void);

/*
 *  Function: 		
 *		void	DelDev(void)
 *
 *  Command Line:	/DelDev <-LGNum n> <-DevNum n>
 *		
 *  Arguments: 	<-LGNum n>  : n is Logical Group Number
 *				<-DevNum n>	: n is unique Device Number under LG
 * 	
 * Returns: Delete Specify Dev under specify LG at driver level
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function sends FTD_DEL_DEVICE IOCTL to driver to delete specify LG 
 */
void	DelDev(void);

/*
 *  Function: 		
 *		void	LGInfo(void)
 *
 *  Command Line:	/LGInfo [-LGNum n]
 *		
 *  Arguments: 	[-LGNum n]  : n is Logical Group Number
 *				<-DevNum n>	: n is unique Device Number under LG
 * 	
 * Returns: Display specified or ALL LG information along with its Devices and statistics info
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function sends FTD_DEL_DEVICE IOCTL to driver to delete specify LG 
 */
void	LGInfo(void);

/*
 *  Function: 		
 *		void	GetState(void)
 *
 *  Command Line:	/GetState <-LGNum n> 
 *		
 *  Arguments: 	<-LGNum n>  : n is Logical Group Number
  * 	
 * Returns: Get specified LG cuurent state mode
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function gets Specified LG's current state mode from driver.
 *		It uses IOCTL : FTD_GET_GROUP_STATE to send it to CTL_DEVICE
 */
void	GetState(void);

/*
 *  Function: 		
 *		void	SetState(void)
 *
 *  Command Line:	/SetState <-LGNum n> <-ID_State n>
 *		
 *  Arguments: 	<-LGNum n>  : n is Logical Group Number
 *				<-State n>  : n is State Mode. 1: PASSTHRU, 2: TRACKING, 3: NORMAL, 4: FULL_REFRESH, 5: SMART_REFRESH
 * 	
 * Returns: Set LG new specified state mode
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function changes Specified LG's state in driver.
 *		It uses IOCTL : FTD_SET_GROUP_STATE to send it to CTL_DEVICE
 */
void	SetState(void);

/*
 *  Function: 		
 *		void	LGStats(void)
 *
 *  Command Line:	/LGStats [ <-LGNum n> [-DevNum n]] 
 *		
 *  Arguments: 	[-LGNum n]  : n is Logical Group Number
 *				[-DevNum n]	: n is unique Device Number under LG
 * 	
 * Returns: Display specified or ALL LG's and their Devices Statistics information 
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function sends FTD_GET_ALL_STATS_INFO/or FTD_GET_LG_STATS_INFO IOCTL to driver to get stats info 
 */
void	LGStats(void);

/*
 * Function: 		
 *		void	LGMonitor(void)
 *
 * Command Line:	/LGMonitor	<-LgNum n> [<-timedelay n>] [<-timeout n>] 
 *		
 * Arguments: 	-timeout n		- n is value in seconds. This much seconds
 *								  it monitors data continusly. Defualt is INDEFINITE.
 *				-timedelay n	- n is value in miliseconds. This much miliseconds
 *								  gets delay between data retrival. Defualt is 1 Sec
 *	
 * Returns: It Continuosuly Collects LG Statistics/Perfromance data and display till specify
 *			time get expired.
 *
 * Conclusion: 
 *
 * Description:
 *		This Function sends FTD_GET_ALL_STATS_INFO/or FTD_GET_LG_STATS_INFO IOCTL to driver to get stats info 
 */
void	LGMonitor(void);

/*
 * Function: 		
 *		void	MMMonitor(void)
 *
 * Command Line:	/MMMonitor	<-LgNum n> [<-timedelay n>] [<-timeout n>] 
 *		
 * Arguments: 	-timeout n		- n is value in seconds. This much seconds
 *								  it monitors data continusly. Defualt is INDEFINITE.
 *				-timedelay n	- n is value in miliseconds. This much miliseconds
 *								  gets delay between data retrival. Defualt is 1 Sec
 *	
 * Returns: It Continuosuly Collects MM Statistics/Perfromance data and display till specify
 *			time get expired.
 *
 * Conclusion: 
 *
 * Description:
 *		This Function sends FTD_GET_ALL_STATS_INFO/or FTD_GET_LG_STATS_INFO IOCTL to driver to get stats info 
 */
void	MMMonitor(void);

/*
 *  Function: 		
 *		void	GetParms(void)
 *
 *  Command Line:	/GetParms <-Lgnum n> 
 *
 *  Arguments: 	
 * 	
 * Returns: Display Driver's Specified LG Tunning Parms Information. input: <-Lgnum n>
 * 			
 * Conclusion: 
 *
 * Description:
 *		Send FTD_GET_LG_TUNING_PARAM IOCTL to driver.
 */
void	GetParms(void);

/*
 *  Function: 		
 *		void	SetParms(void)
 *
 *  Command Line:	/SetParms <-Lgnum n> [-TrackingIoCount n] [-MaxTransferUnit n] [-NumOfAsyncRefreshIO n] [-AckWakeupTimeout n] 
 *							  [-throtal_refresh_send_pkts n] [-throtal_commit_send_pkts n]
 *  Arguments: 	
 *				[-TrackingIoCount n]			: n is MAX_IO_FOR_TRACKING_TO_SMART_REFRESH than we change the state, Default is 10
 *				[-MaxTransferUnit n]			:n is Maximum Raw Data Size which gets transfered to secondary., Default is 256K
 *				[-NumOfAsyncRefreshIO n]		:n is Maximum Number of Refresh Async Read IO allowed, Default is 5
 *				[-AckWakeupTimeout n]			:n is Timeout used to wake up Ack thread and prepare its LRDB or HRDB, Default is 60 secs
 *				[-throtal_refresh_send_pkts n]  :n is the max send pkts sync during refresh, Default is 100
 *				[-throtal_commit_send_pkts n]	:n is the max send pkts sync during Commit, Default is 400
 * 	
 * Returns: Set specified Tunning Parms For Specified LG in Driver
 * 			
 * Conclusion: 
 *
 * Description:
 *		Send FTD_SET_LG_TUNING_PARAM IOCTL to driver.
 */
void	SetParms(void);

/*
 *  Function: 		
 *		void	DiskList(void)
 *
 *  Command Line:	/DiskList 
 *		
 *  Arguments: 	none
 * 	
 * Returns: Display List of all Disk Device Attached by Driver and its informations
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function sends FTD_GET_All_ATTACHED_DISKINFO to driver to get All Attached Disk Info List
 */
void	DiskList (void);

/*
 *  Function: 		
 *		void	AddSock(void)
 *
 *  Command Line:	/AddSock <-LGNum n> <-SrcIP s> <-SrcPort n> <-DstIP s> <-DstPort n>
 *		
 *  Arguments: 	<-LGNum n>  : n is Logical Group Number
 *				<-SrcIP s>	: s is IP Address String. Example: 129.212.65.20
 *				<-DstIP s>	: s is IP Address String. Example: 129.212.65.20
 *				<-SrcPort n>: n is Socket Port Number. Example: 575
 *				<-DstPort n>: n is Socket Port Number. Example: 575
 * 	
 * Returns: Add New Socket Info per Logical Group.
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function sends SFTK_IOCTL_TCP_ADD_CONNECTIONS to driver. 
 */
void	AddSock(void);

/*
 *  Function: 		
 *		void	StartPmd(void)
 *
 *  Command Line:	/StartPmd <-LGNum n> [-MaxSize n] [-MaxSendPkts n] [-MaxRecvPkts n]
 *		
 *  Arguments: 	<-LGNum n>		: n is Logical Group Number
 *				[-MaxSize n]	: n is Max Transfer Size, Default is 256 K bytes
 *				[-MaxSendPkts n]: n is Max Send Buffers, Default is 5
 *				[-MaxRecvPkts n]: n is Max Recv Buffers, Default is 5
 * 	
 * Returns: Starts Driver's PMD Connection Thread.
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function sends SFTK_IOCTL_START_PMD to driver. 
 */
void	StartPmd(void);

/*
 *  Function: 		
 *		void	StopPmd(void)
 *
 *  Command Line:	/StopPmd <-LGNum n> [-MaxSize n] [-MaxSendPkts n] [-MaxRecvPkts n]
 *		
 *  Arguments: 	<-LGNum n>		: n is Logical Group Number
 *				[-MaxSize n]	: n is Max Transfer Size, Default is 256 K bytes
 *				[-MaxSendPkts n]: n is Max Send Buffers, Default is 5
 *				[-MaxRecvPkts n]: n is Max Recv Buffers, Default is 5
 * 	
 * Returns: Starts Driver's PMD Connection Thread.
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function sends SFTK_IOCTL_STOP_PMD to driver. 
 */
void	StopPmd(void);

/*
 *  Function: 		
 *		void	DelSock(void)
 *
 *  Command Line:	/DelSock <-LGNum n> <-SrcIP s> <-SrcPort n> <-DstIP s> <-DstPort n>
 *		
 *  Arguments: 	<-LGNum n>  : n is Logical Group Number
 *				<-SrcIP s>	: s is IP Address String. Example: 129.212.65.20
 *				<-DstIP s>	: s is IP Address String. Example: 129.212.65.20
 *				<-SrcPort n>: n is Socket Port Number. Example: 575
 *				<-DstPort n>: n is Socket Port Number. Example: 575
 * 	
 * Returns: Delete Socket Info per Logical Group.
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function sends SFTK_IOCTL_TCP_REMOVE_CONNECTIONS to driver. 
 */
void	DelSock(void);

/*
 *  Function: 		
 *		void	EnableSock(void)
 *
 *  Command Line:	/EnableSock <-LGNum n> <-SrcIP s> <-SrcPort n> <-DstIP s> <-DstPort n>
 *		
 *  Arguments: 	<-LGNum n>  : n is Logical Group Number
 *				<-SrcIP s>	: s is IP Address String. Example: 129.212.65.20
 *				<-DstIP s>	: s is IP Address String. Example: 129.212.65.20
 *				<-SrcPort n>: n is Socket Port Number. Example: 575
 *				<-DstPort n>: n is Socket Port Number. Example: 575
 * 	
 * Returns: Enable Existing Socket Info per Logical Group.
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function sends SFTK_IOCTL_TCP_ENABLE_CONNECTIONS to driver. 
 */
void	EnableSock(void);
/*
 *  Function: 		
 *		void	DisableSock(void)
 *
 *  Command Line:	/DisableSock <-LGNum n> <-SrcIP s> <-SrcPort n> <-DstIP s> <-DstPort n>
 *		
 *  Arguments: 	<-LGNum n>  : n is Logical Group Number
 *				<-SrcIP s>	: s is IP Address String. Example: 129.212.65.20
 *				<-DstIP s>	: s is IP Address String. Example: 129.212.65.20
 *				<-SrcPort n>: n is Socket Port Number. Example: 575
 *				<-DstPort n>: n is Socket Port Number. Example: 575
 * 	
 * Returns: Disable Existing Socket Info per Logical Group.
 * 			
 * Conclusion: 
 *
 * Description:
*		This Function sends SFTK_IOCTL_TCP_DISABLE_CONNECTIONS to driver. 
 */
void	DisableSock(void);

/*
 *  Function: 		
 *		void	ParseConfigFiles(void)
 *
 *  Command Line:	/ParseConfigFiles 
 *		
 *  Arguments: 	
 *				
 *				
 *				
 *				
 * 	
 * Returns: 
 * 			
 * Conclusion: 
 *
 * Description:
 *		This Function just returns all the config files for all Logical Groups. 
 */
void	ParseConfigFiles(void);

//
// -------------- Internal APIs definations ------------------------------
//
VOID
DisplayLGStatsInfo( PLG_STATISTICS		LgStats	);

VOID 
DisplayStatsInfo( PSTATISTICS_INFO StatsInfo );

VOID 
DisplayDiskInfo( PATTACHED_DISK_INFO AttachDiskInfo );

VOID
DisplayDevStatsInfo( PDEV_STATISTICS	DevStats );

VOID
GetLGStatsInString( PCHAR String, ULONG State);

/*
 *  Function: 		
 *		ULONG	Sftk_Get_TotalLgCount()
 *
 *  Arguments: 	
 * 				...
 * Returns: returns Total Number of LG configured in driver
 *
 * Description:
 *		sends FTD_GET_NUM_GROUPS to driver and retrieves Total Num of LG configured in driver
 */
ULONG
Sftk_Get_TotalLgCount(ROLE_TYPE RoleType);

/*
 *  Function: 		
 *		ULONG	Sftk_Get_TotalDevCount()
 *
 *  Arguments: 	
 * 				...
 * Returns: returns Total Number of Dev configured for all LG in driver
 *
 * Description:
 *		sends FTD_GET_DEVICE_NUMS to driver and retrieves Total Num of Devices exist for all LG in driver
 */
ULONG
Sftk_Get_TotalDevCount();

/*
 *  Function: 		
 *		ULONG	Sftk_Get_TotalLgDevCount()
 *
 *  Arguments: 	
 * 				...
 * Returns: returns Total Number of Devices for specified LG configured in driver
 *
 * Description:
 *		sends FTD_GET_NUM_GROUPS to driver and retrieves Total Num of LG configured in driver
 */
ULONG
Sftk_Get_TotalLgDevCount(ULONG LgNum, ROLE_TYPE RoleType);

/*
 *  Function: 		
 *		DWORD
 *		Sftk_Get_AllStatsInfo( PALL_LG_STATISTICS All_LgStats, ULONG Size )
 *
 *  Arguments: 	
 * 				...
 * Returns: returns NO_ERROR on sucess and All_LgStats has valid values else return SDK GetLastError()
 *
 * Description:
 *		sends FTD_GET_ALL_STATS_INFO to driver and retrieves All LG and their Devices Stats info from driver
 */
DWORD
Sftk_Get_AllStatsInfo( PALL_LG_STATISTICS All_LgStats, ULONG Size );

/*
 *  Function: 		
 *		DWORD
 *		Sftk_Get_LGStatsInfo( PLG_STATISTICS LgStats, ULONG Size )
 *
 *  Arguments: 	
 * 				...
 * Returns: returns NO_ERROR on sucess and All_LgStats has valid values else return SDK GetLastError()
 *
 * Description:
 *		sends FTD_GET_LG_STATS_INFO to driver and retrieves asking LG and their Devices Stats info from driver
 */
DWORD
Sftk_Get_LGStatsInfo( PLG_STATISTICS LgStats, ULONG Size );

/*
 *  Function: 		
 *		ULONG	
 *		Sftk_Get_LGInfo( ULONG LgNum, stat_buffer_t *Statebuff, ftd_lg_info_t *LgInfo)
 *
 *  Arguments: 	
 * 				...
 * Returns: returns NO_ERROR on sucess and LgInfo has valid values else return SDK GetLastError()
 *
 * Description:
 *		sends FTD_GET_GROUPS_INFO to driver and retrieves LG Information from driver
 */
DWORD	
Sftk_Get_LGInfo( ULONG LgNum, ROLE_TYPE RoleType, stat_buffer_t *Statebuff, ftd_lg_info_t *LgInfo);

/*
 *  Function: 		
 *		ULONG	
 *		Sftk_Get_LGState( ULONG LgNum, ftd_state_t *StateInfo)
 *
 *  Arguments: 	
 * 				...
 * Returns: returns NO_ERROR on sucess and StateInfo has valid values else return SDK GetLastError()
 *
 * Description:
 *		sends FTD_GET_GROUP_STATE to driver and LG current state from driver
 */
DWORD	
Sftk_Get_LGState( ULONG LgNum, ROLE_TYPE RoleType, ftd_state_t *StateInfo);

/*
 *  Function: 		
 *		ULONG	
 *		Sftk_Get_LGStatistics( ULONG LgNum, ftd_stat_t *StateInfo)
 *
 *  Arguments: 	
 * 				...
 * Returns: returns NO_ERROR on sucess and StateInfo has valid values else return SDK GetLastError()
 *
 * Description:
 *		sends FTD_GET_GROUP_STATE to driver and LG current statistics from driver
 */
DWORD	
Sftk_Get_LGStatistics( ULONG LgNum, ROLE_TYPE RoleType, ftd_stat_t *StateInfo);

/*
 *  Function: 		
 *		ULONG	
 *		Sftk_Get_LGDevStatistics( ULONG LgNum, ULONG Cdev,  disk_stats_t *Dev_stats)
 *
 *  Arguments: 	
 * 				...
 * Returns: returns NO_ERROR on sucess and Dev_stats has valid values else return SDK GetLastError()
 *
 * Description:
 *		sends FTD_GET_DEVICE_STATS to driver and get specified Dev of LG current statistics from driver
 */
DWORD	
Sftk_Get_LGDevStatistics( ULONG LgNum, ROLE_TYPE RoleType, ULONG Cdev,  disk_stats_t *Dev_stats);

/*
 *  Function: 		
 *		DWORD	sftk_DeviceIoControl(...)
 *
 *  Arguments: 	HandleDevice : Valid Handle else pass NULL or INVALID_HANDLE_VALUE
 *				DeviceName	 : Pass string to which IOControlCode need to send
 * 				...
 * Returns: Execute IOCTL on specify device
 *
 * Description:
 *		return NO_ERROR on success else returns SDK GetLastError() 
 */
DWORD
sftk_DeviceIoControl(	IN	HANDLE	HandleDevice,
						IN	PCHAR	DeviceName,
						IN	DWORD	IoControlCode,
						IN	LPVOID	InBuffer,
						IN	DWORD	InBufferSize,
						OUT LPVOID	OutBuffer,
						IN	DWORD	OutBufferSize,
						OUT LPDWORD	BytesReturned);
