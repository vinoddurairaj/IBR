// FsTdmfDb.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRec.h"

FsTdmfRec::FsTdmfRec ( FsTdmfDb* pDb, CString pszTblName ) : CRecordset ( (CDatabase*)pDb )
{
    mbFastMode  = FALSE; 
	mpDb        = pDb;
	mszTblName  = pszTblName;
	//muiOpenType = AFX_DB_USE_DEFAULT_TYPE;
	muiOpenType = CRecordset::dynaset;
	//muiOpenType = CRecordset::snapshot;
	//muiOpenType = CRecordset::forwardOnly;

	SQLRETURN lSqlRet = SQLAllocHandle ( SQL_HANDLE_STMT, mpDb->m_hdbc, &mHstmt );
	if ( lSqlRet != SQL_SUCCESS )
	{
		mpDb->FtdErr ( "FsTdmfDb::FsTdmfDb():Odbc Direct has problems" );
	}

	FtdPreFills();

    m_hMutex = CreateMutex(0,0,0);

} // FsTdmfRec::FsTdmfRec ()


// Position the current t_agent record using the given Sql Select
// Example1 of pszSelect:	"BabSize >= 100"
// Example2 of pszSelect:	"AgentName LIKE ( 'Sof%' )
// Example3 of pszSelect:	"BabSize >= 100" AND "AgentName LIKE ( 'Sof%' )
// 
// Example1 of pszSort1:	"BabSize"
// Example2 of pszSort2:	"BabSize, AgentName"
//
BOOL FsTdmfRec::FtdFirst ( CString pszSelect, CString pszSort )
{
	return FALSE;

} // FsTdmfDb::FtdFirst ()


CString FsTdmfRec::FtdGetCreate ()
{
	return "FsTdmfRec::FtdGetCreate ():Virtual Class Without Table";

} // FsTdmfRec::FtdGetCreate ()


FsTdmfRec::~FsTdmfRec ()
{
	FtdDestroy();

    CloseHandle( m_hMutex );

} // FsTdmfRec::~FsTdmfRec ()


BOOL FsTdmfRec::FtdNew ( CString pszName )
{
	return FALSE;

} // FsTdmfRec::FtdNew ( CString pszName )


BOOL FsTdmfRec::FtdNew ( CString pszName, CString pszVal )
{
	return FALSE;

} // FsTdmfRec::FtdNew ( CString pszName )


BOOL FsTdmfRec::FtdNew ( int piId )
{
	return FALSE;

} // FsTdmfRec::FtdNew ( int piId )


BOOL FsTdmfRec::FtdNext ()
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}

	//mList.RemoveAll();

	if ( !mpDb->FtdIsOpen() )
	{
		mpDb->FtdErr( 
			"FsTdmfRec::FtdNext(): Db not openned, cannot move cursor"
		);
		return FALSE;
	}
	else if ( !IsOpen () )
	{
		mpDb->FtdErr( 
			"FsTdmfRec::FtdNext(): Rec not openned, cannot move cursor"
		);
		return FALSE;
	}
	else try
	{
		MoveNext ();
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRec::FtdNext(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	if ( IsEOF () )
	{
		return FALSE;
	}

	return FtdFills();

} // FsTdmfRec::FtdNext ()


BOOL FsTdmfRec::FtdMove ( long plIndex )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}

	//mList.RemoveAll();

	if ( !mpDb->FtdIsOpen() )
	{
		mpDb->FtdErr( 
			"FsTdmfRec::FtdNext(): Db not openned, cannot move cursor"
		);
		return FALSE;
	}
	else if ( !IsOpen () )
	{
		mpDb->FtdErr( 
			"FsTdmfRec::FtdNext(): Rec not openned, cannot move cursor"
		);
		return FALSE;
	}
	else try
	{
		Move ( plIndex, SQL_FETCH_ABSOLUTE );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRec::FtdNext(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	if ( IsEOF () )
	{
		return FALSE;
	}

	return FtdFills();

} // FsTdmfRec::FtdMove ()


BOOL FsTdmfRec::FtdNextFast ()
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
			"FsTdmfRec::FtdNextFast(): Db not openned, cannot move cursor"
		);
		return FALSE;
	}
	else if ( SQL_SUCCESS != SQLFetch ( mHstmt ) ) 
	{
		mpDb->FtdErrSql( mHstmt, "FsTdmfRec::FtdNext:SQLFetch()" );
		return FALSE;
	}

	return TRUE;

} // FsTdmfRec::FtdNextFast ()


