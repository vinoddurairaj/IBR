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
/* #ident "@(#)$Id: licplat.c,v 1.15 2012/02/14 21:45:42 paulclou Exp $" */
/*
 * Copyright (C) Softek Storage Solutions, Corp. 2002-2005.
 * All Rights Reserved.
 */

 /*========================================================================*/

/* Common routine to support AIX LPAR, HP-UX vPar and concurrent Agents
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pathnames.h"
#include "licplat.h"

/* Maximum LPAR ID CAP for LPAR/VPAR node number.  I.e., valid numbers are
 * 0, ..., LPARID_CAP-1.
 *
 * It is a simple protection preventing customer using the 
 * LPARID configuration file to work agaist our license protection.
 * 
 * AIX:  unamex valuse is NOT the same as "hostid" (which returns IP) command
 * HPUX: uname returns NON-IP address
 * Solaris: the serial number returned is the IP address used.
 */

#define LPARID_CAP 512

static int
my_getuniqueid(long *unique_id)
{
	char buffer[256];
	unsigned long uid = 0;
	char *cp, *ecp;
	FILE *ufp;
	if ((ufp = fopen( UNIQUE_LPARID_FILE, "r" )) == NULL) {
		return 1;
	}
	cp = fgets( buffer, sizeof buffer, ufp );
	fclose(ufp);
	if (cp == NULL || strlen(cp) <= 0 || 
            (uid = strtoul( cp, &ecp, 0)) <= 0 || *ecp  != '\n') {
		return 1;
	}
	*unique_id = uid % LPARID_CAP;
	return 0;
}

/*=======================================================================*/
/*                        TDMF UNIX USES THIS CODE                       */
/*=======================================================================*/
#ifdef TDMFUNIXGENUS
// This code is borrowed by TDMF UNIX (not for Replicator / TDMF IP)
int
my_gethostid(void)
{
    static long my_hostid = 0;
    long hostid;

    if (my_hostid == 0)
    {
	    my_hostid = gethostid();
	    if (my_getuniqueid(&hostid) == 0)
			my_hostid += hostid;
    }
    return my_hostid;
}
#endif



/*=======================================================================*/
/*                           REPLICATOR SECTION                          */
/*=======================================================================*/
#ifndef TDMFUNIXGENUS

/*=======================================================================*/
/*                           AIX PLATFORM SUPPORT                        */
/*=======================================================================*/
#if defined(_AIX)
#include <memory.h>
#include <sys/utsname.h>
int
my_gethostid(hostid_purpose_t purpose)
{
    long my_hostid = 0;
    struct xutsname u;

    memset(&u, 0, sizeof(u));
    if (unamex(&u) < 0)
        return(-1L);

    if (purpose == HOSTID_LICENSE || my_getuniqueid(&my_hostid)) {

        /* no dtclparid.cfg existed
         * use the existing uname identification only
         */
        my_hostid = u.nid;
    } else {
        /* dtclparid.cfg existed.
         * Add to the existing uname identification only.
         * Avoid easy license violation.
         */
        my_hostid += u.nid;
    }

    return my_hostid;
}

/*-----------------------------------------------------------------------*/
char*
my_get_machine_model(void)
{
  static struct utsname uts;

  memset(uts.sysname, 0, sizeof(uts.sysname));
  if (uname(&uts) == -1) {
    return ((char *) NULL);
  }
  return (uts.machine);
}

#endif /* defined(_AIX) */

/*=======================================================================*/
/*                           HPUX PLATFORM SUPPORT                       */
/*=======================================================================*/
#ifdef HPUX
#include <unistd.h>
#include <sys/utsname.h>
int
my_gethostid(hostid_purpose_t purpose)
{
    long my_hostid = 0;
    struct utsname uts;

    memset(uts.sysname, 0, sizeof(uts.sysname));
    if (uname(&uts) == -1) 
        return -1;
   
    if (purpose == HOSTID_LICENSE || my_getuniqueid(&my_hostid)) {
        /* no dtclparid.cfg existed
         * use the existing uname identification only
         */

        my_hostid = atol(uts.__idnumber);
    } else {
        /* dtclparid.cfg existed.
         * Add to the existing uname identification only.
         * Avoid easy license violation.
         */

        my_hostid += atol(uts.__idnumber);
    }

  return my_hostid;
}

/*-----------------------------------------------------------------------*/
char*
my_get_machine_model(void)
{
  static struct utsname uts;

  memset(uts.sysname, 0, sizeof(uts.sysname));
  if (uname(&uts) == -1) {
    return ((char *) NULL);
  }
  return (uts.machine);
}

#endif /* HPUX */
/*=======================================================================*/
/*                      SOLARIS 2.4/2.5/2.6 PLATFORM SUPPORT                 */
/*=======================================================================*/
#ifdef SOLARIS
/*
 * 2.4 has gethostid() in ucblib 2.5 has gethostid() is libc If we
 * dynamically link our software on 2.5, it will not be able to run under 2.4
 * because ucblib is not searched.  So, we get this information from the
 * system serial number, which is the same under 2.4 and 2.5 and where
 * gethostid gets its information from anyways
 */
#include <stdio.h>
#include <sys/systeminfo.h>
int
my_gethostid(hostid_purpose_t purpose)
{
    long  my_hostid = 0;
    long  hostid;
 
    int   command = SI_HW_SERIAL;
    char  buf[258];	/* man page suggest 257 + 1 */

    if (sysinfo(command, buf, 257) == -1)
        return (long) -1;
    if (sscanf(buf, "%ld", &hostid) != 1)
        return (long) -1;
    my_hostid = hostid;
    /* see if we need adjust the hostid value */
    if (purpose == HOSTID_IDENTIFICATION && my_getuniqueid(&hostid) == 0)
        my_hostid += hostid;

    return my_hostid;
}

/*-----------------------------------------------------------------------*/
/*
 * for Solaris 2.5 and up this is `uname -i`
 */
char           *
my_get_machine_model(void)
{
  int             command = SI_PLATFORM;
  static char     buf[258];	/* man page suggest 257 + 1 */
  
  if (sysinfo(command, buf, 257) == -1)
    return (char *) NULL;
  return &(buf[0]);
}
#endif /* SOLARIS */

/*====================================================================
		LINUX VERSION 
//===================================================================*/

#if defined(linux)

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <sys/utsname.h>

//IP number of linux system.
int
my_gethostid(hostid_purpose_t purpose)
{
    long  my_hostid = 0;
    long hostid;
    char buffer[16];
    int i;																   
    char new_hostid[8];
  
    my_hostid = gethostid();  	
    if (purpose == HOSTID_IDENTIFICATION && my_getuniqueid(&hostid) == 0)
        my_hostid += hostid;

    return my_hostid;
}

// architecture name.

char*
my_get_machine_model(void)
{
	static struct utsname uts;

	memset(uts.sysname, 0, sizeof(uts.sysname));
	if (uname(&uts) == -1) {
		return ((char *) NULL);
	}
	return (uts.machine);
}

#endif /* defined(linux) */
#endif /* REPLICATOR SECTION (versus TDMF UNIX) */

/*-----------------------------------------------------------------------*/
/* end of file                                                           */
/*-----------------------------------------------------------------------*/
