#!/bin/sh

# use the crash(1) program to run a list of bab buffer structs

# prime the environment
if [ ! -f `/bin/pwd`/aixsymbols ]
then
	echo "Need aixsymbols...";
	exit 1;
fi
CRASHSYM=`/bin/pwd`/aixsymbols ; export CRASHSYM;

# extract address of free buffer chain
crshcmds=./crshcmds;
cat > ${crshcmds} << EOCRSHCMDS
knlist free_bab_buffers
quit
EOCRSHCMDS
eval `cat ${crshcmds} | crash 2> /dev/null | \
                        grep free_bab |\
                        sed -e '\@ @s@@@g' |\
                        awk -F: '{printf "buflst=%s",$2}' `;

echo knlist symbol free_bab_buffers = ${buflst};

# check whether nil sym
tst_addr=`echo ${buflst} | sed -e '\@0x@s@@@' | tr "[a-f]" "[A-F]"`
tst_addr=`echo "10o 16i ${tst_addr}p" | dc`
if [ ${tst_addr} -eq 0 ]
then
	echo "free_bab_buffers (nil)";
	exit 1;
fi
	

# display list of free buffers
cat > ${crshcmds} << EOCRSHCMDS
pr -l 0 _bab_buffer_ *${buflst}
quit
EOCRSHCMDS
cat ${crshcmds} | crash 2> /dev/null | egrep -v "(^WARN|system crash)";

rm -f ${crshcmds};

exit 0;