BOOL FsTdmfRec::FtdOpen ( CString pszSql )
{
	if ( !FtdOpenTmp( pszSql ) )
	{
		return FALSE;
	}

	return FtdFills();

} // FsTdmfRec::FtdOpen ()


BOOL FsTdmfRec::FtdOpenFast ( CString pszSql )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}
	else if ( !mpDb->FtdIsOpen() )
	{
		mpDb->FtdErr( "FsTdmfRec::FtdOpenFast(): Db not openned" );
		return FALSE;
	}
	else try
	{
		if ( SQL_SUCCESS != 
			 SQLExecDirect ( mHstmt, (unsigned char*)(LPCTSTR)pszSql, SQL_NTS )
		   )
		{
			mpDb->FtdErrSql( 
				mHstmt, 
				"FsTdmfRec::FtdOpenFast(): cant SQLExecDirect() with\n<" + pszSql + ">"
			);

			return FALSE;
		}
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( 
			"FsTdmfRec::FtdOpenFast():CDBException,\n<%.256s>\n<%.256s>", e->m_strError, pszSql
		);
		e->Delete ();
		return FALSE;
	}

	return FtdNextFast();

} // FsTdmfRec::FtdOpenFast ()


BOOL FsTdmfRec::FtdOpenTmp ( CString pszSql )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}
	else if ( !mpDb->FtdIsOpen() )
	{
		mpDb->FtdErr( "FsTdmfRec::FtdOpenTmp(): Db not openned" );
		return FALSE;
	}
	else try
	{
		if ( IsOpen () )
		{
			Close ();
		}

		if ( !Open ( muiOpenType, pszSql ) )
		{
			mpDb->FtdErr( 
				"FsTdmfRec::FtdOpenTmp():Open() with\n<" + pszSql + ">" );
			return FALSE;
		}
		else if ( IsEOF () ) // No Record in RecordSet
		{
			return FALSE;
		}
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( 
			"FsTdmfRec::FtdOpenTmp():CDBException,\n<%.256s>\n<%.256s>", e->m_strError, pszSql
		);
		e->Delete ();
		return FALSE;
	}
	catch ( CMemoryException* e )
	{
		mpDb->FtdErr( 
			"FsTdmfRec::FtdOpenTmp():CMemoryException,\n<%.256s>\n<Memory>", pszSql
		);
		e->Delete ();
		return FALSE;
	}

	return TRUE;

} // FsTdmfRec::FtdOpenTmp ()


int FsTdmfRec::FtdCount (CString cszWhere)
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return -1;
	}

    SQLUINTEGER   NRecords;
    SQLINTEGER    NRecordsInd;
    SQLHSTMT      hstmt;
    CString       cszSQL;  

    cszSQL =  "SELECT COUNT(*) FROM ";
    cszSQL += mszTblName;
	if ( !cszWhere.IsEmpty() )
	{
		cszSQL += " WHERE ";
		cszSQL += cszWhere;
	}
   
    if ( SQL_SUCCESS != ::SQLAllocHandle(SQL_HANDLE_STMT, mpDb->m_hdbc, &hstmt) )
        return -1;
    if ( SQL_SUCCESS != ::SQLBindCol(hstmt, 1, SQL_C_ULONG, &NRecords, 0, &NRecordsInd) )
        return -1;
    // Execute a statement to get the sales person/customer of all orders.
    if ( SQL_SUCCESS != ::SQLExecDirect(hstmt, (unsigned char*)(LPCTSTR)cszSQL, SQL_NTS) )
        return -1;
    // NRecords is available after a successful SQLFetch()
    if ( SQL_SUCCESS != ::SQLFetch(hstmt) ) 
        return -1;

    // Cleanup
    ::SQLCloseCursor(hstmt);
	::SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    return (int)NRecords;


} // FsTdmfRec::FtdCount ()


