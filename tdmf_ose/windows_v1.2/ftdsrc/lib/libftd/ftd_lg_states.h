/*
 * ftd_lg_states.h - FTD logical group states
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

#ifndef _FTD_LG_STATES_H
#define _FTD_LG_STATES_H

#define FTD_CINVALID	255				/* FTD invalid input			*/

#define	FTD_CPASSTHRU	1				/* FTD passthru cmd				*/ 
#define FTD_CNORMAL		2				/* FTD normal cmd				*/
#define FTD_CREFRESH	3				/* FTD refresh cmd				*/
#define FTD_CREFRESHF	4				/* FTD refresh full cmd			*/
#define FTD_CREFRESHC	5				/* FTD refresh check cmd		*/
#define FTD_CBACKFRESH	6				/* FTD backfresh cmd			*/
#define FTD_CTRACKING	7				/* FTD tracking cmd				*/
#define FTD_CCPON		8				/* FTD checkpoint on cmd		*/
#define FTD_CCPOFF		9				/* FTD checkpoint off cmd		*/
#define FTD_CSTARTJRNL	10				/* FTD start jrnling cmd		*/
#define FTD_CSTOPJRNL	11				/* FTD stop jrnling cmd			*/
#define FTD_CCPSTART	12				/* FTD checkpoint start cmd		*/
#define FTD_CCPSTOP		14				/* FTD checkpoint stop cmd		*/
#define FTD_CCPPEND		15				/* FTD checkpoint pend cmd		*/
#define FTD_CREFTIMEO	16				/* FTD refresh timeout			*/

#define	FTD_SPASSTHRU	0				/* FTD passthru					*/ 
#define FTD_SNORMAL		1				/* FTD normal					*/
#define FTD_SREFRESH	2				/* FTD refresh					*/
#define FTD_SREFRESHF	3				/* FTD refresh full				*/
#define FTD_SREFRESHC	4				/* FTD refresh check			*/
#define FTD_SBACKFRESH	5				/* FTD backfresh				*/
#define FTD_SREFOFLOW	6				/* FTD refresh overflow			*/
#define FTD_STRACKING	7				/* FTD tracking					*/
#define FTD_SAPPLY		8				/* FTD journal apply state		*/
#define FTD_SINVALID	9				/* FTD invalid state			*/

#define FTD_SBACKFRESHF	10				/* FTD backfresh - force		*/

#endif

