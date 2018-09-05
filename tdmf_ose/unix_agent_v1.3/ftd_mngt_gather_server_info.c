/* #ident "@(#)$Id: ftd_mngt_gather_server_info.c,v 1.33 2004/05/30 05:08:29 jqwn0 Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_GATER_SERVER_INFO_C_
#define _FTD_MNGT_GATER_SERVER_INFO_C_

#if defined(HPUX)
#include <sys/param.h>
#include <sys/pstat.h>

int iOSversion;

#elif defined(linux)
#include <sys/param.h>

#endif


extern void execCommand(char *, char *);

void
getversion(char *vbuf)
{
	char	cmdline[256];
	FILE	*f;

	sprintf(cmdline, "%s/dtcinfo -v 2> /dev/null | awk '{print $4,$6}'", DTCCMDDIR);
	if ((f = popen(cmdline, "r")) != NULL) {
		if (fgets(vbuf, 16, f) == NULL) {
			logout(4, F_getversion, "cannot get version info.\n");
		}
		pclose(f);
	} else {
		logoutx(4, F_getversion, "popen failed", "errno", errno);
	}
	return;
}

void
ftd_mngt_gather_server_info(mmp_TdmfServerInfo *srvrInfo)
{
	struct utsname	utsname;
#if defined(HPUX)
	struct pst_static pst;
#endif
	int	error;
	long long	pagesize, phys_pages, avail_pages;
	int	i;
	int	j;
	struct servent  *port;
	int ctlfd;
	int tmp;
	int tmplen;
	int sock;
	int n_ip = 0;
	unsigned short cnvport;
	char buf[MAXPATHLEN];
	char version[16];
	char ver[8], build[8];
	int num_chunks, chunk_size;
	u_long *ipaddrs;
	char ipstr[16];
	ftd_babinfo_t babinfo;
#if defined(_AIX)
	char cmdline[256];
	FILE *f;
#elif defined(HPUX)
	struct pst_dynamic pstdyn;
#endif

	memset((char *)srvrInfo, 0, sizeof(mmp_TdmfServerInfo));

	strcpy(srvrInfo->szServerUID, gszServerUID);

	if (uname(&utsname) == -1) {
		logoutx(4, F_ftd_mngt_gather_server_info, "uname failed", "errno", errno);
	} else {
		strcpy(srvrInfo->szMachineName, utsname.nodename);
#if defined(SOLARIS)
		if (strcmp(utsname.sysname, "SunOS") == 0) {
			strcpy(srvrInfo->szOsType, "Solaris");
			if (strcmp(utsname.release, "5.7") == 0) {
				strcpy(srvrInfo->szOsVersion, "7");
			} else if (strcmp(utsname.release, "5.8") == 0) {
				strcpy(srvrInfo->szOsVersion, "8");
			} else if (strcmp(utsname.release, "5.9") == 0) {
				strcpy(srvrInfo->szOsVersion, "9");
			} else if (strcmp(utsname.release, "5.6") == 0) {
				strcpy(srvrInfo->szOsVersion, "2.6");
			} else {
				strcpy(srvrInfo->szOsVersion, utsname.release);
			}
		} else {
			strcpy(srvrInfo->szOsType, utsname.sysname);
			strcpy(srvrInfo->szOsVersion, utsname.release);
		}
#elif defined(_AIX)
		strcpy(srvrInfo->szOsType, utsname.sysname);
		sprintf(buf,"%s.%s",utsname.version,utsname.release);
		if (strlen(buf) > MMP_MNGT_MAX_OS_VERSION_SZ)
			srvrInfo->szOsVersion[0] = '\0';
		else
			strcpy(srvrInfo->szOsVersion,buf);
#elif defined(HPUX)
		strcpy(srvrInfo->szOsType, utsname.sysname);
		memcpy(srvrInfo->szOsVersion, utsname.release, UTSLEN);
#elif defined(linux)
		strcpy(srvrInfo->szOsType, utsname.sysname);
		strcpy(srvrInfo->szOsVersion, utsname.release);
#endif
	}

	strcpy(srvrInfo->szIPRouter, NULL_IP_STR);
	if (cfg_get_software_key_value("domain",
				 srvrInfo->szTDMFDomain, sizeof(srvrInfo->szTDMFDomain)) != 0) {
		strcpy(srvrInfo->szTDMFDomain, DEFAULT_TDMF_DOMAIN_NAME);
	}

	/* Get cpus */
