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
#include "pathnames.h"
#include "config.h"
#include "ftdio.h"
#include "errors.h"
#include "ps_intr.h"
#include "cfg_intr.h"
#include "ftd_cmd.h"

#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <stropts.h> /* for HP-UX ioctl */

static void printshutdownstate(ps_group_info_t *ginfo);
static void printdriver(ftd_stat_t Info);
static void printlocaldisk(disk_stats_t DiskInfo, sddisk_t *);
static void printbab(disk_stats_t DiskInfo, sddisk_t *);
static void printother(ftd_stat_t Info);
static void printpmdmode(char *ps_name, char *group_name, int group);
static void printbabinfo(int);
static void dump_psdev(int lgnum, char *ps_name);

#define TRUE    1
#define FALSE   0

FILE    *dbgfd;

extern machine_t* mysys;
extern machine_t* othersys;

char *argv0;

/* This is how the program is used */
static void
usage(void)
{
    fprintf(stderr,"Usage: %s <options>\n\n", argv0);
    fprintf(stderr,"\
    One of the following three options is mandatory:\n\
        -g <logical_group_num> : Display info for a logical group\n\
        -a                     : Display info for all groups\n\
        -v                     : Display Version info\n\
        -h                     : Print This Listing\n");
    exit(1);
}

static char  paths[512][32];
static char version[512];

