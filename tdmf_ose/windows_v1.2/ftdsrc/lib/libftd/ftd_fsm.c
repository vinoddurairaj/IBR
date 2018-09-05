/*
 * ftd_fsm.c - FTD logical group finite state machine  
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

#include "ftd_fsm.h"
#include "ftd_lg_states.h"
#include "ftd_lg.h"
#include "ftd_sock.h"
#include "ftd_ioctl.h"
#include "ftd_error.h"
#include "ftd_rsync.h"
#include "ftd_journal.h"
#include "ftd_stat.h"
#include "ftd_cpoint.h"
#include "ftd_ps.h"

#if defined(_WINDOWS)
#include "DbgDmp.h"
#include "ftd_devlock.h"
#endif

extern int bDbgLogON ;

static int ftd_fsm_primary(ftd_lg_t *lgp, u_char fsm[][FTD_NCHRS],
	ftd_fsm_trans_t ttab[], u_char curstate, u_char curchar);
static int ftd_fsm_secondary(ftd_lg_t *lgp, u_char fsm[][FTD_NCHRS],
	ftd_fsm_trans_t ttab[], u_char curstate, u_char curchar);
static int ftd_fsm_do_normal(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_do_normal_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_normal_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_refresh_check(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_do_refresh(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_do_refresh_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_refresh_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_refresh_full(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_do_refresh_full_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_refresh_full_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_backfresh(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_do_backfresh_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_backfresh_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_refoflow(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_do_refoflow_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_refoflow_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_tracking(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_tracking_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_tracking_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_abort(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_noop(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_do_cpstart(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_cpstop(ftd_lg_t *lgp, ftd_fsm_trans_t *trans); 
static int ftd_fsm_do_cppend(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_do_cpon(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_do_cpoff(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static int ftd_fsm_do_report_bad_state(ftd_lg_t *lgp, ftd_fsm_trans_t *trans);
static void ftd_fsm_report_run_state(int state);

static int ftd_fsm_tracking_primary(ftd_lg_t *lgp);
static int ftd_fsm_refoflow_primary(ftd_lg_t *lgp);

ftd_fsm_trans_t ftd_ttstab[] = {
	/* State			Input			Next State		Action  */
    /* -----			-----			----------		------  */
	{ FTD_SNORMAL,		FTD_CINVALID,	FTD_SINVALID,	ftd_fsm_do_abort },
	{ FTD_SNORMAL,		FTD_CPASSTHRU,	FTD_SINVALID,	ftd_fsm_do_report_bad_state },
	{ FTD_SNORMAL,		FTD_CNORMAL,	FTD_SNORMAL,	ftd_fsm_do_report_bad_state },
	{ FTD_SNORMAL,		FTD_CREFRESH,	FTD_STRACKING,	ftd_fsm_do_tracking },
	{ FTD_SNORMAL,		FTD_CREFRESHF,	FTD_STRACKING,	ftd_fsm_do_tracking },
	{ FTD_SNORMAL,		FTD_CREFRESHC,	FTD_STRACKING,	ftd_fsm_do_tracking },
	{ FTD_SNORMAL,		FTD_CBACKFRESH,	FTD_SBACKFRESH,	ftd_fsm_do_backfresh },
	{ FTD_SNORMAL,		FTD_CTRACKING,	FTD_STRACKING,	ftd_fsm_do_tracking },
	{ FTD_SNORMAL,		FTD_CCPSTART,	FTD_SNORMAL,	ftd_fsm_do_cpstart },
	{ FTD_SNORMAL,		FTD_CCPSTOP,	FTD_SNORMAL,	ftd_fsm_do_cpstop },
	{ FTD_SNORMAL,		FTD_CCPON,		FTD_SNORMAL,	ftd_fsm_do_cpon },
	{ FTD_SNORMAL,		FTD_CCPPEND,	FTD_SNORMAL,	ftd_fsm_do_cppend },
	{ FTD_SNORMAL,		FTD_CCPOFF,		FTD_SNORMAL,	ftd_fsm_do_cpoff },
	{ FTD_SNORMAL,		FTD_CREFTIMEO,	FTD_SREFOFLOW,	ftd_fsm_do_refoflow },
	
	{ FTD_SREFRESH,		FTD_CINVALID,	FTD_SINVALID,	ftd_fsm_do_abort },
	{ FTD_SREFRESH,		FTD_CPASSTHRU,	FTD_SINVALID,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESH,		FTD_CREFRESH,	FTD_SREFRESH,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESH,		FTD_CREFRESHF,	FTD_SREFRESH,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESH,		FTD_CREFRESHC,	FTD_SREFRESH,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESH,		FTD_CBACKFRESH,	FTD_SREFRESH,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESH,		FTD_CTRACKING,	FTD_SREFOFLOW,	ftd_fsm_do_refoflow },
	{ FTD_SREFRESH,		FTD_CNORMAL,	FTD_SNORMAL,	ftd_fsm_do_normal },
	{ FTD_SREFRESH,		FTD_CCPSTART,	FTD_SREFRESH,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESH,		FTD_CCPSTOP,	FTD_SREFRESH,	ftd_fsm_do_report_bad_state },
	
	{ FTD_SREFRESHF,	FTD_CINVALID,	FTD_SINVALID,	ftd_fsm_do_abort },
	{ FTD_SREFRESHF,	FTD_CPASSTHRU,	FTD_SINVALID,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHF,	FTD_CREFRESH,	FTD_SREFRESHF,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHF,	FTD_CREFRESHF,	FTD_SREFRESHF,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHF,	FTD_CREFRESHC,	FTD_SREFRESHF,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHF,	FTD_CBACKFRESH,	FTD_SREFRESHF,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHF,	FTD_CTRACKING,	FTD_SREFOFLOW,	ftd_fsm_do_refoflow },
	{ FTD_SREFRESHF,	FTD_CNORMAL,	FTD_SNORMAL,	ftd_fsm_do_normal },
	{ FTD_SREFRESHF,	FTD_CCPSTART,	FTD_SREFRESHF,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHF,	FTD_CCPSTOP,	FTD_SREFRESHF,	ftd_fsm_do_report_bad_state },

	{ FTD_SREFRESHC,	FTD_CINVALID,	FTD_SINVALID,	ftd_fsm_do_abort },
	{ FTD_SREFRESHC,	FTD_CPASSTHRU,	FTD_SINVALID,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHC,	FTD_CREFRESH,	FTD_SREFRESHC,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHC,	FTD_CREFRESHF,	FTD_SREFRESHC,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHC,	FTD_CREFRESHC,	FTD_SREFRESHC,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHC,	FTD_CBACKFRESH,	FTD_SREFRESHC,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHC,	FTD_CTRACKING,	FTD_SREFOFLOW,	ftd_fsm_do_refoflow },
	{ FTD_SREFRESHC,	FTD_CNORMAL,	FTD_SNORMAL,	ftd_fsm_do_normal },
	{ FTD_SREFRESHC,	FTD_CCPSTART,	FTD_SREFRESHC,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFRESHC,	FTD_CCPSTOP,	FTD_SREFRESHC,	ftd_fsm_do_report_bad_state },

	{ FTD_SBACKFRESH,	FTD_CINVALID,	FTD_SINVALID,	ftd_fsm_do_abort },
	{ FTD_SBACKFRESH,	FTD_CPASSTHRU,	FTD_SINVALID,	ftd_fsm_do_report_bad_state },
	{ FTD_SBACKFRESH,	FTD_CTRACKING,	FTD_SINVALID,	ftd_fsm_do_report_bad_state	},
	{ FTD_SBACKFRESH,	FTD_CREFRESH,	FTD_SBACKFRESH,	ftd_fsm_do_report_bad_state },
	{ FTD_SBACKFRESH,	FTD_CREFRESHF,	FTD_SBACKFRESH,	ftd_fsm_do_report_bad_state },
	{ FTD_SBACKFRESH,	FTD_CREFRESHC,	FTD_SBACKFRESH,	ftd_fsm_do_report_bad_state },
	{ FTD_SBACKFRESH,	FTD_CBACKFRESH,	FTD_SBACKFRESH,	ftd_fsm_do_report_bad_state },
	{ FTD_SBACKFRESH,	FTD_CNORMAL,	FTD_SNORMAL,	ftd_fsm_do_normal },
	{ FTD_SBACKFRESH,	FTD_CCPSTART,	FTD_SBACKFRESH,	ftd_fsm_do_report_bad_state },
	{ FTD_SBACKFRESH,	FTD_CCPSTOP,	FTD_SBACKFRESH,	ftd_fsm_do_report_bad_state },

	{ FTD_SREFOFLOW,	FTD_CINVALID,	FTD_SINVALID,	ftd_fsm_do_abort },
	{ FTD_SREFOFLOW,	FTD_CPASSTHRU,	FTD_SINVALID,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFOFLOW,	FTD_CREFRESH,	FTD_SREFRESH,	ftd_fsm_do_refresh },
	{ FTD_SREFOFLOW,	FTD_CREFRESHF,	FTD_SREFRESHF,	ftd_fsm_do_refresh_full },
	{ FTD_SREFOFLOW,	FTD_CREFRESHC,	FTD_SREFRESHC,	ftd_fsm_do_refresh_check },
	{ FTD_SREFOFLOW,	FTD_CNORMAL,	FTD_SNORMAL,	ftd_fsm_do_normal },
	{ FTD_SREFOFLOW,	FTD_CBACKFRESH,	FTD_SBACKFRESH,	ftd_fsm_do_backfresh },
	{ FTD_SREFOFLOW,	FTD_CTRACKING,	FTD_SREFOFLOW,	ftd_fsm_noop },
	{ FTD_SREFOFLOW,	FTD_CCPSTART,	FTD_SREFOFLOW,	ftd_fsm_do_report_bad_state },
	{ FTD_SREFOFLOW,	FTD_CCPSTOP,	FTD_SREFOFLOW,	ftd_fsm_do_report_bad_state },
	
	{ FTD_STRACKING,	FTD_CINVALID,	FTD_SINVALID,	ftd_fsm_do_abort },
	{ FTD_STRACKING,	FTD_CPASSTHRU,	FTD_SINVALID,	ftd_fsm_do_report_bad_state },
	{ FTD_STRACKING,	FTD_CREFRESH,	FTD_SREFRESH,	ftd_fsm_do_refresh },
	{ FTD_STRACKING,	FTD_CREFRESHF,	FTD_SREFRESHF,	ftd_fsm_do_refresh_full },
	{ FTD_STRACKING,	FTD_CREFRESHC,	FTD_SREFRESHC,	ftd_fsm_do_refresh_check },
	{ FTD_STRACKING,	FTD_CNORMAL,	FTD_SNORMAL,	ftd_fsm_do_normal },
	{ FTD_STRACKING,	FTD_CBACKFRESH,	FTD_SBACKFRESH,	ftd_fsm_do_backfresh },
	{ FTD_STRACKING,	FTD_CTRACKING,	FTD_STRACKING,	ftd_fsm_noop },
	{ FTD_STRACKING,	FTD_CCPSTART,	FTD_STRACKING,	ftd_fsm_do_report_bad_state },
	{ FTD_STRACKING,	FTD_CCPSTOP,	FTD_STRACKING,	ftd_fsm_do_report_bad_state },

	{ FTD_SAPPLY,		FTD_CANY,		FTD_SINVALID,	ftd_fsm_noop },

	{ FTD_SINVALID,		FTD_CANY,		FTD_SINVALID,	ftd_fsm_do_abort },
};

