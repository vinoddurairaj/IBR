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
/*
 * Copyright (c) 1998 The FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
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
#endif
#include "errors.h"
#include "devcheck.h"
#include "config.h"
#include "misc.h"
#include "pathnames.h"
#include "ftd_cmd.h"
#include "common.h"     /* WR16973 */
#if defined(_AIX)
#include "aixcmn.h"
#endif
#include "ps_intr.h"

#if defined(HPUX) && (SYSVERS >= 1100)
#define DLKM
#endif

static char *progname = NULL;

/* for errors: */
char *argv0;

extern char *paths;
static char configpaths[MAXLG][32];

// Proportional HRDB Tracking Resolution structure
static tracking_resolution_info     HRDB_tracking_resolution;

static int init_bab(int bab_size);
static int init_pstore(char *dev_name);
static int init_group_pstore(int group, int delete_devices_also);
static int init_HRDB_Tracking_resolution_level( tracking_resolution_info *HRDB_tracking_resolution, char *Pstore_name );

void init_hrt_size(int hrt_size);

extern int ps_get_tracking_resolution_info( tracking_resolution_info *tracking_res_info_ptr );

static void
usage(void)
{
    fprintf(stderr,
        "Usage: %s -b <bab_size> | -p <pstore_device> | -g <group number> [-d] | -l | -s | -r <HIGH | MEDIUM | LOW> [-p <pstore_device]\n", progname);
    fprintf(stderr,
        "\t-b <bab_size> is integral number of megabytes for the BAB. (%d - %d)\n",
        BAB_SIZE_MIN, BAB_SIZE_MAX);
    fprintf(stderr,
        "\t-p <pstore_device> is the device to initialize to use as pstore (it zeroes the device data).\n");
    fprintf(stderr,
        "\t-g <group number> is the replication group for which to reset the pstore information (without affecting other groups in the same Pstore).\n");
    fprintf(stderr,
        "\t-d, optional with -g, tells to clear also the specified group's device table in the Pstore.\n");
    fprintf(stderr,
        "\t-l will initialize all pstores with large HRT (12MB per device). This option should be used for large Replication devices.\n");
    fprintf(stderr,
        "\t-s will initialize all pstores with small HRT (128KB per device). This option should be used for small Replication devices.\n");
    fprintf(stderr,
        "\t-r will initialize all (or specified) pstore(s) for Proportional HRT. This option should be used to make HRDB sizes proportional to device sizes.\n");
    fprintf(stderr,
        "\t\t-r must specify a tracking Resolution among HIGH | MEDIUM | LOW (upper or lower case).\n" );
}

