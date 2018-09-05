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

#include "CLI.h"
#include "sftk_config.h"

#define SPACE_CHAR ' '
#define	HELP_START_COLUMN		5
#define PHYSICAL_DEVICE_NAME	"\\\\.\\PhysicalDrive%d"

#define MIN_LEGAL_SIZE			6
#define MIN_CONFIG_SIZE			16

//
// ------ Global Variable Definations	------ 
//

// Define command and option prefixes, and the deletion string.
CMD_LINE_STRINGS CmdPrefixes = {"/", "-", "-"};	// Command prefix is: /	
												// Option prefix is:  -
												// Deletion String:   -
// Define commands and their functions
CMD_LINE_CMD Commands[] = 
{
	{CMD_FLAG_FIRST_ENTRY | CMD_FLAG_START_SECTION,	CID_CpuInfo,	
								"cpuinfo",		"processor information",		CPUInfo},
	{CMD_FLAG_NULL,			CID_MemInfo, 		"meminfo",		"memory information",			MemoryInfo},
	{CMD_FLAG_END_SECTION,	CID_RescanDisks,	"RescanDisks",  "Rescan Disks on Local System",	RescanDisks},

	{CMD_FLAG_START_SECTION, CID_ConfigBegin,	"ConfigBegin",	"Send FTD_CONFIG_END IOCTL to driver to Start Configuration Process. input: <-Version s> <-HostId n> <-SystemName s>",	ConfigBegin},
	{CMD_FLAG_NULL,			 CID_ConfigEnd,		"ConfigEnd",	"Send FTD_CONFIG_END IOCTL to driver to End Configuration Process",	ConfigEnd},
	{CMD_FLAG_NULL,			 CID_NewLG,			"NewLG",		"Create Specify SFTK_LG at driver level. input: <-LGNum n> <-LGCreationRole n> <-LGJournalPath s> <-PFile s>",			NewLG},
	{CMD_FLAG_NULL,			 CID_NewDev,		"NewDev",		"Create Specify Device under specify LG. input: <-LGNum n> <-LGCreationRole n> <-DevNum n> [<-DevId s>] <-DevSize s> < [-vdevname s] | [-UniqueId s] | [-DriveLetter s] > [-SignUniqueId s] [-lrdbSize n] [-hrdbSize n]",	NewDev},
	{CMD_FLAG_NULL,			 CID_DelLG,			"DelLG",		"Delete Specify SFTK_LG at driver level. input: <-LGNum n>  <-LGCreationRole n>",			DelLG},
	{CMD_FLAG_NULL,			 CID_DelDev,		"DelDev",		"Delete Specify Dev under specify LG at driver level. input: <-LGNum n> <-DevNum n>",	DelDev},
	{CMD_FLAG_NULL,			 CID_LGInfo,		"LGInfo",		"Display specified or ALL LG information along with its Devices and statistics info. input: [-LGNum n] <-LGCreationRole n>",	LGInfo},
	{CMD_FLAG_NULL,			 CID_LGStats,		"LGStats",		"Display specified or ALL LG's and their Devices Statistics information. input: [<-LGNum n> <-LGCreationRole n> [-DevNum n]]",	LGStats},
	{CMD_FLAG_NULL,			 CID_GetState,		"GetState",		"Get specified LG cuurent state mode. input: <-LGNum n> <-LGCreationRole n>",	GetState},
	{CMD_FLAG_NULL,			 CID_SetState,		"SetState",		"Set LG new specified state mode. input: <-LGNum n> <-LGCreationRole n> <-State n>",	SetState},
	{CMD_FLAG_NULL,			 CID_DiskList,		"DiskList",		"Display List of all Disk Device Attached by Driver and its informations. input: none",	DiskList},

	{CMD_FLAG_NULL,			 CID_AddSock,		"AddSock",		"Add New Socket Info per Logical Group. input: <-LGNum n> <-LGCreationRole n> <-SrcIP s> <-SrcPort n> <-DstIP s> <-DstPort n>",	AddSock},
	{CMD_FLAG_NULL,			 CID_StartPmd,		"StartPmd",		"Starts Driver's PMD Connection Thread. input: <-LGNum n> <-LGCreationRole n> [-MaxSize n] [-MaxSendPkts n] [-MaxRecvPkts n]",	StartPmd},
	{CMD_FLAG_NULL,			 CID_StopPmd,		"StopPmd",		"Starts Driver's PMD Connection Thread. input: <-LGNum n> <-LGCreationRole n> [-MaxSize n] [-MaxSendPkts n] [-MaxRecvPkts n]",	StopPmd},
	{CMD_FLAG_NULL,			 CID_DelSock,		"DelSock",		"Deletes Socket Info per Logical Group. input: <-LGNum n> <-LGCreationRole n> <-SrcIP s> <-SrcPort n> <-DstIP s> <-DstPort n>",	DelSock},
	{CMD_FLAG_NULL,			 CID_EnableSock,	"EnableSock",	"Enable Existing Socket Info per Logical Group. input: <-LGNum n> <-LGCreationRole n> <-SrcIP s> <-SrcPort n> <-DstIP s> <-DstPort n>",	EnableSock},
	{CMD_FLAG_NULL,			 CID_DisableSock,	"DisableSock",	"Disable Existing Socket Info per Logical Group. input: <-LGNum n> <-LGCreationRole n> <-SrcIP s> <-SrcPort n> <-DstIP s> <-DstPort n>",	DisableSock},
	{CMD_FLAG_NULL,			 CID_ParseConfig,	"ParseConfig",	"Parse All the Config Files in the installation directory.",	ParseConfigFiles},
	{CMD_FLAG_NULL,			 CID_GetParms,		"GetParms",		"Display Driver's Specified LG Tunning Parms Information. input: <-Lgnum n> <-LGCreationRole n>",	GetParms},
	{CMD_FLAG_NULL,			 CID_SetParms,		"SetParms",		"Set specified Tunning Parms For Specified LG in Driver. input: <-Lgnum n> <-LGCreationRole n> [-TrackingIoCount n] [-MaxTransferUnit n] [-NumOfAsyncRefreshIO n] [-throtal_refresh_send_pkts n] [-throtal_commit_send_pkts n] [-DebugLevel n] [-NumOfPktsSendAtaTime n] [-NumOfPktsRecvAtaTime n] [-NumOfSendBuffers n]",	SetParms},
	{CMD_FLAG_NULL,			 CID_LGMonitor,		"LGMonitor",	"Continuosly Collects LG Statistics/Perfromance data and display till specify timeout. input: <-LgNum n> <-LGCreationRole n> [<-timedelay n>] [<-timeout n>] ",	LGMonitor},
	{CMD_FLAG_END_SECTION,	 CID_MMMonitor,		"MMMonitor",	"Continuosly Collects MM Statistics/Perfromance data and display till specify timeout. input: <-LgNum n> <-LGCreationRole n> [<-timedelay n>] [<-timeout n>] ",	MMMonitor},
	

	{CMD_FLAG_START_SECTION | CMD_FLAG_END_SECTION,	 
							CID_ConfigFromReg,	"ConfigFromReg","Driver reread and reconfigure From Registry, Tests Driver's Boot-time-init code. input: none",	ConfigFromReg},
	
	// Help
	{CMD_FLAG_START_SECTION,	CID_Help,	"help",		"this help",	CHAR_Help},
	{CMD_FLAG_NULL,				CID_Help1,	"?",		"this help",	CHAR_Help},
	{CMD_FLAG_LAST_ENTRY,		CID_Help2,	"h",		"this help",	Help},

	// {CMD_FLAG_LAST_ENTRY,	CID_MAX_NULL, "",		"",	NULL},
};

