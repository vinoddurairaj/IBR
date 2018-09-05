#ifndef FTDIO_H
#define FTDIO_H

#include "ftd_kern_ctypes.h"


#if defined(SOLARIS)

#include <sys/ioccom.h>
#include <sys/types.h>

#define FTDERRNO(a) a

#elif defined(HPUX)

#include <sys/ioctl.h>

#define FTDERRNO(a) (1000 - (a))

#define MAXMIN 0xffffff

#elif defined(_AIX)

#define MAXMIN 0xffff
#include <sys/ioctl.h>

#endif

#if defined(SOLARIS) && (SYSVERS >= 570)
#define FTD_MAXMIN MAXMIN32
#else  /* defined(SOLARIS) */
#define FTD_MAXMIN MAXMIN
#endif /* defined(SOLARIS) */

/* journal fs parameters */
#if !defined(MAXJRNL)
/* upper bound on journal fs file size */
#define MAXJRNL (512 * 1024 * 1024)
#endif /* !defined(MAXJRNL) */

#if defined(_AIX) && defined(_KERNEL)
#include "ftd_timers.h"
#endif /* defined(_AIX) */

#define DATASTAR_MAJIC_NUM  0x0123456789abcdefULL
#define FTD_OFFSET       16

typedef ftd_int32_t ftd_unknown_t;	/* FIXME XXX */

#define FTD_CMD 'q'


/* 
 * AIX IOC's use high order bits to encode
 * in/out-ness, generating much compiler 
 * grief. our kernel code needs only see 
 * the IOC parms, not the rest of the
 * information encoded in the IOC number.
 */
#if defined(_AIX)
#define IOC_CST(c) ((c) & 0xff)
#else /* defined(_AIX) */
#define IOC_CST(c) ((c))
#endif /* defined(_AIX) */
#define FTD_INIT_STOP            _IOW(FTD_CMD, 0x29, ftd_dev_t_t)

/* This is the header for a writelog entry */
typedef struct wlheader_s
  {
    ftd_uint64_t majicnum;	/* somebody doesn't know how to spell */
    ftd_uint32_t offset;	/* in sectors */
    ftd_uint32_t length;	/* in sectors */

    ftd_time_t timestamp;
    ftd_dev_t_t dev;		/* device id */
    ftd_dev_t_t diskdev;	/* disk device id */
    ftd_int32_t flags;		/* flags */
    ftd_int32_t complete;	/* I/O completion flag */
#if defined(_AIX)
    struct trb *timoid;		/* sync mode timeout id */
#else /* defined(_AIX) */

#if defined(SOLARIS) 
    union {
      ftd_timeout_id_t timoid_sol7; /* type of timeout(9f), apres SunOS 2.7 */
      ftd_int32_t timoid_pre_sol7;  /* type of timeout(9f), aprior SunOS 2.7 */
    } timo_u;
#else  /* defined(SOLARIS) */
    ftd_int32_t timoid;		/* sync mode timeout id */
#endif /* defined(SOLARIS) */

#endif /* defined(_AIX) */

#ifdef KERNEL
    ftd_uint64ptr_t group_ptr;	/* internal use only */
    ftd_uint64ptr_t bp;		/* bp to call biodone on migrate */
#else
    ftd_uint64ptr_t opaque1;	/* Use these and you'll hate life */
    ftd_uint64ptr_t opaque2;	/* and we won't care!!!! */
#endif

#if defined(HPUX) 
    ftd_uint64ptr_t syncForw;	/* head pointer in sync mode */
    ftd_uint64ptr_t syncBack;	/* tail pointer ini sync mode */
    ftd_int32_t syncTimeComplete;
#endif /* defined(HPUX)  */
    ftd_int32_t  span;                 /* # of fragments for this block */
  }
wlheader_t;

/* dki func timeout(9f) return type changed twixt 5.6 and 5.7 */
#if defined(SOLARIS) 
#if (SYSVERS >= 570)
#define timoid timo_u.timoid_sol7
#define INVALID_TIMEOUT_ID (ftd_timeout_id_t)0
#else  /* defined(SOLARIS) && (SYSVERS >= 570) */
#define timoid timo_u.timoid_pre_sol7
#define INVALID_TIMEOUT_ID -1
#endif
#endif /* defined(SOLARIS) */

