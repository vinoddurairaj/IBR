// FsTdmfDbView.cpp : implementation of the CFsTdmfDbView class
//

#include "stdafx.h"
#include "FsTdmfDbApp.h"

#include "FsTdmfDbDoc.h"
#include "FsTdmfDbView.h"

//#include "FsTdmfDb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbView

IMPLEMENT_DYNCREATE(CFsTdmfDbView, CFormView)

BEGIN_MESSAGE_MAP(CFsTdmfDbView, CFormView)
	//{{AFX_MSG_MAP(CFsTdmfDbView)
	ON_BN_CLICKED(IDC_BUTTON1, OnButCreateTbl)
	ON_BN_CLICKED(IDC_BUTTON2, OnButTc)
	ON_BN_CLICKED(IDC_BUTTON3, OnButDropTbl)
	ON_BN_CLICKED(IDC_BUTTON4, OnButStress)
	ON_BN_CLICKED(IDC_BUTTON5, OnButFill)
	ON_BN_CLICKED(IDC_BUTTON6, OnButCreateDb)
	ON_BN_CLICKED(IDC_BUTTON7, OnButCreateLogins)
	ON_BN_CLICKED(IDC_BUTTON8, OnButHelpDemo)
	ON_BN_CLICKED(IDC_BUTTON9, OnButRandom)
	ON_BN_CLICKED(IDC_BUTTON10, OnButCreateTbl2)
	ON_BN_CLICKED(IDC_BUTTON11, OnButWalkThr)
	ON_BN_CLICKED(IDC_BUTTON12, OnButWlkThr)
	ON_BN_CLICKED(IDC_BUTTON13, OnButExFillsTbl2)
	ON_BN_CLICKED(IDC_BUTTON14, OnButCrTblSql)
	ON_BN_CLICKED(IDC_BUTTON15, OnButFast)
	ON_BN_CLICKED(IDC_BUTTON16, OnButFast2)
	ON_BN_CLICKED(IDC_BUTTON17, OnButCrScenario)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CFormView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbView construction/destruction

CFsTdmfDbView::CFsTdmfDbView ()
	: CFormView (CFsTdmfDbView::IDD)
{
	//{{AFX_DATA_INIT(CFsTdmfDbView)
	m_Ctr1 = 0;
	m_Ctr2 = 0;
	m_iNuOfTest = 5;
	//}}AFX_DATA_INIT

	mpDb = NULL;
}

CFsTdmfDbView::~CFsTdmfDbView ()
{
	FvDbDel();

} // CFsTdmfDbView::~CFsTdmfDbView ()

void CFsTdmfDbView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFsTdmfDbView)
	DDX_Text(pDX, IDC_EDIT1, m_Ctr1);
	DDX_Text(pDX, IDC_EDIT2, m_Ctr2);
	DDX_Text(pDX, IDC_EDIT3, m_iNuOfTest);
	DDV_MinMaxLong(pDX, m_iNuOfTest, 1, 999999);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbView printing

/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbView diagnostics

#ifdef _DEBUG
void CFsTdmfDbView::AssertValid() const
{
	CFormView::AssertValid();
}

void CFsTdmfDbView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CFsTdmfDbDoc* CFsTdmfDbView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFsTdmfDbDoc)));
	return (CFsTdmfDbDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbView message handlers

// Create Table

void CFsTdmfDbView::OnButCreateTbl () 
{
	CWaitCursor lWaitCursor;

	FvDbDel(); // Just in case
	mpDb = new FsTdmfDb ( "*", "*", "AlRodrigue" );
	mpDb->FtdSetErrPop();

	try
	{
		mpDb->FtdCreateTblEx();
		FvDbDel();
	}
	catch ( ... )
	{
		AfxMessageBox 
		( 
			"Tbl Not Created CFsTdmfDbView::OnButCreateTbl () <"
			//+ mpDb->FtdGetErrMsg() + ">" 
		);
		FvDbDel();
		return;
	}

	AfxMessageBox ( "CFsTdmfDbView::OnButCreateTbl ():SUCCESS\nCreation of Tbl" );

} // CFsTdmfDbView::OnButCreateTbl ()

/////////////////////////////////////////////////////////////////////////////
//
// CFsTdmfDbView::OnButTc ()
//
//	Test for the Db access API.
//
//	The t_setup will keep the setup data along with t_setupDev at the
// record:SetupId=1.  The
// initial default values will be taken from the record:SetupId=2.  More
// default values will come from t_group and t_groupDev or will be
// uploaded from other databases into record:SetupId=3, 4, ...
//

void CFsTdmfDbView::OnButTc () 
{
	CWaitCursor lWaitCursor;

	if ( FvDbTc1() )
	{
		AfxMessageBox ( 
			"CFsTdmfDbView::OnButTc(): "
			"SUCCESS\nTestCases for Acessing Db completed. " 
		);
	}
	else
	{
		AfxMessageBox ( 
			"Oups CFsTdmfDbView::OnButTc(): "
			"TestCases for Acessing Db not completed " 
		);
	}

} // CFsTdmfDbView::OnButTc ()


// Drop Test

void CFsTdmfDbView::OnButDropTbl () 
{
	CWaitCursor lWaitCursor;

	FvDbDel(); // Just in case
	mpDb = new FsTdmfDb ( "TdmfUserAdmin", "useradmin" );

	try
	{
		mpDb->FtdDrop();
		FvDbDel();
	}
	catch ( ... )
	{
		AfxMessageBox ( "Tbl Not Dropped CFsTdmfDbView::OnButDropTbl ()" );
		return;
	}

	AfxMessageBox ( "CFsTdmfDbView::OnButDropTbl ():SUCCESS\nDrop of Db" );

} // CFsTdmfDbView::OnButDropTbl ()

// Stress Test

void CFsTdmfDbView::OnButStress () 
{
	CWaitCursor lWaitCursor;

	if ( FvDbTcStress() )
	{
		AfxMessageBox ( 
			"CFsTdmfDbView::OnButStress(): "
			"SUCCESS\nTestCases for Stress. " 
		);
	}
	else
	{
		AfxMessageBox ( 
			"Oups CFsTdmfDbView::OnButStress(): "
			"TestCases for Stress " 
		);
	}

} // CFsTdmfDbView::OnButStress ()


void CFsTdmfDbView::FvDbDel () 
{
	if ( mpDb != NULL )
	{
		mpDb->FtdClose();
		delete mpDb;
		mpDb = NULL;
	}
	
} // CFsTdmfDbView::FvDbDel ()


BOOL CFsTdmfDbView::FvDbTc1 () 
{
	FvDbDel(); // Just in case
	mpDb = new FsTdmfDb ( "TdmfUsrAdmin", "usradmin", "Tdmf_WinG" );
	//mpDb = new FsTdmfDb ( "*", "", "ALRODRIGUE" );
	//mpDb = new FsTdmfDb ( "TdmfUsrDaniel", "usrdaniel", "ALRODRIGUE" );
	//mpDb = new FsTdmfDb ( "TdmfUsrRod", "usrrod", "ALRODRIGUE" );
	//mpDb = new FsTdmfDb ( "TdmfAdmin", "tdmf", "TDMF_WIND" );
	mpDb->FtdSetErrPop();

	mpDb->FtdCreateTbls();
	mpDb->FtdErrReset();

	mpDb->mpRecDom->FtdNew( "Alpha" );
	mpDb->mpRecDom->FtdNew( "Beta" );
	mpDb->mpRecDom->FtdNew( "Ceta" );
	mpDb->mpRecDom->FtdRename( "Beta", "Betrave" );

	mpDb->mpRecDom->FtdPos( "Alpha" );
		mpDb->mpRecSrvInf->FtdNew( "MachA" );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldSrvId,      123 );
		mpDb->mpRecSrvInf->FtdPos( "MachA" );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     514 );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldHostId,     0xA7A7A7A7 );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldKey,    "Quelle Gallere" );

		CString lszKey = mpDb->mpRecSrvInf->FtdSel ( FsTdmfRecSrvInf::smszFldKey );

		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldJournalVol, "C:\\Ra" );
			mpDb->mpRecGrp->FtdNew( "000" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore, "C:\\Pa" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes, "Bla Bla" );
			mpDb->mpRecGrp->FtdNew( "001" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore, "C:\\Pb" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes, "Ble Ble" );
				mpDb->mpRecPair->FtdNew( "000" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldSrcDisk, "E:" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldTgtDisk, "F:" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldNotes, "Pla Pla" );
					mpDb->mpRecAlert->FtdNew( "Artung Zi" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  3 );
					mpDb->mpRecAlert->FtdNew( "Artung Zi Enk Korr" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  4 );
					mpDb->mpRecAlert->FtdNew( "Artung Zi A Waye" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  5 );

					//mpDb->mpRecPerf->FtdNew( 14 );
					//mpDb->mpRecPerf->FtdUpd( FsTdmfRecPerf::smszFldP2, 114 );
					//mpDb->mpRecPerf->FtdNew( 15 );
					//mpDb->mpRecPerf->FtdUpd( FsTdmfRecPerf::smszFldP2, 115 );
				mpDb->mpRecPair->FtdNew( "001" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldSrcDisk, "G:" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldTgtDisk, "H:" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldNotes, "Ple Ple" );
					mpDb->mpRecAlert->FtdNew( "Artung Zi" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  3 );
					mpDb->mpRecAlert->FtdNew( "Artung Zi Enk Korr" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  4 );
					mpDb->mpRecAlert->FtdNew( "Artung Zi A Waye" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  5 );
			mpDb->mpRecGrp->FtdNew( "002" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore, "C:\\Pc" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes, "Bli Bli" );

			mpDb->mpRecGrp->FtdPos( 001 );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes, "Blo Blo" );
		mpDb->mpRecSrvInf->FtdNew( "MachB" );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldSrvId,      456 );
		//mpDb->mpRecSrvInf->FtdPos( 456 );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     515 );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldJournalVol, "C:\\Rb" );
	mpDb->mpRecDom->FtdNew( "Delta" );
		mpDb->mpRecSrvInf->FtdNew( "MachC" );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldSrvId,      789 );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     516 );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldJournalVol, "C:\\Rc" );
		mpDb->mpRecSrvInf->FtdNew( "MachD" );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldName,       "MachD" );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     516 );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldJournalVol, "C:\\Rd" );
			mpDb->mpRecGrp->FtdNew( "001" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore, "C:\\Pd" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes, "Blu Blu" );

	mpDb->mpRecNvp->FtdNew( "Delta" );
	mpDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldVal,  "D" );
	mpDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "D en Grec" );
	mpDb->mpRecNvp->FtdNew( "Epsilon", "E" );
	mpDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "E en Grec" );
	mpDb->mpRecNvp->FtdNew( "Phi", "F" );
	mpDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "F en Grec" );
	mpDb->mpRecNvp->FtdUpdNvp( "Phi", "F et PH" );
	mpDb->mpRecNvp->FtdNew( "Gamma", "7" );
	mpDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "7 ieme lettre Grec" );
	mpDb->mpRecNvp->FtdUpdNvp( "Gamma", 71 );

	mpDb->mpRecHum->FtdNew( "Alain" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameMid, "A" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameLast, "Aubrey" );
	mpDb->mpRecHum->FtdNew( "Bruno" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameMid, "B" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameLast, "Belanger" );
	mpDb->mpRecHum->FtdNew( "Christian" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameMid, "C" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameLast, "Cartier" );
	mpDb->mpRecHum->FtdPos( "Bruno" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameLast, "DeLange" );



	CString lszErr = mpDb->FtdGetErrMsg();
	CString lszMsg = "CFsTdmfDbView::FvDbTc1(): ";

	if ( lszErr != "" )
	{
		lszMsg.Format ( lszMsg + "ERROR\n<" + lszErr + ">" );
		AfxMessageBox ( lszMsg );
		FvDbDel();
		return FALSE;
	}
	else
	{
		FvWt(); // WalkThrough
	}

	FvDbDel();
	return TRUE;

} // CFsTdmfDbView::FvDbTc1 ()


