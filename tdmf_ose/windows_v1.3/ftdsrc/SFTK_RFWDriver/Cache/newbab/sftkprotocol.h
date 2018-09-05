/*HDR************************************************************************
 *                                                                         
 * Softek -                                     
 *
 *===========================================================================
 *
 * N sftkprotocol.h
 * P Replicator 
 * S Cache & FIFO
 * V Generic
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 03.05.2004
 * O This file contains the Protocol API that will be used to exchange 
 *   data between the primary and the secondary.
 * T Cache design requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_
 * H 04.28.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: sftkprotocol.h,v 1.3 2004/07/01 18:27:48 vqba0 Exp $"
 *
 *HDR************************************************************************/

#ifndef  _RPLCC_PROTOC_
#define  _RPLCC_PROTOC_

#define FTDCFGMAGIC 0xBADF00D5
 
#define FTDLGMAGIC  0xBADF00D6

#define ROLEPRIMARY		1
#define ROLESECONDARY	2

/*
 * Prefix on our primary-system config files
 */
#define PRIMARY_CFG_PREFIX	"p"

/*
 * Prefix on our secondary-system config files
 */
#define SECONDARY_CFG_PREFIX	"s"

/*
 * Suffix on our config files
 */
#define PATH_CFG_SUFFIX	"cfg"
#define PATH_STARTED_CFG_SUFFIX	"cur"

 
 /* protocol magic numbers */
#define MAGICACK  0xbabeface
#define MAGICERR  0xdeadbabe
#define MAGICHDR  0xcafeface

#define MSG_AVOID_JOURNALS		"DO_NOT_USE_JOURNALS"
#define MSG_INCO				"INCOHERENT"	// traditional SmartRefresh is using Journal
#define MSG_CO					"COHERENT"
#define MSG_CPON				"CHECKPOINT_ON"
#define MSG_CPOFF				"CHECKPOINT_OFF"

#define DEV_BSHIFT  9		//Block Shift
#define DEV_BSIZE   512	    //Block Size

/* peer <-> peer protocol */

#define FTDZERO		-1

#define CHKSEGSIZE		32768
#define DIGESTSIZE		16

#define FTDACKVERSION1	0

enum protocol {

	FTDCHANDSHAKE =1,	//Done
	FTDCCHKCONFIG,	   
	FTDCNOOP,		   //Done
	FTDCVERSION,	   //Done
	FTDCCHUNK,		   
	FTDCHUP,		   
	FTDCCPONERR,	   
	FTDCCPOFFERR,	   
	FTDCEXIT,		   
	FTDCBFBLK,		   
	FTDCBFSTART,	   
	FTDCBFREAD,		   
	FTDCBFEND,	   
	FTDCCHKSUM,		   
	FTDCRSYNCDEVS,	   
	FTDCRSYNCDEVE,	   
	FTDCRFBLK,		   
	FTDCRFEND,		   
	FTDCRFFEND,		   
	FTDCKILL,		   
	FTDCRFSTART,	   
	FTDCRFFSTART,	   
	FTDCCPSTARTP,	   
	FTDCCPSTARTS,	   
	FTDCCPSTOPP,	   
	FTDCCPSTOPS,	   
	FTDCCPON,		   
	FTDCCPOFF,		   
	FTDCSTARTAPPLY,	   
	FTDCSTOPAPPLY,	   
	FTDCSTARTPMD,	   
	FTDCAPPLYDONECPON, 
	FTDCREFOFLOW,	   
	FTDCSIGNALPMD,	   
	FTDCSIGNALRMD,	   
	FTDCSTARTRECO,	   

