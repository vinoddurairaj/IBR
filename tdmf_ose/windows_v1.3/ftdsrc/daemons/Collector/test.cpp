/*
 * test.cpp - test code
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
#ifdef _DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "DBServer.h"
#include "libmngt.h"
#include "libmngtmsg.h"
extern "C" {
#include "sock.h"
#include "iputil.h"
}

#pragma comment(lib, "../../lib/libmngt/Debug/libmngt.lib") 


void debug_test_brdcst(int argc, char *argv[], CollectorConfig *cfg , bool isServer )
{
    int r;

    sock_startup();

    sock_t* brdcst = sock_create();
    r = sock_init( brdcst, NULL, NULL, cfg->ulIP, cfg->ulBroadcastIP, SOCK_DGRAM, AF_INET, 1, 0);
    if ( r >= 0 ) 
    {
        r = sock_bind_to_port( brdcst, TDMF_BROADCAST_PORT );

        if ( r >= 0 ) 
        {
            if ( !isServer )
            {
                mmp_mngt_BroadcastReqMsg_t  msg;
                printf("\n client waiting for broadcast ...");
                r = sock_recvfrom( brdcst, (char*)&msg, sizeof(msg) );_ASSERT(r == sizeof(msg));
                printf("\n recvfrom %x , %d bytes : mngttype=%d , TDMF Server brdcst response port=%d ", brdcst->rip, r, ntohl(msg.hdr.mngttype), ntohl(msg.data.iBrdcstResponsePort));
            }
            else
            {
                mmp_mngt_BroadcastReqMsg_t  msg;
                msg.hdr.magicnumber          = MNGT_MSG_MAGICNUMBER;
                msg.hdr.mngttype             = MMP_MNGT_AGENT_INFO_REQUEST;
                msg.hdr.sendertype           = SENDERTYPE_TDMF_SERVER;
                mmp_convert_mngt_hdr_hton(&msg.hdr);
                msg.data.iBrdcstResponsePort = htonl(cfg->iPort);

                printf("\n server is broadcasting now ...");
                r = sock_sendto( brdcst, (char*)&msg, sizeof(msg) );_ASSERT(r == sizeof(msg));
                printf("\n sendto %x , %d bytes : mngttype=%d , Server iPort=%d ", brdcst->rip, r, ntohl(msg.hdr.mngttype), ntohl(msg.data.iBrdcstResponsePort));
            }
        }
        else
        {
            printf("\nWarning! unable to bind to port TDMF_BROADCAST_PORT\n");
        }
    }
    sock_delete(&brdcst);
    printf("\n hit a key");getch();
}


void debug_simul_client(int argc, char* argv[], CollectorConfig *cfg )
{
    sock_startup();

    //first client sends a broadcast request
    MMP_HANDLE  handle = mmp_Create(    argv[2] , //tdmf server ip
                                        atoi(argv[3]) //tdmf server port
                                        );

    printf("client calls mmp_getAllTdmfAgentInfo() ...\n");
    int r = mmp_getAllTdmfAgentInfo( handle );
    printf("mmp_getAllTdmfAgentInfo() returns %d...\n",r);


    printf("\n hit a key");getch();
}


void debug_simul_client_regkeyreq(int argc, char* argv[], CollectorConfig *cfg )
{
    sock_startup();

    char szRegKey[80];
    char szAgentId[80];
    szRegKey[0] = 0;

    printf("\nEnter TDMF Agent Id (machine name) :");scanf("%s",szAgentId);


    //first client sends a broadcast request
    MMP_HANDLE  handle = mmp_Create(    argv[2] , //tdmf server ip
                                        atoi(argv[3]) //tdmf server port
                                        );

    printf("client calls mmp_getTdmfAgentRegistrationKey() for %s...\n",szAgentId);
    int r = mmp_getTdmfAgentRegistrationKey(handle, szAgentId, szRegKey, sizeof(szRegKey));
    printf("mmp_getTdmfAgentRegistrationKey() returns %d, key=%s...\n",r,szRegKey);


    printf("\n hit a key");getch();

}




void debug_simul_client_setregkey(int argc, char* argv[], CollectorConfig *cfg )
{
    sock_startup();

    char szRegKey[80];
    char szAgentId[80];
    szRegKey[0] = 0;

    printf("\nEnter TDMF Agent Id (machine name) :");scanf("%s",szAgentId);
    printf("\nEnter Reg Key :");scanf("%s",szRegKey);


    //first client sends a broadcast request
    MMP_HANDLE  handle = mmp_Create(    argv[2] , //tdmf server ip
                                        atoi(argv[3]) //tdmf server port
                                        );

    printf("client calls mmp_setTdmfAgentRegistrationKey() for %s , %s...\n",szAgentId,szRegKey);
    int r = mmp_setTdmfAgentRegistrationKey(handle, szAgentId, szRegKey);
    printf("mmp_setTdmfAgentRegistrationKey() returns %d ...\n",r);


    printf("\n hit a key");getch();

}



void debug_simul_client_getLGcfg(int argc, char* argv[], CollectorConfig *cfg )
{
    sock_startup();

    char szRegKey[80];
    char szAgentId[80];
    int lgid;
    char cPriSec;
    char *pCfgData;
    unsigned int uiDataSize;
    szRegKey[0] = 0;

    printf("\nEnter TDMF Agent Id (machine name) :");scanf("%s",szAgentId);
    printf("\nEnter logical group nbr :");scanf("%d",&lgid);
    //printf("\nEnter primary (p) or secondary (s) :");scanf("%c",&cPriSec);
    cPriSec = 'p';

    //first client sends a broadcast request
    MMP_HANDLE  handle = mmp_Create(    argv[2] , //tdmf server ip
                                        atoi(argv[3]) //tdmf server port
                                        );

    char fname[32];
    sprintf(fname,"%c%03d.cfg",cPriSec,lgid);
    printf("client calls mmp_getTdmfAgentLGConfig() for %s , cfg file=%s ...\n",szAgentId,fname);

    int r = mmp_getTdmfAgentLGConfig(handle, 
                                 szAgentId,
                                 (short)lgid,
                                 cPriSec == 'p' || cPriSec == 'P' ,
                                 &pCfgData,
                                 &uiDataSize
                                 );
    printf("mmp_getTdmfAgentLGConfig() returns %d ...\n",r);
    if ( r == 0 )
    {
        FILE *f = fopen(fname,"w+b");
        if (f)
        {
            fwrite(pCfgData,1,uiDataSize,f);
            fclose(f);
            printf("cfg file %s saved to disk\n",fname);
        }
    }

    /*
    mmp_TdmfSystemConfig  syscfg;
    mmp_TdmfLGConfig      lgcfg[5];  

    syscfg.iPriIP = 0x11223344;
    strcpy( syscfg.szPriPStore, "c:\\primary\\pstore");         //primary system complete file name or device name
    syscfg.iSecIP = 0x55667788;                     //secondary system ip address
    strcpy( syscfg.szSecJournalPath, "c:\\secondary\\journalpath");    //secondary system journal path 
    syscfg.iSecPort = 577;                   //secondary system socket Port number
    syscfg.bChaining = 0;                  //boolean flag: 0 = off, 1 = on 
    //Tunable parameters ??
	syscfg.iPort = 575;				        //port nbr exposed by this TDMF Agent
	syscfg.iTCPWindowSizeKB = 256;	        //KiloBytes
    syscfg.iBABSizeMB = 64;                 //MegaBytes           
    for(int i=0;i<5;i++)
    {
        lgcfg[i].sNumber    =  i;
        sprintf( lgcfg[i].szRemark, "remarques pour lg #%d", i);
        lgcfg[i].cTDMFDeviceName = 'A' + i;
        lgcfg[i].sTDMFDeviceId   = i;
        lgcfg[i].cDataDisk   = 'G'+ i;
        sprintf(lgcfg[i].szDataDisk1, "%d", 111100 + 10 + i );
        sprintf(lgcfg[i].szDataDisk2, "%d", 111100 + 20 + i );
        sprintf(lgcfg[i].szDataDisk3, "%d", 111100 + 30 + i );
        sprintf(lgcfg[i].szMirrorDisk1, "%d", 222200 + 10 + i );
        sprintf(lgcfg[i].szMirrorDisk2, "%d", 222200 + 20 + i );
        sprintf(lgcfg[i].szMirrorDisk3, "%d", 222200 + 30 + i );
        lgcfg[i].cMirrorDisk = 'P' + i;
    }


    //first client sends a broadcast request
    MMP_HANDLE  handle = mmp_Create(    argv[2] , //tdmf server ip
                                        atoi(argv[3]) //tdmf server port
                                        );

    printf("client calls mmp_setTdmfAgentConfigFile() for %s ...\n",szAgentId);

    int r =     mmp_setTdmfAgentConfigFile( handle, 
                                            szAgentId,
                                            &syscfg,
                                            lgcfg,
                                            5 );

    printf("mmp_setTdmfAgentConfigFile() returns %d ...\n",r);
    */


    printf("\n hit a key");getch();

}


