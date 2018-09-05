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
/* #ident "@(#)$Id: Agent_proc.c,v 1.8 2014/05/06 16:19:16 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <sys/utsname.h>
#if defined(SOLARIS) || defined(HPUX)
#include <sys/syscall.h>
#endif
#if defined(SOLARIS) || defined(_AIX)	/* for HP */
#include <sys/sysconfig.h>
#endif					/* for HP */
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <fcntl.h>
#include <dirent.h>     /* WR15542 : add */
#include "tdmfAgent.h"
#include "tdmfAgent_trace.h"
#include "license.h"	/* for NP_CRYPT_GROUP_SIZE */
#include "ftdio.h"	/* for NP_CRYPT_GROUP_SIZE */
#include "limits.h"

static int
ftd_mngt_get_all_devices(sock_t *sockID);

int     dbg;
// extern FILE	*log;
extern int  Debug;
extern char logmsg[];
int RequestID;

extern	ipAddress_t                     giTDMFCollectorIP;
extern	unsigned int                     giTDMFCollectorPort;
extern	mmp_TdmfPerfConfig      gTdmfPerfConfig;

int Agent_proc(sock_t * sockID)
{
	char dummyread[1024];
	fd_set  readset;
	int     nselect = 0;
	int n;
	mmp_mngt_header_t *header;
	int rc;

	logout(12,F_Agent_proc,"start\n");

#if defined(linux)
	signal (SIGCLD,  SIG_DFL);
	signal (SIGCHLD, SIG_DFL);
#endif

retry:
	/* Wait message */
	FD_ZERO(&readset);
	FD_SET(sockID->sockID,&readset);

	nselect = sockID->sockID+1;

	logout(12,F_Agent_proc,"select call.\n");
	n = select(nselect, &readset, NULL, NULL, NULL);
	logout(12,F_Agent_proc,"select return.\n");

	switch(n)
	{
	case -1:
	case 0:
		break;
	default:
		/* collector data */
		sprintf(logmsg,"select end code = %d.\n",n); 
		logout(12,F_Agent_proc,logmsg);
		if (FD_ISSET(sockID->sockID, &readset))
		{
			header = (mmp_mngt_header_t *)malloc(sizeof(mmp_mngt_header_t));
			if (header == NULL)
			{
				logoutx(4, F_Agent_proc, "malloc failed", "errno", errno);
				return(-1);
			}
			errno = 0;
			rc = read(sockID->sockID, (char*)header, sizeof(mmp_mngt_header_t));
			/* recv length check */
			sprintf(logmsg,"read socket, header len = %d, code = %d.\n",rc,errno); 
			logout(12,F_Agent_proc,logmsg);
			if (rc <= 0) 
			{
				/* socket error */
				logoutx(4, F_Agent_proc, "request header read failed", "errno", errno);
				free(header);
				header = NULL;
				return(-1);
			}
			/* recv header check */

			sprintf(logmsg,"magicnumber = %x, mngttype = %x(%x), sendertype = %d, mngtstatus = %d\n", header->magicnumber, ntohl(header->mngttype), header->mngttype, header->sendertype, header->mngtstatus);
			logout(12,F_Agent_proc,logmsg);

			/* select request */
			switch(ntohl(header->mngttype))
			{
			case MMP_MNGT_SET_LG_CONFIG:
				logout(11,F_Agent_proc,"MMP_MNGT_SET_LG_CONFIG: call ftd_mngt_set_config()\n");
				rc = ftd_mngt_set_config(sockID);
				break;
			case MMP_MNGT_GET_LG_CONFIG:
				logout(11,F_Agent_proc,"MMP_MNGT_GET_LG_CONFIG: call ftd_mngt_get_config()\n");
				rc = ftd_mngt_get_config(sockID);
				break;
			case MMP_MNGT_REGISTRATION_KEY:
				logout(11,F_Agent_proc,"MMP_MNGT_REGISTRATION_KEY: call ftd_mngt_registration_key_req()\n");
				rc = ftd_mngt_registration_key_req(sockID);
				break;
			case MMP_MNGT_TDMF_CMD:
				logout(11,F_Agent_proc,"MMP_MNGT_CMD: call ftd_mngt_cmd()\n");
				rc = ftd_mngt_tdmf_cmd(sockID);
				break;
			case MMP_MNGT_SET_AGENT_GEN_CONFIG:
				logout(11,F_Agent_proc,"MMP_MNGT_SET_AGENT_GEN_CONFIG: call ftd_mngt_agent_general_config()\n");
			case MMP_MNGT_GET_AGENT_GEN_CONFIG:
				logout(11,F_Agent_proc,"MMP_MNGT_GET_AGENT_GEN_CONFIG: call ftd_mngt_agent_general_config()\n");
		  		rc = ftd_mngt_agent_general_config(sockID,
		 		ntohl(header->mngttype));
				break;
			case MMP_MNGT_GET_ALL_DEVICES:
				logout(11,F_Agent_proc,"MMP_MNGT_GET_ALL_DEVICES: call ftd_mngt_get_all_devices()\n");
				rc = ftd_mngt_get_all_devices(sockID);
				break;
			case MMP_MNGT_AGENT_INFO_REQUEST:
				logout(11,F_Agent_proc,"MMP_MNGT_AGENT_INFO_REQUEST: No operation\n");
				rc = -1;
				break;
			case MMP_MNGT_PERF_CFG_MSG:
				logout(11,F_Agent_proc,"MMP_MNGT_PERF_CFG_MSG: call ftd_mngt_get_perf_cfg()\n");
				rc = ftd_mngt_get_perf_cfg(sockID);
				break;
			case MMP_MNGT_SET_ALL_DEVICES:
				logout(11,F_Agent_proc,"MMP_MNGT_SET_ALL_DEVICES: No operation\n");
			case MMP_MNGT_AGENT_INFO:
				logout(11,F_Agent_proc,"MMP_MNGT_AGENT_INFO: No operation\n");
				rc = -1;
				break;
			case MMP_AGN_TDMF_CMD_RMDCHK:
				logout(12,F_Agent_proc,"MMP_AGN_CMD_RMDCHK: call ftd_agn_get_rmd_stat()\n");
				rc = ftd_agn_get_rmd_stat(sockID);
				break;
			case MMP_MNGT_TDMF_SENDFILE:	/* receiving a TDMF file */
				logout(11,F_Agent_proc,"MMP_MNGT_SENDFILE: call ftd_mngt_set_file()\n");
				rc = ftd_mngt_set_file(sockID);
				break;
			case MMP_MNGT_TDMF_GETFILE:		/* send TDMF file(s) */
				logout(11,F_Agent_proc,"MMP_MNGT_GETFILE: call ftd_mngt_get_file()\n");
				rc = ftd_mngt_get_file(sockID);
				break;
			case MMP_MNGT_GET_PRODUCT_USAGE_DATA:  	/* send Product usage statistics to the DMC */
				logout(11,F_Agent_proc,"MMP_MNGT_GET_PRODUCT_USAGE_DATA: call ftd_mngt_get_product_usage_data()\n");
				rc = ftd_mngt_get_product_usage_data(sockID);
				break;
			default:
				logout(9,F_Agent_proc,"unknown : try to read\n");
				rc = read(sockID->sockID, (char*)dummyread,48);
				sprintf(logmsg,"unknown packet read. size = %d.\n",rc);
				logout(12,F_Agent_proc,logmsg);
				free(header);
				header = NULL;
				goto retry;
				break;
			}
            sprintf(logmsg, "finish mngttype = %x(%x), rc = %d, errno = %d.\n", 
                    ntohl(header->mngttype), header->mngttype, rc, errno); 
            logout(12, F_Agent_proc, logmsg);																  

			free(header);
			header = NULL;
		}
	}

	return (0);

}

#include "ftd_mngt_set_config.c"

#include "ftd_mngt_get_config.c"

#include "ftd_mngt_registration_key_req.c"

#include "ftd_mngt_tdmf_cmd.c"

#include "ftd_mngt_agent_general_config.c"

/*
 * Manages MMP_MNGT_GET_ALL_DEVICES request
 */
#include "ftd_mngt_get_all_devices.c"

#include "ftd_mngt_get_perf_cfg.c"

#include "ftd_agn_get_rmd_stat.c"

#include "ftd_mngt_get_product_usage_data.c"
