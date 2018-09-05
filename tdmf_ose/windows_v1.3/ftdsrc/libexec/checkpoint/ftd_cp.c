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

// Include this library to force the linker to link the MFC libraries in the correct order Mike Pollett
#include "../../forcelib.h"
// Mike Pollett
#include "../../tdmf.inc"

#include <conio.h>

#include "ftd_port.h"
#include "ftd_error.h"
#include "ftd_ps.h"
#include "errors.h"
#include "ftd_sock.h"
#include "ftd_config.h"
#include "ftd_cpoint.h"
#include "ftd_devlock.h"

#include "ftd_mngt.h"

//libftd, ftd_mngt_msgs.cpp
void ftd_mngt_msgs_log(const char **argv, int argc);

#define CP_MAX_TRY    5
#define ever ;;

extern volatile char qdsreleasenumber[];

static int gsbArgOn   = 0;
static int gsbArgWait = 0;
static int gsiPortNum = 0;
static int gsbSecMode = 0;

struct CpInBasket
{
	int bTodo;
	int iRole;     // Primary or Secondary
};

#define PRI_SIDE 1
#define SEC_SIDE 2

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
		"Usage: %s -a|-g <group_number> [-s] [-on | -off] [-w]\n",
		pcppArgV[0]
	);
	fprintf ( stderr, "OPTIONS:\n" );
	fprintf ( stderr, "         -a   => all groups\n" );
	fprintf ( stderr, "         -g   => group number\n" );
	fprintf ( stderr, "         -on	 => turn checkpoint on\n" );
	fprintf ( stderr, "         -off => turn checkpoint off\n" );
	fprintf ( stderr, "         -s   => turn checkpoint on or off from secondary\n");
	fprintf ( stderr, "         -w   => wait for response\n" );

	return CpMainErr( 0, "" );

} // CpUsage ()


