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
/***************************************************************************
 * netanalysis.c - FullTime Data start network analysis command utility
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the functionality for starting a PMD/RMD pair
 * in Network data transfer mode for bandwidth analysis
 *
 ***************************************************************************/

#include <stdio.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "pathnames.h"
#include "errors.h"
#include "ipc.h"
#include "process.h"

#ifdef TDMF_TRACE
FILE *dbgfd;
#endif

char *argv0;
static int silent = 0;
static int mode;
static int sig;
static char *statstr;

extern int lgcnt;
extern int targets[MAXLG];

char configpaths[MAXLG][32];
char *paths = (char*)configpaths;
int len;

extern int ftdsock;

/*
 * usage -- This is how the program is used
 */
void usage(void)
{
    if (silent)
        exit(1);

    fprintf(stderr,"\nUsage: %s <options>\n", argv0);
    fprintf(stderr,"\t-n <number of PMD-RMD pairs to launch> (mandatory argument)\n");
    fprintf(stderr,"\t-h: print this usage paragraph\n\n");
	return;

} /* usage */

/* Check if a group number is unused
   Return: 0 if used; 1 if unused
*/
int group_number_unused( int group_number )
{
    int i;

    for (i = 0; i < lgcnt; i++)
    {
        if( cfgtonum(i) == group_number )
		    return( 0 );
    }
	return( 1 );
}

/*
 * Get the number of network analysis groups already running 
 */
static int num_of_netanalysis_groups_already_running()
{
    FILE *fp;
    char cmd[128];
    char data[8];
	int  number_of_network_analysis_cfg_files_found;

    sprintf( cmd, "/bin/grep NETWORK-ANALYSIS %s/p*.cfg | /bin/grep -i on | wc -l | /bin/awk '{print $1}'", PATH_CONFIG );

    fp = popen( cmd, "r" );
    if (fp == NULL)
    {
        return( 0 );
    }
	memset(data, 0x00, sizeof(data));
    if( fgets(data, sizeof(data), fp) == NULL )
	{
        pclose(fp);
	    return( 0 );
	}

    pclose(fp);
	number_of_network_analysis_cfg_files_found = atoi(data);

	return( number_of_network_analysis_cfg_files_found );
}


/*
 * parseargs -- parse the run-time arguments and create the vector of group numbers
 * and the group configuration files 
 */
void
parseargs(int argc, char **argv)
{
    int ch;
    int i;
	int number_of_PMD_RMD_pairs = 0;
	int group_number, group_numbers_found;
	int netanal_groups_already_running;
    
    /* At least one argument is required in addition to the command itself */
    if (argc == 1) {
        usage();
		exit(1);
    }
    opterr = 0;
    while ((ch = getopt(argc, argv, "n:ph")) != -1) {
        switch (ch)
        {
        case 'n':
            /* Number of PMD-RMD pairs to launch */
            number_of_PMD_RMD_pairs = ftd_strtol(optarg);
            if (number_of_PMD_RMD_pairs < 1 || number_of_PMD_RMD_pairs > 100) {
                fprintf(stderr, "[%s] is an invalid number for the number of PMD-RMD pairs to launch [1-100]\n", optarg);
                usage();
				exit(1);
            }
            break;
        case 'p':
            silent = 1;
            break;
        case 'h':
            usage();
            break;
        default:
            usage();
            break;
        }
    }
    
    if (optind != argc) {
        fprintf(stderr, "Invalid arguments\n");
        usage();
		exit(1);
    }

    if( number_of_PMD_RMD_pairs == 0 )
	    exit(1);

    netanal_groups_already_running = num_of_netanalysis_groups_already_running();
    if( (number_of_PMD_RMD_pairs + netanal_groups_already_running) > 100 )
	{
        reporterr(ERRFAC, M_TOOMANYNETGRPS, ERRCRIT, netanal_groups_already_running, number_of_PMD_RMD_pairs + netanal_groups_already_running);
	    exit(1);
	}

    sig  = FTDQNETANALYSIS;
    mode = FTDNETANALYSIS;

    // Find group numbers which are unused and set the corresponding targets[] vector entries
	group_numbers_found = 0;
	for( i = MAXLG-1; i >= 0; i-- )
	{
	    if( group_number_unused(i) )
		{
            targets[i] = mode;
			if( ++group_numbers_found == number_of_PMD_RMD_pairs )
			    break;
		}
	}
	if( group_numbers_found < number_of_PMD_RMD_pairs )
	{
        reporterr(ERRFAC, M_NOTENOUGHGROUPS, ERRCRIT, number_of_PMD_RMD_pairs);
        exit(1);
	}

    return;
} /* parseargs */

