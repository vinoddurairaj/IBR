/*
 * ftd_journal.c - FTD journal interface 
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

#include "ftd_port.h"
#include "ftd_journal.h"
#include "ftd_lg.h"
#include "ftd_dev.h"
#include "ftd_sock.h"
#include "ftd_error.h"
#include "misc.h"

static int ftd_journal_file_name_is_cp(char *name);
static int ftd_journal_get_file_names(char paths[][MAXPATHLEN], char *srchpath, char *prefix);

/*
 * ftd_journal_create -- create a ftd_journal_t object
 */
ftd_journal_t *
ftd_journal_create(char *jrnpath, char *fileprefix)
{
	ftd_journal_t *jrnp;

	if ((jrnp = (ftd_journal_t*)calloc(1, sizeof(ftd_journal_t))) == NULL) {
		return NULL;
	}
	strncpy(jrnp->prefix, fileprefix, sizeof(jrnp->prefix));
	jrnp->prefix[sizeof(jrnp->prefix) - 1] = '\0';

	if (jrnpath) {
		jrnp->jrnpath = strdup(jrnpath);
	} else {
		free(jrnp);
		return NULL;
	}

	if ((jrnp->jrnlist = CreateLList(sizeof(ftd_journal_file_t*))) == NULL) {
		free(jrnp->jrnpath);
		free(jrnp);
		return NULL;
	}

	jrnp->magicvalue = FTDJRNMAGIC;

	return jrnp;
}

/*
 * ftd_journal_delete -- delete a ftd_journal_t object
 */
int
ftd_journal_delete(ftd_journal_t *jrnp)
{
	ftd_journal_file_t	**jrnfpp;

	if (jrnp && jrnp->magicvalue != FTDJRNMAGIC) {
		// not a valid journal object
		return 0;
	}

	if (jrnp->jrnlist) {
		ForEachLLElement(jrnp->jrnlist, jrnfpp) {
			ftd_journal_file_close(*jrnfpp);
		}
		FreeLList(jrnp->jrnlist);
	}

	if (jrnp->jrnpath) {
		free(jrnp->jrnpath);
	}

	free(jrnp);

	return 0;
}

/*
 * ftd_journal_add_to_list  -- add ftd_journal_file_t object to linked list 
 */
int
ftd_journal_add_to_list(LList *jrnlist, ftd_journal_file_t **jrnfpp)
{

	AddToTailLL(jrnlist, jrnfpp);

	return 0;
}

/*
 * ftd_journal_remove_to_list  -- remove ftd_journal_file_t object from linked list 
 */
int
ftd_journal_remove_from_list(LList *jrnlist, ftd_journal_file_t **jrnfpp)
{

	RemCurrOfLL(jrnlist, jrnfpp);

	return 0;
}

/*
 * ftd_journal_file_create -- create a ftd_journal_file_t object
 */
ftd_journal_file_t *
ftd_journal_file_create(ftd_journal_t *jrnp)
{
	ftd_journal_file_t *jrnfp;

	if ((jrnfp =
		(ftd_journal_file_t*)calloc(1, sizeof(ftd_journal_file_t))) == NULL) {
		return NULL;
	}
	jrnfp->chunksize = JRN_CHUNKSIZE;
	jrnfp->fd = INVALID_HANDLE_VALUE;
	
	jrnfp->locked_offset = 0;
	jrnfp->locked_length = 0;

	return jrnfp;
}

/*
 * ftd_journal_file_delete -- delete a ftd_journal_file_t object
 */
int
ftd_journal_file_delete(ftd_journal_t *jrnp, ftd_journal_file_t **jrnfpp)
{
	ftd_uint64_t	ret;
    char            filename[MAXPATHLEN];
	int				rc = 0;

	if ((*jrnfpp != NULL) && (*jrnfpp)->fd != INVALID_HANDLE_VALUE)
	{
#if defined(_WINDOWS)
		// Before closing mark file as read 
		ret = ftd_llseek((*jrnfpp)->fd, 0L, SEEK_SET);
		if (ret == (ftd_uint64_t)-1) {
			reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
				(*jrnfpp)->name,
				(*jrnfpp)->size,
				ftd_strerror());
			return -1;
		}

		SetEndOfFile((*jrnfpp)->fd);
#else
		ftruncate((*jrnfpp)->fd, 0);
#endif
	}
	
    // first unlink and close the file 
#if defined(_WINDOWS)
	sprintf(filename, "%s\\%s", jrnp->jrnpath, (*jrnfpp)->name);
#else
	sprintf(filename, "%s/%s", jrnp->jrnpath, (*jrnfpp)->name);
#endif
	// close the journal file object 
	ftd_journal_file_close((*jrnfpp));

	// test for apply complete 
