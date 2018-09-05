/*
 * DbgDmp.cpp - Debug Dump Files
 *
 *	See DbgDmp.h for comments.
 *
 * Copyright (c) 2002 Fujitsu Softek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

//#include "errors.h"
//#include "errmsg_list.h"

#include "DbgDmp.h"

// PRIVATE ///////////////////////////////////////////////////////////////////

#define DBGDMP_CHAR_TIME_SZ    16
#define DBGDMP_MAX_MSG       1024
#define DBGDMP_MAX_FILE_NAME   64
#define DBGDMP_MAX_FILE_DIR  1024
#define DBGDMP_MAX_FILE_PATH DBGDMP_MAX_FILE_NAME + DBGDMP_MAX_FILE_DIR + 1

//#define DBGDMP_DEFAULT_FILE_PATH ""

static char  gscaFilePath [DBGDMP_MAX_FILE_PATH + 1];
static char* gscpDefaultFileDir  = NULL;
static char* gscpFileName        = "TdmfDmp.Txt";
static FILE* gspFile             = NULL;

int gbToScr       =     1;
int giLevel       =     4;           // Level 0 - 4 will be printed
int glFileMaxSize = 10000; // 10 Kb

// PUBLIC ///////////////////////////////////////////////////////////////////

void DbgDmpSetFileDir ( const char* pcpFileDir )
{
	if ( pcpFileDir != NULL )
	{
		if ( strlen ( pcpFileDir ) >= DBGDMP_MAX_FILE_DIR )
		{
			printf ( "DbgDmpSetFileDir():Error File Dir Too Long\n" );
			return;
		}
	}

	gscpDefaultFileDir = (char*)pcpFileDir;

} // DbgDmpSetFileDir ()


void DbgDmpSetFileMaxSize ( long plSz )
{
	glFileMaxSize = plSz;

} // DbgDmpSetFileMaxSize ()


void DbgDmpSetFileName ( const char* pcpFileName )
{
	if ( pcpFileName != NULL )
	{
		if ( strlen ( pcpFileName ) >= DBGDMP_MAX_FILE_NAME )
		{
			printf ( "\nDbgDmpSetFileName():Error File Name Too Long\n" );
			return;
		}
	}

	gscpFileName = (char*)pcpFileName;

} // DbgDmpSetLevel ()


void DbgDmpSetLevel ( int piLevel )
{
	giLevel = piLevel;

} // DbgDmpSetLevel ()


void DbgDmpSetToScreen ( int pbSetToScr ) // Output will go to screen as well.
{
	gbToScr = pbSetToScr;

} // DbgDmpSetToScreen ()


// DbgDmpPrintf ()
//	piLevel : 1 == Error, piLevel : 2 == Warning, piLevel : 3 == Info
//  piLevel : 0 == Critical
void DbgDmpPrintf ( int piLevel, char* pcpFunc, char* pcpMsg, ... )
{
	va_list     lVaLst;

	va_start ( lVaLst, pcpMsg );

	DbgDmpPrint( piLevel, pcpFunc, pcpMsg, &lVaLst );

} // DbgDmpPrintf ()


// DbgDmpErrPrintf ()
//	Stub of DbgDmpPrintf ()
void DbgDmpErrPrintf ( char* pcpFunc, char* pcpMsg, ... )
{
	va_list     lVaLst;

	va_start ( lVaLst, pcpMsg );

	DbgDmpPrint ( 1, pcpFunc, pcpMsg, &lVaLst );

} // DbgDmpErrPrintf ()


// DbgDmpWrnPrintf ()
//	Stub of DbgDmpPrintf ()
void DbgDmpWrnPrintf ( char* pcpFunc, char* pcpMsg, ... )
{
	va_list     lVaLst;

	va_start ( lVaLst, pcpMsg );

	DbgDmpPrint ( 2, pcpFunc, pcpMsg, &lVaLst );

} // DbgDmpWrnPrintf ()


// DbgDmpInfPrintf ()
//	Stub of DbgDmpPrintf ()
void DbgDmpInfPrintf ( char* pcpFunc, char* pcpMsg, ... )
{
	va_list     lVaLst;

	va_start ( lVaLst, pcpMsg );

	DbgDmpPrint ( 3, pcpFunc, pcpMsg, &lVaLst );

} // DbgDmpInfPrintf ()


// PRIVATE ///////////////////////////////////////////////////////////////////

void DbgDmpPrint ( int piLevel, char* pcpFunc, char* pcpMsg, va_list* ppVaLst )
{
	static char lscaMsg [ DBGDMP_MAX_MSG + 1 ];
	static char lscaFmt [ DBGDMP_MAX_MSG + 1 ];
	int         liPrnCnt;
	char*       lcpLevel;

	if ( giLevel < piLevel )
	{
		return;
	}

	if ( piLevel == 0 )
	{
		lcpLevel = "CRITICAL";
	}
	else if ( piLevel == 1 )
	{
		lcpLevel = "ERROR";
	}
	else if ( piLevel == 2 )
	{
		lcpLevel = "WARNING";
	}
	else // piLevel >2
	{
		lcpLevel = "INFO";
	}

	liPrnCnt = _snprintf ( lscaFmt,
		DBGDMP_MAX_MSG, "[%s]%s:<%s>%s",
		DbgDmpTime(), lcpLevel, pcpFunc, pcpMsg
	);

	vsprintf ( lscaMsg, lscaFmt, *ppVaLst );

	if ( liPrnCnt >= DBGDMP_MAX_MSG )
	{
		liPrnCnt = _snprintf ( 
			lscaFmt, DBGDMP_MAX_MSG, 
			"DbgDmpPrintf Internal Error, Message Too Big From <%s>", 
			pcpFunc 
		);

		if ( liPrnCnt >= DBGDMP_MAX_MSG )
		{
			DbgDmpWrite ( "\nDbgDmpPrint() OverFlow\n" );
			return;
		}
	}

	DbgDmpWrite ( lscaMsg );

} // DbgDmpPrint ()


void DbgDmpFileClose ()
{
	if ( gspFile )
	{
		fclose ( gspFile );
	}

	gspFile = NULL;

} // DbgDmpFileClose ()


void DbgDmpFileBackupRound ()
{
	char lcaDrive [ _MAX_DRIVE ];
	char lcaDir   [ _MAX_DIR ];
	char lcaName  [ _MAX_FNAME ];
	char lcaName1 [ DBGDMP_MAX_FILE_NAME ];
	char lcaName2 [ DBGDMP_MAX_FILE_NAME ];
	char lcaName3 [ DBGDMP_MAX_FILE_NAME ];
	char lcaPath1 [ DBGDMP_MAX_FILE_PATH ];
	char lcaPath2 [ DBGDMP_MAX_FILE_PATH ];
	char lcaPath3 [ DBGDMP_MAX_FILE_PATH ];
	// Ext always .Txt
	long llStrLen;

	_splitpath( gscaFilePath, lcaDrive, lcaDir, lcaName, NULL );

	llStrLen = strlen (lcaDrive) + strlen (lcaDir) + strlen (lcaDir) + 5;
	if ( llStrLen >= DBGDMP_MAX_FILE_PATH )
	{
		printf ( "DbgDmpFileBackupRound():Path Too Long\n" );
		return;
	}

	sprintf ( lcaName1, "%.*s%s", DBGDMP_MAX_FILE_NAME - 5, lcaName, "Bak1" );
	sprintf ( lcaName2, "%.*s%s", DBGDMP_MAX_FILE_NAME - 5, lcaName, "Bak2" );
	sprintf ( lcaName3, "%.*s%s", DBGDMP_MAX_FILE_NAME - 5, lcaName, "Bak3" );

	_makepath( lcaPath1, lcaDrive, lcaDir, lcaName1, "Txt" );
	_makepath( lcaPath2, lcaDrive, lcaDir, lcaName2, "Txt" );
	_makepath( lcaPath3, lcaDrive, lcaDir, lcaName3, "Txt" );

	// It does not really matter if the rest always works or not
	remove ( lcaPath3 );
	rename ( lcaPath2, lcaPath3 );

	remove ( lcaPath2 );
	rename ( lcaPath1, lcaPath2 );

	remove ( lcaPath1 );
	DbgDmpFileClose();
	rename ( gscaFilePath, lcaPath1 );
	DbgDmpFileOpen();

} // DbgDmpFileBackupRound ()


void DbgDmpFileBuildPath ( char* pcpFileDir, char* pcpFileName )
{
	int  liSz;

	liSz = strlen ( pcpFileDir );

	if ( liSz > 0 )
	{
		if ( pcpFileDir [ liSz - 1 ] == '\\' )
		{
			sprintf ( 
				gscaFilePath, "%.*s%.*s", 
				DBGDMP_MAX_FILE_DIR,  pcpFileDir,
				DBGDMP_MAX_FILE_NAME, pcpFileName
			);
		}
		else
		{
			sprintf ( 
				gscaFilePath, "%.*s\\%.*s", 
				DBGDMP_MAX_FILE_DIR,  pcpFileDir,
				DBGDMP_MAX_FILE_NAME, pcpFileName
			);
		}
	}
	else
	{
		sprintf ( gscaFilePath, "%.*s", DBGDMP_MAX_FILE_NAME, pcpFileName );
	}

} // DbgDmpFileBuildPath ()


void DbgDmpFileOpen ()
{
	gspFile = fopen ( gscaFilePath, "a" );

	if( !gspFile )
	{
		printf ( 
			"DbgDmpFileOpen():Cant Open file %.*s",
			DBGDMP_MAX_FILE_PATH, gscaFilePath
		);
	}

} // DbgDmpFileOpen ()


void DbgDmpFileMng ()
{
	static int   lsbInit = 0;
	static char  lscaFileDir  [DBGDMP_MAX_FILE_DIR + 1];
	static char  lscaFileName [DBGDMP_MAX_FILE_DIR + 1];

	int          liFh; // File Handle
	long         llFs; // File Size

	if ( !lsbInit )
	{
		lsbInit = 1;
		if ( gscpDefaultFileDir )
		{
			sprintf ( lscaFileDir, "%.*s", DBGDMP_MAX_FILE_DIR, gscpDefaultFileDir );
			gscpDefaultFileDir = NULL;
		}
		else if( _getcwd ( lscaFileDir, DBGDMP_MAX_FILE_DIR ) == NULL )
		{
			printf ( "\nDbgDmpMngFile():Cant _getcwd() error\n" );
			return;
		}
	}
	else if ( gscpDefaultFileDir )
	{
		sprintf ( lscaFileDir, "%.*s", DBGDMP_MAX_FILE_DIR, gscpDefaultFileDir );
		gscpDefaultFileDir = 0;
		DbgDmpFileClose();
	}

	if ( gscpFileName )
	{
		sprintf ( lscaFileName, "%.*s", DBGDMP_MAX_FILE_NAME, gscpFileName );
		gscpFileName = NULL;
		DbgDmpFileClose();
	}

	if ( !gspFile )
	{
		DbgDmpFileBuildPath( lscaFileDir, lscaFileName );

		gspFile = fopen ( gscaFilePath, "a" );

		if( !gspFile )
		{
			printf ( 
				"DbgDmpFileOpen():Cant Open file %.*s",
				DBGDMP_MAX_FILE_PATH, gscaFilePath
			);

			return;
		}
	}

	liFh = _fileno     ( gspFile );
	llFs = _filelength ( liFh );

	if ( llFs > glFileMaxSize )
	{
		DbgDmpFileBackupRound();
	}

} // DbgDmpFileMng ()


char* DbgDmpTime ()
{
	static char   lscaTime [ DBGDMP_CHAR_TIME_SZ ];
	struct _timeb lTimeBuff;
	char*         lcpTimeLine;

	_ftime ( &lTimeBuff );
	lcpTimeLine = ctime ( &(lTimeBuff.time) );
	_snprintf ( lscaTime, DBGDMP_CHAR_TIME_SZ, "%8.8s.%03hu", &lcpTimeLine [11], lTimeBuff.millitm );

	return lscaTime;

} // DbgDmpTime ()


void DbgDmpWrite ( char* pcpMsg )
{
	if ( gbToScr )
	{
		printf ( pcpMsg );
	}

	DbgDmpFileMng();

	if ( gspFile )
	{
		fprintf ( gspFile, pcpMsg );
	}
	else
	{
		printf ( "\nDbgDmpWrite() Cant write to dumpfile\n" );
	}

} // DbgDmpWrite ()






