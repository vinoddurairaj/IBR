/*
 * ftd_cp.c - checkpoint command 
 *
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#include <conio.h>

#include "ftd_port.h"
#include "ftd_error.h"
#include "ftd_ps.h"
#include "errors.h"
#include "ftd_sock.h"
#include "ftd_config.h"
#include "ftd_cpoint.h"

#if defined(_WINDOWS)
#include "ftd_devlock.h"
#endif

#define CP_MAX_TRY    5
#define ever ;;

extern volatile char qdsreleasenumber[];

static int gsbArgOn   = 0;
static int gsbArgWait = 0;
static int gsiPortNum = 0;

struct CpInBasket
{
	int bTodo;
	//int bApplied;  //
	//int iRole;     // Primary or Secondary
};

enum CpWaitRslt
{ 
	eFalse, 
	eTrue, 
	eTry 
};

char* gcpExe; // Executable File Name 

static int 
CpUsage            ( char** pcppArgV );

static int      
CpDoCheckPoint     ( int piLgNum, int piMsgType, char* pcpHostName );

static int      
CpDoCpErr          ( char* pcpMsg, ftd_lg_t* ppFtdLg );

static BOOL     
CpInitErrFac       ( char* pcpProcName );

static int      
CpMainErr          ( int pciRet, char* pcpMsg );

static ftd_sock_t*
CpSockCreate       ();

static int      
CpWaitForKbHit     ( int pciRet );

static int      
CpWaitForResult    ( struct CpInBasket paInBasket[] );

static enum CpWaitRslt
CpWaitForPrimary   ( int piC );

static enum CpWaitRslt 
CpWaitForSecondary ( int piC );


static int
CpUsage ( char** pcppArgV )
{
	fprintf (
		stderr,
		"Usage: %s -a|-g <group_number> [-p | -s] [-on | -off] [-w]\n",
		pcppArgV[0]
	);
	fprintf ( stderr, "OPTIONS:\n" );
	fprintf ( stderr, "         -a	=> all groups\n" );
	fprintf ( stderr, "         -g	=> group number\n" );
	fprintf ( stderr, "         -on	=> turn checkpoint on\n" );
	fprintf ( stderr, "         -off	=> turn checkpoint off\n" );
	fprintf ( stderr, "         -w	=> wait for response\n" );

	return CpMainErr( 0, "" );

} // CpUsage ()


static int
CpDoCpErr ( char* pcpMsg, ftd_lg_t* ppFtdLg )
{
	error_tracef( TRACEINF4, "%s:%s", gcpExe, pcpMsg );

	ftd_lg_delete( ppFtdLg );

#if defined(_WINDOWS)
	ftd_dev_lock_delete();
#endif

	return 0;

} // CpDoCpErr ()


static int
CpDoCheckPoint ( int piLgNum, int piMsgType, char* pcpHostName )
{
	ftd_lg_t* lpFtdLg = NULL;
	char      lcaPrefix [ 16 ];
	char      lcaGrpStr [ 32 ];
	int       liRc;

#if defined(_WINDOWS)
	if ( ftd_dev_lock_create() == -1 ) 
	{
		return CpDoCpErr( "CpDoCheckPoint():Calling ftd_dev_lock_create()", lpFtdLg );
	}
#endif

	// perform checkpoint for down group
	if ( (lpFtdLg = ftd_lg_create()) == NULL )
	{
		return CpDoCpErr( "CpDoCheckPoint():Calling ftd_lg_create()", lpFtdLg );
	}
	else if ( ftd_lg_init( lpFtdLg, piLgNum, ROLESECONDARY, 0 ) < 0 )
	{
		return CpDoCpErr( "CpDoCheckPoint():Calling ftd_lg_init()", lpFtdLg );
	}

	sprintf ( lcaPrefix, "j%03d", lpFtdLg->lgnum );

	if ( (lpFtdLg->jrnp = 
		  ftd_journal_create( lpFtdLg->cfgp->jrnpath, lcaPrefix )
		 ) == NULL
	   )
	{
		return CpDoCpErr( "CpDoCheckPoint():Calling ftd_journal_create()", lpFtdLg );
	}

	ftd_journal_get_cur_state( lpFtdLg->jrnp, 0 );

	// set group flags according to journal flags
	if ( GET_JRN_CP_ON(lpFtdLg->jrnp->flags) ) 
	{
		SET_LG_CPON(lpFtdLg->flags);
	}

	if ( GET_JRN_CP_PEND(lpFtdLg->jrnp->flags) )
	{
		SET_LG_CPPEND(lpFtdLg->flags);
	}

	sprintf ( lcaGrpStr, "Group %03d", lpFtdLg->lgnum );

	if ( (lpFtdLg->isockp = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL )
	{
		return CpDoCpErr( "CpDoCheckPoint():Calling ftd_sock_create()", lpFtdLg );
	}
	else if ( ftd_sock_init( 
				lpFtdLg->isockp, "localhost", "localhost", 
				LOCALHOSTIP, LOCALHOSTIP,
				SOCK_STREAM, AF_INET, 1, 0
			  ) < 0
			)
	{
		return CpDoCpErr( "CpDoCheckPoint():Calling ftd_sock_init()", lpFtdLg );
	}
	else if ( piMsgType == FTDACKDOCPON )
	{
		// if checkpoint file exits then already in
		// cp or transitioning to cp
		if ( GET_LG_CPPEND(lpFtdLg->flags) )
		{
			reporterr( ERRFAC, M_CPONAGAIN, ERRWARN, lcaGrpStr );
			return CpDoCpErr( "CpDoCheckPoint():Calling ftd_sock_init()", lpFtdLg );
		} 
		else if ( GET_LG_CPON(lpFtdLg->flags) )
		{
			reporterr( ERRFAC, M_CPONAGAIN, ERRWARN, lcaGrpStr );
			return CpDoCpErr( "CpDoCheckPoint():Calling ftd_sock_init()", lpFtdLg );
		}
		else
		{
			liRc = ftd_lg_cpstart( lpFtdLg );
		}
	} 
	else if ( piMsgType == FTDACKDOCPOFF )
	{
		if (   !GET_LG_CPON(lpFtdLg->flags)
			&& !GET_LG_CPPEND(lpFtdLg->flags)
		   )
		{
			reporterr( ERRFAC, M_CPOFFAGAIN, ERRWARN, lcaGrpStr );
			return CpDoCpErr( "CpDoCheckPoint():Calling ftd_sock_init()", lpFtdLg );
		}
		else
		{
			liRc = ftd_lg_cpstop(lpFtdLg);
		}
	}

	return (liRc==-1)?0:1;

} // CpDoCheckPoint ()


static int
lg_is_this_host(int piLgNum, int role)
{
	ftd_lg_t	*lpFtdLg;
	int			ret = 0;
	char		lcaHostName[MAXHOST];

	if ((lpFtdLg = ftd_lg_create()) == NULL) {
		goto errret;
	}

	if (role == ROLESECONDARY) {
		// Special case for SECONDARY where you skip access to locked device
		if (ftd_lg_init(lpFtdLg, piLgNum, role, 1) < 0) {
			error_tracef( TRACEERR, "%s:lg_is_this_host():Calling ftd_lg_init(), role == ROLESECONDARY", gcpExe );
			goto errret;
		}
		// shove remote hostname into local hostname of ftd_sock_t
		strcpy(lcaHostName, lpFtdLg->cfgp->shostname);
	} else {
		if (ftd_lg_init(lpFtdLg, piLgNum, role, 1) < 0) {
			error_tracef( TRACEERR, "%s:lg_is_this_host():Calling ftd_lg_init()", gcpExe );
			goto errret;
		}
		strcpy(lcaHostName, lpFtdLg->cfgp->phostname);
	}

	if (ftd_sock_init(lpFtdLg->dsockp, lcaHostName, "", 0, 0,
		SOCK_STREAM, AF_INET, 1, 0) < 0)
	{
		error_tracef( TRACEERR, "%s:lg_is_this_host():Calling ftd_sock_init()", gcpExe );
		goto errret;
	}

	// find out if the remote hostname in sxxx.cfg is this machine
	// and that if it is that it agrees with the pxxx.cfg's story 
	
	// 'ftd_sock_is_me' looks at local hostname in ftd_sock_t
	if (!strcmp(lcaHostName, "localhost") || ftd_sock_is_me(lpFtdLg->dsockp, 1)) {
		ret = 1;
	}

errret:

	ftd_lg_delete(lpFtdLg);

	return ret;
}

static int
lg_is_loopback(int piLgNum)
{
	ftd_lg_t	*plgp, *slgp;
	int			ret = 0;

	if (!lg_is_this_host(piLgNum, ROLESECONDARY)) {
		return 0;
	}

	if ((plgp = ftd_lg_create()) == NULL) {
		error_tracef( TRACEERR, "%s:lg_is_loopback():Calling ftd_lg_create(),p", gcpExe );
		goto errret;
	}

	if (ftd_lg_init(plgp, piLgNum, ROLEPRIMARY, 1) < 0) {
		error_tracef( TRACEERR, "%s:lg_is_loopback():Calling ftd_lg_init(),p", gcpExe );
		goto errret;
	}
	
	if ((slgp = ftd_lg_create()) == NULL) {
		error_tracef( TRACEERR, "%s:lg_is_loopback():Calling ftd_lg_create(),p", gcpExe );
		goto errret;
	}

	if (ftd_lg_init(slgp, piLgNum, ROLESECONDARY, 0) < 0) {
		error_tracef( TRACEERR, "%s:lg_is_loopback():Calling ftd_lg_init(),p", gcpExe );
		goto errret;
	}

	// does the primary remote hostname match ?
	if ( !strncmp(plgp->cfgp->shostname, slgp->cfgp->shostname, MAXHOST ) ) {
		// lg is in loopback configuration
		ret = 1;
	}

errret:

	ftd_lg_delete(plgp);
	ftd_lg_delete(slgp);

	return ret;
}

int 
main ( int piArgC, char* pcpaArgV[] )
{
	ftd_header_t	lFtdHeaderCp;
	ftd_header_t	lFtdHeaderAck;
	ftd_sock_t*		lpFtdSock      = NULL;
	ftd_lg_cfg_t**	lppFtdLgCfg;
	LList*			lpLstCfg       = NULL;
	int             liLgGrp        = -1;
	int             liLgCnt        = 0;
	BOOL            lbAll          = FALSE;
	int             liRc;
	int				liBuf          = 0;
	//char            lcaHostName [ MAXHOST ];
	//unsigned char   mask[FTD_MAX_GRP_NUM]; // obsolete replaced with laInBasket
	//int             exitcode = 0; // ??? always 0 except on ftd_dev_lock_create()
									// error where it becomes 1
	//int             targets[FTD_MAX_GRP_NUM];  // obsolete replaced with laInBasket

#if defined(_AIX)
	int                 ch;
#else  /* defined(_AIX) */
	char                ch;
