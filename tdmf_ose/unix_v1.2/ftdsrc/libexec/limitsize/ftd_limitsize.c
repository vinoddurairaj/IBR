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
 * dtclimitsize
 * 
 * Copyright (c) 2006 Softek Storage Solutions, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * Command Description:
 *
 * This command provides an interface for setting a 'DISK-LIMIT-SIZE' parameter
 * in the pXXX.cfg files.  This size is a multiplier that indicates an upper limit 
 * on the size that a device can be expanded to while remaining on line. 
 * The dtcstart command sets the device size in the driver to be 
 * (DISK-LIMIT-SIZE)  * (actual size).  This allows future growth of the
 * underlying disk device without re-calculation of bitmaps and device sizes on the
 * fly by the driver. 
 *
 * Author: Brad Musolff - re-write of Nagoya code 9/2006
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
#include <sys/stat.h>

#include "aixcmn.h"
#include "pathnames.h"
#include "errors.h"
#include "common.h"
#include "config.h"
#include "devcheck.h"
#include "ps_intr.h"
#include <fnmatch.h> 

/* -- structure used for reading and parsing a line from the config file */

#define DEV_KEYWORD  CAPQ"-DEVICE:"
#define LIMIT_KEYWORD "DISK-LIMIT-SIZE:"
#define PRO_KEYWORD  "PROFILE:"
#define DISK_KEYWORD "DATA-DISK:"
#define PSTORE_KEYWORD "PSTORE:"
#define TRUE  1
#define FALSE 0
#define PROFILE_BUF_SIZE 1024
#define LINE_LENGTH 4096
#define BUFSIZE LINE_LENGTH*10

static char *progname = NULL;
typedef struct _line_state {
    char key[LINE_LENGTH];
    char val[LINE_LENGTH];
    char line[LINE_LENGTH];
} LineState;

/*
 * usage 
 * 
 * print a usage message
 *
 */
static void
usage(void)
{
    fprintf(stderr, "Usage: %s -g <group#> [ -d <device path> ] -s <device size multiplier>\n", progname);
    fprintf(stderr, "\t<group#> is " GROUPNAME " group number. (0 - %d)\n", MAXLG-1);
    fprintf(stderr, "\t<device path> is the path of the replicated device. The path can be the source device name or the virtual dtc device name in the form /dev/"QNM"/lg#/rdsk/"QNM"#\n");
    fprintf(stderr, "\t<device size mulplier> is the maximum size to which the device may be expanded,\n");
    fprintf(stderr, "\twithout re-starting groups.  This number is expressed as a multiple of the current\n");
    fprintf(stderr, "\tsize. (i.e. -s 2 will allow up to 2X online expansion)\n");
}

/* 
 * read_line
 *
 * reads a line from an input file into a LineState struct
 *
 * Returns: TRUE (1) if something is read
 *          FALSE(0) if we hit end of file
 */
static int
read_line (FILE *fd, LineState *line)
{
    if (fgets(line->line, sizeof(line->line), fd) == NULL)
    {
        return FALSE;
    }

    sscanf(line->line, "%s%s", line->key, line->val);

    return TRUE;
}

/*
 * write_line
 *
 * writes a line to an output file
 *
 * Returns: TRUE (1) on success
 *          FALSE (0) on failure
 */
static int
write_line (FILE *fp, LineState *line)
{
    return ((int) fputs(line->line, fp));
}

/* 
 * get_line 
 * 
 * reads a line into a LineState struct from a
 * buffer
 *
 * Returns: char ptr to next line in buffer
 *
 */
static char * 
get_line (char * buf, LineState *line)
{
    char linebuf[LINE_LENGTH];
    char * pos;
    
    memset(line, 0, sizeof(LineState));

    memset(linebuf,0, LINE_LENGTH);
  
    pos = strchr(buf, '\n'); 
    if (pos) {
       pos++;
    } else {
       return (0);
    }
    memcpy(line->line, buf, (int) pos-(int)buf);
    sscanf(line->line, "%s%s", line->key, line->val);
    return (pos);
}

/*
 * copy_header_lines
 *
 * copies the initial .cfg file lines into the temp file
 * stops when it hits the first occurance of "PROFILE:"
 *
 */
