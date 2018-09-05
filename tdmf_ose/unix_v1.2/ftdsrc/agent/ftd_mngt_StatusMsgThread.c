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
/* #ident "@(#)$Id: ftd_mngt_StatusMsgThread.c,v 1.11 2010/12/20 20:12:25 dkodjo Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
															
#include "ps_intr.h"
#include "common.h"

#ifndef _FTD_MNGT_STATUSMSGTHREAD_C_
#define _FTD_MNGT_STATUSMSGTHREAD_C_

/*
 * These two strings MUST MATCH the Status messages produced by the PMD
 * of the checkpoint on/off state.
 * CPON/CPOFF messages are defined in errors.MSG.
 */
static const char lg_num_str[]="PMD_000"; /* make sure this matches the PMD number string below in both messages */
static const char CPON_STR[] ="CPON]: " GROUPNAME " group [PMD_000] is in checkpoint mode";
static const char CPOFF_STR[]="CPOFF]: " GROUPNAME " group [PMD_000] successfully transitioned out of checkpoint mode";

extern void ftd_mngt_performance_send_all_to_Collector();
extern void program_exit();

int Wrtcounter;
static int initstat = 0;
static char configpaths[MAXLG][32];
extern char *path;

static void
ftd_mngt_alive_socket()
{
    int r = 0;
    unsigned int dum_ip;
    mmp_mngt_TdmfAgentAliveMsg_t msg;
    struct hostent *host;
    struct sockaddr_in server;
	int i=0;

#if !defined(FTD_IPV4)
	struct sockaddr_in6 server6;
#endif

	int s,len;
    static int ilSockID = 0;
    char  szAliveMsgTagValue[32];
    char  tmp[48];
	int level;

	ipAddress_t tmp_verify;
	ipAddress_t empty_struct;


    logout(17,F_ftd_mngt_alive_socket,"start.\n");
    if (ilSockID == 0)
    {
        memset(tmp, 0x00, sizeof(tmp));
        cfg_get_software_key_value(AGENT_CFG_IP, tmp, sizeof(tmp));
        ipstring_to_ip(tmp, &tmp_verify);

        /********** IPV4 or IPV6 ******************/
        if(tmp_verify.Version == IPVER_4) {      
      	  bzero((char *)&server,sizeof(server));			  
      	  server.sin_family = AF_INET;


      	  server.sin_port = htons(giTDMFCollectorPort);
      	  server.sin_addr.s_addr = inet_addr(tmp);//tmp_verify.Addr.V4;//inet_addr(tmp);

    	  ilSockID = socket(AF_INET, SOCK_STREAM, 0);
    		    if (ilSockID < 0) {
     		   	     logoutx(4, F_ftd_mngt_alive_socket, "socket error", "errno", errno);
	 	 	   ilSockID = 0;
     	  	      return;
      		  	}
      		  	s = connect_st(ilSockID,(struct sockaddr *) &server, sizeof(server), &level);
      	}																		
		else { /*************************IPV6******************/

#if !defined(FTD_IPV4)

     	  
     	   bzero((char *)&server6,sizeof(server6));
     	   server6.sin6_family = AF_INET6;																 
				
    	   server6.sin6_port = htons(giTDMFCollectorPort);
					
       	 inet_pton(AF_INET6,tmp,&server6.sin6_addr.s6_addr);
		 logoutx(4, F_ftd_mngt_alive_socket, "after inet_pton", "errno", 4);
    	    ilSockID = socket(AF_INET6, SOCK_STREAM, 0);
    	    if (ilSockID < 0) {
        	    logoutx(4, F_ftd_mngt_alive_socket, "socket error", "errno", errno);
	  	   		ilSockID = 0;
       	      return;
        	}
		    ip_to_ipstring(tmp_verify,tmp);

    	    s = connect_st(ilSockID,(struct sockaddr *) &server6, sizeof(server6), &level);       
#endif        															  
        } /*************************End of IPV6******************/											

        
        						  
        if (s < 0) {
             logoutx(level, F_ftd_mngt_alive_socket, "connect error", "errno", s);
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
        sprintf(szAliveMsgTagValue,"%s%d ","MMPVER=",MMP_PROTOCOL_VERSION);  /* Was hard coded to 3 (pc070817) */
        len                     = strlen(szAliveMsgTagValue) + 1;
				/* include '\0' */
        msg.iMsgLength          = htonl(len);

        /*send status message to requester */
        r = write(ilSockID, (char*)&msg, sizeof(msg));
        if ( r == sizeof(msg) )
        {   /* send variable data portion of message */
    	    Wrtcounter = 1;
            write(ilSockID, szAliveMsgTagValue, len); 
		    ftd_mngt_send_agentinfo_msg(empty_struct,0); 	   
		    ftd_mngt_performance_send_all_to_Collector();
		    ftd_mngt_send_registration_key();

        }
		else {
			logoutx(level, F_ftd_mngt_alive_socket, "SEND VARIABLE DATA", "errno", errno);
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
			logoutx(4,F_ftd_mngt_alive_socket,"packet write error","sizeof(msg) =  ",sizeof(msg));		
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

    int             i,r,towrite,len,cnt;
    char            *pData;
    char            stddata[BUF_SIZE];
    FILE            *eventfd;
    sock_t          *aliveSocket;
    struct stat     shstat;
    char            *pMsgString;
    time_t          tmt;
    off_t           point;  /* lseek point */
    int             cpfifo,cpfifoidx=0;
    char            group_name[MAXPATHLEN];
    char            ps_name[MAXPATHLEN];
    char            *lg_num_loc;
    int             cpon_lg_num_offset;
    int             cpoff_lg_num_offset;
    ps_group_info_t group_info;
    mmp_mngt_TdmfStatusMsgMsg_t *pMsg;

    lg_num_loc = strstr(CPON_STR, lg_num_str);
    if (lg_num_loc == NULL || lg_num_loc == CPON_STR)
    {
		/* coding error */
		logoutx(3, F_ftd_mngt_StatusMsgThread, "Cannot locate " GROUPNAME " group number in the CPON message.", NULL, 0);
		program_exit();
    }
    cpon_lg_num_offset = lg_num_loc - CPON_STR;

    lg_num_loc = strstr(CPOFF_STR, lg_num_str);
    if (lg_num_loc == NULL || lg_num_loc == CPOFF_STR)
    {
		/* coding error */
		logoutx(3, F_ftd_mngt_StatusMsgThread, "Cannot locate " GROUPNAME " group number in the CPOFF message.", NULL, 0);
		program_exit();
    }
    cpoff_lg_num_offset = lg_num_loc - CPOFF_STR;

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

		/* send stat when thread started */
		if (initstat == 0) 
		{
			int  lgcnt;
			int  group;
			int  rc;

			initstat = 1;
			logoutx(11, F_ftd_mngt_StatusMsgThread, "FIRST TIME", NULL, 0);

			/* get valid logic groups */
			lgcnt = GETCONFIGS(configpaths, 1, 1);
			if (lgcnt != 0)
			{
				sprintf(logmsg, "lgcnt = %d\n", lgcnt);
				logout(11,F_ftd_mngt_StatusMsgThread, logmsg);

				for (i = 0; i < lgcnt; i++)
				{
					group = cfgpathtonum(configpaths[i]);

					/* create the group name */
    				FTD_CREATE_GROUP_NAME(group_name, group);

    				if (GETPSNAME(group, ps_name) != 0)
					{
						logoutx(4, F_ftd_mngt_StatusMsgThread,
								"GETPSNAME error\n", NULL, 0);
						continue;
					}

					sprintf(logmsg, "group = %s, ps = %s\n",
							group_name, ps_name);
					logout(11,F_ftd_mngt_StatusMsgThread, logmsg);

					/* get group state */
					group_info.name = NULL;
    				if ((rc = ps_get_group_info(ps_name, group_name, &group_info)) != PS_OK)
					{
						logoutx(4, F_ftd_mngt_StatusMsgThread,
								"ps_get_group_info error\n", NULL, 0);
                        continue;
					}

					if (group_info.checkpoint == 1)
					{
						cp_fifo_t c;
						c.idx    = ++cpfifoidx;
						c.lgnum  = group;
						c.on_off = 1;
						if (cpfifo != -1)
						{
							int r;
							r = write(cpfifo, &c, sizeof(c));
							sprintf(logmsg,
									"write cpon to cpfifo, idx=%d lgnum=%d\n",
									c.idx, c.lgnum);
							logout(11,F_ftd_mngt_StatusMsgThread, logmsg);

							if (r != sizeof(c))
							{
								logoutx(4, F_ftd_mngt_StatusMsgThread,
										"write error", "errno", errno);
								/* continueing next lg */
							}
						}
					}
				}														 
			}						  
		}

		/* check file upfdate */
		/* check command stdout file */
		memset(&shstat, 0x00, sizeof(shstat));
		stat(EVENT_FILE,&shstat);
		if (shstat.st_size != point)
		{
			sprintf(logmsg, "event file %s update.\n", EVENT_FILE);
			logout(17, F_ftd_mngt_StatusMsgThread, logmsg);
			/* stdout data read */
			eventfd = fopen(EVENT_FILE,"r");
			if(eventfd == NULL)
			{
				logoutx(3, F_ftd_mngt_StatusMsgThread, "fopen failed.", "errno", errno);
				goto cont;
			}
			if (point != 0 && shstat.st_size > point)
			{
				fseek (eventfd, (long)point, SEEK_SET);
			}
			/* clear read buf */
			memset(stddata, 0x00, BUF_SIZE);
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

                    /*																	
                     * this string parsing is hard coded for CPON_STR, make
                     * sure you change this code once CPON_STR is changed.
                     */
					if (pMsgString[ioffset+cpon_lg_num_offset] == 'P' && !memcmp(&pMsgString[ioffset],CPON_STR,4) )
					{
						strncpy(ccpnbr,&pMsgString[ioffset+cpon_lg_num_offset+4],3);
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
								sprintf(logmsg,
									"write cpon to cpfifo, idx=%d lgnum=%d\n",
									c.idx, c.lgnum);
								logout(11, F_ftd_mngt_StatusMsgThread, logmsg);
							}
						}
					}
                    /*
                     * this string parsing is hard coded for CPOFF_STR, make
                     * sure you change this code once CPOFF_STR is changed.
                     */
					else if (pMsgString[ioffset2+cpoff_lg_num_offset] == 'P' && !memcmp(&pMsgString[ioffset2],CPOFF_STR,5))
					{
						strncpy(ccpnbr,&pMsgString[ioffset2+cpoff_lg_num_offset+4],3);
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
								sprintf(logmsg,
									"write cpoff to cpfifo, idx=%d lgnum=%d\n",
									c.idx, c.lgnum);
								logout(11, F_ftd_mngt_StatusMsgThread, logmsg);
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
				while(sock_sendtox(giTDMFCollectorIP,giTDMFCollectorPort,(char*)pMsg,towrite,0,0) < 0) {
					/* write packet error */
					if(cnt++ >= RETRY_COUNT) {
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
		if (Wrtcounter >= 2) Wrtcounter = 0;			   
	}
	close(cpfifo);
	free(pData);
}
#endif	/* _FTD_MNGT_STATUSMSGTHREAD_C_ */