BOOL CFsTdmfDbView::FvDbTcStress ()
{
	CWaitCursor lWaitCursor;

	CString lszDesc = 
		"Adin, dva, try, tchetyry, piatt, chest, ciem, vociem, dyvyatt, "
		"dycyatt, adinatsat, dvynatsat, trynatsat, tchetyrynatsat, "
		"pytnatsat, chestnatsat, ciemnatsat, vocymnatsat, dyvytnatsat, "
		"dvatsat, sto, dvysto, pitsot";
	double lgTc = 0;

	try
	{
		FvDbDel(); // Just in case
		mpDb = new FsTdmfDb ( "TdmfUsrRod", "usrrod", "ALRODRIGUE" );
		mpDb->FtdSetErrPop();

		if ( !UpdateData ( TRUE ) )
		{
			return FALSE;
		}

		mpDb->mpRecDom->FtdFirst( "Id_Ka = 1005" );

		CString lszRes;

		CTime lTimeStart = CTime::GetCurrentTime();
		for ( int liTc = 0; liTc < m_iNuOfTest; liTc++ )
		{
			m_Ctr2 = liTc;

			mpDb->mpRecSrvInf->FtdFirst();

			for ( int liIns = 0; liIns < 1000; liIns++ )
			{
				mpDb->mpRecSrvInf->FtdNext();

				//lszRes = mpDb->mpRecSrvInf->FtdSel(0);
				//lszRes = mpDb->mpRecSrvInf->FtdSel(1);
				//lszRes = mpDb->mpRecSrvInf->FtdSel(2);

				lgTc += 1;
				m_Ctr1 = liIns;
				//UpdateWindow ();
			}

			mpDb->mpRecSrvInf->Close();

			for ( int liDel = 0; liDel < 000; liDel++ )
			{
				lgTc += 0;
				m_Ctr1 = liDel;
				//UpdateData ( FALSE );
			}

			UpdateData ( FALSE );
			UpdateWindow ();

		} // for ( int liTc = 0; liTc <=2000; liTc++ )

		CTimeSpan lTs   = CTime::GetCurrentTime() - lTimeStart;
		int       lPerf = lgTc / 
			( lTs.GetHours()*3600 + lTs.GetMinutes()*60 + lTs.GetSeconds() );

		CString lszTstReport;
		lszTstReport.Format ( 
			"Stress Test Report:\n"
			"\tNumber of Operation	:	<%20g>\n"
			"\tTime spent			:	<%02d:%02d:%02d>\n"
			"\tPerformance			:	<%03d> (Trans/sec)", 
			lgTc, lTs.GetHours (), lTs.GetMinutes (), lTs.GetSeconds (), lPerf
		);

		AfxMessageBox ( lszTstReport );
	}
	catch ( FsExDb e )
	{
		return FALSE;
	}

	FvDbDel();

	return TRUE;

} // CFsTdmfDbView::FvDbTcStress ()
		//CRecordset lRsDom ( (CDatabase*)mpDb );
		//mpDb->mpRecDom->muiOpenType = CRecordset::forwardOnly;
		//mpDb = new FsTdmfDb ( "TdmfUserAdmin", "useradmin", "ALRODRIGUE" );

		//mpDb->FtdCreateTbls();

		//for ( int liDom = 0; liDom < 1001; liDom++ )
		//{
		//	CString lszDom;
		//	lszDom.Format ( "TcStress%-d", liDom );
		//	mpDb->mpRecDom->FtdNew( lszDom );
		//}

		//for ( int liSrv = 0; liSrv < 1001; liSrv++ )
		//{
		//	CString lszSrv;
		//	lszSrv.Format ( "TcStress%-d", liSrv );
		//	mpDb->mpRecSrvInf->FtdNew( lszSrv );
		//}
				/*
				CString lszSql;
				lszSql.Format (
					"DELETE t_ServerInfo\n"
					"WHERE Domain_Fk = %d", liDel
				);

				//mpDb->ExecuteSQL ( lszSql );
				*/

