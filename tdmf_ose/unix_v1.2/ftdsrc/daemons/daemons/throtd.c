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
/***************************************************************************
 * throtd.c - FullTime Data throttle evaluation/performance calc  daemon
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the high level functionality for throtd
 *
 * $Id: throtd.c,v 1.27 2012/10/03 22:52:21 paulclou Exp $
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#if defined(linux)
#include <stdlib.h> 
#endif
#if defined(SOLARIS)
#include <sys/procfs.h>
#elif defined(HPUX)
#include <sys/pstat.h>
#elif defined(_AIX)
#include <sys/../procinfo.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/un.h>
#include "errors.h"
#include "config.h"
#include "devcheck.h"
#include "pathnames.h"
#include "process.h"
#include "ftdio.h"
#include "ps_intr.h"
#include "cfg_intr.h"
#include "stat_intr.h"
#include "network.h"
#include "ftd_cmd.h"
#include "aixcmn.h"

#define IN_USE	0xAA
#define NOT_USE	0x00
#if defined(SOLARIS) 
#define FTD_USEC_PER_SEC MICROSEC
#elif defined(_AIX)
#define FTD_USEC_PER_SEC uS_PER_SECOND
#elif defined(HPUX) || defined(linux)
#define FTD_USEC_PER_SEC 1000000
#endif

#define PIDLIST_INTERVAL 5

#if defined(SOLARIS) /* 1LG-ManyDEV */ 
extern int dummyfd_throtd[4];
#endif
pid_t pidlist[MAXLG];

typedef struct List_group
{
  struct List_group 	*prev;
  struct List_group 	*next;
  int			lgnum;	
  char			configpaths[32];	
  machine_t    		configsys;
} List_group_t;

List_group_t        *Started_List_head;
ftd_lg_map_t        Started_lgmap,  
                    prevStarted_lgmap;
ftd_uchar_t         Started_lgs[MAXLG];
ftd_uchar_t         prevStarted_lgs[MAXLG];
stat_buffer_t       StatBuf;
static int          masterfd = -1;
int                 len;
time_t              ts;
time_t              cfgchkts;
struct timeval      wakeupcall;
char                group_name[MAXPATHLEN];
ftd_stat_t          lgdrvstats;

#ifdef TDMF_TRACE
FILE *dbgfd;
#endif

extern char *argv0;
extern int lgcnt;

#if !defined(SECTORSIZE)
#if defined(SOLARIS) || defined(_AIX) || defined(linux)
#define SECTORSIZE 512
#elif defined(HPUX)
#define SECTORSIZE 1024
#endif
#endif /* !defined(SECTORSIZE) */

static int psfd = -1;

/****************************************************************************
 * getstats -- compile a list of device stats 
 ***************************************************************************/