#if defined(HPUX) 
#define INVALID_TIMEOUT_ID -1
#elif defined(_AIX)
#define INVALID_TIMEOUT_ID (struct trb *)0
#endif /* defined(_AIX) */

typedef struct disk_stats_s
  {
    ftd_dev_t_t localbdisk;	/* block device number */
#if defined(HPUX)
    ftd_dev_t_t localcdisk;	/* character device number */
#endif
    ftd_uint32_t localdisksize;

    ftd_uint32_t wlentries;
    ftd_uint32_t wlsectors;

    ftd_uint64_t readiocnt;
    ftd_uint64_t writeiocnt;
    ftd_uint64_t sectorsread;
    ftd_uint64_t sectorswritten;
  }
disk_stats_t;

typedef struct ftd_stat_s
  {
    /* Statistics */
    ftd_uint32_t loadtimesecs;	/* in seconds */
    ftd_uint32_t loadtimesystics;	/* in sys ticks */
    ftd_uint32_t wlentries;
    ftd_uint32_t wlsectors;
    ftd_int32_t bab_free;
    ftd_int32_t bab_used;
    ftd_int32_t state;
    ftd_int32_t ndevs;
    ftd_uint32_t sync_depth;
    ftd_uint32_t sync_timeout;
    ftd_uint32_t iodelay;
  }
ftd_stat_t;

/* Struct for ENTRIES_HAVE_MIGRATED */
typedef struct migrated_entries_s
  {
    ftd_uint32_t bytes;		/* Number of bytes migrated */
  }
migrated_entries_t;

/* info needed to add a logical group */
typedef struct ftd_lg_info_s
  {
    ftd_dev_t_t lgdev;
    ftd_dev_t_t persistent_store;
    ftd_int32_t statsize;	/* number of bytes in the statistics buffer */
  }
ftd_lg_info_t;

typedef struct ftd_devnum_s
  {
    ftd_dev_t_t b_major;
    ftd_dev_t_t c_major;
  }
ftd_devnum_t;

/* info needed to add a device to a logical group */
typedef struct ftd_dev_info_s
  {
    ftd_dev_t_t lgnum;
    ftd_dev_t_t localcdev;	/* dev_t for raw disk device */
    ftd_dev_t_t cdev;		/* dev_t for raw ftd disk device */
    ftd_dev_t_t bdev;		/* dev_t for block ftd disk device */
    ftd_uint32_t disksize;
    ftd_int32_t lrdbsize32;	/* number of 32-bit words in LRDB */
    ftd_int32_t hrdbsize32;	/* number of 32-bit words in HRDB */
    ftd_int32_t lrdb_res;	/* LRDB resolution */
    ftd_int32_t hrdb_res;	/* HRDB resolution */
    ftd_int32_t lrdb_numbits;	/* number of valid bits in LRDB */
    ftd_int32_t hrdb_numbits;	/* number of valid bits in HRDB */
    ftd_int32_t statsize;	/* number of bytes in the statistics buffer */
    ftd_uint32_t lrdb_offset;	/* Where to place the lrdb in pstore */
  }
ftd_dev_info_t;

/* structure for GET_OLDEST_ENTRIES */
typedef struct oldest_entries_s
  {
    ftd_uint64ptr_t addr;	/* address of buffer */
    ftd_uint32_t offset32;	/* offset from tail to begin copying in 32-bit */
    ftd_uint32_t len32;		/* length of buffer in 32-bit words */
    ftd_uint32_t retlen32;	/* RETURNED: length of data returned */
    ftd_int32_t state;		/* RETURNED: state of the group */
  }
oldest_entries_t;

/*
 * stat buffer descriptor
 */
typedef struct stat_buffer_s
  {
    ftd_dev_t_t lg_num;		/* logical group device id */
    ftd_dev_t_t dev_num;	/* device id. used only for GET_DEV_STATS */
    ftd_int32_t len;		/* length in bytes */
    ftd_uint64ptr_t addr;	/* pointer to the buffer */
  }
stat_buffer_t;

/* FIXME: This needs work. Add more info about each device, especially
   the bitmap info. */
