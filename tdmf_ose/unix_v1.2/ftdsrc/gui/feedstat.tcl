#
# LICENSED MATERIALS / PROPERTY OF IBM
# %PRODUCTNAME% version %VERSION%
# (c) Copyright %COMPANYNAME%  %COPYRIGHTYEAR 2001%.  All Rights Reserved.
# The source code for this program is not published or otherwise
# divested of its trade secrets, irrespective of what has been
# deposited with the U.S. Copyright Office.
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with %COMPANYNAME%
#
#
#!/usr/local/bin/wish4.2
wm withdraw .
catch "exec /bin/rm /var/opt/QLIXds/dsgrp000.prf"
set fd [open ./dsgrp000.prf r]
set tfd [open /var/opt/QLIXds/dsgrp000.prf w]
while {1} {
	set line [gets $fd]
	if {[eof $fd]} {break} 
	puts $tfd $line
	flush $tfd
#	after 10000
	after 2000
}
close $fd
close $tfd
exit
