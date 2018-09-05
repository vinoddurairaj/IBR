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
 *  04-27-2004  Parag Sanghvi/Veerababu Arja   Initial version.                             *
 *                                                                           *
 *                                                                           *
 *****************************************************************************/
#include "CLI.h"
#include "sftk_config.h"

#define		ONE_SEC_IN_100_NANO_SECS	-(10*1000*1000)	// 1 seconds in relative time of 100 nano seconds
#define		DEFUALT_TIME_INTERVAL		1000 // value in milliseconds, = 1 sec
#define		ONEMILLION					(1024 * 1024)
#define		KB							(1024)
#define		MB							(1024 * 1024)
#define		GB							(1024 * 1024 * 1024)
// #define		TB							(1024 * 1024 * 1024 * 1024)

VOID
sftk_convertBytesIntoString( PCHAR	String, UINT64 Bytes);
VOID
sftk_convertBytesIntoStringForMM( PCHAR	String, UINT64 Bytes);


/*
 *  Function: 		
 *		void	ConfigBegin(void)
 *
 *  Command Line:	/ConfigBegin <-Version s> <-HostId n> <-SystemName s>
 *		
 *  Arguments: 	
 *				<-Version s>		s  : s is PMD Version String Name. Example: '5.0.0' 
 *				<-HostId n>			n  : n is Local System HostID. Example: check from ipconfig
 *				<-SystemName s>		s  : s is Local Computer system name string Example: 'mycomputer' 
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
void	ConfigBegin(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pVersion, pHostId, pSystemName;
	DWORD				retLength = 0;
	CONFIG_BEGIN_INFO	configBegin;

	// Get -Version s parameter
	pVersion = CLIGetOpt(Options[ID_Version].OptStr);
	// pVersion->String
	if ((pVersion == NULL) || (pVersion->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_Version].OptStr, Options[ID_Version].Desc);
		goto ret;
	}
	// Get -Version s parameter
	pHostId = CLIGetOpt(Options[ID_HostId].OptStr);
	// pVersion->String
	if ((pHostId == NULL) || (pHostId->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_HostId].OptStr, Options[ID_HostId].Desc);
		goto ret;
	}
	// Get -Version s parameter
	pSystemName = CLIGetOpt(Options[ID_SystemName].OptStr);
	// pVersion->String
	if ((pSystemName == NULL) || (pSystemName->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_SystemName].OptStr, Options[ID_SystemName].Desc);
		goto ret;
	}

	RtlZeroMemory( &configBegin, sizeof(configBegin));
	configBegin.HostId = pHostId->Int;
	strcpy( configBegin.SystemName, pSystemName->String);
	strcpy( configBegin.Version, pVersion->String);

	printf("\n Input Parameter: \n");
	printf(" HostId    : %d (0x%08x) \n", configBegin.HostId, configBegin.HostId);
	printf(" SystemName: %s \n", configBegin.SystemName);
	printf(" Version   : %s \n", configBegin.Version);

	// Prepare Command and execute it on driver, 
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_CONFIG_BEGIN,
									&configBegin,		// LPVOID lpInBuffer,
									sizeof(configBegin),// DWORD nInBufferSize,
									NULL,				// LPVOID lpOutBuffer,
									0,					// DWORD nOutBufferSize,
									&retLength);		// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_CONFIG_BEGIN", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_ConfigBegin].CmdStr);

ret: 
	return;
} // ConfigBegin()

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
void	ConfigEnd(void)
{
	DWORD				status;
	DWORD				retLength = 0;
		
	// Prepare Command and execute it on driver, 
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_CONFIG_END,
									NULL,				// LPVOID lpInBuffer,
									0,					// DWORD nInBufferSize,
									NULL,				// LPVOID lpInBuffer,
									0,					// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_CONFIG_END", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_ConfigEnd].CmdStr);

ret: 
	return;
} // ConfigEnd()

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
void	ConfigFromReg(void)
{
	DWORD				status;
	DWORD				retLength = 0;
		
	// Prepare Command and execute it on driver, 
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_RECONFIGURE_FROM_REGISTRY,
									NULL,				// LPVOID lpInBuffer,
									0,					// DWORD nInBufferSize,
									NULL,				// LPVOID lpInBuffer,
									0,					// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_RECONFIGURE_FROM_REGISTRY", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_ConfigFromReg].CmdStr);

ret: 
	return;
} // ConfigFromReg()


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
void	NewLG(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole, pLGJournalPath, pPstorFile;
	DWORD				retLength = 0;
	ftd_lg_info_t		lg_info;

	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	// pLGNum->Int
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	// Get -LGJournalPath s parameter
	pLGJournalPath = CLIGetOpt(Options[ID_LGJournalPath].OptStr);
	// pLGNum->Int
	if ((pLGJournalPath == NULL) || (pLGJournalPath->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGJournalPath].OptStr, Options[ID_LGJournalPath].Desc);
		goto ret;
	}

	// Get -PFile s parameter
	pPstorFile = CLIGetOpt(Options[ID_PFile].OptStr);
	// pPstorFile->String
	if ((pPstorFile == NULL) || (pPstorFile->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_PFile].OptStr, Options[ID_PFile].Desc);
		goto ret;
	}

	// Prepare Command and execute it on driver
	memset(&lg_info,0,sizeof(lg_info));

    lg_info.lgdev	 = pLGNum->Int;
	lg_info.lgCreationRole = pLGCreationRole->Int;
	strcpy( lg_info.JournalPath, pLGJournalPath->String);
	strcpy( lg_info.vdevname, pPstorFile->String);
	lg_info.statsize = DEFAULT_STATE_BUFFER_SIZE; // allocate 4K buffer, not sure sounds sufficient for moment

	printf("\n Input Parameter: \n");
	printf(" lgdev    : %d (0x%08x) \n", lg_info.lgdev, lg_info.lgdev);
	printf(" vdevname : %s \n", lg_info.vdevname);
	printf(" statsize : %d (0x%08x) \n", lg_info.statsize, lg_info.statsize);

	// Send IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_NEW_LG,
									&lg_info,				// LPVOID lpInBuffer,
									sizeof(lg_info),		// DWORD nInBufferSize,
									NULL,					// LPVOID lpOutBuffer,
									0,						// DWORD nOutBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_NEW_LG", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_NewLG].CmdStr);

ret: 
	return;
} // NewLG()

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
 *		It uses IOCTL : FTD_NEW_DEVICE to send it to CTL_DEVICE DLL_PROCESS_ATTACH
 */
void	NewDev(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole, pDevNum, plrdbSize, phrdbSize;
	PCMD_LINE_OPT		pDevId, pDevSize, pvdevname, pstrRemoteDeviceName, pUniqueId, pSignUniqueId, pDriveLetter;
	DWORD				retLength = 0;
	ftd_dev_info_t		dev_info;
	UINT64				diskSize;
	PCHAR				ptrVdevName;
	CHAR				devId[128];
	CHAR				emptyStr[10];

	memset(emptyStr, 0, sizeof(emptyStr));

	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	// Get -DevNum n parameter
	pDevNum = CLIGetOpt(Options[ID_DevNum].OptStr);
	if ((pDevNum == NULL) || (pDevNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DevNum].OptStr, Options[ID_DevNum].Desc);
		goto ret;
	}
	// Get -DevSize s parameter
	pDevSize = CLIGetOpt(Options[ID_DevSize].OptStr);
	if ((pDevSize == NULL) || (pDevSize->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DevSize].OptStr, Options[ID_DevSize].Desc);
		goto ret;
	}
	// Get -RemoteDriveName s parameter
	pstrRemoteDeviceName = CLIGetOpt(Options[ID_RemoteDriveName].OptStr);
	if ((pstrRemoteDeviceName == NULL) || (pstrRemoteDeviceName->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_RemoteDriveName].OptStr, Options[ID_RemoteDriveName].Desc);
		goto ret;
	}
	// Get -DevId n parameter
	pDevId = CLIGetOpt(Options[ID_DevId].OptStr);
	// Get -vdevname s parameter
	pvdevname = CLIGetOpt(Options[ID_vdevname].OptStr);
	// Get -UniqueId s parameter
	pUniqueId = CLIGetOpt(Options[ID_UniqueId].OptStr);
	// Get -DriveLetter s parameter
	pDriveLetter = CLIGetOpt(Options[ID_DriveLetter].OptStr);
	
	if	(	( (pvdevname == NULL) || (pvdevname->Value != TRUE) )	&&
			( (pUniqueId == NULL) || (pUniqueId->Value != TRUE) )	 &&
			( (pDriveLetter == NULL) || (pDriveLetter->Value != TRUE)) )
	{ //Error	
		printf("Error specify one of argument from < [-vdevname s] | [-UniqueId s] | [-DriveLetter s] >. Invalid command line argument.\n");
		goto ret;
	}

	// Get -SignUniqueId s parameter
	pSignUniqueId = CLIGetOpt(Options[ID_SignUniqueId].OptStr);
	// Get -lrdbSize n parameter
	plrdbSize = CLIGetOpt(Options[ID_lrdbSize].OptStr);
	// Get -hrdbSize n parameter
	phrdbSize = CLIGetOpt(Options[ID_hrdbSize].OptStr);
	
	// Prepare Command and execute it on driver
	memset(&dev_info,0,sizeof(dev_info));
	dev_info.lgnum		= pLGNum->Int;
	dev_info.lgCreationRole = pLGCreationRole->Int;

	dev_info.localcdev	= pDevNum->Int;	// Not Used 
	dev_info.cdev		= pDevNum->Int;
	dev_info.bdev		= pDevNum->Int;	// Not Used 

	diskSize = _atoi64( pDevSize->String);
	dev_info.disksize	= (ULONG) (diskSize / (UINT64)512);

	// TODO : Remove Divide by 4 requirement this is for old way support
	if ( (plrdbSize) && (plrdbSize->Value == TRUE))
		dev_info.lrdbsize32	= plrdbSize->Int/4;
	else
		dev_info.lrdbsize32	= DEFAULT_LRDB_BITMAP_SIZE/4;	// Make It emnpty String

	// TODO : Remove Divide by 4 requirement this is for old way support
	if ( (phrdbSize) && (phrdbSize->Value == TRUE))
		dev_info.hrdbsize32	= phrdbSize->Int/4;
	else
		dev_info.hrdbsize32	= DEFAULT_HRDB_BITMAP_SIZE/4;	// Make It emnpty String

	dev_info.lrdb_res		= 0;	// Not Used 
	dev_info.hrdb_res		= 0;	// Not Used 
	dev_info.lrdb_numbits	= 0;	// Not Used 
	dev_info.hrdb_numbits	= 0;	// Not Used 
	dev_info.statsize		= DEFAULT_STATE_BUFFER_SIZE;	// Not Used 
	dev_info.lrdb_offset	= 0;	// Not Used 

	//if ( (pDevId) && (pDevId->Value == TRUE))
	//		dev_info.devname = pDevId->String;
	//else
	{
		sprintf(devId, "%d_%d",dev_info.lgnum,dev_info.cdev );	// LG_NUM + DEVNUM
		strcpy( dev_info.devname, devId);
	}
	
	ptrVdevName = emptyStr;
	if ( (pDriveLetter) && (pDriveLetter->Value == TRUE))
	{
		if	( ( (pvdevname == NULL) || (pvdevname->Value != TRUE) )	&&
			  ( (pUniqueId == NULL) || (pUniqueId->Value != TRUE) ) )
		{
		// Get Proper VdevName from DriveLetter and initialze it
		// TODO: Veera, insert your routine to get \\Device\\HardDiskVolume(n) from DriveLetter, till than user has to pass else error
		printf("Error specify one of argument from < [-vdevname s] | [-UniqueId s] >. Invalid command line argument.\n");
		goto ret;
		}
		strcpy(dev_info.SuggestedDriveLetterLink, pDriveLetter->String);
		dev_info.SuggestedDriveLetterLinkLength = strlen(dev_info.SuggestedDriveLetterLink);
		if (dev_info.SuggestedDriveLetterLinkLength > 0)
			dev_info.bSuggestedDriveLetterLinkValid = TRUE;

	}

	if ( (pvdevname) && (pvdevname->Value == TRUE))
		strcpy( dev_info.vdevname, pvdevname->String);
	else
		strcpy( dev_info.vdevname, ptrVdevName);	// Make It emnpty String

	if ( (pstrRemoteDeviceName) && (pstrRemoteDeviceName->Value == TRUE))
		strcpy( dev_info.strRemoteDeviceName, pstrRemoteDeviceName->String);
	else
		strcpy( dev_info.strRemoteDeviceName, emptyStr);	// Make It emnpty String
	
	if ( (pUniqueId) && (pUniqueId->Value == TRUE))
	{
		dev_info.bUniqueVolumeIdValid = TRUE;
		dev_info.UniqueIdLength	= (strlen( pUniqueId->String )) / 2;
		strcpy( dev_info.UniqueId, pUniqueId->String);
	}
	else
		memset( dev_info.UniqueId, 0, sizeof(dev_info.UniqueId));	// Make It emnpty String
	
	if ( (pSignUniqueId) && (pSignUniqueId->Value == TRUE))
	{
		dev_info.bSignatureUniqueVolumeIdValid = TRUE;
		dev_info.SignatureUniqueIdLength	= strlen( pSignUniqueId->String );
		strcpy( dev_info.SignatureUniqueId, pSignUniqueId->String);
	}
	else
		memset( dev_info.SignatureUniqueId, 0, sizeof(dev_info.SignatureUniqueId));	// Make It emnpty String
	
	printf("\n Input Parameter: \n");
	printf(" lgnum       : %d (0x%08x) \n", dev_info.lgnum, dev_info.lgnum);
	printf(" localcdev   : %d (0x%08x) \n", dev_info.localcdev, dev_info.localcdev);
	printf(" cdev        : %d (0x%08x) \n", dev_info.cdev, dev_info.cdev);
	printf(" bdev        : %d (0x%08x) \n", dev_info.bdev, dev_info.bdev);
	printf(" disksize    : %d (0x%08x) in sectors (%I64d in bytes) \n", dev_info.disksize, dev_info.disksize, (UINT64) (dev_info.disksize * 512));
	printf(" lrdbsize32  : %d (0x%08x) \n", dev_info.lrdbsize32, dev_info.lrdbsize32);
	printf(" hrdbsize32  : %d (0x%08x) \n", dev_info.hrdbsize32, dev_info.hrdbsize32);
	printf(" lrdb_res    : %d (0x%08x) \n", dev_info.lrdb_res, dev_info.lrdb_res);
	printf(" hrdb_res    : %d (0x%08x) \n", dev_info.hrdb_res, dev_info.hrdb_res);
	printf(" lrdb_numbits: %d (0x%08x) \n", dev_info.lrdb_numbits, dev_info.lrdb_numbits);
	printf(" hrdb_numbits: %d (0x%08x) \n", dev_info.hrdb_numbits, dev_info.hrdb_numbits);
	printf(" statsize    : %d (0x%08x) \n", dev_info.statsize, dev_info.statsize);
	printf(" lrdb_offset : %d (0x%08x) \n", dev_info.lrdb_offset, dev_info.lrdb_offset);
	printf(" devname     : %s \n", dev_info.devname);
	printf(" vdevname    : %s \n", dev_info.vdevname);
	printf(" RemoteDeviceName : %s \n", dev_info.strRemoteDeviceName);
	printf(" bUniqueVolumeIdValid	         : %s \n", (dev_info.bUniqueVolumeIdValid == TRUE)?"TRUE":"FALSE");
	printf(" UniqueIdLength                  : %d \n", dev_info.UniqueIdLength);
	printf(" UniqueId                        : %s \n", dev_info.UniqueId);

	printf(" bSignatureUniqueVolumeIdValid	 : %s \n", (dev_info.bSignatureUniqueVolumeIdValid == TRUE)?"TRUE":"FALSE");
	printf(" SignatureUniqueIdLength         : %d \n", dev_info.SignatureUniqueIdLength);
	printf(" SignatureUniqueId               : %s \n", dev_info.SignatureUniqueId);

	printf(" bSuggestedDriveLetterLinkValid	 : %s \n", (dev_info.bSuggestedDriveLetterLinkValid == TRUE)?"TRUE":"FALSE");
	printf(" SuggestedDriveLetterLinkLength  : %d \n", dev_info.SuggestedDriveLetterLinkLength);
	printf(" SuggestedDriveLetterLink        : %s \n", dev_info.SuggestedDriveLetterLink);
	
	// Send IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_NEW_DEVICE,
									&dev_info,				// LPVOID lpInBuffer,
									sizeof(dev_info),		// DWORD nInBufferSize,
									NULL,					// LPVOID lpOutBuffer,
									0,						// DWORD nOutBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_NEW_DEVICE", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_NewDev].CmdStr);