	/* acks */
	FTDACKERR ,   
	FTDACKRSYNC,	   
	FTDACKCHKSUM,	   
	FTDACKCHUNK,	   
	FTDACKHUP,		   
	FTDACKCPSTART,	   
	FTDACKCPSTOP,	   
	FTDACKCPON,		   
	FTDACKCPOFF,	   
	FTDACKCPOFFERR,	   
	FTDACKCPONERR,	   
	FTDACKNOOP,		   
	FTDACKCONFIG, 	   
	FTDACKKILL ,  
	FTDACKHANDSHAKE,   
	FTDACKVERSION,	   
	FTDACKCLI,		   
	FTDACKRFSTART,	   
	FTDACKNOPMD,	   
	FTDACKNORMD,	   
	FTDACKDOCPON,	   
	FTDACKDOCPOFF,	   
	FTDACKCLD,		   
	FTDACKCPERR,	   

	/* signals */
	FTDCSIGUSR1,
	FTDCSIGTERM,	   
	FTDCSIGPIPE,		   

	/* management msg type */
	FTDCMANAGEMENT,

	/* new debug level msg */
	FTDCSETTRACELEVEL,
	FTDCSETTRACELEVELACK,

	/* REMOTE WAKEUP msg */
	FTDCREMWAKEUP,

	/* REMOTE DRIVE ERROR */
	FTDCREMDRIVEERR,

	/* SMART REFRESH RELAUNCH FROM RMD,RMDA */
	FTDCREFRSRELAUNCH,

	//This is for Sentinal

	//These will be used to set as commands for the Sentinals.
	//Care will be taken so that they are inserted into the Data Pool
	//Only after the Refresh and Migration Queue's are empty.
	FTDSENTINAL,
	FTDMSGINCO,
	FTDMSGCO,
	FTDMSGAVOIDJOURNALS,
	FTDMSGCPON,
	FTDMSGCPOFF
};

typedef enum protocol protocol_e;

/*
 * Compatiblity mode:
 * -----------------
 *
 * This is take directly from the orginal code
 * for Compatibility with secondary server for now.
 */
typedef unsigned __int64    ftd_uint64_t;
typedef __int64             ftd_int64_t;

#define DATASTAR_MAJIC_NUM  0x0123456789abcdefUL

#pragma pack(push, 4) 

//#pragma pack(4)

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

    void            *group_ptr;         /* internal use only */
    void            *bp;                /* IRP to complete on migrate */

} wlheader_t;

	/* FTD logical group HEADER message structure */
typedef struct ftd_lg_header_s {
	int lgnum;						/* lgnum                         */
	int devid;						/* device id                     */
	int bsize;						/* device sector size (bytes)    */
	int offset;						/* device i/o offset (sectors)   */
	int len;						/* device i/o len (sectors)      */
	int data;						/* store miscellaneous info      */
	int flags;						/* miscelaneous peer state flags */
	int reserved;					/* not used */
} ftd_lg_header_t;

#define	FTD_MSGSIZE				32	// this makes it a 64 byte header	

/* standard message header structure */
typedef struct ftd_header_s {
	unsigned long magicvalue;		/* indicate that this is a
									 header packet */
	time_t ts;						/* timestamp transaction was sent      */
	int msgtype;					/* packet type                         */
	HANDLE cli;						/* packet is from cli			       */
	int compress;					/* compression algorithm employed      */
	int len;						/* data length that follows            */
	int uncomplen;					/* uncompressed data length            */
	int ackwanted;					/* 0 = No ACK                          */
	union {
		ftd_lg_header_t lg;			/* FTD logical group header msg */
		char data[FTD_MSGSIZE];		/* generic all-purpose message buffer */
	} msg;
} ftd_header_t;

typedef struct ftd_auth_s {
	int len;
	char auth[512];
	char configpath[MAX_PATH];
} ftd_auth_t;

typedef union {
	UCHAR uc[4];
	ULONG ul;
} encodeunion;

typedef struct ftd_rdev_s {
	int devid;
	dev_t ftd;
	dev_t minor;
	int len;
	char path[MAX_PATH];
} ftd_rdev_t;

///////////////////////////////////////////
//FTDCVERSION
//////////////////////////////////////////

typedef struct ftd_version_s {
	LONG pmdts;
	int tsbias;
	char configpath[MAX_PATH];
	char version[256];
} ftd_version_t;

