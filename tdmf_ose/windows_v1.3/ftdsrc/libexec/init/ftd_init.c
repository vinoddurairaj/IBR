/*
 * ftd_init.c - initialize the persistent store or BAB
 *
 * Copyright (c) 1999 The Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 *
 */

// Include this library to force the linker to link the MFC libraries in the correct order Mike Pollett
#include "../../forcelib.h"

#include "ftd_port.h"
#include "ftd_error.h"
#include "ftd_cfgsys.h"
#include "ftd_ioctl.h"
#include "sock.h"
#include "ftd_devio.h"


// DTurrin - Oct 16th, 2001
// Make sure that Windows 2000 functions are used if available.
#if defined(_WINDOWS)
//#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif


#define BAB_GRANULARITY     1
#define MB_SIZE             (1024*1024)
//libftd, ftd_mngt_msgs.cpp
void ftd_mngt_msgs_log(const char **argv, int argc);

#define ERROR_CODE  1
#define OK_CODE     0

static char *argv0;

int 
main(int argc, char *argv[])
{
    char buf[32];
	char *remainstr = NULL;
#if defined(_AIX)
    int  ch;
#else /* defined(_AIX) */
    char ch;
#endif /* defined(_AIX) */
    int  bab_size, max_dev, create;
#if defined(HPUX)
    char cmd[MAXPATHLEN * 2];
#endif

    int num_chunks = 0,
        chunk_size = 0;


#if defined(_WINDOWS)
	HANDLE	ctlfd = INVALID_HANDLE_VALUE;
	unsigned int actual_bab_size = 0;
        int availPhysMemMb = 0; 
	int maxBabSizeMb = 0;
	int curBabSizeMb = 0;

#if defined NTFOUR //NonPagedPool MAX SIZE for Win NT4.0
	int maxPagedPoolSizeMb = 192;         // WR17275 requirement, change from 160 to 192
#else //NonPagedPool MAX SIZE for Win 2K
	int maxPagedPoolSizeMb = 224;
#endif

#if !defined MEMORYSTATUSEX
    MEMORYSTATUS stat;
	GlobalMemoryStatus (&stat);
	availPhysMemMb = (int)(stat.dwTotalPhys / (1024 * 1024));     // Available PhysMem in MB
#else  // #if !defined MEMORYSTATUSEX
    // If the OS version is Windows 2000 or greater,
    // GlobalMemoryStatusEx must be used instead of
	// GlobalMemoryStatus in case the memory size
	// exceeds 4 GBs.
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);
	GlobalMemoryStatusEx (&statex);
	availPhysMemMb = (int)(statex.ullTotalPhys / (1024 * 1024)); // Available PhysMem in MB
#endif // #if !defined MEMORYSTATUSEX
#endif  // #if defined(_WINDOWS)

////------------------------------------------
#if defined(_WINDOWS)
	{
	void ftd_util_force_to_use_only_one_processor();
	ftd_util_force_to_use_only_one_processor();
	}
#endif
//------------------------------------------

	argv0 = argv[0];

    //
    // Moved errfac init here (because on errors we try to talk to it!)
    //
#if defined(_WINDOWS)
    if (sock_startup() == -1) {
        exit(ERROR_CODE);
    }
#endif

    if (ftd_init_errfac("Replicator", argv[0], NULL, NULL, 0, 1) == NULL) {
        exit(ERROR_CODE);
    }


#if defined(_WINDOWS)
	// open the master control device 
	ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0);
    if (ctlfd == INVALID_HANDLE_VALUE) 
    {
		reporterr(ERRFAC, M_CTLOPEN, ERRCRIT, ftd_strerror());
        ftd_delete_errfac();
		exit(1);
		return 0; /* for stupid compiler */
	}
    /* get bab size from driver */
    ftd_ioctl_get_bab_size(ctlfd, &actual_bab_size); // in BYTES
	FTD_CLOSE_FUNC(__FILE__,__LINE__,ctlfd);

    curBabSizeMb = actual_bab_size / (1024 * 1024);
    // 60% of AVAILABLE PagedPoolSize in BAB_GRANULARITY MB unit
	maxBabSizeMb = (((availPhysMemMb * 6) / 10) / BAB_GRANULARITY) * BAB_GRANULARITY; 

	if (maxBabSizeMb > maxPagedPoolSizeMb) 
		maxBabSizeMb = maxPagedPoolSizeMb;
