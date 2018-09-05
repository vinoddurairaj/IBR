/*
 * ftd_mngt_lg.c - TDMF management related to logical groups 
 * 
 * Copyright (c) 2001 Fujitsu Softek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#include <sys\types.h>
#include <sys\stat.h>
#include <crtdbg.h>
extern "C"
{
#include "ftd_port.h"
#include "ftd_lg.h"
#include "ftd_mngt_lg.h"
#include "ftd_mngt.h"
#include "iputil.h"
}
#include "libmngtmsg.h"

// By Saumya 03/05/04
extern int giTDMFCollectorIP;
extern bool gbTDMFCollectorPresent;
extern bool gbTDMFCollectorPresentWarning;
extern int giTDMFCollectorPort;

/**
 * Called in ftd_lg_create(), at lg group creation
 */
ftd_mngt_lg_monit_t* ftd_mngt_create_lg_monit()              
{
    ftd_mngt_lg_monit_t *monitp;
    monitp = (ftd_mngt_lg_monit_t *) calloc( sizeof(ftd_mngt_lg_monit_t), 1 );

    mmp_mngt_TdmfReplGroupMonitorMsg_t *pmsg = new mmp_mngt_TdmfReplGroupMonitorMsg_t;
    pmsg->hdr.magicnumber = 0;//mark it as unintialized

    monitp->msg = pmsg;

    return monitp;
}

/**
 * Called in ftd_lg_cleanup() 
 */
int ftd_mngt_delete_lg_monit(ftd_mngt_lg_monit_t* monitp)   
{
    if ( monitp == NULL )
        return -1;

    if ( monitp->msg )
    {
        mmp_mngt_TdmfReplGroupMonitorMsg_t *pmsg = (mmp_mngt_TdmfReplGroupMonitorMsg_t *)monitp->msg;
        delete pmsg;
    }

    free( monitp );
    return 0;
}

/**
 * 
 */
// modified b y Mike Pollett to support new service.
int sftk_mngt_init_lg_monit( ftd_mngt_lg_monit_t *monitp, int role, char * pstore, char *phostname, char * jrnpath ) {
    DWORD   SectorsPerCluster,     // sectors per cluster
            BytesPerSector,        // bytes per sector
            NumberOfFreeClusters,  // free clusters
            TotalNumberOfClusters; // total clusters

    if ( monitp == NULL )
        return -1;

    void *tmp = monitp->msg;
    //clear all values to zero
    memset(monitp,0,sizeof(ftd_mngt_lg_monit_t));
    monitp->msg = tmp;
    /* requested period value between msgs to Collector */
    monitp->monit_dt = ftd_mngt_get_repl_grp_monit_upload_period();
    
    if ( role == ROLEPRIMARY )
    {   //PStore
        char szDrive[MAX_PATH];
        _splitpath(pstore,szDrive,0,0,0);
        szDrive[2] = '\\';
        szDrive[3] = '\0';
        if ( FALSE == GetDiskFreeSpace( szDrive,      // root path
                                        &SectorsPerCluster,     // sectors per cluster
                                        &BytesPerSector,        // bytes per sector
                                        &NumberOfFreeClusters,  // free clusters
                                        &TotalNumberOfClusters  // total clusters
                                        ) )
        {
            return -3;
        }
        //prepare some static values to be tx to Collector
        monitp->pstore_disk_total_sz = (ftd_int64_t)BytesPerSector * SectorsPerCluster * TotalNumberOfClusters ;
    }
    else
    {   //Journal 
        char szDrive[MAX_PATH];
        _splitpath(jrnpath,szDrive,0,0,0);
        szDrive[2] = '\\';
        szDrive[3] = '\0';
        if ( FALSE == GetDiskFreeSpace( szDrive,     // root path
                                        &SectorsPerCluster,     // sectors per cluster
                                        &BytesPerSector,        // bytes per sector
                                        &NumberOfFreeClusters,  // free clusters
                                        &TotalNumberOfClusters  // total clusters
                                        ) )
        {
            return -2;
        }
        //prepare some static values to be tx to Collector
        monitp->journal_disk_total_sz = (ftd_int64_t)BytesPerSector * SectorsPerCluster * TotalNumberOfClusters;
        // Collector must known IP addr. of PRIMARY system. Get it from cfg file.
        // phostname can be an IP addr. or an host name.
        if (name_is_ipstring(phostname)) {
            ipstring_to_ip(phostname, (unsigned long*)&monitp->iReplGrpSourceIP);
        } else {
            name_to_ip(phostname, (unsigned long*)&monitp->iReplGrpSourceIP);
        }
        _ASSERT(monitp->iReplGrpSourceIP != 0);
    }

    return 0;
}
// modified by Mike Pollett to support new service.
extern "C" int ftd_mngt_init_lg_monit(ftd_lg_t* lgp)
{
	if ( lgp == NULL ) {
        return -1;
	}
	return sftk_mngt_init_lg_monit( lgp->monitp, lgp->cfgp->role, lgp->cfgp->pstore, lgp->cfgp->phostname, lgp->cfgp->jrnpath );
}


