// ComSystem.cpp : Implementation of CComSystem
#include "stdafx.h"
#include "TDMFObjects.h"
#include "ComSystem.h"
#include "ComDomain.h"
#include "ComEvent.h"
#include "ComCollectorStats.h"
#include "mmp_API.h"


/////////////////////////////////////////////////////////////////////////////
// CComSystem

STDMETHODIMP CComSystem::get_Name(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pSystem->m_strName.c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::put_Name(BSTR Val)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	m_pSystem->m_strName = OLE2A(Val);

	return S_OK;
}

STDMETHODIMP CComSystem::get_DomainCount(long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (pVal != NULL)
	{
		*pVal = m_pSystem->m_listDomain.size();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetDomain(long nIndex, IDomain** ppDomain)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	*ppDomain = NULL;
	if ((nIndex < 0) || (ppDomain == NULL) || ((UINT)nIndex >= m_pSystem->m_listDomain.size()))
	{
		hr = E_INVALIDARG;
	}
	else
	{
		std::list<CDomain>::iterator it;
		long nCount;
		for (it = m_pSystem->m_listDomain.begin(), nCount = 0;
			 (nCount < nIndex) && (it != m_pSystem->m_listDomain.end()); it++, nCount++);
		if (nCount == nIndex)
		{
			CComObject<CComDomain>* pComDomain = NULL;
			hr = CComObject<CComDomain>::CreateInstance(&pComDomain);
			if (SUCCEEDED(hr))
			{
				CDomain& Domain = *it;
				pComDomain->m_pDomain = &Domain;
				pComDomain->AddRef();
				hr = pComDomain->QueryInterface(ppDomain);
				pComDomain->Release();
			}
		}
		else
		{
			hr = E_INVALIDARG;
		}
	}

	return hr;
}

STDMETHODIMP CComSystem::CreateNewDomain(IDomain **ppNewDomain)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (ppNewDomain == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		// Create a new replication group
		CDomain& Domain = m_pSystem->AddNewDomain();

		CComObject<CComDomain>* pComDomain = NULL;
		hr = CComObject<CComDomain>::CreateInstance(&pComDomain);
		if (SUCCEEDED(hr))
		{
			pComDomain->m_pDomain = &Domain;
			pComDomain->AddRef();
			hr = pComDomain->QueryInterface(ppNewDomain);
			pComDomain->Release();
		}
	}

	return hr;
}

STDMETHODIMP CComSystem::RemoveDomain(IDomain *pDomain, long* pRetCode)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pRetCode = MMPAPI_Error::OK;

	long nKey;
	if (SUCCEEDED(pDomain->get_Key(&nKey)))
	{
		std::list<CDomain>::iterator it = m_pSystem->m_listDomain.begin();
		while(it != m_pSystem->m_listDomain.end())
		{
			if (it->GetKey() == nKey)
			{
				if (it->m_iDbK > 0)
				{
					// Remove from DB
					*pRetCode = it->RemoveFromDB();
				}
				
				if (*pRetCode == MMPAPI_Error::OK)
				{
					m_pSystem->m_listDomain.erase(it);
				}

				break;
			}
			it++;
		}
	}

	return S_OK;
}

STDMETHODIMP CComSystem::Refresh(long* pErr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// save critical values
	HWND hWnd = m_pSystem->m_hWnd;
	unsigned short nTraceLevel = m_pSystem->m_nTraceLevel;
	CComBSTR bstrUser = m_pSystem->m_bstrUser;
	CComBSTR bstrPwd  = m_pSystem->m_bstrPwd;
	std::string strName = m_pSystem->m_strName;
	FsTdmfDbUserInfo::TdmfRoles eTdmfRole = m_pSystem->m_eTdmfRole;

	Uninitialize();

	// Reload data
	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}
	// Restore critical values
	m_pSystem->m_bstrUser = bstrUser;
	m_pSystem->m_bstrPwd  = bstrPwd;
	m_pSystem->m_strName  = strName;
	m_pSystem->m_eTdmfRole= eTdmfRole;

	long nErr;
	Init((long)hWnd, &nErr);

	// Reset trace level.  We lost traces that could have been generated in the init() func.
	m_pSystem->m_nTraceLevel = nTraceLevel;

	if (nErr == 0)
	{
		Open(CComBSTR(""), &nErr);
	}

	*pErr = nErr;

	return S_OK;
}

STDMETHODIMP CComSystem::Init(long hWnd, long* pErr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	*pErr = m_pSystem->Initialize((HWND)hWnd);

	return S_OK;
}

