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
 * ftdio.c - FullTime Data
 *
 * (c) Copyright 1998 FullTime Software, Inc.
 *     All Rights Reserved
 *
 * This module implements the functionality for FTD local/mirror I/O
 *
 ***************************************************************************/

#include <sys/param.h>

#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include "aixcmn.h"

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef SOLARIS
#include <values.h>
#endif
#include <signal.h>

#include "devcheck.h"
#include "process.h"
#include "ftdio.h"
#include "errors.h"
#include "config.h"
#include "network.h"
#include "license.h"
#include "licprod.h"
#include "pathnames.h"
#include "common.h"
#include "cfg_intr.h"
#include "stat_intr.h"

#ifdef NEED_SYS_MNTTAB
#include "ftd_mount.h"
#else
#include <sys/mnttab.h>
#endif
#ifdef TDMF_TRACE
FILE *dbgfd;
#endif

#define LINK_SUBDIR "hlink"

#if defined(linux)
static const int mirror_flags = O_SYNC | O_LARGEFILE;
#else
static const int mirror_flags = O_LARGEFILE;
#endif

int save_journals = 0;

char *argv0;

ftd_jrnpaths_t  jrnpath_handle = {0, 0, NULL};
ftd_jrnpaths_t *jrnphp         = &jrnpath_handle;

extern int ftd_lg_merge_hrdbs(group_t* group, int lgnum);

int
jrnpath_getcnt( ftd_jrnpaths_t *jrnphp )
{
    return jrnphp->jp_paths_used; 
}
char *
jrnpath_get( ftd_jrnpaths_t *jrnphp, int idx )
{
    /*TODO add range checks */
    return jrnphp->jp_paths[idx]; 
}

char *
jrnpath_getlast( ftd_jrnpaths_t *jrnphp )
{
    return jrnphp->jp_paths[jrnphp->jp_paths_used -1]; 
}

void
jrnpath_reset( ftd_jrnpaths_t *jrnphp )
{
    jrnphp->jp_paths_used = 0;
}

#define FTD_JRNPATH_INC 1000

void
jrnpath_alloc( ftd_jrnpaths_t *jrnphp )
{
    char *dp;
    jrnphp->jp_paths = (char (*)[24])ftdrealloc( (char *)jrnphp->jp_paths,
				     (jrnphp->jp_paths_alloc + FTD_JRNPATH_INC)
				      * FTD_JRNNAME_MAXLEN );
    jrnphp->jp_paths_alloc += FTD_JRNPATH_INC;
}

int
jrnpath_insert( ftd_jrnpaths_t *jrnphp, char *jrnnamep  )
{
    if (jrnphp->jp_paths_used == jrnphp->jp_paths_alloc)
    {
	jrnpath_alloc( jrnphp );
    }
    strcpy( jrnphp->jp_paths[jrnphp->jp_paths_used], jrnnamep );
    return ++(jrnphp->jp_paths_used);
}

int
jrnpath_sort( ftd_jrnpaths_t *jrnphp )
{
    qsort( (void *)jrnphp->jp_paths
	  ,jrnphp->jp_paths_used
	  ,FTD_JRNNAME_MAXLEN
	  ,stringcompare );
    return jrnphp->jp_paths_used;
}

static char fmt[384], msg[384];

int _rmd_state;
int _rmd_apply;
ftd_jrnnum_t _rmd_jrn_cp_lnum;
int _rmd_jrn_state;
int _rmd_jrn_mode;
ftd_jrnnum_t _rmd_jrn_num;
int _rmd_cpon;
int _rmd_cpstart;
int _rmd_cppend;
int _rmd_cpstop;
u_longlong_t _rmd_jrn_size;
static u_longlong_t jrn_offset;
int _rmd_jrn_chunksize;
int _rmd_rfddone;
int _pmd_refresh = 0; /* this variable is set only when driver moves to
                       * refresh from tracking. We use this variable
                       * in setmode() to prevent the restarting of full
                       * refresh if driver went into tracking during
                       * full refresh phase 2
                       */
struct iovec *iov;
int iovlen;
int max_idle_time = 10;

float compr_ratio;

static struct timeval skosh;

extern const int exitfail;
extern char *_pmd_configpath;
extern int _pmd_state;
extern int _pmd_state_change;
extern int _pmd_cpon;
extern int _pmd_cpstart;
extern int _pmd_cpstop;
extern int _pmd_rpipefd;
extern int _pmd_wpipefd;
extern int refresh_started;
extern int refresh_oflow;
extern int g_ackhup;

static char *aligned_buf = NULL;
static size_t aligned_buf_len = 0;
#if defined(linux)
static unsigned long addr_mask = ~0l;
#else
static unsigned long addr_mask = 0l;
#endif
static int write_jrn_to_mir (headpack_t *header, char *datap);
static void link_journal(const char *);


/* force flushing if coalesce buffer reaches 64K */
#define WC_FORCE_FLUSH_BUFLEN (0x010000 >> DEV_BSHIFT)


/* wciov_t array should be a power of 2 */
typedef struct wciov_s
{
    wlheader_t * wciov_entry;
} wciov_t;

/* wc_length == 0 means no coalesced buffers */

typedef struct wc_s
{
    int      		wc_length;	/* length for coalesced buffer */
    EntryFunc *		wc_func;	/* write function */
    wciov_t *		wc_iovp;	/* buffer vector to be coalesced */
    int			wc_iovmax;	/* max number of buffer vectors */
    int			wc_iovbidx;	/* start index in buffer vector */
    int			wc_ioveidx;	/* end index in buffer vector */
    char *   		wc_bufp;	/* coalesced buffer */
    int   		wc_buflen;	/* coalesced buffer length*/
} wc_t;

void
init_wc( wc_t * wcp, wciov_t *wciovp, int wciovlen )
{
    wcp->wc_length = 0;
    wcp->wc_bufp   = 0;
    wcp->wc_buflen = 0;
    wcp->wc_iovp   = wciovp;
    wcp->wc_iovmax = wciovlen;
    wcp->wc_iovbidx = -1;
    wcp->wc_ioveidx = -1;
}

void
free_wc( wc_t *wcp )
{
    if (wcp->wc_bufp)
    {
	ftdfree( wcp->wc_bufp );
    }
}

int
is_buffer_adjacent_on_top( wlheader_t *keyp, wlheader_t *nodep )
{
    u_longlong_t key_offset, node_offset;

    key_offset = keyp->offset;
    node_offset = nodep->offset + nodep->length;
    return key_offset == node_offset;
}

int
is_buffer_adjacent_on_bottom( wlheader_t *keyp, wlheader_t *nodep )
{
    u_longlong_t key_offset, node_offset;

    key_offset = keyp->offset + keyp->length;
    node_offset = nodep->offset;
    return key_offset == node_offset;
}

int
flush_coalesce( wc_t *wcp )
{
    wlheader_t * wlhead;
    char *       bp;
    char *       dataptr;
    int		 i, length, ret;
    int		 tmplen;

    if (wcp->wc_length <= 0 || wcp->wc_iovbidx < 0 || wcp->wc_ioveidx < 0 || wcp->wc_ioveidx >= wcp->wc_iovmax || wcp->wc_ioveidx < wcp->wc_iovbidx)
    {
	if (wcp->wc_length > 0)
	{
	    fprintf( stderr, "iov indexes are bad begin %d end %d max %d\n",
			wcp->wc_iovbidx, wcp->wc_ioveidx, wcp->wc_iovmax);
	}
	return 0;
    }

    /* single buffer, so no reason to copy */

    if (wcp->wc_iovbidx == wcp->wc_ioveidx)
    {
	wlhead = wcp->wc_iovp[wcp->wc_iovbidx].wciov_entry;
	dataptr = (char *)(wlhead + 1);
    }
    else 
    {
	if (wcp->wc_length > wcp->wc_buflen)
	{
	    if (wcp->wc_bufp)
	    {
		ftdfree( wcp->wc_bufp );
	    }
	    wcp->wc_bufp = ftdmalloc( wcp->wc_length << DEV_BSHIFT);
	    wcp->wc_buflen = wcp->wc_length;
	}
	bp = wcp->wc_bufp;
	tmplen = 0;
	for (i = wcp->wc_iovbidx; i <= wcp->wc_ioveidx; i++)
	{
	    wlhead = wcp->wc_iovp[i].wciov_entry;
	    dataptr = (char *)(wlhead + 1);
	    tmplen += wlhead->length;
	    length = wlhead->length << DEV_BSHIFT;
	    memcpy( bp, dataptr, length); 
	    bp += length;
	}
	wlhead = wcp->wc_iovp[wcp->wc_iovbidx].wciov_entry;
	wlhead->length = wcp->wc_length;
	dataptr = wcp->wc_bufp;
	if (wcp->wc_length != tmplen)
	{
	    fprintf( stderr, "merged len wc %d temp %d\n",
			wcp->wc_length, tmplen );
	}
    }
    ret = (*wcp->wc_func)( wlhead, dataptr );
    wcp->wc_length = 0;
    wcp->wc_iovbidx = wcp->wc_ioveidx = -1;
    return ret;
}

int
write_coalesce( wc_t *wcp, wlheader_t *nodep )
{
    wlheader_t *first, *last;
    int ret = 0;
    if (wcp->wc_iovbidx >= 0)
    {
	first = wcp->wc_iovp[wcp->wc_iovbidx].wciov_entry;
	last  = wcp->wc_iovp[wcp->wc_ioveidx].wciov_entry;

	/* 
	 * if different device write coalesced data
	 * else try coalescing on top
	 * else try coalescing on bottom
	 * else flush existing coalesced data before collecting again
	 */

	if (first->diskdev != nodep->diskdev)
	{
	    ret = flush_coalesce( wcp );
	}
	else if (is_buffer_adjacent_on_top( first, nodep ))
	{
	    if (wcp->wc_iovbidx > 0)
	    {
		wcp->wc_iovbidx--;
		wcp->wc_iovp[wcp->wc_iovbidx].wciov_entry = nodep;
		wcp->wc_length += nodep->length;
		return 0;
	    }
	    else
	    {
		/* used up available iov entries so flush and start over */
		ret = flush_coalesce( wcp );
	    }
	}
	else if (is_buffer_adjacent_on_bottom( last, nodep ))
	{
	    if (wcp->wc_ioveidx < (wcp->wc_iovmax - 1))
	    {
		wcp->wc_ioveidx++;
		wcp->wc_iovp[wcp->wc_ioveidx].wciov_entry = nodep;
		wcp->wc_length += nodep->length;
		return 0;
	    }
	    else
	    {
		/* used up available iov entries so flush and start over */
		ret = flush_coalesce( wcp );
	    }
	}
	else
	{
	    /* cannot coalesce, so flush and start over */
	    ret = flush_coalesce( wcp );
	}
    }

    /* come here if only the first insert into wciov */

    wcp->wc_ioveidx = wcp->wc_iovbidx = wcp->wc_iovmax/2 - 1;
    wcp->wc_iovp[wcp->wc_iovbidx].wciov_entry = nodep;
    wcp->wc_length = nodep->length;

    /* if buffer size is large enough, do the I/O now */

    if (nodep->length >= WC_FORCE_FLUSH_BUFLEN)
    {
	ret = flush_coalesce( wcp );
    }
    return ret;
}

char *
page_aligned(char *p, size_t length)
{
    if (!(addr_mask & (unsigned long)p))
        return p;
    if (length > aligned_buf_len) {
        ftdfree(aligned_buf);
        aligned_buf = (char *)ftdvalloc(length);
        aligned_buf_len = length;
        addr_mask = 1;
        while (!(addr_mask & (unsigned long)aligned_buf))
            addr_mask <<= 1;
        addr_mask -= 1;
    }
    memcpy(aligned_buf, p, length);
    return aligned_buf;
}

/*
 * ftdread -- wrap read with a retry loop
 */
int
ftdread(int fd, char *buf, int len)
{
    int sofar, tries, maxtries, rlen;

    rlen = len;
    maxtries = 5;
    tries = 0;

    while (len > 0) {
        if (tries++ == maxtries) {
            break;
        }
        sofar = read(fd, buf, len);
        if (sofar == -1) {
            if (errno == EINTR
            || errno == EAGAIN) {
                usleep(10000);
                continue;
            } else {
                return -1;
            }
        }
        len -= sofar;
        buf += sofar;
    }

    return (rlen-len); /* actual bytes read */
} /* ftdread */

/*
 * ftdwrite -- wrap write with a retry loop
 */
int
ftdwrite(int fd, char *buf, int len)
{
    int sofar, tries, maxtries, wlen;

    wlen = len;
    maxtries = 5;
    tries = 0;

    while (len > 0) {
        if (tries++ == maxtries) {
            break;
        }
        sofar = write(fd, buf, len);
        if (sofar == -1) {
            if (errno == EINTR
            || errno == EAGAIN) {
                usleep(10000);
                continue;
            } else {
                return -1;
            }
        }
        len -= sofar;
        buf += sofar;
    }

    return (wlen-len); /* actual bytes written */
} /* ftdwrite */

/****************************************************************************
 * cp_file -- check for existence of given checkpoint file
 ***************************************************************************/
int
cp_file(char *jrnpath, char suffix_char)
{
    char *dot = strrchr(jrnpath, '.');
    return (dot && !dot[2] && dot[1] == suffix_char);
} /* cp_file */

/****************************************************************************
 * parse_journal_name  -- parse the components from journal name
 *
 * <jrnname>   := <jrnprefix><lgnum><dot><jrnnum><dot><jrnsuffix>
 * <jrnprefix> := "j"
 * <dot>       := "."
 * <lgnum>     := logical group number n digits
 * <jrnnum>    := journal number
 * <jrnsuffix> := "i" | "c" | "p"
 * j000.99991231235959.x
 * Note: on HP-UX 11.11, __strtoull() parsing errors fixed by using strtoul()
 *       which still allows for 4 billion journals; 
 *       so the format would be j000.4294967295.x (maximum value) in this case
 * 
 ***************************************************************************/