#if defined(_WINDOWS)
	// unlink will fail on NT if opened by someone else (ie. rmd)  
    if (unlink(filename) == -1) {
#else
	// UNIX  
	{
		struct stat statbuf;

		stat(filename, &statbuf);

		if (statbuf.st_nlink > 1) {
#endif
			rc = 1;
		}
#if !defined(_WINDOWS)
		else {
			unlink(filename);
		}
	}
#endif

	// remove it from the journal file list 
	ftd_journal_remove_from_list(jrnp->jrnlist, jrnfpp);

	// clobber it 
	free((*jrnfpp));

	*jrnfpp = NULL;

	return rc;
}

/*
 * ftd_journal_get_cur_state --
 * get current state of FTD journal
 */
int
ftd_journal_get_cur_state(ftd_journal_t *jrnp, int prune_inco)
{
	ftd_journal_file_t	**jrnfpp, *jrnfp;
	int					jrncnt, state, mode, num, inco_flag, i;
	struct stat			statbuf;
	char				filename[MAXPATHLEN];

	// set default journal state 
	UNSET_JRN_CP_ON(jrnp->flags);
	UNSET_JRN_CP_PEND(jrnp->flags);

	SET_JRN_STATE(jrnp->flags, FTDJRNCO);
	SET_JRN_MODE(jrnp->flags, FTDMIRONLY);

	jrnp->cpnum = 0;
	inco_flag = 0;

	if ((jrncnt = ftd_journal_get_all_files(jrnp)) < 0) {
		return jrncnt;
	} else if (jrncnt > 0) {

		/* set checkpoint state */
		i = 0;
		ForEachLLElement(jrnp->jrnlist, jrnfpp) {
			jrnfp = *jrnfpp;
			if (ftd_journal_file_is_cp(jrnfp)) {
				if (i == 0) {
					SET_JRN_CP_ON(jrnp->flags);
				} else {
					SET_JRN_CP_PEND(jrnp->flags);
				}
				SET_JRN_MODE(jrnp->flags, FTDJRNONLY);
			}
			i++;
		}

		/* get current journal mode and state */
		i = 0;
		ForEachLLElement(jrnp->jrnlist, jrnfpp) {
			jrnfp = *jrnfpp;

			if (!ftd_journal_file_is_cp(jrnfp)) {
                
				ftd_journal_parse_name(jrnp->flags,
					jrnfp->name, &num, &state, &mode);
 
				if (state == FTDJRNINCO) {
					/* weed out inco journals - if we are asked to */
					if (prune_inco) {
						ftd_journal_file_delete(jrnp, jrnfpp);
						continue;
					} else {
						SET_JRN_STATE(jrnp->flags, state);
						SET_JRN_MODE(jrnp->flags, mode);
						inco_flag = 1;
						break;
					}
				} else {
#if defined(_WINDOWS)
					sprintf(filename, "%s\\%s", jrnp->jrnpath, jrnfp->name);
#else
					sprintf(filename, "%s/%s", jrnp->jrnpath, jrnfp->name);
#endif
					if (stat(filename, &statbuf) < 0) {
						continue;
					}
					// if file size == 0 then blow it away 
					if (statbuf.st_size == 0) {
						if (ftd_journal_file_delete(jrnp, jrnfpp) != 0) {
							reporterr(ERRFAC, M_JRNFILDEL, ERRCRIT,
								(*jrnfpp)->name, ftd_strerror());
							return -1;
						}
						continue;
					}

					/* coherent journal found - we know we */
					/* are in coherent journaling mode */
					SET_JRN_STATE(jrnp->flags, state);
					SET_JRN_MODE(jrnp->flags, mode);
					break;
				}
			}
			i++;
		}
	} 

	if (GET_JRN_CP_ON(jrnp->flags) || GET_JRN_CP_PEND(jrnp->flags)) {
		SET_JRN_MODE(jrnp->flags, FTDJRNONLY);
	}

	if (inco_flag) {
		/* INCO journals found - we were prev in an INCO state */
		SET_JRN_STATE(jrnp->flags, FTDJRNINCO);
		SET_JRN_MODE(jrnp->flags, FTDJRNONLY);
	}

	jrnp->cur = NULL;

	error_tracef( TRACEINF4, "ftd_journal_get_cur_state():Journal state: cpon, jrnmode, jrnstate = %d, %d, %d", GET_JRN_CP_ON(jrnp->flags), GET_JRN_MODE(jrnp->flags), GET_JRN_STATE(jrnp->flags) );

	return 0;
}

/*
 * ftd_journal_get_all_files --
 * get the current list of ftd_journal_file_t objects including .cp file 
 */
