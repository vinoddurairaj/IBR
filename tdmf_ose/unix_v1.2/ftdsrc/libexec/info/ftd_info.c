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
/**************************************************************************
 * ftd_info.c
 *
 *   (c) Copyright 1998 FullTime Software, Inc.  All Rights Reserved 
 *
 *   Description:    Print out the values of specific variables within 
 *           the device driver.
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <dirent.h>
#if defined(linux)
#include <time.h>
#endif /* defined(linux) */

#include "pathnames.h"
#include "config.h"
#include "ftdio.h"
#include "errors.h"
#include "ps_intr.h"
#include "cfg_intr.h"
#include "ftd_cmd.h"
#include "common.h"
#include "devcheck.h"

static void printshutdownstate(ps_group_info_t *ginfo);
static void printdriver(ftd_stat_t Info);
static void printlocaldisk(disk_stats_t DiskInfo, sddisk_t *, struct stat *sbp);
static void printbab(disk_stats_t DiskInfo, sddisk_t *);
static void PrintSysChkCounters(IOCTL_Get_SysChk_Counters_t* poCounters);
static void printother(ftd_stat_t Info);
static void printpmdmode(char *ps_name, char *group_name, int group);
static void printbabinfo(int);
static void dump_psdev(int lgnum, char *ps_name, int Ps_diags, unsigned int hrdb_type, int Limited_Ps_diags, char *previous_ps_name);

#define TRUE    1
#define FALSE   0

FILE    *dbgfd;

extern machine_t* mysys;
extern machine_t* othersys;

char *argv0;

/**
 * The various kinds of bitmaps that can be requested and dumped.
 */
typedef enum bitmap_request_type {
    REQUESTED_NONE,
    REQUESTED_HIGH_RES_FROM_DRIVER,
    REQUESTED_LOW_RES_FROM_DRIVER,
    REQUESTED_HIGH_RES_FROM_PSTORE,
    REQUESTED_LOW_RES_FROM_PSTORE
} bitmap_request_type_t;

/* This is how the program is used */
static void
usage(void)
{
    fprintf(stderr,"Usage: %s <options>\n\n", argv0);
    fprintf(stderr,"\
    One of the following options is mandatory:\n\
        -g <group#>    : Display info for a " GROUPNAME " group (0 - %d)\n\
        -a             : Display info for all groups\n\
        -v             : Display Version info\n\
        -a -s          : Dump Pstore structures for all groups\n\
        -g <group#> -s : Dump Pstore structures for the specified group\n\
        -g <group#> -P <dev pathname>   : Dump HIGH resolution bitmap for specified device from this group's Pstore\n\
        -g <group#> -p <dev pathname>   : Dump LOW resolution bitmap for specified device from this group's Pstore\n\
        -g <group#> -I <dev pathname>   : Dump driver's HIGH resolution bitmap for specified device of this group\n\
        -g <group#> -i <dev pathname>   : Dump driver's LOW resolution bitmap for specified device of this group\n\
         NOTE          : for the options -P, -p, -I and -i, you must redirect the output to a formatting utility,\n\
                         for instance the octal dump utility od; example: dtcinfo -g 0 -P /dev/jfs2xx4 | od -tx4\n\
        -q             : quiet mode: do not log the info command in the error log file\n\
        -h             : Print This Listing\n", MAXLG-1);
    exit(1);
}

static char  paths[MAXLG][32];	/*For Bug WR12548 */
static char version[512];

int dump_bitmap_to_stdout(int lgnum, char* source_device_name, bitmap_request_type_t bitmap_requested, char* ps_name);

