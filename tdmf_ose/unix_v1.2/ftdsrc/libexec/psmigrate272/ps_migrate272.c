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
 * ps_migrate272.c --> Migrates all pstores of version 2.6.x, 2.7.0 or 2.7.1 format to a pstore of version 2.7.2.
 */

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
#include <ctype.h>       //for lib function 'isdigit'
#if !defined(linux)
#include <macros.h>
#endif /* !defined(linux) */
#include <sys/statvfs.h>
#include <dirent.h>
#if defined(linux)
#include <linux/fs.h>
#include <sys/utsname.h>
#endif

#include "ps_migrate.h" // Pstore definitions of previous releases (pre-RFX272)
#include "ps_pvt.h"     // Current release Pstore definitions
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
int is_exit;
static char *migrated_pstores[MAXLG];
static int  number_of_migrated_pstores = 0;

static FILE *logfp = NULL;
static char logfile[MAXPATHLEN];
static char message[MAXPATHLEN];

#define MAX_VER_LENGTH 20

// The following is taken from our driver macros to recompute dirty bits from RFX270 and from RFX271 in Legacy Small and Large HRT mode
/* this assumes 32-bits per int */
#define WORD_BOUNDARY(x)  ((x) >> 5)
#define SINGLE_BIT(x)     (1 << ((x) & 31))
#define TEST_BIT(ptr,bit) (SINGLE_BIT(bit) & *(ptr + WORD_BOUNDARY(bit)))
#define SET_BIT(ptr,bit)  (*(ptr + WORD_BOUNDARY(bit)) |= SINGLE_BIT(bit))

/* use natural integer bit ordering (bit 0 = LSB, bit 31 = MSB) */
#define START_MASK(x)     (((ftd_uint32_t)0xffffffff) << ((x) & 31))
#define END_MASK(x)       (((ftd_uint32_t)0xffffffff) >> (31 - ((x) & 31)))

/* A LRDB bit should address no less than 256K (extracted from ftd_def.h) */
#define MINIMUM_LRDB_BITSIZE      18


// Function to display and log a message
void display_and_log_message( char *message )
{
    static int logfile_error_displayed = 0;

	if ((logfp = fopen(logfile,"a")) == NULL)
	{
	    if( !logfile_error_displayed ) fprintf( stderr, "Error opening logfile: %s. Will display message only on screen:\n", strerror( errno ) );
	    logfile_error_displayed = 1;
	}
	else
	{
		fprintf(logfp,"%s",message);
		fflush(logfp);
		fclose(logfp);
	}
    fprintf(stderr, message);
}

/*-
 * set_bits()
 *
 * set bits between [x1, x2] (inclusive) in an array of 32-bit integers.
 * the bit ordering within the integer is controlled by the macros 
 * START_MASK and END_MASK. 
 *
 * we assume the caller has clamped x1 and x2 to the boundaries of the
 * array.
 */
static void set_bits (unsigned int *ptr, int x1, int x2)
{
  unsigned int *dest;
  unsigned int mask, word_left;
  int          n_words;

  word_left = WORD_BOUNDARY (x1);
  dest = ptr + word_left;
  mask = START_MASK (x1);
  if ((n_words = (WORD_BOUNDARY (x2) - word_left)) == 0)
    {
      mask &= END_MASK (x2);
      *dest |= mask;
      return;
    }
      
  *dest++ |= mask;
  while (--n_words > 0)
    {
      *dest++ = 0xffffffff;
    }
  *dest |= END_MASK (x2);
  return;
}

// Extracted from driver code, for bit precision calculations
static int ftd_get_msb_64 (ftd_int64_t num)
{
  ftd_int32_t i;
  ftd_uint64_t temp = (ftd_uint64_t) num;

  for (i = 0; i < 64; i++, temp >>= 1)
    {
      if (temp == 0)
        break;
    }

  return (i - 1);
}

static int ftd_get_msb_32 (ftd_int32_t num)
{
  ftd_int32_t i;
  ftd_uint32_t temp = (ftd_uint32_t) num;

  for (i = 0; i < 32; i++, temp >>= 1)
    {
      if (temp == 0)
        break;
    }

  return (i - 1);
}

// Recalculate dirty bits to the optimized format introduced in WR PROD7660, where each bit
// of the old format becomes 8 bits in the new format
// NOTE: this should not be called for a group in PASSTHRU mode, whose bits are all dirty
//       up to the end of the allocated space, nor for the HRDB of a group using Large HRT mode,
//       to which the optimization of PROD7660 did not apply.
// Return: non-zero status if target buffer would overflow
static int recalculate_dirty_bits(unsigned int *source_buffer, unsigned int *target_buffer,
            unsigned int source_buffer_size_32bit_words, unsigned int target_buffer_size_32bit_words,
            unsigned long long device_size_sectors, unsigned long long limitsize_multiple, int is_LRDB,
            char *dtc_device_name, int sector_size )
{
	int          source_word_index, source_word_bit_index, found_bit, target_word_index;
	unsigned int start_target_bit, end_target_bit;
	unsigned long long expanded_disksize;
    int          diskbits, bitsize_RFX270, bitsize_RFX272, tracking_bits_RFX270, tracking_bits_RFX272;
	int          local_device_hardware_sector_size_bit_order;
	int          translation_factor;
	char         *LRDB_string = "LRDB";
	char         *HRDB_string = "HRDB";
	char         *buffer_type;

    found_bit = 0;

    // Driver logic to calculate tracking bit precision:
    expanded_disksize = limitsize_multiple * device_size_sectors;

	diskbits = ftd_get_msb_64 (expanded_disksize) + 1 + DEV_BSHIFT;
	tracking_bits_RFX270 = ftd_get_msb_64 (source_buffer_size_32bit_words * 4);
	tracking_bits_RFX272 = ftd_get_msb_64 (target_buffer_size_32bit_words * 4 * 8);
	bitsize_RFX270 = diskbits - tracking_bits_RFX270;
	bitsize_RFX272 = diskbits - tracking_bits_RFX272;
	if( is_LRDB )
	{
	    buffer_type = LRDB_string;
	    if (bitsize_RFX270 < MINIMUM_LRDB_BITSIZE)
	        bitsize_RFX270 = MINIMUM_LRDB_BITSIZE;
	    if (bitsize_RFX272 < MINIMUM_LRDB_BITSIZE)
	        bitsize_RFX272 = MINIMUM_LRDB_BITSIZE;
	}
	else  // HRDB
	{
	    buffer_type = HRDB_string;
   		if( sector_size < 512 )
		{
		    sprintf( message, "The sector size passed to the tracking bit conversion function for %s of device %s is not valid (%d).\n",
		                       buffer_type, dtc_device_name, sector_size );
            display_and_log_message( message );
			return( -1 );
		}
        local_device_hardware_sector_size_bit_order =  ftd_get_msb_32(sector_size);

        if(bitsize_RFX270 < local_device_hardware_sector_size_bit_order)
           bitsize_RFX270 = local_device_hardware_sector_size_bit_order;
        if(bitsize_RFX272 < local_device_hardware_sector_size_bit_order)
           bitsize_RFX272 = local_device_hardware_sector_size_bit_order;
	}
	if( bitsize_RFX270 == bitsize_RFX272 )
	{
		sprintf( message, "%s %s tracking bit resolution is the same in the old pstore format as in the new format; copying...\n",
		                   dtc_device_name, buffer_type );
		display_and_log_message( message );
		// Copy from source to target and return
        memset( target_buffer, 0, target_buffer_size_32bit_words * 4 );
		memcpy( target_buffer, source_buffer, target_buffer_size_32bit_words * 4 );
		return( 0 );
	}
	
	// Bit precision is not the same; do the translation
	// The bitsize value gives the exponent of 2 to get the bit precision;
	// For instance, if bitsize_RFX270 == 18, one bit covers 256 KB, and if bitsize_RFX272 == 16, one bit covers 64 KB; then
	// we need to translate each tracking bit of RFX270 to 4 bits in RFX272 to cover the same data, i.e.
	// translation factor = 2 ^ (bitsize_RFX270 - bitsize_RFX272) == 1 << (bitsize_RFX270 - bitsize_RFX272)
	translation_factor = 1 << (bitsize_RFX270 - bitsize_RFX272);
	sprintf( message, "%s %s tracking bit resolution RFX270 = %d, RFX272 = %d; translating with conversion factor %d...\n",
	                   dtc_device_name, buffer_type, bitsize_RFX270, bitsize_RFX272, translation_factor );
	display_and_log_message( message );

    // Find the last bit set in the source buffer and check that we will not overflow the target buffer size
    for( source_word_index = source_buffer_size_32bit_words-1; (source_word_index >= 0) && !found_bit; source_word_index-- )
	{
	    for( source_word_bit_index = 31; source_word_bit_index >= 0; source_word_bit_index-- )
		{
            if( TEST_BIT(&(source_buffer[source_word_index]), source_word_bit_index) )
            {
			    // Every source bit maps to its multiple of 8 in the target and becomes one byte
				// Check against target buffer overflow here (found last source bit set)
				found_bit = 1;
				target_word_index = source_word_index * 8;
				if( (target_word_index+1) > target_buffer_size_32bit_words )
				{
				    sprintf( message, "Cannot migrate %s source dirty bits due to target buffer overflow.\n", buffer_type );
	                display_and_log_message( message );
					return( -1 );
				}
				break;
			}
		}
	}

    // Ok to proceed...
    for( source_word_index = 0; source_word_index < source_buffer_size_32bit_words;	source_word_index++ )
	{
	    for( source_word_bit_index = 0; source_word_bit_index <= 31; source_word_bit_index++ )
		{
            if( TEST_BIT(&(source_buffer[source_word_index]), source_word_bit_index) )
            {
#ifdef DEBUG_TRACKING_BIT_CONVERSION
			    sprintf( message, "Converting %s tracking bit %d of word %d (bit %d),", buffer_type, source_word_bit_index,
			                       source_word_index, (source_word_index*32)+source_word_bit_index );
                display_and_log_message( message );
#endif
			    // Every source bit maps to its multiple of 8 in the target and becomes one byte
			    start_target_bit = ((source_word_index * 32) + source_word_bit_index) * translation_factor;
				end_target_bit =  start_target_bit + (translation_factor-1);
#ifdef DEBUG_TRACKING_BIT_CONVERSION
			    sprintf( message, " mapping it to target buffer bits %d to %d).\n", start_target_bit, end_target_bit );
                display_and_log_message( message );
#endif
			    set_bits( target_buffer, start_target_bit, end_target_bit );
			}
		}
	}

	return( 0 );

}


