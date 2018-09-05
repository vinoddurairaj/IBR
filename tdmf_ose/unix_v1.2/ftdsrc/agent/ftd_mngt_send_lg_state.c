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
/* #ident "@(#)$Id: ftd_mngt_send_lg_state.c,v 1.4 2018/02/28 00:25:45 paulclou Exp $" */
/*
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_SEND_LG_STATE_C_
#define _FTD_MNGT_SEND_LG_STATE_C_

#include <dirent.h>

typedef struct _lg_journal {
	time_t		timestamp;
	char		journaldir[MMP_MNGT_MAX_FILENAME_SZ + 1];
	char		mountpoint[MMP_MNGT_MAX_FILENAME_SZ + 1];
	int			sourceIP;
	ftd_int64	disktotal;		/* in Byte */
	ftd_int64	diskavail;		/* in Byte */
	ftd_int64	useingrp;		/* in Byte */
} lg_journal_t;

lg_journal_t lg_journal[FTD_MAX_GROUPS];

static int logged_bad_IP_message[FTD_MAX_GROUPS];
static int first_run = 1;

typedef struct _mpsize {
	char		mountpoint[MMP_MNGT_MAX_FILENAME_SZ + 1];
	ftd_int64	disktotal;		/* in Byte */
	ftd_int64	diskavail;		/* in Byte */
} mp_table_t;

static void send_lg_state_debug_trace(char *trace_string)
{
        sprintf(logmsg, ">>> TRACING %s\n", trace_string);
        logout(4,F_main,logmsg);
}

static void send_lg_state_debug_trace_and_value(char *trace_string, unsigned long value)
{
        sprintf(logmsg, ">>> TRACING %s: %lu\n", trace_string, value);
        logout(4,F_main,logmsg);
}

static void send_lg_state_debug_trace_and_int(char *trace_string, int value)
{
        sprintf(logmsg, ">>> TRACING %s: %d\n", trace_string, value);
        logout(4,F_main,logmsg);
}

int
getjournaldirname(lg_journal_t *jp, int lgnum)
{
	char	cmdline[256];
	char	journaldir[256];
	FILE	*f;
	int	i;

	sprintf(cmdline, "/bin/grep \"JOURNAL:\" %s/s%03d.cfg | awk '{print $2}'", DTCCFGDIR, lgnum);
	if ((f = popen(cmdline, "r")) == NULL) {
		logoutx(4, F_getjournaldirname, "popen failed", "errno", errno);
		return -1;
	}
	if (fgets(journaldir, 256, f) == NULL) {
		pclose(f);
		return -1;
	}
	i = strlen(journaldir);
	journaldir[i - 1] = 0;
	strcpy(jp->journaldir, journaldir);
	pclose(f);
	return 0;
}

int
getdiskuseinfo(lg_journal_t *jp, int lgnum)
{
	char		mountpoint[MMP_MNGT_MAX_TDMF_PATH_SZ + 1];
	char		cmdline[256];
	char		line[256];
	char		tmp[16], tmp2[16];
#if defined(HPUX) || defined(linux)
	char		tmp3[16], tmp4[16], tmp5[256], tmp6[256];
#endif
	ftd_uint64	disktotal = 0;
	ftd_uint64	avail = 0;
	FILE		*f;
	int		i;

	memset(line, 0, sizeof(line));

#if defined(SOLARIS)
	sprintf(cmdline, "/usr/bin/df -k %s 2> /dev/null | /usr/bin/tail -1 | awk '{print $2,$4,$6}'", jp->journaldir);
#elif defined(_AIX)
	sprintf(cmdline, "/usr/bin/df -k %s 2> /dev/null | /usr/bin/tail -1 | awk '{print $2,$3,$7}'", jp->journaldir);
#elif defined(HPUX)
	sprintf(cmdline, "/usr/bin/bdf %s 2> /dev/null | /usr/bin/tail -1", jp->journaldir);
#elif defined(linux)
	sprintf(cmdline, "/bin/df -k %s 2> /dev/null | /usr/bin/tail -1", jp->journaldir);
#else
	BAD ARCHITECTURE !!
#endif

	if ((f = popen(cmdline, "r")) == NULL) {
		logoutx(4, F_getdiskuseinfo, "popen failed", "errno", errno);
		return -1;
	}
	if (fgets(line, 256, f) == NULL) {
		pclose(f);
		jp->disktotal = 0;	/* in Byte. from KB */
		jp->diskavail = 0;	/* in Byte. from KB */
		return 0;
	}
	pclose(f);

#if defined(SOLARIS) || defined(_AIX)
	sscanf(line, "%s %s %s", tmp, tmp2, mountpoint);
	atollx(tmp, &disktotal);
	atollx(tmp2, &avail);
#elif defined(HPUX) || defined(linux)
	i = sscanf(line, "%s %s %s %s %s %s", tmp, tmp2, tmp3, tmp4, tmp5, tmp6);
	if(i == 5){
		atollx(tmp, &disktotal);
		atollx(tmp3, &avail);
		strcpy(mountpoint, tmp5);
	} else if (i == 6){
		atollx(tmp2, &disktotal);
		atollx(tmp4, &avail);
		strcpy(mountpoint, tmp6);
	}
#endif
	strcpy(jp->mountpoint, mountpoint);
	jp->disktotal = disktotal * 1024;	/* in Byte. from KB */
	jp->diskavail = avail * 1024;	/* in Byte. from KB */
	return 0;
}

