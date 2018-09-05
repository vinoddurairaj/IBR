/*
 * DbgDmp.h - Debug Dump Files
 *
 *	It manages traces and trace files.
 *
 *	The main buisness function is DbgDmpPrintf() wich output
 * messages to a dump file and optionnaly also to the screen.
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

#ifndef _DBG_DMP_H_
#define _DBG_DMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys\timeb.h>
#include <direct.h>	    // getcwd()
//#include <varargs.h>  // Unix

// PUBLIC:

void  DbgDmpPrintf      ( int piLevel, char* pcpFunc, char* pcpMsg, ... );
void  DbgDmpErrPrintf   ( char* pcpFunc, char* pcpMsg, ... );
void  DbgDmpInfPrintf   ( char* pcpFunc, char* pcpMsg, ... );
void  DbgDmpWrnPrintf   ( char* pcpFunc, char* pcpMsg, ... );

// DbgDmpSetLevel ()
//	If piLevel == 1 then only Errors And Criticals gets printed.
//	If piLevel == 4 then every thing from Criticals to Info 
// and Info4 gets printed
void  DbgDmpSetLevel       ( int piLevel );
void  DbgDmpSetFileMaxSize ( long plSz );
void  DbgDmpSetToScreen    ( int pbSetToScr ); // Output will go to screen as well.

// PRIVATE:

void  DbgDmpPrint           ( int piLevel, char* pcpFunc, char* pcpMsg, va_list* ppVaLst );
char* DbgDmpTime            ();
void  DbgDmpWrite           ( char* pcpMsg );
void  DbgDmpFileBackupRound ();
void  DbgDmpFileBuildPath   ( char* pcpFileDir, char* pcpFileName );
void  DbgDmpFileClose       ();
void  DbgDmpFileMng         ();
void  DbgDmpFileOpen        ();

#endif