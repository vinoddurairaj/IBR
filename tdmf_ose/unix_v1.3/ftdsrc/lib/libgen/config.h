/***************************************************************************
 * config.h - DataStar Primary / Remote Mirror Daemon configuration file
 *            datatypes
 *
 * (c) Copyright 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved
 *
 *
 * History:
 *   11/11/96 - Steve Wahl - original code
 *
 ***************************************************************************
 */

#ifndef _CONFIG_H
#define _CONFIG_H 1

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include "throttle.h"
#include "ftdio.h"


#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#define EXITNORMAL      0
#define EXITRESTART     1
#define EXITANDDIE      2
#define EXITNETWORK     3

#define ROLEPRIMARY 0
#define ROLESECONDARY 1

#define ISPRIMARY(s) (((s) != NULL) && ((s) == &sys[0]))
#define ISSECONDARY(s) (((s) != NULL) && ((s) == &sys[1]))

#define CHUNKSIZE       256 
#define STATINTERVAL    10
#define MAXSTATFILESIZE 64
#define SECONDARYPORT   575
#define PRIMARYPORT     575
#define TRACETHROTTLE   0
#define NETMAXKBPS      -1
#define SYNCMODE        0
#define SYNCMODEDEPTH   1
#define SYNCMODETIMEOUT 30
#define COMPRESSION     0
#define TCPWINDOWSIZE   256
#define CHUNKDELAY      0

#define FILE_PATH_LEN   256

/* checkpoint path prefixes */
#define PRE_CP_ON       "cp_pre_on_" 
#define PRE_CP_OFF      "cp_pre_off_" 
#define POST_CP_ON      "cp_post_on_" 
#define POST_CP_OFF     "cp_post_off_" 
 
#define CP_ON           'p'

/* MAX number of journal files */
#define MAXJRN		256

/* MAX filename */
#if defined(_AIX) && defined(MAXPATH) 
#undef MAXPATH
#endif /* defined(_AIX) && defined(MAXPATH) */
#define MAXPATH		256

/* MAX number of logical groups */
#define MAXLG	    1000	

#define DIGESTSIZE	16	
#define MINCHK	    32768 
#define MAXBITS		128*1024	

/* secondary journal states */
#define JRN_CO       1
#define JRN_INCO     2
#define JRN_CP       3

/* secondary journal modes */
#define MIR_ONLY     1
#define JRN_ONLY     2
#define JRN_AND_MIR  3

/* queue entry types */
#define Q_RSYNCRFD   1
#define Q_RSYNCBFD   2
#define Q_WRITERFD   3
#define Q_WRITERFDF  4
#define Q_WRITEBFD   5
#define Q_WRITE      6
#define Q_RFDCHKSUM  7
#define Q_WRITEZERO  8
#define Q_WRITEHRDB  9

/* device states for throttle tests */
#define DEV_JOURNAL    0
#define DEV_OVERFLOW   1
#define DEV_PASSTHRU   2

/* pmd states for throttle tests */
#define REFRESH        0
#define BACKFRESH      1
#define TRANSFER       2

/*states for throttle tests */
#define DRVR_NORMAL     0
#define DRVR_TRACKING   1
#define DRVR_PASSTHRU   2
#define DRVR_REFRESH    3
#define DRVR_BACKFRESH  4

/* initial size of dirty segment buffer */
#define INIT_DELTA_SIZE 8 
#if defined(_AIX) && defined(MAXSEG) 
#undef MAXSEG
#endif /* defined(_AIX) && defined(MAXSEG) */

#define MAXFILES 30000

/* pid -> cfgidx structure */
typedef struct ftdpid_s {
    pid_t pid;                     /* process id */
    int fd[2][2];                  /* pipes file descriptors */
} ftdpid_t;

/* mirror buffer structure  */
typedef struct qentry_s {
    int type;                       /* entry type - indicator for data */
    char *data;                     /* data buffer */
} qentry_t;

