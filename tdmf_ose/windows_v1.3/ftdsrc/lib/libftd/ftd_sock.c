/* 
 * ftd_sock.c - FTD logical group message protocol interface
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

#define TDMF_TRACE_MAKER

#include "ftd_port.h"
#include "ftd_sock.h"
#include "ftd_rsync.h"
#include "ftd_error.h"
#include "ftd_lg.h"
#include "ftd_lg_states.h"
#include "ftd_ioctl.h"
#include "ftd_ps.h"
#include "ftd_proc.h"
#include "ftd_fsm.h"
#include "ftd_stat.h"
#include "disksize.h"
#include "comp.h"
#include "misc.h"
#if defined(_WINDOWS)
#include "ftd_cfgsys.h"
#include "ftd_devlock.h"
#endif

#undef TDMF_TRACE_MAKER

struct tdmf_trace_text  tdmf_socket_msg[]= 
{
{FTDCHANDSHAKE,                "FTDCHANDSHAKE \0"               },
{FTDCCHKCONFIG,                "FTDCCHKCONFIG \0"               },
{FTDCNOOP,                     "FTDCNOOP \0"                    },
{FTDCVERSION,                  "FTDCVERSION \0"                 },
{FTDCCHUNK,                    "FTDCCHUNK \0"                   },
{FTDCHUP,                      "FTDCHUP \0"                     },
{FTDCCPONERR,                  "FTDCCPONERR \0"                 },
{FTDCCPOFFERR,                 "FTDCCPOFFERR \0"                },
{FTDCEXIT,                     "FTDCEXIT \0"                    },
{FTDCBFBLK,                    "FTDCBFBLK \0"                   },
{FTDCBFSTART,                  "FTDCBFSTART \0"                 },
{FTDCBFREAD,                   "FTDCBFREAD \0"                  },
{FTDCBFEND,                    "FTDCBFEND \0"                   },
{FTDCCHKSUM,                   "FTDCCHKSUM \0"                  },
{FTDCRSYNCDEVS,                "FTDCRSYNCDEVS \0"               },
{FTDCRSYNCDEVE,                "FTDCRSYNCDEVE \0"               },
{FTDCRFBLK,                    "FTDCRFBLK \0"                   },
{FTDCRFEND,                    "FTDCRFEND \0"                   },
{FTDCRFFEND,                   "FTDCRFFEND \0"                  },
{FTDCKILL,                     "FTDCKILL \0"                    },
{FTDCRFSTART,                  "FTDCRFSTART \0"                 },
{FTDCRFFSTART,                 "FTDCRFFSTART \0"                },
{FTDCCPSTARTP,                 "FTDCCPSTARTP \0"                },
{FTDCCPSTARTS,                 "FTDCCPSTARTS \0"                },
{FTDCCPSTOPP,                  "FTDCCPSTOPP \0"                 },
{FTDCCPSTOPS,                  "FTDCCPSTOPS \0"                 },
{FTDCCPON,                     "FTDCCPON \0"                    },
{FTDCCPOFF,                    "FTDCCPOFF \0"                   },
{FTDCSTARTAPPLY,               "FTDCSTARTAPPLY \0"              },
{FTDCSTOPAPPLY,                "FTDCSTOPAPPLY \0"               },
{FTDCSTARTPMD,                 "FTDCSTARTPMD \0"                },
{FTDCAPPLYDONECPON,            "FTDCAPPLYDONECPON \0"           },
{FTDCREFOFLOW,                 "FTDCREFOFLOW \0"                },
{FTDCSIGNALPMD,                "FTDCSIGNALPMD \0"               },
{FTDCSIGNALRMD,                "FTDCSIGNALRMD \0"               },
{FTDCSTARTRECO,                "FTDCSTARTRECO \0"               },
{FTDACKERR ,                   "FTDACKERR \0"                   },
{FTDACKRSYNC,                  "FTDACKRSYNC \0"                 },
{FTDACKCHKSUM,                 "FTDACKCHKSUM \0"                },
{FTDACKCHUNK,                  "FTDACKCHUNK \0"                 },
{FTDACKHUP,                    "FTDACKHUP \0"                   },
{FTDACKCPSTART,                "FTDACKCPSTART \0"               },
{FTDACKCPSTOP,                 "FTDACKCPSTOP \0"                },
{FTDACKCPON,                   "FTDACKCPON \0"                  },
{FTDACKCPOFF,                  "FTDACKCPOFF \0"                 },
{FTDACKCPOFFERR,               "FTDACKCPOFFERR \0"              },
{FTDACKCPONERR,                "FTDACKCPONERR  \0"              },
{FTDACKNOOP,                   "FTDACKNOOP  \0"                 },
{FTDACKCONFIG,                 "FTDACKCONFIG  \0"               },
{FTDACKKILL ,                  "FTDACKKILL   \0"                },
{FTDACKHANDSHAKE,              "FTDACKHANDSHAKE  \0"            },
{FTDACKVERSION,                "FTDACKVERSION  \0"              },
{FTDACKCLI,                    "FTDACKCLI  \0"                  },
{FTDACKRFSTART,                "FTDACKRFSTART  \0"              },
{FTDACKNOPMD,                  "FTDACKNOPMD  \0"                },
{FTDACKNORMD,                  "FTDACKNORMD  \0"                },
{FTDACKDOCPON,                 "FTDACKDOCPON  \0"               },
{FTDACKDOCPOFF,                "FTDACKDOCPOFF  \0"              },
{FTDACKCLD,                    "FTDACKCLD  \0"                  },
{FTDACKCPERR,                  "FTDACKCPERR  \0"                },
{FTDCSIGUSR1,                  "FTDCSIGUSR1  \0"                },
{FTDCSIGTERM,                  "FTDCSIGTERM  \0"                },
{FTDCSIGPIPE,                  "FTDCSIGPIPE  \0"                },
{FTDCMANAGEMENT,               "FTDCMANAGEMENT  \0"             },
{FTDCSETTRACELEVEL,            "FTDCSETTRACELEVEL  \0"          },
{FTDCSETTRACELEVELACK,         "FTDCSETTRACELEVELACK  \0"       },
{FTDCREMWAKEUP,                "FTDCREMWAKEUP \0"               },
{FTDCREMDRIVEERR,              "FTDCREMDRIVEERR \0"             },
{FTDCREFRSRELAUNCH,            "FTDCREFRSRELAUNCH \0"           }
};                                                                

extern int bDbgLogON;


#define diStrMax 500

static void
ftd_sock_encode_auth(time_t ts, char* hostname, u_long hostid, u_long ip,
            int* encodelen, char* encode);
static void
ftd_sock_decode_auth(int encodelen, char* encodestr, time_t ts,
    u_long hostid, char* hostname, size_t hostname_len, u_long ip);

#if defined(_WINDOWS)
int
ftd_sock_startup(void)
{
    return sock_startup();
}

int
ftd_sock_cleanup(void)
{
    return sock_cleanup();
}

#endif

/*
 * ftd_sock_create_list -- create a linked list of ftd_sock_t objects 
 */
LList *
ftd_sock_create_list(void)
{

    LList *socklist = CreateLList(sizeof(ftd_sock_t**));
    
    return socklist;
}

/*
 * ftd_sock_delete_list -- delete the list of ftd_sock_t objects 
 */
int
ftd_sock_delete_list(LList *socklist)
{
    ftd_sock_t  **sockpp;

    ForEachLLElement(socklist, sockpp) {
        ftd_sock_delete(sockpp);
    }   
    FreeLList(socklist);
    
    return 0;
}

/*
 * ftd_sock_add_to_list --
 * add ftd_sock_t object to linked list of ftd_sock_t objects 
 */
int
ftd_sock_add_to_list(LList *socklist, ftd_sock_t **fsockpp)
{

    AddToTailLL(socklist, fsockpp);

    return 0;
}

/*
 * ftd_sock_remove_from_list --
 * remove ftd_sock_t object from linked list of ftd_sock_t objects 
 */
int
ftd_sock_remove_from_list(LList *socklist, ftd_sock_t **fsockp)
{

    RemCurrOfLL(socklist, fsockp);

    return 0;
}

/*
 * ftd_sock_create --
 * create a ftd_sock_t object
 */
ftd_sock_t *
ftd_sock_create(int type)
{
    ftd_sock_t  *fsockp;

    if ((fsockp = (ftd_sock_t*)calloc(1, sizeof(ftd_sock_t))) == NULL) {
        goto errret;
    }

    if ((fsockp->sockp = sock_create()) == NULL) {
        goto errret;
    }

    fsockp->type = type;

    error_tracef( TRACEINF, "ftd_sock_create():<%x>,tid=%x", fsockp->sockp,GetCurrentThreadId() );
    return fsockp;

errret:

    if (fsockp) {
        free(fsockp);
    }
    reporterr (ERRFAC, M_SOCKCREAT, ERRCRIT, ftd_strerror());

    return NULL;
}

/*
 * ftd_sock_delete --
 * delete the ftd_sock_t object
 */
int
ftd_sock_delete(ftd_sock_t **fsockpp)
{
    ftd_sock_t  *fsockp;

    if (fsockpp == NULL || *fsockpp == NULL) {
        return 0;
    }

    fsockp = *fsockpp;

    error_tracef( TRACEINF, "ftd_sock_delete():<%x>,tid=%x", fsockp->sockp,GetCurrentThreadId() );

    if (FTD_SOCK_VALID(fsockp)) {
        // inititialized ftd_sock_t object
        sock_delete(&fsockp->sockp);
    } else {
        if (fsockp->sockp) {
            free(fsockp->sockp);
        }   
    }

    free(fsockp);

    *fsockpp = NULL;
    
    return 0;
}

/*
 * ftd_sock_init --
 */
int
ftd_sock_init(ftd_sock_t *fsockp, char *lhostname, char *rhostname,
    unsigned long lip, unsigned long rip, int type, int family,
    int create, int verifylocal)
{
    int rc;

    if ((rc = sock_init(fsockp->sockp, lhostname, rhostname,
        lip, rip, type, family, create, verifylocal)) < 0)
    {
        error_tracef( TRACEINF, "***ERR ftd_sock_init(),rc=%d:<%x><%s><%s>,tid=%x", 
            rc, fsockp->sockp, lhostname, rhostname, GetCurrentThreadId() );
        if (rc == -2) {
            reporterr (ERRFAC, M_SOCKNOTME, ERRCRIT, FTD_SOCK_LHOST(fsockp));
        }   
        reporterr (ERRFAC, M_SOCKINIT, ERRCRIT, sock_strerror(sock_errno()));
        return -1;
    }

    error_tracef( TRACEINF, "ftd_sock_init():<%x><%s><%s>,tid=%x", 
        fsockp->sockp, lhostname, rhostname, GetCurrentThreadId() );

    fsockp->magicvalue = FTDSOCKMAGIC;
    fsockp->contries = 0;

    return 0;
}

/*
 * ftd_sock_set_connect --
 * set connected flag for socket
 */
void
ftd_sock_set_connect(ftd_sock_t *fsockp)
{

    SET_SOCK_CONNECT(fsockp->sockp->flags);

    return;
}

/*
 * ftd_sock_set_disconnect --
 * un-set connected flag for socket
 */
void
ftd_sock_set_disconnect(ftd_sock_t *fsockp)
{

    UNSET_SOCK_CONNECT(fsockp->sockp->flags);

    return;
}

/*
 * ftd_sock_connect_forever --
 * keep trying to connect to remote host - check signals
 */
int
ftd_sock_connect_forever(ftd_lg_t *lgp, int port)
{
    int             rc = 0, sockerr, report_tries = 100, tries = -1; 
#if !defined(_WINDOWS)
    fd_set          rset;
#endif
    short           inchar;

    sock_t          sockp;
	int		nErrorLogged = 0;

    // close the current connection
    ftd_sock_disconnect(lgp->dsockp);

    while (1) {
        
        memcpy(&sockp, lgp->dsockp->sockp, sizeof(sock_t));

#if defined(_WINDOWS)
		//WR33688 Veera, Just setting the Handles to NULL
        if (sockp.hEvent) {
            CloseHandle(sockp.hEvent);
			sockp.hEvent = NULL;
			lgp->dsockp->sockp->hEvent = NULL;
        }
#endif
        if ((rc = sock_socket(AF_INET)) < 0) {
            error_tracef( TRACEERR, "***ERR ftd_sock_connect_forever(), sock_socket() failed rc=%d:<%x>,tid=%x", 
                rc, lgp->dsockp->sockp, GetCurrentThreadId() );
            return rc;
        }

        sockp.sock = rc;

#if defined(_WINDOWS)
        sockp.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
        

		//WR33688 Veera, Retrying the Sock_bind after a failure
		//Also checking for events in the ftd_lg_dispatch_io().
RetrySocket:
        
        if ((rc = sock_bind(&sockp, port)) < 0) {

			if(nErrorLogged==0)
			{
            error_tracef( TRACEERR, "***ERR ftd_sock_connect_forever(), sock_bind() failed rc=%d:<%x>,tid=%x", 
                rc, lgp->dsockp->sockp, GetCurrentThreadId() );
				

			}
			if(++nErrorLogged ==1000)
			{
				nErrorLogged =0;
			}
			//Changed by Veera, to account for relaunch of the PMD 04-20-2004

			Sleep(1000);
	        memcpy(lgp->dsockp->sockp, &sockp, sizeof(sock_t));

		    inchar = 0;

			// this call will only handle local events
			// since we are not yet connected to the remote peer
    
			inchar = ftd_lg_dispatch_io(lgp);

			if (inchar) {
				if (inchar == FTD_CINVALID) {
					error_tracef( TRACEERR, "***ERR ftd_sock_connect_forever(), inchar == FTD_CINVALID :<%x>,tid=%x", 
						lgp->dsockp->sockp, GetCurrentThreadId() );
					return -1;
				}
				ftd_lg_report_invalid_op(lgp, inchar);
			}   

			goto RetrySocket;

//			return FTD_LG_NET_BROKEN;
//            return rc;
        }

        // TODO: connect timeout needs to be adjusted for
        // slower links 
        if ((rc = sock_connect_nonb(&sockp, port,
            FTD_SOCK_CONNECT_SHORT_TIMEO, 0, &sockerr)) == 0)
        {
            // got it
#if defined(_WINDOWS)
            if (GET_LG_BAB_READY(lgp->flags)) {
                // The bab was in a 'signaled' state but we weren't able
                // to service it because we had no connection. This poses
                // a problem because the only way the event is reset is
                // to call the 'get_oldest_entries' ioctl and we didn't do
                // it due to the lack of a connection (if we had
                // we would have gotten into a tight spin state and used 
                // too much CPU for no good reason). So,we set the bab event
                // back to 'signaled' here since we are now connected and
                // can service the event.
                SetEvent(lgp->babfd);
                UNSET_LG_BAB_READY(lgp->flags);
            }   
#endif          
            break;
        }

        // sock has been closed in sock_connect_nonb

        if (tries == -1 || tries == report_tries) {
            reporterr(ERRFAC, M_SOCKRECONN, ERRWARN,
                sockp.lhostname,
                sockp.rhostname,
                port,
                sock_strerror(sockerr));
            tries = 0;
        }
        tries++;

#if defined(_WINDOWS)
        // Event handle has been closed in sock_connect_nonb
        sockp.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
        memcpy(lgp->dsockp->sockp, &sockp, sizeof(sock_t));

        inchar = 0;

        // this call will only handle local events
        // since we are not yet connected to the remote peer
    
        inchar = ftd_lg_dispatch_io(lgp);

        if (inchar) {
            if (inchar == FTD_CINVALID) {
                error_tracef( TRACEERR, "***ERR ftd_sock_connect_forever(), inchar == FTD_CINVALID :<%x>,tid=%x", 
                    lgp->dsockp->sockp, GetCurrentThreadId() );
                return -1;
            }
            ftd_lg_report_invalid_op(lgp, inchar);
        }   
    }

    memcpy(lgp->dsockp->sockp, &sockp, sizeof(sock_t));

    ftd_sock_set_connect(lgp->dsockp);

    return rc;
}

/*
 * ftd_sock_connect --
 */
int
ftd_sock_connect(ftd_sock_t *fsockp, int port)
{
    int rc = sock_bind(fsockp->sockp, port);

    if (rc < 0) {
        reporterr (ERRFAC, M_SOCKCONNECT, ERRWARN, 
            fsockp->sockp->lhostname, 
            fsockp->sockp->rhostname, 
            fsockp->sockp->port, sock_strerror(sock_errno()));
        return -1;
    }

    if (sock_connect(fsockp->sockp, port) < 0) {
        reporterr (ERRFAC, M_SOCKCONNECT, ERRWARN, 
            fsockp->sockp->lhostname, 
            fsockp->sockp->rhostname, 
            fsockp->sockp->port, sock_strerror(sock_errno()));
        return -1;
    }

    ftd_sock_set_connect(fsockp);

    return 0;
}

/*
 * ftd_sock_connect_nonb --
 */
int
ftd_sock_connect_nonb(ftd_sock_t *fsockp, int port, int sec, int usec, int silent)
{
    int sockerr, rc;

    if ((rc = sock_connect_nonb(fsockp->sockp, port, sec, usec, &sockerr)) < 0) 
	{
        if (!silent) 
		{
			error_tracef( TRACEERR, "ftd_sock_connect_nonb() failed rc=%d:<%x><%d>,tid=%x", 
            rc, fsockp->sockp, port, GetCurrentThreadId() );

            reporterr (ERRFAC, M_SOCKCONNECT, ERRWARN, 
                fsockp->sockp->lhostname, 
                fsockp->sockp->rhostname, 
                port,
                sock_strerror(sockerr));
        }
        return 0;
    }

    ftd_sock_set_connect(fsockp);

    error_tracef( TRACEINF, "ftd_sock_connect_nonb():<%x><%d>,tid=%x", 
        fsockp->sockp, port, GetCurrentThreadId() );
    return 1;
}
 
