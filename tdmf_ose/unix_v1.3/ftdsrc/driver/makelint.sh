#!/bin/sh

# collect output from fake make
# throwout anything that isn't ${CC}
# build lint commands file
CC=/opt/SUNWspro/bin/cc
LINTOPTS="-errchk=%all -errhdr=%user -errfmt=macro -Nlevel=1 -DFTD_LINT"
LINT="/opt/SUNWspro/bin/lint ${LINTOPTS}"
mk=/usr/local/bin/make
mkout=./mklint.out.0.$$
${mk} clean 1> /dev/null 2>&1
${mk} -n -f ./Makefile |\
	grep "${CC}" |\
	sed -e "\@${CC}@ s@@${LINT}@" |\
	awk '{ printf "echo \"FILE: %s\"\n", $NF ; printf "%s\n", $0}' \
	  1> ${mkout} 2>&1
