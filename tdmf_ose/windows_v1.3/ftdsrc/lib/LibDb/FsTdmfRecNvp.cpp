// FsTdmfRecNvp.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecNvp.h"

//CString FsTdmfRecNvp::smszTblName      = "t_Nvp";
CString FsTdmfRecNvp::smszTblName      = "t_SystemParameters";	// DbVersion = "1.3.2"

CString FsTdmfRecNvp::smszFldKa         = "Id_Ka";				// DbVersion = "1.3.2"
CString FsTdmfRecNvp::smszFldName       = "NvpName";			// DbVersion = "1.3.2"
CString FsTdmfRecNvp::smszFldVal	    = "NvpVal";				// DbVersion = "1.3.2"
CString FsTdmfRecNvp::smszFldDesc       = "NvpDesc";			// DbVersion = "1.3.2"


FsTdmfRecNvp::FsTdmfRecNvp ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{

} // FsTdmfRecNvp::FsTdmfRecNvp ()


BOOL FsTdmfRecNvp::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecNvp::FtdFirst ()


BOOL FsTdmfRecNvp::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecNvp::FtdFirstFast ()


// If pszWhere is null then it is a sub select
CString FsTdmfRecNvp::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecNvp::FtdFirstCurs(): Db not openned, cannot init"
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
		lszSort = smszFldKa;
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

} // FsTdmfRecNvp::FtdFirstCurs ()


CString FsTdmfRecNvp::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"(\n"
			+ smszFldKa         + " INT              IDENTITY NOT NULL,\n"
			+ smszFldName       + " VARCHAR (  30 )           NOT NULL,\n"
			+ smszFldVal        + " VARCHAR ( 256 ),\n"
			+ smszFldDesc       + " VARCHAR ( 256 ),\n"

			"CONSTRAINT NvpIdPk   PRIMARY KEY ( " + smszFldKa   + " ),\n"
			"CONSTRAINT NvpNameUk UNIQUE      ( " + smszFldName + " )\n"
		")\n"
	);

	return pszCreateTbl;

} // FsTdmfRecNvp::FtdGetCreate ()


FsTdmfRecNvp::~FsTdmfRecNvp ()
{

} // FsTdmfRecNvp::~FsTdmfRecNvp ()

BOOL FsTdmfRecNvp::FtdNew ( CString pszName )
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
			"FsTdmfRecNvp::FtdNew(): "
			"Db not openned, cannot create new record with <" + pszName + ">"
		);

		return FALSE;
	}

	try
	{
		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " + smszFldName + " ) "
			"VALUES ( '" + pszName + "' )"
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecNvp::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldName + " = '" + pszName + "'"
	);

	return FtdOpen( lszSel );

} // FsTdmfRecNvp::FtdNew ( CString pszName )


BOOL FsTdmfRecNvp::FtdNew ( CString pszName, CString pszVal )
{
	if ( !FtdNew( pszName ) )
	{
		return FALSE;
	}

	return FtdUpd( FsTdmfRecNvp::smszFldVal, pszVal );

} // FsTdmfRecNvp::FtdNew ()


// Position the current t_nvp record using the name
BOOL FsTdmfRecNvp::FtdPos ( CString pszName )
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
			"FsTdmfRecNvp::FtdPos(): "
			"Db not openned, cannot pos record with <" + pszName + ">"
		);

		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldName + " = '" + pszName + "'"
	);

	return FtdOpen( lszSel );

} // FsTdmfRecNvp::FtdPos ()


// Position the current t_nvp record using the Nvp K AutoNumber
BOOL FsTdmfRecNvp::FtdPos ( int piKa )
{
	return FsTdmfRec::FtdPos( piKa, smszFldKa );

} // FsTdmfRecNvp::FtdPos ()


BOOL FsTdmfRecNvp::FtdUpd ( CString pszFldName, CString pszVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, pszVal, smszFldKa );

} // FsTdmfRecNvp::FtdUpd ()


BOOL FsTdmfRecNvp::FtdUpdNvp ( CString pszName, CString pszVal )
{
	if ( !FtdPos( pszName ) )
	{
		return FALSE;
	}

	return FsTdmfRec::FtdUpd( FsTdmfRecNvp::smszFldVal, pszVal, smszFldKa );

} // FsTdmfRecNvp::FtdUpdNvp ()


BOOL FsTdmfRecNvp::FtdUpdNvp ( CString pszName, int piVal )
{
	return FtdUpdNvp( pszName, (__int64)piVal );

} // FsTdmfRecNvp::FtdUpdNvp ()



BOOL FsTdmfRecNvp::FtdUpdNvp ( CString pszName, __int64 pjVal )
{
	if ( !FtdPos( pszName ) )
	{
		return FALSE;
	}

	CString lszVal;
	lszVal.Format ( "%I64d", pjVal );

	return FsTdmfRec::FtdUpd( FsTdmfRecNvp::smszFldVal, lszVal, smszFldKa );

} // FsTdmfRecNvp::FtdUpdNvp ()



