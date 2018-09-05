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
 * network.h - DataStar Primary / Remote Mirror Daemon network communications
 *             datatypes and prototypes
 *
 * (c) Copyright 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved
 *
 *
 * History:
 *   11/15/96 - Steve Wahl - original code
 *
 ***************************************************************************
 */

#ifndef _NETWORK_H
#define _NETWORK_H 1

#ifdef NEED_BIGINTS
#include "bigints.h"
#endif
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <malloc.h>
#include <sys/time.h>
#include <unistd.h>

#include <poll.h>
#include <limits.h>

#if defined(_AIX)
#include <sys/un.h>
#endif /* defined(_AIX) */

#if defined(SOLARIS)
#include <sys/select.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/fs/ufs_fs.h>
#endif

#if defined(linux) 
#include "ftdio.h"
#include <time.h>
#endif /* defined(linux) */

/* WR PROD4508 and HIST WR 38281: the following switch activates a fix for these WRs,
   in which the PMD sends a message to the RMD, requesting verification against
   mounted target devices, to prevent launching Full, Checksum, Restartable or Smart Refresh.
   At the moment, this fix is turned OFF.
*/
/* #define CHECK_IF_RMD_MIRRORS_MOUNTED */

#define MAGICACK 0xbabeface
#define MAGICERR 0xdeadbabe
#define MAGICHDR 0xcafeface
#define MAGICJRN 0xdeaddead

#define CMDHANDSHAKE 1
#define CMDCHKCONFIG 2
#define CMDNOOP      3
#define CMDVERSION   4
#define CMDFAILOVER	 5

#define CMDWRITE     50
#define CMDHUP       51 
#define CMDCPONERR   52 
#define CMDCPOFFERR  53 

#define CMDEXIT      1000

#define CMDBFDSTART  100
#define CMDBFDDEVS   101
#define CMDBFDDEVE   102
#define CMDBFDREAD   103
#define CMDBFDEND    104
#define CMDBFDLGE    105
#define CMDBFDCHKSUM 106

#define CMDRFDFSTART 110
#define CMDRFDDEVS   111
#define CMDRFDDEVE   112
#define CMDRFDFWRITE 114
#define CMDRFDEND    115
#define CMDRFDFEND   116
#define CMDRFDCHKSUM 117
#define CMDRFDFSWRITE 118

#define CMDMSGINCO   120
#define CMDMSGCO     121
#define CMDCLEARBITS 122
#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED
#define CMDCKMIRSMTD 123
#endif

#define CMDFO_BOOT_DRIVE              0x00010000 // Bit flag for CMDFAILOVER
#define CMDVER_READ_RMD_CFG_FILE      0x00020000 // Bit flag for CMDVERSION
#define CMDFO_SHUTDOWN_SOURCE         0x00040000 // Bit flag for CMDFAILOVER
#define CMDFO_KEEP_AIX_TARGET_RUNNING 0x00080000 // Bit flag for CMDFAILOVER


#define ACKNORM        1
#define ACKERR         2
#define ACKNOOP        3
#define ACKRFD         4
#define ACKBFD         5
#define ACKBFDDELTA    6
#define ACKRFDCHKSUM   7
#define ACKCHUNK       8
#define ACKRFDCMD      10
#define ACKBFDCMD      11
#define ACKHUP         12
#define ACKCPSTART     14
#define ACKCPSTOP      15
#define ACKCPON        16
#define ACKCPOFF       17
#define ACKCPOFFERR    18
#define ACKCPONERR     19
#define ACKKILLPMD     20 
#define ACKRFDF        21 
#define ACKMSGCO       22
#define ACKCLEARBITS   23
#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED
#define RFDF_TGT_MNTED       24
#define RFDF_TGT_NOTMNTED    25
#endif
#define UNCLEAN_SHUTDOWN_RMD 26
#define ACKNOLICENSE         27
#define RMDA_CORRUPTED_JOURNAL 28

#define BLOCKALLZERO -1 

/* message types for 2ndary journal */
#define MSG_INCO     "MIRROR_INCOHERENT"
#define MSG_CO       "MIRROR_COHERENT"
#define MSG_RFD      "REFRESH"
#define MSG_RFDF     "FULL_REFRESH"
#define MSG_CPON     "CHECKPOINT_ON"
#define MSG_CPOFF    "CHECKPOINT_OFF"
#define SIZE 2048

#if defined(FTD_IPV4)
#define IP   mysys->ip 
#define RIP  othersys->ip
#else
#define IP   (u_long)mysys->ipv
#define RIP  (u_long)othersys->ipv
#endif 

#define FREFRESH_BACKWARD_ADJUSTMENT  ((double)0.25)  /* 25 percent */
/* At the moment, if Full Refresh stopped by RMD crash, force complete restart */
#define FORCE_COMPLETE_RESTART  1