static void
usage(void)
{
    fprintf(stderr, "Usage: %s [-y] [-h]\n", progname);
    fprintf(stderr, "\t %s will migrate pstore versions 2.6.x, 2.7.0 and 2.7.1 to the pstore format of\n", progname);
    fprintf(stderr, "\t Replicator for Unix / TDMF IP for Unix release 2.7.2.\n");
    fprintf(stderr, "\t For older pstores, migrate them to the 2.7.0 format first wih dtcpsmigrate, and then use dtcpsmigrate272 after.\n");
    fprintf(stderr, "\t -y option confirms launching a pstore migration.\n");
    fprintf(stderr, "\t -h option displays this help paragraph.\n");
    fprintf(stderr, "\t NOTES:\n");
    fprintf(stderr, "\t 1) it is assumed that the OS release remains the same as it was before, i.e. the old pstores were used on the same OS level\n");
    fprintf(stderr, "\t       as the one in use now\n");
    fprintf(stderr, "\t 2) messages displayed on screen are also logged in %s\n", logfile);
#if !defined(linux)
    fprintf(stderr, "\t 3) if some of the devices were extended (dtcexpand) in the older release, you must reset these\n");
    fprintf(stderr, "\t       devices expansion limit to 1 before the dtcstart command that will follow the pstore migration.\n");
    fprintf(stderr, "\t       The command to do so is:\n");
    fprintf(stderr, "\t            dtclimitsize -g<group number> -d <device name as defined in the group cfg file> -s 1\n");
    fprintf(stderr, "\t       Otherwise the groups to which these devices belong will need a Full Refresh after pstore migration.\n");
#endif
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


// Display a prompt and read the yes or no answer
// Return: 1 means yes; 0 means no.
int yesorno(char *message)
{
	char	in;
	int		ans = -1;

	while (1) {
		printf("%s [y/n] : ", message);
		in = getchar();
		if (in == '\n') continue;
		if (getchar() != '\n') {
			while(getchar() != '\n');
			continue;
		}
		switch (in) {
			case 'y':
			case 'Y':
				ans = 1;
				break;
			case 'n':
			case 'N':
				ans = 0;
				break;
			default :
				break;
		}
		if (ans == 1 || ans == 0) return ans;
	}
}


/*---------- Getting a device sector size ----------
Our driver does the following:
   int hardware_sector_size_bit_order =  DEV_BSHIFT; // Default value
#if defined (LINUX260)
   struct block_device *bdev = bdget(physical_block_device);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   hardware_sector_size_bit_order = get_bitmask_order(bdev_hardsect_size(bdev)) - 1;
#else
   hardware_sector_size_bit_order =  get_bitmask_order(bdev_logical_block_size(bdev)) - 1;
#endif
#endif

From Linux documentation, the user-space ioctl BLKSSZGET should behave the same way
and call the equivalent functions depending on the OS release:
version 2.6.32 and above:
        case BLKSSZGET: // get block device logical block size
                return put_int(arg, bdev_logical_block_size(bdev));
Version 2.6.18:
        case BLKSSZGET: // get block device hardware sector size
                 return put_int(arg, bdev_hardsect_size(bdev));

Other note: our driver checks if the Linux release is >= 2.6.0, but this logic is not put here
since RFX 272 cannot be installed on Linux releases older than 2.6.9 for Redhat (RH 4) and 2.6.16
for SuSE (SuSE 10).
*/ 
static int get_sector_size( char *device_name )
{
#if !defined(linux)
    // Non Linux OSes use a default constant (from our driver code, which we must be consistent with)
	return( DEV_BSIZE );
#else
// Linux
    int devfd;
    int blksize;

	// Linux code must use an ioctl to be consistent with what our driver does, as documented above
    if ((devfd = open(device_name, O_RDONLY)) == -1)
    {
        sprintf( message, "Error while opening device %s to determine its sector size [error: %s].\n", device_name, strerror(errno) );
	    display_and_log_message( message );
        return( -1 );
    }

    if (FTD_IOCTL_CALL(devfd, BLKSSZGET, &blksize) < 0)
    {
        sprintf( message, "Error in BLKSSZGET ioctl to get device %s sector size [error: %s].\n", device_name, strerror(errno) );
	    display_and_log_message( message );
		close( devfd );
        return( -1 );
    }

    if (blksize <= 0)
    {
        sprintf( message, "BLKSSZGET ioctl to get device %s sector size returned an invalid size.\n", device_name );
	    display_and_log_message( message );
		close( devfd );
        return( -1 );
    }

	close( devfd );
	return( blksize );
#endif
}

static ftd_uint64_t
get_limitsize_multiple_and_device_and_sector_size(int group, char *dtc_devname, unsigned long long *device_size_sectors, int *sector_size)
{
    char       cfg_file[MAXPATHLEN];
    sddisk_t   *temp;
    FILE *fp;
    char cmd[128];
    char data[8];
	int  local_sector_size;

    /* make file name */
    sprintf(cfg_file, "%s%03d.cfg", PMD_CFG_PREFIX, group);

    if (readconfig(1, 0, 0, cfg_file) < 0)
    {
        sprintf( message, "Error while reading configuration file %s to determine limitsize_multiple factor.\n", cfg_file );
	    display_and_log_message( message );
		*device_size_sectors = 0;
        return 0;
    }
    for (temp = mysys->group->headsddisk; temp != NULL; temp = temp->n)
    {
        if(strcmp(temp->sddevname, dtc_devname) == 0)
        {
            // Get the device sector size
			// NOTE: if an error occurs, we return a sector size of 0, which has to be detected by the caller, and continue
			// in this function to get the other information.
			if( (local_sector_size = get_sector_size(temp->devname)) < 0 )
			{
		        sprintf( message, "Error while getting device %s sector size.\n", temp->devname );
			    display_and_log_message( message );
				*sector_size = 0;
			}
			else
			{
				*sector_size = local_sector_size;
			}
            // Get the device size in sectors and the expansion factor
		    *device_size_sectors = temp->devsize;
            if (temp->limitsize_multiple == 0) 
                return (DEFAULT_LIMITSIZE_MULTIPLE);
            else 
                return temp->limitsize_multiple;
        }
    }
    sprintf( message, "In get_limitsize_multiple_and_device_size, did not find device %s\n", dtc_devname );
	display_and_log_message( message );
	*device_size_sectors = 0;
    return 0;

}

int check_pstore_space_for_migrating_RFX271_to_RFX272(char *ps_name, ps_hdr_t *header_RFX271)
{
    unsigned long long 	 needed_HRDB_space_KBs, new_HRDB_offset, hrdb_size_alignment;
    long long            dsize;
    char                 raw_name[MAXPATHLEN];
	ps_hdr_t			 header_RFX272;

    // Calculate the actually used HRDB space, then add it to the post-migration HRDB offset,
	// and check if we will exceed the Pstore size
	needed_HRDB_space_KBs = header_RFX271->data.ver1.next_available_HRDB_offset - header_RFX271->data.ver1.hrdb_offset;
    // If the HRDB mode is Proportional to device sizes, we use the Legacy Small HRDB size as alignment factor
    if (header_RFX271->data.ver1.hrdb_type == FTD_HS_PROPORTIONAL)
	{
	    hrdb_size_alignment =  FTD_PS_HRDB_SIZE_SMALL;
	}
	else
	{
	    hrdb_size_alignment = header_RFX271->data.ver1.Small_or_Large_hrdb_size;
	}

    // The only change in structure size from RFX271 to RFX272 is in the HRDB info table, just preceding HRDB space
#if defined(_AIX) || defined(linux)
     new_HRDB_offset = ((header_RFX271->data.ver1.dev_HRDB_info_table_offset * 1024 +
               header_RFX271->data.ver1.max_dev * sizeof(header_RFX272.data.ver1.dev_HRDB_info_entry_size) + hrdb_size_alignment-1) 
		       / hrdb_size_alignment) * (hrdb_size_alignment/1024);

#else
    new_HRDB_offset = header_RFX271->data.ver1.dev_HRDB_info_table_offset +
        (((header_RFX271->data.ver1.max_dev * sizeof(header_RFX272.data.ver1.dev_HRDB_info_entry_size) + 1023) / 1024));

#endif  /* _AIX and linux */

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

    // Get the Pstore size in terms if DEV_BSIZE bytes sectors
    if ((dsize = (long long)disksize(raw_name)) <= 0)
    {
        sprintf( message, "Error while determining size of %s [%s]\n", ps_name, strerror(errno));
	    display_and_log_message( message );
        return -1;
    }

    if( (dsize * DEV_BSIZE) < ((new_HRDB_offset + needed_HRDB_space_KBs) * 1024) )
	    return( -1 );
	else
	    return( 0 );
}


/*
 * returns the maximum number of devices a pstore can support given that it will have the legacy
 * Small or Large HRT format
 *
 */
int ps_max_devices_RFX272_Small_or_Large_HRDB_mode(char *ps_name, int *max_dev, int hrdb_size)
{
    unsigned long long  size, err;
    char                raw_name[MAXPATHLEN];
    u_longlong_t        dsize;

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
       	sprintf(message, "ps_max_devices_RFX272_Small_or_Large_HRDB_mode: could not determine the size of pstore %s.\n", ps_name);
	    display_and_log_message( message );
        return -1;
    }

    /* the size of one device assuming one device per group
       Note: dev_entry_2_t is not taken into account in calculating max devices in the actual product
             the legacy Small or Large HRT mode, so we must not take it into account here either
             (see create_ps in ftd_init.c). */
    size = FTD_PS_LRDB_SIZE + hrdb_size + FTD_PS_DEVICE_ATTR_SIZE +
           FTD_PS_GROUP_ATTR_SIZE + 3*MAXPATHLEN;
           // + sizeof(ps_dev_entry_2_t);

    /* Note: DEV_BSIZE is of different value on different OSes.
     * HPUX 1 KB, others 512 B
     */
    size = size / DEV_BSIZE;


#if defined(_AIX) || defined(linux)
    dsize = (daddr_t)((dsize - 32 -
                (FTD_PS_LRDB_SIZE+hrdb_size)/DEV_BSIZE ) / size);
#else
    dsize = (daddr_t)((dsize - 32) / size);
#endif

    if (dsize > FTD_MAX_DEVICES) {
        dsize = FTD_MAX_DEVICES;
    }

    *max_dev = dsize;

    return(0);
}

// Read a Pstore header of length specified in function argument
int get_header_common(char *source_name, void *buffer, int header_length)
{
    int      fd;

    /* open the pstore or file */
    if ((fd = open(source_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    if( llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
        close(fd);
        return PS_SEEK_ERROR;
	}

    /* read the header */
    if( read(fd, buffer, header_length) != header_length )
	{
        close(fd);
	    return PS_READ_ERROR;
	}
    close(fd);

    return( PS_OK );
}

/*
 *  writes a header into a temporary file or a pstore
 */
int set_header_common(char *target_name, void *buffer, int header_length)
{
    int fd;

    if (!access(target_name, F_OK))
    {
 	    if ((fd = open(target_name, O_RDWR | O_SYNC)) == -1)
 	    {
       		sprintf(message, "Could not open the target file or device (%s) to set the header in it.\n", target_name);
	        display_and_log_message( message );
			return(PS_BOGUS_PS_NAME);
	    }
	}
	else
	{
	    if ((fd = open(target_name, O_CREAT | O_RDWR, S_IRWXU)) == -1)
	    {
       		sprintf(message, "Could not create a temporary file to enable migration\n");
	        display_and_log_message( message );
			return(PS_BOGUS_PS_NAME);
	    }
	}
    if (llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET) == -1) {
       	sprintf(message, "Error seeking to header offset of %s\n", target_name );
	    display_and_log_message( message );
        close(fd);
		return(PS_SEEK_ERROR);
    }
    if (write(fd, buffer, header_length) != header_length) {
       	sprintf(message, "Error writing header to %s\n", target_name );
	    display_and_log_message( message );
        close(fd);
		return(PS_WRITE_ERROR);
    }
    close(fd);
    return(PS_OK);
}


int migrate_RFX271_device_data_to_RFX272(char *source_name, char *target_name)
{
    int             infd, outfd, i = 0, j;
    ps_hdr_t        inhdr, outhdr;	// RFX271 and 272 format are the same

    unsigned int    table_size_271, table_size_272, offset;
    ps_dev_entry_RFX271_t *dev_table271 = NULL;
    ps_dev_entry_t  dev_table272;

	ps_dev_entry_2_RFX271_t *HRDB_info_table271;
	ps_dev_entry_2_t        HRDB_info_table272;
    unsigned int            HRDB_table_size_271, HRDB_table_size_272;
	unsigned int            next_available_hrdb_offset_in_KBs;

    char 	        *buffer = NULL;
	int             result1, result2;

    if ((infd = open(source_name, O_RDWR | O_SYNC)) == -1)
    {
        sprintf( message, "migrate_RFX271_device_data_to_RFX272: error opening source %s, errno = %d.\n", source_name, errno );
	    display_and_log_message( message );
        return PS_BOGUS_PS_NAME;
    }

    if ((outfd = open(target_name, O_RDWR | O_SYNC)) == -1)
    {
        sprintf( message, "migrate_RFX271_device_data_to_RFX272: error opening target %s, errno = %d.\n", target_name, errno );
	    display_and_log_message( message );
	    close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    result1 = llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET);
    result2 = llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET);
	if( result1 == -1 || result2 == -1 )
	{
        sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to header of source %s or target %s.\n", source_name, target_name );
	    display_and_log_message( message );
	    close(infd);
	    close(outfd);
	    return PS_SEEK_ERROR;
	}
    /* read the headers */
    result1 = (read(infd, &inhdr, sizeof(ps_hdr_t)) == sizeof(ps_hdr_t));
    result2 = (read(outfd, &outhdr, sizeof(ps_hdr_t)) == sizeof(ps_hdr_t));
	if( !(result1 && result2) )
	{
        sprintf( message, "migrate_RFX271_device_data_to_RFX272: error reading header of source %s or target %s.\n", source_name, target_name );
	    display_and_log_message( message );
	    close(infd);
	    close(outfd);
	    return PS_READ_ERROR;
	}

    // Seek to the device table location
    if (llseek(infd, inhdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1)
    {
        sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to source %s device table.\n", source_name );
	    display_and_log_message( message );
        goto mig271_devcleanup;
    }
	table_size_271 = sizeof(ps_dev_entry_RFX271_t) * inhdr.data.ver1.max_dev;
	dev_table271 = (ps_dev_entry_RFX271_t *)ftdcalloc(1, table_size_271);
    if (read(infd, (caddr_t)dev_table271, table_size_271) != table_size_271)
    {
        sprintf( message, "migrate_RFX271_device_data_to_RFX272: error reading source %s device table.\n", source_name );
	    display_and_log_message( message );
		goto mig271_devcleanup;
    }

    // Seek to the device HRDB info table location (table 2) and read it before the loop
    if (llseek(infd, inhdr.data.ver1.dev_HRDB_info_table_offset * 1024, SEEK_SET) == -1)
    {
        sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to source %s HRDB info table.\n", source_name );
	    display_and_log_message( message );
        goto mig271_devcleanup;
    }
	HRDB_table_size_271 = sizeof(ps_dev_entry_2_RFX271_t) * inhdr.data.ver1.max_dev;
	HRDB_info_table271 = (ps_dev_entry_2_RFX271_t *)ftdcalloc(1, HRDB_table_size_271);
    if (read(infd, (caddr_t)HRDB_info_table271, HRDB_table_size_271) != HRDB_table_size_271)
    {
        sprintf( message, "migrate_RFX271_device_data_to_RFX272: error reading source %s HRDB info table.\n", source_name );
	    display_and_log_message( message );
		goto mig271_devcleanup;
    }

    j = 0;
    for (i = 0; i < inhdr.data.ver1.max_dev; i++)
    {
		if( (strlen(dev_table271[i].path) == strspn(dev_table271[i].path, " ")) || (dev_table271[i].pathlen == 0) )
		{	// Do not copy empty entries
		    continue;
		}
		/*
		 * Migrate device table
		 */
	    memset( dev_table272.path, 0, MAXPATHLEN );
        strncpy( dev_table272.path, dev_table271[i].path, MAXPATHLEN );
		dev_table272.pathlen = dev_table271[i].pathlen;
		dev_table272.ps_allocated_lrdb_bits = dev_table271[i].num_lrdb_bits;
		dev_table272.ps_allocated_hrdb_bits = dev_table271[i].num_hrdb_bits;
		dev_table272.ps_valid_lrdb_bits = 0;  // These 0 values will be set at the first dtcstart after migration
		dev_table272.ps_valid_hrdb_bits = 0;
		dev_table272.ackoff = dev_table271[i].ackoff;

		offset = (outhdr.data.ver1.dev_table_offset * 1024) + (j * sizeof(ps_dev_entry_t));
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to target %s device table entry.\n", target_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
        }
		if (write(outfd, &dev_table272, sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t))
		{
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error writing target %s device table entry.\n", target_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
	    }
		/*
		 * Migrate (copy) device attributes
		 */
    	buffer = ftdcalloc(1, inhdr.data.ver1.dev_attr_size);
    	offset = (inhdr.data.ver1.dev_attr_offset * 1024) +	(i * inhdr.data.ver1.dev_attr_size);
    	if (llseek(infd, offset, SEEK_SET) == -1)
    	{
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to source %s device attribute entry.\n", source_name );
	        display_and_log_message( message );
       		goto mig271_devcleanup;
    	}
    	if (read(infd, buffer, inhdr.data.ver1.dev_attr_size) != inhdr.data.ver1.dev_attr_size)
    	{
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error reading source %s device attribute entry.\n", source_name );
	        display_and_log_message( message );
       		goto mig271_devcleanup;
    	}
    	offset = (outhdr.data.ver1.dev_attr_offset * 1024) + (j * outhdr.data.ver1.dev_attr_size);
    	if (llseek(outfd, offset, SEEK_SET) == -1)
    	{
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to target %s device attribute entry.\n", target_name );
	        display_and_log_message( message );
       		goto mig271_devcleanup;
    	}
    	if (write(outfd, buffer, outhdr.data.ver1.dev_attr_size) != outhdr.data.ver1.dev_attr_size)
    	{
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error writing target %s device attribute entry.\n", target_name );
	        display_and_log_message( message );
       		goto mig271_devcleanup;
    	}
	    free(buffer);
		buffer = NULL;
        /*
         * copy LRDB; note: RFX271 pstores do not need bit recalculation (PROD7660) even in Legacy modes
         */
	    offset = (inhdr.data.ver1.lrdb_offset * 1024) + (i * inhdr.data.ver1.lrdb_size);
        if (llseek(infd, offset, SEEK_SET) == -1)
        {
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to source %s device LRDB.\n", source_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
        }
        buffer = ftdcalloc(1, inhdr.data.ver1.lrdb_size);
        if (read(infd, buffer, inhdr.data.ver1.lrdb_size) != inhdr.data.ver1.lrdb_size) {
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error reading source %s device LRDB.\n", source_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
        }
        offset = (outhdr.data.ver1.lrdb_offset * 1024) + (j * outhdr.data.ver1.lrdb_size);
        if (llseek(outfd, offset, SEEK_SET) == -1) {
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to target %s device LRDB.\n", target_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
        }
        if (write(outfd, buffer, outhdr.data.ver1.lrdb_size) != outhdr.data.ver1.lrdb_size) {
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error writing target %s device LRDB.\n", target_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
        }
        free(buffer);
		buffer = NULL;

		/*
		 * Migrate device HRDB info table (table 2)
		 */
	    memset( HRDB_info_table272.path, 0, MAXPATHLEN );
        strncpy( HRDB_info_table272.path, HRDB_info_table271[i].path, MAXPATHLEN );
		HRDB_info_table272.pathlen = HRDB_info_table271[i].pathlen;

		HRDB_info_table272.hrdb_resolution_KBs_per_bit = HRDB_info_table271[i].hrdb_resolution_KBs_per_bit;
		HRDB_info_table272.hrdb_size = HRDB_info_table271[i].hrdb_size;
		HRDB_info_table272.dev_HRDB_offset_in_KBs = HRDB_info_table271[i].dev_HRDB_offset_in_KBs;
		HRDB_info_table272.has_reusable_HRDB = HRDB_info_table271[i].has_reusable_HRDB;
		HRDB_info_table272.num_sectors_64bits = HRDB_info_table271[i].num_sectors_64bits;
		HRDB_info_table272.orig_num_sectors_64bits = HRDB_info_table271[i].orig_num_sectors_64bits;
		HRDB_info_table272.limitsize_multiple = HRDB_info_table271[i].limitsize_multiple;
		HRDB_info_table272.lrdb_res_shift_count = 0;
		HRDB_info_table272.lrdb_res_sectors_per_bit = 0;

		offset = (outhdr.data.ver1.dev_HRDB_info_table_offset * 1024) + (j * sizeof(ps_dev_entry_2_t));
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to target %s HRDB info table entry.\n", target_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
        }
		if (write(outfd, &HRDB_info_table272, sizeof(ps_dev_entry_2_t)) != sizeof(ps_dev_entry_2_t))
		{
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error writing target %s HRDB info table entry.\n", target_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
	    }

        /*
         * copy HRDB
         */
        offset = HRDB_info_table271[i].dev_HRDB_offset_in_KBs * 1024;
        if (llseek(infd, offset, SEEK_SET) == -1)
        {
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to source %s device HRDB.\n", source_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
        }
        buffer = ftdcalloc(1, HRDB_info_table271[i].hrdb_size);
        if (read(infd, buffer, HRDB_info_table271[i].hrdb_size) != HRDB_info_table271[i].hrdb_size) {
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error reading source %s device HRDB.\n", source_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
        }
        offset = HRDB_info_table272.dev_HRDB_offset_in_KBs * 1024;
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to target %s device HRDB.\n", target_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
        }
        if (write(outfd, buffer, HRDB_info_table272.hrdb_size) != HRDB_info_table272.hrdb_size) {
            sprintf( message, "migrate_RFX271_device_data_to_RFX272: error writing target %s device HRDB.\n", target_name );
	        display_and_log_message( message );
            goto mig271_devcleanup;
        }
		// Update next available hrdb offset in KBs
		next_available_hrdb_offset_in_KBs = (unsigned int)(((offset + HRDB_info_table272.hrdb_size) + 1023) / 1024);
        free(buffer);
		buffer = NULL;

	    j++;
    } //... next device...

    outhdr.data.ver1.last_device = j - 1;
	outhdr.data.ver1.next_available_HRDB_offset = next_available_hrdb_offset_in_KBs;
    if( llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
        sprintf( message, "migrate_RFX271_device_data_to_RFX272: error seeking to target %s header to update its last_device and next_available HRDB offset entries.\n", target_name );
	    display_and_log_message( message );
        i = 0;	 // indicate migration is not valid
        goto mig271_devcleanup;
    }
    if( write(outfd, &outhdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t) )
	{
        sprintf( message, "migrate_RFX271_device_data_to_RFX272: error writing target %s header to update its last_device and next_available HRDB offset entries.\n", target_name );
	    display_and_log_message( message );
        i = 0;	 // indicate migration is not valid
    }

