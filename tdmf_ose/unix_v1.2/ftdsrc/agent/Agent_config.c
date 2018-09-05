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
/* #ident "@(#)$Id: Agent_config.c,v 1.18 2014/06/30 18:05:56 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <values.h>
#include <unistd.h> 
#include <stdlib.h>
#include <string.h>

#include "tdmfAgent.h"
#include "tdmfAgent_trace.h"
#include "misc.h"

/* Agent_uty.c */

extern int name_is_ipstring(char *strip);
extern int cfg_set_software_key_value(char *key, char *data);
extern int cfg_get_software_key_value(char *key, char *data, int size);
extern int yesorno(char *message);
extern int survey_cfg_file(char *location, int cfg_file_type);
extern int cfg_get_key_value(char *key, char *value, int stringval);
extern int cfg_set_key_value(char *key, char *value, int stringval);


/* agent_msg.c */

extern void logout_failure(int loglvl,int func, char *logmsg);
extern void logout_success(int loglvl,int func, char *logmsg);

int config_file_edit(char *input_data, char select, int bab_mbsize, int transmit_interval_value, int listener_port_value);
int config_file_delete();
int config_file_list();
void migrate_agent_cfg_file(int prompt_user);
void migrate_lg_cfg_file(int prompt_user);
int get_migrate_dir(char *newestdir);
void program_exit(int code);

int Pid;

extern FILE *logfp;

static void usage(char *argv0)
{
	fprintf(stderr, "Usage: %s [ [-e CollectorIP[/CollectorPort]] [-i AgentIP] [-b bab_size] | [-d] | [-l] [-t TransmitInterval ] [-p listener_port ]]\n", argv0);
	fprintf(stderr, "       [-r 0 | 1 {if 0: do not recover previous config files and do not ask; if 1: recover previous config files without prompting the user}]\n" );
	exit(1);
}

static int ftd_strtol(char *str)
{
	char	*p;
	int	val;

	val = (int)strtol(str, &p, 10);
	if (errno == ERANGE || p == str || *p != '\0') {
		return (-1);
	}

	return (val);
}

