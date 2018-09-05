rem ********************************************************* {COPYRIGHT-TOP} ***
rem IBM Confidential
rem OCO Source Materials
rem 6949-32F - Softek Replicator for Unix and 6949-32K - Softek TDMF (IP) for Unix
rem
rem
rem (C) Copyright IBM Corp. 2006, 2011  All Rights Reserved.
rem The source code for this program is not published or otherwise  
rem divested of its trade secrets, irrespective of what has been 
rem deposited with the U.S. Copyright Office.
rem ********************************************************* {COPYRIGHT-END} ***

rem example : "%JRE_HOME%\bin\java" -jar cod.jar -verbose  -defs defs -root ..\.. -exclude ..\..\dir_to_exclude

echo Applying copyrights for Agent...

call java -jar cod.jar -verbose -defs defs -root ..\bin\


pause