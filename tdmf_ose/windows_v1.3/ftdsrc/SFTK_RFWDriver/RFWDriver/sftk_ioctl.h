/*
 * ftdio.h - FTD user/kernel interface
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
#ifndef FTDIO_H
#define FTDIO_H

#define FTD_DRIVER_ERROR_CODE   0xE0000000L
#define FTD_IOCTL_INDEX         0x0800

#ifdef _IOR
#undef _IOR
#endif

#ifdef _IOW
#undef _IOW
#endif

#ifdef _IO
#undef _IO
#endif

#define _IOR(cmd, index, data) CTL_CODE( FILE_DEVICE_UNKNOWN, FTD_IOCTL_INDEX + index,  \
        METHOD_BUFFERED, FILE_READ_ACCESS )
#define _IOW(cmd, index, data)CTL_CODE( FILE_DEVICE_UNKNOWN, FTD_IOCTL_INDEX + index,   \
        METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define _IOWR(cmd, index, data)CTL_CODE( FILE_DEVICE_UNKNOWN, FTD_IOCTL_INDEX + index,  \
        METHOD_BUFFERED, FILE_ANY_ACCESS )
#define _IO(cmd, index)CTL_CODE( FILE_DEVICE_UNKNOWN, FTD_IOCTL_INDEX + index,  \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

#define MAXMIN 0xffffff

#define DATASTAR_MAJIC_NUM  0x0123456789abcdefUL

#define FTD_OFFSET       16

typedef int ftd_unknown_t;      /* FIXME XXX */

#define FTD_CMD 'q'

#define FTD_GET_CONFIG           _IOR(FTD_CMD,  0x1, ftd_unknown_t)
#define FTD_NEW_DEVICE           _IOW(FTD_CMD,  0x2, ftd_dev_info_t)
#define FTD_NEW_LG               _IOW(FTD_CMD,  0x3, ftd_lg_info_t)
#define FTD_DEL_DEVICE           _IOW(FTD_CMD,  0x4, dev_t)
#define FTD_DEL_LG               _IOW(FTD_CMD,  0x5, dev_t)
#define FTD_CTL_CONFIG           _IOW(FTD_CMD,  0x6, ftd_unknown_t)
#define FTD_GET_DEV_STATE_BUFFER _IOWR(FTD_CMD, 0x7, stat_buffer_t)
#define FTD_GET_LG_STATE_BUFFER  _IOWR(FTD_CMD, 0x8, stat_buffer_t)
#define FTD_SEND_LG_MESSAGE      _IOW(FTD_CMD,  0x9, stat_buffer_t)
#define FTD_OLDEST_ENTRIES       _IOWR(FTD_CMD, 0xa, oldest_entries_t)
#define FTD_MIGRATE              _IOW(FTD_CMD,  0xb, migrated_entries_t)
#define FTD_GET_LRDBS            _IOWR(FTD_CMD, 0xc, dirtybit_array_t)
#define FTD_GET_HRDBS            _IOWR(FTD_CMD, 0xd, dirtybit_array_t)
#define FTD_SET_LRDBS            _IOWR(FTD_CMD, 0xe, dirtybit_array_t)
#define FTD_SET_HRDBS            _IOWR(FTD_CMD, 0xf, dirtybit_array_t)
#define FTD_GET_LRDB_INFO        _IOWR(FTD_CMD, 0x11, dirtybit_array_t)
#define FTD_GET_HRDB_INFO        _IOWR(FTD_CMD, 0x12, dirtybit_array_t)
#define FTD_GET_DEVICE_NUMS      _IOWR(FTD_CMD, 0x13, ftd_devnum_t)
#define FTD_SET_DEV_STATE_BUFFER _IOWR(FTD_CMD, 0x14, stat_buffer_t)
#define FTD_SET_LG_STATE_BUFFER  _IOWR(FTD_CMD, 0x15, stat_buffer_t)
#define FTD_START_LG             _IOW(FTD_CMD,  0x16, dev_t)
#define FTD_GET_NUM_DEVICES      _IOWR(FTD_CMD, 0x17, int)
#define FTD_GET_NUM_GROUPS       _IOWR(FTD_CMD, 0x18, int)
#define FTD_GET_DEVICES_INFO     _IOW(FTD_CMD,  0x19, stat_buffer_t)
#define FTD_GET_GROUPS_INFO      _IOW(FTD_CMD,  0x1a, stat_buffer_t)
#define FTD_GET_DEVICE_STATS     _IOW(FTD_CMD,  0x1b, stat_buffer_t)
#define FTD_GET_GROUP_STATS      _IOW(FTD_CMD,  0x1c, stat_buffer_t)
#define FTD_SET_GROUP_STATE      _IOWR(FTD_CMD, 0x1d, ftd_state_t)
#define FTD_UPDATE_DIRTYBITS     _IO(FTD_CMD,   0x1e)
#define FTD_CLEAR_BAB            _IOWR(FTD_CMD, 0x1f, dev_t)
#define FTD_CLEAR_HRDBS          _IOWR(FTD_CMD, 0x20, dev_t)
#define FTD_CLEAR_LRDBS          _IOWR(FTD_CMD, 0x21, dev_t)
#define FTD_GET_GROUP_STATE      _IOWR(FTD_CMD, 0x22, ftd_state_t)
#define FTD_GET_BAB_SIZE         _IOWR(FTD_CMD, 0x23, int)
#define FTD_UPDATE_LRDBS         _IOW(FTD_CMD,  0x24, dev_t)
#define FTD_UPDATE_HRDBS         _IOW(FTD_CMD,  0x25, dev_t)
#define FTD_SET_SYNC_DEPTH       _IOW(FTD_CMD,  0x26, ftd_param_t)
#define FTD_SET_IODELAY          _IOW(FTD_CMD,  0x27, ftd_param_t)
#define FTD_SET_SYNC_TIMEOUT     _IOW(FTD_CMD,  0x28, ftd_param_t)
#define FTD_SET_TRACE_LEVEL      _IOW(FTD_CMD,  0x29, int)
#define FTD_INIT_STOP            _IOW(FTD_CMD,  0x30, dev_t)