#endif /* defined(_AIX) */

	int    liC;
	struct CpInBasket laInBasket [ FTD_MAX_GRP_NUM ];

////------------------------------------------
#if defined(_WINDOWS)
	//void ftd_util_force_to_use_only_one_processor();
	//ftd_util_force_to_use_only_one_processor();
#endif
//------------------------------------------

	gcpExe   = pcpaArgV[0];

	//memset ( targets, 0, sizeof(targets) ); // obsolete replaced with laInBasket
	//memset ( mask,    0, sizeof(mask) ); // obsolete replaced with laInBasket
	memset ( &lFtdHeaderCp, 0, sizeof ( lFtdHeaderCp ) );

	for ( liC = 0; liC < FTD_MAX_GRP_NUM; liC++ )
	{
		laInBasket [ liC ].bTodo    = FALSE;
		//laInBasket [ liC ].bApplied = FALSE;
		//laInBasket [ liC ].iRole    = 0;
	}

//	CpTracef( TRACECMT, "CheckPoint" );

	if ( !CpInitErrFac( pcpaArgV[0] ) )
	{
		return CpMainErr( 0, "Calling ftd_init_errfac()" );
	}
	else if ( piArgC < 3 ) 
	{
		return CpUsage( pcpaArgV );
	}
	else while ( (ch = getopt ( piArgC, pcpaArgV, "apswg:o:" )) != -1 )
	{
		switch(ch) 
		{
		case 'a':
			if ( liLgGrp != -1 ) 
			{
				return CpUsage( pcpaArgV );
			}
			lbAll = TRUE;
			break;
		case 'g':
			if ( lbAll ) 
			{
				return CpUsage( pcpaArgV );
			}
			liLgGrp = strtol ( optarg, NULL, 0 );
			//laInBasket [ liLgGrp ].bTodo = TRUE;
			break;
		case 'o':
			if (optarg[0] == 'n') 
			{
				gsbArgOn = 1; 
			} else 
			{
				gsbArgOn = 0; 
			}
			break;
		case 'w':
			gsbArgWait = 1;
			break;
		default:
			return CpUsage( pcpaArgV );
		}
	}

	if ( lbAll && liLgGrp >= 0 ) // No Lg Specified
	{
		return CpUsage( pcpaArgV );
	}

