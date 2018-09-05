/*
 * ftd_launch.c - ftd primary system launch command
 *
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

// Include this library to force the linker to link the MFC libraries in the correct order Mike Pollett
#include "../../forcelib.h"
// Mike Pollett
#include "../../tdmf.inc"

#if defined(_DEBUG)
#include <conio.h>
#endif
#include "ftd_port.h"
#include "ftd_config.h"
#include "ftd_sock.h"
#include "ftd_proc.h"
#include "ftd_ps.h"
#include "ftd_ioctl.h"
#include "ftd_error.h"
#include "ftd_lg_states.h"
#include "ftd_mngt.h"

//libftd, ftd_mngt_msgs.cpp
void ftd_mngt_msgs_log(const char **argv, int argc);
#include "ftd_devlock.h"

static int		all = FALSE;
static int		group = FALSE;
static int		pending = FALSE;
static int		state, msgtype;
static char		*statstr, *argv0;
static int		targets[FTD_MAX_GRP_NUM];
static LList	*cfglist;

/*
 * usage -- This is how the program is used
 */
void
usage(void)
{
	if (pending)
		goto quit;

	fprintf(stderr,"\nUsage: %s <options>\n\n", argv0);
	
	fprintf(stderr,"\
	One of the following two options is mandatory:\n\
	\t-g <group_num>\t: %s the group\n\
	\t-a            \t: %s all groups\n\n", statstr, statstr);
#if defined(FTD_LAUNCH_REFRESH)
	fprintf(stderr, "\
	-f\t\t\t: Perform a Full Refresh\n\
	-c\t\t\t: Perform a Checksum Refresh\n");
#elif defined(FTD_LAUNCH_BACKFRESH)
	fprintf(stderr, "\
	-f\t\t\t: Force\n");
#endif
	fprintf(stderr,"\t-h\t\t\t: Print this listing\n");

quit:

	exit(1);
}

/*
 * proc_argv -- process argument vector 
 */
