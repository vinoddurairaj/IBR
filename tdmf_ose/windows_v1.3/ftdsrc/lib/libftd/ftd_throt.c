


#include "ftd_port.h"
#include "ftd_lg.h"
#include "ftd_error.h"
#include "ftdio.h"
#include "ftd_throt.h"

/* global variables */
int tracethrottle = TRACETHROTTLE;

int exec_action(ftd_lg_t *lgp, action_t *action);
static int ldom[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static meas_val_t measure_value[NUM_MEAS_TOKENS];
static int measure_value_inited = 0;

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

#ifdef WANT_MORE_THAN_PARSING_FUNCTIONS
/***************************************************************************
 * eval_dom -- evaluate whether we should proceed based on day of month
 **************************************************************************/
int 
eval_dom (throttle_t* throttle, int dom, int lastdayofmon, int day)
{
    if (throttle->end_of_month_flag == 1 && lastdayofmon == day) {
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "(eval_dom: about to return 1 for em \n");
#endif /* DEBUG_THROTTLE */        
        return (1);
    }
    if (throttle->day_of_month_mask == -1 ||
        (0 != (throttle->day_of_month_mask & (1 << (day-1))))) {
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "(eval_dom: about to return 1\n");
#endif /* DEBUG_THROTTLE */        
        return (1);
    }
#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "(eval_dom: about to return 0\n");
#endif /* DEBUG_THROTTLE */        
    return (0);
}

/***************************************************************************
 * eval_dow -- evaluate whether we should proceed based on day of week
 **************************************************************************/
int 
eval_dow (throttle_t* throttle, int dow)
{
    if (throttle->day_of_week_mask == -1 ||
        (0 != (throttle->day_of_week_mask & (1 << dow)))) {
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "(eval_dow: about to return 1\n");
#endif /* DEBUG_THROTTLE */        
        return (1);
    }
#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "(eval_dow: about to return 0\n");
#endif /* DEBUG_THROTTLE */        
    return (0);
}


/***************************************************************************
 * eval_throttles -- evaluate all throttles defined
 **************************************************************************/
void 
eval_throttles (ftd_lg_t *lgp)
{
    time_t tim;
    struct tm* timst1;
    struct tm timst2;
    throttle_t* throttle;
    action_t* action;
    int i;
    int prevtest;
    int thistest;
    int prevlogop;
	int lastdayofmon, day, wday;
	time_t midnight;
    throt_test_t* ttest;

    /* -- test to see if this is a moot point in proceeding */
	if (SizeOfLL(lgp->throttles) == 0) {
		return;
	}
	/* -- test for new day rollover */
	timst1 = localtime(&lgp->statp->statts);
    day = timst1->tm_mday;
    wday = timst1->tm_wday;
    lastdayofmon = ldom[timst1->tm_mon];
    
	if (timst1->tm_mon == 1 && ((1900+timst1->tm_year)%4) == 0) 
        lastdayofmon = 29;
    
	memcpy ((void*)&timst2, (void*)timst1, sizeof(timst2));
    timst2.tm_sec = 0;
    timst2.tm_min = 0;
    timst2.tm_hour = 0;
    tim = mktime (&timst2);
    
	//if (tim != midnight) {
		midnight = tim;
	//}
    
#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "\n=====================================\n");
    fprintf (throtfd, "  (in eval_throttles -- about to update throttle measure values)\n");
    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    /* -- update the throttle measure values to test against */
    update_throttle_measure_values (lgp);
    
#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "  (updated throttle measure values)\n");
    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    /* -- walk through the linked list of throttles and evaluate */
	ForEachLLElement(lgp->throttles, throttle) {
        /*===== day-of-week, day-of-month, and time period processing ===== */
        if (0 == eval_dow(throttle, wday) &&
            0 == eval_dom(throttle, day, lastdayofmon, day)) {
#ifdef DEBUG_THROTTLE
            fprintf (throtfd, "***** FAILED DOWDOM TEST *****\n");
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
			continue;
        }
		tim = lgp->statp->statts - midnight;
	if (throttle->from != (time_t) -1) {
	    if (tim < throttle->from) {
		throttle = throttle->n;
#ifdef DEBUG_THROTTLE
                fprintf (throtfd, "***** FAILED \"FROM\" TEST *****\n");
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
		continue;
	    }
	}
	if (throttle->to != (time_t) -1) {
	    if (tim > throttle->to) {
		throttle = throttle->n;
#ifdef DEBUG_THROTTLE
                fprintf (throtfd, "***** FAILED \"TO\" TEST *****\n");
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
		continue;
	    }
	}
        /*===== process the throttle tests ===== */
        prevtest = 0;
        prevlogop = LOGOP_OR;
        for (i=0; i<throttle->num_throttest; i++) {
            ttest = &(throttle->throttest[i]);
            /* -- test for early exit from evaluation */
            if (prevtest == 0 && prevlogop == LOGOP_AND) {
                prevtest = 0;
#ifdef DEBUG_THROTTLE
                fprintf (throtfd, "***** early exit of test 0 + AND *****\n");
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                break;
            }
            if (prevtest == 1 && prevlogop == LOGOP_OR) {
                prevtest = 1;
#ifdef DEBUG_THROTTLE
                fprintf (throtfd, "***** early exit of test 1 + OR *****\n");
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                break;
            }
            thistest = eval_throttest (ttest);
            if (prevlogop == LOGOP_AND) {
                if (prevtest == 1 && thistest == 1) {
                    prevtest = 1;
                } else {
                    prevtest = 0;
                }
#ifdef DEBUG_THROTTLE
                fprintf (throtfd, "    \"AND\" eval = %d\n", prevtest);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            } else if (prevlogop == LOGOP_OR) {
                if (prevtest == 0 && thistest == 0) {
                    prevtest = 0;
                } else {
                    prevtest = 1;
                }
#ifdef DEBUG_THROTTLE
                fprintf (throtfd, "    \"OR\" eval = %d\n", prevtest);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            }
            prevlogop = ttest->logop_tok;
        }
        /* -- process the actions if the tests were true */
        if (1 == prevtest) {
            for (i=0; i<throttle->num_actions; i++) {
                action = &throttle->actions[i];
                if (lgp->tunables->tracethrottle) 
                    printthrottletrace (lgp, throttle, action);
                (void) exec_action (lgp, action);
            }
        }
        throttle = throttle->n;
    }
    return;
}