ret: 
	return;
} // NewDev()

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
void	DelLG(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole;
	DWORD				retLength = 0;
	ftd_state_t			ftdState;

	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	// pLGNum->Int
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}


	// Prepare Command and execute it on driver
	printf("\n Input Parameter: \n");
	printf(" lgNum			: %d (0x%08x) \n", pLGNum->Int, pLGNum->Int);
	printf(" Creation Role	: %d (0x%08x) \n", pLGCreationRole->Int, pLGCreationRole->Int);

	ftdState.lg_num			= pLGNum->Int;
	ftdState.lgCreationRole = pLGCreationRole->Int;

	// Send IOCTL to Driver
	// Change the IOCTL such that we pass the Logical Group Number and the Creation Role
	// to this IOCTL. FTD_DEL_LG
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_DEL_LG,
									&ftdState,			// LPVOID lpInBuffer,
									sizeof(ftdState),	// DWORD nInBufferSize,
									NULL,					// LPVOID lpInBuffer,
									0,						// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_DEL_LG", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_DelLG].CmdStr);

ret: 
	return;
} // DelLG()

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
void	DelDev(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pLGNum, pDevNum;
	DWORD				retLength = 0;
	stat_buffer_t		statebuff;
	CHAR				devId[128];

	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	// pLGNum->Int
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}
	// Get -DevNum n parameter
	pDevNum = CLIGetOpt(Options[ID_DevNum].OptStr);
	if ((pDevNum == NULL) || (pDevNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DevNum].OptStr, Options[ID_DevNum].Desc);
		goto ret;
	}

	// Prepare Command and execute it on driver
	memset( &statebuff, 0, sizeof(statebuff));

	statebuff.lg_num	= pLGNum->Int;	// Not Used in Driver
	statebuff.dev_num	= pDevNum->Int;	// Not Used in Driver
	sprintf(devId, "%d_%d",pLGNum->Int,pDevNum->Int );	// LG_NUM + DEVNUM
	statebuff.DevId		= devId;		// Used in Driver

	printf("\n Input Parameter: \n");
	printf(" lgNum    : %d (0x%08x) \n", statebuff.lg_num, statebuff.lg_num);
	printf(" dev_num  : %d (0x%08x) \n", statebuff.dev_num, statebuff.dev_num);
	printf(" DevId    : %s \n", statebuff.DevId);

	// Send IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_DEL_DEVICE,
									&statebuff,				// LPVOID lpInBuffer,
									sizeof(statebuff),		// DWORD nInBufferSize,
									NULL,					// LPVOID lpInBuffer,
									0,						// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_DEL_DEVICE", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_DelDev].CmdStr);

ret: 
	return;
} // DelDev()

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
void	LGInfo(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pLGNum , pLGCreationRole;
	DWORD				retLength = 0;
	BOOLEAN				bLgNumSpecified = FALSE;
	ULONG				i, j, lgCount, lgNum, devCount, totalLg;
	stat_buffer_t		statebuff;
	ftd_dev_info_t		*pDev_info;
	ftd_lg_info_t		lgInfo;
	ftd_state_t			lg_state;
	ftd_stat_t			lg_statistics;
	disk_stats_t		dev_stats;
	//CHAR				devId[128];

	totalLg = lgNum = 0;
	
	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	// pLGNum->Int
	if ((pLGNum) && (pLGNum->Value == TRUE) )	
	{ // Specified LG Num
		bLgNumSpecified = TRUE;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	if (bLgNumSpecified == TRUE)
	{ // if : Specified LGNumber
		printf("\n Input Parameter: \n");
		printf(" lgnum       : %d (0x%08x) \n", pLGNum->Int, pLGNum->Int);

		totalLg = 1;
	} // if : Specified LGNumber
	else
	{ // else : Display ALL LG Info 
		totalLg = Sftk_Get_TotalLgCount(pLGCreationRole->Int);
	} // else : Display ALL LG Info 

	
	printf("\n Result: \n");

	for (i=0, lgCount=0; lgCount < totalLg; i++)
	{ // For : Go thru each and LG
		if (bLgNumSpecified == TRUE)
			lgNum = pLGNum->Int;
		else
			lgNum = i;

		// --- Get LG Information for current LgNum 
		status = Sftk_Get_LGInfo(lgNum, pLGCreationRole->Int, &statebuff, &lgInfo);
		if (status != NO_ERROR) 
		{	// Failed.
			continue;
		}
		lgCount++;	// increment retrieve LG Num ocunter

		printf(" lgdev        : %d (0x%08x) \n", lgInfo.lgdev, lgInfo.lgdev);
		printf(" statsize     : %d Bytes \n", lgInfo.statsize);

		status = Sftk_Get_LGState(lgNum, pLGCreationRole->Int, &lg_state);
		if (status == NO_ERROR) 
		{	// successed.
			switch(lg_state.state)
			{
				case SFTK_MODE_PASSTHRU		:	printf(" Lg_state     : 0x%08x (SFTK_MODE_PASSTHRU) \n", lg_state.state); break;
				case SFTK_MODE_TRACKING		:	printf(" Lg_state     : 0x%08x (SFTK_MODE_TRACKING) \n", lg_state.state); break;
				case SFTK_MODE_NORMAL		:	printf(" Lg_state     : 0x%08x (SFTK_MODE_NORMAL  ) \n", lg_state.state); break;
				case SFTK_MODE_FULL_REFRESH	:	printf(" Lg_state     : 0x%08x (SFTK_MODE_FULL_REFRESH) \n", lg_state.state); break;
				case SFTK_MODE_SMART_REFRESH:	printf(" Lg_state     : 0x%08x (SFTK_MODE_SMART_REFRESH) \n", lg_state.state); break;
				case SFTK_MODE_BACKFRESH	:	printf(" Lg_state     : 0x%08x (SFTK_MODE_BACKFRESH) \n", lg_state.state); break;
				default:						printf(" Lg_state     : 0x%08x (INVALID_STATE) !!!\n", lg_state.state); break;
			}
		}

		// Display LG Statistics
		status = Sftk_Get_LGStatistics(lgNum, pLGCreationRole->Int, &lg_statistics);
		if (status == NO_ERROR) 
		{	// successed.
			printf(" LG StatiStics: \n");		
			printf(" connected       : %d (0x%08x) \n", lg_statistics.connected, lg_statistics.connected);		// NOT USED in driver
			printf(" loadtimesecs    : %d (0x%08x) \n", lg_statistics.loadtimesecs, lg_statistics.loadtimesecs);	// NOT USED in driver
			printf(" loadtimesystics : %d (0x%08x) \n", lg_statistics.loadtimesystics, lg_statistics.loadtimesystics );// NOT USED in driver	
			printf(" wlentries       : %d (0x%08x) \n", lg_statistics.wlentries, lg_statistics.wlentries);
			printf(" wlsectors       : %d (0x%08x) \n", lg_statistics.wlsectors, lg_statistics.wlsectors);
			printf(" bab_free        : %d (0x%08x) \n", lg_statistics.bab_free, lg_statistics.bab_free);	// NOT USED in driver
			printf(" bab_used        : %d (0x%08x) \n", lg_statistics.bab_used, lg_statistics.bab_used);	// NOT USED in driver
			printf(" state           : %d (0x%08x) \n", lg_statistics.state, lg_statistics.state);
			printf(" ndevs           : %d (0x%08x) \n", lg_statistics.ndevs, lg_statistics.ndevs);
			printf(" sync_depth      : %d (0x%08x) \n", lg_statistics.sync_depth, lg_statistics.sync_depth);
			printf(" sync_timeout    : %d (0x%08x) \n", lg_statistics.sync_timeout, lg_statistics.sync_timeout);
			printf(" iodelay         : %d (0x%08x) \n", lg_statistics.iodelay, lg_statistics.iodelay);
		}

		// -- Get Total Devices count under LG
		devCount = Sftk_Get_TotalLgDevCount( lgNum , pLGCreationRole->Int);
		printf(" lgnum        : %d (0x%08x) \n", lgNum, lgNum);
		printf(" TotalDevices : %d  \n", devCount);

		if (devCount == 0)
			continue;

		//---- Get All Device info for current LgNum 
		memset( &statebuff, 0, sizeof(statebuff));

		statebuff.lg_num	= pLGNum->Int;	// Used in Driver
		statebuff.dev_num	= 0;			// Not Used in Driver
		// sprintf(devId, "%d_%d",pLGNum->Int,pDevNum->Int );	// LG_NUM + DEVNUM
		statebuff.DevId		= NULL;		// Not Used in Driver

		statebuff.len =	sizeof(ftd_dev_info_t) * (devCount+2); // Total Size of buffer to get Dev info
		statebuff.addr = malloc(statebuff.len);
		if (statebuff.addr == NULL)
		{
			printf("Error: malloc(size %d) failed. GetLastError() %d \n",
								statebuff.len, GetLastError());
			GetErrorText();
			goto ret;
		}
		memset( statebuff.addr, 0, sizeof(statebuff.len));

		status = sftk_DeviceIoControl(	NULL, 
										SFTK_CTL_USER_MODE_DEVICE_NAME,
										FTD_GET_DEVICES_INFO,
										&statebuff,				// LPVOID lpInBuffer,
										sizeof(statebuff),		// DWORD nInBufferSize,
										&statebuff,				// LPVOID lpOutBuffer,
										sizeof(statebuff),		// DWORD nOutBufferSize,
										&retLength);			// LPDWORD lpBytesReturned,

		if (status != NO_ERROR) 
		{	// Failed.
			printf("Error: LGNum %d, TotalDevice %d: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
								lgNum, devCount, "FTD_GET_DEVICES_INFO", SFTK_CTL_USER_MODE_DEVICE_NAME);

			if(statebuff.addr)
				free(statebuff.addr);
			continue;
		}      

		pDev_info = (ftd_dev_info_t *) statebuff.addr;
		for (j=0;	j < devCount; j++)
		{ // for : each and every devices under current LG display info
			printf("  Devices : %d  \n", j);
			printf("     lgnum       : %d (0x%08x) \n", pDev_info->lgnum, pDev_info->lgnum);
			printf("     localcdev   : %d (0x%08x) \n", pDev_info->localcdev, pDev_info->localcdev);
			printf("     cdev        : %d (0x%08x) \n", pDev_info->cdev, pDev_info->cdev);
			printf("     bdev        : %d (0x%08x) \n", pDev_info->bdev, pDev_info->bdev);
			printf("     disksize    : %d (0x%08x) in sectors (%I64d in bytes) \n", pDev_info->disksize, pDev_info->disksize, (UINT64) (pDev_info->disksize * 512));
			printf("     lrdbsize32  : %d (0x%08x) \n", pDev_info->lrdbsize32, pDev_info->lrdbsize32);
			printf("     hrdbsize32  : %d (0x%08x) \n", pDev_info->hrdbsize32, pDev_info->hrdbsize32);
			printf("     lrdb_res    : %d (0x%08x) \n", pDev_info->lrdb_res, pDev_info->lrdb_res);
			printf("     hrdb_res    : %d (0x%08x) \n", pDev_info->hrdb_res, pDev_info->hrdb_res);
			printf("     lrdb_numbits: %d (0x%08x) \n", pDev_info->lrdb_numbits, pDev_info->lrdb_numbits);
			printf("     hrdb_numbits: %d (0x%08x) \n", pDev_info->hrdb_numbits, pDev_info->hrdb_numbits);
			printf("     statsize    : %d (0x%08x) \n", pDev_info->statsize, pDev_info->statsize);
			printf("     lrdb_offset : %d (0x%08x) \n", pDev_info->lrdb_offset, pDev_info->lrdb_offset);
			// printf("     devname     : %s \n", pDev_info->devname);
			// printf("     vdevname    : %s \n", pDev_info->vdevname);
			printf("     bUniqueVolumeIdValid	         : %s \n", (pDev_info->bUniqueVolumeIdValid == TRUE)?"TRUE":"FALSE");
			printf("     UniqueIdLength                  : %d \n", pDev_info->UniqueIdLength);
			printf("     UniqueId                        : %s \n", pDev_info->UniqueId);

			printf("     bSignatureUniqueVolumeIdValid	 : %s \n", (pDev_info->bSignatureUniqueVolumeIdValid == TRUE)?"TRUE":"FALSE");
			printf("     SignatureUniqueIdLength         : %d \n", pDev_info->SignatureUniqueIdLength);
			printf("     SignatureUniqueId               : %s \n", pDev_info->SignatureUniqueId);

			printf("     bSuggestedDriveLetterLinkValid	 : %s \n", (pDev_info->bSuggestedDriveLetterLinkValid == TRUE)?"TRUE":"FALSE");
			printf("     SuggestedDriveLetterLinkLength  : %d \n", pDev_info->SuggestedDriveLetterLinkLength);
			printf("     SuggestedDriveLetterLink        : %s \n", pDev_info->SuggestedDriveLetterLink);
			
			// Get and Display Device Statistics
			status = Sftk_Get_LGDevStatistics( lgNum, pLGCreationRole->Int, pDev_info->cdev, &dev_stats);
			if (status == NO_ERROR) 
			{	// successed.
				printf(" Device cdev %d StatiStics: \n",pDev_info->cdev);
				printf(" devname         : %s \n", dev_stats.devname);		
				printf(" localbdisk      : %d (0x%08x) \n", dev_stats.localbdisk, dev_stats.localbdisk);	
				printf(" localdisksize   : %d (0x%08x) \n", dev_stats.localdisksize, dev_stats.localdisksize);
				printf(" wlentries       : %d (0x%08x) \n", dev_stats.wlentries, dev_stats.wlentries);	
				printf(" wlsectors       : %d (0x%08x) \n", dev_stats.wlsectors, dev_stats.wlsectors);	

				printf(" readiocnt       : %I64d (0x%0I64x) \n", dev_stats.readiocnt, dev_stats.readiocnt);	
				printf(" writeiocnt      : %I64d (0x%0I64x) \n", dev_stats.writeiocnt, dev_stats.writeiocnt);	
				printf(" sectorsread     : %I64d (0x%0I64x) \n", dev_stats.sectorsread, dev_stats.sectorsread);	
				printf(" sectorswritten  : %I64d (0x%0I64x) \n", dev_stats.sectorswritten, dev_stats.sectorswritten);	
			}

			pDev_info ++;
		} // for : each and every devices under current LG display info

		if(statebuff.addr)
			free(statebuff.addr);

		printf("\n");
	} // For : Go thru each and LG

ret: 
	return;
} // LGInfo()

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
void	GetState(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole;
	DWORD				retLength = 0;
	ftd_state_t			in_state;
	CHAR				string[256];

	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	// pLGNum->Int
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}


	// Retrieve current state from driver :
	memset(&in_state,0,sizeof(in_state));
    in_state.lg_num = pLGNum->Int;
	in_state.lgCreationRole = pLGCreationRole->Int;

	printf("\n Input Parameter: \n");
	printf(" lgNum        : %d (0x%08x) \n", in_state.lg_num, in_state.lg_num);
	
	status = sftk_DeviceIoControl(	NULL, SFTK_CTL_USER_MODE_DEVICE_NAME, FTD_GET_GROUP_STATE,
									&in_state,				// LPVOID lpInBuffer,
									sizeof(ftd_state_t),	// DWORD nInBufferSize,
									&in_state,				// LPVOID lpOutBuffer,
									sizeof(ftd_state_t),	// DWORD nOutBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_GROUP_STATE", SFTK_CTL_USER_MODE_DEVICE_NAME);
		GetErrorText();
		goto ret;
	}      

	printf("\n Result: \n");

	printf(" lgNum        : %d (0x%08x) \n", in_state.lg_num, in_state.lg_num);
	GetLGStatsInString( string, in_state.state);
	printf(" Current State: %d (0x%08x) (%s) \n", in_state.state, in_state.state, string);

	printf(" /%s command Executed Successfully. \n", Commands[CID_GetState].CmdStr);

ret: 
	return;
} // GetState()

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
void	SetState(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole, pState;
	DWORD				retLength = 0;
	ftd_state_t			state, in_state;
	CHAR				string[256];

	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	// pLGNum->Int
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	// Get -State n parameter
	pState = CLIGetOpt(Options[ID_State].OptStr);
	// pLGNum->Int
	if ((pState == NULL) || (pState->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_State].OptStr, Options[ID_State].Desc);
		goto ret;
	}
	if ( (pState->Int < 1) || (pState->Int > 5) )	{
		printf("Error '%s %s' input argument %d is not valid value. Please enter value between 1 and 5.\n", 
						Options[ID_State].OptStr, Options[ID_State].Desc, pState->Int);
		goto ret;
	}

	// Retrieve current state from driver :
	memset(&in_state,0,sizeof(in_state));
    in_state.lg_num = pLGNum->Int;
	in_state.lgCreationRole = pLGCreationRole->Int;
	
	status = sftk_DeviceIoControl(	NULL, SFTK_CTL_USER_MODE_DEVICE_NAME, FTD_GET_GROUP_STATE,
									&in_state,					// LPVOID lpInBuffer,
									sizeof(ftd_state_t),	// DWORD nInBufferSize,
									&in_state,					// LPVOID lpInBuffer,
									sizeof(ftd_state_t),	// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_GROUP_STATE", SFTK_CTL_USER_MODE_DEVICE_NAME);
		GetErrorText();
		goto ret;
	}      

	// Prepare Command and execute it on driver
	memset(&state,0,sizeof(state));

    state.lg_num = pLGNum->Int;
	switch(pState->Int)
	{
		case 1:	state.state = SFTK_MODE_PASSTHRU; break;
		case 2:	state.state = SFTK_MODE_TRACKING; break;
		case 3:	state.state = SFTK_MODE_NORMAL; break;
		case 4:	state.state = SFTK_MODE_FULL_REFRESH; break;
		case 5:	state.state = SFTK_MODE_SMART_REFRESH; break;
		case 6:	state.state = SFTK_MODE_BACKFRESH; break;
	}

	state.lgCreationRole = pLGCreationRole->Int;

	printf("\n Input Parameter: \n");

	printf(" lgNum              : %d (0x%08x) \n", state.lg_num, state.lg_num);

	GetLGStatsInString( string, in_state.state);
	printf(" Current State      : %d (0x%08x) (%s) \n", in_state.state, in_state.state, string);

	GetLGStatsInString( string, state.state);
	printf(" NewState to Change : %d (0x%08x) (%s) \n", state.state, state.state, string);
	
	// Send IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, SFTK_CTL_USER_MODE_DEVICE_NAME, FTD_SET_GROUP_STATE,
									&state,					// LPVOID lpInBuffer,
									sizeof(ftd_state_t),	// DWORD nInBufferSize,
									NULL,					// LPVOID lpOutBuffer,
									0,						// DWORD nOutBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_SET_GROUP_STATE", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_SetState].CmdStr);

