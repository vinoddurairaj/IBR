/*
 * ftd_config.c - FTD config file interface
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
#include <string.h>
#include "ftd_port.h"
#include "ftd_throt.h"
#include "ftd_config.h"
#include "ftd_error.h"
#include "ftd_ps.h"
#include "ftd_ioctl.h"
#include "misc.h"
#include "llist.h"
#include "volmntpt.h"

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifdef DEBUG_THROTTLE
FILE* throtfd;
FILE* oldthrotfd;
#endif /* DEBUG_THROTTLE */

/* internal prototypes */
static int config_get(char *cfgpath, int role, int startflag, LList *cfglist); 
static int config_init(ftd_lg_cfg_t *cfg, int lgnum, int role, int startflag);
static int config_init_2(ftd_lg_cfg_t *cfg, int lgnum, int role);
static int getline(FILE *fd, LineState *ls);
static void get_word (FILE *fd, LineState *ls);
static void drain_to_eol (FILE *fd, LineState* ls);
static void forget_throttle(ftd_lg_cfg_t *cfg, throttle_t* throttle);
static int verify_primary_entries(ftd_lg_cfg_t *cfg);
static int verify_secondary_entries(ftd_lg_cfg_t *cfg);

extern int parse_throttles ( ftd_lg_cfg_t* cfg, LineState* ls);
static int parse_system    ( ftd_lg_cfg_t* cfg, LineState* ls);
static int parse_profile   ( ftd_lg_cfg_t* cfg, BOOL bIsWindows, LineState* ls);
static int parse_notes     ( ftd_lg_cfg_t* cfg, LineState* ls);

/*
 * ftd_config_create_list --
 * returns an unitialized linked list of ftd_lg_cfg_t objects
 */
LList *
ftd_config_create_list(void) 
{
    
    LList *cfglist = CreateLList(sizeof(ftd_lg_cfg_t**));

    return cfglist;
}

/*
 * ftd_config_delete_list --
 * returns an unitialized linked list of ftd_lg_cfg_t objects
 */
int
ftd_config_delete_list(LList *cfglist) 
{
    ftd_lg_cfg_t    **cfgpp;
 
    ForEachLLElement(cfglist, cfgpp) {
        ftd_config_lg_delete((*cfgpp));
    } 

    FreeLList(cfglist);
    cfglist = NULL;

    return 0;
}

/*
 * ftd_config_lg_add_to_list --
 * add ftd_lg_cfg_t object to linked list 
 */
int
ftd_config_lg_add_to_list(LList *cfglist, ftd_lg_cfg_t **cfgpp) 
{
    
    AddToTailLL(cfglist, cfgpp);

    return 0;
}

/*
 * ftd_config_lg_remove_from_list --
 * add ftd_lg_cfg_t object to linked list 
 */
int
ftd_config_lg_remove_from_list(LList *cfglist, ftd_lg_cfg_t **cfgpp) 
{
    
    RemCurrOfLL(cfglist, cfgpp);

    return 0;
}

/*
 * ftd_config_dev_add_to_list --
 * add ftd_dev_cfg_t object to linked list 
 */
int
ftd_config_dev_add_to_list(LList *cfglist, ftd_dev_cfg_t **devcfgpp) 
{
    
    //
    // This is one place where we add to the list of objects
    // may be related to WR 20361
    //

    AddToTailLL(cfglist, devcfgpp);

    return 0;
}

/*
 * ftd_config_dev_remove_from_list --
 * add ftd_dev_cfg_t object to linked list 
 */
int
ftd_config_dev_remove_from_list(LList *cfglist, ftd_dev_cfg_t **devcfgpp) 
{
    
    RemCurrOfLL(cfglist, devcfgpp);

    return 0;
}

/*
 * ftd_config_dev_create --
 * returns an unitialized ftd_dev_cfg_t object
 */
ftd_dev_cfg_t *
ftd_config_dev_create(void) 
{
    ftd_dev_cfg_t   *devcfgp;
    
    if ((devcfgp = (ftd_dev_cfg_t*)calloc(1, sizeof(ftd_dev_cfg_t))) == NULL) {
        return NULL;
    }

    devcfgp->magicvalue = FTDCFGMAGIC;

    return devcfgp;
}

/*
 * ftd_config_dev_delete --
 * delete the ftd_dev_cfg_t object
 */
int
ftd_config_dev_delete(ftd_dev_cfg_t *devcfgp) 
{
    
    if (devcfgp && devcfgp->magicvalue != FTDCFGMAGIC) {
        // not a valid config object
        return -1;
    }
  
    free(devcfgp);
 
    return 0;
}

/*
 * ftd_config_lg_create --
 * returns an unitialized ftd_lg_cfg_t object
 */
ftd_lg_cfg_t *
ftd_config_lg_create(void) 
{
    ftd_lg_cfg_t    *cfgp;
    
    if ((cfgp = (ftd_lg_cfg_t*)calloc(1, sizeof(ftd_lg_cfg_t))) == NULL) {
        return NULL;
    }

    return cfgp;
}

/*
 * ftd_config_lg_cleanup --
 * remove the ftd_lg_cfg_t object state
 */
int
ftd_config_lg_cleanup(ftd_lg_cfg_t *cfgp) 
{
    ftd_dev_cfg_t   **devcfgpp;
 
    if (cfgp && cfgp->magicvalue != FTDCFGMAGIC) {
        // not a valid config object
        return -1;
    }
   
    if (cfgp->throttles) {
        FreeLList(cfgp->throttles);
    }
    if (cfgp->devlist) {
        ForEachLLElement(cfgp->devlist, devcfgpp) {
            ftd_config_dev_delete((*devcfgpp));
        }
        FreeLList(cfgp->devlist);
    }
    if (cfgp->cfgfd != NULL) {
        fclose(cfgp->cfgfd);
        cfgp->cfgfd = NULL;
    }

    return 0;
}

/*
 * ftd_config_lg_delete --
 * delete the ftd_lg_cfg_t object
 */
int
ftd_config_lg_delete(ftd_lg_cfg_t *cfgp) 
{
 
    if (ftd_config_lg_cleanup(cfgp) < 0) {
        // not a valid config object
        return -1;
    }
   
    free(cfgp); 
    cfgp = NULL;

    return 0;
}

/*
 * ftd_config_get_all --
 * returns the sorted list of all configuration files.
 */
int
ftd_config_get_all(char *cfgpath, LList *cfglist) 
{
    
    return config_get(cfgpath, -1, 0, cfglist);
}

/*
 * ftd_config_get_primary --
 * returns the sorted list of primary configuration files.
 */
int
ftd_config_get_primary(char *cfgpath, LList *cfglist) 
{

    return config_get(cfgpath, ROLEPRIMARY, 0, cfglist);
}

/*
 * ftd_config_get_primary_started --
 * returns the sorted list of primary, started config files.
 */
int
ftd_config_get_primary_started(char *cfgpath, LList *cfglist) 
{
    return config_get(cfgpath, ROLEPRIMARY, 1, cfglist);
}

/*
 * ftd_config_get_secondary --
 * returns the sorted list of secondary configuration files.
 */
int
ftd_config_get_secondary(char *cfgpath, LList *cfglist) 
{
    return config_get(cfgpath, ROLESECONDARY, 0, cfglist);
}

/*
 * ftd_config_read --
 * parse a configuration file
 */
