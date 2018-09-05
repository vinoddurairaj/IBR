#############################################################################
#
#       Sample makefile for the WDM stream class mini driver
#
#       $Date: 2003/12/17 15:45:28 $
#       $Revision: 1.1.1.1 $
#       $Author: bmachine $
#
#  $Copyright:	(c) 1997 - 1999  ATI Technologies Inc.  All Rights Reserved.$
#
#############################################################################

#BSCMAKE         = 1
#BSCTARGETS      = ATIXBAR.BSC 

ROOT            = $(BASEDIR)
MINIPORT        = atixbar
SRCDIR          = ..
ALTSRCDIR       = $(ROOT)\src\wdm\capture\mini\ATIShare
WANT_LATEST     = TRUE
WANT_WDMDDK     = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = Streaming class sample minidriver
VERDIRLIST      = maxdebug debug retail
LIBS			= $(BASEDIR)\lib\*\$(DDKBUILDENV)\stream.lib\
              	  $(BASEDIR)\lib\*\$(DDKBUILDENV)\ksguid.lib\
                  $(BASEDIR)\lib\*\$(DDKBUILDENV)\dxapi.lib \
                  $(BASEDIR)\lib\*\$(DDKBUILDENV)\atishare.lib
!if "$(VERDIR)" == "debug"
CFLAGS 			= -DMSC $(CFLAGS) -TP
!else
CFLAGS 			= -DMSC $(CFLAGS) -TP -Oi
!endif

RESNAME         = atixbar

NOVXDVERSION    = TRUE
CFLAGS          = -DMSC $(CFLAGS)

!IF "$(VERDIR)" != "debug"
CFLAGS	 = $(CFLAGS) -Oi
!endif

OBJS =	atixbar.obj wdmxbar.obj xbarprop.obj \
		aticonfg.obj i2script.obj pinmedia.obj registry.obj \
		i2clog.obj atixbar.res
 
!include $(ROOT)\ddk\inc\master.mk

INCLUDE=$(WDMROOT)\tools\vc\include;$(INCLUDE)
INCLUDE=$(WDMROOT)\ddk\inc\win98;$(INCLUDE)
INCLUDE=$(WDMROOT)\capture\mini\ATIShare;$(INCLUDE)