int
parse_journal_name(char *name, ftd_jrnnum_t *jrn_num, int *jrn_state, int *jrn_mode)
{
    char state_c;
    int byte;
    ftd_jrnnum_t num;
    char *bjrnp, *ejrnp;
    int len = strlen(name);

    state_c = name[len-1];


    bjrnp = strchr( name, '.' );
    ejrnp = &name[len-2];

    if (len < FTD_JRNNAME_MINLEN || !bjrnp || !ejrnp || bjrnp >= ejrnp
     || *ejrnp != '.' || !strchr("icp",state_c ) || len >= FTD_JRNNAME_MAXLEN)
    {
	ftd_debugf("%s", " parse_journal_name failure\n", name);
	goto err_out;
    }
    ejrnp = 0;
#if defined(HPUX) && SYSVERS <= 1111
    // WR PROD3851 (hpux 11.11 checkpoint on does not create .c file): the usage of __strtoull
    // (invoked on HP11.11 because of compilation issues with strtoull()) caused the problem; ull is not 
    // necessary as unsigned long int allows an increase in number of journals from 999 (previous limit)
    // to 4 billion, so strtoul() can be used.
    num = strtoul( ++bjrnp, &ejrnp, 10 );
#else
    num = strtoull( ++bjrnp, &ejrnp, 10 );
#endif
    if (!ejrnp || *ejrnp != '.' || (num == 0 && errno != 0))
    {
        reporterr(ERRFAC, M_JRNNAME, ERRCRIT, name);
	    ftd_debugf("%s", " parse_journal_name number conversion failure\n", name);
	    goto err_out;
    }

    switch (state_c)
    {
	case 'i':	/* incoherent */
	    *jrn_state = JRN_INCO;
	    *jrn_mode = JRN_ONLY;
	    *jrn_num = num;
	    break;
        case 'c':
	    *jrn_state = JRN_CO;
	    *jrn_mode = _rmd_cpon ? JRN_ONLY: JRN_AND_MIR;
	    *jrn_num = num;
	    break;
        case 'p':
	    *jrn_state = _rmd_jrn_state ? _rmd_jrn_state: JRN_CO;
	    *jrn_mode = _rmd_jrn_mode ? _rmd_jrn_mode: MIR_ONLY;
	    *jrn_num = num;
	    break;
	default: goto err_out;
    }
    return 0;
err_out:
    /* error */
    *jrn_state = JRN_INCO;
    *jrn_mode = MIR_ONLY;
    *jrn_num = -1;
    return -1;
} /* parse_journal_name */

/****************************************************************************
 * get_journals -- compile a list of journal paths
 * IN: prune  - 0: pickup .p
 *              1: ignore .p
 *     coflag - 0: pickup .i
 *              1: ignore .i
 ***************************************************************************/
int
get_journals(ftd_jrnpaths_t *jrnphp, int prune, int coflag)
{
    DIR *dfd;
    group_t *group;
    struct dirent *dent;
    char jrn_path[MAXPATH];
    char jrn_prefix[MAXPATH];
    int lstate, lmode;
    ftd_jrnnum_t lnum;
    int lgnum;
    int prefix_len;

    group = mysys->group;
    strcpy(jrn_path, group->journal_path);

    jrnpath_reset( jrnphp );
    /* read journal directory - compile a sorted list of journals */
    if ((dfd = opendir(jrn_path)) == NULL) {
        reporterr(ERRFAC, M_JRNPATH, ERRCRIT, jrn_path, strerror(errno));
        if (errno == ENOENT) {
            EXIT(EXITANDDIE);
        } else {
            return -1;
        }
     }

    lgnum = cfgpathtonum(mysys->configpath);
    prefix_len = sprintf(jrn_prefix, FTD_JRNLGNUM_PREFIX_FMT, lgnum);
    while (NULL != (dent = readdir(dfd))) {
        if (memcmp(dent->d_name, jrn_prefix, prefix_len)) {
            continue;
        }

        if (parse_journal_name(dent->d_name, &lnum, &lstate, &lmode) < 0) {
	    continue;
	}
	
        if (cp_file(dent->d_name, CP_ON)) {
            _rmd_jrn_cp_lnum = lnum;
            if (prune)
                continue;
        }
        if (coflag && lstate == JRN_INCO)
            continue;
	jrnpath_insert(jrnphp, dent->d_name);
    }
    (void)closedir(dfd);
    jrnpath_sort(jrnphp);
    return jrnpath_getcnt(jrnphp);
} /* get_journals */

/****************************************************************************
 * link_journal -- create hard link of journal file with inode-prepended name
 ***************************************************************************/
static void
link_journal(const char *journal)
{
    struct stat jrn_stat;
    static char *link_dir = NULL;
    static char link_path[MAXPATH];
    const char *slash;
    char *p;

    if (!link_dir) {
        strcpy(link_path, journal);
        p = strrchr(link_path, '/');
        if (p) {
            link_dir = p;
        } else {
            strcpy(link_path, "./");
            link_dir = link_path + 2;
        }
            
        link_dir += snprintf(link_dir,
                             sizeof(link_path) - (link_dir - link_path),
                             "/%s",
                             LINK_SUBDIR);
        mkdir(link_path, S_IRUSR|S_IXUSR|S_IWUSR |
                         S_IRGRP|S_IXGRP |
                         S_IROTH|S_IXOTH);
    }
    slash = strrchr(journal, '/');
    if (slash)
        slash++;
    else
        slash = journal;
    if (!stat(journal, &jrn_stat)) {
        snprintf(link_dir, sizeof(link_path) - (link_dir - link_path),
                           "/%llu.%s", (unsigned long long)jrn_stat.st_ino, slash);
        if (!link(journal, link_path)) {
            ftd_debugf("%s", "created link to %s\n", journal, link_path);
        } else {
            ftd_debugf("%s", "failed to created link to %s(%s)\n",
                journal, link_path, get_error_str(errno));
        }
    } else {
        ftd_debugf("%s", "failed to create link to %s(%s)\n",
            journal, link_path, get_error_str(errno));
    }
}

/****************************************************************************
 * rename_journals -- rename all journals
 ***************************************************************************/
int
rename_journals(int state)
{
    group_t *group;
    char jrn_path[MAXPATH];
    char old_jrn_path[MAXPATH];
    char *jrnname;
    char *dot;
    int jrncnt;
    int i;

    group = mysys->group;
    jrncnt = get_journals(jrnphp, 1, 0);

    for (i = 0; i < jrncnt; i++) {
	jrnname = jrnpath_get(jrnphp, i);
        dot = strrchr(jrnname, '.');
        /*
         * get_journals will only return '.i' or '.c' files
         */
        if (!dot || dot[1] == 'c' || state == JRN_INCO)
            continue;
        sprintf(old_jrn_path, "%s/%s", group->journal_path, jrnname);
        strcpy(jrn_path, old_jrn_path);
        dot = strrchr(jrn_path, '.');
        dot[1] = 'c'; /* We only rename '.i' to '.c' */
        DPRINTF("\n*** renaming old to new = %s, %s\n", old_jrn_path, jrn_path);
        rename(old_jrn_path, jrn_path);
        if (save_journals)
            link_journal(jrn_path);
    }
    return 0;
} /* rename_journals */

/****************************************************************************
 * nuke_journals -- unlink all journals
 * In:  DELETE_ALL          - delete all files include .i, .c, .p
 *      DELETE_JOURNAL_ONLY - delete .i and .c only
 ***************************************************************************/
int
nuke_journals(int flag)
{
    group_t *group;
    char jrn_path[MAXPATH];
    char jrn_prefix[MAXPATH];
    char *jrnname;
    int prefix_len;
    int lgnum;
    int jrncnt;
    int i;
    int prune = 0;
    int coflag = 0;

    if (flag == DELETE_JOURNAL_ALL) {
        prune = 1;
    }

    if (flag == DELETE_JOURNAL_CO) {
        coflag = 1;
    }

    if ((jrncnt = get_journals(jrnphp, prune, coflag)) <= 0) {
        return 0;
    }

    lgnum = cfgpathtonum(mysys->configpath);
    group = mysys->group;
    sprintf(jrn_prefix, FTD_JRNLGNUM_PREFIX_FMT, lgnum);
    prefix_len = strlen(jrn_prefix);

    for (i = 0; i < jrncnt; i++) {
	jrnname = jrnpath_get(jrnphp, i);
        if (strlen(jrnname)
        && strncmp(jrnname, jrn_prefix, prefix_len)) {
            continue;
        }
        sprintf(jrn_path, "%s/%s", group->journal_path, jrnname);
        UNLINK(jrn_path);
    }
    return 0;
} /* nuke_journals */

/****************************************************************************
 * new_journal -- create a new journal
 ***************************************************************************/
int
new_journal(int lgnum, int state, int clobber)
{
    group_t *group;
    headpack_t header[1];
    jrnheader_t jrnheader[1];
    struct stat statbuf[1];
    char tmp_path[MAXPATH];
    char jrn_path[MAXPATH];
    char jrn_prefix[MAXPATH];
    char lgname[32];
    char *last_journal;
    char *state_str;
    u_longlong_t rc;
    u_longlong_t uln;
    int jrncnt;
    ftd_jrnnum_t lnum;
    int lstate;
    int lmode;
    int jrn_pathlen;
    int fd;
    int i;
    int n;
    u_longlong_t lock_len;
    longlong_t unlock_len;

    group = mysys->group;
    strcpy(jrn_path, group->journal_path);
    jrn_pathlen = strlen(jrn_path);
    sprintf(jrn_prefix, FTD_JRNLGNUM_PREFIX_FMT, lgnum);
    jrncnt = get_journals(jrnphp, 0, 0);

    lnum = 0;
    if (clobber && state == JRN_INCO) {
        /* we are INCO - get rid of directly prior INCO journals */
        while (jrncnt > 0) {
            last_journal = jrnpath_getlast(jrnphp);
            if (cp_file(last_journal, CP_ON)) {
                break;
            }
            if (last_journal != NULL) {
                parse_journal_name(last_journal, &lnum, &lstate, &lmode);
                if (lstate != JRN_INCO)
                    break;
                sprintf(tmp_path, "%s/%s", jrn_path, last_journal);
                UNLINK(tmp_path);
            }
            jrncnt = get_journals(jrnphp, 0, 0);
        }
    }
    if (jrncnt > 0) {
        last_journal = jrnpath_getlast(jrnphp);
        parse_journal_name(last_journal, &lnum, &lstate, &lmode);
    }
    lnum++;
    if (state == JRN_CP) {
        /* if checkpoint already on or pending then return */
        for (i = 0; i < jrncnt; i++) {
            if (cp_file(jrnpath_get(jrnphp, i), CP_ON)) {
                if (i == 0) {
                    sprintf(lgname, FTD_JRNLGNUM_FMT, lgnum);
                    reporterr(ERRFAC, M_CPONAGAIN, ERRWARN, lgname);
                } else {
                    reporterr(ERRFAC, M_CPSTARTAGAIN, ERRWARN, argv0);
                }
                return 1;
            }
        }
        sprintf(tmp_path, "%s/%s." FTD_JRNNUM_FMT ".p", jrn_path, jrn_prefix, lnum);
        /* create a checkpoint-on file */
        if ((fd = open(tmp_path, O_CREAT, 0600)) == -1) {
            reporterr(ERRFAC, M_JRNOPEN, ERRCRIT, tmp_path, strerror(errno));
            return -1;
        }
        close(fd);
        return 0;
    }
    state_str = state == JRN_CO ? "c": "i";
    sprintf(tmp_path, "%s/%s." FTD_JRNNUM_FMT ".%s", jrn_path, jrn_prefix, lnum, state_str);

    /* open a new journal */
    if (group->jrnfd >= 0) {
        close_journal(group);
    }
    if ((group->jrnfd = open(tmp_path, O_RDWR | O_CREAT, 0600)) == -1) {
        reporterr(ERRFAC, M_JRNOPEN, ERRCRIT, tmp_path, strerror(errno));
        return -1;
    }
    if (save_journals)
        link_journal(tmp_path);

    /* lock it */
    // Note: lock_journal() will do an access() call using the journal file name expected in group->journal; 
    //       yet the file name is stored in group->journal below, after this block of instruction. The resulting
    //       behavior is that lock_journal() will return either:
    //  0: if the lock was acquired at the first attempt;
    //  1: if the lock necessitated at least 1 retry, in which case it checks wether the journal still exists
    //     or if it has been deleted (finished being applied);
    //  It is unclear wether this approach in the programming was voluntary; if so, it should have been documented;
    //  an attempt at fixing this by moving the instruction "strcpy(group->journal, tmp_path);" here seems to have caused
    //  problems (processes getting into futex wait state and journals remaining unapplied);
    lock_len = sizeof(jrnheader);
    if ((rc = lock_journal(0ULL, lock_len)) < 0) {
        reporterr(ERRFAC, M_JRNLOCK, ERRCRIT,
                group->journal, argv0);
        return -1;
    } else if (rc == 1) {
        /* file is gone */
        return -2;
    }

    strcpy(group->journal, tmp_path);
    parse_journal_name(tmp_path + jrn_pathlen + 1, &_rmd_jrn_num, &_rmd_jrn_state, &_rmd_jrn_mode);
    /* write header to journal */
    jrnheader->magicnum = MAGICJRN;
    jrnheader->state = _rmd_jrn_state;
    jrnheader->mode = _rmd_jrn_mode;

    rc = llseek(group->jrnfd, (offset_t)0, SEEK_SET);
    if (rc == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, group->journal, 0ULL, strerror(errno));
        return -1;
    }
    uln = n = sizeof(jrnheader_t);
    if (ftdwrite(group->jrnfd, (char*)jrnheader, n) != n) {
        geterrmsg(ERRFAC, M_WRITEERR, fmt);
        sprintf(msg, fmt, argv0, group->journal, 0,
            0ULL, uln, strerror(errno));
        logerrmsg(ERRFAC, ERRCRIT, M_WRITEERR, msg);
        senderr(mysys->sock, header, 0, ERRCRIT, M_WRITEERR, msg);
        return -1;
    }
    fsync(group->jrnfd);
    jrn_offset = _rmd_jrn_size = sizeof(jrnheader_t);

    /* unlock it */
    unlock_len = ~(lock_len)+1;
    if (unlock_journal(lock_len, unlock_len) < 0) {
        reporterr(ERRFAC, M_JRNUNLOCK, ERRCRIT,
                group->journal, argv0, strerror(errno));
        return -1;
    }

    return group->jrnfd;
} /* new_journal */

/****************************************************************************
 * get_lg_checkpoint -- check if .p exist. Only used by RMD.
 ***************************************************************************/
