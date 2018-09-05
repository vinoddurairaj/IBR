/* #ident "@(#)$Id: Agent_main.c,v 1.29 2003/11/13 02:48:20 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
#include <stdio.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <pwd.h>
#include "tdmfAgent.h"
#include "tdmfAgent_trace.h"

void	Agent_accept_proc();
sock_t	*ftd_mngt_create_broadcast_listener_socket();
void	program_exit();
void	child_chk();
void	abnormal_exit ();
void	time_out_daemon();
void    errout(char *);

int     dbg;
FILE	*log;
char    logmsg[1024];
char    logfile[1024];
int	Debug;
int	RequestID;

int	Listner_port;
int	Agent_port;
int	Pid;
int	MasterPid;

extern	int	giTDMFCollectorIP;
extern	int	giTDMFBabSize;

extern void execCommand(char *, char *);

char gszServerUID[MMP_MNGT_MAX_MACHINE_NAME_SZ];

int main(int argc,char *argv[])
{
	struct sockaddr_in me;
	struct servent *servent;        /* Server entry structure */
	int	me_addrlen ;
	int     s, n ;
	unsigned i;
	char    indata[1];
	FILE    *fp;
	time_t  tmt;
	pid_t   ppid;
	int	log_fd;			/* grand father log file descripter */
	int     c_bindretry;            /* bind retry counter         */
	char    log_time[16];
	int	on;
        fd_set  readset;
	int     nselect = 0;
	sock_t  *mngt_brdcst_listener = 0;
	char 	tmpcmd[256];
	char 	tmpbuf[256];

	Listner_port = 0;
	Agent_port = 0;
	Pid = 0;
	MasterPid = 0;

	/* user check */
        if ( getuid() != 0 ) {
		errout("You must be superuser to run SFTKdua.\n");
		exit(-1);
        }
	/* dtcAgent execute check */
	Pid = getpid();
	memset(tmpbuf,0x00,sizeof(tmpbuf));
	sprintf(tmpcmd ,"/bin/ps -e | /bin/grep dtcAgent | /bin/egrep -v grep | egrep -v %d",Pid);
	execCommand(tmpcmd,tmpbuf);
	if (strlen(tmpbuf) != 0) {
		errout("dtcAgent is already running.\n");
		exit(-1);
        }
	Pid = 0;
	

	Debug =1;
        memset(logmsg,0x00,sizeof(logmsg));
        cfg_get_software_key_value("tracelevel", logmsg, sizeof(logmsg));
        if(strlen(logmsg) == 0)
        {
           /* not output trace */
           Debug = 10;
        }
        else {
           /* set trace level */
           Debug = atoi(logmsg);
	}
        memset(logmsg,0x00,sizeof(logmsg));
        cfg_get_software_key_value("MsgFilePath", logmsg, sizeof(logmsg));
        if (strlen(logmsg) ==0) 
        {
	    sprintf(logmsg,AGTVAROPTDIR);
        }
	time(&tmt);
	strftime(log_time,100,"%b%d.%Y",localtime(&tmt));
	sprintf(logfile,"%s/%s",logmsg,log_time);
	/* INIT */
	memset(tmpcmd,0x00,sizeof(tmpcmd));
#if defined(linux)
	sprintf(tmpcmd,"/bin/rm "AGTTMPDIR"/* 1> /dev/null 2> /dev/null");
#else
	sprintf(tmpcmd,"/usr/bin/rm "AGTTMPDIR"/* 1> /dev/null 2> /dev/null");
#endif
	system(tmpcmd);
	/* Management stuff */
	/*
	 * log file open process
	 */
	umask(00022); /* WR15542 : chane umask 00000 -> 00022 */
	if ((log = fopen(logfile,"a")) == NULL) {
		/*** change message text ***/
		sprintf(logmsg,"%s fopen failed. errno = %d\n", logfile, errno);
		errout(logmsg);
		exit(-1);
	}
	logoutdelta("****************************************************************************\n");
	sprintf(logmsg,"dtcAgent: Replicator agent daemon started. %s",ctime(&tmt));
	logoutdelta(logmsg);
	logoutdelta("****************************************************************************\n");
	ftd_mngt_initialize();
	/* Collector IP addr check */
	if (giTDMFCollectorIP == 0)
	{
		logout(1,F_main,"Collector IP address not specified.\n");
		program_exit();
	}
	if (giTDMFBabSize != 0)
	{
		ftd_bab_initialize(giTDMFBabSize);
	}

	signal (SIGHUP, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGPIPE, SIG_IGN);
	signal (SIGTERM, program_exit);
	signal (SIGUSR1, SIG_IGN);
	signal (SIGUSR2, SIG_IGN);
