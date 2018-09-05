#! /bin/sh
############################################################################
#
#  dtc_post_failover_sxxx.sh
#  Target (secondary) server post-failover script
#
#  Make a copy of this empty script and replace the xxx part of its
#  name by the 3-digit group number of the group for which the failover
#  is launched.
#  Then insert in the script the necessary instructions for operations
#  to be done after the failover sequence ON THE TARGET (SECONDARY) SERVER.
#
############################################################################

# WARNING: please make sure your final script has the Executable
#          permission set (chmod if necessary)

# IMPORTANT: you must return the proper status code:
# Return 1 wherever an error occurs
# Return 0 at the end if success
exit 1