int
get_lg_checkpoint(void)
{
    int jrncnt = get_journals(jrnphp, 0, 1);
    int prefix_len;
    int i;
    int lgnum = cfgpathtonum(mysys->configpath);
    char jrn_prefix[MAXPATH];
    char *jrnname;

    prefix_len = sprintf(jrn_prefix, FTD_JRNLGNUM_PREFIX_FMT, lgnum);

    for (i = 0; i < jrncnt; i++) {
	jrnname = jrnpath_get( jrnphp, i);
        if (strlen(jrnname) &&
            strncmp(jrnname, jrn_prefix, prefix_len)) {
            continue;
        }

        if (cp_file(jrnname, CP_ON)) {
            return 1;
        }
    }
    return 0;
}

/****************************************************************************
 * close_journal -- close journal
 ***************************************************************************/
int
close_journal(group_t *group)
{
    fsync(group->jrnfd);  // Sync data to disk (PROD7131)
    close(group->jrnfd);
    group->jrnfd = -1;
    memset(group->journal, 0, sizeof(group->journal));

    return 0;
} /* close_journal */

/****************************************************************************
 * clobber_journal -- clobber journal
 ***************************************************************************/
static void
clobber_journal(int *jrnfdp, char *journal)
{
    ftd_trace_flow(FTD_DBG_FLOW8, "\n*** fd = %d clobber_journal %s \n", (jrnfdp == NULL ? -1: *jrnfdp), journal);
    UNLINK(journal);
    if (jrnfdp) {
        close(*jrnfdp);
        *jrnfdp = -1;
    }
    *journal = '\0';

} /* clobber_journal */

/**
 * Leaves a trace that a corrupted journal has been found and stops the RMD.
 */
void flag_corrupted_journal_and_stop_rmd(int lgnum)
{
    flag_corrupted_journal(lgnum);
    reporterr(ERRFAC, M_RMDA_CORR_JRN, ERRCRIT, lgnum);
    stop_rmd();
}

/****************************************************************************
 * apply_journal -- apply journal entries to mirrors
 ***************************************************************************/
int
apply_journal(group_t *group)
{
    headpack_t local_header;
    headpack_t *header = NULL;
    char *datap;
    static char *databuf = NULL;
    static int datalen = 0;
    struct stat statbuf;
    u_longlong_t offset;
    u_longlong_t bytes_remain;
    int chunksize;
    u_longlong_t lock_len, lock_end;
    ssize_t inc;
    int i;
    int chunkparts = 0;
    u_longlong_t expected_sequence_number= 0;
    time_t last_time_stamp= 0;
    int first_header = 1;
    int lgnum = cfgpathtonum(mysys->configpath);
        
    ftd_trace_flow(FTD_DBG_FLOW1,
            "\n*** apply_journal: _rmd_jrn_state = %d\n",_rmd_jrn_state);
    ftd_trace_flow(FTD_DBG_FLOW1,
            "\n*** apply_journal: _rmd_jrn_mode = %d\n",_rmd_jrn_mode);
    ftd_trace_flow(FTD_DBG_FLOW1,
            "\n*** apply_journal: group->jrnfd, journal = %d, %s\n", group->jrnfd,group->journal);

    jrn_offset = sizeof(jrnheader_t);
    _rmd_jrn_size = -1;

    ftd_trace_flow(FTD_DBG_FLOW1,
            "\n*** apply_journal: jrn_offset = %llu\n",jrn_offset);

    _rmd_jrn_chunksize = 1024*1024;
    lock_len = _rmd_jrn_chunksize+1;

    /* process the journal */
    if (save_journals) {
        fstat(group->jrnfd, &statbuf);
        ftd_debugf("%d", "applying %llu.%s\n",
            group->jrnfd, (unsigned long long)statbuf.st_ino, group->journal);
    }
    while (1) {
        if ((offset = llseek(group->jrnfd, jrn_offset, SEEK_SET)) == -1) {
            reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
                argv0, group->journal, jrn_offset, strerror(errno));
            return -1;
        }

        /*
         * if we lock past EOF with chunksize then we will
         * prevent the rmd from writing and changing the
         * file size out from under us
         */

/*
always lock for journal "chunksize"
        if ((_rmd_jrn_size-offset) < lock_len) {
            lock_len = (_rmd_jrn_size-offset);
        }
*/
        /*
         * don't try to lock past MAXJRNL
         */
        if ((lock_end = (offset + lock_len)) > MAXJRNL) {
            lock_len = (MAXJRNL - offset);
        }
        ftd_trace_flow(FTD_DBG_FLOW1,
            "\n$$$ %s: locking journal, offset=%llu, length=%llu, lock_end=%llu\n",
            argv0,offset,lock_len,lock_end);

        if (lock_journal(offset, lock_len) == -1) {
            reporterr(ERRFAC, M_JRNLOCK, ERRCRIT,
                group->journal, argv0);
            return -1;
        }
        /* we have a lock on the section - get the size */
        if (fstat(group->jrnfd, &statbuf) == -1) {
            reporterr(ERRFAC, M_STAT, ERRCRIT, group->journal, strerror(errno));
            return -1;
        }

        _rmd_jrn_size = statbuf.st_size;

        ftd_trace_flow(FTD_DBG_FLOW8,
            "\n*** jrn_offset = %llu\n",jrn_offset);
        ftd_trace_flow(FTD_DBG_FLOW8,
            "\n*** _rmd_jrn_size = %llu\n",_rmd_jrn_size);

        bytes_remain = _rmd_jrn_size - jrn_offset;
        
        if (header && bytes_remain < header->len)
        {
            // We have incomplete data at the end of the journal.
            reporterr(ERRFAC, M_JRNINCDATA, ERRWARN, group->journal);
            // Just ignore the entry.
            bytes_remain = 0;
        } 
        if (!header && bytes_remain > 0 && bytes_remain < sizeof(*header))
        {
            // We have an incomplete header at the end of the journal.
            reporterr(ERRFAC, M_JRNINCDATAHEAD, ERRWARN, group->journal);
            // It will be ignored.
        }
        
        chunksize = _rmd_jrn_chunksize < bytes_remain ? _rmd_jrn_chunksize:
                    bytes_remain;

        if (bytes_remain <= sizeof(*header)) {
#if defined(linux)
            /*
             * before clobbering the journal, sync all data to the devices.
             */
            syncgroup(1);
#endif
            if (llseek(group->jrnfd, 0L, SEEK_SET) == -1) {
                reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
                    argv0, group->journal, 0, strerror(errno));
                return -1;
            }
            if (lockf(group->jrnfd, F_LOCK, 0L) == -1) {
                 reporterr(ERRFAC, M_JRNLOCK, ERRCRIT,
                    group->journal, argv0);
            }
            if (fstat(group->jrnfd, &statbuf) == -1) {
                reporterr(ERRFAC, M_STAT, ERRCRIT, group->journal, strerror(errno));
                return -1;
            }
            if (statbuf.st_size != _rmd_jrn_size) {
               syslog(LOG_WARNING, "%s\"%s\":%d: "
                                   "apply_journal(%s) file size change %#lx(new) %#lx(old)\n",
                                   argv0, __FILE__, __LINE__,
                                   group->journal,
                                   (unsigned long)statbuf.st_size,
                                   (unsigned long)_rmd_jrn_size);
               lockf(group->jrnfd, F_ULOCK, 0L);
               close(group->jrnfd);
               group->jrnfd = -1;
            } else {
                clobber_journal(&group->jrnfd, group->journal);
		// NAA - Leaving comment until we get past 2.6.6 release.
		// After rmd.c:apply_journal is called the function 
		// get_journals is called, so clearing the journal path
		// list is not useful and we have no concurrency to worry
		// about for side effects.
                // memset(jrnpaths[0], 0, sizeof(jrnpaths[0]));
            }
            return 0;
        }
        offset = 0;
        if (!header) {
            /* read first header */
            header = &local_header;
            if (read(group->jrnfd, (char *)header, sizeof(*header)) != sizeof(*header)) {
                reporterr(ERRFAC, M_READERR, ERRCRIT,
                    group->journal, jrn_offset, sizeof(*header), get_error_str(errno));
                return -1;
            }
            if (header->magicvalue != MAGICHDR) {
                /*
                 * Dump out the particulars in the dtc error log
                 */
                ftd_debugf("%s", "bad %lx != %lx offset %#llx chunksize %#x header->len %d\n",
                       group->journal, header->magicvalue, (unsigned long)MAGICHDR,
                       offset, chunksize, header->len);
                reporterr(ERRFAC, M_JRNHEADER, ERRCRIT, group->journal);
                flag_corrupted_journal_and_stop_rmd(lgnum);
                EXIT(EXITANDDIE);
            }

            if (first_header)
            {
                expected_sequence_number = header->lgsn;
                last_time_stamp = header->ts;
                first_header = 0;
            }

            if (header->lgsn != expected_sequence_number++ || header->ts < last_time_stamp)
            {
                reporterr(ERRFAC, M_JRNHEADER, ERRCRIT, group->journal);
                flag_corrupted_journal_and_stop_rmd(lgnum);
                EXIT(EXITANDDIE);
            }
            
            last_time_stamp = header->ts;
                
            offset += sizeof(*header);
        }

        /* process journal chunk entries */
        inc = 0;
	chunkparts = 0;
        while ((offset + header->len) <= chunksize) {
            /*
             * Read data for current header and, chunksize permitting,
             * the next header.
             */
            inc = header->len + sizeof(*header);
            if ((offset + inc) > chunksize)
                inc -= sizeof(*header);

            ftd_trace_flow(FTD_DBG_FLOW8,
                "\n*** chunksize %d offset %llu header->len %d inc %d\n",
                chunksize, offset, header->len);

            local_header = *header;
            if (inc > datalen) {
                datalen = inc;
                ftdfree(databuf);
                databuf = (char *)ftdvalloc(datalen);
            }
            if (read(group->jrnfd, databuf, inc) < inc) {
                int err = errno;
                /*
                 * Dump out the particulars in the dtc error log
                 */
                ftd_debugf("%s","jrn_offset %#llx offset %#llx inc %d\n",
                       group->journal, jrn_offset, offset, inc);
                fflush(stdout);
                reporterr(ERRFAC, M_READERR, ERRCRIT,
                    group->journal, offset, inc, get_error_str(err));
                return -1;
            }
            offset += inc;

	    chunkparts++;
            /* write data to appropriate mirror */
            if (write_jrn_to_mir(&local_header, databuf) == -1) {
                reporterr(ERRFAC, M_JRNWRITE, ERRCRIT, argv0, group->journal);
                return -1;
            }
            if (inc <= local_header.len) {
                header = NULL;
                break;
            }
            header = (headpack_t *)&databuf[inc - sizeof(*header)];
            if (header->magicvalue != MAGICHDR) {
                /*
                 * Dump out the particulars in the dtc error log
                 */
                ftd_debugf("%s", "bad header %#lx jrn_offset %#llx "
                       "offset %#llx chunksize %#x header->len %#x inc %#x\n",
                       group->journal, header->magicvalue, jrn_offset,
                       offset, chunksize, header->len, inc);
                reporterr(ERRFAC, M_JRNHEADER, ERRCRIT, group->journal);
                flag_corrupted_journal_and_stop_rmd(lgnum);
                EXIT(EXITANDDIE);
            }
            
            if (header->lgsn != expected_sequence_number++ || header->ts < last_time_stamp)
            {
                reporterr(ERRFAC, M_JRNHEADER, ERRCRIT, group->journal);
                flag_corrupted_journal_and_stop_rmd(lgnum);
                EXIT(EXITANDDIE);
            }
            
            last_time_stamp = header->ts;
        }
        /* unlock the locked journal section */
        if (unlock_journal(jrn_offset+lock_len, (~(lock_len)+1)) == -1) {
            reporterr(ERRFAC, M_JRNUNLOCK, ERRCRIT,
                group->journal, argv0, strerror(errno));
            return -1;
        }
        if (!inc) {
            _rmd_jrn_chunksize *= 2;
            lock_len = _rmd_jrn_chunksize+1;
        }
        jrn_offset += offset;
    }
} /* apply_journal */

/****************************************************************************
 * journal_writable -- check for journal area writable
 ***************************************************************************/
int
journal_writable(char *jrnpath)
{
    char testf[128];
    int fd;
    struct stat statbuf;
    int save_errno = 0;

    if (stat(jrnpath, &statbuf) != 0) {
        save_errno = errno;
        if (save_errno == ENOENT) {
            reporterr(ERRFAC, M_JRNACCESS, ERRCRIT,
                mysys->group->journal_path, strerror(save_errno));
            reporterr(ERRFAC, M_JRNPATH, ERRCRIT, jrnpath, strerror(save_errno));
            EXIT(EXITANDDIE);
        } else if (save_errno != ENOTDIR) {
            return 0;
        }
    }
    if (!S_ISDIR(statbuf.st_mode))
        return 0;

    sprintf(testf, "%s/testf",jrnpath);
    if ((fd = open(testf, O_RDWR | O_CREAT, 0600)) == -1) { /* WR16793 */
        return 0;
    }
    if (ftdwrite(fd, testf, sizeof(testf)) != sizeof(testf)) {
        close(fd);
        UNLINK(testf);
        return 0;
    }
    close(fd);
    UNLINK(testf);
    return 1;
}

/****************************************************************************
 * init_journal -- initialize secondary journaling
 ***************************************************************************/
