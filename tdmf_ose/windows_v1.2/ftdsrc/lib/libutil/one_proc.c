#include <windows.h>
#include <crtdbg.h>

void ftd_util_force_to_use_only_one_processor()
{
//
// SVG TST 03-06-03 - remove this for processor independence
//
#if 1
	HANDLE  hProcess = GetCurrentProcess();
	DWORD   ProcessAffinityMask, // process affinity mask
			SystemAffinityMask;  // system affinity mask
	BOOL b = GetProcessAffinityMask(  hProcess,                  // handle to process
								      &ProcessAffinityMask, // process affinity mask
								      &SystemAffinityMask   // system affinity mask
								    );
	if ( b )
	{
		DWORD bit = 1;
		//find first bit set
		while( (bit & ProcessAffinityMask) == 0 )	
			bit <<= 1;
		//clear all other bits
		ProcessAffinityMask &= bit;

		b = SetProcessAffinityMask( hProcess, 
								    ProcessAffinityMask );

	}	

	_ASSERT(b);
#endif
}