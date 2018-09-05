// FsTdmfDb.h
// Fujitsu Software DataBase Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//

#ifndef _FSTDMFDN_H_
#	define _FSTDMFDN_H_

#pragma warning(disable: 4786)
#pragma warning(disable: 4503)

#include <afxdb.h>			// ODBC
#include <vector>			// STL
#include <map>   			// STL
#include <list>   			// STL

#include "FsTdmfEtc.h"
#include "FsTdmfRec.h"
#include "FsTdmfRecNvp.h"
#include "FsTdmfRecHum.h"
#include "FsTdmfRecDom.h"
#include "FsTdmfRecSrvInf.h"
#include "FsTdmfRecLgGrp.h"
#include "FsTdmfRecPair.h"
#include "FsTdmfRecAlert.h"
#include "FsTdmfRecPerf.h"
#include "FsTdmfRecCmd.h"
#include "FsTdmfRecSysLog.h"
#include "FsTdmfRecKeyLog.h"
#include "FsTdmfRecSrvScript.h"

class FsTdmfRec;
//class FsTdmfRecDom; // Domain
//class FsTdmfRecGroup;
//class FsTdmfRecPair;
//class FsTdmfRecNvp;
class FsTdmfRecAlert;


#define DB_ROLE_OWNER             0x0001
#define DB_ROLE_SECURITYADMIN     0x0002
#define DB_ROLE_ACCESSADMIN       0x0004
#define DB_ROLE_DATAREADER        0x0008
#define DB_SRV_ROLE_SECURITYADMIN 0x0100

class FsTdmfDbUserInfo
{
public:
	typedef enum TdmfRoles
	{
		TdmfRoleUndef,
		TdmfRoleAdministrator,
		TdmfRoleSupervisor,
		TdmfRoleUser
	} TdmfRoles;

	typedef enum TdmfLoginTypes
	{
		TdmfLoginUndef,
		TdmfLoginStandard,
		TdmfLoginWindowsUser,
		TdmfLoginWindowsGroup,
	} TdmfLoginTypes;


	CString        m_cstrLogin;
	CString        m_cstrName;
	CString        m_cstrLocation;
	TdmfLoginTypes m_eType;
	//TdmfRoles      m_eRole;
	CString        m_cstrApp;
};


class FsTdmfDb : public CDatabase
{
public:
	FsTdmfDb  ( CString pszUsr, CString pszPwd = "", CString pszSrv = "", CString pszDB = "", UINT nPort = 0 );
	~FsTdmfDb ();

	void    FtdClose        ();
	BOOL    FtdCreateDb     ( CString pszDb , CString pszDbBasePath );
	BOOL    FtdCreateLogins ( CString pszUsrName, CString pszPwd, CString pszDb, bool bDBOwner = false );
	BOOL    FtdCreateLoginsWithAccess( CString pszUsrName, CString pszPwd, CString pszDb, UINT nAccesFlag );

	FsTdmfDbUserInfo::TdmfRoles      FtdGetUserRole(CString cstrUserName);
	FsTdmfDbUserInfo::TdmfLoginTypes FtdGetLoginType(LPCSTR lpstrLogin);
	BOOL      FtdGetUsersInfo(std::list<FsTdmfDbUserInfo>& vecUser);

	//BOOL    FtdCreateTbl    ( CString pszDdl );
	BOOL    FtdCreateTbl    ( FsTdmfRec* ppRec );
	BOOL    FtdCreateTbls   ();
	void    FtdCreateTblEx  ();	// With exeption mechanism
	void    FtdCreateTblsEx ();
	//void    FtdCreateTblsEx ( CString pszCreateTbl );
	void    FtdCreateTblsEx ( FsTdmfRec* ppRec );
	void    FtdDrop         ();
	BOOL	FtdDrop         ( CString pszTbl );
	BOOL    FtdDropUsr      ( CString pszusrName, CString pszDb );
	void	FtdErrReset     ();
	BOOL    FtdIsOpen       ();
	BOOL    FtdOpen         ();
	BOOL    FtdOpen         ( CString pszUsr, CString pszPwd, CString pszSrv = "", CString pszDb = "", UINT nPort = 0 );
	void    FtdOpenEx       ();	// With exeption mechanism
	void    FtdOpenEx       ( CString pszUsr, CString pszPwd, CString pszSrv = "", CString pszDb = "", UINT nPort = 0 );
    //issue a SQL query that generates 0 to N number of numbers as result.
    //the std::vector will be populated with the N numbers 
    //cFieldType must specify the type of the Column retreived by the query: 0 = char, 1 = short , 2 = int , 3 = long .
    union NumericType        // Declare a union that can hold the following:
    {
        char        cValue; //cFieldType = 0
        short       sValue; //cFieldType = 1
        int         iValue; //cFieldType = 2
        long        lValue; //cFieldType = 3
    };
    BOOL    FtdQueryNumberList( const CString & pcszSQLQuery, char cFieldType, std::vector<NumericType> & pvecFieldList );


	void    FtdErr          ( CString  mszFmt, ... );
	void    FtdErrSql       ( SQLHSTMT pHstmt, CString  mszFmt, ...  );
	CString FtdGetErrMsg    ();
	BOOL    FtdGetErrPop    ();
	BOOL    FtdIsErrPop     ();
	void    FtdSetErrPop    ( BOOL pbVal = TRUE );

    //get exclusive access on DB
    void    FtdLock         ();
    //release exclusive access on DB
    void    FtdUnlock       ();

	FsTdmfRecDom*    mpRecDom;
	FsTdmfRecSrvInf* mpRecSrvInf;
	FsTdmfRecLgGrp*  mpRecGrp;
	FsTdmfRecPair*   mpRecPair;
	FsTdmfRecAlert*  mpRecAlert;
	FsTdmfRecPerf*   mpRecPerf;

	FsTdmfRecHum*    mpRecHum;
	FsTdmfRecCmd*    mpRecCmd;
	FsTdmfRecNvp*    mpRecNvp; // Admin and Etc Table

	FsTdmfRecSysLog* mpRecSysLog;
	FsTdmfRecKeyLog* mpRecKeyLog;
	FsTdmfRecSrvScript* mpRecScriptSrv;

	static CString mszDbVersion;

private:

	CString mszUsr;
	CString mszPwd;
	CString mszSrv;
	CString mszDb;
	UINT    mnPort;

	BOOL    mbFtdErrPop;
	CString mszErrMsg;

    HANDLE  m_hMutex;

}; // class FsTdmfDb

#endif