int
main(int argc, char *argv[])
{
    int ch;
    int bflag = 0;      /* -b option flag */
    int pflag = 0;      /* -p option flag */
    int gflag = 0;      /* -g option flag */
    int sflag = 0;      /* -s option flag */
    int lflag = 0;      /* -l option flag */
    int rflag = 0;      /* -r option flag */
	int dflag = 0;      /* -d option, goes with -g (optionally) */
    long    babsize = 0;
    char    ps_name[MAXPATHLEN];
    int rc = 0;
    int     hrt_type = FTD_HS_NOT_SET;
    int group;

    putenv("LANG=C");

    /* Make sure we are root */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }

    progname = argv[0];

    if (argc < 2) {
        usage();
        exit(1);
    }

    /*
     * check the second argument.
     * If the first character is not hyphen, it's error.
     */
    if (*argv[1] != '-') {
        usage();
        exit(1);
    }

    argv0 = argv[0];

    /* process all of the command line options */
    opterr = 0;
    while ((ch = getopt(argc, argv, "b:p:g:lsr:dh")) != -1) {
        switch (ch) {
        case 'b':
            if (bflag) {
                fprintf(stderr, "-b options are multiple specified\n");
                usage();
                exit(1);
            }
            babsize = ftd_strtol(optarg);
            if (babsize < BAB_SIZE_MIN || babsize > BAB_SIZE_MAX) {
                fprintf(stderr, "Invalid value for BAB size\n");
                usage();
                exit(1);
            }
            bflag++;
            break;
        case 'p':
            if (pflag) {
                fprintf(stderr, "-p options are multiple specified\n");
                usage();
                exit(1);
            }
            if (gflag) {
                fprintf(stderr, "-p and -g options are mutually exclusive\n");
                usage();
                exit(1);
            }
            if (strlen(optarg) >= MAXPATHLEN) {
                fprintf(stderr, "Too long name for device name\n");
                usage();
                exit(1);
            }
            (void)strcpy(ps_name, optarg);
            pflag++;
            break;
		/* Addition of -g option as per WR PROD1632, request to support pstore initialization 
		   for a single group without affecting other groups. */
        case 'g':
            if (gflag) {
                fprintf(stderr, "-g options are multiple specified\n");
                usage();
                exit(1);
            }
            if (pflag) {
                fprintf(stderr, "-p and -g options are mutually exclusive\n");
                usage();
                exit(1);
            }
            group = ftd_strtol(optarg);
            if (group < 0 || group >= MAXLG) {
                fprintf(stderr, "Invalid number for " GROUPNAME " group\n");
                usage();
                exit(1);
            }
            gflag++;
            break;
        case 's':
            hrt_type = FTD_HS_SMALL;
            sflag = 1;
            break;
        case 'l':
            hrt_type = FTD_HS_LARGE;
            lflag = 1;
            break;
        case 'r':
            if (rflag) {
                fprintf(stderr, "-r options are multiple specified\n");
                usage();
                exit(1);
            }
            if (lflag || sflag) {
                fprintf(stderr, "-l, -s and -r options are mutually exclusive\n");
                usage();
                exit(1);
            }
			if( strlen( optarg ) == 0 )
			{
			   /* If resolution level unspecified, take default tracking resolution */
               HRDB_tracking_resolution.level = PS_DEFAULT_TRACKING_RES;
               fprintf(stderr, "Tracking resolution keyword unspecified; will use default resolution.\n");
			}
			else if( (strcmp( optarg, "HIGH" ) == 0) || (strcmp( optarg, "high" ) == 0) )
			{
               HRDB_tracking_resolution.level = PS_HIGH_TRACKING_RES;
			}
			else if( (strcmp( optarg, "MEDIUM" ) == 0) || (strcmp( optarg, "medium" ) == 0) )
			{
               HRDB_tracking_resolution.level = PS_MEDIUM_TRACKING_RES;
			}
			else if( (strcmp( optarg, "LOW" ) == 0) || (strcmp( optarg, "low" ) == 0) )
			{
               HRDB_tracking_resolution.level = PS_LOW_TRACKING_RES;
			}
			else
			{
                fprintf(stderr, "Invalid tracking resolution keyword.\n");
                usage();
                exit(1);
            }
            hrt_type = FTD_HS_PROPORTIONAL;
            rflag++;
            break;
        case 'd':
            dflag = 1;
            break;
        case 'h':
            usage();
            exit(0);
        default:
            usage();
            exit(1);
        }
    }

    if (optind != argc) {
        fprintf(stderr, "Invalid arguments\n");
        usage();
        exit(1);
    }

    if (bflag && (pflag || gflag)) {
        fprintf(stderr, "-b option cannot be given with -p or -g option\n");
        usage();
        exit(1);
    }
    if( (sflag && lflag) || (rflag && lflag) || (rflag && sflag) ) 
    {
            fprintf(stderr, "Only one of -s, -l and -r options should be specified\n");
            usage();
            exit(1);
    }
    if ((bflag || pflag || gflag) && (sflag || lflag)) 
    {
            fprintf(stderr, "The -s/-l options cannot be used along with -b/-p/-g options\n");
            usage();
            exit(1);
    }

    if ((bflag || gflag) && rflag) 
    {
            fprintf(stderr, "The -r option cannot be used along with -b/-g options\n");
            usage();
            exit(1);
    }

    if( dflag && !gflag )
	{
            fprintf(stderr, "The -d option cannot be used without the -g option\n");
            usage();
            exit(1);
	}

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    log_command(argc, argv);    /* trace the command in dtcerror.log */

    if (bflag) {
        rc = init_bab(babsize);
        if (rc != 0) {
            exit(1);
        }
    } else if (pflag && !rflag) {
        rc = init_pstore(ps_name);
        if (rc != 0) {
            exit(1);
        }
    } else if (gflag) {
	    /* WR PROD1632: pstore initialization for a single group without affecting other groups */
        rc = init_group_pstore(group, dflag);
        if (rc != 0) {
            exit(1);
        }
    } else if (sflag || lflag) {
        init_hrt_size(hrt_type);
    } else if (rflag) {
	    /* User selected Proportional HRDB tracking method */
		if( pflag )
		{
		  // The user specified a Pstore
          if( init_HRDB_Tracking_resolution_level( &HRDB_tracking_resolution, ps_name ) != 0 )
		  {
              // <<< error message? TODO;
		      exit(1);
		  }
		}
		else
		{
		  // No Pstore specified; initialize all Pstores
          if( init_HRDB_Tracking_resolution_level( &HRDB_tracking_resolution, NULL ) != 0 )
		  {
              // <<< error message? TODO;
		      exit(1);
		  }
		}
    }

    return 0;
}

