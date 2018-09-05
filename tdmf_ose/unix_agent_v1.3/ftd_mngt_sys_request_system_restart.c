/* #ident "@(#)$Id: ftd_mngt_sys_request_system_restart.c,v 1.15 2003/11/17 10:58:02 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_SYS_REQUEST_SYSTEM_RESTART_C_
#define _FTD_MNGT_SYS_REQUEST_SYSTEM_RESTART_C_


#if defined(HPUX)
	int     reboot = 1;
#endif	/* HPUX */


void
ftd_bab_initialize(int babsize)
{
	char	buf[MAXPATHLEN];
	char	cmdline[256];
#if defined(_AIX)
	char	line[16];
	FILE	*f;
#endif	/* _AIX */
#if defined(HPUX)
	struct stat  shstat;
#endif	/* HPUX */
#if defined(linux)
	char	line[16];
	FILE	*f;
#endif	/* linux */

	if (cfg_get_key_value("tcp_window_size", buf, CFG_IS_NOT_STRINGVAL) != 0) {
		/* wasn't there, just default to max value */
		sprintf(buf, "%d", 256 * 1024);
		if (cfg_set_key_value("tcp_window_size", buf,
                                               CFG_IS_NOT_STRINGVAL) != 0) {
			logout(4, F_ftd_bab_initialize, "TCP Window size initialization failed.\n");
		}
		logout(12, F_ftd_bab_initialize, "tcp_window_size OK.\n");
	}

	if (cfg_get_key_value("num_chunks", buf, CFG_IS_NOT_STRINGVAL) == 0 && 
	    cfg_get_key_value("chunk_size", buf, CFG_IS_NOT_STRINGVAL) == 0) {
		logout(12, F_ftd_bab_initialize, "num_chunks & chunk_size OK.\n");
		/* already bab was initialized, and driver was add. */
#if defined(HPUX)
		if ((get_OSversion() >= DLKM_OS_VER) && (check_dlkm_version() == true)) {
			memset(&shstat, 0x00, sizeof(shstat));
			if (stat(FTD_CTLDEV, &shstat) != 0) {
				memset(cmdline, 0, sizeof(cmdline));
				sprintf(cmdline, "/etc/mksf -d %s -m 0xffffff -r %s", QNM,FTD_CTLDEV);
				system(cmdline);
			}
		}
#endif	/* HPUX */
		return;
	}

	memset(cmdline, 0, sizeof(cmdline));
#if defined(_AIX)
	/* Make sure that odmadd has happened */
	sprintf(cmdline, "/usr/sbin/lsdev -P | grep "QNM" 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_bab_initialize, "Cannot initialize BAB.\nDTC driver is not included in the Predefined Devices object class. This may be a result of an incomplete or failed installation.\nYou should remove and re-install Softek Replicator to resolve this problem.\n");
		return;
	}
	/* Make sure that defdtc has happened */
	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "/usr/sbin/lsdev -C | grep "QNM"0 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_bab_initialize, "Cannot initialize BAB.\nDTC driver is not included in the Customized Devices object class. This may be a result of an incomplete or failed installation.\nYou should remove and re-install Softek Replicator to resolve this problem.\n");
		return;
	}
	sprintf(cmdline, "/usr/sbin/lsdev -C | grep "QNM"0 2> /dev/null | awk '{print $2}'");
	if ((f = popen(cmdline, "r")) == NULL) {
		logoutx(4, F_ftd_bab_initialize, "popen failed", "errno", errno);
		return;
	}
	if (fgets(line, 16, f) != NULL) {
		if (strncmp("Available", line, 9) == 0) {
			/* Driver has already attached */
			pclose(f);
			logout(4, F_ftd_bab_initialize, "The DTC Driver has already been attached, you must first stop all logical groups, then unattach the driver using the following command:\n     /usr/lib/methods/ucfg"QNM" -l "QNM"0");
			return;
		}
	}
	pclose(f);
#endif	/* linux */

	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "%s/"QNM"init -b %d 2> /dev/null", DTCCMDDIR, babsize);
	if (system(cmdline) != 0) {
		logout(4, F_ftd_bab_initialize, "BAB initialization failed.\n");
		return;
	}

#if defined(SOLARIS)
	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "/usr/sbin/add_drv "QNM" 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_bab_initialize, "An error occurred during an attempt to load the driver.\nYou may try a manual configuration of the BAB.\n");
		return;
	}

