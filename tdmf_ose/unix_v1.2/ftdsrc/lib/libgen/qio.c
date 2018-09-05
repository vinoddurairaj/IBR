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
 * qio.c - FullTime Data 
 *
 * (c) Copyright 1998 FullTime Software, Inc.
 *     All Rights Reserved
 *
 * This module implements the functionality for queued I/O
 *
 * NOTE: this module no longer needed. It was used when daemon were MT
 *       this code needs to go somewhere else.
 ***************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "config.h"
#include "network.h"
#include "errors.h"
#include "common.h"
#include "aixcmn.h"

#define QUEUESIZE 4 

extern char *argv0;

ioqueue_t *q_init (void);
void q_delete (ioqueue_t*);

int q_put_syncbfd(ioqueue_t*);
int q_put_syncrfd(ioqueue_t*, headpack_t*);
int q_put_datarfdf(ioqueue_t*, headpack_t*);
int q_put_databfd(ioqueue_t*, headpack_t*, char*);
int q_put_data(ioqueue_t*, headpack_t*);

extern u_longlong_t _rmd_jrn_size;
extern int _rmd_jrn_mode;
extern int _rmd_jrn_state;
extern int _rmd_rfddone;
extern int _rmd_cpon;
extern int net_writable;

static char *qbuf = NULL;
static int qlen = 0;

static char *qcbuf = NULL;
static int qclen = 0;

static char *zerobuf = NULL;
static int zerolen = 0;

int msg_server(qentry_t*);

extern int flush_delta(void);
extern sddisk_t *get_lg_dev(group_t*, int);
extern EntryFunc write_mirror;
extern EntryFunc write_journal;

extern struct iovec *iov;
extern int iovlen;

extern float compr_ratio;

// TODO A very heavy solution for just testing if journals exist */
ftd_jrnpaths_t tmppaths;
ftd_jrnpaths_t *tjrnphp = &tmppaths;

/*
 * msg_server -- RMD - send msg to PMD and/or perform local I/O
 */