void FsTdmfRec::FtdDestroy ()
{
	int      liT  = mList.GetCount ();
	POSITION lPos = mList.GetHeadPosition ();

	for ( int liC = 0; liC < liT; liC++ )
	{
		FsNvp &lNvp = mList.GetNext ( lPos );

		delete [] lNvp.mcpV;
	}
	mList.RemoveAll();
	
	//for ( int liC = 0; liC < liT; liC++ )
	//{
	//	POSITION lPos = mList.FindIndex ( liC  );
	//	FsNvp    lNvp = mList.GetAt     ( lPos );

	//	delete [] lNvp.mcpV;

	//} // for
	
} // FsTdmfRec::FtdDestroy ()


BOOL FsTdmfRec::FtdFills ()
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}

	int      liT  = mList.GetCount ();
	POSITION lPos = mList.GetHeadPosition ();

	for ( int liC = 0; liC < liT; liC++ )
	{
		FsNvp &lNvp = mList.GetNext ( lPos );

		GetFieldValue ( liC, lNvp.mszV );
		//mList.SetAt( lPos, lNvp );
	}
	
	//for ( int liC = 0; liC < liT; liC++ )
	//{
	//	POSITION lPos = mList.FindIndex ( liC  );
	//	FsNvp    lNvp = mList.GetAt     ( lPos );

	//	GetFieldValue ( liC, lNvp.mszV );
	//	mList.SetAt( lPos, lNvp );
	//} // for
	

	return TRUE;

} // FsTdmfRec::FtdFills ()


BOOL FsTdmfRec::FtdPos ( CString pszVal, CString pszFldId )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + pszFldId + " = '" + pszVal + "'"
	);

	return FtdOpen( lszSel );

} // FsTdmfRec::FtdPos ( CString pszVal, CString pszFldId )


BOOL FsTdmfRec::FtdPos ( int piKa, CString pszFldId )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + pszFldId + " = %ld", piKa
	);

	return FtdOpen( lszSel );

} // FsTdmfRec::FtdPos ( int piKa, CString pszFldId )


BOOL FsTdmfRec::FtdPos ( CString pszName )
{
	return FALSE;

} // FsTdmfRec::FtdPos ()

BOOL FsTdmfRec::FtdPos ( int piId )
{
	return FALSE;

} // FsTdmfRec::FtdPos ()

//////////////////////////////////////////////////////////////////////////////
//
// FtdPreFills ()
//
//	The return is checked on FtdOpenTmp() and IsOpen() because
// we have to make sure to fill the list even if the table is empty, and
// we also make sure NOT to 
BOOL FsTdmfRec::FtdPreFills ()
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}
	else if ( !FtdOpenTmp( "SELECT TOP 1 * FROM " + mszTblName ) )
	//else if ( !FtdOpenTmp( "SELECT * FROM " + mszTblName ) )
	{
		if ( !IsOpen() )
		{
			return FALSE;
		}
	}

	CODBCFieldInfo lFi;
	FsNvp          lNvp;
    SQLINTEGER     lSqlIntInd;

    lNvp.mcpV = NULL;

	for ( int liC = 0; liC < GetODBCFieldCount (); liC++ )
	{
		try
		{
			GetODBCFieldInfo( liC, lFi );

			lNvp.mszN = lFi.m_strName;

			lNvp.mcpV = new char[lFi.m_nPrecision+1024];//sg //was +1???
			//GetFieldValue ( liC, lNvp.mszV );
			if ( SQL_SUCCESS != SQLBindCol 
				( mHstmt, liC+1, SQL_C_CHAR, lNvp.mcpV, 1024, &lSqlIntInd) )
			{
				Close ();
                delete [] lNvp.mcpV;
				return FALSE;
			}

			mList.AddTail ( lNvp );
		}
		catch ( CDBException* e )
		{
			mpDb->FtdErr( "FsTdmfRec::FtdFills(),\n <%.256s>", e->m_strError );
			//e->Delete ();
            if (lNvp.mcpV)
            {
                delete [] lNvp.mcpV;
            }
			Close ();
			return FALSE;
		}
	} // for

	//int liT =  mList.GetCount ();

	Close ();

	return TRUE;

} // FsTdmfRec::FtdPreFills ()


