
/*===========================================================================
 * createkey.c
 *
 * Copyright (c) 1999 Legato Systems, Inc. All Rights Reserved.
 *
 * To safeguard against this program being copied, I am checking
 * for hostid at the start to make sure that the current hostid
 * is matched with an embedded hostid. Therefore, each machine allowed
 * to run this program must have its own copy of this program.
 * To change the embedded hostid value, see the array allowed_hostid[]
 *
 * This program will also run on systems that are not in the allowed_hostid
 * list, but the program will disable after a compiled-in date.
 *
 ============================================================================*/

#include "ftd_port.h"

#include "license.h"

#define STR_MAX 80

static char* argv0;
/* expires October 1, 1999 */
static int exp_mon = 10;
static int exp_day = 1;
static int exp_year = 1999;
static int permkeyokflag=0;

extern volatile char qdsreleasenumber[];

static void
do_usage()
{
  fprintf(stderr, "\nUsage: %s\n", argv0);
  fprintf(stderr, "            for prompted execution, or\n");
  fprintf(stderr, "       %s -h \n", argv0);
  fprintf(stderr, "            for help, or\n");
  fprintf(stderr, "       %s [-p | -d <days>] [-i <hostid> e.g. 0x80712345]\n", argv0);
  fprintf(stderr, "            for command line execution.\n\n");
}

/***********************************************************************/
int clean_input (char* str)
{
  int len;
  int i, j;

  j=0;
  len = strlen(str);
  for (i=0; i<len; i++) {
    if (0 == isspace(str[i])) 
      str[j++] = str[i];
  }
  str[j] = '\0';
  return(j);
}

/***********************************************************************/
time_t parse_date (char* str)
{
  int len, i, year, mon, day;
  time_t now;
  time_t expires;
  struct tm tm;
  
  i = 0;
  year = 0;
  mon = 0;
  day = 0;
  
  tm.tm_sec = 00;
  tm.tm_min = 00;
  tm.tm_hour = 13;
  tm.tm_mday = 0;
  tm.tm_mon = 0;
  tm.tm_year = 0;
  tm.tm_wday = 0;
  tm.tm_yday = 0;
  tm.tm_isdst = 0;

  len = clean_input(str);
  if (len != 8) 
    return ((time_t)0);
  
  for (i=0; i<len; i++) {
    if (0 == isdigit(str[i])) 
      return ((time_t)0);
  }
  year += (1000 * (str[0] - '0'));
  year += (100 * (str[1] - '0'));
  year += (10 * (str[2] - '0'));
  year += (str[3] - '0');
  tm.tm_year = year - 1900;
  if (year < 2003 || year > 2100)
    return ((time_t)0);
  mon += (10 * (str[4] - '0'));
  mon += (str[5] - '0');
  tm.tm_mon = mon-1;
  if (mon > 12)
    return((time_t)0);
  day += (10 * (str[6] - '0'));
  day += (str[7] - '0');
  tm.tm_mday = day;
  if (day > 31)
    return((time_t)0);
  
  expires = mktime(&tm);
  (void) time(&now);
  if (expires < now)
    return ((time_t)0);
  expires = ((expires >> 16) << 16);
  return(expires);
} 

/***********************************************************************/
time_t parse_days (char* str)
{

  int len, i;
  time_t now;
  time_t expires;
  int days;
  struct tm current;
  
  len = clean_input(str);
  if (len > 3) 
    return ((time_t)0);

  days = 0;
  for (i=0; i<len; i++) {
    if (0 == isdigit(str[i])) 
      return ((time_t)0);
    days = ((days * 10) + (str[i] - '0'));
  }

  (void) time(&now);

  /* Force current hour to 13:00:00 */
  current = *localtime( &now );  
  current.tm_sec = 00;
  current.tm_min = 00;
  current.tm_hour = 13;
  now = mktime(&current);

  /* Evaluate expires time using current time fixed to 13:00:00 */
  /* plus nbr of days times (24*60*60) , secs/day               */
  expires = now + (days * 86400);
  expires = ((expires >> 16) << 16);
  return(expires);
}
 