/* 
 * AIX IOC's use high order bits to encode
 * in/out-ness, generating much compiler 
 * grief. our kernel code needs only see 
 * the IOC parms, not the rest of the
 * information encoded in the IOC number.
 */
#define IOC_CST(c) ((c))

typedef unsigned __int64    ftd_uint64_t;
typedef __int64             ftd_int64_t;

#pragma pack(4)

/* This is the header for a writelog entry */
typedef struct wlheader_s {
    ftd_uint64_t    majicnum;           /* somebody doesn't know how to spell */
    unsigned int    offset;             /* in sectors */
    unsigned int    length;             /* in sectors */
    time_t          timestamp;
    dev_t           dev;                /* device id */
    dev_t           diskdev;            /* disk device id */
    int             flags;              /* flags */
    int             complete;           /* I/O completion flag */
    int             timoid;             /* sync mode timeout id */
#ifdef KERNEL
    void            *group_ptr;         /* internal use only */
    IRP             *bp;                /* IRP to complete on migrate */
#else
    void            *opaque1;           /* Use these and you'll hate life */
    void            *opaque2;           /* and we won't care!!!! */
#endif
} wlheader_t;

typedef struct disk_stats_s {
    char            devname[64];
    dev_t           localbdisk ; /* block device number */
    unsigned long   localdisksize ;

    int             wlentries ;
    int             wlsectors ;

    ftd_uint64_t    readiocnt;
    ftd_uint64_t    writeiocnt;
    ftd_uint64_t    sectorsread;
    ftd_uint64_t    sectorswritten;
} disk_stats_t ;

typedef struct ftd_stat_s {
    int                   lgnum;
    int                   connected;
    /* Statistics */
    unsigned long       loadtimesecs ;         /* in seconds */
    unsigned long       loadtimesystics ;        /* in sys ticks */
    int                 wlentries ;
    int                 wlsectors ;
    int                 bab_free;
    int                 bab_used;
    int                 state;
    int                 ndevs;
    unsigned int        sync_depth;
    unsigned int        sync_timeout;
    unsigned int        iodelay;
} ftd_stat_t;

/* Struct for ENTRIES_HAVE_MIGRATED */
typedef struct migrated_entries_s {
    unsigned long   bytes;      /* Number of bytes migrated */
} migrated_entries_t ;

/* info needed to add a logical group */
typedef struct ftd_lg_info_s {
    dev_t        lgdev;
    char         *vdevname;
    int          statsize;   /* number of bytes in the statistics buffer */
} ftd_lg_info_t;

typedef struct ftd_devnum_s {
    dev_t b_major;
    dev_t c_major;
} ftd_devnum_t;

