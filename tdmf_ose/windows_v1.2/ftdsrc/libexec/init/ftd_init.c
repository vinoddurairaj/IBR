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

#include "ftd_port.h"
#include "ftd_error.h"
#include "ftd_cfgsys.h"
#include "ftd_ioctl.h"

// DTurrin - Oct 16th, 2001
// Make sure that Windows 2000 functions are used if available.
#if defined(_WINDOWS)
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif

#define BAB_GRANULARITY   32
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

#if defined(_WINDOWS)
	HANDLE	ctlfd = INVALID_HANDLE_VALUE;
	unsigned int actual_bab_size = 0;
    int availPhysMemMb = 0; 
	int maxBabSizeMb = 0;
	int curBabSizeMb = 0;

#if defined NTFOUR //NonPagedPool MAX SIZE for Win NT4.0
	int maxPagedPoolSizeMb = 160;
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

#if defined(_WINDOWS)
	// open the master control device 
	ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0);
	if (ctlfd == INVALID_HANDLE_VALUE) {
		reporterr(ERRFAC, M_CTLOPEN, ERRCRIT, ftd_strerror());
		exit(1);
		return 0; /* for stupid compiler */
	}
    /* get bab size from driver */
    ftd_ioctl_get_bab_size(ctlfd, &actual_bab_size); // in BYTES
	ftd_close(ctlfd);

    curBabSizeMb = actual_bab_size / (1024 * 1024);
	// 60% of AVAILABLE PagedPoolSize in 32 MB unit
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

    /* process all of the command line options */
    while ((ch = getopt(argc, argv, "b:")) != -1) {
        switch(ch) {
        case 'b':
            bab_size = strtol(optarg, &remainstr, 10);

			if (*remainstr != NULL) { //rddev 021127
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
            fprintf(stderr, "Usage: %s [-b bab_size_MB] , 32MB unit\n", argv0);
            goto usage_error;
        }
    }

	// Convert bab_size to 32MB unit
	bab_size = (bab_size / BAB_GRANULARITY) * BAB_GRANULARITY;
	fprintf(stderr, "Requested BAB size is %d MB (%dMB granularity)\n", bab_size, BAB_GRANULARITY);

	if (ftd_init_errfac(
#if !defined(_WINDOWS)
		CAPQ, 
#else
		PRODUCTNAME,
#endif		
		argv[0], NULL, NULL, 0, 1) == NULL) {
        exit(ERROR_CODE);
    }

    /* if we are changing the bab size, change it in config file */
    if (bab_size != 0) {
        /* change num_chunks to bab_size. set chunk_size to 512*1024 */
      //  sprintf(buf, "%d", bab_size*2);
		sprintf(buf, "%d", bab_size);
#if defined(_WINDOWS)
        if (cfg_set_driver_key_value("num_chunks", buf, CFG_IS_NOT_STRINGVAL) != CFG_OK) {
#else
        if (cfg_set_key_value("num_chunks", buf, CFG_IS_NOT_STRINGVAL) != CFG_OK) {
#endif            
			reporterr(ERRFAC, M_SYSCFGERR, ERRCRIT, 
                "couldn't set num_chunks");
            exit(ERROR_CODE);
        }
        sprintf(buf, "%d", 1024*1024);
#if defined(_WINDOWS)
        if (cfg_set_driver_key_value("chunk_size", buf, CFG_IS_NOT_STRINGVAL) != CFG_OK) {
#else
        if (cfg_set_key_value("chunk_size", buf, CFG_IS_NOT_STRINGVAL) != CFG_OK) {
#endif            
            reporterr(ERRFAC, M_SYSCFGERR, ERRCRIT, 
                "couldn't set chunk_size");
            exit(ERROR_CODE);
        }
#if defined(HPUX)
        /* execute the shell script that rebuilds the kernel */
        sprintf(cmd, PATH_LIBEXEC_FILES "/ftd_bab_rebuild -b ftd -s %d -d %d", 
                bab_size*1024*1024, max_dev);
		if (system(cmd) != 0) {
			exit(ERROR_CODE);
		}

#endif
    }

    exit(OK_CODE);
    return 0; /* for stupid compiler */

usage_error:
	fprintf(stderr, "       Actual BAB size: %d MB\n", curBabSizeMb);
	fprintf(stderr, "       BAB size minimum: 32 MB (required)\n");
	fprintf(stderr, "       BAB size maximum: %d MB (allowable) \n", maxBabSizeMb);
    exit(ERROR_CODE);
    return 0; /* for stupid compiler */
}
