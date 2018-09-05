#!/usr/bin/ksh
# LICENSED MATERIALS / PROPERTY OF IBM
#
# Offline Migration Package (OMP)   Version %VERSION%
#
# (c) Copyright %COMPANYNAME%  %COPYRIGHTYEAR 2007%.  All Rights Reserved.
#
# The source code for this program is not published or otherwise
# divested of its trade secrets, irrespective of what has been
# deposited with the U.S. Copyright Office.
#
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with %COMPANYNAME%

while true
do

    exec 3>&1
    result=$(dialog.new --title "OMP" \
		--backtitle "%COMPANYNAME2% OMP" \
		--menu "Choose a menu item and press enter:" 0 50 8 \
		quit "Exit Menu" \
		scan "Discover Devices" \
		choose "Determine Source and Target Devices" \
		prepare "Create Migration Schedule" \
		begin "Execute Migraion" \
		done "Cleanup" 2>&1 1>&3)
    retcode=$?
    exec 3>&-
    print "retcode=" ${retcode} " result=" ${result}
    case ${retcode} in
	0)
	    case ${result} in
		quit)
		    exit 0
		    ;;
		scan)
		    omp_getscsidsk
		    ;;
		choose)
		    omp_selectctlr
		    ;;
		prepare)
		    omp_genconf
		    ;;
		begin)
		    omp_admin start
		    ;;
		done)
		    echo TBD
		    ;;
	    esac
	    ;;
	1|255)	# Cancel or ESC
	    exit 1
	    ;;
	2)	# Help
	    ;;
	3)	# Extra Help
	    ;;
	*)
	    exit 2
	    ;;
    esac
    read junk?"Press any key to continue"
done
