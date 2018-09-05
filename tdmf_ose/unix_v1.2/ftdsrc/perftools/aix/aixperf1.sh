#!/bin/sh
#
# filename: aixperf1.sh
#
######################################################################
# AIX related network performance statistic gathering
######################################################################

LANG=C
export LANG

work_file="/var/tmp/aixperf1.out"

# zero out network related statistics
#
/usr/bin/netstat -Zc
/usr/bin/netstat -Zi
/usr/bin/netstat -Zm
/usr/bin/netstat -Zs

##### make sure all volume groups are ready to be fully refreshed
##### BAD initialied, dtcstart -a ...

/usr/dtc/bin/dtcrefresh -fa 

# netpmon detail info
# make sure that netpmon, trcstop, and libtrace.a are installed
#
/usr/bin/netpmon -o ${work_file} -O all

# sllep for ~600 secs or 10 mins 
sleep 600
#sleep 10

# stop netpmon capture
/usr/bin/trcstop

## who is on the system
echo  >> ${work_file}
echo "===== who are on the system ? =====" >> ${work_file}
who >> ${work_file}

# new dtcdebugcapture ...
#
./dtcdebugcapture_new

/usr/bin/cat <<EOF

Please ftp/email the AIX network performance output file 
${work_file} to
your vendor or reseller of Replicator support organization.

EOF

exit 0