/***************************************************************************
 * eval_throttest -- evaluate a specfic throttle test
 **************************************************************************/
int
eval_throttest (throt_test_t* ttest){
    int evalcode; /* 0=not equal, 1=equal, 2=less than, 3=greater than */
    int prevcode; /* bottom 4 bits=evalcode, bits 4-7=previous evalcode */

    /* -- dispatch to the specific measurement token evaluator -- */
    evalcode = eval_measure (ttest);
    prevcode = (evalcode >> 4) & 0x00000003;
    evalcode = evalcode & 0x00000003;
#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "    (eval_throttest - eval measure = prevcode=%d evalcode=%d)\n", prevcode, evalcode);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    /* -- perform relational operator evaluation and return */
    switch (ttest->relop_tok) {
    case GREATERTHAN:     return ((evalcode==3)?1:0);
    case GREATEREQUAL:    return ((evalcode==3||evalcode==1)?1:0);
    case LESSTHAN:        return ((evalcode==2)?1:0);
    case LESSEQUAL:       return ((evalcode==2||evalcode==1)?1:0);
    case EQUALTO:         return ((evalcode==1)?1:0);
    case NOTEQUAL:        return ((evalcode!=1)?1:0);
    case TRAN2GT:         return ((prevcode!=3&&evalcode==3)?1:0);
    case TRAN2GE:        
        return (((prevcode!=3&&prevcode!=1)&&(evalcode==3||evalcode==1))?1:0);
    case TRAN2LT:         return ((prevcode!=2&&evalcode==2)?1:0);
    case TRAN2LE:
        return (((prevcode!=2&&prevcode!=1)&&(evalcode==2||evalcode==1))?1:0);
    case TRAN2EQ:         return ((prevcode!=1&&evalcode==1)?1:0);
    case TRAN2NE:         return ((prevcode==1&&evalcode!=1)?1:0);
    }
    return (0);
}

/***************************************************************************
 * eval_measure -- evaluate meaurement token against a value
 *                 return 1=equal, 2=less than, 3=greater than 
 *                 current test = bottom 4 bits, previous test, bits 4-7
 **************************************************************************/
int
eval_measure(throt_test_t* ttest)
{
    int prevretcode;
    int retcode;
    int tt;
    int t;
    int retvalue;

    if (measure_value[ttest->measure_tok].val_type == 0 || 
        ttest->valueflag == 0) {
        /* -- string comparison -- */
        tt = strcmp (measure_value[ttest->measure_tok].prev_value_string, 
                     ttest->valuestring);
        if (tt == 0) prevretcode = 1;
        if (tt < 0) prevretcode = 2;
        if (tt > 0) prevretcode = 3;
        t = strcmp (measure_value[ttest->measure_tok].value_string, 
                    ttest->valuestring);
        if (t == 0) retcode = 1;
        if (t < 0) retcode = 2;
        if (t > 0) retcode = 3;
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    eval_measure - string test - measure_value=[%s] ttest->valuestring=[%s] prevretcode=%d retcode=%d\n",
                 measure_value[ttest->measure_tok].value_string, ttest->valuestring, prevretcode, retcode);
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    } else {
        /* -- numeric comparison -- */
        if (measure_value[ttest->measure_tok].prev_value == ttest->value) {
            prevretcode = 1;
        } else if (measure_value[ttest->measure_tok].prev_value < ttest->value) {
            prevretcode = 2;
        } else {
            prevretcode = 3;
        }
        if (measure_value[ttest->measure_tok].value == ttest->value) {
            retcode = 1;
        } else if (measure_value[ttest->measure_tok].value < ttest->value) {
            retcode = 2;
        } else {
            retcode = 3;
        }
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    eval_measure - numeric test - measure_value=[%d] ttest->valuestring=[%d] prevretcode=%d retcode=%d\n",
                 measure_value[ttest->measure_tok].value, ttest->value, prevretcode, retcode);
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    }
    retvalue = (int) ((retcode&0x00000003) | ((prevretcode&0x00000003)<<4));
    return (retvalue);
}

/***************************************************************************
 * update_throttle_measure_values -- fetch and set throttle measurement values
 **************************************************************************/
