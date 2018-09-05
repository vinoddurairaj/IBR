/*****************************************************************************
 *                                                                           *
 *  This software  is the licensed software of Fujitsu Software              *
 *  Technology Corporation                                                   *
 *                                                                           *
 *  Copyright (c) 2004 by Softek Storage Technology Corporation              *
 *                                                                           *
 *  THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF            *
 *  FUJITSU SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED             *
 *  UNDER LICENSE FROM FUJITSU SOFTWARE TECHNOLOGY CORPORATION               *
 *                                                                           *
 *****************************************************************************

 *****************************************************************************
 *                                                                           *
 *  Revision History:                                                        *
 *                                                                           *
 *  Date        By              Change                                       *
 *  ----------- --------------  -------------------------------------------  *
 *  04-27-2004  Veerababu Arja   Initial version.                             *
 *                                                                           *
 *                                                                           *
 *****************************************************************************/

#ifndef _SFTK_CONFIG_H
#define _SFTK_CONFIG_H

/* DEV_BSHIFT is set for 512 bytes */
#define DEV_BSHIFT  9
#define DEV_BSIZE   512

#include <malloc.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <process.h>
#include <signal.h>
#include <winsock2.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <winioctl.h>
#include <limits.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN


#include "llist.h"

#ifndef MAXPATH
#define MAXPATH MAX_PATH
#endif /* MAXPATH */

#ifndef MAXPATHLEN
#define MAXPATHLEN MAXPATH
#endif /* MAXPATHLEN */


typedef struct ftd_dev_cfg_s {
	int magicvalue;					/* so we know it's been initialized */
	int devid;						/* device number					*/
	char remark[256];				/* device remark from config file	*/
	char devname[MAXPATHLEN];		/* FTD device name					*/
	char pdevname[MAXPATHLEN];		/* primary raw device name			*/
	char sdevname[MAXPATHLEN];		/* secondary (mirror) device name	*/
	char pdriverid[24];		        /* primary volume drive id          */
	char ppartstartoffset[24];	    /* primary volume partition starting offset */
	char ppartlength[24];	        /* primary volume partition length  */
	char sdriverid[24];		        /* secondary volume drive id        */
	char spartstartoffset[24];	    /* secondary volume partition starting offset */
	char spartlength[24];	        /* secondary volume partition length*/
	// SAUMYA_FIX_CONFIG_FILE_PARSING
//#if 0
	char symlink1[128];				/* symbolic link in \Device\Harddisk(%x)\Partition(%x) format*/
	char symlink2[128];				/* symbolic link in \Device\HarddiskVolume(%x) format */
	char symlink3[128];				/* symbolic link in volume((disk_signature)-offset-partition_length) format */
//#endif  // SAUMYA_FIX_CONFIG_FILE_PARSING

#if defined(_WINDOWS)
	char vdevname[MAXPATHLEN];		/* volume device name	*/
#endif
} ftd_dev_cfg_t;
		
typedef struct ftd_lg_cfg_s {
	int magicvalue;					/* so we know it's been initialized */
	int lgnum;						/* logical group number				*/
	int role;						/* lg role: 1=primary, 2=secondary */
	FILE *cfgfd;					/* ftd config file descriptor		*/
	char cfgpath[MAXPATHLEN];		/* logical group config file path	*/
	char pstore[MAXPATHLEN];		/* pstore path from config file	*/
	char phostname[MAXPATHLEN];		/* primary hostname					*/
	char shostname[MAXPATHLEN];		/* secondary hostname				*/
	char jrnpath[MAXPATHLEN];		/* lg journal area path	*/
	char notes[MAXPATHLEN];			/* lg notes from config file	*/
	int port;						/* lg peer communications port num */
	int chaining;					/* lg mirror chain state (A->B->C) */
	LList *throttles;				/* list of throttles for group	*/
	LList *devlist;					/* list of device config state for lg */
} ftd_lg_cfg_t;

