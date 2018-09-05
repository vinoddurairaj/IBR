// FsTdmfRecDom.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecDom.h"

CString FsTdmfRecDom::smszTblName  = "t_Domain";	// DbVersion = "1.3.2"

CString FsTdmfRecDom::smszFldKa    = "Id_Ka";		// DbVersion = "1.3.2"
CString FsTdmfRecDom::smszFldName  = "DomainName";	// DbVersion = "1.3.2"
CString FsTdmfRecDom::smszFldDesc  = "Description";	// DbVersion = "1.3.2"
CString FsTdmfRecDom::smszFldState = "State";		// DbVersion = "1.3.2"


FsTdmfRecDom::FsTdmfRecDom ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{

} // FsTdmfRecDom::FsTdmfRecDom ()


BOOL FsTdmfRecDom::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecDom::FtdFirst ()


BOOL FsTdmfRecDom::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecDom::FtdFirstFast ()


// If pszWhere is null then it is a sub select
CString FsTdmfRecDom::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecDom::FtdFirstCurs(): Db not openned, cannot init"
		);
		return "";
	}

	CString lszSqlWhere;

	if ( pszWhere != "" )
	{
		//lszSqlWhere.Format ( "WHERE " + pszWhere );
		lszSqlWhere = "WHERE " + pszWhere;
	}

	CString lszSort;

	if ( pszSort == "" )
	{
		lszSort = smszFldName;
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

} // FsTdmfRecDom::FtdFirstCurs ()


CString FsTdmfRecDom::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"(\n"
			+ smszFldKa    + " INT         IDENTITY NOT NULL,\n"
			+ smszFldName  + " VARCHAR(30)          NOT NULL,\n"
			+ smszFldDesc  + " VARCHAR(80)          NULL,\n"
			+ smszFldState + " INT                  NOT NULL DEFAULT 0,\n"

			"CONSTRAINT DomainPk PRIMARY KEY ( " + smszFldKa   + " ),\n"
			"CONSTRAINT DomainUk UNIQUE      ( " + smszFldName + " )\n"
		")\n"
	);

	return pszCreateTbl;

} // FsTdmfRecDom::FtdGetCreate ()


FsTdmfRecDom::~FsTdmfRecDom ()
{

} // FsTdmfRecDom::~FsTdmfRecDom ()


BOOL FsTdmfRecDom::FtdNew ( CString pszName )
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
			"FsTdmfRecDom::FtdNew(): "
			"Db not openned, cannot create new record with <" + pszName + ">"
		);

		return FALSE;
	}
	else try
	{
		pszName.Replace("'", "''");

		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " + smszFldName + " ) "
			"VALUES ( '" + pszName + "' )"
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecDom::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldName + " = '" + pszName + "'"
	);

	return FtdOpen( lszSel );

} // FsTdmfRecDom::FtdNew ( CString pszName )


BOOL FsTdmfRecDom::FtdPos ( CString pszName )
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
			"FsTdmfRecDom::FtdPos(): "
			"Db not openned, cannot pos record with <" + pszName + ">"
		);

		return FALSE;
	}

	pszName.Replace("'", "''");
	pszName.Replace("\"", "\"\"");
	pszName.Replace("%", "%%");

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldName + " = '" + pszName + "'"
	);

	return FtdOpen( lszSel );

} // FsTdmfRecDom::FtdPos ()


// Position the current record using the Key
BOOL FsTdmfRecDom::FtdPos ( int piKa )
{
	return FsTdmfRec::FtdPos( piKa, smszFldKa );

} // FsTdmfRecDom::FtdPos ()


BOOL FsTdmfRecDom::FtdRename ( CString pszOldName, CString pszNewName )
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
			"FsTdmfRecDom::FtdRename(): "
			"Db Not Openned, cannot rename record with <" + pszOldName + ">"
		);

		return FALSE;
	}
	else if ( !FtdPos( pszOldName ) )
	{
		mpDb->FtdErr( 
			"FsTdmfRecDom::FtdRename(): "
			"Cannot rename record <" + pszOldName + ">"
		);

		return FALSE;
	}
	else if ( !FtdUpd( smszFldName, pszNewName ) )
	{
		mpDb->FtdErr( 
			"FsTdmfRecDom::FtdRename(): "
			"Cannot rename record with <" + pszNewName + ">"
		);

		return FALSE;
	}

	return TRUE;

} // FsTdmfRecDom::FtdRename ()


BOOL FsTdmfRecDom::FtdUpd ( CString pszFldName, CString pszVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, pszVal, smszFldKa );

} // FsTdmfRecDom::FtdUpd ()


BOOL FsTdmfRecDom::FtdDelete ( int piKa )
{
	CString strWhere;
	strWhere.Format ( smszFldKa + " = %ld", piKa );
	return FsTdmfRec::FtdDelete( strWhere );
}