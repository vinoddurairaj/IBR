/*
 * ftd_trace.c - Set trace level of TDMF Server
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

//#include "ftd_port.h"

#include "ftd_sock.h"
#include "ftd_error.h"
#include "ftd_ioctl.h"
//#include "ftd_stat.h"

#if defined(_WINDOWS) && defined(_DEBUG)
#include <conio.h>
#endif

static void
usage(char **argv)
{
	//fprintf(stderr, "Usage: %s [-lx | -off | -on] [-b]\n", argv[0]);
	//fprintf(stderr, "Set trace level of Tdmf Server or Tdmf Block device driver.\n");
	fprintf(stderr, "Usage: %s [-lx | -off | -on] \n", argv[0]);
	fprintf(stderr, "Set trace level of Tdmf Server.\n");
	fprintf(stderr, "OPTIONS: -lx => set trace level to decimal value x.\n");
	fprintf(stderr, "                levels : 0 = disable traces;\n");
	fprintf(stderr, "                         1 = errors only;\n");
	fprintf(stderr, "                         2 = warnings and higher priorities levels of traces;\n");
	fprintf(stderr, "                         3 = L0 information and higher priorities levels of traces;\n");
	fprintf(stderr, "                         4 = L1 information and higher priorities levels of traces;\n");
	fprintf(stderr, "                         5 = L2 information and higher priorities levels of traces;\n");
	fprintf(stderr, "         -on  => equivalent to -l3.\n");
	fprintf(stderr, "         -off => equivalent to -l0.\n");
	//fprintf(stderr, "         -b   => send trace level to Tdmf Block device driver.\n");
	//fprintf(stderr, "                 Without -b, trace level request is sent only to Tdmf Server.\n");
}

/*
static int
setDriverTraceLevel(int level)
{
	HANDLE  ctlfd = INVALID_HANDLE_VALUE;
    int     rc;
	// open the master control device 
	ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0);
	if (ctlfd == INVALID_HANDLE_VALUE) {
		reporterr(ERRFAC, M_CTLOPEN, ERRCRIT, ftd_strerror());
		return -1;
	}

	rc = ftd_ioctl_set_trace_level(ctlfd, level);

	ftd_close(ctlfd);

    return rc;
}
*/

/*
 * main
 */