/***********************************************************************/
unsigned long parse_hostid (char* str)
{

  int len, i;
  unsigned long hstid;
  unsigned long tempid;
  int val;

  hstid = tempid = 0;
  len = clean_input(str);


  if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
	if (len !=10) 
		return (hstid);	
	 i = 2;
  } else {
	if (len !=8) 
		return (hstid);	
	i = 0;
  }

  for (; i<len; i++) {
    if (0 == isxdigit(str[i])) 
      return (hstid);
    if (isdigit(str[i])) {
      val = str[i] - '0';
    } else {
      if (isupper(str[i]))
	  {
	val = 10 + (str[i] - 'A');
      } else {
	val = 10 + (str[i] - 'a');
      }
    }
    tempid = (tempid * 16) + val;
  }
  hstid = tempid;
  return (hstid);
} 

/***********************************************************************/
static int
prompt_input (int* permflag, time_t* expires, long* host_id, char* custname)
{
  int ok;
  char buffer[BUFSIZ+1];
  char* ptr;
  time_t now;
  time_t defexpires;
  time_t rettime;
  
  (void) time(&now);
  defexpires = now + (86400 * 90);

  /* customer name */
  ok = 0;
  while (!ok) {
    buffer[0] = '\0';
    fprintf (stdout, "Give Customer Name:  ");
    fflush (stdout);
    if (fgets(buffer, BUFSIZ, stdin) == (char *) NULL) continue;
    if ((ptr = strchr(buffer, '\n')) != (char *) NULL)
      *ptr = '\0';
    strcpy (custname, buffer);
    ok = 1;
  }

  /* permanent or non-permanent license key */
  ok = 0;
  while (!ok) {
    *permflag = 0;
    buffer[0] = '\0';
    fprintf (stdout, "Generate a permanent license?  [N]  ");
    fflush (stdout);
    if (fgets(buffer, BUFSIZ, stdin) == (char*)NULL) continue;
    if ((ptr = strchr(buffer, '\n')) != (char *) NULL)
      *ptr = '\0';
    if (strlen(buffer) == 0) {
      break;
    }
    if (buffer[0] == 'n' || buffer[0] == 'N') {
      break;
    }
    if (buffer[0] == 'y' || buffer[0] == 'Y') {
      if (!permkeyokflag) {
	fprintf (stderr, "ERROR [%s]: This system is not authorized to create permanent license keys\n", argv0);
	exit (1);
      }
      *permflag = 1;
      ok = 1;
      continue;
    }
    fprintf (stdout, "  *** answer \'y\' or \'n\' please ***\n");
    fflush (stdout);
  }
  /* expiration date or days license is valid from now for nonperm key */
  if (!(*permflag)) {
    ok = 0;
    while (!ok) {
      *expires = defexpires;
      buffer[0] = '\0';
      fprintf (stdout, "Expiration date or number of day(s) license [YYYYMMDD or N (1-999)]\n");
      fprintf (stdout, "(Default is 90 days from now): ");
      fflush (stdout);
      if (fgets(buffer, BUFSIZ, stdin) == (char*)NULL) continue;
      if ((ptr = strchr(buffer, '\n')) != (char *) NULL)
	*ptr = '\0';
      if (strlen(buffer) == 0) {
	break;
      }
      if (0 == (rettime = parse_date(buffer))) {
	if (0 == (rettime = parse_days(buffer))) {
	  continue;
	}
      }
      *expires = rettime;
      ok = 1;
    }
  }

  /* hostid to lock the license to */
  ok = 0;
  while (!ok) {
    *host_id = 0;
    buffer[0] = '\0';
    fprintf (stdout, "Hostid which to lock the license key to  (e.g. 0x80712345)\n");
    fprintf (stdout, "or enter \"ANY\" for any host:  ");
    fflush (stdout);
    if (fgets(buffer, BUFSIZ, stdin) == (char*)NULL) continue;
    if ((ptr = strchr(buffer, '\n')) != (char *) NULL)
      *ptr = '\0';
    if (strlen(buffer) == 0) 
      continue;
    if (0 == strcmp(buffer, "any") || 0 == strcmp(buffer, "ANY")) 
      break;
    *host_id = parse_hostid(buffer);
    if (*host_id != 0)
      break;
  }
  return (0);
}            