/*
	    SQLINTEGER lSqlIntLen;
		SQLCHAR    laSqlChar [ 32 ];

		if ( SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, mpDb->m_hdbc, &hstmt) )
		{
			return FALSE;
		}
		//else if ( SQL_SUCCESS != SQLBindCol(hstmt, 3, SQL_C_CHAR, &laSqlChar, 30, &lSqlIntLen) )
		//{
		//	return FALSE;
		//}

		CTime lTimeStart = CTime::GetCurrentTime();
		for ( int liTc = 0; liTc < m_iNuOfTest; liTc++ )
		{
			m_Ctr2 = liTc;

			if ( SQL_SUCCESS != SQLExecDirect(hstmt, (unsigned char*)(LPCTSTR)lszSql, SQL_NTS) )
			{
				return FALSE;
			}

			for ( int liIns = 0; liIns < 1000; liIns++ )
			{
				if ( SQL_SUCCESS != SQLFetch(hstmt) ) 
				{
					break;
				}

				//lRet = SQLGetData(hstmt, 2, SQL_C_CHAR, &laSqlChar, 30, &lSqlIntLen);
				//lRet = SQLGetData(hstmt, 3, SQL_C_CHAR, &laSqlChar, 30, &lSqlIntLen);

  //lRsDom.Open ( CRecordset::dynaset, "SELECT * FROM t_Domain" );
			//lRsDom.Open ( CRecordset::forwardOnly, "SELECT * FROM t_Domain" );
			//lRsDom.Open ( CRecordset::snapshot, "SELECT * FROM t_Domain" );

			//mpDb->mpRecSrvInf->Open ( CRecordset::forwardOnly, "SELECT * FROM t_ServerInfo",CRecordset::useMultiRowFetch );
			//mpDb->mpRecSrvInf->Open ( CRecordset::dynaset, "SELECT * FROM t_ServerInfo" );
			//mpDb->mpRecSrvInf->Open ( CRecordset::snapshot, "SELECT * FROM t_ServerInfo" );

			//mpDb->mpRecDom->FtdFirst();
			//mpDb->mpRecSrvInf->FtdFirst();
				CString lszName;
				lszName.Format ( "ServerStress%-d", liIns+liTc*1000 );
				mpDb->ExecuteSQL ( 
					"INSERT INTO t_ServerInfo\n"
					"	    ( Domain_Fk, ServerName     )\n"
					"VALUES ( 1,         '" + lszName + "' )"
				);

				lRsDom.MoveNext ();

				//if ( !mpDb->mpRecDom->FtdNext() )
				//{
				//	TRACE0 ( "asdf" );
				//}


				mpDb->mpRecSrvInf->MoveNext ();
				CODBCFieldInfo lFi;
				CString        lszVal;
				int            liTot = mpDb->mpRecSrvInf->GetODBCFieldCount ();

				mpDb->mpRecSrvInf->GetODBCFieldInfo( int(0), lFi );
				mpDb->mpRecSrvInf->GetODBCFieldInfo( 2, lFi );
				mpDb->mpRecSrvInf->GetODBCFieldInfo( 1, lFi );
				mpDb->mpRecSrvInf->GetODBCFieldInfo( 9, lFi );
				mpDb->mpRecSrvInf->GetODBCFieldInfo( 8, lFi );
				mpDb->mpRecSrvInf->GetODBCFieldInfo( 4, lFi );

				mpDb->mpRecSrvInf->GetFieldValue ( int(0), lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 1, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 2, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 3, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 4, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 5, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 6, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 7, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 8, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 9, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 10, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 11, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 12, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 13, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 14, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 15, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 16, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 17, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 18, lszVal );
				mpDb->mpRecSrvInf->GetFieldValue ( 19, lszVal );

				for ( int liC = 0; liC < mpDb->mpRecSrvInf->GetODBCFieldCount (); liC++ )
				{
					//try
					//{
						mpDb->mpRecSrvInf->GetODBCFieldInfo( liC, lFi );

						mpDb->mpRecSrvInf->GetFieldValue ( liC, lszVal );
					//}
					//catch ( CDBException* e )
					//{
					//	mpDb->FtdErr( "FsTdmfRec::FtdFills(),\n <%.256s>", e->m_strError );
						//e->Delete ();
					//	return FALSE;
					//}
				} // for

				mpDb->mpRecSrvInf->FtdNext();
*/


// Exemple of Encapsulation for writting
// Prerequisite: Db already oppened
void CFsTdmfDbView::FvExEncap1 ( CString pszName, CString pszDesc, int piBabSize, int piWinSize )
{
	CString lszErr = mpDb->FtdGetErrMsg();
	CString lszMsg = "CFsTdmfDbView::FvExEncap1(): ";

	if ( lszErr == "" )
	{
		lszMsg.Format ( lszMsg + "SUCCESS" );
	}
	else
	{
		lszMsg.Format ( lszMsg + "ERROR\n<" + lszErr + ">" );
	}

	AfxMessageBox ( lszMsg );

} // CFsTdmfDbView::FvExEncap1 ()	


// Exemples
// Best Path aproach
// To see a non best path aproach, look at test cases
void CFsTdmfDbView::FvExFillsTbls1 ()
{
	FvDbDel(); // To start the example from scratch

	mpDb = new FsTdmfDb ( "TdmfUserTester", "usertester", "ALRODRIGUE" );

} // CFsTdmfDbView::FvExFillsTbls1 ()


// Examples
// Best Path aproach
// To see a non best path aproach, look at test cases
void CFsTdmfDbView::FvExFillsTbls2 ()
{
	FvDbDel(); // To start the example from scratch

	mpDb = new FsTdmfDb ( "*", "", "ALRODRIGUE" );

	CString lszErr = mpDb->FtdGetErrMsg();
	CString lszMsg = "CFsTdmfDbView::OnButCreateTbl2(): ";

	if ( lszErr == "" )
	{
		lszMsg.Format ( lszMsg + "SUCCESS" );
	}
	else
	{
		lszMsg.Format ( lszMsg + "ERROR\n<%s>", lszErr );
	}

	AfxMessageBox ( lszMsg );

} // CFsTdmfDbView::FvExFillsTbls2 ()


// Examples
// Best Path aproach
// To see a non best path aproach, look at test cases
void CFsTdmfDbView::FvExSelect1 ()
{
	FvDbDel(); // To start the example from scratch

	mpDb = new FsTdmfDb ( "TdmfUserTester", "usertester", "ALRODRIGUE" );

} // CFsTdmfDbView::FvExSelect1 ()


// Examples
// Best Path aproach
// To see a non best path aproach, look at test cases
void CFsTdmfDbView::FvExSelect2 ()
{
	FvDbDel(); // To start the example from scratch

	mpDb = new FsTdmfDb ( "TdmfUserTester", "usertester", "ALRODRIGUE" );

} // CFsTdmfDbView::FvExSelect2 ()


BOOL CFsTdmfDbView::FvFillsTbls ()
{
	if ( mpDb == NULL )
	{
		return FALSE;
	}
	else if ( !mpDb->FtdIsOpen() )
	{
		return FALSE;
	}

	return TRUE;

} // CFsTdmfDbView::FvFillsTbls ()


// WalkThrough
void CFsTdmfDbView::FvWt ()
{
	CString lszResult;

	// Db has to be openned

	if ( !mpDb->mpRecNvp->FtdFirst() )
	{
		//continue;
	}
	else do
	{
		CString lszKa   = mpDb->mpRecNvp->FtdSel( FsTdmfRecNvp::smszFldKa );
		CString lszName = mpDb->mpRecNvp->FtdSel( FsTdmfRecNvp::smszFldName );
		CString lszVal  = mpDb->mpRecNvp->FtdSel( FsTdmfRecNvp::smszFldVal );
		CString lszDesc = mpDb->mpRecNvp->FtdSel( FsTdmfRecNvp::smszFldDesc );

		CString lszLine;
		lszLine.Format ( mpDb->mpRecNvp->mszTblName + ":"
			"<" + lszKa + "><" + lszName + "><" + lszVal + "><" + lszDesc + ">\n"
		);
		lszResult += lszLine;
	}
	while ( mpDb->mpRecNvp->FtdNext() );

	if ( !mpDb->mpRecHum->FtdFirst() )
	{
		//continue;
	}
	else do
	{
		CString lszKa = mpDb->mpRecHum->FtdSel( FsTdmfRecHum::smszFldKa );
		CString lszNf = mpDb->mpRecHum->FtdSel( FsTdmfRecHum::smszFldNameFirst );
		CString lszNl = mpDb->mpRecHum->FtdSel( FsTdmfRecHum::smszFldNameLast );

		CString lszLine;
		lszLine.Format ( mpDb->mpRecHum->mszTblName + ":"
			"<" + lszKa + "><" + lszNf + "><" + lszNl + ">\n"
		);
		lszResult += lszLine;
	}
	while ( mpDb->mpRecHum->FtdNext() );

	if ( !mpDb->mpRecHum->FtdFirst( "", FsTdmfRecHum::smszFldNameLast) )
	{
		//continue;
	}
	else do
	{
		CString lszKa = mpDb->mpRecHum->FtdSel( FsTdmfRecHum::smszFldKa );
		CString lszNf = mpDb->mpRecHum->FtdSel( FsTdmfRecHum::smszFldNameFirst );
		CString lszNl = mpDb->mpRecHum->FtdSel( FsTdmfRecHum::smszFldNameLast );

		CString lszLine;
		lszLine.Format ( mpDb->mpRecHum->mszTblName + ":"
			"<" + lszKa + "><" + lszNf + "><" + lszNl + ">\n"
		);
		lszResult += lszLine;
	}
	while ( mpDb->mpRecHum->FtdNext() );

// Domain
	if ( !mpDb->mpRecDom->FtdFirst() )
	{
		//continue;
	}
	else do
	{
		CString lszKa   = mpDb->mpRecDom->FtdSel( FsTdmfRecDom::smszFldKa );
		CString lszName = mpDb->mpRecDom->FtdSel( FsTdmfRecDom::smszFldName );

		CString lszLine;
		lszLine.Format ( 
			mpDb->mpRecDom->mszTblName + ":<" + lszKa + "><" + lszName + ">\n"
		);
		lszResult += lszLine;

// Server Info
		if ( !mpDb->mpRecSrvInf->FtdFirst() )
		{
			//continue;
		}
		else do
		{
			CString lszSrvId   = mpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvId );
			CString lszSrvName = mpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldName );
			CString lszGroupId = mpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldIpPort );
			//CString lszJlVol   = mpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldJournalVol );

			lszLine.Format (
				"\t" + mpDb->mpRecSrvInf->mszTblName + ":<" 
				+ lszSrvId   + "><" + lszSrvName + "><" 
				+ lszGroupId + ">\n"
			);
			lszResult += lszLine;

			if ( !mpDb->mpRecGrp->FtdFirst() )
			{
				//continue;
			}
			else do
			{
				CString lszGrpId  = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldLgGrpId );
				CString lszPStore = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldPStore );
				CString lszNotes  = mpDb->mpRecGrp->FtdSel( FsTdmfRecLgGrp::smszFldNotes );

				lszLine.Format (
					"\t\t" + mpDb->mpRecGrp->mszTblName + ":<" 
					+ lszGrpId   + "><" + lszPStore + "><" + lszNotes + ">\n"
				);
				lszResult += lszLine;
			}
			while ( mpDb->mpRecGrp->FtdNext() );
		}
		while ( mpDb->mpRecSrvInf->FtdNext() );
	}
	while ( mpDb->mpRecDom->FtdNext() );

	AfxMessageBox ( lszResult );

	CString lszErr = mpDb->FtdGetErrMsg();
	CString lszMsg = "CFsTdmfDbView::FvDbTc1(): ";

	if ( lszErr != "" )
	{
		lszMsg.Format ( lszMsg + "ERROR\n<" + lszErr + ">" );
		AfxMessageBox ( lszMsg );
	}

} // CFsTdmfDbView::FvWt ()