int
ftd_config_read(ftd_lg_cfg_t *cfgp, BOOL bIsWindowsFile, int lgnum, int role, int startflag)
{
    LineState       lstate;
    int             rc = 0;
    
    ftd_config_lg_cleanup(cfgp);

    /* initialize the config object */
    if ( (rc = config_init(cfgp, lgnum, role, startflag)) != 0 )
        return rc;

    if ((cfgp->cfgfd = fopen(cfgp->cfgpath, "r")) == NULL)     {
        reporterr(ERRFAC, M_CFGFILE, ERRCRIT,
            cfgp->cfgpath, ftd_strerror());
        return FTD_CFG_NOT_FOUND;
    }

    /* initialize the parsing state */
    lstate.lineno = 0;
    lstate.invalid = TRUE;

    while (1) {
        /* if we need a line and don't get it, then we're done */
        if (!getline(cfgp->cfgfd, &lstate)) {
            break;
        }
        /* major sections of the file */
#if !defined(_WINDOWS)
        if (strncmp("THROTTLE", lstate.key, 8) == 0) {
            if ((rc = parse_throttles (cfgp, &lstate)) < 0) {
                break;
            }
        } else 
#endif          
        if (strcmp("SYSTEM-TAG:", lstate.key) == 0) {
            if ((rc = parse_system (cfgp, &lstate)) < 0) {
                break;
            }
        } else if (strcmp("PROFILE:", lstate.key) == 0) {
            if ((rc = parse_profile (cfgp, bIsWindowsFile, &lstate)) < 0) {
                reporterr(ERRFAC, M_CFGERR, ERRCRIT, cfgp->cfgpath, lstate.lineno); // ardeb 020912
                break;
            }
        } else if (strcmp("NOTES:", lstate.key) == 0) {
           continue;
        } else {
            reporterr(ERRFAC, M_CFGBDKEY, ERRCRIT, cfgp->cfgpath, 
                lstate.lineno, lstate.key);
            rc = FTD_CFG_BAD_KEY;
            break;
        }
    }
    fclose(cfgp->cfgfd);
    cfgp->cfgfd = NULL;

    if (rc < 0)
        return rc;

    if (cfgp->role == ROLEPRIMARY)     {
        if ((rc = verify_primary_entries(cfgp)) != 0) {
            return rc;
        }
    } else if (cfgp->role == ROLESECONDARY)     {
        if ((rc = verify_secondary_entries(cfgp)) != 0) {
            return rc;
        }
    }

    cfgp->magicvalue = FTDCFGMAGIC;

    return rc;
}

/*
 * ftd_config_read_2 --
 * parse a configuration file
 * cfgp->cfgpath MUST be already set to cfg file name to be read.
 */
int
ftd_config_read_2(ftd_lg_cfg_t *cfgp, BOOL bIsWindowsFile, int lgnum, int role)
{
    LineState       lstate;
    int             rc = 0;
    
    ftd_config_lg_cleanup(cfgp);

    /* initialize the config object */
    if ( (rc = config_init_2(cfgp, lgnum, role)) != 0 )
        return rc;

    if ((cfgp->cfgfd = fopen(cfgp->cfgpath, "r")) == NULL) {
        reporterr(ERRFAC, M_CFGFILE, ERRCRIT,
            cfgp->cfgpath, ftd_strerror());
        return FTD_CFG_NOT_FOUND;
    }

    /* initialize the parsing state */
    lstate.lineno = 0;
    lstate.invalid = TRUE;

    while (1) {
        /* if we need a line and don't get it, then we're done */
        if (!getline(cfgp->cfgfd, &lstate)) {
            break;
        }
        /* major sections of the file */
        if (strncmp("THROTTLE", lstate.key, 8) == 0) {
            if ((rc = parse_throttles (cfgp, &lstate)) < 0) {
                break;
            }
        } 
        else 
        if (strcmp("SYSTEM-TAG:", lstate.key) == 0) {
            if ((rc = parse_system (cfgp, &lstate)) < 0) {
                break;
            }
        } else if (strcmp("PROFILE:", lstate.key) == 0) {
            if ((rc = parse_profile (cfgp, bIsWindowsFile, &lstate)) < 0) {
                break;
            }
        } else if (strcmp("NOTES:", lstate.key) == 0) {
            if ((rc = parse_notes (cfgp, &lstate)) < 0) {
                break;
            }
        } else {
            /*reporterr(ERRFAC, M_CFGBDKEY, ERRCRIT, cfgp->cfgpath, 
                lstate.lineno, lstate.key);*/
            rc = FTD_CFG_BAD_KEY;
            break;
        }
    }
    fclose(cfgp->cfgfd);
    cfgp->cfgfd = NULL;

    if (rc < 0)
        return rc;

    cfgp->magicvalue = FTDCFGMAGIC;

    return rc;
}

/*
 * ftd_config_get_psname -
 * retrieves ps_name from cfg file for a lg
 */
int
ftd_config_get_psname(ftd_lg_cfg_t *cfgp)
{
    LineState   lstate; 
    int         error;

    lstate.lineno = 0;
    lstate.invalid = TRUE;
    error = 0;
    while (1) {
        /* if we need a line and don't get it, then we're done */
        if (!getline(cfgp->cfgfd, &lstate)) {
            break;
        }
        if (strcmp("PSTORE:", lstate.key) == 0) {
            strncpy(cfgp->pstore, lstate.p1, sizeof(cfgp->pstore));
            error_tracef( TRACEINF, "ftd_config_get_psname():%s", cfgp->pstore );
            return 0;
        }
    }

    return -1;
}

/*
 * config_init --
 * initialize cfg object
 */
static int
config_init(ftd_lg_cfg_t *cfgp, int lgnum, int role, int startflag)
{
    char    *cfg_suffix;

    memset(cfgp->pstore, 0, sizeof(cfgp->pstore));
    memset(cfgp->phostname, 0, sizeof(cfgp->phostname));
    memset(cfgp->shostname, 0, sizeof(cfgp->shostname));
    memset(cfgp->notes, 0, sizeof(cfgp->notes));

    cfgp->lgnum = lgnum;
    cfgp->role = role;

    if ((cfgp->throttles = CreateLList(sizeof(throttle_t))) == NULL) {
        return -1;
    }
    if ((cfgp->devlist = CreateLList(sizeof(ftd_dev_cfg_t**))) == NULL) {
        return -1;
    }
    
    if (cfgp->role == ROLESECONDARY) {
#if defined (_WINDOWS)
        sprintf(cfgp->cfgpath, "%s\\%s%03d.cfg", 
            PATH_CONFIG, SECONDARY_CFG_PREFIX, cfgp->lgnum); 
#else
        sprintf(cfgp->cfgpath, "%s/%s%03d.cfg", 
            PATH_CONFIG, SECONDARY_CFG_PREFIX, cfgp->lgnum); 
#endif
    } else {
        if (startflag) {
            cfg_suffix = "cur";
        } else {
            cfg_suffix = "cfg";
        }
#if defined (_WINDOWS)
        sprintf(cfgp->cfgpath, "%s\\%s%03d.%s",
            PATH_CONFIG, PRIMARY_CFG_PREFIX, cfgp->lgnum, cfg_suffix); 
#else
        sprintf(cfgp->cfgpath, "%s/%s%03d.%s",
            PATH_CONFIG, PRIMARY_CFG_PREFIX, cfgp->lgnum, cfg_suffix); 
#endif
    }
    
    return 0;
}

/*
 * config_init_2 --
 * initialize cfg object
 */
static int
config_init_2(ftd_lg_cfg_t *cfgp, int lgnum, int role)
{
    memset(cfgp->pstore, 0, sizeof(cfgp->pstore));
    memset(cfgp->phostname, 0, sizeof(cfgp->phostname));
    memset(cfgp->shostname, 0, sizeof(cfgp->shostname));
    memset(cfgp->notes, 0, sizeof(cfgp->notes));

    cfgp->lgnum = lgnum;
    cfgp->role = role;

    if ((cfgp->throttles = CreateLList(sizeof(throttle_t))) == NULL) {
        return -1;
    }
    if ((cfgp->devlist = CreateLList(sizeof(ftd_dev_cfg_t**))) == NULL) {
        return -1;
    }

    //cfgp->cfgpath must be already initialized.  that's the deal.
    return 0;
}

