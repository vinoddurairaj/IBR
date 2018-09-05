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
/* #ident "@(#)$Id: Agent_main.c,v 1.17 2018/01/25 20:56:47 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include "tdmfAgent.h"
#include "tdmfAgent_trace.h"
#include "common.h"

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


extern void ftd_mngt_initialize();
extern void ftd_bab_initialize(int babsize);
extern int survey_cfg_file(char *location, int cfg_file_type);


extern void ftd_mngt_receive_on_broadcast_listener_socket(sock_t* brdcst);
extern int  ftd_mngt_send_agentinfo_msg(ipAddress_t rip, int rport);
extern int  ftd_mngt_getServerId();
extern void ftd_mngt_StatusMsgThread();
extern void StatThread();
extern int Agent_proc(sock_t * sockID);


void	Agent_accept_proc();
sock_t	*ftd_mngt_create_broadcast_listener_socket();
void	program_exit();
void	child_chk();
void	abnormal_exit ();
void	time_out_daemon();

int     dbg;
int	RequestID;

int	Listner_port;
int	Agent_port;
int	Pid;
int	MasterPid;		   

extern	ipAddress_t	giTDMFCollectorIP;
extern  ipAddress_t giTDMFAgentIP;
extern	unsigned int	giTDMFBabSize;

extern FILE *logfp;
extern char logmsg[1024];

extern void errout(char *);
extern void execCommand(char *, char *);

char gszServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];

// Turned off: #define SIMULATE_GET_PRODUCT_USAGE_STATS_CMD

#ifdef SIMULATE_GET_PRODUCT_USAGE_STATS_CMD
extern int collect_server_product_usage_stats( char **temp_pDataBuffer, int *temp_iNumberOfBytes );
#endif

int debug_RPO = 0;

int main(int argc,char *argv[])
{
	int     s, n ;
	unsigned i;
	FILE    *fp;
	pid_t   ppid;
	int	on;
	fd_set  readset;
	int     nselect = 0;
	sock_t  *mngt_brdcst_listener = 0;
	char 	tmpcmd[256];
	char 	tmpbuf[256];
	char tmp[16];
	char msg_title[50];
	ipAddress_t empty_struct;

#ifdef SIMULATE_GET_PRODUCT_USAGE_STATS_CMD
    char *temp_pDataBuffer;
    int  temp_iNumberOfBytes;
	static int  called_function_already = 0;
#endif

	putenv("LANG=C");

   	
	Listner_port = 0;
	Agent_port = 0;
	Pid = 0;
	MasterPid = 0;

    // WI_338550 December 2017, implementing RPO / RTT
    debug_RPO = 1;

	/* user check */
	if ( getuid() != 0 ) {
		errout("You must be superuser to run "PKGNAME"-startagent.\n");
		exit(-1);
	}
	/* Agent execute check */
	Pid = getpid();
	memset(tmpbuf,0x00,sizeof(tmpbuf));
	sprintf(tmpcmd ,"/bin/ps -e | /bin/grep in."QAGN" | /bin/grep -v grep | /bin/grep -v %d",Pid);
	execCommand(tmpcmd,tmpbuf);
	if (strlen(tmpbuf) != 0) {
		errout("Agent is already running.\n");
		exit(-1);
	}
	Pid = 0;
	
	umask(00022); /* WR15542 : chane umask 00000 -> 00022 */
	strcpy(msg_title, "in."QAGN": "PRODUCTNAME" agent daemon started.");
	if (msg_init(msg_title) != 0) {
		exit(-1);
	}
	ftd_mngt_initialize();
	/* Collector IP addr check */
	if (is_unspecified(giTDMFCollectorIP) == 1)
	{
		logout(1,F_main,"Collector IP address not specified.\n");
		program_exit();															 
	}

	/* rework for WR 15450, add CC to init BAB size for X-less Unix server
	 * requires dtcAgent.cfg to contain BabSize
	 */
	if (giTDMFBabSize != 0) {
	    /* bab size is set
	     * must be for primary server setting.  dtcinit ... load driver ...
	     */
	    ftd_bab_initialize(giTDMFBabSize);

	} else {
	    /*  BAB size not set in agent config gile, must be non-primary */
	    logout(1,F_main,"Agent: BAB size not specified.  Continue!\n");

	    if (survey_cfg_file(DTCCFGDIR, PRIMARY_CFG) == TRUE) {
			memset(tmp, 0, sizeof(tmp));
		if (cfg_get_key_value("tcp_window_size", tmp, CFG_IS_NOT_STRINGVAL) != 0) {
			sprintf(tmp, "%d", DEFAULT_TCP_WINDOW_SIZE);
			if (cfg_set_key_value("tcp_window_size", tmp, CFG_IS_NOT_STRINGVAL) != 0) {
				logout(4, F_main, "TCP Window size initialization failed.\n");
			}
			logout(12, F_main, "tcp_window_size OK.\n");
		}

		if (cfg_get_key_value("num_chunks", tmp, CFG_IS_NOT_STRINGVAL) != 0 ||
			cfg_get_key_value("chunk_size", tmp, CFG_IS_NOT_STRINGVAL) != 0) {
			logout(4, F_main, "BAB size is not set up. Please start Agent after setting BAB size.\n");
			errout( "BAB size parameters are not set, but a primary side configuration file exists.\n" );
			errout( "Pleast start Agent after setting BAB size parameters.\n" );
			program_exit();
		}
	    }
	}	/* if (giTDMFBabSize != 0) ... */						  

	signal (SIGHUP, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGPIPE, SIG_IGN);
	signal (SIGTERM, program_exit);
	signal (SIGUSR1, SIG_IGN);
	signal (SIGUSR2, SIG_IGN);
