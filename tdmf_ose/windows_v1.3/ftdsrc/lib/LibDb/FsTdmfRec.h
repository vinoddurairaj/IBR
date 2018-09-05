// FsTdmfRec.h
// Fujitsu Software DataBase Record for Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_REC_H_
#define _FS_TDMF_REC_H_

#include "FsTdmfEtc.h"
#include "FsTdmfDb.h"

#include <afxtempl.h>		// List Ctrl

class FsTdmfDb;

class FsTdmfRec : public CRecordset
{
public:
	FsTdmfRec  ( FsTdmfDb* pDb, CString pszTblName );
	~FsTdmfRec ();

	virtual CString FtdGetCreate (); // Returns the create string
	virtual BOOL    FtdNew       ( CString pszName );
	virtual BOOL    FtdNew       ( CString pszName,       CString pszVal );
	virtual BOOL    FtdNew       ( int     piId );
	virtual BOOL    FtdOpen      ( CString pszSql );
	virtual BOOL    FtdPos       ( CString pszName );
	virtual BOOL    FtdPos       ( int     piId );
	virtual BOOL    FtdFirst     ( CString pszWhere = "", CString pszSort = "" );
	        BOOL    FtdMove      ( long plIndex );
	virtual BOOL    FtdNext      ();
	virtual BOOL    FtdRename    ( CString pszOldName,    CString pszNewName );
	virtual BOOL    FtdUpd       ( CString pszFldName,    CString pszVal );
	virtual BOOL    FtdUpd       ( CString pszFldName,    int     piVal );
	virtual BOOL    FtdUpd       ( CString pszFldName,    __int64 pjVal );

    virtual BOOL    FtdDelete    ( int iDeleteNFirstInRecordset , CString pcszWhere );
    virtual BOOL    FtdDelete    ( CString pcszWhere = "" );
    virtual BOOL    FtdDelete    ( int iDeleteNFirstInRecordset );

	int     FtdCount     (CString pszWhere = "");// Returns -1 on error, >= 0 on success.
	BOOL    FtdFills     ();
	BOOL    FtdPreFills  ();
	BOOL    FtdPos       ( int     piKa,   CString pszFldId );
	BOOL    FtdPos       ( CString pszVal, CString pszFldId );
	CString FtdSel       ( CString pszFld ); // Sel fld in the current record.
	CString FtdSel       ( int     piFld );
	//char* FtdSel       ( int     piFld );
	BOOL    FtdSetList   ( CString pszFld, CString pszVal );
	BOOL	FtdUpd       ( 
		CString pszFldName, CString pszVal, CString pszKaName );
	BOOL	FtdUpd       ( 
		CString pszFldName, int     piVal,  CString pszKaName );
	BOOL	FtdUpd       ( 
		CString pszFldName, __int64 pjVal,  CString pszKaName );
	BOOL	FtdSave      ( CString pszSet, CString pszWhere );
	BOOL	FtdAlter     ( CString pszSql );

	void    FtdDestroy   ();
	BOOL    FtdNextFast  ();
	BOOL    FtdOpenFast  ( CString pszSql );
	BOOL    FtdOpenTmp   ( CString pszSql );

	CString FtdGetTableSize ();
	BOOL	FtdDeleteTableRecords ();
    //get exclusive access on table
    void    FtdLock      ();
    //release exclusive access on table
    void    FtdUnlock    ();

	// Transaction
	BOOL    BeginTrans();
	BOOL    CommitTrans();
	BOOL    Rollback();

	FsTdmfDb*           mpDb;
	CList<FsNvp,FsNvp&> mList;

	CString mszTblName;

	UINT     muiOpenType;

	SQLHSTMT mHstmt;
	bool     mbFastMode;

private:

    HANDLE  m_hMutex;

}; // class FsTdmfRec

#endif