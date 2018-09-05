// FsTdmfRecPerf.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecPerf.h"
#include "FsTdmfRecLgGrp.h"

CString FsTdmfRecPerf::smszTblName       = "t_Performance";			// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldIk         = "Ik"; // Internal Key	// DbVersion = "1.3.2"

CString FsTdmfRecPerf::smszFldPairFk     = "PairId_Fk";				// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldGrpFk      = "LgGroupId_Fk";			// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldSrcFk      = "Source_Fk";				// DbVersion = "1.3.2"

CString FsTdmfRecPerf::smszFldTs         = "Time_Stamp";			// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldRole       = "Role";					// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldConn       = "Connection";			// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldDrvMode    = "Drv_Mode";				// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldLgNum      = "Logical_Group_Number"; // ardeb see LgGroupId_Fk	// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldGuiLstIns  = "Gui_List_Insert";		// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldDevId      = "Device_Id";				// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldRSyncOff   = "R_Sync_Off";			// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldRSyncDelta = "R_Sync_Delta";			// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldEntries    = "Bab_Entries";			// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldSectors    = "Bab_Sectors";			// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldPctDone    = "Pct_Done";				// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldPctBab     = "Pct_Bab";				// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldInstName   = "Instance_Name";			// DbVersion = "1.3.2"

CString FsTdmfRecPerf::smszFldActual       = "Actual";				// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldEffective    = "Effective";			// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldBytesRead    = "Bytes_Read";			// DbVersion = "1.3.2"
CString FsTdmfRecPerf::smszFldBytesWritten = "Bytes_Written";		// DbVersion = "1.3.2"

CString FsTdmfRecPerf::smszInsertQueryFrmt;

FsTdmfRecPerf::FsTdmfRecPerf ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{

#define SHORT_FRMT  "%hd"
#define INT_FRMT    "%d"
#define INT64_FRMT  "%I64d"
#define STR_FRMT    "%s"
#define CHAR_FRMT   "%c"
    smszInsertQueryFrmt = "INSERT INTO " + mszTblName + " ( " 
			                + smszFldPairFk         + ", " 
                            + smszFldGrpFk          + ", " 
			                + smszFldSrcFk          + ", " 
                            + smszFldTs             + ", " 
                            + smszFldActual         + ", "   
                            + smszFldEffective      + ", "   
                            + smszFldBytesRead      + ", "   
                            + smszFldBytesWritten   + ", "   
			                + smszFldRole           + ", " 
                            + smszFldConn           + ", " 
			                + smszFldDrvMode        + ", " 
                            + smszFldLgNum          + ", " 
			                + smszFldGuiLstIns      + ", " 
                            + smszFldDevId          + ", " 
			                + smszFldRSyncOff       + ", " 
                            + smszFldRSyncDelta     + ", " 
			                + smszFldEntries        + ", " 
                            + smszFldSectors        + ", " 
			                + smszFldPctDone        + ", " 
                            + smszFldPctBab         + ", " 
			                + smszFldInstName       + 
		                 " ) "
		                 "VALUES ( " 
			                + INT_FRMT      + ", "//smszFldPairFk as device id         
                            + STR_FRMT      + ", "//smszFldGrpFk           
			                + STR_FRMT      + ", "//smszFldSrcFk           
                        "'" + STR_FRMT      + "', "//smszFldTs              
                            + INT64_FRMT    + ", "//smszFldActual            
                            + INT64_FRMT    + ", "//smszFldEffective         
                            + INT64_FRMT    + ", "//smszFldBytesRead         
                            + INT64_FRMT    + ", "//smszFldBytesWritten      
			            "'" + CHAR_FRMT     + "', "//smszFldRole            
                            + INT_FRMT      + ", "//smszFldConn            
			                + INT_FRMT      + ", "//smszFldDrvMode         
                            + INT_FRMT      + ", "//smszFldLgNum           
			                + INT_FRMT      + ", "//smszFldGuiLstIns       
                            + INT_FRMT      + ", "//smszFldDevId           
			                + INT_FRMT      + ", "//smszFldRSyncOff        
                            + INT_FRMT      + ", "//smszFldRSyncDelta      
			                + INT_FRMT      + ", "//smszFldEntries         
                            + INT_FRMT      + ", "//smszFldSectors         
			                + INT_FRMT      + ", "//smszFldPctDone         
                            + INT_FRMT      + ", "//smszFldPctBab          
			            "'" + STR_FRMT      + "'" //smszFldInstName        
                         + " )";

} // FsTdmfRecPerf::FsTdmfRecPerf ()


