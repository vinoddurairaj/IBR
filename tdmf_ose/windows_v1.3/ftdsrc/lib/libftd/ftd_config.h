/*
 * ftd_config.h - FTD config file interface
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
#ifndef _FTD_CONFIG_H
#define _FTD_CONFIG_H

#include "ftd_port.h"
#include "llist.h"

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
#if 0
	char symlink1[128];				/* symbolic link in \Device\Harddisk(%x)\Partition(%x) format*/
	char symlink2[128];				/* symbolic link in \Device\HarddiskVolume(%x) format */
	char symlink3[128];				/* symbolic link in volume((disk_signature)-offset-partition_length) format */
#endif  // SAUMYA_FIX_CONFIG_FILE_PARSING

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
#if 0
	char *p5;
	char *p6;
	char *p7;
#endif // SAUMYA_FIX_CONFIG_FILE_PARSING

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

 /*
 * Config error codes
 */
#define FTD_CFG_NOT_FOUND		-10
#define FTD_CFG_DIR_NOT_FOUND	-20
#define FTD_CFG_BAD_KEY			-30
#define FTD_CFG_MIR_STAT		-40
#define FTD_CFG_MIR_TYPE		-50

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
	
#endif /* _FTD_CONFIG_H */
