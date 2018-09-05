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
#include "ftdio.h"
#include "errors.h"
#include "config.h"
#include "network.h"
#include "license.h"
#include "licprod.h"
#include "pathnames.h"
#include "common.h"
#include "cfg_intr.h"

#ifdef NEED_SYS_MNTTAB
#include "ftd_mnttab.h" 
#else
#include <sys/mnttab.h> 
#endif
#ifdef DEBUG
FILE *dbgfd;
#endif

char *argv0;

char jrnpaths[MAXJRN][32];
char fmt[256], msg[256];

int _rmd_state;
int _rmd_apply;
int _rmd_jrn_cp_lnum;
int _rmd_jrn_state;
int _rmd_jrn_mode;
int _rmd_jrn_num;
int _rmd_cpon;
int _rmd_cpstart;
int _rmd_cppend;
int _rmd_cpstop;
u_longlong_t _rmd_jrn_size;
static u_longlong_t jrn_offset;
int _rmd_jrn_chunksize;
int _rmd_rfddone;

struct iovec *iov;
int iovlen;
int max_idle_time = 10;

float compr_ratio;

static struct timeval skosh;

extern int exitsuccess;
extern int exitfail;
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
 * cp_file -- check for exitstence of given checkpoint file
 ***************************************************************************/
int
cp_file(char *jrnpath, char suffix_char)
{
    int len;
 
    if ((len = strlen(jrnpath))) {
        if (jrnpath[len-1] == suffix_char) {
            return 1;
        }
    }
    return 0;
} /* cp_file */

/****************************************************************************
 * parse_journal_name  -- parse the components from journal name 
 ***************************************************************************/
int
parse_journal_name(char *name, int *jrn_num, int *jrn_state, int *jrn_mode)
{
    char *lname;
    char state_c;
    int byte;
    int num;
    int len;

    lname = strdup(name);
    len = strlen(lname);
    state_c = lname[len-1];

    byte = len-5;    /* this better take us to the numeric part of the name */
    sscanf(&lname[byte], "%03d", &num);
   
    DPRINTF((ERRFAC,LOG_INFO,"\n*** parse_journal_name: name = %s\n", name));
 
    if (state_c == 'i') {
        *jrn_state = JRN_INCO;
        *jrn_mode = JRN_ONLY;
        *jrn_num = num;
    } else if (state_c == 'c') {
        *jrn_state = JRN_CO;
        *jrn_mode = _rmd_cpon ? JRN_ONLY: JRN_AND_MIR;
        *jrn_num = num;
    } else if (state_c == 'p') {
        *jrn_num = num;
        *jrn_state = _rmd_jrn_state ? _rmd_jrn_state: JRN_CO;
        *jrn_mode = _rmd_jrn_mode ? _rmd_jrn_mode: MIR_ONLY;
    } else {
        /* error */
        *jrn_state = JRN_INCO;
        *jrn_mode = MIR_ONLY;
        *jrn_num = -1;
    }
    free(lname);
    return 0;
} /* parse_journal_name */

/****************************************************************************
 * get_journals -- compile a list of journal paths 
 ***************************************************************************/
int
get_journals(char jrnpaths[][32], int prune, int coflag)
{
    DIR *dfd;
    group_t *group;
    struct dirent *dent;
    char jrn_path[MAXPATH];
    char jrn_prefix[MAXPATH];
    int lnum, lstate, lmode;
    int lgnum;
    int jrncnt;
    int prefix_len;
    int len;

    group = mysys->group;
    strcpy(jrn_path, group->journal_path);
    lgnum = cfgpathtonum(mysys->configpath);
    sprintf(jrn_prefix, "j%03d", lgnum);
 
    /* read journal directory - compile a sorted list of journals */
    if ((dfd = opendir(jrn_path)) == NULL) {
        reporterr(ERRFAC, M_JRNPATH, ERRCRIT, jrn_path, strerror(errno));
        return -1;
    }
    prefix_len = strlen(jrn_prefix);
    jrncnt = 0;
    while (NULL != (dent = readdir(dfd))) {
        if (memcmp(dent->d_name, jrn_prefix, prefix_len)) {
            continue;
        }
        parse_journal_name(dent->d_name, &lnum, &lstate, &lmode);
        if (cp_file(dent->d_name, CP_ON)) {
            _rmd_jrn_cp_lnum = lnum;
        }
        if (prune) {
            len = strlen(dent->d_name);
            if (dent->d_name[len-1] == CP_ON) {
                continue;
            }
        }
        if (coflag) {
            if (lstate == JRN_INCO) {
                continue;
            }
        }
        strcpy(jrnpaths[jrncnt], dent->d_name);
        jrncnt++;
    }
    (void)closedir(dfd);
    qsort((char*)jrnpaths, jrncnt, 32, stringcompare);
    return jrncnt;
} /* get_journals */

/****************************************************************************
 * rename_journals -- rename all journals 
 ***************************************************************************/
int
rename_journals(int state)
{
    group_t *group;
    char jrn_path[MAXPATH];
    char old_jrn_path[MAXPATH];
    char *lgnumstr;
    int jrncnt;
    int len;
    int i;

    group = mysys->group;
    jrncnt = get_journals(jrnpaths, 1, 0);

    if (jrncnt == 0) {
        return 0;
    }
    lgnumstr = cfgpathtoname(mysys->configpath);

    for (i = 0; i < jrncnt; i++) {
        len = strlen(jrnpaths[i]);
        if (len
        && (strncmp(&jrnpaths[i][1], lgnumstr, 3)
           || jrnpaths[i][len-1] == 'c'
           || jrnpaths[i][len-1] == 'p')) {
            continue;
        }
        sprintf(old_jrn_path, "%s/%s", group->journal_path, jrnpaths[i]);
        jrnpaths[i][strlen(jrnpaths[i])-2] = 0;
        if (state == JRN_CO) {
            sprintf(jrn_path, "%s/%s.c", group->journal_path, jrnpaths[i]);
        } else if (state == JRN_INCO) {
            sprintf(jrn_path, "%s/%s.i", group->journal_path, jrnpaths[i]);
        }
        DPRINTF((ERRFAC,LOG_INFO,"\n*** renaming old to new = %s, %s\n",            old_jrn_path, jrn_path));
        rename(old_jrn_path, jrn_path);
    }
    return 0;
} /* rename_journals */

