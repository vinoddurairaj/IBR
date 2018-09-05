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
/*
 * ftd_set.c - set/get parameters in the persistent store and driver
 *
 * Copyright (c) 1998 The FullTime Software, Inc.  All Rights Reserved.
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
#include <dirent.h>

#include "cfg_intr.h"
#include "ps_intr.h"
#include "ftdio.h"
#include "ftd_cmd.h"
#include "errors.h"
#include "config.h"
#include "pathnames.h"
#include "common.h"
#ifdef _AIX
#include "aixcmn.h"
#endif

static char *progname;
char *argv0;

static char *input_key[FTD_MAX_KEY_VALUE_PAIRS];
static char *input_value[FTD_MAX_KEY_VALUE_PAIRS];

extern machine_t *mysys;
static char configpaths[MAXLG][32];
extern char *paths;
extern int ftd_debugFlag;

static void
usage(void)
{
    fprintf(stderr, "Usage: %s -g <group#> [ key=value ... ]\n", progname);
    fprintf(stderr, "\t<group#> is " GROUPNAME " group number. (0 - %d)\n", MAXLG-1);
    exit(1);
}

int
ftdset_set_tunable_cfg_file(int lgnum, char *key, char *val)
{
    FILE            *cfd;
    FILE            *tfd;
    DIR             *dfd;
    struct dirent   *dent;
    char            cfgfile[MAXPATH];
    char            guimarker[MAXPATH];
    char            tmpcfgfile[MAXPATH];
    char            line[MAXPATH];
    char            tmpstr[MAXPATH];
    int             found = 0;
    int             i;
    int             j;
    int             k;
    int             notmatch = 1;
    struct stat     statbuf;

    sprintf(cfgfile, "settunables%d.tmp", lgnum);
    sprintf(guimarker, "settunables%d.gui", lgnum);
    sprintf(tmpcfgfile, "settunables%d.tmp2", lgnum);

    if (stat(guimarker, &statbuf) != -1) {
        /*
         * GUI is currently writing to settunables.tmp, don't mess it up
         */
        return -1;
    }

    ftd_trace_flow(FTD_DBG_FLOW1, "look for tmp cfg file: %s/%s\n", 
            PATH_CONFIG, cfgfile);

    if ((dfd = opendir(PATH_CONFIG)) == NULL) {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, PATH_CONFIG, strerror(errno));
        if (errno == ENOENT) {
            exit(EXITANDDIE);
        } else {
            return -1;
        }
    }

    while ((dent = readdir(dfd)) != NULL) {
        if ((strlen(dent->d_name) == strlen(cfgfile)) &&
            (memcmp(dent->d_name, cfgfile, strlen(cfgfile)) == 0)) {
            ftd_trace_flow(FTD_DBG_FLOW1, "get file: %s\n", dent->d_name);
            found++;
            break;
        }
    }

    (void)closedir(dfd);
    
    if (found) {
        if ((cfd = fopen(cfgfile, "r")) == NULL) {
            ftd_trace_flow(FTD_DBG_ERROR, "fail to open file: %s/%s\n",
                            PATH_CONFIG, cfgfile);
            return -1;
        }
    } else {
        if ((cfd = fopen(cfgfile, "w+")) == NULL) {
            ftd_trace_flow(FTD_DBG_ERROR, "fail to open file: %s/%s\n",
                            PATH_CONFIG, cfgfile);
            return -1;
        }
    }

    if (found) {
        /*
         * file exist, parse and update
         */
        unlink(tmpcfgfile);

        if ((tfd = fopen(tmpcfgfile, "w+")) == NULL) {
            ftd_trace_flow(FTD_DBG_ERROR, "fail to open file: %s/%s\n",
                            PATH_CONFIG, tmpcfgfile);
            return -1;
        }

        while (1) {
            for (k = 0; k < MAXPATH; k++) {
                tmpstr[k] = '\0';
                line[k] = '\0';
            }

            if (fgets(line, MAXPATH, cfd) == NULL) {
                break;
            }

            i = 0;      /* index for line */
            j = 0;      /* index for tmpstr */
            notmatch = 1;

            while (i < MAXPATH) {
                j = 0;  /* index for tmpstr */
                tmpstr[0] = '\0';

                if ((line[i] == '\n') || (line[i] == '\0')) {
                    break;
                }

                while ((line[i] != ' ') && (line[i] != '\t') && (i < MAXPATH)) {
                    tmpstr[j] = line[i];
                    i++;
                    j++;
                    if ((line[i] == '\n') || (line[i] == '\0')) {
                        break;
                    } else {
                        continue;
                    }
                }

                tmpstr[j] = '\0';

                if ((tmpstr[0] != '\0') && (sizeof(key) >= 1)) {
                    if (strncmp(tmpstr, key, sizeof(key)-1) == 0) {
                        notmatch = 0;
                        fprintf(tfd, "%s/%sset -q -g %d %s=%s\n",
                                PATH_BIN_FILES, QNM, lgnum, key, val);
                        break;
                    }
                }

                i++;
            }

            if (notmatch) {
                fprintf(tfd, line);
                notmatch = 0;
            }
        }

        fclose(tfd);

    } else {
        /*
         * new file
         */
        fprintf(cfd, "#!/bin/sh\n");
        fprintf(cfd, "%s/%sset -q -g %d %s=%s\n",
                PATH_BIN_FILES, QNM, lgnum, key, val);
    }

    fclose(cfd);

    if (found) {
        unlink(cfgfile);
        rename(tmpcfgfile, cfgfile);
    }

    if (stat(cfgfile, &statbuf) != -1) {
        chmod(cfgfile, S_IRWXU | S_IRGRP | S_IROTH);
    }

    return 0;
}