/* I/O queue */
typedef struct ioqueue_s {
    qentry_t *entry;
    long head;
    long tail;
    int full;
    int empty;
} ioqueue_t;

/* dirty bit map - dynamic size */
typedef struct dirtymap_s {
    int res;                        /* dirty bit resolution        */
    int len;                        /* dirty bit mask length       */
    int seglen;                     /* length of masked data       */
    u_int bits;                     /* number valid dirty bits     */
    u_int bits_used;                /* number dirty bits processed */
    u_char *mask;                   /* dirty bits                  */
} dirtymap_t;

/* checksum structure */
typedef struct checksum_s {
    int cnt;                        /* number of checksum lists    */
    int num;                        /* number of checksums in list */
    int dirtyoff;                   /* dirty offset */
    int dirtylen;                   /* dirty len accumulator */
    int segoff;                     /* data offset digest represents  */
    int seglen;                     /* data len digest represents  */
    int digestlen;                  /* digest buffer length        */
    char *digest;                   /* checksums/misc. data        */
} checksum_t;

/* checksum delta structure */
typedef struct ref_s {
    int offset;
    int length;
} ref_t;

/* checksum delta map structure */
typedef struct refmap_s {
    int size;                       
    int cnt;
    ref_t *refp;
} refmap_t;

/* rsync device structure - should go into network.h ???? */
typedef struct rsync_s {
    int devid;                      /* PMD/RMD device id           */
    int devfd;                      /* data device file descriptor */
    int size;                       /* total number of sectors     */
    off_t ackoff;                   /* ACKd sector offset          */
    off_t offset;                   /* committed sector            */
    int length;                     /* data length - sectors       */
    int delta;                      /* total sectors written       */
    int datalen;                    /* data buffer length - bytes  */
    int done;                       /* rsync - done flag           */
    char *data;                     /* device data buffer          */
    checksum_t cs;                  /* checksums                   */
    refmap_t deltamap;              /* offsets/lengths for deltas  */
} rsync_t;

typedef struct devstat_s {
    struct devstat_s *p;            /* previous device stats */ 
    struct devstat_s *n;            /* next device stats */
    int devid;                      /* device id */
    int a_tdatacnt;                 /* header+compr bytes from lastts */
    int a_datacnt;                  /* compr data bytes sent */
    int e_tdatacnt;                 /* header+uncompr bytes sent */
    int e_datacnt;                  /* uncompr data bytes sent */
    int entries;                    /* entries sent/recv */
    int entage;                     /* time between BAB write and recvd by RMD */
    int jrnentage;                  /* time between journal write and mirro write */
    off_t rfshoffset;               /* rsync sectors done */
    int rfshdelta;                  /* rsync sectors changed */
} devstat_t;

typedef struct devthrotstat_s {
    int entries;                    /* number of entries in bab for device */
    int sectors;                    /* number of sectors in bab for device */
    float actualkbps;               /* actual transfer rate */
    float effectkbps;               /* effective transfer rate w/compression */
    float pctdone;                  /* % of refresh or backfresh complete */
    float pctbab;                   /* % of bab used by this device */
    u_longlong_t dev_ctotread_sectors; /* current total no. sectors read */
    u_longlong_t dev_ctotwrite_sectors;/* current total no. sectors written */
    u_longlong_t dev_ptotread_sectors; /* previous total no. sectors read */
    u_longlong_t dev_ptotwrite_sectors;/* previous total no. sectors written */
    double local_kbps_read;          /* ftd device kbps read */
    double local_kbps_written;       /* ftd device kbps written */
} devthrotstat_t;

typedef struct lgthrotstat_s {
    int drvmode;                    /* driver mode */
    pid_t pid;
    int percentcpu;
    int pmdup;                      /* 1 if pmd is running 0 if not*/
    time_t statts;                  /* timestamp of last stats dump */
    float actualkbps;               /* kbps transfer rate */
    float effectkbps;               /* effective transfer rate w/compression */
    float pctdone;                  /* % of refresh or backfresh complete */
    int entries;                    /* # of entries in bab */
    int sectors;                    /* # of sectors in bab */
    int pctbab;                     /* pct of bab in use */
    double local_kbps_read;          /* ftd device kbps read */
    double local_kbps_written;       /* ftd device kbps written */
} lgthrotstat_t;

