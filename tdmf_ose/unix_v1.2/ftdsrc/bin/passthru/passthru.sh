#
# LICENSED MATERIALS / PROPERTY OF IBM
# %PRODUCTNAME% version %VERSION%
# (c) Copyright %COMPANYNAME%  %COPYRIGHTYEAR 2001%.  All Rights Reserved.
# The source code for this program is not published or otherwise
# divested of its trade secrets, irrespective of what has been
# deposited with the U.S. Copyright Office.
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with %COMPANYNAME%
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
