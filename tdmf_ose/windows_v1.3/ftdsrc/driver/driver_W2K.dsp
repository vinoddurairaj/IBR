# Microsoft Developer Studio Project File - Name="driver_W2K" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=driver_W2K - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "driver_W2K.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "driver_W2K.mak" CFG="driver_W2K - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "driver_W2K - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "driver_W2K - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/ftdsrc/driver_W2K", NIBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "driver_W2K - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "driver_W2K\Free"
# PROP Intermediate_Dir "driver_W2K\Free"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE F90 /compile_only /include:"Release/" /nologo /warn:nofileopt
# ADD F90 /compile_only /include:"Release/" /nologo /warn:nofileopt
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gz /MLd /W3 /Oi /Gy /I "$(BASEDIR)\inc" /I "$(BASEDIR)\inc\ddk" /FI"warning.h" /D WIN32=100 /D "_WINDOWS" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _X86_=1 /D "UNICODE" /D "_CPU_X86_" /D "KERNEL" /D POOL_TAGGING=1 /Fp".\driver_W2K\Free/Ftd.pch" /YX /Fo".\driver_W2K\Free/" /Fd".\driver_W2K\Free/" /FD /Zel -cbstring /QIfdiv- /QIf /GF /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 $(BASEDIR)\libfre\i386\int64.lib $(BASEDIR)\libfre\i386\ntoskrnl.lib $(BASEDIR)\libfre\i386\hal.lib /nologo /base:"0x10000" /version:4.0 /entry:"DriverEntry" /pdb:none /map /debug /debugtype:both /machine:IX86 /nodefaultlib /out:"driver_W2K\Free\DtcBlock.sys" /libpath:"$(BASEDIR)\libfre\i386\\" /driver /IGNORE:4001,4037,4039,4065,4078,4087,4089,4096 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /FORCE:MULTIPLE /OPT:REF /OPTIDATA /align:0x20 /osversion:4.00 /subsystem:native
# Begin Custom Build - Striping Symbols
InputPath=.\driver_W2K\Free\DtcBlock.sys
SOURCE="$(InputPath)"

"$(TOP)\driver\driver_W2K\Free\$(PRODUCTNAME)Block.dbg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	rebase -x .. -b 0x10000 $(TOP)\driver\driver_W2K\Free\$(PRODUCTNAME)Block.sys

# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\"$(PRODUCTNAME)"\$(TOS)	copy driver_W2K\Free\DtcBlock.sys $(TOP)\"$(PRODUCTNAME)"\$(TOS)\DtcBlock.sys
# End Special Build Tool

!ELSEIF  "$(CFG)" == "driver_W2K - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "driver_W2K\Checked"
# PROP Intermediate_Dir "driver_W2K\Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE F90 /check:bounds /compile_only /debug:full /include:"Debug/" /nologo /warn:argument_checking /warn:nofileopt
# ADD F90 /check:bounds /compile_only /debug:full /include:"Debug/" /nologo /warn:argument_checking /warn:nofileopt
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /W3 /Z7 /Oi /Gy /I "$(BASEDIR)\inc" /I "$(BASEDIR)\inc\ddk" /FI"warning.h" /D DBG=1 /D WIN32=100 /D "_DEBUG" /D "_WINDOWS" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _X86_=1 /D "UNICODE" /D "_CPU_X86_" /D "KERNEL" /D POOL_TAGGING=1 /Fp".\driver_W2K\Checked/Ftd.pch" /YX /Fo".\driver_W2K\Checked/" /Fd".\driver_W2K\Checked/" /FD /Zel -cbstring /QIfdiv- /QIf /GF /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 $(BASEDIR)\libchk\i386\int64.lib $(BASEDIR)\libchk\i386\ntoskrnl.lib $(BASEDIR)\libchk\i386\hal.lib /nologo /base:"0x10000" /version:4.0 /stack:0x200000 /entry:"DriverEntry" /map /debug /debugtype:both /machine:IX86 /nodefaultlib /out:"driver_W2K\Checked\DtcBlock.sys" /libpath:"$(BASEDIR)\libchk\i386\\" /driver /debug:notmapped,FULL /IGNORE:4001,4037,4039,4065,4078,4087,4089,4096 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /FORCE:MULTIPLE /OPT:REF /OPTIDATA /align:0x20 /osversion:4.00 /subsystem:native
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build -
InputPath=.\driver_W2K\Checked\DtcBlock.sys
SOURCE="$(InputPath)"

"$(TOP)\driver\driver_W2K\Checked\$(CMDPREFIX)Block.dbg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	rebase -x .. -b 0x10000 $(TOP)\driver\driver_W2K\Checked\$(CMDPREFIX)Block.sys

# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\"$(PRODUCTNAME)"\debug	copy driver_W2K\Checked\DtcBlock.sys $(TOP)\"$(PRODUCTNAME)"\debug\DtcBlock.sys
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "driver_W2K - Win32 Release"
# Name "driver_W2K - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;f90;for;f;fpp"
# Begin Source File

SOURCE=.\ftd_all.c
# End Source File
# Begin Source File

SOURCE=.\ftd_bab.c
# End Source File
# Begin Source File

SOURCE=.\ftd_bits.c
# End Source File
# Begin Source File

SOURCE=.\ftd_ddi.c
# End Source File
# Begin Source File

SOURCE=.\ftd_ioctl.c
# End Source File
# Begin Source File

SOURCE=.\ftd_klog.c
# End Source File
# Begin Source File

SOURCE=.\ftd_nt.c
# End Source File
# Begin Source File

SOURCE=.\mapmem.c
# End Source File
# Begin Source File

SOURCE=.\memset.c
# End Source File
# Begin Source File

SOURCE=..\version.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\ftd_all.h
# End Source File
# Begin Source File

SOURCE=.\ftd_bab.h
# End Source File
# Begin Source File

SOURCE=.\ftd_bits.h
# End Source File
# Begin Source File

SOURCE=.\ftd_ddi.h
# End Source File
# Begin Source File

SOURCE=.\ftd_def.h
# End Source File
# Begin Source File

SOURCE=.\ftd_klog.h
# End Source File
# Begin Source File

SOURCE=.\ftd_nt.h
# End Source File
# Begin Source File

SOURCE=.\ftd_var.h
# End Source File
# Begin Source File

SOURCE=.\ftdio.h
# End Source File
# Begin Source File

SOURCE=.\sysmacros.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\RES.RC
# End Source File
# End Group
# End Target
# End Project
