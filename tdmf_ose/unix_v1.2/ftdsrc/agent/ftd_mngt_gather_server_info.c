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
/* #ident "@(#)$Id: ftd_mngt_gather_server_info.c,v 1.12 2010/12/20 20:12:25 dkodjo Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_GATER_SERVER_INFO_C_
#define _FTD_MNGT_GATER_SERVER_INFO_C_

#if defined(HPUX)
#include <sys/param.h>
#include <sys/pstat.h>

#elif defined(linux)
#include <sys/param.h>

#endif


extern void execCommand(char *, char *);
extern void getversion(char *, int);
extern int agent_getnetconfs(unsigned int *);

void
ftd_mngt_gather_server_info(mmp_TdmfServerInfo2 *srvrInfo)
{
	struct utsname	utsname;
#if defined(HPUX)
	struct pst_static pst;
#endif
	int	error;
	long long	pagesize, phys_pages, avail_pages;
	int	i;
	int	j = 0;
	struct servent  *port;
	int ctlfd;
	int tmp;
	char temp_ip_addr[48];
	int tmplen;
	int sock;
	int n_ip = 0;
	unsigned short cnvport;
	char buf[MAXPATHLEN];
	char version[64];
	char ver[32], build[32];												
	int num_chunks, chunk_size;
	u_int *ipaddrs;
	char ipstr[48];
	ftd_babinfo_t babinfo;
	char hostname_long[80];
#if !defined (FTD_IPV4)
	int n_ip_v6 = 0;
	char *ipv6_addr;
	int k = 0; 
	char *tmp_ipv6;
	char *token_ipv6;
	char *pch;
	char *linkloc;
#endif
#if defined(_AIX)
	char cmdline[256];
	FILE *f;
#elif defined(HPUX)
	struct pst_dynamic pstdyn;
#endif
	ipAddress_t temp_struct_tofill;
	memset((char *)srvrInfo, 0, sizeof(mmp_TdmfServerInfo2));

	strcpy(srvrInfo->ServerHostID, gszServerUID);

	if (uname(&utsname) == -1) {
		logoutx(4, F_ftd_mngt_gather_server_info, "uname failed", "errno", errno);
	} else {
		gethostname(hostname_long,sizeof(hostname_long));
		strcpy(srvrInfo->MachineName, hostname_long);
#if defined(SOLARIS)
		if (strcmp(utsname.sysname, "SunOS") == 0) {
			strcpy(srvrInfo->OsType, "Solaris");
			if (strcmp(utsname.release, "5.7") == 0) {
				strcpy(srvrInfo->OsVersion, "7");
			} else if (strcmp(utsname.release, "5.8") == 0) {
				strcpy(srvrInfo->OsVersion, "8");
			} else if (strcmp(utsname.release, "5.9") == 0) {
				strcpy(srvrInfo->OsVersion, "9");
			} else if (strcmp(utsname.release, "5.10") == 0) {
				strcpy(srvrInfo->OsVersion, "10");
   			} else if (strcmp(utsname.release, "5.6") == 0) {
				strcpy(srvrInfo->OsVersion, "2.6");
			} else {
				strcpy(srvrInfo->OsVersion, utsname.release);
			}
		} else {
			strcpy(srvrInfo->OsType, utsname.sysname);
			strcpy(srvrInfo->OsVersion, utsname.release);
		}
#elif defined(_AIX)
		strcpy(srvrInfo->OsType, utsname.sysname);
		sprintf(buf,"%s.%s",utsname.version,utsname.release);
		if (strlen(buf) > MMP_MNGT_MAX_OS_VERSION_SZ)
			srvrInfo->OsVersion[0] = '\0';
		else
			strcpy(srvrInfo->OsVersion,buf);
#elif defined(HPUX)
		strcpy(srvrInfo->OsType, utsname.sysname);
		memcpy(srvrInfo->OsVersion, utsname.release, UTSLEN);
#elif defined(linux)
		strcpy(srvrInfo->OsType, utsname.sysname);
		strcpy(srvrInfo->OsVersion, utsname.release);
#endif
	}
																															   
	strcpy(srvrInfo->IPRouter, NULL_IP_STR);
	if (cfg_get_software_key_value(AGENT_CFG_DOMAIN,
				 srvrInfo->DomainName, sizeof(srvrInfo->DomainName)) != 0) {
		strcpy(srvrInfo->DomainName, DEFAULT_TDMF_DOMAIN_NAME);
	}							
	/* Get cpus */
