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
/* #ident "@(#)$Id: ftd_mngt_tdmf_cmd.c,v 1.8 2014/09/15 20:25:44 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_TDMF_CMD_C_
#define _FTD_MNGT_TDMF_CMD_C_

extern void    ftd_mngt_performance_reduce_stat_upload_period(int, int);
void    send_event(char *,int);

extern  char    gszServerUID[];

int ftd_mngt_tdmf_cmd(sock_t *sockID)
{
    pid_t	pid;
    int		fd [2];		              /* File descriptor for stdout */
    int		status = -1;
    char 	linebuf[1025];	              /* Line buffer */

    int      r,exitcode = -1;                 /* assume error */
    mmp_mngt_TdmfCommandMsg_t       cmdmsg;
    mmp_mngt_TdmfCommandStatusMsg_t response;
    char tmpdata[10+1];
    char   szShortApplicationName[1024+1];    /* name of executable module */
    char   szApplicationName[1024+1];         /* name of executable module */
    char   szCurrentDirectory[1024+1];        /*  current directory name   */
    char   *pszCurDir = szCurrentDirectory;
    char   *pCmdLine;
    int    dwCmdCompleteWaitTime;             /* in milliseconds */
    int    iCmdLineLen;
    int    iOutputMsgLen;
    int    towrite = 0;
    FILE   *cmdout;
    struct stat  shstat;
    char *outdata = NULL;
    int  filelen;
    char                            *pWk;
    char   msgpath[1024+1];        		/*  stdout file path	*/

    logout(17,F_ftd_mngt_tdmf_cmd,"start.\n");

    pWk  = (char*)&cmdmsg;
    pWk += sizeof(mmp_mngt_header_t);

    /* clear data area */
    memset(&cmdmsg,0x00,sizeof(cmdmsg));
    memset(&response,0x00,sizeof(response));
    /*************************************/
    /* at this point, mmp_mngt_header_t header is read.
     * now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure
     */
    r = ftd_sock_recv(sockID,(char*)pWk,
		sizeof(mmp_mngt_TdmfCommandMsg_t)-sizeof(mmp_mngt_header_t));
    if ( r != sizeof(mmp_mngt_TdmfCommandMsg_t)-sizeof(mmp_mngt_header_t) )
    {
        logout(4,F_ftd_mngt_tdmf_cmd,"data recv error.\n");
        return -1;
    }
    /* convert multi-byte values from network bytes order to host byte order */
    cmdmsg.iSubCmd = ntohl(cmdmsg.iSubCmd);

    /*****************************************/
    /* analyze mmp_mngt_TdmfCommandMsg_t content */
    /*  */
    /* validate */
    /*  */
    if ( cmdmsg.iSubCmd < FIRST_TDMF_CMD || cmdmsg.iSubCmd > LAST_TDMF_CMD )
    {
	sprintf(logmsg,"invalid command received(0x%x).\n",cmdmsg.iSubCmd);
	logout(4,F_ftd_mngt_tdmf_cmd,logmsg);
        return -1;
    }

    /***************************************/
    /* retreive values used to execute command
     */
    sprintf(szCurrentDirectory,"%s", DTCCMDDIR);

    sprintf(szApplicationName, "%s/", szCurrentDirectory);
    strcpy(szShortApplicationName, QNM);
    switch( cmdmsg.iSubCmd ) {
    case MMP_MNGT_TDMF_CMD_START:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = start\n");
        strcat( szShortApplicationName, "start" );
        ftd_mngt_performance_reduce_stat_upload_period(10, 0);
	     /* for next 10 seconds, sends stats to Collector at each second */
        break;
    case MMP_MNGT_TDMF_CMD_STOP:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = stop\n");
        strcat( szShortApplicationName, "stop" );
        ftd_mngt_performance_reduce_stat_upload_period(10, 0);
	     /* for next 10 seconds, sends stats to Collector at each second */
        break;
    case MMP_MNGT_TDMF_CMD_INIT:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = init\n");
        strcat( szShortApplicationName, "init" );
        break;
    case MMP_MNGT_TDMF_CMD_OVERRIDE:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = override\n");
        strcat( szShortApplicationName, "override" );
        break;
    case MMP_MNGT_TDMF_CMD_INFO:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = info\n");
        strcat( szShortApplicationName, "info" );
        break;
    case MMP_MNGT_TDMF_CMD_HOSTINFO:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = hostinfo\n");
        strcat( szShortApplicationName, "hostinfo" );
        break;
    case MMP_MNGT_TDMF_CMD_LICINFO:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = licinfo\n");
        strcat( szShortApplicationName, "licinfo" );
        break;
    case MMP_MNGT_TDMF_CMD_RECO:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = reco\n");
        strcat( szShortApplicationName, "rmdreco" );
        break;
    case MMP_MNGT_TDMF_CMD_SET:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = set\n");
        strcat( szShortApplicationName, "set" );
        break;
    case MMP_MNGT_TDMF_CMD_LAUNCH_PMD:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = launchpmd\n");
        sprintf( szShortApplicationName, "launchpmds" );
        ftd_mngt_performance_reduce_stat_upload_period(10, 0);
	     /* for next 10 seconds, sends stats to Collector at each second */
        break;
    case MMP_MNGT_TDMF_CMD_LAUNCH_REFRESH:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = launchrefresh\n");
        sprintf( szShortApplicationName, "launchrefresh");
        ftd_mngt_performance_reduce_stat_upload_period(10, 0);
	     /* for next 10 seconds, sends stats to Collector at each second */
        break;
    case MMP_MNGT_TDMF_CMD_LAUNCH_BACKFRESH:
#ifdef SUPPORT_BACKFRESH
	logout(9,F_ftd_mngt_tdmf_cmd,"command = launchbackfresh\n");
        sprintf( szShortApplicationName, "launchbackfresh");
        ftd_mngt_performance_reduce_stat_upload_period(10, 0);
	     /* for next 10 seconds, sends stats to Collector at each second */
        break;
#else
		logout(9,F_ftd_mngt_tdmf_cmd,"command = launchbackfresh\n");
		sprintf(logmsg,"launchbackfresh is no longer supported; command rejected.");
		logout(4,F_ftd_mngt_tdmf_cmd,logmsg);
        return -2;
        break;
#endif
    case MMP_MNGT_TDMF_CMD_KILL_PMD:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = killpmd\n");
        sprintf( szShortApplicationName, "killpmds" );
        ftd_mngt_performance_reduce_stat_upload_period(10, 0);
	     /* for next 10 seconds, sends stats to Collector at each second */
        dwCmdCompleteWaitTime = 5000;
        break;
    case MMP_MNGT_TDMF_CMD_KILL_RMD:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = killrmd\n");
        sprintf( szShortApplicationName, "killrmds" );
        ftd_mngt_performance_reduce_stat_upload_period(10, 0);
	     /* for next 10 seconds, sends stats to Collector at each second */
        dwCmdCompleteWaitTime = 5000;
        break;
    case MMP_MNGT_TDMF_CMD_KILL_REFRESH:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = killrefresh\n");
        strcat( szShortApplicationName, "killrefresh" );
        ftd_mngt_performance_reduce_stat_upload_period(10, 0);
	     /* for next 10 seconds, sends stats to Collector at each second */
        dwCmdCompleteWaitTime = 5000;
        break;
    case MMP_MNGT_TDMF_CMD_KILL_BACKFRESH:
#ifdef SUPPORT_BACKFRESH
	logout(9,F_ftd_mngt_tdmf_cmd,"command = killbackfresh\n");
        strcat( szShortApplicationName, "killbackfresh" );
        ftd_mngt_performance_reduce_stat_upload_period(10, 0);
	     /* for next 10 seconds, sends stats to Collector at each second */
        dwCmdCompleteWaitTime = 5000;
        break;
#else
		logout(9,F_ftd_mngt_tdmf_cmd,"command = killbackfresh\n");
		sprintf(logmsg,"launchbackfresh and killbackfresh no longer supported; command rejected.");
		logout(4,F_ftd_mngt_tdmf_cmd,logmsg);
        return -2;
        break;
#endif
    case MMP_MNGT_TDMF_CMD_CHECKPOINT:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = checkpoint\n");
        strcat( szShortApplicationName, "checkpoint" );
        ftd_mngt_performance_reduce_stat_upload_period(30, 0);
	     /* for next 10 seconds, sends stats to Collector at each second */
        break;
/*
    case MMP_MNGT_TDMF_CMD_TRACE:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = trace\n");
        strcat( szShortApplicationName, "trace" );
        break;
 */
    case MMP_MNGT_TDMF_CMD_OS_CMD_EXE:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = os_command\n");
        memset(&szShortApplicationName,0x00,sizeof(szShortApplicationName));
        dwCmdCompleteWaitTime = 0;
        pszCurDir = 0; /* current directory has to be NULL with cmd.exe */
        break;
    case MMP_MNGT_TDMF_CMD_TESTTDMF:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = test\n");
        strcat( szShortApplicationName, "test" ); /* only used for test !! */
        break;
    case MMP_MNGT_TDMF_CMD_EXPAND:
        logout(9,F_ftd_mngt_tdmf_cmd,"command = expand\n");
        strcat( szShortApplicationName, "expand" );
        break;
    case MMP_MNGT_TDMF_CMD_LIMITSIZE:
        logout(9,F_ftd_mngt_tdmf_cmd,"command = limitsize\n");
        strcat( szShortApplicationName, "limitsize" );
        break;

    case MMP_MNGT_TDMF_CMD_PANALYZE:
        logout(9,F_ftd_mngt_tdmf_cmd,"command = panalyze\n");
        strcat( szShortApplicationName, "panalyze" );
        break;

    default:
	logout(9,F_ftd_mngt_tdmf_cmd,"command = default\n");
	sprintf(logmsg,"don\'t know executable file name for sub-cmd 0x%x!",cmdmsg.iSubCmd);
	logout(4,F_ftd_mngt_tdmf_cmd,logmsg);
        return -2;
    }
    if ( cmdmsg.iSubCmd == MMP_MNGT_TDMF_CMD_OS_CMD_EXE )
        szApplicationName[0] = 0;
	  /* to run cmd.exe, must provide first param NULLto CreateProcess() */
    else
        strcat( szApplicationName, szShortApplicationName );

    /* add app name to cmd line. */
    iCmdLineLen = strlen(szShortApplicationName) + strlen(cmdmsg.szCmdOptions) +1 + 1 + 8 + 64; /* 1 for a space char, +1 for \0 at the end. + 8 for ... me! */
    pCmdLine = (char*)malloc(iCmdLineLen);
    if (pCmdLine == NULL)
    {
	/* cmdline alloc error */
	logoutx(4, F_ftd_mngt_tdmf_cmd, "malloc failed", "errno", errno);
	return(-1);
    }
    /* short app name + one space */
    sprintf(pCmdLine,"%s ",szApplicationName);
    /* append options to complete cmd line */
    strcat(pCmdLine,cmdmsg.szCmdOptions);

    logoutdelta("                            ++++++++++++++++++++++++++++++++++++++++++++++\n");
    sprintf(logmsg,"                             command = %s\n",pCmdLine);
    logoutdelta(logmsg);
    logoutdelta("                            ++++++++++++++++++++++++++++++++++++++++++++++\n");
    sprintf(msgpath, AGTTMPDIR"/.%d",getpid());
    strcat(pCmdLine," > ");
    strcat(pCmdLine,msgpath);
    strcat(pCmdLine," 2>&1");

    exitcode =  system(pCmdLine);
		
