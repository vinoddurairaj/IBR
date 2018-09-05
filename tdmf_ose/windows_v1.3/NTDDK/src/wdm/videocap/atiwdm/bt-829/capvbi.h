#pragma once

//==========================================================================;
//
//	CWDMVBICaptureStream - VBI Capture Stream class declarations
//
//		$Date: 2003/12/17 15:45:27 $
//	$Revision: 1.1.1.1 $
//	  $Author: bmachine $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "i2script.h"
#include "aticonfg.h"

#include "CapStrm.h"


class CWDMVBICaptureStream : public CWDMCaptureStream
{
public:
	CWDMVBICaptureStream(PHW_STREAM_OBJECT pStreamObject,
						CWDMVideoDecoder * pCWDMVideoDecoder,
						PKSDATAFORMAT pKSDataFormat,
						PUINT puiErrorCode);
	~CWDMVBICaptureStream();

	void * operator new(size_t size, void * pAllocation) { return(pAllocation);}
	void operator delete(void * pAllocation) {}

private:
	PKS_VBIINFOHEADER		m_pVBIInfoHeader;    //
	KS_VBI_FRAME_INFO       m_VBIFrameInfo;
	BOOL					m_bVBIinitialized;

	void ResetFrameCounters();
	ULONG GetFrameSize() { return m_pVBIInfoHeader->BufferSize; }
	void GetDroppedFrames(PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames);
	BOOL GetCaptureHandle();
	VOID SetFrameInfo(PHW_STREAM_REQUEST_BLOCK);
	ULONG GetFieldInterval() { return 1; }
};