BOOL FsTdmfRec::FtdRename ( CString pszOldName, CString pszNewName )
{
	return FALSE;

} // FsTdmfRec::FtdRename ()


CString FsTdmfRec::FtdSel ( int piFld )
//char* FsTdmfRec::FtdSel ( int piFld )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return "";//"0";
	}
	//else if ( !IsOpen () )
	//{
	//	return "";//"0";
	//}

	POSITION lPos = mList.FindIndex ( piFld );

	if ( lPos == 0 )
	{
		return "";
	}

	FsNvp &lNvp = mList.GetAt ( lPos );

	lNvp.mszV.Replace("''", "'");
	lNvp.mszV.Replace("\"\"", "\"");

	return lNvp.mszV;
	//return lNvp.mcaV;
	//return lNvp.mcpV;

} // FsTdmfRec::FtdSel ( int piFld )


CString FsTdmfRec::FtdSel ( CString pszFld )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return "";//"0";
	}
	else if ( !mbFastMode )
	{
		if ( !IsOpen () )
		{
			return "";//"0";
		}
	}

	CString  lszVal;
	POSITION lPos = mList.GetHeadPosition ();

	for ( int liC = 0; liC < mList.GetCount (); liC++ )
	{
		FsNvp &lNvp = mList.GetNext ( lPos );

		if ( pszFld == lNvp.mszN )
		{
			lNvp.mszV.Replace("''", "'");
			lNvp.mszV.Replace("\"\"", "\"");

			return lNvp.mszV;
		}
	}

	return "";

} // FsTdmfRec::FtdSel ()


BOOL FsTdmfRec::FtdSetList ( CString pszFld, CString pszVal )
{
	if (   this == NULL )
	{
		return FALSE;
	}

	CString  lszVal;
	POSITION lPos = mList.GetHeadPosition ();

	for ( int liC = 0; liC < mList.GetCount (); liC++ )
	{
		FsNvp &lNvp = mList.GetNext ( lPos );

		if ( pszFld == lNvp.mszN )
		{
			lNvp.mszV = pszVal;
			//mList.SetAt( lPos, lNvp );
			return TRUE;
		}
	}

	return FALSE;

} // FsTdmfRec::FtdSetList ()


BOOL FsTdmfRec::FtdUpd ( CString pszFld, CString pszVal )
{
	return FALSE;

} // FsTdmfDb::FtdUpd ()