int
ftd_journal_get_all_files(ftd_journal_t *jrnp)
{
	ftd_journal_file_t	*jrnfp, **jrnfpp;
	int					found, jrncnt, i, lnum, lstate, lmode;
    char	JournalLog[JRN_MAXFILES][MAXPATHLEN];

	if (jrnp == NULL || jrnp->magicvalue != FTDJRNMAGIC) {
		// not a valid ftd journal
		error_tracef( TRACEERR, "ftd_journal_get_all_files():Journal invalid");
		return -1;
	}

	if ((jrncnt = ftd_journal_get_file_names(JournalLog,
		jrnp->jrnpath, jrnp->prefix)) < 0) {
		error_tracef( TRACEERR, "ftd_journal_get_all_files():Journal count error");
		return -1;
	}

	if ( jrncnt == JRN_MAXFILES ) {
		error_tracef( TRACEERR, "ftd_journal_get_all_files():Maximum number of journals");
		return -1;
	}

	// remove from list any deleted journal files
	ForEachLLElement(jrnp->jrnlist, jrnfpp) {
		// check for existence of file in list 
		found = 0;
		for (i = 0; i < jrncnt; i++) {
			if (strcmp((*jrnfpp)->name, JournalLog[i]) == 0) {
				found = 1;
				break;
			}
		}
		if (found) {
			continue;
		}
		ftd_journal_file_close(*jrnfpp);

		ftd_journal_remove_from_list(jrnp->jrnlist, jrnfpp);
	}

	// add to list any new journal files
	for (i = 0; i < jrncnt; i++) {
		if (ftd_journal_file_name_is_cp(JournalLog[i])) {
			ftd_journal_parse_name(jrnp->flags, JournalLog[i],
				&lnum, &lstate, &lmode);
			jrnp->cpnum = lnum;
		}
		found = 0;
		// check for existence of file in list 
		ForEachLLElement(jrnp->jrnlist, jrnfpp) {
			if (strcmp((*jrnfpp)->name, JournalLog[i]) == 0) {
				found = 1;
				break;
			}
		}
		if (found) {
			continue;
		}
		// add to list 
		if ((jrnfp = ftd_journal_file_create(jrnp)) == NULL) {
			return -1;
		}
		strcpy(jrnfp->name, JournalLog[i]);

		ftd_journal_add_to_list(jrnp->jrnlist, &jrnfp);
	}

	// first field in struct is journal name string
	SortLL(jrnp->jrnlist, stringcompare_addr);

	return SizeOfLL(jrnp->jrnlist);
}

/*
 * ftd_journal_co --
 * return true if any journal files are coherent
 */
int
ftd_journal_co(ftd_journal_t *jrnp)
{
	ftd_journal_file_t	**jrnfpp;
	int					num, state, mode, cnt;

	cnt = 0;
	ForEachLLElement(jrnp->jrnlist, jrnfpp) {
		ftd_journal_parse_name(jrnp->flags, (*jrnfpp)->name,
			&num, &state, &mode);
		if (state == FTDJRNCO) {
			cnt++;
		}
	}

	return cnt;
}

/*
 * ftd_journal_delete_all_files --
 * unlink all journal files in the journal
 */
int
ftd_journal_delete_all_files(ftd_journal_t *jrnp)
{
    ftd_journal_file_t	**jrnfpp;

    ForEachLLElement(jrnp->jrnlist, jrnfpp) {
        ftd_journal_file_delete(jrnp, jrnfpp);
    }

    return 0;
}

/*
 * ftd_journal_close_all_files --
 * close all journal files in the journal 
 */
int
ftd_journal_close_all_files(ftd_journal_t *jrnp)
{
    ftd_journal_file_t	**jrnfpp;

    ForEachLLElement(jrnp->jrnlist, jrnfpp) {
        ftd_journal_file_close((*jrnfpp));
    }

    return 0;
}

/*
 * ftd_journal_delete_inco_files --
 * unlink all incoherent journals for a group 
 */
int
ftd_journal_delete_inco_files(ftd_journal_t *jrnp)
{
    ftd_journal_file_t	**jrnfpp;
	int					lnum, lstate, lmode;

    ForEachLLElement(jrnp->jrnlist, jrnfpp) {
		ftd_journal_parse_name(jrnp->flags,
			(*jrnfpp)->name, &lnum, &lstate, &lmode);
		if (lstate != FTDJRNINCO) {
			continue;
		}
        ftd_journal_file_delete(jrnp, jrnfpp);
    }

    return 0;
}

/*
 * ftd_journal_set_state --
 * set state of journal 
 */
