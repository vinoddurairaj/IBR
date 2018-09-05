/* #ident "@(#)$Id: Agent_uty.c,v 1.23 2003/11/13 02:48:20 FJjapan Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <stdlib.h>
#include <dirent.h>
#if defined(SOLARIS) || defined(HPUX)
#include <macros.h>
#endif
#if defined(HPUX)
#include <mntent.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "tdmfAgent.h"
#include "tdmfAgent_trace.h"

#if defined(HPUX)
#define FTD_SYSTEM_CONFIG_FILE ""DTCCFGDIR"/" QNM ".conf"
#elif defined(SOLARIS)                
#define FTD_SYSTEM_CONFIG_FILE "/usr/kernel/drv/" QNM ".conf"
#elif defined(_AIX)
#define FTD_SYSTEM_CONFIG_FILE "/usr/lib/drivers/" QNM ".conf"
#elif defined(linux)
#define MODULES_CONFIG_PATH "/etc/modules.conf"
#define FTD_SYSTEM_CONFIG_FILE ""PATH_DRIVER_FILES"/" QNM ".conf"
#endif

/* function return values, if anyone cares */
#define CFG_OK                  0
#define CFG_BOGUS_PS_NAME      -1
#define CFG_MALLOC_ERROR       -2
#define CFG_READ_ERROR         -3
#define CFG_WRITE_ERROR        -4
#define CFG_BOGUS_CONFIG_FILE  -5

#define CFG_IS_NOT_STRINGVAL   0
#define CFG_IS_STRINGVAL       1

#define CFG_MAX_KEY_SIZE       32
#define CFG_MAX_DATA_SIZE      1024 
 
int cfg_get_key_value(char *key, char *value, int stringval);
int cfg_set_key_value(char *key, char *value, int stringval);

void getnetconfs(unsigned long *iplist);
void cnvmsg(char *,int);
char *parmcpy(char *,char *,int,char*,char*);
void abort_Agn();
void atollx(char [],unsigned long long *);
void lltoax(char [],unsigned long long);
void rv4(unsigned char []); 
int getversion_num();
void htonI64(ftd_int64 *);

int     dbg;
FILE	*log;
int  Debug;
int RequestID;
int Alarm;

extern char    logmsg[];

static char *getline (char **buffer, int *linelen);

/*
 * cfg_get_software_key_value --
 */
int cfg_get_software_key_value(char *key,       /* get key */
            char *data, int size)               /* read data, data size */
{
	FILE *cfg;
	int rc = -1;
	int i,j;
	int keylen;
	char text[CFG_MAX_KEY_SIZE + CFG_MAX_DATA_SIZE];
	
	cfg = fopen(AGN_CFGFILE,"r");
	if (cfg == NULL)
	{
		return (-1);
	}
	if ((keylen = strlen(key)) > CFG_MAX_KEY_SIZE) {
		sprintf(logmsg, "The key length of %s exceeds %d.\n", key, CFG_MAX_KEY_SIZE);
		logout(6, F_cfg_get_software_key_value, logmsg);
		fclose(cfg);
		return (-1);
	}
	for(;;)
	{
		memset(text,0x00,sizeof(text));
		fgets(text,sizeof(text),cfg);
		if ((i = strlen(text)) == 0)
		{
			break;
		}
                for(j=0;j<i;j++)
                {
                    if (text[j] == '\n') text[j] = 0x00;
                }
		if (strncmp(key,text,keylen) == 0) 
		{
			memset(data,0x00,size);
			i = keylen;
			j = strlen(text);
			if (j-i-1 > size)
			{
				sprintf(logmsg, "The data length of %s exceeds %d.\n", key, size);
				logout(6, F_cfg_get_software_key_value, logmsg);
				fclose(cfg);
				return (-1);
			}
				
			strcpy(data,&text[i+1]);
			rc = 0;
		}
	}
	fclose(cfg);
	return rc;
	
}

/*
 * cfg_set_software_key_value --
 */
int cfg_set_software_key_value(char *key,       /* get key */
            char *data)                      /* read data */
{
        int     rc = 0;
        int     flag = 0;
        int     freeflag = 1;
        FILE    *infd;
        FILE    *outfd;
        char    buf[1024+1];
#if defined(linux)
		char    tmpfile[PATH_MAX] = ""AGTTMPDIR"/dua_tmpcfg_XXXXXX";
		int     tmp_flag = -1;
#else
        char    *tmpfile = NULL;
#endif
        /* copy services file */
#if defined(linux)
		tmp_flag = mkstemp(tmpfile);
		if (tmp_flag < 0)
#else
        tmpfile = (char *)tempnam(AGTTMPDIR,"dua");
        if (tmpfile == NULL)
#endif
        {
                /* tmp file alloc error */
#if defined(linux)
				strcpy(tmpfile, PRE_AGN_CFGFILE);
#else
                tmpfile = PRE_AGN_CFGFILE;
#endif
                freeflag = 0;
        }
#if defined(linux)
        sprintf(buf,"/bin/cp %s %s 1>/dev/null 2> /dev/null",AGN_CFGFILE, tmpfile);
#else
        sprintf(buf,"/usr/bin/cp %s %s 1>/dev/null 2> /dev/null",AGN_CFGFILE, tmpfile);
#endif
        system(buf);

        /* opem config file */
        outfd = fopen(AGN_CFGFILE,"w");
        if (outfd <= (FILE *)NULL)
	{
		/* config file open error */
        	unlink(tmpfile);
#if !defined(linux)
        	if (freeflag != 0) free(tmpfile);
#endif
		return (-1);
	}
        infd = fopen(tmpfile,"r");
        if (infd <= (FILE *)0)
        {
                /* file open error */
                fclose (outfd);
                outfd = NULL;
        	unlink(tmpfile);
#if !defined(linux)
        	if (freeflag != 0) free(tmpfile);
#endif
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
                if (strncmp(key,buf,strlen(key)) == 0)
                {
                        sprintf(buf,"%s:%s\n",key,data);
                        flag = 1;

                }
                fputs(buf,outfd);
        }

        /* request key not sprcified ? */
        if (flag == 0)
        {
                sprintf(buf,"%s:%s\n",key,data);
                fputs(buf,outfd);
        }
        fclose(infd);
        fclose(outfd);
        unlink(tmpfile);
#if !defined(linux)
        if (freeflag != 0) free(tmpfile);
#endif
        return(rc);
}

