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
 * ftd_reconfig.c - Non disruptive adding volume.
 *
 * Copyright (c) 1998 The FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * This dtcreconfig command add a new devices to LG while it active.
 * Dtcreconfig read a new configuration from pxxx.cfg and read a current 
 * configuration from pxxx.cur to get a difference of these two.
 *
 * After get the new devices infomation, modify Pstore's device information.
 * And add new devices to driver.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <signal.h>
#if !defined(linux)
#include <macros.h>
#else
#include <sys/param.h>
#define  min MIN
#define  max MAX
#include <sys/wait.h>
#endif /* !defined(linux) */

#if defined(SOLARIS)
#include <sys/mkdev.h>
#endif

#include "cfg_intr.h"
#include "ps_intr.h"

#include "ftdio.h"
#include "ftdif.h"
#include "pathnames.h"
#include "config.h"
#include "ftd_cmd.h"
#include "errors.h"
#include "platform.h"
#include "devcheck.h"
#include "aixcmn.h"
#include "common.h"

static char *progname = NULL;

static int parent_ctlfd = 0;
static grp_started;

char *argv0;

static int modify_group(int group);
static int add_new_dev(char *ps_name, diffdev_t *diffdev, 
           char *group_name, int group, ftd_dev_info_t *devp);

static void
usage(void)
{
    fprintf(stderr, "Usage: %s -g <group#>\n", progname);
    fprintf(stderr, "\t<group#> is " GROUPNAME " group number. (0 - %d)\n", MAXLG-1);
}

/* this assumes 32-bits per int */ 

#if defined(HPUX) || defined(_AIX)    /* WR16074 */
#if defined(HPUX)
char ftdctlpath[] = "/sbin/" QNM "ctl";
#else /* defined(_AIX) WR16074 */
char ftdctlpath[] = "/usr/sbin/" QNM "ctl";
#endif
char control_daemon_psname[MAXPATHLEN];
#endif /* HPUX _AIX */

