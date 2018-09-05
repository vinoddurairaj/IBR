# Microsoft Developer Studio Generated NMAKE File, Based on reco.dsp
!IF "$(CFG)" == ""
CFG=reco - Win32 Debug
!MESSAGE No configuration specified. Defaulting to reco - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "reco - Win32 Release" && "$(CFG)" != "reco - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "reco.mak" CFG="reco - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "reco - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "reco - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "reco - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\reco.exe"


CLEAN :
	-@erase "$(OUTDIR)\reco.exe"
	-@erase ".\Debug\ftd_reco.obj"
	-@erase ".\Debug\info.pdb"
	-@erase ".\Debug\vc60.idb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /GX /Z7 /Od /I "..\..\lib\liblic" /I "..\..\lib\libftd" /I "..\..\driver" /I "..\..\lib\libutil" /I "..\..\lib\liberr" /I "..\..\lib\liblst" /I "..\..\lib\libproc" /I "..\..\lib\libcomp" /I "..\..\lib\libsock" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" /Fp"Debug/info.pch" /YX /Fo"Debug/" /Fd"Debug/" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\reco.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBPROCREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBERRREL) $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBCOMPREL) $(TOP)\$(LIBLSTREL) $(TOP)\$(LIBMD5REL) ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"Debug/info.pdb" /debug /debugtype:both /machine:I386 /out:"$(OUTDIR)\reco.exe" /pdbtype:sept /opt:ref,icf 
LINK32_OBJS= \
	".\Debug\ftd_reco.obj"

"$(OUTDIR)\reco.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\Release\reco.exe
TargetName=Reco
SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\reco.exe"
   md C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"
	copy .\Release\reco.exe C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"\lrreco.exe
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "reco - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\reco.exe"


CLEAN :
	-@erase "$(INTDIR)\ftd_reco.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\reco.exe"
	-@erase "$(OUTDIR)\reco.ilk"
	-@erase "$(OUTDIR)\reco.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\liblic" /I "..\..\lib\libftd" /I "..\..\driver" /I "..\..\lib\libutil" /I "..\..\lib\liberr" /I "..\..\lib\liblst" /I "..\..\lib\libproc" /I "..\..\lib\libcomp" /I "..\..\lib\libsock" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\info.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\reco.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBPROCDBG) $(TOP)\$(LIBFTDDBG) $(TOP)\$(LIBERRDBG) $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBCOMPDBG) $(TOP)\$(LIBLSTDBG) $(TOP)\$(LIBMD5DBG) ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\reco.pdb" /debug /machine:I386 /out:"$(OUTDIR)\reco.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\ftd_reco.obj"

"$(OUTDIR)\reco.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("reco.dep")
!INCLUDE "reco.dep"
!ELSE 
!MESSAGE Warning: cannot find "reco.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "reco - Win32 Release" || "$(CFG)" == "reco - Win32 Debug"
SOURCE=.\ftd_reco.c

!IF  "$(CFG)" == "reco - Win32 Release"


".\Debug\ftd_reco.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "reco - Win32 Debug"


"$(INTDIR)\ftd_reco.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