/*
 * ftd_sock_disconnect --
 */
int
ftd_sock_disconnect(ftd_sock_t *fsockp)
{
    int sock;


    if ((sock = FTD_SOCK_FD(fsockp)) == (SOCKET)-1) {
        error_tracef( TRACEINF, "ftd_sock_disconnect():<%x>,tid=%x", sock,GetCurrentThreadId() );
        return 0;
    }

    error_tracef( TRACEINF, "ftd_sock_disconnect():<%x>,tid=%x", fsockp->sockp,GetCurrentThreadId() );

    sock_disconnect(fsockp->sockp);

    ftd_sock_set_disconnect(fsockp);

    sock = -1;

    return 0;
}

/*
 * ftd_sock_bind --
 */
int
ftd_sock_bind(ftd_sock_t *fsockp, int port)
{
    int rc;

    if ((rc = sock_bind(fsockp->sockp, port)) < 0) {
        error_tracef( TRACEERR, "***ERR ftd_sock_bind(), rc=%d:<%x><%d>,tid=%x", 
            rc, fsockp->sockp, port, GetCurrentThreadId() );
        return rc;
    }

    error_tracef( TRACEINF, "ftd_sock_bind():<%x><%d>,tid=%x", 
        fsockp->sockp, port, GetCurrentThreadId() );
    return 0;
}

/*
 * ftd_sock_listen --
 */
int
ftd_sock_listen(ftd_sock_t *fsockp, int port)
{

    if (sock_listen(fsockp->sockp, port) < 0) {
        reporterr (ERRFAC, M_SOCKLISTEN, ERRCRIT, fsockp->sockp->sock,
            sock_strerror(sock_errno()));
        return -1;
    }

    return 0;
}

/*
 * ftd_sock_accept --
 */
int
ftd_sock_accept(ftd_sock_t *listener, ftd_sock_t *fsockp)
{

    if (sock_accept(listener->sockp, fsockp->sockp) < 0) {
        reporterr (ERRFAC, M_SOCKACCEPT, ERRCRIT, fsockp->sockp->sock,
            sock_strerror(sock_errno()));
        return -1;
    }

    fsockp->magicvalue = FTDSOCKMAGIC;

    return 0;
}

/*
 * ftd_sock_accept_nonb --
 */
int
ftd_sock_accept_nonb(ftd_sock_t *listener, ftd_sock_t *fsockp,
    int sec, int usec)
{

    if (sock_accept_nonb(listener->sockp, fsockp->sockp, sec, usec) < 0) {
        reporterr (ERRFAC, M_SOCKACCEPT, ERRCRIT, fsockp->sockp->sock,
            sock_strerror(sock_errno()));
        return -1;
    }

    fsockp->magicvalue = FTDSOCKMAGIC;

    return 0;
}
 
/*
 * ftd_sock_set_opt --
 */
int
ftd_sock_set_opt(ftd_sock_t *fsockp, int level, int optname, char *optval, int optlen)
{

    if (sock_set_opt(fsockp->sockp, level, optname, optval, optlen) < 0) {
        reporterr (ERRFAC, M_SOCKOP, ERRCRIT, sock_strerror(sock_errno()));
        return -1;
    }

    return 0;
}

/*
 * ftd_sock_get_opt --
 */
int
ftd_sock_get_opt(ftd_sock_t *fsockp, int level, int optname, char *optval, int *optlen)
{

    if (sock_get_opt(fsockp->sockp, level, optname, optval, optlen) < 0) {
        reporterr (ERRFAC, M_SOCKOP, ERRCRIT, sock_strerror(sock_errno()));
        return -1;
    }

    return 0;
}

/*
 * ftd_sock_send --
 */
int
ftd_sock_send ( char tracing,ftd_sock_t *fsockp, char *buf, int len)
{
    sock_t          *sockp;
    time_t          now, starttime;
    int             wlen, wrc, errnum;
    int             deltatime = 0;
    char            *wbuf;

    // to shorten the TCP timeout in the write, we test the network 
    // channel if we don't send anything for a while. If the network
    // link has been lost we report it and return.

    

    time(&starttime);

    sockp = fsockp->sockp;
    sockp->writecnt = 0;

    while (TRUE) 
        {
        if (sockp->writecnt == len) 
            break;

    
        if (ftd_sock_check_send(fsockp, 1000) == 0) 
            {
            // net not writable
            UNSET_SOCK_WRITABLE(sockp->flags);
            
            // if we can't send for 1 minute - give up - something
            // is terribly wrong with the peer
            time(&now);
            if ((deltatime = (now-starttime)) > 60000) 
                {
                // report error and return
                //reporterr(ERRFAC, M_SOCKSEND, ERRWARN,sockp->lhostname, sockp->rhostname,sockp->port, "Send timed out");
                error_tracef( TRACEERR, "ftd_sock_send() Send timed out!:<%x><%s><%s><%d>,tid=%x", 
                    sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );

                return -1;
                }
            } 
        else 
            {
            // adjust buf offset if some already sent
            if (sockp->writecnt) 
                { 
                wbuf = buf + sockp->writecnt;
                wlen = len - sockp->writecnt;
                } 
            else 
                {
                wbuf = buf;
                wlen = len;
                }


            wrc = sock_send(sockp, wbuf, wlen);
            
            if (wrc < 0) 
                {
                errnum = sock_errno();

                if (errnum == ECONNRESET
                    || errnum == EPIPE
                    || errnum == ESHUTDOWN
                    || errnum == ECONNABORTED)
                    {
                    if (fsockp->type == FTD_SOCK_INET) 
                        {
                        // we only want to do this if we are
                        // an 'internet' type connection
                        if (ftd_sock_test_link(sockp->lhostname,sockp->rhostname,sockp->lip,sockp->rip, 
                            sockp->port,    // TODO: this needs to be adjusted for link speed
                            FTD_SOCK_CONNECT_SHORT_TIMEO) != 1)
                            {
                            error_tracef(TRACEERR, "ftd_sock_send - TIME0- ERROR: %d %s , tid=%x",errnum,sock_strerror(errnum),GetCurrentThreadId());
                            return FTD_LG_NET_TIMEO;
                            }
                        else 
                            {
                            error_tracef(TRACEERR, "ftd_sock_send - NET BROKEN - ERROR: %d %s , tid=%x",errnum,sock_strerror(errnum),GetCurrentThreadId());
                            return FTD_LG_NET_BROKEN;
                            }
                        
                        } 
                    else 
                        {
                        error_tracef(TRACEERR, "ftd_sock_send - NET BROKEN2 - ERROR: %d %s , tid=%x",errnum,sock_strerror(errnum),GetCurrentThreadId());
                        return FTD_LG_NET_BROKEN;
                        }                   
                    } 
                else if (errnum == EWOULDBLOCK) 
                    {
                    // do nothing, loop
                    }
#if defined(_WINDOWS)
                else if (errnum == ETIMEDOUT) 
                    {
                    error_tracef(TRACEERR, "ftd_sock_send - TIME1- ERROR: %d %s , tid=%x",errnum,sock_strerror(errnum),GetCurrentThreadId()); 
                    return FTD_LG_NET_TIMEO;
                    }
#endif
                else 
                    {
                    error_tracef(TRACEERR, "ftd_sock_send() M_SOCKSEND:<%x><%s><%s><%d><%d : %s>,tid=%x", 
                        sockp, sockp->lhostname, sockp->rhostname, sockp->port, errnum, sock_strerror(errnum), GetCurrentThreadId() );

                    return wrc;
                    }
                } 
            else if (wrc == 0) 
                {
                // connection lost
                UNSET_SOCK_WRITABLE(sockp->flags);
                errnum = sock_errno();
                error_tracef(TRACEINF5, "ftd_sock_send - NET BROKEN3 - ERROR: %d %s , tid=%x",errnum,sock_strerror(errnum),GetCurrentThreadId()); 
                return FTD_LG_NET_BROKEN;
                } 
            else if (wrc < wlen) 
                { 
                // partial write complete 
                sockp->writecnt += wrc;
                } 
            else 
                {
                // write completed
                sockp->writecnt = len;
                break;
                }
            } // else
        }   // while(TRUE)

    return len;
}

/*
 * ftd_sock_send_lg --
 */
int
ftd_sock_send_lg(ftd_sock_t *fsockp, ftd_lg_t *lgp, char *buf, int len)
{
    sock_t          *sockp;
    time_t          now, starttime;
    int             deltatime, wlen, wrc, rc, errnum;
    int             timeo_recv, timeo_send;
    char            *wbuf;

    // to shorten the TCP timeout in the write, we test the network 
    // channel if we don't send anything for a while. If the network
    // link has been lost we report it and return.
    // since both peers are writing, to prevent a deadlock in the write
    // we test the connection for writability (ie. write won't block) 
    // before calling write. If is not writable then we read until it
    // is writable (ie. TCP recources available).

    time(&starttime);
    sockp = fsockp->sockp;
    sockp->writecnt = 0;

    if (lgp->cfgp->role == ROLEPRIMARY) {
        timeo_recv = 100;
        timeo_send = 1; // non-zero because 0 blocks
    } else {
        timeo_send = 1000;
    }

#ifdef TDMF_TRACE4
    error_tracef(TRACEINF, "%s - Tx msg: %s , tid=%x",LG_PROCNAME(lgp),
             tdmf_socket_msg[((ftd_header_t *) buf)->msgtype-1].cmd_txt,GetCurrentThreadId());
#endif

    while (TRUE) {
        if (sockp->writecnt == len) {
            break;
        }
        
        if (ftd_sock_check_send(fsockp, timeo_send) == 0) {
            // net not writable
            UNSET_SOCK_WRITABLE(sockp->flags);
            
            // if we can't send for 1 minute - give up - something
            // is terribly wrong with the peer

            time(&now);
            if ((deltatime = (now-starttime)) > 60000) {
                // report error and return 
                /*reporterr(ERRFAC, M_SOCKSEND, ERRWARN,
                    sockp->lhostname, sockp->rhostname,
                    sockp->port, "Send timed out");*/
                error_tracef( TRACEWRN, "ftd_sock_send_lg() Send timed out!:<%x><%s><%s><%d>,tid=%x", 
                    sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );

                return -1;
            }
        } else {
            // adjust buf offset if some already sent
            if (sockp->writecnt) { 
                wbuf = buf + sockp->writecnt;
                wlen = len - sockp->writecnt;
            } else {
                wbuf = buf;
                wlen = len;
            }

            wrc = sock_send(sockp, wbuf, wlen);

            if (wrc < 0) {
                
                errnum = sock_errno();
                
                if (errnum == ECONNRESET
                    || errnum == EPIPE
                    || errnum == ESHUTDOWN
                    || errnum == ECONNABORTED)
                {
                    if (fsockp->type == FTD_SOCK_INET) {
                        // we only want to do this if we are
                        // an 'internet' type connection
                        if (ftd_sock_test_link(
                            sockp->lhostname,
                            sockp->rhostname,
                            sockp->lip,
                            sockp->rip,
                            sockp->port,
                            // TODO: this needs to be adjusted for link speed
                            FTD_SOCK_CONNECT_SHORT_TIMEO) != 1)
                        {
                            error_tracef( TRACEERR, "ftd_sock_send_lg() FTD_LG_NET_TIMEO!:<%x><%s><%s><%d>,tid=%x", 
                                sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );
                            return FTD_LG_NET_TIMEO;
                        } else {
                            error_tracef( TRACEERR, "ftd_sock_send_lg() FTD_LG_NET_BROKEN1 !:<%x><%s><%s><%d>,tid=%x", 
                                sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );
                            return FTD_LG_NET_BROKEN;
                        }
                    } else {
                        error_tracef( TRACEERR, "ftd_sock_send_lg() FTD_LG_NET_BROKEN2 !:<%x><%s><%s><%d>,tid=%x", 
                            sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );
                        return FTD_LG_NET_BROKEN;
                    }
                } else if (errnum == EWOULDBLOCK
                    || errnum == ENOBUFS)
                {
                    // do nothing, loop
                }
#if defined(_WINDOWS)
                else if (errnum == ETIMEDOUT) {
                    error_tracef(TRACEERR, "ftd_sock_send_lg() FTD_LG_NET_TIMEO2 !:<%x><%s><%s><%d>,tid=%x", 
                        sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );
                    return FTD_LG_NET_TIMEO;
                }
#endif
                else {
                    error_tracef(TRACEERR, "ftd_sock_send_lg() M_SOCKSEND !:<%x><%s><%s><%d><%d : %s> , tid=%x", 
                        sockp, sockp->lhostname, sockp->rhostname, sockp->port, errnum, sock_strerror(errnum), GetCurrentThreadId() );
                    return wrc;
                }
            } else if (wrc == 0) {
                // connection lost
                UNSET_SOCK_WRITABLE(sockp->flags);
                error_tracef(TRACEINF5, "ftd_sock_send_lg() FTD_LG_NET_BROKEN3 !:<%x><%s><%s><%d> , tid=%x", 
                    sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );
                return FTD_LG_NET_BROKEN;
            } else if (wrc < wlen) { 
                UNSET_SOCK_WRITABLE(sockp->flags);
                sockp->writecnt += wrc;
            } else {
                // write completed
                sockp->writecnt = len;
                SET_SOCK_WRITABLE(sockp->flags);
                break;
            }
        }

        if (lgp->cfgp->role == ROLEPRIMARY) {
            // primary - read to free up remote in case it's blocked 
            if ((rc = ftd_sock_recv_lg_msg(lgp->dsockp, lgp, timeo_recv)) != 0) {
                if (rc == FTD_LG_NET_NOT_READABLE) {
                    // no more to read
                    //break;
                    //error_tracef( TRACEERR, "***ERR ftd_sock_send_lg() , rc=FTD_LG_NET_NOT_READABLE %s :<%x> ,tid=%x", 
                    //    LG_PROCNAME(lgp), lgp->dsockp->sockp, GetCurrentThreadId() );
                } else {
                    error_tracef( TRACEERR, "*** ftd_sock_send_lg() , ftd_sock_recv_lg_msg() returns %d ! %s :<%x> , tid=%x", 
                        rc, LG_PROCNAME(lgp), lgp->dsockp->sockp, GetCurrentThreadId() );
                    return rc;
                }
            }
            ftd_lg_housekeeping(lgp, 0);
        }
    }   // while(TRUE)

    return len;
}

/*
 * ftd_sock_send_lg_vector --
 */
int
ftd_sock_send_lg_vector(ftd_sock_t *fsockp, ftd_lg_t *lgp, 
    struct iovec iov[], int iovcnt)
{
    sock_t          *sockp;
    struct iovec    *liov;
    time_t          now, starttime;
    char            *iov_base, *save_iov_base;
    int             iov_offset, save_iov_len, deltatime, errnum;
    int             cnt, rc = 0, wrc, wlen, len, len1, i, j;
    int             timeo_recv, timeo_send;
    char            *buf;

    // to shorten the TCP timeout in the write, we test the network 
    // channel if we don't send anything for a while. If the network
    // link has been lost we report it and return.
    // since both peers are writing, to prevent a deadlock in the write
    // we test the connection for writability (ie. write won't block) 
    // before calling write. If is not writable then we read until it
    // is writable (ie. TCP recources available).

    // get write length 
    len = 0;
    for (i = 0; i < iovcnt; i++) {
        len += iov[i].iov_len;
    } 
    time(&starttime);

    sockp = fsockp->sockp;
    sockp->writecnt = 0; 

    wlen = len;

    if (lgp->cfgp->role == ROLEPRIMARY) {
        timeo_recv = 100;
        timeo_send = 1; // non-zero because 0 blocks indefinately
    } else {
        timeo_send = 1000;
    }
#ifdef TDMF_TRACE4
    error_tracef(TRACEINF4, "%s - TxVector msg: %s ",LG_PROCNAME(lgp), tdmf_socket_msg[((ftd_header_t *) iov[0].iov_base)->msgtype-1].cmd_txt);
#endif

