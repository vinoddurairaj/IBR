/*
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>
#include <malloc.h>
#include <ctype.h>
#include "errors.h"

#include "procinfo.h"

#if defined(SOLARIS)

#define PROCDIR "/proc/"
#include <dirent.h>
#include <sys/procfs.h>

typedef struct _process_root {
    DIR *dir;
    int dirpathlen;
    char dirpath[256];
} ProcessRoot;

#elif defined(HPUX)

#include <sys/pstat.h>

typedef struct _process_root {
    int idx;
    char path[256];
} ProcessRoot;

#elif defined(_AIX)

#include <sys/types.h>
/* 
 * this oddpath keeps AIX cpp from picking 
 * using ./procinfo.h for both "procinfo.h"
 * and <procinfo.h>. sigh.
 */
#include <sys/../procinfo.h>

typedef struct _process_root {
	pid_t           idx;
	char            path[256];
}               ProcessRoot;

#endif

#if defined(SOLARIS)

/*
 * open the /proc directory 
 */
static ProcessRoot *
init_process_info(void)
{
    ProcessRoot *pr;

    if ((pr = (ProcessRoot *)malloc(sizeof(ProcessRoot))) == NULL) {
        return NULL;
    }
    strcpy(pr->dirpath, PROCDIR);
    pr->dirpathlen = strlen (pr->dirpath);

    /* -- open the /proc directory */
    if ((DIR*)NULL == (pr->dir = opendir(pr->dirpath))) {
        free(pr);
	if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT, pr->dirpath, strerror(errno));
	}
        return NULL;
    }
    return pr;
}

/*
 * get the next entry in the /proc directory, open it, and get the
 * command line for the process. 
 */
static char *
get_next_process (ProcessRoot *pr, int *procid, char *procname )
{
    struct dirent *dent;
    int pathlen;
    int pid;
    int fd;
    int i;
    int status;
    char *ptr;
    char *pname;
    struct prpsinfo ps;

    /* loop until we hit the end of the directory or find a valid entry */
    fd = 0;
    while (NULL != (dent = readdir(pr->dir))) {
        if (fd != 0) {
            close(fd);
            fd = 0;
        }
        if (0 == strcmp(dent->d_name, ".")) continue;
        if (0 == strcmp(dent->d_name, "..")) continue;
        ptr = dent->d_name;
        pathlen = pr->dirpathlen;
        while (*ptr) pr->dirpath[pathlen++] = *(ptr++);
        pr->dirpath[pathlen] = '\0';

        pid = 0;
        ptr = dent->d_name;
        while (*ptr) {
            pid = (pid*10) + ((*ptr)-'0');
            ptr++;
        }

        if (0 >= (fd = open (pr->dirpath, O_RDONLY))) {
            fd = 0;
            continue;
        }
        if (0 != (status = ioctl (fd, PIOCPSINFO, (char*)&ps))) continue;
        if (ps.pr_zomb) continue;

        /* get last component of argv[0] for comparison */
        pname = strtok(ps.pr_psargs, " ");
        for (i=strlen(pname); i>0; i--) {
            if (pname[i-1] == '/') {
                break;
            }
        }
        pname+=i;

        close(fd);
        *procid = pid;
        strcpy (procname, pname);
        return pname;
    }
    if (fd != 0) {
        close(fd);
    }
    *procid = 0;
    return NULL;
}

static void
close_process_info(ProcessRoot *pr)
{
    closedir(pr->dir);
    free(pr);
}

#elif defined(HPUX)

/*
 * open the /proc directory 
 */
static ProcessRoot *
init_process_info()
{
    ProcessRoot *pr;

    if ((pr = (ProcessRoot *)malloc(sizeof(ProcessRoot))) == NULL) {
        return NULL;
    }
    memset(pr, 0, sizeof(ProcessRoot));

    return pr;
}

/*
 * get the next entry in the /proc directory, open it, and get the
 * command line for the process. 
 */
static char *
get_next_process(ProcessRoot *pr, int *procid, char* procname)
{
    char *pname;
    char *pgm;
    struct pst_status ps;
	
    /* get the next non-zombie process */
    while (pstat_getproc(&ps, sizeof(ps), 1, pr->idx) > 0) {
        pr->idx++;
        if (ps.pst_stat == PS_ZOMBIE) continue;

        /* just get the last token in path */
        if ((pgm = strrchr(ps.pst_cmd, '/'))) {
            strcpy(pr->path, pgm+1); 
        } else {
            strcpy(pr->path, ps.pst_cmd); 
        }
        *procid = ps.pst_pid;
        strcpy (procname, pr->path);
        return (pr->path);
    }
    *procid = 0;
    return NULL;
}