typedef struct devstat_ls {
    int numdevs;                    /* number of DataStar devices */
    devstat_t *head;                /* head of the device list */
    devstat_t *tail;                /* tail of the device list */
} devstat_l;

extern devstat_l g_statl;                  /* for dumping stats */

/* -- data structure per DataStar device */
typedef struct sddisk {
    struct sddisk *p;               /* previous disk profile definition */
    struct sddisk *n;               /* next disk profile definintion    */
    char remark[256];               /* remark describing the device     */
    char sddevname[FILE_PATH_LEN];  /* ftd device name    */
    char devname[FILE_PATH_LEN];    /* data device name (local disk)    */
    char mirname[FILE_PATH_LEN];    /* mirror name                      */ 
    int devid;                      /* device ID insofar as PMD/RMD com */
    dev_t dd_rdev;                  /* data device major/minor number */
    dev_t sd_rdev;                  /* ftd device major/minor number */
    int mirfd;                      /* device descriptor for data device*/
    int no0write;                   /* disallow write to sector 0 */
    u_long ts;                      /* timestamp of the current entry   */
    u_long offset;                  /* device offset (sectors)   */
    u_longlong_t devsize;           /* device size (bytes)   */
    u_longlong_t mirsize;           /* mirror size (bytes)   */
    char* data;                     /* actual data for the disk block   */
    rsync_t rsync;                  /* rsync device info                */
    rsync_t ackrsync;               /* ACK'd checksum state             */
    dirtymap_t dm;                  /* dirty-bit map for entire device  */
    devstat_t stat;                 /* statistics  */
    devthrotstat_t throtstats;      /* throttle test stats */
} sddisk_t;

#ifndef SDDISK_P
#define SDDISK_P 1
typedef struct sddisk* sddisk_p;
#endif

typedef struct jrnheader_s {
    u_long magicnum;
    int state;
    int mode;
} jrnheader_t;

/* -- data structure per DataStar Logical Group */
typedef struct group_s {
    char devname[FILE_PATH_LEN];    /* group name */  
    char journal_path[FILE_PATH_LEN]; /* secondary journal path */  
    char journal[FILE_PATH_LEN];    /* current journal name */  
    char pstore[FILE_PATH_LEN];
    char chkpntenterexec[FILE_PATH_LEN]; /* secondary chkpnt start script */  
    char chkpntexitexec[FILE_PATH_LEN];  /* secondary chkpnt stop script */  
    int devfd;                      /* logical group device handle */
    int jrnfd;                      /* secondary journal file descirptor */
    time_t lastackts;               /* timestamp of last ACK received */
    int chunkdatasize;              /* length of chunk buffer */
    char *chunk;                    /* chunk that we're working on */
    char *endofdata;                /* last address of data in chunk */
    char *data;                     /* actual data for the disk block */
    u_long ts;                      /* timestamp of the current entry */
    u_long blkts;                   /* timestamp that block was read */
    u_long size;                    /* length of bab chunk */
    u_long len;                     /* length of transient bab chunk */
    u_long offset;                  /* journal offset (bytes) */
    int babsize;                    /* total size of BAB - in sectors */
    int babused;                    /* total size of used BAB - in sectors */
    int babfree;                    /* total size of free BAB - in sectors */
    int ackseen;                    /* Zero until first ack request seen on con */
    int numsddisks;                 /* number of DataStar devices for group */
    sddisk_t *headsddisk;           /* head of the device list */
    sddisk_t *tailsddisk;           /* tail of the device list */
    lgthrotstat_t throtstats;       /* throttle test statistics */
    int sync_depth;                 /* How deep is the syncmode? */
    int chaining;                   /* 0 - open mirror excl */
} group_t;