/*
 * ipstring_to_ip -- convert ip address string to ip address
 */
int
ipstring_to_ip(char *name, unsigned long *ip)
{
        if (name == NULL || strlen(name) == 0) {
                return -1;
        }

        {
                int             n1, n2, n3, n4;

                char    *s, *lname = strdup(name);

                if ((s = strtok(lname, ".\n")))
                        n1 = atoi(s);
                if ((s = strtok(NULL, ".\n")))
                        n2 = atoi(s);
                if ((s = strtok(NULL, ".\n")))
                        n3 = atoi(s);
                if ((s = strtok(NULL, ".\n")))
                        n4 = atoi(s);

        free(lname);

                *ip = n4 + (n3 << 8) + (n2 << 16) + (n1 << 24);

        }

        return 0;
}

/*
 * ip_to_name -- convert ip address to hostname
 */
int
ip_to_name(unsigned long ip, char *name)
{
        struct hostent  *host;
        unsigned long   addr;
        char                    ipstring[32], **p;

        ip_to_ipstring(ip, ipstring);

        addr = inet_addr(ipstring);
        host = gethostbyaddr((const char *)&addr, sizeof(addr), AF_INET);

        if (host == NULL) {
                return -1;
        }
        p = host->h_addr_list;
        strcpy(name, host->h_name);
   
        return 0;
}


/*
 * name_to_ip -- convert hostname to ip address
 */
int
name_to_ip(char *name, unsigned long *ip)
{
        struct hostent *host; 
        struct in_addr in;
        char **p;  

        if (name == NULL || strlen(name) == 0) {
                return -1;
        }
        
        host = gethostbyname(name);
      
        if (host == NULL) {
                return -1;
        }
        
        p = host->h_addr_list;
   
        memcpy(&in.s_addr, *p, sizeof(in.s_addr));
        *ip = in.s_addr;

        return 0;
}


/*              
 * sock_is_me - figures out if the target IP address contained in the
 *              sock_t object is this machine
 *        1 = the ip addr pertains to this machine
 *        0 = the ip addr does not pertain to this machine
 *       -1 = invalid IP addr
 */             
int
sock_is_me(sock_t *sockp, int local)
{                       
        u_long                  *ips = NULL, ip;
        int                             ipcount, ipretval;

        ipretval = -1;
                        
        ipcount = 0;
        ipcount = getnetconfcount();
                               
        if (ipcount > 0) {
                ips = (u_long*)malloc(ipcount*sizeof(u_long));
                getnetconfs(ips);
        }
                                
        if (local) {
                ip = sockp->lip; 
        } else { 
                ip = sockp->rip;
        }

        if (ip <= 0) {
                return ipretval; 
        }        
                
        if (ip == LOCALHOSTIP) {
                return 1;
        }

        {
                int i = 0;

                ipretval = 0;

                while(ips[i]) {
                        if (ips[i] == ip) {
                                ipretval = 1;
                                break;
                        }
                        i++;
                }
        }

        free(ips);

        return ipretval;
}


/*
 * sock_ip_to_ipstring -- convert ip to ipstring in dot notation  
 */
int
sock_ip_to_ipstring(unsigned long ip, char *ipstring) {
   
        ip_to_ipstring(ip, ipstring);

        return 0;
}

/*
 * ip_to_ipstring -- convert ip address to ipstring in dot notation
 *
 *                   AC, 2002-04-10 : bug correction:
 *                   Make sure ip value is parsed as a network byte order
 *                   and not Intel (little endian) byte order
 */
int 
ip_to_ipstring(unsigned long ip, char *ipstring)
{ 
    int a1, a2, a3, a4;
    a1 = 0x000000ff & (ip >> 24);
    a2 = 0x000000ff & (ip >> 16);
    a3 = 0x000000ff & (ip >> 8);
    a4 = 0x000000ff & ip;
    sprintf(ipstring, "%d.%d.%d.%d", a1, a2, a3, a4);
    return (0);
}


/*
 * name_is_ipstring -- determine if the hostname is an ip address string
 */
int
name_is_ipstring(char *name)
{
        int     i;
    
        int len = strlen(name);
  
        for (i=0;i<len;i++) {
                if (isalpha(name[i])) {
                        return 0;
                }
        }
      
        return 1;
}       


/****************************************************************************
 * reporterr -- report an error
 ***************************************************************************/
void
_reporterr(char *facility, char *name, int level, ...)
{
#if 0
        va_list args;
        char fmt[256];
        char msg[256];
        va_start(args, level);
        if (0 == geterrmsg(facility, name, fmt))  
                {
                vsprintf(msg, fmt, args);
                if (log_errs)
                        logerrmsg(facility, level, name, msg);
                }
        else
                {
                if (log_errs)  
                         logerrmsg(facility, ERRWARN, name, fmt);
                }
#endif
}