ret: 
	return;
} // SetState()

/*
 *  Function: 		
 *		void	LGStats(void)
 *
 *  Command Line:	/LGStats [<-LGNum n> [-DevNum n]] 
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
void	LGStats(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pLGNum, pDevNum, pLGCreationRole;
	DWORD				retLength;
	BOOLEAN				bLgNumSpecified, bDevNumSpecified;
	ULONG				i, j, size, totalDev, totalLG;
	PALL_LG_STATISTICS	pAllStats	= NULL;
	PLG_STATISTICS		pLgStats	= NULL;
	PDEV_STATISTICS		pDevStats	= NULL;

	bLgNumSpecified = bDevNumSpecified = FALSE;
	retLength = 0;
	
	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	if ((pLGNum) && (pLGNum->Value == TRUE) )	
		bLgNumSpecified = TRUE;	// Specified LG Num

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	// Get -LGNum n parameter
	pDevNum = CLIGetOpt(Options[ID_DevNum].OptStr);
	if ((pDevNum) && (pDevNum->Value == TRUE) )	
		bDevNumSpecified = TRUE;	// Specified Dev Num

	if (bLgNumSpecified == TRUE)
	{ // if : Specified LGNumber, use FTD_GET_LG_STATS_INFO IOCTL 
		printf("\n Input Parameter: \n");
		printf(" lgnum       : %d (0x%08x) \n", pLGNum->Int, pLGNum->Int);

		totalDev = 0;
		if (bDevNumSpecified == FALSE)
		{
			// Get Total number Devs exist for all LG in Driver 
			totalDev = Sftk_Get_TotalLgDevCount( pLGNum->Int ,pLGCreationRole->Int);
			printf(" Devnum      : %d (0x%08x) \n", pDevNum->Int, pDevNum->Int);
		}
		else
			totalDev = 1;

		size = MAX_SIZE_LG_STATISTICS( totalDev );
		pLgStats = calloc(1, size);
		if (pLgStats == NULL)
		{
			printf("Error: calloc(size %d) failed. GetLastError() %d \n", size, GetLastError());
			GetErrorText();
			goto ret;
		}
		pAllStats = (PALL_LG_STATISTICS) pLgStats; // so later we can free this !!

		// Initialize it 
		RtlZeroMemory( pLgStats, size );
		pLgStats->LgNum = pLGNum->Int;
		if (bDevNumSpecified == FALSE)
		{
			pLgStats->Flags = GET_LG_AND_ITS_ALL_DEV_INFO;
		}
		else
		{
			pLgStats->Flags = GET_LG_AND_SPECIFIED_ONE_DEV_INFO;
			pLgStats->cdev	= pDevNum->Int;
		}
		pLgStats->NumOfDevStats = totalDev;
		pLgStats->lgCreationRole = pLGCreationRole->Int;
		
		// Get All Stat Information
		status = Sftk_Get_LGStatsInfo( pLgStats, size );
		if (status != NO_ERROR) 
		{	// Failed.
			printf("Error: Failed to Retrieve LG %d stats information, GetLastError() %d \n",pLGNum->Int, GetLastError());
			GetErrorText();
			goto ret;
		}

		printf("\n Result: \n");
		DisplayLGStatsInfo( pLgStats );
		pDevStats = &pLgStats->DevStats[0];
		for (j=0; j < pLgStats->NumOfDevStats; j++)
		{
			printf("\n");
			DisplayDevStatsInfo( pDevStats );
			pDevStats = (PDEV_STATISTICS) ((ULONG) pDevStats + sizeof(DEV_STATISTICS));
		}

		printf(" /%s command Executed Successfully. \n", Commands[CID_LGStats].CmdStr);
		goto ret;
	} // if : Specified LGNumber

	// Display ALL LG Info use Ioctl FTD_GET_ALL_STATS_INFO
	totalDev = totalLG = 0;

	// Get Total number LG exist in Driver 
	totalLG = Sftk_Get_TotalLgCount(pLGCreationRole->Int);

	// Get Total number Devs exist for all LG in Driver 
	totalDev = Sftk_Get_TotalDevCount();

	size = MAX_SIZE_ALL_LG_STATISTICS( totalLG, totalDev );
	pAllStats = calloc(1, size);
	if (pAllStats == NULL)
	{
		printf("Error: calloc(size %d) failed. GetLastError() %d \n", size, GetLastError());
		GetErrorText();
		goto ret;
	}
	RtlZeroMemory( pAllStats, size );

	pAllStats->NumOfLgEntries  = totalLG;
	pAllStats->NumOfDevEntries = totalDev;

	// Get All Stat Information
	status = Sftk_Get_AllStatsInfo( pAllStats, size );
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: Failed to Retrieve All LG stats information, GetLastError() %d \n",GetLastError());
		GetErrorText();
		goto ret;
	}

	printf("\n Result: \n");
	printf(" TotalLg      : %d (0x%08x) \n", pAllStats->NumOfLgEntries, pAllStats->NumOfLgEntries);
	printf(" TotalDev     : %d (0x%08x) \n", pAllStats->NumOfDevEntries, pAllStats->NumOfDevEntries);
		
	// display All LG Retrieved Stats info 
	pLgStats = &pAllStats->LgStats[0];
	for (i = 0; i < pAllStats->NumOfLgEntries; i ++)
	{
		printf("\n");
		DisplayLGStatsInfo( pLgStats );

		pDevStats = &pLgStats->DevStats[0];

		for (j=0; j < pLgStats->NumOfDevStats; j++)
		{
			printf("\n");
			DisplayDevStatsInfo( pDevStats );
			pDevStats = (PDEV_STATISTICS) ((ULONG) pDevStats + sizeof(DEV_STATISTICS));
		}

		if (pLgStats->NumOfDevStats == 0)
			pDevStats = (PDEV_STATISTICS) ((ULONG) pDevStats + sizeof(DEV_STATISTICS));

		pLgStats = (PLG_STATISTICS) pDevStats;
	}

	printf(" /%s command Executed Successfully. \n", Commands[CID_LGStats].CmdStr);
ret: 
	if (pAllStats)
		free(pAllStats);

	return;
} // LGStats()

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
void	LGMonitor(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole, pID_TimeOut, pID_TimeDelay, pDevNum;
	DWORD				retLength;
	ULONG				size, totalDev, timeOut, timeDelay;
	BOOLEAN				bDevNumSpecified;
	BOOLEAN				bDone = FALSE;
	time_t				StartTime, CurrentTime;
	ULONG				elapsed_time;
	PALL_LG_STATISTICS	pAllStats	= NULL;
	PLG_STATISTICS		pLgStats	= NULL;
	PDEV_STATISTICS		pDevStats	= NULL;
	CHAR				displayStr[512];
	CHAR				string[64];
	ULONG				percentage = 0;
	ULONG				mm_percentage = 0;
	CHAR				migrate[20], refresh[20], commit[20], pending[20];

	retLength = 0;
	bDevNumSpecified = FALSE;

	// Get -LGNum n parameter
	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	// pLGNum->Int
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	printf("\n");

	// Get -LGNum n parameter
	pDevNum = CLIGetOpt(Options[ID_DevNum].OptStr);
	if ((pDevNum) && (pDevNum->Value == TRUE) )	
		bDevNumSpecified = TRUE;	// Specified Dev Num

	// Get -LGNum n parameter
	pID_TimeOut = CLIGetOpt(Options[ID_TimeDelay].OptStr);
	if ((pID_TimeOut == NULL) || (pID_TimeOut->Value != TRUE) )	{ //Error	
		printf("NOTE: 'timeout' input argument not specified. Use 'CTRL+C' key to terminate this command.\n");
		timeOut = 0;
	} else	{
		timeOut = pID_TimeOut->Int;
	}
	// Get timedelay if specified.
	pID_TimeDelay = CLIGetOpt(Options[31].OptStr);
	if ((pID_TimeDelay == NULL) || (pID_TimeDelay->Value != TRUE) )	{ //Error	
		timeDelay = DEFUALT_TIME_INTERVAL;
		printf("NOTE: 'timedelay' input argument not specified. Using Default %d miliseconds as a time interval.\n",
				timeDelay);
		
	} else	{
		timeDelay = pID_TimeDelay->Int;
	}

	printf("\n Input Parameter: \n");
	printf(" lgnum       : %d (0x%08x) ", pLGNum->Int, pLGNum->Int);
	printf(" TimeOut     : %d (0x%08x) ", timeOut, timeOut);
	printf(" TimeDelay   : %d (0x%08x) \n", timeDelay, timeDelay);
	

	// Get Total number Devs exist for all LG in Driver 
	totalDev = 0;
	if (bDevNumSpecified == FALSE)
	{ // Get Total number Devs exist for all LG in Driver 
		totalDev = Sftk_Get_TotalLgDevCount( pLGNum->Int, pLGCreationRole->Int);
		printf(" total Devs  : %d (0x%08x) \n", totalDev, totalDev);
	}
	else
	{
		totalDev = 1;
		printf(" Devnum      : %d (0x%08x) \n", pDevNum->Int, pDevNum->Int);
	}
	
	// Allocate memory
	size = MAX_SIZE_LG_STATISTICS( totalDev );
	pLgStats = calloc(1, size);
	if (pLgStats == NULL)
	{
		printf("Error: calloc(size %d) failed. GetLastError() %d \n", size, GetLastError());
		GetErrorText();
		goto ret;
	}
	pAllStats = (PALL_LG_STATISTICS) pLgStats; // so later we can free this !!

	// Initialize it
	bDone = FALSE;
	time(&StartTime);
	printf("\n\n LG Statistics/Performance Monitor Numeric Details: LGNum %d \n\n", pLGNum->Int);

	printf("   LG           MM    LG     |     QM_MigratPkt     |    QM_RefreshPkt     |    QM_CommitPkt      |    QM_PendingPkt    \n");
	printf(" StateMode     USE%% Refresh %%| NoOfPkts   TotalSize | NoOfPkts   TotalSize | NoOfPkts   TotalSize | NoOfPkts   TotalSize  \n");
	printf(" ------------------------------------------------------------------------------------------------------------------------\n");
	do	{
		RtlZeroMemory( pLgStats, size );
		pLgStats->LgNum = pLGNum->Int;
		if (bDevNumSpecified == FALSE)
		{
			pLgStats->Flags = GET_LG_AND_ITS_ALL_DEV_INFO;
		}
		else
		{
			pLgStats->Flags = GET_LG_AND_SPECIFIED_ONE_DEV_INFO;
			pLgStats->cdev	= pDevNum->Int;
		}
		pLgStats->NumOfDevStats = totalDev;
		pLgStats->lgCreationRole = pLGCreationRole->Int;
		
		// Get All Stat Information
		status = Sftk_Get_LGStatsInfo( pLgStats, size );
		if (status != NO_ERROR) 
		{	// Failed.
			printf("Error: Failed to Retrieve LG %d stats information, GetLastError() %d \n",pLGNum->Int, GetLastError());
			GetErrorText();
			goto ret;
		}

		GetLGStatsInString(string, pLgStats->LgState);

		mm_percentage = 0;
		if (pLgStats->LgStats.MM_TotalMemAllocated > (UINT64) 0) 
		{
			mm_percentage = (ULONG) ( (UINT64) (pLgStats->LgStats.MM_TotalMemIsUsed * (UINT64) 100) / (UINT64) pLgStats->LgStats.MM_TotalMemAllocated);
		}
	
		percentage = 0;
		if ( (pLgStats->LgStats.TotalBitsDirty > (UINT64) 0)  &&
				( (pLgStats->LgState == SFTK_MODE_FULL_REFRESH) || (pLgStats->LgState == SFTK_MODE_SMART_REFRESH) || 
				  (pLgStats->LgState == SFTK_MODE_BACKFRESH)) )
		{
			percentage = (ULONG) ( (UINT64) (pLgStats->LgStats.TotalBitsDone * (UINT64) 100) / (UINT64) pLgStats->LgStats.TotalBitsDirty);
		}

		
		sftk_convertBytesIntoString( migrate, (pLgStats->LgStats.QM_BlksWrMigrate * 512));
		sftk_convertBytesIntoString( refresh, (pLgStats->LgStats.QM_BlksWrRefresh * 512));
		sftk_convertBytesIntoString( commit,  (pLgStats->LgStats.QM_BlksWrCommit  * 512));
		sftk_convertBytesIntoString( pending, (pLgStats->LgStats.QM_BlksWrPending * 512));
		
		//			printf("   LG           MM    LG     |     QM_MigratPkt     |    QM_RefreshPkt     |    QM_CommitPkt      |    QM_PendingPkt    \n");
		//			printf(" StateMode     USE% Refresh %%| NoOfPkts   TotalSize | NoOfPkts   TotalSize | NoOfPkts   TotalSize | NoOfPkts   TotalSize  \n");
		//			printf(" ------------------------------------------------------------------------------------------------------------------------\n");
		sprintf(displayStr," %s  %-03d   %-03d    | %-10I64d %-s| %-10I64d %-s| %-10I64d %-s| %-10I64d %-s", 
											string, mm_percentage, percentage, 
											pLgStats->LgStats.QM_WrMigrate,	migrate,
											pLgStats->LgStats.QM_WrRefresh,	refresh,
											pLgStats->LgStats.QM_WrCommit,	commit,
											pLgStats->LgStats.QM_WrPending, pending); 
		printf("%s\r",displayStr);
		
		time(&CurrentTime);

		elapsed_time = (ULONG) difftime(CurrentTime,StartTime);

		if(timeOut > 0)	
		{
			if (elapsed_time > timeOut)	
			{
				printf("\n");
				printf(" -------------------------------------------------------------------------------------------------------------------\n");
				printf("\n\n Specified Time Out %d occured, Terminating Command.\n",timeOut);
				bDone = TRUE;
				goto ret;
			}
		}
		
		if(timeDelay > 0)	
			Sleep(timeDelay);

	} while(bDone == FALSE); 

	printf("\n");
	printf(" -------------------------------------------------------------------------------------------------------------------\n");

	// printf(" /%s command Executed Successfully. \n", Commands[CID_LGMonitor].CmdStr);
	goto ret;

ret: 
	if (pAllStats)
		free(pAllStats);

	return;
} // LGMonitor()

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
void	MMMonitor(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole, pID_TimeOut, pID_TimeDelay, pDevNum;
	DWORD				retLength;
	ULONG				size, totalDev, timeOut, timeDelay;
	BOOLEAN				bDevNumSpecified;
	BOOLEAN				bDone = FALSE;
	time_t				StartTime, CurrentTime;
	ULONG				elapsed_time;
	PALL_LG_STATISTICS	pAllStats	= NULL;
	PLG_STATISTICS		pLgStats	= NULL;
	PDEV_STATISTICS		pDevStats	= NULL;
	CHAR				displayStr[512];
//	CHAR				string[64];
	ULONG				percentage = 0;
	CHAR				MM_TotalMemAllocated[20], MM_TotalMemIsUsed[20], MM_LgTotalMemUsed[20], MM_LgMemUsed[20];
	CHAR				MM_LgTotalRawMemUsed[20],MM_LgRawMemUsed[20], MM_TotalOSMemUsed[20],MM_OSMemUsed[20];
	CHAR				MM_TotalNumOfMdlLocked[20],MM_TotalSizeOfMdlLocked[20],MM_TotalNumOfMdlLockedAtPresent[20],MM_TotalSizeOfMdlLockedAtPresent[20];


	retLength = 0;
	bDevNumSpecified = FALSE;

	// Get -LGNum n parameter
	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	// pLGNum->Int
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	printf("\n");

	// Get -LGNum n parameter
	pDevNum = CLIGetOpt(Options[ID_DevNum].OptStr);
	if ((pDevNum) && (pDevNum->Value == TRUE) )	
		bDevNumSpecified = TRUE;	// Specified Dev Num

	// Get -LGNum n parameter
	pID_TimeOut = CLIGetOpt(Options[ID_TimeDelay].OptStr);
	if ((pID_TimeOut == NULL) || (pID_TimeOut->Value != TRUE) )	{ //Error	
		printf("NOTE: 'timeout' input argument not specified. Use 'CTRL+C' key to terminate this command.\n");
		timeOut = 0;
	} else	{
		timeOut = pID_TimeOut->Int;
	}
	// Get timedelay if specified.
	pID_TimeDelay = CLIGetOpt(Options[31].OptStr);
	if ((pID_TimeDelay == NULL) || (pID_TimeDelay->Value != TRUE) )	{ //Error	
		timeDelay = DEFUALT_TIME_INTERVAL;
		printf("NOTE: 'timedelay' input argument not specified. Using Default %d miliseconds as a time interval.\n",
				timeDelay);
		
	} else	{
		timeDelay = pID_TimeDelay->Int;
	}

	printf("\n Input Parameter: \n");
	printf(" lgnum       : %d (0x%08x) ", pLGNum->Int, pLGNum->Int);
	printf(" TimeOut     : %d (0x%08x) ", timeOut, timeOut);
	printf(" TimeDelay   : %d (0x%08x) \n", timeDelay, timeDelay);

	// Get Total number Devs exist for all LG in Driver 
	totalDev = 0;
	if (bDevNumSpecified == FALSE)
	{ // Get Total number Devs exist for all LG in Driver 
		totalDev = Sftk_Get_TotalLgDevCount( pLGNum->Int, pLGCreationRole->Int );
		printf(" total Devs  : %d (0x%08x) \n", totalDev, totalDev);
	}
	else
	{
		totalDev = 1;
		printf(" Devnum      : %d (0x%08x) \n", pDevNum->Int, pDevNum->Int);
	}
	
	// Allocate memory
	size = MAX_SIZE_LG_STATISTICS( totalDev );
	pLgStats = calloc(1, size);
	if (pLgStats == NULL)
	{
		printf("Error: calloc(size %d) failed. GetLastError() %d \n", size, GetLastError());
		GetErrorText();
		goto ret;
	}
	pAllStats = (PALL_LG_STATISTICS) pLgStats; // so later we can free this !!

	// Initialize it
	bDone = FALSE;
	time(&StartTime);
	printf("\n\n MM Statistics/Performance Monitor Numeric Details: LGNum %d \n\n", pLGNum->Int);

	printf("  MM |     MM  Total   |    LG  Current    |    LG  Total      |    MDL Current    |    MDL Total      |    OS Direct Mem   \n");
	printf(" Use%%| Alloc  | InUsed | MemUsed | RawUsed | MemUsed | RawUsed | NoOfMdl | SizeMdl | NoOfMdl | SizeMdl | Current | Total    \n");
	printf(" -------------------------------------------------------------------------------------------------------------------------\n");
	do	{
		RtlZeroMemory( pLgStats, size );
		pLgStats->LgNum = pLGNum->Int;
		if (bDevNumSpecified == FALSE)
		{
			pLgStats->Flags = GET_LG_AND_ITS_ALL_DEV_INFO;
		}
		else
		{
			pLgStats->Flags = GET_LG_AND_SPECIFIED_ONE_DEV_INFO;
			pLgStats->cdev	= pDevNum->Int;
		}
		pLgStats->NumOfDevStats = totalDev;
		pLgStats->lgCreationRole = pLGCreationRole->Int;
		
		// Get All Stat Information
		status = Sftk_Get_LGStatsInfo( pLgStats, size );
		if (status != NO_ERROR) 
		{	// Failed.
			printf("Error: Failed to Retrieve LG %d stats information, GetLastError() %d \n",pLGNum->Int, GetLastError());
			GetErrorText();
			goto ret;
		}

		// GetLGStatsInString(string, pLgStats->LgState);

		percentage = 0;
		if (pLgStats->LgStats.MM_TotalMemAllocated > (UINT64) 0) 
		{
			percentage = (ULONG) ( (UINT64) (pLgStats->LgStats.MM_TotalMemIsUsed * (UINT64) 100) / (UINT64) pLgStats->LgStats.MM_TotalMemAllocated);
		}
	
		sftk_convertBytesIntoStringForMM( MM_TotalMemAllocated, pLgStats->LgStats.MM_TotalMemAllocated );
		sftk_convertBytesIntoStringForMM( MM_TotalMemIsUsed, pLgStats->LgStats.MM_TotalMemIsUsed );
		sftk_convertBytesIntoStringForMM( MM_LgTotalMemUsed, pLgStats->LgStats.MM_LgTotalMemUsed );
		sftk_convertBytesIntoStringForMM( MM_LgMemUsed, pLgStats->LgStats.MM_LgMemUsed );
		sftk_convertBytesIntoStringForMM( MM_LgTotalRawMemUsed, pLgStats->LgStats.MM_LgTotalRawMemUsed );
		sftk_convertBytesIntoStringForMM( MM_LgRawMemUsed, pLgStats->LgStats.MM_LgRawMemUsed );
		sftk_convertBytesIntoStringForMM( MM_TotalOSMemUsed, pLgStats->LgStats.MM_TotalOSMemUsed );
		sftk_convertBytesIntoStringForMM( MM_OSMemUsed, pLgStats->LgStats.MM_OSMemUsed );
		sftk_convertBytesIntoStringForMM( MM_TotalNumOfMdlLocked, pLgStats->LgStats.MM_TotalNumOfMdlLocked );
		sftk_convertBytesIntoStringForMM( MM_TotalSizeOfMdlLocked, pLgStats->LgStats.MM_TotalSizeOfMdlLocked );
		sftk_convertBytesIntoStringForMM( MM_TotalNumOfMdlLockedAtPresent, pLgStats->LgStats.MM_TotalNumOfMdlLockedAtPresent );
		sftk_convertBytesIntoStringForMM( MM_TotalSizeOfMdlLockedAtPresent, pLgStats->LgStats.MM_TotalSizeOfMdlLockedAtPresent );
				// printf( "  MM |     MM  Total   |    LG  Current    |    LG  Total      |    MDL Current    |    MDL Total      |    OS Direct Mem   \n");
				// printf( " Use%%| Alloc  | InUsed | MemUsed | RawUsed | MemUsed | RawUsed | NoOfMdl | SizeMdl | NoOfMdl | SizeMdl | Current | Total    \n");
				// printf( " -------------------------------------------------------------------------------------------------------------------------\n");
		sprintf(displayStr," %-03d | %-s| %-s| %-s | %-s | %-s | %-s | %-s | %-s | %-s | %-s | %-s | %-s", 
											percentage, 
											MM_TotalMemAllocated, MM_TotalMemIsUsed,
											MM_LgMemUsed, MM_LgRawMemUsed,
											MM_LgTotalMemUsed, MM_LgTotalRawMemUsed,
											MM_TotalNumOfMdlLockedAtPresent, MM_TotalSizeOfMdlLockedAtPresent,
											MM_TotalNumOfMdlLocked, MM_TotalSizeOfMdlLocked,
											MM_OSMemUsed, MM_TotalOSMemUsed); 
		printf("%s\r",displayStr);
		
		time(&CurrentTime);

		elapsed_time = (ULONG) difftime(CurrentTime,StartTime);

		if(timeOut > 0)	
		{
			if (elapsed_time > timeOut)	
			{
				printf("\n");
				printf(" -------------------------------------------------------------------------------------------------------------------------\n");
				printf("\n\n Specified Time Out %d occured, Terminating Command.\n",timeOut);
				bDone = TRUE;
				goto ret;
			}
		}
		
		if(timeDelay > 0)	
			Sleep(timeDelay);

	} while(bDone == FALSE); 

	printf("\n");
	printf(" -------------------------------------------------------------------------------------------------------------------------\n");

	// printf(" /%s command Executed Successfully. \n", Commands[CID_MMMonitor].CmdStr);
	goto ret;

ret: 
	if (pAllStats)
		free(pAllStats);

	return;
} // MMMonitor()

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
void	GetParms(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pID_LGNum, pLGCreationRole;
	DWORD				retLength = 0;
	LG_PARAM			lgParam;
	ULONG				seconds = 0;

	RtlZeroMemory( &lgParam, sizeof(lgParam));

	// Get -Version s parameter
	pID_LGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	// pVersion->String
	if ((pID_LGNum == NULL) || (pID_LGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	lgParam.LgNum = pID_LGNum->Int;
	lgParam.lgCreationRole = pLGCreationRole->Int;
	lgParam.ParamFlags = 0;

	printf("\n Input Parameter: \n");
	printf(" LgNum                : %d (0x%08x) \n", lgParam.LgNum, lgParam.LgNum);

	// Prepare Command and execute it on driver, 
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_GET_LG_TUNING_PARAM,
									&lgParam,		// LPVOID lpInBuffer,
									sizeof(lgParam),// DWORD nInBufferSize,
									&lgParam,		// LPVOID lpInBuffer,
									sizeof(lgParam),// DWORD nInBufferSize,
									&retLength);		// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_LG_TUNING_PARAM", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" LgNum                : %d (0x%08x) \n", lgParam.LgNum, lgParam.LgNum);
	printf(" TrackingIoCount               : %d (0x%08x) \n", lgParam.Param.TrackingIoCount, lgParam.Param.TrackingIoCount);
	printf(" MaxTransferUnit               : %d (0x%08x) \n", lgParam.Param.MaxTransferUnit, lgParam.Param.MaxTransferUnit);
	printf(" NumOfAsyncRefreshIO           : %d (0x%08x) \n", lgParam.Param.NumOfAsyncRefreshIO, lgParam.Param.NumOfAsyncRefreshIO);
	printf(" AckWakeupTimeout              : 0x%I64x \n", lgParam.Param.AckWakeupTimeout);
	printf(" RefreshThreadWakeupTimeout    : 0x%I64x \n", lgParam.Param.RefreshThreadWakeupTimeout);
	printf(" sync_depth                    : %d (0x%08x) \n", lgParam.Param.sync_depth, lgParam.Param.sync_depth);
	printf(" sync_timeout                  : %d (0x%08x) \n", lgParam.Param.sync_timeout, lgParam.Param.sync_timeout);
	printf(" iodelay                       : %d (0x%08x) \n", lgParam.Param.iodelay, lgParam.Param.iodelay);
	printf(" throtal_refresh_send_totalsize: %d (0x%08x) \n", lgParam.Param.throtal_refresh_send_totalsize, lgParam.Param.throtal_refresh_send_totalsize);
	printf(" throtal_commit_send_totalsize : %d (0x%08x) \n", lgParam.Param.throtal_commit_send_totalsize, lgParam.Param.throtal_commit_send_totalsize);
	printf(" throtal_commit_send_pkts      : %d (0x%08x) \n", lgParam.Param.throtal_commit_send_pkts, lgParam.Param.throtal_commit_send_pkts);
	printf(" throtal_refresh_send_pkts     : %d (0x%08x) \n", lgParam.Param.throtal_refresh_send_pkts, lgParam.Param.throtal_refresh_send_pkts);
	printf(" NumOfPktsSendAtaTime          : %d (0x%08x) \n", lgParam.Param.NumOfPktsSendAtaTime, lgParam.Param.NumOfPktsSendAtaTime);
	printf(" NumOfPktsRecvAtaTime          : %d (0x%08x) \n", lgParam.Param.NumOfPktsRecvAtaTime, lgParam.Param.NumOfPktsRecvAtaTime);
	printf(" DebugLevel                    : %d (0x%08x) \n", lgParam.Param.DebugLevel, lgParam.Param.DebugLevel);
	
	// printf("\n /%s command Executed Successfully. \n", Commands[CID_GetParms].CmdStr);

ret: 
	return;
} // GetParms()

/*
 *  Function: 		
 *		void	SetParms(void)
 *
 *  Command Line:	/SetParms <-Lgnum n> [-TrackingIoCount n] [-MaxTransferUnit n] [-NumOfAsyncRefreshIO n] [-AckWakeupTimeout n] 
 *							  [-throtal_refresh_send_pkts n] [-throtal_commit_send_pkts n] [-NumOfPktsSendAtaTime n] [-NumOfPktsRecvAtaTime n]
 *  Arguments: 	
 *				[-TrackingIoCount n]			: n is MAX_IO_FOR_TRACKING_TO_SMART_REFRESH than we change the state, Default is 10
 *				[-MaxTransferUnit n]			: n is Maximum Raw Data Size which gets transfered to secondary., Default is 256K
 *				[-NumOfAsyncRefreshIO n]		: n is Maximum Number of Refresh Async Read IO allowed, Default is 5
 *				[-AckWakeupTimeout n]			: n is Timeout used to wake up Ack thread and prepare its LRDB or HRDB, Default is 60 secs
 *				[-throtal_refresh_send_pkts n]  : n is the max send pkts sync during refresh, Default is 100
 *				[-throtal_commit_send_pkts n]	: n is the max send pkts sync during Commit, Default is 400
 *				[-NumOfPktsSendAtaTime n]		: n is Num Of max pkts will get send at a time, Default is 1
 *				[-NumOfPktsRecvAtaTime n]		: n is Num Of max pkts will get Recv at a time, Default is 1
 * 	
 * Returns: Set specified Tunning Parms For Specified LG in Driver
 * 			
 * Conclusion: 
 *
 * Description:
 *		Send FTD_SET_LG_TUNING_PARAM IOCTL to driver.
 */