static int
CpDoCpErr ( char* pcpMsg, ftd_lg_t* ppFtdLg )
{
	error_tracef( TRACEINF4, "%s:%s", gcpExe, pcpMsg );

	ftd_lg_delete( ppFtdLg );
	ftd_dev_lock_delete();

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

	if (GET_LG_JLESS(lpFtdLg->flags))
	  {
	  SET_JRN_JLESS(lpFtdLg->jrnp->flags);
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

	if ((lpFtdLg = ftd_lg_create()) == NULL) 
	{
		goto errret;
	}

	if (role == ROLESECONDARY) 
	{
		// Special case for SECONDARY where you skip access to locked device
		if (ftd_lg_init(lpFtdLg, piLgNum, role, 1) < 0) 
		{
			error_tracef( TRACEERR, "%s:lg_is_this_host():Calling ftd_lg_init(), role == ROLESECONDARY", gcpExe );
			goto errret;
		}
		// shove remote hostname into local hostname of ftd_sock_t
		strcpy(lcaHostName, lpFtdLg->cfgp->shostname);
	} 
	else 
	{
		if (ftd_lg_init(lpFtdLg, piLgNum, role, 1) < 0) 
		{
			error_tracef( TRACEERR, "%s:lg_is_this_host():Calling ftd_lg_init()", gcpExe );
			goto errret;
		}
		strcpy(lcaHostName, lpFtdLg->cfgp->phostname);
	}

	if (ftd_sock_init(lpFtdLg->dsockp, lcaHostName, "", 0, 0, SOCK_STREAM, AF_INET, 1, 0) < 0)
	{
		error_tracef( TRACEERR, "%s:lg_is_this_host():Calling ftd_sock_init()", gcpExe );
		goto errret;
	}

	// find out if the remote hostname in sxxx.cfg is this machine
	// and that if it is that it agrees with the pxxx.cfg's story 
	
	// 'ftd_sock_is_me' looks at local hostname in ftd_sock_t
	if (!strcmp(lcaHostName, "localhost") || ftd_sock_is_me(lpFtdLg->dsockp, 1)) 
	{
		ret = 1;
	}

errret:

	ftd_lg_delete(lpFtdLg);

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
	char lcPriHostName[MAXHOST];
	char lcSecHostName[MAXHOST];

	char                ch;
	int    liC;
	struct CpInBasket laInBasket [ FTD_MAX_GRP_NUM ];

////------------------------------------------
	void ftd_util_force_to_use_only_one_processor();
	ftd_util_force_to_use_only_one_processor();
//------------------------------------------

	gcpExe   = pcpaArgV[0];

	//memset ( targets, 0, sizeof(targets) ); // obsolete replaced with laInBasket
	//memset ( mask,    0, sizeof(mask) ); // obsolete replaced with laInBasket

	for ( liC = 0; liC < FTD_MAX_GRP_NUM; liC++ )
	{
		laInBasket [ liC ].bTodo    = FALSE;
		//laInBasket [ liC ].bApplied = FALSE;
		laInBasket [ liC ].iRole    = PRI_SIDE;
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
				return CpUsage( pcpaArgV );

			lbAll = TRUE;
			break;
		case 'g':
			if ( lbAll ) 
				return CpUsage( pcpaArgV );

			liLgGrp = strtol ( optarg, NULL, 0 );
			//laInBasket [ liLgGrp ].bTodo = TRUE;
			break;
		case 'o':
			if (optarg[0] == 'n') 
				gsbArgOn = 1; 
			else 
				gsbArgOn = 0; 
			break;
		case 'w':
			gsbArgWait = 1;
			break;
		case 's':
			gsbSecMode = 1;
			break;
		default:
			return CpUsage( pcpaArgV );
		}
	}

	if ( lbAll && liLgGrp >= 0 ) // No Lg Specified
	{
		return CpUsage( pcpaArgV );
	}

	if ( tcp_startup() < 0 )
	{
		return CpMainErr( 0, "Calling tcp_startup()" );
	}
	else if ( ftd_dev_lock_create() == -1 )
	{
		return CpMainErr( 1, "Calling ftd_dev_lock_create()" );
	}

    //send status msg to System Event Log and TDMF Collector
    ftd_mngt_msgs_log( pcpaArgV, piArgC );

	if ( !(gsiPortNum = ftd_sock_get_port( FTD_MASTER )) )
	{
		error_tracef( TRACEWRN, "%s:Calling ftd_sock_get_port()", gcpExe );
		gsiPortNum = FTD_SERVER_PORT;
	}

	if ( (lpLstCfg = ftd_config_create_list()) == NULL )
	{
		return CpMainErr( 0, "Calling ftd_config_create_list()" );
	}

	if ( !gsbSecMode )  // Run checkpoint cmd on Primary
	{
		if ( ftd_config_get_primary_started( PATH_CONFIG, lpLstCfg ) < 0 )
		{
			ftd_config_delete_list( lpLstCfg );
			return CpMainErr( 0, "Calling ftd_config_get_primary_started()" );
		}
		else ForEachLLElement( lpLstCfg, lppFtdLgCfg )	
		{
			if ( lbAll || (*lppFtdLgCfg)->lgnum == liLgGrp )
			{
				if ( lg_is_this_host( (*lppFtdLgCfg)->lgnum, ROLEPRIMARY) )
				{
					laInBasket [ (*lppFtdLgCfg)->lgnum ].bTodo = TRUE;
					laInBasket [ (*lppFtdLgCfg)->lgnum ].iRole = PRI_SIDE;
				}
			}
		}
	}
	else                // Run checkpoint cmd on Secondary
	{
		if ( ftd_config_get_secondary( PATH_CONFIG, lpLstCfg ) < 0 )
		{
			ftd_config_delete_list( lpLstCfg );
			return CpMainErr( 0, "Calling ftd_config_get_secondary()" );
		}
		else ForEachLLElement( lpLstCfg, lppFtdLgCfg )	
		{
			if ( lbAll || (*lppFtdLgCfg)->lgnum == liLgGrp )
			{
				if ( !ftd_lg_get_hostip((*lppFtdLgCfg)->lgnum , ROLESECONDARY , lcPriHostName, lcSecHostName) )
				{
					if ( !strncmp( lcPriHostName, lcSecHostName, MAXHOST ) ) 
					{
						laInBasket [ (*lppFtdLgCfg)->lgnum ].bTodo = TRUE;
						laInBasket [ (*lppFtdLgCfg)->lgnum ].iRole = PRI_SIDE;
					}
					else
					{
						laInBasket [ (*lppFtdLgCfg)->lgnum ].bTodo = TRUE;
						laInBasket [ (*lppFtdLgCfg)->lgnum ].iRole = SEC_SIDE;
					}
				}
			}
		}
	}

	memset ( &lFtdHeaderCp, 0, sizeof ( lFtdHeaderCp ) );
	lFtdHeaderCp.magicvalue = MAGICHDR;
	lFtdHeaderCp.cli        = 1;

	for ( liLgGrp = 0; liLgGrp < FTD_MAX_GRP_NUM; liLgGrp++ ) 
	{
		lFtdHeaderCp.msg.lg.lgnum = liLgGrp;

		if ( laInBasket [ liLgGrp ].bTodo )
		{
			if ( laInBasket [ liLgGrp ].iRole == PRI_SIDE )
			{
				lFtdHeaderCp.msgtype = gsbArgOn ? FTDCCPSTARTP: FTDCCPSTOPP;
				liLgCnt++;
			}
			else if ( laInBasket [ liLgGrp ].iRole == SEC_SIDE )
			{
				lFtdHeaderCp.msgtype = gsbArgOn ? FTDCCPSTARTS: FTDCCPSTOPS;
				liLgCnt++;
			}
			else
			{
				continue;
			}
		} 
		else 
		{
			continue;
		}

		if ( (lpFtdSock = ftd_sock_create( FTD_SOCK_GENERIC )) == NULL )
			return CpMainErr( 0, "Calling ftd_sock_create()" );

		if ( ftd_sock_init( lpFtdSock, "localhost", "localhost", LOCALHOSTIP, LOCALHOSTIP,
				            SOCK_STREAM, AF_INET, 1, 0 ) < 0 )
			return CpMainErr( 0, "Calling ftd_sock_init()" );

		FTD_SOCK_PORT( lpFtdSock ) = gsiPortNum;

		liRc = ftd_sock_connect_nonb( lpFtdSock, gsiPortNum, 3, 0, 1 );

		if (liRc == 1) 
		{

		} 
		else if (liRc == 0) 
		{
			reporterr ( ERRFAC, M_SOCKCONNECT, ERRWARN,
			            FTD_SOCK_LHOST(lpFtdSock),
			            FTD_SOCK_RHOST(lpFtdSock),
			            FTD_SOCK_PORT(lpFtdSock),
			            sock_strerror(ETIMEDOUT));

			ftd_sock_delete( &lpFtdSock );
			return CpMainErr( 0, "Calling ftd_sock_connect_nonb()" );
		} 
		else 
		{
			ftd_sock_delete( &lpFtdSock );
			return CpMainErr( 0, "Calling ftd_sock_connect_nonb()" );
		}

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

    SendAllPendingMessagesToCollector();
    
	(void)tcp_cleanup();
	ftd_dev_lock_delete();



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

	ftd_dev_lock_delete();
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

	char* lcpFacility = "Replicator";

	if ( ftd_init_errfac ( lcpFacility,  pcpProcName, NULL, NULL, 0, 1) == NULL )
	{
		return FALSE;
	}

	return TRUE;

} // CpInitErrFac ()


static int
CpMainErr ( int pciRet, char* pcpMsg )
{

    SendAllPendingMessagesToCollector();

	(void)tcp_cleanup();
	ftd_dev_lock_delete();
	error_tracef( TRACEERR, "%s:%s",      gcpExe, pcpMsg );
	error_tracef( TRACEERR,  "%s aborted", gcpExe );
	printf ( "Command Can Not Be Applied\n" );

	return pciRet;

} // CpMainErr ()