int main(int argc, char **argv)
{
    char           buf[MAXPATHLEN];
    char           ps_name[MAXPATHLEN];
    char           previous_ps_name[MAXPATHLEN];
    char           group_name[MAXPATHLEN];
    int            master;
    int            ch;
    stat_buffer_t  statbuf;
    disk_stats_t   DiskInfo;
    ftd_stat_t     Info;
    ps_group_info_t ginfo;
    int            i;
    int            err;
    unsigned long  DevicesDisplayed = 0;
    int            NumGroups;
    int            GroupNum;
    sddisk_t*      sdp;
    long           lg;
    struct stat    sb;

    /* Assume all options are off */
    unsigned long  LogicalGroup = 0;
    /* int            lgnum = 0; */
    int            group;
    unsigned short lgnum_list[MAXLG];           /* For Bug WR12548 */
    int            iArgLgNum = -1, iLgCount = 0;/* For Bug WR12548 */
    unsigned long  DisplayAll = 0;
    unsigned long  Other = 0;
    int            Ps_diags = 0;
	int            Limited_Ps_diags = 0;
    char           lgstr[4];
    bitmap_request_type_t bitmap_requested = REQUESTED_NONE;
    char* bitmap_device = NULL;
    int            quiet_mode = 0;
    int            check_disable_marker_file = 0;
                    
#if defined(linux)
    IOCTL_Get_SysChk_Counters_t oCounters;
#endif

    ps_version_1_attr_t 	attr;	 /* To get the attributes from the PStore and determine hrt size (pc080201) */

    putenv("LANG=C");

    /* Make sure we are root */
    if (geteuid()) {
        fprintf(stderr, "Error: You must be root to run this process...aborted\n");
        exit(1);
    }

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    argv0 = argv[0];
    if (argc < 2) {
        usage();
    }
    
    sprintf (version,"%s  Version %s%s", PRODUCTNAME, VERSION, VERSIONBUILD );

    /* Determine what options were specified */
    opterr = 0;
    while ((ch =getopt(argc, argv, "vahog:dI:i:P:p:sq")) != -1) {
        switch (ch) {
        case 'a':
            /* Change the operation mode of all devices in all Logical Groups */
            DisplayAll = 1;
            break;
        case 'g':
            /* Change the operation mode of all devices in a Logical Group */
            group = ftd_strtol(optarg);
            if (group < 0 || group >= MAXLG) {
                fprintf(stderr, "ERROR: [%s] is invalid number for " GROUPNAME " group\n", optarg);
                usage();
            }
            LogicalGroup = 1;
            /* For Bug WR12548 */
            iArgLgNum++;
            lgnum_list[iArgLgNum]=group;
            /*lgnum = strtol(optarg, NULL, 0);*//* For Bug WR12548 */
            break;
        case 'v':
            /* print the VERSION information */
            printf("%s\n", version);
            exit(0);
        case 'o':
            /* Display Other Info ( Not for Customer Use ) */
            Other = 1;
            break;
        case 'd':
            /* pstore diagnostics */
            Ps_diags = 1;
            break;
        case 's':
            /* Limited pstore diagnostics (no dump of dirty bits, just the header, group and device Structures) */
            Ps_diags = 1;
            Limited_Ps_diags = 1;
            break;
        case 'I':
            /* Dump driver's high resolution bitmap. */
            bitmap_requested = REQUESTED_HIGH_RES_FROM_DRIVER;
            bitmap_device = optarg;
            break;
        case 'i':
            /* Dump driver's low resolution bitmap. */
            bitmap_requested = REQUESTED_LOW_RES_FROM_DRIVER;
            bitmap_device = optarg;
            break;
        case 'P':
            /* Dump pstore's low resolution bitmap. */
            bitmap_requested = REQUESTED_HIGH_RES_FROM_PSTORE;
            bitmap_device = optarg;
            break;
        case 'p':
            /* Dump pstore's high resolution bitmap. */
            bitmap_requested = REQUESTED_LOW_RES_FROM_PSTORE;
            bitmap_device = optarg;
            break;
        case 'q':
            /* Limited pstore diagnostics (no dump of dirty bits, just the header, group and device Structures) */
            quiet_mode = 1;
            break;
        case 'h':
        default:
            usage();
            break;
        }
    }

    if (optind != argc) {
        fprintf(stderr, "Error: Invalid arguments\n");
        usage();
    }

    /* Make sure the specified options are consistent */
    /****************************************************************************************/
    /* Only one of options -g and -a should be specified */
    if ((LogicalGroup + DisplayAll) > 1) {
        fprintf(stderr,"Error: Only one of the -g and -a options should be specified.\n");
        usage();
    }

    /* One of options -g or -a must be specified */
    if ((LogicalGroup + DisplayAll) == 0) {
        fprintf(stderr,"Error: One of the -g and -a options must be specified.\n");
        usage();
    }

    if( !quiet_mode )
        log_command(argc, argv);  /* add trace of command to dtcerror.log */

#if defined(linux)
    check_disable_marker_file = 1;
#endif
    /* open the master ftd device */
    if ((master = open(FTD_CTLDEV, O_RDWR)) < 0) {
		if( check_disable_marker_file && (stat(SFTKDTC_SERVICES_DISABLE_FILE, &sb) == 0) )
		{
	        fprintf(stderr, "Error: Failed to open " PRODUCTNAME " master device: %s.\n", FTD_CTLDEV);
	        fprintf(stderr, "The SFTKdtc services have been disabled (presence of file " SFTKDTC_SERVICES_DISABLE_FILE " has been detected).\n");
	        fprintf(stderr, "To re-enable SFTKdtc services, delete the file; then reboot or manually load the " QNM " driver and start the master daemon.\n" );
		}
		else
		{
	        fprintf(stderr,"Error: Failed to open " PRODUCTNAME " master device: %s.  \nHas " QNM " driver been added?\n", FTD_CTLDEV);
		}
        exit(1);
    }


    /* print out the bab data */
    if (!Ps_diags && bitmap_requested == REQUESTED_NONE)
        printbabinfo(master);

    initconfigs();
    if (Ps_diags || bitmap_requested != REQUESTED_NONE)
        NumGroups = GETCONFIGS(paths, 1, 0);
    else
        NumGroups = GETCONFIGS(paths, 1, 1);
    /* Make sure that there is at least 1 Logical Group Defined */
    if ((!Ps_diags || bitmap_requested == REQUESTED_NONE) && NumGroups == 0) {
        fprintf(stderr,"       No " CAPGROUPNAME " Groups have been started.\n");
        exit(1);
    }

    /* For each logical group... */
    for (GroupNum = 0; GroupNum < NumGroups; GroupNum++) {

        if (LogicalGroup) {
            /* For Bug WR12548 */
            for ( iLgCount = 0; iLgCount <= iArgLgNum; iLgCount++) {
                if (Ps_diags || bitmap_requested != REQUESTED_NONE)
	            /*sprintf(buf, "p%03d.cfg", lgnum);*/
	            sprintf(buf, "p%03d.cfg", lgnum_list[iLgCount]);
                else
                    /*sprintf(buf, "p%03d.cur", lgnum);*/
					sprintf(buf, "p%03d.cur", lgnum_list[iLgCount]);
              /*if (strcmp(buf, paths[GroupNum]) != 0)*/ 
              /*    continue; */
                if (strcmp(buf, paths[GroupNum]) == 0)
                    break;
            }
            if( iLgCount > iArgLgNum)
                continue;
            /* For Bug WR12548 */
        }
        readconfig(1, 0, 0, paths[GroupNum]);
        strcpy (ps_name, sys[0].pstore);
        strncpy(lgstr, paths[GroupNum]+1, 3);
        lgstr[3] = 0;
        lg = atol(lgstr);

        if (bitmap_requested == REQUESTED_NONE)
        {
            printf("\n" CAPGROUPNAME " Group %ld (%s -> %s)\n", lg, mysys->name,
                   othersys->name);
        }
       
        /* get the state from the pstore */
        FTD_CREATE_GROUP_NAME(group_name, (int) lg);

        memset(&ginfo, 0, sizeof(ginfo));
        if ((err = ps_get_group_info(ps_name, group_name, &ginfo)) != PS_OK) {
            if (err == PS_GROUP_NOT_FOUND) {
                fprintf(stderr,"group doesn't exist in pstore: %d\n", err);
            } else {
                fprintf(stderr,"PSTORE error: %d [%s]", err, ps_get_pstore_error_string(err));
            }
            fprintf(stderr," %s, %s\n", ps_name, group_name);
            exit(1);
        }

        // Get Pstore info to know the hrdb_type.
        if( (err = ps_get_version_1_attr(ps_name, &attr, 1)) != PS_OK )
		{
            fprintf(stderr,"Failed getting Pstore %s attributes for group %s (ps_get_version_1_attr) [%s].\n", ps_name, group_name, ps_get_pstore_error_string(err));
            exit(1);
		}

        /* diags */
        if (Ps_diags) {
            if(LogicalGroup) { /* For Bug WR12548 */
				dump_psdev(lgnum_list[iLgCount], ps_name, Ps_diags, attr.hrdb_type, Limited_Ps_diags, previous_ps_name);	/* For Bug WR12548 */
            } else {
                /*  Use lg instead of lgnum which is never updated (stay as 0)
                 *  This will cause error if -a is used instead of -g
                 */
				dump_psdev(lg, ps_name, Ps_diags, attr.hrdb_type, Limited_Ps_diags, previous_ps_name);
            }
            
            ++DevicesDisplayed;
			strcpy( previous_ps_name, ps_name ); // Take note of Pstore to avoid multiple displays (PROD8678)
            continue;
        }

        if (bitmap_requested != REQUESTED_NONE)
        {
            if (dump_bitmap_to_stdout(lg, bitmap_device, bitmap_requested, ps_name) == -1)
            {
                exit(1);
            }
            DevicesDisplayed++;
            break; // We only want to dump a single bitmap for a single device.
        }
        
        if ((ginfo.state == FTD_MODE_BACKFRESH) || (ginfo.shutdown == 1)) {
            printshutdownstate(&ginfo);
            DevicesDisplayed++;
            continue;
        }

        /* get the group stats */
        statbuf.lg_num = lg;
        statbuf.dev_num = 0;
        statbuf.len = sizeof(Info);
        statbuf.addr = (ftd_uint64ptr_t)(unsigned long)&Info;

		if (0) {
			fprintf(stderr, "    statbuf{lg_num = %llx, dev_num= %llx, len= %d, addr= %llx\n",
				statbuf.lg_num, statbuf.dev_num,
				statbuf.len, statbuf.addr);
		}

        /* Get the info from the Device */
        if((err = FTD_IOCTL_CALL(master, FTD_GET_GROUP_STATS, &statbuf)) < 0) {
			fprintf(stderr, "FTD_GET_GROUP_STATS ioctl(%d:%llx): error = %s(%d)\n",
				statbuf.len, statbuf.addr, strerror(errno), errno);
            exit(1);
        }

        ++DevicesDisplayed;

        /* print logical group info */
        printdriver(Info);

	    /* Display current hrt type */
        if( ps_get_version_1_attr(ps_name, &attr, 1) == PS_OK )
        {
          printf( "    HRT Type........................ " );
          if( attr.hrdb_type == FTD_HS_SMALL )
            printf( "Small\n" );
          else if( attr.hrdb_type == FTD_HS_LARGE )
            printf( "Large\n" );
		  else
            printf( "Proportional\n" );
        }
    	else
          fprintf(stderr,"    Cannot determine HRT size (error reading PStore attributes from %s)\n", ps_name );

        if (!Ps_diags) {
            if(LogicalGroup) {  /* For Bug WR12548 */
              dump_psdev(lgnum_list[iLgCount], ps_name, Ps_diags, attr.hrdb_type, Limited_Ps_diags, previous_ps_name);    /* For Bug WR12548 */
            } else {
              dump_psdev(lg, ps_name, Ps_diags, attr.hrdb_type, Limited_Ps_diags, previous_ps_name);
            }
        }

#if 0
        printpmdmode(ps_name, group_name, lg);
#endif

        if (Other)
            printother(Info);


        /* Pointer to the first device in this Logical Group */
        sdp = mysys->group[0].headsddisk;

        /* While there are Disks left in this Logical Group...*/
        i = 0;
        while (sdp != NULL) {

            printf("\n    Device %s:\n", sdp->sddevname);

            stat(sdp->sddevname, &sb);
            statbuf.lg_num = lg;
            statbuf.dev_num = sb.st_rdev;
            statbuf.len = sizeof(DiskInfo);
            statbuf.addr = (ftd_uint64ptr_t)(unsigned long)&DiskInfo;

            /* Get the info from the Device */
            if((err = FTD_IOCTL_CALL(master, FTD_GET_DEVICE_STATS, &statbuf)) < 0) {
                fprintf(stderr,"FTD_GET_DEVICE_STATS ioctl: error = %d\n", errno);
                exit(1);
            }

            printlocaldisk(DiskInfo, sdp, &sb);
            printbab(DiskInfo, sdp);

            sdp = sdp->n;
            i++;
        }

        // Getting counters are only implemented for Linux at this moment.
#if defined(linux)
        oCounters.lgnum = GroupNum;
        if((err = FTD_IOCTL_CALL(master, FTD_GET_SYSCHK_COUNTERS, &oCounters)) < 0) {
            //fprintf(stderr,"FTD_GET_SYSCHK_COUNTERS ioctl: error = %d\n", errno);
            //exit(1);
        }
        else
        {
            PrintSysChkCounters(&oCounters);
        }
#endif
    }
    close(master);

    if ( DevicesDisplayed == 0) {
        fprintf(stderr,"Failure: No info could be displayed for any device.\n");
        fprintf(stderr,"     Either the group(s) have not been started or\n");
        fprintf(stderr,"     the group number(s) specified with \"-g\" is incorrect.\n");
        exit(1);
    }
    return 0;
}

