/*========================================================================
 * licplat.c
 *
 * This file contains non-portable, OS & Hardware specific function calls
 * needed in support of the license key management library.
 *
 * Copyright (c) 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved.
 *========================================================================*/

/*=======================================================================*/
/*                           AIX PLATFORM SUPPORT                        */
/*=======================================================================*/
#if defined(_AIX)
#include <memory.h>
#include <sys/utsname.h>
long
my_gethostid(void)
{
        struct xutsname u;

        memset(&u, 0, sizeof(u));
        if (unamex(&u) < 0)
                return(-1L);
        return(u.nid);
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
long
my_gethostid(void)
{
  struct utsname uts;

  memset(uts.sysname, 0, sizeof(uts.sysname));
  if (uname(&uts) == -1) {
    return -1;
  }
  /* evidently, some systems may not have an id number */
  if (strlen(uts.__idnumber) == 0) {
    return -1;
  }
  return (atol(uts.__idnumber));
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
long
my_gethostid(void)
{
  int   command = SI_HW_SERIAL;
  char  buf[258];	/* man page suggest 257 + 1 */
  long  hostid;
  
  if (sysinfo(command, buf, 257) == -1)
    return (long) -1;
  if (sscanf(buf, "%ld", &hostid) != 1)
    return (long) -1;
  return hostid;
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

/*-----------------------------------------------------------------------*/
/* end of file                                                           */
/*-----------------------------------------------------------------------*/







