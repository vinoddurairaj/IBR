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
/* #ident "@(#)$Id: ftd_mngt.c,v 1.9 2010/12/20 20:12:25 dkodjo Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

/*
 * ftd_mngt.c - ftd management message handlers
 *
 * Copyright (c) 2002 Softek Technology Corporation, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
/* 
 * same strategy as in DTCConfigToolDlg.cpp
 * Make sure that the Windows 2000 functions are used if they exist. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/utsname.h>
#include <sys/time.h>
#if defined(SOLARIS) || defined(_AIX)
#include <sys/sysconfig.h>
#endif
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#if defined(SOLARIS) || defined(HPUX)
#include <sys/syscall.h>
#endif
#if defined(SOLARIS)
#include <sys/sockio.h>
#include <sys/ioccom.h>
#endif
#if defined(HPUX)
#include <sys/param.h>
#endif
#if defined(_AIX)
#include <net/if.h>
#include <sys/param.h>
#endif
#if defined(linux)
#include <linux/param.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "tdmfAgent.h"
#include "tdmfAgent_trace.h"
#include "ftdio.h"
#include "common.h"

void ftd_mngt_gather_server_info(mmp_TdmfServerInfo2 *srvrInfo);
void ftd_mngt_performance_send_data();
void ftd_mngt_send_all_state();
int connect_st(int s, const struct sockaddr *name, int namelen, int *level);

extern int Listner_port;
extern int Agent_port;
extern char logmsg[];

time_t ModifyTime;

extern  char    gszServerUID[];

/* updated each time a msg is received from Collector */




ipAddress_t  			giTDMFCollectorIP;
unsigned int			giTDMFCollectorPort;
unsigned int			giTDMFAgentPort;
unsigned int			giTdmfPerfFastCheckCnt;
ipAddress_t				giTDMFAgentIP;
unsigned int			giTDMFBabSize;
mmp_TdmfPerfConfig	gTdmfPerfConfig;
TDMFAgentEmulator	gTdmfAgentEmulator;
										   
/*****************************************************************************/
/*
 * Called once, when TDMF Agent starts.
 */