void copy_header_lines(FILE *infile, FILE *outfile, char *ps_name)
{
   LineState line;
   do {
       read_line(infile, &line);
       write_line(outfile, &line);
	   if( strcmp( PSTORE_KEYWORD, line.key ) == 0 )
	   {
	       strcpy( ps_name, line.val );
	   }
   } while (strcmp(line.key, PRO_KEYWORD)!=0);
}

/* read_device_stanza
 * 
 * reads a device stanza into a buffer
 *
 * Returns: TRUE (1) if something was read into the buffer
 *          FALSE (0) if nothing was read
 */
int read_device_stanza (FILE *infile, FILE *outfile, char * buffer)
{
    LineState line;
    int linelen;
    char *bufp = buffer;
    int done = 0;
    int eof = 0;
    int read_something = 0;
   
    memset(&line, 0, sizeof(LineState));

    while (!done && !eof) {
        if (!read_line(infile, &line)) {
            eof = 1;
        } else { 
            read_something = 1;
            linelen = strlen(line.line);
            memcpy (bufp, line.line, strlen(line.line));
            bufp=bufp+linelen;
            if (strcmp(line.key, PRO_KEYWORD)==0) {
                done = 1;
            }
        }
    } 
    if ((eof == 1)  && !read_something)
       return (FALSE);
    bufp[0] = '\0';

    return (TRUE);
}

/* stanza_match_device
 *
 * check whether a particular device exists in stanza 
 *
 * Returns:  TRUE (1) if device appears in stanza
 *           FALSE (0) otherwise
 */ 
int stanza_match_device(char *buffer, char *devname)
{
    char raw_devname[MAXPATHLEN];
    char * bufp;
    int len = 0;

    /* changes into a raw_devname */
    force_dsk_or_rdsk(raw_devname, devname, 1);

    if (bufp = strstr(buffer, raw_devname))  
    { 
         /* make sure this is really the right device and not just a substring
          */
        do 
        {
            len++; 
            bufp++;
        } 
        while (*bufp != '\n' && *bufp != '\0' && *bufp != ' ' && *bufp != '\t');

        if (len != strlen(raw_devname)) 
        {
              return (FALSE);
        }
        return (TRUE);
    } 
    else 
    {
         return (FALSE);
    }
}

/* modify_stanza_disk_size
 *
 * Given a buffer, modifies the existing DISK-LIMIT-SIZE parameter or
 * adds the parameter if none existed.  Results are place in newbuffer.
 *  
 */
int modify_stanza_disk_size(char * oldbuffer, char * newbuffer, int size, char *ps_name)
{
    char * bufp = oldbuffer;
    char * oldbufp;
    char disksizestr[LINE_LENGTH];
    LineState line;
	char *dtc_dev_name[MAXPATHLEN];
	int ret;
    
    memset(newbuffer, 0, BUFSIZE);
    memset(&line, 0, sizeof(LineState));
    sprintf(disksizestr, "  DISK-LIMIT-SIZE:   %d\n", size);
    while (bufp=get_line(bufp, &line))
    {
        if (strcmp(DISK_KEYWORD, line.key) == 0) 
        {
            strcat(newbuffer, line.line);
            strcat(newbuffer, disksizestr);
        } 
        else if (strcmp(LIMIT_KEYWORD, line.key) != 0)
        {
            strcat(newbuffer, line.line);
        }
		// Get the dtc device name to set the limitsize value in the Pstore also
		if( strcmp(DEV_KEYWORD, line.key) == 0)
		{
		    strcpy( dtc_dev_name, line.val );
            if( (ret = ps_set_device_limitsize_multiple(ps_name, dtc_dev_name, (unsigned long long)size)) != PS_OK )
			{
			    fprintf( stderr, "Error setting device %s limitsize factor in Pstore %s, error code = %d\n",
			             dtc_dev_name, ps_name, ret );
				return( -1 );
			}
			else
			{
			    fprintf( stderr, "Device %s limitsize factor has been set to %d in Pstore %s.\n", dtc_dev_name, size, ps_name );
			}
		}
    }
	return( 0 );
}

          
/* create_temp
 *
 * Conversion function that creates a temporary file with new DISK-LIMIT-SIZE parameters
 * set.  
 *  
 *  Parameters: devname - Device name to set DISK-LIMIT-SIZE for
 *              size - value for DISK-LIMIT-SIZE
 *              cfg_path - original .cfg file path (i.e. /etc/opt/SFTKdtc/pXXX.cfg)                           
 *              temp_cfg_path - temp .cfg file path
 *              setall - flag to indicate whether DISK-LIMIT-SIZE should be set for all
 *                       devices
 *  Returns:  0 on Success
 *            -1 on failure
 */ 