int
init_journal(void)
{
    char journal[MAXPATH];
    char *jrn_path;
    int jrncnt;
    int rmd = !strncmp(argv0, "RMD_", strlen("RMD_"));
    int i;

    /* set global defaults */
    ftd_trace_flow(FTD_DBG_FLOW1,
            "_rmd_cppend: %d -> 0; _rmd_cpon: %d -> 0\n",
             _rmd_cppend, _rmd_cpon);
    _rmd_cpon = 0;
    _rmd_cppend = 0;
    _rmd_jrn_state = JRN_CO;
    _rmd_jrn_mode = MIR_ONLY;
    _rmd_jrn_num = 0;

    if (!journal_writable(mysys->group->journal_path)) {
        reporterr(ERRFAC, M_JRNACCESS, ERRCRIT,
            mysys->group->journal_path, strerror(errno));
        return -1;
    }

    /* see if we were in checkpoint mode */
    jrncnt = get_journals(jrnphp, 0, 0);
    for (i = 0; i < jrncnt; i++) {
        if (cp_file(jrnpath_get(jrnphp, i), CP_ON)) {
            if (i == 0) {
                ftd_trace_flow(FTD_DBG_FLOW1, "_rmd_cpon: %d -> 1\n",
                        _rmd_cpon);
                _rmd_cpon = 1;
            } else {
                ftd_trace_flow(FTD_DBG_FLOW1,
                        "_rmd_cppend: %d -> 1\n", _rmd_cppend);
                _rmd_cppend = 1;
            }
            break;
        }
    }
    while ((jrncnt = get_journals(jrnphp, 1, 0)) > 0) {
        /* this time prune checkpoint files */
        jrn_path = jrnpath_getlast(jrnphp);    /* most recent journal file */
        if (!strlen(jrn_path)) {
            _rmd_jrn_num = 0;
            _rmd_jrn_state = JRN_CO;
            _rmd_jrn_mode = MIR_ONLY;
            break;
        }
        parse_journal_name(jrn_path, &_rmd_jrn_num,
                           &_rmd_jrn_state, &_rmd_jrn_mode);
        if (_rmd_jrn_state != JRN_INCO) {
            /* coherent journal found */
            break;
        }
        /* weed out previous inco journals - if we are the RMD */
        if (!rmd)
            break;

        // Now, even if we are the RMD, since we have smarter smart refreshes, we do not want to delete existing .i journal entries,
        // as their content will not be sent again.
        break;
    }
    _rmd_jrn_chunksize = 1024 * 1024;
    if (_rmd_cpon || _rmd_cppend) {
        _rmd_jrn_mode = JRN_ONLY;
    }

/*
    if (inco_flag) {
        * INCO journals were found - we were previously in an INCO state *
        _rmd_jrn_state = JRN_INCO;
    }
*/
    ftd_trace_flow(FTD_DBG_FLOW1,
            "init_j: cpon: %d, jrn_num: %d, jrn_state: %d, jrn_mode = %d \n",
            _rmd_cpon, _rmd_jrn_num, _rmd_jrn_state, _rmd_jrn_mode);

    mysys->group->jrnfd = -1;
    memset(mysys->group->journal, 0, sizeof(mysys->group->journal));

    return 0;
} /* init_journal */

/****************************************************************************
 * get_counts -- get device counts from entries
 ***************************************************************************/
int
get_counts(wlheader_t *entry, char *dataptr)
{
    group_t *group;
    sddisk_t *sd;
    int entrylen;

    group = mysys->group;

    if ((sd = get_lg_rdev(group, entry->diskdev)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, (unsigned long)entry->diskdev);
/* XXXTEMP + */
        kill(getpid(), 10);
/* XXXTEMP - */
        EXIT(EXITANDDIE);
    }
    entrylen = entry->length << DEV_BSHIFT;

    if (mysys->tunables.compression) {
        /* estimate this - since no way of knowing per device actual */
        sd->stat.a_datacnt += (compr_ratio*entrylen);
        sd->stat.a_tdatacnt += (compr_ratio*entrylen);
    } else {
        sd->stat.a_datacnt += entrylen;
        sd->stat.a_tdatacnt += entrylen;
    }
    sd->stat.e_datacnt += entrylen;
    sd->stat.e_tdatacnt += entrylen;
    sd->stat.entries++;

    return 0;
} /* get_counts */

/****************************************************************************
 * entry_noop -- return entry length
 ***************************************************************************/
int
entry_noop(wlheader_t *entry, char *dataptr)
{
    return (entry->length << DEV_BSHIFT);
} /* entry_noop */

/****************************************************************************
 * entry_null -- is entry all zeros ?
 ***************************************************************************/
int
entry_null(wlheader_t *entry, char *dataptr)
{
    int i;

    for (i = 0; i < entry->length; i++) {
        if (dataptr[i]) {
            return 0;
        }
    }
    return 1;
} /* entry_null */

/****************************************************************************
 * write_jrn_to_mir -- RMD - write journal entry to mirror device
 ***************************************************************************/
static int
write_jrn_to_mir (headpack_t *header, char *datap)
{
    group_t *group;
    sddisk_t *sd;
    u_longlong_t offset;
    u_longlong_t length;
    u_longlong_t vsize;
    time_t currentts;
    int len32;
    int err;

    group = mysys->group;

    if ((sd = get_lg_dev(group, header->devid)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, header->devid);
        return -1;
    }
    offset = header->offset;
    offset <<= DEV_BSHIFT;
    length = header->len;

    if (offset == 0 && sd->no0write) {
        ftd_trace_flow(FTD_DBG_FLOW8,
                "\n*** skipping sector 0 write on mirror: %s\n",sd->mirname);
        offset += DEV_BSIZE;
        datap += DEV_BSIZE;
        length -= DEV_BSIZE;
    }
    ftd_trace_flow(FTD_DBG_FLOW8,
            "\n*** write_jrn_to_mir: %llu bytes @ offset %llu\n",length, offset);
    ftd_trace_flow(FTD_DBG_FLOW8,
            "\n*** write_jrn_to_mir: buffer address %d\n",datap);

    /* ignore operations that write beyond the end of device */
    vsize = sd->mirsize * (u_longlong_t) DEV_BSIZE;
    if (offset >= vsize) {
        return 0;
    }

    if (llseek(sd->mirfd, offset, SEEK_SET) == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, sd->mirname, offset, strerror(errno));
        return -1;
    }
    /* truncate writes that extend beyond the end of the device */
    if ((offset + length) > vsize) {
        length = vsize - offset;
    }
    len32 = length;
    if (write(sd->mirfd, page_aligned(datap, len32), len32) != len32) {
        err = errno;
        geterrmsg(ERRFAC, M_WRITEERR, fmt);
        sprintf(msg, fmt, argv0, sd->mirname, sd->devid,
            offset, length, strerror(err));
        logerrmsg(ERRFAC, ERRCRIT, M_WRITEERR, msg);
        senderr(mysys->sock, header, 0, ERRCRIT, M_WRITEERR, msg);
        return -1;
    }
    time(&currentts);
    sd->stat.jrnentage = (currentts - header->ts) - mysys->tsbias;

    return 0;
} /* write_jrn_to_mir */

/****************************************************************************
 * update_net_analysis_stats -- RMD: update stats in network analysis mode
 ***************************************************************************/
void
update_net_analysis_stats (wlheader_t *entry)
{
    group_t *group;
    sddisk_t *sd;
    u_longlong_t length;

    group = mysys->group;

    if ((sd = get_lg_rdev(group, entry->diskdev)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, (unsigned long)entry->diskdev);
        EXIT(EXITANDDIE);
    }
    length = entry->length;
    length <<= DEV_BSHIFT;

    if (mysys->tunables.compression) {
        sd->stat.a_tdatacnt += compr_ratio*(sizeof(headpack_t)+length);
    } else {
        sd->stat.a_tdatacnt += sizeof(headpack_t)+length;
    }
    sd->stat.e_tdatacnt += sizeof(headpack_t)+length;
    sd->stat.entries++;
    return;
} /* update_net_analysis_stats */

/****************************************************************************
 * write_mirror -- RMD - write entry to mirror
 * note: if in network bandwidth analysis mode, just update network stats (no
 *       physical device write)
 ***************************************************************************/
int
write_mirror (wlheader_t *entry, char *dataptr)
{
    group_t *group;
    headpack_t header[1];
    sddisk_t *sd;
    u_longlong_t offset;
    u_longlong_t length;
    u_longlong_t vsize;
    int len32;
    int lgnum;
    int rc;

    // If in network bandwidth analysis mode, just update network stats and return.
    if( _net_bandwidth_analysis )
	{
        update_net_analysis_stats( entry );
		return( 0 );
	}

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    if ((sd = get_lg_rdev(group, entry->diskdev)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, (unsigned long)entry->diskdev);
        EXIT(EXITANDDIE);
    }
    offset = entry->offset;
    offset <<= DEV_BSHIFT;
    length = entry->length;
    length <<= DEV_BSHIFT;

    if (offset == 0 && sd->no0write) {
        DPRINTF("\n*** skipping sector 0 write on mirror: %s\n",sd->mirname);
        offset += DEV_BSIZE;
        dataptr += DEV_BSIZE;
        length -= DEV_BSIZE;
    }
    ftd_trace_flow(FTD_DBG_FLOW1,
            "%llu bytes @ offset %llu\n", length, offset);

    /* ignore operations that write beyond the end of the device */
    vsize = sd->mirsize * (u_longlong_t) DEV_BSIZE;
    if (offset >= vsize) {
        return 0;
    }

    if (llseek(sd->mirfd, offset, SEEK_SET) == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, sd->mirname, offset, strerror(errno));
        return -1;
    }
    /* truncate writes that span beyond end of device */
    if ((offset + length) > vsize) {
        length = vsize - offset;
    }
    len32 = length;
    if (write(sd->mirfd, page_aligned(dataptr, len32), len32) != len32) {
        int err = errno;
        int flags = fcntl(sd->mirfd, F_GETFL);
        geterrmsg(ERRFAC, M_WRITEERR, fmt);
        sprintf(msg, fmt, argv0, sd->mirname, sd->devid,
            offset, length, get_error_str(err));
        logerrmsg(ERRFAC, ERRCRIT, M_WRITEERR, msg);
        senderr(mysys->sock, header, 0, ERRCRIT, M_WRITEERR, msg);
        return -1;
    }
    /*
     * mark the disk dirty and flush (data sync) to be triggered later
     */
    sd->dirty = 1;
    if (mysys->tunables.compression) {
        sd->stat.a_tdatacnt += compr_ratio*(sizeof(headpack_t)+length);
    } else {
        sd->stat.a_tdatacnt += sizeof(headpack_t)+length;
    }
    sd->stat.e_tdatacnt += sizeof(headpack_t)+length;
    sd->stat.entries++;
    return 0;
} /* write_mirror */

/****************************************************************************
 * write_journal -- RMD - write entry to journal
 ***************************************************************************/
int
write_journal (wlheader_t *entry, char *dataptr)
{
    headpack_t header[1];
    group_t *group;
    sddisk_t *sd;
    int length;
    u_longlong_t new_rmd_jrn_size;
    u_longlong_t ulength;
    time_t currentts;
    int lgnum;
    int rc;
    int i;
    int jrn_locked = 0;
    u_longlong_t lock_len = 0;
    longlong_t unlock_len;
    int lstate, lmode, jrncnt;
    ftd_jrnnum_t lnum;
    static u_longlong_t sequence_number= 0;
    group = mysys->group;

    lgnum = cfgpathtonum(mysys->configpath);

    if ((sd = get_lg_rdev(group, entry->diskdev)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, header->devid);
        EXIT(EXITANDDIE);
    }

    length = (entry->length << DEV_BSHIFT);

    /* static header values */
    memset(header, 0, sizeof(headpack_t));
    header->magicvalue = MAGICHDR;
    header->cmd = CMDWRITE;
    header->lgsn = sequence_number++;
    header->devid = sd->devid;
    header->offset = entry->offset;
    header->len = length;
    header->ackwanted = 0;
    time(&header->ts);

    /* if write will make journal > threshold then create new journal */
    new_rmd_jrn_size = sizeof(headpack_t)+length+_rmd_jrn_size;
      
    if (new_rmd_jrn_size > MAXJRNL) {
        group->jrnfd = new_journal(lgnum, _rmd_jrn_state, 0);
        if (group->jrnfd == -2) {
            /* file is gone */
            /* re-open mirrors */
            closedevs(O_RDWR | O_EXCL);
            _rmd_jrn_state = JRN_CO;        
            _rmd_jrn_mode = MIR_ONLY;
            return write_mirror(entry, dataptr);
        }
        /* lock it */
        lock_len = length;
        if ((rc = lock_journal(0ULL, lock_len)) < 0) {
            reporterr(ERRFAC, M_JRNLOCK, ERRCRIT,
                    group->journal, argv0);
            return -1;
        } else if (rc == 1) {
            /* file is gone */
            /* re-open mirrors */
            closedevs(O_RDWR | O_EXCL);
            _rmd_jrn_state = JRN_CO;        
            _rmd_jrn_mode = MIR_ONLY;        
            return write_mirror(entry, dataptr);
        }
        jrn_locked = 1;
    } else if (group->jrnfd <= 0) {
        if ((jrncnt = get_journals(jrnphp, 0, 0)) <= 0) {
            /* no journals. We should never be here.
               Journals files should be created if we are in this function */
            return -1;
        }
        for (i = 0; i < jrncnt; i++) {
          if (cp_file(jrnpath_get(jrnphp, i), CP_ON)) {
             if (i == 0) {
                ftd_trace_flow(FTD_DBG_FLOW1, "_rmd_cpon: %d -> 1\n",
                        _rmd_cpon);
                _rmd_cpon = 1;
             } else {
                ftd_trace_flow(FTD_DBG_FLOW1,
                        "_rmd_cppend: %d -> 1\n", _rmd_cppend);
               _rmd_cppend = 1;
             }
             break;
          }
        }
        
        parse_journal_name(jrnpath_getlast(jrnphp), &lnum, &lstate, &lmode);
        _rmd_jrn_num = lnum;
        _rmd_jrn_state = lstate;
        _rmd_jrn_mode = lmode;

        sprintf(group->journal, "%s/%s",group->journal_path, jrnpath_getlast(jrnphp));
        if ((group->jrnfd = open(group->journal, O_RDWR)) == -1) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, group->journal, strerror(errno));
        }
        _rmd_jrn_size = llseek(group->jrnfd, (offset_t)0, SEEK_END);
    }
    if (llseek(group->jrnfd, _rmd_jrn_size, SEEK_SET) == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, group->journal, _rmd_jrn_size, strerror(errno));
        return -1;
    }
    rc = ftdwrite(group->jrnfd, (char*)header, sizeof(headpack_t));
    if (rc != sizeof(headpack_t)) {
        if (errno == ENOSPC) {
            geterrmsg(ERRFAC, M_JRNSPACE, fmt);
            sprintf(msg, fmt, argv0, group->journal);
            logerrmsg(ERRFAC, ERRCRIT, M_JRNSPACE, msg);
            senderr(mysys->sock, header, 0, ERRCRIT, M_JRNSPACE, msg);
            EXIT(EXITANDDIE);
        } else {
            geterrmsg(ERRFAC, M_WRITEERR, fmt);
            ulength = sizeof(headpack_t);
            sprintf(msg, fmt, argv0, group->journal, sd->devid,
                _rmd_jrn_size, ulength, strerror(errno));
            logerrmsg(ERRFAC, ERRCRIT, M_WRITEERR, msg);
            senderr(mysys->sock, header, 0, ERRCRIT, M_WRITEERR, msg);
            EXIT(EXITANDDIE);
        }
    }
        if ((rc = ftdwrite(group->jrnfd, dataptr, length)) != length) {
        if (errno == ENOSPC) {
            geterrmsg(ERRFAC, M_JRNSPACE, fmt);
            sprintf(msg, fmt, argv0, group->journal);
            logerrmsg(ERRFAC, ERRCRIT, M_JRNSPACE, msg);
            senderr(mysys->sock, header, 0, ERRCRIT, M_JRNSPACE, msg);
            EXIT(EXITANDDIE);
        } else {
            geterrmsg(ERRFAC, M_WRITEERR, fmt);
            ulength = length;
            sprintf(msg, fmt, argv0, group->journal, sd->devid,
                _rmd_jrn_size, ulength, strerror(errno));
            logerrmsg(ERRFAC, ERRCRIT, M_WRITEERR, msg);
            senderr(mysys->sock, header, 0, ERRCRIT, M_WRITEERR, msg);
            EXIT(EXITANDDIE);
        }
    }
    fsync(group->jrnfd);   
    _rmd_jrn_size += sizeof(headpack_t)+length;

    if (mysys->tunables.compression) {
        sd->stat.a_tdatacnt += compr_ratio*(sizeof(headpack_t)+length);
    } else {
        sd->stat.a_tdatacnt += sizeof(headpack_t)+length;
    }
    sd->stat.e_tdatacnt += sizeof(headpack_t)+length;
    sd->stat.entries++;
    time(&currentts);
    sd->stat.entage = (currentts - header->ts) - mysys->tsbias;

    if (jrn_locked) {
        unlock_len = ~(lock_len)+1;
        if (unlock_journal(lock_len, unlock_len) < 0) {
            reporterr(ERRFAC, M_JRNUNLOCK, ERRCRIT,
                group->journal, argv0, strerror(errno));
            return -1;
        }
    }

    return 0;
} /* write_journal */