/*
 * Initialize or resize BAB
 */
static int
init_bab(int bab_size)
{
    int rc;
    char    buf[32];
#if defined(HPUX)
    char    cmd[MAXPATHLEN * 2];
#endif
        unsigned long ftd_drive_chunksize;

    /*
     * WR16973:
     * check if any groups are running.
     * if yes, then the BAB initializing does not be executed.
     */
    if (GETCONFIGS(configpaths, 1, 1) != 0) {
        fprintf(stderr, "Cannot initialize BAB. Some " GROUPNAME " groups are running now. You should retry after stopping all " GROUPNAME "  groups\n");
        return (-1);
    }

    /*
     * If we are changing the bab size, change it in config file.
     * Change num_chunks according to bab_size and set chunk_size to
         * default. Later if we ready to make chunk_size tunable, remove
         * the #ifdef TUNABLE_CHUNKSIZE.
     */
#if defined(TUNABLE_CHUNKSIZE)
    if ((rc = cfg_get_key_value("chunk_size", buf, CFG_IS_NOT_STRINGVAL))
                == 0) {
                ftd_drive_chunksize = strtol(buf, NULL, 0);
        } else {
#endif
                ftd_drive_chunksize = FTD_DRIVER_CHUNKSIZE;
#if defined(TUNABLE_CHUNKSIZE)
        }
#endif

    sprintf(buf, "%lu", (unsigned long) ((unsigned long)(bab_size * 1024) / ((unsigned long)ftd_drive_chunksize / 1024)));
    rc = cfg_set_key_value("num_chunks", buf, CFG_IS_NOT_STRINGVAL);
    if (rc != CFG_OK) {
        reporterr(ERRFAC, M_SYSCFGERR, ERRCRIT,
                    "couldn't set num_chunks");
        return (-1);
    }
    sprintf(buf, "%lu", ftd_drive_chunksize);
    rc = cfg_set_key_value("chunk_size", buf, CFG_IS_NOT_STRINGVAL);
    if (rc != CFG_OK) {
        reporterr(ERRFAC, M_SYSCFGERR, ERRCRIT,
                    "couldn't set chunk_size");
        return (-1);
    }

#if defined(linux)
        if (update_config_value() != CFG_OK) {
                fprintf(stderr, "updating %s failed\n", MODULES_CONFIG_PATH);
                exit(1);
        }
#endif /* defined(linux) */

#if defined(HPUX)
#if (SYSVERS >= 1100)

    #if (SYSVERS < 1123)
        
    /* NOTE: stdin must be a non-terminal device to avoid prompts */
    /* NOTE: Combining stdout and stderr */
    /* execute kmtune */
        sprintf(cmd, "/usr/sbin/kmtune -s ftd_num_chunk=%d </dev/null 2>&1", bab_size);
        if (system(cmd) != 0) {
                fprintf(stderr, "/usr/sbin/kmtune ftd_num_chunk failed\n");
               return (-1);
        }
    
    #else 
       /* execute kc commands */
    sprintf(cmd, "/usr/sbin/kcmodule -C %s %s=unused </dev/null 2>&1", PRODUCTNAME_TOKEN, QNM);
    if (system(cmd) != 0) {
        fprintf(stderr, "/usr/sbin/kcmodule unused failed\n");
        return (-1);
    }
  
       sprintf(cmd, "/usr/sbin/kctune -C %s -s ftd_num_chunk=%d </dev/null 2>&1", PRODUCTNAME_TOKEN, bab_size);
       if (system(cmd) != 0) {
       fprintf(stderr, "/usr/sbin/kctune ftd_num_chunk failed\n");
       return (-1);
       }
 
    #endif

#if defined(DLKM)

    #if (SYSVERS < 1123)

    /* execute kmsystem */
    sprintf(cmd, "/usr/sbin/kmsystem -l Y %s </dev/null 2>&1", QNM);
    if (system(cmd) != 0) {
        fprintf(stderr, "/usr/sbin/kmsystem -l Y failed\n");
        return (-1);
    }

    /* execute config */
    sprintf(cmd, "/usr/sbin/config -M %s -u </dev/null 2>&1", QNM);
    if (system(cmd) != 0) {
        fprintf(stderr, "/usr/sbin/config -M failed\n");
        return (-1);
    }
  
     #else
        sprintf(cmd, "/usr/sbin/kcmodule -C %s %s=loaded </dev/null 2>&1", PRODUCTNAME_TOKEN, QNM);
        if (system(cmd) != 0) {
        fprintf(stderr, "/usr/sbin/kcmodule loaded failed\n");
        return (-1);
        }
    #endif 

#else /* defined(DLKM) */

        # if (SYSVERS < 1123)

    /* execute kmsystem */
    sprintf(cmd, "/usr/sbin/kmsystem -l N %s </dev/null 2>&1", QNM);
    if (system(cmd) != 0) {
        fprintf(stderr, "/usr/sbin/kmsystem l N failed\n");
        return (-1);
    }

    /* execute config */
    sprintf(cmd, "/usr/sbin/config -u /stand/system </dev/null 2>&1");
    if (system(cmd) != 0) {
        fprintf(stderr, "/usr/sbin/config -u failed\n");
        return (-1);
    }

    /* execute kmupdate */
    sprintf(cmd, "/usr/sbin/kmupdate /stand/build/vmunix_test </dev/null 2>&1");
    if (system(cmd) != 0) {
        fprintf(stderr, "/usr/sbin/kmupdate vmunix_test failed\n");
        return (-1);
    }
     
        #else
        /* 
         * Kernel Loading for Non DLKM mode is not tested 
         * on Hpux 11.23 and later
         */
        sprintf(cmd, "/usr/sbin/kcmodule -C %s %s=static < /dev/null 2>&1", PRODUCTNAME_TOKEN, QNM);
        if (system(cmd) != 0) {
        fprintf(stderr, "/usr/sbin/kcmodule static failed\n");
        return (-1);
        }
    #endif

        
#endif /* defined(DLKM) */

#else /* (SYSVERS >= 1100) */

    /* execute the shell script that rebuilds the kernel */
        sprintf(cmd,
        PATH_LIBEXEC_FILES "/" QNM "_bab_rebuild -b " QNM " -s %d -p %d",
        bab_size * 1024 * 1024, BUF_POOL_MEM_SIZ);
    if (system(cmd) != 0) {
        fprintf(stderr, "Rebuilding kernel failed\n");
        return (-1);
    }

#endif /* (SYSVERS >= 1100) */
#endif /* defined(HPUX) */

    return (0);
}

