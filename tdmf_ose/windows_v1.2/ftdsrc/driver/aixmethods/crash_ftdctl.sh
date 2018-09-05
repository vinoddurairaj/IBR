#!/bin/sh

# use the crash(1) program to analyze the ftd_global_state struct

core=/var/adm/ras/vmcore.*
nlist=""

# prime the environment
if [ ! -f `/bin/pwd`/aixsymbols ]
then
	echo "Need aixsymbols...";
	exit 1;
fi
CRASHSYM=`/bin/pwd`/aixsymbols ; export CRASHSYM;

# extract address of ftd_global_state ptr
crshcmds=./crshcmds;
cat > ${crshcmds} << EOCRSHCMDS
knlist ftd_global_state
quit
EOCRSHCMDS
eval `cat ${crshcmds} | crash ${core} ${nlist} 2> /dev/null | \
                        grep ftd_glob |\
                        sed -e '\@ @s@@@g' |\
                        awk -F: '{printf "ftd_global_state=%s",$2}' `;

# check whether nil sym
tst_addr=`echo ${ftd_global_state} | sed -e '\@0x@s@@@' | tr "[a-f]" "[A-F]"`
tst_addr=`echo "10o 16i ${tst_addr}p" | dc`
if [ ${tst_addr} -eq 0 ]
then
	echo "ftd_global_state (nil)";
	exit 1;
fi


# display ftd_global_state struct 
crshtmp=./crshtmp
cat > ${crshcmds} << EOCRSHCMDS
pr ftd_ctl *${ftd_global_state}
quit
EOCRSHCMDS
cat ${crshcmds} | crash  ${core} ${nlist} 2> /dev/null |\
                  egrep -v "(^WARN|system crash)" > ${crshtmp};

echo "ftd_global_state struct:";
echo "knlist symbol ftd_global_state = ${ftd_global_state}";
cat ${crshtmp}

# extract base address of logical group structs
eval ` cat ${crshtmp} | grep lghead |\
                        sed -e '\@ @s@@@g' -e '\@;@s@@@' |\
                        awk -F= '{printf "lghead=%s",$2}'`
   

# dump logical group structs
echo ""
echo "ftd logical group structs:";
echo "knlist symbol ftd_global_state->lghead = ${lghead}";

# check whether nil sym
tst_addr=`echo ${lghead} | sed -e '\@0x@s@@@' | tr "[a-f]" "[A-F]"`
tst_addr=`echo "10o 16i ${tst_addr}p" | dc`
if [ ${tst_addr} -eq 0 ]
then
	echo "lghead (nil)";
	exit 1;
fi

# run list of ftd_lg structs
crshtmp=./crshtmp
cat > ${crshcmds} << EOCRSHCMDS
pr -l next ftd_lg ${lghead}
quit
EOCRSHCMDS
cat ${crshcmds} | crash 2> /dev/null |\
                  egrep -v "(^WARN|system crash)" > ${crshtmp};
cat ${crshtmp};

rm -f ${crshcmds} ${crshtmp};

exit 0;
