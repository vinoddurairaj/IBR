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
#include "ftdio.h"
#include "config.h"
#include "errors.h"
#include "devcheck.h"
#include "aixcmn.h"
#include "common.h"
#include "pathnames.h"

static char *progname = NULL;
char *argv0;

static void
usage(void)
{
    fprintf(stderr, "Usage: %s -g <group#> -d <device path>\n", progname);
    fprintf(stderr, "\t<group#> is " GROUPNAME " group number. (0 - %d)\n", MAXLG-1);
    fprintf(stderr, "\t<device path> is the path of the replicated device. The path can be the source device name or the virtual dtc device name in the form /dev/"QNM"/lg#/rdsk/"QNM"#\n");
}

#define KILLPMDWAIT 30
#define KEY_DEVICE      CAPQ"-DEVICE:"
#define KEY_SIZE        "DISK-LIMIT-SIZE:"
#define PROFILE         "PROFILE:"
/***********************************************************************/
/*  Add ftd_expanding for Non disruptive adding volume                  */
/***********************************************************************/
ftd_uint64_t
get_max_size(int lgnum, dev_t rdev)
{
    ftd_dev_info_t info;
    int ctlfd;

    /*---------------------*/
    /* Open DTC CTL device */
    /*---------------------*/
    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0)
    {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return -1;
    }

    info.lgnum = lgnum;              /* set lg number */
    info.cdev = rdev;          /* set destination device id */
    if(FTD_IOCTL_CALL(ctlfd, FTD_GET_MAX_DEVICE_SIZE, &info) < 0)
    {
        reporterr(ERRFAC, M_GETDEVSIZ, ERRCRIT);
        close(ctlfd);
        return 0;
    }
    close(ctlfd);

    return (info.maxdisksize);
}
/******************************/
/* Get DISK-LIMIT-SIZE        */
/******************************/
static ftd_uint64_t
get_limitsize_multiple(int group, char *devname)
{
    char       cur_file[MAXPATHLEN];
    char       raw[MAXPATHLEN];
    sddisk_t   *temp;

    /* make cur file name */
    sprintf(cur_file, "%s%03d.cur", PMD_CFG_PREFIX, group);
    /* get raw device name and length */
    force_dsk_or_rdsk(raw,devname,1);

    if (readconfig(1, 0, 0, cur_file) < 0)
    {
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, cur_file);
        return 0;
    }
    for (temp = mysys->group->headsddisk; temp != NULL; temp = temp->n)
    {
        if(strcmp(temp->sddevname, raw) == 0){
            if (temp->limitsize_multiple == 0) 
                return (DEFAULT_LIMITSIZE_MULTIPLE);
            else 
                return temp->limitsize_multiple;
        }
    }
    return 0;

}

/* kill_process
 * 
 * Returns 0 if process not found
 *         1 if process killed
 *        -1 if process can't be killed
 */
int
kill_process(char *pname)
{
    int pcnt,pid,j;

    if ((pid = getprocessid(pname, 1, &pcnt)) > 0)
    {
        /* first time SIGTERM and wait 30 sec.            */
        /* Please change the number of seconds if needed. */

        kill(pid, SIGTERM);
        for (j = 0; j < KILLPMDWAIT; j++)
        {
            if (getprocessid(pname, 1, &pcnt) == 0)
            {
                 break;
            }
            sleep(1);
        }
        /* if SIGTERM not effect then SIGKILL */
        if (j >= KILLPMDWAIT)
        {
            kill(pid, SIGKILL);
            sleep(1);
        }
        if (getprocessid(pname, 1, &pcnt) > 0)
        {
            return -1;
        } else {
            return 1;
        }
    } else {
        return 0;
    }  
}