#ifdef UPDATE_P_FILE
int
ftdset_set_cfg_file(int lgnum, char *key, char *val)
{
    FILE            *cfd;
    FILE            *tfd;
    DIR             *dfd;
    struct dirent   *dent;
    char            cfgfile[MAXPATH];
    char            guimarker[MAXPATH];
    char            tmpcfgfile[MAXPATH];
    char            line[MAXPATH];
    char            tmpstr[MAXPATH];
    struct stat     statbuf;
    int             jlocale = 0;
    int             jval = 0;
    int             swap = 1;
    int             found = 0;
    int             i;
    int             j;
    int             k;

    sprintf(cfgfile, "p%03d.cfg", lgnum);
    sprintf(guimarker, "settunables%d.gui", lgnum);
    sprintf(tmpcfgfile, "p%03d.tmp", lgnum);

    if (stat(guimarker, &statbuf) != -1) {
        /*
         * GUI is currently writing to settunables.tmp, don't mess it up
         */
        return -1;
    }

    if ((dfd = opendir(PATH_CONFIG)) == NULL) {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, PATH_CONFIG, strerror(errno));
        if (errno == ENOENT) {
            exit(EXITANDDIE);
        } else {
            return -1;
        }
    }

    while ((dent = readdir(dfd)) != NULL) {
        if ((strlen(dent->d_name) == strlen(cfgfile)) &&
            (memcmp(dent->d_name, cfgfile, strlen(cfgfile)) == 0)) {
            ftd_trace_flow(FTD_DBG_FLOW1, "get file: %s\n", dent->d_name);
            found++;
            break;
        }
    }

    (void)closedir(dfd);
    
    if (found) {
        if ((cfd = fopen(cfgfile, "r")) == NULL) {
            ftd_trace_flow(FTD_DBG_ERROR, "fail to open file: %s/%s\n",
                            PATH_CONFIG, cfgfile);
            return -1;
        }
    } else {
        ftd_trace_flow(FTD_DBG_ERROR, "config file: %s/%s missing\n",
                        PATH_CONFIG, cfgfile);
        return -1;
    }

    unlink(tmpcfgfile);

    if ((tfd = fopen(tmpcfgfile, "w+")) == NULL) {
        ftd_trace_flow(FTD_DBG_ERROR, "fail to open file: %s/%s\n",
                        PATH_CONFIG, tmpcfgfile);
        return -1;
    }

    while (1) {
        if (strcmp("JOURNAL", key) != 0) {
            /*
             * only handle key = JOURNAL for now
             */
            swap = 0;
            break;
        }
        
        for (k = 0; k < MAXPATH; k++) {
            tmpstr[k] = '\0';
            line[k] = '\0';
        }

        if (fgets(line, MAXPATH, cfd) == NULL) {
            break;
        }

        i = 0;      /* index for line */
        j = 0;      /* index for tmpstr */

        if (jlocale) {
            jlocale++;
        }

        while (i < MAXPATH) {
            j = 0;  /* index for tmpstr */
            tmpstr[0] = '\0';

            if ((line[i] == '\n') || (line[i] == '\0')) {
                break;
            }

            while ((line[i] != ' ') && (line[i] != '\t') && (i < MAXPATH)) {
                tmpstr[j] = line[i];
                i++;
                j++;
                if ((line[i] == '\n') || (line[i] == '\0')) {
                    break;
                } else {
                    continue;
                }
            }

            tmpstr[j] = '\0';

            if ((tmpstr[0] != '\0')) {
                if (strncmp(tmpstr, "JOURNAL:", sizeof("JOURNAL:")-1) == 0) {
                    jlocale = 1;
                    break;
                }
            }

            if ((tmpstr[0] != '\0')) {
                if (strncmp(tmpstr, "JOURNAL", sizeof("JOURNAL")) == 0) {
                    jval = 1;
                    break;
                }
            }

            i++;
        }

        if (jval == 1) {
            fprintf(tfd, "  %s %s\n", key, val);
            jval = 0;
        } else {
            if (jlocale == 2) {
                /*
                 * this configure file does not have line "JOURNAL on/off"
                 * add one into config file
                 */
                fprintf(tfd, "# %s %s\n", key, val);
                fprintf(tfd, line);
            } else {
                fprintf(tfd, line);
            } 
        }
    }

    fclose(cfd);
    fclose(tfd);

    if (swap) {
        unlink(cfgfile);
        rename(tmpcfgfile, cfgfile);
    }

    return 0;
}
#endif

