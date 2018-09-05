/*
 * ftd_sock.h - FTD socket interface
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
#ifndef _FTD_SOCK_H
#define _FTD_SOCK_H

#include "ftd_port.h"
#include "ftd_lg.h"
#include "ftd_dev.h"
#include "ftd_error.h"

#define	FTD_SOCK_CONNECT_SHORT_TIMEO	1
#define	FTD_SOCK_CONNECT_LONG_TIMEO		10
#define	FTD_NET_SEND_NOOP_TIME			20
#define	FTD_NET_MAX_IDLE				30	

#define   FTD_MAX_TCP_WINDOW_SIZE     (16*1024*1024) // 16 MB

/* protocol magic numbers */
#define MAGICACK		0xbabeface
#define MAGICERR		0xdeadbabe
#define MAGICHDR		0xcafeface

/* ftd connection types */
enum contypes {

FTD_CON_UTIL = 0,
FTD_CON_IPC,
FTD_CON_PMD,
FTD_CON_CHILD

};

/* ftd socket macros */
#define	FTD_SOCK_FD(fsockp)	(SOCKET)((fsockp)->sockp->sock)

#define	FTD_SOCK_LHOST(fsockp)	(fsockp)->sockp->lhostname
#define	FTD_SOCK_RHOST(fsockp)	(fsockp)->sockp->rhostname

#define	FTD_SOCK_LIP(fsockp)	(fsockp)->sockp->lip
#define	FTD_SOCK_RIP(fsockp)	(fsockp)->sockp->rip

#define	FTD_SOCK_PORT(fsockp)	(fsockp)->sockp->port
#define	FTD_SOCK_FLAGS(fsockp)	(fsockp)->sockp->flags

#define	FTD_SOCK_VALID(fsockp) \
	( (fsockp) != NULL && (fsockp)->magicvalue == FTDSOCKMAGIC )
	
#define	FTD_SOCK_CONNECT(fsockp) \
	(FTD_SOCK_VALID((fsockp)) && GET_SOCK_CONNECT((fsockp)->sockp->flags))

#define	FTD_SOCK_KEEPALIVE(fsockp) \
	(FTD_SOCK_VALID((fsockp)) && (fsockp)->keepalive)

#if defined(_WINDOWS)
#define	FTD_SOCK_HEVENT(fsockp)	(fsockp)->sockp->hEvent
#endif

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
FTDCREFRSRELAUNCH,

/* PMD connection */
FTDCPMDCONNECTION,

/* RMD connection */
FTDCRMDCONNECTION

};


//#ifdef  TDMF_TRACE
	
	struct tdmf_trace_text
		{
		int  cmd;
		char cmd_txt[30];
		};
    //defined in ftd_sock.c
	extern struct tdmf_trace_text	tdmf_socket_msg[];															  

//	#ifdef TDMF_TRACE_MAKER
//	#else
//		extern struct tdmf_trace_text	tdmf_socket_msg[];															  
//	#endif																  
//#endif															  
																  
#define FTDZERO			-1 

/* statndard error packet structure */
typedef struct ftd_err_s {
	unsigned long errcode;			/* error severity code                 */
	char errkey[12];				/* error mnemonic key                  */
	unsigned long length;			/* length of error message string      */
	char msg[MAXERR];				/* error message describing error      */
	char fnm[MAXPATH];				/* source file name					*/
	int lnum;						/* line number						*/
} ftd_err_t;

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

		
#define	FTD_MSGSIZE				32	// this makes it a 64 byte header	