static void
close_process_info(ProcessRoot *pr)
{
    free(pr);
}

#elif defined(_AIX)

/* init structs */
static ProcessRoot *
init_process_info()
{
    ProcessRoot *pr;

    if ((pr = (ProcessRoot *)malloc(sizeof(ProcessRoot))) == NULL) {
        return NULL;
    }
    memset(pr, 0, sizeof(ProcessRoot));

    return pr;
}

/* get the next entry in the process table. */
static char    *
get_next_process(ProcessRoot * pr, int *procid, char *procname)
{
	/* don't free(3) me ! */
	static struct procsinfo pb;
	struct fdsinfo *fb = (struct fdsinfo *) 0;
    char argsbuf[MAXPATHLEN];
    char *argv0;
   
	while (getprocs(&pb, sizeof(pb), fb, 0, &pr->idx, 1)) {
		*procid = pb.pi_pid;
        getargs(&pb, sizeof(pb), argsbuf, sizeof(argsbuf));
        /* just get the last token in path */
        if ((argv0 = strrchr(argsbuf, '/'))) {
            strcpy(procname, argv0+1); 
        } else {
            strcpy(procname, argsbuf); 
        }
		return (procname);
	}
	*procid = 0;
	return NULL;
}

static void
close_process_info(ProcessRoot *pr)
{
    free(pr);
}

#endif

/************************************************************************/
/*                         private functions                            */
/************************************************************************/
static int
compare_proc_info (void* a, void* b)
{
    proc_info_t* aa;
    proc_info_t* bb;
    aa = (proc_info_t*) a;
    bb = (proc_info_t*) b;
    return (((aa->pid - bb->pid)==0)?0:(((aa->pid - bb->pid)<0)?-1:1));
}

/************************************************************************/
void
del_proc_names (proc_info_t*** proclist, int numelem)
{
    int i;
    if (*proclist == (proc_info_t**)NULL) return;
    for (i=0; i<numelem; i++)
        free ((*proclist)[i]);
    free(*proclist);
    *proclist = (proc_info_t**)NULL;
}  

/************************************************************************/
static void sswap(void *v[], int i, int j)
{
    void* temp;
    temp=v[i];
    v[i]=v[j];
    v[j]=temp;
}

/************************************************************************/
static void qqsort (void *v[], int left, int right, 
    int (*comp)(void*, void*))
{
    int i, last;
    void sswap(void *v[], int, int); 
    if (left >= right)
        return;
    sswap(v, left, (left+right)/2);
    last = left;
    for (i=left+1; i<=right; i++)
        if ((*comp)(v[i], v[left]) < 0) 
            sswap(v, ++last, i);
    sswap(v, left, last);
    qqsort(v, left, last-1, comp);
    qqsort(v, last+1, right, comp);
}
/************************************************************************/
/*                         public functions                             */
/************************************************************************
 * get_proc_names -- returns -1 on error, or count of process names
 *                       starting with the prefix.  Pass as input
 *                       an NULL assigned proc_info_t*** pointer, 
 *                       returned array is dynamically sized and
 *                       elements are malloced into this array.
 *                       Use del_proc_names to free the returned list.
 *                       BTW, the list is sorted in ascending PID order.
 *                       (see standalone "main" at bottom for example use)
 ***********************************************************************/
