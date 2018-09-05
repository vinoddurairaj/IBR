# Microsoft Developer Studio Generated NMAKE File, Based on liblst.dsp
!IF "$(CFG)" == ""
CFG=liblst - Win32 Debug
!MESSAGE No configuration specified. Defaulting to liblst - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "liblst - Win32 Release" && "$(CFG)" != "liblst - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "liblst.mak" CFG="liblst - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "liblst - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "liblst - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "liblst - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\liblst.lib"


CLEAN :
	-@erase "$(INTDIR)\alist.obj"
	-@erase "$(INTDIR)\alistadd.obj"
	-@erase "$(INTDIR)\alistdel.obj"
	-@erase "$(INTDIR)\alistins.obj"
	-@erase "$(INTDIR)\alistsrt.obj"
	-@erase "$(INTDIR)\llist.obj"
	-@erase "$(INTDIR)\llistadd.obj"
	-@erase "$(INTDIR)\llistdel.obj"
	-@erase "$(INTDIR)\llistsrt.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\liblst.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /Fp"$(INTDIR)\liblst.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\liblst.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\liblst.lib" 
LIB32_OBJS= \
	"$(INTDIR)\alist.obj" \
	"$(INTDIR)\alistadd.obj" \
	"$(INTDIR)\alistdel.obj" \
	"$(INTDIR)\alistins.obj" \
	"$(INTDIR)\alistsrt.obj" \
	"$(INTDIR)\llist.obj" \
	"$(INTDIR)\llistadd.obj" \
	"$(INTDIR)\llistdel.obj" \
	"$(INTDIR)\llistsrt.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\liblst.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "liblst - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\liblst.lib"


CLEAN :
	-@erase "$(INTDIR)\alist.obj"
	-@erase "$(INTDIR)\alistadd.obj"
	-@erase "$(INTDIR)\alistdel.obj"
	-@erase "$(INTDIR)\alistins.obj"
	-@erase "$(INTDIR)\alistsrt.obj"
	-@erase "$(INTDIR)\llist.obj"
	-@erase "$(INTDIR)\llistadd.obj"
	-@erase "$(INTDIR)\llistdel.obj"
	-@erase "$(INTDIR)\llistsrt.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\liblst.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /Fp"$(INTDIR)\liblst.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\liblst.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\liblst.lib" 
LIB32_OBJS= \
	"$(INTDIR)\alist.obj" \
	"$(INTDIR)\alistadd.obj" \
	"$(INTDIR)\alistdel.obj" \
	"$(INTDIR)\alistins.obj" \
	"$(INTDIR)\alistsrt.obj" \
	"$(INTDIR)\llist.obj" \
	"$(INTDIR)\llistadd.obj" \
	"$(INTDIR)\llistdel.obj" \
	"$(INTDIR)\llistsrt.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\liblst.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("liblst.dep")
!INCLUDE "liblst.dep"
!ELSE 
!MESSAGE Warning: cannot find "liblst.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "liblst - Win32 Release" || "$(CFG)" == "liblst - Win32 Debug"
SOURCE=.\alist.c

"$(INTDIR)\alist.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\alistadd.c

"$(INTDIR)\alistadd.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\alistdel.c

"$(INTDIR)\alistdel.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\alistins.c

"$(INTDIR)\alistins.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\alistsrt.c

"$(INTDIR)\alistsrt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\llist.c

"$(INTDIR)\llist.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\llistadd.c

"$(INTDIR)\llistadd.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\llistdel.c

"$(INTDIR)\llistdel.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\llistsrt.c

"$(INTDIR)\llistsrt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\..\version.c

"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