mig271_devcleanup:
	if( dev_table271 != NULL ) free(dev_table271);
	if( HRDB_info_table271 != NULL ) free(HRDB_info_table271);
	if( buffer != NULL ) free(buffer);
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_dev)
    {
    	return PS_OK;
    }
    else
    {
	    return PS_ERR;
    }
}

int migrate_RFX270_device_data_to_RFX272(char *source_name, char *target_name)
{
  // WARNING: it is mandatory to migrate the groups data before the devices data;
  // so here we expect that it has been done before calling this function here.

    int             infd, outfd, i = 0, j;
    ps_hdr_RFX270_t inhdr;
    ps_hdr_t        outhdr;	// RFX272 format
    unsigned int    table_size_270, table_size_272, offset;
    ps_dev_entry_RFX270_t *table270 = NULL;
    ps_dev_entry_t  table272;
    char 	        *buffer = NULL;
    char 	        *buffer2 = NULL;
	int             result1, result2;
	unsigned int    next_available_hrdb_offset_in_KBs;
	ps_dev_entry_2_t  HRDB_info_table272;
    unsigned int      HRDB_table_size_272;
	int               lgnum, n;
	char              *p;
	char              group_name[MAXPATHLEN];
    ps_group_info_t   ginfo;
    ftd_uint64_t	  limitsize_multiple;
	unsigned long long device_size_sectors;
	int               sector_size = 0;

    if ((infd = open(source_name, O_RDWR | O_SYNC)) == -1)
    {
	    sprintf( message, "migrate_RFX270_device_data_to_RFX272: error opening source %s, errno = %d.\n", source_name, errno );
	    display_and_log_message( message );
        return PS_BOGUS_PS_NAME;
    }

    if ((outfd = open(target_name, O_RDWR | O_SYNC)) == -1)
    {
	    sprintf( message, "migrate_RFX270_device_data_to_RFX272: error opening target %s, errno = %d.\n", target_name, errno );
	    display_and_log_message( message );
	    close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    result1 = llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET);
    result2 = llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET);
	if( result1 == -1 || result2 == -1 )
	{
	    sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to header of source %s or target %s.\n", source_name, target_name );
	    display_and_log_message( message );
	    close(infd);
	    close(outfd);
	    return PS_SEEK_ERROR;
	}
    /* read the headers */
    result1 = (read(infd, &inhdr, sizeof(ps_hdr_RFX270_t)) == sizeof(ps_hdr_RFX270_t));
    result2 = (read(outfd, &outhdr, sizeof(ps_hdr_t)) == sizeof(ps_hdr_t));
	if( !(result1 && result2) )
	{
	    sprintf( message, "migrate_RFX270_device_data_to_RFX272: error reading header of source %s or target %s.\n", source_name, target_name );
	    display_and_log_message( message );
	    close(infd);
	    close(outfd);
	    return PS_READ_ERROR;
	}

    // Seek to the device table location
    if (llseek(infd, inhdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1)
    {
	    sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to source %s device table.\n", source_name );
	    display_and_log_message( message );
        goto mig270_devcleanup;
    }
	table_size_270 = sizeof(ps_dev_entry_RFX270_t) * inhdr.data.ver1.max_dev;
	table270 = (ps_dev_entry_RFX270_t *)ftdcalloc(1, table_size_270);
    if (read(infd, (caddr_t)table270, table_size_270) != table_size_270)
    {
	    sprintf( message, "migrate_RFX270_device_data_to_RFX272: error reading source %s device table.\n", source_name );
	    display_and_log_message( message );
		goto mig270_devcleanup;
    }
    j = 0;
    for (i = 0; i < inhdr.data.ver1.max_dev; i++)
    {
		if( (strlen(table270[i].path) == strspn(table270[i].path, " ")) || (table270[i].pathlen == 0) )
		{	// Do not copy empty entries
		    continue;
		}
		/*
		 * Migrate device table
		 */
	    memset( table272.path, 0, MAXPATHLEN );
        strncpy( table272.path, table270[i].path, MAXPATHLEN );
		table272.pathlen = table270[i].pathlen;
		table272.ps_allocated_lrdb_bits = table270[i].num_lrdb_bits;
		table272.ps_allocated_hrdb_bits = table270[i].num_hrdb_bits;
		table272.ps_valid_lrdb_bits = 0;  // These 0 values will be set at the first dtcstart after migration
		table272.ps_valid_hrdb_bits = 0;
		table272.ackoff = table270[i].ackoff;

		offset = (outhdr.data.ver1.dev_table_offset * 1024) + (j * sizeof(ps_dev_entry_t));
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to target %s device table entry.\n", target_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }
		if (write(outfd, &table272, sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t))
		{
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error writing target %s device table entry.\n", target_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
	    }
		/*
		 * Migrate (copy) device attributes
		 */
    	buffer = ftdcalloc(1, inhdr.data.ver1.dev_attr_size);
    	offset = (inhdr.data.ver1.dev_attr_offset * 1024) +	(i * inhdr.data.ver1.dev_attr_size);
    	if (llseek(infd, offset, SEEK_SET) == -1)
    	{
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to source %s device attribute entry.\n", source_name );
	        display_and_log_message( message );
       		goto mig270_devcleanup;
    	}
    	if (read(infd, buffer, inhdr.data.ver1.dev_attr_size) != inhdr.data.ver1.dev_attr_size)
    	{
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error reading source %s device attribute entry.\n", source_name );
	        display_and_log_message( message );
       		goto mig270_devcleanup;
    	}
    	offset = (outhdr.data.ver1.dev_attr_offset * 1024) + (j * outhdr.data.ver1.dev_attr_size);
    	if (llseek(outfd, offset, SEEK_SET) == -1)
    	{
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to target %s device attribute entry.\n", target_name );
	        display_and_log_message( message );
       		goto mig270_devcleanup;
    	}
    	if (write(outfd, buffer, outhdr.data.ver1.dev_attr_size) != outhdr.data.ver1.dev_attr_size)
    	{
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error writing target %s device attribute entry.\n", target_name );
	        display_and_log_message( message );
       		goto mig270_devcleanup;
    	}
	    free(buffer);
		buffer = NULL;

	    // Deduce the group number from the dtc device name, which has the format, for instance: /dev/dtc/lg19/rdsk/dtc99
	    lgnum = 0;
	    p = strstr(table270[i].path, "/dev/dtc/lg");
	    if( p != NULL)
		{
		    p += 11;
		    for (n=0; p[n] != '/'; n++)
		    {
		        if (isdigit(p[n]))
		        {
		            lgnum *= 10;
		            lgnum += (int)(p[n] - '0');
		        }
				else
				{
		            sprintf( message, "migrate_RFX270_device_data_to_RFX272: error determining the group number for device %s.\n", table270[i].path );
	                display_and_log_message( message );
		            goto mig270_devcleanup;
				}
		    }
		}
		else
		{
            sprintf( message, "migrate_RFX270_device_data_to_RFX272: error determining the group number for device %s.\n", table270[i].path );
	        display_and_log_message( message );
            goto mig270_devcleanup;
		}
        /* Get the group state from the target pstore info */
        FTD_CREATE_GROUP_NAME(group_name, lgnum);
        memset(&ginfo, 0, sizeof(ginfo));
        if ((result1 = ps_get_group_info(target_name, group_name, &ginfo)) != PS_OK)
        {
            if (result1 == PS_GROUP_NOT_FOUND) {
                sprintf(message,"Group %s does not exist in %s.\n", group_name, target_name);
	            display_and_log_message( message );
            } else {
                sprintf(message,"PSTORE data from %s, error: %d [%s]", target_name, result1, ps_get_pstore_error_string(result1));
	            display_and_log_message( message );
            }
            goto mig270_devcleanup;
        }
        /*
         * Migrate LRDB: must recalculate all bits due to optimization of PROD7660, unless the group is in passthru mode
         */
		limitsize_multiple = get_limitsize_multiple_and_device_and_sector_size(lgnum, table270[i].path, &device_size_sectors, &sector_size);
        if ((device_size_sectors == 0) || ((long long)device_size_sectors == -1))
        {
            sprintf( message, "migrate_RFX270_device_data_to_RFX272: error determining the size of device %s.\n", table270[i].path );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }

	    offset = (inhdr.data.ver1.lrdb_offset * 1024) + (i * inhdr.data.ver1.lrdb_size);
        if (llseek(infd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to source %s device LRDB.\n", source_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }
        buffer = ftdcalloc(1, inhdr.data.ver1.lrdb_size);
        buffer2 = ftdcalloc(1, outhdr.data.ver1.lrdb_size);	// For recalculated bits
	    memset( buffer2, 0, outhdr.data.ver1.lrdb_size );

        if (read(infd, buffer, inhdr.data.ver1.lrdb_size) != inhdr.data.ver1.lrdb_size) {
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error reading source %s device LRDB.\n", source_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }
		// Recalculate the bits in buffer2
		// NOTE: this should not be called for a group in PASSTHRU mode, whose bits are all dirty
		//       up to the end of the allocated space, nor for the HRDB of a group using Large HRT mode,
		//       to which the optimization of PROD7660 did not apply.
        if ( ginfo.state == FTD_MODE_PASSTHRU )
        {
		    sprintf( message, "NOTE: %s is in PASSTHRU, copying its LRDB as is (not recalculating it). This is not an error but just for info purposes.\n", table270[i].path );
	        display_and_log_message( message );
		    memcpy( buffer2, buffer, inhdr.data.ver1.lrdb_size );
		}
		else
		{
	        if( recalculate_dirty_bits( (unsigned int *)buffer, (unsigned int *)buffer2,
	                        inhdr.data.ver1.lrdb_size / 4, outhdr.data.ver1.lrdb_size / 4,
	                        device_size_sectors, limitsize_multiple, 1, table270[i].path, sector_size ) != 0 )
			{
		        sprintf( message, "Error occurred while converting the LRDB tracking bits to RFX272 enhanced precision.\n" );
	            display_and_log_message( message );
	            goto mig270_devcleanup;
			}
		}

        offset = (outhdr.data.ver1.lrdb_offset * 1024) + (j * outhdr.data.ver1.lrdb_size);
        if (llseek(outfd, offset, SEEK_SET) == -1) {
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to target %s device LRDB.\n", target_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }
        if (write(outfd, buffer2, outhdr.data.ver1.lrdb_size) != outhdr.data.ver1.lrdb_size) {
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error writing target %s device LRDB.\n", target_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }
        free(buffer);
        free(buffer2);
		buffer = buffer2 = NULL;
		/*
		 * Create device HRDB info table entry (table 2)
		 */
	    memset( HRDB_info_table272.path, 0, MAXPATHLEN );
        strncpy( HRDB_info_table272.path, table270[i].path, MAXPATHLEN );
		HRDB_info_table272.pathlen = table270[i].pathlen;
		HRDB_info_table272.hrdb_size = outhdr.data.ver1.Small_or_Large_hrdb_size;
		HRDB_info_table272.dev_HRDB_offset_in_KBs = (((outhdr.data.ver1.hrdb_offset * 1024) + (j * outhdr.data.ver1.Small_or_Large_hrdb_size)) +1023) / 1024;
		HRDB_info_table272.has_reusable_HRDB = 0;
		HRDB_info_table272.lrdb_res_shift_count = 0; // Will be adjusted by dtcstart
		HRDB_info_table272.lrdb_res_sectors_per_bit = 0; // Will be adjusted by dtcstart
		HRDB_info_table272.hrdb_resolution_KBs_per_bit = 0; // Will be adjusted by dtcstart
		HRDB_info_table272.limitsize_multiple = limitsize_multiple;
		HRDB_info_table272.num_sectors_64bits = device_size_sectors;
		HRDB_info_table272.orig_num_sectors_64bits = HRDB_info_table272.num_sectors_64bits;

		offset = (outhdr.data.ver1.dev_HRDB_info_table_offset * 1024) + (j * sizeof(ps_dev_entry_2_t));
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
            sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to target %s HRDB info table entry.\n", target_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }
		if (write(outfd, &HRDB_info_table272, sizeof(ps_dev_entry_2_t)) != sizeof(ps_dev_entry_2_t))
		{
            sprintf( message, "migrate_RFX270_device_data_to_RFX272: error writing target %s HRDB info table entry.\n", target_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
	    }

        /*
         * Migrate HRDB
         */
        buffer = ftdcalloc(1, inhdr.data.ver1.hrdb_size);
        buffer2 = ftdcalloc(1, outhdr.data.ver1.Small_or_Large_hrdb_size);	// For recalculated bits
	    memset( buffer2, 0, outhdr.data.ver1.Small_or_Large_hrdb_size );

        offset = (inhdr.data.ver1.hrdb_offset * 1024) +	(i * inhdr.data.ver1.hrdb_size);
        if (llseek(infd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to source %s device HRDB.\n", source_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }
        if (read(infd, buffer, inhdr.data.ver1.hrdb_size) != inhdr.data.ver1.hrdb_size) {
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error reading source %s device HRDB.\n", source_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }

		// Recalculate the bits in buffer2 if we are in Small HRT mode and not in PASSTHRU
		// We can check	the HRDB type in the target buffer because it preserves the same type as the source HRDB
		// and is already formatted for this.
		if( outhdr.data.ver1.hrdb_type == FTD_HS_SMALL )
		{
		    if( ginfo.state != FTD_MODE_PASSTHRU )
			{
		        if( recalculate_dirty_bits( (unsigned int *)buffer, (unsigned int *)buffer2,
		                        inhdr.data.ver1.hrdb_size / 4, outhdr.data.ver1.Small_or_Large_hrdb_size / 4,
		                        device_size_sectors, limitsize_multiple, 0, table270[i].path, sector_size ) != 0 )
				{
		            sprintf( message, "Error occurred while converting the HRDB tracking bits to RFX272 enhanced precision.\n" );
		            display_and_log_message( message );
		            goto mig270_devcleanup;
				}
			}
			else
			{
			    // The group is in Small HRT mode but in PASSTHRU: log an information message.
		        sprintf( message, "NOTE: %s is in PASSTHRU, copying its HRDB as is (not recalculating it). This is not an error but just for info purposes.\n", table270[i].path );
	            display_and_log_message( message );
		        memcpy( buffer2, buffer, outhdr.data.ver1.Small_or_Large_hrdb_size );
			}
		}
		else // Large HRT, we just copy the HRDB as is, regardless whether the group is in PASSTHRU or not (no message logged here)
		{
		    memcpy( buffer2, buffer, outhdr.data.ver1.Small_or_Large_hrdb_size );
		}

        offset = (outhdr.data.ver1.hrdb_offset * 1024) + (j * outhdr.data.ver1.Small_or_Large_hrdb_size);
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to target %s device HRDB.\n", target_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }
        if (write(outfd, buffer2, outhdr.data.ver1.Small_or_Large_hrdb_size) != outhdr.data.ver1.Small_or_Large_hrdb_size)
        {
	        sprintf( message, "migrate_RFX270_device_data_to_RFX272: error writing target %s device HRDB.\n", target_name );
	        display_and_log_message( message );
            goto mig270_devcleanup;
        }
		// Update next available hrdb offset in KBs
		next_available_hrdb_offset_in_KBs = (unsigned int)(((offset + outhdr.data.ver1.Small_or_Large_hrdb_size) + 1023) / 1024);

        free(buffer);
        free(buffer2);
		buffer = buffer2 = NULL;

	    j++;
    } //... next device...

    outhdr.data.ver1.last_device = j - 1;
	outhdr.data.ver1.next_available_HRDB_offset = next_available_hrdb_offset_in_KBs;
    if( llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
	    sprintf( message, "migrate_RFX270_device_data_to_RFX272: error seeking to target %s header to update its last_device and next_available HRDB offset entries.\n", target_name );
	    display_and_log_message( message );
        i = 0;	 // indicate migration is not valid
        goto mig270_devcleanup;
    }
    if( write(outfd, &outhdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t) )
	{
	    sprintf( message, "migrate_RFX270_device_data_to_RFX272: error writing target %s header to update its last_device and next_available HRDB offset entries.\n", target_name );
	    display_and_log_message( message );
        i = 0;	 // indicate migration is not valid
    }

mig270_devcleanup:
	if( table270 != NULL ) free(table270);
	if( buffer != NULL ) free(buffer);
	if( buffer2 != NULL ) free(buffer2);
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_dev)
    {
    	return PS_OK;
    }
    else
    {
	    return PS_ERR;
    }
}


// Copy device data from an RFX271 Pstore from or to a file; it is assumed 
// that the header is already set in the target file or device.
int ps_copy_RFX271_device_data(char *source_name, char *target_name)
{
    int             infd, outfd, i = 0, j;
    ps_hdr_t        inhdr, outhdr;
    unsigned int    table_size, table2_size, offset;
    char 	        *buffer = NULL;
	int             result1, result2;
    ps_dev_entry_RFX271_t *table = NULL;
    ps_dev_entry_2_RFX271_t *table2 = NULL;
	int            device_HRDB_offset, device_HRDB_size;

    if ((infd = open(source_name, O_RDWR | O_SYNC)) == -1)
    {
	    sprintf(message, "ps_copy_RFX271_device_data: error opening %s\n", source_name);
	    display_and_log_message( message );
        return PS_BOGUS_PS_NAME;
    }

    if ((outfd = open(target_name, O_RDWR | O_SYNC)) == -1)
    {
	    sprintf(message, "ps_copy_RFX271_device_data: error opening %s\n", target_name);
	    display_and_log_message( message );
	    close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    result1 = llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET);
    result2 = llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET);
	if( result1 == -1 || result2 == -1 )
	{
	    sprintf(message, "ps_copy_RFX271_device_data: error seeking to header of source %s or target %s\n", source_name, target_name);
	    display_and_log_message( message );
	    close(infd);
	    close(outfd);
	    return PS_SEEK_ERROR;
	}
    /* read the headers */
    result1 = ( read(infd, &inhdr, sizeof(ps_hdr_t)) == sizeof(ps_hdr_t) );
    result2 = ( read(outfd, &outhdr, sizeof(ps_hdr_t)) == sizeof(ps_hdr_t) );
	if( !(result1 && result2) )
	{
	    sprintf(message, "ps_copy_RFX271_device_data: error reading header of source %s or target %s\n", source_name, target_name);
	    display_and_log_message( message );
	    close(infd);
	    close(outfd);
	    return PS_READ_ERROR;
	}

    // Seek to the device table location
    if (llseek(infd, inhdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1)
    {
	    sprintf(message, "ps_copy_RFX271_device_data: error seeking to source %s device table\n", source_name);
	    display_and_log_message( message );
        goto devcleanup271;
    }
	table_size = sizeof(ps_dev_entry_RFX271_t) * inhdr.data.ver1.max_dev;
	table = (ps_dev_entry_RFX271_t *)ftdcalloc(1, table_size);
    if (read(infd, (caddr_t)table, table_size) != table_size)
    {
	    sprintf(message, "ps_copy_RFX271_device_data: error reading source %s device table\n", source_name);
	    display_and_log_message( message );
		goto devcleanup271;
    }

    // Seek to the device HRDB info table location (table 2)
    if (llseek(infd, inhdr.data.ver1.dev_HRDB_info_table_offset * 1024, SEEK_SET) == -1)
    {
	    sprintf(message, "ps_copy_RFX271_device_data: error seeking to source %s HRDB info table\n", source_name);
	    display_and_log_message( message );
        goto devcleanup271;
    }
	table2_size = sizeof(ps_dev_entry_2_RFX271_t) * inhdr.data.ver1.max_dev;
	table2 = (ps_dev_entry_2_RFX271_t *)ftdcalloc(1, table2_size);
    if (read(infd, (caddr_t)table2, table2_size) != table2_size)
    {
	    sprintf(message, "ps_copy_RFX271_device_data: error reading source %s HRDB info table\n", source_name);
	    display_and_log_message( message );
		goto devcleanup271;
    }

    j = 0;  // Target device index, in case it differs from source device index, which is possible if there were empty source entries

    // Copy the various elements of all devices
    for (i = 0; i < inhdr.data.ver1.max_dev; i++)
    {
		if( (strlen(table[i].path) == strspn(table[i].path, " ")) || (table[i].pathlen == 0) )
		{	// Do not copy empty entries
		    continue;
		}
		/*
		 * copy device table
		 */
		offset = (outhdr.data.ver1.dev_table_offset * 1024) + (j * sizeof(ps_dev_entry_RFX271_t));
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
	        sprintf(message, "ps_copy_RFX271_device_data: error seeking to target %s device table entry\n", target_name);
	        display_and_log_message( message );
            goto devcleanup271;
        }
		if (write(outfd, &table[i], sizeof(ps_dev_entry_RFX271_t)) != sizeof(ps_dev_entry_RFX271_t))
		{
	        sprintf(message, "ps_copy_RFX271_device_data: error writing target %s device table entry\n", target_name);
	        display_and_log_message( message );
            goto devcleanup271;
	    }
		/*
		 * copy device attributes
		 */
    	buffer = ftdcalloc(1, inhdr.data.ver1.dev_attr_size);
    	offset = (inhdr.data.ver1.dev_attr_offset * 1024) +	(i * inhdr.data.ver1.dev_attr_size);
    	if (llseek(infd, offset, SEEK_SET) == -1)
    	{
	        sprintf(message, "ps_copy_RFX271_device_data: error seeking to source %s device attribute entry\n", source_name);
	        display_and_log_message( message );
       		goto devcleanup271;
    	}
    	if (read(infd, buffer, inhdr.data.ver1.dev_attr_size) != inhdr.data.ver1.dev_attr_size)
    	{
	        sprintf(message, "ps_copy_RFX271_device_data: error reading source %s device attribute entry\n", source_name);
	        display_and_log_message( message );
       		goto devcleanup271;
    	}
    	offset = (outhdr.data.ver1.dev_attr_offset * 1024) + (j * outhdr.data.ver1.dev_attr_size);
    	if (llseek(outfd, offset, SEEK_SET) == -1)
    	{
	        sprintf(message, "ps_copy_RFX271_device_data: error seeking to target %s device attribute entry\n", target_name);
	        display_and_log_message( message );
       		goto devcleanup271;
    	}
    	if (write(outfd, buffer, outhdr.data.ver1.dev_attr_size) != outhdr.data.ver1.dev_attr_size)
    	{
	        sprintf(message, "ps_copy_RFX271_device_data: error writing target %s device attribute entry\n", target_name);
	        display_and_log_message( message );
       		goto devcleanup271;
    	}
	    free(buffer);
		buffer = NULL;
        /*
         * copy LRDB
         */
	    offset = (inhdr.data.ver1.lrdb_offset * 1024) + (i * inhdr.data.ver1.lrdb_size);
        if (llseek(infd, offset, SEEK_SET) == -1)
        {
	        sprintf(message, "ps_copy_RFX271_device_data: error seeking to source %s LRDB\n", source_name);
	        display_and_log_message( message );
            goto devcleanup271;
        }
        buffer = ftdcalloc(1, inhdr.data.ver1.lrdb_size);
        if (read(infd, buffer, inhdr.data.ver1.lrdb_size) != inhdr.data.ver1.lrdb_size) {
	        sprintf(message, "ps_copy_RFX271_device_data: error reading source %s LRDB\n", source_name);
	        display_and_log_message( message );
            goto devcleanup271;
        }
        offset = (outhdr.data.ver1.lrdb_offset * 1024) + (j * outhdr.data.ver1.lrdb_size);
        if (llseek(outfd, offset, SEEK_SET) == -1) {
	        sprintf(message, "ps_copy_RFX271_device_data: error seeking to target %s LRDB\n", target_name);
	        display_and_log_message( message );
            goto devcleanup271;
        }
        if (write(outfd, buffer, outhdr.data.ver1.lrdb_size) != outhdr.data.ver1.lrdb_size) {
	        sprintf(message, "ps_copy_RFX271_device_data: error writing target %s LRDB\n", target_name);
	        display_and_log_message( message );
            goto devcleanup271;
        }
        free(buffer);
		buffer = NULL;

		/*
		 * copy device HRDB info table (table 2)
		 */
		offset = (outhdr.data.ver1.dev_HRDB_info_table_offset * 1024) + (j * sizeof(ps_dev_entry_2_RFX271_t));
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
	        sprintf(message, "ps_copy_RFX271_device_data: error seeking to target %s HRDB info table entry\n", target_name);
	        display_and_log_message( message );
            goto devcleanup271;
        }
		if (write(outfd, &table2[i], sizeof(ps_dev_entry_2_RFX271_t)) != sizeof(ps_dev_entry_2_RFX271_t))
		{
	        sprintf(message, "ps_copy_RFX271_device_data: error writing target %s HRDB info table entry\n", target_name);
	        display_and_log_message( message );
            goto devcleanup271;
	    }

		// Take note of this device's source HRDB size and offset
		device_HRDB_offset =  table2[i].dev_HRDB_offset_in_KBs;
		device_HRDB_size = 	table2[i].hrdb_size;
        /*
         * copy HRDB
         */
        if (llseek(infd, device_HRDB_offset*1024, SEEK_SET) == -1)
        {
	        sprintf(message, "ps_copy_RFX271_device_data: error seeking to source %s HRDB\n", source_name);
	        display_and_log_message( message );
            goto devcleanup271;
        }
        buffer = ftdcalloc(1, device_HRDB_size);
        if (read(infd, buffer, device_HRDB_size) != device_HRDB_size) {
	        sprintf(message, "ps_copy_RFX271_device_data: error reading source %s HRDB\n", source_name);
	        display_and_log_message( message );
            goto devcleanup271;
        }
		// Copy the HRDB at the same offset as on source
        if (llseek(outfd, device_HRDB_offset * 1024, SEEK_SET) == -1)
        {
	        sprintf(message, "ps_copy_RFX271_device_data: error seeking to target %s HRDB\n", target_name);
	        display_and_log_message( message );
            goto devcleanup271;
        }
        if (write(outfd, buffer, device_HRDB_size) != device_HRDB_size) {
	        sprintf(message, "ps_copy_RFX271_device_data: error writing target %s HRDB\n", target_name);
	        display_and_log_message( message );
            goto devcleanup271;
        }
        free(buffer);
		buffer = NULL;

	    j++;
    } //... next device...

    outhdr.data.ver1.last_device = j - 1;
    if( llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
	    sprintf(message, "ps_copy_RFX271_device_data: error seeking to target %s header to update its last_device field\n", target_name);
	    display_and_log_message( message );
        i = 0;	 // indicate copy is not valid
        goto devcleanup271;
    }
    if( write(outfd, &outhdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t) )
	{
	    sprintf(message, "ps_copy_RFX271_device_data: error writing to target %s header to update its last_device field\n", target_name);
	    display_and_log_message( message );
        i = 0;	 // indicate copy is not valid
    }

devcleanup271:
	if( table != NULL ) free(table);
	if( table2 != NULL ) free(table2);
	if( buffer != NULL ) free(buffer);
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_dev)
    {
    	return PS_OK;
    }
    else
    {
	    return PS_ERR;
    }
}


// Copy device data from an RFX270 Pstore from or to a file; it is assumed 
// that the header is already set in the target file or device.
int ps_copy_RFX270_device_data(char *source_name, char *target_name)
{
    int             infd, outfd, i = 0, j;
    ps_hdr_RFX270_t inhdr, outhdr;
    unsigned int    table_size, offset;
    ps_dev_entry_RFX270_t *table = NULL;
    char 	        *buffer = NULL;
	int             result1, result2, final_result;

    if ((infd = open(source_name, O_RDWR | O_SYNC)) == -1)
    {
	    sprintf( message, "ps_copy_RFX270_device_data: error opening %s.\n", source_name );
	    display_and_log_message( message );
        return PS_BOGUS_PS_NAME;
    }

    if ((outfd = open(target_name, O_RDWR | O_SYNC)) == -1)
    {
	    sprintf( message, "ps_copy_RFX270_device_data: error opening %s.\n", target_name );
	    display_and_log_message( message );
	    close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    result1 = llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET);
    result2 = llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET);
	if( result1 == -1 || result2 == -1 )
	{
	    sprintf( message, "ps_copy_RFX270_device_data: error seeking source %s or target %s.\n", source_name, target_name );
	    display_and_log_message( message );
	    close(infd);
	    close(outfd);
	    return PS_SEEK_ERROR;
	}
    /* read the headers */
    result1 = ( read(infd, &inhdr, sizeof(ps_hdr_RFX270_t)) == sizeof(ps_hdr_RFX270_t) );
    result2 = ( read(outfd, &outhdr, sizeof(ps_hdr_RFX270_t)) == sizeof(ps_hdr_RFX270_t) );
	if( !(result1 && result2) )
	{
	    sprintf( message, "ps_copy_RFX270_device_data: error seeking header from source %s or target %s.\n", source_name, target_name );
	    display_and_log_message( message );
	    close(infd);
	    close(outfd);
	    return PS_READ_ERROR;
	}

    // Seek to the device table location
    if (llseek(infd, inhdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1)
    {
	    sprintf( message, "ps_copy_RFX270_device_data: error seeking source %s device table.\n", source_name );
	    display_and_log_message( message );
        final_result = PS_SEEK_ERROR;
        goto devcleanup;
    }
	table_size = sizeof(ps_dev_entry_RFX270_t) * inhdr.data.ver1.max_dev;
	table = (ps_dev_entry_RFX270_t *)ftdcalloc(1, table_size);
    if (read(infd, (caddr_t)table, table_size) != table_size)
    {
	    sprintf( message, "ps_copy_RFX270_device_data: error reading source %s device table.\n", source_name );
	    display_and_log_message( message );
        final_result = PS_READ_ERROR;
		goto devcleanup;
    }
    j = 0;
    for (i = 0; i < inhdr.data.ver1.max_dev; i++)
    {
		if( (strlen(table[i].path) == strspn(table[i].path, " ")) || (table[i].pathlen == 0) )
		{	// Do not copy empty entries
		    continue;
		}
		/*
		 * copy device table
		 */
		offset = (outhdr.data.ver1.dev_table_offset * 1024) + (j * sizeof(ps_dev_entry_RFX270_t));
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "ps_copy_RFX270_device_data: error seeking target %s device table entry for writing.\n", target_name );
	        display_and_log_message( message );
            final_result = PS_SEEK_ERROR;
            goto devcleanup;
        }
		if (write(outfd, &table[i], sizeof(ps_dev_entry_RFX270_t)) != sizeof(ps_dev_entry_RFX270_t))
		{
	        sprintf( message, "ps_copy_RFX270_device_data: error writing target %s device table entry.\n", target_name );
	        display_and_log_message( message );
            final_result = PS_WRITE_ERROR;
            goto devcleanup;
	    }
		/*
		 * copy device attributes
		 */
    	buffer = ftdcalloc(1, inhdr.data.ver1.dev_attr_size);
    	offset = (inhdr.data.ver1.dev_attr_offset * 1024) +	(i * inhdr.data.ver1.dev_attr_size);
    	if (llseek(infd, offset, SEEK_SET) == -1)
    	{
	        sprintf( message, "ps_copy_RFX270_device_data: error seeking source %s device attribute entry.\n", source_name );
	        display_and_log_message( message );
            final_result = PS_SEEK_ERROR;
       		goto devcleanup;
    	}
    	if (read(infd, buffer, inhdr.data.ver1.dev_attr_size) != inhdr.data.ver1.dev_attr_size)
    	{
	        sprintf( message, "ps_copy_RFX270_device_data: error reading source %s device attribute entry.\n", source_name );
	        display_and_log_message( message );
            final_result = PS_READ_ERROR;
       		goto devcleanup;
    	}
    	offset = (outhdr.data.ver1.dev_attr_offset * 1024) + (j * outhdr.data.ver1.dev_attr_size);
    	if (llseek(outfd, offset, SEEK_SET) == -1)
    	{
	        sprintf( message, "ps_copy_RFX270_device_data: error seeking target %s device attribute entry.\n", target_name );
	        display_and_log_message( message );
            final_result = PS_SEEK_ERROR;
       		goto devcleanup;
    	}
    	if (write(outfd, buffer, outhdr.data.ver1.dev_attr_size) != outhdr.data.ver1.dev_attr_size)
    	{
	        sprintf( message, "ps_copy_RFX270_device_data: error writing target %s device attribute entry.\n", target_name );
	        display_and_log_message( message );
            final_result = PS_WRITE_ERROR;
       		goto devcleanup;
    	}
	    free(buffer);
		buffer = NULL;
        /*
         * copy LRDB
         */
	    offset = (inhdr.data.ver1.lrdb_offset * 1024) + (i * inhdr.data.ver1.lrdb_size);
        if (llseek(infd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "ps_copy_RFX270_device_data: error seeking source %s LRDB.\n", source_name );
	        display_and_log_message( message );
            final_result = PS_SEEK_ERROR;
            goto devcleanup;
        }
        buffer = ftdcalloc(1, inhdr.data.ver1.lrdb_size);
        if (read(infd, buffer, inhdr.data.ver1.lrdb_size) != inhdr.data.ver1.lrdb_size) {
	        sprintf( message, "ps_copy_RFX270_device_data: error reading source %s LRDB.\n", source_name );
	        display_and_log_message( message );
            final_result = PS_READ_ERROR;
            goto devcleanup;
        }
        offset = (outhdr.data.ver1.lrdb_offset * 1024) + (j * outhdr.data.ver1.lrdb_size);
        if (llseek(outfd, offset, SEEK_SET) == -1) {
	        sprintf( message, "ps_copy_RFX270_device_data: error seeking target %s LRDB.\n", target_name );
	        display_and_log_message( message );
            final_result = PS_SEEK_ERROR;
            goto devcleanup;
        }
        if (write(outfd, buffer, outhdr.data.ver1.lrdb_size) != outhdr.data.ver1.lrdb_size) {
	        sprintf( message, "ps_copy_RFX270_device_data: error writing target %s LRDB.\n", target_name );
	        display_and_log_message( message );
            final_result = PS_WRITE_ERROR;
            goto devcleanup;
        }
        free(buffer);
		buffer = NULL;
        /*
         * copy HRDB
         */
        offset = (inhdr.data.ver1.hrdb_offset * 1024) +	(i * inhdr.data.ver1.hrdb_size);
        if (llseek(infd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "ps_copy_RFX270_device_data: error seeking source %s HRDB.\n", source_name );
	        display_and_log_message( message );
            final_result = PS_SEEK_ERROR;
            goto devcleanup;
        }
        buffer = ftdcalloc(1, inhdr.data.ver1.hrdb_size);
        if (read(infd, buffer, inhdr.data.ver1.hrdb_size) != inhdr.data.ver1.hrdb_size) {
	        sprintf( message, "ps_copy_RFX270_device_data: error reading source %s HRDB.\n", source_name );
	        display_and_log_message( message );
            final_result = PS_READ_ERROR;
            goto devcleanup;
        }
        offset = (outhdr.data.ver1.hrdb_offset * 1024) + (j * outhdr.data.ver1.hrdb_size);
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "ps_copy_RFX270_device_data: error seeking target %s HRDB.\n", target_name );
	        display_and_log_message( message );
            final_result = PS_SEEK_ERROR;
            goto devcleanup;
        }
        if (write(outfd, buffer, outhdr.data.ver1.hrdb_size) != outhdr.data.ver1.hrdb_size) {
	        sprintf( message, "ps_copy_RFX270_device_data: error writing target %s HRDB.\n", target_name );
	        display_and_log_message( message );
            final_result = PS_WRITE_ERROR;
            goto devcleanup;
        }
        free(buffer);
		buffer = NULL;

	    j++;
    } //... next device...

    outhdr.data.ver1.last_device = j - 1;
    if( llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
	    sprintf( message, "ps_copy_RFX270_device_data: error seeking target %s header to update last_device field.\n", target_name );
	    display_and_log_message( message );
        i = 0;	 // indicate copy is not valid
        final_result = PS_SEEK_ERROR;
        goto devcleanup;
    }
    if( write(outfd, &outhdr, sizeof(ps_hdr_RFX270_t)) != sizeof(ps_hdr_RFX270_t) )
	{
	    sprintf( message, "ps_copy_RFX270_device_data: error writing target %s header to update last_device field.\n", target_name );
	    display_and_log_message( message );
        final_result = PS_WRITE_ERROR;
        i = 0;	 // indicate copy is not valid
    }

devcleanup:
	if( table != NULL ) free(table);
	if( buffer != NULL ) free(buffer);
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_dev)
    {
    	return PS_OK;
    }
    else
    {
	    return final_result;
    }
}