int
ftd_journal_set_state(ftd_journal_t *jrnp, int tstate)
{
	ftd_journal_file_t	**jrnfpp, *jrnfp;
	int				jrncnt, num, state, mode;
	char				path[MAXPATH], tpath[MAXPATH], *statec;


	if ((jrncnt = ftd_journal_get_all_files(jrnp)) <= 0) {
		return 0;
	}

	// XXX JRL is this the right place to put this?
	ftd_journal_close_all_files(jrnp);

	ForEachLLElement(jrnp->jrnlist, jrnfpp) {
		jrnfp = *jrnfpp;

		if (ftd_journal_file_is_cp(jrnfp)) {
			continue;
		}

		ftd_journal_parse_name(jrnp->flags,
			jrnfp->name, &num, &state, &mode); 

		if (state == tstate) {
			/* don't need to change - already there */
			continue;
		}
#if !defined(_WINDOWS)
		sprintf(path, "%s/%s", jrnp->jrnpath, jrnfp->name);
#else
		sprintf(path, "%s\\%s", jrnp->jrnpath, jrnfp->name);
#endif

		if (tstate == FTDJRNCO) {
			statec = "c";
		} else if (tstate == FTDJRNINCO) {
			statec = "i";
		}

#if !defined(_WINDOWS)
		sprintf(tpath, "%s/%s.%03d.%s", jrnp->jrnpath, jrnp->prefix, num, statec);
#else
		sprintf(tpath, "%s\\%s%04d.%s", jrnp->jrnpath, jrnp->prefix, num, statec);
#endif
		error_tracef( TRACEINF4, "ftd_journal_set_state():Lrename old to new = %s, %s", path, tpath );
		rename(path, tpath);
		//unlink(jrnfp->name);
	}

	SET_JRN_STATE(jrnp->flags, tstate);

	return 0;
}

/*
 * ftd_journal_file_seek --
 * seek to offset in journal file (header+data)
 */
ftd_uint64_t
ftd_journal_file_seek(ftd_journal_file_t *jrnfp, ftd_uint64_t offset, int whence)
{
    ftd_uint64_t ret;

    if ((ret = ftd_llseek(jrnfp->fd, offset, whence)) == (ftd_uint64_t)-1) {
        return -1;
    }

    return ret;
}

/*
 * ftd_journal_file_read --
 * read entry from journal (header+data)
 */
int
ftd_journal_file_read(ftd_journal_file_t *jrnfp, char *buf, int *buflen)
{
	ftd_header_t	header;
    int				readcnt = 0;

    if (ftd_read(jrnfp->fd, (char*)&header, sizeof(header)) == -1) {
		{
		ftd_uint64_t	off64 = ftd_llseek(jrnfp->fd, SEEK_CUR, 0);
		ftd_uint64_t	len64 = sizeof(header);

		reporterr(ERRFAC, M_READERR, ERRCRIT,
			jrnfp->name,
			off64,
			len64,
			ftd_strerror());
        return -1;
		}
    }
	if (header.len > (int)*buflen) {
		*buflen = header.len;
		buf = realloc(buf, *buflen);
	}
    if ((readcnt = ftd_read(jrnfp->fd, buf, header.len)) == -1) {
		{
		ftd_uint64_t	off64 = ftd_llseek(jrnfp->fd, SEEK_CUR, 0);
		ftd_uint64_t	len64 = header.len;

		reporterr(ERRFAC, M_READERR, ERRCRIT,
			jrnfp->name,
			off64,
			len64,
			ftd_strerror());
        return -1;
		}
    }

    return readcnt;
}

/*
 * ftd_journal_file_write --
 * write primary i/o to journal file (header+data)
 */
int
ftd_journal_file_write(ftd_journal_t *jrnp, int lgnum, int devid,
	int offset, int len, char *buf)
{
	ftd_journal_file_t	*jrnfp;
	ftd_lg_header_t		header;
	int					bytelen, length, rc;
	ftd_uint64_t		size_threshold, new_jrn_size;
	offset_t			ret;
	struct iovec		iov[2];

	//size_threshold = 100*1024*1024;
	size_threshold = FTD_MAX_JRN_SIZE;

	/* static header values */
	memset(&header, 0, sizeof(header));

	header.lgnum = lgnum;
	header.devid = devid;
	header.offset = offset;		/* blocks */
	header.len = len;			/* blocks */

	bytelen = (len << DEV_BSHIFT);

	jrnfp = jrnp->cur;

	/* if write will make journal > max journal size then */
	/* create new journal */
	new_jrn_size = sizeof(ftd_lg_header_t) + bytelen + jrnfp->size;

	if (new_jrn_size > size_threshold) {
    	/* close journal file */
		if (ftd_journal_file_close(jrnfp)) {
			return -1;
		}
		if ((jrnfp = ftd_journal_create_next(jrnp,
			GET_JRN_STATE(jrnp->flags))) == NULL) {
			return -1;
		}
	}

    ret = ftd_llseek(jrnfp->fd, 0L, SEEK_END);

	if (ret == (ftd_uint64_t)-1) {
		reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
			jrnfp->name,
			jrnfp->size,
			ftd_strerror());
		return -1;
	}

	iov[0].iov_base = (char*)&header;
	iov[0].iov_len = sizeof(ftd_lg_header_t);
	iov[1].iov_base = (char*)buf;
	iov[1].iov_len = bytelen;

	length = iov[0].iov_len + iov[1].iov_len;

	rc = ftd_writev(jrnfp->fd, iov, 2);
	if (rc != length) {
		if (ftd_errno() == ENOSPC) {
			return -2;
        } else {
			return -3;
        }
    }

	jrnfp->size += sizeof(ftd_lg_header_t) + bytelen;

    return 0;
}

