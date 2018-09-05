/*****************************************************************************
 *                                                                           *
 *  This software  is the licensed software of Softek Software               *
 *  Technology Corporation                                                   *
 *                                                                           *
 *  Copyright (c) 2002 by Softek Software Technology Corporation             *
 *                                                                           *
 *  THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF            *
 *  Softek SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED              *
 *  UNDER LICENSE FROM Softek SOFTWARE TECHNOLOGY CORPORATION                *
 *                                                                           *
 *****************************************************************************

 *****************************************************************************
 *                                                                           *
 *  Revision History:                                                        *
 *                                                                           *
 *  Date          By              Change                                     *
 *  -----------   --------------  -----------------------------------------	 *
 *  27-July-2004 Parag				Added this new project for sftkblk.sys*
 *									installation/UnInstallation				 *
 *                                                                           *
 *****************************************************************************/

/*****************************************************************************
 *                                                                           *
 *                        NOTICE                                             *
 *                                                                           *
 *          COPYRIGHT (c) 1998-2001 SOFTEK SOFTWARE CORPORATION              *
 *                                                                           *
 *                    ALL RIGHTS RESERVED                                    *
 *                                                                           *
 *                                                                           *
 *    This Computer Program is CONFIDENTIAL and a TRADE SECRET of            *
 *    SOFTEK SOFTWARE CORPORATION. Any reproduction of this program			 *
 *    without the express written consent of SOFTEK SOFTWARE CORPORATION	 *
 *    is a violation of the copyright laws and may subject you to criminal   *
 *    prosecution.                                                           *
 *                                                                           *
 *****************************************************************************/

//
// Include files 
//
#include <windows.h>
#include <stdio.h>

#include <wtypes.h>
#include <windef.h>
#include <winnt.h>
#include <winsvc.h>
#include <winreg.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include <lmerr.h>			// for GetErrorText()

//
// Macro Definations
//
#define DISK_FILTER_KEY		_T("System\\CurrentControlSet\\Control\\Class\\{4D36E967-E325-11CE-BFC1-08002BE10318}")
// #define SCSI_CONTROLLER_FILTER_KEY		_T("System\\CurrentControlSet\\Control\\Class\\{4D36E97B-E325-11CE-BFC1-08002BE10318}")
#define VOLUME_FILTER_KEY	_T("System\\CurrentControlSet\\Control\\Class\\{71A27CDD-812A-11D0-BEC7-08002BE2092F}")

//#define FSTAPPNP	"FSTApPnp"
#define SFTK_BLOCK_PNP		"SftkBlk"
#define KEY_UPPER_FILTERS	_T("UpperFilters")

//
// API Pre-Declarations
//
VOID	display_usage();
LONG	Alloc_Registry(TCHAR * pPath, HKEY *pkeyHandle);
LONG	Free_Registry(HKEY keyHandle);
LONG	Get_Registry_Value(HKEY keyHandle,  TCHAR * Key,  ULONG *type, LPBYTE Data, ULONG *size);
LONG	Set_Registry_Value(HKEY keyHandle,  TCHAR * Key,  ULONG type, TCHAR * Data, ULONG *size);
LONG	Delete_Registry_Value(HKEY keyHandle,  TCHAR * Key);
void	main(int argc, char* argv[]);
VOID	GetErrorText();


VOID	
display_usage()
{
	printf("usage: AddFilter [-install | -uninstall ]\n");
    exit ( 1 );
} // display_usage()

LONG
Alloc_Registry(TCHAR * pPath, HKEY *pkeyHandle)
{
	ULONG keyDisposition;

	return(RegCreateKeyEx(HKEY_LOCAL_MACHINE, pPath, 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
		pkeyHandle, &keyDisposition));
} // Alloc_Registry()

LONG 
Free_Registry(HKEY keyHandle) 
{
	if(keyHandle) RegCloseKey(keyHandle);
	return(0);
}


LONG
Get_Registry_Value(HKEY keyHandle,  TCHAR * Key,  ULONG *type, LPBYTE Data, ULONG *size)
{
	if ( Data )
	 memset((PVOID)Data, 0, *size);
	return( RegQueryValueEx(keyHandle, Key, NULL, type, Data, size ) );
}


