/*
 * ftd_ps.c - Persistent store interface
 *
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#include "ftd_port.h"
#include "ftd_lg.h"
#include "ftd_ps.h"
#include "ftd_devio.h"
#include "disksize.h"
#include "ftd_error.h" 

static int open_ps_get_dev_info(char *ps_name, char *dev_name, HANDLE *outfd, 
    ps_hdr_t *hdr, int *dev_index,
    ps_dev_entry_t **ret_table);
static int open_ps_get_group_info(char *ps_name, char *group_name, 
    HANDLE *outfd, ps_hdr_t *hdr, 
    int *group_index, 
    ps_group_entry_t **ret_table);
static int ps_getline (char **buffer, char **key, char **value, char delim);

/*
 * A very simple function. Just write out a new header. Everything on the
 * disk is wasted by the create function.
 */
int
ps_create(char *ps_name, int *max_dev)
{
	ftd_uint64_t		size, err;
#if defined(HPUX)
	char				raw_name[MAXPATHLEN];
#endif
	ps_version_1_attr_t	attr;
	int					dsize, maxdev;

	/* convert the block device name to the raw device name */
#if defined(HPUX)
	if (is_logical_volume(ps_name)) {
		convert_lv_name(raw_name, ps_name, 1);
/* FIXME: for logical volumes, should we make sure it has contiguous blocks? */
	} else {
		force_dsk_or_rdsk(raw_name, ps_name, 1);
	}
#else
#ifdef OLD_PSTORE
	force_dsk_or_rdsk(raw_name, ps_name, 1);

	/* stat the pstore device and figure out the maximum number of devices */
	if ((dsize = disksize(raw_name)) < 0) {
		reporterr(ERRFAC, M_STAT, ERRCRIT, ps_name, ftd_strerror());
		return -1;
	}
#else
	/* Force dsize to the maximum number of devices  */
      dsize = 295968;
#endif

#endif

	/* FIXME: the 3*MAXPATHLEN fudge is not kosher, but it works. The fudge
	 * takes into account the table entry sizes for devices and groups, but
	 * the table entry sizes may change in the future. Fix this by asking 
	 * the pstore interface how much space each device uses and how much space
	 * each group uses.
	 */
    
	/* the size of one device assuming one device per group */
    size = FTD_PS_LRDB_SIZE + FTD_PS_HRDB_SIZE + FTD_PS_DEVICE_ATTR_SIZE +
           FTD_PS_GROUP_ATTR_SIZE + 3*MAXPATHLEN;

    size = size / DEV_BSIZE;
	
	/* BOGUSITY: assume the pstore starts at 16K from the start of the device */
    maxdev = (daddr_t)((dsize - 32) / size);

    /* create a new pstore with the defaults */
    attr.max_dev = maxdev < (FTD_MAX_GROUPS*FTD_MAX_DEVICES) ? maxdev: (FTD_MAX_GROUPS*FTD_MAX_DEVICES);
    attr.max_group = maxdev < (FTD_MAX_GROUPS*FTD_MAX_DEVICES) ? maxdev: (FTD_MAX_GROUPS*FTD_MAX_DEVICES);
    attr.lrdb_size = FTD_PS_LRDB_SIZE;
    attr.hrdb_size = FTD_PS_HRDB_SIZE;
    attr.group_attr_size = FTD_PS_GROUP_ATTR_SIZE;
    attr.dev_attr_size = FTD_PS_DEVICE_ATTR_SIZE;

	*max_dev = attr.max_dev;

    if ((err = ps_create_version_1(ps_name, &attr)) != PS_OK) {
        reporterr(ERRFAC, M_PSCREATE, ERRCRIT, ps_name, err);
        return -1;
    }

    return 0;
}

/*
 * A common action of almost all public device functions. Open the 
 * persistent store, read the header, verify the header magic number, 
 * read the device table, and search the device table for a matching path. 
 * Returns device index, header info, open file descriptor, and device 
 * entry info, if no errors occur. 
 */
