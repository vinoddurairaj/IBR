/*
 * misc.c -- miscellaneous routines
 *
 * (c) Copyright 1999 Legato Systems, Inc. All Rights Reserved
 *
 */

#include <ctype.h>
#include <string.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*
 * stringcompare_addr --
 * compare two strings given the address of their address for qsort
 */
int
stringcompare_addr (void* p1, void* p2) {
	char *pp1 = *(char**)p1;
	char *pp2 = *(char**)p2;

    return (strcmp(pp1, pp2));
}

/*
 * stringcompare_const --
 * compare two strings for qsort
 */
int
stringcompare_const (const void* p1, const void* p2) {
    return (strcmp((char*)p1, (char*)p2));
}

/*
 * stringcompare --
 * compare two strings for general purpose use 
 */
int
stringcompare (void* p1, void* p2) {
    return (strcmp((char*)p1, (char*)p2));
}

/*
 * Read buffer until we hit an "end of pair" delimiter, break up key 
 * and value into two strings in place, and return the pointers to the 
 * key and value. The buffer
 * pointer is incremented to the next "line." The delimiter should be 
 * a reasonable non-white space character. This code will skip over 
 * comment lines and lines that are too short to hold valid data, 
 * which should never happen but we do it anyway.
 *
 * Returns FALSE, if parsing failed (end-of-buffer reached). 
 * Returns TRUE, if parsing succeeded. 
 */
int 
getbufline (char **buffer, char **key, char **value, char delim)
{
    int i, len;
    int blankflag;
    char *line, *tempbuf;
    int commentflag;

    *key = *value = NULL;

    /* BOGUS DELIMITER CHECK */
    if ((delim == 0) || (delim == ' ') || (delim == '\t')) {
        return FALSE;
    }

    while (1) {
        line = tempbuf = *buffer;
        if (tempbuf == NULL) {
            return FALSE;
        }

        /* search for the delimiter or a NULL */
        len = 0;
        while ((tempbuf[len] != delim) && (tempbuf[len] != 0)) {
            len++;
        }
        if (tempbuf[len] == delim) {
            tempbuf[len] = 0;
            *buffer = &tempbuf[len+1];
        } else {
            /* must be done! */
            *buffer = NULL;
        }
        if (len == 0) {
            return FALSE;
        }
  
        /* skip comment lines and lines that are too short to count */
        if (len < 5) continue;
        commentflag = 0;
        for (i=0; i<len; i++) {
            if (isgraph(line[i]) == 0) continue;
            if (line[i] == '#') {
                commentflag = 1;
            }
            break;
        }
        if (commentflag) continue;

        /* ignore blank lines */
        blankflag = 1;
        for (i = 0; i < len; i++) {
            if (isgraph(line[i])) {
                blankflag = 0;
                break;
            }
        }
        if (blankflag) continue;

        /* -- get rid of leading whitespace -- */
        i = 0;
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) continue;

        /* -- accumulate the key */
        *key = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
               (line[i] != '\n')) i++;
        line[i++] = 0;

        /* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;

        /* -- accumulate the value */
        *value = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
               (line[i] != '\n')) i++;
        line[i] = 0;

        /* done */
        return TRUE;
    }
    
	return TRUE;        /* Impossible, but keeps gcc happy */
}

/*
 * bub_all_zero --
 * compare data block against all zeros 
 */
int
buf_all_zero(char *buf, int buflen)
{
    int i;

    for (i = 0; i < buflen; i++) {
        if (buf[i]) {
            return 0;
        }
    }

    return 1;
} /* buf_all_zero */

