/*****************************************************************************
 *                                                                           *
 *  This software  is the licensed software of Fujitsu Software              *
 *  Technology Corporation                                                   *
 *                                                                           *
 *  Copyright (c) 2004 by Softek Storage Technology Corporation              *
 *                                                                           *
 *  THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF            *
 *  FUJITSU SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED             *
 *  UNDER LICENSE FROM FUJITSU SOFTWARE TECHNOLOGY CORPORATION               *
 *                                                                           *
 *****************************************************************************

 *****************************************************************************
 *                                                                           *
 *  Revision History:                                                        *
 *                                                                           *
 *  Date        By              Change                                       *
 *  ----------- --------------  -------------------------------------------  *
 *  04-27-2004  Veerababu Arja   Initial version.                             *
 *                                                                           *
 *                                                                           *
 *****************************************************************************/

#include "sftk_config.h"

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

extern int parse_throttles ( ftd_lg_cfg_t* cfg, LineState* ls);
static int parse_system    ( ftd_lg_cfg_t* cfg, LineState* ls);
static int parse_profile   ( ftd_lg_cfg_t* cfg, BOOL bIsWindows, LineState* ls);
static int parse_notes     ( ftd_lg_cfg_t* cfg, LineState* ls);


// path should be of size char path[_MAX_PATH = 260];

char path[_MAX_PATH];
char*
ftd_nt_path(char* key)
{
    HKEY	happ;
	DWORD	dwType = 0;
    DWORD	dwSize = sizeof(path);

    // Read the current state from the registry
    // Try opening the registry key:
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SFTK_SOFTWARE_KEY,
                     0,
                     KEY_QUERY_VALUE,
                     &happ) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(happ,
                        key,
                        NULL,
                        &dwType,
                        (BYTE*)path,
                        &dwSize) != ERROR_SUCCESS) {
			return NULL;
		}
	}
	else {
		return NULL;
	}
	
	RegCloseKey(happ);

	return path;
}

/*
 * stringcompare_addr --
 * compare two strings given the address of their address for qsort
 */
int
stringcompare_addr (void* p1, void* p2) {
	char *pp1 = *(char**)p1;
	char *pp2 = *(char**)p2;

    return (strcmp(pp1, pp2));
}


LList *CreateLList (objsize)
	int objsize;
{
	LList *ptr = (LList *) malloc (sizeof (LList));

	InitLList (ptr, objsize);
	return (ptr);
}

void InitLList (list, objsize)
	LList *list;
	int objsize;
{
	list->tail = 0;
	list->head = 0;
	list->objsize = objsize;
}

void FreeLList (list)
	LList *list;
{
	UndoLList (list);
	free (list);
}

void UndoLList (list)
	LList *list;
{
	EmptyLList (list);
	memset(list, 0, sizeof(LList)) ;
}

void EmptyLList (list)
	LList *list;
{
	void *lep;
	void *next ;

	lep = HeadOfLL(list) ;
	while (lep) {
		next = NextLLElement(lep) ;
		FreeLListElement (lep);
		lep = next ;
	}
	list->head = 0;
	list->tail = 0;
}

void *IsInLList (list, obj, cmpfn)
	LList *list;
	void *obj;
	int (*cmpfn) ();
{
	void *lep;

	ForEachLLElement (list, lep) {
		
		if (! (*cmpfn) (obj, lep)) return (lep);
	}
	return (0);
}

int SizeOfLL(list)
	LList *list;
{
	void *lep;
    int cnt;

    cnt = 0;
	ForEachLLElement(list, lep) {
        cnt++;
	}

	return (cnt);
}


void *AllocateLListElement (size)
	int size;
{
	void **new = malloc (sizeof(void *) + size);

	return ((void *) (new+1));
}