static int
open_ps_get_dev_info(char *ps_name, char *dev_name, HANDLE *outfd, 
    ps_hdr_t *hdr, int *dev_index, ps_dev_entry_t **ret_table)
{
	HANDLE			fd;
    int            i, pathlen;
    unsigned int   table_size;
    ps_dev_entry_t *table;

    *dev_index = -1;
    *outfd = (HANDLE)-1;

    /* get the device number */
    if ((pathlen = strlen(dev_name)) == 0) {
        return PS_BOGUS_DEVICE_NAME;
    }

    /* open the store */
    //if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
    if ((fd = ftd_open(ps_name, O_RDWR, 0)) == INVALID_HANDLE_VALUE) {
        reporterr(ERRFAC, M_OPEN, ERRWARN, ps_name, ftd_strerror());
		return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    ftd_read(fd, hdr, sizeof(ps_hdr_t));
    
	if (hdr->magic != PS_VERSION_1_MAGIC) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
        return PS_BOGUS_HEADER;
    }

    table_size = BLOCK_SIZE(sizeof(ps_dev_entry_t) * hdr->data.ver1.max_dev);
    if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (ftd_llseek(fd, hdr->data.ver1.dev_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, (caddr_t)table, table_size) != (int)table_size) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_READ_ERROR;
    }

    /* search for the device number */
    for (i = 0; i < (int)hdr->data.ver1.max_dev; i++) {
        if ((table[i].pathlen == pathlen) && 
            (strncmp(table[i].path, dev_name, pathlen) == 0))
		{
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
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

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
open_ps_get_group_info(char *ps_name, char *group_name, HANDLE *outfd, 
    ps_hdr_t *hdr, int *group_index, 
    ps_group_entry_t **ret_table)
{
	HANDLE			fd;
    int              i, pathlen;
    unsigned int     table_size;
    ps_group_entry_t *table;

    *group_index = -1;
    *outfd = (HANDLE)-1;

	error_tracef(TRACEINF4,"open_ps_get_group_info()");		
    /* get the device number */
    if ((pathlen = strlen(group_name)) == 0) {
		error_tracef(TRACEERR,"open_ps_get_group_info(): PS_BOGUS_DEVICE_NAME");		
        return PS_BOGUS_DEVICE_NAME;
    }

    /* open the store */
      if ((fd = ftd_open(ps_name, O_RDWR, 0)) == INVALID_HANDLE_VALUE) {
		error_tracef(TRACEERR,"open_ps_get_group_info(): PS_BOGUS_PS_NAME");		
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    ftd_read(fd, hdr, sizeof(ps_hdr_t));
    
	if (hdr->magic != PS_VERSION_1_MAGIC) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
		error_tracef(TRACEERR,"open_ps_get_group_info(): PS_BOGUS_HEADER");
        return PS_BOGUS_HEADER;
    }

    table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr->data.ver1.max_group);
    if ((table = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		error_tracef( TRACEERR,"open_ps_get_group_info(): PS_MALLOC_ERROR");
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (ftd_llseek(fd, hdr->data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		error_tracef(TRACEERR,"open_ps_get_group_info(): PS_SEEK_ERROR");
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, (caddr_t)table, table_size) != (int)table_size) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		error_tracef(TRACEERR,"open_ps_get_group_info(): PS_READ_ERROR");
        return PS_READ_ERROR;
    }

    /* search for the device number */
    for (i = 0; i < (int)hdr->data.ver1.max_group; i++) {
        if (    (table[i].pathlen == pathlen)
		     && (strncmp(table[i].path, group_name, pathlen) == 0) ) {

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
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

	error_tracef(TRACEERR,"open_ps_get_group_info(): PS_GROUP_NOT_FOUND");
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
	HANDLE	fd;
    int      ret, dev_index;
    ps_hdr_t header;

    *offset = 0xffffffff;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

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
	HANDLE			fd;
    int            ret, dev_index;
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
    if (buf_len > (int)header.data.ver1.lrdb_size) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.lrdb_offset * 1024) +
             (dev_index * header.data.ver1.lrdb_size);
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, buffer, buf_len) != buf_len) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_READ_ERROR;
    }

    *num_bits = table[dev_index].num_lrdb_bits;

    /* close the store */
    free(table);
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 * load the device LRDB into the driver 
 */
int
ps_set_lrdb(char *ps_name, char *dev_name, char *buffer, int buf_len)
{
	HANDLE		fd;
    int          ret, dev_index;
    ps_hdr_t     header;
    unsigned int offset;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > (int)header.data.ver1.lrdb_size) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.lrdb_offset * 1024) +
             (dev_index * header.data.ver1.lrdb_size);
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }

    if (ftd_write(fd, buffer, buf_len) != buf_len) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}


/*
 * get the device HRDB from the driver 
 */
int
ps_get_hrdb(char *ps_name, char *dev_name, char *buffer, 
    int buf_len, unsigned int *num_bits)
{
	HANDLE		fd;
    int            ret, dev_index;
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
    if (buf_len > (int)header.data.ver1.hrdb_size) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.hrdb_offset * 1024) +
             (dev_index * header.data.ver1.hrdb_size);
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, buffer, buf_len) != buf_len) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_READ_ERROR;
    }

    *num_bits = table[dev_index].num_hrdb_bits;

    /* close the store */
    free(table);
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 * load the device HRDB into the driver 
 */
int
ps_set_hrdb(char *ps_name, char *dev_name, char *buffer, int buf_len)
{
	HANDLE		fd;
    int          ret, dev_index;
    ps_hdr_t     header;
    unsigned int offset;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > (int)header.data.ver1.hrdb_size) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.hrdb_offset * 1024) +
             (dev_index * header.data.ver1.hrdb_size);
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_write(fd, buffer, buf_len) != buf_len) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 * Set the attribute buffer for a device
 */
int
ps_set_device_attr(char *ps_name, char *dev_name, char *buffer, int buf_len)
{
	HANDLE		fd;
    int          ret, dev_index;
    ps_hdr_t     header;
    unsigned int offset;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > (int)header.data.ver1.dev_attr_size) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.dev_attr_offset * 1024) +
             (dev_index * header.data.ver1.dev_attr_size);
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_write(fd, buffer, buf_len) != buf_len) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 * Get the attribute buffer for a device
 */
int
ps_get_device_attr(char *ps_name, char *dev_name, char *buffer, int buf_len)
{
	HANDLE		fd;
    int          ret, dev_index;
    ps_hdr_t     header;
    unsigned int offset;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &header, 
        &dev_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > (int)header.data.ver1.dev_attr_size) {
        buf_len = (int)header.data.ver1.dev_attr_size;
    }

    /* read the data */
    offset = (header.data.ver1.dev_attr_offset * 1024) +
             (dev_index * header.data.ver1.dev_attr_size);
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, buffer, buf_len) != buf_len) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_READ_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 * Set the state stamp value, LG_INACTIVE - LG_STARTED - LG_ACTIVE
 */
