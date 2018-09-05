/*
 * ftd_journal.h - FTD journal interface
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

#ifndef _FTD_JOURNAL_H_
#define _FTD_JOURNAL_H_

#include "ftd_port.h"
#include "ftd_devio.h"
#include "llist.h"

#define	JRNCPONAGAIN			100
#define	JRNCPPENDING			200
#define JRN_MAXFILES			1000 
#define JRN_CHUNKSIZE			2*1024*1024

#define FTDJRNMAGIC				0xBADF00D2	

#define CP_SUFFIX      		    'p'
#define CO_SUFFIX      		    'c'
#define INCO_SUFFIX    		    'i'

/* journal states */
#define FTDJRNCO				0x0001
#define FTDJRNINCO				0x0002

/* journal modes */
#define FTDMIRONLY				0x0004
#define FTDJRNONLY				0x0008
#define FTDJRNMIR				(FTDMIRONLY | FTDJRNONLY) 

/* journal file lock states */
#define JRN_LOCK				0x0010
#define JRN_UNLOCK				0x0000

/* journal flags */
#define JRN_CPON				0x0100
#define JRN_CPPEND				0x0200

#define JRN_JLESS      0x0400

/* journal macros */
#define GET_JRN_STATE(x)		((x) & 0x0003) 
#define SET_JRN_STATE(x,y)		((x) = ((x) & 0xfffc) | (y)) 

#define GET_JRN_MODE(x)         ((x) & 0x000c) 
#define SET_JRN_MODE(x,y)		((x) = ((x) & 0xfff3) | (y)) 

#define GET_JRN_LOCK(x)			((x) &  JRN_LOCK) 
#define SET_JRN_LOCK(x)			((x) |= JRN_LOCK)
#define UNSET_JRN_LOCK(x)		((x) &= 0xffef)

#define GET_JRN_CP_ON(x)		((x) &  JRN_CPON) 
#define SET_JRN_CP_ON(x)		((x) |= JRN_CPON)
#define UNSET_JRN_CP_ON(x)		((x) &= 0xfeff)

#define GET_JRN_CP_PEND(x)		((x) &  JRN_CPPEND) 
#define SET_JRN_CP_PEND(x)		((x) |= JRN_CPPEND)
#define UNSET_JRN_CP_PEND(x)	((x) &= 0xfdff) 

#define GET_JRN_JLESS(x)  	((x) &  JRN_JLESS) 		// it's duplicating LG flags, but it will make ftd_journal.c readable...with much less change
#define SET_JRN_JLESS(x)	((x) |= JRN_JLESS) 
#define UNSET_JRN_JLESS(x)	((x) &= 0xfbff) 


typedef struct ftd_jrnheader_s {
    unsigned long magicnum;
    int state;
    int mode;
} ftd_jrnheader_t;

typedef struct ftd_journal_file_s {
	char name[MAXPATHLEN];			/* journal file name             */
	HANDLE fd;						/* journal file handle               */
	unsigned int flags;				/* journal file state bits            */
	ftd_int64_t offset;				/* journal file offset           */
	ftd_int64_t size;				/* journal file size (bytes)     */
	ftd_int64_t locked_offset;		/* journal file locked offset    */
	ftd_int64_t locked_length;		/* journal file locked length (bytes)   */
	int chunksize;					/* journal I/O chunk size            */
} ftd_journal_file_t;
	
typedef struct ftd_journal_s {
	int magicvalue;					/* so we know it's been initialized	*/
	HANDLE fd;						/* journal directory file handle   */
	unsigned int flags;				/* journal state bits              */
	int cpnum;						/* checkpoint journal file number  */
	char *jrnpath;					/* journal area path                  */
	char prefix[5];					/* prefix to name file objects */
	int buflen;						/* I/O buffer length */
	char *buf;						/* I/O buffer address */
	ftd_journal_file_t *cur;		/* active journal file address */
	LList *jrnlist;					/* list of files in the journal */
} ftd_journal_t;
	
/* function prototypes */
extern ftd_journal_t *ftd_journal_create(char *jrnpath, char *fileprefix);
extern int ftd_journal_delete(ftd_journal_t *jrnp);
extern ftd_journal_file_t *ftd_journal_file_create(ftd_journal_t *jrnp);
extern int ftd_journal_file_delete(ftd_journal_t *jrnp,
	ftd_journal_file_t **jrnfpp);
extern int ftd_journal_get_cur_state(ftd_journal_t *jrnp, int prune_inco);
extern int ftd_journal_get_all_files(ftd_journal_t *jrnp);
extern int ftd_journal_co(ftd_journal_t *jrnp);
extern int ftd_journal_delete_all_files(ftd_journal_t *jrnp);
extern int ftd_journal_close_all_files(ftd_journal_t *jrnp);
extern int ftd_journal_delete_inco_files(ftd_journal_t *jrnp);
extern int ftd_journal_set_state(ftd_journal_t *jrnp, int tstate);
extern ftd_uint64_t ftd_journal_file_seek(ftd_journal_file_t *jrnfp,
	ftd_uint64_t offset, int whence);
extern int ftd_journal_file_read(ftd_journal_file_t *jrnfp,
	char *buf, int *buflen);
extern int ftd_journal_file_write(ftd_journal_t *jrnp, int lgnum, int devid,
	int offset, int len, char *buf);
extern int ftd_journal_delete_cp(ftd_journal_t *jrnp);
extern ftd_journal_file_t *ftd_journal_create_next(ftd_journal_t *jrnp,
	int tstate);
extern int ftd_journal_create_next_cp(ftd_journal_t *jrnp,
	ftd_journal_file_t *jrnfp);
extern int ftd_journal_file_open(ftd_journal_t *jrnp,
	ftd_journal_file_t *jrnfp, int mode, int permis);
extern int ftd_journal_file_close(ftd_journal_file_t *jrnfp);
extern int ftd_journal_file_lock(ftd_journal_file_t *jrnfp,
	ftd_uint64_t offset, ftd_uint64_t lock_len); 
extern int ftd_journal_file_lock_header(ftd_journal_file_t *jrnfp); 
extern int ftd_journal_file_unlock(ftd_journal_file_t *jrnfp,
	ftd_uint64_t offset, ftd_int64_t lock_len); 
extern int ftd_journal_file_unlock_header(ftd_journal_file_t *jrnfp); 
extern int ftd_journal_file_is_cp(ftd_journal_file_t *jrnfp);
extern int ftd_journal_file_is_co(ftd_journal_file_t *jrnfp);
extern int ftd_journal_file_is_inco(ftd_journal_file_t *jrnfp);
extern int ftd_journal_parse_name(int flags, char *name, int *num,
	int *state, int *mode);
extern ftd_uint64_t ftd_journal_get_journal_files_total_size(ftd_journal_t *jrnp);

#endif
