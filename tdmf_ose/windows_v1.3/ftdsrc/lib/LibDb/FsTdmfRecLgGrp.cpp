// FsTdmfRecLgGrp.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecLgGrp.h"
#include "FsTdmfRecSrvInf.h"

CString FsTdmfRecLgGrp::smszTblName					= "t_LogicalGroup";			// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldIk					= "Ik"; // Internal Key		// DbVersion = "1.3.2"

CString FsTdmfRecLgGrp::smszFldLgGrpId				= "LgGroupId";				// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldSrcFk				= "Source_Fk";				// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldTgtFk				= "Target_Fk";				// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldPStore				= "PStoreVolume";			// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldJournalVol			= "JournalVolume";			// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldChainning			= "Chainning";				// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldNotes				= "Notes";					// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldChunkDelay			= "ChunkDelay";				// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldChunkSize			= "ChunkSize";				// DbVersion = "1.3.3"

CString FsTdmfRecLgGrp::smszFldSyncMode				= "SyncMode";				// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldSyncModeDepth		= "SyncModeDepth";			// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldSyncModeTimeOut		= "SyncModeTimeOut";		// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldRefreshNeverTimeOut	= "RefreshNeverTimeOut";	// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldRefreshTimeOut		= "RefreshTimeOut";			// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldEnableCompression	= "EnableCompression";		// DbVersion = "1.3.3"

CString FsTdmfRecLgGrp::smszFldNetUsageThreshold	= "NetUsageThreshold";		// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldNetUsageValue		= "NetUsageValue";			// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldUpdateInterval		= "UpdateInterval";			// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldMaxFileStatSize		= "MaxFileStatSize";		// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldJournalLess          = "JournalLess";            // DbVersion = "1.3.7"

CString FsTdmfRecLgGrp::smszFldConnection			= "Connection";				// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldStateTimeStamp		= "State_Time_Stamp";		// DbVersion = "1.3.2"
CString FsTdmfRecLgGrp::smszFldTgtDHCPNameUsed		= "TargetDHCPNameUsed";		// DbVersion = "1.3.4"
CString FsTdmfRecLgGrp::smszFldPrimaryDHCPNameUsed	= "PrimaryDHCPNameUsed";	// DbVersion = "1.3.4" 
CString FsTdmfRecLgGrp::smszFldPrimaryEditedIPUsed	= "PrimaryEditedIPUsed";	// DbVersion = "1.3.4" 
CString FsTdmfRecLgGrp::smszFldTgtEditedIPUsed		= "TgtEditedIPUsed";        // DbVersion = "1.3.4" 

CString FsTdmfRecLgGrp::smszFldThrottles			= "Throttles";				// DbVersion = "1.3.5"

CString FsTdmfRecLgGrp::smszFldPrimaryEditedIP				= "PrimaryEditedIP";		// DbVersion = "1.3.6"
CString FsTdmfRecLgGrp::smszFldTgtEditedIP					= "TargetEditedIP";			// DbVersion = "1.3.6"

CString FsTdmfRecLgGrp::smszFldSymmetric            = "Symmetric";	                    // DbVersion = "1.3.8"
CString FsTdmfRecLgGrp::smszFldSymmetricGroupNumber = "SymmetricGroupNumber";           // DbVersion = "1.3.8"
CString FsTdmfRecLgGrp::smszFldSymmetricNormallyStarted = "SymmetricNormallyStarted";	// DbVersion = "1.3.8"
CString FsTdmfRecLgGrp::smszFldFailoverInitialState = "FailoverInitialState";	        // DbVersion = "1.3.8"
CString FsTdmfRecLgGrp::smszFldSymmetricJournalDirectory = "SymmetricJournalDirectory";	// DbVersion = "1.3.8"
CString FsTdmfRecLgGrp::smszFldSymmetricPStoreFile  = "SymmetricPStoreFile";	        // DbVersion = "1.3.8"


FsTdmfRecLgGrp::FsTdmfRecLgGrp ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{
	mszTblName = smszTblName;

} // FsTdmfRecLgGrp::FsTdmfRecLgGrp ()


BOOL FsTdmfRecLgGrp::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecLgGrp::FtdFirst ()


BOOL FsTdmfRecLgGrp::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecLgGrp::FtdFirstFast ()