int
ps_set_group_state_stamp(char *ps_name, char *group_name, int value)
{
	HANDLE				fd;
    int					ret, index;
    ps_hdr_t			hdr;
    ps_group_entry_t	*table;
    unsigned int        table_size;

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].state_stamp = value;

    if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
        return PS_SEEK_ERROR;
    }

    table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr.data.ver1.max_group);
    if (ftd_write(fd, table, table_size) != (int)table_size) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
    free(table);

    return PS_OK;
}

/*
 * Set the attribute buffer for the group
 */
int
ps_set_group_attr(char *ps_name, char *group_name, char *buffer, int buf_len)
{
	HANDLE		fd;
    int          ret, group_index;
    ps_hdr_t     header;
    unsigned int offset;

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
        &group_index, NULL);
    if (ret != PS_OK) {
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > (int)header.data.ver1.group_attr_size) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_BOGUS_BUFFER_LEN;
    }

    /* read the data */
    offset = (header.data.ver1.group_attr_offset * 1024) +
             (group_index * header.data.ver1.group_attr_size);
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_write(fd, buffer, buf_len) != buf_len) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 * set the state value for the group
 */
int
ps_set_group_state(char *ps_name, char *group_name, int state)
{
    HANDLE              fd;
    int                 ret, index;
    ps_hdr_t            hdr;
    ps_group_entry_t    *table;
    unsigned int        table_size;

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].state = state;

    if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
        return PS_SEEK_ERROR;
    }

    table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr.data.ver1.max_group);
    if (ftd_write(fd, (caddr_t)table, table_size) != (int)table_size) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
    free(table);

    return PS_OK;
}

/*
 * set the autostart value for the group
 */
int
ps_set_group_autostart(char *ps_name, char *group_name, int autostart)
{
    HANDLE              fd;
    int                 ret, index;
    ps_hdr_t            hdr;
    ps_group_entry_t    *table;
    unsigned int        table_size;

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    table[index].autostart = autostart;

    if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
        return PS_SEEK_ERROR;
    }

    table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr.data.ver1.max_group);
    if (ftd_write(fd, (caddr_t)table, table_size) != (int)table_size) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
    free(table);

    return PS_OK;
}

/*
 * set the checkpoint value for the group
 */
int
ps_set_group_checkpoint(char *ps_name, char *group_name, int checkpoint)
{
    HANDLE              fd;
    int                 ret, index;
    ps_hdr_t            hdr;
    ps_group_entry_t    *table;
    unsigned int        table_size;

	error_tracef( TRACEINF4,"ps_set_group_checkpoint() ");
    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
		error_tracef(TRACEERR,"ps_set_group_checkpoint(): PS_OK");
        return (ret);
    }

    table[index].checkpoint = checkpoint;

    if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
		error_tracef(TRACEERR,"ps_set_group_checkpoint(): PS_SEEK_ERROR");
        return PS_SEEK_ERROR;
    }

    table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr.data.ver1.max_group);
    if (ftd_write(fd, (caddr_t)table, table_size) != (int)table_size) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
		error_tracef(TRACEERR,"ps_set_group_checkpoint(): PS_WRITE_ERROR");
        return PS_WRITE_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
    free(table);

    return PS_OK;
}

/*
 * set the state value for the device
 */
int
ps_set_device_state(char *ps_name, char *dev_name, int state)
{
    HANDLE          fd;
    int             ret, index;
    ps_hdr_t        hdr;
    ps_dev_entry_t  *table;
	unsigned int	table_size;

	error_tracef( TRACEINF4,"ps_set_device_state() ");
    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
		error_tracef(TRACEERR,"ps_set_device_state(): PS_OK");
        return (ret);
    }

    table[index].state = state;

    if (ftd_llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
		error_tracef(TRACEERR,"ps_set_device_state(): PS_SEEK_ERROR");
        return PS_SEEK_ERROR;
    }

    table_size = BLOCK_SIZE(sizeof(ps_dev_entry_t) * hdr.data.ver1.max_group);
    if (ftd_write(fd, table, table_size) != (int)table_size) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
		error_tracef(TRACEERR,"ps_set_device_state(): PS_WRITE_ERROR");
        return PS_WRITE_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
    free(table);

    return PS_OK;
}

/*
 * set the state value for the device
 */
int
ps_set_device_size(char *ps_name, char *dev_name, unsigned int num_sectors)
{
    HANDLE          fd;
    int             ret, index;
    ps_hdr_t        hdr;
    ps_dev_entry_t  *table;
	unsigned int	table_size;


	error_tracef( TRACEINF4,"ps_set_device_size() ");
		
    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        error_tracef(TRACEERR,"ps_set_device_size(): PS_OK");
        return (ret);
    }

    table[index].num_sectors = num_sectors;

    if (ftd_llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
		error_tracef(TRACEERR,"ps_set_device_size(): PS_SEEK_ERROR");
        return PS_SEEK_ERROR;
    }

    table_size = BLOCK_SIZE(sizeof(ps_dev_entry_t) * hdr.data.ver1.max_group);
    if (ftd_write(fd, table, table_size) != (int)table_size) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(table);
		error_tracef(TRACEERR,"ps_set_device_size(): PS_WRITE_ERROR");
        return PS_WRITE_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
    free(table);

    return PS_OK;
}

