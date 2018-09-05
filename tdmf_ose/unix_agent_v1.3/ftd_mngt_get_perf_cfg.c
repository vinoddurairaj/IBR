/* #ident "@(#)$Id: ftd_mngt_get_perf_cfg.c,v 1.7 2003/11/13 02:48:21 FJjapan Exp $" */
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
        if ( cfg_set_software_key_value("PerfUploadPeriod", tmp) != 0 )
        {
	    logout(6,F_ftd_mngt_get_perf_cfg,"Unable to write \'PerfUploadPeriod\' configuration value.\n");
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
        if(cfg_set_software_key_value("ReplGroupMonitUploadPeriod", tmp) != 0 )
        {
	    logout(6,F_ftd_mngt_get_perf_cfg,"Unable to write \'ReplGroupMonitUploadPeriod\' configuration value.\n");
        }
        /* modify dynamic instance of this value */
        gTdmfPerfConfig.iReplGrpMonitPeriod = msg.data.iReplGrpMonitPeriod;
    }

    return 0;
}
#endif /* _FTD_MNGT_GET_PERF_CFG_C_ */
