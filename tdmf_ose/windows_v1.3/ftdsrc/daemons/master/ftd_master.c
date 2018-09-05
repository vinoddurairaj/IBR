/*
 * ftd_master.c - ftd master daemon
 *
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */


// Include this library to force the linker to link the MFC libraries in the correct order Mike Pollett
#include "../../forcelib.h"
// Mike Pollett
#include "../../tdmf.inc"

#include "ftd_port.h"
#include "ftd_proc.h"
#include "ftd_sock.h"
#include "ftd_error.h"
#include "ftd_cfgsys.h"
#include "ftd_proc.h"
#include "ftd_ps.h"
#include "ftd_lg_states.h"
#include "ftd_cpoint.h"
#ifdef TDMF_COLLECTOR
#include "ftd_mngt.h"
#endif

#include "iputil.h"


#if defined(_WINDOWS)
#include "ftd_devlock.h"
#endif

static LList        *proclist;      /* list of child process objects    */
                                    
char** share_envp = (char**)NULL;   /* shared environment vector        */
char** share_argv = (char**)NULL;   /* shared argument vector           */

#if !defined(_WINDOWS)
static int      sigpipe[2];         /* signal queue                     */
#else
extern HANDLE ghTerminateEvent;
#endif

static int recv_cp(LList *proclist, ftd_sock_t *fsockp,
    ftd_header_t *header); 
static int recv_child_ack(LList *proclist, ftd_header_t *header); 
static int recv_start_pmd(LList *proclist, ftd_sock_t *listener,
    ftd_sock_t *fsockp, ftd_header_t *header, char **envp); 
static int recv_start_apply(LList *proclist, ftd_sock_t *listener,
    ftd_sock_t *fsockp, ftd_header_t *header, char **envp); 
static int recv_start_reco(LList *proclist, ftd_sock_t *listener,
    ftd_sock_t *fsockp, ftd_header_t *header, char **envp); 
static int recv_stop_apply(LList *proclist, ftd_header_t *header); 
static int recv_apply_done_cpon(LList *proclist, ftd_sock_t *fsockp, 
    ftd_header_t *header, char **envp); 
static int recv_ipc_msg(LList *proclist, ftd_sock_t *listener,
    ftd_sock_t *fsockp, ftd_header_t *header, char **envp); 
static int get_connect_type(ftd_sock_t *fsockp, ftd_header_t *header);
static void proc_child_io(LList *proclist, fd_set *readset,
    ftd_sock_t *listener, char **envp);
static void dispatch_io(LList *proclist,
    ftd_sock_t *listener, int sigqueue, char **envp);
static int recv_child_connect(LList *proclist, ftd_sock_t *listener,
    ftd_sock_t *fsockp, ftd_header_t *header); 

struct RemGroupInBasket
{
    int bTodo;
    char lcaHostName[48];
};

static void proc_envp(char **envp); 
void proc_argv(int argc, char **argv); 

/*
 * recv_child_ack --
 * handle an ack packet from child
 * ack the command - if any - so it can go away
 */
static int
recv_child_ack(LList *proclist, ftd_header_t *header) 
{
    ftd_proc_t      *fprocp;
    int             rc = 0;

    ftd_sock_t      **csockpp;

    if ((fprocp = ftd_proc_lgnum_to_proc(proclist,
        header->msg.lg.lgnum, header->msg.lg.data)) == NULL)
    {
        return -1;
    }   
    
    switch(header->msgtype) {
    case FTDCCPON:
    case FTDCCPOFF:
    case FTDACKCPERR:
        header->msgtype = FTDACKCLI;
        break;
    default:
        break;
    }

    csockpp = HeadOfLL(fprocp->csockplist);

    if (csockpp) {
        if (FTD_SOCK_VALID(*csockpp)) {
            // send header to command
            rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_child_ack",*csockpp, header);
        }
        // remove sock object from proc connection list
        RemCurrOfLL(fprocp->csockplist, csockpp);

        // destroy the command socket object 
        // the command line program will die when it notices the close 
        ftd_sock_delete(csockpp);
    }

    return rc;
}

static int
recv_signal(LList *proclist, ftd_sock_t *fsockp, ftd_header_t *header)
{
    ftd_proc_t  **fprocpp, *fprocp;
    int     rc, found = 0;

    ForEachLLElement(proclist, fprocpp) {
        fprocp = *fprocpp;
        if (!strcmp(fprocp->procp->procname, header->msg.data)) {
            found = 1;
            break;
        }
    }

    if (found)
    {
        if (proc_signal(fprocp->procp, header->msgtype) != -1)
            header->msgtype = FTDACKCLI;
        else {
            header->msgtype = FTDACKERR;
        }
        header->msg.lg.data = 1;    // run state
    } else {
        // target thread not running
        header->msgtype = FTDACKCLI;
        header->msg.lg.data = 0;    // run state
    }

    rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_signal",fsockp, header);
    return rc;
}

/*
 * recv_cp --
 * handle a checkpoint request from command
 */