/************************/
/* Modify device size.  */
/************************/
int
mod_dev_size(int lgnum,char *dev_name,dev_t st_rdev,ftd_uint64_t new_dev_size)
{
    char group_name[MAXPATHLEN];
    char ps_name[MAXPATHLEN];
    char pname[8];
    int pcnt,pid,j,ctlfd;
    int drv_mode;
    ftd_state_t st;
    ftd_dev_info_t info;
    char ans[10];
    int rc;
	unsigned long long limitsize_factor_from_pstore;

    // Verify that we will not exceed the expansion provision that was done for this device	(PROD9517)
	rc = ps_verify_expansion_provision( mysys->group->pstore, dev_name, new_dev_size, &limitsize_factor_from_pstore );
	if( rc != PS_OK )
	{
        reporterr(ERRFAC, M_DEVEXP_ERROR, ERRCRIT, ps_get_pstore_error_string(rc) );
		if( rc == PS_DEV_EXPANSION_EXCEEDED )
		{
            reporterr(ERRFAC, M_DEVEXP_ERR2, ERRINFO );
            reporterr(ERRFAC, M_DEVEXP_ERR3, ERRINFO );
		}
        return -1;
	}
	// Check if device expansion has been turned off (factor set to 1)
	if( limitsize_factor_from_pstore <= 1 )
	{
        reporterr( ERRFAC, M_DEVEXP_OFF, ERRINFO, dev_name, limitsize_factor_from_pstore );
        return -1;
	}

    /*-------------------------*/
    /* check current LG status */
    /*-------------------------*/
    /* get LG state from Driver */
    drv_mode = get_driver_mode(lgnum);
    if (( drv_mode != FTD_MODE_TRACKING ) && ( drv_mode != FTD_MODE_NORMAL )
          && (drv_mode != FTD_MODE_PASSTHRU))
    {
        reporterr(ERRFAC, M_ISREFRESH, ERRCRIT,lgnum );
        return -1;
    }
    /*-----------------*/
    /* Stop PMD/RMD    */
    /*-----------------*/
    sprintf(pname,"PMD_%03d",lgnum);
 
    rc = kill_process(pname);
    if (!rc) {
        /* PMD was already stopped */
        /* if driver mode is TRACKING then PMD can not alive */
        /* so when TRACKING except check PMD */
        if ( (drv_mode != FTD_MODE_TRACKING) && (drv_mode !=FTD_MODE_PASSTHRU) )
        {
            reporterr(ERRFAC, M_PMDSTOPED, ERRCRIT);
            return -1;
        } 
    } else if (rc == -1) { 
        reporterr(ERRFAC, M_CNTSTOP, ERRCRIT,"PMD",pid );
        return (-1);
    }


    /*---------------------*/
    /* Open DTC CTL device */
    /*---------------------*/
    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0)
    {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return -1;
    }
    /*--------------------------*/
    /* Set LG state to Tracking */
    /*--------------------------*/
    /* Set LG state to Tracking whith out BAB and bit operation. */
    st.lg_num = lgnum;
    if(FTD_IOCTL_CALL(ctlfd, FTD_SET_MODE_TRACKING, &st) < 0)
    {
        reporterr(ERRFAC, M_SETTRACKING, ERRCRIT,lgnum);
        close(ctlfd);
        return -1;
    }
    /*---------------------------------*/
    /* Expanding dtc device.           */
    /*---------------------------------*/
    info.lgnum = lgnum;              /* set lg number */
    info.cdev = st_rdev;          /* set destination device id */
    info.disksize = new_dev_size;    /* set new device size */
    if(FTD_IOCTL_CALL(ctlfd, FTD_SET_DEVICE_SIZE, &info) < 0) 
    {
        reporterr(ERRFAC, M_SETDEVSIZ, ERRCRIT, dev_name);
        close(ctlfd);
        return -1;
    }
    close(ctlfd);

    // Update this device's num_sectors in the Pstore (PROD9517)
	rc = ps_update_num_sectors( mysys->group->pstore, dev_name, new_dev_size );
	if( rc != PS_OK )
	{
        reporterr(ERRFAC, M_DEVEXP_SAVERR, ERRWARN, ps_get_pstore_error_string(rc) );
        reporterr(ERRFAC, M_DEVEXP_SAVERR, ERRINFO );
	}

    return 0;
}

