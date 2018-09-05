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
 * panalyze.c - 
 *
 * This program will return the amount of journal size required
 * to be able to do a smart-refresh from the source to the target
 *
 * NOTE: this is an absolute minimum! It does not take into consideration
 * any amount of data transfers between the primary and the secondary
 * while doing the refresh.
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

#define PANALYZE_DEBUG 0

#if defined(HPUX)
#define divide_factor	1ULL
#else
#define divide_factor	2ULL
#endif


static char       *prog_name;
static int        verbose = FALSE;
static int        print_smart_refresh_remaining = FALSE;
static char       *argv0;
static int        targets[MAXLG];
static char       configpaths[MAXLG][32];
extern char       *paths;
extern int        dbarr_ioc(int fd, int ioc, dirtybit_array_t *db, ftd_dev_info_t *dev);

static int      for_system_call = 0;
static int      for_system_call_group_number = 0;
static int      pending_MBs = 0;

/*
 * usage -- This is how the program is used
 */
void
usage(void)
{
    fprintf(stderr,"Usage: %s <options>\n", prog_name);
    fprintf(stderr,"\t-g <group_num>\t: group number to analyze\n");
    fprintf(stderr,"\t-a            \t: analyze all started groups (the groups must be started)\n"); 
    fprintf(stderr,"\t-v            \t: verbose\n");
    fprintf(stderr,"\t-r            \t: prints the amount of data remaining to be transferred during a smart refresh\n");
    exit(1);
}

/*
 * proc_argv -- process argument vector
 */
void
proc_argv(int argc, char **argv)
{
    int             ch;

    long            lgnum = 0;
    int             gflag = 0;      /* -g option flag */
    int             aflag = 0;      /* -a option flag */
    int             i     = 0;
    int             sflag = 0;

    argv0 = strdup(argv[0]);
    /* At least one argument is required */
    if (argc == 1) {
        usage();
    }
    opterr = 0;
    while ((ch = getopt(argc, argv, "vag:rs")) != EOF) {
        switch (ch) {
           case 'g':
               if (gflag) {
                  fprintf(stderr, "-g options are multiple specified\n");
                  usage();
               }
               if (aflag) {
                  fprintf(stderr, "-a option cannot be specified with -g option\n");
                  usage();
               }
               lgnum = ftd_strtol(optarg);
               if (lgnum < 0 || lgnum >= MAXLG) {
                  fprintf(stderr, "Invalid number for " GROUPNAME " group\n");
                  usage();
               }
               targets[lgnum] = 1;
               gflag++;
               break;
           case 'a':
               if (gflag) {
                  fprintf(stderr, "-a option cannot be specified with -g option\n");
                  usage();
               }
               for (i = 0; i < MAXLG; i++)
                   targets[i] = 1;
               aflag++;
               break;    
           case 'v':
               verbose = TRUE;
               break;
           case 'r':
               print_smart_refresh_remaining = TRUE;
               break;
           // WI_338550 December 2017, implementing RPO / RTT
           case 's':
               sflag = TRUE;
               for_system_call = TRUE;
               break;
           default:
               usage();
               break;
        }
    }
    if ((gflag == 0) && (aflag == 0)){
        fprintf(stderr, "-g/-a option is not specified. It is mandatory\n");
        usage();
    }
    if ((sflag != 0) && (gflag == 0)){
        fprintf(stderr, "-s option cannot be specified without -g. It is mandatory\n");
        exit(1);
    }
    // Take note of the group number if this is a system call, it will be used in the output file name
    if( for_system_call )
    {
        for_system_call_group_number = lgnum;
    }
    
    return;
}

