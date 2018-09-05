-- Nicolas Druet - Oct 2008 --

This file documents how to update copyrights header using the IBM tool called COD (Copyrights On Demand)

The current version used is 3.0 and teh jar file is into the cvs repository, so there is no need to download the tool.
(if needed the tool can be found at http://web.opensource.ibm.com/www/cod)

The only files needed are : COD.jar and some definition files under Defs directory.

DMC Product code is : 6949-34J

What to do to update copyright header from one release to another (changing year for example)
---------------------------------------------------------------------------------------------

1. Edit the LaunchCOD.bat file and change the 'year' parameter (or leave it blank as default is current year)
The launch command should be something like :
java -jar cod.jar -year 2008 -verbose -defs Defs -root ..\..
/!\ : Java 5 or higher is required.

2. Execute the LaunchCOD.bat and click run to update.


To add a new file type
----------------------
Under Defs, build a new *.copyright file using the extention of new file type and build the header templates.