u_char	ftd_ttfsm[FTD_NSTATES][FTD_NCHRS];

#ifdef TDMF_TRACE
	typedef struct ftd_fsm_trans_TXT_s {
		u_char	ft_state_TXT[20];					/* current state				*/
		u_char 	ft_char_TXT[20];					/* input character				*/
		u_char	ft_next_TXT[20];					/* next state					*/
		u_char	ft_action_TXT[30];				/* action to take				*/
	} ftd_fsm_trans_TXT_t;

	ftd_fsm_trans_TXT_t ftd_ttstab_TXT[] = {
		/* State			Input			    Next State		     Action  */
	    /* -----			-----			    ----------		     ------  */
		{ "FTD_SNORMAL \0",	"FTD_CINVALID \0",		"FTD_SINVALID \0",		"ftd_fsm_do_abort  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CPASSTHRU \0",	"FTD_SINVALID \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CNORMAL \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CREFRESH \0",		"FTD_STRACKING \0",	"ftd_fsm_do_tracking  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CREFRESHF \0",	"FTD_STRACKING \0",	"ftd_fsm_do_tracking  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CREFRESHC \0",	"FTD_STRACKING \0",	"ftd_fsm_do_tracking  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CBACKFRESH \0",	"FTD_SBACKFRESH \0",	"ftd_fsm_do_backfresh  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CTRACKING \0",	"FTD_STRACKING \0",	"ftd_fsm_do_tracking  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CCPSTART \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_cpstart  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CCPSTOP \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_cpstop  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CCPON \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_cpon  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CCPPEND \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_cppend  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CCPOFF \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_cpoff  \0"},
		{ "FTD_SNORMAL \0",	"FTD_CREFTIMEO \0",	"FTD_SREFOFLOW \0",	"ftd_fsm_do_refoflow  \0"},
		
		{ "FTD_SREFRESH \0",	"FTD_CINVALID \0",		"FTD_SINVALID \0",		"ftd_fsm_do_abort  \0"},
		{ "FTD_SREFRESH \0",	"FTD_CPASSTHRU \0",	"FTD_SINVALID \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESH \0",	"FTD_CREFRESH \0",		"FTD_SREFRESH \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESH \0",	"FTD_CREFRESHF \0",	"FTD_SREFRESH \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESH \0",	"FTD_CREFRESHC \0",	"FTD_SREFRESH \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESH \0",	"FTD_CBACKFRESH \0",	"FTD_SREFRESH \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESH \0",	"FTD_CTRACKING \0",	"FTD_SREFOFLOW \0",	"ftd_fsm_do_refoflow  \0"},
		{ "FTD_SREFRESH \0",	"FTD_CNORMAL \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_normal  \0"},
		{ "FTD_SREFRESH \0",	"FTD_CCPSTART \0",		"FTD_SREFRESH \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESH \0",	"FTD_CCPSTOP \0",		"FTD_SREFRESH \0",		"ftd_fsm_do_report_bad_state  \0"},
		
		{ "FTD_SREFRESHF \0",	"FTD_CINVALID \0",		"FTD_SINVALID \0",		"ftd_fsm_do_abort  \0"},
		{ "FTD_SREFRESHF \0",	"FTD_CPASSTHRU \0",	"FTD_SINVALID \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHF \0",	"FTD_CREFRESH \0",		"FTD_SREFRESHF \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHF \0",	"FTD_CREFRESHF \0",	"FTD_SREFRESHF \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHF \0",	"FTD_CREFRESHC \0",	"FTD_SREFRESHF \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHF \0",	"FTD_CBACKFRESH \0",	"FTD_SREFRESHF \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHF \0",	"FTD_CTRACKING \0",	"FTD_SREFOFLOW \0",	"ftd_fsm_do_refoflow  \0"},
		{ "FTD_SREFRESHF \0",	"FTD_CNORMAL \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_normal  \0"},
		{ "FTD_SREFRESHF \0",	"FTD_CCPSTART \0",		"FTD_SREFRESHF \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHF \0",	"FTD_CCPSTOP \0",		"FTD_SREFRESHF \0",	"ftd_fsm_do_report_bad_state  \0"},

		{ "FTD_SREFRESHC \0",	"FTD_CINVALID \0",		"FTD_SINVALID \0",		"ftd_fsm_do_abort  \0"},
		{ "FTD_SREFRESHC \0",	"FTD_CPASSTHRU \0",	"FTD_SINVALID \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHC \0",	"FTD_CREFRESH \0",		"FTD_SREFRESHC \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHC \0",	"FTD_CREFRESHF \0",	"FTD_SREFRESHC \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHC \0",	"FTD_CREFRESHC \0",	"FTD_SREFRESHC \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHC \0",	"FTD_CBACKFRESH \0",	"FTD_SREFRESHC \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHC \0",	"FTD_CTRACKING \0",	"FTD_SREFOFLOW \0",	"ftd_fsm_do_refoflow  \0"},
		{ "FTD_SREFRESHC \0",	"FTD_CNORMAL \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_normal  \0"},
		{ "FTD_SREFRESHC \0",	"FTD_CCPSTART \0",		"FTD_SREFRESHC \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFRESHC \0",	"FTD_CCPSTOP \0",		"FTD_SREFRESHC \0",	"ftd_fsm_do_report_bad_state  \0"},

		{ "FTD_SBACKFRESH \0",	"FTD_CINVALID \0",		"FTD_SINVALID \0",		"ftd_fsm_do_abort  \0"},
		{ "FTD_SBACKFRESH \0",	"FTD_CPASSTHRU \0",	"FTD_SINVALID \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SBACKFRESH \0",	"FTD_CTRACKING \0",	"FTD_SINVALID \0",		"ftd_fsm_do_report_bad_state	 \0"},
		{ "FTD_SBACKFRESH \0",	"FTD_CREFRESH \0",		"FTD_SBACKFRESH \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SBACKFRESH \0",	"FTD_CREFRESHF \0",	"FTD_SBACKFRESH \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SBACKFRESH \0",	"FTD_CREFRESHC \0",	"FTD_SBACKFRESH \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SBACKFRESH \0",	"FTD_CBACKFRESH \0",	"FTD_SBACKFRESH \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SBACKFRESH \0",	"FTD_CNORMAL \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_normal  \0"},
		{ "FTD_SBACKFRESH \0",	"FTD_CCPSTART \0",		"FTD_SBACKFRESH \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SBACKFRESH \0",	"FTD_CCPSTOP \0",		"FTD_SBACKFRESH \0",	"ftd_fsm_do_report_bad_state  \0"},

		{ "FTD_SREFOFLOW \0",	"FTD_CINVALID \0",		"FTD_SINVALID \0",		"ftd_fsm_do_abort  \0"},
		{ "FTD_SREFOFLOW \0",	"FTD_CPASSTHRU \0",	"FTD_SINVALID \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFOFLOW \0",	"FTD_CREFRESH \0",		"FTD_SREFRESH \0",		"ftd_fsm_do_refresh  \0"},
		{ "FTD_SREFOFLOW \0",	"FTD_CREFRESHF \0",	"FTD_SREFRESHF \0",	"ftd_fsm_do_refresh_full  \0"},
		{ "FTD_SREFOFLOW \0",	"FTD_CREFRESHC \0",	"FTD_SREFRESHC \0",	"ftd_fsm_do_refresh_check  \0"},
		{ "FTD_SREFOFLOW \0",	"FTD_CNORMAL \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_normal  \0"},
		{ "FTD_SREFOFLOW \0",	"FTD_CBACKFRESH \0",	"FTD_SBACKFRESH \0",	"ftd_fsm_do_backfresh  \0"},
		{ "FTD_SREFOFLOW \0",	"FTD_CTRACKING \0",	"FTD_SREFOFLOW \0",	"ftd_fsmnoop  \0"},
		{ "FTD_SREFOFLOW \0",	"FTD_CCPSTART \0",		"FTD_SREFOFLOW \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SREFOFLOW \0",	"FTD_CCPSTOP \0",		"FTD_SREFOFLOW \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_STRACKING \0",	"FTD_CINVALID \0",		"FTD_SINVALID \0",		"ftd_fsm_do_abort  \0"},
		{ "FTD_STRACKING \0",	"FTD_CPASSTHRU \0",	"FTD_SINVALID \0",		"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_STRACKING \0",	"FTD_CREFRESH \0",		"FTD_SREFRESH \0",		"ftd_fsm_do_refresh  \0"},
		{ "FTD_STRACKING \0",	"FTD_CREFRESHF \0",	"FTD_SREFRESHF \0",	"ftd_fsm_do_refresh_full  \0"},
		{ "FTD_STRACKING \0",	"FTD_CREFRESHC \0",	"FTD_SREFRESHC \0",	"ftd_fsm_do_refresh_check  \0"},
		{ "FTD_STRACKING \0",	"FTD_CNORMAL \0",		"FTD_SNORMAL \0",		"ftd_fsm_do_normal  \0"},
		{ "FTD_STRACKING \0",	"FTD_CBACKFRESH \0",	"FTD_SBACKFRESH \0",	"ftd_fsm_do_backfresh  \0"},
		{ "FTD_STRACKING \0",	"FTD_CTRACKING \0",	"FTD_STRACKING \0",	"ftd_fsmnoop  \0"},
		{ "FTD_STRACKING \0",	"FTD_CCPSTART \0",		"FTD_STRACKING \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_STRACKING \0",	"FTD_CCPSTOP \0",		"FTD_STRACKING \0",	"ftd_fsm_do_report_bad_state  \0"},
		{ "FTD_SAPPLY \0",		"FTD_CANY \0",			"FTD_SINVALID \0",		"ftd_fsmnoop  \0"},
		{ "FTD_SINVALID \0",	"FTD_CANY \0",			"FTD_SINVALID \0",		"ftd_fsm_do_abort  \0"},
	};