void 
update_throttle_measure_values (ftd_lg_t *lgp)
{
    int i;

    /* -- initialize the array to empty if this is the first time through */
    if (measure_value_inited == 0) {
        for (i=0; i<NUM_MEAS_TOKENS; i++) {
            measure_value[i].val_type = 1;
            measure_value[i].prev_value = 0;
            measure_value[i].value = 0;
            measure_value[i].value_string[0] = '\0';
            measure_value[i].prev_value_string[0] = '\0';
        }
    }
    /* -- update previous values */
    for (i=0; i<NUM_MEAS_TOKENS; i++) {
        measure_value[i].prev_value = measure_value[i].value;
        strcpy (measure_value[i].prev_value_string, 
                measure_value[i].value_string);
    }
    /* -- get current values -- */
    measure_value[TOK_NETKBPS].value = (int) (.5 + lgp->statp->effectkbps);
    measure_value[TOK_PCTCPU].value = lgp->statp->percentcpu;
    measure_value[TOK_PCTWL].value = lgp->statp->pctbab;
    measure_value[TOK_NETCONNECTFLAG].value = lgp->statp->pmdup;
    measure_value[TOK_PID].value = lgp->statp->pid;
    measure_value[TOK_CHUNKSIZE].value = lgp->tunables->chunksize;
    measure_value[TOK_STATINTERVAL].value = lgp->tunables->statinterval;
    measure_value[TOK_MAXSTATFILESIZE].value = lgp->tunables->maxstatfilesize;
    measure_value[TOK_STATTOFILEFLAG].value = lgp->tunables->stattofileflag;
    measure_value[TOK_TRACETHROTTLE].value = lgp->tunables->tracethrottle;
    measure_value[TOK_SYNCMODE].value = lgp->tunables->syncmode;
    measure_value[TOK_SYNCMODEDEPTH].value = lgp->tunables->syncmodedepth;
    measure_value[TOK_SYNCMODETIMEOUT].value = lgp->tunables->syncmodetimeout;
    measure_value[TOK_COMPRESSION].value = lgp->tunables->compression;
    measure_value[TOK_TCPWINDOW].value = lgp->tunables->tcpwindowsize;
    measure_value[TOK_NETMAXKBPS].value = lgp->tunables->netmaxkbps;
    measure_value[TOK_CHUNKDELAY].value = lgp->tunables->chunkdelay;
    measure_value[TOK_IODELAY].value = lgp->tunables->iodelay;
    measure_value[TOK_DRVMODE].value = lgp->statp->drvmode;
    measure_value[TOK_ACTUALKBPS].value = (int) (.5 + lgp->statp->actualkbps);
    measure_value[TOK_EFFECTKBPS].value = (int) (.5 + lgp->statp->effectkbps);
    measure_value[TOK_PCTDONE].value = (int) (.5 + lgp->statp->pctdone);
    measure_value[TOK_ENTRIES].value = lgp->statp->entries;
    measure_value[TOK_PCTBAB].value = lgp->statp->pctbab;
    measure_value[TOK_READKBPS].value = (int) (.5 + lgp->statp->local_kbps_read);
    measure_value[TOK_WRITEKBPS].value = (int) (.5 + lgp->statp->local_kbps_written);
#ifdef DEBUG_THROTTLE 
    fprintf (throtfd, "  (in update_throttle_measure_values)\n");
    fprintf (throtfd, "     NETKBPS=%d PCTCPU=%d PCTWL=%d NETCONNECTFLAG=%d PID=%d CHUNKSIZE=%d\n     STATINTERVAL=%d MAXSTATFILESIZE=%d STATTOFILEFLAG=%d TRACETHROTTLE=%d \n     SYNCMODE=%d SYNCMODEDEPTH=%d SYNCMODETIMEOUT=%d COMPRESSION=%d TCPWINDOW=%d\n     NETMAXKBPS=%d CHUNKDELAY=%d IODELAY=%d DRVMODE=%d ACTUALKBPS=%d EFFECTKBPS=%d PCTDONE=%d ENTRIES=%d PCTBAB=%d READKBPS=%d WRITEKBPS=%d\n",
             measure_value[TOK_NETKBPS].value,
             measure_value[TOK_PCTCPU].value,
             measure_value[TOK_PCTWL].value,
             measure_value[TOK_NETCONNECTFLAG].value,
             measure_value[TOK_PID].value,
             measure_value[TOK_CHUNKSIZE].value,
             measure_value[TOK_STATINTERVAL].value,
             measure_value[TOK_MAXSTATFILESIZE].value,
             measure_value[TOK_STATTOFILEFLAG].value,
             measure_value[TOK_TRACETHROTTLE].value,
             measure_value[TOK_SYNCMODE].value,
             measure_value[TOK_SYNCMODEDEPTH].value,
             measure_value[TOK_SYNCMODETIMEOUT].value,
             measure_value[TOK_COMPRESSION].value,
             measure_value[TOK_TCPWINDOW].value,
             measure_value[TOK_NETMAXKBPS].value,
             measure_value[TOK_CHUNKDELAY].value,
             measure_value[TOK_IODELAY].value,
             measure_value[TOK_DRVMODE].value,
             measure_value[TOK_ACTUALKBPS].value,
             measure_value[TOK_EFFECTKBPS].value,
             measure_value[TOK_PCTDONE].value,
             measure_value[TOK_ENTRIES].value,
             measure_value[TOK_PCTBAB].value,
             measure_value[TOK_READKBPS].value,
             measure_value[TOK_WRITEKBPS].value);
    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    /* -- initial previous value to current values if this is first time */
    if (measure_value_inited == 0) {
        for (i=0; i<NUM_MEAS_TOKENS; i++) {
            measure_value[i].prev_value = measure_value[i].value;
            strcpy (measure_value[i].prev_value_string, 
                    measure_value[i].value_string);
        }
        measure_value_inited = 1;
    }
}

/***************************************************************************
 * action_console_msg -- send a message to the console (via the daemon.err
 *                  syslog facility)
 **************************************************************************/
int 
action_console_msg (char* message)
{
    char line[256];

#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "  executing console message action, message=[%s]\n",
             message);
    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    sprintf (line, "%s", message);
    errfac_log_errmsg (ERRFAC, 100000, "DATASTAR_THROTTLE_NOTICE", line);
    return 0;
}

/***************************************************************************
 * action_mail -- send a e-mail message to someone
 **************************************************************************/
int 
action_mail (char* whoto, char* message)
{
#if defined(_WINDOWS)
	// can we do this on NT ???? There is no generic mailer is there ????
#else
    FILE* fd;
    char cmd[256];
    
#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "  executing mail action, whoto=[%s] message=[%s]\n",
             whoto, message);
    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    sprintf (cmd, "/bin/mail -s \"DATASTAR PMD THROTTLE NOTICE\" %s", whoto);
    if ((fd = popen (cmd, "w")) == NULL) {
	if (ftd_errno() == EMFILE)
	    reporterr(ERRFAC, M_FILE, ERRCRIT, "pipe", ftd_strerror());
    }
    (void) fprintf (fd, "%s", message);
    (void) fflush (fd);
    (void) fclose(fd); /* do not use pclose, do not want to wait for exit */
#endif    
	return 0;
}

/***************************************************************************
 * action_exec -- execute an arbitrary command line under the "sh" shell
 **************************************************************************/