void debug_simul_client_setLGcfg(int argc, char* argv[], CollectorConfig *cfg )
{
    sock_startup();

    char szRegKey[80];
    char szAgentId[80];
    int lgid;
    char cPriSec;
    //unsigned int uiDataSize;
    szRegKey[0] = 0;

    printf("\nSet configuration file cmd.");
    printf("\nEnter TDMF Agent Id (machine name) :");scanf("%s",szAgentId);
    printf("\nEnter logical group nbr :");scanf("%d",&lgid);
    //printf("\nEnter primary (p) or secondary (s) :");scanf("%c",&cPriSec);
    cPriSec = 'p';

    //first client sends a broadcast request
    MMP_HANDLE  handle = mmp_Create(    argv[2] , //tdmf server ip
                                        atoi(argv[3]) //tdmf server port
                                        );

    char fname[32];
    sprintf(fname,"%c%03d.cfg",cPriSec,lgid);
    printf("client calls mmp_getTdmfAgentLGConfig() for %s , cfg file=%s ...\n",szAgentId,fname);

    int r;
    FILE *file;
    long filesize;
    unsigned int uiFileSize=0;
    char *pCfgData;

    file = fopen(fname,"rb");
    if (file)
    {
        r = fseek( file, 0, SEEK_END );_ASSERT(r==0);
        filesize = ftell( file );

        pCfgData = (char*)malloc(filesize) ;
        uiFileSize = (unsigned int)filesize;

        r = fseek( file, 0, SEEK_SET );_ASSERT(r==0);
        if ( fread(pCfgData,sizeof(char),filesize,file) == (size_t)filesize )
        {
            r = 0;
        }
        else 
        {
            printf("***Error, while reading file %s .\n",fname);
            exit(0);
        }
        fclose(file);
    }
    else
    {
        printf("***Error, file %s not found.\n",fname);
        exit(0);
    }

    r = mmp_setTdmfAgentLGConfig(handle, 
                                 szAgentId,
                                 (short)lgid,
                                 cPriSec == 'p' || cPriSec == 'P' ,
                                 pCfgData,
                                 uiFileSize
                                 );
    printf("mmp_getTdmfAgentLGConfig() returns %d ...\n",r);

    printf("\n hit a key");getch();

}