#endif
/*
 * ftd_fsm_init --
 * Initialize Finite State Machine
 */
void
ftd_fsm_init(u_char fsm[][FTD_NCHRS], ftd_fsm_trans_t ttab[], int nstates)
{
	ftd_fsm_trans_t	*pt;
	int			sn, ti, cn;
	
	for (cn = 0; cn < FTD_NCHRS; cn++)
		for (ti = 0; ti < nstates; ti++)
			fsm[ti][cn] = FTD_TINVALID;
			
	for (ti = 0; ttab[ti].ft_state != FTD_SINVALID; ti++) {
		pt = &ttab[ti];
		sn = pt->ft_state;
		if (pt->ft_char == FTD_CANY) {
			for (cn = 0; cn < FTD_NCHRS; cn++)
				if (fsm[sn][cn] == FTD_TINVALID)
					fsm[sn][cn] = ti;
		} else
			fsm[sn][pt->ft_char] = ti;
	}

	// set all uninitialized indices to an invalid transition 
	for (cn = 0; cn < FTD_NCHRS; cn++)
		for (ti = 0; ti < nstates; ti++)
			if (fsm[ti][cn] == FTD_TINVALID)
				fsm[ti][cn] = ti;

	return;
}

/*
 * ftd_fsm_report_run_state -- 
 * report the state of the group 
 */
static void
ftd_fsm_report_run_state(int state)
{
	
	switch(state) {
	case FTD_SNORMAL:
		reporterr(ERRFAC, M_NORMAL, ERRINFO);
		break;
	case FTD_SREFRESH:
	case FTD_SREFRESHC:
	case FTD_SREFRESHF:
		reporterr(ERRFAC, M_REFRESH, ERRINFO);
		break;
	case FTD_SBACKFRESH:
		reporterr(ERRFAC, M_BACKFRESH, ERRINFO);
		break;
	default:
		break;
	}

	return;
}

/*
 * ftd_fsm_primary -- 
 * primary system state machine
 */
static int
ftd_fsm_primary(ftd_lg_t *lgp, u_char fsm[][FTD_NCHRS],
	ftd_fsm_trans_t ttab[], u_char curstate, u_char curchar)
{
	int		ostate, state, dstate;
	u_char	idx;
	char	inchar;
	ftd_fsm_trans_t	*transp;

	state = curstate;
	inchar = curchar;

	bDbgLogON = 0;

	while (1) {
		if (inchar > 0) 
			{
			error_tracef( TRACEINF, "ftd_fsm_primary():%s FSM input state, inchar = %d, %d",LG_PROCNAME(lgp), state, inchar );
			idx = fsm[state][inchar];
			transp = &ttab[idx];

			error_tracef( TRACEINF, "ftd_fsm_primary():%s FSM state=> current: %s - input: %s - next: %s - action: %s",
					LG_PROCNAME(lgp),
					ftd_ttstab_TXT[idx].ft_state_TXT,
					ftd_ttstab_TXT[idx].ft_char_TXT,
					ftd_ttstab_TXT[idx].ft_next_TXT,
					ftd_ttstab_TXT[idx].ft_action_TXT );
	
			if ((state = (*transp->ft_action)(lgp, transp)) < 0) {
				error_tracef( TRACEINF, "ftd_fsm_primary():%s FSM DIE! state: %d", LG_PROCNAME(lgp),state );
				return FTD_EXIT_DIE;
			} else if (state == FTD_SINVALID) {
				// cause call to do_abort
				inchar = (char)FTD_CINVALID;
				continue;
			}

			// don't record temporary tracking state in driver/pstore
			if (state != FTD_STRACKING && state != FTD_SREFOFLOW) {
				
				// set group state in pstore 
				if (ftd_lg_set_pstore_run_state(lgp->lgnum, lgp->cfgp, state) != 0) {
                    //DbgDmpErrPrintf( "ftd_fsm_primary()", "FSM DIE PSTORE \n" );
					error_tracef( TRACEINF, "ftd_fsm_primary():%s FSM DIE PSTORE! state: %d", LG_PROCNAME(lgp),state );
					return FTD_EXIT_DIE;
				}

				// set group state in driver
				dstate = ftd_lg_state_to_driver_state(state);
					
				if (ftd_lg_set_driver_run_state(lgp->lgnum, dstate) != 0) {
                    //DbgDmpErrPrintf( "ftd_fsm_primary()", "FSM DIE DRIVER \n" );
					error_tracef( TRACEINF, "ftd_fsm_primary():%s FSM DIE DRIVER! state: %d", LG_PROCNAME(lgp),state );
					return FTD_EXIT_DIE;
				}
			}
            else if (state == FTD_STRACKING)
            {
			//WR32435
			//Adding this for the APAC Problem

                // set group state in driver
                dstate = ftd_lg_state_to_driver_state(state);
                    
                if (ftd_lg_set_driver_run_state(lgp->lgnum, dstate) != 0) {
                    //DbgDmpErrPrintf( "ftd_fsm_primary()", "FSM DIE DRIVER \n" );
                    error_tracef( TRACEINF, "ftd_fsm_primary():%s FSM DIE DRIVER! state: %d", LG_PROCNAME(lgp),state );
                    return FTD_EXIT_DIE;
                }
            }

		} else if (inchar < 0) {
			// error
			if (inchar == FTD_LG_NET_BROKEN) {
				
				reporterr(ERRFAC, M_NETBROKE, ERRWARN,
					FTD_SOCK_RHOST(lgp->dsockp));

				ftd_sock_set_disconnect(lgp->dsockp);
				return FTD_EXIT_NETWORK;
			
			} else if (inchar == FTD_LG_NET_TIMEO) {
			
				reporterr(ERRFAC, M_NETTIMEO, ERRWARN,
					FTD_SOCK_LHOST(lgp->dsockp),
					FTD_SOCK_RHOST(lgp->dsockp),
					sock_strerror(sock_errno()));
					
				ftd_sock_set_disconnect(lgp->dsockp);
				return FTD_EXIT_NETWORK;
			
			} else if (inchar == FTD_LG_APPLY_DONE) {
				return 0;
			} else {
				return FTD_EXIT_DIE;
			}
		} else {
			// no state change
		}

		// do peripheral shit
		ftd_lg_housekeeping(lgp, 1);

		ostate = GET_LG_STATE(lgp->flags);

		if (!GET_LG_STARTED(lgp->flags)
			|| (ostate != state
				&& (ostate != FTD_SREFOFLOW || GET_LG_RFTIMEO(lgp->flags))))
		{
			// report new state
			ftd_fsm_report_run_state(state);	
		}

		SET_LG_STATE(lgp->flags, state);
		SET_LG_STARTED(lgp->flags);

		inchar = 0;
		
		// execute FTD function corresponding to current state
		// FTD function returns input to state machine 

		switch(state) {
		case FTD_SNORMAL:
			break;
		case FTD_SREFRESH:
		case FTD_SREFRESHC:
			// process next block
			// can't send blocks until secondary has transitioned
			if (GET_LG_RFSTART_ACKPEND(lgp->flags)) {
				break;
			}

			if (GET_LG_RFTIMEO(lgp->flags)) {
				UNSET_LG_RFTIMEO(lgp->flags);
			}

			if ((inchar = ftd_lg_refresh(lgp)) != 0) {
				continue;
			}
			break;
		case FTD_SREFRESHF:
			// send next block 
			if ((inchar = ftd_lg_refresh_full(lgp)) != 0) {
				continue;
			}
			break;
		case FTD_SBACKFRESH:
			// get next block
			if ((inchar = ftd_lg_backfresh(lgp)) != 0) {
				continue;
			}
			break;
		case FTD_STRACKING:
			if ((inchar = ftd_fsm_tracking_primary(lgp)) != 0) {
				continue;
			}
			break;			
		case FTD_SREFOFLOW:
			if ((inchar = ftd_fsm_refoflow_primary(lgp)) != 0) {
				continue;
			}
			break;
		default:
			// quit state machine
			error_tracef( TRACEWRN, "ftd_fsm_primary():%s FSM QUIT! state: %d",	LG_PROCNAME(lgp), state );
			return FTD_EXIT_DIE;
		}

		inchar = ftd_lg_dispatch_io(lgp);

        if ( bDbgLogON ) { error_tracef( TRACEINF4, "ftd_fsm_primary():%s inchar=%d state=%d(from ftd_lg_dispatch_io())", LG_PROCNAME(lgp), (int)inchar, state ); }


#if !defined(_WINDOWS)
		// reset signal mask
		sigprocmask(SIG_SETMASK, &lgp->fprocp->sigs, NULL);
#endif
	}
	
}

