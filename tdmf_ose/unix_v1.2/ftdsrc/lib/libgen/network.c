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
/**************************************************************************
 * network.c - FullTime Data Network communications module
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module defines the functions for implementing network communications
 * between the PMD and RMD daemons.
 *
 * History:
 *   10/14/96 - Steve Wahl	- original code
 *
 ***************************************************************************
 */
#ifdef NEED_BIGINTS
#include "bigints.h"
#endif
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <poll.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <limits.h>
#include <netinet/in.h>

#include "ftdio.h"
#include "ftdif.h"
#include "errors.h"
#include "config.h"
#include "network.h"
#include "pathnames.h"
#include "process.h"
#include "platform.h"
#include "common.h"
#include "aixcmn.h"
#include "ftd_trace.h"
#include "cfg_intr.h"

#include  <netinet/tcp.h>

// WI_338550 December 2017, implementing RPO / RTT
#include <sys/time.h>


#define BUFSIZE 30

ioqueue_t *fifo;

int g_rfddone = 0;
int g_ackhup = 0;


int max_elapsed_time = 1200; /* Changes for the timeout of manyLG.
                                (before int max_elapsed_time = 30)
                                This definition must be shorter
                                              than write_timeo(30min).
                                Therefore, it is referred to as 20min. */
int elapsed_time = 0;

int _net_bandwidth_analysis = 0;  // Network bandwidth analysis mode for PMD-RMD

int sizediff[SIZE];
volatile int _pmd_cpstart;
volatile int _pmd_cpstop;
volatile int _pmd_cpon;
volatile int _pmd_state;
volatile int _pmd_state_change;
char *_pmd_configpath;
volatile int check_commands_due_to_unclean_RMD_shutdown = 0;

const int exitfail = EXITRESTART;
#if defined(SOLARIS)
int dummyfd;
#endif

#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED   /* WR PROD4508 and HIST WR 38281 */
int do_check_if_RMD_mirrors_mounted = 0;
#endif

static struct timeval skosh;

static u_char *cbuf = NULL;
static int clen = 0;
static int net_write_timeo = 0;

static int RMD_version_known = 0;    /* WR 43770 */

extern sddisk_t *get_lg_dev(group_t*, int);
extern int compress(u_char*, u_char*, int);

extern void adjust_FRefresh_offset_backward( double percentage_backward, int force_complete_restart );

extern char *argv0;

static char pmdversionstring[256];
static char rmdversionstring[256];

EntryFunc entry_null;
extern EntryFunc get_counts;

extern float compr_ratio;

extern int compress_buf_len;
extern char* compress_buf;

extern void log_wlheader_and_previous_data(const wlheader_t* wlhead, const char* chunk);
extern void log_wlheader32_and_previous_data(const wlheader32_t* wlhead, const char* chunk);

int net_writable = 1;

extern int chksum_flag;

/* WR 43926: for sync mode, a counter of BAB overflows
   follows the chuncks for PMD to determine if BAB has been
   cleared between sending a chunk and receiving the corresponding ack; in
   which case data cannot be migrated off the BAB */
unsigned int  BAB_oflow_counter;

int remove_FR_watchdog_if_PMD_disconnets = 1;

int net_write_vector(int fd, struct iovec *iov, int iovcnt, int role);
int process_acks(void);

/* WR PROD6885: reboot-autostart flag files, HP-UX exception: 
   /var is not mounted on HPUX at time the groups start during reboot sequence; 
   the files must be in an accessible filesystem.
*/
#if defined(HPUX)
static char        autostart_base_directory[] = PATH_CONFIG;
#else
static char        autostart_base_directory[] = PATH_RUN_FILES;
#endif


/*******************************/
static void  system_sync( void )
{
#if  defined( linux )
   char  cmdline[16] =  "/bin/sync";
#else
   char  cmdline[16] =  "/usr/sbin/sync";
#endif
   system( cmdline );
   return;
}

/********************************************************************************
 * WR43376: check_unclean_shutdown: verify for the presence of the watchdog file
 *     which the RMD creates upon start, and deletes upon completion, of a Full
 *     Refresh; if the file is there, the RMD was shutdown in the middle of a Full
 *     Refresh.
 * return: 1 = unclean shutdown occurred; 0 otherwise.
 ********************************************************************************/
int check_unclean_shutdown( int lgnum )
{
   struct stat stat_buf;
   int         unclean_shutdown_flag;
   char        shutdown_file_path[MAXPATHLEN];

   sprintf( shutdown_file_path, "%s/RMD_shutdown_flags/RMD%03d_unclean_shutdown_check", PATH_RUN_FILES, lgnum );
   unclean_shutdown_flag = ( stat( shutdown_file_path , &stat_buf ) == 0 );
   return( unclean_shutdown_flag );
}

/*******************************************************************************
 * is_network_analysis_cfg_file: check if a group cfg file is for
 * network bandwidth analysis
 Return: 1 == yes
         0 = no
 *******************************************************************************/
int is_network_analysis_cfg_file( char *cfg_filename )
{
    FILE *fp;
    char cmd[128];
    char data[8];

    sprintf( cmd, "/bin/grep NETWORK-ANALYSIS %s | /bin/awk '{print $2}'", cfg_filename );

    fp = popen( cmd, "r" );
    if (fp == NULL)
    {
        return( 0 );
    }
    if( fgets(data, sizeof(data), fp) == NULL )
	{
	    // If the keyword is not found in the file, it can be a real config file; do not delete it
        pclose(fp);
	    return( 0 );
	}

    pclose(fp);
	data[2] = (char)0;
    if ((strcmp(data,"on") != 0) && (strcmp(data,"ON") != 0))
    {
	    return( 0 );
    }

	// It is a network analysis cfg file
	return( 1 );
}



/*******************************************************************************
 * remove_fictitious_cfg_files: delete network bandwidth analysis group cfg files
 * and prf files (backup to prf.1 files); we keep only the PMDs' .csv files
 *******************************************************************************/
void  remove_fictitious_cfg_files( int lgnum, int is_pmd )
{
    char full_cfg_path[MAXPATHLEN];
    char perfpath[MAXPATHLEN];
    char perfpathbackup[MAXPATHLEN];
    FILE *fp;
    char cmd[128];
    char data[8];
    struct stat statbuf;

    // Verify if the specified group has indeed a network-analysis (fictitious) config file
    if( is_pmd )
        sprintf( full_cfg_path, "%s/p%03d.cfg", PATH_CONFIG, lgnum );
	else
        sprintf( full_cfg_path, "%s/s%03d.cfg", PATH_CONFIG, lgnum );

    if( !is_network_analysis_cfg_file( full_cfg_path ) )
	    return;

    // Backup the prf file to prf.1 so dtcmonitortool knows the group is gone, and proceed with file deletion 
	if( is_pmd )
	{
	    sprintf (perfpath, "%s/p%03d.prf", PATH_RUN_FILES, lgnum);
	    sprintf (perfpathbackup, "%s/p%03d.prf.1", PATH_RUN_FILES, lgnum);
	    if (0 == stat(perfpath, &statbuf))
	    {
	        if (0 == stat(perfpathbackup, &statbuf))
	        {
	            (void) unlink (perfpathbackup);
	        }
	        (void) rename (perfpath, perfpathbackup);
	    }
		sync();
		sprintf( full_cfg_path, "%s/p%03d.cur", PATH_CONFIG, lgnum );
		unlink( full_cfg_path );
		sprintf( full_cfg_path, "%s/p%03d.cfg", PATH_CONFIG, lgnum );
		unlink( full_cfg_path );
	}
	else
	{
	    sprintf (perfpath, "%s/s%03d.prf", PATH_RUN_FILES, lgnum);
	    sprintf (perfpathbackup, "%s/s%03d.prf.1", PATH_RUN_FILES, lgnum);
	    if (0 == stat(perfpath, &statbuf))
	    {
	        if (0 == stat(perfpathbackup, &statbuf))
	        {
	            (void) unlink (perfpathbackup);
	        }
	        (void) rename (perfpath, perfpathbackup);
	    }
		sync();
		sprintf( full_cfg_path, "%s/s%03d.cfg", PATH_CONFIG, lgnum );
		unlink( full_cfg_path );
	}

	return;
}

/****************************************************************************
 * WR43376: remove_unclean_shutdown_file: delete Full Refresh watchdog file
 ****************************************************************************/
void  remove_unclean_shutdown_file( int lgnum )
{
   char        shutdown_file_path[MAXPATHLEN];
   time_t      current_time;

   syncgroup( 1 );      /* Force flushing pending IO buffers of this group's devices */
   sprintf( shutdown_file_path, "%s/RMD_shutdown_flags/RMD%03d_unclean_shutdown_check", PATH_RUN_FILES, lgnum );
   if( unlink( shutdown_file_path ) != 0)
   {
      if( errno != ENOENT )  /* If file not there, do not report any error; else, do so */
      {
         sprintf( debug_msg, "Error while attempting to remove unclean-shutdown flag file; errno = %d\n", errno );
         reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg );
      }
   }
   else  // No error removing the watchdog file: sync now
   {
       system_sync();
   }
   return;
}

/****************************************************************************
 * WR43376: create_unclean_shutdown_file: create Full Refresh watchdog file
 ****************************************************************************/
void  create_unclean_shutdown_file( int lgnum )
{
   char        shutdown_file_path[MAXPATHLEN];
   FILE        *fp;

   sprintf( shutdown_file_path, "%s/RMD_shutdown_flags", PATH_RUN_FILES );
   if( (mkdir( shutdown_file_path, 0744 ) != 0) && (errno != EEXIST) )
   {
      sprintf( debug_msg, "Error while attempting to create unclean-shutdown flag directory; errno = %d\n", errno );
      reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg );
   }
   else
   {
      sprintf( shutdown_file_path, "%s/RMD_shutdown_flags/RMD%03d_unclean_shutdown_check", PATH_RUN_FILES, lgnum );
      if( (fp = fopen( shutdown_file_path, "w" )) == 0 )
      {
         sprintf( debug_msg, "Error while attempting to create unclean-shutdown flag file; errno = %d\n", errno );
         reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg );
      }
      else
      {
         fclose( fp );
      }
      system_sync();
   }
   return;
}

/**
 * Obtains the path of the corrupted journal file flag for a given group.
 */
static char* get_corrupted_journal_file_path(int lgnum)
{
    static char corrupted_journal_file_path[MAXPATHLEN];
    
    sprintf(corrupted_journal_file_path, "%s/RMDA%03d_corrupted_journal_found", PATH_RUN_FILES, lgnum);

    return corrupted_journal_file_path;
}

/**
 * Checks if a corrupted journal has been signalled by the RMDA.
 *
 * @return 1 if the flag is present, 0 otherwise.
 */
int check_corrupted_journal_flag( int lgnum )
{
    struct stat stat_buf;
    int corrupted_journal_found = 0;
    
    corrupted_journal_found = ( stat( get_corrupted_journal_file_path(lgnum), &stat_buf ) == 0 );
    return corrupted_journal_found;
}

/**
 * Removes the corrupted journal file.
 */
void remove_corrupted_journal_flag( int lgnum )
{
    if( unlink( get_corrupted_journal_file_path(lgnum) ) != 0)
    {
        if( errno != ENOENT )  /* If file not there, do not report any error; else, do so */
        {
            sprintf( debug_msg, "Error while attempting to remove corrupted journal flag file; errno = %d\n", errno );
            reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg );
        }
    }
    
    return;
}

/**
 * Signals that a corrupted journal has been found by the RMDA.
 */
void flag_corrupted_journal( int lgnum )
{
    FILE *fp;
    
    if( (fp = fopen( get_corrupted_journal_file_path(lgnum), "w" )) == 0 )
    {
        sprintf( debug_msg, "Error while attempting to create corrupted journal flag file; errno = %d\n", errno );
        reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg );
    }
    else
    {
        fclose( fp );
    }
    
    return;
}


/********************************************************************************
 WR PROD6443: if we are restarting after a reboot, this function allows to
 create a file which will tell this situation to the PMD and then, if the reboot
 occurred while a Full Refresh was underway, the PMD will know it must take
 special actions */
void  create_reboot_autostart_file( int lgnum )
{
   char        autostart_file_path[MAXPATHLEN];
   FILE        *fp;

   sprintf( autostart_file_path, "%s/PMD_autostart_flags", autostart_base_directory );
   if( (mkdir( autostart_file_path, 0744 ) != 0) && (errno != EEXIST) )
   {
      sprintf( debug_msg, "Error while attempting to create PMD reboot-autostart flag directory; errno = %d\n", errno );
      reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg );
   }
   else
   {
      sprintf( autostart_file_path, "%s/PMD_autostart_flags/PMD%03d_reboot_autostart", autostart_base_directory, lgnum );
      if( (fp = fopen( autostart_file_path, "w" )) == 0 )
      {
         sprintf( debug_msg, "Error while attempting to create PMD reboot-autostart flag file; errno = %d\n", errno );
         reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg );
      }
      else
      {
         fclose( fp );
      }
      system_sync();
   }
   return;
}

/* WR PROD6443 *******************************************************************************/
void  remove_reboot_autostart_file( int lgnum )
{
   char        autostart_file_path[MAXPATHLEN];

   sprintf( autostart_file_path, "%s/PMD_autostart_flags/PMD%03d_reboot_autostart", autostart_base_directory, lgnum );
   if( unlink( autostart_file_path ) != 0)
   {
      if( errno != ENOENT )  /* If file not there, do not report any error; else, do so */
      {
         sprintf( debug_msg, "Error while attempting to remove PMD reboot-autostart flag file; errno = %d\n", errno );
         reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg );
      }
   }
   else  // No error removing the watchdog file: sync now
   {
       /* If no file left in the directory, remove the directory (rmdir will remove only if directory is empty) */
	   sprintf( autostart_file_path, "%s/PMD_autostart_flags", autostart_base_directory );
       rmdir( autostart_file_path );
       system_sync();
   }
   return;
}

/* WR PROD6443 *******************************************************************************/
int in_reboot_autostart_mode( int lgnum )
{
   struct stat stat_buf;
   int         reboot_autostart;
   char        autostart_file_path[MAXPATHLEN];

   sprintf( autostart_file_path, "%s/PMD_autostart_flags/PMD%03d_reboot_autostart", autostart_base_directory, lgnum );
   reboot_autostart = ( stat( autostart_file_path , &stat_buf ) == 0 );
   return( reboot_autostart );
}

/*
 * set_tcp_send_low -- set TCP send low water mark 
 */
int
set_tcp_send_low(void) 
{
#if defined(_AIX) || defined(linux)
    size_t len; 
#else /* defined(_AIX) */
    int len;
#endif /* defined(_AIX) */
    int max_packet, n, rc;

    len = sizeof(int); 
    rc = getsockopt(mysys->sock, SOL_SOCKET, SO_SNDBUF,
#if defined(_AIX)
        (void *)&max_packet, 
#else /* defined(_AIX) */
        (char*)&max_packet, 
#endif /* defined(_AIX) */
        &len);
    ftd_trace_flow(FTD_DBG_SOCK, "TCP send buffer size = %d\n", max_packet);
    if (rc == -1) {
        return -1;
    }
#if defined(_AIX)
    max_packet = 1;
#endif
    rc = setsockopt(mysys->sock, SOL_SOCKET, SO_SNDLOWAT,
#if defined(_AIX)
        (void*)&max_packet, sizeof(int));
#else /* defined(_AIX) */
        (char*)&max_packet, sizeof(int));
#endif

    len = sizeof(int);
    getsockopt(mysys->sock, SOL_SOCKET, SO_SNDLOWAT,
        (char*)&n, &len);
    ftd_trace_flow(FTD_DBG_SOCK, "TCP send lowater = %d\n", n);
    return 0;
}

/*
 * set_tcp_send_timeo -- set TCP send timeout
 */
int
set_tcp_send_timeo(void) 
{
    struct timeval timeo[1];
#if defined(_AIX)
    size_t len; 
#else /* defined(_AIX) */
    int len;
#endif /* defined(_AIX) */

    timeo->tv_usec = 100000;
    timeo->tv_sec = 0;

    setsockopt(mysys->sock, SOL_SOCKET, SO_SNDTIMEO,
        (char*)timeo, sizeof(struct timeval));
#ifdef TDMF_TRACE
    len = sizeof(struct timeval);
    getsockopt(mysys->sock, SOL_SOCKET, SO_SNDTIMEO,
#if defined(_AIX)
        (void *)timeo,
#else /* defined(_AIX) */
        (char*)timeo,
#endif /* defined(_AIX) */
        &len);
    fprintf(stderr,"\n*** TCP send timeout = %d:%d\n",
        timeo->tv_sec, timeo->tv_usec);
#endif
    return 0;
}

/*
 * set_tcp_nonb -- set TCP socket to non-blocking 
 */
int
set_tcp_nonb(void) 
{
    int val;

    /* get the socket state */
    val = fcntl(mysys->sock, F_GETFL, 0);

    /* set the socket to non-blocking */
    return fcntl(mysys->sock, F_SETFL, val | O_NONBLOCK);
}

/****************************************************************************
 * initnetwork -- initialize the network interface
 ***************************************************************************/
int
initnetwork () 
{
    int retval;
    int lineno;
    char buf[INET6_ADDRSTRLEN];
    lineno = 0;
    /* -- verify our hostname and/or IP address */
#if defined(FTD_IPV4)
     retval = isthisme4 (mysys->name, mysys->ip);
#else
     retval = isthisme6 (mysys->name, mysys->ipv);
#endif
    switch (retval) {
    case UNRESOLVNAME:
        reporterr (ERRFAC, M_BADHOSTNAM, ERRCRIT, mysys->configpath, lineno, \
            mysys->name);
        return (1);

    case UNRESOLVIPADDR:
#if defined(FTD_IPV4)
            reporterr (ERRFAC, M_BADIPADDR, ERRCRIT, mysys->configpath, lineno, \
            mysys->name, iptoa(mysys->ip));
            return (1);
#else
            reporterr (ERRFAC, M_BADIPADDR, ERRCRIT, mysys->configpath, lineno, \
            mysys->name, inet_ntop(AF_INET6, &((struct sockaddr_in6 *)mysys->ipv)->sin6_addr,buf,sizeof(buf)));
            return (1);
#endif /* defined(FTD_IPV4)*/

    case NAMEIPADDRCONFLICT:
#if defined(FTD_IPV4)
            reporterr (ERRFAC, M_BADNAMIP, ERRCRIT, mysys->configpath, lineno,
            mysys->name, iptoa(mysys->ip));
            return (1);
#else
           reporterr (ERRFAC, M_BADNAMIP, ERRCRIT, mysys->configpath, lineno,
           mysys->name,inet_ntop(AF_INET6, &((struct sockaddr_in6 *)mysys->ipv)->sin6_addr,buf,sizeof(buf)));
           return (1);
#endif /* defined(FTD_IPV4)*/
    case NAMEIPNOTFOUND: return (0);
    case NAMEIPFOUND: return (0);
    }

    return (1);
}