/****************************************************************************
 * nuke_journals -- unlink all journals 
 ***************************************************************************/
int
nuke_journals(void)
{
    group_t *group;
    char jrn_path[MAXPATH];
    char jrn_prefix[MAXPATH];
    int prefix_len;
    int lgnum;
    int jrncnt;
    int i;

    if ((jrncnt = get_journals(jrnpaths, 0, 0)) == 0) {
        return 0;
    }
    lgnum = cfgpathtonum(mysys->configpath);
    group = mysys->group;
    sprintf(jrn_prefix, "j%03d", lgnum);
    prefix_len = strlen(jrn_prefix);

    for (i = 0; i < jrncnt; i++) {
        if (strlen(jrnpaths[i])
        && strncmp(jrnpaths[i], jrn_prefix, prefix_len)) {
            continue;
        }
        sprintf(jrn_path, "%s/%s", group->journal_path, jrnpaths[i]);
        unlink(jrn_path);
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
    int lnum;
    int lstate;
    int lmode;
    int len;
    int fd;
    int i; 
    int n; 

    group = mysys->group;
    strcpy(jrn_path, group->journal_path);
    sprintf(jrn_prefix, "j%03d", lgnum);
    jrncnt = get_journals(jrnpaths, 0, 0);

    lnum = 0;
    if (clobber && state == JRN_INCO) {
        /* we are INCO - get rid of directly prior INCO journals */
        while (jrncnt) {
            last_journal = jrnpaths[jrncnt-1];
            if (cp_file(last_journal, CP_ON)) {
                break;    
            } else {
                if (last_journal != NULL) {
                    parse_journal_name(last_journal, &lnum, &lstate, &lmode);
                    if (lstate == JRN_INCO) {
                        sprintf(tmp_path, "%s/%s", jrn_path, last_journal);
                        unlink(tmp_path);
                    } else {
                        break;
                    }
                }
            } 
            jrncnt = get_journals(jrnpaths, 0, 0); 
        } 
    }
    if (jrncnt) {
        last_journal = jrnpaths[jrncnt-1];
        parse_journal_name(last_journal, &lnum, &lstate, &lmode);
    }
    lnum++;
    if (state == JRN_CP) {
        /* if checkpoint already on or pending then return */
        for (i = 0; i < jrncnt; i++) {
            if ((len = strlen(jrnpaths[i]))) {
                if (jrnpaths[i][len-1] == 'p') {
                    sprintf(lgname, "%03d", lgnum);
                    if (i == 0) {
                        reporterr(ERRFAC, M_CPONAGAIN, ERRWARN, lgname);
                    } else { 
                        reporterr(ERRFAC, M_CPSTARTAGAIN, ERRWARN, argv0);
                    }
                    return 1;
                }        
            }        
        }     
        sprintf(tmp_path, "%s/%s.%03d.p", jrn_path, jrn_prefix, lnum);
        /* create a checkpoint-on file */
        if ((fd = open(tmp_path, O_CREAT, 0600)) == -1) {
            reporterr(ERRFAC, M_JRNOPEN, ERRCRIT, tmp_path, strerror(errno));
            return -1;
        }
        close(fd);
        return 0;
    } 
    state_str = state == JRN_CO ? "c": "i"; 
    sprintf(tmp_path, "%s/%s.%03d.%s", jrn_path, jrn_prefix, lnum, state_str);

    /* open a new journal */
    if (group->jrnfd >= 0) {
        close_journal(group);
    }
    if ((group->jrnfd = open(tmp_path, O_RDWR | O_CREAT, 0600)) == -1) {
        reporterr(ERRFAC, M_JRNOPEN, ERRCRIT, tmp_path, strerror(errno));
        return -1;
    }
    strcpy(group->journal, tmp_path);
    parse_journal_name(tmp_path, &_rmd_jrn_num, &_rmd_jrn_state, &_rmd_jrn_mode);
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

    return group->jrnfd;
} /* new_journal */

/****************************************************************************
 * close_journal -- close journal 
 ***************************************************************************/
int
close_journal(group_t *group)
{
    close(group->jrnfd);
    group->jrnfd = -1;
    memset(group->journal, 0, sizeof(group->journal));

    return 0;
} /* close_journal */

/****************************************************************************
 * clobber_journal -- clobber journal 
 ***************************************************************************/
int
clobber_journal(int *jrnfdp, char *journal)
{
    unlink(journal); 
    if (jrnfdp) {
        close(*jrnfdp);
        *jrnfdp = -1;
    }
    memset(journal, 0, sizeof(journal));

    return 0;
} /* clobber_journal */

/****************************************************************************
 * apply_journal -- apply journal entries to mirrors 
 ***************************************************************************/
int
apply_journal(group_t *group)
{
    headpack_t *header;
    char *datap;
    static char *databuf = NULL;
    static int datalen = 0;
    struct stat statbuf;
    u_longlong_t offset;
    u_longlong_t bytes_remain;
    int chunksize;
    u_longlong_t lock_len;
    int rc;
    int i;

    DPRINTF((ERRFAC,LOG_INFO,"\n*** apply_journal: _rmd_jrn_state = %d\n",         _rmd_jrn_state));
    DPRINTF((ERRFAC,LOG_INFO,"\n*** apply_journal: _rmd_jrn_mode = %d\n",        _rmd_jrn_mode));
    DPRINTF((ERRFAC,LOG_INFO,"\n*** apply_journal: group->jrnfd, journal = %d, %s\n",         group->jrnfd,group->journal));

    jrn_offset = sizeof(jrnheader_t);
    _rmd_jrn_size = -1;

    DPRINTF((ERRFAC,LOG_INFO,"\n*** apply_journal: jrn_offset = %llu\n",	        jrn_offset));

    _rmd_jrn_chunksize = 1024*1024;
    lock_len = _rmd_jrn_chunksize+1;

    /* process the journal */
    while (1) {
        if ((offset = llseek(group->jrnfd, jrn_offset, SEEK_SET)) == -1) {
            reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
                argv0, group->journal, jrn_offset, strerror(errno));
            return -1;
        }
        if ((_rmd_jrn_size-offset) < lock_len) {
            lock_len = (_rmd_jrn_size-offset);
        }
        rc = lock_journal(offset, lock_len);
        if (rc == -1) {
            reporterr(ERRFAC, "JRNLOCK", ERRCRIT,
                group->journal, argv0);
            return -1;
        }
        /* we have a lock on the section - get the size */
        if (stat(group->journal, &statbuf) == -1) {
            reporterr(ERRFAC, M_STAT, ERRCRIT, group->journal, strerror(errno));
            return -1;
        }
        _rmd_jrn_size = statbuf.st_size;
        DPRINTF((ERRFAC,LOG_INFO,"\n*** jrn_offset = %llu\n",            jrn_offset));
        DPRINTF((ERRFAC,LOG_INFO,"\n*** _rmd_jrn_size = %llu\n",            _rmd_jrn_size));

        bytes_remain = _rmd_jrn_size - jrn_offset;
        chunksize = _rmd_jrn_chunksize < bytes_remain ? _rmd_jrn_chunksize:
                    bytes_remain;

        if (chunksize <= sizeof(headpack_t)) {
            clobber_journal(&group->jrnfd, group->journal);
            memset(jrnpaths[0], 0, sizeof(jrnpaths[0]));
            return 0;
        }
        if (chunksize > datalen) {
            /* re-allocate data buffer */
            datalen = chunksize;
            databuf = ftdrealloc(databuf, datalen);
        }
        /* read journal chunk */
        if ((rc = ftdread(group->jrnfd, databuf, chunksize)) != chunksize) {
            reporterr(ERRFAC, M_READERR, ERRCRIT,
                group->journal, jrn_offset, chunksize, strerror(errno));
            return -1;
        }
        offset = 0;

        /* process journal chunk entries */
        while (chunksize > 0) {
            if (chunksize < sizeof(headpack_t)) {
                break;
            }
            header = (headpack_t*)(databuf+offset);
            offset += sizeof(headpack_t);
            chunksize -= sizeof(headpack_t);

            DPRINTF((ERRFAC,LOG_INFO,"\n*** chunksize, offset, header->len = %d, %llu, %d\n",                 chunksize, offset, header->len));

            if (chunksize < header->len) {
                /* done with this chunk */
                offset -= sizeof(headpack_t);
                if (offset == 0) {
                    /* need bigger chunk */
                    _rmd_jrn_chunksize *= 2;
                }
                break;
            }
            datap = databuf+offset;

            /* write data to appropriate mirror */
            if (write_jrn_to_mir(header, datap) == -1) {
                reporterr(ERRFAC, "JRNWRITE", ERRCRIT, argv0, group->journal);
                return -1;
            }
            /* bump journal offset */
            offset += header->len;
            chunksize -= header->len;
        } 
        /* unlock the locked journal section */
        rc = unlock_journal(jrn_offset+lock_len, ~(lock_len));
        if (rc == -1) {
            reporterr(ERRFAC, "JRNUNLOCK", ERRCRIT,
                group->journal, argv0, strerror(errno));
            return -1;
        }
        jrn_offset += offset;
        DPRINTF((ERRFAC,LOG_INFO,"\n*** [%s] released lock on journal: %s [%llu-(%lld)]\n",             argv0, group->journal, jrn_offset, ~(jrn_offset-1)));
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

    sprintf(testf, "%s/testf",jrnpath);
    if ((fd = open(testf, O_RDWR | O_CREAT)) == -1) {
        return 0;
    }
    if (ftdwrite(fd, testf, sizeof(testf)) != sizeof(testf)) {
        close(fd);
        unlink(testf);
        return 0;
    }
    close(fd);
    unlink(testf);
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
    int inco_flag;
    int i;

    /* set global defaults */
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
    if ((jrncnt = get_journals(jrnpaths, 0, 0))) {
        for (i = 0; i < jrncnt; i++) {
            if (cp_file(jrnpaths[i], CP_ON)) {
                if (i == 0) {
                    _rmd_cpon = 1;
                } else {
                    _rmd_cppend = 1;
                }
                break;
            }
        }
    } 
    inco_flag = 0;
    while (1) {
        /* this time prune checkpoint files */
        if ((jrncnt = get_journals(jrnpaths, 1, 0)) == 0) {
            break;
        } else {
            jrn_path = jrnpaths[jrncnt-1];    /* most recent journal file */
            if (strlen(jrn_path)) {
                parse_journal_name(jrn_path,
                    &_rmd_jrn_num, &_rmd_jrn_state, &_rmd_jrn_mode);
                if (_rmd_jrn_state == JRN_INCO) {
                    inco_flag = 1;
                    /* weed out previous inco journals - if we are the RMD */
                    if (!strncmp(argv0, "RMD_", 4)) {
                        sprintf(journal,
                        "%s/%s", mysys->group->journal_path, jrn_path);
                        unlink(journal);
                        _rmd_jrn_state = JRN_CO;
                        continue;
                    } else {
                        break;
                    }
                } else {
                    /* coherent journal found */
                    break;
                }
            } else {
                _rmd_jrn_num = 0;
                _rmd_jrn_state = JRN_CO;
                _rmd_jrn_mode = MIR_ONLY;
                break;
            }
        }
    }
    _rmd_jrn_chunksize = 1024000;
    if (_rmd_cpon || _rmd_cppend) {
        _rmd_jrn_mode = JRN_ONLY;
    }

/*
    if (inco_flag) {
        * INCO journals were found - we were previously in an INCO state *
        _rmd_jrn_state = JRN_INCO;
    }
*/
    DPRINTF((ERRFAC,LOG_INFO,         "\n*** init_j: cpon, jrn_num, jrn_state, jrn_mode = "\
        "%d, %d, %d, %d\n",
        _rmd_cpon, _rmd_jrn_num, _rmd_jrn_state, _rmd_jrn_mode));

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
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, entry->diskdev);
        exit(EXITANDDIE);
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
int
write_jrn_to_mir (headpack_t *header, char *datap)
{
    group_t *group;
    sddisk_t *sd;
    u_longlong_t offset;
    u_longlong_t length;
    u_longlong_t ret;
    time_t currentts;
    int len32;
    int err;
    int rc;

    group = mysys->group;

    if ((sd = get_lg_dev(group, header->devid)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, header->devid);
        exit(EXITANDDIE);
    }
    offset = header->offset;
    offset <<= DEV_BSHIFT;
    length = header->len;

    if (offset == 0 && sd->no0write) {
        DPRINTF((ERRFAC,LOG_INFO,"\n*** skipping sector 0 write on mirror: %s\n",	            sd->mirname));
        offset += DEV_BSIZE;
        datap += DEV_BSIZE;
        length -= DEV_BSIZE;
    }
    DPRINTF((ERRFAC,LOG_INFO,"\n*** write_jrn_to_mir: %llu bytes @ offset %llu\n",         length, offset));
    DPRINTF((ERRFAC,LOG_INFO,"\n*** write_jrn_to_mir: buffer address %d\n",	        datap));

    if ((ret = llseek(sd->mirfd, offset, SEEK_SET)) == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, sd->mirname, offset, strerror(errno));
        return -1;
    }
    len32 = length;
    if ((rc = ftdwrite(sd->mirfd, (char*)datap, len32)) != len32) {
        err = errno;
        geterrmsg(ERRFAC, M_WRITEERR, fmt);
        sprintf(msg, fmt, argv0, sd->mirname, sd->devid,
            offset, length, strerror(err));
        logerrmsg(ERRFAC, ERRCRIT, M_WRITEERR, msg);
        senderr(mysys->sock, header, 0, ERRCRIT, M_WRITEERR, msg); 
    }
    time(&currentts);
    sd->stat.jrnentage = (currentts - header->ts) - mysys->tsbias;

    return 0;
} /* write_jrn_to_mir */