int main(int argc,char *argv[])
{
	int edit = 0;
	int agentip = 0;
	int bab = 0;
	int disable = 0;
	int list = 0;
	int transmit_interval = 0;
	int listener_port = 0;
	int got_recover_flag = 0;
	char *input_data = NULL;
	char *agentip_data = NULL;
	char *bab_data;
	int  bab_mbsize = 0;
	int  transmit_interval_value = 0;
	int  listener_port_value = 0;
	char *p;
	int r =0;
	char msg_title[LOGMSG_SIZE];
	int lgmigrateasked = 0;
	int	recover_configs, do_not_recover_configs, recover_configs_no_prompt;
    int ch;

	/* Make sure we are root */
	if ( getuid() != 0 ) {
		fprintf(stderr, "You must be root to run this process...aborted\n");
		exit(1);
	}

	putenv("LANG=C");

    do_not_recover_configs = 0;	    // false by default
	recover_configs_no_prompt = 0;

	p = (char *)strrchr(argv[0], '/');
	if (p == NULL) {
		p = argv[0];
	} else {
		p++;
	}

	while ((ch = getopt(argc, argv, "r:e:i:dlht:b:p:"))!=EOF) {
		switch (ch) {
		case 'r':
		    got_recover_flag = 1;
			recover_configs = ftd_strtol(optarg);
			if( recover_configs == 0 )
			    do_not_recover_configs = 1; // True: do not call functions that recover old configs
			else
			    recover_configs_no_prompt = 1; // Recover old configs without prompting the user
			break;
		case 'e':
			edit++;
			input_data = optarg;
			break;
		case 'i':
			agentip++;
			agentip_data = optarg;
			break;
		case 'd':
			disable++;
			break;
		case 'l':
			list++;
			break;
		case 'b':
			/* primary server side setting BAB size in MB */
			/* secondary server does not need this parameter */
			bab++;
			bab_data = optarg;
			bab_mbsize = ftd_strtol(optarg);
			if (bab_mbsize < BAB_SIZE_MIN 
				|| bab_mbsize > BAB_SIZE_MAX) {
				fprintf(stderr, "Invalid value for (%d MB) BAB size.  Must be in (%d - %d) MB range.\n", bab_mbsize, BAB_SIZE_MIN, BAB_SIZE_MAX);
				usage(p);
				exit(1);
			}
			break;
		case 't':
			transmit_interval++;
			transmit_interval_value = ftd_strtol(optarg);
			if (transmit_interval_value < MMP_MNGT_DEFAULT_INTERVAL) {
				transmit_interval_value = MMP_MNGT_DEFAULT_INTERVAL;
				printf("  Transmit Interval can not be less than %d ! \n", transmit_interval_value);
			}
			if (transmit_interval_value > MMP_MNGT_MAX_INTERVAL) {
				transmit_interval_value = MMP_MNGT_MAX_INTERVAL;
				printf("  Transmit Interval can not be greater than %d ! \n", transmit_interval_value);
			}
			break;
		case 'p':
			listener_port++;
			listener_port_value = ftd_strtol(optarg);
			break;
		case 'h':						
		default:
			usage(p);
			break;
		}
	}

	if (optind != argc) {
		fprintf(stderr, "Invalid arguments.\n");
		usage(p);
		exit(1);
	}

	/* Make sure the specified options are consistent */
	/****************************************************************************************/
	/* Only one of options -r, -e (and/or -i, -b), -d and -l should be specified */
	if ((edit + disable + list) > 1 || (agentip + disable + list) > 1 
	    || (bab + disable + list) > 1 || (transmit_interval + disable + list) > 1
		|| (listener_port + disable + list) > 1
	    || ((edit || disable || list || bab || agentip || transmit_interval || listener_port) && (got_recover_flag && recover_configs_no_prompt)) ) {
		fprintf(stderr,"Error: Only one of options -r1, -e (and/or -i, -b, -p), -d and -l should be specified.\n");
		usage(p);
	}

    if( got_recover_flag )
	{
	    if( (recover_configs != 0) && (recover_configs != 1) )
		{
			fprintf(stderr,"Error: -r value can only be 0 or 1; parameter provided: %d.\n", recover_configs);
			usage(p);
		}
	}

	Pid = getpid();

	umask(00022);
	strcpy(msg_title, p);
	strcat(msg_title, ": ");
	if (edit > 0 || agentip > 0 || bab > 0 || transmit_interval > 0) {
		 strcat(msg_title, "Edit processing of Agent configuration file started.");
	} else if (disable > 0) {
		strcat(msg_title, "Delete processing of Agent configuration file started.");
	} else {
		strcat(msg_title, "Content display processing of Agent configuration file started.");
	}

	if (msg_init(msg_title) != 0) {
		exit(1);
	}

	/* only BAB size, transmit interval or listener port info is to be changed */
	if ((agentip == 0 && edit == 0 && bab > 0) || (agentip == 0 && edit == 0 && transmit_interval > 0) || (agentip == 0 && edit == 0 && listener_port > 0))
	{
        if( do_not_recover_configs == 0 )
            migrate_lg_cfg_file( 1 );
		
		r = config_file_edit(NULL, 'x', bab_mbsize, transmit_interval_value, listener_port_value);
		if (bab)
		{
                	if (r == 0) {
                        	logout_success(9, F_main, "BAB size processing ended normally.\n");
                	} else {
                        	logout_failure(4, F_main, "BAB size processing failed.\n");
                	}
		}

		if (transmit_interval)
		{
                	if (r == 0) {
                        	logout_success(9, F_main, "Transmit Interval processing ended normally.\n");
                	} else {
                        	logout_failure(4, F_main, "Transmit Interval processing failed.\n");
                	}
		}
		if (listener_port)
		{
		            if (r == 0) {
                        	logout_success(9, F_main, "Listener Port processing ended normally.\n");
                	} else {
                        	logout_failure(4, F_main, "Listener Port processing failed.\n");
                	}

		}
		goto main_end;
	}

    // Check if -r was specified to recover previous config files or to prevent it
	if( recover_configs_no_prompt )
	{
		migrate_agent_cfg_file( 0 );
		migrate_lg_cfg_file( 0 );
		r = config_file_list();
	}
	else
	{
		/* handle AgentIP option seperately */
		if (agentip > 0) {
			if( do_not_recover_configs == 0 ) migrate_lg_cfg_file( 1 );
			lgmigrateasked++;

			/* Agent IP configuration */
			r = config_file_edit(agentip_data, 'A', bab_mbsize, transmit_interval_value, listener_port_value);
			if (r == 0) {
				logout_success(9, F_main, "AgentIP processing ended normally.\n");
			} else {
				logout_failure(4, F_main, "Processing failed. Agent is not active.\n");
			}
		} 

		if (edit > 0) {
			if (lgmigrateasked == 0)	/* only ask LG migration question once */
				if( do_not_recover_configs == 0 ) migrate_lg_cfg_file( 1 );

			/* CollectorIP and port configuration */
			r = config_file_edit(input_data, 'C', bab_mbsize, transmit_interval_value, listener_port_value);
			if (r == 0) {
				logout_success(9, F_main, "Collector processing ended normally.\n");
			} else {
				logout_failure(4, F_main, "Processing failed. Agent is not active.\n");
			}
		} else if (disable > 0) {
			r = config_file_delete();

		} else if (agentip == 0 ) {
			/* -l or no option specified, migrate agent and LG config files (unless do_not_recover_configs is set) and show agent config info */
			if( do_not_recover_configs == 0 )
			{
			    migrate_agent_cfg_file( 1 );
			    migrate_lg_cfg_file( 1 );
			}
			r = config_file_list();
		}
	}

main_end:
	program_exit(r);
    // Never reached:
    return 0; // To avoid a compiler warning.
}