void	SetParms(void)
{
	DWORD				status;
	PCMD_LINE_OPT		pID_LGNum, pLGCreationRole, pID_TrackingIoCount, pID_MaxTransferUnit, pID_DebugLevel;
	PCMD_LINE_OPT		pID_NumOfAsyncRefreshIO,pID_throtal_commit_send_pkts, pID_throtal_refresh_send_pkts, pID_NumOfPktsRecvAtaTime, pID_NumOfPktsSendAtaTime, pID_NumOfSendBuffers;
//	PCMD_LINE_OPT		pID_AckWakeupTimeout;
	DWORD				retLength = 0;
	LG_PARAM			lgParam;

	RtlZeroMemory( &lgParam, sizeof(lgParam));

	// Get -Version s parameter
	pID_LGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	// pVersion->String
	if ((pID_LGNum == NULL) || (pID_LGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}
	
	lgParam.LgNum = pID_LGNum->Int;
	lgParam.lgCreationRole = pLGCreationRole->Int;
	lgParam.ParamFlags = 0;

	printf("\n Input Parameter: \n");
	printf(" LgNum                : %d (0x%08x) \n", lgParam.LgNum, lgParam.LgNum);

	pID_TrackingIoCount = CLIGetOpt(Options[ID_TrackingIoCount].OptStr);
	if ((pID_TrackingIoCount != NULL) && (pID_TrackingIoCount->Value == TRUE) )	{ //Error	
		lgParam.Param.TrackingIoCount = pID_TrackingIoCount->Int;
		lgParam.ParamFlags |= LG_PARAM_FLAG_USE_TrackingIoCount;
		printf(" TrackingIoCount      : %d (0x%08x) \n", lgParam.Param.TrackingIoCount, lgParam.Param.TrackingIoCount);
	}

	pID_DebugLevel = CLIGetOpt(Options[ID_DebugLevel].OptStr);
	if ((pID_DebugLevel != NULL) && (pID_DebugLevel->Value == TRUE) )	{ //Error	
		lgParam.Param.DebugLevel = pID_DebugLevel->Int;
		lgParam.ParamFlags |= LG_PARAM_FLAG_DebugLevel;
		printf(" DebugLevel           : %d (0x%08x) \n", lgParam.Param.DebugLevel, lgParam.Param.DebugLevel);
	}
	
	pID_MaxTransferUnit = CLIGetOpt(Options[ID_MaxTransferUnit].OptStr);
	if ((pID_MaxTransferUnit != NULL) && (pID_MaxTransferUnit->Value == TRUE) )	{ //Error	
		lgParam.Param.MaxTransferUnit = pID_MaxTransferUnit->Int;
		lgParam.ParamFlags |= LG_PARAM_FLAG_USE_MaxTransferUnit;
		printf(" MaxTransferUnit      : %d (0x%08x) \n", lgParam.Param.MaxTransferUnit, lgParam.Param.MaxTransferUnit);
	}	

	pID_NumOfAsyncRefreshIO = CLIGetOpt(Options[ID_NumOfAsyncRefreshIO].OptStr);
	if ((pID_NumOfAsyncRefreshIO != NULL) && (pID_NumOfAsyncRefreshIO->Value == TRUE) )	{ //Error	
		lgParam.Param.NumOfAsyncRefreshIO = pID_NumOfAsyncRefreshIO->Int;
		lgParam.ParamFlags |= LG_PARAM_FLAG_USE_NumOfAsyncRefreshIO;
		printf(" NumOfAsyncRefreshIO      : %d (0x%08x) \n", lgParam.Param.NumOfAsyncRefreshIO, lgParam.Param.NumOfAsyncRefreshIO);
	}

	/*
	pID_AckWakeupTimeout = CLIGetOpt(Options[ID_AckWakeupTimeout].OptStr);
	if ((pID_AckWakeupTimeout != NULL) && (pID_AckWakeupTimeout->Value == TRUE) )	{ //Error	
		lgParam.Param.AckWakeupTimeout = ((pID_AckWakeupTimeout->Int) * ONE_SEC_IN_100_NANO_SECS);
		lgParam.ParamFlags |= LG_PARAM_FLAG_USE_AckWakeupTimeout;
		printf(" AckWakeupTimeout      : (0x%I64x) (%d seconds) \n", lgParam.Param.AckWakeupTimeout, pID_AckWakeupTimeout->Int);
	}
	*/

	pID_throtal_refresh_send_pkts = CLIGetOpt(Options[ID_throtal_refresh_send_pkts].OptStr);
	if ((pID_throtal_refresh_send_pkts != NULL) && (pID_throtal_refresh_send_pkts->Value == TRUE) )	{ //Error	
		lgParam.Param.throtal_refresh_send_pkts = pID_throtal_refresh_send_pkts->Int;
		lgParam.ParamFlags |= LG_PARAM_FLAG_THROTAL_REFRESH_SEND_PKTS;
		printf(" throtal_refresh_send_pkts      : %d (0x%08x) \n", lgParam.Param.throtal_refresh_send_pkts, lgParam.Param.throtal_refresh_send_pkts);
	}

	pID_throtal_commit_send_pkts = CLIGetOpt(Options[ID_throtal_commit_send_pkts].OptStr);
	if ((pID_throtal_commit_send_pkts != NULL) && (pID_throtal_commit_send_pkts->Value == TRUE) )	{ //Error	
		lgParam.Param.throtal_commit_send_pkts = pID_throtal_commit_send_pkts->Int;
		lgParam.ParamFlags |= LG_PARAM_FLAG_THROTAL_COMMIT_SEND_PKTS;
		printf(" throtal_commit_send_pkts      : %d (0x%08x) \n", lgParam.Param.throtal_commit_send_pkts, lgParam.Param.throtal_commit_send_pkts);
	}

	pID_NumOfPktsSendAtaTime = CLIGetOpt(Options[ID_NumOfPktsSendAtaTime].OptStr);
	if ((pID_NumOfPktsSendAtaTime != NULL) && (pID_NumOfPktsSendAtaTime->Value == TRUE) )	{ //Error	
		lgParam.Param.NumOfPktsSendAtaTime = pID_NumOfPktsSendAtaTime->Int;
		lgParam.ParamFlags |= LG_PARAM_FLAG_NumOfPktsSendAtaTime;
		printf(" NumOfPktsSendAtaTime          : %d (0x%08x) \n", lgParam.Param.NumOfPktsSendAtaTime, lgParam.Param.NumOfPktsSendAtaTime);
	}

	pID_NumOfPktsRecvAtaTime = CLIGetOpt(Options[ID_NumOfPktsRecvAtaTime].OptStr);
	if ((pID_NumOfPktsRecvAtaTime != NULL) && (pID_NumOfPktsRecvAtaTime->Value == TRUE) )	{ //Error	
		lgParam.Param.NumOfPktsRecvAtaTime = pID_NumOfPktsRecvAtaTime->Int;
		lgParam.ParamFlags |= LG_PARAM_FLAG_NumOfPktsRecvAtaTime;
		printf(" NumOfPktsRecvAtaTime          : %d (0x%08x) \n", lgParam.Param.NumOfPktsRecvAtaTime, lgParam.Param.NumOfPktsRecvAtaTime);
	}

	pID_NumOfSendBuffers = CLIGetOpt(Options[ID_NumOfSendBuffers].OptStr);
	if ((pID_NumOfSendBuffers != NULL) && (pID_NumOfSendBuffers->Value == TRUE) )	{ //Error	
		lgParam.Param.NumOfSendBuffers = pID_NumOfSendBuffers->Int;
		lgParam.ParamFlags |= LG_PARAM_FLAG_NumOfSendBuffers;
		printf(" NumOfSendBuffers          : %d (0x%08x) \n", lgParam.Param.NumOfSendBuffers, lgParam.Param.NumOfSendBuffers);
	}

	// Prepare Command and execute it on driver, 
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_SET_LG_TUNING_PARAM,
									&lgParam,		// LPVOID lpInBuffer,
									sizeof(lgParam),// DWORD nInBufferSize,
									NULL,				// LPVOID lpOutBuffer,
									0,					// DWORD nOutBufferSize,
									&retLength);		// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_SET_LG_TUNING_PARAM", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_SetParms].CmdStr);

ret: 
	return;
} // SetParms()

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
void	DiskList (void)
{
	DWORD						status;
	ULONG						i, size, totalDisks;
	DWORD						retLength	= 0;
	ATTACHED_DISK_INFO_LIST		attachDiskInfoList;
	PATTACHED_DISK_INFO_LIST	pAttachDiskInfoList = NULL;
	PATTACHED_DISK_INFO			pAttachDiskInfo		= NULL;

	totalDisks = 0;
	RtlZeroMemory( &attachDiskInfoList, sizeof(attachDiskInfoList));
	attachDiskInfoList.NumOfDisks = 0;
	
	// Get Total Attached Disk Count by sending IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_GET_All_ATTACHED_DISKINFO,
									&attachDiskInfoList,			// LPVOID lpInBuffer,
									sizeof(attachDiskInfoList),		// DWORD nInBufferSize,
									&attachDiskInfoList,			// LPVOID lpOutBuffer,
									sizeof(attachDiskInfoList),		// DWORD nOutBufferSize,
									&retLength);					// LPDWORD lpBytesReturned,
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_All_ATTACHED_DISKINFO To Get DiskCounts", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      
	totalDisks = attachDiskInfoList.NumOfDisks;

	printf("\n Result: \n");
	printf(" TotalNumOfAttachedDisks  : %d (0x%08x) \n", totalDisks, totalDisks);
	if (totalDisks == 0)
	{
		printf(" Nothing To Display, Count is zero...  \n");
		goto ret;
	}

	size = MAX_SIZE_ATTACH_DISK_INFOLIST( totalDisks );
	pAttachDiskInfoList = calloc(1, size);
	if (pAttachDiskInfoList == NULL)
	{
		printf("Error: calloc(size %d) failed. GetLastError() %d \n", size, GetLastError());
		GetErrorText();
		goto ret;
	}
	// Initialize it 
	RtlZeroMemory( pAttachDiskInfoList, size );

	pAttachDiskInfoList->NumOfDisks = totalDisks;

	// Get All Attached Disk Info List by sending IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_GET_All_ATTACHED_DISKINFO,
									pAttachDiskInfoList,			// LPVOID lpInBuffer,
									size,							// DWORD nInBufferSize,
									pAttachDiskInfoList,			// LPVOID lpOutBuffer,
									size,							// DWORD nOutBufferSize,
									&retLength);					// LPDWORD lpBytesReturned,
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_All_ATTACHED_DISKINFO To Get AllDiskInfoList", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      
	
	pAttachDiskInfo = &pAttachDiskInfoList->DiskInfo[0];
	for(i=0; i < pAttachDiskInfoList->NumOfDisks; i++)
	{
		printf("\n DiskDevice Entry: %d \n", i);
		// DisplayLGStatsInfo( pLgStats );
		DisplayDiskInfo( pAttachDiskInfo );
		pAttachDiskInfo = (PATTACHED_DISK_INFO) ((ULONG) pAttachDiskInfo + sizeof(ATTACHED_DISK_INFO));
	}
	
	printf("\n /%s command Executed Successfully. \n", Commands[CID_DiskList].CmdStr);

