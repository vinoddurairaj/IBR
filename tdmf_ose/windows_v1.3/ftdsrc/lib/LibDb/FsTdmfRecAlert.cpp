// FsTdmfNvp.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecAlert.h"
#include "FsTdmfRecPair.h"

CString FsTdmfRecAlert::smszTblName  = "t_Alert_Status";		// DbVersion = "1.3.2"
CString FsTdmfRecAlert::smszFldIk    = "Ik"; // Internal Key	// DbVersion = "1.3.2"

CString FsTdmfRecAlert::smszFldPairFk = "PairId_Fk";			// DbVersion = "1.3.2"
CString FsTdmfRecAlert::smszFldGrpFk = "LgGroupId_Fk";			// DbVersion = "1.3.2"
CString FsTdmfRecAlert::smszFldSrcFk = "Source_Fk";				// DbVersion = "1.3.2"

CString FsTdmfRecAlert::smszFldType  = "Type";					// DbVersion = "1.3.2"
CString FsTdmfRecAlert::smszFldSev   = "Severity";				// DbVersion = "1.3.2"
CString FsTdmfRecAlert::smszFldTs    = "Time_Stamp";			// DbVersion = "1.3.2"
CString FsTdmfRecAlert::smszFldTxt   = "Text";					// DbVersion = "1.3.2"

CString FsTdmfRecAlert::smszInsertQueryFrmt;

FsTdmfRecAlert::FsTdmfRecAlert ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{
    /*
#define SHORT_FRMT  "%hd"
#define STR_FRMT    "%s"
    smszInsertQueryFrmt = "INSERT INTO " + mszTblName + " ( " 
			                + smszFldPairFk     + ", " 
                            + smszFldGrpFk      + ", " 
			                + smszFldSrcFk      + ", " 
                            + smszFldTs         + ", " 
                            + smszFldType       + ", "   
                            + smszFldSev        + ", "   
                            + smszFldTxt        +
		                 " ) "
		                 "VALUES ( " 
			            "'" + STR_FRMT      + "', "//smszFldPairFk          
                        "'" + STR_FRMT      + "', "//smszFldGrpFk           
			            "'" + STR_FRMT      + "', "//smszFldSrcFk           
                        "'" + STR_FRMT      + "', "//smszFldTs              
                        "'" + STR_FRMT      + "', "//smszFldType            
                            + SHORT_FRMT    + ", " //smszFldSev         
                        "'" + STR_FRMT      + "' " //smszFldTxt         
                         " )";
                         */

} // FsTdmfRecAlert::FsTdmfRecAlert ()


BOOL FsTdmfRecAlert::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecAlert::FtdFirst ()


BOOL FsTdmfRecAlert::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecAlert::FtdFirstFast ()


// If pszWhere is null then it is a sub select
CString FsTdmfRecAlert::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecAlert::FtdFirstCurs(): Db not openned, cannot init"
		);
		return "";
	}

	CString lszSqlWhere; // = "WHERE ";

	if ( pszWhere == "" )
	{
		CString lszPairFk = 
			mpDb->mpRecPair->FtdSel( FsTdmfRecPair::smszFldPairId );
		CString lszGrpFk = 
			mpDb->mpRecPair->FtdSel( FsTdmfRecPair::smszFldGrpFk );
		CString lszSrcFk = 
			mpDb->mpRecPair->FtdSel( FsTdmfRecPair::smszFldSrcFk );

        //allow Alert record to not be associated to SrcFk,GrpFk,PairFk
        /*
		if ( lszPairFk == "" || lszGrpFk == "" || lszSrcFk == "" )
		{
			mpDb->FtdErr( 
				"FsTdmfRecAlert::FtdFirst(): No current Logical Group record" 
			);
			return FALSE;
		}

		lszSqlWhere += 
			         smszFldPairFk + " = " + lszPairFk + " "
			"AND " + smszFldGrpFk + " = " + lszGrpFk + " "
			"AND " + smszFldSrcFk + " = " + lszSrcFk;
        */

        //allow Alert record to not be associated to SrcFk,GrpFk,PairFk :
        lszSqlWhere = "";
		if ( lszPairFk != "" )
		{
            if ( lszSqlWhere != "" )
                lszSqlWhere += "AND ";
            lszSqlWhere += smszFldPairFk + " = " + lszPairFk + " ";
		}
		if ( lszGrpFk != "" )
		{
            if ( lszSqlWhere != "" )
                lszSqlWhere += "AND ";
            lszSqlWhere += smszFldGrpFk + " = " + lszGrpFk + " ";
		}
		if ( lszSrcFk != "" )
		{
            if ( lszSqlWhere != "" )
                lszSqlWhere += "AND ";
            lszSqlWhere += smszFldSrcFk + " = " + lszSrcFk + " ";
		}
	}
	else
	{
		lszSqlWhere += pszWhere;
	}

	CString lszSort;

	if ( pszSort == "" )
	{
		lszSort = smszFldIk;
	}
	else
	{
		lszSort = pszSort;
	}

	CString lszSql;
	lszSql.Format ( 
		"SELECT * FROM " + mszTblName + " WHERE " + lszSqlWhere + " "
		"ORDER BY " + lszSort
	);

	return lszSql;

} // FsTdmfRecAlert::FtdFirstCurs ()


