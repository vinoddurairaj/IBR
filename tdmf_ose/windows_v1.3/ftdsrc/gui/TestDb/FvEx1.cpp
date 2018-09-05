// FvEx1.cpp : implementation file
//

#include "stdafx.h"
#include "FsTdmfDbApp.h"

#include "FvEx1.h"

#include "..\..\lib\LibDb\FsTdmfDb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// FvEx1

IMPLEMENT_DYNCREATE(FvEx1, CFormView)

FvEx1::FvEx1()
	: CFormView(FvEx1::IDD)
{
	//{{AFX_DATA_INIT(FvEx1)
	m_szSql = _T("");
	m_szAdd = _T("");
	m_szSrvName = _T("");
	//}}AFX_DATA_INIT
}

FvEx1::~FvEx1()
{
}

void FvEx1::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(FvEx1)
	DDX_Text(pDX, IDCE_Sql, m_szSql);
	DDX_Text(pDX, IDCE_SrvIpP, m_szAdd);
	DDX_Text(pDX, IDCE_SrvName, m_szSrvName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(FvEx1, CFormView)
	//{{AFX_MSG_MAP(FvEx1)
	ON_BN_CLICKED(IDCB_Fills, OnFills)
	ON_BN_CLICKED(IDCB_Show, OnShowTbl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// FvEx1 diagnostics

#ifdef _DEBUG
void FvEx1::AssertValid() const
{
	CFormView::AssertValid();
}

void FvEx1::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// FvEx1 message handlers

void FvEx1::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	//ShowWindow ( SW_SHOWMAXIMIZED );
	
	CFormView::OnActivateView(bActivate, pActivateView, pDeactiveView);

	//ShowWindow ( SW_SHOWMAXIMIZED );
}

void FvEx1::OnDraw(CDC* pDC) 
{
	// TODO: Add your specialized code here and/or call the base class

}

void FvEx1::OnInitialUpdate () 
{
	CFormView::OnInitialUpdate();
	ResizeParentToFit ( FALSE );
	//ShowWindow ( SW_SHOWMAXIMIZED );

	CListCtrl* lpLc = (CListCtrl*) GetDlgItem ( IDCLC_Tbl );

	lpLc->InsertColumn( 0, "Name",           LVCFMT_LEFT,  80 );
	lpLc->InsertColumn( 1, "IP Adress",      LVCFMT_LEFT, 100 );
	lpLc->InsertColumn( 2, "OS",             LVCFMT_LEFT,  40 );
	lpLc->InsertColumn( 3, "OS Level",       LVCFMT_LEFT,  60 );
	lpLc->InsertColumn( 4, "Defined/Active", LVCFMT_LEFT, 100 );
	lpLc->InsertColumn( 5, "Port",           LVCFMT_LEFT,  40 );
	lpLc->InsertColumn( 6, "Volumes",        LVCFMT_LEFT,  60 );
	lpLc->InsertColumn( 6, "Status",         LVCFMT_LEFT,  60 );

} // FvEx1::OnInitialUpdate ( ()

void FvEx1::OnFills () 
{
	FsTdmfDb* lpDb = new FsTdmfDb ( "TdmfUsrDaniel", "usrdaniel", "ALRODRIGUE" );

	lpDb->FtdCreateTbls();

	lpDb->mpRecDom->FtdNew( "Domain Alpha" );

	lpDb->mpRecSrvInf->FtdNew( "Server01" );
	lpDb->mpRecSrvInf->FtdUpd( 
		FsTdmfRecSrvInf::smszFldSrvIp1,    
		IpSzToInt ( "199.121.121.001" )
	);
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsType,    "NT" );
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsVersion, "4.0 SP6" );
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     562 );
	//lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldNumOfVol, 12 );
	//lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldStatus, "Normal" );
		lpDb->mpRecGrp->FtdNew( "001" );
		lpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,    "C:\\Pstore" );
		lpDb->mpRecGrp->FtdNew( "002" );
		lpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,    "C:\\Pstore" );
		lpDb->mpRecGrp->FtdNew( "003" );
		lpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,    "C:\\Pstore" );
		lpDb->mpRecGrp->FtdNew( "004" );
		lpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,    "C:\\Pstore" );

	lpDb->mpRecSrvInf->FtdNew( "Server02" );
	lpDb->mpRecSrvInf->FtdUpd( 
		FsTdmfRecSrvInf::smszFldSrvIp1,    
		IpSzToInt ( "123.132.234.254" )
	);
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsType,    "W2K" );
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsVersion, "SP1" );
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     562 );
	//lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldNumOfVol, 3 );
	//lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldStatus, "CheckPoint" );
		lpDb->mpRecGrp->FtdNew( "001" );
		lpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,    "C:\\Pstore" );
		lpDb->mpRecGrp->FtdNew( "002" );
		lpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,    "C:\\Pstore" );

	lpDb->mpRecSrvInf->FtdNew( "Server03" );
	lpDb->mpRecSrvInf->FtdUpd( 
		FsTdmfRecSrvInf::smszFldSrvIp1,    
		IpSzToInt ( "121.223.022.022" )
	);
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsType,    "W2K" );
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsVersion, "SP1" );
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     562 );
	//lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldNumOfVol, 2 );
	//lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldStatus, "Tracking" );
		lpDb->mpRecGrp->FtdNew( "001" );
		lpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,    "C:\\Pstore" );
		lpDb->mpRecGrp->FtdNew( "002" );
		lpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,    "C:\\Pstore" );

	lpDb->mpRecSrvInf->FtdNew( "Server04" );
	lpDb->mpRecSrvInf->FtdUpd( 
		FsTdmfRecSrvInf::smszFldSrvIp1,    
		IpSzToInt ( "121.223.022.044" )
	);
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsType,    "W2K" );
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsVersion, "SP2" );
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     562 );
	//lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldNumOfVol, 4 );
	//lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldStatus, "PassThru" );
		lpDb->mpRecGrp->FtdNew( "001" );
		lpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,    "C:\\Pstore" );
		lpDb->mpRecGrp->FtdNew( "002" );
		lpDb->mpRecGrp->FtdUpd( FsTdmfRecLgGrp::smszFldPStore,    "C:\\Pstore" );

	lpDb->mpRecSrvInf->FtdNew( "Server05" );
	lpDb->mpRecSrvInf->FtdUpd( 
		FsTdmfRecSrvInf::smszFldSrvIp1,    
		IpSzToInt ( "132.123.123.012" )
	);
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsType,    "W2K" );
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldOsVersion, "SP2" );
	lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldIpPort,     560 );
	//lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldNumOfVol, 0 );
	//lpDb->mpRecSrvInf->FtdUpd( FsTdmfRecSrvInf::smszFldStatus, "Undefined" );

	//lpDb->mpRecSrvInf->FtdPos( "Server02" );
	//CString lszIp = lpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvIp1 );

	//AfxMessageBox ( IpIntToSz( lszIp ) );

	delete lpDb;

} // FvEx1::OnFills ()