/*
 * config_get -- returns the sorted list of primary configuration files.
 */
static int
config_get(char *cfgpath, int role, int startflag, LList *cfglist) 
{
    char            lgnumstr[4], cfg_path_suffix[4], *cfg_path_prefix;
    int             i;
    HANDLE          hFile, ctlfd = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA data;
    BOOL            bFound = TRUE;
    ftd_lg_cfg_t    *cfgp = NULL;
    char            path[_MAX_PATH];
    char    buf[FTD_PS_GROUP_ATTR_SIZE];

    switch ( role ) 
    {
        case ROLEPRIMARY:
            cfg_path_prefix = PRIMARY_CFG_PREFIX;
            if (startflag) 
                strcpy(cfg_path_suffix, PATH_STARTED_CFG_SUFFIX); /* .cur files verification */
            else
                strcpy(cfg_path_suffix, PATH_CFG_SUFFIX);         /* .cfg files verification */

            break;
        case ROLESECONDARY:
            cfg_path_prefix = SECONDARY_CFG_PREFIX;
            strcpy(cfg_path_suffix, PATH_CFG_SUFFIX);             /* .cfg files verification */
            break;
        default:
            break;
    }

    strcpy(path, cfgpath);
    strcat(path, "\\");
    strcat(path, cfg_path_prefix);
    strcat(path, "*.");
    strcat(path, cfg_path_suffix);

    i = 0;
    hFile = FindFirstFile(path, &data);

    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    if (startflag) 
    {
        /* open ctl device */
        if ((ctlfd = ftd_open(FTD_CTLDEV, O_RDWR, 0)) == INVALID_HANDLE_VALUE) 
            return 0;
    }

    /* Create the cfglist for each LG present */
    while (bFound) 
    {
        if (role == -1 || 0 == strncmp(data.cFileName, cfg_path_prefix, strlen(cfg_path_prefix))) 
        {
            if ((cfgp = ftd_config_lg_create()) == NULL) 
                goto errret;
                
            sprintf(cfgp->cfgpath, "%s\\%s", cfgpath, data.cFileName);
            strncpy(lgnumstr, data.cFileName + strlen(cfg_path_prefix), 3);
            lgnumstr[3] = '\0';
            cfgp->lgnum = atoi(lgnumstr);
                
            if (startflag) /* *.cur file mapping */
            {
                if (ftd_ioctl_get_lg_state_buffer(ctlfd, cfgp->lgnum, buf, 1) < 0)
                {
                    // delete the .cur file
                    unlink(cfgp->cfgpath);
                    /* Modify the file name suffix .cur to .cfg */
                    strncpy( cfgp->cfgpath + ( strlen( cfgp->cfgpath) - 3 ), PATH_CFG_SUFFIX, 3 );
                }       
            }

            cfgp->magicvalue = FTDCFGMAGIC;

            ftd_config_lg_add_to_list(cfglist, &cfgp);
        }        
        bFound = FindNextFile(hFile, &data); 
    }

errret:

    SortLL(cfglist, stringcompare_addr);

    FindClose(hFile);

    if (ctlfd != INVALID_HANDLE_VALUE) {
        FTD_CLOSE_FUNC(__FILE__,__LINE__,ctlfd);
    }

    return i;
}

/*
 * getline -- reads and parses the next line of the config file
 */
static int 
getline (FILE *fd, LineState *ls)
{
    int i, len;
    int blankflag;
    char *line;
    
    if (!ls->invalid) {
        ls->invalid = TRUE;
        return TRUE;
    }
    ls->invalid = TRUE;
    
    ls->key = ls->p1 = ls->p2 = ls->p3 = ls->p4 = NULL;
// SAUMYA_FIX_CONFIG_FILE_PARSING
#if 0
	ls->p5 = ls->p6 = ls->p7 = NULL;
#endif

    ls->word[0] = '\0';
    ls->linelen = 0;
    ls->linepos = 0;
    ls->plinepos = 0;
    line = ls->line;
    
    while (1) {
        if (fgets(line, 256, fd) == NULL) {
            return FALSE;
        }
        
        ls->lineno++;
        len = strlen(line);
        ls->linelen = len;
        if (len < 5) continue;
        
        /* ignore blank lines */
        blankflag = 1;
        for (i = 0; i < len; i++) {
            if (isgraph(line[i])) {
                blankflag = 0;
                break;
            }
        }
        if (blankflag) continue;
        
        strcpy(ls->readline, ls->line);
        
        /* -- get rid of leading whitespace -- */
        i = 0;
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) continue;

        /* -- if the first non-whitespace character is a "#", ignore the
           line */
        if (line[i] == '#') continue;
        
        /* -- accumulate the key */
        ls->key = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;
        
        /* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;
        
        /* -- accumulate first parameter */
        /* -- EXCEPTION for JOURNAL and PSTORE keys, copy the remaining string */
        if ((strcmp("JOURNAL:", ls->key) == 0) || (strcmp("PSTORE:", ls->key) == 0)){
            ls->p1 = &line[i];
            // Go to end of line and place NULL
            while ((i < len) && (line[i] != '\n')) i++;
            line[i--] = 0;
            // Remove previous whitespaces
            while (line[i] == ' ') line[i--] = 0;
            return TRUE;
        } else {
        ls->p1 = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;
        }

        /* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;
        
        /* -- accumlate parameter */
        ls->p2 = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;
        
        /* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;
        
        /* -- accumlate parameter */
        ls->p3 = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;
        
        /* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;
        
        /* -- accumlate parameter */
        ls->p4 = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;
// SAUMYA_FIX_CONFIG_FILE_PARSING
#if 0
        /* -- accumlate parameter */
        ls->p5 = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;

		/* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;
        
        /* -- accumlate parameter */
        ls->p6 = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;

		/* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;
        
        /* -- accumlate parameter */
        ls->p7 = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;
#endif // SAUMYA_FIX_CONFIG_FILE_PARSING

        break;
    }
/*
  //DPRINTF((ERRFAC,LOG_INFO,"LINE(%d): = %s\n", ls->lineno - 1, ls->readline));
  */
    return TRUE;
}

/*
 * get_word -- obtains the next word from the config file's line / file
 *              (and handles continuation characters)
 *
 */
static void
get_word (FILE* fd, LineState* ls)
{
    int i;
    
    i = 0;
    ls->word[i] = '\0';
    ls->plinepos = ls->linepos;
    while (ls->linepos < ls->linelen) {
        if (isspace(ls->readline[ls->linepos])) {
            ls->linepos++;
        } else {
            break;
        }
    }
    if (ls->linepos < ls->linelen) {
        if (ls->readline[ls->linepos] != '\"') {
            while (ls->linepos < ls->linelen) {
                if (0 == isspace(ls->readline[ls->linepos])) {
                    ls->word[i++] = ls->readline[ls->linepos++];
                    ls->word[i] = '\0';
                } else {
                    break;
                }
            }
        } else {
            /* -- quoted string */
            ls->linepos++;
            while (ls->linepos < ls->linelen) {
                if (ls->readline[ls->linepos] == '\\') {
                    ls->linepos++;
                    if (ls->linepos < ls->linelen) {
                        ls->word[i++] = ls->readline[ls->linepos++];
                        ls->word[i] = '\0';
                    }
                } else {
                    if (ls->readline[ls->linepos] == '\"') {
                        ls->linepos++;
                        break;
                    }
                    ls->word[i++] = ls->readline[ls->linepos++];
                    ls->word[i] = '\0';
                }
            }
        }
    }
    if (0 == strcmp(ls->word, "\\")) {
        i = ls->linepos;
        while (i < ls->linelen) {
            if (0 == isspace(ls->readline[i])) return;
            i++;
        }
        ls->word[0] = '\0';
        if (getline (fd, ls)) {
            get_word (fd, ls);
        }
    }
    return;
}