#if !defined(_KERNEL)
typedef struct dirtybit_array_s
  {
    dev_t *devs;	        /* add more here */
    ftd_uint64ptr_t dbbuf;	/* the bitmap data */
    ftd_int32_t numdevs;
    ftd_int32_t dblen32;
    ftd_int32_t state_change;	/* 1 -> do automatic state change, 0 -> don't */
  }
dirtybit_array_t;
#endif /* !defined(_KERNEL) */

/* what's passed to the kernel, see rsync.c:dbarr_ioctl */
typedef struct dirtybit_array_kern_s
  {
    ftd_uint32_t    dev;	
    ftd_uint64ptr_t dbbuf;	
    ftd_int32_t dblen32;
    ftd_int32_t state_change;	
  }
dirtybit_array_kern_t;

/* a generic buffer passed to the kernel. Contents determined by context */
typedef struct buffer_desc_s
  {
    ftd_uint64ptr_t addr;	/* a user-mode address */
    ftd_int32_t len32;		/* number of 32 bit words in addr */
  }
buffer_desc_t;

/* used to change the state of a device */
typedef struct ftd_state_s
  {
    ftd_dev_t_t lg_num;
    ftd_int32_t state;
  }
ftd_state_t;

/*
 * The kernel assigned device numbers for the driver
 */
typedef struct ftd_dev_num_s
  {
    ftd_dev_t_t b_major;
    ftd_dev_t_t c_major;
  }
ftd_dev_num_t;

typedef struct ftd_param_s
  {
    ftd_int32_t lgnum;
    ftd_int32_t value;
  }
ftd_param_t;

#define DRVNAME QNM

#if defined(HPUX)
#define FTD_CTLDEV      "/dev/"QNM"/ctl"
#elif defined(SOLARIS)
#define FTD_CTLDEV      "/devices/pseudo/"QNM"@0:ctl"
#elif defined(_AIX)
#define FTD_DEV_DIR	"/dev/"QNM
#define FTD_CTLDEV	FTD_DEV_DIR"/ctl"
#endif

/* the first stab at logical group states */
#define FTD_M_JNLUPDATE 0x01
#define FTD_M_BITUPDATE 0x02

#define FTD_MODE_PASSTHRU  0x10
#define FTD_MODE_NORMAL    FTD_M_JNLUPDATE
#define FTD_MODE_TRACKING  FTD_M_BITUPDATE
#define FTD_MODE_REFRESH   (FTD_M_JNLUPDATE | FTD_M_BITUPDATE)
#define FTD_MODE_BACKFRESH 0x20

#define FTD_LGFLAG              ((FTD_MAXMIN + 1) >> 1)

#define FTD_CTL                 FTD_MAXMIN

/* calculate the size of "x" in 64-bit words */
#define sizeof_64bit(x) ((sizeof(x) + 7) >> 3)

#define FTD_WLH_QUADS(hp)       (sizeof_64bit(wlheader_t) + \
                                ((hp)->length << (DEV_BSHIFT - 3)))
#define FTD_LEN_QUADS(hp)       ((hp)->length << (DEV_BSHIFT - 3))
#define FTD_WLH_BYTES(hp)       FTD_WLH_QUADS(hp) << 3;

