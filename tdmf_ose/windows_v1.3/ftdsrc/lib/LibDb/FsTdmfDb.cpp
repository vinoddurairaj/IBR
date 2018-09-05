// FsTdmfDb.cpp
// Fujitsu Software DataBase Tdmf
//
//	This is an API to access the Tdmf DataBase
//

#include "stdafx.h"

#include "FsTdmfDb.h"


CString FsTdmfDb::mszDbVersion = "1.3.8";


FsTdmfDb::FsTdmfDb ( CString pszUsr, CString pszPwd, CString pszSrv, CString pszDb, UINT nPort ) : CDatabase ()
{
	mbFtdErrPop = FALSE;

	try
	{
		FtdOpenEx( pszUsr, pszPwd, pszSrv, pszDb, nPort );
	}
	catch ( FsExDb e )
	{
		FtdErr( "FsTdmfDb::FsTdmfDb():Instanciated but not opened" );
	}

	//SQLSMALLINT  InfoValuePtr;
	//SQLSMALLINT  BufferLength;
	//SQLSMALLINT* StringLengthPtr;

	//SQLGetInfo ( m_hdbc, SQL_MAX_CONCURRENT_ACTIVITIES, (SQLPOINTER)&InfoValuePtr, NULL, NULL );

	mpRecDom    = new FsTdmfRecDom    ( this );
	mpRecSrvInf = new FsTdmfRecSrvInf ( this );
	mpRecGrp    = new FsTdmfRecLgGrp  ( this );
	mpRecPair   = new FsTdmfRecPair   ( this );
	mpRecAlert  = new FsTdmfRecAlert  ( this );
	mpRecPerf   = new FsTdmfRecPerf   ( this );

	mpRecNvp    = new FsTdmfRecNvp ( this );
	mpRecHum    = new FsTdmfRecHum ( this );
	mpRecCmd    = new FsTdmfRecCmd ( this );

	mpRecSysLog = new FsTdmfRecSysLog ( this );
	mpRecKeyLog = new FsTdmfRecKeyLog ( this );
    mpRecScriptSrv = new FsTdmfRecSrvScript ( this );
    m_hMutex = CreateMutex(0,0,0);

} // FsTdmfDb::FsTdmfDb ()


FsTdmfDb::~FsTdmfDb ()
{
	FtdClose();

    CloseHandle( m_hMutex );

	if ( mpRecDom != NULL )
	{
		delete mpRecDom;
	}

	if ( mpRecSrvInf != NULL )
	{
		delete mpRecSrvInf;
	}

	if ( mpRecGrp != NULL )
	{
		delete mpRecGrp;
	}

	if ( mpRecPair != NULL )
	{
		delete mpRecPair;
	}

	if ( mpRecAlert != NULL )
	{
		delete mpRecAlert;
	}

	if ( mpRecPerf != NULL )
	{
		delete mpRecPerf;
	}

	if ( mpRecNvp != NULL )
	{
		delete mpRecNvp;
	}

	if ( mpRecHum != NULL )
	{
		delete mpRecHum;
	}

	if ( mpRecCmd != NULL )
	{
		delete mpRecCmd;
	}

	if ( mpRecSysLog != NULL )
	{
		delete mpRecSysLog;
	}

	if ( mpRecKeyLog != NULL )
	{
		delete mpRecKeyLog;
	}

	if ( mpRecScriptSrv != NULL )
	{
		delete mpRecScriptSrv;
	}

} // FsTdmfDb::~FsTdmfDb ()


void FsTdmfDb::FtdClose ()
{
	if ( this )
	{
		if ( FtdIsOpen () )
		{
			Close();
		}
	}

} // FsTdmfDb::FtdClose ()