static int
recv_cp(LList *proclist, ftd_sock_t *fsockp, ftd_header_t *header) 
{
    ftd_proc_t  *fprocp;
    int     role;

    if (header->msgtype == FTDCCPSTARTP
        || header->msgtype == FTDCCPSTOPP)
        { 
        role = ROLEPRIMARY;
        } 
    else 
        {
        role = ROLESECONDARY;
        }

    // find the target group process
    if ((fprocp = ftd_proc_lgnum_to_proc(proclist,header->msg.lg.lgnum, role)) != NULL)
        {
        // group process running OK
        if (!FTD_SOCK_VALID(fprocp->fsockp)) 
            {
            // child running but connection not valid -
            // ack the command and let it retry if it wants to
            header->msgtype = FTDACKERR;
            FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_cp",fsockp, header);
            return 0;
            }
    
        // tell child
        if (FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_cp",fprocp->fsockp, header) < 0) 
            {
            return -1;
            }
        
        // save command sock - so command stays around until it's acked 
        fsockp->keepalive = TRUE;
        AddToTailLL(fprocp->csockplist, &fsockp);
        } 
    else {
        // group process not running
        if (role == ROLEPRIMARY) 
            {
            // checkpoint primary type ack
            header->msgtype = FTDACKNOPMD;
            } 
        else 
            {
            // checkpoint secondary type ack 
            header->msgtype = (header->msgtype == FTDCCPSTARTS ? FTDACKDOCPON:  FTDACKDOCPOFF);
            }
        
        // send error ack to checkpoint command 
        if (FTD_SOCK_VALID(fsockp)) {

            if (FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_cp",fsockp, header) < 0) {
                return -1;
            }
        }
    }

    return 0;
}

/*
 * recv_start_pmd --
 * handle a start pmd request from the launch command
 */
static int
recv_start_pmd(LList *proclist, ftd_sock_t *listener, 
    ftd_sock_t *fsockp, ftd_header_t *header, char **envp) 
{
    ftd_proc_t  **fprocpp, *fprocp;
    int     found = 0;

    ForEachLLElement(proclist, fprocpp) {
        fprocp = *fprocpp;
        if (header->msg.lg.lgnum == fprocp->lgnum
            && fprocp->proctype == FTD_PROC_PMD)
        {
            found = 1;
            break;
        }
    }

    if (found) {
        if (!FTD_SOCK_VALID(fprocp->fsockp)) { 
            // child running but connection not valid -
            // ack the command and let it retry if it wants to
            header->msgtype = FTDACKERR;
            FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_start_pmd",fsockp, header);
            return 0;
        }
        // tell child
        if (FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_start_pmd",fprocp->fsockp, header) < 0) {
            return -1;
        }
    } else {
        // if process not running - start it
#if defined(_WINDOWS)       
        if ((fprocp = ftd_proc_create(PROCTYPE_THREAD)) == NULL) {
            return -1;
        }
        fprocp->procp->function = PrimaryThread; // XXX JRL, outof band data
#else
        if ((fprocp = ftd_proc_create(PROCTYPE_PROC)) == NULL) {
            return -1;
        }
#endif
        
        if (ftd_proc_exec_pmd(proclist, fprocp, listener,
            header->msg.lg.lgnum,
            header->msg.lg.data,
            0, envp) < 0)
        {
            ftd_proc_delete(fprocp);    
            return -1;
        }
        ftd_proc_add_to_list(proclist, &fprocp);
    }

    // save command sock - so command stays around until it's acked 
    fsockp->keepalive = TRUE;
    AddToTailLL(fprocp->csockplist, &fsockp);

    return 0;
}

/*
 * recv_start_apply --
 * handle a start apply request from a child rmd
 */
static int
recv_start_apply(LList *proclist, ftd_sock_t *listener,
    ftd_sock_t *fsockp, ftd_header_t *header, char **envp) 
{
    ftd_proc_t      *fprocp, **fprocpp, *rmdp;
    ftd_header_t    ack;
    char            procname[MAXPATHLEN];
    int             running = 0;

    sprintf(procname, "RMDA_%03d", header->msg.lg.lgnum);

    // if it's already running then skip it
    ForEachLLElement(proclist, fprocpp) {
        if (!strcmp((*fprocpp)->procp->procname, procname)) {
            running = 1;
            goto errret;
        }
    }
    
#if defined(_WINDOWS)       
    if ((fprocp = ftd_proc_create(PROCTYPE_THREAD)) == NULL) {
        goto errret;
    }
    fprocp->procp->function = RemoteThread; // XXX JRL, outof band data
    fprocp->procp->command = (LPDWORD)&fprocp->args;
#else
    if ((fprocp = ftd_proc_create(PROCTYPE_PROC)) == NULL) {
        goto errret;
    }
#endif
    fprocp->lgnum = header->msg.lg.lgnum;

    if (ftd_proc_exec_apply(fprocp, listener,
        header->msg.lg.lgnum, header->msg.lg.data, envp) < 0)
    {
        ftd_proc_delete(fprocp);    
        goto errret;
    }

    ftd_proc_add_to_list(proclist, &fprocp);

errret:

    memset(&ack, 0, sizeof(ack));
    
    ack.msgtype = FTDCNOOP;
    ack.msg.lg.data = running;

    // it might be the rmd talking to us -
    // just send it a NOOP in case it's waiting
    if ((rmdp = ftd_proc_lgnum_to_proc(proclist,
        header->msg.lg.lgnum, ROLESECONDARY)))
    {
        if (FTD_SOCK_CONNECT(rmdp->fsockp)) {
            FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_start_apply",rmdp->fsockp, &ack);
        }
    } else {
        // it's cli talking to us
        if (FTD_SOCK_CONNECT(fsockp)) {

            FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_start_apply",fsockp, &ack);
        }
    }

    return 0;
}

/*
 * recv_start_reco --
 * handle a start reco request
 */
static int
recv_start_reco(LList *proclist, ftd_sock_t *listener,
    ftd_sock_t *fsockp, ftd_header_t *header, char **envp) 
{
    ftd_proc_t      **fprocpp;
    ftd_header_t    ack;
    char            procname[MAXPATHLEN];
    int             force, running = FALSE;

    // generic ack
    memset(&ack, 0, sizeof(ack));
    ack.msgtype = FTDACKCLI;

    sprintf(procname, "RMD_%03d", header->msg.lg.lgnum);

    // if group rmd running then can't do
    ForEachLLElement(proclist, fprocpp) {
        if (!strcmp((*fprocpp)->procp->procname, procname)) {
            running = TRUE;
            break;
        }
    }
    
    // data field contains force flag
    force = header->msg.lg.data;

    if (running) {
        if (force) {
            // kill it
            if (proc_signal((*fprocpp)->procp, FTDCSIGTERM) < 0) {
                ack.msg.lg.data = -1;
                goto errret;
            }
        } else {
            ack.msg.lg.data = running;
            goto errret;
        }
    }

    // start the apply process for the group
    if (recv_start_apply(proclist, listener, fsockp, header, envp) < 0) {
        ack.msg.lg.data = -1;
        goto errret;
    }

    // it's dtcreco talking to us
    if (FTD_SOCK_CONNECT(fsockp)) {
        FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_start_reco",fsockp, &ack);
    }

    return 0;

errret:

    // it's dtcreco talking to us
    if (FTD_SOCK_CONNECT(fsockp)) {
        FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_start_reco",fsockp, &ack);
    }

    return -1;
}

/*
 * recv_stop_apply --
 * handle a stop apply request from a child rmd
 */
static int
recv_stop_apply(LList *proclist, ftd_header_t *header) 
{
    ftd_proc_t      **fprocpp, *rmdp;
    ftd_header_t    ack;
    char            procname[MAXPATHLEN];

    sprintf(procname, "RMDA_%03d", header->msg.lg.lgnum);

    if (ftd_proc_terminate(procname) < 0) {
        return -1;
    }
    
    // if it's still not dead make sure it dies
    if (ftd_proc_kill(procname) < 0) {
        return -1;
    }

    // remove proc from list
    ForEachLLElement(proclist, fprocpp) 
    {
        if (!strcmp((*fprocpp)->procp->procname, procname)) 
        {
            ftd_proc_remove_from_list(proclist, fprocpp);
            break;
        }
    }
    
    // it is the rmd talking to us -
    // just send it a NOOP in case it's waiting
    if ((rmdp = ftd_proc_lgnum_to_proc(proclist,
        header->msg.lg.lgnum, ROLESECONDARY))) {

        memset(&ack, 0, sizeof(ack));
        ack.msgtype = FTDCNOOP;


        FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_stop_apply",rmdp->fsockp, &ack);
    }

    return 0;
}

/*
 * recv_apply_done_cpon --
 * handle a cpon msg packet from apply process 
 */
static int
recv_apply_done_cpon(LList *proclist, ftd_sock_t *fsockp, 
    ftd_header_t *header, char **envp) 
{
    ftd_lg_t    *lgp = NULL;
    ftd_proc_t  **fprocpp, *fprocp;
    int     found = 0;

    ForEachLLElement(proclist, fprocpp) {
        fprocp = *fprocpp;
        if (header->msg.lg.lgnum == fprocp->lgnum
            && fprocp->proctype == FTD_PROC_RMD)
        {
            found = 1;
            break;
        }
    }

    if (found) {
        // send the rmd the msg
        header->msgtype = FTDCCPON;
        if (FTD_SOCK_SEND_HEADER( TRUE,__FILE__ ,"recv_apply_done_cpon",fprocp->fsockp, header) < 0) {
            goto errret;
        }
    } else {
        // if process not running - do cpon shit here 
        if ((lgp = ftd_lg_create()) == NULL) {
            goto errret;
        }
        if (ftd_lg_init(lgp, header->msg.lg.lgnum, ROLESECONDARY, 0) < 0) {
            goto errret;
        }
        if (ftd_lg_cpon(lgp) < 0) {
            goto errret;
        }
    }

    if (lgp)
        ftd_lg_delete(lgp);

    // ack the rmda process
    header->msgtype = FTDACKCLI;
    if (FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_apply_done_cpon",fsockp, header) < 0) {
        return -1;
    }

    return 0;

errret:
    
    if (lgp)
        ftd_lg_delete(lgp);

    // ack the rmda process
    header->msgtype = FTDACKERR;

    FTD_SOCK_SEND_HEADER( TRUE,__FILE__ ,"recv_apply_done_cpon",fsockp, header);

    return -1;
}

/*
 * recv_trace_level
 * process an incoming FTDCSETTRACELEVEL message.
 */
static int recv_trace_level(ftd_sock_t *fsockp, ftd_header_t *header)
{
    //the first byte of the all-purpose data buffer in the header contains the trace level
    error_SetTraceLevel ( (unsigned char)header->msg.data[0] );
    error_tracef(TRACEWRN , "recv_trace_level(): new level=%d", (int)header->msg.data[0] );

    // send back ACK 
    header->msgtype = FTDCSETTRACELEVELACK;
    FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_trace_level",fsockp, header);

    return -1;//return < 0 because this fnct does not use the socket.
}

/*
 * recv_rem_start
 * process an incoming start message from remote. WR14440 & WR16562
 */
static int recv_remote_wakeup(ftd_sock_t *fsockp, ftd_header_t *header)
{
    ftd_lg_t            *lgp = NULL;
    char                *psname, *groupname;
    ps_group_info_t     group_info;
    int                 rc = 0;
    int                 pstate;
    char                command[MAXPATH];
    char                groupstr[MAXPATH];

    HANDLE          hFile;
    WIN32_FIND_DATA data;
    PROCESS_INFORMATION ProcessInfo;
    // Set up the start up info struct.
    STARTUPINFO si;

    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    /* Verify if LGn is started  */
    sprintf(command, "%s\\p%03d.cur", PATH_CONFIG, header->msg.lg.lgnum);
    hFile = FindFirstFile(command, &data);

    if (hFile == INVALID_HANDLE_VALUE)
        header->msgtype = FTDACKERR;
    else
    {
        // Verify the previous LG state stamp, if it was LG_ACTIVE, start his PMD here
        if ((lgp = ftd_lg_create()) == NULL) 
            error_tracef( TRACEINF6, "MASTER: recv_remote_wakeup(), can't create LG%d \n", header->msg.lg.lgnum );
        else if (ftd_lg_init(lgp, header->msg.lg.lgnum, ROLEPRIMARY, 0) < 0) 
            error_tracef( TRACEINF6, "MASTER: recv_remote_wakeup(), can't init LG%d \n", header->msg.lg.lgnum );
        else
        {
            psname = lgp->cfgp->pstore;
            groupname = lgp->devname;

            // get the group info to see if this group exists
            group_info.name = NULL;

            rc = ps_get_group_info(psname, groupname, &group_info);

            if (rc == PS_OK) 
            {       
                // see if the group was at NORMAL level previously (not shutdown properly)
                if (group_info.state_stamp == LG_ACTIVE) 
                {
                    // pstore state
                    pstate = ftd_lg_get_pstore_run_state(lgp->lgnum, lgp->cfgp);
                    error_tracef( TRACEINF, "MASTER: recv_remote_wakeup(LG%d), was LG_ACTIVE \n", header->msg.lg.lgnum );
                    strcpy(command, PATH_RUN_FILES);
                    SetCurrentDirectory(command);
                    switch ( pstate ) 
                    {
                        case FTD_SREFRESHC:
                            sprintf(command, CMDPREFIX"launchrefresh.exe -g%d -c", header->msg.lg.lgnum);
                            break;
                        case FTD_SREFRESHF:
                            sprintf(command, CMDPREFIX"launchrefresh.exe -g%d -f", header->msg.lg.lgnum);
                            break;
                        case FTD_SNORMAL:
                        case FTD_SREFRESH:
                        default:
                            sprintf(command, CMDPREFIX"launchpmd.exe -g%d", header->msg.lg.lgnum);
                            break;                  
                    }

                    sprintf(groupstr, "REMOTE WAKEUP, %s cmd initiated ", command );
                    reporterr( ERRFAC, M_REMWAKEUP, ERRWARN, groupstr );

                    rc = CreateProcess(NULL, command, NULL, NULL, TRUE,
                                        CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
                }
                else
                {
                    error_tracef( TRACEINF, 
                                  "MASTER: recv_remote_wakeup(LG%d), was Inactive/Started \n", 
                                  header->msg.lg.lgnum );
                }
            }
            else
            {
                error_tracef( TRACEINF, "MASTER: recv_remote_wakeup(LG%d), PS_NOT_FOUND \n", header->msg.lg.lgnum );
            }
        }
    }
    return 0;
}

/*
 * recv_remote_drive_error
 * process an incoming start message from remote, Autoamtic Smart Refresh relaunch. WR17511
 */
static int recv_remote_drive_error(ftd_sock_t *fsockp, ftd_header_t *header)
{
    ftd_lg_t            *lgp = NULL;
    char                *psname, *groupname;
    ps_group_info_t     group_info;
    int                 rc = 0;
    int                 pstate;
    char                command[MAXPATH];
    int                 iValidProcess = 0;
    PROCESS_INFORMATION ProcessInfo;
    // Set up the start up info struct.
    STARTUPINFO si;

    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    if ((lgp = ftd_lg_create()) == NULL) 
        error_tracef( TRACEINF6, "MASTER: recv_remote_drive_error(), can't create LG%d \n", header->msg.lg.lgnum );
    else if (ftd_lg_init(lgp, header->msg.lg.lgnum, ROLEPRIMARY, 0) < 0) 
        error_tracef( TRACEINF6, "MASTER: recv_remote_drive_error(), can't init LG%d \n", header->msg.lg.lgnum );
    else
    {
        psname = lgp->cfgp->pstore;
        groupname = lgp->devname;

        // get the group info to see if this group exists
        group_info.name = NULL;

        rc = ps_get_group_info(psname, groupname, &group_info);

        if (rc == PS_OK) 
        {   
            // see if the group was ACTIVE previously
            if (group_info.state_stamp == LG_ACTIVE) 
            {
                // pstore state
                pstate = ftd_lg_get_pstore_run_state(lgp->lgnum, lgp->cfgp);

                strcpy(command, PATH_RUN_FILES);
                SetCurrentDirectory(command);

                switch ( pstate ) 
                {
                    case FTD_SREFRESH:
                        sprintf(command, CMDPREFIX"launchrefresh.exe -g%d -p", header->msg.lg.lgnum);
                        iValidProcess = 1;
                        break;
                    case FTD_SNORMAL:
                        sprintf(command, CMDPREFIX"launchrefresh.exe -g%d -p", header->msg.lg.lgnum);
                        iValidProcess = 1;
                        break;
                    case FTD_SREFRESHC:
                    case FTD_SREFRESHF:
                        break;
                    default:
                        break;                  
                }

                error_tracef( TRACEINF, "MASTER: recv_remote_drive_error(LG%d), was ACTIVE \n", header->msg.lg.lgnum );

                if ( iValidProcess )
                {
                    rc = CreateProcess(NULL, command, NULL, NULL, TRUE,
                                    CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
                }
            }
            else
            {
                error_tracef( TRACEINF, 
                              "MASTER: recv_remote_drive_error(LG%d), was Inactive/Started \n", 
                               header->msg.lg.lgnum );
            }
        }
        else
        {
            error_tracef( TRACEINF, "MASTER: recv_remote_drive_error(LG%d), PS_NOT_FOUND \n", header->msg.lg.lgnum );
        }
    }

    return 0;
}

/*
 * recv_refreshs_relaunch
 * process an incoming SMART REFRESH RELAUNCH message from RMDA, Automatic Smart Refresh relaunch. WR17511
 */
static int recv_refreshs_relaunch( LList *proclist, ftd_sock_t *fsockp, ftd_header_t *header )
{
    int         rc = 0;
    ftd_proc_t  **fprocpp, *fprocp;

    ForEachLLElement(proclist, fprocpp) 
    {
        fprocp = *fprocpp;
        if (header->msg.lg.lgnum == fprocp->lgnum && fprocp->proctype == FTD_PROC_RMD)
        {
            // send to RMD the FTDCREMDRIVEERR signal
            header->msg.lg.flags = ROLESECONDARY;
            header->msg.lg.data  = ~ROLESECONDARY;
            header->msgtype = FTDCREMDRIVEERR;
            FTD_SOCK_SEND_HEADER( TRUE,__FILE__ ,"recv_refreshs_relaunch",fprocp->fsockp, header);
        }
    }

    // ack the rmda process
    header->msgtype = FTDACKCLI;
    FTD_SOCK_SEND_HEADER( FALSE ,__FILE__ ,"recv_refreshs_relaunch",fsockp, header);
    return 0;
}

// WR14440 & WR16562
// This function create a list of 'remote' secondary groups and send a
// FTDCREMWAKEUP msg to related Primary Master Thread.
//

DWORD ftd_rem_wakeup( void ) 
{
    ftd_header_t    lFtdHeader;
    ftd_sock_t*     lpFtdSock      = NULL;
    ftd_lg_cfg_t**  lppFtdLgCfg;
    LList*          lpLstCfg       = NULL;
    int             liLgCnt        = 0;
    int             liRc;
    int             iPortNum;

    struct RemGroupInBasket laInBasket [ FTD_MAX_GRP_NUM ];
    char lcSecHostName[MAXHOST];

    for ( liLgCnt = 0; liLgCnt < FTD_MAX_GRP_NUM; liLgCnt++ )
        laInBasket [ liLgCnt ].bTodo    = FALSE;

    if ( !(iPortNum = ftd_sock_get_port( FTD_MASTER )) )
    {
        iPortNum = FTD_SERVER_PORT;
    }

    if ( (lpLstCfg = ftd_config_create_list()) == NULL )
    {
        ExitThread(0);
    }
    else
    {
        // 
        // Create a list of all existing SECONDARY group(s), by refering to sxxx.cfg
        // and tag this group if not in LOOPBACK mode
        //
        liLgCnt = 0;

        if ( ftd_config_get_secondary( PATH_CONFIG, lpLstCfg ) < 0 )
        {
            ftd_config_delete_list( lpLstCfg );
                ExitThread(0);
        }
        else ForEachLLElement( lpLstCfg, lppFtdLgCfg )  
        {
            if ( !ftd_lg_get_hostip((*lppFtdLgCfg)->lgnum , ROLESECONDARY ,(char *)(laInBasket[(*lppFtdLgCfg)->lgnum].lcaHostName) , lcSecHostName) )
            {
                // Command not permitted if LOOPBACK mode
                if ( strncmp( laInBasket[(*lppFtdLgCfg)->lgnum].lcaHostName, lcSecHostName, MAXHOST ) ) 
                {
                    laInBasket [ (*lppFtdLgCfg)->lgnum ].bTodo = TRUE;
                    liLgCnt++;
                }
            } 
        }

        memset ( &lFtdHeader, 0, sizeof ( lFtdHeader ) );
        lFtdHeader.magicvalue = MAGICHDR;
        lFtdHeader.cli        = (HANDLE)1;
    
        // For each validated SECONDARY group, send a FTDCREMWAKEUP msg to the related Primary Master Thread,
        // verify the ACK message if the command is properly received/executed                                  

        if ( liLgCnt )
        {
            ForEachLLElement( lpLstCfg, lppFtdLgCfg )
            {
                lFtdHeader.msg.lg.lgnum = (*lppFtdLgCfg)->lgnum;
                lFtdHeader.msgtype = FTDCREMWAKEUP;
                lFtdHeader.ackwanted = 0;

                if ( laInBasket [ (*lppFtdLgCfg)->lgnum ].bTodo )
                {
                    if ( (lpFtdSock = ftd_sock_create( FTD_SOCK_GENERIC )) == NULL )
                        ExitThread(0);

                    // Send Remote Start message to Primary Master for targeted group
                    if ( ftd_sock_init( lpFtdSock, "localhost", laInBasket[(*lppFtdLgCfg)->lgnum].lcaHostName , 0, 0,
                                        SOCK_STREAM, AF_INET, 1, 0 ) < 0 )
                        continue;

                    FTD_SOCK_PORT( lpFtdSock ) = iPortNum;
                
                    liRc = ftd_sock_connect_nonb( lpFtdSock, iPortNum, 3, 0, 1 );

                    if (liRc != 1)
                    {
                        ftd_sock_delete( &lpFtdSock );
                        continue;
                    }

                    if ( FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"main" , lpFtdSock, &lFtdHeader ) < 0 )
                    {
                        ftd_sock_delete( &lpFtdSock );
                        continue;
                    }
                    ftd_sock_delete( &lpFtdSock );
                }
            } // ForEachLLElement( lpLstCfg, lppFtdLgCfg )
        } // if ( liLgCnt )
    }

    //
    // delete the list
    //
    if (lpLstCfg)
        ftd_config_delete_list( lpLstCfg );

    ExitThread(0);

} // ftd_rem_wakeup()

/*
 * recv_ipc_msg --
 * read a ftd ipc message header and dispatch accordingly
 */
static int
recv_ipc_msg(LList *proclist, ftd_sock_t *listener, ftd_sock_t *fsockp,
    ftd_header_t *header, char **envp) 
{
    int rc = 0;

    error_tracef( TRACEINF, "Master Rx msg: %s", tdmf_socket_msg[header->msgtype-1].cmd_txt );
        
    switch(header->msgtype) {
    case FTDCCPSTARTP:
    case FTDCCPSTARTS:
    case FTDCCPSTOPP:
    case FTDCCPSTOPS:
        // from checkpoint 
        rc = recv_cp(proclist, fsockp, header);
        break;
    case FTDCCPON:
    case FTDCCPOFF:
    case FTDACKCPERR:
    case FTDACKCLD:
        // from pmd, rmd 
        rc = recv_child_ack(proclist, header);
        break;
    case FTDCSTARTPMD:
        // from launch
        rc = recv_start_pmd(proclist, listener, fsockp, header, envp);
        break;
    case FTDCSTARTAPPLY:
        // from rmd 
        rc = recv_start_apply(proclist, listener, fsockp, header, envp);
        break;
    case FTDCSTOPAPPLY:
        // from rmd 
        rc = recv_stop_apply(proclist, header);
        break;
    case FTDCAPPLYDONECPON:
        // from rmda
        rc = recv_apply_done_cpon(proclist, fsockp, header, envp);
        break;
    case FTDCSTARTRECO:
        // from dtcreco 
        rc = recv_start_reco(proclist, listener, fsockp, header, envp);
        break;
    case FTDCSIGTERM:
    case FTDCSIGUSR1:
        // from kill
        rc = recv_signal(proclist, fsockp, header);
        break;
#ifdef TDMF_COLLECTOR
    case FTDCMANAGEMENT:
        // from TDMF Collector
        rc = ftd_mngt_recv_msg(fsockp);
        break;
#endif
    case FTDCSETTRACELEVEL:
        // from tdmftrace
        rc = recv_trace_level(fsockp,header);
        break;

    case FTDCREMWAKEUP:
        // remote wakeup call !! WR14440 & WR16562
        rc = recv_remote_wakeup(fsockp,header);
        break;

    case FTDCREMDRIVEERR:
        // FTDCREMDRIVEERR sent from PMD to Primary Master!! WR17511
        rc = recv_remote_drive_error(fsockp, header);
        break;

    case FTDCREFRSRELAUNCH:
        // from rmda !! WR17511
        rc = recv_refreshs_relaunch(proclist, fsockp, header);
        break;

    default:
        break;
    }

    return rc;
}

/*
 * recv_child_connect --
 * handle a connection from a child process
 */
static int
recv_child_connect(LList *proclist, ftd_sock_t *listener,
    ftd_sock_t *fsockp, ftd_header_t *header) 
{
    ftd_header_t    ack;
    ftd_proc_t  **fprocpp, *fprocp;
    char        hostname[MAXHOST];

    ftd_sock_t  **csockpp;

    // assign fsockp to target fprocp
    ForEachLLElement(proclist, fprocpp) {
        fprocp = *fprocpp;
        
        if (fprocp->procp->pid == header->cli) {

            // found child process

            // copy hostname for id 
            gethostname(hostname, sizeof(hostname));

            // always local->local for child
            strcpy(fsockp->sockp->lhostname, hostname);
            strcpy(fsockp->sockp->rhostname, hostname);
            fsockp->sockp->port = listener->sockp->port;

            // save child thread sock - so we can use it in the future 
            fsockp->keepalive = TRUE;
            fprocp->fsockp = fsockp;

            if (fprocp->proctype == FTD_PROC_PMD) {

                csockpp = HeadOfLL(fprocp->csockplist);

                // we don't have a command channel on restart of PMD
                if (csockpp) {
                    if (FTD_SOCK_VALID(*csockpp)) {
                        // send ack to launch command - we saved
                        // the socket in csockp
                        memset(&ack, 0, sizeof(ack));
                        ack.msgtype = FTDACKCLI;

                        if (FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"recv_child_connect",*csockpp, &ack) < 0) {
                            return 0;
                        }
                    }
                    // remove sock object from proc connection list
                    RemCurrOfLL(fprocp->csockplist, csockpp);
                    
                    // destroy the command socket object
                    ftd_sock_delete(csockpp);
                }
            }
    
            break;
        }
    }   

    return 0;
}

/*
 * proc_child_io -- process child ipc events
 */
static void
proc_child_io(LList *proclist, fd_set *readset, ftd_sock_t *listener,
    char **envp)
{
    ftd_proc_t      **fprocpp, *fprocp;
    ftd_header_t    header;
    int             rc;
    SOCKET          sock;

    // check child communication channels
    ForEachLLElement(proclist, fprocpp) {
        fprocp = *fprocpp;
        if (!FTD_SOCK_VALID(fprocp->fsockp)) {
            continue;
        }
        sock = FTD_SOCK_FD(fprocp->fsockp);
        if (sock == (SOCKET)-1 || sock == 0) {
            continue;
        }

        if (FD_ISSET(sock, readset)) {
            // get ipc msg from child
            rc = FTD_SOCK_RECV_HEADER(__FILE__ ,"proc_child_io",fprocp->fsockp, &header);
            if (rc <= 0) {
                // socket error - or connection closed
                ftd_sock_delete(&fprocp->fsockp);
                continue;
            }

            if (recv_ipc_msg(proclist, listener, 
                fprocp->fsockp, &header, envp) < 0) {
            }
        }
    }

    return;
}

/*
 * get_connect_type --
 * read first four bytes from connection to determine connection request type
 */
static int
get_connect_type(ftd_sock_t *fsockp, ftd_header_t *header)
{
    int     rc, len, ret = -1;
    char    *msg, *savemsg;

    msg = savemsg = (char*)header;
    len = 4;

    // timeout if no i/o coming
    // - fucking kluge - where is the connect coming from ??? 

    if (ftd_sock_check_recv(fsockp, 100000) <= 0) {
        return -1;
    }

    /*
     * read first 4 characters to see if this is a valid request 
     * this is a blocking read
     */
    //<<AC>>:bug if loop more than once : rc not cumulative ? 
    //       ...but it is ok because ftd_sock_recv always returns 3rd param ...  loop unnecessary here
    while (len > 0) {
        rc = ftd_sock_recv(fsockp, msg, 4);
        if (rc < 0) {
            // connection error 
            ret = rc;
            goto done;
        } else if (rc == 0) {
            ret = -1;
            goto done;
        } else {
            len -= rc;
            msg += rc;
        }
    }

    msg = savemsg;
    if (!strncmp((char*)msg, "ftd ", 4)) {
        // that's all there is 
        ret = FTD_CON_UTIL;
        goto done;
    }

    //<<AC>>:bug if loop above more than once : rc not cumulative
    // get rest of protocol header 
    rc = ftd_sock_recv(fsockp, (char*)msg+rc,
        sizeof(ftd_header_t)-rc); 

    if (rc < 0) {
        ret = rc;
        goto done;
    }

    if (header->cli == (HANDLE)1) {
        return FTD_CON_IPC;
    } else if (header->cli > (HANDLE)1) {
        return FTD_CON_CHILD;
    } else {
        return FTD_CON_PMD;
    }

done:

    return ret;
}

/*
 * dispatch_io -- process io events requests
 */
static void
dispatch_io(LList *proclist, ftd_sock_t *listener,
    int sigqueue, char **envp)
{
    fd_set          readset;
    ftd_proc_t      **fprocpp, *fprocp;
    ftd_sock_t      *fsockp;
    ftd_header_t    header;
    int             nselect = 0, n, contype;
    struct timeval  seltime;
    int             loop = 1;
#if !defined(_WINDOWS)
    int             s;
#else
    int             lbDoneReaping = 0;
    int             liReaped      = 0;
#endif
    int             liWakeUpDone    = 0;
    int             lbRemStartSig = 0;
    DWORD           dwThreadId; 
    HANDLE          hThread;
    char            szMasterMsg[MAXPATH];

#ifdef TDMF_COLLECTOR
    ///////////////////////////////////////////////////
    // Management stuff
    ftd_mngt_initialize();
    //if TDMF Collector is known, send our TDMF Agent information
    ftd_mngt_send_agentinfo_msg(0, 0);
    ftd_mngt_send_registration_key();
    //
    ///////////////////////////////////////////////////
#endif

    seltime.tv_sec = 1;
    seltime.tv_usec = 0;

    while (loop) 
        {
        FD_ZERO(&readset);
        FD_SET(FTD_SOCK_FD(listener), &readset);

        #if !defined(_WINDOWS)
            FD_SET(sigqueue, &readset);

            nselect = max(FTD_SOCK_FD(listener), sigqueue);
        #endif

        ForEachLLElement(proclist, fprocpp) 
            {
            fprocp = *fprocpp;
            if (FTD_SOCK_VALID(fprocp->fsockp)
                && FTD_SOCK_FD(fprocp->fsockp) != (SOCKET)-1)
                {
                FD_SET(FTD_SOCK_FD(fprocp->fsockp), &readset);
                nselect = max(nselect, FTD_SOCK_FD(fprocp->fsockp));
                }
            }

        n = select(++nselect, &readset, NULL, NULL, &seltime);
       
        switch(n) 
        {
        case -1:
        case 0:
            break;
        default:

            // handle the request
            #if !defined(_WINDOWS)
            if (FD_ISSET(sigqueue, &readset)) 
                {
                /* got a signal */
                if (read(sigqueue, (char*)&s, sizeof(int)) < 0) 
                    {
                    break;
                    }
                if (s == SIGHUP) 
                    {
                    // hup all pmd's
                    ftd_proc_hup_pmds(proclist, listener, 0, envp);
                    } 
                else if (s == SIGCHLD) 
                    {
                    ftd_proc_reaper(proclist, listener, envp);
                    } 
                else 
                    {
                    // unhandled signal
                    }
                } 
            #endif  
            //
            //<<AC>>:est-ce le socket listener qui a recu un msg ?
            //
            if (FD_ISSET(FTD_SOCK_FD(listener), &readset)) 
                {
                //<<AC>>: oui
                if ((fsockp = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL) 
                    {
                    break;
                    }
                // got a ipc connection request 
                if (ftd_sock_accept(listener, fsockp) < 0) 
                    {
                    reporterr (ERRFAC, M_SOCKACCEPT, ERRCRIT,   FTD_SOCK_FD(listener), sock_strerror(sock_errno()));
                    ftd_sock_delete(&fsockp);
                    break;
                    }

                // validate it
                fsockp->magicvalue = FTDSOCKMAGIC;

                // default to blowing away the socket object when
                // finished with it
                fsockp->keepalive = FALSE;

                memset(&header, 0, sizeof(header));

                // query connection type 
                //<<AC>>:ret value is from the enum contypes, so above or equal to 0
                contype = get_connect_type(fsockp, &header);

                if (contype < 0) 
                    {
                    ftd_sock_delete(&fsockp);
                    break;
                    } 
                else if (contype == FTD_CON_UTIL) 
                    {
                    // execute the command
                    if (ftd_proc_do_command(fsockp, (char*)&header) < 0) 
                        {
                        ftd_sock_delete(&fsockp);
                        break;
                        }
                    } 
                else if (contype == FTD_CON_PMD) 
                    {
                    #if defined(_WINDOWS)       
                        if ((fprocp = ftd_proc_create(PROCTYPE_THREAD)) == NULL) 
                            {
                            ftd_sock_delete(&fsockp);
                            break;
                            }
                        fprocp->procp->function = RemoteThread; // XXX JRL, outof band data
                        fprocp->procp->command = (LPDWORD)&fprocp->args;
                    #else
                        if ((fprocp = ftd_proc_create(PROCTYPE_PROC)) == NULL) 
                            {
                            ftd_sock_delete(&fsockp);
                            break;
                            }
                    #endif
                    // start the target rmd
                    if (ftd_proc_exec_rmd(proclist, fprocp, fsockp, listener,header.msg.lg.lgnum, envp) == 0)
                        {
                        ftd_proc_add_to_list(proclist, &fprocp);
                        }
                        else
                        {
                            ftd_proc_delete(fprocp);    
                        }
                    } 
                else if (contype == FTD_CON_IPC) 
                    {
                    // got a ipc connect request from CLI
                    if (recv_ipc_msg(proclist, listener, fsockp,&header, envp) < 0)
                        {
                        ftd_sock_delete(&fsockp);
                        break;
                        }
                    } 
                else if (contype == FTD_CON_CHILD) 
                    {
                    //
                    //<<AC>>:  in this situation a child process is a process running on the SAME host.
                    //          because recv_child_connect() relies on the header.cli and the pid 
                    //          also in this function, lhostname and rhostname are considered identical.

                    // got a ipc connect request from child process
                    recv_child_connect(proclist, listener, fsockp, &header);
                    }

                // blow away temporary socket object 
                if (FTD_SOCK_VALID(fsockp) && !fsockp->keepalive) 
                    {
                    ftd_sock_delete(&fsockp);
                }
            }  //   if (FD_ISSET(FTD_SOCK_FD(listener), &readset)) 
            else 
            {
                // must be a child process
                (void)proc_child_io(proclist, &readset, listener, envp);
            }

        } // switch

#if defined(_WINDOWS)

#ifdef TDMF_COLLECTOR
        //check if critical system parameters (e.g. BAB size) have been modified
        if ( ftd_mngt_sys_need_to_restart_system() )
        {   
            loop = 0;//leave this function
        }
#endif

        // ardeb 020919 v The folowing change is to ensure that 
        // "multiple reaping" is properly handled
        // see if anybody died
        lbDoneReaping = 0;
        liReaped      = 0;
        while ( !lbDoneReaping )
        {
            HANDLE       lhEvents [MAXIMUM_WAIT_OBJECTS];
            ftd_proc_t** lppProc  [MAXIMUM_WAIT_OBJECTS];
            int          liPid = 0;
            
            lhEvents[0] = ghTerminateEvent;

            ForEachLLElement(proclist, fprocpp) 
            {
                if ((*fprocpp)->procp->pid) 
                {
                    lppProc[++liPid] = fprocpp;
                    lhEvents[liPid]  = (*fprocpp)->procp->pid;
                }
            }

            //if (npid == 0) continue; // ardeb 020919 We dont need that

            n = WaitForMultipleObjects ( liPid+1, (CONST HANDLE *)&lhEvents, FALSE, 0 );
            
            switch(n)
            {
            case WAIT_TIMEOUT:
                    error_tracef( 
                        TRACEINF6, 
                        "dispatch_io():OK no more reaping for now"
                    );
                    lbDoneReaping = 1;
                    break;
            case WAIT_FAILED:
                    error_tracef( 
                        TRACEINF5, 
                        "dispatch_io():SJ no more reaping for now"
                    );
                    lbDoneReaping = 1; // we should retry before we exit
                break;
            case WAIT_OBJECT_0:
                loop = 0;//leave this function
                    error_tracef( 
                        TRACEWRN, 
                        "dispatch_io():WAIT_OBJECT_0 detected"
                    );
                lbDoneReaping = 1;//leave while( !lbDoneReaping )
                break;
            default:
                    if ( liReaped )
                    {
#if _DEBUG
                        OutputDebugString("Procreaper is called\n");
#endif
                        error_tracef( 
                            TRACEINF, 
                            "dispatch_io():Multiple Reaping Detected <%d|%d>",
                            liPid, liReaped
                        );
                    }

                    liReaped++;
                    ftd_proc_reaper( proclist, lppProc[n-WAIT_OBJECT_0], listener, envp );
                break;
            }
        } // while ( !lbDoneReaping )
        // ardeb 020919 ^
#endif

        // WR14440 & WR16562
        // Master Thread started for the first time, automatically signal the presence of SEC Master to
        // PRI Master (verify the existence of (sXXX.cfg files)
        
        if ( !liWakeUpDone )
        {
            liWakeUpDone = 1;
            // Start the Remote Wake-up thread
            // Send a WAKEUP Signal to Primary Master using ftd_rem_wakeup()

            sprintf(szMasterMsg, "REMOTE WAKEUP call(s) initiated for secondary groups");
            reporterr( ERRFAC, M_REMWAKEUP, ERRWARN, szMasterMsg );
            hThread = CreateThread( NULL,         
                                    0,          
                                    (LPTHREAD_START_ROUTINE)ftd_rem_wakeup,  
                                    0,            
                                    0,            
                                    &dwThreadId); 
            
            // Close this handle even if the thread is running  
            if (hThread != NULL) 
                CloseHandle( hThread );
        }

    } // while

#ifdef TDMF_COLLECTOR
    ftd_mngt_shutdown();
#endif

} //dispatch_io()

#if !defined(_WINDOWS)
/*
 * sig_handler -- master daemon signal handler
 */
static void
sig_handler(int s)
{
    int sig = s;

    switch(s) {
    case SIGTERM:
        ftd_proc_kill_all_pmd(proclist);
        ftd_proc_kill_all_apply(proclist);
        Return(0);
    default:
        break;
    }

    // save it 
    write(sigpipe[STDOUT_FILENO], &sig, sizeof(int));

    return;
}

/*
 * install_sigaction -- installs signal actions and creates signal pipe
 */
static void
install_sigaction(void)
{
    int                 i, numsig;
    struct sigaction    msaction;
    sigset_t            block_mask;
    static int          msignal[] = { SIGHUP, SIGCHLD, SIGTERM };

    sigemptyset(&block_mask);
    numsig = sizeof(msignal)/sizeof(*msignal);

    for (i = 0; i < numsig; i++) {
        msaction.sa_handler = sig_handler;
        msaction.sa_mask = block_mask;
        msaction.sa_flags = SA_RESTART;
        sigaction(msignal[i], &msaction, NULL);
    } 

    return;
}


/*
 * usage -- print usage message and Return 
 */
static void 
usage(int argc, char **argv) 
{

    printf ("%s usage:\n", argv[0]);
    printf ("%s [options]\n", argv[0]);
    printf ("  %s \\\n", argv[0]);
    printf ("  %s -version\n", argv[0]);
    printf ("  %s -help\n", argv[0]);

    Return(0);
}
#endif

/*
 * proc_argv -- process argv
 */
void 
proc_argv(int argc, char** argv) 
{
    int argcnt = 0;
       
    while (argcnt < argc) {
#if !defined(_WINDOWS)
        if (argcnt > 0) {
            if (0 == strcmp("-version", argv[argcnt])) {
                fprintf (stderr, "Version " VERSION "\n");
                Return(0);
            } else {
                usage(argc, argv);
            }
        }
#endif
        share_argv = (char**)realloc(share_argv, (argcnt+1) * sizeof(char**));
        share_argv[argcnt] = strdup(argv[argcnt]);
        argcnt++;
    }
    share_argv = (char**)realloc(share_argv, (argcnt+1) * sizeof(char**));
    share_argv[argcnt] = (char*)NULL;

    return;
}
 
/*
 * proc_envp -- process environment variables
 */
static void 
proc_envp(char** envp) 
{
    int envcnt = 0;

    /* count environment variables */
    while (envp[envcnt]) {
        share_envp = (char**)realloc(share_envp, (envcnt+1) * sizeof(char**));
        share_envp[envcnt] = strdup(envp[envcnt]);
        envcnt++;       
    }
    share_envp[envcnt] = (char*)NULL;

    return;
}


/*
 * MasterThread - FTD master thread
 */
DWORD 
MasterThread(LPDWORD param)
{
    char        **envp = NULL;
    ftd_proc_t  *throt;
    int         n,tcpwin,rc,portnum; 
    char        hostname[256], 
                tcpwinsize[256];
    ftd_sock_t  *listener;          /* socket listener objects  */

    if ( ftd_dev_lock_create() == -1) {     
        error_tracef( TRACEINF, "MASTER : ftd_dev_lock_create() error" );
        goto errexit;
    }

    if (ftd_init_errfac("Replicator", FTD_MASTER, NULL, NULL, 0, 0) == NULL) {
        Return(1);
    }

    error_SetTraceLevel ( TRACEWRN );//allow errors msgs only to be logged

    proclist = ftd_proc_create_list();

    // start statd
    if ((throt = ftd_proc_create(PROCTYPE_THREAD)) == NULL) {
        error_tracef( TRACEERR, "MASTER THREAD:Calling ftd_proc_create(PROCTYPE_THREAD)" );
        goto errexit;
    }

    throt->procp->function = StatThread; // XXX JRL, outof band data
    throt->procp->command = (LPDWORD)&throt->args;

    if (ftd_proc_exec_throt(throt, 1, envp) < 0) 
    {
        ftd_proc_delete(throt);    
        error_tracef( TRACEERR, "MASTER THREAD:Calling ftd_proc_exec_throt()" );
        goto errexit;
    }
    
    ftd_proc_add_to_list(proclist, &throt);

    /*
     * create and initialize communication socket objects
     */
    if ((listener = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL) {
        error_tracef( TRACEERR, "MASTER THREAD:Calling ftd_sock_create()" );
        goto errexit;
    }
    gethostname(hostname, sizeof(hostname));

    error_tracef( TRACEINF, "MASTER : Id %d" , GetCurrentThreadId() );

    // szg
    // This fixes the multiple IP's on one machine bug
    //
    // We need to know which IP the collector will talk to
    //
    // To do this, we check the ip trough which he can reach us
    //
    // Make sure the collector ip is known!
    //
#ifdef TDMF_COLLECTOR
    ftd_mngt_initialize_collector_values();
#endif

    //
    // Test HostID validity (send error message if invalid)
    //
    if (!IsHostIdValid(GetHostId()))
    {
        unsigned int ulTempValue;
        char         TempMsg[255];

        ulTempValue    = GetHostId();
        if (ulTempValue)
        {
            sprintf(TempMsg,"[FATAL / MASTER THREAD]: No valid IP was found for your HOSTID 0x%08x - check if card is missing/disabled", ulTempValue);
        }
        else
        {
            //
            // send selected ip during install
            //
            char   tmp [80];
            if ( cfg_get_software_key_value("DtcIP", tmp, CFG_IS_STRINGVAL) == CFG_OK )
            {
                sprintf(TempMsg,"[FATAL / MASTER THREAD]: NO VALID HOSTID (IP=%s) - check your TCP/IP card selection",tmp);
            }
            else
            {
                sprintf(TempMsg,"[FATAL / MASTER THREAD]: NO VALID HOSTID (IP=unknown) - check your TCP/IP card selection");
            }
        }

        ftd_mngt_msgs_send_status_msg_to_collector(TempMsg,time(0));
        error_tracef( TRACEERR, "MASTER THREAD: HOSTID is invalid" );
        goto errexit;
    }

    //
    // want to listen on any NIC so leave hostname args NULL    
    if (ftd_sock_init(listener, NULL, NULL, 0,0,
        SOCK_STREAM, AF_INET, 1, 0) < 0)
    {
        error_tracef( TRACEERR, "MASTER THREAD:Calling ftd_sock_init()" );
        goto errexit;
    }

    // get it from /etc/services if there
    if (!(portnum = ftd_sock_get_port(FTD_MASTER))) 
    {
        portnum = FTD_SERVER_PORT;
    }

    if (ftd_sock_listen(listener, portnum) < 0) 
    {
        error_tracef( TRACEERR, "MASTER THREAD:Calling ftd_sock_listen()" );
        goto errexit;
    }

    n = 1;
    if (ftd_sock_set_opt(listener, SOL_SOCKET, SO_REUSEADDR,
        (char*)&n, sizeof(int)) < 0)
    {
        error_tracef( TRACEERR, "MASTER THREAD:Calling ftd_sock_set_opt():SO_REUSEADDR" );
        goto errexit;
    }
    
    /* set the TCP window from the value in the ftd.conf file */
    if ((cfg_get_software_key_value("tcp_window_size", tcpwinsize, CFG_IS_NOT_STRINGVAL)) == CFG_OK)
    {
        n = atoi(tcpwinsize);
		if ((n > FTD_MAX_TCP_WINDOW_SIZE) || (n < 0)) // pb, try to chenge tcpwinsize bigger than 256k, even if manuals are saying 64kb limitation...
            n = 256 * 1024;
    } 
    else 
    {
        n = 256*1024;
    }

    if (ftd_sock_set_opt(listener, SOL_SOCKET, SO_SNDBUF,(char*)&n, sizeof(int)) < 0)
    {
        error_tracef( TRACEERR, "MASTER THREAD:Calling ftd_sock_set_opt():SO_SNDBUF" );
        goto errexit;
    }
    if (ftd_sock_set_opt(listener, SOL_SOCKET, SO_RCVBUF,
        (char*)&n, sizeof(int)) < 0)
    {
        error_tracef( TRACEERR, "MASTER THREAD:Calling ftd_sock_set_opt():SO_RCVBUF" );
        goto errexit;
    }

	// verify
    n = sizeof (int);                                                                               // pb ++
    tcpwin=0;                                                                           
    rc=ftd_sock_get_opt(listener, SOL_SOCKET, SO_SNDBUF,(char*)&tcpwin, &n);  
    error_tracef( TRACEWRN, "MASTER is booting :TCP_WINDOWS_SIZE (SO_SNDBUF) = %d,rc=%d",tcpwin,rc );       
    n = sizeof (int);
    tcpwin=0;                                                                           
    rc=ftd_sock_get_opt(listener, SOL_SOCKET, SO_RCVBUF,(char*)&tcpwin, &n);  
    error_tracef( TRACEWRN, "MASTER is booting :TCP_WINDOWS_SIZE (SO_RCVBUF) = %d,rc=%d",tcpwin,rc );     // pb --    


    /* process io events */
    (void)dispatch_io(proclist, listener, -1, envp);

errexit:
    
    ftd_proc_kill_all(proclist);

    ftd_dev_lock_delete();

    ftd_proc_delete_list(proclist);

    ftd_delete_errfac();

#ifdef TDMF_COLLECTOR
    //initiate a system shutdown and restart, if required.
    if ( ftd_mngt_sys_need_to_restart_system() )
        ftd_mngt_sys_restart_system();
#endif

    ftd_sock_delete(&listener);

    return(0);
}