int
getstats (group_t *group, int lgnum, char *configpath, int deltasec)
{
  struct stat sb[1];
  sddisk_t *sd;
  devstat_t *statp;
  pid_t pmdpid;
  int err;
  int pcnt;
  int procfd;
  char devname[MAXPATHLEN];
  char devbuffer[FTD_PS_DEVICE_ATTR_SIZE];
  devthrotstat_t *devthrotstats;
  lgthrotstat_t *lgthrotstats;
  disk_stats_t devdrvstats;
  char procfile[MAXPATHLEN] = "";
  char procname[MAXPATHLEN] =  "";
  char pmdgrpno[4];
  long long totalsects;
  int StatBuflen;
#if defined(SOLARIS)
  struct prpsinfo ps;
#elif defined(HPUX)
  struct pst_status ps;
#endif
  
  lgthrotstats = &(group->throtstats);
  
  /* find out where the pstore is */
  
  /* get driver lg stats */
  StatBuf.lg_num = lgnum;
  StatBuf.dev_num = 0;
  StatBuf.len = sizeof(ftd_stat_t);
  StatBuf.addr = (ftd_uint64ptr_t)&lgdrvstats;
  
  if ((err = FTD_IOCTL_CALL(masterfd, FTD_GET_GROUP_STATS, &StatBuf)) < 0) {
#ifdef DEBUG_THROTTLE
    fprintf(stderr,"FTD_GET_GROUP_STATS FTD_IOCTL_CALL: error = %d\n", errno);
#endif /* DEBUG_THROTTLE */
    return (0);
  }
  lgthrotstats->pctbab = (int) ((((double)lgdrvstats.bab_used) * 100.0) / 
				(((double)lgdrvstats.bab_used) + 
				 ((double)lgdrvstats.bab_free)));
  lgthrotstats->entries = lgdrvstats.wlentries;
  lgthrotstats->sectors = lgdrvstats.bab_used / SECTORSIZE;
  /* get the driver mode */
  if (lgdrvstats.state == FTD_MODE_PASSTHRU) 
    lgthrotstats->drvmode = DRVR_PASSTHRU;
  if (lgdrvstats.state == FTD_MODE_NORMAL)
    lgthrotstats->drvmode = DRVR_NORMAL;
  if (lgdrvstats.state == FTD_MODE_TRACKING)
    lgthrotstats->drvmode = DRVR_TRACKING;
  if (lgdrvstats.state == FTD_MODE_REFRESH)
    lgthrotstats->drvmode = DRVR_REFRESH;
  if (lgdrvstats.state == FTD_MODE_BACKFRESH)
    lgthrotstats->drvmode = DRVR_BACKFRESH;
  if (lgdrvstats.state == FTD_MODE_FULLREFRESH)
    lgthrotstats->drvmode = DRVR_REFRESH;
  if (lgdrvstats.state == FTD_MODE_CHECKPOINT_JLESS)
    lgthrotstats->drvmode = DRVR_NORMAL;
  
  strncpy (pmdgrpno, configpath+1, 3);
  pmdgrpno[3] = '\0';
  
  sprintf(procname, "PMD_%s", pmdgrpno);
  
  /*  figure out whether pmd is running and get pid form pidlist */ 
  if ((pmdpid = pidlist[atoi(pmdgrpno)]) <= 0) {
    lgthrotstats->pmdup = FALSE;
    lgthrotstats->pid = -1;
  } else {
    lgthrotstats->pmdup = TRUE;
    lgthrotstats->pid = pmdpid;
    
#if defined(SOLARIS)
    sprintf (procfile,"/proc/%05d", (int) pmdpid);
    if ((procfd = open (procfile, O_RDONLY))==-1) {
#ifdef DEBUG_THROTTLE
      fprintf(stderr, "Couldn't open proc file: %s",procfile);
#endif /* DEBUG_THROTTLE */
      lgthrotstats->percentcpu = 0;
    } else {
      (void) FTD_IOCTL_CALL (procfd, PIOCPSINFO, (char*)&ps);
      lgthrotstats->percentcpu  = (int)((((ps.pr_pctcpu && 0x0000ffff) * 1.0)
					 / 327.68) + 0.5);
      close (procfd);
    }
#elif defined(HPUX)
    if (pstat_getproc(&ps, sizeof(ps), 0, pmdpid) == -1) {
#ifdef DEBUG_THROTTLE
      fprintf(stderr, "pstat_getproc failed for pid %d\n",pmdpid);
#endif /* DEBUG_THROTTLE */
      lgthrotstats->percentcpu=0;
    } else {
      lgthrotstats->percentcpu = (int) ((((ps.pst_pctcpu && 0x0000ffff) 
					  * 1.0) / 327.68) + 0.5);
    }
#elif defined(_AIX)
    {
      /*- 
       * _AIX doesn't hand you the equivalent of %CPU. 
       * so...compute it from various times...
       */
      int ncpu;
      int pctcpu;
      unsigned long now;
      unsigned long ttime;
      unsigned long etime;
      struct procsinfo pb;
      
      pid_t pididx=pmdpid;
      
      struct fdsinfo *fb = (struct fdsinfo *) 0;
      
      if(getprocs(&pb, sizeof(pb), fb, 0, &pididx, 1) ==  1) {
	now = (unsigned long)time(0);
	ncpu = sysconf(_SC_NPROCESSORS_CONF);
	ttime = now - pb.pi_start;
	etime = pb.pi_utime + pb.pi_stime;
	pctcpu = (100 * etime) / ttime;
	pctcpu = pctcpu / ncpu;
	lgthrotstats->percentcpu = pctcpu;
      } else {
	/* getprocs(3) bummer */
	lgthrotstats->percentcpu = 0;
      }
    }
#elif defined(linux)
      {
              FILE *fp;
              char cmd_line[100];
              char cmd_result[100];
              char *cpu_percent;

              cpu_percent=NULL;
              memset(cmd_line,0,sizeof(cmd_line));
              sprintf(cmd_line,"ps -p  %d  -o \"%s\"",pmdpid,"%C");
              if ((fp = popen(cmd_line,"r")) == NULL) {
#ifdef DEBUG_THROTTLE
                      fprintf(stderr, "popen failed for pid %d\n",pmdpid);
#endif /* DEBUG_THROTTLE */
                      lgthrotstats->percentcpu=0;
              } else {
                      memset(cmd_result,0,sizeof(cmd_result));
                      if(fread(cmd_result, sizeof(char), sizeof(cmd_result), fp) < 0) {
#ifdef DEBUG_THROTTLE
                              fprintf(stderr,"fread error for pid %d, errno  %d\n",pmdpid,errno);
#endif /* DEBUG_THROTTLE */
                              lgthrotstats->percentcpu=0;
                              pclose(fp);
                      } else {
                              strtok(cmd_result,"\n\r");
                              cpu_percent = strtok(NULL,"\n\r");
                              if (cpu_percent != NULL ) {
                                 lgthrotstats->percentcpu = atoi(cpu_percent);
                              } else {
                                 lgthrotstats->percentcpu = 0;
                              }
                              pclose(fp);
                      }
              }
      }
#endif
  }
  /* We'll calculate the lg % done by the lowest device %, so init to 100 */
  lgthrotstats->pctdone = 100;
  StatBuflen = FTD_PS_DEVICE_ATTR_SIZE;
  lgthrotstats->actualkbps = 0;
  lgthrotstats->effectkbps = 0;
  lgthrotstats->local_kbps_read = 0.0;
  lgthrotstats->local_kbps_written = 0.0;
  
  /* get stats for each device */
  sd = group->headsddisk;
  while (sd) {
    force_dsk_or_rdsk(devname, sd->sddevname, 1);
    stat(devname, sb);
    sd->sd_rdev = sb->st_rdev;
    
    StatBuf.lg_num = lgnum;
    StatBuf.dev_num = sd->sd_rdev;
    StatBuf.len = FTD_PS_DEVICE_ATTR_SIZE;
    StatBuf.addr = (ftd_uint64ptr_t)(unsigned long)devbuffer;
    
    /* get the pmds stat info for the device */
    if((err = FTD_IOCTL_CALL(masterfd, FTD_GET_DEV_STATE_BUFFER, &StatBuf)) < 0) {
#ifdef DEBUG_THROTTLE
      fprintf(stderr,"FTD_GET_DEV_STATE_BUFFER FTD_IOCTL_CALL: error = %d\n",errno);
#endif /* DEBUG THROTTLE */
      return (0); 
    }
    /* stats from pmd are in statp */
    statp = (devstat_t*)devbuffer;
    
    StatBuf.len = sizeof(disk_stats_t);
    StatBuf.addr = (ftd_uint64ptr_t)(unsigned long)&devdrvstats;
    
    /* get the drvr stat info for the device */
    if((err = FTD_IOCTL_CALL(masterfd, FTD_GET_DEVICE_STATS, &StatBuf)) < 0) {
#ifdef DEBUG_THROTTLE
      fprintf(stderr,"FTD_GET_DEV_DEVOCE_STATS FTD_IOCTL_CALL: error = %d\n",errno);
#endif /* DEBUG_THROTTLE */
      return (0);
    }
    /* calculate dev stats */
    devthrotstats = &(sd->throtstats);
    /* device reads/writes --> sum to group reads/writes */
    devthrotstats->dev_ptotread_sectors = devthrotstats->dev_ctotread_sectors;
    devthrotstats->dev_ptotwrite_sectors = 
      devthrotstats->dev_ctotwrite_sectors;
    devthrotstats->dev_ctotread_sectors = devdrvstats.sectorsread;
    devthrotstats->dev_ctotwrite_sectors = devdrvstats.sectorswritten;
    
    /* 
     * WR 37777/37803: The code below was causing the writes not being caught if
     *                 no reads were done ever.
     *                 Since the kernel counters are always incrementing, this if
     *                 statement is only useful in case we restart this thread.
     */
    if (devthrotstats->dev_ptotread_sectors == 0L) {
      devthrotstats->dev_ptotread_sectors = 
	devthrotstats->dev_ctotread_sectors;
    }

    if (devthrotstats->dev_ptotwrite_sectors == 0L) {
      devthrotstats->dev_ptotwrite_sectors = 
	devthrotstats->dev_ctotwrite_sectors;
    }

    devthrotstats->local_kbps_read = 
      ((((double)(int)(devthrotstats->dev_ctotread_sectors - 
		       devthrotstats->dev_ptotread_sectors)) * 
	(double)DEV_BSIZE) / 1024.0) / 
      (double) mysys->tunables.statinterval;
    devthrotstats->local_kbps_written = 
      ((((double)(int)(devthrotstats->dev_ctotwrite_sectors - 
		       devthrotstats->dev_ptotwrite_sectors)) * 
	(double)DEV_BSIZE) / 1024.0) / 
      (double) mysys->tunables.statinterval;
    lgthrotstats->local_kbps_read += devthrotstats->local_kbps_read;
    lgthrotstats->local_kbps_written += devthrotstats->local_kbps_written;
    /* -- other per device statistics from driver */
    devthrotstats->sectors = devdrvstats.wlsectors;
    devthrotstats->entries = devdrvstats.wlentries;
    devthrotstats->pctbab = (devdrvstats.wlsectors * SECTORSIZE * 100.0) /
      ((lgdrvstats.bab_used + lgdrvstats.bab_free) * 1.0);
    devthrotstats->actualkbps = ((statp->a_tdatacnt * 1.0) / 
				 (deltasec * 1.0)) / 1024.0;
    devthrotstats->effectkbps = ((statp->e_tdatacnt * 1.0) /  
				 (deltasec * 1.0)) / 1024.0;
    /*  calculate percent done  */
    totalsects = (long long)sd->devsize;
    devthrotstats->pctdone = (( statp->rfshoffset * 1.0) /
			      (totalsects * 1.0)) * 100.0;
    
    
    /* reset the pmds devstat info since it has been read*/
    StatBuf.len = FTD_PS_DEVICE_ATTR_SIZE;
    StatBuf.addr = (ftd_uint64ptr_t)(unsigned long)devbuffer;
    statp->a_tdatacnt = statp->e_tdatacnt = statp->e_datacnt = 
      statp->a_datacnt = statp->entries = 0;
    
    if((err = FTD_IOCTL_CALL(masterfd, FTD_SET_DEV_STATE_BUFFER,  &StatBuf)) <
       0) {
#ifdef DEBUG_THROTTLE
      fprintf(stderr,"FTD_SET_DEV_STATE_BUFFER FTD_IOCTL_CALL: error = %d\n", errno);
#endif /* DEBUG_THROTTLE */
      return (0);
    }
    /* lg % done should be equal to the lowest device percentage */
    if (devthrotstats->pctdone < lgthrotstats->pctdone) {
      lgthrotstats->pctdone = devthrotstats->pctdone;
    }
    lgthrotstats->actualkbps = lgthrotstats->actualkbps + 
      devthrotstats->actualkbps;
    lgthrotstats->effectkbps = lgthrotstats->effectkbps + 
      devthrotstats->effectkbps;
    sd = sd->n;
  }
  return (1);
} /* getstats */

