/*
 * ftd_override.c - override the state of the driver 
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
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

#if defined(SOLARIS)
#include <sys/mkdev.h>
#endif

#include "pathnames.h"
#include "ps_intr.h"
#include "ftdio.h"
#include "errors.h"

#include "ftd_cmd.h"
#include "common.h"
/* for errors: */
char *argv0;

extern char *paths;
static char configpaths[512][32];

/* internal defines */
#define SOMETHING_BAB   1
#define SOMETHING_LRDB  2
#define SOMETHING_HRDB  3

static void
usage(void)
{
    fprintf(stderr, "Usage: %s [-g group_number | -a] \n", argv0);
    fprintf(stderr, "          [ [clear [bab | LRT| HRT]] | \n");
    fprintf(stderr, "            [state [passthru|normal|refresh|tracking|backfresh]] ]\n");
}

static void
maybe_report_state_failure(int group, int state, int err)
{
    /* XXX Should be REPORT ERRORIFIED XXX */
    switch(err) {
    case -6:
        fprintf(stderr, "Can't get PSTORE value\n");
        break;
    case -5:
        fprintf(stderr, "Can't open CTL device: %s\n", strerror(errno));
        break;
    case -4:
        fprintf(stderr, "PS Get attributes failed\n");
        break;
    case -3:
        fprintf(stderr, "Can't get PSTORE group info for group %d\n", group);
        break;
    case -2:
        fprintf(stderr, "PS Set Group State failed for group %d state %d\n",
            group, state);
        break;
    case -1:
        fprintf(stderr, "Driver error setting state to %d for group %d\n%s\n",
            state, group, strerror(errno));
        break;
    }
}

/*
 *
 */
static int
set_state(int group, int state)
{
    int         fd, ret;
    char        ps_name[MAXPATHLEN];
    char        group_name[MAXPATHLEN];
    ftd_state_t st;
    ps_group_info_t ginfo;
    ps_version_1_attr_t attr;

    /* get the name of the pstore */
    if (GETPSNAME(group, ps_name) == -1) {
        sprintf(group_name, "%s/%s%03d.cfg",
            PATH_CONFIG, PMD_CFG_PREFIX, group);
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, group_name);
        return -6;
    }
    if (ps_get_version_1_attr(ps_name, &attr) != PS_OK) {
	    return -4;
    }
    FTD_CREATE_GROUP_NAME(group_name, group);

    /* set the state in the pstore */
    ginfo.name = NULL;
    if (ps_get_group_info(ps_name, group_name, &ginfo) != PS_OK) {
        return -3;
    }

    /* if we aren't changing state, we're done */
#if 0
    /*
     *  No, we're not done.  We need to sync between the driver and 
     * the pstore
     */
    if (ginfo.state == state) {
        return 0;
    }
#endif
    /* if we are in backfresh mode, change the state in the pstore 
        and that's it */
    if (ginfo.state == FTD_MODE_BACKFRESH) {
        if (ps_set_group_state(ps_name, group_name, state) != PS_OK) {
            return -2;
        }
        /* set the shutdown state to 1 */
        ps_set_group_shutdown(ps_name, group_name, 1);
        return 0;
    }

    /* if we are going into backfresh mode, stop the driver,
       and load pertinent data into the pstore */
    if (state == FTD_MODE_BACKFRESH) {
        /* stop the driver (ala ftdstop) */
        ftd_stop_group(ps_name, group, &attr);
        return 0;
    } 

    /* now, set the new state in the driver */
    st.lg_num = group | FTD_LGFLAG;
    st.state = state;
   
    if ((fd = open(FTD_CTLDEV, O_RDWR)) < 0) {
        return -5;
    }
    ret = FTD_IOCTL_CALL(fd, FTD_SET_GROUP_STATE, &st);
    close(fd);

    /* set the state in the pstore */
    if (ps_set_group_state(ps_name, group_name, state) != PS_OK) {
        return -2;
    }
    return (ret);
}

/*
 *
 */
static int
clear_something(int grp, int state)
{
    int fd, ret, count;
    ftd_dev_t_t group=grp;

    if ((fd = open(FTD_CTLDEV, O_RDWR)) < 0) {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return -1;
    }
    group |= FTD_LGFLAG;

    if (state == SOMETHING_BAB) {
        /* retry at most 10 times */
        count = 0;
        while (((ret = FTD_IOCTL_CALL(fd, FTD_CLEAR_BAB, &group)) == EAGAIN) && 
                (count < 10)) {
			sleep(1);
            count++;
        }
        close(fd);
        if (ret != 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "CLEAR_BAB", 
                strerror(errno));
            return -1;
        }
    } else if (state == SOMETHING_LRDB) {
        ret = FTD_IOCTL_CALL(fd, FTD_CLEAR_LRDBS, &group);
        close(fd);
        if (ret != 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "CLEAR_LRDBS",
                strerror(errno));
            return -1;
        }
    } else if (state == SOMETHING_HRDB) {
        ret = FTD_IOCTL_CALL(fd, FTD_CLEAR_HRDBS, &group);
        close(fd);
        if (ret != 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "CLEAR_HRDBS",
                strerror(errno));
            return -1;
        }
    } else {
        close(fd);
        return -1;        
    }

    
	return 0;
}