#endif

	if (argc < 3) {
		fprintf(stderr, "Usage: %s [-b bab_size_MB]\n", argv0);
		goto usage_error;
	}

    max_dev =  0;
    bab_size = 0;
    create = 0;

    //
    // Fix BAB change to reflect actual number in the registry
    // for the size of the chunks.
    // 

    //
    // Read chunk size
    //
    memset(buf, 0, sizeof(buf));
    cfg_get_driver_key_value("chunk_size", FTD_DRIVER_PARAMETERS_KEY_TYPE, buf, CFG_IS_NOT_STRINGVAL);
    chunk_size = strtol(buf, NULL, 0);
    //
    // We validate chunk size to be a multiple of MB
    // If not valid, set it to 1024KB!
    //
    if (    (!chunk_size)
        ||  (chunk_size%(1024*1024))    )
    {
        fprintf(stderr, "BAB chunk size is %d, chunk size must be multiple of 1MB\nReset to 1MB\n", chunk_size, BAB_GRANULARITY);
        reporterr(ERRFAC, M_SYSCFGERR, ERRCRIT, "BAB chunk size is not a multiple of 1MB");
        // invalid!
        chunk_size = 1024*1024;
    }


    /* process all of the command line options */
    while ((ch = getopt(argc, argv, "b:")) != -1) 
    {
        switch(ch) 
        {
        case 'b':
            bab_size = strtol(optarg, &remainstr, 10);

            if (*remainstr != NULL) //rddev 021127
            { 
				fprintf(stderr, "Usage: %s [-b bab_size_MB] , 32MB unit\n", argv0);
				goto usage_error;
			}

			if (maxBabSizeMb < 32) //rddev 020925
			{
				fprintf(stderr, "Error: Not enough PHYSICAL memory. \n");
				goto usage_error;
			}

			if ((bab_size < 32) || (bab_size > maxBabSizeMb)) //rddeb020925
			{
				fprintf(stderr, "Error: bab_size is outside limit's boundary.\n");
				goto usage_error;
			}
            break;
        default:
            fprintf(stderr, "Usage: %s [-b bab_size_MB] , with %dMB granularity\n", argv0, (chunk_size/MB_SIZE));
            goto usage_error;
        }
    }

    // Convert bab_size to chunk granularity unit
    bab_size = ( ( (bab_size * MB_SIZE ) / chunk_size ) * chunk_size ) / MB_SIZE;

    fprintf(stderr, "BAB chunk_size is %dMB\n", (chunk_size / MB_SIZE));
    fprintf(stderr, "Requested BAB size is %d MB (%dMB granularity)\n", bab_size, (chunk_size / MB_SIZE));


#if defined(_WINDOWS)
    //send status msg to System Event Log and TDMF Collector
    ftd_mngt_msgs_log(argv, argc);
#endif

    //
    // set num_chunks according to wanted size and chunk_size
    // 
    num_chunks = ((bab_size * (1024*1024)) / chunk_size);

    /* if we are changing the bab size, change it in config file */
    if (bab_size != 0) 
	{
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%d", num_chunks);
        if (cfg_set_driver_key_value("num_chunks", FTD_DRIVER_PARAMETERS_KEY_TYPE, buf, CFG_IS_NOT_STRINGVAL) != CFG_OK) 
		{
			reporterr(ERRFAC, M_SYSCFGERR, ERRCRIT, 
                "couldn't set num_chunks");
            exit(ERROR_CODE);
        }

        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%d", chunk_size);

        if (cfg_set_driver_key_value("chunk_size", FTD_DRIVER_PARAMETERS_KEY_TYPE, buf, CFG_IS_NOT_STRINGVAL) != CFG_OK) 
		{
            reporterr(ERRFAC, M_SYSCFGERR, ERRCRIT, 
                "couldn't set chunk_size");
            exit(ERROR_CODE);
        }
    }

    ftd_delete_errfac();

    exit(OK_CODE);
    return 0; /* for stupid compiler */

usage_error:
	fprintf(stderr, "       Actual BAB size: %d MB\n", curBabSizeMb);
	fprintf(stderr, "       BAB size minimum: 32 MB (required)\n");
	fprintf(stderr, "       BAB size maximum: %d MB (allowable) \n", maxBabSizeMb);

    ftd_delete_errfac();

    exit(ERROR_CODE);
    return 0; /* for stupid compiler */
}
