# Microsoft Developer Studio Project File - Name="info" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=info - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "info.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "info.mak" CFG="info - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "info - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "info - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SafeData/libexec/info", BSEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "info - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE F90 /compile_only /include:"Release/" /nologo /warn:nofileopt /winapp
# ADD F90 /compile_only /include:"Release/" /nologo /warn:nofileopt /winapp
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Z7 /O2 /I "..\..\lib\liblic" /I "..\..\lib\libftd" /I "..\..\driver" /I "..\..\lib\libutil" /I "..\..\lib\liberr" /I "..\..\lib\liblst" /I "..\..\lib\libproc" /I "..\..\lib\libcomp" /I "..\..\lib\libsock" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 $(TOP)\$(LIBPROCREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBERRREL) $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBCOMPREL) $(TOP)\$(LIBLSTREL) $(TOP)\$(LIBMD5REL) ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /debugtype:both /machine:I386 /opt:ref,icf
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetPath=.\Release\info.exe
TargetName=info
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\"$(PRODUCTNAME)"	copy $(TargetPath) $(TOP)\"$(PRODUCTNAME)"\$(QNM)$(TargetName).exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "info - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE F90 /check:bounds /compile_only /debug:full /include:"Debug/" /nologo /warn:argument_checking /warn:nofileopt /winapp
# ADD F90 /check:bounds /compile_only /debug:full /include:"Debug/" /nologo /warn:argument_checking /warn:nofileopt /winapp
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\liblic" /I "..\..\lib\libftd" /I "..\..\driver" /I "..\..\lib\libutil" /I "..\..\lib\liberr" /I "..\..\lib\liblst" /I "..\..\lib\libproc" /I "..\..\lib\libcomp" /I "..\..\lib\libsock" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 $(TOP)\$(LIBPROCDBG) $(TOP)\$(LIBFTDDBG) $(TOP)\$(LIBERRDBG) $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBCOMPDBG) $(TOP)\$(LIBLSTDBG) $(TOP)\$(LIBMD5DBG) ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetPath=.\Debug\info.exe
TargetName=info
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\DEBUG\"$(PRODUCTNAME)"	copy $(TargetPath) $(TOP)\DEBUG\"$(PRODUCTNAME)"\$(QNM)$(TargetName).exe
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "info - Win32 Release"
# Name "info - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;f90;for;f;fpp"
# Begin Source File

SOURCE=.\ftd_info.c
# End Source File
# Begin Source File

SOURCE=..\..\version.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