static void
printbabinfo(int master)
{
    char buf[MAXPATHLEN];
    ftd_babinfo_t babinfo;
    int num_chunks, chunk_size, size;

    /* get bab size from driver */
    FTD_IOCTL_CALL(master, FTD_GET_BAB_SIZE, &babinfo);

    memset(buf, 0, sizeof(buf));
    if (cfg_get_key_value("num_chunks", buf, CFG_IS_NOT_STRINGVAL) != CFG_OK) {
/*
        return;
*/
    }
    num_chunks = strtol(buf, NULL, 0);

    memset(buf, 0, sizeof(buf));
    if (cfg_get_key_value("chunk_size", buf, CFG_IS_NOT_STRINGVAL) != CFG_OK) {
/*
        return;
*/
    }

    chunk_size = strtol(buf, NULL, 0);
    
    size = chunk_size * num_chunks;
    printf("\nRequested BAB size ................ %d (~ %d MB)\n", size, size >> 20);
    printf("Actual BAB size ................... %d (~ %d MB)\n", babinfo.actual, babinfo.actual >> 20);
    printf("Free BAB size ..................... ~ %d MB\n", babinfo.free >> 20);


}

static void
printdriver(ftd_stat_t Info) 
{
    char *state;

    switch (Info.state) {
    case FTD_MODE_PASSTHRU:
        state = "Passthru";
        break;
    case FTD_MODE_NORMAL:
    case FTD_MODE_CHECKPOINT_JLESS:
        state = "Normal";
        break;
    case FTD_MODE_TRACKING:
        state = "Tracking";
        break;
    case FTD_MODE_REFRESH:
        state = "Refresh";
        break;
    case FTD_MODE_BACKFRESH:
        state = "Backfresh";
        break;
    case FTD_MODE_FULLREFRESH:
    default:
	state = "Full Refresh"; 
	break;
    }

    printf("\n    Mode of operations.............. %s\n", state);
	// NOTE: the following substring is grepped for by the dtcfailover script: do not change it
    printf("    Entries in the BAB.............. %u\n", Info.wlentries);
    printf("    Sectors in the BAB.............. %llu\n", Info.wlsectors);
    if (Info.sync_depth != (unsigned int) -1) {
        printf("    Sync/Async mode................. Sync\n");
        printf("    Sync mode target depth.......... %u\n", Info.sync_depth);
        printf("    Sync mode timeout............... %u\n", Info.sync_timeout);
    } else {
        printf("    Sync/Async mode................. Async\n");
    }
    printf("    I/O delay....................... %u\n", Info.iodelay);
    printf("    Persistent Store................ %s\n", sys[0].pstore);
}

static void
printshutdownstate(ps_group_info_t *ginfo)
{
    char *state;

    switch (ginfo->state) {
    case FTD_MODE_PASSTHRU:
        state = "Passthru";
        break;
    case FTD_MODE_NORMAL:
        state = "Normal";
        break;
    case FTD_MODE_TRACKING:
        state = "Tracking";
        break;
    case FTD_MODE_REFRESH:
        state = "Refresh";
        break;
    case FTD_MODE_BACKFRESH:
    default:
        state = "Backfresh";
        break;
    }

    // IMPORTANT: the substring "Mode" in the following output is grepped by certain scripts that depend on it
    printf("\n    Mode of operations.............. %s\n", state);
    printf("    Shutdown state ................. %u\n", ginfo->shutdown);
}

