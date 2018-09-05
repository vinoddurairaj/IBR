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
 * cfg_intr.c - system config file interface
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#include <unistd.h>
#if defined(_AIX)
#include <macros.h> 
#include <stdlib.h>
#else  /* defined(_AIX) */
#include <stdlib.h>
#if !defined(linux)
#include <macros.h>
#endif /* !defined(linux) */
#endif /* defined(_AIX) */
#include <stdio.h>
#include <fcntl.h> 
#include <string.h> 
#include <sys/param.h> 
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "cfg_intr.h"
#include "errors.h"
#include "ftd_cmd.h"
#include "ftdio.h"
#include "pathnames.h"

static char *getline2 (char **buffer, int *linelen);

/*
 * Parse the system config file for something like: key=value;
 *
 * If stringval is TRUE, then remove quotes from the value.
 */
int
cfg_get_key_value(char *key, char *value, int stringval)
{
    int         fd, len;
    char        *ptr, *buffer, *temp, *line;
    struct stat statbuf;

    if (stat(FTD_SYSTEM_CONFIG_FILE, &statbuf) != 0) {
        return CFG_BOGUS_CONFIG_FILE;
    }

    /* open the config file and look for the pstore key */
    if ((fd = open(FTD_SYSTEM_CONFIG_FILE, O_RDONLY)) == -1) {
        return CFG_BOGUS_CONFIG_FILE;
    }

    if ((buffer = (char*)malloc(2*statbuf.st_size)) == NULL) { /* TODO: Why allocate twice the size of the file? */
        close(fd);
        return CFG_MALLOC_ERROR;
    }

    /* read the entire file into the buffer. */
    if (read(fd, buffer, statbuf.st_size) != statbuf.st_size) {
        close(fd);
        free(buffer);
        return CFG_READ_ERROR;
    }
    close(fd);
    // The read value needs to be NULL terminated, otherwise, we'll keep looking for unfound values (and modify things) out of the allocated array.
    buffer[statbuf.st_size] = 0x0;
    temp = buffer;

    /* get lines until we find the right one */
    while ((line = getline2(&temp, &len)) != NULL) {
        if (line[0] == '#') {
            continue;
        } else if ((ptr = strstr(line, key)) != NULL) {
            /* search for quotes, if this is a string */
            if (stringval) {
                while (*ptr) {
                    if (*ptr++ == '\"') {
                        while (*ptr && (*ptr != '\"')) {
                            *value++ = *ptr++;
                        }
                        *value = 0;
                        free(buffer);
                        return CFG_OK;
                    }
                }
            } else {
                while (*ptr) {
                    if (*ptr++ == '=') {
                        while (*ptr && (*ptr != ';')) {
                            *value++ = *ptr++;
                        }
                        free(buffer);
                        return CFG_OK;
                    }
                }
            }
        }
    }
    free(buffer);
    return CFG_BOGUS_CONFIG_FILE;    
}

/*
 * Parse the system config file for something like: key=value; 
 * Replace old value with new value. If key doesn't exist, add key/value.
 *
 * If stringval is TRUE, put quotes around the value.
 */