void *NewHeadOfLL (list)
	LList *list;
{
	void *new = AllocateLListElement(list->objsize);

	if (IsLLEmpty (list)) {
		NextLListElement(new) = 0;
		list->head = new;
		list->tail = new;
	}
	else {
		NextLListElement(new) = list->head;
		list->head = new;
	}
	return (new);
}

void *NewTailOfLL (list)
	LList *list;
{	
	void *new = AllocateLListElement(list->objsize);

	NextLListElement(new) = 0;

	if (IsLLEmpty (list)) {
		list->head = new;
		list->tail = new;
	}
	else {
		NextLListElement(list->tail) = new;
		list->tail = new;
	}
	return (new);
}

/* .bp */

void *NewAppendOfLL (list, prev)
	LList *list;
	void *prev;
{
	void *new = AllocateLLElement (list->objsize);

	if (!prev) {
		/* do an add to head */

		NextLListElement(new) = list->head;
		list->head = new;
	}
	else {
		NextLListElement(new) = NextLListElement(prev);
		NextLListElement(prev) = new;
	}
	if (prev == list->tail) {
		/* 
		 * prev is the current tail 
		 * prev could be 0
		 */
		list->tail = new;
	}
	return (new);
}

void DelHeadOfLL (list)
	LList *list;
{
	void *head = list->head;

	list->head = NextLLElement (head);
	if (list->head == 0) {
		list->tail = 0;
	}
	FreeLLElement (head);
}

void RemHeadOfLL (list)
	LList *list;
{
	void *head = list->head;

	list->head = NextLLElement(head);
	if (list->head == 0) {
		list->tail = 0;
	}
}

int DelNextOfLL(list, curr)
    LList *list;
    void *curr;
{
    void *next;

    if (curr == (void*)NULL
    || (next = (void*)NextLLElement(curr)) == (void*)NULL) {
        /*
         * curr is 0 or
         * curr was the tail
         */
        return 0;
    }
    NextLLElement(curr) = NextLLElement(next);
    if (next == TailOfLL(list)) {
        TailOfLL(list) = curr;
    }
    FreeLLElement(next);

    return 1;
}

// Just remove the element from the list without
// freeing the memory associated with the element
int RemNextOfLL(list, curr)
    LList *list;
    void *curr;
{
    void *next;

    if (curr == (void*)NULL
    || (next = (void*)NextLLElement(curr)) == (void*)NULL) {
        /*
         * curr is 0 or
         * curr was the tail
         */
        return 0;
    }
    NextLLElement(curr) = NextLLElement(next);
    if (next == TailOfLL(list)) {
        TailOfLL(list) = curr;
    }

    return 1;
}

int DelCurrOfLL(list, curr)
    LList *list;
    void *curr;
{
    int i;
    void *prev, *p;

    if (curr == HeadOfLL(list)) {
        DelHeadOfLL(list);
        if (IsEmptyLL(list)) {
            return 0;
        }
    } else {
        i = 0;
        ForEachLLElement(list, p) {
           if (i == 0) {
               prev = p;
           } else {
               if (p == curr) {
                   DelNextOfLL(list, prev);
                   break;
               } else {
                   prev = p;
               }
           } 
           i++;
        }
    }

    return 1;
}

int RemCurrOfLL(list, curr)
    LList *list;
    void *curr;
{
    int i;
    void *prev, *p;

    if (curr == HeadOfLL(list)) {
        RemHeadOfLL(list);
        if (IsEmptyLL(list)) {
            return 0;
        }
    } else {
        i = 0;
        ForEachLLElement(list, p) {
           if (i == 0) {
               prev = p;
           } else {
               if (p == curr) {
                   RemNextOfLL(list, prev);
                   break;
               } else {
                   prev = p;
               }
           } 
           i++;
        }
    }

    return 1;
}

char **
GetNextElemAddrLL(void *elemp)
{
	/* back up 4 bytes to get next element address */
	return (char**)((char*)elemp - sizeof(void*));
}