int rmd64;
int pmd64;

/* -- ackpack: RMD->PMD acknowledgement structure */
/* structure for communication between different
 * versions of PMD and RMD 
 */
typedef struct ackpack32 {
  u_long             magicvalue;  /* indicate type of ACK (ACK or ERR) */
  int                acktype;     /* type of ack */
  u_longlong_t       lgsn;        /* local group sequence number */
  time_t             ts;          /* timestamp packet was initially sent */
  int                devid;       /* device id for CMDWRTIE */
  off_t              ackoff;      /* offset (sectors) from WL tail */
  int                cpon;        /* checkpoint on */
  int                mirco;       /* mirror(s) are coherent */
  u_long             data;        /* misc data returned by RMD */
  u_long             bab_oflow;   /* BAB oflow count received from PMD in CMDWRITE for sync mode processing */
} ackpack32_t;
typedef struct ackpack {
  u_long             magicvalue;  /* indicate type of ACK (ACK or ERR) */
  int                acktype;     /* type of ack */
  u_longlong_t       lgsn;        /* local group sequence number */
  time_t             ts;          /* timestamp packet was initially sent */
  int                devid;       /* device id for CMDWRTIE */
  u_longlong_t       ackoff;      /* offset (sectors) from WL tail */ /* Misnomer: Never an offset, usually a length. */
  int                cpon;        /* checkpoint on */
  int                mirco;       /* mirror(s) are coherent */
  u_longlong_t       data;        /* misc data returned by RMD */
  u_long             bab_oflow;   /* BAB oflow count received from PMD in CMDWRITE for sync mode processing */
  u_longlong_t       write_ackoff;/* offset (sectors) when acking CMDRFDFWRITE. (ACKRFDF or ACKRFD) */

} ackpack_t;

/* -- errpack: RMD->PMD structure following an ackpack containing a MAGICERR */
typedef struct errpack {
  u_long             magicvalue;  /* indicate that this is an error packet */
  u_long             errcode;     /* error severity code */
  char               errkey[12];  /* error mnemonic key */
  u_long             length;      /* length of error message string */
  char               msg[256];    /* error message describing error */
} errpack_t;

/* -- headpack: PMD->RMD standard message structure */
/* structure for communication between different
 * versions of PMD and RMD
 */
typedef struct headpack32 {
  u_long             magicvalue;  /* indicate that this is a header packet */
  u_short            cmd;         /* command code to RMD */
  u_longlong_t       lgsn;        /* logical group sequence number */
  time_t             ts;          /* timestamp transaction was sent */
  int                ackwanted;   /* send ack after processing */
  int                devid;       /* device id for CMDWRTIE */
  size_t             len;         /* buffer size (bytes) for CMDWRITE */
  off_t              offset;      /* offset on device (sectors) for CMDWRITE */
  int                compress;    /* compression scheme */
  u_long             decodelen;   /* un-compressed data length */
  u_long             data;        /* misc. data */
  u_long             bab_oflow;   /* BAB oflow count sent by PMD in CMDWRITE for sync mode processing */
} headpack32_t;
typedef struct headpack {
  u_long             magicvalue;  /* indicate that this is a header packet */
  u_short            cmd;         /* command code to RMD */
  u_longlong_t       lgsn;        /* logical group sequence number */
  time_t             ts;          /* timestamp transaction was sent */
  int                ackwanted;   /* send ack after processing */
  int                devid;       /* device id for CMDWRTIE */
  size_t             len;         /* buffer size (bytes) for CMDWRITE */
  u_longlong_t       offset;      /* offset on device (sectors) for CMDWRITE */
  int                compress;    /* compression scheme */
  u_long             decodelen;   /* un-compressed data length */
  u_long             data;        /* misc. data */
  u_long             bab_oflow;   /* BAB oflow count sent by PMD in CMDWRITE for sync mode processing */
} headpack_t;

typedef struct authpack {
  int len;
  char auth[512];
  char configpath[256];
} authpack_t;

typedef struct rdevpack {
  int devid;
  dev_t dd_rdev;
  dev_t sd_rdev;
  int len;
  char path[256];
} rdevpack_t;

typedef struct reqlist {
  struct reqlist *next;
  headpack_t header;
  void *datap;
} reqlist_t;

typedef struct cfgfilepack {
  int file_length;
  char file_data[2048];
} cfgfilepack_t;

typedef struct versionpack {
  time_t pmdts;
  char configpath[256];
  char version[128];	/* make the string shorter from 256 to 128 bytes */
  int jless;    	/* notify RMD about jless mode. 1: jless on */
  cfgfilepack_t cfg_file; /* To allow sending the config file to the RMD with the version command */
  int resv[31];		/* reserved for future use */
} versionpack_t;