static int 
create_temp(char *devname, int size, char * cfg_path, char *temp_cfg_path, int setall)
{
    
   FILE *infile,*outfile;
   int modify;
   char buffer[BUFSIZE];
   char modbuffer[BUFSIZE];
   char * outbuf;
   int found_dev;
   char ps_name[MAXPATHLEN];

    /*open cfg_file*/
    infile = fopen (cfg_path, "r");
    if(infile == NULL)
    {
        fprintf(stderr, "Error opening %s.", cfg_path);
        return -1;
    }

    /*open org_file*/
    outfile = fopen(temp_cfg_path, "w");
    if(outfile == NULL)
    {
        fclose(infile);
        fprintf(stderr, "Error opening %s.", temp_cfg_path);
        return -1;
    }

    /* copy beginning of config file up to device Profiles */
    copy_header_lines(infile, outfile, ps_name);

    found_dev = 0;

    /* read device profiles, determine whether to update, update  
       if necessary */
    while ((read_device_stanza(infile, outfile, buffer))) {
         outbuf = buffer;
         if (!setall) {
             /* we're only setting DISK-LIMIT-SIZE for one device */
             if (stanza_match_device (buffer, devname)) 
             {
	        	if( modify_stanza_disk_size(buffer, modbuffer, size, ps_name) == 0 )
				{
                    found_dev = 1;
				}
                outbuf = modbuffer;
             }  
         } else {
            /* We're setting it for all devices */
             if( modify_stanza_disk_size(buffer, modbuffer, size, ps_name) == 0 )
			 {
                 found_dev = 1;
		     }
             outbuf = modbuffer;
         }
         /* write this device profile out */
         fputs(outbuf, outfile);
    } 

    fclose(infile);
    fclose(outfile);

    if (!found_dev)
    {
	    if( devname != NULL)
            fprintf(stderr, "Device %s not found or other error occurred.\n", devname);
		else
            fprintf(stderr, "Error occurred while processing into temporary file.\n");
        return (-1);
    }
    else 
    {
        return 0;
    }
}

/* copy_temp_to_cfg 
 *
 *  Copies the temp file to the .cfg file
 *
 *  Returns:  0 on Success
 *            -1 on failure
 */
static int
copy_temp_to_cfg(char * cfg_path, char *temp_cfg_path)
{

    FILE *infile,*outfile;
    char line[LINE_LENGTH];

    infile = fopen (temp_cfg_path, "r");
    if(infile == NULL)
    {
        fprintf(stderr, "Error opening %s.", temp_cfg_path);
        return -1;
    }

    outfile = fopen(cfg_path, "w");
    if(outfile == NULL)
    {
        fclose(infile);
        fprintf(stderr, "Error opening %s.", cfg_path);
        return -1;
    }

    /* copy the file */
    while (fgets(line, sizeof(line), infile)) {
        fputs(line, outfile);
    }

    fclose(infile);
    fclose(outfile);
 
    /* remove the temp file */
    unlink(temp_cfg_path);

    return 0;
}


/*
 * get_cmd_options
 *
 * Retreive command line args 
 * 
 * Returns 0 on success
 *        -1 on failure
 */
