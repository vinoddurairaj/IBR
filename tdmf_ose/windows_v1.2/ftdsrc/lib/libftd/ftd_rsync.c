/*
 * ftd_rsync.c - FTD rsync interface 
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
#include "ftd_lg.h"
#include "ftd_dev.h"
#include "ftd_sock.h"
#include "ftd_rsync.h"
#include "ftd_lg_states.h"
#include "ftd_ioctl.h"
#include "ftd_stat.h"
#include "ftd_error.h"
#include "md5const.h"

static int ftd_rsync_process_seg(ftd_lg_t *lgp, ftd_dev_t *devp);
static int ftd_rsync_dirtyseg(unsigned char *mask, ftd_uint64_t offset,
							  ftd_uint64_t length, int res);

/*
 * ftd_rsync_flush_delta --
 * flush the compiled list of deltas to the network 
 */
int
ftd_rsync_flush_delta(ftd_sock_t *fsockp, ftd_lg_t *lgp, ftd_dev_t *devp)
{
	ftd_dev_t		ldevp;
	ftd_dev_cfg_t	*devcfgp;
	ftd_dev_delta_t	*dp;
	char			*devname;
	ftd_uint64_t	off64;
	int				rc;

	if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, devp->devid)) == NULL) {
		reporterr(ERRFAC, M_PROTDEV, ERRCRIT, devp->devid);
		return -1;
	}

	if (lgp->cfgp->role == ROLEPRIMARY) {
		devname = devcfgp->pdevname;
	} else {
		devname = devcfgp->sdevname;
	}

	memcpy(&ldevp, devp, sizeof(ftd_dev_t));

	ForEachLLElement(devp->deltamap, dp) {
		// send local data to peer 
		ldevp.rsyncoff = dp->offset;
		ldevp.rsynclen = dp->length;
		ldevp.rsyncbytelen = ldevp.rsynclen << DEV_BSHIFT;
		
		ldevp.rsyncbuf = lgp->buf;

		off64 = ldevp.rsyncoff;
		off64 <<= DEV_BSHIFT;

		if (ftd_llseek(ldevp.devfd, off64, SEEK_SET) == (ftd_uint64_t)-1) {
			reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
				devname,
				ldevp.rsyncoff,
				ftd_strerror());
			return -1;
		}
		if (ftd_read(ldevp.devfd, ldevp.rsyncbuf, ldevp.rsyncbytelen) == -1) {
			reporterr(ERRFAC, M_READERR, ERRCRIT,
				devname,
				off64,
				ldevp.rsyncbytelen,
				ftd_strerror());
			return -1;
		}
		
		if ((rc = ftd_sock_send_rsync_block(fsockp, lgp, &ldevp, 1)) < 0) {
			return rc;
		}

		devp->rsyncdelta += ldevp.rsynclen;

		// stats, tunables
		ftd_lg_housekeeping(lgp, 0);
    }

	EmptyLList(devp->deltamap);

    return 0;
}

/*
 * ftd_rsync_lg_flush_delta --
 * flush the compiled list of deltas for the entire lg to the network 
 */
int
ftd_rsync_lg_flush_delta(ftd_lg_t *lgp)
{
	ftd_dev_t	**devpp;
	int			rc;

	ForEachLLElement(lgp->devlist, devpp) {
		if ((*devpp)->deltamap && SizeOfLL((*devpp)->deltamap) > 0) {
			if ((rc = ftd_rsync_flush_delta(lgp->dsockp, lgp, *devpp)) < 0) {					return rc;
				return -1;
			}
		}
	}

	return 0;
}

/*
 * ftd_rsync_chksum_diff --
 * compare local/remote checksums and create delta buffer
 * delta map goes with local device 
 */
