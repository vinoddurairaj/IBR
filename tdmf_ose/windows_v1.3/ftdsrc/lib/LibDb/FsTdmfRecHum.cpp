// FsTdmfRecHum.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecHum.h"

CString FsTdmfRecHum::smszTblName      = "t_Peoples";	// DbVersion = "1.3.2"

CString FsTdmfRecHum::smszFldKa        = "Id_Ka";		// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldSubFk     = "Id_Fk";		// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldNameFirst = "NameFirst";	// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldNameMid   = "NameMid";		// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldNameLast  = "NameLast";	// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldJobTitle  = "JobTitle";	// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldDep       = "Department";	// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldTel       = "Telephone";	// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldCel       = "Cellular";	// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldPager     = "Pager";		// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldEmail     = "Email";		// DbVersion = "1.3.2"
CString FsTdmfRecHum::smszFldNotes     = "Notes";		// DbVersion = "1.3.2"

FsTdmfRecHum::FsTdmfRecHum ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{

} // FsTdmfRecHum::FsTdmfRecHum ()


BOOL FsTdmfRecHum::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecHum::FtdFirst ()


BOOL FsTdmfRecHum::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecHum::FtdFirstFast ()


// If pszWhere is null then it is a sub select
CString FsTdmfRecHum::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecHum::FtdFirstCurs(): Db not openned, cannot init"
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
		lszSort = smszFldNameFirst;
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

} // FsTdmfRecHum::FtdFirstCurs ()


CString FsTdmfRecHum::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"( "
			+ smszFldKa        + " INT                 IDENTITY NOT NULL,\n"
			+ smszFldSubFk     + " INT,\n"
			+ smszFldNameFirst + " VARCHAR ( 30)           NOT NULL,\n"
			+ smszFldNameMid   + " CHAR                    NOT NULL,\n"
			+ smszFldNameLast  + " VARCHAR ( 30)           NOT NULL,\n"

			+ smszFldJobTitle  + " VARCHAR ( 30),\n"
			+ smszFldDep       + " VARCHAR ( 30),\n"
			+ smszFldTel       + " VARCHAR ( 30),\n"
			+ smszFldCel       + " VARCHAR ( 30),\n"
			+ smszFldPager     + " VARCHAR ( 30),\n"
			+ smszFldEmail     + " VARCHAR ( 30),\n"

			+ smszFldNotes     + " VARCHAR (256),\n"

			"CONSTRAINT PeoplePk PRIMARY KEY ( " + smszFldKa + " ),\n"
			"CONSTRAINT PeopleFk FOREIGN KEY ( " + smszFldSubFk + " )\n"
				"REFERENCES t_Peoples ( " + smszFldKa + " ),\n"
			"CONSTRAINT PeopleUk UNIQUE (\n" 
				+ smszFldNameFirst + ", " 
				+ smszFldNameMid +   ", " 
				+ smszFldNameLast + 
			"),\n"
		")\n"
	);

	return pszCreateTbl;

} // FsTdmfRecHum::FtdGetCreate ()


FsTdmfRecHum::~FsTdmfRecHum ()
{

} // FsTdmfRecHum::~FsTdmfRecHum ()

BOOL FsTdmfRecHum::FtdNew ( CString pszName )
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
			"FsTdmfRecHum::FtdNew(): "
			"Db not openned, cannot create new record with <" + pszName + ">"
		);

		return FALSE;
	}

	try
	{
		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " 
				+ smszFldNameFirst + ", "
				+ smszFldNameMid   + ", "
				+ smszFldNameLast  +
			" ) "
			"VALUES ( '" + pszName + "', '.', '.' )"
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecHum::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldNameFirst + " = '" + pszName + "'"
	);

	return FtdOpen( lszSel );

} // FsTdmfRecHum::FtdNew ( CString pszName )


// Position the current record using the First Name
BOOL FsTdmfRecHum::FtdPos ( CString pszName )
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
			"FsTdmfRecHum::FtdPos(): "
			"Db not openned, cannot pos record with <" + pszName + ">"
		);

		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldNameFirst + " = '" + pszName + "'"
	);

	return FtdOpen( lszSel );

} // FsTdmfRecHum::FtdPos ()


// Position the current record using the Nvp K AutoNumber
BOOL FsTdmfRecHum::FtdPos ( int piKa )
{
	return FsTdmfRec::FtdPos( piKa, smszFldKa );

} // FsTdmfRecHum::FtdPos ()


BOOL FsTdmfRecHum::FtdUpd ( CString pszFldName, CString pszVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, pszVal, smszFldKa );

} // FsTdmfRecHum::FtdUpd ()


BOOL FsTdmfRecHum::FtdUpd ( CString pszFldName, int piVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, piVal, smszFldKa );

} // FsTdmfRecHum::FtdUpd ()


