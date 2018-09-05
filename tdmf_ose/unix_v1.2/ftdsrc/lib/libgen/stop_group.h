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
#ifndef _STOP_GROUP_H_ 
#define _STOP_GROUP_H_ 

/*
 * stop_group.h 
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
int ftd_stop_group(char *ps_name, int group, ps_version_1_attr_t *attr, int autostart_flag, int clear_ODM);

#if defined( _AIX )
int   update_ODM_entry( int group, int clear_ODM );
void  update_all_ODM_entries( int clear_ODM );
#endif

#endif /* _STOP_GROUP_H_ */