int 
action_exec (char* sh_string)
{
#if defined (_WINDOWS)
	// probably want to execute a thread here
#else
    char* targv[4];
    int targc;
    
#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "  executing exec action, sh_string = [%s]\n",
             sh_string);
    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    targc = 3;
    targv[0] = "/bin/sh";
    targv[1] = "-c";
    targv[2] = sh_string;
    targv[targc] = (char*) NULL;
    
    if (0 == fork()) {
	(void) execv ("/bin/sh", targv);
	_exit(0);
    }
#endif

    return 0;
}

/***************************************************************************
 * action_log -- send a message to the syslog facility
 **************************************************************************/
int 
action_log (char* string)
{
#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "  executing log action, string=[%s]\n",
             string);
    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    reporterr (ERRFAC, M_THROTLOG, ERRWARN, string);
    return 0;
}

/***************************************************************************
 * action_update_tunable -- modify a pmd tunable and make it available
 *                              to the pmd
 **************************************************************************/
int
action_update_tunable (ftd_lg_t *lgp, int verbtok, int whattok, int value)
{
    int lgnum;
    char key[256];
    char valstr[256];
    throt_test_t ttest;
    int len;
    int val;

#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "  executing update tunable action - ");
    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    lgnum = lgp->lgnum;
    ttest.measure_tok = whattok;
    key[0] = '\0';
    printmeasure (key, &ttest);
    if (0 < (len = strlen(key))) {
        /* replace trailing blank with colon */
        key[len-1] = ':';
    }
#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "key=[%s] ", key);
    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
    val = measure_value[whattok].value;
    switch (verbtok) {
    case VERB_SET:
	val = value;
	break;
    case VERB_INCR:
	val += value;
	break;
    case VERB_DECR:
	val -= value;
	break;
    default:
	return -1;
    }
    /* -- validate/adjust value setting */
    switch (whattok) {
    case TOK_CHUNKSIZE:
        if (val < 32768) val = 32768;
        if (val > 10485760) val = 10485760;
        lgp->tunables->chunksize = val;
        break;
    case TOK_STATINTERVAL:
        if (val < 0) val = 0;
        if (val > 86400) val = 86400;
        lgp->tunables->statinterval = val;
        break;
    case TOK_MAXSTATFILESIZE:
        if (val < 0) val = 0;
        lgp->tunables->maxstatfilesize = (int) (val/1024);
        break;
    case TOK_STATTOFILEFLAG:
        if (val != 0) val = 1;
        lgp->tunables->stattofileflag = val;
        break;
    case TOK_TRACETHROTTLE:
        if (val != 0) val = 1;
        lgp->tunables->tracethrottle = val;
        break;
    case TOK_SYNCMODE:
        if (val != 0) val = 1;
        lgp->tunables->syncmode = val;
        break;
    case TOK_SYNCMODEDEPTH:
        if (val < 0) val = 0;
        lgp->tunables->syncmodedepth = val;
        break;
    case TOK_SYNCMODETIMEOUT:
        if (val < 0) val = 0;
        lgp->tunables->syncmodetimeout = val;
        break;
    case TOK_COMPRESSION:
        if (val != 0) val = 1;
        lgp->tunables->compression = val;
        break;
    case TOK_NETMAXKBPS:
        if (val < 1) val = 1;
        lgp->tunables->netmaxkbps = val;
        break;
    case TOK_CHUNKDELAY:
        if (val < 0) val = 0;
        lgp->tunables->chunkdelay = val;
        break;
    case TOK_IODELAY:
        if (val < 0) val = 0;
        if (val > 500000) val = 500000;
        lgp->tunables->iodelay = val;
        break;
    default:
        return (0);
    }
    sprintf (valstr, "%d", val);
#ifdef DEBUG_THROTTLE
    fprintf (throtfd, "value=[%d]\n", val);
    fprintf (throtfd, "\n>>>>> set tunable: lgnum=%d key=[%s] valuestring=[%s]\n\n",
             lgnum, key, valstr);
    fflush (throtfd);
#endif /* DEBUG_THROTTLE */

    fprintf (stderr, "\n>>>>> set tunable: lgnum=%d key=[%s] valuestring=[%s]\n\n",
             lgnum, key, valstr);

    (void) ftd_lg_set_state_value(lgp, key, valstr);

    return (0);
}

/***************************************************************************
 * exec_action -- execute an action from a throttle
 **************************************************************************/
int
exec_action (ftd_lg_t *lgp, action_t* action)
{
    char ostring[1024];
    int ostrlen;
    char* p;
    char to[256];
    
    ostrlen = 1024;
    (void) substitute_actionstring (lgp, action->actionstring, ostring, &ostrlen);
    switch (action->actionwhat_tok) {
    case ACTION_SLEEP:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_SLEEP - value = %d\n", action->actionvalue);
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       TOK_CHUNKDELAY, 
                                       action->actionvalue));
    case ACTION_IODELAY:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_IODELAY - value = %d\n", action->actionvalue);
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       TOK_IODELAY, 
                                       action->actionvalue));
    case ACTION_CONSOLE:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_CONSOLE_MSG - string = [%s]\n", ostring);
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
	return (action_console_msg (ostring));
    case ACTION_MAIL:
	p = strtok(ostring, " \t");
	strcpy (to, p);
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_MAIL - to=[%s] string = [%s]\n", to, 
                 ostring);
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
	return (action_mail (to, strtok((char*)NULL, "")));
    case ACTION_EXEC:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_EXEC - string = [%s]\n",  
                 ostring);
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
	return (action_exec (ostring));
    case ACTION_LOG:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_LOG - string = [%s]\n",  
                 ostring);
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
	return (action_log(ostring));
    case TOK_NETKBPS:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_NETKBPS - (readonly)\n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_PCTCPU:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_PCTCPU - (readonly)\n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_PCTWL:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_PCTWL - (readonly)\n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_NETCONNECTFLAG:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_NETCONNECTFLAG - (readonly)\n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_PID:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_PID - (readonly)\n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_CHUNKSIZE:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_CHUNKSIZE \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_STATINTERVAL:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_STATINTERVAL \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_MAXSTATFILESIZE:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_MAXSTATFILESIZE \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));

    case TOK_STATTOFILEFLAG:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_STATTOFILEFLAG \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_TRACETHROTTLE:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_TRACETHROTTLE \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_SYNCMODE:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_SYNCMODE \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_SYNCMODEDEPTH:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_SYNCMODEDEPTH \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_SYNCMODETIMEOUT:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_SYNCMODETIMEOUT \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_COMPRESSION:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_COMPRESSION \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_TCPWINDOW:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_TCPWINDOW (readonly) \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_NETMAXKBPS:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_NETMAXKBPS \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_CHUNKDELAY:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_CHUNKDELAY \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_IODELAY:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_IODELAY \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        return (action_update_tunable (lgp, action->actionverb_tok, 
                                       action->actionwhat_tok, 
                                       action->actionvalue));
    case TOK_DRVMODE:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_DRVMODE (readonly) \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_ACTUALKBPS:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_ACTUALKBPS (readonly) \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_EFFECTKBPS:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_EFFECTKBPS (readonly) \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_PCTDONE:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_PCTDONE (readonly) \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_ENTRIES:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_ENTRIES (readonly) \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_PCTBAB:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_PCTBAB (readonly) \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_READKBPS:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_READKBPS (readonly) \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    case TOK_WRITEKBPS:
