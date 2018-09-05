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
/* #ident "@(#)$Id: Agent_msg.c,v 1.7 2011/10/31 15:16:37 proulxm Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "tdmfAgent.h"
#include "tdmfAgent_trace.h"

void errout(char *);

FILE *logfp = NULL;
static char logfile[1024];
static char agn_logfilebk[1024];
int agn_log_errs = 1;

char logmsg[1024];
int Debug;

extern int Pid;

int msg_init(char *title)
{
	time_t  tmt;
	char    log_time[16];
	char 	tmpcmd[256];

	Debug =1;
	memset(logmsg,0x00,sizeof(logmsg));
	cfg_get_software_key_value(TRACELEVEL, logmsg, sizeof(logmsg));
	if(strlen(logmsg) == 0) {
		/* not output trace */
		Debug = 10;
	} else {
		/* set trace level */
		Debug = atoi(logmsg);
	}

	memset(logmsg,0x00,sizeof(logmsg));
	cfg_get_software_key_value(MSGFILEPATH, logmsg, sizeof(logmsg));
	if (strlen(logmsg) ==0) {
		sprintf(logmsg,AGNVAROPTDIR);
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
	if ((logfp = fopen(logfile,"a")) == NULL) {
		agn_log_errs = 0;
		/*** change message text ***/
		sprintf(logmsg,"%s fopen failed. errno = %d\n", logfile, errno);
		errout(logmsg);
		return 1;
	}
	logoutdelta("****************************************************************************\n");
	sprintf(logmsg,"%s %s tracelevel=%d\n", title, ctime(&tmt), Debug);
	logoutdelta(logmsg);
	logoutdelta("****************************************************************************\n");

	return 0;
}

void chk_agn_errlog_size(void)
{
   if (ftell(logfp) >= AGN_ERRLOGMAX) {
      if (logfp != NULL){
         fclose(logfp);
         logfp = NULL;
      }
      sprintf(agn_logfilebk, "%s.1", logfile);
      (void) unlink(agn_logfilebk);
      (void) rename(logfile,agn_logfilebk);
      if ((logfp = fopen(logfile, "a")) == NULL) {
	 agn_log_errs = 0;
         sprintf(logmsg,"in.dua: %s fopen failed with errno = %d\n", logfile, errno);
         errout(logmsg);
         return;
      }
   }
   return;   
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
		chk_agn_errlog_size( );
		if (agn_log_errs) { 
		    fprintf(logfp,"[%s][%3d][%5d]:%s%s",log_time,func,Pid,condition,logmsg);
		    fflush(logfp);
		}
	}
	return;
}


/*** logoutx
 * 
 */
void logoutx(int loglvl,int func, const char *logmsg, const char *text,int value)
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
	chk_agn_errlog_size( );
	if (agn_log_errs) {
	   fprintf(logfp,"%s",logmsg);
	   fflush(logfp);
	}
	return;
}

void logout_success(int loglvl,int func, char *logmsg)
{
	logout(loglvl, func, logmsg);
	printf(logmsg);
	return;
}

void logout_failure(int loglvl,int func, char *logmsg)
{
	logout(loglvl, func, logmsg);
	fprintf(stderr, logmsg);
	return;
}

void errout( char *msg)
{
	fprintf(stderr, "UX:in."QAGN": ERROR: %s", msg);
	fflush(stderr);
}
