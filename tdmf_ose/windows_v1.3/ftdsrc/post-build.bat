@Echo Off
REM Post Build commands
Echo Processing Parm1: %1
Echo Processing Parm2: %2 
Echo Processing Parm3: %3
Echo Processing Parm4: %4 
Echo Processing Parm5: %5 



REM Set the enviroment variable from the pre-build
call %1ftdsrc\pre-build.bat %3 %1

REM Make sure the directory exist.
REM Make
mkdir %1ftdsrc\%2\%3
REM Do the copies

if %5 == SoftekRes.dll goto Branding

copy %4 %1ftdsrc\%2\%3\%prefix%%5
goto end

:Branding
copy %4 %1ftdsrc\%2\%3\RBRes.dll
goto end

:end



