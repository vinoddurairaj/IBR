/*
 * ftd_lg.h - FTD logical group interface
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
#ifndef _FTD_LG_H_
#define _FTD_LG_H_

#include "ftd_config.h"
#include "ftd_dev.h"
#include "ftd_journal.h"
#include "ftd_proc.h"
#include "ftd_sock_t.h"
#include "ftdio.h"
#include "comp.h"
#include "llist.h"
#if defined(_WINDOWS)
#include "ftd_perf.h"
#endif

#if !defined(_WINDOWS)

#define FTD_CREATE_GROUP_NAME(group_name, group_num) \
	sprintf(group_name, "/dev/" QNM "/lg%d/ctl", group_num);

#define FTD_CREATE_GROUP_DIR(dir_name, group_num) \
	sprintf(group_name, "/dev/" QNM "/lg%d", group_num);

#else

// DTurrin - Nov 7th, 2001
// If you are using Windows 2000 or later,
// you need to add the Global\ prefix so that
// client session will use kernel objects in
// the server's global name space.
// #define NTFOUR
#ifdef NTFOUR
#define FTD_CREATE_GROUP_NAME(group_name, group_num) \
                      sprintf(group_name, "\\\\.\\" QNM "\\lg%d", group_num);
#define FTD_CREATE_GROUP_BAB_EVENT_NAME(group_name, group_num) \
                      sprintf(group_name, QNM "lg%dbab", group_num);
#else // #ifdef NTFOUR
#define FTD_CREATE_GROUP_NAME(group_name, group_num) \
                      sprintf(group_name, "\\\\.\\Global\\" QNM "\\lg%d", group_num);
#define FTD_CREATE_GROUP_BAB_EVENT_NAME(group_name, group_num) \
                      sprintf(group_name, "Global\\" QNM "lg%dbab", group_num);
#endif // #ifdef NTFOUR
// #undef NTFOUR

#define FTD_CREATE_GROUP_DIR(dir_name, group_num) \
	sprintf(dir_name, QNM "/lg%d", group_num);

// partition structure
typedef struct part_s {
	char	devname[_MAX_PATH];
	char	pdevname[_MAX_PATH];
	char	symname[_MAX_PATH];
	int		dev_num;
} part_t;
	
#endif

/* the maximum number of key/value pairs of tunables */
#define FTD_MAX_KEY_VALUE_PAIRS 1024

#if defined(_WINDOWS)
/* this string is used to set the default tunables for a new logical group */
#define FTD_DEFAULT_TUNABLES "CHUNKSIZE: 256\nCHUNKDELAY: 0\nSYNCMODE: off\n\
SYNCMODEDEPTH: 1\nSYNCMODETIMEOUT: 30\nCOMPRESSION: off\nREFRESHTIMEOUT: 1\n\
_MODE: ACCUMULATE"
#else
/* this string is used to set the default tunables for a new logical group */
#define FTD_DEFAULT_TUNABLES "CHUNKSIZE: 256\nCHUNKDELAY: 0\nSYNCMODE: off\n\
SYNCMODEDEPTH: 1\nSYNCMODETIMEOUT: 30\nIODELAY: 0\nNETMAXKBPS: -1\nSTATINTERVAL: 10\n\
MAXSTATFILESIZE: 64\nLOGSTATS: on\n\
TRACETHROTTLE: off\nCOMPRESSION: off\nREFRESHTIMEOUT: 1\n\
_MODE: ACCUMULATE"
#endif

#define	TRACETHROTTLE		0
#define	MIN_CHUNKSIZE		32
#define	MAX_CHUNKSIZE		4000
#define	MIN_STATINTERVAL	0
#define	MAX_STATINTERVAL	86400
#define	MIN_MAXSTATFILESIZE	0
#define	MAX_MAXSTATFILESIZE	32000
#define	MIN_NETMAXKBPS		0
#define	MAX_NETMAXKBPS		INT_MAX

#define CHKSEGSIZE			32768
#define	DIGESTSIZE			16

#define	FTD_LG_NET_BROKEN		-100
#define	FTD_LG_NET_TIMEO		-101
#define	FTD_LG_APPLY_DONE		-102