ret: 
	if (pAttachDiskInfoList)
		free(pAttachDiskInfoList);
	return;
} // DiskList()

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
void	AddSock(void)
{
	DWORD				status, retLength;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole, pSrcIp, pDstIp, pSrcPort, pDstPort;
	CONNECTION_DETAILS	conn_Detail;
	
	retLength = 0;
	RtlZeroMemory(&conn_Detail, sizeof(conn_Detail) );
	
	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	// Get -SrcIP s parameter
	pSrcIp = CLIGetOpt(Options[ID_SrcIP].OptStr);
	if ((pSrcIp == NULL) || (pSrcIp->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_SrcIP].OptStr, Options[ID_SrcIP].Desc);
		goto ret;
	}
	// Get -DstIP s parameter
	pDstIp = CLIGetOpt(Options[ID_DstIP].OptStr);
	if ((pDstIp == NULL) || (pDstIp->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DstIP].OptStr, Options[ID_DstIP].Desc);
		goto ret;
	}
	// Get -SrcPort n parameter
	pSrcPort = CLIGetOpt(Options[ID_SrcPort].OptStr);
	if ((pSrcPort == NULL) || (pSrcPort->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_SrcPort].OptStr, Options[ID_SrcPort].Desc);
		goto ret;
	}
	// Get -DstPort s parameter
	pDstPort = CLIGetOpt(Options[ID_DstPort].OptStr);
	if ((pDstPort == NULL) || (pDstPort->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DstPort].OptStr, Options[ID_DstPort].Desc);
		goto ret;
	}

	printf("\n Input: \n");
	printf(" LGNum    : %d (0x%08x) \n", pLGNum->Int, pLGNum->Int);
	printf(" SrcIP    : %s  \n", pSrcIp->String);
	printf(" SrcPort  : %d (0x%04x) \n", pSrcPort->Int, pSrcPort->Int);
	printf(" DstIP    : %s  \n", pDstIp->String);
	printf(" DstPort  : %d (0x%04x) \n", pDstPort->Int, pDstPort->Int);
	
	// Prepare Input structure

	conn_Detail.nConnections = 1;
	conn_Detail.lgnum = pLGNum->Int;
	conn_Detail.lgCreationRole = pLGCreationRole->Int;
	conn_Detail.ConnectionDetails[0].ipLocalAddress.in_addr		= inet_addr(pSrcIp->String);
	conn_Detail.ConnectionDetails[0].ipLocalAddress.sin_port	= htons((USHORT)pSrcPort->Int);
	conn_Detail.ConnectionDetails[0].ipRemoteAddress.in_addr	= inet_addr(pDstIp->String);
	conn_Detail.ConnectionDetails[0].ipRemoteAddress.sin_port	= htons((USHORT)pDstPort->Int);

	// Send IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									SFTK_IOCTL_TCP_ADD_CONNECTIONS,
									&conn_Detail,				// LPVOID lpInBuffer,
									sizeof(conn_Detail),		// DWORD nInBufferSize,
									NULL,					// LPVOID lpInBuffer,
									0,						// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"SFTK_IOCTL_TCP_ADD_CONNECTIONS", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_AddSock].CmdStr);

