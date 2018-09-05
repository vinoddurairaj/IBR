@echo off
REM ECHO ***************************************************************************
REM ECHO This Batch Files Creates numega debug symbol files (.nms) for all NT 
REM ECHO and Win2k based generated sys, exe, and dll files. This Batch file must 
REM ECHO run on windows sytem where all softek virtualization source code resides for
REM ECHO generation of nms file to debug DCS files in numega softice debugger.
REM ECHO         Command Line :  "Csym.bat"
REM ECHO WARNING: This batch file assumes that project path is "D:\Project\Softek"
REM ECHO          Directory. In case of different location of softek project, modify
REM ECHO		  Dos environment varible PROJECT_PATH in this batch file.
REM ECHO ***************************************************************************

SET SRCPATH=C:\asyncio\newbab\newbab
SET TARGET_PATH=C:\asyncio\newbab\newbab

SET NT_DDK_INC_PATH=C:\asyncio\newbab\newbab\NETDDK\3790\inc\ddk\w2k

SET PATH=%PATH%;C:\Program Files\NuMega\SoftICE Driver Suite\SoftICE;
REM ECHO PROJECT_PATH is set to %PROJECT_PATH%

SET MSDEV_INC_PATH0="C:\Program Files\Microsoft Visual Studio\VC98\CRT\SRC"
SET MSDEV_INC_PATH1="C:\Program Files\Microsoft Visual Studio\VC98\CRT\SRC\INTEL"

ECHO ---------------------------------------------------------------------------
ECHO BUILD Numega NMS Symbol Files For your driver 
ECHO ---------------------------------------------------------------------------

:Build_Sym_File
nmsym /TRANSLATE:ALWAYS,SOURCE,PACKAGE /SOURCE:%SRCPATH%;%NT_DDK_INC_PATH%;%MSDEV_INC_PATH0%	%TARGET_PATH%
REM nmsym /PROMPT /TRANSLATE:ALWAYS,SOURCE,PACKAGE /SOURCE:%SRCPATH%;%NT_DDK_INC_PATH%;%MSDEV_INC_PATH0%	%TARGET_PATH%\diskperf.sys

@echo on