    while (TRUE) {
        buf = (char*)iov; 

        if (sockp->writecnt == len) {
            break;
        }
        
        if (ftd_sock_check_send(fsockp, timeo_send) == 0) {
            // net not writable
            UNSET_SOCK_WRITABLE(sockp->flags);
            
            // if we can't send for 1 minute - give up - something
            // is wrong with the channel
            // should recover from this by reconnecting ?

            time(&now);
            if ((deltatime = (now-starttime)) > 60000) {
                // report error and die 
                /*reporterr(ERRFAC, M_SOCKSEND, ERRWARN,
                    sockp->lhostname, sockp->rhostname,
                    sockp->port, "Send timed out");*/
                error_tracef( TRACEERR, "ftd_sock_send_lg_vector() , Send timed out!: rc=%d %s :<%x><%s><%s><%d>, tid=%x", 
                    rc, LG_PROCNAME(lgp), 
                    sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );

                return -1;
            }
        } else {
            // adjust vector if some already sent
            if (sockp->writecnt) { 
                // partial write has already completed
                // adjust vector and try to write the rest 
                len1 = 0;
                for (j = 0; j < iovcnt; j++) {
                    if ((len1 += iov[j].iov_len) > sockp->writecnt) {
                        break;
                    }
                } 
                iov_base = &(iov[j].iov_base[0]); 
                iov_offset = (sockp->writecnt -
                    (len1 - iov[j].iov_len));

                // save base, len
                save_iov_base = iov[j].iov_base;
                save_iov_len = iov[j].iov_len;

                // adjust base, len 
                iov[j].iov_base = iov_base + iov_offset; 
                iov[j].iov_len -= iov_offset; 

                liov = &iov[j];
                cnt = iovcnt - j;
            } else {
                // save base, len
                j = 0;
                liov = iov;
                cnt = iovcnt;
                save_iov_base = iov[j].iov_base;
                save_iov_len = iov[j].iov_len;
            }

#if defined(_WINDOWS)
            wrc = sock_send(sockp, iov[j].iov_base, iov[j].iov_len);
#else
            wrc = sock_send_vector(sockp, liov, cnt);
#endif

            if (wrc < 0) {
                errnum = sock_errno();

                if (errnum == ECONNRESET
                    || errnum == EPIPE
                    || errnum == ESHUTDOWN
                    || errnum == ECONNABORTED)
                {
                    if (fsockp->type == FTD_SOCK_INET) {
                        // we only want to do this if we are
                        // an 'internet' type connection
                        if (ftd_sock_test_link(
                            sockp->lhostname,
                            sockp->rhostname,
                            sockp->lip,
                            sockp->rip,
                            sockp->port,
                            // TODO: this needs to be adjusted for link speed
                            FTD_SOCK_CONNECT_SHORT_TIMEO) != 1)
                        {
                            error_tracef( TRACEERR, "ftd_sock_send_lg_vector() FTD_LG_NET_TIMEO!:<%x><%s><%s><%d>,tid=%x", 
                                sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );
                            return FTD_LG_NET_TIMEO;
                        } else {
                            error_tracef( TRACEERR, "ftd_sock_send_lg_vector() FTD_LG_NET_BROKEN!:<%x><%s><%s><%d>,tid=%x", 
                                sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );
                            return FTD_LG_NET_BROKEN;
                        }
                    } else {
                            error_tracef( TRACEERR, "ftd_sock_send_lg_vector() FTD_LG_NET_BROKEN2 !:<%x><%s><%s><%d>,tid=%x", 
                                sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );
                        return FTD_LG_NET_BROKEN;
                    }
                } else if (errnum == EWOULDBLOCK
                    || errnum == ENOBUFS)
                {
                    // do nothing, loop
                } else {
                    error_tracef( TRACEERR, "ftd_sock_send_lg_vector() M_SOCKSEND !:<%x><%s><%s><%d>,tid=%x", 
                        sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );

                    return wrc;
                }
            } else if (wrc == 0) {
                // connection lost
                UNSET_SOCK_WRITABLE(sockp->flags);
                    error_tracef( TRACEINF5, "ftd_sock_send_lg_vector() FTD_LG_NET_BROKEN3 !:<%x><%s><%s><%d>,tid=%x", 
                        sockp, sockp->lhostname, sockp->rhostname, sockp->port, GetCurrentThreadId() );
                return FTD_LG_NET_BROKEN;
            } else if (wrc < wlen) { 
                // partial write complete
                // bump writecnt
#if defined(_WINDOWS)
                if (wrc != iov[j].iov_len) {
#endif
                    UNSET_SOCK_WRITABLE(sockp->flags);
#if defined(_WINDOWS)
                }
#endif
                sockp->writecnt += wrc;
                wlen -= wrc;
            } else {
                // write completed
                sockp->writecnt = len;
                SET_SOCK_WRITABLE(sockp->flags);
                break;
            }
            // re-assign base, len
            iov[j].iov_base = save_iov_base; 
            iov[j].iov_len = save_iov_len; 

            if (sockp->writecnt == len) {
                break;
            }
        }

        if (lgp->cfgp->role == ROLEPRIMARY) {
            // primary - read net to free up remote in case it's blocked
#if defined(_WINDOWS)
            if (GET_SOCK_WRITABLE(FTD_SOCK_FLAGS(fsockp)))
                continue;
#endif
            /* rc = ftd_sock_recv_lg_msg(lgp->dsockp, lgp, timeo_recv);
            if (rc != 0) {
                if (rc == FTD_LG_NET_NOT_READABLE) {
                    // no more to read
                    //break;
                    //error_tracef( TRACEERR, "***ERR ftd_sock_send_lg_vector() , rc=FTD_LG_NET_NOT_READABLE %s :<%x> ,tid=%x", 
                    //    LG_PROCNAME(lgp), lgp->dsockp->sockp, GetCurrentThreadId() );
                } else {

                    //if ( rc == 8 )
                    //  DebugBreak();

                    error_tracef( TRACEERR, "***ERR ftd_sock_send_lg_vector() , ftd_sock_recv_lg_msg rc=%d %s :<%x> ,tid=%x", 
                        rc, LG_PROCNAME(lgp), lgp->dsockp->sockp, GetCurrentThreadId() );
                    return rc;
                }
            } */
            ftd_lg_housekeeping(lgp, 0);
        }

    }   // while(TRUE)

    return len;
}

/*
 * ftd_sock_recv --
 */
int
ftd_sock_recv(ftd_sock_t *fsockp, char *buf, int len)
{
    time_t          now, lastts = 0;
    int             savelen, errnum, rc, rrc, elapsed_time = 0, deltatime;
    sock_t          *sockp;
    char            *lbuf;

    // to shorten the TCP timeout in the read, we test the network 
    // channel if we don't recv anything for a while. If the network
    // link has been lost we report it and return.

    sockp = fsockp->sockp;
    savelen = len;
    elapsed_time = 0;

    lbuf = buf;

    while (len > 0) 
        {
        rrc = sock_recv(sockp, lbuf, len);

        if (rrc < 0) 
            {
            errnum = sock_errno();

            if (errnum == ECONNRESET
                || errnum == EPIPE
                || errnum == ESHUTDOWN
                || errnum == ECONNABORTED)
                {
                if (fsockp->type == FTD_SOCK_INET) 
                    {
                    // we only want to do this if we are
                    // an 'internet' type connection
                    if (ftd_sock_test_link(sockp->lhostname,sockp->rhostname,sockp->lip,sockp->rip,
                        sockp->port,
                        // TODO: this needs to be adjusted for link speed
                        FTD_SOCK_CONNECT_SHORT_TIMEO) != 1)
                        {
                        error_tracef(TRACEERR, "ftd_sock_recv():TIME0 : %d,%s , tid=%x", errnum,sock_strerror(errnum), GetCurrentThreadId());
                        return FTD_LG_NET_TIMEO;
                        }
                    else 
                        {
                        error_tracef(TRACEINF5, "ftd_sock_recv():NET BROKEN: %d %s, tid=%x", errnum,sock_strerror(errnum), GetCurrentThreadId());
                        return FTD_LG_NET_BROKEN;
                        }
                    } 
                else 
                    {
                    error_tracef(TRACEINF5, "ftd_sock_recv():NET BROKEN2 %d %s, tid=%x ", errnum,sock_strerror(errnum), GetCurrentThreadId());
                    return FTD_LG_NET_BROKEN;
                    }
                }

#if defined(_WINDOWS)
            else if (errnum == ETIMEDOUT) 
                {
                error_tracef(TRACEERR, "ftd_sock_recv():TIMEO2 %d %s, tid=%x", errnum,sock_strerror(errnum), GetCurrentThreadId());
                return FTD_LG_NET_TIMEO;
                }
#endif

            else if (errnum != EINTR && errnum != EAGAIN && errnum != EWOULDBLOCK)
                { 
                // real error. report and return
                error_tracef(TRACEERR, "ftd_sock_recv():M_SOCKRECV <%s><%s><%d> <%d : %s>, tid=%x", 
                    sockp->lhostname, sockp->rhostname, sockp->port, 
                    errnum, sock_strerror(errnum), GetCurrentThreadId());
                return rrc;
                }
            else 
                {
                rc = sock_check_recv(sockp, 1000);
                
                if (rc == 0) 
                    {
                    // nothing to read
                    // check network if FTD_NET_SEND_NOOP_TIME has passed 
                    time(&now);
                    if (lastts > 0) 
                        deltatime = now-lastts;
                    else 
                        deltatime = 0;
                    
                    elapsed_time += deltatime;  

                    if (elapsed_time > FTD_NET_SEND_NOOP_TIME)
                        {
                        if (fsockp->type == FTD_SOCK_INET) 
                            {
                            // we only want to do this if we are
                            // an 'internet' type connection
                            
                            // test peer connect/network link
                            if ((rc = ftd_sock_send_noop(fsockp, -1, 0)) < 0) 
                                {
                                error_tracef(TRACEERR, "ftd_sock_recv():NET NOOP %d, tid=%x", rc, GetCurrentThreadId());
                                return rc;
                                }
                            
                            
                            // if everything was cool with the send then
                            // test the link
                            if (ftd_sock_test_link(
                                sockp->lhostname,
                                sockp->rhostname,
                                sockp->lip,
                                sockp->rip,
                                sockp->port,
                                // TODO: this needs to be adjusted for link speed
                                FTD_SOCK_CONNECT_SHORT_TIMEO) != 1)
                                {
                                error_tracef(TRACEERR, "ftd_sock_recv():TIME03 %d %s, tid=%x", errnum,sock_strerror(errnum), GetCurrentThreadId());
                                return FTD_LG_NET_TIMEO;
                                }
                            else 
                                {
                                error_tracef(TRACEERR, "ftd_sock_recv():NET BROKEN3 %d %s, tid=%x", errnum,sock_strerror(errnum), GetCurrentThreadId());
                                return FTD_LG_NET_BROKEN;
                                }
                            }
                        elapsed_time = 0; 
                        }
                    lastts = now;
                    }
                }
        }        
        else if (rrc == 0) 
            {
            errnum = sock_errno();
            error_tracef(TRACEINF5, "ftd_sock_recv():NET BROKEN4 %d %s<%x><%x>, tid=%x", 
                errnum,sock_strerror(errnum), sockp, sockp->sock, GetCurrentThreadId());
            return FTD_LG_NET_BROKEN;
            }
        else 
            {
            lbuf += rrc;
            len -= rrc;
            // something was read
            elapsed_time = lastts = 0;
            }
        } // while

    return savelen;
}

/*
 * ftd_sock_check_connect --
 */
int
ftd_sock_check_connect(ftd_sock_t *fsockp)
{

    return sock_check_connect(fsockp->sockp);
}

/*
 * ftd_sock_check_recv --
 */
int
ftd_sock_check_recv(ftd_sock_t *fsockp, int timeo)
{

    return sock_check_recv(fsockp->sockp, timeo);
}

/*
 * ftd_sock_check_send --
 */
int
ftd_sock_check_send(ftd_sock_t *fsockp, int timeo)
{

    return sock_check_send(fsockp->sockp, timeo);
}

/*
 * ftd_sock_set_nonb --
 */
int
ftd_sock_set_nonb(ftd_sock_t *fsockp)
{

    if (sock_set_nonb(fsockp->sockp) < 0) {
        reporterr (ERRFAC, M_SOCKSETNONB, ERRCRIT, fsockp->sockp->sock,
            sock_strerror(sock_errno()));
        return -1;
    }

    return 0;
}

/*
 * ftd_sock_set_b --
 */
int
ftd_sock_set_b(ftd_sock_t *fsockp)
{

    if (sock_set_b(fsockp->sockp) < 0) {
        reporterr (ERRFAC, M_SOCKSETB, ERRCRIT, fsockp->sockp->sock,
            sock_strerror(sock_errno()));
        return -1;
    }

    return 0;
}

/*
 * ftd_sock_is_me --
 */
int
ftd_sock_is_me(ftd_sock_t *fsockp, int local)
{

    return sock_is_me(fsockp->sockp, local);
}

/*
 * ftd_sock_send_header --
 * send header structure to peer
 */

#ifdef TDMF_TRACE4
int ftd_sock_send_header(char tracing,char *mod,char *funct,ftd_sock_t *fsockp, ftd_header_t *header)
{
    char who [ 4 ];
    int rc;

    // for making the thace more readable...++
    if (mod[0] != 'C')  // like "C:\tdmf_nt\...   "
    {
        //strncpy (who,mod, diStrMax-1);
        //who[diStrMax-1] = '\0';
        //if (who[0] == 'P')
        if (mod[0] == 'P')
            sprintf ( who, "PRI" );
        else
            sprintf ( who, "SEC" );
    }
    // for making the thace more readable...--

    switch(header->msgtype) 
        {   
        case FTDCNOOP:
        //case FTDACKCHUNK:
        case FTDACKRSYNC:
                //error_tracef(TRACEINF5, "ftd_sock_send_header(): OK %s.%s Tx hdr: %s ",(mod[0] == 'C' ? mod:who),funct,tdmf_socket_msg[header->msgtype-1].cmd_txt);
                break;
        default:
                error_tracef(TRACEINF4, "ftd_sock_send_header(): %s.%s Tx hdr: %s tid=%x",(mod[0] == 'C' ? mod:who),funct,tdmf_socket_msg[header->msgtype-1].cmd_txt,GetCurrentThreadId());
                break;
        }

    rc = ftd_sock_send ( tracing,fsockp, (char*)header, sizeof(ftd_header_t));

    if (rc != sizeof(ftd_header_t))
        error_tracef(TRACEWRN, "ftd_sock_send_header(): %s.%s Tx hdr: %s, RC= %d , tid=%x***",(mod[0] == 'C' ? mod:who),funct,tdmf_socket_msg[header->msgtype-1].cmd_txt,rc, GetCurrentThreadId());
    
    return rc;
}
#else
int ftd_sock_send_header(char tracing,ftd_sock_t *fsockp, ftd_header_t *header)
{
    return (ftd_sock_send ( tracing,fsockp, (char*)header, sizeof(ftd_header_t)));
}
#endif

/*
 * ftd_sock_recv_header --
 * recv header structure from peer
 */
#ifdef TDMF_TRACE4
int ftd_sock_recv_header(char *mod,char *funct,ftd_sock_t *fsockp, ftd_header_t *header)
{
    char who [ 4 ];
    int rc = ftd_sock_recv(fsockp, (char*)header, sizeof(ftd_header_t));
    
    // for making the thace more readable...++
    if (mod[0] != 'C')  // like "C:\tdmf_nt\...   "
    {
        //strncpy (who,mod, diStrMax-1);
        //who[diStrMax-1] = '\0';
        //if (who[0] == 'P')
        if (mod[0] == 'P')
            sprintf ( who, "PRI" );
        else
            sprintf ( who, "SEC" );
    }
    // for making the thace more readable...--

    switch(header->msgtype) 
        {   
        case FTDACKRSYNC:
        case FTDACKCHUNK:
        case FTDCNOOP:
                break;

        case FTDCCHUNK:
        case FTDCCHKSUM:
        case FTDCRFBLK:
            error_tracef(TRACEINF4, "ftd_sock_recv_header():OK %s.%s Rx hdr: %s rc: %d",(mod[0] == 'C' ? mod:who),funct,
                            (rc == sizeof(ftd_header_t) ? tdmf_socket_msg[header->msgtype-1].cmd_txt: "??"),
                            (rc == sizeof(ftd_header_t) ? 0: rc));

                break;
        default:
            error_tracef(TRACEINF4, "ftd_sock_recv_header():%s.%s Rx hdr: %s rc: %d , tid=%x",(mod[0] == 'C' ? mod:who),funct,
                            (rc == sizeof(ftd_header_t) ? tdmf_socket_msg[header->msgtype-1].cmd_txt: "??"),
                            (rc == sizeof(ftd_header_t) ? 0: rc), GetCurrentThreadId());
            break;
        }
  
    return rc;
}
#else
int ftd_sock_recv_header(ftd_sock_t *fsockp, ftd_header_t *header)

{
    int rc = ftd_sock_recv(fsockp, (char*)header, sizeof(ftd_header_t));
  
    return rc;
}
#endif

/*
 * _ftd_sock_send_and_report_err --
 * send err structure to peer
 */
int
_ftd_sock_send_and_report_err(ftd_sock_t *fsockp, char *errkey, int severity, ...)
{
    ftd_header_t    ack;
    ftd_err_t       err;
    va_list         args;
    char            fmt[256];
    int             rc;

    memset(&ack, 0, sizeof(ack));

    ack.msgtype = FTDACKERR;
   if ((rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"_ftd_sock_send_and_report_err" ,fsockp, &ack)) < 0) {
        return rc;
    }

    // find error in error database
    if (errfac_get_errmsg(ERRFAC, errkey, fmt) < 0) {
        return -1;
    }
    strcpy(err.errkey, errkey);
    err.errcode = severity;

    strcpy(err.fnm, err_glb_fnm);
    err.lnum = err_glb_lnum;

    va_start(args, severity);
    vsprintf(err.msg, fmt, args);
  
    errfac_log_errmsg(ERRFAC, severity, errkey, err.msg);
 
    // send to peer
    if ((rc = ftd_sock_send ( FALSE,fsockp, (char*)&err, sizeof(err))) < 0) {
        return rc;
    }

    return 0;
}

/*
 * ftd_sock_send_err --
 * send err to peer
 */