typedef struct stat_buffer_t {
    unsigned int lg_num;  /* logical group device id */
    unsigned int dev_num; /* device id. used only for GET_DEV_STATS */
    int   len;     /* length in bytes */
    char  *addr;   /* pointer to the buffer */
} stat_buffer_t;

#define MAXERR	512
/* statndard error packet structure */
typedef struct ftd_err_s {
	unsigned long errcode;			/* error severity code                 */
	char errkey[12];				/* error mnemonic key                  */
	unsigned long length;			/* length of error message string      */
	char msg[MAXERR];				/* error message describing error      */
	char fnm[MAX_PATH];				/* source file name					*/
	int lnum;						/* line number						*/
} ftd_err_t;

/* device level statistics structure */
typedef struct ftd_dev_stat_s {
	int devid;					/* device id                             */
#if !defined(_WINDOWS)
	int				actual;		/* device actual (bytes)                 */
	int				effective;	/* device effective (bytes)              */
#else
	int				connection;
	ftd_uint64_t	actual;
	ftd_uint64_t	effective;
#endif
	int entries;				/* device entries                        */
	int entage;					/* time between BAB write and RMD recv   */
	int jrnentage;				/* time between jrnl write and mir write */
	off_t rsyncoff;				/* rsync sectors done                    */
	int rsyncdelta;				/* rsync sectors changed                 */
	float actualkbps;			/* kbps transfer rate                    */
	float effectkbps;			/* effective transfer rate w/compression */
	float pctdone;				/* % of refresh or backfresh complete    */
	int sectors;				/* # of sectors in bab                   */
	int pctbab;					/* pct of bab in use                     */
	double local_kbps_read;		/* ftd device kbps read                  */
	double local_kbps_written;	/* ftd device kbps written               */
	ftd_uint64_t	ctotread_sectors;
	ftd_uint64_t	ctotwrite_sectors;
} ftd_dev_stat_t;

typedef struct {
	void* tail;
	void* head;
	int objsize;
}LList;
/* ftd volume structure */
typedef struct ftd_dev_s {
	int devid;					/* device ID insofar as PMD/RMD com      */
	HANDLE devfd;				/* local device handle                */
	HANDLE ftddevfd;			/* ftd device handle                */
    int devsize;				/* device size (sectors)                 */
	int devbsize;				/* device block size (bytes)             */
	int devbshift;				/* device block size bit shift           */
	int no0write;				/* meta-dev flag - don't write sector 0  */
	dev_t ftdnum;				/* ftd device number - primary only      */
	dev_t num;					/* local device number              */
	int dbvalid;				/* number valid dirty bits in map        */
	int dbres;					/* dirty bit map resolution              */
	int dblen;					/* dirty bit map length                  */
	unsigned char *db;			/* high-res dirty bit map address		*/
	off_t dirtyoff;				/* start of dirty segment offset         */
	int dirtylen;				/* dirty segment length            */
	off_t rsyncoff;				/* sector read offset                    */
	off_t rsyncackoff;			/* ACKd sector offset                    */
	int rsynclen;				/* data read length - sectors            */
	int rsyncbytelen;			/* data read length - bytes              */
	int rsyncdelta;				/* total sectors transferred/written     */
	int rsyncdone;				/* rsync - done flag                     */
	char *rsyncbuf;				/* device data buffer address            */
	int sumcnt;					/* number of checksum lists              */
	int sumnum;					/* number of checksums in each list      */
	int sumoff;					/* device offset digest represents       */
	int sumlen;					/* device length digest represents       */
	int sumbuflen;				/* checksum digest buffer length         */
	char *sumbuf;				/* checksum digest data buffer  address    */
	ftd_dev_stat_t *statp;		/* device-level statistics               */
	LList *deltamap;			/* offsets/lengths for deltas            */
} ftd_dev_t;


