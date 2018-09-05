/*
 * lgcfg_buf_obj.cpp -- Utility functions to convert CReplicationGroup
 *                      objects to text buffer containing the text of a 
 *                      logical group configuration files (.cfg)
 *
 * Copyright (c) 2002 Fujitsu Softek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
//#include <stdafx.h>   *** cannot use precompiled header because of ftd_config.h ***

#pragma warning(disable: 4786)
#pragma warning(disable: 4503)

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#include <afx.h>
#include <afxwin.h>
#include <afxdisp.h>
#include <sstream>
#include <string>
#include <list>
#include <queue>
#include <map>

extern "C"
{
#include "ftd_config.h" //need some functions from libftd.lib 
}

#include "lgcfg_buf_obj.h"
#include "Server.h"
#include "ReplicationGroup.h"
#include "../libResMgr/ResourceManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern CResourceManager g_ResourceManager;


/*
 * Received a request to save this send this TDMF Agent configuration
 *
 * Create/Overwrite a logical group config file
 * Code inspired from .\ftdsrc\gui\configtool\Config.cpp, void CConfig::writeConfigFile().
 */
#ifndef PRIMARY_CFG_PREFIX 
#define PRIMARY_CFG_PREFIX      "p"
#endif
#ifndef SECONDARY_CFG_PREFIX
#define SECONDARY_CFG_PREFIX    "s"
#endif
#define IDS_STRING_HEADER_BREAK "#===============================================================\n"
#define IDS_STRING_OEM_FILE     "Configuration File:  "
#define IDS_STRING_OEM_VERSION  "Version "
#define IDS_STRING_UPDATE       "#\n#  Last Updated:  "
#define IDS_STRING_NOTE         "#\nNOTES:  "
#define IDS_STRING_PRIMARY_DEF  "#\n#\n# Primary System Definition:\n#\nSYSTEM-TAG:          SYSTEM-A                  PRIMARY\n"
#define IDS_STRING_HOST         "  HOST:                "
#define IDS_STRING_PSTORE       "  PSTORE:              "
#define IDS_STRING_SECONDARY    "#\n# Secondary System Definition:\n#\nSYSTEM-TAG:          SYSTEM-B                  SECONDARY\n"
#define IDS_STRING_JOURNAL      "  JOURNAL:             "
#define IDS_STRING_CHAIN_ON     "  CHAINING:             on\n"
#define IDS_STRING_CHAIN_OFF    "  CHAINING:             off\n"
//#define IDS_STRING_THROTTLE     "#\nTHROTTLE:  "
//#define IDS_STRING_ACTION_LIST  "ACTIONLIST\n"
//#define IDS_STRING_END_ACTION_LIST "\nENDACTIONLIST\n\n"
#define IDS_STRING_DEVICE_DEF   "#\n# Device Definitions:\n#\n"
#define IDS_STRING_END_CONFIG   "#\n#\n# -- End of "
#define IDS_STRING_PROFILE      "PROFILE:            "
#define IDS_STRING_REMARK       "  REMARK:  "
#define IDS_STRING_PRIMARY_SYSA "  PRIMARY:          SYSTEM-A\n"
#define IDS_STRING_OPENSTORAGE  "  OPENSTORAGE_DATA_REPLICATION-DEVICE:  "
#define IDS_STRING_DATADISK     "  DATA-DISK:        "
#define IDS_STRING_SEC_SYSB     "  SECONDARY:        SYSTEM-B\n"
#define IDS_STRING_MIRROR       "  MIRROR-DISK:      "
#define IDS_STRING_SECOND_PORT  "  SECONDARY-PORT:      "
/*
#define IDS_UNIX_STRING_HEADER_BREAK "#===============================================================\n"
#define IDS_UNIX_STRING_OEM_FILE     "Configuration File:  "
#define IDS_UNIX_STRING_OEM_VERSION  "Version "
#define IDS_UNIX_STRING_UPDATE       "#\n#  Last Updated:  "
#define IDS_UNIX_STRING_NOTE         "#\nNOTES:  "
#define IDS_UNIX_STRING_PRIMARY_DEF  "#\n#\n# Primary System Definition:\n#\nSYSTEM-TAG:          SYSTEM-A                  PRIMARY\n"
#define IDS_UNIX_STRING_HOST         "  HOST:                "
#define IDS_UNIX_STRING_PSTORE       "  PSTORE:              "
#define IDS_UNIX_STRING_SECONDARY    "#\n# Secondary System Definition:\n#\nSYSTEM-TAG:          SYSTEM-B                  SECONDARY\n"
#define IDS_UNIX_STRING_JOURNAL      "  JOURNAL:             "
#define IDS_UNIX_STRING_CHAIN_ON     "  CHAINING:             on\n"
#define IDS_UNIX_STRING_CHAIN_OFF    "  CHAINING:             off\n"
//#define IDS_UNIX_STRING_THROTTLE     "#\nTHROTTLE:  "
//#define IDS_UNIX_STRING_ACTION_LIST  "ACTIONLIST\n"
//#define IDS_UNIX_STRING_END_ACTION_LIST "\nENDACTIONLIST\n\n"
#define IDS_UNIX_STRING_DEVICE_DEF   "#\n# Device Definitions:\n#\n"
#define IDS_UNIX_STRING_END_CONFIG   "#\n#\n# -- End of "
#define IDS_UNIX_STRING_PROFILE      "PROFILE:            "
#define IDS_UNIX_STRING_REMARK       "  REMARK:  "
#define IDS_UNIX_STRING_PRIMARY_SYSA "  PRIMARY:          SYSTEM-A\n"
#define IDS_UNIX_STRING_OPENSTORAGE  "  OPENSTORAGE_DATA_REPLICATION-DEVICE:  "
#define IDS_UNIX_STRING_DATADISK     "  DATA-DISK:        "
#define IDS_UNIX_STRING_SEC_SYSB     "  SECONDARY:        SYSTEM-B\n"
#define IDS_UNIX_STRING_MIRROR       "  MIRROR-DISK:      "
#define IDS_UNIX_STRING_SECOND_PORT  "  SECONDARY-PORT:      "
*/

