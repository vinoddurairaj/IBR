
TOP=../..

PROGRAMS=aixsymbols cfg${Q} ucfg${Q} def${Q} udef${Q} 
PROGRAMS+=${Q}.cat ${Q}.add ${Q}.delete ${Q}.get ${Q}-rcedit ${Q}-startdriver
PKGHDRS=drvodm.h

include ${TOP}/Makefile.inc

_LIBS=-lcfg -lodm ../../lib/libgen/libgen.a

# product name dep's
drvodm.h: drvodm.H
	$(PKGIFY)

# aix config methods
cfg${Q}: cfg.c drvodm.h
	$(CC) $(CFLAGS) -o $@ $< ${_LIBS}

ucfg${Q}: ucfg.c drvodm.h
	$(CC) $(CFLAGS) -o $@ $< ${_LIBS}

def${Q}: def.c drvodm.h
	$(CC) $(CFLAGS) -o $@ $< ${_LIBS}

udef${Q}: udef.c drvodm.h
	$(CC) $(CFLAGS) -o $@ $< ${_LIBS}

# aix config methods

${Q}.add:add
	make -f Makefile.add

${Q}.delete: delete
	make -f Makefile.delete

${Q}.get: get
	make -f Makefile.get

${Q}-rcedit: rcedit
	make -f Makefile.rcedit

${Q}-startdriver: startdriver
	make -f Makefile.startdriver


# crash(1) print function support:
#
# e.g.:
#
# $ CRASHSYM=`/bin/pwd`/aixsymbols ; export CRASHSYM
# $ crash dump unix
#
aixsymbols:
	cc $(CFLAGS) -g -o aixsymbols aixsymbols.c -qdbxextra


${Q}.cat: msg
	runcat ${Q} $< 

msg: product.msg
	cat product.msg | sed -e '\@%PRODUCTNAME%@s@@${PRODUCTNAME}@' > $@
	
