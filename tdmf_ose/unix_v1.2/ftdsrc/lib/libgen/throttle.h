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
/***************************************************************************
 * throttle.h - DataStar throttles header - this defines the data structures
 *              constants, and functions used in parsing and processing
 *              throttles (in the PMD)
 *
 * (c) Copyright 1997, 1998 FullTime Software, Inc. All Rights Reserved
 *
 *
 * History:
 *   1/22/97 - Steve Wahl - original code
 *
 ***************************************************************************
 */

#ifndef _THROTTLE_H
#define _THROTTLE_H 1

#include <time.h>

#ifndef SDDISK_P
#define SDDISK_P 1
typedef struct sddisk *sddisk_p;
#endif

/* == TOKENS == */
#define TOKEN_UNKNOWN    0
#define TOKEN_MEASURE    1
#define TOKEN_RELOP      2
#define TOKEN_VALUE      3
#define TOKEN_VERB       4
#define TOKEN_ACTION     5
#define TOKEN_STRING     6

/* == MEASUREMENT TOKENS == */
#define TOK_UNKNOWN         0
#define TOK_NETKBPS         1
#define TOK_PCTCPU          2
#define TOK_PCTWL           3
#define TOK_NETCONNECTFLAG  4
#define TOK_PID             5
/* -- tunable parameters -- */ 
#define TOK_CHUNKSIZE       11
#define TOK_STATINTERVAL    12
#define TOK_MAXSTATFILESIZE 13
#define TOK_STATTOFILEFLAG  14
#define TOK_TRACETHROTTLE   15
#define TOK_SYNCMODE        16
#define TOK_SYNCMODEDEPTH   17
#define TOK_SYNCMODETIMEOUT 18
#define TOK_COMPRESSION     19
#define TOK_TCPWINDOW       20
#define TOK_NETMAXKBPS      21
#define TOK_CHUNKDELAY      22
#define TOK_IODELAY         23
/* -- performance metrics -- */
#define TOK_DRVMODE         30
#define TOK_ACTUALKBPS      31
#define TOK_EFFECTKBPS      32
#define TOK_PCTDONE         33
#define TOK_ENTRIES         34
#define TOK_PCTBAB          35
#define TOK_READKBPS        36
#define TOK_WRITEKBPS       37

#define NUM_MEAS_TOKENS     38

/* -- RELATIONAL OPERATOR TOKENS -- */
#define GREATERTHAN      1
#define GREATEREQUAL     2
#define LESSTHAN         3
#define LESSEQUAL        4
#define EQUALTO          5
#define NOTEQUAL         6
#define TRAN2GT          7
#define TRAN2GE          8
#define TRAN2LT          9
#define TRAN2LE         10
#define TRAN2EQ         11
#define TRAN2NE         12

/* -- LOGICAL OPERATOR TOKENS -- */
#define LOGOP_DONE       0
#define LOGOP_AND        1
#define LOGOP_OR         2

/* -- ACTION VERB TOKENS -- */
#define VERB_SET         1
#define VERB_INCR        2
#define VERB_DECR        3
#define VERB_DO          4

/* -- ACTION ITEM TOKENS -- */
#define ACTION_SLEEP     101
#define ACTION_IODELAY   102
#define ACTION_ADDWL     103
#define ACTION_REMWL     104
#define ACTION_CONSOLE   105
#define ACTION_MAIL      106
#define ACTION_EXEC      107
#define ACTION_LOG       108

typedef struct action_s {
    int    actionverb_tok;       
    int    actionwhat_tok;
    int    actionvalue;
    char   actionstring[256];
} action_t;

typedef struct throt_test_s {
    int measure_tok;
    int relop_tok;
    int value;
    int valueflag;
    char valuestring[256];
    int logop_tok;
} throt_test_t;

typedef struct throttle_s {
    struct throttle_s* n;
    int    day_of_week_mask;
    int    day_of_month_mask;
    int    end_of_month_flag;
    time_t from;
    time_t to;
    int    num_throttest;
    throt_test_t throttest[16];
    int    num_actions;
    action_t actions[16];
} throttle_t;

typedef struct meas_val_s {
    int val_type;
    int prev_value;
    int value;
    char prev_value_string[256];
    char value_string[256];
} meas_val_t;

/* -- function prototypes for parsing throttles from config files */
/* extern int parse_throttles (FILE* fd, char* line, int* lineno); */
extern int parse_dowdom (char* token, throttle_t* throt);
extern int parse_time (char* buf, time_t* tim);
extern int parse_throtmeasure (char* token, throt_test_t* ttest);
extern int parse_throtrelop (char* token, throt_test_t* ttest);
extern int parse_actiontunable (char* token);

/* -- function prototypes for executing actions */
extern int action_console_msg (char* message);
extern int action_mail (char* whoto, char* message);
extern int action_exec (char* sh_string);
extern int action_log (char* string);
extern int action_update_tunable (int verb, int what, int value);

/* -- function prototype(s) for evaluating throttles */
extern void eval_throttles (void);
extern int eval_throttest (throt_test_t* ttest);

/* -- functions for evaluating specific measurements against values */
extern int eval_measure(throt_test_t* ttest);

/* -- manage measurement value -- */
extern void update_throttle_measure_values (void);

/* -- substitution utilities -- */
extern int substitute_actionstring (char* instr, char* outstr, 
                                      int* outsize);
/* -- utility */
extern void printdowdom (char* outstr, throttle_t* throttle);
extern void print_time (char* outstr, time_t tim);
extern void printmeasure (char* outstr, throt_test_t* ttest);
extern void printrelop (char* outstr, throt_test_t* ttest);
extern void printlogop (char* outstr, throt_test_t* ttest);
extern void printvalue (char* outstr, throt_test_t* ttest);
extern void printaction (char* outstr, action_t* action);
extern void printthrottletrace (throttle_t* throttle, action_t* action);

/* -- global variables */
extern int tracethrottle;
#endif /* _THROTTLE_H */






