/*
 * ftd_port.h - FTD portability includes
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

#ifndef _FTD_PORT_H
#define _FTD_PORT_H

//
// ++ SVG 01-06-03
//
// This function determines level of priority of threads
// 
// because this include file is shared by the utilities that
// use are able to change priority
//
#define _HIGHER_PRIORITY_

//
// This defines are used so we can easily find conditional 
// messages that appear during compile
//
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning Msg: "
#define __LOC2__ __FILE__ "("__STR1__(__LINE__)") : "

// --

#if defined(_WINDOWS)
#   include "ftd_port_win.h"
#else
#	include "ftd_port_unix.h"
#endif

#include "ftd.h"

#endif /* _FTD_PORT_H */