/*
 * drain_to_eol -- reads remaining words in the logical (continued) line until
 *                 line termination encountered
 */
static void
drain_to_eol (FILE* fd, LineState* ls)
{
    while (strlen(ls->word) > 0) 
        get_word (fd, ls);
}

/*
 * parse_value -- parse a throttle test value as a number, word, or a string
 */
static int
parse_value (LineState* ls)
{
    int i, j, len;
    int digitflag;
    int value;
    int sign;

    sign = 1;
    ls->valueflag = 0;
    if (0 == (len = strlen(ls->word))) return (0);
    digitflag = 1;
    value = 0;
    j=0;
    while (ls->word[j] == '-') {
        sign = -1;
        j++;
    }
    for (i=j; i<len; i++) {
        if ('0' <= ls->word[i] && '9' >= ls->word[i]) {
            value = (value * 10) + (int) (ls->word[i] - '0');
        } else {
            digitflag = 0;
            break;
        }
    }
    if (digitflag) {
        ls->valueflag = 1;
        ls->value = value * sign;
    } else {
        ls->valueflag = 0;
        ls->value = 0;
    }
    return (1);
}

/*
 * parse_devnum -- return the integer value of the datastar device name
 */
static int 
parse_devnum (char* path)
{
    char numstr[32];
    int retval;
    int len;
    int i;
    int starti;
    
    if ((path == (char*)NULL) || ((len = strlen(path)) == 0)) {
        return (-1);
    }
    starti = -1;
    retval = 0;
    for (i=len-1; i>=0; i--) {
        if (isdigit(path[i])) {
            starti = i;
        } else if (isspace(path[i])) {
            continue;
        } else {
            break;
        }
    }
    if (starti == -1) {
        return (-1);
    }
    memset(numstr, 0, sizeof(numstr));
    strcpy(numstr, &path[starti]);
    retval = atoi(numstr);

    return (retval);
}

/*
 * NOTES for parse_??? functions.
 * 1) Set ls->invalid to FALSE and return, if the key is unknown.
 *
 */

/*
 * parse_system -- Parse the line passed in. It defines the system type 
 *                 (PRIMARY or SECONDARY) for the subsequent data.
 */
static int
parse_system(ftd_lg_cfg_t *cfgp, LineState *ls)
{
    int role;

    /* we don't read a line; we just parse the one we have */
    if (strcmp("PRIMARY", ls->p2) == 0) {
        role = ROLEPRIMARY;
    } else if (strcmp("SECONDARY", ls->p2) == 0) {
        role = ROLESECONDARY;
    } else {
        reporterr(ERRFAC, M_CFGERR, ERRCRIT, cfgp->cfgpath, ls->lineno);
        return -1; 
    }

    while (1) {
        if (!getline(cfgp->cfgfd, ls)) {
            break;
        }
        if (strcmp("HOST:", ls->key) == 0) {
            if (role == ROLEPRIMARY) {
                strncpy(cfgp->phostname, ls->p1, sizeof(cfgp->phostname));
            } else {
                strncpy(cfgp->shostname, ls->p1, sizeof(cfgp->shostname));
            }
        } else if (strcmp("PSTORE:", ls->key) == 0) {
            strncpy(cfgp->pstore, ls->p1, sizeof(cfgp->pstore));
        } else if (strcmp("JOURNAL:", ls->key) == 0) {
            strncpy(cfgp->jrnpath, ls->p1, sizeof(cfgp->jrnpath));
        } else if (0 == strcmp("SECONDARY-PORT:", ls->key )) {
            cfgp->port = atoi(ls->p1);
        } else if (0 == strcmp("CHAINING:", ls->key )) {
            if ((strcmp("on", ls->p1) == 0 ) || (strcmp("ON",ls->p1)==0) ) {
                cfgp->chaining = 1;
            } else {
                cfgp->chaining = 0;
            }
        } else {
            ls->invalid = FALSE;
            break;
        }
    }
    /* we must have either the name or the IP address for the host */
    if (role == ROLEPRIMARY && strlen(cfgp->phostname) == 0) {
        reporterr(ERRFAC, M_BADHOSTNAM, ERRCRIT,
            cfgp->cfgpath, ls->lineno, cfgp->phostname);
        return -1; 
    }
    /* we must have either the name or the IP address for the host */
    if (role == ROLESECONDARY && strlen(cfgp->shostname) == 0) {
        reporterr(ERRFAC, M_BADHOSTNAM, ERRCRIT,
            cfgp->cfgpath, ls->lineno, cfgp->shostname);
        return -1; 
    }
    /* we must have secondary journal path */
    if (role == ROLESECONDARY) {
        if (strlen(cfgp->jrnpath) == 0) {
            reporterr(ERRFAC, M_JRNMISS, ERRCRIT, cfgp->cfgpath, ls->lineno);
            return -1;
        }
    }

    return 0;
}

/*
 * parse_profile -- Read zero or more lines of device definitions. The 
 *                  current line passed in is of no use. Read lines until 
 *                  we don't match the key or EOF.
 */