/****************************************************************************
 * IPV4_to_IPv6 -- Converts an IPv4 address to an IPv6 address
 *                 i.e. IPv4 mapped IPv6 address                 
 ***************************************************************************/
#if !defined(FTD_IPV4)
void IPv4_to_IPv6(struct in_addr *address_4,struct in6_addr *address_6)
{
memset(address_6,0,sizeof(struct in6_addr));

#if defined(linux)|| defined(HPUX)|| defined(_AIX)
address_6->s6_addr32[3]=address_4->s_addr;
address_6->s6_addr16[5]=0xFFFF;
#endif

#if defined(SOLARIS)
address_6->_S6_un._S6_u32[3]=address_4->s_addr;
address_6->s6_addr[11]=0xFF;
address_6->s6_addr[10]=0xFF;
#endif 
}
#endif 

/****************************************************************************
 * createconnection -- establishes a connection with a remote daemon
 *                     returns 0 = success, 1 = failure
 ***************************************************************************/
int
createconnection (int reporterrflag)
{
    struct sockaddr_in myaddr; 
    struct sockaddr_in servaddr;
    int len;
    int n;
    char tcpwinsize[32];
    int tcp_window_size;
    int result;
    int hp; 
    struct hostent *lp;
    char tempport[30];
    int err;
    int rc;
    char host[NI_MAXHOST];
    char buf[INET6_ADDRSTRLEN];
    char addrbuf[INET_ADDRSTRLEN];
    int found = 1;
    int sd;
#if !defined(FTD_IPV4)
    struct in6_addr addr_6,serveraddr;
    struct in_addr addr_4;
    struct sockaddr_storage saddr;
    struct addrinfo hints,*ai, *res, *tmp, *ap;
    struct sockaddr_in6 addr;
    int retval = -1;
    int rec = -1;
    int ret = -1;
    int family = AF_INET6;
    int sock_family;
    int val = -1;
    int rt = -1;
    char sysname[4096];
#endif

    if ((cfg_get_key_value("tcp_window_size", tcpwinsize, CFG_IS_NOT_STRINGVAL)) == CFG_OK)
    {
        tcp_window_size = atoi(tcpwinsize);
    }
    else
    {
        tcp_window_size = DEFAULT_TCP_WINDOW_SIZE;
    }
    
#if defined(FTD_IPV4)
       if (mysys->isconnected) {
         if (reporterrflag) reporterr (ERRFAC, M_CNCTED, ERRWARN);
          return 0;
    }
    if ((mysys->sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
       if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, "socket", strerror(errno));
        }
        reporterr (ERRFAC, M_SOCKFAIL, ERRCRIT, strerror(errno));
         return 1;
    }
    memset((char*)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htons(INADDR_ANY);
    myaddr.sin_port = htons(0);

    len = sizeof(myaddr);
    if (bind(mysys->sock, (struct sockaddr *) &myaddr, len) < 0) {
          reporterr (ERRFAC, M_SOCKBIND, ERRCRIT, strerror(errno));
          close(mysys->sock);
          return 1;
    }
    memset ((char*)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(othersys->secondaryport);
  
    /* -- if the other system host name is empty or gethostbyname fails
     *    then try gethostbyaddr */
    lp = 0;
    if (othersys->ip != 0L) {
#if defined(HPUX) || defined(_AIX) || defined(linux)
        lp = gethostbyaddr((char*)&othersys->ip, sizeof(u_long), AF_INET);
#elif defined(SOLARIS)  /* 1LG-ManyDEV */
        GETHOSTBYADDR_MANYDEV((char*)&othersys->ip, sizeof(u_long), AF_INET, lp, dummyfd);
#endif
    } else if (strlen(othersys->name) > 0) {
#if defined(HPUX) || defined(_AIX) || defined(linux)
        lp = gethostbyname(othersys->name);
#elif defined(SOLARIS)  /* 1LG-ManyDEV */
        GETHOSTBYNAME_MANYDEV(othersys->name, lp, dummyfd);
#endif
    }
    if (lp == 0) {
        reporterr (ERRFAC, M_HOSTLKUP, ERRCRIT, othersys->name);
        close(mysys->sock);
        return 1;
    }
    
#ifdef _AIX  /* TCP_RFC1323 support */
    // The TCP_RFC1323 option is only available on AIX.
    // We'll only enable the RFC1323 improvements if we request a buffer larger than 64k and know we'll need the window scale option. 
    if (tcp_window_size > (64*1024) )
    {
        int on = 1;
        setsockopt(mysys->sock, IPPROTO_TCP, TCP_RFC1323, (char*)&on, sizeof(on));
    }
#endif /* TCP_RFC1323 */
    
    /* set the TCP SNDBUF, RCVBUF to the configured tcp_window_size, if any. */
    if (tcp_window_size > 0)
    {
        n = tcp_window_size;
        setsockopt(mysys->sock, SOL_SOCKET, SO_SNDBUF, (char*)&n, sizeof(int));
        setsockopt(mysys->sock, SOL_SOCKET, SO_RCVBUF, (char*)&n, sizeof(int));
    }

    memcpy((caddr_t)&servaddr.sin_addr, lp->h_addr_list[0], lp->h_length);
    if (0 > (result = connect(mysys->sock,
        (struct sockaddr*)&servaddr,sizeof(servaddr)))) {
        if (reporterrflag) {
            reporterr (ERRFAC, M_CONERR, ERRWARN,
                lp->h_name, othersys->secondaryport, strerror(errno));
        }
        close(mysys->sock);
        return 1;
    }
    n = 1;
    if (setsockopt(mysys->sock, SOL_SOCKET, SO_KEEPALIVE,(char*)&n,sizeof(int))) {
        reporterr (ERRFAC, M_SOCKOP, ERRCRIT, strerror(errno));
        close(mysys->sock);
        return (1);
    }
    mysys->isconnected = 1;

    return (0);
#else /* FTD_IPV4 */
    if (mysys->isconnected) {
        if (reporterrflag) reporterr (ERRFAC, M_CNCTED, ERRWARN);
        return 0;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    retval = getaddrinfo(NULL, "0", &hints, &ai);
    if (retval != 0) {
       reporterr(ERRFAC, M_SOCKFAIL, ERRCRIT, gai_strerror(retval));
       return (1);
    }
    tmp = ai;
    sock_family = ai->ai_family;
    retval = -1;
#if defined(_AIX) || defined(linux)
    while(ai) {
      if (ai->ai_family == AF_INET6) {
        mysys->sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (mysys->sock > 0) {
           if (bind(mysys->sock,ai->ai_addr, ai->ai_addrlen) == 0) {
             retval = 0;
             sock_family = AF_INET6;
             break;
           }
        }
      }
      ai = ai->ai_next;
    }


    if (ai == NULL) {
      ai = tmp;
      while(ai) {
        if (ai->ai_family == AF_INET) {
          mysys->sock = socket(AF_INET, ai->ai_socktype, ai->ai_protocol);
          if (mysys->sock > 0) {
            if (bind(mysys->sock,ai->ai_addr, ai->ai_addrlen) == 0) {
              retval = 0;
              sock_family = AF_INET;
              break;
            }
          }
	}
        ai = ai->ai_next;
      }
    }
#else /* defined(_AIX) */
    while(ai) {
      mysys->sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
      if (mysys->sock > 0) {
           if (bind(mysys->sock,ai->ai_addr, ai->ai_addrlen) == 0) {
             retval = 0;
             sock_family = ai->ai_family;
             break;
           }
      }
      ai = ai->ai_next;
    }
#endif /* defined(_AIX) */
    freeaddrinfo(tmp);
    if (retval == -1) {
       reporterr (ERRFAC, M_SOCKBIND, ERRCRIT, strerror(errno));
       close(mysys->sock);
       EXIT(EXITNORMAL);
    }
    sprintf(tempport,"%d",othersys->secondaryport);

    /* -- Get the Name resoluted using getaddrinfo--*/

     if (strlen(othersys->name) > 0 || othersys->ipv != NULL) {

      strcpy(sysname, othersys->name);
      memset(&hints, 0, sizeof(hints));

      rec = inet_pton(AF_INET, sysname, &saddr);
      if (rec == 1)    /* If a valid IPv4 numeric address,then no IP address resolution is required */
      {
         hints.ai_family = AF_INET;
         hints.ai_flags = AI_NUMERICHOST;
         hints.ai_socktype = SOCK_STREAM;
      }
      else
      {
         rec = inet_pton(AF_INET6, sysname, &saddr);
         if (rec == 1) /* If a valid IPv6 numeric address,then no IP address resolution is required */
         {
            hints.ai_family = AF_INET6;
            hints.ai_flags = AI_NUMERICHOST;
	    hints.ai_socktype = SOCK_STREAM;
         }
      }
     /* If machine name is passed from the configuration file, we require name resolution*/
      if (rec != 1){
          memset(&hints, 0, sizeof(hints));
          hints.ai_flags = AI_CANONNAME;
          hints.ai_socktype = SOCK_STREAM;
      }
     err = getaddrinfo(sysname,(const char*)tempport, &hints, &res);

     if (err != 0) {
        reporterr(ERRFAC, M_SNAMRESOLVFAIL, ERRCRIT, gai_strerror(err));
        close(mysys->sock);
	EXIT(EXITANDDIE);
        }
     
       /*-- Check for IPv6 Link local address --*/
#if defined(linux)
       memset(buf, 0, sizeof(buf));
       inet_ntop(AF_INET6, &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr,buf,sizeof(buf));

       ret = inet_pton(AF_INET6, buf,&addr.sin6_addr);

       if (ret == 1)
           val = IN6_IS_ADDR_LINKLOCAL(&addr.sin6_addr);

       if (val == 1){
           reporterr(ERRFAC,M_LINKLOCAL,ERRCRIT);
           EXIT(EXITANDDIE);
       }
#endif /* linux */

      }

#ifdef _AIX  /* TCP_RFC1323 support */  
     //  The TCP_RFC1323 option is only available on AIX.
     // We'll only enable the RFC1323 improvements if we request a buffer larger than 64k and know we'll need the window scale option. 
     if (tcp_window_size > (64*1024) )
     {
         int on = 1;
         setsockopt(mysys->sock, IPPROTO_TCP, TCP_RFC1323, (char*)&on, sizeof(on));
     }
#endif /* TCP_RFC1323 */
     
    /* set the TCP SNDBUF, RCVBUF to the configured tcp_window_size, if any. */
    if (tcp_window_size > 0)
    {
        n = tcp_window_size;
        setsockopt(mysys->sock, SOL_SOCKET, SO_SNDBUF, (char*)&n, sizeof(int));
        setsockopt(mysys->sock, SOL_SOCKET, SO_RCVBUF, (char*)&n, sizeof(int));
    }

    /* If PMD opens a IPv4 socket and the secondary address is of IPv6 type, then kill the PMD*/
     if (sock_family == AF_INET && res->ai_family == AF_INET6){
        reporterr (ERRFAC,M_MISMATCH,ERRCRIT);
        EXIT(EXITANDDIE);
     }
     /*-- Try connecting using address list returned by getaddrinfo --*/
       tmp = res;
    while(res) {
       if (connect(mysys->sock, res->ai_addr, res->ai_addrlen) == 0) {
           break;
       }
       else {
           if (res->ai_family == AF_INET) {
              family = AF_INET;
            }
           res = res->ai_next;
           if (res == NULL) {
              found = 0;
              break;
           }
       }
    }
    /* The below code is used for establishing connection with the RMD
       if no IPv6 address is configured for the machine i.e. when found is
       zero
    */
#if defined(_AIX)
     if (found == 0) {
       if (family == AF_INET) {
       close(mysys->sock);
       retval = -1;

      memset(&hints, 0, sizeof(hints));
      hints.ai_flags = AI_PASSIVE;
      hints.ai_socktype = SOCK_STREAM;
      retval = getaddrinfo(NULL, "0", &hints, &ap);
       if (retval != 0) {
          reporterr(ERRFAC, M_SNAMRESOLVFAIL, ERRCRIT, gai_strerror(retval));
          return (1);
       }

      memset(buf, 0, sizeof(buf));

      inet_ntop (AF_INET,&((struct sockaddr_in*)tmp->ai_addr)->sin_addr,buf,sizeof(buf));
      if (inet_pton (AF_INET,buf,&addr_4) != 0) {
             IPv4_to_IPv6(&addr_4,&addr_6);
             inet_ntop (AF_INET6, &addr_6, buf, INET6_ADDRSTRLEN);
          }

      rt = inet_pton(AF_INET, buf, &saddr);

      if (rt == 1)    /* valid IPv4 text address? */
      {
         hints.ai_family = AF_INET;
         hints.ai_flags |= AI_NUMERICHOST;
      }
      else
      {
         rt = inet_pton(AF_INET6, buf, &saddr);
         if (rt == 1) /* valid IPv6 text address? */
         {
            hints.ai_family = AF_INET6;
            hints.ai_flags |= AI_NUMERICHOST;
         }
      }

      rt = getaddrinfo(buf, (const char*)tempport, &hints, &res);

      if (rt != 0) {
          reporterr(ERRFAC, M_SNAMRESOLVFAIL, ERRCRIT, gai_strerror(retval));
	  EXIT(EXITANDDIE);
       }

      mysys->sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

      while(ap) {
        if (mysys->sock > 0) {
           if(ap->ai_family == AF_INET6){
              if (bind(mysys->sock,ap->ai_addr, ap->ai_addrlen) == 0) {
                    retval = 0;
                    break;
                }
             }
           ap = ap->ai_next;
         }
      }

      freeaddrinfo(ap);

     if (retval == -1) {
          reporterr (ERRFAC, M_SOCKBIND, ERRCRIT, strerror(errno));
          close(mysys->sock);
          freeaddrinfo(res);
          freeaddrinfo(tmp);
          EXIT(EXITNORMAL);
        }

#ifdef _AIX  /* TCP_RFC1323 support */ 
     //  The TCP_RFC1323 option is only available on AIX.
     // We'll only enable the RFC1323 improvements if we request a buffer larger than 64k and know we'll need the window scale option.
     if (tcp_window_size > (64*1024) )
     {
         int on = 1;
         setsockopt(mysys->sock, IPPROTO_TCP, TCP_RFC1323, (char*)&on, sizeof(on));
     }
#endif /* TCP_RFC1323 */
     
      /* set the TCP SNDBUF, RCVBUF to the configured tcp_window_size, if any. */
      if (tcp_window_size > 0)
      {
          n = tcp_window_size;
          setsockopt(mysys->sock, SOL_SOCKET, SO_SNDBUF, (char*)&n, sizeof(int));
          setsockopt(mysys->sock, SOL_SOCKET, SO_RCVBUF, (char*)&n, sizeof(int));
      }

      sd = connect(mysys->sock,res->ai_addr,res->ai_addrlen);

     if (sd < 0) {
            if (reporterrflag){
             reporterr (ERRFAC, M_CONERR, ERRWARN,\
                           sysname, othersys->secondaryport, strerror(errno));
          }
          close(mysys->sock);
          freeaddrinfo(res);
          freeaddrinfo(tmp);
          return 1;
       }
  freeaddrinfo(res);
    }
  }
#endif /*defined(_AIX)*/
#if defined(HPUX)||defined(linux)
    if (found == 0) {
       if (family == AF_INET) {
          memset(buf, 0, sizeof(buf));
          inet_ntop (AF_INET,&((struct sockaddr_in*)tmp->ai_addr)->sin_addr,buf,sizeof(buf));
          if (inet_pton (AF_INET,buf,&addr_4) != 0) {
             IPv4_to_IPv6(&addr_4,&addr_6);
             inet_ntop (AF_INET6, &addr_6, buf, INET6_ADDRSTRLEN);
          }
          memset(&hints, 0, sizeof(hints));
          hints.ai_flags = AI_NUMERICHOST;
          hints.ai_socktype = SOCK_STREAM;
          rc = getaddrinfo(buf, (const char*)tempport, &hints, &res);

          if (rc != 0) {
             reporterr(ERRFAC,M_SNAMRESOLVFAIL,ERRWARN);
	     EXIT(EXITANDDIE);
          }

          /* set the TCP SNDBUF, RCVBUF to the configured tcp_window_size, if any. */
          if (tcp_window_size > 0)
          {
              n = tcp_window_size;
              setsockopt(mysys->sock, SOL_SOCKET, SO_SNDBUF, (char*)&n, sizeof(int));
              setsockopt(mysys->sock, SOL_SOCKET, SO_RCVBUF, (char*)&n, sizeof(int));
          }     

      sd = connect(mysys->sock, res->ai_addr, res->ai_addrlen);

       if (sd < 0) {
        if (reporterrflag){
          reporterr (ERRFAC, M_CONERR, ERRWARN,
             sysname, othersys->secondaryport, strerror(errno));
          }
          close(mysys->sock);
          freeaddrinfo(res);
          freeaddrinfo(tmp);
          return 1;
       }
        freeaddrinfo(res);
      }
    }
#endif /* defined(HPUX)||defined(linux) */
#if defined(SOLARIS)
    if (found == 0) {
       close(mysys->sock);
       retval = -1;

       memset(&hints, 0, sizeof(hints));
       hints.ai_flags = AI_PASSIVE;
       hints.ai_socktype = SOCK_STREAM;
       retval = getaddrinfo(NULL, "0", &hints, &ap);
       if (retval != 0) {
          reporterr(ERRFAC, M_SNAMRESOLVFAIL, ERRCRIT, gai_strerror(retval));
	  return (1);
       }
       mysys->sock = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
       if (mysys->sock > 0) {
          if (bind(mysys->sock,ap->ai_addr, ap->ai_addrlen) == 0) {
             retval = 0;
          }
       }
       freeaddrinfo(ap);
       if (retval == -1) {
          reporterr (ERRFAC, M_SOCKBIND, ERRCRIT, strerror(errno));
          close(mysys->sock);
          freeaddrinfo(tmp);
          EXIT(EXITNORMAL);
        }

      /* set the TCP SNDBUF, RCVBUF to the configured tcp_window_size, if any. */
       if (tcp_window_size > 0)
       {
           n = tcp_window_size;   
           setsockopt(mysys->sock, SOL_SOCKET, SO_SNDBUF, (char*)&n, sizeof(int));
           setsockopt(mysys->sock, SOL_SOCKET, SO_RCVBUF, (char*)&n, sizeof(int));
       }

      sd = connect(mysys->sock,tmp->ai_addr,tmp->ai_addrlen);

       if (sd < 0) {
            if (reporterrflag){
             reporterr (ERRFAC, M_CONERR, ERRWARN,\
                           sysname, othersys->secondaryport, strerror(errno));
          }
          close(mysys->sock);
          freeaddrinfo(tmp);
          return 1;
       }
    }
#endif /* defined(SOLARIS)*/
    n = 1;
    if (setsockopt(mysys->sock, SOL_SOCKET, SO_KEEPALIVE,(char*)&n,sizeof(int))) {
        reporterr (ERRFAC, M_SOCKOP, ERRCRIT, strerror(errno));
        close(mysys->sock);
        freeaddrinfo(tmp);
        return (1);
    }
    freeaddrinfo(tmp);  
    mysys->isconnected = 1;
    return 0;
#endif /* defined(FTD_IPV4) */
} /* createconnection */

/****************************************************************************
 * closeconnection -- severs the connection between two daemons orderly
 ***************************************************************************/
void
closeconnection (void)
{
    headpack_t header;
    headpack32_t header32;
    sddisk_t* sd;
    sddisk_t* nextsd;
    int       warning_double_free = 0;    /* WR 43926 */

    if (mysys == NULL) {
        return;
    }
    if (mysys->isconnected) {
        if (ISPRIMARY(mysys)) {
            header.magicvalue = MAGICHDR;
            header.cmd = CMDEXIT;
            (void)time(&header.ts);
            header.ackwanted = 0;
            if (!rmd64)
            {
   		converthdrfrom64to32 (&header, &header32);
            	(void)writesock(mysys->sock, (char*)&header32, sizeof(headpack32_t)); 
            }
            else
            {
            	(void)writesock(mysys->sock, (char*)&header, sizeof(headpack_t)); 
            }
            close(mysys->sock);
            mysys->isconnected = 0;
        }
    }
    sd = mysys->group->headsddisk; 
    /* Check if linked lists of device structures are same on mysys and othersys (WR 43926) */
    warning_double_free = (othersys->group->headsddisk == mysys->group->headsddisk);
    while(sd) {
        nextsd = sd->n;
        close(sd->rsync.devfd);
        free (sd);
        sd = nextsd;
    }
    if( !warning_double_free )   /* WR 43926 */
    {
       sd = othersys->group->headsddisk;
       while (sd) {
           nextsd = sd->n;
           close(sd->rsync.devfd);
           free (sd);
           sd = nextsd;
       }
    }
    mysys->isconnected = 0;

    if (ISSECONDARY(mysys)) {
        reporterr (ERRFAC, M_RMDEXIT, ERRINFO, argv0);
        EXIT(EXITNORMAL);
    }
} /* closeconnection */

/****************************************************************************
 * sendhup -- send a hup packet to the RMD 
 ***************************************************************************/
int
sendhup(void) 
{
    headpack_t header[1];
    headpack32_t header32[1];

    memset(header, 0, sizeof(header));
    memset(header32, 0, sizeof(header32));
    header->magicvalue = MAGICHDR;
    header->cmd = CMDHUP;
    header->ackwanted = 1;
    if (!rmd64)
    {
    	converthdrfrom64to32 (header, header32);
    	if (-1 == writesock(mysys->sock, (char*)header32, sizeof(headpack32_t))) {
        	EXIT(EXITNETWORK);
    	}
    }
    else
    {
    	if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        	EXIT(EXITNETWORK);
    	}
    }
    return 0;
} /* sendhup */
  
/****************************************************************************
 * ask_if_RMD_mirrors_mounted: send a cmd to the RMD to verify if any of its
 * target mirror devices is mounted; the answer will be treated in a
 * subsequent process_acks()call.
 * WR PROD4508 and HIST WR 38281
 ***************************************************************************/
#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED
void
ask_if_RMD_mirrors_mounted( void )
{
    headpack_t header[1];
    headpack32_t header32[1];

    /* We can send this command only if we talk to an RMD of a release which has this fix */
    if( !do_check_if_RMD_mirrors_mounted )
	{
	    return;
	}
    memset(header, 0, sizeof(header));
    memset(header32, 0, sizeof(header32));
    header->magicvalue = MAGICHDR;
    header->cmd = CMDCKMIRSMTD;
    header->ackwanted = 1;
    if (!rmd64)
    {
        converthdrfrom64to32 (header, header32);
        if (-1 == writesock(mysys->sock, (char*)header32, sizeof(headpack32_t))) {
                EXIT(EXITNETWORK);
        }
    }
    else
    {
        if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
                EXIT(EXITNETWORK);
        }
    }

    return;
} /* ask_if_RMD_mirrors_mounted */
#endif  /* of CHECK_IF_RMD_MIRRORS_MOUNTED */


