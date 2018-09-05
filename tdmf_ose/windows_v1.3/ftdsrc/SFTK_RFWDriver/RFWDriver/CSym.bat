@echo off
REM ECHO ***************************************************************************
REM ECHO Command: CSym.bat <[free | f | FREE | fre | FRE] | [checked | c | CHECKED | chk | CHK]>
REM ECHO This Batch Files Creates numega debug symbol files (.nms) for all NT 
REM ECHO and Win2k based generated sys, exe, and dll files. This Batch file must 
REM ECHO run on windows sytem where all softek virtualization source code resides for
REM ECHO generation of nms file to debug DCS files in numega softice debugger.
REM ECHO         Command Line :  "Csym.bat"
REM ECHO WARNING: This batch file assumes that project path is "D:\Project\Softek"
REM ECHO          Directory. In case of different location of softek project, modify
REM ECHO		  Dos environment varible PROJECT_PATH in this batch file.
REM ECHO ***************************************************************************


SET SRCPATH=%2

SET OUTPUTPATH="%3."
echo "Mode = %1 and source path = %2 and outpath = %3"

set TARGET_PATH=

for %%f in (free f FREE fre FRE) do if %%f == %1 set mode=free
for %%f in (checked c CHECKED chk CHK) do if %%f == %1 set mode=checked

if "%mode%"=="free"		set TARGET_PATH="%OUTPUTPATH%\i386\sftkBlk.sys"
if "%mode%"=="checked"	set TARGET_PATH="%OUTPUTPATH%\i386\sftkBlk.sys"

SET NT_DDK_INC_PATH=%BASEDIR%\Inc

SET NMS_PATH="%DRIVERWORKS%\Bin\NmSym.exe"
REM ECHO PROJECT_PATH is set to %PROJECT_PATH%

REM SET MSDEV_INC_PATH0="C:\Program Files\Microsoft Visual Studio\VC98\CRT\SRC"
REM SET MSDEV_INC_PATH1="C:\Program Files\Microsoft Visual Studio\VC98\CRT\SRC\INTEL"

REM Defualt : MSDevDir = C:\Program Files\Microsoft Visual Studio\Common\MSDev98

REM SET MSDEV_INC_PATH0="%MSDevDir%\..\..\VC98\CRT\SRC"
SET MSDEV_INC_PATH0="%VS71COMNTOOLS%..\..\VC7\CRT\SRC"
REM SET MSDEV_INC_PATH1="%MSDevDir%\..\..\VC98\CRT\SRC\INTEL"


ECHO ---------------------------------------------------------------------------
ECHO BUILD Numega NMS Symbol Files For your driver 
ECHO ---------------------------------------------------------------------------

:DcsCache_sys
%NMS_PATH% /TRANSLATE:ALWAYS,SOURCE,PACKAGE /SOURCE:%SRCPATH%;%NT_DDK_INC_PATH%;%MSDEV_INC_PATH0%;%MSDEV_INC_PATH0%\INTEL	%TARGET_PATH%

@echo on