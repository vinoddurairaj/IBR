#
# LICENSED MATERIALS / PROPERTY OF IBM
# %PRODUCTNAME% version %VERSION%
# (c) Copyright %COMPANYNAME%  %COPYRIGHTYEAR 2012%.  All Rights Reserved.
# The source code for this program is not published or otherwise
# divested of its trade secrets, irrespective of what has been
# deposited with the U.S. Copyright Office.
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with %COMPANYNAME%
#
#
#
#
#
TOP=../..

BIN_INSTALL=${Q}failover ${Q}sendfailover
SH_PROGRAM=${Q}failover
SHSRC=failover.sh

PROGRAM=sendfailover
OBJS=ftd_fo.o 
LIBS= ${LIBGEN} ${SOCKET_LIBS} ${LIBMD5} ${LVM_LIBS}
CINCLUDES=${INCGEN} ${INCDEVICE}

include ${TOP}/mk/prog.gmk

CINCLUDES += $(UINCLUDES)
CDEFINES  += $(UDEFINES)
COPTFLAGS += $(UCFLAGS)