/****************************************************************************
 * send_cp_err -- send a checkpoint error packet to the RMD 
 ***************************************************************************/
int
send_cp_err(int msgtype) 
{
    headpack_t header[1];
    headpack32_t header32[1];

    memset(header, 0, sizeof(header));
    memset(header32, 0, sizeof(header32));
    header->magicvalue = MAGICHDR;
    header->cmd = msgtype;
    header->ackwanted = 1;
    if (!rmd64)
    {
 	converthdrfrom64to32 (header, header32);
    	if (-1 == writesock(mysys->sock, (char*)header32, sizeof(headpack32_t))) {
        	EXIT(EXITNETWORK);
    	}
    }
    else
    {
    	if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        	EXIT(EXITNETWORK);
    	}
    }
    return 0;
} /* send_cp_err */
  
/****************************************************************************
 * send_cp_err_ack -- send a checkpoint error packet to the PMD 
 ***************************************************************************/
int
send_cp_err_ack(int msgtype) 
{
    ackpack_t ack[1];
    ackpack32_t ack32[1];

    memset(ack, 0, sizeof(ackpack_t));
    memset(ack32, 0, sizeof(ackpack32_t));
    ack->magicvalue = MAGICACK;
    ack->acktype = ACKNORM;
    ack->data = msgtype;
    if (!pmd64)
    {
	convertackfrom64to32 (ack, ack32);
    	if (-1 == writesock(mysys->sock, (char*)ack32, sizeof(ackpack32_t))) {
        	EXIT(EXITNETWORK);
    	}
    }
    else
    {
    	if (-1 == writesock(mysys->sock, (char*)ack, sizeof(ackpack_t))) {
        	EXIT(EXITNETWORK);
    	}
    }
    return 0;
} /* send_cp_err */
  
/****************************************************************************
 * sendnoop -- send a noop packet to the RMD if no data in writelog 
 ***************************************************************************/
int
sendnoop(void) 
{
    headpack_t header[1];
    headpack32_t header32[1];

    memset(header, 0, sizeof(header));
    memset(header32, 0, sizeof(header32));
    header->magicvalue = MAGICHDR;
    header->cmd = CMDNOOP;
    header->ackwanted = 1;
    if (!rmd64)
    {
	converthdrfrom64to32 (header, header32);
    	if (-1 == writesock(mysys->sock, (char*)header32, sizeof(headpack32_t)))
   	{ 
      		EXIT(EXITNETWORK);
    	}
    }
    else
    {
    	if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t)))
	{ 
        	EXIT(EXITNETWORK);
    	}
    }

    return 0;
} /* sendnoop */
  
/****************************************************************************
 * send_cp_on -- write checkpoint-start entry to BAB
 ***************************************************************************/
int
send_cp_on(int lgnum) 
{
    group_t *group;
    stat_buffer_t sb;
    char msgbuf[DEV_BSIZE];
    int maxtries;
    int rc;
    int i;

    group = mysys->group;
    memset(&sb, 0, sizeof(stat_buffer_t));

    sprintf(msgbuf, "%s|%s", MSG_CPON, "");
    sb.lg_num = lgnum;
    sb.len = sizeof(msgbuf);
    sb.addr = (ftd_uint64ptr_t)(ulong)msgbuf;

    maxtries = 10;
    for (i = 0; i < maxtries; i++) {
        rc = FTD_IOCTL_CALL(group->devfd, FTD_SEND_LG_MESSAGE, &sb);
        if (rc != 0) {
            if (errno == EAGAIN) {
                usleep(200000);
                continue;
            }
            reporterr(ERRFAC, M_SENDLGMSG, ERRCRIT, get_error_str(errno));
            EXIT(EXITANDDIE);
        } else {
            break;
        }
    }
    return 0;
} /* send_cp_on */

/****************************************************************************
 * send_cp_off -- write checkpoint-stop entry to BAB
 ***************************************************************************/
int
send_cp_off(int lgnum) 
{
    group_t *group;
    stat_buffer_t sb;
    char msgbuf[DEV_BSIZE];
    int maxtries;
    int rc;
    int i;

    group = mysys->group;
    if (GET_LG_JLESS(mysys)) {
       ftd_dev_t_t pass_lgnum = lgnum;

       FTD_IOCTL_CALL(mysys->ctlfd, FTD_UPDATE_HRDBS, &pass_lgnum);
       flush_bab(mysys->ctlfd, lgnum);
       group->offset = 0;
    }

    memset(&sb, 0, sizeof(stat_buffer_t));

    sprintf(msgbuf, "%s|%s", MSG_CPOFF, "");
    sb.lg_num = lgnum;
    sb.len = sizeof(msgbuf);
    sb.addr = (ftd_uint64ptr_t)(ulong)msgbuf;

    maxtries = 10;
    for (i = 0; i < maxtries; i++) {
        rc = FTD_IOCTL_CALL(group->devfd, FTD_SEND_LG_MESSAGE, &sb);
        if (rc != 0) {
            if (errno == EAGAIN) {
                usleep(200000);
                continue;
            }
        } else {
            break;
        }
        reporterr(ERRFAC, M_SENDLGMSG, ERRCRIT, get_error_str(errno));
        EXIT(EXITANDDIE);
    }
    return 0;
} /* send_cp_off */

/****************************************************************************
 * sendkill -- send an kill ACK from RMD to the PMD
 ***************************************************************************/
int
sendkill (int fd)
{
    headpack_t header[1];
    ackpack_t ack[1];

    memset(header, 0, sizeof(headpack_t));
    memset(ack, 0, sizeof(ackpack_t));
    ack->data = ACKKILLPMD;
    sendack(fd, header, ack);

    return 0;
} /* sendkill */

/****************************************************************************
 * send_no_lic_msg -- send a no-license message from RMD to PMD
 ***************************************************************************/
int
send_no_lic_msg (int fd)
{
    headpack_t header[1];
    ackpack_t ack[1];

    memset(header, 0, sizeof(headpack_t));
    memset(ack, 0, sizeof(ackpack_t));
    ack->data = ACKNOLICENSE;
    sendack(fd, header, ack);

    return 0;
} /* send_no_lic_msg */

/****************************************************************************
 * sendack -- send an ACK from RMD to the PMD
 ***************************************************************************/
int
sendack (int fd, headpack_t *header, ackpack_t *ack)
{
    ackpack_t lack;
    ackpack32_t lack32;
    int rc;
  
    memset(&lack, 0, sizeof(ackpack_t));
    memset(&lack32, 0, sizeof(ackpack32_t));

    /* -- send the ACK */
    lack.acktype = ACKNORM;
    lack.magicvalue = MAGICACK;
    lack.devid = header->devid;
    lack.ts = header->ts;
    lack.ackoff = ack->ackoff;
    lack.data = ack->data;
    lack.mirco = ack->mirco;
    lack.lgsn = ack->lgsn;
    lack.bab_oflow = ack->bab_oflow;
    lack.write_ackoff = ack->write_ackoff;

    if ((lack.data != ACKNOOP) || (ftd_debugFlag & FTD_DBG_FLOW3)) {
        ftd_trace_flow(FTD_DBG_SOCK,
                "ack->acktype = %d, ack->data = %llu, ack->lgsn = %d\n",
                lack.acktype, lack.data, lack.lgsn);
    }
    if ((!pmd64) || (!rmd64))
    { 
	rmd64 = 1;
	convertackfrom64to32 (&lack, &lack32);
    	rc = writesock(fd, (char*)&lack32, sizeof(ackpack32_t));
    }
    else
    {
    	rc = writesock(fd, (char*)&lack, sizeof(ackpack_t));
    }

    return rc;
} /* sendack */

/****************************************************************************
 * senderr -- send an ERR to the PMD
 ***************************************************************************/
int
senderr (int fd, headpack_t *header, u_long data, u_long class, char *name, char *msg)
{
    ackpack_t ack;
    ackpack32_t ack32;
    errpack_t err;

    skosh.tv_sec = 10;
    skosh.tv_usec = 0;

    /* -- fake sending of the ack if the unit isn't the socket */
    if (NULL != mysys && 0 == mysys->isconnected) {
        return 1;
    }
    /* -- send the ACK */
    ack.magicvalue = MAGICACK;
    ack.acktype = ACKERR;
    ack.ts = header->ts;
    ack.data = data;
    if (!pmd64)
    {
	convertackfrom64to32 (&ack, &ack32);
    	if (-1 == writesock(fd, (char*)&ack32, sizeof(ackpack32_t))) {
        	return -1;
    	}
    }
    else
    {
    	if (-1 == writesock(fd, (char*)&ack, sizeof(ackpack_t))) {
        	return -1;
    	}
    }
    err.magicvalue = MAGICERR;
    err.errcode = class;
    strcpy (err.errkey, name);
    if (strlen(msg) > 255) {
        msg[255] = '\0';
    }
    strcpy (err.msg, msg);
    (void) writesock(fd, (char*)&err, sizeof(errpack_t));
    ftd_trace_flow(FTD_DBG_ERROR, "senderr -- sent message over the socket\n=> %s\n", msg);
    (void) select (0, NULL, NULL, NULL, &skosh); 
    return 0;
} /* senderr */

/****************************************************************************
 * join_config_file_for_RMD (network analysis mode)
 ***************************************************************************/
static int
join_config_file_for_RMD( int lgnum, cfgfilepack_t *cfg_file )
{
	struct stat statbuf;
	char filename[128];
	int  cfg_fd;

    memset(cfg_file, 0, sizeof(cfg_file));

    /* Read in the cfg file */
	sprintf(filename, "%s/p%03d.cfg", PATH_CONFIG, lgnum);

    if( stat(filename, &statbuf) != 0 )
	    return( -2 );
    if( statbuf.st_size > sizeof(cfg_file->file_data) )
	    return( -3 );
	if ((cfg_fd = open(filename, O_RDONLY)) == -1) {
		return( -4 );
	}
	if (read(cfg_fd, cfg_file->file_data, statbuf.st_size) != statbuf.st_size) {
		close(cfg_fd);
		return( -5 );
	}
	close(cfg_fd);

    cfg_file->file_length = statbuf.st_size;

    return (1);
} /* join_config_file_for_RMD */

/****************************************************************************
 * sendversion -- send protocol version checking information
 ***************************************************************************/
int
sendversion (int fd, int network_analysis_mode)
{
    headpack_t header;
    headpack32_t header32;
    versionpack_t version;
    int response;
    ackpack_t ack;
	int lgnum;
    int i;
    int retval;

    memset(&header, 0, sizeof(header));
    memset(&header32, 0, sizeof(header32));
    memset(&version, 0, sizeof(version));
    memset (sizediff, 0, SIZE);

    /* -- fake out if this isn't going to the socket */
    if (!mysys->isconnected) {
        return (1);
    }
    if (fd != mysys->sock) {
        return (1);
    }
    /* -- get the system information */
    header.magicvalue = MAGICHDR;
    header.cmd = CMDVERSION;
    header.ackwanted = 1;
    header.devid = 0;
    header.len = 0;
    header.offset = 0;
    lgnum = header.data = cfgpathtonum(mysys->configpath);
	if( network_analysis_mode )
	{
	    header.data |= CMDVER_READ_RMD_CFG_FILE;
		join_config_file_for_RMD( lgnum, &(version.cfg_file) ); 
	}
    (void) time(&header.ts);
    strcpy (version.configpath, 
        &(othersys->configpath[strlen(othersys->configpath)-8]));
    (void) time(&version.pmdts);
    /* -- set default protocol version numbers */
    strcpy (pmdversionstring, "4.0.0");
    strcpy (rmdversionstring, "4.0.0");
    strcpy (version.version, pmdversionstring);
    /* -- process the version number put into the works by the Makefile */
#ifdef VERSION
    strcpy (pmdversionstring, VERSION);
    i = 0;
    /* -- eliminate beta, intermediate build information from version */
    while (pmdversionstring[i]) {
        if ((!(isdigit(pmdversionstring[i]))) && pmdversionstring[i] != '.') {
            pmdversionstring[i] = '\0';
            break;
        }
        i++;
    }
    strcpy (version.version, pmdversionstring);
    version.jless = GET_LG_JLESS(mysys);
#endif

    /* -- send header and version packets */
    pmd64 = 1;
    rmd64 = 0;
    converthdrfrom64to32 (&header, &header32); 
    if (-1 == writesock (fd, (char*)&header32, sizeof(headpack32_t))) return (-1);
    (void) time(&version.pmdts);
    if (-1 == writesock (fd, (char*)&version, sizeof(versionpack_t))) return (-1);
    while (1 != (response = checkresponse (fd, &ack))) {
        if (0 >= response) {
            EXIT(EXITANDDIE);
        }
    }
    /* -- read the version number of the RMD -- */
    retval = readsock (fd, rmdversionstring, (int)ack.data);
    if (retval <= 0) return (retval);
    rmdversionstring[(int)ack.data] = '\0';
    /* -- put version specific processing here */
    if (strncmp (rmdversionstring, "2.5.0.0", strlen(rmdversionstring))>=0)
    {
        rmd64 = 1;
    }
    else
    {
        rmd64 = 0;
    }
#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED   /* WR PROD4508 and HIST WR 38281 */
    /* WARNING: the following test would fail if we produce a release >= 2.10.x.y */
    do_check_if_RMD_mirrors_mounted = (strncmp (rmdversionstring, "2.7.0.0", strlen(rmdversionstring)) >= 0);
#endif

    RMD_version_known = 1;    /* WR 43770 */
    return (1);
} /* sendversion */