/* input_data: 	opt_arg from getopt function
 * select:	'C' for Collector parameters, 'A' for Agent parameters
 *		else for BAB size change
 * bab_mbsize:  BAB size in Mbytes
 */
int config_file_edit(char *input_data, char select, int bab_mbsize, int transmit_interval_value, int listener_port_value)
{
	char *p;
	char *ipaddr;
	char babstr[16];
	char ti_str[16];
	char lp_str[16];
	int i, port;
	char strport[AGN_CFG_PARM_SIZE];
	char logmsg[LOGMSG_SIZE];
	FILE *fp;

	if ((fp = fopen(AGN_CFGFILE, "a")) == NULL) {
		logoutx(4, F_config_file_edit, "fopen failed.\n", "errno", errno);
		return 1;
	}
	fclose(fp);

	if (select == 'C') {	/* Collector parameter setup */
	    p = (char *)strchr(input_data, '/');
	    if (p == NULL) {
		ipaddr = (char *)malloc(strlen(input_data) + 1);
		if (ipaddr == NULL) {
			logoutx(4, F_config_file_edit, "malloc failed.", "errno", errno);
			return 1;
		}
		strcpy(ipaddr, input_data);
	    } else {
		i = p - input_data;
	 	while((*p == '/') || (*p == ' ') || (*p == '\0')) p++;
		if ((port = atoi(p)) == 0) {
			logout_failure(4, F_config_file_edit, "The port number is not set.\n");
			return 1;
		}	 
	   	sprintf(strport, "%d", port);
	   	if (strcmp(p, strport) != 0) {
			logout_failure(4, F_config_file_edit, "The input port number is invalid.\n");
			return 1;
		}	 
		ipaddr = (char *)malloc(i + 1);
		if (ipaddr == NULL) {
			logoutx(4, F_config_file_edit, "malloc failed.", "errno", errno);
			return 1;
		}
		memset(ipaddr, 0, i + 1);
		strncpy(ipaddr, input_data, i);
	    }
	    

	    /* save TDMF Collector coordinates  */
	    if ( cfg_set_software_key_value(AGENT_CFG_IP, ipaddr) != 0 ) {
		logout(4, F_config_file_edit, "Unable to write \'"AGENT_CFG_IP"\' configuration value!\n");
		free(ipaddr);
		return 1;
	    }
	    sprintf(logmsg, "IP address  = %s\n", ipaddr);
	    logout(9, F_config_file_edit, logmsg);
	    free(ipaddr);

	    if (p != NULL) {
		if ( cfg_set_software_key_value(AGENT_CFG_PORT, p) != 0 ) {
			logout(4, F_config_file_edit, "Unable to write \'"AGENT_CFG_PORT"\' configuration value!\n");
			return 1;
		}
		sprintf(logmsg, "Port number = %s\n", p);
		logout(9, F_config_file_edit, logmsg);
	    }
	}	/* if (select == 'C')  */

	if (select == 'A') {
	     /* select == 'A'  for agent parameter setup */
	    ipaddr = (char *)malloc(strlen(input_data) + 1);
	    if (ipaddr == NULL) {
		logoutx(4, F_config_file_edit, "malloc failed.", "errno", errno);
		return 1;
	    }
	    strcpy(ipaddr, input_data);
	    if ( cfg_set_software_key_value(AGENT_CFG_AGENTIP, ipaddr) != 0 ) {
		logout(4, F_config_file_edit, "Unable to write \'"AGENT_CFG_AGENTIP"\' configuration value!\n");
		free(ipaddr);
		return 1;
	    }
	    sprintf(logmsg, "Agent IP address  = %s\n", ipaddr);
	    logout(9, F_config_file_edit, logmsg);
	    free(ipaddr);
	}	/* (select == 'A') */

	/* WR 32382:
	 * Add BabSize: xxx parameter back to dtcAgent.cfg file to allow
	 * common console init BAB and load driver in a X-less UNIX server.
	 *
	 * bab_mbsize != 0 for primary server side setting only.
	 *
	 * The dtcinit will handle the BAB init ...
	 */
	if (bab_mbsize) {
	    sprintf(babstr, "%d", bab_mbsize);
	    if ( cfg_set_software_key_value(AGENT_CFG_BAB, babstr) != 0 ) {
		logout(4, F_config_file_edit, "Unable to write \'"AGENT_CFG_BAB"\' configuration value!\n");
		return 1;
	    }
	    sprintf(logmsg, "BAB size = %s (MB)\n", babstr);
            logout(9, F_config_file_edit, logmsg);
	}

	if (transmit_interval_value) {
	    sprintf(ti_str, "%d", transmit_interval_value);
	    if ( cfg_set_software_key_value(TRANSMITINTERVAL, ti_str) != 0 ) {
		logout(4, F_config_file_edit, "Unable to write \'"TRANSMITINTERVAL "\' configuration value!\n");
		return 1;
	    }
	    sprintf(logmsg, "Transmit Interval = %s (s)\n", ti_str);
            logout(9, F_config_file_edit, logmsg);
	}

	if (listener_port_value)
	{
	    sprintf(lp_str, "%d", listener_port_value);
	    if ( cfg_set_software_key_value(LISTENERPORT, lp_str) != 0 ) {
		logout(4, F_config_file_edit, "Unable to write \'"LISTENERPORT "\' configuration value!\n");
		return 1;
	    }
	    sprintf(logmsg, "Listener Port = %s (s)\n", lp_str);
            logout(9, F_config_file_edit, logmsg);


	}
	return 0;
}