#if !defined(linux)
#if (defined(HPUX) && (SYSVERS == 1131))
/* On HP-UX 11.31, the following signal handler definitions cause pclose() failures and,
   consequently, popen() calls to fail after 30000 calls in the Agent, which seem to be a limit 
   on forked processes left open on the HPUX 11.31 test machine as it is currently configured. 
   pclose() waits on child processes to terminate. Asking the system to ignore these signals causes 
   pclose() to return the "No child process" error. */
#else
	signal (SIGCLD,  SIG_IGN);
	signal (SIGCHLD, SIG_IGN);
#endif
#endif
	
	if (ftd_mngt_getServerId() == -1) {
		logout(1,F_main,"cannot get hostname.\n");
		program_exit();
	}

#ifdef  ENABLE_RPO_DEBUGGING
    sprintf(logmsg, ">>> TRACING, sizeof(ftd_perf_instance_t): %d\n", sizeof(ftd_perf_instance_t));
    logout(4,F_main,logmsg);
#endif
    
// Depending on the following compile switch, if defined, we can simulate collection of the Product Usage statistics
#ifdef SIMULATE_GET_PRODUCT_USAGE_STATS_CMD
    if( !called_function_already )
	{
		sprintf(logmsg, "<<< Calling collect_server_product_usage_stats()...\n");
		logout(4,F_Agent_proc,logmsg);
	    collect_server_product_usage_stats( &temp_pDataBuffer, &temp_iNumberOfBytes );
		if( temp_pDataBuffer != NULL )
		    free( temp_pDataBuffer );

		sprintf(logmsg, "<<< Received %d bytes from function collect_server_product_usage_stats.\n", temp_iNumberOfBytes);
		logout(4,F_Agent_proc,logmsg);
		called_function_already = 1;
	}