void
GeneralLListSort (LList *list, int (*cmpfn)(void*, void*))
{
	void	*p, *r, *q, *newhead, *last, *newlast;
	char	**pp, **rp, **qp, *t;
	int		sorted;

	if (SizeOfLL(list) <= 1) {
		return;
	}
	/* create dummy head */
	newhead = NewHeadOfLL(list);

	last = newlast = NULL;
	sorted = 0;
	
	while (!sorted) {
		sorted = 1;
		
		r = HeadOfLL(list);
		rp = GetNextElemAddrLL(r);
		memcpy(&p, rp, sizeof(void*));
		pp = GetNextElemAddrLL(p);
		memcpy(&q, pp, sizeof(void*));
		qp = GetNextElemAddrLL(q);
		
		while (q != last) {
			if (((*cmpfn)(p, q)) > 0) {
				// swap elements
				memcpy(rp, &q, sizeof(void*));
				memcpy(pp, qp, sizeof(void*));
				memcpy(qp, &p, sizeof(void*));

				if (q == list->tail) {
					list->tail = p;
				}

				t = p;
				p = q;
				pp = GetNextElemAddrLL(p);
				q = t;
				qp = GetNextElemAddrLL(q);

				newlast = q;
				sorted = 0;
			}
			r = p;
			rp = GetNextElemAddrLL(r);
			p = q;
			pp = GetNextElemAddrLL(p);
			memcpy(&q, qp, sizeof(void*));
			qp = GetNextElemAddrLL(q);
		}
		last = newlast;
	}

	/* delete dummy head */
	DelHeadOfLL(list);

	return;
}


/***************************************************************************
 * parse_dowdom -- parses a days-of-week / days-of-month specification
 *
 **************************************************************************/
int
parse_dowdom (char* token, throttle_t* throt)
{
    int slen;
    int i;
    char c;
    char c1;
    int day;
    int wday;
    
    day = 0;
    wday = -1;
    c1 = '\0';
    c = '\0';
    slen = strlen(token);
    for (i=0; i<slen; i++) {
	if (token[i] == ',') {
	    /* test value */
	    if (day > 0) {
		if (day > 31) return (1);
                if (throt->day_of_month_mask == -1) 
                    throt->day_of_month_mask = 0;
		throt->day_of_month_mask |= 0x01 << (day-1);
		day = 0;
	    } else if (wday == -1 && throt->end_of_month_flag == 0) {
		return (0);
	    } else if (wday != -1) {
                if (throt->day_of_week_mask == -1) throt->day_of_week_mask = 0;
		throt->day_of_week_mask |= (0x01 << wday);
		wday = -1;
	    }
	    continue;
	}
	if (token[i] >= '0' && token[i] <= '9') {
	    day = (day * 10) + (token[i] - '0');
	} else {
	    c = tolower(token[i]);
	    if (c1 == '\0') {
		if (c != 'e' && c != 's' && c != 'm' && c != 't' && 
                    c != 'w' && c != 'f') return(1);
		c1 = c;
	    } else {
                wday = -1;
                if (c1 == 'e' && c == 'm') {
                    throt->end_of_month_flag = 1;
                }
		if (c1 == 's' && c == 'u') wday = 0;
		if (c1 == 'm' && c == 'o') wday = 1;
		if (c1 == 't' && c == 'u') wday = 2;
		if (c1 == 'w' && c == 'e') wday = 3;
		if (c1 == 't' && c == 'h') wday = 4;
		if (c1 == 'f' && c == 'r') wday = 5;
		if (c1 == 's' && c == 'a') wday = 6;
                c = '\0';
                c1 = '\0';
		if (wday == -1 && throt->end_of_month_flag == 0) return (0);
	    }
	}
    }
    /* test value */
    if (day > 0) {
	if (day > 31) return (1);
        if (throt->day_of_month_mask == -1) 
            throt->day_of_month_mask = 0;
	throt->day_of_month_mask |= 0x01 << (day-1);
    } else if (wday == -1 && throt->end_of_month_flag == 0) {
	return (0);
    } else if (wday != -1) {
        if (throt->day_of_week_mask == -1) throt->day_of_week_mask = 0;
	throt->day_of_week_mask |= 0x01 << wday;
    }
    return (1);
}

