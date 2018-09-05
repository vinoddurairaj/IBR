# Microsoft Developer Studio Generated NMAKE File, Based on LibLic2.dsp
!IF "$(CFG)" == ""
CFG=LibLic2 - Win32 Debug
!MESSAGE No configuration specified. Defaulting to LibLic2 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "LibLic2 - Win32 Release" && "$(CFG)" != "LibLic2 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LibLic2.mak" CFG="LibLic2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LibLic2 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "LibLic2 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "LibLic2 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\LibLic2.lib"


CLEAN :
	-@erase "$(INTDIR)\LicAPI.obj"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\octolic.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\LibLic2.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /Z7 /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\LibLic2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\LibLic2.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\LibLic2.lib" 
LIB32_OBJS= \
	"$(INTDIR)\LicAPI.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\octolic.obj"

"$(OUTDIR)\LibLic2.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "LibLic2 - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\LibLic2.lib"


CLEAN :
	-@erase "$(INTDIR)\LicAPI.obj"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\octolic.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\LibLic2.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\LibLic2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\LibLic2.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\LibLic2.lib" 
LIB32_OBJS= \
	"$(INTDIR)\LicAPI.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\octolic.obj"

"$(OUTDIR)\LibLic2.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("LibLic2.dep")
!INCLUDE "LibLic2.dep"
!ELSE 
!MESSAGE Warning: cannot find "LibLic2.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "LibLic2 - Win32 Release" || "$(CFG)" == "LibLic2 - Win32 Debug"
SOURCE=.\LicAPI.c

"$(INTDIR)\LicAPI.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\md5c.c

"$(INTDIR)\md5c.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\octolic.c

"$(INTDIR)\octolic.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