// should never receive something this large
// so use it as a special return val for 'ftd_recv_lg_msg'
#define	FTD_LG_NET_NOT_READABLE	MAXINT

/* logical group tunable parameters */
typedef struct ftd_tune_s {
	int chunksize;				/* TUNABLE: default - 256 Kbyte          */
	int statinterval;			/* TUNABLE: default - 10 seconds         */
	int maxstatfilesize;		/* TUNABLE: default - 64 Kbyte           */
	int stattofileflag;			/* TUNABLE: default - 1                  */
	int tracethrottle;			/* TUNABLE: default - 0                  */
	int syncmode;				/* TUNABLE: default - 0 sync mode switch */
	int syncmodedepth;			/* TUNABLE: default - 1 deep queue       */
	int syncmodetimeout;		/* TUNABLE: default - 30 seconds         */
	int compression;			/* TUNABLE: default - none               */
	int tcpwindowsize;			/* TUNABLE: default - 256 Kbyte          */
	int netmaxkbps;				/* TUNABLE: default - -1                 */
	int chunkdelay;				/* TUNABLE: default - 0                  */
	int iodelay;				/* TUNABLE: default - 0                  */
	int refrintrvl;				/* TUNABLE: default - 60 seconds         */
} ftd_tune_t;

typedef struct ftd_lg_stat_s {
	pid_t pid;					/* pmd process id						*/
#if !defined(_WINDOWS)
	FILE *statfd;				/* FTD lg stat file descriptor		*/
#endif
	int pmdup;					/* 1 if pmd is running 0 if not          */
	int drvmode;				/* driver mode                          */
	int percentcpu;				/* pmd cpu usage percent */
	time_t statts;				/* timestamp of last stats dump          */
	float actualkbps;			/* kbps transfer rate                    */
	float effectkbps;			/* effective transfer rate w/compression */
	float pctdone;				/* % of refresh or backfresh complete    */
	int entries;				/* # of entries in bab                   */
	int sectors;				/* # of sectors in bab                   */
	int pctbab;					/* pct of bab in use                     */
	double local_kbps_read;		/* ftd group kbps read                  */
	double local_kbps_written;	/* ftd group kbps written               */
} ftd_lg_stat_t;

#define FTDLGMAGIC		0xBADF00D6

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
	char devname[MAXPATHLEN];	/* lg control device name		*/
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
	LList *throttles;			/* lg throttle list			*/
	LList *devlist;				/* lg device list			*/
} ftd_lg_t;

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

#define GET_LG_CHKSUM(x)		(LG_BYTE_2(x) & 0x01) 
#define SET_LG_CHKSUM(x)		(LG_BYTE_2(x) |= 0x01) 
#define UNSET_LG_CHKSUM(x)		(LG_BYTE_2(x) &= 0xfe) 

#define GET_LG_BACK_FORCE(x)	(LG_BYTE_2(x) & 0x02) 
#define SET_LG_BACK_FORCE(x)	(LG_BYTE_2(x) |= 0x02) 
#define UNSET_LG_BACK_FORCE(x)	(LG_BYTE_2(x) &= 0xfd) 

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

#define MSG_INCO				"INCOHERENT"
#define MSG_CO					"COHERENT"
#define MSG_CPON				"CHECKPOINT_ON"
#define MSG_CPOFF				"CHECKPOINT_OFF"

#define LG_PROCNAME(lgp)		(lgp)->fprocp->procp->procname 
#define LG_PID(lgp)				(lgp)->fprocp->procp->pid

