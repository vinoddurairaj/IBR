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
#if !defined(linux)
#include <sys/types.h>
#else
#include <linux/types.h>
#endif /* !defined(linux) */
#include <sys/param.h>
#include "throttle.h"
#include "ftdio.h"
#include "ftd_trace.h"


#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#define EXITNORMAL      0
#define EXITRESTART     1
#define EXITANDDIE      2
#define EXITNETWORK     3

#if defined(DEBUG)
#define EXIT(code) (reporterr(ERRFAC, M_INTERNAL, ERRWARN, "EXIT(" #code ") [abort() if code EXITNETWORK or EXITANDDIE]"), \
                    (code) == EXITANDDIE || (code) == EXITNETWORK ? abort() : exit(code))
#else
#define EXIT(code) exit(code)
// NOTE: if you want to see the exit location (source and line number) for debugging
//       the exit points that don't log error messages, you can use the following
// #define EXIT(code) (reporterr(ERRFAC, M_GENMSG, ERRINFO, "<<< EXITING >>>" ), exit(code))
#endif
#define DEBUG_TRACE(a) reporterr( ERRFAC, M_GENMSG, ERRINFO, a )
#define DEBUG_VALUE(a) (sprintf( debug_msg, "<<< value of " #a " = %d", a ), reporterr( ERRFAC, M_GENMSG, ERRINFO, debug_msg ))
#define DEBUG_ADDRESS(a) (sprintf( debug_msg, "<<< adress of " #a " = 0x%p", &a ), reporterr( ERRFAC, M_GENMSG, ERRINFO, debug_msg ))
#define DEBUG_STRING(a) (sprintf( debug_msg, "<<< string " #a " = %s", a ), reporterr( ERRFAC, M_GENMSG, ERRINFO, debug_msg ))

#define NAMEIPNOTFOUND       0
#define NAMEIPFOUND          1
#define UNRESOLVNAME        -1
#define UNRESOLVIPADDR      -2
#define NAMEIPADDRCONFLICT  -3

#define ROLEPRIMARY 0
#define ROLESECONDARY 1

#define ISPRIMARY(s) (((s) != NULL) && ((s) == &sys[0]))
#define ISSECONDARY(s) (((s) != NULL) && ((s) == &sys[1]))

#define CHUNKSIZE       2048
#define STATINTERVAL    10
#define MAXSTATFILESIZE 1024 /*set default maxstatfilesize value to 1024 Kb*/  
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
#define LRT		1

#define FILE_PATH_LEN   256

/* checkpoint path prefixes */
#define PRE_CP_ON       "cp_pre_on_" 
#define PRE_CP_OFF      "cp_pre_off_" 
#define POST_CP_ON      "cp_post_on_" 
#define POST_CP_OFF     "cp_post_off_" 
 
#define CP_ON           'p'

/* MAX number of journal files */
#define MAXJRN		999		/* original define was 256 */

/* MAX filename */
#if defined(_AIX) && defined(MAXPATH) 
#undef MAXPATH
#endif /* defined(_AIX) && defined(MAXPATH) */
#define MAXPATH		256


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
#define DRVR_NORMAL      0
#define DRVR_TRACKING    1
#define DRVR_PASSTHRU    2
#define DRVR_REFRESH     3
#define DRVR_BACKFRESH   4
#define DRVR_NETANALYSIS 5

// Error codes for read_key_value_from_file()
#define KEY_OK                 0
#define KEY_FILE_NOT_FOUND    -1
#define KEY_FILE_OPEN_ERROR	  -2
#define KEY_MALLOC_ERROR      -3
#define KEY_READ_ERROR        -4
#define KEY_NOT_FOUND         -5

/* MAX TCP window buffer to 1MB from original 256KB (hardcoded in ftdd.c */
#define MAX_TCP_WINDOW_SIZE     1048576

#ifdef linux
    // Rely on linux's default auto tuning values by default.
    #define DEFAULT_TCP_WINDOW_SIZE     0
#else
    #define DEFAULT_TCP_WINDOW_SIZE     (1024*1024) /* 1 Megabyte */
#endif /* linux */

#define DEFAULT_LIMITSIZE_MULTIPLE 3

/* initial size of dirty segment buffer */
#define INIT_DELTA_SIZE 8 
#if defined(_AIX) && defined(MAXSEG) 
#undef MAXSEG
#endif /* defined(_AIX) && defined(MAXSEG) */

