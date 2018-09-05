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
#include <macros.h> 
#endif /* defined(_AIX) */
#include <stdio.h>
#include <fcntl.h> 
#include <string.h> 
#include <sys/param.h> 
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "cfg_intr.h"
#include "ftd_cmd.h"
#include "pathnames.h"

static char *getline (char **buffer, int *linelen);

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

    if ((buffer = (char*)malloc(2*statbuf.st_size)) == NULL) {
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
    temp = buffer;

    /* get lines until we find the right one */
    while ((line = getline(&temp, &len)) != NULL) {
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

    if ((fd = open(FTD_SYSTEM_CONFIG_FILE, O_RDWR)) == NULL) {
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
    while ((line = getline(&tempin, &linelen)) != NULL) {
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
getline (char **buffer, int *outlen)
{
    int  len;
    char *tempbuf;

/* XXX FIXME: I don't think that the outer while(1) is needed at all, since
 * there is no way for it to terminate or continue...  ifdef it out
 * to keep the compiler happy.  if getline breaks, then back out this change.
 * warner
 */
#ifdef NOT_NEEDED
    while (1) {
#endif
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
                break;
            } else if (tempbuf[len] == 0) {
                /* must be done! */
                *buffer = NULL;
                break;
            }
            len++;
        }
        if ((*outlen = len) == 0) {
            return NULL;
        }

        /* done */
        return tempbuf;
#ifdef NOT_NEEDED
    }
#endif
}