#ifdef DEBUG_THROTTLE
        fprintf (throtfd, "    ACTION_WRITEKBPS (readonly) \n");
        fflush (throtfd);
#endif /* DEBUG_THROTTLE */
        /* read only - no action possible */
        return (0);
    }
    return -1;
}


/***************************************************************************
 * substitute_actionstring -- perform inline replacement of reserved word
 *                              variables for use in actions like "email"
 **************************************************************************/
int 
substitute_actionstring (ftd_lg_t *lgp, char* instr, char* outstr, int* outsize)
{
    char* p1;
    char* p2;
    char* pk;
    char* pt;
    int i;
    int len;
    char tempstr[256];
    time_t tim;
    int parsekw;
    char keyword[256];
    int kwlen;

    parsekw = 0;
    kwlen = 0;
    keyword[0] = '\0';
    *outsize = 0;
    outstr[0] = '\0';
    len = strlen (instr);
    p1 = instr;
    p2 = outstr;

    p1 = instr;
    while (*p1) {
        /* copy up to next %% */
        while (*p1) {
            if (0 != strncmp("%%", p1, 2)){
                *p2 = *p1;
                *outsize += 1;
                p1++;
                p2++;
                *p2 = '\0';
            } else {
                /* extract a substitute keyword */
                p1++;
                p1++; /* skip "%%" */
                pk = keyword;
                *pk = '\0';
                while (*p1) {
                    if (0 != strncmp("%%", p1, 2)) {
                        *pk = toupper(*p1);
                        p1++;
                        pk++;
                        *pk = '\0';
                    } else {
                        p1++;
                        p1++; /* skip "%%" */
                        tempstr[0] = '\0';
                        /* translate the keyword to a variable */
                        if (0 == strcmp(keyword, "PID")) {
                            sprintf(tempstr, "%d", lgp->statp->pid);
                        } else if (0 == strcmp(keyword, "GROUPNO")) {
                            i = strlen(lgp->cfgp->cfgpath) - 7;
                            sscanf ((lgp->cfgp->cfgpath+i), "%03d", &i);
                            sprintf(tempstr, "%d", i);
                        } else if (0 == strcmp(keyword, "CFGFILE")) {
                            strcpy (tempstr, lgp->cfgp->cfgpath);
                        } else if (0 == strcmp(keyword, "CPU")) {
                            sprintf (tempstr, "%d", lgp->statp->percentcpu);
                        } else if (0 == strcmp(keyword, "PCTCPU")) {
                            sprintf (tempstr, "%d", lgp->statp->percentcpu);
                        } else if (0 == strcmp(keyword, "KBPS")) {
                            sprintf (tempstr, "%d", 
                                     (int) lgp->statp->effectkbps);
                        } else if (0 == strcmp(keyword, "NETKBPS")) {
                            sprintf (tempstr, "%d", 
                                     (int) lgp->statp->effectkbps);
                        } else if (0 == strcmp(keyword, "PCTWL")) {
                            sprintf (tempstr, "%d", 
                                     lgp->statp->pctbab);
                        } else if (0 == strcmp(keyword, "SLEEP")) {
                            sprintf (tempstr, "%ld", 
                                     (long) lgp->tunables->chunkdelay);
                        } else if (0 == strcmp(keyword, "TIME")) {
                            tim = time((time_t*)NULL);
                            pt = ctime(&tim);
                            strncpy (tempstr, (pt+11), 8);
                            tempstr[8] = '\0';
                        } else if (0 == strcmp(keyword, "DATE")) {
                            tim = time((time_t*)NULL);
                            pt = ctime(&tim);
                            strncpy (tempstr, &(pt[4]), 6);
                            tempstr[6] = ',';
                            tempstr[7] = ' ';
                            strncpy (&(tempstr[8]), &(pt[20]), 4);
                            tempstr[12] = '\0';
                        } else if (0 == strcmp(keyword, "NETCONNECTFLAG")) {
                            sprintf (tempstr, "%d", 
                                     lgp->statp->pmdup);
                        } else if (0 == strcmp(keyword, "CHUNKSIZE")) {
                            sprintf (tempstr, "%d", lgp->tunables->chunksize);
                        } else if (0 == strcmp(keyword, "STATINTERVAL")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->statinterval);
                        } else if (0 == strcmp(keyword, "MAXSTATFILESIZE")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->maxstatfilesize);
                        } else if (0 == strcmp(keyword, "STATTOFILEFLAG")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->stattofileflag);
                        } else if (0 == strcmp(keyword, "LOGSTATS")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->stattofileflag);
                        } else if (0 == strcmp(keyword, "TRACETHROTTLE")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->tracethrottle);
                        } else if (0 == strcmp(keyword, "SYNCMODE")) {
                            sprintf (tempstr, "%d", lgp->tunables->syncmode);
                        } else if (0 == strcmp(keyword, "SYNCMODEDEPTH")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->syncmodedepth);
                        } else if (0 == strcmp(keyword, "SYNCMODETIMEOUT")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->syncmodetimeout);
                        } else if (0 == strcmp(keyword, "COMPRESSION")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->compression);
                        } else if (0 == strcmp(keyword, "TCPWINDOW")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->tcpwindowsize);
                        } else if (0 == strcmp(keyword, "TCPWINDOWSIZE")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->tcpwindowsize);
                        } else if (0 == strcmp(keyword, "NETMAXKBPS")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->netmaxkbps);
                        } else if (0 == strcmp(keyword, "CHUNKDELAY")) {
                            sprintf (tempstr, "%d", 
                                     lgp->tunables->chunkdelay);
                        } else if (0 == strcmp(keyword, "IODELAY")) {
                            sprintf (tempstr, "%d", lgp->tunables->iodelay);
                        } else if (0 == strcmp(keyword, "DRIVERMODE")) {
                            switch (lgp->statp->drvmode) {
                            case FTD_MODE_NORMAL:
                                strcpy (tempstr, "NORMAL");
                                break;
                            case FTD_MODE_TRACKING:
                                strcpy (tempstr, "TRACKING");
                                break;
                            case FTD_MODE_PASSTHRU:                    
                                strcpy (tempstr, "PASSTHRU");
                                break;
                            case FTD_MODE_REFRESH:                    
                                strcpy (tempstr, "REFRESH");
                                break;
                            case FTD_MODE_BACKFRESH:                    
                                strcpy (tempstr, "BACKFRESH");
                                break;
                            default:
                                sprintf (tempstr, "%d", 
                                         lgp->statp->drvmode);
                            }
                        } else if (0 == strcmp(keyword, "ACTUALKBPS")) {
                            sprintf (tempstr, "%d", 
                                     (int) lgp->statp->actualkbps);
                        } else if (0 == strcmp(keyword, "EFFECTKBPS")) {
                            sprintf (tempstr, "%d", 
                                     (int) lgp->statp->effectkbps);
                        } else if (0 == strcmp(keyword, "PERCENTDONE")) {
                            sprintf (tempstr, "%d", 
                                     (int) lgp->statp->pctdone);
                        } else if (0 == strcmp(keyword, "ENTRIES")) {
                            sprintf (tempstr, "%d", 
                                     lgp->statp->entries);
                        } else if (0 == strcmp(keyword, "PERCENTBABINUSE")) {
                            sprintf (tempstr, "%d", 
                                     lgp->statp->pctbab);
                        } else if (0 == strcmp(keyword, "READKBPS")) {
                            sprintf (tempstr, "%d", (int)
                                     lgp->statp->local_kbps_read);
                        } else if (0 == strcmp(keyword, "WRITEKBPS")) {
                            sprintf (tempstr, "%d", (int)
                                     lgp->statp->local_kbps_written);
                        } else {
                            sprintf (tempstr, "%%%%%s%%%%[?]", keyword);
                        }
                        if (strlen(tempstr) > 0) {
                            strcpy (p2, tempstr);
                            p2 += strlen(tempstr);
                        }