#define MAXFILES 30000

/* This is the number of BAB chunks sent before the dirty bits are cleared */
#define CLEAR_BITS_HISTORY      10000

#if defined(SOLARIS)
/* gethostbyname(),gethostbysddr() */
/* h_errno==NO_RECOVERY -> Maybe have to use 255 or less filedescriptor. */ 
#define GETHOSTBYNAME_MANYDEV(name, hp, dummyfd) { \
    int newfd; \
    if ( ((hp) = gethostbyname((name))) == NULL && h_errno == NO_RECOVERY) { \
        newfd = fcntl((dummyfd), F_DUPFD, 0); \
        close((dummyfd)); \
        hp = gethostbyname((name)); \
        dup2(newfd,(dummyfd)); \
        close(newfd); \
    } \
}
#define GETHOSTBYADDR_MANYDEV(addr, len, type, hp, dummyfd) { \
    int newfd; \
    if ( ((hp) = gethostbyaddr ((addr), (len), (type)))==NULL && h_errno == NO_RECOVERY){ \
        newfd = fcntl((dummyfd), F_DUPFD, 0); \
        close((dummyfd)); \
        hp = gethostbyaddr ((addr), (len), (type)); \
        dup2(newfd, (dummyfd)); \
        close(newfd); \
    } \
}
#endif

// WI_338550 December 2017, implementing RPO / RTT
// This is the maximum value of the sequencer (id of each packet between PMD and RMD)
#define MAX_SEQUENCER           0xFFFF
typedef int Sequencer;
typedef int SequencerTag;


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
/* structure to communicate between different versions of
 * PMD and RMD
 */
typedef struct checksum32_s {
    int cnt;                        /* number of checksum lists    */
    int num;                        /* number of checksums in list */
    int dirtyoff;                   /* dirty offset */
    int dirtylen;                   /* dirty len accumulator */
    int segoff;                     /* data offset digest represents  */
    int seglen;                     /* data len digest represents  */
    int digestlen;                  /* digest buffer length        */
    char *digest;                   /* checksums/misc. data        */
} checksum32_t;
typedef struct checksum_s {
    int cnt;                        /* number of checksum lists    */
    int num;                        /* number of checksums in list */
    u_longlong_t dirtyoff;          /* dirty offset */    
    u_longlong_t dirtylen;          /* dirty len accumulator */    
    u_longlong_t segoff;            /* data offset digest represents  */    
    int seglen;                     /* data len digest represents  */
    int digestlen;                  /* digest buffer length        */
    char *digest;                   /* checksums/misc. data        */
} checksum_t;

/* checksum delta structure */
typedef struct ref_s {
    u_longlong_t offset;
    int length;
} ref_t;

/* checksum delta map structure */
typedef struct refmap_s {
    int size;                       
    int cnt;
    ref_t *refp;
} refmap_t;

/* rsync device structure - should go into network.h ???? */
/* structure to communicate between different versions of 
 * PMD and RMD 
 */
