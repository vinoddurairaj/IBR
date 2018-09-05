/**************************************************************************************

Module Name: sftk_main.h   
Author Name: Parag sanghvi
Description: Define all General macro used in driver. 
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#ifndef _SFTK_MAIN_H_
#define _SFTK_MAIN_H_

// To Enables Target Side code, define this macro to 1 
#define		TARGET_SIDE					1

// PS_HRDB_VALIDATION_USE_TIMESTAMP is used for pstore file. It writes Time Stamp After 
// LRDB and HRDB Bitmap memory. TimeStamp is the time Bitmap got flush it to Pstore file.
// So Always TimeStamp Following LRDB Bitmap region must be >= TimeStamp Following HRDB Bitmap Region
// If it is than HRDB Bitmap region is valid to use during boot time.
#define PS_HRDB_VALIDATION_USE_TIMESTAMP	1

// PS_BITMAP_CHECKSUM_ON: if this is ON means All Bitmap (LRDB abd HRDB) memory is always
// use checksum, and that checksum gets written to SFTK_PS_DEV structure in pstore file
// this costly work as each time LRDB geting updated it calculate Checksum and writes it 
// to SFTK_PS_HDR and SFTK_PS_HDR also gets writen to pstore file (which 1K size) !!
//
#define PS_BITMAP_CHECKSUM_ON				0

// DBG_MESSAGE_DUMP_TO_FILE : to 1 if we would like to pring Debug message like flushing to Logfile !!
#define DBG_MESSAGE_DUMP_TO_FILE			1

// MM_TEST_WINDOWS_SLAB is ON to test windows OS supplied Fixed Size Slab Allocator usage 
#define MM_TEST_WINDOWS_SLAB			0

// For Windows OS define this macro
#define WINDOWS_NT

// Define all General macro used in driver. 
// All OS Dependant macros are defined in Sftk_OS.h file.
#include <sftk_os.h> 
#include <sftk_macro.h>

#include <sftk_block.h>	// for Message ID defination for system event log messaging

#include <ftdio.h>	// internal serice and commong macros, error and structure definations 
// #include "..\Cache\newbab\sftkprotocol.h"	// internal serice and commong macros, error and structure definations 

// sftk_mm.h file has this defined #include <sftk_protocol.h>
#if 1 // _PROTO_TDI_CODE_

// These are the Standard Header Files of the SFTK_TDI
#include <sftk_tdiutil.h>
#include <INetInc.h>
// These are the Protocol and Communication Files
#include <sftk_protocol.h>	// Veera : Defines all Protocol related structures, macros and enums
#include <sftk_com.h>			// Veera : Communication main header file
#endif

#include <sftk_def.h>
#include <sftk_NT.h>


#include <sftk_ps.h>
// #include <sftk_MM.h> // its included in sftk_def.h file

// All API Prototypes definations
#include <sftk_Proto.h> 

#if 1	//COMPRESSION HEADER VEERA
#include <sftk_comp.h>
#endif

#if 1	//The CHECKSUM ALGORITHM VEERA
#include <sftk_md5const.h>
#endif

#if 1 // _PROTO_TDI_CODE_
#include <sftk_comProto.h>		// Veera : Communication Module API Prototype header file
// #include <sftk_protocol.h>	// Veera : Defines all Protocol related API definations
#endif




#endif // _SFTK_MAIN_H_