/*
 * ftd_journal_delete_cp  --
 * delete checkpoint journal file from journal 
 */
int
ftd_journal_delete_cp(ftd_journal_t *jrnp)
{
	ftd_journal_file_t	**jrnfpp;

	/* clobber checkpoint file object */
	ForEachLLElement(jrnp->jrnlist, jrnfpp) {
		if (ftd_journal_file_is_cp((*jrnfpp))) {
			ftd_journal_file_delete(jrnp, jrnfpp);
			jrnp->cpnum = -1;
        }
 	}

	return 0;
}

/*
 * ftd_journal_create_next --
 * create the next-in-sequence-number-order ftd_journal_file object
 * for the journal 
 */
ftd_journal_file_t *
ftd_journal_create_next(ftd_journal_t *jrnp, int tstate)
{
	ftd_journal_file_t *jrnfp, **jrnfpp;
    ftd_jrnheader_t		jrnheader;
    char				*statestr;
    int					jrncnt, lnum, lstate, lmode, n; 

	lnum = 0;

	// we are the only process that 'creates' files - get the list
	jrncnt = ftd_journal_get_all_files(jrnp);

	if (jrncnt < 0) {
		return NULL;
	} else if (jrncnt > 0) {
		// get state of last journal file in the list 
		jrnfpp = TailOfLL(jrnp->jrnlist);

		ftd_journal_parse_name(jrnp->flags,
			(*jrnfpp)->name, &lnum, &lstate, &lmode);
	}

    if (lnum++ >= FTD_MAX_JRN_NUM) {
		// we're fucked - blindly roll back to 1 for now
		error_tracef( TRACEERR, "ftd_journal_create_next(): Roll Back to 1");
		lnum = 1;
	}

    if ((jrnfp = ftd_journal_file_create(jrnp)) == NULL) {
		error_tracef( TRACEERR, "ftd_journal_create_next(): File create error");
		return NULL;
	}
	statestr = tstate == FTDJRNCO ? "c": "i";
	
	// create the journal file name from the state info 
#if !defined(_WINDOWS)
    sprintf(jrnfp->name, "%s.%03d.%s", jrnp->prefix, lnum, statestr);
#else
    sprintf(jrnfp->name, "%s%04d.%s", jrnp->prefix, lnum, statestr);
#endif    
    // open the new journal file object for the journal 

	// TODO: don't we need to create this locked somehow ? 
	// if rmda is running it can clobber before we write the header 

	if ((ftd_journal_file_open(jrnp, jrnfp, O_WRONLY | O_CREAT, 0600)) == -1) {
		error_tracef( TRACEERR, "ftd_journal_create_next(): File open error");
		return NULL;
	}
	ftd_journal_parse_name(jrnfp->flags, jrnfp->name,
		&lnum, &lstate, &lmode);

	// write header to journal 
	jrnheader.magicnum = FTDJRNMAGIC;
	jrnheader.state = tstate;
	jrnheader.mode = lmode; 

	if (ftd_llseek(jrnfp->fd, (offset_t)0, SEEK_SET) == (ftd_uint64_t)-1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            jrnfp->name,
            0,
			ftd_strerror());
		return NULL;
	}
	n = sizeof(ftd_jrnheader_t);
	if (ftd_write(jrnfp->fd, (char*)&jrnheader, n) != n) {
		{
			ftd_uint64_t len64 = n;

			if (ftd_errno() == ENOSPC) {
				reporterr(ERRFAC, M_JRNSPACE, ERRCRIT, jrnp->jrnpath);
			} else {
				reporterr(ERRFAC, M_WRITEERR, ERRCRIT,
					jrnfp->name, 
					0,
					0,
					len64,
					ftd_strerror());
			}
			return NULL;
		}
	}
	ftd_fsync(jrnfp->fd);

	jrnfp->size = sizeof(ftd_jrnheader_t);
	jrnfp->offset = jrnfp->size = sizeof(ftd_jrnheader_t);
	jrnfp->chunksize = JRN_CHUNKSIZE;

	ftd_journal_add_to_list(jrnp->jrnlist, &jrnfp);

	// reset journal cur address to new file object address 
	jrnp->cur = jrnfp;

	// set journal state and mode 
	SET_JRN_STATE(jrnp->flags, lstate);
	SET_JRN_MODE(jrnp->flags, lmode);

    return jrnfp;
}