#elif defined(HPUX)

	/* check hp-ux os version */
	if (get_OSversion() < DLKM_OS_VER)
	{
		/* kernel rebuild is dtcinit command */
	}
	else 
	{
		/* check install tdmf version */

		/* OS version 11.00 or 11i */
		memset(cmdline, 0, sizeof(cmdline));
		sprintf(cmdline, "/usr/sbin/kmtune -s ftd_num_chunk=%d 2> /dev/null", babsize);
		if (system(cmdline) != 0) {
			logout(4, F_ftd_bab_initialize, "kmtune failed. You will have to kmtune the "QNM" module manually.\n");
			return;
		}
		/***********************************/
		/* DLKM function                   */
		if (check_dlkm_version() == true) {
			if (stat(FTD_CTLDEV, &shstat) != 0) {
				memset(cmdline, 0, sizeof(cmdline));
				sprintf(cmdline, "/etc/mksf -d %s -m 0xffffff -r %s", QNM,FTD_CTLDEV);
				system(cmdline);
			}
			memset(cmdline, 0, sizeof(cmdline));
			sprintf(cmdline, "/usr/sbin/kmadmin -L "QNM" 2> /dev/null");
			if (system(cmdline) != 0) {
				logout(4, F_ftd_bab_initialize, "kmadmin failed. You will have to reload the "QNM" module manually.\n");
				return;
			}
		}
	}

	if ((get_OSversion() < DLKM_OS_VER) || (check_dlkm_version() == false)) {
		/* You must ask user to reboot manually or auto? */
		if (reboot == 0) {
			logout(4, F_ftd_bab_initialize, "You will need to reboot the system manually for these settings to take effect.\n");
		} else {
			chdir("/");
			system("/etc/shutdown  -y -r 60");
			return;
		}
	}

#elif defined(_AIX)
	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "/usr/lib/methods/cfg"QNM" -l "QNM"0 -2 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_bab_initialize, "An error occurred during an attempt to load the driver.\nYou may try a manual configuration of the BAB.\n");
		return;
	}
#elif defined(linux)
	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "/sbin/depmod -a >& /dev/null");
	system(cmdline);
	sprintf(cmdline, "/sbin/modprobe "MODULES_NAME" 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_bab_initialize, "An error occurred during an attempt to load the driver.\nYou may try a manual configuration of the BAB.\n");
		return;
	}
	sprintf(cmdline, "/bin/mkdir /dev/"QNM" >& /dev/null");
	system(cmdline);
	sprintf(cmdline, "/bin/mknod -m 640 /dev/"QNM"/ctl b 254 255 >& /dev/null");
	system(cmdline);
	sprintf(cmdline, "/%s/in."QNM" >& /dev/null", DTCCMDDIR);
	system(cmdline);
#endif
	logout(9, F_ftd_bab_initialize, "Driver loaded successfully.\n");
	return;
}

void
ftd_system_restarting(int babsize)
{
	char	cmdline[256];
	int	failed = 0;
	int dtcmasterstate = 0;
#if defined(HPUX)
	struct stat  shstat;
#endif	/* HPUX */

	memset(cmdline, 0, sizeof(cmdline));
#if defined(_AIX)
	/* Make sure that odmadd has happened */
	sprintf(cmdline, "/usr/sbin/lsdev -P | grep "QNM" 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_system_restarting, "Cannot initialize BAB.\nDTC driver is not included in the Predefined Devices object class. This may be a result of an incomplete or failed installation.\nYou should remove and re-install Softek Replicator to resolve this problem.\n");
		return;
	}
	/* Make sure that defdtc has happened */
	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "/usr/sbin/lsdev -C | grep "QNM"0 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_system_restarting, "Cannot initialize BAB.\nDTC driver is not included in the Customized Devices object class. This may be a result of an incomplete or failed installation.\nYou should remove and re-install Softek Replicator to resolve this problem.\n");
		return;
	}

#elif defined(HPUX)
	/* check hp-ux os version */
	if ((get_OSversion() >= DLKM_OS_VER) && (check_dlkm_version() == true)) {
		/* Warn about the driver reload */
		logout(4, F_ftd_system_restarting, "For this change to take effect, the driver will be reloaded.\nBefore proceeding, shut down any processes that may be accessing DTC devices, unmount any file systems that reside on DTC devices, and stop all logical groups using "QNM"stop.\nAll DTC daemons will be temporarily shut down while the reload is taking place.\n");
	}
#endif	/* HPUX */

#if defined(SOLARIS) || defined(_AIX) || defined(linux)
	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "%s/"QNM"init -b %d 2> /dev/null", DTCCMDDIR, babsize);
	if (system(cmdline) != 0) {
		sprintf(logmsg, "BAB Initialization failed. \nYou may execute the %s/"QNM"init -b %d command manually to configure the BAB.\n", DTCCMDDIR, babsize);
		logout(4, F_ftd_system_restarting, logmsg);
		return;
	}
