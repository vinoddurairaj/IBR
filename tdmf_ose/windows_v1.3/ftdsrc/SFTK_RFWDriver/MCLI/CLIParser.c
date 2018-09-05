/*****************************************************************************
 *                                                                           *
 *  This software  is the licensed software of Fujitsu Software              *
 *  Technology Corporation                                                   *
 *                                                                           *
 *  Copyright (c) 2004 by Softek Storage Technology Corporation              *
 *                                                                           *
 *  THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF            *
 *  FUJITSU SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED             *
 *  UNDER LICENSE FROM FUJITSU SOFTWARE TECHNOLOGY CORPORATION               *
 *                                                                           *
 *****************************************************************************

 *****************************************************************************
 *                                                                           *
 *  Revision History:                                                        *
 *                                                                           *
 *  Date        By              Change                                       *
 *  ----------- --------------  -------------------------------------------  *
 *  04-27-2004  Parag Sanghvi   Initial version.                             *
 *                                                                           *
 *                                                                           *
 *****************************************************************************/
#include "CLI.h"

// Globals used in this file only
PCMD_LINE_STRINGS	gCmdStrs = NULL;
PCMD_LINE_OPT		gOptions = NULL;
HANDLE				gHandle = NULL;

//
// CLIParseCmdLine
//
VOID	CLIParseCmdLine(IN PCMD_LINE_STRINGS CmdStrs,
						IN PCMD_LINE_CMD Cmds,
						IN PCMD_LINE_OPT Opts,
						IN DWORD argc,
						IN CHAR **argv)
{
	DWORD	i;
	DWORD	x;

	// Check parameters -  need CmdStrs and Cmds to continue
	if(!CmdStrs || !Cmds || !CmdStrs->CmdPrefix){
		return;
	}

	// Set global CMD_LINE_STRINGS pointer
	gCmdStrs = CmdStrs;

	// Set global CMD_LINE_OPT pointer
	gOptions = Opts;

	// Loop through the argvs
	for(i=0; i<argc; i++){

		// Check for command prefix string
		if(!_strnicmp(argv[i], gCmdStrs->CmdPrefix, strlen(gCmdStrs->CmdPrefix))){

			// Found command prefix string, now look for a match from the command structure
			x = 0;
			do{
				if(!Cmds[x].CmdStr){
					break;
				}
				// Check for match
				if(!_stricmp(&argv[i][strlen(gCmdStrs->CmdPrefix)], Cmds[x].CmdStr)){

					// Found a match, now set options for this command
					SetOptions(argc, argv, i + 1);
					
					// Call the command function
					Cmds[x].Func();

					// break;
				}

				// Check for last entry
				if(Cmds[x].Flags & CMD_FLAG_LAST_ENTRY){
					break;
				}

				// Increment the command structure index
				x++;

			}while(TRUE);
		}
	}
} // CLIParseCmdLine()

//
// CLIGetOpt
//
PCMD_LINE_OPT	CLIGetOpt(IN PCHAR OptStr)
{
	DWORD	i;

	// Loop through the global CMD_LINE_OPT structure looking for a match
	i = 0;
	while(gOptions[i].OptStr){
		// Check for match
		if(!_stricmp(OptStr, gOptions[i].OptStr)){
			// Return the pointer to the CMD_LINE_OPT structure
			return(&gOptions[i]);
		}
		// Increment the index
		i++;
	}
	// Didn't find the option
	return(NULL);
} // CLIGetOpt()