/****************************************************************************
 * ManageList -- add, delete, find elements in double linked list
 ***************************************************************************/
List_group_t *
ManageList(char mode, List_group_t *ptr, int i)
{
  static List_group_t *pPrev,*pNext,*pDbg;
  
  if (mode == 'A') {
    /* Add element in double linked list */
    if (!ptr)
      ptr = Started_List_head;
    
    while (ptr) {
      if (ptr->next) 
	ptr=ptr->next;
      else
	break;
    }
    pNext = (List_group_t *) ftdmalloc (sizeof(List_group_t));  
    if (!pNext) {
      fprintf(stderr,"No more memory available for Throtd. ");
      return(NULL);
    }
    memset ((void*)pNext,0,sizeof(List_group_t));
    pNext->lgnum=i;
    pNext->next=NULL;
    if (ptr) {
      ptr->next = pNext;						
      pNext->prev=ptr;
    } else {
      pNext->prev=NULL;
      Started_List_head = pNext;
    }
      
#ifdef PRINTING_INFO
    printf ("\nManageList - Add group # %d, ptr=%0x ptr->next=%0x, ptr->prev=%0x, Pnext=%0x, pNext->next=%0x, pNext->prev=%0x " ,
	    i,ptr,ptr->next, ptr->prev, pNext, pNext->next, pNext->prev);
      
    pDbg = Started_List_head;			
    while (pDbg) {
      printf("\n     Lg %d => addr:%x next:%x",pDbg->lgnum,pDbg,pDbg->next);
      pDbg = pDbg->next;
    }
#endif
    return (pNext);

  } else if (mode == 'F') { 
    /* Find element in FIFO */
    if (!ptr)
      ptr = Started_List_head;
    while (ptr)	{
      if (ptr->lgnum != i) 
	ptr=ptr->next;
      else
	{
#ifdef PRINTING_INFO
	  printf ("\nManageList - Found group # %d" , i);
#endif /* PRINTING_INFO */
	  return (ptr); /* found it !, return it */
	}
    }
    /* not found, should be a timing  problem, so we should ignore  ??? */
    return (NULL); 	
  
  } else if (mode == 'D') {
    /* Delete element in FIFO */
    if (ptr) {
      if ((pPrev = ptr->prev))
	pPrev->next = ptr->next;
      else if (ptr->next)					
	Started_List_head = ptr->next;				
      else
	Started_List_head = NULL;				
	  
      if ((pNext = ptr->next))
	pNext->prev = pPrev;
    }
#ifdef PRINTING_INFO		
    printf ("\nManageList - Delete group # %d, PList=%0x ptr->next=%0x, ptr->prev=%0x, Pnext=%0x, pNext->next=%0x, pNext->prev=%0x " ,
	    i,ptr,ptr->next, ptr->prev, pNext, pNext->next, pNext->prev);
    pDbg = Started_List_head;			
    while (pDbg) {
      printf("\n     Lg %d => addr:%x next:%x",pDbg->lgnum,pDbg,pDbg->next);
      pDbg = pDbg->next;
    }
#endif /* PRINTING_INFO */
    free (ptr);
    return (pPrev);
  }
  return NULL;
}