/****************************************************************************
 * sendfailover -- send fail over command to target master daemon
 ***************************************************************************/
int
sendfailover (int fd, int group_number, int boot_drive_migration, int shutdown_source, int keep_AIX_target_running)
{
    headpack_t header;
    headpack32_t header32;
    int response;
    ackpack_t ack;
    int fo_result;
    int retval;

    memset(&header, 0, sizeof(header));
    memset(&header32, 0, sizeof(header32));

    /* -- fake out if this isn't going to the socket */
    if (!mysys->isconnected) {
        return (1);
    }
    if (fd != mysys->sock) {
        return (1);
    }
    /* -- get the system information */
    header.magicvalue = MAGICHDR;
    header.cmd = CMDFAILOVER;
    header.ackwanted = 1; // Will be overwritten below if AIX root drive failover
    header.devid = 0;
    header.len = 0;
    header.offset = 0;
    header.data = group_number;
	if( boot_drive_migration )
	{
        header.data |= CMDFO_BOOT_DRIVE;
	}
	if( shutdown_source )
	{
        header.data |= CMDFO_SHUTDOWN_SOURCE;
	}
	if( keep_AIX_target_running )
	{
        header.data |= CMDFO_KEEP_AIX_TARGET_RUNNING;
	}

#if defined(_AIX)
    // On AIX, if this is a root drive failover, do not request for an acknowledge
	// because the target server will be rebooted before the response can be sent.
	if( boot_drive_migration )
	{
        header.ackwanted = 0;
	}
#endif
    (void) time(&header.ts);
    /* -- send header */
    pmd64 = 1;
    rmd64 = 0;
    converthdrfrom64to32 (&header, &header32); 
    if (-1 == writesock (fd, (char*)&header32, sizeof(headpack32_t))) return (-1);

#if defined(_AIX)
	if( boot_drive_migration )
	{
	    return( 1 ); // Return success status if AIX rootvg failover (no response expected)
	}
#endif

    // If not AIX boot drive failover, wait for the response
    while (1 != (response = checkresponse (fd, &ack))) {
        if (0 >= response) {
            exit( 1 );
        }
    }
    /* -- read the result -- */
    retval = readsock (fd, &fo_result, (int)ack.data);
    if (retval <= 0) return (retval);
    return (1);
} /* sendfailover */

/****************************************************************************
 * sendhandshake -- send a handshake message 
 ***************************************************************************/
int
sendhandshake (int fd, int* rmda_corrupted_journal_found)
{
    headpack_t header;
    headpack32_t header32;
    authpack_t auth;
    ackpack_t ack;
    time_t ts;
    u_long hostid;
    int response;
    int len;

    memset(&header, 0, sizeof(header));
    memset(&header32, 0, sizeof(header32));
    memset(&auth, 0, sizeof(auth));
    /* -- fake out if this isn't going to the socket */
    if (!mysys->isconnected) {
        return 1;
    }
    if (fd != mysys->sock) {
        return 1;
    }
    /* -- get the system information */
    time(&ts);
    len = 256;
    hostid = mysys->hostid;

    encodeauth(ts, mysys->name, hostid,IP, &auth.len, auth.auth);
    strcpy (auth.configpath, &(mysys->configpath[strlen(mysys->configpath)-8]));
    header.magicvalue = MAGICHDR;
    header.cmd = CMDHANDSHAKE;
    header.ts = ts;
    header.ackwanted = 1;
    header.devid = 0;
    header.len = 0;
    header.offset = 0;
    ack.mirco = 0;

    /* -- send header and auth packets */
    if (!rmd64)
    {
        converthdrfrom64to32 (&header, &header32);
    	if (-1 == writesock (fd, (char*)&header32, sizeof(headpack32_t))) return -1;
    }
    else
    {
    	if (-1 == writesock (fd, (char*)&header, sizeof(headpack_t))) return -1;
    }
    if (-1 == writesock (fd, (char*)&auth, sizeof(authpack_t))) return -1;
    while (1 != (response = checkresponse (fd, &ack))) {
        if (0 >= response) {
            EXIT(EXITANDDIE);
        }
    }
    if (_pmd_cpon != ack.data) {
        /*
         * mismatch checkpoint state between PMD&PSTORE and RMD
         * we want to trust RMD for now.
         */
        _pmd_cpon = ack.data;
    }
	/* WR PROD6443: check if an unclean shutdown has occurred on the RMD during a previous Full Refresh */
    check_commands_due_to_unclean_RMD_shutdown = (ack.mirco == UNCLEAN_SHUTDOWN_RMD);

    *rmda_corrupted_journal_found = (ack.mirco == RMDA_CORRUPTED_JOURNAL);
    
    return 1;
} /* sendhandshake */

/****************************************************************************
 * sendconfiginfo -- send a configuration of a remote mirror device for
 *                   verification
 ***************************************************************************/
int
sendconfiginfo (int fd)
{
    headpack_t header;
    headpack32_t header32;
    ackpack_t ack;
    rdevpack_t rdev;
    sddisk_t* sd;
    u_longlong_t locsize;
    int response;

    memset(&header, 0, sizeof(header));
    memset(&header32, 0, sizeof(header32));
    memset(&rdev, 0, sizeof(rdev));

    /* -- fake out if this isn't going to the socket */
    if (!mysys->isconnected || fd != mysys->sock) {
        return (1);
    }
    /* -- walk through devices, send believed mirror device, 
       for size of the device */
    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
        if (sd->devsize <= 0) {
                if (0 >= (sd->devsize = (u_longlong_t)disksize(sd->devname))) {  
                reporterr (ERRFAC, M_BADDEVSIZ, ERRCRIT, sd->devname);
                return(-1);
            }
        }
        locsize = sd->devsize;
        /* -- send the mirror device name to the remote system for verification */
        header.magicvalue = MAGICHDR;
        header.cmd = CMDCHKCONFIG;
        (void)time(&header.ts);
        header.ackwanted = 1;
        header.devid = sd->devid;
        header.len = 0;
        header.offset = 0;

        rdev.devid = sd->devid;
        rdev.dd_rdev = sd->dd_rdev;
        rdev.sd_rdev = sd->sd_rdev;

        (void) strcpy(rdev.path, sd->mirname);
        rdev.len = strlen(sd->mirname);
        if (!rmd64)
        {
	        converthdrfrom64to32 (&header, &header32);
        	if (-1 == (response = writesock (fd, (char*)&header32, 
            		sizeof(headpack32_t)))) return (response);
        }
        else
        {
        	if (-1 == (response = writesock (fd, (char*)&header, 
            		sizeof(headpack_t)))) return (response);
        }
        if (-1 == (response = writesock (fd, (char*)&rdev, 
            sizeof(rdevpack_t)))) return (response);
        while (1 != (response = checkresponse (fd, &ack))) {
            if (0 >= response) {
                EXIT(EXITANDDIE);
            }
        }
        /* -- check if remote device size is less than local data disk */
            if (locsize > (u_longlong_t)ack.data) {   
               reporterr (ERRFAC, M_MIR2SMAL, ERRWARN, mysys->configpath,
                  sd->devname, sd->mirname, locsize, (u_longlong_t)ack.data); 
               sd->devsize = (u_longlong_t) ack.data;
               sd->rsync.size = sd->devsize;
           } else if (locsize < (u_longlong_t)ack.data) {              
#if defined (HPUX) 
               if (ack.data != ULONG_LONG_MAX) {
#elif defined (_AIX) && (SYSVERS < 520)
               if (ack.data != ULONGLONG_MAX) {
#else
               if (ack.data != ULLONG_MAX) {
#endif 
                  reporterr (ERRFAC, M_MIR2LARGE, ERRINFO, mysys->configpath,
                    sd->devname, sd->mirname, locsize, (u_longlong_t)ack.data); 
                }
           }
        /* -- move on to the next device for this group */
    }
    return 1;
} /* sendconfiginfo */

/****************************************************************************
 * sendchunk -- send a journal chunk to the RMD
 ***************************************************************************/
int
sendchunk (int fd, group_t *group)
{
    struct iovec iov[2];
    headpack_t header;
    headpack32_t header32;
    static char *save_chunk_addr;
    int length;
    int rc;
    int i;
    int newgroupsize = group->size;

    memset(&header, 0, sizeof(headpack_t));
    memset(&header32, 0, sizeof(headpack32_t));

    /* calculate newgroupsize */
    if (!rmd64) {
    	for (i = 0; i < SIZE; i++) {
        	if (i == (SIZE - 1) || sizediff[i+1] == 0) {
                newgroupsize = group->size - sizediff[i];
                break;
        	}
    	}
    }
    /* create a header for the current entry */
    header.magicvalue = MAGICHDR;
    header.cmd = CMDWRITE;
    header.devid = 0;
    header.ts = group->ts;
    /* Sequencer: */
    Sequencer_Inc(&group->SendSequencer);
    // WI_338550 December 2017, implementing RPO / RTT
    update_RTT_based_on_SentSequencer(group);
    header.lgsn = group->SendSequencer;
    /* BAB overflow counter to send with data; when acknowledging, RMD returns
       this field; if it does not match the overflow counter at reception time,
       new overflows have occured and BAB has been cleared; then must not attempt
       to migrate data which has gone (WR 43926) */
    header.bab_oflow = BAB_oflow_counter;

    ftd_trace_flow(FTD_DBG_SEQCER, "sendchunk CMDWRITE %d lgsn = %d\n", ISSECONDARY(mysys), header.lgsn);
    

#ifdef CRASH_ON_BAD_SIZE
    if (newgroupsize & 0x7)
        *(char *)0 = 0;
#endif

    header.ackwanted = 1;
    header.compress = mysys->tunables.compression;
    header.decodelen = newgroupsize;

    save_chunk_addr = group->chunk;

    if (mysys->tunables.compression) {
        if ((length = newgroupsize) > compress_buf_len) {
            compress_buf_len = length + (length >> 1) + 1;
            ftdfree(compress_buf);
            compress_buf = (char*)ftdmalloc(compress_buf_len);  
        }
        /* predictor compression */
        group->len =
            compress((u_char*)group->chunk, (u_char*)compress_buf, 
                newgroupsize);
        group->chunk = compress_buf;
        if (newgroupsize) {
            compr_ratio =
                (float)((float)group->len/(float)newgroupsize);
        } else {
            compr_ratio = 1;
        }
    } else {
        group->len = newgroupsize;
    }
    header.len = group->len;

    ftd_trace_flow(FTD_DBG_SOCK,
            "writing header, magicvalue = %08x\n", header.magicvalue);
    ftd_trace_flow(FTD_DBG_SOCK,
            "writing %d bytes @ addr %d to socket\n", header.len, group->chunk);

    /* send the header and the data */
    if (!rmd64) {
        converthdrfrom64to32 (&header, &header32);
        iov[0].iov_base = (void*)&header32;
        iov[0].iov_len = sizeof(headpack32_t);
        iov[1].iov_base = (void*)group->chunk;
        iov[1].iov_len = header32.len;
    } else {
    	iov[0].iov_base = (void*)&header;
    	iov[0].iov_len = sizeof(headpack_t);
    	iov[1].iov_base = (void*)group->chunk;
    	iov[1].iov_len = header.len;
    }

    rc = net_write_vector(fd, iov, 2, ROLEPRIMARY);
    group->chunk = save_chunk_addr;
 
    return rc;
} /* sendchunk */

/****************************************************************************
* sendclearbits -- send CMDCLEARBITS token to RMD
***************************************************************************/
int
sendclearbits(void)
{
    headpack_t header[1];
    headpack32_t header32[1];

    memset(header, 0, sizeof(headpack_t));
    memset(header32, 0, sizeof(headpack32_t));
    header->magicvalue = MAGICHDR;
    header->cmd = CMDCLEARBITS;
    header->ackwanted = 1;
    if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
            EXIT (exitfail);
        }
    return 0;
} /* sendclearbits */

/****************************************************************************
 * checkresponse -- check if an ACK or ERR is pending to be read, if so, read
 *                  and process it (0=nothing waiting, -1=ERR, >0=ACK)
 ***************************************************************************/
int
checkresponse (int fd, ackpack_t* ack) 
{
    errpack_t err;
    int retval;
    int retries;
    ackpack32_t ack32[1];

    sddisk_t *sd;

    retries = 0;
    /* -- fake out networked ACK if not reading from the network */
    if (!mysys->isconnected) {
        return 1;
    }
    if (mysys->sock != fd) {
        return 1;
    }
    memset(ack, 0, sizeof(ackpack_t));
    memset(ack32, 0, sizeof(ackpack32_t));
    /* -- see if there is something waiting to be read */
#ifdef FIX_WR_43770
NOTE: this fix has side effects; it is turned off for the moment
    if( !RMD_version_known )    /* WR 43770 */
    {
       /* If we receive RMD messages before we got the RMD's version (which has been seen in the case of
          missing RMD config files message, we decode the data according to the most recent protocol,
          which should be true unless this PMD talks to an older version of RMD (PMD-RMD releases mismatch)
       */
       sprintf( debug_msg, "Receiving from RMD before RMD version known; defaulting to rmd64 compatible\n" );
       reporterr (ERRFAC, M_GENMSG, ERRINFO, debug_msg); 
    }
    if (RMD_version_known && !rmd64)   /* If RMD version unknown, default: rmd64 == 1;  WR 43770 */
#else
    if (!rmd64)
#endif
    {
    	retval = readsock (fd, (char*)ack32, sizeof(ackpack32_t));
    	if (retval <= 0) {
        	return retval;
    	}
	convertackfrom32to64 (ack32, ack);
    }
    else
    {
    	retval = readsock (fd, (char*)ack, sizeof(ackpack_t));
    	if (retval <= 0) {
        	return retval;
    	}
    }
    /* -- process the ACK packet */
    if (ack->magicvalue != MAGICACK) {
        reporterr (ERRFAC, M_ACKMAGIC, ERRCRIT, _pmd_configpath);
        return -1;
    }
    if (ack->acktype != ACKNOOP
    && ack->acktype != ACKNORM
    && ack->acktype != ACKERR) {
        reporterr (ERRFAC, M_ACKCMD, ERRCRIT, _pmd_configpath);
        return -1;
    }
 
    /* Check if RMD reports invalid license (WR 43583) */
    if( ack->data == ACKNOLICENSE )
    {
       sprintf( debug_msg, "Invalid License on RMD\n" );
       reporterr (ERRFAC, M_GENMSG, ERRCRIT, debug_msg);
       return -1;
    }
    if (ack->acktype == ACKNOOP) {
        return 2;
    }
    if (ack->acktype == ACKERR) {
        /* -- get error packet */
        retval = readsock (fd, (char*)&err, sizeof(errpack_t));
        if (retval < 0) return retval;
        if (retval == 0) {
            retries++;
            if (retries >= 20) {
                reporterr (ERRFAC, M_ERRMIS, ERRCRIT, _pmd_configpath);
                return -1;
            }
        }
        reporterr (ERRFAC, M_RMDERR, ERRWARN, err.msg);
        if (err.errcode == ERRINFO) {
            return 0;
        }
        return -1;
    } 

    return 1;
} /* checkresponse */

/****************************************************************************
 * readsock -- read from a socket (-1=err, 0=nothing pending, 1=OK)
 ***************************************************************************/
int
readsock (int fd, char* buf, int len)
{
    fd_set rset;
    time_t now, lastts = 0;
    int deltatime, elapsed_time;
    int connect_timeo;
    int sofar;
    int rc;
    int lgnum;

    skosh.tv_sec = 0L;
    skosh.tv_usec = 100000L;
 
    elapsed_time = 0;
    connect_timeo = 5;
 
    /* read the message */ 
    sofar = 0;
    while (len > 0) {
        FD_ZERO(&rset);
        FD_SET(fd, &rset);

#if defined(linux)
        skosh.tv_sec = 0L;
        skosh.tv_usec = 100000L;
#endif /* defined(linux) */
        rc = select(fd+1, &rset, NULL, NULL, &skosh);

        if (rc == -1) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        } else if (rc == 0 || !FD_ISSET(fd, &rset)) {
            /*
             *  nothing to read on socket
             *  continue unless network channel
             *  has been severed
             */

            time(&now);
            if (lastts > 0) {
                deltatime = now - lastts;
            } else {
                deltatime = 0;
            }
            elapsed_time += deltatime;

            if (elapsed_time > max_elapsed_time) {
                /*
                 * insert this 'if' so that a non-lg-daemon caller
                 * will not try to test the link - it causes seg fault 
                 * when trying to access null sys structs
                 *
                 * mysys and othersys are only valid in pmd, rmd
                 *
                 */ 
                if (mysys && othersys) {
                    if (net_test_channel(connect_timeo) <= 0) {
                        reporterr (ERRFAC, M_NETTIMEO, ERRWARN,
                            mysys->name, othersys->name);
                        if( ISSECONDARY(mysys) )
                        {
                            if( remove_FR_watchdog_if_PMD_disconnets )
							{
                                lgnum = cfgpathtonum(mysys->configpath);
                                remove_unclean_shutdown_file( lgnum );
							}
                        }
                        EXIT(EXITNETWORK);
                    }
                } else {
                   /* do we need to do something here ? */
                   /* for now just keep trying */
                }
            }
            /* try again */
            elapsed_time = 0;
            lastts = now;
            continue;
        }
        elapsed_time = lastts = 0;

        sofar = read(fd, buf, len);
        if (sofar > 0) {
            len -= sofar;
            buf += sofar;
        } else if (sofar == -1) {
            /* -- real errors, report error and return */
            switch (errno) {
            case EFAULT:
            case EIO:
            case EISDIR:
            case ENOLINK:
            case ENXIO:
                reporterr (ERRFAC, M_SOCKRERR, ERRWARN, argv0, strerror(errno));
                return (-1);
            default:
#if defined(linux)
                skosh.tv_sec = 0L;
                skosh.tv_usec = 100000L;
#endif /* defined(linux) */
                /* -- soft errors, sleep, then retry the read -- */
                (void) select (0, NULL, NULL, NULL, &skosh);
            }
        } else {
            goto netbroke;
        }
    }

    return 1;