static int
parse_profile(ftd_lg_cfg_t *cfgp, BOOL bIsWindowsFile, LineState *ls)
{
    ftd_dev_cfg_t   *devcfgp;
    char            *rest;
    int             moreprofiles = TRUE;

    while (moreprofiles) {
        moreprofiles=FALSE;

        if ((devcfgp = ftd_config_dev_create()) == NULL) {
            return -1;
        }
      
        while (1)  {
          if (!getline(cfgp->cfgfd, ls)) {
            break;
          } 
          /*  create and link in a group profile to both systems */
          if (0 == strcmp("REMARK:", ls->key)) {
            rest = strtok(ls->readline, " \t");
            rest = strtok((char *)NULL, "\n");
            if (rest != (char *)NULL) {
              strncpy(devcfgp->remark, rest, sizeof(devcfgp->remark));
            }
          } else if (0 == strcmp("PRIMARY:", ls->key)) {
            /* do nothing */
          } else if (0 == strcmp("SECONDARY:", ls->key)) {
            /* do nothing */
          } else if ((0 == strcmp("TDMF-DEVICE:", ls->key)) ||
					 (0 == strcmp("DTC-DEVICE:", ls->key))) {
            strncpy(devcfgp->devname, ls->p1, sizeof(devcfgp->devname));
            devcfgp->devid = parse_devnum(devcfgp->devname);
#if defined(_WINDOWS)
            if (bIsWindowsFile)
            {
                devcfgp->devid = atoi(strrchr(devcfgp->devname, ':') + 1); // Point to last occurence of :
                *(strrchr(devcfgp->devname, ':') + 1) = '\0'; // get rid of the devid

                if ( strlen(devcfgp->devname) > 2 )
                    *(strrchr(devcfgp->devname, ':')) = '\0'; // get rid of : for Mount Point case
            }
#endif
          } else if (0 == strcmp("DATA-DISK:", ls->key)) {
            if ( (bIsWindowsFile &&
                 (ls->p1 == NULL || ls->p2 == NULL || ls->p3 == NULL || ls->p4 == NULL)) ||
                 (!bIsWindowsFile && ls->p1 == NULL) )
            {
              return -1;
            }
            strncpy(devcfgp->pdevname, ls->p1, sizeof(devcfgp->pdevname)); 
            if (bIsWindowsFile)
            {
                strncpy(devcfgp->pdriverid, ls->p2, sizeof(devcfgp->pdriverid));
                strncpy(devcfgp->ppartstartoffset, ls->p3, sizeof(devcfgp->ppartstartoffset));
                strncpy(devcfgp->ppartlength, ls->p4, sizeof(devcfgp->ppartlength));
				
// SAUMYA_FIX_CONFIG_FILE_PARSING
#if 0
				if (ls->p5 != NULL && ls->p6 != NULL && ls->p7 != NULL)
				{
					strncpy(devcfgp->symlink1, ls->p5, sizeof(devcfgp->symlink1));
					strncpy(devcfgp->symlink2, ls->p6, sizeof(devcfgp->symlink2));
					strncpy(devcfgp->symlink3, ls->p7, sizeof(devcfgp->symlink3));
				}
#endif // SAUMYA_FIX_CONFIG_FILE_PARSING

            }
          } else if (0 == strcmp("MIRROR-DISK:", ls->key)){
            if ( (bIsWindowsFile &&
                 (ls->p1 == NULL || ls->p2 == NULL || ls->p3 == NULL || ls->p4 == NULL)) ||
                 (!bIsWindowsFile && ls->p1 == NULL) )
            {
              return -1;
            }
            strncpy(devcfgp->sdevname, ls->p1, sizeof(devcfgp->sdevname)); 
            if (bIsWindowsFile)
            {
                strncpy(devcfgp->sdriverid, ls->p2, sizeof(devcfgp->sdriverid));
                strncpy(devcfgp->spartstartoffset, ls->p3, sizeof(devcfgp->spartstartoffset));
                strncpy(devcfgp->spartlength, ls->p4, sizeof(devcfgp->spartlength));
            }
          } else if (0 == strcmp("PROFILE:", ls->key)){
            moreprofiles=TRUE;
            break;
          } else if (0 == strcmp("THROTTLE:", ls->key)){
			  moreprofiles=FALSE;
			  break;
          } else { /* unknown key */
            reporterr(ERRFAC, M_CFGBDKEY, ERRCRIT, cfgp->cfgpath, ls->lineno, ls->key);
            ls->invalid = FALSE;
            return -1;
          }
        }
        
        /* We must have: sddevname, devname, mirname, and secondary journal */
        if (strlen(devcfgp->devname) == 0) {
          reporterr(ERRFAC, M_DEVMISS, ERRCRIT, cfgp->cfgpath, ls->lineno);
          return -1;
        }
        
        if (strlen(devcfgp->sdevname) == 0) {
          reporterr(ERRFAC, M_MIRMISS, ERRCRIT, cfgp->cfgpath, ls->lineno);
          return -1;
        }
        
        if (strlen(devcfgp->pdevname) == 0) {
          reporterr(ERRFAC, M_DEVMISS, ERRCRIT, cfgp->cfgpath, ls->lineno);
          return -1;
        }
        
        /* add device to devlist */
        ftd_config_dev_add_to_list(cfgp->devlist, &devcfgp);
    }

    return 0;
}

/*
 * parse_notes -- Read one line for the NOTES. The \0x0A and \0xD
 *                  are removed.
 *
 * 030114
 *
 */
static int
parse_notes ( ftd_lg_cfg_t* ppFtdLgCfg, LineState* ppLs )
{
    char lcaNotes [ MAXPATHLEN ];
    int  liC;
    int  liDest = 0;
    int  liSize = sizeof ( ppFtdLgCfg->notes );

    strncpy( ppFtdLgCfg->notes, ppLs->readline+strlen ( "NOTES:" ), liSize );

    for ( liC = liSize -1; liC >= 0; liC-- ) // remove the trailing ...
    {
        if (    ppFtdLgCfg->notes[liC] == 10
             || ppFtdLgCfg->notes[liC] == 13
             || ppFtdLgCfg->notes[liC] == '\t'
             || ppFtdLgCfg->notes[liC] == ' '
           )
        {
            ppFtdLgCfg->notes[liC] = '\0';
        }
        else if ( ppFtdLgCfg->notes[liC] != '\0' )
        {
            break;
        }
    }

    liSize = ++liC;

    for ( liC = 0; liC < liSize+1; liC++ ) // replace the TAB with whites
    {
        if ( liDest == MAXPATHLEN-1 )
        {
            lcaNotes [ liDest++ ] = '\0'; // Trunk it
            break;
        }
        else if ( ppFtdLgCfg->notes[liC] == '\t' )
        {
            lcaNotes [ liDest++ ] = ' ';
            lcaNotes [ liDest++ ] = ' ';
            lcaNotes [ liDest++ ] = ' ';
        }
        else
        {
            lcaNotes [ liDest++ ] = ppFtdLgCfg->notes[liC];
        }
    }

    strncpy ( ppFtdLgCfg->notes, lcaNotes, liDest );

    return 0;

} // parse_notes ()


/*
 * forget_throttle -- remove a throttle (and subsequent) definition from 
 *                    the linked list of throttles
 */
static void
forget_throttle (ftd_lg_cfg_t *cfg, throttle_t *throttle)
{
    throttle_t *t;
    
    if (cfg->throttles == NULL)
        return;

    if (SizeOfLL(cfg->throttles) <= 1) {
        FreeLList(cfg->throttles);
        cfg->throttles = NULL;
    } else {
        ForEachLLElement(cfg->throttles, t) {
            if (t == throttle) {
                /* found it */
                break;
            }
        }

	    /* remove it from the list */
	   DelCurrOfLL(cfg->throttles, t);
    }
}

/*
 * parse_throttles -- parse throttle definitions from the config file
 *                    and create the appropriate data structures from them
 */
