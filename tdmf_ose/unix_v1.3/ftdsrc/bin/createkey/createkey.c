
/*===========================================================================
 * createkey.c
 *
 * Copyright (c) 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef SOLARIS
#include <sys/systeminfo.h>
#endif
#include <stropts.h>

#include "license.h"
#include "../../lib/libgen/pathnames.h"

#define STR_MAX 80

static char* argv0;
/* expires October 1, 1998 */
static int exp_mon = 1;
static int exp_day = 1;
static int exp_year = 1999;
static int permkeyokflag=0;

/***********************************************************************/
static void
do_usage()
{
    fprintf(stderr, "\nUsage: %s\n", argv0);
    fprintf(stderr, "            for prompted execution, or\n");
    fprintf(stderr, "       %s -h \n", argv0);
    fprintf(stderr, "            for help, or\n");
    fprintf(stderr, "       %s [-p | -d <days>] [-i <hostid>]\n", argv0);
    fprintf(stderr, "            for command line execution.\n\n");
}

/***********************************************************************/
void query_input (char* prompt, char* response)
{
    char* previous;
    char buffer[BUFSIZ+1];
    char* ptr;
    do {
        previous = (response[0] == '\0') ? "NULL" : response;
        fprintf (stdout, "%s [%s]: ", prompt, previous);
        fflush(stdout);
        if (fgets(buffer, BUFSIZ, stdin) == (char *) NULL) {
            fprintf(stderr, "\nNULL input. Exit.\n");
            continue;
        }
        if ((ptr = strchr(buffer, '\n')) != (char *) NULL)
            *ptr = '\0';
        if (buffer[0] != '\0')
            strcpy(response, buffer);
        if (response[0] == '\0')
            fprintf(stderr, "*** Please enter requested input ***\n");
    } while (response[0] == '\0');
}

/***********************************************************************/
static int
prompt_input (int* permflag, int* demodays, long* host_id, char* custname)
{
    int ok;
    char buffer[BUFSIZ+1];
    char hbuf[BUFSIZ+1];
    char* ptr;
    int alldigits;
    int i;

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
    ok = 0;
    while (!ok) {
        buffer[0] = '\0';
        fprintf (stdout, "Generate a demo license?  [Y]  ");
        fflush (stdout);
        if (fgets(buffer, BUFSIZ, stdin) == (char*)NULL) continue;
        if ((ptr = strchr(buffer, '\n')) != (char *) NULL)
            *ptr = '\0';
        if (strlen(buffer) == 0) {
            *permflag = 0;
            break;
        }
        if (buffer[0] == 'n' || buffer[0] == 'N') {
            if (!permkeyokflag) {
                fprintf (stderr, "ERROR [%s]: This system is not authorized to create permanent license keys\n", argv0);
                exit (1);
            }
            *permflag = 1;
            break;
        }
        if (buffer[0] == 'y' || buffer[0] == 'Y') {
            *permflag = 0;
            break;
        }
        fprintf (stdout, "  *** answer \'y\' or \'n\' please ***\n");
        fflush (stdout);
    }
    if (!(*permflag)) {
        ok = 0;
        while (!ok) {
            *demodays = -1;
            buffer[0] = '\0';
            fprintf (stdout, "Number of days demo license is valid (5-90) [14]:  ");
            fflush (stdout);
            if (fgets(buffer, BUFSIZ, stdin) == (char*)NULL) continue;
            if ((ptr = strchr(buffer, '\n')) != (char *) NULL)
                *ptr = '\0';
            if (strlen(buffer) == 0) {
                *demodays = 14;
                break;
            }
            alldigits = 0;
            for (i=0; i<strlen(buffer); i++) {
                if (!isdigit(buffer[i])) {
                    fprintf (stdout, "  *** enter number between 5 and 90 ***\n");
                    fflush (stdout);
                    alldigits = 0;
                    break;
                } else {
                    alldigits = 1;
                }
            }
            if (alldigits) {
                i = 0;
                i = atoi(buffer);
                if (i < 5 || i > 90) {
                    fprintf (stdout, "  *** enter number between 5 and 90 ***\n");
                    fflush (stdout);
                    continue;
                }
                *demodays = i;
                ok = 1;
                *host_id = 0x00000000;
            }
        }
    } else {
        ok = 0;
        while (!ok) {
            alldigits = 1;
            *host_id = -1;
            buffer[0] = '\0';
            fprintf (stdout, "Hostid of for the permanent license (e.g. 0x80712345):  ");
            fflush (stdout);
            if (fgets(buffer, BUFSIZ, stdin) == (char*)NULL) continue;
            if ((ptr = strchr(buffer, '\n')) != (char *) NULL)
                *ptr = '\0';
            if (strlen(buffer) == 0) continue;
            if (0 != strncmp (buffer, "0x", 2) &&
                0 != strncmp (buffer, "0X", 2)) {
                sprintf (hbuf, "0x%s", buffer);
                strcpy (buffer, hbuf);
            } 
            for (i=2; i<strlen(buffer); i++) {
                if ((buffer[i] >= '0' && buffer[i] <= '9') ||
                    (buffer[i] >= 'a' && buffer[i] <= 'f') ||
                    (buffer[i] >= 'A' && buffer[i] <= 'F')) {
                    continue;
                } else {
                    alldigits = 0;
                }
            }
            if (!alldigits) {
                fprintf (stdout, "  *** Hostid must be of the form:  0xabcd1234 ***\n");
                fflush (stdout);
                continue;
            }
            sscanf (buffer, "%lx", host_id);
            ok = 1;
        }
    }
    return (0);
}            

