/**************************************************************************************

Module Name: sftk_protocol.h   
Author Name: Saumya Tripathy , Veera Arja
Description: Define all Protocol Structures, Enums, Macros and API definations 
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#ifndef _SFTK_PROTOCOL_H_
#define _SFTK_PROTOCOL_H_

// Timeout values for different Connections

#define	FTD_SOCK_CONNECT_SHORT_TIMEO	1
#define	FTD_SOCK_CONNECT_LONG_TIMEO		10
#define	FTD_NET_SEND_NOOP_TIME			20
#define	FTD_NET_MAX_IDLE				30	

#if DBG
#define FTD_NET_ACK_RECEIVE_TIMEOUT		180	// for debugging make it 300 seconds, 5 minutes
#else
#define FTD_NET_ACK_RECEIVE_TIMEOUT		30	// for debugging make it 60 seconds
#endif

// Maximum TCP window size used

#define FTD_MAX_TCP_WINDOW_SIZE			(16*1024*1024) // 16 MB


#define	FTD_MSGSIZE			32	// this makes it a 64 byte header
#define MAX_PATH			260
#define MAXPATH				MAX_PATH
#define MAXERR				512

// DEV_BSHIFT is set for 512 bytes 
#define DEV_BSHIFT			9
#define DEV_BSIZE			512

// The Checksum algorithms supported are MD5 only for present

#define A_MD5				1	// This is MD5 Hash

#define A_RLE				2	// This is Run Length Encoding

#define A_CRC				3	// The CRC Algorithm used for creating the chucksum
								// of the Network Packets This is Faster

// The Parameters used for creating the Checksum

#define CHKSEGSIZE			32768	// This is the Amount of data that is used for creating the
									// Checksum

#define	DIGESTSIZE			16		// The Digest Size generated for MD5 Algorithm


/* protocol magic numbers */
#define MAGICACK			0xbabeface	// Magic Value Used for the ACk
#define MAGICERR			0xdeadbabe	// Magic Value Used for the Error
#define MAGICHDR			0xcafeface	// Magic Value Used for the Header

// This is the magic value used for the IO Data
#define DATASTAR_MAJIC_NUM  0x0123456789abcdefUL	

#define MSG_AVOID_JOURNALS	"DO_NOT_USE_JOURNALS"
#define MSG_INCO			"INCOHERENT"	// traditional SmartRefresh is using Journal
#define MSG_CO				"COHERENT"
#define MSG_CPON			"CHECKPOINT_ON"
#define MSG_CPOFF			"CHECKPOINT_OFF"

#define FTDZERO				-1

#define FTDACKVERSION1		0	// ACK Returned for the VERSION Command


typedef __int64					ftd_int64_t;
typedef unsigned __int64		ftd_uint64_t;
typedef __int64					offset_t;

//
// Basic system type definitions, taken from the BSD file sys/types.h.
//
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;


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
#define STRING_CONFIG_FILENAME	"%s%03d.%s"	// example: s000.cfg, s001.cfg
#define PATH_CFG_SUFFIX	"cfg"
#define PATH_STARTED_CFG_SUFFIX	"cur"

/* peer <-> peer protocol */
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
	FTDMSGCPOFF,

	FTDRECEIVEERROR
};


//
// defines the type of sentinels we can support
// in the cache.
//
typedef enum sentinals
{
	MSGNONE = 1,
	MSGINCO,
	MSGCO,
	MSGCPON,
	MSGCPOFF,
	MSGAVOIDJOURNALS
}sentinels_e;


#define LG_BYTE_0(x)			(((unsigned char*)&(x))[0])
#define LG_BYTE_1(x)			(((unsigned char*)&(x))[1])
#define LG_BYTE_2(x)			(((unsigned char*)&(x))[2])
#define LG_BYTE_3(x)			(((unsigned char*)&(x))[3])

#define LG_CPSTARTP				0x04
#define LG_CPSTARTS				0x08
#define LG_CPSTOPP				0x10
#define LG_CPSTOPS				0x20

#define	FTD_LG_BACK_FORCE		0x100

/* lg state macros */
#define GET_LG_STATE(x)			(LG_BYTE_0(x) & 0xff) 
#define SET_LG_STATE(x,y)		(LG_BYTE_0(x) = ((y) & 0xff)) 