int migrate_RFX270_group_data_to_RFX272(char *source_name, char *target_name)
{
    int              infd, outfd, i = 0, j;
    ps_hdr_RFX270_t  inhdr;
    ps_hdr_t         outhdr; // RFX272 format
    unsigned int     table_size, offset;
    ps_group_entry_RFX270_t *table270 = NULL;
    ps_group_entry_t table272;
    char	         *buffer = NULL;

    if ((infd = open(source_name, O_RDWR | O_SYNC)) == -1)
    {
        return PS_BOGUS_PS_NAME;
    }
    if ((outfd = open(target_name, O_RDWR | O_SYNC)) == -1)
    {
	    close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET);
    llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET);
    /* read the header */
    if( read(infd, &inhdr, sizeof(ps_hdr_RFX270_t)) != sizeof(ps_hdr_RFX270_t) )
	{
	    sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while reading source header.\n" );
	    display_and_log_message( message );
        goto grpcleanup;
	}
    if( read(outfd, &outhdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t) )
	{
	    sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while reading target header.\n" );
	    display_and_log_message( message );
        goto grpcleanup;
	}

    if (llseek(infd, inhdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1)
	{
	    sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while seeking to source group table offset.\n" );
	    display_and_log_message( message );
        goto grpcleanup;
	}

	table_size = sizeof(ps_group_entry_RFX270_t) * inhdr.data.ver1.max_group;
	table270 = (ps_group_entry_RFX270_t *)ftdcalloc(1, table_size);
	if (read(infd, (caddr_t)table270, table_size) != table_size)
	{
	    sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while reading source group table.\n" );
	    display_and_log_message( message );
        goto grpcleanup;
	}
    j = 0;
    for (i = 0; i < inhdr.data.ver1.max_group; i++)
    {
        if ((strlen(table270[i].path) == strspn(table270[i].path," ")) || table270[i].pathlen == 0)
        {
	        continue;  // Skipp empty entries
	    }
	    memset( table272.path, 0, MAXPATHLEN );
        strncpy( table272.path, table270[i].path, MAXPATHLEN );
		table272.pathlen = table270[i].pathlen;
		table272.state = table270[i].state;
		table272.hostid = table270[i].hostid;
		table272.shutdown = table270[i].shutdown;
		table272.checkpoint = table270[i].checkpoint;

        offset = (outhdr.data.ver1.group_table_offset * 1024) +	(j * sizeof(ps_group_entry_t));
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while seeking to target group table.\n" );
	        display_and_log_message( message );
        	goto grpcleanup;
        }
        if (write(outfd, &table272, sizeof(ps_group_entry_t)) != sizeof(ps_group_entry_t))
        {
	        sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while writing target group table.\n" );
	        display_and_log_message( message );
		    goto grpcleanup;
        }
	/*
	 * copy group attributes
	 */
        buffer = ftdcalloc(1, inhdr.data.ver1.group_attr_size);
        offset = (inhdr.data.ver1.group_attr_offset * 1024) + (i * inhdr.data.ver1.group_attr_size);
        if (llseek(infd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while seeking to source group attributes.\n" );
	        display_and_log_message( message );
        	goto grpcleanup;
        }
        if (read(infd, buffer, inhdr.data.ver1.group_attr_size) != inhdr.data.ver1.group_attr_size)
        {
	        sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while reading source group attributes.\n" );
	        display_and_log_message( message );
        	goto grpcleanup;
        }
        offset = (outhdr.data.ver1.group_attr_offset * 1024) + (j * outhdr.data.ver1.group_attr_size);
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
	        sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while seeking to target group attributes.\n" );
	        display_and_log_message( message );
	        goto grpcleanup;
        }
        if (write(outfd, buffer, outhdr.data.ver1.group_attr_size) != outhdr.data.ver1.group_attr_size)
        {
	        sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while writing target group attributes.\n" );
	        display_and_log_message( message );
	        goto grpcleanup;
        }
	    free(buffer);
		buffer = NULL;
        j++; //...next group...
    }

    outhdr.data.ver1.last_group = j - 1;
    if( llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
	    sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while seeking target header to update it.\n" );
	    display_and_log_message( message );
        i = 0;	 // indicate copy is not valid
        goto grpcleanup;
    }
    if( write(outfd, &outhdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t) )
	{
	    sprintf( message, "migrate_RFX270_group_data_to_RFX272: error while writing target header to update it.\n" );
	    display_and_log_message( message );
        i = 0;	 // indicate copy is not valid
    }