// Define options
CMD_LINE_OPT Options[] = 
{
	// UCHAR	Flags, OPTION_TYPE	Index, PCHAR	OptStr, PCHAR	Desc, UCHAR	Type, BOOL	Present, BOOL	Value, valuetype
	//0
	{OPT_FLAG_START_SECTION, ID_LGNum,			"LGNum",			"n  : n is Logical Group Number",				INT_OPT, FALSE, FALSE, 0},
	{CMD_FLAG_NULL,			 ID_LGCreationRole,	"LGCreationRole",	"n  : n is Creation Role",						INT_OPT, FALSE, FALSE, 0},
	{CMD_FLAG_NULL,			 ID_LGJournalPath,	"LGJournalPath",	"s  : s is Journal Directory",					STRING_OPT, FALSE, FALSE, 0},
	{CMD_FLAG_NULL,			 ID_PFile,			"PFile",			"s  : s is Pstore File name including complete path",	STRING_OPT, FALSE, FALSE, 0},

	{CMD_FLAG_NULL,			 ID_DevNum,			"DevNum",	"n  : n is unique Device Number under LG",				INT_OPT, FALSE, FALSE, 0},
	{CMD_FLAG_NULL,			 ID_DevId,			"DevId",	"s  : s is Unique string to identify Device under LG",	STRING_OPT, FALSE, FALSE, 0},
	{CMD_FLAG_NULL,			 ID_DevSize,		"DevSize",	"s  : s is Device Size in bytes.",						STRING_OPT, FALSE, FALSE, 0},
	{CMD_FLAG_NULL,			 ID_vdevname,		"vdevname",	"s  : s is '\\Device\\HardDiskVolume(n)' where n is Partition Volume number",	STRING_OPT, FALSE, FALSE, 0},

	{CMD_FLAG_NULL,			 ID_UniqueId,		"UniqueId",		"s  : s is '\\??\\Volume{GUID}\\' where GUID is numbers",	STRING_OPT, FALSE, FALSE, 0},
	{CMD_FLAG_NULL,			 ID_SignUniqueId,	"SignUniqueId",	"s  : s is 'volume(nnnnnnnn-nnnnnnnnnnnnnnnn-nnnnnnnnnnnnnnnn)' volume(Signature-StartingOffset.QuadPart-PartitionLength.QuadPart)",	STRING_OPT, FALSE, FALSE, 0},

	{CMD_FLAG_NULL,			 ID_DriveLetter,	"DriveLetter",	"s  : s is 's is like '\\??\\D:' where D can be any drive letter",	STRING_OPT, FALSE, FALSE, 0},
	{CMD_FLAG_NULL,			 ID_lrdbSize,		"lrdbSize",		"n  : n is in bytes of LRDB Bitmap Size Defualt is 8K",				INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_hrdbSize,		"hrdbSize",		"n  : n is in bytes of HRDB Bitmap Size Defualt is 128K",			INT_OPT, FALSE, FALSE, 0},	

	{CMD_FLAG_NULL,			 ID_State,			"State",		"n  : n is State Mode. 1: PASSTHRU, 2: TRACKING, 3: NORMAL, 4: FULL_REFRESH, 5: SMART_REFRESH",			INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_SrcIP,			"SrcIP",		"s  : s is IP Address String. Example: 129.212.65.20",	STRING_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_DstIP,			"DstIP",		"s  : s is IP Address String. Example: 129.212.65.20",	STRING_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_SrcPort,		"SrcPort",		"n  : n is Socket Port Number. Example: 575",	INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_DstPort,		"DstPort",		"n  : n is Socket Port Number. Example: 575",	INT_OPT, FALSE, FALSE, 0},	

	{CMD_FLAG_NULL,			 ID_Version,		"Version",		"s  : s is PMD Version String Name. Example: '5.0.0' ",	STRING_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_HostId,			"HostId",		"n  : n is Local System HostID. Example: check from ipconfig",	INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_SystemName,		"SystemName",	"s  : s is Local Computer system name string Example: 'mycomputer' ",	STRING_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_RemoteDriveName,"RemoteDriveName",	"s  : s is Remote secondary Drive Name pair. Example: 'F:' ",	STRING_OPT, FALSE, FALSE, 0},	

	{CMD_FLAG_NULL,			 ID_MaxSize,		"MaxSize",			"n  : n is Max Transfer Size, Default is 256K bytes ",			INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_MaxSendPkts,	"MaxSendPkts",		"n  : n is is Max Send Buffers, Default is 5",					INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_MaxRecvPkts,	"MaxRecvPkts",		"n  : n is is Max Recv Buffers, Default is 5",					INT_OPT, FALSE, FALSE, 0},	

	// Tuning Params
	{CMD_FLAG_NULL,			 ID_TrackingIoCount,			"TrackingIoCount",			"n  : n is MAX_IO_FOR_TRACKING_TO_SMART_REFRESH than we change the state, Default is 10",	INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_MaxTransferUnit,			"MaxTransferUnit",			"n  : n is Maximum Raw Data Size which gets transfered to secondary., Default is 256K",	INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_NumOfAsyncRefreshIO,		"NumOfAsyncRefreshIO",		"n  : n is Maximum Number of Refresh Async Read IO allowed, Default is 5",					INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_AckWakeupTimeout,			"AckWakeupTimeout",			"n  : n is Timeout used to wake up Ack thread and prepare its LRDB or HRDB, Default is 60 secs",					INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_throtal_refresh_send_pkts,	"throtal_refresh_send_pkts","n  : n is the max send pkts sync during refresh, Default is 100",					INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_throtal_commit_send_pkts,	"throtal_commit_send_pkts",	"n  : n is the max send pkts sync during Commit, Default is 400",					INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_NumOfPktsSendAtaTime,		"NumOfPktsSendAtaTime",		"n  : n is Num Of max pkts will get send at a time, Default is 1",					INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_NumOfPktsRecvAtaTime,		"NumOfPktsRecvAtaTime",		"n  : n is Num Of max pkts will get Recv at a time, Default is 1",					INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_NumOfSendBuffers,			"NumOfSendBuffers",			"n  : n is Num Of max Send Buffers used for sending the TDI Data, Default is 5",	INT_OPT, FALSE, FALSE, 0},	
	{CMD_FLAG_NULL,			 ID_DebugLevel,					"DebugLevel",				"n  : n is Driver Debug Level Printf Message control display values, Default is 0x1",INT_OPT, FALSE, FALSE, 0},	

	{CMD_FLAG_NULL,			 ID_TimeOut,					"timeout",					"n  : value in seconds. This much seconds specify Perfomance command runs and than terminates",					INT_OPT,	FALSE, FALSE, 0},
	{CMD_FLAG_NULL,			 ID_TimeDelay,					"timedelay",				"n  : value in MiliSeconds. This much Miliseconds delay will introduced between Perfomance data retrival. Default is 1 sec",		INT_OPT,	FALSE, FALSE, 0},

	// OPT_FLAG_END_SECTION
	{0, ID_MAX_NULL, NULL, NULL, INT_OPT, FALSE, FALSE, 0},
};