/****************************************************************************
 * write_mirror -- RMD - write entry to mirror 
 ***************************************************************************/
int
write_mirror (wlheader_t *entry, char *dataptr)
{
    group_t *group;
    headpack_t header[1];
    sddisk_t *sd;
    u_longlong_t ret;
    u_longlong_t offset;
    u_longlong_t length;
    int len32;
    int lgnum; 
    int rc;

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    if ((sd = get_lg_rdev(group, entry->diskdev)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, (int) entry->diskdev);
        exit(EXITANDDIE);
    }
    offset = entry->offset;
    offset <<= DEV_BSHIFT;
    length = entry->length;
    length <<= DEV_BSHIFT;

    if (offset == 0 && sd->no0write) {
        DPRINTF((ERRFAC,LOG_INFO,"\n*** skipping sector 0 write on mirror: %s\n",              sd->mirname));
        offset += DEV_BSIZE;
        dataptr += DEV_BSIZE;
        length -= DEV_BSIZE;
    }
    DPRINTF((ERRFAC,LOG_INFO,"\n*** write_mirror: %llu bytes @ offset %llu\n",         length, offset));

    ret = llseek(sd->mirfd, offset, SEEK_SET);
    if (ret == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, sd->mirname, offset, strerror(errno));
        return -1;
    }
    len32 = length;
    if (ftdwrite(sd->mirfd, (char*)dataptr, len32) != len32) {
        geterrmsg(ERRFAC, M_WRITEERR, fmt);
        sprintf(msg, fmt, argv0, sd->mirname, sd->devid,
            offset, length, strerror(errno));
        logerrmsg(ERRFAC, ERRCRIT, M_WRITEERR, msg);
        senderr(mysys->sock, header, 0, ERRCRIT, M_WRITEERR, msg); 
    }
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
    u_longlong_t lock_len;
    u_longlong_t new_rmd_jrn_size;
    u_longlong_t size_threshold;
    u_longlong_t ulength;
    offset_t ret;
    time_t currentts;
    int lgnum;
    int rc;
    int i;

    group = mysys->group;

    lgnum = cfgpathtonum(mysys->configpath);

    if ((sd = get_lg_rdev(group, entry->diskdev)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, header->devid);
        exit(EXITANDDIE);
    }
    size_threshold = MAXINT;

    length = (entry->length << DEV_BSHIFT);

    /* static header values */
    memset(header, 0, sizeof(headpack_t));
    header->magicvalue = MAGICHDR;
    header->cmd = CMDWRITE;
    header->devid = sd->devid;
    header->offset = entry->offset;
    header->len = length;
    header->ackwanted = 0;
    time(&header->ts);

    /* if write will make journal > 2G then create new journal */
    new_rmd_jrn_size = sizeof(headpack_t)+length+_rmd_jrn_size;
    if (new_rmd_jrn_size > size_threshold) {
        group->jrnfd = new_journal(lgnum, _rmd_jrn_state, 0);
    }
    ret = llseek(group->jrnfd, _rmd_jrn_size, SEEK_SET);
    if (ret == -1) {
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
        } else {
            geterrmsg(ERRFAC, M_WRITEERR, fmt);
            ulength = sizeof(headpack_t);
            sprintf(msg, fmt, argv0, group->journal, sd->devid, 
                _rmd_jrn_size, ulength, strerror(errno));
            logerrmsg(ERRFAC, ERRCRIT, M_WRITEERR, msg);
            senderr(mysys->sock, header, 0, ERRCRIT, M_WRITEERR, msg); 
        }
    }
    if ((rc = ftdwrite(group->jrnfd, dataptr, length)) != length) {
        if (errno == ENOSPC) {
            geterrmsg(ERRFAC, M_JRNSPACE, fmt);
            sprintf(msg, fmt, argv0, group->journal);
            logerrmsg(ERRFAC, ERRCRIT, M_JRNSPACE, msg);
            senderr(mysys->sock, header, 0, ERRCRIT, M_JRNSPACE, msg); 
        } else {
            geterrmsg(ERRFAC, M_WRITEERR, fmt);
            ulength = length;  
            sprintf(msg, fmt, argv0, group->journal, sd->devid, 
                _rmd_jrn_size, ulength, strerror(errno));
            logerrmsg(ERRFAC, ERRCRIT, M_WRITEERR, msg);
            senderr(mysys->sock, header, 0, ERRCRIT, M_WRITEERR, msg); 
        }
    }
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

    group = mysys->group;

    ret = llseek(group->jrnfd, offset, SEEK_SET);
    if (ret == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, group->journal, offset, strerror(errno));
        return -1;
    }
    tries = 100;
    for (i=0;i<tries;i++) {
        DPRINTF((ERRFAC,LOG_INFO,
            "\n*** [%s] locking journal: %s, offset, length = %llu, %llu\n",             argv0, group->journal, offset, lock_len));
        if ((rc = lockf(group->jrnfd, F_TLOCK, lock_len)) == 0) {
            break;
        } else {
            if (access(group->journal, F_OK) == -1) {
                /* must be done applying */
                return 1;
            }
            DPRINTF((ERRFAC,LOG_INFO,                "\n*** [%s] journal lock failure: %s\n",strerror(errno)));
            sleep(1);
        }
    }
    if (i == tries) {
        return -1;
    }
    DPRINTF((ERRFAC,LOG_INFO,"\n*** [%s] aquired lock on journal: %s [%llu-(%llu)]\n",        argv0, group->journal, offset, lock_len));
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

    ret = llseek(group->jrnfd, offset, SEEK_SET);
    if (ret == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, group->journal, offset, strerror(errno));
        return -1;
    }
    if (lockf(group->jrnfd, F_ULOCK, lock_len) == -1) {
        return -1;
    }
    DPRINTF((ERRFAC,LOG_INFO,"\n*** [%s] released lock on journal: %s [%llu-(%lld)]\n",        argv0, group->journal, offset, lock_len));
    return 0;
} /* unlock_journal */