#ifdef NOT_USED
/* logical group structure */
typedef struct ftd_lg_s {
	int magicvalue;				/* so we know it's been initialized */
	int lgnum;					/* lg number				*/
	unsigned int flags;			/* lg state bits			*/
	HANDLE ctlfd;				/* FTD master control device file decriptor*/
	HANDLE devfd;				/* FTD lg device file descripto		*/
#if defined(_WINDOWS)
	HANDLE babfd;
	HANDLE perffd;				/* get perf data event */
#endif
	char devname[MAX_PATH];	/* lg control device name		*/
	int buflen;					/* length of BAB I/O buffer		*/
	char *buf;					/* BAB buffer address			*/
	int recvbuflen;				/* length of ack recv buffer	*/
	char *recvbuf;				/* ack receive buffer address	*/
	int cbuflen;				/* compress buffer length */
	float cratio;				/* compress ratio of comp/uncomp	*/
	char *cbuf;					/* compress buffer address */
	int bufoff;					/* BAB buffer current entry offset */
	int datalen;				/* BAB buffer valid data length		*/
	unsigned long offset;		/* current BAB read offset (bytes)	*/
	unsigned long ts;			/* timestamp of the current BAB entry	*/
	unsigned long blkts;		/* timestamp of PMD get			*/
	unsigned long lrdbts;		/* timestamp of lrdb update		*/
	unsigned long kbpsts;		/* timestamp of kbps test		*/
	int tsbias;					/* time differential betw. prim. & sec.	*/
	int babsize;				/* total size of lg BAB - in sectors	*/
	int babused;				/* total size of lg used BAB - in secto	*/
	int babfree;				/* total size of lg free BAB - in secto	*/
	long timeslept;				/* idle time				*/
	long timerefresh;			/* time spent trying to resync after oflow */
	time_t idlets;				/* timestamp of initial idling state */
	time_t refreshts;			/* timestamp when refresh started */
	time_t oflowts;				/* timestamp when BAB overflowed */
	float kbps;					/* current kbps for lg			*/
	float prev_kbps;			/* previous kbps for lg			*/
	unsigned int netsleep;		/* pmd sleep value for netmax tunable */
	int consleep;				/* pmd sleep value for reconnect */
	offset_t rsyncoff;			/* lg rsync offset			*/
#if !defined(_WINDOWS)
	int sigpipe[2];				/* signal queue				*/
#else
	proc_t *procp;
#endif		
	comp_t *compp;				/* compressor object address */
	dirtybit_array_t *db;		/* dirtybit map address			*/
	ftd_sock_t *isockp;			/* lg ipc socket object address		*/	
	ftd_sock_t *dsockp;			/* lg data socket object address	*/	
	ftd_proc_t *fprocp;			/* lg process object address		*/
	ftd_lg_cfg_t *cfgp;			/* lg configuration file state		*/ 
	ftd_tune_t *tunables;		/* lg tunable parameters		*/
	ftd_journal_t *jrnp;		/* lg journal object address		*/
	ftd_lg_stat_t *statp;		/* lg runtime statistics		*/
#if defined(_WINDOWS)
    ftd_mngt_lg_monit_t *monitp;/* lg runtime monitoring */
#endif
	LList *throttles;			/* lg throttle list			*/
	LList *devlist;				/* lg device list			*/

	ftd_sz_context	*SectorZero;	/* List of context structures for sector zero save/restore */ 

} ftd_lg_t;
#endif /*NOT_USED*/
#pragma pack(pop) // packing is default


#define DATASTAR_MAJIC_NUM  0x0123456789abcdefUL

OS_NTSTATUS 
SftkPrepareSentinal( IN enum sentinals   eSentinal, 
					 IN OUT wlheader_t  *softHeader, 
					 IN rplcc_iorp_t    *inputData,
					 OUT PVOID           outputData);

OS_NTSTATUS 
SftkPrepareProtocolHeader( IN enum protocol     eProtocol,
 					       IN OUT ftd_header_t *protocolHeader,
						   IN rplcc_iorp_t     *inputData,
						   OUT PVOID            outputData,
						   LONG                 ackwanted );

OS_NTSTATUS 
SftkPrepareSoftHeader( IN PVOID           *p_data,
					   IN rplcc_iorp_t *inputData);


#endif /*_RPLCC_PROTOC_*/

/*EOF*/