#define GET_LG_CPON(x)			(LG_BYTE_1(x) & 0x01) 
#define SET_LG_CPON(x)			(LG_BYTE_1(x) |= 0x01) 
#define UNSET_LG_CPON(x)		(LG_BYTE_1(x) &= 0xfe) 

#define GET_LG_CPPEND(x)		(LG_BYTE_1(x) & 0x02) 
#define SET_LG_CPPEND(x)		(LG_BYTE_1(x) |= 0x02) 
#define UNSET_LG_CPPEND(x)		(LG_BYTE_1(x) &= 0xfd) 

#define GET_LG_CPSTART(x)		(LG_BYTE_1(x) & 0x0c) 
#define SET_LG_CPSTART(x,y)		(LG_BYTE_1(x) |= (y)) 
#define UNSET_LG_CPSTART(x)		(LG_BYTE_1(x) &= 0xf3) 

#define GET_LG_CPSTOP(x)		(LG_BYTE_1(x) & 0x30) 
#define SET_LG_CPSTOP(x,y)		(LG_BYTE_1(x) |= (y)) 
#define UNSET_LG_CPSTOP(x)		(LG_BYTE_1(x) &= 0xcf) 

#define GET_LG_CHKSUM(x)				 (LG_BYTE_2(x) &  0x01) 
#define SET_LG_CHKSUM(x)				 (LG_BYTE_2(x) |= 0x01) 
#define UNSET_LG_CHKSUM(x)				 (LG_BYTE_2(x) &= 0xfe) 

#define GET_LG_BACK_FORCE(x)			 (LG_BYTE_2(x) &  0x02) 
#define SET_LG_BACK_FORCE(x)			 (LG_BYTE_2(x) |= 0x02) 
#define UNSET_LG_BACK_FORCE(x)			 (LG_BYTE_2(x) &= 0xfd) 

#define GET_LG_SZ_INVALID(x)			 (LG_BYTE_2(x) &  0x10) 
#define SET_LG_SZ_INVALID(x)			 (LG_BYTE_2(x) |= 0x10) 
#define UNSET_LG_SZ_INVALID(x)			 (LG_BYTE_2(x) &= 0xef);


#if 1 
/* SET_LG_FULLREFRESH_PHASE2 means that Refresh blocks and BAB entries will go in the target drive (like MIRONLY mode).
   This mode changes the behavior the SmartRefresh
*/	
#define GET_LG_FULLREFRESH_PHASE2(x)     (LG_BYTE_2(x) &  0x20) 
#define SET_LG_FULLREFRESH_PHASE2(x)	 (LG_BYTE_2(x) |= 0x20) 
#define UNSET_LG_FULLREFRESH_PHASE2(x)   (LG_BYTE_2(x) &= 0xdf)

#define GET_LG_JLESS(x)  		 (LG_BYTE_2(x) &  0x40) 
#define SET_LG_JLESS(x)		     (LG_BYTE_2(x) |= 0x40) 
#define UNSET_LG_JLESS(x)		 (LG_BYTE_2(x) &= 0xbf) 

#define GET_LG_CPOFF_JLESS_ACKPEND(x)	         (LG_BYTE_2(x) &  0x04) 
#define SET_LG_CPOFF_JLESS_ACKPEND(x)	         (LG_BYTE_2(x) |= 0x04) 
#define UNSET_LG_CPOFF_JLESS_ACKPEND(x)	     (LG_BYTE_2(x) &= 0xfb) 

#define GET_LG_RFDONE(x)		(LG_BYTE_3(x) & 0x02) 
#define SET_LG_RFDONE(x)		(LG_BYTE_3(x) |= 0x02) 
#define UNSET_LG_RFDONE(x)		(LG_BYTE_3(x) &= 0xfd) 

#define GET_LG_RFSTART_ACKPEND(x)		(LG_BYTE_3(x) & 0x01) 
#define SET_LG_RFSTART_ACKPEND(x)		(LG_BYTE_3(x) |= 0x01) 
#define UNSET_LG_RFSTART_ACKPEND(x)		(LG_BYTE_3(x) &= 0xfe) 

#define GET_LG_RFDONE_ACKPEND(x)	(LG_BYTE_3(x) & 0x04) 
#define SET_LG_RFDONE_ACKPEND(x)	(LG_BYTE_3(x) |= 0x04) 
#define UNSET_LG_RFDONE_ACKPEND(x)	(LG_BYTE_3(x) &= 0xfb) 