#if !defined(linux)
	signal (SIGCLD,  SIG_IGN);
	signal (SIGCHLD, SIG_IGN);
#endif
	
	if (ftd_mngt_getServerId() == -1) {
		logout(1,F_main,"cannot get hostname.\n");
		program_exit();
	}

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
    		ftd_mngt_send_agentinfo_msg(0, 0);
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
    int socket_fd;
#if defined(SOLARIS) || defined(HPUX) || defined(linux)
    int n,s,len;
#elif defined(_AIX)
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

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	logoutx(1, F_Agent_accept_proc, "socket error", "errno", errno);
        return;
    }   

    n = 1;  
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n,
        sizeof(int)) < 0) {
	logoutx(1, F_Agent_accept_proc, "setsockopt error", "errno", errno);
	close(socket_fd);
        return;
    }   

    memset((char*)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(Agent_port);
    len = sizeof(myaddr);  
    if (bind(socket_fd, (struct sockaddr*)&myaddr, len) < 0) {
	logoutx(1, F_Agent_accept_proc, "bind error", "errno", errno);
	close(socket_fd);
        return;
    }

    setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&n, sizeof(int));

    if (listen(socket_fd, 1) < 0) {
	logoutx(1, F_Agent_accept_proc, "listen error", "errno", errno);
	close(socket_fd);
        return;
    }

    sockt = sock_create();
    sockt->type = AF_INET;
    sockt->family = SOCK_STREAM;
    sockt->lhostid = gethostid(); 

    for (;;) {
        errno = 0;
        sockt->sockID = accept (socket_fd, (struct sockaddr *)&myaddr, &len);
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

/*** logout
 * 
 */
void logout(int loglvl,int func, char *logmsg)
{
	time_t  tmt;
	char    log_time[16];
	char 	condition[8];

	if (loglvl <= Debug) {
		time(&tmt);
		strftime(log_time,100,"%b%d %H:%M:%S",localtime(&tmt));

		/* 
		 * loglvl
		 *       0 : Cannot start Agent
		 *  1 -  3 : Error - Cannot continue Agent
		 *  4 -  6 : Warning
		 *  7 -  8 : Notice
		 *  9 - 10 : Info
		 * 11 - 20 : Trace
		 */
		memset(condition, 0x00, sizeof(condition));
		if (loglvl == 0) {
			sprintf(condition, "%s", "");
		} else if (1 <= loglvl && loglvl <= 3) {
			sprintf(condition, "%s", "ERROR: ");
		} else if (4 <= loglvl && loglvl <= 6) {
			sprintf(condition, "%s", "WARN: ");
		} else if (7 <= loglvl && loglvl <= 8) {
			sprintf(condition, "%s", "NOTICE: ");
		} else if (9 <= loglvl && loglvl <= 10) {
			sprintf(condition, "%s", "INFO: ");
		} else if (11 <= loglvl && loglvl <= 20) {
			sprintf(condition, "%s", "TRACE: ");
		}

		fprintf(log,"[%s][%3d][%5d]:%s%s",log_time,func,Pid,condition,logmsg);
		fflush(log);
	}
	return;
}

/*** logoutx
 * 
 */
void logoutx(int loglvl,int func, char *logmsg,char *text,int value)
{
	char logtext[1024+1];

	memset(logtext,0x00,sizeof(logtext));
	if (text != NULL)
	{
		sprintf(logtext,"%s,%s = %d.\n",logmsg,text,value);
	}
	else
	{
		sprintf(logtext,"%s,code = %d.\n",logmsg,value);
	}
	logout(loglvl,func,logtext);
	return;
}

void logoutdelta(char *logmsg)
{
	fprintf(log,"%s",logmsg);
	fflush(log);
	return;
}

/*** program_exit
 */
void program_exit()
{
	/*	if (MasterPid == Pid) */
	logout(9,F_program_exit,"dtcAgent daemon terminated.\n");
	fclose(log);
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
		logout(1,F_abnormal_exit,"dtcAgent daemon abnormal end terminated.\n");
	fclose(log);
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
		logout(1,F_time_out_daemon,"dtcAgent daemon pre_server time out.\n");
	system(AGN_STOP_SH);
	exit(1);
}

void errout( char *msg)
{
	fprintf(stderr, "UX:dtcAgent: ERROR: %s", msg);
	fflush(stderr);
}
