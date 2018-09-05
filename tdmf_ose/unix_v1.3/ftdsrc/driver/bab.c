/*
 * This file is generated automatically by ftd_bab_rebuild, but since
 * it is necessary for the HP build we include it in the CVS tree.
 *
 * $Id: bab.c,v 1.3 2001/01/17 17:58:55 hutch Exp $
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