#define GET_LG_CHAINING(x)		(LG_BYTE_3(x) & 0x08) 
#define SET_LG_CHAINING(x)		(LG_BYTE_3(x) |= 0x08) 
#define UNSET_LG_CHAINING(x)	(LG_BYTE_3(x) &= 0xf7) 

#define GET_LG_STARTED(x)		(LG_BYTE_3(x) & 0x10) 
#define SET_LG_STARTED(x)		(LG_BYTE_3(x) |= 0x10) 
#define UNSET_LG_STARTED(x)		(LG_BYTE_3(x) &= 0xef) 

#define GET_LG_RFSTART(x)		(LG_BYTE_3(x) & 0x20) 
#define SET_LG_RFSTART(x)		(LG_BYTE_3(x) |= 0x20) 
#define UNSET_LG_RFSTART(x)		(LG_BYTE_3(x) &= 0xdf) 

#define GET_LG_RFTIMEO(x)		(LG_BYTE_3(x) & 0x40) 
#define SET_LG_RFTIMEO(x)		(LG_BYTE_3(x) |= 0x40) 
#define UNSET_LG_RFTIMEO(x)		(LG_BYTE_3(x) &= 0xbf) 

#define GET_LG_BAB_READY(x)		(LG_BYTE_3(x) & 0x80) 
#define SET_LG_BAB_READY(x)		(LG_BYTE_3(x) |= 0x80) 
#define UNSET_LG_BAB_READY(x)	(LG_BYTE_3(x) &= 0x7f) 

#endif

#define VERSIONSTR		"5.0.0"

#pragma pack(push, 4)	// TODO : Veera : verify this packing with RMD - from parag

//
// Structure definations
//
typedef struct _SFTK_IO_VECTOR
{
	PVOID	pBuffer;
	ULONG	uLength;
} SFTK_IO_VECTOR, *PSFTK_IO_VECTOR;

/* FTD logical group HEADER message structure */
typedef struct ftd_lg_header_s {
	int lgnum;						/* lgnum              */
	int devid;						/* device id              */
	int bsize;						/* device sector size (bytes)       */
	int offset;						/* device i/o offset (sectors)         */
	int len;						/* device i/o len (sectors)          */
	int data;						/* store miscellaneous info */
	int flags;						/* miscelaneous peer state flags */
	int reserved;					/* not used */ 
} ftd_lg_header_t;

/* standard message header structure */
typedef struct ftd_header_s {
	unsigned long magicvalue;		/* indicate that this is a
									 header packet */
	// Saumya_Fix:: It is defined as 'long' in the previous project
	LONG ts;						/* timestamp transaction was sent      */

	int msgtype;					/* packet type                         */
	HANDLE cli;						/* packet is from cli				*/
	int compress;					/* compression algorithm employed      */
	int len;						/* data length that follows            */
	int uncomplen;					/* uncompressed data length            */
	int ackwanted;					/* 0 = No ACK            */
	union {
		ftd_lg_header_t lg;			/* FTD logical group header msg */
		char data[FTD_MSGSIZE];		/* generic all-purpose message buffer */
	} msg;
} ftd_header_t;

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

typedef struct ftd_version_s {
	// Saumya_Fix:: It is defined as 'long' in the previous project
	LONG pmdts;

	int tsbias;
	char configpath[MAXPATH];
	char version[256];
} ftd_version_t;

typedef struct ftd_auth_s {
	int len;
	char auth[512];
	char configpath[MAXPATH];
} ftd_auth_t;

typedef union {
	u_char uc[4];
	u_long ul;
} encodeunion;

typedef struct ftd_rdev_s {
	int devid;
	dev_t ftd;
	dev_t minor;
	int len;
	char path[MAXPATH];
} ftd_rdev_t;

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
	void *tail;
	void *head;
	int objsize;
} LList;

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
	LList  *deltamap;			/* offsets/lengths for deltas            */
} ftd_dev_t;

#pragma pack(pop) // TODO : Veera : verify this packing with RMD - from parag

// This structure is a temprory structure that will be removed later on when
// We replace this with the Parag's Structure.
typedef struct _rplcc_iorp_
{
	PVOID         DevicePtr;     // Windows Device ptr.
	ULONG         DeviceId;      // bdev;
	ULONGLONG     Blk_Start;     // 64bits;
	ULONG         Size;
	PVOID         pBuffer;
	PVOID         pHdlEntry;     // Ptr to cache entry.
	sentinels_e   SentinelType;  // if the data is a Sentinel

} rplcc_iorp_t;