void ftd_mngt_initialize()
{
    char tmp[48];
    struct servent *port;        /* Server entry structure */
    unsigned short cnvport;

    /* INIT  */
    giTDMFCollectorPort = FTD_DEF_COLLECTORPORT;
    giTdmfPerfFastCheckCnt = 0;

    memset(tmp,0x00,sizeof(tmp));
    if ((port = getservbyname(FTD_LISTNER, "udp"))) {
            cnvport = (unsigned short)ntohs(port->s_port);
            Listner_port = (int)cnvport;
        
    } else {
            cnvport = (unsigned short)FTD_DEF_LISTNERPORT;
            Listner_port = (int)cnvport;						 
    }
    
    /*
    if ((port = getservbyname(FTD_AGENT, "tcp"))) {
            cnvport = (unsigned short)ntohs(port->s_port);
            Agent_port = (int)cnvport;
    } else {

            cnvport = (unsigned short)FTD_DEF_AGENTPORT;
            Agent_port = (int)cnvport;
    } */

	memset(tmp, 0x00, sizeof(tmp));
	if ( cfg_get_software_key_value(LISTENERPORT, tmp, sizeof(tmp)) == 0 ) {
		Agent_port = atoi(tmp);
	} else {
		Agent_port = FTD_DEF_AGENTPORT;
	}

	


    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value(AGENT_CFG_IP, tmp, sizeof(tmp)) == 0 )
    {
        ipstring_to_ip(tmp,(ipAddress_t *)&giTDMFCollectorIP);
    }
    else										
    {
        ipReset(&giTDMFCollectorIP);
			/*to be init one first msg received from TDMF Collector
                              until then , no msg can be sent. */
    }

    /* If specific agnet IP is used, collector should use this IP to contact
       agent.
    */
    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value(AGENT_CFG_AGENTIP, tmp, sizeof(tmp)) == 0 )
    {
        ipstring_to_ip(tmp,(ipAddress_t *)&giTDMFAgentIP);

    }
    else
    {
        ipReset(&giTDMFAgentIP);
    }

    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value(AGENT_CFG_PORT, tmp, sizeof(tmp)) == 0 )
    {
            cnvport = (unsigned short)atoi(tmp);
            giTDMFCollectorPort = (int)cnvport;
    }
    else
    {
            cnvport = (unsigned short)FTD_DEF_COLLECTORPORT;
            giTDMFCollectorPort = (int)cnvport;
    }

    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value(AGENT_CFG_BAB, tmp, sizeof(tmp)) == 0 )
    {
        giTDMFBabSize = atoi(tmp);	/* MB order */
    }
    else
    {
        giTDMFBabSize = 0;
    }

    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value(PERFUPLOADPERIOD, tmp, sizeof(tmp)) == 0 )
    {
        gTdmfPerfConfig.iPerfUploadPeriod = atoi(tmp);
    }
    else
    {
        gTdmfPerfConfig.iPerfUploadPeriod = 100; /*default value = 10 seconds */
    }
    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value(REPLGROUPMONITUPLOADPERIOD, tmp, sizeof(tmp)) == 0 )
    {
        gTdmfPerfConfig.iReplGrpMonitPeriod = atoi(tmp);
    }
    else
    {
        gTdmfPerfConfig.iReplGrpMonitPeriod = 100; /*default value = 10 sec */
    }
    giTdmfPerfFastCheckCnt = 0;

    gTdmfAgentEmulator.bEmulatorEnabled = 0; /*disabled*/
    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value(AGENT_CFG_EMULATOR, tmp, sizeof(tmp)) == 0 )
    {
        if ( 0 == strncmp(tmp,"true",4) || 0 == strncmp(tmp,"yes",3) || 0 == strncmp(tmp,"1",1) )
        {
            gTdmfAgentEmulator.bEmulatorEnabled = ~0; /* enabled */
            memset(tmp,0x00,sizeof(tmp));
            if ( cfg_get_software_key_value(EMULATORRANGEMIN, tmp, sizeof(tmp)) == 0 )
            {
                gTdmfAgentEmulator.iAgentRangeMin = atoi(tmp);
            }
            else
            {
                gTdmfAgentEmulator.iAgentRangeMin = 1;
            }
            memset(tmp,0x00,sizeof(tmp));
            if ( cfg_get_software_key_value(EMULATORRANGEMAX, tmp, sizeof(tmp)) == 0 )
            {
                gTdmfAgentEmulator.iAgentRangeMax = atoi(tmp);
            }
            else
            {
                gTdmfAgentEmulator.iAgentRangeMax = 10;
            }
        }
    }

    return;
}

/*
 * Create socket used to received broadcasts messages issued by TDMF Collector.
 */