static int Mon_Timeout_Multiplier = 10;

void ftd_mngt_set_monit_timeout_multiplier(int newTimeoutValue)
{
    Mon_Timeout_Multiplier = newTimeoutValue;
}
/**
 *  Called periodically to perform for various monitoring tasks
 *  related to logical group diagnostic. 
 */
void sftk_mngt_do_lg_monit( ftd_mngt_lg_monit_t *monitp, int role, char * pstore, char *phostname, char * jrnpath, int lgnum, ftd_journal_t *jrnp ) {
    time_t  now = time(0);

    if ( monitp == NULL )
        return;

    //
    //  We implemented a dirty flag in the ftd_mngt_lg_monit_t
    //  structure. Right now this flag is used to check a 
    //  certain amount of time that has passed, but this could
    //  be used in the future to set this flag by an external
    //  function that could be notified when we are reconnecting
    //  to the collector.
    //
    //  The dirty flag right now waits for 10* the timeout
    //  to pass. i.e. if the timeout is 10secs, 
    //  we send the info every 100secs
    //
    //  Modified this to also be a variable that is currently
    //  not initialized. In future we may want to initialize
    //  this variable trough the GUI (Mon_Timeout_Multiplier)
    //
    if ( now >= (monitp->last_monit_ts + monitp->monit_dt) )
    {
        bool sendmsg = false;
        
        //check Journal file size ...
        if ( role == ROLESECONDARY )
        {
            ftd_int64_t jrnlTotalsz = ftd_journal_get_journal_files_total_size(jrnp);

            monitp->iDirtyFlag++;
            //
            // Check if values different, or need to update (dirtyflag)
            //
            if (        ( jrnlTotalsz != monitp->last_journal_files_sz )
                    ||  ( monitp->iDirtyFlag > Mon_Timeout_Multiplier ) )
            {   //new values, need to update 
                sendmsg                         = true;
                monitp->iDirtyFlag              = 0;
                monitp->curr_journal_files_sz   = jrnlTotalsz;

                DWORD   SectorsPerCluster,     // sectors per cluster
                        BytesPerSector,        // bytes per sector
                        NumberOfFreeClusters,  // free clusters
                        TotalNumberOfClusters; // total clusters
                char    szDrive[MAX_PATH];
                _splitpath(jrnpath,szDrive,0,0,0);
                szDrive[2] = '\\';
                szDrive[3] = '\0';
                if ( GetDiskFreeSpace( szDrive,     // root path
                                        &SectorsPerCluster,     // sectors per cluster
                                        &BytesPerSector,        // bytes per sector
                                        &NumberOfFreeClusters,  // free clusters
                                        &TotalNumberOfClusters  // total clusters
                                        ) )
                {
                    monitp->journal_disk_free_sz  = (ftd_int64_t)BytesPerSector * SectorsPerCluster * NumberOfFreeClusters ;  
                }
                else
                {
                    monitp->journal_disk_free_sz  = monitp->journal_disk_total_sz;
                    //DWORD err = GetLastError();_ASSERT(0);
                }
            }
            //Collector must known IP addr. of PRIMARY system.  Get it from isockp.
            //monitp->iReplGrpSourceIP = lgp->isockp->sockp->rip; _ASSERT(lgp->isockp->sockp->rip != 0);
        }
        //...or check PStore file size
        else if ( role == ROLEPRIMARY )
        {
            ftd_int64_t pstoreTotalsz;
            struct _stati64 statbuf;
            //get file size
            if ( 0 == _stati64(pstore , &statbuf) )
            {
                pstoreTotalsz = statbuf.st_size;
            }
            else
            {  //file could not be found
                pstoreTotalsz = 0;
            }

            monitp->iDirtyFlag++;
            //
            // Check if values different, or need to update (dirtyflag)
            //
            if (        ( pstoreTotalsz != monitp->last_pstore_file_sz )
                    ||  ( monitp->iDirtyFlag > Mon_Timeout_Multiplier ) )
            {   //new values, need to update 
                sendmsg                         = true;
                monitp->iDirtyFlag              = 0;
                monitp->curr_pstore_file_sz     = pstoreTotalsz;

                DWORD   SectorsPerCluster,     // sectors per cluster
                        BytesPerSector,        // bytes per sector
                        NumberOfFreeClusters,  // free clusters
                        TotalNumberOfClusters; // total clusters
                char    szDrive[MAX_PATH];
                _splitpath(pstore,szDrive,0,0,0);
                szDrive[2] = '\\';
                szDrive[3] = '\0';
                if ( GetDiskFreeSpace( szDrive,     // root path
                                        &SectorsPerCluster,     // sectors per cluster
                                        &BytesPerSector,        // bytes per sector
                                        &NumberOfFreeClusters,  // free clusters
                                        &TotalNumberOfClusters  // total clusters
                                        ) )
                {
                    monitp->pstore_disk_free_sz  = (ftd_int64_t)BytesPerSector * SectorsPerCluster * NumberOfFreeClusters ;  
                }
                else
                {
                    monitp->pstore_disk_free_sz  = monitp->pstore_disk_total_sz;
                    //DWORD err = GetLastError();_ASSERT(0);
                }
            }
        }

        bool waitNextTimeout = true;
        //send msg to Collector
		// By Saumya 03/03/04
		// With these modifications it will run in a collector less environment also
		// Big GUI won't work without the collector; But the Mini GUI and the Config tool will
		// still work; all the CLIs will work too

        // Modified By Saumya Tripathi 03/18/04
		// Fixing WR 32854

		if ( (gbTDMFCollectorPresent == false) && (gbTDMFCollectorPresentWarning == false) && ( giTDMFCollectorIP != 0 ) )
		{
			char ipstr[24];
            ip_to_ipstring(giTDMFCollectorIP,ipstr);
			error_syslog(ERRFAC,LOG_WARNING,"****Warning, Collector not found at IP=%s , Port=%d !\n",ipstr,giTDMFCollectorPort);
			gbTDMFCollectorPresentWarning = true;
		}

        if ( sendmsg && ( giTDMFCollectorIP != 0 ) && (gbTDMFCollectorPresent != false))
        {
            bool tx_success = false;
            if ( 0 == ftd_mngt_send_lg_monit(lgnum, role == ROLEPRIMARY, monitp) )
                tx_success = true;

            //in case Collector could NOT been notified, 
            //make sure to retry on next pass 
            if ( !tx_success )
            {
                waitNextTimeout = false;
            }
        }

        //if Collector has been notified successfully or nothing to be signaled
        if ( waitNextTimeout )
        {
            monitp->last_journal_files_sz   = monitp->curr_journal_files_sz;
            monitp->last_pstore_file_sz     = monitp->curr_pstore_file_sz;
            monitp->last_monit_ts           = time(0);
        }

    }//if its time to check monitoring values

    /* in case a new value has been received */
    monitp->monit_dt = ftd_mngt_get_repl_grp_monit_upload_period();
}
extern "C" void ftd_mngt_do_lg_monit( ftd_lg_t *lgp ) 
{
	sftk_mngt_do_lg_monit( lgp->monitp, lgp->cfgp->role, lgp->cfgp->pstore, lgp->cfgp->phostname, lgp->cfgp->jrnpath, lgp->cfgp->lgnum, lgp->jrnp );
}