ret: 
	return;
} // _AddSock()

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
void	StartPmd(void)
{
	DWORD				status, retLength;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole, pMaxSize, pMaxSendPkts, pMaxRecvPkts;
	ULONG				maxSize, maxSendPkts, maxRecvPkts;
	SM_INIT_PARAMS		sm_Params;
	
	retLength = 0;
	RtlZeroMemory(&sm_Params, sizeof(sm_Params) );
	
	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	maxSize		= DEFAULT_MAX_TRANSFER_UNIT_SIZE;
	maxSendPkts	= DEFAULT_MAX_SEND_BUFFERS;
	maxRecvPkts = DEFAULT_MAX_RECEIVE_BUFFERS;

	// Get Optional -MaxSize n parameter
	pMaxSize = CLIGetOpt(Options[ID_MaxSize].OptStr);
	if ((pMaxSize != NULL) && (pMaxSize->Value == TRUE) )	{ //Error	
		maxSize = pMaxSize->Int;
	}
	// Get Optional -MaxSendPkts n parameter
	pMaxSendPkts = CLIGetOpt(Options[ID_MaxSendPkts].OptStr);
	if ((pMaxSendPkts != NULL) && (pMaxSendPkts->Value == TRUE) )	{ //Error	
		maxSendPkts = pMaxSendPkts->Int;
	}
	// Get Optional --MaxRecvPkts n parameter
	pMaxRecvPkts = CLIGetOpt(Options[ID_MaxRecvPkts].OptStr);
	if ((pMaxRecvPkts != NULL) && (pMaxRecvPkts->Value == TRUE) )	{ //Error	
		maxRecvPkts = pMaxRecvPkts->Int;
	}

	sm_Params.lgnum						= pLGNum->Int;
	sm_Params.lgCreationRole			= pLGCreationRole->Int;
	sm_Params.nSendWindowSize			= maxSize;					// DEFAULT_MAX_TRANSFER_UNIT_SIZE = 256 K
	sm_Params.nMaxNumberOfSendBuffers	= (USHORT) maxSendPkts;				// 5 defined in ftdio.h
	sm_Params.nReceiveWindowSize		= maxSize;	
	sm_Params.nMaxNumberOfReceiveBuffers= (USHORT) maxRecvPkts;				// 5 defined in ftdio.h
	sm_Params.nChunkSize				= 0;	 
	sm_Params.nChunkDelay				= 0;	 

	printf("\n Input: \n");
	printf(" LGNum                      : %d (0x%08x) \n", sm_Params.lgnum, sm_Params.lgnum);
	printf(" nSendWindowSize            : %d (0x%08x) \n", sm_Params.nSendWindowSize, sm_Params.nSendWindowSize);
	printf(" nMaxNumberOfSendBuffers    : %d (0x%08x) \n", sm_Params.nMaxNumberOfSendBuffers, sm_Params.nMaxNumberOfSendBuffers);
	printf(" nReceiveWindowSize         : %d (0x%08x) \n", sm_Params.nReceiveWindowSize, sm_Params.nReceiveWindowSize);
	printf(" nMaxNumberOfReceiveBuffers : %d (0x%08x) \n", sm_Params.nMaxNumberOfReceiveBuffers, sm_Params.nMaxNumberOfReceiveBuffers);
	printf(" nChunkSize                 : %d (0x%08x) \n", sm_Params.nChunkSize, sm_Params.nChunkSize);
	printf(" nChunkDelay                : %d (0x%08x) \n", sm_Params.nChunkDelay, sm_Params.nChunkDelay);

	// Send IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									SFTK_IOCTL_START_PMD,
									&sm_Params,				// LPVOID lpInBuffer,
									sizeof(sm_Params),		// DWORD nInBufferSize,
									NULL,					// LPVOID lpInBuffer,
									0,						// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"SFTK_IOCTL_START_PMD", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_StartPmd].CmdStr);

ret: 
	return;
} // StartPmd()

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
void	StopPmd(void)
{
	DWORD				status, retLength;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole;
	ftd_state_t			ftdState;
	
	retLength = 0;

	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	printf("\n Input: \n");
	printf(" LGNum                      : %d (0x%08x) \n", pLGNum->Int,pLGNum->Int);

	ftdState.lg_num = pLGNum->Int;
	ftdState.lgCreationRole = pLGCreationRole->Int;

	// Send IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									SFTK_IOCTL_STOP_PMD,
									&ftdState,				// LPVOID lpInBuffer,
									sizeof(ftdState),		// DWORD nInBufferSize,
									NULL,					// LPVOID lpInBuffer,
									0,						// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"SFTK_IOCTL_STOP_PMD", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_StopPmd].CmdStr);

ret: 
	return;
} // StopPmd()

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
void	DelSock(void)
{
	DWORD				status, retLength;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole, pSrcIp, pDstIp, pSrcPort, pDstPort;
	CONNECTION_DETAILS	conn_Detail;
	
	retLength = 0;
	RtlZeroMemory(&conn_Detail, sizeof(conn_Detail) );
	
	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	// Get -SrcIP s parameter
	pSrcIp = CLIGetOpt(Options[ID_SrcIP].OptStr);
	if ((pSrcIp == NULL) || (pSrcIp->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_SrcIP].OptStr, Options[ID_SrcIP].Desc);
		goto ret;
	}
	// Get -DstIP s parameter
	pDstIp = CLIGetOpt(Options[ID_DstIP].OptStr);
	if ((pDstIp == NULL) || (pDstIp->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DstIP].OptStr, Options[ID_DstIP].Desc);
		goto ret;
	}
	// Get -SrcPort n parameter
	pSrcPort = CLIGetOpt(Options[ID_SrcPort].OptStr);
	if ((pSrcPort == NULL) || (pSrcPort->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_SrcPort].OptStr, Options[ID_SrcPort].Desc);
		goto ret;
	}
	// Get -DstPort s parameter
	pDstPort = CLIGetOpt(Options[ID_DstPort].OptStr);
	if ((pDstPort == NULL) || (pDstPort->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DstPort].OptStr, Options[ID_DstPort].Desc);
		goto ret;
	}

	printf("\n Input: \n");
	printf(" LGNum    : %d (0x%08x) \n", pLGNum->Int, pLGNum->Int);
	printf(" SrcIP    : %s  \n", pSrcIp->String);
	printf(" SrcPort  : %d (0x%04x) \n", pSrcPort->Int, pSrcPort->Int);
	printf(" DstIP    : %s  \n", pDstIp->String);
	printf(" DstPort  : %d (0x%04x) \n", pDstPort->Int, pDstPort->Int);
	
	// Prepare Input structure
	conn_Detail.nConnections = 1;
	conn_Detail.lgnum = pLGNum->Int;
	conn_Detail.lgCreationRole = pLGCreationRole->Int;
	conn_Detail.ConnectionDetails[0].ipLocalAddress.in_addr		= inet_addr(pSrcIp->String);
	conn_Detail.ConnectionDetails[0].ipLocalAddress.sin_port	= htons((USHORT)pSrcPort->Int);
	conn_Detail.ConnectionDetails[0].ipRemoteAddress.in_addr	= inet_addr(pDstIp->String);
	conn_Detail.ConnectionDetails[0].ipRemoteAddress.sin_port	= htons((USHORT)pDstPort->Int);

	// Send IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									SFTK_IOCTL_TCP_REMOVE_CONNECTIONS,
									&conn_Detail,				// LPVOID lpInBuffer,
									sizeof(conn_Detail),		// DWORD nInBufferSize,
									NULL,					// LPVOID lpInBuffer,
									0,						// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"SFTK_IOCTL_TCP_REMOVE_CONNECTIONS", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_DelSock].CmdStr);

ret: 
	return;
} // DelSock()


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
void	EnableSock(void)
{
	DWORD				status, retLength;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole, pSrcIp, pDstIp, pSrcPort, pDstPort;
	CONNECTION_DETAILS	conn_Detail;
	
	retLength = 0;
	RtlZeroMemory(&conn_Detail, sizeof(conn_Detail) );
	
	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	// Get -SrcIP s parameter
	pSrcIp = CLIGetOpt(Options[ID_SrcIP].OptStr);
	if ((pSrcIp == NULL) || (pSrcIp->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_SrcIP].OptStr, Options[ID_SrcIP].Desc);
		goto ret;
	}
	// Get -DstIP s parameter
	pDstIp = CLIGetOpt(Options[ID_DstIP].OptStr);
	if ((pDstIp == NULL) || (pDstIp->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DstIP].OptStr, Options[ID_DstIP].Desc);
		goto ret;
	}
	// Get -SrcPort n parameter
	pSrcPort = CLIGetOpt(Options[ID_SrcPort].OptStr);
	if ((pSrcPort == NULL) || (pSrcPort->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_SrcPort].OptStr, Options[ID_SrcPort].Desc);
		goto ret;
	}
	// Get -DstPort s parameter
	pDstPort = CLIGetOpt(Options[ID_DstPort].OptStr);
	if ((pDstPort == NULL) || (pDstPort->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DstPort].OptStr, Options[ID_DstPort].Desc);
		goto ret;
	}

	printf("\n Input: \n");
	printf(" LGNum    : %d (0x%08x) \n", pLGNum->Int, pLGNum->Int);
	printf(" SrcIP    : %s  \n", pSrcIp->String);
	printf(" SrcPort  : %d (0x%04x) \n", pSrcPort->Int, pSrcPort->Int);
	printf(" DstIP    : %s  \n", pDstIp->String);
	printf(" DstPort  : %d (0x%04x) \n", pDstPort->Int, pDstPort->Int);
	
	// Prepare Input structure
	conn_Detail.nConnections = 1;
	conn_Detail.lgnum = pLGNum->Int;
	conn_Detail.lgCreationRole = pLGCreationRole->Int;
	conn_Detail.ConnectionDetails[0].ipLocalAddress.in_addr		= inet_addr(pSrcIp->String);
	conn_Detail.ConnectionDetails[0].ipLocalAddress.sin_port	= htons((USHORT)pSrcPort->Int);
	conn_Detail.ConnectionDetails[0].ipRemoteAddress.in_addr	= inet_addr(pDstIp->String);
	conn_Detail.ConnectionDetails[0].ipRemoteAddress.sin_port	= htons((USHORT)pDstPort->Int);

	// Send IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									SFTK_IOCTL_TCP_ENABLE_CONNECTIONS,
									&conn_Detail,				// LPVOID lpInBuffer,
									sizeof(conn_Detail),		// DWORD nInBufferSize,
									NULL,					// LPVOID lpInBuffer,
									0,						// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"SFTK_IOCTL_TCP_ENABLE_CONNECTIONS", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_EnableSock].CmdStr);

ret: 
	return;
} // EnableSock()

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
void	DisableSock(void)
{
	DWORD				status, retLength;
	PCMD_LINE_OPT		pLGNum, pLGCreationRole, pSrcIp, pDstIp, pSrcPort, pDstPort;
	CONNECTION_DETAILS	conn_Detail;
	
	retLength = 0;
	RtlZeroMemory(&conn_Detail, sizeof(conn_Detail) );
	
	// Get -LGNum n parameter
	pLGNum = CLIGetOpt(Options[ID_LGNum].OptStr);
	if ((pLGNum == NULL) || (pLGNum->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGNum].OptStr, Options[ID_LGNum].Desc);
		goto ret;
	}

	// Get -LGCreationRole n parameter
	pLGCreationRole = CLIGetOpt(Options[ID_LGCreationRole].OptStr);
	// pLGNum->Int
	if ((pLGCreationRole == NULL) || (pLGCreationRole->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_LGCreationRole].OptStr, Options[ID_LGCreationRole].Desc);
		goto ret;
	}

	// Get -SrcIP s parameter
	pSrcIp = CLIGetOpt(Options[ID_SrcIP].OptStr);
	if ((pSrcIp == NULL) || (pSrcIp->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_SrcIP].OptStr, Options[ID_SrcIP].Desc);
		goto ret;
	}
	// Get -DstIP s parameter
	pDstIp = CLIGetOpt(Options[ID_DstIP].OptStr);
	if ((pDstIp == NULL) || (pDstIp->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DstIP].OptStr, Options[ID_DstIP].Desc);
		goto ret;
	}
	// Get -SrcPort n parameter
	pSrcPort = CLIGetOpt(Options[ID_SrcPort].OptStr);
	if ((pSrcPort == NULL) || (pSrcPort->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_SrcPort].OptStr, Options[ID_SrcPort].Desc);
		goto ret;
	}
	// Get -DstPort s parameter
	pDstPort = CLIGetOpt(Options[ID_DstPort].OptStr);
	if ((pDstPort == NULL) || (pDstPort->Value != TRUE) )	{ //Error	
		printf("Error '%s %s' input argument not specified. Invalid command line argument.\n", 
						Options[ID_DstPort].OptStr, Options[ID_DstPort].Desc);
		goto ret;
	}

	printf("\n Input: \n");
	printf(" LGNum    : %d (0x%08x) \n", pLGNum->Int, pLGNum->Int);
	printf(" SrcIP    : %s  \n", pSrcIp->String);
	printf(" SrcPort  : %d (0x%04x) \n", pSrcPort->Int, pSrcPort->Int);
	printf(" DstIP    : %s  \n", pDstIp->String);
	printf(" DstPort  : %d (0x%04x) \n", pDstPort->Int, pDstPort->Int);
	
	// Prepare Input structure
	conn_Detail.nConnections = 1;
	conn_Detail.lgnum = pLGNum->Int;
	conn_Detail.lgCreationRole = pLGCreationRole->Int;
	conn_Detail.ConnectionDetails[0].ipLocalAddress.in_addr		= inet_addr(pSrcIp->String);
	conn_Detail.ConnectionDetails[0].ipLocalAddress.sin_port	= htons((USHORT)pSrcPort->Int);
	conn_Detail.ConnectionDetails[0].ipRemoteAddress.in_addr	= inet_addr(pDstIp->String);
	conn_Detail.ConnectionDetails[0].ipRemoteAddress.sin_port	= htons((USHORT)pDstPort->Int);

	// Send IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									SFTK_IOCTL_TCP_DISABLE_CONNECTIONS,
									&conn_Detail,				// LPVOID lpInBuffer,
									sizeof(conn_Detail),		// DWORD nInBufferSize,
									NULL,					// LPVOID lpInBuffer,
									0,						// DWORD nInBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"SFTK_IOCTL_TCP_DISABLE_CONNECTIONS", SFTK_CTL_USER_MODE_DEVICE_NAME);
		goto ret;
	}      

	printf("\n Result: \n");
	printf(" /%s command Executed Successfully. \n", Commands[CID_DisableSock].CmdStr);