int 
main(int argc, char *argv[])
{
	ftd_header_t	header, ack;
	char            ch;
    unsigned char   level = 0;
	int				exitcode = 0;
    int             rc;
    int             portnum;
    long            llevel;
	ftd_sock_t		*fsockp = NULL;
	char			*remainstr = NULL;
    int             bDriverTraceLvl = 0;

	if (argc < 2) {
		goto usage_error;
	}

	// process all of the command line options 
	while ((ch = getopt(argc, argv, "bo:l:")) != EOF) {
		switch(ch) {
		case 'l':
            llevel = strtol(optarg, &remainstr, 10);
			if (*remainstr != NULL) { //rddev 0301015
				goto usage_error;
			}

            if ( llevel > TRACEINF5 || llevel < 0 ) //rddev 0301015
                goto usage_error;
            level = (unsigned char)llevel;
            break;
        case 'o':
            if ( *optarg == 'n' )// -on
                level = 3;
            else if ( *optarg == 'f' && *(optarg+1) == 'f' )// -off
                level = 0;
            else
                goto usage_error;
            break;
        /*case 'b':
            bDriverTraceLvl = 1;
            break;*/
        default:
            goto usage_error;
        }
    }
	
	/*
    if ( bDriverTraceLvl )
    {
        exitcode = setDriverTraceLevel(bDriverTraceLvl);
        if ( exitcode == 0 )
            printf("\nTdmfBlock driver trace level successfully set to %d.",bDriverTraceLvl);
        else
            printf("\nERROR, could not set TdmfBlock driver trace level !"
                   "\nThe installed version of the TdmfBlock driver may not support this feature.");
        exit( exitcode );
    }
	*/
	
	if (ftd_sock_startup() == -1) {
        printf("\nERROR, internal error (1)!");
		exitcode = 1;
		goto errexit;
	}

	if (ftd_init_errfac(
		PRODUCTNAME,
		argv[0], NULL, NULL, 0, 1) == NULL) {
        printf("\nERROR, internal error (2)!");
		exitcode = 1;
		goto errexit;
	}

	if ((fsockp = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL) {
        printf("\nERROR, unable to create socket!");
		exitcode = 1;
		goto errexit;
	}

	if (ftd_sock_init(fsockp, "localhost", "localhost", LOCALHOSTIP, LOCALHOSTIP,
		SOCK_STREAM, AF_INET, 1, 0) < 0)
	{
        reporterr(ERRFAC, M_NOCONFIG, ERRWARN);
        printf("\nERROR, unable to initialize socket!");
		exitcode = 1;
        goto errexit;
	}

	if (!(portnum = ftd_sock_get_port(FTD_MASTER))) {
		portnum = FTD_SERVER_PORT;
	}
	FTD_SOCK_PORT(fsockp) = portnum;

	// connect the ipc socket object
	rc = ftd_sock_connect_nonb(fsockp, portnum, 3, 0, 0);
			
	if (rc == 1) {
		// got it
	} else if (rc == 0) {
/*
		reporterr (ERRFAC, M_SOCKCONNECT, ERRWARN, 
			FTD_SOCK_LHOST(fsockp), 
			FTD_SOCK_RHOST(fsockp), 
			FTD_SOCK_PORT(fsockp), 
			sock_strerror(ETIMEDOUT));*/
        printf("\nERROR, unable to connect to Tdmf Server (localhost, port=%d).",portnum);
		exitcode = 1;
		goto errexit;
	} else {
        printf("\nERROR, unable to send data to Tdmf Server."
               "\nThe installed version of the Tdmf Server may not support this feature.");
		exitcode = 1;
		goto errexit;
	}

	memset(&header, 0, sizeof(header));
   	header.magicvalue   = MAGICHDR;
    header.msgtype      = FTDCSETTRACELEVEL;
	header.cli          = (void*)1;
	header.msg.data[0]  = level;

	if (FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"main" ,fsockp, &header) < 0) {
        printf("\nERROR, unable to send data to Tdmf Server."
               "\nThe installed version of the Tdmf Server may not support this feature.");
		exitcode = 1;
		goto errexit;
	}

	// get ack from master
	rc = FTD_SOCK_RECV_HEADER(__FILE__ ,"main",fsockp, &ack);
	if (rc < 0) {
        printf("\nERROR, Tdmf Server did not respond to request."
               "\nThe installed version of the Tdmf Server may not support this feature.");
		exitcode = 1;
		goto errexit;
	}

	if ( ack.msgtype != FTDCSETTRACELEVELACK ) {
		//reporterr(ERRFAC, M_NOPMD, ERRWARN, ack.msg.lg.lgnum);
        printf("\nERROR, Tdmf Server did not acknowledged."
               "\nThe installed version of the Tdmf Server may not support this feature.");
		exitcode = 1;
		goto errexit;
	}
    printf("\nTdmf Server trace level successfully set to %d.",level);


/*
    sprintf(cfgpath, "%s", PATH_CONFIG);
	// get all primary configs 
	cfglist = ftd_config_create_list();
	
    exitcode = 0;
    if (all != 0) {
        if (autostart) {
            // get paths of previously started groups 
			if (ftd_config_get_primary_started(cfgpath, cfglist) < 0) {
				exitcode = 1;
				goto errexit;
			}
            if (SizeOfLL(cfglist) == 0) {
				exitcode = 1;
                goto errexit;
            }
        } else {
            // get paths of all groups 
			if (ftd_config_get_primary(cfgpath, cfglist) < 0) {
				exitcode = 1;
				goto errexit;
			}
            if (SizeOfLL(cfglist) == 0) {
                reporterr(ERRFAC, M_NOCONFIG, ERRWARN);
				exitcode = 1;
                goto errexit;
            }
        }
		ForEachLLElement(cfglist, cfgpp) {
			cfgp = *cfgpp;
			if (start_group(cfgp->lgnum, force, autostart)!=0) {
                reporterr(ERRFAC, M_STARTGRP, ERRCRIT, cfgp->lgnum);
				exitcode = 1;
            }
        }
    } else {
        if (start_group(group, force, autostart)!=0) {
            reporterr(ERRFAC, M_STARTGRP, ERRCRIT, group);
			exitcode = 1;
        }
    }
*/

    ftd_sock_delete(&fsockp);
	ftd_delete_errfac();
	ftd_sock_cleanup();

#if defined(_WINDOWS) && defined(_DEBUG)
	printf("\nPress any key to continue....\n");
	while(!_kbhit())
		Sleep(1);
#endif

	exit(exitcode);

	return exitcode; // for stupid compiler 

usage_error:
	exitcode = 1;
	usage(argv);

errexit:

    ftd_sock_delete(&fsockp);
	ftd_delete_errfac();
	ftd_sock_cleanup();

#if defined(_WINDOWS) && defined(_DEBUG)
	printf("\nPress any key to continue....\n");
	while(!_kbhit())
		Sleep(1);
#endif

	exit(exitcode);
	
	return 0; /* for stupid compiler */
}