int main(int argc, char **argv)
{
    char           buf[MAXPATHLEN];
    char           ps_name[MAXPATHLEN];
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
    int            lgnum = 0;
    unsigned long  DisplayAll = 0;
    unsigned long  Other = 0;
    unsigned long  Ps_diags = 0;
    char           lgstr[4];

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    argv0 = argv[0];
 
    strcpy(version, "Softek %PRODUCTNAME% Version %REV% ");
    strcat(version, "%FIX%");
    strcat(version, "%VSUFF%"); 
    strcat(version, BLDSEQNUM);

    /* Determine what options were specified */
    opterr = 0;
    while ((ch =getopt(argc, argv, "vahog:d")) != -1) {
        switch (ch) {
        case 'a':
            /* Change the operation mode of all devices in all Logical Groups */
            DisplayAll = 1;
            break;
        case 'g':
            /* Change the operation mode of all devices in a Logical Group */
            LogicalGroup = 1;
            lgnum = strtol(optarg, NULL, 0);
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
        case 'h':
        default:
            usage();
            break;
        }
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


    /* open the master ftd device */
    if ((master = open(FTD_CTLDEV, O_RDWR)) < 0) {
        fprintf(stderr,"Error: Failed to open " PRODUCTNAME " master device: %s.  \nHas " QNM " driver been added?\n", FTD_CTLDEV);
        exit(1);
    }


    /* print out the bab data */
    if (!Ps_diags )
        printbabinfo(master);

    initconfigs();
    if (Ps_diags)
        NumGroups = getconfigs(paths, 1, 0);
    else
        NumGroups = getconfigs(paths, 1, 1);
    /* Make sure that there is at least 1 Logical Group Defined */
    if (!Ps_diags && NumGroups == 0) {
        fprintf(stderr,"       No Logical Groups have been started.\n");
        exit(1);
    }

    /* For each logical group... */
    for (GroupNum = 0; GroupNum < NumGroups; GroupNum++) {

        if (LogicalGroup) {
            if (Ps_diags)
                sprintf(buf, "p%03d.cfg", lgnum);
            else
                sprintf(buf, "p%03d.cur", lgnum);
            if (strcmp(buf, paths[GroupNum]) != 0)
                continue;
        }
        readconfig(1, 0, 0, paths[GroupNum]);
        strcpy (ps_name, sys[0].pstore);
        strncpy(lgstr, paths[GroupNum]+1, 3);
        lgstr[3] = 0;
        lg = atol(lgstr);
        printf("\nLogical Group %ld (%s -> %s)\n", lg, mysys->name,
            othersys->name);
       
        /* get the state from the pstore */
        FTD_CREATE_GROUP_NAME(group_name, (int) lg);

        memset(&ginfo, 0, sizeof(ginfo));
        if ((err = ps_get_group_info(ps_name, group_name, &ginfo)) != PS_OK) {
            if (err == PS_GROUP_NOT_FOUND) {
                fprintf(stderr,"group doesn't exist in pstore: %d\n", err);
            } else {
                fprintf(stderr,"PSTORE error: %d", err);
            }
            fprintf(stderr,"%s, %s\n", ps_name, group_name);
            exit(1);
        }


        /* diags */
        if (Ps_diags) {
            dump_psdev(lgnum, ps_name);
            ++DevicesDisplayed;
            continue;
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
        statbuf.addr = (ftd_uint64ptr_t)&Info;


        /* Get the info from the Device */
        if((err = ioctl(master, FTD_GET_GROUP_STATS, &statbuf)) < 0) {
            fprintf(stderr,"FTD_GET_GROUP_STATS ioctl: error = %d\n", errno);
            exit(1);
        }

        ++DevicesDisplayed;

        /* print logical group info */
        printdriver(Info);

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
            statbuf.addr = (ftd_uint64ptr_t)&DiskInfo;

            /* Get the info from the Device */
            if((err = ioctl(master, FTD_GET_DEVICE_STATS, &statbuf)) < 0) {
                fprintf(stderr,"FTD_GET_DEVICE_STATS ioctl: error = %d\n", errno);
                exit(1);
            }

            printlocaldisk(DiskInfo, sdp);
            printbab(DiskInfo, sdp);

            sdp = sdp->n;
            i++;
        }
    }
    close(master);

    if ( DevicesDisplayed == 0) {
        fprintf(stderr,"Failure: No info could be displayed for any device.\n");
        fprintf(stderr,"     Either the group(s) have not been started or\n");
        fprintf(stderr,"     the group number(s) specified with \"-g\" is incorrect.\n");
        exit(1);
    }
    exit(0);
}

static void
printbabinfo(int master)
{
    char buf[MAXPATHLEN];
    int num_chunks, chunk_size, size, actual;

    /* get bab size from driver */
    ioctl(master, FTD_GET_BAB_SIZE, &actual);

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
    printf("Actual BAB size ................... %d (~ %d MB)\n", actual, actual >> 20);


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

    printf("\n    Mode of operations.............. %s\n", state);
    printf("    Entries in the BAB.............. %lu\n", Info.wlentries);
    printf("    Sectors in the BAB.............. %lu\n", Info.wlsectors);
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

    printf("\n    Mode of operations.............. %s\n", state);
    printf("    Shutdown state ................. %u\n", ginfo->shutdown);
}

static void
printlocaldisk(disk_stats_t DiskInfo, sddisk_t *sdp)
{
    printf("\n        Local disk device number........ 0x%x\n", (int) DiskInfo.localbdisk);
    printf("        Local disk size (sectors)....... %lu\n", DiskInfo.localdisksize);
    printf("        Local disk name................. %s\n", sdp->devname);
    printf("        Remote mirror disk.............. %s:%s\n", 
        othersys->name, sdp->mirname);
}

static void 
printbab(disk_stats_t DiskInfo, sddisk_t *sdp)
{
    printf("        Read I/O count.................. %llu\n", DiskInfo.readiocnt);
    printf("        Total # of sectors read......... %llu\n", DiskInfo.sectorsread);
    printf("        Write I/O count................. %llu\n", DiskInfo.writeiocnt);
    printf("        Total # of sectors written...... %llu\n", DiskInfo.sectorswritten);
    printf("        Entries in the BAB.............. %lu\n", DiskInfo.wlentries);
    printf("        Sectors in the BAB.............. %lu\n", DiskInfo.wlsectors);
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
    printf("    Load time system ticks........... %lu\n", Info.loadtimesystics);

    printf("   Used (calc): %d Free (calc): %d\n", Info.bab_used, Info.bab_free);

}

#include "ps_pvt.h"

/*-
 * cough up pstore states...
 */
static void
dump_devinfo(ps_name, dev_name, dev_info)
	char *ps_name; 
	char *dev_name;
	ps_dev_info_t *dev_info;
{

	/* poot forth a dev_info struct */
	fprintf(stdout,"\n");
	fprintf(stdout,"ps_dev_info_t for dev %s:\n", dev_name);
	fprintf(stdout,"struct _ps_dev_info {\n");
	fprintf(stdout,"    char                 *name = %s\n", 
        	dev_info->name);
	fprintf(stdout,"    int                  state = 0x%08x\n", 
        	dev_info->state);
	fprintf(stdout,"    unsigned int num_lrdb_bits = 0x%08x\n", 
        	dev_info->num_lrdb_bits);
	fprintf(stdout,"    unsigned int num_hrdb_bits = 0x%08x\n", 
        	dev_info->num_hrdb_bits);
	fprintf(stdout,"    unsigned int num_sectors   = 0x%08x\n", 
        	dev_info->num_sectors);
	fprintf(stdout,"} ps_dev_info_t;\n");

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
	fprintf(stdout,"    int             state = 0x%08x\n", group_info->state);
	fprintf(stdout,"    unsigned int   hostid = 0x%08x\n", group_info->hostid);
	fprintf(stdout,"    int          shutdown = 0x%08x\n", group_info->shutdown);
	fprintf(stdout,"} ps_group_info_t;\n");

	return;
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

	/* first call in `descriptor' mode */
	if ( ps_get_lrdb(ps_name, dev_name, bits, buf_len, &num_bits) != PS_OK) {
			fprintf(stderr,"dump_psdev: ps_get_lrdb failed\n");
			fprintf(stderr," ps_name: %s dev_name: %s\n", ps_name, &dev_name[0]);
			return;
	}

	buf_len = num_bits/8;
	bits = (char *)malloc(buf_len);
	/* get the bits */
	if ( ps_get_lrdb(ps_name, dev_name, bits, buf_len, &num_bits) != PS_OK) {
			fprintf(stderr,"dump_psdev: ps_get_lrdb failed\n");
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

	/* first call in `descriptor' mode */
	if ( ps_get_hrdb(ps_name, dev_name, bits, buf_len, &num_bits) != PS_OK) {
			fprintf(stderr,"dump_psdev: ps_get_hrdb failed\n");
			fprintf(stderr," ps_name: %s dev_name: %s\n", ps_name, &dev_name[0]);
			return;
	}

	buf_len = num_bits/8;
	bits = (char *)malloc(buf_len);
	/* get the bits */
	if ( ps_get_hrdb(ps_name, dev_name, bits, buf_len, &num_bits) != PS_OK) {
			fprintf(stderr,"dump_psdev: ps_get_hrdb failed\n");
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
dump_psdev(lgnum, ps_name)
	int lgnum; 
	char *ps_name;
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
	if (ps_hdr.magic != PS_VERSION_1_MAGIC) {
		fprintf(stderr,"dump_psdev: bad PS_VERSION_1_MAGIC: %s\n", 
		        ps_name);
		return;
	}

	/* whip out the group info */
	if(ps_get_group_info(ps_name, &group_info.name[0], &group_info) != PS_OK) {
			fprintf(stderr,"dump_psdev: ps_get_group_info failed\n");
			fprintf(stderr," ps_name: %s group_name: %s\n", ps_name, 
			        &group_info.name[0]);
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


	/* dump each dev */
	for (i = 0; i < ps_hdr.data.ver1.max_dev; i++) {
		dev_name = table[i].path;
		if( ps_get_device_info(ps_name, dev_name, &dev_info) != PS_OK ){
			fprintf(stderr,"dump_psdev: ps_get_device_info failed\n");
			fprintf(stderr," ps_name: %s dev_name: %s\n", ps_name, dev_name);
			free(table);
			close(fd);
			return;
		}
		fprintf(stdout, "\n");
		dump_devinfo(ps_name, dev_name, &dev_info);
		fprintf(stdout, "\n");
		dump_lrdb(ps_name, dev_name);
		fprintf(stdout, "\n");
		dump_hrdb(ps_name, dev_name);
		
	}

	free(table);
	close(fd);
	return;

}