#ifdef DEBUG_THROTTLE
                        fprintf (throtfd, "   substitute actionstring %%%%%s%%%% with [%s]\n", keyword, tempstr);
#endif /* DEBUG_THROTTLE */
                        break;
                    }
                }
            }
        }
        *outsize = strlen (outstr);
    }
    return 0;
}

/***************************************************************************
 * printdowdom -- formats day-of-month mask for printing
 **************************************************************************/
void 
printdowdom (char* outstr, throttle_t* throttle) {
    int dom;
    int t;
    int i;
    int j;
    int dow;

    j = 0;
    dom = throttle->day_of_month_mask;
    if (dom != -1) {
        for (i=1; i<32; i++) {
            t = 0x01 << (i-1);
            if ((t&dom) != 0) {
                sprintf (&outstr[j], "%d,", i);
                j+=2;
                if (i>9) j+=1;
            }
         }
    }
    if (throttle->end_of_month_flag == 1) {
        sprintf (&outstr[j], "em,");
        j += 3;
    }
    dow = throttle->day_of_week_mask;
    if (dow != -1) {
        if ((dow & (0x01<<0)) != 0) {
            sprintf (&outstr[j], "su,");
            j+=3;
        }
        if ((dow & (0x01<<1)) != 0) {
            sprintf (&outstr[j], "mo,");
            j+=3;
        }
        if ((dow & (0x01<<2)) != 0) {
            sprintf (&outstr[j], "tu,");
            j+=3;
        }
        if ((dow & (0x01<<3)) != 0) {
            sprintf (&outstr[j], "we,");
            j+=3;
        }
        if ((dow & (0x01<<4)) != 0) {
            sprintf (&outstr[j], "th,");
            j+=3;
        }
        if ((dow & (0x01<<5)) != 0) {
            sprintf (&outstr[j], "fr,");
            j+=3;
        }
        if ((dow & (0x01<<6)) != 0) {
            sprintf (&outstr[j], "sa,");
            j+=3;
        }
    }
    if (j == 0) {
        strcpy (outstr, "- ");
    } else {
        outstr[j-1] = ' ';
        outstr[j] = '\0';
    }
}

/***************************************************************************
 * print_time -- formats a timestamp for printing
 **************************************************************************/
void
print_time (char* outstr, time_t tim)
{
    int h, m, s;
    if (tim == -1) {
        outstr[0] = '-';
        outstr[1] = ' ';
        outstr[2] = '\0';
    } else {
        h = tim / 3600;
        tim = tim % 3600;
        m = tim / 60;
        s = tim % 60;
        sprintf (outstr, "%02d:%02d:%02d ", h, m, s);
    }
}

