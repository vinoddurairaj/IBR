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
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_GET_PRODUCT_USAGE_DATA_C_
#define _FTD_MNGT_GET_PRODUCT_USAGE_DATA_C_

#include <ctype.h>

#include "md5.h"
#include "stat_intr.h"

extern unsigned char checksum_secret_key[];


/****************************************************************************
 * Agent_calculate_product_usage_checksum -- compute checksum on given product usage
 * tracking file
 * Return: the length of the checksum string or -1 if error
 ***************************************************************************/
static int Agent_calculate_product_usage_checksum( char *product_usage_tracking_file, unsigned char *checksum_digests_ptr )
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
		sprintf(logmsg, "in function Agent_calculate_product_usage_checksum: file %s not found [%s].\n", product_usage_tracking_file, strerror(errno));
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		return( -1 );
	}

	if( (fd = open(product_usage_tracking_file, O_RDONLY)) == -1 )
	{
		sprintf(logmsg, "in function Agent_calculate_product_usage_checksum: failed opening file %s [%s].\n", product_usage_tracking_file, strerror(errno));
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		return( -1 );
	}

	if( (buffer = (char*)malloc(statbuf.st_size + checksum_secret_key_length + 4)) == NULL)
	{
		sprintf(logmsg, "in function Agent_calculate_product_usage_checksum: failed allocating memory to read %s.\n", product_usage_tracking_file);
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		close(fd);
		return( -1 );
	}

    // Copy our secret key at the beginning of the buffer
    strcpy( buffer, checksum_secret_key );


    /* Read the product usage tracking file in the buffer, following the secret key */
	if( read( fd, buffer + checksum_secret_key_length, statbuf.st_size) != statbuf.st_size )
	{
		sprintf(logmsg, "in function Agent_calculate_product_usage_checksum: failed reading file %s [%s].\n", product_usage_tracking_file, strerror(errno));
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
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
		sprintf(logmsg, "in function Agent_calculate_product_usage_checksum: failed calculating checksum on %s; buffer overflow would occur.\n", product_usage_tracking_file);
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		digest_total_length = -1;
	}

	close(fd);
	free(buffer);
    return( digest_total_length );
}