// ***********************************************************
/*
 * Transfer a ftd_lg_cfg_t structure to a CReplicationGroup object.
 *
 * ftd_lg_cfg_t is used by functions reading .cfg files.
 */
static 
void copy_ftd_lg_cfg_t_to_CReplicationGroup(ftd_lg_cfg_t* cfgp ,CReplicationGroup & replGrp)
{
    //todo : a completer ...
    replGrp.m_bChaining            = cfgp->chaining != 0 ? true : false;
    replGrp.m_strDescription       = cfgp->notes;
    replGrp.m_strPStoreFile        = cfgp->pstore;
    replGrp.m_nGroupNumber         = cfgp->lgnum;
    replGrp.m_strJournalDirectory  = cfgp->jrnpath;
   
    //cfgp->phostname;
    //cfgp->shostname;

    //
    //the following fields values are not available from .cfg file :
    //
    /*
    replGrp.m_bEnableCompression   = ;
    replGrp.m_bRefreshNeverTimeout = ;
    replGrp.m_bSync                = ;
    replGrp.m_nConnectionState     = ;
    replGrp.m_nJournalSize         = ;
    replGrp.m_nSyncDepth           = ;
    replGrp.m_nSyncTimeout         = ;
    */
    //
    //the following fields values can be found by looking in the CServer list 
    //
    //cfgp->phostname contains the IP addr. of the Source server;
    //cfgp->shostname contains the IP addr. of the Target server;
    //Find the IP addr among the list of CServer 
    /*
    replGrp.m_pParent = ;
    replGrp.m_pServerTarget = ;
    */
    /*
    replGrp.m_iDbSrcFk = ;
    replGrp.m_iDbTgtFk = ;
    */

    ftd_dev_cfg_t	**devpp,*devp;
    void*           pvoid;
    ForEachLLElement(cfgp->devlist, pvoid) 
    {
        CReplicationPair & pair = replGrp.AddNewReplicationPair();

        devpp = (ftd_dev_cfg_t	**)pvoid;
        devp  = *devpp;

        pair.m_nPairNumber                = devp->devid;
        pair.m_strDescription             = devp->remark;
        pair.m_DeviceSource.m_strPath     = devp->pdevname;
        pair.m_DeviceSource.m_strDriveId  = devp->pdriverid;
        pair.m_DeviceSource.m_strStartOff = devp->ppartstartoffset;
        pair.m_DeviceSource.m_strLength   = devp->ppartlength; 
        //unknown pair.m_DeviceSource.m_strFileSystem 
        pair.m_DeviceTarget.m_strPath     = devp->sdevname;
        pair.m_DeviceTarget.m_strDriveId  = devp->sdriverid;
        pair.m_DeviceTarget.m_strStartOff = devp->spartstartoffset;
        pair.m_DeviceTarget.m_strLength   = devp->spartlength; 
        //unknown pair.m_DeviceSource.m_strFileSystem 
	}
}

static 
bool read_lgcfg_file_into_buffer(const char *pFname, char **ppData, unsigned int *puiFileSize)
{
    bool ret;
    int  r;
    FILE *file;
    long filesize;

    *puiFileSize = 0;
    *ppData      = 0;

    file = fopen(pFname,"rb");
    if (file)
    {
        r = fseek( file, 0, SEEK_END );ASSERT(r==0);
        filesize = ftell( file );

        *ppData = (char*)malloc(filesize) ;
        *puiFileSize = (unsigned int)filesize;

        r = fseek( file, 0, SEEK_SET );ASSERT(r==0);
        if ( fread(*ppData,sizeof(char),filesize,file) == (size_t)filesize )
        {
            ret = true;
        }
        else 
        {
            ASSERT(0);
            ret = false;
        }
        fclose(file);
    }
    else
    {
        ret = false;
    }

    return ret;
}

