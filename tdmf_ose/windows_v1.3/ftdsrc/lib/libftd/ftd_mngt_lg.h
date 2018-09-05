/*
 * ftd_mngt_lg_monit.h - TDMF management related to logical groups 
 * 
 * Copyright (c) 2002 Fujitsu Softek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef _FTD_MNGT_LG_H_
#define _FTD_MNGT_LG_H_


/* logical group monitoring information */
typedef struct ftd_mngt_lg_monit_s {
    time_t      last_monit_ts;              /* last time monitoring data was sent to Collector */
    time_t      monit_dt;                   /* requested period value between msgs to Collector */
    ftd_int64_t curr_pstore_file_sz;        /* current pstore file size value (bytes) sent */
    ftd_int64_t curr_journal_files_sz;      /* current journal file size value (bytes) sent*/
    ftd_int64_t last_pstore_file_sz;        /* last pstore file size value (bytes) sent, to avoid resending same information   */
    ftd_int64_t last_journal_files_sz;      /* last journal file size value (bytes) sent, to avoid resending same information  */
    ftd_int64_t journal_disk_total_sz;      /* data about the disk where the journal is stored, size in bytes */
    ftd_int64_t journal_disk_free_sz;       /* data about the disk where the journal is stored, size in bytes */
    ftd_int64_t pstore_disk_total_sz;       /* data about the disk where the PStore is stored, size in bytes */
    ftd_int64_t pstore_disk_free_sz;        /* data about the disk where the PStore is stored, size in bytes */

    int         iReplGrpSourceIP;

    int         iDirtyFlag;                 /* Value to know if the monitor info should be sent to the
                                               collector regardless if it changed or not               */

    //optimization : pre-processed values, ready to be tx to Collector
    //char        szPStoreDiskTotalSpace[24]; //Total size (in MB) of disk on which PStore file is maintained, 64 bit integer value. eg: "1234567890987654"
    //char        szJournalDiskTotalSpace[24];//Total size (in MB) of disk on which Journal file is maintained, 64 bit integer value. eg: "1234567890987654"

    void        *msg;   //internal use 

} ftd_mngt_lg_monit_t;

//make sure to have C-style prototypes 
#ifdef __cplusplus
extern "C" {
#endif
    
ftd_mngt_lg_monit_t* ftd_mngt_create_lg_monit();                /* called in ftd_lg_create() */
int  ftd_mngt_delete_lg_monit(ftd_mngt_lg_monit_t* monitp);     /* called in ftd_lg_cleanup() */
int sftk_mngt_init_lg_monit( ftd_mngt_lg_monit_t *monitp, int role, char * pstore, char *phostname, char * jrnpath );
void sftk_mngt_do_lg_monit( ftd_mngt_lg_monit_t *monitp, int role, char * pstore, char *phostname, char * jrnpath, int lgnum, ftd_journal_t *jrnp );

#ifdef __cplusplus
}
#endif


#endif  //_FTD_MNGT_LG_H_