/*
 * ftd_fsm_secondary -- 
 * secondary system state machine
 */
static int
ftd_fsm_secondary(ftd_lg_t *lgp, u_char fsm[][FTD_NCHRS],
	ftd_fsm_trans_t ttab[], u_char curstate, u_char curchar)
{
	int		state;
	u_char	idx;
	char	inchar;
	ftd_fsm_trans_t	*transp;

	state = curstate;
	inchar = curchar;

	while (1) 
		{
		
		if (inchar > 0) 
			{
			// transition to next state 
			//DPRINTF((ERRFAC,LOG_INFO, "%s FSM input state, inchar = %d, %d\n",LG_PROCNAME(lgp), state, inchar));	

			idx = fsm[state][inchar];
			transp = &ttab[idx];

			error_tracef( TRACEINF, "ftd_fsm_secondary():%s FSM state=> current: %s - input: %s - next: %s - action: %s",
					LG_PROCNAME(lgp),
					ftd_ttstab_TXT[idx].ft_state_TXT,
					ftd_ttstab_TXT[idx].ft_char_TXT,
					ftd_ttstab_TXT[idx].ft_next_TXT,
					ftd_ttstab_TXT[idx].ft_action_TXT );
				
			if ((state = (*transp->ft_action)(lgp, transp)) < 0) 
				{
				error_tracef( TRACEWRN, "ftd_fsm_secondary():%s FSM DIE! state: %d", LG_PROCNAME(lgp), state );
				return FTD_EXIT_DIE;
				} 
			else if (state == FTD_SINVALID) 
				{
				// cause call to do_abort
				inchar = (char)FTD_CINVALID;
				continue;
				}
			} 
		else if (inchar < 0) 
			{
			// error
			if (inchar == FTD_LG_NET_BROKEN) 
				{
				reporterr(ERRFAC, M_NETBROKE, ERRWARN,
					FTD_SOCK_RHOST(lgp->dsockp));

				ftd_sock_set_disconnect(lgp->dsockp);

				return FTD_EXIT_DIE;
			
				} 
			else if (inchar == FTD_LG_NET_TIMEO) 
				{
			
				reporterr(ERRFAC, M_NETTIMEO, ERRWARN,
					FTD_SOCK_LHOST(lgp->dsockp),
					FTD_SOCK_RHOST(lgp->dsockp),
					sock_strerror(sock_errno()));
					
				ftd_sock_set_disconnect(lgp->dsockp);

				return FTD_EXIT_DIE;
			
				} 
			else if (inchar == FTD_LG_APPLY_DONE) 
				{
				return 0;
				}
			else 
				{
				return FTD_EXIT_DIE;
				}
			} 
		else // == 0 
			{
			// no state change
			}

		// do peripheral shit
		ftd_lg_housekeeping(lgp, 0);

		SET_LG_STATE(lgp->flags, state);
		SET_LG_STARTED(lgp->flags);

		inchar = 0;
		
		// execute FTD function corresponding to current state
		// FTD function returns input to state machine 

		switch(state) 
			{
			case FTD_SNORMAL:
			case FTD_SREFRESH:
			case FTD_SREFRESHC:
			case FTD_SREFRESHF:
			case FTD_SBACKFRESH:
			case FTD_STRACKING:
			case FTD_SREFOFLOW:
				// nothing to do on secondary
				break;
			case FTD_SAPPLY:
				// apply next chunk 
				if ((inchar = ftd_lg_apply(lgp)) != 0) {
					continue;
				}
				break;
			default:
				// quit state machine
				error_tracef( TRACEWRN, "ftd_fsm_secondary():%s FSM QUIT! state: %d", LG_PROCNAME(lgp), state );
				return FTD_EXIT_DIE;
			}

		inchar = ftd_lg_dispatch_io(lgp);
		} // WHILE
	
}

/*
 * ftd_fsm -- 
 * State Machine
 */
int
ftd_fsm(ftd_lg_t *lgp, u_char fsm[][FTD_NCHRS],
	ftd_fsm_trans_t ttab[], u_char curstate, u_char curchar)
{
	
	if (lgp->cfgp->role == ROLEPRIMARY) {
		return ftd_fsm_primary(lgp, fsm, ttab, curstate, curchar);
	} else if (lgp->cfgp->role == ROLESECONDARY) {
		return ftd_fsm_secondary(lgp, fsm, ttab, curstate, curchar);
	}

	return -1;
}

/*
 * ftd_fsm_state_to_input -- 
 * translate state value to input char value 
 */
u_char
ftd_fsm_state_to_input(int state)
{

	switch(state) {
	case FTD_SNORMAL:
		return FTD_CNORMAL;
	case FTD_SREFRESH:
		return FTD_CREFRESH;
	case FTD_SREFRESHC:
		return FTD_CREFRESHC;
	case FTD_SREFRESHF:
		return FTD_CREFRESHF;
	case FTD_SBACKFRESH:
		return FTD_CBACKFRESH;
	case FTD_SPASSTHRU:
		return FTD_CPASSTHRU;
	case FTD_STRACKING:
		return FTD_CTRACKING;
	default:
		break;
	}

	return 0;
}

/*
 * ftd_fsm_input_to_state -- 
 * translate input char value to state value  
 */
int
ftd_fsm_input_to_state(short inchar)
{

	switch(inchar) {
	case FTD_CNORMAL:
		return FTD_SNORMAL;
	case FTD_CREFRESH:
		return FTD_SREFRESH;
	case FTD_CREFRESHC:
		return FTD_SREFRESHC;
	case FTD_CREFRESHF:
		return FTD_SREFRESHF;
	case FTD_CBACKFRESH:
		return FTD_SBACKFRESH;
	case FTD_CPASSTHRU:
		return FTD_SPASSTHRU;
	case FTD_CTRACKING:
		return FTD_STRACKING;
	default:
		break;
	}

	return 0;
}

/*
 * ftd_fsm_noop --
 * does nothing but return target lg state.
 */
static int
ftd_fsm_noop(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	return trans->ft_next;  
}

/*
 * ftd_fsm_do_abort --
 * exit 
 */
static int
ftd_fsm_do_abort(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	error_tracef( TRACEINF4, "ftd_fsm_do_abort():%s", LG_PROCNAME(lgp) );

	if (lgp->cfgp->role == ROLEPRIMARY) {
		reporterr(ERRFAC, M_PMDEXIT, ERRINFO);
	} else {
		reporterr(ERRFAC, M_RMDEXIT, ERRINFO);
	}

	ftd_lg_delete(lgp);

	exit(FTD_EXIT_DIE);

	return 0;
}

/*
 * ftd_fsm_do_normal_primary --
 * transition to FTD_NORMAL state on primary.
 */
static int
ftd_fsm_do_normal_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	ftd_dev_t		**devpp, *devp;
	ftd_dev_stat_t	*statp;
	ftd_lg_info_t	lgip;
	ftd_header_t	ack;
	char			*devbuf;
	int				rc, state;
	
	error_tracef( TRACEINF4, "ftd_fsm_do_normal_primary():%s", LG_PROCNAME(lgp) );

	memset(&ack, 0, sizeof(ack));
	ack.msg.lg.lgnum = lgp->lgnum;
	ack.msg.lg.data = lgp->cfgp->role;

	switch(trans->ft_state) {
	case FTD_SREFRESH:
	case FTD_SREFRESHC:
	case FTD_SREFRESHF:

		lgp->refreshts = 0;
		
		if (GET_LG_RFDONE(lgp->flags)) {

			UNSET_LG_RFDONE_ACKPEND(lgp->flags);
			UNSET_LG_RFDONE(lgp->flags);

			reporterr(ERRFAC, M_REFREND, ERRINFO);

			ftd_stat_dump_driver(lgp);
		} else {
			// we got here by launch while running in refresh
			state = trans->ft_state;
				
			trans->ft_state = FTD_SNORMAL;
			ftd_fsm_do_report_bad_state(lgp, trans);
			trans->ft_state = state;

			return trans->ft_state;
		}	        
		
		ftd_lg_init_devs(lgp);
		ftd_stat_dump_driver(lgp);
		
		break;
	case FTD_SBACKFRESH:
		UNSET_LG_RFDONE(lgp->flags);
		
		reporterr(ERRFAC, M_BACKEND, ERRINFO);
		
		ftd_sock_send_rsync_end(lgp->dsockp, lgp,
			GET_LG_STATE(lgp->flags));

		ftd_stat_dump_driver(lgp);
		ftd_lg_init_devs(lgp);
		ftd_stat_dump_driver(lgp);

#if defined(_WINDOWS)
		// unlock the devices
		if (ftd_lg_unlock_devs(lgp) < 0) {
			goto errret;
		}
#endif
		break;
	default:
		ftd_lg_init_devs(lgp);
		break;
	}

	if (lgp->db && lgp->db->dbbuf) {
		// free the bit map 
		free(lgp->db->dbbuf);
		lgp->db->dbbuf = NULL;
	}

	if ((rc = ftd_ioctl_get_groups_info(lgp->ctlfd,
		lgp->lgnum, &lgip, 0)) != 0)
	{
		goto errret;
	}

	devbuf = (char*)calloc(1, lgip.statsize);
    
	// free device rsync resources 
	ForEachLLElement(lgp->devlist, devpp) {
		
		devp = *devpp;
		
		statp = (ftd_dev_stat_t*)devbuf;
		
		// retain devid, connection status
		statp->connection = devp->statp->connection;
		statp->devid = devp->statp->devid;

		if ((rc = ftd_ioctl_set_dev_state_buffer(lgp->ctlfd, lgp->lgnum,
			devp->ftdnum, lgip.statsize, devbuf, 0)) != 0)
		{
			goto errret;
		}
		
		// close raw disk device 
		if (devp->devfd != INVALID_HANDLE_VALUE) {
#if defined(_WINDOWS)
			if (ftd_dev_unlock(devp->devfd) == 0)
				ftd_close(devp->devfd);
#else
				ftd_close(devp->devfd);
#endif
			devp->devfd = INVALID_HANDLE_VALUE;
		}
	}
	
	free(devbuf);

	// update the LRDBs for the group 
	if ((rc = ftd_ioctl_update_lrdbs(lgp->ctlfd, lgp->lgnum, 0)) != 0) {
		goto errret;
	}

	ack.msgtype = FTDACKCLD;

	FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_fsm_do_normal_primary",lgp->isockp, &ack);

	if (trans->ft_state == FTD_SBACKFRESH) {
		
		ftd_sock_set_b(lgp->dsockp);

		// check for mirror enabled
		if ((rc = ftd_sock_send_chkconfig(lgp)) < 0) {
			
			// set group state in pstore to ACCUMULATE
			if (ftd_lg_set_pstore_run_state(lgp->lgnum, lgp->cfgp, 0) != 0) {
				goto errret;
			}

			// set group state in pstore to TRACKING
			if (ftd_lg_set_driver_run_state(lgp->lgnum, FTD_MODE_TRACKING) != 0) {
				goto errret;
			}

			goto errret;
		}
	
		ftd_sock_set_nonb(lgp->dsockp);
	}

	return trans->ft_next;
	