// ***********************************************************
// Function name	: tdmf_create_lgcfgfile_buffer
// Description	    : Generates a buffer filled with the content of a .cfg file
//                    based on the content of the provided input arguments.
// Return type		: void 
// Argument         : CReplicationGroup *replGrp: contains configuration of the targetted logical group
// Argument         : CServer *srvrSource : contains info. defining the Source system related 
//                                              to the targetted logical group
// Argument         : CServer *srvrTarget : contains info. defining the Target system related 
//                                              to the targetted logical group
// Argument         : bool bIsPrimary : indicates if the logical group cfg information needs to be 
//                                      generated for a Source (Primary) system or for a Target 
//                                      (Secondary system)
// Argument         : char **ppCfgData : address of a ptr to be assigned to a buffer containg the 
//                                      the content of a cfg file for the targetted logical group. 
//                                      Whendone with buffer, free memory using delete [] *ppCfgData.
// Argument         : unsigned int *piCfgDataSize : addr. to contain the number of bytes available in *pCfgData.
// 
// Argument         : bool bSymmetric : indicates if symmetric must be created (source and target will be inverted).
//
// ***********************************************************
static 
void tdmf_create_lgcfgfile_buffer_Windows(  CReplicationGroup *replGrp, /*CServer *srvrSource, CServer *srvrTarget,*/ 
                                            bool bIsPrimary, char **ppCfgData, unsigned int *piCfgDataSize, bool bSymmetric )
{
	FILE		*file;
	time_t		ltime;
	char		szTime[64];
	struct tm	*gmt;
    char        szLGCfgFname[256];
    char        sztemp[512];
    char        szTempFileName[80];
    int         len;
    CServer     *srvrSource = replGrp->m_pParent;
    CServer     *srvrTarget = replGrp->m_pServerTarget;
    std::string strSourceIP,strTargetIP;

    *piCfgDataSize  = 0;
    *ppCfgData      = 0;    

    _ASSERT(srvrSource != 0);
    _ASSERT(srvrTarget != 0);

    if (srvrSource == 0 || srvrTarget == 0)
        return;

	// Set source ip adress
	if(replGrp->m_bPrimaryDHCPAdressUsed)
	{
		strSourceIP = srvrSource->m_strName;
	}
	else if(replGrp->m_bPrimaryEditedIPUsed)
		{
			strSourceIP = replGrp->m_strPrimaryEditedIP.c_str();
		}
		else
		{
			strSourceIP = srvrSource->m_vecstrIPAddress[0];
		}

	// Set target ip adress
	if(replGrp->m_bTargetDHCPAdressUsed)
	{
		strTargetIP = srvrTarget->m_strName;
	}
	else if(replGrp->m_bTargetEditedIPUsed)
		{
			strTargetIP = replGrp->m_strTargetEditedIP.c_str();
		}
		else
		{
			strTargetIP = srvrTarget->m_vecstrIPAddress[0];
		}

	if (bSymmetric)
	{
		// Invert source/target IP
		std::string strTmp = strSourceIP;
		strSourceIP = strTargetIP;
		strTargetIP = strTmp;
	}

	time( &ltime );
	gmt = localtime( &ltime );
    _snprintf(szTime, 64, "%s", asctime( gmt ));

    _snprintf( szLGCfgFname, 256, "%s%03d.cfg", bIsPrimary ? PRIMARY_CFG_PREFIX : SECONDARY_CFG_PREFIX,
               bSymmetric ? replGrp->m_nSymmetricGroupNumber : replGrp->m_nGroupNumber);

    GetTempFileName(".", // dir. for temp. files 
                    "temp_lg",                // temp. file name prefix 
                    0,                    // create unique name 
                    szTempFileName);          // buffer for name 

	file = fopen( szTempFileName, "w+");

	fwrite(IDS_STRING_HEADER_BREAK, sizeof(char), strlen(IDS_STRING_HEADER_BREAK), file);

    len = _snprintf( sztemp, 512, "#  %s %s %s \n", g_ResourceManager.GetFullProductName(), IDS_STRING_OEM_FILE, szLGCfgFname );
	fwrite( sztemp, sizeof(char), len, file);
	
    len = _snprintf( sztemp, 512, "#  %s %s %s \n", g_ResourceManager.GetFullProductName(), IDS_STRING_OEM_VERSION, replGrp->m_pParent->m_strAgentVersion.c_str() );
	fwrite( sztemp, sizeof(char), len, file);

    len = _snprintf( sztemp, 512, "%s %s", IDS_STRING_UPDATE, szTime );//szTime ends with \n\0 - see asctime()
	fwrite( sztemp, sizeof(char), len, file);
	
	fwrite(IDS_STRING_HEADER_BREAK, sizeof(char), strlen(IDS_STRING_HEADER_BREAK), file);

    len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_NOTE, replGrp->m_strDescription.c_str() );
	fwrite( sztemp, sizeof(char), len, file);
	
	// Begin System Values
	fwrite(IDS_STRING_PRIMARY_DEF, sizeof(char), strlen(IDS_STRING_PRIMARY_DEF), file);

    len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_HOST, strSourceIP.c_str() );
	fwrite( sztemp, sizeof(char), len, file);
    
    len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_PSTORE, bSymmetric ? replGrp->m_strSymmetricPStoreFile.c_str() : replGrp->m_strPStoreFile.c_str() );
	fwrite( sztemp, sizeof(char), len, file);

	fwrite( IDS_STRING_SECONDARY, sizeof(char), strlen(IDS_STRING_SECONDARY), file);
	
    //todo : PStore in Secondary ???
    //len = _snprintf( sztemp, "%s %s \n", IDS_STRING_PSTORE, replGrp->SystemsDef.szPriPStore );
	//fwrite( sztemp, sizeof(char), len, file);

    //ip_to_ipstring( replGrp->SystemsDef.iSecIP, szip );
    len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_HOST, strTargetIP.c_str() );
	fwrite( sztemp, sizeof(char), len, file);
	
	std::string strJournalDirectory = bSymmetric ? replGrp->m_strSymmetricJournalDirectory.c_str() : replGrp->m_strJournalDirectory.c_str();
	//len = strlen(srvrTarget->m_strJournalVolume);
	len = strJournalDirectory.size();
	if(len == 1)
		//strcat(replGrp->SystemsDef.szSecJournalPath, ":\\");
        strJournalDirectory += ":\\";
	else if( replGrp->m_strJournalDirectory[len - 1] == ':' )
		//replGrp->SystemsDef.szSecJournalPath[len] = '\\';
        strJournalDirectory += "\\";

    len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_JOURNAL, strJournalDirectory.c_str() );
	fwrite( sztemp, sizeof(char), len, file);
	
    len = _snprintf( sztemp, 512, "%s %d \n", IDS_STRING_SECOND_PORT, bSymmetric ? srvrSource->m_nPort : srvrTarget->m_nPort );
	fwrite( sztemp, sizeof(char), len, file);


	if(replGrp->m_bChaining)
	    fwrite( IDS_STRING_CHAIN_ON,  sizeof(char), strlen(IDS_STRING_CHAIN_ON),  file);
	else
	    fwrite( IDS_STRING_CHAIN_OFF, sizeof(char), strlen(IDS_STRING_CHAIN_OFF), file);
	// End System Values


	// Begin Throttles
	// End Throttles


	// Begin DTC devices
	fwrite(IDS_STRING_DEVICE_DEF, sizeof(char), strlen(IDS_STRING_DEVICE_DEF), file);
	// Profile section. 
	//
    int              index=1,iNbrDtc,iDtcCount;
    iDtcCount = replGrp->m_listReplicationPair.size();
    /*
    int              iDtcId;
    CReplicationPair *pair;
	for( iDtcId=0, iNbrDtc=0 ; iDtcId < 1000 && iNbrDtc < iDtcCount; iDtcId++)
	{
        if ( FALSE == replGrp->m_listReplicationPair.Lookup( iDtcId, pair ) )
            continue;


        iNbrDtc++;
    */
    //just write DTCs to file in the same order than in the list 
    std::list<CReplicationPair>::const_iterator it  = replGrp->m_listReplicationPair.begin();
    std::list<CReplicationPair>::const_iterator end = replGrp->m_listReplicationPair.end();
    iNbrDtc = 1;
	while( it != end )
    {
	char cDrive;
        std::list<CReplicationPair>::const_reference  refPair = *it;

        len = _snprintf( sztemp, 512, "%s %d \n", IDS_STRING_PROFILE, iNbrDtc++ );
	    fwrite( sztemp, sizeof(char), len, file);

        len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_REMARK, refPair.m_strDescription.c_str() );
	    fwrite( sztemp, sizeof(char), len, file);

	    fwrite(IDS_STRING_PRIMARY_SYSA, sizeof(char), strlen(IDS_STRING_PRIMARY_SYSA), file);

		if (bSymmetric)
		{
			if ( strlen( refPair.m_DeviceTarget.m_strPath.c_str() ) <= 3 )
			{     // Drive Letter
				cDrive = refPair.m_DeviceTarget.m_strPath.at(0);  
				len = _snprintf( sztemp, 512, "  DTC-DEVICE:  %c:%d \n", cDrive, refPair.m_nPairNumber );
				fwrite( sztemp, sizeof(char), len, file);
			}
			else  // Mount Point
			{
				len = _snprintf( sztemp, 512, "  DTC-DEVICE:       %s:%d \n", refPair.m_DeviceTarget.m_strPath.c_str(), refPair.m_nPairNumber );
				fwrite( sztemp, sizeof(char), len, file);
			}
		}
		else
		{
			if ( strlen( refPair.m_DeviceSource.m_strPath.c_str() ) <= 3 )
			{     // Drive Letter
				cDrive = refPair.m_DeviceSource.m_strPath.at(0);  
				len = _snprintf( sztemp, 512, "  DTC-DEVICE:  %c:%d \n", cDrive, refPair.m_nPairNumber );
				fwrite( sztemp, sizeof(char), len, file);
			}
			else  // Mount Point
			{
				len = _snprintf( sztemp, 512, "  DTC-DEVICE:       %s:%d \n", refPair.m_DeviceSource.m_strPath.c_str(), refPair.m_nPairNumber );
				fwrite( sztemp, sizeof(char), len, file);
			}
		}

        /*
        szDiskDev[0] = lgdevcfg->cDataDisk;
        szDiskDev[1] = ':';
        szDiskDev[2] = '\0';
		////getDiskSigAndInfo(szDiskDev, szDiskInfo, -1);
        ////len = _snprintf( sztemp, "%s %s%s\n", IDS_STRING_DATADISK, szDiskDev, szDiskInfo);
        len = _snprintf( sztemp, "%s %s %s %s %s\n", IDS_STRING_DATADISK, szDiskDev, lgdevcfg->szDataDisk1, lgdevcfg->szDataDisk2, lgdevcfg->szDataDisk3 );
        */

		if (bSymmetric)
		{
			if ( strlen( refPair.m_DeviceTarget.m_strPath.c_str() ) <= 3 )
			{
// SAUMYA_FIX_CONFIG_FILE_WRITING
// Get symbolic link information
#if 0
			DEVICE_INFO devinfo;
			devinfo = SymbolicLinkInfo( cDrive /*symLink1, symLink2, symLink3*/ );


			// Drive Letter
				len = _snprintf( sztemp, 512, "%s %c: %s %s %s %S %S %s\n", IDS_STRING_DATADISK, cDrive, refPair.m_DeviceTarget.m_strDriveId.c_str(), refPair.m_DeviceTarget.m_strStartOff.c_str(), refPair.m_DeviceTarget.m_strLength.c_str(), devinfo.DiskPartitionName, devinfo.DiskVolumeName, devinfo.SignatureUniqueId );
#else // SAUMYA_FIX_CONFIG_FILE_WRITING
				len = _snprintf( sztemp, 512, "%s %c: %s %s %s \n", IDS_STRING_DATADISK, cDrive, refPair.m_DeviceTarget.m_strDriveId.c_str(), refPair.m_DeviceTarget.m_strStartOff.c_str(), refPair.m_DeviceTarget.m_strLength.c_str());
#endif

				fwrite( sztemp, sizeof(char), len, file);
			}
			else
			{   // Mount Point
				len = _snprintf( sztemp, 512, "%s %s  %s %s %s\n", IDS_STRING_DATADISK, refPair.m_DeviceTarget.m_strPath.c_str(), refPair.m_DeviceTarget.m_strDriveId.c_str(), refPair.m_DeviceTarget.m_strStartOff.c_str(), refPair.m_DeviceTarget.m_strLength.c_str() );
				fwrite( sztemp, sizeof(char), len, file);
			}
		}
		else
		{
			if ( strlen( refPair.m_DeviceSource.m_strPath.c_str() ) <= 3 )
			{    // Drive Letter
// SAUMYA_FIX_CONFIG_FILE_WRITING
// Get symbolic link information
#if 0
				DEVICE_INFO devinfo;
				devinfo = SymbolicLinkInfo( cDrive /*symLink1, symLink2, symLink3*/ );

				len = _snprintf( sztemp, 512, "%s %c: %s %s %s %S %S %s\n", IDS_STRING_DATADISK, cDrive, refPair.m_DeviceSource.m_strDriveId.c_str(), refPair.m_DeviceSource.m_strStartOff.c_str(), refPair.m_DeviceSource.m_strLength.c_str(), devinfo.DiskPartitionName, devinfo.DiskVolumeName, devinfo.SignatureUniqueId );
#else // SAUMYA_FIX_CONFIG_FILE_WRITING
				len = _snprintf( sztemp, 512, "%s %c: %s %s %s \n", IDS_STRING_DATADISK, cDrive, refPair.m_DeviceSource.m_strDriveId.c_str(), refPair.m_DeviceSource.m_strStartOff.c_str(), refPair.m_DeviceSource.m_strLength.c_str());

#endif
				fwrite( sztemp, sizeof(char), len, file);
			}
			else
			{   // Mount Point
				len = _snprintf( sztemp, 512, "%s %s  %s %s %s\n", IDS_STRING_DATADISK, refPair.m_DeviceSource.m_strPath.c_str(), refPair.m_DeviceSource.m_strDriveId.c_str(), refPair.m_DeviceSource.m_strStartOff.c_str(), refPair.m_DeviceSource.m_strLength.c_str() );
				fwrite( sztemp, sizeof(char), len, file);
			}
		}

	    fwrite(IDS_STRING_SEC_SYSB, sizeof(char), strlen(IDS_STRING_SEC_SYSB), file);

        /*
        szDiskDev[0] = lgdevcfg->cMirrorDisk;
        szDiskDev[1] = ':';
        szDiskDev[2] = '\0';
        len = _snprintf( sztemp, "%s %s %s %s %s\n", IDS_STRING_MIRROR, szDiskDev, lgdevcfg->szMirrorDisk1, lgdevcfg->szMirrorDisk2, lgdevcfg->szMirrorDisk3 );
        */

		if (bSymmetric)
		{
			if ( strlen( refPair.m_DeviceSource.m_strPath.c_str() ) <= 3 )
			{   // Drive Letter
			                cDrive = refPair.m_DeviceSource.m_strPath.at(0);
				            len = _snprintf( sztemp, 512, "%s %c: %s %s %s\n", IDS_STRING_MIRROR, cDrive, refPair.m_DeviceSource.m_strDriveId.c_str(), refPair.m_DeviceSource.m_strStartOff.c_str(), refPair.m_DeviceSource.m_strLength.c_str() );
					    fwrite( sztemp, sizeof(char), len, file);
			}
			else
			{   // Mount Point
				len = _snprintf( sztemp, 512, "%s %s  %s %s %s\n", IDS_STRING_MIRROR, refPair.m_DeviceSource.m_strPath.c_str(), refPair.m_DeviceSource.m_strDriveId.c_str(), refPair.m_DeviceSource.m_strStartOff.c_str(), refPair.m_DeviceSource.m_strLength.c_str() );
				fwrite( sztemp, sizeof(char), len, file);
			}
		}
		else
		{
			if ( strlen( refPair.m_DeviceTarget.m_strPath.c_str() ) <= 3 )
			{   // Drive Letter
			                cDrive = refPair.m_DeviceTarget.m_strPath.at(0);
				            len = _snprintf( sztemp, 512, "%s %c: %s %s %s\n", IDS_STRING_MIRROR, cDrive, refPair.m_DeviceTarget.m_strDriveId.c_str(), refPair.m_DeviceTarget.m_strStartOff.c_str(), refPair.m_DeviceTarget.m_strLength.c_str() );
					    fwrite( sztemp, sizeof(char), len, file);
			}
			else
			{   // Mount Point
				len = _snprintf( sztemp, 512, "%s %s  %s %s %s\n", IDS_STRING_MIRROR, refPair.m_DeviceTarget.m_strPath.c_str(), refPair.m_DeviceTarget.m_strDriveId.c_str(), refPair.m_DeviceTarget.m_strStartOff.c_str(), refPair.m_DeviceTarget.m_strLength.c_str() );
				fwrite( sztemp, sizeof(char), len, file);
			}
		}

        it++;
	}
	//
	// End Profile Section

	// End DTC devices
    len = _snprintf( sztemp, 512, "%s%s%s%s", IDS_STRING_END_CONFIG, g_ResourceManager.GetFullProductName(),  " Configuration File: ", szLGCfgFname );
	fwrite( sztemp, sizeof(char), len, file);
	
	// Tunable params ?

    fclose(file);

    //now, read back cfg file content into a buffer
    read_lgcfg_file_into_buffer(szTempFileName, ppCfgData, piCfgDataSize);

    //delete temp file from disk
    remove(szTempFileName);
}