//
// main
// 
int	main(int argc, char *argv[])
{
	if (argc <= 1)
		CHAR_Help();	
	else
		CLIParseCmdLine(&CmdPrefixes, Commands, Options, argc, argv);

	return(0);	// Stop the compiler from complaining.
} // main()


void	PrintDesc(PCHAR String, DWORD Length, DWORD Start, DWORD End)
{
	CHAR	Save;
	DWORD	Size;
	PCHAR	Str;
	PCHAR	Temp;
	DWORD	X,Y;

	Str = String;
	Size = (End - Start) - 2;

	do{
		if(strlen(Str) < Size){
			printf("%s", Str);
			break;
		}

		Temp = Str + Size;
		while((*(Temp) != ' ') && (Temp > Str)){
			Temp--;
		}
		if(*Temp == ' '){
			Temp++;
		}

		Save = *(Temp);
		*(Temp) = 0;
		
		printf("%s", Str);

		Str = Temp;

		*Str = Save;

		CLIGetXY(&X, &Y);
		CLISetXY(Start, Y + 1);

	}while(TRUE);
} // PrintDesc()

//
// CHAR_Help
//
#define DEFAULT_SCREEN_CHAR_WIDTH	80

#define TAB_SIZE	4
//
// CHAR_Help: character based help display
//
void	CHAR_Help(void)
{
	DWORD	ScreenWidth;
	DWORD	i;
	DWORD	strSize,MaxWidthOfCmdDesc;
	DWORD	CurrentScreen = 0;
	DWORD	TabSize;
	BOOL						ConsoleInfoValid = FALSE;
	CONSOLE_SCREEN_BUFFER_INFO	ConsoleInfo;

	ConsoleInfoValid = CLIGetConsoleInfo(&ConsoleInfo);

	if (ConsoleInfoValid)
		ScreenWidth		= 	ConsoleInfo.dwSize.X;
	else
		ScreenWidth		= DEFAULT_SCREEN_CHAR_WIDTH;

	TabSize = TAB_SIZE;
	CurrentScreen	= 0;

	printf("\nSyntax: MCLI %scommand [%soption value] [%soption ]... \n\n",
		CmdPrefixes.CmdPrefix, CmdPrefixes.OptPrefix, CmdPrefixes.OptPrefix);

	printf("Commands:\n");

	// first calculate MaxWidthOfCmdDesc for command string
	strSize				= 0;
	MaxWidthOfCmdDesc	= 0;

	for (i=0; ;i++)	{

		if((!Commands[i].CmdStr) || (!Commands[i].Desc)){
			break;	// we are done displaying command help
		}

		strSize = strlen(Commands[i].CmdStr);
		if (strSize > MaxWidthOfCmdDesc)
			MaxWidthOfCmdDesc = strSize;

		if(Commands[i].Flags & CMD_FLAG_LAST_ENTRY){
			break;
		}
	}
	MaxWidthOfCmdDesc++; // increment one character for prefix

	// display Command line output
	for (i=0; ;i++)	{

		if((!Commands[i].CmdStr) || (!Commands[i].Desc)){
			break;	// we are done displaying command help
		}

		if((Commands[i].Flags & CMD_FLAG_FIRST_ENTRY) || (Commands[i].Flags & CMD_FLAG_START_SECTION)){
			// line break
			if (i == 0)
				printf("\n");
			else
				printf("\n\n");
			CurrentScreen = 0;
		}
		
		// display command CmdStr
		Display_String( Commands[i].CmdStr,			// String, 
						MaxWidthOfCmdDesc,			// DisplaySize, 
						TabSize,					// StartScreen,
						ScreenWidth,				// ScreenWidth,
						&CurrentScreen,
						CmdPrefixes.CmdPrefix);			// CurrentScreen
		// display command Desc

		Display_String( Commands[i].Desc,					// String, 
						strlen(Commands[i].Desc),			// DisplaySize, 
						MaxWidthOfCmdDesc + TabSize + 1,	// StartScreen,
						ScreenWidth,						// ScreenWidth,
						&CurrentScreen,
						NULL);					// CurrentScreen

		if(Commands[i].Flags & CMD_FLAG_LAST_ENTRY){
			break;
		}

	} // for : display Command line output


	printf("\n\n");

	printf("Options: (example: %sOPTION value or %sOPTIONvalue )\n",
			CmdPrefixes.OptPrefix, CmdPrefixes.OptPrefix, CmdPrefixes.OptPrefix);

	// calculate MaxWidthOfCmdDesc for command Desc string
	strSize				= 0;
	MaxWidthOfCmdDesc	= 0;

	for (i=0; ;i++)	{

		if((!Options[i].OptStr) || (!Options[i].Desc)){
			break;
		}

		strSize = strlen(Options[i].OptStr);

		if (strSize > MaxWidthOfCmdDesc)
			MaxWidthOfCmdDesc = strSize;

		if(Options[i].Flags & OPT_FLAG_LAST_ENTRY){
			break;
		}
	}
	MaxWidthOfCmdDesc++; // increment one character for prefix

	// display Option output
	for (i=0; ;i++)	{

		if((!Options[i].OptStr) || (!Options[i].Desc)){
			break;
		}

		if((Options[i].Flags & OPT_FLAG_FIRST_ENTRY) || (Options[i].Flags & OPT_FLAG_START_SECTION)){
			// line break
			if (i == 0)
				printf("\n");
			else
				printf("\n\n");
			CurrentScreen = 0;
		}

		// display command CmdStr
		Display_String( Options[i].OptStr,			// String, 
						MaxWidthOfCmdDesc,			// DisplaySize, 
						TabSize,					// StartScreen,
						ScreenWidth,				// ScreenWidth,
						&CurrentScreen,				
						CmdPrefixes.OptPrefix);			
		// display command Desc

		Display_String( Options[i].Desc,					// String, 
						strlen(Options[i].Desc),			// DisplaySize, 
						MaxWidthOfCmdDesc + TabSize + 1,	// StartScreen,
						ScreenWidth,						// ScreenWidth,
						&CurrentScreen,
						NULL);					// CurrentScreen


		if(Options[i].Flags & OPT_FLAG_LAST_ENTRY){
			break;
		}

	} // for : display Command line output

	printf("\n");

} // CHAR_Help()