#if defined(SOLARIS) || defined(linux)
	srvrInfo->iNbrCPU = sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_AIX)
	sprintf(cmdline, "/usr/sbin/lsdev -C -c processor -s sys | wc -l 2> /dev/null");
	if ((f = popen(cmdline, "r")) == NULL) {
		logoutx(4, F_ftd_mngt_gather_server_info, "popen failed", "errno", errno);
	} else {
		if (fgets(buf, 80, f) == NULL) {
			pclose(f);
		} else {
			srvrInfo->iNbrCPU = strtol(buf, NULL, 10);
		}
	}
#elif defined(HPUX)
	pstat_getdynamic(&pstdyn, sizeof(pstdyn), 1, 0);
	srvrInfo->iNbrCPU = pstdyn.psd_proc_cnt;
#endif

	/*
	 * Get memory information from the system.
	 * The 10-bit's shift changes the unit from byte to Kbyte. 
	 */
#if defined(SOLARIS)
	pagesize = _sysconfig(_CONFIG_PAGESIZE);
	phys_pages = _sysconfig(_CONFIG_PHYS_PAGES);
	avail_pages = _sysconfig(_CONFIG_AVPHYS_PAGES);
	srvrInfo->iRAMSize = phys_pages * pagesize >> 10;
	srvrInfo->iAvailableRAMSize = avail_pages * pagesize >> 10;
#elif defined(_AIX)
	execCommand("bootinfo -r", buf);
	phys_pages = atol(buf);
	srvrInfo->iRAMSize = phys_pages;
	srvrInfo->iAvailableRAMSize = 0;
#elif defined(HPUX)
	if ( (pstat_getstatic(&pst, sizeof(pst), (size_t) 1, 0)) < 0) {
		srvrInfo->iRAMSize = 0;
		logout(4, F_ftd_mngt_gather_server_info, "pstat_getstatic failed.\n");
	} else {
		pagesize = pst.page_size;
		phys_pages = pst.physical_memory;
		srvrInfo->iRAMSize = phys_pages * pagesize >> 10;
	}
	srvrInfo->iAvailableRAMSize = 0;
#elif defined(linux)
	pagesize = sysconf(_SC_PAGESIZE);
	phys_pages = sysconf(_SC_PHYS_PAGES);
	avail_pages = sysconf(_SC_AVPHYS_PAGES);
	srvrInfo->iRAMSize = phys_pages * pagesize >> 10;
	srvrInfo->iAvailableRAMSize = avail_pages * pagesize >> 10;
