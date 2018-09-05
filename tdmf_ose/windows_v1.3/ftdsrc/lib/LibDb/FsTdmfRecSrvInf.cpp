// FsTdmfRecSrvInf.cpp
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"
#include "FsTdmfRecSrvInf.h"
#include "FsTdmfRecDom.h"

CString FsTdmfRecSrvInf::smszTblName        = "t_ServerInfo";				// DbVersion = "1.3.2"

CString FsTdmfRecSrvInf::smszFldSrvId       = "SrvId";						// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldDomFk       = "Domain_Fk";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldName        = "ServerName";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldState       = "State";						// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldHostId      = "HostId";						// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldSrvIp1      = "ServerIp1";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldSrvIp2      = "ServerIp2";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldSrvIp3      = "ServerIp3";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldSrvIp4      = "ServerIp4";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldIpPort      = "IpPort";						// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldDefaultJournalVol = "DefaultJournalVolume";	// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldDefaultPStoreFile = "DefaultPStoreFile";	// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldBabSizeReq  = "BabSizeReq";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldBabSizeAct  = "BabSizeAct";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldTcpWinSize  = "TcpWinSize";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldOsType      = "OsType";						// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldOsVersion   = "OsVersion";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldTdmfVersion = "TdmfVersion";				// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldKey         = "KeyCurrent";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldKeyOld      = "KeyOld";						// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldKeyExpire   = "KeyExpiration";				// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldNumberOfCpu = "NumberOfCpu";				// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldRamSize     = "RamSize";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldRtrIp1      = "RouterIp1";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldRtrIp2      = "RouterIp2";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldRtrIp3      = "RouterIp3";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldRtrIp4      = "RouterIp4";					// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldNotes       = "Notes";						// DbVersion = "1.3.2"
CString FsTdmfRecSrvInf::smszFldReplicationPort = "ReplicationPort";		// DbVersion = "1.3.5"

///////////////////////////////////////////////////////////
// Removed in ver 1.3.3
//
//CString FsTdmfRecSrvInf::smszFldChunkSize   = "ChunkSize";			    // DbVersion = "1.3.2"
//
///////////////////////////////////////////////////////////

FsTdmfRecSrvInf::FsTdmfRecSrvInf ( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{

} // FsTdmfRecSrvInf::FsTdmfRecSrvInf ()


BOOL FsTdmfRecSrvInf::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecSrvInf::FtdFirst ()


BOOL FsTdmfRecSrvInf::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecSrvInf::FtdFirstFast ()


// If pszWhere is null then it is a sub select
CString FsTdmfRecSrvInf::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecSrvInf::FtdFirstCurs(): Db not openned, cannot init"
		);
		return "";
	}

	CString lszSqlWhere = "WHERE ";

	if ( pszWhere == "" )
	{
		CString lszDomKa = mpDb->mpRecDom->FtdSel( FsTdmfRecDom::smszFldKa );

		if ( lszDomKa == "" )
		{
			mpDb->FtdErr( 
				"FsTdmfRecSrvInf::FtdFirstCurs(): No current Domain record" 
			);
			return "";
		}

		lszSqlWhere += smszFldDomFk + " = " + lszDomKa;
	}
	else
	{
		lszSqlWhere += pszWhere;
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

} // FsTdmfRecSrvInf::FtdFirstCurs ()