int config_file_delete()
{
	char cmdline[CMDLINE_SIZE];
	struct stat statbuf;

	sprintf(cmdline ,"/bin/ps -ef | /bin/grep in."QAGN" | /bin/grep -v grep > /dev/null 2>&1");
	if (system(cmdline) == 0) {
		logout_failure(4, F_config_file_delete, "Since Agent is running, Agent cannot make inactive.\n");
		return 1;
	}

	if (stat(AGN_CFGFILE, &statbuf) != 0) {
		logoutx(4, F_config_file_delete, "stat failed.", "errno", errno);
		logout_failure(4, F_config_file_delete, "Agent is already inactive.\n");
		return 1;
	}

	if (yesorno("Is Agent invalidated?")) {
		if(unlink(AGN_CFGFILE) != 0){
			logoutx(4, F_config_file_delete, "unlink failed.", "errno", errno);
			logout_failure(4, F_config_file_delete, "Processing failed.\n");
			return 1;
		}
		logout_success(9, F_config_file_delete, "Inactivate processing of Agent ended normally.\n");
	} else {
		logout_failure(4, F_config_file_delete, "Inactivate processing of Agent was canceled.\n");
	}
	return 0;
}

int config_file_list()
{
	char input_data[AGN_CFG_PARM_SIZE];
	char ipaddr[AGN_CFG_PARM_SIZE];
	char babstr[16];			/* bab size (MB) string */
	char ti_str[16];
	char lp_str[16];
	int port;
	int listener_port;

	memset(ipaddr,0x00,sizeof(ipaddr));
	if ( cfg_get_software_key_value(AGENT_CFG_IP, ipaddr, sizeof(ipaddr)) != 0 ) {
		logout_failure(4, F_config_file_list, "Collector connection information is not set up.\n");
		return 1;
	}

	memset(input_data, 0x00, sizeof(input_data));
	if ( cfg_get_software_key_value(AGENT_CFG_PORT, input_data, sizeof(input_data)) == 0 ) {
		port = atoi(input_data);
	} else {
		port = FTD_DEF_COLLECTORPORT;
	}

	printf("Collector connection information.\n");
	printf("  IP address       = %s\n", ipaddr);
	printf("  Port number      = %d\n", port);
	
	memset(ipaddr,0x00,sizeof(ipaddr));
	if ( cfg_get_software_key_value(AGENT_CFG_AGENTIP, ipaddr, sizeof(ipaddr)) == 0 ) {
		printf("  AgentIP address  = %s\n", ipaddr);
	} else {
		printf("  AgentIP address  = %s\n", "default");
	}

	memset(babstr,0x00,sizeof(babstr));
	if ( cfg_get_software_key_value(AGENT_CFG_BAB, babstr, sizeof(babstr)) == 0 ) {
		printf("  BAB size         = %s (MB)\n", babstr);
	} else {
		printf("  BAB size not set ! \n");
	}

	memset(ti_str,0x00,sizeof(ti_str));
	if ( cfg_get_software_key_value(TRANSMITINTERVAL, ti_str, sizeof(ti_str)) == 0 ) {
		printf("  Transmit Interval= %s sec\n", ti_str);
	} else {
		printf("  Transmit Interval not set ! \n"); 
	}

	memset(lp_str, 0x00, sizeof(lp_str));
	if ( cfg_get_software_key_value(LISTENERPORT, lp_str, sizeof(lp_str)) == 0 ) {
		listener_port = atoi(lp_str);
	} else {
		listener_port = FTD_DEF_AGENTPORT;
	}
	printf("  Listener Port    = %d\n", listener_port); 

	logout(9, F_config_file_list, "Processing ended normally.\n");

	return 0;
}