#if defined(SOLARIS) || defined(linux)
	srvrInfo->CPUCount = sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_AIX)
	sprintf(cmdline, "/usr/sbin/lsdev -C -c processor -s sys | wc -l 2> /dev/null");
	if ((f = popen(cmdline, "r")) == NULL) {
		logoutx(4, F_ftd_mngt_gather_server_info, "popen failed", "errno", errno);
	} else {
		if (fgets(buf, 80, f) == NULL) {
			pclose(f);
		} else {
			srvrInfo->CPUCount = strtol(buf, NULL, 10);
		}
	}
#elif defined(HPUX)
	pstat_getdynamic(&pstdyn, sizeof(pstdyn), 1, 0);
	srvrInfo->CPUCount = pstdyn.psd_proc_cnt;
#endif
	/*
	 * Get memory information from the system.
	 * The 10-bit's shift changes the unit from byte to Kbyte. 
	 */
#if defined(SOLARIS)
	pagesize = _sysconfig(_CONFIG_PAGESIZE);
	phys_pages = _sysconfig(_CONFIG_PHYS_PAGES);
	avail_pages = _sysconfig(_CONFIG_AVPHYS_PAGES);
	srvrInfo->RAMSize = phys_pages * pagesize >> 10;
	srvrInfo->AvailableRAMSize = avail_pages * pagesize >> 10;	 
#elif defined(_AIX)
	execCommand("bootinfo -r", buf);
	phys_pages = atol(buf);
	srvrInfo->RAMSize = phys_pages;
	srvrInfo->AvailableRAMSize = 0;
#elif defined(HPUX)
	if ( (pstat_getstatic(&pst, sizeof(pst), (size_t) 1, 0)) < 0) {
		srvrInfo->RAMSize = 0;
		logout(4, F_ftd_mngt_gather_server_info, "pstat_getstatic failed.\n");
	} else {
		pagesize = pst.page_size;
		phys_pages = pst.physical_memory;
		srvrInfo->RAMSize = phys_pages * pagesize >> 10;
	}
	srvrInfo->AvailableRAMSize = 0;
#elif defined(linux)
	pagesize = sysconf(_SC_PAGESIZE);
	phys_pages = sysconf(_SC_PHYS_PAGES);
	avail_pages = sysconf(_SC_AVPHYS_PAGES);
	srvrInfo->RAMSize = phys_pages * pagesize >> 10;
	srvrInfo->AvailableRAMSize = avail_pages * pagesize >> 10;
#endif

	/*
	 * Get IP address
	 */
	/* nbr of valid IPAgent strings */
	n_ip = agent_getnetconfcount();

	ipaddrs = NULL;
	if(n_ip > 0) {
		if ((ipaddrs = (u_int*)malloc(n_ip * sizeof(u_int))) == NULL) {
			logout(11, F_ftd_mngt_gather_server_info, "malloc failed.\n");
		}
	} else {
		logout(11, F_ftd_mngt_gather_server_info, "agent_getnetconfcount failed.\n");
	}
        /* set ipstr to NULL */
	memset(ipstr, 0, sizeof(ipstr));
	if(ipaddrs != NULL) {
		if (n_ip == agent_getnetconfs(ipaddrs)) {

			/* if Agent IP set ? Yes, make it the first element.
			 * work around Windows collector BUG.
			 */

		    int u = (int)giTDMFAgentIP.Version;
			
		   	if (is_unspecified(giTDMFAgentIP) != 1) {
/* TODO NAA - This should be cleaned up */
#if defined(linux)
	   		ip_to_ipstring(giTDMFAgentIP, ipstr);
#else
			   	if(u == IPVER_4)
				{
					ip_to_ipstring_uint(giTDMFAgentIP.Addr.V4, ipstr);
				}
				else
			   	{
			   		ip_to_ipstring(giTDMFAgentIP, ipstr);
			   	}
#endif
#if defined(FTD_IPV4)
	strcpy(srvrInfo->IPAgent[0], ipstr);	
#else
  	#if defined(linux)
   		token_ipv6 = strtok(ipstr,"%");
		if(token_ipv6 != NULL)
			strcpy(srvrInfo->IPAgent[0], token_ipv6);
		else
		   strcpy(srvrInfo->IPAgent[0], ipstr);
	#else
	   strcpy(srvrInfo->IPAgent[0], ipstr);
	   
	#endif 	

#endif


				j = 1;

		 	} else {
		 		j = 0;
		 	}
			 /*copy from j=1 if AgentIP is set in config file */
			for (i = 0; i < n_ip && j < N_MAX_IP ; i++) {
				if (ipaddrs[i] != giTDMFAgentIP.Addr.V4 && ipaddrs[i] != 0
						&& ipaddrs[i] != LOOPBACKIP) {
					ip_to_ipstring_uint(ipaddrs[i], ipstr);
					if (strcmp(ipstr,srvrInfo->IPAgent[0]) != 0)
					{
						logoutx(17, F_ftd_mngt_gather_server_info, "get ip adress", ipstr, i);
						strcpy(srvrInfo->IPAgent[j++], ipstr);
					}
				}
			}
		} else {
			logout(4, F_ftd_mngt_gather_server_info, "IP adress is not suitable for count.\n");
		}
		free(ipaddrs);
	}