static 
void tdmf_create_lgcfgfile_buffer_Unix(  CReplicationGroup *replGrp, 
                                         bool bIsPrimary, char **ppCfgData, unsigned int *piCfgDataSize, bool bSymmetric )
{
	FILE		*file;
	time_t		ltime;
	char		szTime[64];
	struct tm	*gmt;
    char        szLGCfgFname[256];
    char        sztemp[512];
	char        szThrottles[7000];
    char        szTempFileName[80];
    int         len;
    CServer     *srvrSource = replGrp->m_pParent;
    CServer     *srvrTarget = replGrp->m_pServerTarget;
    std::string strSourceIP,strTargetIP;

    *piCfgDataSize  = 0;
    *ppCfgData      = 0;    

    _ASSERT(srvrSource != 0);
    _ASSERT(srvrTarget != 0);

    if (srvrSource == 0 || srvrTarget == 0)
        return;

	// Set source ip adress
	if(replGrp->m_bPrimaryDHCPAdressUsed)
	{
		strSourceIP = srvrSource->m_strName;
	}
	else if(replGrp->m_bPrimaryEditedIPUsed)
		{
			strSourceIP = replGrp->m_strPrimaryEditedIP.c_str();
		}
		else
			{
				strSourceIP = srvrSource->m_vecstrIPAddress[0];
			}

	// Set target ip adress
	if(replGrp->m_bTargetDHCPAdressUsed)
	{
		strTargetIP = srvrTarget->m_strName;
	}
	else if(replGrp->m_bTargetEditedIPUsed)
		{
			strTargetIP = replGrp->m_strTargetEditedIP.c_str();
		}
		else
		{
			strTargetIP = srvrTarget->m_vecstrIPAddress[0];
		}

	if (bSymmetric)
	{
		// Invert source/target IP
		std::string strTmp = strSourceIP;
		strSourceIP = strTargetIP;
		strTargetIP = strTmp;
	}

 	time( &ltime );
	gmt = localtime( &ltime );
    _snprintf(szTime, 64, "%s", asctime( gmt ));

    _snprintf( szLGCfgFname, 256, "%s%03d.cfg", bIsPrimary ? PRIMARY_CFG_PREFIX : SECONDARY_CFG_PREFIX	
                                      , replGrp->m_nGroupNumber   
                                      );

    GetTempFileName(".", // dir. for temp. files 
                    "temp_lg",                // temp. file name prefix 
                    0,                    // create unique name 
                    szTempFileName);          // buffer for name 

	file = fopen( szTempFileName, "w+b");

	fwrite(IDS_STRING_HEADER_BREAK, sizeof(char), strlen(IDS_STRING_HEADER_BREAK), file);

    len = _snprintf( sztemp, 512, "#  %s %s %s \n", g_ResourceManager.GetFullProductName(), IDS_STRING_OEM_FILE, szLGCfgFname );
	fwrite( sztemp, sizeof(char), len, file);
	

    len = _snprintf( sztemp, 512, "#  %s %s %s \n", g_ResourceManager.GetFullProductName(), IDS_STRING_OEM_VERSION, replGrp->m_pParent->m_strAgentVersion.c_str());

    fwrite( sztemp, sizeof(char), len, file);

    len = _snprintf( sztemp, 512, "%s %s", IDS_STRING_UPDATE, szTime );//szTime ends with \n\0 - see asctime()
	fwrite( sztemp, sizeof(char), len, file);
	
	fwrite(IDS_STRING_HEADER_BREAK, sizeof(char), strlen(IDS_STRING_HEADER_BREAK), file);

    len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_NOTE, replGrp->m_strDescription.c_str() );
	fwrite( sztemp, sizeof(char), len, file);
	
	// Begin System Values
	fwrite(IDS_STRING_PRIMARY_DEF, sizeof(char), strlen(IDS_STRING_PRIMARY_DEF), file);

    len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_HOST, strSourceIP.c_str() );
	fwrite( sztemp, sizeof(char), len, file);
    
    len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_PSTORE, bSymmetric ? replGrp->m_strSymmetricPStoreFile.c_str() : replGrp->m_strPStoreFile.c_str() );
	fwrite( sztemp, sizeof(char), len, file);

	fwrite( IDS_STRING_SECONDARY, sizeof(char), strlen(IDS_STRING_SECONDARY), file);
	
    len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_HOST, strTargetIP.c_str() );
	fwrite( sztemp, sizeof(char), len, file);
	
    len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_JOURNAL, bSymmetric ? replGrp->m_strSymmetricJournalDirectory.c_str() : replGrp->m_strJournalDirectory.c_str() );
	fwrite( sztemp, sizeof(char), len, file);
	
    len = _snprintf( sztemp, 512, "%s %d \n", IDS_STRING_SECOND_PORT, bSymmetric ? replGrp->m_pParent->m_nReplicationPort : replGrp->m_pServerTarget->m_nReplicationPort /*575*/ );
	fwrite( sztemp, sizeof(char), len, file);

	if(replGrp->m_bChaining || replGrp->m_bSymmetric)
	    fwrite( IDS_STRING_CHAIN_ON,  sizeof(char), strlen(IDS_STRING_CHAIN_ON),  file);
	else
	    fwrite( IDS_STRING_CHAIN_OFF, sizeof(char), strlen(IDS_STRING_CHAIN_OFF), file);
	// End System Values

	// Throttles
    len = _snprintf( szThrottles, 7000, "%s", replGrp->m_strThrottles.c_str() );
	fwrite( szThrottles, sizeof(char), len, file);

	// Begin DTC devices
	fwrite(IDS_STRING_DEVICE_DEF, sizeof(char), strlen(IDS_STRING_DEVICE_DEF), file);
	// Profile section. 
	//
    int              index=1,iNbrDtc,iDtcCount;
    iDtcCount = replGrp->m_listReplicationPair.size();

    //just write DTCs to file in the same order than in the list 
    std::list<CReplicationPair>::const_iterator it  = replGrp->m_listReplicationPair.begin();
    std::list<CReplicationPair>::const_iterator end = replGrp->m_listReplicationPair.end();
    iNbrDtc = 0;
	while( it != end )
    {
        std::list<CReplicationPair>::const_reference  refPair = *it;

        len = _snprintf( sztemp, 512, "%s %d \n", IDS_STRING_PROFILE, iNbrDtc+1 );
	    fwrite( sztemp, sizeof(char), len, file);

        len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_REMARK, refPair.m_strDescription.c_str() );
	    fwrite( sztemp, sizeof(char), len, file);

	    fwrite(IDS_STRING_PRIMARY_SYSA, sizeof(char), strlen(IDS_STRING_PRIMARY_SYSA), file);

        char cDrive = refPair.m_DeviceSource.m_strPath.at(0);

		if ( strstr(srvrSource->m_strOSType.c_str(),"Linux") != 0 ||
			 strstr(srvrSource->m_strOSType.c_str(),"linux") != 0 ||
			 strstr(srvrSource->m_strOSType.c_str(),"LINUX") != 0 )
		{
			len = _snprintf( sztemp, 512, "  DTC-DEVICE:       /dev/dtc/lg%d/dsk/dtc%d \n", bSymmetric ? replGrp->m_nSymmetricGroupNumber : replGrp->m_nGroupNumber, refPair.m_nPairNumber );
			fwrite( sztemp, sizeof(char), len, file);
		}
		else // For other Unix platform (HP-UX,Solaris and AIX)
		{
			len = _snprintf( sztemp, 512, "  DTC-DEVICE:       /dev/dtc/lg%d/rdsk/dtc%d \n", bSymmetric ? replGrp->m_nSymmetricGroupNumber : replGrp->m_nGroupNumber, refPair.m_nPairNumber );
			fwrite( sztemp, sizeof(char), len, file);
		}

        len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_DATADISK, bSymmetric ? refPair.m_DeviceTarget.m_strPath.c_str() : refPair.m_DeviceSource.m_strPath.c_str() );
	    fwrite( sztemp, sizeof(char), len, file);

	    fwrite(IDS_STRING_SEC_SYSB, sizeof(char), strlen(IDS_STRING_SEC_SYSB), file);

        len = _snprintf( sztemp, 512, "%s %s \n", IDS_STRING_MIRROR, bSymmetric ? refPair.m_DeviceSource.m_strPath.c_str() : refPair.m_DeviceTarget.m_strPath.c_str() );
	    fwrite( sztemp, sizeof(char), len, file);

        iNbrDtc++;
        it++;
	}
	//
	// End Profile Section

	// End DTC devices
    len = _snprintf( sztemp, 512, "%s%s%s%s \n", IDS_STRING_END_CONFIG, g_ResourceManager.GetFullProductName(), " Configuration File: ", szLGCfgFname );
	fwrite( sztemp, sizeof(char), len, file);

    fclose(file);

    //now, read back cfg file content into a buffer
    read_lgcfg_file_into_buffer(szTempFileName, ppCfgData, piCfgDataSize);

    //delete temp file from disk
    remove(szTempFileName);
}