#elif defined(HPUX)
	if ((get_OSversion() < DLKM_OS_VER) || (check_dlkm_version() == false)) {
		memset(cmdline, 0, sizeof(cmdline));
		sprintf(cmdline, "%s/"QNM"init -b %d 2> /dev/null", DTCCMDDIR, babsize);
		if (system(cmdline) != 0) {
			sprintf(logmsg, "BAB Initialization failed. \nYou may execute the %s/"QNM"init -b %d command manually to configure the BAB.\n", DTCCMDDIR, babsize);
			logout(4, F_ftd_system_restarting, logmsg);
			return;
		}
	}
#endif

	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "/bin/ps -e | /bin/grep in."QNM" | /bin/grep -v grep 2> /dev/null");
	if ((dtcmasterstate = system(cmdline)) == 0) {
		memset(cmdline, 0, sizeof(cmdline));
		sprintf(cmdline, "%s/kill"QNM"master > /dev/null 2>&1", DTCCMDDIR);
		if (system(cmdline) != 0) {
			logout(4, F_ftd_system_restarting, "Stop "QNM"master failed.\n");
			return;
		}
	}

#if defined(HPUX)
	if ((get_OSversion() >= DLKM_OS_VER) && (check_dlkm_version() == true)) {
		memset(cmdline, 0, sizeof(cmdline));
		sprintf(cmdline, "%s/"QNM"init -b %d 2> /dev/null", DTCCMDDIR, babsize);
		if (system(cmdline) != 0) {
			sprintf(logmsg, "BAB Initialization failed. \nYou may execute the %s/"QNM"init -b %d command manually to configure the BAB.\n", DTCCMDDIR, babsize);
			logout(4, F_ftd_system_restarting, logmsg);
			if (dtcmasterstate == 0) {
				memset(cmdline, 0, sizeof(cmdline));
				sprintf(cmdline, "%s/launch"QNM"master > /dev/null 2>&1", DTCCMDDIR);
				system(cmdline);
			}
			return;
		}
	}
#endif

#if defined(HPUX)
	if ((get_OSversion() < DLKM_OS_VER) || (check_dlkm_version() == false)) {
		chdir("/");
		system("/etc/shutdown  -y -r 60");
		return;
	}
#endif

#if defined(SOLARIS)
	/* Remove driver */
	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "/usr/sbin/rem_drv "QNM" 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_system_restarting, "Driver remove failed.\n");
		failed = 1;
	} else {
		/* attach driver */
		memset(cmdline, 0, sizeof(cmdline));
		sprintf(cmdline, "/usr/sbin/add_drv "QNM" 2> /dev/null");
		if (system(cmdline) != 0) {
			logout(4, F_ftd_system_restarting, "Driver load failed.\n");
			failed = 1;
		}
	}
#elif defined(HPUX)
	/* Remove driver */
	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "/usr/sbin/kmadmin -U "QNM" 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_system_restarting, "Driver remove failed.\n");
		failed = 1;
	} else {
		memset(&shstat, 0x00, sizeof(shstat));
		if (stat(FTD_CTLDEV, &shstat) != 0) {
			memset(cmdline, 0, sizeof(cmdline));
			sprintf(cmdline, "/etc/mksf -d %s -m 0xffffff -r %s", QNM,FTD_CTLDEV);
			system(cmdline);
		}
		/* attach driver */
		memset(cmdline, 0, sizeof(cmdline));
		sprintf(cmdline, "/usr/sbin/kmadmin -L "QNM" 2> /dev/null");
		if (system(cmdline) != 0) {
			logout(4, F_ftd_system_restarting, "Driver load failed.\n");
			failed = 1;
		}
	}
#elif defined(_AIX)
	/* Driver has already attached, remove it */
	sprintf(cmdline, "/usr/lib/methods/ucfg"QNM" -l "QNM"0 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_system_restarting, "Driver unconfigure failed.\n");
		failed = 1;
	} 
	/* attach driver */
	if (!failed) {
		sprintf(cmdline, "/usr/lib/methods/cfg"QNM" -l "QNM"0 2> /dev/null");
		if (system(cmdline) != 0) {
			logout(4, F_ftd_system_restarting, "Driver configure failed.\n");
			failed = 1;
		}
	}
#elif defined(linux)
	sprintf(cmdline, "/sbin/depmod -a 2> /dev/null");
	system(cmdline);
	/* Driver has already attached, remove it */
	sprintf(cmdline, "/sbin/rmmod "MODULES_NAME" 2> /dev/null");
	if (system(cmdline) != 0) {
		logout(4, F_ftd_system_restarting, "Driver remove failed.\n");
		failed = 1;
	} 
	/* attach driver */
	if (!failed) {
		sprintf(cmdline, "/sbin/modprobe "MODULES_NAME" 2> /dev/null");
		if (system(cmdline) != 0) {
			logout(4, F_ftd_system_restarting, "Driver load failed.\n");
			failed = 1;
		}
	}