/****************************************************************************
 * GetStartedGroups -- obtain a map of currently started logical groups
 ***************************************************************************/
int 
GetStartedGroups ()
{
  sddisk_t *sd,*nsd;
  int i,err;
  List_group_t *pList = Started_List_head;
		
  /*  -- if not open, try to open the masterfd ftd device */
  if (masterfd == -1) { 
    if ((masterfd = open(FTD_CTLDEV, O_RDWR)) < 0) {
      masterfd = -1;
      return (0);
    }
  }
  
  Started_lgmap.lg_map = (ftd_uint64ptr_t)Started_lgs;
  Started_lgmap.count = 0;
  Started_lgmap.lg_max = 0;
  for (i=0; i<MAXLG; i++)
    Started_lgs[i] = FTD_STOP_GROUP_FLAG;
  /* obtain from driver a map of which groups are started */
  if ((err = FTD_IOCTL_CALL(masterfd, FTD_GET_GROUP_STARTED, &Started_lgmap)) >= 0) {
#ifdef PRINTING_INFO
    printf ("\nGetStartedGroups => count = %d/%d, lg_max = %d/%d map =>",
	    Started_lgmap.count,
	    prevStarted_lgmap.count,
	    Started_lgmap.lg_max,
	    prevStarted_lgmap.lg_max);
    for (i=0;i<=10;i++)
      putchar((Started_lgs)[i]);
#endif
    for (i=0; i<MAXLG; i++) {
      if (prevStarted_lgs[i] != Started_lgs[i]) {
	if (Started_lgs[i] == FTD_STARTED_GROUP_FLAG) {
	  /* a new group was started since previous pass*/
	  if ((pList= ManageList('A',pList,i)) == NULL)		
	    exit(1);
	  sprintf(pList->configpaths,"p%03d.cur", i);
	  memcpy ((void*)mysys, &pList->configsys, sizeof(machine_t));
	  initconfigs();
	  (void) readconfig (1, 2, 0, pList->configpaths);
	  FTD_CREATE_GROUP_NAME(group_name, i);
	  gettunables (group_name, 0, 1);
	  initstats(mysys);
	  memcpy (&pList->configsys, (void*)mysys, sizeof(machine_t));
	  pList->configsys.group->throtstats.statts=ts;
	  /* to be faster in the first pass */
	} else {
	  /* group no longer in the map (stopped, get rid of it) */
	  if ((pList= ManageList('F',pList,i))) {
	    /* find in List */	
	    /* it was found */
#if defined(HPUX) || defined(_AIX) || defined(linux)
	    fclose (pList->configsys.statfd);
#elif defined(SOLARIS)
	    close (pList->configsys.statfd);
#endif
	    sd = pList->configsys.group->headsddisk;
	    while (sd) { 
	      nsd = sd->n;
	      free(sd);
	      sd = nsd;
	    }
	    pList=ManageList('D',pList,i);	/* Delete in List */
	  }
	} /* a group was deleted */
      } /* if prev Map[i] != new Map[i] */				
    } /* for */
    /* whole array is processed, now replace prev Map by New Map */
    prevStarted_lgmap.count = Started_lgmap.count;
    prevStarted_lgmap.lg_max = Started_lgmap.lg_max;
    for (i=0; i<MAXLG; i++)
      prevStarted_lgs[i] = Started_lgs[i];
  } /* FTD_IOCTL_CALL */
  return (Started_lgmap.count);				
}

