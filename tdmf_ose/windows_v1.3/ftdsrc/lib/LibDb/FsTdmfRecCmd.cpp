// FsTdmfRecCmd.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecCmd.h"

CString FsTdmfRecCmd::smszTblName = "t_Command";	// DbVersion = "1.3.2"

CString FsTdmfRecCmd::smszFldKa   = "Id_Ka";		// DbVersion = "1.3.2"
CString FsTdmfRecCmd::smszFldCmd  = "Command_Text";	// DbVersion = "1.3.2"
CString FsTdmfRecCmd::smszFldTs   = "Time_Stamp";	// DbVersion = "1.3.2"


FsTdmfRecCmd::FsTdmfRecCmd ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{

} // FsTdmfRecCmd::FsTdmfRecCmd ()


BOOL FsTdmfRecCmd::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecCmd::FtdFirst ()


BOOL FsTdmfRecCmd::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecCmd::FtdFirstFast ()


// If pszWhere is null then it is a sub select
CString FsTdmfRecCmd::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecCmd::FtdFirstCurs(): Db not openned, cannot init"
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

} // FsTdmfRecCmd::FtdFirstCurs ()


CString FsTdmfRecCmd::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"(\n"
			+ smszFldKa  + " INT          IDENTITY NOT NULL,\n"
			+ smszFldCmd + " VARCHAR(256)          NOT NULL,\n"
			+ smszFldTs  + " TIMESTAMP,\n"

			"CONSTRAINT CommandPk PRIMARY KEY ( " + smszFldKa   + " )\n"
		")\n"
	);

	return pszCreateTbl;

} // FsTdmfRecCmd::FtdGetCreate ()


FsTdmfRecCmd::~FsTdmfRecCmd ()
{

} // FsTdmfRecCmd::~FsTdmfRecCmd ()


BOOL FsTdmfRecCmd::FtdNew ( CString pszCmd )
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
			"FsTdmfRecCmd::FtdNew(): "
			"Db not openned, cannot create new record with <" + pszCmd + ">"
		);

		return FALSE;
	}
	else try
	{
		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " + smszFldCmd + " ) "
			"VALUES ( '" + pszCmd + "' )"
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecCmd::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"ORDER BY " + smszFldTs + " DESC "
	);

	return FtdOpen( lszSel );

} // FsTdmfRecCmd::FtdNew ( CString pszName )


// Position the current record using the Key
BOOL FsTdmfRecCmd::FtdPos ( int piKa )
{
	return FsTdmfRec::FtdPos( piKa, smszFldKa );

} // FsTdmfRecCmd::FtdPos ()

