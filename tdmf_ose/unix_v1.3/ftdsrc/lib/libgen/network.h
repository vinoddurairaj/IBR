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
#include <stropts.h>
#include <poll.h>

#if defined(_AIX)
#include <sys/un.h>
#endif /* defined(_AIX) */

#if defined(SOLARIS)
#include <sys/select.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/fs/ufs_fs.h>
#endif

#define MAGICACK 0xbabeface
#define MAGICERR 0xdeadbabe
#define MAGICHDR 0xcafeface
#define MAGICJRN 0xdeaddead

#define CMDHANDSHAKE 1
#define CMDCHKCONFIG 2
#define CMDNOOP      3
#define CMDVERSION   4

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

#define BLOCKALLZERO -1 

/* message types for 2ndary journal */
#define MSG_INCO     "MIRROR_INCOHERENT"
#define MSG_CO       "MIRROR_COHERENT"
#define MSG_RFD      "REFRESH"
#define MSG_RFDF     "FULL_REFRESH"
#define MSG_CPON     "CHECKPOINT_ON"
#define MSG_CPOFF    "CHECKPOINT_OFF"

/* -- ackpack: RMD->PMD acknowledgement structure */
typedef struct ackpack {
  u_long             magicvalue;  /* indicate type of ACK (ACK or ERR) */
  int                acktype;     /* type of ack */
  u_longlong_t       lgsn;        /* local group sequence number */
  time_t             ts;          /* timestamp packet was initially sent */
  int                devid;       /* device id for CMDWRTIE */
  off_t              ackoff;      /* offset (sectors) from WL tail */
  int                cpon;        /* checkpoint on */
  int                mirco;       /* mirror(s) are coherent */
  u_long             data;        /* misc data returned by RMD */
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
typedef struct headpack {
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

typedef struct versionpack {
  time_t pmdts;
  char configpath[256];
  char version[256];
} versionpack_t;

extern char* databuf;             /* -- data buffer for RMD to read into */
extern int databuflen;            /* -- size of the RMD data buffer */

/* -- tunable parameters */
extern int chunksize;       /* 256 Kbyte */
extern u_long nodatasleep;  /* microseconds */
extern int syncinterval;    /* seconds */
extern int synccount;       /* number of entries */
extern int statinterval;    /* seconds */
extern u_long chunkdelay;   /* microseconds */ 

/* PMD -> RMD interface */
extern int createconnection (int reporterrflag);
extern void closeconnection (void);
extern int sendack (int fd, headpack_t*, ackpack_t*);
extern int senderr (int fd,  headpack_t*, ulong data,
		    u_long errclass, char* errname, char* errmsg);
extern int sendversion (int fd);
extern int sendhandshake (int fd);
extern int sendconfiginfo (int fd);
extern int sendchunk (int fd, group_t* lg);
extern int migrate (int bytes);
extern int sendnoop (void);

/* RMD -> PMD interface */
extern void waitforconnect (pid_t* childpid);
extern void *processrequests (void*);

extern int readitem (int fd);
extern int doauth (authpack_t* auth, time_t* ts);
extern void decodeauth (int encodelen, char* encode, time_t ts, 
			u_long hostid, char* hostname, 
	                size_t hostname_len, u_long ip);
extern int doconfiginfo (rdevpack_t* rdev, time_t);
extern int doversioncheck (int fd, headpack_t *header, versionpack_t* version);

/* Both PMD/RMD interface */
extern int initnetwork (void);
extern int netread (int fd, char* buf, int len);
extern int writesock (int fd, char* buf, int len);
extern int readsock (int fd, char* buf, int len);
extern int checkresponse (int fd, ackpack_t *ack);
extern int checksock (int fd, int op, struct timeval *seltime);

extern int chksumdiff(void);
extern int rsyncwrite_bfd(rsync_t *lrsync);

extern int net_test_channel(int connect_timeo);

#endif /* _NETWORK_H */