/*
 * ftd_journal_create_next_cp --
 * create the next-in-sequence-number-order ftd_journal_file object as a checkpoint marker 
 */
int
ftd_journal_create_next_cp(ftd_journal_t *jrnp, ftd_journal_file_t *jrnfp)
{
	ftd_journal_file_t	**ljrnfpp, *ljrnfp;
    int					jrncnt, lnum, lstate, lmode, i; 

	/* if checkpoint already on or pending then return */
	i = 0;
	ForEachLLElement(jrnp->jrnlist, ljrnfpp) {
		if (ftd_journal_file_is_cp((*ljrnfpp))) {
			if (i == 0) {
				/* checkpoint is currently on */
				return JRNCPONAGAIN;
			} else {
				/* checkpoint is pending */
				return JRNCPPENDING;
			}
		}
		i++;
	}

	jrncnt = ftd_journal_get_all_files(jrnp);

    lnum = 0;
    if (jrncnt > 0) {
        ljrnfpp = TailOfLL(jrnp->jrnlist);
        ftd_journal_parse_name(jrnp->flags,
            (*ljrnfpp)->name, &lnum, &lstate, &lmode);
    }
    lnum++;

	/* cp not on or pending - create the checkpoint journal file object */
	ljrnfp = ftd_journal_file_create(jrnp);
	
#if !defined(_WINDOWS)
	sprintf(ljrnfp->name, "%s.%03d.p", jrnp->prefix, lnum);
#else
	sprintf(ljrnfp->name, "%s%04d.p", jrnp->prefix, lnum);
#endif
	/* create a checkpoint journal file object */
	// XXX JRL why dont we open this write/create?
	// TM - because we never write to it
	if ((ftd_journal_file_open(jrnp, ljrnfp, O_CREAT, 0600)) == -1) {
		return -1;
	}

	ftd_journal_file_close(ljrnfp);

	ftd_journal_add_to_list(jrnp->jrnlist, &ljrnfp);

	jrnfp = ljrnfp;

	return 0;
}

/*
 * ftd_journal_file_open --
 * open ftd_journal_file_t object 
 */
int
ftd_journal_file_open(ftd_journal_t *jrnp,
	ftd_journal_file_t *jrnfp, int mode, int permissions)
{
	char	filename[MAXPATHLEN];

#if defined(_WINDOWS)
	sprintf(filename, "%s\\%s", jrnp->jrnpath, jrnfp->name);
#else
	sprintf(filename, "%s/%s", jrnp->jrnpath, jrnfp->name);
#endif    
	if ((jrnfp->fd = ftd_open(filename, mode, permissions))
		== INVALID_HANDLE_VALUE) {
        reporterr(ERRFAC, M_JRNOPEN, ERRCRIT, filename, ftd_strerror());
		return -1;
	}

    return 0;
}

/*
 * ftd_journal_file_close -- close ftd_journal_file_t object 
 */
int
ftd_journal_file_close(ftd_journal_file_t *jrnfp)
{

    if (jrnfp && (jrnfp->fd != INVALID_HANDLE_VALUE) ) {

		if (GET_JRN_LOCK(jrnfp->flags)) {
			// unlock before closing
#if defined(_WINDOWS)
			ftd_journal_file_unlock(jrnfp,
				jrnfp->locked_offset, jrnfp->locked_length);
#else
			ftd_journal_file_unlock(jrnfp,
				jrnfp->locked_offset + jrnfp->locked_length,
				~(jrnfp->locked_length)); 
#endif
		}

		ftd_close(jrnfp->fd);

		jrnfp->fd = INVALID_HANDLE_VALUE;
	}

	return 0;
}

/*
 * ftd_journal_file_lock -- lock a ftd_journal_file_t object
 */
