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
# $Id: Makefile,v 1.1 2009/02/11 05:58:32 naat0 Exp $
#
# Function: Execute Administrative Commands
#
########################################################################
TOP=../..

OMPBIN_INSTALL= omp_admin omp_cleanup omp_genconf.awk omp_genconf \
		omp_getscsidsk omp_help omp_matchup.awk omp_matchup \
		omp_monitor.awk omp_monitor omp_sched omp_selectctlr \
		omp_setup omp_showsched.awk omp_verifysched
OMPDOCS_INSTALL= omp_help.topic omp_help.text
OMPPROFILE_INSTALL= omp.csh omp.sh

SHSRC=  omp_admin.ksh omp_cleanup.ksh omp_genconf.awk.brand omp_genconf.ksh \
	omp_getscsidsk.ksh omp_help.ksh omp_matchup.awk.brand omp_matchup.ksh \
	omp_monitor.awk.brand omp_monitor.ksh omp_sched.ksh omp_selectctlr.ksh \
	omp_setup.ksh omp_showsched.awk.brand omp_verifysched.ksh \
	omp_help.text.brand omp_help.topic.brand \
	omp.csh.brand omp.sh.brand

SH_PROGRAM= $(basename ${SHSRC})

include ${TOP}/mk/prog.gmk

omp_help.text omp_help.topic : PKGIFY_MODE=666