static void
printlocaldisk(disk_stats_t DiskInfo, sddisk_t *sdp, struct stat *sbp)
{
    printf("\n        dtc device number............... 0x%x\n", (int) sbp->st_rdev);
    printf("        Local disk device number........ 0x%x\n", (int) DiskInfo.localbdisk);
    printf("        Local disk size (sectors)....... %llu\n", DiskInfo.localdisksize);
	// Do not change the substring "Local disk name" in the following output; it is used
	// in grep by the dtcfailover script
    printf("        Local disk name................. %s\n", sdp->devname);
    printf("        Local disk dynamically activated %s\n", DiskInfo.local_disk_io_captured ? "Yes" : "No");
    printf("        Remote mirror disk.............. %s:%s\n", 
        othersys->name, sdp->mirname);
#if defined(_AIX)
    if (sdp->md_minor != -1  || sdp->md_major != -1)
    {
	printf("        Remote mirror device number..... 0x%x\n",
	    makedev((sdp->md_major <= -1? 0 : sdp->md_major),
		    (sdp->md_minor <= -1? 0 : sdp->md_minor)));
    }
#endif
}

static void 
printbab(disk_stats_t DiskInfo, sddisk_t *sdp)
{
    printf("        Read I/O count.................. %llu\n", DiskInfo.readiocnt);
    printf("        Total # of sectors read......... %llu\n", DiskInfo.sectorsread);
	// NOTE: the following substring is grepped for by the dtcfailover script: do not change it
    printf("        Write I/O count................. %llu\n", DiskInfo.writeiocnt);
    printf("        Total # of sectors written...... %llu\n", DiskInfo.sectorswritten);
	// NOTE: the following substring is grepped for by the dtcfailover script: do not change it
    printf("        Entries in the BAB.............. %u\n", DiskInfo.wlentries);
    printf("        Sectors in the BAB.............. %llu\n", DiskInfo.wlsectors);
}

void PrintSysChkCounters(IOCTL_Get_SysChk_Counters_t* poCounters)
{
    printf("                     --- BAB stats ---\n");
    printf("        Bytes tracked................... %llu\n", poCounters->ulWriteByteCount);
    printf("        Bytes written................... %llu\n", poCounters->ulWriteDoneByteCount);
    printf("        Bytes pending................... %llu\n", poCounters->ulPendingByteCount);
    printf("        Bytes committed................. %llu\n", poCounters->ulCommittedByteCount);
    printf("        Bytes migrated.................. %llu\n", poCounters->ulMigratedByteCount);
}

#if 0
static void
printpmdmode(char *ps_name, char *group_name, int group)
{
    char    value[MAXPATHLEN];

    FTD_CREATE_GROUP_NAME(group_name, group);

    /* get mode from the pstore */
    if (ps_get_group_key_value(ps_name, group_name, "MODE", value) != PS_OK) {
        return;
    }
    printf("\n    PMD mode........................ %s\n", value);

    return;
}
#endif

static void
printother(ftd_stat_t Info)
{
    char *tb;
    time_t lt;

    lt = (time_t) Info.loadtimesecs;
    tb = ctime(&lt);
    tb[strlen(tb) - 1] = '\0';
    printf("\n    Load time....................... %s\n", tb);
    printf("    Load time system ticks........... %u\n", Info.loadtimesystics);

    printf("   Used (calc): %d Free (calc): %d\n", Info.bab_used, Info.bab_free);

}

#include "ps_pvt.h"

// Dump pstore header info
static void
dump_Pstore_header_info( char *ps_name, ps_hdr_t *ps_hdr )
{
    unsigned int supports_Proportional_HRDB = 0;
    unsigned int hrdb_type;
    
	supports_Proportional_HRDB = (ps_hdr->magic == PS_VERSION_1_MAGIC);

    if( supports_Proportional_HRDB )
	{
        hrdb_type = ps_hdr->data.ver1.hrdb_type;
	}
	else // Old pre-RFX271 Pstore format
	{
		switch( ps_hdr->data.ver1.Small_or_Large_hrdb_size )
		{
	      case FTD_PS_HRDB_SIZE_SMALL:
	          hrdb_type = FTD_HS_SMALL;
		      break;
	      case FTD_PS_HRDB_SIZE_LARGE:
	          hrdb_type = FTD_HS_LARGE;
		      break;
		}
	    fprintf(stdout,"\nPre-RFX271 Pstore format is used in Pstore %s\n", ps_name);
	}

	fprintf(stdout,"\n");
	fprintf(stdout,"Header ps_version_1_hdr_t for Pstore %s:\n", ps_name);
    fprintf(stdout,"    unsigned int         max_dev = %d\n", ps_hdr->data.ver1.max_dev);
    fprintf(stdout,"    unsigned int         max_group = %d\n", ps_hdr->data.ver1.max_group);
    fprintf(stdout,"    unsigned int         dev_attr_size = %d\n", ps_hdr->data.ver1.dev_attr_size);
    fprintf(stdout,"    unsigned int         group_attr_size = %d\n", ps_hdr->data.ver1.group_attr_size);
    fprintf(stdout,"    unsigned int         dev_table_entry_size = %d\n", ps_hdr->data.ver1.dev_table_entry_size);
    fprintf(stdout,"    unsigned int         group_table_entry_size = %d\n", ps_hdr->data.ver1.group_table_entry_size);
    fprintf(stdout,"    unsigned int         lrdb_size = %d\n", ps_hdr->data.ver1.lrdb_size);

	if( hrdb_type != FTD_HS_PROPORTIONAL )
        fprintf(stdout,"    unsigned int         Small_or_Large_hrdb_size = %d\n",
                ps_hdr->data.ver1.Small_or_Large_hrdb_size);
	else
        fprintf(stdout,"    unsigned int         Small_or_Large_hrdb_size = N/A (Proportional HRDB)\n");

    fprintf(stdout,"    unsigned int         num_device = %d\n", ps_hdr->data.ver1.num_device);
    fprintf(stdout,"    unsigned int         num_group = %d\n", ps_hdr->data.ver1.num_group);
    fprintf(stdout,"    unsigned int         last_device = %d\n", ps_hdr->data.ver1.last_device);
    fprintf(stdout,"    unsigned int         last_group = %d\n", ps_hdr->data.ver1.last_group);

	fprintf(stdout,"\n");
    fprintf(stdout,"  Offsets to structures in KBytes:\n" );
    fprintf(stdout,"    unsigned int         dev_attr_offset = %d\n", ps_hdr->data.ver1.dev_attr_offset);
    fprintf(stdout,"    unsigned int         group_attr_offset = %d\n", ps_hdr->data.ver1.group_attr_offset);
    fprintf(stdout,"    unsigned int         dev_table_offset = %d\n", ps_hdr->data.ver1.dev_table_offset);
    fprintf(stdout,"    unsigned int         group_table_offset = %d\n", ps_hdr->data.ver1.group_table_offset);
    fprintf(stdout,"    unsigned int         lrdb_offset = %d\n", ps_hdr->data.ver1.lrdb_offset);
    fprintf(stdout,"    unsigned int         hrdb_offset = %d\n", ps_hdr->data.ver1.hrdb_offset);
    fprintf(stdout,"    unsigned int         last_block = %d\n", ps_hdr->data.ver1.last_block);

	fprintf(stdout,"\n");
    if( supports_Proportional_HRDB ) // TODO: all these should be removed (cleanup) if time permits
	{
        fprintf(stdout,"    unsigned int         hrdb_type = %d ", ps_hdr->data.ver1.hrdb_type);
		switch( ps_hdr->data.ver1.hrdb_type )
		{
		    case FTD_HS_SMALL:
			    fprintf(stdout," (Small)\n" );
				break;
		    case FTD_HS_LARGE:
			    fprintf(stdout," (Large)\n" );
				break;
		    case FTD_HS_PROPORTIONAL:
			    fprintf(stdout," (Proportional)\n" );
				break;
			default:
			    fprintf(stdout," (invalid ?)\n" );
				break;
		}
        fprintf(stdout,"    unsigned int         dev_HRDB_info_table_offset (KB) = %d\n", ps_hdr->data.ver1.dev_HRDB_info_table_offset);

	    if( hrdb_type == FTD_HS_PROPORTIONAL )
		{
            fprintf(stdout,"    unsigned int         tracking_resolution_level = %d", ps_hdr->data.ver1.tracking_resolution_level);
			fprintf(stdout," (%s)\n", ps_get_tracking_resolution_string(ps_hdr->data.ver1.tracking_resolution_level) );
		}
		else
		{
            fprintf(stdout,"    unsigned int         tracking_resolution_level = N/A (not Proportional HRDB)\n");
		}
        fprintf(stdout,"    unsigned int         max_HRDB_size_KBs = %d\n", ps_hdr->data.ver1.max_HRDB_size_KBs);
        fprintf(stdout,"    unsigned int         next_available_HRDB_offset (KB) = %d\n", ps_hdr->data.ver1.next_available_HRDB_offset);
        fprintf(stdout,"    unsigned int         dev_HRDB_info_entry_size = %d\n", ps_hdr->data.ver1.dev_HRDB_info_entry_size);
	}

    fprintf(stdout,"} ps_version_1_hdr_t;\n");
}

