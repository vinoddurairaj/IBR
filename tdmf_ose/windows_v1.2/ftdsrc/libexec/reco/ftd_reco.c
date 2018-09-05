/*
 * ftd_reco.c - failover - primary to secondary 
 *
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#include "ftd_port.h"
#include "ftd_sock.h"
#include "ftd_config.h"

#define ERROR_CODE  1
#define OK_CODE     0

int aflag = 0;			/* Do all the recovery files that we find */
int gflag = 0;			/* Do one logical group */
int dflag = 0;			/* Disable failover */
int vflag = 0;			/* Verbose output */
int force = 0;			/* Force mirror offline (ie. kill rmd if running) */
long lgnum;

static void usage(void);
static int  applygroups(char **argv, int force);

int
main (int argc, char** argv)
{
#if !defined(_WINDOWS)
	pid_t	pid;
	int		pcnt,i;
#endif
	int		ch;
    int     exitcode;

////------------------------------------------
#if defined(_WINDOWS)
	void ftd_util_force_to_use_only_one_processor();
	ftd_util_force_to_use_only_one_processor();
#endif
//------------------------------------------


#if !defined(_WINDOWS)
    /* -- Make sure we're root. */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this program\n");
        exit(ERROR_CODE);
    }
#endif

    if (ftd_init_errfac(CAPQ, argv[0], NULL, NULL, 1, 1) == NULL) {
        exit(ERROR_CODE);
    }

    /* -- Parse the args here */
    while ((ch = getopt(argc, argv, "adfg:v")) != -1) {
        switch (ch) {
        case 'a':
            aflag = 1;
            break;
        case 'd':
            dflag = 1;
            break;
        case 'g':
            gflag = 1;
            lgnum = strtol(optarg, NULL, 10);
            break;
        case 'v':
            vflag = 1;
            break;
        case 'f':
            force = 1;
            break;
        default:
            usage();
        }
    }
    if (aflag == 0 && gflag == 0) {
        fprintf(stderr, "You must specify -a or -g.\n");
        usage();
    }
    if (aflag != 0 && gflag != 0) {
        usage();
    }
    if (argc > optind) {
        fprintf(stderr, "Invalid arguments\n");
        usage();
    }
    
	/* -- OK, now we go and recover */
    exitcode = applygroups(argv, force);

#if defined(_WINDOWS) && defined(_DEBUG)
	printf("\nPress any key to continue....\n");
	while(!_kbhit())
		Sleep(1);
#endif

    exit(exitcode);	
}

static int
applygroups(char **argv, int force)
{
	LList			*cfglist = NULL;
	ftd_lg_cfg_t	**cfgpp = NULL;
	ftd_sock_t		*fsockp = NULL;
	int				portnum, fd, rc;
	char			*dev, fn[MAXPATHLEN], procname[MAXPATHLEN], errstr[64];
	long			thislg;

	if ((cfglist = ftd_config_create_list()) == NULL) {
		return ERROR_CODE;
	}

	if (ftd_config_get_secondary(PATH_CONFIG, cfglist) < 0) {
		return ERROR_CODE;
	}

#if defined(_WINDOWS)
    if (ftd_sock_startup() == -1) {
		goto errret;
    }
#endif

	if (!(portnum = ftd_sock_get_port(FTD_MASTER))) {
		error_tracef ( TRACEERR, "tdmfreco.exe:Calling ftd_sock_get_port()");
		portnum = FTD_SERVER_PORT;
	}

	ForEachLLElement(cfglist, cfgpp) { 
		dev = (*cfgpp)->cfgpath;
		
		dev = dev + strlen(dev) - 7;
        
		thislg = strtol(dev, NULL, 10);
#if defined(_WINDOWS)
		sprintf(fn, "%s\\s%3.3s.off", PATH_CONFIG, dev);
#else		
		sprintf(fn, "%s/s%3.3s.off", PATH_CONFIG, dev);
#endif
		if (aflag || (thislg == lgnum && gflag)) {
			if (vflag)
				printf("Recovering logical group %d\n", (*cfgpp)->lgnum);
			if (dflag) {
				if (unlink(fn) < 0) {
					fprintf(stderr,"\nCouldn't unlink file %s: %s\n",
						fn, ftd_strerror());
					//goto errret;
				}
			} else {
				// create a .off file for this group 
#if defined(_WINDOWS)
				if ((fd = open(fn, O_CREAT, _S_IWRITE)) == -1) {
#else				
				if ((fd = open(fn, O_CREAT)) == -1) {
#endif
					reporterr(ERRFAC, M_RECOOFF, ERRWARN,
						thislg, strerror(errno));
				}
				close(fd);

				if ((fsockp = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL) {
					goto errret;
				}
				
				if (ftd_sock_init(fsockp, "localhost", "localhost", LOCALHOSTIP, LOCALHOSTIP,
					SOCK_STREAM, AF_INET, 1, 0) < 0)
				{
					goto errret;
				}
				
				FTD_SOCK_PORT(fsockp) = portnum;

				// connect the ipc socket object
				rc = ftd_sock_connect_nonb(fsockp, portnum, 3, 0, 1);
						
				if (rc == 1) {
					// got it
				} else if (rc == 0) {

					reporterr (ERRFAC, M_SOCKCONNECT, ERRWARN, 
						FTD_SOCK_LHOST(fsockp), 
						FTD_SOCK_RHOST(fsockp), 
						FTD_SOCK_PORT(fsockp), 
						sock_strerror(ETIMEDOUT));
							
					goto errret;
				} else {
					goto errret;
				}

				if ((rc = ftd_sock_send_start_reco(thislg, fsockp, force)) < 0) {
					reporterr (ERRFAC, M_RECOFAIL, ERRWARN, argv[0], "");
					goto errret;
				} else if (rc == 1) {
					if (!force) {
						sprintf(procname, "RMD_%03d", thislg);
						sprintf(errstr, " Use '%s' -f to kill '%s' and force the mirror offline",
							argv[0], procname);
						reporterr (ERRFAC, M_RECOFAIL, ERRWARN,	argv[0], errstr);
						goto errret;
					}
				}
				
				ftd_sock_delete(&fsockp);
			}
		}
	}

	ftd_config_delete_list(cfglist);

#if defined(_WINDOWS)
    ftd_sock_cleanup();
#endif

	return OK_CODE;

errret:

	ftd_sock_delete(&fsockp);
	ftd_config_delete_list(cfglist);
	unlink(fn);

#if defined(_WINDOWS)
    ftd_sock_cleanup();
#endif
	
	return ERROR_CODE;
}

static void
usage()
{
    fprintf(stderr, "usage: " QNM "reco <options>\n");
    fprintf(stderr,"\
Exactly one of the following two options is mandatory:\n\
\t-g<logical_group_num>  : Fail over logical group.\n\
\t-a                     : Fail over all logical groups\n\
The following options modify the behavior:\n\
\t-f                     : Force mirror offline (ie. kill rmd if running)\n\
\t-d                     : Disable the fail over (go back to primary)\n\
\t-v                     : Verbose mode\n"
    );
    exit(ERROR_CODE);
}