int
ftd_sock_send_err(ftd_sock_t *fsockp, err_msg_t *errp)
{
    ftd_header_t    ack;
    ftd_err_t       err;
    int             rc;

    memset(&ack, 0, sizeof(ack));
    memset(&err, 0, sizeof(err));

    ack.msgtype = FTDACKERR;
    if ((rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_send_err",fsockp, &ack)) < 0) {
        return rc;
    }
    
    err.errcode = errp->level;
    err.length = strlen(errp->msg);
    strcpy(err.msg, errp->msg);

    if ((rc = ftd_sock_send ( FALSE,fsockp, (char*)&err, sizeof(err))) < 0) {
        return rc;
    }

    return 0;
}

/*
 * ftd_sock_recv_err --
 * recv err structure from peer
 */
int
ftd_sock_recv_err(ftd_sock_t *fsockp, ftd_header_t *header)
{
    ftd_err_t       err;
    int             rc;

    if ((rc = ftd_sock_recv(fsockp, (char*)&err, sizeof(err))) < 0) {
        return rc;
    }

    // report locally
#if defined( _TLS_ERRFAC )
	if ( ((errfac_t *)TlsGetValue( TLSIndex ))->reportwhere ) {;
#else
    if (ERRFAC->reportwhere) {
#endif

		err_glb_fnm = __FILE__;
        err_glb_lnum = __LINE__;
    }
    errfac_log_errmsg(ERRFAC, err.errcode, M_REMOTERR, err.msg);

    if (err.errcode == ERRCRIT) {
        return -1;
    }

    return 0;
}

/*
 * ftd_sock_send_hup --
 * tell peer to hup
 */
int
ftd_sock_send_hup(ftd_lg_t *lgp) 
{
    ftd_header_t    header;
    int             rc;

    error_tracef(TRACEINF5, "ftd_sock_send_hup: tid=%x", GetCurrentThreadId());

    memset(&header, 0, sizeof(header));
    
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCHUP;
    header.ackwanted = 1;
    
    if ((rc = ftd_sock_send_lg(lgp->dsockp, lgp,
        (char*)&header, sizeof(header))) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_hup(): rc=%d , tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    return 0;
}

/*
 * ftd_sock_send_version --
 * tell peer about version 
 */
int
ftd_sock_send_version(ftd_sock_t *fsockp, ftd_lg_t *lgp) 
{
    ftd_header_t    header, ack;
    ftd_version_t   version;
    struct iovec    iov[2];
    char            versionstr[256], remversionstr[256];
    int             i, rc, len;

    /* NOTE: socket is blocking when this is called */ 
    error_tracef(TRACEINF,"ftd_sock_send_version(): tid=%x", GetCurrentThreadId());

    memset(&header, 0, sizeof(ftd_header_t));
    memset(&version, 0, sizeof(ftd_version_t));

    /* get the system information */
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCVERSION;
    header.ackwanted = 1;
    header.msg.lg.lgnum = lgp->lgnum;

    (void)time(&header.ts);

    sprintf(version.configpath, "%s%03d.%s\n",
        SECONDARY_CFG_PREFIX, lgp->lgnum, PATH_CFG_SUFFIX);

    (void)time(&version.pmdts);

    /* set default protocol version numbers */
    strcpy(versionstr, "5.0.0");
    strcpy(version.version, versionstr);

    /* process the version number put into the works by the Makefile */
#ifdef VERSION
    strcpy(versionstr, VERSION);
    i = 0;
    /* eliminate beta, intermediate build information from version */
    while (versionstr[i]) {
        if ((!(isdigit(versionstr[i]))) && versionstr[i] != '.') {
            versionstr[i] = '\0';
            break;
        }
        i++;
    }
    versionstr[i] = '\0';
    strcpy (version.version, versionstr);
#endif
    (void)time(&version.pmdts);

    /* send header and version packets */
    iov[0].iov_base = (void*)&header;
    iov[0].iov_len = sizeof(ftd_header_t);
    iov[1].iov_base = (void*)&version;
    iov[1].iov_len = sizeof(ftd_version_t);

    if ((rc = ftd_sock_send_lg_vector(fsockp, lgp, iov, 2)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_version(): 1 rc=%d , tid=%x", rc, GetCurrentThreadId());
        return rc;
    }
    
    if ((rc = FTD_SOCK_RECV_HEADER(__FILE__,"ftd_sock_send_version",fsockp, &ack)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_version(): 2 rc=%d , tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    if (ack.msgtype == FTDACKERR) {
        error_tracef(TRACEERR,"ftd_sock_send_version(): FTDACKERR : tid=%x", GetCurrentThreadId());

        return ftd_sock_recv_err(fsockp, &ack);
    } else if (ack.msgtype == FTDACKKILL) {
        error_tracef(TRACEERR,"ftd_sock_send_version(): FTDACKKILL : tid=%x", GetCurrentThreadId());
        return -1;
    }

    memset(remversionstr, 0, sizeof(remversionstr));
    len = ack.msg.lg.data;

    if ((rc = ftd_sock_recv(fsockp, remversionstr, len)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_version(): 4 rc=%d , tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    //
    // TO ADD: compare only 1.3, not following numbers!
    // maybe later we will add a compare on the numbers if 
    // we make a major change in the protocol layer
    // or if there is a need for it.
    //
    // Right now disable version control till we put
    // the correct compare in
    //
#if 0
    if (strcmp(versionstr, remversionstr)) {
        error_tracef(TRACEERR,"ftd_sock_send_version(): 5 tid=%x", GetCurrentThreadId());
        reporterr(ERRFAC, M_VERSION, ERRCRIT, versionstr, remversionstr);
        return -1;
    }
#endif

    return 0;
}

/*
 * ftd_sock_send_handshake --
 * send initial handshake to peer 
 */
int
ftd_sock_send_handshake(ftd_lg_t *lgp) 
{
    struct iovec    iov[2];
    ftd_sock_t      *fsockp;
    ftd_auth_t      auth;
    ftd_header_t    header, ack;
    time_t          ts;
    int             rc, cp;

    // NOTE: network connection is blocking when this is called 

    fsockp = lgp->dsockp;

    memset(&header, 0, sizeof(header));
    memset(&auth, 0, sizeof(auth));

    // get the system information 
    time(&ts);

    ftd_sock_encode_auth(ts, FTD_SOCK_LHOST(fsockp),
        fsockp->sockp->lhostid, FTD_SOCK_LIP(fsockp), &auth.len, auth.auth);

    strcpy(auth.configpath,
        &(lgp->cfgp->cfgpath[strlen(lgp->cfgp->cfgpath)-8]));

    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCHANDSHAKE;
    header.ts = ts;
    header.ackwanted = 1;
    header.msg.lg.flags = lgp->flags;
    header.msg.lg.data = fsockp->sockp->lhostid;

    iov[0].iov_base = (void*)&header;
    iov[0].iov_len = sizeof(header);
    iov[1].iov_base = (void*)&auth;
    iov[1].iov_len = sizeof(auth);

    if ((rc = ftd_sock_send_lg_vector(fsockp, lgp, iov, 2)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_handshake(): 1 tid=%x", GetCurrentThreadId());
        return rc;
    }
    
    if ((rc = FTD_SOCK_RECV_HEADER(LG_PROCNAME(lgp),"ftd_sock_send_handshake",fsockp, &ack)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_handshake(): 2 tid=%x", GetCurrentThreadId());
        return rc;
    }

    if (ack.msgtype == FTDACKERR) {
        error_tracef(TRACEERR,"ftd_sock_send_handshake(): 3 tid=%x", GetCurrentThreadId());
        return ftd_sock_recv_err(fsockp, &ack);
    } else if (ack.msgtype == FTDACKKILL) {
        error_tracef(TRACEERR,"ftd_sock_send_handshake(): 4 tid=%x", GetCurrentThreadId());
        return -1;
    }

    cp = FALSE;

    if (GET_LG_CPON(ack.msg.lg.flags)) {
        cp = TRUE;
        SET_LG_CPON(lgp->flags);
    }

    // set checkpoint flag in pstore
    if (ps_set_group_checkpoint(lgp->cfgp->pstore,
        lgp->devname, cp) < 0)
    {
        error_tracef(TRACEERR,"ftd_sock_send_handshake(): 5 tid=%x", GetCurrentThreadId());
        return -1;
    }

    return 0;
}

/*
 * ftd_sock_send_chkconfig --
 * send device configuration to peer 
 */
int
ftd_sock_send_chkconfig(ftd_lg_t *lgp) 
{
    struct iovec    iov[2];
    ftd_sock_t      *fsockp;
    ftd_header_t    header, ack;
    ftd_rdev_t      rdev;
    ftd_dev_t       **devpp, *devp;
    ftd_dev_cfg_t   *devcfgp;
    daddr_t         locsize;
    int             rc;

    // NOTE: socket should be blocking when this is called  
    
    fsockp = lgp->dsockp;

    memset(&header, 0, sizeof(header));
    memset(&rdev, 0, sizeof(rdev));

    /*
     * walk through devs, send believed mirror device, for size of the device
     */
    ForEachLLElement(lgp->devlist, devpp) {
        devp = *devpp;

        if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, devp->devid)) == NULL) {
            reporterr(ERRFAC, M_PROTDEV, ERRCRIT, devp->devid);
            error_tracef(TRACEERR,"ftd_sock_send_chkconfig(): 1 tid=%x", GetCurrentThreadId());
            return -1;
        }

        if (lgp->cfgp->chaining) 
		{
            HANDLE fd = ftd_dev_lock(devcfgp->pdevname, lgp->lgnum);
            if (fd == INVALID_HANDLE_VALUE) {
                reporterr(ERRFAC, M_LOCSIZ, ERRWARN, devcfgp->pdevname, ftd_strerror());

                error_tracef(TRACEERR,"ftd_sock_send_chkconfig(): 2 tid=%x", GetCurrentThreadId());
                return -1;
            }
            locsize = ftd_dev_locked_disksize(fd);

            ftd_dev_unlock(fd);
        } 
		else 
		{
            locsize = disksize(devcfgp->pdevname);
        }

        if (locsize < 0) 
		{
            reporterr (ERRFAC, M_LOCSIZ, ERRCRIT,
                devcfgp->devname, ftd_strerror());
            error_tracef(TRACEERR,"ftd_sock_send_chkconfig(): 3 tid=%x", GetCurrentThreadId());
            return -1;
        }

        // send mirror device name to the remote system for verification 
        header.magicvalue = MAGICHDR;
        header.msgtype = FTDCCHKCONFIG;
        (void)time(&header.ts);
        header.ackwanted = 1;
        header.msg.lg.devid = devp->devid;

        rdev.devid = devp->devid;
        rdev.minor = devp->num;
        rdev.ftd = devp->ftdnum;

        (void)strcpy(rdev.path, devcfgp->sdevname);
        rdev.len = strlen(devcfgp->sdevname);

        iov[0].iov_base = (void*)&header;
        iov[0].iov_len = sizeof(header);
        iov[1].iov_base = (void*)&rdev;
        iov[1].iov_len = sizeof(rdev);

        if ((rc = ftd_sock_send_lg_vector(fsockp, lgp, iov, 2)) < 0) {
            error_tracef(TRACEERR,"ftd_sock_send_chkconfig(): 4 tid=%x", GetCurrentThreadId());
            return rc;
        }

        if ((rc = FTD_SOCK_RECV_HEADER(LG_PROCNAME(lgp),"ftd_sock_send_chkconfig",fsockp, &ack)) < 0) {
            error_tracef(TRACEERR,"ftd_sock_send_chkconfig(): 5 tid=%x", GetCurrentThreadId());
            return rc;
        }
        
        if (ack.msgtype == FTDACKERR) {
            error_tracef(TRACEERR,"ftd_sock_send_chkconfig(): 6 tid=%x", GetCurrentThreadId());
            return ftd_sock_recv_err(fsockp, &ack);
        } else if (ack.msgtype == FTDACKKILL) {
            error_tracef(TRACEERR,"ftd_sock_send_chkconfig(): 7 tid=%x", GetCurrentThreadId());
            return -1;
        }

        // check if remote device size is less than local data disk 
        if (locsize > ack.msg.lg.data) {
            reporterr (ERRFAC, M_MIR2SMAL, ERRCRIT,
                lgp->cfgp->cfgpath, devcfgp->pdevname, devcfgp->sdevname, 
                locsize, (daddr_t)ack.msg.lg.data);
            error_tracef(TRACEERR,"ftd_sock_send_chkconfig(): 8 tid=%x", GetCurrentThreadId());
            return -1;
        }
    }

    return 0;
}

/*
 * ftd_sock_send_noop --
 * send a NOOP message  
 */
int
ftd_sock_send_noop(ftd_sock_t *fsockp, int lgnum, int ackwanted) 
{
    ftd_header_t    header;
    int             rc;

    memset(&header, 0, sizeof(header));
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCNOOP;
    header.ackwanted = ackwanted;
    header.msg.lg.lgnum = lgnum;

    if ((rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_send_noop",fsockp, &header)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_noop() : tid=%x", GetCurrentThreadId());
        return rc;
    }

    return 0;
}
  
/*
 * ftd_sock_send_rsync_block --
 * send a data block to peer 
 */
int
ftd_sock_send_rsync_block(ftd_sock_t *fsockp, ftd_lg_t *lgp, 
    ftd_dev_t *devp, int ackwanted) 
{
    ftd_header_t    header;
    char            *datap;
    int             length, rc, state;
    struct iovec    iov[2];


    if (lgp->tunables->chunkdelay > 0) 
        {
        Sleep(lgp->tunables->chunkdelay);
        }

    memset(&header, 0, sizeof(header));
    memset(iov, 0, sizeof(struct iovec));

    state = GET_LG_STATE(lgp->flags);

    header.magicvalue = MAGICHDR;
    header.msgtype = state == FTD_SBACKFRESH ? FTDCBFBLK: FTDCRFBLK;
    header.compress = lgp->compp->algorithm;
    header.uncomplen = devp->rsynclen << DEV_BSHIFT;
    header.ackwanted = ackwanted;

    header.msg.lg.devid = devp->devid;
    header.msg.lg.offset = devp->rsyncoff;
    header.msg.lg.len = devp->rsynclen;

    iov[0].iov_base = (void*)&header;
    iov[0].iov_len = sizeof(header);

    if (buf_all_zero(devp->rsyncbuf, devp->rsyncbytelen)) {
        header.len = devp->rsyncbytelen;
        header.msg.lg.data = FTDZERO;

        // bump actual data count 
        devp->statp->actual += sizeof(header);

        if ((rc = ftd_sock_send_lg_vector(fsockp, lgp, iov, 1)) < 0) {
            error_tracef(TRACEERR,"ftd_sock_send_rsync_block(): ftd_sock_send_lg_vector() error rc=%d : tid=%x", rc, GetCurrentThreadId());
            return rc;
        }
    } else {
        if (lgp->compp->algorithm != 0) {
            // compress data 
            if ((length = devp->rsyncbytelen) > lgp->cbuflen) {
                lgp->cbuflen = length + (length >> 1) + 1;
                lgp->cbuf = (char*)realloc(lgp->cbuf, lgp->cbuflen);
            }
            length = (int)comp_compress((u_char*)lgp->cbuf,
                (u_int*)&lgp->cbuflen,
                (u_char*)devp->rsyncbuf,
                devp->rsyncbytelen,
                lgp->compp);
            datap = lgp->cbuf;
        } else {
            datap = devp->rsyncbuf;
            length = devp->rsyncbytelen;
        } 
        header.len = length;

        iov[1].iov_base = (void*)datap;
        iov[1].iov_len = length;

        if ((rc = ftd_sock_send_lg_vector(fsockp, lgp, iov, 2)) < 0) {
            return rc;
        }
        // bump actual data count 
        devp->statp->actual += sizeof(ftd_header_t) + header.len;
    }

    // bump effective data count 
    devp->statp->effective += sizeof(ftd_header_t) + header.uncomplen;

    return 0;
}

/*
 * ftd_sock_send_rsync_end --
 * tell peer to transition this lg out of rsync mode
 */
int
ftd_sock_send_rsync_end(ftd_sock_t *fsockp, ftd_lg_t *lgp, int state) 
{
    ftd_header_t    header;
    int             rc;

    memset(&header, 0, sizeof(header));
    header.magicvalue = MAGICHDR;
    
    switch(state) {
    case FTD_SBACKFRESH:
        header.msgtype = FTDCBFEND;
        break;  
    case FTD_SREFRESH:
    case FTD_SREFRESHC:
        header.msgtype = FTDCRFEND;
        break;  
    case FTD_SREFRESHF:
        header.msgtype = FTDCRFFEND;
        break;  
    default:
        header.msgtype = -1;
        break;
    }
    
    header.msg.lg.devid = lgp->lgnum;
    header.ackwanted = 0;
    
    if ((rc = ftd_sock_send_lg(lgp->dsockp, lgp,
        (char*)&header, sizeof(header))) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_rsync_end(): ftd_sock_send_lg() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    return 0;
}

/*
 * ftd_sock_send_rsync_chksum --
 * send a lg rsync checksum packet to peer 
 */
int
ftd_sock_send_rsync_chksum(ftd_sock_t *fsockp, ftd_lg_t *lgp,
    LList *devlist, int msgtype)
{
    ftd_header_t    header;
    ftd_dev_t       **devpp, *devp;
    int             iovcnt, rc, digestlen;
    struct iovec    iov[FTD_MAX_DEVICES];

    memset(&header, 0, sizeof(header));

    header.magicvalue = MAGICHDR;
    header.msgtype = msgtype;
    header.ackwanted = 1;
    header.len = 1;

    header.msg.lg.lgnum = lgp->lgnum;

    ForEachLLElement(devlist, devpp) {
        devp = *devpp;
        if (devp->sumlen == 0) {
            continue;
        }
        digestlen = devp->sumnum * DIGESTSIZE;

        header.msg.lg.devid = devp->devid;

        iovcnt = 0;

        iov[iovcnt].iov_base = (void*)&header;
        iov[iovcnt].iov_len = sizeof(header);
        iovcnt++;
        iov[iovcnt].iov_base = (void*)devp;
        iov[iovcnt].iov_len = sizeof(ftd_dev_t);
        iovcnt++;
        iov[iovcnt].iov_base = (void*)devp->sumbuf;
        iov[iovcnt].iov_len = digestlen;
        iovcnt++;

        devp->statp->actual += sizeof(ftd_dev_t) + digestlen;
        devp->statp->effective += sizeof(ftd_dev_t) + digestlen;

        if ((rc = ftd_sock_send_lg_vector(fsockp, lgp, iov, iovcnt)) < 0) {
            error_tracef(TRACEERR,"ftd_sock_send_rsync_chksum(): ftd_sock_send_lg_vector() error rc=%d : tid=%x", rc, GetCurrentThreadId() );
            return rc;
        }
    }

    ForEachLLElement(devlist, devpp) {
        (*devpp)->sumoff = 0;
        (*devpp)->sumlen = 0;
    }

    return 0;
}

/*
 * ftd_sock_send_rsync_start --
 * tell peer to transition into rsync mode 
 */
int
ftd_sock_send_rsync_start(ftd_sock_t *fsockp, ftd_lg_t *lgp, int msgtype) 
{
    ftd_header_t    header;
    int             rc;

    memset(&header, 0, sizeof(header));
    header.magicvalue = MAGICHDR;
    header.msgtype = msgtype;
    
    if (GET_LG_BACK_FORCE(lgp->flags)) {
        SET_LG_BACK_FORCE(header.msg.lg.flags);
    }

    UNSET_LG_BACK_FORCE(lgp->flags);

    header.ackwanted = 1;
    
    if ((rc = ftd_sock_send_lg(fsockp, lgp,
        (char*)&header, sizeof(header))) < 0)
    {
        error_tracef(TRACEERR,"ftd_sock_send_rsync_start(): ftd_sock_send_lg() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    return 0;
}

/*
 * ftd_sock_send_rsync_devs --
 * tell peer to put this device into refresh mode
 */
int
ftd_sock_send_rsync_devs(ftd_sock_t *fsockp, ftd_lg_t *lgp, ftd_dev_t *devp,
    int state) 
{
    ftd_header_t    header;
    int             rc;

    memset(&header, 0, sizeof(header));
    
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCRSYNCDEVS;
    header.msg.lg.devid = devp->devid;
    header.msg.lg.offset = devp->rsyncoff;
    header.msg.lg.data = devp->dbres;
    header.ackwanted = 1;

    if ((rc = ftd_sock_send_lg(fsockp, lgp,
        (char*)&header, sizeof(header))) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_rsync_devs(): ftd_sock_send_lg() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }
    
    return 0;
}

/*
 * ftd_sock_send_rsync_deve --
 * tell peer to take this device out of rsync mode
 */
int
ftd_sock_send_rsync_deve(ftd_sock_t *fsockp, ftd_lg_t *lgp, ftd_dev_t *devp,
    int state) 
{
    ftd_header_t    header;
    int             rc;

    memset(&header, 0, sizeof(header));
    
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCRSYNCDEVE;
    header.msg.lg.devid = devp->devid;
    header.ackwanted = 1;
    
    if ((rc = ftd_sock_send_lg(fsockp, lgp,
        (char*)&header, sizeof(header))) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_rsync_deve(): ftd_sock_send_lg() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    return 0;
}

/*
 * ftd_sock_send_bab_chunk --
 * tell peer to take this device out of backfresh mode
 */
int
ftd_sock_send_bab_chunk(ftd_lg_t *lgp) 
{
    ftd_sock_t      *fsockp;
    struct iovec    iov[2];
    ftd_header_t    header;
    char            *buf;
    int             length, rc, buflen;

    memset(&header, 0, sizeof(header));

    buf = lgp->buf;
    buflen = lgp->datalen;

    fsockp = lgp->dsockp;

    // create a header for the current entry 
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCCHUNK;
    header.msg.lg.devid = 0;
    header.ts = lgp->ts;
    header.ackwanted = 1;
    header.compress = lgp->compp->algorithm;
    header.uncomplen = lgp->datalen;

    if (header.compress) {
        if (lgp->datalen > lgp->cbuflen) {
            lgp->cbuflen = lgp->datalen + (lgp->datalen >> 1) + 1;
            lgp->cbuf =
                (char*)realloc((void*)lgp->cbuf, lgp->cbuflen);  
        }
        length = comp_compress((u_char*)lgp->cbuf,
            (u_int*)&lgp->cbuflen,
            (u_char*)buf,
            buflen,
            lgp->compp);

        buf = lgp->cbuf;

        if (buflen) {
            lgp->cratio = (float)((float)length / (float)buflen);
        } else {
            lgp->cratio = 1;
        }
    } else {
        length = buflen;
    }
    
    header.len = header.msg.lg.len = length;

    //error_tracef( TRACEINF,       " ftd_sock_send_chunk: writing header, magicvalue = %08x\n",        header.magicvalue));
    //error_tracef( TRACEINF,           " ftd_sock_send_chunk: writing %d bytes to socket\n",       header.len));

    // send the header and the data 
    iov[0].iov_base = (void*)&header;
    iov[0].iov_len = sizeof(ftd_header_t);
    iov[1].iov_base = (void*)buf;
    iov[1].iov_len = header.len;

    rc = ftd_sock_send_lg_vector(fsockp, lgp, iov, 2);
    if ( rc < 0 ) {
        error_tracef(TRACEERR,"ftd_sock_send_bab_chunk(): ftd_sock_send_lg_vector() error rc=%d : tid=%x", rc, GetCurrentThreadId());
    }

    return rc;
}

/*
 * ftd_sock_send_exit --
 * send an exit command to the peer
 */
int
ftd_sock_send_exit(ftd_sock_t *fsockp) 
{
    ftd_header_t    header;
    int             rc;

    memset(&header, 0, sizeof(header));
    
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCEXIT;
    
    if ((rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_send_exit",fsockp, &header)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_exit(): FTD_SOCK_SEND_HEADER() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    return 0;
}

/*
 * ftd_sock_send_start_pmd --
 * send a start pmd msg to master  
 */
int
ftd_sock_send_start_pmd(int lgnum, ftd_sock_t *fsockp) 
{
    ftd_header_t    header;
    int             rc;

    memset(&header, 0, sizeof(header));
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCSTARTPMD;
    header.ackwanted = 1;
    header.cli = (HANDLE)1;
    header.msg.lg.lgnum = lgnum;

    if ((rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_send_start_pmd",fsockp, &header)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_start_pmd(): FTD_SOCK_SEND_HEADER() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    return 0;
}
  
/*
 * ftd_sock_send_start_apply --
 * send a start apply msg to master  
 */
int
ftd_sock_send_start_apply(int lgnum, ftd_sock_t *fsockp, int cpon) 
{
    ftd_header_t header;
    int             rc;

    memset(&header, 0, sizeof(header));
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCSTARTAPPLY;
    header.ackwanted = 1;
    header.cli = (HANDLE)1;
    header.msg.lg.lgnum = lgnum;
    header.msg.lg.data = cpon;


    if ((rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_send_start_apply",fsockp, &header)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_start_apply(): FTD_SOCK_SEND_HEADER() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    // wait for ACK from master
    if ((rc = FTD_SOCK_RECV_HEADER(__FILE__ ,"ftd_sock_send_start_apply",fsockp, &header)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_start_apply(): FTD_SOCK_RECV_HEADER() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    return header.msg.lg.data;  // 1 = already running
}
  
/*
 * ftd_sock_send_start_reco --
 * send a start reco msg to master  
 */
int
ftd_sock_send_start_reco(int lgnum, ftd_sock_t *fsockp, int force) 
{
    ftd_header_t header;
    int             rc;

    memset(&header, 0, sizeof(header));
    
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCSTARTRECO;
    header.ackwanted = 1;
    header.cli = (HANDLE)1;
    header.msg.lg.lgnum = lgnum;
    header.msg.lg.data = force;

    if ((rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_send_start_reco",fsockp, &header)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_start_reco(): FTD_SOCK_SEND_HEADER() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    // wait for ACK from master
    if ((rc = FTD_SOCK_RECV_HEADER(__FILE__ ,"ftd_sock_send_start_reco",fsockp, &header)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_start_reco(): FTD_SOCK_RECV_HEADER() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    return header.msg.lg.data;  // 1 = group apply already running, -1 = error
}
  
/*
 * ftd_sock_send_stop_apply --
 * send a stop apply msg to master  
 */
int
ftd_sock_send_stop_apply(int lgnum, ftd_sock_t *fsockp) 
{
    ftd_header_t    header;
    int             rc;

    memset(&header, 0, sizeof(header));
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCSTOPAPPLY;
    header.ackwanted = 1;
    header.cli = (HANDLE)1;
    header.msg.lg.lgnum = lgnum;

#if defined(_WINDOWS)
    sprintf((char*)header.msg.data, "RMDA_%03d", header.msg.lg.lgnum);
    header.msgtype = FTDCSIGTERM;
#endif

    if ((rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_send_stop_apply",fsockp, &header)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_stop_apply(): FTD_SOCK_SEND_HEADER() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    // wait for ACK from master
    if ((rc = FTD_SOCK_RECV_HEADER(__FILE__ ,"ftd_sock_send_stop_apply",fsockp, &header)) < 0) {
        error_tracef(TRACEERR,"ftd_sock_send_stop_apply(): FTD_SOCK_RECV_HEADER() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    return 0;
}
  
/*
 * ftd_sock_encode_auth --
 * encode an authorization handshake string
 */
static void
ftd_sock_encode_auth(time_t ts, char *hostname, u_long hostid, u_long ip,
            int *encodelen, char *encode)
{
    encodeunion *kp, key1, key2;
    int         i, j;
    u_char      k, t;

    key1.ul = ((u_long) ts) ^ ((u_long) hostid);
    key2.ul = ((u_long) ts) ^ ((u_long) ip);
  
    i = j = 0;
    while (i < (int)strlen(hostname)) {
        kp = ((i%8) > 3) ? &key1 : &key2;
        k = kp->uc[i%4];
        t = (u_char) (0x000000ff & ((u_long)k ^ (u_long) hostname[i++]));
        sprintf (&(encode[j]), "%02x", t);
        j += 2;
    }
    encode[j] = '\0';
    *encodelen = j;

    return;
}

/*
 * ftd_sock_decode_auth --
 * decode an authorization handshake string
 */
static void
ftd_sock_decode_auth (int encodelen, char *encodestr, time_t ts,
    u_long hostid, char *hostname, size_t hostname_len, u_long ip)
{
    encodeunion key1, key2, *kp;
    int         i, j, temp;
    u_char      k, t;

    key1.ul = ((u_long) ts) ^ ((u_long) hostid);
    key2.ul = ((u_long) ts) ^ ((u_long) ip);

    i = j = 0;
    while (j < encodelen && i < (int)hostname_len - 1) {
        kp = ((i%8) > 3) ? &key1 : &key2;
        k = kp->uc[i%4];
        sscanf ((encodestr+j), "%2x", &temp);
        t = (unsigned char) (0x000000ff & temp);
        hostname[i++] = (char)(0x0000007f & ((u_long)k ^ (u_long)t));
        j += 2;
    }
    hostname[i] = '\0';

    return;
}

/*
 * ftd_sock_recv_version --
 * verify version info from peer 
 */
int
ftd_sock_recv_version(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    ftd_header_t    ack;
    ftd_version_t   version;
    struct stat     statbuf;
    int             i = 0, rc;
    time_t          tsdiff, currentts;
    char            primary_version_str[64], secondary_version_str[64];  
    char            local_configpath[MAXPATH];

    rc = ftd_sock_recv(fsockp, (char*)&version, sizeof(version));
    if (rc < 0) {
        error_tracef(TRACEERR,"ftd_sock_recv_version(): ftd_sock_recv() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }

    /* figure out time differentials between primary and secondary */
    (void)time(&currentts);
    tsdiff = currentts - version.pmdts;
    lgp->tsbias = tsdiff;
  
    strcpy(primary_version_str, "5.0.0");
    strcpy(secondary_version_str, "5.0.0");

    /* figure out the version number put into the works by the Makefile */
#ifdef VERSION
    strcpy(secondary_version_str, VERSION);

    i = 0;
    /* eliminate beta, intermediate build information from version */
    while (secondary_version_str[i]) {
        if ((!(isdigit(secondary_version_str[i])))
        && secondary_version_str[i] != '.') {
            secondary_version_str[i] = '\0';
            break;
        }
        i++;
    }
#endif
	secondary_version_str[i] = '\0';

//
// SVG 02-06-03 
//
// Check only first parts of version to see if they are similar...
// We allow a difference in the last part of the version number
// 1.x.y (y is open) i.e. 1.3.16 can talk to 1.3.17
//
// Right now we check the first 4 chars... 
// Which means we will catch 1.15.yy versus 1.16.yy
// but not 1.151.yy versus 1.155.yy
//
    /* capture the pmd's version number */
    strcpy(primary_version_str, version.version);

#ifdef _OLD_VERSION_CHECKING_
    /* protocol version mismatch checking goes here */
    if (strcmp(primary_version_str, secondary_version_str)) {
        reporterr(ERRFAC, M_VERSION, ERRWARN, 
            primary_version_str, secondary_version_str);
        return -1;
    }
#else
    /* protocol version mismatch checking goes here */
    if (strncmp(primary_version_str, secondary_version_str,4)) {
        reporterr(ERRFAC, M_VERSION, ERRWARN, 
            primary_version_str, secondary_version_str);
        return -1;
    }
#endif

    /* eliminate newline from configpath */
    i = 0;
    while (version.configpath[i]) {
        if (version.configpath[i] == '\n') {
            version.configpath[i] = '\0';
            break;
        }
        i++;
    }
    
    // need to use local registry entries for config path
#if defined(_WINDOWS)   
    sprintf(local_configpath, "%s\\%s", PATH_CONFIG, version.configpath);
#else
    sprintf(local_configpath, "%s/%s", PATH_CONFIG, version.configpath);
#endif
    if (stat(local_configpath, &statbuf) == -1) {
        reporterr(ERRFAC, M_CFGFILE, ERRWARN, 
            local_configpath, ftd_strerror());
        return -1;
    }

    memset(&ack, 0, sizeof(ack));
    ack.msg.lg.data = strlen(secondary_version_str);
    

    FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_recv_version",fsockp, &ack);
    ftd_sock_send ( FALSE,fsockp, secondary_version_str,        strlen(secondary_version_str));

    return 0;
}

/*
 * ftd_sock_recv_refoflow --
 * refresh overflow on primary
 */
int
ftd_sock_recv_refoflow(ftd_lg_t *lgp) 
{
    return FTD_CTRACKING;
}

/*
 * ftd_sock_recv_handshake --
 * process a handshake message
 */
int
ftd_sock_recv_handshake(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    ftd_header_t    ack;
    ftd_auth_t      auth;
    time_t          ts;
    int             rc;
    char            hostname[MAXHOST];

    rc = ftd_sock_recv(fsockp, (char*)&auth, sizeof(auth));
    if (rc < 0) {
        // config file problem 
        reporterr(ERRFAC, M_CFGPROB, ERRCRIT);
        return -1;
    }
    
    if ((rc = ftd_sock_is_me(fsockp, 1)) < 0) {
        // init network problem 
        reporterr(ERRFAC, M_NETPROB, ERRCRIT);
        return -2;
    }

    // we steal the data field to pass remote hostid 
    fsockp->sockp->rhostid = header->msg.lg.data;

    strcpy(hostname, FTD_SOCK_RHOST(fsockp));
    ts = header->ts;

    if (auth.len > sizeof(auth.auth)) {
        // authentication error 
        reporterr(ERRFAC, M_BADAUTH, ERRCRIT,
            hostname,
            "invalid authentication packet - length");
        return -3;
    }
    
    ftd_sock_decode_auth(auth.len, auth.auth, ts, fsockp->sockp->rhostid, 
        FTD_SOCK_RHOST(fsockp), sizeof(FTD_SOCK_RHOST(fsockp)),
        FTD_SOCK_RIP(fsockp));

    if (strcmp(FTD_SOCK_RHOST(fsockp), hostname)) {
        // authentication error 
        reporterr(ERRFAC, M_BADAUTH, ERRCRIT,
            hostname,
            "invalid authentication packet - hostname");
        return -3;
    }

    memset(&ack, 0, sizeof(ftd_header_t));
    ack.msgtype = FTDACKHANDSHAKE;

    SET_LG_STATE(lgp->flags, GET_LG_STATE(header->msg.lg.flags));

    if (GET_LG_JLESS(header->msg.lg.flags))
        {
        SET_LG_JLESS(lgp->flags);
        }
    if (GET_LG_CPON(lgp->flags)) {
        // tell primary that we are in checkpoint mode 
        SET_LG_CPON(ack.msg.lg.flags);
    }

    ack.msg.lg.devid = header->msg.lg.devid;


    if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_sock_recv_handshake",fsockp, &ack)) < 0) {
        return -1;
    } 
   
    return 0;
}

/*
 * ftd_sock_recv_chkconfig --
 * process a chkconfig message
 */
int
ftd_sock_recv_chkconfig(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    ftd_header_t    ack;
    ftd_dev_t       *devp;
    ftd_dev_cfg_t   *devcfgp;
    ftd_rdev_t      rdev;
    time_t          nowts;
    struct stat     statbuf;
    char            path[MAXPATH];
    int             size, tsbias, rc;

    rc = ftd_sock_recv(fsockp, (char*)&rdev, sizeof(ftd_rdev_t));
    if (rc < 0) 
	{
        error_tracef(TRACEERR,"ftd_sock_recv_chkconfig(): ftd_sock_recv() error rc=%d : tid=%x", rc, GetCurrentThreadId());
        return rc;
    }
    
    (void)time(&nowts);
    tsbias = nowts - header->ts;
    fsockp->sockp->tsbias = tsbias;

    if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, rdev.devid)) == NULL) {
        // cant't find this dev id in secondary config
        reporterr(ERRFAC, M_MIRMISMTCH, ERRCRIT, rdev.devid);
        return -1;
    }

    if ((devp = ftd_lg_devid_to_dev(lgp, rdev.devid)) == NULL) 
    {
        // cant't find this dev id in secondary config
        reporterr(ERRFAC, M_MIRDEVID, ERRCRIT, rdev.devid);
        return -1;
    }

    if (0 == strcmp(rdev.path, devcfgp->sdevname)) 
	{
        devp->num = rdev.minor;
        devp->ftdnum = rdev.ftd;

        size = devp->devsize;
        if (size == (u_long)-1) 
		{
            reporterr(ERRFAC, M_MIRSIZ, ERRCRIT, devcfgp->sdevname, "error: Size of device reported as -1\n");
            return -1;
        }
    } 
	else 
	{
        // mir names don't match
        reporterr(ERRFAC, M_MIRMISMTCH, ERRCRIT, rdev.devid);
        return -1;
    }

    if (GET_LG_STATE(lgp->flags) != FTD_SBACKFRESH) {
        /*
         * Make sure that ftd server for this is enabled.
         * If not, then reporterr only for the FIRST devid (0) refer to WR17045
		 * The state is NORMAL for remaining devids inside the same group 
         * backfresh - ok
         */
        strcpy(path, lgp->cfgp->cfgpath);
        strcpy(path + strlen(path) - 4, ".off");
        if ( ( stat(path, &statbuf) == 0 ) && ( rdev.devid == 0 ) ) 
		{
            reporterr(ERRFAC, M_MIRDISABLE, ERRCRIT, path);
            ftd_sock_send_err(lgp->dsockp, ftd_get_last_error(ERRFAC));
        }
    }

    /* return mirror volume size for a configuration query */
    ack.msgtype = FTDACKCONFIG;
    ack.msg.lg.data = size;
    ack.msg.lg.devid = devp->devid;


    if (FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp),"ftd_sock_recv_chkconfig",fsockp, &ack) == -1) {
        return -1;
    } 
    
    return 0;
}

/*
 * ftd_sock_recv_hup --
 * process a hup message
 */
int
ftd_sock_recv_hup(ftd_sock_t *fsockp, ftd_header_t *header) 
{
    ftd_header_t    ack;

    memset(&ack, 0, sizeof(ack));
    ack.msgtype = FTDACKHUP;
    ack.msg.lg.devid = header->msg.lg.devid;
    ack.msg.lg.data = FTDACKHUP;


    if (FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_recv_hup",fsockp, &ack) < 0) {
        return -1;
    }

    return 0;
}

/*
 * ftd_sock_recv_rsync_chksum --
 * process a rsync checksum packet
 */
int
ftd_sock_recv_rsync_chksum(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    ftd_header_t    ack;
    ftd_dev_t       *devp, rdevp;
    ftd_dev_cfg_t   *devcfgp;
    struct iovec    iov[2];
    int             deltamap_len = 0, digestlen = 0, rc, ret, i, state;
    ftd_dev_delta_t *dp;

    // get device structure from lg device list
    if ((devp = ftd_lg_devid_to_dev(lgp, header->msg.lg.devid)) == NULL) {  
        reporterr(ERRFAC, M_PROTDEV, ERRCRIT,
            header->msg.lg.devid);
        ret = -1;
        goto errret;
    }
    
    devp->statp->actual += sizeof(ftd_header_t);
    devp->statp->effective += sizeof(ftd_header_t);

    // get device config structure from lg device config list
    if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, header->msg.lg.devid)) == NULL) { 
        reporterr(ERRFAC, M_PROTDEV, ERRCRIT, header->msg.lg.devid);
        return -1;
    }

    // read primary device data structure from the network
    if ((rc = ftd_sock_recv(fsockp, (char*)&rdevp, sizeof(ftd_dev_t))) < 0) {
        ret = rc;
        error_tracef(TRACEERR,"ftd_sock_recv_rsync_chksum(): ftd_sock_recv() error 1 rc=%d : tid=%x", rc, GetCurrentThreadId());
        goto errret;
    }
    
    devp->statp->actual += sizeof(ftd_dev_t);
    devp->statp->effective += sizeof(ftd_dev_t);

    // make sure the local digest buffer is large enough 
    if (rdevp.sumbuflen > devp->sumbuflen) {
        devp->sumbuflen = rdevp.sumbuflen; 
        devp->sumbuf = (char*)realloc((char*)devp->sumbuf, devp->sumbuflen);
    }

    digestlen = rdevp.sumnum * DIGESTSIZE;
    rdevp.sumbuf = (devp->sumbuf + digestlen);

    // read digest buffer from the network/primary
    if ((rc = ftd_sock_recv(fsockp, (char*)rdevp.sumbuf, digestlen)) < 0) {
        ret = rc;
        error_tracef(TRACEERR,"ftd_sock_recv_rsync_chksum(): ftd_sock_recv() error 2 rc=%d : tid=%x", rc, GetCurrentThreadId());
        goto errret;
    }

    // make sure the local data buffer is large enough 
    if (rdevp.rsyncbytelen > lgp->buflen) {
        lgp->buflen = rdevp.rsyncbytelen; 
        lgp->buf = (char*)realloc((char*)lgp->buf, lgp->buflen);
    }

    devp->sumnum = rdevp.sumnum;        /* # chksums */
    devp->sumcnt = rdevp.sumcnt;        /* # chksum lists */
    devp->sumoff = rdevp.sumoff;        /* data offset sum represents */
    devp->sumlen = rdevp.sumlen;        /* data len sum represents */

    devp->rsyncoff = rdevp.sumoff;                      /* blocks */
    devp->rsyncbytelen = rdevp.sumlen;  /* data len sum represents */
    devp->rsynclen = devp->rsyncbytelen >> DEV_BSHIFT;  /* blocks */

    // point device at lg buffer (we do one dev at a time so ok)
    devp->rsyncbuf = lgp->buf;
    
    // compute checksums for mirror data block
    if ((rc = ftd_rsync_chksum_seg(lgp, devp)) == -1) {
        reporterr(ERRFAC, M_CHKSUM, ERRCRIT, devcfgp->sdevname);
        ret = -1;
        goto errret;
    }

    // diff the checksum digests for device and build a delta list
    if (ftd_rsync_chksum_diff(devp, &rdevp) == -1) {
        ret = -1;
        error_tracef(TRACEERR,"ftd_sock_recv_rsync_chksum(): ftd_rsync_chksum_diff() error : tid=%x", GetCurrentThreadId());
        goto errret;
    }

    deltamap_len = SizeOfLL(devp->deltamap) * sizeof(ftd_dev_delta_t);

    if (deltamap_len == 0) {
        // send an ack containing clean len  
        memset(&ack, 0, sizeof(ack));
        ack.msgtype = FTDACKRSYNC;
        ack.msg.lg.devid = rdevp.devid;
        ack.msg.lg.len = (rdevp.sumlen >> DEV_BSHIFT);


        if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp),"ftd_sock_recv_rsync_chksum" ,fsockp, &ack)) < 0) {
            ret = rc;
            goto errret;
        }
        
        // bump effective count since there are no deltas for this block
        devp->statp->effective += rdevp.sumlen;

        return 0;
    }

    state = GET_LG_STATE(lgp->flags);

    // if backfresh - send the delta blocks back to the primary
    // if refresh - send the delta map back to the primary
    if (state == FTD_SBACKFRESH) {

        // send an ack containing clean len  
        memset(&ack, 0, sizeof(ack));
        ack.msgtype = FTDACKRSYNC;
        ack.msg.lg.devid = rdevp.devid;
        ack.msg.lg.len = ((rdevp.sumlen - rdevp.dirtylen) >> DEV_BSHIFT);

        if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp),"ftd_sock_recv_rsync_chksum",fsockp, &ack)) < 0) {
            ret = rc;
            goto errret;
        }
        
        devp->statp->actual += sizeof(ftd_header_t);
        devp->statp->effective += sizeof(ftd_header_t);

        if ((rc = ftd_rsync_flush_delta(fsockp, lgp, devp)) < 0) {
            ret = rc;
            goto errret;
        }
    } else {

        // refresh
        
        memset(&ack, 0, sizeof(ack));
        ack.msgtype = FTDACKCHKSUM;
        ack.msg.lg.devid = rdevp.devid;
        ack.msg.lg.len = (rdevp.sumlen >> DEV_BSHIFT);
        ack.len = deltamap_len;

        if ((rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_recv_rsync_chksum",fsockp, &ack)) < 0) {
            ret = rc;
            goto errret;
        }

        devp->statp->actual += sizeof(ftd_header_t);
        devp->statp->effective += sizeof(ftd_header_t);

        // build and send delta buffer
        if (deltamap_len > lgp->buflen) {
            lgp->buflen = deltamap_len; 
            lgp->buf = (char*)realloc((char*)lgp->buf, deltamap_len);
        }
        i = 0;
        ForEachLLElement(devp->deltamap, dp) {
            memcpy(lgp->buf+(i*sizeof(ftd_dev_delta_t)),
                (char*)dp, sizeof(ftd_dev_delta_t));
            i++;
        }
        iov[0].iov_base = (char*)devp;
        iov[0].iov_len = sizeof(ftd_dev_t);
        iov[1].iov_base = (char*)lgp->buf;
        iov[1].iov_len = deltamap_len;

        if ((rc = ftd_sock_send_lg_vector(fsockp, lgp, iov, 2)) < 0) {
            ret = rc;
            error_tracef(TRACEERR,"ftd_sock_recv_rsync_chksum(): ftd_sock_send_lg_vector() error rc=%d : tid=%x", rc, GetCurrentThreadId());
            goto errret;
        }
        
        devp->statp->actual += sizeof(ftd_dev_t) + deltamap_len;
        devp->statp->effective += sizeof(ftd_dev_t) + deltamap_len;
    }

    if (devp->deltamap) {
        EmptyLList(devp->deltamap);
    }

    return 0;

errret:

    if (devp->deltamap) {
        EmptyLList(devp->deltamap);
    }

    return ret;
}

/*
 * ftd_sock_recv_bab_chunk --
 * process a BAB chunk from peer
 */
int
ftd_sock_recv_bab_chunk(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    int             rc;

    // re-allocate i/o buffer if necessary 
    if (header->uncomplen > lgp->buflen) {
        lgp->buflen = header->uncomplen;
        lgp->buf = (char*)realloc((char*)lgp->buf, lgp->buflen);
    }
    
    if (header->msg.lg.data == FTDZERO) {
        error_tracef(TRACEINF4,"ftd_sock_recv_bab_chunk(): block all zero: offset: %d, length: %d", header->msg.lg.offset, header->msg.lg.len);
        memset(lgp->buf, 0, lgp->buflen);
    } else {
        lgp->datalen = header->msg.lg.len;
        
        // read chunk data from network 
        if (header->compress) {
            // make sure the compress buf is large enough 
            if (lgp->datalen > lgp->cbuflen) {
                lgp->cbuflen = lgp->datalen;
                lgp->cbuf = (char*)realloc(lgp->cbuf, lgp->cbuflen);
            }
            rc = ftd_sock_recv(fsockp, lgp->cbuf, lgp->datalen);
        } else {
            rc = ftd_sock_recv(fsockp, lgp->buf, lgp->datalen);
        }

        //DPRINTF((ERRFAC,LOG_INFO,         " ftd_sock_recv_chunk: lgp->datalen, recv rc = %d, %d\n",           lgp->datalen, rc));
        if (rc < 0) {
            error_tracef(TRACEINF4,"ftd_sock_recv_bab_chunk(): lgp->datalen = %d, recv rc = %d , tid=%x", lgp->datalen, rc, GetCurrentThreadId());
            return rc;
        }
    
        if (header->compress) {
            lgp->compp->algorithm = header->compress;
    
            // decompress the data 
            error_tracef(TRACEINF5,"ftd_sock_recv_bab_chunk(): compr len = %d", header->msg.lg.len);
            error_tracef(TRACEINF5,"ftd_sock_recv_bab_chunk(): uncompr len = %d", header->uncomplen);
            
            header->len = comp_decompress((BYTE*)lgp->buf,
                (size_t*)&lgp->cbuflen,
                (BYTE*)lgp->cbuf,
                (size_t*)&lgp->datalen,
                lgp->compp);
            
            error_tracef(TRACEINF5,"ftd_sock_recv_bab_chunk(): de-compr len = %d, %d", lgp->cbuflen, header->len);

            if (header->len <= 0) {
                // report this - reporterr(ERRFAC, M_COMPRESS, ERRCRIT);
                rc = -1;
                goto errexit;
            }
            // use compress length
            lgp->datalen = header->len;
        
            if (header->uncomplen) {
                lgp->cratio =
                    (float)((float)header->msg.lg.len / (float)header->uncomplen);
            } else {
                lgp->cratio = 1;
            }
        }
    }
   
    // process chunk entries 
    lgp->bufoff = -1;

    return 0;

errexit:

    return rc;
}

/*
 *
 * ftd_sock_store_sector_zero() --
 *      stores sector zero context for later write - we commit it at the end of the full refresh, 
 *          or at a checkpoint. 
 */ 
int 
ftd_sock_store_sector_zero(ftd_sz_context *SectorZero, ftd_dev_t *devp, 
                                ftd_dev_cfg_t *devcfgp, char *buffer, ftd_uint64_t len) 
{ 
    ftd_sz_context *ThisSectorZero = NULL; 

    error_tracef(TRACEINF5,"ftd_sock_store_sector_zero()");

    // Walk the list looking for the sector zero corresponding to our device 
    ThisSectorZero = SectorZero; 
    while(ThisSectorZero != NULL) 
    {
        printf("comparing %s to %s\n", devcfgp->sdevname, ThisSectorZero->devcfgp.sdevname); 
        if(!memcmp(devcfgp->sdevname, ThisSectorZero->devcfgp.sdevname, sizeof(devcfgp->sdevname)))
        {
            printf("Found sector zero %s\n", devcfgp->sdevname); 
            break; 
        }
        ThisSectorZero = ThisSectorZero->next; 
    } 

    if(ThisSectorZero == NULL)
    { 
        printf("store_sector_zero() :: Device Comparison failed!!!\n"); 
        return 0; 
    } 

    // Copy the data
    ThisSectorZero->SectorZeroBuffer = LocalAlloc(LPTR, (long)len); 
    
    if(ThisSectorZero->SectorZeroBuffer == NULL)
    { 
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, "sector zero buffer");
        return 1; 
    } 
    
    CopyMemory(&(ThisSectorZero->devp), devp, sizeof(ftd_dev_t)); 

    // Before we save off the sector zero, we zero it on the disk
    ZeroMemory(ThisSectorZero->SectorZeroBuffer, (size_t)len); 
    if(ftd_dev_write(devp, (char*)ThisSectorZero->SectorZeroBuffer, 0, (size_t)len, devcfgp->sdevname) < 0) 
    { 
        // we'll ignore it for now.. 
        reporterr(  ERRFAC, 
                    M_WRITEERR, 
                    ERRCRIT, 
                    "clearing sector zero", // filename/problem
                    devcfgp->devid,         // device id
                    0,                      // offset
                    len,                    // length
                    ftd_strerror());        // error string
    } 
    
    // Now copy actual sector zero data into temp buffer
    CopyMemory(ThisSectorZero->SectorZeroBuffer, buffer, (size_t)len); 
    ThisSectorZero->SectorZeroLength = len; 

    // We're done -
    return 0; 

} // end ftd_sock_store_sector_zero() 

/* 
 *
 * ftd_sock_replace_sector_zero() --
 *      restores sector zero from context at the end of the full refresh, 
 *          or at a checkpoint. Allows the volume to be mounted. 
 */ 
int 
ftd_sock_replace_sector_zero(ftd_sz_context *SectorZero) 
{ 
    ftd_sz_context *ThisSectorZero = NULL; 

    error_tracef(TRACEINF5,"ftd_sock_replace_sector_zero()");

    // Walk the list looking for the sector zero corresponding to our device 
    ThisSectorZero = SectorZero; 

    while(ThisSectorZero)
    { 
        printf("Restoring sector zero on drive %s\n", ThisSectorZero->devcfgp.sdevname ); 

        if(ftd_dev_write((ftd_dev_t *)&(ThisSectorZero->devp), 
                        (char*)ThisSectorZero->SectorZeroBuffer, 
                        0, 
                        (size_t)ThisSectorZero->SectorZeroLength, 
                        SectorZero->devcfgp.sdevname)           < 0) 
        { 
            reporterr(  ERRFAC, 
                        M_WRITEERR, 
                        ERRCRIT, 
                        "sector zero",                      // filename/problem
                        SectorZero->devcfgp.devid,          // device id
                        0,                                  // offset
                        ThisSectorZero->SectorZeroLength,   // length
                        ftd_strerror());                    // error string
            return 1; 
        } 
    
        LocalFree(ThisSectorZero->SectorZeroBuffer); 

        ThisSectorZero = ThisSectorZero->next; 
    } 

    return 0; 

} // end ftd_sock_replace_sector_zero()

/*
 * ftd_sock_recv_refresh_block --
 * process a refresh block message
 */
int
ftd_sock_recv_refresh_block(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    ftd_dev_t       *devp;
    ftd_dev_cfg_t   *devcfgp;
    ftd_header_t    ack;
    ftd_uint64_t    offset;
    int             rc, len = 0;
    time_t          currentts;
    char            *errorstr;

    // get device structure from lg
    devp = ftd_lg_devid_to_dev(lgp, header->msg.lg.devid);
    if (devp == NULL) { 
        reporterr(ERRFAC, M_PROTDEV, ERRCRIT, header->msg.lg.devid);
        return -1;
    }

    if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, header->msg.lg.devid)) == NULL) {
        reporterr(ERRFAC, M_PROTDEV, ERRCRIT, header->msg.lg.devid);
        return -1;
    }
    
    // re-allocate buffer if necessary
    if (header->len > lgp->buflen) {
        lgp->buflen = header->len;
        lgp->buf = (char*)realloc(lgp->buf, lgp->buflen);
    }
        
    if (header->msg.lg.data == FTDZERO) {
        error_tracef(TRACEINF5,"ftd_sock_recv_refresh_block(): block all zero: offset: %d, length: %d",header->msg.lg.offset, header->msg.lg.len);
        memset(lgp->buf, 0, lgp->buflen);
        header->msg.lg.data = (u_long)lgp->buf;
    } else {
        // read chunk data from network 
        rc = ftd_sock_recv(fsockp, lgp->buf, header->len);
        if (rc < 0) {
            return rc;
        }
        
        header->msg.lg.data = (u_long)lgp->buf;
        
        if (header->compress) {
            lgp->compp->algorithm = header->compress;
    
            // decompress the data 
            len = header->len > header->uncomplen ?
                header->len: header->uncomplen;
        
            if (len > lgp->cbuflen) {
                lgp->cbuflen = len;
                lgp->cbuf = (char*)realloc(lgp->cbuf, lgp->cbuflen);
            }
            error_tracef(TRACEINF5,"ftd_sock_recv_refresh_block(): compr len = %d", header->len);
            error_tracef(TRACEINF5,"ftd_sock_recv_refresh_block(): uncompr len = %d", header->uncomplen);
            
            header->len = comp_decompress((BYTE*)lgp->cbuf,
                (size_t*)&lgp->cbuflen,
                (BYTE*)header->msg.lg.data,
                (size_t*)&header->len,
                lgp->compp);

            error_tracef(TRACEINF5,"ftd_sock_recv_refresh_block(): de-compr len = %d", header->len);
            
            if (header->len <= 0) {
                return -1;
            }
            header->msg.lg.data = (u_long)lgp->cbuf;

            if (header->uncomplen) {
                lgp->cratio =
                    (float)((float)header->len / (float)header->uncomplen);
            } else {
                lgp->cratio = 1;
            }
            header->msg.lg.data = (u_long)lgp->cbuf;
        }
        
        devp->statp->actual += len;
        devp->statp->effective += header->len;
    } 

    if ( GET_JRN_MODE(lgp->jrnp->flags) == FTDMIRONLY)
        {
        // write block directly to mirror 
        offset = header->msg.lg.offset;
        offset <<= DEV_BSHIFT;

        len = (header->msg.lg.len << DEV_BSHIFT);
    
        // If this is block zero, we store it off someplace until the full refresh has completed
        if ((offset == 0) && GET_LG_SZ_INVALID(lgp->flags))
            { 
            ftd_sock_store_sector_zero(lgp->SectorZero, devp, devcfgp, (char *)header->msg.lg.data, len); 
            }
        else
            {
            if (ftd_dev_write(devp, (char*)header->msg.lg.data, offset, len, devcfgp->sdevname) < 0) 
                {
			return FTD_LG_SEC_DRIVE_ERR; //WR 17511
            //return -1;
        }
            }
        } 
    else {

        // write block to journal 
        header->msg.lg.devid = devp->devid;

        rc = ftd_journal_file_write(lgp->jrnp,
            header->msg.lg.lgnum,
            header->msg.lg.devid,
            header->msg.lg.offset,
            header->msg.lg.len,
            (char*)header->msg.lg.data);

        if (rc < 0) {
            errorstr = ftd_strerror();

            if (rc == -2) {
                reporterr(ERRFAC, M_JRNSPACE, ERRCRIT, lgp->jrnp->cur->name);
            } else if (rc == -3) {
                {
                    char *filename;

                    if (lgp->jrnp == NULL || lgp->jrnp->cur == NULL) {
                        filename = "No journal file";
                        offset = 0;
                    } else {
                        filename = lgp->jrnp->cur->name;
                        offset = lgp->jrnp->cur->offset;
                    }
                    reporterr(ERRFAC, M_WRITEERR, ERRCRIT,
                        filename,
                        0,
                        offset,
                        (header->msg.lg.len << DEV_BSHIFT),
                        errorstr);
                }
            }
            //return 0;
            return rc;
        }
    }
   
    // bump device i/o byte counts
    if (lgp->tunables->compression) {
        devp->statp->actual +=
            (int)(lgp->cratio * (sizeof(ftd_header_t) + header->len));
    } else {
        devp->statp->actual += (int)(sizeof(ftd_header_t) + header->len);
    }
    
    devp->statp->effective += (int)(sizeof(ftd_header_t) + header->len);
    devp->statp->entries++;
    time(&currentts);
    devp->statp->entage =
        (currentts - header->ts) - lgp->dsockp->sockp->tsbias;

    memset(&ack, 0, sizeof(ack));
    ack.msgtype = FTDACKRSYNC;
    ack.msg.lg.devid = header->msg.lg.devid;
    ack.msg.lg.len = header->msg.lg.len;    

    if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_sock_recv_refresh_block",fsockp, &ack)) < 0) {
        return rc;
    }

    return 0;
}

/*
 * ftd_sock_recv_rsync_start --
 * process a rsync start message
 */
int
ftd_sock_recv_rsync_start(ftd_header_t *header, ftd_lg_t *lgp) 
{

	switch(header->msgtype) 
	{
		case FTDCRFSTART:
			return FTD_CREFRESH;

		case FTDCRFFSTART:
			SET_LG_SZ_INVALID(lgp->flags);		// Invalid block zero.. we save it later 
			return FTD_CREFRESHF;

		case FTDCBFSTART:
            if (GET_LG_BACK_FORCE(header->msg.lg.flags)) 
            {
				SET_LG_BACK_FORCE(lgp->flags);
			}

	        return FTD_CBACKFRESH;

		default:
			break;

   }

    return 0;

} // end ftd_sock_recv_rsync_start()

/*
 * ftd_sock_recv_rsync_end --
 * process a rsync end message
 */
int
ftd_sock_recv_rsync_end(int msgtype, ftd_lg_t *lgp) 
{
    // At the end of a full resync, we restore sector zero. It has been hiding so that if the 
    //  resync failed, the target user couldn't mount it. Everything's cool, so we restore it here. 
    if(GET_LG_SZ_INVALID(lgp->flags))
    { 
        ftd_sock_replace_sector_zero(lgp->SectorZero);
        lgp->SectorZero = NULL;
        UNSET_LG_SZ_INVALID(lgp->flags); 
    } 

    return FTD_CNORMAL;

} // end ftd_sock_recv_rsync_end() 

/*
 * ftd_sock_recv_rsync_devs --
 * process a rsync devs message
 */
int
ftd_sock_recv_rsync_devs(ftd_sock_t *fsockp,
    ftd_header_t *header, ftd_lg_t *lgp) 
{
    ftd_header_t    ack;
    ftd_dev_t       *devp;
    ftd_dev_cfg_t   *devcfgp;
    ftd_uint64_t    offset;

    // get device structure from lg
    devp = ftd_lg_devid_to_dev(lgp, header->msg.lg.devid);
    if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, header->msg.lg.devid)) == NULL) {
        reporterr(ERRFAC, M_PROTDEV, ERRCRIT, header->msg.lg.devid);
        return -1;
    }

    if (devp == NULL) { 
        reporterr(ERRFAC, M_PROTDEV, ERRCRIT,
            header->msg.lg.devid);
        return -1;
    }

    /* Now, seek to the appropriate offset */
    error_tracef( TRACEINF,"*** Refresh device: %s start sector offset = %d", devcfgp->sdevname, header->msg.lg.offset );

    offset = header->msg.lg.offset;
    offset <<= offset;

    if (ftd_llseek(devp->devfd, offset, SEEK_SET) == (ftd_uint64_t)-1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            devcfgp->sdevname,
            offset,
            ftd_strerror());
        return -1;
    }

    memset(&ack, 0, sizeof(ack));
    ack.msgtype = FTDACKNOOP;
    ack.msg.lg.devid = header->msg.lg.devid;

    if (FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp),"ftd_sock_recv_rsync_devs",fsockp, &ack) < 0) {
        return -1;
    }

    return 0;
}

/*
 * ftd_sock_recv_rsync_deve --
 * process a rsync deve message
 */
int
ftd_sock_recv_rsync_deve(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    ftd_header_t    ack;
    ftd_dev_t       *devp;
    int             rc;

    switch(header->msgtype) {
    case FTD_SREFRESH:
        // get device structure from lg
        devp = ftd_lg_devid_to_dev(lgp, header->msg.lg.devid);
    
        if (devp == NULL) { 
            reporterr(ERRFAC, M_PROTDEV, ERRCRIT,
                header->msg.lg.devid);
            return -1;
        }

        if (devp->devfd != INVALID_HANDLE_VALUE)
        {
#if defined(_WINDOWS)
            ftd_dev_unlock(devp->devfd);
#else
                FTD_CLOSE_FUNC(__FILE__,__LINE__,devp->devfd);
#endif
            devp->devfd = INVALID_HANDLE_VALUE;
        }

        memset(&ack, 0, sizeof(ack));
        ack.msgtype = FTDACKNOOP;
        ack.msg.lg.devid = header->msg.lg.devid;

        if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp),"ftd_sock_recv_rsync_deve",fsockp, &ack)) < 0) {
            return rc;
        }
        break;
    default:
        break;
    }

    return 0;
}


