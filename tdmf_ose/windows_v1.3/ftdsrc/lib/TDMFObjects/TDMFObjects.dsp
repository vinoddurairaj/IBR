# Microsoft Developer Studio Project File - Name="TDMFObjects" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TDMFObjects - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TDMFObjects.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TDMFObjects.mak" CFG="TDMFObjects - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TDMFObjects - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TDMFObjects - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TDMFObjects - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\libdb" /I "..\libutil" /I "..\libsock" /I "..\libmngt" /I "..\libftd" /I "..\liblst" /I "..\liblic" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_WINDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc0c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Kernel32.lib ws2_32.lib $(TOP)\$(LIBMNGTDBG) $(TOP)\$(LIBDBDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBFTDDBG) $(TOP)\$(LIBLSTDBG) $(TOP)\$(LIBERRDBG) $(TOP)\$(LIBLICDBG) $(TOP)\lib\libResMgr\Debug\libResMgr.lib Version.lib Iphlpapi.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"Debug/DtcObjects.dll" /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\DtcObjects.dll
InputPath=.\Debug\DtcObjects.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TDMFObjects - Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TDMFObjects___Win32_Release"
# PROP BASE Intermediate_Dir "TDMFObjects___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /I "..\libdb" /I "..\libutil" /I "..\libsock" /I "..\libmngt" /I "..\libftd" /I "..\liblst" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_WINDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\libdb" /I "..\libutil" /I "..\libsock" /I "..\libmngt" /I "..\libftd" /I "..\liblst" /I "..\liblic" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_WINDLL" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0xc0c /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ws2_32.lib $(TOP)\$(LIBMNGTREL) $(TOP)\$(LIBDBREL) $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBLSTREL) $(TOP)\$(LIBERRREL) $(TOP)\$(LIBLICREL) $(TOP)\lib\libResMgr\Release\libResMgr.lib Version.lib Iphlpapi.lib /nologo /subsystem:windows /dll /machine:I386 /out:"Release/DtcObjects.dll"
# Begin Custom Build - Performing registration
OutDir=.\Release
TargetPath=.\Release\DtcObjects.dll
InputPath=.\Release\DtcObjects.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
TargetPath=.\Release\DtcObjects.dll
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\"$(PRODUCTNAME)"\$(TOS)	copy $(TargetPath) $(TOP)\"$(PRODUCTNAME)"\$(TOS)
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "TDMFObjects - Win32 Debug"
# Name "TDMFObjects - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Device.cpp
# End Source File
# Begin Source File

SOURCE=.\DeviceList.cpp
# End Source File
# Begin Source File

SOURCE=.\Domain.cpp
# End Source File
# Begin Source File

SOURCE=.\Event.cpp
# End Source File
# Begin Source File

SOURCE=.\lgcfg_buf_obj.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\mmp_API.cpp
# End Source File
# Begin Source File

SOURCE=.\ReplicationGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\ReplicationPair.cpp
# End Source File
# Begin Source File

SOURCE=.\ScriptServer.cpp
# End Source File
# Begin Source File

SOURCE=.\Server.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\System.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemTestWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\TDMFObjects.cpp
# End Source File
# Begin Source File

SOURCE=.\TDMFObjects.def
# End Source File
# Begin Source File

SOURCE=.\TDMFObjects.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Device.h
# End Source File
# Begin Source File

SOURCE=.\DeviceList.h
# End Source File
# Begin Source File

SOURCE=.\Domain.h
# End Source File
# Begin Source File

SOURCE=.\Event.h
# End Source File
# Begin Source File

SOURCE=.\lgcfg_buf_obj.h
# End Source File
# Begin Source File

SOURCE=.\mmp_API.h
# End Source File
# Begin Source File

SOURCE=.\ReplicationGroup.h
# End Source File
# Begin Source File

SOURCE=.\ReplicationPair.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\ScriptServer.h
# End Source File
# Begin Source File

SOURCE=.\Server.h
# End Source File
# Begin Source File

SOURCE=.\States.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\System.h
# End Source File
# Begin Source File

SOURCE=.\SystemTestWnd.h
# End Source File
# Begin Source File

SOURCE=.\TDMFObjectsDef.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\CComCollectorStats.rgs
# End Source File
# Begin Source File

SOURCE=.\CComTdmfCollectorState.rgs
# End Source File
# Begin Source File

SOURCE=.\ComCollectorStats.rgs
# End Source File
# Begin Source File

SOURCE=.\Device.rgs
# End Source File
# Begin Source File

SOURCE=.\DeviceList.rgs
# End Source File
# Begin Source File

SOURCE=.\Domain.rgs
# End Source File
# Begin Source File

SOURCE=.\Event.rgs
# End Source File
# Begin Source File

SOURCE=.\itdmfcol.bin
# End Source File
# Begin Source File

SOURCE=.\ITDMFCollectorState.rgs
# End Source File
# Begin Source File

SOURCE=.\ReplicationGroup.rgs
# End Source File
# Begin Source File

SOURCE=.\ReplicationPair.rgs
# End Source File
# Begin Source File

SOURCE=.\ScriptServer.rgs
# End Source File
# Begin Source File

SOURCE=.\ScriptServerFile.rgs
# End Source File
# Begin Source File

SOURCE=.\Server.rgs
# End Source File
# Begin Source File

SOURCE=.\System.rgs
# End Source File
# Begin Source File

SOURCE=.\TdmfCollectorState.rgs
# End Source File
# End Group
# Begin Group "COMObjects"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ComCollectorStats.cpp
# End Source File
# Begin Source File

SOURCE=.\ComCollectorStats.h
# End Source File
# Begin Source File

SOURCE=.\ComDevice.cpp
# End Source File
# Begin Source File

SOURCE=.\ComDevice.h
# End Source File
# Begin Source File

SOURCE=.\ComDeviceList.cpp
# End Source File
# Begin Source File

SOURCE=.\ComDeviceList.h
# End Source File
# Begin Source File

SOURCE=.\ComDomain.cpp
# End Source File
# Begin Source File

SOURCE=.\ComDomain.h
# End Source File
# Begin Source File

SOURCE=.\ComEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\ComEvent.h
# End Source File
# Begin Source File

SOURCE=.\ComReplicationGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\ComReplicationGroup.h
# End Source File
# Begin Source File

SOURCE=.\ComReplicationPair.cpp
# End Source File
# Begin Source File

SOURCE=.\ComReplicationPair.h
# End Source File
# Begin Source File

SOURCE=.\ComScriptServerFile.cpp
# End Source File
# Begin Source File

SOURCE=.\ComScriptServerFile.h
# End Source File
# Begin Source File

SOURCE=.\ComServer.cpp
# End Source File
# Begin Source File

SOURCE=.\ComServer.h
# End Source File
# Begin Source File

SOURCE=.\ComSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\ComSystem.h
# End Source File
# Begin Source File

SOURCE=.\TDMFObjects.idl
# ADD MTL /tlb ".\TDMFObjects.tlb" /h "TDMFObjects.h" /iid "TDMFObjects_i.c" /Oicf
# End Source File
# End Group
# End Target
# End Project