//-------------------------------------------------------
static int set_real_group_number_in_dtc_device(char *filename, int lgnum)
{
	int     rc = 0;
	FILE    *infd;
	FILE    *outfd;
	char    buf[1024+1];
	char    new_dtc_device_line[1024+1];
	char    newfile[MAXPATHLEN];

    sprintf( newfile, "%s.dtcdevset", filename );
	// Delete any old temporary work file
	unlink( newfile );

	outfd = fopen( newfile, "w" );
	if( outfd <= (FILE *)NULL )
	{
		/* Output file open error */
		unlink( newfile );
		return( -1 );
	}
	infd = fopen( filename, "r" );
	if( infd <= (FILE *)NULL )
	{
		/* Base file open error */
		fclose( outfd );
		unlink( newfile );
		return (-1);
	}

    // Set the new dtc device line
	sprintf( new_dtc_device_line, "  DTC-DEVICE:       /dev/dtc/lg%d/dsk/dtc0\n", lgnum );

	/* Copy all lines, checking for the DTC-DEVICE line */
	for(;;)
	{
		memset( buf, 0x00, sizeof(buf) );
		fgets( buf, sizeof(buf), infd);
		if (strlen(buf) == 0)
		{
			break;
		}
		if( strstr( buf, "DTC-DEVICE" )	 != NULL )
		{
		    // Found the line to replace
		    fputs( new_dtc_device_line, outfd );
		}
		else
		{
		    fputs( buf, outfd );
		}
	}

	fclose( infd );
	fclose( outfd );

	// Make the output file the new effective file
	unlink( filename );
	sprintf( buf, "/bin/mv %s %s 1>/dev/null 2> /dev/null", newfile, filename);
	rc = system( buf );

	return( rc );
}


/*
 * main - start the daemons in network bandwidth analysis mode
 */
int main (int argc, char **argv, char **envp)
{
    pid_t mpmdpid;
    int i, rc;
    int	pcnt;
	char cmdline[128], filename[128];
	char *copy_command;

    putenv("LANG=C");

    /* -- Make sure we're root. */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }

    statstr = "network_analysis";
    sig  = FTDQNETANALYSIS;
    mode = FTDNETANALYSIS;


    FTD_TIME_STAMP(FTD_DBG_FLOW1, "%s start, statstr: %s\n", argv[0], statstr);

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }
  
    argv0 = argv[0];
    log_command(argc, argv);    /* trace command line in dtcerror.log */

    memset(targets, 0, sizeof(targets));

    lgcnt = GETCONFIGS(configpaths, 1, 0);
    parseargs(argc, argv);

    if ((mpmdpid = getprocessid(FTDD, 0, &pcnt)) <= 0) {
        reporterr(ERRFAC, M_NOMASTER, ERRWARN, argv0);
        exit(1);
    }
    if ((ftdsock = connecttomaster(FTDD, 0)) == -1) {
        exit(1);
    }

    // If all went well, create the configuration files by copying the base config file
	// prepared by the launchnetanalysis script, making one copy for each group
#if defined(linux)
	copy_command = "/bin/cp";
#else
	copy_command = "/usr/bin/cp";
#endif
	for( i = MAXLG-1; i >= 0; i-- )
	{
        if( targets[i] == FTDNETANALYSIS )
		{
			sprintf(cmdline, "%s %s/net_analysis_group.cfg %s/p%03d.cfg 1>/dev/null 2> /dev/null", copy_command, PATH_CONFIG, PATH_CONFIG, i);
			rc = system(cmdline);
	        if( rc == 0 )
			{
			    // Replace the group number by the actual group number in the dtc device name in the config file just created
			   sprintf( filename, "%s/p%03d.cfg", PATH_CONFIG, i );
               if( i != 999 )
			   {
			       if( (rc = set_real_group_number_in_dtc_device(filename, i)) != 0 )
				   {
				       unlink( filename );
	                   reporterr(ERRFAC, M_NETCFG_DTCERR, ERRCRIT, filename, rc);
	                   exit(1);
				   }
			   }
			    // Create also the .cur file to simulate started groups
				sprintf(cmdline, "%s %s/p%03d.cfg %s/p%03d.cur 1>/dev/null 2> /dev/null", copy_command, PATH_CONFIG, i, PATH_CONFIG, i);
				if( (rc = system(cmdline)) != 0)
				{
					unlink( filename );
				}
			}
	        if( rc != 0 )
			{
	            reporterr(ERRFAC, M_NETCFG_COPYERR, ERRCRIT, i, i, rc);
	            exit(1);
			}
		}
	}

    if (ipcsend_sig(ftdsock, 0, &sig) == -1) {
        exit(1);
    }
    if (ipcsend_lg_list(ftdsock) == -1) {
        exit(1);
    }
    return 0;
} /* main */