// Volume Test
void CFsTdmfDbView::OnButFill () 
{
	CWaitCursor lWaitCursor;

	CString lszDesc = 
		"Mot, Hai, Ba, Bon, Nam, Sao, Bai, Tam, Tien, Muoi,"
		"am stram gram pik et pik et colegram "
		"bour et bour et ratatam am stram gram, "
		"une poule sur un mur qui picotait du pain dure, "
		"Na vkous na tsviett tavarishtie niet";

	FvDbDel(); // Just in case
	mpDb = new FsTdmfDb ( "TdmfUsrRod", "usrrod", "ALRODRIGUE" );
	//mpDb = new FsTdmfDb ( "TdmfUserTester", "usertester", "ALRODRIGUE" );

	
	for ( int liDom = 0; liDom < 1001; liDom++ )
	{
		CString lszDom;
		lszDom.Format ( "TcStress%-d", liDom );
		mpDb->mpRecDom->FtdNew( lszDom );
	}

	for ( int liSrv = 0; liSrv < 2001; liSrv++ )
	{
		CString lszSrv;
		lszSrv.Format ( "TcStress%-d", liSrv );
		mpDb->mpRecSrvInf->FtdNew( lszSrv );
	}
/*
	try
	{
		mpDb->FtdCreateTblEx();
	}
	catch ( FsExDb e )
	{
		AfxMessageBox ( "Oups CFsTdmfDbView::OnButFill (): Cant create tbl" );
	}
		
	if ( !UpdateData ( TRUE ) )
	{
		return;
	}

	long   llTc;
	CTime  lTimeStart = CTime::GetCurrentTime();

	{ // Just a block

		for ( llTc = 0; llTc < m_iNuOfTest; llTc++ )
		{
			CString lszName;
			lszName.Format ( "NameStpVlmTst%-ld", llTc );

			m_Ctr1 = llTc;
			UpdateData ( FALSE );
			UpdateWindow ();
		}
	} // End of block

	CTimeSpan lTs = CTime::GetCurrentTime() - lTimeStart;

	CString lszTstReport;
	lszTstReport.Format ( 
		"Volume Test Report:\n"
		"\tNumber of Operation :	<%ld>\n"
		"\tTime spent ():			<%02d:%02d:%02d>", 
		llTc, lTs.GetHours (), lTs.GetMinutes (), lTs.GetSeconds ()
	);
	AfxMessageBox ( lszTstReport );
*/
} // CFsTdmfDbView::OnButFill ()


// Create Database

void CFsTdmfDbView::OnButCreateDb () 
{
	CWaitCursor lWaitCursor;

	FvDbDel(); // Just in case
	mpDb = new FsTdmfDb ( "*", "", "AlRodrigue" );

	if ( mpDb->FtdCreateDb( "TdmfDb" ) )
	{
		AfxMessageBox ( "CFsTdmfDbView::OnButCreateDb ():SUCCESS\nCreation of Db" );
	}
	else
	{
		CString lszMsg;
		lszMsg.Format ( "Db Not Created CFsTdmfDbView::OnButCreateDb () <%s>", mpDb->FtdGetErrMsg () );
		AfxMessageBox ( lszMsg );
	}

	FvDbDel();

} // CFsTdmfDbView::OnButCreateDb ()


// Create Logins
void CFsTdmfDbView::OnButCreateLogins () 
{
	CWaitCursor lWaitCursor;

	FvDbDel(); // Just in case
	mpDb = new FsTdmfDb ( "*", "", "AlRodrigue" );
	//mpDb = new FsTdmfDb ( "TdmfUserAdmin", "useradmin", "ALRODRIGUE" );
	//mpDb = new FsTdmfDb ( "TdmfUserTester", "usertester", "ALRODRIGUE" );

	//if ( !mpDb->FtdCreateLogins( "TdmfUsrQelQonQ2", "rugndudju" ) )
	if ( !mpDb->FtdCreateLogins( "TdmfUserTester", "usertester", "TdmfDbTester" ) )
	{
		AfxMessageBox ( "Usr Not Created CFsTdmfDbView::OnButCreateLogins ()" );
		return;
	}

	AfxMessageBox ( "CFsTdmfDbView::OnButCreateLogins ():SUCCESS\nCreation of Usr" );

	try
	{
		mpDb->FtdClose();
		//mpDb->FtdOpenEx( "TdmfUsrQelQonQ2", "rugndudju", "ALRODRIGUE" );
		mpDb->FtdOpenEx( "TdmfUserTester", "usertester", "ALRODRIGUE" );
	}
	catch ( FsExDb e )
	{
		AfxMessageBox ( "Error CFsTdmfDbView::OnButCreateLogins (): cannot use new usr " );
	}

	FvDbDel();

} // CFsTdmfDbView::OnButCreateLogins ()


// Code demo
void CFsTdmfDbView::OnButHelpDemo () 
{
	CWaitCursor lWaitCursor;

} // CFsTdmfDbView::OnButHelpDemo ()


// Random Test
void CFsTdmfDbView::OnButRandom () 
{
	mpDb = new FsTdmfDb ( "TdmfUsrRod", "usrrod" );
	mpDb->FtdSetErrPop();

	mpDb->FtdDrop( mpDb->mpRecNvp->mszTblName );
	mpDb->FtdCreateTblsEx( mpDb->mpRecNvp );

/*
	CString lszMsg;

	CList<FsNvp,FsNvp> mLstTst;
	FsNvp lNvp;

	lNvp.mszN = "Adin";
	lNvp.mszV = "Am";

	mLstTst.AddTail ( lNvp );
	lNvp.mszN = "Dva";
	lNvp.mszV = "Stram";

	mLstTst.AddTail ( lNvp );
	lNvp.mszN = "Tri";
	lNvp.mszV = "Gram";

	mLstTst.AddTail ( lNvp );

	POSITION lPos = mLstTst.GetHeadPosition ();
	for (int i=0;i < mLstTst.GetCount ();i++)
	{
		lNvp = mLstTst.GetNext ( lPos );
		TRACE("%s, %s\r\n", lNvp.mszN, lNvp.mszV );
	}

	int     piVal1 = 127;
	int     piVal2 = 227;

	FvDbDel();
	mpDb = new FsTdmfDb ( "TdmfUsrRod", "usrrod" );
	while ( 1 ) // Once
	{
		if ( !mpDb->FtdCreateTbls() )
		{
			break;
		}

		mpDb->mpRecAgent->FtdNew( "Any" );
		mpDb->mpRecAgent->FtdUpd( "AgentName", "AgentName1" );
		mpDb->mpRecAgent->FtdUpd( "BabSize",   piVal1 );
		mpDb->mpRecAgent->FtdNew( "Any" );
		mpDb->mpRecAgent->FtdUpd( "AgentName", "AgentName2" );
		mpDb->mpRecAgent->FtdUpd( "BabSize",   piVal2 );

		mpDb->mpRecAgent->FtdPos( "AgentName1" );
		CString lszVal = mpDb->mpRecAgent->FtdSel( "AgentId" );
		lszVal = mpDb->mpRecAgent->FtdSel( "AgentName" );
		lszVal = mpDb->mpRecAgent->FtdSel( "AgentId" );

		if ( atoi ( mpDb->mpRecAgent->FtdSel( "BabSize" ) ) != piVal1 )
		{
			break;
		}

		lszMsg.Format (
			"SUCCESS:CFsTdmfDbView::OnButRandom():\n<%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;

	} // while ()
	lszMsg.Format (
		"CFsTdmfDbView::OnButRandom(): Tst not conclusive <%s>",
		mpDb->FtdGetErrMsg()
	);
	AfxMessageBox ( lszMsg );
	mpDb->FtdErrReset();
*/

} // CFsTdmfDbView::OnButRandom ()


