/*
 * ftd_mngt.c - ftd management message handlers
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
//
// same strategy as in DTCConfigToolDlg.cpp
// Make sure that the Windows 2000 functions are used if they exist.
#if defined(_WINDOWS)
//activating the define below conflicts with libutil\misc.h
//#define _WIN32_WINNT 0x0500
#endif
#include <direct.h> //for _getcwd()

// Mike Pollett
#include "../../tdmf.inc"


extern "C" 
{
#include "sock.h"
#include "iputil.h"
#include "ftd_cfgsys.h"
#include "ftd_mngt.h"
#include "ftd_ioctl.h"
#include "conOutput.h"
}
#include "libmngtdef.h"
#include "libmngtmsg.h"
#include "errmsg_list.h"
#include "ftd_mngt_lg.h"
#include "volmntpt.h"

bool    gbEmulUnix = false;


#if defined(_WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#include "errors.h"
#define ASSERT(exp)     _ASSERT(exp)
#else
#define ASSERT(exp)     ((void)0)
#endif
#define DBGPRINT(a)     ftd_mngt_tracef a 


///////////////////////////////////////////////////////////////////////////////
/* global variables */
//TDMFAgentEmulator    gTdmfAgentEmulator = { false };
extern TDMFAgentEmulator    gTdmfAgentEmulator;
//mmp_TdmfPerfConfig   gTdmfPerfConfig = {0,0};
extern mmp_TdmfPerfConfig   gTdmfPerfConfig;

//int  giTDMFCollectorIP   = 0;    //updated each time a msg is received from Collector
//int  giTDMFCollectorPort = TDMF_COLLECTOR_DEF_PORT;  //default values
extern int  giTDMFCollectorIP   ;  // updated each time a msg is received from Collector
extern int  giTDMFCollectorPort ;  // default values
extern bool		gbTDMFCollectorPresent;
extern bool		gbTDMFCollectorPresentWarning;

static unsigned int g_uiAliveTimeout = MMP_MNGT_ALIVE_TIMEOUT;

///////////////////////////////////////////////////////////////////////////////
#define DEFAULT_TDMF_DOMAIN_NAME    "unassigned domain"

#ifndef MIN
#define MIN(a,b)    (a<b?a:b)
#endif


///////////////////////////////////////////////////////////////////////////////
/* local prototypes */
static int  ftd_mngt_write_file_to_disk(const mmp_TdmfFileTransferData *filedata, const char* pData);
static int  ftd_mngt_read_file_from_disk(const char *pFname, char **ppData, unsigned int *puiFileSize);
static void ftd_mngt_get_all_cfg_files(char **pAllCfgFileData, unsigned int *puiAllCfgFileSize);
static void ftd_mngt_get_all_tdmf_files(char **pAllCfgFileData, unsigned int *puiAllCfgFileSize);
static void ftd_mngt_acquire_alldevices( mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices );
static void ftd_mngt_gather_server_info(mmp_TdmfServerInfo *srvrInfo);
static void ftd_mngt_alive_socket(sock_t* aliveSocket,bool *pbAliveSockConnected,WSAEVENT wsaevClose,time_t * pLastAliveMsgTime);

static DWORD WINAPI ftd_mngt_StatusMsgThread(PVOID notused);
static unsigned int __stdcall receiveMngtMsgThread(void* pContext);
static bool isValidBABSize(int iBABSizeMb);

static bool consoleOutput_init   (ConsoleOutputCtrl *ctrl);
static bool consoleOutput_read   (ConsoleOutputCtrl *ctrl);
static void consoleOutput_delete (ConsoleOutputCtrl *ctrl);

static int   ftd_mngt_get_ip_list(unsigned long *pulIPList, int nMaxInList);


///////////////////////////////////////////////////////////////////////////////
/* local variables */
static HANDLE   gevEndStatusMsgThread   = NULL;
static HANDLE   ghThread                = NULL;
///////////////////////////////////////////////////////////////////////////////
/*
 * Called once, when TDMF Agent starts.
 */