LONG 
Set_Registry_Value(HKEY keyHandle,  TCHAR * Key,  ULONG type, TCHAR * Data, ULONG *size)
{
	return(RegSetValueEx(keyHandle, Key, 0, type, ((const PUCHAR)Data), (DWORD) *size)); 
}

LONG 
Delete_Registry_Value(HKEY keyHandle,  TCHAR * Key)
{
	return(RegDeleteValue(keyHandle, Key)); 
}

//
// Main
//
void 
main(int argc, char* argv[])
{
	DWORD	dwError		= 0;
	CHAR	*pszStr		= NULL;
	CHAR	*List		= NULL;
	CHAR	*newList	= NULL;
	HKEY	regHandle	= NULL;
	ULONG	BufSize		= 0;
	ULONG	length		= 0;
	ULONG	TotalLength = 0;
	ULONG	keytype;
	CHAR	*pOldStr, *pEndStr, *pNewStr;
	LONG	i, ntStatus;

	// Initialize Registry pointer
	if ( ! lstrcmpi( argv[1], "-install" ) )
	{
		for (i=0; i < 2; i++)
		{
			// Reset the Total Length Or Else we will endup with a bad UpperFilter Settings for the UpperFilter

			TotalLength = 0;
			if (i == 0)
			{
				// Open the Registry key
				ntStatus = Alloc_Registry(DISK_FILTER_KEY, &regHandle);
			}
			else
			{
				ntStatus = Alloc_Registry(VOLUME_FILTER_KEY, &regHandle);
			}

			if (  ntStatus !=ERROR_SUCCESS ) {

				printf("AddFilter:  RegCreateKeyEx('System\\CurrentControlSet\\Control\\Class\\{4D36E97B-E325-11CE-BFC1-08002BE10318}') Failed !!\n");
				GetErrorText();
				goto done;
			}

			// Check if UpperFilters Key vaLUE EXIST if not than create it else add our driver name 
			// to it
			ntStatus = Get_Registry_Value(regHandle, KEY_UPPER_FILTERS, &keytype, NULL, &BufSize);	
			if ( ( ntStatus != ERROR_SUCCESS ) || (BufSize < 2) ) {
				// create this key value in registry and add our driver name in this value

				// Allocate New List memory which will be added to this Key value list 
				newList = (CHAR *)malloc((strlen(SFTK_BLOCK_PNP) + 2));
				if(newList == NULL) {

					printf("AddFilter:  malloc(size = %d) Failed !!\n",(strlen(SFTK_BLOCK_PNP) + 1));
					GetErrorText();

					if(regHandle) Free_Registry(regHandle);
					free( List );
					goto done;
				}
				memset( newList, 0, (strlen(SFTK_BLOCK_PNP) + 2) );
				memcpy( newList, SFTK_BLOCK_PNP, (strlen(SFTK_BLOCK_PNP)+1 ) );

				TotalLength = strlen( SFTK_BLOCK_PNP ) + 2;

				// Set New List to registry key
				keytype = REG_MULTI_SZ;
				ntStatus = Set_Registry_Value(regHandle,  KEY_UPPER_FILTERS,  keytype, newList, &TotalLength);

				if (  ntStatus !=ERROR_SUCCESS ) {
					printf("AddFilter:  RegSetValueEx('UpperFilters') Failed !!\n");
					GetErrorText();

					if(regHandle) Free_Registry(regHandle);
					free ( newList );
					goto done;
				}
				
				if(regHandle) Free_Registry(regHandle);
				free ( newList );

				continue; // for next one
			}

			// Allocate old Existing Key value list memory
			List = (CHAR *)malloc(BufSize);
			if(List == NULL) {

				printf("AddFilter:  malloc(size = %d) Failed !!\n",BufSize);
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				goto done;
			}

			memset( List, 0, BufSize );

			// Allocate New List memory which will be added to this Key value list 
			newList = (CHAR *)malloc((BufSize + strlen(SFTK_BLOCK_PNP) + 1));
			
			if(newList == NULL) {
				printf("AddFilter:  malloc(size = %d) Failed !!\n",(BufSize + strlen(SFTK_BLOCK_PNP) + 1) );
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				free( List );
				goto done;
			}

			memset( newList, 0, (BufSize + strlen(SFTK_BLOCK_PNP) + 1) );

			//Now get the Reg value data in buffer	
			ntStatus = Get_Registry_Value(regHandle, KEY_UPPER_FILTERS, &keytype, (LPBYTE)List, &BufSize);
			if (  ntStatus!=ERROR_SUCCESS  ) {

				printf("AddFilter: RegQueryValueEx('UpperFilters') Failed !!\n");
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				free ( List );
				free ( newList );
				goto done;
			}
			
			// Prepare new List which we want to set
			// Scan thru old list and copy one by one to new list along witg addition of our driver name in new list

			for(pOldStr = List, pNewStr = newList; pOldStr < (List + BufSize); ) {	//iterrate through the entire list

				for(pEndStr = pOldStr; *pEndStr != '\0'; pEndStr++)						//find the end of each string
					;
				
				if( (!lstrcmpi( pOldStr, SFTK_BLOCK_PNP) ) ) 
				{ // we found our driver name so skip this and continue for next driver name
					pOldStr = pEndStr + 1;	
					continue;
				}
				
				// Copy other driver name to new list
				length = (pEndStr - pOldStr) + 1;									//record its lenght
				
				if(length==1 && (!lstrcmpi( pOldStr, "")))
				{
					memcpy( pNewStr, SFTK_BLOCK_PNP, (strlen(SFTK_BLOCK_PNP)+1 ) );
					pNewStr = pNewStr + (strlen(SFTK_BLOCK_PNP) + 1);
					TotalLength = TotalLength + strlen( SFTK_BLOCK_PNP ) + 1;
				}
				
				TotalLength = TotalLength + length;									//total valid lenght of new list
				memcpy( pNewStr, pOldStr, length );								//copy into new string
				pNewStr = pNewStr + length ;

				pOldStr = pEndStr + 1;	
			}

			// if ( (bFoundOurDriver == TRUE) && (bAddedOurDriver == FALSE) )
				
			pszStr = (CHAR *)malloc( TotalLength);
			if ( pszStr == NULL) {

				printf("AddFilter:  Final Buffer allocation using malloc(size = %d) Failed !!\n",TotalLength);
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				free ( List );
				free ( newList );
				goto done;
			}

			memset( pszStr, 0, TotalLength );

			memcpy( pszStr, newList, TotalLength );

			// Set New List to registry key
			ntStatus = Set_Registry_Value(regHandle,  KEY_UPPER_FILTERS,  keytype, pszStr, &TotalLength);
			if (  ntStatus !=ERROR_SUCCESS ) {

				printf("AddFilter: Adding our driver in existing Driver List using RegSetValueEx('UpperFilters') Failed !!\n");
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				free ( pszStr );
				free ( List );
				free ( newList );
				goto done;
			}

			if(regHandle) 
				Free_Registry(regHandle);

			regHandle = NULL;

			if (pszStr)
				free ( pszStr );

			if(List)
				free ( List );

			if (newList)
				free ( newList );

			pszStr = NULL;
			List = NULL;
			newList = NULL;

		} // for (i=0; i < 2; i++)

		exit ( 0 ) ;	// success
	}  // if ( ! lstrcmpi( argv[1], "-install" ) )
	
	if ( ! lstrcmpi( argv[1], "-uninstall" ) )
	{
done:
		for (i=0; i < 2; i++)
		{
			if (i == 0)
			{
				ntStatus = Alloc_Registry(DISK_FILTER_KEY, &regHandle);
			}
			else
			{
				ntStatus = Alloc_Registry(VOLUME_FILTER_KEY, &regHandle);
			}

			if (  ntStatus !=ERROR_SUCCESS ) {

				printf("AddFilter:  RegCreateKeyEx('System\\CurrentControlSet\\Control\\Class\\{4D36E97B-E325-11CE-BFC1-08002BE10318}') Failed !!\n");
				GetErrorText();
				continue;
			}

			ntStatus = Get_Registry_Value(regHandle, KEY_UPPER_FILTERS, &keytype, NULL, &BufSize);	
			if (  ntStatus !=ERROR_SUCCESS ) {
				
				printf("AddFilter: RegQueryValueEx('UpperFilters') Failed !!\n");
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				continue;
			}

			List = (CHAR *)malloc( BufSize);
			if(List == NULL) {

				printf("AddFilter:  malloc(size = %d) Failed !!\n",BufSize);
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				continue;
			}

			memset( List, 0, BufSize );

			newList = (CHAR *)malloc(BufSize);
			if(newList == NULL) {
				
				printf("AddFilter:  malloc(size = %d) Failed !!\n",BufSize);
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				free ( List );
				continue;
			}

			memset( newList, 0, BufSize);

			//Now get the Reg value data	
			ntStatus = Get_Registry_Value(regHandle, KEY_UPPER_FILTERS, &keytype, (LPBYTE)List, &BufSize);
			if ( ntStatus !=ERROR_SUCCESS ) {
				
				printf("AddFilter: RegQueryValueEx('UpperFilters') Failed !!\n");
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				free ( List );
				free ( newList );
				continue;
			}

			TotalLength=0;

			for(pOldStr = List, pNewStr = newList; pOldStr < (List + BufSize); ) {	//iterrate through the entire list

				for(pEndStr = pOldStr; *pEndStr != '\0'; pEndStr++)						//find the end of each string
					;

				//
				// if the entry is one we install remove it
				//
				if(  (lstrcmpi( pOldStr, SFTK_BLOCK_PNP) ) != 0 ) {
					memcpy( pNewStr, pOldStr, (strlen(pOldStr)+1 ) );
						pNewStr = pNewStr + (strlen(pOldStr) + 1);
						TotalLength = TotalLength + strlen( pOldStr ) + 1;
				}

				pOldStr = pEndStr + 1;												//move to the next string the the original list

			} //end for

			pszStr = (CHAR *)malloc(TotalLength);
			if ( pszStr == NULL) {
				
				printf("AddFilter:  Final Buffer malloc(size = %d) Failed !!\n",TotalLength);
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				free ( List );
				free ( newList );
				continue;
			}

			memset( pszStr, 0, TotalLength );

			memcpy( pszStr, newList, TotalLength );

			ntStatus = Set_Registry_Value(regHandle,  KEY_UPPER_FILTERS,  keytype, pszStr, &TotalLength);

			if ( ntStatus!=ERROR_SUCCESS  ) {

				printf("AddFilter:  UnInstallaing using RegSetValueEx('UpperFilters') Failed !!\n");
				GetErrorText();

				if(regHandle) Free_Registry(regHandle);
				free ( pszStr );
				free ( List );
				free ( newList );
				continue;
			}

			if(TotalLength<2)		{
				Delete_Registry_Value(regHandle,  KEY_UPPER_FILTERS);
			}

			if(regHandle) Free_Registry(regHandle);

			free ( pszStr );
			free ( List );
			free ( newList );
		} // for (i=0; i < 2; i++)

		exit ( 0 ) ; // success
	}   // if ( ! lstrcmpi( argv[1], "-uninstall" ) )
	
	display_usage();
	exit ( 0 ) ;
} // main()
			
