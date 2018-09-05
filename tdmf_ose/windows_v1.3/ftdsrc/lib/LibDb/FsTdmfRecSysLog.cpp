// FsTdmfRecSysLog.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecSysLog.h"

CString FsTdmfRecSysLog::smszTblName = "t_SystemLog";	// DbVersion = "1.3.4"

CString FsTdmfRecSysLog::smszFldKa   = "Id_Ka";		    // DbVersion = "1.3.4"
CString FsTdmfRecSysLog::smszFldTs   = "Time_Stamp";	// DbVersion = "1.3.4"
CString FsTdmfRecSysLog::smszFldHostname = "Hostname";  // DbVersion = "1.3.4"
CString FsTdmfRecSysLog::smszFldUser = "UserName";		// DbVersion = "1.3.4"
CString FsTdmfRecSysLog::smszFldMsg  = "Message_Text";	// DbVersion = "1.3.4"


FsTdmfRecSysLog::FsTdmfRecSysLog ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{

} // FsTdmfRecSysLog::FsTdmfRecCmd ()


BOOL FsTdmfRecSysLog::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecSysLog::FtdFirst ()


BOOL FsTdmfRecSysLog::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecSysLog::FtdFirstFast ()


// If pszWhere is null then it is a sub select
CString FsTdmfRecSysLog::FtdFirstCurs ( CString pszWhere, CString pszSort )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return "";
	}
	else if ( !mpDb->FtdIsOpen() )
	{
		mpDb->FtdErr( 
			"FsTdmfRecSysLog::FtdFirstCurs(): Db not openned, cannot init"
		);
		return "";
	}

	CString lszSqlWhere;

	if ( pszWhere != "" )
	{
		lszSqlWhere.Format ( "WHERE " + pszWhere );
	}

	CString lszSort;

	if ( pszSort == "" )
	{
		lszSort = smszFldTs + " DESC ";
	}
	else
	{
		lszSort = pszSort;
	}

	CString lszSql;
	lszSql.Format ( 
		"SELECT * FROM " + mszTblName + " " + lszSqlWhere + " "
		"ORDER BY " + lszSort
	);

	return lszSql;

} // FsTdmfRecSysLog::FtdFirstCurs ()


CString FsTdmfRecSysLog::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"(\n"
			+ smszFldKa       + " INT          IDENTITY NOT NULL,\n"
			+ smszFldTs       + " DATETIME              NOT NULL,\n"
			+ smszFldHostname + " VARCHAR(32)           NOT NULL,\n"
			+ smszFldUser     + " VARCHAR(32)           NOT NULL,\n"
			+ smszFldMsg      + " VARCHAR(256)          NOT NULL,\n"
			
			"CONSTRAINT SystemLogPk PRIMARY KEY ( " + smszFldKa   + " )\n"
		")\n"
	);

	return pszCreateTbl;

} // FsTdmfRecSysLog::FtdGetCreate ()


FsTdmfRecSysLog::~FsTdmfRecSysLog ()
{

} // FsTdmfRecSysLog::~FsTdmfRecCmd ()


BOOL FsTdmfRecSysLog::FtdNew ( CString pszTimestamp, CString pszHostname, CString pszUser, CString pszMsg )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}
	else if ( !mpDb->FtdIsOpen() )
	{
		mpDb->FtdErr( 
			"FsTdmfRecSysLog::FtdNew(): "
			"Db not openned, cannot create new record with <" + pszMsg + ">"
		);

		return FALSE;
	}
	else try
	{
		pszMsg.Replace("'", "''");
		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " + smszFldTs + ", " + smszFldHostname + ", " + smszFldUser + ", " + smszFldMsg + " ) "
			"VALUES ( '" + pszTimestamp +  "', '" + pszHostname + "', '" + pszUser + "', '" + pszMsg + "' )"
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecSysLog::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"ORDER BY " + smszFldTs + " DESC "
	);

	return FtdOpen( lszSel );

} // FsTdmfRecSysLog::FtdNew ( CString pszName )


// Position the current record using the Key
BOOL FsTdmfRecSysLog::FtdPos ( int piKa )
{
	return FsTdmfRec::FtdPos( piKa, smszFldKa );

} // FsTdmfRecSysLog::FtdPos ()