/*-
 * cough up pstore states...
 */
static void
dump_devinfo(ps_name, dev_name, dev_info, hrdb_type, tracking_resolution_level, supports_Proportional_HRDB)
	char *ps_name; 
	char *dev_name;
	ps_dev_info_t *dev_info;
	int  tracking_resolution_level;
{
    sddisk_t *sd = NULL;
    int err;

	/* poot forth a dev_info struct */
	fprintf(stdout,"\n");
	fprintf(stdout,"Fields from ps_dev_info_t and ps_dev_entry_2_t for dev %s:\n", dev_name);

    // PROD8646: verify if the device has been removed from the config file
    for (sd = mysys->group->headsddisk; sd; sd = sd->n)
    {
	    if(strcmp(dev_name, sd->sddevname) == 0)
            break;
    }
	if( sd == NULL )
	{
	    fprintf(stdout,"  <<< NOTICE: device %s is still defined in the Pstore but is no longer in this group's configuration file >>>\n", dev_name); 
	}

	fprintf(stdout,"    char         *name                       = %s\n", dev_info->name);
	fprintf(stdout,"    unsigned int info_valid_lrdb_bits        = %u\n", dev_info->info_valid_lrdb_bits);
	fprintf(stdout,"    unsigned int info_allocated_lrdb_bits    = %u\n", dev_info->info_allocated_lrdb_bits);
	fprintf(stdout,"    unsigned int info_allocated_hrdb_bits    = %u\n", dev_info->info_allocated_hrdb_bits);
	fprintf(stdout,"    unsigned int info_valid_hrdb_bits        = %u\n", dev_info->info_valid_hrdb_bits);
	fprintf(stdout,"    unsigned int hrdb_type                   = " );
    switch( hrdb_type )
	{
	    case FTD_HS_SMALL:
		  fprintf(stdout,"Small\n");
		  break;
	    case FTD_HS_LARGE:
		  fprintf(stdout,"Large\n");
		  break;
	    case FTD_HS_PROPORTIONAL:
		  fprintf(stdout,"Proportional (%s)\n", ps_get_tracking_resolution_string(tracking_resolution_level));
		  break;
	}

	fprintf(stdout,"    unsigned int lrdb_res_shift_count        = %u  (power of 2 giving the lrdb resolution of %u sectors per bit)\n",
	        dev_info->lrdb_res_shift_count, (1 << dev_info->lrdb_res_shift_count));
    fprintf(stdout,"    unsigned int lrdb_res_sectors_per_bit    = %u  (* %u valid bits = coverage for %llu bytes = %llu sectors)\n",
            dev_info->lrdb_res_sectors_per_bit, dev_info->info_valid_lrdb_bits,
            (unsigned long long)(dev_info->lrdb_res_sectors_per_bit) * DEV_BSIZE * dev_info->info_valid_lrdb_bits,
            (unsigned long long)(dev_info->lrdb_res_sectors_per_bit) * dev_info->info_valid_lrdb_bits);
     
    fprintf(stdout,"    unsigned int hrdb_resolution_KBs_per_bit = %u", dev_info->hrdb_resolution_KBs_per_bit);
	// Check if HRDB resolution in KBs per bit is 0; in the case of legacy Small or Large HRT, this is possible
	// because the driver may have set a resolution of 512 bytes for small devices on platforms that support
	// a 512-byte sector size.
    if( dev_info->hrdb_resolution_KBs_per_bit == 0 )
	{
	    if( (hrdb_type != FTD_HS_PROPORTIONAL) && (DEV_BSIZE < 1024) )
		{
            fprintf(stdout," (512 bytes per bit; * %u valid bits = coverage for %llu bytes = %llu sectors)",
                             dev_info->info_valid_hrdb_bits, (unsigned long long)512 * dev_info->info_valid_hrdb_bits, 
                             ((unsigned long long)512 * dev_info->info_valid_hrdb_bits) / DEV_BSIZE); 
		}
		else
		{
            fprintf(stdout," <<< WARNING: ABNORMAL VALUE !"); 
		}
	}
	else
	{
        fprintf(stdout," (* %u valid bits = coverage for %llu bytes = %llu sectors)", 
                         dev_info->info_valid_hrdb_bits, 
                         (unsigned long long)(dev_info->hrdb_resolution_KBs_per_bit) * 1024 * dev_info->info_valid_hrdb_bits, 
                         ((unsigned long long)(dev_info->hrdb_resolution_KBs_per_bit) * 1024 * dev_info->info_valid_hrdb_bits) / DEV_BSIZE); 
	}
    fprintf(stdout,"\n"); 
	fprintf(stdout,"    unsigned int hrdb_size (bytes)           = %u\n", 
        	dev_info->hrdb_size);
	fprintf(stdout,"    unsigned int dev_HRDB_offset_in_KBs      = %u\n", 
        	dev_info->dev_HRDB_offset_in_KBs);
    fprintf(stdout,"    unsigned long long num_sectors           = %llu (real device size in %d-byte sectors)\n", 
    	dev_info->num_sectors, DEV_BSIZE);
    fprintf(stdout,"    unsigned long long orig_num_sectors      = %llu (original device size in %d-byte sectors before any device expansion)\n", 
    	dev_info->orig_num_sectors, DEV_BSIZE);
	fprintf(stdout,"    unsigned long long limitsize_multiple    = %llu (device expansion factor (provision))\n", 
        	dev_info->limitsize_multiple);

	return;

}

