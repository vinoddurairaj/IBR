/*
 * ftd_fsm_tab.h - FTD logical group state table  
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
#ifndef _FTD_FSM_TAB_H_
#define _FTD_FSM_TAB_H_

#include "ftd_rsync.h"
#include "ftd_lg_states.h"
#include "ftd_fsm.h"

extern ftd_fsm_trans_t ftd_ttstab[];
extern u_char ftd_ttfsm[FTD_NSTATES][FTD_NCHRS];

#endif
