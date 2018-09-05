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
 * ps_intr.c - Persistent store interface
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
#endif/* !defined(linux) */
#endif /* defined(_AIX) */
#include <stdio.h>
#include <fcntl.h> 
#include <string.h> 
#include <errno.h> 
#include <ctype.h>
#include <sys/param.h> 
#include <sys/stat.h>
#include <sys/sysmacros.h>
#if defined(linux)
#define  min MIN
#define  max MAX
#include <sys/wait.h>
#include <linux/fs.h>
#endif /* defined(linux) */
#if defined(SOLARIS)
#include <sys/mkdev.h>
#endif

#include "ps_intr.h"
#include "ps_pvt.h"
#include "config.h"

#include "ftd_cmd.h"
#include "aixcmn.h"
#include "errors.h"
#include "ftdio.h"
#include "common.h"
#include "cfg_intr.h"
#include "ftdif.h"
#include "pathnames.h"
#include "config.h"
#include "platform.h"
#include "devcheck.h"
#include "aixcmn.h"

#if defined(PS_CACHE)
/*
 * -- attribute cache variables and functions
 */
static char ***ps_att_cache_key = (char ***)NULL;
static char ***ps_att_cache_val = (char ***)NULL;
static int ps_att_cache_numgrps = 0;
static int *ps_att_cache_numkeys = (int *)NULL;
static void ps_att_cache_destroy ();
static int ps_att_cache_get_value (int lg, char *key, char* val);
static int ps_att_cache_set_value (int lg, char *key, char* val);
#endif /* PS_CACHE */

/*
 * -- internal pstore functions 
 */
static int ps_parse_lg_from_group_name(char* grpnam);
static int   open_ps_get_dev_info(char *ps_name, char *dev_name, int *outfd, 
    ps_hdr_t *hdr, int *dev_index,
    ps_dev_entry_t **ret_table);
static int   open_ps_get_group_info(char *ps_name, char *group_name, 
    int *outfd, ps_hdr_t *hdr, 
    int *group_index, 
    ps_group_entry_t **ret_table);
#if defined(linux)
static int getline1 (char **buffer, char **key, char **value, char delim);
#else
static int getline (char **buffer, char **key, char **value, char delim);
#endif /*  defined(linux) */

static int  ps_check_if_old_Pstore_version_or_bad_header( ps_hdr_t *hdr, int log_message, char *ps_name );


#ifdef PS_DEBUG
static FILE* psdbgfd = (FILE*)NULL;

#define PSDBG(a, b, c)                           \
{                                                \
    if (psdbgfd == (FILE*)NULL) {                \
        psdbgfd = fopen("/tmp/psdbg.txt", "a+"); \
    }                                            \
    fprintf(psdbgfd, "%s: [%s : %s]\n",a, b, c); \
    fflush(psdbgfd);                             \
} 

#else 
#define PSDBG(a, b, c) /* do nothing */
#endif /* PS_DEBUG */

#if defined(PS_CACHE)
/*
 * ATTRIBUTE CACHE FUNCTIONS
 *
 */
/*
 * ps_att_cache_destroy -- free the cache's allocated memory 
 */
static void
ps_att_cache_destroy ()
{
  int g, i;

PSDBG("ps_att_cache_destroy", " ", " ")
  if (ps_att_cache_numgrps > 0) {
    for (g = 0; g < ps_att_cache_numgrps; g++) {
      if (ps_att_cache_numkeys[g] > 0) {
        for (i = 0; i < ps_att_cache_numkeys[g]; i++) {
          if (ps_att_cache_key[g][i] != (char *)NULL) {
  	    free((void *)ps_att_cache_key[g][i]);
	  }
	  ps_att_cache_key[g][i] = (char*)NULL; 
          if (ps_att_cache_val[g][i] != (char *)NULL) {
	    free((void *)ps_att_cache_val[g][i]);
          }
	  ps_att_cache_val[g][i] = (char*)NULL;
        }
      }
      if (ps_att_cache_key[g] != (char**)NULL) {
        free((void *)ps_att_cache_key[g]);
      }
      ps_att_cache_key[g] = (char**)NULL;
      if (ps_att_cache_val[g] != (char**)NULL) {
        free((void *)ps_att_cache_val[g]);
      }
      ps_att_cache_val[g] = (char**)NULL;
      ps_att_cache_numkeys[g] = 0;
    }
  }
  if (ps_att_cache_numkeys != (int*)NULL) {
    free((void *)ps_att_cache_numkeys);
  }
  ps_att_cache_numkeys = (int *)NULL;
  if (ps_att_cache_key != (char***)NULL) {
    free((void *)ps_att_cache_key);
  }
  ps_att_cache_key = (char***)NULL;
  if (ps_att_cache_val != (char***)NULL) {
    free((void *)ps_att_cache_val);
  }
  ps_att_cache_val = (char***)NULL;
  ps_att_cache_numgrps = 0;
}

/*
 * ps_att_cache_get_value - return value (and index) of an attribute key 
 */
static int ps_att_cache_get_value (int lg, char *key, char *val)
{
  int i;
  int idx;

PSDBG("ps_att_cache_get_value", key, " ")
  idx = -1;
  /* remove trailing blanks from the key */
  i = strlen(key) - 1;
  while ((i+1)) {
    if (isspace(key[i])) {
      key[i--] = '\0';
    } else {
      break;
    }
  }
  /* if the cache doesn't have the logical group in it, return a miss */
  if (ps_att_cache_numgrps > lg) {
    /* linear search the cache for the group for a key match */
    for (i = 0; i < ps_att_cache_numkeys[lg]; i++) {
      if (strcmp(key, ps_att_cache_key[lg][i]) == 0) {
	/* copy the attribute value from the cache */
	strcpy(val, ps_att_cache_val[lg][i]);
	idx = i;
	break;
      }
    }
  }
  return(idx);
}

/*
 * ps_att_cache_set_value - put a key and its value into the cache 
 */
static int ps_att_cache_set_value (int lg, char *key, char *val)
{
  int i;
  int idx;
  char tmpval[4096];

PSDBG("ps_att_cache_set_value", key, val)
  /* remove trailing blanks from the key */
  i = strlen(key) - 1;
  while ((i+1)) {
    if (isspace(key[i])) {
      key[i--] = '\0';
    } else {
      break;
    }
  }
  /* remove trailing blanks from the value */
  i = strlen(val) - 1;
  while ((i+1)) {
    if (isspace(val[i])) {
      val[i--] = '\0';
    } else {
      break;
    }
  }

  tmpval[0] = '\0';
  /* lookup the key and see if it exists already in the cache */
  idx = ps_att_cache_get_value(lg, key, tmpval);
  if (idx > -1) {
    /* attribute key already exists, replace the value */
    if (strlen(ps_att_cache_val[lg][idx]) != strlen(val)) {
      if ((ps_att_cache_val[lg][idx] = 
	     (char*) realloc((void*)ps_att_cache_val[lg][idx], 
                        (sizeof(char)*(1+strlen(val))))) == (char*)NULL) {
        return(PS_MALLOC_ERROR);
      }
    }
    strcpy(ps_att_cache_val[lg][idx], val);
  } else {
    /* cache miss - put the key/value pair in the cache for 1st time */
    if (ps_att_cache_numgrps <= lg) {
      if (ps_att_cache_numgrps == 0) {
	/* first time cache is being set */
	if ((ps_att_cache_numkeys = (int*) calloc((lg+1), sizeof(int))) 
	    == NULL) {
	  return(PS_MALLOC_ERROR);
	}
	if ((ps_att_cache_key = (char***) calloc((lg+1), sizeof(char**))) 
	    == NULL) {
	  return(PS_MALLOC_ERROR);
	}
	if ((ps_att_cache_val = (char***) calloc((lg+1), sizeof(char**)))
	    == NULL) {
	  return(PS_MALLOC_ERROR);
	}
      } else {
	/* extend the cache for the new logical group */
	if ((ps_att_cache_numkeys = 
	     (int*) realloc((void*)ps_att_cache_numkeys, 
                            (sizeof(int)*(lg+1)))) == NULL) {
	  return(PS_MALLOC_ERROR);
	}
	if ((ps_att_cache_key = (char***) realloc((void*)ps_att_cache_key,
		(sizeof(char**)*(lg+1)))) == NULL) {
		return(PS_MALLOC_ERROR);
	}
	if ((ps_att_cache_val = (char***) realloc((void*)ps_att_cache_val,
		(sizeof(char**)*(lg+1)))) == NULL) {
		return(PS_MALLOC_ERROR);
	}
        for (i = ps_att_cache_numgrps; i < (lg+1); i++) {
		ps_att_cache_numkeys[i] = 0;
		ps_att_cache_key[i] = (char**)NULL;
		ps_att_cache_val[i] = (char**)NULL;
        }
      }
      ps_att_cache_numgrps = lg + 1;
    }
    /* add space for the new key and value to the group's cache */
    if (ps_att_cache_numkeys[lg] == 0) {
	if ((ps_att_cache_key[lg] = (char**) calloc(1, sizeof(char*)))
            == NULL) {
             return(PS_MALLOC_ERROR);
         }
         if ((ps_att_cache_val[lg] = (char**) calloc(1, sizeof(char*)))
            == NULL) {
             return(PS_MALLOC_ERROR);
         }
    } else {
         if ((ps_att_cache_key[lg] = 
                 (char**) realloc((void*)ps_att_cache_key[lg], 
                                  ((ps_att_cache_numkeys[lg]+1)*sizeof(char*))))
              == NULL) {
             return(PS_MALLOC_ERROR);
         }
         if ((ps_att_cache_val[lg] = 
                 (char**) realloc((void*)ps_att_cache_val[lg], 
                                  ((ps_att_cache_numkeys[lg]+1)*sizeof(char*)))) 
              == NULL) {
             return(PS_MALLOC_ERROR);
         }
    }
    ps_att_cache_key[lg][ps_att_cache_numkeys[lg]] = (char*)NULL;
    ps_att_cache_val[lg][ps_att_cache_numkeys[lg]] = (char*)NULL;
    /* allocate space for key and value and copy into cache */
    if ((ps_att_cache_key[lg][ps_att_cache_numkeys[lg]] = 
        (char*) calloc((strlen(key)+1), sizeof(char))) == NULL) {
        return(PS_MALLOC_ERROR);
    }
    strcpy(ps_att_cache_key[lg][ps_att_cache_numkeys[lg]], key);
    if ((ps_att_cache_val[lg][ps_att_cache_numkeys[lg]] = 
        (char*) calloc((strlen(val)+1), sizeof(char))) == NULL) {
        return(PS_MALLOC_ERROR);
    }
    strcpy(ps_att_cache_val[lg][ps_att_cache_numkeys[lg]], val);
    ps_att_cache_numkeys[lg]++; 
  }
  return 0;
}
#endif /* PS_CACHE */ 

// Take a value and, if necessary, modify it to make it the closest power of 2 
// (up or down depending on round_up argument).
// NOTE: if round_up is TRUE and the value > 0x80000000, we cannot round up
//       and return a negative status.
static int	 enforce_power_of_2( unsigned int *value, int round_up )
{
  unsigned int mask = 0x80000000;
  int          status = 0;

  if( *value == 1 )
  {
    if( round_up )
        *value = 2;
    else
        *value = 0;
	return;
  }
  // Find the most significant bit set in the value and clear all the lower bits
  while( mask >= 0x00000002 )
  {
    if( *value & mask )
	{
	  // Found the most significant bit set
	  if( round_up )
	  {
	    // Check if some lower bit is set (not power of 2 then)
		if( *value & ~mask )
		{
		  if( *value >= 0x80000000 )
		  {
		    status = -1;
		  }
		  else
		  {
		    *value <<= 1;
		    mask <<= 1;
		  }
		}
	  }
	  *value &= mask;
	  break;
	}
	mask >>= 1;
  }
  return( status );
}

static int ps_parse_lg_from_group_name(char* grpnam)
{
  int len;
  int i, j;
  int lg;

PSDBG("ps_parse_lg_from_group_name", grpnam, " ")
  lg = -1;
  i = 0;
  len = strlen(grpnam);
  for (j=0; j<3; j++) {
    while (i<len && grpnam[i]!='/') {
      i++;
    }
    i++;
    if ((i+1)<len && grpnam[i] == 'l' && grpnam[i+1] == 'g') {
      i+=2;
      break;
    }
  }
  if ((i+1)<len && grpnam[i] == 'l' && grpnam[i+1] == 'g') {
    i+=2;
  }
  if ((i+1)>=len) {
    return(lg);
  }
  j=0;
  if (i<len && isdigit(grpnam[i])) {
    lg = 0;
  } else {
    return(lg);
  }
  while(i<len && isdigit(grpnam[i])) {
    lg = (lg * 10) + (grpnam[i] - '0');
    i++;
    j++;
    if (j>=10) {
      break;
    }
  }
  return(lg);
}  

/*
 * Get the descriptive Pstore error string from specified error code
 */
char *ps_get_pstore_error_string( int error_code )
{
    static char *msg_PS_OK                 = "No error reported";
    static char *msg_PS_BOGUS_PS_NAME      = "Invalid Pstore name";
    static char *msg_PS_BOGUS_GROUP_NAME   = "Invalid group identification";
    static char *msg_PS_BOGUS_DEVICE_NAME  = "Invalid device identification";
    static char *msg_PS_BOGUS_BUFFER_LEN   = "Invalid buffer length";
    static char *msg_PS_INVALID_PS_VERSION = "Invalid Pstore version for this Product release level";
    static char *msg_PS_NO_ROOM            = "Not enough space in Pstore";
    static char *msg_PS_BOGUS_HEADER       = "Invalid Pstore header";
    static char *msg_PS_SEEK_ERROR         = "Pstore seek error";
    static char *msg_PS_MALLOC_ERROR       = "Memory allocation error";
    static char *msg_PS_READ_ERROR         = "Pstore read error";
    static char *msg_PS_WRITE_ERROR        = "Pstore write error";
    static char *msg_PS_DEVICE_NOT_FOUND   = "Device not found in the Pstore";
    static char *msg_PS_GROUP_NOT_FOUND    = "Group not found in the Pstore";
    static char *msg_PS_BOGUS_PATH_LEN     = "Invalid group or device string length";
    static char *msg_PS_BOGUS_CONFIG_FILE  = "Invalid config file";
    static char *msg_PS_KEY_NOT_FOUND      = "Attribute keyword not found";
    static char *msg_PS_DEVICE_MAX         = "Maximum number of devices reached"; 
    static char *msg_PS_NO_HRDB_SPACE      = "HRDB space exhausted";
    static char *msg_PS_NO_PROP_HRDB       = "Not Proportional HRDB";
    static char *msg_PS_HRDB_TABLE_FULL    = "HRDB info table full";
    static char *msg_PS_DEV_EXPANSION_EXCEEDED = "Device expansion limit has been reached";
	static char *default_error_string      = "Unknow error code translation";

	switch( error_code )
	{
        case PS_OK:
		  return( msg_PS_OK );
		  break;
        case PS_BOGUS_PS_NAME:
		  return( msg_PS_BOGUS_PS_NAME );
		  break;
        case PS_BOGUS_GROUP_NAME:
		  return( msg_PS_BOGUS_GROUP_NAME );
		  break;
        case PS_BOGUS_DEVICE_NAME:
		  return( msg_PS_BOGUS_DEVICE_NAME );
		  break;
        case PS_BOGUS_BUFFER_LEN:
		  return( msg_PS_BOGUS_BUFFER_LEN );
		  break;
        case PS_INVALID_PS_VERSION:
		  return( msg_PS_INVALID_PS_VERSION );
		  break;
        case PS_NO_ROOM:
		  return( msg_PS_NO_ROOM );
		  break;
        case PS_BOGUS_HEADER:
		  return( msg_PS_BOGUS_HEADER );
		  break;
        case PS_SEEK_ERROR:
		  return( msg_PS_SEEK_ERROR );
		  break;
        case PS_MALLOC_ERROR:
		  return( msg_PS_MALLOC_ERROR );
		  break;
        case PS_READ_ERROR:
		  return( msg_PS_READ_ERROR );
		  break;
        case PS_WRITE_ERROR:
		  return( msg_PS_WRITE_ERROR );
		  break;
        case PS_DEVICE_NOT_FOUND:
		  return( msg_PS_DEVICE_NOT_FOUND );
		  break;
        case PS_GROUP_NOT_FOUND:
		  return( msg_PS_GROUP_NOT_FOUND );
		  break;
        case PS_BOGUS_PATH_LEN:
		  return( msg_PS_BOGUS_PATH_LEN );
		  break;
        case PS_BOGUS_CONFIG_FILE:
		  return( msg_PS_BOGUS_CONFIG_FILE );
		  break;
        case PS_KEY_NOT_FOUND:
		  return( msg_PS_KEY_NOT_FOUND );
		  break;
        case PS_DEVICE_MAX: 
		  return( msg_PS_DEVICE_MAX );
		  break;
        case PS_NO_HRDB_SPACE:
		  return( msg_PS_NO_HRDB_SPACE );
		  break;
        case PS_NO_PROP_HRDB:
		  return( msg_PS_NO_PROP_HRDB );
		  break;
        case PS_HRDB_TABLE_FULL:
		  return( msg_PS_HRDB_TABLE_FULL );
		  break;
        case PS_DEV_EXPANSION_EXCEEDED:
		  return( msg_PS_DEV_EXPANSION_EXCEEDED );
		  break;
	    default:
	  	  return( default_error_string );
		  break;
	}
}