/*
 * ftd_sock_recv_cpstart --
 * handle a checkpoint start packet from peer
 */
int
ftd_sock_recv_cpstart(ftd_lg_t *lgp, ftd_header_t *header) 
{
    ftd_header_t    ack;

    if (GET_LG_CPSTART(lgp->flags)) {
        if (FTD_SOCK_CONNECT(lgp->dsockp)) {
            reporterr(ERRFAC, M_CPSTARTAGAIN, ERRWARN, LG_PROCNAME(lgp));
            ftd_sock_send_err(lgp->dsockp, ftd_get_last_error(ERRFAC));
        }

        // tell master
        memset(&ack, 0, sizeof(ack));
        ack.msgtype = FTDACKCPERR;
        ack.msg.lg.lgnum = lgp->lgnum;
        ack.msg.lg.data = lgp->cfgp->role;

        FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_sock_recv_cpstart",lgp->isockp, &ack);
        error_tracef(TRACEINF,"ftd_sock_recv_cpstart(): 0 , tid=%x", GetCurrentThreadId());
        return 0;
    }

    switch(header->msgtype) {
    case FTDCCPSTARTP:
        SET_LG_CPSTART(lgp->flags, LG_CPSTARTP);
        break;
    case FTDCCPSTARTS:
    case FTDACKCPSTART:
        SET_LG_CPSTART(lgp->flags, LG_CPSTARTS);
        break;
    default:
        break;
    }

    error_tracef(TRACEINF,"ftd_sock_recv_cpstart(): FTD_CCPSTART , tid=%x", GetCurrentThreadId());
    return FTD_CCPSTART;
}