#endif


	switch (ppid = fork()) {
	case -1:
		/*
		 *  The fork() failed, due to a lack of available processes.
		 */
		sprintf (logmsg,
		   "first fork failed. errno = %d\n",errno);
		logout(1,F_main,logmsg);
		program_exit();
	case 0:
		/*
		 *  The fork() succeeded.  We are the child server process.
		 *
		 *  Close the original socket before handling the new request.
		 */
		setpgrp();
		break;
	default:
		/*
		 *  The fork() succeeded. Close new socket
		 *  and loop to handle another request.
		 */
		exit(0);
	}
	/***********************************************************/
	/***********************************************************/
	/* 
	 * Create stat process.
	 */
	/***********************************************************/
	/***********************************************************/
	switch (ppid = fork()) {
	case -1:
		/*
		 *  The fork() failed, due to a lack of available processes.
		 */
		sprintf (logmsg,
		   "stat process fork failed. errno = %d\n",errno);
		logout(1,F_main,logmsg);
		program_exit();
	case 0:
		/*
		 *  The fork() succeeded.  We are the child server process.
		 */
		Pid = getpid();
		logoutx (14,F_main,"stat process started","pid",Pid);
		ftd_mngt_StatusMsgThread();
		program_exit();
		break;
	default:
		/*
		 *  The fork() succeeded. Close new socket
		 */
		break;
	}
	/***********************************************************/
	/***********************************************************/
	/* 
	 * Create StatThread process.
	 */
	/***********************************************************/
	/***********************************************************/
	switch (ppid = fork()) {
	case -1:
		/*
		 *  The fork() failed, due to a lack of available processes.
		 */
		sprintf (logmsg,
		   "group status process fork failed. errno = %d\n",errno);
		logout(1,F_main,logmsg);

		program_exit();
	case 0:
		/*
		 *  The fork() succeeded.  We are the child server process.
		 */
		Pid = getpid();
		logoutx (14,F_main,"group status process started","pid",Pid);
		StatThread();
		program_exit();
		break;
	default:
		/*
		 *  The fork() succeeded. Close new socket
		 */
		break;
	}
	/***********************************************************/
	/***********************************************************/
	/* 
	 * Create agent process.
	 */
	/***********************************************************/
	/***********************************************************/
	switch (ppid = fork()) {
	case -1:
		/*
		 *  The fork() failed, due to a lack of available processes.
		 */
		sprintf (logmsg,
		   "agent process fork failed. errno = %d\n",errno);
		logout(1,F_main,logmsg);
		program_exit();
	case 0:
		/*
		 *  The fork() succeeded.  We are the child server process.
		 *
		 *  Close the original socket before handling
		 *  the new request.
		 */
		/***********************************************************/
		/***********************************************************/
		/* 
		 * Agent process start.
		 */
		/***********************************************************/
		/***********************************************************/
		Pid = getpid();
		logoutx (14,F_main,"agent process started","pid",Pid);
		Agent_accept_proc();			
		program_exit();
	default:
		/*
		 *  The fork() succeeded. Close new socket
		 *  and loop to handle another request.
		 */
		/***********************************************************/
		/***********************************************************/
		/* 
		 * listner process start.
		 */
		/***********************************************************/
		/***********************************************************/						   
		Pid = getpid();
		MasterPid = Pid;
		logoutx (14,F_main,"listener process started","pid",Pid);
   	 
    		/* to catch broadcast message from TDMF Collector,
				create a dedicated SOCK_DGRAM socket  */
   		logout(14,F_main,"listener: create call.\n");
    		mngt_brdcst_listener = ftd_mngt_create_broadcast_listener_socket();
   		logout(14,F_main,"listener: create return.\n");
    		/* if TDMFCollector is known, send our TDMF Agent information */
   		logout(14,F_main,"listener: send call.\n");			  
    		ftd_mngt_send_agentinfo_msg(empty_struct, 0);
   		logout(14,F_main,"listener: send return.\n");
		  
		for(;;)
		{
		    /* Wait message */
		    FD_ZERO(&readset);
		    FD_SET(mngt_brdcst_listener->sockID,&readset);
			

		    nselect = mngt_brdcst_listener->sockID+1;
	
   		    logout(14,F_main,"listener: select call.\n");
		    n = select(++nselect, &readset, NULL, NULL, NULL);
   		    logout(14,F_main,"listener: select return.\n");
		 
       	     	    switch(n)       
       	     	    {
       	       	    case -1:
               	    case 0: 
               	        break; 
               	    default:
    		        /* collector data */
   		        logout(14,F_main,"listener: isset call.\n");
		   	if (FD_ISSET(mngt_brdcst_listener->sockID, &readset))
		    	{
   		            logout(14,F_main,"listener: call recv.\n");
			    ftd_mngt_receive_on_broadcast_listener_socket(mngt_brdcst_listener);
   		            logout(14,F_main,"listener: return recv.\n");
		    	}
   		    	break;
	    	    } 
		}
	}
}


/********************************************************/
/*
 * start_proc()
 */
/********************************************************/


void Agent_accept_proc()
{
    struct sockaddr_in myaddr;
#if !defined(FTD_IPV4)
	struct sockaddr_in6 myaddr6;
#endif
    int socket_fd;
#if defined(SOLARIS) || defined(HPUX)
    int n,s,len;
#elif defined(_AIX) || defined(linux)
    int n,s;
    socklen_t len;
#endif
    sock_t * sockt;
    char tmp[30];

#if defined(linux)
	signal (SIGCLD,  SIG_IGN);
	signal (SIGCHLD, SIG_IGN);
#endif

    sprintf(logmsg,"start. port number = %d.\n",Agent_port);
    logout(14,F_Agent_accept_proc,logmsg);

#if defined(FTD_IPV4)
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	logoutx(1, F_Agent_accept_proc, "socket error", "errno", errno);
        return;
    }   
#else
    if ((socket_fd = socket((giTDMFCollectorIP.Version == IPVER_4 && giTDMFAgentIP.Version == IPVER_4) ? AF_INET : AF_INET6, SOCK_STREAM, 0)) < 0) {
	logoutx(1, F_Agent_accept_proc, "socket error", "errno", errno);
        return;
    }  