static int
modify_group(int group)
{
    int ret, ctlfd, i, state;
    int n;  
    char copybuf[BUFSIZ];
    char cfg_path[32];
    char cur_cfg_path[32];
    char full_cfg_path[MAXPATHLEN];
    char full_cur_cfg_path[MAXPATHLEN];
    char group_name[MAXPATHLEN];
    char ps_name[MAXPATHLEN];
    char * strp;
    ftd_state_t st;
    struct stat statbuf;
    int tempfd;
    int err,cnt;
    int infile, outfile;
    ps_version_1_attr_t attr;
    ps_group_info_t     group_info;
    ftd_dev_info_t        *devp;
    char pname[8];
    int   pcnt,pid,j,dev_cnt;
    /* add for diff check */
    char old[260], new[260];
    diffent_t *diff;
    diffdev_t *cur,*del_cur;
    int drv_mode;


    /*-------------------*/
    /* open the cfg file */
    /*-------------------*/
    sprintf(cfg_path, "%s%03d.cfg", PMD_CFG_PREFIX, group);
    sprintf(full_cfg_path, "%s/%s", PATH_CONFIG, cfg_path);
    sprintf(cur_cfg_path, "%s%03d.cur", PMD_CFG_PREFIX, group);
    sprintf(full_cur_cfg_path, "%s/%s", PATH_CONFIG, cur_cfg_path);
    if ((ret = readconfig(1, 0, 0, cfg_path)) < 0) 
    {
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, cfg_path);
        return -1 ;
    }
    /*--------------------------------------*/
    /* differnce check of cfg and cur       */
    /*--------------------------------------*/
    sprintf(old, "p%03d.cur", group);
    sprintf(new, "p%03d.cfg", group);
    diff = diffconfig(old,new);
    if (diff == NULL )
    {
        return -1 ;
    }
    if ( diff->status != 1 ) 
    {
        reporterr(ERRFAC, M_CFGWRONG, ERRCRIT, new );
        return -1;
    }
    /*---------------------------*/
    /* Get Pstore header         */
    /*---------------------------*/
    /* create the group name. group(INT) val to group_namr(CHAR) */
    FTD_CREATE_GROUP_NAME(group_name, group);

    if (GETPSNAME(group, ps_name) != 0) 
    {
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, cur_cfg_path);
        return -1 ;
    }
    /* get Pstore header */
    ret = ps_get_version_1_attr(ps_name, &attr, 1);
    if (ret != PS_OK) 
    {
        reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
        return -1 ; 
    }
    dev_cnt = 0;
    for ( cur = diff->h_add; cur != NULL; cur = cur->n ) {
        dev_cnt++;
    }

    /*-------------------------*/
    /* check current LG status */
    /*-------------------------*/
    /* get LG state from Driver */
    drv_mode = get_driver_mode(group);
    if (( drv_mode != FTD_MODE_TRACKING ) && ( drv_mode != FTD_MODE_NORMAL ))
    {
        reporterr(ERRFAC, M_NOTNORMAL, ERRCRIT,group );
        return -1;
    } 
    /*-----------------*/
    /* Stop PMD/RMD    */
    /*-----------------*/
    sprintf(pname,"PMD_%03d",group);

    if ((pid = getprocessid(pname, 1, &pcnt)) > 0)
    {
        /* first time SIGTERM and wait 30 sec.            */
        /* PMD may not stop in 7 seconds.                 */
        /* If it was 30 seconds, PMD stopped.             */
        /* Please change the number of seconds if needed. */

        kill(pid, SIGTERM);
        for (j = 0; j < 30; j++) 
        {
            if (getprocessid(pname, 1, &pcnt) == 0) 
            {
                 break;
            }
            sleep(1);
        }
        /* if SIGTERM not effect then SIGKILL */
        if (j == 30) 
        {
            kill(pid, SIGKILL);
            sleep(1);
        }
        if (getprocessid(pname, 1, &pcnt) > 0) 
        {
            reporterr(ERRFAC, M_CNTSTOP, ERRCRIT,"PMD",pid ); 
            return -1;
        }
    }
    else    /* PMD was already stoped. */
    {
        /* if driver mode is TRACKING then PMD can not alive */
        /* so when TRACKING except check PMD */
        if ( drv_mode != FTD_MODE_TRACKING ) 
        {
            reporterr(ERRFAC, M_PMDSTOPED, ERRCRIT,group_name );
            return -1;
        }
    }
    /*--------------------------*/
    /* Set LG state to Tracking */
    /*--------------------------*/
    /* Open CTL device */
    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0) 
    {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return -1;
    }
    /* Set LG state to Tracking whith out BAB and bit operation. */
    st.lg_num = group ;
    if(FTD_IOCTL_CALL(ctlfd, FTD_SET_MODE_TRACKING, &st) < 0) 
    {
        reporterr(ERRFAC, M_SETTRACKING, ERRCRIT,group);
        return -1;            
    }
    /*-----------------------------------------*/
    /* Adding new divece information to Pstore */
    /*-----------------------------------------*/
    /* open the driver and adding the new devices! */
    devp = (ftd_dev_info_t*)malloc(FTD_MAX_DEVICES*sizeof(ftd_dev_info_t));

    /*-----------------------------------------*/
    /* Adding new devices to the driver        */
    /* loop all new devices                    */
    /*-----------------------------------------*/
    for ( cur = diff->h_add; cur != NULL; cur = cur->n ) 
    {
        if (add_new_dev(ps_name, cur, group_name, group, devp) < 0) 
        {
            free(devp);
            return -1;
        }
    }
    free(devp);

    if(mysys->group->capture_io_of_source_devices)
    {
        reporterr(ERRFAC, M_CAPTSRCDEVSIO, ERRINFO, group);
    }
    
    /* now copy the .cfg file for this group to .cur */
    if ((infile = open (full_cfg_path, O_RDONLY, 0)) != -1) 
    {
        if ((outfile = open (full_cur_cfg_path, O_RDWR)) != -1) 
        {
            while (( n = read(infile, copybuf, BUFSIZ)) > 0) 
            {
                if (write (outfile, copybuf, n) != n) 
                { 
                    reporterr(ERRFAC, M_CURCPYERR, ERRCRIT,outfile);
                    close (infile);
                    close (outfile);
                    return -1;
                }
            }
            fchmod(outfile, S_IRUSR | S_IRGRP);
            close (outfile);
        } 
        else 
        {
            reporterr(ERRFAC, M_CURCPYERR, ERRCRIT,outfile);
            close (infile);
            return -1;
        }
        close (infile);
    }
    else
    {
        reporterr(ERRFAC,M_CFGFILE,ERRCRIT,cfg_path,full_cfg_path, strerror(errno));
        return -1;
    }
    /*-----------------*/
    /* Stop throtd     */
    /*-----------------*/
    if ((pid = getprocessid("throtd", 1, &pcnt)) > 0) 
    {
        kill(pid, SIGKILL);
    }
    /* all process compleated */
    return (0);
}

