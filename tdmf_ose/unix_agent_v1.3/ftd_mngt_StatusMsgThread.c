/* #ident "@(#)$Id: ftd_mngt_StatusMsgThread.c,v 1.20 2003/11/13 11:05:51 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_STATUSMSGTHREAD_C_
#define _FTD_MNGT_STATUSMSGTHREAD_C_

/*
 * these two string MUST MATCH the Status messages produced by the PMD
 *  is the checkpoint on/off state.
 */
static const char CPON_STR[] ="CPON]: logical group [PMD_000] is in checkpoint mode";
static const char CPOFF_STR[]="CPOFF]: logical group [PMD_000] successfully transitioned out of checkpoint mode";

extern void ftd_mngt_performance_send_all_to_Collector();

int Wrtcounter;

static void
ftd_mngt_alive_socket()
{
    int r = 0;
    unsigned long dum_ip;
    mmp_mngt_TdmfAgentAliveMsg_t msg;
    struct hostent *host;
    struct sockaddr_in server;
    int s,len;
    static int ilSockID = 0;
    char  szAliveMsgTagValue[32];
    char  tmp[32];
	int level;

    logout(17,F_ftd_mngt_alive_socket,"start.\n");
    if (ilSockID == 0)
    {
        memset(tmp, 0x00, sizeof(tmp));
        cfg_get_software_key_value("CollectorIP", tmp, sizeof(tmp));
        bzero((char *)&server,sizeof(server));
        server.sin_family = AF_INET;

#if 1
        server.sin_port = htons(giTDMFCollectorPort);
        server.sin_addr.s_addr = inet_addr(tmp);
#else
        server.sin_port = htons(PORT);
        server.sin_addr.s_addr = inet_addr(IP_ADDRESS);
#endif
        sprintf(logmsg,"sin_port = %d, sin_addr = %d\n", server.sin_port, server.sin_addr.s_addr);
        logout(17,F_ftd_mngt_alive_socket,logmsg);

        ilSockID = socket(AF_INET, SOCK_STREAM, 0);
        if (ilSockID < 0) {
             logoutx(4, F_ftd_mngt_alive_socket, "socket error", "errno", errno);
	     ilSockID = 0;
             return;
        }
        s = connect_st(ilSockID,(struct sockaddr *) &server, sizeof(server), &level);
        if (s < 0) {
             logoutx(level, F_ftd_mngt_alive_socket, "connect error", "errno", errno);
		     close(ilSockID);
		     ilSockID = 0;
             return;
        } else if( s == 1){
             logout(level, F_ftd_mngt_alive_socket, "recovery from connect error.\n");
		}

        /*send the MMP_MNGT_AGENT_ALIVE_SOCKET msg */
        msg.hdr.magicnumber     = MNGT_MSG_MAGICNUMBER;
        msg.hdr.mngttype        = MMP_MNGT_AGENT_ALIVE_SOCKET;
        msg.hdr.mngtstatus      = MMP_MNGT_STATUS_OK;
        msg.hdr.sendertype      = SENDERTYPE_TDMF_AGENT;
        mmp_convert_mngt_hdr_hton(&msg.hdr);
	strcpy(msg.szServerUID, gszServerUID);
        /* >>> make sure szAliveMsgTagValue can eat it all up ... */
        sprintf(szAliveMsgTagValue,"%s%d ","MMPVER=",3);
        len                     = strlen(szAliveMsgTagValue) + 1;
				/* include '\0' */
        msg.iMsgLength          = htonl(len);

        /*send status message to requester */
        r = write(ilSockID, (char*)&msg, sizeof(msg));
        if ( r == sizeof(msg) )
        {   /* send variable data portion of message */
    	    Wrtcounter = 1;
            write(ilSockID, szAliveMsgTagValue, len); 
		    ftd_mngt_send_agentinfo_msg(0,0); 
		    ftd_mngt_performance_send_all_to_Collector();
		    ftd_mngt_send_registration_key();
        }
        logout(17,F_ftd_mngt_alive_socket,"send first ALIVE\n");
    }
    else 
    {
        /*send the MMP_MNGT_AGENT_ALIVE_SOCKET msg */
        msg.hdr.magicnumber     = MNGT_MSG_MAGICNUMBER;
        msg.hdr.mngttype        = MMP_MNGT_AGENT_ALIVE_SOCKET;
        msg.hdr.mngtstatus      = MMP_MNGT_STATUS_OK;
        msg.hdr.sendertype      = SENDERTYPE_TDMF_AGENT;
        mmp_convert_mngt_hdr_hton(&msg.hdr);
	strcpy(msg.szServerUID, gszServerUID);
        msg.iMsgLength          = htonl(0);

        /* send status message to requester */
        r = write(ilSockID, (char*)&msg, sizeof(msg));
	if (r != sizeof(msg))
	{
		/* ALIVE write error */
		logoutx(4,F_ftd_mngt_alive_socket,"packet write error","errno",errno);
		close(ilSockID);
		ilSockID = 0;
	}
	else 
	{
        	logout(17,F_ftd_mngt_alive_socket,"send ALIVE\n");
        	Wrtcounter = 1;
	}
    }
}