/*
 * Get the descriptive tracking resolution string from resolution level
 */
char *ps_get_tracking_resolution_string( int level )
{
	static char *low_resolution_string = "LOW";
	static char *medium_resolution_string = "MEDIUM";
	static char *high_resolution_string = "HIGH";
	static char *default_resolution_string = "CHECK"; // Default in case of erroneous level

	switch( level )
	{
	  case PS_HIGH_TRACKING_RES:
	  	return( high_resolution_string );
		break;
	  case PS_MEDIUM_TRACKING_RES:
	  	return( medium_resolution_string );
		break;
	  case PS_LOW_TRACKING_RES:
	  	return( low_resolution_string );
		break;
	  default:
	  	return( default_resolution_string );
		break;
	}
}

/*
 * A common action of almost all public device functions. Open the 
 * persistent store, read the header, verify the header magic number, 
 * read the device table, and search the device table for a matching path. 
 * Returns device index, header info, open file descriptor, and device 
 * entry info, if no errors occur. 
 */
static int
open_ps_get_dev_info(char *ps_name, char *dev_name, int *outfd,
    ps_hdr_t *hdr, int *dev_index, ps_dev_entry_t **ret_table)
{
    int            i, fd, pathlen, ret;
    unsigned int   table_size;
    ps_dev_entry_t *table;

PSDBG("open_ps_get_dev_info", dev_name, " ")
    *dev_index = -1;
    *outfd = -1;

    /* get the device number */
    if ((pathlen = strlen(dev_name)) == 0) {
        return PS_BOGUS_DEVICE_NAME;
    }

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, hdr, sizeof(ps_hdr_t));
	if( (ret = ps_check_if_old_Pstore_version_or_bad_header(hdr, 1, ps_name)) != PS_OK )
	{
        close(fd);
	    return( ret );
	}

    table_size = sizeof(ps_dev_entry_t) * hdr->data.ver1.max_dev;
    if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (llseek(fd, hdr->data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, (caddr_t)table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    /* search for the device name */
    for (i = 0; i < hdr->data.ver1.max_dev; i++) {
        if ((table[i].pathlen == pathlen) && 
            (strncmp(table[i].path, dev_name, pathlen) == 0)){

            /* the only GOOD way out of here */
            *dev_index = i;
            *outfd = fd;
            if (ret_table != NULL) {
                *ret_table = table;
            } else {
                free(table);
            }
            return PS_OK;
        }
    }
    free(table);
    close(fd);

    return PS_DEVICE_NOT_FOUND;
}

/*
 * Get a device's HRDB information (Proportional HRDB only)
 * Returns the address of the device HRDB info table read from the Pstore
 * and the index to this device's HRDB info structure.
 * If the device is not found in the table, we return the index to the first available entry to the caller
 * if he wants to add the device.
 * If the table is full, an index of -1 is returned.
 */
static int open_ps_get_dev_HRDB_info(char *ps_name, char *dev_name, int *outfd,
                      ps_hdr_t *hdr, int *dev_index, ps_dev_entry_2_t **ret_table)
{
    int                    i, fd, pathlen;
    unsigned int           table_size;
    ps_dev_entry_2_t *table;
	int                    index_to_first_available_entry = -1;

PSDBG("open_ps_get_dev_HRDB_info", dev_name, " ")
    *dev_index = -1;
    *outfd = -1;

    /* Validate the device name */
    if( ((pathlen = strlen(dev_name)) == 0) || (strlen(dev_name) > MAXPATHLEN) ) {
        return PS_BOGUS_DEVICE_NAME;
    }

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, hdr, sizeof(ps_hdr_t));
    if( hdr->magic != PS_VERSION_1_MAGIC ) {
        close(fd);
        return PS_BOGUS_HEADER;
    }

    table_size = sizeof(ps_dev_entry_2_t) * hdr->data.ver1.max_dev;
    if ((table = (ps_dev_entry_2_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* Seek to the device HRDB info table */
    if (llseek(fd, hdr->data.ver1.dev_HRDB_info_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, (caddr_t)table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    /* Search for the device name */
    for (i = 0; i < hdr->data.ver1.max_dev; i++)
    {
        if( (table[i].pathlen == 0) && (index_to_first_available_entry == -1) )
        {
            // Remember index of first available entry in the table 
	        index_to_first_available_entry = i;
		}
        if ((table[i].pathlen == pathlen) && 
            (strncmp(table[i].path, dev_name, pathlen) == 0))
        {
		    // Found the device in the table
			// Return its index, the table address (if the caller provided a pointer addres)
			// and the Pstore handle (fd)
            *dev_index = i;
            *outfd = fd;
            if (ret_table != NULL)
            {
                *ret_table = table;
            }
            else
            {
                free(table);
            }
            return PS_OK;
        }
    }
	// We have not found the device in the table.
	// First: check if the table is full (no available entry found).
	// Second: if an entry is available, return the address of the table if a pointer
	// has been provided, in case the caller is adding this device to the Pstore, 
	// along with the index to the first available entry and the handle (fd) to the Pstore.
	if( index_to_first_available_entry == -1 )
	{
        free(table);
        close(fd);
		return(PS_HRDB_TABLE_FULL);
	}
    if (ret_table != NULL)
	{
        *ret_table = table;
	}
	else
	{
        free(table);
	}
    *outfd = fd;
    *dev_index = index_to_first_available_entry;
    return PS_DEVICE_NOT_FOUND;
}

/*
 * A common action of almost all public group functions. Open the 
 * persistent store, read the header, verify the header magic number, 
 * read the group table, and search the group table for a matching path. 
 * Returns group index, header info, and open file descriptor, 
 * if no errors occur. 
 */
static int
open_ps_get_group_info(char *ps_name, char *group_name, int *outfd,
    ps_hdr_t *hdr, int *group_index, 
    ps_group_entry_t **ret_table)
{
    int              i, fd, pathlen, ret;
    unsigned int     table_size;
    ps_group_entry_t *table;

PSDBG("open_ps_get_group_info", group_name, " ")
    *group_index = -1;
    *outfd = -1;

    /* Validate the group name */
    if ((pathlen = strlen(group_name)) == 0) {
        return PS_BOGUS_DEVICE_NAME;
    }

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, hdr, sizeof(ps_hdr_t));
	if( (ret = ps_check_if_old_Pstore_version_or_bad_header(hdr, 1, ps_name)) != PS_OK )
	{
        close(fd);
	    return( ret );
	}

    table_size = sizeof(ps_group_entry_t) * hdr->data.ver1.max_group;
    if ((table = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the group index array */
    if (llseek(fd, hdr->data.ver1.group_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, (caddr_t)table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    /* search for the group name */
    for (i = 0; i < hdr->data.ver1.max_group; i++) {
        if ((table[i].pathlen == pathlen) && 
            (strncmp(table[i].path, group_name, pathlen) == 0)) {

            /* the only GOOD way out of here */
            *group_index = i;
            *outfd = fd;
            if (ret_table != NULL) {
                *ret_table = table;
            } else {
                free(table);
            }
            return PS_OK;
        }
    }
    free(table);
    close(fd);

    return PS_GROUP_NOT_FOUND;
}

/*
 * Adjust a device's HRDB info according to parameters and also the main offset to the
 * next available HRDB.
 * NOTE: this function can be called only if the HRDB type is Proportional. In the legacy cases
 *       of Small or Large HRDB, the old behavior is preserved (i.e. no change to these values
 *       after adding a device in the driver).
*/
int
ps_adjust_device_info(char *ps_name, char *dev_name, 
                   unsigned int hrdb_size, unsigned int previous_hrdb_size, unsigned int HRDB_resolution_KBs_per_bit, 
                   unsigned int LRDB_res_sectors_per_bit, unsigned int dev_hrdb_offset_KBs, long long Pstore_size_KBs, int hrdb_type,
                   int undo_next_Pstore_hrdb_offset, unsigned long long num_sectors, unsigned long long limitsize_multiple,
                   unsigned int lrdb_numbits, unsigned int hrdb_numbits, unsigned int lrdb_res_shift_count)
{
    int            fd, ret;
    ps_hdr_t       header;

PSDBG("ps_adjust_device_info", ps_name, dev_name)

    // If Proportional HRDB, enter HRDB info in devices HRDB info table
    if( hrdb_type != FTD_HS_PROPORTIONAL )
	{
	    return( PS_NO_PROP_HRDB );
	}
    // Update this device's entry in second device table.
    ret = ps_set_device_hrdb_info(ps_name, dev_name, hrdb_size,
                 dev_hrdb_offset_KBs, HRDB_resolution_KBs_per_bit, LRDB_res_sectors_per_bit,
                 lrdb_res_shift_count, num_sectors, limitsize_multiple, 0);
    if( ret != PS_OK )
	{
        reporterr(ERRFAC, M_HRDB_INFO_ERR, ERRCRIT, dev_name, ret); 
	    return( ret );
	}
    // Now update the next available HRDB offset in the Pstore header.
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    if( read(fd, &header, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t) )
	{
      close(fd);
      return PS_READ_ERROR;
	}

    // Check if we must cancel a previous modification of the Pstore's next available HRDB offset
	// done for this same device whose hrdb_size has changed
    if( undo_next_Pstore_hrdb_offset )
	{
	    header.data.ver1.next_available_HRDB_offset = 
	           (header.data.ver1.next_available_HRDB_offset*1024 - previous_hrdb_size) / 1024;
	}
    // Verify that we are not exceeding the Pstore space
    if( (header.data.ver1.next_available_HRDB_offset*1024 + hrdb_size) >= (Pstore_size_KBs * 1024) )
	{
      close(fd);
      return PS_NO_HRDB_SPACE;
	}

	header.data.ver1.next_available_HRDB_offset = (header.data.ver1.next_available_HRDB_offset*1024 + hrdb_size) / 1024;
    // Make sure the next available hrdb space is aligned on a 1KB boundary passed this devices hrdb
    if( (hrdb_size % 1024) != 0 )
	{
	    header.data.ver1.next_available_HRDB_offset++;
	}

    /* Seek to the header location and update the info */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* Write the header */
    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return PS_WRITE_ERROR;
    }

    close(fd);

    // Store the number of LRDB and HRDB bits; this is in the other device info table
    ret = ps_adjust_lrdb_and_hrdb_numbits( ps_name, dev_name, lrdb_numbits, hrdb_numbits, 1 );
    return( ret );
}

/*
 * Verify that we will not exceed the expansion provision that was done for the specified device
*/
int
ps_verify_expansion_provision( char *ps_name, char *dev_name, unsigned long long new_dev_size, unsigned long long *limitsize_factor_from_pstore )
{
    int                ret;
    unsigned long long limitsize_multiple;
    unsigned long long orig_num_sectors;

    ret = ps_get_device_hrdb_info(ps_name, dev_name, NULL, NULL, NULL, 
              NULL, NULL, NULL, &limitsize_multiple, &orig_num_sectors);
    if( ret != PS_OK ) 
    {
        return( ret );
    }
	if( limitsize_factor_from_pstore != NULL )
	    *limitsize_factor_from_pstore = limitsize_multiple;

	if( new_dev_size > (orig_num_sectors * limitsize_multiple) )
	    return( PS_DEV_EXPANSION_EXCEEDED );
	else
	    return( PS_OK );
}

/*
 * Update a device's num_sectors in the Pstore
*/
int
ps_update_num_sectors( char *ps_name, char *dev_name, unsigned long long new_dev_size )
{
    int                    fd, ret, dev_index;
    ps_hdr_t               header;
    ps_dev_entry_2_t *dev_HRDB_table;

    // Check that this Pstore supports Proportional HRDB
    if( ps_Pstore_supports_Proportional_HRDB( ps_name ) <= 0 )
	{
	    return( PS_NO_PROP_HRDB );
	}

    /* Open the pstore and get the device's HRDB info. */
    ret = open_ps_get_dev_HRDB_info(ps_name, dev_name, &fd, &header, &dev_index, &dev_HRDB_table);
    if( ret != PS_OK ) 
    {
        return (ret);
    }

    // Set the new device size
	dev_HRDB_table[dev_index].num_sectors_64bits = new_dev_size;

    // Write the information to the Pstore
    /* Seek to the device HRDB info entry in the Pstore */
    if (llseek(fd, (header.data.ver1.dev_HRDB_info_table_offset * 1024)
                    + (dev_index * header.data.ver1.dev_HRDB_info_entry_size), SEEK_SET) == -1)
    {
        free(dev_HRDB_table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &(dev_HRDB_table[dev_index]), 
              header.data.ver1.dev_HRDB_info_entry_size) != header.data.ver1.dev_HRDB_info_entry_size)
    {
        free(dev_HRDB_table);
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* Free the table allocated in open_ps_get_dev_HRDB_info and close the Pstore */
    free(dev_HRDB_table);
    close(fd);

    return PS_OK;
}

/*
 * fill in the header values based upon the attributes
 * Note: this function is called at Pstore creation, so no need to worry about 
 * Pstore format backward compatibility.
 */
static int
init_ps_header(ps_hdr_t *hdr, ps_version_1_attr_t *attr, char *ps_name)
{
    char psinfostr[1024];
    unsigned long long tmpu64;
	unsigned int       hrdb_size;
	long long          pstore_size_KBs;

PSDBG("init_ps_header", " ", " ")
    hdr->magic = PS_VERSION_1_MAGIC;
    hdr->data.ver1.max_dev = attr->max_dev;
    hdr->data.ver1.max_group = attr->max_group;
    hdr->data.ver1.dev_attr_size = attr->dev_attr_size;
    hdr->data.ver1.group_attr_size = attr->group_attr_size;
    hdr->data.ver1.dev_table_entry_size = sizeof(ps_dev_entry_t);
    hdr->data.ver1.group_table_entry_size = sizeof(ps_group_entry_t);
	hdr->data.ver1.dev_HRDB_info_entry_size = sizeof(ps_dev_entry_2_t);

    /* indices that make buffer allocation easier */
    hdr->data.ver1.num_device = 0;
    hdr->data.ver1.num_group = 0;
    hdr->data.ver1.last_device = -1;
    hdr->data.ver1.last_group = -1;

/* FIXME: may want to make sure these are a power of 2! */
    hdr->data.ver1.lrdb_size = attr->lrdb_size;
    hdr->data.ver1.Small_or_Large_hrdb_size = attr->Small_or_Large_hrdb_size;
    hdr->data.ver1.hrdb_type = attr->hrdb_type;

    /* alignment on 1KB boundary, unit is in KB */
    hdr->data.ver1.dev_table_offset = PS_HEADER_OFFSET +
        ((sizeof(hdr) + 1023) / 1024);
    hdr->data.ver1.group_table_offset = hdr->data.ver1.dev_table_offset +
        (((attr->max_dev * sizeof(ps_dev_entry_t)) + 1023) / 1024);
    hdr->data.ver1.dev_attr_offset = hdr->data.ver1.group_table_offset +
        (((attr->max_group * sizeof(ps_group_entry_t)) + 1023) / 1024);
    hdr->data.ver1.group_attr_offset = hdr->data.ver1.dev_attr_offset +
        (((attr->max_dev * attr->dev_attr_size) + 1023) / 1024);

/* FIXME: make LRDB start on a cylinder boundary or power of 2, if possible */

    // The following is for determining the offset to the first HRDB bitmap
    switch( attr->hrdb_type )
	{
	  case FTD_HS_SMALL:
	    hrdb_size = FTD_PS_HRDB_SIZE_SMALL;
		break;
	  case FTD_HS_LARGE:
	    hrdb_size = FTD_PS_HRDB_SIZE_LARGE;
		break;
	  case FTD_HS_PROPORTIONAL:
	    // In the case of Proportional HRDB, we align the offset to the first HRDB
		// on the same boundary as that used for Small HRDB
	    hrdb_size = FTD_PS_HRDB_SIZE_SMALL;
		break;
	}
#if defined(_AIX) || defined(linux)
    /* WR 32049: LVM restriction in AIX 5.1 and prior.  IO can not across
     * track group.  Hence, IO size can not > 128KB and can not across 128KB
     * boundary.  Workaround: round-up and align LRDB array offset to 8KB 
     * boundary and HRDB array offset to 128KB.
     * unit is in 1KB ...
     */
    /* WR 37834: Linux MD raid devices need 4K alignment too.  */

    // The following calculates the offsets to the first LRDB, to the devices' HRDB info table
    // and to the first HRDB, aligned on a their respective size boundary.
	// Offset to LRDB area:
    tmpu64 = ((hdr->data.ver1.group_attr_offset * 1024 +
               attr->max_group * attr->group_attr_size + FTD_PS_LRDB_SIZE-1) 
		/ FTD_PS_LRDB_SIZE) * (FTD_PS_LRDB_SIZE/1024);
    hdr->data.ver1.lrdb_offset = (unsigned int) tmpu64;

    // Offset to devices' HRDB info table (Proportional HRDB mode):
    hdr->data.ver1.dev_HRDB_info_table_offset = ((hdr->data.ver1.lrdb_offset * 1024 +
               attr->max_dev * attr->lrdb_size + 1023) / 1024);

    // Offset to HRDB area:
    tmpu64 = ((hdr->data.ver1.dev_HRDB_info_table_offset * 1024 +
               attr->max_dev * attr->dev_HRDB_info_entry_size + hrdb_size-1) 
		       / hrdb_size) * (hrdb_size/1024);
    hdr->data.ver1.hrdb_offset = (unsigned int) tmpu64;
#else

	// Offset to LRDB area:
    hdr->data.ver1.lrdb_offset = hdr->data.ver1.group_attr_offset +
        (((attr->max_group * attr->group_attr_size) + 1023) / 1024);

    // Offset to devices' HRDB info table (Proportional HRDB mode)
    hdr->data.ver1.dev_HRDB_info_table_offset = ((hdr->data.ver1.lrdb_offset * 1024 +
               attr->max_dev * attr->lrdb_size + 1023) / 1024);

    // Offset to HRDB area:
    hdr->data.ver1.hrdb_offset = hdr->data.ver1.dev_HRDB_info_table_offset +
        (((attr->max_dev * attr->dev_HRDB_info_entry_size + 1023) / 1024));
#endif

    // Check if the Pstore is too small even for control information and LRDB space
	// (WR PROD9808)
	pstore_size_KBs = ps_get_pstore_size_KBs(ps_name);
	if( pstore_size_KBs <= (long long)(hdr->data.ver1.hrdb_offset) )
	{
        reporterr( ERRFAC, M_PSTOOSMALL, ERRCRIT, ps_name, pstore_size_KBs, hdr->data.ver1.hrdb_offset );
		return( PS_NO_ROOM );
	}

    // Calculate the offset to the last HRDB space block (in KB).
	// This information is actually unused. We keep it as is from legacy code,
	// implying from the above that in case of Proportional HRDB the calculation
	// will be the same as that for Small HRT.
    hdr->data.ver1.last_block = hdr->data.ver1.hrdb_offset +
            (((attr->max_dev * hrdb_size) + 1023) / 1024) - 1;

    hdr->data.ver1.tracking_resolution_level = attr->tracking_resolution_level;
    hdr->data.ver1.max_HRDB_size_KBs = attr->max_HRDB_size_KBs;
	hdr->data.ver1.next_available_HRDB_offset = hdr->data.ver1.hrdb_offset;


    sprintf(psinfostr,"(1) dev_attr_size=%d, group_attr_size=%d, lrdb_size=%d, hrdb_size=%d, dev_table_entry_size=%d, group_table_entry_size=%d, dev_HRDB_info_entry_size=%d\n",
        attr->dev_attr_size, attr->group_attr_size, attr->lrdb_size, hrdb_size,
        attr->dev_table_entry_size, attr->group_table_entry_size,
		attr->dev_HRDB_info_entry_size);

    PSDBG("init_ps_header", ":PSINFO:", psinfostr)
#ifdef TRACE_PSTORE_INFO
    reporterr(ERRFAC, M_PSINFO, ERRINFO, psinfostr);
#endif

    sprintf(psinfostr,"(2) max_dev=%d, max_group=%d, num_device=%d, num_group=%d, last_device=%d, last_group=%d\n",
        attr->max_dev, attr->max_group,
        attr->num_device, attr->num_group,
        attr->last_device, attr->last_group);
    PSDBG("init_ps_header", ":PSINFO:", psinfostr)
#ifdef TRACE_PSTORE_INFO
    reporterr(ERRFAC, M_PSINFO, ERRINFO, psinfostr);
#endif

    sprintf(psinfostr,"(3) dev_table_offset=0x%x, group_table_offset=0x%x, dev_attr_offset=0x%x, group_attr_offset=0x%x, lrdb_offset=0x%x, dev_HRDB_info_table_offset=0x%x, hrdb_offset=0x%x, last_block=0x%x\n",
	hdr->data.ver1.dev_table_offset,
	hdr->data.ver1.group_table_offset,
	hdr->data.ver1.dev_attr_offset,
	hdr->data.ver1.group_attr_offset,
	hdr->data.ver1.lrdb_offset,
	hdr->data.ver1.dev_HRDB_info_table_offset,
	hdr->data.ver1.hrdb_offset,
	hdr->data.ver1.last_block);

    PSDBG("init_ps_header", ":PSINFO:", psinfostr)
#ifdef TRACE_PSTORE_INFO
    reporterr(ERRFAC, M_PSINFO, ERRINFO, psinfostr);
#endif

// Information pertaining to Proportional HRDB
    if( attr->hrdb_type == FTD_HS_PROPORTIONAL )
    {
      sprintf(psinfostr,"(4) tracking_resolution_level=0x%x, max_HRDB_size_KBs=0x%x, next_available_HRDB_offset=0x%x\n",
      hdr->data.ver1.tracking_resolution_level,
	  hdr->data.ver1.max_HRDB_size_KBs,
  	  hdr->data.ver1.next_available_HRDB_offset);

      PSDBG("init_ps_header", ":PSINFO:", psinfostr)
#ifdef TRACE_PSTORE_INFO
      reporterr(ERRFAC, M_PSINFO, ERRINFO, psinfostr);
#endif
	}
    return( PS_OK );
}

/*
 * return the sector offset to the LRDB for a device
 */
int
ps_get_lrdb_offset(char *ps_name, char *dev_name, unsigned int *offset)
{
    int      fd, ret, dev_index;
    ps_hdr_t header;


PSDBG("ps_get_lrdb_offset", ps_name, dev_name)
    *offset = 0xffffffff;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* close the store */
    close(fd);

#if defined(BAD_MATH)
   /*-
    * header.data.ver1.lrdb_offset is an address in 1k units,
    * the computation below returns *offset as a byte address.
    * the routine claims that it returning a sector number...
    */
    *offset = (header.data.ver1.lrdb_offset * 1024) + 
              (dev_index * header.data.ver1.lrdb_size);
#else /* defined(BAD_MATH) */
   /*-
    * sector number please...
    */
    *offset = ((header.data.ver1.lrdb_offset * 1024) + 
              (dev_index * header.data.ver1.lrdb_size)) / DEV_BSIZE ;
#endif /* defined(BAD_MATH) */

    return PS_OK;
}

/*
 * get the device LRDB from the driver 
 */
int
ps_get_lrdb(char *ps_name, char *dev_name, char *buffer,
    int buf_len, unsigned int *num_bits)
{
    int            fd, ret, dev_index;
    ps_hdr_t       header;
    unsigned int   offset;
    ps_dev_entry_t *table;

PSDBG("ps_get_lrdb", ps_name, dev_name)
    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > header.data.ver1.lrdb_size) {
        free(table);
        close(fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.lrdb_offset * 1024) +
             (dev_index * header.data.ver1.lrdb_size);
    if (llseek(fd, offset, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, buffer, buf_len) != buf_len) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    *num_bits = table[dev_index].ps_valid_lrdb_bits;

    /* close the store */
    free(table);
    close(fd);

    return PS_OK;
}

/*
 * load the device LRDB into the driver 
 */
int
ps_set_lrdb(char *ps_name, char *dev_name, char *buffer, int buf_len)
{
    int          fd, ret, dev_index;
    ps_hdr_t     header;
    unsigned int offset;

PSDBG("ps_set_lrdb", ps_name, dev_name)
    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > header.data.ver1.lrdb_size) {
        close(fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.lrdb_offset * 1024) +
             (dev_index * header.data.ver1.lrdb_size);
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, buffer, buf_len) != buf_len) {
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    close(fd);

    return PS_OK;
}

/*
 * Get a device HRDB size, offset and resolution, from the PStore device HRDB info table 
 */
int
ps_get_device_hrdb_info(char *ps_name, char *dev_name, unsigned int *hrdb_size,
            unsigned int *hrdb_offset_in_KBs, unsigned int *lrdb_res_sectors_per_bit,
            unsigned int *hrdb_resolution_KBs_per_bit, unsigned int *lrdb_res_shift_count,
            unsigned long long *num_sectors, unsigned long long *limitsize_multiple, 
            unsigned long long *orig_num_sectors)
{
    int                    fd, ret, dev_index;
    ps_hdr_t               header;
    ps_dev_entry_2_t *dev_HRDB_table;

PSDBG("ps_get_device_hrdb_info", ps_name, dev_name)

    if( ps_Pstore_supports_Proportional_HRDB( ps_name ) <= 0 )
	{
	    return( PS_NO_PROP_HRDB );
	}

    /* open the store */
    dev_HRDB_table = (ps_dev_entry_2_t *)NULL;
    ret = open_ps_get_dev_HRDB_info(ps_name, dev_name, &fd, &header, &dev_index, &dev_HRDB_table);
    if (ret != PS_OK)
    {
	    if( dev_HRDB_table != NULL )
		{
		    free(dev_HRDB_table);
		}
        return (ret);
    }

    // Return the info of the device for which the pointers provided are not NULL.
	if( hrdb_size != NULL )
	    *hrdb_size =  dev_HRDB_table[dev_index].hrdb_size;
	if( hrdb_offset_in_KBs != NULL )
	    *hrdb_offset_in_KBs = dev_HRDB_table[dev_index].dev_HRDB_offset_in_KBs;
	if( lrdb_res_sectors_per_bit != NULL )
	    *lrdb_res_sectors_per_bit = dev_HRDB_table[dev_index].lrdb_res_sectors_per_bit;
	if( hrdb_resolution_KBs_per_bit != NULL )
	    *hrdb_resolution_KBs_per_bit = dev_HRDB_table[dev_index].hrdb_resolution_KBs_per_bit;
	if( lrdb_res_shift_count != NULL )
	    *lrdb_res_shift_count = dev_HRDB_table[dev_index].lrdb_res_shift_count;
	if( num_sectors != NULL )
	    *num_sectors =  dev_HRDB_table[dev_index].num_sectors_64bits;
	if( orig_num_sectors != NULL )
	    *orig_num_sectors =  dev_HRDB_table[dev_index].orig_num_sectors_64bits;
	if( limitsize_multiple != NULL )
	    *limitsize_multiple =  dev_HRDB_table[dev_index].limitsize_multiple;

    /* Free the table allocated in open_ps_get_dev_HRDB_info and close the Pstore */
    free(dev_HRDB_table);
    close(fd);

    return PS_OK;
}


/*
 * Set a device limitsize_multiple field in the PStore device HRDB info table 
 */
int
ps_set_device_limitsize_multiple(char *ps_name, char *dtc_dev_name, unsigned long long new_limitsize_multiple)
{
    int                    fd, ret, dev_index;
    ps_hdr_t               header;
    ps_dev_entry_2_t       *dev_HRDB_table;

    if( ps_Pstore_supports_Proportional_HRDB( ps_name ) <= 0 )
	{
	    return( PS_NO_PROP_HRDB );
	}

    /* Open the store and get the device's HRDB info */
    ret = open_ps_get_dev_HRDB_info(ps_name, dtc_dev_name, &fd, &header, &dev_index, &dev_HRDB_table);
    if( ret != PS_OK ) 
    {
        return (ret);
    }

    // Set the information for this device's limitsize_multiple
	dev_HRDB_table[dev_index].limitsize_multiple = new_limitsize_multiple;

    // Write the information to the Pstore
    /* Seek to the device HRDB info entry in the Pstore */
    if (llseek(fd, (header.data.ver1.dev_HRDB_info_table_offset * 1024)
                    + (dev_index * header.data.ver1.dev_HRDB_info_entry_size), SEEK_SET) == -1)
    {
        free(dev_HRDB_table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &(dev_HRDB_table[dev_index]), 
              header.data.ver1.dev_HRDB_info_entry_size) != header.data.ver1.dev_HRDB_info_entry_size)
    {
        free(dev_HRDB_table);
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* Free the table allocated in open_ps_get_dev_HRDB_info and close the Pstore */
    free(dev_HRDB_table);
    close(fd);

    return PS_OK;
}

/*
 * Set a device HRDB information in the PStore device HRDB info table 
 */
int
ps_set_device_hrdb_info(char *ps_name, char *dev_name, unsigned int hrdb_size,
                        unsigned int hrdb_offset_in_KBs, unsigned int HRDB_resolution_KBs_per_bit,
						unsigned int LRDB_res_sectors_per_bit, unsigned int lrdb_res_shift_count,
                        unsigned long long num_sectors, unsigned long long limitsize_multiple,
                        int only_resolution)
{
    int                    fd, ret, dev_index, table_size;
    ps_hdr_t               header;
    ps_dev_entry_2_t       *dev_HRDB_table;

PSDBG("ps_set_device_hrdb_info", ps_name, dev_name)

    if( ps_Pstore_supports_Proportional_HRDB( ps_name ) <= 0 )
	{
	    return( PS_NO_PROP_HRDB );
	}

    /* Open the store and get the device's HRDB info if the device is already in the table; if
       it is not, we get the index to the next available table entry. */
    ret = open_ps_get_dev_HRDB_info(ps_name, dev_name, &fd, &header, &dev_index, &dev_HRDB_table);
	// WARNING: do not modify ret; it is tested again further down in the code
    if( (ret != PS_OK) && (ret != PS_DEVICE_NOT_FOUND) ) 
    {
        return (ret);
    }

    // Set the information for this device's HRDB
	if( !only_resolution )
	{
		dev_HRDB_table[dev_index].hrdb_size = hrdb_size;
		dev_HRDB_table[dev_index].dev_HRDB_offset_in_KBs = hrdb_offset_in_KBs;
		dev_HRDB_table[dev_index].orig_num_sectors_64bits = num_sectors;
		dev_HRDB_table[dev_index].num_sectors_64bits = num_sectors;
		dev_HRDB_table[dev_index].limitsize_multiple = limitsize_multiple;
		// If this device gets deleted afterward, its HRDB space will become reusable
		dev_HRDB_table[dev_index].has_reusable_HRDB = 1;
	}

	dev_HRDB_table[dev_index].hrdb_resolution_KBs_per_bit = HRDB_resolution_KBs_per_bit;
	dev_HRDB_table[dev_index].lrdb_res_sectors_per_bit = LRDB_res_sectors_per_bit;
	dev_HRDB_table[dev_index].lrdb_res_shift_count = lrdb_res_shift_count;

	if( ret == PS_DEVICE_NOT_FOUND )
	{
	    dev_HRDB_table[dev_index].pathlen = strlen(dev_name);
		strncpy( dev_HRDB_table[dev_index].path, dev_name, MAXPATHLEN-1 ); 
	}
    // Write the information to the Pstore
    /* Seek to the device HRDB info entry in the Pstore */
    if (llseek(fd, (header.data.ver1.dev_HRDB_info_table_offset * 1024)
                    + (dev_index * header.data.ver1.dev_HRDB_info_entry_size), SEEK_SET) == -1)
    {
        free(dev_HRDB_table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &(dev_HRDB_table[dev_index]), 
              header.data.ver1.dev_HRDB_info_entry_size) != header.data.ver1.dev_HRDB_info_entry_size)
    {
        free(dev_HRDB_table);
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* Free the table allocated in open_ps_get_dev_HRDB_info and close the Pstore */
    free(dev_HRDB_table);
    close(fd);

    return PS_OK;
}

/*
 * Clear a device HRDB entry in the PStore device HRDB info table 
 */
int
ps_clear_device_hrdb_entry(char *ps_name, char *dev_name)
{
    int                    fd, ret, dev_index, table_size;
    ps_hdr_t               header;
    ps_dev_entry_2_t *dev_HRDB_table;

PSDBG("ps_clear_device_hrdb_entry", ps_name, dev_name)

    if( ps_Pstore_supports_Proportional_HRDB( ps_name ) <= 0 )
	{
	    return( PS_NO_PROP_HRDB );
	}

    /* Open the store and get the device's HRDB index. */
    dev_HRDB_table = (ps_dev_entry_2_t *)NULL;
    ret = open_ps_get_dev_HRDB_info(ps_name, dev_name, &fd, &header, &dev_index, &dev_HRDB_table);
    if (ret != PS_OK)
    {
	    if( dev_HRDB_table != NULL )
		{
		    free(dev_HRDB_table);
		}
        return (ret);
    }

	// This entry now has reusable HRDB space
	// <<< TODO: in a next phase we could reuse	these HRDB areas once the Pstore gets full.
	dev_HRDB_table[dev_index].has_reusable_HRDB = 1;
	memset( dev_HRDB_table[dev_index].path, 0, MAXPATHLEN ); // Clear device name
	dev_HRDB_table[dev_index].pathlen = 0;  // Clear name length

    // Write the information to the Pstore
    /* Seek to the device HRDB info entry in the Pstore */
    if (llseek(fd, (header.data.ver1.dev_HRDB_info_table_offset * 1024)
                    + (dev_index * sizeof(ps_dev_entry_2_t)), SEEK_SET) == -1)
    {
        free(dev_HRDB_table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &(dev_HRDB_table[dev_index]), sizeof(ps_dev_entry_2_t)) != sizeof(ps_dev_entry_2_t))
    {
        free(dev_HRDB_table);
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* Free the table allocated in open_ps_get_dev_HRDB_info and close the Pstore */
    free(dev_HRDB_table);
    close(fd);

    return PS_OK;
}

/*
 * Get the device HRDB from the Pstore 
 */
int
ps_get_hrdb(char *ps_name, char *dev_name, char *buffer, int buf_len, unsigned int *num_bits)
{
    int            fd, ret, dev_index, result;
    ps_hdr_t       header;
    ps_dev_entry_t *table;
	unsigned int   hrdb_size;
	unsigned int   hrdb_offset;

PSDBG("ps_get_hrdb", ps_name, dev_name)

    if( (result = ps_Pstore_supports_Proportional_HRDB(ps_name)) > 0 )
	{
        ret = ps_get_device_hrdb_info(ps_name, dev_name, &hrdb_size, &hrdb_offset, NULL, NULL, NULL, NULL, NULL, NULL);
        if (ret != PS_OK)
        {
            return (ret);
        }
		hrdb_offset *= 1024;  // Convert offset in bytes for the seek

        /* Now we need other device info for num_bits. */
        ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, &dev_index, &table);
        if (ret != PS_OK) {
            return (ret);
        }
	}
	else
	{   // Error occurred or invalid Pstore format
	    if( result == 0 ) result = PS_INVALID_PS_VERSION;
        reporterr(ERRFAC, M_PS_VERSION_ERR, ERRCRIT, ps_name, result);
        return (result);
	}
    /* make sure the buffer is not too big */
    if (buf_len > hrdb_size) {
        free(table);
        close(fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    if (llseek(fd, hrdb_offset, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, buffer, buf_len) != buf_len) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    *num_bits = table[dev_index].ps_valid_hrdb_bits;

    /* close the store */
    free(table);
    close(fd);

    return PS_OK;
}

/*
 * Write the device HRDB into the Pstore 
*/
int
ps_set_hrdb(char *ps_name, char *dev_name, char *buffer, int buf_len)
{
    int          fd, ret, dev_index, result;
    ps_hdr_t     header;
    unsigned int hrdb_offset;
	unsigned int hrdb_size;
    ps_dev_entry_2_t *dev_HRDB_table;

PSDBG("ps_set_hrdb", ps_name, dev_name)

    if( (result = ps_Pstore_supports_Proportional_HRDB(ps_name)) > 0 )
	{
        dev_HRDB_table = (ps_dev_entry_2_t *)NULL;
        ret = open_ps_get_dev_HRDB_info(ps_name, dev_name, &fd, &header, &dev_index, &dev_HRDB_table);
        if (ret != PS_OK)
        {
		    if( dev_HRDB_table != NULL )
			{
		        free(dev_HRDB_table);
			}
            return (ret);
        }
	    hrdb_size =  dev_HRDB_table[dev_index].hrdb_size;
	    hrdb_offset = dev_HRDB_table[dev_index].dev_HRDB_offset_in_KBs * 1024;
#ifdef DEBUG_PROP_HRDB  // TODO: delete all these once tests have progressed enough	<<<
	    sprintf( debug_msg, "<<< 1) hrdb_offset to seek to: %d >>>\n", hrdb_offset ); // <<< delete
        reporterr( ERRFAC, M_GENMSG, ERRINFO, debug_msg ); // <<< delete
#endif
		free(dev_HRDB_table);
	}
	else
	{   // Error occurred or invalid Pstore format
	    if( result == 0 ) result = PS_INVALID_PS_VERSION;
        reporterr(ERRFAC, M_PS_VERSION_ERR, ERRCRIT, ps_name, result);
        return (result);
	}

    /* make sure the buffer is not too big */
    if (buf_len > hrdb_size) {
        close(fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* Write the data */
    if (llseek(fd, hrdb_offset, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, buffer, buf_len) != buf_len) {
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    close(fd);
    return PS_OK;
}

/*
 * Set the attribute buffer for a device
 */
int
ps_set_device_attr(char *ps_name, char *dev_name, char *buffer, int buf_len)
{
    int          fd, ret, dev_index;
    ps_hdr_t     header;
    unsigned int offset;

PSDBG("ps_set_device_attr", ps_name, dev_name)
    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > header.data.ver1.dev_attr_size) {
        close(fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* Write the data */
    offset = (header.data.ver1.dev_attr_offset * 1024) +
             (dev_index * header.data.ver1.dev_attr_size);
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, buffer, buf_len) != buf_len) {
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    close(fd);

    return PS_OK;
}

/*
 * Get the attribute buffer and the number of sectors ecorded in the Pstore for a device
 */
int
ps_get_device_attr(char *ps_name, char *dev_name, char *buffer, int buf_len)
{
    int          fd, ret, dev_index;
    ps_hdr_t     header;
    unsigned int offset;

PSDBG("ps_get_device_attr", ps_name, dev_name)
    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, &dev_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > header.data.ver1.dev_attr_size) {
        buf_len = header.data.ver1.dev_attr_size;
    }

    /* read the data */
    offset = (header.data.ver1.dev_attr_offset * 1024) +
             (dev_index * header.data.ver1.dev_attr_size);
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, buffer, buf_len) != buf_len) {
        close(fd);
        return PS_READ_ERROR;
    }

    /* close the store */
    close(fd);

    return PS_OK;
}

/*
 * Set the shutdown state value
 */
int
ps_set_group_shutdown(char *ps_name, char *group_name, int value)
{
    int              fd, ret, index;
    ps_hdr_t         hdr;
    unsigned int     offset;
    ps_group_entry_t *table;

PSDBG("ps_set_group_shutdown", ps_name, group_name)
    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].shutdown = value;

    offset = (hdr.data.ver1.group_table_offset * 1024) +
             (index * sizeof(ps_group_entry_t));
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        free(table);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &table[index], sizeof(ps_group_entry_t)) != sizeof(ps_group_entry_t)) {
        close(fd);
        free(table);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    close(fd);
    free(table);

#if defined(PS_CACHE)
    /* this is probably a good time to clear the attribute cache */
    ps_att_cache_destroy();
#endif /* PS_CACHE */
    return PS_OK;
}

extern int _pmd_cpon;
/*
 * Set the checkpoint state value
 */
int
ps_set_group_checkpoint(char *ps_name, char *group_name, int value)
{
    int              fd, ret, index;
    ps_hdr_t         hdr;
    unsigned int     offset;
    ps_group_entry_t *table;

PSDBG("ps_set_group_checkpoint", ps_name, group_name)
    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].checkpoint = value;

    offset = (hdr.data.ver1.group_table_offset * 1024) +
             (index * sizeof(ps_group_entry_t));
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        free(table);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &table[index], sizeof(ps_group_entry_t)) != sizeof(ps_group_entry_t)) {
        close(fd);
        free(table);
        return PS_WRITE_ERROR;
    }

    _pmd_cpon = value;

    /* close the store */
    close(fd);
    free(table);

#if defined(PS_CACHE)
    /* this is probably a good time to clear the attribute cache */
    ps_att_cache_destroy();
#endif /* PS_CACHE */
    return PS_OK;
}

/*
 * Set the attribute buffer for the group
 */
int
ps_set_group_attr(char *ps_name, char *group_name, char *buffer, int buf_len)
{
    int          fd, ret, group_index;
    ps_hdr_t     header;
    unsigned int offset;

PSDBG("ps_set_group_attr", ps_name, group_name)
    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
        &group_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > header.data.ver1.group_attr_size) {
        close(fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.group_attr_offset * 1024) +
             (group_index * header.data.ver1.group_attr_size);
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, buffer, buf_len) != buf_len) {
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    close(fd);

    return PS_OK;
}

/*
 * set the state value for the group
 */
int
ps_set_group_state(char *ps_name, char *group_name, int state)
{
    int              fd, ret, index;
    ps_hdr_t         hdr;
    unsigned int     offset;
    ps_group_entry_t *table;

PSDBG("ps_set_group_state", ps_name, group_name)
    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].state = state;

    offset = (hdr.data.ver1.group_table_offset * 1024) +
             (index * sizeof(ps_group_entry_t));
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        free(table);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &table[index], sizeof(ps_group_entry_t)) != sizeof(ps_group_entry_t)) {
        close(fd);
        free(table);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    close(fd);
    free(table);

    return PS_OK;
}

#ifdef USE_PS_SET_DEVICE_STATE // This function is currently unused (see also PROD8693)
/*
 * set the state value for the device
 */
int
ps_set_device_state(char *ps_name, char *dev_name, int state)
{
    int            fd, ret, index;
    ps_hdr_t       hdr;
    unsigned int   offset;
    ps_dev_entry_t *table;

PSDBG("ps_set_device_state", ps_name, dev_name)
    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].state = state;

    offset = (hdr.data.ver1.dev_table_offset * 1024) +
             (index * sizeof(ps_dev_entry_t));
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        free(table);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &table[index], sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t)) {
        close(fd);
        free(table);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    close(fd);
    free(table);

    return PS_OK;
}
#endif

#ifdef USE_PS_SET_DEVICE_SIZE
/*
 * set the device size for the device
 * NOTE: this function is unused, and also the old field num_sectors being of 32 bits,
 *       it cannot satisfy TB devices.
 *       There is a new field now num_sectors_64bits in the new table for devices HRDB info;
 *       see function ps_set_device_hrdb_info()
 */
int
ps_set_device_size(char *ps_name, char *dev_name, unsigned int num_sectors)
{
    int            fd, ret, index;
    ps_hdr_t       hdr;
    unsigned int   offset;
    ps_dev_entry_t *table;

PSDBG("ps_set_device_size", ps_name, dev_name)
    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].num_sectors = num_sectors;

    offset = (hdr.data.ver1.dev_table_offset * 1024) +
             (index * sizeof(ps_dev_entry_t));
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        free(table);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &table[index], sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t)) {
        close(fd);
        free(table);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    close(fd);
    free(table);

    return PS_OK;
}
#endif

