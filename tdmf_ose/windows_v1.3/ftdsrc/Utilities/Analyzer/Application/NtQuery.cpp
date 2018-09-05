
#include "stdafx.h"

#include "ntquery.h"

NtSystemInformation::NtSystemInformation(SYSTEMINFOCLASS sic)
{
	m_sic = sic;
	m_pData = NULL;
}

NtSystemInformation::~NtSystemInformation()
{
	if (!m_pData) LocalFree(m_pData);
}
	
BOOL NtSystemInformation::Refresh()
{
	return m_Refresh();
}

BOOL NtSystemInformation::m_Refresh()
{
	if (!m_pData) {
		LocalFree(m_pData);
		m_pData = NULL;
	}

	DWORD ns = -1;
	DWORD dwSize, dwRet;

	// Query now.
	dwSize = 0x10000;
	dwRet = 0;
    while ((m_pData = LocalAlloc(LMEM_FIXED, dwSize)) != NULL)
    {
		ns = NtQuerySystemInformation(m_sic, m_pData, dwSize, &dwRet);
        if (ns != 0) dwRet = 0;
        if (ns != 0xC0000004L) break;

        LocalFree(m_pData);
        dwSize += 0x10000;
    }

	// Shrink returned memory block.
    dwSize = dwRet;
    if (m_pData)
    {
        dwRet = dwSize ? dwSize : 1;
        if ((ns == 0) && ((m_pData = LocalReAlloc(m_pData, dwRet, LMEM_MOVEABLE)) == NULL))
        {
            ns = -1;
        }
        if (ns != 0)
        {
            LocalFree(m_pData);
            m_pData = NULL;
        }
	}
    else
    {
		ns = -1;
	}
	if (ns != 0) return FALSE;
	return TRUE;
}


//
// System Handle Information
//
NtSystemHandleInformation::NtSystemHandleInformation()
: NtSystemInformation(SystemHandleInformation)
{
}
 
NtSystemHandleInformation::~NtSystemHandleInformation()
{
}

BOOL NtSystemHandleInformation::Refresh()
{
	// just pass up... 
	return NtSystemInformation::Refresh();
}

DWORD NtSystemHandleInformation::Count()
{
	if (!m_pData) Refresh();
	if (!m_pData) return 0;
	PSYSTEM_HANDLE_INFORMATION pInfo = (PSYSTEM_HANDLE_INFORMATION)m_pData;
	return pInfo->dCount;
}

PSYSTEM_HANDLE NtSystemHandleInformation::Get(int idx)
{
	if (!m_pData) Refresh();
	
    if (!m_pData) return NULL;
	
    PSYSTEM_HANDLE_INFORMATION pInfo = (PSYSTEM_HANDLE_INFORMATION)m_pData;

	if (idx < 0 || idx > pInfo->dCount) return NULL;

	return &pInfo->ash[idx];
}

//
// System Process Information
//
NtSystemProcessInformation::NtSystemProcessInformation()
: NtSystemInformation(SystemProcessInformation)
{
	m_dwCount = 0;
}
 
NtSystemProcessInformation::~NtSystemProcessInformation()
{
}

BOOL NtSystemProcessInformation::Refresh()
{
	BOOL bRet = NtSystemInformation::Refresh();
	if (!bRet) return bRet;

	// found; count now.
	PSYSTEM_PROCESS_INFORMATION pBase = (PSYSTEM_PROCESS_INFORMATION)m_pData;
	PSYSTEM_PROCESS_INFORMATION pInfo = pBase;
	PBYTE pb = (PBYTE)pBase;
	m_dwCount = 0;
	while (pInfo->dNext)
	{
		m_dwCount++;
		BYTE *pb = (BYTE*)pInfo + pInfo->dNext;
		pInfo = (PSYSTEM_PROCESS_INFORMATION)pb;

	}
	return bRet;
}

DWORD NtSystemProcessInformation::Count()
{
	if (!m_pData) Refresh();
	if (!m_pData) return 0;
	return m_dwCount;
}

PSYSTEM_PROCESS_INFORMATION NtSystemProcessInformation::Find(DWORD dwProcessId)
{
	if (!m_pData) Refresh();
	if (!m_pData) return NULL;

	// found; count now.
	PSYSTEM_PROCESS_INFORMATION pInfo = (PSYSTEM_PROCESS_INFORMATION)m_pData;
	PBYTE pb = (PBYTE)pInfo;
	while (pInfo->dNext)
	{
		if (pInfo->dUniqueProcessId == dwProcessId)
			return pInfo;

		pb += (pInfo->dNext);
		pInfo = (PSYSTEM_PROCESS_INFORMATION)pb;
	}
	return NULL;
}

// Holt den Namen eines Prozesses
// zurückgegebener Zeiger wird über LocalAlloc erzeugt
char* NtSystemProcessInformation::GetProcessName(DWORD dwProcessId)
{
	PSYSTEM_PROCESS_INFORMATION p = Find(dwProcessId);
	if (!p)
		return NULL;			// not found

	char* pStr = (char*)LocalAlloc(LMEM_FIXED, 1 + (p->usName.Length / 2));
	if (!pStr) return NULL;
	WORD *pUni = p->usName.Buffer;
	int s = 0;
	if (pUni)
		while (pUni[s])
			pStr[s] = (char)pUni[s++];
	pStr[s] = '\0';
	return pStr;
}



BOOL EnablePrivilege (HANDLE hprocess, LPSTR privilege, BOOL flag)
{
    HANDLE hToken = NULL;
    LUID DebugValue;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken (hprocess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
		goto errexit;

    if (!LookupPrivilegeValue((LPSTR) NULL, privilege, &DebugValue))
		goto errexit;

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = DebugValue;
    if (flag)
		tkp.Privileges[0].Attributes = 
			SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_USED_FOR_ACCESS;
	else
		tkp.Privileges[0].Attributes = NULL;

    AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL,
        (PDWORD) NULL);

    if (GetLastError() != ERROR_SUCCESS)
		goto errexit;

    CloseHandle (hToken);
	return TRUE;

errexit:
	CloseHandle (hToken);
	return FALSE;
}