/***********************************************************************/
static void
genlickey (char* lickey, int* permflag, time_t* expires, long* host_id, 
           char* custname)
{
    time_t ts;
    license_data_t lic_out;
    license_data_t lic_crypt;
    license_checksum_t rv;

    memset ((void*)&lic_out, 0, sizeof(license_data_t));
    memset ((void*)&lic_crypt, 0, sizeof(license_data_t));
    memset ((void*)&rv, 0, sizeof(license_data_t));
    (void) time (&ts);
    lic_out.checksum = 0;
    lic_out.customer_name_checksum = 0;
    if (*permflag) 
	{
        lic_out.expiration_date = 0;
        lic_out.license_expires = 0;
        lic_out.hostid = *host_id;
    } 
	else 
	{
        lic_out.expiration_date = (short) ((int) (*expires >> 16));
        lic_out.license_expires = 1;
        lic_out.hostid = *host_id;
    }
    rv = x_license_calculate_checksum(&lic_out);
    lic_out.checksum = rv.checksum;
    np_encrypt((unsigned char *) &lic_out, (unsigned char *) &lic_crypt,
               sizeof(license_data_t), np_crypt_key_pmd, NP_CRYPT_KEY_LEN);
    char_encode((unsigned char *) &lic_crypt, (unsigned char *) lickey,
                sizeof(license_data_t));
}

/***********************************************************************/
static void
print_cgi_lickey (char* lickey)
{
  fprintf (stdout, "%s_LICENSE: %s\n", CAPQ, lickey);
  fflush (stdout);
}

/***********************************************************************/
static void
print_prompted_lickey (char* lickey, int permflag, time_t expires, 
		       long host_id, char* custname)
{
  time_t ts;
//  time_t yon;
//  struct tm *timp;
  FILE* fd;
  char *ptr;
  char buf[256];
  
  fd = fopen ("TDMFOSE-licensekeys.log", "a+");
  fprintf (stdout, "==================================================\n");
  fprintf (fd,     "==================================================\n");
  (void) time(&ts);
  strcpy (buf, ctime(&ts));
  if ((ptr = strchr(buf, '\n')) != (char *) NULL)
    *ptr = '\0';
  fprintf (stdout, "Softek TDMF Open License generated:  %s\n", 
	   buf);
  fprintf (fd    , "Softek TDMF Open License generated:  %s\n", 
	   buf);
  fprintf (stdout, "Customer:  %s\n", custname);
  fprintf (fd,     "Customer:  %s\n", custname);
  if (!permflag) {
    if (expires == 0) {
      fprintf(stderr, "error:  demo license with no expiration\n");
      exit(-1);
    }
  }
  if (!permflag) {
    strcpy(buf, asctime(localtime(&expires)));
    if ((ptr = strchr(buf, '\n')) != (char *) NULL)
      *ptr = '\0';
    fprintf (stdout, 
	     "NON-PERMANENT LICENSE -- Expires:  %s\n", buf);
    fprintf (fd, 
	     "NON-PERMANENT LICENSE -- Expires:  %s\n", buf);
  } else {
    fprintf (stdout, "PERMANENT LICENSE\n");
    fprintf (fd    , "PERMANENT LICENSE\n");
  }
  if (host_id) {
    fprintf (stdout, "License locked to host ID:  0x%08x\n", host_id);
    fprintf (fd,     "License locked to host ID:  0x%08x\n", host_id);
  } else {
    fprintf (stdout, "License not locked to specific host ID\n");
    fprintf (fd,     "License not locked to specific host ID\n");
  }
  fprintf (stdout, "%s_LICENSE: %s\n", CAPQ, lickey);
  fprintf (fd,     "%s_LICENSE: %s\n", CAPQ, lickey);
  fprintf (stdout, "==================================================\n\n");
  fprintf (fd,     "==================================================\n\n");
  fflush (stdout);
  fflush (fd);  
  fclose (fd);
}