typedef struct rsync32_s {
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
    checksum32_t cs;                /* checksums                   */
    refmap_t deltamap;              /* offsets/lengths for deltas  */
#if defined(linux)
    int blksize;                    /* volume blocksize */
#endif
} rsync32_t;
typedef struct rsync_s {
    int devid;                      /* PMD/RMD device id           */
    int devfd;                      /* data device file descriptor */
    u_longlong_t size;              /* total number of sectors     */    
    u_longlong_t ackoff;            /* ACKd sector offset*/     
    u_longlong_t offset;            /* committed sector*/    
    int length;                     /* data length - sectors       */
    u_longlong_t delta;             /* total sectors written*/     
    int datalen;                    /* data buffer length - bytes  */
    int done;                       /* rsync - done flag           */
    char *data;                     /* device data buffer          */
    checksum_t cs;                  /* checksums                   */
    refmap_t deltamap;              /* offsets/lengths for deltas  */
#if defined(linux)
    int blksize;		    /* volume blocksize */
#endif
    u_longlong_t refreshed_sectors; /* The amount of sectors known to have been refreshed by a CMDRFDFWRITE command. */
    unsigned int pending_delta_acks;/* The amount of checksum delta blocks that are pending to be acknowledged. */
    int existing_deltas_in_list;    /* Per device scratch variable used to figure out how many new deltas are detected by chksumdiff(). */
    
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
    u_longlong_t rfshoffset;        /* rsync sectors done */
    u_longlong_t rfshdelta;         /* rsync sectors changed */ 
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
    ftd_uint64_t limitsize_multiple; /* disk size limit multiple  
                                        (see libexec/limitsize) */
    int devid;                      /* device ID insofar as PMD/RMD com */
    dev_t dd_rdev;                  /* data device major/minor number */
    dev_t sd_rdev;                  /* ftd device major/minor number */
    int mirfd;                      /* device descriptor for data device*/
    int dirty;			    /* dirty flag and need to 'sync' */
    int no0write;                   /* disallow write to sector 0 */
    u_long ts;                      /* timestamp of the current entry   */
    u_longlong_t offset;            /* device offset (sectors)   */ 
    u_longlong_t devsize;           /* device size (sectors)   */
    u_longlong_t mirsize;           /* mirror size (bytes)   */
    char* data;                     /* actual data for the disk block   */
    rsync_t rsync;                  /* rsync device info                */
    rsync_t ackrsync;               /* ACK'd checksum state             */
    dirtymap_t dm;                  /* dirty-bit map for entire device  */
    devstat_t stat;                 /* statistics  */
    devthrotstat_t throtstats;      /* throttle test stats */
    int dd_minor;		    /* data disk requested minor number */
    int dd_major;		    /* data disk requested major number */
    int md_minor;		    /* mirror disk requested minor number */
    int md_major;		    /* mirror disk requested major number */
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

/* This is the maximum value of the sequencer (id of each packet between PMD and RMD) */
#define MAX_SEQUENCER           0xFFFF

typedef struct tag_BootBlock
{
	char data[1024];
}BootBlock;

typedef struct sz_context
    {
    int	flags;
    sddisk_t  sd;                  /* device state */
    /* ftd_dev_cfg_t   devcfgp; */
    BootBlock LocalSector;        /* Sector zero contained on device */
    BootBlock RemoteSector;		/* Sector zero received from primary */
    void*	next; 			/* Linked list, since we can have multiple */
    void*	prev;
} ftd_sz_context;


#define GET_LG_SZ_INVALID(x)    (x &  0x00000001)
#define SET_LG_SZ_INVALID(x)    (x |= 0x00000001)
#define UNSET_LG_SZ_INVALID(x)  (x &= 0xfffffffe)

// WI_338550 December 2017, implementing RPO / RTT
typedef struct RTTComputingControl_s
    {
    int            State;
    // Time that RTT was computed the last time
    time_t          LastSamplingTime;
    // Requested time interval
    time_t          TimeInterval;
    // Sequencer used to track the chunk
    SequencerTag    iSequencerTag;
    // Number of chunk sent before the tag
    unsigned int    ChunksNumberSentBeforeTag;
    // Tick captured when sending a packet
    u_long           SendTick;
    // RTT computation frequency based sequencer offset
    // available when network flow control is enabled.
    int             SequencerOffsetRTT;
    } RTTComputingControl_struct;
#define RTT_COMPUTE_INTERVAL_SEC 1
#define RTT_COMPUTE_SEQUENCER_OFFSET    10

// WI_338550 December 2017, implementing RPO / RTT
#define NUM_OF_RTT_SAMPLES 128
typedef struct QualityOfService_s
    {
    // Timestamp of last IO successfully applied to the mirror
    u_long LastConsistentIOTimestamp;
    // Timestamp of the oldest IO not applied to the mirror yet
    u_long OldestInconsistentIOTimestamp;
    // Round-Trip-Time for data packets
    u_long   RTT;
    u_long   previous_non_zero_RTT;
    // The following is for maintaining an average of the most recent RTT values 
    // because RTT can fluctuate and is used in places to calculate remaining time to send pending data;
    // this is used in a wrap-around fashion (the oldest entries are overwritten when the whole vector is used)
    int      current_number_of_RTT_samples;
    int      current_RTT_sample_index;
    u_long   average_of_most_recent_RTTs;
    u_long   RTT_samples_for_recent_average[NUM_OF_RTT_SAMPLES];
    // Indicate if the system maintains write ordering to the mirror
    int    WriteOrderingMaintained;
    } QualityOfService_struct;

// WI_338550 December 2017, implementing RPO / RTT
#define LG_CONSISTENCY_POINT_TIMESTAMP_INVALID  (ftd_uint64_t)-1
#define CHUNK_TIMESTAMP_QUEUE_SIZE 1024     // ORIGINAL: 2048
typedef struct chunk_timestamp_queue_entry_s
    {
    ftd_uint64_t OldestChunkTimestamp;
    ftd_uint64_t NewestChunkTimestamp;
    } chunk_timestamp_queue_entry_t;

// WI_338550 December 2017, implementing RPO / RTT
typedef struct queue_s
    {
    void    *pBuffer;
    u_long   Size;
    u_long   Head;
    u_long   Tail;
    } queue_t;

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
    u_longlong_t offset;            /* journal offset (bytes) */ 
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
    int iMigratedBABCtr;            /* */
    int SendSequencer;
    int AckedSequencer;
    ftd_sz_context * pListSectorZero; /* Sector Zero logic */
    int capture_io_of_source_devices; /* Dynamic activation configuration to enable capture or not of the IO
                                         sent to this group's primary source devices. */
    int autostart_on_boot;	    /* activate group on system boot */
    // WI_338550 December 2017, implementing RPO / RTT
    // Data used to report quality of service (RPO, RTT)
    RTTComputingControl_struct RTTComputingControl;
    // Actual stats
    QualityOfService_struct QualityOfService;
    queue_t* pChunkTimeQueue;

} group_t;