/*
 * ftd_sock_recv_cpstop --
 * handle a checkpoint stop packet from peer
 */
int
ftd_sock_recv_cpstop(ftd_lg_t *lgp, ftd_header_t *header) 
{
    ftd_header_t    ack;

    if (GET_LG_CPSTOP(lgp->flags)) {
        if (FTD_SOCK_CONNECT(lgp->dsockp)) {
            reporterr(ERRFAC, M_CPSTOPAGAIN, ERRWARN, LG_PROCNAME(lgp));
            ftd_sock_send_err(lgp->dsockp, ftd_get_last_error(ERRFAC));
        }

        // tell master
        memset(&ack, 0, sizeof(ack));
        ack.msgtype = FTDACKCPERR;
        ack.msg.lg.lgnum = lgp->lgnum;
        ack.msg.lg.data = lgp->cfgp->role;

        FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_sock_recv_cpstop",lgp->isockp, &ack);
        error_tracef(TRACEINF,"ftd_sock_recv_cpstop(): 0 , tid=%x", GetCurrentThreadId());
        return 0;
    }

    switch(header->msgtype) {
    case FTDCCPSTOPP:
        SET_LG_CPSTOP(lgp->flags, LG_CPSTOPP);
        break;
    case FTDCCPSTOPS:
    case FTDACKCPSTOP:
        SET_LG_CPSTOP(lgp->flags, LG_CPSTOPS);
        break;
    default:
        break;
    }

    error_tracef( TRACEINF,"ftd_sock_recv_cpstop(): FTD_CCPSTOP , tid=%x", GetCurrentThreadId());
    return FTD_CCPSTOP;
}

