//
// Copyright (C) 1994 - 1999 Legato Systems, Inc.
// All rights reserved.
//
//
/****************************************************************************
*    octolic.h
*    PURPOSE: main header file for the octopus license key utilities
*
****************************************************************************/

/*
* $Log: octolic.h,v $
* Revision 1.1.1.1  2001/09/14 13:48:10  dturrin
* First import
*
* Revision 1.1.2.2  2000/04/17 20:23:51  jlosche
* Same, Same
*
 * 
 * 1     3/31/00 1:43p Jrl
* Revision 1.1.2.1  2000/03/30 17:15:26  kjute
* Added new oct lic lib
*
* Revision 2.6  1999/12/01 15:30:20  bobk
* remove DLL export defines
*
* Revision 2.5  1999/04/30 13:40:38  bobk
* update company name to Legato
*
* Revision 2.4  1998/06/27 15:10:00  bobk
* added defines from rpcsrv.idl
*
* Revision 2.3  1998/05/06 18:05:13  bobk
* move LICENSE_VERSION_OCTOPUS_3 define to product.h
*
* Revision 2.2  1997/10/09 14:17:01  bobk
* remove DllExport defines since the libraries are not DLLs anymore
*
* Revision 2.1.1.1  1997/10/01 20:07:20  bobk
* Octopus v3 Source
*
* Revision 1.4  1997/06/06 03:12:52  bobk
* make mod so it will compile on UNIX
*
* Revision 1.3  1997/05/28 15:20:12  bobk
* complete expanded feature bit changes
*
* Revision 1.2  1997/05/21 17:31:08  bobk
* added more feature bits and version bits
*
*
*
*/

#ifndef OCTOLIC_H_INCLUDED
#define OCTOLIC_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

/* smallest expiration period */
#define LICENSE_WEEKS	(1)
#define LICENSE_INTERVAL (60*60*24*LICENSE_WEEKS*7)


/* Not used in code.  Causes build error
const unsigned long LICENSE_ACCESS_CODE_LENGTH = 8;
const unsigned long LICENSE_LICENSE_KEY_LENGTH = 16;
*/

typedef struct { unsigned long flags[2]; } FEATURE_TYPE;


/*----------------------------------------------------------------------*/
/* checklicok -- given a license key and access code, check if it is ok to 
                 run (both valid, unexpired)
		 return codes are:
		 >0 = timestamp of when demo license expires (cast to time_t)
		 0 = permanent license
		 -1 = expired demo license
		 -2 = invalid access code
		 -3 = invalid license key
		 -4 = argument error (wrong size?)
		 */
long
checklicok (
	const char* licstring,
	const char* accesscode,
	unsigned long *wks_ret,
	FEATURE_TYPE *feature_flags_ret,
	unsigned long *version_ret);


/*-------------------------------------------------------------------------*/
/* genaccode -- given an integer sequence number, generate an access code 
                string (8 characters) */
void
genaccode (char* accode, int value);

/*-------------------------------------------------------------------------*/
/* genlic -- generates an Octopus license (deposits in licstring (char*8))
             when given (1) the number of weeks from now (wksfromnow) the
	     license should expire [set to 0 for permanent], a 
             feature mask (four active bits for enabling/disabling features)
	     and an access code. */
void
genlic(
	char* licstring,
	const char* accode,
	int wksfromnow,
	int version,
	FEATURE_TYPE feature_flags);


#ifdef __cplusplus
}
#endif


#endif
