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
/***************************************************************************
 * stat.c - FullTime Data PMD/RMD statistics dumper 
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the functionality for statistics calculations 
 *
 ***************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#if !defined(_AIX)
#if defined(HPUX) 
#if (SYSVERS >= 1100)
#include <math.h>
#else /* (SYSVERS >= 1100) */
#include <nan.h>
#endif /* (SYSVERS >= 1100) */
#elif defined (linux)
#include <math.h>
#include <time.h>
#else
#include <nan.h>
#endif /* defined(HPUX) */
#endif /* !defined(_AIX) */

#if defined(HPUX)
#include <sys/pstat.h>
#elif defined (SOLARIS)
#include <sys/procfs.h>
#endif
 
#include "errors.h"
#include "config.h"
#include "pathnames.h"
#include "process.h"
#include "ftdio.h"
#include "stat_intr.h"
#include "common.h"
#include "aixcmn.h"
#include "md5.h"

#ifdef TDMF_TRACE
FILE *dbgfd;
#endif

static struct stat statbuf[1];
static char perfname[FILE_PATH_LEN];
static char csv_perfname[FILE_PATH_LEN];
static char product_usage_tracking_file[FILE_PATH_LEN];
static time_t statts;
static time_t csv_statts;
unsigned char checksum_secret_key[] = "yekegasutcudorp";
extern char *argv0;

extern sddisk_t *get_lg_dev(group_t*, int);

devstat_l g_statl;

/****************************************************************************
 * dumpstats -- writes the current statistics to the statdump file
 ***************************************************************************/
void
dumpstats (machine_t *sys, int network_bandwidth_analysis_mode)
{
    devstat_l *statl;

    if (sys->tunables.statinterval < 1) {
        return;
    }
    (void)dump_pmd_stats(sys, network_bandwidth_analysis_mode);
    return; 
} /* dumpstats */

/****************************************************************************
 * flushstats -- called by PMD, RMD to save stats in driver
 ***************************************************************************/
void
flushstats(int force)
{
    time_t currentts;

    time(&currentts);

    if (force 
    || (mysys->tunables.statinterval > 0 && 
        currentts >= (statts + mysys->tunables.statinterval))) {
        (void)dumpstats(mysys,0);
    }
} /* flushstats */

/****************************************************************************
 * init_csv_stats -- opens and formats the csv stats dump file header
 * for Network analysis mode on the PMD side
 ***************************************************************************/
