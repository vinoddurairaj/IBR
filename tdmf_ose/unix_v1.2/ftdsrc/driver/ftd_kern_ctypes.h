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
#if !defined(FTD_KERN_CTYPES_H)
#define FTD_KERN_CTYPES_H
#if defined(AIX)
#include <sys/types.h>
#endif

#if defined(HPUX)
#include <sys/conf.h>
#include <sys/stat.h>
#endif

#if defined(linux)
#include <linux/types.h>
#endif

/*-
 * resolve c intrinsic types from ftd_<abstract_type>_t declarations
 * 
 * the bulk of the ftd kernel code is now written without reference 
 * to any intrinsic C type. 
 * this has been done to simplify porting from IP32 to LP64 models
 * (ftd was developed for IP32 model environments.).
 */

#if defined(SOLARIS) 
/* to discover and configure for OS vers dki variations */
typedef enum ftd_solvers_e {
  SOL_VERS_UNKN=0,
  SOL_VERS_550=550,
  SOL_VERS_560=560,
  SOL_VERS_570=570,
  SOL_VERS_580=580,
  SOL_VERS_590=590,
  SOL_VERS_5100=5100
} ftd_solvers_t;
extern ftd_solvers_t ftd_solvers;
#endif 

#define ftd_void_t 		void
#define ftd_char_t 		char
#define ftd_uchar_t 	unsigned char
#define ftd_int16_t 	short
#define ftd_uint16_t 	unsigned short
#define ftd_int32_t  	int
#define ftd_uint32_t 	unsigned int
#define ftd_context_t 	unsigned long

#define ftd_int64_t  	long long
#define ftd_uint64_t 	unsigned long long

#ifdef SOLARIS
	#if (SYSVERS < 560) 
		#define ftd_intptr_t 	int
	#else
		#define ftd_intptr_t    intptr_t
	#endif
#else
	#if defined(linux)
		#ifndef __intptr_t_defined
		#define __intptr_t_defined
		typedef unsigned long intptr_t;
		#endif
	#endif

	#define ftd_intptr_t    intptr_t
#endif


#define ftd_float_t  	float
#define ftd_double_t 	double

/*-
 * some conversions to quiet lint(1).
 * to quiet:
 * `passing (i)-bit unsigned integer arg, expecting (i)-bit signed integer'
 * use:
 */
#define U_2_S32(i) \
(((i) & 0x80000000) ? panic("DEBUG: U_2_S32"),0 : *((ftd_int32_t *)&(i)))

#define U_2_S64(i) \
(((i) & 0x8000000000000000) ? panic("DEBUG: U_2_S64"),0 : *((ftd_int64_t *)&(i)))


/*-
 * Solaris user/kernel translation types 
 *
 * all types referenced in our structure definitions, 
 * and passed between user level (32-bit), and kernel 
 * level (either 32-bit or 64-bit) must be made fixed-width.
 * 
 * this is done to minimize the complexity of converting 
 * structures passed between code compiled with different
 * ABIs.
 *
 * these containers are necessarily of `largest common size'.
 *
 * take, for example, the type dev_t. this is a 32-bit 
 * type in sparcv8 kernels, and the common user level 
 * code, but is a 64-bit type in the sparcv9 kernel. 
 * 
 * to make the user level code interoperable with either
 * kernel, user level code exchanging a structure with a 
 * dev_t member must be provided with a container large 
 * enough for this type as defined in either kernel ABI.
 * 
 * dev_t, further, must be converted between internal and
 * external representations. when the type is passed to
 * the kernel, it will make the conversion at the time the
 * type is copied in or out, hiding it's ABI from all user
 * level code.
 */
#if defined(SOLARIS) || defined(_AIX)

/* consistent size time_t */
typedef ftd_uint64_t ftd_time_t;

/* consistent size dev_t */
typedef ftd_uint64_t ftd_dev_t_t;

/* consistent size timeout_id_t */
typedef ftd_uint64_t ftd_timeout_id_t;

#else  /* defined(Solaris) || defined(_AIX) */

/* consistent size time_t */
typedef time_t ftd_time_t;

/* consistent size dev_t */
#if defined(linux)
typedef ftd_uint64_t ftd_dev_t_t;
#else
typedef dev_t ftd_dev_t_t;
#endif /* defined(linux) */

/* consistent size timeout_id_t */
#if defined(TWS)
typedef timeout_id_t ftd_timeout_id_t;
#else
typedef ftd_uint64_t ftd_timeout_id_t;
#endif

#endif /* defined(Solaris) || defined(_AIX) */

/* 
 * consistent size of kernel pointer/scalar type 
 * N.B.: note that this replaces kernel pointer 
 * types with a scalar type, e.g.:
 * 
 * dev_t *some_dev_t_ptr;
 *  becomes:
 * ftd_uint64ptr_t some_dev_t_ptr;
 *
 * see that `*' gets lost in the declaration.
 * usage of such fields requires care...
 */
typedef ftd_uint64_t ftd_uint64ptr_t;

#if defined(SOLARIS) && defined(__sparcv9)
#define DBAR_D_SHFT(a) ((ftd_uint64ptr_t) (a) >> 32)
#else /* defined(SOLARIS) && defined(__sparcv9) */
#define  DBAR_D_SHFT(a) (a)
#endif /* defined(SOLARIS) && defined(__sparcv9) */

#endif /* !defined(FTD_KERN_CTYPES_H) */