int
ftd_rsync_chksum_diff(ftd_dev_t *devp, ftd_dev_t *rdevp)
{
	ftd_dev_delta_t	ref;
	unsigned char	*rptr, *lptr;
    ftd_uint64_t	length, offset, bytesleft;
    int				byteoff, res, i;

	res = CHKSEGSIZE;

	lptr = (unsigned char*)devp->sumbuf;
	rptr = (unsigned char*)rdevp->sumbuf;

	offset = devp->sumoff;
	length = devp->sumlen;
	
	// zero this and start adding to it
	devp->sumlen = 0;
	rdevp->dirtylen = 0;

	EmptyLList(devp->deltamap);

	// create delta data buffer to send to remote 
	for (i = 0; i < devp->sumnum; i++) {
		byteoff = i * res;
		bytesleft = (length < res ? length: res);
#if 0
{
		int j;

		fprintf(stderr," [%d]  local digest: ",i);
		for (j = 0; j < DIGESTSIZE; j++) {
			fprintf(stderr,"%02x",lptr[j]);
		}
		fprintf(stderr," [%d] remote digest: ", i);
		for (j = 0; j < DIGESTSIZE; j++) {
			fprintf(stderr,"%02x",rptr[j]);
		}
}
#endif
		if (memcmp(lptr, rptr, DIGESTSIZE)) {
			// checksums are different - bump delta length 
			devp->sumlen += bytesleft;
		}else {
			// checksums are the same 
			if (devp->sumlen) {
				// save offset, length - in sectors 
				ref.offset = offset;
				ref.length = devp->sumlen >> DEV_BSHIFT;

				AddToTailLL(devp->deltamap, &ref);

				rdevp->dirtylen += devp->sumlen;
				devp->sumlen = 0;
			}
			// bump offset - past this identical checksum 
			offset = devp->sumoff + ((byteoff+bytesleft) >> DEV_BSHIFT);
		}
		lptr += DIGESTSIZE;
		rptr += DIGESTSIZE;
		length -= bytesleft;
	}
	if (devp->sumlen) {
		// save offset, length - in sectors 
		ref.offset = offset;
		ref.length = devp->sumlen >> DEV_BSHIFT;

		AddToTailLL(devp->deltamap, &ref);

		rdevp->dirtylen += devp->sumlen;
		devp->sumlen = 0;
	} 

    return 0;
}

/*
 * ftd_rsync_chksum_seg --
 * compute checksums on given sector range 
 */
int
ftd_rsync_chksum_seg(ftd_lg_t *lgp, ftd_dev_t *devp)
{
	MD5_CTX			*contextp;
	unsigned char	*dataptr, *digestptr;
	int				length, left, bytes, res, rc;
	char			*devname;
	ftd_dev_cfg_t	*devcfgp;
	ftd_uint64_t	off64;

	if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, devp->devid)) == NULL) {
		reporterr(ERRFAC, M_PROTDEV, ERRCRIT, devp->devid);
		return -1;
	}

	if (lgp->cfgp->role == ROLEPRIMARY) {
		devname = devcfgp->pdevname;
	} else {
		devname = devcfgp->sdevname;
	}

	if ((contextp = MD5Create()) == NULL) {
		return -1;
	}

	off64 = devp->rsyncoff;
	off64 <<= DEV_BSHIFT;

	// read the device
	if (ftd_llseek(devp->devfd, off64, SEEK_SET) == (ftd_uint64_t)-1) {
		reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
			devname,
			off64,
			ftd_strerror());
		return -1;
	}

	rc = ftd_read(devp->devfd, devp->rsyncbuf, devp->rsyncbytelen);
	if (rc != devp->rsyncbytelen) {
		{
			int errnum = ftd_errno();
			ftd_uint64_t off64 = devp->rsyncoff << DEV_BSHIFT;
			reporterr(ERRFAC, M_READERR, ERRCRIT,
				devname,
				off64,
				devp->rsyncbytelen,
				strerror(errnum));
			return -1;
		}
	}
	digestptr = (unsigned char*)devp->sumbuf;
	dataptr = (unsigned char*)devp->rsyncbuf;

    res = CHKSEGSIZE; 
    length = (devp->rsynclen << DEV_BSHIFT);

    for (left = length; left > 0; left -= res) {
        if (left < res) {
            bytes = res = left;
        } else {
            bytes = res;
        }

        MD5Init(contextp);
        MD5Update(contextp, (unsigned char*)dataptr, bytes);
        MD5Final((unsigned char*)digestptr, contextp);
        digestptr += DIGESTSIZE;
        dataptr += bytes;
    }

	MD5Delete(contextp);

    return 1;	
}

/*
 * ftd_rsync_devs_to_read --
 * any devices left to read ? 
 */
static int
ftd_rsync_devs_to_read (ftd_lg_t *lgp)
{
	ftd_dev_t	**devpp;
    int			ret = 0;

    ForEachLLElement(lgp->devlist, devpp) {
        if (!(*devpp)->rsyncdone) {
           ret++;
        }
    }

    return ret;
}

/*
 * ftd_lg_refresh_full --
 * sync the peer's mirror devices with the local data devices 
 */