/* info needed to add a device to a logical group */
typedef struct ftd_dev_info_s {
    int          lgnum;
    dev_t        localcdev;  /* dev_t for raw disk device */
/* FIXME: I think that we should just be passing in a minor number here wl */
    dev_t        cdev;       /* dev_t for raw ftd disk device */
    dev_t        bdev;       /* dev_t for block ftd disk device */
    unsigned int disksize;
    int          lrdbsize32; /* number of 32-bit words in LRDB */
    int          hrdbsize32; /* number of 32-bit words in HRDB */
    int          lrdb_res;   /* LRDB resolution */
    int          hrdb_res;   /* HRDB resolution */
    int          lrdb_numbits; /* number of valid bits in LRDB */
    int          hrdb_numbits; /* number of valid bits in HRDB */
    int          statsize;   /* number of bytes in the statistics buffer */
    unsigned int lrdb_offset;   /* Where to place the lrdb in pstore */
    char         *devname;      /* in only */
    char         *vdevname;     /* in only */
} ftd_dev_info_t;

/* structure for GET_OLDEST_ENTRIES */
typedef struct oldest_entries_s {
    int          *addr;     /* address of buffer */
    unsigned int offset32;  /* offset from tail to begin copying in 32-bit */
    unsigned int len32;     /* length of buffer in 32-bit words */
    unsigned int retlen32;  /* RETURNED: length of data returned */
    int          state;     /* RETURNED: state of the group */
} oldest_entries_t;

/*
 * stat buffer descriptor
 */
typedef struct stat_buffer_t {
    dev_t lg_num;  /* logical group device id */
    dev_t dev_num; /* device id. used only for GET_DEV_STATS */
    int   len;     /* length in bytes */
    char  *addr;   /* pointer to the buffer */
} stat_buffer_t;

/* FIXME: This needs work. Add more info about each device, especially
   the bitmap info. */
typedef struct dirtybit_array_t {
    int   numdevs;
    int   dblen32;
    int   state_change; /* 1 -> do automatic state change, 0 -> don't */
    dev_t *devs;  /* add more crap here */
    int   *dbbuf; /* the bitmap data */
} dirtybit_array_t;

/* a generic buffer passed to the kernel. Contents determined by context */
typedef struct buffer_desc_s {
    int *addr;  /* a user-mode address */
    int len32;  /* number of 32 bit words in addr */
} buffer_desc_t;

/* used to change the state of a device */
typedef struct ftd_state_s {
    dev_t lg_num;
    int   state;
} ftd_state_t;

/*
 * The kernel assigned device numbers for the driver
 */
typedef struct ftd_dev_num_s {
    int b_major;
    int c_major;
} ftd_dev_num_t;

typedef struct ftd_param_s {
    int lgnum;
    int value;
} ftd_param_t;

#pragma pack()

// DTurrin - Nov 7th, 2001
// If you are using Windows 2000 or later,
// you need to add the Global\ prefix so that
// client session will use kernel objects in
// the server's global name space.
// #define NTFOUR
#ifdef NTFOUR
#define FTD_DEV_DIR "\\\\.\\DTC"
#else // #ifdef NTFOUR
#define FTD_DEV_DIR "\\\\.\\Global\\DTC"
#endif // #ifdef NTFOUR
// #undef NTFOUR

#define FTD_CTLDEV  FTD_DEV_DIR "\\ctl"

/* the first stab at logical group states */
#define FTD_M_JNLUPDATE 0x01
#define FTD_M_BITUPDATE 0x02

#define FTD_MODE_PASSTHRU  0x10
#define FTD_MODE_NORMAL    FTD_M_JNLUPDATE
#define FTD_MODE_TRACKING  FTD_M_BITUPDATE
#define FTD_MODE_REFRESH   (FTD_M_JNLUPDATE | FTD_M_BITUPDATE)
#define FTD_MODE_BACKFRESH 0x20
#define FTD_MODE_FULLREFRESH    (0x40 | FTD_M_BITUPDATE)    //used only in Unix


#define FTD_LGFLAG              ((MAXMIN + 1) >> 1)

#define FTD_CTL                 MAXMIN

/* calculate the size of "x" in 64-bit words */
#define sizeof_64bit(x) ((sizeof(x) + 7) >> 3)

#define FTD_WLH_QUADS(hp)       (sizeof_64bit(wlheader_t) + \
                                ((hp)->length << (DEV_BSHIFT - 3)))
#define FTD_WLH_BYTES(hp)       FTD_WLH_QUADS(hp) << 3;

#endif /* FTDIO_H */
