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
#include <macros.h> 
#endif /* defined(_AIX) */
#include <stdio.h>
#include <fcntl.h> 
#include <string.h> 
#include <errno.h> 
#include <ctype.h>
#include <sys/param.h> 
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "ps_intr.h"
#include "ps_pvt.h"

#include "ftd_cmd.h"
#include "aixcmn.h"

static int   open_ps_get_dev_info(char *ps_name, char *dev_name, int *outfd, 
    ps_hdr_t *hdr, int *dev_index,
    ps_dev_entry_t **ret_table);
static int   open_ps_get_group_info(char *ps_name, char *group_name, 
    int *outfd, ps_hdr_t *hdr, 
    int *group_index, 
    ps_group_entry_t **ret_table);
static int getline (char **buffer, char **key, char **value, char delim);

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
    int            i, fd, pathlen;
    unsigned int   table_size;
    ps_dev_entry_t *table;

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
    lseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, hdr, sizeof(ps_hdr_t));
    if (hdr->magic != PS_VERSION_1_MAGIC) {
        close(fd);
        return PS_BOGUS_HEADER;
    }

    table_size = sizeof(ps_dev_entry_t) * hdr->data.ver1.max_dev;
    if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (lseek(fd, hdr->data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, (caddr_t)table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    /* search for the device number */
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
    int              i, fd, pathlen;
    unsigned int     table_size;
    ps_group_entry_t *table;

    *group_index = -1;
    *outfd = -1;

    /* get the device number */
    if ((pathlen = strlen(group_name)) == 0) {
        return PS_BOGUS_DEVICE_NAME;
    }

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    lseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, hdr, sizeof(ps_hdr_t));
    if (hdr->magic != PS_VERSION_1_MAGIC) {
        close(fd);
        return PS_BOGUS_HEADER;
    }

    table_size = sizeof(ps_group_entry_t) * hdr->data.ver1.max_group;
    if ((table = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (lseek(fd, hdr->data.ver1.group_table_offset * 1024, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, (caddr_t)table, table_size) != table_size) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    /* search for the device number */
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
 * fill in the header values based upon the attributes
 */
static void
init_ps_header(ps_hdr_t *hdr, ps_version_1_attr_t *attr)
{

    hdr->magic = PS_VERSION_1_MAGIC;
    hdr->data.ver1.max_dev = attr->max_dev;
    hdr->data.ver1.max_group = attr->max_group;
    hdr->data.ver1.dev_attr_size = attr->dev_attr_size;
    hdr->data.ver1.group_attr_size = attr->group_attr_size;
    hdr->data.ver1.dev_table_entry_size = sizeof(ps_dev_entry_t);
    hdr->data.ver1.group_table_entry_size = sizeof(ps_group_entry_t);

    /* indices that make buffer allocation easier */
    hdr->data.ver1.num_device = 0;
    hdr->data.ver1.num_group = 0;
    hdr->data.ver1.last_device = -1;
    hdr->data.ver1.last_group = -1;

/* FIXME: may want to make sure these are a power of 2! */
    hdr->data.ver1.lrdb_size = attr->lrdb_size;
    hdr->data.ver1.hrdb_size = attr->hrdb_size;

    hdr->data.ver1.dev_table_offset = PS_HEADER_OFFSET +
        ((sizeof(hdr) + 1023) / 1024);
    hdr->data.ver1.group_table_offset = hdr->data.ver1.dev_table_offset +
        (((attr->max_dev * sizeof(ps_dev_entry_t)) + 1023) / 1024);
    hdr->data.ver1.dev_attr_offset = hdr->data.ver1.group_table_offset +
        (((attr->max_group * sizeof(ps_group_entry_t)) + 1023) / 1024);
    hdr->data.ver1.group_attr_offset = hdr->data.ver1.dev_attr_offset +
        (((attr->max_dev * attr->dev_attr_size) + 1023) / 1024);

/* FIXME: make LRDB start on a cylinder boundary or power of 2, if possible */
    hdr->data.ver1.lrdb_offset = hdr->data.ver1.group_attr_offset +
        (((attr->max_group * attr->group_attr_size) + 1023) / 1024);
    hdr->data.ver1.hrdb_offset = hdr->data.ver1.lrdb_offset +
        (((attr->max_dev * attr->lrdb_size) + 1023) / 1024);

    hdr->data.ver1.last_block = hdr->data.ver1.hrdb_offset +
        (((attr->max_dev * attr->hrdb_size) + 1023) / 1024) - 1;
}


/*
 * return the sector offset to the LRDB for a device
 */
int
ps_get_lrdb_offset(char *ps_name, char *dev_name, unsigned int *offset)
{
    int      fd, ret, dev_index;
    ps_hdr_t header;

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
    if (lseek(fd, offset, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, buffer, buf_len) != buf_len) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    *num_bits = table[dev_index].num_lrdb_bits;

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
    if (lseek(fd, offset, SEEK_SET) == -1) {
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
 * get the device HRDB from the driver 
 */
int
ps_get_hrdb(char *ps_name, char *dev_name, char *buffer, 
    int buf_len, unsigned int *num_bits)
{
    int            fd, ret, dev_index;
    ps_hdr_t       header;
    unsigned int   offset;
    ps_dev_entry_t *table;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > header.data.ver1.hrdb_size) {
        free(table);
        close(fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.hrdb_offset * 1024) +
             (dev_index * header.data.ver1.hrdb_size);
    if (lseek(fd, offset, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }
    if (read(fd, buffer, buf_len) != buf_len) {
        free(table);
        close(fd);
        return PS_READ_ERROR;
    }

    *num_bits = table[dev_index].num_hrdb_bits;

    /* close the store */
    free(table);
    close(fd);

    return PS_OK;
}

/*
 * load the device HRDB into the driver 
 */
int
ps_set_hrdb(char *ps_name, char *dev_name, char *buffer, int buf_len)
{
    int          fd, ret, dev_index;
    ps_hdr_t     header;
    unsigned int offset;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > header.data.ver1.hrdb_size) {
        close(fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.hrdb_offset * 1024) +
             (dev_index * header.data.ver1.hrdb_size);
    if (lseek(fd, offset, SEEK_SET) == -1) {
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

    /* read the data */
    offset = (header.data.ver1.dev_attr_offset * 1024) +
             (dev_index * header.data.ver1.dev_attr_size);
    if (lseek(fd, offset, SEEK_SET) == -1) {
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
 * Get the attribute buffer for a device
 */
int
ps_get_device_attr(char *ps_name, char *dev_name, char *buffer, int buf_len)
{
    int          fd, ret, dev_index;
    ps_hdr_t     header;
    unsigned int offset;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, NULL);
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
    if (lseek(fd, offset, SEEK_SET) == -1) {
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

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].shutdown = value;

    offset = (hdr.data.ver1.group_table_offset * 1024) +
             (index * sizeof(ps_group_entry_t));
    if (lseek(fd, offset, SEEK_SET) == -1) {
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

/*
 * Set the attribute buffer for the group
 */
int
ps_set_group_attr(char *ps_name, char *group_name, char *buffer, int buf_len)
{
    int          fd, ret, group_index;
    ps_hdr_t     header;
    unsigned int offset;

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
    if (lseek(fd, offset, SEEK_SET) == -1) {
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

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].state = state;

    offset = (hdr.data.ver1.group_table_offset * 1024) +
             (index * sizeof(ps_group_entry_t));
    if (lseek(fd, offset, SEEK_SET) == -1) {
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

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].state = state;

    offset = (hdr.data.ver1.dev_table_offset * 1024) +
             (index * sizeof(ps_dev_entry_t));
    if (lseek(fd, offset, SEEK_SET) == -1) {
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

/*
 * set the state value for the device
 */
int
ps_set_device_size(char *ps_name, char *dev_name, unsigned int num_sectors)
{
    int            fd, ret, index;
    ps_hdr_t       hdr;
    unsigned int   offset;
    ps_dev_entry_t *table;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].num_sectors = num_sectors;

    offset = (hdr.data.ver1.dev_table_offset * 1024) +
             (index * sizeof(ps_dev_entry_t));
    if (lseek(fd, offset, SEEK_SET) == -1) {
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

/*
 * Get the attribute buffer for the group
 */
int
ps_get_group_key_value(char *ps_name, char *group_name, char *key, char *value)
{
    int          fd, ret, group_index, buf_len;
    char         *buffer, *temp, *tempval, *tempkey;
    ps_hdr_t     header;
    unsigned int offset;

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
    if (lseek(fd, offset, SEEK_SET) == -1) {
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
    while (getline(&temp, &tempkey, &tempval, '\n')) {
        if (strcmp(key, tempkey) == 0) {
            strcpy(value, tempval);
            free(buffer);
            return PS_OK;
        }
    }

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

    /* open the store */
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
    if (lseek(fd, offset, SEEK_SET) == -1) {
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
    while (getline(&temp, &ps_key[num_ps], &ps_value[num_ps], '\n')) {
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
    if (lseek(fd, offset, SEEK_SET) == -1) {
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
    if (lseek(fd, offset, SEEK_SET) == -1) {
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
 * add a group to the persistent store
 */
int
ps_add_group(char *ps_name, ps_group_info_t *info)
{
    int              i, fd;
    ps_hdr_t         hdr;
    unsigned int     table_size, offset;
    ps_group_entry_t *table, entry;

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
    lseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    if (hdr.magic != PS_VERSION_1_MAGIC) {
        close(fd);
        return PS_BOGUS_HEADER;
    }

    table_size = sizeof(ps_group_entry_t) * hdr.data.ver1.max_group;
    if ((table = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (lseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1) {
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

    /* search for the device number */
    for (i = 0; i < hdr.data.ver1.max_group; i++) {
        if (table[i].pathlen == 0) {
            free(table);
            offset = (hdr.data.ver1.group_table_offset * 1024) +
                     (i * sizeof(ps_group_entry_t));
            if (lseek(fd, offset, SEEK_SET) == -1) {
                close(fd);
                return PS_SEEK_ERROR;
            }
            if (write(fd, &entry, sizeof(ps_group_entry_t)) != sizeof(ps_group_entry_t)) {
                close(fd);
                return PS_WRITE_ERROR;
            }

            /* increment the number of devices */
            hdr.data.ver1.num_group++;
            if (i > hdr.data.ver1.last_group) {
                hdr.data.ver1.last_group = i;
            }

            /* rewrite the header */
            lseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
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
ps_delete_group(char *ps_name, char *group_name)
{
    int              i, fd, ret, group_index;
    ps_hdr_t         header;
    unsigned int     offset;
    ps_group_entry_t entry, *table;

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
        &group_index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    /* overwrite the data */
    offset = (header.data.ver1.group_table_offset * 1024) +  
             (group_index * sizeof(ps_group_entry_t));
    if (lseek(fd, offset, SEEK_SET) == -1) {
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
    lseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        free(table);
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    free(table);
    close(fd);

    return PS_OK;
}

/*
 * add a device to the persistent store
 */
int
ps_add_device(char *ps_name, ps_dev_info_t *info)
{
    int            i, fd;
    ps_hdr_t       hdr;
    unsigned int   table_size, offset;
    ps_dev_entry_t entry, *table;

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
    entry.num_lrdb_bits = info->num_lrdb_bits;
    entry.num_hrdb_bits = info->num_hrdb_bits;
    entry.num_sectors = info->num_sectors;

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    lseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    if (hdr.magic != PS_VERSION_1_MAGIC) {
        close(fd);
        return PS_BOGUS_HEADER;
    }

    table_size = sizeof(ps_dev_entry_t) * hdr.data.ver1.max_dev;
    if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (lseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
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

    /* search for the device number */
    for (i = 0; i < hdr.data.ver1.max_dev; i++) {
        if (table[i].pathlen == 0) {
            free(table);
            offset = (hdr.data.ver1.dev_table_offset * 1024) +
                (i * sizeof(ps_dev_entry_t));
            if (lseek(fd, offset, SEEK_SET) == -1) {
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
            lseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
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
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    /* overwrite the data */
    offset = (header.data.ver1.dev_table_offset * 1024) +  
        (dev_index * sizeof(ps_dev_entry_t));
    if (lseek(fd, offset, SEEK_SET) == -1) {
        free(table);
        close(fd);
        return PS_SEEK_ERROR;
    }

    entry.pathlen = 0;
    memset(entry.path, 0, MAXPATHLEN);
    entry.num_lrdb_bits = 0xffffffff;
    entry.num_hrdb_bits = 0xffffffff;

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
    lseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        free(table);
        close(fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    free(table);
    close(fd);

    return PS_OK;
}

#define BS_SIZE 64

/*
 * create a Version 1 persistent store 
 */
int
ps_create_version_1(char *ps_name, ps_version_1_attr_t *attr)
{
    int              fd, i, num;
    ps_hdr_t         hdr;
    struct stat      statbuf;
    unsigned int     table_size, buflen;
    unsigned char    *clearbuf;
    ps_dev_entry_t   *dtable;
    ps_group_entry_t *gtable;

    /* stat the device and make sure it is a slice */
    if (stat(ps_name, &statbuf) != 0) {
        return PS_BOGUS_PS_NAME;
    }
    if (!(S_ISBLK(statbuf.st_mode))) {
        return PS_BOGUS_PS_NAME;
    }

/* FIXME: check the size of the volume and make sure we can initialize it. */

    /* open the store */
    if ((fd = open(ps_name, O_RDWR)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    init_ps_header(&hdr, attr);

    /* go back to the beginning and write the header */
    if (lseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET) == -1) {
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
        dtable[i].num_lrdb_bits = 0xffffffff;
        dtable[i].num_hrdb_bits = 0xffffffff;
        dtable[i].num_sectors = 0xffffffff;
    }

    /* write out the device index array */
    if (lseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
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
    if (lseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1) {
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

    /* DONE! */
    close(fd);

    return PS_OK;
}

/*
 * get the attributes of a Version 1.0 persistent store
 */
int
ps_get_version_1_attr(char *ps_name, ps_version_1_attr_t *attr)
{
    int      fd;
    ps_hdr_t hdr;

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    lseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    close(fd);

    if (hdr.magic != PS_VERSION_1_MAGIC) {
        return PS_BOGUS_HEADER;
    }

    attr->max_dev = hdr.data.ver1.max_dev;
    attr->max_group = hdr.data.ver1.max_group;
    attr->dev_attr_size = hdr.data.ver1.dev_attr_size;
    attr->group_attr_size = hdr.data.ver1.group_attr_size;
    attr->lrdb_size = hdr.data.ver1.lrdb_size;
    attr->hrdb_size = hdr.data.ver1.hrdb_size;
    attr->dev_table_entry_size = hdr.data.ver1.dev_table_entry_size;
    attr->group_table_entry_size = hdr.data.ver1.group_table_entry_size;
    attr->num_device = hdr.data.ver1.num_device;
    attr->num_group = hdr.data.ver1.num_group;
    attr->last_device = hdr.data.ver1.last_device;
    attr->last_group = hdr.data.ver1.last_group;

    return PS_OK;
}

/*
 * get the version of the persistent store
 */
int
ps_get_version(char *ps_name, int *version)
{
    int      fd;
    ps_hdr_t hdr;

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    lseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    close(fd);
    if (hdr.magic != PS_VERSION_1_MAGIC) {
        *version = -1;
        return PS_BOGUS_HEADER;
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
    int              fd, i, j;
    ps_hdr_t         hdr;
    unsigned int     table_size;
    ps_group_entry_t *table;

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    lseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    if (hdr.magic != PS_VERSION_1_MAGIC) {
        return PS_BOGUS_HEADER;
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

    /* get the device index array */
    if (lseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == -1) {
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
 * Fill the buffer with the groups in the persistent store
 */
int
ps_get_device_list(char *ps_name, char *buffer, int buf_len)
{
    int            fd, i, j;
    ps_hdr_t       hdr;
    unsigned int   table_size;
    ps_dev_entry_t *table;

    /* open the store */
    if ((fd = open(ps_name, O_RDWR | O_SYNC)) == -1) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    lseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    read(fd, &hdr, sizeof(ps_hdr_t));
    if (hdr.magic != PS_VERSION_1_MAGIC) {
        return PS_BOGUS_HEADER;
    }

    if ((buf_len * MAXPATHLEN) < hdr.data.ver1.max_dev) {
        return PS_BOGUS_BUFFER_LEN;
    }

    table_size = sizeof(ps_dev_entry_t) * hdr.data.ver1.max_dev;
    if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        close(fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (lseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == -1) {
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
 *
 */
int
ps_get_device_index(char *ps_name, char *dev_name, int *index)
{
    int            fd, ret;
    ps_hdr_t       hdr;

    /* open the store */
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

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }
    close(fd);

    strcpy(info->name, table[index].path);
    info->num_lrdb_bits = table[index].num_lrdb_bits;
    info->num_hrdb_bits = table[index].num_hrdb_bits;
    info->num_sectors = table[index].num_sectors;
    info->state = table[index].state;

    free(table);

    return PS_OK;
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
getline (char **buffer, char **key, char **value, char delim)
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