#if !defined(FTD_IPV4)
	if (n_ip < 4)
	{
		
		n_ip_v6 = agent_getnetconfcount6();

		if (n_ip_v6 > 0)
		{
			ipv6_addr = (char*) malloc ((sizeof(char)) * n_ip_v6 * INET6_ADDRSTRLEN);
			
			agent_getnetconfs6(ipv6_addr); 

			tmp_ipv6 = ipv6_addr;
			    
			for ( ; j < N_MAX_IP && k < n_ip_v6 ; k++) 
			{
				   if (strcmp(srvrInfo->IPAgent[0],tmp_ipv6) !=0) 
				   {
				   		pch = strchr(tmp_ipv6,'.');
						linkloc = strstr(tmp_ipv6, "fe80:"); 
						if( pch == NULL && linkloc == NULL) 
						{
				   			
				   			if(strcmp("::1",tmp_ipv6) !=0 )
				   			{
				   				strcpy(srvrInfo->IPAgent[j], tmp_ipv6);
								j++;
				   			}
				   		} 
				   }
				tmp_ipv6= tmp_ipv6 + INET6_ADDRSTRLEN;

			} 

		}


	}
#endif

	srvrInfo->IPCount = j; 
	for ( ; j < N_MAX_IP; j++) {
		srvrInfo->IPAgent[j][0] = 0;
	} 



	 /*if no specific Agent IP set, use the first IP address */
  	if (is_unspecified(giTDMFAgentIP) == 1)
  	{
  	    ipstring_to_ip(srvrInfo->IPAgent[0], &giTDMFAgentIP);
	    logout(11, F_ftd_mngt_gather_server_info, "IS  UNSPECIFIED.\n");
	} 
#if 1
	cnvport = Agent_port;				
	srvrInfo->AgentListenPort = (int)cnvport;

#else
	/* get it from /etc/services if there */
	if ((port = getservbyname("in."QNM, "tcp"))) {
		srvrInfo->AgentListenPort = ntohs(port->s_port);
	}
