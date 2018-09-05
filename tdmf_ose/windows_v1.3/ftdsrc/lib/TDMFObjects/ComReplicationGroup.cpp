// ComReplicationGroup.cpp : Implementation of CComReplicationGroup
#include "stdafx.h"
#include "TDMFObjects.h"
#include "ComReplicationPair.h"
#include "ComReplicationGroup.h"
#include "ComServer.h"
#include "ComEvent.h"
#include <sstream>
#include <iomanip>


/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CComReplicationGroup::get_Name(BSTR *pVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		std::ostringstream oss;
		oss	<< ((m_pRG->m_nType & CReplicationGroup::GT_SOURCE) ? "P" : "S") 
			<< std::setw(3) << std::setfill('0') << m_pRG->m_nGroupNumber;
		*pVal = CComBSTR(oss.str().c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_Parent(IServer **ppServer)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (ppServer != NULL)
	{
		CComObject<CComServer>* pComServer = NULL;
		hr = CComObject<CComServer>::CreateInstance(&pComServer);
		if (SUCCEEDED(hr))
		{
			pComServer->m_pServer = m_pRG->m_pParent;
			pComServer->AddRef();
			hr = pComServer->QueryInterface(ppServer);
			pComServer->Release();
		}
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_State(ElementState *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = (ElementState)m_pRG->m_eState;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_Mode(GroupMode *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = (GroupMode)m_pRG->m_nMode;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_Mode(GroupMode newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nMode = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_Sync(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bSync;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_Sync(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bSync = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_Chaining(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bChaining;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_Chaining(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bChaining = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_StateTimeStamp(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strStateTimeStamp.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_Description(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strDescription.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_Description(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_strDescription = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_GroupNumber(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nGroupNumber;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_GroupNumber(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (newVal != m_pRG->m_nGroupNumber)
	{
		m_pRG->m_nGroupNumber = newVal;
	}

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_TargetIPAddress(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		if (m_pRG->m_pServerTarget != NULL)
		{
			*pVal = CComBSTR(m_pRG->m_pServerTarget->m_vecstrIPAddress[0].c_str()).Copy();
		}
		else
		{
			*pVal = CComBSTR("").Copy();
		}
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_JournalDirectory(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strJournalDirectory.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_JournalDirectory(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_strJournalDirectory = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_PStoreDirectory(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strPStoreFile.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_PStoreDirectory(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_strPStoreFile = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_SyncDepth(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nSyncDepth;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_SyncDepth(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nSyncDepth = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_SyncTimeout(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nSyncTimeout;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_SyncTimeout(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nSyncTimeout = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_EnableCompression(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bEnableCompression;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_EnableCompression(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bEnableCompression = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_RefreshNeverTimeout(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bRefreshNeverTimeout;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_RefreshNeverTimeout(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bRefreshNeverTimeout = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_RefreshTimeoutInterval(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nRefreshTimeoutInterval;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_RefreshTimeoutInterval(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nRefreshTimeoutInterval = newVal;

	return S_OK;
}


STDMETHODIMP CComReplicationGroup::get_ReplicationPairCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_listReplicationPair.size();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::GetReplicationPair(long nIndex, IReplicationPair **ppReplicationPair)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	*ppReplicationPair = NULL;

	if ((nIndex < 0) || (ppReplicationPair == NULL) || ((UINT)nIndex >= m_pRG->m_listReplicationPair.size()))
	{
		hr = E_INVALIDARG;
	}
	else
	{
		std::list<CReplicationPair>::iterator it;
		long nCount;
		for (it = m_pRG->m_listReplicationPair.begin(), nCount = 0;
			 (nCount < nIndex) && (it != m_pRG->m_listReplicationPair.end()); it++, nCount++);
		if (nCount == nIndex)
		{
			CComObject<CComReplicationPair>* pComRP = NULL;
			hr = CComObject<CComReplicationPair>::CreateInstance(&pComRP);
			if (SUCCEEDED(hr))
			{
				CReplicationPair& RP = *it;
				pComRP->m_pRP = &RP;
				pComRP->AddRef();
				hr = pComRP->QueryInterface(ppReplicationPair);
				pComRP->Release();
			}
		}
		else
		{
			hr = E_INVALIDARG;
		}
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::RemoveReplicationPair(IReplicationPair *pReplicationPair)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CComBSTR bstrKey;
	if (SUCCEEDED(pReplicationPair->get_Key(&bstrKey)))
	{
		std::list<CReplicationPair>::iterator it = m_pRG->m_listReplicationPair.begin();
		while(it != m_pRG->m_listReplicationPair.end())
		{
			if (it->GetKey() == OLE2A(bstrKey))
			{
				m_pRG->m_listReplicationPair.erase(it);

				// Remove it from DB

				break;
			}
			it++;
		}
	}

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::CreateNewReplicationPair(IReplicationPair **ppNewReplicationPair)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (ppNewReplicationPair == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		// Create a new Replication Pair
		CReplicationPair& RP = m_pRG->AddNewReplicationPair();

		CComObject<CComReplicationPair>* pComRP = NULL;
		hr = CComObject<CComReplicationPair>::CreateInstance(&pComRP);
		if (SUCCEEDED(hr))
		{
			pComRP->m_pRP = &RP;
			pComRP->AddRef();
			hr = pComRP->QueryInterface(ppNewReplicationPair);
			pComRP->Release();
		}		
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_TargetName(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		if (m_pRG->m_pServerTarget != NULL)
		{
			*pVal = CComBSTR(m_pRG->m_pServerTarget->m_strName.c_str()).Copy();
		}
		else
		{
			*pVal = NULL;
		}
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_TargetServer(IServer **ppTargetServer)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (ppTargetServer == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
        if (m_pRG->m_pServerTarget != NULL)
        {
			CComObject<CComServer>* pComServer = NULL;
			hr = CComObject<CComServer>::CreateInstance(&pComServer);
			if (SUCCEEDED(hr))
			{
				pComServer->m_pServer = m_pRG->m_pServerTarget;
				pComServer->AddRef();
				hr = pComServer->QueryInterface(ppTargetServer);
				pComServer->Release();
			}
        }
        else 
        {
            *ppTargetServer = NULL;
            hr = S_OK;
        }
    }

	return hr;
}

STDMETHODIMP CComReplicationGroup::putref_TargetServer(IServer *newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	long nHostID;
	IDomain* pDomain = NULL;

	if (newVal != NULL)
	{
		
		if (SUCCEEDED(newVal->get_HostID(&nHostID)) && 
			SUCCEEDED(newVal->get_Parent(&pDomain)))
		{
			CComBSTR bstrName;

			if ((pDomain != NULL) && SUCCEEDED(pDomain->get_Name(&bstrName)))
			{
				if (m_pRG->SetNewTargetServer(OLE2A(bstrName), nHostID))
				{
					pDomain->Release();
					
					return S_OK;
				}
			}
		}
	}
	else
	{
		m_pRG->setTarget(NULL);

		return S_OK;
	}

	if (pDomain != NULL)
	{
		pDomain->Release();
	}

	return E_FAIL;
}

STDMETHODIMP CComReplicationGroup::get_Key(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->GetKey().c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::IsEqual(IReplicationGroup* pRG, BOOL *pbRetVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CComBSTR bstrKey;
	HRESULT hr = pRG->get_Key(&bstrKey);
	if (SUCCEEDED(hr))
	{
		*pbRetVal = (m_pRG->GetKey() == OLE2A(bstrKey));
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_ConnectionStatus(ConnectionStatus *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = (ConnectionStatus)m_pRG->m_nConnectionState;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_ConnectionStatus(ConnectionStatus newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nConnectionState = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_ChunkDelay(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nChunkDelay;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_ChunkDelay(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nChunkDelay = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_MaxFileStatSize(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nMaxFileStatSize;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_MaxFileStatSize(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nMaxFileStatSize = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_PctDone(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nPctDone;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::SaveToDB(long nOldGroupNumber, long nOldTgtHostId, BSTR* WarningsMsg, TdmfErrorCode* pRetVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (pRetVal != NULL)
	{
		std::string strWarning;

		*pRetVal = (TdmfErrorCode)m_pRG->SaveToDB(nOldGroupNumber, nOldTgtHostId, &strWarning);
	
		if (WarningsMsg != NULL)
		{
			*WarningsMsg = CComBSTR(strWarning.c_str()).Copy();
		}

		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP CComReplicationGroup::LaunchCommand(TdmfCommand eCmd, BSTR pszOptions, BSTR Log,  BOOL Symmetric, BSTR* Message, long *pRetCode)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CString cstrMsg;

	*pRetCode = m_pRG->LaunchCommand((enum tdmf_commands)eCmd, OLE2A(pszOptions), OLE2A(Log), Symmetric, cstrMsg);

	// return a copy of the returned msg
	CComBSTR bstrMsg = cstrMsg;
	*Message = bstrMsg.Copy();

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_JournalSize(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		CString strBuf;
		strBuf.Format("%I64d", m_pRG->m_liJournalSize);
		*pVal = CComBSTR(strBuf).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_DiskTotalSize(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		CString strBuf;
		strBuf.Format("%I64d", m_pRG->m_liDiskTotalSize);
		*pVal = CComBSTR(strBuf).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_DiskFreeSize(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		CString strBuf;
		strBuf.Format("%I64d", m_pRG->m_liDiskFreeSize);
		*pVal = CComBSTR(strBuf).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::GetUniqueGroupNumber(long *NewGroupNumber)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (NewGroupNumber != NULL)
	{
		*NewGroupNumber = m_pRG->GetUniqueGroupNumber();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_LastCmdOutput(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strLastCmdOutput.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_CmdHistory(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strCmdHistory.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::GetEventFirst(IEvent **ppEvent)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (ppEvent == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		// Create a new COM Event
		CEvent* pEvent = m_pRG->GetEventFirst();

		if (pEvent != NULL)
		{
			CComObject<CComEvent>* pComEvent = NULL;
			hr = CComObject<CComEvent>::CreateInstance(&pComEvent);
			if (SUCCEEDED(hr))
			{
				pComEvent->m_pEvent = pEvent;
				pComEvent->AddRef();
				hr = pComEvent->QueryInterface(ppEvent);
				pComEvent->Release();
			}
		}
		else // Empty list
		{
			*ppEvent = NULL;
			hr = S_OK;
		}

	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_IsSource(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nType & CReplicationGroup::GT_SOURCE;

		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::GetTargetGroup(IReplicationGroup **pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (pVal == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		// Create a new COM Event
		CReplicationGroup* pRGTarget = m_pRG->GetTargetGroup();

		if (pRGTarget != NULL)
		{
			CComObject<CComReplicationGroup>* pComRG = NULL;
			hr = CComObject<CComReplicationGroup>::CreateInstance(&pComRG);
			if (SUCCEEDED(hr))
			{
				pComRG->m_pRG = pRGTarget;
				pComRG->AddRef();
				hr = pComRG->QueryInterface(pVal);
				pComRG->Release();
			}
		}
		else // End of list
		{
			*pVal = NULL;
			hr = S_OK;
		}
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::CreateAssociatedTargetGroup(IReplicationGroup** pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (pVal == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		// Create a new COM RG
		CReplicationGroup* pRGTarget = m_pRG->CreateAssociatedTargetGroup();

		if (pRGTarget != NULL)
		{
			CComObject<CComReplicationGroup>* pComRG = NULL;
			hr = CComObject<CComReplicationGroup>::CreateInstance(&pComRG);
			if (SUCCEEDED(hr))
			{
				pComRG->m_pRG = pRGTarget;
				pComRG->AddRef();
				hr = pComRG->QueryInterface(pVal);
				pComRG->Release();
			}
		}
		else // End of list
		{
			*pVal = NULL;
			hr = S_OK;
		}
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_ReadKbps(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		char szBuf[20];
		*pVal = CComBSTR(_i64toa(m_pRG->m_nReadKbps, szBuf, 10)).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_WriteKbps(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		char szBuf[20];
		*pVal = CComBSTR(_i64toa(m_pRG->m_nWriteKbps, szBuf, 10)).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_ActualNet(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		char szBuf[20];
		*pVal = CComBSTR(_i64toa(m_pRG->m_nActualNet, szBuf, 10)).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_EffectiveNet(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		char szBuf[20];
		*pVal = CComBSTR(_i64toa(m_pRG->m_nEffectiveNet, szBuf, 10)).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_BABEntries(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nBABEntries;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_PctBAB(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nPctBAB;
		hr = S_OK;
	}

	return hr;
}

// ardev 021129 v
STDMETHODIMP CComReplicationGroup::GetEventAt ( long nIndex, IEvent** ppEvent )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (ppEvent == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		// Create a new COM Event
		CEvent* pEvent = m_pRG->GetEventAt( nIndex );

		if (pEvent != NULL)
		{
			CComObject<CComEvent>* pComEvent = NULL;
			hr = CComObject<CComEvent>::CreateInstance(&pComEvent);
			if (SUCCEEDED(hr))
			{
				pComEvent->m_pEvent = pEvent;
				pComEvent->AddRef();
				hr = pComEvent->QueryInterface(ppEvent);
				pComEvent->Release();
			}
		}
		else // End of list
		{
			*ppEvent = NULL;
			hr = S_OK;
		}
	}

	return hr;

} // CComReplicationGroup::GetEventAt ()


STDMETHODIMP CComReplicationGroup::GetEventCount ( long* pnRow )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if ( pnRow == NULL )
	{
		return E_FAIL;
	}

	*pnRow = m_pRG->GetEventCount();

	return S_OK;

} // CComReplicationGroup::GetEventCount ()


STDMETHODIMP CComReplicationGroup::IsEventAt ( long nIndex, BOOL* pbLoaded )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if ( pbLoaded == NULL )
	{
		return E_FAIL;
	}

	*pbLoaded = m_pRG->IsEventAt( nIndex );

	return S_OK;

} // CComReplicationGroup::IsEventAt ()

// ardev 021129 ^



STDMETHODIMP CComReplicationGroup::SaveTunables(TdmfErrorCode *RetVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (RetVal != NULL)
	{
		*RetVal = (TdmfErrorCode)m_pRG->SaveTunables();
	
		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP CComReplicationGroup::SetTunables(TdmfErrorCode *RetVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (RetVal != NULL)
	{
		*RetVal = (TdmfErrorCode)m_pRG->SetTunables();
	
		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP CComReplicationGroup::get_ChunkSize(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nChunkSizeKB;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_ChunkSize(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nChunkSizeKB = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_NetThreshold(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bNetThreshold;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_NetThreshold(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bNetThreshold = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_NetMaxKbps(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nNetMaxKbps;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_NetMaxKbps(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nNetMaxKbps = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_StatInterval(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nStatInterval;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_StatInterval(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nStatInterval = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_JournalLess(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bJournalLess;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_JournalLess(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bJournalLess = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::IsTargetDHCPAdressUsed(BOOL *pVal)
{

    AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bTargetDHCPAdressUsed;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::SetTargetDHCPAddressUsed(BOOL bVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bTargetDHCPAdressUsed = bVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::IsPrimaryDHCPAdressUsed(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bPrimaryDHCPAdressUsed;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::IsPrimaryEditedIPUsed(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bPrimaryEditedIPUsed;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::IsTargetEditedIPUsed(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bTargetEditedIPUsed;
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::SetPrimaryDHCPAddressUsed(BOOL bVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bPrimaryDHCPAdressUsed = bVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_PrimaryEditedIPAdress(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strPrimaryEditedIP.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_PrimaryEditedIPAdress(BSTR newVal)
{

    USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_strPrimaryEditedIP = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_TargetEditedIPAdress(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
        if(m_pRG->m_pServerTarget != NULL)
        {
		    *pVal = CComBSTR(m_pRG->m_strTargetEditedIP.c_str()).Copy();
		    hr = S_OK;
        }
        else
		{
			*pVal = CComBSTR("").Copy();
            hr = S_OK;
		}
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_TargetEditedIPAdress(BSTR newVal)
{
    USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

    if(m_pRG->m_pServerTarget!= NULL)
    {
        m_pRG->m_strTargetEditedIP = OLE2A(newVal);
    }
	
	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_PrimaryIPAdress(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		if (m_pRG->m_pParent != NULL)
		{
			*pVal = CComBSTR(m_pRG->m_pParent->m_vecstrIPAddress[0].c_str()).Copy();
		}
		else
		{
			*pVal = CComBSTR("").Copy();
		}
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_PrimaryIPAdress(BSTR newVal)
{
	USES_CONVERSION;
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_pParent->m_vecstrIPAddress[0] = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_PrimaryName(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
	   *pVal = CComBSTR(m_pRG->m_pParent->m_strName.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::SetPrimaryEditedAddressUsed(BOOL bVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bPrimaryEditedIPUsed = bVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::SetTargetEditedAddressUsed(BOOL bVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bTargetEditedIPUsed = bVal;

	return S_OK;
}



STDMETHODIMP CComReplicationGroup::LockCmds()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->LockCmds();

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::UnlockCmds()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->UnlockCmds();

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_IsLockCmds(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bLockCmds;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_Throttles(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strThrottles.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_PrimaryEditedIP(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strPrimaryEditedIP.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_PrimaryEditedIP(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_strPrimaryEditedIP = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_TargetEditedIP(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strTargetEditedIP.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_TargetEditedIP(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_strTargetEditedIP = OLE2A(newVal);

	return S_OK;
}


STDMETHODIMP CComReplicationGroup::get_ForcePMDRestart(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bForcePMDRestart;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_ForcePMDRestart(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bForcePMDRestart = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_Symmetric(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bSymmetric;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_Symmetric(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bSymmetric = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::RemoveSymmetricGroup()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return m_pRG->RemoveSymmetricGroup() ? S_OK : S_FALSE;
}

STDMETHODIMP CComReplicationGroup::get_SymmetricGroupNumber(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nSymmetricGroupNumber;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_SymmetricGroupNumber(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nSymmetricGroupNumber = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_SymmetricNormallyStarted(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_bSymmetricNormallyStarted;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_SymmetricNormallyStarted(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_bSymmetricNormallyStarted = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_FailoverInitialState(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pRG->m_nFailoverInitialState;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_FailoverInitialState(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_nFailoverInitialState = newVal;

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_SymmetricConnectionStatus(ConnectionStatus *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = (ConnectionStatus)m_pRG->m_nSymmetricConnectionState;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::get_SymmetricMode(GroupMode *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = (GroupMode)m_pRG->m_nSymmetricMode;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::Failover(long* Warning, long *Result)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	long nWarning;
	long nErr = m_pRG->Failover(nWarning);

	if (Result != NULL)
	{
		*Result = nErr;
	}
	if (Warning != NULL)
	{
		*Warning = nWarning;
	}

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_SymmetricPStore(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strSymmetricPStoreFile.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_SymmetricPStore(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_strSymmetricPStoreFile = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComReplicationGroup::get_SymmetricJournal(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pRG->m_strSymmetricJournalDirectory.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComReplicationGroup::put_SymmetricJournal(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pRG->m_strSymmetricJournalDirectory = OLE2A(newVal);

	return S_OK;
}
