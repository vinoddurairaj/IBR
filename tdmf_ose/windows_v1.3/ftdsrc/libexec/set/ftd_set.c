/*
 * ftd_set.c - set/get parameters in the persistent store and driver
 *
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 *
 */

// Include this library to force the linker to link the MFC libraries in the correct order Mike Pollett
#include "../../forcelib.h"

#if defined(_WINDOWS) && defined(_DEBUG)
#include <conio.h>
#endif
#include <ctype.h>             /* for toupper() */ 
#include "ftd_port.h"
#include "ftd_lg.h"
#include "ftd_error.h"
#include "ftd_ps.h"
#include "ftd_sock.h"
#include "llist.h"

#if defined(_WINDOWS)
#include "ftd_devlock.h"
#endif

#if defined(_WINDOWS) 
//libftd, ftd_mngt_msgs.cpp
void ftd_mngt_msgs_log(const char **argv, int argc);
#endif

typedef struct keyval_s {
    int  keylen;
    char *key;
    char *val;
} keyval_t;

static keyval_t keyval, *keyvalp;
static LList    *keyvallist = NULL;

char	szKeyList[8][16] = {"CHUNKSIZE", "CHUNKDELAY", "SYNCMODE", "SYNCMODEDEPTH", 
							"SYNCMODETIMEOUT", "COMPRESSION", "REFRESHTIMEOUT", "JOURNAL"};
#define MAX_KEY_LIST 8

static void
usage(char **argv)
{
    fprintf(stderr, "Usage: %s -g group_number [ key=value ...]\n",
         argv[0]);
    return;
}

int validateKey(char *szKey)
{
    int iRc = 0;
    int i = 0;

    for(i = 0; i < MAX_KEY_LIST; i++)
    {
        iRc = strcmp(szKey, szKeyList[i]);
        if(iRc == 0)
        {
            break;
        }
        else
        {
            iRc = -1;
        }
    }

    return iRc;
}

int 
main(int argc, char *argv[])
{
    FILE        *fd = stdout;
    ftd_lg_t    *lgp;
    char        tempkey[64];
    int         iRc = 0;
    char        *pArgString;
    int         pArgStringLength;

#if defined(_AIX)
    int         ch;
#else /* defined(_AIX) */
    char        ch;
#endif /* defined(_AIX) */
    int         i, j, rc, lgnum, output = 0, quiet = 0;

////------------------------------------------
#if defined(_WINDOWS)
    void ftd_util_force_to_use_only_one_processor();
    ftd_util_force_to_use_only_one_processor();
#endif
//------------------------------------------

    lgnum = -1;

    while ((ch = getopt(argc, argv, "oqg:"))!=-1) {
        switch (ch) {
        case 'o': 
            output = 1;
            break;
        case 'q': 
            quiet = 1;
            break;
        case 'g': 
            lgnum = strtol(optarg, NULL, 0);
            break;
        default:
            usage(argv);
            goto errexit;
        }
    }
    
    if (lgnum == -1) {
        usage(argv);
        goto errexit;
    }
#if defined(_WINDOWS)
    if (ftd_sock_startup() == -1) {
        goto errexit;
    }
    if (ftd_dev_lock_create() == -1) {
        goto errexit;
    }
#endif

    if (ftd_init_errfac("Replicator", argv[0], NULL, NULL, 0, 1) == NULL) {
        goto errexit;
    }

#if defined(_WINDOWS) 
    //send status msg to System Event Log and TDMF Collector
    ftd_mngt_msgs_log(argv, argc);
#endif

    if ((lgp = ftd_lg_create()) == NULL) {
        goto errexit;
    }

	if (ftd_lg_init(lgp, lgnum, ROLEPRIMARY, 0) < 0) {
        goto errexit;
    }

    if (output)
    {
        char path[256];

        sprintf(path, "%s\\pstore.txt", PATH_CONFIG);
        if ( (fd = fopen(path, "w")) == NULL)
            goto errexit;
    }

    /* if no args, then just dump the contents of the persistent store */
    if (optind == argc) {   
        ftd_lg_dump_pstore_attr(lgp, fd);

    if (output)
        fclose(fd);

        ftd_delete_errfac();

#if defined(_WINDOWS)
        ftd_dev_lock_delete();
        ftd_sock_cleanup();
#endif
        exit(0);
    } 

    keyvallist = CreateLList(sizeof(keyval_t));

    for (i = optind; argv[i]; i++) {
        memset(&keyval, 0, sizeof(keyval));
        if ((keyval.key = strtok(argv[i], "=")) == NULL) {
            usage(argv);
            goto errexit;
        }
        keyval.key = strdup(keyval.key);
        if ((keyval.val = strtok(NULL, "\n\t\0")) == NULL) {
            usage(argv);
            goto errexit;
        }

        /* Convert the keyval.key string to UPPERCASE, because getopt() convert argv[i] to lowercase!!! */
        pArgString = keyval.key; /* set this to the current argument ptr */ 
        pArgStringLength = strlen(pArgString);      
        do
            pArgString[pArgStringLength - 1] = toupper(pArgString[pArgStringLength - 1]);
        while (--pArgStringLength);
        /* 030120 rddev */

        iRc = validateKey(keyval.key);
        if(iRc < 0)
        {
            printf("Invalid key: %s\n", keyval.key);
        }

        keyval.val = strdup(keyval.val);
        keyval.keylen = strlen(keyval.key);

        // DTurrin - Sept 5th, 2001
        // Added a check to make sure that the max value for
        // REFRESHTIMEOUT is 8639999 (ie. 99/23/59/59)
        if (strcmp(keyval.key,"REFRESHTIMEOUT") == 0)
        {
            long keyValue = strtol(keyval.val,NULL,0);
            if (keyValue > 8639999)
            {
                keyval.val = "8639999";
                keyval.val = strdup(keyval.val);
            }
        }

        AddToTailLL(keyvallist, &keyval);
    }

    ForEachLLElement(keyvallist, keyvalp) { 
        for (j = 0; j < keyvalp->keylen + 1; j++) {
            tempkey[j] = toupper(keyvalp->key[j]);
        }
        strcat(tempkey, ":\0");
        rc = ftd_lg_set_state_value(lgp, tempkey, keyvalp->val);
        if (rc < 0) {
            printf("\nERROR: Check error log!\n");
            if (rc == -2) {
                if (!quiet) {
                    keyvalp->key[keyvalp->keylen] = '\0';
                    reporterr(ERRFAC, M_BADTUNABLE, ERRWARN,
                        keyvalp->key);
                    printf("\n %s: %s\n", M_BADTUNABLE, keyvalp->key);
                }
            }
            FreeLList(keyvallist);
            keyvallist = NULL;
            goto errexit;
        }
    }

    FreeLList(keyvallist);
            
    ftd_delete_errfac();

#if defined(_WINDOWS)
    ftd_dev_lock_delete();
    ftd_sock_cleanup();
#endif

    exit(0);

errexit:

    if (keyvallist) {
        FreeLList(keyvallist);
    }        
    ftd_delete_errfac();

#if defined(_WINDOWS)
    ftd_dev_lock_delete();
    ftd_sock_cleanup();
#endif

    exit(1);

    return 0; /* for stupid compiler */
}

