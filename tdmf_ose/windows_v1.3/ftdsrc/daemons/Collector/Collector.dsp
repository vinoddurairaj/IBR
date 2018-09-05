# Microsoft Developer Studio Project File - Name="Collector" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Collector - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Collector.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Collector.mak" CFG="Collector - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Collector - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Collector - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SafeData/libexec/Collector", SDBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Collector - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE F90 /include:"Release/"
# ADD F90 /include:"Release/"
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Z7 /Od /I "..\..\libmngt" /I "..\..\lib\libdb" /I "..\..\lib\libmngt" /I "..\..\lib\libftd" /I "..\..\driver" /I "..\..\lib\libutil" /I "..\..\lib\liberr" /I "..\..\lib\liblst" /I "..\..\lib\libproc" /I "..\..\lib\libcomp" /I "..\..\lib\libsock" /I "..\..\lib\liblic" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 $(TOP)\$(LIBERRREL) $(TOP)\$(LIBDBREL) $(TOP)\$(LIBMNGTREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBLICREL) version.lib ws2_32.lib $(TOP)\lib\libResMgr\Release\libResMgr.lib Iphlpapi.lib /nologo /entry:"WinMainCRTStartup" /subsystem:console /machine:I386 /opt:ref,icf
# SUBTRACT LINK32 /pdb:none /debug
# Begin Special Build Tool
TargetPath=.\Release\Collector.exe
TargetName=Collector
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\"$(PRODUCTNAME)"\$(TOS)	copy $(TargetPath) $(TOP)\"$(PRODUCTNAME)"\$(TOS)\Dtc$(TargetName).exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Collector - Win32 Debug"

# PROP BASE Use_MFC 0
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
F90=df.exe
# ADD BASE F90 /include:"Debug/"
# ADD F90 /include:"Debug/"
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libdb" /I "..\..\lib\libmngt" /I "..\..\lib\libftd" /I "..\..\driver" /I "..\..\lib\libutil" /I "..\..\lib\liberr" /I "..\..\lib\liblst" /I "..\..\lib\libproc" /I "..\..\lib\libcomp" /I "..\..\lib\libsock" /I "..\..\lib\liblic" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /YX"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 nafxcwd.lib libcmtd.lib $(TOP)\$(LIBDBDBG) $(TOP)\$(LIBMNGTDBG) $(TOP)\$(LIBFTDDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBLICDBG) version.lib ws2_32.lib $(TOP)\lib\libResMgr\Debug\libResMgr.lib Iphlpapi.lib /nologo /entry:"WinMainCRTStartup" /subsystem:console /debug /machine:I386 /nodefaultlib:"libcmtd.lib nafxcwd.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetPath=.\Debug\Collector.exe
TargetName=Collector
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\"$(PRODUCTNAME)"\debug	copy $(TargetPath) $(TOP)\"$(PRODUCTNAME)"\debug\Dtc$(TargetName).exe
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Collector - Win32 Release"
# Name "Collector - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DBServer.cpp
# End Source File
# Begin Source File

SOURCE=.\DBServer_AgentAliveSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\DBServer_AgentMonitor.cpp
# End Source File
# Begin Source File

SOURCE=.\dbserver_db.cpp
# End Source File
# Begin Source File

SOURCE=.\DBServer_sock.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\version.c
# End Source File
# Begin Source File

SOURCE=.\WinService.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\DBServer.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\FileVersion.RC
# End Source File
# End Group
# End Target
# End Project
