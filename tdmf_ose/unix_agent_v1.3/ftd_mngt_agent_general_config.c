/* #ident "@(#)$Id: ftd_mngt_agent_general_config.c,v 1.20 2003/11/13 02:48:21 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_AGENT_GENERAL_CONFIG_C_
#define _FTD_MNGT_AGENT_GENERAL_CONFIG_C_

/*
 * Manages MMP_MNGT_SET_AGENT_GEN_CONFIG or 
 *         MMP_MNGT_GET_AGENT_GEN_CONFIG requests.
 */

#if defined(HPUX)
#include <sys/param.h>
#include <sys/pstat.h>
#elif defined(linux)
#include <sys/param.h>
#endif

extern void execCommand(char *, char *);

int set_service_port( int );
int isValidBABSize(int);

int ftd_mngt_agent_general_config(sock_t *sockID, int iMngtType)
{
    mmp_mngt_TdmfAgentConfigMsg_t   cmdmsg;
    int                             r,toread;
    char                            *pWk;
    int 			    ctlfd;
    int                             requestRestart = false;
    unsigned long   		    curValue;
    struct servent  		    *port;
#if 0
	ftd_babinfo_t babinfo;
#endif

    pWk  = (char*)&cmdmsg;
    pWk += sizeof(mmp_mngt_header_t);
    /**********************************/
    /* at this point, mmp_mngt_header_t header is read.
     * now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure
     */
    toread = sizeof(mmp_mngt_TdmfAgentConfigMsg_t)-sizeof(mmp_mngt_header_t);
    toread = toread-3;
    r = ftd_sock_recv(sockID,(char*)pWk, toread);
    if ( r != toread )
    {
        logout(3, F_ftd_mngt_agent_general_config, 
		"bad format General config message received.\n");
        return -1;
    }
    /* convert from network bytes order to host byte order */
    mmp_convert_TdmfServerInfo_ntoh(&cmdmsg.data);
    sprintf(logmsg, "%s Agent general config received\n",
		 iMngtType == MMP_MNGT_GET_AGENT_GEN_CONFIG ? "GET" :"SET");
    logout(17, F_ftd_mngt_agent_general_config, logmsg);
    if ( iMngtType == MMP_MNGT_SET_AGENT_GEN_CONFIG )
    {
        char tmp[16];

        sprintf(logmsg, "Received new config: \n"
                         "      BAB size            = %d MB\n"
                         "      TCP Window size     = %d KB\n"
                         "      Primary Port Number = %d \n"
                         "      and more ...\n"
                         ,  cmdmsg.data.iBABSizeReq
                         ,  cmdmsg.data.iTCPWindowSize
                         ,  cmdmsg.data.iPort
                         );
   	logout(17, F_ftd_mngt_agent_general_config, logmsg);

        /* validate entries before saving. */
        /* BAB size cannot be more than 60% of total RAM size */
        if ( !isValidBABSize(cmdmsg.data.iBABSizeReq) )
        {
            cmdmsg.hdr.mngtstatus = 
			MMP_MNGT_STATUS_ERR_INVALID_SET_AGENT_GEN_CONFIG;
        }
        else 
        {   /* must check if critital values are changed.
			 if so, a system restart will be requested. */

            requestRestart = false;
	    memset(tmp, 0, sizeof(tmp));
            /* if ( cfg_get_key_value("num_chunks", */
            if ( cfg_get_key_value("num_chunks", tmp, 
						CFG_IS_NOT_STRINGVAL) == 0 )
            {
                curValue = atol(tmp);
            }
	    memset(tmp, 0, sizeof(tmp));
            if ( cfg_get_key_value("chunk_size", tmp, 
						CFG_IS_NOT_STRINGVAL) == 0 )
            {
                curValue *= atol(tmp);
                curValue = curValue / 1024 /1024;	/* Bytes to MBytes */
		if (curValue != cmdmsg.data.iBABSizeReq) { 
            	    requestRestart |= GENE_CFG_BABSIZE; 
		}
            }
	    memset(tmp, 0, sizeof(tmp));
            if ( cfg_get_key_value("tcp_window_size",tmp,
						CFG_IS_NOT_STRINGVAL) == 0 )
            {
                curValue = atol(tmp);
                curValue /= 1024;	/* Bytes to KBytes */
	        if (curValue != cmdmsg.data.iTCPWindowSize) { 
            	    requestRestart |= GENE_CFG_WINDOWSIZE; 
	        }
            }
            if ((port = getservbyname("in.dtc", "tcp")))
	    {
                curValue = ntohs(port->s_port);
	        if (curValue != cmdmsg.data.iPort) { 
            	    requestRestart |= GENE_CFG_PORT; 
	        }
            }

            /***************************************/
            /* reuse cmdmsg for response to requester */
            cmdmsg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK; /* assuming success */

            /**************************************/

	    /* num_chunks & chunks_size are set by ftd_system_restarting() */

            /* itoa(cmdmsg.data.iTCPWindowSize,tmp,10); */
            sprintf(tmp,"%d",cmdmsg.data.iTCPWindowSize*1024); /* KBytes to Bytes */
            if ( cfg_set_key_value("tcp_window_size",tmp,
						CFG_IS_NOT_STRINGVAL) != 0 )
            {
                logout(4, F_ftd_mngt_agent_general_config,
                   "could not save TCP Window size to Driver config.\n");
                cmdmsg.hdr.mngtstatus  =
			 	MMP_MNGT_STATUS_ERR_SET_AGENT_GEN_CONFIG;
            }
            /* itoa(cmdmsg.data.iPort,tmp,10); */
            if ( set_service_port( cmdmsg.data.iPort ) != 0 )
            {
                logout(4, F_ftd_mngt_agent_general_config, 
                              "could not save Port to Service config.\n");
                cmdmsg.hdr.mngtstatus =
				 MMP_MNGT_STATUS_ERR_SET_AGENT_GEN_CONFIG;
            }
            if ( cfg_set_software_key_value("domain",
		 cmdmsg.data.szTDMFDomain) != 0 )
            {
                logout(4, F_ftd_mngt_agent_general_config, 
                       "could not save domain to Agent config.\n");
                cmdmsg.hdr.mngtstatus =
				 MMP_MNGT_STATUS_ERR_SET_AGENT_GEN_CONFIG;
            }
#if 0 
	    cmdmsg.data.iBABSizeAct = cmdmsg.data.iBABSizeReq;
#endif 
            /* todo : Router IP ? */
        }
    }
    else
    {   /* get config 
         * reuse cmdmsg for response to requester
  	 */
        cmdmsg.hdr.mngtstatus  = MMP_MNGT_STATUS_OK; /* assuming success */

        ftd_mngt_gather_server_info(&cmdmsg.data);
        if ((port = getservbyname("in.dtc", "tcp")))
        {
            cmdmsg.data.iPort = ntohs(port->s_port);
        }

        sprintf(logmsg, "Reported Agent General config:\n"
                         "      requested BAB size  = %d MB\n"
                         "      actual BAB size     = %d MB\n"
                         "      TCP Window size     = %d KB\n"
                         "      Primary Port Number = %d \n"
                         "      and more ...\n"
                         ,  cmdmsg.data.iBABSizeReq
                         ,  cmdmsg.data.iBABSizeAct
                         ,  cmdmsg.data.iTCPWindowSize
                         ,  cmdmsg.data.iPort
                         );
	logout(17, F_ftd_mngt_agent_general_config, logmsg);
    }

    /* complete response msg and send it to requester */
    cmdmsg.hdr.magicnumber  = MNGT_MSG_MAGICNUMBER;
    cmdmsg.hdr.mngttype     = iMngtType;
    cmdmsg.hdr.sendertype   = SENDERTYPE_TDMF_AGENT;
    mmp_convert_mngt_hdr_hton(&cmdmsg.hdr);
    mmp_convert_TdmfServerInfo_hton(&cmdmsg.data);
    /* respond using same socket */
    r = ftd_sock_send(sockID,(char*)&cmdmsg,sizeof(cmdmsg)-3);
    if ( r != sizeof(cmdmsg)-3 )
    {
        logout(4, F_ftd_mngt_agent_general_config, 
               "while sending AgentConfigMsg to requester.\n");
    }

    if ( requestRestart )
    {
		mmp_convert_TdmfServerInfo_ntoh(&cmdmsg.data);
        ftd_mngt_sys_request_system_restart(&cmdmsg.data, requestRestart);
        /*
         * retrieve actual BAB size from the TDMF kernel device driver
         * open the master control device
         */
#if 0
		if ((ctlfd = open(FTD_CTLDEV, O_RDONLY)) < 0) {
			logoutx(4, F_ftd_mngt_agent_general_config, "open failed", "errno", errno);
		} else {
			if (FTD_IOCTL_CALL(ctlfd, FTD_GET_BAB_SIZE, &babinfo) < 0) {
				logoutx(4, F_ftd_mngt_agent_general_config, "ioctl FTD_GET_BAB_SIZE failed", "errno", errno);
			} else {
				/* convert to MB */
				if (cmdmsg.data.iBABSizeAct != babinfo.actual >> 20)
				ftd_mngt_send_agentinfo_msg(0,0);
			}
			close(ctlfd);
		}
#else
		ftd_mngt_send_agentinfo_msg(0,0);
#endif

	}

	return 0;

}

