/*
 * Copyright (c) 1997, 1998 FullTime Software, Inc. All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef SOLARIS
#include <sys/systeminfo.h>
#endif

#include "license.h"

#define STR_MAX 80

/*-------------------------------------------------------------------------*/
void 
parse_args (int argc, char** argv, int* pmdflag, int* rmdflag, 
	    int* daysgood, license_data_t* lic_in)
{
  int i;

  i=0;
  while (++i < argc) {
    if (argv[i] == (char*)NULL) break;
    if (strcmp (argv[i], "-p") == 0) {
      *pmdflag = 1;
    } else if (strcmp (argv[i], "-r") == 0) {
      *rmdflag = 1;
    } else if (strcmp (argv[i], "-d") == 0) {
      *daysgood = atoi(argv[++i]);
      if (*daysgood < 1) *daysgood = 1;
      if (*daysgood > 90) *daysgood = 90;
    } else if (strcmp (argv[i], "-i") == 0) {
      sscanf (argv[++i], "%lx", &(lic_in->hostid));
    } else {
      printf ("Usage:  %s \\ \n", argv[0]);
      printf ("              [-p]             # PMD license\n");
      printf ("              [-r]             # RMD license\n");
      printf ("              [-i <hostid>]    # for this hostid\n");
      printf ("              [-d <days_good>] # for N (1-90) days good\n");
      exit (1);
    }
  }
}
 
/*-------------------------------------------------------------------------*/
int 
main (int argc, char** argv) 
{
  int  pmdflag = 0;
  int  rmdflag = 0;
  license_data_t  lic_in;
  license_data_t  lic_crypt;
  struct tm timebomb;
  time_t nowtime;
  time_t bombtime;
  char str[STR_MAX];
  int daysgood;
  int i;
  
  timebomb.tm_sec = 0;
  timebomb.tm_min = 0;
  timebomb.tm_hour = 0;
  timebomb.tm_mday = 1;
  timebomb.tm_mon = 9 - 1;
  timebomb.tm_year = 98;
  timebomb.tm_wday = 0;
  timebomb.tm_yday = 0;
  timebomb.tm_isdst = 1;

  /* -- check for generator expiration */
  bombtime = mktime (&timebomb);
  nowtime = time ((time_t)NULL);

  if (nowtime > bombtime) {
    fprintf (stderr, "\n\nThis license key generator has expired.\n");
    exit (1);
  }

  lic_in.expiration_date = 0;
  lic_in.checksum = 0;
  lic_in.license_expires = 1;
  lic_in.license_level = 0;
  lic_in.machine_model_checksum = 0;
  lic_in.customer_name_checksum = 0;
  lic_in.hostid = 0;

  /* -- check for interactive or command line mode */
  if (argc > 1) {
    lic_in.hostid = 0;
    daysgood = 14;
    parse_args(argc, argv, &pmdflag, &rmdflag, &daysgood, &lic_in);
  } else {
    pmdflag = 1;
    rmdflag = 1;
    lic_in.hostid = 0;
    daysgood = 14;
  }
  lic_in.expiration_date = 
    (short) ((int) (nowtime + (daysgood * 86400)) >> 16);
  lic_in.license_expires = 1;
  /* -- create a PMD license key */
  if (pmdflag) {
    np_encrypt((unsigned char *) &lic_in, (unsigned char *) &lic_crypt,
	       sizeof(license_data_t), np_crypt_key_pmd, NP_CRYPT_KEY_LEN);
    for (i = 0; i < STR_MAX; i++)
      str[i] = '\0';
    char_encode((unsigned char *) &lic_crypt, (unsigned char *) str,
		sizeof(license_data_t));
    fprintf (stdout, "PMD_LICENSE: %s\n", str);
  }
  /* -- create a RMD license key */
  if (rmdflag) {
    np_encrypt((unsigned char *) &lic_in, (unsigned char *) &lic_crypt,
	       sizeof(license_data_t), np_crypt_key_rmd, NP_CRYPT_KEY_LEN);
    for (i = 0; i < STR_MAX; i++)
      str[i] = '\0';
    char_encode((unsigned char *) &lic_crypt, (unsigned char *) str,
		sizeof(license_data_t));
    fprintf (stdout, "RMD_LICENSE: %s\n", str);
  }
  fflush (stdout);
}