CString FsTdmfRecAlert::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"( "
			+ smszFldIk     + " INT          IDENTITY NOT NULL,\n"
            // v1.3.2 : allow Alert record to not be associated to SrcFk,GrpFk,PairFk
			+ smszFldPairFk + " SMALLINT     DEFAULT   -1,\n"
			+ smszFldGrpFk  + " SMALLINT     DEFAULT   -1,\n"
			+ smszFldSrcFk  + " INT          DEFAULT   -1,\n"

			+ smszFldType   + " CHAR(16),\n"
			+ smszFldSev    + " TINYINT,\n"
			+ smszFldTs     + " DATETIME,\n"
			+ smszFldTxt    + " VARCHAR(512),\n"

			"CONSTRAINT AlertPk PRIMARY KEY ( " + smszFldIk + "),\n"
		")\n"
	);

	return pszCreateTbl;

} // FsTdmfRecAlert::FtdGetCreate ()


FsTdmfRecAlert::~FsTdmfRecAlert ()
{

} // FsTdmfRecAlert::~FsTdmfRecAlert ()


BOOL FsTdmfRecAlert::FtdNew ( CString &     pszTxt,
                              int           pserverHostId,
                              int           pgroupNbr,
                              int           pdeviceId,
                              int           ptimestamp,
                              const char*   pszType,
                              short         pseverity
                              )
{
    CString lszTS;//empty by default
	if ( ptimestamp != -1 )
    {
        CTime ts(ptimestamp);
        lszTS = ts.Format("%Y-%m-%d %H:%M:%S");
    }
    else
    {
        CTime ts(time(0));
        lszTS = ts.Format("%Y-%m-%d %H:%M:%S");
    }

	return FtdNew ( pszTxt,
                    pserverHostId,
                    pgroupNbr,
                    pdeviceId,
                    lszTS,
                    pszType,
                    pseverity
                    );

} // FsTdmfRecAlert::FtdNew ( )

