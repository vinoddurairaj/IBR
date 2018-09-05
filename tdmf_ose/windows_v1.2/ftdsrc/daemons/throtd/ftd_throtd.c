/*
 * ftd_throtd.c - ftd throttle evaluation/stat dumper daemon
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

#include "ftd_port.h"
#include "ftd_error.h"
#include "ftd_config.h"
#include "ftd_lg.h"
#include "ftd_dev.h"
#include "ftd_stat.h"
#include "ftd_ioctl.h"
#include "ftd_throt.h"
#include "ftd_ps.h"
#include "ftdio.h"

static int
isgroupdead(ftd_lg_cfg_t *cfgp, HANDLE ctlfd)
{
	ftd_stat_t	statbuf;
    
	/* if not open, try to open the masterfd ftd device */
	if (ctlfd == INVALID_HANDLE_VALUE) {
		if ((ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0)) < 0) {
			ctlfd = INVALID_HANDLE_VALUE;
			return 1;
		}
	}

	if (ftd_ioctl_get_group_stats(ctlfd, cfgp->lgnum, &statbuf, 1) < 0) {
		return 1;
	}

    return 0;
}

/*
 * main -- throtd process 
 */
void
main(int argc, char **argv) 
{
	HANDLE				ctlfd = INVALID_HANDLE_VALUE;
	LList				*cfglist, *lglist;
	ftd_lg_cfg_t		**cfgpp, *cfgp;
	ftd_lg_t			**lgpp, *lgp;
	char				ps_name[MAXPATHLEN] = "";
	time_t				ts, cfgchkts;
	int					elapsedtime, stopflag, err;
    int					getpstoreflag, rc, found;
	struct timeval		wakeupcall;

    wakeupcall.tv_sec = 1;
    wakeupcall.tv_usec = 0; 
    cfgchkts = (time_t) 0;
    getpstoreflag = 1;

#ifdef DEBUG_THROTTLE
    throtfd = fopen(PATH_RUN_FILES "/throtdump.dbg", "w+");
#endif /* DEBUG_THROTTLE */

	if (ftd_init_errfac(CAPQ, argv[0], NULL, NULL, 0, 1) == NULL) {
		Return(1);
	}

	// TODO: use llist interface instead
	if ((cfglist = ftd_config_create_list()) == NULL) {
		Return(1);
	}

	if ((lglist = CreateLList(sizeof(ftd_lg_t**))) == NULL) {
		Return(1);
	}

	// run forever and evaluate throttles, dump stats
	while (1) {  
		// if not open, try to open the masterfd ftd device
		if (ctlfd == INVALID_HANDLE_VALUE) {
			if ((ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0)) == INVALID_HANDLE_VALUE) {
				fflush(stdout);
				select(NULL, NULL, NULL, NULL, &wakeupcall); 
				continue;
			}
		}

		// see if any groups have been stopped
		stopflag = 0;
		ForEachLLElement(cfglist, cfgpp) {
			cfgp = *cfgpp;
			if (isgroupdead(cfgp, ctlfd)) {
				stopflag = 1;
				break;
			}
		}

		(void)time(&ts); /* get a clean new timestamp with each group */

		if (stopflag || ((ts-cfgchkts) >= 10)) {
			cfgchkts = ts;

			if (SizeOfLL(cfglist)) {
				EmptyLList(cfglist);
			}

			if (ftd_config_get_primary_started(PATH_CONFIG, cfglist) < 0) {
				usleep(500000);
				continue;
			}

			// get rid of groups that have gone away since last time */
			found = 0;
			ForEachLLElement(lglist, lgpp) {
				lgp = *lgpp;
				ForEachLLElement(cfglist, cfgpp) {
					cfgp = *cfgpp;
					if (lgp->lgnum == cfgp->lgnum) {
						found = 1;
						break;
					}
				}
				if (!found) {
					// delete the lg object from list
					RemCurrOfLL(lglist, lgpp);

					ftd_lg_delete(lgp);
				}
			}
            // insert new groups that appeared since last time
			found = 0;
			ForEachLLElement(cfglist, cfgpp) {
				cfgp = *cfgpp;
				ForEachLLElement(lglist, lgpp) {
					lgp = *lgpp;
					if (cfgp->lgnum == lgp->lgnum) {
						found = 1;
						break;
					}
				}
				if (!found) {
					// create & insert the lg object from list
					if ((lgp = ftd_lg_create()) == NULL) {
						continue;
					}

					if (ftd_lg_init(lgp, cfgp->lgnum, ROLEPRIMARY, 1) < 0) {
						ftd_lg_delete(lgp);
						continue;
					}

					if (ftd_lg_get_driver_state(lgp, 1) < 0) {
						ftd_lg_delete(lgp);
						continue;
					}

					if (ftd_stat_init_driver(lgp) < 0) {
						ftd_lg_delete(lgp);
						continue;
					}

					if (ftd_stat_init_file(lgp) < 0) {
						ftd_lg_delete(lgp);
						continue;
					}

					AddToTailLL(lglist, &lgp);
				}
			}
        }

		// now process each group
		ForEachLLElement(cfglist, cfgpp) {
			cfgp = *cfgpp;

			// get corresponding lg object 
			found = 0;

			ForEachLLElement(lglist, lgpp) {
				lgp = *lgpp;

				if (lgp->lgnum == cfgp->lgnum) {
					found = 1;
					break;
				}
			}
			if (!found) {
				continue;
			}
			(void)time(&ts); /* get a clean new timestamp with each group */

			elapsedtime = ts - lgp->statp->statts;

			if (lgp->tunables->statinterval > 0
			&& lgp->tunables->statinterval <= elapsedtime)
			{
				lgp->statp->statts = ts;

				// we need to get the tunables each iteration to catch any 
				// changes.  We specify 0 for the pstore argument because we
				// don't need to hit the pstore since the driver should have
				// a copy in memory */

				if (isgroupdead(cfgp, ctlfd)) {
                    continue;
                }                    

				if (ftd_lg_get_driver_state(lgp, 1) < 0) {
					continue;
				}

				if (!(err = ftd_lg_get_dev_stats(lgp, elapsedtime))) {
					fflush(stdout);
					//continue;
				}

				if (lgp->tunables->stattofileflag) {
					(void)ftd_stat_dump_file(lgp);
				}

				(void)eval_throttles(lgp);
			}
		}	
		fflush(stdout);

		/* check again in a second */
		select(NULL, NULL, NULL, NULL, &wakeupcall); 
    }     
	
} 
