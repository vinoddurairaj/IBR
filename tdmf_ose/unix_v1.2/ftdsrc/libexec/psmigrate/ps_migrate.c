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
 * ps_migrate.c --> Migrates a pstore of older version to a pstore of latest version and vice versa.
 *
 * This utility converts all the old pstores that it finds to pstores of latest version.
 * This ensures that device and group information in the pstores are not lost after upgrading to 
 * a newer version of the product.
 * This utility can also convert a latest pstore version to an older pstore version.
 *
 */
 
// NOTE: this version is reverted to the source of revision 1.10.
// The reason is that we decided, for RFX2.7.1, to keep the previous behavior
// and keep ps_migrate functional for Pstore formats of RFX2.7.0 and earlier.
// This version cannot convert old Pstores to RFX2.7.1 Pstore with Proportional
// HRDB formats. For release RFX 2.7.1, customers who want to use the new Pstore Proportional HRDB feature
// must reinitialize their Pstores and perform the usual Full (or checksum) refresh.

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <signal.h>
#if !defined(linux)
#include <macros.h>
#endif /* !defined(linux) */
#include <sys/statvfs.h>
#include <dirent.h>

#include "ps_migrate.h"
#include "ps_pvt_rev1.9.RFX270.h" // For RFX 2.7.1 release, keep functionnality of psmigrate up to RFX 2.7.0
#include "errors.h"
#include "ftd_cmd.h"
#include "ftdio.h"
#include "pathnames.h"
#include "common.h"
#include "devcheck.h"
#include "aixcmn.h"

extern char *paths;
static char configpaths[MAXLG][32];
static char *progname = NULL;
int lastgrp, lastdev, version, is_exit;

#define MAX_VER_LENGTH 20

static void
usage(void)
{
    fprintf(stderr, "Usage: %s [-r version_number] [-h]\n", progname);
    fprintf(stderr, "\t If %s is run without any arguments then it will migrate an old pstore version to the pstore version\n", progname);
    fprintf(stderr, "\t of Replicator for Unix / TDMF IP for Unix release 2.7.0, with which release 2.7.1 is compatible in the\n");
    fprintf(stderr, "\t legacy Small and Large HRT modes (but not in Proportional HRDB mode).\n");
    fprintf(stderr, "\t -r option is used to restore the pstore to an old version specified by version_number\n");
    fprintf(stderr, "\t -h option displays this help\n");
    fprintf(stderr, "\t NOTE: if you want to migrate a pstore to the release 2.7.2 format, you can use the command dtcpsmigrate272\n");
    fprintf(stderr, "\t       on release 2.7.0 or 2.7.1 pstores; for older pstores, migrate them to the 2.7.0 format first, and then use dtcpsmigrate272 after.\n");
    exit(-1);
}

/* Signal handler for this utility
 */
void signal_handler(int sig)
{
    is_exit = 1;
}

void instsigaction(void)
{
    struct sigaction sact;
    sigset_t mask_set;

    sact.sa_handler = signal_handler;
    sact.sa_mask = mask_set;
    sact.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sact, NULL);
    sigaction(SIGTERM, &sact, NULL);
    sigaction(SIGHUP, &sact, NULL);
}

/*
 * returns the maximum number of devices a pstore can support
 *
 */
int ps_max_devices(char *ps_name, int *max_dev, int *num_dev, int *num_grp)
{
    unsigned long long  size, err;
    char                raw_name[MAXPATHLEN];
    u_longlong_t        dsize;
    int 		fd;
    ps_hdr_t 		hdr;

    /* convert the block device name to the raw device name */
#if defined(HPUX)
    if (is_logical_volume(ps_name)) {
        convert_lv_name(raw_name, ps_name, 1);
    } else {
        force_dsk_or_rdsk(raw_name, ps_name, 1);
    }
#else
    force_dsk_or_rdsk(raw_name, ps_name, 1);
#endif

    /* stat the pstore device and figure out the maximum number of devices */
    if ((dsize = disksize(raw_name)) < 0) {
        reporterr(ERRFAC, M_STAT, ERRCRIT, raw_name, strerror(errno));
        return -1;
    }

    /* the size of one device assuming one device per group */
    size = FTD_PS_LRDB_SIZE + FTD_PS_HRDB_SIZE_SMALL + FTD_PS_DEVICE_ATTR_SIZE +
           FTD_PS_GROUP_ATTR_SIZE + 3*MAXPATHLEN;

    /* Note: DEV_BSIZE is of different value on different OSes.
     * HPUX 1 KB, others 512 B
     */
    size = size / DEV_BSIZE;


#if defined(_AIX) || defined(linux)
    dsize = (daddr_t)((dsize - 32 -
                (FTD_PS_LRDB_SIZE+FTD_PS_HRDB_SIZE_SMALL)/DEV_BSIZE ) / size);
#else
    dsize = (daddr_t)((dsize - 32) / size);
#endif

    if (dsize > FTD_MAX_DEVICES) {
        dsize = FTD_MAX_DEVICES;
    }

    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);
    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    close(fd);

    *num_dev = hdr.data.ver1.num_device;
    *num_grp = hdr.data.ver1.num_group;
    *max_dev = dsize;

    return(0);
}

/*
 * returns the header information
 *
 */
int get_header(char *ps_name, ps_hdr_t *attr)
{
    int      fd;
    ps_hdr_t hdr;

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);
    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    close(fd);

    *attr = hdr;

    return PS_OK;
}

/*
 *  writes a header into the pstore or a temporary file depending on the value of ftrans
 *  ftrans = 1 : copy header as it is
 *  ftrans = 0 : copy header with modifications
 */
int set_header(char *ps_name, ps_hdr_t tmp, int ftrans)
{
    ps_hdr_t hdr;
    int fd;

    if (ftrans) {
        if (!access(ps_name, F_OK)) {
 	    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
	        return PS_BOGUS_PS_NAME;
	    }
	} else {
    	    if ((fd = open(ps_name, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
       		fprintf(stderr, "Could not create a temporary file to enable migration\n");
		exit(-1);
    	    }
	}
	hdr = tmp;
    } else {
    	if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        	return PS_BOGUS_PS_NAME;
    	}
    	if (llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET) == -1) {
        	close(fd);
        	return PS_SEEK_ERROR;
    	}
    	read(fd, &hdr, sizeof(ps_hdr_t));
    	hdr.data.ver1.num_device = tmp.data.ver1.num_device;
    	hdr.data.ver1.num_group = tmp.data.ver1.num_group;
    	hdr.data.ver1.last_device = lastdev;
    	hdr.data.ver1.last_group = lastgrp;
    }
    if (llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &hdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t)) {
        close(fd);
        return PS_WRITE_ERROR;
    }
    close(fd);
    return PS_OK;
}

/*
 * this function is used to copy device information.
 * information is copied from from_ps_name to to_ps_name.
 * this function only does incremental upgrade of device information (old --> latest).
 * switch statements are used to copy information based on the version
 * of the pstore. whenever the ps_dev_entry_t structure changes, add the
 * old structure definition (ps_dev_ver#_t)  in the header file lib/libgen/ps_migrate.h.
 * Add a case to the switch statements for the previous pstore version
 * and use ps_dev_ver#_t for copying device entries.
 */