void
init_csv_stats( machine_t *mysys, int net_analysis_duration, char *fictitious_dtc_dev_name )
{
    char fil[FILE_PATH_LEN];
    char fil2[FILE_PATH_LEN];
    char *t;
    char *cfg_prefix;
    int l;
    mode_t save_mode, new_mode=0177; /* WR16793 */
#if defined(SOLARIS)
    char buf[256];
#endif

    if (mysys->tunables.statinterval < 1) {
        return;
    }
    if (!(ISPRIMARY(mysys))) {
        return;
    }
    if (0 != stat (PATH_VAR_OPT, statbuf)) {
        (void) mkdir (PATH_VAR_OPT, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    if (0 != stat (PATH_RUN_FILES, statbuf)) {
        (void) mkdir (PATH_RUN_FILES, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    l = strlen(mysys->configpath);
    t = mysys->configpath + (l - 7);
    cfg_prefix = "p";

    sprintf(csv_perfname, "%s/%s%3.3s.csv", PATH_RUN_FILES, cfg_prefix, t);
    sprintf(fil, "%s", csv_perfname);
    sprintf(fil2, "%s.1", csv_perfname);
    (void) unlink (fil2);
    (void) rename (fil, fil2);

    save_mode = umask(new_mode);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    mysys->csv_statfd = fopen(fil, "w");
#elif defined(SOLARIS)
    mysys->csv_statfd = open(fil, O_WRONLY|O_CREAT|O_TRUNC, 0600);
#endif
    umask(save_mode);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (mysys->csv_statfd == NULL) {
#elif defined(SOLARIS)
    if (mysys->csv_statfd == -1) {
#endif
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, fil, strerror(errno));
        }
    }
    (void)time(&csv_statts);

#if defined(HPUX) ||  defined(_AIX) || defined(linux)   
    // Write csv header: Type, Version, Server, Interval, Duration
    fprintf( mysys->csv_statfd, "1,1,NETWORK_BANDWIDTH from %s,%d,%d\n",
             mysys->name, mysys->tunables.statinterval, net_analysis_duration );
	// Write the "fictitious" volume name for group identification
    fprintf( mysys->csv_statfd, "2,%s,0\n",	fictitious_dtc_dev_name );
#elif defined(SOLARIS)                                                           
    // Write csv header: Type, Version, Server, Interval, Duration
	sprintf( buf, "1,1,NETWORK_BANDWIDTH from %s,%d,%d\n",	mysys->name, mysys->tunables.statinterval, net_analysis_duration );
	write(mysys->csv_statfd, buf, strlen(buf));
	fsync(mysys->csv_statfd);
	// Write the "fictitious" volume name for group identification
	sprintf( buf, "2,%s,0\n", fictitious_dtc_dev_name );
	write(mysys->csv_statfd, buf, strlen(buf));
	fsync(mysys->csv_statfd);
#endif

    return;
} /* init_csv_stats */


/****************************************************************************
 * calculate_product_usage_checksum -- compute checksum on given product usage
 * tracking file
 * Return: the length of the checksum string or -1 if error
 ***************************************************************************/
int calculate_product_usage_file_checksum( char *product_usage_tracking_file, unsigned char *checksum_digests_ptr )
{
    MD5_CTX context;
    unsigned char *dataptr, *digestptr;
    unsigned int length, left, bytes;
    int res;
	int checksum_secret_key_length = strlen( checksum_secret_key );
	int fd;
	unsigned char *buffer;
	struct stat statbuf;
	int digest_total_length = 0;
	int number_of_digests_used = 0;

	if( stat( product_usage_tracking_file, &statbuf ) != 0 )
	{
        reporterr (ERRFAC, M_FILE, ERRWARN, product_usage_tracking_file, strerror(errno));
		return( -1 );
	}

	if( (fd = open(product_usage_tracking_file, O_RDONLY)) == -1 )
	{
        reporterr (ERRFAC, M_FILE, ERRWARN, product_usage_tracking_file, strerror(errno));
		return( -1 );
	}

	if( (buffer = (char*)malloc(statbuf.st_size + checksum_secret_key_length + 4)) == NULL)
	{
        reporterr (ERRFAC, M_MALLOC, ERRWARN, statbuf.st_size + checksum_secret_key_length + 4);
		close(fd);
		return( -1 );
	}

    // Copy our secret key at the beginning of the buffer
    strcpy( buffer, checksum_secret_key );


    /* Read the product usage tracking file in the buffer, following the secret key */
	if( read( fd, buffer + checksum_secret_key_length, statbuf.st_size) != statbuf.st_size )
	{
        reporterr (ERRFAC, M_READERR, ERRWARN, product_usage_tracking_file, 0,
                   statbuf.st_size, strerror(errno));
		close(fd);
		free(buffer);
		return -1;
	}

    digestptr = checksum_digests_ptr;
    dataptr = buffer;
    length = statbuf.st_size + checksum_secret_key_length;
	digest_total_length = 0;
	number_of_digests_used = 0;

    res = MINCHK;

    for (left = length; left > 0; left -= res) {
	    if( number_of_digests_used == MAX_PROD_USAGE_CHECKSUM_DIGESTS )
		{
		    break;   // Avoid digest buffer overflow
		}
        if (left < res) {
            bytes = res = left;
        } else {
            bytes = res;
        }
        MD5Init(&context);
        MD5Update(&context, (unsigned char*)dataptr, bytes);
        MD5Final((unsigned char*)digestptr, &context);
        digestptr += DIGESTSIZE;
        dataptr += bytes;
		digest_total_length += DIGESTSIZE;
		++number_of_digests_used; // Number of digests used so far
    }
	if( left > 0 )
	{  // Digest buffer overflow would occur; return an error
        reporterr (ERRFAC, M_CHKOFLOW, ERRWARN, product_usage_tracking_file);
		digest_total_length = -1;
	}

	close(fd);
	free(buffer);
    return( digest_total_length );
}


/****************************************************************************
 verify_product_usage_file_checksum
 Calculate the checksum on a product usage tracking file and return 1 if not valid, 0 if valid,
 -1 if error occurred
*/
int verify_product_usage_file_checksum( char *product_usage_tracking_file, char *product_usage_checksum_file )
{
    unsigned char checksum_digests[DIGESTSIZE * MAX_PROD_USAGE_CHECKSUM_DIGESTS];
	int checksum_string_length;
	int fd;
	unsigned char buffer[DIGESTSIZE * MAX_PROD_USAGE_CHECKSUM_DIGESTS];
	struct stat statbuf;
	int j, compare_result;

    // Get the checksum digests on the product usage tracking file
    checksum_string_length = calculate_product_usage_file_checksum( product_usage_tracking_file, checksum_digests );
	if( checksum_string_length < 0 )
	    return( -1 );

    // Compare the calculated file checksum with its related value previously saved in the checksum file
	if( stat( product_usage_checksum_file, &statbuf ) != 0 )
	{
        reporterr (ERRFAC, M_FILE, ERRWARN, product_usage_checksum_file, strerror(errno));
		return( -1 );
	}

	if( (fd = open(product_usage_checksum_file, O_RDONLY)) == -1 )
	{
        reporterr (ERRFAC, M_FILE, ERRWARN, product_usage_checksum_file, strerror(errno));
		return( -1 );
	}

    /* Read the product usage checksum file in the buffer */
	if( read( fd, buffer, statbuf.st_size) != statbuf.st_size )
	{
        reporterr (ERRFAC, M_READERR, ERRWARN, product_usage_checksum_file, 0, statbuf.st_size, strerror(errno));
		close(fd);
		return( -1 );
	}

	close(fd);

    // Validate the checksum
	compare_result = 0;
	for( j = 0; j <	statbuf.st_size; j++ )
	{
	    if(	checksum_digests[j] != buffer[j] )
		{
	        compare_result = 1;	   // Failure
			break;
		}
	}

	return( compare_result );
}


/****************************************************************************
 * set_product_usage_checksum_failed
 * Set all the checksum-validated fields to 0 in the specified
 * product usage tracking file
 Return: 0 = success; -1 = error occurred
*/
int set_product_usage_checksum_failed( char *product_usage_tracking_file )
{
	int fd;
	unsigned char *buffer;
	struct stat statbuf;
	int j;
	int file_size;

	if( stat( product_usage_tracking_file, &statbuf ) != 0 )
	{
        reporterr (ERRFAC, M_FILE, ERRWARN, product_usage_tracking_file, strerror(errno));
		return( -1 );
	}

	if( (fd = open(product_usage_tracking_file, O_RDWR)) == -1 )
	{
        reporterr (ERRFAC, M_FILE, ERRWARN, product_usage_tracking_file, strerror(errno));
		return( -1 );
	}

	if( (buffer = (char*)malloc(statbuf.st_size + 4)) == NULL)
	{
        reporterr (ERRFAC, M_MALLOC, ERRWARN, statbuf.st_size + 4);
		close(fd);
		return( -1 );
	}

    /* Read the product usage tracking file in the buffer */
	if( read( fd, buffer, statbuf.st_size) != statbuf.st_size )
	{
        reporterr (ERRFAC, M_READERR, ERRWARN, product_usage_tracking_file, 0,
                   statbuf.st_size, strerror(errno));
		close(fd);
		free(buffer);
		return -1;
	}

    // Go through all the lines and set the checksum-validated fields to 0 (start with j = 1)
	// NOTE: the last character of each line can only be 1 or 0; if not, the line has been altered
	//       at its end; in this case we do not touch it; the report must detect that this line is
	//       altered by not finding a 1 at the end.
	for( j = 1; j <	statbuf.st_size; j++ )
	{
	    if( buffer[j] == '\n' )
		{
		    if( buffer[j-1]	== '1' )
		    {
		        buffer[j-1]	= '0';
			}
		}
	}

	// Rewind and write back the file from the beginning
    file_size = (int)statbuf.st_size;
    close( fd );
    fd = open(product_usage_tracking_file, O_WRONLY|O_TRUNC, 0600);

	if( write( fd, buffer, file_size) != file_size )
	{
        reporterr (ERRFAC, M_WRITERR, ERRWARN, product_usage_tracking_file, 0,
                   file_size, strerror(errno));
		close(fd);
		free(buffer);
		return -1;
	}

	close(fd);
	return( 0 );
}

/****************************************************************************
 * update_product_usage_checksum_file
 * Write [new or updated] checksum to product tracking checksum file of a group
 * Return: 0 = success; -1 = error
*/
int update_product_usage_checksum_file( char *product_usage_tracking_file, char *product_usage_checksum_file )
{
    unsigned char checksum_digests[DIGESTSIZE * MAX_PROD_USAGE_CHECKSUM_DIGESTS];
	int checksum_string_length;
    int  fd = -1;

    // Calculate new checksum on tracking file
    checksum_string_length = calculate_product_usage_file_checksum( product_usage_tracking_file, checksum_digests );
	if( checksum_string_length < 0 )
	    return( -1 );

    // Write new checksum to checksum file
    fd = open (product_usage_checksum_file, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (fd == -1) {
        reporterr (ERRFAC, M_USAGE_FILE, ERRWARN, product_usage_checksum_file, strerror(errno));
		return( -1 );
    }

	if( write(fd, checksum_digests, checksum_string_length) != checksum_string_length )
	{
        reporterr (ERRFAC, M_WRITERR, ERRWARN, product_usage_checksum_file, 0,
                   checksum_string_length, strerror(errno));
		close(fd);
		return -1;
	}
    (void) close( fd );

	return( 0 );
} 



/****************************************************************************
 * update_product_usage_tracking_file
 
 This is a csv file with which we track how much data is migrated by
 customers. It is created by the PMD at the start of a Full Refresh and updated
 for each device pair in the group and for Full Refresh completion timestamp.
 If the file already exists at the start of a Full Refresh, the new data is
 appended to it. The file contents are not overwritten.

 From the design document:
 The amount of data migrated is based on the completion of a successful Full Refresh.
 So this is based on storage capacity (size) configured for migration and successfully
 transferred to the target storage.
 For instance, if the customer is moving 10TB and due to application activity,
 pieces are moved again during refresh, it would still only count as 10TB.
 On the other hand, if the customer moves 10TB and later, after successful completion
 of the initial Full Refresh, moves another 5 TB, that would be 15TB total.
 This will be true even if some of that 5TB was a repeat migration of some of the
 original 10TB.

 The csv statistics file produced by a run of a PMD-RMD pair in Full Refresh mode is
 named pxxx.migration_tracking.csv (xxx is the PMD-RMD group number) and is located
 in the same directory where we find the usual dtcerror.log file.
 For ease of parsing, all the lines in the file will have the same format.
 When some of the fields are not pertinent on some lines, they will be left empty («,,»).

 File format:
 The following describes the format of all the lines in the file, i.e. comma-separated fields:
 - Type: identifies the CSV entry type. The value will always be 1 because all the lines have
       the same format.
 - Version: is the CSV format version. It is represented using a single digit. Value: 1.
 - Group number: the logical group number of the PMD-RMD pair.
 - Source Server: the host name or IP address of the SOURCE machine where the statistics are collected.
 - Target Server: the host name or IP address of the TARGET machine where the data is migrated.
 - Source device name. 
 - Source device size: the size of the device, expressed in Kbs.
 - Target device name. Note: we don't need the target device size.
 - Full Refresh Start Timestamp: the date and time at which the Full Refresh started.
      The POSIX time format is used.
 - Full Refresh Completion Timestamp: the date and time at which the Full Refresh successfully completed.
      The POSIX time format is used. This field in empty until completion of the Full Refresh of this group.
 - Checksum verification passed: 0 means failure, implying that the information on this line may not be
      reliable; 1 means success.

 NOTE: it may happen that some fields are empty; then we store 2 consecutive commas (,,).

 RETURN: 0 if success; -1 if error preventing file update;
         or 1 if checksum calculation failed after successful file update.
 ***************************************************************************/
int update_product_usage_tracking_file( machine_t *mysys, int lgnum, int Full_Refresh_start,
                                    int Full_Refresh_completion, int register_devices )
{
    char product_usage_tracking_file[FILE_PATH_LEN];
    char product_usage_checksum_file[FILE_PATH_LEN];
    mode_t save_mode, new_mode=0177; /* WR16793 */
    time_t statts;
    ulong current_time;
    group_t *group;
    sddisk_t *sd;
    char *group_number_string;
	int  length;
	int  checksum_file_exists;
	struct stat statbuf;
	int result;

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE *fd = NULL;
#elif defined(SOLARIS)
    int  fd = -1;
#endif

#if defined(SOLARIS)
    char buf[1024];

    memset(buf, 0, 1024);
#endif

    if (!(ISPRIMARY(mysys)))
    {
        return( -1 );	 // Can be called only by the PMD
    }

    (void)time(&statts);
    current_time = (ulong)statts;

    length = strlen(mysys->configpath);
    group_number_string = mysys->configpath + (length - 7);
    sprintf( product_usage_tracking_file, "%s/p%3.3s_migration_tracking.csv", PATH_RUN_FILES, group_number_string );
    sprintf( product_usage_checksum_file, "%s/p%3.3s_migration_tracking.chk", PATH_RUN_FILES, group_number_string );

    // If the checksum file exists for this group, we are not creating the usage file but updating it.
	// Then we must verify the file integrity before doing the update, and change the checksum-valid fields
	// of the lines already entered if the test does not pass.
	// NOTE: if	verify_product_usage_file_checksum() returns a negative value, it means that an error
	//       occurred preventing checksum validation (and a message will have been logged already);
	//       in this case we do not change the checksum-valid fields.
	checksum_file_exists = ( stat(product_usage_checksum_file, &statbuf) == 0 );
	if ( checksum_file_exists )
	{
	    result = verify_product_usage_file_checksum( product_usage_tracking_file, product_usage_checksum_file );
	    if( result == 1 )
		{
		    // The checksum was calculated without error but there was a checksum mismatch.
		    // Set to false the checksum-valid fields of the already entered lines.
		    if( set_product_usage_checksum_failed( product_usage_tracking_file ) != 0 )
			{
                reporterr (ERRFAC, M_WRCHKFAILED, ERRWARN, product_usage_tracking_file);
			}
		}
	}

    save_mode = umask(new_mode);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fd = fopen(product_usage_tracking_file, "a");
#elif defined(SOLARIS)
    fd = open(product_usage_tracking_file, O_WRONLY|O_APPEND|O_CREAT, 0600);
#endif
    umask(save_mode);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (fd == NULL) {
#elif defined(SOLARIS)
    if (fd == -1) {
#endif
        if (errno == EMFILE)
        {
            reporterr (ERRFAC, M_USAGE_FILE, ERRWARN, product_usage_tracking_file, strerror(errno));
			return( -1 );
        }
    }

/* Entries in the csv file
 - Type: 1
 - Version: 1
 - Group number: lgnum
 - Source Server: mysys->name
 - Target Server: othersys->name
 - Source device name: for loop with for (sd = group->headsddisk; sd; sd = sd->n), sd->devname  
 - Source device size: (sd->devsize * (sector size) / 1024) Kbs.
 - Target device name. sd->mirname
 - Full Refresh Start Timestamp:
    (void)time(&statts);
    current_time = (ulong)statts;
 - Full Refresh Completion Timestamp:
    (void)time(&statts);
    current_time = (ulong)statts;
 - Checksum verification passed: 0 or 1; when we update the product usage file, the newly written
    lines have 1 in this field since we recalculate the checksum
*/

    group = mysys->group;

    // Depending on the flag register_devices, write a csv entry for each device pair in the group: Type, Version, Group number,
    //  source server name, target server name, source device name, source device size, target device name, FullRefresh start timestamp
	// FullRefresh completion timestamp if applicable, and 1 for checksum verification passed.
    if( !register_devices )
	{  // No device registration; put empty fields instead
#if defined(HPUX) ||  defined(_AIX) || defined(linux)   
	    fprintf( fd, "1,1,%d,%s,%s,,,,", lgnum, mysys->name, othersys->name );
	    if( Full_Refresh_start )
	        fprintf( fd, "%lu,", current_time );
		else
	        fprintf( fd, "," );
	    if( Full_Refresh_completion )
	        fprintf( fd, "%lu,", current_time );
		else
	        fprintf( fd, "," );

	    fprintf( fd, "1\n" );  // Checksum validation field set to 1 since we will recalculate the checksum

#elif defined(SOLARIS)                                                           
	    sprintf( buf, "1,1,%d,%s,%s,,,,", lgnum, mysys->name, othersys->name );
		write(fd, buf, strlen(buf));
        memset(buf, 0, 1024);

	    if( Full_Refresh_start )
	        sprintf( buf, "%lu,", current_time );
		else
	        sprintf( buf, "," );
		write(fd, buf, strlen(buf));
        memset(buf, 0, 1024);

	    if( Full_Refresh_completion )
	        sprintf( buf, "%lu,", current_time );
		else
	        sprintf( buf, "," );
		write(fd, buf, strlen(buf));
        memset(buf, 0, 1024);

	    sprintf( buf, "1\n" );  // Checksum validation field set to 1 since we will recalculate the checksum
		write(fd, buf, strlen(buf));
		fsync(fd);
#endif
	}
	else
	{
	    // Register all device pairs
	    for (sd = group->headsddisk; sd; sd = sd->n)
	    {

#if defined(HPUX) ||  defined(_AIX) || defined(linux)   
		    fprintf( fd, "1,1,%d,%s,%s,%s,%llu,%s,",
			         lgnum, mysys->name, othersys->name, sd->devname, (sd->devsize * ((u_longlong_t) DEV_BSIZE) / 1024), sd->mirname );

		    if( Full_Refresh_start )
		        fprintf( fd, "%lu,", current_time );
			else
		        fprintf( fd, "," );
		    if( Full_Refresh_completion )
		        fprintf( fd, "%lu,", current_time );
			else
		        fprintf( fd, "," );

		    fprintf( fd, "1\n" );  // Checksum validation field set to 1 since we will recalculate the checksum

#elif defined(SOLARIS)                                                           
		    sprintf( buf, "1,1,%d,%s,%s,%s,%llu,%s,",
			         lgnum, mysys->name, othersys->name, sd->devname, (sd->devsize * ((u_longlong_t) DEV_BSIZE) / 1024), sd->mirname );
			write(fd, buf, strlen(buf));
	        memset(buf, 0, 1024);

		    if( Full_Refresh_start )
		        sprintf( buf, "%lu,", current_time );
			else
		        sprintf( buf, "," );
			write(fd, buf, strlen(buf));
	        memset(buf, 0, 1024);

		    if( Full_Refresh_completion )
		        sprintf( buf, "%lu,", current_time );
			else
		        sprintf( buf, "," );
			write(fd, buf, strlen(buf));
	        memset(buf, 0, 1024);

		    sprintf( buf, "1\n" );  // Checksum validation field set to 1 since we will recalculate the checksum
			write(fd, buf, strlen(buf));
			fsync(fd);
#endif
        } // end of for loop on device pairs
	} // end of if( !register_devices )

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    (void) fclose( fd );
#elif defined(SOLARIS)
    (void) close( fd );
#endif

    result = update_product_usage_checksum_file( product_usage_tracking_file, product_usage_checksum_file );
	if( result != 0 )
	{
	    return( 1 );  // File update done but checksum calculation failed.
	}

    return( 0 );
}

/****************************************************************************
 * initstats -- opens the stats dump file
 ***************************************************************************/
void
initstats (machine_t *mysys)
{
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE* hdrfd;
#elif defined(SOLARIS)
    int  hdrfd;
#endif
    char fil[FILE_PATH_LEN];
    char fil2[FILE_PATH_LEN];
    char filhdr[FILE_PATH_LEN];
#if defined(HPUX)
    char procname[FILE_PATH_LEN];
#endif
    char *t;
    char *cfg_prefix;
    int l;
    mode_t save_mode, new_mode=0177; /* WR16793 */
#if defined(SOLARIS)
    char buf[256];
#endif

    if (mysys->tunables.statinterval < 1) {
        return;
    }
    if (0 != stat (PATH_VAR_OPT, statbuf)) {
        (void) mkdir (PATH_VAR_OPT, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    if (0 != stat (PATH_RUN_FILES, statbuf)) {
        (void) mkdir (PATH_RUN_FILES, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    l = strlen(mysys->configpath);
    t = mysys->configpath + (l - 7);
    if (ISPRIMARY(mysys)) {
        cfg_prefix = "p";
    } else {
        cfg_prefix = "s";
    }
    sprintf(filhdr, "%s/%s%3.3s.phd", PATH_RUN_FILES, cfg_prefix, t);
    sprintf(perfname, "%s/%s%3.3s.prf", PATH_RUN_FILES, cfg_prefix, t);
    sprintf(fil, "%s", perfname);
    sprintf(fil2, "%s.1", perfname);
    (void) unlink (fil2);
    (void) unlink (filhdr);
    (void) rename (fil, fil2);

    /* -- print a "table of contents" for performance file created */
    save_mode = umask(new_mode); /* WR16793 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    hdrfd = fopen (filhdr, "w");
#elif defined(SOLARIS)
    hdrfd = open (filhdr, O_WRONLY|O_CREAT|O_TRUNC, 0600);
#endif
    umask(save_mode); /* WR16793 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (hdrfd  == NULL) {
#elif defined(SOLARIS)
    if (hdrfd  == -1) {
#endif
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, filhdr, strerror(errno));
        }
        return;
    }
    if (ISPRIMARY(mysys)) {
#if defined(HPUX) ||  defined(_AIX) || defined(linux)   
        fprintf (hdrfd, "\"ts\" x\n");
        fprintf (hdrfd, "\"dummy\" 0\n");
        fprintf (hdrfd, "\"Device\" n\n");
        fprintf (hdrfd, "\"Xfer Actual KBps\" y\n");
        fprintf (hdrfd, "\"Xfer Effective KBps\" y\n");
        fprintf (hdrfd, "\"Entries in BAB\" y\n");
        fprintf (hdrfd, "\"Sectors in BAB\" y\n");
        fprintf (hdrfd, "\"Percent Done\" y\n");
        fprintf (hdrfd, "\"Percent BAB Full\" y\n");
        fprintf (hdrfd, "\"Driver Mode\" 0\n");
        fprintf (hdrfd, "\"Read KBps\" y\n");
        fprintf (hdrfd, "\"Write KBps\" y\n");
        
#elif defined(SOLARIS)                                                           
	strcpy(buf, "\"ts\" x\n\"dummy\" 0\n\"Device\" n\n\"Xfer Actual KBps\" y\n\"Xfer Effective KBps\" y\n\"Entries in BAB\" y\n\"Sectors in BAB\" y\n\"Percent Done\" y\n\"Percent BAB Full\" y\n\"Driver Mode\" 0\n\"Read KBps\" y\n\"Write KBps\" y\n");
	write(hdrfd, buf, strlen(buf));
	fsync(hdrfd);
#endif
    } else {
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        fprintf (hdrfd, "\"ts\" x\n");
        fprintf (hdrfd, "\"dummy1\" 0\n");
        fprintf (hdrfd, "\"Device\" n\n");
        fprintf (hdrfd, "\"Xfer Actual KBps\" y\n");
        fprintf (hdrfd, "\"Xfer Effective KBps\" y\n");
        fprintf (hdrfd, "\"Entries per sec recvd\" y\n");
        fprintf (hdrfd, "\"Entry age recvd\" y\n");
        fprintf (hdrfd, "\"Entry age in journal\" y\n");
        fflush (hdrfd);
#elif defined(SOLARIS)
        strcpy(buf, "\"ts\" x\n\"dummy1\" 0\n\"Device\" n\n\"Xfer Actual KBps\" y\n\"Xfer Effective KBps\" y\n\"Entries per sec recvd\" y\n\"Entry age recvd\" y\n\"Entry age in journal\" y\n");
        write(hdrfd, buf, strlen(buf));
        fsync(hdrfd);
#endif
    }
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    (void) fclose (hdrfd);
#elif defined(SOLARIS)
    (void) close (hdrfd);
#endif
    save_mode = umask(new_mode); /* WR16793 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    mysys->statfd = fopen(fil, "w");
#elif defined(SOLARIS)
    mysys->statfd = open(fil, O_WRONLY|O_CREAT|O_TRUNC, 0600);
#endif
    umask(save_mode); /* WR16793 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (mysys->statfd == NULL) {
#elif defined(SOLARIS)
    if (mysys->statfd == -1) {
#endif
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, filhdr, strerror(errno));
        }
    }
    /* DPRINTF("opened stats performance file %s\n", fil); */
    (void)time(&statts);
#if defined(HPUX)
    sprintf (procname, "%05d", (int) getpid());
    if ((mysys->procfd = open (procname, O_RDONLY)) == -1) {
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, filhdr, strerror(errno));
        }
    }
#endif
    return;
} /* initstats */


/****************************************************************************
 * save_csv_stats -- called by PMD to save network stats in csv format
 ***************************************************************************/
int
save_csv_stats( int net_analysis_duration, char *fictitious_dtc_dev_name )
{
    static int firsttime = 1;
    static int lgnum = -1;
    group_t *group;
    devstat_t *devstatp, *statp;
    int rc;

    group = mysys->group;

    if (firsttime)
    {
        lgnum = cfgpathtonum(mysys->configpath);
        (void)init_csv_stats(mysys, net_analysis_duration, fictitious_dtc_dev_name);
        firsttime = 0;
    }
    (void)dump_csv_stats(mysys);

    return (0);
} /* save_csv_stats */

/****************************************************************************
 * savestats -- called by PMD, RMD to save stats into driver memory
 ***************************************************************************/
int
savestats(int force, int network_bandwidth_analysis_mode)
{
    static ftd_lg_info_t lg;
    static ftd_lg_info_t *lgp = NULL;
    static char *devbuf = NULL;
    static int firsttime = 1;
    static int lgnum = -1;
    group_t *group;
    sddisk_t *sd;
    stat_buffer_t sb;
    devstat_t *devstatp, *statp;
    int rc;

    group = mysys->group;

    if (firsttime) {
        lgnum = cfgpathtonum(mysys->configpath);
        if (ISPRIMARY(mysys)) {
		    if( network_bandwidth_analysis_mode )
			{
			    // In Network analysis mode, we have a virtual group, not started in the driver.
				// Then just access the prf file directly.
                (void)initstats(mysys);
			}
			else
			{
	            initdvrstats();
	            memset(&sb, 0, sizeof(stat_buffer_t));
	            sb.lg_num = lgnum;
	            sb.len = sizeof(ftd_lg_info_t);
	            sb.addr = (ftd_uint64ptr_t)&lg;
	            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUPS_INFO, &sb);
	            if (rc != 0) {
#ifdef DEBUG_THROTTLE
	                reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_GROUPS_INFO", 
	                    strerror(errno));
#endif /* DEBUG_THROTTLE */
	                return -1;
	            }
	            lgp = (ftd_lg_info_t*)&lg;
	            devbuf = ftdmalloc(lgp->statsize);
            }
        } else { // RMD
            (void)initstats(mysys);
        }
        firsttime = 0;
    }
    if (ISPRIMARY(mysys)) {
	    if( network_bandwidth_analysis_mode )
		{
		    // In Network analysis mode, we have a virtual group, not started in the driver.
			// Then just access the prf file directly.
            (void)dumpstats(mysys, 1);
		}
		else
		{
	        for (sd = group->headsddisk; sd; sd = sd->n) {
	            memset(&sb, 0, sizeof(stat_buffer_t));
	            sb.lg_num = lgnum;
	            sb.dev_num = sd->sd_rdev;
	            sb.len = lgp->statsize;
	            sb.addr = (ftd_uint64ptr_t)(unsigned long)devbuf;
	            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_DEV_STATE_BUFFER, &sb);
	            if (rc != 0) {
#ifdef DEBUG_THROTTLE
	                reporterr(ERRFAC, M_DRVERR, ERRWARN, "GET_DEV_STATE_BUFFER", 
	                    strerror(errno));
#endif /* DEBUG_THROTTLE */
	                free(devbuf);
	            }
	            /* add our stats to stats returned from shared memory */
	            devstatp = (devstat_t*)(unsigned long)sb.addr;
	            statp = &sd->stat;

	            devstatp->devid = statp->devid;
	            devstatp->a_tdatacnt += statp->a_tdatacnt;
	            devstatp->a_datacnt += statp->a_datacnt;
	            devstatp->e_tdatacnt += statp->e_tdatacnt;
	            devstatp->e_datacnt += statp->e_datacnt;
	            devstatp->entries += statp->entries;

	            devstatp->rfshoffset = statp->rfshoffset;
	            devstatp->rfshdelta = statp->rfshdelta;
#ifdef TDMF_TRACE
	            if (ftd_debugFlag & FTD_DBG_FLOW3) {
	                fprintf(stderr,"\n*** device = %s\n",sd->sddevname);
	                fprintf(stderr,"\n*** devstatp->devid = %d\n",devstatp->devid);
	                fprintf(stderr,"\n*** devstatp->a_datacnt = %d\n",devstatp->a_datacnt);
	                fprintf(stderr,"\n*** devstatp->e_datacnt = %d\n",devstatp->e_datacnt);
	                fprintf(stderr,"\n*** devstatp->entries = %d\n",devstatp->entries);
	                fprintf(stderr,"\n*** devstatp->rfshdelta = %d\n",devstatp->rfshdelta);
	                fprintf(stderr,"\n*** devstatp->rfshoffset = %d\n",devstatp->rfshoffset);
	            }
#endif
	            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_DEV_STATE_BUFFER, &sb);
	            if (rc != 0) {
#ifdef DEBUG_THROTTLE
	                reporterr(ERRFAC, M_DRVERR, ERRWARN, "SET_DEV_STATE_BUFFER", 
	                    strerror(errno));
#endif /* DEBUG_THROTTLE */
	                free(devbuf);
	            }
	            /* reset internal counters */
	            statp->a_tdatacnt = 0;
	            statp->a_datacnt = 0;
	            statp->e_tdatacnt = 0;
	            statp->e_datacnt = 0;
	            statp->entries = 0;
			} // of network_bandwidth_analysis_mode
        }
/*
        free(devbuf);
*/
    } else { // RMD
        flushstats(force);
    }
/* WR37276 -- return value modified for pmd rmd disconnecting issue */
    return (0);
} /* savestats */

/****************************************************************************
 * initdvrstats -- called by PMD to initailize stats in driver memory
 ***************************************************************************/
int
initdvrstats(void)
{
    group_t *group;
    sddisk_t *sd;
    stat_buffer_t sb;
    ftd_lg_info_t lg, *lgp;
    devstat_t devstatp[1];
    char *devbuf;
    int lgnum;
    int rc;

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    memset(devstatp, 0, sizeof(devstat_t));
    memset(&sb, 0, sizeof(stat_buffer_t));

    sb.lg_num = lgnum;
    sb.len = sizeof(ftd_lg_info_t);
    sb.addr = (ftd_uint64ptr_t)(unsigned long)&lg;

    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUPS_INFO, &sb);
    if (rc != 0) {
#ifdef DEBUG_THROTTLE
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_GROUPS_INFO", 
            strerror(errno));
#endif /* DEBUG_THROTTLE */
        return -1;
    }
    lgp = (ftd_lg_info_t*)(unsigned long)sb.addr;
    devbuf = ftdcalloc(1, lgp->statsize);

    for (sd = group->headsddisk; sd; sd = sd->n) {
        sb.lg_num = lgnum;
        sb.dev_num = sd->sd_rdev;
        sb.len = lgp->statsize;
        sb.addr = (ftd_uint64ptr_t)(unsigned long)devbuf;
        rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_DEV_STATE_BUFFER, &sb);
        if (rc != 0) {
#ifdef DEBUG_THROTTLE
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_DEV_STATE_BUFFER", 
                strerror(errno));
#endif /* DEBUG_THROTTLE */
            free(devbuf);
            return -1;
        }
        memset(&sd->stat, 0, sizeof(devstat_t));
        sd->stat.devid = sd->devid;
    }
    free(devbuf);
    return 0;
} /* initdvrstats */

