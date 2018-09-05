#!/usr/bin/ksh

function numericosversion
{
    set -- $(IFS='.'; set -- $1; print "$@")

    if [[ ${OSTYPE} = "hpux" ]]
    then
	shift
    fi
    typeset nv="$1$2$3$4"
    if [[ $# -le 1 || $# -gt 4 || "${nv}" != +([0-9]) || "${nv}" -le 0 ]]
    then
	exit 1
    fi

    print ${nv}
}

typeset -l OSTYPE=$(uname)
typeset -l ARCH

case "${OSTYPE}"  in
    aix)
	OSVERSION=$(oslevel)
	NUMOSVERSION=$(numericosversion ${OSVERSION})
	;;
    hp-ux)
	OSTYPE="hpux"
	ARCH="pa"
	[[ $(uname -m) = "ia64" ]] && ARCH=ipf
	OSVERSION=$(uname -r)
	NUMOSVERSION=$(numericosversion ${OSVERSION})
	;;
    sunos)
	OSTYPE="solaris"
	OSVERSION=$(uname -r)
	NUMOSVERSION=$(numericosversion ${OSVERSION})
	;;
    linux)
	ARCH="$(uname -m)"
	OSVERSION=$(expr `uname -r` : '\([0-9][.][0-9]\)')
	NUMOSVERSION=$(numericosversion ${OSVERSION})
	;;
    *)
	exitcode=5
	exit $exitcode
	;;
esac
print ${OSTYPE}${NUMOSVERSION}${ARCH}
