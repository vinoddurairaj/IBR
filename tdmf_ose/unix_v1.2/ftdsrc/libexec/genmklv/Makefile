MKLVPROG?=mklv

TOP=../..
SYSTYPE:= $(shell uname -s)
SH_PROGRAM=genmklv
SHSRC=genmklv.sh
EXTRA_CLEANS=softek_cmdlvm.msg softek_cmdlvm.cat genmklv.sh genmklv.awk dtcmklv
ETC_INSTALL=softek_cmdlvm.msg softek_cmdlvm.cat
BIN_INSTALL=genmklv genmklv.awk

include ${TOP}/mk/prog.gmk

genmklv.awk: genmklv.awk.$(SYSTYPE)
	${PKGIFY}
	chmod 755 $@

genmklv.sh: genmklv.sh.${SYSTYPE} softek_cmdlvm.cat genmklv.awk
	@sed \
	 -e '\@%MKLV%@s@@${MKLVPROG}@g' \
	 < $< >$@

softek_cmdlvm.msg: softek_cmdlvm.msg.${SYSTYPE}
	${PKGIFY}

softek_cmdlvm.cat: softek_cmdlvm.msg
	gencat $@ $<

localtest: all
	@rm -f genmklv
	@echo Making genmklv from genmklv.sh.$(SYSTYPE);
	sed \
	 -e '\@%MKLV%@s@@${MKLVPROG}@g' \
	 < genmklv.sh.$(SYSTYPE) >genmklv.sh
	sed \
	 -e '\@%FTDBINDIR%@s@@${PWD}@g' \
	 -e '\@%ETCOPTDIR%@s@@${PWD}@g' \
	 -e '\@%USRSBINDIR%@s@@${PWD}@g' \
	 -e '\@%ROOTED%@s@@@g' \
	 -e '\@%Q%@s@@dtc@g' genmklv.sh > genmklv
	 chmod 755 genmklv genmklv.awk
	sed \
	 -e '\@%FTDBINDIR%@s@@${PWD}@g' \
	 -e '\@%ETCOPTDIR%@s@@${PWD}@g' \
	 -e '\@%USRSBINDIR%@s@@${PWD}@g' \
	 -e '\@%ROOTED%@s@@@g' \
	 -e '\@%Q%@s@@dtc@g' softek_cmdlvm.msg.AIX > softek_cmdlvm.msg
	 gencat softek_cmdlvm.cat softek_cmdlvm.msg