netbroke:

    /*
     * insert this 'if' so that a non-lg-daemon caller will not
     * report connect broken to peer - it causes seg fault 
     * when trying to access null sys struct
     *
     * mysys and othersys are only valid in pmd, rmd
     *
     */ 
    if (othersys && mysys) {
        reporterr (ERRFAC, M_NETBROKE, ERRWARN, argv0, othersys->name,"zero length read from socket");
        lgnum = cfgpathtonum(mysys->configpath);
        if (ISPRIMARY(mysys))
        {
           /* WR 43222: if the RMD was stopped (target crash, reboot (hard or soft), killdtcmaster, etc.), set state to go
              to tracking before exit. Problem encountered in the porting to SuSE Linux. We have to determine 
              if this problem was present on other platforms or if this is specific fo Linux
           */
            // Based on the analysis in WR PROD00006836, it may be safe and more efficient to leave the PMD in the mode it's currently in
            // for at least some scenarios, but because there might be unforeseen scenarios where it would not be safe, we'll bring it in tracking mode.
           if( !_net_bandwidth_analysis )
		   {
	           clearmode(lgnum, 0);
		   }
		   else
           // If in network bandwidth analysis mode, remove the fictitious config file
		   {
		       remove_fictitious_cfg_files( lgnum, 1 );
		   }

        /* WR PROD6443: we no longer call adjust_FRefresh_offset_backward(); if an unclean 
           Full Refresh interruption has occurred on the RMD (ex.: crash, power failure, etc) 
           this is now reported by the RMD upon initial handshake; if clean exit on RMD (not a crash),
           a Resumed Full Refresh is now permitted (launchrefresh -r), or a Checksum Refresh as well.
		*/
        }
        if( ISSECONDARY(mysys) )
        {
           /* WR 43376: clean exit of RMD upon detecting PMD disconnection (not an RMD crash);
              1) if we started in a clean state (no Full Refresh left incomplete in previous run),
              remove unclean-shutdown watchdog file; note: we do this even if a Full Refresh
              is currently in progress, because this state here may be caused by a killrefresh on the source side;
              since the Full Refresh interrupt (if applicable) is caused by the PMD, it will be the PMD's
              responsibility (source) to relaunch it;
              2) if a Full Refresh has been detected incomplete upon start of RMD (watchdog was there from
              previous run), do not remove the watchdog upon PMD disconnection; if a Full Refresh has been done
              in the current run, the watchdog will have been removed at the end of the Refresh
           */
           if( remove_FR_watchdog_if_PMD_disconnets )
              remove_unclean_shutdown_file( lgnum );

            // If in network bandwidth analysis mode, remove the fictitious config file
			if( _net_bandwidth_analysis )
			{
		        remove_fictitious_cfg_files( lgnum, 0 );
			}
        }

        EXIT(EXITNETWORK);
    }

    return -1;

} /* readsock */

/****************************************************************************
 * netread -- read a structure from a network socket (-1=err, 
 *            0=nothing pending, 1=OK)
 ***************************************************************************/
int
netread (int fd, char* buf, int len)
{
    int sofar;
    struct pollfd fds[1];
    u_long nfds;
    int timeout;
    int pollretval;
    int count;
    ackpack_t noopack;
    int retval;
    int onesec;

    skosh.tv_sec = 0L;
    skosh.tv_usec = 50000L;
    nfds = 1L;
    timeout = 50;
    fds[0].fd = fd;
    fds[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLERR |
        POLLHUP | POLLNVAL;
    fds[0].revents = 0;
    noopack.magicvalue = MAGICACK;
    noopack.acktype = ACKNOOP;
    noopack.ts = (time_t)0;
    noopack.data = 0L;

    /* -- see if there is something waiting to be read */
    /* -- read the message */ 
    sofar = 0;
    count = 0;
    if (timeout == 0) {
        onesec = 1;
    } else {
        onesec = ((1000 / timeout) > 0) ? (1000 / timeout) : 1 ;
    }
    while (len > 0) {
        while (0 == (pollretval = poll (fds, nfds, timeout))) {
            count++;
            if (count >= 600) {
                /* -- send a gratuitous unsolicited no-op ACK to test connect */
                count = 0;
                retval = writesock(fd, (char*)&noopack, sizeof(ackpack_t));
                if (-1 == retval){
                    reporterr (ERRFAC, M_SOCKWERR, ERRWARN,
                        argv0, strerror(errno));
                }
            }
        }
        count = 0;
        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            if (errno == 0 || errno == ENOENT) {
                reporterr (ERRFAC, M_RMDEXIT, ERRINFO, argv0);
            } else {
                reporterr (ERRFAC, M_SOCKRERR, ERRWARN, argv0, strerror(errno));
            }
            EXIT(EXITANDDIE);
        }
        sofar = read(fd, buf, len);
        if (sofar == -1) {
            /* -- real errors, report error and die */
            if (errno == EFAULT || errno == EIO || errno == EISDIR || 
                errno == ENOLINK || errno == ENXIO) {
                reporterr (ERRFAC, M_SOCKRERR, ERRWARN, argv0, strerror(errno));
                EXIT(EXITANDDIE);
            } else {
                /* -- soft errors, sleep, then retry the read -- */
                (void) select (0, NULL, NULL, NULL, &skosh);
            }
#if defined(linux)
            skosh.tv_sec = 0L;
            skosh.tv_usec = 50000L;
#endif /* defined(linux) */
            continue;
        } 
        if (sofar == 0) {
            reporterr (ERRFAC, M_NETBROKE, ERRWARN, argv0, othersys->name, "");
            EXIT(EXITNETWORK);
        }
        len -= sofar;
        buf += sofar;
#if defined(linux)
        skosh.tv_sec = 0L;
        skosh.tv_usec = 50000L;
#endif /* defined(linux) */
    }

    return 1;
} /* netread */

/****************************************************************************
 * checksock -- check the given file descriptor for I/O pending 
 ***************************************************************************/
int
checksock (int fd, int op, struct timeval *seltime)
{
    ackpack_t noopack;
    fd_set fds;
    int rc = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    if (op == 1) {
        /* see if descriptor can be written to */
        rc = select(fd+1, NULL, &fds, NULL, seltime);
    } else if (op == 0) {
        /* see if there is something waiting to be read */
        rc = select(fd+1, &fds, NULL, NULL, seltime);
    } else if (op == 2) {
        /* see if connection is alive */
        noopack.magicvalue = MAGICACK;
        noopack.acktype = ACKNOOP;
        noopack.ts = (time_t)0;
        noopack.data = 0L;
        rc = writesock(fd, (char*)&noopack, sizeof(ackpack_t));
        if (rc <= 0) {
            return -1;
        } else {
            return 1;
        }
    }
    if (rc <= 0 || !FD_ISSET(fd, &fds)) {
        return 0;
    }

    return 1;
} /* checksock */

/****************************************************************************
 * writesock -- write a structure to a socket (-1=err, 1=OK)
 ***************************************************************************/
int
writesock (int fd, char* buf, int len)
{
    int sofar;
    headpack_t *h = NULL;

    skosh.tv_sec = 0L;
    skosh.tv_usec = 500000L;
    sofar = 0;
    if (len > sizeof(h->magicvalue) &&
        (h = (headpack_t *)buf,
         h->magicvalue == MAGICHDR && h->cmd != CMDNOOP)) {
        ftd_debugf("%d, %p, %d", "%s(%d)",
            fd, buf, len,
            get_cmd_str(h->cmd),
            h->len);
    }
    while (len > 0) {
        /* We have to be connected in order to do this */
        sofar = write(fd, buf, len);
        if (sofar == -1) {
            /* -- real errors, report error and return */
            if (errno == EFAULT || errno == EIO || errno == EPIPE || 
                errno == ENXIO || errno == EBADF || errno == ERANGE ||
                errno == EFBIG || errno == ENOSPC) {
                reporterr (ERRFAC, M_SOCKWERR, ERRWARN, argv0, strerror(errno));
                return (-1);
            } else {
                /* -- soft errors, sleep, then retry the write -- */
                (void) select(0, NULL, NULL, NULL, &skosh);
            }
#if defined(linux)
            skosh.tv_sec = 0L;
            skosh.tv_usec = 500000L;
#endif /* defined(linux) */
            continue;
        }
        len -= sofar;
        buf += sofar;
#if defined(linux)
        skosh.tv_sec = 0L;
        skosh.tv_usec = 500000L;
#endif /* defined(linux) */
    }
    return (1);
} /* writesock */

/*
 * net_test_channel -- try to connect to other end
 */
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif
int
net_test_channel(int connect_timeo)
{
    struct hostent *hp;
    struct sockaddr_in servaddr;
    fd_set rset, wset;
    struct timeval tval;
    const char *const who = "net_test_channel";
    char buf[LINE_MAX];
    int rc = 1;
    int  err_value = 0;
    int  * error = &err_value;
    int flags, n = -1, nsec;
    int sock = -1;
    int hsecPort;
#if !defined(FTD_IPV4)
    struct addrinfo hints,*tmp,*ai,*res;
    struct sockaddr_storage saddr;
    struct in6_addr addr_6;
    struct in_addr addr_4;
    int rec = -1;
    int retval = -1;
    int found = -1;
    char tempport[30];
    char addr[INET6_ADDRSTRLEN];
    char buf6[INET_ADDRSTRLEN];
    char sysname[4096];
#endif
#if defined(_AIX) || defined(linux)
    size_t len = sizeof(*error);
#else /* defined(_AIX) */
    int len = sizeof(*error);
#endif /* defined(_AIX) */

#if defined(FTD_IPV4)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        /* can't create a socket - maybe we'll get it next time around */
        reporterr(ERRFAC, M_INTERNAL, ERRWARN,
            (sprintf(buf, "%s: socket(AF_INET, SOC_STREAM, 0) failed %s",
                          who, get_error_str(errno)), buf));
        err_value = errno;
        goto EXIT;
    }
#else
      strcpy(sysname, othersys->name);
      sprintf(tempport,"%d",othersys->secondaryport);
      memset(&hints, 0, sizeof(hints));

      rec = inet_pton(AF_INET, sysname, &saddr);
      if (rec == 1)    /* If a valid IPv4 numeric address,then no IP address resolution is required */
      {
         hints.ai_family = AF_INET;
         hints.ai_flags = AI_NUMERICHOST;
         hints.ai_socktype = SOCK_STREAM;
      }
      else
      {
         rec = inet_pton(AF_INET6, sysname, &saddr);
         if (rec == 1) /* If a valid IPv6 numeric address,then no IP address resolution is required */
         {
            hints.ai_family = AF_INET6;
            hints.ai_flags = AI_NUMERICHOST;
            hints.ai_socktype = SOCK_STREAM;
         }
      }
     /* If machine name is passed from the configuration file, we require name resolution*/
     if (rec != 1){
     memset(&hints, 0, sizeof(struct addrinfo));
     hints.ai_flags = AI_CANONNAME;
     hints.ai_socktype = SOCK_STREAM;  
     }

     retval = getaddrinfo(sysname, (const char*)tempport, &hints, &tmp);
     if (retval != 0) {
        reporterr(ERRFAC, M_NAMRESOLVFAIL, ERRCRIT, gai_strerror(retval));
        err_value = -3;
        goto EXIT;
     }
     ai = tmp; 
     while (tmp) {
        sock = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
        if (sock >= 0) {
           break;
        }
        else {
           tmp = tmp->ai_next;
           if (tmp == NULL) {
              reporterr(ERRFAC,M_INTERNAL, ERRWARN,\
                 (sprintf(buf, "%s: socket(tmp->ai_family, SOC_STREAM, 0) failed %s",who, get_error_str(errno)),buf));
              err_value = errno;
              freeaddrinfo(ai); 
              goto EXIT; 
           }
        }
     }
    freeaddrinfo(ai);
#endif
     hsecPort = htons(othersys->secondaryport);
     if ((flags = fcntl(sock, F_GETFL, 0)) < 0 ||
          fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
          reporterr(ERRFAC, M_INTERNAL, ERRWARN,
            (sprintf(buf, "%s: fcntl(%d, ..) failed %s",
                          who, sock, get_error_str(errno)), buf));
          err_value = errno;
          goto EXIT;
      }
#if defined(FTD_IPV4)
      memset ((char*)&servaddr, 0, sizeof(servaddr));
      servaddr.sin_family = AF_INET;
      servaddr.sin_port = hsecPort;

      /*
       * if the other system host name is empty or gethostbyname fails
       * then try gethostbyaddr
       */
      hp = 0;
      if (othersys->ip != 0L) {
#if defined(HPUX) || defined(_AIX) || defined(linux)
         hp = gethostbyaddr((char*)&othersys->ip, sizeof(u_long), AF_INET);
#elif defined(SOLARIS)  /* 1LG-ManyDEV */
         GETHOSTBYADDR_MANYDEV((char*)&othersys->ip, sizeof(u_long), AF_INET, hp, dummyfd);
#endif
      } else if (strlen(othersys->name) > 0) {
#if defined(HPUX) || defined(_AIX) || defined(linux)
        hp = gethostbyname(othersys->name);
#elif defined(SOLARIS)  /* 1LG-ManyDEV */
        GETHOSTBYNAME_MANYDEV(othersys->name, hp, dummyfd);
#endif
      }
      if (hp == NULL) {
         reporterr(ERRFAC, M_INTERNAL, ERRWARN,
            (sprintf(buf, "%s: gethostbyXXX(..) failed h_errno %d", who, h_errno), buf));
         err_value = h_errno;
         goto EXIT;
      }
      memcpy((caddr_t)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
      n = connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr));
#else
      memset(&hints, 0, sizeof(hints));

      rec = inet_pton(AF_INET, sysname, &saddr);
      if (rec == 1)    /* If a valid IPv4 numeric address,then no IP address resolution is required */
      {
         hints.ai_family = AF_INET;
         hints.ai_flags = AI_NUMERICHOST;
         hints.ai_socktype = SOCK_STREAM;
      }
      else
      {
         rec = inet_pton(AF_INET6, sysname, &saddr);
         if (rec == 1) /* If a valid IPv6 numeric address,then no IP address resolution is required */
         {
            hints.ai_family = AF_INET6;
            hints.ai_flags = AI_NUMERICHOST;
            hints.ai_socktype = SOCK_STREAM;
         }
      }
      /* If machine name is passed from the configuration file, we require name resolution*/
      if (rec != 1){
      memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_flags = AI_CANONNAME;
      hints.ai_socktype = SOCK_STREAM;
      }
      retval = getaddrinfo(sysname, (const char*)tempport, &hints, &res);
      if (retval != 0){
         reporterr(ERRFAC, M_NAMRESOLVFAIL, ERRCRIT, gai_strerror(retval));
         err_value = -3;
         goto EXIT;
      }
      ai = res; 
      while (res) {
         if (connect(sock, res->ai_addr, res->ai_addrlen) == 0) {
            break;
         }
         else {
            res= res->ai_next;
            if (res == NULL) {
               found = 0;
               n = 1;
               break;
            }
         }
      }
      freeaddrinfo(ai);
      if (found == 0) {
          memset(&hints, 0, sizeof(hints));

          rec = inet_pton(AF_INET, sysname, &saddr);
          if (rec == 1)    /* If a valid IPv4 numeric address,then no IP address resolution is required */
          {
            hints.ai_family = AF_INET;
            hints.ai_flags = AI_NUMERICHOST;
            hints.ai_socktype = SOCK_STREAM;
          }
         else
          {
             rec = inet_pton(AF_INET6, sysname, &saddr);
          if (rec == 1) /* If a valid IPv6 numeric address,then no IP address resolution is required */
          {
            hints.ai_family = AF_INET6;
            hints.ai_flags = AI_NUMERICHOST;
            hints.ai_socktype = SOCK_STREAM;
           }
          }
         /* If machine name is passed from the configuration file, we require name resolution*/
         if(rec !=1){
         memset(&hints, 0, sizeof(hints));
         hints.ai_flags = AI_CANONNAME;
         hints.ai_socktype = SOCK_STREAM;
         }
         retval = getaddrinfo(sysname,(const char*)tempport, &hints, &tmp);
         if (retval != 0) {
            reporterr(ERRFAC,M_NAMRESOLVFAIL,ERRWARN, gai_strerror(retval));
	    err_value = -3;
            goto EXIT;
         }
         memset(buf, 0 ,sizeof(buf6));
         inet_ntop (AF_INET,&((struct sockaddr_in*)tmp->ai_addr)->sin_addr,buf6,sizeof(buf6));
         if (inet_pton (AF_INET,buf6,&addr_4) != 0) {
            IPv4_to_IPv6(&addr_4,&addr_6);
            inet_ntop (AF_INET6, &addr_6, buf6, INET_ADDRSTRLEN);
         }
         freeaddrinfo(tmp);
         memset(&hints, 0, sizeof(hints));
         hints.ai_flags = AI_NUMERICHOST;
         hints.ai_socktype = SOCK_STREAM;
         retval = getaddrinfo(buf6, (const char*)tempport, &hints, &tmp);
         if (retval != 0) {
            reporterr(ERRFAC,M_SNAMRESOLVFAIL,ERRWARN);
	    err_value = -3;
            goto EXIT;
         }

         n = connect(sock, tmp->ai_addr, tmp->ai_addrlen);
         freeaddrinfo(tmp);
     }