int
ftd_lg_refresh_full(ftd_lg_t *lgp)
{
	ftd_dev_t		**devpp, *devp;
	ftd_dev_cfg_t	*devcfgp;
    int				rc = 0, devsize, devbytes;
	ftd_uint64_t	rsyncsize, bytesleft, off64;

	ForEachLLElement(lgp->devlist, devpp) {
		devp = *devpp;
		if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, devp->devid)) == NULL) {
			reporterr(ERRFAC, M_PROTDEV, ERRCRIT, devp->devid);
			return -1;
		}

		devp->rsyncbuf = lgp->buf;

		devsize = devp->devsize - devp->no0write;

		//DPRINTF((ERRFAC,LOG_INFO,			" refresh_full: offset, ackoff, delta, size, no0write = %d, %d, %d, %d, %d\n",			devp->rsyncoff, devp->rsyncackoff, 		devp->rsyncdelta, devsize, devp->no0write));

		// update stats
		devp->statp->rsyncoff = devp->rsyncackoff;
		devp->statp->rsyncdelta = devp->rsyncdelta;

		if (devp->rsyncdone) {
			continue;
		}

		if (devp->rsyncoff >= devsize) {
			// we have sent all blocks 
			if (devp->rsyncackoff >= devsize) {
				// we have recvd acks for all blocks 
				devp->statp->rsyncoff = devp->devsize;

				// this device is done
				devp->rsyncdone = 1;

				ftd_stat_dump_driver(lgp);
			}
			continue;
		}

		bytesleft = (devsize - devp->rsyncoff);
		bytesleft <<= DEV_BSHIFT;
		if (bytesleft < lgp->tunables->chunksize) {
			rsyncsize = bytesleft;
		} else {
			rsyncsize = lgp->tunables->chunksize;
		}
		devp->rsyncbytelen = rsyncsize;
		devp->rsynclen = devp->rsyncbytelen >> DEV_BSHIFT;

		off64 = devp->rsyncoff;
		off64 <<= DEV_BSHIFT;

		if (ftd_llseek(devp->devfd, off64, SEEK_SET) != off64) {
			reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
				devcfgp->pdevname,
				off64,
				ftd_strerror());
			return -1;
		}
		if ((devbytes = ftd_read(devp->devfd, devp->rsyncbuf,
			devp->rsyncbytelen)) == -1)
		{
			reporterr(ERRFAC, M_READERR, ERRCRIT,
				devcfgp->pdevname,
				off64,
				devp->rsyncbytelen,
				ftd_strerror());
			return -1;
		}

#if defined(_WINDOWS)
        if (devbytes != devp->rsyncbytelen) {
            DWORD err = GetLastError();
        }
#endif

        // set length to actual length returned from read 
		devp->rsyncbytelen = devbytes;
		devp->rsynclen = (devbytes >> DEV_BSHIFT);
		devp->rsyncdelta += devp->rsynclen;

		if ((rc = ftd_sock_send_rsync_block(lgp->dsockp, lgp, devp, 1)) < 0) {
			return rc;
		}
		// bump device offset 
		devp->rsyncoff += devp->rsynclen;

		// stats, tunables
		ftd_lg_housekeeping(lgp, 0);
	}

	if (ftd_rsync_devs_to_read(lgp) == 0) {
		
		// all devs have completed
		if ((rc = ftd_sock_send_rsync_end(lgp->dsockp, lgp,
			GET_LG_STATE(lgp->flags))) < 0)
		{
			return rc;
		}
		
		SET_LG_RFDONE(lgp->flags);
		
		return FTD_CNORMAL;
	} 

	return 0;
}

/*
 * ftd_lg_refresh --
 * sync the peer's mirror devices with dirty blocks from local data devices 
 */