int
msg_server (qentry_t *entry)
{
    wlheader_t wlentry[1];
    EntryFunc *entryfunc;
    group_t *group;
    headpack_t header[1];
    ackpack_t ack[1]; 
    headpack_t *lheader;
    rsync_t *rsync;
    rsync_t *lrsync;
    sddisk_t *sd;
    char msg[256];
    char fmt[256];
    u_longlong_t offset;
    u_longlong_t length;
    u_longlong_t vsize;
    int iovcnt = 0;
    int digestlen;
    int ldevid;
    int lgnum;
    int rc;
    int i;
    int j;
	int wlen;
    rsync32_t rsync32array[1024];
    int count = 0;
    int newsize = 0;
    char *newchunk = NULL;

    group = mysys->group;

    memset(header, 0, sizeof(headpack_t));
    header->ackwanted = 1;

    memset(ack, 0, sizeof(ackpack_t));
    lgnum = cfgpathtonum(mysys->configpath);

    /* Sequencer : write back the lgsn into ackpack_t object */
    if (entry->data != NULL)
    {
        ack->lgsn = ((headpack_t *)entry->data)->lgsn;
        ftd_trace_flow(FTD_DBG_SEQCER, "msg_server acking entry->type = %d lgsn = %d\n", entry->type, ack->lgsn);
    }
    else
    {
        ftd_trace_flow(FTD_DBG_SEQCER, "msg_server entry->data == NULL %d\n", entry->type);
    }
    
    memset(rsync32array, 0, 1024*sizeof(rsync32_t));
    /* dispatch work */
    switch(entry->type) {
    case Q_RSYNCBFD:
        if ((rc = chksumdiff()) == -1) {
            return -1;
        }
        if (net_writable) {
            flush_delta();
        }
        ack->acktype = ACKNORM;
        ack->data = ACKBFD;
        for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
            rsync = &sd->ackrsync;
            /* shove the devid in */
            rsync->devid = sd->rsync.devid;
            if (rsync->cs.seglen == -1) {
                continue;
            }
            if (++iovcnt > iovlen) {
                iovlen = (iovlen == 0 ? 1: iovlen*2);
                iov = (struct iovec*)
                    ftdrealloc((char*)iov, iovlen*sizeof(struct iovec));
            }
	    if (!pmd64)
            {
                convertrsyncfrom64to32 (rsync, &rsync32array[count]);
                iov[iovcnt-1].iov_base = (void*)&rsync32array[count];
                iov[iovcnt-1].iov_len = sizeof(rsync32_t);
                count++;
            }
	    else
            {
            	iov[iovcnt-1].iov_base = (void*)rsync;
            	iov[iovcnt-1].iov_len = sizeof(rsync_t);
	    }
        }
        ack->ackoff = iovcnt;
        break;
    case Q_RSYNCRFD:
        ack->acktype = ACKNORM;
        ack->data = ACKRFDCHKSUM;

        for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
            rsync = &sd->rsync;
            if (rsync->cs.seglen == 0) {
                continue;
            }
            if (++iovcnt > iovlen) {
                iovlen = (iovlen == 0 ? 1: iovlen*2);
                iov = (struct iovec*)
                    ftdrealloc((char*)iov, iovlen*sizeof(struct iovec));
            }
	    if (!pmd64)
            {
                convertrsyncfrom64to32 (rsync, &rsync32array[count]);
                iov[iovcnt-1].iov_base = (void*)&rsync32array[count];
                iov[iovcnt-1].iov_len = sizeof(rsync32_t);
                count++;
            }
            else
            {
            	iov[iovcnt-1].iov_base = (void*)rsync;
            	iov[iovcnt-1].iov_len = sizeof(rsync_t);
            }
            if (++iovcnt > iovlen) {
                iovlen = (iovlen == 0 ? 1: iovlen*2);
                iov = (struct iovec*)
                    ftdrealloc((char*)iov, iovlen*sizeof(struct iovec));
            }
            digestlen = rsync->cs.cnt*rsync->cs.num*DIGESTSIZE;
            iov[iovcnt-1].iov_base = (void*)rsync->cs.digest;
            iov[iovcnt-1].iov_len = digestlen;
        }
        if (iovcnt > 0) {
            ack->ackoff = iovcnt/2;
        }
        break;
    case Q_WRITE:
        lheader = (headpack_t*)entry->data;
        header->devid = ldevid = lheader->devid;
        header->ts = lheader->ts;
        header->ackwanted = lheader->ackwanted;
        header->len = lheader->len;
        header->decodelen = lheader->decodelen;
        header->data = lheader->data;
        ack->bab_oflow = header->bab_oflow = lheader->bab_oflow;   /* WR 43926 */

        ftd_trace_flow(FTD_DBG_IO,
            "Q_WRITE(_jrn_mode=%d): header->len = %d, header->decodelen = %d\n",
            _rmd_jrn_mode, header->len, header->decodelen);

        if (_rmd_jrn_mode == JRN_AND_MIR && get_journals(tjrnphp, 1, 1) <= 0) { 
            /* no more journals - go back to mirroring */
            stop_rmdapply();
            closedevs(-1);
                /* re-open devices EXCL */
            if (verify_rmd_entries(mysys->group,
                mysys->configpath, 1) == -1) {
                EXIT(EXITANDDIE);
            }
            _rmd_jrn_state = JRN_CO;
            _rmd_jrn_mode = MIR_ONLY;
        }
        /* assign entry function appropriately */
        entryfunc = _rmd_jrn_mode == MIR_ONLY ? write_mirror: write_journal;

        /* break chunk up and write to mirrors */
	if (!pmd64)
	{
		convertwlhfrom32to64 (lheader, &newsize, newchunk);
        	if (traverse_chunk((char*)lheader->data,
            		&newsize, entryfunc) == -1) {
            		EXIT(EXITANDDIE);
        	}
	}
	else
	{
        	if (traverse_chunk((char*)lheader->data,
            		(int *)&lheader->len, entryfunc) == -1) {
            		EXIT(EXITANDDIE);
        	}
	}
	/*
	 * taking adavantage of big chunk size, RMD write throughput may
	 * be improved with an async open mode.
	 */
        if (_rmd_jrn_mode == MIR_ONLY)
            syncgroup(0);

       	ack->ackoff = lheader->len;	
        ack->acktype = ACKNORM;
        ack->data = ACKCHUNK;
        ack->mirco = _rmd_rfddone;
        ack->cpon = _rmd_cpon;
        break;
    case Q_WRITERFDF:
        /* full, dumb refresh - write blocks directly to mirror */
        lheader = (headpack_t*)entry->data;

        ldevid = lheader->devid;
        offset = lheader->offset;
        offset <<= DEV_BSHIFT;

        header->devid = ldevid;
        header->ts = lheader->ts;
        header->ackwanted = lheader->ackwanted;

        if ((sd = get_lg_dev(group, ldevid)) == NULL) {
            reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, ldevid);
            /* BADDEVID senderr */
            return -1;
        }
        vsize = sd->mirsize * (u_longlong_t) DEV_BSIZE;
        wlen = lheader->len;

        /* write to mirror */
        if (offset == 0 && sd->no0write) {
            ftd_trace_flow(FTD_DBG_IO,
                    "skipping write to sector 0 for device: %s\n", sd->mirname);
            offset += DEV_BSIZE;
            lheader->data += DEV_BSIZE;
            wlen -= DEV_BSIZE;
        }
        /* don't write zero length data or data beyond end of device */
		/* Also, if this is a Full Refresh simulation, do not write the data to disk */
        if( (wlen > 0 && offset < vsize) && (!_net_bandwidth_analysis) ) {
            if (llseek(sd->mirfd, offset, SEEK_SET) == -1) {
                geterrmsg(ERRFAC, M_SEEKERR, msg);
                sprintf(msg, fmt, argv0, sd->mirname, offset,  
                    strerror(errno));
                logerrmsg(ERRFAC, ERRCRIT, M_SEEKERR, msg);
                (void) senderr(mysys->sock, header, 0L, ERRCRIT,
                    M_SEEKERR, msg);
                sleep(5);
                EXIT(EXITANDDIE);
            }
            /* truncate writes that extend beyond end of device */
            if ((((u_longlong_t) wlen) + offset) > vsize) {
                wlen = (int) (vsize - offset);
            }
            rc = write(sd->mirfd, page_aligned((char *)lheader->data, wlen), wlen);
            if (rc != wlen) {
                int err = errno;
                geterrmsg(ERRFAC, M_WRITEERR, fmt);
                length = wlen; 
                sprintf(msg, fmt, argv0, sd->mirname, sd->devid,
                    offset, length, get_error_str(err));
                logerrmsg(ERRFAC, ERRCRIT, M_WRITEERR, msg);
                (void) senderr(mysys->sock, header, 0L, ERRCRIT,
                    M_WRITEERR, msg);
                sleep(5);
                EXIT(EXITANDDIE);
            }
        }
        ack->ackoff = lheader->len >> DEV_BSHIFT;
        ack->write_ackoff = lheader->offset;
        ack->acktype = ACKNORM;
        ack->data = ACKRFDF;
        ack->devid = ldevid;
        break;
    case Q_WRITEHRDB:
        /* highres bitmap buffer - write blocks to journal */
        lheader = (headpack_t*)entry->data;
        ldevid = lheader->devid;
        header->devid = ldevid;
        header->ts = lheader->ts;
        header->ackwanted = lheader->ackwanted;

        if ((sd = get_lg_dev(group, ldevid)) == NULL) {
            reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, ldevid);
            /* BADDEVID senderr */
            return -1;
        }

        /* build a wlentry from header */
        memset(wlentry, 0, sizeof(wlheader_t));
        wlentry->offset = lheader->offset;
        wlentry->length = lheader->len >> DEV_BSHIFT;
        wlentry->diskdev = sd->dd_rdev;
        
        /* write to journal, unless it is a simulation */
		if( !_net_bandwidth_analysis )
		{
	        if (write_journal(wlentry, (char*)lheader->data) < 0)
	            return -1;
		}

        ack->ackoff = lheader->len >> DEV_BSHIFT;
        ack->write_ackoff = lheader->offset;
        ack->acktype = ACKNORM;
        ack->data = ACKRFD;
        ack->devid = ldevid;
        break;
    default:
        break;
    }
    /* send ACK */
    if (header->ackwanted) {
        if (sendack(mysys->sock, header, ack) == -1) {
            return -1;
        }
    }
    /* send required supplemental data */
    switch(entry->type) {
    case Q_RSYNCRFD:
    case Q_RSYNCBFD:
        if (iovcnt) {
            /* RMD - socket is blocking */
            rc = net_write_vector(mysys->sock, iov, iovcnt, ROLESECONDARY);
            if (rc == -1) {
                return -1;
            }
        }
        break;
    default:
        break;
    }
    return 0;
} /* msg_server */