grpcleanup:
	if( table270 != NULL ) free( table270 );
	if( buffer != NULL ) free( buffer );
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_group)
    {
    	return PS_OK;
    }
    else
    {
        return PS_ERR;
    }
}


int ps_copy_RFX271_group_data(char *source_name, char *target_name)
{
    int              infd, outfd, i = 0, j;
    ps_hdr_t         inhdr, outhdr;
    unsigned int     table_size, offset;
    ps_group_entry_t *table = NULL;
    char	         *buffer = NULL;

    if ((infd = open(source_name, O_RDWR | O_SYNC)) == -1)
    {
		sprintf(message, "ps_copy_RFX271_group_data: error opening source %s; errno = %d\n", source_name, errno);
	    display_and_log_message( message );
        return PS_BOGUS_PS_NAME;
    }
    if ((outfd = open(target_name, O_RDWR | O_SYNC)) == -1)
    {
		sprintf(message, "ps_copy_RFX271_group_data: error opening target %s; errno = %d\n", target_name, errno);
	    display_and_log_message( message );
	    close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    if( llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
		sprintf(message, "ps_copy_RFX271_group_data: error seeking to header of source %s\n", source_name);
	    display_and_log_message( message );
        goto grpcleanup271;
	}
    if( llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
		sprintf(message, "ps_copy_RFX271_group_data: error seeking to header of target %s\n", target_name);
	    display_and_log_message( message );
        goto grpcleanup271;
	}
    /* read the header */
    if( read(infd, &inhdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t) )
	{
		sprintf(message, "ps_copy_RFX271_group_data: error reading header of source %s\n", source_name);
	    display_and_log_message( message );
        goto grpcleanup271;
	}
    if( read(outfd, &outhdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t) )
	{
		sprintf(message, "ps_copy_RFX271_group_data: error reading header of target %s\n", target_name);
	    display_and_log_message( message );
        goto grpcleanup271;
	}

    if (llseek(infd, inhdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1)
    {
		sprintf(message, "ps_copy_RFX271_group_data: error seeking to group table of source %s\n", source_name);
	    display_and_log_message( message );
        goto grpcleanup271;
    }

	table_size = sizeof(ps_group_entry_t) * inhdr.data.ver1.max_group;
	table = (ps_group_entry_t *)ftdcalloc(1, table_size);
	if (read(infd, (caddr_t)table, table_size) != table_size)
	{
		sprintf(message, "ps_copy_RFX271_group_data: error reading group table of source %s\n", source_name);
	    display_and_log_message( message );
		goto grpcleanup271;
	}
    j = 0;
    for (i = 0; i < inhdr.data.ver1.max_group; i++)
    {
        if ((strlen(table[i].path) == strspn(table[i].path," ")) || table[i].pathlen == 0)
        {
	        continue;
	    }
        offset = (outhdr.data.ver1.group_table_offset * 1024) +	(j * sizeof(ps_group_entry_t));
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
		    sprintf(message, "ps_copy_RFX271_group_data: error seeking to group table entry of target %s\n", target_name);
	        display_and_log_message( message );
        	goto grpcleanup271;
        }
        if (write(outfd, &table[i], sizeof(ps_group_entry_t)) != sizeof(ps_group_entry_t))
        {
		    sprintf(message, "ps_copy_RFX271_group_data: error writing group table entry of target %s\n", target_name);
	        display_and_log_message( message );
		    goto grpcleanup271;
        }
		 /*
		 * copy group attributes
		 */
        buffer = ftdcalloc(1, inhdr.data.ver1.group_attr_size);
        offset = (inhdr.data.ver1.group_attr_offset * 1024) + (i * inhdr.data.ver1.group_attr_size);
        if (llseek(infd, offset, SEEK_SET) == -1)
        {
		    sprintf(message, "ps_copy_RFX271_group_data: error seeking to group attribute entry of source %s\n", source_name);
	        display_and_log_message( message );
        	goto grpcleanup271;
        }
        if (read(infd, buffer, inhdr.data.ver1.group_attr_size) != inhdr.data.ver1.group_attr_size)
        {
		    sprintf(message, "ps_copy_RFX271_group_data: error reading group attribute entry of source %s\n", source_name);
	        display_and_log_message( message );
        	goto grpcleanup271;
        }
        offset = (outhdr.data.ver1.group_attr_offset * 1024) + (j * outhdr.data.ver1.group_attr_size);
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
		    sprintf(message, "ps_copy_RFX271_group_data: error seeking to group attribute entry of target %s\n", target_name);
	        display_and_log_message( message );
	        goto grpcleanup271;
        }
        if (write(outfd, buffer, outhdr.data.ver1.group_attr_size) != outhdr.data.ver1.group_attr_size)
        {
		    sprintf(message, "ps_copy_RFX271_group_data: error writing group attribute entry of target %s\n", target_name);
	        display_and_log_message( message );
	        goto grpcleanup271;
        }
	    free(buffer);
		buffer = NULL;
        j++; //...next group...
    }

    outhdr.data.ver1.last_group = j - 1;
    if( llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
		sprintf(message, "ps_copy_RFX271_group_data: error seeking to target header to update its last_group field; errno = %d\n", errno);
	    display_and_log_message( message );
        i = 0;	 // indicate copy is not valid
        goto grpcleanup271;
    }
    if( write(outfd, &outhdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t) )
	{
		sprintf(message, "ps_copy_RFX271_group_data: error writing target header to update its last_group field; errno = %d\n", errno);
	    display_and_log_message( message );
        i = 0;	 // indicate copy is not valid
    }

grpcleanup271:
	if( table != NULL ) free( table );
	if( buffer != NULL ) free( buffer );
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_group)
    {
    	return PS_OK;
    }
    else
    {
        return PS_ERR;
    }
}