/*
 * Initialize pstore device (this just zeroes the specified Pstore)
 */
static int
init_pstore(char *dev_name)
{
    int numlgs;
    int i;
    int group;
    char    ps_name[MAXPATHLEN];
    int exist = 0;
    char    raw_name[MAXPATHLEN];
    int fd;
  u_longlong_t blocks;  
  u_longlong_t size;    
  char  wbuf[PS_CLEAR_SIZE];
    int cnt;

    if (dev_name == NULL) {     /* not happen */
        fprintf(stderr, "The pstore device is not specified\n");
        return (-1);
    }

    paths = (char *)configpaths;

    /*
     * check if the specified device is used in running groups.
     * if yes, then the initializing pstore does not be executed.
     */
    numlgs = GETCONFIGS(configpaths, 1, 1);
    for (i = 0; i < numlgs; i++) {
        group = cfgtonum(i);
        if (GETPSNAME(group, ps_name) != 0) {
            fprintf(stderr, "Cannot get pstore name from %s\n", configpaths[i]);
            return (-1);
        }
        if (strcmp(dev_name, ps_name) == 0) {
            fprintf(stderr, "The specified device is already used as pstore of LG%03d\n", group);
            return (-1);
        }
    }

    /*
     * check if the specified device is already defined in any p###.cfg.
     * if no, then the initializing pstore does not be executed.
     */
    numlgs = GETCONFIGS(configpaths, 1, 0);
    for (i = 0; i < numlgs; i++) {
        group = cfgtonum(i);
        if (GETPSNAME(group, ps_name) != 0) {
            fprintf(stderr, "Cannot get pstore name from %s\n", configpaths[i]);
            return (-1);
        }
        if (strcmp(dev_name, ps_name) == 0) {
            exist = 1;  /* found in p###.cfg file */
            break;
        }
    }
    if (exist == 0) {
        fprintf(stderr, "The specified device is not defined as pstore in any groups\n");
        return (-1);
    }

    /*
     * convert to raw device name, if path is block device name
     */
    force_dsk_or_rdsk(raw_name, dev_name, 1);

    /* Call disksize() function instead of calling fdisksize() directly, 
       to handle vpath (IBM SDD) devices case (pc080109) */
    blocks = disksize(raw_name);
    if (blocks == -1) {
        fprintf(stderr, "Cannot get the disk size of %s\n", dev_name);
        return (-1);
    }

    fd = open(raw_name, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Cannot open device - %s\n",strerror(errno));
        return (-1);
    }

    size = blocks * DEV_BSIZE;

    memset(wbuf, 0, sizeof(wbuf));

    printf("Initialize %s (%llu bytes) ... ", dev_name, size);
    fflush(stdout);

        /* fd is set to the  beginning of the file */
        llseek(fd, (offset_t)0, SEEK_SET); 
    while (size > 0) {
        if (size >= PS_CLEAR_SIZE) {
            cnt = write(fd, wbuf, PS_CLEAR_SIZE);
            if (cnt != PS_CLEAR_SIZE) {
                goto write_error;
            }
            size -= PS_CLEAR_SIZE;
        } else {
            /* last writing for the rest */
            cnt = write(fd, wbuf, size);
            if (cnt != size) {
                goto write_error;
            }
            break;
        }
    }

    printf("completed successfully\n");

    close(fd);
    return (0);

write_error:
    fprintf(stderr, "\nWrite failed - %s\n", strerror(errno));
    close(fd);
    return (-1);
}