int
cfg_set_key_value(char *key, char *value, int stringval)
{
    int         fd, found, linelen;
    char        *inbuffer, *outbuffer, *line;
    char        *tempin, *tempout;
    struct stat statbuf;

    if (stat(FTD_SYSTEM_CONFIG_FILE, &statbuf) != 0) {
        return CFG_BOGUS_CONFIG_FILE;
    }

    if ((fd = open(FTD_SYSTEM_CONFIG_FILE, O_RDWR)) < 0) {
        return CFG_BOGUS_CONFIG_FILE;
    }

    if ((inbuffer = (char *)calloc(statbuf.st_size+1, 1)) == NULL) {
        close(fd);
        return CFG_MALLOC_ERROR;
    }

    if ((outbuffer = (char *)malloc(statbuf.st_size+MAXPATHLEN)) == NULL) {
        close(fd);
        return CFG_MALLOC_ERROR;
    }

    /* read the entire file into the buffer. */
    if (read(fd, inbuffer, statbuf.st_size) != statbuf.st_size) {
        close(fd);
        return CFG_READ_ERROR;
    }
    close(fd);
    tempin = inbuffer;
    tempout = outbuffer;
    found = 0;

    /* get lines until we find one with: key=something */
    while ((line = getline2(&tempin, &linelen)) != NULL) {

        /* if this is a comment line or it doesn't have the magic word ... */
        if ((line[0] == '#') || (strstr(line, key) == NULL)) {
            /* copy the line to the output buffer */
            strncpy(tempout, line, linelen);
            tempout[linelen] = '\n';
            tempout += linelen+1;
        } else {
            if (stringval) {
                sprintf(tempout, "%s=\"%s\";\n", key, value);
            } else {
                sprintf(tempout, "%s=%s;\n", key, value);
            }
            tempout += strlen(tempout);
            found = 1;
        }
    }
    free(inbuffer);

    if (!found) {
        if (stringval) {
            sprintf(tempout, "%s=\"%s\";\n", key, value);
        } else {
            sprintf(tempout, "%s=%s;\n", key, value);
        }
        tempout += strlen(tempout);
    }

    /* flush the output buffer to disk */
    if ((fd = creat(FTD_SYSTEM_CONFIG_FILE, S_IRUSR | S_IWUSR)) == -1) {
        free(outbuffer);
        return CFG_WRITE_ERROR;
    }
    lseek(fd, 0, SEEK_SET);
    write(fd, outbuffer, tempout - outbuffer);
    close(fd);

    free(outbuffer);

    return CFG_OK;
}

/*
 * Yet another "getline" function. Parses a buffer looking for the
 * EOL or a NULL. Bumps the buffer pointer to the character following
 * EOL or to NULL, if end-of-buffer. Replaces the EOL with a NULL.
 *
 * Returns NULL, if parsing failed (end-of-buffer reached). 
 * Returns pointer to start-of-line, if parsing succeeded. 
 */
static char *
getline2 (char **buffer, int *outlen)
{
    int  len;
    char *tempbuf;

    tempbuf = *buffer;
    if (tempbuf == NULL) {
	return NULL;
    }

    /* search for EOL or NULL */
    len = 0;
    while (1) {
	if (tempbuf[len] == '\n') {
	    tempbuf[len] = 0;
	    *buffer = &tempbuf[len+1];
	    *outlen = len;
	    break;
	} else if (tempbuf[len] == 0) {
	    /* must be done! */
	    *buffer = NULL;
	    if (len == 0) tempbuf = NULL;
	    *outlen = len;
	    break;
	}
	len++;
    }

    /* done */
    return tempbuf;
}

#if defined(linux)
/*
 * update_config_value(): update MODULES_CONFIG_PATH
 *	base function is cfg_set_key_value().
 *	getting num_chunks and chunk_size, output to /etc/modules.conf.
 */