int
getjournalfilevalue(lg_journal_t *jp, int lgnum)
{
	char		cmdline[256];
	char		line[384];
	FILE		*f;
	char		parmition[16], link[4], fsize[16], month[8];
	char		day[8], time[8], filename[MMP_MNGT_MAX_FILENAME_SZ];
#if defined(linux)
	char		owner[32+1];
#endif
	int		ret;
	ftd_uint64	filesz;

	jp->useingrp = 0;
	memset(line, 0, sizeof(line));

#if defined(linux)
	sprintf(cmdline, "/bin/ls -go1 %s/j%03d.*.[ci] 2> /dev/null", 
	    jp->journaldir, lgnum);
#else
	sprintf(cmdline, "/usr/bin/ls -go1 %s/j%03d.*.[ci] 2> /dev/null", 
	    jp->journaldir, lgnum);
#endif
	if ((f = popen(cmdline, "r")) != NULL) {
		while (fgets(line, 384, f) != NULL) {
			ret = sscanf(line, "%s %s %s %s %s %s %s", 
			    parmition, link, fsize, month, day, time, filename);
			if (ret == 7) {
				atollx(fsize, &filesz);
				jp->useingrp += filesz;	/* in bytes */
			}
		}
		pclose(f);
		return 0;
	} else {
		logoutx(4, F_getjournalfilevalue, "popen failed", "errno", errno);
		return -1;
	}
}

int
getReplGrpSourceIP(int lgnum)
{
	char		cmdline[256];
	char		line[MMP_MNGT_MAX_MACHINE_NAME_SZ];
	char		*buffchar;
	FILE		*f;
	int		rgsip = -1;
	struct hostent	*hp;

	memset(line, 0, sizeof(line));

#if defined(linux)
	sprintf(cmdline, "/bin/grep \"HOST:\" %s/s%03d.cfg 2> /dev/null | /usr/bin/head -1 | awk '{print $2}'", 
	    DTCCFGDIR, lgnum);
#else
	sprintf(cmdline, "/usr/bin/grep \"HOST:\" %s/s%03d.cfg 2> /dev/null | /usr/bin/head -1 | awk '{print $2}'", 
	    DTCCFGDIR, lgnum);
#endif
	if ((f = popen(cmdline, "r")) != NULL) {
		while (NULL != fgets(line, MMP_MNGT_MAX_MACHINE_NAME_SZ, f)) {
			buffchar = strchr(line, '\n');
			if(buffchar != NULL) *buffchar = '\0';
			if (inet_addr(line) == -1) {
				/* not string of IP address */
				if ((hp = gethostbyname(line)) == NULL) {
					sprintf(logmsg, "%s: unknown host\n", line);
					logout(4, F_getReplGrpSourceIP, logmsg);
					pclose(f);
					return -1;
				}
				bcopy((char *)hp->h_addr, line, sizeof(line));
			}
			ipstring_to_ip_uint(line, (unsigned int *)&rgsip);
			memset(line, 0, sizeof(line));
		}
		pclose(f);
		return rgsip;
	} else {
		logoutx(4, F_getReplGrpSourceIP, "popen failed", "errno", errno);
		return -1;
	}
}