/****************************************************************************
 * DisplayStats -- 
 ***************************************************************************/
void DisplayStats()
{
  stat_buffer_t StatBuf;
  int elapsedtime,err;
  List_group_t *pList = Started_List_head;

  /* -- now process each group */
  while (pList)	{
    (void) time (&ts); /* get a clean new timestamp with each group */
    elapsedtime = ts - pList->configsys.group->throtstats.statts;
    if ((pList->configsys.tunables.statinterval > 0) &&
	(pList->configsys.tunables.statinterval <= elapsedtime)) {
      pList->configsys.group->throtstats.statts = ts;
#ifdef PRINTING_INFO
      printf ("\nDisplayStats => %d, elap = %d ",pList->lgnum,elapsedtime);
      fflush (stdout);
#endif /* PRINTING_INFO */
      memcpy((void*)mysys, &pList->configsys, sizeof(machine_t));
#ifdef NOT_SURE
      /* we need to get the tunables each iteration to catch any 
	 changes.  We specify 0 for the pstore argument because we
	 don't need to hit the pstore since the driver should have
	 a copy in memory */
      FTD_CREATE_GROUP_NAME(group_name, pList->lgnum);
      gettunables (group_name, 0, 1);
#endif
      if (getstats(mysys->group, pList->lgnum, pList->configpaths, 
		   elapsedtime)) {
	/* success if getstats == 1 */	
	(void)dumpstats(mysys, 0);
	(void) eval_throttles();
      }
      memcpy (&pList->configsys, (void*)mysys, sizeof(machine_t));
    }
    pList=pList->next;
  }	
}