/*
 * Initialize pstore device for a single group by adding the group to a non-empty pstore
 * WR PROD1632: pstore initialization for a single group without affecting other groups
 */
static int
init_add_group(char *ps_name, char *group_name, ps_version_1_attr_t *pstore_attributes, char *group_attributes, int group_number)
{
    ps_group_info_t     group_info;
    int                 ret = 0;

    group_info.name = group_name;
	/* TODO: this code does the same as the original dtcstart code, and the hostid that we get is 0.
	   Tracing the original (non modified) dtcstart upon adding a group to the pstore, it also gets and sets 0
	   as hostid in the pstore. This does not seem to have any effect since it is like this in the original code,
	   but should be fixed at some point. */
    group_info.hostid = mysys->hostid;
    ret = ps_add_group(ps_name, &group_info);
    if (ret != PS_OK)				  
    {
        reporterr(ERRFAC, M_PSADDGRP, ERRCRIT, group_name, ps_name);
        return -1;
    }

    ret = ps_set_group_attr(ps_name, group_name, group_attributes, pstore_attributes->group_attr_size);
    if (ret != PS_OK) {
        ps_delete_group(ps_name, group_name, group_number, 0);
        reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, group_name, ps_name);
        return -1;
    }

    /* set the group state to PASSTHRU */
    ret = ps_set_group_state(ps_name, group_name, FTD_MODE_PASSTHRU);

    if (ret != PS_OK)
    {
        ps_delete_group(ps_name, group_name, group_number, 0);
        reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, group_name, ps_name);
        return -1;
    }

    /* set the group checkpoint state to default */
    if ((ret = ps_set_group_checkpoint(ps_name, group_name, 0)) != PS_OK)
    {
        ps_delete_group(ps_name, group_name, group_number, 0);
        reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, group_name, ps_name);
        return -1;
    }
    return 0;
} 