#endif
#if defined(FTD_IPV4)
     if (n < 0 )  {
        // PROD9125: check also for "Operation alredy in progress" in addition to "Operation now in progress"
        if( (errno != EINPROGRESS) && (errno != EALREADY) ) {
           char addr[INET_ADDRSTRLEN];
           err_value = errno;
	   sprintf(buf, "%s: connect(%d, %s:%u) fails %s", who,
			sock,
#if defined(HPUX) && (SYSVERS == 1100)
			inet_ntoa(servaddr.sin_addr),
#else
			inet_ntop(servaddr.sin_family,
				  (const void *)&servaddr.sin_addr,
				  addr, sizeof(addr)),
#endif
			 ntohs(servaddr.sin_port),
			 get_error_str(err_value));
           reporterr(ERRFAC, M_INTERNAL, ERRWARN, buf);
           rc = -1;
           goto EXIT;
        }
     } else if (n == 0) {
        /* connect completed immediately */
        goto EXIT;
     }
#else
     if (n < 0) {
        // PROD9125: check also for "Operation alredy in progress" in addition to "Operation now in progress"
        if( (errno != EINPROGRESS) && (errno != EALREADY) ) {
          memset(&hints, 0, sizeof(hints));

          rec = inet_pton(AF_INET, sysname, &saddr);
          if (rec == 1)    /* If a valid IPv4 numeric address,then no IP address resolution is required */
          {
            hints.ai_family = AF_INET;
            hints.ai_flags = AI_NUMERICHOST;
            hints.ai_socktype = SOCK_STREAM;
          }
         else
          {
          rec = inet_pton(AF_INET6, sysname, &saddr);
          if (rec == 1) /* If a valid IPv6 numeric address,then no IP address resolution is required */
          {
            hints.ai_family = AF_INET6;
            hints.ai_flags = AI_NUMERICHOST;
            hints.ai_socktype = SOCK_STREAM;
          }
          }
          /* If machine name is passed from the configuration file, we require name resolution*/
          if(rec != 1){
          memset(&hints, 0, sizeof(struct addrinfo));
          hints.ai_flags = AI_CANONNAME;
          hints.ai_socktype = SOCK_STREAM;
          }
           retval = getaddrinfo(sysname, (const char*)tempport, &hints, &tmp);
          if (retval != 0){
              reporterr(ERRFAC, M_NAMRESOLVFAIL, ERRCRIT, gai_strerror(retval));
              err_value = -3;
              goto EXIT;
          }
           err_value = errno;
          if (tmp->ai_family == AF_INET6) {
              inet_ntop(tmp->ai_family,&((struct sockaddr_in6 *)tmp->ai_addr)->sin6_addr, addr, sizeof(addr));
           }
           else if (tmp->ai_family == AF_INET) {
              inet_ntop(tmp->ai_family,&((struct sockaddr_in *)tmp->ai_addr)->sin_addr, addr, sizeof(addr));
           }
           reporterr(ERRFAC, M_INTERNAL, ERRWARN,
                     (sprintf(buf, "%s: connect(%d, %s:%u) fails %s", who,
                      sock,addr,ntohs(hsecPort),get_error_str(err_value)), buf));
           rc = -1;
           freeaddrinfo(tmp);
           goto EXIT;
        }
        /*
         * No connection, errno == EINPROGRESS or EALREADY
         */
     } else if (n == 0) {
        /* connect completed immediately */
        goto EXIT;
     }
#endif
     nsec = connect_timeo;

     FD_ZERO(&rset);
     FD_SET(sock, &rset);
     wset = rset;
     tval.tv_sec = nsec;
     tval.tv_usec = 0;
        
     if ((n = select(sock+1, &rset, &wset, NULL, nsec ? &tval : NULL)) == 0) {
        err_value = ETIMEDOUT;
        rc = 0;
        goto EXIT;
     }
     if (FD_ISSET(sock, &rset) || FD_ISSET(sock, &wset)) {
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, error, &len) < 0) {
           reporterr(ERRFAC, M_INTERNAL, ERRWARN,
               (sprintf(buf, "%s: getsockopt(%d, SOL_SOCKET, SO_ERROR, ..) failed %s",
                             who, sock, get_error_str(errno)), buf));
           /* Solaris pending error */
           err_value = errno;
           rc = -1;
        }
     } else
        rc = -1;

EXIT:
    if (sock >= 0)
        close(sock);
    if (err_value)
        errno = err_value;
    return rc;
} /* net_test_channel */
/****************************************************************************
 * process_acks -- PMD - process ACKs and do the right thing 
 ***************************************************************************/
int 
process_acks(void)
{
    struct timeval seltime[1];
    headpack_t header[1];
    headpack32_t header32[1];
    ackpack_t ack[1];
    rsync_t rrsync[1];
    rsync32_t rrsync32[1];
    rsync_t *rsync = NULL;
    rsync_t *ackrsync;
    group_t *group;
    sddisk_t* sd = NULL;
    static int buflen = 0;
    static char *buf = NULL;
    static int zerolen = 0;
    static char *zerobuf = NULL;
    static int firsttime = 1;
    static time_t ts, lastts;
    int deltatime;
    int datalen;
    int length;
    int ret;
    int lgnum;
    int cnt;
    int connect_timeo;
    int ackcount;
    int rc;
    int i;
    int blksize;
    char ps_name[MAXPATHLEN];
    char group_name[MAXPATHLEN];
    int ackbytes;
    int can_migrate_data;

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    seltime->tv_sec = 0;
    seltime->tv_usec = 0;
   
    (void)time(&ts);
    ackcount = 0;

    if (firsttime) {
        lastts = ts;
        elapsed_time = 0;
        firsttime = 0;
    }
    deltatime = ts - lastts;
    connect_timeo = 5;

    while (1) {
#if defined(linux)    /* bug54 fix */
        seltime->tv_sec = 0;
        seltime->tv_usec = 0;
#endif
        ret = checksock(mysys->sock, 0, seltime);
        if (ret == 0) {
            elapsed_time += deltatime;
            if (elapsed_time > max_elapsed_time) {
                if (net_test_channel(connect_timeo) == 0) {
                    reporterr (ERRFAC, M_NETTIMEO, ERRWARN,
                        mysys->name, othersys->name);
                    EXIT(EXITNETWORK);
                } else {
                    /* channel still there - rmd must be busy! */
                    elapsed_time = 0;
                }
            }
            lastts = ts;
            return 0;
        }
        elapsed_time = 0;
        lastts = ts;

        if (ackcount++ >= 20) {
            return ackcount;
        }
        rc = checkresponse(mysys->sock, ack);
        ftd_trace_flow(FTD_DBG_FLOW3,
                "checkresponse rc = %d, ack->acktype = %d, ack->data = %llu\n",
                rc, ack->acktype, ack->data);
        if (rc == -1) {
            EXIT(EXITANDDIE);
        }
        if (ack->data == ACKNOOP) {
            ftd_trace_flow(FTD_DBG_FLOW3, "ACKNOOP received\n");
            // WI_338550 December 2017, implementing RPO / RTT
            // Windows increments the sequencer and updates RTT data, but UNIX does not increment the sequencer here
            // So it is kept as is for the moment and we don't call AckedSequencerRTTupdate()
            // This may have to be reviewed while testing
            return ackcount;
        }
        if (ack->acktype == ACKNORM) {
            if (ack->data == ACKBFDDELTA
            || ack->data == ACKRFD
            || ack->data == ACKRFDF) {
                if ((sd = get_lg_dev(group, ack->devid)) == NULL) {
                    reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, ack->devid);
                    EXIT(EXITANDDIE);
                    return ackcount;
                }
                rsync = &sd->rsync;
                ackrsync = &sd->ackrsync;
            }
#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED
	    /* WR PROD4508 and HIST WR 38281: check if RMD reports a mounted target device */
	    else if( ack->data == RFDF_TGT_MNTED )
	    {
               /* Cannot perform Refresh while RMD target device is mounted; must unmount first */
               reporterr (ERRFAC, M_NACKTGTMOUNTED, ERRCRIT );
               EXIT(EXITANDDIE);
            }
	    /* End of WR PROD4508 and HIST WR 38281 */
#endif
	    /* WR 43376: check if RMD reports an unclean-shutdown state */
	    else if( ack->data == UNCLEAN_SHUTDOWN_RMD )
	    {
			   /* WR PROD6443: 
			   	  if the Full Refresh interruption was caused by an unclean shutdown (crash, power failure, etc) on the RMD,
				  the user can launch a Full Refresh (not Restartable Full (-r), but straight Full Refresh (-f) at 0%)
				  or launch a Checksum Refresh (launchrefresh -c)
			   */
               reporterr (ERRFAC, M_RMD_REP_CRASH, ERRCRIT, lgnum );
               EXIT(EXITANDDIE);
        }

        } else {
            return ackcount;
        }

        /* Sequencer: increment acked' sequencer and then check sequence number matches */
        if ((ack->acktype == ACKNORM) &&
            (((u_long)ack->data == ACKRFDF)  ||
             ((u_long)ack->data == ACKRFD)   ||
             ((u_long)ack->data == ACKCHUNK) ||
             ((u_long)ack->data == ACKRFDCHKSUM)))
        {
            Sequencer_Inc(&group->AckedSequencer);
            ftd_trace_flow(FTD_DBG_SEQCER, "Process_acks() Sequencer %d : %d / %d\n", (u_long)ack->data, group->AckedSequencer, ack->lgsn);
            if (ack->lgsn != group->AckedSequencer)
            {
                group->AckedSequencer = ack->lgsn;
                reporterr(ERRFAC, M_GENMSG, ERRCRIT, "Exiting on sequencer count mismatch");
                ftd_trace_flow(FTD_DBG_SEQCER, "Process_acks() exiting: %d / %d\n", (u_long)ack->data, ack->lgsn);
                EXIT(EXITANDDIE);
            }
            // WI_338550 December 2017, implementing RPO / RTT
            // Update RTT calculations
            AckedSequencerRTTupdate(group);
        }
        
        switch((u_long)ack->data) {
        case ACKRFDCHKSUM:
            ftd_trace_flow(FTD_DBG_SOCK, "ACKRFDCHKSUM ack-lgsn = %d\n", ack->lgsn);
            for (sd = group->headsddisk; sd; sd = sd->n) {
                sd->ackrsync.cs.cnt = 0;
                sd->ackrsync.cs.num = 0;
                sd->ackrsync.cs.seglen = 0;
                sd->rsync.existing_deltas_in_list = sd->rsync.deltamap.cnt;
            }
            cnt = ack->ackoff;

            for (i = 0; i < cnt; i++) {
                memset(rrsync, 0, sizeof(rsync_t));
                memset(rrsync32, 0, sizeof(rsync32_t));
                if (!rmd64)
                {
                	rc = readsock(mysys->sock, (char*)rrsync32, sizeof(rsync32_t));
                	if (rc != 1) {
                    		EXIT(exitfail);
                	}  
                        convertrsyncfrom32to64(rrsync32, rrsync);
                }
                else
                {
                	rc = readsock(mysys->sock, (char*)rrsync, sizeof(rsync_t));
                	if (rc != 1) {
                    		EXIT(exitfail);
                	}  
                }
                if ((sd = get_lg_dev(group, rrsync->devid)) == NULL) {
                    reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, rrsync->devid);
                    EXIT(EXITANDDIE);
                    return 0;
                }
                rsync = &sd->rsync;
                ackrsync = &sd->ackrsync;

                ackrsync->cs.cnt = rrsync->cs.cnt;
                ackrsync->cs.num = rrsync->cs.num;
                ackrsync->cs.segoff = rrsync->cs.segoff;
                ackrsync->cs.seglen = rrsync->cs.seglen;

                length = ackrsync->cs.cnt*ackrsync->cs.num*DIGESTSIZE;
                if (length > ackrsync->cs.digestlen) {
                    ftdfree(ackrsync->cs.digest);
                    ackrsync->cs.digest = ftdmalloc(length);
                    ackrsync->cs.digestlen = length;
                } 
                /* read remote checksum digest into local digest buffer */
                rc = readsock(mysys->sock, ackrsync->cs.digest, length);
                if (rc != 1) {
                    EXIT(exitfail);
                }
                /* bump ackoff for device */
                rsync->ackoff = rrsync->offset+rrsync->length;
/*
*/
                sd->stat.rfshoffset = rsync->ackoff;
            }  
            /* compare local/remote checksum digests */
            if ((rc = chksumdiff()) == -1) {
                reporterr (ERRFAC, M_CHKSUMDIFF, ERRCRIT, argv0);
                EXIT(EXITANDDIE);
            }

            for (sd = group->headsddisk; sd; sd = sd->n)
            {
                rsync_t* rsync = &sd->rsync;
                rsync->pending_delta_acks += (rsync->deltamap.cnt - rsync->existing_deltas_in_list);
                
                if (rsync->pending_delta_acks == 0)
                {
                    rsync->refreshed_sectors = rsync->ackoff;
                }
            }
            
            break;
        case ACKBFDDELTA:
            /* read the header packet from network */
            if (!rmd64)
            {
            	rc = readsock(mysys->sock, (char*)header32, sizeof(headpack32_t));
            	if (1 != rc) {
                	EXIT(exitfail);
            	}  
		memset(header, 0, sizeof(headpack_t));
                converthdrfrom32to64 (header32, header);
            }
            else
            {
            	rc = readsock(mysys->sock, (char*)header, sizeof(headpack_t));
            	if (1 != rc) {
                	EXIT(exitfail);
            	}  
            }
            ftd_trace_flow(FTD_DBG_SOCK,
                "ACKBFDDELTA: header->len = %d, header->data = %d\n",
                header->len, header->data);

            if (header->len) {
#if defined(linux)
                if ((sd = get_lg_dev(group, header->devid)) == NULL) {
                    reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, ack->devid);
                    EXIT(EXITANDDIE);
                }
                blksize = sd->rsync.blksize;
#else
                blksize = 1 << DEV_BSHIFT;
#endif
                if ((datalen = header->len) > buflen) {
                    buflen = datalen;
                    ftdfree(buf);
                    buf = (char*)ftdmemalign(buflen, blksize);
                }
                if (header->data == BLOCKALLZERO) {
                    if (header->len > zerolen) {
                        zerolen = header->len;
                        ftdfree(zerobuf);
                        zerobuf = (char*)ftdmemalign(zerolen, blksize);
                        memset(zerobuf, 0, zerolen);
                    }
                    header->data = (u_long)zerobuf;
                } else {
                    if (1 != (rc = readsock(mysys->sock, buf, header->len))) {
                        EXIT(exitfail);
                    }  
                    if (header->compress) {
                        /* decompress the data */
                        if ((length = header->decodelen) > clen) {
                            clen = length;
                            ftdfree((char*)cbuf);
                            cbuf = (u_char*)ftdmemalign(clen, blksize);  
                        }
                        header->len =
                          decompress((u_char*)buf, (u_char*)cbuf, header->len);
                        header->data = (u_long)cbuf;
                    } else {
                        header->data = (u_long)buf;
                    }
                }
                memset(rrsync, 0, sizeof(*rrsync));
                rrsync->devid = header->devid;
                rrsync->offset = header->offset;
                rrsync->datalen = header->len;
                rrsync->length = rrsync->datalen >> DEV_BSHIFT;
                rrsync->data = (char*)header->data;

                rrsync->size = rsync->size;

                if ((rc = rsyncwrite_bfd(rrsync)) < 0) {
                    EXIT(EXITANDDIE);
                }
            }
            rsync->delta += rrsync->length;
            sd->stat.rfshdelta = rsync->delta;
            sd->stat.rfshoffset = rsync->ackoff;
            ftd_trace_flow(FTD_DBG_SOCK,
                "ACKBFDDELTA: datalen = %d, delta = %d, size = %d\n",
                datalen, rsync->delta, rsync->size);
            break;
        case ACKRFDF: /* full-refresh write ACK */
        case ACKRFD:  /* chksum/HRDB write ACK */

            // When in checksum refresh, the ackoff/rfshoffset are maintained on the reception of the ACKRFDCHKSUM acks.
            // The acks of the CMDRFDFWRITE when in checksum refresh are now requested just to maintain the refreshed_sectors.
            if (!chksum_flag)
            {
                rsync->ackoff += ack->ackoff;
                sd->stat.rfshoffset = rsync->ackoff;
            }
            else
            {
                rsync->pending_delta_acks--;
            }
            rsync->refreshed_sectors = ack->write_ackoff + ack->ackoff; // ackoff is really the amount of sectors written.
            break;
        case ACKBFD:
            cnt = ack->ackoff;
            /* read ACK offsets */
            for (i = 0; i < cnt; i++) {
                if (!rmd64)
                {
                	rc = readsock(mysys->sock, (char*)rrsync32, sizeof(rsync32_t));
                	if (rc != 1) {
                    		ftd_trace_flow(FTD_DBG_ERROR,
                            		"readsock returned: %d\n", rc);
                    		EXIT(exitfail);
                	}  
			memset(rrsync, 0, sizeof(rsync_t));
                        convertrsyncfrom32to64 (rrsync32, rrsync);
                }
		else
                {
                	rc = readsock(mysys->sock, (char*)rrsync, sizeof(rsync_t));
                	if (rc != 1) {
                    		ftd_trace_flow(FTD_DBG_ERROR,
                            		"readsock returned: %d\n", rc);
                    		EXIT(exitfail);
                	}
                }
                if ((sd = get_lg_dev(group, rrsync->devid)) == NULL) {
                    reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, rrsync->devid);
                    EXIT(EXITANDDIE);
                    return 0;
                }
                /* bump ackoff for device */
                rsync = &sd->rsync;
                rsync->ackoff = rrsync->offset+rrsync->length;
            }  
            break;
        case ACKCHUNK:
            ftd_trace_flow(FTD_DBG_SOCK,
                    "ACKCHUNK: ack->mirco = %d, ack->cpon = %d\n",
                    ack->mirco, ack->cpon);
            if (ack->mirco) {
                g_rfddone = 1;
            }
            if (ack->cpon) {
                ftd_trace_flow(FTD_DBG_FLOW1,
                        "lg(%d) _pmd_cpstart: %d -> 0, _pmd_cpon: %d -> %d\n",
                        lgnum, _pmd_cpstart, _pmd_cpon, ack->cpon);

                _pmd_cpon = ack->cpon;
                _pmd_cpstart = 0;
            }
            /* Set flag indicating if no BAB overflow occurred since this chunk was sent;
               if BAB overflow occurred, the BAB has been cleared and this chunk is probably gone already;
               otherwise, can migrate the data off the BAB (wr43926)
            */
            can_migrate_data = ( ack->bab_oflow == BAB_oflow_counter );

	    if (!rmd64)
            {
	       ackbytes = ack->ackoff + sizediff[0];
               for (i = 0; i < SIZE; i++)
               {
                  sizediff[i] = sizediff[i+1];
               }
               sizediff[i] = 0;
            }
            else    /* ...rmd64 */
            {
               ackbytes = ack->ackoff;
            }
            // WI_338550 December 2017, implementing RPO / RTT
            ftd_lg_update_consistency_timestamps(group);
            if (mysys->tunables.syncmode == TRUE)
            {
               if( can_migrate_data )
               {
            	  if (1 != (migrate(ackbytes)))
                  {
                	    reporterr (ERRFAC, M_MIGRATE, ERRCRIT, group->devname, 
                    		    strerror(errno));
                	    closeconnection();
                            reporterr (ERRFAC, M_PMDRE, ERRCRIT,lgnum);
                	    EXIT(EXITANDDIE);
                  }
               }
               else
               { /* BAB overflow(s) occurred since we sent this chunk; cannot migrate it; just do offset adjustments (WR43926) */
                  time(&group->lastackts);
                  group->offset -= (ackbytes/sizeof(int));
#ifdef TRACE_WR43926
                  sprintf( debug_msg, "ACKCHUNK; BABOFLOW count mismatch; rx = %d; current = %d; not calling migrate()\n", ack->bab_oflow, BAB_oflow_counter );
                  reporterr (ERRFAC, M_GENMSG, ERRINFO, debug_msg );
#endif
               }
            }   /* syncmode... */
            break;
        case ACKHUP:
            ftd_trace_flow(FTD_DBG_SOCK, "ACKHUP\n");
            g_ackhup = 1;
            break;
        case ACKCPSTART:
            ftd_trace_flow(FTD_DBG_FLOW1, "ACKCPSTART\n");
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "lg(%d) _pmd_cpstart from %d to 2\n", lgnum, _pmd_cpstart);

            _pmd_cpstart = 2; /* cp on pending state */
            break;
        case ACKCPSTOP:
            ftd_trace_flow(FTD_DBG_FLOW1, "ACKCPSTOP\n");
            _pmd_cpstop = 2; /* cp off pending state */
            break;
        case ACKCPON:
            ftd_trace_flow(FTD_DBG_FLOW1, "ACKCPON\n");
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "lg(%d) _pmd_cpstart: %d -> 0, _pmd_cpon: %d -> 0\n",
                    lgnum, _pmd_cpstart, _pmd_cpon);

            _pmd_cpon = 1;
            _pmd_cpstart = 0; /* no longer pending */

            if (ISPRIMARY(mysys)) {
                if (GET_LG_JLESS(mysys)) {
                    set_drv_state_checkpoint_jless(lgnum);
                }

                /*
                 * update pstore
                 */   
                FTD_CREATE_GROUP_NAME(group_name, lgnum);
                if (GETPSNAME(lgnum, ps_name) == -1) {
                    sprintf(group_name, "%s/%s%03d.cfg",
                            PATH_CONFIG, PMD_CFG_PREFIX, lgnum);
                    reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, group_name);
                    return -1;
                }

                ps_set_group_checkpoint(ps_name, group_name, 1);
            }
            
            reporterr(ERRFAC, M_CPON, ERRINFO, argv0);

            break;
        case ACKCPOFF:
            ftd_trace_flow(FTD_DBG_FLOW1, "ACKCPOFF\n");
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "lg(%d) _pmd_cpstop: %d -> 0, _pmd_cpon: %d -> 0\n",
                    lgnum, _pmd_cpstop, _pmd_cpon);
            _pmd_cpon = 0;
            _pmd_cpstop = 0; /* not longer pending */

            if (ISPRIMARY(mysys)) {
                if (GET_LG_JLESS(mysys)) {
                    ftd_trace_flow(FTD_DBG_FLOW1,
                            "_pmd_state: %d -> %d\n", _pmd_state, FTDRFD);
                    _pmd_state = FTDRFD;
                    _pmd_state_change = 1;
                }

                /*
                 * update pstore
                 */   
                FTD_CREATE_GROUP_NAME(group_name, lgnum);
                if (GETPSNAME(lgnum, ps_name) == -1) {
                    sprintf(group_name, "%s/%s%03d.cfg",
                            PATH_CONFIG, PMD_CFG_PREFIX, lgnum);
                    reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, group_name);
                    return -1;
                }

                ps_set_group_checkpoint(ps_name, group_name, 0);
            }

            reporterr(ERRFAC, M_CPOFF, ERRINFO, argv0);

            break;
        case ACKCPONERR:
            ftd_trace_flow(FTD_DBG_SOCK, "ACKCPONERR\n");
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "lg(%d) _pmd_cpstart: %d -> 0, _pmd_cpon: %d -> 0\n",
                    lgnum, _pmd_cpstart, _pmd_cpon);
            _pmd_cpon = 0;
            _pmd_cpstart = 0; /* not longer pending */
            reporterr(ERRFAC, M_CPONERR, ERRWARN, argv0);
            break;
        case ACKCPOFFERR:
            ftd_trace_flow(FTD_DBG_SOCK, "ACKCPOFFERR\n");
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "lg(%d) _pmd_cpstop: %d -> 0\n",
                    lgnum, _pmd_cpstop);
            _pmd_cpstop = 0; /* not longer pending */
            reporterr(ERRFAC, M_CPOFFERR, ERRWARN, argv0);
            break;
        case ACKKILLPMD:
            /* The RMD is exiting and wants the PMD to exit also... */
            ftd_trace_flow(FTD_DBG_SOCK, "ACKKILLPMD\n");
            reporterr(ERRFAC, M_PMDEXIT, ERRINFO, argv0);
			if( _net_bandwidth_analysis )
			{
                // If in network bandwidth analysis mode, remove the fictitious config files
                remove_fictitious_cfg_files( lgnum, 1 );
			}
            /* WR PROD6443: we no longer call adjust_FRefresh_offset_backward(); if an unclean 
               Full Refresh interruption has occurred on the RMD (ex.: crash, power failure, etc) 
               this is now reported by the RMD upon initial handshake; if clean exit on RMD (not a crash),
               a Resumed Full Refresh is now permitted (launchrefresh -r), or a Checksum Refresh as well.
		    */
            EXIT(EXITANDDIE);
