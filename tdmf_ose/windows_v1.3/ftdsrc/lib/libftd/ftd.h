#ifndef _FTD_H_
#define _FTD_H_

#define ftd_ulong   unsigned long
#define ftd_ushort  unsigned short
#define ftd_uint    unsigned int

#define BLOCK_SIZE(a)  ( ((a >> DEV_BSHIFT) + 1) << DEV_BSHIFT )


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include "ftd_pathnames.h"

#if !defined(_WINDOWS)
#define Return(a) exit(a)
#else
#define Return(a) return(a)
#endif
 
/*
 *
 */
#if !defined(_WINDOWS)
#define FTD_MAX_DEVICES         256 
#define FTD_MAX_GROUPS         	64 
#define FTD_MAX_JRN_NUM         1000 
#define FTD_MAX_JRN_SIZE        100*1024*1024
#else
#define FTD_MAX_DEVICES         32
#define FTD_MAX_GROUPS          32
#define FTD_MAX_JRN_NUM         10000
#define FTD_MAX_JRN_SIZE        100*1024*1024
#endif

#define FTD_MAX_GRP_NUM         1000	
#define FTD_SERVER_PORT			575

#endif