/*
 * Get the attribute buffer for the group
 */
int
ps_get_group_key_value(char *ps_name, char *group_name, char *key, char *value)
{
	HANDLE		fd;
    int          ret, group_index, buf_len;
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
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return (PS_MALLOC_ERROR);
    }

    /* read the data */
    offset = (header.data.ver1.group_attr_offset * 1024) +
             (group_index * header.data.ver1.group_attr_size);
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, buffer, buf_len) != buf_len) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_READ_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    /* parse the buffer for a matching key */
/* FIXME: */
    /* parse the attributes into key/value pairs */
    temp = buffer;
    while (ps_getline(&temp, &tempkey, &tempval, '\n')) {
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
	HANDLE		fd;
    int          ret, group_index, buf_len, found, i;
    int          num_ps, len, linelen;
    char         *inbuf, *outbuf, *temp;
    char         line[MAXPATHLEN];
    char         *ps_key[FTD_MAX_KEY_VALUE_PAIRS];
    char         *ps_value[FTD_MAX_KEY_VALUE_PAIRS];
    ps_hdr_t     header;
    unsigned int offset;

	error_tracef(TRACEINF4,"ps_set_group_key_value() ");

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
        &group_index, NULL);
    if (ret != PS_OK) {
		error_tracef(TRACEERR,"ps_set_group_key_value(): Can't open the pstore");
        return (ret);
    }

    /* allocate a buffer for the data */
    buf_len = header.data.ver1.group_attr_size;
    if ((inbuf = (char *)malloc(buf_len)) == NULL) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
	    error_tracef(TRACEERR,"ps_set_group_key_value(): PS_MALLOC_ERROR");
        return (PS_MALLOC_ERROR);
    }
    if ((outbuf = (char *)calloc(1, buf_len)) == NULL) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(inbuf);
		error_tracef(TRACEERR,"ps_set_group_key_value(): PS_MALLOC_ERROR_2");
        return (PS_MALLOC_ERROR);
    }

    /* read the data */
    offset = (header.data.ver1.group_attr_offset * 1024) +
             (group_index * header.data.ver1.group_attr_size);
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		error_tracef(TRACEERR,"ps_set_group_key_value(): PS_SEEK_ERROR");
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, inbuf, buf_len) != buf_len) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		error_tracef(TRACEERR,"ps_set_group_key_value(): PS_READ_ERROR");
        return PS_READ_ERROR;
    }

    /* parse the attributes into key/value pairs */
    temp = inbuf;
    num_ps = 0;
    while (ps_getline(&temp, &ps_key[num_ps], &ps_value[num_ps], '\n')) {
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
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(inbuf);
        free(outbuf);
		error_tracef(TRACEERR,"ps_set_group_key_value(): PS_SEEK_ERROR_2");
        return PS_SEEK_ERROR;
    }
    if (ftd_write(fd, outbuf, buf_len) != buf_len) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        free(inbuf);
        free(outbuf);
		error_tracef(TRACEERR,"ps_set_group_key_value(): PS_WRITE_ERROR");
        return PS_WRITE_ERROR;
    }

    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

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
	HANDLE		fd;
    int          ret, group_index;
    ps_hdr_t     header;
    unsigned int offset;

	error_tracef(TRACEINF4,"ps_get_group_attr() ");

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
        &group_index, NULL);
    if (ret != PS_OK) {
		error_tracef(TRACEERR,"ps_get_group_attr(): Can't open the pstore");
        return (ret);
    }

    /* make sure the buffer is not too big */
    if (buf_len > (int)header.data.ver1.group_attr_size) {
        buf_len = (int)header.data.ver1.group_attr_size;
    }

    /* read the data */
    offset = (header.data.ver1.group_attr_offset * 1024) +
             (group_index * header.data.ver1.group_attr_size);
    if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		error_tracef(TRACEERR,"ps_get_group_attr(): PS_SEEK_ERROR");
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, buffer, buf_len) != buf_len) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
		error_tracef(TRACEERR,"ps_get_group_attr(): PS_READ_ERROR");
        return PS_READ_ERROR;
    }

    /* close the store */
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 * add a group to the persistent store
 */
int
ps_add_group(char *ps_name, ps_group_info_t *info)
{
	HANDLE			fd;
    int              i;
    ps_hdr_t         hdr;
    unsigned int     table_size;
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
    if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    ftd_read(fd, &hdr, sizeof(ps_hdr_t));

    if (hdr.magic != PS_VERSION_1_MAGIC) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
        return PS_BOGUS_HEADER;
    }

    table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr.data.ver1.max_group);
    if ((table = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, (caddr_t)table, table_size) != (int)table_size) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_READ_ERROR;
    }

    /* if group already exists, blow it off */
    for (i = 0; i < (int)hdr.data.ver1.max_group; i++) {
        if ((table[i].pathlen == entry.pathlen) &&
            (strncmp(table[i].path, entry.path, entry.pathlen) == 0)) {
            free(table);
            FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
            return PS_OK;
        }
    }

    /* search for the device number */
    for (i = 0; i < (int)hdr.data.ver1.max_group; i++) {
        if (table[i].pathlen == 0) {
            table[i] = entry;

            if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
                free(table);
                FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
                return PS_SEEK_ERROR;
            }
            if (ftd_write(fd, (caddr_t)table, table_size) != (int)table_size) {
                free(table);
                FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
                return PS_READ_ERROR;
            }

            free(table);

            /* increment the number of devices */
            hdr.data.ver1.num_group++;
            if (i > hdr.data.ver1.last_group) {
                hdr.data.ver1.last_group = i;
            }

            /* rewrite the header */
            ftd_llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
            if (ftd_write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
                FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
                return PS_WRITE_ERROR;
            }
            
            FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
            return PS_OK;
        }
    }

    free(table);
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_NO_ROOM;
}