// Create tables for any user
void CFsTdmfDbView::OnButCreateTbl2 () 
{
	CWaitCursor lWaitCursor;
	CString lszMsg;

	FvDbDel();
	mpDb = new FsTdmfDb ( "*", "" );

	try
	{
		mpDb->FtdCreateTblEx();
	}
	catch ( FsExDb e )
	{
		lszMsg.Format (
			"CFsTdmfDbView::OnButCreateTbl2(): Tbl Not Created <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );
		mpDb->FtdErrReset();
		return;
	}

	lszMsg.Format (
		"CFsTdmfDbView::OnButCreateTbl2(): SUCCESS\nCreation of Tbl <%s>",
		mpDb->FtdGetErrMsg()
	);
	AfxMessageBox ( lszMsg );

	FvDbDel();

} // CFsTdmfDbView::OnButCreateTbl2 ()


void CFsTdmfDbView::OnButWalkThr () 
{
	CWaitCursor lWaitCursor;
	CString lszResult = 
		"CFsTdmfDbView::OnButWalkThr() result\n"
		"--------------------------------------------------------------\n\n";

	FvDbDel();
	mpDb = new FsTdmfDb ( "*", "", "AlRodrigue" );
/*
	while ( 1 ) // Once
	{
		if ( !mpDb->FtdCreateTbls() )
		{
			break;
		}
		else if ( !FvFillsTbls() )
		{
			break;
		}
		else if ( !mpDb->mpRecAgent->FtdFirst() )
		{
			break;
		}
		else do
		{
			CString lszId   = mpDb->mpRecAgent->FtdSel( "AgentId" );
			CString lszName = mpDb->mpRecAgent->FtdSel( "AgentName" );
			CString lszSize = mpDb->mpRecAgent->FtdSel( "BabSize" );

			CString lszLine;
			lszLine.Format ( 
				"t_agent:<%ld><%.64s><%d>\n",
				atoi ( lszId ),	lszName, atoi ( lszSize )
			);
			lszResult += lszLine;

			if ( !mpDb->mpRecGroup->FtdFirst() )
			{
				break;
			}
			else do
			{
				CString lszGroupId = mpDb->mpRecGroup->FtdSel( "GroupId" );
				CString lszAgentId = mpDb->mpRecGroup->FtdSel( "AgentId" );
				CString lszGrpDesc = mpDb->mpRecGroup->FtdSel( "GroupDesc" );
				CString lszPstore  = mpDb->mpRecGroup->FtdSel( "PStore" );

				lszLine.Format (
					"\tt_group:<%s><%s><%.64s><%s>\n", 
					lszGroupId,	lszAgentId, lszGrpDesc, lszPstore
				);
				lszResult += lszLine;

				if ( !mpDb->mpRecPair->FtdFirst() )
				{
					break;
				}
				else do
				{
					CString lszPairId   = mpDb->mpRecPair->FtdSel( "PairId" );
					lszGroupId          = mpDb->mpRecPair->FtdSel( "GroupId" );
					CString lszDevA     = mpDb->mpRecPair->FtdSel( "DevA" );
					CString lszDevB     = mpDb->mpRecPair->FtdSel( "DevB" );
					CString lszPairDesc = mpDb->mpRecPair->FtdSel( "PairDesc" );

					lszLine.Format (
						"\t\tt_group:<%s><%s><%s><%s><%s>\n",
						lszPairId,	lszGroupId, lszDevA, lszDevB, lszPairDesc
					);
					lszResult += lszLine;
				}
				while ( mpDb->mpRecPair->FtdNext() );
			}
			while ( mpDb->mpRecGroup->FtdNext() );
		}
		while ( mpDb->mpRecAgent->FtdNext() );

		AfxMessageBox ( lszResult );

		break;

	} // just a block
*/
	CString lszMsg;
	lszMsg.Format (
		"CFsTdmfDbView::OnButWalkThr(): Error trace: <%s>",
		mpDb->FtdGetErrMsg()
	);
	AfxMessageBox ( lszMsg );

	mpDb->FtdErrReset();

} // CFsTdmfDbView::OnButWalkThr ()


void CFsTdmfDbView::OnButWlkThr () 
{
	CWaitCursor lWaitCursor;
	CString lszResult = 
		"CFsTdmfDbView::OnButWalkThr() result\n"
		"--------------------------------------------------------------\n\n";

	FvDbDel();
	mpDb = new FsTdmfDb ( "*", "", "ALRODRIGUE" );
/*
	while ( 1 ) // Once
	{
		if ( !mpDb->mpRecAgent->FtdFirst() )
		{
			break;
		}
		else do
		{
			CString lszId   = mpDb->mpRecAgent->FtdSel( "AgentId" );
			CString lszName = mpDb->mpRecAgent->FtdSel( "AgentName" );
			CString lszSize = mpDb->mpRecAgent->FtdSel( "BabSize" );

			CString lszLine;
			lszLine.Format ( 
				"t_agent:<%ld><%.64s><%d>\n", 
				atoi ( lszId ),	lszName, atoi ( lszSize )
			);
			lszResult += lszLine;

			if ( !mpDb->mpRecGroup->FtdFirst() )
			{
				break;
			}
			else do
			{
				CString lszGroupId = mpDb->mpRecGroup->FtdSel( "GroupId" );
				CString lszAgentId = mpDb->mpRecGroup->FtdSel( "AgentId" );
				CString lszGrpDesc = mpDb->mpRecGroup->FtdSel( "GroupDesc" );
				CString lszPstore  = mpDb->mpRecGroup->FtdSel( "PStore" );

				lszLine.Format (
					"\tt_group:<%s><%s><%.64s><%s>\n", 
					lszGroupId,	lszAgentId, lszGrpDesc, lszPstore
				);
				lszResult += lszLine;

				if ( !mpDb->mpRecPair->FtdFirst() )
				{
					break;
				}
				else do
				{
					CString lszPairId   = mpDb->mpRecPair->FtdSel( "PairId" );
					lszGroupId          = mpDb->mpRecPair->FtdSel( "GroupId" );
					CString lszDevA     = mpDb->mpRecPair->FtdSel( "DevA" );
					CString lszDevB     = mpDb->mpRecPair->FtdSel( "DevB" );
					CString lszPairDesc = mpDb->mpRecPair->FtdSel( "PairDesc" );

					lszLine.Format (
						"\t\tt_group:<%s><%s><%s><%s><%s>\n",
						lszPairId,	lszGroupId, lszDevA, lszDevB, lszPairDesc
					);
					lszResult += lszLine;
				}
				while ( mpDb->mpRecPair->FtdNext() );
			}
			while ( mpDb->mpRecGroup->FtdNext() );
		}
		while ( mpDb->mpRecAgent->FtdNext() );

		AfxMessageBox ( lszResult );

		break;

	} // just a block
*/
	CString lszMsg;
	lszMsg.Format (
		"CFsTdmfDbView::OnButWalkThr(): Error trace: <%s>",
		mpDb->FtdGetErrMsg()
	);
	AfxMessageBox ( lszMsg );

	mpDb->FtdErrReset();

} // CFsTdmfDbView::OnButWlkThr ()


void CFsTdmfDbView::OnButExFillsTbl2 () 
{
	FvExFillsTbls2();

} // CFsTdmfDbView::OnButExFillsTbl2 ()


void CFsTdmfDbView::OnButCrTblSql () 
{
	return;
/*
	CWaitCursor lWaitCursor;
	CString lszMsg;

	FvDbDel();
	mpDb = new FsTdmfDb ( "TdmfUsrRod", "usrrod", "ALRODRIGUE" );

	mpDb->FtdDrop( "t_Nvp" );
	mpDb->FtdDrop( "t_Peoples" );
	mpDb->FtdDrop( "t_Alert" );
	mpDb->FtdDrop( "t_Perf" );
	mpDb->FtdDrop( "t_RepPair" );
	mpDb->FtdDrop( "t_LogicalGroup" );
	mpDb->FtdDrop( "t_ServerInfo" );
	mpDb->FtdDrop( "t_Domain" );

	CString lszDdlTbl =
		"CREATE TABLE t_Domain \n"
		"(\n"
			"Domain_Ka     INT              IDENTITY NOT NULL,\n"
			"DomainName    VARCHAR(30)      NOT NULL,\n"

			"CONSTRAINT DomainPk PRIMARY KEY ( Domain_Ka ),\n"
			"CONSTRAINT DomainUk UNIQUE      ( DomainName )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_ServerInfo \n"
		"(\n"
			"Server_Ka     INT              IDENTITY NOT NULL,\n"
			"Domain_Fk     INT              NOT NULL,\n"
			"ServerName    VARCHAR( 30)     NOT NULL,\n"
			"ServerIp1     INT              NOT NULL,\n"
			"ServerIp2     INT              NOT NULL DEFAULT   0,\n"
			"ServerIp3     INT              NOT NULL DEFAULT   0,\n"
			"ServerIp4     INT              NOT NULL DEFAULT   0,\n"
			"ServerNotes   VARCHAR(300)     NOT NULL,\n"
			"IpPort        SMALLINT         NOT NULL DEFAULT 575,\n"
			"JournalVolume VARCHAR(300)     NOT NULL,\n"
			"BabSize       INT              NOT NULL DEFAULT  32,\n"
			"TcpWinSize    INT              NOT NULL DEFAULT 256,\n"
			"OsType        VARCHAR( 30),\n"
			"OsVersion     VARCHAR( 50),\n"
			"FileSystem    VARCHAR( 50),\n"
			"TdmfVersion   VARCHAR( 10),\n"

			"NumberOfCpu   SMALLINT         NOT NULL DEFAULT   1,\n"
			"RamSize       INT,\n"
			"RouterIp1     INT,\n"
			"RouterIp2     INT              DEFAULT   0,\n"
			"RouterIp3     INT              DEFAULT   0,\n"
			"RouterIp4     INT              DEFAULT   0,\n"

			"CONSTRAINT ServerPk PRIMARY KEY ( Server_Ka ),\n"
			"CONSTRAINT ServerFk FOREIGN KEY ( Domain_Fk ) "
				"REFERENCES t_Domain ( Domain_Ka ),\n"
			"CONSTRAINT ServerUk UNIQUE\n"
				"( ServerName, ServerIp1, ServerIp2, ServerIp3, ServerIp4 )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_LogicalGroup \n"
		"(\n"
			"LgGroupId         SMALLINT     NOT NULL,\n"
			"Source_Fk         INT          NOT NULL,\n"
			"Target_Fk         INT,\n"
			"PStoreVolume      VARCHAR(300) NOT NULL,\n"
			"Channing          BIT,\n"
			"Notes             VARCHAR(300) NOT NULL,\n"

			"ChunkDelay        INT,\n"
			"ChunkSize         INT,\n"
			"SyncMode          BIT,\n"
			"SyncModeDepth     INT,\n"
			"SyncModeTimeOut   INT,\n"
			"NetUsageThreshold BIT,\n"
			"NetUsageValue     INT,\n"
			"UpdateInterval    INT,\n" // Second
			"MaxFileStatSize   INT,\n" // Kb


			"CONSTRAINT LgPk     PRIMARY KEY ( Source_Fk, LgGroupId ),\n"

			"CONSTRAINT SrcFk    FOREIGN KEY ( Source_Fk ) "
				"REFERENCES t_ServerInfo ( Server_Ka ),"
			"CONSTRAINT TargetFk FOREIGN KEY ( Target_Fk ) "
				"REFERENCES t_ServerInfo ( Server_Ka ),"

			"CONSTRAINT LgUk UNIQUE ( Source_Fk, LgGroupId )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_RepPair \n"
		"(\n"
			"DtcId              SMALLINT     NOT NULL,\n"
			"LgGroupId_Fk       SMALLINT     NOT NULL,\n"
			"Source_Fk          INT          NOT NULL,\n"
			"Notes              VARCHAR(300) NOT NULL,\n"

			"SrcDisk            VARCHAR(128),\n"
			"SrcDisk1           INT,\n"
			"SrcDisk2           INT,\n"
			"SrcDisk3           INT,\n"

			"TargetDisk         VARCHAR(128),\n"
			"TargetDisk1        INT,\n"
			"TargetDisk2        INT,\n"
			"TargetDisk3        INT,\n"

			"CONSTRAINT RepPairPk "
				"PRIMARY KEY ( Source_Fk, LgGroupId_Fk, DtcId ),\n"

			"CONSTRAINT RepPairFk FOREIGN KEY ( Source_Fk, LgGroupId_Fk )\n"
				"REFERENCES t_LogicalGroup ( Source_Fk, LgGroupId    )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_Alert \n"
		"(\n"
			"DtcId_Fk     SMALLINT     NOT NULL,\n"
			"LgGroupId_Fk SMALLINT     NOT NULL,\n"
			"Source_Fk    INT          NOT NULL,\n"

			"Type         CHAR(16),\n"
			"Severity     TINYINT,\n"
			"TimeStamp    DATETIME,\n"
			"AlertText    VARCHAR(512),\n"

			"CONSTRAINT AlertFk\n"
				"FOREIGN KEY ( Source_Fk, LgGroupId_Fk, DtcId_Fk )\n"
				"REFERENCES t_RepPair ( Source_Fk, LgGroupId_Fk, DtcId )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_Perf \n"
		"(\n"
			"DtcId_Fk     SMALLINT     NOT NULL,\n"
			"LgGroupId_Fk SMALLINT     NOT NULL,\n"
			"Source_Fk    INT          NOT NULL,\n"
			"P1           INT,\n"
			"P2           INT,\n"
			"P3           INT,\n"
			"P4           INT,\n"

			"CONSTRAINT PerfFk\n"
				"FOREIGN KEY ( Source_Fk, LgGroupId_Fk, DtcId_Fk )\n"
				"REFERENCES t_RepPair ( Source_Fk, LgGroupId_Fk, DtcId )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_Peoples \n"
		"(\n"
			"Id_Ka         INT                 IDENTITY NOT NULL,\n"
			"Substitute_Fk INT,\n"
			"NameFirst     VARCHAR ( 30)           NOT NULL,\n"
			"NameMid       CHAR                    NOT NULL,\n"
			"NameLast      VARCHAR ( 30)           NOT NULL,\n"

			"JobTitle      VARCHAR ( 30),\n"
			"Department    VARCHAR ( 30),\n"
			"Telephone     VARCHAR ( 30),\n"
			"Cellular      VARCHAR ( 30),\n"
			"Pager         VARCHAR ( 30),\n"
			"email         VARCHAR ( 30),\n"

			"Notes         VARCHAR (256),\n"

			"CONSTRAINT PeoplePk PRIMARY KEY ( Id_Ka ),\n"
			"CONSTRAINT PeopleUk\n"
				"UNIQUE ( NameFirst, NameMid, NameLast ),\n"
			"CONSTRAINT PeopleFk FOREIGN KEY ( Substitute_Fk )\n"
				"REFERENCES t_Peoples ( Id_Ka )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_Nvp \n"
		"(\n"
			"Nvp_Ka INT              IDENTITY NOT NULL,\n"
			"Name   VARCHAR ( 30)           NOT NULL,\n"
			"Value  VARCHAR (256),\n"
			"Notes  VARCHAR (256),\n"

			"CONSTRAINT NvpPk     PRIMARY KEY ( Nvp_Ka ),\n"
			"CONSTRAINT NvpNameUk UNIQUE      ( Name )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}
	*/

} // CFsTdmfDbView::OnButCrTblSql ()


void CFsTdmfDbView::OnButCrTblSqlEff () 
{
	return;
/*
	CWaitCursor lWaitCursor;
	CString lszMsg;

	FvDbDel();
	mpDb = new FsTdmfDb ( "TdmfUsrRod", "usrrod", "ALRODRIGUE" );

	mpDb->FtdDrop( "t_Nvp" );
	mpDb->FtdDrop( "t_Alert" );
	mpDb->FtdDrop( "t_RepPair" );
	mpDb->FtdDrop( "t_LogicalGroup" );
	mpDb->FtdDrop( "t_ServerInfo" );
	mpDb->FtdDrop( "t_Domain" );

	CString lszDdlTbl =
		"CREATE TABLE t_Domain \n"
		"(\n"
			"Domain_Ka     INT              IDENTITY NOT NULL,\n"
			"DomainName    VARCHAR(30)      NOT NULL,\n"

			"CONSTRAINT DomainPk PRIMARY KEY ( Domain_Ka ),\n"
			"CONSTRAINT DomainUk UNIQUE      ( DomainName )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_ServerInfo \n"
		"(\n"
			"Server_Ka     INT              IDENTITY NOT NULL,\n"
			"Domain_Fk     INT              NOT NULL,\n"
			"ServerName    VARCHAR( 30)     NOT NULL,\n"
			"ServerNotes   VARCHAR(300)     NOT NULL,\n"
			"ServerIp1     INT              NOT NULL,\n"
			"ServerIp2     INT              NOT NULL DEFAULT   0,\n"
			"ServerIp3     INT              NOT NULL DEFAULT   0,\n"
			"ServerIp4     INT              NOT NULL DEFAULT   0,\n"
			"IpPort        SMALLINT         NOT NULL DEFAULT 575,\n"
			"JournalVolume VARCHAR(300)     NOT NULL,\n"
			"BabSize       INT              NOT NULL DEFAULT  32,\n"
			"TcpWinSize    INT              NOT NULL DEFAULT 256,\n"
			"OsType        VARCHAR( 30),\n"
			"OsVersion     VARCHAR( 50),\n"
			"FileSystem    VARCHAR( 50),\n"
			"TdmfVersion   VARCHAR( 10),\n"

			"NumberOfCpu   SMALLINT         NOT NULL DEFAULT   1,\n"
			"RamSize       INT,\n"
			"RouterIp1     INT,\n"
			"RouterIp2     INT              DEFAULT   0,\n"
			"RouterIp3     INT              DEFAULT   0,\n"
			"RouterIp4     INT              DEFAULT   0,\n"

			"CONSTRAINT ServerPk PRIMARY KEY ( Server_Ka ),\n"
			"CONSTRAINT ServerFk FOREIGN KEY ( Domain_Fk ) "
				"REFERENCES t_Domain ( Domain_Ka ),"
			"CONSTRAINT ServerUk UNIQUE\n"
				"( ServerName, ServerIp1, ServerIp2, ServerIp3, ServerIp4 )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_LogicalGroup \n"
		"(\n"
			"LogicalGroup_Ka   INT          IDENTITY NOT NULL,\n"
			"Source_Fk         INT          NOT NULL,\n"
			"LgGroupId         SMALLINT     NOT NULL,\n"
			"Notes             VARCHAR(300) NOT NULL,\n"
			"PStoreVolume      VARCHAR(300) NOT NULL,\n"
			"Channing          BIT,\n"

			"ChunkDelay        INT,\n"
			"ChunkSize         INT,\n"
			"SyncMode          BIT,\n"
			"SyncModeDepth     INT,\n"
			"SyncModeTimeOut   INT,\n"
			"NetUsageThreshold BIT,\n"
			"NetUsageValue     INT,\n"
			"UpdateInterval    INT,\n" // Second
			"MaxFileStatSize   INT,\n" // Kb

			"Target_Fk         INT,\n"

			"CONSTRAINT LgPk     PRIMARY KEY ( LogicalGroup_Ka ),\n"

			"CONSTRAINT SrcFk    FOREIGN KEY ( Source_Fk ) "
				"REFERENCES t_ServerInfo ( Server_Ka ),"
			"CONSTRAINT TargetFk FOREIGN KEY ( Target_Fk ) "
				"REFERENCES t_ServerInfo ( Server_Ka ),"

			"CONSTRAINT LgUk UNIQUE ( Source_Fk, LgGroupId )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_RepPair \n"
		"(\n"
			"ReplicationPair_Ka INT          IDENTITY NOT NULL,\n"
			"LogicalGroup_Fk    INT          NOT NULL,\n"
			"DtcId              SMALLINT     NOT NULL,\n"
			"Notes              VARCHAR(300) NOT NULL,\n"

			"SrcDisk            VARCHAR(128),\n"
			"SrcDisk1           INT,\n"
			"SrcDisk2           INT,\n"
			"SrcDisk3           INT,\n"

			"TargetDisk         VARCHAR(128),\n"
			"TargetDisk1        INT,\n"
			"TargetDisk2        INT,\n"
			"TargetDisk3        INT,\n"

			"CONSTRAINT RepPairPk PRIMARY KEY ( ReplicationPair_Ka ),\n"
			"CONSTRAINT RepPairFk FOREIGN KEY ( LogicalGroup_Fk )\n"
				"REFERENCES t_LogicalGroup ( LogicalGroup_Ka )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_Nvp \n"
		"(\n"
			"Nvp_Ka INT              IDENTITY NOT NULL,\n"
			"Name   VARCHAR ( 30)           NOT NULL,\n"
			"Value  VARCHAR (256),\n"
			"Notes  VARCHAR (256),\n"

			"CONSTRAINT NvpPk     PRIMARY KEY ( Nvp_Ka ),\n"
			"CONSTRAINT NvpNameUk UNIQUE      ( Name )\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}

	lszDdlTbl =
		"CREATE TABLE t_Alert \n"
		"(\n"
			"Alert_Ka   INT IDENTITY NOT NULL,\n"
			"LgGroup_Fk INT          NOT NULL,\n"
			"Notes      VARCHAR(512),\n"

			"CONSTRAINT AlertPk PRIMARY KEY ( Alert_Ka ),\n"
			"CONSTRAINT AlertFk FOREIGN KEY ( LgGroup_Fk )\n"
				"REFERENCES t_LogicalGroup ( LogicalGroup_Ka ),\n"
		")\n";

	if ( !mpDb->FtdCreateTbl( lszDdlTbl ) )
	{
		CString lszMsg;
		lszMsg.Format (
			"CFsTdmfDbView::OnButCrTblSql(): Error trace: <%s>",
			mpDb->FtdGetErrMsg()
		);
		AfxMessageBox ( lszMsg );

		return;
	}
*/

} // CFsTdmfDbView::OnButCrTblSqlEff ()

// With logical Group
//			"PStoreVolume  VARCHAR(300)     NOT NULL,\n"


void CFsTdmfDbView::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
	
	ResizeParentToFit ( FALSE );
	
}