VOID	CLIGetXY(	OUT PDWORD Col,
					OUT PDWORD Row)
{
	CONSOLE_SCREEN_BUFFER_INFO 	Info;
	
	// Get the console handle
	if(!gHandle){
		gHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	// Clear the 'result' in case of error
	Info.dwCursorPosition.Y = 0;
	Info.dwCursorPosition.X = 0;

	// Get the info
	GetConsoleScreenBufferInfo(gHandle, &Info);

	// Set the data
	*Col = Info.dwCursorPosition.X;
	*Row = Info.dwCursorPosition.Y;
} // CLIGetXY()

VOID	CLISetXY(	IN DWORD Col,
					IN DWORD Row)
{
	COORD	Coord;

	// Get the console handle
	if(!gHandle){
		gHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	// Set the data
	Coord.X = (SHORT)Col;
	Coord.Y = (SHORT)Row;

	// Set the info
	SetConsoleCursorPosition(gHandle, Coord);
} // CLISetXY()

BOOL	CLIGetConsoleInfo(IN PCONSOLE_SCREEN_BUFFER_INFO ConsoleInfo)
{
	// Get the console handle
	if(!gHandle){
		gHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	// Get the info
	return(GetConsoleScreenBufferInfo(gHandle, ConsoleInfo));
} // CLIGetConsoleInfo()

//
// SetOptions
//
// This function sets all options on the command line for the current command.
//
static
VOID
SetOptions(DWORD argc, char **argv, DWORD Index)
{
	DWORD	i;
	DWORD	x;

	// Verify global Options array exists
	if(!gOptions || !gCmdStrs->OptPrefix || !gCmdStrs->DelString){
		return;
	}

	// Loop through the argv[] array for argc times
	for(i=Index; i<argc; i++){

		// Check for a command
		if(!_strnicmp(argv[i], gCmdStrs->CmdPrefix, strlen(gCmdStrs->CmdPrefix))){
			return;
		}

		// Check for option
		if(!_strnicmp(argv[i], gCmdStrs->OptPrefix, strlen(gCmdStrs->OptPrefix))){

			// Loop through the defined options looking for a match to the current argv
			x = 0;
			do{
				// Check for string
				if(!gOptions[x].OptStr){
					break;
				}

				// Check for a match
				if(!_strnicmp(&argv[i][strlen(gCmdStrs->OptPrefix)], 
							  gOptions[x].OptStr, strlen(gOptions[x].OptStr))){

					// Set the present flag
					gOptions[x].Present = TRUE;

					// Is the argv without an immediate (within the argv) value?
					if(strlen(&argv[i][strlen(gCmdStrs->OptPrefix)]) == strlen(gOptions[x].OptStr)){

						// No immediate value - check for more argvs
						if((i+1) < argc){

							// Check if next argv is not a command or option
							if((_strnicmp(argv[i+1], gCmdStrs->CmdPrefix,
								strlen(gCmdStrs->CmdPrefix))) &&
							   (_strnicmp(argv[i+1], gCmdStrs->OptPrefix,
								   strlen(gCmdStrs->OptPrefix)))){

								// Next argv is a value for the option - convert and set
								switch(gOptions[x].Type){
									case INT_OPT:{
										gOptions[x].Int = strtoul(argv[i+1], NULL, 0);
										break;
									}
									case FLOAT_OPT:{
										gOptions[x].Float = (float)strtod(argv[i+1], NULL);
										break;
									}
									case DOUBLE_OPT:{
										gOptions[x].Double = strtod(argv[i+1], NULL);
										break;
									}
									case STRING_OPT:{
										gOptions[x].String = argv[i+1];
										break;
									}
								}
								// Set the value flag
								gOptions[x].Value = TRUE;
							}else{ // Next argv is a command or option
								gOptions[x].Value = FALSE;
							}
						}else{ // No more argvs
							gOptions[x].Value = FALSE;
						}
					}else{ // Option has immediate value
						
						// Check for delete string first
						if(!_strnicmp(&argv[i][strlen(gCmdStrs->OptPrefix) + strlen(gOptions[x].OptStr)], 
									  gCmdStrs->DelString,
									  strlen(gCmdStrs->DelString))){

							// Clear the present flag
							gOptions[x].Present = FALSE;
						}else{ // Not delete string

							// Immediate is a value for the option - convert and set
							switch(gOptions[x].Type){
								case INT_OPT:{
									gOptions[x].Int = strtoul(&argv[i][strlen(gCmdStrs->OptPrefix) + strlen(gOptions[x].OptStr)], NULL, 0);
									break;
								}
								case FLOAT_OPT:{
									gOptions[x].Float = (float)strtod(&argv[i][strlen(gCmdStrs->OptPrefix) + strlen(gOptions[x].OptStr)], NULL);
									break;
								}
								case DOUBLE_OPT:{
									gOptions[x].Double = strtod(&argv[i][strlen(gCmdStrs->OptPrefix) + strlen(gOptions[x].OptStr)], NULL);
									break;
								}
								case STRING_OPT:{
									gOptions[x].String = &argv[i][strlen(gCmdStrs->OptPrefix) + strlen(gOptions[x].OptStr)];
									break;
								}
							}
							// Set the value flag
							gOptions[x].Value = TRUE;
						}
					}
				}

				// Check for last entry
				if(gOptions[x].Flags & OPT_FLAG_LAST_ENTRY){
					break;
				}

				x++;

			}while(TRUE);
		}
	}
} // SetOptions()