/****************************************************************************
 * time_so_far_usec -- get microsec time
 ***************************************************************************/
static double
time_so_far_usec()
{
  struct timeval tp;

  gettimeofday(&tp, (struct timezone *) NULL);
  return ((double) (tp.tv_sec)*1000000.0) + (((double) tp.tv_usec) );
}

/****************************************************************************
 * main -- throtd process main
 ****************************************************************************/
int main (int argc, char **argv) 
{
  char configpath[FILE_PATH_LEN];
  int i;
  double st_time, en_time, sleep_time;
#if defined(_AIX)
  int   lgnum, message_already_logged = 0;
  char  cmdline[80], line[512], *p;
  pid_t procid;
  FILE	*f;
#endif
	

  putenv("LANG=C");

  cfgchkts = (time_t) 0;
  argv0 = strdup(argv[0]);
#ifdef DEBUG_THROTTLE
  throtfd = fopen ( PATH_RUN_FILES "/throtdump.dbg", "w+");
#endif /* DEBUG_THROTTLE */
  /* -- Make sure we're root -- */          /* WR16793 */
  if ( geteuid() != 0 ) {                   /* WR16793 */
    exit (1);                               /* WR16793 */
  }                                         /* WR16793 */
  /* init memory */
  if (initerrmgt ("FTD") < 0) 
    exit(1);
  log_command(argc, argv);   /* trace command line in dtcerror.log */	

#if defined(SOLARIS)  /* 1LG-ManyDEV */
    /*Dummy open for gethostbyname / gethostbyaddr*/
  for( i=0; i<4; i++){
    if ( (dummyfd_throtd[i] = open("/dev/null", O_RDONLY)) == -1){
      reporterr(ERRFAC, M_FILE, ERRWARN, "/dev/null", strerror(errno));
      break;
    }
  }
#endif
  
  Started_lgmap.count= 0;
  Started_lgmap.lg_max=0;
  Started_lgmap.lg_map = (ftd_uint64ptr_t)Started_lgs;
  for (i=0; i<MAXLG; i++)
    Started_lgs[i] = FTD_STOP_GROUP_FLAG;
  prevStarted_lgmap.count= 0;
  prevStarted_lgmap.lg_max=0;
  prevStarted_lgmap.lg_map = (ftd_uint64ptr_t)prevStarted_lgs;
  for (i=0; i<MAXLG; i++)
    prevStarted_lgs[i] = FTD_STOP_GROUP_FLAG;
  Started_List_head = NULL;



  while (TRUE) {  
    memset(pidlist, 0, sizeof(pid_t)*MAXLG);

    /* -- check the config files for primary */
    (void) time (&ts); /* get a clean new timestamp with each group */
    GetStartedGroups();
    /*
      if ((ts-cfgchkts) >= 10) {
        cfgchkts = ts;
        GetStartedGroups();
      }
    */
#if defined(_AIX)
   /* Case PROD6812 / WR PROD6813: libgen function calls on AIX will lead to high CPU usage if there are
      numerous processes running, as get_next_process() gets called (to build the PMD PID list)
      (which in turns calls getprocs(), getargs() and sys_parm()) for every running process just to find which ones are PMDs.
      Modification: at higher code level, get the PMD PIDs only and then the PMD names in order to link PIDs 
      with PMD numbers in PMD list.
   */
	sprintf(cmdline, "/bin/ps -ef | /bin/grep PMD_[0-9][0-9][0-9] | /bin/grep -v grep");
	if ((f = popen(cmdline, "r")) == NULL)
	{
	  if( !message_already_logged )
	  {
	    reporterr(ERRFAC, M_GENMSG, ERRINFO, "throtd: error attempting to get the list of PMD PIDs; popen failed.\n");
	    message_already_logged = 1;  // We don't want to keep logging messages in this infinite loop
      }
	}
	else
	{
	  memset(line, 0, sizeof(line));
	  while(fgets(line, sizeof(line), f) != NULL)
	  {
	    // Get to the UID of the ps output
	    p = line;
		while( *p == ' ' ) ++p;
		// Get to white space separating UID from PID
		while( *p != ' ' ) ++p;
	    // Get to the PID of the ps output
		while( *p == ' ' ) ++p;
        procid = (pid_t)atoi(p);
		// Now get the PMD name
		p = strstr(line, "PMD_");
		if(p != NULL)
		{
			p += 4;
			lgnum = atoi(p);
			if( lgnum < MAXLG )
	          pidlist[lgnum] = procid;
		}
	  } // ...while
	  pclose(f);
    }
#else
    ftd_getidlist ( pidlist ); /* Create PID list of PMD.*/
#endif

    for (i=0; i<PIDLIST_INTERVAL; i++){
      st_time = time_so_far_usec();
      if (Started_lgmap.count)
        DisplayStats();
      en_time = time_so_far_usec();
      sleep_time =  FTD_USEC_PER_SEC - (en_time - st_time);
      if(sleep_time > 0){
        if( sleep_time >= 1000000 )   /* usleep does not accept values >= 1000000 (pc071109) */
          sleep( (int)(sleep_time / 1000000) );   /* Does the second part */
        usleep( (long)sleep_time % 1000000 );          /* Does the microsecond part */
      }

    }
  }     
  return 0;
} 
