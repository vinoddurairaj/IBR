#if !defined(__dynamic_h__)
#define __dynamic_h__

BOOL UnloadDriver(LPCTSTR DriverName);
HANDLE LoadDriver(BOOL *fNTDynaLoaded, LPCTSTR DosName, LPCTSTR DriverName, LPCTSTR ServiceExe);

#endif // __dynamic_h__

