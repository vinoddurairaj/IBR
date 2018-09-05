/****************************************************************
 * Name:  parse.c                                               *
 *                                                              *
 * Function:  Functions that process arguments                  *
 *            passed to data collector DLL.                     *
 *                                                              *
 ****************************************************************/

//
// All-inclusive header file
//
#include "perf.h"

//
// Data global to this module
//
// (None)

//
// Forward declarations of local routines
//
// (None)



//++
// Function:
//		PerfGetQueryType
//
// Description:
//		This function determines what type of
//		of data the system performance monitor
//		wants from the collection DLL. 
//
// Arguments:
//		Unicode string identifying the data type
//
// Return Value:
//		PERF_QUERY_TYPE_GLOBAL
//		PERF_QUERY_TYPE_FOREIGN
//		PERF_QUERY_TYPE_COSTLY
//		PERF_QUERY_TYPE_ITEMS
//--
DWORD
PerfGetQueryType(
    IN LPWSTR lpValue
	)
{
	//
	// NULL pointer or pointer to empty string
	// means Global
	//
    if (lpValue == 0)
	{
        return PERF_QUERY_TYPE_GLOBAL;
    }
	else if (*lpValue == 0)
	{
        return PERF_QUERY_TYPE_GLOBAL;
    }

	//
    // Check for "Global" request
	//
	if( wcsstr( lpValue, L"Global" ) != NULL )
		return PERF_QUERY_TYPE_GLOBAL;

	//
    // Check for "Foreign" request
	//
	if( wcsstr( lpValue, L"Foreign" ) != NULL )
		return PERF_QUERY_TYPE_FOREIGN;

	//
    // Check for "Costly" request
	//
	if( wcsstr( lpValue, L"Costly" ) != NULL )
		return PERF_QUERY_TYPE_COSTLY;

	//
    // if not Global, Foreign, or Costly, then 
    // it must be a list of object-types
	//    
    return PERF_QUERY_TYPE_ITEMS;

}


//++
// Function:
//		PerfIsNumberInList
//
// Description:
//		This function searches a NULL-terminated
//		Unicode list of decimal numbers for an
//		occurence of a specific integer value. 
//
// Arguments:
//		DWORD integer
//		White-space delimited Unicode list
//
// Return Value:
//		TRUE if the number is in the list
//		FALSE if it's not in the list
//--
BOOL
PerfIsNumberInList(
    IN DWORD dwRequestedNumber,
    IN LPWSTR lpwszList
	)
{
	DWORD dwListNumber;
	DWORD dwCount;
	DWORD dwLength;

	//
	// Null pointer means the number wasn't found
	//
    if (lpwszList == 0) return FALSE;

	//
	// Scan until found or end of list
	//
	while( wcslen( lpwszList ) > 0 )
	{
		dwCount = swscanf(
			lpwszList,
			L"%d%n",
			&dwListNumber,
			&dwLength );

		//
		// No translation means end of list
		//
		if( dwCount == 0 ) return FALSE;

		//
		// See if the translated number was the one
		//
		if( dwListNumber ==	dwRequestedNumber ) return TRUE;

		//
		// Not this one; maybe the next
		//
		lpwszList += dwLength;
	}
	return FALSE;
}