/*
 * delete a group from the persistent store
 */
int
ps_delete_group(char *ps_name, char *group_name)
{
	HANDLE			fd;
    int              i, ret, group_index;
    ps_hdr_t         header;
    unsigned int     table_size;
    ps_group_entry_t entry, *table;

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &header, 
        &group_index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    entry.pathlen = 0;
    memset(entry.path, 0, MAXPATHLEN);
    entry.hostid = 0xffffffff;

    table[group_index] = entry;

	/* overwrite the data */
    if (ftd_llseek(fd, header.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }

    table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * header.data.ver1.max_group);
    if (ftd_write(fd, table, table_size) != (int)table_size) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
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
    ftd_llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
    if (ftd_write(fd, &header, sizeof(header)) != sizeof(header)) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    free(table);
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 * add a device to the persistent store
 */
int
ps_add_device(char *ps_name, ps_dev_info_t *info)
{
	HANDLE		fd;
    int            i;
    ps_hdr_t       hdr;
    unsigned int   table_size;
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
    if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    ftd_read(fd, &hdr, sizeof(ps_hdr_t));
    if (hdr.magic != PS_VERSION_1_MAGIC) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
        return PS_BOGUS_HEADER;
    }

    table_size = BLOCK_SIZE(sizeof(ps_dev_entry_t) * hdr.data.ver1.max_dev);
    if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (ftd_llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, table, table_size) != (int)table_size) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_READ_ERROR;
    }

    /* if device already exists, blow it off */
    for (i = 0; i < (int)hdr.data.ver1.max_dev; i++) {
        if ((table[i].pathlen == entry.pathlen) &&
            (strncmp(table[i].path, entry.path, entry.pathlen) == 0)) {
            free(table);
            FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
            return PS_OK;
        }
    }

    /* search for the device number */
    for (i = 0; i < (int)hdr.data.ver1.max_dev; i++) {
        if (table[i].pathlen == 0) {

            table[i] = entry;

            if (ftd_llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
                free(table);
                FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
                return PS_SEEK_ERROR;
            }
            if (ftd_write(fd, (caddr_t)table, table_size) != (int)table_size) {
                free(table);
                FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
                return PS_READ_ERROR;
            }

            free(table);

            /* increment the number of devices */
            hdr.data.ver1.num_device++;
            if (i > hdr.data.ver1.last_device) {
                hdr.data.ver1.last_device = i;
            }

            /* rewrite the header */
            ftd_llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
            if (ftd_write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
                FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
                return PS_WRITE_ERROR;
            }
            
            FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
            return PS_OK;
        }
    }

    free(table);
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_NO_ROOM;
}

/*
 * delete a device from the persistent store
 */
int
ps_delete_device(char *ps_name, char *dev_name)
{
    HANDLE          fd;
    int             i, ret, dev_index;
    ps_hdr_t        hdr;
    ps_dev_entry_t  entry, *table;
	unsigned int	table_size;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, 
        &dev_index, &table);
    if (ret != PS_OK) {
        return (ret);
    }

    entry.pathlen = 0;
    memset(entry.path, 0, MAXPATH);
    entry.num_lrdb_bits = 0xffffffff;
    entry.num_hrdb_bits = 0xffffffff;

    table[dev_index] = entry;

	/* overwrite the data */
    if (ftd_llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }

    table_size = BLOCK_SIZE(sizeof(ps_dev_entry_t) * hdr.data.ver1.max_dev);
    if (ftd_write(fd, table, table_size) != (int)table_size) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_WRITE_ERROR;
    }

    /* if this is the last index, go find the new last index */
    if (dev_index >= hdr.data.ver1.last_device) {
        for (i = dev_index - 1; i >= 0; i--) {
            if (table[i].pathlen > 0) {
                break;
            }
        }
        hdr.data.ver1.last_device = i;
    }
    hdr.data.ver1.num_device--;

    /* rewrite the header */
    ftd_llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
    if (ftd_write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_WRITE_ERROR;
    }

    /* close the store */
    free(table);
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

#define BS_SIZE 64

/*
 * create a Version 1 persistent store 
 */