int get_cmd_options(int * group, char *devname, int *size, int *setall, int argc, char* argv[])
{
    int sub_size;
    int gflag, dflag, sflag;
    char buf[MAXPATHLEN];
    int ch;

    gflag = sflag = dflag = *setall = 0;

    /* process all of the command line options */
    while ((ch = getopt(argc, argv, "g:d:s:")) != EOF)
    {
        switch(ch)
        {
        case 'g':
            if (gflag)
            {
                fprintf(stderr, "only one -g option allowed.n");
                goto usage_error;
            }
            *group = ftd_strtol(optarg);
            if (*group < 0 || *group >= MAXLG)
            {
                fprintf(stderr, "Invalid number for " GROUPNAME " group\n");
                goto usage_error;
            }
            gflag++;
            break;
        case 'd':
            if (dflag)
            {
                fprintf(stderr, "only one -d option allowed.\n");
                goto usage_error;
            }
            strcpy (devname, optarg);
            dflag++;
            break;
        case 's':
            if (sflag)
            {
                fprintf(stderr, "only one -s option allowed.\n");
                goto usage_error;
            }
            sub_size = ftd_strtol(optarg);
            if(sub_size <= 0)
            {
                fprintf(stderr, "Size must be greater than zero.\n");
                return (-1);
            }
            if (sub_size > 20)
            {
                fprintf(stderr, "Size must be less than 20.\n");
                return (-1);
            }
            *size = (int)sub_size;
            sflag++;
            break;
        default:
            goto usage_error;
        }
   }
    log_command(argc,argv);

    /*check LG-Number dtc-device_name limit-size*/
    if (gflag != 1 ||  sflag != 1 || optind != argc )
    {
       goto usage_error;
    }
    if (dflag == 1 && devname[0] == '\0')
    {
       goto usage_error;
    }
    if (dflag == 1)
    {
       if ( ( fnmatch("/dev/dtc/lg[0-9]*/*dsk/dtc[0-9]*", devname, FNM_PATHNAME) == 0) || 
            ( strncmp( devname, "/dev/", 5 ) == 0) 
          )
       {
         /* we found a matching device type.*/
       }
       else
       {
           fprintf(stderr, "Device format incorrect.\n\n");
           goto usage_error;
       }
    }

    if (gflag && (!dflag)) {
       *setall = 1;
    }
    
    return(0);
usage_error:
    usage();
    return (-1);

}

/* modify_disk_limit_size
 *
 * Check that group files exist, then create a temp file with our changes,
 * then copy over original file
 *
 */
int modify_disk_limit_size(int group, char * devname, int size, int setall)
{
    char full_cfg_path[MAXPATHLEN];
    char full_cfg_path_temp[MAXPATHLEN];
    int add_line_count = 0;
    char buf[PROFILE_BUF_SIZE];

   /* create the cfg_path */
    sprintf(full_cfg_path, "%s/p%03d.cfg", PATH_CONFIG, group);
    sprintf(full_cfg_path_temp, "%s/p%03d.tmp", PATH_CONFIG, group);

    /*search for the cfg_file*/
    if((access(full_cfg_path, F_OK)) != 0)
    {
        /*error cfg_file not found*/
        fprintf(stderr, "Not found - %s\n", full_cfg_path);
        return (-1);
    }

    /* creat temp file that includes all of the changes to DISK-LIMIT-SIZE */
    if(create_temp(devname, size, full_cfg_path, full_cfg_path_temp, setall) != 0)
    {
        remove(full_cfg_path_temp);
        return(-1);
    }

    /* create cfg_file */
    if(copy_temp_to_cfg(full_cfg_path, full_cfg_path_temp) != 0)
    {
        return(-1);
    }
    fprintf(stderr, "Configuration file %s has been updated.\n", full_cfg_path);
}

/* main - 
 *  
 * Make sure we are root, then get command line arguments, then modify the .cfg file 
 *
 */
int main(int argc, char *argv[])
{
    int group;
    char devname[FILE_PATH_LEN];
    int size;
    int setall;
    char * argv0;

    putenv("LANG=C");

    argv0 = argv[0];

    /* for use by 'Usage' */
    progname = argv[0];

    devname[0] = '\0';

    /* Make sure we are root */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }

    if (initerrmgt(ERRFAC) != 0)
    {
       /* NO need to exit here because it causes HP-UX */
       /* replication devices' startup at boot-time to fail */
       /* PRN# 498       */
    }

    /* process cmd line arguments */
    if (get_cmd_options(&group, devname, &size, &setall, argc, argv) < 0) {
	exit(1);
    }

    /* add /modify disk limit size in file */
    if (modify_disk_limit_size(group, devname, size, setall) < 0) {
       exit (1);
    } 

    printf("Device expansion limit set successfully.\n");

    return 0;
}