static int
add_new_dev(char *ps_name, diffdev_t *diffdev, char *group_name,
            int group, ftd_dev_info_t *devp)
{
    stat_buffer_t       sbuf;
    ps_version_1_attr_t attr;
    ps_dev_info_t       dev_info;
    int                 ctlfd, lgfd, index, ret;
    ftd_dev_info_t      info;
    struct stat         sb, sd;
    dev_t               cdev, bdev;
    char                *buffer;
    int                 *lrdb, *hrdb;
    unsigned int        hrdb_bits, lrdb_bits;
    dirtybit_array_t    dbarray;
    char                raw[MAXPATHLEN], block[MAXPATHLEN], 
                        raw_name[MAXPATHLEN],block_name[MAXPATHLEN];
    char		*devname = diffdev->devname;
    char		*mirname = diffdev->mirname;
    char		*sddevname = diffdev->sddevname;
#if defined(HPUX)
    ftd_devnum_t        dev_num;
#endif

    /* make sure the device exists */
    if (stat(devname, &sb)) 
    {
        reporterr(ERRFAC, M_STAT, ERRCRIT, devname, strerror(errno));
        return -1;
    }
   
    strcpy(raw_name,devname);
    force_dsk_or_rdsk(block_name,raw_name,0);

    if (stat(block_name, &sd) != 0 ){
       reporterr(ERRFAC, M_STAT, ERRCRIT, block_name, strerror(errno));
       return -1;
    }

    /* get the header to find out the max size of the dirty bitmaps */
    ret = ps_get_version_1_attr(ps_name, &attr, 1);
    if (ret != PS_OK) 
    {
        /* we're hosed. */
        reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
        return -1;
    }

    if ((buffer = (char *)calloc(attr.dev_attr_size, 1)) == NULL) 
    {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.dev_attr_size);
        return -1;
    }
    /* create the device names */
    strcpy(raw, sddevname);
    force_dsk_or_rdsk(block, raw, 0);

    /* check a device alrady exist or not.( buffre is dummy ) */
    ret = ps_get_device_attr(ps_name, raw, buffer, attr.dev_attr_size);
    if ((ret == PS_OK) || (ret == PS_DEVICE_NOT_FOUND))
    {
        /* if attributes don't exist, add the device to the persistent store */
        dev_info.name = raw;

        /* FIXME: how do we override/calculate these values? */
        dev_info.info_allocated_lrdb_bits = attr.lrdb_size * 8;
        dev_info.info_valid_lrdb_bits = attr.lrdb_size * 8;
        dev_info.info_allocated_hrdb_bits = attr.Small_or_Large_hrdb_size * 8;
        dev_info.info_valid_hrdb_bits = attr.Small_or_Large_hrdb_size * 8;
        ret = ps_add_device(ps_name, &dev_info);
        if (ret != PS_OK) 
        {
            /* we're hosed */
            reporterr(ERRFAC, M_PSADDDEV, ERRCRIT,sddevname,ps_name );
            free(buffer);
            return -1;
        }
        /* set up default LRDB and HRDB */
        if ((lrdb = (int *)malloc(attr.lrdb_size)) == NULL) 
        {
            free(buffer);
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.lrdb_size);
            return -1;
        }
        if ((hrdb = (int *)malloc(attr.Small_or_Large_hrdb_size)) == NULL) 
        {
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.Small_or_Large_hrdb_size);
            free(lrdb);
            free(buffer);
            return -1;
        }
        memset((caddr_t)lrdb, 0xff, attr.lrdb_size);
        memset((caddr_t)hrdb, 0xff, attr.Small_or_Large_hrdb_size);
        ret = ps_set_lrdb(ps_name, raw, (caddr_t)lrdb, attr.lrdb_size);
        if (ret != PS_OK) 
        {
            /* we're hosed */
            ps_delete_device(ps_name, raw);
            reporterr(ERRFAC, M_PSWTLRDB, ERRCRIT, raw,ps_name );
            free(lrdb);
            free(hrdb);
            free(buffer);
            return -1;
        }
        ret = ps_set_hrdb(ps_name, raw, (caddr_t)hrdb, attr.Small_or_Large_hrdb_size);
        if (ret != PS_OK) 
        {
            /* we're hosed */
            ps_delete_device(ps_name, raw);
            reporterr(ERRFAC, M_PSWTHRDB, ERRCRIT, raw, ps_name, ret, ps_get_pstore_error_string(ret));
            free(lrdb);
            free(hrdb);
            free(buffer);
            return -1;
        }
    }
    else 
    {
        /* other error. we're hosed. */
        reporterr(ERRFAC, M_PSADDDEV, ERRCRIT, raw,ps_name );
        free(buffer);
        return -1;
    }
    /*--------------------------------*/
    /*  add new devices to driver     */
    /*--------------------------------*/
    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0) 
    {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
        free(lrdb);
        free(hrdb);
        free(buffer);
        return -1;
    }
 