int
ftd_journal_file_lock_header
(ftd_journal_file_t *jrnfp) 
{
    ftd_uint64_t	ret;
    int				tries, rc, i;

	// lock down the header 
    ret = ftd_llseek(jrnfp->fd, 0L, SEEK_SET);
    if (ret == (ftd_uint64_t)-1) {
		reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
			jrnfp->name,
			0,
			ftd_strerror());
        return -1;
    }
    tries = 30;
    for (i=0;i<tries;i++) {
        //DPRINTF((ERRFAC,LOG_INFO," [%d] locking journal header: %s, offset, length = %d, %d\n",              getpid(), jrnfp->name, 0, sizeof(ftd_jrnheader_t) - 1));
        if ((rc = ftd_lockf(jrnfp->fd, F_TLOCK, 0L, sizeof(ftd_jrnheader_t) - 1)) == 0) {
            break;
        } else {
#if defined(_WINDOWS)
			DWORD err;
			err = GetLastError();
#endif
            sleep(1);
        }
    }
    if (i == tries) {
        return -1;
    }

	return 0;
}


/*
 * ftd_journal_file_lock -- lock a ftd_journal_file_t object
 */
int
ftd_journal_file_lock
(ftd_journal_file_t *jrnfp, ftd_uint64_t offset, ftd_uint64_t lock_len) 
{
    ftd_uint64_t	ret;
    int				rc;

    if (GET_JRN_LOCK(jrnfp->flags)) {
        /* journal file already locked by caller. unlock it */
        if (ftd_journal_file_unlock(jrnfp,
			jrnfp->locked_offset, jrnfp->locked_length) == -1) {
			reporterr(ERRFAC, M_JRNUNLOCK, ERRWARN,
				jrnfp->name, ftd_strerror());
            return -1;
        } 
		UNSET_JRN_LOCK(jrnfp->flags);
    }

	if ( ftd_journal_file_lock_header(jrnfp) == -1)
		return -1;

    /* ok, now lock the section */
	ret = ftd_llseek(jrnfp->fd, offset, SEEK_SET);
    if (ret == (ftd_uint64_t)-1) {
		reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
			jrnfp->name,
			offset,
			ftd_strerror());
        return -1;
    }
    if ((rc = ftd_lockf(jrnfp->fd, F_TLOCK, offset, lock_len)) == -1) {
        reporterr(ERRFAC, "JRNLOCK", ERRWARN,
			jrnfp->name, ftd_strerror());
		return -1;
    }
    //DPRINTF((ERRFAC,LOG_INFO,        " [%d] aquired lock on journal: %s [%llu-(%llu)]\n",         getpid(), jrnfp->name, offset, lock_len));

	jrnfp->locked_offset = offset;
	jrnfp->locked_length = lock_len;

    return 0;
}

/*
 * ftd_journal_file_unlock -- unlock a ftd_journal_file_t object
 */
int
ftd_journal_file_unlock_header
(ftd_journal_file_t *jrnfp) 
{
    ftd_uint64_t	ret;
	char			*name;

    ret = ftd_llseek(jrnfp->fd, 0L, SEEK_SET);
    if (ret != 0) {
		if (jrnfp && strlen(jrnfp->name)) {
			name = jrnfp->name;
		} else {
			name = "";
		}
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
			name, 0, ftd_strerror());
        return -1;
    }

    if (ftd_lockf(jrnfp->fd, F_ULOCK, 0L, sizeof(ftd_jrnheader_t) - 1) != 0) {
#if defined(_WINDOWS)
		DWORD err;
		err = GetLastError();
#endif
        return -1;
    }

	return 0;
}

/*
 * ftd_journal_file_unlock -- unlock a ftd_journal_file_t object
 */
int
ftd_journal_file_unlock
(ftd_journal_file_t *jrnfp, ftd_uint64_t offset, ftd_int64_t lock_len) 
{
    ftd_uint64_t	ret;
	char			*name;

    ret = ftd_llseek(jrnfp->fd, offset, SEEK_SET);
    if (ret != offset) {
		if (jrnfp && strlen(jrnfp->name)) {
			name = jrnfp->name;
		} else {
			name = "";
		}
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
			name, offset, ftd_strerror());
        return -1;
    }
    if (ftd_lockf(jrnfp->fd, F_ULOCK, offset, lock_len) != 0) {
#if defined(_WINDOWS)
		DWORD err;
		err = GetLastError();
#endif
        return -1;
    }

    if (ftd_journal_file_unlock_header(jrnfp) == -1)
		return -1;

	//DPRINTF((ERRFAC,LOG_INFO,        " [%d] released lock on journal: %s [%llu-(%lld)]\n",         getpid(), jrnfp->name, offset, lock_len));

	jrnfp->locked_offset = 0;
	jrnfp->locked_length = 0;

    return 0;
}

/*
 * ftd_journal_parse_name  --
 * parse the components from journal name 
 */