CString FsTdmfRecSrvInf::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"( "
			+ smszFldSrvId  + " INT          IDENTITY NOT NULL,\n"
			+ smszFldDomFk  + " INT                   NOT NULL,\n"
			+ smszFldName   + " VARCHAR( 30)          NOT NULL DEFAULT '.',\n"
			+ smszFldHostId + " DEC                   NOT NULL DEFAULT 0,\n"
			+ smszFldState  + " INT                   NOT NULL DEFAULT 0,\n"
			+ smszFldKey    + " VARCHAR( 40),\n"
			+ smszFldKeyOld + " VARCHAR( 40),\n"
			+ smszFldKeyExpire + " DATETIME,\n"
            // v1.3.2 : ip uses DECIMAL instead of INT because MS SQL object badly handles numbers >= 0x80000000
			+ smszFldSrvIp1 + " DECIMAL                   NOT NULL DEFAULT   0,\n"
			+ smszFldSrvIp2 + " DECIMAL                            DEFAULT   0,\n"
			+ smszFldSrvIp3 + " DECIMAL                            DEFAULT   0,\n"
			+ smszFldSrvIp4 + " DECIMAL                            DEFAULT   0,\n"
			+ smszFldIpPort + " SMALLINT                  NOT NULL DEFAULT 575,\n"

			+ smszFldDefaultJournalVol + " VARCHAR(300),\n"
			+ smszFldDefaultPStoreFile + " VARCHAR(300),\n"
			+ smszFldBabSizeReq  + " INT              NOT NULL DEFAULT  32,\n"
			+ smszFldBabSizeAct  + " INT,\n"
			+ smszFldTcpWinSize  + " INT              NOT NULL DEFAULT 256,\n"
			+ smszFldOsType      + " VARCHAR( 30),\n"
			+ smszFldOsVersion   + " VARCHAR( 50),\n"
			+ smszFldTdmfVersion + " VARCHAR( 40),\n"
			+ smszFldRamSize     + " INT,\n"

			+ smszFldNumberOfCpu + " SMALLINT         NOT NULL DEFAULT   1,\n"

            // v1.3.2 : ip uses DECIMAL instead of INT because MS SQL object badly handles numbers >= 0x80000000
            + smszFldRtrIp1     + " DECIMAL              DEFAULT   0,\n"
			+ smszFldRtrIp2     + " DECIMAL              DEFAULT   0,\n"
			+ smszFldRtrIp3     + " DECIMAL              DEFAULT   0,\n"
			+ smszFldRtrIp4     + " DECIMAL              DEFAULT   0,\n"

			+ smszFldNotes      + " VARCHAR(256),\n"
			+ smszFldReplicationPort  + " SMALLINT       NOT NULL DEFAULT 575,\n"


			"CONSTRAINT ServerPk PRIMARY KEY ( " + smszFldSrvId + " ),\n"
			"CONSTRAINT ServerFk FOREIGN KEY ( " + smszFldDomFk + " ) "
				"REFERENCES " + FsTdmfRecDom::smszTblName + " "
					" ( " + FsTdmfRecDom::smszFldKa + " ),\n"
			"CONSTRAINT ServerUk UNIQUE (\n" 
				+ smszFldHostId + ", "  
                + smszFldName   + 
			" )\n"
		")\n"
	);

	return pszCreateTbl;

} // FsTdmfRecSrvInf::FtdGetCreate ()


FsTdmfRecSrvInf::~FsTdmfRecSrvInf ()
{

} // FsTdmfRecSrvInf::~FsTdmfRecSrvInf ()

// pszName is ServerName
BOOL FsTdmfRecSrvInf::FtdNew ( CString pszName )
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
			"FsTdmfRecSrvInf::FtdNew(): "
			"Db not openned, cannot create new record with <" + pszName + ">"
		);

		return FALSE;
	}

	CString lszDomKa = mpDb->mpRecDom->FtdSel( FsTdmfRecDom::smszFldKa );

	if ( lszDomKa == "" )
	{
		mpDb->FtdErr( 
			"FsTdmfRecSrvInf::FtdNew(): No current Domain record" 
		);
		return FALSE;
	}
	else try
	{
		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " 
				+ smszFldDomFk + ", " + smszFldName + 
			" ) "
			"VALUES ( " + lszDomKa + ", '" + pszName + "' )"
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecSrvInf::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldName + " = '" + pszName + "'"
	);

	return FtdOpen( lszSel );

} // FsTdmfRecSrvInf::FtdNew ( CString pszName )


BOOL FsTdmfRecSrvInf::FtdNew ( int piHostId )
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
			"FsTdmfRecSrvInf::FtdNew(): "
			"Db not openned, cannot create new record with <%ld>",
			piHostId
		);

		return FALSE;
	}

	CString lszDomKa = mpDb->mpRecDom->FtdSel( FsTdmfRecDom::smszFldKa );

	if ( lszDomKa == "" )
	{
		mpDb->FtdErr( 
			"FsTdmfRecSrvInf::FtdNew(): No current Domain record" 
		);
		return FALSE;
	}
	else try
	{
		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " 
				+ smszFldDomFk + ", " + smszFldHostId + 
			" ) "
			"VALUES ( " + lszDomKa + ", %ld )",
			piHostId
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecSrvInf::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"WHERE " + smszFldHostId + " = %ld",
		piHostId
	);

	return FtdOpen( lszSel );

} // FsTdmfRecSrvInf::FtdNew ( int )


// Position the current record using the Server Name
BOOL FsTdmfRecSrvInf::FtdPos ( CString pszName )
{
	return FsTdmfRec::FtdPos( pszName, smszFldName );

} // FsTdmfRecSrvInf::FtdPos ()


BOOL FsTdmfRecSrvInf::FtdPos ( int piHostId )
{
	return FsTdmfRec::FtdPos( piHostId, smszFldHostId );

} // FsTdmfRecSrvInf::FtdPos ()


BOOL FsTdmfRecSrvInf::FtdUpd ( CString pszFldName, CString pszVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, pszVal, smszFldSrvId );

} // FsTdmfRecSrvInf::FtdUpd ()


BOOL FsTdmfRecSrvInf::FtdUpd ( CString pszFldName, int piVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, piVal, smszFldSrvId );

} // FsTdmfRecSrvInf::FtdUpd ()