ret: 
	return;
} // DisableSock()


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
void	ParseConfigFiles(void)
{
	LList* cfglist = NULL;
	ftd_lg_cfg_t    **cfgpp;
	LONG rc = 0;

	cfglist = ftd_config_create_list();

	ftd_config_get_all(PATH_CONFIG, cfglist);

	ForEachLLElement(cfglist, cfgpp) {
		if ((rc = ftd_config_read((*cfgpp), TRUE, (*cfgpp)->lgnum, ROLEPRIMARY, 0)) < 0) {
			break;
		}
	}

	ftd_config_delete_list(cfglist);


	return;
} // ParseConfigFiles()


//
// -------------- Internal APIs definations ------------------------------
//
VOID
DisplayLGStatsInfo( PLG_STATISTICS		LgStats	)
{
	CHAR	string[256];

	printf("LG Statistics: \n");
	printf(" LgNum        : %d (0x%08x) \n", LgStats->LgNum, LgStats->LgNum);
	// printf(" Flags        : %d (0x%08x) \n", LgStats->Flags, LgStats->Flags);
	// printf(" cdev         : %d (0x%08x) \n", LgStats->cdev, LgStats->cdev);
	RtlZeroMemory(string, sizeof(string));
	GetLGStatsInString(string, LgStats->LgState);
	printf(" LgState      : %d (0x%08x) : %s \n", LgStats->LgState, LgStats->LgState, string);
	printf(" NumOfDevStats: %d (0x%08x) : %s \n", LgStats->LgState, LgStats->LgState, string);

	printf(" LG Statistics Info: \n");
	DisplayStatsInfo( &LgStats->LgStats );
} // DisplayLGStatsInfo()

VOID 
DisplayStatsInfo( PSTATISTICS_INFO StatsInfo )
{
	printf(" RdCount             : %I64d  (0x%I64x) \n", StatsInfo->RdCount, StatsInfo->RdCount);
	printf(" WrCount             : %I64d  (0x%I64x) \n", StatsInfo->WrCount, StatsInfo->WrCount);
	printf(" BlksRd              : %I64d  (0x%I64x) \n", StatsInfo->BlksRd, StatsInfo->BlksRd);
	printf(" BlksWr              : %I64d  (0x%I64x) \n", StatsInfo->BlksWr, StatsInfo->BlksWr);
	printf(" RemoteWrPending     : %I64d  (0x%I64x) \n", StatsInfo->RemoteWrPending, StatsInfo->RemoteWrPending);
	printf(" RemoteBlksWrPending : %I64d  (0x%I64x) \n", StatsInfo->RemoteBlksWrPending, StatsInfo->RemoteBlksWrPending);

	// QM
	printf("\n");
	printf(" QM_WrPending           : %I64d   \n", StatsInfo->QM_WrPending);
	printf(" QM_BlksWrPending       : %I64d  (%I64d MB) \n", StatsInfo->QM_BlksWrPending, (StatsInfo->QM_BlksWrPending / ONEMILLION) );
	printf(" QM_WrCommit            : %I64d   \n", StatsInfo->QM_WrCommit);
	printf(" QM_BlksWrCommit        : %I64d  (%I64d MB) \n", StatsInfo->QM_BlksWrCommit, (StatsInfo->QM_BlksWrCommit / ONEMILLION));
	printf(" QM_WrRefresh           : %I64d  \n", StatsInfo->QM_WrRefresh);
	printf(" QM_BlksWrRefresh       : %I64d  (%I64d MB) \n", StatsInfo->QM_BlksWrRefresh, (StatsInfo->QM_BlksWrRefresh / ONEMILLION));
	printf(" QM_WrRefreshPending    : %I64d  \n", StatsInfo->QM_WrRefreshPending);
	printf(" QM_BlksWrRefreshPending: %I64d  (%I64d MB) \n", StatsInfo->QM_BlksWrRefreshPending, (StatsInfo->QM_BlksWrRefreshPending / ONEMILLION));
	printf(" QM_WrMigrate           : %I64d  \n", StatsInfo->QM_WrMigrate);
	printf(" QM_BlksWrMigrate       : %I64d  (%I64d MB) \n", StatsInfo->QM_BlksWrMigrate, (StatsInfo->QM_BlksWrMigrate / ONEMILLION));

	// MM 
	printf("\n");
	printf(" MM_TotalMemAllocated            : %I64d  (%I64d MB) \n", StatsInfo->MM_TotalMemAllocated, (StatsInfo->MM_TotalMemAllocated / ONEMILLION) );
	printf(" MM_TotalMemIsUsed               : %I64d  (%I64d MB) \n", StatsInfo->MM_TotalMemIsUsed, (StatsInfo->MM_TotalMemIsUsed / ONEMILLION) );
	printf(" MM_LgTotalMemUsed               : %I64d  (%I64d MB) \n", StatsInfo->MM_LgTotalMemUsed, (StatsInfo->MM_LgTotalMemUsed / ONEMILLION) );
	printf(" MM_LgMemUsed                    : %I64d  (%I64d MB) \n", StatsInfo->MM_LgMemUsed, (StatsInfo->MM_LgMemUsed / ONEMILLION) );
	printf(" MM_LgTotalRawMemUsed            : %I64d  (%I64d MB) \n", StatsInfo->MM_LgTotalRawMemUsed, (StatsInfo->MM_LgTotalRawMemUsed / ONEMILLION) );
	printf(" MM_LgRawMemUsed                 : %I64d  (%I64d MB) \n", StatsInfo->MM_LgRawMemUsed, (StatsInfo->MM_LgRawMemUsed / ONEMILLION) );
	printf(" MM_TotalOSMemUsed               : %I64d  (%I64d MB) \n", StatsInfo->MM_TotalOSMemUsed, (StatsInfo->MM_TotalOSMemUsed / ONEMILLION) );
	printf(" MM_OSMemUsed                    : %I64d  (%I64d MB) \n", StatsInfo->MM_OSMemUsed, (StatsInfo->MM_OSMemUsed / ONEMILLION) );
	printf(" MM_TotalNumOfMdlLocked          : %I64d  \n", StatsInfo->MM_TotalNumOfMdlLocked);
	printf(" MM_TotalSizeOfMdlLocked         : %I64d  (%I64d MB) \n", StatsInfo->MM_TotalSizeOfMdlLocked, (StatsInfo->MM_TotalSizeOfMdlLocked / ONEMILLION) );
	printf(" MM_TotalNumOfMdlLockedAtPresent : %I64d  \n", StatsInfo->MM_TotalNumOfMdlLockedAtPresent);
	printf(" MM_TotalSizeOfMdlLockedAtPresent: %I64d  (%I64d MB) \n", StatsInfo->MM_TotalSizeOfMdlLockedAtPresent, (StatsInfo->MM_TotalSizeOfMdlLockedAtPresent / ONEMILLION) );

	// Resfresh
	printf("\n");
	printf(" NumOfBlksPerBit        : %d  (0x%x) (ChunkSize in Bytes %d) \n", StatsInfo->NumOfBlksPerBit, StatsInfo->NumOfBlksPerBit, (StatsInfo->NumOfBlksPerBit * 512));
	printf(" TotalBitsDirty         : %I64d  (0x%I64x) \n", StatsInfo->TotalBitsDirty, StatsInfo->TotalBitsDirty);
	printf(" TotalBitsDone          : %I64d  (0x%I64x) \n", StatsInfo->TotalBitsDone, StatsInfo->TotalBitsDone);
	
	if (StatsInfo->TotalBitsDirty > 0)
	{
		ULONG percentage = 0;
		percentage = (ULONG) ( (StatsInfo->TotalBitsDone * 100) / StatsInfo->TotalBitsDirty);
		printf(" ***** Reftresh_Percentage    : %d \n", percentage ); 
	}
	printf(" CurrentRefreshBitIndex : %d  (0x%x) \n", StatsInfo->CurrentRefreshBitIndex, StatsInfo->CurrentRefreshBitIndex);

	// pstore update
	printf("\n");
	printf(" PstoreLrdbUpdateCounts : %I64d  (0x%I64x) \n", StatsInfo->PstoreLrdbUpdateCounts, StatsInfo->PstoreLrdbUpdateCounts);
	printf(" PstoreHrdbUpdateCounts : %I64d  (0x%I64x) \n", StatsInfo->PstoreHrdbUpdateCounts, StatsInfo->PstoreHrdbUpdateCounts);
	printf(" PstoreFlushCounts      : %I64d  (0x%I64x) \n", StatsInfo->PstoreFlushCounts, StatsInfo->PstoreFlushCounts);
	printf(" PstoreBitmapFlushCounts: %I64d  (0x%I64x) \n", StatsInfo->PstoreBitmapFlushCounts, StatsInfo->PstoreBitmapFlushCounts);

	// Socket stats info
	printf("\n");
	printf(" SM_PacketsSent                  : %I64d  (0x%I64x) \n", StatsInfo->SM_PacketsSent, StatsInfo->SM_PacketsSent);
	printf(" SM_BytesSent                    : %I64d  (0x%I64x) \n", StatsInfo->SM_BytesSent, StatsInfo->SM_BytesSent);
	printf(" SM_EffectiveBytesSent           : %I64d  (0x%I64x) \n", StatsInfo->SM_EffectiveBytesSent, StatsInfo->SM_EffectiveBytesSent);
	printf(" SM_PacketsReceived              : %I64d  (0x%I64x) \n", StatsInfo->SM_PacketsReceived, StatsInfo->SM_PacketsReceived);
	printf(" SM_BytesReceived                : %I64d  (0x%I64x) \n", StatsInfo->SM_BytesReceived, StatsInfo->SM_BytesReceived);
	printf(" SM_AverageSendPacketSize        : %d  (0x%x) \n", StatsInfo->SM_AverageSendPacketSize, StatsInfo->SM_AverageSendPacketSize);
	printf(" SM_MaximumSendPacketSendSize    : %d  (0x%x) \n", StatsInfo->SM_MaximumSendPacketSize, StatsInfo->SM_MaximumSendPacketSize);
	printf(" SM_MinimumSendPacketSize        : %d  (0x%x) \n", StatsInfo->SM_MinimumSendPacketSize, StatsInfo->SM_MinimumSendPacketSize);
	printf(" SM_AverageReceivedPacketSize    : %d  (0x%x) \n", StatsInfo->SM_AverageReceivedPacketSize, StatsInfo->SM_AverageReceivedPacketSize);
	printf(" SM_MaximumReceivedPacketSendSize: %d  (0x%x) \n", StatsInfo->SM_MaximumReceivedPacketSize, StatsInfo->SM_MaximumReceivedPacketSize);
	printf(" SM_MinimumReceivedPacketSize    : %d  (0x%x) \n", StatsInfo->SM_MinimumReceivedPacketSize, StatsInfo->SM_MinimumReceivedPacketSize);
	printf(" SM_AverageSendDelay             : %I64d  (0x%I64x) \n", StatsInfo->SM_AverageSendDelay, StatsInfo->SM_AverageSendDelay);
	printf(" SM_Throughput                   : %I64d  (0x%I64x) \n", StatsInfo->SM_Throughput, StatsInfo->SM_Throughput);

} // DisplayStatsInfo()

VOID 
DisplayDiskInfo( PATTACHED_DISK_INFO AttachDiskInfo )
{
	WCHAR	wstring[512];
	ULONG	valuesInMB, valuesInGB, i;

	printf(" DiskNumber                   : %d  (0x%08x) \n", AttachDiskInfo->DiskNumber, AttachDiskInfo->DiskNumber);
	printf(" PhysicalDeviceNameBuffer     : %S \n", AttachDiskInfo->PhysicalDeviceNameBuffer);
	printf(" bValidName                   : %d \n", AttachDiskInfo->bValidName);

	RtlZeroMemory(wstring, sizeof(wstring));
	wcsncpy( wstring, AttachDiskInfo->StorageManagerName, sizeof(AttachDiskInfo->StorageManagerName));
	printf(" StorageManagerName           : %S  \n", wstring);
	
	printf(" bStorage_device_Number       : %d \n", AttachDiskInfo->bStorage_device_Number);
	printf(" DeviceType                   : %d  (0x%08x) \n", AttachDiskInfo->DeviceType, AttachDiskInfo->DeviceType);
	printf(" DeviceNumber                 : %d  (0x%08x) \n", AttachDiskInfo->DeviceNumber, AttachDiskInfo->DeviceNumber);
	printf(" PartitionNumber              : %d  (0x%08x) \n", AttachDiskInfo->PartitionNumber, AttachDiskInfo->PartitionNumber);
	RtlZeroMemory(wstring, sizeof(wstring));
	wcsncpy( wstring, AttachDiskInfo->DiskPartitionName, sizeof(AttachDiskInfo->DiskPartitionName));
	printf(" DiskPartitionName            : %S  \n", wstring);

	printf(" bVolumeNumber                : %d \n", AttachDiskInfo->bVolumeNumber);
	printf(" VolumeNumber                 : %d  (0x%08x) \n", AttachDiskInfo->VolumeNumber, AttachDiskInfo->VolumeNumber);
	RtlZeroMemory(wstring, sizeof(wstring));
	wcsncpy( wstring, AttachDiskInfo->VolumeManagerName, sizeof(AttachDiskInfo->VolumeManagerName));
	printf(" VolumeManagerName            : %S  \n", wstring);

	printf(" bDiskVolumeName              : %d \n", AttachDiskInfo->bDiskVolumeName);
	RtlZeroMemory(wstring, sizeof(wstring));
	wcsncpy( wstring, AttachDiskInfo->DiskVolumeName, sizeof(AttachDiskInfo->DiskVolumeName));
	printf(" DiskVolumeName               : %S  \n", wstring);

	printf(" bUniqueVolumeId              : %d \n", AttachDiskInfo->bUniqueVolumeId);
	printf(" UniqueIdLength               : %d  (0x%08x) \n", AttachDiskInfo->UniqueIdLength, AttachDiskInfo->UniqueIdLength);
	printf(" UniqueId                     : ");
	for (i=0;i < AttachDiskInfo->UniqueIdLength;i++)
		printf("%02X",AttachDiskInfo->UniqueId[i]);
	printf("\n");
	// printf(" UniqueId                     : %s  \n", AttachDiskInfo->UniqueId);

	printf(" bSuggestedDriveLetter        : %d \n", AttachDiskInfo->bSuggestedDriveLetter);
	printf(" UseOnlyIfThereAreNoOtherLinks: %d \n", AttachDiskInfo->UseOnlyIfThereAreNoOtherLinks);
	printf(" NameLength                   : %d  (0x%04x) \n", AttachDiskInfo->NameLength, AttachDiskInfo->NameLength);
	RtlZeroMemory(wstring, sizeof(wstring));
	wcsncpy( wstring, AttachDiskInfo->SuggestedDriveName, sizeof(AttachDiskInfo->SuggestedDriveName));
	printf(" SuggestedDriveName           : %S  \n", wstring);

	printf(" bSignatureUniqueVolumeId     : %d \n", AttachDiskInfo->bSignatureUniqueVolumeId);
	printf(" SignatureUniqueIdLength      : %d  (0x%08x) \n", AttachDiskInfo->SignatureUniqueIdLength, AttachDiskInfo->SignatureUniqueIdLength);
	printf(" SignatureUniqueId            : %s  \n", AttachDiskInfo->SignatureUniqueId);

	printf(" IsVolumeFtVolume             : %d \n", AttachDiskInfo->IsVolumeFtVolume);
	printf(" pRawDiskDevice               : (0x%08x) \n", AttachDiskInfo->pRawDiskDevice, AttachDiskInfo->pRawDiskDevice);
	printf(" Signature                    : %d  (0x%08x) \n", AttachDiskInfo->Signature, AttachDiskInfo->Signature);

	valuesInMB = valuesInGB = 0;
	if (AttachDiskInfo->StartingOffset.QuadPart > 0)
	{
		valuesInMB = (ULONG) (AttachDiskInfo->StartingOffset.QuadPart / (1024*1024));
		valuesInGB = valuesInMB / (1024);
	}
	printf(" StartingOffset               : %I64d  (0x%I64x) (In MB: %d) (In GB: %d)\n", AttachDiskInfo->StartingOffset.QuadPart, AttachDiskInfo->StartingOffset.QuadPart, valuesInMB, valuesInGB);

	valuesInMB = valuesInGB = 0;
	if (AttachDiskInfo->PartitionLength.QuadPart > 0)
	{
		valuesInMB = (ULONG) (AttachDiskInfo->PartitionLength.QuadPart / (1024*1024));
		valuesInGB = valuesInMB / (1024);
	}
	printf(" PartitionLength              : %I64d  (0x%I64x) (In MB: %d) (In GB: %d)\n", AttachDiskInfo->PartitionLength.QuadPart, AttachDiskInfo->PartitionLength.QuadPart, valuesInMB, valuesInGB);

	printf(" SftkDev                      : %d  (0x%08x) \n", AttachDiskInfo->SftkDev, AttachDiskInfo->SftkDev);
	printf(" cdev                         : %d  (0x%08x) \n", AttachDiskInfo->cdev, AttachDiskInfo->cdev);
	printf(" LGNum                        : %d  (0x%08x) \n", AttachDiskInfo->LGNum, AttachDiskInfo->LGNum);
	
} // DisplayDiskInfo()