/****************************************************************************
 * lock_journal -- lock a journal file
 ***************************************************************************/
int
lock_journal(u_longlong_t offset, u_longlong_t lock_len)
{
    struct stat statbuf[1];
    group_t *group;
    u_longlong_t ret;
    int tries;
    int rc;
    int i;
#ifdef TDMF_TRACE
    int note_retry_needed = 0;
#endif

    group = mysys->group;

    ret = llseek(group->jrnfd, offset, SEEK_SET);
    if (ret == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, group->journal, offset, strerror(errno));
        return -1;
    }
    tries = 750;  // WR 3219: on 200 group configs in loopback saw retries go close to 500
    for (i=0;i<tries;i++) {
        DPRINTF("\n*** [%s] locking journal: %s, offset, \
            length = %llu, %llu\n",argv0, group->journal, offset, lock_len);
        if ((rc = lockf(group->jrnfd, F_TLOCK, lock_len)) == 0) {
            break;
        }
        DPRINTF("\n*** [%s] journal lock failure: %s\n",group->journal, strerror(errno));
/*
 * RMDA data inconsistency -
 * moved the sleep() here, to remove the gap between
 * checking for file existence and writing to it
 */
        sleep(3);		 // WR PROD00003219; 1 as argument might not sleep for 1 second

        if (access(group->journal, F_OK) == -1) {
            /* must be done applying */
            return 1;
        }
 
        /* WR PROD00003219, information trace to confirm heavy load situation previously causing failure;
           to appear in debug mode only */
#ifdef TDMF_TRACE
        if (i == 200)
	    {
           ftd_trace_flow(FTD_DBG_FLOW1, "lock_journal: extended wait period on journal %s (heavy load detected).\n", group->journal);
           note_retry_needed = 1;
		}
#endif
    }
    if (i && access(group->journal, F_OK) == -1) {
        /* must be done applying */
        return 1;
    } else if (i == tries) {
        return -1;
    }
    ftd_trace_flow(FTD_DBG_FLOW1,
            "[%s] aquired lock on journal: %s [%llu-(%llu)]\n",
            argv0, group->journal, offset, lock_len);

#ifdef TDMF_TRACE
    if(note_retry_needed)
    {
        ftd_trace_flow(FTD_DBG_FLOW1, "%s, lock_journal: JOURNAL %s needed a number of trials of %d before being successfully locked.\n", argv0, group->journal, i );
    }
#endif

    return 0;
} /* lock_journal */

/****************************************************************************
 * unlock_journal -- unlock a journal file
 ***************************************************************************/
int
unlock_journal(u_longlong_t offset, longlong_t lock_len)
{
    group_t *group;
    u_longlong_t ret;

    group = mysys->group;

#if defined(linux)
    ret = llseek(group->jrnfd, offset + lock_len, SEEK_SET);
#else
    ret = llseek(group->jrnfd, offset, SEEK_SET);
#endif /* defined(linux) */
    if (ret == -1) {
#if defined(linux)
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, group->journal, offset + lock_len, strerror(errno));
#else
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, group->journal, offset, strerror(errno));
#endif /* defined(linux) */
        return -1;
    }
#if defined(linux)
    if (lockf(group->jrnfd, F_ULOCK, offset) == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
                argv0, group->journal, offset, strerror(errno));
#else
    if (lockf(group->jrnfd, F_ULOCK, lock_len) == -1) {
#endif /* defined(linux) */
        return -1;
    }
    ftd_trace_flow(FTD_DBG_FLOW1,
            "[%s] released lock on journal: %s [%llu-(%lld)]\n",
            argv0, group->journal, offset, lock_len);
    return 0;
} /* unlock_journal */

/**
 * Logs the given binary data.
 *
 * @param description The description is reprinted verbatim in the binary dump's header.
 * @param binary_data The binary data we want to log.
 * @param data_length The amount of data to log.
 *
 * The data is logged in hex, with currently 16 bytes per log line.
 *
 */
static void log_binary_data(const char* description, unsigned char* binary_data, int data_length)
{
    static const int BYTES_PER_LOG_LINE = 16;
    int i = 0;
    sprintf(msg, "Dumping %d bytes from %s:\n", data_length, description);
    reporterr(ERRFAC, M_GENMSG, ERRCRIT, msg);

    msg[0] = 0x0;

    for (i = 0; i < data_length; i++)
    {
        sprintf(fmt, "%02X:", binary_data[i]);
        strcat(msg, fmt);

        if( (i+1) % BYTES_PER_LOG_LINE == 0 || (i+1) == data_length)
        {
            strcat(msg, "\n");
            reporterr(ERRFAC, M_GENMSG, ERRCRIT, msg);
            msg[0] = 0x0;
        }
    }
}

/**
 * Logs the contents of the wlheader and up to 128 bytes available before it.
 *
 * @param wlhead        A pointer to the header.
 * @param wlhead_size   The exact size of the wlheader, as there are two variants (wlheader_t and wlheader32_t)
 * @param chunk         A pointer to the beginning of the chunk within which the header should be.
 *
 * Used with the hopes to shed some light on WR PROD00002617.
 */
static void log_wlheader_and_previous_data_helper(const char* wlhead, int wlhead_size, const char* chunk)
{
    int bytes_available_before = (char*)wlhead - (char*)chunk;
    int bytes_to_log_before = 128;
    if(bytes_available_before < bytes_to_log_before)
    {
        bytes_to_log_before = bytes_available_before;
    }
    
    log_binary_data("before wlheader", (unsigned char*)wlhead-bytes_to_log_before, bytes_to_log_before);
    log_binary_data("wlheader", (unsigned char*)wlhead, wlhead_size);
}

/**
 * Logs the contents of the wlheader and up to 128 bytes available before it.
 *
 * @param wlhead  A pointer to the header.
 * @param chunk   A pointer to the beginning of the chunk within which the header should be.
 *
 * Used with the hopes to shed some light on WR PROD00002617.
 */
void log_wlheader_and_previous_data(const wlheader_t* wlhead, const char* chunk)
{
    log_wlheader_and_previous_data_helper((char*) wlhead, sizeof(*wlhead), chunk);
}

// Variant of the above but for the wlheader32_t type.
void log_wlheader32_and_previous_data(const wlheader32_t* wlhead, const char* chunk)
{
    log_wlheader_and_previous_data_helper((char*) wlhead, sizeof(*wlhead), chunk);
}

/****************************************************************************
 * traverse_chunk -- steps thru chunk by entry
 ***************************************************************************/