errret:

	ack.msgtype = FTDACKCLD;

	FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_fsm_do_normal_primary",lgp->isockp, &ack);

	return -1;
}

/*
 * ftd_fsm_do_normal_secondary --
 * transition to FTD_NORMAL state on secondmary.
 */
static int
ftd_fsm_do_normal_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	int				modebits;

	error_tracef( TRACEINF4, "ftd_fsm_do_normal_secondary():%s", LG_PROCNAME(lgp) );

	switch(trans->ft_state) {
	case FTD_SREFRESH:
	case FTD_SREFRESHC:
		// notify primary as it is waiting for the ACK
		SET_LG_RFDONE(lgp->flags);
	case FTD_SREFRESHF:
		if (GET_JRN_MODE(lgp->jrnp->flags) == FTDMIRONLY) {
			ftd_lg_close_devs(lgp);
           
			if (lgp->cfgp->chaining) {
				modebits = O_RDWR;
			} else {
				modebits = O_RDWR | O_EXCL;
			}
			
			if (ftd_lg_open_devs(lgp, modebits, 0, 5) < 0) {
				return -1;
			}
		} else {
			if (ftd_lg_stop_journaling(lgp, FTDJRNCO, FTDJRNMIR) < 0) {
				goto errret;
			}
		}
		
		ftd_lg_init_devs(lgp);

		break;
	case FTD_SBACKFRESH:

		ftd_lg_init_devs(lgp);

		break;
	default:
		break;
	}

	return trans->ft_next;

errret:

	return -1;
}

/*
 * ftd_fsm_do_normal --
 * transition to FTD_NORMAL state from some other state.
 */
static int
ftd_fsm_do_normal(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	error_tracef( TRACEINF4, "ftd_fsm_do_normal():%s", LG_PROCNAME(lgp) );

	if (lgp->cfgp->role == ROLEPRIMARY) {
		return ftd_fsm_do_normal_primary(lgp, trans);
	} else {
		return ftd_fsm_do_normal_secondary(lgp, trans);
	}

}

/*
 * ftd_fsm_do_refresh_primary --
 * transition to FTD_REFRESH state on primary.
 */
static int
ftd_fsm_do_refresh_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	ftd_stat_t		statp;
	ftd_dev_info_t	*dip = NULL, *mapp;
	ftd_dev_t		**devpp, *devp;
	ftd_dev_cfg_t	*devcfgp;
	ftd_header_t	ack;
	int				rc, maskoff, masklen, i;
	time_t			now;
	
	error_tracef( TRACEINF4, "ftd_fsm_do_refresh_primary():%s", LG_PROCNAME(lgp) );

	memset(&ack, 0, sizeof(ack));
	ack.msg.lg.lgnum = lgp->lgnum;
	ack.msg.lg.data = lgp->cfgp->role;
	ack.msgtype = FTDACKCLD;

	// ack the master so it can tell any client that is waiting
			
	FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_fsm_do_refresh_primary",lgp->isockp, &ack);
	
	if (lgp->refreshts == 0) 
    {
		// first try
		time(&lgp->refreshts);
		lgp->timerefresh = 0;
	} 
    else 
    {
		time(&now);
		lgp->timerefresh = now - lgp->refreshts;
	}

	if (lgp->tunables->refrintrvl >= 0) 
    {
		
		// we have overflowed the BAB and the user has
		// a finite refresh interval set
		
		if (lgp->oflowts
			&& (lgp->tunables->refrintrvl == 0 
				|| lgp->timerefresh > lgp->tunables->refrintrvl))
		{
			// we have overflowed the BAB and we have hit the
			// refresh time threshold. force refresh overflow state
					
			if ((rc = ftd_fsm_do_refoflow(lgp, trans)) < 0) {
				return rc;
			}
					
			return FTD_SREFOFLOW;
		}
	}
	
	ftd_lg_init_devs(lgp);

	switch(trans->ft_state) {
	case FTD_SREFOFLOW:
	case FTD_STRACKING:

		// only two states that can get us here

		// update the LRDBs for the group 
		if (ftd_ioctl_update_lrdbs(lgp->ctlfd, lgp->lgnum, 0) != 0) {
			return -1;
		}

		// update the HRDBs for the group 
		if (ftd_ioctl_update_hrdbs(lgp->ctlfd, lgp->lgnum, 0) != 0) {
			return -1;
		}

		// clear bab entries - we have the bits
		if (ftd_ioctl_clear_bab(lgp->ctlfd, lgp->lgnum) < 0) {
			return -1;
		}

		// reset any flags that may be pending a sentinel
		// since we just blew away the bab and don't want
		// to try to migrate

		UNSET_LG_CPSTART(lgp->flags);
		UNSET_LG_CPSTOP(lgp->flags);

		lgp->offset = 0;
		lgp->datalen = 0;
		lgp->bufoff = 0;
		
		SET_LG_RFSTART_ACKPEND(lgp->flags);

		if ((rc = ftd_ioctl_get_group_stats(lgp->ctlfd,
			lgp->lgnum, &statp, 0)) != 0)
		{
			goto errret;
		}

		// insert INCOHERENT sentinel into BAB to notify secondary
		// that everything after this and before a coherent
		// sentinel is to be treated as incoherent data and
		// journaled - so we are recoverable on secondary in case
		// the refresh does not complete.

		if (ftd_ioctl_send_lg_message(lgp->devfd,
			lgp->lgnum, MSG_INCO) < 0)
		{
			goto errret;
		}

		if ((dip = (ftd_dev_info_t*)
			calloc(statp.ndevs, sizeof(ftd_dev_info_t))) == NULL)
		{
			reporterr(ERRFAC, M_MALLOC, ERRWARN,
				(statp.ndevs*sizeof(ftd_dev_info_t)));
			goto errret;
		}
		
		if ((lgp->db->dbbuf =
			(int*)realloc(lgp->db->dbbuf,
				(statp.ndevs*FTD_PS_HRDB_SIZE))) == NULL)
		{
			reporterr(ERRFAC, M_MALLOC, ERRWARN,
				(statp.ndevs*FTD_PS_HRDB_SIZE));
			goto errret;
		}
		
		// set driver state to REFRESH - so we can get the bits.
		// (if we get the bits while in TRACKING then we may miss
		// some that come in after we grab the maps. In REFRESH we have
		// the BAB entry as a safety precaution if an i/o arrives after
		// we've grabbed the bits).
		//
		// this op will clear bits if going from NORMAL->REFRESH
		// this op will always flush the low-res bits to pstore

		if (ftd_lg_set_driver_run_state(lgp->lgnum, FTD_MODE_REFRESH) < 0) {
			goto errret;
		}
		
		if (trans->ft_next == FTD_SREFRESHC) {
			// set bits - if checksumming
			if ((rc = ftd_ioctl_set_lrdbs(lgp->ctlfd, lgp->devfd,
				lgp->lgnum, lgp->db, dip, statp.ndevs)) != 0)
			{
				goto errret;
			}
			
			if ((rc = ftd_ioctl_set_hrdbs(lgp->ctlfd, lgp->devfd,
				lgp->lgnum, lgp->db, dip, statp.ndevs)) != 0)
			{
				goto errret;
			}
		}
	
		// get dirty bit maps from driver
		if ((rc = ftd_ioctl_get_hrdbs(lgp->ctlfd, lgp->devfd,
			lgp->lgnum, lgp->db, dip)) != 0)
		{
			goto errret;
		}

#if 0
		// dump device bit map
		{
			for (i=0;i<2048;i++) {
				printf("%02x",(unsigned char)lgp->db->dbbuf[i]);
			}
			printf("\n");
		}
#endif
		// send it - oldest entry is the sentinel - since it was
		// inserted in tracking with the bab clear prior
		
		if ((rc = ftd_lg_proc_bab(lgp, 1)) < 0) {
			goto errret;
		}

		// point lg devs at hrdbs
		ForEachLLElement(lgp->devlist, devpp) {
			devp = *devpp;
			
			if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, devp->devid)) == NULL) {
				reporterr(ERRFAC, M_PROTDEV, ERRCRIT, devp->devid);
				goto errret;
			}

			// find device in dbarray dev list
			maskoff = 0;
			error_tracef( TRACEINF, "ftd_fsm_do_refresh_primary():lgp->db->numdevs = %d", lgp->db->numdevs );

			for (i = 0; i < lgp->db->numdevs; i++) {
				mapp = dip+i;
				error_tracef( TRACEINF4, "ftd_fsm_do_refresh_primary():devp->ftdnum, mapp->cdev = %08x, %08x", devp->ftdnum, mapp->cdev );
				if (mapp->cdev == devp->ftdnum) {
					break;
				}
				maskoff += mapp->hrdbsize32; 
			}
			
			if (i == lgp->db->numdevs) {
				reporterr(ERRFAC, M_NOHRT, ERRCRIT,
					devcfgp->devname, (int)devp->ftdnum);
				goto errret;
			}
			
			devp->dblen = mapp->hrdbsize32*sizeof(int)*8;
			devp->dbres = 1 << mapp->hrdb_res;
			devp->dbvalid = mapp->hrdb_numbits;
			devp->db = (unsigned char*)lgp->db->dbbuf + (maskoff*sizeof(int));