/***********************************************************************/
int
main(int argc, char **argv)
{
//    int i, j;
    char buffer[BUFSIZ + 1];
    time_t nowtime;
    time_t exptime;
    long myhostid;
    int oktorun;
    struct tm timp;
    char custname[BUFSIZ+1];
    int permflag;
    long host_id;
    int demodays;
    char lickey[80];
    time_t expires;
    char ch;
    
    permkeyokflag = 1;
    
    argv0 = argv[0];
    oktorun = 0;
    (void) time (&nowtime);
    custname[0] = '\0';
    permflag = 0;
    host_id = 0x00000000;
    demodays = 90;
    expires = 0;

    /* -- expiration timestamp stuff */
    timp.tm_sec = 59;
    timp.tm_min = 59;
    timp.tm_hour = 23;
    timp.tm_year = exp_year - 1900;
    timp.tm_mon = exp_mon - 1;
    timp.tm_mday = exp_day;
    timp.tm_wday = 0;
    timp.tm_yday = 0;
    timp.tm_isdst = 0;
    exptime = mktime(&timp);

    /* -- validate that this is an ok system to run this program */
    myhostid = my_gethostid();


    oktorun = 1;
    permkeyokflag = 1;

    /* -- check for time limited availability */
    if (!oktorun) {
      if (nowtime < exptime) 
	oktorun = 1;
    }

    /* -- cancel run if not authorized */
    if (!oktorun) {
      fprintf (stderr, "You may not run %s from this system, expired.\n",
	       argv0);
      goto errexit;
    }
    custname[0] = '\0';
    permflag = 0;
    host_id = 0x00000000;
    demodays = 90;

    if (argc == 1) /* Capture argument values */
	{
		if (0 != prompt_input (&permflag, &expires, &host_id, custname)) 
		{
			fprintf (stderr, "Input error -- exiting\n");
			goto errexit;
		}
		memset (lickey, 0, sizeof(lickey));
		genlickey (lickey, &permflag, &expires, &host_id, custname);
		print_prompted_lickey (lickey, permflag, expires, host_id, custname);
    } 
	else          /* Parse cmd & argument values */
	{
		host_id = 0x00000000;
		demodays = 90;
		custname[0] = '\0';
		permflag = 0;
		while ((ch = getopt(argc, argv, "hpd:i:")) != -1) 
		{
			switch (ch) 
			{
				case 'h':
					do_usage();
					goto errexit;

				case 'p':
					if (!permkeyokflag) 
					{
						fprintf (stderr, "ERROR [%s]: This system is not authorized to create permanent license keys\n", argv0);
						goto errexit;
					}
					permflag = 1;
					break;
				case 'd':
					strcpy (buffer, optarg);
					sscanf (buffer, "%d", &demodays);
 					if (demodays < 1 || demodays > 999) 
					{
						fprintf (stderr, "ERROR [%s]: \"-d\" <demo days> specification must be between 1 and 999\n", argv0);
						goto errexit;
					}
					break;
				case 'i':
					strcpy (buffer, optarg);
					host_id = parse_hostid(buffer);
					if (host_id == 0)
					{
						fprintf (stderr, "ERROR [%s]: \"-i\" <hostid> specification must have 8 digits \"0x00000000\"\n", argv0);
						goto errexit;
					}
					break;
				default:
					do_usage();
					goto errexit;
			}
		}

		if (permflag && (host_id == 0x00000000)) 
		{
			fprintf (stderr, "ERROR [%s]: permanent license \"-p\" requires a hostid \"-i <hostid>\"\n", argv0);
			goto errexit;
		}

		custname[0] = '\0';
		(void) time(&nowtime);
		expires = nowtime + (demodays * 86400);
		memset (lickey, 0, sizeof(lickey));
		genlickey (lickey, &permflag, &expires, &host_id, custname);
		print_cgi_lickey (lickey);
    }

	
//#if defined(_WINDOWS) && defined(_DEBUG)
//	
//	printf("\nPress any key to continue....\n");
//	while(!_kbhit())
//		Sleep(1);
//#endif

	exit (0);

errexit:

#if defined(_WINDOWS) && defined(_DEBUG)
	
	printf("\nPress any key to continue....\n");
	while(!_kbhit())
		Sleep(1);
#endif

	exit (1);
}