STDMETHODIMP CComSystem::Open(BSTR bstrDSN, long* pErr)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pErr = m_pSystem->Open(OLE2A(bstrDSN));

	return S_OK;
}

STDMETHODIMP CComSystem::Uninitialize()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (m_pSystem != NULL)
	{
		m_pSystem->Uninitialize();

		delete m_pSystem;
		m_pSystem = NULL;
	}

	return S_OK;
}

STDMETHODIMP CComSystem::GetDescription(BSTR *pDatabase, BSTR *pVersion, BSTR *pIP, BSTR *pPort, BSTR *pHostId)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_INVALIDARG;

	if ((pDatabase != NULL) && (pVersion != NULL) && (pIP != NULL) && (pPort != NULL) && (pHostId!= NULL))
	{
		*pDatabase = CComBSTR(m_pSystem->m_strDatabaseServer.c_str()).Copy();
		*pVersion  = CComBSTR(m_pSystem->m_strCollectorVersion.c_str()).Copy();
		*pIP       = CComBSTR(m_pSystem->m_strCollectorIP.c_str()).Copy();
		*pPort     = CComBSTR(m_pSystem->m_strCollectorPort.c_str()).Copy();
		*pHostId   = CComBSTR(m_pSystem->m_strCollectorHostId.c_str()).Copy();

		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetEventFirst(IEvent **ppEvent)
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
		CEvent* pEvent = m_pSystem->GetEventFirst();

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

STDMETHODIMP CComSystem::GetLastCommandOutput(BSTR *pCmdOutput)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (pCmdOutput != NULL)
	{
		*pCmdOutput = A2BSTR(m_pSystem->GetMMP()->getTdmfCmdOutput().c_str());

		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::RequestOwnership(BOOL bRequest)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (m_pSystem != NULL)
	{
		long nRetCode = m_pSystem->RequestOwnership(bRequest);
		switch (nRetCode)
		{
		case 0:
			hr = S_FALSE;
			break;
	case 1:
			hr = S_OK;
			break;
		}
	}
	else
	{
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetTableSize(TdmfDBTable Table, long* pCount, BSTR *pVal)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (pVal != NULL)
	{
		*pVal = A2BSTR(m_pSystem->GetTableSize(Table, pCount).c_str());

		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::DeleteTableRecords(TdmfDBTable Table)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (m_pSystem->DeleteTableRecords(Table))
	{
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetDeleteRecords(TdmfDBTable Table, long *pDays, long *pNbRecords)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pSystem->GetDeleteRecords(Table, pDays, pNbRecords);

	return S_OK;
}

STDMETHODIMP CComSystem::SetDeleteRecords(TdmfDBTable Table, long days, long NbRecords)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (m_pSystem->SetDeleteRecords(Table, days, NbRecords))
	{
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetNbCommandMenuEntries(BSTR Key, long* Nb)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (Nb != NULL)
	{
		*Nb = m_pSystem->GetNbCommandMenuEntries(OLE2A(Key));
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetCommandMenuEntryName(BSTR Key, long Index, BSTR *Name)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (Name != NULL)
	{
		*Name = CComBSTR(m_pSystem->GetCommandMenuEntryName(OLE2A(Key), Index).c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetCommandMenuEntryId(BSTR Key, long Index, long *Id)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (Id != NULL)
	{
		*Id = m_pSystem->GetCommandMenuEntryId(OLE2A(Key), Index);
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::IsCommandMenuEntryEnabled(BSTR Key, long Index, long Mode, long ConnectionStatus, long Platform, BOOL *Enabled)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (Enabled != NULL)
	{
		*Enabled = m_pSystem->IsCommandMenuEntryEnabled(OLE2A(Key), Index, Mode, ConnectionStatus, Platform);
		hr = S_OK;
	}

	return hr;
}


STDMETHODIMP CComSystem::GetCommandMenuString(BSTR Key, long Id, BSTR *Val)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (Val != NULL)
	{
		*Val = CComBSTR(m_pSystem->GetCommandMenuString(OLE2A(Key), Id).c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

// ardev 021128

STDMETHODIMP CComSystem::GetEventAt ( long nIndex, IEvent** ppEvent )
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
		CEvent* pEvent = m_pSystem->GetEventAt( nIndex );

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

} // CComSystem::GetEventAt ()


STDMETHODIMP CComSystem::GetEventCount ( long* pnRow )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if ( pnRow == NULL )
	{
		return E_FAIL;
	}

	*pnRow = m_pSystem->GetEventCount();

	return S_OK;

} // CComSystem::GetEventCount ()


STDMETHODIMP CComSystem::IsEventAt (long nIndex, BOOL *pbLoaded )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if ( pbLoaded == NULL )
	{
		return E_FAIL;
	}

	*pbLoaded = m_pSystem->IsEventAt( nIndex );

	return S_OK;

} // CComSystem::IsEventAt ()
// ardev 021128


STDMETHODIMP CComSystem::get_TraceLevel(short *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	if (pVal != NULL)
	{
		*pVal = m_pSystem->m_nTraceLevel;
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::put_TraceLevel(short newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	m_pSystem->m_nTraceLevel = newVal;

	return S_OK;
}

STDMETHODIMP CComSystem::get_UserID(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	if (pVal != NULL)
	{
		*pVal = m_pSystem->m_bstrUser.Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::put_UserID(BSTR newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	m_pSystem->m_bstrUser  = newVal;
	m_pSystem->m_eTdmfRole = FsTdmfDbUserInfo::TdmfRoleUndef;

	return S_OK;
}

STDMETHODIMP CComSystem::get_Password(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	if (pVal != NULL)
	{
		*pVal = m_pSystem->m_bstrPwd.Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::put_Password(BSTR newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	m_pSystem->m_bstrPwd = newVal;

	return S_OK;
}

STDMETHODIMP CComSystem::get_UserRole(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (pVal != NULL)
	{
		*pVal = CComBSTR(m_pSystem->GetUserRole().c_str()).Copy();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetFirstUser(BSTR *Location, BSTR *UserName, BSTR* Type, BSTR *App)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	std::string strLocation;
	std::string strUserName;
	std::string strType;
	std::string strRole;

	m_pSystem->GetFirstUser(strLocation, strUserName, strType, strRole);

	*Location = CComBSTR(strLocation.c_str()).Copy();
	*UserName = CComBSTR(strUserName.c_str()).Copy();
	*Type     = CComBSTR(strType.c_str()).Copy();
	*App      = CComBSTR(strRole.c_str()).Copy();

	return S_OK;
}

STDMETHODIMP CComSystem::GetNextUser(BSTR *Location, BSTR *UserName, BSTR* Type, BSTR *App)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	std::string strLocation;
	std::string strUserName;
	std::string strType;
	std::string strRole;

	m_pSystem->GetNextUser(strLocation, strUserName, strType, strRole);

	*Location = CComBSTR(strLocation.c_str()).Copy();
	*UserName = CComBSTR(strUserName.c_str()).Copy();
	*Type     = CComBSTR(strType.c_str()).Copy();
	*App      = CComBSTR(strRole.c_str()).Copy();

	return S_OK;
}

STDMETHODIMP CComSystem::AlreadyExistDomain(BSTR Name, long lKey, BOOL *pRet)
{
    USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

    if(pRet != NULL)
    {
        *pRet = m_pSystem->AlreadyExistDomain(OLE2A(Name), lKey);
    }
	return S_OK;
}

STDMETHODIMP CComSystem::SendTextMessage(BSTR Msg)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pSystem->GetMMP()->SendTdmfTextMessage(OLE2A(Msg));

	return S_OK;
}



STDMETHODIMP CComSystem::GetCollectorStats(ICollectorStats** pICollectorStats)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (pICollectorStats == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		// Create a new COM CollectorStats
		mmp_TdmfCollectorState* pTdmfCollectorState = m_pSystem->GetTdmfCollectorState();

		if (pTdmfCollectorState != NULL)
		{
			CComObject<CComCollectorStats>* pComCollectorStats = NULL;
			hr = CComObject<CComCollectorStats>::CreateInstance(&pComCollectorStats);
			if (SUCCEEDED(hr))
			{
				pComCollectorStats->m_pTdmfCollectorState = pTdmfCollectorState;
				pComCollectorStats->AddRef();
				hr = pComCollectorStats->QueryInterface(pICollectorStats);
				pComCollectorStats->Release();
			}
		}
		else // End of list
		{
			*pICollectorStats = NULL;
			hr = S_OK;
		}
	}

	return hr;
	
}

STDMETHODIMP CComSystem::SetCollectorStats(ICollectorStats *pICollectorStats)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = E_INVALIDARG;

	if(pICollectorStats != NULL)
    {
        if (m_pSystem == NULL)
	    {
		    m_pSystem = new CSystem;
	    }

	    m_pSystem->SetTdmfCollectorState((mmp_TdmfCollectorState*)pICollectorStats);
        hr = S_OK;
    }

	return hr;
}



STDMETHODIMP CComSystem::get_LogUsersActions(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	if (pVal != NULL)
	{
		*pVal = m_pSystem->LogUsersActions();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::put_LogUsersActions(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pSystem->LogUsersActions(newVal);

	return S_OK;
}

STDMETHODIMP CComSystem::GetFirstLogMsg(BSTR *Date, BSTR *Source, BSTR* User, BSTR *Msg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	std::string strDate;
	std::string strSource;
	std::string strUser;
	std::string strMsg;

	m_pSystem->GetFirstLogMsg(strDate, strSource, strUser, strMsg);

	*Date   = CComBSTR(strDate.c_str()).Copy();
	*Source = CComBSTR(strSource.c_str()).Copy();
	*User   = CComBSTR(strUser.c_str()).Copy();
	*Msg    = CComBSTR(strMsg.c_str()).Copy();

	return S_OK;
}

STDMETHODIMP CComSystem::GetNextLogMsg(BSTR *Date, BSTR *Source, BSTR* User, BSTR *Msg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	std::string strDate;
	std::string strSource;
	std::string strUser;
	std::string strMsg;

	m_pSystem->GetNextLogMsg(strDate, strSource, strUser, strMsg);

	*Date   = CComBSTR(strDate.c_str()).Copy();
	*Source = CComBSTR(strSource.c_str()).Copy();
	*User   = CComBSTR(strUser.c_str()).Copy();
	*Msg    = CComBSTR(strMsg.c_str()).Copy();

	return S_OK;
}

STDMETHODIMP CComSystem::DeleteAllLogMsg(BOOL* pbRet)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (pbRet != NULL)
	{
		*pbRet = m_pSystem->DeleteAllLogMsg();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetFirstKeyLogMsg(BSTR *Date, BSTR *Hostname, long *HostId, BSTR* RegKey, BSTR *ExpDate)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	std::string strDate;
	std::string strHostname;
	long        nHostId;
	std::string strRegKey;
	std::string strExpDate;

	m_pSystem->GetFirstKeyLogMsg(strDate, strHostname, &nHostId, strRegKey, strExpDate);

	*Date     = CComBSTR(strDate.c_str()).Copy();
	*Hostname = CComBSTR(strHostname.c_str()).Copy();
	*HostId   = nHostId;
	*RegKey   = CComBSTR(strRegKey.c_str()).Copy();
	*ExpDate  = CComBSTR(strExpDate.c_str()).Copy();

	return S_OK;
}

STDMETHODIMP CComSystem::GetNextKeyLogMsg(BSTR *Date, BSTR *Hostname, long *HostId, BSTR* RegKey, BSTR *ExpDate)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	std::string strDate;
	std::string strHostname;
	long        nHostId;
	std::string strRegKey;
	std::string strExpDate;

	m_pSystem->GetNextKeyLogMsg(strDate, strHostname, &nHostId, strRegKey, strExpDate);

	*Date     = CComBSTR(strDate.c_str()).Copy();
	*Hostname = CComBSTR(strHostname.c_str()).Copy();
	*HostId   = nHostId;
	*RegKey   = CComBSTR(strRegKey.c_str()).Copy();
	*ExpDate  = CComBSTR(strExpDate.c_str()).Copy();

	return S_OK;
}

STDMETHODIMP CComSystem::SetDeleteDelay(long value)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (m_pSystem->SetDeleteDelay(value))
	{
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetDeleteDelay(long* pValue)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pValue = m_pSystem->GetDeleteDelay();

	return S_OK;
}

STDMETHODIMP CComSystem::GetUserCount(long *pCount)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*pCount = m_pSystem->GetUserCount();

	return S_OK;
}



STDMETHODIMP CComSystem::SetTimeOutToDB()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

 
	HRESULT hr = E_FAIL;

	if (m_pSystem->SetTimeOutToDB() == 0)
	{
		hr = S_OK;
	}

	return hr;

}

STDMETHODIMP CComSystem::GetTimeOutFromDB()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = E_FAIL;
	
  	if (m_pSystem->GetTimeOutFromDB() == 0)
	{
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CComSystem::GetTimeOut(int nIndex, long* pValue)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = E_FAIL;

    if (*pValue = m_pSystem->GetTimeOut(nIndex) == 0)
	{
		hr = S_OK;
	}

	return S_OK;
}

STDMETHODIMP CComSystem::SetTimeOut(int nIndex, int nValue)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	m_pSystem->SetTimeOut(nIndex,nValue);

	return S_OK;
}

STDMETHODIMP CComSystem::get_IsLockCmds(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = E_FAIL;

	if (m_pSystem == NULL)
	{
		m_pSystem = new CSystem;
	}

	if (pVal != NULL)
	{
		*pVal = (m_pSystem->m_nNbLockCmds != 0);
		hr = S_OK;
	}

	return hr;
}