/*
 * ftd_sock_recv_cpon --
 * handle a checkpoint on packet from peer
 */
int
ftd_sock_recv_cpon(void) 
{

    return FTD_CCPON;
}

/*
 * ftd_sock_recv_cpoff --
 * handle a checkpoint off packet from peer
 */
int
ftd_sock_recv_cpoff(void) 
{

    return FTD_CCPOFF;
}

/*
 * ftd_sock_recv_cponerr --
 * handle a checkpoint error packet from peer
 */
int
ftd_sock_recv_cponerr(ftd_sock_t *fsockp, ftd_lg_t *lgp) 
{

    UNSET_LG_CPON(lgp->flags);
    UNSET_LG_CPSTART(lgp->flags);

    reporterr(ERRFAC, M_CPONERR, ERRWARN, LG_PROCNAME(lgp));
    error_tracef(TRACEERR,"ftd_sock_recv_cponerr(): %s , tid=%x", LG_PROCNAME(lgp), GetCurrentThreadId());

    return 0;
}

/*
 * ftd_sock_recv_cpofferr --
 * handle a checkpoint error packet from peer
 */
int
ftd_sock_recv_cpofferr(ftd_sock_t *fsockp, ftd_lg_t *lgp) 
{

    UNSET_LG_CPSTOP(lgp->flags);
    reporterr(ERRFAC, M_CPOFFERR, ERRWARN, LG_PROCNAME(lgp));

    error_tracef(TRACEERR,"ftd_sock_recv_cpofferr(): %s , tid=%x", LG_PROCNAME(lgp), GetCurrentThreadId());
    return 0;
}

/*
 * ftd_sock_recv_exit --
 * process a exit message
 */
int
ftd_sock_recv_exit(ftd_sock_t *fsockp, ftd_header_t *header) 
{

    return FTD_CINVALID;
}

/*
 * ftd_sock_recv_noop --
 * process a noop message
 */