#if defined(_WINDOWS)
	if ( tcp_startup() < 0 )
	{
		return CpMainErr( 0, "Calling tcp_startup()" );
	}
	else if ( ftd_dev_lock_create() == -1 )
	{
		return CpMainErr( 1, "Calling ftd_dev_lock_create()" );
	}

#else
	// see if master running
	if ( ftd_proc_get_pid(FTD_MASTER, 0, &liBuf ) <= 0 )
	{
		reporterr( ERRFAC, M_NOMASTER, ERRWARN, pcpaArgV[0] );
		return CpMainErr( 0, "Calling ftd_proc_get_pid()" );
	}
	else if ( geteuid() )	// Make sure we are root
	{
		reporterr(ERRFAC, M_NOTROOT, ERRCRIT);
		return CpMainErr( 0, "Calling geteuid()" );
	}
#endif

	if ( !(gsiPortNum = ftd_sock_get_port( FTD_MASTER )) )
	{
		error_tracef( TRACEWRN, "%s:Calling ftd_sock_get_port()", gcpExe );
		gsiPortNum = FTD_SERVER_PORT;
	}

	if ( (lpLstCfg = ftd_config_create_list()) == NULL )
	{
		return CpMainErr( 0, "Calling ftd_config_create_list()" );
	}
	else if ( ftd_config_get_primary_started( PATH_CONFIG, lpLstCfg ) < 0 )
	{
		ftd_config_delete_list( lpLstCfg );
		return CpMainErr( 0, "Calling ftd_config_get_primary_started()" );
	}
	else ForEachLLElement( lpLstCfg, lppFtdLgCfg )	
	{
		if ( lbAll || (*lppFtdLgCfg)->lgnum == liLgGrp )
		{
			laInBasket [ (*lppFtdLgCfg)->lgnum ].bTodo = TRUE;
		}
	}

	lFtdHeaderCp.magicvalue = MAGICHDR;

	for ( liLgGrp = 0; liLgGrp < FTD_MAX_GRP_NUM; liLgGrp++ ) 
	{
		if ( laInBasket [ liLgGrp ].bTodo )
		{
			lFtdHeaderCp.msgtype = gsbArgOn ? FTDCCPSTARTP: FTDCCPSTOPP;
			liLgCnt++;
		} 
		else 
		{
			continue;
		}

		if ( (lpFtdSock = CpSockCreate()) == NULL )
		{
			return CpMainErr( 0, "Calling CpSockCreate()" );
		}

		lFtdHeaderCp.msg.lg.lgnum = liLgGrp;
		lFtdHeaderCp.cli          = (HANDLE)1;

		if ( FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"main" , lpFtdSock, &lFtdHeaderCp ) < 0 )
		{
			ftd_sock_delete( &lpFtdSock );
			// ardev maybe we should just clean and continue
			return CpMainErr( 0, "Calling FTD_SOCK_SEND_HEADER()" );
		}

		// get ack from master
		liRc = FTD_SOCK_RECV_HEADER( __FILE__ ,"main",lpFtdSock, &lFtdHeaderAck );
		if (liRc < 0) 
		{
			ftd_sock_delete( &lpFtdSock );
			return CpMainErr( 0, "Calling FTD_SOCK_RECV_HEADER()" );
		}
		else switch ( lFtdHeaderAck.msgtype ) 
		{
		case FTDACKNOPMD:
			error_tracef( TRACEINF4, "%s:Calling FTD_SOCK_RECV_HEADER():FTDACKNOPMD", gcpExe );
			reporterr( ERRFAC, M_NOPMD, ERRWARN, lFtdHeaderAck.msg.lg.lgnum );
			laInBasket [ liLgGrp ].bTodo = FALSE;
			break;
		case FTDACKNORMD:
			error_tracef( TRACEINF4, "%s:Calling FTD_SOCK_RECV_HEADER():FTDACKNORMD", gcpExe );
			reporterr( ERRFAC, M_NORMD, ERRWARN, lFtdHeaderAck.msg.lg.lgnum );
			laInBasket [ liLgGrp ].bTodo = FALSE;
			break;
		case FTDACKDOCPON:
		case FTDACKDOCPOFF:
			if ( !CpDoCheckPoint( 
					lFtdHeaderAck.msg.lg.lgnum,
					lFtdHeaderAck.msgtype, "localhost"
			      )
			   )
			{
				laInBasket [ liLgGrp ].bTodo = FALSE;
			}
			break;
		default: // heeeu ???
			// We Kind of receive Hrd 53 which is FTDACKNOPMD
			// Lets ignore this message and start probing the CheckPoint Mode.
			// laInBasket [ liLgGrp ].bTodo = FALSE;
			break;
		}

		error_tracef(
			TRACEINF4, "%s ack.msgtype, Hdr = %d, Lg Count =%d",
			gcpExe, lFtdHeaderAck.msgtype, liLgCnt
		);

		usleep ( 10000 );

		ftd_sock_delete( &lpFtdSock );

	} // for ( liLgGrp = 0; liLgGrp < FTD_MAX_GRP_NUM; liLgGrp++ )

	if ( liLgCnt == 0 )
	{
		return CpMainErr( 0, "No Matching Logical Group found" );
	}
	else if ( gsbArgWait )
	{
		if ( !CpWaitForResult ( laInBasket ) )
		{
			return CpMainErr( 0, "Can Not Wait For Status Change" );
		}
	}