#ifdef FIX_SIDE_EFFECT_ON_KILLRMD
Following modification for clean RMD shutdown has side effect on killrmds; to be fixed before implementing
            sprintf( debug_msg, "PMD%03d received stop signal from RMD; exiting due to RMD disconnection\n", lgnum );
            reporterr(ERRFAC, M_GENMSG, ERRINFO, debug_msg);
            /* RMD shutting down (target soft reboot, killdtcmaster, etc.), set state to go
               to tracking before exit, so that a Smart Refresh be performed when RMD comes back */
            clearmode(lgnum, 0);
            EXIT(EXITNETWORK);  <<< this causes PMD to be relaunched, the RMD also, so killrmds no longer works
#endif
            break;
        case ACKMSGCO:
            g_rfddone = 1;
            break;
        case ACKCLEARBITS:
            /* Clear bitmap here */
            {
                ftd_int32_t backup_mode = FALSE;
                if ((rc = FTD_IOCTL_CALL(group->devfd, FTD_BACKUP_HISTORY_BITS, &backup_mode)) < 0)
                {
                    reporterr(ERRFAC, M_GENMSG, ERRCRIT, get_error_str(errno));
                }
                ftd_trace_flow(FTD_DBG_FLOW17, "PMD [%d], received ack, clearing Historical Bitmaps!!!!", lgnum );

            }
            break;
                        
        default:
            break;
        }
    }

} /* process_acks */

/****************************************************************************
 * eval_netspeed -- evaluate network thruput against 'netmaxkbps' 
 ***************************************************************************/
int
eval_netspeed(void)
{
    group_t *group;
    sddisk_t *sd;
    struct timeval seltime[1];
    static float prev_kbps = 0.0;
    static float kbps = 0.0;
    static time_t currentts = 0;
    static time_t lastts = 0;
    static unsigned int netsleep = 0;
    static int firsttime = 1;
    int netmaxkbps;
    int deltasec;
    u_longlong_t a_tdatacnt;
    u_longlong_t a_datacnt;
    u_longlong_t e_tdatacnt;
    u_longlong_t e_datacnt;

    netmaxkbps = mysys->tunables.netmaxkbps;

    if (netmaxkbps < 0) {
        return 0;
    }
    if (firsttime) {
        time(&currentts);
        lastts = currentts;
        firsttime = 0;
    } else {
        lastts = currentts;
        time(&currentts);
    }
    deltasec = currentts - lastts;

    if (deltasec <= 0) {
        return 0;
    }
    group = mysys->group;
    prev_kbps = kbps;

    a_tdatacnt = 0;
    a_datacnt = 0;
    e_tdatacnt = 0;
    e_datacnt = 0;

    for (sd = group->headsddisk; sd; sd = sd->n) {
        a_tdatacnt += sd->stat.a_tdatacnt;
        a_datacnt += sd->stat.a_datacnt;
        e_tdatacnt += sd->stat.e_tdatacnt;
        e_datacnt += sd->stat.e_datacnt;
    }
    /* calculate kbps */
    ftd_trace_flow(FTD_DBG_SOCK, "e_datacnt, e_tdatacnt, deltasec = %llu, %llu, %d\n", a_datacnt, a_tdatacnt, deltasec);
    kbps = ((a_tdatacnt*1.0)/(deltasec*1.0))/1024.0;
    DPRINTF("\n*** eval_netspeed: kbps, netmaxkbps = %6.2f, %6.2f\n",  (float) kbps, (float) netmaxkbps);
    if (kbps > netmaxkbps) {
        netsleep += 500000;
    } else if (kbps < netmaxkbps) {
        if (netsleep > 500000) {
            netsleep -= 500000;
        } else  {
            netsleep = 0;
        }
    }
    if (netsleep < 1000000) {
        seltime->tv_sec = 0;
        seltime->tv_usec = netsleep;
    } else {
        seltime->tv_sec = netsleep/1000000;
        seltime->tv_usec = netsleep%1000000;
    }
    DPRINTF("\n### kbps = %6.2f, sleeping for %d\n", kbps, netsleep);

    select(0, NULL, NULL, NULL, seltime);

    return 0;
} /* eval_netspeed */

/*
 * net_write_vector -- write a message vector to the network
 */
int
net_write_vector(int fd, struct iovec *iov, int iovcnt, int role)
{
    struct timeval seltime[1];
    int rc, cnt, len, len1, tlen, i;
    int deltatime;
    time_t now, starttime;
    int iov_offset;
    int writecnt, save_iov_len;
    int write_timeo;
    struct iovec *liov;
    caddr_t iov_base, save_iov_base;

    len = 0;
    /* get total write length */
    for (i = 0; i < iovcnt; i++) {
        len += iov[i].iov_len;
    } 
    tlen = len;
    time(&starttime);

    seltime->tv_sec = 0;
    seltime->tv_usec = 10000;

    writecnt = 0;
    /* if we can't write for 30 minutes - give up ? */ 
    write_timeo = 1800; /* 30 minutes */

    while (1) {
#if defined(linux)    /* bug54 fix */
        seltime->tv_sec = 0;
        seltime->tv_usec = 10000;
#endif
        if (!checksock(fd, 1, seltime)) {
            /*
             * net not writable
             * see if anything to read 
             */
            net_writable = 0;
        } else {
            if (writecnt) {
                /*
                 * partial write complete
                 * adjust vector and retry
                 */
                len1 = 0;
                for (i = 0; i < iovcnt; i++) {
                    if ((len1 += iov[i].iov_len) > writecnt) {
                        break;
                    }
                } 
                iov_base = iov[i].iov_base; 
                iov_offset = (writecnt - (len1 - iov[i].iov_len));
                
                /* save base, len */
                save_iov_base = iov[i].iov_base;
                save_iov_len = iov[i].iov_len;

                iov[i].iov_base = iov_base+iov_offset; 
                iov[i].iov_len -= iov_offset; 

                liov = &iov[i];
                cnt = iovcnt - i;
            } else {
                i = 0;
                liov = iov;
                cnt = iovcnt;
                save_iov_base = iov[i].iov_base;
                save_iov_len = iov[i].iov_len;
            }
               
            rc = writev(fd, liov, cnt);

            if (rc == -1) {
                /* real errors, report error and return */
                if (errno == EFAULT
                || errno == EIO
                || errno == EPIPE
                || errno == ENXIO
                || errno == EBADF
                || errno == ERANGE
                || errno == EFBIG
                || errno == ENOSPC) {
                    reporterr (ERRFAC, M_SOCKWERR, ERRWARN,
                        argv0, strerror(errno));
                    return -1;
                } else {
                    /* soft errors, read, then retry the write */
                    net_writable = 0;
                }
            } else if (rc < len) { 
                writecnt += rc;
                len -= rc;
            } else {
                /* write completed */
                writecnt = tlen;
                net_writable = 1;
                return 0;
            }

            iov[i].iov_base = save_iov_base;
            iov[i].iov_len = save_iov_len;
            if (writecnt == tlen) {
                break;
            }
        }
        /* read net to free up remote in case it's blocked */
        time(&now);
        deltatime = now-starttime;
        if (deltatime > 1) {
            if (role == ROLEPRIMARY) {
                process_acks();
            }
        }
        if (deltatime > write_timeo) {
            /* report error and die */
            reporterr (ERRFAC, M_SOCKWERR, ERRWARN,
                argv0, "network write timed out");
            EXIT(EXITNETWORK);
        }
    }
    return 0;

} /* net_write_vector */

int converthdrfrom64to32(headpack_t* header, headpack32_t* header32)
{
        header32->magicvalue = header->magicvalue;
        header32->cmd        = header->cmd;
        header32->lgsn       = header->lgsn;
        header32->ts         = header->ts;
        header32->ackwanted  = header->ackwanted;
        header32->devid      = header->devid;
        header32->len        = header->len;
        header32->offset     = (off_t)header->offset;
        header32->compress   = header->compress;
        header32->decodelen  = header->decodelen;
        header32->data       = header->data;
        header32->bab_oflow  = header->bab_oflow;
        return 0;
}/* converthdrfrom64to32 */

int converthdrfrom32to64(headpack32_t* header32, headpack_t* header)
{
        header->magicvalue = header32->magicvalue;
        header->cmd        = header32->cmd;
        header->lgsn       = header32->lgsn;
        header->ts         = header32->ts;
        header->ackwanted  = header32->ackwanted;
        header->devid      = header32->devid;
        header->len        = header32->len;
        header->offset     = (u_longlong_t)header32->offset;
        header->compress   = header32->compress;
        header->decodelen  = header32->decodelen;
        header->data       = header32->data;
        header->bab_oflow  = header32->bab_oflow;
        return 0;
}/* converthdrfrom32to64 */

int convertackfrom64to32 (ackpack_t* ack, ackpack32_t* ack32)
{
        ack32->magicvalue = ack->magicvalue;
        ack32->acktype    = ack->acktype;
        ack32->lgsn       = ack->lgsn;
        ack32->ts         = ack->ts;
        ack32->devid      = ack->devid;
        ack32->ackoff     = (off_t)ack->ackoff;
        ack32->cpon       = ack->cpon;
        ack32->mirco      = ack->mirco;
        ack32->data       = (u_long)ack->data;
        ack32->bab_oflow  = ack->bab_oflow;
        // ack32->write_ackoff didn't exist.
        return 0;
} /* converackfrom64to32 */

int convertackfrom32to64 (ackpack32_t* ack32, ackpack_t* ack)
{
        ack->magicvalue = ack32->magicvalue;
        ack->acktype   = ack32->acktype;
        ack->lgsn      = ack32->lgsn;
        ack->ts        = ack32->ts;
        ack->devid     = ack32->devid;
        ack->ackoff    = (u_longlong_t)ack32->ackoff;
        ack->cpon      = ack32->cpon;
        ack->mirco     = ack32->mirco;
        ack->data      = (u_longlong_t)ack32->data;
        ack->bab_oflow = ack32->bab_oflow;
        ack->write_ackoff =  0; // Didn't exist!
        return 0;
} /* converackfrom32to64 */

int convertrsyncfrom64to32 (rsync_t* rsync, rsync32_t* rsync32)
{
        int i;
        rsync32->devid                = rsync->devid;
        rsync32->devfd                = rsync->devfd;
        rsync32->size                 = (int)rsync->size;
        rsync32->ackoff               = (off_t)rsync->ackoff;
        rsync32->offset               = (off_t)rsync->offset;
        rsync32->length               = rsync->length;
        rsync32->delta                = (off_t)rsync->delta;
        rsync32->datalen              = rsync->datalen;
        rsync32->done                 = rsync->done;
        rsync32->data                 = rsync->data;
        rsync32->cs.cnt               = rsync->cs.cnt;
        rsync32->cs.num               = rsync->cs.num;
        rsync32->cs.dirtyoff          = (int)rsync->cs.dirtyoff;
        rsync32->cs.dirtylen          = (int)rsync->cs.dirtylen;
        rsync32->cs.segoff            = (int)rsync->cs.segoff;
        rsync32->cs.seglen            = rsync->cs.seglen;
        rsync32->cs.digestlen         = rsync->cs.digestlen;
        rsync32->cs.digest            = rsync->cs.digest;
        rsync32->deltamap.size        = rsync->deltamap.size;
        rsync32->deltamap.cnt         = rsync->deltamap.cnt;
        rsync32->deltamap.refp        = rsync->deltamap.refp;
#if defined (linux)
        rsync32->blksize              = rsync->blksize;
#endif
        //rsync32->refreshed_sectors  Did not exist!
        
	return 0;
} /* convertrsyncfrom64to32 */