static void
dump_groupinfo(group_name, group_info)
	char *group_name;
	ps_group_info_t *group_info;
{

	fprintf(stdout,"\n");
	fprintf(stdout,"ps_group_info_t for group %s:\n", group_name);
	fprintf(stdout,"struct _ps_group_info {\n");
	fprintf(stdout,"    char            *name = %s\n", group_info->name);
	fprintf(stdout,"    int             state = 0x%08x ", group_info->state);
    switch (group_info->state)
    {
        case FTD_MODE_PASSTHRU:
            fprintf(stdout, "(Passthru)\n");
            break;
        case FTD_MODE_NORMAL:
        case FTD_MODE_CHECKPOINT_JLESS:
            fprintf(stdout, "(Normal)\n");
            break;
        case FTD_MODE_TRACKING:
            fprintf(stdout, "(Tracking)\n");
            break;
        case FTD_MODE_REFRESH:
            fprintf(stdout, "(Refresh)\n");
            break;
        case FTD_MODE_BACKFRESH:
            fprintf(stdout, "(Backfresh)\n");
            break;
        case FTD_MODE_FULLREFRESH:
            fprintf(stdout, "(Full Refresh)\n");
            break;
        default:
            fprintf(stdout, "\n");
            break;
    }
	fprintf(stdout,"    unsigned int   hostid = 0x%08x\n", group_info->hostid);
	fprintf(stdout,"    int          shutdown = 0x%08x\n", group_info->shutdown);
	fprintf(stdout,"    int        checkpoint = 0x%08x\n", group_info->checkpoint);
	fprintf(stdout,"} ps_group_info_t;\n");

	return;
}

/**
 * Obtains the contents of a lrdb from the pstore.
 *
 * Prints an error message to stderr if something goes wrong.
 *
 * @param ps_name           The path of the pstore.
 * @param raw_dtc_dev_name  The name of the dtc device of which we want the bitmap.
 * @param bitmap[in]        A pointer (by reference) that will be set to the obtained bitmap.
 * @param bitmap[out]       A pointer (by reference) to newly allocated memory containing the bitmap.
 *                          It is the caller's responsibility to free this memory afterwards.
 *
 * @return The length of the bitmap, or -1 if the bitmap couldn't be obtained.
 *
 */
static ftd_int32_t get_pstore_lrdb(char* ps_name, char* raw_dtc_dev_name, char** bitmap)
{
	ftd_int32_t bitmap_len=0;
	unsigned int num_bits;

	/* first call in `descriptor' mode */
	if ( ps_get_lrdb(ps_name, raw_dtc_dev_name, *bitmap, bitmap_len, &num_bits) != PS_OK) {
			fprintf(stderr,"get_pstore_lrdb: ps_get_lrdb failed\n");
			fprintf(stderr," ps_name: %s raw_dtc_dev_name: %s\n", ps_name, &raw_dtc_dev_name[0]);
			return -1;
	}

	bitmap_len = num_bits/8;
	*bitmap = (char *)malloc(bitmap_len);

    if (*bitmap == 0)
    {
        perror("get_pstore_lrdb: Could not allocate memory for the bitmap.");
        return -1;
    }
    
	/* get the bitmap */
	if ( ps_get_lrdb(ps_name, raw_dtc_dev_name, *bitmap, bitmap_len, &num_bits) != PS_OK) {
			fprintf(stderr,"get_pstore_lrdb: ps_get_lrdb failed\n");
			fprintf(stderr," ps_name: %s raw_dtc_dev_name: %s\n", ps_name, &raw_dtc_dev_name[0]);
			return -1;
	}
  
    return bitmap_len;
}


/**
 * Obtains the contents of a hrdb from the pstore.
 *
 * Prints an error message to stderr if something goes wrong.
 *
 * @param ps_name           The path of the pstore.
 * @param raw_dtc_dev_name  The name of the dtc device of which we want the bitmap.
 * @param bitmap[in]        A pointer (by reference) that will be set to the obtained bitmap.
 * @param bitmap[out]       A pointer (by reference) to newly allocated memory containing the bitmap.
 *                          It is the caller's responsibility to free this memory afterwards.
 *
 * @return The length of the bitmap, or -1 if the bitmap couldn't be obtained.
 *
 */
static ftd_int32_t get_pstore_hrdb(char* ps_name, char* raw_dtc_dev_name, char** bitmap)
{
	ftd_int32_t bitmap_len=0;
	unsigned int num_bits;

	/* first call in `descriptor' mode */
	if ( ps_get_hrdb(ps_name, raw_dtc_dev_name, *bitmap, bitmap_len, &num_bits) != PS_OK) {
			fprintf(stderr,"get_pstore_hrdb: ps_get_hrdb failed\n");
			fprintf(stderr," ps_name: %s raw_dtc_dev_name: %s\n", ps_name, &raw_dtc_dev_name[0]);
			return -1;
	}

	bitmap_len = num_bits/8;
	*bitmap = (char *)malloc(bitmap_len);

    if (*bitmap == 0)
    {
        perror("get_pstore_hrdb: Could not allocate memory for the bitmap.");
        return -1;
    }
    
	/* get the bitmap */
	if ( ps_get_hrdb(ps_name, raw_dtc_dev_name, *bitmap, bitmap_len, &num_bits) != PS_OK) {
			fprintf(stderr,"get_pstore_hrdb: ps_get_hrdb failed\n");
			fprintf(stderr," ps_name: %s raw_dtc_dev_name: %s\n", ps_name, &raw_dtc_dev_name[0]);
			return -1;
	}
  
    return bitmap_len;
}