// FUNCTION PROTOTYPES
// The Functions Below will represent the Functions exported by the Ptotocol Layer

CHAR* 
PROTO_GetProtocolString(
					LONG protocol
					);

LONG 
PROTO_GetCommandOfAck(
			IN ftd_header_t	*AckProtoHdr 
			);

LONG 
PROTO_GetAckOfCommand(
			IN enum protocol SentProtocolCommand
			);

LONG 
PROTO_GetAckSentinal(
			IN enum sentinals   eSentinal
			);

NTSTATUS 
PROTO_PrepareProtocolSentinal(	IN enum protocol	eProtocol, 
								IN OUT ftd_header_t *protocolHeader,
								IN OUT wlheader_t	*softHeader,
								IN PVOID			RawBuffer,			// Optional, for Payload, Copy this to new allocated buffer
								IN ULONG			RawBufferSize,
								IN ULONGLONG		Offset,
								IN ULONG			Size,
								IN LONG				Lg_Number,
								IN LONG				Dev_Number);	// optional : Size of RetBuffer


NTSTATUS 
PROTO_PrepareSentinal( IN enum sentinals eSentinal, 
					 IN OUT wlheader_t *softHeader, 
					 IN rplcc_iorp_t   *inputData,
					 OUT PVOID outputData);


NTSTATUS 
PROTO_PrepareProtocolHeader( IN enum protocol     eProtocol,
 					       IN OUT ftd_header_t  *protocolHeader,
						   IN rplcc_iorp_t      *inputData,
						   OUT PVOID            outputData,
						   LONG                 ackwanted );

NTSTATUS
PROTO_PrepareSoftHeader(IN PVOID           *p_data,
					  IN IN rplcc_iorp_t *p_iorequest);


// SEND and RECEIVE PROTOCOL COMMANDS

struct _SESSION_MANAGER;
typedef struct _SESSION_MANAGER SESSION_MANAGER , *PSESSION_MANAGER;

NTSTATUS 
PROTO_Perform_Handshake(
					IN PSESSION_MANAGER pSessionManager
					);


// sends FTDCHUP command over to the other server
NTSTATUS 
PROTO_Send_HUP(
			IN PSESSION_MANAGER pSessionManger,
			IN LONG AckWanted
			);

// sends FTDACKHUP command over to the other server
NTSTATUS 
PROTO_Send_ACKHUP( 
				IN int iDeviceID,
				IN PSESSION_MANAGER pSessionManger,
				IN LONG AckWanted
				);

// sends FTDCRFFSTART command over to the other server
NTSTATUS 
PROTO_Send_RFFSTART(
				IN PSESSION_MANAGER pSessionManger,
				IN LONG AckWanted
				);

// sends FTDACKRFSTART command over to the PRIMARY server
NTSTATUS 
PROTO_Send_ACKRFSTART( 
					IN PSESSION_MANAGER pSessionManger,
					IN LONG AckWanted
					);

// sends FTDCRFFEND command over to the secondary server
NTSTATUS 
PROTO_Send_RFFEND(
				IN int iLgnum, 
				IN PSESSION_MANAGER pSessionManger,
				IN LONG AckWanted
				);

// sends FTDCCHKSUM command over to the secondary server
NTSTATUS 
PROTO_Send_CHKSUM( 
				IN  int iLgnumIN,
				IN  int iDeviceId, 
				IN  ftd_dev_t* ptrDeviceStructure,
				OUT PVOID* ptrDeltaMap,
				IN PSESSION_MANAGER pSessionManger,
				IN LONG AckWanted
				);

// sends FTDACKCHKSUM command over to the primary server
NTSTATUS 
PROTO_Send_ACKCHKSUM(
					IN  ftd_dev_t* ptrDeviceStructure,
					IN	int iDeltamap_len,
					IN	PVOID ptrDeltaMap,
					IN	PSESSION_MANAGER pSessionManger,
					IN LONG AckWanted
					);

// sends FTDACKCPSTART command over to the other server
NTSTATUS 
PROTO_Send_ACKCPSTART(
					IN PSESSION_MANAGER pSessionManger,
					IN LONG AckWanted
					);

// sends FTDACKCPSTOP command over to the other server
NTSTATUS 
PROTO_Send_ACKCPSTOP(
				IN PSESSION_MANAGER pSessionManger,
				IN LONG AckWanted
				);