int
traverse_chunk(char *chunk, int *length, EntryFunc *func)
{
    group_t *group = mysys->group;
    wlheader_t *wlhead;
    headpack_t header[1];
    ackpack_t ack[1];
    char path[MAXPATHLEN];
    char *dataptr;
    char *jrnname;
    u_longlong_t ret;
    u_longlong_t offset;
    u_longlong_t lock_len;
    longlong_t unlock_len;
    int chunkoff = 0;
    int entrylen;
    int bytesleft;
    int jrn_locked;
    int jrnpath_len;
    int lgnum = cfgpathtonum(mysys->configpath);
    int jrncnt;
    int pcnt;
    int len;
    int i;
    int rc = 0;
    u_longlong_t unlock_offset;
    sddisk_t *sd;
    char devname[MAXPATH];
    char mountp[MAXPATH];
    int mirror_mounted;
    char pmark[MAXPATH];
    struct stat statbuf;
    int chunkparts = 0;
    wciov_t wciov[32];
    wc_t wc;
    // WI_338550 December 2017, implementing RPO / RTT
    chunk_timestamp_queue_entry_t   QueueEntry = {LG_CONSISTENCY_POINT_TIMESTAMP_INVALID, LG_CONSISTENCY_POINT_TIMESTAMP_INVALID};

    
    init_wc( &wc, wciov, 32 );
    /* TODO clean up usage of func and wc_func */
    wc.wc_func = func;
top:
    jrn_locked = 0;
    unlock_offset = 0;

    if (func == write_journal
    && group->jrnfd >= 0
    && _rmd_jrn_mode == JRN_AND_MIR) {
/*
lock from correct EOF
        offset = llseek(group->jrnfd, (offset_t)1, SEEK_END);
        _rmd_jrn_size = offset-1;
*/
        offset = llseek(group->jrnfd, (offset_t)0, SEEK_END);
        _rmd_jrn_size = offset;

        lock_len = *length;

        rc = lock_journal(offset, lock_len);
        if (rc == -1) {
            reporterr(ERRFAC, M_JRNLOCK, ERRCRIT,
                group->journal, argv0);
            return -1;
        } else if (rc == 1) {
            close_journal(group);
            /* re-open mirrors */
            closedevs(O_RDWR | O_EXCL);
            func = write_mirror;
	    wc.wc_func = func;
            _rmd_jrn_state = JRN_CO;        
            _rmd_jrn_mode = MIR_ONLY;        
        } else {
           jrn_locked = 1;
           unlock_offset = offset + lock_len;
       }
    }

    while (1) {
        if (chunkoff >= *length) {
            rc = 0;
            break;
        }
        /* only have a partial header left in chunk */
        if ((*length-chunkoff) <= sizeof(wlheader_t)) {
            *length = chunkoff;
            rc = 0;
            break;
        }
	chunkparts++;
        wlhead = (wlheader_t*)(chunk+chunkoff);
        ftd_trace_flow(FTD_DBG_FLOW8,
                "%s - trav_chunk: wlhead->majicnum = %08llx\n",
                argv0, wlhead->majicnum);
        ftd_trace_flow(FTD_DBG_FLOW8,
                "%s - trav_chunk: wlhead->offset = %lld\n",
                argv0, wlhead->offset);
        ftd_trace_flow(FTD_DBG_FLOW8,
                "%s - trav_chunk: wlhead->length = %u\n",
                argv0, wlhead->length);
        entrylen = wlhead->length << DEV_BSHIFT;
        bytesleft = *length-chunkoff-sizeof(wlheader_t);

        /* if partial-entry then adjust chunk size and return */
        if (bytesleft && bytesleft < entrylen) {
            *length = chunkoff;
            rc = 0;
            break;
        }
        if (wlhead->majicnum != DATASTAR_MAJIC_NUM) {
            reporterr (ERRFAC, M_BADHDR, ERRCRIT, argv0,
                                                  wlhead->majicnum,
                                                  wlhead->dev,
                                                  wlhead->diskdev);

            log_wlheader_and_previous_data(wlhead, chunk);
            
            if (ISPRIMARY(mysys)) {
                EXIT(EXITANDDIE);
            } else {
		flush_coalesce( &wc ); /* write pent up buffers before quiting */
                geterrmsg(ERRFAC, M_BADHDR, fmt);
                sprintf(msg, fmt, argv0, wlhead->majicnum, wlhead->dev, wlhead->diskdev);
                logerrmsg(ERRFAC, ERRCRIT, M_BADHDR, msg);
                senderr(mysys->sock, header, 0, ERRCRIT, M_BADHDR, msg);
		free_wc( &wc );
                return -1;
            }
        }
        dataptr = ((char*)wlhead+sizeof(*wlhead));

        /* look for special entries */
        if (wlhead->offset == (ftd_uint64_t)-1 && 
            !memcmp(dataptr, MSG_INCO, strlen(MSG_INCO))) {
            /*
             * PMD sends MSG_INCO marker when starting smart refresh.
             * RMD avoid journaling when JLESS is on.
             */
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "[%s] got MSG_INCO = %s\n", argv0, MSG_INCO);
            /* start journaling */
            dataptr += strlen(MSG_INCO)+1;
            if (ISSECONDARY(mysys)) {
		flush_coalesce( &wc ); /* write pent up buffers before state change */
                if (GET_LG_JLESS(mysys)) {
                    ftd_trace_flow(FTD_DBG_FLOW1,
                        "ignore MSG_INCO due to JLESS is on.\n");
                } else {
                    /* refresh */
                    iovlen = 0;
                    iov = NULL;
                    // Now that we have smarter smart refreshes, we do not want to delete existing .i journal entries,
                    // as their content will not be sent again.
                    if ((group->jrnfd = new_journal(lgnum, JRN_INCO, 0)) == -1) {
			free_wc( &wc );
                        return -1;
                    }
                    if ((lock_len = *length-chunkoff-entrylen-sizeof(wlheader_t)) > 0) {
                        if (lock_journal(sizeof(jrnheader_t), lock_len) == -1) {
                            reporterr(ERRFAC, M_JRNLOCK, ERRCRIT,
                                    group->journal, argv0);
			    free_wc( &wc );
                            return -1;
                        }
                        jrn_locked = 1;
                        unlock_offset = sizeof(jrnheader_t) + lock_len;
                    }
                    closedevs(O_RDONLY);
                    func = write_journal;
		    wc.wc_func = func;
                    _rmd_state = FTDRFD;
                    _rmd_rfddone = 0;
                }
            }
        } else if (wlhead->offset == (ftd_uint64_t)-1 && 
            !memcmp(dataptr, MSG_CO, strlen(MSG_CO))) {
            /*
             * PMD sends MSG_CO marker when ending smart refresh.
             * RMD skips handling journals if JLESS is on.
             */
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "[%s] got MSG_CO = %s\n", argv0, MSG_CO);
            dataptr += strlen(MSG_CO)+1;
            if (ISSECONDARY(mysys)) {
		flush_coalesce( &wc ); /* write pent up buffers before state change */
                if (_rmd_state == FTDRFD) {
                    /* free global iov structure */
                    if (iov) {
                        iovlen = 0;
                        free(iov);
                        iov = NULL;
                    }
                    _rmd_rfddone = 1;

                    if (!GET_LG_JLESS(mysys)) {
                        close_journal(group);
                        rename_journals(JRN_CO);
                        if ((group->jrnfd = new_journal(lgnum, JRN_CO, 1)) == -1) {
			    free_wc( &wc );
                            return -1;
                        }
                        if ((lock_len = *length-chunkoff-entrylen-sizeof(wlheader_t)) > 0) {
                            if (lock_journal(sizeof(jrnheader_t), lock_len) == -1) {
                                reporterr(ERRFAC, M_JRNLOCK, ERRCRIT,
                                    group->journal, argv0);
				free_wc( &wc );
                                return -1;
                            }
                            jrn_locked = 1;
                            unlock_offset = sizeof(jrnheader_t) + lock_len;
                        }
                        if (!_rmd_cpon) {
                            /* only apply if we are NOT in checkpoint */
                            closedevs(O_RDONLY);
                            start_rmdapply(0);
                            _rmd_jrn_mode = JRN_AND_MIR;
                        }
                    }
                }
                _rmd_jrn_state = JRN_CO;
                _rmd_state = FTDPMD;
            }
        } else if (wlhead->offset == (ftd_uint64_t)-1 &&
            !memcmp(dataptr, MSG_CPON, strlen(MSG_CPON))) {
            /*
             * PMD sends MSG_CPON marker when starting checkpoint mode.
             * RMD noop if JLESS is on.
             */
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "[%s] got MSG_CPON = %s\n", argv0,MSG_CPON);
            dataptr += strlen(MSG_CPON)+1;
            if (ISSECONDARY(mysys)) {
		flush_coalesce( &wc ); /* write pent up buffers before state change */

                if (GET_LG_JLESS(mysys)) {
                    ftd_trace_flow(FTD_DBG_FLOW1,
                        "MSG_CPON: JLESS is on; "
                        "_rmd_cppend: %d -> 1; _rmd_cpstart: %d -> 0;\n");
                    _rmd_cppend = 1;
                    _rmd_cpstart = 0;

                    /* create stub .p file */
                    if ((rc = new_journal(lgnum, JRN_CP, 1)) == -1) {
			free_wc( &wc );
                        return -1;
                    }
                } else {
                    /* close mirror devices */
                     closedevs(-1);
                    /* checkpoint-on */

                    /* create a checkpoint file */
                    if ((rc = new_journal(lgnum, JRN_CP, 1)) == -1) {
			free_wc( &wc );
                        return -1;
                    } else if (rc == 0) {
                        /* create a new journal file to write to during apply */
                        /* We should not ever be in JRN_INCO mode when checkpoint is begun, but in any case, we make sure
                           any existing journal will not be clobbered. */
                        group->jrnfd = new_journal(lgnum, _rmd_jrn_state, 0);
                        if (group->jrnfd == -1) {
			    free_wc( &wc );
                            return -1;
                        }
                        /* apply coherent data up to cp - wait for completion */
                        start_rmdapply(1);
                    }
                    func = write_journal;
		    wc.wc_func = func;

                    ftd_trace_flow(FTD_DBG_FLOW1,
                            "_rmd_cppend: %d -> 1; _rmd_cpstart: %d -> 0\n",
                            _rmd_cppend, _rmd_cpstart);
                    _rmd_cppend = 1;
                    _rmd_cpstart = 0;
                    _rmd_cpon = 0;
                    _rmd_jrn_mode = JRN_AND_MIR;

                    if ((lock_len = *length-chunkoff-entrylen-sizeof(wlheader_t)) > 0) {
                        if (lock_journal(sizeof(jrnheader_t), lock_len) == -1) {
                            reporterr(ERRFAC, M_JRNLOCK, ERRCRIT,
                                group->journal, argv0);
			    free_wc( &wc );
                            return -1;
                        }
                        jrn_locked = 1;
                        unlock_offset = sizeof(jrnheader_t) + lock_len;
                    }
                }
            } else {
                /* primary system */
            }
        } else if (wlhead->offset == (ftd_uint64_t)-1 &&
            !memcmp(dataptr, MSG_CPOFF, strlen(MSG_CPOFF))) {
            /*
             * PMD sends MSG_CPOFF marker when ending checkpoint mode.
             * RMD assumes no journal files present when recieving MSG_CPOFF.
             */
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "[%s] got MSG_CPOFF = %s\n", argv0, MSG_CPOFF);
            syslog(LOG_WARNING,
                    "[%s] got MSG_CPOFF = %s\n", argv0, MSG_CPOFF);
            dataptr += strlen(MSG_CPOFF)+1;
            if (ISSECONDARY(mysys)) {
		flush_coalesce( &wc ); /* write pent up buffers before state change */
                if (exec_cp_cmd(lgnum, PRE_CP_OFF, 0) == 0) {
                    /* Check mirror device */
                    mirror_mounted = 0;
                    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
                        force_dsk_or_rdsk(devname, sd->mirname, 0);
                        if (dev_mounted(devname, mountp)) {
                            mirror_mounted = 1;
                            break;
                        }
                    }
                    if (!mirror_mounted) {
/*
                        closedevs(-1);
                        rc = verify_rmd_entries(group, mysys->configpath, 1);
*/
                        rc = 0;
                        if (rc == 0) {
                            /* kill RMDA if it is running, we will be restarting it */
                            stop_rmdapply();

                            jrncnt = get_journals(jrnphp, 0, 0);
                            for (i = 0; i < jrncnt; i++) {
				jrnname = jrnpath_get(jrnphp, i);
                                if (cp_file(jrnname, CP_ON)) {
                                    sprintf(path, "%s/%s",
                                            group->journal_path, jrnname);
                                    clobber_journal(NULL, path);
                                }
                            }

                            if (GET_LG_JLESS(mysys)) {

                                /* we need read access to the mirrors */
                                closedevs(O_RDWR | O_EXCL);

                                /*
                                 * delete stub .p file
                                 */
                                sprintf(pmark, "%s/" FTD_JRNLGNUM_PREFIX_FMT "." FTD_JRNNUM_FMT ".p",
                                        mysys->group->journal_path,
                                        lgnum, _rmd_jrn_cp_lnum);

                                UNLINK(pmark);

                                // We'll still apply any remaining journal, as it's possible that journals accumulated in checkpoint
                                // before we went into journal less.
                                start_rmdapply(1);
                                
                                _rmd_cpon = 0;
                                _rmd_cpstop = 0;
                                _rmd_jrn_cp_lnum = -1;
                                _rmd_jrn_mode = MIR_ONLY;
                            } else {

                                /* we need read access to the mirrors */
                                closedevs(O_RDONLY);

                                /* create a CO journal to write to while applying */
                                if ((group->jrnfd =
                                     new_journal(lgnum, JRN_CO, 1)) == -1) {
				    free_wc (&wc );
                                    return -1;
                                }

                                start_rmdapply(0);

                                _rmd_cpon = 0;
                                _rmd_cpstop = 0;
                                _rmd_jrn_cp_lnum = -1;
                                _rmd_jrn_mode = JRN_AND_MIR;
                            }

                            /* out of checkpoint mode */

                            reporterr(ERRFAC, M_CPOFF, ERRINFO, argv0);
                            /* send same msg to PMD */
                            memset(header, 0, sizeof(headpack_t));
                            ack->data = ACKCPOFF;
                            sendack(mysys->sock, header, ack);
                        }
                    } else {
                        send_cp_err_ack(ACKCPOFFERR);
                        reporterr(ERRFAC, M_CPOFFDEV, ERRWARN, argv0, devname, mountp);
                        _rmd_cpstop = 0;
                    }
                } else {
                    send_cp_err_ack(ACKCPOFFERR);
                    reporterr(ERRFAC, M_CPOFFERR, ERRWARN, argv0);
                    _rmd_cpstop = 0;
                }

                if (!GET_LG_JLESS(mysys)) {
                    if ((lock_len = *length-chunkoff-entrylen-sizeof(wlheader_t)) > 0) {
                        if (lock_journal(sizeof(jrnheader_t), lock_len) == -1) {
                            reporterr(ERRFAC, M_JRNLOCK, ERRCRIT,
                                group->journal, argv0);
			    free_wc( &wc );
                            return -1;
                        }
                        jrn_locked = 1;
                        unlock_offset = sizeof(jrnheader_t) + lock_len;
                    }
                }
            }
        } else {
            if (GET_LG_JLESS(mysys) && (_rmd_cppend || _rmd_cpon)) {
                    *length = chunkoff;
                    rc = 0;
                    break;
            }

            if (ISSECONDARY(mysys)) 
	    {
		rc = write_coalesce( &wc, wlhead);
	    }
	    else
	    {
		rc = func((void*)wlhead, dataptr);
        // WI_338550 December 2017, implementing RPO / RTT
        if (wlhead->timestamp > 0)
            {
            if (QueueEntry.OldestChunkTimestamp == LG_CONSISTENCY_POINT_TIMESTAMP_INVALID)
                {
                QueueEntry.OldestChunkTimestamp = wlhead->timestamp;
                }
            else
                {
                QueueEntry.OldestChunkTimestamp = (wlhead->timestamp < QueueEntry.OldestChunkTimestamp ? wlhead->timestamp : QueueEntry.OldestChunkTimestamp);
                }
            if (QueueEntry.NewestChunkTimestamp == LG_CONSISTENCY_POINT_TIMESTAMP_INVALID)
                {
                QueueEntry.NewestChunkTimestamp = wlhead->timestamp;
                }
            else
                {
                QueueEntry.NewestChunkTimestamp = (wlhead->timestamp > QueueEntry.NewestChunkTimestamp ? wlhead->timestamp : QueueEntry.NewestChunkTimestamp);
                }
            }
	    }
        if (rc == 1) {
/*
this return was not used
use it for journal file rollover
                rc = 1;
*/
		    if (ISSECONDARY(mysys)) 
		    {
			flush_coalesce( &wc ); /* flush pent up buffers before file rollover */
		    }
		    goto top;
            }
        }
        chunkoff += sizeof(wlheader_t)+entrylen;
    }
    // WI_338550 December 2017, implementing RPO / RTT
    // Add the chunk time to the queue
    if (ISPRIMARY(mysys))
    {
        // It is possible that we did not process any IO (in case of a sentinel)
        // In this case it is important to queue something because we always dequeue on ack
        // We push an uninitialized time value, which will tell us to disregard that timestamp
        if (*length > 0)
        {
            Chunk_Timestamp_Queue_Push(group->pChunkTimeQueue, &QueueEntry);
        }
    }
    if (ISSECONDARY(mysys)) {
	flush_coalesce( &wc ); /* write any pent up buffers */
    }
    free_wc( &wc );
    /* make sure chunk gets sync'd to disk before ACKing */
    if (jrn_locked) {
        fsync(group->jrnfd);
/*
use correct "locked" offset - not EOF
        offset = llseek(group->jrnfd, (offset_t)1, SEEK_END);
        if (offset == -1) {
            reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
                argv0, group->journal, offset, strerror(errno));
            return -1;
        }
        unlock_len = (offset-1);
*/
		unlock_len = (~unlock_offset)+1;
        rc = unlock_journal(unlock_offset, unlock_len);
        if (rc == -1) {
            reporterr(ERRFAC, M_JRNUNLOCK, ERRCRIT,
                group->journal, argv0, strerror(errno));
            return -1;
        }
    }
    return(rc);
} /* traverse_chunk */

/****************************************************************************
 * spin -- update internal state, stats until interupted
 ***************************************************************************/
static void
spin (int lgnum)
{
    fd_set read_set[1];
    fd_set read_set_copy[1];
    time_t currts, lastts;
    struct timeval seltime[1];
    int deltatime;
    int nselect;
    int rc;

    currts = lastts = 0;
    seltime->tv_usec = 0;
    seltime->tv_sec = 1;
    nselect = _pmd_rpipefd+1;

    FD_ZERO(read_set);
    FD_SET(_pmd_rpipefd, read_set);

    while (1) {
        *read_set_copy = *read_set;
        rc = select(nselect, read_set_copy, NULL, NULL, seltime);
        if (rc > 0) {
            if (FD_ISSET(_pmd_rpipefd, read_set_copy)) {
                sig_action(lgnum);
            }
        }
        if (_pmd_state_change) {
            break;
        }
        /* time to update internal state ? */
        time(&currts);
        deltatime = currts-lastts;
        if (deltatime >= 10) {
            gettunables(mysys->group->devname, 0, 0);
            savestats(0, _net_bandwidth_analysis);
            lastts = currts;
        }
#if defined(linux)
        seltime->tv_usec = 0;
        seltime->tv_sec = 1;
#endif /* defined(linux) */
    }
} /* spin */

/****************************************************************************
 * flush_net -- process all the ACKs on the wire
 ***************************************************************************/