static std::string tdmf_extract_throttle_blocks(char *pCfgData, unsigned int iCfgDataSize)
{
	std::string strThrottles;
    
	std::string strTmp = pCfgData;

	// THROTTLE: ... ENDACTIONLIST

	std::string::size_type posStart = 0;

	while ((posStart = strTmp.find("THROTTLE:", posStart)) != strTmp.npos)
	{
		// Find throttle block's end pos
		std::string::size_type posEnd = strTmp.find("ENDACTIONLIST", posStart);
		posEnd += strlen("ENDACTIONLIST") + 1; // +1 -> '\r'

		strThrottles += strTmp.substr(posStart, posEnd - posStart);

		posStart = posEnd;
	}
	
	return strThrottles;
}

void tdmf_create_lgcfgfile_buffer(  CReplicationGroup *replGrp, /*CServer *srvrSource, CServer *srvrTarget,*/ 
                                    bool bIsPrimary, char **ppCfgData, unsigned int *piCfgDataSize, bool bSymmetric )
{
    CServer     *srvrSource = replGrp->m_pParent;

    _ASSERT(srvrSource != 0);
    if (srvrSource == 0)
        return;

    if ( strstr(srvrSource->m_strOSType.c_str(),"Windows") != 0 ||
         strstr(srvrSource->m_strOSType.c_str(),"windows") != 0 ||
         strstr(srvrSource->m_strOSType.c_str(),"WINDOWS") != 0 )
    {
        tdmf_create_lgcfgfile_buffer_Windows( replGrp, bIsPrimary, ppCfgData, piCfgDataSize, bSymmetric );
    }
    else
    {
        tdmf_create_lgcfgfile_buffer_Unix( replGrp, bIsPrimary, ppCfgData, piCfgDataSize, bSymmetric );
    }
}