#ifdef _AIX
    if (diffdev->dd_minor >= 0) {
	index = diffdev->dd_minor;
    } else if (diffdev->md_minor >= 0) {
	index = diffdev->md_minor;
    } else {
	index = -1;
    }
    int minor_index = index;
#else
    index = -1;
#endif

    /* new code begin- for multiple pstores ... */
    if(FTD_IOCTL_CALL(ctlfd, FTD_CTL_ALLOC_MINOR, &index) < 0) 
    {
        reporterr(ERRFAC, M_GETMINOR, ERRCRIT);
        free(lrdb);
        free(hrdb);
        free(buffer);
        return -1;
    } 
    /* new code ends..... */

#if defined(_AIX)
    if (minor_index >= 0 && minor_index != index) {
	reporterr(ERRFAC, M_MINORMISMATCH, ERRWARN, sddevname, minor_index,
		  mirname, index);
    }
#endif

    lgfd = open(group_name, O_RDWR);
    if (lgfd < 0) 
    {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, group_name, strerror(errno));
        free(lrdb);
        free(hrdb);
        free(buffer);
        close(ctlfd);
        return -1;
    }

#if defined(HPUX)

    if (FTD_IOCTL_CALL(ctlfd, FTD_GET_DEVICE_NUMS, &dev_num) != 0) 
    {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't get device numbers", 
                strerror(errno));
        free(lrdb);
        free(hrdb);
        free(buffer);
        close(lgfd);
        close(ctlfd);
        return -1;
    }

    cdev = makedev(dev_num.c_major, index);
    bdev = makedev(dev_num.b_major, index);

#elif defined(SOLARIS) || defined(_AIX) || defined(linux)
    {
        struct stat csb;
        fstat(ctlfd, &csb);
        cdev = makedev(major(csb.st_rdev), index);
        bdev = cdev;
    }
#endif
    info.lgnum = group;
    info.cdev = cdev;
    info.bdev = bdev;
    info.localcdev = sb.st_rdev;
    info.disksize = disksize(devname);
    if (info.disksize == -1)
    {
      reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't get disk size", strerror(errno));
      free(lrdb);
      free(hrdb);
      free(buffer);
      close(lgfd);
      close(ctlfd);
      return -1;
    }
    info.lrdbsize32 = attr.lrdb_size / 4; /* number of 32-bit words */
    info.hrdbsize32 = attr.Small_or_Large_hrdb_size / 4; /* number of 32-bit words */
    info.statsize = attr.dev_attr_size; 
    ps_get_lrdb_offset(ps_name, raw, &info.lrdb_offset);

	// <<< TODO: this module is unused, but if it becomes used, for Proportional HRDB, give the HRDB resolution to the driver in info.hrdb_res in the FTD_NEW_DEVICE ioctl.
	// SEE this whole section in dtcstart, + verification of value changes after ioctl, by driver <<<
    if (FTD_IOCTL_CALL(ctlfd, FTD_NEW_DEVICE, &info)) 
    {
        if (grp_started && errno == EADDRINUSE) 
        {
            /* do nothing */
        } 
        else 
        {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't add new device", 
                strerror(errno));
            free(lrdb);
            free(hrdb);
            free(buffer);
            close(lgfd);
            close(ctlfd);
            return -1;
        }
    }

#if defined(HPUX)
    ftd_make_devices(&sb, &sd, raw, block, cdev, bdev);
#else
    ftd_make_devices(&sb, &sd, raw, block, bdev); /* SS/IS activity */