static int
parse_throttles(ftd_lg_cfg_t *cfg, LineState *ls) 
{
    throttle_t      *throttle;
    throt_test_t    *ttest;
    action_t        *action;
    char            keyword[256], *tok;
    int             i, state, implied_do_flag;
#ifdef DEBUG_THROTTLE
    char            tbuf[256];
    int             nthrots;
#endif /* DEBUG_THROTTLE */
    
    throttle = (throttle_t*) NULL;
    
    state = 0; /* state is to look for keyword "THROTTLE" */
    /* parse the first word of a line -- */
    /* extract the keyword "THROTTLE", "ACTIONLIST", "ACTION"  */
#ifdef DEBUG_THROTTLE
    nthrots = 0;
    oldthrotfd = throtfd;
    sprintf(tbuf, PATH_RUN_FILES "/throt%03d.parse", cfg->lgnum);
    throtfd = fopen(tbuf, "w");
#endif /* DEBUG_THROTTLE */
    
    while (1) {
        get_word (cfg->cfgfd, ls);
        tok = ls->word;
        if (strlen(tok) == 0) {
            if (0 == getline(cfg->cfgfd, ls)) return (0);
            continue;
        }
        for (i=0; i<(int)strlen(tok); i++) {
            keyword[i] = toupper(tok[i]);
            keyword[i+1] = '\0';
        }
        /* pseudo switch statement on first word */
        if (0 == strncmp("THROTTLE", keyword, 8)) {
            /*=====     T H R O T T L E     =====*/
            if (state != 0 && state != 4 && state != 5) {
                //reporterr (ERRFAC, M_THROTSTA, ERRWARN, cfg->cfgpath, ls->lineno);
            }
            state = 1; /* state is have "THROTTLE", look for "ACTIONLIST" */
            /* find the last throttle definition structure and 
                add a new one */
            throttle = (throttle_t*) malloc (sizeof(throttle_t));
            throttle->n = (throttle_t*) NULL;
            throttle->day_of_week_mask = -1;
            throttle->day_of_month_mask = -1;
                throttle->end_of_month_flag = 0;
            throttle->from = (time_t) -1;
            throttle->to = (time_t) -1;
            throttle->num_throttest = 0;
            throttle->num_actions = 0;
            
            if (cfg->throttles == NULL) {
                cfg->throttles = CreateLList(sizeof(throttle_t));
            } else {
                /* parse the days of week / days of month specification */
                get_word (cfg->cfgfd, ls);
                tok = ls->word;
                if (strlen(ls->word) == 0) {
                    //reporterr (ERRFAC, M_THROTDOWM, ERRWARN, cfg->cfgpath, ls->lineno);
                    state = 5;
                    forget_throttle (cfg , throttle);
                    drain_to_eol (cfg->cfgfd, ls);
                    if (0 == getline(cfg->cfgfd, ls)) {
                        return (0);
                    }
                    continue;
                }
                if (0 != strcmp("-", tok)) {
                    if (0 == parse_dowdom (tok, throttle)) {
                        //reporterr (ERRFAC, M_THROTDOWM, ERRWARN, cfg->cfgpath, ls->lineno);
                        state = 5;
                        forget_throttle (cfg, throttle);
                        drain_to_eol (cfg->cfgfd, ls);
                        if (0 == getline(cfg->cfgfd, ls)) {
                            return (0);
                        }
                        continue;
                    }
                }
            }
#ifdef DEBUG_THROTTLE
            nthrots++;
            tbuf[0] = '\0';
            printdowdom (tbuf, throttle);
            fprintf (throtfd, "========================================\n");
            fprintf (throtfd, "(#%d throttle definition in %s)\n", 
                    nthrots, mysys->configpath);
            fprintf (throtfd, "THROTTLE %s ", tbuf);
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            /* parse the "from" and "to" time definitions */
            get_word (cfg->cfgfd, ls);
            tok = ls->word;
            if (0 != strcmp("-", tok)) {
                if (0 != parse_time(tok, &throttle->from)) {
                    //reporterr (ERRFAC, M_THROTTIM, ERRWARN, cfg->cfgpath, ls->lineno);
                    state = 5;
                    forget_throttle (cfg , throttle);
                    drain_to_eol (cfg->cfgfd, ls);
                    if (0 == getline(cfg->cfgfd, ls)) {
                        return (0);
                    }
                    continue;
                }
            }
#ifdef DEBUG_THROTTLE
            tbuf[0] = '\0';
            print_time (tbuf, throttle->from);
            fprintf (throtfd, "%s ", tbuf);
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            get_word (cfg->cfgfd, ls);
            tok = ls->word;
            if (0 != strcmp("-", tok)) {
                if (0 != parse_time(tok, &throttle->to)) {
                    //reporterr (ERRFAC, M_THROTTIM, ERRWARN, cfg->cfgpath, ls->lineno);
                    state = 5;
                    forget_throttle (cfg , throttle);
                    drain_to_eol (cfg->cfgfd, ls);
                    if (0 == getline(cfg->cfgfd, ls)) {
                        return (0);
                    }
                    continue;
                }
            }
#ifdef DEBUG_THROTTLE
            tbuf[0] = '\0';
            print_time (tbuf, throttle->to);
            fprintf (throtfd, "%s \\\n", tbuf);
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            if (throttle->from > throttle->to) {
                time_t tt;
                tt = throttle->from;
                throttle->from = throttle->to;
                throttle->to = tt;
            }
            /* parse the measurement keyword */
            while (1) {
                get_word (cfg->cfgfd, ls);
                if (ls->word == "") {
                    break;
                }
                tok = ls->word;
                throttle->num_throttest++;
                if (throttle->num_throttest > 16) {
                    //reporterr (ERRFAC, M_THROT2TST, ERRWARN, cfg->cfgpath, ls->lineno);
                    state = 5;
                    forget_throttle (cfg , throttle);
                    drain_to_eol (cfg->cfgfd, ls);
                    if (0 == getline(cfg->cfgfd, ls)) {
                        return (0);
                    }
                    continue;
                }
                i = throttle->num_throttest - 1;
                ttest = &(throttle->throttest[i]);
                ttest->measure_tok = 0;
                ttest->relop_tok = 0;
                ttest->value = 0;
                ttest->valueflag = 0;
                ttest->valuestring[0] = '\0';
                ttest->logop_tok = LOGOP_DONE;
                /* parse measurement keyword */
                if (0 == strlen(tok) || 0 == parse_throtmeasure(tok, ttest)) {
                    //reporterr (ERRFAC, M_THROTMES, ERRWARN, cfg->cfgpath, ls->lineno);
                    state = 5;
                    forget_throttle (cfg , throttle);
                    drain_to_eol (cfg->cfgfd, ls);
                    if (0 == getline(cfg->cfgfd, ls)) {
                        return (0);
                    }
                    continue;
                }
#ifdef DEBUG_THROTTLE
                tbuf[0] = '\0';
                printmeasure (tbuf, ttest);
                fprintf (throtfd, "          %s ", tbuf);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                /* parse the relational operator */
                get_word (cfg->cfgfd, ls);
                tok = ls->word;
                if (0 == strlen(tok) || 0 == parse_throtrelop (tok, ttest)) {
                    //reporterr (ERRFAC, M_THROTREL, ERRWARN, cfg->cfgpath, ls->lineno);
                    state = 5;
                    forget_throttle (cfg , throttle);
                    drain_to_eol (cfg->cfgfd, ls);
                    if (0 == getline(cfg->cfgfd, ls)) {
                        return (0);
                    }
                    continue;
                } else if (0 == strcmp(">", tok)) {
                    ttest->relop_tok = GREATERTHAN;
                } else if (0 == strcmp(">=", tok)) {
                    ttest->relop_tok = GREATEREQUAL;
                } else if (0 == strcmp("<", tok)) {
                    ttest->relop_tok = LESSTHAN;
                } else if (0 == strcmp("<=", tok)) {
                    ttest->relop_tok = LESSEQUAL;
                } else if (0 == strcmp("==", tok)) {
                    ttest->relop_tok = EQUALTO;
                } else if (0 == strcmp("!=", tok)) {
                    ttest->relop_tok = NOTEQUAL;
                } else if (0 == strcmp("T>", tok)) {
                    ttest->relop_tok = TRAN2GT;
                } else if (0 == strcmp("T>=", tok)) {
                    ttest->relop_tok = TRAN2GE;
                } else if (0 == strcmp("T<", tok)) {
                    ttest->relop_tok = TRAN2LT;
                } else if (0 == strcmp("T<=", tok)) {
                    ttest->relop_tok = TRAN2LE;
                } else if (0 == strcmp("T==", tok)) {
                    ttest->relop_tok = TRAN2EQ;
                } else if (0 == strcmp("T!=", tok)) {
                    ttest->relop_tok = TRAN2NE;
                } else {
                    //reporterr (ERRFAC, M_THROTREL, ERRWARN, cfg->cfgpath, ls->lineno);
                    state = 5;
                    forget_throttle (cfg , throttle);
                    drain_to_eol (cfg->cfgfd, ls);
                    if (0 == getline(cfg->cfgfd, ls)) {
                        return (0);
                    }
                    continue;
                }   
#ifdef DEBUG_THROTTLE
                tbuf[0] = '\0';
                printrelop (tbuf, ttest);
                fprintf (throtfd, "%s ", tbuf);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                /* parse a positive integer value or string */
                get_word (cfg->cfgfd, ls);
                strcpy (ttest->valuestring, ls->word);
                if (0 == parse_value (ls)) {
                    //reporterr (ERRFAC, M_THROTSYN, ERRWARN, cfg->cfgpath, ls->lineno);
                    state = 5;
                    forget_throttle (cfg , throttle);
                    drain_to_eol (cfg->cfgfd, ls);
                    if (0 == getline(cfg->cfgfd, ls)) {
                        return (0);
                    }
                    continue;
                } else {
                    ttest->value = ls->value;
                    ttest->valueflag = ls->valueflag;
                }
#ifdef DEBUG_THROTTLE
                tbuf[0] = '\0';
                printvalue (tbuf, ttest);
                fprintf (throtfd, "%s ", tbuf);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                /* parse an optional logical operator */
                get_word (cfg->cfgfd, ls);
                tok = ls->word;
                ttest->logop_tok = LOGOP_DONE;
                if (0 == strlen(tok)) {
#ifdef DEBUG_THROTTLE
                    fprintf (throtfd, "\n");
                    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                    break;
                }
                if (0 == strcmp (tok, "AND")) {
                    ttest->logop_tok = LOGOP_AND;
                } else if (0 == strcmp (tok, "OR")) {
                    ttest->logop_tok = LOGOP_OR;
                } else {
                    //reporterr (ERRFAC, M_THROTLOGOP, ERRWARN, cfg->cfgpath, ls->lineno);
                    state = 5;
                    forget_throttle (cfg , throttle);
                    drain_to_eol (cfg->cfgfd, ls);
                    if (0 == getline(cfg->cfgfd, ls)) {
                        return (0);
                    }
                    continue;
                }
#ifdef DEBUG_THROTTLE
                tbuf[0] = '\0';
                printlogop (tbuf, ttest);
                fprintf (throtfd, "%s \\\n", tbuf);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            }
        } else if (0 == strncmp("ACTIONLIST", keyword, 10)) {
            /*=====     A C T I O N L I S T     =====*/
#ifdef DEBUG_THROTTLE
            fprintf (throtfd, "    ACTIONLIST\n");
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            if (state == 5) {
                if (0 == getline(cfg->cfgfd, ls)) return (0);
                continue;
            }
            if (state != 1) {
                //reporterr (ERRFAC, M_THROTSTA, ERRWARN, cfg->cfgpath, ls->lineno);
            }
            state = 2; /* state is have "ACTIONLIST", looking for "ACTION" */
        } else if (0 == strncmp("ACTION", keyword, 6)) {
            /*=====     A C T I O N     =====*/
#ifdef DEBUG_THROTTLE
            fprintf (throtfd, "        ACTION: ");
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            if (state == 5) {
                if (0 == getline(cfg->cfgfd, ls)) return (0);
                continue;
            }
            if (state != 1 && state != 2 && state != 3) {
                //reporterr (ERRFAC, M_THROTSTA, ERRWARN, cfg->cfgpath, ls->lineno);
                state = 5;
                forget_throttle (cfg , throttle);
                drain_to_eol (cfg->cfgfd, ls);
                if (0 == getline(cfg->cfgfd, ls)) {
                    return (0);
                }
                continue;
            }
            state = 3;
            if (throttle->num_actions >= 15) {
                //reporterr (ERRFAC, M_THROTACT, ERRWARN, cfg->cfgpath, ls->lineno);
            } else {
                action = &(throttle->actions[throttle->num_actions]);
                /* parse action verb */
                get_word (cfg->cfgfd, ls);
                tok = ls->word;
                implied_do_flag = 0;
                
                if (0 == strlen(tok)) {
                    //reporterr (ERRFAC, M_THROTSYN, ERRWARN, cfg->cfgpath, ls->lineno);
                    state = 5;
                    forget_throttle (cfg , throttle);
                    drain_to_eol (cfg->cfgfd, ls);
                    if (0 == getline(cfg->cfgfd, ls)) {
                        return (0);
                    }
                    continue;
                } else if (0 == strcmp ("set", tok)) {
                    action->actionverb_tok = VERB_SET;
                } else if (0 == strcmp ("incr", tok)) {
                    action->actionverb_tok = VERB_INCR;
                } else if (0 == strcmp ("decr", tok)) {
                    action->actionverb_tok = VERB_DECR;
                } else if (0 == strcmp ("do", tok)) {
                    action->actionverb_tok = VERB_DO;
                } else {
                    action->actionverb_tok = VERB_DO;
                    implied_do_flag = 1;
                }

                if (action->actionverb_tok != VERB_DO) {
                    /* parse "set", "incr", "decr" arguments */
                    get_word (cfg->cfgfd, ls);
                    tok = ls->word;
                    action->actionwhat_tok = parse_actiontunable (tok);
                    if (action->actionwhat_tok == 0) {
                        //reporterr (ERRFAC, M_THROTWHA, ERRWARN, cfg->cfgpath, ls->lineno);
                        state = 5;
                        forget_throttle (cfg , throttle);
                        drain_to_eol (cfg->cfgfd, ls);
                        if (0 == getline(cfg->cfgfd, ls)) {
                            return (0);
                        }
                        continue;
                    }
                    get_word (cfg->cfgfd, ls);
                    tok = ls->word;
                    if (1 == parse_value(ls)) {
                        if (ls->valueflag) {
                            action->actionvalue = ls->value;
                        }
                    } else {
                        //reporterr (ERRFAC, M_THROTVAL, ERRWARN, cfg->cfgpath, ls->lineno);
                        state = 5;
                        forget_throttle (cfg , throttle);
                        drain_to_eol (cfg->cfgfd, ls);
                        if (0 == getline(cfg->cfgfd, ls)) {
                            return (0);
                        }
                        continue;
                    }
                    action->actionstring[0] = '\0';
                    (void) strcat (action->actionstring, tok);
                    while (1) {
                        get_word (cfg->cfgfd, ls);
                        if (strlen(ls->word) == 0) {
                            break;
                        }
                    }
                } else {
                    /* parse action to take */
                    if (implied_do_flag == 0) {
                        get_word (cfg->cfgfd, ls);
                    }
                    tok = ls->word;
                    if (0 == strlen(tok)) {
                        //reporterr (ERRFAC, M_THROTSYN, ERRWARN, cfg->cfgpath, ls->lineno);
                        state = 5;
                        forget_throttle (cfg , throttle);
                        drain_to_eol (cfg->cfgfd, ls);
                        if (0 == getline(cfg->cfgfd, ls)) {
                            return (0);
                        }
                        continue;
                    } else if (0 == strcmp ("console", tok)) {
                        action->actionwhat_tok = ACTION_CONSOLE;
                        if (action->actionverb_tok != VERB_DO) {
                            //reporterr (ERRFAC, M_THROTWHA, ERRWARN, cfg->cfgpath, ls->lineno);
                            state = 5;
                            forget_throttle (cfg , throttle);
                            drain_to_eol (cfg->cfgfd, ls);
                            if (0 == getline(cfg->cfgfd, ls)) {
                                return (0);
                            }
                            continue;
                        }
                    } else if (0 == strcmp ("mail", tok)) {
                        action->actionwhat_tok = ACTION_MAIL;
                        if (action->actionverb_tok != VERB_DO) {
                            //reporterr (ERRFAC, M_THROTWHA, ERRWARN, cfg->cfgpath, ls->lineno);
                            state = 5;
                            forget_throttle (cfg , throttle);
                            drain_to_eol (cfg->cfgfd, ls);
                            if (0 == getline(cfg->cfgfd, ls)) {
                                return (0);
                            }
                            continue;
                        }
                    } else if (0 == strcmp ("exec", tok)) {
                        action->actionwhat_tok = ACTION_EXEC;
                        if (action->actionverb_tok != VERB_DO) {
                            //reporterr (ERRFAC, M_THROTWHA, ERRWARN, cfg->cfgpath, ls->lineno);
                            state = 5;
                            forget_throttle (cfg , throttle);
                            drain_to_eol (cfg->cfgfd, ls);
                            if (0 == getline(cfg->cfgfd, ls)) {
                                return (0);
                            }
                            continue;
                        }
                    } else if (0 == strcmp ("log", tok)) {
                        action->actionwhat_tok = ACTION_LOG;
                        if (action->actionverb_tok != VERB_DO) {
                            //reporterr (ERRFAC, M_THROTWHA, ERRWARN, cfg->cfgpath, ls->lineno);
                            state = 5;
                            forget_throttle (cfg , throttle);
                            drain_to_eol (cfg->cfgfd, ls);
                            if (0 == getline(cfg->cfgfd, ls)) {
                                return (0);
                            }
                            continue;
                        }
                    } else {
                        //reporterr (ERRFAC, M_THROTWER, ERRWARN, cfg->cfgpath, ls->lineno);
                        state = 5;
                        forget_throttle (cfg , throttle);
                        drain_to_eol (cfg->cfgfd, ls);
                        if (0 == getline(cfg->cfgfd, ls)) {
                            return (0);
                        }
                        continue;
                    }
                    /* parse value or action string */
                    strcpy (action->actionstring, " ");
                    get_word (cfg->cfgfd, ls);
                    tok = ls->word;
                    while (0 < strlen(tok)) {
                        if (1 == parse_value(ls)) {
                            if (ls->valueflag) {
                                action->actionvalue = ls->value;
                            }       
                        }
                        (void) strcat (action->actionstring, tok);
                        get_word (cfg->cfgfd, ls);
                        tok = ls->word;
                        if (0 < strlen(tok)) {
                            (void) strcat (action->actionstring, " ");
                        }
                    }
                }    
            }
#ifdef DEBUG_THROTTLE
            tbuf[0] = '\0';
            printaction (tbuf, action);
            fprintf (throtfd, "%s\n", tbuf);
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            throttle->num_actions++;
        } else if (0 == strncmp("ENDACTIONLIST", keyword, 13)) {
            /*=====     E N D A C T I O N L I S T     =====*/
#ifdef DEBUG_THROTTLE
            fprintf (throtfd, "    ENDACTIONLIST\n");
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            if (state == 5) {
                if (0 == getline(cfg->cfgfd, ls)) {
                    return (0);
                }
               continue;
            }
            if (state != 2 && state != 3) {
                //reporterr (ERRFAC, M_THROTSTA, ERRWARN, cfg->cfgpath, ls->lineno);
            } 
            state = 4;
        } else {
            /*=====     U N R E C O G N I Z E D    K E Y W O R D     =====*/
            /* -- unrecognized keyword, simply return the line 
             * and let config_read continue its processing
             */
            ls->invalid = FALSE;
#ifdef DEBUG_THROTTLE
            fclose (throtfd);
            throtfd = oldthrotfd;
#endif /* DEBUG_THROTTLE */
            return (ls->lineno);
        }
        /* -- read the next line from the config file */
        if (0 == getline(cfg->cfgfd, ls)) {
            return (0);
        }
    }
}    