void mmp_convert_mngt_hdr_ntoh(mmp_mngt_header_t *hdr)
{
    hdr->magicnumber = ntohl(hdr->magicnumber);
    hdr->mngttype    = ntohl(hdr->mngttype);
    hdr->sendertype  = ntohl(hdr->sendertype);
    hdr->mngtstatus  = ntohl(hdr->mngtstatus);
}

void mmp_convert_mngt_hdr_hton(mmp_mngt_header_t *hdr)
{
    hdr->magicnumber = htonl(hdr->magicnumber);
    hdr->mngttype    = htonl(hdr->mngttype);
    hdr->sendertype  = htonl(hdr->sendertype);
    hdr->mngtstatus  = htonl(hdr->mngtstatus);
}

void mmp_convert_TdmfServerInfo_hton(mmp_TdmfServerInfo *srvrInfo)
{
    srvrInfo->iPort             =   htonl(srvrInfo->iPort);
    srvrInfo->iTCPWindowSize    =   htonl(srvrInfo->iTCPWindowSize);
    srvrInfo->iBABSizeReq       =   htonl(srvrInfo->iBABSizeReq);
    srvrInfo->iBABSizeAct       =   htonl(srvrInfo->iBABSizeAct);
    srvrInfo->iNbrCPU           =   htonl(srvrInfo->iNbrCPU);
    srvrInfo->iRAMSize          =   htonl(srvrInfo->iRAMSize);
    srvrInfo->iAvailableRAMSize =   htonl(srvrInfo->iAvailableRAMSize);
}       

void    mmp_convert_TdmfPerfConfig_ntoh(mmp_TdmfPerfConfig *perfCfg)
{
    perfCfg->iPerfUploadPeriod   = ntohl(perfCfg->iPerfUploadPeriod);
    perfCfg->iReplGrpMonitPeriod = ntohl(perfCfg->iReplGrpMonitPeriod);
}

void mmp_convert_TdmfAgentDeviceInfo_hton(mmp_TdmfDeviceInfo *devInfo)
{
    devInfo->sFileSystem    = htons(devInfo->sFileSystem);
}

void    mmp_convert_TdmfAlert_hton(mmp_TdmfAlertHdr *alertData)
{
    alertData->sLGid        = htons(alertData->sLGid);
    alertData->sDeviceId    = htons(alertData->sDeviceId);
    alertData->uiTimeStamp  = htonl(alertData->uiTimeStamp);
    alertData->cSeverity    = (char)htons((short)alertData->cSeverity);
    alertData->cType        = (char)htons((short)alertData->cType);
}

void    mmp_convert_TdmfAlert_ntoh(mmp_TdmfAlertHdr *alertData)
{
    alertData->sLGid        = ntohs(alertData->sLGid);
    alertData->sDeviceId    = ntohs(alertData->sDeviceId);
    alertData->uiTimeStamp  = ntohl(alertData->uiTimeStamp);
    alertData->cSeverity    = (char)ntohs((short)alertData->cSeverity);
    alertData->cType        = (char)ntohs((short)alertData->cType);
}

void    mmp_convert_TdmfStatusMsg_hton(mmp_TdmfStatusMsg *statusMsg)
{
    statusMsg->iLength      = htonl(statusMsg->iLength);
    statusMsg->iTdmfCmd     = htonl(statusMsg->iTdmfCmd);
    statusMsg->iTimeStamp   = htonl(statusMsg->iTimeStamp);
}

void    mmp_convert_TdmfReplGroupMonitor_hton(mmp_TdmfReplGroupMonitor *GrpMon)
{
	htonI64(&(GrpMon->liActualSz));
	htonI64(&(GrpMon->liDiskTotalSz));
	htonI64(&(GrpMon->liDiskFreeSz));
	GrpMon->iReplGrpSourceIP	= htonl(GrpMon->iReplGrpSourceIP);
    rv4((unsigned char *)&GrpMon->iReplGrpSourceIP);
    GrpMon->sReplGrpNbr		= htons(GrpMon->sReplGrpNbr);
    GrpMon->isSource		= (char)htons(GrpMon->isSource);
}

void    mmp_convert_TdmfGroupState_hton(mmp_TdmfGroupState *state)
{ 
    state->sRepGrpNbr	    = ntohs(state->sRepGrpNbr);
    state->sState	    = ntohs(state->sState);
}

void
htonI64(ftd_int64 *int64InVal)
{   
	ftd_int64 int64OutVal;
    unsigned int *pIntOutVal  = (unsigned int*)&int64OutVal;
	unsigned int *pIntInVal  = (unsigned int*)int64InVal;
	int n = 1;

	if( *(char*)&n ){	/* Little endian */
		pIntOutVal++;
		*pIntOutVal     =   htonl( *pIntInVal );/* first 32-bits */
		pIntInVal++;
		pIntOutVal--;
		*pIntOutVal     =   htonl( *pIntInVal );/* last 32-bits */
	} else {			/* Big endian */
		*pIntOutVal     =   htonl(*pIntInVal);/*first 32-bits */
		pIntOutVal++;     
		pIntInVal++;
		*pIntOutVal     =   htonl(*pIntInVal);/*last 32-bits */
	}
    *int64InVal = int64OutVal;  
}