int FvEx1::IpSzToInt ( CString pszIp )
{
	int liIp;

	int liIpSub = atoi ( pszIp.Mid (  0, 3 ) );
	liIp  = liIpSub*256*256*256;

	liIpSub = atoi ( pszIp.Mid (  4, 3 ) );
	liIp += liIpSub*256*256;

	liIpSub = atoi ( pszIp.Mid (  8, 3 ) );
	liIp += liIpSub*256;

	liIpSub = atoi ( pszIp.Mid ( 12, 3 ) );
	liIp += liIpSub;

	return liIp;

} // FvEx1::IpSzToInt ()


CString FvEx1::IpIntToSz ( unsigned puIp )
{
	int liIp1 = puIp/(256*256*256);
	puIp -= liIp1*256*256*256;

	int liIp2 = puIp/(256*256);
	puIp -= liIp2*256*256;

	int liIp3 = puIp/(256);
	puIp -= liIp3*256;

	CString lszIp;
	lszIp.Format ( "%0.3d:%0.3d:%0.3d:%0.3d", liIp1, liIp2, liIp3, puIp );

	return lszIp;

} // FvEx1::IpIntToSz ()


CString FvEx1::IpIntToSz ( CString pszIp )
{
	return IpIntToSz( atoi ( pszIp ) );

} // FvEx1::IpIntToSz ()

void FvEx1::OnShowTbl () 
{
	CListCtrl* lpLc = (CListCtrl*) GetDlgItem ( IDCLC_Tbl );
	lpLc->DeleteAllItems ();

	FsTdmfDb* lpDb = new FsTdmfDb ( "TdmfUsrDaniel", "usrdaniel", "ALRODRIGUE" );

	lpDb->mpRecDom->FtdPos( "Domain Alpha" );

	UpdateData ( TRUE );

	int liSrv = 0;

	lpDb->mpRecSrvInf->FtdFirst( m_szSql );
	do
	{
		CString lszSrvName = lpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldName );
		lpLc->InsertItem ( liSrv, lszSrvName );

		CString lszIp = 
			IpIntToSz( lpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvIp1 ) );
		lpLc->SetItemText ( liSrv, 1, lszIp );

		CString lszOsT = lpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldOsType );
		lpLc->SetItemText ( liSrv, 2, lszOsT );

		CString lszOsV = lpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldOsVersion );
		lpLc->SetItemText ( liSrv, 3, lszOsV );

		CString lszPort = lpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldIpPort );
		lpLc->SetItemText ( liSrv, 5, lszPort );

		int liDef = lpDb->mpRecGrp->FtdCount();
		CString lszDef;
		lszDef.Format ( "%d", liDef );
		lpLc->SetItemText ( liSrv, 4, lszDef );

		liSrv++;
	}
	while ( lpDb->mpRecSrvInf->FtdNext() );

	lpDb->mpRecSrvInf->FtdPos( "Server01" );

	m_szSrvName = lpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldName );

	CString lszIp = 
		IpIntToSz( lpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldSrvIp1 ) );
	CString lszPort = lpDb->mpRecSrvInf->FtdSel( FsTdmfRecSrvInf::smszFldIpPort );

	m_szAdd.Format ( "%s:%s", lszIp, lszPort );

	UpdateData ( FALSE );

	delete lpDb;

} // FvEx1::OnShowTbl ()