void debug_simul_client_sendtdmfcmd(int argc, char* argv[], CollectorConfig *cfg )
{
    sock_startup();

    enum tdmf_commands cmd;
    char cmdParam[80],*szMsg;
    cmdParam[0] = 0;
    if ( strstr(argv[4],"tdmfinfo") != NULL )
    {
        cmd = MMP_MNGT_TDMF_CMD_INFO;
        for(int i=5; i<argc; i++ )
        {
            strcat( cmdParam, argv[i] );
            strcat( cmdParam, " " );
        }
    }
    else if ( strstr(argv[4],"tdmfhostinfo") != NULL )
    {
        cmd = MMP_MNGT_TDMF_CMD_HOSTINFO;
    }

    MMP_HANDLE  handle = mmp_Create(    argv[2] , //tdmf server ip
                                        atoi(argv[3]) //tdmf server port
                                        );

    printf("client calls mmp_mngt_sendTdmfCommand() for a7a7a7a7 ...\n");
    int r = mmp_mngt_sendTdmfCommand( handle, 
                              /*in*/  "a7a7a7a7",
                              /*in*/  cmd,
                              /*in*/  cmdParam,
                              /*out*/ &szMsg);

    printf("mmp_mngt_sendTdmfCommand() returns %d ...\n",r);


     printf("\n hit a key");getch();


}

