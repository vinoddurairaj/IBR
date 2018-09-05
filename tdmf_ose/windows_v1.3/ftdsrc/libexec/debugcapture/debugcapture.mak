# Microsoft Developer Studio Generated NMAKE File, Based on debugcapture.dsp
!IF "$(CFG)" == ""
CFG=debugcapture - Win32 Debug
!MESSAGE No configuration specified. Defaulting to debugcapture - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "debugcapture - Win32 Release" && "$(CFG)" != "debugcapture - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "debugcapture.mak" CFG="debugcapture - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "debugcapture - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "debugcapture - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "debugcapture - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\debugcapture.exe"


CLEAN :
	-@erase "$(INTDIR)\DebugCapture.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\debugcapture.exe"
	-@erase "$(OUTDIR)\debugcapture.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /I "..\..\libftd" /I "..\..\lib\liblic" /I "..\..\lib\liblic2" /I "..\..\lib\libftd" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\debugcapture.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\debugcapture.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBERRDBG) $(TOP)\$(LIBLSTREL) $(TOP)\$(LIBFTDREL) kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBLICREL) Ws2_32.lib $(TOP)\$(LIBSOCKREL) /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\debugcapture.pdb" /debug /debugtype:both /machine:I386 /out:"$(OUTDIR)\debugcapture.exe" /opt:ref,icf 
LINK32_OBJS= \
	"$(INTDIR)\DebugCapture.obj"

"$(OUTDIR)\debugcapture.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\Release\debugcapture.exe
TargetName=DebugCapture
SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\debugcapture.exe"
   md C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"
	copy .\Release\debugcapture.exe C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"\lrdebugcapture.exe
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "debugcapture - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\debugcapture.exe"


CLEAN :
	-@erase "$(INTDIR)\DebugCapture.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\debugcapture.exe"
	-@erase "$(OUTDIR)\debugcapture.ilk"
	-@erase "$(OUTDIR)\debugcapture.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libftd" /I "..\..\lib\liblic" /I "..\..\lib\liblic2" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\debugcapture.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\debugcapture.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBERRDBG) $(TOP)\$(LIBLSTDBG) kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBLICDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBFTDDBG) Ws2_32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\debugcapture.pdb" /debug /machine:I386 /out:"$(OUTDIR)\debugcapture.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\DebugCapture.obj"

"$(OUTDIR)\debugcapture.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("debugcapture.dep")
!INCLUDE "debugcapture.dep"
!ELSE 
!MESSAGE Warning: cannot find "debugcapture.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "debugcapture - Win32 Release" || "$(CFG)" == "debugcapture - Win32 Debug"
SOURCE=.\DebugCapture.c

"$(INTDIR)\DebugCapture.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

