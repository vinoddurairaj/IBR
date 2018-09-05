# Microsoft Developer Studio Generated NMAKE File, Based on libproc.dsp
!IF "$(CFG)" == ""
CFG=libproc - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libproc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libproc - Win32 Release" && "$(CFG)" != "libproc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libproc.mak" CFG="libproc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libproc - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libproc - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "libproc - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\libproc.lib"


CLEAN :
	-@erase "$(INTDIR)\proc.obj"
	-@erase "$(INTDIR)\procinfo.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\libproc.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /I "..\..\lib\liblic" /I "..\..\lib\libftd" /I "..\..\lib\libutil" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libcomp" /I "..\..\lib\libproc" /I "..\..\lib\liberr" /I "..\..\driver" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /Fp"$(INTDIR)\libproc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libproc.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libproc.lib" 
LIB32_OBJS= \
	"$(INTDIR)\proc.obj" \
	"$(INTDIR)\procinfo.obj"

"$(OUTDIR)\libproc.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libproc - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\libproc.lib"


CLEAN :
	-@erase "$(INTDIR)\proc.obj"
	-@erase "$(INTDIR)\procinfo.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\libproc.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\liblic" /I "..\..\lib\libftd" /I "..\..\lib\libutil" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libcomp" /I "..\..\lib\libproc" /I "..\..\lib\liberr" /I "..\..\driver" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\libproc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libproc.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libproc.lib" 
LIB32_OBJS= \
	"$(INTDIR)\proc.obj" \
	"$(INTDIR)\procinfo.obj"

"$(OUTDIR)\libproc.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("libproc.dep")
!INCLUDE "libproc.dep"
!ELSE 
!MESSAGE Warning: cannot find "libproc.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libproc - Win32 Release" || "$(CFG)" == "libproc - Win32 Debug"
SOURCE=.\proc.c

"$(INTDIR)\proc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\procinfo.c

"$(INTDIR)\procinfo.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

