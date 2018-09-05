// ComServer.cpp : Implementation of CComServer
#include "stdafx.h"
#include "TDMFObjects.h"
#include "ComDomain.h"
#include "ComScriptServerFile.h"
#include "ComServer.h"
#include "ComReplicationGroup.h"
#include "ComEvent.h"
#include "ComDeviceList.h"
#include "System.h"
#include "mmp_api.h"
#include <iomanip>


/////////////////////////////////////////////////////////////////////////////
// CComServer

STDMETHODIMP CComServer::get_Name(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strName.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_ReplicationGroupCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_listReplicationGroup.size();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::GetReplicationGroup(long nIndex, IReplicationGroup **ppReplicationGroup)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	if ((nIndex < 0) || (ppReplicationGroup == NULL) || ((UINT)nIndex >= m_pServer->m_listReplicationGroup.size()))
	{
		hr = E_INVALIDARG;
	}
	else
	{
		*ppReplicationGroup = NULL;

		std::list<CReplicationGroup>::iterator it;
		long nCount;
		for (it = m_pServer->m_listReplicationGroup.begin(), nCount = 0;
			 (nCount < nIndex) && (it != m_pServer->m_listReplicationGroup.end()); it++, nCount++);
		if (nCount == nIndex)
		{
			CComObject<CComReplicationGroup>* pComRG = NULL;
			hr = CComObject<CComReplicationGroup>::CreateInstance(&pComRG);
			if (SUCCEEDED(hr))
			{				
				CReplicationGroup& RG = *it;
				pComRG->m_pRG = &RG;
				pComRG->AddRef();
				hr = pComRG->QueryInterface(ppReplicationGroup);
				pComRG->Release();
			}
		}
		else
		{
			hr = E_INVALIDARG;
		}
	}

	return hr;
}

STDMETHODIMP CComServer::get_Parent(IDomain **ppDomain)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (ppDomain != NULL)
	{
		*ppDomain = NULL;

		CComObject<CComDomain>* pComDomain = NULL;
		hr = CComObject<CComDomain>::CreateInstance(&pComDomain);
		if (SUCCEEDED(hr))
		{
			pComDomain->m_pDomain = m_pServer->m_pParent;
			pComDomain->AddRef();
			hr = pComDomain->QueryInterface(ppDomain);
			pComDomain->Release();
		}
	}

	return hr;
}

STDMETHODIMP CComServer::CreateNewReplicationGroup(IReplicationGroup **ppNewReplicationGroup)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (ppNewReplicationGroup == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		// Create a new replication group
		CReplicationGroup& RG = m_pServer->AddNewReplicationGroup();
		RG.m_nGroupNumber = RG.GetUniqueGroupNumber();

		CComObject<CComReplicationGroup>* pComRG = NULL;
		hr = CComObject<CComReplicationGroup>::CreateInstance(&pComRG);
		if (SUCCEEDED(hr))
		{
			pComRG->m_pRG = &RG;
			pComRG->AddRef();
			hr = pComRG->QueryInterface(ppNewReplicationGroup);
			pComRG->Release();
		}
	}

	return hr;
}


STDMETHODIMP CComServer::MoveTo(IDomain *pDomain, long *pRetCode)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr;
	long nKey;

	if (SUCCEEDED(hr = pDomain->get_Key(&nKey)))
	{
		*pRetCode = m_pServer->MoveToDomain(nKey, &m_pServer);
	}
	
	return hr;
}

STDMETHODIMP CComServer::RemoveReplicationGroup(IReplicationGroup *pReplicationGroup, long *pRetVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;
	*pRetVal = MMPAPI_Error::OK;

	CComBSTR bstrKey;
	if (SUCCEEDED(pReplicationGroup->get_Key(&bstrKey)))
	{
		std::list<CReplicationGroup>::iterator it = m_pServer->m_listReplicationGroup.begin();
		while(it != m_pServer->m_listReplicationGroup.end())
		{
			if (it->GetKey() == OLE2A(bstrKey))
			{
				// Remove it from DB
				*pRetVal = it->RemoveFromDB();

				if (*pRetVal == MMPAPI_Error::OK)
				{
					m_pServer->m_listReplicationGroup.erase(it);
				}
				break;
			}
			it++;
		}
	}

	return hr;
}

STDMETHODIMP CComServer::LaunchCommand(TdmfCommand eCmd, BSTR  pszOptions, BSTR Log, BSTR* Message, long* pRetCode)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CString cstrMsg;

	*pRetCode = m_pServer->LaunchCommand((enum tdmf_commands)eCmd, OLE2A(pszOptions), OLE2A(Log), cstrMsg);

	// return a copy of the returned msg
	CComBSTR bstrMsg = cstrMsg;
	*Message = bstrMsg.Copy();

	return S_OK;
}