/*
 * Initialize pstore device for a single group
 * WR PROD1632: pstore initialization for a single group without affecting other groups
 */
static int
init_group_pstore(int group, int delete_devices_also)
{
    int     numlgs, i, ret;
    char    ps_name[MAXPATHLEN];
    char    group_name[MAXPATHLEN];
    char    *group_attributes;

    ps_group_info_t      group_info;
    ps_version_1_attr_t  pstore_attributes;

        paths = (char *)configpaths;

    /*
     * Check if the specified group is running.
     * If yes, the group must be stopped before initializing its pstore.
     */
    numlgs = GETCONFIGS(configpaths, 1, 1);
    for (i = 0; i < numlgs; i++)
    {
        if( group == cfgtonum(i))
        {
            fprintf(stderr, "The specified group is currently running; must be stopped before initializing its pstore.\n");
            return (-1);
        }
    }
    if (GETPSNAME(group, ps_name) != 0) {
       fprintf(stderr, "Cannot get pstore name from config file of group %d\n", group);
       return (-1);
    }

    /* create the group name */
    FTD_CREATE_GROUP_NAME(group_name, group);

    /* Get pstore attributes */
	ret = ps_get_version_1_attr(ps_name, &pstore_attributes, 1);
	if( ret == PS_INVALID_PS_VERSION )
	{
        exit(1);
	}
    if (ret == PS_BOGUS_HEADER)
    {
        /* This pstore was never initialized; call original init_pstore() function */
        if (init_pstore(ps_name) != 0)
            exit(1);
        else
            exit(0);
    }

    if ((group_attributes = (char *)calloc(pstore_attributes.group_attr_size, 1)) == NULL)
    {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, pstore_attributes.group_attr_size);
        return -1;
    }

    /* Pstore header OK and pstore attributes ok, get group info from pstore */
    ret = ps_get_group_info(ps_name, group_name, &group_info);
    if (ret == PS_GROUP_NOT_FOUND)
    {
        strcpy(group_attributes, FTD_DEFAULT_TUNABLES);
        /* This group was not already in the pstore; add it */
        if (init_add_group(ps_name, group_name, &pstore_attributes, group_attributes, group) != 0)
		{
            fprintf(stderr, "Failed adding group number %d to Pstore %s.\n", group, ps_name);
            free(group_attributes);
            exit(1);
        }
        else
		{
            fprintf(stderr, "Group number %d has been added to Pstore %s.\n", group, ps_name);
            free(group_attributes);
            exit(0);
        }
    }

    /* Getting here means that the group was already in the pstore and the user
       wants to reinitialize only for this group; delete the group from the pstore
       and add it back in a reinitialized state
   */
    /* Get group attributes; if error, set default group attributes */
    if (ps_get_group_attr(ps_name, group_name, group_attributes, pstore_attributes.group_attr_size) != 0)
    {
        strcpy(group_attributes, FTD_DEFAULT_TUNABLES);
    }

    ps_delete_group(ps_name, group_name, group, delete_devices_also);
    /* Add the group back in a reinitialized state */
    ret = init_add_group(ps_name, group_name, &pstore_attributes, group_attributes, group);
    free(group_attributes);

    if( ret == 0 )
	{
        fprintf(stderr, "Group %d data has been reinitialized in Pstore %s.\n", group, ps_name);
        exit(0);
	}
    else
	{
        exit(1);
	}
}

/*
 * Create pstores with HRT size as per the argument hrt_type
 */
