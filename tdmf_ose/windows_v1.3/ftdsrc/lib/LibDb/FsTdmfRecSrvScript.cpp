// FsTdmfRecSrvScript.cpp: implementation of the FsTdmfRecSrvScript class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FsTdmfDb.h"
#include "FsTdmfRecSrvScript.h"
#include "FsTdmfRecSrvInf.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CString FsTdmfRecSrvScript::smszTblName			= "t_ScriptServerFile";		// DbVersion = "1.3.4"

CString FsTdmfRecSrvScript::smszFldScriptSrvId		= "ScriptSrvId";	// DbVersion = "1.3.4"
CString FsTdmfRecSrvScript::smszFldSrvId		    = "SrvId";			// DbVersion = "1.3.4"
CString FsTdmfRecSrvScript::smszFldFileName         = "File_Name";		// DbVersion = "1.3.4"
CString FsTdmfRecSrvScript::smszFldFileType         = "File_Type";		// DbVersion = "1.3.4"
CString FsTdmfRecSrvScript::smszFldFileExtension    = "File_Extension";	// DbVersion = "1.3.4"
CString FsTdmfRecSrvScript::smszFldCreationDate     = "Creation_Date";	// DbVersion = "1.3.4"
CString FsTdmfRecSrvScript::smszFldModificationDate = "Modification_Date";	// DbVersion = "1.3.4"
CString FsTdmfRecSrvScript::smszFldFileContent      = "File_Content";	// DbVersion = "1.3.4"
 
   

FsTdmfRecSrvScript::FsTdmfRecSrvScript( FsTdmfDb* pDb ) : FsTdmfRec( pDb, smszTblName )
{

}

FsTdmfRecSrvScript::~FsTdmfRecSrvScript()
{

}

BOOL FsTdmfRecSrvScript::FtdFirst ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpen( lszSql );

} // FsTdmfRecSrvScript::FtdFirst ()


BOOL FsTdmfRecSrvScript::FtdFirstFast ( CString pszWhere, CString pszSort )
{
	CString lszSql =  FtdFirstCurs ( pszWhere, pszSort );

	if ( lszSql == "" )
	{
		return FALSE;
	}

	return FtdOpenFast( lszSql );

} // FsTdmfRecSrvScript::FtdFirstFast ()

// If pszWhere is null then it is a sub select
CString FsTdmfRecSrvScript::FtdFirstCurs ( CString pszWhere, CString pszSort )
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
			"FsTdmfRecSrvScript::FtdFirstCurs(): Db not openned, cannot init"
		);
		return "";
	}

	CString lszSqlWhere = "WHERE ";

	if ( pszWhere != "" )
	{
			CString lszServerPK = mpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvId );

		if ( lszServerPK == "" )
		{
			mpDb->FtdErr( 
				"FsTdmfRecSrvInf::FtdFirstCurs(): No current Domain record" 
			);
			return "";
		}

		lszSqlWhere += smszFldSrvId + " = " + lszServerPK;
	}
    else
	{
		lszSqlWhere += pszWhere;
	}

	CString lszSort;

	if ( pszSort == "" )
	{
		lszSort = smszFldFileName + " DESC ";
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

} // FsTdmfRecSrvScript::FtdFirstCurs ()

CString FsTdmfRecSrvScript::FtdGetCreate ()
{
	CString pszCreateTbl;
	pszCreateTbl.Format (
		"CREATE TABLE dbo." + mszTblName + "\n"
		"( "
            + smszFldScriptSrvId        + " INT     IDENTITY    NOT NULL,\n"
			+ smszFldSrvId              + " INT                 NOT NULL,\n"
			+ smszFldFileName           + " VARCHAR(80)         NOT NULL,\n"
			+ smszFldFileType           + " VARCHAR(30)         NOT NULL,\n"
			+ smszFldFileExtension      + " VARCHAR(3)                  ,\n"
            + smszFldCreationDate       + " DATETIME            NOT NULL,\n"
            + smszFldModificationDate   + " DATETIME                    ,\n"
  			+ smszFldFileContent        + " VARCHAR(7800)    NOT NULL   ,\n"
  
           	"CONSTRAINT ScriptServerPk PRIMARY KEY ( " + smszFldScriptSrvId + " ),\n"
		    "CONSTRAINT ScriptServerFk FOREIGN KEY ( " + smszFldSrvId + " ) "
				"REFERENCES " + FsTdmfRecSrvInf::smszTblName + " "
					" ( " + FsTdmfRecSrvInf::smszFldSrvId + " )\n"
		")\n"
	);

 	return pszCreateTbl;

} // FsTdmfRecSrvScript::FtdGetCreate ()

//Add new Record
BOOL FsTdmfRecSrvScript::FtdNew (int ServerID , 
                                 CString szFileName, 
                                 CString szFileType, 
                                 CString szFileExtension,
                                 CString szCreationDate, 
                                 CString SzFileContent  )
{
	if (   this == NULL 
		|| mpDb == NULL
	   )
	{
		return FALSE;
	}
	else try
	{
        CString szServerID;
        szServerID.Format("%d",ServerID);

		SzFileContent.Replace("'", "''");
        SzFileContent.Replace("\"\"", "\"");
		SzFileContent.Replace("%", "%%");

		CString lszSql;
		lszSql.Format ( 
			"INSERT INTO " + mszTblName + " ( " + 
            smszFldSrvId + ", " + 
            smszFldFileName + ", " + 
            smszFldFileType + ", " + 
            smszFldFileExtension + ", " +
            smszFldCreationDate + " , " +
            smszFldFileContent + " ) " + "VALUES ( " + 
            szServerID + ", '" + 
            szFileName + "', '" + 
            szFileType + "', '" + 
            szFileExtension + "', '" +
            szCreationDate + "', '" +
            SzFileContent + "' )"
		);

		mpDb->ExecuteSQL ( lszSql );
	}
	catch ( CDBException* e )
	{
		mpDb->FtdErr( "FsTdmfRecSrvScript::FtdNew(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	CString lszSel;
	lszSel.Format ( 
		"SELECT * FROM " + mszTblName + " "
		"ORDER BY " + smszFldScriptSrvId + " Desc "
	);

	return FtdOpen( lszSel );
} // FsTdmfRecSrvScript::FtdNew ( CString pszName )


BOOL FsTdmfRecSrvScript::FtdUpd ( CString pszFldName, CString pszVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, pszVal, smszFldScriptSrvId );

} // FsTdmfRecSrvScript::FtdUpd ()


BOOL FsTdmfRecSrvScript::FtdUpd ( CString pszFldName, int piVal )
{
	return FsTdmfRec::FtdUpd( pszFldName, piVal, smszFldScriptSrvId );

} // FsTdmfRecSrvScript::FtdUpd ()

BOOL FsTdmfRecSrvScript::FtdPos (int   nScriptServerId )
{
	return FsTdmfRec::FtdPos( nScriptServerId, smszFldScriptSrvId );

} // FsTdmfRecSrvScript::FtdPos ()
