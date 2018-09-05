
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the TDMFINSTALL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// TDMFINSTALL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef TDMFINSTALL_EXPORTS
#define TDMFINSTALL_API __declspec(dllexport)
#else
#define TDMFINSTALL_API __declspec(dllimport)
#endif


/**
 * CreateTdmfDB
 * 
 * @return int  0 = success, 
 *
 */
//extern "C" TDMFINSTALL_API 
//int CreateTdmfDB(const char* szPathForDBFiles, const char* szCollectorPort);

/**
 * GetIPFromComputerName
 * 
 * @return int  0 = success, 
 *
 */
//extern "C" TDMFINSTALL_API 
//int GetIPFromComputerName(char* szComputerName, char* szIPAddress);

/**
 * ValidateIPAddress
 * 
 * @return int  0 = success, 
 *
 */
//extern "C" TDMFINSTALL_API 
//int ValidateIPAddress(const char* szIPAddress);