int
ps_create_version_1(char *ps_name, ps_version_1_attr_t *attr)
{
	HANDLE			fd;
	int				i;
    ps_hdr_t         hdr;
    unsigned int     table_size;
    ps_dev_entry_t   *dtable;
    ps_group_entry_t *gtable;

#if !defined(_WINDOWS)
    struct stat      statbuf;

    /* stat the device and make sure it is a slice */
    if (stat(ps_name, &statbuf) != 0) {
        return PS_BOGUS_PS_NAME;
    }
    if (!(S_ISBLK(statbuf.st_mode))) {
        return PS_BOGUS_PS_NAME;
    }
#endif

	/* FIXME: check the size of the volume and make sure we can initialize it. */

    /* open the store */
    if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
        return PS_BOGUS_PS_NAME;
    }

    init_ps_header(&hdr, attr);

    /* go back to the beginning and write the header */
    if (ftd_llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }

    if (ftd_write(fd, &hdr, sizeof(ps_hdr_t)) != sizeof(ps_hdr_t)) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_WRITE_ERROR;
    }

    /* allocate a table for devices */
    table_size = BLOCK_SIZE(attr->max_dev * sizeof(ps_dev_entry_t));
    if ((dtable = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_MALLOC_ERROR;
    }

    /* set all values to the bogus value */
    for (i = 0; i < (int)attr->max_dev; i++) {
        dtable[i].pathlen = 0;
        memset(dtable[i].path, 0, MAXPATHLEN);
        dtable[i].num_lrdb_bits = 0xffffffff;
        dtable[i].num_hrdb_bits = 0xffffffff;
        dtable[i].num_sectors = 0xffffffff;
    }

    /* write out the device index array */
    if (ftd_llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        free(dtable);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_write(fd, dtable, table_size) != (int)table_size) {
        free(dtable);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_WRITE_ERROR;
    }
    free(dtable);

    /* allocate a table for groups */
    table_size = BLOCK_SIZE(attr->max_group * sizeof(ps_group_entry_t));
    if ((gtable = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_MALLOC_ERROR;
    }

    /* set all values to the bogus value */
    for (i = 0; i < (int)attr->max_group; i++) {
        gtable[i].pathlen = 0;
        memset(gtable[i].path, 0, MAXPATHLEN);
        gtable[i].hostid = 0xffffffff;
    }

    /* write out the group index array */
    if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        free(gtable);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_write(fd, gtable, table_size) != (int)table_size) {
        free(gtable);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_WRITE_ERROR;
    }
    free(gtable);

	/* DONE! */
	FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 * get the attributes of a Version 1.0 persistent store
 */
int
ps_get_version_1_attr(char *ps_name, ps_version_1_attr_t *attr)
{
    HANDLE		fd;
    ps_hdr_t	hdr;

    /* open the store */
    if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */

    ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    ftd_read(fd, &hdr, sizeof(ps_hdr_t));
    
	FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    if (hdr.magic != PS_VERSION_1_MAGIC) {
        //reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
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
	HANDLE		fd;
	ps_hdr_t	hdr;

    /* open the store */
    if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
	ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    ftd_read(fd, &hdr, sizeof(ps_hdr_t));
    
	FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
    
	if (hdr.magic != PS_VERSION_1_MAGIC) {
        *version = -1;
        reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
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
	HANDLE			fd;
	int				i, j;
	ps_hdr_t		hdr;
	unsigned int	table_size;
	ps_group_entry_t	*table;

    /* open the store */
    if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    ftd_read(fd, &hdr, sizeof(ps_hdr_t));
    
	if (hdr.magic != PS_VERSION_1_MAGIC) {
        reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
        return PS_BOGUS_HEADER;
    }

/* FIXME: change to num_group */
    if ((buf_len * MAXPATHLEN) < (int)hdr.data.ver1.max_dev) {
        return PS_BOGUS_BUFFER_LEN;
    }

    table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr.data.ver1.max_group);
    if ((table = (ps_group_entry_t *)malloc(table_size)) == NULL) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, (caddr_t)table, table_size) != (int)table_size) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_READ_ERROR;
    }

    j = 0;
    for (i = 0; i < (int)hdr.data.ver1.max_group; i++) {
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
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 * Fill the buffer with the devices in the persistent store
 */
int
ps_get_device_list(char *ps_name, char *buffer, int buf_len)
{
	HANDLE			fd;
    int            i, j;
    ps_hdr_t       hdr;
    unsigned int   table_size;
    ps_dev_entry_t *table;

    /* open the store */
    if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
        return PS_BOGUS_PS_NAME;
    }

    /* seek to the header location */
    ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

    /* read the header */
    ftd_read(fd, &hdr, sizeof(ps_hdr_t));
    
	if (hdr.magic != PS_VERSION_1_MAGIC) {
        reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
        return PS_BOGUS_HEADER;
    }

    if ((buf_len * MAXPATHLEN) < (int)hdr.data.ver1.max_dev) {
        return PS_BOGUS_BUFFER_LEN;
    }

    table_size = sizeof(ps_dev_entry_t) * hdr.data.ver1.max_dev;
    if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_MALLOC_ERROR;
    }

    /* get the device index array */
    if (ftd_llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_SEEK_ERROR;
    }
    if (ftd_read(fd, (caddr_t)table, table_size) != (int)table_size) {
        free(table);
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
        return PS_READ_ERROR;
    }

    j = 0;
    for (i = 0; i < (int)hdr.data.ver1.max_dev; i++) {
        if (table[i].pathlen != 0) {
			strncpy(&buffer[j*MAXPATHLEN], table[i].path, table[i].pathlen);
            j++;
        }
    }

    /* fill in the rest of the buffer with bogus ids */
    while (j*MAXPATHLEN < buf_len) {
        buffer[j*MAXPATHLEN] = 0;
        j++;
    }
    free(table);
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    return PS_OK;
}

/*
 *
 */
int
ps_get_device_index(char *ps_name, char *dev_name, int *index)
{
	HANDLE			fd;
    int            ret;
    ps_hdr_t       hdr;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, index, NULL);
    if (ret == PS_OK) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
    }

    return ret;
}


/*
 * Get the device info for a device
 */