void
mmp_convert_perf_instance_hton(ftd_perf_instance_t *perf)
{ 
    perf->connection =   (int)htonl(perf->connection);
    perf->drvmode    =   (int)htonl(perf->drvmode);
    perf->lgnum      =   (int)htonl(perf->lgnum);
    perf->insert     =   (int)htonl(perf->insert);
    perf->devid      =   (int)htonl(perf->devid);
	htonI64(&(perf->actual));
	htonI64(&(perf->effective));
    perf->rsyncoff   =   (int)htonl(perf->rsyncoff);
    perf->rsyncdelta =   (int)htonl(perf->rsyncdelta);
    perf->entries    =   (int)htonl(perf->entries);
    perf->sectors    =   (int)htonl(perf->sectors);
    perf->pctdone    =   (int)htonl(perf->pctdone);
    perf->pctbab     =   (int)htonl(perf->pctbab);
	htonI64(&(perf->bytesread));
	htonI64(&(perf->byteswritten));
}

void mmp_convert_TdmfServerInfo_ntoh(mmp_TdmfServerInfo *srvrInfo)
{           
    srvrInfo->iPort             =   ntohl(srvrInfo->iPort);
    srvrInfo->iTCPWindowSize    =   ntohl(srvrInfo->iTCPWindowSize);
    srvrInfo->iBABSizeReq       =   ntohl(srvrInfo->iBABSizeReq);
    srvrInfo->iBABSizeAct       =   ntohl(srvrInfo->iBABSizeAct);
    srvrInfo->iNbrCPU           =   ntohl(srvrInfo->iNbrCPU);
    srvrInfo->iRAMSize          =   ntohl(srvrInfo->iRAMSize);
    srvrInfo->iAvailableRAMSize =   ntohl(srvrInfo->iAvailableRAMSize);
}

/* WR15560 : add function  start */
void    mmp_convert_hton(ftd_perf_instance_t *perf)
{
    perf->connection =   (int)htonl(perf->connection);
    perf->drvmode    =   (int)htonl(perf->drvmode);
    perf->lgnum      =   (int)htonl(perf->lgnum);
    perf->insert     =   (int)htonl(perf->insert);
    perf->devid      =   (int)htonl(perf->devid);
	htonI64(&(perf->actual));
	htonI64(&(perf->effective));
    perf->rsyncoff   =   (int)htonl(perf->rsyncoff);
    perf->rsyncdelta =   (int)htonl(perf->rsyncdelta);
    perf->entries    =   (int)htonl(perf->entries);
    perf->sectors    =   (int)htonl(perf->sectors);
    perf->pctdone    =   (int)htonl(perf->pctdone);
    perf->pctbab     =   (int)htonl(perf->pctbab);
	htonI64(&(perf->bytesread));
	htonI64(&(perf->byteswritten));
}
/* WR15560 : add function  end */

void mmp_convert_FileTransferData_hton(mmp_TdmfFileTransferData *data)
{
	data->iType =	(int)htonl(data->iType);
	data->uiSize =	(unsigned int)htonl(data->uiSize);
}

void rv2(char value[])
{
        char    c;

        c = value[0];
        value[0] = value[1];
        value[1] = c;
        return;
}

void rv4(unsigned char value[])
{
		unsigned char c;

		c = value[3];
		value[3] = value[0];
		value[0] = c;
		c = value[2];
		value[2] = value[1];
		value[1] = c;
		return;
}


/*
 * cfg_intr.c - system config file interface
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

/*
 * Parse the system config file for something like: key=value;
 *
 * If stringval is TRUE, then remove quotes from the value.
 */
int
cfg_get_key_value(char *key, char *value, int stringval)
{
    int         fd, len;
    char        *ptr, *buffer, *temp, *line;
    struct stat statbuf;

    if (stat(FTD_SYSTEM_CONFIG_FILE, &statbuf) != 0) {
        return CFG_BOGUS_CONFIG_FILE;
    }

    /* open the config file and look for the pstore key */
    if ((fd = open(FTD_SYSTEM_CONFIG_FILE, O_RDONLY)) == -1) {
        return CFG_BOGUS_CONFIG_FILE;
    }

    if ((buffer = (char*)malloc(2*statbuf.st_size)) == NULL) {
        close(fd);
        return CFG_MALLOC_ERROR;
    }

    /* read the entire file into the buffer. */
    if (read(fd, buffer, statbuf.st_size) != statbuf.st_size) {
        close(fd);
        free(buffer);
        return CFG_READ_ERROR;
    }
    close(fd);
    temp = buffer;

    /* get lines until we find the right one */
    while ((line = getline(&temp, &len)) != NULL) {
        if (line[0] == '#') {
            continue;
        } else if ((ptr = strstr(line, key)) != NULL) {
            /* search for quotes, if this is a string */
            if (stringval) {
                while (*ptr) {
                    if (*ptr++ == '\"') {
                        while (*ptr && (*ptr != '\"')) {
                            *value++ = *ptr++;
                        }
                        *value = 0;
                        free(buffer);
                        return CFG_OK;
                    }
                }
            } else {
                while (*ptr) {
                    if (*ptr++ == '=') {
                        while (*ptr && (*ptr != ';')) {
                            *value++ = *ptr++;
                        }
                        free(buffer);
                        return CFG_OK;
                    }
                }
            }
        }
    }
    free(buffer);
    return CFG_BOGUS_CONFIG_FILE;
}

/*
 * Parse the system config file for something like: key=value;
 * Replace old value with new value. If key doesn't exist, add key/value.
 *
 * If stringval is TRUE, put quotes around the value.
 */