#ifdef TDMF_TRACE
			fprintf(stderr,"\n*** get_highres: found device: %s\n",
				devcfgp->devname);
			fprintf(stderr,"\n*** maskoff = %d\n",maskoff);
			fprintf(stderr,"\n*** mapp->hrdbsize32 = %d\n",mapp->hrdbsize32);
			fprintf(stderr,"\n*** mapp->hrdb_res = %d\n",mapp->hrdb_res);
			fprintf(stderr,"\n*** mapp->disksize = %d\n",mapp->disksize);
			fprintf(stderr,"\n*** mapp->hrdb_numbits = %d\n",mapp->hrdb_numbits);
#endif

#if 0
			// dump device bit map
			{
				int bytes = devp->dblen/8;
				for (i=0;i<2048;i++) {
					printf("%02x",(unsigned char)devp->db[i]);
				}
				printf("\n");
			}
#endif

		}
		masklen = 0;
		for (i = 0; i < lgp->db->numdevs; i++) {
			mapp = dip+i;
			masklen += mapp->hrdbsize32; 
		}

		SET_LG_CHKSUM(lgp->flags);
		for (i = 0; i < masklen-1; i++) {
			if (~lgp->db->dbbuf[i]) {
				UNSET_LG_CHKSUM(lgp->flags);
				break;
			}
		}
		break;
	default:
		break;
	}

	ftd_lg_close_devs(lgp);

	if (ftd_lg_open_devs(lgp, O_RDWR, 0, 5) < 0) {
		goto errret;
	}

	if (dip)
		free(dip);

	return trans->ft_next;
	
errret:

	if (dip)
		free(dip);
	
	return -1;
}

/*
 * ftd_fsm_do_refresh_secondary --
 * transition to FTD_REFRESH state on secondary.
 */
static int
ftd_fsm_do_refresh_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	ftd_header_t	ack;
	int				rc;

	error_tracef( TRACEINF4, "ftd_fsm_do_refresh_secondary():%s", LG_PROCNAME(lgp) );

	// close all open journal files
	if (ftd_journal_close_all_files(lgp->jrnp) < 0) {
		return -1;
	}

	ftd_lg_init_devs(lgp);

	// prune INCO journal files
	if (ftd_journal_get_all_files(lgp->jrnp) < 0) {
		return -1;
	}
	
	if (ftd_journal_delete_inco_files(lgp->jrnp) < 0) {
		return -1;
	}
	
	// secondary - start journaling & return next state
	if (ftd_lg_start_journaling(lgp, FTDJRNINCO) < 0) {
		return -1;
	}
	
	SET_LG_STATE(lgp->flags, FTD_SREFRESH);
	UNSET_LG_RFDONE(lgp->flags);

	SET_LG_RFSTART(lgp->flags);

	// tell primary that we have transitioned to refresh

	memset(&ack, 0, sizeof(ack));
	ack.msgtype = FTDACKRFSTART;
	
	if ((rc = ftd_sock_send_lg(lgp->dsockp, lgp,
		(char*)&ack, sizeof(ack))) < 0)
	{
		return -1;
	}

	return trans->ft_next;
}

/*
 * ftd_fsm_do_refresh --
 * transition to FTD_REFRESH state.
 */
static int
ftd_fsm_do_refresh(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	error_tracef( TRACEINF4, "ftd_fsm_do_refresh():%s", LG_PROCNAME(lgp) );

	if (lgp->cfgp->role == ROLEPRIMARY) {
		return ftd_fsm_do_refresh_primary(lgp, trans);
	} else {
		return ftd_fsm_do_refresh_secondary(lgp, trans);
	}

}

/*
 * ftd_fsm_do_refresh_full_primary --
 * transition to FTD_REFRESHF state on primary.
 */
static int
ftd_fsm_do_refresh_full_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	ftd_header_t	ack;
	time_t			now;
	int				rc, modebits = O_RDWR;

	error_tracef( TRACEINF4, "ftd_fsm_do_refresh_full_primary():%s", LG_PROCNAME(lgp) );

	memset(&ack, 0, sizeof(ack));
	ack.msg.lg.lgnum = lgp->lgnum;
	ack.msg.lg.data = lgp->cfgp->role;

	// ack the master so it can tell any client that is waiting
	ack.msgtype = FTDACKCLD;

	if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_fsm_do_refresh_full_primary",lgp->isockp, &ack)) < 0) 
    {
		return rc;
	}

	if (lgp->refreshts == 0) 
    {
		// first try
		time(&lgp->refreshts);
		lgp->timerefresh = 0;
	} 
    else 
    {
		time(&now);
		lgp->timerefresh = now - lgp->refreshts;
	}

	if (lgp->tunables->refrintrvl >= 0) 
    {
		// we have overflowed the BAB and the user has
		// a finite refresh interval set
				
		if (lgp->oflowts
			&& (lgp->tunables->refrintrvl == 0 
				|| lgp->timerefresh > lgp->tunables->refrintrvl))
		{
			// we have hit the refresh time threshold 
			// force refresh overflow state
					
			if ((rc = ftd_fsm_do_refoflow(lgp, trans)) < 0) 
            {
				return rc;
			}
					
			return FTD_SREFOFLOW;
		}
	}

	switch(trans->ft_state) 
    {
	case FTD_SREFOFLOW:
	case FTD_STRACKING:
		// clear bab entries - we have the bits
		if (ftd_ioctl_clear_bab(lgp->ctlfd, lgp->lgnum) < 0) 
        {
			return -1;
		}
		
		lgp->offset = 0;
		lgp->datalen = 0;
		lgp->bufoff = 0;
	
		break;
	default:
		break;
	}

	ftd_lg_init_devs(lgp);

	// tell secondary to transition to refreshf state
	if (ftd_sock_send_rsync_start(lgp->dsockp, lgp, FTDCRFFSTART) < 0) 
    {
		goto errret;
	}

	// reset checkpoint state flags - since we will be overriding them
	UNSET_LG_CPON(lgp->flags);
	UNSET_LG_CPPEND(lgp->flags);
	UNSET_LG_CPSTART(lgp->flags);
	UNSET_LG_CPSTOP(lgp->flags);
	UNSET_LG_RFDONE(lgp->flags);

	// set checkpoint off in pstore
	if (ps_set_group_checkpoint(lgp->cfgp->pstore, lgp->devname, 0) < 0) {
		goto errret;
	}

	ftd_lg_close_devs(lgp);

	// open devices 
#if defined(_WINDOWS)
	if (lgp->cfgp->chaining) {
		modebits = O_RDWR | O_EXCL;
	} else {
		modebits = O_RDWR;
	}
#endif
	if (ftd_lg_open_devs(lgp, modebits, 0, 5) < 0) {
		goto errret;
	}

	return trans->ft_next;
	
errret:

	return -1;
}

/*
 * ftd_fsm_do_refresh_full_secondary --
 * transition to FTD_REFRESHF state on secondary.
 */
static int
ftd_fsm_do_refresh_full_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	error_tracef( TRACEINF4, "ftd_fsm_do_refresh_full_secondary():%s", LG_PROCNAME(lgp) );

	// reset checkpoint state flags - since we will be overriding them
	UNSET_LG_CPON(lgp->flags);
	UNSET_LG_CPPEND(lgp->flags);
	UNSET_LG_CPSTART(lgp->flags);
	UNSET_LG_CPSTOP(lgp->flags);
	UNSET_LG_RFDONE(lgp->flags);

	// stop apply process
	ftd_sock_send_stop_apply(lgp->lgnum, lgp->isockp);

	// get journals
	if (ftd_journal_get_all_files(lgp->jrnp) < 0) {
		return -1;
	}

	// nuke journals
	ftd_journal_delete_all_files(lgp->jrnp);

	// stop journaling
	if (ftd_lg_stop_journaling(lgp, FTDJRNCO, FTDMIRONLY) < 0) {
		return -1;
	}

	ftd_lg_init_devs(lgp);

	return trans->ft_next;
}

/*
 * ftd_fsm_do_refresh_full --
 * transition to FTD_REFRESHF state.
 */
static int
ftd_fsm_do_refresh_full(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	error_tracef( TRACEINF4, "ftd_fsm_do_refresh_full():%s", LG_PROCNAME(lgp) );

	if (lgp->cfgp->role == ROLEPRIMARY) {
		return ftd_fsm_do_refresh_full_primary(lgp, trans);
	} else {
		return ftd_fsm_do_refresh_full_secondary(lgp, trans);
	}

}

/*
 * ftd_fsm_do_refresh_check --
 * transition to FTD_REFRESHC state.
 */
static int
ftd_fsm_do_refresh_check(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	return ftd_fsm_do_refresh(lgp, trans);
}

/*
 * ftd_fsm_do_backfresh_primary --
 * transition to FTD_BACKFRESH state on primary.
 */
