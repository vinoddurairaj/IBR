/*
 * ftd_cpoint.h - FTD logical group checkpoint interface
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
#ifndef _FTD_CPOINT_H_
#define _FTD_CPOINT_H_

// checkpoint script path prefixes
#define PRE_CP_ON		"cp_pre_on_" 
#define PRE_CP_OFF		"cp_pre_off_" 
#define POST_CP_ON		"cp_post_on_" 
 
extern int ftd_lg_cpstart(ftd_lg_t *lgp);
extern int ftd_lg_cpon(ftd_lg_t *lgp);
extern int ftd_lg_cpoff(ftd_lg_t *lgp);
extern int ftd_lg_cpstop(ftd_lg_t *lgp);
extern int ftd_lg_cppend(ftd_lg_t *lgp);

BOOL existCommands(char *fileName);

#endif