int 
main(int argc, char *argv[])
{
    int i, state, group, index, lgcnt, pcnt;
    char pmd[12]; /* store the pmd names e.g. PMD_000  */
    int cur_state; /* current state of the driver */

    argv0 = argv[0];
    if (argc < 4) {
        goto usage_error;
    }

    /* FIXME: clean up parsing, if anyone cares. */
    if (strcmp("-g", argv[1]) == 0) {
        group = atoi(argv[2]);
        index = 3;
        if (argc < 5) {
            goto usage_error;
        }
    } else if (strcmp("-a", argv[1]) == 0) {
        group = -1;
        index = 2;
    } else {
        goto usage_error;
    }

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    /* get all of the configs */
    paths = (char *)configpaths;
    lgcnt = GETCONFIGS(configpaths, 1,0);		   /* PBouchard, missed 3rd arg, add 0 ? */

    if (strcmp("clear", argv[index]) == 0) {
        if (strcmp("bab", argv[index+1]) == 0) {
            /* clear the bab for one or all groups */
            state = SOMETHING_BAB;
        } else if (strcmp("LRT", argv[index+1]) == 0) {
            /* clear the LRDB for one or all groups */
            state = SOMETHING_LRDB;
        } else if (strcmp("HRT", argv[index+1]) == 0) {
            /* clear the HRDB for one or all groups */
            state = SOMETHING_HRDB;
        } else {
            goto usage_error;
        }
        if (group == -1) {
            for (i = 0; i < lgcnt; i++) {
                group = cfgtonum(i);
                clear_something(group, state);
            }
        } else {
            clear_something(group, state);
        }
    } else if (strcmp("state", argv[index]) == 0) {
        if (strcmp("passthru", argv[index+1]) == 0) {
            state = FTD_MODE_PASSTHRU;
        } else if (strcmp("normal", argv[index+1]) == 0) {
            state = FTD_MODE_NORMAL;
        } else if (strcmp("refresh", argv[index+1]) == 0) {
            state = FTD_MODE_REFRESH;
        } else if (strcmp("tracking", argv[index+1]) == 0) {
            state = FTD_MODE_TRACKING;
        } else if (strcmp("backfresh", argv[index+1]) == 0) {
            state = FTD_MODE_BACKFRESH;
        } else {
            goto usage_error;
        }
        if (group == -1) {
            for (i = 0; i < lgcnt; i++) {
                group = cfgtonum(i);
                sprintf(pmd, "PMD_%03d", group);
                if (getprocessid(pmd, 0, &pcnt) > 0)
                 {
                  pmds_running(pmd, group, state);   /* killpmds -a */
                  maybe_report_state_failure(group, state,
                     set_state(group, state));              /* change state */
                 }
                else
                 {
                  maybe_report_state_failure(group, state,
                     set_state(group, state));              /* change state */
                  pmds_not_running(  pmd, group, state); /* launchpmds -a */
                 }
           } /* end for */
        } else {
            sprintf(pmd, "PMD_%03d", group);
            if (getprocessid(pmd, 0, &pcnt) > 0)
             {
                pmds_running(pmd, group, state);    /* killpmds -g X */
                maybe_report_state_failure(group, state,
                   set_state(group, state));               /* change state */
             } 
           else
             {
                maybe_report_state_failure(group, state,
                   set_state(group, state));               /* change state */
                pmds_not_running(  pmd, group, state); /* launchpmds -g X */
             }
    } 
  }    else {
             goto usage_error;
            }
    exit(0);
    return 0;

usage_error:
    usage();
    exit(1);
    return 0; /* for stupid compiler */
}

/*******  Code added for PRN/503 **********/

pmds_not_running(char* pmd, int group, int req_state)
{
  char cmd[512];
 /* only if current_state=ANYTHING && req_state=NORMAL do this  */
 
 if(req_state==FTD_MODE_NORMAL)
 {
  sprintf(cmd, "%s/launchpmds -g %d", PATH_BIN_FILES, group);
  system(cmd);
 }
}

pmds_running(char* pmd, int group, int req_state)
{
 char cmd[512];

 /* only if current_state=NORMAL && req_state=TRACKING do this  */
 
 if(((get_driver_mode(group)==FTD_MODE_NORMAL) && (req_state==FTD_MODE_TRACKING))
    || (get_driver_mode(group)==FTD_MODE_REFRESH && req_state==FTD_MODE_PASSTHRU))
 {
     sprintf(cmd, "%s/killpmds -g %d", PATH_BIN_FILES, group);
     printf("killing %s\n", pmd);
     system(cmd);
 }
}