void CFsTdmfDbView::OnButFast () 
{
	CWaitCursor lWaitCursor;

	double lgTc = 0;

	FvDbDel(); // Just in case
	mpDb = new FsTdmfDb ( "TdmfUsrRod", "usrrod", "ALRODRIGUE" );
	//mpDb = new FsTdmfDb ( "TdmfUserTester", "usertester", "ALRODRIGUE" );

	mpDb->FtdSetErrPop();

	if ( !UpdateData ( TRUE ) )
	{
		return;
	}

	mpDb->mpRecDom->FtdFirst( "Id_Ka = 1005" );

	CString lszRes;

	CTime lTimeStart = CTime::GetCurrentTime();
	for ( int liTc = 0; liTc < m_iNuOfTest; liTc++ )
	{
		m_Ctr2 = liTc;

		mpDb->mpRecSrvInf->FtdFirstFast();

		for ( int liIns = 0; liIns < 1000; liIns++ )
		{
			mpDb->mpRecSrvInf->FtdNextFast();

			//lszRes = mpDb->mpRecSrvInf->FtdSelFast(0);
			//lszRes = mpDb->mpRecSrvInf->FtdSelFast(1);
			//lszRes = mpDb->mpRecSrvInf->FtdSelFast(2);

			lgTc += 1;
			m_Ctr1 = liIns;
			UpdateData ( FALSE );
			//UpdateWindow ();
		}

		SQLCloseCursor ( mpDb->mpRecSrvInf->mHstmt );

		for ( int liDel = 0; liDel < 000; liDel++ )
		{
			lgTc += 0;
			m_Ctr1 = liDel;
			UpdateData ( FALSE );
		}

		UpdateWindow ();

	} // for ( int liTc = 0; liTc <=2000; liTc++ )

	CTimeSpan lTs   = CTime::GetCurrentTime() - lTimeStart;
	int       lPerf = lgTc / 
		( lTs.GetHours()*3600 + lTs.GetMinutes()*60 + lTs.GetSeconds() );

	CString lszTstReport;
	lszTstReport.Format ( 
		"Stress Test Report:\n"
		"\tNumber of Operation	:	<%20g>\n"
		"\tTime spent			:	<%02d:%02d:%02d>\n"
		"\tPerformance			:	<%03d> (Trans/sec)", 
		lgTc, lTs.GetHours (), lTs.GetMinutes (), lTs.GetSeconds (), lPerf
	);

	AfxMessageBox ( lszTstReport );

	FvDbDel();
	
} // CFsTdmfDbView::OnButFast ()