sock_t* ftd_mngt_create_broadcast_listener_socket()
{
    int r;
    char tmpipaddr[19+1];
	char tmpipaddr6[47+1];
    sock_t* brdcst;
    struct sockaddr_in cli_addr;
#if !defined(FTD_IPV4)
	/* IPV6 SUPPORT */ 
	struct sockaddr_in6 cli_addr6;
#endif
	ipAddress_t	temp_struct;

    int opt;
   
    memset(tmpipaddr,0x00,sizeof(tmpipaddr)); 
    memset(tmpipaddr6,0x00,sizeof(tmpipaddr6));
    cfg_get_software_key_value(MASK, tmpipaddr, sizeof(tmpipaddr));
    cfg_get_software_key_value(MASK, tmpipaddr6, sizeof(tmpipaddr6));  /* */
    brdcst = sock_create();
    
    /***********IPV4********/
    if(giTDMFCollectorIP.Version == IPVER_4) {
    
    
    	if((brdcst->sockID = socket(AF_INET,SOCK_DGRAM,0)) < 0)
			logoutx(7, F_ftd_mngt_create_broadcast_listener_socket, "socket error", "errno", errno);
    
  		  bzero((char *) &cli_addr,sizeof(cli_addr));
  		  cli_addr.sin_family = AF_INET;
  		  cli_addr.sin_addr.s_addr = inet_addr(tmpipaddr);
  		  cli_addr.sin_port = Listner_port;

  		  sprintf(logmsg,"mask = %s, port = %d.\n", tmpipaddr, cli_addr.sin_port);
  		  logout(17,F_ftd_mngt_create_broadcast_listener_socket,logmsg);

    	if(bind(brdcst->sockID,(struct sockaddr *) &cli_addr, sizeof(cli_addr))<0)
    	{
			logoutx(17, F_ftd_mngt_create_broadcast_listener_socket, "bind error", "errno", errno);
    	}
#if !defined(FTD_IPV4)
 	} /*********** END OF IPV4********/
   	else if (giTDMFCollectorIP.Version == IPVER_6) { /***********IPV6********/	

    	if((brdcst->sockID = socket(AF_INET6,SOCK_DGRAM,0)) < 0)
			logoutx(7, F_ftd_mngt_create_broadcast_listener_socket, "socket error", "errno", errno);

  		  bzero((char *) &cli_addr6,sizeof(cli_addr6));
  		  ipstring_to_ip(tmpipaddr6, &temp_struct);
  		  						 
  		  cli_addr6.sin6_family = AF_INET6;
  		  inet_pton(AF_INET6,tmpipaddr6,&cli_addr6.sin6_addr.s6_addr);
  		  cli_addr6.sin6_port = Listner_port;

  		  sprintf(logmsg,"IPV6 mask = %s, port = %d.\n", tmpipaddr6, cli_addr6.sin6_port);
  		  logout(17,F_ftd_mngt_create_broadcast_listener_socket,logmsg);

    	if(bind(brdcst->sockID,(struct sockaddr *) &cli_addr6, sizeof(cli_addr6))<0)
    	{
			logoutx(17, F_ftd_mngt_create_broadcast_listener_socket, "bind error", "errno", errno);
    	}
#endif
 	}
 	else
 	{
 	   logoutx(17, F_ftd_mngt_create_broadcast_listener_socket, "error", "errno", errno);
 	}												 
	/*********** END OF IPV6********/



    return brdcst;
}																		


/*
 * This function is called when a request for Agent information 
 * is received through the broadcast listener socket.
 */
