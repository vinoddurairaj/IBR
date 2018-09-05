//==========================================================================;
//
//	I2CLog.H
//	WDM MiniDrivers development.
//		I2CScript implementation.
//			I2CLog Class definition.
//  Copyright (c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date: 2003/12/17 15:45:26 $
//	$Revision: 1.1.1.1 $
//	  $Author: bmachine $
//
//==========================================================================;

#ifndef _I2CLOG_H_
#define _I2CLOG_H_


#include "i2cgpio.h"


class CI2CLog
{
public:
	CI2CLog					( PDEVICE_OBJECT pDeviceObject);
	~CI2CLog				( void);

	// Attributes	
private:
	BOOL	m_bLogStarted;
	HANDLE	m_hLogFile;

	// Implementation
public:
	inline BOOL	GetLogStatus	( void) { return( m_bLogStarted); };

private:

};


#endif	// _I2CLOG_H_