char*
format_drive_size(ftd_uint64_t uiSize)
{
	static char szSize[64];

	if (uiSize < 1024ULL)
	{
	   snprintf(szSize, 64, "%lld KB", uiSize);
	}
	else if (uiSize < (1024ULL*1024ULL))
	{
	   snprintf(szSize, 64, "%.1f MB ", (double)uiSize/1024ULL);
	}
	else if (uiSize < (1024ULL*1024ULL*1024ULL))
	{
	   snprintf(szSize, 64, "%.1f GB ", (double)uiSize/(1024ULL*1024ULL));
	}
	else
	{
	   snprintf(szSize, 64, "%.2f TB ", (double)uiSize/(1024ULL*1024ULL*1024ULL));
	}
	return szSize;
}

int
count_bits(unsigned char dirty_byte)
{
    dirty_byte = (dirty_byte & 0x55u) + ((dirty_byte >> 1) & 0x55u);
    dirty_byte = (dirty_byte & 0x33u) + ((dirty_byte >> 2) & 0x33u);
    dirty_byte = (dirty_byte & 0x0fu) + ((dirty_byte >> 4) & 0x0fu);
    return (int)dirty_byte;
}

/**
 * Obtains the device specific state buffer.
 *
 * This is actually just a wrapper around the GET_DEV_STATE_BUFFER ioctl.
 *
 * @param cfltd                    File handle of the master control device.
 * @param lgnum                    The number of the group in which the device is.
 * @param cdev                     The (char dtc) device for which we are requesting statistics for.
 * @param device_state_buffer      A pointer to the memory which should receive the device state buffer.
 * @param device_state_buffer_size The size of the device_state_buffer.
 *
 * @return Standard errno codes, as returned from FTD_IOCTL_CALL;
 */
static int get_device_state_buffer(int ctlfd, int lgnum,  ftd_dev_t_t cdev, char* device_state_buffer, int device_state_buffer_size)
{
    stat_buffer_t sb = {0};
    
    sb.lg_num = lgnum;
    sb.dev_num = (ftd_uint32_t)cdev;
    sb.len = device_state_buffer_size;
    sb.addr = (ftd_uint64ptr_t)(unsigned long)device_state_buffer;
    
    return FTD_IOCTL_CALL(ctlfd, FTD_GET_DEV_STATE_BUFFER, &sb);
}