// ***********************************************************
// Function name	: tdmf_create_lgcfgfile_objects
// Description	    : Fill a CReplicationGroup object according to 
//                    the provided logical group configuration data.
//                    Data provided is a text buffer filled with the content
//                    of a .cfg file.
// Return type		: void 
// Argument         : char *pCfgData
// Argument         : int iCfgDataSize
// Argument         : CReplicationGroup *replGrp
// 
// ***********************************************************
void tdmf_fill_objects_from_lgcfgfile(  char *pCfgData, unsigned int iCfgDataSize, 
                                        bool bSourceSrvr, bool bIsWindows, 
										short sGrpNumber, CReplicationGroup & replGrp,
                                        std::string  &  strHostName,
                                        std::string  &  strOtherHostName
                                        )
{
    char szTempFileName[256];

    GetTempFileName(".", // dir. for temp. files 
                    "tmp",                // temp. file name prefix 
                    0,                    // create unique name 
                    szTempFileName);          // buffer for name 
    //create a .cfg file with data buffer
    FILE *fp = fopen( szTempFileName, "w+b" );
    if ( fp )
    {
        if( sizeof(char) == fwrite(pCfgData,iCfgDataSize,sizeof(char),fp) )
        {
            fclose(fp);
            //ok, now read back file using ftd_config functions
            ftd_lg_cfg_t *cfgp = ftd_config_lg_create(); 

            strcpy( cfgp->cfgpath, szTempFileName );

            //we need to extract from the buffer the logical group and role information.
            //this can be found on a line beginning with 

            int role = ( bSourceSrvr ? ROLEPRIMARY : ROLESECONDARY );

            int r = ftd_config_read_2(cfgp, bIsWindows, sGrpNumber, role);
            copy_ftd_lg_cfg_t_to_CReplicationGroup(cfgp,replGrp);

			// Extract THROTTLE blocks: n x (THROTTLE: ... ENDACTIONLIST)
			replGrp.m_strThrottles = tdmf_extract_throttle_blocks(pCfgData, iCfgDataSize);

            if ( bSourceSrvr )
            {   //this content was retreived from a pXXX.cfg file, on the Source Server
                strOtherHostName = cfgp->shostname;
                strHostName      = cfgp->phostname;
            }
            else
            {   //this content was retreived from a sXXX.cfg file, on the Target Server
                strOtherHostName = cfgp->phostname;
                strHostName      = cfgp->shostname;
            }

        	ftd_config_lg_delete(cfgp);
		}
        else
            fclose(fp);

        remove(szTempFileName);
    }
}