static int
ftd_fsm_do_backfresh_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	ftd_dev_t		**devpp, *devp;
	ftd_header_t	ack, mack;
	int				rc;

	error_tracef( TRACEINF4, "ftd_fsm_do_backfresh_primary():%s", LG_PROCNAME(lgp) );

	// init ack packet for master
	memset(&mack, 0, sizeof(mack));
	
	mack.msgtype = FTDACKCLD;
	mack.msg.lg.lgnum = lgp->lgnum;
	mack.msg.lg.data = lgp->cfgp->role;

	// flush wire before transitioning to backfresh state
	// because we are going to hammer bab contents 

	if (ftd_sock_flush(lgp) < 0) {
		goto errret;
	}
	
	// tell secondary to transition to backfresh state
	if (ftd_sock_send_rsync_start(lgp->dsockp, lgp, FTDCBFSTART) < 0) {
		goto errret;
	}
		
	// wait here for ack to know if we can proceed 
	if (FTD_SOCK_RECV_HEADER(LG_PROCNAME(lgp) ,"ftd_fsm_do_backfresh_primary",lgp->dsockp, &ack) < 0) {
		goto errret;
	}

	if (ack.msgtype == FTDACKERR) {
		// get the error msg from remote
		ftd_sock_recv_err(lgp->dsockp, &ack);
		
		// ack master
		rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_fsm_do_backfresh_primary",lgp->isockp, &mack);
		
		if (GET_LG_STARTED(lgp->flags)) {
			return trans->ft_state;
		} else {
			return FTD_CINVALID;
		}
	}

	// set driver state to BACKFRESH - so we can get bitmaps
	if (ftd_lg_set_driver_run_state(lgp->lgnum, FTD_MODE_BACKFRESH) < 0) {
		goto errret;
	}

	// clear bab entries - we don't want to send
	if (ftd_ioctl_clear_bab(lgp->ctlfd, lgp->lgnum) < 0) {
		goto errret;
	}
		
#if !defined(_WINDOWS)	
	// if ftd devs are busy then blow out of here
	// this returns success on Solaris
/*
this doesn't work on UNIX either (i.e. open succeeds regardless).
Best we can do is query for mounted filesystems, I guess.	
	
	ftd_lg_close_ftd_devs(lgp);
	if (ftd_lg_open_ftd_devs(lgp, O_EXCL | O_RDWR, 0, 5) < 0) {
		goto errret;
	}
	ftd_lg_close_ftd_devs(lgp);
*/
	ftd_lg_close_devs(lgp);
	
	if (ftd_lg_open_devs(lgp, O_RDWR, 0, 5) < 0) {
		goto errret;
	}

#else
	// NT - try to lock down the ftd devs. If we can't
	// then blow out.
	
	if (ftd_lg_unlock_devs(lgp) < 0) {
		goto errret;
	}
	// this call will get device handles for all of the group's devices
	if (ftd_lg_lock_devs(lgp, ROLEPRIMARY) < 0) {
		goto errret;
	}

#endif

	// init devices
	ForEachLLElement(lgp->devlist, devpp) {
		devp = *devpp;

		// tell secondary to init the device 
		if (ftd_sock_send_rsync_devs(lgp->dsockp, lgp,
			devp, FTD_SBACKFRESH) < 0) {
			goto errret;
		}
	}

	ftd_lg_init_devs(lgp);
	FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_fsm_do_backfresh_primary",lgp->isockp, &mack);

	return trans->ft_next;
	
errret:

	FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_fsm_do_backfresh_primary",lgp->isockp, &mack);

	return -1;
}

/*
 * ftd_fsm_do_backfresh_secondary --
 * transition to FTD_BACKFRESH state on secondary.
 */
static int
ftd_fsm_do_backfresh_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	ftd_header_t	ack;
	int				jrncnt;

	error_tracef( TRACEINF4, "ftd_fsm_do_backfresh_secondary():%s", LG_PROCNAME(lgp) );

	jrncnt = ftd_journal_get_all_files(lgp->jrnp);
		
	if (GET_LG_CPON(lgp->flags)
		|| (jrncnt > 0 && ftd_journal_co(lgp->jrnp)))
	{
		if (GET_LG_BACK_FORCE(lgp->flags)) {
			// forced operation  
			
			UNSET_LG_BACK_FORCE(lgp->flags);
			
			// kill apply
			if (ftd_sock_send_stop_apply(lgp->lgnum, lgp->isockp) < 0) {
				goto errret;
			}
			// delete all journal files
			if (ftd_journal_delete_all_files(lgp->jrnp) < 0) {
				goto errret;
			}
		} else {
			// report an error
			reporterr(ERRFAC, M_BACKFAIL, ERRCRIT);
			
			// tell primary
			ftd_sock_send_err(lgp->dsockp, ftd_get_last_error(ERRFAC));
			
			if (GET_LG_STARTED(lgp->flags)) {
				return trans->ft_state;
			} else {
				return FTD_CINVALID;
			}
		}
	}
		
	UNSET_LG_RFDONE(lgp->flags);

	memset(&ack, 0, sizeof(ack));
	
	ack.msg.lg.lgnum = lgp->lgnum;
	ack.msg.lg.data = lgp->cfgp->role;

#if !defined(_WINDOWS)	
	// if ftd devs are busy then blow out of here
	// this returns success on Solaris
/*
this doesn't work on UNIX either (i.e. open succeeds regardless).
Best we can do is query for mounted filesystems, I guess.	
	
	ftd_lg_close_ftd_devs(lgp);
	if (ftd_lg_open_ftd_devs(lgp, O_EXCL | O_RDWR, 0, 5) < 0) {
		goto errret;
	}
	ftd_lg_close_ftd_devs(lgp);
*/
	ftd_lg_close_devs(lgp);
	if (ftd_lg_open_devs(lgp, O_RDWR, 0, 5) < 0) {
		goto errret;
	}

#else
	// NT - try to lock down the ftd devs. If we can't
	// then blow out.

	if (ftd_lg_unlock_devs(lgp) < 0) {
		goto errret;
	}
	if (ftd_lg_lock_devs(lgp, ROLESECONDARY) < 0) {
		ftd_lg_unlock_devs(lgp);
		goto errret;
	}
#endif

	ftd_lg_init_devs(lgp);

	ack.msgtype = FTDACKCLI;

	FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_fsm_do_backfresh_secondary",lgp->dsockp, &ack);

	return trans->ft_next;
	
errret:

	ack.msgtype = FTDACKERR;
	FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_fsm_do_backfresh_secondary",lgp->dsockp, &ack);
	return -1;
}

/*
 * ftd_fsm_do_backfresh --
 * transition to FTD_BACKFRESH state.
 */
static int
ftd_fsm_do_backfresh(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	error_tracef( TRACEINF4, "ftd_fsm_do_backfresh():%s", LG_PROCNAME(lgp) );

	if (lgp->cfgp->role == ROLEPRIMARY) {
		return ftd_fsm_do_backfresh_primary(lgp, trans);
	} else {
		return ftd_fsm_do_backfresh_secondary(lgp, trans);
	}

}

/*
 * ftd_fsm_do_refoflow_primary --
 * transition to REFRESH OVERFLOW state on primary.
 */
static int
ftd_fsm_do_refoflow_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	ftd_header_t	header;
	ftd_dev_t		*devp, **devpp;
	int				rc;

	error_tracef( TRACEINF4, "ftd_fsm_do_refoflow_primary():%s", LG_PROCNAME(lgp) );

	UNSET_LG_RFSTART_ACKPEND(lgp->flags);
	UNSET_LG_RFDONE_ACKPEND(lgp->flags);
	UNSET_LG_RFDONE(lgp->flags);
		
	lgp->oflowts = 0;

	// tell secondary
		
	memset(&header, 0, sizeof(header));
	header.msgtype = FTDCREFOFLOW;
		
	if (ftd_sock_send_lg(lgp->dsockp, lgp,
		(char*)&header, sizeof(header)) < 0)
	{
		return -1;
	}

	// send a hup packet - to facilitate flushing the wire
	if ((rc = ftd_sock_send_hup(lgp)) < 0) {
		return rc;
	}

	UNSET_SOCK_HUP(FTD_SOCK_FLAGS(lgp->dsockp));

	ForEachLLElement(lgp->devlist, devpp) {
		devp = *devpp;
		if (devp->deltamap) {
			FreeLList(devp->deltamap);
			devp->deltamap = NULL;
		}
		if (devp->sumbuf) {
			free(devp->sumbuf);
			devp->sumbuf = NULL;
			devp->sumbuflen = 0;
		}
	}

	return trans->ft_next;  
}

/*
 * ftd_fsm_do_refoflow_secondary --
 * transition to REFRESH OVERFLOW state on secondary.
 */
static int
ftd_fsm_do_refoflow_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	ftd_dev_t		*devp, **devpp;

	error_tracef( TRACEINF4, "ftd_fsm_do_refoflow_secondary():%s", LG_PROCNAME(lgp) );

	ForEachLLElement(lgp->devlist, devpp) {
		devp = *devpp;
		if (devp->deltamap) {
			FreeLList(devp->deltamap);
			devp->deltamap = NULL;
		}
		if (devp->sumbuf) {
			free(devp->sumbuf);
			devp->sumbuf = NULL;
			devp->sumbuflen = 0;
		}
	}

	return trans->ft_next;  
}

/*
 * ftd_fsm_do_refoflow --
 * transition to REFRESH OVERFLOW state.
 */
static int
ftd_fsm_do_refoflow(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	error_tracef( TRACEINF4, "ftd_fsm_do_refoflow():%s", LG_PROCNAME(lgp) );

	if (lgp->cfgp->role == ROLEPRIMARY) {
		return ftd_fsm_do_refoflow_primary(lgp, trans);
	} else {
		return ftd_fsm_do_refoflow_secondary(lgp, trans);
	}

}

/*
 * ftd_fsm_do_tracking_primary --
 * transition to TRACKING state on primary.
 */