/****************************************************************************
 * traverse_chunk -- steps thru chunk by entry 
 ***************************************************************************/
int
traverse_chunk(char *chunk, int *length, EntryFunc *func) 
{
    group_t *group;
    wlheader_t *wlhead;
    headpack_t header[1];
    ackpack_t ack[1];
    char path[MAXPATHLEN];
    char lgname[16];
    char prefix[16];
    char *dataptr;
    u_longlong_t ret;
    u_longlong_t offset;
    u_longlong_t lock_len;
    u_longlong_t unlock_len;
    int chunkoff = 0;
    int entrylen;
    int bytesleft;
    int jrn_locked;
    int jrnpath_len;
    int prefix_len;
    int lgnum;
    int jrncnt;
    int pcnt;
    int len;
    int i;
    int lnum, lstate, lmode;
    int rc = 0;

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    jrn_locked = 0;

    if (func == &write_journal
    && group->jrnfd >= 0
    && _rmd_jrn_mode == JRN_AND_MIR) {
        offset = llseek(group->jrnfd, (offset_t)1, SEEK_END);
        _rmd_jrn_size = offset-1;
        lock_len = *length;
        rc = lock_journal(offset, lock_len);
        if (rc == -1) {
            reporterr(ERRFAC, "JRNLOCK", ERRCRIT,
                group->journal, argv0);
            return -1;
        } else if (rc == 1) {
            func = write_mirror;
            _rmd_jrn_state = JRN_CO;        
            _rmd_jrn_mode = MIR_ONLY;        
        }
        jrn_locked = 1;
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
        /* run func on entry  */
        wlhead = (wlheader_t*)(chunk+chunkoff);
#ifdef TDMF_TRACE
        fprintf(stderr,"\n*** %s - trav_chunk: wlhead->majicnum = %08x\n",
            argv0, wlhead->majicnum);
        fprintf(stderr,"\n*** %s - trav_chunk: wlhead->offset = %d\n",
            argv0, wlhead->offset);
        fprintf(stderr,"\n*** %s - trav_chunk: wlhead->length = %d\n",
            argv0, wlhead->length);
#endif
        entrylen = wlhead->length << DEV_BSHIFT;
        bytesleft = *length-chunkoff-sizeof(wlheader_t);

        /* if partial-entry then adjust chunk size and return */
        if (bytesleft && bytesleft < entrylen) {
            *length = chunkoff;
            rc = 0;
            break;
        } 
        if (wlhead->majicnum != DATASTAR_MAJIC_NUM) {
            reporterr (ERRFAC, M_BADHDR, ERRCRIT, argv0, group->devname);
            if (ISPRIMARY(mysys)) {
                exit(EXITANDDIE);
            } else {
                geterrmsg(ERRFAC, M_BADHDR, fmt);
                sprintf(msg, fmt, argv0, group->devname);
                logerrmsg(ERRFAC, ERRCRIT, M_BADHDR, msg);
                senderr(mysys->sock, header, 0, ERRCRIT, M_BADHDR, msg); 
                return -1;
            }
        }
        dataptr = ((char*)wlhead+sizeof(wlheader_t));

        /* look for special entries */
        if (wlhead->offset == -1 && 
            !memcmp(dataptr, MSG_INCO, strlen(MSG_INCO))) {
            DPRINTF((ERRFAC,LOG_INFO,"\n*** [%s] got MSG_INCO = %s\n",                argv0,MSG_INCO));
            /* start journaling */
            dataptr += strlen(MSG_INCO)+1;
            if (ISSECONDARY(mysys)) {
                /* refresh */
                iovlen = 0;
                iov = NULL;
                if ((group->jrnfd = new_journal(lgnum, JRN_INCO, 1)) == -1) {
                    return -1;
                }
                if ((lock_len = *length-chunkoff-entrylen-sizeof(wlheader_t)) > 0) {
                    if (lock_journal(sizeof(jrnheader_t), lock_len) == -1) {
                        reporterr(ERRFAC, "JRNLOCK", ERRCRIT,
                            group->journal, argv0);
                        return -1;
                    }
                    jrn_locked = 1;
                }
                closedevs(O_RDONLY);
                func = &write_journal;
                _rmd_state = FTDRFD;
            }
        } else if (wlhead->offset == -1 && 
            !memcmp(dataptr, MSG_CO, strlen(MSG_CO))) {
            DPRINTF((ERRFAC,LOG_INFO,"\n*** [%s] got MSG_CO = %s\n",                argv0,MSG_CO));
            dataptr += strlen(MSG_CO)+1;
            if (ISSECONDARY(mysys)) {
                if (_rmd_state == FTDRFD) {
                    /* free global iov structure */
                    if (iov) {
                        iovlen = 0;
                        free(iov);
                        iov = NULL;
                    }
                    _rmd_rfddone = 1;
                    close_journal(group);
                    rename_journals(JRN_CO);
                    if ((group->jrnfd = new_journal(lgnum, JRN_CO, 1)) == -1) {
                        return -1;
                    }
                    if ((lock_len = *length-chunkoff-entrylen-sizeof(wlheader_t)) > 0) {
                        if (lock_journal(sizeof(jrnheader_t), lock_len) == -1) { 
                            reporterr(ERRFAC, "JRNLOCK", ERRCRIT,
                                group->journal, argv0);
                            return -1;
                        }
                        jrn_locked = 1;
                    }
                    if (!_rmd_cpon) {
                        /* only apply if we are NOT in checkpoint */
                        closedevs(O_RDONLY);
                        start_rmdapply(0);
                        _rmd_jrn_mode = JRN_AND_MIR;
                    }
                }
                _rmd_jrn_state = JRN_CO;
                _rmd_state = FTDPMD;
            }
        } else if (wlhead->offset == -1 &&
            !memcmp(dataptr, MSG_CPON, strlen(MSG_CPON))) {
            DPRINTF((ERRFAC,LOG_INFO,"\n*** [%s] got MSG_CPON = %s\n",                  argv0,MSG_CPON));
            dataptr += strlen(MSG_CPON)+1;
            if (ISSECONDARY(mysys)) {
                /* checkpoint-on */
                sprintf(lgname, "%03d", lgnum);

                /* close mirror devices */
                closedevs(-1);

                /* create a checkpoint file */
                if ((rc = new_journal(lgnum, JRN_CP, 1)) == -1) {
                    return -1;
                } else if (rc == 0) {
                    /* create a new journal file to write to during apply */
                    group->jrnfd = new_journal(lgnum, _rmd_jrn_state, 1);
                    if (group->jrnfd == -1) {
                        return -1;
                    }
                    /* apply coherent data up to cp - wait for completion */
                    start_rmdapply(1);
                }
                func = &write_journal;

                _rmd_cppend = 1;
                _rmd_cpstart = 0;
                _rmd_cpon = 0;
                _rmd_jrn_mode = JRN_AND_MIR;

                if ((lock_len = *length-chunkoff-entrylen-sizeof(wlheader_t)) > 0) {
                    if (lock_journal(sizeof(jrnheader_t), lock_len) == -1) { 
                        reporterr(ERRFAC, "JRNLOCK", ERRCRIT,
                            group->journal, argv0);
                        return -1;
                    }
                    jrn_locked = 1;
                }
            } else {
                /* primary system */
            }
        } else if (wlhead->offset == -1 &&
            !memcmp(dataptr, MSG_CPOFF, strlen(MSG_CPOFF))) {
            DPRINTF((ERRFAC,LOG_INFO,"\n*** [%s] got MSG_CPOFF = %s\n",	                argv0, MSG_CPOFF));
            dataptr += strlen(MSG_CPOFF)+1;
            if (ISSECONDARY(mysys)) {
                sprintf(lgname, "%03d", lgnum);
                if (exec_cp_cmd(lgnum, PRE_CP_OFF, 0) == 0) {
/*
                    closedevs(-1);
                    rc = verify_rmd_entries(group, mysys->configpath, 1);
*/
                    rc = 0;
                    if (rc == 0) {
                        /* kill RMDA if it is running, we will be restarting it */
                        stop_rmdapply();

                        if ((jrncnt = get_journals(jrnpaths, 0, 0)) > 0) {
                            sprintf(prefix, "j%03d",lgnum);
                            prefix_len = strlen(prefix);
                            for (i = 0; i < jrncnt; i++) {
                                jrnpath_len = strlen(jrnpaths[i]);
                                if (!strncmp(jrnpaths[i], prefix, prefix_len)) {
                                    if (cp_file(jrnpaths[i], CP_ON)) {
                                        sprintf(path, "%s/%s",
                                            group->journal_path, jrnpaths[i]);
                                        clobber_journal(NULL, path);
                                    }
                                } 
                            } 
                        }
                        /* we need read access to the mirrors */
                        closedevs(O_RDONLY);

                        /* create a CO journal to write to while applying */
                        if ((group->jrnfd =
                            new_journal(lgnum, JRN_CO, 1)) == -1) {
                            return -1;
                        }

                        start_rmdapply(0);

                        _rmd_cpon = 0;
                        _rmd_cpstop = 0;
                        _rmd_jrn_cp_lnum = -1;
                        _rmd_jrn_mode = JRN_AND_MIR;

                        /* out of checkpoint mode */

                        reporterr(ERRFAC, M_CPOFF, ERRINFO, argv0); 
                        /* send same msg to PMD */
                        memset(header, 0, sizeof(headpack_t));
                        ack->data = ACKCPOFF;
                        sendack(mysys->sock, header, ack); 
                    } else {
                        send_cp_err_ack(ACKCPOFFERR);
                        reporterr(ERRFAC, M_CPOFFERR, ERRWARN, argv0); 
                        _rmd_cpstop = 0;
                    }
                } else {
                    send_cp_err_ack(ACKCPOFFERR);
                    reporterr(ERRFAC, M_CPOFFERR, ERRWARN, argv0); 
                    _rmd_cpstop = 0;
                }
                if ((lock_len = *length-chunkoff-entrylen-sizeof(wlheader_t)) > 0) {
                    if (lock_journal(sizeof(jrnheader_t), lock_len) == -1) { 
                        reporterr(ERRFAC, "JRNLOCK", ERRCRIT,
                            group->journal, argv0);
                        return -1;
                    }
                    jrn_locked = 1;
                }
            }
        } else {
            rc = func((void*)wlhead, dataptr);
            if (rc == 1) {
                rc = 1;
            }
        }
        chunkoff += sizeof(wlheader_t)+entrylen;
    }
    /* make sure chunk gets sync'd to disk before ACKing */
    if (jrn_locked) {
        fsync(group->jrnfd);
        offset = llseek(group->jrnfd, (offset_t)1, SEEK_END);
        if (offset == -1) {
            reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
                argv0, group->journal, offset, strerror(errno));
            return -1;
        }
        unlock_len = (offset-1);
        rc = unlock_journal(offset, ~(unlock_len));
        if (rc == -1) {
            reporterr(ERRFAC, "JRNUNLOCK", ERRCRIT,
                group->journal, argv0, strerror(errno));
            return -1;
        }
    }
    return rc;
} /* traverse_chunk */

