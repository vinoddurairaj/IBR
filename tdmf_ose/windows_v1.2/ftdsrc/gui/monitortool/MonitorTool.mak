# Microsoft Developer Studio Generated NMAKE File, Based on MonitorTool.dsp
!IF "$(CFG)" == ""
CFG=MonitorTool - Win32 Debug
!MESSAGE No configuration specified. Defaulting to MonitorTool - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "MonitorTool - Win32 Release" && "$(CFG)" != "MonitorTool - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MonitorTool.mak" CFG="MonitorTool - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MonitorTool - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MonitorTool - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "MonitorTool - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\MonitorTool.exe"


CLEAN :
	-@erase "$(INTDIR)\ETSLayout.obj"
	-@erase "$(INTDIR)\MonitorTool.obj"
	-@erase "$(INTDIR)\MonitorTool.pch"
	-@erase "$(INTDIR)\MonitorTool.res"
	-@erase "$(INTDIR)\MonitorToolDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MonitorTool.exe"
	-@erase "$(OUTDIR)\MonitorTool.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /Z7 /O2 /I "..\..\lib\libftd" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libutil" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\MonitorTool.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MonitorTool.res" /d "NDEBUG" /d "_AFXDLL" /d "MTI" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MonitorTool.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBLSTREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBUTILREL) ws2_32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\MonitorTool.pdb" /debug /debugtype:both /machine:I386 /out:"$(OUTDIR)\MonitorTool.exe" /opt:ref,icf 
LINK32_OBJS= \
	"$(INTDIR)\ETSLayout.obj" \
	"$(INTDIR)\MonitorTool.obj" \
	"$(INTDIR)\MonitorToolDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\MonitorTool.res"

"$(OUTDIR)\MonitorTool.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\Release\MonitorTool.exe
TargetName=MonitorTool
SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\MonitorTool.exe"
   md C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"
	copy .\Release\MonitorTool.exe C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"\lrMonitorTool.exe
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "MonitorTool - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\MonitorTool.exe" "$(OUTDIR)\MonitorTool.bsc"


CLEAN :
	-@erase "$(INTDIR)\ETSLayout.obj"
	-@erase "$(INTDIR)\ETSLayout.sbr"
	-@erase "$(INTDIR)\MonitorTool.obj"
	-@erase "$(INTDIR)\MonitorTool.pch"
	-@erase "$(INTDIR)\MonitorTool.res"
	-@erase "$(INTDIR)\MonitorTool.sbr"
	-@erase "$(INTDIR)\MonitorToolDlg.obj"
	-@erase "$(INTDIR)\MonitorToolDlg.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\MonitorTool.bsc"
	-@erase "$(OUTDIR)\MonitorTool.exe"
	-@erase "$(OUTDIR)\MonitorTool.ilk"
	-@erase "$(OUTDIR)\MonitorTool.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libftd" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libutil" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\MonitorTool.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MonitorTool.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MonitorTool.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ETSLayout.sbr" \
	"$(INTDIR)\MonitorTool.sbr" \
	"$(INTDIR)\MonitorToolDlg.sbr" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\MonitorTool.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBLSTDBG) $(TOP)\$(LIBFTDDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBUTILDBG) ws2_32.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\MonitorTool.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MonitorTool.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\ETSLayout.obj" \
	"$(INTDIR)\MonitorTool.obj" \
	"$(INTDIR)\MonitorToolDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\MonitorTool.res"

"$(OUTDIR)\MonitorTool.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MonitorTool.dep")
!INCLUDE "MonitorTool.dep"
!ELSE 
!MESSAGE Warning: cannot find "MonitorTool.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "MonitorTool - Win32 Release" || "$(CFG)" == "MonitorTool - Win32 Debug"
SOURCE=.\ETSLayout.cpp

!IF  "$(CFG)" == "MonitorTool - Win32 Release"


"$(INTDIR)\ETSLayout.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MonitorTool.pch"


!ELSEIF  "$(CFG)" == "MonitorTool - Win32 Debug"


"$(INTDIR)\ETSLayout.obj"	"$(INTDIR)\ETSLayout.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MonitorTool.pch"


!ENDIF 

SOURCE=.\MonitorTool.cpp

!IF  "$(CFG)" == "MonitorTool - Win32 Release"


"$(INTDIR)\MonitorTool.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MonitorTool.pch"


!ELSEIF  "$(CFG)" == "MonitorTool - Win32 Debug"


"$(INTDIR)\MonitorTool.obj"	"$(INTDIR)\MonitorTool.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MonitorTool.pch"


!ENDIF 

SOURCE=.\MonitorTool.rc

"$(INTDIR)\MonitorTool.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\MonitorToolDlg.cpp

!IF  "$(CFG)" == "MonitorTool - Win32 Release"


"$(INTDIR)\MonitorToolDlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MonitorTool.pch"


!ELSEIF  "$(CFG)" == "MonitorTool - Win32 Debug"


"$(INTDIR)\MonitorToolDlg.obj"	"$(INTDIR)\MonitorToolDlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MonitorTool.pch"


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "MonitorTool - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Z7 /O2 /I "..\..\lib\libftd" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libutil" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\MonitorTool.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\MonitorTool.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MonitorTool - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libftd" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libutil" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\MonitorTool.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\MonitorTool.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

