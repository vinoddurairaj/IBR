/*
 *  LZH-Light algorithm implementation v 1.0
 *  Copyright (C) Sergey Ignatchenko 1998
 *  This  software  is  provided  "as is"  without  express  or implied 
 *  warranty.
 *
 *  Permission to use, copy, modify, distribute and sell this  software 
 *  for any purpose is hereby  granted  without  fee,  subject  to  the 
 *  following restrictions:
 *  1. this notice may not be removed or altered;
 *  2. altered source versions must be plainly marked as such, and must 
 *     not be misrepresented as being the original software.
 *
 *  Tom Mills - 10/98 - converted from C++ to C
 */

#ifndef LZHLINTERNAL
#error This file is for LZHL internal use only
#endif

#include "_lzhl.h"

#define LZPOS UINT32
#define LZBUFMASK ( (LZBUFSIZE) - 1 )

#define LZTABLEINT UINT16
typedef LZTABLEINT lztableitem_t;

#define LZHASH UINT32

typedef struct lzbuffer_s {
    BYTE *buf;
    LZPOS bufPos;
} lzbuffer_t;


