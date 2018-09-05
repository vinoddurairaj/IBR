#!/bin/sh

# collect output from fake make
# throwout anything that isn't ${CC}
# build cproto commands file
CC=/opt/SUNWspro/bin/cc
CPROTOOPTS="-s -e -DFTD_CPROTO" 
CPROTO="/usr/local/bin/cproto ${CPROTOOPTS}"
mk=/usr/local/bin/make
mkout=./mkcproto.out.0.$$
${mk} clean 1> /dev/null 2>&1
${mk} -n -f ./Makefile |\
	grep "${CC}" |\
	sed -e "\@${CC}@ s@@${CPROTO}@" |\
	sed -e "\@-Xa@ s@@@" |\
	sed -e "\@-xO2@ s@@@" |\
	sed -e "\@-c@ s@@@" |\
	awk '{ printf "%s %s\n", $0, "2> /dev/null | /usr/local/bin/ctypes"}' \
	  1> ${mkout} 
	  
