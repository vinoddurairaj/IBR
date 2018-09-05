// FsTdmfRecPair.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecPair.h"
#include "FsTdmfRecLgGrp.h"

CString FsTdmfRecPair::smszTblName     = "t_ReplicationPair";	// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldIk       = "Ik"; // Internal Key	// DbVersion = "1.3.2"

CString FsTdmfRecPair::smszFldPairId   = "PairId";				// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldGrpFk    = "Group_Fk";			// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldSrcFk    = "Source_Fk";			// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldNotes    = "Notes";				// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldState    = "State";				// DbVersion = "1.3.2"

CString FsTdmfRecPair::smszFldSrcDisk       = "SrcDisk";		// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldSrcDriveId    = "SrcDriveId";		// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldSrcStartOff   = "SrcStartOffset";	// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldSrcLength     = "SrcLength";		// DbVersion = "1.3.2"

CString FsTdmfRecPair::smszFldTgtDisk       = "TgtDisk";		// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldTgtDriveId    = "TgtDriveId";		// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldTgtStartOff   = "TgtStartOffset";	// DbVersion = "1.3.2"
CString FsTdmfRecPair::smszFldTgtLength     = "TgtLength";		// DbVersion = "1.3.2"

CString FsTdmfRecPair::smszFldFS       = "FileSystem";			// DbVersion = "1.3.2"

FsTdmfRecPair::FsTdmfRecPair ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{

} // FsTdmfRecPair::FsTdmfRecPair ()


BOOL FsTdmfRecPair::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecPair::FtdFirst ()


BOOL FsTdmfRecPair::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecPair::FtdFirstFast ()


// If pszWhere is null then it is a sub select
CString FsTdmfRecPair::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecPair::FtdFirstCurs(): Db not openned, cannot init"
		);
		return "";
	}

	CString lszSqlWhere = "WHERE ";

	if ( pszWhere == "" )
	{
		CString lszSrvId = 
			mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldSrcFk );
		CString lszGrpId = 
			mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldLgGrpId );

		if ( lszSrvId == "" || lszGrpId == "" )
		{
			mpDb->FtdErr( 
				"FsTdmfRecPair::FtdFirstCurs(): No current Logical Group record" 
			);
			return "";
		}

		lszSqlWhere += 
			smszFldSrcFk + " = " + lszSrvId + " "
			"AND " + smszFldGrpFk + " = " + lszGrpId;
	}
	else
	{
		lszSqlWhere += pszWhere;
	}

	CString lszSort;

	if ( pszSort == "" )
	{
		lszSort = smszFldPairId;
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

} // FsTdmfRecPair::FtdFirstCurs ()


CString FsTdmfRecPair::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"( "
			+ smszFldIk       + " INT          IDENTITY NOT NULL,\n"
			+ smszFldPairId   + " SMALLINT              NOT NULL,\n"
			+ smszFldGrpFk    + " SMALLINT              NOT NULL,\n"
			+ smszFldSrcFk    + " INT                   NOT NULL,\n"
			+ smszFldNotes    + " VARCHAR(256),\n"
			+ smszFldState    + " INT                   NOT NULL DEFAULT 0,\n"

			+ smszFldFS       + " VARCHAR(16),\n"

			+ smszFldSrcDisk        + " VARCHAR(128),\n"
			+ smszFldSrcDriveId     + " DECIMAL,\n"
			+ smszFldSrcStartOff    + " DECIMAL,\n"
			+ smszFldSrcLength      + " DECIMAL,\n"

			+ smszFldTgtDisk        + " VARCHAR(128),\n"
			+ smszFldTgtDriveId     + " DECIMAL,\n"
			+ smszFldTgtStartOff    + " DECIMAL,\n"
			+ smszFldTgtLength      + " DECIMAL,\n"

			"CONSTRAINT RepPairPk "
				"PRIMARY KEY ( "
					+ smszFldPairId + ", " 
					+ smszFldGrpFk + ", " 
					+ smszFldSrcFk + " "
				"),\n"

			"CONSTRAINT RepPairFk FOREIGN KEY ( "
					+ smszFldGrpFk +  ", "
					+ smszFldSrcFk + 
				")\n"
				"REFERENCES " + FsTdmfRecLgGrp::smszTblName + " ( "
					+ FsTdmfRecLgGrp::smszFldLgGrpId +  ", "
					+ FsTdmfRecLgGrp::smszFldSrcFk   + 
				")\n"
		")\n"
	);

	return pszCreateTbl;

} // FsTdmfRecPair::FtdGetCreate ()


FsTdmfRecPair::~FsTdmfRecPair ()
{

} // FsTdmfRecPair::~FsTdmfRecPair ()

// pszName is PairId
BOOL FsTdmfRecPair::FtdNew ( CString pszPairId )
{
	return FtdNew( atoi ( pszPairId ) );

} // FsTdmfRecPair::FtdNew ( CString pszName )


BOOL FsTdmfRecPair::FtdNew ( int piPairId )
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
			"FsTdmfRecPair::FtdNew(): "
			"Db not openned, cannot create new record with <%ld>",
			piPairId
		);

		return FALSE;
	}

	CString lszSrvFk = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldSrcFk );
	CString lszGrpFk = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldLgGrpId );

	if ( lszSrvFk == "" || lszGrpFk == "" )
	{
		mpDb->FtdErr( 
			"FsTdmfRecPair::FtdNew(): No current " 
			+ FsTdmfRecPair::smszTblName + " record" 
		);
		return FALSE;
	}
	else try
	{
		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " 
				+ smszFldSrcFk + ", " + smszFldGrpFk + ", " + smszFldPairId +
			" ) "
			"VALUES ( " + lszSrvFk +  ", " + lszGrpFk + ", %ld )",
			piPairId
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecPair::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldSrcFk + " = " + lszSrvFk + " "
		"AND "   + smszFldGrpFk + " = " + lszGrpFk + " "
		"AND "   + smszFldPairId + " = %ld",	piPairId
	);

	return FtdOpen( lszSel );

} // FsTdmfRecPair::FtdNew ( int )


BOOL FsTdmfRecPair::FtdPos ( CString pszName )
{
	return FtdPos( atoi ( pszName ) );

} // FsTdmfRecPair::FtdPos ()


BOOL FsTdmfRecPair::FtdPos ( int piId )
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
			"FsTdmfRecPair::FtdPos(): "
			"Db not openned, cannot pos record with <%ld>",
			piId
		);

		return FALSE;
	}

	CString lszSrvFk = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldSrcFk );
	CString lszGrpFk = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldLgGrpId );

	if ( lszSrvFk == "" || lszGrpFk == "" )
	{
		mpDb->FtdErr( 
			"FsTdmfRecPair::FtdNew(): No current " 
			+ FsTdmfRecPair::smszTblName + " record" 
		);
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldSrcFk + " = " + lszSrvFk + " "
		"AND "   + smszFldGrpFk + " = " + lszGrpFk + " "
		"AND "   + smszFldPairId + " = %ld",	piId
	);

	return FtdOpen( lszSel );

} // FsTdmfRecPair::FtdPos ()


BOOL FsTdmfRecPair::FtdUpd ( CString pszFldName, CString pszVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, pszVal, smszFldIk );

} // FsTdmfRecPair::FtdUpd ()


BOOL FsTdmfRecPair::FtdUpd ( CString pszFldName, int piVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, piVal, smszFldIk );

} // FsTdmfRecPair::FtdUpd ()

BOOL FsTdmfRecPair::FtdUpd ( CString pszFldName, __int64 piVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, piVal, smszFldIk );

} // FsTdmfRecPair::FtdUpd ()