/*
 * Get the attribute value of a particular key for the group
 */
int
ps_get_group_key_value(char *ps_name, char *group_name, char *key, char *value)
{
    int          fd, ret, group_index, buf_len;
    char         *buffer, *temp, *tempval, *tempkey;
    ps_hdr_t     header;
    unsigned int offset;
    int lg;
    int idx;

PSDBG("ps_get_group_key_value", group_name, key)
    /* parse the logical group number from the group name */
    if (-1 == (lg = ps_parse_lg_from_group_name(group_name))) {
      return (PS_BOGUS_GROUP_NAME);
    }
#if defined(PS_CACHE)
    /* see if the key/value pair is in the cache; if so return value */
    if (-1 < (idx = ps_att_cache_get_value(lg, key, value))) {
      return (PS_OK);
    }
#endif /* PS_CACHE */
    
    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
        &group_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* allocate a buffer for the data */
    buf_len = header.data.ver1.group_attr_size;
    if ((buffer = (char *)malloc(buf_len)) == NULL) {
        close(fd);
        return (PS_MALLOC_ERROR);
    }

    /* read the data */
    offset = (header.data.ver1.group_attr_offset * 1024) +
             (group_index * header.data.ver1.group_attr_size);
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, buffer, buf_len) != buf_len) {
        close(fd);
        return PS_READ_ERROR;
    }

    /* close the store */
    close(fd);

    /* parse the buffer for a matching key */