// sends FTDACKCPONERR command over to the other server
NTSTATUS 
PROTO_Send_ACKCPONERR(
					IN PSESSION_MANAGER pSessionManger,
					IN LONG AckWanted
					);

// sends FTDACKCPOFFERR command over to the other server
NTSTATUS 
PROTO_Send_ACKCPOFFERR(
					IN PSESSION_MANAGER pSessionManger,
					IN LONG AckWanted
					);

// Is sent as part of Handshake mechanism when starting the PMD. 
// If RMD is not alive it is spawned. sends FTDCNOOP command over to the other server
NTSTATUS 
PROTO_Send_NOOP(
			IN PSESSION_MANAGER pSessionManger,
			IN LONG LGnum,
			IN LONG AckWanted
			);

// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns the Remote Version String. sends FTDCVERSION command over to the other server
NTSTATUS 
PROTO_Send_VERSION(IN int iLgnumIN,
				  IN PCHAR strConfigFile,
				  OUT PCHAR* strSecondaryVersion,
				  IN PSESSION_MANAGER pSessionManger, 
				  IN LONG AckWanted
				  );

static void
ftd_sock_encode_auth(ULONG ts, PCHAR hostname, ULONG hostid, ULONG ip,
            int *encodelen, PCHAR encode);

// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns STATUS_SUCCESS indicating whether the Secondary is in CP mode or not.
NTSTATUS 
PROTO_Send_HANDSHAKE(
					IN unsigned int nFlags,
					IN ULONG ulHostID,
					IN CONST PCHAR strConfigFilePath,
					IN CONST PCHAR strLocalHostName,
					IN ULONG ulLocalIP,
					OUT int* CP,
					IN PSESSION_MANAGER pSessionManger,
					IN LONG AckWanted
					);

// This ack is prepared and sent from the RMD in response to FTDCHANDSHAKE command
NTSTATUS 
PROTO_Send_ACKHANDSHAKE(
						IN unsigned int nFlags, 
						IN int iDeviceID, 
						IN PSESSION_MANAGER pSessionManger,
						IN LONG AckWanted
						);

// Is sent as part of Handshake mechanism when starting the PMD. 
// For each device in the group, one command is sent. If the remote side can not find the device 
// specified, an error is returned.
NTSTATUS 
PROTO_Send_CHKCONFIG(
					IN int iDeviceID,
					IN dev_t ulDevNum,
					IN dev_t ulFtdNum,
					IN CONST PCHAR strRemoteDeviceName,
					OUT PULONG ulRemoteDevSize,
					OUT PULONG ulDevId,
					IN PSESSION_MANAGER pSessionManger,
					IN LONG AckWanted
					);

// This ack is prepared and sent from the RMD in response to FTDCCHKCONFIG command
NTSTATUS 
PROTO_Send_ACKCONFIG(
					IN int iSize, 
					IN int iDeviceID, 
					IN PSESSION_MANAGER pSessionManger,
					IN LONG AckWanted
					);

// Sends the CPONERR if error happended on Check Point ON
NTSTATUS 
PROTO_Send_CPONERR(
				IN PSESSION_MANAGER pSessionManger,
				IN LONG AckWanted
				);

// Sends the CPOFFSERR if error happened on Check Point OFF
NTSTATUS 
PROTO_Send_CPOFFERR(
					IN PSESSION_MANAGER pSessionManger,
					IN LONG AckWanted
					);


//NTSTATUS
//COM_SendVector( IN SFTK_IO_VECTOR iovector[], IN ULONG length, IN PSESSION_MANAGER pSessionManger );

NTSTATUS
PROTO_RecvVector_Version( 
						OUT PCHAR* strSecondaryVersion, 
						IN PSESSION_MANAGER pSessionManger
						);

NTSTATUS
PROTO_RecvVector_Handshake( 
						OUT int* CP, 
						IN PSESSION_MANAGER pSessionManger
						);

NTSTATUS
PROTO_RecvVector_CheckConfig( 
							OUT PULONG ulRemoteDevSize, 
							OUT PULONG ulDevId, 
							IN PSESSION_MANAGER pSessionManger
							);

NTSTATUS
PROTO_RecvVector_CheckSum( 
						OUT PVOID* ptrDeltaMap, 
						IN PSESSION_MANAGER pSessionManger
						);


#endif //_SFTK_PROTOCOL_H_