#if 0
	cmdout = fopen(msgpath,"w");
	/* cmdout = fopen(DTCVAROPTDIR"/.tdmflogmsg","w"); */
	/* while (fgets (linebuf, 1025, stdin) != NULL) { */
	fgets (linebuf, 1025, stdin);
	while (fgets (linebuf, 1025, stdin) != NULL) {
	        fprintf(cmdout,linebuf);	
	}
	fclose(cmdout);
	cmdout =(FILE *) -1;
#endif

	/* output file size get */
	memset(&shstat, 0x00, sizeof(shstat));
	stat(msgpath,&shstat);
	/* stat(DTCVAROPTDIR"/.tdmflogmsg",&shstat); */
	towrite = shstat.st_size;	
	filelen = shstat.st_size;	
 	sprintf(logmsg ,"file size = %d\n",towrite);
	logout(17,F_ftd_mngt_tdmf_cmd,logmsg);
	if (towrite != 0)
	{
		/* outdata = calloc(1,towrite); */
		outdata = malloc(towrite+1);
              memset(outdata,0x00,towrite+1);

		if (outdata == NULL)
		{
		    /* outdata malloc error */
		    logoutx(4, F_ftd_mngt_tdmf_cmd, "calloc failed", "errno", errno);
		    return(-1);
		}
		cmdout = fopen(msgpath,"r");
		/* cmdout = fopen(DTCVAROPTDIR"/.tdmflogmsg","r"); */
		if (cmdout <= (FILE *)0)
		{
  		    /* stdout open error */
		    logoutx(4, F_ftd_mngt_tdmf_cmd, "fopen failed", "errno", errno);
		}
		else {
  		    fread(outdata,1,towrite,cmdout);	
		    fclose(cmdout);
		    cmdout =(FILE *) -1;
		}
	}
	unlink(msgpath);
    
    free (pCmdLine);
    pCmdLine = NULL;
    /******************************/
    /* 
     * send exit code back to caller
     */
    response.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    response.hdr.mngttype       = MMP_MNGT_TDMF_CMD_STATUS;
    response.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    response.hdr.mngtstatus     = MMP_MNGT_STATUS_OK;
    /* convert to network byte order before sending on socket */
    mmp_convert_mngt_hdr_hton(&response.hdr);

    /* for all TDMF tools, exit code : 0 = ok , else error. */
    response.iStatus            = (!exitcode) ? MMP_MNGT_TDMF_CMD_STATUS_OK : MMP_MNGT_TDMF_CMD_STATUS_ERROR; 
    response.iSubCmd            = cmdmsg.iSubCmd;
    strcpy(response.szServerUID,cmdmsg.szServerUID);
    response.iLength        = towrite;
    iOutputMsgLen           = response.iLength;
    /* convert to network byte order before sending on socket */
    response.iStatus            = htonl(response.iStatus);
    response.iSubCmd            = htonl(response.iSubCmd);
    response.iLength            = htonl(response.iLength);

    /* respond using same socket */
    towrite = sizeof(mmp_mngt_TdmfCommandStatusMsg_t) ;
    r = ftd_sock_send(sockID,(char*)&response,sizeof(response));

    /* std out data send */
    if (filelen != 0) {
	/* stdout data exist */
    	r = ftd_sock_send(sockID,(char*)outdata,filelen);
	/* close socket */
	close(sockID->sockID);
	sockID->sockID = 0;
	/* send event data */
	sleep(2);
	send_event((char*)outdata,filelen);
	free((void *)outdata);
       outdata = NULL;
    }
    return 0;
}