int 
main(int argc, char *argv[])
{
    char  *inbuffer, *temp;
    char  group_name[MAXPATHLEN];
    char  ps_name[MAXPATHLEN];
    int  ch;

    char tempkey[30];
    char tempval[30];
    int  num_input, valid;
    int  i, j, ret, lgnum = -1;
    int quiet = 0;
    int dflag = 0;      /* -d option flag */
    int gflag = 0;      /* -g option flag */
    int Dflag = 0;      /* -D option flag, used internally for debugging */
    int flag = 0;       /* used for settnable() */
    ps_version_1_attr_t attr;
    char  tunablebuffer[512];
    char *tp, *tokptr;
    char *valid_key [FTD_MAX_KEY_VALUE_PAIRS];
    int max_valid;
    tunable_t cur_tunables;
    char cur_cfg[9];
    long chunk;
    long longval;
    int driver_mode;
    int driver_mode_disallow = 0;
    int driver_ready;
    int rc;
    int setjournal = 0;
    int journal_state = -1;
    int group,pcnt;
    char pmd[12]; 
    FTD_TIME_STAMP(FTD_DBG_FLOW1, "Start\n");

    putenv("LANG=C");

    /* Make sure we are root */
    if (geteuid()) {
        fprintf(stderr, "You must be root to run this process...aborted\n");
        exit(1);
    }

    progname = argv[0];    
    argv0 = argv[0];

    if (argc < 2) {
        usage();
    }

    /* initialize valid_keys array */
    strcpy(tunablebuffer, FTD_DEFAULT_TUNABLES);
    tp = tunablebuffer;
    i = 0;
    while (tp) {
        tokptr = strchr(tp,':');
        *tokptr = '\0';
        valid_key[i] = (char *) ftdmalloc((strlen(tp) + 1) * sizeof(char));
        strcpy(valid_key[i], tp);
        tp = tokptr + 1;
        if ((tp = strchr(tp,'\n'))) {
            tp++;
        } else {
            tp = NULL;
        }
        i++;
    }
    max_valid=i;
    
    opterr = 0;
    while ((ch = getopt(argc, argv, "Dd:qg:"))!=-1) {
        switch (ch) {
        case 'D': 
            Dflag++;
            break;
        case 'd': 
            ftd_debugFlag = ftd_strtol(optarg);
            dflag++;
            break;
        case 'g': 
            if (gflag) {
                fprintf(stderr, "-g options are multiple specified\n");
                usage();
            }
            lgnum = ftd_strtol(optarg);
            if (lgnum < 0 || lgnum >= MAXLG) {
                fprintf(stderr, "Invalid number for " GROUPNAME " group\n");
                usage();
            }
            gflag++;
            break;
        case 'q': 
            quiet = 1;
            break;
        default:
            usage();
            break;
        }
    }

    if (gflag == 0) {
        fprintf(stderr, "-g option is not specified. It is mandatory option\n");
        usage();
    }

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    log_command(argc, argv);   /* trace command in dtcerror.log */

    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    sprintf(cur_cfg, "p%03d.cfg", lgnum);
    if (GETPSNAME(lgnum, ps_name) != 0) {
        if (!quiet) {
            reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, cur_cfg);
        }
        exit(1);
    }

    /* read the persistent store for info on this group */
    ret = ps_get_version_1_attr(ps_name, &attr, 1);
    if (ret != PS_OK) {
        if (!quiet) {
            reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
        }
        exit(1);
    }
    /* allocate an input buffer for the parameters */
    if (((inbuffer = (char *)malloc(attr.group_attr_size)) == NULL) ) {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.group_attr_size);
        exit(1);
    }
    ret = ps_get_group_attr(ps_name, group_name, inbuffer, attr.group_attr_size);
    if (ret != PS_OK) {
        if (ret == PS_GROUP_NOT_FOUND) {
            if (!quiet) {
                reporterr(ERRFAC, M_PSGNOTFOUND, ERRWARN, group_name);
            }
        } else {
            if (!quiet) {
                reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, group_name, ps_name, ret);
            }
        }
        exit(1);
    }

    /* if no args, then just dump the contents of the persistent store */
    if ((optind == argc) || Dflag) {   
        ps_dump_info(ps_name, group_name, inbuffer);
#ifdef TDMF_TRACE
        ftd_dump_drv_info(lgnum);
#endif
        free(inbuffer);
        exit(0);
    } 

    /* get current driver tunables */
    driver_ready = getdrvrtunables(lgnum, &cur_tunables);
 
    /* get keys and values */
    num_input=0;
    for (i=optind; i<argc; i++) {
        temp = argv[i];
        /* make sure we have an = */
        strtok(temp, "=");
        if (strtok(NULL, "=") != NULL) {
            strcpy (tempkey,temp);
            strcpy (tempval,temp + strlen(tempkey) + 1);
            valid = 0;
            for (j=0; j<max_valid; j++) {
                if (strcasecmp(tempkey, valid_key[j])==0) {
                    /* copy the upper case version over the input key */
                    strcpy (tempkey, valid_key[j]);
                    valid = 1;
                    break;
                }
            }
            if (!valid) {
                if (!quiet) {
                    reporterr(ERRFAC, M_BADTUNABLE, ERRWARN, tempkey);
                }
                exit (-1);
            }
            input_key[num_input] = (char *) ftdmalloc((strlen(tempkey) + 2)
                                              * sizeof(char));
            input_value[num_input] = (char *) ftdmalloc((strlen(tempval) + 1)
                                                        * sizeof(char));
            strcpy(input_key[num_input],tempkey);
            strcpy(input_key[num_input] + strlen(tempkey), ":");
            strcpy(input_value[num_input],tempval);

         if ((strcmp(input_key[num_input], "NETMAXKBPS:")) ==  0 && cur_tunables.syncmode ==1  ) {
                 group = lgnum;
                 sprintf(pmd, "PMD_%03d", group);
         	 if ( (getprocessid(pmd, 0, &pcnt)) > 0){
                   reporterr(ERRFAC, M_SETTUNABLE, ERRINFO); 
		   /* The following message will be displayed on the Console Event log window (pc070821) */
		   printf( "%s: error setting Network Usage Threshold (netmaxkbps); Kill PMD and retry\n", pmd );
                   exit(-1);
                 }
            }
            journal_state = cur_tunables.use_journal;
            if ( -1 == verify_and_set_tunable(input_key[num_input], 
                                   input_value[num_input], &cur_tunables, 1, NULL) ) {
                exit (-1);
            }
            
            /*
             * Check the logic group state for setting JOURNAL ON/OFF. This
             * code is redundant to checking done in verify_and_set_tunable()
             * but it does not check group state in verify_and_set_tunable().
             */
            if ((strcmp(input_key[num_input], "JOURNAL:")) == 0) {
                if ((driver_mode = get_driver_mode(lgnum)) == -1) {
                    ftd_trace_flow(FTD_DBG_FLOW1,
                            "can not get lg(%ld) driver mode, "
                            "continue save to PStore\n",
                            lgnum);
                    driver_mode_disallow = 1;
                }

                if ((strcmp(input_value[num_input], "ON") == 0) ||
                    (strcmp(input_value[num_input], "on") == 0) ||
                    (strcmp(input_value[num_input], "1") == 0)) {
                    setjournal = 1;
                } else if ((strcmp(input_value[num_input], "OFF") == 0) ||
                           (strcmp(input_value[num_input], "off") == 0) ||
                           (strcmp(input_value[num_input], "0") == 0)) {
                    setjournal = 0;
                }

                if ((driver_mode != FTD_MODE_TRACKING) && 
                    (driver_mode != FTD_MODE_PASSTHRU)) {
                    ftd_trace_flow(FTD_DBG_FLOW1,
                            "lg(%ld) driver mode: %ld, "
                            "continue save to PStore\n",
                            lgnum, driver_mode);
                    driver_mode_disallow = 1;

                    if (setjournal == 1) {
                        reporterr(ERRFAC, M_JOURNAL_ON, ERRWARN);
                    } else {
                        reporterr(ERRFAC, M_JOURNAL_OFF, ERRWARN);
                    }
                }

#if 0
                if ((driver_ready != -1) && (driver_mode != -1)) {
                    if ((setjournal == 1) && (journal_state == 1)) {
                        ftd_trace_flow(FTD_DBG_FLOW1,
                                    "Journal is already on\n");
                        exit(0);
                    } else if ((setjournal == 0) && (journal_state == 0)) {
                        ftd_trace_flow(FTD_DBG_FLOW1,
                                    "Journal is already off\n");
                        exit(0);
                    }
                }
#endif
            }

            num_input++;
        } else {
            usage();
        }
    }
    
    /* now set each tunable */
    for (i=0; i< num_input; i++) {
        if ((strncmp(input_key[i], "JOURNAL", strlen(input_key[i])-1) == 0) &&
            (driver_mode_disallow == 1)) {
            flag = 0x10;    /* update pstore only */
        } else {
            flag = 0x11;    /* update both driver & pstore */
        }

        if (settunable(lgnum, input_key[i], input_value[i], flag)!=0) {
            strncpy(tempkey, input_key[i], strlen(input_key[i])-1);
            *(tempkey+strlen(tempkey)) = '\0';
            if (!quiet) {
                reporterr(ERRFAC, M_SETFAIL, ERRCRIT, tempkey, lgnum);
            }

            /*
             * save change in tmp file for next dtcstart to take effect
             */
            ftdset_set_tunable_cfg_file(lgnum, input_key[i], input_value[i]);

            exit (-1);
        }

#ifdef UPDATE_P_FILE
        if (strncpy("JOURNAL", input_key[i], strlen(input_key[i])-1); == 0) {
            if ((rc = ftdset_set_cfg_file(lgnum,
                                          input_key[i],
                                          input_value[i])) < 0) {
                exit(rc);
            }
        }
#endif

        free(input_key[i]);
        free(input_value[i]);
    }

    for (i=0; i< max_valid; i++) {
        free(valid_key[i]);
    }

    free(inbuffer);

    exit(0);
    return 0; /* for stupid compiler */
}