static int
ftd_fsm_do_tracking_primary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	int	rc, state;

	// primary system 

	error_tracef( TRACEINF4, "ftd_fsm_do_tracking_primary():%s", LG_PROCNAME(lgp) );

	// send a hup packet - to facilitate flushing the wire
	if ((rc = ftd_sock_send_hup(lgp)) < 0) 
    {
		return rc;
	}

	UNSET_SOCK_HUP(FTD_SOCK_FLAGS(lgp->dsockp));

	ftd_lg_init_devs(lgp);

	state = ftd_fsm_input_to_state(trans->ft_char);

	if (state == FTD_STRACKING) 
    {
		// NORMAL->TRACKING - force to REFRESH next
		if (!GET_LG_CPON(lgp->flags)) 
        {
			// only REFRESH if we are NOT in checkpoint   rddev 021126
		state = FTD_SREFRESH;
		} 
        else 
        {
			state = FTD_SREFOFLOW;
			if ((rc = ftd_lg_set_pstore_run_state(lgp->lgnum, lgp->cfgp, state)) < 0) 
            {
				return rc;
			}
			
			// we have overflowed the BAB in CHECKPOINT ON MODE, go to REFOFLOW state
			if ((rc = ftd_fsm_do_refoflow(lgp, trans)) < 0) 
            {
				return rc;
			}

			return FTD_SREFOFLOW;
		}
	}

	if ((rc = ftd_lg_set_pstore_run_state(lgp->lgnum, lgp->cfgp, state)) < 0) 
    {
		return rc;
	}

	return trans->ft_next;
}

/*
 * ftd_fsm_do_tracking_secondary --
 * there is no such thing as TRACKING on secondary 
 * transition to REFRESH state on secondary.
 */
static int
ftd_fsm_do_tracking_secondary(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	int	rc, state;

	// force refresh state for secondary
	state = ftd_fsm_input_to_state(trans->ft_char);
		
	switch(state) {
	case FTD_SREFRESH:
	case FTD_SREFRESHC:
	case FTD_SNORMAL:
		if ((rc = ftd_fsm_do_refresh(lgp, trans)) < 0) {
			return rc;
		}
		return FTD_SREFRESH;
	case FTD_SREFRESHF:
		if ((rc = ftd_fsm_do_refresh_full(lgp, trans)) < 0) {
			return rc;
		}
		return FTD_SREFRESHF;
	default:
		// this should never happen
		return -1;
	}

}

/*
 * ftd_fsm_do_tracking --
 * transition to TRACKING state.
 */
static int
ftd_fsm_do_tracking(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	if (lgp->cfgp->role == ROLEPRIMARY) {
		return ftd_fsm_do_tracking_primary(lgp, trans);
	} else {
		return ftd_fsm_do_tracking_secondary(lgp, trans);
	}

}

/*
 * ftd_fsm_do_cpstart --
 * transition to checkpoint start.
 */
static int
ftd_fsm_do_cpstart(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	ftd_lg_cpstart(lgp);

	return trans->ft_next;  
}

/*
 * ftd_fsm_do_cpstop --
 * transition to checkpoint stop.
 */
static int
ftd_fsm_do_cpstop(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	ftd_lg_cpstop(lgp);

	return trans->ft_next;  
}

/*
 * ftd_fsm_do_cppend --
 * transition to checkpoint pending.
 */
static int
ftd_fsm_do_cppend(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	ftd_lg_cppend(lgp);

	return trans->ft_next;  
}

/*
 * ftd_fsm_do_cpon --
 * transition to checkpoint on.
 */
static int
ftd_fsm_do_cpon(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	ftd_lg_cpon(lgp);

	return trans->ft_next;  
}

/*
 * ftd_fsm_do_cpoff --
 * transition to checkpoint off.
 */
static int
ftd_fsm_do_cpoff(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{

	ftd_lg_cpoff(lgp);

	return trans->ft_next;  
}

/*
 * ftd_fsm_tracking --
 * holding pattern on primary until the secondary catches up
 */
static int
ftd_fsm_tracking_primary(ftd_lg_t *lgp) 
{
	int	rc, pstate;

	if (lgp->cfgp->role == ROLEPRIMARY) {

		// has hup packet made the round trip ?
		if (!GET_SOCK_HUP(FTD_SOCK_FLAGS(lgp->dsockp))) {
			rc = 0;
		} else {		
			UNSET_SOCK_HUP(FTD_SOCK_FLAGS(lgp->dsockp));
			
			// this sucks but we have to get the state
			// from the pstore since we don't know what
			// type of refresh we were in before TRACKING

			pstate = ftd_lg_get_pstore_run_state(lgp->lgnum, lgp->cfgp);

			rc = ftd_fsm_state_to_input(pstate);	
		}
	}

	return rc;
}

/*
 * ftd_fsm_refoflow --
 * holding pattern on primary unless we need to retry the refresh 
 */
static int
ftd_fsm_refoflow_primary(ftd_lg_t *lgp) 
{
	char	rc = 0;
	int		pstate, tryagain = FALSE;

	if (lgp->cfgp->role == ROLEPRIMARY) {
				
		// has hup packet made the round trip ?
		if (!GET_SOCK_HUP(FTD_SOCK_FLAGS(lgp->dsockp))) {
			return 0;
		}
					
		UNSET_SOCK_HUP(FTD_SOCK_FLAGS(lgp->dsockp));
					
		if (lgp->tunables->refrintrvl >= 0) {
			// we have overflowed the BAB and the user has
			// a finite refresh interval set
			{
				time_t			now;
							
				time(&now);
				lgp->timerefresh = now - lgp->refreshts;

				if (lgp->tunables->refrintrvl != 0 
					&& lgp->timerefresh < lgp->tunables->refrintrvl)
				{
					// we have not hit the refresh time threshold yet 
					// force another refresh try

					tryagain = TRUE;
				} else {
					lgp->refreshts = 0;
					
					SET_LG_RFTIMEO(lgp->flags);

                    //
                    // SVG 29-05-03 changed to critical from warn
                    //
					reporterr (ERRFAC, M_REFRTIMEO, ERRCRIT,
						lgp->tunables->refrintrvl);
				}
			}
		} else {

			// (refrintrvl < 0) - try forever
			tryagain = TRUE;
		}

		if (tryagain) {

			// this sucks but we have to get the state
			// from the pstore since we don't know what
			// type of refresh we were in before TRACKING

			pstate = ftd_lg_get_pstore_run_state(lgp->lgnum, lgp->cfgp);
			
			return ftd_fsm_state_to_input(pstate);	
		}
	}	

	return 0;
}

/*
 * ftd_fsm_do_report_bad_state --
 * report a bad state error msg and return current state.
 */
static int
ftd_fsm_do_report_bad_state(ftd_lg_t *lgp, ftd_fsm_trans_t *trans) 
{
	ftd_header_t	header;

	memset(&header, 0, sizeof(header));

	switch(trans->ft_char) {
	case FTD_CPASSTHRU:
		if (lgp->cfgp->role == ROLESECONDARY) {
			return trans->ft_next;
		}
		reporterr(ERRFAC, M_PASSTHRU, ERRWARN);
		break;
	case FTD_CNORMAL:
		if (lgp->cfgp->role == ROLESECONDARY) {
			return trans->ft_next;
		}
		if (trans->ft_state == FTD_SNORMAL && GET_LG_STARTED(lgp->flags) ) {
			reporterr(ERRFAC, M_PMDAGAIN, ERRWARN);
			header.msgtype = FTDACKCLD;
		}
		break;
	case FTD_CREFRESH:
	case FTD_CREFRESHC:
	case FTD_CREFRESHF:
		if (lgp->cfgp->role == ROLESECONDARY) {
			return trans->ft_next;
		}
		if (trans->ft_state == FTD_SREFRESH
			|| trans->ft_state == FTD_SREFRESHF
			|| trans->ft_state == FTD_SREFRESHC)
		{
			reporterr(ERRFAC, M_REFRAGAIN, ERRWARN);
		} else if (trans->ft_state == FTD_SBACKFRESH) {
			reporterr(ERRFAC, M_REFRBACK, ERRWARN);
		}
		header.msgtype = FTDACKCLD;
		break;
	case FTD_CBACKFRESH:
		if (lgp->cfgp->role == ROLESECONDARY) {
			return trans->ft_next;
		}
		if (trans->ft_state == FTD_SREFRESH
			|| trans->ft_state == FTD_SREFRESHF
			|| trans->ft_state == FTD_SREFRESHC)
		{
			reporterr(ERRFAC, M_BACKREFR, ERRWARN);
		} else if (trans->ft_state == FTD_SBACKFRESH) {
			reporterr(ERRFAC, M_BACKAGAIN, ERRWARN);
		}
		header.msgtype = FTDACKCLD;
		break;
	case FTD_CCPSTART:
		if (trans->ft_state != FTD_SNORMAL) {
			reporterr(ERRFAC, M_RSYNCCPON, ERRWARN, LG_PROCNAME(lgp));
			if (lgp->cfgp->role == ROLESECONDARY) {
				ftd_sock_send_err(lgp->dsockp, ftd_get_last_error(ERRFAC));
			}
		}
		UNSET_LG_CPSTART(lgp->flags);
		header.msgtype = FTDACKCLD;
		header.msg.lg.data = lgp->cfgp->role;
		break;
	case FTD_CCPSTOP:
		if (trans->ft_state != FTD_SNORMAL) {
			reporterr(ERRFAC, M_RSYNCCPOFF, ERRWARN, LG_PROCNAME(lgp));
			if (lgp->cfgp->role == ROLESECONDARY) {
				ftd_sock_send_err(lgp->dsockp, ftd_get_last_error(ERRFAC));
			}
		}
		UNSET_LG_CPSTOP(lgp->flags);
		header.msgtype = FTDCCPOFF;
		header.msg.lg.data = lgp->cfgp->role;
		break;
	default:
		break;
	}

	if (header.msgtype) {
		// tell master
		header.msg.lg.lgnum = lgp->lgnum;
		header.msg.lg.data = lgp->cfgp->role;

		FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_fsm_do_report_bad_state",lgp->isockp, &header);
	}

	return trans->ft_next;
}