#if defined(_WINDOWS)
	(void)tcp_cleanup();
	ftd_dev_lock_delete();
#endif

#if defined(_WINDOWS) && defined(_DEBUG)

	printf ( "\nPress any key to continue....\n" );
	while ( !_kbhit () )
	{
		Sleep ( 1 );
	}
#endif

	exit ( 0 );

} // main ()


int CpWaitForResult ( struct CpInBasket paInBasket[] )
{
	int                liC;
	int                liTry;
	enum CpWaitForRslt leCatched;

	for ( liC = 0; liC < FTD_MAX_GRP_NUM; liC++ ) 
	{
		liTry = 0;

		if ( !paInBasket [ liC ].bTodo )
		{
			continue;
		}
		else for ( ever )
		{
			leCatched = CpWaitForPrimary( liC );

			if ( leCatched = eTrue )
			{
				error_tracef( 
					TRACEINF,
					"%s:group %d is in CheckPoint mode %s",
					gcpExe, liC, (gsbArgOn)?"ON":"OFF"
				);
				break;
			}
			else if  ( leCatched = eFalse && liTry++ == CP_MAX_TRY )
			{
				error_tracef(
					TRACEINF,
					"%s:CheckPoint submited for group %d, but cannot get status",
					gcpExe, liC
				);
			}
			else
			{
				usleep ( 1000000 );
			}
		} // for ( ever )
	} // for ( liC = 0; liC < FTD_MAX_GRP_NUM; liC++ )

#if defined(_WINDOWS)
	ftd_dev_lock_delete();
#endif

	return TRUE;

} // CpWaitForResult ()