/*
 * q_init -- q constructor 
 */
ioqueue_t *
q_init (void)
{
    ioqueue_t *q;

    q = (ioqueue_t*)ftdmalloc(sizeof(ioqueue_t));
    q->entry = (qentry_t*)ftdcalloc(QUEUESIZE, sizeof(qentry_t));
    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    return (q);
} /* q_init */

/*
 * q_delete -- q destructor 
 */
/*
 * WR 36420:: memory leak in pmd code, up to 500MB each used 
 * after 50 hours bab oflows
 */
void
q_delete (ioqueue_t *q)
{
    if (q) {
        if (q->entry) {
            if(q->entry->data) {
                ftdfree((char *)q->entry->data);
                q->entry->data = NULL;
            }
            ftdfree((char *)q->entry);
            q->entry = NULL;
        }
        free(q);
	q = NULL;
    }
} /* q_delete */

/*
 * q_put_datarfdf -- insert a full-refresh packet into the queue 
 */
int
q_put_datarfdf(ioqueue_t *fifo, headpack_t *header)
{
    sddisk_t *sd;
    qentry_t entry[1];
    int length;
    int len;

    if (header->data == BLOCKALLZERO) {
        ftd_trace_flow(FTD_DBG_IO,
                "block contains all zero: offset: %d, length: %d\n",
                header->offset, header->len);
        if (header->len > zerolen) {
            zerolen = header->len;
            ftdfree(zerobuf);
            zerobuf = (char*)ftdmalloc(zerolen);
            memset(zerobuf, 0, zerolen);
        }
        header->data = (u_long)zerobuf;
    } else {
        if ((sd = get_lg_dev(mysys->group, header->devid)) == NULL) {
            reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, header->devid);
            return -1;
        }
        if ((length = header->len) > qlen) {
            qlen = length;
            ftdfree(qbuf);
            qbuf = ftdmalloc(qlen);
        }
        /* read remote data from net */
        if (1 != readsock(mysys->sock, (char*)qbuf, length)) {
            return -1;
        } 
        header->data = (u_long)qbuf;
        if (header->compress) {
            /* decompress the data */
            len = header->len > header->decodelen ? header->len:
                header->decodelen;
            if ((length = len) > qclen) {
                /* -- toss in a little malloc fudge factor if overwrite */
                if (qclen != 0) {
                    ftdfree(qcbuf);
                }
                qcbuf = ftdmalloc(length+sizeof(long));
                qclen = length;
            }
            ftd_trace_flow(FTD_DBG_IO, "compr len = %d\n", header->len);
            ftd_trace_flow(FTD_DBG_IO, "decode len = %d\n", header->decodelen);
            header->len =
                decompress((u_char*)header->data,
                    (u_char*)qcbuf, header->len);
            ftd_trace_flow(FTD_DBG_IO, "de-compr len = %d\n", header->len);
            header->data = (u_long)qcbuf;
            if (header->decodelen) {
                compr_ratio = (float)((float)header->len / (float)header->decodelen);
            } else {
                compr_ratio = 1;
            }
        }
        sd->stat.a_tdatacnt += length;
        sd->stat.e_tdatacnt += length;
        sd->stat.a_datacnt += length;
        sd->stat.e_datacnt += length;
    } 
    if (_rmd_jrn_mode == MIR_ONLY) {
        /* writing to mirror directly */
        entry->type = Q_WRITERFDF;
    } else {
        /* writing to journal */
        entry->type = Q_WRITEHRDB;
    }

    entry->data = (char*)header;
    return msg_server(entry);
} /* q_put_datarfdf */

