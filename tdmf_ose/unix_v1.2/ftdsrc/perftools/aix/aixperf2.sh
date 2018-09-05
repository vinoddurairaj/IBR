#!/bin/sh
#
# filename: aixperf2.sh
#
######################################################################
# AIX related network performance statistic gathering
######################################################################

LANG=C
export LANG

work_file="/var/tmp/aixperf2.out"

# zero out network related statistics
#
/usr/bin/netstat -Zc
/usr/bin/netstat -Zi
/usr/bin/netstat -Zm
/usr/bin/netstat -Zs

##### make sure all volume groups are ready to be fully refreshed
##### BAD initialied, dtcstart -a ...

/usr/dtc/bin/dtcrefresh -fa 

# sleep for a while to pass init effect ...
sleep 5

# IP trace
# make sure that iptrace, startsrc, stopsrc, and ipreport are installed
#

/usr/bin/startsrc -s iptrace -a ${work_file}_tmp

# sllep for ~300 secs or 5 mins 
sleep 300

# stop IP tracing ...
/usr/bin/stopsrc -s iptrace 

# format the output of IPtrace log
/usr/sbin/ipreport -ns  ${work_file}_tmp > ${work_file}

/usr/bin/cat <<EOF

Please ftp/email the AIX network performance output file 
${work_file} to
your vendor or reseller of Replicator support organization.

EOF

exit 0
