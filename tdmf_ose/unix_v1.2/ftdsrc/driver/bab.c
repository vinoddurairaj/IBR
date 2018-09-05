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
/*
 * This file is generated automatically by ftd_bab_rebuild, but since
 * it is necessary for the HP build we include it in the CVS tree.
 *
 * $Id: bab.c,v 1.4 2010/12/20 20:17:19 dkodjo Exp $
 */
#include "ftd_kern_ctypes.h"
#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_bab.h"
#include "ftd_bits.h"
#include "ftd_var.h"
#include "ftd_all.h"
#include "ftd_klog.h"
#include "ftd_kern_cproto.h"

#if !defined(_AIX)
ftd_char_t big_ass_buffer[4 * 1024 * 1024];
ftd_int32_t big_ass_buffer_size = 4 * 1024 * 1024;
#endif /* !defined(_AIX) */