int
cfg_set_key_value(char *key, char *value, int stringval)
{
    int         fd, found, linelen;
    char        *inbuffer, *outbuffer, *line;
    char        *tempin, *tempout;
    struct stat statbuf;

    if (stat(FTD_SYSTEM_CONFIG_FILE, &statbuf) != 0) {
        return CFG_BOGUS_CONFIG_FILE;
    }

    if ((fd = open(FTD_SYSTEM_CONFIG_FILE, O_RDWR)) == NULL) {
        return CFG_BOGUS_CONFIG_FILE;
    }

    if ((inbuffer = (char *)calloc(statbuf.st_size+1, 1)) == NULL) {
        close(fd);
        return CFG_MALLOC_ERROR;
    }

    if ((outbuffer = (char *)malloc(statbuf.st_size+MAXPATHLEN)) == NULL) {
        close(fd);
	free(inbuffer);
	inbuffer = NULL;
        return CFG_MALLOC_ERROR;
    }

    /* read the entire file into the buffer. */
    if (read(fd, inbuffer, statbuf.st_size) != statbuf.st_size) {
        close(fd);
	free(inbuffer);
	free(outbuffer);
	inbuffer = NULL;
	outbuffer = NULL;
        return CFG_READ_ERROR;
    }
    close(fd);
    tempin = inbuffer;
    tempout = outbuffer;
    found = 0;

    /* get lines until we find one with: key=something */
    while ((line = getline(&tempin, &linelen)) != NULL) {
        /* if this is a comment line or it doesn't have the magic word ... */
        if ((line[0] == '#') || (strstr(line, key) == NULL)) {
            /* copy the line to the output buffer */
            strncpy(tempout, line, linelen);
            tempout[linelen] = '\n';
            tempout += linelen+1;
        } else {
            if (stringval) {
                sprintf(tempout, "%s=\"%s\";\n", key, value);
            } else {
                sprintf(tempout, "%s=%s;\n", key, value);
            }
            tempout += strlen(tempout);
            found = 1;
        }
    }
    free(inbuffer);

    if (!found) {
        if (stringval) {
            sprintf(tempout, "%s=\"%s\";\n", key, value);
        } else {
            sprintf(tempout, "%s=%s;\n", key, value);
        }
        tempout += strlen(tempout);
    }

    /* flush the output buffer to disk */
    if ((fd = creat(FTD_SYSTEM_CONFIG_FILE, S_IRUSR | S_IWUSR)) == -1) {
        free(outbuffer);
	outbuffer = NULL;
        return CFG_WRITE_ERROR;
    }
    lseek(fd, 0, SEEK_SET);
    write(fd, outbuffer, tempout - outbuffer);
    close(fd);

    free(outbuffer);

    return CFG_OK;
}

/*
 * Yet another "getline" function. Parses a buffer looking for the
 * EOL or a NULL. Bumps the buffer pointer to the character following
 * EOL or to NULL, if end-of-buffer. Replaces the EOL with a NULL.
 *
 * Returns NULL, if parsing failed (end-of-buffer reached).
 * Returns pointer to start-of-line, if parsing succeeded.
 */
static char *
getline (char **buffer, int *outlen)
{
    int  len;
    char *tempbuf;

/* XXX FIXME: I don't think that the outer while(1) is needed at all, since
 * there is no way for it to terminate or continue...  ifdef it out
 * to keep the compiler happy.  if getline breaks, then back out this change.
 * warner
 */
#ifdef NOT_NEEDED
    while (1) {
#endif
        tempbuf = *buffer;
        if (tempbuf == NULL) {
            return NULL;
        }

        /* search for EOL or NULL */
        len = 0;
        while (1) {
            if (tempbuf[len] == '\n') {
                tempbuf[len] = 0;
                *buffer = &tempbuf[len+1];
                break;
            } else if (tempbuf[len] == 0) {
                /* must be done! */
                *buffer = NULL;
                break;
            }
            len++;
        }
        if ((*outlen = len) == 0) {
            return NULL;
        }

        /* done */
        return tempbuf;
#ifdef NOT_NEEDED
    }
#endif
}

/*************************************************************/
/*                                                           */
/*    file lock func                                         */
/*                                                           */
/*************************************************************/

static struct sembuf op_lock[2] = {
	2,0,0,
	2,1,SEM_UNDO
};

static struct sembuf op_endcreate[2] = {
	1,-1,SEM_UNDO,
	2,-1,SEM_UNDO
};

static struct sembuf op_open[1] = {
	1,-1,SEM_UNDO,
};

static struct sembuf op_close[3] = {
	2,0,0,
	2,1,SEM_UNDO,
	1,1,SEM_UNDO
};

static struct sembuf op_unlock[1] = {
	2,-1,SEM_UNDO
};

static struct sembuf op_op[1] = {
	0,99,SEM_UNDO
};

int sem_create(key_t key,int initval)
{
	register int id,semval;
	union semun {
		int val;
		struct semid_ds *buf;
		ushort		*array;
	}semctl_arg;

	if (key == IPC_PRIVATE)
		return (-1);
	else if (key == (key_t) -1)
		return (-1);
again:
	if (id = semget(key,3,0666 | IPC_CREAT) < 0)
		return (-1);
	if (semop(id,&op_lock[0],2) < 0) {
		if (errno == EINVAL)
			goto again;
		logout(19,F_sem_create,"can not lock.\n");
	}

	if ((semval = semctl(id,1,GETVAL,0)) < 0)
		logout(19,F_sem_create,"semctl GETVAL error\n");

	if (semval == 0) {
		semctl_arg.val = initval;
		if (semctl(id,0,SETVAL,semctl_arg) < 0)
		        logout(19,F_sem_create,"semctl SETVAL[0] error\n");
		semctl_arg.val = BIGCOUNT;
		if (semctl(id,1,SETVAL,semctl_arg) < 0)
		        logout(19,F_sem_create,"semctl SETVAL[1] error\n");
	}

	if (semop(id, &op_endcreate[0],2) < 0)
		logout(19,F_sem_create,"can not endcreate.\n");
	return(id);
}