int
ps_get_group_info(char *ps_name, char *group_name, ps_group_info_t *info)
{
	HANDLE				fd;
    int              ret, index;
    ps_hdr_t         hdr;
    ps_group_entry_t *table;

    /* open the store */
    ret = open_ps_get_group_info(ps_name, group_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

    if (info->name == NULL) {
        info->name = table[index].path;
    }
    info->hostid = table[index].hostid;
    info->state = table[index].state;
    info->state_stamp = table[index].state_stamp;
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
	HANDLE			fd;
    int            ret, index;
    ps_hdr_t       hdr;
    ps_dev_entry_t *table;

    /* open the store */
    ret = open_ps_get_dev_info(ps_name, dev_name, &fd, &hdr, &index, &table);
    if (ret != PS_OK) {
        return (ret);
    }
    FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

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
ps_getline (char **buffer, char **key, char **value, char delim)
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

/*
 * create a new group in the persistent store
 * does all of: ps_add_group, ps_set_group_attr, ps_set_group_state
 */
int
ps_create_group(char *ps_name, ps_group_info_t *info,
	char *buffer, int buf_len, int state)
{
	HANDLE				fd = INVALID_HANDLE_VALUE;
	int					i, ret = PS_OK;
	ps_hdr_t			hdr;
	unsigned int		table_size, offset;
	ps_group_entry_t	*table = NULL, entry;

	if (info == NULL) {
		ret = PS_BOGUS_DEVICE_NAME;
		goto errret;
	}

	// get the device number
	if (((entry.pathlen = strlen(info->name)) == 0)
	|| (entry.pathlen >= MAXPATHLEN)) {
		ret = PS_BOGUS_PATH_LEN;
		goto errret;
	}

	memset(entry.path, 0, sizeof(entry.path));
	strncpy(entry.path, info->name, entry.pathlen);
	entry.hostid = info->hostid;

	// open the store
	if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
		ret = PS_BOGUS_PS_NAME;
		goto errret;
	}

	// seek to the header location
	ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

	// read the header
	ftd_read(fd, &hdr, sizeof(ps_hdr_t));

	if (hdr.magic != PS_VERSION_1_MAGIC) {
		ret = PS_BOGUS_HEADER;
        reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
		goto errret;
	}

	table_size = BLOCK_SIZE(sizeof(ps_group_entry_t) * hdr.data.ver1.max_group);
	if ((table = (ps_group_entry_t *)malloc(table_size)) == NULL) {
		ret = PS_MALLOC_ERROR;
		goto errret;
	}

	// get the device index array
	if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
		ret = PS_SEEK_ERROR;
		goto errret;
	}
	if (ftd_read(fd, (caddr_t)table, table_size) != (int)table_size) {
		ret = PS_READ_ERROR;
		goto errret;
	}

	// if group already exists, blow it off
	for (i = 0; i < (int)hdr.data.ver1.max_group; i++) {
		if ((table[i].pathlen == entry.pathlen)
		&& (strncmp(table[i].path, entry.path, entry.pathlen) == 0)) {
			ret = PS_OK;
			goto errret;
		}
	}

	// search for the device number
	for (i = 0; i < (int)hdr.data.ver1.max_group; i++) {
		if (table[i].pathlen == 0) {

			// found first available group table entry
			
			table[i] = entry;
			table[i].state = state;

			if (ftd_llseek(fd, hdr.data.ver1.group_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
				ret = PS_SEEK_ERROR;
				goto errret;
			}
			if (ftd_write(fd, (caddr_t)table, table_size) != (int)table_size) {
				ret = PS_READ_ERROR;
				goto errret;
			}

			// increment the number of groups
			hdr.data.ver1.num_group++;
			if (i > hdr.data.ver1.last_group) {
				hdr.data.ver1.last_group = i;
			}

			// rewrite the header
			ftd_llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
			if (ftd_write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
				ret = PS_WRITE_ERROR;
				goto errret;
			}
			break;
		}
	}

	if (i == (int)hdr.data.ver1.max_group) {
		ret = PS_NO_ROOM;
		goto errret;
	}

	// make sure the buffer is not too big */
	if (buf_len > (int)hdr.data.ver1.group_attr_size) {
		ret = PS_BOGUS_BUFFER_LEN;
		goto errret;
	}

	// write the group attributes */
	offset = (hdr.data.ver1.group_attr_offset * 1024) +
		(i * hdr.data.ver1.group_attr_size);
    
	if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
		ret = PS_SEEK_ERROR;
		goto errret;
	}
	if (ftd_write(fd, buffer, buf_len) != buf_len) {
		ret = PS_WRITE_ERROR;
		goto errret;
	}

	free(table);
	FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);

	return ret;

errret:
	
	if (table) {
		free(table);
	}

	if (fd != INVALID_HANDLE_VALUE) {
		FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
	}
    
	return ret;
}

/*
 * create a new device in the persistent store
 * does all of: ps_add_device, ps_set_lrdb, ps_set_hrdb,
 * ps_et_device_attr 
 */
