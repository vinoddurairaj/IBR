/****************************************************************
 * Name:  data.c                                                *
 *                                                              *
 * Function:  This file defines the static portions             *
 *            of the data returned by the collection DLL.       *
 *                                                              *
 ****************************************************************/

//
// All-inclusive header file
//
#include "perf.h"

// dummy local variable
static    FTD_COUNTERS     counters;

//
//  Constant structure initializations 
//
FTD_HEADER_DEFINITION FtdHeaderDefinition =
{
	//
	// PERF_OBJECT_TYPE for FtdDevice
	//
	{
		sizeof( FTD_HEADER_DEFINITION ) + sizeof(FTD_COUNTERS),
		sizeof(FTD_HEADER_DEFINITION),
		sizeof(PERF_OBJECT_TYPE),
		FTDDEVICE,
		NULL,
		FTDDEVICE,
		NULL,
		PERF_DETAIL_NOVICE,
		(sizeof( FTD_HEADER_DEFINITION )-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
		0,  // application memory bytes is the default counter
		0,  // 0 instances to start with
		0,  // unicode instance names
		{0,0},
		{0,0}
    },
	//
	// PERF_COUNTER_DEFINITION for the
	// XFERACTUAL counter
	//
	{
		sizeof(PERF_COUNTER_DEFINITION),
		XFERACTUAL,
		NULL,
		XFERACTUAL,
		NULL,
		-5,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_BULK_COUNT,
		sizeof(counters.actual),
		0
	},
	//
	// PERF_COUNTER_DEFINITION for the
	// XFEREFECTIVE counter
	//
	{
		sizeof(PERF_COUNTER_DEFINITION),
		XFEREFECTIVE,
		NULL,
		XFEREFECTIVE,
		NULL,
		-5,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_BULK_COUNT,
		sizeof(counters.effective),
		0
	},
	//
	// PERF_COUNTER_DEFINITION for the
	// BABENTRIES counter
	//
	{
		sizeof(PERF_COUNTER_DEFINITION),
		BABENTRIES,
		NULL,
		BABENTRIES,
		NULL,
		-1,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(counters.entries),
		0
	},
	//
	// PERF_COUNTER_DEFINITION for the
	// BABSECTORS counter
	//
	{
		sizeof(PERF_COUNTER_DEFINITION),
		BABSECTORS,
		NULL,
		BABSECTORS,
		NULL,
		-3,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(counters.sectors),
		0
	},
	//
	// PERF_COUNTER_DEFINITION for the
	// PERCENTDONE counter
	//
	{
		sizeof(PERF_COUNTER_DEFINITION),
		PERCENTDONE,
		NULL,
		PERCENTDONE,
		NULL,
		0,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(counters.pctdone),
		0
	},
	//
	// PERF_COUNTER_DEFINITION for the
	// PERCENTBABFULL counter
	//
	{
		sizeof(PERF_COUNTER_DEFINITION),
		PERCENTBABFULL,
		NULL,
		PERCENTBABFULL,
		NULL,
		0,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(counters.pctbab),
		0
	},

	//
	// PERF_COUNTER_DEFINITION for the
	// READBYTES counter
	//
	{
		sizeof(PERF_COUNTER_DEFINITION),
		READBYTES,
		NULL,
		READBYTES,
		NULL,
		-5,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_BULK_COUNT,
		sizeof(counters.bytesread),
		0
	},
	//
	// PERF_COUNTER_DEFINITION for the
	// WRITTENBYTES counter
	//
	{
		sizeof(PERF_COUNTER_DEFINITION),
		WRITTENBYTES,
		NULL,
		WRITTENBYTES,
		NULL,
		-5,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_BULK_COUNT,
		sizeof(counters.byteswritten),
		0
	}

};