int
flush_net (int fd)
{
    int i;
    int connect_timeo = 5;
    int count_loops = 0;
    int save_errno = 0;
    int count_EALREADY = 0;

    /* tell RMD that we have been interrupted */
    if (sendhup() == -1) {
        return -1;
    }
    g_ackhup = 0;
    for (;;) {
        process_acks();
        if (g_ackhup) {
            break;
        }
        /* sleep and try again; note: if you change the sleep time, adjust the count_loops modulo below */
        usleep(100000);
        /* Check if channel is still up every 2 minutes approximately (WR 44057) */
        ++count_loops;
        if( (count_loops > 1) && ((count_loops % 1200) == 0) )
        {
          if (net_test_channel(connect_timeo) <= 0)
          {
             save_errno = errno;
             // WR PROD00003444: if net_test_channel status == EALREADY (operation in progress)
             // extend the retry period
             if( save_errno == EALREADY )
             {
                if(++count_EALREADY >= 5)
                {
                    reporterr (ERRFAC, M_MAXEALREADY, ERRWARN, mysys->name, othersys->name);
                    EXIT(EXITNETWORK);
                }
             }
             else
             {
                reporterr (ERRFAC, M_NETTIMEO, ERRWARN, mysys->name, othersys->name);
                ftd_trace_flow(FTD_DBG_FLOW1, "net_test_channel status (errno) = %d\n", save_errno);
                EXIT(EXITNETWORK);
             }
          }
        }
    }
    mysys->group->offset = 0;
    mysys->group->size = 0;

#ifdef TDMF_TRACE
    if(count_EALREADY)
    {
        // Information trace for debug mode only
        ftd_trace_flow(FTD_DBG_FLOW1, "flush_net(): got %d times EALREADY status and finally succeeded\n", count_EALREADY);
    }
#endif

    return 0;
} /* flush_net */

/****************************************************************************
 * getchunk -- gets a block of entries from the journal
 ***************************************************************************/
int
getchunk (group_t *group)
{
  char ps_name[MAXPATHLEN];
  int lgnum;
  ftd_dev_t_t pass_lgnum;
  int rc,count=0;
  /* add for WR15599 01/28/2003 */
  static int prior_pmd_state = 0;
  static int first_time_through = 1;
  /******************************/

  oldest_entries_t oe[1];

  rc = 0;
  lgnum = cfgpathtonum(mysys->configpath);
  pass_lgnum = lgnum;

  group->data = NULL;
  group->size = 0;
  if (group->chunk) {
    group->endofdata = group->chunk - 1;
  }

  memset(oe, 0, sizeof(oldest_entries_t));

  if (mysys->tunables.chunksize > group->chunkdatasize) {
    if (group->chunk) {
      ftdfree(group->chunk);
    }
    group->chunk = (char*)ftdmalloc(mysys->tunables.chunksize);
    group->chunkdatasize = mysys->tunables.chunksize;
  }
  oe->addr = (ftd_uint64ptr_t)(unsigned long)group->chunk;
  oe->offset32 = group->offset;
  oe->len32 = group->chunkdatasize / sizeof(int);

  if (0 > (rc = FTD_IOCTL_CALL(group->devfd, FTD_OLDEST_ENTRIES, oe))) {
    if (errno == EINVAL) {
      /* no more journal to read */
      ftd_trace_flow(FTD_DBG_FLOW1, "no more to read\n");
      return 0;
    }
    reporterr (ERRFAC, M_BLKGET, ERRCRIT, group->devname, errno);
    ftdfree(group->chunk);
	group->chunk = NULL;
    group->chunkdatasize = 0;
    return -1;
  }
  /* add for WR15599 01/28/2003 */
  if (first_time_through) {
      if (oe->state != FTD_MODE_SYNCTIMEO) {	/* for Dynamic Mode Change */
          prior_pmd_state = oe->state;
      }
    first_time_through = 0;
  }

  ftd_trace_flow(FTD_DBG_FLOW1, "prior state = %d, oe->state = %d\n",
          prior_pmd_state, oe->state);

  /******************************/
  /* look for a state transition */
  if (oe->state != prior_pmd_state) {  /* add for WR15599 01/28/2003 */
    switch(oe->state) {
    case FTD_MODE_PASSTHRU:
      prior_pmd_state = oe->state;     /* add for WR15599 01/28/2003 */
      reporterr (ERRFAC, M_PASSTHRU, ERRCRIT, group->devname);
      EXIT(EXITANDDIE);
    case FTD_MODE_NORMAL:
      prior_pmd_state = oe->state;	/* add for WR15599 01/28/2003 */
      break;
    case FTD_MODE_REFRESH:
      prior_pmd_state = oe->state;	/* add for WR15599 01/28/2003 */
      break;
    case FTD_MODE_TRACKING:

      if (_pmd_state == FTDRFD && refresh_started)
      {
          // If we detect an overflow here while we're in refresh, we'll clean up the bitmaps according to what has already been done.
          // To detect a refresh, it's important to rely on the state of the PMD and not the prior_pmd_state, as this could be pretty much anything
          // since it only records what the state was the last time getchunk() was called.
          if (ftd_lg_merge_hrdbs(group, lgnum) != 0)
          {
              reporterr(ERRFAC, M_MERGE_FAILED, ERRWARN);
          }
      }
      prior_pmd_state = oe->state;	/* add for WR15599 01/28/2003 */
      reporterr(ERRFAC, M_BABOFLOW, ERRWARN, argv0);
      _pmd_state = FTDRFD;
      _pmd_state_change = 1;
      _pmd_refresh = 1;
      ++BAB_oflow_counter;     /* WR 43926 */

      // WI_338550 December 2017, implementing RPO / RTT
      // Invalidate RTT sequencer tag (as is done on Windows)
      SequencerTag_Invalidate(&group->RTTComputingControl.iSequencerTag);

       /* RFW does this here, but into the RFX ugly way */
      while ((FTD_IOCTL_CALL(mysys->ctlfd, FTD_CLEAR_BAB, &pass_lgnum) == EAGAIN) &&  (count < 1)) 
      {
         sleep(10);
         count++;
      }
    // WI_338550 December 2017, implementing RPO / RTT
    // Clear RPO queue
    Chunk_Timestamp_Queue_Clear(group->pChunkTimeQueue);

      /* state change needed, leave BAB data intact */
      rc = -2;
      goto getchunkabort;
      /* break; put break back in if goto removed  */
    case FTD_MODE_SYNCTIMEO:	/* for Dynamic Mode change */
      if (prior_pmd_state != FTD_MODE_TRACKING) {
          reporterr(ERRFAC, M_SYNCTIMEO, ERRWARN, argv0);
          oe->state = FTD_MODE_TRACKING;
          prior_pmd_state = oe->state;	/* add for WR15599 01/28/2003 */
          _pmd_state = FTDRFD;
          _pmd_state_change = 1;
          /* state change needed, leave BAB data intact */
          rc = -2;
          goto getchunkabort;
      }
      break;
    case FTD_MODE_CHECKPOINT_JLESS:
    default:
      prior_pmd_state = oe->state;	/* add for WR15599 01/28/2003 */
      break;
    }
  }					/* add for WR15599 01/28/2003 */
  group->size = oe->retlen32*sizeof(int); /* bytes */
  group->data = group->chunk;
  group->endofdata = group->chunk+group->size-1;

  ftd_trace_flow(FTD_DBG_FLOW1, "prior state = %d, oe->state = %d, len = %d\n",
          prior_pmd_state, oe->state, oe->retlen32);

  if (oe->retlen32 == 0) {
    return 0;
  }
  return rc;

getchunkabort:
  ftd_trace_flow(FTD_DBG_FLOW15, "abort, prior state = %d, oe->state = %d\n",
          prior_pmd_state, oe->state);
  if (group->chunk) {
      ftdfree(group->chunk);
      group->chunk = NULL;
      group->chunkdatasize = 0;
      group->data = NULL;
      group->size = 0;
  }
  return rc;

} /* getchunk */

/****************************************************************************
 * bab_has_entries -- check BAB usage
 ***************************************************************************/
int
bab_has_entries(int lgnum, int *used, int *free)
{
    stat_buffer_t sb;
    ftd_stat_t info;
    int rc;

    sb.lg_num = lgnum;
    sb.dev_num = 0;
    sb.len = sizeof(info);
    sb.addr = (ftd_uint64ptr_t)(unsigned long)&info;

    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUP_STATS, &sb);
    if (rc != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT,
            "GET_GROUP_STATS", strerror(errno));
        return -1;
    }
    *used = info.bab_used;
    *free = info.bab_free;

    if (info.wlentries > 0
    && info.wlsectors > 0) {
        return 1;
    }

    return 0;
} /* bab_has_entries */

/****************************************************************************
 * sync_fs -- sync filesystems mounted on ftd devices
 * Note: the comment above was the intent but it uses a global system sync()
 *       call, which sync all on the system, not only ftd devices; and this
 *       was done for every detected mounted dtc device;
 *       on SuSE 11, this caused a delay of close to 5 minutes to transition
 *       to checkpoint state with 20 groups of 5 devices each;
 *       WR3224: change this to call sync() only once per group (not per dev)
 *               if this group has at least one mounted dtcdevice . 
 ***************************************************************************/
int
sync_fs (void)
{
    sddisk_t *sd;
    char     mountp[MAXPATHLEN];
    DIR      *fd = NULL;
    int      do_sync = 0;

    for (sd = mysys->group->headsddisk; sd; sd = sd->n)
    {
        // Note: in the case of Linux, we should probably sync even if the
        //       dtc devices have just been unmounted (to verify)
        if (dev_mounted(sd->sddevname, mountp))
        {
            // Found a dtc device registered as mounted
            // Verify if the registered mount point exists (legacy code)
            if ((fd = opendir(mountp)) == NULL)
            {
                reporterr(ERRFAC, M_FILE, ERRWARN, mountp, strerror(errno));
                continue;
            }
            // dtc device mounted; set the sync flag
            do_sync = 1;
            closedir(fd);
            // WARNING: since this is currently a per-group sync, no use continuing
            //          if we found one mounted device; we will sync. BUT if you plan
            //          to implement a per-device sync (fsync() for instance), take out
            //          this break.
            break;
        }  // ...if dev_mounted
    }  // ...for

    if(do_sync)
    {
        // At least one mounted dtc device; call sync
        (void)sync();
    }

    return 0;
} /* sync_fs */

int
set_drv_state_checkpoint_jless (int lgnum)
{
    ftd_state_t stb;
    int         saved_state;
    int         rc;

    saved_state = get_driver_mode(lgnum);
    stb.lg_num = lgnum;
    stb.state = FTD_MODE_CHECKPOINT_JLESS;
    ftd_trace_flow(FTD_DBG_FLOW1,
            "set lg(%d) state: %d -> %d\n",
            lgnum, saved_state, FTD_MODE_CHECKPOINT_JLESS);
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
    if (rc != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT,
                "SET_GROUP_STATE", strerror(errno));
        return -1;
    }

    return 0;
}

extern int chk_baboverflow(group_t *group);
extern statestr_ts statestr[];
int syncinterval;
ftd_stat_t lgstat;

/**
 * @brief Initiates the bitmap refresh mechanism.
 *
 * This implies:
 *
 * -) Resetting the group's iMigratedBABCtr.
 *
 * And if the group's state is in normal mode:
 *
 * -) Turning on the historical bitmap mechanism of the driver.
 * -) Sending the clear bits command to the RMD.
 *
 * @param lgnum   The group number.
 * @param group   The group's information.
 *
 * @return 0 on success or the return code of the FTD_BACKUP_HISTORY_BITS ioctl on error.
 *
 */
int initiate_bitmap_refresh(int lgnum, group_t* group)
{
    int rc = 0;

    group->iMigratedBABCtr = 1;
    
    if (_pmd_state == FTDPMD) /* When in normal mode */
    {
        static const ftd_int32_t backup_mode = TRUE; /*starting transition from historical (.map)	--> future (.mapnext) */
        
        if ((rc = FTD_IOCTL_CALL(group->devfd, FTD_BACKUP_HISTORY_BITS, &backup_mode)) < 0)
        {
            reporterr(ERRFAC, M_GENMSG, ERRCRIT, get_error_str(errno));
            return rc;
        }
        /* Send clear bit token */
        sendclearbits();
        ftd_trace_flow(FTD_DBG_FLOW16, "PMD [%d], sending token to clearsbits!!!", lgnum );
    }
    
    return rc;
}

/****************************************************************************
 * dispatch -- run forever and send chunks to remote
 ***************************************************************************/