/* FIXME: */
    /* parse the attributes into key/value pairs */
    temp = buffer;
#if defined(linux)
    while (getline1(&temp, &tempkey, &tempval, '\n')) {
#else
    while (getline(&temp, &tempkey, &tempval, '\n')) {
#endif /* defined(linux) */
        if (strcmp(key, tempkey) == 0) {
            strcpy(value, tempval);
            free(buffer);
#if defined(PS_CACHE)
	    ps_att_cache_set_value(lg, key, value);
#endif /* PS_CACHE */
            return PS_OK;
        }
    }

#ifdef TDMF_TRACE
    ps_dump_info(ps_name, group_name, buffer);
#endif

    free(buffer);

    return PS_KEY_NOT_FOUND;
}

/*
 * Set a key/value pair in the group buffer 
 * If value is NULL, we delete the key/value pair.
 */
int
ps_set_group_key_value(char *ps_name, char *group_name, char *key, char *value)
{
    int          fd, ret, group_index, buf_len, found, i;
    int          num_ps, len, linelen;
    char         *inbuf, *outbuf, *temp;
    char         line[MAXPATHLEN];
    char         *ps_key[FTD_MAX_KEY_VALUE_PAIRS];
    char         *ps_value[FTD_MAX_KEY_VALUE_PAIRS];
    ps_hdr_t     header;
    unsigned int offset;
    int lg, idx;

PSDBG("ps_set_group_key_value", key, value)
    /* parse the logical group number from the group name */
    if (-1 == (lg = ps_parse_lg_from_group_name(group_name))) {
      return (PS_BOGUS_GROUP_NAME);
    }
#if defined(PS_CACHE)
    /* store the key/value pair in the cache */
    if (0 != (idx = ps_att_cache_set_value(lg, key, value))) {
      return (idx);
    }
#endif /* PS_CACHE */

    /* open the store and get group information */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
        &group_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* allocate a buffer for the data */
    buf_len = header.data.ver1.group_attr_size;
    if ((inbuf = (char *)malloc(buf_len)) == NULL) {
        close(fd);
        return (PS_MALLOC_ERROR);
    }
    if ((outbuf = (char *)calloc(buf_len, 1)) == NULL) {
        close(fd);
        free(inbuf);
        return (PS_MALLOC_ERROR);
    }

    /* read the data */
    offset = (header.data.ver1.group_attr_offset * 1024) +
             (group_index * header.data.ver1.group_attr_size);
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, inbuf, buf_len) != buf_len) {
        close(fd);
        return PS_READ_ERROR;
    }

    /* parse the attributes into key/value pairs */
    temp = inbuf;
    num_ps = 0;
