// Event.cpp : Implementation of CEvent
#include "stdafx.h"
#include "TDMFObjects.h"
#include "System.h"
#include "Event.h"
#include "FsTdmfDb.h"


/////////////////////////////////////////////////////////////////////////////
// CEvent

bool CEvent::Initialize
		(
			FsTdmfRecAlert* pRec,
			std::string     pStrSrvName
		)
{
	m_nIk               = atol( pRec->FtdSel( FsTdmfRecAlert::smszFldIk ) );

	CString strDateTime = pRec->FtdSel( FsTdmfRecAlert::smszFldTs );
	m_strDate           = (LPCTSTR)(strDateTime.Left( 11 ) );
	m_strTime           = (LPCTSTR)(strDateTime.Right( 12 ) );
	m_strTime.resize(8);

	m_strSource = pStrSrvName;

	m_nGroupID          = atol( pRec->FtdSel( FsTdmfRecAlert::smszFldGrpFk ) );
	m_nPairID           = atol( pRec->FtdSel( FsTdmfRecAlert::smszFldPairFk ) );

	CString strType     = pRec->FtdSel( FsTdmfRecAlert::smszFldType );
	strType.TrimRight(' ');
	m_strType           = (LPCTSTR)(strType);

	m_nSeverity         = atol( pRec->FtdSel( FsTdmfRecAlert::smszFldSev ) );
	m_strDescription    = (LPCTSTR)pRec->FtdSel( FsTdmfRecAlert::smszFldTxt );

	return true;

} // CEvent::Initialize ()


/////////////////////////////////////////////////////////////////////////////
// CEventList


// ardev 021121 v

void CEventList::Reset()
{
	m_mapEvent.clear();
}


CEvent* CEventList::ReadRangeFromDb 
	(
		CSystem* ppSystem,
		long     pnIndex,
		long     pnServerId,
		long     pnGroupId
	)
{
	CString         lszWhere;
	FsTdmfRecAlert* lpRec = ppSystem->GetDB()->mpRecAlert;

	if ((pnServerId >= 0) && (pnGroupId >= 0))
	{
		// WHERE Source_Fk = pnServerId AND LgGroupId_Fk = pnGroupId AND Time_Stamp > m_strLastEventDateTime AND Ik != m_nLastEvent
		lszWhere.Format
			(
				" %s = %ld AND %s = %ld ",
				FsTdmfRecAlert::smszFldSrcFk, pnServerId,
				FsTdmfRecAlert::smszFldGrpFk, pnGroupId
			);
	}
	else if (pnServerId >= 0)
	{
		// WHERE Source_Fk = pnServerId AND Time_Stamp > m_strLastEventDateTime AND Ik != m_nLastEvent
		lszWhere.Format( " %s = %ld ", FsTdmfRecAlert::smszFldSrcFk, pnServerId );
	}
	else
	{
		// WHERE Time_Stamp >= m_strLastTimeStamp AND Ik != m_nLastEvent
		lszWhere.Format( " %s >= 0 ", FsTdmfRecAlert::smszFldIk );
	}

	CString cszOrder = FsTdmfRecAlert::smszFldTs + " ASC ";

	long llRangeMin = pnIndex - EVENT_PAGE_SIZE;
	long llRangeMax = pnIndex + EVENT_PAGE_SIZE;
	llRangeMin = (llRangeMin >0?llRangeMin:1);

	for ( long llC = llRangeMin; llC <= llRangeMax; llC++ )
	{
		if ( !GetAt( llC ) )
		{
			llRangeMin = llC;
			break;
		}
	}

	CEvent      lEventNew;
	std::string lStrSrvName;

	if ( !lpRec->FtdFirst( lszWhere, cszOrder ) )
	{
		return NULL;
	}
	else for ( llC = llRangeMin; llC <= llRangeMax; llC++ )
	{
		if ( !lpRec->FtdMove ( llC ) ) 
		{
			break;
		}
		else if ( !GetAt( llC ) )
		{
			if ( pnServerId < 0 )
			{
				long lnSrcFk = atol( lpRec->FtdSel( FsTdmfRecAlert::smszFldSrcFk ) );
				lStrSrvName = ppSystem->GetServerName( lnSrcFk );
			}
			lEventNew.Initialize( lpRec, lStrSrvName );
			m_mapEvent[llC] = lEventNew;
		}
		else
		{
			break;
		}
	}

	return GetAt ( pnIndex );

} // CEventList::ReadRangeFromDb ()


CEvent* CEventList::GetAt ( long nIndex )
{
	if ( m_mapEvent.find(nIndex) != m_mapEvent.end() )
	{
		return &(m_mapEvent[nIndex]);
	}
	else
	{
		return NULL;
	}

} // CEventList::GetAt ()

// ardev 021121 ^