enum CpWaitRslt CpWaitForPrimary ( int piC )
{
	ftd_lg_t*       lpFtdLg;
	ps_group_info_t	lPsGrpInf;

	//lPsGrpInf.name = NULL;

	if ( (lpFtdLg = ftd_lg_create()) == NULL ) 
	{
		return eFalse;
	}
	else if ( ftd_lg_init( lpFtdLg, piC, ROLEPRIMARY, 1 ) < 0 )
	{
		return eFalse;
	}
	else if ( ps_get_group_info(
					lpFtdLg->cfgp->pstore,
					lpFtdLg->devname, 
					&lPsGrpInf
			  ) != PS_OK
			)
	{
		return eFalse;
	}
	else if ( lPsGrpInf.checkpoint == gsbArgOn ) // OK, We Catched it
	{
		return eTrue;
	}
	else
	{
		return eTry;
	}

} // CpWaitForPrimary ()


BOOL CpInitErrFac ( char* pcpProcName )
{
#if !defined(_WINDOWS)
	char* lcpFacility = CAPQ;
#else
	char* lcpFacility = PRODUCTNAME;
#endif		

	if ( ftd_init_errfac ( lcpFacility,  pcpProcName, NULL, NULL, 0, 1) == NULL )
	{
		return FALSE;
	}

	return TRUE;

} // CpInitErrFac ()