void CFsTdmfDbView::OnButFast2 () 
{
	AfxMessageBox ( "Not Implemented" );
	return;

	CWaitCursor lWaitCursor;

	double lgTc = 0;

	FvDbDel(); // Just in case
	mpDb = new FsTdmfDb ( "TdmfUsrRod", "usrrod", "ALRODRIGUE" );
	mpDb->FtdSetErrPop();

	if ( !UpdateData ( TRUE ) )
	{
		return;
	}

	//mpDb->mpRecDom->FtdFirst();

	CString lszRes;
	CString lszSql = "SELECT * FROM t_ServerInfo";

	CTime lTimeStart = CTime::GetCurrentTime();
	for ( int liTc = 0; liTc < m_iNuOfTest; liTc++ )
	{
		m_Ctr2 = liTc;

		if ( SQL_SUCCESS != SQLExecDirect 
							(
								mpDb->mpRecSrvInf->mHstmt, 
								(unsigned char*)(LPCTSTR)lszSql,
								SQL_NTS
							)
		   )
		{
			mpDb->FtdErrSql
			( 
				mpDb->mpRecSrvInf->mHstmt, 
				"FsTdmfRec::FtdOpenFast(): cant SQLExecDirect() with\n<" + lszSql + ">"
			);

			break;
		}
		else for ( int liIns = 1; liIns < 1000; liIns++ )
		{
			if ( SQL_SUCCESS != SQLFetchScroll 
								( 
									mpDb->mpRecSrvInf->mHstmt,
									SQL_FETCH_ABSOLUTE,
									liIns
								)
			   ) 
			{
				CString lszMsg;
				lszMsg.Format ( "FsTdmfRec::OnButFast2:SQLFetch() <%d>", liIns );
				mpDb->FtdErrSql( mpDb->mpRecSrvInf->mHstmt, lszMsg );
				//break;
			}

			lgTc += 1;
			m_Ctr1 = liIns;
			UpdateData ( FALSE );
		}

		SQLCloseCursor ( mpDb->mpRecSrvInf->mHstmt );

		for ( int liDel = 0; liDel < 000; liDel++ )
		{
			lgTc += 0;
			m_Ctr1 = liDel;
			UpdateData ( FALSE );
		}

		UpdateWindow ();

	} // for ( int liTc = 0; liTc <=2000; liTc++ )

	CTimeSpan lTs   = CTime::GetCurrentTime() - lTimeStart;
	int       lPerf = lgTc / 
		( lTs.GetHours()*3600 + lTs.GetMinutes()*60 + lTs.GetSeconds() );

	CString lszTstReport;
	lszTstReport.Format ( 
		"Stress Test Report:\n"
		"\tNumber of Operation	:	<%20g>\n"
		"\tTime spent			:	<%02d:%02d:%02d>\n"
		"\tPerformance			:	<%03d> (Trans/sec)", 
		lgTc, lTs.GetHours (), lTs.GetMinutes (), lTs.GetSeconds (), lPerf
	);

	AfxMessageBox ( lszTstReport );

	FvDbDel();

} // CFsTdmfDbView::OnButFast2 ()