BOOL FsTdmfRecPerf::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecPerf::FtdFirst ()


BOOL FsTdmfRecPerf::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecPerf::FtdFirstFast ()


// If pszWhere is null then it is a sub select
CString FsTdmfRecPerf::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecPerf::FtdFirstCurs(): Db not openned, cannot init"
		);
		return "";
	}

	CString lszSqlWhere = "WHERE ";

	if ( pszWhere == "" )
	{
		//CString lszPairFk = 
		//	mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldPairId );
		CString lszGrpFk = 
            mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldLgGrpId );
		CString lszSrcFk = 
			mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldSrcFk );

		if ( /*lszPairFk != "" &&*/ lszGrpFk != "" && lszSrcFk != "" )
		{
		    lszSqlWhere += 
			             //smszFldPairFk + " = " + lszPairFk + " "
			    //"AND " + smszFldGrpFk  + " = " + lszGrpFk  + " "
			             smszFldGrpFk  + " = " + lszGrpFk  + " "
			    "AND " + smszFldSrcFk  + " = " + lszSrcFk;
		}
        else
        {
            lszSqlWhere.Empty();
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
		"SELECT * FROM " + mszTblName + " " + lszSqlWhere + " "
		"ORDER BY " + lszSort
	);

	return lszSql;

} // FsTdmfRecPerf::FtdFirstCurs ()


CString FsTdmfRecPerf::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"( "
			+ smszFldIk     + " INT       IDENTITY NOT NULL,\n"
			+ smszFldPairFk + " SMALLINT           NOT NULL,\n"
			+ smszFldGrpFk  + " SMALLINT           NOT NULL,\n"
			+ smszFldSrcFk  + " INT                NOT NULL,\n"

			+ smszFldTs           + " DATETIME,\n"
			+ smszFldActual       + " DECIMAL,\n"
			+ smszFldEffective    + " DECIMAL,\n"
			+ smszFldBytesRead    + " DECIMAL,\n"
			+ smszFldBytesWritten + " DECIMAL,\n"

			+ smszFldRole       + " CHAR,\n"
			+ smszFldConn       + " INT,\n"
			+ smszFldDrvMode    + " INT,\n"
			+ smszFldLgNum      + " INT,\n"
			+ smszFldGuiLstIns  + " INT,\n"
			+ smszFldDevId      + " INT,\n"
			+ smszFldRSyncOff   + " INT,\n"
			+ smszFldRSyncDelta + " INT,\n"
			+ smszFldEntries    + " INT,\n"
			+ smszFldSectors    + " INT,\n"
			+ smszFldPctDone    + " INT,\n"
			+ smszFldPctBab     + " INT,\n"
			+ smszFldInstName   + " VARCHAR(32),\n"

			"CONSTRAINT PerfPk PRIMARY KEY ( " + smszFldIk + "),\n"
		")\n"   
	);

	return pszCreateTbl;

} // FsTdmfRecPerf::FtdGetCreate ()


FsTdmfRecPerf::~FsTdmfRecPerf ()
{

} // FsTdmfRecPerf::~FsTdmfRecPerf ()


BOOL FsTdmfRecPerf::FtdNew ( int iTimeStamp )//N seconds since 01-01-1970
{
    CTime ts(iTimeStamp);

    return FtdNew( ts.Format("%Y-%m-%d %H:%M:%S") );

} // FsTdmfRecPerf::FtdNew ()


