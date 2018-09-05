#############################################################################
#
#	Makefile for AtiBt829 WDM driver
#
#		$Date: 2003/12/17 15:45:27 $
#	$Revision: 1.1.1.1 $
#	  $Author: bmachine $
#
#  $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
#
##########################################################################

BSCMAKE         = 1
BSCTARGETS      = AtiBt829.BSC 
	
ROOT            = $(BASEDIR)
MINIPORT		= AtiBt829
SRCDIR          = ..
ALTSRCDIR       = $(ROOT)\src\wdm\capture\mini\bt-829
WANT_LATEST     = TRUE
WANT_WDMDDK		= TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = WDM ATI Bt829 MiniDriver
VERDIRLIST		= maxdebug debug retail

LIBS			= $(BASEDIR)\lib\*\$(DDKBUILDENV)\stream.lib\
              	  $(BASEDIR)\lib\*\$(DDKBUILDENV)\ksguid.lib\
                  $(BASEDIR)\lib\*\$(DDKBUILDENV)\dxapi.lib \
                  $(BASEDIR)\lib\*\$(DDKBUILDENV)\atishare.lib

!if "$(VERDIR)" == "debug"
CFLAGS 			= -DMSC $(CFLAGS) -TP
!else
CFLAGS 			= -DMSC $(CFLAGS) -TP -Oi
!endif

RESNAME			= AtiBt

NOVXDVERSION    = TRUE

OBJS =	capmain.obj device.obj decoder.obj scaler.obj xbar.obj ourcrt.obj \
        regbase.obj register.obj atibt.res \
				wdmvdec.obj drventry.obj \
				vidstrm.obj vpstrm.obj capstrm.obj capvbi.obj capvideo.obj \
				decdev.obj decvport.obj mediums.obj strminfo.obj
 
!include $(ROOT)\ddk\inc\master.mk

INCLUDE=$(WDMROOT)\tools\vc\include;$(INCLUDE)
INCLUDE=$(WDMROOT)\ddk\inc\win98;$(INCLUDE)
INCLUDE=$(WDMROOT)\capture\mini\ATIShare;$(INCLUDE)
INCLUDE=$(WDMROOT)\capture\mini\bt-829;$(INCLUDE)
INCLUDE=$(WDMROOT)\capture\mini\bt-829\AtiBt;$(INCLUDE)