typedef struct tunable_s {
    int chunksize;       /* TUNABLE: default - 256 Kbyte */
    int statinterval;    /* TUNABLE: default - 10 seconds */
    int maxstatfilesize; /* TUNABLE: default - 1024 Kbyte *//*set default value of maxstatfilesize to 1024 Kb*/
    int tracethrottle;   /* TUNABLE: default - 0 */
    int syncmode;        /* TUNABLE: default - 0 -- Am I doing sync? */
    int	syncmodedepth;   /* TUNABLE: default - 1 deep queue */
    int syncmodetimeout; /* TUNABLE: default - 30 seconds */
    int compression;     /* TUNABLE: default - none */
    int tcpwindowsize;   /* TUNABLE: default - 256 Kbyte */
    int netmaxkbps;      /* TUNABLE: default - -1 */
    int chunkdelay;      
    int use_journal;     /* TUNABLE: default - 1: use journal, 0: journalless */
    int use_lrt;	 /* TUNABLE: default - 1; use LRT */
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
    char        *ipv;
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
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE*       statfd;          /* file descriptor of stat file */
    FILE*       csv_statfd;      /* file descriptor of stat file in csv format */
#elif defined(SOLARIS)
    int         statfd;          /* file descriptor of stat file */
    int         csv_statfd;      /* file descriptor of stat file in csv format */
#endif
    int         tkbps;           /* total network kbps for the logical group */
    int         prevtkbps;       /* previous network kbps for the logical grp */
    throttle_t  *throttles;      /* list of throttles to eval with stat update */
    group_t     group[1];        /* logical group */
    tunable_t   tunables;        /* tunable parameters */
    int         flags;
} machine_t;


#define FTD_LG_JLESS            0x01

#define SET_LG_JLESS(sys, x)    (((x) == 0) ? \
                                (((machine_t *)(sys))->flags &= ~FTD_LG_JLESS) :\
                                (((machine_t *)(sys))->flags |= FTD_LG_JLESS))
#define GET_LG_JLESS(sys)       ((((machine_t *)(sys))->flags & FTD_LG_JLESS) ?\
                                 1 : 0)

#ifdef DEBUG_THROTTLE
extern FILE*       throtfd;             /* throttle trace file */
#endif /* DEBUG_THROTTLE */

/* -- function prototypes for configuration file management -- */
extern void initconfigs (void);
extern int readconfig (int pmdflag, int pstoreflag, int openflag, char* configpath);
extern int stringcompare (const void* p1, const void* p2);
extern char* iptoa (u_long ip);
extern int isthisme4 (char* hostnodename, u_long );
#if !defined(FTD_IPV4)
extern int isthisme6 (char* hostnodename, char* );
#endif
extern int getnumdtcdevices (int lgnum, int *numdevs, char **dtcdevs);
extern int settunable (int lgnum, char *key, char *value, int flag);