#endif

	/*
	 * Get IP address
	 */

	/* nbr of valid szIPAgent strings */
	n_ip = getnetconfcount();

	ipaddrs = NULL;
	if(n_ip > 0) {
		if ((ipaddrs = (u_long*)malloc(n_ip * sizeof(u_long))) == NULL) {
			logout(11, F_ftd_mngt_gather_server_info, "malloc failed.\n");
		}
	} else {
		logout(11, F_ftd_mngt_gather_server_info, "getnetconfcount failed.\n");
	}

	/* set ipstr to NULL */
	memset(ipstr, 0, sizeof(ipstr));
	if(ipaddrs != NULL) {
		if (n_ip == getnetconfs(ipaddrs)) {

			/* if Agent IP set ? Yes, make it the first element.
			 * work around Windows collector BUG.
			 */
			if (giTDMFAgentIP != 0) {
				ip_to_ipstring(giTDMFAgentIP, ipstr);
				strcpy(srvrInfo->szIPAgent[0], ipstr);
				j = 1;

				sprintf(logmsg, "Agent IP address is set!!  Agent IP=(0x%x) %s\n", giTDMFAgentIP, ipstr);
				logout(6, F_ftd_mngt_gather_server_info, logmsg);

			} else {
				j = 0;
			}

			/* copy from j=1 if AgentIP is set in config file */
			for (i=0; i < n_ip && j < N_MAX_IP ; i++) {
				if (ipaddrs[i] != giTDMFAgentIP && ipaddrs[i] != 0
						&& ipaddrs[i] != LOOPBACKIP) {
					ip_to_ipstring(ipaddrs[i], ipstr);
					logoutx(17, F_ftd_mngt_gather_server_info, "get ip address", ipstr, i);
					strcpy(srvrInfo->szIPAgent[j++], ipstr);
				}
			}
			srvrInfo->ucNbrIP = j;
			for ( ; j < N_MAX_IP; j++) {
				srvrInfo->szIPAgent[j][0] = 0;
			}
		} else {
			logout(11, F_ftd_mngt_gather_server_info, "IP address is not suitable for count.\n");
		}
		free(ipaddrs);
	}

	/* if no specific Agent IP set, use the first IP address */
	if (giTDMFAgentIP != 0)
	    ipstring_to_ip(srvrInfo->szIPAgent, (unsigned long *)&giTDMFAgentIP);
#if 1
	cnvport = Agent_port;
	rv2((char *)&cnvport);
	srvrInfo->iPort = (int)cnvport;

#else
	/* get it from /etc/services if there */
	if ((port = getservbyname("in.dtc", "tcp"))) {
		srvrInfo->iPort = ntohs(port->s_port);
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
			srvrInfo->iBABSizeAct = babinfo.actual >> 20;
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
	srvrInfo->iBABSizeReq = ((long long)chunk_size * num_chunks) >> 20;

        memset(buf, 0, sizeof(buf));
	if ( cfg_get_key_value("tcp_window_size",buf,
					CFG_IS_NOT_STRINGVAL) == 0 )
	{
		tmp = atoi(buf);
        } else {
		/* fixed 256KB */
		tmp = 256 * 1024;
	}

	/* Bytes to KBytes */
	srvrInfo->iTCPWindowSize = tmp >> 10;

	tmp = 0;
	memset(version, 0, sizeof(version));
	getversion(version);
	tmp = sscanf(version, "%s %s", ver, build);
	srvrInfo->szTdmfVersion[0] = 0;
	if (tmp == 1)
	{
		sprintf(srvrInfo->szTdmfVersion, "%s%s", "Replicator Version ", ver);
	}
	else
	{
		sprintf(srvrInfo->szTdmfVersion, "%s%s%s%s", "Replicator Version ", ver, " Build ", build);
	}

	srvrInfo->szTdmfPath[0] = 0;
	cfg_get_software_key_value("InstallPath", srvrInfo->szTdmfPath, sizeof(srvrInfo->szTdmfPath));

	srvrInfo->szTdmfPStorePath[0]  = 0;
	srvrInfo->szTdmfJournalPath[0] = 0;
	if (srvrInfo->szTdmfPath[0] != 0) {
		strcpy(srvrInfo->szTdmfPStorePath, srvrInfo->szTdmfPath);
		strcpy(srvrInfo->szTdmfJournalPath, srvrInfo->szTdmfPath);
		if (srvrInfo->szTdmfPath[strlen(srvrInfo->szTdmfPath) - 1] != '/') {
			strcat(srvrInfo->szTdmfPStorePath,  "/");
			strcat(srvrInfo->szTdmfJournalPath, "/");
		}
		strcat(srvrInfo->szTdmfPStorePath, "PStore");
		strcat(srvrInfo->szTdmfJournalPath, "Journal");
	}
}
#endif /* _FTD_MNGT_GATER_SERVER_INFO_C_ */