void send_event(char *orgdata,int orglen)
{

    mmp_mngt_TdmfStatusMsgMsg_t *pMsg;
    int iSize;
    char *pData;
    char *stddata;
    char *pMsgString;
    struct tm *tim;
    time_t t;
    char timebuf[20];
    int pid;
    int ppid;
    int i;
    int len,towrite,r;

    pid = getpid();
    ppid = getppid();

    iSize = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + 256 + orglen;
    pData = (char *)calloc(1,iSize);
    if (pData == NULL)
    {
	/* calloc error */
        sprintf(logmsg,"response data allocate error.\n");
        logout(4,F_send_event,logmsg);
	return;
	
    }
    stddata = (char *)calloc(1,orglen + 256);
    if (stddata == NULL)
    {
	/* calloc error */
        sprintf(logmsg,"message data allocate error.\n");
        logout(4,F_send_event,logmsg);
	free(pData);
	return;
	
    }
    pMsg = (mmp_mngt_TdmfStatusMsgMsg_t *)pData;

    pMsg->hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    pMsg->hdr.mngttype       = MMP_MNGT_STATUS_MSG;
    pMsg->hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    pMsg->hdr.mngtstatus     = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&pMsg->hdr);
    sprintf(pMsg->data.szServerUID,gszServerUID);

    i=0;
    while  (i <orglen ) 
    {
       if (!isprint(*(orgdata+i)))
       {
	        *(orgdata+i) = ' ';
       }
       i++;
    }


    (void)time(&t);
    tim = localtime(&t);
    strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S", tim);
    sprintf(stddata,
            "[%s] [proc: FTD_MNGT] [pid,ppid: %d, %d] [src,line: ftd_mngt_tdmf_cmd.c, 225] "CAPQ": [INFO / COMMAND]: [%s]\n",
            timebuf, pid, ppid, orgdata);
    cnvmsg(stddata,orglen + 256);/* mtldev : this function causes problems */
    len = strlen(stddata);

    pMsg->data.iLength = len;
    pMsgString            = (char*)(pMsg+1);
    pMsg->data.cPriority  = 4;
    pMsg->data.iTimeStamp = t;
    strcpy( pMsgString, stddata);
    pMsgString[ len - 1 ] = 0; /* force EOS at end of string */

    sprintf(logmsg,"%s\n",pMsgString);
    logout(9,F_send_event,logmsg);

    towrite = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + len;
    mmp_convert_TdmfStatusMsg_hton(&pMsg->data);

    r = sock_sendtox(giTDMFCollectorIP,giTDMFCollectorPort,(char*)pMsg,towrite,0,0);
    free (pData);
    pData = NULL;
    free (stddata);
    stddata = NULL;
}
#endif /* _FTD_MNGT_TDMF_CMD_C_ */