#endif
	if (dtcmasterstate == 0) {
		memset(cmdline, 0, sizeof(cmdline));
		sprintf(cmdline, "%s/launch"QNM"master > /dev/null 2>&1", DTCCMDDIR);
		if (system(cmdline) != 0) {
			logout(4, F_ftd_system_restarting, "Start "QNM"master failed.\n");
			return;
		}
	}
	if (failed) {
#if defined(SOLARIS)
		logout(4, F_ftd_system_restarting, "Errors occurred during an attempt to reload the driver. \nThis is most likely because some DTC devices are in use. \nYou will need to rectify the problem, then manually reload the driver using the rem_drv and add_drv commands\n");
		return;
#elif defined(HPUX)
		logout(4, F_ftd_system_restarting, "Errors occurred during an attempt to reload the driver. \nThis is most likely because some DTC devices are in use. \nYou will need to rectify the problem, then manually reload the driver using the kmadmin -U and kmadmin -L commands\n");
		return;
#elif defined(_AIX)
		logout(4, F_ftd_system_restarting, "Errors occurred during an attempt to reload the driver.\nThis is most likely because some DTC devices are in use.\nYou will need to rectify the problem, then manually detach the driver by issueing /usr/lib/methods/ucfg"QNM" -l "QNM"0 after stopping master daemon.\nAnd, manually attach the driver by issueing /usr/lib/methods/cfg"QNM" -l "QNM"0 -2, then restart master daemon.\n");
		return;
#elif defined(linux)
		logout(4, F_ftd_system_restarting, "Errors occured during an attempt to reload the driver. This is most likely because some DTC devices are in use. You will need to reactify the problem, then manually reload the driver using rmmod and modprobe commands.");
		return;
#endif
	} else {
		logout(9, F_ftd_system_restarting, "Driver reloaded successfully.\n");
		return;
	}
}

void
ftd_master_restarting()
{
	char	cmdline[256];

	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "%s/kill"QNM"master > /dev/null 2>&1", DTCCMDDIR);
	if (system(cmdline) != 0) {
		logout(4, F_ftd_master_restarting, "Stop "QNM"master failed.\n");
		return;
	}
	memset(cmdline, 0, sizeof(cmdline));
	sprintf(cmdline, "%s/launch"QNM"master > /dev/null 2>&1", DTCCMDDIR);
	if (system(cmdline) != 0) {
		logout(4, F_ftd_master_restarting, "Start "QNM"master failed.\n");
		return;
	}
}

void
ftd_mngt_sys_request_system_restart(mmp_TdmfServerInfo *sip, int flag)
{
	if (flag & GENE_CFG_BABSIZE) {
		ftd_system_restarting(sip->iBABSizeReq);
	} else if ((flag & GENE_CFG_WINDOWSIZE) || (flag & GENE_CFG_PORT)) {
		ftd_master_restarting();
	}
	return;
}
#endif /* _FTD_MNGT_SYS_REQUEST_SYSTEM_RESTART_C_ */

#if defined(HPUX)
int
check_dlkm_version()
{
	int	version_num;

	if ((version_num = getversion_num()) == 0) {
		logout(4, F_ftd_mngt_sys_request_system_restart, "getversion_num() return 0.\n");
		return false;
	}
	sprintf(logmsg, "Replicator version = %d\n", version_num);
	logout(17, F_ftd_mngt_sys_request_system_restart, logmsg);
	if (version_num >= DLKM_TDMF_VER) {
		return true;
	} else {
		return false;
	}
}

int
get_OSversion()
{
	static int iosver = 0;
	struct utsname  utsname;
	char OSversion[50];
	char *s;
	char v1[1];
	int  v2=0, v3=0;

	if (iosver != 0) {
		return iosver;
	}
	if (uname(&utsname) == -1) {
		return 0;
	}
	memset(OSversion, 0, sizeof(OSversion));
	memcpy(OSversion, utsname.release, UTSLEN);
	if ((s = (char *)strtok(OSversion, ".\n")))
		strcpy(v1, s);
	if ((s = (char *)strtok(NULL, ".\n")))
		v2 = strtol(s, NULL, 0);
	if ((s = (char *)strtok(NULL, ".\n")))
		v3 = strtol(s, NULL, 0);

	if (strcmp(v1, "B") == 0) {
		if (v2 != 0 || v3 != 0) {
			iosver = (v2 * 100) + v3;
		} else {
			iosver = 1020;
		}
	} else {
		iosver = 1020;
	}
	sprintf(logmsg, "OS version = %d\n", iosver);
	logout(17, F_ftd_mngt_sys_request_system_restart, logmsg);
	return iosver;
}
#endif	/* HPUX */