static int
verify_journal_size(int lgnum)
{
    int                 lgfd, ctlfd;
    unsigned int        i, dev_offset = 0;
    int                 rc;
    int                 bhighres    = FALSE;
    int                 blowres     = FALSE;
    ftd_dev_info_t      *dip        = NULL;
    unsigned char       *hrdb       = NULL;
    unsigned char       *lrdb       = NULL;
    ps_version_1_attr_t group_attributes;
    char                group_name[MAXPATHLEN];
    char                dev_name[FTD_MAX_DEVICES][MAXPATHLEN];
    char                ps_name[MAXPATHLEN];
    dev_t               *tempdev    = NULL;
    unsigned int        LowResBitmapSize;
    dirtybit_array_t    dbHighResBitmap;
    dirtybit_array_t    dbLowResBitmap;
    stat_buffer_t       sb;
    ftd_stat_t          lgstat;
    unsigned int        LRDBoffset32        = 0;
    unsigned int        HRDBoffset32        = 0;
    ftd_uint64_t        uiMBrequiredLRDB    = 0;
    ftd_uint64_t        uiMBrequiredHRDB    = 0;
    ftd_uint64_t        uiMBRemainingToBeRefreshed = 0;
    char                tempDevName[MAXPATHLEN];
    unsigned int        totalHRDBbytes      = 0;
    char                cfg_path[MAXPATHLEN];
    sddisk_t            *sd = NULL;
    sddisk_t            *tempsd = NULL;

    /* Initialize all variables and structures */
    dbHighResBitmap.devs    = NULL;
    dbLowResBitmap.devs     = NULL;
    dbHighResBitmap.numdevs = 0;
    dbLowResBitmap.numdevs  = 0;
    dbHighResBitmap.state_change = 0;
    dbLowResBitmap.state_change  = 0;
    /* end initializations */

    /* read the config file */
    sprintf(cfg_path, "%s%03d.cur", PMD_CFG_PREFIX, lgnum);
    if (readconfig(1, 0, 0, cfg_path) < 0) {
        reporterr(ERRFAC, M_CFGFILE, ERRCRIT, "error", cfg_path, strerror(errno));
        return -1;
    }
    if ((sd = mysys->group->headsddisk) == NULL) {
       fprintf(stderr,"ERROR. No devices found in group %d\n", lgnum);
       return -1;
    } 

    if ((ctlfd = open(FTD_CTLDEV, O_RDONLY)) < 0) {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return -1;
    }

    /* get the name of the pstore */
    if (GETPSNAME(lgnum, ps_name) == -1) {
        fprintf(stderr,"ERROR. Couldn't get pstore name for group %d\n", lgnum);
        close(ctlfd);
        return -1;
    }

    if (ps_get_version_1_attr((char *)ps_name, &group_attributes, 1) != PS_OK) {
        close(ctlfd);
	      return -1;
    }

    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    /* get device name list */
    if ((rc = ps_get_device_list(ps_name, dev_name, FTD_MAX_DEVICES) != PS_OK)) {
        fprintf(stderr,"ps_get_device_list() failed\n");
        close(ctlfd);
        return -1;
    }

    /* open the logical group device */
    if ((lgfd = open(group_name, O_RDONLY)) < 0) {
        fprintf(stderr, "\nError:%d:Unable to open logical group device [%s].\n", errno, group_name);
        close(ctlfd);
        return -1;
    }

    if ((dip = (ftd_dev_info_t*)ftdmalloc(group_attributes.num_device*sizeof(ftd_dev_info_t))) == NULL) {
        fprintf(stderr,"Unable to allocate memory for device names\n");
        goto errret;
    }

    /* get group info */
    memset(&sb, 0, sizeof(stat_buffer_t));
    sb.lg_num = lgnum;
    sb.len = sizeof(ftd_stat_t);
    sb.addr = (ftd_uint64ptr_t)(unsigned long)&lgstat;
    if (FTD_IOCTL_CALL(ctlfd, FTD_GET_GROUP_STATS, &sb) != 0) {
       reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_GROUP_STATS", strerror(errno));
       goto errret;
    }

    if(print_smart_refresh_remaining && lgstat.state != FTD_MODE_REFRESH )
    {
        fprintf(stderr,"Cannot report data remaining to be refreshed when group %d is not in smart refresh.\n", lgnum);
        goto errret;
    }
    
    /* get devices info */
    memset(&sb, 0, sizeof(stat_buffer_t));
    sb.lg_num = lgnum;
    sb.addr = (ftd_uint64ptr_t)(unsigned long)dip;
    if (FTD_IOCTL_CALL(ctlfd, FTD_GET_DEVICES_INFO, &sb) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO", strerror(errno));
        goto errret;
    }

    /* Allocate space for low resolution dirty bits data (LRDB) */
    if ((lrdb = (unsigned char *)ftdmalloc(lgstat.ndevs*FTD_PS_LRDB_SIZE)) == NULL) {
        fprintf(stderr,"Unable to allocate memory for LRDB's\n");
        goto errret;
    }

    dbLowResBitmap.dbbuf = (ftd_uint64ptr_t)(unsigned long)lrdb;
    dbLowResBitmap.numdevs = lgstat.ndevs;

    tempdev = (dev_t*)ftdmalloc(lgstat.ndevs*sizeof(dev_t));
    memset(tempdev, 0, lgstat.ndevs*sizeof(dev_t));
    dbLowResBitmap.devs = (dev_t *)tempdev;
    for (i = 0; i < lgstat.ndevs; i++) {
        dbLowResBitmap.devs[i] = dip[i].cdev;
    }
      
    /* Get LRDB Array */
    if ((rc = dbarr_ioc(lgfd, FTD_GET_LRDBS, &dbLowResBitmap, dip)) < 0) {
        fprintf(stderr,"Unable to read the LRDB from the driver (rc = %d); will calculate only from HRDB\n", rc);
        blowres = FALSE;
    } else {
        blowres = TRUE;
    }

    for (i = 0; i < lgstat.ndevs; i++) {
        totalHRDBbytes +=  (dip[i].hrdbsize32 * 4);
    }
    /* Allocate space for high resolution dirty bits data (HRDB) */
    if ((hrdb = (unsigned char *)ftdmalloc(totalHRDBbytes)) == NULL) {
        fprintf(stderr,"Unable to allocate memory for HRDB's\n");
        goto errret;
    }

    dbHighResBitmap.dbbuf = (ftd_uint64ptr_t)(unsigned long)hrdb;
    dbHighResBitmap.numdevs = lgstat.ndevs;
    memset(tempdev, 0, lgstat.ndevs*sizeof(dev_t));
    dbHighResBitmap.devs = (dev_t *)tempdev;
    for (i = 0; i < lgstat.ndevs; i++) {
        dbHighResBitmap.devs[i] = dip[i].cdev;
    }

    /* Get HRDB Array */
    if ((rc = dbarr_ioc(lgfd, FTD_GET_HRDBS, &dbHighResBitmap, dip)) < 0) {
	    if( blowres )
            fprintf(stderr,"Unable to read the HRDB from the driver (rc = %d); will calculate only from LRDB\n", rc);
		else
            fprintf(stderr,"Unable to read the HRDB from the driver: rc = %d\n", rc);
        bhighres = FALSE;
    } else {
        bhighres = TRUE;
    }

    if (!blowres && !bhighres) {
        fprintf(stderr, "Target group not started/does not exist.\n");
        goto errret;
    }

    /* Check if the pstore is home to more than just this group */
    if (group_attributes.num_device > lgstat.ndevs ) {
       dev_offset = 0;
       snprintf(tempDevName, MAXPATHLEN, "/dev/dtc/lg%d/", lgnum);
       if (lgnum < 10) {
	        while (strncmp(tempDevName, &dev_name[dev_offset][0], 13) != 0)
	              dev_offset++;
	     }
	     else if (lgnum < 100) {
	        while (strncmp(tempDevName, &dev_name[dev_offset][0], 14) != 0)
	              dev_offset++;
	     }
	     else if (lgnum < 1000) {
	        while (strncmp(tempDevName, &dev_name[dev_offset][0], 15) != 0)
	              dev_offset++;
	     }
	     else {
          fprintf(stderr, "Device name not found\n");
          goto errret;
       }
    }

           
    /* Calculate needed journal space! */
    for (i = 0; i < lgstat.ndevs; i++)
    {
        // When calculating the HRDB and LRDB pointers, we need to cast the dbbuf to an int* so that the pointer arithmetic works correctly.
        // This is identical to what's done internally within dbarr_ioc().
        unsigned char          *HRDB     = (unsigned char *) ((int*)dbHighResBitmap.dbbuf + HRDBoffset32);
        unsigned char          *LRDB     = (unsigned char *) ((int*)dbLowResBitmap.dbbuf + LRDBoffset32);
        unsigned int               j     = 0, k;
        unsigned int           total     = 0;
        double           dPercentage     = 0.0;
        ftd_uint64_t   uiDiskRequired    = (dip[i].disksize / divide_factor) ;

        LowResBitmapSize = FTD_PS_LRDB_SIZE * 8;

        /* skip empty devices */
        if ((dip[i].hrdb_numbits==0) && (dip[i].lrdb_numbits==0)) { 
             continue;
        }

        if (verbose) {
            // Because the order of the device names stored within the pstore is reversed compared to what we obtain from the driver,
            // we need to obtain the name from the list of devices in the pstore using a reversed index.
            fprintf(stderr,"\nSize for device: %s Disk %s",
                    (char *)&dev_name[dev_offset + (lgstat.ndevs - 1 - i)][0],
                    format_drive_size(uiDiskRequired));
        }

        /* check % of bitmap is in use */
        /* use high resolution bitmaps if possible 
         * (don't use pstore if at all possible!) */
        if (bhighres) {
            total = 0;
            k = 0;
           /* Counting bits is just a matter of adding up each byte of the bitmap */
            for (j = 0; j < (unsigned int)(dip[i].hrdb_numbits/8); j++) {
                unsigned char g = HRDB[j];
#if PANALYZE_DEBUG
                /* This will print out the entire HRT map for the device */
                if (verbose) {
                   if (k == 0)
                      fprintf(stderr,"\nPrinting HRT map...");
                   if (k%16 == 0)
                      fprintf(stderr,"\n");
                   if (k%4 == 0)
                      fprintf(stderr,"\t0x");
                   
                   fprintf(stderr,"%02x", g);
                   k++;                    
                }
#endif /* PANALYZE_DEBUG */
                total += count_bits(g);
            }

            /* Get percentage of bits dirty */
            dPercentage   = (((double)total / (double)dip[i].hrdb_numbits) *
                             ((double)dip[i].maxdisksize / (double)dip[i].disksize));
            if (dPercentage > 1) /* This can happen when the group is in PASSTHRU mode */
               dPercentage = 1;

            uiDiskRequired = (unsigned int)((dip[i].disksize / divide_factor) * dPercentage);
            /* add a 10% fudge factor here, since we aren't accounting for 
             * headers or any potential inefficient allocation of 
             * file system space */
            uiDiskRequired += uiDiskRequired * 0.1; 

            if (verbose) 
            {
	            fprintf(stderr, "\nHRDB usage:\n");
			    if( dip[i].disksize != dip[i].maxdisksize )
				{
				    // Device expansion has not occurred to its allocated provision (PROD10347: clarify output)
	                fprintf(stderr, "    %d dirty bits out of %d bits total (including extra bits allocated for device expansion)\n",
	                                total, dip[i].hrdb_numbits);
	                fprintf(stderr, "    percentage of actual dirty data based on real device size: %2.3f%%\n", dPercentage*100);
				}
				else
				{
				    // Device expansion has occurred to its allocated provision
	                fprintf(stderr, "    %d dirty bits out of %d allocated bits\n",
	                                total, dip[i].hrdb_numbits);
	                fprintf(stderr, "    percentage of dirty data: %2.3f%%\n", dPercentage*100);
				}
                if (!print_smart_refresh_remaining)
                {
                    fprintf(stderr, "    disk size required is %s", format_drive_size(uiDiskRequired));
                }
            }

          if(print_smart_refresh_remaining)
          {
              devstat_t* device_statistics = NULL;
              
              if( (device_statistics = (devstat_t*) ftdmalloc(group_attributes.dev_attr_size)) != NULL)
              {
                  int rc = get_device_state_buffer(ctlfd, lgnum, dip[i].cdev, (char*)device_statistics, group_attributes.dev_attr_size);
                  
                  ftd_uint64_t uiMBToBeRefreshed = total * (1 << (dip[i].hrdb_res - 10));
                  ftd_uint64_t uiDiskMBRemainingToBeRefreshed = uiMBToBeRefreshed - ((device_statistics->rfshdelta << DEV_BSHIFT) / 1024);
                  
                  if (dPercentage == 1) // If 100% of dirty bits, we assume a checksum refresh.
                  {
                      // For checksum refreshes, the data remaining to be refreshed really implies the data remaining to be compared.
                      // To report it, we substract the amount of data analyzed from the disk size.
                      uiMBToBeRefreshed = (dip[i].disksize / divide_factor);
                      uiDiskMBRemainingToBeRefreshed = uiMBToBeRefreshed - ((device_statistics->rfshoffset << DEV_BSHIFT) / 1024);
                  }

                  if (verbose)
                  {
                      fprintf(stderr, "\nData remaining to be refreshed is %s",
                              format_drive_size(uiDiskMBRemainingToBeRefreshed));
                      
                      fprintf(stderr, " / %s\n",
                              format_drive_size(uiMBToBeRefreshed));
                  }

                  uiMBRemainingToBeRefreshed += uiDiskMBRemainingToBeRefreshed;
              }
              else
              {
                  fprintf(stderr,"Unable to allocate memory for the device statistics.\n");
              }
          }
          
            uiMBrequiredHRDB += uiDiskRequired;
        }
        
        /* use low resolution bitmaps from driver if possible (don't use pstore) */
        if (blowres) {
            total = 0;
            k = 0;
            for (j = 0; j < (unsigned int)(dip[i].lrdb_numbits/8); j++) {
               unsigned char g = LRDB[j];
#if PANALYZE_DEBUG
                /* This will print out the entire LRT map for the device */
                if (verbose) {
                   if (k == 0)
                      fprintf(stderr,"\nPrinting LRT map...");
                   if (k%16 == 0)
                      fprintf(stderr,"\n");
                   if (k%4 == 0)
                      fprintf(stderr,"\t0x");

                   fprintf(stderr,"%02x", g);
                   k++;
                }
#endif /* PANALYZE_DEBUG */
                total += count_bits(g);
            }

            /* dump sizes */
            /* Get percentage of bits dirty */
            dPercentage   = (((double)total / (double)(dip[i].lrdb_numbits)) *
                             ((double)dip[i].maxdisksize / (double)dip[i].disksize));
            if (dPercentage > 1) /* This can happen when the group is in PASSTHRU mode */
               dPercentage = 1;
               
            uiDiskRequired = (unsigned int)((dip[i].disksize / divide_factor) * dPercentage);
            /* add a 10% fudge factor here, since we aren't accounting for
             * headers or any potential inefficient allocation of
             * file system space */
            uiDiskRequired += uiDiskRequired * 0.1;
                                                                                
            if (verbose && !print_smart_refresh_remaining)
            {
	            fprintf(stderr, "\nLRDB usage:\n");
			    // Use the effective number of LRDB bits from driver info for calculation (WR PROD7920)
                fprintf(stderr,"    effective number of LRDB bits from the driver: %d\n", dip[i].lrdb_numbits);

			    if( dip[i].disksize != dip[i].maxdisksize )
				{
				    // Device expansion has not occurred to its allocated provision (PROD10347: clarify output)
	                fprintf(stderr, "    %d dirty bits out of %d bits total (including extra bits allocated for device expansion)\n",
	                                total, dip[i].lrdb_numbits);
	                fprintf(stderr, "    percentage of actual dirty data based on real device size: %2.3f%%\n", dPercentage*100);
				}
				else
				{
				    // Device expansion has occurred to its allocated provision
	                fprintf(stderr, "    %d dirty bits out of %d allocated bits\n",
	                                total, dip[i].lrdb_numbits);
	                fprintf(stderr, "    percentage of dirty data: %2.3f%%\n", dPercentage*100);
				}
                fprintf(stderr, "    disk size required is %s\n", format_drive_size(uiDiskRequired));
            }

            uiMBrequiredLRDB += uiDiskRequired;
        }

        HRDBoffset32 += dip[i].hrdbsize32;
        LRDBoffset32 += dip[i].lrdbsize32;
  
    }

    if (print_smart_refresh_remaining)
    {
        fprintf(stderr,
                "\nData remaining to be refreshed for group %d is %s\n",
                lgnum,
                format_drive_size(uiMBRemainingToBeRefreshed));
        goto errret;
    }
    
    if (verbose) {
        fprintf(stderr,"\nDisk size required for group %d is\n",lgnum);
        if (bhighres) {
            fprintf(stderr,
                    "HRDB based size requires %s for journals\n",
                    format_drive_size(uiMBrequiredHRDB));
        }
        if (blowres) {
            fprintf(stderr,
                    "LRDB based size requires %s for journals\n",
                    format_drive_size(uiMBrequiredLRDB));
        }
        fprintf(stderr,
                    "BAB  based size requires %s for journals\n",
                    format_drive_size(lgstat.bab_used/1024));
    } else {
        if (bhighres) {
            fprintf(stderr,
                    "Journal size required for group %d is %s\n",
                    lgnum,
                    format_drive_size(uiMBrequiredHRDB + (lgstat.bab_used/1024)));
            // WI_338550 December 2017, implementing RPO / RTT
            if( for_system_call )
            {
                pending_MBs = (uiMBrequiredHRDB + (lgstat.bab_used/1024)) / 1024;
                fprintf(stderr,
                        "Output for system call Journal size required for group %d is %d\n",
                        lgnum, pending_MBs);
            }
        } else {
            fprintf(stderr,
                    "Journal size required for group %d is %s\n",
                    lgnum,
                    format_drive_size(uiMBrequiredLRDB + (lgstat.bab_used/1024)));
            // WI_338550 December 2017, implementing RPO / RTT
            if( for_system_call )
            {
                pending_MBs = (uiMBrequiredLRDB + (lgstat.bab_used/1024)) / 1024;
                fprintf(stderr,
                        "Output for system call Journal size required for group %d is %d\n",
                        lgnum, pending_MBs);
            }
        }
    }


errret:
    /* free allocated memory */
    if (dip)
        free (dip);
    if (hrdb)
        free (hrdb);
    if (lrdb)
        free (lrdb);
    if (tempdev)
        free (tempdev);        

    /* close the devices */    
    close(ctlfd);
    close(lgfd);        

    return 0;
}