/* -- function prototypes for IP address utilities */
extern int getnetconfcount(void);
extern int getnetconfcount6(void);  /*----IP Address count function----*/
extern void getnetconfs(u_long* ipaddrs);
extern void getnetconfs6(char* ipv6); /*----IP Address fetching function----*/
extern u_longlong_t fdisksize(int fd); 
extern u_longlong_t disksize(char *disk); 

extern int parseftddevnum(char*);

extern int gettunables (char *group_name, int psflag, int quiet);
extern int getdrvrtunables (int lgnum, tunable_t *tunables);

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
extern u_long* myips;   /* array of IPv4  addresses for this machine */
extern char* myip6;      /* array of IPv6  addresses for this machine */
extern int myipcount;   /* number of IP addresses this machine has */
extern int myip6count;   /* number of IP addresses this machine has */

/* -- structures and functions for dynamic device addtion -- */
/* -- structures -- */
typedef struct diffdev {
    struct diffdev *p;              /* previous disk profile definition */
    struct diffdev *n;              /* next disk profile definintion */
    char sddevname[FILE_PATH_LEN];  /* ftd device name */
    char devname[FILE_PATH_LEN];    /* data device name (local disk) */
    char mirname[FILE_PATH_LEN];    /* mirror name */
    int dd_minor;		    /* data disk requested minor number */
    int dd_major;		    /* data disk requested major number */
    int md_minor;		    /* mirror disk requested minor number */
    int md_major;		    /* mirror disk requested major number */
} diffdev_t;
typedef struct difflg {
    char phost[256];                /* fully qualified name of primary system */
    char shost[256];                /* fully qualified name of secondary system */
    char journal_path[FILE_PATH_LEN]; /* secondary journal path */
    char pstore[FILE_PATH_LEN];
    char devname[FILE_PATH_LEN];    /* group ctl device name */
    int chaining;                   /* 0 - open mirror excl */
} difflg_t;

typedef enum diff_stat {
    DIFFSTAT_UNMATCH = -2,
    DIFFSTAT_DEV_REMOVED,
    DIFFSTAT_ALL_MATCH,
    DIFFSTAT_DEV_ADDED } diff_stat_t;
typedef struct diffent {
    diff_stat_t status;             /* diff status */
    diffdev_t *h_same;              /* head of the same device list */
    diffdev_t *t_same;              /* tail of the same device list */
    diffdev_t *h_add;               /* head of the added device list */
    diffdev_t *t_add;               /* tail of the added device list */
    diffdev_t *h_remove;            /* head of the removed device list */
    diffdev_t *t_remove;            /* tail of the removed device list */
    difflg_t oldlg;                 /* old lg information */
    difflg_t newlg;                 /* new lg information */
    throttle_t *oldthrl;            /* list of old throttles */
    throttle_t *newthrl;            /* list of new throttles */
} diffent_t;

/* -- functions -- */
diffent_t *
diffconfig(char *basefilename, char *newfilename);
/* parameters
    basefilename: old config file name (LGNUM.cur)
    newfilename:  new config file name (LGNUM.cfg)
   return value
    0 : fail to analyze config file
    >0 : address of diffent
*/
void
freediffent(diffent_t *target);
/* parameters
    target: target to free (return value of diffconfig())
*/

#ifdef TDMF_TRACE
extern int getpsname (char *mod, char *funct,int lgnum, char *ps_name);
#else
extern int getpsname (int lgnum, char *ps_name);
#endif

extern int getautostart(int lgnum);
extern int inittunables(char *ps_name, char *group_name, char *pstbuf, int pstbuf_len);
extern int setdrvrtunables (int lgnum, tunable_t *tunables);
extern int verify_and_set_tunable(char *key, char *value, tunable_t *tunables, int validate_key, int *net_analysis_duration);
extern int close_journal(group_t *group);
extern int verify_rmd_entries(group_t *group, char *path, int openflag);
extern int read_key_value_from_file(char *file_path, char *key, char *value, int buffer_length, int stringval);
 
#if defined(linux)
extern char *adjust_data_ptr(char *data_ptr, int blksize);
#endif

extern int check_if_RHEL7();

#endif /* _CONFIG_H */