unsigned long getid(char *szDir)
{
    HANDLE hVolume;
    unsigned long serial = -1;
    DWORD dwRes, dwBytesRead;
    DWORD dwSize = (sizeof(DWORD) * 2) + (128 * sizeof(PARTITION_INFORMATION));
    DRIVE_LAYOUT_INFORMATION *drive = (struct _DRIVE_LAYOUT_INFORMATION *)malloc(dwSize);
    
    if ( (hVolume = OpenAVolume(szDir, GENERIC_READ)) == INVALID_HANDLE_VALUE )
        return serial;

    dwRes = DeviceIoControl(  hVolume,  // handle to a device
        IOCTL_DISK_GET_DRIVE_LAYOUT, // dwIoControlCode operation to perform
        NULL,                        // lpInBuffer; must be NULL
        0,                           // nInBufferSize; must be zero
        drive,                      // pointer to output buffer
        dwSize,      // size of output buffer
        &dwBytesRead,               // receives number of bytes returned
        NULL                        // pointer to OVERLAPPED structure); 
    );


    CloseVolume(hVolume);

    if (dwRes)
        serial = drive->Signature;

    free(drive);

    return serial;
}

int verifyDrive(char *szCfgFile, char *szSystemValue)
{
    int iRc = 0;

    char    *strReadValue;
    FILE    *file;
    struct _stat buf;

    if ( (file  = fopen(szCfgFile, "r")) == NULL)
        return -1;

    _stat(szCfgFile, &buf);

    if ( (strReadValue = malloc(buf.st_size)) == NULL) {
        fclose(file);

        return -1;
    }

    fread( strReadValue, sizeof( char ), buf.st_size, file );
    if(!strstr(strReadValue, szSystemValue))
        iRc = -1;

    fclose(file);   
    free(strReadValue);
    
    return iRc;
}

