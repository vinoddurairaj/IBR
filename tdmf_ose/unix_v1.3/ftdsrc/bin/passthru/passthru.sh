#
# Copyright (c) 2001 Fujitsu Software Technology Corporation. All rights reserved.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
# Government is subject to restrictions as set forth in
# subparagraph (c)(1)(ii) of the Rights in Technical Data and
# Computer Software clause at DFARS 52.227-7013 and in similar
# clauses in the FAR and NASA FAR Supplement.
#
#
#! /bin/sh

usage()
{
    echo "usage: %Q%override [-a] [-g n] [on|off]"
    exit 1
}

args=
for i do
    case $i in
    on) state=passthru; break ;;
    off) state=normal; break ;;
    *) args="$args $i";;
    esac
done
if [ x"$args" = x -o x$state = x ]; then
    usage
fi
echo %Q%override $args state=$state
