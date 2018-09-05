# Microsoft Developer Studio Generated NMAKE File, Based on libutil.dsp
!IF "$(CFG)" == ""
CFG=libutil - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libutil - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libutil - Win32 Release" && "$(CFG)" != "libutil - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libutil.mak" CFG="libutil - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libutil - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libutil - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "libutil - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\libutil.lib"


CLEAN :
	-@erase "$(INTDIR)\bitmap.obj"
	-@erase "$(INTDIR)\diskkey.obj"
	-@erase "$(INTDIR)\disksize.obj"
	-@erase "$(INTDIR)\error.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\ntfsinfo.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\volume.obj"
	-@erase "$(OUTDIR)\libutil.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /I "..\..\lib\libftd" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /Fp"$(INTDIR)\libutil.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libutil.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libutil.lib" 
LIB32_OBJS= \
	"$(INTDIR)\bitmap.obj" \
	"$(INTDIR)\diskkey.obj" \
	"$(INTDIR)\disksize.obj" \
	"$(INTDIR)\error.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\ntfsinfo.obj" \
	"$(INTDIR)\volume.obj"

"$(OUTDIR)\libutil.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libutil - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\libutil.lib"


CLEAN :
	-@erase "$(INTDIR)\bitmap.obj"
	-@erase "$(INTDIR)\diskkey.obj"
	-@erase "$(INTDIR)\disksize.obj"
	-@erase "$(INTDIR)\error.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\ntfsinfo.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\volume.obj"
	-@erase "$(OUTDIR)\libutil.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libftd" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\libutil.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libutil.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libutil.lib" 
LIB32_OBJS= \
	"$(INTDIR)\bitmap.obj" \
	"$(INTDIR)\diskkey.obj" \
	"$(INTDIR)\disksize.obj" \
	"$(INTDIR)\error.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\ntfsinfo.obj" \
	"$(INTDIR)\volume.obj"

"$(OUTDIR)\libutil.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("libutil.dep")
!INCLUDE "libutil.dep"
!ELSE 
!MESSAGE Warning: cannot find "libutil.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libutil - Win32 Release" || "$(CFG)" == "libutil - Win32 Debug"
SOURCE=.\bitmap.c

"$(INTDIR)\bitmap.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\diskkey.c

"$(INTDIR)\diskkey.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\disksize.c

"$(INTDIR)\disksize.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\error.c

"$(INTDIR)\error.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getopt.c

"$(INTDIR)\getopt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\misc.c

"$(INTDIR)\misc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ntfsinfo.c

"$(INTDIR)\ntfsinfo.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\volume.c

"$(INTDIR)\volume.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

