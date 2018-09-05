/*
 * ftd_cpoint.c - FTD logical group checkpoint interface
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

#include "ftd_port.h"
#include "ftd_lg.h"
#include "ftd_sock.h"
#include "ftd_ps.h"
#include "ftd_ioctl.h"
#include "ftd_cpoint.h"


/*
 * ftd_lg_cppend --
 * cp mode on secondary pending apply of journaled data
 */
int
ftd_lg_cppend(ftd_lg_t *lgp) 
{
	ftd_journal_file_t	*jrnfp = NULL;
	int					rc;
	char				groupstr[MAXPATH];

	// close mirror devices 
	ftd_lg_close_devs(lgp);


	if (ftd_journal_get_all_files(lgp->jrnp) < 0) {
		return -1;
	}

	// create a checkpoint file 
	if ((rc = ftd_journal_create_next_cp(lgp->jrnp, jrnfp)) < 0) {
		return -1;
	} else if (rc == 0) {

		// close current, opened journal file 
		if (lgp->jrnp->cur && lgp->jrnp->cur->fd != INVALID_HANDLE_VALUE
			&& ftd_journal_file_close(lgp->jrnp->cur) < 0)
		{
			return -1;
		}

		// create a new journal file to write to during apply 
		if ((jrnfp = ftd_journal_create_next(lgp->jrnp, FTDJRNCO)) == NULL) {
			return -1;
		}

		// tell master to start apply daemon for group 
		if ((rc = ftd_sock_send_start_apply(lgp->lgnum, lgp->isockp, 1)) < 0) {
			return rc;
		}
		sprintf(groupstr, "Group: %03d Secondary", lgp->lgnum);
		reporterr(ERRFAC, M_CPPEND, ERRINFO, groupstr);
	}

	SET_LG_CPPEND(lgp->flags);
	UNSET_LG_CPSTART(lgp->flags);
	UNSET_LG_CPON(lgp->flags);
	SET_JRN_MODE(lgp->jrnp->flags, FTDJRNMIR);
	
	return 0;
}

/*
 * ftd_lg_cpstart --
 * start transition to checkpoint ON 
 */