int
ps_create_device(char *ps_name, ps_dev_info_t *info,
	int *lrdb, int lrdb_len, int *hrdb, int hrdb_len)
{
	HANDLE			fd;
	int				i, buf_len, ret = PS_OK;
	ps_hdr_t		hdr;
	unsigned int	table_size, offset;
	ps_dev_entry_t	entry, *table;
	char			buffer[FTD_PS_DEVICE_ATTR_SIZE];

	if (info == NULL) {
		ret = PS_BOGUS_DEVICE_NAME;
		goto errret;
	}

	// get the device number 
	if (((entry.pathlen = strlen(info->name)) == 0)
		|| (entry.pathlen >= MAXPATHLEN))
	{
		ret = PS_BOGUS_PATH_LEN;
		goto errret;
	}

	memset(entry.path, 0, sizeof(entry.path));
	strncpy(entry.path, info->name, entry.pathlen);
	entry.num_lrdb_bits = info->num_lrdb_bits;
	entry.num_hrdb_bits = info->num_hrdb_bits;
	entry.num_sectors = info->num_sectors;

	// open the store
	if ((fd = ftd_open(ps_name, O_RDWR | O_SYNC, 0)) == INVALID_HANDLE_VALUE) {
		ret = PS_BOGUS_PS_NAME;
		goto errret;
	}

	// seek to the header location
	ftd_llseek(fd, PS_HEADER_OFFSET*1024, SEEK_SET);

	// read the header 
	ftd_read(fd, &hdr, sizeof(ps_hdr_t));
	if (hdr.magic != PS_VERSION_1_MAGIC) {
        ret = PS_BOGUS_HEADER;
        reporterr(ERRFAC, M_PSBADHDR, ERRWARN, ps_name);
		goto errret;
	}

	// make sure the buffer is not too big 
	if (lrdb_len > (int)hdr.data.ver1.lrdb_size) {
		ret = PS_BOGUS_BUFFER_LEN;
		goto errret;
	}

	// make sure the buffer is not too big 
	if (hrdb_len > (int)hdr.data.ver1.hrdb_size) {
		ret = PS_BOGUS_BUFFER_LEN;
		goto errret;
	}

	table_size = BLOCK_SIZE(sizeof(ps_dev_entry_t) * hdr.data.ver1.max_dev);
	if ((table = (ps_dev_entry_t *)malloc(table_size)) == NULL) {
		ret = PS_MALLOC_ERROR;
		goto errret;
	}

	// get the device index array 
	if (ftd_llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
		ret = PS_SEEK_ERROR;
		goto errret;
	}

	if (ftd_read(fd, table, table_size) != (int)table_size) {
		ret = PS_READ_ERROR;
		goto errret;
	}

	// if device already exists, blow it off 
	for (i = 0; i < (int)hdr.data.ver1.max_dev; i++) {
		if ((table[i].pathlen == entry.pathlen) &&
			(strncmp(table[i].path, entry.path, entry.pathlen) == 0)) {
            ret = PS_OK;
            goto errret;
		}
	}

	// search for the device number 
	for (i = 0; i < (int)hdr.data.ver1.max_dev; i++) {
		if (table[i].pathlen == 0) {

			table[i] = entry;

			if (ftd_llseek(fd, hdr.data.ver1.dev_table_offset * 1024, SEEK_SET) == (ftd_uint64_t)-1) {
				ret = PS_SEEK_ERROR;
				goto errret;
			}
			if (ftd_write(fd, (caddr_t)table, table_size) != (int)table_size) {
				ret = PS_READ_ERROR;
				goto errret;
			}

			// increment the number of devices
			hdr.data.ver1.num_device++;
			if (i > hdr.data.ver1.last_device) {
				hdr.data.ver1.last_device = i;
			}

			// rewrite the header 
			ftd_llseek(fd, PS_HEADER_OFFSET * 1024, SEEK_SET);
			if (ftd_write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
				ret = PS_WRITE_ERROR;
				goto errret;
			}

			// write the low-res bitmap 
			offset = (hdr.data.ver1.lrdb_offset * 1024) +
				(i * hdr.data.ver1.lrdb_size);

			if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
				ret = PS_SEEK_ERROR;
				goto errret;
			}

			if (ftd_write(fd, lrdb, lrdb_len) != lrdb_len) {
				ret = PS_WRITE_ERROR;
				goto errret;
			}

			// write the high-res bitmap 
			offset = (hdr.data.ver1.hrdb_offset * 1024) +
				(i * hdr.data.ver1.hrdb_size);

			if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
				ret = PS_SEEK_ERROR;
				goto errret;
			}

			if (ftd_write(fd, hrdb, hrdb_len) != hrdb_len) {
				ret = PS_WRITE_ERROR;
				goto errret;
			}

			// write the device attributes
			offset = (hdr.data.ver1.dev_attr_offset * 1024) +
				(i * hdr.data.ver1.dev_attr_size);
			
			if (ftd_llseek(fd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
				ret = PS_SEEK_ERROR;
				goto errret;
			}

			sprintf(buffer, "DEFAULT_DEVICE_ATTRIBUTES: NONE\n");
			buf_len = sizeof(buffer);

			if (ftd_write(fd, buffer, buf_len) != buf_len) {
				ret = PS_WRITE_ERROR;
				goto errret;
			}

			break;
		}
	}

	if (i == (int)hdr.data.ver1.max_dev) {
		ret = PS_NO_ROOM;
	}

	if (table) {
		free(table);
	}
	
	if (fd != INVALID_HANDLE_VALUE) {
		FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
	}

	return ret;

errret:
	
	if (table) {
		free(table);
	}

	if (fd != INVALID_HANDLE_VALUE) {
		FTD_CLOSE_FUNC(__FILE__,__LINE__,fd);
	}
    
	return ret;
}