int
get_proc_names (char* prefix, int exactflag, proc_info_t*** proclist)
{
    int prefixlen;
    ProcessRoot* pr;
    int matchcnt;
    int matchall;
    int listsize;
    int listused;
    int procid;
    int cont;
    int i;
    char pname[512];
 
    matchall = 0;
    matchcnt = 0;
    prefixlen = 0;
    listsize = 0;
    listused = 0;

    /* -- detect a get everything condition */
    if (prefix != (char*)NULL) {
        prefixlen = strlen(prefix);
    } else {
        matchall = 1;
    }
    if ((prefixlen == 0) || (prefixlen == 1 && prefix[0] == '*')) matchall = 1;
    /* -- do an initial malloc of the proc_info table */
    if (*proclist != (proc_info_t**)NULL) {
        del_proc_names (proclist, listused);
    }
    listsize += 256;
    if ((proc_info_t**)NULL == 
        (*proclist = (proc_info_t**) malloc (sizeof(proc_info_t*)*listsize))) {
        return (-1);
    }

    if ((pr = init_process_info()) == NULL) {
        return (-1);
    }
    while (NULL != (get_next_process (pr, &procid, pname))) {
        /* if we have already seen this process then we're done - stupid HP */
        cont = 0;
        for (i = 0; i < listused; i++) {
            if (procid == (*proclist)[i]->pid) {
                cont = 1;
                break;
            }
        }
        if (cont) {
            continue;
        }
        if (!matchall) {
            if (exactflag) {
                if (0 != strcmp(prefix, pname)) continue;
            } else {
                if (0 != strncmp(prefix, pname, prefixlen)) continue;
            }
        }
        /* -- see if the array need to be resized */
        if (listused == listsize) {
            listsize += 256;
            realloc ((void*)*proclist, (sizeof(proc_info_t**) * listsize));
            if (*proclist == (proc_info_t**)NULL) {
                del_proc_names (proclist, listused);
                *proclist = (proc_info_t**)NULL; 
                return (-1);
            }
        }
        /* -- allocate a new proc_info_t structure */
        (*proclist)[listused] = 
            (proc_info_t*) malloc (sizeof(proc_info_t));
        if ((*proclist)[listused] == (proc_info_t*)NULL) {
            del_proc_names (proclist, listused);
            *proclist = (proc_info_t**)NULL; 
            return (-1);
        }
        /* -- add the name and pid to the list */
        matchcnt++;
        strcpy((*proclist)[listused]->name, pname);
        (*proclist)[listused]->pid = (int) procid;
        listused++;
        if (exactflag) break;
    }
    close_process_info (pr);
    if (matchcnt > 1) { 
        /* qsort ((void*)(*proclist), sizeof(proc_info_t*), matchcnt, 
           compare_proc_info); */
        qqsort ((void**)(*proclist), 0, matchcnt-1, compare_proc_info);
    } 
    return (matchcnt);
}

/***********************************************************************
 * capture_proc_names -- returns -1 on error, or count of process names
 *                       starting with the prefix.  mallocs a buffer
 *                       long enough to hold the Tcl list formatted 
 *                       string (caller will have to free this buffer). 
 *                       This function is wrapper for get_proc_names
 ***********************************************************************/
int
capture_proc_names (char* prefix, int exactflag, char** bbuf) 
{
    proc_info_t** pi;
    int pisize; 
    int piused;
    int retval;
    int len;
    char tmpstring[256];
    char countstr[256];
    char trailstr[256];
    int tlen;
    int i, j, p;

    pisize = 0;
    piused = 0;
    pi = (proc_info_t**)NULL;
    retval = 0;
    if (0 > (retval = get_proc_names (prefix, exactflag, &pi))) {
        del_proc_names (&pi, piused);
        return (retval);
    }
    sprintf(countstr, "%d { ", retval);
    len = strlen(countstr);
    for (i=0; i<retval; i++) {
        sprintf (tmpstring, " {%s %d}", pi[i]->name, pi[i]->pid);
        len += strlen(tmpstring);
    }
    sprintf (trailstr, " } ");
    len += strlen (trailstr);
    *bbuf = (char*) malloc ((len+2) * sizeof(char));
    if (*bbuf == (char*)NULL) {
        del_proc_names (&pi, piused);
        return (-1);
    }
    p = 0;
    tlen = strlen(countstr);
    j=0;
    while (tlen) {
        (*bbuf)[p] = countstr[j];
        p++;
        j++;
        tlen--;
    }
    for (i=0; i<retval; i++) {
        j=0;
        sprintf (tmpstring, " {%s %d}", pi[i]->name, pi[i]->pid);
        tlen = strlen(tmpstring);
        while (tlen) {
            (*bbuf)[p] = tmpstring[j];
            p++;
            j++;
            tlen--;
        }
    }
    (*bbuf)[p++] = ' ';
    (*bbuf)[p++] = '}';
    (*bbuf)[p++] = ' ';
    (*bbuf)[p] = '\0';
    del_proc_names (&pi, piused);
    return (retval);
}

/***********************************************************************
 * process_proc_request -- write a character buffer containing process
 *                         name and pid - either for the 1st exact match
 *                         found or for all processes with a certain
 *                         prefix string.
 ***********************************************************************/
