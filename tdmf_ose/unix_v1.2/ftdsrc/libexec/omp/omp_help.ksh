#!/usr/bin/ksh
########################################################################
#
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
#
# $Id: omp_help.ksh,v 1.2 2008/12/28 10:12:16 naat0 Exp $
#
# Function: Schedule and Execute Migrations
#
########################################################################
set -o nounset
umask 0

PROGNAME=${0##*/}

#
# Return codes
#

readonly EC_TRAP=8
readonly EC_FATAL=4
readonly EC_ERROR=2
readonly EC_WARN=1
readonly EC_OK=0

readonly OMPDIR_HOME=/etc/opt/SFTKomp
readonly OMPDIR_LOG=${OMPDIR_HOME}/log

readonly LogFile="${PROGNAME}.log"
readonly HelpTopicFile="omp_help.topic"
readonly HelpIndexFile="omp_help.index"
readonly HelpTextFile="omp_help.text"

readonly LogPath="${OMPDIR_LOG}/${LogFile}"
readonly tab=$(print \\t)

###############################################################################
#
# display - Displays a message
#
###############################################################################

typeset discard_output=""

function no_std_output
{
    if [[ -z "${discard_output}" ]]
    then
	discard_output="no_std"
    else
	discard_output="${discard_output} no_std"
    fi
}

function no_log_output
{
    if [[ -z "${discard_output}" ]]
    then
	discard_output="no_log"
    else
	discard_output="${discard_output} no_log"
    fi
}

function display
{
    typeset message
    typeset -i i
    typeset msgtype func linenum parm

    [[ $# < 3 ]] && {
	set -- TooFewArgs display ${LINENO} "$@"
    }

    msgtype=$1; shift
    func=$1; shift
    linenum=$1; shift

    case ${msgtype} in
	NotRootUser)
	    severity="ERROR"
	     message[0]="You must be super user to run this program."
	    ;;
	BadTopic)
	    severity="ERROR"
	     message[0]="No help found for topic \"$1\"."
	    ;;
	IndexError)
	    severity="ERROR"
	     message[0]="Topic \"$1\" not found in the help index file."
	    ;;
	ExpectedBegin)
	    severity="ERROR"
	     message[0]="Topic \"$1\" not found in the help text file."
	    ;;
	NoDocsFound)
	    severity="ERROR"
	     message[0]="Cannot locate the help text file."
	    ;;
	Usage)
	    severity="USAGE"
	    message[0]="${PROGNAME} [topics]..." 
	    ;;
	TooFewArgs)
	    severity="ERROR"
	    message[0]="Too few arguments passed to function \"${func}\"." 
	    (( i=1 ))
	    for parm in "$@"
	    do
		if (( i == 1 ))
		then
		    message[1]="parm${i}=\"${parm}\""
		else
		    message[1]="${message[1]} parm${i}=\"${parm}\""
		fi

		(( i++ ))
	    done
	    ;;
	*)
	    severity="ERROR"
	    message[0]="Message key \"${msgtype}\" is not defined."
	    msgtype="MissingMsg"
	    (( i=1 ))
	    for parm in "$@"
	    do
		if (( i == 1 ))
		then
		    message[1]="parm${i}=\"${parm}\""
		else
		    message[1]="${message[1]} parm${i}=\"${parm}\""
		fi
		(( i++ ))
	    done
	    ;;
    esac
    if (( ${#message[*]} > 0 ))
    then
        (( i = 0 ))
        while (( i < ${#message[*]} ))
        do
	    [[ "${discard_output}" != *no_std* ]] && {
		if [[ "${severity}" = USAGE ]]
		then
		    print "${message[i]}"
		elif [[ "${severity}" = ?(INFO|WARNING) ]]
		then
		    print "${severity}:" ${message[i]} "[${msgtype}]"
		else
		    print "${severity}:" ${message[i]} "[${msgtype} ${func} ${linenum}]"
		fi
	    }
	    [[ "${discard_output}" != *no_log* && -w ${OMPDIR_LOG} ]] && {
		print $(date '+[%Y.%m.%d--%T]') "${PROGNAME}[$$]" "${severity}:" ${message[i]} "[${msgtype} ${func} ${linenum}]" >>${LogPath}
	    }
            (( i += 1 ))
        done
    fi
    discard_output=""
}

if [[ -f "./${HelpTextFile}" ]]
then
    readonly OMPDIR_DOCS=.
elif [[ -f "${OMPDIR_HOME}/docs/${HelpTextFile}" ]]
then
    readonly OMPDIR_DOCS=${OMPDIR_HOME}/docs
else
    display NoDocsFound main ${LINENO}
    exit $(( EC_ERROR ))
fi

readonly HelpTopicPath="${OMPDIR_DOCS}/${HelpTopicFile}"
readonly HelpIndexPath="${OMPDIR_DOCS}/${HelpIndexFile}"
readonly HelpTextPath="${OMPDIR_DOCS}/${HelpTextFile}"

function get_helpkey
{
    typeset topic topiclist helpkey

    topic=":$1:"
    while read helpkey topiclist
    do
	[[ ${helpkey} = \#* ]] && continue
	topiclist=":${topiclist}:"
	[[ "${topiclist}" = *${topic}* ]] && {
	    print "${helpkey}"
	    return 0
	}
    done < ${HelpTopicPath}
    return 1
}

function print_helptext
{
    typeset linenum
    typeset keyword=$1
    typeset needsep=$2

    IFS=":"
    set -- $(grep ":${keyword}[ ${tab}]*\$" ${HelpIndexPath})
    IFS=
    if [[ $# -ne 2 ]]
    then
	display IndexError ${keyword}
	return 1
    fi
    (( linenum = $1 ))

    if (( HelpTextLineNum > linenum ))
    then
	exec 3<&-
	exec 3<${HelpTextPath}
	(( HelpTextLineNum=0 ))
    fi
    while (( HelpTextLineNum < linenum ))
    do
	read -u3 line
	(( HelpTextLineNum++ ))
    done

    [[ "${line}" = \#begin+([\ ${tab}])${keyword}*([\ ${tab}]) ]] || {
	print "${line}"
	display ExpectedBegin ${keyword}
    }
    [[ ${needsep} -ne 0 ]] && print "\n\n"

    while read -u3 line
    do
	(( HelpTextLineNum++ ))
	[[ "${line}" = \#end+([\ ${tab}])${keyword}*([\ ${tab}]) ]] && {
	    return 0
	}
	print "${line}"
    done
    display UnexpectedEOF ${keyword}
    return 1
}

[[ ${HelpIndexPath} -ot ${HelpTextPath} || ${HelpIndexPath} -ot ${HelpTopicPath} ]] && {
    grep -n '^#begin' ${HelpTextPath} | sed -e '/#begin[\ \t][\ \t]*/s///' > ${HelpIndexPath}
}

[[ $# -le 0 ]] && {
    set -- usage
}

[[ -o xtrace || -n "${OMP_DEBUG-}" ]] && {
    typeset -ft $(typeset +f)
}

for topic in ${@}
do
    helpkey=$(get_helpkey ${topic}) 
    if [[ $? -eq 0 ]]
    then
	keywords="${keywords-} $(IFS=\":\"; set -- ${helpkey}; print ${@})"
    else
	display BadTopic main ${LINENO} ${topic}
	exit $(( EC_ERROR )) 
    fi
done

(( HelpTextLineNum=0 ))
(( AddSeparator=0 ))
exec 3<${HelpTextPath}
for textkey in ${keywords}
do
    print_helptext ${textkey} ${AddSeparator}
    (( AddSeparator=1 ))
done