void
send_lg_journal(lg_journal_t *jp, int lgnum)
{
	mmp_mngt_TdmfReplGroupMonitorMsg_t	gromonim;
	mmp_mngt_TdmfReplGroupMonitorMsg_t	*pmsg;
	int		towrite, r;
    int     activate_debug_traces = 0;
	struct  stat statbuf;

    if( first_run )
    {
        memset(logged_bad_IP_message, 0, sizeof(logged_bad_IP_message));
        first_run = 0;
    }
    
	pmsg = &gromonim;
    
    memset(pmsg, 0, sizeof(mmp_mngt_TdmfReplGroupMonitorMsg_t));
    
	/* init. static data of msg only once */
	pmsg->hdr.magicnumber     = MNGT_MSG_MAGICNUMBER;
	pmsg->hdr.mngttype        = MMP_MNGT_GROUP_MONITORING;
	pmsg->hdr.mngtstatus      = MMP_MNGT_STATUS_OK;
	pmsg->hdr.sendertype      = SENDERTYPE_TDMF_AGENT;
    
	mmp_convert_mngt_hdr_hton(&pmsg->hdr);

	memset(&pmsg->data, 0, sizeof(pmsg->data));

	/* a ReplGroup Secondary machine uses the Journal */
	strcpy(pmsg->data.szServerUID, gszServerUID);
	pmsg->data.liDiskTotalSz    = jp->disktotal;
	pmsg->data.liDiskFreeSz     = jp->diskavail;
	pmsg->data.liActualSz       = jp->useingrp;
	pmsg->data.isSource         = 0;
	pmsg->data.iReplGrpSourceIP = jp->sourceIP;
    
    if( (pmsg->data.iReplGrpSourceIP == 0) && (logged_bad_IP_message[lgnum] == 0) )
    {
        sprintf(logmsg, "WARNING: send_lg_journal error source IP address = 0. Reporting journal stats for group %d might fail.\n", lgnum);
        logout(4,F_main,logmsg);
        sprintf(logmsg, "WARNING: Please verify that IP addresses are used in the group config file HOST definition.\n");
        logout(4,F_main,logmsg);
        logged_bad_IP_message[lgnum] = 1;
    }
    
	pmsg->data.sReplGrpNbr = lgnum;
    
    if( stat("/tmp/debug_journal_stats", &statbuf) == 0)
        activate_debug_traces = 1;
    else
        activate_debug_traces = 0;
    
    if( activate_debug_traces )
    {
        send_lg_state_debug_trace_and_value("GROUP NUMBER pmsg->data.sReplGrpNbr BEFORE hton", (unsigned long)pmsg->data.sReplGrpNbr);
        send_lg_state_debug_trace(pmsg->data.szServerUID);
        send_lg_state_debug_trace_and_value("pmsg->data.liDiskTotalSz BEFORE hton", (unsigned long)pmsg->data.liDiskTotalSz);
        send_lg_state_debug_trace_and_value("pmsg->data.liDiskFreeSz BEFORE hton", (unsigned long)pmsg->data.liDiskFreeSz);
        send_lg_state_debug_trace_and_value("pmsg->data.liActualSz BEFORE hton", (unsigned long)pmsg->data.liActualSz);
        send_lg_state_debug_trace_and_value("pmsg->data.isSource BEFORE hton", (unsigned long)pmsg->data.isSource);
        send_lg_state_debug_trace_and_value("pmsg->data.iReplGrpSourceIP BEFORE hton", (unsigned long)pmsg->data.iReplGrpSourceIP);
        send_lg_state_debug_trace( "---------------" );
    }
    
	mmp_convert_TdmfReplGroupMonitor_hton(&pmsg->data);
    
    if( activate_debug_traces )
    {
        send_lg_state_debug_trace_and_value("GROUP NUMBER pmsg->data.sReplGrpNbr AFTER hton", (unsigned long)pmsg->data.sReplGrpNbr);
        send_lg_state_debug_trace(pmsg->data.szServerUID);
        send_lg_state_debug_trace_and_value("pmsg->data.liDiskTotalSz AFTER hton", (unsigned long)pmsg->data.liDiskTotalSz);
        send_lg_state_debug_trace_and_value("pmsg->data.liDiskFreeSz AFTER hton", (unsigned long)pmsg->data.liDiskFreeSz);
        send_lg_state_debug_trace_and_value("pmsg->data.liActualSz AFTER hton", (unsigned long)pmsg->data.liActualSz);
        send_lg_state_debug_trace_and_value("pmsg->data.isSource AFTER hton", (unsigned long)pmsg->data.isSource);
        send_lg_state_debug_trace_and_value("pmsg->data.iReplGrpSourceIP AFTER hton", (unsigned long)pmsg->data.iReplGrpSourceIP);
        send_lg_state_debug_trace_and_value(">>> MESSAGE OPCODE AFTER hton (pmsg->hdr.mngttype)", (unsigned long)pmsg->hdr.mngttype);
    }
    
	towrite = sizeof(mmp_mngt_TdmfReplGroupMonitorMsg_t);
    
    if( activate_debug_traces )
    {
        send_lg_state_debug_trace_and_value(">>> SENDING TO DMC; towrite", (unsigned long)towrite);
        send_lg_state_debug_trace( "--------------------------------------------" );
    }
    
	r = sock_sendtox(giTDMFCollectorIP, giTDMFCollectorPort, (char *)pmsg, towrite, 0, 0);
	if (r != towrite) {
		sprintf(logmsg, "sock_sendtox failed. r = %d errno = %d\n", r, errno);
		logout(4, F_send_lg_journal, logmsg);
	}
}