/* -- structure used for reading and parsing a line from the config file */
typedef struct _line_state {
    int  invalid;					/* bogus boolean. */
    int  lineno;
    char line[256];					/* storage for parsed line */
    char readline[256];				/* unparsed line */
    /* next four elements used for parsing throttles */
    char word[256];
    int plinepos;
    int linepos;
    int linelen;
    /* next two elements for parsing a value or a string */
    long value;
    int valueflag;
    /* ptrs to parsed parameters in line[] */
    char *key;
    char *p1;
    char *p2;
    char *p3;
    char *p4;
// SAUMYA_FIX_CONFIG_FILE_PARSING
//#if 0
	char *p5;
	char *p6;
	char *p7;
//#endif // SAUMYA_FIX_CONFIG_FILE_PARSING

} LineState;

#define ROLEPRIMARY		1
#define ROLESECONDARY	2

#define FTDCFGMAGIC		0xBADF00D5

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

#define FTD_CFG_LHOST(cfgp)		(cfgp)->phostname
#define FTD_CFG_RHOST(cfgp)		(cfgp)->shostname


#define SFTK_SOFTWARE_KEY "Software\\Softek\\Dtc\\CurrentVersion"

extern char* ftd_nt_path(char* key);

/*
 * Where we put the config files
 */
#define PATH_CONFIG			ftd_nt_path("InstallPath")

#define FTD_PS_LRDB_SIZE        8*1024
#define FTD_PS_HRDB_SIZE        128*1024
#define FTD_PS_GROUP_ATTR_SIZE  4*1024
#define FTD_PS_DEVICE_ATTR_SIZE 4*1024

/* offset from beginning of slice to the pstore header in 1024 byte blocks */
#define PS_HEADER_OFFSET      16  /* 16K */

#define FTD_MAX_KEY_VALUE_PAIRS	1024

#ifdef NTFOUR
#define FTD_DEV_DIR "\\\\.\\DTC"
#else // #ifdef NTFOUR
#define FTD_DEV_DIR "\\\\.\\Global\\DTC"
#endif // #ifdef NTFOUR
// #undef NTFOUR

#define FTD_CTLDEV  FTD_DEV_DIR "\\ctl"

/*


 /*
 * Config error codes
 */
#define FTD_CFG_NOT_FOUND		-10
#define FTD_CFG_DIR_NOT_FOUND	-20
#define FTD_CFG_BAD_KEY			-30
#define FTD_CFG_MIR_STAT		-40
#define FTD_CFG_MIR_TYPE		-50


/* == TOKENS == */
#define TOKEN_UNKNOWN    0
#define TOKEN_MEASURE    1
#define TOKEN_RELOP      2
#define TOKEN_VALUE      3
#define TOKEN_VERB       4
#define TOKEN_ACTION     5
#define TOKEN_STRING     6

/* == MEASUREMENT TOKENS == */
#define TOK_UNKNOWN         0
#define TOK_NETKBPS         1
#define TOK_PCTCPU          2
#define TOK_PCTWL           3
#define TOK_NETCONNECTFLAG  4
#define TOK_PID             5
/* -- tunable parameters -- */ 
#define TOK_CHUNKSIZE       11
#define TOK_STATINTERVAL    12
#define TOK_MAXSTATFILESIZE 13
#define TOK_STATTOFILEFLAG  14
#define TOK_TRACETHROTTLE   15
#define TOK_SYNCMODE        16
#define TOK_SYNCMODEDEPTH   17
#define TOK_SYNCMODETIMEOUT 18
#define TOK_COMPRESSION     19
#define TOK_TCPWINDOW       20
#define TOK_NETMAXKBPS      21
#define TOK_CHUNKDELAY      22
#define TOK_IODELAY         23
/* -- performance metrics -- */
#define TOK_DRVMODE         30
#define TOK_ACTUALKBPS      31
#define TOK_EFFECTKBPS      32
#define TOK_PCTDONE         33
#define TOK_ENTRIES         34
#define TOK_PCTBAB          35
#define TOK_READKBPS        36
#define TOK_WRITEKBPS       37