int
ftd_lg_cpstart(ftd_lg_t *lgp) 
{
	ftd_header_t	ack;
	char			command[MAXPATH], groupstr[MAXPATH];
	int				portnum, rc, cperr = FALSE, save_flags;

	error_tracef(TRACEINF,"ftd_lg_cpstart in");

	save_flags = lgp->flags;

	memset(&ack, 0, sizeof(ack));

	switch(lgp->cfgp->role) {
	case ROLEPRIMARY:
		if (GET_LG_CPON(lgp->flags))
			{
			sprintf(groupstr, "Group: %03d Primary", lgp->lgnum);
			reporterr(ERRFAC, M_CPONAGAIN, ERRWARN, groupstr);
			cperr = TRUE;
			} 
		else {
#if defined(_WINDOWS)
			sprintf(command, "%s\\%s%03d.bat",
				PATH_CONFIG, PRE_CP_ON, lgp->lgnum);
			// if it doesn't have any executable commands,
			// don't try to run it
			if (!existCommands(command)
#else			
			sprintf(command, "%s/%s%03d.sh",
				PATH_CONFIG, PRE_CP_ON, lgp->lgnum);
			// if it doesn't exist then don't try to run it
			if (access(command, 0) != 0
#endif			
				|| ftd_proc_exec(command, 1) == 0)
				{
				// sync devs 
#if !defined(_WINDOWS)
				strcpy(command, "/bin/sync");
				if (ftd_proc_exec(command, 1) < 0) {
#else
				// this will unmount/sync the devices
//				ftd_lg_unlock_devs(lgp);
//				if (ftd_lg_lock_devs(lgp, ROLEPRIMARY) < 0) {
//					ftd_lg_unlock_devs(lgp);

//				if (ftd_lg_sync_prim_devs(lgp) < 0) {

				if (ftd_lg_unlock_devs(lgp) < 0) {
#endif

					// tell secondary
					ftd_sock_send_err(lgp->dsockp, ftd_get_last_error(ERRFAC));

					sprintf(groupstr, "Group: %03d Primary", lgp->lgnum);
					reporterr(ERRFAC, M_CPONERR, ERRWARN, groupstr);
				
					ack.msgtype = FTDCCPONERR;

					FTD_SOCK_SEND_HEADER( FALSE, LG_PROCNAME(lgp),"ftd_lg_cpstart" ,lgp->dsockp, &ack);
					
					cperr = TRUE;
					} 
				else 
					{
#if defined(_WINDOWS)
//					ftd_lg_unlock_devs(lgp);
#endif
					ftd_ioctl_send_lg_message(lgp->devfd, lgp->lgnum, MSG_CPON);
#if defined(_WINDOWS)
					sprintf(command, "%s\\%s%03d.bat",
						PATH_CONFIG, POST_CP_ON, lgp->lgnum);
					// if it doesn't have any executable commands,
					// don't try to run it
					if (existCommands(command)) 
						{
#else			
					sprintf(command, "%s/%s%03d.sh",
						PATH_CONFIG, POST_CP_ON, lgp->lgnum);
					// if it doesn't exist then don't try to run it
					if (access(command, 0) == 0) 
						{ 
#endif			
						ftd_proc_exec(command, 1);
						}		
					}
				} 
			else 
				{
				cperr = TRUE;
				
				// tell secondary
				ftd_sock_send_err(lgp->dsockp, ftd_get_last_error(ERRFAC));
				
				sprintf(groupstr, "Group: %03d Primary", lgp->lgnum);
				reporterr(ERRFAC, M_CPONERR, ERRWARN, groupstr);
				
				ack.msgtype = FTDCCPONERR;
				FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_cpstart",lgp->dsockp, &ack);
				}
			}

		if (GET_LG_CPSTART(lgp->flags) == LG_CPSTARTP) {	
			// tell master 
			memset(&ack, 0, sizeof(ack));

			ack.msgtype = FTDCCPON;
			ack.cli = LG_PID(lgp);
			ack.msg.lg.lgnum = lgp->lgnum;
			ack.msg.lg.data = lgp->cfgp->role; 


			rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_cpstart",lgp->isockp, &ack);	
		}

		if (cperr) {
			UNSET_LG_CPSTART(lgp->flags);
		}
		break;
	case ROLESECONDARY:
		if (GET_LG_CPON(lgp->flags)) {
			if (FTD_SOCK_CONNECT(lgp->dsockp)) {
				
				sprintf(groupstr, "Group: %03d Secondary", lgp->lgnum);
				reporterr(ERRFAC, M_CPONAGAIN, ERRWARN,	groupstr);
				
				if ((rc = ftd_sock_send_err(lgp->dsockp,
					ftd_get_last_error(ERRFAC))) < 0)
				{
					return rc;
				}
			}
			cperr = TRUE;
		} else {
			if (FTD_SOCK_CONNECT(lgp->dsockp)) {
				// tell primary that we are going into checkpoint
				memset(&ack, 0, sizeof(ack));

				ack.msgtype = FTDACKCPSTART;


				if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_cpstart",lgp->dsockp, &ack)) < 0) {
					return rc;
				}
			} else {

				// mirror threads not running - do cppend shit
				if (!(portnum = ftd_sock_get_port(FTD_MASTER))) {
					portnum = FTD_SERVER_PORT;
				}

				// connect ipc socket object to master
				if (ftd_sock_connect_nonb(lgp->isockp, portnum, 3, 0, 0) < 0) {
					break;
				}

				ftd_lg_cppend(lgp);
			}
		}

		if (GET_LG_CPSTART(lgp->flags) == LG_CPSTARTS) {	
			// tell master to let command know 
			memset(&ack, 0, sizeof(ack));
			ack.msgtype = FTDCCPON;
			ack.cli = LG_PID(lgp);
			ack.msg.lg.lgnum = lgp->lgnum;
			ack.msg.lg.data = lgp->cfgp->role; 


			rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_cpstart",lgp->isockp, &ack);	
		}

		if (cperr) {
			UNSET_LG_CPSTART(lgp->flags);
		}

		break;
	}

	return 0;
}

/*
 * ftd_lg_cpon --
 * go into cp mode for this group
 */
int
ftd_lg_cpon(ftd_lg_t *lgp) 
{
	ftd_header_t	ack;
	int				rc;
	char			command[MAXPATH], groupstr[MAXPATH];

	switch(lgp->cfgp->role) {
	case ROLEPRIMARY:

		// set checkpoint on in pstore
		if (ps_set_group_checkpoint(lgp->cfgp->pstore, lgp->devname, 1) < 0) {
			return -1;
		}

		sprintf(groupstr, "Group: %03d Primary", lgp->lgnum);
		reporterr(ERRFAC, M_CPON, ERRINFO, groupstr);

		break;
	case ROLESECONDARY:
		
		if (FTD_SOCK_CONNECT(lgp->dsockp)) {
			memset(&ack, 0, sizeof(ack));
			ack.msgtype = FTDACKCPON;

			if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_cpon",lgp->dsockp, &ack)) < 0) {
				return rc;
			}
		}
		
		UNSET_LG_CPPEND(lgp->flags);
		if (lgp->jrnp) {
			SET_JRN_MODE(lgp->jrnp->flags, FTDJRNONLY);
		}

		sprintf(groupstr, "Group: %03d Secondary", lgp->lgnum);
		reporterr(ERRFAC, M_CPON, ERRINFO, groupstr);

#if defined(_WINDOWS)
		sprintf(command, "%s\\%s%03d.bat",
			PATH_CONFIG, POST_CP_ON, lgp->lgnum);
		// if it doesn't have any executable commands,
		// don't try to run it
		if (existCommands(command)) {
#else			
		sprintf(command, "%s/%s%03d.sh",
			PATH_CONFIG, POST_CP_ON, lgp->lgnum);
		// if it doesn't exist then don't try to run it
		if (access(command, 0) == 0) {
#endif			
			ftd_proc_exec(command, 1);
		}

		break;
	default:
		break;
	}
	
	UNSET_LG_CPSTART(lgp->flags);
	SET_LG_CPON(lgp->flags);

	return 0;
}

/*
 * ftd_lg_cpoff --
 * secondary system - take group out of checkpoint 
 */
int
ftd_lg_cpoff(ftd_lg_t *lgp) 
{
	ftd_header_t	ack;
	char			command[MAXPATH], groupstr[MAXPATH];
	int				rc, save_flags;

	save_flags = lgp->flags;

	switch(lgp->cfgp->role) {
	case ROLEPRIMARY:
		// set checkpoint off in pstore
		if (ps_set_group_checkpoint(lgp->cfgp->pstore, lgp->devname, 0) < 0) {
			goto errret;
		}

		UNSET_LG_CPON(lgp->flags);
		UNSET_LG_CPSTART(lgp->flags);

		sprintf(groupstr, "Group: %03d Primary", lgp->lgnum);
		reporterr(ERRFAC, M_CPOFF, ERRINFO, groupstr); 

		break;
	case ROLESECONDARY:
#if defined(_WINDOWS)
		sprintf(command, "%s\\%s%03d.bat",
			PATH_CONFIG, PRE_CP_OFF, lgp->lgnum);
		// if it doesn't have any executable commands,
		// don't try to run it
		if (!existCommands(command)
#else			
		sprintf(command, "%s/%s%03d.sh",
			PATH_CONFIG, PRE_CP_OFF, lgp->lgnum);
		// if it doesn't exist then don't try to run it
		if (access(command, 0) != 0
#endif			
			|| ftd_proc_exec(command, 1) == 0)
		{
			UNSET_LG_CPON(lgp->flags);
			UNSET_LG_CPSTART(lgp->flags);
			UNSET_LG_CPPEND(lgp->flags);

			if (ftd_journal_get_all_files(lgp->jrnp) < 0) {
				goto errret;
			}

			if (ftd_journal_delete_cp(lgp->jrnp) < 0) {
				goto errret;
			}
		
			if (lgp->fprocp->proctype == FTD_PROC_RMD) {
				// if we are the rmd - do this
				if (ftd_lg_stop_journaling(lgp, FTDJRNCO, FTDJRNMIR) < 0) {
					goto errret;
				}
			} else {
				if ((rc = ftd_sock_send_start_apply(lgp->lgnum, lgp->isockp, 0)) < 0)
				{
					goto errret;
				}
			}

			// out of checkpoint mode 
			sprintf(groupstr, "Group: %03d Secondary", lgp->lgnum);
			reporterr(ERRFAC, M_CPOFF, ERRINFO, groupstr); 

			if (FTD_SOCK_CONNECT(lgp->dsockp)) {
				// ack the pmd 
				memset(&ack, 0, sizeof(ack));
				ack.msgtype = FTDACKCPOFF;
				rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_cpoff",lgp->dsockp, &ack);
			}
		} else {
			goto errret;
		}

		break;
	default:
		break;
	}

	UNSET_LG_CPSTOP(lgp->flags);

	return 0;

errret:

	{
		char	groupstr[MAXPATH];

		if (lgp->cfgp->role == ROLEPRIMARY) {
			sprintf(groupstr, "Group: %03d Primary", lgp->lgnum);
		} else {
			sprintf(groupstr, "Group: %03d Secondary", lgp->lgnum);
		}

		reporterr(ERRFAC, M_CPOFFERR, ERRWARN, groupstr); 
	}

	if (FTD_SOCK_CONNECT(lgp->dsockp)) {
		if ((rc = ftd_sock_send_err(lgp->dsockp,
			ftd_get_last_error(ERRFAC))) < 0)
		{
			return rc;
		}
	}

	lgp->flags = save_flags;

	UNSET_LG_CPSTOP(lgp->flags);

	return -1;
}

/*
 * ftd_lg_cpstop --
 * start transition to checkpoint OFF
 */
int
ftd_lg_cpstop(ftd_lg_t *lgp) 
{
	ftd_header_t	ack;
	int				portnum, rc, cperr = FALSE;
	char			groupstr[MAXPATH];

	switch(lgp->cfgp->role) {
	case ROLEPRIMARY:
		
		if (!GET_LG_CPON(lgp->flags) && !GET_LG_CPSTART(lgp->flags)) {
			sprintf(groupstr, "Group: %03d Primary", lgp->lgnum);
			reporterr(ERRFAC, M_CPOFFAGAIN, ERRWARN, groupstr);
			cperr = TRUE;
		} else {
			if (ftd_ioctl_send_lg_message(lgp->devfd,
				lgp->lgnum, MSG_CPOFF) < 0) {
				return -1;
			}
		}

		if (GET_LG_CPSTOP(lgp->flags) == LG_CPSTOPP) {
			// tell master to let command know 
			memset(&ack, 0, sizeof(ack));
			ack.msgtype = FTDCCPOFF;
			ack.cli = LG_PID(lgp);
			ack.msg.lg.lgnum = lgp->lgnum;
			ack.msg.lg.data = lgp->cfgp->role; 


			FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_cpstop",lgp->isockp, &ack);	
		}
		
		UNSET_LG_CPSTOP(lgp->flags);

		break;
	case ROLESECONDARY:
		if (!GET_LG_CPPEND(lgp->flags) && !GET_LG_CPON(lgp->flags)) {
			if (FTD_SOCK_CONNECT(lgp->dsockp)) {
				
				sprintf(groupstr, "Group: %03d Secondary", lgp->lgnum);
				reporterr(ERRFAC, M_CPOFFAGAIN, ERRWARN, groupstr);
				
				if ((rc = ftd_sock_send_err(lgp->dsockp,
					ftd_get_last_error(ERRFAC))) < 0)
				{
					return rc;
				}
			}
			cperr = TRUE;
		} else {
			if (FTD_SOCK_CONNECT(lgp->dsockp)) {
				// tell primary that we are going out of checkpoint
				memset(&ack, 0, sizeof(ack));
				ack.msgtype = FTDACKCPSTOP;

				if ((rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_cpstop",lgp->dsockp, &ack)) < 0) {
					return rc;
				}
			} else {
				// mirror daemons not running - do cpoff shit
				if (!(portnum = ftd_sock_get_port(FTD_MASTER))) {
					portnum = FTD_SERVER_PORT;
				}

				// connect ipc socket object to master
				if (ftd_sock_connect_nonb(lgp->isockp, portnum, 3, 0, 0) != 1) {
					break;
				}
		
				ftd_lg_cpoff(lgp);
			}
		}

		if (GET_LG_CPSTOP(lgp->flags) == LG_CPSTOPS) {	
			// tell master to let command know 
			memset(&ack, 0, sizeof(ack));
			ack.msgtype = FTDCCPOFF;
			ack.cli = LG_PID(lgp);
			ack.msg.lg.lgnum = lgp->lgnum;
			ack.msg.lg.data = lgp->cfgp->role; 

			rc = FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"ftd_lg_cpstop",lgp->isockp, &ack);	
		}

		if (cperr) {
			UNSET_LG_CPSTOP(lgp->flags);
		}

		break;
	}

	return 0;
}

// DTurrin - Oct 17th, 2001
//
// This procedure checks if a .bat file exists. If it exists,
// a check is made to see if any commands will be executed by
// running the .bat file. If commands can be executed, then
// the procedure returns TRUE.
BOOL existCommands ( char *fileName )
{
   FILE *comFile;
   char line[255], compLine[3];
   BOOL runComFile = FALSE;

   if ((comFile = fopen(fileName,"r")) != NULL)
   {
       while(!runComFile && (fgets(line,255,comFile) != NULL))
       {
           int i = 0;
           while (strlen(&line[i]) > 2)
           {
               // If the line is a remark (ie. REM), break the loop.
			   sprintf(compLine,"%c%c%c",
				   toupper(line[i]),toupper(line[i + 1]),toupper(line[i + 2]));
			   if (strcmp(compLine,"REM") == 0)
			   { 
				   break;
			   }

               // If the current character is not a space or a tab,
			   // break the loop.
               if ((line[i] != ' ') || (line[i] != '\t')) break;

               i++;
           }

		   // If the line length is greater than 2 and it is not a
		   // remark (ie. REM), then we assume that it is a command
		   // line. So return TRUE.
		   if ((strlen(&line[i]) > 2) &&
               (strcmp(compLine,"REM") != 0)) runComFile = TRUE;
       }

       fclose(comFile);
   }

   return runComFile;
}