#endif
    n = 1;  
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n,
        sizeof(int)) < 0) {
	logoutx(1, F_Agent_accept_proc, "setsockopt error", "errno", errno);
	close(socket_fd);
        return;
    }   

	if(giTDMFCollectorIP.Version == IPVER_4 && giTDMFAgentIP.Version == IPVER_4) {	
    	memset((char*)&myaddr, 0, sizeof(myaddr));
    	myaddr.sin_family = AF_INET;
    	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    	myaddr.sin_port = htons(Agent_port);
    	len = sizeof(myaddr);  

    	if (bind(socket_fd, (struct sockaddr*)&myaddr, len) < 0) {
		logoutx(1, F_Agent_accept_proc, "bind error agent_proc", "errno", errno);
		close(socket_fd);
    	    return;
		}
   	 }
	 else {
#if !defined(FTD_IPV4)   
	    int i = 0;
	    memset((char*)&myaddr6, 0, sizeof(myaddr6));
    	myaddr6.sin6_family = AF_INET6;
    	
    	myaddr6.sin6_port = htons(Agent_port);
    	len = sizeof(myaddr6);  
    
    
    	
    	if (bind(socket_fd, (struct sockaddr*)&myaddr6, len) < 0) {
		logoutx(1, F_Agent_accept_proc, "bind error agent_proc", "errno", errno);
		close(socket_fd);
    	    return;
		}
#endif																	
	}

    setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&n, sizeof(int));

    if (listen(socket_fd, 1) < 0) {
	logoutx(1, F_Agent_accept_proc, "listen error", "errno", errno);
	close(socket_fd);
        return;
    }

    sockt = sock_create();
#if defined(FTD_IPV4)
	sockt->type = AF_INET;
#else
	sockt->type = (giTDMFCollectorIP.Version == IPVER_4 && giTDMFAgentIP.Version == IPVER_4) ? AF_INET : AF_INET6;		 
#endif
    sockt->family = SOCK_STREAM;
    sockt->lhostid = (unsigned int) gethostid(); 

    for (;;) {
        errno = 0;						   


        if(giTDMFCollectorIP.Version == IPVER_4 && giTDMFAgentIP.Version == IPVER_4)		   
        {
        	sockt->sockID = accept (socket_fd, (struct sockaddr *)&myaddr, &len);
		}
        else
        {
#if !defined(FTD_IPV4)
			sockt->sockID = accept (socket_fd, (struct sockaddr *)&myaddr6, &len);
#endif
		}



        sprintf(logmsg,"accept socket = %d.\n",sockt->sockID);
        logout(14,F_Agent_accept_proc,logmsg);
        if (sockt->sockID == -1) {
	    logoutx(4, F_Agent_accept_proc, "accept error", "errno", errno);
        }
        else {
            switch (fork()) {
            case -1:
                    /*
                     *  The fork() failed, due to a lack of available processes.
                     */
                    close (sockt->sockID);     /* Break connection */
                    logout(4,F_Agent_accept_proc,"accept process fork failed.\n");
                    break;
            case 0:
                    /*
                     *  The fork() succeeded.  We are the child server process.
                     *
                     *  Close the original socket before handling the new request.
                     */
                    logout(12,F_Agent_accept_proc,"accept OK.\n");
		 		    close(socket_fd);
                    Agent_proc(sockt);
		    if (sockt->sockID > 0)
		    {
  		        close(sockt->sockID);
		    }
		    sockt->sockID = -1;
		    exit(1);
                    return;
            default:
                    /*
                     *  The fork() succeeded. Close new socket
                     *  and loop to handle another request.
                     */
                    close (sockt->sockID);
                    break;
            }
        }
    }				 
}

/*** program_exit
 */
void program_exit()
{
#if defined(linux)
	signal (SIGCLD,  SIG_DFL);
	signal (SIGCHLD, SIG_DFL);
#endif
	/*	if (MasterPid == Pid) */
	logout(9,F_program_exit,"Agent daemon terminated.\n");
	if (logfp)
	    fclose(logfp);
	logfp = NULL;
	sleep(1);
	system(AGN_STOP_SH);
	exit(0);
}

/*** abnormal_exit
 *
 */
void abnormal_exit (int code)
{
	if (MasterPid == Pid)
		logout(1,F_abnormal_exit,"Agent daemon abnormal end terminated.\n");
	if (logfp)
	    fclose(logfp);
	logfp = NULL;
	system(AGN_STOP_SH);
	exit(code);
}

/*** time_out_daemon
 *
 * SIGALRM exit
 */
void time_out_daemon()
{
	if (MasterPid == Pid)
		logout(1,F_time_out_daemon,"Agent daemon pre_server time out.\n");
	system(AGN_STOP_SH);
	exit(1);
}