#endif
	/* 
	 * retrieve actual BAB size from the TDMF kernel device driver
         * open the master control device
	 */
	if ((ctlfd = open(FTD_CTLDEV, O_RDONLY)) < 0) {
		logoutx(4, F_ftd_mngt_gather_server_info, "open failed", "errno", errno);
	} else {
		if (FTD_IOCTL_CALL(ctlfd, FTD_GET_BAB_SIZE, &babinfo) < 0) {
			logoutx(4, F_ftd_mngt_gather_server_info, "ioctl FTD_GET_BAB_SIZE failed", "errno", errno);
		} else {
			/* convert to MB */
		   	srvrInfo->Features.BAB.ActualSize = babinfo.actual >> 20;
		}
		close(ctlfd);
	}																	
	num_chunks = 0;
	memset(buf, 0, sizeof(buf));
	if (cfg_get_key_value("num_chunks", buf, CFG_IS_NOT_STRINGVAL) == 0) {
		num_chunks = strtol(buf, NULL, 0);
	}

	chunk_size = 0;
	memset(buf, 0, sizeof(buf));
	if (cfg_get_key_value("chunk_size", buf, CFG_IS_NOT_STRINGVAL) == 0) {
		chunk_size = strtol(buf, NULL, 0);
	}
	if (num_chunks && chunk_size) {
		srvrInfo->Features.BAB.RequestedSize = ((long long)chunk_size * num_chunks) >> 20;
	}

	memset(buf, 0, sizeof(buf));
	if ( cfg_get_key_value("tcp_window_size",buf,
					CFG_IS_NOT_STRINGVAL) == 0 )
	{
		tmp = atoi(buf);
        } else {
		/* Use default value */
		tmp = DEFAULT_TCP_WINDOW_SIZE;
	}

	/* Bytes to KBytes */
	srvrInfo->TCPWindowSize = tmp >> 10;
   
	tmp = 0;
	memset(version, 0, sizeof(version));
	getversion(version, sizeof version);
	tmp = sscanf(version, "%s %s", ver, build);
  	srvrInfo->ProductVersion[0] = 0;
	/* Send also the short Product name to the Collector (allows defining cetain default parameters (ex: journals ON/OFF)
	   differently depending on the target brand (RFX vs TUIP) on the Collector/Console side) (pc071214). */
  	if (tmp == 1)
	{
		sprintf(srvrInfo->ProductVersion, "%s%s %s", "Version ", ver, PRODUCTNAME_SHORT);
	}
	else
	{
		sprintf(srvrInfo->ProductVersion, "%s%s%s%s %s", "Version ", ver, " Build ", build, PRODUCTNAME_SHORT);
	}

	srvrInfo->InstallPath[0] = 0;
	cfg_get_software_key_value(INSTALLPATH, srvrInfo->InstallPath, sizeof(srvrInfo->InstallPath));

	srvrInfo->Features.PStore.Path[0]  = 0;
	srvrInfo->Features.Journal.Path[0] = 0;
	if (srvrInfo->InstallPath[0] != 0) {
		strcpy(srvrInfo->Features.PStore.Path, srvrInfo->InstallPath);
		strcpy(srvrInfo->Features.Journal.Path, srvrInfo->InstallPath);
		if (srvrInfo->InstallPath[strlen(srvrInfo->InstallPath) - 1] != '/') {
			strcat(srvrInfo->Features.PStore.Path,  "/");
			strcat(srvrInfo->Features.Journal.Path, "/");
		}
		strcat(srvrInfo->Features.PStore.Path, "PStore");
		strcat(srvrInfo->Features.Journal.Path, "Journal");
	}  
	/* Features supported now in groups and future ones - all added from "SERVERINFO2_VERSION 1" to "SERVERINFO2_VERSION 4 " */
    srvrInfo->Features.Group.DynamicInsertion = 1;
	srvrInfo->Features.Group.AutoStart = 1;
	srvrInfo->Features.Group.ThinCopy.MirrorState = 0;
	srvrInfo->Features.Group.ThinCopy.NonMirrorState = 0;

	srvrInfo->Features.BAB.OptimizedRecording.State = 0;
	srvrInfo->Features.BAB.OptimizedRecording.HighThreshold = 0;
	srvrInfo->Features.BAB.OptimizedRecording.LowThreshold = 0;
	srvrInfo->Features.BAB.OptimizedRecording.CollisionDetectionState = 0;
	srvrInfo->Features.BAB.State = 1;
	srvrInfo->Features.BAB.DynamicAllocationState = 0;

	srvrInfo->Features.PStore.LowResolutionBitmap.PersistencyState = 0;
	srvrInfo->Features.PStore.LowResolutionBitmap.State = 1;
	srvrInfo->Features.PStore.LowResolutionBitmap.Size = 0;
	srvrInfo->Features.PStore.HighResolutionBitmap.State = 1;
	srvrInfo->Features.PStore.HighResolutionBitmap.Size = 0;

	srvrInfo->Features.Journal.State = 1;

	srvrInfo->Features.Cluster.State = 1;
}
#endif /* _FTD_MNGT_GATER_SERVER_INFO_C_ */
