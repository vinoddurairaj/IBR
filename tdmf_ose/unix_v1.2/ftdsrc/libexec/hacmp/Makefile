#
TOP=../..

BIN_INSTALL=${Q}modhacmp
SH_PROGRAM=${Q}modhacmp
SHSRC=modhacmp.sh
HACMP_INSTALL=./cl_activate_dtc ./cl_deactivate_dtc
EXTRA_CLEANS=./cl_activate_dtc ./cl_deactivate_dtc

include ${TOP}/mk/prog.gmk

modhacmp.sh: cl_activate_dtc cl_deactivate_dtc

cl_activate_dtc: cl_activate_dtc.sh
	${PKGIFY}

cl_deactivate_dtc: cl_deactivate_dtc.sh
	${PKGIFY}