int set_service_port( int port )
{
        int     rc = 0;
        FILE    *infd;
        FILE    *outfd;
        char    buf[1024+1];

#if 0   /* After for the PORT number, Collector is support. */
        /* copy services file */
        sprintf(buf,"/usr/bin/cp %s %s 1>/dev/null 2>/dev/null",SERVICES_FILE, PRE_SERVICES_FILE);
        system(buf);

        /* opem services file */
        outfd = fopen(SERVICES_FILE,"w");
        if (outfd <= (FILE *)NULL) return (-1);
        infd = fopen(PRE_SERVICES_FILE,"r");
        if (infd <= (FILE *)0)
        {
                /* file open error */
                fclose (infd);
                infd = NULL;
                return (-1);
        }

        /* copy servies data */
        for(;;)
        {
                memset(buf,0x00,sizeof(buf));
                fgets(buf,sizeof(buf),infd);
                if (strlen(buf) == 0)
                {
                        break;
                }
                if (strncmp("in.dtc",buf,6) == 0)
                {
                        sprintf(buf,
                        "in.dtc\t\t%d/tcp\t\t\t# SFTKdtc master daemon\n",port);
                }
                fputs(buf,outfd);
        }
        fclose(infd);
        fclose(outfd);
#endif  /* After for the PORT number, Collector is support. */
        return(rc);
}