void debug_simul_client_getalldevices(int argc, char* argv[], CollectorConfig *cfg )
{
    sock_startup();

    char szRegKey[80];
    //char szAgentId[80];
    szRegKey[0] = 0;

    MMP_HANDLE  handle = mmp_Create(    argv[2] , //tdmf server ip
                                        atoi(argv[3]) //tdmf server port
                                        );

    mmp_TdmfDeviceInfo  *pTdmfDeviceInfoVector,*pWk;
    unsigned int        uiNbrDeviceInfoInVector;

    printf("client calls mmp_getTdmfAgentAllDevices() for a7a7a7a7 ...\n");
    int r = mmp_getTdmfAgentAllDevices( /*in*/ handle, 
                                        /*in*/ "a7a7a7a7",
                                        &pTdmfDeviceInfoVector,
                                        &uiNbrDeviceInfoInVector
                                        );
     printf("mmp_mngt_sendTdmfCommand() returns %d , uiNbrDeviceInfoInVector=%d...\n",r,uiNbrDeviceInfoInVector);
     if ( r == 0 )
     {
         pWk = pTdmfDeviceInfoVector;
         for(int i=0;i<uiNbrDeviceInfoInVector;i++,pWk++)
         {
             printf("device %2d : %s  :  %15s  %15s  %15s \n", 
                 i,
                 pWk->szDriveId, 
                 pWk->szSize,
                 pWk->szStartOffset,
                 pWk->szLength );
         }
     }

     printf("\n hit a key");getch();
}



void debug_simul_client_getAgentCfg(int argc, char* argv[], CollectorConfig *cfg )
{
    sock_startup();

    char szRegKey[80];
    //char szAgentId[80];
    szRegKey[0] = 0;

    MMP_HANDLE  handle = mmp_Create(    argv[2] , //tdmf server ip
                                        atoi(argv[3]) //tdmf server port
                                        );

    mmp_TdmfServerInfo agentCfg;

    printf("client calls mmp_mngt_getTdmfAgentConfig() for a7a7a7a7 ...\n");

    int r = mmp_mngt_getTdmfAgentConfig(handle, "a7a7a7a7", &agentCfg );

    printf("mmp_mngt_getTdmfAgentConfig() returns %d \n",r);
    if ( r == 0 )
    {
         printf("Agent configuration: BABSize=%d MB , Port=%d , TCPWindowSize=%d KB\n", 
             agentCfg.iBABSize,agentCfg.iPort,agentCfg.iTCPWindowSize );
         printf("                     CPU=%d , RAM=%d , FileSys=%s, OSType=%s , OSVer=%s , TdmfVer=%s\n", 
             agentCfg.iNbrCPU, agentCfg.iRAMSize, agentCfg.szFileSystem, 
             agentCfg.szOsType, agentCfg.szOsVersion, agentCfg.szTdmfVersion );

        /* for tests only
        agentCfg.iBABSize++;
        agentCfg.iPort++;
        agentCfg.iTCPWindowSize++;*/
    
        int r = mmp_mngt_setTdmfAgentConfig(handle, "a7a7a7a7", &agentCfg );
    }


    printf("\n hit a key");getch();
}

void    debug_simul_client_SetPerfCfg(int argc, char* argv[], CollectorConfig *cfg )
{
    sock_startup();

    MMP_HANDLE  handle = mmp_Create(    argv[2] , //tdmf server ip
                                        atoi(argv[3]) //tdmf server port
                                        );

    printf("client calls mmp_setTdmfPerfConfig() ...\n");

    mmp_TdmfPerfConfig perfCfg;
    perfCfg.iPerfUploadPeriod = 500;//5 seconds

    int r = mmp_setTdmfPerfConfig(handle, &perfCfg);

    printf("mmp_setTdmfPerfConfig() returns %d \n",r);

    printf("\n hit a key");getch();
}

#endif  //DEBUG