void ftd_mngt_receive_on_broadcast_listener_socket(sock_t* brdcst)
{
    char tmp[80];
    int r;
    mmp_mngt_BroadcastReqMsg_t  msg;

    logout(17,F_ftd_mngt_receive_on_broadcast_listener_socket,"start.\n");


    memset(&msg,0x00,sizeof(msg));
    r = sock_recvfrom( brdcst, (char*)&msg, sizeof(msg) );
    if ( r == sizeof(msg) )
    {
        logout(17,F_ftd_mngt_receive_on_broadcast_listener_socket,"recv size OK.\n");
        mmp_convert_mngt_hdr_ntoh(&msg.hdr);
        /* validate */
        if ( msg.hdr.magicnumber != MNGT_MSG_MAGICNUMBER )
        {
            logout(4,F_ftd_mngt_receive_on_broadcast_listener_socket,"magic number mismatch.\n");
            return;
        }

        /* it is a valid TDMF management message. */
        /* should be from TDMF Collector. */
        if ( msg.hdr.sendertype == SENDERTYPE_TDMF_SERVER )
        {   /* refresh local value used throughout mngt functions */
            ip_to_ipstring(brdcst->rip,tmp);
            giTDMFCollectorIP = brdcst->rip;
        }

        if ( msg.hdr.mngttype == MMP_MNGT_AGENT_INFO_REQUEST )
        {
            logout(17,F_ftd_mngt_receive_on_broadcast_listener_socket,"mngttype OK.\n");
            msg.data.iBrdcstResponsePort = ntohl(msg.data.iBrdcstResponsePort);

            sprintf(logmsg,"received port = %d\n",msg.data.iBrdcstResponsePort);
            logout(17,F_ftd_mngt_receive_on_broadcast_listener_socket,logmsg);
            ip_to_ipstring(brdcst->rip,tmp);

            /* save TDMF Collector coordinates  */
            if ( cfg_set_software_key_value(AGENT_CFG_IP, tmp) != 0 )
            {
                logout(4,F_ftd_mngt_receive_on_broadcast_listener_socket,"Unable to write \'"AGENT_CFG_IP"\' configuration value!\n");
            }

            if ( cfg_set_software_key_value(AGENT_CFG_PORT, tmp) != 0 )
            {
                logout(4,F_ftd_mngt_receive_on_broadcast_listener_socket,"Unable to write \'"AGENT_CFG_PORT"\' configuration value !\n");
            }

            /* refresh local value used throughout mngt functions */
            giTDMFCollectorPort = msg.data.iBrdcstResponsePort;
      	    msg.data.iBrdcstResponsePort = Agent_port;

            ftd_mngt_send_agentinfo_msg( giTDMFCollectorIP, giTDMFCollectorPort); 
        }
    }
    logout(17,F_ftd_mngt_receive_on_broadcast_listener_socket,"return.\n");
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
ftd_mngt_send_agentinfo_msg(ipAddress_t rip, int rport)
{
    
  
    int r = 0;
    char tmp[48];
    mmp_mngt_TdmfAgentConfig2Msg_t msg;
	if(rip.Version != IPVER_4 && rip.Version != IPVER_6)
	{
			memset(tmp,0x00,sizeof(tmp));
		    cfg_get_software_key_value(AGENT_CFG_IP, tmp, sizeof(tmp));
        	 
		    ipstring_to_ip(tmp,&rip);

			if(rip.Version == IPVER_4) {
        		rip.Addr.V4 = inet_addr(tmp);
			}
	 		else {
#if !defined(FTD_IPV4)			 
				inet_pton(AF_INET6, tmp, (struct in6_addr16 *) rip.Addr.V6.Word) ;
			   //rip.Addr.V6.ScopeID = 1;
#endif
			} 
	}
		  /*	 														 
#if defined(linux)					   
		
		rip = ntohl(rip);
#endif	*/
		   				   
  												   
    if (rport == 0)
    {
        rport = giTDMFCollectorPort;
    }
    
    ip_to_ipstring(rip,tmp);
    sprintf(logmsg,"send IPaddr = %s,port = %d.\n",tmp,rport);				   
    logout(17,F_ftd_mngt_send_agentinfo_msg,logmsg);
    if ( rport != 0 )
    {																														  
																				  
        memset(&msg,0x00,sizeof(msg));
        msg.hdr.magicnumber     = MNGT_MSG_MAGICNUMBER;
        msg.hdr.mngttype        = MMP_MNGT_EX_AGENT_INFO2;
        msg.hdr.mngtstatus      = MMP_MNGT_STATUS_OK; 
        msg.hdr.sendertype      = SENDERTYPE_TDMF_AGENT;


		strcpy(msg.hdrEx.szServerUID,gszServerUID);

		msg.hdrEx.ulMessageSize     = sizeof(msg.data) ;

        msg.hdrEx.ulMessageVersion  = SERVERINFO2_VERSION;
        msg.hdrEx.ulInstanceCount   = 1;
        /* Agent info  */
        ftd_mngt_gather_server_info(&msg.data);


        mmp_convert_mngt_hdr_hton(&msg.hdr);

		mmp_convert_mngt_hdrEx_hton(&msg.hdrEx);
               
        
        mmp_convert_TdmfServerInfo2_hton(&msg.data);
        

									// -3 has been removed from data len
       r = sock_sendtox(rip,rport,(char*)&msg,sizeof(msg) ,0,0); 

 
        if ( r < 0 )
        {
    	    		logout(17,F_ftd_mngt_send_agentinfo_msg,"sock_sendtox error.\n");
        }							   		
    }									 
															   
														  

    logout(17,F_ftd_mngt_send_agentinfo_msg,"return.\n");
    return r;
}

/****************************************************
 *  ftd status message thread
 ****************************************************/
#include "ftd_mngt_StatusMsgThread.c"

#include "ftd_mngt_gather_server_info.c"

#include "ftd_mngt_send_lg_state.c"

#include "ftd_mngt_performance_send_data.c"

#include "ftd_mngt_sys_request_system_restart.c"