#if defined(linux)
    while (getline1(&temp, &ps_key[num_ps], &ps_value[num_ps], '\n')) {
#else
    while (getline(&temp, &ps_key[num_ps], &ps_value[num_ps], '\n')) {
#endif /* defined(linux) */
        num_ps++;
    }

    found = FALSE;
    for (i = 0; i < num_ps; i++) {
        if (strcmp(key, ps_key[i]) == 0) {
            /* replace value */
            ps_value[i] = value;
            found = TRUE;
            break;
        }
    }

    /* if we didn't find it, add it. */
    if (!found) {
        ps_key[num_ps] = key;
        ps_value[num_ps] = value;
        num_ps++;
    }

    /* create a new buffer */
    len = 0;
    for (i = 0; i < num_ps; i++) {
        /* we may be deleting this pair ... */
        if (ps_value[i] != NULL) {
            sprintf(line, "%s %s\n", ps_key[i], ps_value[i]);
            linelen = strlen(line);
            strncpy(&outbuf[len], line, linelen);
            len += linelen;
        }
    }

    /* write out the new buffer */
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        free(inbuf);
        free(outbuf);
        return PS_SEEK_ERROR;
    }
    if (write(fd, outbuf, buf_len) != buf_len) {
        close(fd);
        free(inbuf);
        free(outbuf);
        return PS_WRITE_ERROR;
    }

    close(fd);

    free(inbuf);
    free(outbuf);

    return PS_OK;
}

/*
 * Get the attribute buffer for the group
 */
int
ps_get_group_attr(char *ps_name, char *group_name, char *buffer, int buf_len)
{
    int          fd, ret, group_index;
    ps_hdr_t     header;
    unsigned int offset;

PSDBG("ps_get_group_attr", group_name, " ")
    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
        &group_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > header.data.ver1.group_attr_size) {
        buf_len = header.data.ver1.group_attr_size;
    }

    /* read the data */
    offset = (header.data.ver1.group_attr_offset * 1024) +
             (group_index * header.data.ver1.group_attr_size);
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, buffer, buf_len) != buf_len) {
        close(fd);
        return PS_READ_ERROR;
    }

    /* close the store */
    close(fd);

    return PS_OK;
}

/*
 * Get the size in KBs of a Pstore
   if error, return 0
 */
long long ps_get_pstore_size_KBs( char *ps_name )
{
  char raw_name[MAXPATHLEN];
  long long Pstore_size;
   
    /* convert the block device name to the raw device name */
#if defined(HPUX)
    if (is_logical_volume(ps_name))
    {
        convert_lv_name(raw_name, ps_name, 1);
    }
    else
    {
        force_dsk_or_rdsk(raw_name, ps_name, 1);
    }
#else
    force_dsk_or_rdsk(raw_name, ps_name, 1);
#endif

    if ((Pstore_size = disksize(raw_name)) <= 0)
    {
        reporterr(ERRFAC, M_PS_SIZE_ERR, ERRCRIT, raw_name);
        return 0;
    }

    return( (Pstore_size * DEV_BSIZE) / 1024 );
}

/*
 * add a group to the persistent store
 */