int
ftd_lg_refresh(ftd_lg_t *lgp)
{
	ftd_dev_t		**devpp, *devp;
	ftd_uint64_t	byteoff, bytelen, bytesleft;
    int				rc, devsize, csnum, len, digestlen;

	if (lgp->tunables->chunksize > lgp->buflen) {
		// flush any dirty data accumulated to this point
		// since the buffer size has changed  
		ForEachLLElement(lgp->devlist, devpp) {
			devp = *devpp;
			if (devp->dirtylen > 0) {
				if ((rc = ftd_rsync_process_seg(lgp, devp)) < 0) {
					return rc;
				}
			}
		}
		if (GET_LG_CHKSUM(lgp->flags)) {
			if ((rc = ftd_sock_send_rsync_chksum(lgp->dsockp, lgp, lgp->devlist,
				FTDCCHKSUM)) < 0)
			{
				return rc;
			}
		}
	}

	csnum = (lgp->tunables->chunksize / CHKSEGSIZE) + 2;
	digestlen = 2 * (csnum * DIGESTSIZE);

	ForEachLLElement(lgp->devlist, devpp) {
		devp = *devpp;
		
		devsize = devp->devsize - devp->no0write;
		
		devp->rsyncbuf = lgp->buf;

		if (digestlen > devp->sumbuflen) {
			devp->sumbuf = (char*)realloc(devp->sumbuf, digestlen);
		}

		devp->sumcnt = 2;
		devp->sumnum = csnum;
		devp->sumbuflen = digestlen;

		//DPRINTF((ERRFAC,LOG_INFO,		" refresh: offset, ackoff, delta, size, no0write = %d, %d, %d, %d, %d\n",			devp->rsyncoff, devp->rsyncackoff, 		devp->rsyncdelta, devsize, devp->no0write));

		// update stats
		devp->statp->rsyncoff = devp->rsyncackoff;
		devp->statp->rsyncdelta = devp->rsyncdelta;

		if (devp->rsyncdone) {
			continue;
		}

		if (devp->rsyncoff >= devsize) {
			// we have sent all blocks 
			if (devp->dirtylen) {
				if ((rc = ftd_rsync_process_seg(lgp, devp)) < 0) {
					return rc;
				}
			}
			if (devp->rsyncackoff >= devsize) {
				// we have recvd acks for all blocks 
				devp->statp->rsyncoff = devp->devsize;

				// this device is done
				devp->rsyncdone = 1;

				ftd_stat_dump_driver(lgp);
			}
			continue;
		}

		bytesleft = (devsize - devp->rsyncoff);
		bytesleft <<= DEV_BSHIFT;

		if (GET_LG_CHKSUM(lgp->flags)) {
			len = lgp->tunables->chunksize;
		} else {
			len = devp->dbres;
		}
		devp->rsyncbytelen = len < bytesleft ? len: bytesleft;
		devp->rsynclen = devp->rsyncbytelen >> DEV_BSHIFT;

		byteoff = devp->rsyncoff;
		byteoff <<= DEV_BSHIFT;

		bytelen = devp->rsyncbytelen;

		// test dirty 
		if (ftd_rsync_dirtyseg(devp->db, byteoff, bytelen, devp->dbres) > 0) {
			// will the next segment overflow the buf ? 
			if (devp->dirtylen > 0
				&& (devp->dirtylen + bytelen) >= lgp->tunables->chunksize)
			{
				if ((rc = ftd_rsync_process_seg(lgp, devp)) < 0) {
					return rc;
				}
			}
			// bump contiguous dirty length 
			devp->dirtylen += bytelen;   
		} else {
			// clean segment found - dump current buf 
			if (devp->dirtylen > 0) {
				if ((rc = ftd_rsync_process_seg(lgp, devp)) < 0) {
					return rc;
				}
			}
			devp->dirtyoff = (long)((byteoff + bytelen) >> DEV_BSHIFT);
			devp->rsyncackoff += devp->rsynclen;
		}
		
		// bump device offsets
		devp->rsyncoff += devp->rsynclen;
		devp->statp->effective += devp->rsyncbytelen;

		// stats, tunables
		ftd_lg_housekeeping(lgp, 0);
	}

	// send checksums for the group 
	if (GET_LG_CHKSUM(lgp->flags)) {
		if ((rc = ftd_sock_send_rsync_chksum(lgp->dsockp,
			lgp, lgp->devlist, FTDCCHKSUM)) < 0)
		{
			return rc;
		}
	}

	// flush deltas 
	if ((rc = ftd_rsync_lg_flush_delta(lgp)) < 0) {
		return rc;
	}

	if (ftd_rsync_devs_to_read(lgp) == 0) {
		
		// we are not really done until the peer ack's the sentinel
		if (!GET_LG_RFDONE_ACKPEND(lgp->flags)) {
			// flush deltas - final dirty residue 
			if ((rc = ftd_rsync_lg_flush_delta(lgp)) < 0) {
				return rc;
			}	
			// insert a go-coherent msg into bab 
			if (ftd_ioctl_send_lg_message(lgp->devfd,
				lgp->lgnum, MSG_CO) < 0)
			{
				return -1;
			}
			SET_LG_RFDONE_ACKPEND(lgp->flags);
		}

		if (GET_LG_RFDONE(lgp->flags)) {
			// we received the MSG_CO ack - transition to normal
			return FTD_CNORMAL;
		} 
	}
  
	return 0;
}