static void
dump_lrdb(ps_name, dev_name)
	char *ps_name;
	char *dev_name;
{
	int i;
	int cnt;
	int buf_len=0;
	char *bits=(char *)0;
	unsigned int num_bits;
	unsigned int *uintp;

    buf_len = get_pstore_lrdb(ps_name, dev_name, &bits);
	/* first call in `descriptor' mode */
	if ( ps_get_lrdb(ps_name, dev_name, bits, buf_len, &num_bits) != PS_OK) {
			fprintf(stderr,"dump_lrdb: ps_get_lrdb failed\n");
			fprintf(stderr," ps_name: %s dev_name: %s\n", ps_name, &dev_name[0]);
			return;
	}

    if (buf_len < 0)
    {
        return;
    }
	buf_len = num_bits/8;
	bits = (char *)malloc(buf_len);
	/* get the bits */
	if ( ps_get_lrdb(ps_name, dev_name, bits, buf_len, &num_bits) != PS_OK) {
			fprintf(stderr,"dump_lrdb: ps_get_lrdb failed\n");
			fprintf(stderr," ps_name: %s dev_name: %s\n", ps_name, &dev_name[0]);
			return;
	}

	/* display bits */
	fprintf(stdout,"dump of LRT for ps_name: %s dev_name: %s\n",
	        ps_name, dev_name);
	cnt = 0;
	for(i=0; i < buf_len ; i += sizeof(unsigned int)){
		uintp = (unsigned int *)&bits[i];
		fprintf(stdout," 0x%08x", *uintp);
		cnt++;
		if ( (cnt % 4) == 0 ) 
			fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");

	free(bits);
	return;
}

static void
dump_hrdb(ps_name, dev_name)
	char *ps_name;
	char *dev_name;
{
	int i;
	int cnt;
	int buf_len=0;
	char *bits=(char *)0;
	unsigned int num_bits;
	unsigned int *uintp;

    buf_len = get_pstore_hrdb(ps_name, dev_name, &bits);
	/* first call in `descriptor' mode (buf_len == 0) */
	if ( ps_get_hrdb(ps_name, dev_name, bits, buf_len, &num_bits) != PS_OK) {
			fprintf(stderr,"dump_hrdb: ps_get_hrdb failed\n");
			fprintf(stderr," ps_name: %s dev_name: %s\n", ps_name, &dev_name[0]);
			return;
	}

    if (buf_len < 0)
    {
        return;
    }
	buf_len = num_bits/8;
	if( (bits = (char *)malloc(buf_len)) == 0 )
	{
			fprintf(stderr,"dump_hrdb: ps_get_hrdb failed due to malloc error\n");
			fprintf(stderr," ps_name: %s dev_name: %s\n", ps_name, &dev_name[0]);
			return;
	}
	/* get the bits */
	if ( ps_get_hrdb(ps_name, dev_name, bits, buf_len, &num_bits) != PS_OK) {
			fprintf(stderr,"dump_hrdb: ps_get_hrdb failed\n");
			fprintf(stderr," ps_name: %s dev_name: %s\n", ps_name, &dev_name[0]);
			return;
	}

	/* display bits */
	fprintf(stdout,"dump of HRT for ps_name: %s dev_name: %s\n",
	        ps_name, dev_name);
	cnt = 0;
	for(i=0; i < buf_len ; i += sizeof(unsigned int)){
		uintp = (unsigned int *)&bits[i];
		fprintf(stdout," 0x%08x", *uintp);
		cnt++;
		if ( (cnt % 4) == 0 ) 
			fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");

	free(bits);
	return;
}

static void
dump_psdev(lgnum, ps_name, Ps_diags, hrdb_type, Limited_Ps_diags, previous_ps_name)
	int lgnum; 
	char *ps_name;
    int Ps_diags;
	unsigned int hrdb_type;
    int Limited_Ps_diags;
	char *previous_ps_name;
{
	int i;
	int fd;
	int pathlen;
	ps_hdr_t ps_hdr;
	char *dev_name;
	ps_dev_entry_t *table;
	unsigned int table_size;
	ps_dev_info_t dev_info;
	ps_group_info_t group_info;
	char dev_name_buf[MAXPATHLEN];
	char group_info_name_buf[MAXPATHLEN];
	ps_dev_entry_t *dev_entry_p = NULL;
	int  tracking_resolution_level;
	int  supports_Proportional_HRDB;
	char sub_group_name[32];

	/* pathname scratch */
	dev_info.name = &dev_name_buf[0];
	group_info.name = &group_info_name_buf[0];
	FTD_CREATE_GROUP_NAME(group_info.name, lgnum);

	/* whip out the dev hdr */
	if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
		fprintf(stderr,"dump_psdev: open(%s, ...)  failed\n", ps_name);
		return;
	}
	lseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);
	read(fd, &ps_hdr, sizeof(ps_hdr_t));
    if( ps_hdr.magic != PS_VERSION_1_MAGIC )
	{
	    close( fd );
		fprintf(stderr,"dump_psdev: bad PS_VERSION_1_MAGIC: %s\n", ps_name);
	    return;
	}

    if( (supports_Proportional_HRDB = (ps_hdr.magic == PS_VERSION_1_MAGIC)) )
	{
        tracking_resolution_level = ps_hdr.data.ver1.tracking_resolution_level;
	}
	else
	{
        tracking_resolution_level = -1;	// If pre-RFX2.7.1 Pstore, we cannot have Proportional HRDB in it
	}

	// Dump pstore header info if option -d or -s was given
    if( Ps_diags )
    {
	    if( strcmp( ps_name, previous_ps_name ) != 0 )
		{
		    // Dump the Pstore header if we have not done so already for the previous group(s) (PROD8678)
            dump_Pstore_header_info( ps_name, &ps_hdr );
		}
	}

	/* whip out the group info */
	if(ps_get_group_info(ps_name, &group_info.name[0], &group_info) != PS_OK) {
			fprintf(stderr,"dump_psdev: ps_get_group_info failed\n");
			fprintf(stderr," ps_name: %s group_name: %s\n", ps_name, 
			        &group_info.name[0]);
			return;
	}

        if (!Ps_diags) {
            /*
             * display only minimum info and exit
             */
     	    fprintf(stdout,"    Checkpoint State................ %s\n",
                    (group_info.checkpoint) ? "On" : "Off");
            return;
        }

	dump_groupinfo(&group_info.name[0], &group_info);

	/* whip out the device table */
	table_size = sizeof(ps_dev_entry_t) * ps_hdr.data.ver1.max_dev;
	if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
		close(fd);
		fprintf(stderr,"dump_psdev: malloc: %s\n", ps_name);
		return ;
	}
	if (lseek(fd, ps_hdr.data.ver1.dev_table_offset*1024, SEEK_SET) == -1){
		free(table);
		close(fd);
		fprintf(stderr,"dump_psdev: lseek: %s\n", ps_name);
		return;
	}
	if (read(fd, (caddr_t)table, table_size) != table_size) {
		free(table);
		close(fd);
		fprintf(stderr,"dump_psdev: read: %s\n", ps_name);
		return;
	}

    sprintf( sub_group_name, "/dev/dtc/lg%d/", lgnum ); // WR PROD8678

	/* dump each dev belonging to this group */
	for (i = 0; i < ps_hdr.data.ver1.max_dev; i++) {
		/* should always report devices with valid path length */ 
	    dev_entry_p = &table[i];
        if (dev_entry_p->pathlen == 0)
           continue;
		dev_name = table[i].path;
        if( strstr( dev_name, sub_group_name ) == NULL )
		{
		    // WR PROD8678: this device does not belong to this group; go to next device
			continue;
		}
		if( ps_get_device_info(ps_name, dev_name, &dev_info) != PS_OK ){
			fprintf(stderr,"dump_psdev: ps_get_device_info failed\n");
			fprintf(stderr," ps_name: %s dev_name: %s\n", ps_name, dev_name);
			free(table);
			close(fd);
			return;
		}
		fprintf(stdout, "\n");
		dump_devinfo(ps_name, dev_name, &dev_info, hrdb_type, tracking_resolution_level, supports_Proportional_HRDB);
		fprintf(stdout, "\n");
		if( !Limited_Ps_diags )
		{
		    dump_lrdb(ps_name, dev_name);
		    fprintf(stdout, "\n");
		    dump_hrdb(ps_name, dev_name);
		}
		
	}

	free(table);
	close(fd);
	return;

}

/**
 * Dumps a raw buffer to stdout.
 *
 * Meant to dump a bitmap and be redirected to a file or another program.
 *
 * Prints an error message to stderr if something goes wrong.
 *
 * @param buffer The buffer to dump to stdout.
 * @param length The length of the buffer.
 *
 * @return The length written or -1 in case of an error.
 * 
 */