BOOL FsTdmfDb::FtdCreateDb ( CString pszDb , CString pszDbBasePath )
{
	if ( this == NULL )
	{
		return FALSE;
	}
	else if ( !FtdIsOpen() )
	{
		FtdErr( "FsTdmfDb::FtdCreateDb(): Cannot create <%s>", pszDb );
		return FALSE;
	}

	CString lszDdlDrop;
	lszDdlDrop.Format ( "DROP DATABASE %s", pszDb );

	// At Drop time, it is normal to receive certain errors.
	// Because sometime we try to drop a table that does not
	// exist.
	try
	{
		ExecuteSQL ( lszDdlDrop );
	}
	catch ( CDBException* e )
	{
		FtdErr( "CFsTdmfDb::FtdCreateDb(),CDBException: <%.256s>", e->m_strError );
		e->Delete ();
	}

    //ensure pszDbBasePath ends with a '\'
    if ( pszDbBasePath.Right(0).Compare("\\") != 0 && pszDbBasePath.Right(0).Compare("/") != 0 )
        pszDbBasePath.Insert(999,'\\');//append 

	CString lszDdlCreate;
	lszDdlCreate.Format ( 
		"CREATE DATABASE "+pszDb+" "
		"ON "
		"( "
			"NAME = "+pszDb+"_dat, "
			//"FILENAME = 'c:\\mssql7\\data\\"+pszDb+"dat.mdf', "
			"FILENAME = '"+pszDbBasePath+pszDb+"dat.mdf', "
			"SIZE = 10, "
			"MAXSIZE = 1000, "
			"FILEGROWTH = 5 "
		") "
		"LOG ON "
		"( "
			"NAME = '"+pszDb+"_log', "
			//"FILENAME = 'c:\\mssql7\\data\\"+pszDb+"log.ldf', "
            "FILENAME = '"+pszDbBasePath+pszDb+"log.ldf', "
			"SIZE = 5MB, "
//			"MAXSIZE = 25MB, "
			"FILEGROWTH = 5MB "
		") "
	);

	try
	{
		ExecuteSQL ( lszDdlCreate );
	}
	catch ( CDBException* e )
	{
		FtdErr( "CFsTdmfDb::FtdCreateDb(),CDBException: <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	return TRUE;

} // FsTdmfDb::FtdCreateDb ()


//////////////////////////////////////////////////////////////////////////////
//
// Provide a bDBOwner as true to have 'sp_addrolemember' called with 'db_owner'
//
//////////////////////////////////////////////////////////////////////////////
BOOL FsTdmfDb::FtdCreateLogins ( CString pszUsrName, CString pszPwd, CString pszDb, bool bDBOwner )
{
	return FtdCreateLoginsWithAccess(pszUsrName, pszPwd, pszDb, DB_ROLE_OWNER);
}

//////////////////////////////////////////////////////////////////////////////
//
// Should not use sp_adduser : has been replaced by sp_grantdbaccess
//
//////////////////////////////////////////////////////////////////////////////
BOOL FsTdmfDb::FtdCreateLoginsWithAccess( CString pszUsrName, CString pszPwd, CString pszDb, UINT nAccesFlag )
{
	if ( this == NULL )
	{
		return FALSE;
	}
	else if ( !FtdIsOpen() )
	{
		FtdErr(
			"FsTdmfDb::FtdCreateLogins(): "
			"Cannot create logins <"+pszUsrName+">"
		);
		return FALSE;
	}

	FtdDropUsr ( pszUsrName, pszDb );

	try
	{
	    CString lszQuery;

	    lszQuery.Format ( "EXEC sp_addlogin '"+pszUsrName+"', '"+pszPwd+"', '"+pszDb+"'" );
		ExecuteSQL ( lszQuery );

	    lszQuery.Format ( "USE "+pszDb+" EXEC sp_grantdbaccess '"+pszUsrName+"'" );	 
        ExecuteSQL ( lszQuery );

        if (nAccesFlag & DB_ROLE_OWNER)
        {
	        lszQuery.Format ( "USE "+pszDb+" EXEC sp_addrolemember  'db_owner' , '"+pszUsrName+"'" );
            ExecuteSQL ( lszQuery );
        }
		if (nAccesFlag & DB_ROLE_SECURITYADMIN)
        {
	        lszQuery.Format ( "USE "+pszDb+" EXEC sp_addrolemember  'db_securityadmin' , '"+pszUsrName+"'" );
            ExecuteSQL ( lszQuery );
        }
		if (nAccesFlag & DB_ROLE_ACCESSADMIN)
        {
	        lszQuery.Format ( "USE "+pszDb+" EXEC sp_addrolemember  'db_accessadmin' , '"+pszUsrName+"'" );
            ExecuteSQL ( lszQuery );
        }
		if (nAccesFlag & DB_ROLE_DATAREADER)
        {
	        lszQuery.Format ( "USE "+pszDb+" EXEC sp_addrolemember  'db_datareader' , '"+pszUsrName+"'" );
            ExecuteSQL ( lszQuery );
        }
		if (nAccesFlag & DB_SRV_ROLE_SECURITYADMIN)
        {
	        lszQuery.Format ( "EXEC sp_addsrvrolemember '"+pszUsrName+"', 'securityadmin'");
            ExecuteSQL ( lszQuery );
        }
	}
	catch ( CDBException* e )
	{
		FtdErr( 
			"CFsTdmfDb::FtdCreateLogins(),CDBException: <%.256s>", 
			e->m_strError 
		);
		e->Delete ();
		return FALSE;
	}

	return TRUE;

} // FsTdmfDb::FtdCreateLogins ()

FsTdmfDbUserInfo::TdmfRoles FsTdmfDb::FtdGetUserRole(CString cstrUserName)
{
	FsTdmfDbUserInfo::TdmfRoles eRole = FsTdmfDbUserInfo::TdmfRoleSupervisor; // By default, the user is a supervisor

	try
	{
		CRecordset Recordset(this);
		CString cstrSQL;

		// if user is member of db_, then he's an administrator.
		if (cstrUserName.GetLength() > 0)
		{
			cstrSQL.Format("SELECT IS_SRVROLEMEMBER ('securityadmin', '%s')", cstrUserName);
		}
		else
		{
			cstrSQL = "SELECT IS_SRVROLEMEMBER ('securityadmin')";
		}
		Recordset.Open(CRecordset::forwardOnly, cstrSQL, CRecordset::readOnly|CRecordset::executeDirect);

		CString cstrValue;
		Recordset.GetFieldValue((short)0, cstrValue);
		if (cstrValue == "1")
		{
			eRole = FsTdmfDbUserInfo::TdmfRoleAdministrator;
		}
		Recordset.Close();


		if (eRole != FsTdmfDbUserInfo::TdmfRoleAdministrator)
		{
			if (cstrUserName.GetLength() > 0)
			{
				// if user is member of db_datareader, then he's a normal user
				cstrSQL.Format("{CALL sp_helpuser('%s')}", cstrUserName);
				Recordset.Open(CRecordset::forwardOnly, cstrSQL, CRecordset::readOnly|CRecordset::executeDirect);
				while (!Recordset.IsEOF())
				{
					CString cstrGroupName;
					Recordset.GetFieldValue("GroupName",  cstrGroupName);
					if (cstrGroupName == "db_datareader")
					{
						eRole = FsTdmfDbUserInfo::TdmfRoleUser;
						break;
					}
					Recordset.MoveNext();
				}			
				Recordset.Close();
			}
			else
			{
				// if user is member of db_datareader, then he's a normal user
				cstrSQL = "SELECT IS_MEMBER ('db_datareader')";
				Recordset.Open(CRecordset::forwardOnly, cstrSQL, CRecordset::readOnly|CRecordset::executeDirect);

				CString cstrValue;
				Recordset.GetFieldValue((short)0, cstrValue);
				if (cstrValue == "1")
				{
					eRole = FsTdmfDbUserInfo::TdmfRoleUser;
				}
				Recordset.Close();
			}
		}
	}
	catch ( CDBException* e )
	{
		FtdErr( "CFsTdmfDb::FtdGetUserRole(),CDBException: <%.256s>", e->m_strError );
		e->Delete ();
		return FsTdmfDbUserInfo::TdmfRoleUser;
	}

	return eRole;
}

FsTdmfDbUserInfo::TdmfLoginTypes FsTdmfDb::FtdGetLoginType(LPCSTR lpcstrLogin)
{
	FsTdmfDbUserInfo::TdmfLoginTypes eType = FsTdmfDbUserInfo::TdmfLoginUndef;

	try
	{
		CRecordset Recordset(this);
		CString cstrSQL;

		CString cstrUserName;
		BOOL    bFound = FALSE;

		// Find user name
		cstrSQL.Format("{CALL sp_helplogins('%s')}", lpcstrLogin);
		Recordset.Open(CRecordset::forwardOnly, cstrSQL, CRecordset::readOnly|CRecordset::executeDirect);
		Recordset.FlushResultSet(); // Next recordset
		Recordset.MoveNext( ); // must call MoveNext because cursor is invalid
		while (!Recordset.IsEOF())
		{
			CString cstrUserOrAlias;
			Recordset.GetFieldValue((short)2/*"UserName"*/, cstrUserName);
			Recordset.GetFieldValue((short)3/*"UserOrAlias"*/, cstrUserOrAlias);
			if (strnicmp(cstrUserOrAlias, "User", 4) == 0)
			{
				bFound = TRUE;
				break;
			}
			Recordset.MoveNext();
		}			
		Recordset.Close();

		if (bFound)
		{
			cstrSQL.Format("SELECT isntgroup, isntuser, issqluser FROM sysusers WHERE name = '%s'", cstrUserName);
			Recordset.Open(CRecordset::forwardOnly, cstrSQL, CRecordset::readOnly|CRecordset::executeDirect);
			CString cstrIsNtGroup;
			CString cstrIsNtUser;
			CString cstrIsSqlUser;
			Recordset.GetFieldValue((short)0, cstrIsNtGroup);
			Recordset.GetFieldValue((short)1, cstrIsNtUser);
			Recordset.GetFieldValue((short)2, cstrIsSqlUser);
			Recordset.Close();

			if (cstrIsNtGroup == "1")
			{
				eType = FsTdmfDbUserInfo::TdmfLoginWindowsGroup;
			}
			else if (cstrIsSqlUser == "1")
			{
				eType = FsTdmfDbUserInfo::TdmfLoginStandard;
			}
			else
			{
				eType = FsTdmfDbUserInfo::TdmfLoginWindowsUser;
			}
		}
	}
	catch ( CDBException* e )
	{
		FtdErr( "CFsTdmfDb::FtdGetLogintype(),CDBException: <%.256s>", e->m_strError );
		e->Delete ();
		return FsTdmfDbUserInfo::TdmfLoginUndef;
	}

	return eType;
}


BOOL FsTdmfDb::FtdGetUsersInfo(std::list<FsTdmfDbUserInfo>& listUser)
{
	try
	{
		CRecordset Recordset(this);

		Recordset.Open(CRecordset::forwardOnly, "SELECT program_name, loginame, nt_username, hostname FROM master.dbo.sysprocesses",
					   CRecordset::readOnly|CRecordset::executeDirect);
		while (!Recordset.IsEOF())
		{
			CString cstrProgramName;
			CString cstrLogin;

			Recordset.GetFieldValue("program_name", cstrProgramName);
			cstrProgramName.TrimLeft();
			cstrProgramName.TrimRight();
			if ((cstrProgramName != "") && 
				(cstrProgramName != "TDMF OSE ® for Windows ®"))
			{
				FsTdmfDbUserInfo DbUserInfo;

				Recordset.GetFieldValue("loginame", DbUserInfo.m_cstrLogin);
				DbUserInfo.m_cstrLogin.TrimRight();

				try
				{
					Recordset.GetFieldValue("nt_username", DbUserInfo.m_cstrName);
					DbUserInfo.m_cstrName.TrimRight();
				}
				catch(CDBException* e)
				{
					e->Delete ();
				}
				if (DbUserInfo.m_cstrName == "")
				{
					DbUserInfo.m_cstrName = DbUserInfo.m_cstrLogin;
				}

				try
				{
					Recordset.GetFieldValue("hostname", DbUserInfo.m_cstrLocation);
					DbUserInfo.m_cstrLocation.TrimRight();
				}
				catch(CDBException* e)
				{
					e->Delete ();
				}

				DbUserInfo.m_cstrApp = cstrProgramName;

				//DbUserInfo.m_eRole = FsTdmfDbUserInfo::TdmfRoleUndef;

				listUser.push_back(DbUserInfo);
			}
			Recordset.MoveNext();
		}			
		Recordset.Close();

		// We can't open two recordset at the same time, so rescan list to set role
		for (std::list<FsTdmfDbUserInfo>::iterator it = listUser.begin();
			 it != listUser.end(); it++)
		{
			FsTdmfDbUserInfo& DbUserInfo = *it;
			//DbUserInfo.m_eRole = FtdGetUserRole(DbUserInfo.m_cstrLogin);
			DbUserInfo.m_eType = FtdGetLoginType(DbUserInfo.m_cstrLogin);
		}
	}
	catch ( CDBException* e )
	{
		FtdErr( "CFsTdmfDb::FtdGetUserInfo(),CDBException: <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	return TRUE;
}

void FsTdmfDb::FtdCreateTblEx ()
{
	if ( this == NULL )
	{
		throw FsExDb();
	}
	else try
	{
		FtdOpenEx();
		FtdDrop();
		FtdCreateTblsEx();
	}
	catch ( FsExDb e )
	{
		FtdErr( "FsTdmfDb::FtdCreateTblEx(): Creation not completed" );
		throw FsExDb();
	}

} // FsTdmfDb::FtdCreateTblEx ()


//BOOL FsTdmfDb::FtdCreateTbl ( CString pszDdl )
BOOL FsTdmfDb::FtdCreateTbl ( FsTdmfRec* ppRec )
{
	if ( this == NULL )
	{
		return FALSE;
	}
	else try
	{
		//FtdCreateTblsEx( pszDdl );
		FtdCreateTblsEx( ppRec );
	}
	catch ( FsExDb e )
	{
		FtdErr( "FsTdmfDb::FtdCreateTbl(): Creation not completed" );
		return FALSE;
	}

	return TRUE;

} // FsTdmfDb::FtdCreateTbl ()


BOOL FsTdmfDb::FtdCreateTbls ()
{
	if ( this == NULL )
	{
		return FALSE;
	}
	else try
	{
		FtdCreateTblEx();
	}
	catch ( FsExDb e )
	{
		FtdErr( "FsTdmfDb::FtdCreateTbls(): Creation not completed" );
		return FALSE;
	}

	return TRUE;

} // FsTdmfDb::FtdCreateTbls ()


void FsTdmfDb::FtdCreateTblsEx ()
{
	if ( this == NULL )
	{
		throw FsExDb();
	}
	else if ( !FtdIsOpen() )
	{
		FtdErr( 
			"FsTdmfDb::FtdCreateTblsEx(): Not opened, cannot create tables"
		);
		throw FsExDb();
	}

	try
	{
		FtdCreateTblsEx( mpRecDom    );
		FtdCreateTblsEx( mpRecCmd    );
		FtdCreateTblsEx( mpRecSrvInf );
		FtdCreateTblsEx( mpRecGrp    );
		FtdCreateTblsEx( mpRecPair   );
		FtdCreateTblsEx( mpRecAlert  );
		FtdCreateTblsEx( mpRecPerf   );

		FtdCreateTblsEx( mpRecHum );
		FtdCreateTblsEx( mpRecNvp );

		FtdCreateTblsEx( mpRecSysLog );
		FtdCreateTblsEx( mpRecKeyLog );
		FtdCreateTblsEx( mpRecScriptSrv );

	}
	catch ( FsExDb e )
	{
		FtdErr( "FsTdmfDb::FtdCreateTblsEx()" );
		throw FsExDb();
	}

} // FsTdmfDb::FtdCreateTblsEx ()


void FsTdmfDb::FtdCreateTblsEx ( FsTdmfRec* ppRec )
{
	if ( this == NULL || ppRec == NULL )
	{
		throw FsExDb();
	}
	else if ( !FtdIsOpen() )
	{
		FtdErr( 
			"FsTdmfDb::FtdCreateTblsEx(CString): "
			"Not opened, cannot create table <"+ppRec->FtdGetCreate()+">"
		);
		throw FsExDb();
	}
	else try
	{
		CString lszCreateTbl = ppRec->FtdGetCreate();
		ExecuteSQL ( lszCreateTbl );
		ppRec->FtdDestroy();
		ppRec->FtdPreFills();
	}
	catch ( CDBException* e )
	{
		FtdErr( 
			"CFsTdmfDb::FtdCreateTblsEx(CString),\n <%.1024s>\n<%.256s>", 
			ppRec->FtdGetCreate(), e->m_strError 
		);
		e->Delete ();
		throw FsExDb();
	}

} // FsTdmfDb::FtdCreateTblsEx ()


void FsTdmfDb::FtdDrop ()
{
	FtdDrop( mpRecPerf->mszTblName   );
	FtdDrop( mpRecAlert->mszTblName  );
	FtdDrop( mpRecPair->mszTblName   );
	FtdDrop( mpRecGrp->mszTblName    );
	FtdDrop( mpRecSrvInf->mszTblName );
	FtdDrop( mpRecDom->mszTblName    );
	FtdDrop( mpRecNvp->mszTblName    );
	FtdDrop( mpRecHum->mszTblName    );
	FtdDrop( mpRecCmd->mszTblName    );
	FtdDrop( mpRecSysLog->mszTblName );
	FtdDrop( mpRecKeyLog->mszTblName );
	FtdDrop( mpRecScriptSrv->mszTblName );

} // FsTdmfDb::FtdDrop ()


BOOL FsTdmfDb::FtdDrop ( CString pszTbl )
{
	if ( this == NULL )
	{
		return FALSE;
	}
	else if ( !FtdIsOpen() )
	{
		FtdErr( 
			"FsTdmfDb::FtdDrop(): Not opened, cannot drop table <%s>",
			pszTbl 
		);
		return FALSE;
	}

	CString lszDrop;
	//Example : "DROP TABLE TdmfDb.TdmfUserAdmin.t_setup"
	//                      ---------------------
	//                      mszUserName
	//                                           -------
	//                                           pszTbl

	lszDrop.Format ( "DROP TABLE "+pszTbl );
	
	try
	{
		ExecuteSQL ( lszDrop );
	}
	catch ( CDBException* e )
	{
		FtdErr( 
			"CFsTdmfDb::FtdDrop(): Cannot drop <"+pszTbl+"> <%.256s>",
			e->m_strError
		);
		e->Delete ();
		return FALSE;
	}

	return TRUE;

} // FsTdmfDb::FtdDrop ()


BOOL FsTdmfDb::FtdDropUsr ( CString pszUsrName , CString pszDb )
{
	if ( this == NULL )
	{
		return FALSE;
	}
	else if ( !FtdIsOpen() )
	{
		FtdErr( 
			"FsTdmfDb::FtdDrop(): "
			"Not opened, cannot drop user <"+pszUsrName+">"
		);
		return FALSE;
	}

	CString lszDdlRevoke;
	lszDdlRevoke.Format ( "EXEC sp_revokedbaccess "+pszUsrName );
	CString lszDdlDropUsr;
	lszDdlDropUsr.Format ( "USE " + pszDb + " EXEC sp_dropuser "+pszUsrName );
	CString lszDdlDropLogin;
	lszDdlDropLogin.Format ( "EXEC sp_droplogin "+pszUsrName );

	try
	{
		//ExecuteSQL ( lszDdlRevoke );
		ExecuteSQL ( lszDdlDropUsr );
	}
	catch ( CDBException* e )
	{
		FtdErr( "CFsTdmfDb::FtdDropUsr(): <%.256s>", e->m_strError );
		e->Delete ();
		//return FALSE;
	}

	try
	{
		ExecuteSQL ( lszDdlDropLogin );
	}
	catch ( CDBException* e )
	{
		FtdErr( "CFsTdmfDb::FtdDropUsr(): <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	return TRUE;

} // FsTdmfDb::FtdDropUsr ()


void FsTdmfDb::FtdErr ( CString pszFmt, ... )
{
	if ( this == NULL )
	{
		return;
	}

	va_list lVaLst;
	va_start ( lVaLst, pszFmt );

	CString lszMsg;
	lszMsg.FormatV ( pszFmt, lVaLst );

	mszErrMsg += "\nERROR: " + lszMsg;

	if ( mbFtdErrPop )
	{
		AfxMessageBox ( lszMsg );
	}

} // FsTdmfDb::FtdErr ()


void FsTdmfDb::FtdErrReset ()
{
	if ( this == NULL )
	{
		return;
	}

	mszErrMsg = "";

} // FsTdmfDb::FtdErrReste ()


void FsTdmfDb::FtdErrSql ( SQLHSTMT pHstmt, CString pszFmt, ... )
{
	if ( this == NULL )
	{
		return;
	}

	va_list lVaLst;
	va_start ( lVaLst, pszFmt );

	CString lszMsg;
	lszMsg.FormatV ( pszFmt, lVaLst );

	SQLCHAR       SqlState [ 6 ];
	SQLCHAR       Msg      [ SQL_MAX_MESSAGE_LENGTH ];
	SQLINTEGER    NativeError;
	SQLSMALLINT   i, MsgLen;
	SQLRETURN     rc2;

	i = 1;

	while ( ( rc2 = SQLGetDiagRec ( 
				SQL_HANDLE_STMT, pHstmt, i, SqlState, &NativeError,
				Msg, sizeof(Msg), &MsgLen)
			) != SQL_NO_DATA
		  ) 
	{
		lszMsg.Format ( lszMsg + "\n ODBCERR[%d]:SQLSTATE<%s>:", i, SqlState );
		lszMsg += (char*)Msg;
		i++;
	}

	if ( mbFtdErrPop )
	{
		AfxMessageBox ( lszMsg );
	}

	mszErrMsg += "\nERROR: " + lszMsg;

} // FsTdmfDb::FtdErrSql ()


CString FsTdmfDb::FtdGetErrMsg ()
{
	if ( this == NULL )
	{
		return "";
	}

	return mszErrMsg;

} // FsTdmfDb::FtdGetErrMsg ()


BOOL FsTdmfDb::FtdGetErrPop ()
{
	if ( this == NULL )
	{
		return FALSE;
	}

	return mbFtdErrPop;

} // FsTdmfDb::FtdGetErrPop ()


BOOL FsTdmfDb::FtdIsErrPop ()
{
	return FtdGetErrPop ();

} // FsTdmfDb::FtdIsErrPop ()


BOOL FsTdmfDb::FtdIsOpen ()
{
	if ( this == NULL )
	{
		return FALSE;
	}

	return IsOpen ();

} // FsTdmfDb::FtdIsOpen ()


/////////////////////////////////////////////////////////////////////////////
//
// FsTdmfDb::FtdOpen ()
//
//	Eventually, mechanise the Db parameters.
//
//		OpenEx( 
//			"ODBC;"
//			"DSN=;"
//			"DRIVER=SQL Server;"
//			"SERVER=ALRODRIGUE;"
//			//"UID=SA;"
//			//"UID=dbo;"
//			"UID=TdmfUserAdmin;"
//			"PWD=useradmin;"
//			//"APP=Microsoft Open Database Connectivity;"
//			//"WSID=ALRODRIGUE;"
//				// computer on which the application runs.
//			//"Trusted_Connection=Yes"
//		);

BOOL FsTdmfDb::FtdOpen ()
{
	if ( this == NULL )
	{
		return FALSE;
	}
	else if ( FtdIsOpen () )
	{
		return TRUE;
	}

	return FtdOpen ( mszUsr, mszPwd, mszSrv, mszDb, mnPort );

} // FsTdmfDb::FtdOpen ()


BOOL FsTdmfDb::FtdOpen ( CString pszUsr, CString pszPwd, CString pszSrv, CString pszDb, UINT nPort )
{
	if ( this == NULL )
	{
		return FALSE;
	}
	else try
	{
		FtdOpenEx( pszUsr, pszPwd, pszSrv, pszDb, nPort );
	}
	catch ( FsExDb e )
	{
		return FALSE;
	}

	return TRUE;

} // FsTdmfDb::DvDbOpen ()


void FsTdmfDb::FtdOpenEx ()
{
	if ( this == NULL )
	{
		throw FsExDb();
	}
	else if ( FtdIsOpen () )
	{
		return; // To reopen with dif Usr, please use arguments.
	}

	FtdOpenEx ( mszUsr, mszPwd, mszSrv, mszDb, mnPort );

} // FsTdmfDb::FtdOpenEx ()


void FsTdmfDb::FtdOpenEx ( CString pszUsr, CString pszPwd, CString pszSrv, CString pszDatabase, UINT nPort )
{
	if ( this == NULL )
	{
		throw FsExDb();
	}

	CString lszTrusted = "";
	if ( pszUsr == "Administrator" || pszUsr == "sa" )
	{
		lszTrusted = "Trusted_Connection=Yes;";
	}

	CString pszSrvFullDef = pszSrv;
	if (nPort > 0)
	{
		pszSrvFullDef.Format("%s,%d", pszSrv, nPort);
	}

	CString pszDatabaseString;
	if (pszDatabase.GetLength() > 0)
	{
		pszDatabaseString = ";DATABASE=" + pszDatabase;
	}

	CString lszConnect;
	lszConnect.Format ( 
		"ODBC;DSN=;DRIVER=SQL Server;"
		"SERVER="+pszSrvFullDef + pszDatabaseString + 
		";NETWORK=dbmssocn"
		";UID="+pszUsr+";PWD="+pszPwd+";"+lszTrusted
	);

	mszUsr = pszUsr;
	mszPwd = pszPwd;
	mszSrv = pszSrv;
	mszDb  = pszDatabase;
	mnPort = nPort;

	// Set login timeout
	SetLoginTimeout(15);
	// Set query timeout
	SetQueryTimeout(120);

	try
	{
		if ( pszUsr == "*" || pszPwd == "*" || pszSrv == "*" )
		{
			OpenEx( lszConnect, CDatabase::forceOdbcDialog );
		}
		else
		{
            //printf("\nlszConnect=%s\n",(LPCTSTR)lszConnect);
            OpenEx( lszConnect
                    , CDatabase::noOdbcDialog );
		}
	}
	catch ( CDBException* e )
	{
		FtdErr( "CFsTdmfDb::FtdOpenEx()\n <%.256s>", e->m_strError );
		e->Delete ();
		throw FsExDb();
	}

	if ( !FtdIsOpen () )
	{
		FtdErr( "CFsTdmfDb::FtdOpenEx(): Db not opened" );
		throw FsExDb();
	}

} // FsTdmfDb::FtdOpenEx ()


void FsTdmfDb::FtdSetErrPop ( BOOL pbVal )
{
	if ( this == NULL )
	{
		return;
	}

	mbFtdErrPop = pbVal;

	if ( mbFtdErrPop && mszErrMsg != "" )
	{
		AfxMessageBox ( mszErrMsg );
	}

} // FsTdmfDb::FtdSetErrPop ()


BOOL FsTdmfDb::FtdQueryNumberList( const CString & pcszSQLQuery, char cFieldType, std::vector<NumericType> & pvecFieldList)
{
	if ( this == NULL )
	{
		return FALSE;
	}
	else if ( !FtdIsOpen() )
	{
		FtdErr( "FsTdmfRec::FtdQueryIntegerList(): Db not opened" );
		return FALSE;
	}
	else try
	{
        SQLUINTEGER   result;
        SQLINTEGER    resultInd;
        SQLHSTMT      hstmt;
        SQLSMALLINT   type;  

        if ( SQL_SUCCESS != ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &hstmt) )
            return -1;

        switch( cFieldType )
        {
        case 0: type = SQL_C_STINYINT;  break;
        case 1: type = SQL_C_SSHORT;    break;
        case 2: 
        case 3: type = SQL_C_SLONG;     break;
        default:    ASSERT(0);          break;
        }
        if ( SQL_SUCCESS != ::SQLBindCol(hstmt, 1, type, &result, 0, &resultInd) )
            return -1;
        // Execute a statement to get the sales person/customer of all orders.
        if ( SQL_SUCCESS != ::SQLExecDirect(hstmt, (unsigned char*)(LPCTSTR)pcszSQLQuery, SQL_NTS) )
            return -1;
        // NRecords is available after a successful SQLFetch()
        while ( SQL_SUCCESS == ::SQLFetch(hstmt) ) 
        {
            NumericType num;
            switch( cFieldType )
            {
            case 0: num.cValue = (char)result;  break;
            case 1: num.sValue = (short)result; break;
            case 2: num.iValue = (int)result;   break;
            case 3: num.lValue = (long)result;  break;
            }
            pvecFieldList.push_back( num ); 
        }
        // Close the cursor.
        ::SQLCloseCursor(hstmt);

		::SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

        return TRUE;
	}
	catch ( CDBException* e )
	{
		FtdErr( "FsTdmfRec::FtdQueryIntegerList(),\n <%.256s>", e->m_strError );
		e->Delete ();
		return FALSE;
	}

	return TRUE;
} // FsTdmfDb::FtdQueryIntegerList

//get exclusive access on table
void FsTdmfDb::FtdLock()
{
    WaitForSingleObject(m_hMutex,INFINITE);
}//FsTdmfRec::FtdLock()

//release exclusive access on table
void FsTdmfDb::FtdUnlock()
{
    ReleaseMutex(m_hMutex);
}