int
ftd_journal_parse_name(int flags, char *name, int *num, int *state, int *mode)
{
    char	*lname, state_c;
    int		byte, lnum, len, lstate, lmode;

    lname = strdup(name);
    len = strlen(lname);
    
	state_c = lname[len-1];

    byte = len-6; /* this kluge takes us to the numeric part of the name */
    sscanf(&lname[byte], "%04d", &lnum);
   
    //DPRINTF((ERRFAC,LOG_INFO," journal_parse_name: name = %s\n", name));

	lstate = GET_JRN_STATE(flags);
    lmode = GET_JRN_MODE(flags);
	
    if (state_c == 'i') {
        *state = FTDJRNINCO;
        *mode = FTDJRNONLY;
        *num = lnum;
    } else if (state_c == 'c') {
        *state = FTDJRNCO;
        *mode = GET_JRN_CP_ON(flags) ? FTDJRNONLY: FTDJRNMIR;
        *num = lnum;
    } else if (state_c == 'p') {
        *num = lnum;
        *state = lstate ? lstate: FTDJRNCO;
        *mode = lmode ? lmode: FTDMIRONLY;
    } else {
        /* error */
        *state = FTDJRNINCO;
        *mode = FTDMIRONLY;
        *num = -1;
    }
    free(lname);

    return 0;
}

/*
 * ftd_journal_file_is_cp --
 * is the target journal file a checkpoint journal file ?
 */
static int
ftd_journal_file_is_cp(ftd_journal_file_t *jrnfp)
{
    int len;
 
    if ((len = strlen(jrnfp->name))) {
        if (jrnfp->name[len-1] == CP_SUFFIX) {
            return 1;
        }
    }

    return 0;
}

/*
 * ftd_journal_file_is_co --
 * is the target journal file a coherent journal file ?
 */
int
ftd_journal_file_is_co(ftd_journal_file_t *jrnfp)
{
    int len;
 
    if ((len = strlen(jrnfp->name))) {
        if (jrnfp->name[len-1] == CO_SUFFIX) {
            return 1;
        }
    }

    return 0;
}

/*
 * ftd_journal_file_is_inco --
 * is the target journal file a incoherent journal file ?
 */
int
ftd_journal_file_is_inco(ftd_journal_file_t *jrnfp)
{
    int len;
 
    if ((len = strlen(jrnfp->name))) {
        if (jrnfp->name[len-1] == INCO_SUFFIX) {
            return 1;
        }
    }

    return 0;
}

/*
 * ftd_journal_file_name_is_cp --
 * is the target journal file name a checkpoint journal file name ?
 */
static int
ftd_journal_file_name_is_cp(char *name)
{
    int len;
 
    if ((len = strlen(name))) {
        if (name[len-1] == CP_SUFFIX) {
            return 1;
        }
    }

    return 0;
}

/*
 * ftd_journal_get_file_names --
 * get the current sorted list of FTD journal file names 
 */
#ifdef _WINDOWS
static int
ftd_journal_get_file_names(char paths[][MAXPATHLEN], char *srchpath, char *prefix)
{
    HANDLE			hFile;
    WIN32_FIND_DATA	data;
    BOOL			bFound = TRUE;
	char			srchstr[MAXPATH];
	int				jrncnt, prefix_len;

	sprintf(srchstr, "%s\\%s*", srchpath, prefix);
    
	hFile = FindFirstFile(srchstr, &data);    
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

	jrncnt = 0;
	prefix_len = strlen(prefix);
    while (bFound) {
        if (memcmp(data.cFileName, prefix, prefix_len)) {
            continue;
        }
        strcpy(paths[jrncnt++], data.cFileName);
        bFound = FindNextFile(hFile, &data); 
    }
    FindClose(hFile);
    qsort((void*)paths, (size_t)jrncnt, MAXPATHLEN, stringcompare_const);
    
	return jrncnt;
}

#else /* _WINDOWS */

static int
ftd_journal_get_file_names
(char paths[][MAXPATHLEN], char *srchpath, char *prefix)
{
	DIR				*dfd;
	struct dirent	*dent;
	int				jrncnt, prefix_len;

	/* read journal directory - compile a sorted list of journals */
	if ((dfd = opendir(srchpath)) == NULL) {
		reporterr(ERRFAC, M_JRNPATH, ERRCRIT,
			srchpath, ftd_strerror());
		return -1;
	}
	prefix_len = strlen(prefix);
    jrncnt = 0;
    while (NULL != (dent = readdir(dfd))) {
        if (memcmp(dent->d_name, prefix, prefix_len)) {
            continue;
        }
        strcpy(paths[jrncnt++], dent->d_name);
    }
    (void)closedir(dfd);
    qsort((char*)paths, jrncnt, MAXPATHLEN, stringcompare_const);

    return jrncnt;
}

#endif