sem_rm(int id)
{
	if(semctl(id,0,IPC_RMID,0) < 0)
		logout(19,F_sem_create,"semctl IPC_RMID error\n");
}

sem_close(int id)
{
	register int semval;

	if (semop(id, &op_close[0],3) < 0)
		logout(19,F_sem_create,"can not close.\n");

	if ((semval = semctl(id,1,GETVAL,0)) < 0)
		logout(19,F_sem_create,"semctl GETVAL error\n");

	if (semval > BIGCOUNT)
		logout(19,F_sem_create,"sem[1] > BIGCOUNT.\n");
	else if (semval == BIGCOUNT)
		sem_rm(id);
	else 
		if (semop(id, &op_unlock[0],1) <0)
			logout(19,F_sem_create,"can not unlock.\n");
	
}

sem_wait(int id)
{
	sem_op(id, -1);
}

sem_signal(int id)
{
	sem_op(id,1);
}

sem_op(int id,int value)
{
	if ((op_op[0].sem_op = value) == 0)
		logout(19,F_sem_op,"value = 0.\n");

	if (semop(id, &op_op[0],1) < 0)
		logout(19,F_sem_op,"can not op.\n");
}

#if 0
void cnvmsg(char *text)
{

        char *tmpbuf;
        char *tmpbuf_p;

        char hostname[20];
        char time[64+1];
        char buf[64+1];
	memset(hostname,0x00,sizeof(hostname));
        gethostname(hostname,sizeof(hostname));
        tmpbuf = (char *)malloc(strlen(text) +1);
	tmpbuf_p = tmpbuf;

        strcpy(tmpbuf,text);
        memset(text,0x00,strlen(text)+1);

        memset(time,0x00,sizeof(time));
        tmpbuf = parmcpy(tmpbuf,time,sizeof(time),"[","]");
        strcpy(text,time);
        strcat(text," Replicator: [");
        strcat(text,hostname);
        tmpbuf = (char *)strstr(tmpbuf,":");
        memset(buf,0x00,sizeof(buf));
        tmpbuf = parmcpy(tmpbuf,buf,sizeof(buf)," ","]");
        strcat(text,buf);
        tmpbuf = (char *)strstr(tmpbuf,":");
        tmpbuf++;
        tmpbuf = (char *)strstr(tmpbuf,":");
        tmpbuf++;
        tmpbuf = (char *)strstr(tmpbuf,":");
        tmpbuf++;
        strcat(text,tmpbuf);
	free (tmpbuf_p);
        return;


}
#endif

void cnvmsg(char *text,int size)
{
    /* format expected:   "[date-time...] [proc:...] [pid,...] [src,line...] DTC: [...]: [....]" */
    int textlen = strlen(text);
    char    *p0,*p1;
    char    datetime[64];
    char    hostname[20];
    char    proc[256];
    char    *msg, *msgcopy;
    int     msgcopylen,n;
#if defined(HPUX) && SYSVERS == 1020
	int		msglen;
	char	*tmptxt;
#endif

    p0 = strchr(text,'[');
    if ( p0 == NULL )   return; /*not the expected format*/
    p1 = strchr(p0,']');
    if ( p1 == NULL )   return; /*not the expected format*/
    strncpy(datetime, p0, p1 - p0 + 1 );
    datetime[ p1 - p0 + 1 ] = 0;


    p0 = strstr(p1,"[proc:");
    if ( p0 == NULL )   return; /*not the expected format*/
    p1 = strchr(p0,']');
    if ( p1 == NULL )   return; /*not the expected format*/
    strncpy(proc, p0+6, p1 - (p0+6) );
    proc[ p1 - (p0+6) ] = 0;


    p0 = strstr(p1,"DTC:");
    if ( p0 == NULL )   return; /*not the expected format*/
    msg = p0 + 4;

    msgcopylen = textlen-(msg-text);
    msgcopy = (char*)malloc( msgcopylen + 1);
    if ( msgcopy == NULL )  return; 
    strcpy( msgcopy, msg );

	memset(hostname,0x00,sizeof(hostname));
    gethostname(hostname,sizeof(hostname));

    /* rebuild text */
#if defined(HPUX) && SYSVERS == 1020
	msglen = strlen(datetime) + strlen(hostname) + strlen(proc) + msgcopylen + 12;
	tmptxt = (char*)malloc(msglen);
	sprintf(tmptxt, "%s Replicator: [%s:%s] %s",datetime, hostname, proc, msgcopy);
	strncpy(text, tmptxt, size);
	text[size - 1] = '\0';
	free(tmptxt);
#else
    snprintf(text, size, "%s Replicator: [%s:%s] %s",datetime, hostname, proc, msgcopy);
#endif

    free(msgcopy);
}