int ps_copy_device_data(char *from_ps_name, char *to_ps_name, int ftrans)
{
    int            infd, outfd, i = 0, j, cpflag;
    ps_hdr_t       inhdr, outhdr;
    unsigned int   table_size, offset;
    ps_dev_ver1_t  *table_ver1 = NULL;
    ps_dev_ver3_t  *table_ver3 = NULL;
    ps_dev_entry_t *table = NULL, entry;
    char 	   *buffer;

    if ((infd = open(from_ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    if ((outfd = open(to_ps_name, O_RDWR | O_SYNC)) == -1) {
	close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET);
    llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET);
    /* read the header */
    read(infd, &inhdr, sizeof(ps_hdr_t));
    read(outfd, &outhdr, sizeof(ps_hdr_t));

    if (llseek(infd, inhdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
             goto devcleanup;
    }
    /* 
     * allocate memory as per the version of pstore
     */
    switch(version) {
  	case PS_OLD3_HEADER:
        	table_size = sizeof(ps_dev_ver3_t) * inhdr.data.ver1.max_dev;
        	table_ver3 = (ps_dev_ver3_t *)ftdcalloc(1, table_size);
        	if (read(infd, (caddr_t)table_ver3, table_size) != table_size) {
          		goto devcleanup;
        	}
     		break;
  	case PS_OLD2_HEADER:
	case PS_OLD1_HEADER:
    		table_size = sizeof(ps_dev_ver1_t) * inhdr.data.ver1.max_dev;
    		table_ver1 = (ps_dev_ver1_t *)ftdcalloc(1, table_size);
    		if (read(infd, (caddr_t)table_ver1, table_size) != table_size) {
			goto devcleanup;
    		}
		break;
  	case PS_OLD4_HEADER:
	default:
		table_size = sizeof(ps_dev_entry_t) * inhdr.data.ver1.max_dev;
		table = (ps_dev_entry_t *)ftdcalloc(1, table_size);
                if (read(infd, (caddr_t)table, table_size) != table_size) {
			goto devcleanup;
                }
    }
    j = 0;
    cpflag = 0;
    for (i = 0; i < inhdr.data.ver1.max_dev; i++) {
	switch(version) {
     	   case PS_OLD3_HEADER:
          	if ((strlen(table_ver3[i].path) != strspn(table_ver3[i].path," ")) && table_ver3[i].pathlen > 0) {
             	  	cpflag = 1;
          	}
        	break;
     	   case PS_OLD2_HEADER:
	   case PS_OLD1_HEADER:
        	if ((strlen(table_ver1[i].path) != strspn(table_ver1[i].path," ")) && table_ver1[i].pathlen > 0) {
			cpflag = 1;
		}
		break;
	   case PS_OLD4_HEADER:
	   default:
		if ((strlen(table[i].path) != strspn(table[i].path, " ")) && table[i].pathlen > 0) {
			cpflag = 1;
		}
	}
	if (!cpflag) {
		continue;
	}
	/*
	 * if ftrans is 1 then copy data as it is.
	 * if ftrans is 0 then copy data with the necessary modifications
	 */
	if (ftrans) {
	    switch(version) {
		case PS_OLD3_HEADER:
			offset = (outhdr.data.ver1.dev_table_offset * 1024) +
	            			(j * sizeof(ps_dev_ver3_t));
    			if (llseek(outfd, offset, SEEK_SET) == -1) {
               			goto devcleanup;
    			}
			if (write(outfd, &table_ver3[i], sizeof(ps_dev_ver3_t)) != sizeof(ps_dev_ver3_t)) {
               			goto devcleanup;
    			}
			break;
    		case PS_OLD2_HEADER:
		case PS_OLD1_HEADER:
			offset = (outhdr.data.ver1.dev_table_offset * 1024) +
	            			(j * sizeof(ps_dev_ver1_t));
    			if (llseek(outfd, offset, SEEK_SET) == -1) {
               			goto devcleanup;
    			}
			if (write(outfd, &table_ver1[i], sizeof(ps_dev_ver1_t)) != sizeof(ps_dev_ver1_t)) {
               			goto devcleanup;
    			}
			break;
		case PS_OLD4_HEADER:
		default:
			offset = (outhdr.data.ver1.dev_table_offset * 1024) +
                                         (j * sizeof(ps_dev_entry_t));
                        if (llseek(outfd, offset, SEEK_SET) == -1) {
                                goto devcleanup;
                        }
			if (write(outfd, &table[i], sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t)) {
                                goto devcleanup;
                        }
	    }
	} else {
		switch(version) {
			case PS_OLD3_HEADER:
					strcpy(entry.path, table_ver3[i].path);
					entry.pathlen = table_ver3[i].pathlen;
					entry.state = table_ver3[i].state;
					entry.num_lrdb_bits = table_ver3[i].num_lrdb_bits;
					entry.num_hrdb_bits = table_ver3[i].num_hrdb_bits;
					entry.num_sectors = table_ver3[i].num_sectors;
					entry.ackoff = table_ver3[i].ackoff;
					break;      
      case PS_OLD2_HEADER:
			case PS_OLD1_HEADER:
					strcpy(entry.path, table_ver1[i].path);
					entry.pathlen = table_ver1[i].pathlen;
					entry.state = table_ver1[i].state;
					entry.num_lrdb_bits = table_ver1[i].num_lrdb_bits;
					entry.num_hrdb_bits = table_ver1[i].num_hrdb_bits;
					entry.num_sectors = table_ver1[i].num_sectors;
					entry.ackoff = 0;
					break;
			case PS_OLD4_HEADER:
			default:
					entry = table[i];
		}
		offset = (outhdr.data.ver1.dev_table_offset * 1024) +
	               		(j * sizeof(ps_dev_entry_t));
    		if (llseek(outfd, offset, SEEK_SET) == -1) {
               		goto devcleanup;
    		}
		if (write(outfd, &entry, sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t)) {
               		goto devcleanup;
    		}
	}
	/*
	 * copy device attributes
	 */
    	buffer = ftdcalloc(1, inhdr.data.ver1.dev_attr_size);
    	offset = (inhdr.data.ver1.dev_attr_offset * 1024) +
           		(i * inhdr.data.ver1.dev_attr_size);
    	if (llseek(infd, offset, SEEK_SET) == -1) {
		free(buffer);
       		goto devcleanup;
    	}
    	if (read(infd, buffer, inhdr.data.ver1.dev_attr_size) != inhdr.data.ver1.dev_attr_size) {
                free(buffer);
       		goto devcleanup;
    	}
    	offset = (outhdr.data.ver1.dev_attr_offset * 1024) +
            		(j * outhdr.data.ver1.dev_attr_size);
    	if (llseek(outfd, offset, SEEK_SET) == -1) {
                free(buffer);
       		goto devcleanup;
    	}
    	if (write(outfd, buffer, outhdr.data.ver1.dev_attr_size) != outhdr.data.ver1.dev_attr_size) {
                free(buffer);
       		goto devcleanup;
    	}
	free(buffer);
        /*
         * copy LRDB
         */
	offset = (inhdr.data.ver1.lrdb_offset * 1024) +
                    (i * inhdr.data.ver1.lrdb_size);
        if (llseek(infd, offset, SEEK_SET) == -1) {
                goto devcleanup;
        }
        buffer = ftdcalloc(1, inhdr.data.ver1.lrdb_size);
        if (read(infd, buffer, inhdr.data.ver1.lrdb_size) != inhdr.data.ver1.lrdb_size) {
                free(buffer);
                goto devcleanup;
        }
        offset = (outhdr.data.ver1.lrdb_offset * 1024) +
                  (j * outhdr.data.ver1.lrdb_size);
        if (llseek(outfd, offset, SEEK_SET) == -1) {
                free(buffer);
                goto devcleanup;
        }
        if (write(outfd, buffer, outhdr.data.ver1.lrdb_size) != outhdr.data.ver1.lrdb_size) {
                free(buffer);
                goto devcleanup;
        }
        free(buffer);
        /*
         * copy HRDB
         */
        offset = (inhdr.data.ver1.hrdb_offset * 1024) +
                   (i * inhdr.data.ver1.hrdb_size);
        if (llseek(infd, offset, SEEK_SET) == -1) {
                goto devcleanup;
        }
        buffer = ftdcalloc(1, inhdr.data.ver1.hrdb_size);
        if (read(infd, buffer, inhdr.data.ver1.hrdb_size) != inhdr.data.ver1.hrdb_size) {
                free(buffer);
                goto devcleanup;
        }
        offset = (outhdr.data.ver1.hrdb_offset * 1024) +
                  (j * outhdr.data.ver1.hrdb_size);
        if (llseek(outfd, offset, SEEK_SET) == -1) {
                free(buffer);
                goto devcleanup;
        }
        if (write(outfd, buffer, outhdr.data.ver1.hrdb_size) != outhdr.data.ver1.hrdb_size) {
                free(buffer);
                goto devcleanup;
        }
        free(buffer);
	j++;
        cpflag = 0;
    }
    lastdev = j - 1;
devcleanup:
    switch(version) {
  	case PS_OLD3_HEADER:
       		free(table_ver3);
     		break;
  	case PS_OLD2_HEADER:
	case PS_OLD1_HEADER:
    		free(table_ver1);
		break;
	case PS_OLD4_HEADER:
	default:
		free(table);
    }
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_dev) {
    	return PS_OK;
    } else {
	return PS_ERR;
    }
}

/*
 * this function is used to copy device information.
 * information is copied from from_ps_name to to_ps_name.
 * this function only does downgrade of device information (latest --> old).
 * switch statements are used to copy information based on the version
 * of the pstore. whenever the ps_dev_entry_t structure changes, add the
 * old structure definition (ps_dev_ver#_t)  in the header file lib/libgen/ps_migrate.h.
 * Add a case to the switch statements for the previous pstore version
 * and use ps_dev_ver#_t for copying device entries.
 */
int ps_restore_device_data(char *from_ps_name, char *to_ps_name, int ftrans)
{
    int            infd, outfd, i = 0, j;
    ps_hdr_t       inhdr, outhdr;
    unsigned int   table_size, offset;
    ps_dev_ver1_t  entry_ver1;
    ps_dev_ver3_t  entry_ver3;
    ps_dev_entry_t *table = NULL;
    char 	   *buffer;

    if ((infd = open(from_ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    if ((outfd = open(to_ps_name, O_RDWR | O_SYNC)) == -1) {
	close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET);
    llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET);
    /* read the header */
    read(infd, &inhdr, sizeof(ps_hdr_t));
    read(outfd, &outhdr, sizeof(ps_hdr_t));

    if (llseek(infd, inhdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
             goto restore_devcleanup;
    }
    table_size = sizeof(ps_dev_entry_t) * inhdr.data.ver1.max_dev;
    table = (ps_dev_entry_t *)ftdcalloc(1, table_size);
    if (read(infd, (caddr_t)table, table_size) != table_size) {
	goto restore_devcleanup;
    }
    j = 0;
    for (i = 0; i < inhdr.data.ver1.max_dev; i++) {
	if ((strlen(table[i].path) != strspn(table[i].path, " ")) && table[i].pathlen > 0) {
        /*
         * if ftrans is 1 then copy data as it is.
         * if ftrans is 0 then copy data with the necessary modifications
         */
	if (ftrans) {
		offset = (outhdr.data.ver1.dev_table_offset * 1024) +
                                (j * sizeof(ps_dev_entry_t));
                if (llseek(outfd, offset, SEEK_SET) == -1) {
                      goto restore_devcleanup;
                }
		if (write(outfd, &table[i], sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t)) {
                      goto restore_devcleanup;
                }
	} else {
		switch(version) {
      			case PS_OLD3_HEADER:
				strcpy(entry_ver3.path, table[i].path);
				entry_ver3.pathlen = table[i].pathlen;
				entry_ver3.state = table[i].state;
				entry_ver3.num_lrdb_bits = table[i].num_lrdb_bits;
				entry_ver3.num_hrdb_bits = table[i].num_hrdb_bits;
				entry_ver3.num_sectors = table[i].num_sectors;
        			entry_ver3.ackoff = (off_t)table[i].ackoff;
			     	offset = (outhdr.data.ver1.dev_table_offset * 1024) +
			   			(j * sizeof(ps_dev_ver3_t));
			     	if (llseek(outfd, offset, SEEK_SET) == -1) {
                        		goto restore_devcleanup;
                	     	}
                	     	if (write(outfd, &entry_ver3, sizeof(ps_dev_ver3_t)) != sizeof(ps_dev_ver3_t)) {
                        		goto restore_devcleanup;
                	     	}
			     	break;      
      			case PS_OLD2_HEADER:
			case PS_OLD1_HEADER:
				strcpy(entry_ver1.path, table[i].path);
				entry_ver1.pathlen = table[i].pathlen;
				entry_ver1.state = table[i].state;
				entry_ver1.num_lrdb_bits = table[i].num_lrdb_bits;
				entry_ver1.num_hrdb_bits = table[i].num_hrdb_bits;
				entry_ver1.num_sectors = table[i].num_sectors;
			     	offset = (outhdr.data.ver1.dev_table_offset * 1024) +
			   			(j * sizeof(ps_dev_ver1_t));
			     	if (llseek(outfd, offset, SEEK_SET) == -1) {
                        		goto restore_devcleanup;
                	     	}
                	     	if (write(outfd, &entry_ver1, sizeof(ps_dev_ver1_t)) != sizeof(ps_dev_ver1_t)) {
                        		goto restore_devcleanup;
                	     	}
			     	break;
			case PS_OLD4_HEADER:
			default:
		                offset = (outhdr.data.ver1.dev_table_offset * 1024) +
                                	      (j * sizeof(ps_dev_entry_t));
                		if (llseek(outfd, offset, SEEK_SET) == -1) {
                        	    goto restore_devcleanup;
                		}
                		if (write(outfd, &table[i], sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t)) {
                        	    goto restore_devcleanup;
                		}
		}
	}
	/*
	 * copy device attributes
	 */
    	buffer = ftdcalloc(1, inhdr.data.ver1.dev_attr_size);
    	offset = (inhdr.data.ver1.dev_attr_offset * 1024) +
           		(i * inhdr.data.ver1.dev_attr_size);
    	if (llseek(infd, offset, SEEK_SET) == -1) {
		free(buffer);
       		goto restore_devcleanup;
    	}
    	if (read(infd, buffer, inhdr.data.ver1.dev_attr_size) != inhdr.data.ver1.dev_attr_size) {
                free(buffer);
       		goto restore_devcleanup;
    	}
    	offset = (outhdr.data.ver1.dev_attr_offset * 1024) +
            		(j * outhdr.data.ver1.dev_attr_size);
    	if (llseek(outfd, offset, SEEK_SET) == -1) {
                free(buffer);
       		goto restore_devcleanup;
    	}
    	if (write(outfd, buffer, outhdr.data.ver1.dev_attr_size) != outhdr.data.ver1.dev_attr_size) {
                free(buffer);
       		goto restore_devcleanup;
    	}
	free(buffer);
	/*
	 * copy LRDB
	 */
        offset = (inhdr.data.ver1.lrdb_offset * 1024) +
                     (i * inhdr.data.ver1.lrdb_size);
        if (llseek(infd, offset, SEEK_SET) == -1) {
                goto restore_devcleanup;
        }
        buffer = ftdcalloc(1, inhdr.data.ver1.lrdb_size);
        if (read(infd, buffer, inhdr.data.ver1.lrdb_size) != inhdr.data.ver1.lrdb_size) {
                free(buffer);
                goto restore_devcleanup;
        }
	offset = (outhdr.data.ver1.lrdb_offset * 1024) +
                   (j * outhdr.data.ver1.lrdb_size);
        if (llseek(outfd, offset, SEEK_SET) == -1) {
                free(buffer);
                goto restore_devcleanup;
        }
        if (write(outfd, buffer, outhdr.data.ver1.lrdb_size) != outhdr.data.ver1.lrdb_size) {
                free(buffer);
                goto restore_devcleanup;
        }
        free(buffer);
	/*
	 * copy HRDB
	 */
        offset = (inhdr.data.ver1.hrdb_offset * 1024) +
                      (i * inhdr.data.ver1.hrdb_size);
        if (llseek(infd, offset, SEEK_SET) == -1) {
                goto restore_devcleanup;
        }
        buffer = ftdcalloc(1, inhdr.data.ver1.hrdb_size);
        if (read(infd, buffer, inhdr.data.ver1.hrdb_size) != inhdr.data.ver1.hrdb_size) {
                free(buffer);
                goto restore_devcleanup;
        }
        offset = (outhdr.data.ver1.hrdb_offset * 1024) +
                     (j * outhdr.data.ver1.hrdb_size);
        if (llseek(outfd, offset, SEEK_SET) == -1) {
                free(buffer);
                goto restore_devcleanup;
        }
        if (write(outfd, buffer, outhdr.data.ver1.hrdb_size) != outhdr.data.ver1.hrdb_size) {
                free(buffer);
                goto restore_devcleanup;
        }
        free(buffer);

	j++;
       }
    }
    lastdev = j - 1;
restore_devcleanup:
    free(table);
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_dev) {
    	return PS_OK;
    } else {
	return PS_ERR;
    }
}

/*
 * this function is used to copy group information.
 * information is copied from from_ps_name to to_ps_name.
 * this function only does incremental upgrade of group information (old --> latest).
 * switch statements are used to copy information based on the version
 * of the pstore. whenever the ps_group_entry_t structure changes, add the
 * old structure definition (ps_group_ver#_t)  in the header file lib/libgen/ps_migrate.h.
 * Add a case to the switch statements for the previous pstore version
 * and use ps_group_ver#_t for copying device entries.
 */
int ps_copy_group_data(char *from_ps_name, char *to_ps_name, int ftrans)
{
    int              infd, outfd, i = 0, j, cpflag;
    ps_hdr_t         inhdr, outhdr;
    unsigned int     table_size, offset;
    ps_group_entry_t *table = NULL;
    char	     *buffer;

    if ((infd = open(from_ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }
    if ((outfd = open(to_ps_name, O_RDWR | O_SYNC)) == -1) {
	close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET);
    llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET);
    /* read the header */
    read(infd, &inhdr, sizeof(ps_hdr_t));
    read(outfd, &outhdr, sizeof(ps_hdr_t));

    if (llseek(infd, inhdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1) {
        goto grpcleanup;
    }

    switch(version) {
  	case PS_OLD4_HEADER:
  	case PS_OLD3_HEADER:
  	case PS_OLD2_HEADER:
	case PS_OLD1_HEADER:
	default:
    		table_size = sizeof(ps_group_entry_t) * inhdr.data.ver1.max_group;
    		table = (ps_group_entry_t *)ftdcalloc(1, table_size);
    		if (read(infd, (caddr_t)table, table_size) != table_size) {
        		goto grpcleanup;
    		}
    }
    j = 0;
    cpflag = 0;
    for (i = 0; i < inhdr.data.ver1.max_group; i++) {
	switch(version) {
              case PS_OLD4_HEADER:
              case PS_OLD3_HEADER:
              case PS_OLD2_HEADER:
	      case PS_OLD1_HEADER:
	      default:
        	 if ((strlen(table[i].path) != strspn(table[i].path," ")) && table[i].pathlen > 0) {
			cpflag = 1;
		 }
	}
	if (!cpflag) {
	      continue;
	}
	/*
	 * ftrans can be used in a similar manner as it is done for copying device entry
	 * but since currently since there is no structure change for group entry we do not
	 * use it as it will lead to code repetition
	 */
        offset = (outhdr.data.ver1.group_table_offset * 1024) +
                        (j * sizeof(ps_group_entry_t));
        if (llseek(outfd, offset, SEEK_SET) == -1) {
        	goto grpcleanup;
        }
        if (write(outfd, &table[i], sizeof(ps_group_entry_t)) != sizeof(ps_group_entry_t)) {
		goto grpcleanup;
        }
	/*
	 * copy group attributs
	 */
        buffer = ftdcalloc(1, inhdr.data.ver1.group_attr_size);
        offset = (inhdr.data.ver1.group_attr_offset * 1024) +
                      (i * inhdr.data.ver1.group_attr_size);
        if (llseek(infd, offset, SEEK_SET) == -1) {
                free(buffer);
        	goto grpcleanup;
        }
        if (read(infd, buffer, inhdr.data.ver1.group_attr_size) != inhdr.data.ver1.group_attr_size) {
                free(buffer);
        	goto grpcleanup;
        }
        if (!ftrans) {
                switch(version) {
                        case PS_OLD1_HEADER:
                        case PS_OLD2_HEADER:
                        case PS_OLD3_HEADER:
                        case PS_OLD4_HEADER:
                        default:
                                strcat(buffer, "LRT: on\n");
                }
        }
        offset = (outhdr.data.ver1.group_attr_offset * 1024) +
                        (j * outhdr.data.ver1.group_attr_size);
        if (llseek(outfd, offset, SEEK_SET) == -1) {
                free(buffer);
	        goto grpcleanup;
        }
        if (write(outfd, buffer, outhdr.data.ver1.group_attr_size) != outhdr.data.ver1.group_attr_size) {
                free(buffer);
	        goto grpcleanup;
        }
	free(buffer);
        j++;
	cpflag = 0;
    }
    lastgrp = j - 1;
grpcleanup:
    switch(version) {
        case PS_OLD4_HEADER:
        case PS_OLD3_HEADER:
        case PS_OLD2_HEADER:
        case PS_OLD1_HEADER:
        default:
                        free(table);
    }
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_group) {
    	return PS_OK;
    } else {
	return PS_ERR;
    }
}

/*
 * this function is used to copy group information.
 * information is copied from from_ps_name to to_ps_name.
 * this function only does downgrade of group information (latest --> old).
 * switch statements are used to copy information based on the version
 * of the pstore. whenever the ps_group_entry_t structure changes, add the
 * old structure definition (ps_group_ver#_t)  in the header file lib/libgen/ps_migrate.h.
 * Add a case to the switch statements for the previous pstore version
 * and use ps_group_ver#_t for copying device entries.
 */
int ps_restore_group_data(char *from_ps_name, char *to_ps_name, int ftrans)
{
    int              infd, outfd, i = 0, j;
    ps_hdr_t         inhdr, outhdr;
    unsigned int     table_size, offset;
    ps_group_entry_t *table = NULL;
    char	     *buffer, *tmp;

    if ((infd = open(from_ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }
    if ((outfd = open(to_ps_name, O_RDWR | O_SYNC)) == -1) {
	close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET);
    llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET);
    /* read the header */
    read(infd, &inhdr, sizeof(ps_hdr_t));
    read(outfd, &outhdr, sizeof(ps_hdr_t));

    if (llseek(infd, inhdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1) {
        goto restore_grpcleanup;
    }

    table_size = sizeof(ps_group_entry_t) * inhdr.data.ver1.max_group;
    table = (ps_group_entry_t *)ftdcalloc(1, table_size);
    if (read(infd, (caddr_t)table, table_size) != table_size) {
     	goto restore_grpcleanup;
    }
    j = 0;
    for (i = 0; i < inhdr.data.ver1.max_group; i++) {
        if ((strlen(table[i].path) != strspn(table[i].path," ")) && table[i].pathlen > 0) {
	/*
	 * ftrans can be used in a similar manner as it is done for copying device entry
	 * but since currently since there is no structure change for group entry we do not
	 * use it as it will lead to code repetition
	 */

            offset = (outhdr.data.ver1.group_table_offset * 1024) +
                        (j * sizeof(ps_group_entry_t));
            if (llseek(outfd, offset, SEEK_SET) == -1) {
        	goto restore_grpcleanup;
            }
            if (write(outfd, &table[i], sizeof(ps_group_entry_t)) != sizeof(ps_group_entry_t)) {
		goto restore_grpcleanup;
            }
	    /*
	     * copy group attributes
	     */
            buffer = ftdcalloc(1, inhdr.data.ver1.group_attr_size);
            offset = (inhdr.data.ver1.group_attr_offset * 1024) +
                      (i * inhdr.data.ver1.group_attr_size);
            if (llseek(infd, offset, SEEK_SET) == -1) {
                free(buffer);
        	goto restore_grpcleanup;
            }
            if (read(infd, buffer, inhdr.data.ver1.group_attr_size) != inhdr.data.ver1.group_attr_size) {
                free(buffer);
        	goto restore_grpcleanup;
            }
            if (!ftrans) {
                switch(version) {
                        case PS_OLD1_HEADER:
                        case PS_OLD2_HEADER:
                        case PS_OLD3_HEADER:
                        case PS_OLD4_HEADER:
                        default:
                                tmp = strstr(buffer, "LRT");
                                tmp[0] = '\0';
                }
            }
            offset = (outhdr.data.ver1.group_attr_offset * 1024) +
                        (j * outhdr.data.ver1.group_attr_size);
            if (llseek(outfd, offset, SEEK_SET) == -1) {
                free(buffer);
	        goto restore_grpcleanup;
            }
            if (write(outfd, buffer, outhdr.data.ver1.group_attr_size) != outhdr.data.ver1.group_attr_size) {
                free(buffer);
	        goto restore_grpcleanup;
            }
	    free(buffer);
            j++;
      } 
    }
    lastgrp = j - 1;
restore_grpcleanup:
    free(table);
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_group) {
    	return PS_OK;
    } else {
	return PS_ERR;
    }
}

/*
 * this function initializes the pstore.
 * It writes the header information into the pstore for the version
 * passed to it by the argument ver.
 */ 
int initialize_ps(char *ps_name, int *max_dev, int ver)
{
    unsigned long long  size, tmpu64;
    char                raw_name[MAXPATHLEN];
    daddr_t             dsize;
    int              	fd, i;
    ps_hdr_t         	hdr;
    struct stat      	statbuf;
    unsigned int     	table_size = 0, lrdbsize = 0, hrdbsize = 0, grpattrsize = 0, devattrsize = 0;
    ps_dev_entry_t   	*dtable;
    ps_dev_ver1_t    	*dtable_ver1;
    ps_dev_ver3_t     *dtable_ver3;
    ps_group_entry_t 	*gtable = NULL;

#if defined(HPUX)
    if (is_logical_volume(ps_name)) {
        convert_lv_name(raw_name, ps_name, 1);
    } else {
        force_dsk_or_rdsk(raw_name, ps_name, 1);
    }
#else
    force_dsk_or_rdsk(raw_name, ps_name, 1);
#endif

    if ((dsize = disksize(raw_name)) < 0) {
	fprintf(stderr, "Stat error on %s\n", raw_name);
        return -1;
    }
    /*
     * set sizes as per pstore version
     * these will be stored in ps_migrate.h whenever there is any change in these 
     * sizes among different versions.
     */
    switch(ver) {
	case PS_LATEST_HEADER:
	case PS_OLD1_HEADER:
	case PS_OLD2_HEADER:
  	case PS_OLD3_HEADER:
  	case PS_OLD4_HEADER:
	default:
		lrdbsize = FTD_PS_LRDB_SIZE;
		hrdbsize = FTD_PS_HRDB_SIZE_SMALL;
		grpattrsize = FTD_PS_GROUP_ATTR_SIZE;
		devattrsize = FTD_PS_DEVICE_ATTR_SIZE;
    }

    size = lrdbsize + hrdbsize + devattrsize +
          	grpattrsize + 3*MAXPATHLEN;
    size = size / DEV_BSIZE;


#if defined(_AIX)
   dsize = (daddr_t)((dsize - 32 -
               	(lrdbsize+hrdbsize)/DEV_BSIZE ) / size);
#else
    dsize = (daddr_t)((dsize - 32) / size);
#endif

    if (dsize > FTD_MAX_DEVICES) {
        dsize = FTD_MAX_DEVICES;
    }

    *max_dev = dsize;

    /* stat the device and make sure it is a slice */
    if (stat(ps_name, &statbuf) != 0) {
        return PS_BOGUS_PS_NAME;
    }

    if (!(S_ISBLK(statbuf.st_mode))) {
        return PS_BOGUS_PS_NAME;
    }
    /* open the store */
    if ((fd = open(ps_name, O_RDWR)) == -1) {
        return PS_BOGUS_PS_NAME;
    }
    hdr.data.ver1.max_dev = dsize;
    hdr.data.ver1.max_group = dsize;
    hdr.data.ver1.dev_attr_size = devattrsize;
    hdr.data.ver1.group_attr_size = grpattrsize;
    hdr.data.ver1.num_device = 0;
    hdr.data.ver1.num_group = 0;
    hdr.data.ver1.last_device = -1;
    hdr.data.ver1.last_group = -1;
    hdr.data.ver1.lrdb_size = lrdbsize;
    hdr.data.ver1.hrdb_size = hrdbsize;

    switch(ver) {
	case PS_LATEST_HEADER:
    		hdr.magic = PS_VERSION_1_MAGIC;
	    	hdr.data.ver1.dev_table_entry_size = sizeof(ps_dev_entry_t);
    		hdr.data.ver1.group_table_entry_size = sizeof(ps_group_entry_t);
		break;
	case PS_OLD1_HEADER:
		hdr.magic = PS_VERSION_1_MAGIC_OLD1;
		hdr.data.ver1.dev_table_entry_size = sizeof(ps_dev_ver1_t);
		hdr.data.ver1.group_table_entry_size = sizeof(ps_group_entry_t);
                break;
	case PS_OLD2_HEADER:
                hdr.magic = PS_VERSION_1_MAGIC_OLD2;
                hdr.data.ver1.dev_table_entry_size = sizeof(ps_dev_ver1_t);
                hdr.data.ver1.group_table_entry_size = sizeof(ps_group_entry_t);
                break;
  	case PS_OLD3_HEADER:
       		hdr.magic = PS_VERSION_1_MAGIC_OLD3;
       		hdr.data.ver1.dev_table_entry_size = sizeof(ps_dev_ver3_t);
       		hdr.data.ver1.group_table_entry_size = sizeof(ps_group_entry_t);
       		break;
  	case PS_OLD4_HEADER:
       		hdr.magic = PS_VERSION_1_MAGIC_OLD4;
       		hdr.data.ver1.dev_table_entry_size = sizeof(ps_dev_entry_t);
       		hdr.data.ver1.group_table_entry_size = sizeof(ps_group_entry_t);
       		break;
    }

    /*
     * calculate offsets as per the pstore version
     */
    hdr.data.ver1.dev_table_offset = PS_HEADER_OFFSET +
        ((sizeof(hdr) + 1023) / 1024);
    hdr.data.ver1.group_table_offset = hdr.data.ver1.dev_table_offset +
        (((hdr.data.ver1.max_dev * hdr.data.ver1.dev_table_entry_size) + 1023) / 1024);
    hdr.data.ver1.dev_attr_offset = hdr.data.ver1.group_table_offset +
        (((hdr.data.ver1.max_group * hdr.data.ver1.group_table_entry_size) + 1023) / 1024);
    hdr.data.ver1.group_attr_offset = hdr.data.ver1.dev_attr_offset +
        (((hdr.data.ver1.max_dev * hdr.data.ver1.dev_attr_size) + 1023) / 1024);


#if defined(_AIX) || defined(linux)
    switch(ver) {
	case PS_LATEST_HEADER:
  	case PS_OLD4_HEADER:
  	case PS_OLD3_HEADER:
	case PS_OLD2_HEADER:
    	     tmpu64 = ((hdr.data.ver1.group_attr_offset * 1024 +
                	hdr.data.ver1.max_group * hdr.data.ver1.group_attr_size + lrdbsize-1)
                	/ lrdbsize) * (lrdbsize/1024);
    	     hdr.data.ver1.lrdb_offset = (unsigned int) tmpu64;

    	     tmpu64 = ((hdr.data.ver1.lrdb_offset * 1024 +
             		  hdr.data.ver1.max_dev * hdr.data.ver1.lrdb_size + hrdbsize-1)
                	  / hrdbsize) * (hrdbsize/1024);
    	     hdr.data.ver1.hrdb_offset = (unsigned int) tmpu64;
	     break;
	case PS_OLD1_HEADER:
    	     hdr.data.ver1.lrdb_offset = hdr.data.ver1.group_attr_offset +
        	(((hdr.data.ver1.max_group * hdr.data.ver1.group_attr_size) + 1023) / 1024);
    	     hdr.data.ver1.hrdb_offset = hdr.data.ver1.lrdb_offset +
        	(((hdr.data.ver1.max_dev * hdr.data.ver1.lrdb_size) + 1023) / 1024);
    }
#else

    hdr.data.ver1.lrdb_offset = hdr.data.ver1.group_attr_offset +
        (((hdr.data.ver1.max_group * hdr.data.ver1.group_attr_size) + 1023) / 1024);
    hdr.data.ver1.hrdb_offset = hdr.data.ver1.lrdb_offset +
        (((hdr.data.ver1.max_dev * hdr.data.ver1.lrdb_size) + 1023) / 1024);
#endif  /* _AIX and linux */

    hdr.data.ver1.last_block = hdr.data.ver1.hrdb_offset +
        (((hdr.data.ver1.max_dev * hdr.data.ver1.hrdb_size) + 1023) / 1024) - 1;

    if (llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &hdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t)) {
        close(fd);
        return PS_WRITE_ERROR;
    }
    /* allocate a table for devices */
    switch(ver) {
	case PS_LATEST_HEADER:
	case PS_OLD4_HEADER:
    		table_size = hdr.data.ver1.max_dev * sizeof(ps_dev_entry_t);
    		if ((dtable = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        	    close(fd);
        	    return PS_MALLOC_ERROR;
    		}
    /* set all values to the bogus value */
    		for (i = 0; i < hdr.data.ver1.max_dev; i++) {
        	    dtable[i].pathlen = 0;
        	    memset(dtable[i].path, 0, MAXPATHLEN);
            	    dtable[i].num_lrdb_bits = 0xffffffff;
        	    dtable[i].num_hrdb_bits = 0xffffffff;
        	    dtable[i].num_sectors = 0xffffffff;
        	    dtable[i].ackoff = 0;
    		}
        /* write out the device index array */
    		if (llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
        	    close(fd);
		    free(dtable);
		    return PS_SEEK_ERROR;
    		}
    		if (write(fd, dtable, table_size) != table_size) {
        	    close(fd);
        	    free(dtable);
        	    return PS_WRITE_ERROR;
    		}
     		free(dtable);
		break;
	case PS_OLD1_HEADER:
	case PS_OLD2_HEADER:
    		table_size = hdr.data.ver1.max_dev * sizeof(ps_dev_ver1_t);
    		if ((dtable_ver1 = (ps_dev_ver1_t *)malloc(table_size)) == NULL) {
        	    close(fd);
        	    return PS_MALLOC_ERROR;
    		}
    /* set all values to the bogus value */
    		for (i = 0; i < hdr.data.ver1.max_dev; i++) {
        	    dtable_ver1[i].pathlen = 0;
        	    memset(dtable_ver1[i].path, 0, MAXPATHLEN);
        	    dtable_ver1[i].num_lrdb_bits = 0xffffffff;
        	    dtable_ver1[i].num_hrdb_bits = 0xffffffff;
        	    dtable_ver1[i].num_sectors = 0xffffffff;
    		}
        /* write out the device index array */
    		if (llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
        	    close(fd);
        	    free(dtable_ver1);
        	    return PS_SEEK_ERROR;
    		}
    		if (write(fd, dtable_ver1, table_size) != table_size) {
         	    close(fd);
        	    free(dtable_ver1);
        	    return PS_WRITE_ERROR;
    		}
    		free(dtable_ver1);
        break;
	case PS_OLD3_HEADER:
    		table_size = hdr.data.ver1.max_dev * sizeof(ps_dev_ver3_t);
    		if ((dtable_ver3 = (ps_dev_ver3_t *)malloc(table_size)) == NULL) {
        	    close(fd);
        	    return PS_MALLOC_ERROR;
    		}
    /* set all values to the bogus value */
    		for (i = 0; i < hdr.data.ver1.max_dev; i++) {
        	    dtable_ver3[i].pathlen = 0;
        	    memset(dtable_ver3[i].path, 0, MAXPATHLEN);
        	    dtable_ver3[i].num_lrdb_bits = 0xffffffff;
        	    dtable_ver3[i].num_hrdb_bits = 0xffffffff;
        	    dtable_ver3[i].num_sectors = 0xffffffff;
              dtable_ver3[i].ackoff = 0;
    		}
        /* write out the device index array */
    		if (llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
        	    close(fd);
        	    free(dtable_ver3);
        	    return PS_SEEK_ERROR;
    		}
    		if (write(fd, dtable_ver3, table_size) != table_size) {
         	    close(fd);
        	    free(dtable_ver3);
        	    return PS_WRITE_ERROR;
    		}
    		free(dtable_ver3);
        break;
    }

    /* allocate a table for groups */
    switch(ver) {
	case PS_LATEST_HEADER:
	case PS_OLD1_HEADER:
	case PS_OLD2_HEADER:
  	case PS_OLD3_HEADER:
  	case PS_OLD4_HEADER:
    		table_size = hdr.data.ver1.max_group * sizeof(ps_group_entry_t);
    		if ((gtable = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        	    close(fd);
        	    return PS_MALLOC_ERROR;
    		}
    /* set all values to the bogus value */
    		for (i = 0; i < hdr.data.ver1.max_group; i++) {
        	    gtable[i].pathlen = 0;
        	    memset(gtable[i].path, 0, MAXPATHLEN);
        	    gtable[i].hostid = 0xffffffff;
    		}
    }
    /* write out the group index array */
    if (llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1) {
        free(gtable);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, gtable, table_size) != table_size) {
        free(gtable);
        close(fd);
        return PS_WRITE_ERROR;
    }
    free(gtable);

    close(fd);

    return PS_OK;
}

/*
 * This function upgrades (old --> latest) the pstore by calling the functions to copy
 * device and group information.
 *
 */
int copy_pstore(char *from_ps_name, char *to_ps_name, int copy_to_file)
{
  ps_hdr_t attr;

  if (get_header(from_ps_name, &attr) != PS_OK) {
	fprintf(stderr, "Error while reading header information\n");
	return(-1);
  }

  if (set_header(to_ps_name, attr, copy_to_file) != PS_OK) {
	fprintf(stderr, "Error while writing header information\n");
	return(-1);
  }

  if (ps_copy_device_data(from_ps_name, to_ps_name, copy_to_file) == PS_ERR) {
	fprintf(stderr, "Error while migrating device data\n");
	return(-1);
  }
  if (ps_copy_group_data(from_ps_name, to_ps_name, copy_to_file) == PS_ERR) {
       	fprintf(stderr, "Error while migrating group data\n");
       	return(-1);
  }

  return 0;
}

/*
 * This function downgrades i(latest --> old) the pstore by calling the functions to copy
 * device and group information.
 *
 */
int restore_pstore(char *from_ps_name, char *to_ps_name, int copy_to_file)
{
  ps_hdr_t attr;

  if (get_header(from_ps_name, &attr) != PS_OK) {
        fprintf(stderr, "Error while reading header information\n");
        return(-1);
  }

  if (set_header(to_ps_name, attr, copy_to_file) != PS_OK) {
        fprintf(stderr, "Error while writing header information\n");
        return(-1);
  }
  if (ps_restore_device_data(from_ps_name, to_ps_name, copy_to_file) == PS_ERR) {
        fprintf(stderr, "Error while migrating device data\n");
        return(-1);
  }
  if (ps_restore_group_data(from_ps_name, to_ps_name, copy_to_file) == PS_ERR) {
        fprintf(stderr, "Error while migrating group data\n");
        return(-1);
  }

  return 0;
}

unsigned int get_header_version( ps_hdr_t *hdr )
{
    if( hdr->magic == PS_VERSION_1_MAGIC_RFX270 )
    {
		if (hdr->data.ver1.hrdb_size == FTD_PS_HRDB_SIZE_SMALL)
	        return PS_LATEST_HEADER;
		else
		    return PS_LARGE_HRDB;
    }
    else if (hdr->magic == PS_VERSION_1_MAGIC_OLD1)
    {
        return PS_OLD1_HEADER;
    }
    else if (hdr->magic == PS_VERSION_1_MAGIC_OLD2)
    {
        return PS_OLD2_HEADER;
    }
    else if (hdr->magic == PS_VERSION_1_MAGIC_OLD3)
    {
        return PS_OLD3_HEADER;
    }
    else if (hdr->magic == PS_VERSION_1_MAGIC_OLD4)
    {
        return PS_OLD4_HEADER;
    }
    return PS_BOGUS_HEADER;
}


int main(int argc, char *argv[])
{
  int group, resgrp, rflag, lgcnt, i, j, max_devices, num_started_grps, tmpver;
  int ps_migrated, num_old_grps, num_old_devs, major, minor, pat1, pat2;
  char ps_name[MAXPATHLEN], psfilename[MAXPATHLEN], strgrp[5], repver[MAX_VER_LENGTH+1];
  struct statvfs st;
  char *started_ps_names = NULL;
  struct dirent *dent;
  DIR *curdir;
  ps_hdr_t hdr_info;
  unsigned long req_space;
  int ch;
  int req_version_length;

  putenv("LANG=C");

  /* Make sure we are root */
  if (geteuid()) {
      fprintf(stderr, "You must be root to run this process...aborted\n");
      exit(1);
  }
  progname = argv[0];

  opterr = 0;
  rflag = 0;
  while ((ch = getopt(argc, argv, "hr:")) != EOF) {
      switch(ch) {
        case 'r':
                if (rflag) {
                    fprintf(stderr, "Specify -r option only once\n");
                    exit(-1);
                }
                rflag = 1;
                if((req_version_length = strlen(optarg)) > MAX_VER_LENGTH)
                {
                    fprintf(stderr, "Error: required version's string length exceeds memory allocated for validations.\n");
                    free(started_ps_names);
                    exit(1);
                }
                strncpy(repver, optarg, req_version_length);
                repver[req_version_length] = '\0';
                if (sscanf(repver, "%d.%d.%d.%d", &major, &minor, &pat1, &pat2) < 4) {
                        fprintf(stderr, "Please specify a valid product version (e.g. 2.4.2.0).\n");
                        exit(1);
                }
                break;
        case 'h':
                usage();
                break;
        default:
                fprintf(stderr, "Invalid usage\n");
                usage();
      }
  }
  if (optind != argc) {
      fprintf(stderr, "Invalid arguments\n");
      usage();
  }

/*
 * check if any group has been started 
 */
  num_started_grps = 0;
  if ((curdir = opendir(PATH_CONFIG)) == NULL) {
      fprintf(stderr, "Error opening the CFG directory\n");
  }
  started_ps_names = (char *)ftdmalloc(MAXPATHLEN);
  while ((dent = readdir(curdir)) != NULL) {
      if (!strncmp(&dent->d_name[4], ".cur", 4)) {
          strncpy(strgrp, &dent->d_name[1], 3);
	  strgrp[3] = '\0';
          group = ftd_strtol(strgrp);
	  if (group < 0) {
		continue;
	  }
          fprintf(stderr, "Logical group %d is started. Please stop the group before migrating it's pstore.\n", group);
          GETPSNAME(group, ps_name);
	  if (num_started_grps == 0) {
		strcpy(started_ps_names, ps_name);
	  } else {
          	strcpy(started_ps_names+(num_started_grps*MAXPATHLEN)+1, ps_name);
	  }
	  num_started_grps++;
	  started_ps_names = ftdrealloc(started_ps_names, (num_started_grps+1)*MAXPATHLEN);
      }
  }
  closedir(curdir);

  paths = (char *)configpaths;
  lgcnt = GETCONFIGS(configpaths, 1, 0);
  if (lgcnt == 0) {
      fprintf(stderr, "No CFG file found\n");
      exit(1);
  }

  if (rflag) {
      version = -1;  /* Default value to detect error */
      if (!strncmp(repver, "2.2.0.0", 7) || !strncmp(repver, "2.1", 3)) {
          version = PS_OLD1_HEADER;
      } else if(!strncmp(repver, "2.2", 3) || !strncmp(repver, "2.3", 3)) {
          version = PS_OLD2_HEADER;
      } else if(!strncmp(repver, "2.4", 3) || !strncmp(repver, "2.4.2.0", 7)) {
          version = PS_OLD3_HEADER;
      } else if(!strncmp(repver, "2.5", 3) || !strncmp(repver, "2.5.3.0", 7)) {
          version = PS_OLD4_HEADER;
	  } else if(!strncmp(repver, "2.6.0", 5)) {
		  version = PS_OLD4_HEADER;
	  } 
	  else
      {
          /* Now checking for the range of version that take on the PS_LATEST_HEADER value */
          if(strncmp(repver, "2.6.1", 5) >= 0)
          { /* Above or equal to 2.6.1; check uppper bound using integer version componants */
            int             running_major=0, running_minor=0, running_pat1=0, running_pat2=0;

            if(strlen(VERSION) > MAX_VER_LENGTH)
            {
                fprintf(stderr, "Error: running version's string length exceeds memory allocated for validations.\n");
                free(started_ps_names);
                exit(1);
            }

            sscanf(VERSION, "%d.%d.%d.%d", &running_major, &running_minor, &running_pat1, &running_pat2);
            if( major < running_major )
            {
              version = PS_LATEST_HEADER;   // OK, done
            }
            else if( major == running_major )  // Match, check next componant
            {
                if( minor < running_minor )
                {
                  version = PS_LATEST_HEADER;   // OK, done
                }
                else if( minor == running_minor )  // Match, check next componant
                {
                    if( pat1 < running_pat1 )
                    {
                      version = PS_LATEST_HEADER;   // OK, done
                    }
                    else if( pat1 == running_pat1 )  // Match, check next componant
                    {
                        if( pat2 <= running_pat2 )
                        {
                          version = PS_LATEST_HEADER;   // OK, done
                        }
                    }
                }
            }
          }
      }
      if( version < 0 )
      {
          fprintf(stderr, "Please specify a correct product version (2.1.0.0 - %s).\n", VERSION);
          free(started_ps_names);
          exit(1);
      }
      if(version != PS_LATEST_HEADER)
      {
        fprintf(stderr, "Pstore format will be converted to version %s pstore format.\n", repver);
      }
      else
      {
        // If we go to latest pstore format, mimick no argument on command line (which does exactly that)
        rflag = 0;
        fprintf(stderr, "Pstore format will be converted to version %s pstore format (which is the latest format prior to RFX/TUIP 2.7.1).\n", repver);
     }
  }

  ps_migrated = 0;
  is_exit = 0;
  instsigaction();
/*
 * loop through the groups and migrate the pstore for each group. Since a pstore can
 * be shared by many groups, if the pstore is migrated for one group we do not
 * migrate that pstore again for another group.
 */
  for (i = 0; i < lgcnt; i++) {
	if (is_exit) {
	    exit(0);
	}
  	group = cfgtonum(i);
        if (GETPSNAME(group, ps_name) != 0) {
            fprintf(stderr, "Could not get pstore name for group %d\n", group);
            continue;
        }
        for (j = 0; j < num_started_grps; j++) {
	     if (j == 0) {
		if (!strcmp(started_ps_names, ps_name)) {
                    break;
             	}
	     } else {
	     	if (!strcmp(started_ps_names+(j*MAXPATHLEN)+1, ps_name)) {
 	            break;
	     	}
	     }
        }
        if (j == num_started_grps) { 
 	    if (0 != statvfs(PS_TMP_FILE_PATH, &st)) {
      	        fprintf(stderr, "Could not calculate free space in /tmp\n");
      	        exit(1);
  	    }

		get_header(ps_name, &hdr_info);
	    if( hdr_info.magic > PS_VERSION_1_MAGIC_RFX270 )
		{
			fprintf(stderr, "The pstore for group %d has been formatted with a release more recent than RFX/TUIP 2.7.0.\n", group);
			fprintf(stderr, "This cannot be converted to an older or more recent version and hence will be skipped from the migration process.\n");
	        continue;
		}

	    if (rflag)
	    {
		    tmpver = get_header_version( &hdr_info );
			if( tmpver != PS_LATEST_HEADER )
			{
				if( tmpver == PS_LARGE_HRDB )
				{
					fprintf(stderr, "The pstore for group %d has large HRT. This cannot be converted to an older version and hence will be skipped from the migration process.\n", group);
				}
			    continue;
			}
	    }
		else
		{
	       	version = get_header_version( &hdr_info );
		}

	    if (version >= PS_LATEST_HEADER && version <= PS_OLD4_HEADER) {
	/*
	 * Calculate the amount of space required in /tmp to migrate this pstore.
	 */
		req_space = hdr_info.data.ver1.hrdb_offset * 1024 + hdr_info.data.ver1.num_device * hdr_info.data.ver1.hrdb_size;
		if (req_space > (st.f_bsize * st.f_bfree)) {
		    fprintf(stderr, "More than %lu bytes of free space is required in /tmp to migrate the pstore for group %d. Please free the requisite amount of space in /tmp before continuing pstore migration for this group.\n", req_space, group);
		    continue;
		}
	    }
	    switch(version) {
		  case PS_LATEST_HEADER:
			break;
      		  case PS_OLD4_HEADER:
      		  case PS_OLD3_HEADER:
      		  case PS_OLD2_HEADER:
		  case PS_OLD1_HEADER:
			fprintf(stderr, "Migrating pstore %s ...\n", ps_name);
			if (rflag) {
			    sprintf(psfilename, "%spstoredata%03d", PS_TMP_FILE_PATH, group);
			/*
			 * copy pstore contents to a file
			 */
                            if (restore_pstore(ps_name, psfilename, 1) == -1) {
                                fprintf(stderr, "An error occured while migrating this pstore. The pstore currently holds the latest version contents.\n");
                                unlink(psfilename);
                                exit(-1);
                            }
                        /*
                         * initialize the pstore to the specified version
                         */
                            if (initialize_ps(ps_name, &max_devices, version) != 0) {
                                restore_pstore(psfilename, ps_name, 1);
                                fprintf(stderr, "An error occured while migrating this pstore. The pstore currently holds the latest version contents.\n");
                                unlink(psfilename);
                                exit(-1);
                            }
			/*
                         * copy the file contents back to the pstore
                         */
                            if (restore_pstore(psfilename, ps_name, 0) == -1) {
                                restore_pstore(psfilename, ps_name, 1);
                                fprintf(stderr, "An error occured while migrating this pstore. The pstore currently holds the latest version contents.\n");
                                unlink(psfilename);
                                exit(-1);
                            }
			} else {
			    ps_max_devices(ps_name, &max_devices, &num_old_devs, &num_old_grps);
			    if (num_old_devs > max_devices || num_old_grps > max_devices) {
			        fprintf(stderr, "The pstore %s will not be able to support all the devices/groups configured on it after it is converted to the latest version. Hence this pstore will not be migrated.\n", ps_name);
			        continue;
			    }
			    sprintf(psfilename, "%spstoredata%03d", PS_TMP_FILE_PATH, group);
                        /*
                         * copy pstore contents to a file
                         */
		            if (copy_pstore(ps_name, psfilename, 1) == -1) {
			        fprintf(stderr, "An error occured while migrating this pstore. The pstore currently holds the old version contents.\n");
			        unlink(psfilename);
			        exit(-1);
			    }
                        /*
                         * initialize the pstore to the latest version
                         */
  			    if (initialize_ps(ps_name, &max_devices, PS_LATEST_HEADER) != 0) {
			        copy_pstore(psfilename, ps_name, 1);
			        fprintf(stderr, "An error occured while migrating this pstore. The pstore currently holds the old version contents.\n");
                                unlink(psfilename);
                                exit(-1);
                            }
                        /*
                         * copy the file contents back to the pstore
                         */
			    if (copy_pstore(psfilename, ps_name, 0) == -1) {
				copy_pstore(psfilename, ps_name, 1);
				fprintf(stderr, "An error occured while migrating this pstore. The pstore currently holds the old version contents.\n");
				unlink(psfilename);
  			        exit(-1);
			    }
			}
			fprintf(stderr, "Migration completed for pstore %s\n", ps_name);
			unlink(psfilename);
			ps_migrated = 1;
			break;
		  default:
                    	fprintf(stderr, "No pstore to migrate for logical group %d.\n", group);
	   }
	}
  }
  if (ps_migrated == 0 && rflag) {
      fprintf(stderr, "No pstores were migrated. This may happen because all the groups are started, there are no pstores to migrate, all the pstores are of old version, all the pstores have large HRT or are post-RFX2.7.0 format or there is not enough free space in /tmp.\n");
  } else if (ps_migrated == 0) {
      fprintf(stderr, "No pstores were migrated. This may happen because all the groups are started, there are no pstores to migrate, all the pstores are of latest version or there is not enough free space in /tmp.\n");
  }
  free(started_ps_names);

  return 0;
}

#if defined(HPUX) && (SYSVERS >= 1100)
  shl_load () {}
  shl_unload () {}
  shl_findsym () {}
#endif

