#! /sbin/sh 
#
#  bab_rebuild.sh
#
#  For HP-UX 10.20.
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
#########################################################################
#
# This script generates a source file containing a buffer that resides 
# in BSS, compiles the source file, archives the object file into a 
# library, and rebuilds the kernel. The /stand/system file presumably 
# includes the library with the BSS buffer, therefore adding the buffer 
# to the kernel. The kernel rebuild procedure saves the old kernel and
# replaces it with the new one. A reboot will activate the new kernel.
#
# This script requires two arguments: 1) a basename for the library and 
# BSS buffer, and 2) the size of the buffer in bytes.
#
#########################################################################
Usage_exit()
{
    echo "Usage:  $CMD -b <base_name> -s <bab_size>"
    exit $FAILURE
}
Qualify_environment ()
{

    typeset -i cmd_result
    typeset path

    for path in        \
        $CONFIG                \
        $MAKE
    do
        whence $path > /dev/null
        cmd_result=$?
        if [[ $cmd_result -ne 0 ]]
        then
            echo "ERROR:   Cannot find \"$path\" on the system."
            echo "         This command is necessary to build a kernel."
            Usage_exit
        fi
    done
    if [[ ! -f ${KERNLIBS}/libhp-ux.a ]]
    then
        echo "ERROR:   Cannot find \"${KERNLIBS}/libhp-ux.a\" on the system."
        echo "         This file is necessary to build a kernel."
        Usage_exit
    fi

    if [[ ! -f $SYSPATH ]]
    then
        if [[ -e $SYSPATH ]]
        then
            echo "ERROR:   \"$SYSPATH\" must be an ordinary file."
        else
            echo "ERROR:   Cannot find file \"$SYSPATH\"."
        fi
        echo "         This file is necessary in order to build a kernel."
        Usage_exit
    fi
    if [[ -e $KERNEL_TARGET && ! -f $KERNEL_TARGET ]]
    then
        echo "ERROR:   Cannot place a kernel in \"$KERNEL_TARGET\"."
        echo "         It must either not exist or else exist as an ordinary file."
        Usage_exit
    fi

    return $SUCCESS
} 
Save_vmunix ()
{
    retval=$SUCCESS
    
    #
    # we don't want to overwrite the original kernel
    #
    if [ -f ${KERNEL_TARGET}.pre%CAPQ% ]
    then
        KERNEL_TARGET_SVD=${KERNEL_TARGET}.%CAPQ%bak
    else
        KERNEL_TARGET_SVD=${KERNEL_TARGET}.pre%CAPQ%
    fi

    if [[ -e ${KERNEL_TARGET_SVD} ]]
    then
        rm -rf ${KERNEL_TARGET_SVD}
    fi

    if [[ -f $KERNEL_TARGET ]]
    then
        mv $KERNEL_TARGET ${KERNEL_TARGET_SVD}
        retval=$?
        if [[ $retval -ne $SUCCESS ]]
        then
            echo "WARNING: Could not move the current kernel from ${KERNEL_TARGET}"
            echo "         to ${KERNEL_TARGET_SVD}"
            retval=$WARNING
        fi
    fi
    return $retval
}
Move_new_kernel ()
{
    retval=$SUCCESS
    Save_vmunix
    if [[ -e $KERNEL_OUTPATH ]]
    then
        mv ${KERNEL_OUTPATH} ${KERNEL_TARGET}
        retval=$?
        if [[ $retval -ne $SUCCESS ]]
        then
            echo "ERROR:   Could not move the newly built kernel from"
            echo "         ${KERNEL_OUTPATH} to"
            echo "         ${KERNEL_TARGET}."
            echo "         The newly built kernel remains in ${KERNEL_OUTPATH}."
            retval=$FAILURE
        fi
    else
        echo "ERROR:   Could not move the newly built kernel from"
        echo "         ${KERNEL_OUTPATH} to"
        echo "         ${KERNEL_TARGET}."
        echo "         Could not find ${KERNEL_OUTPATH}."
        retval=$FAILURE
    fi
    return $retval
}
Parse_cmd()
{
    while getopts b:s:p: arg
    do
    case $arg in
    b)  BASE_NAME=$OPTARG
        ;;
    s)  BAB_SIZE=$OPTARG
        ;;
    p)  MBPOOL_SIZE=$OPTARG
        ;;
    \?) Usage_exit
        ;;
    esac
    done 2> /dev/null