BOOL FsTdmfRec::FtdUpd ( 
	CString pszFldName, CString pszVal, CString pszKaName )
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
			"FsTdmfRec::FtdUpd(): "
			"Db not openned, cannot update record " + mszTblName + " with <%s>",
			pszFldName 
		);

		return FALSE;
	}

	CString lszKa = FtdSel( pszKaName );

	if ( lszKa == "" )
	{
		mpDb->FtdErr(
			"FsTdmfRec::FtdUpd(): "
			"No current record " + mszTblName + ", "
			"cannot update field <" + pszFldName + "> "
			"with <" + pszVal + ">"
		);

		return FALSE;
	}

	pszVal.Replace("'", "''");
	pszVal.Replace("\"", "\"\"");
	pszVal.Replace("%", "%%");

	CString lszSql;
	lszSql.Format ( 
		"UPDATE " + mszTblName + " SET " + pszFldName + " = '" + pszVal + "' "
		"WHERE "  + pszKaName + " = " + lszKa
	);

	try
	{
		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRec::FtdUpd(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	if ( pszFldName == pszKaName ) // Changing Key Aouch
	{
		Close ();
		lszKa = pszVal;
	}

	if ( IsOpen () )
	{
		return FtdSetList( pszFldName, pszVal );
	}
	else
	{
		CString lszSql = 
			"SELECT TOP 1 * FROM " + mszTblName + " WHERE " + pszKaName + " = " + lszKa;

		return FtdOpen( lszSql );
	}

} // FsTdmfRec::FtdUpd ()


BOOL FsTdmfRec::FtdUpd ( CString pszFld, int piVal )
{
	return FALSE;

} // FsTdmfRec::FtdUpd ()


BOOL FsTdmfRec::FtdUpd ( CString pszFld, __int64 pjVal )
{
	return FALSE;

} // FsTdmfRec::FtdUpd ()


BOOL FsTdmfRec::FtdUpd ( 
	CString pszFldName, __int64 pjVal, CString pszKaName )
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
			"FsTdmfRec::FtdUpd(): "
			"Db not openned, cannot update record " + mszTblName + " "
			"field < " + pszFldName + " > with <%I64d>",
			pjVal
		);

		return FALSE;
	}

	CString lszKa = FtdSel( pszKaName );

	if ( lszKa == "" )
	{
		mpDb->FtdErr(
			"FsTdmfRec::FtdUpd(): "
			"No current record " + mszTblName + ", cannot update field <"
			+ pszFldName + "> with <%I64d>", pjVal
		);
		return FALSE;
	}

	CString lszSql;
	lszSql.Format ( 
		"UPDATE " + mszTblName + " SET " + pszFldName + " = %I64d "
		"WHERE "  + pszKaName  + " = "   + lszKa,
		pjVal
	);

	try
	{
		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRec::FtdUpd(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	if ( pszFldName == pszKaName ) // Changing Key Aouch
	{
		Close ();
		lszKa.Format ( "%I64d", pjVal );
	}

	if ( IsOpen () )
	{
		CString lszVal;
		lszVal.Format ( "%I64d", pjVal ); 
		return FtdSetList( pszFldName, lszVal );
	}
	else
	{
		CString lszSql = 
			"SELECT TOP 1 * FROM " + mszTblName + " WHERE " + pszKaName + " = " + lszKa;

		return FtdOpen( lszSql );
	}

} // FsTdmfRec::FtdUpd ( ..., __int64, ... )


// No need of current record when Saving
BOOL FsTdmfRec::FtdSave ( CString pszSet, CString pszWhere )
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
			"FsTdmfRec::FtdSave(): "
			"Db not openned, cannot update record " + mszTblName + " "
			"field <" + pszSet + ">\nwhere <" + pszWhere + ">"
		);

		return FALSE;
	}

	CString lszSql = 
		"UPDATE " + mszTblName + "\n"
		+ "SET " + pszSet + "\n"
		+ "WHERE " + pszWhere;

	try
	{
		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRec::FtdSave(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	return TRUE;
	// Please No Record Open When Save

} // FsTdmfRec::FtdSave ()


BOOL FsTdmfRec::FtdUpd ( CString pszFldName, int piVal, CString pszKaName )
{
	return FtdUpd( pszFldName, (__int64)piVal, pszKaName );

} // FsTdmfRec::FtdUpd ()


BOOL FsTdmfRec::FtdDelete    ( CString pcszWhere )
{
	CString strWhere;
	if (pcszWhere != "")
	{
		strWhere = " WHERE " + pcszWhere;
	}

    if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}
	else if ( !mpDb->FtdIsOpen() )
	{
		mpDb->FtdErr( "FsTdmfRec::FtdDelete(): Db not openned" );
		return FALSE;
	}
	else try
	{
        CString cszSQL;
        cszSQL.Format("DELETE "
                      "FROM  %s " 
                      "%s "
                      , (LPCTSTR)mszTblName
                      , (LPCTSTR)strWhere
                      );

		mpDb->ExecuteSQL ( cszSQL );
        return TRUE;
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( 
			"FsTdmfRec::FtdDelete(),\n <%.256s>", e->m_strError
		);
		e->Delete ();
		return FALSE;
	}

	return TRUE;
}

BOOL FsTdmfRec::FtdDelete    ( int iDeleteNFirstInRecordset )
{
    return FtdDelete( iDeleteNFirstInRecordset, "" );
}