// If pszWhere is null then it is a sub select
CString FsTdmfRecLgGrp::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecLgGrp::FtdFirstCurs(): Db not openned, cannot init"
		);
		return "";
	}

	CString lszSqlWhere = "WHERE ";

	if ( pszWhere == "" )
	{
		CString lszSrvId = 
			mpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvId );

		if ( lszSrvId == "" )
		{
			mpDb->FtdErr( 
				"FsTdmfRecLgGrp::FtdFirstCurs(): No current Server Info record" 
			);
			return "";
		}

		lszSqlWhere += smszFldSrcFk + " = " + lszSrvId;
	}
	else
	{
		lszSqlWhere += pszWhere;
	}

	CString lszSort;

	if ( pszSort == "" )
	{
		lszSort = smszFldLgGrpId;
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

} // FsTdmfRecLgGrp::FtdFirstCurs ()


CString FsTdmfRecLgGrp::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"( "
			+ smszFldIk                + " INT          IDENTITY NOT NULL,\n"
			+ smszFldLgGrpId           + " SMALLINT              NOT NULL,\n"
			+ smszFldSrcFk             + " INT                   NOT NULL,\n"
			+ smszFldTgtFk             + " INT,\n"
			+ smszFldPStore            + " VARCHAR(300),\n"
			+ smszFldJournalVol        + " VARCHAR(300),\n"
			+ smszFldChainning         + " BIT,\n"
			+ smszFldNotes             + " VARCHAR(256),\n"
			+ smszFldThrottles         + " VARCHAR(7000),\n"
			+ smszFldChunkDelay        + " INT,\n"
			+ smszFldChunkSize         + " INT,\n"
			+ smszFldEnableCompression + " BIT,\n"
			+ smszFldSyncMode          + " BIT,\n"
			+ smszFldSyncModeDepth     + " INT,\n"
			+ smszFldSyncModeTimeOut   + " INT,\n"
			+ smszFldRefreshNeverTimeOut+ " BIT,\n"
			+ smszFldRefreshTimeOut    + " INT,\n"
			+ smszFldNetUsageThreshold + " BIT,\n"
			+ smszFldNetUsageValue     + " INT,\n" 
			+ smszFldUpdateInterval    + " INT,\n" // Second
			+ smszFldMaxFileStatSize   + " INT,\n" // Kb
			+ smszFldJournalLess       + " BIT,\n"

			+ smszFldConnection        + " INT                 NOT NULL DEFAULT 0,\n"
			+ smszFldStateTimeStamp    + " DATETIME,\n"
            + smszFldPrimaryDHCPNameUsed   + " INT             NOT NULL DEFAULT 0,\n"
            + smszFldPrimaryEditedIPUsed   + " INT             NOT NULL DEFAULT 0,\n"
			+ smszFldPrimaryEditedIP       + " DECIMAL         NOT NULL DEFAULT   0,\n"
			+ smszFldTgtDHCPNameUsed   + " INT                 NOT NULL DEFAULT 0,\n"
          	+ smszFldTgtEditedIPUsed   + " INT                 NOT NULL DEFAULT 0,\n"
			+ smszFldTgtEditedIP       + " DECIMAL             NOT NULL DEFAULT   0,\n"
			+ smszFldSymmetric         + " BIT                 NOT NULL DEFAULT 0,\n"
			+ smszFldSymmetricGroupNumber + " INT              NOT NULL DEFAULT 0,\n"
			+ smszFldSymmetricNormallyStarted + " BIT          NOT NULL DEFAULT 0,\n"
			+ smszFldFailoverInitialState + " INT              NOT NULL DEFAULT 0,\n"
			+ smszFldSymmetricJournalDirectory + " VARCHAR(300),\n"
			+ smszFldSymmetricPStoreFile       + " VARCHAR(300),\n"

			"CONSTRAINT LgPk     PRIMARY KEY ( " 
				+ smszFldLgGrpId + ", " + smszFldSrcFk + " ),\n"

			"CONSTRAINT SrcFk    FOREIGN KEY ( " + smszFldSrcFk + " ) "
				"REFERENCES " + FsTdmfRecSrvInf::smszTblName + 
					" ( " + FsTdmfRecSrvInf::smszFldSrvId + " ),\n"
			"CONSTRAINT TargetFk FOREIGN KEY ( " + smszFldTgtFk + " ) "
				"REFERENCES " + FsTdmfRecSrvInf::smszTblName + 
					" ( " + FsTdmfRecSrvInf::smszFldSrvId + " )\n"
		")\n"
	);

	return pszCreateTbl;

} // FsTdmfRecLgGrp::FtdGetCreate ()