int main (int argc, char **argv, char **envp)
{
    int     lgcnt = 0;
    int     group = 0;
    int     i, found = 0;
	FILE    *outfd;
	char    tmpfile_prefix[32] = "/tmp/dtcpanalyze_output.";
    char    tmpfilename[64];
    char    result_buf[32];

    putenv("LANG=C");

    fprintf(stderr,"\npanalyze for %s \n",
#if defined (SOLARIS)
                "Solaris");
#elif defined (_AIX)
                "AIX");
#elif defined (HPUX)
                "HPUX");
#elif defined (linux)
                "Linux");                               
#endif
    /* Initialize the targets array to 0 */
    memset(targets, 0, sizeof(targets));

    prog_name = argv[0]; 
    /* Process the argument vector */
    proc_argv(argc, argv);
    if (initerrmgt(ERRFAC) < 0) {
       exit(1);
    }

    initconfigs();
    /* get list of all configs the groups for which are started */
    paths = (char *)configpaths;
    lgcnt = GETCONFIGS(configpaths, 1, 1);

    if (lgcnt == 0) {
       fprintf(stderr, "Target group not started/does not exist.\n");
       exit(1);
    }

    for (i = 0; i < lgcnt; i++) {
        group = cfgtonum(i);
        if (!targets[group]) {
           continue;
        }
        found = 1;
        /* Just verify journal sizes for group */
        verify_journal_size(group);
    }

    if (!found) {
        fprintf(stderr, "Target group not started/does not exist.\n");
        exit(1);
    }            
    // WI_338550 December 2017, implementing RPO / RTT
    if( for_system_call )
    {
        // Write value to output file
        sprintf(tmpfilename, "%s%d\0", tmpfile_prefix, for_system_call_group_number);
        outfd = fopen(tmpfilename,"w");
        if (outfd <= (FILE *)0)
        {
            /* file open error */
            fprintf(stderr, "Error opening output file to return the result for system call.\n");
        }
        else
        {
            memset(result_buf, 0, sizeof(result_buf));
            sprintf( result_buf, "%d\n", pending_MBs );
            fputs(result_buf,outfd);
            fclose(outfd);
        }
    }
    return 0;
}

#if defined(HPUX) && (SYSVERS >= 1100)
  shl_load () {}
  shl_unload () {}
  shl_findsym () {}
#endif