/***************************************************************************
 * printmeasure -- formats a measurement token for printing
 **************************************************************************/
void 
printmeasure (char* outstr, throt_test_t* ttest)
{
    switch (ttest->measure_tok) {

    case TOK_UNKNOWN:
        strcpy (outstr, "UNKNOWN ");
        break;
    case TOK_NETKBPS:
        strcpy (outstr, "NETKBPS ");
        break;
    case TOK_PCTCPU:
        strcpy (outstr, "PCTCPU ");
        break;
    case TOK_PCTWL:       
        strcpy (outstr, "PCTWL ");
        break;
    case TOK_NETCONNECTFLAG:
        strcpy (outstr, "NETCONNECTFLAG ");
        break;
    case TOK_PID:
        strcpy (outstr, "PID ");
        break;
    case TOK_CHUNKSIZE:
        strcpy (outstr, "CHUNKSIZE ");
        break;
    case TOK_STATINTERVAL:
        strcpy (outstr, "STATINTERVAL ");
        break;
    case TOK_MAXSTATFILESIZE:
        strcpy (outstr, "MAXSTATFILESIZE ");
        break;
    case TOK_STATTOFILEFLAG:
        strcpy (outstr, "LOGSTATS ");
        break;
    case TOK_TRACETHROTTLE:
        strcpy (outstr, "TRACETHROTTLE ");
        break;
    case TOK_SYNCMODE:
        strcpy (outstr, "SYNCMODE ");
        break;
    case TOK_SYNCMODEDEPTH:
        strcpy (outstr, "SYNCMODEDEPTH ");
        break;
    case TOK_SYNCMODETIMEOUT:
        strcpy (outstr, "SYNCMODETIMEOUT ");
        break;
    case TOK_COMPRESSION:
        strcpy (outstr, "COMPRESSION ");
        break;
    case TOK_TCPWINDOW:
        strcpy (outstr, "TCPWINDOWSIZE ");
        break;
    case TOK_NETMAXKBPS:
        strcpy (outstr, "NETMAXKBPS ");
        break;
    case TOK_CHUNKDELAY:
        strcpy (outstr, "CHUNKDELAY ");
        break;
    case TOK_IODELAY:
        strcpy (outstr, "IODELAY ");
        break;
    case TOK_DRVMODE:
        strcpy (outstr, "DRIVERMODE ");
        break;
    case TOK_ACTUALKBPS:
        strcpy (outstr, "ACTUALKBPS ");
        break;
    case TOK_EFFECTKBPS:
        strcpy (outstr, "EFFECTKBPS ");
        break;
    case TOK_PCTDONE:
        strcpy (outstr, "PERCENTDONE ");
        break;
    case TOK_ENTRIES:
        strcpy (outstr, "ENTRIES ");
        break;
    case TOK_PCTBAB:
        strcpy (outstr, "PERCENTBABINUSE ");
        break;
    case TOK_READKBPS:
        strcpy (outstr, "READKBPS ");
        break;
    case TOK_WRITEKBPS:
        strcpy (outstr, "WRITEKBPS ");
        break;
    default:
        strcpy (outstr, "UNKNOWN ");
    }
}

/***************************************************************************
 * printrelop -- formats a relational operator token for printing
 **************************************************************************/
void
printrelop (char* outstr, throt_test_t* ttest)
{
    switch (ttest->relop_tok) {
    case GREATERTHAN:
        strcpy (outstr, "> ");
        break;
    case GREATEREQUAL:
        strcpy (outstr, ">= ");
        break;
    case LESSTHAN:
        strcpy (outstr, "< ");
        break;
    case LESSEQUAL:
        strcpy (outstr, "<= ");
        break;
    case EQUALTO:
        strcpy (outstr, "== ");
        break;
    case NOTEQUAL:
        strcpy (outstr, "!= ");
        break;
    case TRAN2GT:
        strcpy (outstr, "T> ");
        break;
    case TRAN2GE:
        strcpy (outstr, "T>= ");
        break;
    case TRAN2LT:
        strcpy (outstr, "T< ");
        break;
    case TRAN2LE:
        strcpy (outstr, "T<= ");
        break;
    case TRAN2EQ:
        strcpy (outstr, "T== ");
        break;
    case TRAN2NE:
        strcpy (outstr, "T!= ");
        break;
    default:
        strcpy (outstr, "<BAD RELOP TOKEN> ");
    }
}

/***************************************************************************
 * printlogop -- formats a logical operator token for printing
 **************************************************************************/
void
printlogop (char* outstr, throt_test_t* ttest)
{
    switch (ttest->logop_tok) {
    case LOGOP_DONE:
        break;
    case LOGOP_AND:
        strcpy (outstr, "AND ");
        break;
    case LOGOP_OR:
        strcpy (outstr, "OR ");
        break;
    default:
        strcpy (outstr, "<BAD LOGOP TOKEN> ");
    }
}

/***************************************************************************
 * printvalue -- formats a throttle test value printing
 **************************************************************************/
void
printvalue (char* outstr, throt_test_t* ttest)
{
    int i, j, len;
    j = 0;

    if (ttest->valueflag == 1) {
        sprintf (outstr, "%d ", ttest->value);
    } else {
        outstr[j++] = '\"';
        len = strlen (ttest->valuestring);
        for (i=0; i<len; i++) {
            if (ttest->valuestring[i] == '\"') {
                outstr[j++] = '\\';
            }
            outstr[j++] = ttest->valuestring[i];
        }
        outstr[j++] = '\"';
        outstr[j++] = ' ';
        outstr[j] = '\0';
    }
}

/***************************************************************************
 * printaction -- formats an action expression printing
 **************************************************************************/