int
dispatch (void)
{
    fd_set read_set[1];
    fd_set read_set_copy[1];
    group_t *group;
    time_t currts, lastts, lastlrdb;
    long timeslept = 0;
    long io_sleep;
    int lgnum;
    int deltatime;
    int nselect;
    int rc;
    int byteoffset;
    static int bab_percent_full_threshold = 0;
    float bab_percent_full;
    ftd_state_t stb;
    long loopcnt = 0;
    long loopcnt_nodata = 0;
    stat_buffer_t statBuf;

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    FD_ZERO(read_set);
    FD_SET(group->devfd, read_set);
    FD_SET(_pmd_rpipefd, read_set);

    if (_pmd_state != FTDPMD) {
        /* refresh - just want to return immediately */
        io_sleep = 0;
    } else {
        io_sleep = 1;
    }
    skosh.tv_sec = io_sleep;
    skosh.tv_usec = 0;

    nselect = group->devfd > _pmd_rpipefd ? group->devfd: _pmd_rpipefd;
    nselect++;

    lastts = lastlrdb = 0;

    while (1) {
        if (_pmd_state_change) {
            ftd_trace_flow(FTD_DBG_FLOW12,
                    "_pmd_state_change, current _pmd_state %d, %s\n",
                    _pmd_state, statestr[_pmd_state].str);
            return 0;
        }

        if (_pmd_cpstart == 1 || _pmd_cpstart == 2) {
            if (_pmd_state == FTDRFD || _pmd_state == FTDRFDF) {
                /* for now don't let this happen */
                reporterr(ERRFAC, M_RFDCPON, ERRWARN, argv0);
                /* if request came from RMD then send err */
                if (_pmd_cpstart == 2) {
                    send_cp_err(CMDCPONERR);
                }
                _pmd_cpstart = 0;
            } else if (_pmd_state == FTDBFD) {
                /* for now don't let this happen */
                reporterr(ERRFAC, M_BFDCPON, ERRWARN, argv0);
                /* if request came from RMD then send err */
                if (_pmd_cpstart == 2) {
                    send_cp_err(CMDCPONERR);
                }
                _pmd_cpstart = 0;
            } else {
                /*
                 * _pmd_state == FTDPMD
                 */
                /* this must succeed ! - DO NOT transition into cp on failure */
                if (exec_cp_cmd(lgnum, PRE_CP_ON, 1) == 0) {
                    /* sync filesystems */
                    sync_fs();
                    send_cp_on(lgnum);
                    exec_cp_cmd(lgnum, POST_CP_ON, 1);

                    ftd_trace_flow(FTD_DBG_FLOW1,
                            "lg(%d) _pmd_cpstart from %d to 3\n",
                            lgnum, _pmd_cpstart);

                    _pmd_cpstart = 3;
                } else {
                    reporterr(ERRFAC, M_CPONERR, ERRWARN, argv0);
                    /* if request came from RMD then send err */
                    if (_pmd_cpstart == 2) {
                        send_cp_err(CMDCPONERR);
                    }

                    ftd_trace_flow(FTD_DBG_FLOW1,
                            "lg(%d) _pmd_cpstart from %d to 0\n",
                            lgnum, _pmd_cpstart);

                    _pmd_cpstart = 0;
                }
            }
            /* ack the ftdcheckpoint command process */
            ipcsend_ack(_pmd_wpipefd);
        } else if (_pmd_cpstop == 1 || _pmd_cpstop == 2) {
            if (_pmd_state == FTDRFD || _pmd_state == FTDRFDF) {
                /* for now don't let this happen */
                reporterr(ERRFAC, M_RFDCPOFF, ERRWARN, argv0);
                /* if request came from RMD then send err */
                if (_pmd_cpstop == 2) {
                    send_cp_err(CMDCPOFFERR);
                }
                _pmd_cpstop = 0;
            } else if (_pmd_state == FTDBFD) {
                /* for now don't let this happen */
                reporterr(ERRFAC, M_BFDCPOFF, ERRWARN, argv0);
                /* if request came from RMD then send err */
                if (_pmd_cpstop == 2) {
                    send_cp_err(CMDCPOFFERR);
                }
                _pmd_cpstop = 0;
            } else {
                /*
                 * Checkpoint only can be clear when FTDPMD (Normal)
                 */
                send_cp_off(lgnum);
                ftd_trace_flow(FTD_DBG_FLOW1,
                                "lg(%d) _pmd_cpstop from %d to 3\n",
                                lgnum, _pmd_cpstop);
                _pmd_cpstop = 3;
            }
            /* ack the ftdcheckpoint command process */
            ipcsend_ack(_pmd_wpipefd);
        }
        process_acks();

        /*
         * JLESS: add possible state change after PMD recieves ACKCPOFF
         */
        if (_pmd_state_change) {
            ftd_trace_flow(FTD_DBG_SYNC, "_pmd_state_change - cpoff\n");
            return 0;
        }

        if (mysys->tunables.chunkdelay > 0) {
            usleep(mysys->tunables.chunkdelay*1000);
        }
        /* mod for WR15335 by PST '02/12/12  skosh shuld reset per loop */
        skosh.tv_sec = io_sleep;
        skosh.tv_usec = 0;
        /**********************************/
        *read_set_copy = *read_set;

        /* wait for something to read - wake up periodically */
        rc = select(nselect, read_set_copy, NULL, NULL, &skosh);

        if (rc != 0) {
            ftd_trace_flow(FTD_DBG_FLOW12,
                    "[%s]: select rc = %d, bits: %ld _pmd_state: %d, %s\n", 
                    argv0, rc, read_set_copy[0].fds_bits, 
                    _pmd_state, statestr[_pmd_state].str);
        }

        if (rc == -1) {
           ftd_trace_flow(FTD_DBG_SYNC,
                    "select rc, errno = %d, %d\n",rc,errno);
           if (++loopcnt == 1800) {
               ftd_trace_flow(FTD_DBG_DEFAULT,
                    "LG_%03d: select interrupted errno(%d %s) \n",
                    lgnum, errno, strerror(errno));
               reporterr(ERRFAC, M_DRVERR, ERRCRIT,
                   "select interrupted", strerror(errno));
               loopcnt = 0;
           }

           if (errno == EAGAIN || errno == EINTR) {
               if (_pmd_state_change) {
                   return 0;
               }
               continue;
           } else {
               reporterr(ERRFAC, M_DRVERR, ERRCRIT,
                   "select", strerror(errno));
               return -1; /* mod by PST '02/12/11 */
               /* break; */
           }
        } else if (rc == 0) {
           if (_pmd_state != FTDPMD) {
               return -1;
           }
           if (timeslept >= max_idle_time) {
               /* send a NOOP to the RMD - every max_idle_time */
               sendnoop();
               timeslept = 0;
           } else {
               timeslept += io_sleep;
           }

           /*
            * WR#37061
            * We saw groups left in tracking mode while PMD is in normal mode.
            * Extra checking codes put here is not the fix to this problem but
            * a cover up. We still need to find the root cause of it.
            */
           if (++loopcnt == 1800) {
                ftd_get_lg_group_stat(lgnum, &statBuf, 0);
                switch (lgstat.state) {
                case FTD_MODE_TRACKING:
                    /* driver in TRACKING mode */
                    reporterr(ERRFAC, M_MODESYNC, ERRCRIT,
                              lgnum, "TRACKING", "FTDPMD");
                    _pmd_state_change = 1;
                    _pmd_state = FTDRFD;
                    return 0;
                default:
                    loopcnt = 0;
                    break;
                }
           }
        } else {
            if (FD_ISSET(_pmd_rpipefd, read_set_copy)) {
                sig_action(lgnum);
            } else if (FD_ISSET(group->devfd, read_set_copy)) {
                if ((_pmd_state == FTDRFD || _pmd_state == FTDRFDF)
                && refresh_started) {
                    /*
                     * Check BAB usage --
                     * if below high-water-mark and above low-water mark
                     * then leave BAB entries in the BAB.
                     * Don't do this before sentinel has migrated
                     *
                     */
                    if (bab_has_entries(lgnum, &group->babused,
                        &group->babfree) < 0)  {
                        return -1;
                    }
                    /* subtract offset since already sent */
/*
                    byteoffset = group->offset*sizeof(int);
*/
                    byteoffset = 0;
                    group->babused -= (byteoffset >> DEV_BSHIFT);
                    if (group->babused < mysys->tunables.chunksize) {
                        /*
                         * leave BAB alone for now
                         */
                        return 0;
                    }
                }

                if ((rc = getchunk(group)) < 0) {
                    ftd_trace_flow(FTD_DBG_FLOW1, "getchunk rc = %d\n", rc);
                    if (rc == -2) {
                        return 0;
                    } else {
                        /* error */
                        return rc;
                    }
                }
                ftd_trace_flow(FTD_DBG_FLOW1,
                        "group->size = %d\n", group->size);

                if (group->size > 0) {
                    /* traverse chunk - get device counts - adjust length */
                    traverse_chunk(group->chunk,
                        (int*)&(group->size), &get_counts);
                    /* if group->size == 0 then we need a bigger chunk */
                    if (group->size == 0) {
                        /* temporarily change chunksize to accomodate */
                        mysys->tunables.chunksize *= 2;
                        continue;
                    }
                    if (!rmd64)
                    {
                        convertwlhfrom64to32 ();
                    }
                    loopcnt_nodata = 0;
                    /* send the data to the remote */
                    rc = sendchunk(mysys->sock, group);
                    ftd_trace_flow(FTD_DBG_SYNC, "sendchunk rc = %d\n", rc);
                    if (rc == -1) {
                        closeconnection();
                        EXIT(EXITRESTART);
                    }
                    /* TBV RFW don't migrate in sync mode */
                    if (mysys->tunables.syncmode == FALSE)
                    {
                        /* draining immediately BAB */
                        migrate(group->size);			/* PB-TODO: VERIFY HERE, POSSIBLE bad ->size!!!!!!! */
         
                        /* Every now and then, we want to reset the dirty bits.
                         * We have to wait until we're sure that we clear bits that have migrated successfully
                         * For this purpose, we keep a backup copy of the bits during that period in the driver */
                        group->iMigratedBABCtr++;

                        if (group->iMigratedBABCtr == CLEAR_BITS_HISTORY)
                        {
                            if((rc = initiate_bitmap_refresh(lgnum, group)) != 0)
                            {
                                return rc;
                            }
                        }                  
                    }
                    else
                    {
                        group->offset += (group->size/sizeof(int));
                    }
                } else {
                    /* no data returned from getchunk */
                    // WI_338550 December 2017, implementing RPO / RTT
                    // If Normal mode and no pending data, update consistency point
                    if( (get_driver_mode(lgnum)) == FTD_MODE_NORMAL )
                    {
                        // >>> iciici Feb7 put a trace here to determine if this is the right place to put this code
                        ftd_lg_set_consistency_point_reached(group);
                        give_RPO_stats_to_driver(group);
                    }
                    /*
                     * WR#37061
                     * We saw groups left in tracking mode while PMD is in
                     * normal mode. Extra checking codes put here is not the
                     * fix to this problem but a cover up. We still need to
                     * find the root cause of it.
                     */
                    if (_pmd_state == FTDPMD) {
                        int pout = 0;
                        if (++loopcnt_nodata == 1800) {
                            /*
                            sprintf(msg, "LG_%03d: no data from getchunk. errno(%d)", lgnum, errno);
                            ftd_trace_flow(FTD_DBG_FLOW1, "%s\n", msg);
                            reporterr(ERRFAC, M_DRVERR, ERRCRIT,
                                      msg, strerror(errno));
                                      */
                            pout = 1;
                        }
                        usleep(10000);	  /* MP: If this delay is removed, we notice that the following logic to force the state
                                           * into a smart refresh kicks in way too frequently. */
                        if (pout == 1) {
                            /*
                            sprintf(msg, "LG_%03d: wakeup from usleep. errno(%d)", lgnum, errno);
                            ftd_trace_flow(FTD_DBG_FLOW1, "%s\n", msg);
                            reporterr(ERRFAC, M_DRVERR, ERRCRIT,
                                      msg, strerror(errno));
                                      */
                            pout = 0;
                            loopcnt_nodata = 0;
                            if (_pmd_cpon == 0) {
                                _pmd_state = FTDRFD;
                                _pmd_state_change = 1;
                                return 0;
                            }
                        }
                        continue;
                    } else if (refresh_started) {
                        return 0;
                    } else {
                        /* if entries in BAB && transitioning to refresh */
                        rc = 0;
                        if (bab_has_entries(lgnum, &group->babused,
                            &group->babfree)) {
                            ftd_trace_flow(FTD_DBG_SYNC, "bab_has_entries\n");
                            usleep(100000);
                            continue;
                        }
                    }
                    return -1;
                }
            } else {
                ftd_trace_flow(FTD_DBG_DEFAULT,
                    "LG_%03d: select waken up with fd not set\n",
                    lgnum);
            }
        }
        /* time to update internal state ? */
        time(&currts);
        deltatime = currts-lastts;
        if (deltatime >= 1) {
            int dbgFlag = ftd_debugFlag;

            ftd_trace_flow(FTD_DBG_FLOW3, "deltatime = %d\n", deltatime);
            ftd_debugFlag = 0;
            gettunables(group->devname, 0, 0);
            ftd_debugFlag = dbgFlag;
            eval_netspeed();
            savestats(0, _net_bandwidth_analysis);
            lastts = currts;
        }
    }

} /* dispatch */

/****************************************************************************
 * migrate -- migrate entries in BAB
 ***************************************************************************/

/* XXX SYNCMODEDIAG + */
int sync_timo_test=0;
/* XXX SYNCMODEDIAG - */

int
migrate(int bytes)
{
    group_t *group;
    migrated_entries_t mig[1];
    int rc;

/* XXX SYNCMODEDIAG + */
{
    if(sync_timo_test)
	sleep(mysys->tunables.syncmodetimeout + 3);
}
/* XXX SYNCMODEDIAG - */

    group = mysys->group;

    (void)time(&group->lastackts);
    if (bytes & 0x7) {                           /* DUMP CORE! */
        *(char*) 0 = 1;
    }
    mig->bytes = bytes;
    rc = FTD_IOCTL_CALL(group->devfd, FTD_MIGRATE, mig);
    if (0 > rc) {
        reporterr (ERRFAC, M_MIGRATE, ERRCRIT,
            group->devname, strerror(errno));
        return -1;
    }
    if (mysys->tunables.syncmode == TRUE)
    {
        group->offset -= (bytes/sizeof(int));
        ftd_trace_flow(FTD_DBG_IO, "%d bytes migrated\n", mig->bytes);
        ftd_trace_flow(FTD_DBG_IO, "new BAB offset = %d\n", group->offset);
    }

    return 1;
} /* migrate */

const char *
get_jrn_mode_str(const int mode)
{
    static char *modestr;
    static char buf[64];
    #undef CASE
    #define CASE(mode) case mode: modestr = #mode; break
    switch (mode) {
    CASE(MIR_ONLY);
    CASE(JRN_ONLY);
    CASE(JRN_AND_MIR);
    default:
        modestr = (snprintf(buf, sizeof(buf), "JRN_MODE_%d", mode), buf);
    }
    return modestr;
}

const char *
get_jrn_state_str(const int state)
{
    static char *statestr;
    static char buf[64];
    #undef CASE
    #define CASE(state) case state: statestr = #state; break
    switch (state) {
    CASE(JRN_CO);
    CASE(JRN_INCO);
    CASE(JRN_CP);
    default:
        statestr = (snprintf(buf, sizeof(buf), "JRN_STATE_%d", state), buf);
    }
    return statestr;
}

const char *
get_cmd_str(const int cmd)
{
    static char *cmdstr;
    static char buf[64];
    #undef CASE
    #define CASE(cmd) case cmd: cmdstr = #cmd; break
    switch (cmd) {
    CASE(CMDHANDSHAKE);
    CASE(CMDCHKCONFIG);
    CASE(CMDNOOP);
    CASE(CMDVERSION);
    CASE(CMDWRITE);
    CASE(CMDHUP);
    CASE(CMDCPONERR);
    CASE(CMDCPOFFERR);
    CASE(CMDEXIT);
    CASE(CMDBFDSTART);
    CASE(CMDBFDDEVS);
    CASE(CMDBFDDEVE);
    CASE(CMDBFDREAD);
    CASE(CMDBFDEND);
    CASE(CMDBFDLGE);
    CASE(CMDBFDCHKSUM);
    CASE(CMDRFDFSTART);
    CASE(CMDRFDDEVS);
    CASE(CMDRFDDEVE);
    CASE(CMDRFDFWRITE);
    CASE(CMDRFDEND);
    CASE(CMDRFDFEND);
    CASE(CMDRFDCHKSUM);
    CASE(CMDMSGINCO);
    CASE(CMDMSGCO);
    default:
        cmdstr = (snprintf(buf, sizeof(buf), "CMD_%d", cmd), buf);
    }
    return cmdstr;
}