int
process_proc_request (int fd, char* cmd)
{
    int retval;
    char* buf;
    char* ebuf;
    int ebufsize;
    int numemsgs;
    int exactflag;
    int i;
    char prefix[256];
    int len;
    int geterrorsflag;
    int offset;
    char offstring[256];
    int j;
    char outstring[256];
    int oldoffset;
  
    i=0;
    j=0;
    offset = 0;
    retval = 0;
    offstring[0] = '0';
    offstring[1] = '\0';
    geterrorsflag = 0;
    if (0 == strncmp(cmd, "ftd get process info", 20)) {
        exactflag = 1;
        len = 20;
    } else if (0 == strncmp(cmd, "ftd get all process info", 24)) {
        exactflag = 0;
        geterrorsflag = 0;
        len = 24;
    } else if (0 == strncmp(cmd, "ftd get all process and error info", 34)) {
        exactflag = 0;
        len = 34;
        geterrorsflag = 1;
    } else {
        return (-999);
    }
    cmd += len;
    while (isspace(*cmd)) cmd++;
    while (isprint(*cmd) && !(isspace(*cmd))) prefix[i++] = *cmd++;
    prefix[i] = '\0';
    if (geterrorsflag) {
        while (isspace(*cmd)) cmd++;
        while (isprint(*cmd) && !(isspace(*cmd))) offstring[j++] = *cmd++;
        offstring[j] = '\0';
        sscanf (offstring, "%d", &offset);
    }
    *cmd = '\0';
    if (geterrorsflag) {
        oldoffset = offset;
        getlogmsgs (&ebuf, &ebufsize, &numemsgs, &offset);
    }
    if (0 >= (retval = capture_proc_names(prefix, exactflag, &buf))) {
        (void) write (fd, (void*)" 0 {} \n", strlen(" 0 {} \n"));
    } else {
        if (retval == 0) {
            free (buf);
            buf = (char *)NULL;
        } else {
            if (geterrorsflag == 0) buf[strlen(buf)] = '\n';
            (void) write (fd, (void*)buf, strlen(buf));
            free (buf);
            buf = (char *)NULL;
        }
    } 
    if (geterrorsflag != 0) {
        if (oldoffset == offset) {
            sprintf (outstring, " { ***NOERRORS*** %d } {} \n", offset);
            (void) write (fd, (void*) outstring, strlen(outstring));
        } else {
            sprintf (outstring, " { ***ERRORS*** %d } { ", offset);
            (void) write (fd, (void*)outstring, strlen(outstring));
            (void) write (fd, (void*)ebuf, ebufsize);
            (void) write (fd, " } \n", strlen(" } \n"));
        }
        if (ebufsize > 0) free (ebuf);
    }
    return (retval);
}    

#ifdef STANDALONE_DEBUG

#include <stdlib.h>

int
main (int argc, char** argv)
{
    int bufused, bufsize, retval;
    char* buf;
    proc_info_t** pi;
    char *prefix;
    int i;
    char tempbuf[256];
    int exactflag;
  
    buf = (char*)NULL;
    bufsize = 0;
    bufused = 0;
  
    prefix = "*";
    exactflag = 0;
    if (argc >= 2) {
        prefix = argv[1];
        if (argc >= 3) {
            exactflag = atoi(argv[2]);
        }
    }

    if (initerrmgt (ERRFAC) < 0) {
	exit(1);
    }
    fprintf (stderr, "-------------get_proc_names---------------\n");
    retval = get_proc_names (prefix, exactflag, &pi);
    fprintf(stderr, "get_proc_names = %d\n", retval);
    if (retval > 0) {
        for (i=0; i<retval; i++) {
            fprintf (stderr, "pid=[%d], name=[%s]\n", pi[i]->pid, pi[i]->name);
        }
        del_proc_names (&pi, retval);
    }
    fprintf (stderr, "-------------get_proc_names---------------\n");
    retval = get_proc_names (prefix, exactflag, &pi);
    fprintf(stderr, "get_proc_names = %d\n", retval);
    if (retval > 0) {
        for (i=0; i<retval; i++) {
            fprintf (stderr, "pid=[%d], name=[%s]\n", pi[i]->pid, pi[i]->name);
        }
        del_proc_names (&pi, retval);
    }
    fprintf (stderr, "-------------capture_proc_names---------------\n");
    retval = capture_proc_names (prefix, exactflag, &buf);
    fprintf (stderr, "%d = capture_proc_names -> [%s]\n", retval, buf);
    free (buf);
    fprintf (stderr, "-------------process_proc_request---------------\n");
    if (exactflag == 0) {
        sprintf (tempbuf, "ftd get all process and error info %s %s", argv[1],
            argv[3]);
    } else {
        exactflag = 1;
        sprintf (tempbuf, "ftd get process info %s", prefix);
    }
    fprintf(stderr, "   cmd = [%s]\n", tempbuf);
    retval = process_proc_request (1, tempbuf);
    fprintf (stderr, "process_proc_request returned: %d\n", retval);
}
#endif /* STANDALONE_DEBUG */