int
ftd_sock_recv_noop(ftd_sock_t *fsockp, ftd_header_t *header) 
{
    int rc;

    if (!header->ackwanted) {
        return 0;
    }

    header->ackwanted = 0;
    if ((rc = FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_sock_recv_noop",fsockp, header)) < 0) {
        return rc;
    }

    return 0;
}

/*
 * ftd_sock_recv_ack_rfstart --
 * process a refresh start ack 
 */
int
ftd_sock_recv_ack_rfstart(ftd_lg_t *lgp) 
{

    //UNSET_LG_RFSTART_ACKPEND(lgp->flags);

    return 0;
}

/*
 * ftd_sock_recv_ack_rsync --
 * process a rsync ack 
 */
int
ftd_sock_recv_ack_rsync(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    ftd_dev_t   *devp;

    if (GET_LG_STATE(lgp->flags) == FTD_SNORMAL) {
        return 0;
    }

    // get device structure from lg
    devp = ftd_lg_devid_to_dev(lgp, header->msg.lg.devid);
    
    if (devp == NULL) { 
        reporterr(ERRFAC, M_PROTDEV, ERRCRIT,
            header->msg.lg.devid);
        return -1;
    }
    // device ack 
    devp->rsyncackoff += header->msg.lg.len;    /* blocks */

    return 0;
}

/*
 * ftd_sock_recv_ack_chunk --
 * process a BAB chunk ack 
 */
int
ftd_sock_recv_ack_chunk(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    int rc = 0;

    //error_tracef(TRACEINF5,"ftd_sock_recv_ack_chunk(): %d bytes ack'd", header->msg.lg.len);

    if (ftd_ioctl_migrate(lgp->devfd, lgp->lgnum,
        header->msg.lg.len) < 0) {
        error_tracef(TRACEERR,"ftd_sock_recv_ack_chunk(): ftd_ioctl_migrate error tid=%x", GetCurrentThreadId());
        return -1;
    }

    // adjust BAB "get" offset by migrated length
    lgp->offset -= (header->msg.lg.len / sizeof(int));

    if (GET_LG_RFDONE(header->msg.lg.flags))
    {
        SET_LG_RFDONE(lgp->flags);
    }

    if (GET_LG_RFSTART(header->msg.lg.flags)) {
        UNSET_LG_RFSTART_ACKPEND(lgp->flags);
    }

    if (GET_LG_CPOFF_JLESS_ACKPEND(lgp->flags))
        {
        UNSET_LG_CPOFF_JLESS_ACKPEND(lgp->flags);
        rc = FTD_CCPOFF;
        }
    return rc;
}

/*
 * ftd_sock_recv_ack_hup --
 * process a hup ack 
 */
int
ftd_sock_recv_ack_hup(ftd_sock_t *fsockp, ftd_header_t *header) 
{

    SET_SOCK_HUP(fsockp->sockp->flags);

    return 0;
}

/*
 * ftd_sock_recv_backfresh_block --
 * process a backfresh block from secondary
 */
int
ftd_sock_recv_backfresh_block(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    ftd_dev_t       *devp, ldevp;
    ftd_dev_cfg_t   *devcfgp;
    int             len, rc;

    // get device structures from lg
    devp = ftd_lg_devid_to_dev(lgp, header->msg.lg.devid);

    if (devp == NULL) { 
        reporterr(ERRFAC, M_PROTDEV, ERRCRIT, ERRCRIT,
            header->msg.lg.devid);
        return -1;
    }

    if ((devcfgp = ftd_lg_devid_to_devcfg(lgp, header->msg.lg.devid)) == NULL) {
        reporterr(ERRFAC, M_PROTDEV, ERRCRIT, header->msg.lg.devid);
        return -1;
    }

    error_tracef(TRACEINF5,"ftd_sock_recv_backfresh_block(): fd, header->msg.lg.devid, header->msg.lg.offset, header->msg.lg.len, header->len = %d, %d, %d, %d, %d",
        devp->devfd,
        header->msg.lg.devid,
        header->msg.lg.offset,
        header->msg.lg.len,
        header->len);

    // bump counters    
    devp->rsyncackoff += header->msg.lg.len;
    devp->rsyncdelta += header->msg.lg.len;

    if (header->len) {
        // re-allocate buffer if necessary
        if (header->len > lgp->recvbuflen) {
            lgp->recvbuflen = header->len;
            lgp->recvbuf = (char*)realloc(lgp->recvbuf, lgp->recvbuflen);
        }
        
        if (header->msg.lg.data == FTDZERO) {
            error_tracef( TRACEINF5,
            "ftd_sock_recv_backfresh_block(): zero block: offset: %d, length: %d\n",
            header->msg.lg.offset, header->msg.lg.len);
            memset(lgp->recvbuf, 0, lgp->recvbuflen);
            header->msg.lg.data = (u_long)lgp->recvbuf;
        } else {
            // read data block from network 
            rc = ftd_sock_recv(fsockp, lgp->recvbuf, header->len);
            if (rc < 0) {
                return rc;
            }
            
            header->msg.lg.data = (u_long)lgp->recvbuf;

            if (header->compress) {
                lgp->compp->algorithm = header->compress;
    
                // decompress the data 
                len = header->len > header->uncomplen ?
                    header->len: header->uncomplen;

                if (len > lgp->cbuflen) {
                    lgp->cbuflen = len;
                    lgp->cbuf =
                        (char*)realloc(lgp->cbuf, lgp->cbuflen);
                }
                error_tracef( TRACEINF5,"ftd_sock_recv_backfresh_block(): compr len = %d",  header->len);
                error_tracef( TRACEINF5,"ftd_sock_recv_backfresh_block(): uncompr len = %d", header->uncomplen);
                
                header->len = comp_decompress((BYTE*)lgp->cbuf,
                    (size_t*)&lgp->cbuflen,
                    (BYTE*)header->msg.lg.data,
                    (size_t*)&header->len,
                    lgp->compp);
                
                error_tracef( TRACEINF5,"ftd_sock_recv_backfresh_block(): de-compr len = %d", header->len);
                
                if (header->len <= 0) {
                    return -1;
                }
                header->msg.lg.data = (u_long)lgp->cbuf;

                if (header->uncomplen) {
                    lgp->cratio =
                        (float)((float)header->len / (float)header->uncomplen);
                } else {
                    lgp->cratio = 1;
                }
            }
            
        }

        // bump network data counts 
        devp->statp->actual += header->len;
        devp->statp->effective += header->uncomplen;

        ldevp.devid = header->msg.lg.devid;
        ldevp.rsyncoff = header->msg.lg.offset;
        ldevp.rsynclen = header->msg.lg.len;
        ldevp.rsyncbytelen = ldevp.rsynclen << DEV_BSHIFT;
        ldevp.rsyncbuf = (char*)header->msg.lg.data;

        ldevp.devfd = devp->devfd;
        ldevp.no0write = devp->no0write;

        {
            ftd_uint64_t    off64 = ldevp.rsyncoff;
                            off64 <<= DEV_BSHIFT;

            if (ftd_dev_write(&ldevp, ldevp.rsyncbuf, off64, ldevp.rsyncbytelen,
                devcfgp->pdevname) < 0) {
                return -1;
            }
        }
    }

    return 0;
}

/*
 * ftd_sock_recv_ack_chksum --
 * handle a chksum ack packet from peer
 */
int
ftd_sock_recv_ack_chksum(ftd_sock_t *fsockp, ftd_header_t *header,
    ftd_lg_t *lgp) 
{
    ftd_dev_t       *devp, ldevp;
    ftd_dev_delta_t *dp;
    int             len, rc, i, cnt;

    // get device structure from lg device list
    if ((devp = ftd_lg_devid_to_dev(lgp, header->msg.lg.devid)) == NULL) {  
        reporterr(ERRFAC, M_PROTDEV, ERRWARN, ERRCRIT,
            header->msg.lg.devid);
        return -1;
    }

    // read device info from network/secondary
    if ((rc = ftd_sock_recv(fsockp, (char*)&ldevp, sizeof(ldevp))) < 0) {
        return rc;
    }  

    if (header->len > lgp->recvbuflen) {
        lgp->recvbuflen = header->len;
        lgp->recvbuf = (char*)realloc(lgp->recvbuf, lgp->recvbuflen);
    }

    // read remote delta map into local buffer
    if ((rc = ftd_sock_recv(fsockp, lgp->recvbuf, header->len)) < 0) {
        return rc;
    }  
    
    // if we are now not in refresh mode due to override
    // then just drop the message

    if (GET_LG_STATE(lgp->flags) != FTD_SREFRESH
        && GET_LG_STATE(lgp->flags) != FTD_SREFRESHC)
    {
        return 0;
    }

    // add deltas to device delta list 
    cnt = header->len / sizeof(ftd_dev_delta_t);
    len = 0;
    for (i = 0; i < cnt; i++) {
        dp = (ftd_dev_delta_t*)(lgp->recvbuf+(i*sizeof(ftd_dev_delta_t)));
        AddToTailLL(devp->deltamap, dp);
        len += dp->length;
    }

    // account for clean len
    devp->rsyncackoff += (header->msg.lg.len - len);

    if (GET_SOCK_WRITABLE(FTD_SOCK_FLAGS(fsockp))) {
        // flush deltas to secondary
        // ONLY if connection is currently writable
        if (ftd_rsync_flush_delta(fsockp, lgp, devp) == -1) {
            return -1;
        }
    } else {
        // connection is blocked - we got here because we 
        // couldn't complete a send operation. if we try to
        // send again here all hell will break loose.
    }

    return 0;
}

/*
 * ftd_sock_recv_ack_kill --
 * handle a KILL packet from peer
 */
int
ftd_sock_recv_ack_kill(void) 
{
    //return FTD_CINVALID; make it negative
    return -1;
}

/*
 * ftd_sock_recv_ack_kill --
 * handle a REMDRIVEERR packet from peer, should be from RemoteThread
 */
int
ftd_sock_recv_drive_err(ftd_lg_t *lgp, ftd_header_t *header) 
{
	ftd_header_t    ack;
	
	if ( ( header->msg.lg.data == ~ROLESECONDARY ) && ( header->msg.lg.flags == ROLESECONDARY ) )
	{
		// RMD must exit with FTD_LG_SEC_DRIVE_ERR condition
		return FTD_LG_SEC_DRIVE_ERR;
	}

	// PMD must report it to Primary Master 
	// tell master
	memset(&ack, 0, sizeof(ack));
	ack.msgtype = FTDCREMDRIVEERR;
	ack.msg.lg.lgnum = lgp->lgnum;
	ack.msg.lg.data = lgp->cfgp->role;

	FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_sock_recv_drive_err",lgp->isockp, &ack);
	error_tracef(TRACEINF,"ftd_sock_drive_err() tid=%x", GetCurrentThreadId());
    
    //return FTD_CINVALID; make it negative
    return -1;
}

/*
 * ftd_sock_recv_ack_handshake --
 * process a handshake ack 
 */
int
ftd_sock_recv_ack_handshake(ftd_lg_t *lgp, ftd_header_t *header) 
{

    if (header->msg.lg.data) {
        SET_LG_CPON(lgp->flags);
    }

    return 0;
}

/*
 * ftd_sock_recv_stop_apply --
 * handle a STOPAPPLY packet from peer
 */
int
ftd_sock_recv_stop_apply(ftd_sock_t *fsockp, int lgnum) 
{
    char    procname[MAXPATH];

    sprintf(procname, "RMDA_%03d", lgnum);

    ftd_proc_kill(procname);

    return 0;
}

/*
 * ftd_sock_recv_lg_msg --
 * read a ftd message header and dispatch the message accordingly
 */
int
ftd_sock_recv_lg_msg(ftd_sock_t *fsockp, ftd_lg_t *lgp, int timeo) 
{
    ftd_header_t    header;
    int             rc;

    memset(&header, 0, sizeof(ftd_header_t));

    if (ftd_sock_check_recv(fsockp, timeo) == 0) {
        // nothing to read
        return FTD_LG_NET_NOT_READABLE;
    }

    if ((rc = FTD_SOCK_RECV_HEADER(LG_PROCNAME(lgp),"ftd_sock_recv_lg_msg",fsockp, &header)) < 0) {
        return rc;
    }

    rc = 0; 

    switch(header.msgtype) {
    case FTDCNOOP:
        rc = ftd_sock_recv_noop(fsockp, &header);
        break;
    case FTDCHANDSHAKE:
        rc = ftd_sock_recv_handshake(fsockp, &header, lgp);
        break;
    case FTDCCHKCONFIG:
        rc = ftd_sock_recv_chkconfig(fsockp, &header, lgp);
        break;
    case FTDCHUP:
        rc = ftd_sock_recv_hup(fsockp, &header);
        break;
    case FTDCCHKSUM:
        rc = ftd_sock_recv_rsync_chksum(fsockp, &header, lgp);
        break;
    case FTDCCHUNK:
        rc = ftd_sock_recv_bab_chunk(fsockp, &header, lgp);
        break;
    case FTDCRFBLK:
        rc = ftd_sock_recv_refresh_block(fsockp, &header, lgp);
        break;
    case FTDCBFBLK:
        rc = ftd_sock_recv_backfresh_block(fsockp, &header, lgp);
        break;
    case FTDCRFSTART:
    case FTDCRFFSTART:
    case FTDCBFSTART:
        rc = ftd_sock_recv_rsync_start(&header, lgp);
        break;
    case FTDCRFEND:
    case FTDCRFFEND:
    case FTDCBFEND:
        rc = ftd_sock_recv_rsync_end(header.msgtype, lgp);
        break;
    case FTDCRSYNCDEVS:
        rc = ftd_sock_recv_rsync_devs(fsockp, &header, lgp);
        break;
    case FTDCRSYNCDEVE:
        rc = ftd_sock_recv_rsync_deve(fsockp, &header, lgp);
        break;
    case FTDCVERSION:
        rc = ftd_sock_recv_version(fsockp, &header, lgp);
        break;
    case FTDCREFOFLOW:
        rc = ftd_sock_recv_refoflow(lgp);
        break;
    case FTDCEXIT:
        rc = ftd_sock_recv_exit(fsockp, &header);
        break;
    case FTDACKERR:
        rc = ftd_sock_recv_err(fsockp, &header);
        break;
    case FTDACKHANDSHAKE:
        rc = ftd_sock_recv_ack_handshake(lgp, &header);
        break;
    case FTDACKCONFIG:
        break;
    case FTDACKRSYNC:
        rc = ftd_sock_recv_ack_rsync(fsockp, &header, lgp);
        break;
    case FTDACKHUP:
        rc = ftd_sock_recv_ack_hup(fsockp, &header);
        break;
    case FTDACKCHUNK:
        rc = ftd_sock_recv_ack_chunk(fsockp, &header, lgp);
        break;
    case FTDACKKILL:
        rc = ftd_sock_recv_ack_kill();
        break;
    case FTDACKCHKSUM:
        rc = ftd_sock_recv_ack_chksum(fsockp, &header, lgp);
        break;
    case FTDCCPSTARTP:
    case FTDCCPSTARTS:
    case FTDACKCPSTART:
        rc = ftd_sock_recv_cpstart(lgp, &header);
        break;
    case FTDCCPSTOPP:
    case FTDCCPSTOPS:
    case FTDACKCPSTOP:
        bDbgLogON = 0;      
        rc = ftd_sock_recv_cpstop(lgp, &header);
        break;
    case FTDACKRFSTART:
        rc = ftd_sock_recv_ack_rfstart(lgp);
        break;

    case FTDCCPON:
        bDbgLogON = 1;  //no break, ok
    case FTDACKCPON:
        rc = ftd_sock_recv_cpon();
        break;

    case FTDCCPOFF:
    case FTDACKCPOFF:
        rc = ftd_sock_recv_cpoff();
        break;
    case FTDCCPONERR:
    case FTDACKCPONERR:
        rc = ftd_sock_recv_cponerr(fsockp, lgp);
        break;
    case FTDCCPOFFERR:
    case FTDACKCPOFFERR:
        rc = ftd_sock_recv_cpofferr(fsockp, lgp);
        break;
    case FTDCSTARTPMD:
        
        // just return the translated state
        rc = header.msg.lg.data;
        
        // hack for backfresh -f
        if ((rc & 0x000000ff) == FTD_SBACKFRESH) {
            if ((rc & FTD_LG_BACK_FORCE)) {
                rc = FTD_SBACKFRESH;
                SET_LG_BACK_FORCE(lgp->flags);
            }
        }

        return ftd_fsm_state_to_input(rc);

	// FTDCREMDRIVEERR sent from Secondary Master to RMD or RMD to PMD peer!! WR17511
    case FTDCREMDRIVEERR:
        rc = ftd_sock_recv_drive_err(lgp, &header);
        break;

    default:
        break;
    }

    return rc;
}

/*
 * ftd_sock_flush --
 * send a packet for a round trip - thus flushing al lpending packets
 */
int
ftd_sock_flush(ftd_lg_t *lgp) 
{
    int rc;

    // send it
    if ((rc = ftd_sock_send_hup(lgp)) < 0) {
        return rc;
    }
    
    UNSET_SOCK_HUP(lgp->dsockp->sockp->flags);
    
    for (;;) {
        if ((rc = ftd_sock_recv_lg_msg(lgp->dsockp, lgp, 0)) != 0) {
            if (rc != FTD_LG_NET_NOT_READABLE) {
                return rc;
            }
        }
        if (GET_SOCK_HUP(lgp->dsockp->sockp->flags)) {
            break;
        } 
        // sleep and try again 
        usleep(100000);
    }

    return 0;
}

/*
 * ftd_sock_test_link -- try to connect to the target ip/port
 */
int 
ftd_sock_test_link(char *lhostname, char *rhostname,
    unsigned long lip, unsigned long rip, int port, int timeo)
{
    ftd_sock_t          *fsockp;
    int                 rc;

    if ((fsockp = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL) {
        return -1;
    }
    
    if (ftd_sock_init(fsockp, lhostname, rhostname, lip, rip,
        SOCK_STREAM, AF_INET, 1, 0) < 0)
    {
        error_tracef(TRACEERR,"ftd_sock_test_link(): ftd_sock_init failed error tid=%x", GetCurrentThreadId());
        ftd_sock_delete(&fsockp);
        return -1;
    }
#if defined(_WINDOWS)   
    // create a bogus hEvent used to set connect to non-blocking
    FTD_SOCK_HEVENT(fsockp) = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
    
    if ((rc = ftd_sock_connect_nonb(fsockp, port, timeo, 0, 1)) != 1) {
        error_tracef(TRACEERR,"ftd_sock_test_link(): ftd_sock_connect_nonb failed() error , rc=%d , tid=%x", rc, GetCurrentThreadId());
        ftd_sock_delete(&fsockp);
        return rc;
    }

    ftd_sock_delete(&fsockp);
 
    return 1;
}

int 
ftd_sock_get_port(char *service)
{

    int             portnum = 0;
#if !defined(_WINDOWS)
    struct servent  *port;

    // get it from /etc/services if there
    if ((port = getservbyname(service, "tcp"))) {
        portnum = ntohs(port->s_port);
    }
#else
    char    portstr[32];

    cfg_get_software_key_value("port", portstr, CFG_IS_NOT_STRINGVAL);
    portnum = atoi(portstr);
#endif

    return portnum;
}

/*
 * ftd_sock_wait_for_peer_close
 * read until we get a close return on socket
 * timeout after 1 minute
 */
int 
ftd_sock_wait_for_peer_close(ftd_sock_t *fsockp)
{
    char    buf[4096];
    int rc, elapsed_time;
    time_t  now, start;

    time(&start);

    while (1) {
        if ((rc = sock_recv(fsockp->sockp, buf, sizeof(buf))) == 0) {
            return 1;
        }
        time(&now);
        elapsed_time = now - start;
        
        if (elapsed_time > 30) {
            ftd_sock_disconnect(fsockp);
            return 1;
        }
    }
}

