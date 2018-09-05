// ReplicationPair.cpp : Implementation of CReplicationPair
#include "stdafx.h"
#include "ReplicationPair.h"
#include "ReplicationGroup.h"

#include "FsTdmfDb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReplicationPair

FsTdmfDb* CReplicationPair::GetDB() 
{ 
    return m_pParent->GetDB(); 
}


bool CReplicationPair::Initialize(FsTdmfRecPair *pRec)
{
	m_iGrpFk            = atoi( pRec->FtdSel( FsTdmfRecPair::smszFldGrpFk ) );
	m_iSrcFk            = atoi( pRec->FtdSel( FsTdmfRecPair::smszFldSrcFk ) );

	m_iDbIk             = atoi( pRec->FtdSel( FsTdmfRecPair::smszFldIk ) );
	m_nPairNumber       = atol( pRec->FtdSel( FsTdmfRecPair::smszFldPairId ) );
	m_nState            = atol( pRec->FtdSel( FsTdmfRecPair::smszFldState ) );
	m_strDescription    = (LPCTSTR)pRec->FtdSel( FsTdmfRecPair::smszFldNotes );
    m_strFileSystem     = (LPCTSTR)pRec->FtdSel( FsTdmfRecPair::smszFldFS );

	m_DeviceSource.m_strPath        = (LPCTSTR)pRec->FtdSel( FsTdmfRecPair::smszFldSrcDisk );
	m_DeviceSource.m_strDriveId     = (LPCTSTR)pRec->FtdSel( FsTdmfRecPair::smszFldSrcDriveId );
	m_DeviceSource.m_strStartOff    = (LPCTSTR)pRec->FtdSel( FsTdmfRecPair::smszFldSrcStartOff );
	m_DeviceSource.m_strLength      = (LPCTSTR)pRec->FtdSel( FsTdmfRecPair::smszFldSrcLength );
    m_DeviceSource.m_strFileSystem  = m_strFileSystem;

	m_DeviceTarget.m_strPath        = (LPCTSTR)pRec->FtdSel( FsTdmfRecPair::smszFldTgtDisk );
	m_DeviceTarget.m_strDriveId     = (LPCTSTR)pRec->FtdSel( FsTdmfRecPair::smszFldTgtDriveId );
	m_DeviceTarget.m_strStartOff    = (LPCTSTR)pRec->FtdSel( FsTdmfRecPair::smszFldTgtStartOff );
	m_DeviceTarget.m_strLength      = (LPCTSTR)pRec->FtdSel( FsTdmfRecPair::smszFldTgtLength );
    m_DeviceTarget.m_strFileSystem  = m_strFileSystem;

	// Read from DB so... set object state to saved
	m_eObjectState = RPO_SAVED;

    return true;
}