void migrate_agent_cfg_file( int prompt_user )
{
	char migratedir[PATH_MAX];
	char cmdline[CMDLINE_SIZE];
	int  do_restore;

	if (get_migrate_dir(migratedir) == 0) {
		logout(9, F_migrate_agent_cfg_file, "There is no restoration directory.\n");
		return;
	}

	/* migrate Softek TDMF Agent cfg file */
	if ((survey_cfg_file(DTCCFGDIR, AGENT_CFG) == 0) &&
		(survey_cfg_file(migratedir, AGENT_CFG) > 0)) {
		if( prompt_user )
		{
			printf("A previous set of Collector connection information has been detected on this system.\n");
			do_restore = (yesorno("Would you like to migrate them into the current environment?"));
		}
		else
		{
		    do_restore = 1;
		}
		if( do_restore )
		{
			memset(cmdline, 0, sizeof(cmdline));
#if defined(linux)
			sprintf(cmdline, "/bin/cp %s/"QNM"Agent.cfg %s 1>/dev/null 2> /dev/null", migratedir, DTCCFGDIR);
#else
			sprintf(cmdline, "/usr/bin/cp %s/"QNM"Agent.cfg %s 1>/dev/null 2> /dev/null", migratedir, DTCCFGDIR);
#endif
			if (system(cmdline) != 0) {
				logout_failure(4, F_migrate_agent_cfg_file, "Restoration of Collector connection information failed. Agent is not active.\n");
			} else {
				logout_success(9, F_migrate_agent_cfg_file, "Collector connection information was restored. Agent is active.\n");
			}
		}
	}
}