int
ps_add_group(char *ps_name, ps_group_info_t *info)
{
    int              i, fd, ret;
    ps_hdr_t         hdr;
    unsigned int     table_size, offset;
    ps_group_entry_t *table, entry;

PSDBG("ps_add_group", ps_name, " ")
    if (info == NULL) {
        return PS_BOGUS_DEVICE_NAME;
    }
    /* get the device number */
    if (((entry.pathlen = strlen(info->name)) == 0) ||
        (entry.pathlen >= MAXPATHLEN)) {
        return PS_BOGUS_PATH_LEN;
    }
    memset(entry.path, 0, sizeof(entry.path));
    strncpy(entry.path, info->name, entry.pathlen);
    entry.hostid = info->hostid;

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
	if( (ret = ps_check_if_old_Pstore_version_or_bad_header(&hdr, 1, ps_name)) != PS_OK )
	{
        close(fd);
	    return( ret );
	}

    table_size = sizeof(ps_group_entry_t) * hdr.data.ver1.max_group;
    if ((table = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the group index array */
    if (llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, (caddr_t)table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    /* if group already exists, blow it off */
    for (i = 0; i < hdr.data.ver1.max_group; i++) {
        if ((table[i].pathlen == entry.pathlen) &&
            (strncmp(table[i].path, entry.path, entry.pathlen) == 0)) {
            free(table);
            close(fd);
            return PS_OK;
        }
    }

    /* search for the next available space for a goup entry */
    for (i = 0; i < hdr.data.ver1.max_group; i++) {
        if (table[i].pathlen == 0) {
            free(table);
            offset = (hdr.data.ver1.group_table_offset * 1024) +
                     (i * sizeof(ps_group_entry_t));
            if (llseek(fd, offset, SEEK_SET) == -1) {
                close(fd);
                return PS_SEEK_ERROR;
            }
            if (write(fd, &entry, sizeof(ps_group_entry_t)) != sizeof(ps_group_entry_t)) {
                close(fd);
                return PS_WRITE_ERROR;
            }

            /* increment the number of groups */
            hdr.data.ver1.num_group++;
            if (i > hdr.data.ver1.last_group) {
                hdr.data.ver1.last_group = i;
            }

            /* rewrite the header */
            llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
            if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
                close(fd);
                return PS_WRITE_ERROR;
            }
            
            close(fd);
            return PS_OK;
        }
    }

    free(table);
    close(fd);

    return PS_NO_ROOM;
}

/*
 * delete a group from the persistent store
 */
int
ps_delete_group(char *ps_name, char *group_name, int group_number, int delete_devices_also)
{
    int              i, fd, ret, group_index;
    ps_hdr_t         header;
    unsigned int     offset;
    ps_group_entry_t entry, *table;
    int              table_size;
    ps_dev_entry_t   *dev_table;
	char device_substring[64];

PSDBG("ps_delete_group", ps_name, group_name)
    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
        &group_index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    /* overwrite the data */
    offset = (header.data.ver1.group_table_offset * 1024) +  
             (group_index * sizeof(ps_group_entry_t));
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }

    entry.pathlen = 0;
    memset(entry.path, 0, MAXPATHLEN);
    entry.hostid = 0xffffffff;

    if (write(fd, &entry, sizeof(ps_group_entry_t)) != sizeof(ps_group_entry_t)) {
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* if this is the last index, go find the new last index */
    if (group_index >= header.data.ver1.last_group) {
        for (i = group_index - 1; i >= 0; i--) {
            if (table[i].pathlen > 0) {
                break;
            }
        }
        header.data.ver1.last_group = i;
    }
    header.data.ver1.num_group--;

    /* rewrite the header */
    llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        free(table);
        close(fd);
        return PS_WRITE_ERROR;
    }

    free(table);

    // Check if we must now delete this group's devices;
    if( delete_devices_also )
	{
        table_size = sizeof(ps_dev_entry_t) * header.data.ver1.max_dev;
        if ((dev_table = (ps_dev_entry_t *)malloc(table_size)) == NULL)
        {
            reporterr(ERRFAC, M_DEL_DEV_ERR, ERRWARN, group_number, ps_get_pstore_error_string(PS_MALLOC_ERROR));
            close(fd);
            return PS_MALLOC_ERROR;
        }

        // Get the device table
        if (llseek(fd, header.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1)
        {
            reporterr(ERRFAC, M_DEL_DEV_ERR, ERRWARN, group_number, ps_get_pstore_error_string(PS_SEEK_ERROR));
            free(dev_table);
            close(fd);
            return PS_SEEK_ERROR;
        }
        if (read(fd, (caddr_t)dev_table, table_size) != table_size)
        {
            reporterr(ERRFAC, M_DEL_DEV_ERR, ERRWARN, group_number, ps_get_pstore_error_string(PS_READ_ERROR));
            free(dev_table);
            close(fd);
            return PS_READ_ERROR;
        }

        // Search for all the devices belonging to this group
	    sprintf( device_substring, "/dev/dtc/lg%d/rdsk/dtc", group_number );
        for (i = 0; i < header.data.ver1.max_dev; i++)
        {
	        if( strstr( dev_table[i].path, device_substring ) != NULL )
		    {
		        // Found a device belonging to this group; delete it
                if( (ret = ps_delete_device( ps_name, dev_table[i].path )) != PS_OK )
				{
				    // If error here, continue with other devices anyway
                    reporterr(ERRFAC, M_DEL_DEV_ERR, ERRWARN, group_number, ps_get_pstore_error_string(ret));
				}
			}
		}
        free(dev_table);
    }

    /* Close the Pstore */
    close(fd);

    return PS_OK;
}

/*
 * add a device to the persistent store
 */
int
ps_add_device(char *ps_name, ps_dev_info_t *info, tracking_resolution_info *HRDB_tracking_resolution,
              long long Pstore_size_KBs, int hrdb_type)
{
    int            i, fd, result;
    ps_hdr_t       hdr;
    unsigned int   table_size, offset;
    ps_dev_entry_t entry, *table;
    int            num_dev_present,cur_max_dev;
    unsigned int   bit_resolution_in_bytes_per_bit;
	unsigned int   save_bit_resolution_in_bytes_per_bit;
    u_longlong_t   fudged_device_size; 

PSDBG("ps_add_device", ps_name, " ")
    if (info == NULL) {
        return PS_BOGUS_DEVICE_NAME;
    }

    /* Before adding the device check for the maximum limit supported by p-store */
    num_dev_present = ps_get_num_device(ps_name, &cur_max_dev);

    if (num_dev_present < 0) // If negative, it is the error code
        return num_dev_present;
  
    if (num_dev_present == cur_max_dev)
    {
      reporterr(ERRFAC,M_PSMAX,ERRWARN);
      return PS_DEVICE_MAX;
    }

	// The following preserves the legacy RFX 2.7.0 device size fudge factor which
	// fakes having a device size a factor times greater than it really is. Here, we
	// apply it to Proportional HRDB. This was the way to limit dirty bits usage to a 
	// fraction of what is allocated so as to provide fo future device expansion.
	fudged_device_size = (info->num_sectors * DEV_BSIZE) * info->limitsize_multiple;

    switch( hrdb_type )
	{
	  case FTD_HS_SMALL:
	    info->hrdb_size = FTD_PS_HRDB_SIZE_SMALL;
		// NOTE: the following values of num_hrdb_bits and bit resolution may be changed and returned to us by the driver
		// at the FTD_NEW_DEVICE call when the device will be started (created in the driver). This is applicable
		// also to the FTD_HS_LARGE case. However, the code to do so is written but activated only for Proportional HRDB
		// mode, to avoid side effects at this stage of release 2.7.1 in the legacy modes. 
		// This is a possible optimization for the next release.
		// <<< TODO: see WR PROD10057.
		info->info_allocated_hrdb_bits = FTD_PS_HRDB_SIZE_SMALL * 8;
	    bit_resolution_in_bytes_per_bit = (fudged_device_size + (info->info_allocated_hrdb_bits-1)) / info->info_allocated_hrdb_bits;
		break;
	  case FTD_HS_LARGE:
	    info->hrdb_size = FTD_PS_HRDB_SIZE_LARGE;
		info->info_allocated_hrdb_bits = FTD_PS_HRDB_SIZE_LARGE * 8;
	    bit_resolution_in_bytes_per_bit = (fudged_device_size + (info->info_allocated_hrdb_bits-1)) / info->info_allocated_hrdb_bits;
		break;
	  case FTD_HS_PROPORTIONAL:
	    /* In the case of proportional HRDB size, most of the calculations for bitmap resolution and size is
		   done here in User Space. In the legacy design of Small and Large HRTs, the size of the bitmaps is fixed
		   but adjusted by the driver.
		   TODO: an improvement would be to do also the calculations for Small and Large HRTs in User Space (all
		         at the same place).
		   Algorithm for Proportional HRDB:
		   1) calculate the HRDB size based on the chosen Tracking resolution and the device size
		   2) verify that the HRDB size does not exceed the maximum HRDB size for the chosen Traking resolution level;
		      if does, set the HRDB size to the maximum, calculate the new resulting bit resolution;
		   3) Make sure the resulting tracking bit resolution is a power of 2; if not, round up to closest power of 2
		      and adjust the HRDB size to avoid waste of Pstore space
		   4) store in this device record the resulting HRDB size, number of bits, HRDB offset and the effective 
		      Tracking resolution
		*/
		// Proportional HRDB num of bits = (device size in bytes) / (HRDB bit resolution in bytes per bit)
	    bit_resolution_in_bytes_per_bit = HRDB_tracking_resolution->bit_resolution_KBs_per_bit * 1024;
	    info->info_allocated_hrdb_bits = (fudged_device_size + (bit_resolution_in_bytes_per_bit-1)) / bit_resolution_in_bytes_per_bit;
	    info->hrdb_size = info->info_allocated_hrdb_bits >> 3;  // == / 8
		if( (info->info_allocated_hrdb_bits % 8) != 0 )
		{
		    info->hrdb_size++;
		}
		// Check for maximum HRDB size
		if( info->hrdb_size > (HRDB_tracking_resolution->max_HRDB_size_KBs * 1024) )
		{
		   info->hrdb_size = HRDB_tracking_resolution->max_HRDB_size_KBs * 1024;
	       info->info_allocated_hrdb_bits = info->hrdb_size << 3;
		}
		// We previously checked that the HRDB size is a power of 2; tests with RFX 2.7.0 Large HRT demonstrated
		// that it does not have to be the case. We no longer do that then.

    	// Set final bit resolution
    	bit_resolution_in_bytes_per_bit = (fudged_device_size + (info->info_allocated_hrdb_bits-1)) / info->info_allocated_hrdb_bits;

    	// Round up to the nearest (higher) power of 2 if necessary, which can occur if we have changed the
    	// HRDB size because of exceeded maximum size; this power of 2 is required by the driver (we
    	// pass a shift count to the driver, not an actual number of bytes)
    	// NOTE: we do not check the status returned by enforce_power_of_2; if -1, it would mean that it cannot round up
    	//       because bit 31 is set in the original value and it would become 0, but this case should
    	//       not occur because a prevalidation is done upon reading the resolution config file
    	//       (and if it did, it would imply a resolution above 2 GB per tracking bit !)
    	save_bit_resolution_in_bytes_per_bit = bit_resolution_in_bytes_per_bit;
    	enforce_power_of_2( &bit_resolution_in_bytes_per_bit, 1 );

    	// Smallest resolution is 1 KB: note that this will be re-verified at the driver level to make
    	// sure we respect the physical disk sector size; if not, the driver will return the adjusted values to us
    	// at FTD_NEW_DEVICE call.
    	if( bit_resolution_in_bytes_per_bit < 1024 )
    	{
    	    bit_resolution_in_bytes_per_bit = 1024;
    	}

    	if( bit_resolution_in_bytes_per_bit != save_bit_resolution_in_bytes_per_bit )
    	{
    	    // Resolution has been rounded up; adjust hrdb size to avoid wasting Pstore space
    	    info->info_allocated_hrdb_bits = (fudged_device_size + (bit_resolution_in_bytes_per_bit-1)) / bit_resolution_in_bytes_per_bit;
    	    info->hrdb_size = info->info_allocated_hrdb_bits >> 3;  // == / 8
    		if( (info->info_allocated_hrdb_bits % 8) != 0 )
    		{
    		    info->hrdb_size++;
    		}
    	}
		break;
	}

    info->hrdb_resolution_KBs_per_bit = bit_resolution_in_bytes_per_bit / 1024;

    // We set the number of valid bits the same as the number of allocated bits but they may be changed
	// by the driver upon adding the device into it; then we will adjust also.
	info->info_valid_lrdb_bits = info->info_allocated_lrdb_bits;
	info->info_valid_hrdb_bits = info->info_allocated_hrdb_bits;

    if (((entry.pathlen = strlen(info->name)) == 0) ||
        (entry.pathlen >= MAXPATHLEN)) {
        return PS_BOGUS_PATH_LEN;
    }
    memset(entry.path, 0, sizeof(entry.path));
    strncpy(entry.path, info->name, entry.pathlen);
    entry.ps_allocated_lrdb_bits = info->info_allocated_lrdb_bits;
    entry.ps_allocated_hrdb_bits = info->info_allocated_hrdb_bits;
	// We store the number of allocated bits as valid bits upon registering the device to the Pstore
	// but these values may be adjusted afterward based on driver calculations (PROD10057)
    entry.ps_valid_lrdb_bits = info->info_allocated_lrdb_bits;
    entry.ps_valid_hrdb_bits = info->info_allocated_hrdb_bits;

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
	if( (result = ps_check_if_old_Pstore_version_or_bad_header(&hdr, 1, ps_name)) != PS_OK )
	{
        close(fd);
	    return( result );
	}

    if( hdr.magic == PS_VERSION_1_MAGIC )
    {
        // Verify that we are not exceeding the Pstore space
        if( (hdr.data.ver1.next_available_HRDB_offset*1024 + info->hrdb_size) >= (Pstore_size_KBs * 1024) )
	    {
          close(fd);
          return PS_NO_HRDB_SPACE;
	    }

        // Enter HRDB (and some LRDB info) info in devices second info table.
		// NOTE: some of the fields will be adjusted later on based on driver calculations
		// (such as lrdb_res_shift_count).
        info->dev_HRDB_offset_in_KBs = hdr.data.ver1.next_available_HRDB_offset;
        result = ps_set_device_hrdb_info(ps_name, info->name, info->hrdb_size,
                    info->dev_HRDB_offset_in_KBs, info->hrdb_resolution_KBs_per_bit,
                    info->lrdb_res_sectors_per_bit, info->lrdb_res_shift_count,
                    info->num_sectors, info->limitsize_multiple, 0);
		if( result != PS_OK )
		{
            reporterr(ERRFAC, M_HRDB_INFO_ERR, ERRCRIT, info->name, result); 
            close(fd);
		    return( result );
		}

	    hdr.data.ver1.next_available_HRDB_offset = (hdr.data.ver1.next_available_HRDB_offset*1024 + info->hrdb_size) / 1024;
        // Make sure the next available hrdb space is aligned on a 1KB boundary passed this devices hrdb
        if( (info->hrdb_size % 1024) != 0 )
	    {
	        hdr.data.ver1.next_available_HRDB_offset++;
	    }
	}

    // Write the device entry in the device table
    table_size = sizeof(ps_dev_entry_t) * hdr.data.ver1.max_dev;
    if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    /* if device already exists, blow it off */
    for (i = 0; i < hdr.data.ver1.max_dev; i++) {
        if ((table[i].pathlen == entry.pathlen) &&
            (strncmp(table[i].path, entry.path, entry.pathlen) == 0)) {
            free(table);
            close(fd);
            return PS_OK;
        }
    }

    /* Search for the first available entry */
	/* TODO <<<: if we are re-using an entry that was used by a deleted device,
	   it implies that there is now a hole in the HRDB space (if we use Proportional HRDBs).
	   We could mark the deleted devices in the device HRDB info table but keep the HRDB info to reuse the space. */
    for (i = 0; i < hdr.data.ver1.max_dev; i++) {
        if (table[i].pathlen == 0) {
            free(table);
            offset = (hdr.data.ver1.dev_table_offset * 1024) +
                (i * sizeof(ps_dev_entry_t));
            if (llseek(fd, offset, SEEK_SET) == -1) {
                close(fd);
                return PS_SEEK_ERROR;
            }
            if (write(fd, &entry, sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t)) {
                close(fd);
                return PS_WRITE_ERROR;
            }

            /* increment the number of devices */
            hdr.data.ver1.num_device++;
            if (i > hdr.data.ver1.last_device) {
                hdr.data.ver1.last_device = i;
            }

            /* rewrite the header */
            llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
            if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
                close(fd);
                return PS_WRITE_ERROR;
            }
            
            close(fd);
            return PS_OK;
        }
    }

    free(table);
    close(fd);

    return PS_NO_ROOM;
}

/*
 * delete a device from the persistent store
 */
int
ps_delete_device(char *ps_name, char *dev_name)
{
    int            i, fd, ret, dev_index;
    ps_hdr_t       header;
    unsigned int   offset;
    ps_dev_entry_t entry, *table;

    /* open the store */
PSDBG("ps_delete_device", ps_name, dev_name)
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    /* overwrite the data */
    offset = (header.data.ver1.dev_table_offset * 1024) +  
        (dev_index * sizeof(ps_dev_entry_t));
    if (llseek(fd, offset, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }

    // TODO <<<: add a field indicating that this device's HRDB is available
    //           and keep the HRDB info intact (size, num of bits, offset, etc.)
    entry.pathlen = 0;
    memset(entry.path, 0, MAXPATHLEN);
    entry.ps_allocated_lrdb_bits = 0xffffffff;
    entry.ps_allocated_hrdb_bits = 0xffffffff;
    entry.ps_valid_lrdb_bits = 0xffffffff;
    entry.ps_valid_hrdb_bits = 0xffffffff;

    if (write(fd, &entry, sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t)) {
        free(table);
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* if this is the last index, go find the new last index */
    if (dev_index >= header.data.ver1.last_device) {
        for (i = dev_index - 1; i >= 0; i--) {
            if (table[i].pathlen > 0) {
                break;
            }
        }
        header.data.ver1.last_device = i;
    }
    header.data.ver1.num_device--;

    /* rewrite the header */
    llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        free(table);
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    free(table);
    close(fd);

    if( (ret = ps_clear_device_hrdb_entry( ps_name, dev_name )) != PS_OK )
	{
	    // If error here, we still return an OK status, because the main device information
		// has been deleted successfully (main device entry), but we log a message.
        reporterr(ERRFAC, M_HRDB_CLEAR_ERR, ERRWARN, dev_name, ret); 
	}

    return PS_OK;
}

#define BS_SIZE 64

/*
 * create a Version 1 persistent store 
 */
int
ps_create_version_1(char *ps_name, ps_version_1_attr_t *attr)
{
    int              fd, i, num, ret;
    ps_hdr_t         hdr;
    struct stat      statbuf;
    unsigned int     table_size, buflen;
    unsigned char    *clearbuf;
    ps_dev_entry_t   *dtable;
    ps_group_entry_t *gtable;
	ps_dev_entry_2_t *HRDB_info_table;

    /* stat the device and make sure it is a slice */
PSDBG("ps_create_version_1", ps_name, " ")
    if (stat(ps_name, &statbuf) != 0) {
        return PS_BOGUS_PS_NAME;
    }
    if (!(S_ISBLK(statbuf.st_mode))) {
        return PS_BOGUS_PS_NAME;
    }

/* FIXME: check the size of the volume and make sure we can initialize it. */

    /* open the store */
    if ((fd = open(ps_name, O_RDWR|O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    ret = init_ps_header(&hdr, attr, ps_name);
	if( ret != PS_OK )
	    return( ret );

    /* go back to the beginning and write the header */
    if (llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET) == -1) {
        close(fd);
        return PS_SEEK_ERROR;
    }

    if (write(fd, &hdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t)) {
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* allocate a table for devices */
    table_size = attr->max_dev * sizeof(ps_dev_entry_t);
    if ((dtable = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* set all values to the bogus value */
    for (i = 0; i < attr->max_dev; i++) {
        dtable[i].pathlen = 0;
        memset(dtable[i].path, 0, MAXPATHLEN);
        dtable[i].ps_allocated_lrdb_bits = 0xffffffff;
        dtable[i].ps_allocated_hrdb_bits = 0xffffffff;
        dtable[i].ps_valid_lrdb_bits = 0xffffffff;
        dtable[i].ps_valid_hrdb_bits = 0xffffffff;
        /* FRF - initialize the ackoff value to zero */
        dtable[i].ackoff = 0;
    }
    
	/* write out the device index array */
    if (llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
        free(dtable);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, dtable, table_size) != table_size) {
        free(dtable);
        close(fd);
        return PS_WRITE_ERROR;
    }
    free(dtable);

    /* allocate a table for groups */
    table_size = attr->max_group * sizeof(ps_group_entry_t);
    if ((gtable = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* set all values to the bogus value */
    for (i = 0; i < attr->max_group; i++) {
        gtable[i].pathlen = 0;
        memset(gtable[i].path, 0, MAXPATHLEN);
        gtable[i].hostid = 0xffffffff;
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

    // WR PROD10081, initialize also the HRDB info table
    table_size = attr->max_dev * sizeof(ps_dev_entry_2_t);
    if ((HRDB_info_table = (ps_dev_entry_2_t *)malloc(table_size)) == NULL)
    {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* Clear all device name entries */
    for (i = 0; i < attr->max_dev; i++)
    {
        HRDB_info_table[i].pathlen = 0;
        memset(HRDB_info_table[i].path, 0, MAXPATHLEN);
	}

    /* Seek to the device HRDB info table */
    if (llseek(fd, hdr.data.ver1.dev_HRDB_info_table_offset * 1024, SEEK_SET) == -1)
    {
        free(HRDB_info_table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, HRDB_info_table, table_size) != table_size)
    {
        free(HRDB_info_table);
        close(fd);
        return PS_WRITE_ERROR;
    }
    free(HRDB_info_table);

    /* DONE! */
    close(fd);

    return PS_OK;
}

/*
 * A very simple function. Just write out a new header. Everything on the
 * disk is wasted by the create function.
 */
int
create_ps( char *ps_name, int64_t *max_dev, tracking_resolution_info *HRDB_tracking_resolution )
{
    unsigned long long  size, err;
    char                raw_name[MAXPATHLEN];
    u_longlong_t        dsize;
    ps_version_1_attr_t attr;
    int                 hrt_type = FTD_HS_NOT_SET, ctlfd;
	int                 HRDB_size;

#ifdef DEBUG_PROP_HRDB  // TODO: delete all these once tests have progressed enough	<<<
    sprintf( debug_msg, "<<< In create_ps, HRDB_tracking_resolution->level = %d <<<\n", HRDB_tracking_resolution->level ); // <<< delete
    reporterr(ERRFAC, M_GENMSG, ERRINFO, debug_msg);
#endif

    /* convert the block device name to the raw device name */
#if defined(HPUX)
    if (is_logical_volume(ps_name)) {
        convert_lv_name(raw_name, ps_name, 1);
/* FIXME: for logical volumes, should we make sure it has contiguous blocks? */
    } else {
        force_dsk_or_rdsk(raw_name, ps_name, 1);
    }
#else
    force_dsk_or_rdsk(raw_name, ps_name, 1);
#endif

    /* stat the pstore device and figure out the maximum number of devices */
    if ((dsize = disksize(raw_name)) < 0) {	// BUG: checking < 0 on unsigned items... TODO <<<
        reporterr(ERRFAC, M_STAT, ERRCRIT, raw_name, strerror(errno));
        return -1;
    }

#ifdef DEBUG_PROP_HRDB  // TODO: delete all these once tests have progressed enough	<<<
    sprintf( debug_msg, "<<< Pstore size = %lld blocks of %d bytes >>>\n", dsize, DEV_BSIZE ); // <<< delete
    reporterr(ERRFAC, M_GENMSG, ERRINFO, debug_msg); // <<< delete
#endif

/* FIXME: the 3*MAXPATHLEN fudge is not kosher, but it works. The fudge
 * takes into account the table entry sizes for devices and groups, but
 * the table entry sizes may change in the future. Fix this by asking
 * the pstore interface how much space each device uses and how much space
 * each group uses.
 */
    if ((ctlfd = open(FTD_CTLDEV, O_RDWR)) < 0) {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return -1;
    }

    // Get the HRDB type from the driver
    FTD_IOCTL_CALL(ctlfd, FTD_HRDB_TYPE, &hrt_type);
    close(ctlfd);

#ifdef DEBUG_PROP_HRDB  // TODO: delete all these once tests have progressed enough	<<<
    sprintf( debug_msg, "<<< hrt_type from driver = %d <<<\n", hrt_type ); // <<< delete
    reporterr(ERRFAC, M_GENMSG, ERRINFO, debug_msg);
#endif

	// Validate the pointer that we have been passed for HRDB tracking resolution
    if( (hrt_type == FTD_HS_PROPORTIONAL) && (HRDB_tracking_resolution == NULL) )
	{
        return -1;
	}

    /* The Pstore space used by one device (assuming one device per group).	*/
    if (hrt_type == FTD_HS_LARGE)
        size = FTD_PS_LRDB_SIZE + FTD_PS_HRDB_SIZE_LARGE + FTD_PS_DEVICE_ATTR_SIZE +
           FTD_PS_GROUP_ATTR_SIZE + 3*MAXPATHLEN;
    else if (hrt_type == FTD_HS_SMALL)
        size = FTD_PS_LRDB_SIZE + FTD_PS_HRDB_SIZE_SMALL + FTD_PS_DEVICE_ATTR_SIZE +
           FTD_PS_GROUP_ATTR_SIZE + 3*MAXPATHLEN;
    else if (hrt_type == FTD_HS_PROPORTIONAL)
	{
	  // NOTE: this calculation is meant to determine the maximum number of devices for
      //  this Pstore. In the case of Proportional HRDB, the HRDB size can vary from one device to another.
	  // In a first phase we will allocate for the fixed size records (Pstore header, device and group
	  // tables, device and group attributes, and LRDBs) enough space to support the absolute
	  // maximum number of devices (hard coded constant FTD_MAX_DEVICES). The actual number of supported devices
	  // will be determined when groups are actually added to the pstore by dtcstart.
	  // So we do not calculate the size needed by one device in this case.
	  // TODO ? In a second phase, we could try to come up with a more precise algorithm; for instance we could
	  // scan the group configuration files to determine the disk size of each device associated with the
	  // PStore we are working on, and then make provision for some more devices and get a max_dev from that.
	}

    /* Note: DEV_BSIZE is of different value on different OSes.
     * HPUX 1 KB, others 512 B
     */
    if (hrt_type != FTD_HS_PROPORTIONAL)
        size = size / DEV_BSIZE;

/* BOGUSITY: assume the pstore starts at 16K from the start of the device */

// NOTE: in the following, dsize (now the PStore size) becomes the number of devices this Pstore can support
//       this calculation applies only to Small and Large HRT; Proportional HRDB implies variable size HRDB
//       and sets the number of devices to the absolute supported maximum.
#if defined(_AIX) || defined(linux)
    /* WR 32409: align LRDB to 8KB and HRDB to 128KB.
     * change dsize accordingly.
     */
     if (hrt_type == FTD_HS_LARGE)
         dsize = (u_longlong_t)((dsize - 32 -
                (FTD_PS_LRDB_SIZE+FTD_PS_HRDB_SIZE_LARGE)/DEV_BSIZE ) / size);
     else if (hrt_type == FTD_HS_SMALL)
         dsize = (u_longlong_t)((dsize - 32 -
                (FTD_PS_LRDB_SIZE+FTD_PS_HRDB_SIZE_SMALL)/DEV_BSIZE ) / size);
     else if (hrt_type == FTD_HS_PROPORTIONAL)
	 {
         dsize = FTD_MAX_DEVICES;
	 }
#else
     if (hrt_type == FTD_HS_PROPORTIONAL)
	 {
         dsize = FTD_MAX_DEVICES;
	 }
	 else
	 {
	      dsize = (u_longlong_t)((dsize- 32) / size);
	 }
#endif

    if (dsize > FTD_MAX_DEVICES) {
        dsize = FTD_MAX_DEVICES;
    }

    *max_dev = (int64_t)dsize;


    /* create a new pstore with the defaults */
    attr.max_dev = (unsigned int)dsize;
    attr.max_group = (unsigned int)dsize;  // Making provision for worst case: 1 device per group
    attr.lrdb_size = FTD_PS_LRDB_SIZE;
    attr.hrdb_type = hrt_type;

    if (hrt_type == FTD_HS_LARGE)
	{
        attr.Small_or_Large_hrdb_size = FTD_PS_HRDB_SIZE_LARGE;
        attr.max_HRDB_size_KBs = FTD_MAXIMUM_HRDB_SIZE_LARGE / 1024;
	}
    else if (hrt_type == FTD_HS_SMALL)
	{
        attr.Small_or_Large_hrdb_size = FTD_PS_HRDB_SIZE_SMALL;
        attr.max_HRDB_size_KBs = FTD_MAXIMUM_HRDB_SIZE_SMALL / 1024;
	}

    attr.dev_table_entry_size = sizeof(ps_dev_entry_t);
    attr.group_table_entry_size = sizeof(ps_group_entry_t);
    attr.group_attr_size = FTD_PS_GROUP_ATTR_SIZE;
    attr.dev_attr_size = FTD_PS_DEVICE_ATTR_SIZE;
	attr.dev_HRDB_info_entry_size = sizeof(ps_dev_entry_2_t);

    if (hrt_type == FTD_HS_PROPORTIONAL)
	{
      attr.tracking_resolution_level = HRDB_tracking_resolution->level; // Applicable only to Proportional HRDB type (high, medium, low)
      attr.max_HRDB_size_KBs = HRDB_tracking_resolution->max_HRDB_size_KBs; // Maximum HRDB size associated with the Tracking resolution level
	}

    if ((err = ps_create_version_1(ps_name, &attr)) != PS_OK) {
        reporterr(ERRFAC, M_PSCREATE, ERRCRIT, ps_name, err);
        return -1;
    }

    return 0;
}


int ps_check_if_old_Pstore_version_or_bad_header( ps_hdr_t *hdr, int log_message, char *ps_name )
{
	char     *magic_label;
	int      wrong_pstore = 0;

    switch( hdr->magic )
    {
		case PS_VERSION_1_MAGIC:
		    return( PS_OK );	   // Pstore OK, return
			break;
		case PS_VERSION_1_MAGIC_OLD1:
		    magic_label = "PS_VERSION_1_MAGIC_OLD1";
			wrong_pstore = 1;
			break;
		case PS_VERSION_1_MAGIC_OLD2:
		    magic_label = "PS_VERSION_1_MAGIC_OLD2";
			wrong_pstore = 1;
			break;
		case PS_VERSION_1_MAGIC_OLD3:
		    magic_label = "PS_VERSION_1_MAGIC_OLD3";
			wrong_pstore = 1;
			break;
		case PS_VERSION_1_MAGIC_OLD4:
		    magic_label = "PS_VERSION_1_MAGIC_OLD4";
			wrong_pstore = 1;
			break;
		case PS_VERSION_1_MAGIC_RFX26X_RFX270:
		    magic_label = "PS_VERSION_1_MAGIC_RFX26X_RFX270";
			wrong_pstore = 1;
			break;
		case PS_VERSION_1_MAGIC_RFX271:
		    magic_label = "PS_VERSION_1_MAGIC_RFX271";
			wrong_pstore = 1;
			break;
	}
	if( wrong_pstore )
	{
	    if( log_message )
		{
	        reporterr( ERRFAC, M_OLD_PSTORE, ERRCRIT, ps_name, magic_label );
	        reporterr( ERRFAC, M_OLD_PSTORE2, ERRCRIT );
		}
	    return( PS_INVALID_PS_VERSION );
	}
	else
	{
	    // Here we don't log a message since this case may be valid when we need to create the Pstore
	    return( PS_BOGUS_HEADER );
	}
}


/*
 * get the attributes of a Version 1.0 persistent store
 */
int
ps_get_version_1_attr(char *ps_name, ps_version_1_attr_t *attr, int log_message)
{
    int      fd, ret;
    ps_hdr_t hdr;

PSDBG("ps_get_version_1_attr", ps_name, " ")
    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1)
    {
	    if( log_message )
		{
             reporterr( ERRFAC, M_PSOPEN, ERRCRIT, ps_name );
		}
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    close(fd);

#ifdef DEBUG_PROP_HRDB  // TODO: delete all these once tests have progressed enough	<<<
	sprintf( debug_msg, "<<< hdr.magic: 0x%x >>>\n", hdr.magic ); // <<< delete
    reporterr( ERRFAC, M_GENMSG, ERRINFO, debug_msg ); // <<< delete
#endif

    // For RFX 2.7.2, at this stage, we do not support old Pstores; if time permits, 
    // we will develop the necessary logic in dtcpsmigrate.
	if( (ret = ps_check_if_old_Pstore_version_or_bad_header(&hdr, log_message, ps_name)) != PS_OK )
	{
	    return( ret );
	}

    attr->max_dev = hdr.data.ver1.max_dev;
    attr->max_group = hdr.data.ver1.max_group;
    attr->dev_attr_size = hdr.data.ver1.dev_attr_size;
    attr->group_attr_size = hdr.data.ver1.group_attr_size;
    attr->lrdb_size = hdr.data.ver1.lrdb_size;
    attr->Small_or_Large_hrdb_size = hdr.data.ver1.Small_or_Large_hrdb_size;
    attr->dev_table_entry_size = hdr.data.ver1.dev_table_entry_size;
    attr->group_table_entry_size = hdr.data.ver1.group_table_entry_size;
    attr->num_device = hdr.data.ver1.num_device;
    attr->num_group = hdr.data.ver1.num_group;
    attr->last_device = hdr.data.ver1.last_device;
    attr->last_group = hdr.data.ver1.last_group;
	if( hdr.magic == PS_VERSION_1_MAGIC )
	{
        attr->hrdb_type = hdr.data.ver1.hrdb_type;
		if( hdr.data.ver1.hrdb_type == FTD_HS_PROPORTIONAL )
		{
            attr->tracking_resolution_level = hdr.data.ver1.tracking_resolution_level;
	        attr->max_HRDB_size_KBs = hdr.data.ver1.max_HRDB_size_KBs;
	        attr->next_available_HRDB_offset = hdr.data.ver1.next_available_HRDB_offset;
		}
	}
	else
	{
	    // Pre-RFX271 Pstore format. Return at least the hrdb_type.
		if( hdr.data.ver1.Small_or_Large_hrdb_size == FTD_PS_HRDB_SIZE_SMALL )
		{
		    attr->hrdb_type = FTD_HS_SMALL;
		}
		else
		{
		   	attr->hrdb_type = FTD_HS_LARGE;
		}
	}

    return PS_OK;
}

// Tell if a Pstore supports Proportional HRDB or if it is a pre-RFX271 Pstore
// Return: if yes, return the HRT type (all type values are greater than 0); 0 == no; negative value == error.
// <<< TODO: have a vector of the Pstore names which we already checked along with a boolean
// <<< that tells which Pstore format they have, to avoid reading the Pstore too often.
int ps_Pstore_supports_Proportional_HRDB(char *ps_name)
{
    int      fd, result;
    ps_hdr_t hdr;

    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    close(fd);

	if( (result = ps_check_if_old_Pstore_version_or_bad_header(&hdr, 1, ps_name)) != PS_OK )
	{
	    return( result );
	}

    return( hdr.data.ver1.hrdb_type );
}

/*
 * get the version of the persistent store
 */
int
ps_get_version(char *ps_name, int *version)
{
    int      fd, result;
    ps_hdr_t hdr;

PSDBG("ps_get_version", ps_name, " ")
    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    close(fd);
	if( (result = ps_check_if_old_Pstore_version_or_bad_header(&hdr, 1, ps_name)) != PS_OK )
	{
        *version = -1;
	    return( result );
	}

    *version = 1;
    return PS_OK;
}

/*
 * Fill the buffer with the groups in the persistent store
 */
int
ps_get_group_list(char *ps_name, char *buffer, int buf_len)
{
    int              fd, i, j, ret;
    ps_hdr_t         hdr;
    unsigned int     table_size;
    ps_group_entry_t *table;

PSDBG("ps_get_group_list", ps_name, " ")
    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
	if( (ret = ps_check_if_old_Pstore_version_or_bad_header(&hdr, 1, ps_name)) != PS_OK )
	{
	    return( ret );
	}

/* FIXME: change to num_group */
    if ((buf_len * MAXPATHLEN) < hdr.data.ver1.max_dev) {
        return PS_BOGUS_BUFFER_LEN;
    }

    table_size = sizeof(ps_group_entry_t) * hdr.data.ver1.max_group;
    if ((table = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the group index array */
    if (llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, (caddr_t)table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    j = 0;
    for (i = 0; i < hdr.data.ver1.max_group; i++) {
        if (table[i].pathlen != 0) {
            strncpy(&buffer[j*MAXPATHLEN], table[i].path, table[i].pathlen);
            j++;
        }
    }

    /* fill in the rest of the buffer with bogus ids */
    while (j < buf_len) {
        buffer[j*MAXPATHLEN] = 0;
        j++;
    }
    free(table);
    close(fd);

    return PS_OK;
}

/*
 * Get the number of devices present in the p-store
 */
int
ps_get_num_device(char *ps_name, int *cur_max_dev)
{
  int            fd, i, ret, j=0;
  ps_hdr_t       hdr;
  unsigned int   table_size;
  ps_dev_entry_t *table;

  // Set current max devices to 0 in case of Pstore not initialized or in case of error
  *cur_max_dev = 0;

  /* open the store */
  if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
       return PS_BOGUS_PS_NAME;
  }

   /* seek to the header location */
   llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

   /* read the header */
   read(fd, &hdr, sizeof(ps_hdr_t));
	if( (ret = ps_check_if_old_Pstore_version_or_bad_header(&hdr, 1, ps_name)) != PS_OK )
	{
        close(fd);
	    return( ret );
	}
 
   table_size = sizeof(ps_dev_entry_t) * hdr.data.ver1.max_dev;
   if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
   }

   *cur_max_dev = hdr.data.ver1.max_dev;

   /* get the device index array */ 
    if (llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
   }
    if (read(fd, (caddr_t)table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }
 
    for (i = 0; i < hdr.data.ver1.max_dev; i++) {
         if (table[i].pathlen != 0) {
             j++;
        }
    }

   free(table);
   close(fd);

   return j;

} 
   
/*
 * Fill the buffer with the devices in the persistent store
 */
int
ps_get_device_list(char *ps_name, char (*buffer)[MAXPATHLEN], int buf_len)
{
    int            fd, i, j, ret;
    ps_hdr_t       hdr;
    unsigned int   table_size;
    ps_dev_entry_t *table;

PSDBG("ps_get_device_list", ps_name, " ")
    /* open the store */
    memset( buffer, '\0', buf_len * MAXPATHLEN );
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
	if( (ret = ps_check_if_old_Pstore_version_or_bad_header(&hdr, 1, ps_name)) != PS_OK )
	{
        close(fd);
	    return( ret );
	}

    if ((buf_len * MAXPATHLEN) < hdr.data.ver1.max_dev) {  // TODO <<< should be a division ? BUG ?
        close(fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    table_size = sizeof(ps_dev_entry_t) * hdr.data.ver1.max_dev;
    if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, (caddr_t)table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    j = 0;
    for (i = 0; i < hdr.data.ver1.max_dev; i++) {
        if ((table[i].pathlen > 0) && (table[i].pathlen < MAXPATHLEN)) {
            strncpy((char *)&buffer[j][0], table[i].path, table[i].pathlen);
            j++;
        }
    }

    free(table);
    close(fd);

    return PS_OK;
}

/*
 *
 */
int
ps_get_device_index(char *ps_name, char *dev_name, int *index)
{
    int            fd, ret;
    ps_hdr_t       hdr;

    /* open the store */
PSDBG("ps_get_device_index", ps_name, dev_name)
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, index, NULL);
    if (ret == PS_OK) {
        close(fd);
    }
    return ret;
}


/*
 * Get the device info for a device
 */
int
ps_get_group_info(char *ps_name, char *group_name, ps_group_info_t *info)
{
    int              fd, ret, index;
    ps_hdr_t         hdr;
    ps_group_entry_t *table;

PSDBG("ps_get_group_info", ps_name, group_name)
    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }
    close(fd);

    if (info->name == NULL) {
        info->name = table[index].path;
    }
    info->hostid = table[index].hostid;
    info->state = table[index].state;
    info->shutdown = table[index].shutdown;
    info->checkpoint = table[index].checkpoint;

    free(table);

    return PS_OK;
}

/*
 * Get the device info for a device
 */
int
ps_get_device_info(char *ps_name, char *dev_name, ps_dev_info_t *info)
{
    int            fd, ret, index;
    ps_hdr_t       hdr;
    ps_dev_entry_t *table;

PSDBG("ps_get_device_info", ps_name, dev_name)

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }
    close(fd);

    strcpy(info->name, table[index].path);
    info->info_allocated_lrdb_bits = table[index].ps_allocated_lrdb_bits;
    info->info_allocated_hrdb_bits = table[index].ps_allocated_hrdb_bits;
    info->info_valid_lrdb_bits = table[index].ps_valid_lrdb_bits;
    info->info_valid_hrdb_bits = table[index].ps_valid_hrdb_bits;

    ret = ps_get_device_hrdb_info(ps_name, dev_name, &info->hrdb_size,
               &info->dev_HRDB_offset_in_KBs, &info->lrdb_res_sectors_per_bit,
               &info->hrdb_resolution_KBs_per_bit, &info->lrdb_res_shift_count,
               &info->num_sectors, &info->limitsize_multiple, &info->orig_num_sectors);
    free(table);

    return( ret );
}

/*
 * Gratuitously swiped from config.c. Read buffer until we hit an 
 * "end of pair" delimiter, break up key and value into two strings
 * in place, and return the pointers to the key and value. The buffer
 * pointer is incremented to the next "line." The delimiter should be 
 * a reasonable non-white space character. This code will skip over 
 * comment lines and lines that are too short to hold valid data, 
 * which should never happen but we do it anyway.
 *
 * Returns FALSE, if parsing failed (end-of-buffer reached). 
 * Returns TRUE, if parsing succeeded. 
 */
static int 
#if defined(linux)
getline1 (char **buffer, char **key, char **value, char delim)
#else
getline (char **buffer, char **key, char **value, char delim)
#endif /* defined(linux) */
{
    int i, len;
    int blankflag;
    char *line, *tempbuf;

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
        if ((len < 5) || (line[0] == '#')) continue;

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

#if STUPID /* requires a : at the end of each key */
        if (line[i-1] != ':') {
            return FALSE;
        }
#endif

        line[i++] = 0;

        /* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) {
            return FALSE;
        }

        /* -- accumulate the value */
        *value = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
               (line[i] != '\n')) i++;
        line[i] = 0;

        /* done */
        return TRUE;
    }
}


/* FRF - Function to get the ackoff value from the pstore for perticular device */
int ps_get_device_entry(char *ps_name, char *dev_name,u_longlong_t *ackoff)
{
    int            fd, ret, index;
    ps_hdr_t       hdr;
    unsigned int   offset;
    ps_dev_entry_t *table;

    PSDBG("ps_get_device_entry", ps_name, dev_name)
    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }
    *ackoff=table[index].ackoff;

    /* close the store */
    close(fd);
    free(table);
    return PS_OK;
}


/* FRF - Function to set the ackoff value in the pstore for a particular device */
int ps_set_device_entry(char *ps_name, char *dev_name,u_longlong_t ackoff)
{
    int            fd, ret, index;
    ps_hdr_t       hdr;
    unsigned int   offset;
    ps_dev_entry_t *table;

    PSDBG("ps_set_device_entry", ps_name, dev_name)
    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].ackoff = ackoff;

    offset = (hdr.data.ver1.dev_table_offset * 1024) +
             (index * sizeof(ps_dev_entry_t));
    if (llseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        free(table);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &table[index], sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t))
    {
        close(fd);
        free(table);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    close(fd);
    free(table);
    return PS_OK;
}


int
ps_dump_info(char *ps_name, char *group_name, char *inbuffer)
{
    int     ret;
    char    *buf;
    char    *p;
    char    *endline;

    p = inbuffer;

    while (*p) {
        endline = strchr(p, '\n');
        if (*p == '_') {
            while ((p != endline) && (*p != '\0')) {
                p++;
            }
            p++; 
        } else {
            while (p != endline) {
                putc(*p, stdout);
                p++;
            } 
            putc(*p, stdout);
            p++;
        }
    }
    fprintf(stdout, "\n");
    
    return 0;
}

/*
  Get Tracking Resolution information for specified level from configuration file for Proportional HRDB.
  Return: 0 if OK, else -1
  Expected file format:
  ###############
  # All numbers are in KB (resolutions are KBs per bit)
  LOW_RESOLUTION=64;
  MAX_HRDB_SIZE_LOW=2048;
  MEDIUM_RESOLUTION=32;
  MAX_HRDB_SIZE_MEDIUM=4096;
  HIGH_RESOLUTION=8;
  MAX_HRDB_SIZE_HIGH=8192;
  ###############
*/
int ps_get_tracking_resolution_info( tracking_resolution_info *tracking_res_info_ptr )
{

  char	input_buffer[80];
  char *tracking_resolution_keyword;
  char *max_HRDB_size_keyword;
  int  result = 0;
  unsigned int  res_KBs_per_bit_from_file;

#ifdef DEBUG_PROP_HRDB  // TODO: delete all these once tests have progressed enough	<<<
    sprintf( debug_msg, "<<< In ps_get_tracking_resolution_info, tracking_res_info_ptr->level = %d <<<\n", tracking_res_info_ptr->level ); // <<< delete
    reporterr(ERRFAC, M_GENMSG, ERRINFO, debug_msg);
#endif

  switch( tracking_res_info_ptr->level )
  {
    case PS_LOW_TRACKING_RES:
	  tracking_resolution_keyword = "LOW_RESOLUTION";
	  max_HRDB_size_keyword = "MAX_HRDB_SIZE_LOW";
	  break;
    case PS_MEDIUM_TRACKING_RES:
	  tracking_resolution_keyword = "MEDIUM_RESOLUTION";
	  max_HRDB_size_keyword = "MAX_HRDB_SIZE_MEDIUM";
	  break;
    case PS_HIGH_TRACKING_RES:
	  tracking_resolution_keyword = "HIGH_RESOLUTION";
	  max_HRDB_size_keyword = "MAX_HRDB_SIZE_HIGH";
	  break;
	default:
      reporterr( ERRFAC, M_BADRESCODE, ERRCRIT, tracking_res_info_ptr->level );
	  return( -1 );
	  break;
  }

  result = read_key_value_from_file(TRACKING_RESOLUTIONS_FILE, tracking_resolution_keyword, 
                                    input_buffer, sizeof(input_buffer), CFG_IS_NOT_STRINGVAL);
  if( result ==  KEY_OK )
  {
    // Got tracking resolution value
    res_KBs_per_bit_from_file = tracking_res_info_ptr->bit_resolution_KBs_per_bit = atoi(input_buffer);

	// Verify that the value is a power of 2 (driver requirement) and that it does not exceed the highest
	// unsigned int power of 2 (otherwise a value round up to closest power of 2 would fail)
	if( res_KBs_per_bit_from_file > (unsigned int)0x80000000 )
	{
        reporterr( ERRFAC, M_RES_NOT_POWER2, ERRINFO, 
                   (unsigned long long)res_KBs_per_bit_from_file, (unsigned long long)0x80000000 );
	    res_KBs_per_bit_from_file = tracking_res_info_ptr->bit_resolution_KBs_per_bit = (unsigned int)0x80000000;
	}
	else
	{
  	    enforce_power_of_2( &(tracking_res_info_ptr->bit_resolution_KBs_per_bit), 1 );
    	if( res_KBs_per_bit_from_file != tracking_res_info_ptr->bit_resolution_KBs_per_bit )
    	{
  	      // The value was not a power of 2 and has been rounded up to the closest upward power of 2; log message
          reporterr( ERRFAC, M_RES_NOT_POWER2, ERRINFO, (unsigned long long)res_KBs_per_bit_from_file,
                     (unsigned long long)(tracking_res_info_ptr->bit_resolution_KBs_per_bit) );
  	    }
	}

	// Get max HRDB size for this resolution level
    result = read_key_value_from_file(TRACKING_RESOLUTIONS_FILE, max_HRDB_size_keyword,
                                      input_buffer, sizeof(input_buffer), CFG_IS_NOT_STRINGVAL);
    if( result ==  KEY_OK )
    {
      tracking_res_info_ptr->max_HRDB_size_KBs = atoi(input_buffer);
	}
  }

  if( result !=  KEY_OK )
  {
    if( result == KEY_FILE_NOT_FOUND )
    {
      // If the Tracking Resolution config file does not exist, set default info
	  result = 0;
      reporterr( ERRFAC, M_NORESFILE, ERRINFO, TRACKING_RESOLUTIONS_FILE );

      switch( tracking_res_info_ptr->level )
      {
        case PS_LOW_TRACKING_RES:
     	  tracking_res_info_ptr->bit_resolution_KBs_per_bit = PS_LOW_RES_GRANULARITY;
	      tracking_res_info_ptr->max_HRDB_size_KBs = PS_LOW_MAX_HRDB_SIZE;
	      break;
        case PS_MEDIUM_TRACKING_RES:
	      tracking_res_info_ptr->bit_resolution_KBs_per_bit = PS_MEDIUM_RES_GRANULARITY;
	      tracking_res_info_ptr->max_HRDB_size_KBs = PS_MEDIUM_MAX_HRDB_SIZE;
	      break;
        case PS_HIGH_TRACKING_RES:
	      tracking_res_info_ptr->bit_resolution_KBs_per_bit = PS_HIGH_RES_GRANULARITY;
	      tracking_res_info_ptr->max_HRDB_size_KBs = PS_HIGH_MAX_HRDB_SIZE;
	      break;
      }
    }  // of result == KEY_FILE_NOT_FOUND
	else
	{
	  // File exists but failed getting the information; return error status
      reporterr( ERRFAC, M_GETRESERROR, ERRCRIT, TRACKING_RESOLUTIONS_FILE, result );
	  result = -1;
	}
  } // result == KEY_OK
  else
  {
      result = 0;
  }
  return( result );
}

// Update the number of LRDB and HRDB bits for a specific device
// NOTE: we do not change the previous existing fields if the parameter also_allocated_bits is false,
// to avoid breaking functionality of modules such as ps_get_hrdb().
// The valid number of bits is saved in new fields (PROD10057) and in the old fields if also_allocated_bits is true.
int ps_adjust_lrdb_and_hrdb_numbits( char *ps_name, char *raw, unsigned int lrdb_numbits, unsigned int hrdb_numbits, int also_allocated_bits )
{
    int            i, fd, result;
    ps_hdr_t       hdr;
    unsigned int   table_size, offset;
    ps_dev_entry_t entry, *table;

    /* open the Pstore */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    if( hdr.magic != PS_VERSION_1_MAGIC ) {
        close(fd);
        return PS_INVALID_PS_VERSION;
    }

    table_size = sizeof(ps_dev_entry_t) * hdr.data.ver1.max_dev;
    if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    /* Search for the device entry */
    for (i = 0; i < hdr.data.ver1.max_dev; i++)
    {
        if ((table[i].pathlen == strlen(raw)) &&
            (strncmp(table[i].path, raw, strlen(raw)) == 0))
        {
            break;
        }
    }
	if( i == hdr.data.ver1.max_dev )
	{
        free(table);
        close(fd);
        return PS_DEVICE_NOT_FOUND;
	}

    table[i].ps_valid_lrdb_bits = lrdb_numbits;
    table[i].ps_valid_hrdb_bits = hrdb_numbits;
	// If specified, adjust also the fields giving the number of Pstore total allocated bits
	// which may have been changed in Proportional mode after first registration of the device
	// in the Pstore, due to driver calculations.
    if( also_allocated_bits )
	{
	    table[i].ps_allocated_lrdb_bits = lrdb_numbits;
	    table[i].ps_allocated_hrdb_bits = hrdb_numbits;
	}
    offset = (hdr.data.ver1.dev_table_offset * 1024) + (i * sizeof(ps_dev_entry_t));
    if (llseek(fd, offset, SEEK_SET) == -1)
    {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (write(fd, &(table[i]), sizeof(ps_dev_entry_t)) != sizeof(ps_dev_entry_t))
    {
        free(table);
        close(fd);
        return PS_WRITE_ERROR;
    }

    free(table);
    close(fd);
    return PS_OK;
}

// To get the base offset of the global HRDB area of a Pstore
int  ps_get_hrdb_base_offset( char *ps_name, unsigned int *hrdb_base_offset )
{
    int            fd, ret;
    ps_hdr_t       hdr;

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1)
    {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    close(fd);

	if( (ret = ps_check_if_old_Pstore_version_or_bad_header(&hdr, 1, ps_name)) != PS_OK )
	{
	    return( ret );
	}

    *hrdb_base_offset = hdr.data.ver1.hrdb_offset;

	return PS_OK;
}

