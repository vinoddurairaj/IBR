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
/*****************************************************************************
 *
 * dtcchfs
 *
 * Copyright (c) 2007 Softek Storage Solutions, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * Command Description:
 * 
 * This command acts a wrapper for the AIX chfs command, with additional
 * functionality to protect against growing a file system on top of a dtc 
 * device which has not been expanded to the size of an underlying volume.
 *
 * Author: Brad Musolff 
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "ftd_mount.h"
#include "common.h"
#include "config.h"


extern machine_t* mysys;
extern machine_t* othersys;

#define CHFSPATH "/usr/sbin/chfs.aix.orig"
#define CHFSEXEC "chfs"

/*
 * mounted_dtc 
 *
 * Check if the passed in file system is a mounted dtc device, 
 * if so, set the lgnum and dtcnum for the device
 *
 * Returns: 1 if the mount point is a mounted dtc device
 *          0 otherwise
 *
 */  
int mounted_dtc(char * mountpoint, int* lgnum, int* dtcnum)
{
    FILE *fp = (FILE *) NULL;
    struct mnttab mp;
    struct mnttab mpref;
    int rc = 0;

    mpref.mnt_special = (char*) NULL;
    mpref.mnt_mountp = mountpoint;
    mpref.mnt_fstype = (char*) NULL;
    mpref.mnt_mntopts = (char*) NULL;
    mpref.mnt_time = (char*) NULL;

    /* check if this file system is mounted */
    if (0 == getmntany (fp, &mp, &mpref)) {

        if ((char*)NULL != mp.mnt_special && 0 < strlen (mp.mnt_special)) {

           /* check if the mounted device looks like one of ours, best would 
              be to check if the device is *really* ours, but for now we'll 
              bet that this dev name format is unique */
           if (sscanf(mp.mnt_special, "/dev/lg%ddtc%d", lgnum, dtcnum)==2) {
               rc = 1;
           }
        }
    }
          
    return (rc);
}

/*
 * getlocaldisk 
 *
 * Given a dtc device name, return the local data device for the
 * dtc device if it is found in any of the config files
 *
 * Returns: 0 if the dtc device is found
 *          1 otherwise
 *
 */  
int getlocaldisk(char * devname, char * localdevname)
{
    static char paths[MAXLG][32];
    int i;
    sddisk_t* sdp; 
    int rc = 1;
    int lgcnt = GETCONFIGS(paths, 1, 0);

    for (i = 0; i < lgcnt; i++) {
        readconfig(1, 0, 0, paths[i]);

        /* Pointer to the first device in this Logical Group */
        sdp = mysys->group[0].headsddisk;

        /* While there are Disks left in this Logical Group...*/
        while (sdp != NULL) {
            if (strcmp(sdp->sddevname, devname) == 0) {
               strcpy(localdevname, sdp->devname);
               rc = 0;
            }
            sdp = sdp->n;
        }
     }

     return (rc);

}
 
/*
 * dev_sizes_match 
 *
 * Determine if a dtc device is the same size as its underlying 
 * volume, given a dtc device number and logical group number
 *
 * Returns: 1 if the size match
 *          1 otherwise
 *
 */  
int dev_sizes_match(int lgnum, int dtcnum, char *localdevstr)
{ 
   u_longlong_t size;
   u_longlong_t localsize;
   char devstr[255];

   /* construct the device name */
   sprintf(devstr,"/dev/dtc/lg%d/rdsk/dtc%d", lgnum, dtcnum);
   
   /* get the dtc device size */ 
   size = disksize(devstr);

   /* get the name of the local data device for this dtc device */
   if (getlocaldisk(devstr, localdevstr) != 0) {
      return (1);
   }

   /* get the local device size */
   localsize = disksize(localdevstr);

   /* compare */
   if (localsize == size) {
       return (1);
   } else { 
       return (0);
   }
}

/* 
 * 
 *  Main program
 *
 *  Check if the  mount point passed to chfs has a dtc device mounted on it.
 *  If so, verify that the dtc device is at least as big as the underlying 
 *  volume (local data device). If not, print an error message and exit. If 
 *  the device sizes match, exec the original AIX chfs command.
 *   
 */
int main (int argc, char * argv[])
{
    int lgnum, dtcnum;
    char localdevstr[255];
    char argument[255];
    int ch;
    int sizeset=0;

    /* get command line argument to see if we are changing the file system size */
    /* We make sure that no parsing errors are ever reported, as we just want to see if the -a option has been given for the size attribute. */
    opterr = 0;    
    /* WARNING: The AIX version of getopt doesn't mention anything about the possibility of permutating the contents of argv, */
    /*          but other systems do, so this might be a possible problem depending on the options given to chfs and AIX's implementation. */
    while ((ch = getopt(argc, argv, "a:")) != -1) {
       switch(ch) {
       case 'a':
           /* chfs can accept a "Size" or "size" argument to change 
              the file system size (for JFS or JFS2 respectively */
           if ((strncmp(optarg, "Size", 4) == 0) || 
               (strncmp(optarg, "size", 4) == 0)) {
              sizeset=1;
           }
           break;
       }
    }

    /* if the file system is on top of a dtc device, and the device size
       does not match the underlying volume size, then an expansion 
       needs to be done first. */
    if (sizeset && mounted_dtc(argv[argc-1], &lgnum, &dtcnum) &&
        (!dev_sizes_match(lgnum, dtcnum, localdevstr))) {
           fprintf(stderr, "\nThe size of /dev/lg%ddtc%d does not match "  
                           "the size of the\nunderlying volume (%s).\n\n", 
                           lgnum, dtcnum, localdevstr);
           fprintf(stderr, "First run dtcexpand to expand the dtc" 
                           " device, then run chfs.\n");
           exit(1);
    } else {
        /* For any other case, simply execute the original chfs command */
        strcpy (argv[0], CHFSEXEC); 
        if (execv(CHFSPATH, argv) < 0) {
            exit (1);
        }
    }
}
               