void	Display_String( PCHAR String, 
						DWORD DisplaySize, 
						DWORD StartScreen,	// relative to 0
						DWORD ScreenWidth,	// relative to 1
						DWORD *CurrentScreen,	// relative to 0
						PCHAR  Prefix)	
{
	DWORD	i, j;
	DWORD	strSize		= strlen(String);
	DWORD	Cur_cursor	= *CurrentScreen;

	if (StartScreen > ScreenWidth)
		StartScreen = TAB_SIZE;

	// check Current Screen location, if its not StartScreen than insert spaces at start.
	if (Cur_cursor > StartScreen)	{
		printf("\n");
		Cur_cursor = 0;
	}

	if (Cur_cursor < StartScreen)	{
		// Relocate current cursor to start screen 
		for (j=Cur_cursor;j < StartScreen; j++)
			printf("%c",SPACE_CHAR);

		Cur_cursor = StartScreen;
	}

	for (i=0; i < DisplaySize; i++)	{

		if (Cur_cursor >= ScreenWidth)	{
			// start new line at StartScreen position
//			printf("\n");

			for (j=0;j < StartScreen; j++)
				printf("%c",SPACE_CHAR);

			Cur_cursor = StartScreen;
		}

		if (i < strSize)
			{
			if ( (i == 0) && (Prefix != NULL) )
				{
				// display prefix character first
				printf("%c",Prefix[i]);	
				Cur_cursor++;

				printf("%c",String[i]);
				}
			else
				printf("%c",String[i]);
			}
		else
			printf("%c",SPACE_CHAR);

		Cur_cursor++;
	}

	*CurrentScreen = Cur_cursor;
	return;
} // Display_String()

