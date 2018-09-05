#
# Copyright (c) 2001 Fujitsu Software Technology Corporation. All rights reserved.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
# Government is subject to restrictions as set forth in
# subparagraph (c)(1)(ii) of the Rights in Technical Data and
# Computer Software clause at DFARS 52.227-7013 and in similar
# clauses in the FAR and NASA FAR Supplement.
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