#define FTD_GET_CONFIG           _IOR(FTD_CMD,  0x01, ftd_unknown_t)
#define FTD_NEW_DEVICE           _IOW(FTD_CMD,  0x02, ftd_dev_info_t)
#define FTD_NEW_LG               _IOW(FTD_CMD,  0x03, ftd_lg_info_t)
#define FTD_DEL_DEVICE           _IOW(FTD_CMD,  0x04, ftd_dev_t_t)
#define FTD_DEL_LG               _IOW(FTD_CMD,  0x05, ftd_dev_t_t)
#define FTD_CTL_CONFIG           _IOW(FTD_CMD,  0x06, ftd_unknown_t)
#define FTD_GET_DEV_STATE_BUFFER _IOWR(FTD_CMD, 0x07, stat_buffer_t)
#define FTD_GET_LG_STATE_BUFFER  _IOWR(FTD_CMD, 0x08, stat_buffer_t)
#define FTD_SEND_LG_MESSAGE      _IOW(FTD_CMD,  0x09, stat_buffer_t)
#define FTD_OLDEST_ENTRIES       _IOWR(FTD_CMD, 0x0a, oldest_entries_t)
#define FTD_MIGRATE              _IOW(FTD_CMD,  0x0b, migrated_entries_t)
#define FTD_GET_LRDBS            _IOWR(FTD_CMD, 0x0c, dirtybit_array_kern_t)
#define FTD_GET_HRDBS            _IOWR(FTD_CMD, 0x0d, dirtybit_array_kern_t)
#define FTD_SET_LRDBS            _IOWR(FTD_CMD, 0x0e, dirtybit_array_kern_t)
#define FTD_SET_HRDBS            _IOWR(FTD_CMD, 0x0f, dirtybit_array_kern_t)
/*#if defined(DEPRECATED)*/								  /*PB: for tracing, we -1 from now in the array */
#define FTD_GET_LRDB_INFO        _IOWR(FTD_CMD, 0x10, dirtybit_array_kern_t)
#define FTD_GET_HRDB_INFO        _IOWR(FTD_CMD, 0x11, dirtybit_array_kern_t)
/*#endif  defined(DEPRECATED) */
#define FTD_GET_DEVICE_NUMS      _IOWR(FTD_CMD, 0x12, ftd_devnum_t)
#define FTD_SET_DEV_STATE_BUFFER _IOWR(FTD_CMD, 0x13, stat_buffer_t)
#define FTD_SET_LG_STATE_BUFFER  _IOWR(FTD_CMD, 0x14, stat_buffer_t)
#define FTD_START_LG             _IOW(FTD_CMD,  0x15, ftd_dev_t_t)
/*#if defined(DEPRECATED)*/
#define FTD_GET_NUM_DEVICES      _IOWR(FTD_CMD, 0x16, ftd_int32_t)
#define FTD_GET_NUM_GROUPS       _IOWR(FTD_CMD, 0x17, ftd_int32_t)
/*#endif defined(DEPRECATED) */
#define FTD_GET_DEVICES_INFO     _IOW(FTD_CMD,  0x18, stat_buffer_t)
#define FTD_GET_GROUPS_INFO      _IOW(FTD_CMD,  0x19, stat_buffer_t)
#define FTD_GET_DEVICE_STATS     _IOW(FTD_CMD,  0x1a, stat_buffer_t)
#define FTD_GET_GROUP_STATS      _IOW(FTD_CMD,  0x1b, stat_buffer_t)
#define FTD_SET_GROUP_STATE      _IOWR(FTD_CMD, 0x1c, ftd_state_t)
#define FTD_UPDATE_DIRTYBITS     _IO(FTD_CMD,   0x1d)
#define FTD_CLEAR_BAB            _IOWR(FTD_CMD, 0x1e, ftd_dev_t_t)
#define FTD_CLEAR_HRDBS          _IOWR(FTD_CMD, 0x1f, ftd_dev_t_t)
#define FTD_CLEAR_LRDBS          _IOWR(FTD_CMD, 0x20, ftd_dev_t_t)
#define FTD_GET_GROUP_STATE      _IOWR(FTD_CMD, 0x21, ftd_state_t)
#define FTD_GET_BAB_SIZE         _IOWR(FTD_CMD, 0x22, ftd_int32_t)
#define FTD_UPDATE_LRDBS         _IOW(FTD_CMD,  0x23, ftd_dev_t_t)
#define FTD_UPDATE_HRDBS         _IOW(FTD_CMD,  0x24, ftd_dev_t_t)
#define FTD_SET_SYNC_DEPTH       _IOW(FTD_CMD,  0x25, ftd_param_t)
#define FTD_SET_IODELAY          _IOW(FTD_CMD,  0x26, ftd_param_t)
#define FTD_SET_SYNC_TIMEOUT     _IOW(FTD_CMD,  0x27, ftd_param_t)
#define FTD_CTL_ALLOC_MINOR      _IOWR(FTD_CMD, 0x28, ftd_int32_t)
#if defined(HPUX)
/* We need a way to force a panic on HP for debugging */
#define FTD_PANIC		 		_IOW(FTD_CMD,   0x29, ftd_param_t)
#endif

#endif /* FTDIO_H */
