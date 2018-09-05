# Microsoft Developer Studio Generated NMAKE File, Based on LogMsg.dsp
!IF "$(CFG)" == ""
CFG=LogMsg - Win32 Debug
!MESSAGE No configuration specified. Defaulting to LogMsg - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "LogMsg - Win32 Release" && "$(CFG)" != "LogMsg - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LogMsg.mak" CFG="LogMsg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LogMsg - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "LogMsg - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "LogMsg - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\LogMsg.dll"


CLEAN :
	-@erase "$(INTDIR)\LogMsg.obj"
	-@erase "$(INTDIR)\LogMsg.pch"
	-@erase "$(INTDIR)\LogMsg.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\LogMsg.dll"
	-@erase "$(OUTDIR)\LogMsg.exp"
	-@erase "$(OUTDIR)\LogMsg.lib"
	-@erase "$(OUTDIR)\LogMsg.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LOGMSG_EXPORTS" /Fp"$(INTDIR)\LogMsg.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\LogMsg.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\LogMsg.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\LogMsg.pdb" /debug /debugtype:both /machine:I386 /out:"$(OUTDIR)\LogMsg.dll" /implib:"$(OUTDIR)\LogMsg.lib" /opt:ref,icf 
LINK32_OBJS= \
	"$(INTDIR)\LogMsg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\LogMsg.res"

"$(OUTDIR)\LogMsg.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\Release\LogMsg.dll
TargetName=LogMsg
SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\LogMsg.dll"
   md C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"
	copy .\Release\LogMsg.dll C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"\lrLogMsg.dll
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "LogMsg - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\$(CAPQ)LogMsg.dll"


CLEAN :
	-@erase "$(INTDIR)\LogMsg.obj"
	-@erase "$(INTDIR)\LogMsg.pch"
	-@erase "$(INTDIR)\LogMsg.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\$(CAPQ)LogMsg.dll"
	-@erase "$(OUTDIR)\$(CAPQ)LogMsg.exp"
	-@erase "$(OUTDIR)\$(CAPQ)LogMsg.ilk"
	-@erase "$(OUTDIR)\$(CAPQ)LogMsg.lib"
	-@erase "$(OUTDIR)\$(CAPQ)LogMsg.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LOGMSG_EXPORTS" /Fp"$(INTDIR)\LogMsg.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\LogMsg.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\LogMsg.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\$(CAPQ)LogMsg.pdb" /debug /machine:I386 /out:"$(OUTDIR)\$(CAPQ)LogMsg.dll" /implib:"$(OUTDIR)\$(CAPQ)LogMsg.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\LogMsg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\LogMsg.res"

"$(OUTDIR)\$(CAPQ)LogMsg.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("LogMsg.dep")
!INCLUDE "LogMsg.dep"
!ELSE 
!MESSAGE Warning: cannot find "LogMsg.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "LogMsg - Win32 Release" || "$(CFG)" == "LogMsg - Win32 Debug"
SOURCE=.\LogMsg.cpp

"$(INTDIR)\LogMsg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\LogMsg.pch"


SOURCE=.\LogMsg.mc

!IF  "$(CFG)" == "LogMsg - Win32 Release"

InputPath=.\LogMsg.mc

".\LogMsg.rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	mc -v -c -s -h ..\..\ -U LogMsg.mc
<< 
	

!ELSEIF  "$(CFG)" == "LogMsg - Win32 Debug"

InputPath=.\LogMsg.mc

".\LogMsg.rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	mc -v -c -s -h ..\..\ -U LogMsg.mc
<< 
	

!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "LogMsg - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /Z7 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LOGMSG_EXPORTS" /Fp"$(INTDIR)\LogMsg.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\LogMsg.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "LogMsg - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LOGMSG_EXPORTS" /Fp"$(INTDIR)\LogMsg.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\LogMsg.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\LogMsg.rc

"$(INTDIR)\LogMsg.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

