/***************************************************************************
 * ftd_cp.c - Transition one or more logical groups into or out of
 *            checkpoint mode
 *
 * Copyright (c) 1998 The FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 ***************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h> 
#include <errno.h> 
#include <malloc.h> 
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>

#include "ipc.h"
#include "cfg_intr.h"
#include "ps_intr.h"

#include "ftd_cmd.h"
#include "errors.h"
#include "pathnames.h"
#include "config.h"
#include "platform.h"

static char *progname = NULL;
static char p_configpaths[MAXLG][32];
static char s_configpaths[MAXLG][32];

static int on_flag;
static int primary;
static int secondary;

extern char *paths;
char *argv0;

extern unsigned char targets[MAXLG];

static void
usage(void)
{
    fprintf(stderr, "Usage: %s -a|-g <group_number> [-p | -s] -on|-off\n", progname);
    fprintf(stderr, "OPTIONS: -a   => all groups\n");
    fprintf(stderr, "         -g   => group number\n");
    fprintf(stderr, "         -p   => turn checkpoint on or off from primary (if loopback)\n");
    fprintf(stderr, "         -s   => turn checkpoint on or off from secondary (if loopback)\n");
    fprintf(stderr, "         -on  => turn checkpoint on\n");
    fprintf(stderr, "         -off => turn checkpoint off\n");
}

int 
main(int argc, char *argv[])
{
    int                 i, j, fd, lgnum, all, p_lgcnt, s_lgcnt;
    int                 mpid, sig;
    int                 ftdsock, pcnt, cnt;
#if defined(_AIX)
    int                 ch;
#else  /* defined(_AIX) */
    char                ch;
#endif /* defined(_AIX) */
    unsigned char mask[MAXLG];

    progname = argv[0];
    if (argc < 3) {
        goto usage_error;
    }
    argv0 = argv[0];

    all = 0;
    lgnum = -1;
    on_flag = 0;

    memset(targets, 0, sizeof(targets));
    memset(mask, 0, sizeof(mask));

    primary = secondary = 0;

    /* process all of the command line options */
    opterr = 0;
    while ((ch = getopt(argc, argv, "apsg:o:")) != -1) {
        switch(ch) {
        case 'a':
            if (lgnum != -1) {
                goto usage_error;
            }
            all = 1;
            break;
        case 'g':
            if (all) {
                goto usage_error;
            }
            lgnum = strtol(optarg, NULL, 0);
            targets[lgnum] = 1;
            break;
        case 'p':
            if (secondary) {
                goto usage_error;
            }
            primary = 1;
            break;
        case 's':
            if (primary) {
                goto usage_error;
            }
            secondary = 1;
            break;
        case 'o':
            if (optarg[0] == 'n') {
                on_flag = 1; 
            } else {
                on_flag = 0; 
            }
            break;
        default:
            goto usage_error;
        }
    }
    if (!all && lgnum < 0) {
        goto usage_error;
    }

    if (initerrmgt(ERRFAC) < 0) {
        exit (1);
    }

    /* signal the Master daemon */ 
    if ((mpid = getprocessid(FTDD, 0, &pcnt)) <= 0) {
        reporterr(ERRFAC, M_NOMASTER, ERRWARN, argv0);
        exit(1);
    }
    if ((ftdsock = connecttomaster(FTDD, 0)) == -1) {
        exit(1);
    }
    
    if (!secondary) {
        /* if secondary explicitly specified then skip this */
        p_lgcnt = GETCONFIGS(p_configpaths, 1, 1);
        paths = (char*)p_configpaths;
        for (i=0; i<p_lgcnt; i++) {
            lgnum = cfgpathtonum(p_configpaths[i]);
            if (all) {
                targets[lgnum] = 1;
            }
            if (targets[lgnum]) {
                mask[lgnum] |= 0x01;
            }
        }
    }
    if (!primary) {
        /* if primary explicitly specified then skip this */
        s_lgcnt = GETCONFIGS(s_configpaths, 2, 0);
        for (i=0; i<s_lgcnt; i++) {
            lgnum = cfgpathtonum(s_configpaths[i]);
            if (all) {
                targets[lgnum] = 1;
            }
            if (targets[lgnum]) {
                mask[lgnum] |= 0x02;
            }
        }
    }
    cnt = 0;

    for (i = 0; i < MAXLG; i++) {
        if (!targets[i] || !mask[i]) { 
            continue;
        }
        if (mask[i] == 0x03) {
            /* -- handle loopback - use -p to assume primary role, otherwise
               assume secondary role */
            if (primary) {
                mask[i] = 0x01;
            } else {
                mask[i] = 0x02;
            }
        }
        cnt++;
    }
    sig = FTDQCP;
    if (ipcsend_sig(ftdsock, cnt, &sig) == -1) {
        exit(1);
    }
    pcnt = 0;

    for (i = 0; i < MAXLG; i++) {
        if (mask[i] == 0x01) {
            /* find primary targets */
            sig = on_flag ? FTDQCPONP: FTDQCPOFFP;
            pcnt++;
        } else if (mask[i] == 0x02) {
            /* find secondary targets */
            sig = on_flag ? FTDQCPONS: FTDQCPOFFS;
        } else {
            continue;
        }
        if (ipcsend_sig(ftdsock, 0, &sig) == -1) {
            exit(1);
        }
        lgnum = i;
        if (ipcsend_lg(ftdsock, lgnum) == -1) {
            exit(1);
        }
    }
    /* wait for ACK(s) from master */
    while (pcnt > 0) {
        ipcrecv_ack(ftdsock);
        pcnt--;
        usleep(10000);
    }
    
    exit(0);
    return 0; /* for stupid compiler */
    
usage_error:
    usage();
    exit(1);
    return 0; /* for stupid compiler */
}