//
// Help : character based help display
//
void	Help(void)
{
	DWORD						i,x;
	DWORD						temp;
	DWORD						X,Y;
	DWORD						CmdLen;
	DWORD						DescLen;
	DWORD						DescStart;
	DWORD						DescSize;
	BOOL						ConsoleInfoValid = FALSE;
	CONSOLE_SCREEN_BUFFER_INFO	ConsoleInfo;

	ConsoleInfoValid = CLIGetConsoleInfo(&ConsoleInfo);

	printf("\nSyntax: CLI %scommand [%soption value] [%soption ]... \n\n",
		CmdPrefixes.CmdPrefix, CmdPrefixes.OptPrefix, CmdPrefixes.OptPrefix);

	printf("Commands:\n\n");

	i = 0;
	do{
		if((!Commands[i].CmdStr) || (!Commands[i].Desc)){
			break;
		}
		
		if((Commands[i].Flags & CMD_FLAG_FIRST_ENTRY) || (Commands[i].Flags & CMD_FLAG_START_SECTION)){

			CmdLen = 0;
			x = i;
			do{
				temp = strlen(Commands[x].CmdStr);
				CmdLen = temp > CmdLen ? temp : CmdLen;
				if((Commands[x].Flags & CMD_FLAG_END_SECTION) || (Commands[x].Flags & CMD_FLAG_LAST_ENTRY)){
					break;
				}
				x++;
			}while(TRUE);

			DescLen = 0;
			x = i;
			do{
				temp = strlen(Commands[x].Desc);
				DescLen = temp > DescLen ? temp : DescLen;
				if((Commands[x].Flags & CMD_FLAG_END_SECTION) || (Commands[x].Flags & CMD_FLAG_LAST_ENTRY)){
					break;
				}
				x++;
			}while(TRUE);

			DescStart = HELP_START_COLUMN + CmdLen + 5;
			DescSize = ConsoleInfo.dwSize.X - DescStart;

			printf("\n");
		}

		CLIGetXY(&X, &Y);
		CLISetXY(HELP_START_COLUMN, Y);

		printf("%s%s", CmdPrefixes.CmdPrefix, Commands[i].CmdStr);

		CLISetXY(DescStart, Y);

		if( (SHORT) (DescLen + DescStart) < ConsoleInfo.dwSize.X){
			printf("%s", Commands[i].Desc);
		}else{
			PrintDesc(Commands[i].Desc, DescSize, DescStart, ConsoleInfo.dwSize.X);
		}

		printf("\n");

		if(Commands[i].Flags & CMD_FLAG_LAST_ENTRY){
			break;
		}
		i++;

	}while(TRUE);

	printf("\n");

	printf("Options: (example: %sOPTION value or %sOPTIONvalue )\n",
			CmdPrefixes.OptPrefix, CmdPrefixes.OptPrefix, CmdPrefixes.OptPrefix);

	i = 0;
	do{

		if((!Options[i].OptStr) || (!Options[i].Desc)){
			break;
		}

		if((Options[i].Flags & OPT_FLAG_FIRST_ENTRY) || (Options[i].Flags & OPT_FLAG_START_SECTION)){

			CmdLen = 0;
			x = i;
			do{
				temp = strlen(Options[x].OptStr);
				CmdLen = temp > CmdLen ? temp : CmdLen;
				if((Options[x].Flags & OPT_FLAG_END_SECTION) || (Options[x].Flags & OPT_FLAG_LAST_ENTRY)){
					break;
				}
				x++;
			}while(TRUE);

			DescLen = 0;
			x = i;
			do{
				temp = strlen(Options[x].Desc);
				DescLen = temp > DescLen ? temp : DescLen;
				if((Options[x].Flags & OPT_FLAG_END_SECTION) || (Options[x].Flags & OPT_FLAG_LAST_ENTRY)){
					break;
				}
				x++;
			}while(TRUE);

			DescStart = HELP_START_COLUMN + CmdLen + 5;
			DescSize = ConsoleInfo.dwSize.X - DescStart;

			printf("\n");
		}

		CLIGetXY(&X, &Y);
		CLISetXY(HELP_START_COLUMN, Y);

		printf("%s%s", CmdPrefixes.OptPrefix, Options[i].OptStr);

		CLISetXY(DescStart, Y);

		if( (SHORT) (DescLen + DescStart) < ConsoleInfo.dwSize.X){
			printf("%s", Options[i].Desc);
		}else{
			PrintDesc(Options[i].Desc, DescSize, DescStart, ConsoleInfo.dwSize.X);
		}

		printf("\n");

		if(Options[i].Flags & OPT_FLAG_LAST_ENTRY){
			break;
		}
		i++;

	}while(TRUE);

	printf("\n");
} // Help()

