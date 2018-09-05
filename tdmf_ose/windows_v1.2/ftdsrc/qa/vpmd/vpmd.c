/*
 * ftd_set.c - set/get parameters in the persistent store and driver
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h> 
#include <malloc.h> 
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include "ftdio.h"
#include "ftd_cmd.h"
#include "ftdif.h"
#include "err.h"
#include "errors.h"

#define SCALE_FACTOR 1.1
#define THRESHOLD 0.85

char *__argv0; /* required for err* and warn* */

static void
usage()
{
    fprintf(stderr, "Usage: %s -g gnum [-d latency]\n", __argv0);
    exit(1);
}

void
nop(int sig)
{
    signal(sig, nop);
}
    
void
handle_overflow(int ctlfd, int fd, int lgnum)
{
    dirtybit_array_t    db;
    ftd_state_t         newstate;
    int                 i;
    ftd_dev_info_t      *dip;
    stat_buffer_t       sb;
    char msgbuf[DEV_BSIZE];

    if (get_dirtybits(ctlfd, fd, lgnum, &db, &dip) == 0) {
        free(db.devs);
        free(db.dbbuf);
        free(dip);
    
        printf("RECOVER...");
        fflush(stdout);
        for (i = 5; i > 0; i--) {
            printf("%d...", i);
            fflush(stdout);
            flush_bab(ctlfd, lgnum);
            sleep(1);
        }
    } else {
        flush_bab(fd, lgnum);
    }
    memset(msgbuf, 0xed, sizeof(msgbuf));
    sb.lg_num = lgnum;
    sb.len = sizeof(msgbuf);
    sb.addr = msgbuf;
    if (ioctl(fd, FTD_SEND_LG_MESSAGE, &sb))
        err(1, "Send message failed");
    newstate.state = FTD_MODE_NORMAL;
    newstate.lg_num = lgnum;
    if (ioctl(ctlfd, FTD_SET_GROUP_STATE, &newstate))
        err(1, "Can't transition states");
    printf("JOURNAL\n");
}

int 
main(int argc, char *argv[])
{
    char group_name[MAXPATHLEN + 1];
    int i;
    int ctlfd;
    int fd;
    oldest_entries_t oe[1];
    char *chunk;
    char *walker;
    char *eod;
    wlheader_t *hdr;
    int chunksize = 1024 * 256;         /* -c */
    fd_set fds;
    migrated_entries_t mig[1];
    int gnum = -1;
    float latency = 0;
    long long bytes = 0;
    long long migs = 0;
    int ch;
    double scale = SCALE_FACTOR;        /* -s */
    double threshold = THRESHOLD;       /* -t */
    int do_select = 0;                  /* -S turns this on */
    int do_latency_tweak = 0;           /* -L turns this on */

    /* gotta have enough args */
    if (argc < 2)
        usage();
    __argv0 = argv[0];

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }
    signal(SIGHUP, nop);

    while ((ch = getopt(argc, argv, "c:d:g:LSs:t:")) != -1) {
        switch(ch) {
        case 'c':
            chunksize = strtol(optarg, NULL, 0);
            break;
        case 'd':               /* Set the delay to simulate latency */
            latency = strtod(optarg, NULL);
            break;
        case 'g':
            gnum = strtol(optarg, NULL, 0);
            break;
        case 's':
            scale = strtod(optarg, NULL);
            break;
        case 't':
            threshold = strtod(optarg, NULL);
            break;
        case 'S':
            do_select = 1;
            break;
        case 'L':
            do_latency_tweak = 1;
            break;
        default:
            usage();
            break;
        }
    }
    
    if (!(chunk = (void *) malloc(chunksize)))
        errx(1, "Malloc failed for %d bytes for chunk\n", chunksize);
#ifdef PURE
    memset(chunk, 0, chunksize);
#endif

    if (gnum == -1)
        usage();

    FTD_CREATE_GROUP_NAME(group_name, gnum);
    printf("Using %s to control logical group... The virtual pmd is online\n",
        group_name);

    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0)
        err(1, "Can't open control device %s", FTD_CTLDEV);

    fd = open(group_name, O_RDWR);
    if (fd < 0)
        err(1, "Can't open logical group control file %s", group_name);

    while (1) {
        if (do_select) {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            i = select(32, &fds, 0, 0, 0);
            if (i < 0) {
                if (errno == EINTR) {
                    printf("EINTR!\n");
                    continue;
                } else {
                    err(1,"Error from select");
                }
            }
        }

        usleep((int) (latency * 1000.0));
        memset(oe, 0, sizeof(oldest_entries_t));
        oe->addr = (int*)chunk;
        oe->offset32 = 0;
        oe->len32 = chunksize / sizeof(int);
        if (ioctl(fd, FTD_OLDEST_ENTRIES, oe) < 0) {
            if (errno == EINVAL)
                /* no more journal to read */
                continue;
            err(1,"Error getting oldest entries");
        }
        if (oe->state != FTD_MODE_NORMAL && oe->retlen32 == 0) {
            printf("\nOverflow!!!!\n");
            handle_overflow(ctlfd, fd, gnum);
            continue;
        }
        if (oe->retlen32 == 0)
            continue;
        if (do_latency_tweak) {
            if (oe->retlen32 < oe->len32 * threshold) {
                latency *= scale;
                if (latency > 2048)
                    latency = 2048;
            } else {
                latency = latency / scale;
            }
            if (latency < 1)
                latency = 1;
        }
        walker = chunk;
        eod = chunk + oe->retlen32 * sizeof(int);
        memset(mig, 0, sizeof(*mig));
        mig->bytes = 0;
        while (walker < eod) {
            hdr = (wlheader_t *) walker;
            if (hdr->majicnum != DATASTAR_MAJIC_NUM || 
                (walker + sizeof(wlheader_t)) > eod ||
                (walker + (hdr->length << DEV_BSHIFT)) > eod)
                break;
            walker += (hdr->length << DEV_BSHIFT) + sizeof(wlheader_t);
            mig->bytes = walker - chunk;
        }
        /*
         * Note: The following is a kludge and means that the stats for
         * a device's use of the bab isn't reported.
         */
        if (ioctl(fd, FTD_MIGRATE, mig))
            err(1, "Can't migrate data");
        bytes += mig->bytes;
        migs++;
        printf("Migrated %lld bytes in %lld migrations %4.2f %d\r",
            bytes, migs, latency, oe->retlen32);
        fflush(stdout);
    }
}