/***********************************************************************/
static void
genlickey (char* lickey, int* permflag, int* demodays, long* host_id, 
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
    if (*permflag) {
        lic_out.expiration_date = 0;
        lic_out.license_expires = 0;
        lic_out.hostid = *host_id;
    } else {
        lic_out.expiration_date = (short) ((int) (ts + (*demodays * 86400))>>16);
        lic_out.license_expires = 1;
        lic_out.hostid = 0;
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
print_prompted_lickey (char* lickey, int permflag, int demodays, long host_id,
                       char* custname)
{
    time_t ts;
    time_t yon;
    struct tm *timp;
    FILE* fd;
    char *ptr;
    char buf[256];

    (void) time (&ts);
    fprintf (stdout, "==================================================\n");
    strcpy (buf, ctime(&ts));
    if ((ptr = strchr(buf, '\n')) != (char *) NULL)
        *ptr = '\0';
    fprintf (stdout, "DataCast for UNIX License generated:  %s\n", 
             buf);
    fprintf (stdout, "Customer:  %s\n", custname);
    if (!permflag) {
        yon = ts + (time_t)(demodays * 24 * 60 * 60);
        strcpy(buf, asctime(localtime(&yon)));
        if ((ptr = strchr(buf, '\n')) != (char *) NULL)
            *ptr = '\0';
        fprintf (stdout, 
                 "DEMO LICENSE [%d days] -- Expires:  %s\n", 
                 demodays, 
                 buf);
    } else {
        fprintf (stdout, "PERMANENT LICENSE -- hostid:  0x%08x\n", host_id);
    }
    fprintf (stdout, "  (please enter the following line into %s/%s.lic)\n",
             PATH_CONFIG, CAPQ);
    fprintf (stdout, "%s_LICENSE: %s\n", CAPQ, lickey);
    fprintf (stdout, "==================================================\n\n");
    fflush (stdout);
    
    fd = fopen ("ftd-licensekeys.log", "a+");
    fprintf (fd, "==================================================\n");
    strcpy (buf, ctime(&ts));
    if ((ptr = strchr(buf, '\n')) != (char *) NULL)
        *ptr = '\0';
    fprintf (fd, "DataCast for UNIX License generated:  %s\n", 
             buf);
    fprintf (fd, "Customer:  %s\n", custname);
    if (!permflag) {
        yon = ts + (time_t)(demodays *24 * 60 *60);
        strcpy(buf, asctime(localtime(&yon)));
        if ((ptr = strchr(buf, '\n')) != (char *) NULL)
            *ptr = '\0';
        fprintf (fd, "DEMO LICENSE [%d days] -- Expires:  %s\n", 
                 demodays, 
                 buf);
    } else {
        fprintf (fd, "PERMANENT LICENSE -- hostid:  0x%08x\n", host_id);
    }
    fprintf (fd, "  (please enter the following line into %s/%s.lic)\n",
             PATH_CONFIG, CAPQ);
    fprintf (fd, "%s_LICENSE: %s\n", CAPQ, lickey);
    fprintf (fd, "==================================================\n\n");
    fflush (fd);
    fclose (fd);
}

/***********************************************************************/
int
main(int argc, char **argv)
{
    int i, j;
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
#if defined(_AIX)
    int ch;
#else /* defined(_AIX) */
    char ch;
#endif /* defined(_AIX) */
    char hbuf[BUFSIZ+1];
    
    /* -- let main system at HQ and sophie in Boulder generate keys */
    long allowed_hosts[5];

    allowed_hosts[0] = 0x727215f8; 
    allowed_hosts[1] = 0x807c2ac7;
    allowed_hosts[2] = 0x8076ea89;
    allowed_hosts[3] = 0x808cf56c;
    allowed_hosts[4] = 0x80826742;
    permkeyokflag = 0;
    
    argv0 = argv[0];
    oktorun = 0;
    (void) time (&nowtime);
    custname[0] = '\0';
    permflag = 0;
    host_id = 0x00000000;
    demodays = 14;

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
/* XXXTEMP + */
#if 0
    for (i=0; i<5; i++) {
        if (myhostid == allowed_hosts[i]) {
            oktorun = 1;
            permkeyokflag = 1;
        }
    }
#endif /* 0 */
oktorun = 1;
permkeyokflag = 1;
/* XXXTEMP - */
    /* -- check for time limited availability */
    if (!oktorun) {
        if (nowtime < exptime) 
            oktorun = 1;
    }
    /* -- cancel run if not authorized */
    if (!oktorun) {
        fprintf (stderr, "You may not run %s from this system, expired.\n",
                 argv0);
        exit (1);
    }
    custname[0] = '\0';
    permflag = 0;
    host_id = 0x00000000;
    demodays = 14;

    if (argc == 1) {
        if (0 != prompt_input (&permflag, &demodays, &host_id, custname)) {
            fprintf (stderr, "Input error -- exiting\n");
            exit (1);
        }
        memset (lickey, 0, sizeof(lickey));
        genlickey (lickey, &permflag, &demodays, &host_id, custname);
        print_prompted_lickey (lickey, permflag, demodays, host_id, custname);
    } else {
        host_id = 0x00000000;
        demodays = 14;
        custname[0] = '\0';
        permflag = 0;
        while ((ch = getopt(argc, argv, "hpd:i:")) != -1) {
            switch (ch) {
            case 'h':
                do_usage();
                exit (0);
            case 'p':
                if (!permkeyokflag) {
                    fprintf (stderr, "ERROR [%s]: This system is not authorized to create permanent license keys\n", argv0);
                    exit (1);
                }
                permflag = 1;
                break;
            case 'd':
                strcpy (buffer, optarg);
                sscanf (buffer, "%d", &demodays);
                if (demodays < 5 || demodays > 90) {
                    fprintf (stderr, "ERROR [%s]: \"-d\" (demo days) specification must be between 5 and 90\n", argv0);
                    exit (1);
                }
                break;
            case 'i':
                strcpy (buffer, optarg);
                if (0 != strncmp("0x", buffer, 2) && 
                    0 != strncmp("0X", buffer, 2)) { 
                    sprintf (hbuf, "0x%s", buffer);
                    strcpy (buffer, hbuf);
                }
                sscanf (buffer, "%lx", &host_id);
                break;
            default:
                do_usage();
                exit(1);
            }
        }
        if (permflag && host_id == 0x00000000) {
            fprintf (stderr, "ERROR [%s]: permanent license (\"-p\") requires a hostid (\"-i <hostid>\"\n", argv0);
            exit (1);
        }
        custname[0] = '\0';
        genlickey (lickey, &permflag, &demodays, &host_id, custname);
        print_cgi_lickey (lickey);
    }
}