/* standard message header structure */
typedef struct ftd_header_s {
	unsigned long magicvalue;		/* indicate that this is a
									 header packet */
	time_t ts;						/* timestamp transaction was sent      */
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

typedef union {
	u_char uc[4];
	u_long ul;
} encodeunion;

typedef struct ftd_auth_s {
	int len;
	char auth[512];
	char configpath[MAXPATH];
} ftd_auth_t;

typedef struct ftd_rdev_s {
	int devid;
	dev_t ftd;
	dev_t minor;
	int len;
	char path[MAXPATH];
} ftd_rdev_t;

typedef struct ftd_version_s {
	time_t pmdts;
	int tsbias;
	char configpath[MAXPATH];
	char version[256];
} ftd_version_t;

/* external prototypes */
extern LList *ftd_sock_create_list(void);
extern int ftd_sock_add_to_list(LList *socklist, ftd_sock_t **fsockp);
extern int ftd_sock_remove_from_list(LList *socklist, ftd_sock_t **fsockp);
extern int ftd_sock_delete_list(LList *socklist);
extern ftd_sock_t *ftd_sock_create(int type);
extern int ftd_sock_delete(ftd_sock_t **fsockpp);
extern int ftd_sock_init(ftd_sock_t *fsockp, char *lhostname,
	char *rhostname, unsigned long lip, unsigned long rip,
	int type, int family, int create, int verifylocal);
extern void ftd_sock_set_connect(ftd_sock_t *fsockp);
extern void ftd_sock_set_disconnect(ftd_sock_t *fsockp);
extern int ftd_sock_connect(ftd_sock_t *fsockp, int port);
extern int ftd_sock_connect_nonb(ftd_sock_t *fsockp, int port,
	int sec, int usec, int silent);
extern int ftd_sock_disconnect(ftd_sock_t *fsockp);
extern int ftd_sock_bind(ftd_sock_t *fsockp, int port);
extern int ftd_sock_listen(ftd_sock_t *fsockp, int port);
extern int ftd_sock_accept(ftd_sock_t *listener, ftd_sock_t *sockp);
extern int ftd_sock_accept_nonb(ftd_sock_t *listener,
	ftd_sock_t *sockp, int sec, int usec);

extern int ftd_sock_set_opt(ftd_sock_t *fsockp, int level,
	int optname, char *optval, int optlen);
extern int ftd_sock_get_opt(ftd_sock_t *fsockp, int level,
	int optname, char *optval, int *optlen);

extern int ftd_sock_send ( char tracing,ftd_sock_t *fsockp, char *buf, int len);
extern int ftd_sock_send_lg(ftd_sock_t *fsockp, ftd_lg_t *lgp,
	char *buf, int len);
extern int ftd_sock_recv(ftd_sock_t *fsockp, char *buf, int len);

extern int ftd_sock_send_vector(ftd_sock_t *fsockp, struct iovec *iov,
	int iovcnt);

extern int ftd_sock_check_connect(ftd_sock_t *fsockp);
extern int ftd_sock_check_recv(ftd_sock_t *fsockp, int timeo);
extern int ftd_sock_check_send(ftd_sock_t *fsockp, int timeo);

extern int ftd_sock_set_nonb(ftd_sock_t *fsockp);
extern int ftd_sock_set_b(ftd_sock_t *fsockp);

extern int ftd_sock_is_me(ftd_sock_t *fsockp, int local);

#ifdef TDMF_TRACE
	extern int ftd_sock_send_header(char tracing, char *mod,char *funct,ftd_sock_t *fsockp, ftd_header_t *header);
	#define FTD_SOCK_SEND_HEADER(a,b,c,d,e) 	ftd_sock_send_header(a,b,c,d,e)
#else
	extern int ftd_sock_send_header(char tracing,ftd_sock_t *fsockp, ftd_header_t *header);
	#define FTD_SOCK_SEND_HEADER(a,b,c,d,e) 	ftd_sock_send_header(FALSE,d,e)
#endif

	   
#ifdef TDMF_TRACE
	extern int ftd_sock_recv_header(char *mod,char *funct,ftd_sock_t *fsockp, ftd_header_t *header);
	#define FTD_SOCK_RECV_HEADER(a,b,c,d) 	ftd_sock_recv_header(a,b,c,d)
#else
extern int ftd_sock_recv_header(ftd_sock_t *fsockp, ftd_header_t *header);
	#define FTD_SOCK_RECV_HEADER(a,b,c,d) 	ftd_sock_recv_header(c,d)
#endif

extern int _ftd_sock_send_and_report_err(ftd_sock_t *fsockp, char *errkey,
	int severity, ...);

#define	ftd_sock_send_and_report_err	\
err_glb_fnm = __FILE__; err_glb_lnum = __LINE__; _ftd_sock_send_and_report_err

extern int ftd_sock_send_err(ftd_sock_t *fsockp, err_msg_t *errp);

extern int ftd_sock_recv_err(ftd_sock_t *fsockp, ftd_header_t *header);

extern int ftd_sock_send_noop(ftd_sock_t *fsockp, int lgnum, int ackwanted); 
extern int ftd_sock_recv_noop(ftd_sock_t *fsockp, ftd_header_t *header); 

extern int ftd_sock_send_refresh_block(ftd_sock_t *fsockp, ftd_lg_t *lgp,
	ftd_dev_t *devp, int ackwanted); 
extern int ftd_sock_recv_refresh_block(ftd_sock_t *fsockp,
	ftd_header_t *header, ftd_lg_t *lgp); 

extern int ftd_sock_send_backfresh_block(ftd_sock_t *fsockp, ftd_lg_t *lgp,
	ftd_dev_t *devp, int ackwanted); 

extern int ftd_sock_send_rsync_block(ftd_sock_t *fsockp, ftd_lg_t *lgp, 
	ftd_dev_t *devp, int ackwanted);

extern int ftd_sock_send_rsync_start(ftd_sock_t *fsockp, ftd_lg_t *lgp,
	int state); 
extern int ftd_sock_recv_rsync_start(ftd_header_t *header, ftd_lg_t *lgp); 

extern int ftd_sock_send_rsync_end(ftd_sock_t *fsockp, ftd_lg_t *lgp,
	int state); 
extern int ftd_sock_recv_rsync_end(int msgtype, ftd_lg_t *lgp); 

extern int ftd_sock_send_rsync_devs(ftd_sock_t *fsockp, ftd_lg_t *lgp,
	ftd_dev_t *devp, int state); 
extern int ftd_sock_recv_rsync_devs(ftd_sock_t *fsockp,
	ftd_header_t *header, ftd_lg_t *lgp); 

extern int ftd_sock_send_rsync_deve(ftd_sock_t *fsockp, ftd_lg_t *lgp,
	ftd_dev_t *devp, int state); 
extern int ftd_sock_recv_rsync_deve(ftd_sock_t *fsockp,
	ftd_header_t *header, ftd_lg_t *lgp); 

extern int ftd_sock_send_rsync_chksum(ftd_sock_t *fsockp, ftd_lg_t *lgp, 
	LList *devlist, int msgtype);
extern int ftd_sock_recv_refresh_chksum(ftd_sock_t *fsockp,
	ftd_header_t *header, ftd_lg_t *lgp); 
extern int ftd_sock_recv_backfresh_chksum(ftd_sock_t *fsockp,
	ftd_header_t *header, ftd_lg_t *lgp); 

extern int ftd_sock_send_bab_chunk(ftd_lg_t *lgp); 
extern int ftd_sock_recv_bab_chunk(ftd_sock_t *fsockp, ftd_header_t *header,
	ftd_lg_t *lgp); 

extern int ftd_sock_send_hup(ftd_lg_t *lgp); 
extern int ftd_sock_recv_hup(ftd_sock_t *fsockp, ftd_header_t *header); 

extern int ftd_sock_send_version(ftd_sock_t *fsockp, ftd_lg_t *lgp); 
extern int ftd_sock_recv_version(ftd_sock_t *fsockp,
	ftd_header_t *header, ftd_lg_t *lgp); 
extern int ftd_sock_recv_refoflow(ftd_lg_t *lgp); 

extern int ftd_sock_send_handshake(ftd_lg_t *lgp); 
extern int ftd_sock_recv_handshake(ftd_sock_t *fsockp,
	ftd_header_t *header, ftd_lg_t *lgp); 

extern int ftd_sock_send_chkconfig(ftd_lg_t *lgp); 
extern int ftd_sock_recv_chkconfig(ftd_sock_t *fsockp,
	ftd_header_t *header, ftd_lg_t *lgp); 

extern int ftd_sock_send_exit(ftd_sock_t *fsockp); 
extern int ftd_sock_recv_exit(ftd_sock_t *fsockp, ftd_header_t *header); 

extern int ftd_sock_send_start_apply(int lgnum, ftd_sock_t *fsockp,
	int cpon); 
extern int ftd_sock_send_start_reco(int lgnum, ftd_sock_t *fsockp, int force); 

extern int ftd_sock_send_stop_apply(int lgnum, ftd_sock_t *fsockp);
extern int ftd_sock_recv_stop_apply(ftd_sock_t *listener, int lgnum); 
 
extern int ftd_sock_send_start_pmd(int lgnum, ftd_sock_t *fsockp); 

extern int ftd_sock_recv_lg_msg(ftd_sock_t *fsockp, ftd_lg_t *lgp, int timeo);

extern int ftd_sock_recv_cpstart(ftd_lg_t *lgp, ftd_header_t *header); 
extern int ftd_sock_recv_cpstop(ftd_lg_t *lgp, ftd_header_t *header); 
extern int ftd_sock_recv_cpon(void); 
extern int ftd_sock_recv_cpoff(void); 
extern int ftd_sock_recv_cponerr(ftd_sock_t *fsockp, ftd_lg_t *lgp); 
extern int ftd_sock_recv_cpofferr(ftd_sock_t *fsockp, ftd_lg_t *lgp); 

extern int ftd_sock_recv_ack_backfresh(ftd_sock_t *fsockp, ftd_header_t *header, ftd_lg_t *lgp); 
extern int ftd_sock_recv_ack_refresh(ftd_sock_t *fsockp, ftd_header_t *header, ftd_lg_t *lgp); 

extern int ftd_sock_recv_ack_kill(void); 
extern int ftd_sock_recv_drive_err(ftd_lg_t *lgp, ftd_header_t *header);  
extern int ftd_sock_recv_ack_back_delta(ftd_sock_t *fsockp,
	ftd_header_t *header, ftd_lg_t *lgp); 
extern int ftd_sock_recv_ack_cponerr(ftd_sock_t *fsockp, ftd_lg_t *lgp); 
extern int ftd_sock_recv_ack_cpofferr(ftd_sock_t *fsockp, ftd_lg_t *lgp); 
	
extern int ftd_sock_test_link(char *lhostname, char *rhostname,
	unsigned long lip, unsigned long rip, int port, int connect_timeo);

extern int ftd_sock_get_port(char *service);
extern int ftd_sock_flush(ftd_lg_t *lgp);

#if defined(_WINDOWS)
extern int ftd_sock_startup(void);
extern int ftd_sock_cleanup(void);
#endif

extern int ftd_sock_connect_forever(ftd_lg_t *lgp, int port);

extern int ftd_sock_wait_for_peer_close(ftd_sock_t *fsockp);

#endif