//--------------- Save PMD network stats cumulative total ------------
void
save_network_analysis_total( machine_t *sys, int net_analysis_duration, u_longlong_t net_analysis_total_KBs, char *fictitious_dtc_dev_name )
{
    char fil[FILE_PATH_LEN];
    char fil2[FILE_PATH_LEN];
    char csv_total_perfname[FILE_PATH_LEN];
    char *t;
    char *cfg_prefix;
    int l;
    mode_t save_mode, new_mode=0177; /* WR16793 */
    time_t csv_total_statts;
	ulong  current_time;
#if defined(SOLARIS)
    char buf[256];
#endif
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE*       csv_total_statfd;      /* file descriptor of stat file in csv format */
#elif defined(SOLARIS)
    int         csv_total_statfd;      /* file descriptor of stat file in csv format */
#endif

    (void)time(&csv_total_statts);
    current_time = (ulong)csv_total_statts;

    l = strlen(mysys->configpath);
    t = mysys->configpath + (l - 7);
    cfg_prefix = "p";

    sprintf(csv_total_perfname, "%s/%s%3.3s.csv_total", PATH_RUN_FILES, cfg_prefix, t);
    sprintf(fil, "%s", csv_total_perfname);
    sprintf(fil2, "%s.1", csv_total_perfname);
    (void) unlink (fil2);
    (void) rename (fil, fil2);

    save_mode = umask(new_mode);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    csv_total_statfd = fopen(fil, "w");
#elif defined(SOLARIS)
    csv_total_statfd = open(fil, O_WRONLY|O_CREAT|O_TRUNC, 0600);
#endif
    umask(save_mode);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (csv_total_statfd == NULL) {
#elif defined(SOLARIS)
    if (csv_total_statfd == -1) {
#endif
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, fil, strerror(errno));
			return;
        }
    }