VOID
DisplayDevStatsInfo( PDEV_STATISTICS	DevStats )
{
	printf("Device Statistics: \n");
	printf(" LgNum     : %d (0x%08x) \n", DevStats->LgNum, DevStats->LgNum);
	printf(" cdev      : %d (0x%08x) \n", DevStats->cdev, DevStats->cdev);
	printf(" Disksize  : %d (0x%08x) in Sectors (%I64d in Bytes) \n", DevStats->Disksize, DevStats->Disksize, (UINT64) (DevStats->Disksize * 512) );
	printf(" Dev Statistics Info: \n");
	DisplayStatsInfo( &DevStats->DevStats );
} // DisplayLGStatsInfo()


VOID
GetLGStatsInString( PCHAR String, ULONG State)
{
	switch(State)
	{
		case SFTK_MODE_PASSTHRU		:	strcpy(String, "PASSTHRU     ");  break;
		case SFTK_MODE_TRACKING		:	strcpy(String, "TRACKING     ");  break;
		case SFTK_MODE_NORMAL		:	strcpy(String, "NORMAL       ");  break;
		case SFTK_MODE_FULL_REFRESH	:	strcpy(String, "FULL_REFRESH ");  break;
		case SFTK_MODE_SMART_REFRESH:	strcpy(String, "SMART_REFRESH");  break;
		case SFTK_MODE_BACKFRESH	:	strcpy(String, "BACKFRESH    ");  break;
		default:						sprintf(String, "Unknown State Values 0x%08x (%d) !!",State, State);  break;
	}
} // GetLGStatsInString()

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
Sftk_Get_TotalLgCount(ROLE_TYPE RoleType)
{
	DWORD	status;
	DWORD	retLength = 0;
	ULONG	totalLg = 0;

	totalLg = RoleType;  // Set if PRIMARY or SECONDARY

	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_GET_NUM_GROUPS,
									&totalLg,				// LPVOID lpInBuffer,
									sizeof(totalLg),		// DWORD nInBufferSize,
									&totalLg,				// LPVOID lpOutBuffer,
									sizeof(totalLg),		// DWORD nOutBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_NUM_GROUPS", SFTK_CTL_USER_MODE_DEVICE_NAME);
		return 0;
	}      

	return totalLg;
} // Sftk_Get_TotalLgCount()

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
Sftk_Get_TotalDevCount()
{
	DWORD			status;
	ftd_devnum_t	devNum;
	DWORD			retLength = 0;
	ULONG			totalDev = 0;

	RtlZeroMemory(&devNum, sizeof(devNum));
	status = sftk_DeviceIoControl(	NULL, SFTK_CTL_USER_MODE_DEVICE_NAME,FTD_GET_DEVICE_NUMS, 
									&devNum,	sizeof(devNum),			// Input info 
									&devNum,	sizeof(devNum),			// OutPut Info
									&retLength );			
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n","FTD_GET_DEVICE_NUMS", SFTK_CTL_USER_MODE_DEVICE_NAME);
		return 0;
	}      
	totalDev = devNum.b_major;

	return totalDev;
} // Sftk_Get_TotalDevCount()

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
Sftk_Get_TotalLgDevCount(ULONG LgNum, ROLE_TYPE RoleType)
{
	DWORD	status;
	DWORD	retLength = 0;
	ftd_state_t ftdState;

	ftdState.lg_num = LgNum;
	ftdState.lgCreationRole = RoleType;

	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_GET_NUM_DEVICES,
									&ftdState,				// LPVOID lpInBuffer,
									sizeof(ftdState),		// DWORD nInBufferSize,
									&ftdState,				// LPVOID lpOutBuffer,
									sizeof(ftdState),		// DWORD nOutBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_NUM_DEVICES", SFTK_CTL_USER_MODE_DEVICE_NAME);
		return 0;
	}      

	return ftdState.lg_num;
} // Sftk_Get_TotalLgDevCount()

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
Sftk_Get_AllStatsInfo( PALL_LG_STATISTICS All_LgStats, ULONG Size )
{
	DWORD			status;
	DWORD			retLength = 0;

	status = sftk_DeviceIoControl(	NULL, SFTK_CTL_USER_MODE_DEVICE_NAME,FTD_GET_ALL_STATS_INFO, 
									All_LgStats,	Size,			// Input info 
									All_LgStats,	Size,			// OutPut Info
									&retLength );			
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. LastError %d\n",
							"FTD_GET_ALL_STATS_INFO", SFTK_CTL_USER_MODE_DEVICE_NAME, GetLastError());
		GetErrorText();
		return status;
	}
		
	return NO_ERROR;
} // Sftk_Get_AllStatsInfo()

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
Sftk_Get_LGStatsInfo( PLG_STATISTICS LgStats, ULONG Size )
{
	DWORD			status;
	DWORD			retLength = 0;

	status = sftk_DeviceIoControl(	NULL, SFTK_CTL_USER_MODE_DEVICE_NAME,FTD_GET_LG_STATS_INFO, 
									LgStats,	Size,			// Input info 
									LgStats,	Size,			// OutPut Info
									&retLength );			
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. LastError %d\n",
							"FTD_GET_LG_STATS_INFO", SFTK_CTL_USER_MODE_DEVICE_NAME, GetLastError());
		GetErrorText();
		return status;
	}
		
	return NO_ERROR;
} // Sftk_Get_LGStatsInfo()


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
Sftk_Get_LGInfo( ULONG LgNum, ROLE_TYPE RoleType, stat_buffer_t *Statebuff, ftd_lg_info_t *LgInfo)
{
	DWORD	status;
	DWORD	retLength = 0;
	
	memset( Statebuff, 0, sizeof(stat_buffer_t));
	memset( LgInfo, 0, sizeof(ftd_lg_info_t));

	Statebuff->lg_num	= LgNum;	// Not Used in Driver
	// Statebuff->dev_num	= pDevNum->Int;	// Not Used in Driver
	// sprintf(devId, "%d_%d",pLGNum->Int,pDevNum->Int );	// LG_NUM + DEVNUM
	// Statebuff->DevId		= devId;		// Not Used in Driver

	Statebuff->len = sizeof(ftd_lg_info_t); // Total Size of buffer to get Dev info
	Statebuff->addr = (char *) LgInfo;
	Statebuff->lgCreationRole = RoleType;
			
	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_GET_GROUPS_INFO,
									Statebuff,					// LPVOID lpInBuffer,
									sizeof(stat_buffer_t),		// DWORD nInBufferSize,
									Statebuff,					// LPVOID lpOutBuffer,
									sizeof(stat_buffer_t),		// DWORD nOutBufferSize,
									&retLength);				// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: LGNum %d : DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							LgNum, "FTD_GET_GROUPS_INFO", SFTK_CTL_USER_MODE_DEVICE_NAME);
		return status;
	}
		
	return NO_ERROR;
} // Sftk_Get_LGInfo()

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
Sftk_Get_LGState( ULONG LgNum, ROLE_TYPE RoleType, ftd_state_t *StateInfo)
{
	DWORD	status;
	DWORD	retLength = 0;
	
	memset( StateInfo, 0, sizeof(ftd_state_t));
	StateInfo->lg_num = LgNum;
	StateInfo->lgCreationRole = RoleType;
	// StateInfo->state  = 0;

	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_GET_GROUP_STATE,
									StateInfo,					// LPVOID lpInBuffer,
									sizeof(ftd_state_t),		// DWORD nInBufferSize,
									StateInfo,					// LPVOID lpOutBuffer,
									sizeof(ftd_state_t),		// DWORD nOutBufferSize,
									&retLength);				// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: LGNum %d : DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							LgNum, "FTD_GET_GROUP_STATE", SFTK_CTL_USER_MODE_DEVICE_NAME);
		return status;
	}
		
	return NO_ERROR;
} // Sftk_Get_LGState()

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
 *		sends FTD_GET_GROUP_STATS to driver and LG current statistics from driver
 */
DWORD	
Sftk_Get_LGStatistics( ULONG LgNum, ROLE_TYPE RoleType, ftd_stat_t *StateInfo)
{
	DWORD	status;
	DWORD	retLength = 0;
	
	memset( StateInfo, 0, sizeof(ftd_stat_t));
	StateInfo->lgnum = LgNum;
	StateInfo->lgCreationRole = RoleType;
	// StateInfo->state  = 0;

	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_GET_GROUP_STATS,
									StateInfo,					// LPVOID lpInBuffer,
									sizeof(ftd_stat_t),		// DWORD nInBufferSize,
									StateInfo,					// LPVOID lpOutBuffer,
									sizeof(ftd_stat_t),		// DWORD nOutBufferSize,
									&retLength);				// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: LGNum %d : DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							LgNum, "FTD_GET_GROUP_STATS", SFTK_CTL_USER_MODE_DEVICE_NAME);
		return status;
	}
		
	return NO_ERROR;
} // Sftk_Get_LGStatistics()

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
Sftk_Get_LGDevStatistics( ULONG LgNum, ROLE_TYPE RoleType, ULONG Cdev,  disk_stats_t *Dev_stats)
{
	DWORD			status;
	DWORD			retLength = 0;
	stat_buffer_t   sb;

	memset( Dev_stats, 0, sizeof(disk_stats_t));
	memset( &sb, 0, sizeof(sb));

	sb.lg_num	= LgNum;
	sb.lgCreationRole = RoleType;
	sb.dev_num	= Cdev;
	sb.addr		= (char *) Dev_stats;
	sb.len		= sizeof(disk_stats_t);

	// StateInfo->state  = 0;

	status = sftk_DeviceIoControl(	NULL, 
									SFTK_CTL_USER_MODE_DEVICE_NAME,
									FTD_GET_DEVICE_STATS,
									&sb,					// LPVOID lpInBuffer,
									sizeof(sb),				// DWORD nInBufferSize,
									&sb,					// LPVOID lpOutBuffer,
									sizeof(sb),				// DWORD nOutBufferSize,
									&retLength);			// LPDWORD lpBytesReturned,

	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: LGNum %d : DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							LgNum, "FTD_GET_DEVICE_STATS", SFTK_CTL_USER_MODE_DEVICE_NAME);
		return status;
	}
		
	return NO_ERROR;
} // Sftk_Get_LGDevStatistics()

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
						OUT LPDWORD	BytesReturned)
{
	HANDLE	handle = HandleDevice;
	BOOL	bret;
	DWORD	status = NO_ERROR;

	if ( (HandleDevice == INVALID_HANDLE_VALUE) || (HandleDevice == NULL))
	{ // Open the device locally
		handle = CreateFile(	DeviceName,
								GENERIC_READ | GENERIC_WRITE,
								(FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE),
    							NULL,
    							OPEN_EXISTING,
    							0,		// FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
    							NULL);

		if(handle==INVALID_HANDLE_VALUE)
		{ // failed !!
			status = GetLastError();
			printf("Error : CreateFile( Device : '%s') Failed with SDK Error %d \n", 
						DeviceName, status);
			GetErrorText();
			goto done;
		}
	}
	else
		handle = HandleDevice;

	bret = DeviceIoControl(		handle,
								IoControlCode,
								InBuffer,
								InBufferSize,
								OutBuffer,
								OutBufferSize,
								BytesReturned,
								NULL);
	if (!bret) 
	{ // Zero indicated Failuer
		status = GetLastError(); 
		printf("Error : DeviceIoControl( Device:'%s', IOCTL %d (0x%08x)) Failed with SDK Error %d \n", 
						DeviceName, IoControlCode, IoControlCode,status);
		GetErrorText();
		goto done;
	}
	
	status = NO_ERROR;	// success
done:
	if ( !((HandleDevice == INVALID_HANDLE_VALUE) || (HandleDevice == NULL)) )
	{
		if ( !((handle == INVALID_HANDLE_VALUE) || (handle == NULL)) )
			CloseHandle(handle);
	}

	return status;
} // sftk_DeviceIoControl()

VOID
sftk_convertBytesIntoString( PCHAR	String, UINT64 Bytes)
{
	if (Bytes < KB )
		sprintf(String, "%-10I64d", Bytes);
	else
	{
		if (Bytes < MB )
			sprintf(String, "%-07I64d KB", (Bytes / KB));
		else
		{
			if (Bytes < GB )
				sprintf(String, "%-07I64d MB", (Bytes / MB) );
			else
			{
				// if (Bytes < TB )
					sprintf(String, "%-07I64d GB", (Bytes / GB) );
				// else
				//	sprintf(String, "%-07I64d TB", (Bytes / TB) );
			}

		}
	}
}

VOID
sftk_convertBytesIntoStringForMM( PCHAR	String, UINT64 Bytes)
{
	if (Bytes < KB )
		sprintf(String, "%-07I64d", Bytes);
	else
	{
		if (Bytes < MB )
			sprintf(String, "%-04I64d KB", (Bytes / KB));
		else
		{
			if (Bytes < GB )
				sprintf(String, "%-04I64d MB", (Bytes / MB) );
			else
			{
				// if (Bytes < TB )
					sprintf(String, "%-04I64d GB", (Bytes / GB) );
				// else
				//	sprintf(String, "%-07I64d TB", (Bytes / TB) );
			}

		}
	}
}