void CFsTdmfDbView::OnButCrScenario () 
{
	FvDbDel(); // Just in case
	mpDb = new FsTdmfDb ( "TdmfUsrAdmin", "usradmin", "Tdmf_WinG" );
	mpDb->FtdSetErrPop();

	mpDb->FtdCreateTbls();
	mpDb->FtdErrReset();

	mpDb->mpRecDom->FtdNew( "Alpha" );
	mpDb->mpRecDom->FtdNew( "Beta" );
	mpDb->mpRecDom->FtdNew( "Ceta" );

		mpDb->mpRecSrvInf->FtdNew( "MachA" );
		CString lszTgt = mpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvId );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort, 514 );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldHostId, 0x00112200 );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldKey,    "Quelle Gallere" );

		CString lszKey = mpDb->mpRecSrvInf->FtdSel ( FsTdmfRecSrvInf::smszFldKey );

			mpDb->mpRecGrp->FtdNew( "000" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtFk,  lszTgt );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore, "C:\\Pa" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes,  "Bla Bla" );
			mpDb->mpRecGrp->FtdNew( "001" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtFk,  lszTgt );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore, "C:\\Pb" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes, "Ble Ble" );
				mpDb->mpRecPair->FtdNew( "000" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldSrcDisk, "E:" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldTgtDisk, "F:" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldNotes, "Pla Pla" );
					mpDb->mpRecAlert->FtdNew( "Artung Zi" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  3 );
					mpDb->mpRecAlert->FtdNew( "Artung Zi Enk Korr" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  4 );
					mpDb->mpRecAlert->FtdNew( "Artung Zi A Waye" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  5 );

					//mpDb->mpRecPerf->FtdNew( 14 );
					//mpDb->mpRecPerf->FtdUpd( FsTdmfRecPerf::smszFldP2, 114 );
					//mpDb->mpRecPerf->FtdNew( 15 );
					//mpDb->mpRecPerf->FtdUpd( FsTdmfRecPerf::smszFldP2, 115 );

				mpDb->mpRecPair->FtdNew( "001" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldSrcDisk, "G:" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldTgtDisk, "H:" );
				mpDb->mpRecPair->FtdUpd( FsTdmfRecPair::smszFldNotes, "Ple Ple" );
					mpDb->mpRecAlert->FtdNew( "Artung Zi" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  3 );
					mpDb->mpRecAlert->FtdNew( "Artung Zi Enk Korr" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  4 );
					mpDb->mpRecAlert->FtdNew( "Artung Zi A Waye" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldType, "External" );
					mpDb->mpRecAlert->FtdUpd( FsTdmfRecAlert::smszFldSev,  5 );
			mpDb->mpRecGrp->FtdNew( "002" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtFk,  lszTgt );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore, "C:\\Pc" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes, "Bli Bli" );

			mpDb->mpRecGrp->FtdPos( 001 );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes, "Blo Blo" );
		mpDb->mpRecSrvInf->FtdNew( "MachB" );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldHostId, 0x00112201 );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldSrvId,      456 );
		//mpDb->mpRecSrvInf->FtdPos( 456 );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     515 );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldJournalVol, "C:\\Rb" );
	mpDb->mpRecDom->FtdNew( "Delta" );
		mpDb->mpRecSrvInf->FtdNew( "MachC" );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldHostId, 0x00112202 );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldSrvId,      789 );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     516 );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldJournalVol, "C:\\Rc" );
		mpDb->mpRecSrvInf->FtdNew( "MachD" );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldHostId, 0x00112203 );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldName,       "MachD" );
		mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     516 );
		//mpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldJournalVol, "C:\\Rd" );
			mpDb->mpRecGrp->FtdNew( "004" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldTgtFk,  lszTgt );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore, "C:\\Pd" );
			mpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldNotes, "Blu Blu" );

	mpDb->mpRecNvp->FtdNew( "Delta" );
	mpDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldVal,  "D" );
	mpDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "D en Grec" );
	mpDb->mpRecNvp->FtdNew( "Epsilon", "E" );
	mpDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "E en Grec" );
	mpDb->mpRecNvp->FtdNew( "Phi", "F" );
	mpDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "F en Grec" );
	mpDb->mpRecNvp->FtdUpdNvp( "Phi", "F et PH" );
	mpDb->mpRecNvp->FtdNew( "Gamma", "7" );
	mpDb->mpRecNvp->FtdUpd( FsTdmfRecNvp::smszFldDesc, "7 ieme lettre Grec" );
	mpDb->mpRecNvp->FtdUpdNvp( "Gamma", 71 );

	mpDb->mpRecHum->FtdNew( "Alain" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameMid, "A" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameLast, "Aubrey" );
	mpDb->mpRecHum->FtdNew( "Bruno" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameMid, "B" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameLast, "Belanger" );
	mpDb->mpRecHum->FtdNew( "Christian" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameMid, "C" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameLast, "Cartier" );
	mpDb->mpRecHum->FtdPos( "Bruno" );
	mpDb->mpRecHum->FtdUpd( FsTdmfRecHum::smszFldNameLast, "DeLange" );

	CString lszErr = mpDb->FtdGetErrMsg();
	CString lszMsg = "CFsTdmfDbView::FvDbTc1(): ";

	if ( lszErr != "" )
	{
		lszMsg.Format ( lszMsg + "ERROR\n<" + lszErr + ">" );
		AfxMessageBox ( lszMsg );
	}
	else
	{
		AfxMessageBox ( "Scenario created" );
	}

	FvDbDel();

} // CFsTdmfDbView::OnButCrScenario ()