#if defined(HPUX) ||  defined(_AIX) || defined(linux)   
    // Write csv header: Type, Version, Server, Interval, Duration
    fprintf( csv_total_statfd, "NETWORK_BANDWIDTH total %llu KBs transferred during session of %d seconds, server %s, %s (time stamp: %lu)\n",
             net_analysis_total_KBs, net_analysis_duration,  mysys->name, fictitious_dtc_dev_name, current_time );
#elif defined(SOLARIS)                                                           
    // Write csv header: Type, Version, Server, Interval, Duration
	sprintf( buf, "NETWORK_BANDWIDTH total %llu KBs transferred during session of %d seconds, server %s, %s (time stamp: %lu)\n",
             net_analysis_total_KBs, net_analysis_duration,  mysys->name, fictitious_dtc_dev_name, current_time );
	write(csv_total_statfd, buf, strlen(buf));
	fsync(csv_total_statfd);
#endif

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    (void) fclose (csv_total_statfd);
#elif defined(SOLARIS)
    (void) close (csv_total_statfd);
#endif

    return;
} /* save_network_analysis_total */


//--------------- Dump PMD network stats in csv format ------------
void
dump_csv_stats (machine_t *sys)
{
    time_t statts;
    ulong current_time;
    group_t *group;
    sddisk_t *sd;
    char fil[FILE_PATH_LEN];
    char fil2[FILE_PATH_LEN];
    char *t;
    int l;
    char *cfg_prefix;
    mode_t save_mode, new_mode=0177; /* WR16793 */
#if defined(SOLARIS)
    char buf[256];

    memset(buf, 0, 256);
#endif

    (void)time(&statts);
    current_time = (ulong)statts;

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (sys->csv_statfd == (FILE*)NULL) {
#elif defined(SOLARIS)
    if (sys->csv_statfd == -1) {
#endif    
        return;
    }

    group = sys->group;

    for (sd = group->headsddisk; sd; sd = sd->n)
    {
        if (sd->throtstats.actualkbps <= 0.0) sd->throtstats.actualkbps = 0.0;

        // Write the entry type and the current POSIX timestamp.
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        fprintf(sys->csv_statfd, "3,%lu,", current_time);
#elif defined(SOLARIS)
        sprintf(buf, "3,%lu,", current_time);
        write(sys->csv_statfd, buf, strlen(buf));
#endif

        // Write the volume name, the actual Bytes per second sent over the network and the number of data chunks sent.
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        fprintf(sys->csv_statfd, "%s,%lu,%d\n",
                sd->sddevname,
                (ulong)sd->throtstats.actualkbps * 1024,
                (int)(sd->throtstats.actualkbps / (float)(sys->tunables.chunksize/1024)) );
        fflush(sys->csv_statfd);
#elif defined(SOLARIS)
        sprintf(buf, "%s,%lu,%d\n",
                sd->sddevname,
                (ulong)(sd->throtstats.actualkbps) * 1024,
                (int)(sd->throtstats.actualkbps / (float)(sys->tunables.chunksize/1024)) );
        write(sys->csv_statfd, buf, strlen(buf));
        fsync(sys->csv_statfd);
#endif
    } // ... for

    // Check if max stat file size reached.
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (sys->tunables.maxstatfilesize < ftell(sys->csv_statfd)) {
        (void) fclose (sys->csv_statfd);
#elif defined(SOLARIS)
    if (sys->tunables.maxstatfilesize < tell(sys->csv_statfd)) {
        (void) close (sys->csv_statfd);
#endif
        l = strlen(mysys->configpath);
        t = sys->configpath + (l - 7);
        cfg_prefix = "p";
        sprintf(csv_perfname, "%s/%s%3.3s.csv", PATH_RUN_FILES, cfg_prefix, t);
        sprintf(fil, "%s", csv_perfname);
        sprintf(fil2, "%s.1", csv_perfname);
        (void) unlink (fil2);
        (void) rename (fil, fil2);
        save_mode = umask(new_mode);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        sys->csv_statfd = fopen(fil, "w");
#elif defined(SOLARIS)
        sys->csv_statfd = open(fil, O_WRONLY|O_CREAT|O_TRUNC, 0600);
#endif
        umask(save_mode);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        if (sys->csv_statfd == NULL) {
#elif defined(SOLARIS)
        if (mysys->csv_statfd == -1) {
#endif
            if (errno == EMFILE) {
                reporterr (ERRFAC, M_FILE, ERRCRIT, fil, strerror(errno));
            }
        }
    }

  return;
} /* dump_csv_stats */

void
dump_pmd_stats (machine_t *sys, int network_bandwidth_analysis_mode)
{
    time_t statts;
    struct tm *tim;
    group_t *group;
    sddisk_t *sd;
    char fil[FILE_PATH_LEN];
    char fil2[FILE_PATH_LEN];
    char *t;
    int l;
    char *cfg_prefix;
    double pctbab;
    mode_t save_mode, new_mode=0177; /* WR16793 */
#if defined(SOLARIS)
    char buf[256];

    memset(buf, 0, 256);
#endif

    (void)time(&statts);
    tim = localtime(&statts);

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (sys->statfd == (FILE*)NULL) {
#elif defined(SOLARIS)
    if (sys->statfd == -1) {
#endif    
        return;
    }

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fprintf(sys->statfd, "%02d:%02d:%02d ", tim->tm_hour, tim->tm_min,
            tim->tm_sec);
#elif defined(SOLARIS)
    sprintf(buf, "%02d:%02d:%02d ", tim->tm_hour, tim->tm_min,
            tim->tm_sec);
    write(sys->statfd, buf, strlen(buf));
#endif

    group = sys->group;

    for (sd = group->headsddisk; sd; sd = sd->n) {
        if (sd->throtstats.actualkbps <= 0.0) sd->throtstats.actualkbps = 0.0;
        if (sd->throtstats.effectkbps <= 0.0) sd->throtstats.effectkbps = 0.0;
        if (sd->throtstats.entries < 0) sd->throtstats.entries = 0;
        if (sd->throtstats.sectors < 0) sd->throtstats.sectors = 0;
#if defined(HPUX)
        if (ISNANF(sd->throtstats.pctdone))
#elif defined(_AIX)
        if (finite(sd->throtstats.pctdone) == 0)
#elif defined(linux)
        if (isfinite(sd->throtstats.pctdone) == 0)
#else
        if (IsNANorINF(sd->throtstats.pctdone)) 
#endif
            sd->throtstats.pctdone = 0.0;
        if (sd->throtstats.pctdone <= 0.0) sd->throtstats.pctdone = 0.0;
        if (sd->throtstats.pctdone >100.0) sd->throtstats.pctdone = 0.0;
        pctbab = (double) sd->throtstats.pctbab;
#if defined(HPUX)
        if (ISNANF(pctbab))
#elif defined(_AIX)
        if (finite(pctbab) == 0)
#elif defined(linux)
        if (isfinite(pctbab) == 0)
#else
        if (IsNANorINF(pctbab))
#endif
            pctbab = 0.0;
        if (pctbab<0.0) pctbab = 0.0;
        if (pctbab>100.0) pctbab = 100.0;

        if (sd->throtstats.local_kbps_read <= 0.0)
            sd->throtstats.local_kbps_read = 0.0;
        if (sd->throtstats.local_kbps_written <= 0.0)
            sd->throtstats.local_kbps_written = 0.0;

        if( network_bandwidth_analysis_mode )
		    sys->group->throtstats.drvmode = DRVR_NETANALYSIS;

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        fprintf(sys->statfd, " || %s %6.2f %6.2f %d %d %6.2f %6.2f %d %6.2f %6.2f",
                sd->sddevname,
                sd->throtstats.actualkbps,
                sd->throtstats.effectkbps,
                sd->throtstats.entries,
                sd->throtstats.sectors,
                sd->throtstats.pctdone,
                pctbab,
                sys->group->throtstats.drvmode,
                sd->throtstats.local_kbps_read,
                sd->throtstats.local_kbps_written);
        
        fflush(sys->statfd);
#elif defined(SOLARIS)
        sprintf(buf, " || %s %6.2f %6.2f %d %d %6.2f %6.2f %d %6.2f %6.2f",
                sd->sddevname,
                sd->throtstats.actualkbps,
                sd->throtstats.effectkbps,
                sd->throtstats.entries,
                sd->throtstats.sectors,
                sd->throtstats.pctdone,
                pctbab,
                sys->group->throtstats.drvmode,
                sd->throtstats.local_kbps_read,
                sd->throtstats.local_kbps_written);
        write(sys->statfd, buf, strlen(buf));
        fsync(sys->statfd);
#endif
    }
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fprintf(sys->statfd, "\n");
    fflush(sys->statfd);
#elif defined(SOLARIS)
    strcpy(buf,"\n");
    write(sys->statfd, buf, strlen(buf));
    fsync(sys->statfd);
#endif
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (sys->tunables.maxstatfilesize < ftell(sys->statfd)) {
        (void) fclose (sys->statfd);
#elif defined(SOLARIS)
    if (sys->tunables.maxstatfilesize < tell(sys->statfd)) {
        (void) close (sys->statfd);
#endif
        gettunables(group->devname, 1, 0);       /* calling gettunables to get the latest value of tunables from pstore */ 
        l = strlen(mysys->configpath);
        t = sys->configpath + (l - 7);
        cfg_prefix = "p";
        sprintf(perfname, "%s/%s%3.3s.prf", PATH_RUN_FILES, cfg_prefix, t);
        sprintf(fil, "%s", perfname);
        sprintf(fil2, "%s.1", perfname);
        (void) unlink (fil2);
        (void) rename (fil, fil2);
        save_mode = umask(new_mode); /* WR16793 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        sys->statfd = fopen(fil, "w");
#elif defined(SOLARIS)
        sys->statfd = open(fil, O_WRONLY|O_CREAT|O_TRUNC, 0600);
#endif
        umask(save_mode); /* WR16793 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        if (sys->statfd == NULL) {
#elif defined(SOLARIS)
        if (mysys->statfd == -1) {
#endif
            if (errno == EMFILE) {
                reporterr (ERRFAC, M_FILE, ERRCRIT, fil, strerror(errno));
            }
        }
    }

  return;
} /* dump_pmd_stats */

/****************************************************************************
 * dump_rmd_stats -- writes the current statistics to the statdump file
 ***************************************************************************/
void
dump_rmd_stats (machine_t *sys)
{
    time_t statts;
    static time_t lastts;
    static int firsttime = 1;
    struct tm *tim;
    group_t *group;
    sddisk_t *sd;
    char fil[FILE_PATH_LEN];
    char fil2[FILE_PATH_LEN];
    char *cfg_prefix;
    char *t;
    float actualkbps;
    float effectkbps;
    float entries;
    int deltasec;
    int l;
    mode_t save_mode, new_mode=0177; /* WR16793 */
char                group_name[MAXPATHLEN];
 int cf;
#if defined(SOLARIS)
    char buf[256];

    memset(buf, 0, 256);
#endif
    (void)time(&statts);
    if (firsttime) {
        lastts = statts - sys->tunables.statinterval;
        firsttime = 0;
    }
    deltasec = statts - lastts;

    if (deltasec < sys->tunables.statinterval) {
        return;
    }
    tim = localtime(&statts);

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (sys->statfd == (FILE*)NULL) {
#elif defined(SOLARIS)
     if (sys->statfd == -1) {	
#endif    
        return;
    }
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fprintf(sys->statfd, "%02d:%02d:%02d ", tim->tm_hour, tim->tm_min,
            tim->tm_sec);
#elif defined(SOLARIS) 
    sprintf(buf, "%02d:%02d:%02d ", tim->tm_hour, tim->tm_min, tim->tm_sec);
    write(sys->statfd, buf, strlen(buf));
#endif

    group = sys->group;

    for (sd = group->headsddisk; sd; sd = sd->n) {
        /* calculate rmd stats */
        actualkbps = ((sd->stat.a_tdatacnt * 1.0) / (deltasec * 1.0)) / 1024.0;
        effectkbps = ((sd->stat.e_tdatacnt * 1.0) / (deltasec * 1.0)) / 1024.0;
        entries = ((sd->stat.entries * 1.0) / deltasec);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        fprintf(sys->statfd, " || %s %6.2f %6.2f %6.2f %d %d",
                sd->mirname,
                actualkbps,
                effectkbps,
                entries,
                sd->stat.entage,
                sd->stat.jrnentage);
        fflush(sys->statfd);
        sd->stat.a_tdatacnt = 0;
        sd->stat.e_tdatacnt = 0;
        sd->stat.entries = 0;
#elif defined(SOLARIS)
	sprintf(buf, " || %s %6.2f %6.2f %6.2f %d %d",
                sd->mirname,
                actualkbps,
                effectkbps,
                entries,
                sd->stat.entage,
                sd->stat.jrnentage);
	write(sys->statfd, buf, strlen(buf));
	fsync(sys->statfd);
        sd->stat.a_tdatacnt = 0;
        sd->stat.e_tdatacnt = 0;
        sd->stat.entries = 0;
#endif
    }
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fprintf(sys->statfd, "\n");
    fflush(sys->statfd);
#elif defined(SOLARIS)
    strcpy(buf,"\n");
    write(sys->statfd, buf, strlen(buf));
    fsync(sys->statfd);
#endif
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (sys->tunables.maxstatfilesize < ftell(sys->statfd)) {
        (void) fclose (sys->statfd);
#elif defined(SOLARIS)
    if (sys->tunables.maxstatfilesize < tell(sys->statfd)) {
        (void) close (sys->statfd);
#endif
        
        l = strlen(mysys->configpath);
        t = sys->configpath + (l - 7);
        cfg_prefix = "s";
        sprintf(perfname, "%s/%s%3.3s.prf", PATH_RUN_FILES, cfg_prefix, t);
        sprintf(fil, "%s", perfname);
        sprintf(fil2, "%s.1", perfname);
        (void) unlink (fil2);
        (void) rename (fil, fil2);
        save_mode = umask(new_mode); /* WR16793 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        sys->statfd = fopen(fil, "w");
#elif defined(SOLARIS)
        sys->statfd = open(fil, O_WRONLY|O_CREAT|O_TRUNC, 0600);
#endif
        umask(save_mode); /* WR16793 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        if (sys->statfd == NULL) {
#elif defined(SOLARIS)
        if (sys->statfd  == -1) {
#endif
            if (errno == EMFILE) {
                reporterr (ERRFAC, M_FILE, ERRCRIT, fil, strerror(errno));
            }
        }
    }
    lastts = statts;
    return;
} /* dump_rmd_stats */