static ssize_t dump_buffer_to_stdout(const char* buffer, size_t length)
{
    ssize_t bytes_written = 0;

    while( length > 0)
    {
        bytes_written = write(STDOUT_FILENO, buffer, length);

        if (bytes_written == -1)
        {
            perror("Cannot write buffer to stdout.");
            return -1;
        }

        length -= bytes_written;
        buffer += bytes_written;
    }
    return bytes_written;
}

/**
 * Obtains a bitmap from the driver.
 *
 * Prints an error message to stderr if something goes wrong.
 *
 * @pre The mysys global variable must have been set up and by GETCONFIGS() and opendevs().
 *
 * @param lgnum             The group number.
 * @param raw_dtc_dev       The name of the dtc device of which we want the bitmap.
 * @param bitmap_requested  The kind of bitmap requested.
 * @param bitmap[in]        A pointer (by reference) that will be set to the obtained bitmap.
 * @param bitmap[out]       A pointer (by reference) to newly allocated memory containing the bitmap.
 *                          It is the caller's responsibility to free this memory afterwards.
 *
 * @return The length of the bitmap, or -1 if the bitmap couldn't be obtained.
 */
static ftd_int32_t get_driver_bitmap(int lgnum, dev_t raw_dtc_dev, bitmap_request_type_t bitmap_requested, char** bitmap)
{
    stat_buffer_t sb = {0};
    ftd_dev_info_t *dev_info = NULL;
    ftd_int32_t bitmap_len = 0;
    int i;

    dev_info = (ftd_dev_info_t*)malloc(mysys->group->numsddisks * sizeof(ftd_dev_info_t));

    if (dev_info == 0)
    {
        perror("get_driver_bitmap: Could not allocate memory for the device info.");
        bitmap_len = -1;
        goto cleanup;
    }

    sb.addr = (ftd_uint64ptr_t)(unsigned long) dev_info;
    sb.lg_num = lgnum; 
    
    if (FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_DEVICES_INFO, &sb) < 0)
    {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO", strerror(errno));
        bitmap_len = -1;
        goto cleanup;
    }

    // Find matching device to obtain the size of its bitmap.
    for (i = 0; i < mysys->group->numsddisks; i++)
    {
        if (dev_info[i].cdev == raw_dtc_dev)
        {
            dirtybit_array_kern_t db = {0};
            int driver_request;
            ftd_int32_t bitmap_len32 = 0;
            
            if (bitmap_requested == REQUESTED_HIGH_RES_FROM_DRIVER)
            {
                driver_request =  FTD_GET_HRDBS;
                bitmap_len = dev_info[i].hrdb_numbits / 8;
                bitmap_len32 = dev_info[i].hrdbsize32;
            }
            else
            {
                // asume low res.
                driver_request =  FTD_GET_LRDBS;
                bitmap_len = dev_info[i].lrdb_numbits / 8;
                bitmap_len32 = dev_info[i].lrdbsize32;
            }

            *bitmap = malloc(bitmap_len32 * sizeof(ftd_int32_t));

            if (*bitmap == 0)
            {
                perror("get_driver_bitmap: Could not allocate memory for the bitmap.");
                bitmap_len = -1;
                goto cleanup;
            }
            
            db.dev = (ftd_uint32_t) raw_dtc_dev;
            db.dblen32 = bitmap_len32;
            db.dbbuf = (ftd_uint64ptr_t)(unsigned long) *bitmap;
            db.state_change = 0;
            
            if (FTD_IOCTL_CALL(mysys->group->devfd, driver_request, &db) < 0)
            {
                free(*bitmap);
                *bitmap = 0;
                reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_HRDBS", strerror(errno));
                bitmap_len = -1;
                goto cleanup;
            }
            break;
        }
    }

    if (bitmap_len == 0)
    {
        fprintf(stderr, "get_driver_bitmap: Could not find device 0x%lx within group %s\n",
                (long)raw_dtc_dev, mysys->group->devname);
        bitmap_len = -1;
    }
    
cleanup:
    free(dev_info);
    
    return bitmap_len;
}

/**
 * Dumps a bitmap to stdout
 *
 * The bitmap is meant to be redirected to a file or another program.
 * Prints an error message to stderr if something goes wrong.
 *
 * @pre The mysys global variable has been set up and by GETCONFIGS().
 *
 * @param lgnum              The group number.
 * @param source_device_name The path of the source device which we want to get the bitmap of (either block or char device).
 * @param bitmap_requested   The kind of bitmap we're interested in.
 * @param ps_name            The path to the pstore.
 *
 * @return The length of the bitmap dumped or -1 in case of an error.
 */
int dump_bitmap_to_stdout(int lgnum, char* source_device_name, bitmap_request_type_t bitmap_requested, char* ps_name)
{
    sddisk_t *sd = NULL;
    char devname[MAXPATH] = {0x0};
    char raw_dtc_dev_name[MAXPATH] = {0x0};
    dev_t raw_dtc_dev = 0;
    ftd_int32_t bitmap_len = -1;
    char* bitmap = NULL;

    if (bitmap_requested == REQUESTED_HIGH_RES_FROM_DRIVER ||
        bitmap_requested == REQUESTED_LOW_RES_FROM_DRIVER)
    {
        if (opendevs(lgnum) != 0)
        {
            return -1;
        }
    }

    force_dsk_or_rdsk(devname, source_device_name, 1);

    for (sd = mysys->group->headsddisk; sd; sd = sd->n)
    {
        if(strcmp(devname, sd->devname) == 0)
        {
            raw_dtc_dev = sd->sd_rdev;
            strcpy(raw_dtc_dev_name, sd->sddevname);
            break;
        }
    }
    
    if (strlen(raw_dtc_dev_name) == 0)
    {
        fprintf(stderr, "Device %s not found in group %d.\n", source_device_name, lgnum);
        goto cleanup;
    }
    
    switch(bitmap_requested)
    {
        case REQUESTED_HIGH_RES_FROM_DRIVER:
        case REQUESTED_LOW_RES_FROM_DRIVER:
        {
            bitmap_len = get_driver_bitmap(lgnum, raw_dtc_dev, bitmap_requested, &bitmap);
            break;
        }
        case REQUESTED_HIGH_RES_FROM_PSTORE:
        {
            bitmap_len = get_pstore_hrdb(ps_name, raw_dtc_dev_name, &bitmap);
            break;
        }
        case REQUESTED_LOW_RES_FROM_PSTORE:
        {
            bitmap_len = get_pstore_lrdb(ps_name, raw_dtc_dev_name, &bitmap);
            break;
        }
        default:
        {
            fprintf(stderr, "Unknown kind of bitmap requested: %d.\n", bitmap_requested);
            break;
        }
    }

    // Errors from failing to obtain the device will already have been reported by the helper functions.
    if (bitmap_len > 0)
    {
        bitmap_len = dump_buffer_to_stdout(bitmap, bitmap_len);
    }

cleanup:
    free(bitmap); 
   
    closedevs(lgnum);
    return bitmap_len;
}