/*
 * verify_primary_entries -- verify the primary system config info
 */
static int
verify_primary_entries(ftd_lg_cfg_t *cfgp)
{
    ftd_dev_cfg_t   **devpp;
    char            szDiskInfo[256];

    if (SizeOfLL(cfgp->devlist) == 0) {
        reporterr(ERRFAC, M_NODEVS, ERRCRIT, cfgp->cfgpath);
        return -1;
    }


    ForEachLLElement(cfgp->devlist, devpp) 
    {
        if (!getDeviceNameSymbolicLink((*devpp)->pdevname, (*devpp)->vdevname, MAXPATHLEN) ) 
        {
            reporterr(ERRFAC, M_MIRSTAT, ERRCRIT, (*devpp)->pdevname, ftd_strerror());
            return -1;
        }

    //
    // szDir format : \\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    //

        if ( strlen((*devpp)->pdevname) == 2 )
        {   /* Drive Letter  */
            getDiskSigAndInfo((*devpp)->pdevname, szDiskInfo, -1);
        }
        else
        {   /* Mount Point  */
            if (!getMntPtSigAndInfo((*devpp)->pdevname, szDiskInfo, -1))
            {
                return -1;
            }
        }

        if(verifyDrive(cfgp->cfgpath, szDiskInfo) < 0)
        {
            error_tracef(TRACEINF, "verify_primary_entries():Drives do not match <%.256s>",
                (*devpp)->sdevname );
            //reporterr(ERRFAC, M_DEVVERIFY, ERRCRIT, (*devpp)->sdevname);

            //return -1; // rddeb 020917
        }
    }

    return 0;
}

/*
 * verify_secondary_entries -- verify the secondary system config info
 */
static int
verify_secondary_entries(ftd_lg_cfg_t *cfgp)
{
    ftd_dev_cfg_t   **devpp;
    struct stat     statbuf;
    char            szDiskInfo[256];

    if (SizeOfLL(cfgp->devlist) == 0) {
        reporterr(ERRFAC, M_NODEVS, ERRCRIT, cfgp->cfgpath);
        return -1;
    }

    ForEachLLElement(cfgp->devlist, devpp) 
    {
        if (!getDeviceNameSymbolicLink((*devpp)->sdevname, (*devpp)->vdevname, MAXPATHLEN) ) 
        {
            reporterr(ERRFAC, M_MIRSTAT, ERRCRIT, (*devpp)->sdevname, ftd_strerror());
            //return -1;
            return FTD_CFG_MIR_STAT;
        }

        if ( strlen((*devpp)->sdevname) == 2 )
        {   /* Drive Letter  */
            getDiskSigAndInfo((*devpp)->sdevname, szDiskInfo, -1);
        }
        else
        {   /* Mount Point  */
            if (!getMntPtSigAndInfo((*devpp)->sdevname, szDiskInfo, -1))
            {
                return -1;
            }
        }

        if(verifyDrive(cfgp->cfgpath, szDiskInfo) < 0)
        {
            error_tracef( 
                TRACEINF, 
                "verify_secondary_entries():Drives do not match <%.256s>",
                (*devpp)->sdevname );
            //reporterr(ERRFAC, M_DEVVERIFY, ERRCRIT, (*devpp)->sdevname);\
            //return FTD_CFG_MIR_STAT; // rddeb 020917
        }
  }

    // verify journal area
    if (stat(cfgp->jrnpath, &statbuf) != 0) 
    {
        reporterr(ERRFAC, M_JRNPATH, ERRCRIT, cfgp->jrnpath, ftd_strerror());
        return -1;
    }
    return 0;
}

