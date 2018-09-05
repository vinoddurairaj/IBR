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

#include "cfg_intr.h"
#include "ps_intr.h"
#include "ftdio.h"
#include "ftd_cmd.h"
#include "errors.h"
#include "config.h"
#include "pathnames.h"
#include "common.h"

static char *progname;
char *argv0;

static char *input_key[FTD_MAX_KEY_VALUE_PAIRS];
static char *input_value[FTD_MAX_KEY_VALUE_PAIRS];


static void
usage(void)
{
    fprintf(stderr, "Usage: %s -g group_number [ key=value ...]\n", progname);
    exit(1);
}

int 
main(int argc, char *argv[])
{
    char  *inbuffer, *temp, *endline, *p;
    char  group_name[MAXPATHLEN];
    char  ps_name[MAXPATHLEN];
#if defined(_AIX)
    int  ch;
#else /* defined(_AIX) */
    char ch;
#endif /* defined(_AIX) */
    char tempkey[30];
    char tempval[30];
    int   num_input, valid;
    int  i, j, ret, lgnum;
    int quiet = 0;
    int group;
    ps_version_1_attr_t attr;
    char  tunablebuffer[512];
    char *tp, *tokptr;
    char *valid_key [FTD_MAX_KEY_VALUE_PAIRS];
    int max_valid;
    tunable_t dummy_tunables;
    char cur_cfg[9];

    progname = argv[0];    
    argv0 = argv[0];

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
        if (tp = strchr(tp,'\n')) {
            tp++;
        } else {
            tp = NULL;
        }
        i++;
    }
    max_valid=i;
    group = -1;
    
    opterr = 0;
    while ((ch = getopt(argc, argv, "qg:"))!=-1) {
        switch (ch) {
        case 'q': 
            quiet = 1;
            break;
        case 'g': 
            group = strtol(optarg, NULL, 0);
            break;
        default:
            usage();
            break;
        }
    }
    
    if (group == -1)
        usage();

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    FTD_CREATE_GROUP_NAME(group_name, group);

    lgnum = group;

    sprintf(cur_cfg, "p%03d.cfg", lgnum);
    if (GETPSNAME(lgnum, ps_name) != 0) {
        if (!quiet) {
            reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, cur_cfg);
        }
        exit(1);
    }

    /* read the persistent store for info on this group */
    ret = ps_get_version_1_attr(ps_name, &attr);
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
                reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, group_name, ps_name);
            }
        }
        exit(1);
    }

    /* if no args, then just dump the contents of the persistent store */
    if (optind == argc) {   
        p = inbuffer;
        while (*p!=NULL) {
            endline = strchr (p, '\n');
            if (*p=='_') {
                while ((p!=endline) && (*p!='\0')) {
                    p++;
                }
                p++; 
            } else {
                while (p!=endline) {
                    putc(*p,stdout);
                    p++;
                } 
                putc(*p,stdout);
                p++;
            }
        }
        free(inbuffer);
        exit(0);
    } 
    /* get current driver tunables */
    getdrvrtunables(lgnum, &dummy_tunables);
 
    /* get keys and values */
    num_input=0;
    for (i=optind; i<argc; i++) {
        temp = argv[i];
        /* make sure we have an = */
        if (strtok(temp, "=")!=NULL) {
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

            if ( -1 == verify_and_set_tunable(input_key[num_input], 
                                   input_value[num_input], &dummy_tunables) ) {
                exit (-1);
            }
            
            num_input++;
        } else
            usage();
    }
    
    /* now set each tunable */
    for (i=0; i< num_input; i++) {
        if (settunable(lgnum,input_key[i],input_value[i])!=0) {
            strncpy(tempkey, input_key[i], strlen(input_key[i])-1);
            *(tempkey+strlen(tempkey)) = '\0';
            if (!quiet) {
                   reporterr(ERRFAC, M_SETFAIL, ERRCRIT, tempkey, lgnum);
            }
            exit (-1);
        }
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

