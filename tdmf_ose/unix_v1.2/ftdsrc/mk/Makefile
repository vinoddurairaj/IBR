TOP=..

BUILDBRAND=$(shell ${TOP}/mk/getosarch.sh)
ifeq (${BUILDBRAND},)
$(error Unable to collect the operating system architecture information)
endif

ifeq ($(wildcard ${TOP}/mk/vars_tdmf_${BUILDBRAND}.gmk),)
include ${TOP}/mk/vars.gmk
else
include ${TOP}/mk/vars_tdmf_${BUILDBRAND}.gmk
endif

all: vars.pm vars.sh

clean:
	@rm -f vars.pm vars.sh

install:

vars.sh:: vars.gmk

vars.pm:: vars.gmk

vars.pm:: vars.pm.src
	${PKGIFY}

vars.sh:: vars.sh.src
	${PKGIFY}