/*
 * ftd_lg_backfresh --
 * sync local data devices with dirty blocks from peer's mirror devices 
 */
int
ftd_lg_backfresh(ftd_lg_t *lgp)
{
	ftd_dev_t		**devpp, *devp;
	ftd_uint64_t	bytesleft;
    int				devsize, digestlen, csnum, num;
	int				rc, done = 0;

	// backfresh all devices for the logical group
	csnum = (lgp->tunables->chunksize / CHKSEGSIZE) + 2;
	digestlen = 2 * (csnum * DIGESTSIZE);

	ForEachLLElement(lgp->devlist, devpp) {
		devp = *devpp;
		devsize = devp->devsize - devp->no0write;

		devp->rsyncbuf = lgp->buf;	

		if (digestlen > devp->sumbuflen) {
			devp->sumbuf = (char*)realloc(devp->sumbuf, digestlen);
		}
		devp->sumbuflen = digestlen;

		error_tracef( TRACEINF, "ftd_lg_backfresh():backfresh: offset, ackoff, delta, size = %d, %d, %d, %d", 
			devp->rsyncoff, devp->rsyncackoff, devp->rsyncdelta, devsize );

		// update stats
		devp->statp->rsyncoff = devp->rsyncackoff;
		devp->statp->rsyncdelta = devp->rsyncdelta;

		if (devp->rsyncdone) {
			continue;
		}

		if (devp->rsyncoff >= devsize) {
			// we have sent all blocks 
			if (devp->rsyncackoff >= devsize) {
				// we have recvd acks for all blocks 
				devp->statp->rsyncoff = devp->devsize;

				// this device is done
				devp->rsyncdone = 1;

				ftd_stat_dump_driver(lgp);
			}
			continue;
		}

		devp->rsyncbytelen = lgp->tunables->chunksize;
		bytesleft = (devsize - devp->rsyncoff);
		bytesleft <<= DEV_BSHIFT;

		if (bytesleft < devp->rsyncbytelen) {
			devp->rsyncbytelen = bytesleft;
		}

		devp->rsynclen = (devp->rsyncbytelen >> DEV_BSHIFT);
		error_tracef( TRACEINF, "ftd_lg_backfresh():bytesleft, devp->rsyncbytelen, devp->rsynclen = %llu, %d, %d", 
			bytesleft, devp->rsyncbytelen, devp->rsynclen );

		num = (devp->rsyncbytelen % CHKSEGSIZE) ? (devp->rsyncbytelen / CHKSEGSIZE) + 1:
			(devp->rsyncbytelen / CHKSEGSIZE);
		devp->sumcnt = 2;
		devp->sumnum = num;
		devp->sumoff = devp->rsyncoff;
		devp->sumlen = devp->rsyncbytelen;

		// compute checksums 
		if ((rc = ftd_rsync_chksum_seg(lgp, devp)) < 0) {
			return -1;
		}

		devp->rsyncoff += (devp->rsyncbytelen >> DEV_BSHIFT);
		devp->statp->effective += devp->rsyncbytelen;

		// stats, tunables
		ftd_lg_housekeeping(lgp, 0);
	}
	
	// request remote checksums - just this segment 
	if ((rc = ftd_sock_send_rsync_chksum(lgp->dsockp, lgp, lgp->devlist,
		FTDCCHKSUM)) < 0)
	{
		return rc;
	}

	if (ftd_rsync_devs_to_read(lgp) == 0) {
		SET_LG_RFDONE(lgp->flags);
		return FTD_CNORMAL;
	} else {
		return 0;
	}
}

/*
 * ftd_rsync_dirtyseg --
 * inspect a device segment for 'dirty'ness 
 */