/***************************************************************************
 * parse_time -- parses a time value as HH:MM:SS into a time_t value
 *
 **************************************************************************/
int
parse_time (char* token, time_t* timval)
{
    int hh, mm, ss;
    
    if ((token == (char*) NULL) ||
	(strlen(token) != 8) ||
	(token[2] != ':') ||
	(token[5] != ':') ||
	(!(isdigit(token[0]))) ||
	(!(isdigit(token[1]))) ||
	(!(isdigit(token[3]))) ||
	(!(isdigit(token[4]))) ||
	(!(isdigit(token[6]))) ||
	(!(isdigit(token[7])))) return 1;
    hh = ((token[0] - '0') * 10) + (token[1] - '0');
    if (hh > 23) return 1;
    mm = ((token[3] - '0') * 10) + (token[4] - '0');
    if (mm > 59) return 1;
    ss = ((token[6] - '0') * 10) + (token[7] - '0');
    if (ss > 59) return 1;
    *timval = (time_t) (hh*3600) + (mm*60) + ss;
    return 0;
}

/***************************************************************************
 * parse_throtmeasure -- parses a throttle test measurement keyword and
 *                       sets the appropriate token into the structure if
 *                       valid
 **************************************************************************/
int 
parse_throtmeasure (char* token, throt_test_t* ttest)
{
    char word[256];
    int i;
    int len;

    len = strlen(token);
    for (i=0; i<len; i++) {
        word[i] = tolower(token[i]);
        word[i+1] = '\0';
    }
    if (0 == strcmp("netkbps", word)) {
        ttest->measure_tok = TOK_NETKBPS;
    } else if (0 == strcmp("pctcpu", word)) {
        ttest->measure_tok = TOK_PCTCPU;
    } else if (0 == strcmp("pctwl", word)) {
        ttest->measure_tok = TOK_PCTWL;
    } else if (0 == strcmp("netconnectflag", word)) {
        ttest->measure_tok = TOK_NETCONNECTFLAG;
    } else if (0 == strcmp("pid", word)) {
        ttest->measure_tok = TOK_PID;
    } else if (0 == strcmp("chunksize", word)) {
        ttest->measure_tok = TOK_CHUNKSIZE;
    } else if (0 == strcmp("statinterval", word)) {
        ttest->measure_tok = TOK_STATINTERVAL;
    } else if (0 == strcmp("maxstatfilesize", word)) {
        ttest->measure_tok = TOK_MAXSTATFILESIZE;
    } else if (0 == strcmp("stattofileflag", word)) {
        ttest->measure_tok = TOK_STATTOFILEFLAG;
    } else if (0 == strcmp("logstats", word)) {
        ttest->measure_tok = TOK_STATTOFILEFLAG;
    } else if (0 == strcmp("tracethrottle", word)) {
        ttest->measure_tok = TOK_TRACETHROTTLE;
    } else if (0 == strcmp("syncmode", word)) {
        ttest->measure_tok = TOK_SYNCMODE;
    } else if (0 == strcmp("syncmodedepth", word)) {
        ttest->measure_tok = TOK_SYNCMODEDEPTH;
    } else if (0 == strcmp("syncmodetimeout", word)) {
        ttest->measure_tok = TOK_SYNCMODETIMEOUT;
    } else if (0 == strcmp("compression", word)) {
        ttest->measure_tok = TOK_COMPRESSION;
    } else if (0 == strcmp("tcpwindowsize", word)) {
        ttest->measure_tok = TOK_TCPWINDOW;
    } else if (0 == strcmp("tcpwindow", word)) {
        ttest->measure_tok = TOK_TCPWINDOW;
    } else if (0 == strcmp("netmaxkbps", word)) {
        ttest->measure_tok = TOK_NETMAXKBPS;
    } else if (0 == strcmp("chunkdelay", word)) {
        ttest->measure_tok = TOK_CHUNKDELAY;
    } else if (0 == strcmp("iodelay", word)) {
        ttest->measure_tok = TOK_IODELAY;
    } else if (0 == strcmp("drivermode", word)) {
        ttest->measure_tok = TOK_DRVMODE;
    } else if (0 == strcmp("actualkbps", word)) {
        ttest->measure_tok = TOK_ACTUALKBPS;
    } else if (0 == strcmp("effectkbps", word)) {
        ttest->measure_tok = TOK_EFFECTKBPS;
    } else if (0 == strcmp("percentdone", word)) {
        ttest->measure_tok = TOK_PCTDONE;
    } else if (0 == strcmp("entries", word)) {
        ttest->measure_tok = TOK_ENTRIES;
    } else if (0 == strcmp("percentbabinuse", word)) {
        ttest->measure_tok = TOK_PCTBAB;
    } else if (0 == strcmp("pctbabinuse", word)) {
        ttest->measure_tok = TOK_PCTBAB;
    } else if (0 == strcmp("readkbps", word)) {
        ttest->measure_tok = TOK_READKBPS;
    } else if (0 == strcmp("writekbps", word)) {
        ttest->measure_tok = TOK_WRITEKBPS;
    } else {
        ttest->measure_tok = TOK_UNKNOWN;
        return (0);
    }
    return (1);
}