// internal function used to convert SDK API GetLastError() into string format and returns error text string to caller
VOID	GetErrorText()
{
    HMODULE hModule		= NULL; // default to system source
	DWORD	dwLastError	= GetLastError();
	DWORD	dwFormatFlags;
    LPSTR	MessageBuffer;
    DWORD	dwBufferLength;
	
	// display error details
	printf("\tError Detailed Information: \n");	
	printf("\tGetLastError() = %d (0x%08x) \n", dwLastError, dwLastError);	
	
	
    dwFormatFlags =	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_IGNORE_INSERTS |
					FORMAT_MESSAGE_FROM_SYSTEM ;

    // If dwLastError is in the network range, 
    //  load the message source.

    if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) {
        hModule = LoadLibraryEx(
            TEXT("netmsg.dll"),
            NULL,
            LOAD_LIBRARY_AS_DATAFILE
            );

        if(hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    // Call FormatMessage() to allow for message 
    //  text to be acquired from the system 
    //  or from the supplied module handle.

    if(dwBufferLength = FormatMessageA(
        dwFormatFlags,
        hModule, // module to get message from (NULL == system)
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
        (LPSTR) &MessageBuffer,
        0,
        NULL
        ))
    {
        // Output message string on stderr.
		printf("\tErrorString    = %s \n",MessageBuffer);	

        // Free the buffer allocated by the system.
        LocalFree(MessageBuffer);
    } 
	else
	{
		printf("\tErrorString    = Unknown Error \n");	
	}

    // If we loaded a message source, unload it.
    if(hModule != NULL)
        FreeLibrary(hModule);

} // GetErrorText()