static int
ftd_rsync_dirtyseg(unsigned char *mask, ftd_uint64_t offset, ftd_uint64_t length, int res)
{
    ftd_uint64_t	startbit, endbit, endbyte, i;
    unsigned int	*wp;
    int				rc;


    //DPRINTF((ERRFAC,LOG_INFO,        " dirtyseg: res, offset, length %d, [%llu-%llu]\n",          res, offset, length));

    rc = 0;

    if (res == 0) {
        return -1;
    }

    /* scan this 'segment' of bits */
    endbyte = (ftd_uint64_t)(offset+length-1);
    startbit = (ftd_uint64_t)(offset/res);
    endbit = (ftd_uint64_t)(endbyte/res); 


    //DPRINTF((ERRFAC,LOG_INFO,        " offset, endbyte, startbit, endbit = %llu, %llu, %llu, %llu\n",          offset, endbyte, startbit, endbit));

    for (i = startbit; i <= endbit; i++) {
        /* find the word */
        //DPRINTF((ERRFAC,LOG_INFO," i, word = %llu, %llu\n",i, (i>>5)));
        wp = (unsigned int*)((unsigned int*)mask+(i>>5));

        /* test the bit */
#if defined(_AIX) || defined(_WINDOWS)
        if((*wp) & (0x00000001 << (i % (sizeof(*wp) * NBPB))))
#else /* defined(_AIX) */
        if ((*wp)&(0x01<<(i&0xffffffff)))
#endif /* defined(_AIX) */
        {
            rc = 1;
            break;
        }

    }

    //DPRINTF((ERRFAC,LOG_INFO, "dirtyseg: returning %d\n",        rc));

    return rc;
} 

/*
 * ftd_rsync_process_seg --
 * process data segment - rsync 
 */
static int
ftd_rsync_process_seg(ftd_lg_t *lgp, ftd_dev_t *devp)
{
	ftd_dev_t		ldevp;
	ftd_dev_cfg_t	*devcfgp;
	ftd_uint64_t	bytelen;
	int				devbytes = 0, rc;
	ftd_uint64_t	offset;

	if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, devp->devid)) == NULL) {
		reporterr(ERRFAC, M_PROTDEV, ERRCRIT, devp->devid);
		return -1;
	}

	// check buffer length
	if (devp->dirtylen > lgp->buflen) {
		lgp->buflen = devp->dirtylen;
		lgp->buf = (char*)realloc(lgp->buf, lgp->buflen);
	}

	memcpy(&ldevp, devp, sizeof(ftd_dev_t));
    
	// modify some state for checksum function 
    ldevp.rsyncoff = devp->dirtyoff;
    ldevp.rsynclen = devp->dirtylen >> DEV_BSHIFT;
    ldevp.rsyncbytelen = ldevp.sumlen = devp->dirtylen;

	ldevp.rsyncbuf = lgp->buf;

    bytelen = ldevp.rsyncbytelen;
    ldevp.sumnum = (int)((bytelen % CHKSEGSIZE) ?
		(bytelen / CHKSEGSIZE) + 1: (bytelen / CHKSEGSIZE));
    ldevp.sumcnt = devp->sumcnt;

	if (GET_LG_CHKSUM(lgp->flags)) {
		// build local checksum buffer 
		if ((rc = ftd_rsync_chksum_seg(lgp, &ldevp)) < 0) {
			return -1;
        }
		// save offset, length for send 
		devp->sumnum = ldevp.sumnum;
		devp->sumoff = devp->dirtyoff;
        devp->sumlen = devp->dirtylen;
    } else {	
		/*
		 * just send delta (it's faster). fuck a chksum
		 */
		offset = (((ftd_uint64_t)ldevp.rsyncoff) << DEV_BSHIFT);
		if (ftd_llseek(ldevp.devfd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
			reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
				devcfgp->pdevname,
				ldevp.rsyncoff,
				ftd_strerror());
			return -1;
		}
		if (ftd_read(ldevp.devfd, ldevp.rsyncbuf, ldevp.rsyncbytelen) == -1) {
			reporterr(ERRFAC, M_READERR, ERRCRIT,
				devcfgp->pdevname,
				ldevp.rsyncoff,
				ldevp.rsyncbytelen,
				ftd_strerror());
			return -1;
		}
		devp->rsyncdelta += ldevp.rsynclen;

		if ((rc = ftd_sock_send_rsync_block(lgp->dsockp, lgp, &ldevp, 1)) < 0) {
			// can't send - return
			return rc;
		}

		devp->sumoff = devp->dirtyoff;
		devp->sumlen = devp->dirtylen;
    }

    // reset dirty len accumulator 
    devp->dirtyoff = devp->rsyncoff;
    devp->dirtylen = 0;

    return 0;
}