#endif

    /* add the attributes to the driver */
    sbuf.lg_num = group; /* by luck: the minor number of the group */
    sbuf.dev_num = cdev; /* was: index */
    sbuf.len = attr.dev_attr_size;
    sbuf.addr = (ftd_uint64ptr_t)buffer;
    if (FTD_IOCTL_CALL(ctlfd, FTD_SET_DEV_STATE_BUFFER, &sbuf)) 
    {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set group stats", 
            strerror(errno));
        close(lgfd);
        close(ctlfd);
        free(lrdb);
        free(hrdb);
        free(buffer);
        return -1;
    }
    /* set the LRDB in the driver */
    dbarray.numdevs = 1;
    {
        dev_t tempdev[2];
        ftd_dev_info_t devinfo;
        tempdev[0] = cdev;
        tempdev[1] = 0x12345678;
        dbarray.devs = (dev_t *)&tempdev[0];
        dbarray.dblen32 = info.lrdbsize32;
        dbarray.dbbuf = (ftd_uint64ptr_t)lrdb;
        devinfo.lrdbsize32 = dbarray.dblen32;
        if (dbarr_ioc(lgfd, FTD_SET_LRDBS, &dbarray, &devinfo))
        {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set lrdb", 
                strerror(errno));
            close(lgfd);
            close(ctlfd);
            free(lrdb);
            free(hrdb);
            free(buffer);
            return -1;
        }
    }

    /* set the HRDB in the driver */
    dbarray.numdevs = 1;
    {
        dev_t tempdev=cdev;
        ftd_dev_info_t devinfo;
        dbarray.devs = (dev_t *)&tempdev;
        dbarray.dblen32 = info.hrdbsize32;
        dbarray.dbbuf = (ftd_uint64ptr_t)hrdb;
        devinfo.hrdbsize32 = dbarray.dblen32;
        if (dbarr_ioc(lgfd, FTD_SET_HRDBS, &dbarray, &devinfo)) // <<< OK Prop HRDB
        {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set hrdb", 
                strerror(errno));
            close(lgfd);
            close(ctlfd);
            free(lrdb);
            free(hrdb);
            free(buffer);
            return -1;
        }
    }

    if(mysys->group->capture_io_of_source_devices)
    {
        ftd_dev_t_t new_device = bdev;
        
        if (FTD_IOCTL_CALL(ctlfd, FTD_CAPTURE_SOURCE_DEVICE_IO, &new_device) != 0) {
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, "FTD_CAPTURE_SOURCE_DEVICE_IO", strerror(errno));
			return -1;
		}   
    }
    
    free(lrdb);
    free(hrdb);
    free(buffer);
    close(ctlfd);
    close(lgfd);
    return 0;
}
/***********************************************************************/
/*  Add ftd_reconfig for Non disruptive adding volume                  */
/***********************************************************************/
int 
main(int argc, char *argv[])
{
    int group, gflag;
    int ch;

    int exitcode;
#if defined(HPUX)
    struct stat statbuf; 
    char cmd[128];
#endif

    putenv("LANG=C");

    /* Make sure we are root */
    if (geteuid()) 
    {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }
    
    progname = argv[0];
    if (argc < 2) 
    {
        goto usage_error;
    }
    argv0 = argv[0];
    group = -1;
        
    setmaxfiles();
    /* process all of the command line options */
    opterr = gflag = 0;
    while ((ch = getopt(argc, argv, "g:")) != EOF) 
    {
        switch(ch) 
        {
        case 'g':  
            if (gflag)
            {
                fprintf(stderr, "-g options are multiple specified\n");
                goto usage_error;
            }
            group = ftd_strtol(optarg);
            if (group < 0 || group >= MAXLG)
            {
                fprintf(stderr, "Invalid number for " GROUPNAME " group\n");
                goto usage_error;
            }
            gflag++;
            break;
        default:
            goto usage_error;
        }
    }
    if (gflag != 1 || optind != argc ) 
    {
        goto usage_error;
    }
    if (initerrmgt(ERRFAC) != 0) 
    {
       /* NO need to exit here because it causes HP-UX */
       /* replication devices' startup at boot-time to fail */
       /* PRN# 498       */
    }
    log_command(argc, argv);   /* trace command in dtcerror.log */

    exitcode = 0;
    if (modify_group(group)!=0) 
    {
        reporterr(ERRFAC, M_RECNFERR, ERRCRIT, group);
        exitcode = -1;
    } 
    else
    {
        printf ("Adding new divece is compleated. Please launchrefresh.\n");
    }
    exit(exitcode);
    return exitcode; /* for stupid compiler */
usage_error:
    usage();
    exit(1);
    return 0; /* for stupid compiler */
}

/* The Following 3 lines of code are intended to be a hack 
   only for HP-UX 11.0 . 
   HP-UX 11.0 compile complains about not finding the below
   function when they are staticaly linked - also we DON'T want
   to link it dynamically. So for the time being - ignore these
   functions while linking. */ 
#if defined(HPUX) && (SYSVERS >= 1100)
  shl_load () {}
  shl_unload () {}
  shl_findsym () {}
#endif