void init_hrt_size(int hrt_type)
{
        int ctlfd, i, numlgs, startedlgs, group;
        int64_t max_devices;
        char ps_name[MAXPATHLEN];
        ps_version_1_attr_t attr;

        paths = (char *)configpaths;

        /* Don't initialize pstores if groups are started. */
        startedlgs = GETCONFIGS(configpaths, 1, 1);
        if (startedlgs > 0) {
                fprintf(stderr, "Some groups are running. Please stop all the groups before changing the HRT size.\n");
                exit(1);
        }

        if ((ctlfd = open(FTD_CTLDEV, O_RDWR)) < 0) {
                fprintf(stderr, "Error: Failed to open " PRODUCTNAME " master device: %s.  \nHas " QNM " driver been added?\n", FTD_CTLDEV);
                exit(1);
        }
        /*
         * Store the information in the driver. This will help in keeping HRT size common across
         * groups.
         */
        FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &hrt_type);
        close(ctlfd);

        numlgs = GETCONFIGS(configpaths, 1, 0);
        for (i = 0; i < numlgs; i++) {
                group = cfgtonum(i);
                if (GETPSNAME(group, ps_name) != 0) {
                        fprintf(stderr, "Cannot get pstore name from %s\n", configpaths[i]);
                        exit(1);
                }
                if (ps_get_version_1_attr(ps_name, &attr, 0) == PS_OK) {
                        if( attr.hrdb_type == hrt_type )
                                continue;
                }
                // Note: create_ps will get the hrt_type from the driver
                if (create_ps(ps_name, &max_devices, &HRDB_tracking_resolution) == 0) {
                        reporterr(ERRFAC, M_PSCREAT_SUCCESS, ERRINFO, ps_name, max_devices);
                } else {
                        reporterr(ERRFAC, M_PSCREATE, ERRCRIT, ps_name, group);
                        exit(1);
                }
        }
        if (hrt_type == FTD_HS_LARGE)
                fprintf(stderr, "All pstores support large HRTs.\n");
        else
                fprintf(stderr, "All pstores support small HRTs.\n");
}