int isValidBABSize(int iBABSizeMb)
{
	int          maxPhysMemKb;
	long long    pagesize, phys_pages;
	char		buf[MAXPATHLEN];
#if defined(HPUX)
	struct pst_static pst;
#endif

#if defined(SOLARIS)
	pagesize = _sysconfig(_CONFIG_PAGESIZE);
	phys_pages = _sysconfig(_CONFIG_PHYS_PAGES);
	maxPhysMemKb = pagesize * phys_pages >> 10;
#elif defined(_AIX)
	execCommand("bootinfo -r", buf);
	phys_pages = atol(buf);
	maxPhysMemKb = phys_pages;
#elif defined(HPUX)
	if ( (pstat_getstatic(&pst, sizeof(pst), (size_t) 1, 0)) < 0) {
		maxPhysMemKb = 0;
	} else {
		pagesize = pst.page_size;;
		phys_pages = pst.physical_memory;
		maxPhysMemKb = pagesize * phys_pages >> 10;
	}
#elif defined(linux)
    pagesize = sysconf(_SC_PAGESIZE);
    phys_pages = sysconf(_SC_PHYS_PAGES);
	maxPhysMemKb = pagesize * phys_pages >> 10;
#endif

	maxPhysMemKb = maxPhysMemKb * 6 / 10;

	if (iBABSizeMb*1024 <= maxPhysMemKb)
		return true;
	return false;
}

#endif /* _FTD_MNGT_AGENT_GENERAL_CONFIG_C_ */