// internal function used to convert SDK API GetLastError() into string format and returns error text string to caller
VOID	GetErrorText()
{
    HMODULE hModule = NULL; // default to system source
    LPSTR MessageBuffer;
    DWORD dwBufferLength;
	DWORD dwLastError	= GetLastError();
	DWORD dwFormatFlags =	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_IGNORE_INSERTS |
							FORMAT_MESSAGE_FROM_SYSTEM ;

	// display error details
	printf("\tError Detailed Information: \n");	
	printf("\tGetLastError() = %d (0x%08x) \n", dwLastError, dwLastError);	

    
    // If dwLastError is in the network range, 
    //  load the message source.

    if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) {
        hModule = LoadLibraryEx(
            TEXT("netmsg.dll"),
            NULL,
            LOAD_LIBRARY_AS_DATAFILE
            );

        if(hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    // Call FormatMessage() to allow for message 
    //  text to be acquired from the system 
    //  or from the supplied module handle.

    if(dwBufferLength = FormatMessageA(
        dwFormatFlags,
        hModule, // module to get message from (NULL == system)
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
        (LPSTR) &MessageBuffer,
        0,
        NULL
        ))
    {
        // Output message string on stderr.
		printf("\tErrorString    = %s \n",MessageBuffer);	

        // Free the buffer allocated by the system.
        LocalFree(MessageBuffer);
    } 
	else
	{
		printf("\tErrorString    = Unknown Error \n");	
	}

    // If we loaded a message source, unload it.
    if(hModule != NULL)
        FreeLibrary(hModule);

} // GetErrorText()


