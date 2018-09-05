/*
 * ftd_init.c - initialize the persistent store or BAB
 *
 * Copyright (c) 1998 The FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 *
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h> 
#include <malloc.h> 
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <values.h>

#include "cfg_intr.h"
#include "ftdio.h"
#if defined(HPUX)
#include "ftd_buf.h"
#endif /* defined(HPUX) */
#include "errors.h"
#include "devcheck.h"
#include "config.h"
#include "pathnames.h"

#include "ftd_cmd.h"

#if defined(HPUX) && (SYSVERS >= 1100)
//#define DLKM
#endif

static char *progname = NULL;

/* for errors: */
char *argv0;


static void
usage(void)
{
    fprintf(stderr, "Usage: %s [-b bab_size_MB (1 - %d)]\n",
		progname, (MAXINT - (500*1048576))/1048576);
    fprintf(stderr, "       bab_size is number of megabytes.\n");
}

int 
main(int argc, char *argv[])
{

    char buf[32];
#if defined(_AIX)
    int  ch;
#else /* defined(_AIX) */
    char ch;
#endif /* defined(_AIX) */
    long bab_size;
    int  create, ret;
#if defined(HPUX)
    char cmd[MAXPATHLEN * 2];
#endif

    progname = argv[0];
    if (argc < 3) {
        goto usage_error;
    }
    argv0 = argv[0];

    bab_size = 0;
    create = 0;

    /* process all of the command line options */
    opterr = 0;
    while ((ch = getopt(argc, argv, "b:")) != -1) {
        switch(ch) {
        case 'b':
            bab_size = strtol(optarg, NULL, 0);
            if (bab_size <= 0 || bab_size > (MAXINT - (500*1048576))/1048576) {
                goto usage_error;
            }
            break;
        default:
            goto usage_error;
        }
    }

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    /* if we are changing the bab size, change it in config file */
    if (bab_size != 0) {
        /* change num_chunks to bab_size. set chunk_size */
	   
        sprintf(buf, "%lu", (unsigned long) ((unsigned long)(bab_size * 1024) /((unsigned long)FTD_DRIVER_CHUNKSIZE/1024)));
        if (cfg_set_key_value("num_chunks", buf, CFG_IS_NOT_STRINGVAL) != CFG_OK) {
            reporterr(ERRFAC, M_SYSCFGERR, ERRCRIT, 
                "couldn't set num_chunks");
            exit(1);
        }
        sprintf(buf, "%d", FTD_DRIVER_CHUNKSIZE);
        if (cfg_set_key_value("chunk_size", buf, CFG_IS_NOT_STRINGVAL) != CFG_OK) {
            reporterr(ERRFAC, M_SYSCFGERR, ERRCRIT, 
                "couldn't set chunk_size");
            exit(1);
        }
#if defined(HPUX) && (SYSVERS == 1020)
{
        /* execute the shell script that rebuilds the kernel */
        sprintf(cmd, 
               PATH_LIBEXEC_FILES "/" QNM "_bab_rebuild -b " QNM " -s %d -p %d", 
                bab_size*1024*1024, BUF_POOL_MEM_SIZ);
        if (system(cmd) != 0) {
		exit(1);
        }
}
#elif defined(HPUX) && (SYSVERS >= 1100)
{
	/* execute kmtune */
        sprintf(cmd, "/usr/sbin/kmtune -s ftd_num_chunk=%d", bab_size);
        if (system(cmd) != 0) {
        	printf("/usr/sbin/kmtune failed\n");
		exit(1);
        }

#if defined(DLKM)
	/* execute kmsystem */
        sprintf(cmd, "/usr/sbin/kmsystem -l Y " QNM " ");
        if (system(cmd) != 0) {
        	fprintf(stderr, "/usr/sbin/kmtune failed\n");
		exit(1);
        }

	/* execute config */
        sprintf(cmd, "/usr/sbin/config -M " QNM " -u");
        if (system(cmd) != 0) {
        	fprintf(stderr, "/usr/sbin/config failed\n");
		exit(1);
        }
#else /* DLKM */
	/* execute kmsystem */
        sprintf(cmd, "/usr/sbin/kmsystem -l N " QNM " ");
        if (system(cmd) != 0) {
        	fprintf(stderr, "/usr/sbin/kmtune failed\n");
		exit(1);
        }

	/* execute config */
        sprintf(cmd, "/usr/sbin/config -u /stand/system");
        if (system(cmd) != 0) {
        	fprintf(stderr, "/usr/sbin/config failed\n");
		exit(1);
        }

	/* execute kmupdate */
        sprintf(cmd, "/usr/sbin/kmupdate /stand/build/vmunix_test");
        if (system(cmd) != 0) {
        	fprintf(stderr, "/usr/sbin/kmupdate failed\n");
		exit(1);
        }
#endif  /* DLKM */
	
}
#endif
    }

    exit(0);
    return 0; /* for stupid compiler */

usage_error:
    usage();
    exit(1);
    return 0; /* for stupid compiler */
}
