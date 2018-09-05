/*
 * ftd_fsm.h - FTD logical group finite state machine  
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
#ifndef _FTD_FSM_H_
#define _FTD_FSM_H_

#include "ftd_lg.h"
#include "ftd_lg_states.h"

#define FTD_NSTATES		10				/* FTD number of states			*/

#define FTD_NCHRS		256				/* number of valid character	*/
#define FTD_CANY		(FTD_NCHRS+1)	/* match any input				*/

#define FTD_TINVALID	0xff

typedef struct ftd_fsm_trans_s {
	u_char	ft_state;					/* current state				*/
	short	ft_char;					/* input character				*/
	u_char	ft_next;					/* next state					*/
	int		(*ft_action)();				/* action to take				*/
} ftd_fsm_trans_t;


#define NTRANS (sizeof(ftd_ttstab)/sizeof(ftd_ttstab[0]))

extern ftd_fsm_trans_t ftd_ttstab[];
extern u_char ftd_ttfsm[FTD_NSTATES][FTD_NCHRS];

extern void ftd_fsm_init(u_char fsm[][FTD_NCHRS],
	ftd_fsm_trans_t ttab[], int nstates);
extern int ftd_fsm(ftd_lg_t *lgp, u_char fsm[][FTD_NCHRS],
	ftd_fsm_trans_t ttab[], u_char curstate, u_char curchar);
extern u_char ftd_fsm_state_to_input(int state);
extern int ftd_fsm_input_to_state(short inchar);


#endif