void
printaction (ftd_lg_t *lgp, char* outstr, action_t* action)
{
    int j;
    int outlen;
    char outbuf[256];

    j = 0;
    /* -- get the verb -- */
    switch (action->actionverb_tok) {
    case VERB_SET:
        strcpy (&outstr[j], "set ");
        j += 4;
        break;
    case VERB_INCR:
        strcpy (&outstr[j], "incr ");
        j += 5;
        break;
    case VERB_DECR:
        strcpy (&outstr[j], "decr ");
        j += 5;
        break;
    case VERB_DO:
        strcpy (&outstr[j], "do ");
        j += 3;
        break;
    }
    /* -- get the "what" or subject of the verb */
    switch (action->actionwhat_tok) {
    case ACTION_SLEEP:
        strcpy (&outstr[j], "sleep ");
        j += 6;
        break;
    case ACTION_IODELAY:
        strcpy (&outstr[j], "writedelay ");
        j += 11;
        break;
    case ACTION_CONSOLE:
        strcpy (&outstr[j], "console ");
        j += 8;
        break;
    case ACTION_MAIL:
        strcpy (&outstr[j], "mail ");
        j += 5;
        break;
    case ACTION_EXEC:
        strcpy (&outstr[j], "exec ");
        j += 5;
        break;
    case TOK_NETKBPS:
        strcpy (&outstr[j], "NETKBPS ");
        j += 8;
        break;
    case TOK_PCTCPU:
        strcpy (&outstr[j], "PCTCPU ");
        j += 7;
        break;
    case TOK_PCTWL:       
        strcpy (&outstr[j], "PCTWL ");
        j += 6;
        break;
    case TOK_NETCONNECTFLAG:
        strcpy (&outstr[j], "NETCONNECTFLAG ");
        j += 15;
        break;
    case TOK_PID:
        strcpy (&outstr[j], "PID ");
        j += 4;
        break;
    case TOK_CHUNKSIZE:
        strcpy (&outstr[j], "CHUNKSIZE ");
        j += 10;
        break;
    case TOK_STATINTERVAL:
        strcpy (&outstr[j], "STATINTERVAL ");
        j += 13;
        break;
    case TOK_MAXSTATFILESIZE:
        strcpy (&outstr[j], "MAXSTATFILESIZE ");
        j += 16;
        break;
    case TOK_STATTOFILEFLAG:
        strcpy (&outstr[j], "LOGSTATS ");
        j += 9;
        break;
    case TOK_TRACETHROTTLE:
        strcpy (&outstr[j], "TRACETHROTTLE ");
        j += 14;
        break;
    case TOK_SYNCMODE:
        strcpy (&outstr[j], "SYNCMODE ");
        j += 9;
        break;
    case TOK_SYNCMODEDEPTH:
        strcpy (&outstr[j], "SYNCMODEDEPTH ");
        j += 14;
        break;
    case TOK_SYNCMODETIMEOUT:
        strcpy (&outstr[j], "SYNCMODETIMEOUT ");
        j += 16;
        break;
    case TOK_COMPRESSION:
        strcpy (&outstr[j], "COMPRESSION ");
        j += 12;
        break;
    case TOK_TCPWINDOW:
        strcpy (&outstr[0], "TCPWINDOWSIZE ");
        j += 14;
        break;
    case TOK_NETMAXKBPS:
        strcpy (&outstr[j], "NETMAXKBPS ");
        j += 11;
        break;
    case TOK_CHUNKDELAY:
        strcpy (&outstr[j], "CHUNKDELAY ");
        j += 11;
        break;
    case TOK_IODELAY:
        strcpy (&outstr[j], "IODELAY ");
        j += 8;
        break;
    case TOK_DRVMODE:
        strcpy (&outstr[j], "DRIVERMODE ");
        j += 11;
        break;
    case TOK_ACTUALKBPS:
        strcpy (&outstr[j], "ACTUALKBPS ");
        j += 11;
        break;
    case TOK_EFFECTKBPS:
        strcpy (&outstr[j], "EFFECTKBPS ");
        j += 14;
        break;
    case TOK_PCTDONE:
        strcpy (&outstr[j], "PERCENTDONE ");
        j += 12;
        break;
    case TOK_ENTRIES:
        strcpy (&outstr[j], "ENTRIES ");
        j += 8;
        break;
    case TOK_PCTBAB:
        strcpy (&outstr[j], "PERCENTBABINUSE ");
        j += 16;
        break;
    case TOK_READKBPS:
        strcpy (&outstr[j], "READKBPS ");
        j += 9;
        break;
    case TOK_WRITEKBPS:
        strcpy (&outstr[j], "WRITEKBPS ");
        j += 10;
        break;
    default:
        strcpy (&outstr[j], "<UNKNOWN_TOKEN> ");
        j += 16;
    }
    /* -- perform inline placeholder variable replacement */
    (void) substitute_actionstring (lgp, action->actionstring, outbuf, &outlen);
    strcpy (&outstr[j], outbuf);
    j += outlen;
    outstr[j] = '\0';
}

/***************************************************************************
 * printthrottletrace -- prints a throttle to the syslog when it is about to
 *                       be executed
 **************************************************************************/
void
printthrottletrace (ftd_lg_t *lgp, throttle_t* throttle, action_t* action)
{
    char otest[256];
    char oaction[256];
    
    int j, i;
    throt_test_t* ttest;

    j = 0;
    otest[0] = '\0';
    oaction[0] = '\0';

    /* -- build the throttle test into printable statement */
    for (i = 0; i < throttle->num_throttest; i++) {
        ttest = &(throttle->throttest[i]);
        j = strlen(otest);
        printmeasure (&otest[j], ttest);
        j = strlen(otest);
        printrelop (&otest[j], ttest);
        j = strlen(otest);
        printvalue (&otest[j], ttest);
        j = strlen(otest);
        printlogop (&otest[j], ttest);
    }
    /* -- build the action tokens into a printable action statement */
    printaction (lgp, oaction, action);
    
    reporterr (ERRFAC, M_THROTTRC, ERRWARN, lgp->lgnum, 
               otest, oaction);

}

#endif