/***************************************************************************
 * parse_throtrelop -- parses a throttle relational operator and sets
 *                      the appropriate token into the structure if
 *                      valid
 **************************************************************************/
int 
parse_throtrelop (char* tok, throt_test_t* ttest)
{
    if (0 == strcmp(">", tok)) {
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
        return (0);
    }
    return (1);
}

/***************************************************************************
 * parse_actiontunable -- parse the "what" for set, incr, decr actions
 **************************************************************************/
int
parse_actiontunable (char* tok)
{
    char t[256];
    int i;
    int len;

    len = strlen (tok);
    t[0] = '\0';
    for (i=0; i<len; i++) {
        t[i] = tolower(tok[i]);
        t[i+1] = '\0';
    }
    if (0 == strcmp ("netmaxkbps", t)) return (TOK_NETMAXKBPS);
    if (0 == strcmp ("chunksize", t)) return (TOK_CHUNKSIZE);
    if (0 == strcmp ("statinterval", t)) return (TOK_STATINTERVAL);
    if (0 == strcmp ("maxstatfilesize", t)) return (TOK_MAXSTATFILESIZE);
    if (0 == strcmp ("stattofileflag", t)) return (TOK_STATTOFILEFLAG);
    if (0 == strcmp ("logstats", t)) return (TOK_STATTOFILEFLAG);
    if (0 == strcmp ("tracethrottle", t)) return (TOK_TRACETHROTTLE);
    if (0 == strcmp ("syncmode", t)) return (TOK_SYNCMODE);
    if (0 == strcmp ("syncmodedepth", t)) return (TOK_SYNCMODEDEPTH);
    if (0 == strcmp ("syncmodetimeout", t)) return (TOK_SYNCMODETIMEOUT);
    if (0 == strcmp ("compression", t)) return (TOK_COMPRESSION);
    if (0 == strcmp ("chunkdelay", t)) return (TOK_CHUNKDELAY);
    if (0 == strcmp ("sleep", t)) return (TOK_CHUNKDELAY);
    if (0 == strcmp ("writedelay", t)) return (TOK_IODELAY);
    if (0 == strcmp ("iodelay", t)) return (TOK_IODELAY);
    return (TOK_UNKNOWN);
    
}


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
    
    return config_get(cfgpath, 1, 0, cfglist);
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
//        reporterr(ERRFAC, M_CFGFILE, ERRCRIT,
//            cfgp->cfgpath, ftd_strerror());
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
//                reporterr(ERRFAC, M_CFGERR, ERRCRIT, cfgp->cfgpath, lstate.lineno); // ardeb 020912
                break;
            }
        } else if (strcmp("NOTES:", lstate.key) == 0) {
           continue;
        } else {
//            reporterr(ERRFAC, M_CFGBDKEY, ERRCRIT, cfgp->cfgpath, 
//                lstate.lineno, lstate.key);
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
//        reporterr(ERRFAC, M_CFGFILE, ERRCRIT,
//            cfgp->cfgpath, ftd_strerror());
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
//            error_tracef( TRACEINF, "ftd_config_get_psname():%s", cfgp->pstore );
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
    HANDLE          hFile; 
    WIN32_FIND_DATA data;
    BOOL            bFound = TRUE;
    ftd_lg_cfg_t    *cfgp = NULL;
    char            path[_MAX_PATH];

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
                
            cfgp->magicvalue = FTDCFGMAGIC;

            ftd_config_lg_add_to_list(cfglist, &cfgp);
        }        
        bFound = FindNextFile(hFile, &data); 
    }