int get_device_name(int group, char *dev_name, char *local_dev_name, char *dtc_dev_name, int do_read_config)
{
   
    char       cur_file[MAXPATHLEN];
    char       raw[MAXPATHLEN];
    sddisk_t   *temp;

    /* get raw device name and length */
    force_dsk_or_rdsk(raw,dev_name,1);

    if( do_read_config )
	{
        /* make cur file name */
        sprintf(cur_file, "%s%03d.cur", PMD_CFG_PREFIX, group);

        if (readconfig(1, 0, 0, cur_file) < 0)
        {
            reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, cur_file);
            return 0;
        }
	}
    for (temp = mysys->group->headsddisk; temp != NULL; temp = temp->n)
    {
        if( (strcmp(temp->sddevname, raw) == 0) ||
            (strcmp(temp->devname, raw) == 0) )
        {
           strcpy(local_dev_name, temp->devname);
           strcpy(dtc_dev_name, temp->sddevname);
           break;
        }
    }
    if (temp == NULL) {
        return (-1);
    } else {
        return 0;
    }
}
int modify_device(char *dev_name, int lgnum, int do_read_config)
{
    ftd_dev_t_t cdev;
    ftd_uint64_t new_dev_size,max_dev_size;
    struct stat sb;
    ftd_dev_info_t info;
    char local_dev_name[MAXPATHLEN];
    char dtc_dev_name[MAXPATHLEN];
 
    /*-----------------------------------*/
    /* Get the device stat and dev id    */
    /*-----------------------------------*/
    /* get local data device name from dtc dev name */
    if (get_device_name(lgnum, dev_name, local_dev_name, dtc_dev_name, do_read_config) < 0 ) 
    {
       fprintf(stderr, "get_device_name() failed.\n");
       return (-1);
    }

    if ( stat( dtc_dev_name,&sb ) == -1 )            /* get device information */
    {
        reporterr(ERRFAC, M_STAT, ERRWARN, dev_name, strerror(errno));
        return (-1);
    }
    max_dev_size = get_max_size(lgnum, sb.st_rdev);


    if ((new_dev_size = disksize(local_dev_name)) <= 0 )
    {
       fprintf(stderr,"Get disksize error for %s\n",local_dev_name );
       return (-1);
    }
    if ( max_dev_size < new_dev_size )
    {
        fprintf(stderr,"Required disk size (%lld) is larger than maximum for this device (%lld)\n",new_dev_size,max_dev_size );
        return (-1);
    }
    /*-----------------------*/
    /* expanding device size */
    /*-----------------------*/
    if( mod_dev_size(lgnum,dtc_dev_name, sb.st_rdev, new_dev_size) < 0 )
    {
        return(-1);
    }
    return (0);
}
int 
main(int argc, char *argv[])
{
    char **setargv;
    int  lgnum;
    int gflag,dflag;
    char dev_name[MAXPATHLEN];
    char cfg_path[MAXPATHLEN];
    sddisk_t         *temp;
    int i;

    int ch;

    putenv("LANG=C");

    /*---------------------------------*/
    /* Prepare for command             */
    /*---------------------------------*/
    /* Make sure we are root */
    if (geteuid())
    {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }

    progname = argv[0];
    argv0 = argv[0];
    if (initerrmgt(ERRFAC) != 0)
    {
       /* NO need to exit here because it causes HP-UX */
       /* replication devices' startup at boot-time to fail */
       /* PRN# 498       */
    }
    log_command(argc, argv);   /* trace command in dtcerror.log */

    /*---------------------------------*/
    /* Get parmater and check          */
    /*---------------------------------*/
    lgnum = -1;
    gflag = dflag = 0;
    while ((ch = getopt(argc, argv, "g:d:")) != EOF)
    {
        switch(ch)
        {
        case 'g':
            if (gflag) 
            {
                fprintf(stderr, "multiple -g options specified\n");
                goto usage_error;
            }
            lgnum = ftd_strtol(optarg);
            if (lgnum < 0 || lgnum >= MAXLG) 
            {
                fprintf(stderr, "Invalid number for " GROUPNAME " group\n");
                goto usage_error;
            }
            gflag++;
            break;
        case 'd':
            if (dflag) 
            {
                fprintf(stderr, "multiple -d options specified\n");
                goto usage_error;
            }
            strcpy (dev_name,optarg);
            dflag++;
            break;
        default:
            goto usage_error;
        }
    }
    /*--------------------------*/
    /* Check command argument   */
    /*--------------------------*/
    if ( gflag != 1 || optind != argc )
    {
        goto usage_error;
    }
   
    if (!dflag) 
    {
        sprintf(cfg_path, "%s%03d.cur", PMD_CFG_PREFIX, lgnum);
        if (readconfig(1, 0, 0, cfg_path) < 0) 
        {
            reporterr(ERRFAC, M_CFGFILE, ERRCRIT, argv0, cfg_path,
				strerror(errno));
            return -1;
        }  
        /* loop thru the list of devices in the configuration file */
        for (temp = mysys->group->headsddisk, i=0; temp != NULL; temp = temp->n, i++) 
		{
            if (modify_device(temp->sddevname, lgnum, 0)< 0) 
            {	
		        exit(1);
	        }
        }
    } 
    else
    {
        if (modify_device(dev_name, lgnum, 1) < 0)
        {
	        exit(1);
	    }
    }

    kill_process("throtd");

    fprintf(stderr,"Device expansion completed. You will need to run launchrefresh for the group.\n");

    return 0; /* for stupid compiler */
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