char *parmcpy(char *buf,char *copyarea,int arealen,char *start,char *end)
{
   int        copylen;
   int        pare = 0;
    /* 1.    PARAMETA CHECK             */
   memset(copyarea,0x00,arealen);
   for(copylen = 0;copylen <= arealen;copylen++) {
        if (*buf == *start) {
                pare = pare + 1;
                if (*start == 0x20)
                {
                        buf = buf +1;
                        continue;
                }
        }
        if (*buf == *end) {
                pare = pare -1;
                if (pare == 0) {
                        if (copylen == 0) {
                                goto proc_end;
                        }
                        else
                        {
                                *copyarea = *buf;
                                buf = buf + 1;
                        }
                }
                break;
        }
        else if (*buf == '\0') {
                goto proc_end;
        }
        *copyarea = *buf;
        copyarea = copyarea + 1;
        buf = buf + 1;
   }
   if (copylen > arealen) {
   }
proc_end:
   buf = buf +1;
   return(buf);
}

void atollx(char convtext[],unsigned long long *bin)
{
        unsigned long long      rc = 0;
        int                     i = 0;

	
        for(i = 0 ; isdigit(convtext[i]) != 0 ; i++)
        {
                rc = rc * 10;
                switch(convtext[i])
                {
                case '1' :
                        rc = rc + 1;
                        break;
                case '2' :
                        rc = rc + 2;
                        break;
                case '3' :
                        rc = rc + 3;
                        break;
                case '4' :
                        rc = rc + 4;
                        break;
                case '5' :
                        rc = rc + 5;
                        break;
                case '6' :
                        rc = rc + 6;
                        break;
                case '7' :
                        rc = rc + 7;
                        break;
                case '8' :
                        rc = rc + 8;
                        break;
                case '9' :
                        rc = rc + 9;
                        break;
                }
        }
	*bin = rc;
        return;
}


void lltoax(char text[],unsigned long long convbin)
{
        int                     i = 0;
        int                     j = 0;
        char                    tmpbuf[512+1];
        char                    tmpint[4+1];

        memset(tmpbuf,0x00,sizeof(tmpbuf));
	/* Zero check */
	if (convbin == 0)
	{
		sprintf(text,"0");
	}
	else
	{
        	for(i = 0 ; convbin != 0 ; i++)
        	{
                	j = convbin%10;
               	 	convbin = convbin /10;
                	sprintf(tmpint,"%d",j);
                	tmpbuf[i] = tmpint[0];
        	}
        	j = 0;
        	text[i] = 0x00;
        	i--;
        	for (;i >= 0; i--)
        	{
                	text[j] = tmpbuf[i];
                	j++;
        	}
	}
        return;
}

#if defined(HPUX)

/*
 * search /etc/mnttab (referenced by fp) for an entry that
 * matches the properties passed in mpref. For now, we only
 * support searching for a matching name (mnt_special). This
 * function starts at the current file position and iterates
 * through the file until a match or EOF is found. If EOF is
 * found, then it returns EOF. See manpage on Solaris for
 * full description of this function.
 */
int
getmntany(FILE *fp, struct mnttab *mp, struct mnttab *mpref)
{
    struct mntent *ent;
    int count, found;
    static char timebuf[16];

    /* check for stupidity */
    if ((mpref == NULL) || (mp == NULL)) {
        return EOF;
    }
    count = 0;
    if (mpref->mnt_mountp != NULL) count++;
    if (mpref->mnt_fstype != NULL) count++;
    if (mpref->mnt_mntopts != NULL) count++;
    if (mpref->mnt_time != NULL) count++;

    while ((ent = getmntent(fp)) != NULL) {
        found = 0;
        if ((mpref->mnt_mountp != NULL) && 
            (strcmp(ent->mnt_dir, mpref->mnt_mountp) == 0)) {
            found++;
        }
        if ((mpref->mnt_fstype != NULL) && 
            (strcmp(ent->mnt_fsname, mpref->mnt_fstype) == 0)) {
            found++;
        }
        if ((mpref->mnt_mntopts != NULL) && 
            (strcmp(ent->mnt_opts, mpref->mnt_mntopts) == 0)) {
            found++;
        }
        if ((mpref->mnt_time != NULL) && 
            ((atol(mpref->mnt_time) == ent->mnt_time) == 0)) {
            found++;
        }
        if (found == count) {
            mp->mnt_mountp = ent->mnt_dir;
            mp->mnt_fstype = ent->mnt_fsname;
            mp->mnt_mntopts = ent->mnt_opts;
            sprintf(timebuf, "%ld", ent->mnt_time);
            mp->mnt_time = timebuf; 
            return 0;
        }
    }
    return EOF;
}

#elif defined(_AIX)

#include <sys/mntctl.h>
#include <sys/vmount.h>

/* don't free(3) these... */
static char     timebuf[16];
static char     mnt_special_buf[MAXPATHLEN];
static char     mnt_mountp_buf[MAXPATHLEN];
static char     mnt_fstype_buf[16];
static char     mnt_mntopts_buf[16];