errret:

    SortLL(cfglist, stringcompare_addr);

    FindClose(hFile);

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
//#if 0
	ls->p5 = ls->p6 = ls->p7 = NULL;
//#endif

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
//#if 0
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
//#endif // SAUMYA_FIX_CONFIG_FILE_PARSING

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
//        reporterr(ERRFAC, M_CFGERR, ERRCRIT, cfgp->cfgpath, ls->lineno);
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
//        reporterr(ERRFAC, M_BADHOSTNAM, ERRCRIT,
//            cfgp->cfgpath, ls->lineno, cfgp->phostname);
        return -1; 
    }
    /* we must have either the name or the IP address for the host */
    if (role == ROLESECONDARY && strlen(cfgp->shostname) == 0) {
//        reporterr(ERRFAC, M_BADHOSTNAM, ERRCRIT,
//            cfgp->cfgpath, ls->lineno, cfgp->shostname);
        return -1; 
    }
    /* we must have secondary journal path */
    if (role == ROLESECONDARY) {
        if (strlen(cfgp->jrnpath) == 0) {
//            reporterr(ERRFAC, M_JRNMISS, ERRCRIT, cfgp->cfgpath, ls->lineno);
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
//#if 0
				if (ls->p5 != NULL && ls->p6 != NULL && ls->p7 != NULL)
				{
					strncpy(devcfgp->symlink1, ls->p5, sizeof(devcfgp->symlink1));
					strncpy(devcfgp->symlink2, ls->p6, sizeof(devcfgp->symlink2));
					strncpy(devcfgp->symlink3, ls->p7, sizeof(devcfgp->symlink3));
				}
//#endif // SAUMYA_FIX_CONFIG_FILE_PARSING

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
//            reporterr(ERRFAC, M_CFGBDKEY, ERRCRIT, cfgp->cfgpath, ls->lineno, ls->key);
            ls->invalid = FALSE;
            return -1;
          }
        }
        
        /* We must have: sddevname, devname, mirname, and secondary journal */
        if (strlen(devcfgp->devname) == 0) {
//          reporterr(ERRFAC, M_DEVMISS, ERRCRIT, cfgp->cfgpath, ls->lineno);
          return -1;
        }
        
        if (strlen(devcfgp->sdevname) == 0) {
//          reporterr(ERRFAC, M_MIRMISS, ERRCRIT, cfgp->cfgpath, ls->lineno);
          return -1;
        }
        
        if (strlen(devcfgp->pdevname) == 0) {
//          reporterr(ERRFAC, M_DEVMISS, ERRCRIT, cfgp->cfgpath, ls->lineno);
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
