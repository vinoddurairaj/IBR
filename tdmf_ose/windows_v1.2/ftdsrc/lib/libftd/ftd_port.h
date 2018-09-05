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

#if defined(_WINDOWS)
#   include "ftd_port_win.h"
#else
#	include "ftd_port_unix.h"
#endif

#include "ftd.h"

#endif /* _FTD_PORT_H */
