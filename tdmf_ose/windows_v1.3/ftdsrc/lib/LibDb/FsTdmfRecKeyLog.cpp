// FsTdmfRecKeyLog.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecKeyLog.h"

CString FsTdmfRecKeyLog::smszTblName = "t_KeyLog";	     // DbVersion = "1.3.4"

CString FsTdmfRecKeyLog::smszFldKa       = "Id_Ka";		 // DbVersion = "1.3.4"
CString FsTdmfRecKeyLog::smszFldTs       = "Time_Stamp"; // DbVersion = "1.3.4"
CString FsTdmfRecKeyLog::smszFldHostname = "Hostname";   // DbVersion = "1.3.4"
CString FsTdmfRecKeyLog::smszFldHostId   = "HostId";     // DbVersion = "1.3.4"
CString FsTdmfRecKeyLog::smszFldKey      = "RegKey";     // DbVersion = "1.3.4"
CString FsTdmfRecKeyLog::smszFldExpDate  = "Expiration"; // DbVersion = "1.3.4"


FsTdmfRecKeyLog::FsTdmfRecKeyLog ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{
} // FsTdmfRecKeyLog::FsTdmfRecCmd ()

BOOL FsTdmfRecKeyLog::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecKeyLog::FtdFirst ()

BOOL FsTdmfRecKeyLog::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecKeyLog::FtdFirstFast ()

// If pszWhere is null then it is a sub select
CString FsTdmfRecKeyLog::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecKeyLog::FtdFirstCurs(): Db not openned, cannot init"
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

} // FsTdmfRecKeyLog::FtdFirstCurs ()

CString FsTdmfRecKeyLog::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"(\n"
			+ smszFldKa       + " INT          IDENTITY NOT NULL,\n"
			+ smszFldTs       + " DATETIME              NOT NULL,\n"
			+ smszFldHostname + " VARCHAR(32)           NOT NULL,\n"
			+ smszFldHostId   + " DEC                   NOT NULL DEFAULT 0,\n"
			+ smszFldKey      + " VARCHAR(32)           NOT NULL,\n"
			+ smszFldExpDate  + " VARCHAR(40)           NOT NULL,\n"
			
			"CONSTRAINT KeyLogPk PRIMARY KEY ( " + smszFldKa   + " )\n"
		")\n"
	);

	return pszCreateTbl;

} // FsTdmfRecKeyLog::FtdGetCreate ()

FsTdmfRecKeyLog::~FsTdmfRecKeyLog ()
{

} // FsTdmfRecKeyLog::~FsTdmfRecCmd ()

BOOL FsTdmfRecKeyLog::FtdNew ( CString pszTimestamp, CString pszHostname, int nHostId, CString pszKey, CString pszExpDate )
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
			"FsTdmfRecKeyLog::FtdNew(): "
			"Db not openned, cannot create new record with <" + pszKey+ ">"
		);

		return FALSE;
	}
	else try
	{
		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " + smszFldTs + ", " + smszFldHostname + ", " + smszFldHostId + ", " + smszFldKey + ", " + smszFldExpDate + " ) "
			"VALUES ( '" + pszTimestamp +  "', '" + pszHostname + "', '%d', '" + pszKey + "', '" + pszExpDate + "' )",
			nHostId
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecKeyLog::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"ORDER BY " + smszFldTs + " DESC "
	);

	return FtdOpen( lszSel );

} // FsTdmfRecKeyLog::FtdNew ( CString pszName )

// Position the current record using the Key
BOOL FsTdmfRecKeyLog::FtdPos ( int piKa )
{
	return FsTdmfRec::FtdPos( piKa, smszFldKa );

} // FsTdmfRecKeyLog::FtdPos ()