extern "C" void ftd_mngt_initialize()
{
    char tmp[32];

    //init error Ftd mngt errfac_t * object

#if defined( _TLS_ERRFAC )
	if ( TLSIndex != TLS_OUT_OF_INDEXES ) {
		gMngtErrfac = (errfac_t*)TlsGetValue( TLSIndex );
	}
#else 
    if ( ERRFAC != 0 ) {
        gMngtErrfac = ERRFAC;
    }
#endif

    if ( cfg_get_software_key_value("DtcCollectorIP", tmp, CFG_IS_STRINGVAL) == CFG_OK )
    {
        ipstring_to_ip(tmp,(unsigned long*)&giTDMFCollectorIP);
    }
    else 
    {
        giTDMFCollectorIP = 0;//to be init one first msg received from TDMF Collector
                              //until then , no msg can be sent.
    }

// By Saumya 03/03/04
// With these modifications it will run in a collector less environment also
// Big GUI won't work without the collector; But the Mini GUI and the Config tool will
// still work; all the CLIs will work too
if ( giTDMFCollectorIP != 0 )
    {
    if ( cfg_get_software_key_value("DtcCollectorPort", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
    {
        giTDMFCollectorPort = atoi(tmp);
    }
    else
    {
        giTDMFCollectorPort = TDMF_COLLECTOR_DEF_PORT;//default value
    }

    if ( cfg_get_software_key_value("PerfUploadPeriod", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
    {
        gTdmfPerfConfig.iPerfUploadPeriod = atoi(tmp);
        if ( gTdmfPerfConfig.iPerfUploadPeriod <= 0 )
            gTdmfPerfConfig.iPerfUploadPeriod = 100;//10 seconds
    }
    else
    {
        gTdmfPerfConfig.iPerfUploadPeriod = 100;//default value = 10 seconds
    }
    if ( cfg_get_software_key_value("ReplGroupMonitUploadPeriod", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
    {
        gTdmfPerfConfig.iReplGrpMonitPeriod = atoi(tmp);
        if ( gTdmfPerfConfig.iReplGrpMonitPeriod <= 0 )
            gTdmfPerfConfig.iReplGrpMonitPeriod = 100;//10 seconds
    }
    else
    {
        gTdmfPerfConfig.iReplGrpMonitPeriod = 100;//default value = 10 seconds
    }

    gTdmfAgentEmulator.bEmulatorEnabled = 0;//disabled
    if ( cfg_get_software_key_value("TDMFAgentEmulator", tmp, CFG_IS_STRINGVAL) == CFG_OK )
    {
        if ( 0 == _strnicmp(tmp,"true",4) || 0 == _strnicmp(tmp,"yes",3) || 0 == _strnicmp(tmp,"1",1) )
        {
            gTdmfAgentEmulator.bEmulatorEnabled = ~0;//enabled
            if ( cfg_get_software_key_value("EmulatorRangeMin", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
            {
                gTdmfAgentEmulator.iAgentRangeMin = atoi(tmp);
            }
            else
            {
                gTdmfAgentEmulator.iAgentRangeMin = 1;
            }
            if ( cfg_get_software_key_value("EmulatorRangeMax", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
            {
                gTdmfAgentEmulator.iAgentRangeMax = atoi(tmp);
            }
            else
            {
                gTdmfAgentEmulator.iAgentRangeMax = 10;
            }
        }
    }

    if ( cfg_get_software_key_value("TraceLevel", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
    {
        error_SetTraceLevel ( (unsigned char) atoi(tmp) );
    }

    gbEmulUnix = false;
    if ( cfg_get_software_key_value("EmulUnix", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
    {
        gbEmulUnix = (atoi(tmp) != 0 );
        DBGPRINT((2,">>>>>>>> This Agent is EMULATING a UNIX agent <<<<<<<<<\n"));
    }

    //event used to end ftd_mngt_StatusMsgThread thread
    gevEndStatusMsgThread = CreateEvent(0,0,0,0);
    //launch the thread responsible for transfering Status messages (messages sent to Event Viewer)
    //to TDMF Collector
    //
    //This thread also contains the processing related to the Agent Alive Socket
    //
    DWORD tid;

    ghThread = CreateThread(0,0,ftd_mngt_StatusMsgThread,0,0,&tid);
    }

    ftd_mngt_sys_initialize();
}

void ftd_mngt_shutdown()
{

	// By Saumya 03/06/04
	// With these modifications it will run in a collector less environment also
	// Big GUI won't work without the collector; But the Mini GUI and the Config tool will
	// still work; all the CLIs will work too
	if ( giTDMFCollectorIP != 0 )
	{
    SetEvent(gevEndStatusMsgThread);
    WaitForSingleObject(ghThread, 30000);
    CloseHandle(ghThread);
                    CloseHandle(gevEndStatusMsgThread);
	}
}

/*
 * Send this Agent's basic management information message (MMP_MNGT_AGENT_INFO) to TDMF Collector.
 * TDMF Collector coordinates are based on last time a MMP_MNGT_AGENT_INFO_REQUEST was received from it.
 * return 0 on success, 
 *        +1 if TDMF Collector coordinates are not known, 
 *        -1 if cannot connect on TDMF Collector,
 *        -2 on tx error.
 */
int 
ftd_mngt_send_agentinfo_msg(int rip, int rport)
{
    int r;

    if ( rip == 0 )
    {
        rip = giTDMFCollectorIP;
    }
    if ( rport == 0 )
    {
        rport = giTDMFCollectorPort;
    }

    if ( rip != 0 && rport != 0 )
    {
        //////////////////////////////////////////////////////
        //build Agent Info response msg
        //////////////////////////////////////////////////////
        mmp_mngt_TdmfAgentConfigMsg_t msg;
		char szOriginServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];//same size as msg.data.szServerUID;

        msg.hdr.magicnumber     = MNGT_MSG_MAGICNUMBER;
        msg.hdr.mngttype        = MMP_MNGT_AGENT_INFO;
        msg.hdr.mngtstatus      = MMP_MNGT_STATUS_OK; 
        msg.hdr.sendertype      = SENDERTYPE_TDMF_AGENT;

        //
        // SZG
        // This call gets the list of ipaddresses
        ftd_mngt_gather_server_info(&msg.data);
        mmp_convert_mngt_hdr_hton(&msg.hdr);
        mmp_convert_TdmfServerInfo_hton(&msg.data);

        //
        //the following section of code is done only once in NORMAL mode.
        //loop is performed in EMULATOR mode only.
        //
        char szMachineName[MMP_MNGT_MAX_MACHINE_NAME_SZ];
        int iEmulatorIdx = gTdmfAgentEmulator.iAgentRangeMin;
        strcpy(szMachineName,msg.data.szMachineName);
		strcpy(szOriginServerUID,msg.data.szServerUID);
        do
        {
            //section begin
            //this section of code is done only once in NORMAL mode, loop only in EMULATOR mode.
            r = mmp_sendmsg(rip,rport,(char*)&msg,sizeof(msg),0,0); //ASSERT(r == 0);
            if ( r==0 )
            {
                DBGPRINT((3,"Agent info. (%d bytes) sent to Collector.\n",sizeof(msg)));
            }
            else
            {
                char ipstr[32];
                ip_to_ipstring(rip,ipstr);
                DBGPRINT((2,"****Error %d, while sending Agent info. (host id = %s) to Collector using IP=%s and port=%d.\n",r,msg.data.szServerUID,ipstr,rport));
            }
            //section end

            //emulate different Agents with different host ids
            if (EMULATOR_MODE)
            {   //prepare next message, emulating the rpesence of TDMF Agent
				ftd_mngt_emulatorGetHostID(szOriginServerUID, iEmulatorIdx, msg.data.szServerUID, sizeof(msg.data.szServerUID) );
                ftd_mngt_emulatorGetMachineName(iEmulatorIdx,szMachineName,msg.data.szMachineName,sizeof(msg.data.szMachineName));
            }

        }while( EMULATOR_MODE && iEmulatorIdx++ <= gTdmfAgentEmulator.iAgentRangeMax );
    }
    else 
    {
        r = +1;//unknown TDMF Collector
    }
    return r;
}

static 
void ftd_mngt_gather_server_info(mmp_TdmfServerInfo *srvrInfo)
{
    unsigned long   *iplist;
    char            tmp[32];
    char            tmp2[32];
    int             i;
	int				iDriverState;
	unsigned long   mac_id;

    srvrInfo->iPort = ftd_sock_get_port(NULL);//port exposed by this TDMF Agent

	mac_id  = GetHostId();
	/*
	if (mac_id == 0)
		{
	    DBGPRINT((1,"****Error, Registry is missing: DtcIP , hostid is found. \n"));
		}
	*/
    /*
    srvrInfo->ucNbrIP = getnetconfcount(); ASSERT(srvrInfo->ucNbrIP > 0);
    iplist = (unsigned long *)malloc( sizeof(unsigned long) * srvrInfo->ucNbrIP );
    getnetconfs(iplist);
    */
    iplist = (unsigned long *)malloc( sizeof(unsigned long) * 8 );
    srvrInfo->ucNbrIP = ftd_mngt_get_ip_list(iplist, 8);

    if ( srvrInfo->ucNbrIP > N_MAX_IP )
        srvrInfo->ucNbrIP = N_MAX_IP;

    //build "a.b.c.d" strings for IP address
    for ( i=0; i<srvrInfo->ucNbrIP ; i++ )
        ip_to_ipstring(iplist[i], srvrInfo->szIPAgent[i] );
    for (        ; i<N_MAX_IP ; i++ )
        srvrInfo->szIPAgent[i][0] = 0;

    //get host id 
    ftd_mngt_getServerId( srvrInfo->szServerUID );
	



    gethostname( srvrInfo->szMachineName, sizeof(srvrInfo->szMachineName) );

    /*if ( ip_to_name(iplist[0], srvrInfo->szServerUID) == -1 )
    {   //oups! unable to find machine name from its IP !  
        //set machine name to ip string ...
        strcpy(srvrInfo->szServerUID, srvrInfo->szIPAgent[0] );
        DBGPRINT((3,"****Error, unable to get this machine\'s name from IP: %s \n",srvrInfo->szIPAgent[0]));
    }*/
    free(iplist);

    //todo : get router info
    strcpy( srvrInfo->szIPRouter , NULL_IP_STR ); //clean the field

    //WR-32867 ++
    if ( cfg_get_software_key_value("IPTranslation", tmp, CFG_IS_STRINGVAL) == CFG_OK )  
        {
        if (! _stricmp(tmp,"on"))
            {
            strcpy( srvrInfo->szIPRouter , MMP_NAT_ENABLED);
            DBGPRINT((3,"INFO, IPTranslation is ON."));
            }
		else
			{
			DBGPRINT((3,"INFO, IPTranslation is OFF."));
			}
		}
    else
        {
        //force a default value, for backward compatibility
        cfg_set_software_key_value("IPTranslation", "off", CFG_IS_STRINGVAL);
		DBGPRINT((3,"INFO, IPTranslation is OFF."));
        }
    //WR-32867 --

    srvrInfo->szTDMFDomain[0] = 0;
    if ( cfg_get_software_key_sz_value("DtcDomain", srvrInfo->szTDMFDomain, sizeof(srvrInfo->szTDMFDomain)) != CFG_OK )
    {
        strcpy( srvrInfo->szTDMFDomain, DEFAULT_TDMF_DOMAIN_NAME );
    }

    srvrInfo->iBABSizeReq = 0;

    //
    // Report requested BAB size
    //
    if (        (cfg_get_driver_key_value("chunk_size", FTD_DRIVER_PARAMETERS_KEY_TYPE, tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK)
            &&  (cfg_get_driver_key_value("num_chunks", FTD_DRIVER_PARAMETERS_KEY_TYPE, tmp2, CFG_IS_NOT_STRINGVAL) == CFG_OK)  )
    {
        int chunksize = atoi(tmp);
        int num_chunks = atoi(tmp2);

        srvrInfo->iBABSizeReq = (chunksize * num_chunks) / (1024*1024);
    }
    else
    {
        DBGPRINT((1,"****Error, could not read BAB size from Agent config!!\n"));
    }

    srvrInfo->iTCPWindowSize = 0;
    if ( cfg_get_software_key_value("tcp_window_size", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
    {
        srvrInfo->iTCPWindowSize = atoi(tmp) / 1024;//bytes to KBytes
    }
    else
    {
        DBGPRINT((1,"****Error, could not read TCP Window size from Agent config!!\n"));
    }

    srvrInfo->iPort = 0;
    if ( cfg_get_software_key_value("port", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
    {
        srvrInfo->iPort = atoi(tmp);
    }
    else
    {
        DBGPRINT((1,"****Error, could not read Port from Agent config!!\n"));
    }

    srvrInfo->szTdmfPath[0] = 0;
    if ( cfg_get_software_key_sz_value("InstallPath", srvrInfo->szTdmfPath, sizeof(srvrInfo->szTdmfPath)) != CFG_OK )
    {
        DBGPRINT((1,"****Error, could not read Path from Agent config!!\n"));
    }
    srvrInfo->szTdmfPStorePath[0]  = 0;
    srvrInfo->szTdmfJournalPath[0] = 0;
    if ( srvrInfo->szTdmfPath[0] != 0 )
    {
        strcpy(srvrInfo->szTdmfPStorePath,  srvrInfo->szTdmfPath);
        strcpy(srvrInfo->szTdmfJournalPath, srvrInfo->szTdmfPath);
        if ( srvrInfo->szTdmfPath[ strlen(srvrInfo->szTdmfPath)-1 ] != '\\' )
        {
            strcat(srvrInfo->szTdmfPStorePath,  "\\");
            strcat(srvrInfo->szTdmfJournalPath, "\\");
        }
        strcat(srvrInfo->szTdmfPStorePath,  "PStore");
        strcat(srvrInfo->szTdmfJournalPath, "Journal");
    }
    if ( gbEmulUnix )
    {
        strcpy(srvrInfo->szTdmfPStorePath,  "/pstore");
        strcpy(srvrInfo->szTdmfJournalPath, "/journal");
    }

    //retrieve actual Start key value before opening the master control device  
	if ( cfg_get_driver_key_value("Start", FTD_DRIVER_KEY_TYPE, tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
	{
        iDriverState = atoi(tmp);
	}
	else
	{
		DBGPRINT((1,"****Error, could not read Start from Agent config!!\n"));
		iDriverState = 4;
	}

    //retrieve actual BAB size from the TDMF kernel device driver
	// open the master control device , if STARTED!!!
	if ( iDriverState == 4 )
	{
		srvrInfo->iBABSizeAct = 0; // Driver NOT STARTED
	}
	else
	{
		HANDLE ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0);
		if (ctlfd != INVALID_HANDLE_VALUE) 
		{
			/* get bab size from driver */
			ftd_ioctl_get_bab_size(ctlfd, &srvrInfo->iBABSizeAct);
			srvrInfo->iBABSizeAct = srvrInfo->iBABSizeAct >> 20;//convert to MB
			FTD_CLOSE_FUNC(__FILE__,__LINE__,ctlfd);

            //
            // Verify that size == requested
            // Otherwise dump an error in the log
            //
            if (srvrInfo->iBABSizeAct != srvrInfo->iBABSizeReq)
            {
                reporterr(ERRFAC, M_BABSIZE, ERRWARN, srvrInfo->iBABSizeAct, srvrInfo->iBABSizeReq);
            }


		} 
		else 
		{
		reporterr(ERRFAC, M_CTLOPEN, ERRCRIT, ftd_strerror());
		srvrInfo->iBABSizeAct = 0; // Driver NOT AVAILABLE
		}
	}

    //qdsreleasenumber is defined in version.c
    //strcpy( srvrInfo->szTdmfVersion, (char*)qdsreleasenumber );
    strcpy( srvrInfo->szTdmfVersion, "v" VERSION );

    //
    // We want to keep the build number in our version string!
    //
#if 0
    //remove "Build" str found at end of VERSION
    char *p = strstr(srvrInfo->szTdmfVersion,"Build");
    if ( p ) 
        *p = 0;//cut off "Build" 
#endif

    //get number of processor in system
    DWORD processMask, systemMask;
    if ( GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask) )
    {   //one bit set for each CPU in system
        srvrInfo->iNbrCPU = 0;
        for( DWORD bit = 0; bit < sizeof(DWORD)*8; bit++) {
            if( (1<<bit) & systemMask ) {
                srvrInfo->iNbrCPU++;
            }
        }
    }
    else {   //well, there is at least one CPU, right ?! 
        srvrInfo->iNbrCPU   = 1;
    }

    MEMORYSTATUS Mem;
    Mem.dwLength = sizeof(Mem);
    GlobalMemoryStatus(&Mem);//this function will fail for system with more than 4GB RAM.
    srvrInfo->iAvailableRAMSize = Mem.dwAvailPhys / 1024;//Bytes to KBytes
    srvrInfo->iRAMSize          = Mem.dwTotalPhys / 1024;//Bytes to KBytes

    OSVERSIONINFO osver;
    osver.dwOSVersionInfoSize = sizeof(osver);
    if ( GetVersionEx(&osver) )
    {
        strcpy( srvrInfo->szOsType, "MS Windows" );

        if ( osver.dwMajorVersion == 4 )
        {   //Windows NT 4
            strcpy( srvrInfo->szOsVersion, "NT4 " );
        }
        else if ( osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0 )
        {   //Windows 2000
            strcpy( srvrInfo->szOsVersion, "2000 ");
        }
        else if ( osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1 )
        {   //Windows XP or .NET Server
            strcpy( srvrInfo->szOsVersion, "XP ");
        }
		else if ( osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2 )			// PB
		{			
            strcpy( srvrInfo->szOsVersion, "2003 ");
		}
        else
        {
            strcpy( srvrInfo->szOsVersion, "Unknown version!! " );
        }
        //append service pack info, if any.
        strcat( srvrInfo->szOsVersion, osver.szCSDVersion );
    }
    if ( gbEmulUnix )
    {
        strcpy(srvrInfo->szOsType,    "Unix");
        strcpy(srvrInfo->szOsVersion, "(emulation)");
    }
}


/*
 * handle reception of a cfg file data
 *
 */
int
ftd_mngt_set_config(sock_t *request)
{
    int r,status;
    mmp_mngt_ConfigurationMsg_t     *pRcvCfgData;
    mmp_mngt_ConfigurationStatusMsg_t response;

    //complete reception of group cfg data message 
    mmp_mngt_recv_cfg_data( request, NULL, &pRcvCfgData);

    if ( pRcvCfgData == NULL )
    {   //problem receiving the message data 
        DBGPRINT((1,"****Error  while receiving Logical group configuration from 0x%x %s\n",request->rip, request->rhostname));
        return -1;
    }
    //convertion from network bytes order to host byte order done in mmp_mngt_recv_cfg_data()
   
    //dump file to disk.  file data is contiguous to mmp_mngt_ConfigurationMsg_t structure.
    r = ftd_mngt_write_file_to_disk(&pRcvCfgData->data,(char*)(pRcvCfgData+1));
    if ( r == 0 )
    {   //success
        status = 0;//success
    }
    else
    {
        status = 2;//err writing cfg file
        DBGPRINT((1,"***Error, while writing cfg file %s to disk !\n",pRcvCfgData->data.szFilename));
    }

    //prepare status message 
    response.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    response.hdr.mngttype       = MMP_MNGT_SET_CONFIG_STATUS;
    response.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    response.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
    mmp_convert_mngt_hdr_hton(&response.hdr);
    //skip cfg file prefix('p' or 's')
    response.usLgId = (unsigned short)strtoul( &pRcvCfgData->data.szFilename[1], NULL, 10 );
    //convert to network byte order
    response.usLgId     = htons(response.usLgId);
    response.iStatus    = htonl(status);
    strcpy( response.szServerUID, pRcvCfgData->szServerUID );
    //send status message to requester    
    r = mmp_mngt_sock_send(request, (char*)&response, sizeof(response)); ASSERT(r == sizeof(response));

    //release memory obtained from mmp_mngt_recv_cfg_data()
    mmp_mngt_free_cfg_data_mem(&pRcvCfgData);

    return r;
}

/*
 * handle reception of a TDMF file
 *
 */
int
ftd_mngt_set_file(sock_t *request)
{
    int r,status;
    mmp_mngt_FileMsg_t       *pRcvFileData;
    mmp_mngt_FileStatusMsg_t  response;

    //complete reception of group cfg data message 
    mmp_mngt_recv_file_data( request, NULL, &pRcvFileData);

    if ( pRcvFileData == NULL )
    {   //problem receiving the message data 
        DBGPRINT((1,"****Error  while receiving Logical group configuration from 0x%x %s\n",request->rip, request->rhostname));
        return -1;
    }
    //convertion from network bytes order to host byte order done in mmp_mngt_recv_script_data()
   
    //dump file to disk.  file data is contiguous to mmp_mngt_FileMsg_t structure.
    r = ftd_mngt_write_file_to_disk(&pRcvFileData->data,(char*)(pRcvFileData+1));
    if ( r == 0 )
    {   //success
        status = 0;//success
    }
    else
    {
        status = 2;//err writing TDMF file
        DBGPRINT((1,"***Error, while writing file %s to disk !\n",pRcvFileData->data.szFilename));
    }

    //prepare status message 
    response.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    response.hdr.mngttype       = MMP_MNGT_TDMF_SENDFILE_STATUS;
    response.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    response.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
    mmp_convert_mngt_hdr_hton(&response.hdr);
    // response.usLgId = (unsigned short)strtoul( &pRcvFileData->data.szFilename[1], NULL, 10 );
    //convert to network byte order
    response.usLgId     = htons(0);
    response.iStatus    = htonl(status);
    strcpy( response.szServerUID, pRcvFileData->szServerUID );
    //send status message to requester    
    r = mmp_mngt_sock_send(request, (char*)&response, sizeof(response)); ASSERT(r == sizeof(response));

    //release memory obtained from mmp_mngt_recv_cfg_data()
    mmp_mngt_free_file_data_mem(&pRcvFileData);

    return r;
}

/*
 * requested for actual configuration of one logical group cfg file
 */
int
ftd_mngt_get_config(sock_t *request)
{
    int             r,towrite;
    char            c;
    char            *pFileData;
    char            szFullPathCfgFname[256];
    unsigned int    uiFileSize ;
    mmp_mngt_ConfigurationMsg_t     *pRcvCfgMsg;
    mmp_mngt_ConfigurationMsg_t     *pResponseCfgMsg;

    //complete reception of logical group cfg data message 
    mmp_mngt_recv_cfg_data( request, NULL, &pRcvCfgMsg);

    //in a GetCONFIG request, only pRcvCfgData->data.szFileName
    if ( pRcvCfgMsg == NULL )
    {   //problem receiving the message data 
        DBGPRINT((1,"****Error  while receiving Logical group configuration from 0x%x %s\n",request->rip, request->rhostname));
        return -1;
    }
    //convertion from network bytes order to host byte order done in mmp_mngt_recv_cfg_data()
    DBGPRINT((3,"GET configuration msg received.  Cfg file requested = %s\n",
        pRcvCfgMsg->data.szFilename));

    if ((cfg_get_software_key_sz_value("InstallPath", szFullPathCfgFname, sizeof(szFullPathCfgFname))) != CFG_OK)
    {
        DBGPRINT((2,"WARNING! Unable to read \'InstallPath\' configuration value, using current dir."));
       /* Get the current working directory: */
       _getcwd( szFullPathCfgFname, sizeof(szFullPathCfgFname) );
    }
    c = szFullPathCfgFname[strlen(szFullPathCfgFname)-1];
    if ( c != '/' && c != '\\' )
    {   
        strcat(szFullPathCfgFname, "/");
    }
    strcat(szFullPathCfgFname,pRcvCfgMsg->data.szFilename);

    if ( stricmp(pRcvCfgMsg->data.szFilename,"*.cfg") == 0 )
    {   //request to recv ALL .cfg files content. 
        pFileData   = 0; 
        uiFileSize  = 0;
        //create a vector of ( mmp_TdmfFileTransferData + file data )
        ftd_mngt_get_all_cfg_files(&pFileData, &uiFileSize);
        //prepare response message 
        //make believe to mmp_mngt_build_SetConfigurationMsg() that pFileData contains one big file data,
        //instead of being a vector of ( mmp_TdmfFileTransferData + file data )
        mmp_mngt_build_SetConfigurationMsg( &pResponseCfgMsg, 
                                            pRcvCfgMsg->szServerUID, true,//it is a host id 
                                            SENDERTYPE_TDMF_AGENT,  
                                            pRcvCfgMsg->data.szFilename, 
                                            (char*)pFileData, uiFileSize );
    }
    else
    {   //request to recv ONE .cfg files content. 
        //content of file is transfered to pFileData (memory allocated within fnct)
        pFileData   = 0; 
        uiFileSize  = 0;
        r = ftd_mngt_read_file_from_disk(szFullPathCfgFname, &pFileData, &uiFileSize);
        if ( r != 0 )
        {   //err reading cfg file. maybe wrong lgid provided.
            DBGPRINT((1,"***Error, while reading cfg file %s !\n",szFullPathCfgFname));
        }

        //prepare response message 
        mmp_mngt_build_SetConfigurationMsg( &pResponseCfgMsg, 
                                            pRcvCfgMsg->szServerUID, true,//it is a host id 
                                            SENDERTYPE_TDMF_AGENT,  
                                            pRcvCfgMsg->data.szFilename, 
                                            pFileData, uiFileSize );
    }

    //send status message to requester   
    towrite = sizeof(mmp_mngt_ConfigurationMsg_t) + ntohl(pResponseCfgMsg->data.uiSize) ;
    r = mmp_mngt_sock_send(request, (char*)pResponseCfgMsg, towrite); ASSERT(r == towrite);

    //release memory obtained from mmp_mngt_build_SetConfigurationMsg()
    mmp_mngt_release_SetConfigurationMsg(&pResponseCfgMsg);
    //release memory obtained from mmp_mngt_recv_cfg_data()
    mmp_mngt_free_cfg_data_mem(&pRcvCfgMsg);

    if (pFileData)
        free(pFileData);

    return r;
}

void
ftd_mngt_get_all_cfg_files(char **pAllCfgFileData, unsigned int *puiAllCfgFileSize)
{
    HANDLE hFind;
    BOOL   bContinue;
    WIN32_FIND_DATA findFileData;
    char *pCumulDataBuf = 0;
    unsigned int uiCumulDataBufSize = 0;

    *pAllCfgFileData = 0;
    *puiAllCfgFileSize = 0;

    char szFindMask[2][16];
    strcpy(szFindMask[0],"p*.cfg");
    //strcpy(szFindMask[1],"s*.cfg");
    for ( int i=0; i<1; i++)
    {
        hFind = FindFirstFile(szFindMask[i],&findFileData);
        bContinue = (hFind != INVALID_HANDLE_VALUE);
        while( bContinue )
        {
            char *pFileData   = 0; 
            unsigned int uiFileSize  = 0;
            int r = ftd_mngt_read_file_from_disk(findFileData.cFileName, &pFileData, &uiFileSize);
            if ( r == 0 )
            {
                int iNewDataSize = sizeof(mmp_TdmfFileTransferData) + uiFileSize;
                //resize output buffer
                pCumulDataBuf = (char *)realloc(pCumulDataBuf, uiCumulDataBufSize + iNewDataSize);
                //init mmp_TdmfFileTransferData portion of new file data
                mmp_TdmfFileTransferData *p = (mmp_TdmfFileTransferData*)(pCumulDataBuf + uiCumulDataBufSize);
                p->iType  = MMP_MNGT_FILETYPE_TDMF_CFG;
                strcpy(p->szFilename, findFileData.cFileName);
                p->uiSize = uiFileSize;
                mmp_convert_hton(p);
                //append new file text data to cumulative buffer
                memmove( p + 1, pFileData, uiFileSize);

                uiCumulDataBufSize += iNewDataSize;
            }
            if ( pFileData )
                free(pFileData);
            bContinue = FindNextFile(hFind,&findFileData);
        }
        FindClose(hFind);
    }


    *pAllCfgFileData    = pCumulDataBuf;
    *puiAllCfgFileSize  = uiCumulDataBufSize;
}

/*
 * requested for actual configuration of one logical group cfg file
 */
int
ftd_mngt_get_file(sock_t *request)
{
    int             r,towrite;
    char            c;
    char            *pFileData;
    char            szFullPathFname[256];
    unsigned int    uiFileSize ;
    mmp_mngt_FileMsg_t     *pRcvFileMsg;
    mmp_mngt_FileMsg_t     *pResponseFileMsg;

    //complete reception of logical group cfg data message 
    mmp_mngt_recv_file_data( request, NULL, &pRcvFileMsg);

    //in a GET_TDMFFILE request, only pRcvFileData->data.szFileName
    if ( pRcvFileMsg == NULL )
    {   //problem receiving the message data 
        DBGPRINT((1,"****Error  while receiving FILE request from 0x%x %s\n",request->rip, request->rhostname));
        return -1;
    }
    //convertion from network bytes order to host byte order done in mmp_mngt_recv_file_data()
    DBGPRINT((3,"GET FILE msg received.  file requested = %s\n", pRcvFileMsg->data.szFilename));

    if ((cfg_get_software_key_sz_value("InstallPath", szFullPathFname, sizeof(szFullPathFname))) != CFG_OK)
    {
        DBGPRINT((2,"WARNING! Unable to read \'InstallPath\' configuration value, using current dir."));
       /* Get the current working directory: */
       _getcwd( szFullPathFname, sizeof(szFullPathFname) );
    }
    c = szFullPathFname[strlen(szFullPathFname)-1];
    if ( c != '/' && c != '\\' )
    {   
        strcat(szFullPathFname, "/");
    }
    strcat(szFullPathFname,pRcvFileMsg->data.szFilename);

    if ( strnicmp(pRcvFileMsg->data.szFilename,"*.",2) == 0 )
    {   //request to recv ALL TDMF files content. 
        pFileData   = 0; 
        uiFileSize  = 0;
        //create a vector of ( mmp_TdmfFileTransferData + file data )
        ftd_mngt_get_all_tdmf_files(&pFileData, &uiFileSize);
        //prepare response message 
        //make believe to mmp_mngt_build_SetConfigurationMsg() that pFileData contains one big file data,
        //instead of being a vector of ( mmp_TdmfFileTransferData + file data )
        mmp_mngt_build_SetTdmfFileMsg( &pResponseFileMsg, 
                                        pRcvFileMsg->szServerUID, true,//it is a host id 
                                        SENDERTYPE_TDMF_AGENT,  
                                        pRcvFileMsg->data.szFilename, 
                                        (char*)pFileData, uiFileSize ,
										pRcvFileMsg->data.iType );
    }
    else
    {   //request to recv ONE TDMF file content. 
        //content of file is transfered to pFileData (memory allocated within fnct)
        pFileData   = 0; 
        uiFileSize  = 0;
        r = ftd_mngt_read_file_from_disk(szFullPathFname, &pFileData, &uiFileSize);
        if ( r != 0 )
        {   //err reading cfg file. maybe wrong lgid provided.
            DBGPRINT((1,"***Error, while reading cfg file %s !\n",szFullPathFname));
        }

        //prepare response message 
        mmp_mngt_build_SetTdmfFileMsg( &pResponseFileMsg, 
                                        pRcvFileMsg->szServerUID, true,//it is a host id 
                                        SENDERTYPE_TDMF_AGENT,  
                                        pRcvFileMsg->data.szFilename, 
                                        pFileData, uiFileSize ,
										pRcvFileMsg->data.iType );
    }

    //send status message to requester   
    towrite = sizeof(mmp_mngt_FileMsg_t) + ntohl(pResponseFileMsg->data.uiSize) ;
    r = mmp_mngt_sock_send(request, (char*)pResponseFileMsg, towrite); ASSERT(r == towrite);

    //release memory obtained from mmp_mngt_build_SetFileMsg()
    mmp_mngt_release_SetTdmfFileMsg(&pResponseFileMsg);
    //release memory obtained from mmp_mngt_recv_file_data()
    mmp_mngt_free_file_data_mem(&pRcvFileMsg);

    if (pFileData)
        free(pFileData);

    return r;
}

void
ftd_mngt_get_all_tdmf_files(char **pAllTdmfFileData, unsigned int *puiAllTdmfFileSize)
{
    HANDLE hFind;
    BOOL   bContinue;
    WIN32_FIND_DATA findFileData;
    char *pCumulDataBuf = 0;
    unsigned int uiCumulDataBufSize = 0;

    *pAllTdmfFileData = 0;
    *puiAllTdmfFileSize = 0;

    char szFindMask[2][16];

    strcpy(szFindMask[0],"*.bat");

    for ( int i=0; i<1; i++)
    {
        hFind = FindFirstFile(szFindMask[i],&findFileData);
        bContinue = (hFind != INVALID_HANDLE_VALUE);
        while( bContinue )
        {
			if ((stricmp(findFileData.cFileName, "tdmfconfigtool.bat") != NULL) &&
				(stricmp(findFileData.cFileName, "tdmfmonitortool.bat") != NULL))
			{
            char *pFileData   = 0; 
            unsigned int uiFileSize  = 0;
            int r = ftd_mngt_read_file_from_disk(findFileData.cFileName, &pFileData, &uiFileSize);
            if ( r == 0 )
            {
                int iNewDataSize = sizeof(mmp_TdmfFileTransferData) + uiFileSize;
                //resize output buffer
                pCumulDataBuf = (char *)realloc(pCumulDataBuf, uiCumulDataBufSize + iNewDataSize);
                //init mmp_TdmfFileTransferData portion of new file data
                mmp_TdmfFileTransferData *p = (mmp_TdmfFileTransferData*)(pCumulDataBuf + uiCumulDataBufSize);
                p->iType  = MMP_MNGT_FILETYPE_TDMF_BAT;
                strcpy(p->szFilename, findFileData.cFileName);
                p->uiSize = uiFileSize;
                mmp_convert_hton(p);
                //append new file text data to cumulative buffer
                memmove( p + 1, pFileData, uiFileSize);

                uiCumulDataBufSize += iNewDataSize;
            }
            if ( pFileData )
                free(pFileData);
			}
            bContinue = FindNextFile(hFind,&findFileData);
        }
        FindClose(hFind);
    }


    *pAllTdmfFileData    = pCumulDataBuf;
    *puiAllTdmfFileSize  = uiCumulDataBufSize;
}

/*
 * 
 */
static int
ftd_mngt_write_file_to_disk(const mmp_TdmfFileTransferData *filedata,const char* pData)
{
    int r;
    FILE *file;

    if ( filedata->uiSize > 0 )
    {
        file = fopen(filedata->szFilename,"w+b");
        if (file)
        {
            if ( fwrite(pData,sizeof(char),filedata->uiSize,file) == filedata->uiSize )
            {
                r = 0;
            }
            else 
            {
                //ASSERT(0);
                r = -1;
            }
            fclose(file);
        }
        else
        {
            r = -2;
        }
    }
    else
    {   //request to delete file.  
        r = remove(filedata->szFilename);//upon success, r is set to 0.
        if ( r == -1 )
        {   if ( errno == ENOENT )
                r = 0;//file does not exist
        }
    }

    return r;
}

/*
 * Caller must free allocated memory with delete [] *ppData.
 */
static int
ftd_mngt_read_file_from_disk(const char *pFname, char **ppData, unsigned int *puiFileSize)
{
    int r;
    FILE *file;
    long filesize;

    *puiFileSize = 0;
    *ppData      = 0;

    file = fopen(pFname,"rb");
    if (file)
    {
        r = fseek( file, 0, SEEK_END );//ASSERT(r==0);
        filesize = ftell( file );

        *ppData = (char*)malloc(filesize) ;
        *puiFileSize = (unsigned int)filesize;

        r = fseek( file, 0, SEEK_SET );//ASSERT(r==0);
        if ( fread(*ppData,sizeof(char),filesize,file) == (size_t)filesize )
        {
            r = 0;
        }
        else 
        {
            //ASSERT(0);
            r = -1;
        }
        fclose(file);
    }
    else
    {
        r = -2;
    }

    return r;
}


/*
 * Received a mmp_mngt_header_t indicating a MMP_MNGT_TDMF_CMD command
 * read and process cmd
 *
 */
int
ftd_mngt_tdmf_cmd(sock_t *sockp)
{
    BOOL                            b;
    int                             r, exitcode = -1;//assume error
    char                            szShortApplicationName[_MAX_PATH];// name of executable module
    char                            szApplicationName[_MAX_PATH];// name of executable module
    char                            szCurrentDirectory[_MAX_PATH];// current directory name
    char                            *pszCurDir = szCurrentDirectory;
    DWORD                           dwCreationFlags;
    PROCESS_INFORMATION             pInfo;
    STARTUPINFO                     sInfo;
    mmp_mngt_TdmfCommandMsg_t       cmdmsg;
    mmp_mngt_TdmfCommandStatusMsg_t response;
    char                            *pWk;
    char                            *pCmdLine;
    int                              iCmdLineLen;
    DWORD                           dwCmdCompleteWaitTime;//in milliseconds
    DWORD                           dwTID;

    dwTID = GetCurrentThreadId();

    pWk  = (char*)&cmdmsg;
    pWk += sizeof(mmp_mngt_header_t);

    //////////////////////////////////
    //at this point, mmp_mngt_header_t header is read.
    //now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure 
    r = mmp_mngt_sock_recv(sockp,(char*)pWk,sizeof(mmp_mngt_TdmfCommandMsg_t)-sizeof(mmp_mngt_header_t));ASSERT(r == sizeof(mmp_mngt_TdmfCommandMsg_t)-sizeof(mmp_mngt_header_t));
    if ( r != sizeof(mmp_mngt_TdmfCommandMsg_t)-sizeof(mmp_mngt_header_t) )
    {
        return -1;
    }
    //convert multi-byte values from network bytes order to host byte order
    cmdmsg.iSubCmd = ntohl(cmdmsg.iSubCmd);

    ///////////////////////////////////////////
    //analyze mmp_mngt_TdmfCommandMsg_t content
    //
    //validate
    if ( cmdmsg.iSubCmd < FIRST_TDMF_CMD || cmdmsg.iSubCmd > LAST_TDMF_CMD )
    {
        DBGPRINT((1,"****Error, invalid commands received (0x%x) !",cmdmsg.iSubCmd));
        return -1;
    }

    /////////////////////////////////////////
    //retreive values used to execute command
    //
    if ((cfg_get_software_key_sz_value("InstallPath", szCurrentDirectory, sizeof(szCurrentDirectory) )) != CFG_OK)
    {
        DBGPRINT((2,"WARNING! Unable to read \'InstallPath\' configuration value, using current dir."));
       /* Get the current working directory: */
       _getcwd( szCurrentDirectory, sizeof(szCurrentDirectory) );
    }

    bool bNeedToCaptureConsoleOutput = true;
    dwCmdCompleteWaitTime = 0;
    //init. with directory and prefix.  
    //For Windows prefix is 'tdmf'
    sprintf(szApplicationName, "%s/", szCurrentDirectory);
    strcpy(szShortApplicationName, CMDPREFIX);
    switch( cmdmsg.iSubCmd ) {
    case MMP_MNGT_TDMF_CMD_START:
        strcat( szShortApplicationName, "start" );
        ftd_mngt_performance_reduce_stat_upload_period(10,1);//for next 10 seconds, sends stats to Collector at each second
        break;
    case MMP_MNGT_TDMF_CMD_STOP:
        strcat( szShortApplicationName, "stop" );
        ftd_mngt_performance_reduce_stat_upload_period(10,1);//for next 10 seconds, sends stats to Collector at each second
        break;
    case MMP_MNGT_TDMF_CMD_INIT:
        strcat( szShortApplicationName, "init" );
        break;
    case MMP_MNGT_TDMF_CMD_OVERRIDE:
        strcat( szShortApplicationName, "override" );
        break;
    case MMP_MNGT_TDMF_CMD_INFO:
        strcat( szShortApplicationName, "info" );       
        break;
    case MMP_MNGT_TDMF_CMD_HOSTINFO:
        strcat( szShortApplicationName, "hostinfo" );   
        break;
    case MMP_MNGT_TDMF_CMD_LICINFO:
        strcat( szShortApplicationName, "licinfo" );    
        break;
    case MMP_MNGT_TDMF_CMD_RECO:
        strcat( szShortApplicationName, "reco" );
        break;
    case MMP_MNGT_TDMF_CMD_SET:
        strcat( szShortApplicationName, "set" );
        break;
    case MMP_MNGT_TDMF_CMD_LAUNCH_PMD:
        strcat( szShortApplicationName, "launchpmd" );  
        ftd_mngt_performance_reduce_stat_upload_period(10,1);//for next 10 seconds, sends stats to Collector at each second
        //dwCmdCompleteWaitTime = 5000;
        break;
    case MMP_MNGT_TDMF_CMD_LAUNCH_REFRESH:
        strcat( szShortApplicationName, "launchrefresh");
        ftd_mngt_performance_reduce_stat_upload_period(10,1);//for next 10 seconds, sends stats to Collector at each second
        //dwCmdCompleteWaitTime = 5000;
        break;
    case MMP_MNGT_TDMF_CMD_LAUNCH_BACKFRESH:
        strcat( szShortApplicationName, "launchbackfresh");
        ftd_mngt_performance_reduce_stat_upload_period(10,1);//for next 10 seconds, sends stats to Collector at each second
        //dwCmdCompleteWaitTime = 5000;
        break;
    case MMP_MNGT_TDMF_CMD_KILL_PMD:
        strcat( szShortApplicationName, "killpmd" );
        ftd_mngt_performance_reduce_stat_upload_period(10,1);//for next 10 seconds, sends stats to Collector at each second
        dwCmdCompleteWaitTime = 5000;
        break;
    case MMP_MNGT_TDMF_CMD_KILL_RMD:
        strcat( szShortApplicationName, "killrmd" );
        ftd_mngt_performance_reduce_stat_upload_period(10,1);//for next 10 seconds, sends stats to Collector at each second
        dwCmdCompleteWaitTime = 5000;
        break;
    case MMP_MNGT_TDMF_CMD_KILL_REFRESH:
        strcat( szShortApplicationName, "killrefresh" );
        ftd_mngt_performance_reduce_stat_upload_period(10,1);//for next 10 seconds, sends stats to Collector at each second
        dwCmdCompleteWaitTime = 5000;
        break;
    case MMP_MNGT_TDMF_CMD_KILL_BACKFRESH:
        strcat( szShortApplicationName, "killbackfresh" );
        ftd_mngt_performance_reduce_stat_upload_period(10,1);//for next 10 seconds, sends stats to Collector at each second
        dwCmdCompleteWaitTime = 5000;
        break;
    case MMP_MNGT_TDMF_CMD_CHECKPOINT:
        strcat( szShortApplicationName, "checkpoint" );
        ftd_mngt_performance_reduce_stat_upload_period(30,1);//for next 30 seconds, sends stats to Collector at each second
        break;
    case MMP_MNGT_TDMF_CMD_TRACE:
        strcat( szShortApplicationName, "trace" );
        break;
    case MMP_MNGT_TDMF_CMD_OS_CMD_EXE:
        strcpy( szShortApplicationName, "cmd" );
        bNeedToCaptureConsoleOutput = false;
        dwCmdCompleteWaitTime = 0;
        pszCurDir = 0;//current directory has to be NULL with cmd.exe
        break;
    case MMP_MNGT_TDMF_CMD_TESTTDMF:
        strcat( szShortApplicationName, "test" );//only used for test !!
        break;
    case MMP_MNGT_TDMF_CMD_HANDLE:
        strcat( szShortApplicationName, "Analyzer" );
        break;
    default:
        DBGPRINT((1,"****Error, don\'t know executable file name for sub-cmd 0x%x !",cmdmsg.iSubCmd));
        ASSERT(0);
        return -2;
    }
    strcat( szShortApplicationName, ".exe" );
    if ( cmdmsg.iSubCmd == MMP_MNGT_TDMF_CMD_OS_CMD_EXE )
        szApplicationName[0] = 0;//to run cmd.exe, must provide first param NULL to CreateProcess()
    else
        strcat( szApplicationName, szShortApplicationName );

    //add app name to cmd line.
    iCmdLineLen = strlen(szShortApplicationName) + strlen(cmdmsg.szCmdOptions) + 1 + 1 + 8;//+1 for a space char, +1 for \0 at the end. + 8 for ... me!
    pCmdLine = (char*)malloc(iCmdLineLen);
    sprintf(pCmdLine,"%s ",szShortApplicationName);//short app name + one space
    if ( cmdmsg.iSubCmd == MMP_MNGT_TDMF_CMD_OS_CMD_EXE )
        strcat(pCmdLine,"/C ");//cmd.exe /C ...
    strcat(pCmdLine,cmdmsg.szCmdOptions);//append options to complete cmd line 

    //if required, init console output stream trapping
    ConsoleOutputCtrl   conctrl;
    if (bNeedToCaptureConsoleOutput)
    {
        consoleOutput_init(&conctrl);
    }

    //add a BEGIN tag to status msg list, to enclose in list any status msg output by the thread 
    //that will perform the cmd.
    if ( dwCmdCompleteWaitTime >= 0 )
    {
        error_msg_list_addBeginTag(dwTID);
    }

    //launch process command 
    memset(&sInfo,0,sizeof(sInfo));
    sInfo.cb = sizeof(sInfo);
    dwCreationFlags = CREATE_NO_WINDOW; // Windows NT/2000/XP: This flag is valid only when starting a console application. If set, the console application is run without a console window 
    if (bNeedToCaptureConsoleOutput)
    {
        sInfo.dwFlags    = STARTF_USESTDHANDLES;
        sInfo.hStdError  = conctrl.hErrorWrite;//GetStdHandle(STD_ERROR_HANDLE);
        sInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
        sInfo.hStdOutput = conctrl.hOutputWrite;//GetStdHandle(STD_OUTPUT_HANDLE);
        dwCreationFlags  = CREATE_NO_WINDOW;
    }
    DBGPRINT((3,"Launching <%s> , options = <%s>...\n",szApplicationName,pCmdLine));
    b =   CreateProcess(szApplicationName[0] == 0 ? NULL : szApplicationName,    // name of executable module
                        pCmdLine,             // command line string
                        NULL,                 // SD
                        NULL,                 // SD
                        TRUE,                 // handle inheritance option
                        dwCreationFlags,      // creation flags
                        NULL,                 // new environment block
                        pszCurDir,            // current directory name
                        &sInfo,               // startup information
                        &pInfo                // process information
                        );//ASSERT(b);
    if ( b )
    {
        DBGPRINT((3,"  Waiting for cmd process to end ...\n"));

        if (bNeedToCaptureConsoleOutput)
        {   //accumulate all console output into pData buffer
            consoleOutput_read(&conctrl);
            //printf("\nConsole output:>>>>\n%s",conctrl.pData);

            //console messages are sent back to TDMF Collector as Status Messages using 
            //StatusMsgThread (below)
            char *pTimeAndConsoleMsg = new char [ conctrl.iTotalDataRead + 512 ];
            error_format_datetime(ERRFAC, "INFO", "CMD", conctrl.pData, pTimeAndConsoleMsg);
            error_msg_list_addMessageThread(LOG_INFO, pTimeAndConsoleMsg, dwTID);
            delete [] pTimeAndConsoleMsg ;

        }

        //wait until tdmf....exe process has completed and exits
        if ( WAIT_OBJECT_0 == WaitForSingleObject(pInfo.hProcess,INFINITE) )
        {
            exitcode = 0;
            Sleep(dwCmdCompleteWaitTime);//sit and wait...
        }

        //filter some return codes:
        //cmd.exe returns 1 if directory already exists.
        if ( cmdmsg.iSubCmd == MMP_MNGT_TDMF_CMD_OS_CMD_EXE && exitcode == 1)
            exitcode = 0;//ok
    }
    else
    {
        DWORD err=GetLastError();
        DBGPRINT((2,"****Error (%d) while launching app (see previous message)!\n",err));
    }

    //add a END tag to status msg list, to enclose any status msg output by the thread that will perform the cmd.
    if ( dwCmdCompleteWaitTime >= 0 )
    {
        error_msg_list_addEndTag(dwTID);
        if ( error_msg_list_getAllThreadMessages(dwTID) )
        {   //at least one error msg detected in cmd status messages 
            exitcode = 1;//error
            DBGPRINT((2,"CommandMsgStatus : ERROR reported in one of the Status msg\n"));
        }
        //this will wake up the ftd_mngt_StatusMsgThread() thread.
        error_msg_list_markAllThreadMessagesAsProcessed(dwTID);
    }

    CloseHandle(pInfo.hProcess);
    CloseHandle(pInfo.hThread);

    free(pCmdLine);


    ////////////////////////////////
    //send exit code back to caller
    //
    response.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    response.hdr.mngttype       = MMP_MNGT_TDMF_CMD_STATUS;
    response.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    response.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
    //convert to network byte order before sending on socket
    mmp_convert_mngt_hdr_hton(&response.hdr);
    //for all TDMF tools, exit code : 0 = ok , else error.
    response.iStatus            = exitcode == 0 ? MMP_MNGT_TDMF_CMD_STATUS_OK : MMP_MNGT_TDMF_CMD_STATUS_ERROR;
    response.iSubCmd            = cmdmsg.iSubCmd;    
    strcpy(response.szServerUID,cmdmsg.szServerUID);
    if (bNeedToCaptureConsoleOutput)
        response.iLength        = conctrl.iTotalDataRead;//0 or length of console output
    else 
        response.iLength        = 0;
    int iOutputMsgLen           = response.iLength;
    //convert to network byte order before sending on socket
    response.iStatus            = htonl(response.iStatus);
    response.iSubCmd            = htonl(response.iSubCmd);
    response.iLength            = htonl(response.iLength);

    //respond using same socket
    int towrite = sizeof(mmp_mngt_TdmfCommandStatusMsg_t) ;
    r = mmp_mngt_sock_send(sockp,(char*)&response,towrite);ASSERT(r == towrite);
    if ( r != towrite )
    {
        DBGPRINT((1,"****Error (%d) while sending CommandMsgStatus to requester!\n",r));
    }
    else
    {
        DBGPRINT((3,"CommandMsgStatus response : sub-cmd=0x%x , status=%s\n",ntohl(response.iSubCmd), exitcode == 0 ? "MMP_MNGT_TDMF_CMD_STATUS_OK" : "MMP_MNGT_TDMF_CMD_STATUS_ERROR"));
        if ( iOutputMsgLen > 0 )
        {
            DBGPRINT((3,"      outputmsg=%s\n",conctrl.pData));
            towrite = iOutputMsgLen;
            r = mmp_mngt_sock_send(sockp,(char*)conctrl.pData,towrite);ASSERT(r == towrite);
        }
    }

    if (bNeedToCaptureConsoleOutput)
    {
        consoleOutput_delete(&conctrl);
    }
    
    return 0;
}


/*
 * Manages MMP_MNGT_SET_AGENT_GEN_CONFIG or MMP_MNGT_GET_AGENT_GEN_CONFIG requests.
 *
 */
int ftd_mngt_agent_general_config(sock_t *sockp, int iMngtType)
{
    mmp_mngt_TdmfAgentConfigMsg_t   cmdmsg;
    int                             r,toread;
    char                            *pWk;
    bool                            requestRestart = false;

    pWk  = (char*)&cmdmsg;
    pWk += sizeof(mmp_mngt_header_t);
    //////////////////////////////////
    //at this point, mmp_mngt_header_t header is read.
    //now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure 
    toread = sizeof(mmp_mngt_TdmfAgentConfigMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(sockp,(char*)pWk, toread);ASSERT(r == toread);
    if ( r != toread )
    {
        DBGPRINT((1,"****Error, bad format General config message received from 0x%x \n",sockp->rip));
        return -1;
    }
    //convert from network bytes order to host byte order
    mmp_convert_TdmfServerInfo_ntoh(&cmdmsg.data);
    DBGPRINT((3,"%s Agent general config received\n", iMngtType == MMP_MNGT_GET_AGENT_GEN_CONFIG ? "GET" :"SET" ));
    if ( iMngtType == MMP_MNGT_SET_AGENT_GEN_CONFIG )
    {
        char tmp[32];
        char tmp2[32];

        DBGPRINT((3,"   Received new config : \n"
                         "      BAB size        = %d MB\n"
                         "      TCP Window size = %d KB\n"
                         "      TDMF Port       = %d \n"
                         "      and more ...\n"
                         ,  cmdmsg.data.iBABSizeReq
                         ,  cmdmsg.data.iTCPWindowSize
                         ,  cmdmsg.data.iPort
                         ));
		
        //validate entries before saving.
        //BAB size cannot be more than 60% of total RAM size
        if ( !isValidBABSize(cmdmsg.data.iBABSizeReq) )
        {
            cmdmsg.hdr.mngtstatus = MMP_MNGT_STATUS_ERR_INVALID_SET_AGENT_GEN_CONFIG;
        }
        else
        {   //must check if critital values are changed. if so, a system restart will be requested.
            //
            int     NumChunks   = 0;
            int     ChunkSize   = 0;
            int     curValue    = 0;

            requestRestart = false;
            
            if (        ( cfg_get_driver_key_value("num_chunks", FTD_DRIVER_PARAMETERS_KEY_TYPE, tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
                    &&  ( cfg_get_driver_key_value("chunk_size", FTD_DRIVER_PARAMETERS_KEY_TYPE, tmp2, CFG_IS_NOT_STRINGVAL) == CFG_OK ) )
            {
                NumChunks = atoi(tmp);
                ChunkSize = atoi(tmp2);
                curValue = (NumChunks*ChunkSize) / (1024*1024);

                requestRestart = ( requestRestart || ( curValue != cmdmsg.data.iBABSizeReq ) );
            }

            if ( cfg_get_software_key_value("tcp_window_size", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
            {
                curValue=atoi(tmp);
                curValue /= 1024;//Bytes to KBytes
                //requestRestart = ( requestRestart || ( curValue != cmdmsg.data.iTCPWindowSize ) ); /* PB: to prevent reboot for TCPWindow */
            }
            if ( cfg_get_software_key_value("port", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
            {
                curValue=atoi(tmp);
                requestRestart = ( requestRestart || ( curValue != cmdmsg.data.iPort ) );
            }

            /////////////////////////////////////////
            //reuse cmdmsg for response to requester
            cmdmsg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;//assuming success

            /////////////////////////////////////////

            if (requestRestart && ChunkSize)
            {
                NumChunks = (cmdmsg.data.iBABSizeReq * (1024*1024)) / ChunkSize;
                itoa(NumChunks,tmp,10);

            if ( cfg_set_driver_key_value("num_chunks", FTD_DRIVER_PARAMETERS_KEY_TYPE, tmp, CFG_IS_NOT_STRINGVAL) != CFG_OK )
            {
                DBGPRINT((1,"****Error, could not save BAB size to Agent config!!\n"));
                cmdmsg.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_SET_AGENT_GEN_CONFIG;
            }
            }
			// If requested BAB Size is 0, the TDMFBlock driver is not loaded at boot-up
			if ( cmdmsg.data.iBABSizeReq == 0 )
				itoa(4,tmp,10);  // SERVICE DISABLED (4)
			else
				itoa(0,tmp,10);  // SERVICE BOOT START (0)

			if ( cfg_set_driver_key_value("Start", FTD_DRIVER_KEY_TYPE, tmp, CFG_IS_NOT_STRINGVAL) != CFG_OK )
			{
				DBGPRINT((1,"****Error, could not save Start KEY to Agent config!!\n"));
				cmdmsg.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_SET_AGENT_GEN_CONFIG;
			}

            cmdmsg.data.iTCPWindowSize *= 1024;//KBytes to Bytes
            itoa(cmdmsg.data.iTCPWindowSize,tmp,10);
            if ( cfg_set_software_key_value("tcp_window_size", tmp, CFG_IS_NOT_STRINGVAL) != CFG_OK )
            {
                DBGPRINT((1,"****Error, could not save TCP Window size to Agent config!!\n"));
                cmdmsg.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_SET_AGENT_GEN_CONFIG;
            }
            itoa(cmdmsg.data.iPort,tmp,10);
            if ( cfg_set_software_key_value("port", tmp, CFG_IS_NOT_STRINGVAL) != CFG_OK )
            {
                DBGPRINT((1,"****Error, could not save Port to Agent config!!\n"));
                cmdmsg.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_SET_AGENT_GEN_CONFIG;
            }
            if ( cfg_set_software_key_value("DtcDomain", cmdmsg.data.szTDMFDomain, CFG_IS_STRINGVAL) != CFG_OK )
            {
                DBGPRINT((1,"****Error, could not save domain to Agent config!!\n"));
                cmdmsg.hdr.mngtstatus  = MMP_MNGT_STATUS_ERR_SET_AGENT_GEN_CONFIG;
            }

            //todo : Router IP ?
        }
    }
    else
    {   //get config
        //reuse cmdmsg for response to requester
        cmdmsg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK;//assuming success

        ftd_mngt_gather_server_info(&cmdmsg.data);

        DBGPRINT((3,"   Reported Agent General config :\n"
                         "      requested BAB size = %d MB\n"
                         "      actual BAB size    = %d MB\n"
                         "      TCP Window size    = %d KB\n"
                         "      TDMF Port          = %d \n"
                         "      and more ...\n"
                         ,  cmdmsg.data.iBABSizeReq
                         ,  cmdmsg.data.iBABSizeAct
                         ,  cmdmsg.data.iTCPWindowSize
                         ,  cmdmsg.data.iPort
                         ));
        
    }

    //complete response msg and send it to requester
    cmdmsg.hdr.magicnumber  = MNGT_MSG_MAGICNUMBER;
    cmdmsg.hdr.mngttype     = iMngtType;
    cmdmsg.hdr.sendertype   = SENDERTYPE_TDMF_AGENT;
    mmp_convert_mngt_hdr_hton(&cmdmsg.hdr);
    mmp_convert_TdmfServerInfo_hton(&cmdmsg.data);
    //respond using same socket
    r = mmp_mngt_sock_send(sockp,(char*)&cmdmsg,sizeof(cmdmsg));ASSERT(r == sizeof(cmdmsg));
    if ( r != sizeof(cmdmsg) )
    {
        DBGPRINT((1,"****Error (%d) while sending AgentConfigMsg to requester!\n",r));
    }

    if ( requestRestart )   
        ftd_mngt_sys_request_system_restart();

    return 0;

}

static bool
isValidBABSize(int iBABSizeMb)
{
   int          maxPhysMemKb;

#if !defined MEMORYSTATUSEX

   // Windows version is before Windows 2000
   MEMORYSTATUS stat;
   GlobalMemoryStatus (&stat);
   maxPhysMemKb = (int)(((stat.dwTotalPhys / 1024) * 6) / 10);

#else // #if !defined MEMORYSTATUSEX

   // If the OS version is Windows 2000 or greater,
   // GlobalMemoryStatusEx must be used instead of
   // GlobalMemoryStatus in case the memory size
   // exceeds 4 GBs. 
   MEMORYSTATUSEX statex;
   statex.dwLength = sizeof(statex);
   GlobalMemoryStatusEx (&statex);
   maxPhysMemKb = (int)(((statex.ullTotalPhys / 1024) * 6) / 10);

#endif // #if !defined MEMORYSTATUSEX

   // maxPhysMemKb is equal to the total physical memory
   // divided by 2. So if the Bab size in KB is lower than
   // or equal to 60% of the total physical memory,
   // return TRUE.
   if (iBABSizeMb*1024 <= maxPhysMemKb) 
       return true;
   return false;
}

/*
 * Manages MMP_MNGT_GET_ALL_DEVICES request
 */
int 
ftd_mngt_get_all_devices(sock_t *sockp)
{
    mmp_mngt_TdmfAgentDevicesMsg_t  msg;
    mmp_TdmfDeviceInfo              *pDevicesVector;
    int                             iNbrDevices;
    int                             r,toread,towrite;
    char                            *pWk;

    pWk  = (char*)&msg;
    pWk += sizeof(mmp_mngt_header_t);
    //////////////////////////////////
    //at this point, mmp_mngt_header_t header is read.
    //now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure 
    //don't care about it, just empty socket to be able to response
    toread = sizeof(mmp_mngt_TdmfAgentDevicesMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(sockp,(char*)pWk, toread);ASSERT(r == toread);
    if ( r != toread )
    {
        DBGPRINT((1,"****Error, bad format Get ALL Devices message received from 0x%x \n",sockp->rip));
        return -1;
    }
    //no data to convert from network byte order to host byte order

    //

    ftd_mngt_acquire_alldevices(&pDevicesVector,&iNbrDevices);
    //getDiskSigAndInfo(szDiskDev, szDiskInfo, -1);

    ///////////////////////////
    //build response message 
    ///////////////////////////
    msg.hdr.magicnumber   = MNGT_MSG_MAGICNUMBER;
    msg.hdr.mngttype      = MMP_MNGT_SET_ALL_DEVICES;
    msg.hdr.sendertype    = SENDERTYPE_TDMF_AGENT;
    msg.hdr.mngtstatus    = MMP_MNGT_STATUS_OK; 
    //convert to network byte order before sending on socket
    mmp_convert_mngt_hdr_hton(&msg.hdr);

    msg.szServerUID[0]      = 0;//don't care
    msg.iNbrDevices       = htonl(iNbrDevices);

    //convert to network byte order before sending on socket
    for (r=0; r<iNbrDevices; r++)
        mmp_convert_TdmfAgentDeviceInfo_hton( pDevicesVector + r );

    //////////////////////////////
    //respond using same socket.  first send mmp_mngt_TdmfAgentDevicesMsg_t than send pDevicesVector
    //
    towrite = sizeof(mmp_mngt_TdmfAgentDevicesMsg_t);
    r = mmp_mngt_sock_send(sockp, (char*)&msg, towrite);ASSERT(r == towrite);
    if ( r == towrite )
    {
        towrite = iNbrDevices*sizeof(mmp_TdmfDeviceInfo);
        r = mmp_mngt_sock_send(sockp,(char*)pDevicesVector,towrite);ASSERT(r == towrite);
        if ( r == towrite )
        {   //success
        }
        else
        {
            DBGPRINT((1,"****Error (%d) while sending AgentDevicesMsg to requester!\n",r));
        }
    }
    else
    {
        DBGPRINT((1,"****Error (%d) while sending AgentDevicesMsg to requester!\n",r));
    }

    //cleanup
    if (pDevicesVector != 0)
        free(pDevicesVector);

    return 0;
}

// By Saumya Tripathi 09/10/03
// SAUMYA_FIX_DEVICE_UNIQUE_ID
#if 0
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
			//GetErrorText();
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
		//GetErrorText();
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
#endif // SAUMYA_FIX_DEVICE_UNIQUE_ID

static void
ftd_mngt_acquire_alldevices( mmp_TdmfDeviceInfo **ppDevicesVector, int *piNbrDevices )
{
#define STRUCT_INCR     10
#define VECTOR_SZ_INCR  STRUCT_INCR*sizeof(mmp_TdmfDeviceInfo)

    unsigned int        iVectorSzBytes;
    mmp_TdmfDeviceInfo  *pWk;
    
#if defined(_WINDOWS)
	char	szDriveString[_MAX_PATH];
	char	szDrive[4];
	char	szDeviceList[3 * 1024];
	int		i = 0, iDrive, j=0;
	char	szDiskInfo[256];
	char	strWinDir[_MAX_PATH];
	int		iGroupID = -1;

	// Basic/Dynamic disk check
	unsigned int uiPartitionType = 0xFFFFFFFF;

	// Volume Mount Point Check
	int  iDoMntPntCheck = 0;
	int  iMntPntCount   = 0;
	char szVolumeGuid[BUFSIZE];
	char szVolumePath[BUFSIZE];
	char szVolumeMntPt[BUFSIZE];

	char szDosDevice[BUFSIZE];
	char PtBuf[BUFSIZE];
    char bFlags = 0;
    HANDLE hPt = INVALID_HANDLE_VALUE; // handle for mount point scan
    HANDLE  hMutex;

	// By Saumya Tripathi 09/10/04
	// SAUMYA_FIX_DEVICE_UNIQUE_ID
#if 0
	DWORD						status;
	ULONG						x, size, totalDisks;
	DWORD						retLength	= 0;
	ATTACHED_DISK_INFO_LIST		attachDiskInfoList;
	PATTACHED_DISK_INFO_LIST	pAttachDiskInfoList = NULL;
	PATTACHED_DISK_INFO			pAttachDiskInfo		= NULL;

	// Get Total Attached Disk Count by sending IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									FTD_CTLDEV,
									FTD_GET_All_ATTACHED_DISKINFO,
									&attachDiskInfoList,			// LPVOID lpInBuffer,
									sizeof(attachDiskInfoList),		// DWORD nInBufferSize,
									&attachDiskInfoList,			// LPVOID lpOutBuffer,
									sizeof(attachDiskInfoList),		// DWORD nOutBufferSize,
									&retLength);					// LPDWORD lpBytesReturned,
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_All_ATTACHED_DISKINFO To Get DiskCounts", FTD_CTLDEV);
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
	pAttachDiskInfoList = (PATTACHED_DISK_INFO_LIST)calloc(1, size);
	if (pAttachDiskInfoList == NULL)
	{
		printf("Error: calloc(size %d) failed. GetLastError() %d \n", size, GetLastError());
		//GetErrorText();
		goto ret;
	}
	// Initialize it 
	RtlZeroMemory( pAttachDiskInfoList, size );

	pAttachDiskInfoList->NumOfDisks = totalDisks;

	// Get All Attached Disk Info List by sending IOCTL to Driver
	status = sftk_DeviceIoControl(	NULL, 
									FTD_CTLDEV,
									FTD_GET_All_ATTACHED_DISKINFO,
									pAttachDiskInfoList,			// LPVOID lpInBuffer,
									size,							// DWORD nInBufferSize,
									pAttachDiskInfoList,			// LPVOID lpOutBuffer,
									size,							// DWORD nOutBufferSize,
									&retLength);					// LPDWORD lpBytesReturned,
	if (status != NO_ERROR) 
	{	// Failed.
		printf("Error: DeviceIoControl(Ioctl- %s) to %s Device failed. \n",
							"FTD_GET_All_ATTACHED_DISKINFO To Get AllDiskInfoList", FTD_CTLDEV);
		goto ret;
	}      
	
	pAttachDiskInfo = &pAttachDiskInfoList->DiskInfo[0];
	for(i=0; i < pAttachDiskInfoList->NumOfDisks; i++)
	{
		printf("\n DiskDevice Entry: %d \n", i);
		pAttachDiskInfo = (PATTACHED_DISK_INFO) ((ULONG) pAttachDiskInfo + sizeof(ATTACHED_DISK_INFO));
	}

ret: 
	if (pAttachDiskInfoList)
		free(pAttachDiskInfoList);
#endif // SAUMYA_FIX_DEVICE_UNIQUE_ID

    //problems were encounters when calling ftd_mngt_acquire_alldevices() 
    //by multiple threads (LOOPBACK mode), simultaneously.  
	// This is the reason for this Mutex.

    hMutex = CreateMutex(0,0,"ftd_mngt_acquire_alldevices");
    if ( hMutex != 0 )
    {
        WaitForSingleObject(hMutex,INFINITE);
    }
    else
    {
        ASSERT(0);
    }

    iVectorSzBytes      = 0;
    *piNbrDevices       = 0;
    *ppDevicesVector    = 0;

	//szGroup = strrchr(msg, ' ');
	//iGroupID = atoi(szGroup);

	GetWindowsDirectory(strWinDir, _MAX_PATH);

	memset(szDriveString, 0, sizeof(szDriveString));
	memset(szDeviceList, 0, sizeof(szDeviceList));
		
	GetLogicalDriveStrings(sizeof(szDriveString), szDriveString);
	
	while(szDriveString[i] != 0 || szDriveString[i+1] != 0)
	{
		//sprintf(szDrive, "%c%c%c", szDriveString[i], szDriveString[i+1], szDriveString[i+2]);
        szDrive[0] = szDriveString[i];    //Drive Letter is here !
        szDrive[1] = szDriveString[i+1];
        szDrive[2] = szDriveString[i+2];
        szDrive[3] = 0;
		i = i + 4;

		memset(szDosDevice, 0, BUFSIZE);
		strcpy( szDosDevice , "\\DosDevices\\");
		strncat(szDosDevice , &szDrive[0], 1);
		strcat( szDosDevice , ":");

		iDrive = GetDriveType(szDrive);

		if(		iDrive != DRIVE_REMOVABLE 
			&&	iDrive != DRIVE_CDROM 
			&&	iDrive != DRIVE_RAMDISK 
			&&	iDrive != DRIVE_REMOTE  )
		{
			iDoMntPntCheck = 1;
			iMntPntCount   = 0;

			while( iDoMntPntCheck ) 
			{
				memset(szDiskInfo, 0, sizeof(szDiskInfo));
				// if SYSTEM DRIVE , skip it
 				if ( toupper(szDrive[0]) == toupper(strWinDir[0]) )
					iMntPntCount++;      //Activate Volume Mount Point verification

				// First Pass, retreive DiskInfo for current DosDevice 
				if ( !iMntPntCount )  
				{  
					getDiskSigAndInfo(szDrive, szDiskInfo, iGroupID);
				}
				else // Scan all the Volume Mount Points of current DosDevice
				{
#if !defined(NTFOUR) // Not supported in NT4 OS					
					if ( iMntPntCount == 1) // First Pass, get Volume GUID for ROOT DIRECTORY and HANDLE
					{
						if( QueryVolMntPtInfoFromDevName( szDosDevice, BUFSIZE, szVolumeGuid, BUFSIZE, VOLUME_GUID) != VALID_MNT_PT_INFO )
							break;	

						szVolumeGuid[1] = '\\';  // Convert GUID for \\?\Volume{---}\ format 
						szVolumeGuid[48] = '\\';
						
						hPt = getFirstVolMntPtHandle( szVolumeGuid, // root path of volume to be scanned
													  PtBuf,        // pointer to output string
													  BUFSIZE
												    );

						if (hPt == INVALID_HANDLE_VALUE) 
						{
							closeVolMntPtHandle(hPt);
							break;
						}
					}
					else //	Scan remaining Volume Mount Points of current DosDevice
					{
						if(!getNextVolMntPt( hPt,    // handle to scan
											 PtBuf,  // pointer to output string
											 BUFSIZE )) // size of output buffer
						{
							closeVolMntPtHandle(hPt);
							break;
						}
					}

					memset( szVolumeMntPt, 0, BUFSIZE);
					strcpy( szVolumeMntPt, szVolumeGuid );
					strcat( szVolumeMntPt, PtBuf );

					if (!getVolumeNameForVolMntPt( szVolumeMntPt,    // input volume mount point or directory
												   szVolumePath,     // output volume name buffer
												   BUFSIZE ))        // size of volume name buffer
					{ 
						closeVolMntPtHandle(hPt);
						break;							
					}

					szVolumePath[1] = '\\';  // Convert GUID for \\?\Volume{---} format 
					szVolumePath[48] = 0;
                            
                    getMntPtSigAndInfo(szVolumeMntPt, szDiskInfo, 0);
#else
					//
					// This is the root drive, skip it!
					//
					break;
#endif
				}

				//if we get no disk info back set the string to 1 blank for parse
				if(strlen(szDiskInfo) == 0)
				{   //flag error
					strcpy( szDiskInfo, "0 0 0" );//szDiskInfo[0] = ' ';
				}

                //if output vector full, stretch it.
				if ( (*piNbrDevices)*sizeof(mmp_TdmfDeviceInfo) == iVectorSzBytes ) 
				{
					*ppDevicesVector = (mmp_TdmfDeviceInfo *)realloc(*ppDevicesVector, iVectorSzBytes += VECTOR_SZ_INCR );
				}
				//add to vector
				pWk = (*ppDevicesVector) + (*piNbrDevices);

				if ( !iMntPntCount )
					strcpy( pWk->szDrivePath, szDrive );
				else
					sprintf (pWk->szDrivePath,"%c:\\%s", szDrive[0], PtBuf);

                //
                // Steve add basic/dynamic detection
                //
#if !defined(NTFOUR) // Not supported in NT4 OS					
				uiPartitionType = IsDiskBasic(pWk->szDrivePath);
#else
				uiPartitionType = 1; // NT is ALWAYS BASIC partition type
#endif

				sscanf(szDiskInfo,"%s %s %s", pWk->szDriveId, pWk->szStartOffset, pWk->szLength );

				//validation, continue to scan Drives and Mount Points even if szLength == 0, WR17128
				//Error level from 1 to 3 , WR17308
				if ( _atoi64(pWk->szLength) == 0 )
				{
					DBGPRINT((3,"**** ERROR retreiving parameters for logical drive %s !\n",szDrive));
				} 

				//get file system info
				char    szDontcare[MAX_PATH], szFileSystem[MAX_PATH];
				DWORD   dwDontcare;
				pWk->sFileSystem = MMP_MNGT_FS_UNKNOWN;
				if ( GetVolumeInformation( pWk->szDrivePath,                // root directory
					                       szDontcare,                      // volume name buffer
						                   sizeof(szDontcare),              // length of name buffer
							               &dwDontcare,                     // volume serial number
								           &dwDontcare,                     // maximum file name length
									       &dwDontcare,                     // file system options
										   szFileSystem,                    // file system name buffer
										   sizeof(szFileSystem)             // length of file system name buffer
										   ) )
				{
					if ( strstr(szFileSystem,"fat") || strstr(szFileSystem,"FAT") )
					{
						if (uiPartitionType == 0)
						{
							pWk->sFileSystem = MMP_MNGT_FS_FAT_DYN;
						}
						else
						{
							pWk->sFileSystem = MMP_MNGT_FS_FAT;
						}

						if ( strstr(szFileSystem,"16") )
						{
							if (uiPartitionType == 0)
							{
								pWk->sFileSystem = MMP_MNGT_FS_FAT16_DYN;
							}
							else
							{
								pWk->sFileSystem = MMP_MNGT_FS_FAT16;
							}
						}
						else if ( strstr(szFileSystem,"32") )
						{
							if (uiPartitionType == 0)
							{
								pWk->sFileSystem = MMP_MNGT_FS_FAT32_DYN;
							}
							else
							{
								pWk->sFileSystem = MMP_MNGT_FS_FAT32;
							}
						}
					}
					else if ( strstr(szFileSystem,"ntfs") || strstr(szFileSystem,"NTFS") )
					{
						if (uiPartitionType == 0)
						{
							pWk->sFileSystem = MMP_MNGT_FS_NTFS_DYN;
						}
						else
						{
							pWk->sFileSystem = MMP_MNGT_FS_NTFS;
						}
					}
				}
#if !defined(NTFOUR)
				iMntPntCount++;      //Activate Volume Mount Point verification
#else
				iDoMntPntCheck = 0;  //Mount Point not supported in NT4 OS!!
#endif
				(*piNbrDevices) ++;
			}	//while( iDoMntPntCheck )
		} //if( iDrive== ... )
	} //while(szDriveString[i]..
#else
#error ftd_mngt_acquire_alldevices() not implemented for this platform
#endif
}

//
// Called by TDMF Agent when an Alert event occurs.
// This function sends a msg to TDMF Collector
//
void    ftd_mngt_send_alert_msg(mmp_TdmfAlertHdr *alertData, char* szModuleName, char* szAlertMessage )
{
    mmp_mngt_TdmfAlertMsg_t *pMsg;
    char                    *pData,*pWk;
    int                     iMsgSize,towrite,r;
    int                     iModuleNameSize = 0;
    int                     iAlertMessageSize = 0;

    iMsgSize = sizeof(mmp_mngt_TdmfAlertMsg_t);
    if ( szModuleName != 0 )
    {
        iModuleNameSize = strlen( szModuleName );
        iMsgSize += iModuleNameSize;
    }
    iMsgSize ++;//reserve space for '\0' appended to szModuleName
    if ( szAlertMessage != 0 )
    {
        iAlertMessageSize = strlen( szAlertMessage );
        iMsgSize += iAlertMessageSize;
    }
    iMsgSize ++;//reserve space for '\0' appended to szAlertMessage

    pData = (char*)malloc(iMsgSize);
    pMsg  = (mmp_mngt_TdmfAlertMsg_t*)pData;
    pWk   = (char*)(pMsg+1);//points after mmp_mngt_TdmfAlertMsg_t struct, on first byte of messages area
    
    pMsg->hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    pMsg->hdr.mngttype       = MMP_MNGT_ALERT_DATA;
    pMsg->hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    pMsg->hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
    pMsg->data               = *alertData;
    pMsg->iLength            = iModuleNameSize + 1 + iAlertMessageSize + 1;

    if ( szModuleName != 0 )
        strcpy(pWk, szModuleName);
    else
        *pWk = '\0';//empty string

    pWk += iModuleNameSize + 1;//+ 1 to skip '\0'

    if ( szAlertMessage != 0 )
        strcpy(pWk, szAlertMessage);
    else
        *pWk = '\0';//empty string

    //convert to network byte order before sending on socket
    mmp_convert_mngt_hdr_hton(&pMsg->hdr);
    mmp_convert_TdmfAlert_hton(&pMsg->data);
    pMsg->iLength = htonl(pMsg->iLength);
    //open socket on Collector port ,send msg and close socket connection.
    towrite = iMsgSize;
    r = mmp_sendmsg(giTDMFCollectorIP,giTDMFCollectorPort,(char*)pMsg,towrite,0,0);ASSERT(r == towrite);
    if ( r != towrite )
    {
        if ( r == -MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR )
        {   //TDMF Collector not found !
            char ipstr[24];
            ip_to_ipstring(giTDMFCollectorIP,ipstr);
            DBGPRINT((2,"****Warning, Collector not found at IP=%s , Port=%d !\n",ipstr,giTDMFCollectorPort));
        }
        else
        {
            DBGPRINT((2,"****Error (%d) while sending AlertMsg to Collector!\n",r));
        }
        r = -1;//failure
    }
    else
    {
        pWk = (char*)(pMsg+1);
        DBGPRINT((3,"AlertMsg sent : <%s>,<%s>\n", pWk, pWk + iModuleNameSize + 1 ));
        r = 0;//success
    }

    free(pData);
}


int  ftd_mngt_get_perf_cfg(sock_t *sockp)
{
    mmp_mngt_TdmfPerfCfgMsg_t   msg;
    int                         r,toread;
    char                        *pWk;

    pWk  = (char*)&msg;
    pWk += sizeof(mmp_mngt_header_t);
    //////////////////////////////////
    //at this point, mmp_mngt_header_t header is read.
    //now read the remainder of the mmp_mngt_TdmfPerfCfgMsg_t structure 
    toread = sizeof(mmp_mngt_TdmfPerfCfgMsg_t)-sizeof(mmp_mngt_header_t);
    r = mmp_mngt_sock_recv(sockp,(char*)pWk, toread);ASSERT(r == toread);
    if ( r != toread )
    {
        DBGPRINT((1,"****Error, bad format of Performance Config message received from 0x%x \n",sockp->rip));
        return -1;
    }
    mmp_convert_TdmfPerfConfig_ntoh( &msg.data );


    //save config to registry
    //do same thing for all values in msg.data
    char tmp[32];
    if (msg.data.iPerfUploadPeriod > 0)
    {
        //modify dynamic instance of this value
        gTdmfPerfConfig.iPerfUploadPeriod = msg.data.iPerfUploadPeriod;
        if ( gTdmfPerfConfig.iPerfUploadPeriod <= 0 )
            gTdmfPerfConfig.iPerfUploadPeriod = 100;//10 seconds

        itoa(gTdmfPerfConfig.iPerfUploadPeriod,tmp,10);
        if ( cfg_set_software_key_value("PerfUploadPeriod", tmp, CFG_IS_NOT_STRINGVAL) != CFG_OK )
        {
            DBGPRINT((2,"WARNING! Unable to write \'PerfUploadPeriod\' configuration value !\n"));
        }
    }
    if (msg.data.iReplGrpMonitPeriod > 0)
    {
        //modify dynamic instance of this value
        gTdmfPerfConfig.iReplGrpMonitPeriod = msg.data.iReplGrpMonitPeriod;
        if ( gTdmfPerfConfig.iReplGrpMonitPeriod <= 0 )
            gTdmfPerfConfig.iReplGrpMonitPeriod = 100;//10 seconds

        itoa(gTdmfPerfConfig.iReplGrpMonitPeriod,tmp,10);
        if ( cfg_set_software_key_value("ReplGroupMonitUploadPeriod", tmp, CFG_IS_NOT_STRINGVAL) != CFG_OK )
        {
            DBGPRINT((2,"WARNING! Unable to write \'ReplGroupMonitUploadPeriod\' configuration value !\n"));
        }
    }

    return 0;
}


#include "errmsg_list.h"
//#define MSG_LOG_FILE

//THE one and only StatusMsgList list instance of a TDMF Agent/Client
//defined in errmsg_list.h
extern StatusMsgList    gStatusMsgList;

//
// Took out the lastAliveMsgTime variable from the thread
// this is safe because there is only one copy of this 
// thread (otherwise we would have to create a more complex 
// mechanism)
//
static time_t  lastAliveMsgTime = 0;

//
// Set alive message time stamp to current time
// this allows us to not send a alive message
// whenever there are other messages being
// sent around by the agent to the collector
// minimizing traffic
//
void ftd_mngt_UpdateAliveMsgTime(void)
{
    //
    // Make sure we sent at least one alive message to the collector.
    // since we initialize this variable to 0 when we connect the
    // alive socket, and we update it the first time we send the message
    // this is a correct test.
    //
    if (lastAliveMsgTime!=0)
    {
        lastAliveMsgTime = time(0);
    }
}

//Thread launched when TDMF Agent starts.
static DWORD WINAPI ftd_mngt_StatusMsgThread(PVOID notused)
{
    int r,towrite;
    mmp_mngt_TdmfStatusMsgMsg_t *pMsg;
    int iSize = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + 1024;
    char *pData = new char [iSize];
    pMsg = (mmp_mngt_TdmfStatusMsgMsg_t *)pData;

    //////////////////////////////////
    ftd_init_errfac("Replicator", "StatusMsgThread", NULL, NULL, 0, 0);

    //////////////////////////////////////////////////////
    //init. Status msg constant values
    //////////////////////////////////////////////////////
    pMsg->hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    pMsg->hdr.mngttype       = MMP_MNGT_STATUS_MSG;
    pMsg->hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    pMsg->hdr.mngtstatus     = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&pMsg->hdr);
    ftd_mngt_getServerId( pMsg->data.szServerUID );

    //////////////////////////////////////////////////////
    //stuff releated to the Agent Alive Socket
    //////////////////////////////////////////////////////
    sock_t  *aliveSocket = sock_create();
    bool    bAliveSockConnected = false;
    WSAEVENT wsaevClose = WSACreateEvent();//Windows specific ...

#ifdef MSG_LOG_FILE
    FILE *fp = fopen("debugTDMFStatusMsg.txt","a");
#endif
    //loop until thread is requested to quit
    while( WaitForSingleObject(gevEndStatusMsgThread,0) == WAIT_TIMEOUT )
    {
        //wake up periodically and send any available Status Msg to TDMF Collector
        if ( gStatusMsgList.WaitForNewMessages(1000) )
        {
            //gStatusMsgList.Lock();//ac : avoid locking because mmp_sendmsg() may take a few seconds...
            IterStatusMsg it = gStatusMsgList.m_list.begin();
            //scan all status msg in list
            while( it != gStatusMsgList.m_list.end() )
            {
#ifdef _DEBUG
                StatusMsg msg = *it;//allow to view *it content in debugger
#endif
                if ( (*it).isMsgProcessed() )
                {
                    if ( (*it).getTag() == StatusMsg::TAG_MESSAGE )
                    {
                        ///////////////////////////////////////////////////
                        //prepare Status Msg to be sent to TDMF Collector
                        ///////////////////////////////////////////////////
                        pMsg->data.iLength    = (*it).getMessageLength() + 1;      //length of message string following this structure, including terminating '\0' character.

                        if ( sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + pMsg->data.iLength >= iSize )
                        {
                            int iNewSize  = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + pMsg->data.iLength + 512;
                            pData = new char [ iNewSize ];
                            memmove(pData,pMsg,iSize);
                            delete [] pMsg;
                            pMsg  = (mmp_mngt_TdmfStatusMsgMsg_t *)pData;
                            iSize = iNewSize;
                        }

                        char *pMsgString      = (char*)(pMsg+1);
                        pMsg->data.cPriority  = (*it).getPriority();
                        pMsg->data.cTag       = (*it).getTag();
                        pMsg->data.iTdmfCmd   = (*it).getTdmfCmd();
                        pMsg->data.iTimeStamp = (*it).getTimeStamp();
                        strcpy( pMsgString, (*it).getMessage() );
                        pMsgString[ pMsg->data.iLength - 1 ] = 0;//force EOS at end of string
#ifdef MSG_LOG_FILE
                        if (fp) 
                        {   //debug only.  code to be removed
                            char *dt = ctime( (const long*)&pMsg->data.iTimeStamp );
                            dt[23] = 0;
                            fprintf(fp,"tag=%-20s cmd=0x%x prio=%-10s ts=%s    msg=<%s>\n"
                                        ,pMsg->data.cTag == StatusMsg::TAG_MESSAGE ? "msg" : pMsg->data.cTag == StatusMsg::TAG_SUBLIST_BEGIN ? "Begin" : "End"
                                        ,pMsg->data.iTdmfCmd
                                        ,pMsg->data.cPriority == LOG_INFO ? "info" : pMsg->data.cTag == LOG_WARNING ? "warning" : pMsg->data.cTag == LOG_CRIT ? "critical" : "error" 
                                        ,dt
                                        ,pMsg->data.cTag == StatusMsg::TAG_MESSAGE ? (char*)(pMsg+1) : ""
                                        );
                            fflush(fp);
                        }
#endif

                        ///////////////////////////////////
                        //send Status Msg to TDMF Collector
                        ///////////////////////////////////
                        towrite = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + pMsg->data.iLength;
                        mmp_convert_TdmfStatusMsg_hton(&pMsg->data);
                        r = mmp_sendmsg(giTDMFCollectorIP,giTDMFCollectorPort,(char*)pMsg,towrite,0,0);
                        //
                        // If we sent a message, update
                        // the last alive timestamp
                        //
                        ftd_mngt_UpdateAliveMsgTime();
                    }
                    //delete msg from Status Msg list
                    //IterStatusMsg itdel = it;
                    //it++;
                    //gStatusMsgList.m_list.erase(itdel,it);//remove the message just processed.
                    gStatusMsgList.Lock();
                    it = gStatusMsgList.m_list.erase(it);//remove the message just processed.
                    gStatusMsgList.Unlock();
                }
                else
                    it++;
            }
            //gStatusMsgList.Unlock();//ac : avoid locking because mmp_sendmsg() may take a few seconds...
        }

        //here, we work with the Alive Socket
        ftd_mngt_alive_socket(aliveSocket,&bAliveSockConnected,wsaevClose,&lastAliveMsgTime);
    }

#ifdef MSG_LOG_FILE
    if (fp) fclose(fp);
#endif

    delete [] pData;

    WSACloseEvent(wsaevClose);//windows specific ...
    sock_disconnect(aliveSocket);
    sock_delete(&aliveSocket);
    ftd_delete_errfac();

    return 0;
}

void 
ftd_mgnt_SetAliveTimeout(unsigned int Timeout)
{
    g_uiAliveTimeout = Timeout;
}

static void
ftd_mngt_alive_socket(sock_t* aliveSocket,bool *pbAliveSockConnected,WSAEVENT wsaevClose,time_t * pLastAliveMsgTime)
{
    if ( *pbAliveSockConnected == false )
    {   
        if ( giTDMFCollectorIP == 0 || giTDMFCollectorPort == 0 )
            return;

        int r = sock_init(aliveSocket,0,0,0, giTDMFCollectorIP, SOCK_STREAM, AF_INET, 1, 0);
        if ( r < 0 )
            return;

        r = sock_connect(aliveSocket,giTDMFCollectorPort);

		// Modified By Saumya Tripathi 03/18/04
		// Fixing WR 32854
        if ( r < 0 )
        {
            sock_disconnect(aliveSocket);
			gbTDMFCollectorPresent = false;
            return;
        }
		else
		{
			gbTDMFCollectorPresent = true;
			gbTDMFCollectorPresentWarning = false;
		}

        //send the MMP_MNGT_AGENT_ALIVE_SOCKET msg
        int   len;
        char  szAliveMsgTagValue[32];//for now, 32 is enough ...
        mmp_mngt_TdmfAgentAliveMsg_t msg;
        msg.hdr.magicnumber     = MNGT_MSG_MAGICNUMBER;
        msg.hdr.mngttype        = MMP_MNGT_AGENT_ALIVE_SOCKET;
        msg.hdr.mngtstatus      = MMP_MNGT_STATUS_OK; 
        msg.hdr.sendertype      = SENDERTYPE_TDMF_AGENT;
        mmp_convert_mngt_hdr_hton(&msg.hdr);

        ftd_mngt_getServerId( msg.szServerUID );
        // >>> make sure szAliveMsgTagValue can eat it all up ...
        sprintf(szAliveMsgTagValue,"%s%d ",MMP_MNGT_ALIVE_TAG_MMPVER,MMP_PROTOCOL_VERSION);
        len                     = strlen(szAliveMsgTagValue) + 1;//include '\0'
        msg.iMsgLength          = htonl(len);

        //send status message to requester    
        r = mmp_mngt_sock_send(aliveSocket, (char*)&msg, sizeof(msg)); 
        if ( r == sizeof(msg) )
        {   //send variable data portion of message
            mmp_mngt_sock_send(aliveSocket, szAliveMsgTagValue, len); 

            WSAResetEvent(wsaevClose);
            WSAEventSelect( aliveSocket->sock, wsaevClose, FD_CLOSE );
            *pbAliveSockConnected = true;
            //some information must be sent to Collector
            //begin by sending this repl. server general info
            ftd_mngt_send_agentinfo_msg(0,0);
            ftd_mngt_send_registration_key();
            //now, update Collector with latest repl. group stats and state
            ftd_mngt_performance_send_all_to_Collector();
        }
        else 
        {
            sock_disconnect(aliveSocket);
        }
        *pLastAliveMsgTime = 0;
    }
    else
    {   //verify that socket is still connected
        if ( WSA_WAIT_EVENT_0 == WSAWaitForMultipleEvents(1,&wsaevClose,FALSE,0/*no wait*/,FALSE) )
        {   //socket's dead, Collector has gone for lunch...
            WSAResetEvent(wsaevClose);
            *pbAliveSockConnected = false;
            sock_disconnect(aliveSocket);
        }
        else
        {   //ok, socket still connected 
            time_t now = time(0);
            if ( (now - *pLastAliveMsgTime) >= g_uiAliveTimeout )
            {   //time to send a Alive msg to peer (Collector)
                //send the MMP_MNGT_AGENT_ALIVE_SOCKET msg
                mmp_mngt_TdmfAgentAliveMsg_t msg;
                msg.hdr.magicnumber     = MNGT_MSG_MAGICNUMBER;
                msg.hdr.mngttype        = MMP_MNGT_AGENT_ALIVE_SOCKET;
                msg.hdr.mngtstatus      = MMP_MNGT_STATUS_OK; 
                msg.hdr.sendertype      = SENDERTYPE_TDMF_AGENT;
                mmp_convert_mngt_hdr_hton(&msg.hdr);
                ftd_mngt_getServerId( msg.szServerUID );
                msg.iMsgLength          = htonl(0);//no Tag-Value pairs in this msg
                //send status message to requester    
                mmp_mngt_sock_send(aliveSocket, (char*)&msg, sizeof(msg)); 

                *pLastAliveMsgTime = now;
            }

        }
    }
}

//////////////////////////////////////////////////////////////////////////////
static bool consoleOutput_init(ConsoleOutputCtrl *ctrl)
{
    HANDLE hOutputReadTmp;
    SECURITY_ATTRIBUTES sa;
    // Set up the security attributes struct.
    sa.nLength= sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    ctrl->iSize = 1024;
    ctrl->pData = new char [ctrl->iSize];
    ctrl->iTotalDataRead = 0;

    // Create the child process output pipe.
    if (!CreatePipe(&hOutputReadTmp,&ctrl->hOutputWrite,&sa,0))
    {
        _ASSERT(0);
        return false;
    }
    // Create a duplicate of the output write handle for the std error
    // write handle. This is necessary in case the child application
    // closes one of its std output handles.
    if (!DuplicateHandle(GetCurrentProcess(), ctrl->hOutputWrite,
                         GetCurrentProcess(),&ctrl->hErrorWrite,
                         0,TRUE,
                         DUPLICATE_SAME_ACCESS))
    {
        _ASSERT(0);
        return false;
    }

    // Create new output read handle and the input write handles. Set
    // the Properties to FALSE. Otherwise, the child inherits the
    // properties and, as a result, non-closeable handles to the pipes
    // are created.
    if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
                         GetCurrentProcess(),&ctrl->hOutputRead, // Address of new handle.
                         0,FALSE, // Make it uninheritable.
                         DUPLICATE_SAME_ACCESS))
    {
        _ASSERT(0);
        return false;
    }

    // Close inheritable copies of the handles you do not want to be
    // inherited.
    if (!CloseHandle(hOutputReadTmp)) 
    {
        _ASSERT(0);
        return false;
    }

    return true;
}

static bool consoleOutput_read(ConsoleOutputCtrl *ctrl)
{
    if ( ctrl->pData == 0 )
    {
        _ASSERT(0);
        return false;
    }
    // Close pipe handles (do not continue to modify the parent).
    // You need to make sure that no handles to the write end of the
    // output pipe are maintained in this process or else the pipe will
    // not close when the child process exits and the ReadFile will hang.
    if (!CloseHandle(ctrl->hOutputWrite)) 
    {
        _ASSERT(0);
        return false;
    }
    if (!CloseHandle(ctrl->hErrorWrite)) 
    {
        _ASSERT(0);
        return false;
    }

    DWORD read = 0;
    ctrl->iTotalDataRead = 0;
    while ( ReadFile(ctrl->hOutputRead, 
                     ctrl->pData + ctrl->iTotalDataRead, 
                     ctrl->iSize - ctrl->iTotalDataRead, 
                     &read,0) )
    {
        ctrl->iTotalDataRead += read;
        if ( ctrl->iTotalDataRead == ctrl->iSize )
        {   //enlarge buffer by 1KB and continue reading from child output stream Pipe
            char *pNewData = new char [ ctrl->iSize + 1024 ];
            memmove(pNewData, ctrl->pData, ctrl->iSize );
            ctrl->iSize += 1024;
            delete [] ctrl->pData;
            ctrl->pData = pNewData;
        }
    }
    //printf("\n\nNo more data received from child process.");

    //because console output data is text-based, append '\0' at end of data received
    if ( ctrl->iTotalDataRead == ctrl->iSize )
    {   //enlarge buffer by 1 byte to ensure EOS
        char *pNewData = new char [ ctrl->iSize + 1 ];
        memmove(pNewData, ctrl->pData, ctrl->iSize );
        ctrl->iSize += 1;
        delete [] ctrl->pData;
        ctrl->pData = pNewData;
    }
    ctrl->pData[ ctrl->iTotalDataRead ] = 0;//ensure end of text string
    ctrl->iTotalDataRead++;

    return true;
}


static void consoleOutput_delete(ConsoleOutputCtrl *ctrl)
{
    if (!CloseHandle(ctrl->hOutputRead)) 
    {
        _ASSERT(0);
    }
  
    delete [] ctrl->pData;
    ctrl->iSize = 0;
    ctrl->pData = 0;
    ctrl->iTotalDataRead = 0;
}



///////////////////////////////////////////////////////////////////////////////
int  ftd_mngt_send_lg_state(int lgnum, bool isPrimary, ftd_mngt_lg_monit_t* monitp)
{
    mmp_mngt_TdmfReplGroupMonitorMsg_t *pmsg = (mmp_mngt_TdmfReplGroupMonitorMsg_t *)monitp->msg;
    
    if ( pmsg == NULL )
        return -1;

    //build msg
    if ( pmsg->hdr.magicnumber == 0 )
    {   //most of the message info. is static, so init it once and use it many times.
        //init. static data of msg only once
        pmsg->hdr.magicnumber     = MNGT_MSG_MAGICNUMBER;
        pmsg->hdr.mngttype        = MMP_MNGT_GROUP_MONITORING;
        pmsg->hdr.mngtstatus      = MMP_MNGT_STATUS_OK; 
        pmsg->hdr.sendertype      = SENDERTYPE_TDMF_AGENT;
        mmp_convert_mngt_hdr_hton(&pmsg->hdr);

        memset(&pmsg->data,0,sizeof(pmsg->data));
        pmsg->data.iReplGrpSourceIP = 0;//updated only if ROLESECONDARY
        pmsg->data.liDiskFreeSz     = 0;
        pmsg->data.liActualSz       = 0;
        ftd_mngt_getServerId( pmsg->data.szServerUID );
    }

    //init dynamic data of msg
    if ( isPrimary )
    {   //a ReplGroup Primary machine uses the PStore 
        pmsg->data.liDiskTotalSz    = monitp->pstore_disk_total_sz;
        pmsg->data.liDiskFreeSz     = monitp->pstore_disk_free_sz;   
        pmsg->data.liActualSz       = monitp->curr_pstore_file_sz;
        pmsg->data.isSource         = 1;
    }
    else
    {   //a ReplGroup Secondary machine uses the Journal
        pmsg->data.liDiskTotalSz    = monitp->journal_disk_total_sz;
        pmsg->data.liDiskFreeSz     = monitp->journal_disk_free_sz;   
        pmsg->data.liActualSz       = monitp->curr_journal_files_sz;
        pmsg->data.isSource         = 0;
        pmsg->data.iReplGrpSourceIP = monitp->iReplGrpSourceIP;
    }
    pmsg->data.sReplGrpNbr      = (short)lgnum;
    mmp_convert_hton(&pmsg->data);

    //send msg to Collector
    //open socket on Collector port ,send msg and close socket connection.
    int r, towrite = sizeof(mmp_mngt_TdmfReplGroupMonitorMsg_t);
    r = mmp_sendmsg(giTDMFCollectorIP,giTDMFCollectorPort,(char*)pmsg,towrite,0,0);
    if ( r != 0 )
    {
        if ( r == -MMP_MNGT_STATUS_ERR_CONNECT_TDMFCOLLECTOR )
        {   //TDMF Collector not found !
            char ipstr[24];
            ip_to_ipstring(giTDMFCollectorIP,ipstr);
            DBGPRINT((2,"****Warning, Collector not found at IP=%s , Port=%d !\n",ipstr,giTDMFCollectorPort));
        }
        else
        {
            DBGPRINT((1,"****Error (%d) while sending ReplGroupMonitorMsg to Collector!\n",r));
        }
        r = -2;//failure
    }

    return r;
}

int ftd_mngt_get_ip_list(unsigned long *pulIPList, int nMaxInList)
{
    char    *pszAppOutput = NULL;
    int     i,iAppExitCode;
    int     ipfound = 0;
    int     TempIpValue =0;
    int     ipValue =0;
    HANDLE  hMutex;

    //problems were encounters when calling ftd_mngt_get_ip_list() ( ipconfig.exe /all )
    //by multiple threads, simultaneously.  This is the reason for this Mutex.
    hMutex = CreateMutex(0,0,"ftd_mngt_get_ip_list_Mutex");
    if ( hMutex != 0 )
    {
        WaitForSingleObject(hMutex,INFINITE);
    }
    else
    {
        ASSERT(0);
    }


    //for ( i = 0 ; i < nMaxInList ; i++ )
    //    pulIPList[ i ] = 0;//reset members in list 

    //i = consoleOutput_Launch( NULL, "ipconfig.exe /all ", 0, &pszAppOutput, &iAppExitCode );
    //if ( i == 0 && pszAppOutput != NULL )
    //{
    //    char *pCur = pszAppOutput;//pszAppOutput is a zero-terminated string

    //    while( (pCur = util_find_ip_addr(pCur)) != NULL     && 
    //           ipfound < nMaxInList                             )
    //    {
    //        ipstring_to_ip( pCur, pulIPList + ipfound );
    //        ipfound++ ;
    //    }

    //    free( pszAppOutput );
    //}


    //if ( hMutex != 0 )
    //{
    //    ReleaseMutex(hMutex);
    //    if (!CloseHandle(hMutex)) 
    //    {
    //        ASSERT(0);
    //    }
    //}

	for ( i = 0 ; i < nMaxInList ; i++ )
        pulIPList[ i ] = 0;//reset members in list 

    ipfound = GetAllIpAddresses(pulIPList, nMaxInList);

    if ( hMutex != 0 )
    {
        ReleaseMutex(hMutex);
        if (!CloseHandle(hMutex)) 
        {
            ASSERT(0);
        }
    }

    //
    // This is a quick and dirty trick!
    // We get the local ip of the machine
    // that we know connects to the collector
    // and we make sure that it is the first
    // ip in the list on the collector!
    // (The collector uses this ip to talk to us)
    //
    //ipValue = ftd_mngt_get_local_ip_for_collector_routing();
	ipValue = Get_Selected_DtcIP_from_Install();

	if (!ipValue)
       ipValue = ftd_mngt_get_local_ip_for_collector_routing(); // in case...

    if (ipValue != 0)
    {
        for (i = 0; i < nMaxInList ; i++ )
        {
            if (pulIPList[i] == ipValue)
            {
                //
                // set our ip as first in the list,
                // and set the first in the list in
                // our own entry
                //
#ifdef _DEBUG
                DBGPRINT((2,"found at index %d\n\n",i));
#endif

                TempIpValue = pulIPList[0];
                pulIPList[0] = ipValue;
                pulIPList[i] = TempIpValue;
                break; // get out of for loop, we found it!
            }
        }
#ifdef _DEBUG
        if (TempIpValue == 0)
        {
            DBGPRINT((2,"Did not find ip in iplist!\n"));
        }
#endif
    }
#ifdef _DEBUG
    else
    {
        DBGPRINT((2,"No ip is able to reach the collector!\n"));
    }
#endif

    return ipfound;
}

//
// Called to make sure we have read the collector values from the
// registry and know where to connect to.
//
void ftd_mngt_initialize_collector_values(void)
{
    char                tmp[32];

    giTDMFCollectorIP   = 0;
    giTDMFCollectorPort = 0;

    //
    // get the ip of the collector
    //
    if ( cfg_get_software_key_value("DtcCollectorIP", tmp, CFG_IS_STRINGVAL) == CFG_OK )
    {
        ipstring_to_ip(tmp,(unsigned long*)&giTDMFCollectorIP);
    }
    else 
    {
        giTDMFCollectorIP = 0;//to be init one first msg received from TDMF Collector
                              //until then , no msg can be sent.
    }

    //
    // Get the port of the collector
    //
    if ( cfg_get_software_key_value("DtcCollectorPort", tmp, CFG_IS_NOT_STRINGVAL) == CFG_OK )
    {
        giTDMFCollectorPort = atoi(tmp);
    }
    else
    {
        giTDMFCollectorPort = TDMF_COLLECTOR_DEF_PORT;//default value
    }
}

//
// This function assumes that the global values
// for the TDMFCollector ip and port have
// been assigned by calling the previous function
// (ftd_mngt_initialize_collector_values)
//
// The function connects to the collector and
// checks the ip we used to connect to him
// the function then returns the value of the ip 
// that was used 
extern "C" int ftd_mngt_get_local_ip_for_collector_routing(void)
{
    int                 lip;            // Local IP
    int                 rip;            // Remote IP
    int                 rport;          // Remote port
    int                 r;              // result
    int                 ilen;           // length of ip result
    struct sockaddr_in  sa_in;          // TCP/IP socket address structure

    rip     = giTDMFCollectorIP;
    rport   = giTDMFCollectorPort;    

    //
    // Create a temporary throw away socket
    //
    sock_t * s = sock_create();

    //
    // Initialize a socket with the remote ip set to TDMFCollectorIP
    //
    r = sock_init( s, NULL, NULL, 0, rip, SOCK_STREAM, AF_INET, 1, 0); _ASSERT(r>=0);

    if ( r >= 0 ) 
    {
        //
        // Connect the socket to the TDMFCollectorSocket
        //
        r = sock_connect(s, rport); 

        //
        // If we were able to connect to the collector
        //
        if ( r >= 0 ) 
        {
            //
            // Get the ip address of the card we used to connect to the collector
            //
            ilen = sizeof(sa_in);                
            r = getsockname(s->sock, (struct sockaddr *)&sa_in, &ilen); 

            if (r >=0)
            {
#ifdef _DEBUG
                DBGPRINT((2,"Local ip that sees collector: %d.%d.%d.%d\n",
                            sa_in.sin_addr.S_un.S_un_b.s_b1,
                            sa_in.sin_addr.S_un.S_un_b.s_b2,
                            sa_in.sin_addr.S_un.S_un_b.s_b3,
                            sa_in.sin_addr.S_un.S_un_b.s_b4));
#endif                
                lip = sa_in.sin_addr.S_un.S_addr;;
            }
        }
        //
        // Disconnect the throw-away socket
        //
        sock_disconnect(s);
    }  
    else
    {
        r = -1;
    }
    //
    // delete the throw-away socket
    //
    sock_delete(&s);

    //
    // If we were able to find a local ip, return it's value
    //
    if (r>=0)
    {
        return lip;
    }
    //
    // otherwise return 0!
    //
    else
    {
        return 0;
    }
}

void SendAllPendingMessagesToCollector(void)
{
    int r,towrite;
    mmp_mngt_TdmfStatusMsgMsg_t *pMsg;
    int iSize = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + 1024;
    char *pData = new char [iSize];
    pMsg = (mmp_mngt_TdmfStatusMsgMsg_t *)pData;

    //////////////////////////////////////////////////////
    //init. Status msg constant values
    //////////////////////////////////////////////////////
    pMsg->hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    pMsg->hdr.mngttype       = MMP_MNGT_STATUS_MSG;
    pMsg->hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    pMsg->hdr.mngtstatus     = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&pMsg->hdr);
    ftd_mngt_getServerId( pMsg->data.szServerUID );

    IterStatusMsg it = gStatusMsgList.m_list.begin();
    //scan all status msg in list
    while( it != gStatusMsgList.m_list.end() )
    {
#ifdef _DEBUG
        StatusMsg msg = *it;//allow to view *it content in debugger
#endif
        if ( (*it).isMsgProcessed() )
        {
            if ( (*it).getTag() == StatusMsg::TAG_MESSAGE )
            {
                ///////////////////////////////////////////////////
                //prepare Status Msg to be sent to TDMF Collector
                ///////////////////////////////////////////////////
                pMsg->data.iLength    = (*it).getMessageLength() + 1;      //length of message string following this structure, including terminating '\0' character.

                if ( sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + pMsg->data.iLength >= iSize )
                {
                    int iNewSize  = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + pMsg->data.iLength + 512;
                    pData = new char [ iNewSize ];
                    memmove(pData,pMsg,iSize);
                    delete [] pMsg;
                    pMsg  = (mmp_mngt_TdmfStatusMsgMsg_t *)pData;
                    iSize = iNewSize;
                }

                char *pMsgString      = (char*)(pMsg+1);
                pMsg->data.cPriority  = (*it).getPriority();
                pMsg->data.cTag       = (*it).getTag();
                pMsg->data.iTdmfCmd   = (*it).getTdmfCmd();
                pMsg->data.iTimeStamp = (*it).getTimeStamp();
                strcpy( pMsgString, (*it).getMessage() );
                pMsgString[ pMsg->data.iLength - 1 ] = 0;//force EOS at end of string

                ///////////////////////////////////
                //send Status Msg to TDMF Collector
                ///////////////////////////////////
                towrite = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + pMsg->data.iLength;
                mmp_convert_TdmfStatusMsg_hton(&pMsg->data);
                r = mmp_sendmsg(giTDMFCollectorIP,giTDMFCollectorPort,(char*)pMsg,towrite,0,0);
            }
            //delete msg from Status Msg list
            //IterStatusMsg itdel = it;
            //it++;
            //gStatusMsgList.m_list.erase(itdel,it);//remove the message just processed.
            gStatusMsgList.Lock();
            it = gStatusMsgList.m_list.erase(it);//remove the message just processed.
            gStatusMsgList.Unlock();
        }
        else
            it++;
    }
}