BOOL FsTdmfRecAlert::FtdNew ( CString &     pszTxt,
                              int           pserverHostId,
                              int           pgroupNbr,
                              int           pdeviceId,
                              CString &     pszTimestamp,
                              const char*   pszType,
                              short         pseverity
                              )
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
			"FsTdmfRecAlert::FtdNew(): "
			"Db not openned, cannot create new record with <" + pszTxt + ">"
		);

		return FALSE;
	}

    CString lszGrpNbr;
    CString lszPairId;
    CString lszSrvId;

    if ( mpDb->mpRecSrvInf->FtdPos( pserverHostId ) )
        lszSrvId = mpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvId );

    if ( mpDb->mpRecGrp->FtdPos( pgroupNbr ) )
        lszGrpNbr = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldLgGrpId );

    if ( mpDb->mpRecPair->FtdPos( pdeviceId ) )
        lszPairId = mpDb->mpRecPair->FtdSel( FsTdmfRecPair::smszFldPairId );


    //allow Alert record to not be associated to SrcFk,GrpFk,PairFk
	/*
	if ( lszPairFk == "" || lszGrpFk == "" || lszSrvFk == "" )
	{
		mpDb->FtdErr( 
			"FsTdmfRecAlert::FtdNew(): No current " 
			+ FsTdmfRecPair::smszTblName + " record" 
		);
		return FALSE;
	}
	else try
    {
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " 
				+ smszFldPairFk + ", " + smszFldGrpFk + ", " 
				+ smszFldSrcFk  + ", " + smszFldTxt +
			" ) "
			"VALUES ( " 
				+ lszPairFk + ", "  + lszGrpFk + ", " 
				+ lszSrvFk  + ", '" + pszTxt   + "' )"
		);

		mpDb->ExecuteSQL ( lszInsert + lszValues );
	}
    */
    try
	{
		CString lszSql,lszInsert,lszValues;

        lszInsert = "INSERT INTO " + mszTblName + " ( ";
	    if ( lszPairId != "" )
            lszInsert += smszFldPairFk + ", ";
	    if ( lszGrpNbr != "" )
            lszInsert += smszFldGrpFk + ", ";
	    if ( lszSrvId != "" )
            lszInsert += smszFldSrcFk + ", ";
	    if ( pszTimestamp != "" )
            lszInsert += smszFldTs + ", ";
	    if ( pszType != 0 )
            lszInsert += smszFldType + ", ";
	    if ( pseverity != -1 )
            lszInsert += smszFldSev + ", ";
        lszInsert += smszFldTxt + " ) ";

        lszValues = "VALUES ( ";
	    if ( lszPairId != "" )
            lszValues += lszPairId + ", ";
	    if ( lszGrpNbr != "" )
            lszValues += lszGrpNbr + ", ";
	    if ( lszSrvId != "" )
            lszValues += lszSrvId + ", ";
	    if ( pszTimestamp != "" )
            lszValues += "'" + pszTimestamp + "', ";
	    if ( pszType != 0 )
            lszValues += "'" + CString(pszType) + "', ";
	    if ( pseverity != -1 )
        {
            char buf[32];
            lszValues += CString( itoa(pseverity,buf,10) ) + ", ";
        }

		CString cstrText = pszTxt;
		cstrText.Replace("'", "''");
		cstrText.Replace("\"", "\"\"");

        lszValues += "'" + cstrText + "' ) ";

		mpDb->ExecuteSQL ( lszInsert + lszValues );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecAlert::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

    
	//CString lszSel;
    //allow Alert record to not be associated to SrcFk,GrpFk,PairFk
    /*
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldPairFk + " = "  + lszPairFk + " "
		"AND "   + smszFldGrpFk  + " = "  + lszGrpFk  + " "
		"AND "   + smszFldSrcFk  + " = "  + lszSrvFk  + " "
		"AND "   + smszFldTxt    + " = '" + pszTxt    + "' " 
		"ORDER BY " + smszFldTs
	);
    */
    /*
    lszSel =    "SELECT * FROM " + mszTblName + " "
		        "WHERE " + smszFldTxt    + " = '" + pszTxt    + "' ";
	if ( lszPairId != "" )
        lszSel += "AND "   + smszFldPairFk  + " = "  + lszPairId  + " ";
	if ( lszGrpNbr != "" )
        lszSel += "AND "   + smszFldGrpFk  + " = "  + lszGrpNbr  + " ";
	if ( lszSrvId != "" )
        lszSel += "AND "   + smszFldSrcFk  + " = "  + lszSrvId  + " ";
    lszSel += "ORDER BY " + smszFldTs;
    */

    return TRUE;//FtdOpen( lszSel );

} // FsTdmfRecAlert::FtdNew ( CString pszName )

// Position the current record using the Internal Key
BOOL FsTdmfRecAlert::FtdPos ( CString pszIk )
{
	return FtdPos( atoi ( pszIk ) );

} // FsTdmfRecAlert::FtdPos ()


BOOL FsTdmfRecAlert::FtdPos ( int piIk )
{
	return FsTdmfRec::FtdPos( piIk, smszFldIk );

} // FsTdmfRecAlert::FtdPos ()


BOOL FsTdmfRecAlert::FtdUpd ( CString pszFldName, CString pszVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, pszVal, smszFldIk );

} // FsTdmfRecAlert::FtdUpd ()


BOOL FsTdmfRecAlert::FtdUpd ( CString pszFldName, int piVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, piVal, smszFldIk );

} // FsTdmfRecAlert::FtdUpd ()

double  FsTdmfRecAlert::FtdGetAlertDateTime(int nAlertIk)
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return -1;
	}

	SQLDOUBLE     NRecords;
    SQLINTEGER    NRecordsInd;
    SQLHSTMT      hstmt;

    CString       cszSQL;

	//select convert(float, Time_Stamp) from t_Alert_Status where Ik = nAlertIk
	cszSQL.Format (	"SELECT CONVERT(float, Time_Stamp) FROM " + mszTblName +
					" WHERE " + smszFldIk + " = %ld ", nAlertIk );

    if ( SQL_SUCCESS != ::SQLAllocHandle(SQL_HANDLE_STMT, mpDb->m_hdbc, &hstmt) )
        return -1;
    if ( SQL_SUCCESS != ::SQLBindCol(hstmt, 1, SQL_C_DOUBLE, &NRecords, 0, &NRecordsInd) )
        return -1;
    // Execute the statement.
    if ( SQL_SUCCESS != ::SQLExecDirect(hstmt, (unsigned char*)(LPCTSTR)cszSQL, SQL_NTS) )
        return -1;
    // NRecords is available after a successful SQLFetch()
    if ( SQL_SUCCESS != ::SQLFetch(hstmt) ) 
        return -1;
    // Close the cursor.
    ::SQLCloseCursor(hstmt);

	::SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    return (double)NRecords;

} // FsTdmfRecAlert::FtdGetAlertDateTime