int ps_copy_RFX270_group_data(char *source_name, char *target_name)
{
    int              infd, outfd, i = 0, j;
    ps_hdr_RFX270_t  inhdr, outhdr;
    unsigned int     table_size, offset;
    ps_group_entry_RFX270_t *table = NULL;
    char	         *buffer = NULL;

    if ((infd = open(source_name, O_RDWR | O_SYNC)) == -1)
    {
		sprintf(message, "ps_copy_RFX270_group_data: error opening source %s; errno = %d\n", source_name, errno);
	    display_and_log_message( message );
        return PS_BOGUS_PS_NAME;
    }
    if ((outfd = open(target_name, O_RDWR | O_SYNC)) == -1)
    {
		sprintf(message, "ps_copy_RFX270_group_data: error opening target %s; errno = %d\n", target_name, errno);
	    display_and_log_message( message );
	    close(infd);
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    if( llseek(infd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
		sprintf(message, "ps_copy_RFX270_group_data: error seeking source %s header.\n", source_name);
	    display_and_log_message( message );
        goto grpcleanup;
	}

    if( llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
		sprintf(message, "ps_copy_RFX270_group_data: error seeking target %s header.\n", target_name);
	    display_and_log_message( message );
        goto grpcleanup;
	}

    /* read the header */
    if( read(infd, &inhdr, sizeof(ps_hdr_RFX270_t)) != sizeof(ps_hdr_RFX270_t) )
	{
		sprintf(message, "ps_copy_RFX270_group_data: error reading source %s header.\n", source_name);
	    display_and_log_message( message );
        goto grpcleanup;
	}
    if( read(outfd, &outhdr, sizeof(ps_hdr_RFX270_t)) != sizeof(ps_hdr_RFX270_t) )
	{
		sprintf(message, "ps_copy_RFX270_group_data: error reading target %s header.\n", target_name);
	    display_and_log_message( message );
        goto grpcleanup;
	}

    if (llseek(infd, inhdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1)
    {
		sprintf(message, "ps_copy_RFX270_group_data: error seeking source %s group table.\n", source_name);
	    display_and_log_message( message );
        goto grpcleanup;
    }

	table_size = sizeof(ps_group_entry_RFX270_t) * inhdr.data.ver1.max_group;
	table = (ps_group_entry_RFX270_t *)ftdcalloc(1, table_size);
	if (read(infd, (caddr_t)table, table_size) != table_size)
	{
		sprintf(message, "ps_copy_RFX270_group_data: error reading source %s group table.\n", source_name);
	    display_and_log_message( message );
		goto grpcleanup;
	}
    j = 0;
    for (i = 0; i < inhdr.data.ver1.max_group; i++)
    {
        if ((strlen(table[i].path) == strspn(table[i].path," ")) || table[i].pathlen == 0)
        {
	        continue;
	    }
        offset = (outhdr.data.ver1.group_table_offset * 1024) +	(j * sizeof(ps_group_entry_RFX270_t));
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
		    sprintf(message, "ps_copy_RFX270_group_data: error seeking target %s group table entry.\n", target_name);
	        display_and_log_message( message );
        	goto grpcleanup;
        }
        if (write(outfd, &table[i], sizeof(ps_group_entry_RFX270_t)) != sizeof(ps_group_entry_RFX270_t))
        {
		    sprintf(message, "ps_copy_RFX270_group_data: error writing target %s group table entry.\n", target_name);
	        display_and_log_message( message );
		    goto grpcleanup;
        }
	/*
	 * copy group attributes
	 */
        buffer = ftdcalloc(1, inhdr.data.ver1.group_attr_size);
        offset = (inhdr.data.ver1.group_attr_offset * 1024) + (i * inhdr.data.ver1.group_attr_size);
        if (llseek(infd, offset, SEEK_SET) == -1)
        {
		    sprintf(message, "ps_copy_RFX270_group_data: error seeking source %s group attribute entry.\n", source_name);
	        display_and_log_message( message );
        	goto grpcleanup;
        }
        if (read(infd, buffer, inhdr.data.ver1.group_attr_size) != inhdr.data.ver1.group_attr_size)
        {
		    sprintf(message, "ps_copy_RFX270_group_data: error reading source %s group attribute entry.\n", source_name);
	        display_and_log_message( message );
        	goto grpcleanup;
        }
        offset = (outhdr.data.ver1.group_attr_offset * 1024) + (j * outhdr.data.ver1.group_attr_size);
        if (llseek(outfd, offset, SEEK_SET) == -1)
        {
		    sprintf(message, "ps_copy_RFX270_group_data: error seeking target %s group attribute entry.\n", target_name);
	        display_and_log_message( message );
	        goto grpcleanup;
        }
        if (write(outfd, buffer, outhdr.data.ver1.group_attr_size) != outhdr.data.ver1.group_attr_size)
        {
		    sprintf(message, "ps_copy_RFX270_group_data: error writing target %s group attribute entry.\n", target_name);
	        display_and_log_message( message );
	        goto grpcleanup;
        }
	    free(buffer);
		buffer = NULL;
        j++; //...next group...
    }

    outhdr.data.ver1.last_group = j - 1;
    if( llseek(outfd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
		sprintf(message, "ps_copy_RFX270_group_data: error seeking to target %s header to update its last_group field; errno = %d\n", target_name, errno);
	    display_and_log_message( message );
        i = 0;	 // indicate copy is not valid
        goto grpcleanup;
    }
    if( write(outfd, &outhdr, sizeof(ps_hdr_RFX270_t)) != sizeof(ps_hdr_RFX270_t) )
	{
		sprintf(message, "ps_copy_RFX270_group_data: error writing target %s header to update its last_group field; errno = %d\n", target_name, errno);
	    display_and_log_message( message );
        i = 0;	 // indicate copy is not valid
    }

grpcleanup:
	if( table != NULL ) free( table );
	if( buffer != NULL ) free( buffer );
    close(infd);
    close(outfd);
    if (i == inhdr.data.ver1.max_group)
    {
    	return PS_OK;
    }
    else
    {
        return PS_ERR;
    }
}


/*
 * this function initializes the pstore.
 * It writes the header information into the pstore for the RFX272 version
 */ 
int initialize_ps_RFX272(char *ps_name, int *max_dev, int hrdbsize, int hrdbtype, void *source_Pstore_header)
{
    unsigned long long  size, tmpu64;
    char                raw_name[MAXPATHLEN];
    daddr_t             dsize;
    int              	fd, i;
    ps_hdr_t         	hdr;
    struct stat      	statbuf;
    unsigned int     	table_size = 0, lrdbsize = 0, grpattrsize = 0, devattrsize = 0;
    ps_dev_entry_t   	*dtable;
    ps_group_entry_t 	*gtable = NULL;
    ps_dev_entry_2_t   	*dtable2;
	int                 hrdb_size_alignment;

#if defined(HPUX)
    if (is_logical_volume(ps_name)) {
        convert_lv_name(raw_name, ps_name, 1);
    } else {
        force_dsk_or_rdsk(raw_name, ps_name, 1);
    }
#else
    force_dsk_or_rdsk(raw_name, ps_name, 1);
#endif

    if ((dsize = disksize(raw_name)) < 0)
    {
	    sprintf(message, "Error determining the size of %s\n", raw_name);
	    display_and_log_message( message );
        return -1;
    }
	lrdbsize = FTD_PS_LRDB_SIZE;
	grpattrsize = FTD_PS_GROUP_ATTR_SIZE;
	devattrsize = FTD_PS_DEVICE_ATTR_SIZE;

    if (hrdbtype == FTD_HS_PROPORTIONAL)
	{
        dsize = FTD_MAX_DEVICES;
	}
	else
	{
	    size = lrdbsize + hrdbsize + devattrsize +
	          	grpattrsize + 3*MAXPATHLEN;
	    size = size / DEV_BSIZE;
#if defined(_AIX) || defined(linux)
        dsize = (daddr_t)((dsize - 32 -	(lrdbsize+hrdbsize)/DEV_BSIZE ) / size);
#else
        dsize = (daddr_t)((dsize - 32) / size);
#endif

	    if (dsize > FTD_MAX_DEVICES)
	    {
	        dsize = FTD_MAX_DEVICES;
	    }
    }

    *max_dev = dsize;

    /* stat the device and make sure it is a slice */
    if (stat(ps_name, &statbuf) != 0) {
        return PS_BOGUS_PS_NAME;
    }

    if (!(S_ISBLK(statbuf.st_mode))) {
        return PS_BOGUS_PS_NAME;
    }
    /* open the pstore */
    if ((fd = open(ps_name, O_RDWR)) == -1) {
        return PS_BOGUS_PS_NAME;
    }
	hdr.magic = PS_VERSION_1_MAGIC;
    hdr.data.ver1.max_dev = dsize;
    hdr.data.ver1.max_group = dsize;
    hdr.data.ver1.dev_attr_size = devattrsize;
    hdr.data.ver1.group_attr_size = grpattrsize;
    hdr.data.ver1.num_device = 0;
    hdr.data.ver1.num_group = 0;
    hdr.data.ver1.last_device = -1;
    hdr.data.ver1.last_group = -1;
    hdr.data.ver1.lrdb_size = lrdbsize;
    hdr.data.ver1.Small_or_Large_hrdb_size = hrdbsize;
	hdr.data.ver1.dev_table_entry_size = sizeof(ps_dev_entry_t);
	hdr.data.ver1.group_table_entry_size = sizeof(ps_group_entry_t);
	hdr.data.ver1.dev_HRDB_info_entry_size = sizeof(ps_dev_entry_2_t);

    hdr.data.ver1.dev_table_offset = PS_HEADER_OFFSET +
        ((sizeof(hdr) + 1023) / 1024);
    hdr.data.ver1.group_table_offset = hdr.data.ver1.dev_table_offset +
        (((hdr.data.ver1.max_dev * hdr.data.ver1.dev_table_entry_size) + 1023) / 1024);
    hdr.data.ver1.dev_attr_offset = hdr.data.ver1.group_table_offset +
        (((hdr.data.ver1.max_group * hdr.data.ver1.group_table_entry_size) + 1023) / 1024);
    hdr.data.ver1.group_attr_offset = hdr.data.ver1.dev_attr_offset +
        (((hdr.data.ver1.max_dev * hdr.data.ver1.dev_attr_size) + 1023) / 1024);

    // If the HRDB mode is Proportional to device sizes, we use the Legacy Small HRDB size as alignment factor
    if (hrdbtype == FTD_HS_PROPORTIONAL)
	{
	    hrdb_size_alignment =  FTD_PS_HRDB_SIZE_SMALL;
	}
	else
	{
	    hrdb_size_alignment = hrdbsize;
	}

#if defined(_AIX) || defined(linux)
    // The following calculates the offsets to the first LRDB, to the devices' HRDB info table
    // and to the first HRDB, aligned on a their respective size boundary.
	// Offset to LRDB area:
     tmpu64 = ((hdr.data.ver1.group_attr_offset * 1024 +
        	hdr.data.ver1.max_group * hdr.data.ver1.group_attr_size + lrdbsize-1)
        	/ lrdbsize) * (lrdbsize/1024);
     hdr.data.ver1.lrdb_offset = (unsigned int) tmpu64;

     // Offset to devices' HRDB info table (Proportional HRDB mode):
     hdr.data.ver1.dev_HRDB_info_table_offset = ((hdr.data.ver1.lrdb_offset * 1024 +
               hdr.data.ver1.max_dev * lrdbsize + 1023) / 1024);

     // Offset to HRDB area:
     tmpu64 = ((hdr.data.ver1.dev_HRDB_info_table_offset * 1024 +
               hdr.data.ver1.max_dev * hdr.data.ver1.dev_HRDB_info_entry_size + hrdb_size_alignment-1) 
		       / hrdb_size_alignment) * (hrdb_size_alignment/1024);
     hdr.data.ver1.hrdb_offset = (unsigned int) tmpu64;

#else
	// Offset to LRDB area:
    hdr.data.ver1.lrdb_offset = hdr.data.ver1.group_attr_offset +
        (((hdr.data.ver1.max_group * hdr.data.ver1.group_attr_size) + 1023) / 1024);

    // Offset to devices' HRDB info table (Proportional HRDB mode)
    hdr.data.ver1.dev_HRDB_info_table_offset = ((hdr.data.ver1.lrdb_offset * 1024 +
               hdr.data.ver1.max_dev * hdr.data.ver1.lrdb_size + 1023) / 1024);

    // Offset to HRDB area:
    hdr.data.ver1.hrdb_offset = hdr.data.ver1.dev_HRDB_info_table_offset +
        (((hdr.data.ver1.max_dev * hdr.data.ver1.dev_HRDB_info_entry_size + 1023) / 1024));

#endif  /* _AIX and linux */

	// last_block is actually unused. We keep it as is from legacy code,
	// implying from the above that in case of Proportional HRDB the calculation
	// will be the same as that for Small HRT.
    hdr.data.ver1.last_block = hdr.data.ver1.hrdb_offset +
        (((hdr.data.ver1.max_dev * hrdb_size_alignment) + 1023) / 1024) - 1;

    hdr.data.ver1.hrdb_type = hrdbtype;
    hdr.data.ver1.next_available_HRDB_offset = hdr.data.ver1.hrdb_offset;

    if( (hrdbtype == FTD_HS_PROPORTIONAL) && (source_Pstore_header != NULL) )
	{
        hdr.data.ver1.tracking_resolution_level = ((ps_hdr_t *)source_Pstore_header)->data.ver1.tracking_resolution_level;
        hdr.data.ver1.max_HRDB_size_KBs = ((ps_hdr_t *)source_Pstore_header)->data.ver1.max_HRDB_size_KBs;
	}
	else
	{
        hdr.data.ver1.tracking_resolution_level = PS_DEFAULT_TRACKING_RES;
        hdr.data.ver1.max_HRDB_size_KBs = hrdbsize;
	}

    if (llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &hdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t)) {
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* allocate a table for devices */
	table_size = hdr.data.ver1.max_dev * sizeof(ps_dev_entry_t);
	if ((dtable = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
	    close(fd);
	    return PS_MALLOC_ERROR;
	}
    /* set all values to the bogus values */
	for (i = 0; i < hdr.data.ver1.max_dev; i++)
	{
	    dtable[i].pathlen = 0;
	    memset(dtable[i].path, 0, MAXPATHLEN);
    	dtable[i].ps_allocated_lrdb_bits = 0xffffffff;
	    dtable[i].ps_allocated_hrdb_bits = 0xffffffff;
	    dtable[i].ps_valid_lrdb_bits = 0;  // These 0 values will be set at the first dtcstart after migration
	    dtable[i].ps_valid_hrdb_bits = 0;
	    dtable[i].ackoff = 0;
	}
    /* write out the device table array */
	if (llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1)
	{
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

    /* allocate a table for groups */
	table_size = hdr.data.ver1.max_group * sizeof(ps_group_entry_t);
	if ((gtable = (ps_group_entry_t *)malloc(table_size)) == NULL)
	{
	    close(fd);
	    return PS_MALLOC_ERROR;
	}

    /* set all values to the bogus values */
	for (i = 0; i < hdr.data.ver1.max_group; i++)
	{
	    gtable[i].pathlen = 0;
	    memset(gtable[i].path, 0, MAXPATHLEN);
	    gtable[i].hostid = 0xffffffff;
	    gtable[i].state = 0;
	    gtable[i].shutdown = 0;
	    gtable[i].checkpoint = 0;
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

    /* allocate a table for device HRDB info */
	table_size = hdr.data.ver1.max_dev * sizeof(ps_dev_entry_2_t);
	if ((dtable2 = (ps_dev_entry_2_t *)malloc(table_size)) == NULL) {
	    close(fd);
	    return PS_MALLOC_ERROR;
	}
    /* set all values to the bogus values */
	for (i = 0; i < hdr.data.ver1.max_dev; i++)
	{
	    dtable2[i].pathlen = 0;
	    memset(dtable2[i].path, 0, MAXPATHLEN);
    	dtable2[i].lrdb_res_shift_count = 0;
	    dtable2[i].lrdb_res_sectors_per_bit = 0;
	    dtable2[i].hrdb_resolution_KBs_per_bit = 0;
	    dtable2[i].hrdb_size = 0;
	    dtable2[i].dev_HRDB_offset_in_KBs = 0;
	    dtable2[i].has_reusable_HRDB = 0;
	    dtable2[i].num_sectors_64bits = 0;
	    dtable2[i].orig_num_sectors_64bits = 0;
	    dtable2[i].limitsize_multiple = 0;
	}
    /* write out the device HRDB info table array */
	if (llseek(fd, hdr.data.ver1.dev_HRDB_info_table_offset * 1024, SEEK_SET) == -1)
	{
	    close(fd);
	    free(dtable2);
	    return PS_SEEK_ERROR;
	}
	if (write(fd, dtable2, table_size) != table_size) {
	    close(fd);
	    free(dtable2);
	    return PS_WRITE_ERROR;
	}
	free(dtable2);
    close(fd);

    return PS_OK;
}

int migrate_pstore_RFX271_to_RFX272(char *source_name, char *target_name)
{
  ps_hdr_t        hdr271, hdr272;  // RFX271 and 272 have the same header format
  int             result;

    // Read the source 271 header
	result = get_header_common( source_name, (void *)(&hdr271), sizeof(ps_hdr_t) );
	if( result != PS_OK )
	{
		sprintf(message, "Error while reading header information from %s, error code = %d\n", source_name, result);
	    display_and_log_message( message );
		return(-1);
	}

    // Read the target 272 header
	result = get_header_common( target_name, (void *)(&hdr272), sizeof(ps_hdr_t) );
	if( result != PS_OK )
	{
		sprintf(message, "Error while reading header information from %s, error code = %d\n", target_name, result);
	    display_and_log_message( message );
		return(-1);
	}

    // Transfer the needed information in the target header (what was not previously set)
	hdr272.data.ver1.num_group = hdr271.data.ver1.num_group;
	hdr272.data.ver1.num_device = hdr271.data.ver1.num_device;
	hdr272.data.ver1.last_group = hdr271.data.ver1.last_group;
	hdr272.data.ver1.last_device = hdr271.data.ver1.last_device;
	hdr272.data.ver1.Small_or_Large_hrdb_size = hdr271.data.ver1.Small_or_Large_hrdb_size;
	hdr272.data.ver1.hrdb_type = hdr271.data.ver1.hrdb_type;
	hdr272.data.ver1.tracking_resolution_level = hdr271.data.ver1.tracking_resolution_level;
	hdr272.data.ver1.max_HRDB_size_KBs = hdr271.data.ver1.max_HRDB_size_KBs;

  // Save the target header
  if( (result = set_header_common( target_name, (void *)(&hdr272), sizeof(ps_hdr_t) )) != PS_OK )
  {
	    sprintf(message, "Error while writing header information to %s; status = %d\n", target_name, result);
	    display_and_log_message( message );
	    return(-1);
  }

  // Migrate device control info and HRDB info table plus LRDB and HRDB
  if (migrate_RFX271_device_data_to_RFX272(source_name, target_name) != PS_OK)
  {
	    sprintf(message, "Error while migrating device data to %s\n", target_name);
	    display_and_log_message( message );
	    return(-1);
  }
  // For group information, just copy it back as is to the Pstore
  if (ps_copy_RFX271_group_data(source_name, target_name) != PS_OK)
  {
       	sprintf(message, "Error while migrating group data to %s\n", target_name);
	    display_and_log_message( message );
       	return(-1);
  }

  return 0;
}


int migrate_pstore_RFX270_to_RFX272(char *source_name, char *target_name)
{
  ps_hdr_RFX270_t hdr270;
  ps_hdr_t        hdr272;
  int             result;

    // Read the source 270 header
	result = get_header_common( source_name, (void *)(&hdr270), sizeof(ps_hdr_RFX270_t) );
	if( result != PS_OK )
	{
		sprintf(message, "Error while reading header information from %s, error code = %d\n", source_name, result);
	    display_and_log_message( message );
		return(-1);
	}

    // Read the target 272 header
	result = get_header_common( target_name, (void *)(&hdr272), sizeof(ps_hdr_t) );
	if( result != PS_OK )
	{
		sprintf(message, "Error while reading header information from %s, error code = %d\n", target_name, result);
	    display_and_log_message( message );
		return(-1);
	}

    // Transfer the needed information in the target header (what was not previously set)
	hdr272.data.ver1.num_group = hdr270.data.ver1.num_group;
	hdr272.data.ver1.num_device = hdr270.data.ver1.num_device;
	hdr272.data.ver1.last_group = hdr270.data.ver1.last_group;
	hdr272.data.ver1.last_device = hdr270.data.ver1.last_device;
	hdr272.data.ver1.max_HRDB_size_KBs = hdr272.data.ver1.Small_or_Large_hrdb_size / 1024;
	hdr272.data.ver1.tracking_resolution_level = PS_DEFAULT_TRACKING_RES; // N/A but initialized to default

	hdr272.data.ver1.next_available_HRDB_offset = hdr272.data.ver1.hrdb_offset
	                 + (hdr272.data.ver1.num_device * hdr272.data.ver1.Small_or_Large_hrdb_size);
	hdr272.data.ver1.last_block = hdr272.data.ver1.hrdb_offset
	                 + (((hdr272.data.ver1.max_dev * hdr272.data.ver1.Small_or_Large_hrdb_size) + 1023) / 1024) - 1;


  // Save the target header
  if( (result = set_header_common( target_name, (void *)(&hdr272), sizeof(ps_hdr_t) )) != PS_OK)
  {
	    sprintf(message, "Error while writing header information to %s; status = %d\n", target_name, result);
	    display_and_log_message( message );
	    return(-1);
  }

  // Migrate group control info
  // WARNING: it is mandatory to migrate the groups data before the devices data
  if (migrate_RFX270_group_data_to_RFX272(source_name, target_name) != PS_OK)
  {
       	sprintf(message, "Error while migrating group data to %s\n", target_name);
	    display_and_log_message( message );
       	return(-1);
  }
  // Migrate device control info plus LRDB and HRDB
  if (migrate_RFX270_device_data_to_RFX272(source_name, target_name) != PS_OK)
  {
	    sprintf(message, "Error while migrating device data to %s\n", target_name);
	    display_and_log_message( message );
	    return(-1);
  }

  return 0;
}


int copy_pstore_RFX271(char *source_name, char *target_name)
{
  ps_hdr_t hdr;
  int      result;

	result = get_header_common( source_name, (void *)(&hdr), sizeof(ps_hdr_t) );
	if( result != PS_OK )
	{
		sprintf(message, "copy_pstore_RFX271: error while reading header information from %s, error code = %d\n", source_name, result);
	    display_and_log_message( message );
		return(-1);
	}

  if( (result = set_header_common(target_name, (void *)(&hdr), sizeof(ps_hdr_t))) != PS_OK )
  {
	    sprintf(message, "copy_pstore_RFX271: error while writing header information to %s; status = %d\n", target_name, result);
	    display_and_log_message( message );
	    return(-1);
  }

  // Copy device control info plus LRDB and HRDB
  if (ps_copy_RFX271_device_data(source_name, target_name) != PS_OK)
  {
	    sprintf(message, "copy_pstore_RFX271: error while copying device data to %s\n", target_name);
	    display_and_log_message( message );
	    return(-1);
  }
  if (ps_copy_RFX271_group_data(source_name, target_name) != PS_OK)
  {
       	sprintf(message, "copy_pstore_RFX271: error while copying group data to %s\n", target_name);
	    display_and_log_message( message );
       	return(-1);
  }

  return 0;
}


int copy_pstore_RFX270(char *source_name, char *target_name)
{
  ps_hdr_RFX270_t hdr;
  int             result;

	result = get_header_common( source_name, (void *)(&hdr), sizeof(ps_hdr_RFX270_t) );
	if( result != PS_OK )
	{
		sprintf(message, "Error while reading header information from %s, error code = %d\n", source_name, result);
	    display_and_log_message( message );
		return(-1);
	}

  if( (result = set_header_common(target_name, (void *)(&hdr), sizeof(ps_hdr_RFX270_t))) != PS_OK )
  {
	    sprintf(message, "Error while writing header information to %s; status = %d\n", target_name, result);
	    display_and_log_message( message );
	    return(-1);
  }

  // Copy device control info plus LRDB and HRDB
  if (ps_copy_RFX270_device_data(source_name, target_name) != PS_OK)
  {
	    sprintf(message, "Error while copying device data to %s\n", target_name);
	    display_and_log_message( message );
	    return(-1);
  }
  if (ps_copy_RFX270_group_data(source_name, target_name) != PS_OK)
  {
       	sprintf(message, "Error while copying group data to %s\n", target_name);
	    display_and_log_message( message );
       	return(-1);
  }

  return 0;
}

int get_header_version( char *ps_name, unsigned int *version_code )
{
    // Here we use the 270 format for reading; we just want the first field	of the header
    int      fd;
    ps_hdr_RFX270_t hdr;

    /* open the pstore */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    if( llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET) == -1 )
	{
        close(fd);
        return PS_SEEK_ERROR;
	}

    /* read the header */
    if( read(fd, &hdr, sizeof(ps_hdr_RFX270_t)) != sizeof(ps_hdr_RFX270_t) )
	{
        close(fd);
	    return PS_READ_ERROR;
	}
    close(fd);

    *version_code = hdr.magic;
    return( PS_OK );
}


//========================== MAIN =============================
int main(int argc, char *argv[])
{
  int group, resgrp, lgcnt, i, j, max_devices, num_started_grps, tmpver;
  int num_old_grps, num_old_devs, major, minor, pat1, pat2;
  char ps_name[MAXPATHLEN], psfilename[MAXPATHLEN], strgrp[5], repver[MAX_VER_LENGTH+1];
  char cmdline[MAXPATHLEN];
  struct statvfs st;
  char *started_ps_names = NULL;
  struct dirent *dent;
  DIR *curdir;
  ps_hdr_t header_RFX272;
  ps_hdr_t header_RFX271; // RFX271 has the same header format as RFX272
  ps_hdr_RFX270_t header_RFX270;
  unsigned long req_space;
  int ch, result;
  int req_version_length;
  int pstore_in_use;
  unsigned int version;
  int hrdbtype;
  int print_message, found_pstore;
  int migration_confirmed;

  putenv("LANG=C");

  /* Make sure we are root */
  if (geteuid()) {
      fprintf(stderr, "You must be root to run this process...aborted\n");
      exit(1);
  }

  // Initialize the log file with time stamp
  sprintf( logfile, "%s/psmigrate272.log", PATH_RUN_FILES );
  sprintf( cmdline, "date >> %s", logfile );
  system( cmdline );

  progname = argv[0];

  migration_confirmed = 0;
  opterr = 0;
  while ((ch = getopt(argc, argv, "yh")) != EOF) {
      switch(ch) {
        case 'h':
                usage();
				exit(0);
                break;
        case 'y':
		        // Do the pstore migration.
				migration_confirmed = 1;
                break;
        default:
                usage();
				exit(0);
				break;
      }
  }
  if (optind != argc) {
      fprintf(stderr, "Invalid arguments\n");
      usage();
	  exit(1);
  }

  // Check that -y argument has been provided (some people give no argument to get the usage paragraph)
  if( !migration_confirmed )
  {
        usage();
		exit(0);
  }

  if( migration_confirmed )
  {
	  fprintf(stderr, "NOTE: messages displayed on screen are also logged in %s.\n", logfile);
	  sleep(3);
#if !defined(linux)
	  fprintf(stderr, "\nWARNING: if some of the devices were extended (dtcexpand) in the older release, you must reset these\n");
	  fprintf(stderr, "         devices expansion limit to 1 before the dtcstart command that will follow the pstore migration.\n");
	  fprintf(stderr, "         The command to do so is:\n");
	  fprintf(stderr, "              dtclimitsize -g<group number> -d <device name as defined in the group cfg file> -s 1\n");
	  fprintf(stderr, "         Otherwise the groups to which these devices belong will need a Full Refresh after pstore migration.\n");
	  result = yesorno( "Please confirm that you want to proceed with pstore migration"	);
	  if( result != 1 )
	  {
	      exit( 0 );
	  }
#endif
  }

  // Check that the installed dtcstart is compatible with RFX 2.7.2 pstore migrations; dtcstart must complete the migration work
	sprintf(cmdline, "%s/dtcstart -p 1>/dev/null 2> /dev/null", PATH_BIN_FILES);
	if( system(cmdline) != 0)
	{
	      sprintf(message, "The installed version of dtcstart is not compatible with pstore migration to the format of release 2.7.2.\n");
	      display_and_log_message( message );
		  exit( 1 );
	}

/*
 * check if any group has been started 
 */
  num_started_grps = 0;
  if ((curdir = opendir(PATH_CONFIG)) == NULL)
  {
      sprintf(message, "Error opening the configuration directory %s\n", PATH_CONFIG);
	  display_and_log_message( message );
  }
  started_ps_names = (char *)ftdmalloc(MAXPATHLEN);
  while ((dent = readdir(curdir)) != NULL)
  {
      if (!strncmp(&dent->d_name[4], ".cur", 4))
      {
          strncpy(strgrp, &dent->d_name[1], 3);
	      strgrp[3] = '\0';
          group = ftd_strtol(strgrp);
		  if (group < 0)
			continue;
	      sprintf(message, "Logical group %d is started. Please stop the group before migrating its pstore.\n", group);
	      display_and_log_message( message );
	      GETPSNAME(group, ps_name);
		  if (num_started_grps == 0)
		  {
			strcpy(started_ps_names, ps_name);
		  }
		  else
		  {
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
      sprintf(message, "No CFG file found\n");
	  display_and_log_message( message );
      exit(1);
  }

  is_exit = 0;
  instsigaction();

/*
 * loop through the groups and migrate the pstore for each group. Since a pstore can
 * be shared by many groups, if the pstore is migrated for one group we do not
 * migrate that pstore again for another group.
 */
  for (i = 0; i < lgcnt; i++)
  {
	    if (is_exit)
		    exit(0);

	    group = cfgtonum(i);
        if (GETPSNAME(group, ps_name) != 0)
        {
            sprintf(message, "Could not get pstore name for group %d\n", group);
	        display_and_log_message( message );
            continue;
        }
		pstore_in_use = 0;
        for (j = 0; j < num_started_grps; j++)
        {
		     if (j == 0)
		     {
				if (!strcmp(started_ps_names, ps_name))
				{
		            pstore_in_use = 1;
		            break;
	            }
		     }
		     else
		     {
		     	if (!strcmp(started_ps_names+(j*MAXPATHLEN)+1, ps_name))
		     	{
		            pstore_in_use = 1;
	 	            break;
		     	}
		     }
        }
        if (pstore_in_use)
        {
	        sprintf(message, "Pstore %s, to which group %d is associated, is in use. Skipping this pstore...\n", ps_name, group);
	        display_and_log_message( message );
            continue;   // Gor for another group...
        }
         
        if (0 != statvfs(PS_TMP_FILE_PATH, &st))
        {
	        sprintf(message, "Could not calculate free space in /tmp\n");
	        display_and_log_message( message );
	        exit(1);
        }

		if( (result = get_header_version( ps_name, &version )) != PS_OK )
		{
	        sprintf(message, "Could not determine the current version of Pstore %s; error = %d. Skipping this Pstore.\n", ps_name, result);
	        display_and_log_message( message );
	        continue;
		}
	    if( version == 0 )
		{
	        sprintf(message, "Pstore %s has a non-initialized version code (value 0). Skipping this Pstore.\n", ps_name);
	        display_and_log_message( message );
	        continue;
		}
	    if( version < PS_VERSION_1_MAGIC_RFX26X_RFX270 )
		{
	        sprintf(message, "Pstore %s is of a release earlier than 2.6.x; this cannot be migrated with dtcpsmigrate272.\n", ps_name);
	        display_and_log_message( message );
	        sprintf(message, "It must be migrated with dtcpsmigrate in a first phase.\n");
	        display_and_log_message( message );
	        continue;
		}
	    else if( version == PS_VERSION_1_MAGIC )
		{
			print_message = 1;
			found_pstore = 0;
            // Check if we just migrated this pstore or if it was already a 2.7.2 pstore, to avoid redundant "skipping" messages
			for( j = 0; j < number_of_migrated_pstores; j++ )
			{
			    if( strcmp( ps_name, migrated_pstores[j] ) == 0 )
				{
			        print_message = 0;
					found_pstore = 1;
					break;
				}
			}
			if( print_message )
			{
	            sprintf(message, "Pstore %s is already of release 2.7.2 format. Skipping it...\n", ps_name);
	            display_and_log_message( message );
			}

			// If this pstore is not recorded as migrated or as of 2.7.2 format, record it
			if( !found_pstore )
			{
			   if( (migrated_pstores[number_of_migrated_pstores] = (char *)malloc( MAXPATHLEN )) != NULL )
			   {
	               memset(migrated_pstores[number_of_migrated_pstores], 0, MAXPATHLEN);
				   strncpy(migrated_pstores[number_of_migrated_pstores], ps_name, MAXPATHLEN);
				   ++number_of_migrated_pstores;  
			   }
			}

	        continue; // Go to next group
		}

		switch( version )
		{
		    //------------------------------- RFX26X - RFX270 Pstore migration ------------------------------
		    case PS_VERSION_1_MAGIC_RFX26X_RFX270:

				result = get_header_common( ps_name, (void *)(&header_RFX270), sizeof(header_RFX270) );
				if( result != PS_OK )
				{
			        sprintf(message, "Failed reading the header of Pstore %s; error = %d. Skipping this Pstore...\n", ps_name, result);
	                display_and_log_message( message );
			        continue;
				}
				// Calculate the amount of space required in /tmp to migrate this pstore.
				req_space = header_RFX270.data.ver1.hrdb_offset * 1024 + header_RFX270.data.ver1.num_device * header_RFX270.data.ver1.hrdb_size;
				if (req_space > (st.f_bsize * st.f_bfree))
				{
				    sprintf(message, "More than %lu bytes of free space is required in /tmp to migrate the pstore %s for group %d.\n", req_space, ps_name, group);
	                display_and_log_message( message );
				    sprintf(message, "Please free the required amount of space in /tmp before continuing pstore migration for this group.\n");
	                display_and_log_message( message );
				    continue;
				}
				sprintf(message, "Migrating pstore %s to RFX/TUIP 2.7.2 format...\n", ps_name);
	            display_and_log_message( message );
			    if( ps_max_devices_RFX272_Small_or_Large_HRDB_mode(ps_name, &max_devices, header_RFX270.data.ver1.hrdb_size) != 0 )
				{
			        sprintf(message, "Could not determine the maximum number of devices for pstore %s.\n", ps_name);
	                display_and_log_message( message );
			        sprintf(message, "Hence this pstore will not be migrated.\n");
	                display_and_log_message( message );
			        continue;
				}
			    num_old_devs = header_RFX270.data.ver1.num_device;
			    num_old_grps = header_RFX270.data.ver1.num_group;

			    if (num_old_devs > max_devices || num_old_grps > max_devices)
			    {
			        sprintf(message, "The pstore %s will not be able to support all the devices/groups configured on it after it is converted to the RFX272 version.\n", ps_name);
	                display_and_log_message( message );
			        sprintf(message, "Hence this pstore will not be migrated.\n");
	                display_and_log_message( message );
			        continue;
			    }
			    sprintf(psfilename, "%spstoredata%03d", PS_TMP_FILE_PATH, group);
                /*
                 * copy pstore contents to a file
                 */
	            if (copy_pstore_RFX270(ps_name, psfilename) != 0)
	            {
			        sprintf(message, "An error occured while copying this pstore contents to a file. The pstore currently holds the old version contents.\n");
	                display_and_log_message( message );
			        unlink(psfilename);
			        exit(-1);
			    }
                /*
                 * initialize the pstore to the RFX272 version
                 */
				if( header_RFX270.data.ver1.hrdb_size == FTD_PS_HRDB_SIZE_SMALL )
				{
				    hrdbtype = FTD_HS_SMALL;
				}
				else
				{
				    hrdbtype = FTD_HS_LARGE;
				}
				if( (result = initialize_ps_RFX272(ps_name, &max_devices, header_RFX270.data.ver1.hrdb_size, hrdbtype, (void *)(&header_RFX270))) != 0 )
				{
			        sprintf(message, "An error occured (status %d) while initializing pstore %s for its migration. Restoring to original contents.\n", result, ps_name);
	                display_and_log_message( message );
                    // If an error occurred while formatting the pstore for RFX272, restore the Pstore contents as they were and exit
	                if( copy_pstore_RFX270(psfilename, ps_name) != 0 )
					{
			            sprintf(message, "An error occured also while attempting to restore it back to its original contents.\n");
	                    display_and_log_message( message );
			            sprintf(message, "The pstore will need to be reinitialized with dtcinit -p and a Full Refresh will be required for its associated groups.\n");
	                    display_and_log_message( message );
					}
					else
					{
			            sprintf(message, "The pstore currently holds the old version contents.\n");
	                    display_and_log_message( message );
					}
                    unlink(psfilename);
                    exit(-1);
                }
                /*
                 * copy the file contents back to the pstore
                 */

	            if( (result = migrate_pstore_RFX270_to_RFX272(psfilename, ps_name)) != 0 )
				{
			        sprintf(message, "An error occured (status %d) while migrating pstore %s. Restoring to original contents.\n", result, ps_name);
	                display_and_log_message( message );
                    // If an error occurred while migrating the pstore to RFX272, restore the Pstore contents as they were and exit
	                if( copy_pstore_RFX270(psfilename, ps_name) != 0 )
					{
			            sprintf(message, "An error occured also while attempting to restore it back to its original contents.\n");
	                    display_and_log_message( message );
			            sprintf(message, "The pstore will need to be reinitialized with dtcinit -p and a Full Refresh will be required for its associated groups.\n");
	                    display_and_log_message( message );
					}
					else
					{
			            sprintf(message, "The pstore currently holds the old version contents.\n");
	                    display_and_log_message( message );
					}
					unlink(psfilename);
  			        exit(-1);
			   }
			   sprintf(message, "Migration phase 1 completed for pstore %s. dtcstart will perform the final phase of the migration.\n", ps_name);
	           display_and_log_message( message );
			   unlink(psfilename);
			   // Take note of the migrated pstore to avoid "skipping" messages when processing other groups
			   if( (migrated_pstores[number_of_migrated_pstores] = (char *)malloc( MAXPATHLEN )) != NULL )
			   {
	               memset(migrated_pstores[number_of_migrated_pstores], 0, MAXPATHLEN);
				   strncpy(migrated_pstores[number_of_migrated_pstores], ps_name, MAXPATHLEN);
				   ++number_of_migrated_pstores;  
			   }
			   break;

		    //------------------------------- RFX271 Pstore migration ------------------------------
		    case PS_VERSION_1_MAGIC_RFX271:

				result = get_header_common( ps_name, (void *)(&header_RFX271), sizeof(header_RFX271) );
				if( result != PS_OK )
				{
			        sprintf(message, "Failed reading the header of Pstore %s; error = %d. Skipping this Pstore...\n", ps_name, result);
	                display_and_log_message( message );
			        continue;
				}
				// Calculate the amount of space required in /tmp to migrate this pstore.
				req_space = header_RFX271.data.ver1.next_available_HRDB_offset * 1024;
				if (req_space > (st.f_bsize * st.f_bfree))
				{
				    sprintf(message, "More than %lu bytes of free space is required in /tmp to migrate the pstore %s for group %d.\n", req_space, ps_name, group);
	                display_and_log_message( message );
				    sprintf(message, "Please free the required amount of space in /tmp before continuing pstore migration for this group.\n");
	                display_and_log_message( message );
				    continue;
				}
				sprintf(message, "Migrating pstore %s to RFX/TUIP 2.7.2 format...\n", ps_name);
	            display_and_log_message( message );

				// RFX272 just has 2 integers more than RFX271 in its ps_dev_entry_2_RFX271_t structure and we know that we allocate table space
				// for 1024 (max) devices, so RFX272 needs 8K more space than RFX271, unless this difference pushes the HRDB area by a factor of 128 KB,
				// for alignment purposes; check Pstore space here.
				if( check_pstore_space_for_migrating_RFX271_to_RFX272(ps_name, &header_RFX271) != 0 )
				{
			        sprintf(message, "The pstore %s will not be able to support all the devices/groups configured on it after it is converted to the RFX272 version.\n", ps_name);
	                display_and_log_message( message );
			        sprintf(message, "Hence this pstore will not be migrated.\n");
	                display_and_log_message( message );
			        continue;
				}

			    sprintf(psfilename, "%spstoredata%03d", PS_TMP_FILE_PATH, group);
                
                // Copy pstore contents to a file
	            if (copy_pstore_RFX271(ps_name, psfilename) != 0)
	            {
			        sprintf(message, "An error occured while migrating this pstore. The pstore currently holds the old version contents.\n");
	                display_and_log_message( message );
			        unlink(psfilename);
			        exit(-1);
			    }
                
                // initialize the pstore to the RFX272 version
				if( (result = initialize_ps_RFX272(ps_name, &max_devices, header_RFX271.data.ver1.Small_or_Large_hrdb_size,
				                         header_RFX271.data.ver1.hrdb_type, (void *)(&header_RFX271))) != 0 )
				{
			        sprintf(message, "An error occured (status %d) while initializing pstore %s for its migration. Restoring to original contents.\n", result, ps_name);
	                display_and_log_message( message );
                    // If an error occurred while formatting the pstore for RFX272, restore the Pstore contents as they were and exit
	                if( copy_pstore_RFX271(psfilename, ps_name) != 0 )
					{
			            sprintf(message, "An error also occured while attempting to restore it back to its original contents.\n");
	                    display_and_log_message( message );
			            sprintf(message, "The pstore will need to be reinitialized with dtcinit -p and a Full Refresh will be required for its associated groups.\n");
	                    display_and_log_message( message );
					}
					else
					{
			            sprintf(message, "The pstore currently holds the old version contents.\n");
	                    display_and_log_message( message );
					}
                    unlink(psfilename);
                    exit(-1);
                }

                // Migrate the file contents back to the pstore
	            if( (result = migrate_pstore_RFX271_to_RFX272(psfilename, ps_name)) != 0 )
				{
			        sprintf(message, "An error occured (status %d) while migrating pstore %s. Restoring to original contents.\n", result, ps_name);
	                display_and_log_message( message );
                    // If an error occurred while migrating the pstore to RFX272, restore the Pstore contents as they were and exit
	                if( copy_pstore_RFX271(psfilename, ps_name) != 0 )
					{
			            sprintf(message, "An error also occured while attempting to restore it back to its original contents.\n");
	                    display_and_log_message( message );
			            sprintf(message, "The pstore will need to be reinitialized with dtcinit -p and a Full Refresh will be required for its associated groups.\n");
	                    display_and_log_message( message );
					}
					else
					{
					    sprintf(message, "The pstore currently holds the old version contents.\n");
	                    display_and_log_message( message );
					}
					unlink(psfilename);
  			        exit(-1);
			   }
			   sprintf(message, "Migration phase 1 completed for pstore %s. dtcstart will perform the final phase of the migration.\n", ps_name);
	           display_and_log_message( message );
			   unlink(psfilename);
			   // Take note of the migrated pstore to avoid "skipping" messages when processing other groups
			   if( (migrated_pstores[number_of_migrated_pstores] = (char *)malloc( MAXPATHLEN )) != NULL )
			   {
	               memset(migrated_pstores[number_of_migrated_pstores], 0, MAXPATHLEN);
				   strncpy(migrated_pstores[number_of_migrated_pstores], ps_name, MAXPATHLEN);
				   ++number_of_migrated_pstores;  
			   }
			   break;
	   } //...switch
  } //...for

  return( 0 );
}

#if defined(HPUX) && (SYSVERS >= 1100)
  shl_load () {}
  shl_unload () {}
  shl_findsym () {}
#endif