/****************************************************************************
 * spin -- update internal state, stats until interupted
 ***************************************************************************/
static int
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
            savestats(0);
            lastts = currts;
        }
    }
} /* spin */

/****************************************************************************
 * flush_net -- process all the ACKs on the wire
 ***************************************************************************/
int
flush_net (int fd) 
{
    int i;

    /* tell RMD that we have been interupted */
    if (sendhup() == -1) {
        return -1;
    }
    g_ackhup = 0;
    for (;;) {
        process_acks();
        if (g_ackhup) {
            break;
        } 
        /* sleep and try again */
        usleep(100000);
    }
    mysys->group->offset = 0;
    mysys->group->size = 0;

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
    int rc;

    oldest_entries_t oe[1];

    rc = 0;
    lgnum = cfgpathtonum(mysys->configpath);

    group->data = NULL;
    group->size = 0;
    group->endofdata = group->chunk - 1;

    memset(oe, 0, sizeof(oldest_entries_t));

    if (mysys->tunables.chunksize > group->chunkdatasize) {
        group->chunk = (char*)ftdmalloc(mysys->tunables.chunksize);
        group->chunkdatasize = mysys->tunables.chunksize;
    }
    oe->addr = (int*)group->chunk;
    oe->offset32 = group->offset;
    oe->len32 = group->chunkdatasize / sizeof(int);

#ifdef TDMF_TRACE
    fprintf(stderr,"\n*** getchunk: oe->offset32 = %d\n",
        oe->offset32);
    fprintf(stderr,"\n*** getchunk: requested length = %d\n",
        oe->len32*sizeof(int));
#endif
    if (0 > (rc = ioctl(group->devfd, FTD_OLDEST_ENTRIES, oe))) {
        if (errno == EINVAL) {
            /* no more journal to read */
            return 0;
        }
        reporterr (ERRFAC, M_BLKGET, ERRCRIT, group->devname, errno);
        return -1;
    }
#ifdef TDMF_TRACE
    fprintf(stderr,"\n*** getchunk: get-oldest rc = %d\n",rc);
    fprintf(stderr,"\n*** getchunk: oe->retlen32 = %d\n",oe->retlen32);
    fprintf(stderr,"\n*** getchunk: oe->state = %d\n",oe->state);
#endif

    switch(oe->state) {
    case FTD_MODE_PASSTHRU:
        reporterr (ERRFAC, M_PASSTHRU, ERRCRIT, group->devname);
        exit(EXITANDDIE);
    case FTD_MODE_NORMAL:
        break;
    case FTD_MODE_REFRESH:
        break;
    case FTD_MODE_TRACKING:
        DPRINTF((ERRFAC,LOG_INFO,
        "\n*** drvr in TRACKING!, refresh_started, refresh_oflow = %d, %d\n",             refresh_started, refresh_oflow));
        if (_pmd_state == FTDRFD || _pmd_state == FTDRFDF) {
            if (refresh_started || refresh_oflow) {
                /* if we made the transition from REFRESH mode */
                reporterr (ERRFAC, M_RFDOFLOW, ERRCRIT, argv0);
                if (getpsname(lgnum, ps_name)) {
                    reporterr(ERRFAC, M_CFGPS, ERRWARN,
                        argv0, mysys->configpath);
                    exit (EXITANDDIE);
                }
                if ((rc = ps_set_group_key_value(ps_name, group->devname,
                    "_MODE:", "ACCUMULATE")) != PS_OK) {
                    reporterr(ERRFAC, M_PSWTDATTR, ERRWARN, group->devname, ps_name);
                    exit (EXITANDDIE);
                }
                /* spin - until interupted */
                spin(lgnum);
                return -2;
            }
        } else {
            reporterr(ERRFAC, M_BABOFLOW, ERRWARN, argv0);
            _pmd_state = FTDRFD;
            _pmd_state_change = 1;
            return -2;
        }
        break;
    default:
        break;
    }
    group->size = oe->retlen32*sizeof(int); /* bytes */
    group->data = group->chunk;
    group->endofdata = group->chunk+group->size-1;

    if (oe->retlen32 == 0) {
        return 0;
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
    sb.addr = (char*)&info;

    rc = ioctl(mysys->ctlfd, FTD_GET_GROUP_STATS, &sb);
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
 ***************************************************************************/
int
sync_fs (void)
{
    sddisk_t *sd;
    char mountp[MAXPATHLEN];
    FILE *fp;
    DIR *fd;
    int rc;
    int i;

#if !defined(_AIX)
    fp = fopen ("/etc/mnttab", "r");
#else  /* !defined(_AIX) */
    fp = (FILE *)NULL;
#endif /* !defined(_AIX) */
    fd = NULL;

    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
        if (dev_mounted(sd->sddevname, mountp)) {
            /* sync it */
            if ((fd = opendir(mountp)) == NULL) {
                reporterr(ERRFAC, M_FILE, ERRWARN,
                    mountp, strerror(errno));
#if !defined(_AIX)
                fclose(fp);
#endif /* !defined(_AIX) */
                return -1;
            }
#if defined(SOLARIS)
            /* lock the fs */
/*
this is dangerous
            if ((rc = execc_fs_sync(mountp == -1) {
                * oh well, didn't sync the fs *
            }
*/
            (void)sync();
            /* unlock the fs */
/*
            for (i = 0; i < 20; i++) {
                if ((rc = exec_fs_sync(mountp, 0)) == -1) {
                    sleep(1);
                } else {
                    break;
                }
            }
*/
#elif defined(HPUX) || defined(_AIX)
            (void)sync();
#endif
            if (fd) {
                closedir(fd);
            }
        }
    }
#if !defined(_AIX)
    fclose(fp);
#endif /* !defined(_AIX) */
    return 0;
} /* sync_fs */

/****************************************************************************
 * dispatch -- run forever and send chunks to remote
 ***************************************************************************/
int
dispatch (void *args)
{
    fd_set read_set[1];
    fd_set read_set_copy[1];
    group_t *group;
    time_t currts, lastts, lastlrdb;
    long timeslept;
    long io_sleep;
    int lgnum;
    int deltatime;
    int nselect;
    int rc;
    int byteoffset;
    static int bab_percent_full_threshold = 0;
    float bab_percent_full;

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
                /* this must succeed ! - DO NOT transition into cp on failure */
                if (exec_cp_cmd(lgnum, PRE_CP_ON, 1) == 0) {
                    /* sync filesystems */
                    sync_fs();
                    send_cp_on(lgnum);
                    exec_cp_cmd(lgnum, POST_CP_ON, 1);
                    _pmd_cpstart = 3;
                } else {
                    reporterr(ERRFAC, M_CPONERR, ERRWARN, argv0); 
                    /* if request came from RMD then send err */
                    if (_pmd_cpstart == 2) {
                        send_cp_err(CMDCPONERR);
                    }
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
                send_cp_off(lgnum);
                _pmd_cpstop = 3;
            }
            /* ack the ftdcheckpoint command process */
            ipcsend_ack(_pmd_wpipefd);
        }
        process_acks();

        if (mysys->tunables.chunkdelay > 0) {
            usleep(mysys->tunables.chunkdelay);
        }	
        *read_set_copy = *read_set;

        /* wait for something to read - wake up periodically */
        rc = select(nselect, read_set_copy, NULL, NULL, &skosh);

        DPRINTF((ERRFAC,LOG_INFO,"\n*** [%s] dispatch: select rc = %d\n",             argv0,rc));

        if (rc == -1) {
           DPRINTF((ERRFAC,LOG_INFO,"\n*** dispatch: select rc, errno = %d, %d\n",                rc,errno));
           if (errno == EAGAIN || errno == EINTR) {
               if (_pmd_state_change) {
                   return 0;
               }
               continue;
           } else {
               reporterr(ERRFAC, M_DRVERR, ERRCRIT,
                   "select", strerror(errno));
               break;
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
                    bab_percent_full = 
                        (100.0*(float)((float)group->babused/
                        ((float)group->babused + (float)group->babfree)));
                    if (bab_percent_full <= (float)mysys->tunables.bablow) {
                        bab_percent_full_threshold = mysys->tunables.babhigh;
                    }
                    if (bab_percent_full >= (float)mysys->tunables.babhigh) {
                        bab_percent_full_threshold = mysys->tunables.bablow;
                    }
                    if (bab_percent_full < bab_percent_full_threshold) {
                        /* 
                         * leave BAB alone for now
                         */
                        return 0;
                    }
                }

                if ((rc = getchunk(group)) < 0) {
                    if (rc == -2) {
                        return 0;
                    } else {
                        return rc;
                    }
                }
                DPRINTF((ERRFAC,LOG_INFO,"\n*** dispatch: group->size = %d\n",                     group->size));

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
                    /* send the data to the remote */
                    rc = sendchunk(mysys->sock, group);
                    DPRINTF((ERRFAC,LOG_INFO,"\n*** sendchunk rc = %d\n",                            rc));
                    if (rc == -1) {
                        closeconnection();
                        exit(EXITRESTART);
                    }
                    group->offset += (group->size/sizeof(int));
                } else {
                    /* no data returned from getchunk */
                    if (_pmd_state == FTDPMD) {
                        usleep(100000);
                        continue;
                    } else if (refresh_started) {
                        return 0;
                    } else {
                        /* if entries in BAB && transitioning to refresh */
                        rc = 0;
                        if (bab_has_entries(lgnum, &group->babused,
                            &group->babfree)) {
                            DPRINTF((ERRFAC,LOG_INFO,                                "\n*** dispatch: bab_has_entries\n"));
                            usleep(100000);
                            continue;
                        }
                    }
                    return -1;
                }
            }
        }
        /* time to update internal state ? */
        time(&currts);
        deltatime = currts-lastts;
        if (deltatime >= 1) {
            gettunables(group->devname, 0, 0);
            eval_netspeed();
            savestats(0);
            lastts = currts;
        }
        /* time to update LRDBs ? */
        deltatime = currts-lastlrdb;
        if (deltatime >= 360) {
            update_lrdb(lgnum);
            lastlrdb = currts;
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
    rc = ioctl(group->devfd, FTD_MIGRATE, mig);
    if (0 > rc) {
        reporterr (ERRFAC, M_MIGRATE, ERRCRIT,
            group->devname, strerror(errno)); 
        return -1; 
    }
    group->offset -= (bytes/sizeof(int));
    DPRINTF((ERRFAC,LOG_INFO,"\n*** MIGRATE: %d bytes migrated\n",        mig->bytes));
    DPRINTF((ERRFAC,LOG_INFO,"\n*** MIGRATE: new BAB offset = %d\n",        group->offset));

    return 1;
} /* migrate */