void
ftd_mngt_send_lg_state()
{
	static int		is_first = 1;
	int				lgnum, mptblsize, i, tbl_check;
	DIR				*dfd;
	struct			dirent *dent;
	struct			stat cfgstat;
	char			fullpath[MMP_MNGT_MAX_FILENAME_SZ];
	lg_journal_t	*jp;
	lg_journal_t	pre_jp;
	mp_table_t		*mptable, *tmptable;



	mptable = NULL;
	mptblsize = 0;
	if ((dfd = opendir(DTCCFGDIR)) == (DIR*)NULL){
		sprintf(logmsg, "cannot open %s directory.\n", DTCCFGDIR);
		logout(4, F_ftd_mngt_send_lg_state, logmsg);
		return;
	}
	while ((dent = readdir(dfd)) != NULL) {
		if((lgnum = check_cfg_filename(dent, SECONDARY_CFG)) == -1) continue;

		sprintf (fullpath, "%s/%s", DTCCFGDIR, dent->d_name);
		if (0 != stat (fullpath, &cfgstat)) continue;
		jp = &lg_journal[lgnum];
		pre_jp.disktotal = jp->disktotal;
		pre_jp.diskavail = jp->diskavail;
		pre_jp.useingrp = jp->useingrp;
		if(is_first || jp->timestamp != cfgstat.st_mtime){
			if (getjournaldirname(jp, lgnum) != 0) {
				continue;
			}
			if (getdiskuseinfo(jp, lgnum) != 0) {
				continue;
			}
			if ((jp->sourceIP = getReplGrpSourceIP(lgnum)) == -1) {
				continue;
			}

			jp->timestamp = cfgstat.st_mtime;
			if(!is_first){
				for(i=0;i<mptblsize;++i){
					tmptable = mptable + i;
					if(strcmp(jp->mountpoint, tmptable->mountpoint) == 0){
						break;
					}
				}
				if(i == mptblsize){
					mptable = (mp_table_t *)realloc(mptable, sizeof(mp_table_t) * (mptblsize + 1));
					if(mptable == NULL){
						logoutx(1, F_ftd_mngt_send_lg_state, "realloc error", "errno", errno);
						closedir (dfd);
						return;
					}
					tmptable = mptable + mptblsize;
					strcpy(tmptable->mountpoint, jp->mountpoint);
					tmptable->disktotal = jp->disktotal;
					tmptable->diskavail = jp->diskavail;
					++mptblsize;
				}
			}
		} else {
			for(i=0;i<mptblsize;++i){
				tmptable = mptable + i;
				if(strcmp(jp->mountpoint, tmptable->mountpoint) == 0){
					jp->disktotal = tmptable->disktotal;
					jp->diskavail = tmptable->diskavail;
					break;
				}
			}
			if(i == mptblsize){
				if (getdiskuseinfo(jp, lgnum) != 0) {
					continue;
				}
				mptable = (mp_table_t *)realloc(mptable, sizeof(mp_table_t) * (mptblsize + 1));
				if(mptable == NULL){
					logoutx(1, F_ftd_mngt_send_lg_state, "realloc error", "errno", errno);
					closedir (dfd);
					return;
				}
				tmptable = mptable + mptblsize;
				strcpy(tmptable->mountpoint, jp->mountpoint);
				tmptable->disktotal = jp->disktotal;
				tmptable->diskavail = jp->diskavail;
				++mptblsize;
			}
		}

		if (getjournalfilevalue(jp, lgnum) != 0) {
			continue;
		}

		if(jp->disktotal != pre_jp.disktotal ||
					jp->diskavail != pre_jp.diskavail ||
					jp->useingrp != pre_jp.useingrp){

			send_lg_journal(jp, lgnum);

		} 
	}

	is_first = 0;
	closedir (dfd);
	if(mptable != NULL) free(mptable);

	return;
}

#endif /* _FTD_MNGT_SEND_LG_STATE_C_ */
