/****************************************************************
 * Name:  data.h                                                *
 *                                                              *
 * Function:  Constants, structures, and function               *
 *            declarations specific to this DLL.                *
 *                                                              *
 ****************************************************************/

#ifndef __DATA__
#define __DATA__

#pragma pack(8)

//
// Data definition for the counters maintained by
// FTD
//
typedef struct _FTD_HEADER_DEFINITION 
{
	PERF_OBJECT_TYPE		FtdDevice;
	PERF_COUNTER_DEFINITION actual;
	PERF_COUNTER_DEFINITION effective;
	PERF_COUNTER_DEFINITION entries;
	PERF_COUNTER_DEFINITION sectors;
	PERF_COUNTER_DEFINITION pctdone;
	PERF_COUNTER_DEFINITION pctbab;
	PERF_COUNTER_DEFINITION bytesread;
	PERF_COUNTER_DEFINITION byteswritten;
} FTD_HEADER_DEFINITION, *PFTD_HEADER_DEFINITION;

typedef struct _FTD_COUNTERS {
    PERF_COUNTER_BLOCK  CounterBlock;
	__int64				actual;
	__int64				effective;
	int					entries;	/* # of entries in bab */
	int					sectors;	/* # of sectors in bab */
	int					pctdone;
	int					pctbab;
	__int64				bytesread;
	__int64				byteswritten;
} FTD_COUNTERS, *PFTD_COUNTERS;

#pragma pack ()

#endif  // __DATA__