int
getmntany(FILE * fp, struct mnttab * mp, struct mnttab * mpref)
{
	struct mntent  *ent;
	int             count, found;


	int             mcerr;
	int             vmdesc;
	struct vmount  *vma=(struct vmount  *)NULL;
	struct vmount  *vmp;
	char           *vmlim;
	char           *vmtobjp;
	char           *vmtstbp;

	/* check for stupidity */
	if ((mpref == NULL) || (mp == NULL)) {
		return EOF;
	}
	count = 0;
	if (mpref->mnt_special != NULL)
		count++;
	if (mpref->mnt_mountp != NULL)
		count++;
	if (mpref->mnt_fstype != NULL)
		count++;
	if (mpref->mnt_mntopts != NULL)
		count++;
	if (mpref->mnt_time != NULL)
		count++;

	/* query mntctl in descriptor mode */
	mcerr = mntctl(MCTL_QUERY, sizeof(vmdesc), (char *) &vmdesc);
	if (mcerr < 0)
		return (-1);

	/* vmdesc is the size of the buffer to allocate */
	vma = (struct vmount *) malloc(vmdesc);
	mcerr = mntctl(MCTL_QUERY, vmdesc, (char *) &vma[0]);
	if (mcerr <= 0) {
		if(vma)
			free(vma);
		return (-1);
	}

	/* scan for matching entry */
	vmp = vma;
	vmlim = ((char *) vma) + vmdesc;
	while ((char *) vmp < vmlim) {

		found = 0;

		vmtobjp = vmt2dataptr(vmp, VMT_OBJECT);
		vmtstbp = vmt2dataptr(vmp, VMT_STUB);

		{
			char           *typp;

			strncpy(mnt_special_buf, vmtobjp, MAXPATHLEN - 1);
			strncpy(mnt_mountp_buf, vmtstbp, MAXPATHLEN - 1);

			/* not sure how to represent fs types here... */
			switch (vmp->vmt_gfstype) {
			case MNT_AIX:
				typp = "oaix";
			case MNT_NFS:
				typp = "nfs";
				break;
			case MNT_JFS:
				typp = "jfs";
				break;
			case MNT_CDROM:
				typp = "cdrom";
				break;
			default:
				typp = "unknown";
				break;
			}
			strcpy(mnt_fstype_buf, typp);

			/* not sure how to represent flags here... */
			sprintf(mnt_mntopts_buf, "0x%08x", &vmp->vmt_flags);
		}
		if ((mpref->mnt_special != NULL) &&
		    (strcmp(vmtobjp, mpref->mnt_special) == 0)) {
			found++;
		}
		if ((mpref->mnt_mountp != NULL) &&
		    (strcmp(vmtstbp, mpref->mnt_mountp) == 0)) {
			found++;
		}
		if ((mpref->mnt_fstype != NULL) &&
		    (strcmp((const char *)vmp->vmt_gfstype, (const char *)mpref->mnt_fstype) == 0)) {
			found++;
		}
		if ((mpref->mnt_mntopts != NULL) &&
		    (strcmp((const char *)vmp->vmt_flags, (const char *)mpref->mnt_mntopts) == 0)) {
			found++;
		}
		if ((mpref->mnt_time != NULL) &&
		    ((atol(mpref->mnt_time) == vmp->vmt_time) == 0)) {
			found++;
		}
		if (found == count) {
			mp->mnt_special = mnt_special_buf;
			mp->mnt_mountp = mnt_mountp_buf;
			mp->mnt_fstype = mnt_fstype_buf;
			mp->mnt_mntopts = mnt_mntopts_buf;
			sprintf(timebuf, "%ld", vmp->vmt_time);
			mp->mnt_time = timebuf;
			if(vma)
				free(vma);
			return 0;
		}
		vmp = (struct vmount *) ((char *) vmp + vmp->vmt_length);
	}
	if(vma)
		free(vma);
	return (EOF);
}

#endif

void execCommand(char* cmd, char* result)
{
	int len;
	char data[256+1];
	FILE *fp;

	memset(data, '\0', sizeof(data));
	if( (fp = popen( cmd, "r" )) == NULL )
		return;
	fgets(data, sizeof(data), fp);
	if( (len = strlen(data)) <= 0 ){
		pclose( fp );
		return;
	}
	if( data[len - 1] == '\n' )
		data[len - 1] = '\0';
	if( strlen(data) <= 0 ){
		pclose( fp );
		return;
	}
	strcpy( result, data );
	pclose( fp );
	return;
}

int getversion_num()
{
	static int ver_n = 0;
	char	version[16];
	char	ver[8], build[8];
	char	*s;
	int		v1=0, v2=0, v3=0;
	int		b=0;

	if (ver_n != 0) {
		return ver_n;
	}
	memset(version, 0, sizeof(version));
	getversion(version);
	if (sscanf(version, "%s %s", ver, build) > 0) {
		if ((s = (char *)strtok(ver, ".\n")))
			v1 = (int)strtol(s, NULL, 0);
		if ((s = (char *)strtok(NULL, ".\n")))
			v2 = (int)strtol(s, NULL, 0);
		if ((s = (char *)strtok(NULL, ".\n")))
			v3 = (int)strtol(s, NULL, 0);
		b = (int)strtol(build, NULL, 10);

		ver_n = (v1 * 10000000) + (v2 * 100000) + (v3 * 1000) + b;
		return ver_n;
	} else {
		return 0;
	}
}

int check_cfg_filename(struct dirent *dent, int type)
{
	int i;
	char lgn[4];

	if (strlen(dent->d_name) != 8) return -1;
	switch (type) {
		case PRIMARY_CFG:
			if (dent->d_name[0] != 'p') return -1;
			break;
		case SECONDARY_CFG:
			if (dent->d_name[0] != 's') return -1;
			break;
		case ALL_CFG:
			if ((dent->d_name[0] != 'p') && (dent->d_name[0] != 's')) return -1;
			break;
	}
	for (i = 1; i <= 3; i++) {
		if (isdigit(dent->d_name[i]) == FALSE) {
			return -1;
		}
		lgn[i-1] = dent->d_name[i];
	}
	lgn[3] = '\0';
	if (strcmp(&(dent->d_name[i]),".cfg") != 0) return -1;
	/* filename check OK */
	return atoi(lgn);
}