FsTdmfRecLgGrp::~FsTdmfRecLgGrp ()
{

} // FsTdmfRecLgGrp::~FsTdmfRecLgGrp ()

// pszName is LgGroupId
BOOL FsTdmfRecLgGrp::FtdNew ( CString pszGrpId )
{
	return FtdNew( atoi ( pszGrpId ) );

} // FsTdmfRecLgGrp::FtdNew ( CString pszName )


BOOL FsTdmfRecLgGrp::FtdNew ( int piGrpId )
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
			"FsTdmfRecLgGrp::FtdNew(): "
			"Db not openned, cannot create new record with <%ld>",
			piGrpId
		);

		return FALSE;
	}

	CString lszSrvFk = mpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvId );

	if ( lszSrvFk == "" )
	{
		mpDb->FtdErr( 
			"FsTdmfRecLgGrp::FtdNew(): No current " 
			+ FsTdmfRecSrvInf::smszTblName + " record" 
		);
		return FALSE;
	}
	else try
	{
		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " 
				+ smszFldSrcFk + ", " + smszFldLgGrpId + 
			" ) "
			"VALUES ( " + lszSrvFk + ", %ld )",piGrpId
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecLgGrp::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldSrcFk   + " = " + lszSrvFk + " "
		"AND "   + smszFldLgGrpId + " = %ld", piGrpId
	);

	return FtdOpen( lszSel );

} // FsTdmfRecLgGrp::FtdNew ( int )


// Position the current record using the Group Id
BOOL FsTdmfRecLgGrp::FtdPos ( CString pszName )
{
	return FtdPos( atoi ( pszName ) );

} // FsTdmfRecLgGrp::FtdPos ()


// Position the current record using the Group Id
BOOL FsTdmfRecLgGrp::FtdPos ( int piId )
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
			"FsTdmfRecLgGrp::FtdPos(): "
			"Db not openned, cannot pos record with <%d>",piId
		);

		return FALSE;
	}

	CString lszSrvFk = mpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvId );

	if ( lszSrvFk == "" )
	{
		mpDb->FtdErr( 
			"FsTdmfRecLgGrp::FtdNew(): No current " 
			+ FsTdmfRecSrvInf::smszTblName + " record" 
		);
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldSrcFk   + " = " + lszSrvFk + " "
		"AND "   + smszFldLgGrpId + " = %ld",piId
	);

	return FtdOpen( lszSel );

} // FsTdmfRecLgGrp::FtdPos ()


BOOL FsTdmfRecLgGrp::FtdUpd ( CString pszFldName, CString pszVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, pszVal, smszFldIk );

} // FsTdmfRecLgGrp::FtdUpd ()


BOOL FsTdmfRecLgGrp::FtdUpd ( CString pszFldName, int piVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, piVal, smszFldIk );

} // FsTdmfRecLgGrp::FtdUpd ()


int FsTdmfRecLgGrp::FtdGetTargetReplicationPairCount(int nTargetServerFk)
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
	//select count(*) from t_LogicalGroup LG, t_ReplicationPair RP
	//where Target_Fk = 1 and LG.LgGroupId = RP.Group_Fk and LG.Source_Fk = RP.Source_Fk
	cszSQL.Format (
		"SELECT COUNT(*) FROM " + mszTblName + " LG, " + FsTdmfRecPair::smszTblName + " RP "
		"WHERE " + smszFldTgtFk + " = %ld "
		"AND LG." + smszFldLgGrpId + " = RP." + FsTdmfRecPair::smszFldGrpFk + " "
		"AND LG." + smszFldSrcFk + " = RP." + FsTdmfRecPair::smszFldSrcFk, nTargetServerFk
	);

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
    // Close the cursor.
    ::SQLCloseCursor(hstmt);

	::SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    return (int)NRecords;

} // // FsTdmfRecLgGrp::FtdTargetReplicationPairCount ()