int convertrsyncfrom32to64 (rsync32_t* rsync32, rsync_t* rsync)
{
        int i;
        rsync->devid                = rsync32->devid;
        rsync->devfd                = rsync32->devfd;
        rsync->size                 = (u_longlong_t)rsync32->size;
        rsync->ackoff               = (u_longlong_t)rsync32->ackoff;
        rsync->offset               = (u_longlong_t)rsync32->offset;
        rsync->length               = rsync32->length;
        rsync->delta                = (u_longlong_t)rsync32->delta;
        rsync->datalen              = rsync32->datalen;
        rsync->done                 = rsync32->done;
        rsync->data                 = rsync32->data;
        rsync->cs.cnt               = rsync32->cs.cnt;
        rsync->cs.num               = rsync32->cs.num;
        rsync->cs.dirtyoff          = (u_longlong_t)rsync32->cs.dirtyoff;
        rsync->cs.dirtylen          = (u_longlong_t)rsync32->cs.dirtylen;
        rsync->cs.segoff            = (u_longlong_t)rsync32->cs.segoff;
        rsync->cs.seglen            = rsync32->cs.seglen;
        rsync->cs.digestlen         = rsync32->cs.digestlen;
        rsync->cs.digest            = rsync32->cs.digest;
        rsync->deltamap.size        = rsync32->deltamap.size;
        rsync->deltamap.cnt         = rsync32->deltamap.cnt;
        rsync->deltamap.refp        = rsync32->deltamap.refp;
#if defined (linux)
        rsync->blksize              = rsync32->blksize;
#endif
        rsync->refreshed_sectors    = 0; // Did not exist.

	return 0;
} /* convertrsyncfrom32to64 */

/* convertwlhfrom32to64 */
int convertwlhfrom64to32 ()
{
    group_t *group;
    wlheader_t *wlhead;
    wlheader32_t *wlhead32; 
    int i;
    int entrylen;
    int length;
    int chunkoffset = 0;
    int chunkoffset32 = 0; 
    int nobuf = 0;
    char *chunk;
    char *chunk32 = NULL;

    group = mysys->group;
    chunk = group->chunk;
    length = group->size;
    chunk32 = (char*)ftdmalloc(length);
    memset (chunk32, 0 , (length));
    while (chunkoffset < (length))
    {
        wlhead = (wlheader_t*)(chunk + chunkoffset);
        if (wlhead->majicnum != DATASTAR_MAJIC_NUM) {
            reporterr (ERRFAC, M_BADHDR, ERRCRIT, argv0,
                                                  wlhead->majicnum,
                                                  wlhead->dev,
                                                  wlhead->diskdev);

            log_wlheader_and_previous_data(wlhead, chunk);
            
            EXIT (EXITANDDIE);
        }
        nobuf += 1;
        entrylen = wlhead->length << DEV_BSHIFT;
        wlhead32 = (wlheader32_t*)(chunk32 + chunkoffset32);
        wlhead32->majicnum               = wlhead->majicnum; 
        wlhead32->offset                 = (ftd_uint32_t)wlhead->offset;
        wlhead32->length                 = (ftd_uint32_t)wlhead->length;
        wlhead32->timestamp              = wlhead->timestamp;
        wlhead32->dev                    = wlhead->dev;
        wlhead32->diskdev                = wlhead->diskdev;
        wlhead32->flags                  = wlhead->flags;
        wlhead32->complete               = wlhead->complete;
                                                                                               
#if defined(_AIX)
    wlhead32->timoid                     = wlhead->timoid;
#elif defined(SOLARIS)
#if (SYSVERS >= 570)
	wlhead32->timo_u.timoid_sol7     = wlhead->timo_u.timoid_sol7; 
#else  
	wlhead32->timo_u.timoid_pre_sol7 = wlhead->timo_u.timoid_pre_sol7; 
#endif /* defined SYSVERS >= 570 */
#elif defined(linux)
	wlhead32->timoid                 = wlhead->timoid; 
#else
    	wlhead32->timoid                 = wlhead->timoid; 
#endif /* defined(_AIX) */
 
#if defined(__KERNEL__) || defined(KERNEL)
    	wlhead32->group_ptr              = wlhead->group_ptr;
    	wlhead32->bp                     = wlhead->bp;         
#else
    	wlhead32->opaque1                = wlhead->opaque1; 
    	wlhead32->opaque2                = wlhead->opaque2;  
#endif
 
#if defined(HPUX)
    	wlhead32->syncForw               = wlhead->syncForw;
    	wlhead32->syncBack               = wlhead->syncBack;
    	wlhead32->syncTimeComplete       = wlhead->syncTimeComplete;
#endif /* defined(HPUX)  */
    	wlhead32->span                   = wlhead->span;
#if defined(linux)
#ifdef __KERNEL__
	wlhead32->timer_list.list.next   = wlhead->timer_list.list.next; 
	wlhead32->timer_list.list.prev   = wlhead->timer_list.list.prev; 
	wlhead32->timer_list.expires     = wlhead->timer_list.expires; 
        wlhead32->timer_list.data        = wlhead->timer_list.data;
        wlhead32->timer_list.function    = wlhead->timer_list.function;
        wlhead32->timer_list.base        = wlhead->timer_list.base;
#else
        /* size adjust */
	for (i = 0; i < max_ksizeof_timer_list; i++)
	{
    		wlhead32->padding[i]     = wlhead->padding[i];
	}
#endif /* __KERNEL__ */
#endif /* defined(linux) */

        memcpy (chunk32 + chunkoffset32 + sizeof(wlheader32_t), chunk + chunkoffset + sizeof(wlheader_t), entrylen);
        chunkoffset += sizeof(wlheader_t) + entrylen;
        chunkoffset32 += sizeof(wlheader32_t) + entrylen;
    }
    for (i = 0; i < SIZE; i++)
    {
        if (sizediff[i] == 0)
        {
                sizediff[i] = nobuf * sizeof (ftd_uint64_t);
                break;
        }
    }
    ftdfree (group->chunk);
    group->chunk = chunk32;
    chunk32 = NULL;
    group->chunkdatasize = 0;
    return 0;
} /* convertwlhfrom64to32*/

int convertwlhfrom32to64 (headpack_t *lheader, int *newsize, char *newchunk)
{
    int i;
    wlheader32_t *wlhead32;
    wlheader_t *wlhead;
    int entrylen32;
    int nobuf = 0;
    int chunkoffset32 = 0;
    int chunkoffset = 0;
    char *chunk = NULL;
    char *chunk32;

    (*newsize) = lheader->len;
    chunk32 = (char*)lheader->data;
    while (chunkoffset32 < lheader->len)
    {
        wlhead32 = (wlheader32_t*)(chunk32 + chunkoffset32);
        if (wlhead32->majicnum != DATASTAR_MAJIC_NUM) {
            reporterr (ERRFAC, M_BADHDR, ERRCRIT, argv0,
                                                  wlhead32->majicnum,
                                                  wlhead32->dev,
                                                  wlhead32->diskdev);

            log_wlheader32_and_previous_data(wlhead32, chunk32);
            
            EXIT (EXITANDDIE);
        }
        nobuf += 1;
        (*newsize) = (*newsize) + sizeof(ftd_uint64_t);
        chunk = (char*)ftdrealloc(chunk, (*newsize));
        memset (chunk + chunkoffset, 0 , (*newsize));
        wlhead = (wlheader_t*)(chunk + chunkoffset);
        entrylen32 = wlhead32->length << DEV_BSHIFT;
        wlhead->majicnum               = wlhead32->majicnum;
	if (wlhead32->offset == (-1))
	{
        	wlhead->offset                 = (ftd_uint64_t)-1;
	}
	else
	{
        	wlhead->offset                 = (ftd_uint64_t)wlhead32->offset;
	}
        wlhead->length                 = (ftd_uint64_t)wlhead32->length;
        wlhead->timestamp              = wlhead32->timestamp;
        wlhead->dev                    = wlhead32->dev;
        wlhead->diskdev                = wlhead32->diskdev;
        wlhead->flags                  = wlhead32->flags;
        wlhead->complete               = wlhead32->complete;

#if defined(_AIX)
    wlhead->timoid                     = wlhead32->timoid;
#elif defined(SOLARIS)
#if (SYSVERS >= 570)
        wlhead->timo_u.timoid_sol7     = wlhead32->timo_u.timoid_sol7;
#else
        wlhead->timo_u.timoid_pre_sol7 = wlhead32->timo_u.timoid_pre_sol7;
#endif /* defined SYSVERS >= 570 */
#elif defined(linux)
    	wlhead->timoid                 = wlhead32->timoid;
#else
    	wlhead->timoid                 = wlhead32->timoid;
#endif /* defined(_AIX) */

#if defined(__KERNEL__) || defined(KERNEL)
    	wlhead->group_ptr              = wlhead32->group_ptr;
    	wlhead->bp                     = wlhead32->bp;
#else
    	wlhead->opaque1                = wlhead32->opaque1;
    	wlhead->opaque2                = wlhead32->opaque2;
#endif

#if defined(HPUX)
    	wlhead->syncForw               = wlhead32->syncForw;
    	wlhead->syncBack               = wlhead32->syncBack;
    	wlhead->syncTimeComplete       = wlhead32->syncTimeComplete;
#endif /* defined(HPUX)  */
    	wlhead->span                   = wlhead32->span;
#if defined(linux)
#ifdef __KERNEL__
        wlhead->timer_list.list.next   = wlhead32->timer_list.list.next;
        wlhead->timer_list.list.prev   = wlhead32->timer_list.list.prev;
        wlhead->timer_list.expires     = wlhead32->timer_list.expires;
        wlhead->timer_list.data        = wlhead32->timer_list.data;
        wlhead->timer_list.function    = wlhead32->timer_list.function;
        wlhead->timer_list.base        = wlhead32->timer_list.base;
#else
        /* size adjust */
	for (i = 0; i < max_ksizeof_timer_list; i++)
        {
                wlhead->padding[i]     = wlhead32->padding[i];
        }
#endif /* __KERNEL__ */
#endif /* defined(linux) */
	memcpy (chunk + chunkoffset + sizeof(wlheader_t), chunk32 + chunkoffset32 + sizeof(wlheader32_t), entrylen32);
        chunkoffset32 += sizeof(wlheader32_t) + entrylen32;
        chunkoffset += sizeof(wlheader_t) + entrylen32;
    }

    lheader->data = (u_long)chunk;
/*    chunk = NULL; */
    return 0;
}

/***************************************************************************************\

Function:       Sequencer_Inc

Description:    Increment the sequencer value.

Parameters:     piSeq

Return Value:   void 
None.

Comments:       We use a maximum value and wraparound to make the values more readable.

\***************************************************************************************/
void Sequencer_Inc(int* piSeq)
{
    if (piSeq != NULL)
    {
        *piSeq = (*piSeq  + 1) % MAX_SEQUENCER;
    }
}

/***************************************************************************************\

Function:       Sequencer_Set

Description:    Set the sequencer to the given value.

Parameters:     piSeq
iSeqVal

Return Value:   void 
None.

Comments:       None.

\***************************************************************************************/
void Sequencer_Set(int* piSeq, int iSeqVal)
{
    if (piSeq != NULL)
    {
        *piSeq = iSeqVal;
    }
}

/***************************************************************************************\

Function:       Sequencer_Get

Description:    Return the sequencer's value.

Parameters:     piSeq

Return Value:   int 
Not Specified.

Comments:       None.

\***************************************************************************************/
int Sequencer_Get(int* piSeq)
{
    if (piSeq != NULL)
    {
        return *piSeq;
    }
    return 0;
}

/***************************************************************************************\

Function:       Sequencer_Reset

Description:    Reset a sequencer to it's default value.

Parameters:     piSeq

Return Value:   void 
None.

Comments:       None.

\***************************************************************************************/
void Sequencer_Reset(int* piSeq)
{
    if (piSeq != NULL)
    {
        *piSeq = 0;
    }
}

/***************************************************************************************\

Function:       SequencerTag_Set

Description:    Set tag value.

Parameters:     piTag
iSeqVal

Return Value:   void 
None.

Comments:       None.

\***************************************************************************************/
void SequencerTag_Set(int* piTag, int iSeqVal)
{
    if (piTag != NULL)
    {
        *piTag = iSeqVal;
    }
}

/***************************************************************************************\

Function:       SequencerTag_Invalidate

Description:    Set the tag to a value that will make it invalid.  An invalid tag means
that it carries an unreachable sequencer value.  It can be used to turn
off features using sequencers.  For example, a feature can invalidate the
tag as soon as it's reached, so further calls to IsReached will return FALSE,
thus turning off the feature.                

Parameters:     iTag

Return Value:   void 
None.

Comments:       None.

\***************************************************************************************/
void SequencerTag_Invalidate(int* piTag)
{
    if (piTag != NULL)
    {
        *piTag = MAX_SEQUENCER;
    }
}

/***************************************************************************************\

Function:       SequencerTag_IsValid

Description:    Verify if the tag contains a valid value.

Parameters:     iTag

Return Value:   BOOL 
Not Specified.

Comments:       None.

\***************************************************************************************/
int SequencerTag_IsValid(int iTag)
{
    if (iTag >= 0 &&
        iTag < MAX_SEQUENCER)
    {
        return 1;
    }
    return 0;
}

/***************************************************************************************\

Function:       SequencerTag_IsReached

Description:    Verify if the given sequencer value has reached the tag.

Parameters:     iTag            Test point
iSequencerValue Value to check for


Return Value:   BOOL 
Not Specified.

Comments:       This function is meant to work with the other Sequencer functions.  
It assumes that the sequencer will never pass the tag's value (so it must
be reset or invalidated once it is reached).

\***************************************************************************************/
int SequencerTag_IsReached(int iTag, int iSequencerValue)
{
    if (iSequencerValue == iTag)
    {
        return 1;
    }
    return 0;
}

//---------------------------------------------------------------
// WI_338550 December 2017, implementing RPO / RTT
unsigned GetTickCount()
{
        struct timeval tv;
        if(gettimeofday(&tv, NULL) != 0)
                return 0;

        return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

//---------------------------------------------------------------------------
// WI_338550 December 2017, implementing RPO / RTT
void update_RTT_samples_average( group_t* group, u_long new_RTT_sample )
{
    int     i;
    u_long  new_average;

    if( group->QualityOfService.current_RTT_sample_index == (NUM_OF_RTT_SAMPLES-1) )
        group->QualityOfService.current_RTT_sample_index = 0;
    else
        group->QualityOfService.current_RTT_sample_index += 1;
    
    group->QualityOfService.RTT_samples_for_recent_average[group->QualityOfService.current_RTT_sample_index] = new_RTT_sample;

    if( group->QualityOfService.current_number_of_RTT_samples < NUM_OF_RTT_SAMPLES )
        group->QualityOfService.current_number_of_RTT_samples += 1;

    new_average = 0;
    for ( i = 0; i < group->QualityOfService.current_number_of_RTT_samples; i++ )
    {
        new_average += group->QualityOfService.RTT_samples_for_recent_average[i];
    }
    new_average = (new_average / group->QualityOfService.current_number_of_RTT_samples);
    group->QualityOfService.average_of_most_recent_RTTs = new_average;

    return;
}
    
    
//---------------------------------------------------------------------------
// WI_338550 December 2017, implementing RPO / RTT
// Do the RTT updates upon receiving acknowledges from RMD
int AckedSequencerRTTupdate(group_t* lgp)
{
    u_long OldRTT;
    
    // Check if the Sequencer has reached the QOS tag and clear it.
    if (SequencerTag_IsReached(lgp->RTTComputingControl.iSequencerTag, lgp->AckedSequencer))
    {
        OldRTT = lgp->QualityOfService.RTT;
        if( OldRTT != 0 )
        {
            lgp->QualityOfService.previous_non_zero_RTT = OldRTT;
        }
        
        SequencerTag_Invalidate(&lgp->RTTComputingControl.iSequencerTag);
        if (lgp->RTTComputingControl.ChunksNumberSentBeforeTag == 0)
        {
            lgp->RTTComputingControl.ChunksNumberSentBeforeTag = 1;
        }
        lgp->QualityOfService.RTT = (GetTickCount() - lgp->RTTComputingControl.SendTick) / lgp->RTTComputingControl.ChunksNumberSentBeforeTag;
        
        if ((lgp->RTTComputingControl.ChunksNumberSentBeforeTag == 1)  && OldRTT)
        {
            // Add a weighted RTT computation to reduce imprecision due to lack of samples.
            lgp->QualityOfService.RTT = (2 * OldRTT + lgp->QualityOfService.RTT) / 3;
        }
         
        update_RTT_samples_average( lgp, lgp->QualityOfService.RTT );
        give_RPO_stats_to_driver( lgp );

    }

    return 0;
}

// WI_338550 December 2017, implementing RPO / RTT
/************************************************************************************//**

            Adjust RTT data based on the sequencer.

\param      lgp 

\return     

****************************************************************************************/
int update_RTT_based_on_SentSequencer(group_t* lgp)
// Update RTT. Sequencer is incremented outside this function.
{
    time_t CurrentTime;

    time(&CurrentTime);

    // Initiate RTT computing sequence
    if (lgp->RTTComputingControl.State &&
        !SequencerTag_IsValid(lgp->RTTComputingControl.iSequencerTag))
        {
        time_t TimeDiff = CurrentTime - lgp->RTTComputingControl.LastSamplingTime;
        
        if (TimeDiff >= lgp->RTTComputingControl.TimeInterval)
            {
            SequencerTag_Set(&lgp->RTTComputingControl.iSequencerTag, lgp->SendSequencer);
            if (lgp->SendSequencer > lgp->AckedSequencer + 1)
                {
                lgp->RTTComputingControl.ChunksNumberSentBeforeTag = lgp->SendSequencer - lgp->AckedSequencer;
                }
            else
                {
                lgp->RTTComputingControl.ChunksNumberSentBeforeTag = 1;
                }
            lgp->RTTComputingControl.SendTick = GetTickCount();
            lgp->RTTComputingControl.LastSamplingTime = CurrentTime;
            }
        }

    return lgp->SendSequencer;
}