/* external prototypes */
extern ftd_lg_t *ftd_lg_create(void);
extern int ftd_lg_delete(ftd_lg_t *lgp);
extern int ftd_lg_init(ftd_lg_t *lgp, int lgnum, int role, int startflag);
extern int ftd_lg_open(ftd_lg_t *lgp);
extern int ftd_lg_close(ftd_lg_t *lgp);
extern int ftd_lg_add(ftd_lg_t *lgp, int autostart);
extern int ftd_lg_rem(ftd_lg_t *lgp, int autostart, int silent);
extern int ftd_lg_start(ftd_lg_t *lgp);
extern int ftd_lg_stop(ftd_lg_t *lgp);
extern int ftd_lg_set_driver_state(ftd_lg_t *lgp);
extern int ftd_lg_get_driver_state(ftd_lg_t *lgp, int silent);
extern int ftd_lg_set_pstore_state(ftd_lg_t *lgp, char *buf, int buflen);
extern int ftd_lg_get_pstore_state(ftd_lg_t *lgp, int silent);
extern int ftd_lg_set_pstore_state_value(ftd_lg_t *lgp, char *key, char *value);
extern int ftd_lg_get_pstore_state_value(ftd_lg_t *lgp, char *key, char *value);
extern int ftd_lg_set_state_value(ftd_lg_t *lgp, char *key, char *value);
extern int ftd_lg_get_state_value(ftd_lg_t *lgp, char *key, char *value);
extern ftd_dev_t *ftd_lg_devid_to_dev(ftd_lg_t *lgp, int devid);
extern ftd_dev_cfg_t *ftd_lg_devid_to_devcfg(ftd_lg_t *lgp, int devid);
extern ftd_dev_t *ftd_lg_minor_to_dev(ftd_lg_t *lgp, int num);
extern ftd_dev_t *ftd_lg_ftd_to_dev(ftd_lg_t *lgp, int ftdnum);
extern int ftd_lg_flush_net(ftd_lg_t *lgp); 
extern int ftd_lg_abort(ftd_lg_t *lgp);
extern int ftd_lg_noop(ftd_lg_t *lgp);
extern int ftd_lg_housekeeping(ftd_lg_t *lgp, int tune);
extern int ftd_lg_dispatch_io(ftd_lg_t *lgp);
extern int ftd_lg_eval_net_usage(ftd_lg_t *lgp); 
extern int ftd_lg_open_devs(ftd_lg_t *lgp, int mode, int permis, int tries);
extern int ftd_lg_close_devs(ftd_lg_t *lgp);
extern int ftd_lg_open_ftd_devs(ftd_lg_t *lgp, int mode, int permis, int tries);
extern int ftd_lg_close_ftd_devs(ftd_lg_t *lgp);
extern int ftd_lg_traverse_chunk(ftd_lg_t *lgp);
extern int ftd_lg_get_driver_run_state(int lgnum);
extern int ftd_lg_get_pstore_run_state(int lgnum, ftd_lg_cfg_t *cfgp);
extern int ftd_lg_dump_pstore_attr(ftd_lg_t *lgp, FILE *fd);
extern int ftd_lg_set_pstore_run_state(int lgnum, ftd_lg_cfg_t *cfgp, int state);
extern int ftd_lg_set_driver_run_state(int lgnum, int state);
extern int ftd_lg_driver_state_to_lg_state(int dstate);
extern int ftd_lg_state_to_driver_state(int state);
extern int ftd_lg_open(ftd_lg_t *lgp);
extern int ftd_lg_start_journaling(ftd_lg_t *lgp, int jrnstate);
extern int ftd_lg_stop_journaling(ftd_lg_t *lgp, int jrnstate, int jrnmode);
extern int ftd_lg_kill(ftd_lg_t *lgp);
extern int ftd_lg_get_dev_stats(ftd_lg_t *lgp, int deltasec);
extern int ftd_lg_report_invalid_op(ftd_lg_t *lgp, short inchar);
extern int ftd_lg_bab_has_entries(ftd_lg_t *lgp, int allowed);
extern int ftd_lg_apply(ftd_lg_t *lgp);
extern int ftd_lg_init_devs(ftd_lg_t *lgp);
extern int ftd_lg_proc_bab(ftd_lg_t *lgp, int force);
extern int ftd_lg_proc_signal(ftd_lg_t *lgp);

#if defined(_WINDOWS)
extern int ftd_lg_lock_devs(ftd_lg_t *lgp, int role);
extern int ftd_lg_unlock_devs(ftd_lg_t *lgp);
extern int ftd_lg_check_signals(ftd_lg_t *lgp, int timeo);
extern int ftd_lg_sync_prim_devs(ftd_lg_t *lgp);
#endif

#endif