STDMETHODIMP CComServer::get_IPAddress(long Index, BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_vecstrIPAddress[Index].c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_IPAddressCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_vecstrIPAddress.size();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_OSType(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strOSType.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_OSVersion(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strOSVersion.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_AgentVersion(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strAgentVersion.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_KeyExpirationDate(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strKeyExpirationDate.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_State(ElementState *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = (ElementState)m_pServer->m_eState;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_BABSize(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_nBABSizeReq;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::put_BABSize(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->m_nBABSizeReq = newVal;

	return S_OK;
}

STDMETHODIMP CComServer::get_BABSizeAllocated(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_nBABSizeAct;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_PStoreDirectory(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strPStoreFile.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::put_PStoreDirectory(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->m_strPStoreFile = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComServer::get_TCPWndSize(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_nTCPWndSize;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::put_TCPWndSize(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->m_nTCPWndSize = newVal;

	return S_OK;
}

STDMETHODIMP CComServer::get_Port(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_nPort;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::put_Port(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->m_nPort = newVal;

	return S_OK;
}

STDMETHODIMP CComServer::get_Key(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->GetKey();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_Description(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strDescription.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::put_Description(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->m_strDescription = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComServer::get_HostID(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_nHostID;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_RAMSize(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_nRAMSizeKB;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_RegKey(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strKey.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::put_RegKey(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->m_strKey = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComServer::IsEqual(IServer *pServer, BOOL* pbRetVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
    if(pServer == NULL)
	{
		*pbRetVal = FALSE;
		return S_OK;
 	}

	long nKey;
	HRESULT hr = pServer->get_Key(&nKey);
	if (SUCCEEDED(hr))
	{
		*pbRetVal = (m_pServer->GetKey() == nKey);
	}

	return hr;
}

STDMETHODIMP CComServer::get_Connected(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_bConnected;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_PerformanceNotifications(BOOL* pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_bEnablePerformanceNotifications;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::put_PerformanceNotifications(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (m_pServer->EnablePerformanceNotifications(newVal))
	{
		m_bEnablePerformanceNotifications = newVal;

		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP CComServer::get_PctBAB(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_nPctBAB;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_BABEntries(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_nEntries;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_TargetReplicationGroupCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->GetTargetReplicationGroupCount();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_JournalDirectory(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strJournalDirectory.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::put_JournalDirectory(BSTR newVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->m_strJournalDirectory = OLE2A(newVal);

	return S_OK;
}

STDMETHODIMP CComServer::get_ReplicationPairCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->GetReplicationPairCount();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_TargetReplicationPairCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->GetTargetReplicationPairCount();
		hr = S_OK;
	}

	return hr;
}


STDMETHODIMP CComServer::GetEventFirst(IEvent **ppEvent)
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
		CEvent* pEvent = m_pServer->GetEventFirst();

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

STDMETHODIMP CComServer::SaveToDB(/*[out, retval]*/ TdmfErrorCode* pRetVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pRetVal != NULL)
	{
		*pRetVal = (TdmfErrorCode)m_pServer->SaveToDB();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::GetDeviceList(IDeviceList **ppDeviceList)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (ppDeviceList == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		CDeviceList* pDeviceList;
		m_pServer->GetDeviceList(&pDeviceList);

		CComObject<CComDeviceList>* pComDeviceList = NULL;
		hr = CComObject<CComDeviceList>::CreateInstance(&pComDeviceList);
		if (SUCCEEDED(hr))
		{
			pComDeviceList->m_pDeviceList = pDeviceList;
			pComDeviceList->AddRef();
			hr = pComDeviceList->QueryInterface(ppDeviceList);
			pComDeviceList->Release();
		}
	}

	return hr;
}

STDMETHODIMP CComServer::GetDeviceLists(IServer* pServerTarget,
										IDeviceList** ppDeviceList,
										IDeviceList** ppDeviceListTarget)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if ((ppDeviceList == NULL) || (ppDeviceListTarget == NULL))
	{
		hr = E_INVALIDARG;
	}
	else
	{
		CDeviceList* pDeviceList;
		CDeviceList* pDeviceListTarget;

		if (pServerTarget != NULL)
		{
			long nHostID;
			hr = pServerTarget->get_HostID(&nHostID);
			m_pServer->GetDeviceList(&pDeviceList, nHostID, &pDeviceListTarget);
		}
		else
		{
			m_pServer->GetDeviceList(&pDeviceList);
		}

		// Source List
		CComObject<CComDeviceList>* pComDeviceList = NULL;
		hr = CComObject<CComDeviceList>::CreateInstance(&pComDeviceList);
		if (SUCCEEDED(hr))
		{
			pComDeviceList->m_pDeviceList = pDeviceList;
			pComDeviceList->AddRef();
			hr = pComDeviceList->QueryInterface(ppDeviceList);
			pComDeviceList->Release();
		}

		// Target List
		if (SUCCEEDED(hr) && (pServerTarget != NULL))
		{
			CComObject<CComDeviceList>* pComDeviceList = NULL;
			hr = CComObject<CComDeviceList>::CreateInstance(&pComDeviceList);
			if (SUCCEEDED(hr))
			{
				pComDeviceList->m_pDeviceList = pDeviceListTarget;
				pComDeviceList->AddRef();
				hr = pComDeviceList->QueryInterface(ppDeviceListTarget);
				pComDeviceList->Release();
			}
		}
	}

	return hr;
}

STDMETHODIMP CComServer::GetPerformanceValues(BSTR Stats, DATE DateBegin, DATE DateEnd, BSTR* RetVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	USES_CONVERSION;

	HRESULT hr = E_INVALIDARG;

	if (RetVal != NULL)
	{
		*RetVal = CComBSTR(m_pServer->GetPerformanceValues(OLE2A(Stats), DateBegin, DateEnd)).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_PStoreSize(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		CString strSize;

		if (m_pServer->m_liPStoreSize > 1024)
		{
			__int64 liSizeKB = m_pServer->m_liPStoreSize/1024;
			strSize.Format("%I64d KB", liSizeKB);
		}
		else
		{
			strSize.Format("%I64d Bytes", m_pServer->m_liPStoreSize);
		}

		*pVal = CComBSTR(strSize).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_LastCmdOutput(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strLastCmdOutput.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_CmdHistory(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pServer->m_strCmdHistory.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_JournalDriveCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_mapJournalDrive.size();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_JournalSize(long Index, BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if ((pVal != NULL) && ((Index >= 0) && ((UINT)Index < m_pServer->m_mapJournalDrive.size())))
	{
		__int64 liJournalSize = 0;

		long nItem = 0;
		for(std::map<std::string, CJournalDrive>::iterator it = m_pServer->m_mapJournalDrive.begin();
			it != m_pServer->m_mapJournalDrive.end(); it++)
		{
			if (nItem == Index)
			{
				liJournalSize = it->second.m_liJournalTotalSize;
				break;
			}
			nItem++;
		}

		CString strSize;
		strSize.Format("%I64d Bytes", liJournalSize);

		*pVal = CComBSTR(strSize).Copy();

		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_JournalDrive(long Index, BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if ((pVal != NULL) && ((Index >= 0) && ((UINT)Index < m_pServer->m_mapJournalDrive.size())))
	{
		CComBSTR bstrDrive;

		long nItem = 0;
		for(std::map<std::string, CJournalDrive>::iterator it = m_pServer->m_mapJournalDrive.begin();
			it != m_pServer->m_mapJournalDrive.end(); it++)
		{
			if (nItem == Index)
			{
				bstrDrive = it->first.c_str();
				break;
			}
			nItem++;
		}

		*pVal = bstrDrive.Copy();

		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_JournalDiskSize(long Index, BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if ((pVal != NULL) && ((Index >= 0) && ((UINT)Index < m_pServer->m_mapJournalDrive.size())))
	{
		__int64 liJournalSize = 0;

		long nItem = 0;
		for(std::map<std::string, CJournalDrive>::iterator it = m_pServer->m_mapJournalDrive.begin();
			it != m_pServer->m_mapJournalDrive.end(); it++)
		{
			if (nItem == Index)
			{
				liJournalSize = it->second.m_liDiskTotalSize;
				break;
			}
			nItem++;
		}

		CString strSize;
		strSize.Format("%I64d Bytes", liJournalSize);

		*pVal = CComBSTR(strSize).Copy();

		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::get_JournalDiskFreeSize(long Index, BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())


	HRESULT hr = E_INVALIDARG;

	if ((pVal != NULL) && ((Index >= 0) && ((UINT)Index < m_pServer->m_mapJournalDrive.size())))
	{
		__int64 liJournalSize = 0;

		long nItem = 0;
		for(std::map<std::string, CJournalDrive>::iterator it = m_pServer->m_mapJournalDrive.begin();
			it != m_pServer->m_mapJournalDrive.end(); it++)
		{
			if (nItem == Index)
			{
				liJournalSize = it->second.m_liDiskFreeSize;
				break;
			}
			nItem++;
		}

		CString strSize;
		strSize.Format("%I64d Bytes", liJournalSize);

		*pVal = CComBSTR(strSize).Copy();

		hr = S_OK;
	}

	return hr;
}

//ardev 021129 v
STDMETHODIMP CComServer::GetEventAt ( long nIndex, IEvent** ppEvent )
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
		CEvent* pEvent = m_pServer->GetEventAt( nIndex );

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

} // CComServer::GetEventAt ()


STDMETHODIMP CComServer::GetEventCount ( long* pnRow )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if ( pnRow == NULL )
	{
		return E_FAIL;
	}

	*pnRow = m_pServer->GetEventCount();

	return S_OK;

} // CComServer::GetEventCount ()


STDMETHODIMP CComServer::IsEventAt ( long nIndex, BOOL *pbLoaded )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if ( pbLoaded == NULL )
	{
		return E_FAIL;
	}

	*pbLoaded = m_pServer->IsEventAt( nIndex );

	return S_OK;

} // CComServer::IsEventAt ()

//ardev 021129 v


STDMETHODIMP CComServer::Import(BSTR* Message, long* pErrCode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (pErrCode == NULL)
	{
		return E_FAIL;
	}

	std::string strErrMsg;
	*pErrCode = m_pServer->ImportReplGroup(strErrMsg);

	// return a copy of the returned msg
	if ((*pErrCode != 0) && (Message != NULL))
	{
		CComBSTR bstrMsg = strErrMsg.c_str();
		*Message = bstrMsg.Copy();
	}

	return S_OK;
}

STDMETHODIMP CComServer::CreateScriptServerFile(IScriptServerFile **ppScriptServerFile)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (ppScriptServerFile == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		// Create a new Script server File
		CScriptServer& SS = m_pServer->AddNewScriptServerFile();
	
		CComObject<CComScriptServerFile>* pComSS = NULL;
		hr = CComObject<CComScriptServerFile>::CreateInstance(&pComSS);
		if (SUCCEEDED(hr))
		{
            pComSS->m_pScriptServer = &SS;
			pComSS->AddRef();
			hr = pComSS->QueryInterface(ppScriptServerFile);
			pComSS->Release();
		}
	}

	return hr;
}

STDMETHODIMP CComServer::GetScriptServerFileCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->GetScriptServerFileCount();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::GetScriptServerFile(long nIndex, IScriptServerFile **ppScriptServerFile)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;


    if ((nIndex < 0) || (ppScriptServerFile == NULL) || ((UINT)nIndex >= m_pServer->m_listScriptServerFile.size()))
	{
		hr = E_INVALIDARG;
	}
	else
	{
		*ppScriptServerFile = NULL;

		std::list<CScriptServer>::iterator it;
		long nCount;
		for (it = m_pServer->m_listScriptServerFile.begin(), nCount = 0;
			 (nCount < nIndex) && (it != m_pServer->m_listScriptServerFile.end()); it++, nCount++);
		if (nCount == nIndex)
		{
			CComObject<CComScriptServerFile>* pComSS = NULL;
			hr = CComObject<CComScriptServerFile>::CreateInstance(&pComSS);
			if (SUCCEEDED(hr))
			{				
				CScriptServer& SS = *it;
                pComSS->m_pScriptServer = &SS;
				pComSS->AddRef();
				hr = pComSS->QueryInterface(ppScriptServerFile);
				pComSS->Release();
			}
		}
		else
		{
			hr = E_INVALIDARG;
		}
	}

	return hr;
}

STDMETHODIMP CComServer::RemoveScriptServerFile(long nScriptServerID,long *pRetVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr; 

    hr = m_pServer->RemoveScriptServerFile(nScriptServerID);

	return hr;
}

STDMETHODIMP CComServer::ImportAllScriptServerFiles(BOOL bOverwriteExistingFile, BSTR strExtension)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = m_pServer->ImportAllScriptServerFiles(bOverwriteExistingFile,OLE2A(strExtension));

	return hr;
}

STDMETHODIMP CComServer::ImportOneScriptServerFile(BSTR strFilename)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = m_pServer->ImportOneScriptServerFile(OLE2A(strFilename));

	return hr;
}

STDMETHODIMP CComServer::LockCmds()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->LockCmds();

	return S_OK;
}

STDMETHODIMP CComServer::UnlockCmds()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->UnlockCmds();

	return S_OK;
}

STDMETHODIMP CComServer::get_IsLockCmds(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_bLockCmds;
		hr = S_OK;
	}

	return hr;
}


STDMETHODIMP CComServer::get_NbrCPU(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_nNbrCPU;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::put_NbrCPU(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->m_nNbrCPU = newVal;

	return S_OK;
}

STDMETHODIMP CComServer::get_ReplicationPort(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if (pVal != NULL)
	{
		*pVal = m_pServer->m_nReplicationPort;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComServer::put_ReplicationPort(long newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pServer->m_nReplicationPort = newVal;

	return S_OK;
}