BOOL FsTdmfRec::FtdDelete ( int iMaxRecordToDelete , CString pcszWhere )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}
	else if ( !mpDb->FtdIsOpen() )
	{
		mpDb->FtdErr( "FsTdmfRec::FtdDelete(): Db not openned" );
		return FALSE;
	}
	else try
	{
        char buf[32]; 
        CString cszSQL;
        cszSQL.Format("DELETE %s "
                      "FROM (SELECT %s * " 
                           " FROM %s "  
                           " %s) AS t1 "
                      "WHERE %s.ik = t1.ik "
                      , (LPCTSTR)mszTblName
                      , iMaxRecordToDelete == 0 ? "" : (LPCTSTR)( CString("TOP ") + itoa(iMaxRecordToDelete,buf,10) )
                      , (LPCTSTR)mszTblName
                      , pcszWhere.IsEmpty() ? "" : (LPCTSTR)pcszWhere
                      , (LPCTSTR)mszTblName
                      );

		mpDb->ExecuteSQL ( cszSQL );
        return TRUE;
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( 
			"FsTdmfRec::FtdDelete(),\n <%.256s>", e->m_strError
		);
		e->Delete ();
		return FALSE;
	}

	return TRUE;
}

//get table size string in KB
CString FsTdmfRec::FtdGetTableSize ()
{
	#define		TABLESIZE_LEN  50
	SQLCHAR		szTableSize[TABLESIZE_LEN];
	SQLINTEGER	cbTableSize;
	SQLHSTMT	hstmt;
	SQLRETURN	retcode;
	CString		resultStr;

	SQLAllocHandle(SQL_HANDLE_STMT, mpDb->m_hdbc, &hstmt);

	CString sqlStat = "sp_spaceused '";
	sqlStat += mszTblName;
	sqlStat += "', 'TRUE'";

	unsigned char pBuffer[128];
	strcpy( (char *) pBuffer, (LPCTSTR)sqlStat );

	retcode = SQLExecDirect(hstmt, pBuffer, SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		/* Bind columns 1 */
		SQLBindCol(hstmt, 3, SQL_C_CHAR, szTableSize, TABLESIZE_LEN, &cbTableSize);
		/* Fetch and print the row of data. On an error, display a message and exit. */
		retcode = SQLFetch(hstmt);
		if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
		{
			mpDb->FtdErr("FsTdmfRec::FtdGetTableSize()\n ");
		}

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			resultStr = szTableSize;
		}
		else
		{
			resultStr =  "-1 KB";
		}
	}
	else
	{
		resultStr = "-1 KB";
	}

	// Close the cursor.
    SQLCloseCursor(hstmt);

	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

	return resultStr;
}

//delete the records from the table
BOOL FsTdmfRec::FtdDeleteTableRecords ()
{
	SQLHSTMT	hstmt;
	SQLRETURN	retcode;
	BOOL		result;

	SQLAllocHandle(SQL_HANDLE_STMT, mpDb->m_hdbc, &hstmt);

	CString sqlStat = "DELETE ";
	sqlStat += mszTblName;

	unsigned char pBuffer[128];

	strcpy( (char *) pBuffer, (LPCTSTR)sqlStat );

	retcode = SQLExecDirect(hstmt, pBuffer, SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		result = TRUE;
	}
	else
	{
		result = FALSE;
	}

	sqlStat = "DBCC UPDATEUSAGE 0";

	strcpy( (char *) pBuffer, (LPCTSTR)sqlStat );

	SQLExecDirect(hstmt, pBuffer, SQL_NTS);

	// Close the cursor.
    SQLCloseCursor(hstmt);

	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

	return result;
}

//alter table by adding/droping a column/contraint
BOOL FsTdmfRec::FtdAlter ( CString pszSql )
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
			"FsTdmfRec::FtdAlter(): "
			"Db not openned, cannot alter table " + mszTblName + " "
			"for <" + pszSql + ">"
		);

		return FALSE;
	}

	CString lszSql = 
		"ALTER TABLE " + mszTblName + "\n"
		+ pszSql;

	try
	{
		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRec::FtdAlter(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	return TRUE;
	// Please No Record Open When Alter
}

//get exclusive access on table
void FsTdmfRec::FtdLock()
{
    WaitForSingleObject(m_hMutex,INFINITE);
}

//release exclusive access on table
void FsTdmfRec::FtdUnlock()
{
    ReleaseMutex(m_hMutex);
}

BOOL FsTdmfRec::BeginTrans()
{
	return mpDb->BeginTrans();
}

BOOL FsTdmfRec::CommitTrans()
{
	return mpDb->CommitTrans();
}

BOOL FsTdmfRec::Rollback()
{
	return mpDb->Rollback();
}