void migrate_lg_cfg_file( int prompt_user )
{
	char migratedir[PATH_MAX];
	char buf[MAXPATHLEN];
	char cmdline[CMDLINE_SIZE];
	int  do_restore;

	if (get_migrate_dir(migratedir) == 0) {
		logout(9, F_migrate_lg_cfg_file, "There is no restoration directory.\n");
		return;
	}

	if (cfg_get_key_value("num_chunks", buf, CFG_IS_NOT_STRINGVAL) == 0 &&
		cfg_get_key_value("chunk_size", buf, CFG_IS_NOT_STRINGVAL) == 0) {
		logout(9, F_migrate_lg_cfg_file, "BAB size is set up.\n");
		return;
	}

	/* migrate Softek TDMF .cfg files */
	if ((survey_cfg_file(DTCCFGDIR, ALL_CFG) == 0) &&
		(survey_cfg_file(migratedir, ALL_CFG) > 0)) {
		if( prompt_user )
		{
			printf("A previous set of " GROUPNAME " group configuration file has been detected on this system.\n");
			do_restore = (yesorno("Would you like to migrate them into the current environment?"));
		}
		else
		{
		    do_restore = 1;
		}
		if( do_restore )
		{
			memset(cmdline, 0, sizeof(cmdline));
#if defined(linux)
			sprintf(cmdline, "/bin/cp %s/[ps][0-9][0-9][0-9].cfg %s 1>/dev/null 2> /dev/null", migratedir, DTCCFGDIR);
#else
			sprintf(cmdline, "/usr/bin/cp %s/[ps][0-9][0-9][0-9].cfg %s 1>/dev/null 2> /dev/null", migratedir, DTCCFGDIR);
#endif
			if (system(cmdline) != 0) {
				logout_failure(4, F_migrate_lg_cfg_file, "Migration of the " GROUPNAME " group configuration file failed.\n");
			} else {
				logout_success(9, F_migrate_lg_cfg_file, "The " GROUPNAME " group configuration file is migrated.\n");
			}
		}
	}
}

int get_migrate_dir(char * newestdir)
{
	struct stat statbuf;
	struct dirent *dirp;
	time_t keeptime;
	DIR *dp;
	char fullpath[PATH_MAX];
	int r = 0;

	if ((dp = opendir(DTCVAROPTDIR)) == NULL) {
		return 0;
	}

	keeptime = 0;
	while ((dirp = readdir(dp)) != NULL) {
		if (strcmp(dirp->d_name, ".") == 0) continue;
		if (strcmp(dirp->d_name, "..") == 0) continue;
		if (strncmp(dirp->d_name, PKGNAME, sizeof(PKGNAME) - 1) != 0) continue;

		sprintf(fullpath, "%s/%s", DTCVAROPTDIR, dirp->d_name);
		if (stat(fullpath, &statbuf) != 0) {
			r = 0;
			break;
		}
		if (!S_ISDIR(statbuf.st_mode)) continue;
		if (statbuf.st_mtime > keeptime) {
			strcpy(newestdir, fullpath);
			keeptime = statbuf.st_mtime;
			r = 1;
		}
	}
	closedir (dp);
	return r;
}

void program_exit(int code)
{
	if (logfp)
	    fclose(logfp);
	logfp=NULL;
	exit(code);
}