#
    ((arg_cnt=OPTIND-1))
    if [[ $arg_cnt -ne $# ]]
    then
    echo "$CMD:  Incorrect number of arguments."
    Usage_exit
    fi
#
    if [[ $arg_cnt -ne 6 ]]
    then
    Usage_exit
    fi
#
    return $SUCCESS
}


##################################################################
# newBuild & rebuild modify SZLIMIT in /usr/conf/gen/config.sys
##################################################################
## newBuild:  config.sys has not been touch ##
newBuild()
{
   # backup original $CONFIGSYS to $CONFIGSYS.preDTC
   cp $CONFIGSYS $CONFIGSYS.preDTC

   # get current_size, new_size and line#
   line=`sed -n -e '/^SZLIMIT/=' $CONFIGSYS`
   start=`echo ${line} | awk '{ print $1 }'`
   end=`echo ${line} | awk '{ print $'NF' }'`
   csize=`cat $CONFIGSYS | awk 'NR == '$end' { print $3 }'`
   nsize=`echo "${BAB_SIZE} $csize ${MBPOOL_SIZE}" |\
          awk ' { printf "%d", ($1 + $2 + $3) } '`
   sed 's/^SZLIMIT/# SZLIMIT/' $CONFIGSYS > $CONFIGSYS.tmp

   # Build SEDTMP for sed -f
   echo "${start}i\\" > $SEDTMP
   echo "###### Modify %Q%SZLIMIT ######" >> $SEDTMP
   echo "${end}a\\" >> $SEDTMP
   echo "SZLIMIT= -l $nsize\\" >> $SEDTMP
   echo "###### Modify %Q%SZLIMIT" >> $SEDTMP
}


## rebuild: rebuild new BAB size
rebuild()
{
   # Save current $CONFIGSYS
   cp $CONFIGSYS $CONFIGSYS.old

   # get original_size, current_size, new_size and line#
   osize=`cat $CONFIGSYS | awk '/^# SZLIMIT/ { print $4 } '`
   csize=`cat $CONFIGSYS | awk '/^SZLIMIT/ { print $3 } '`
   nsize=`echo "${BAB_SIZE} $csize ${MBPOOL_SIZE}" |\
          awk ' { printf "%d", ($1 + $2 + $3) } '`
   line=`cat $CONFIGSYS | sed -n -e '/^SZLIMIT/='`

   # Build SEDTMP for sed -f
   echo "${line}s/$csize/$nsize/" > $SEDTMP
}



#
# main
#
    CMD=${0##*/}
    PATH=/usr/lbin/sw/bin:/usr/bin:/usr/ccs/bin:/sbin:/usr/sbin:/usr/lbin/sw
    KERNLIBS="/usr/conf/lib"
    SUCCESS=0
    FAILURE=1
    WARNING=2

    exitval=$SUCCESS
#
    CONF="/usr/conf"
    MASTER="${CONF}/master.d"
    KERNLIBS=${CONF}/lib

    SYSPATH=/stand/system
    KERNEL_OUTFILE=vmunix_test
    KERNEL_BUILDDIR=/stand/build
    KERNEL_OUTPATH=${KERNEL_BUILDDIR}/${KERNEL_OUTFILE}
    KERNEL_TARGET=/stand/vmunix
    CONFIG="config"
    MAKE="make"

    OUTPUT_FILE=${KERNEL_BUILDDIR}/bab.o 
    INPUT_FILE=${KERNEL_BUILDDIR}/bab.c 
#
#   WARNING: This value must match that defined in ../../driver/ftd_def.h
#
    Parse_cmd $@

    Qualify_environment


    # Modify /usr/conf/gen/config.sys
    # check if CONFIGSYS is touched by SofTek
    SEDTMP=/tmp/mod_config.sed
    CONFIGSYS=/usr/conf/gen/config.sys
    grep "%Q%SZLIMIT" ${CONFIGSYS} > /dev/null 2>&1
    if [ $? -ne 0 ]
    then
       # new build
       newBuild
       echo "\nModify SZLIMIT in ${CONFIGSYS}.\n" 
       sed -f $SEDTMP $CONFIGSYS.tmp > $CONFIGSYS
       rm $CONFIGSYS.tmp
    else
        # rebuild
       rebuild
       echo "\nModify SZLIMIT in ${CONFIGSYS}.\n" 
       cat $CONFIGSYS | sed -f $SEDTMP > $CONFIGSYS.tmp
       mv $CONFIGSYS.tmp $CONFIGSYS
    fi
    rm $SEDTMP

    OUTPUT_LIB=${KERNLIBS}/lib${BASE_NAME}.a
    COMPILE_COMMAND="cc -c +DAportable +DS899 -o ${OUTPUT_FILE} ${INPUT_FILE}"
    ARCHIVE_COMMAND="ar r ${OUTPUT_LIB} ${OUTPUT_FILE}"
#
    echo "#include <sys/buf.h>" > $INPUT_FILE
    echo "unsigned long long big_ass_buffer[${BAB_SIZE}/8];" >> $INPUT_FILE
    echo "int big_ass_buffer_size = ${BAB_SIZE};" >> $INPUT_FILE
#
# compile our source file
#
    $COMPILE_COMMAND 2> /dev/null
    cmd_result=$?
    if [[ $cmd_result -ne 0 ]]
    then
        echo "ERROR: Compile failure"
        exit $FAILURE
    fi
    rm -f $INPUT_FILE
#
# add the object to our library
#
    $ARCHIVE_COMMAND 2> /dev/null
    if [[ $cmd_result -ne 0 ]]
    then
        echo "ERROR: Archive failure"
        exit $FAILURE
    fi
    rm -f $OUTPUT_FILE
#
# rebuild the kernel, save the old one, and replace old with new
#
    echo "Building a new kernel based on template file \"${SYSPATH}\""

    $CONFIG -m $MASTER $SYSPATH
    exitval=$?
    if [[ $exitval -ne $SUCCESS ]]
    then
        echo "         $CONFIG failure."
        exit $FAILURE
    else
        Move_new_kernel
        exitval=$?
    fi

    if [[ $exitval -eq $SUCCESS ]]
    then
        echo "Kernel rebuild succeeded."
        echo "Reboot this system to start the new kernel."
    fi

    exit $exitval