void
proc_argv(int argc, char **argv)
{
	ftd_lg_cfg_t	**cfgpp;
	int				devcnt, ch;
	unsigned long	lgnum = 0;

	argv0 = strdup(argv[0]);

	// At least one argument is required 
	if (argc == 1) {
		usage();
	}
	devcnt = 0;

#if defined(FTD_LAUNCH_REFRESH)

	while ((ch = getopt(argc, argv, "facpg:")) != -1) {
		switch (ch) {
		case 'f':
			// perform Full Sync - No dirtybit/chksum
			state = FTD_SREFRESHF;
			break;
		case 'c':
			// perform verify secondary - All dirtybit/chksum
			state = FTD_SREFRESHC;
			break;
#elif defined(FTD_LAUNCH_BACKFRESH)

	while ((ch = getopt(argc, argv, "fapg:")) != -1) {
		switch (ch) {
		case 'f':
			// force - kill journals and apply
			state = FTD_SBACKFRESH | FTD_LG_BACK_FORCE;
			break;
#else

	while ((ch = getopt(argc, argv, "apg:")) != -1) {
		switch (ch) {
#endif
		case 'a':
			// change the operation mode of all devices in all logical groups
			if (group) {
				usage();
			}
			all = TRUE;
			ForEachLLElement(cfglist, cfgpp) {
				targets[(*cfgpp)->lgnum] = state;
			}
			break;
		case 'g':
			// change the operation mode of all devices in a logical group
			if (all) {
				usage();
			}
			group = TRUE;
			lgnum = strtol(optarg, NULL, 0);
			targets[lgnum] = state;
			break;
		case 'p':
			pending = TRUE;
			break;
		case 'h':
			usage();
			break;
		default:
			usage();
			break;
		}
	}
  
	return;
}


static int
verify_checkpoint(int lgnum)
{
	ftd_lg_t	*lgp;
	int			rc;
	int			checkpoint = -1;
	ps_group_info_t	group_info;

	if (ftd_dev_lock_create() == -1) 
		goto errret;

	// Verify checkpoint status for down group
	if ((lgp = ftd_lg_create()) == NULL) 
	{
		error_tracef ( TRACEERR, "verify_checkpoint():Calling ftd_lg_create()");
		goto errret;
	}

	if (ftd_lg_init(lgp, lgnum, ROLEPRIMARY, 1) < 0) 
	{
		error_tracef ( TRACEERR, "verify_checkpoint():Calling ftd_lg_init()");
		goto errret;
	}
        
    ftd_lg_get_pstore_state(lgp, 0);
          
	group_info.name = NULL;
	if ((rc = ps_get_group_info(lgp->cfgp->pstore, lgp->devname, &group_info)) != PS_OK) 
		goto errret;

	if (group_info.checkpoint)
        {
        if (lgp->tunables->use_journal == TRUE)
		checkpoint = 1;
	else 
            {
            //reporterr(ERRFAC, M_JLESS, ERRINFO);
    		checkpoint = 0;    // in JLESS mode, we could ignite a smart-refresh
            }
        }
    else
		checkpoint = 0;

errret:

	ftd_lg_delete(lgp);
	ftd_dev_lock_delete();

	return checkpoint;
}

static int
get_lg_state(int lgnum)
{
	ftd_lg_t	*lgp;
	int			rc;
	ftd_stat_t		lgstat;

	if (ftd_dev_lock_create() == -1) 
		goto errret;

	// Verify LG state for down group
	if ((lgp = ftd_lg_create()) == NULL) 
	{
		error_tracef ( TRACEERR, "get_lg_state():Calling ftd_lg_create()");
		goto errret;
	}

	if (ftd_lg_init(lgp, lgnum, ROLEPRIMARY, 1) < 0) 
	{
		error_tracef ( TRACEERR, "get_lg_state():Calling ftd_lg_init()");
		goto errret;
	}
 
	rc = ftd_ioctl_get_group_stats(lgp->ctlfd, lgp->lgnum, &lgstat, 0);

	if ( rc < 0 )
		return rc;
	else
		return lgstat.state;

errret:

	ftd_lg_delete(lgp);
	ftd_dev_lock_delete();

	return -1;
}

/*
 * main - start the daemons
 */
int main (int argc, char **argv, char **envp)
{
	ftd_lg_cfg_t	**cfgpp;
	ftd_header_t	header;
	ftd_sock_t		*fsockp = NULL;
	int				conflicts, found, lgs_started = 0;
    int             exitcode = 1;//assume error
	int				portnum, rc, lg_checkpoint;
	int				lg_state,iTryAgain = 0;
////------------------------------------------
	void ftd_util_force_to_use_only_one_processor();
	ftd_util_force_to_use_only_one_processor();
//------------------------------------------

#if defined (FTD_LAUNCH_BACKFRESH)
	statstr	= "Backfresh";
	state = FTD_SBACKFRESH;
#elif defined (FTD_LAUNCH_REFRESH)
	statstr	= "Refresh";
	state = FTD_SREFRESH;
#elif defined (FTD_LAUNCH_NORMAL)
	statstr	= "Start Mirroring";
	state = FTD_SNORMAL;
#endif
	msgtype = FTDCSTARTPMD;

	if (ftd_sock_startup() == -1)
		goto errexit;

    if (ftd_init_errfac("Replicator", argv[0], NULL, NULL, 0, 1) == NULL) {
		goto errexit;
	}

    //send status msg to System Event Log and TDMF Collector
    ftd_mngt_msgs_log(argv, argc);

	memset(targets, 0, sizeof(targets));

	if ((cfglist = ftd_config_create_list()) == NULL) 
		goto errexit;
	
	
	if (ftd_config_get_primary_started(PATH_CONFIG, cfglist) < 0) 
		goto errexit;

  	proc_argv(argc, argv);
    
	if (SizeOfLL(cfglist) == 0) 
	{
		reporterr(ERRFAC, M_NOGROUP, ERRWARN);
		goto errexit;
	}

	if (pending)        // Use Pending mode to introduced a delayed launchrefresh, WR 17511
	{
		Sleep(15000);
		iTryAgain = 1;
	}

	// at least one target must be specified 
	found = 0;
	ForEachLLElement(cfglist, cfgpp) {
		if (targets[(*cfgpp)->lgnum]) {
			found = 1;
			targets[(*cfgpp)->lgnum] = state;
		}
	}
	if (!found) 
	{
		reporterr(ERRFAC, M_NOGROUP, ERRWARN);
		goto errexit;
	}
	// look for mode conflicts 
	conflicts = 0;

	// initialize the socket object
	//gethostname(hostname, sizeof(hostname));

	if (!(portnum = ftd_sock_get_port(FTD_MASTER))) {
		portnum = FTD_SERVER_PORT;
	}

	memset(&header, 0, sizeof(header));
	header.magicvalue = MAGICHDR;
	header.msgtype = msgtype;
	header.cli = (HANDLE)1;

	header.msg.lg.data = state;

    ForEachLLElement(cfglist, cfgpp) 
	{
		if (!targets[(*cfgpp)->lgnum]) 
		{
			continue;
		}
		
		header.msg.lg.lgnum = (*cfgpp)->lgnum;

		lg_checkpoint = verify_checkpoint((*cfgpp)->lgnum); // Verify the checkpoint status **rddev 021202

		if(lg_checkpoint < 0) {
			fprintf(stderr,"Error: LG%d PSTORE: Unable to verify the checkpoint status.", (*cfgpp)->lgnum);
			goto errexit;
		}

		if(lg_checkpoint) 
		{
			if((state == FTD_SREFRESH) || (state == FTD_SREFRESHC)) 
			{
				fprintf(stderr,"Error: LG%d Checkpoint is ON.",(*cfgpp)->lgnum);
				fprintf(stderr,"Launch a full refresh. ");
				goto errexit;
			}
		}
        
        if ( state == FTD_SNORMAL  || /*launch pmd */
             state == FTD_SREFRESH ) /* launch smart refresh */
            {
			lg_state = get_lg_state((*cfgpp)->lgnum);	
			if (lg_state == FTD_MODE_PASSTHRU)
                {
                reporterr(ERRFAC, M_PASSTHRU, ERRWARN);
                goto errexit;
                }
            }
        

		do // while ( iTryAgain )
		{
			int				tries, maxtries;
			ftd_header_t	ack;	

			maxtries = 3;
			tries = 0;

			// WR 17511 , waits 20 secs before launching a SMART REFRESH or FULL REFRESH
			if ( pending && ((state == FTD_SREFRESH) || (state == FTD_SREFRESHF)))
			{
				Sleep(20000);                         
			}

				// create an ipc socket and connect it to the local master
				if ((fsockp = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL) 
					goto errexit;

				if (ftd_sock_init(fsockp, "localhost", "localhost", LOCALHOSTIP, LOCALHOSTIP,
					SOCK_STREAM, AF_INET, 1, 1) < 0)
					goto errexit;

				FTD_SOCK_PORT(fsockp) = portnum;

				// connect the ipc socket object
				rc = ftd_sock_connect_nonb(fsockp, portnum, 1, 0, 1);
				
				if (rc == 1) 
				{
					// got it
				} 
				else if (rc == 0) 
				{					
					if (++tries == maxtries) 
					{
						reporterr (ERRFAC, M_SOCKCONNECT, ERRWARN, 
							FTD_SOCK_LHOST(fsockp), 
							FTD_SOCK_RHOST(fsockp), 
							FTD_SOCK_PORT(fsockp), 
							sock_strerror(ETIMEDOUT));
					}
					
					ftd_sock_delete(&fsockp);
					
					continue;
				} 
				else 
				{
					goto errexit;
				}
				
				// send the message
				rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"main",fsockp, &header);
				if (rc < 0) 
				{
					goto errexit;
				}
				
				// recv the ack 
				rc = FTD_SOCK_RECV_HEADER(__FILE__ ,"main",fsockp, &ack);
				if (rc < 0) 
				{
					goto errexit;
				} 
				else if (rc == 0) 
				{
					// connection to master down
					goto errexit;
				}
				
				error_tracef( TRACEINF, "start daemon main():ack.msgtpe = %s", tdmf_socket_msg[ack.msgtype-1].cmd_txt );
				ftd_sock_delete(&fsockp);

				if ( pending )
				{   // WR 17511 , waits 20 secs before checking the LG state
					Sleep(20000);                         
					lg_state = get_lg_state((*cfgpp)->lgnum);
				
					if ( lg_state != FTD_MODE_TRACKING )
						iTryAgain = 0;
				        }
		} while ( iTryAgain );

		}
    exitcode = 0;//success

errexit:

    SendAllPendingMessagesToCollector();

	ftd_sock_delete(&fsockp);

	ftd_sock_cleanup();

    exit(exitcode);
}