/*
 * Create Pstore(s) for Proportional HRDB with specified tracking resolution level
*/
int init_HRDB_Tracking_resolution_level( tracking_resolution_info *HRDB_tracking_resolution_ptr, char *Pstore_name )
{
  char                 error_log_file[MAXPATHLEN];
  ps_version_1_attr_t  Pstore_attr;
  int                  ctlfd, i, numlgs, startedlgs, group;
  int                  HRDB_type;
  int64_t              max_devices;
  char                 ps_name[MAXPATHLEN];

  sprintf(error_log_file, "%s/" QNM "error.log", PATH_RUN_FILES);

  if( ps_get_tracking_resolution_info( HRDB_tracking_resolution_ptr ) != 0 )
  {
    fprintf( stderr, "An error occurred while attempting to get the Tracking resolution information.\n" );
    fprintf( stderr, "Please consult the error log file %s for details.\n", error_log_file );
	return( -1 );
  }

#ifdef DEBUG_PROP_HRDB  // TODO: delete all these once tests have progressed enough
    sprintf( debug_msg, "<<< In init_HRDB_Tracking_resolution_level, HRDB_tracking_resolution_ptr->level = %d <<<\n", HRDB_tracking_resolution_ptr->level ); // <<< delete
    reporterr(ERRFAC, M_GENMSG, ERRINFO, debug_msg);
#endif

  paths = (char *)configpaths;

  if( Pstore_name != NULL )
  {
    // A single Pstore has been specified
	// ----------------------------------

	// Check that the pstore does not have groups currently running (PROD9792)
    numlgs = GETCONFIGS(configpaths, 1, 1);
    for (i = 0; i < numlgs; i++) {
        group = cfgtonum(i);
        if (GETPSNAME(group, ps_name) != 0) {
            fprintf(stderr, "Cannot get pstore name from %s\n", configpaths[i]);
            return (-1);
        }
        if (strcmp(Pstore_name, ps_name) == 0) {
            fprintf(stderr, "The specified device is already used as pstore of LG%03d\n", group);
            return (-1);
        }
    }

    if ((ctlfd = open(FTD_CTLDEV, O_RDWR)) < 0)
    {
      fprintf(stderr, "Error: Failed to open " PRODUCTNAME " master device: %s.\nHas " QNM " driver been added?\n", FTD_CTLDEV);
      exit(1);
    }

    // Verify if the Pstores are already initialized for Legacy Small or Large HRT.
	// Do not allow a single Pstore init to Proportional HRDB if driver says the hrdb_type is Small or Large.
	// We can have different tracking resolution levels from one Pstore to another as long as they are all
	// of Proportional HRT type, but not a mixture of Legacy Small/Large HRT with Proportional HRT on same server. 
    // Prompt to use dtcinit -r without pstore specification to have a common Pstore type accross Pstores,
	// as the driver knows only about a single HRDB type.
    HRDB_type = FTD_HS_NOT_SET;
    FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &HRDB_type);
	if( (HRDB_type == FTD_HS_SMALL) || (HRDB_type == FTD_HS_LARGE) )
	{
        fprintf( stderr, "The driver indicates that Small or Large HRT is already selected as common Pstore type.\n" );
        fprintf( stderr, "It is not allowed to have a mixture of Pstore types. If you want to apply Proportional HRDB,\n" );
        fprintf( stderr, "it must be applied to all Pstores (dtcinit -r <resolution> without Pstore specification).\n" );
        close(ctlfd);
	    return( -1 );
	}

    // OK to proceed.
    // Give the HRDB type to the driver
    HRDB_type = FTD_HS_PROPORTIONAL;
    FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &HRDB_type);
    close(ctlfd);

    // Note: create_ps will get the hrt_type from the driver
    if( create_ps( Pstore_name, &max_devices, HRDB_tracking_resolution_ptr ) == 0)
    {
      ps_get_version_1_attr( Pstore_name, &Pstore_attr, 0 );
      reporterr( ERRFAC, M_PSCREAT_VARHRDB, ERRINFO, Pstore_name, 
                 ps_get_tracking_resolution_string(HRDB_tracking_resolution_ptr->level), (unsigned int)max_devices );
    }
    else
    {
      reporterr( ERRFAC, M_PSCREAT_FAILED, ERRCRIT, ps_name );
      exit(1);
    }
  }
  else
  {
    // Initialize all Pstores
	// ----------------------
	// NOTE: the usage of this command will cause that any group that was already in the Pstore(s) will be in
	//       Passthru state and will need to perform a Full Refresh

    /* Don't initialize if groups are started. */
    startedlgs = GETCONFIGS(configpaths, 1, 1);
    if (startedlgs > 0)
    {
      fprintf(stderr, "Some groups are running. Please stop all the groups before executing this command.\n");
      exit(1);
    }

    // Give the HRDB type to the driver
    if ((ctlfd = open(FTD_CTLDEV, O_RDWR)) < 0)
    {
      fprintf(stderr, "Error: Failed to open " PRODUCTNAME " master device: %s.\nHas " QNM " driver been added?\n", FTD_CTLDEV);
      exit(1);
    }
    HRDB_type = FTD_HS_PROPORTIONAL;
    FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &HRDB_type);
    close(ctlfd);

    numlgs = GETCONFIGS(configpaths, 1, 0);

    for (i = 0; i < numlgs; i++)
    {
      group = cfgtonum(i);
      if (GETPSNAME(group, ps_name) != 0)
      {
        fprintf(stderr, "Cannot get pstore name from %s\n", configpaths[i]);
        exit(1);
      }
      if (ps_get_version_1_attr(ps_name, &Pstore_attr, 0) == PS_OK)
      {
		// This Pstore was already formatted before. Check the hrdb_type field of the header and the Tracking Resolution
        if( (Pstore_attr.hrdb_type == FTD_HS_PROPORTIONAL) && 
            (Pstore_attr.tracking_resolution_level == HRDB_tracking_resolution_ptr->level) )
		{
		  // This group's PStore already has the same Tracking resolution level.
		  // We have just processed it from another group's config. Continue to next group.
          continue;
        }
	  }

      // Note: create_ps will get the hrt_type from the driver
      if( create_ps( ps_name, &max_devices, HRDB_tracking_resolution_ptr ) == 0)
      {
        reporterr( ERRFAC, M_PSCREAT_VARHRDB, ERRINFO, ps_name, 
                   ps_get_tracking_resolution_string(HRDB_tracking_resolution_ptr->level), (unsigned int)max_devices );
      }
      else
      {
        reporterr(ERRFAC, M_PSCREATE, ERRCRIT, ps_name, group);
        exit(1);
      }
    }  // for( ...
    fprintf( stderr, "All Pstores support Proportional HRTs with %s Tracking Resolution level.\n",
                     ps_get_tracking_resolution_string(HRDB_tracking_resolution_ptr->level) );
  }
  return( 0 );
}

