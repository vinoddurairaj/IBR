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
/* #ident "@(#)$Id: ftd_agn_get_rmd_stat.c,v 1.3 2010/12/20 20:12:25 dkodjo Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_AGN_GET_RMD_STAT_C_
#define _FTD_AGN_GET_RMD_STAT_C_

int ftd_agn_get_rmd_stat(sock_t *sockID)
{

#if 0
    pid_t	pid;
    int		fd [2];		              /* File descriptor for stdout */
    int		status = -1;
    char 	linebuf[1025];	              /* Line buffer */

    int      r,exitcode = -1;                 /* assume error */
    mmp_mngt_TdmfCommandMsg_t       cmdmsg;
    char tmpdata[10+1];
    char   szShortApplicationName[1024+1];    /* name of executable module */
    char   szApplicationName[1024+1];         /* name of executable module */
    char   szCurrentDirectory[1024+1];        /*  current directory name   */
    char   *pszCurDir = szCurrentDirectory;
    char   *pCmdLine;
    int    dwCmdCompleteWaitTime;             /* in milliseconds */
    int    iCmdLineLen;
    int    iOutputMsgLen;
    int    towrite = 0;
    FILE   *cmdout;
    struct stat  shstat;
    char *outdata;
    int  filelen;

    char                            *pWk;
    pWk  = (char*)&cmdmsg;
    pWk += sizeof(mmp_mngt_header_t);
#endif

    int		lgnum;
    int      	r;
    mmp_agn_CheckRmd_t response;

    /* clear data area */
    memset(&response,0x00,sizeof(response));
    lgnum = 0;
    logout(17, F_ftd_agn_get_rmd_stat, "start.\n");

    /*
     * at this point, mmp_agn_CheckRmd_t header is read.
     * now read the remainder of the mmp_agn_CheckRmd_t structure
     */
    r = ftd_sock_recv(sockID,(char*)&lgnum,sizeof(lgnum)); 
    if ( r != sizeof(lgnum))
    {
        logout(4, F_ftd_agn_get_rmd_stat, "data recv error.\n");
        return -1;
    }

    /* 
     * send exit code back to caller
     */
    response.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    response.hdr.mngttype       = MMP_AGN_TDMF_CMD_RMDCHK;
    response.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    response.hdr.mngtstatus     = MMP_MNGT_STATUS_OK;
    response.lgnum 		= isrmdx(lgnum);
    
    /* stdout data exist */
    r = ftd_sock_send(sockID,(char*)&response,sizeof(response));
    /* close socket */
    close(sockID->sockID);
    sockID->sockID = 0;
    return 0;
}
#endif /* _FTD_AGN_GET_RMD_STAT_C_ */
