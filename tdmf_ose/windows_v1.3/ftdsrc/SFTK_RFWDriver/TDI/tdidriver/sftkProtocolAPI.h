/*
 * sftkProtocolAPI.h - Protocol APIs
 * 
 * Copyright (c) 2004 Softek Storage Solutions, Inc.  All Rights Reserved.
 * Confidential property of Softek Storage Solutions, Inc. May not be used or 
 * distributed without proper authorization.
 *
 *
 * AUTHOR:	Saumya Tripathi
 * Date:	May 24, 2004
 */

#ifndef _SFTKPROTOCOLAPI_H_
#define _SFTKPROTOCOLAPI_H_

#include            "NTDDK.h"
#include			"NDIS.H"
#include			"tdiutil.h"
#include			"stdio.h"
#include			"types.h"
//#include			"..\lib\liblst\llist.h"
// #include			"..\lib\libftd\ftd_dev.h"
//#include			"..\lib\libftd\ftd_sock.h"
//#include			"..\lib\libftd\ftd_lg.h"

#define	FTD_MSGSIZE				32	// this makes it a 64 byte header
#define MAX_PATH				260
#define MAXPATH					MAX_PATH

/* DEV_BSHIFT is set for 512 bytes */
#define DEV_BSHIFT  9
#define DEV_BSIZE   512

#define CHKSEGSIZE			32768
#define	DIGESTSIZE			16

/* protocol magic numbers */
#define MAGICACK		0xbabeface
#define MAGICERR		0xdeadbabe
#define MAGICHDR		0xcafeface

typedef __int64					ftd_int64_t;
typedef unsigned __int64		ftd_uint64_t;
typedef __int64					offset_t;

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

/* peer <-> peer protocol */
enum protocol {

FTDCHANDSHAKE =1, 
FTDCCHKCONFIG,	   
FTDCNOOP,		   
FTDCVERSION,	   
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
FTDCREFRSRELAUNCH
};

#define LG_BYTE_0(x)			(((unsigned char*)&(x))[0])
#define LG_BYTE_1(x)			(((unsigned char*)&(x))[1])
#define LG_BYTE_2(x)			(((unsigned char*)&(x))[2])
#define LG_BYTE_3(x)			(((unsigned char*)&(x))[3])

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

#define VERSIONSTR		"5.0.0"

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

// sends FTDCHUP command over to the other server
NTSTATUS Send_HUP(
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKHUP command over to the other server
NTSTATUS Send_ACKHUP( 
IN int iDeviceID,
IN PSESSION_MANAGER pSessionManger
);

// sends FTDCRFFSTART command over to the other server
NTSTATUS Send_RFFSTART(
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKRFSTART command over to the PRIMARY server
NTSTATUS Send_ACKRFSTART( 
IN PSESSION_MANAGER pSessionManger 
);

// sends FTDCRFFEND command over to the secondary server
NTSTATUS Send_RFFEND(
IN int iLgnum, 
IN PSESSION_MANAGER pSessionManger
);

// sends FTDCCHKSUM command over to the secondary server
NTSTATUS Send_CHKSUM( 
IN  int iLgnumIN,
IN  int iDeviceId, 
IN  ftd_dev_t* ptrDeviceStructure,
OUT PVOID* ptrDeltaMap,
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKCHKSUM command over to the primary server
NTSTATUS Send_ACKCHKSUM(
IN  ftd_dev_t* ptrDeviceStructure,
IN	int iDeltamap_len,
IN	PVOID ptrDeltaMap,
IN	PSESSION_MANAGER pSessionManger
);

// sends FTDACKCPSTART command over to the other server
NTSTATUS Send_ACKCPSTART(
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKCPSTOP command over to the other server
NTSTATUS Send_ACKCPSTOP(
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKCPONERR command over to the other server
NTSTATUS Send_ACKCPONERR(
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKCPOFFERR command over to the other server
NTSTATUS Send_ACKCPOFFERR(
IN PSESSION_MANAGER pSessionManger
);

// Is sent as part of Handshake mechanism when starting the PMD. 
// If RMD is not alive it is spawned. sends FTDCNOOP command over to the other server
NTSTATUS Send_NOOP(
IN PSESSION_MANAGER pSessionManger
);

// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns the Remote Version String. sends FTDCVERSION command over to the other server
NTSTATUS Send_VERSION(IN int iLgnumIN,
OUT PCHAR* strSecondaryVersion,
IN PSESSION_MANAGER pSessionManger
);

static void
ftd_sock_encode_auth(ULONG ts, PCHAR hostname, ULONG hostid, ULONG ip,
            int *encodelen, PCHAR encode);

// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns STATUS_SUCCESS indicating whether the Secondary is in CP mode or not.
NTSTATUS Send_HANDSHAKE(IN unsigned int nFlags,
					  IN ULONG ulHostID,
					  IN CONST PCHAR strConfigFilePath,
					  IN CONST PCHAR strLocalHostName,
					  IN ULONG ulLocalIP,
					  OUT int* CP,
					  IN PSESSION_MANAGER pSessionManger);

// This ack is prepared and sent from the RMD in response to FTDCHANDSHAKE command
NTSTATUS Send_ACKHANDSHAKE(IN unsigned int nFlags, IN int iDeviceID, IN PSESSION_MANAGER pSessionManger);

// Is sent as part of Handshake mechanism when starting the PMD. 
// For each device in the group, one command is sent. If the remote side can not find the device 
// specified, an error is returned.
NTSTATUS Send_CHKCONFIG(IN int iDeviceID,
					  IN dev_t ulDevNum,
					  IN dev_t ulFtdNum,
					  IN CONST PCHAR strRemoteDeviceName,
					  OUT PULONG ulRemoteDevSize,
					  OUT PULONG ulDevId,
					  IN PSESSION_MANAGER pSessionManger);

// This ack is prepared and sent from the RMD in response to FTDCCHKCONFIG command
NTSTATUS Send_ACKCONFIG(
IN int iSize, 
IN int iDeviceID, 
IN PSESSION_MANAGER pSessionManger
);

NTSTATUS
SftkSendVector( IN SFTK_IO_VECTOR iovector[], IN ULONG length, IN PSESSION_MANAGER pSessionManger );

NTSTATUS
SftkRecvVector_Version( OUT PCHAR* strSecondaryVersion, IN PSESSION_MANAGER pSessionManger);

NTSTATUS
SftkRecvVector_Handshake( OUT int* CP, IN PSESSION_MANAGER pSessionManger);

NTSTATUS
SftkRecvVector_CheckConfig( OUT PULONG ulRemoteDevSize, OUT PULONG ulDevId, IN PSESSION_MANAGER pSessionManger);

NTSTATUS
SftkRecvVector_CheckSum( OUT PVOID* ptrDeltaMap, IN PSESSION_MANAGER pSessionManger);

#endif //_SFTKPROTOCOLAPI_H_