int
update_config_value()
{
    int         fd, found, linelen;
    char        *inbuffer, *outbuffer, *line;
    char        *tempin, *tempout;
    struct stat statbuf;
    char        tempbuff[1024];
    int         num_chunks, chunk_size;
    char        key[32];

    num_chunks = chunk_size = 0;

    memset(tempbuff, 0, 1024);
    if ((cfg_get_key_value( "num_chunks", tempbuff, 
                           CFG_IS_NOT_STRINGVAL)) == CFG_OK) {
        num_chunks = atoi(tempbuff);
    } else {
        return CFG_READ_ERROR;
    }

    memset(tempbuff, 0, 1024);
    if ((cfg_get_key_value( "chunk_size", tempbuff, 
                           CFG_IS_NOT_STRINGVAL)) == CFG_OK) {
        chunk_size = atoi(tempbuff);
    } else {
        return CFG_READ_ERROR;
    }

    if (stat(MODULES_CONFIG_PATH, &statbuf) != 0) {
        return CFG_BOGUS_CONFIG_FILE;
    }

    if ((fd = open(MODULES_CONFIG_PATH, O_RDWR)) < 0) {
        return CFG_BOGUS_CONFIG_FILE;
    }

    if ((inbuffer = (char *)calloc(statbuf.st_size+1, 1)) == NULL) {
        close(fd);
        return CFG_MALLOC_ERROR;
    }

    if ((outbuffer = (char *)malloc(statbuf.st_size+MAXPATHLEN)) == NULL) {
        close(fd);
        return CFG_MALLOC_ERROR;
    }

    /* read the entire file into the buffer. */
    if (read(fd, inbuffer, statbuf.st_size) != statbuf.st_size) {
        close(fd);
        return CFG_READ_ERROR;
    }
    close(fd);
    tempin = inbuffer;
    tempout = outbuffer;
    found = 0;

    /* get lines until we find one with: key=something */
    sprintf(key, "options %s", MODULES_NAME);
    while ((line = getline2(&tempin, &linelen)) != NULL) {
        /* if this is a comment line or it doesn't have the magic word ... */
        if ((line[0] == '#') || (strstr(line, key) == NULL)) {
            /* copy the line to the output buffer */
            strncpy(tempout, line, linelen);
            tempout[linelen] = '\n';
            tempout += linelen+1;
        } else {
            sprintf(tempout, "options %s num_chunks=%d chunk_size=%d\n", MODULES_NAME, num_chunks, chunk_size);
            tempout += strlen(tempout);
            found = 1;
        }
    }
    free(inbuffer);

    if (!found) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
        sprintf(tempout, "keep\n");
        tempout += strlen(tempout);
        sprintf(tempout, "path[misc]=%s\n", PATH_DRIVER_FILES);
        tempout += strlen(tempout);
#endif /* < KERNEL_VERSION(2, 6, 0) */
        sprintf(tempout, "options %s num_chunks=%d chunk_size=%d\n", MODULES_NAME, num_chunks, chunk_size);
        tempout += strlen(tempout);
    }

    /* flush the output buffer to disk */
    if ((fd = creat(MODULES_CONFIG_PATH, S_IRUSR | S_IWUSR)) == -1) {
        free(outbuffer);
        return CFG_WRITE_ERROR;
    }
    lseek(fd, 0, SEEK_SET);
    write(fd, outbuffer, tempout - outbuffer);
    close(fd);

    free(outbuffer);

    return CFG_OK;
}
#endif

/*
 * dtc_open_err_check_driver_config_file_and_log_error_message(): upon failure opening the dtc ctl device, 
   this module can be called to check the driver config file to determine if the BAB size has been set 
   (dtcinit -b has been called) and to log a proper error message. If BAB parameters have not been defined,
   configuration procedures have not been followed and the driver has not been loaded properly.
   WR PROD1436
 */
void
dtc_open_err_check_driver_config_file_and_log_error_message(int error_code)
{
    char        tempbuff[1024];
	int         config_ok = 0;

    memset(tempbuff, 0, 1024);
    config_ok = ( (cfg_get_key_value( "num_chunks", tempbuff, CFG_IS_NOT_STRINGVAL) == CFG_OK) &&
				  (cfg_get_key_value( "chunk_size", tempbuff, CFG_IS_NOT_STRINGVAL) == CFG_OK) );
    if(config_ok)
    {
	    /* BAB parameters have been defined OK so we log a more generic message using errno */
        reporterr(ERRFAC, M_DRVCTL_OPENFAIL, ERRCRIT, FTD_CTLDEV, strerror(error_code));
	}
	else 
    {
	    /* BAB parameters have not been set and driver load procedures have not been followed properly */
        reporterr (ERRFAC, M_BABNOTSET, ERRCRIT);
        reporterr (ERRFAC, M_BABNOTSET2, ERRCRIT);
    }
	return;
}