/****************************************************************************
 Agent_verify_product_usage_file_checksum
 Calculate the checksum on a product usage tracking file and return 1 if not valid, 0 if valid,
 -1 if error occurred
*/
static int Agent_verify_product_usage_file_checksum( char *product_usage_tracking_file, char *product_usage_checksum_file )
{
    unsigned char checksum_digests[DIGESTSIZE * MAX_PROD_USAGE_CHECKSUM_DIGESTS];
	int checksum_string_length;
	int fd;
	unsigned char buffer[DIGESTSIZE * MAX_PROD_USAGE_CHECKSUM_DIGESTS];
	struct stat statbuf;
	int j, compare_result;

    // Get the checksum digests on the product usage tracking file
    checksum_string_length = Agent_calculate_product_usage_checksum( product_usage_tracking_file, checksum_digests );
	if( checksum_string_length < 0 )
	    return( -1 );

    // Compare the calculated file checksum with its related value previously saved in the checksum file
	if( stat( product_usage_checksum_file, &statbuf ) != 0 )
	{
		sprintf(logmsg, "in function Agent_verify_product_usage_file_checksum: file %s not found [%s].\n", product_usage_checksum_file, strerror(errno));
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		return( -1 );
	}

	if( (fd = open(product_usage_checksum_file, O_RDONLY)) == -1 )
	{
		sprintf(logmsg, "in function Agent_verify_product_usage_file_checksum: failed opening %s [%s].\n", product_usage_checksum_file, strerror(errno));
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		return( -1 );
	}

    /* Read the product usage checksum file in the buffer */
	if( read( fd, buffer, statbuf.st_size) != statbuf.st_size )
	{
		sprintf(logmsg, "in function Agent_verify_product_usage_file_checksum: failed reading file %s [%s].\n", product_usage_checksum_file, strerror(errno));
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
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
 * Agent_set_product_usage_checksum_failed
 * Set all the checksum-validated fields to 0 in the specified
 * product usage tracking file
 Return: 0 = success; -1 = error occurred
*/
static int Agent_set_product_usage_checksum_failed( char *product_usage_tracking_file )
{
	int fd;
	unsigned char *buffer;
	struct stat statbuf;
	int j;
	int file_size;

	if( stat( product_usage_tracking_file, &statbuf ) != 0 )
	{
		sprintf(logmsg, "in function Agent_set_product_usage_checksum_failed: file %s not found [%s].\n", product_usage_tracking_file, strerror(errno));
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		return( -1 );
	}

	if( (fd = open(product_usage_tracking_file, O_RDWR)) == -1 )
	{
		sprintf(logmsg, "in function Agent_set_product_usage_checksum_failed: failed opening file %s [%s].\n", product_usage_tracking_file, strerror(errno));
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		return( -1 );
	}

	if( (buffer = (char*)malloc(statbuf.st_size + 4)) == NULL)
	{
		sprintf(logmsg, "in function Agent_set_product_usage_checksum_failed: failed allocating memory to read %s.\n", product_usage_tracking_file);
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		close(fd);
		return( -1 );
	}

    /* Read the product usage tracking file in the buffer */
	if( read( fd, buffer, statbuf.st_size) != statbuf.st_size )
	{
		sprintf(logmsg, "in function Agent_set_product_usage_checksum_failed: failed reading file %s [%s].\n", product_usage_tracking_file, strerror(errno));
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		close(fd);
		free(buffer);
		return -1;
	}

    // Go through all the lines and set the checksum-validated fields to 0 (start with j = 1)
	// NOTE: the last character of each line can only be 1 or 0; if not, the line has been altered
	//       at its end; in this case we do not touch it; the report must detect that this line is
	//       altered by not finding a 1 or a 0 at the end.
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
		sprintf(logmsg, "in function Agent_set_product_usage_checksum_failed: failed writing file %s [%s].\n", product_usage_tracking_file, strerror(errno));
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		close(fd);
		free(buffer);
		return -1;
	}

	close(fd);
	return( 0 );
}


/*
   collect_server_product_usage_stats
   The Agent verifies the checksums on the product usage tracking files and, if applicable,
   adjusts the field Checksum-verification-passed in the entries of the individual files;
   then it concatenates all the individual group files and the global file, called:
   SFTKdtc_<server name or IP address>_product_usage_stats_<timestamp>.csv, is loaded in memory
   and returned to the caller.
   Return: 0 if success; -1 if error.
*/
int collect_server_product_usage_stats( char **pDataBuffer, int *iNumberOfBytes )
{

	char		cmdline[256], cat_command[256], cat_path[32];
	FILE		*f;
	char		filename[MMP_MNGT_MAX_FILENAME_SZ];
	char		product_usage_tracking_file[MMP_MNGT_MAX_FILENAME_SZ];
	char		product_usage_checksum_file[MMP_MNGT_MAX_FILENAME_SZ];
	char		server_product_usage_global_file[MMP_MNGT_MAX_FILENAME_SZ];
	char        hostname_string[128];
	int         checksum_result;
    time_t      current_time;
    struct tm*  time_info;
	char        timestamp[32];
	int         first_pass;
	struct stat statbuf;
	int         fd;
	int         file_extension_offset;
	char        *buffer;
	int         csv_file_found;

    // Zero the caller's variables in case of error
    *pDataBuffer = NULL;
    *iNumberOfBytes	= 0;

	memset(product_usage_tracking_file, 0, sizeof(product_usage_tracking_file));
	memset(product_usage_checksum_file, 0, sizeof(product_usage_checksum_file));

    // Get the hostname for the global product usage stats file
	memset(hostname_string,0x00,sizeof(hostname_string));
	gethostname(hostname_string,sizeof(hostname_string));

    // Get the timestamp of the current time for the global product usage stats file
    (void)time( &current_time );
    time_info = localtime( &current_time );
	memset( timestamp, 0x00, sizeof(timestamp) );
    strftime( timestamp, sizeof(timestamp), "%Y-%m-%d-%Hh%Mm%S", time_info );

	// Set the global product usage stats file name
    sprintf( server_product_usage_global_file, "%s/SFTKdtc_%s_product_usage_stats_%s.csv", DTCVAROPTDIR, hostname_string, timestamp );

	sprintf(logmsg, "Collecting SFTKdtc product usage statistics in %s...\n", server_product_usage_global_file);
	logout(9,F_collect_server_product_usage_stats,logmsg);

#if defined(linux)
	sprintf( cmdline, "/bin/ls -1 %s/p[0-9][0-9][0-9]_migration_tracking.csv 2> /dev/null", DTCVAROPTDIR );
#else
	sprintf( cmdline, "/usr/bin/ls -1 %s/p[0-9][0-9][0-9]_migration_tracking.csv 2> /dev/null", DTCVAROPTDIR );
#endif
    
    // Dynamically discover the path to the cat command; may differ between OSes and even between Linux releases (see defect 69702)
	if( stat( "/usr/bin/cat", &statbuf ) == 0 )
	{
	    strcpy( cat_path, "/usr/bin/cat" );
	}
	else
	{
	    strcpy( cat_path, "/bin/cat" );
	}

    first_pass = 1;
    csv_file_found = 0;
    // For each product usage stats file (pxxx_migration_tracking.csv), verify its integrity
	// by validating with its corresponding checksum file (pxxx_migration_tracking.chk);
	// reset the fields checksum-verification-passed of the files that fail the verification.
	if( (f = popen(cmdline, "r")) != NULL )
	{
		while( fgets(filename, MMP_MNGT_MAX_FILENAME_SZ, f) != NULL )
		{
		    filename[ strlen(filename)-1 ] = '\0';
            csv_file_found = 1;
            // For each product usage tracking file, set its full path and name, and also for its
            // corresponding checksum file
			sprintf( product_usage_tracking_file, "%s", filename );
			strcpy( product_usage_checksum_file, product_usage_tracking_file );
			// Replace the .csv by .chk in the checksum filename
			file_extension_offset = strlen( product_usage_checksum_file ) - 3;
			strcpy( product_usage_checksum_file+file_extension_offset, "chk" );

			sprintf(logmsg, "Processing %s...\n", product_usage_tracking_file);
			logout(9,F_collect_server_product_usage_stats,logmsg);

            // Check each stats file<s integrity (checksum)
			// NOTE: Agent_verify_product_usage_file_checksum	returns 1 if not valid, 0 if valid,  -1 if error occurred
            checksum_result = Agent_verify_product_usage_file_checksum( product_usage_tracking_file, product_usage_checksum_file );
			if( checksum_result == 1 )
			{
			    // Checksum verification failed; set the checksum-ok fields to 0 in the csv file
				sprintf(logmsg, "WARNING: %s: checksum verification reports checksum mismatch.\n", product_usage_tracking_file);
				logout(4,F_collect_server_product_usage_stats,logmsg);

                if( Agent_set_product_usage_checksum_failed( product_usage_tracking_file ) < 0 )
				{
					sprintf(logmsg, "in function collect_server_product_usage_stats: failed setting checksum fields to 0 (bad) in %s.\n", product_usage_tracking_file);
					logout(4,F_collect_server_product_usage_stats,logmsg);
				}
			}
			else if( checksum_result < 0 )
			{
			    // An error occurred; log a message
				sprintf(logmsg, "WARNING: %s: checksum verification could not be done (error occurred).\n", product_usage_tracking_file);
				logout(4,F_collect_server_product_usage_stats,logmsg);
			}

			// concatenate each product usage tracking file to the end of the server_product_usage_global_file
			if( first_pass )
			{
	            sprintf( cat_command, "%s %s > %s 2> /dev/null", cat_path, product_usage_tracking_file, server_product_usage_global_file );
                first_pass = 0;
			}
			else
			{
	            sprintf( cat_command, "%s %s >> %s 2> /dev/null", cat_path, product_usage_tracking_file, server_product_usage_global_file );
			}
			if( system( cat_command ) != 0 )
		    {
		        logoutx(4, F_collect_server_product_usage_stats, " error while concatenating product usage stats to global file", "errno", errno);
			}
	        memset(product_usage_tracking_file, 0, sizeof(product_usage_tracking_file));
	        memset(product_usage_checksum_file, 0, sizeof(product_usage_checksum_file));
		}
		pclose(f);

        if( csv_file_found )
		{
		    // Read the global file in memory and return it to the caller
			if( stat( server_product_usage_global_file, &statbuf ) != 0 )
			{
				logoutx(4, F_collect_server_product_usage_stats, server_product_usage_global_file, " failed reading the file, errno ", errno);
				return( -1 );
			}

			if( (fd = open(server_product_usage_global_file, O_RDONLY)) == -1 )
			{
				logoutx(4, F_collect_server_product_usage_stats, server_product_usage_global_file, " failed reading the file, errno ", errno);
				return( -1 );
			}

			if( (buffer = (char*)malloc(statbuf.st_size + 4)) == NULL)
			{
				logoutx(4, F_collect_server_product_usage_stats, server_product_usage_global_file, " failed allocating memory for reading the file, errno ", errno);
				close(fd);
				return( -1 );
			}

		    memset(buffer, 0, statbuf.st_size + 4);

		    /* Read the file */
			if( read( fd, buffer, statbuf.st_size) != statbuf.st_size )
			{
				logoutx(4, F_collect_server_product_usage_stats, server_product_usage_global_file, " failed reading the file, errno ", errno);
				close(fd);
				free( buffer );
				return( -1 );
			}
			close( fd );
		}

	    *pDataBuffer = buffer;

        if( csv_file_found )
		{
	        *iNumberOfBytes	= statbuf.st_size;
		}
		else
		{
			logoutx(9, F_collect_server_product_usage_stats, server_product_usage_global_file, " not generated because there are no csv files to collect; returning length ", 0);
	        *iNumberOfBytes	= 0;
		}

		return( 0 );
	}
	else
	{
		logoutx(4, F_collect_server_product_usage_stats, "popen failed", "errno", errno);
		return( -1 );
	}
}

/*
   MMP_MNGT_GET_PRODUCT_USAGE_DATA request
*/
int
ftd_mngt_get_product_usage_data(sock_t *sockID)
{
	int		        r = 0;
	int		        towrite;
	char            c;
	char            *pFileData;
	char            szFullPathFname[256];
	char            ipstr[48];
	unsigned int    uiFileSize;
	mmp_mngt_FileMsg_t     *pRcvFileMsg;
	mmp_mngt_FileMsg_t     *pResponseFileMsg;

	/* complete reception of TDMF file message */
	mmp_mngt_recv_file_data(sockID, NULL, &pRcvFileMsg, 1);

	if (pRcvFileMsg == NULL) {
		/* problem receiving the message data */
		ip_to_ipstring( sockID->rip, ipstr );
		sprintf(logmsg, "while receiving "PRODUCTNAME" Product usage data request from %s %s\n", ipstr, sockID->rhostname);
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
		return -1;
	}

	/* convertion from network bytes order to host byte order done in mmp_mngt_recv_file_data() */
	sprintf(logmsg, "Get "PRODUCTNAME" PRODUCT USAGE msg received.\n");
	logout(9, F_ftd_mngt_get_product_usage_data, logmsg);


	pFileData   = 0;
	uiFileSize  = 0;
		/* content of file is transfered to pFileData           */
		/*                      (memory allocated within fnct)  */
	r = collect_server_product_usage_stats( &pFileData, &uiFileSize );
	if ( r != 0 ) {
		/* err reading file */
		sprintf(logmsg, "reading, "PRODUCTNAME" Product usage statistics file\n");
		logout(4,F_ftd_mngt_get_product_usage_data,logmsg);
	}

	/* prepare response message */
	mmp_mngt_build_SetTdmfFileMsg(&pResponseFileMsg, 
									pRcvFileMsg->szServerUID,
									1, /* it is a host id */
									SENDERTYPE_TDMF_AGENT,  
									NULL,  // no filename 
									pFileData, uiFileSize,
									pRcvFileMsg->data.iType );

	/* send status message to requester */
	towrite = sizeof(mmp_mngt_FileMsg_t) + ntohl(pResponseFileMsg->data.uiSize);
	r = ftd_sock_send(sockID, (char *)pResponseFileMsg, towrite);
    if( r < 0 )
	{
	    // Log error message
		sprintf(logmsg, "Error while attempting to send Product Usage data.\n");
		logout(4, F_ftd_mngt_get_product_usage_data, logmsg);
	}
	else
	{
	    // Log success message
		sprintf(logmsg, "Sent Product Usage data.\n");
		logout(9, F_ftd_mngt_get_product_usage_data, logmsg);
	}
	/* release memory obtained from mmp_mngt_build_SetTdmfFileMsg() */
	mmp_mngt_release_SetTdmfFileMsg(&pResponseFileMsg);
	/* release memory obtained from mmp_mngt_recv_file_data() */
	mmp_mngt_free_file_data_mem(&pRcvFileMsg);

	if (pFileData) {
		free(pFileData);
	}

	return r;
}
#endif /* _FTD_MNGT_GET_PRODUCT_USAGE_DATA_C_ */

