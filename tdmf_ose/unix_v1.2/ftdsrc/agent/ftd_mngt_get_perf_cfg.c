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
/* #ident "@(#)$Id: ftd_mngt_get_perf_cfg.c,v 1.2 2010/12/20 20:12:25 dkodjo Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_GET_PERF_CFG_C_
#define _FTD_MNGT_GET_PERF_CFG_C_

int  ftd_mngt_get_perf_cfg(sock_t *sockp)
{
    mmp_mngt_TdmfPerfCfgMsg_t   msg;
    int                         r,toread;
    char                        *pWk;
    char tmp[32];

    pWk  = (char*)&msg;
    pWk += sizeof(mmp_mngt_header_t);
    memset(&msg,0x00,sizeof(msg));

    /*
     * at this point, mmp_mngt_header_t header is read.
     * now read the remainder of the mmp_mngt_TdmfPerfCfgMsg_t structure
     */
    toread = sizeof(mmp_mngt_TdmfPerfCfgMsg_t)-sizeof(mmp_mngt_header_t);
    r = ftd_sock_recv(sockp,(char*)pWk, toread);
    if ( r != toread )
    {
	logout(4,F_ftd_mngt_get_perf_cfg,"receive data error.\n");
        return -1;
    }
    mmp_convert_TdmfPerfConfig_ntoh( &msg.data );

    /* 
     * save config to registry
     * do same thing for all values in msg.data
     */
    if (msg.data.iPerfUploadPeriod > 0)
    {
	/*
        itoa(msg.data.iPerfUploadPeriod,tmp,10);
	 */
	sprintf(tmp,"%d",msg.data.iPerfUploadPeriod);
        if ( cfg_set_software_key_value(PERFUPLOADPERIOD, tmp) != 0 )
        {
	    logout(6,F_ftd_mngt_get_perf_cfg,"Unable to write \'"PERFUPLOADPERIOD"\' configuration value.\n");
        }
        /* modify dynamic instance of this value */
        gTdmfPerfConfig.iPerfUploadPeriod = msg.data.iPerfUploadPeriod;
    }
    if (msg.data.iReplGrpMonitPeriod > 0)
    {
	/*
        itoa(msg.data.iReplGrpMonitPeriod,tmp,10);
	 */
	sprintf(tmp,"%d",msg.data.iReplGrpMonitPeriod);
        if(cfg_set_software_key_value(REPLGROUPMONITUPLOADPERIOD, tmp) != 0 )
        {
	    logout(6,F_ftd_mngt_get_perf_cfg,"Unable to write \'"REPLGROUPMONITUPLOADPERIOD"\' configuration value.\n");
        }
        /* modify dynamic instance of this value */
        gTdmfPerfConfig.iReplGrpMonitPeriod = msg.data.iReplGrpMonitPeriod;
    }

    return 0;
}
#endif /* _FTD_MNGT_GET_PERF_CFG_C_ */