#define NUM_MEAS_TOKENS     38

/* -- RELATIONAL OPERATOR TOKENS -- */
#define GREATERTHAN      1
#define GREATEREQUAL     2
#define LESSTHAN         3
#define LESSEQUAL        4
#define EQUALTO          5
#define NOTEQUAL         6
#define TRAN2GT          7
#define TRAN2GE          8
#define TRAN2LT          9
#define TRAN2LE         10
#define TRAN2EQ         11
#define TRAN2NE         12

/* -- LOGICAL OPERATOR TOKENS -- */
#define LOGOP_DONE       0
#define LOGOP_AND        1
#define LOGOP_OR         2

/* -- ACTION VERB TOKENS -- */
#define VERB_SET         1
#define VERB_INCR        2
#define VERB_DECR        3
#define VERB_DO          4

/* -- ACTION ITEM TOKENS -- */
#define ACTION_SLEEP     101
#define ACTION_IODELAY   102
#define ACTION_ADDWL     103
#define ACTION_REMWL     104
#define ACTION_CONSOLE   105
#define ACTION_MAIL      106
#define ACTION_EXEC      107
#define ACTION_LOG       108

typedef struct action_s {
    int    actionverb_tok;       
    int    actionwhat_tok;
    int    actionvalue;
    char   actionstring[256];
} action_t;

typedef struct throt_test_s {
    int measure_tok;
    int relop_tok;
    int value;
    int valueflag;
    char valuestring[256];
    int logop_tok;
} throt_test_t;

typedef struct throttle_s {
    struct throttle_s* n;
    int    day_of_week_mask;
    int    day_of_month_mask;
    int    end_of_month_flag;
    time_t from;
    time_t to;
    int    num_throttest;
    throt_test_t throttest[16];
    int    num_actions;
    action_t actions[16];
} throttle_t;

typedef struct meas_val_s {
    int val_type;
    int prev_value;
    int value;
    char prev_value_string[256];
    char value_string[256];
} meas_val_t;


/* external prototypes */
extern LList *ftd_config_create_list(void);
extern int ftd_config_delete_list(LList *cfglist);

extern ftd_dev_cfg_t *ftd_config_dev_create(void); 
extern int ftd_config_dev_delete(ftd_dev_cfg_t *devcfgp); 
extern int ftd_config_dev_add_to_list(LList *cfglist, ftd_dev_cfg_t **devcfgpp); 
extern int ftd_config_dev_remove_from_list(LList *cfglist, ftd_dev_cfg_t **devcfgpp); 

extern ftd_lg_cfg_t *ftd_config_lg_create(void); 
extern int ftd_config_lg_delete(ftd_lg_cfg_t *cfgp); 
extern int ftd_config_lg_add_to_list(LList *cfglist, ftd_lg_cfg_t **cfgpp); 
extern int ftd_config_lg_remove_from_list(LList *cfglist, ftd_lg_cfg_t **cfgpp); 

extern int ftd_config_get_all(char *cfgpath, LList *cfglist);
extern int ftd_config_get_primary(char *cfgpath, LList *cfglist);
extern int ftd_config_get_primary_started(char *cfgpath, LList *cfglist); 
extern int ftd_config_get_secondary(char *cfgpath, LList *cfglist);
extern int ftd_config_read(ftd_lg_cfg_t *cfgp, BOOL bIsWindows, int lgnum, int role, int startflag);
extern int ftd_config_read_2(ftd_lg_cfg_t *cfgp, BOOL bIsWindows, int lgnum, int role);
extern int ftd_config_get_psname(ftd_lg_cfg_t *cfgp);
	
#endif /* _SFTK_CONFIG_H */