/*
 * q_put_data -- insert a journal chunk into the queue 
 */
int
q_put_data(ioqueue_t *fifo, headpack_t *header)
{
    headpack_t lheader[1];
    qentry_t entry[1];
    int length;
    int len;

    memcpy(lheader, header, sizeof(headpack_t));

    if (lheader->data == BLOCKALLZERO) {
        ftd_trace_flow(FTD_DBG_IO,
                "block all zero: offset: %d, length: %d\n",
                lheader->offset, lheader->len);
        if (lheader->len > zerolen) {
            zerolen = header->len;
            ftdfree(zerobuf);
            zerobuf = (char*)ftdmalloc(zerolen);
            memset(zerobuf, 0, zerolen);
        }
        entry->type = Q_WRITE;
        lheader->data = (u_long)zerobuf;
    } else {
        entry->type = Q_WRITE;
        if ((length = lheader->len) > qlen) {
            qlen = length;
            ftdfree(qbuf);
            qbuf = ftdmalloc(qlen);
        } 
        /* read remote data from net */
        if (1 != readsock(mysys->sock, qbuf, length)) {
            return -1;
        } 
        if (lheader->compress) {
            /* decompress the data */
            len = lheader->len > lheader->decodelen ? lheader->len:
                lheader->decodelen;
            if ((length = len) > qclen) {
                /* -- toss in a little malloc fudge factor if overwrite */
                if (qclen != 0) {
                    ftdfree(qcbuf);
                }
                qcbuf = ftdmalloc(length+sizeof(long));
                qclen = length;
            }
            ftd_trace_flow(FTD_DBG_IO, "compr len = %d\n", lheader->len);
            ftd_trace_flow(FTD_DBG_IO, "decode len = %d\n", lheader->decodelen);
            memset(qcbuf, 0, qclen);
            lheader->len =
                decompress((u_char*)qbuf, (u_char*)qcbuf, lheader->len);
            ftd_trace_flow(FTD_DBG_IO, "de-compr len = %d\n", lheader->len);
            if (lheader->decodelen) {
                compr_ratio =
                (float)((float)lheader->len/(float)lheader->decodelen);
            } else {
                compr_ratio = 1;
            }
            lheader->data = (u_long)qcbuf;
        } else {
            lheader->data = (u_long)qbuf;
        }
    }
    entry->data = (char*)lheader;
    return msg_server(entry);
} /* q_put_data */

/*
 * q_put_databfd -- insert a backfresh packet into the queue 
 */
int
q_put_databfd(ioqueue_t *fifo, headpack_t *header, char *data)
{
    qentry_t entry[1];

    entry->type = Q_WRITEBFD;
    entry->data = (char*)header;
    return msg_server(entry);
} /* q_put_databfd */

/*
 * q_put_syncrfd -- insert an rsync entry into the queue 
 */
int
q_put_syncrfd(ioqueue_t *fifo, headpack_t *header)
{
    qentry_t entry[1];

    entry->type = Q_RSYNCRFD;
    entry->data = (char*)header;
    return msg_server(entry);
} /* q_put_syncrfd */

/*
 * q_put_syncbfd -- insert an rsync entry into the queue 
 */
int
q_put_syncbfd(ioqueue_t *fifo)
{
    qentry_t entry[1];

    entry->type = Q_RSYNCBFD;
    entry->data = NULL;
    return msg_server(entry);
} /* q_put_syncbfd */