BOOL FsTdmfRecPerf::FtdNew ( CString pszTs )
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
			"FsTdmfRecPerf::FtdNew(): "
			"Db not openned, cannot create new record with <" + pszTs + ">"
		);

		return FALSE;
	}

    //ac 2002-11-28 : remove constraint on smszFldPairId because Collector does not have access to a unique Pair identificator when creating a record
	//CString lszPairFk = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldPairId );
	CString lszGrpFk  = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldLgGrpId );
	CString lszSrvFk  = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldSrcFk );

	if ( /*lszPairFk == "" ||*/ lszGrpFk == "" || lszSrvFk == "" )
	{
		mpDb->FtdErr( 
			"FsTdmfRecPerf::FtdNew(): No current " 
			+ FsTdmfRecLgGrp::smszTblName + " record" 
		);
		return FALSE;
	}
	else try
	{
		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " 
				//+ smszFldPairFk + ", " 
                + smszFldGrpFk + ", " 
				+ smszFldSrcFk  + ", " + smszFldTs +
			" ) "
			"VALUES ( " 
				//+ lszPairFk + ", "  
                + lszGrpFk + ", " 
				+ lszSrvFk  + ", '" + pszTs    + "' )"
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecPerf::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		//"WHERE " + smszFldPairFk + " = " + lszPairFk + " "
		//"AND "   + smszFldGrpFk + " = "  + lszGrpFk + " "
		"WHERE " + smszFldGrpFk + " = "  + lszGrpFk + " "
		"AND "   + smszFldSrcFk + " = "  + lszSrvFk + " "
		"AND "   + smszFldTs    + " = '" + pszTs + "'"
	);

	return FtdOpen( lszSel );

} // FsTdmfRecPerf::FtdNew ( CString pszName )

// optimized version to allow record insertion using only one SQL query
BOOL FsTdmfRecPerf::FtdNew ( int        timestamp,
                             __int64    actual,
                             __int64    effective,
                             __int64    bytesread,
                             __int64    byteswritten,
                             char       role,
                             int        connection,
                             int        drvmode,
                             int        lgnum,
                             int        insert,
                             int        devid,
                             int        rsyncoff,
                             int        rsyncdelta,
                             int        entries,
                             int        sectors,
                             int        pctdone,
                             int        pctbab,
                             const char *szInstName 
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
        CTime   ts(timestamp);

		mpDb->FtdErr( 
			"FsTdmfRecPerf::FtdNew(): "
			"Db not openned, cannot create new record with <" + ts.Format("%Y-%m-%d %H:%M:%S") + ">"
		);

		return FALSE;
	}

	//CString lszPairFk = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldPairId );
	CString lszGrpFk  = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldLgGrpId );
	CString lszSrvFk  = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldSrcFk );

	if ( /*lszPairFk == "" ||*/ lszGrpFk == "" || lszSrvFk == "" )
	{
		mpDb->FtdErr( 
			"FsTdmfRecPerf::FtdNew(): No current " 
			+ FsTdmfRecLgGrp::smszTblName + " record" 
		);
		return FALSE;
	}
	else try
	{
        CTime   ts(timestamp);
        CString lszSql;
        //order of parameters in variable arg list MUST match the format within smszInsertQueryFrmt
        lszSql.Format( smszInsertQueryFrmt, devid,//(LPCTSTR)lszPairFk,
                                            (LPCTSTR)lszGrpFk,
                                            (LPCTSTR)lszSrvFk,
                                            (LPCTSTR)ts.Format("%Y-%m-%d %H:%M:%S"),
                                            actual,
                                            effective,
                                            bytesread,
                                            byteswritten,
                                            role,
                                            connection,
                                            drvmode,
                                            lgnum,
                                            insert,
                                            devid,
                                            rsyncoff,
                                            rsyncdelta,
                                            entries,
                                            sectors,
                                            pctdone,
                                            pctbab,
                                            szInstName 
                                            );

		mpDb->ExecuteSQL ( lszSql );
        return TRUE;
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecPerf::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}
}

// Position the current record using the Internal Key
BOOL FsTdmfRecPerf::FtdPos ( CString pszIk )
{
	return FtdPos( atoi ( pszIk ) );

} // FsTdmfRecPerf::FtdPos ()


BOOL FsTdmfRecPerf::FtdPos ( int piIk )
{
	return FsTdmfRec::FtdPos( piIk, smszFldIk );

} // FsTdmfRecPerf::FtdPos ()


BOOL FsTdmfRecPerf::FtdUpd ( CString pszFldName, CString pszVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, pszVal, smszFldIk );

} // FsTdmfRecPerf::FtdUpd ()


BOOL FsTdmfRecPerf::FtdUpd ( CString pszFldName, int piVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, piVal, smszFldIk );

} // FsTdmfRecPerf::FtdUpd ()