#define BUF_SIZE    65536

void ftd_mngt_StatusMsgThread()
{

    int i,r,towrite,len,cnt;
    mmp_mngt_TdmfStatusMsgMsg_t *pMsg;
    char *pData;
    char stddata[BUF_SIZE];
    FILE *eventfd;
    sock_t  *aliveSocket;
    struct stat  shstat;
    char *pMsgString;
    time_t tmt;
    off_t point;            /* lseek point */
    int cpfifo,cpfifoidx=0;

    Wrtcounter = 0;
    eventfd = 0;

    pData = (char *)malloc(sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + BUF_SIZE);
    if (pData == NULL) 
    {
	/* malloc error */
	logoutx(3, F_ftd_mngt_StatusMsgThread, "packet data area allocate error", "errno", errno);
	program_exit();
    }
    memset(pData, 0, sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + BUF_SIZE);
    pMsg = (mmp_mngt_TdmfStatusMsgMsg_t *)pData;

    pMsg->hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    pMsg->hdr.mngttype       = MMP_MNGT_STATUS_MSG;
    pMsg->hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    pMsg->hdr.mngtstatus     = MMP_MNGT_STATUS_OK;
    mmp_convert_mngt_hdr_hton(&pMsg->hdr);
    strcpy(pMsg->data.szServerUID, gszServerUID);

    memset(&shstat, 0x00, sizeof(shstat));
    stat(EVENT_FILE,&shstat);
    point = shstat.st_size;

    if (mkfifo(CHECKPOINT_FIFO,0) == -1)
    {
	logoutx(3, F_ftd_mngt_StatusMsgThread, "mkfifo error", "errno", errno);
    free(pData);
	return;
    }
    cpfifo = open(CHECKPOINT_FIFO, O_NONBLOCK|O_RDWR|O_TRUNC|O_APPEND|O_CREAT,S_IRUSR|S_IWUSR|S_IXUSR);
    if ( cpfifo == -1 )
    {
	logoutx(3, F_ftd_mngt_StatusMsgThread, "open error", "errno", errno);
    free(pData);
	return;
    }

    for(;;)
    {
		/* here, we work with the Alive Socket */
		if (Wrtcounter == 0) 
		{
			ftd_mngt_alive_socket();
		}
		/* check file upfdate */
		/* check command stdout file */
		memset(&shstat, 0x00, sizeof(shstat));
		stat(EVENT_FILE,&shstat);
		if (shstat.st_size != point)
		{
			logout(17, F_ftd_mngt_StatusMsgThread, "event file update.\n");
			/* stdout data read */
			eventfd = fopen(EVENT_FILE,"r");
			if(eventfd == NULL)
			{
				logoutx(3, F_ftd_mngt_StatusMsgThread, "fopen failed.", "errno", errno);
				goto cont;
			}
			if (point != 0 && shstat.st_size > point) fseek (eventfd, (long)point, SEEK_SET);
			/* clear read buf */
			memset(stddata,0x00,BUF_SIZE);
			while (fgets (stddata, BUF_SIZE, eventfd) != NULL)
			{
				sprintf(logmsg,"stddata = %s", stddata);
				logout(17,F_ftd_mngt_StatusMsgThread,logmsg);
				cnvmsg(stddata,BUF_SIZE);
				len = strlen(stddata);
				pMsg->data.iLength = len;

				pMsgString            = (char*)(pMsg+1);
				pMsg->data.cPriority  = 4;
				time(&tmt);
				pMsg->data.iTimeStamp = tmt;
				strcpy( pMsgString, stddata);

				if ( len >= sizeof(CPON_STR) )
				{
					int  icpnbr, ioffset, ioffset2;
					char ccpnbr[4];
					/* check msg from the end of it */

					/* ioffset = position at 'CPON]' */
					ioffset =len-sizeof(CPON_STR);
					/* ioffset2 = position at 'CPOFF]' */
					ioffset2 =len-sizeof(CPOFF_STR);

					if (pMsgString[ioffset+22] == 'P' && !memcmp(&pMsgString[ioffset],CPON_STR,4) )
					{
						strncpy(ccpnbr,&pMsgString[ioffset+22+4],3);
						ccpnbr[3] = 0;
						icpnbr=atoi(ccpnbr); /* grp num */
						if (icpnbr >= 0 && icpnbr < FTD_MAX_GROUPS)
						{   /* CP is ON */
							cp_fifo_t c;
							c.idx    = ++cpfifoidx;
							c.lgnum  = icpnbr;
							c.on_off = 1;
							if ( cpfifo != -1 )
							{
								int r;
								r = write(cpfifo,&c,sizeof(c));
								if ( r != sizeof(c) ){
									logoutx(4, F_ftd_mngt_StatusMsgThread, "write error", "errno", errno);
									break;
								}
							}
						}
					}
					else if (pMsgString[ioffset2+23] == 'P' && !memcmp(&pMsgString[ioffset2],CPOFF_STR,5))
					{
						strncpy(ccpnbr,&pMsgString[ioffset2+23+4],3);
						ccpnbr[3] = 0;
						icpnbr=atoi(ccpnbr); /* grp num */
						if (icpnbr >= 0 && icpnbr < FTD_MAX_GROUPS)
						{   /* CP is OFF */
							cp_fifo_t c;
							c.idx    = ++cpfifoidx;
							c.lgnum  = icpnbr;
							c.on_off = 0;
							if ( cpfifo != -1 ) 
							{
								int r = write(cpfifo,&c,sizeof(c));
								if ( r != sizeof(c) ){
									logoutx(4, F_ftd_mngt_StatusMsgThread, "write error", "errno", errno);
									break;
								}
							}
						}
					}
				}

				pMsgString[ len - 1 ] = 0;
				/* force EOS at end of string */

				/*************************************/
				/* send Status Msg to TDMF Collector */
				/*************************************/
				towrite = sizeof(mmp_mngt_TdmfStatusMsgMsg_t) + len;
				mmp_convert_TdmfStatusMsg_hton(&pMsg->data);
				cnt = 0;
				while(sock_sendtox(giTDMFCollectorIP,giTDMFCollectorPort,(char*)pMsg,towrite,0,0) < 0){
					/* write packet error */
					if(cnt++ >= RETRY_COUNT){
						Wrtcounter = 0;
						logoutx(3, F_ftd_mngt_StatusMsgThread, "write error", "errno", errno);
						break;
					}
					usleep(10000);
				}
				/* clear read buf */
				memset(stddata,0x00,BUF_SIZE);
				if(cnt >= RETRY_COUNT + 1) break;
				point = ftell(eventfd);
			}
			logout(17, F_ftd_mngt_StatusMsgThread, "file send loop end.\n");
			fclose(eventfd);
			eventfd = 0;
		}
cont:
		sleep(4);
		Wrtcounter++;
		if (Wrtcounter >= 2) Wrtcounter =0;
	}
	close(cpfifo);
    free(pData);
}
#endif	/* _FTD_MNGT_STATUSMSGTHREAD_C_ */
