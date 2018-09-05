// TDMFInstall.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "TDMFInstall.h"

static std::vector<std::string> g_vecIPAddress;

static bool GetIPAddressList();


/*
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
*/



//extern "C" TDMFINSTALL_API 
//int GetIPFromComputerName(char* szComputerName, char* szIPAddress)
STDAPI GetIPFromComputerName(char* szComputerName, char* szIPAddress)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ULONG ip;

	if(sock_startup() == 0)
	{
		if(name_to_ip(szComputerName, &ip) != 0)
		{
			sock_cleanup();
			printf("\nError, call to 'name_to_ip' function caused an error.\n");
			return -1;
		}

		ip_to_ipstring(ip, szIPAddress);

		if(sock_cleanup() != 0)
		{
			printf("\nError, call to 'sock_cleanup' function caused an error.\n");
			return -1;
		}
	}
	else
	{
		printf("\nError, call to 'sock_startup' function caused an error.\n");
		return -1;
	}

	return 0;
}

//extern "C" TDMFINSTALL_API 
//int ValidateIPAddress(const char* szIPAddress)
STDAPI ValidateIPAddress(char* szIPAddress)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Check if Ip string is valid by
    //
    // check no alpha in name
    if (name_is_ipstring(szIPAddress))
    {
        // check all numbers 0<x<255
        unsigned long iptest;
        if (0==ipstring_to_ip(szIPAddress, &iptest))
        {
            return 0;
        }
    }
     
	return -1;
}

//extern "C" TDMFINSTALL_API 
//int GetIPCount()
STDAPI GetIPCount(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (GetIPAddressList())
	{
		return g_vecIPAddress.size();
	}

	return 0;
}

STDAPI GetIPAddressFromList(int nIndex, char *szIPAddress)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if ((nIndex >= 0) && (nIndex < GetIPCount()))
	{
		strcpy(szIPAddress, g_vecIPAddress[nIndex].c_str());

		return 0;	
	}

	return -1;
}

bool GetIPAddressList()
{
	if (g_vecIPAddress.empty())
	{
		if(sock_startup() == 0)
		{
			char szIPAddress[32];
			char szHostName[64];
			gethostname(szHostName, 64);
			
			// List physical IP address
			std::set<std::string> setPhysicalIPAddress;
			ULONG rgulIPList[32]; // Max 32 IP address
			
			getnetconfs(rgulIPList);
			for (int i = 0; i < getnetconfcount(); i++)
			{
				ip_to_ipstring(rgulIPList[i], szIPAddress);
				setPhysicalIPAddress.insert(szIPAddress);
			}
			
			// Get Host information (IP addresses)
			struct hostent* HostStructPtr = gethostbyname(szHostName);
			struct in_addr HostAddressStruct;
			
			if(!HostStructPtr)
			{
				printf("\nError, hostent is empty for %s.\n", szHostName);
				return false;
			}
			
			for (i = 0; HostStructPtr->h_addr_list[i]; i++)
			{
				HostAddressStruct.s_addr = *((u_long FAR *)(HostStructPtr->h_addr_list[i]));
				sprintf(szIPAddress, "%d.%d.%d.%d", HostAddressStruct.S_un.S_un_b.s_b1, HostAddressStruct.S_un.S_un_b.s_b2, HostAddressStruct.S_un.S_un_b.s_b3, HostAddressStruct.S_un.S_un_b.s_b4);
				if (setPhysicalIPAddress.find(szIPAddress) != setPhysicalIPAddress.end())
				{
					g_vecIPAddress.push_back(szIPAddress);
				}
			}

			sock_cleanup();
		}
		else
		{
			printf("\nError, call to 'sock_startup' function caused an error.\n");
			return false;
		}
	}

	return true;
}