typedef struct tunable_s {
    int chunksize;       /* TUNABLE: default - 256 Kbyte */
    int statinterval;    /* TUNABLE: default - 10 seconds */
    int maxstatfilesize; /* TUNABLE: default - 64 Kbyte */
    int tracethrottle;   /* TUNABLE: default - 0 */
    int syncmode;        /* TUNABLE: default - 0 -- Am I doing sync? */
    int	syncmodedepth;   /* TUNABLE: default - 1 deep queue */
    int syncmodetimeout; /* TUNABLE: default - 30 seconds */
    int compression;     /* TUNABLE: default - none */
    int tcpwindowsize;   /* TUNABLE: default - 256 Kbyte */
    int netmaxkbps;      /* TUNABLE: default - -1 */
    int chunkdelay;      
} tunable_t;

/* -- data structure per PMD and RMD on each system */
typedef struct machine {
    int         ctlfd;           /* control device */
    char        pstore[256];     /* persistent store path */
    FILE        *fd;             /* config file handle */
    time_t      mtime;           /* st_mtime from stat */
    char        notes[256];      /* notes describing this logical group */
    int         role;            /* Primary (or secondary) flag */
    char        configpath[FILE_PATH_LEN]; /* path to configuration file */
    char        tag[32];         /* tag to refer to this machine by */
    char        name[256];       /* fully qualified name of this machine */
    long        hostid;          /* hostid of this machine */
    int         procfd;          /* process file identifier for this process */
    int         pcpu;            /* current process cpu % utilization */
    int         pcpu_prev;       /* previous process cpu % utilization */
    long        entrysleep;      /* microseconds to sleep after entry sent */
    u_long      ip;              /* IP address of this machine */
    int         secondaryport;   /* public port for data communictations */
    int         sock;            /* socket for communicating to remote system */
    int         isconnected;     /* 1=connected to remote system */
    u_longlong_t lgsnmaster;     /* master lsgn value for group */
    time_t      tsbias;          /* time difference between primary/secondary */
    time_t      midnight;        /* timestamp for 00:00:00 today */
    int         monlastday;      /* last day of the month for this month */
    int         day;             /* current day of month 1-31 */
    int         wday;            /* current day of week 0-6 */
    int         lastdayofmon;    /* last day in this month */
    FILE*       statfd;          /* file descriptor of stat file */
    int         tkbps;           /* total network kbps for the logical group */
    int         prevtkbps;       /* previous network kbps for the logical grp */
    throttle_t  *throttles;      /* list of throttles to eval with stat update */
    group_t     group[1];        /* logical group */
    tunable_t   tunables;        /* tunable parameters */
} machine_t;

#ifdef DEBUG_THROTTLE
extern FILE*       throtfd;             /* throttle trace file */
#endif /* DEBUG_THROTTLE */

/* -- function prototypes for configuration file management -- */
extern void initconfigs (void);
extern int readconfig (int pmdflag, int pstoreflag, int openflag, char* configpath);
extern int stringcompare (const void* p1, const void* p2);
extern char* iptoa (u_long ip);
extern int isthisme (char* hostnodename, u_long ip);
extern int settunable (int lgnum, char *key, char *value);

/* -- function prototypes for IP address utilities */
extern int getnetconfcount(void);
extern void getnetconfs(u_long* ipaddrs);

extern daddr_t fdisksize(int fd);
extern daddr_t disksize(char *disk);

extern int parseftddevnum(char*);

extern int gettunables (char *group_name, int psflag, int quiet);

/* -- global variables for configuration file management -- */
/* -- tunable parameters */
extern int chunksize;
extern int statinterval;
extern int maxstatfilesize;
extern int rmtrmdport;
extern int locrmdport;
extern int syncmode;
extern int syncmodedepth;
extern int syncmodetimeout;

/* -- system description structures and pointers */
extern machine_t sys[2];
extern machine_t* mysys;
extern machine_t* othersys;
extern u_long* myips;   /* array of IP addresses for this machine */
extern int myipcount;   /* number of IP addresses this machine has */

#endif /* _CONFIG_H */