static int
CpMainErr ( int pciRet, char* pcpMsg )
{
#if defined(_WINDOWS)
	(void)tcp_cleanup();
	ftd_dev_lock_delete();
#endif

	error_tracef( TRACEERR, "%s:%s",      gcpExe, pcpMsg );
	error_tracef( TRACEERR,  "%s aborted", gcpExe );
	printf ( "Command Can Not Be Applied\n" );

#if defined(_WINDOWS) && defined(_DEBUG)
	printf ( "\nPress any key to continue....\n" );

	while ( !_kbhit () )
	{
		Sleep(1);
	}
#endif

	return pciRet;

} // CpMainErr ()


static ftd_sock_t* CpSockCreate ()
{
	int         liRc;
 	ftd_sock_t* lpFtdSock = NULL;

	if ( (lpFtdSock = ftd_sock_create( FTD_SOCK_GENERIC )) == NULL )
	{
		error_tracef( TRACEERR, "Calling ftd_sock_create()" );
		return NULL;
	}
	else if ( ftd_sock_init( 
			lpFtdSock, "localhost", "localhost", LOCALHOSTIP, LOCALHOSTIP,
			SOCK_STREAM, AF_INET, 1, 0
		 ) < 0
	   )
	{
		error_tracef( TRACEERR, "Calling ftd_sock_init()" );
		return NULL;
	}

	FTD_SOCK_PORT( lpFtdSock ) = gsiPortNum;

	liRc = ftd_sock_connect_nonb( lpFtdSock, gsiPortNum, 3, 0, 1 );

	if (liRc == 1) 
	{
		return lpFtdSock;
	} 
	else if (liRc == 0) 
	{
		reporterr (
			ERRFAC, M_SOCKCONNECT, ERRWARN,
			FTD_SOCK_LHOST(lpFtdSock),
			FTD_SOCK_RHOST(lpFtdSock),
			FTD_SOCK_PORT(lpFtdSock),
			sock_strerror(ETIMEDOUT)
		);

		ftd_sock_delete( &lpFtdSock );
		error_tracef( TRACEERR, "Calling ftd_sock_connect_nonb()" );
		return NULL;
	} 
	else 
	{
		ftd_sock_delete( &lpFtdSock );
		error_tracef( TRACEERR, "Calling ftd_sock_connect_nonb()" );
		return NULL;
	}

} // ftd_sock_t* CpSockCreate ()