/* -- tunable parameters */
extern int chunksize;       /* 256 Kbyte */
extern u_long nodatasleep;  /* microseconds */
extern int syncinterval;    /* seconds */
extern int synccount;       /* number of entries */
extern int statinterval;    /* seconds */
extern u_long chunkdelay;   /* microseconds */ 
extern int _net_bandwidth_analysis;


/* PMD -> RMD interface */
extern int createconnection (int reporterrflag);
extern void closeconnection (void);
extern int sendack (int fd, headpack_t*, ackpack_t*);
extern int senderr (int fd,  headpack_t*, ulong data,
		    u_long errclass, char* errname, char* errmsg);
extern int sendversion (int fd, int network_analysis_mode);
extern int sendhandshake (int fd, int* rmda_corrupted_journal_found);
extern int sendconfiginfo (int fd);
extern int sendchunk (int fd, group_t* lg);
extern int migrate (int bytes);
extern int sendnoop (void);
extern int sendclearbits(void);

/* RMD -> PMD interface */
extern void waitforconnect (pid_t* childpid);
extern void processrequests (void*);

extern int readitem (int fd);
extern int doauth (authpack_t* auth, time_t* ts);
extern void decodeauth (int encodelen, char* encode, time_t ts, 
			u_long hostid, char* hostname, 
	                size_t hostname_len, u_long );
extern int doconfiginfo (rdevpack_t* rdev, time_t);
extern int doversioncheck (int fd, headpack32_t *header32, versionpack_t* version);

/* Both PMD/RMD interface */
extern int initnetwork (); 
extern int netread (int fd, char* buf, int len);
extern int writesock (int fd, char* buf, int len);
extern int readsock (int fd, char* buf, int len);
extern int checkresponse (int fd, ackpack_t *ack);
extern int checksock (int fd, int op, struct timeval *seltime);

extern int chksumdiff(void);
extern int rsyncwrite_bfd(rsync_t *lrsync);
extern int net_test_channel(int connect_timeo);
extern int send_cp_err_ack(int msgtype);
extern int sendhup(void);
extern int process_acks(void);
extern int send_cp_err(int msgtype);
extern int send_cp_on(int lgnum);
extern int send_cp_off(int lgnum);
extern int eval_netspeed(void);
extern int net_write_vector(int fd, struct iovec *iov, int iovcnt, int role);
extern int set_tcp_send_low(void);
extern int set_tcp_nonb(void);
extern int flush_net (int fd);
extern int sendkill (int fd);
extern int send_no_lic_msg (int fd);
extern int sendfailover (int fd, int group_number, int boot_drive_migration, int shutdown_source, int keep_AIX_target_running);
extern void remove_fictitious_cfg_files( int lgnum, int is_pmd );
extern int is_network_analysis_cfg_file( char *cfg_filename );

#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED   /* WR PROD4508 and HIST WR 38281 */
extern void ask_if_RMD_mirrors_mounted( void );
#endif

int converthdrfrom64to32(headpack_t* header, headpack32_t* header32);
int converthdrfrom32to64(headpack32_t* header32, headpack_t* header);
int convertackfrom64to32 (ackpack_t* ack, ackpack32_t* ack32);
int convertackfrom32to64 (ackpack32_t* ack32, ackpack_t* ack);
int convertrsyncfrom64to32 (rsync_t* rsync, rsync32_t* rsync32);
int convertrsyncfrom32to64 (rsync32_t* rsync32, rsync_t* rsync);
int convertwlhfrom64to32 ();
int convertwlhfrom32to64 (headpack_t *lheader, int *newsize, char *newchunk);

/* Sequencer method*/
extern void Sequencer_Inc(int* piSeq);
extern void Sequencer_Set(int* piSeq, int iSeqVal);
extern int  Sequencer_Get(int* piSeq);
extern void Sequencer_Reset(int* piSeq);
extern void SequencerTag_Set(int* piTag, int iSeqVal);
extern void SequencerTag_Invalidate(int* piTag);
extern int  SequencerTag_IsValid(int iTag);
extern int  SequencerTag_IsReached(int iTag, int iSequencerValue);

/* Corrupted journal found flag file. */
int check_corrupted_journal_flag( int lgnum );
void remove_corrupted_journal_flag( int lgnum );
void flag_corrupted_journal( int lgnum );

/* WR 43926: for sync mode, a counter of BAB overflows follows the chuncks
   for PMD to determine if BAB has been cleared between sending a chunk and
   receiving the corresponding ack; in which case data cannot be migrated off the BAB */
extern unsigned int  BAB_oflow_counter;

/* The following has been added because 
 * Linux does not recognise ULLONG_MAX 
 * without __USE_ISOC99 being defined
 */
#if defined (linux)
#ifndef ULLONG_MAX
#define ULLONG_MAX   18446744073709551615ULL
#endif /* ULLONG_MAX */
#endif /* (linux) */
#endif /* _NETWORK_H */
