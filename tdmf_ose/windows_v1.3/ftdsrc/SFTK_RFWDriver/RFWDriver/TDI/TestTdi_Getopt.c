/*****************************************************************************
 *
 *  MODULE NAME : GETOPT.C
 *
 *  COPYRIGHTS:
 *             This module contains code made available by IBM
 *             Corporation on an AS IS basis.  Any one receiving the
 *             module is considered to be licensed under IBM copyrights
 *             to use the IBM-provided source code in any way he or she
 *             deems fit, including copying it, compiling it, modifying
 *             it, and redistributing it, with or without
 *             modifications.  No license under any IBM patents or
 *             patent applications is to be implied from this copyright
 *             license.
 *
 *             A user of the module should understand that IBM cannot
 *             provide technical support for the module and will not be
 *             responsible for any consequences of use of the program.
 *
 *             Any notices, including this one, are not to be removed
 *             from the module without the prior written consent of
 *             IBM.
 *
 *  AUTHOR:   Original author:
 *                 G. R. Blair (BOBBLAIR at AUSVM1)
 *                 Internet: bobblair@bobblair.austin.ibm.com
 *
 *            Extensively revised by:
 *                 John Q. Walker II, Ph.D. (JOHHQ at RALVM6)
 *                 Internet: johnq@ralvm6.vnet.ibm.com
 *
 *****************************************************************************/

/******************************************************************************
 * getopt()
 *
 * The getopt() function is a command line parser.  It returns the next
 * option character in argv that matches an option character in opstring.
 *
 * The argv argument points to an array of argc+1 elements containing argc
 * pointers to character strings followed by a null pointer.
 *
 * The opstring argument points to a string of option characters; if an
 * option character is followed by a colon, the option is expected to have
 * an argument that may or may not be separated from it by white space.
 * The external variable optarg is set to point to the start of the option
 * argument on return from getopt().
 *
 * The getopt() function places in optind the argv index of the next argument
 * to be processed.  The system initializes the external variable optind to
 * 1 before the first call to getopt().
 *
 * When all options have been processed (that is, up to the first nonoption
 * argument), getopt() returns EOF.  The special option "--" may be used to
 * delimit the end of the options; EOF will be returned, and "--" will be
 * skipped.
 *
 * The getopt() function returns a question mark (?) when it encounters an
 * option character not included in opstring.  This error message can be
 * disabled by setting opterr to zero.  Otherwise, it returns the option
 * character that was detected.
 *
 * If the special option "--" is detected, or all options have been
 * processed, EOF is returned.
 *
 * Options are marked by either a minus sign (-) or a slash (/).
 *
 * No errors are defined.
 *****************************************************************************/

#include <stdio.h>                  /* for EOF */
#include <string.h>                 /* for strchr() */
#include <stdlib.h>
#include <tchar.h>

#include <winsock2.h>
#include <winioctl.h>

// Add all the definitions that are required in the User Space
#define USER_SPACE

#include <pshpack1.h>
#include "../ftdio.h"
#include "TestTdi_Getopt.h"
#include <packoff.h>

/* static (global) variables that are specified as exported by getopt() */
char *optarg = NULL;    /* pointer to the start of the option argument  */
int   optind = 1;       /* number of the next argv[] to be evaluated    */
int   opterr = 1;       /* non-zero if a question mark should be returned
                           when a non-valid option character is detected */

/* handle possible future character set concerns by putting this in a macro */
#define _next_char(string)  (char)(*(string+1))


struct Command_t g_Commands[] = {
	{ sm_add						, "ad"		, FALSE	},
	{ sm_remove						, "rm"		, FALSE },
	{ sm_enable						, "en"		, FALSE },
	{ sm_disable					, "ds"		, FALSE },
	{ sm_init						, "init"	, FALSE },
	{ sm_start						, "strt"	, FALSE },
	{ sm_stop						, "stop"	, FALSE },
	{ sm_uninit						, "uint"	, FALSE },
	{ sm_logicalgroup				, "lgp"		, TRUE	},
	{ sm_sendwindowsize				, "sews"	, TRUE	},
	{ sm_receivewindowsize			, "rcws"	, TRUE	},
	{ sm_maxnumberofsendbuffers		, "maxsebf" , TRUE	},
	{ sm_maxnumberofreceivebuffers	, "maxrcbf" , TRUE	},
	{ sm_chunksize					, "cksz"	, TRUE	},
	{ sm_chunkdelay					, "ckdl"	, TRUE	},
	{ sm_primary					, "pri"		, FALSE },
	{ sm_secondary					, "sec"		, FALSE },
	{ sm_sourceipaddress			, "srcip"	, TRUE	},
	{ sm_remoteipaddress			, "remip"	, TRUE	},
	{ sm_sourceport					, "srcpt"	, TRUE	},
	{ sm_remoteport					, "rempt"	, TRUE	},
	{ sm_sendnormal					, "sndnrl"	, FALSE	},
	{ sm_sendrefresh				, "sndrsh"	, TRUE	}
};

LONG g_nCommands = sizeof(g_Commands)/sizeof(struct Command_t);

char* getcommand(int command)
{
	if(command <= 0 || command > g_nCommands)
		return NULL;

	return g_Commands[command-1].strCommand;
}

int getopt(int argc, char *argv[])
{
	char *pArgString = NULL;        /* where to start from next */
	int counter = 0;
	BOOL bFound = FALSE;


    if (pArgString == NULL) {
        /* we didn't leave off in the middle of an argv string */
        if (optind >= argc) {
            return EOF;             /* used up all command-line arguments */
        }

        /*---------------------------------------------------------------------
         * If the next argv[] is not an option, there can be no more options.
         *-------------------------------------------------------------------*/
        pArgString = argv[optind++]; /* set this to the next argument ptr */

        if (('/' != *pArgString) && /* doesn't start with a slash or a dash? */
            ('-' != *pArgString)) {
            --optind;               /* point to current arg once we're done */
            optarg = NULL;          /* no argument follows the option */
            return EOF;             /* used up all the command-line flags */
        }

        /* check for special end-of-flags markers */
        if ((strcmp(pArgString, "-") == 0) ||
            (strcmp(pArgString, "--") == 0)) {
            optarg = NULL;          /* no argument follows the option */
            return EOF;             /* encountered the special flag */
        }

        pArgString++;               /* look past the / or - */
    }

	for(counter = 0; counter < g_nCommands ; counter++){
		if (stricmp(g_Commands[counter].strCommand, pArgString) == 0){
			bFound = TRUE;
			if ( g_Commands[counter].bOption == TRUE ) {
				/*-------------------------------------------------------------
				* The argument string must be in the next argv.
				* But, what if there is none (bad input from the user)?
				* In that case, return the letter, and optarg as NULL.
				*-----------------------------------------------------------*/
				if (optind < argc)
					optarg = argv[optind++];
				else {
					optarg = NULL;
					return 0;
				}
			}
			else {
				/* it's not a colon, so just return the letter */
				optarg = NULL;          /* no argument follows the option */
			}
			return (int)g_Commands[counter].eCommand;    /* return the letter that matched */
		}
	}
	if(bFound == FALSE){
		/*---------------------------------------------------------------------
		* The letter on the command-line wasn't any good.
		*-------------------------------------------------------------------*/
		optarg = NULL;              /* no argument follows the option */
		return 0;
		/*---------------------------------------------------------------------
		* The letter on the command-line matches one we expect to see
		*-------------------------------------------------------------------*/
	}
	return 0;
}
