/* #ident "@(#)$Id: ftd_mngt.c,v 1.25 2004/06/19 22:50:32 jqwn0 Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

/*
 * ftd_mngt.c - ftd management message handlers
 *
 * Copyright (c) 2002 Softek Technology Corporation.  All Rights Reserved.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/utsname.h>
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

void ftd_mngt_gather_server_info(mmp_TdmfServerInfo *srvrInfo);
void ftd_mngt_performance_send_data();
void ftd_mngt_send_all_state();

extern int Listner_port;
extern int Agent_port;
extern char logmsg[];

time_t ModifyTime;

extern  char    gszServerUID[];

/* updated each time a msg is received from Collector */
int			giTDMFCollectorIP;
int			giTDMFCollectorPort;
int			giTDMFAgentPort;
int			giTdmfPerfFastCheckCnt;
int			giTDMFAgentIP;
int			giTDMFBabSize;
mmp_TdmfPerfConfig	gTdmfPerfConfig;
TDMFAgentEmulator	gTdmfAgentEmulator;

/*****************************************************************************/
/*
 * Called once, when TDMF Agent starts.
 */
void ftd_mngt_initialize()
{
    char tmp[32];
    struct servent *port;        /* Server entry structure */
    unsigned short cnvport;
    int reverse;

    /* INIT  */
    giTDMFCollectorIP = 0;
    giTDMFAgentIP = 0;
    giTDMFCollectorPort = FTD_DEF_AGENTPORT;
    giTdmfPerfFastCheckCnt = 0;

    memset(tmp,0x00,sizeof(tmp));
    cfg_get_software_key_value("reverse", tmp, sizeof(tmp));
    if (strlen(tmp) == 0)
    {
        reverse = 1;
    }
    else 
    {
        reverse = 0;
    }
    if ((port = getservbyname(FTD_LISTNER, "udp"))) {
        if (reverse ==0)
        {
            Listner_port = ntohs(port->s_port);
        }
        else {
            cnvport = (unsigned short)ntohs(port->s_port);
            rv2((char *)&cnvport);
            Listner_port = (int)cnvport;
        }
        
    } else {
        if(reverse == 0) 
        {
            Listner_port = (int) FTD_DEF_LISTNERPORT;
        }
        else
        {
            cnvport = (unsigned short)FTD_DEF_LISTNERPORT;
            rv2((char *)&cnvport);
            Listner_port = (int)cnvport;
        }
        sprintf(logmsg,"convert LISTNER port = %d.\n",Listner_port);
        logout(17,F_ftd_mngt_initialize,logmsg);
    }
    if ((port = getservbyname(FTD_AGENT, "tcp"))) {
        if (reverse == 0)
        {
            Agent_port = ntohs(port->s_port);
        }
        else
        {
            cnvport = (unsigned short)ntohs(port->s_port);
            rv2((char *)&cnvport);
            Agent_port = (int)cnvport;
        }
    } else {
        if (reverse == 0)
        {
            Agent_port = (int) FTD_DEF_AGENTPORT;
        }
        else
        {
            cnvport = (unsigned short)FTD_DEF_AGENTPORT;
            rv2((char *)&cnvport);
            Agent_port = (int)cnvport;
        }
    }
    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value("CollectorIP", tmp, sizeof(tmp)) == 0 )
    {
        ipstring_to_ip(tmp,(unsigned long*)&giTDMFCollectorIP);
    }
    else
    {
        giTDMFCollectorIP = 0;
			/*to be init one first msg received from TDMF Collector
                              until then , no msg can be sent. */
    }

    /* If specific agnet IP is used, collector should use this IP to contact
       agent.
    */
    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value("AgentIP", tmp, sizeof(tmp)) == 0 )
    {
        ipstring_to_ip(tmp,(unsigned long*)&giTDMFAgentIP);
    }
    else
    {
        giTDMFAgentIP= 0;
    }

    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value("CollectorPort", tmp, sizeof(tmp)) == 0 )
    {
        if(reverse == 0)
        {
            giTDMFCollectorPort = atoi(tmp);
        }
        else
        {
            cnvport = (unsigned short)atoi(tmp);
            rv2((char *)&cnvport);
            giTDMFCollectorPort = (int)cnvport;
        }
    }
    else
    {
        if(reverse == 0)
        {
            giTDMFCollectorPort = FTD_DEF_COLLECTORPORT; /*default value */
        }
        else
        {
            cnvport = (unsigned short)FTD_DEF_COLLECTORPORT;
            rv2((char *)&cnvport);
            giTDMFCollectorPort = (int)cnvport;
        }
    }

    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value("BabSize", tmp, sizeof(tmp)) == 0 )
    {
        giTDMFBabSize = atoi(tmp);	/* MB order */
    }
    else
    {
        giTDMFBabSize = 0;
    }

    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value("PerfUploadPeriod", tmp, sizeof(tmp)) == 0 )
    {
        gTdmfPerfConfig.iPerfUploadPeriod = atoi(tmp);
    }
    else
    {
        gTdmfPerfConfig.iPerfUploadPeriod = 100; /*default value = 10 seconds */
    }
    memset(tmp,0x00,sizeof(tmp));
    if ( cfg_get_software_key_value("ReplGroupMonitUploadPeriod", tmp, sizeof(tmp)) == 0 )
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
    if ( cfg_get_software_key_value("AgentEmulator", tmp, sizeof(tmp)) == 0 )
    {
        if ( 0 == strncmp(tmp,"true",4) || 0 == strncmp(tmp,"yes",3) || 0 == strncmp(tmp,"1",1) )
        {
            gTdmfAgentEmulator.bEmulatorEnabled = ~0; /* enabled */
            memset(tmp,0x00,sizeof(tmp));
            if ( cfg_get_software_key_value("EmulatorRangeMin", tmp, sizeof(tmp)) == 0 )
            {
                gTdmfAgentEmulator.iAgentRangeMin = atoi(tmp);
            }
            else
            {
                gTdmfAgentEmulator.iAgentRangeMin = 1;
            }
            memset(tmp,0x00,sizeof(tmp));
            if ( cfg_get_software_key_value("EmulatorRangeMax", tmp, sizeof(tmp)) == 0 )
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
    unsigned long ulBroadcastIP,ulBroadcastPort;
    int r;
    char tmpipaddr[15+1];
    sock_t* brdcst;
    struct sockaddr_in cli_addr;
    int opt;
   
    memset(tmpipaddr,0x00,sizeof(tmpipaddr)); 
    cfg_get_software_key_value("mask", tmpipaddr, sizeof(tmpipaddr));
    brdcst = sock_create();
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
            if ( cfg_set_software_key_value("CollectorIP", tmp) != 0 )
            {
                logout(4,F_ftd_mngt_receive_on_broadcast_listener_socket,"Unable to write \'CollectorIP\' configuration value!\n");
            }
  	    /*
            itoa(msg.data.iBrdcstResponsePort,tmp,10);
	     */

            if ( cfg_set_software_key_value("CollectorPort", tmp) != 0 )
            {
                logout(4,F_ftd_mngt_receive_on_broadcast_listener_socket,"Unable to write \'CollectorPort\' configuration value !\n");
            }

            /* refresh local value used throughout mngt functions */
            giTDMFCollectorPort = msg.data.iBrdcstResponsePort;
      	    msg.data.iBrdcstResponsePort = Agent_port;

#if 1
	    /* (brdcst->rip, giTDMFCollectorPort); */
            ftd_mngt_send_agentinfo_msg( giTDMFCollectorIP, giTDMFCollectorPort); 
#else
	    /*(brdcst->rip, msg.data.iBrdcstResponsePort);*/
            ftd_mngt_send_agentinfo_msg( giTDMFCollectorIP, giTDMFCollectorPort); 
#endif
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
ftd_mngt_send_agentinfo_msg(int rip, int rport)
{
    int r;
    char tmp[50];
    mmp_mngt_TdmfAgentConfigMsg_t msg;

    logout(17,F_ftd_mngt_send_agentinfo_msg,"start.\n");
    /****************************************************/
    /* build Agent Info response msg                    */
    /****************************************************/
    if (rip == 0)
    {
        memset(tmp,0x00,sizeof(tmp));
        cfg_get_software_key_value("CollectorIP", tmp, sizeof(tmp));
        rip = inet_addr(tmp);
#if defined(linux)
		rv4((unsigned char*)&rip);
#endif
    }
    if (rport == 0)
    {
        rport = giTDMFCollectorPort;
    }
    ip_to_ipstring(rip,tmp);
    sprintf(logmsg,"send IPaddr = %s,port = %d.\n",tmp,rport);
    logout(17,F_ftd_mngt_send_agentinfo_msg,logmsg);
    if ( rip != 0 && rport != 0 )
    {

        memset(&msg,0x00,sizeof(msg));
        msg.hdr.magicnumber     = MNGT_MSG_MAGICNUMBER;
        msg.hdr.mngttype        = MMP_MNGT_AGENT_INFO;
        msg.hdr.mngtstatus      = MMP_MNGT_STATUS_OK; 
        msg.hdr.sendertype      = SENDERTYPE_TDMF_AGENT;
        /* Agent info  */
        ftd_mngt_gather_server_info(&msg.data);
        mmp_convert_mngt_hdr_hton(&msg.hdr);
        mmp_convert_TdmfServerInfo_hton(&msg.data);
        
        r = sock_sendtox(rip,rport,(char*)&msg,sizeof(msg)-3,0,0); 
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
