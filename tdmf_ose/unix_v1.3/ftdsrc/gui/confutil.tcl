#-----------------------------------------------------------------------------
# confutil.tcl -- 
#                 
#    Tcl utilities for GUI tools for working with 
#    configuration files, devices, processes.
#
# Copyright (c) 2001 Fujitsu Software Technology Corporation 
# All Rights Reserved.
#
#-----------------------------------------------------------------------------

#=============================================================================
# ftdmkdevlist --
#
#    Builds a global array (devlist-<sysname-or-ip>) containing information
#    about each disk device or volume for the targetted system.
#
# Arguments:
#    sysname -- the hostname of the system to query (or blank)
#    portnum -- port number that the RMD daemon is listening to at the
#               targetted system (defaults to 575)
# Results:
#    returns the name of the global array created.  This name has an
#    element named status, which may be 0=good, -1=bad host/port number, 
#    -2=operation failed in remote RMD.
#=============================================================================

proc ftdmkdevlist {sysname {portnum 575} {timeout 60000}} {
    global tcl_platform
    set SYS(platform) $tcl_platform(os)
    if {$sysname == ""} { return }
    set gn "devlist-$sysname"
    global $gn
    upvar #0 $gn d
    if {[info exists $gn]} {
	catch {unset d(names)}
	foreach i [array names d *..inuse] { catch "unset d($i)" }
	foreach i [array names d *..size] { catch "unset d($i)" }
	foreach i [array names d *..listentry] { catch "unset d($i)" }
    }
    set d(status) 0
    set d(errmsg) ""
    set d(portnum) $portnum
    set d(timeout) $timeout
    set d(timeoutid) ""
    set d(buffer) ""
    set d(done) 0
    if {[catch "socket $sysname $portnum" sock]} {
	set d(status) -1
	set d(errmsg) "Could not retrieve the device information from $sysname on port $portnum.  Either the master Softek %PRODUCTNAME% daemon process (in.%Q%) is not running on $sysname, or the port number has been specified incorrectly."
	return $gn
    }
    fconfigure $sock -translation {auto lf} -buffersize 10000
    fconfigure $sock -blocking on
    puts $sock "ftd get all devices"
    flush $sock
    fconfigure $sock -blocking off
    set d(timeoutid) [after $d(timeout) [list ftdreadtimeout $sock $gn]]
    fileevent $sock readable [list ftdreadfromsock $sock $gn]
    vwait ${gn}(done)
    if {$d(status) != 0} {return $gn}
    set devs $d(buffer)
    unset d(timeout)
    unset d(buffer)
    unset d(timeoutid)
    if {[llength $devs] < 1} {
	set d(status) -2
	set d(errmsg) "RMD server at port $portnum on $sysname failed device lookup"
	return $gn
    }
    set d(namewidth) 0
    foreach item [lrange $devs 1 end] {
	set n [lindex $item 0]
	if {[string length $n] > $d(namewidth)} {
	    set d(namwidth) [string length $n]
	}
	set u [lindex $item 1]
	set s [lrange $item 2 end]
	lappend d(names) $n
	set d($n..inuse) $u
	set d($n..size) $s
    }
    set d(names) [lsort $d(names)]
    set d(linewidth) 0
    foreach n $d(names) {
	if {$d($n..inuse)} {
	    set d($n..listentry) [format "%-[set d(namewidth)]s \[INUSE %s\]" $n $d($n..size)]
	} else {
	  set d($n..listentry) [format "%-[set d(namewidth)]s \[AVAIL %s\]" $n $d($n..size)]
          }

	if {[string length $d($n..listentry)] > $d(linewidth)} {
	    set d(linewidth) [string length $d($n..listentry)]
	}
    }
    return $gn
}

proc ftdreadfromsock {sock var} {
    global $var
    upvar #0 $var d
    set b [read $sock]
    if {[eof $sock] || [string length $b] == 0} {
	if {[string length $b] > 0} {append d(buffer) $b}
	catch "close $sock"
	catch "after cancel $d(timeoutid)"
	set d(done) 1
    } else {
	append d(buffer) $b
	catch "after cancel $d(timeoutid)"
	set d(timeoutid) [after $d(timeout) "ftdreadtimeout $sock $var"]
    }
}

proc ftdreadtimeout {sock var} {
    global $var
    upvar #0 $var d
    catch "after cancel $d(timeoutid)"
    set d(status) -1
    set d(errmsg) "Connection timed out."
    fileevent $sock readable {}
    catch "close $sock"
    set d(done) 1
}




#=============================================================================
# ftddevadd --
#
#    adds a device to a list of devices targetted as inuse on a system
#    This is used to check for double allocations or
#    conflicts.
#
# Arguments:
#    sysname -- name (or IP) of targetted system
#    devpath -- path of device to add (e.g. "/dev/rdsk/c0t0d0s0")
#    loggrp -- logical group number associated with this device allocation
#    ftddev -- device name associated with this device allocation
#    role -- role that device is taking on (e.g. LOCDATA, RMTMIRROR)
#    override -- non-zero value forces allocation even with conflict
#    start -- optional starting sector for allocation
#    stop -- optional ending sector for allocation
#
# Results:
#    Returns empty string if no conflict detected.  If a conflict detected,
#    returns list of loggrp, ftddev, and role of conflicting allocation
#
#=============================================================================
proc ftddevadd {sysname devpath loggrp ftddev role {override 0} {start 0} {stop 999999999}} {
    if {$start == ""} {set start 0}
    if {$stop == ""} {set stop 999999999}
    if {$start == 0 && $stop == 999999999} {
	set useincr 10000
    } else {
	set useincr 2
    }
    set gn "devlist-$sysname"
    global $gn
    upvar #0 $gn d
    if {![info exists $gn]} {catch "ftdmkdevlist $sysname"}
    set d(status) 0
    set d(errmsg) ""
    if {[info exists d(names)] && (![info exists d($devpath..size)]) && $override == 0} {
	set d(status) -2
	set d(errmsg) "device $devpath may not exist on $sysname"
	return -2
    }
    if {[info exists d(names)] && [info exists d($devpath..size)]} {
	set devsize [lindex $d($devpath..size) 3]
	if {$stop == 999999999} {
	    set stop [expr $devsize - 1]
	}
	if {$start >= $devsize && $override == 0} {
	    set d(status) -3
	    set d(errmsg) "$devpath start sector $start > device size $devsize"
	    return -3
	}
	if {$stop >= $devsize && $override == 0} {
	    set d(status) -3
	    set d(errmsg) "$devpath end sector $stop >= device size $devsize"
	    return -3
	}
    }
    if {[info exists d($devpath..inuse)]} {
	if {($d($devpath..inuse) == 1 || $d($devpath..inuse) >= 10000) && $override == 0} {
	    set d(status) -3
	    set d(errmsg) "Device $devpath is not available for use"
	    return -3
	}
    }
    if {[info exists d($devpath..cfg)]} {
	foreach entry $d($devpath..cfg) {
	    set eloggrp [lindex $entry 0]
	    set eftddev [lindex $entry 1]
	    set estart [lindex $entry 2]
	    set estop [lindex $entry 3]
	    set erole [lindex $entry 4]
	    if {($start >= $estart && $start <= $estop) || ($stop >= $estart && $stop <= $estop)} {
		if {$override == 0} {
		    set d(status) -3
		    set d(errmsg) "$devpath conflict: [format "dsgrp%03d" $eloggrp]:$eftddev/$erole"
		    return -3
		}
	    }
	}
    }
    lappend d($devpath..cfg) [list $loggrp $ftddev $start $stop $role] 
    set testv 1
    if {[info exists d(names)]} {
	if {-1 == [lsearch $d(names) $devpath]} {
	    set testv 1
	} else {
	    set testv 0
	}
    }
    if {$testv} {
	set d($devpath..inuse) $useincr
    } else {
	if {$override == 0 && ($d($devpath..inuse) == 1 || $d($devpath..inuse) == 2)} {
	    set d(status) -3
	    set d(errmsg) "$devpath is not available for use"
	    return -3
	}
	incr d($devpath..inuse) $useincr
	set d($devpath..listentry) [format "%-[set d(namewidth)]s \[%Q% %s\]" $devpath $d($devpath..size)]
    }
    return 0
}

#=============================================================================
# ftddevchk --
#
#    checks a device against a list of devices targetted as inuse on a system
#    This is used to check for double allocations or
#    conflicts.
#
# Arguments:
#    sysname -- name (or IP) of targetted system
#    devpath -- path of device to add (e.g. "/dev/rdsk/c0t0d0s0")
#    loggrp -- logical group number associated with this device allocation
#    ftddev -- Data device name associated with this device allocation
#    role -- role that device is taking on (e.g. LOCDATA, RMTMIRROR)
#    override -- non-zero value forces allocation even with conflict
#    start -- optional starting sector for allocation
#    stop -- optional ending sector for allocation
#
# Results:
#    Returns empty string if no conflict detected.  If a conflict detected,
#    returns list of loggrp, ftddev, and role of conflicting allocation
#
#=============================================================================
proc ftddevchk {sysname devpath loggrp ftddev role {override 0} {start 0} {stop 999999999}} {
    if {$start == ""} {set start 0}
    if {$stop == ""} {set stop 999999999}
    set gn "devlist-$sysname"
    global $gn
    upvar #0 $gn d
    if {![info exists $gn]} {
	catch "ftdmkdevlist $sysname"
    }
    set d(status) 0
    set d(errmsg) ""
    if {[info exists d(names)] && [info exists d($devpath..size)]} {
	set devsize [lindex $d($devpath..size) 3]
	if {$stop == 999999999} {
	    set stop [expr $devsize - 1]
	}
	if {$start >= $devsize && $override == 0} {
	    set d(status) -3
	    set d(errmsg) "$devpath start sector $start >= device size $devsize"
	    return -3
	}
	if {$stop >= $devsize && $override == 0} {
	    set d(status) -3
	    set d(errmsg) "$devpath end sector $stop >= device size $devsize"
	    return -3
	}
    }
    if {[info exists d(names)] && (![info exists d($devpath..size)]) && $override == 0} {
	set d(status) -2
	set d(errmsg) "device $devpath does not exist on $sysname"
	return -2
    }
    if {[info exists d($devpath..inuse)]} {
	if {($d($devpath..inuse) == 1 || $d($devpath..inuse) >= 10000) && $override == 0} {
	    set d(status) -2
	    set d(errmsg) "Device $devpath is not available for use"
	    return -2
	}
    }
    if {[info exists d($devpath..cfg)]} {
	foreach entry $d($devpath..cfg) {
	    set eloggrp [lindex $entry 0]
	    set eftddev [lindex $entry 1]
	    set estart [lindex $entry 2]
	    set estop [lindex $entry 3]
	    set erole [lindex $entry 4]
	    if {$start >= $estart && $start <= $estop} {
		if {$override == 0} {
		    set d(status) -3
		    set d(errmsg) "$devpath conflict: [format "dsgrp%03d" $eloggrp]:$eftddev/$erole"
		    return -3
		}
	    }
	    if {$stop >= $estart && $stop <= $estop} {
		if {$override == 0} {
		    set d(status) -3
		    set d(errmsg) "$devpath conflict: [format "dsgrp%03d" $eloggrp]:$eftddev/$erole"
		    return -3
		}
	    }
	}	
    } 
    return 0
}

#=============================================================================
# ftddevdel --
#
#    removes a device from a list of devices targetted as inuse on a system
#    This complements ftddevadd.
#
# Arguments:
#    sysname -- name (or IP) of targetted system
#    devpath -- path of device to add (e.g. "/dev/rdsk/c0t0d0s0")
#    start -- optional starting sector for allocation
#    stop -- optional ending sector for allocation
#
# Results:
#    Returns empty string.
#
#=============================================================================
proc ftddevdel {sysname devpath loggrp ftddev role {start 0} {stop 999999999}} {
    if {$start == ""} {set start 0}
    if {$stop == ""} {set stop 999999999}
    if {$start == 0 && $stop == 999999999} {
	set usedecr -10000
    } else {
	set usedecr -2
    }
    set gn "devlist-$sysname"
    global $gn
    upvar #0 $gn d
    set d(status) 0
    set d(errmsg) ""
    if {[info exists d($devpath..size)]} {
	set devsize [lindex $d($devpath..size) 3]
	if {$stop == 999999999} {
	    set stop [expr $devsize - 1]
	}
    }
    if {![info exists d($devpath..cfg)]} {
	set d(status) -3
	set d(errmsg) "device $devpath not in use on $sysname"
	return -3
    }
    set newent ""
    set found 0
    foreach entry $d($devpath..cfg) {
	set eloggrp [lindex $entry 0]
	set eftddev [lindex $entry 1]
	set estart [lindex $entry 2]
	set estop [lindex $entry 3]
	set erole [lindex $entry 4]
	if {$start != $estart || $stop != $estop || $role != $erole || \
	    $loggrp != $eloggrp || $ftddev != $eftddev} {
	    lappend newent $entry
	} else {
	    if {$found == 1} {lappend newent $entry}
	    set found 1
	}
    }
    if {[llength $newent] > 0} {
	set d($devpath..cfg) $newent
    } else {
	unset d($devpath..cfg)
    }
    if {!$found} {
	set d(status) -3
	set d(errmsg) "device $devpath \[start sector=$start end sector=$stop\] not found on $sysname"
	return -3
    }
    if {[info exists d($devpath..inuse)]} {
	incr d($devpath..inuse) $usedecr
	if {$d($devpath..inuse) < 0} {set d($devpath..inuse) 0}
	if {![info exists d($devpath..size)]} {return 0}
	if {$d($devpath..inuse) > 1} {return 0}
	if {$d($devpath..inuse) == 0} {
	    set d($devpath..listentry) [format "%-[set d(namewidth)]s \[AVAIL %s\]" $devpath $d($devpath..size)]
	} else {
	    set d($devpath..listentry) [format "%-[set d(namewidth)]s \[INUSE %s\]" $devpath $d($devpath..size)]
	}
    }
    return 0
}

#=============================================================================
# ftdmkproclist --
#
#    Builds a global array (proclist-<sysname-or-ip>) containing information
#    about each process found (indexed by PID) meeting search criteria on 
#    the targetted system.
#
# Arguments:
#    sysname -- the hostname of the system to query (or blank)
#    portnum -- port number that the RMD daemon is listening to at the
#               targetted system
#    prefix  -- leading characters (or empty for all processes) of process
#               names we're interested in.
# Results:
#    returns the name of the global array created.  This name has an
#    element named "status", which may be 0=good, -1=bad host/port number,
#    -2=operation failed in remote RMD.  The element "names" contains a
#    list of all process names returned.  The other elements are PID
#    numbers whose value is the name of the process for that PID.
#
#=============================================================================
proc ftdmkproclist {sysname {portnum 575} {prefix "*"}} {
    if {$sysname == ""} { return }
    set gn "proclist-$sysname"
    global $gn
    upvar #0 $gn d
    catch {unset d}
    set d(status) 0
    set d(portnum) $portnum
    if {[catch "socket $sysname $portnum" sock]} {
	set d(status) -1
	return $gn
    }
    puts $sock "ftd get all process info $prefix"
    flush $sock
    set devs [read $sock]
    set trash [read $sock]
    close $sock
    if {[llength $devs] <= 1} {
	set d(status) -2
	return $gn
    }
    foreach item [lrange $devs 1 end] {
	set n [lindex $item 0]
	set pid [lindex $item 1]
	lappend d(names) $n
	set d(pid..$pid) $n
    }
    return $gn
}

#=============================================================================
# ftdfindproc --
#
#    returns a list of process name and PID if a process by that name
#    is founnd on the targetted system.  Otherwise, it returns "0"
#    either to indicate that the process wasn't found or there was
#    some other error.
#
# Arguments:
#    sysname -- the hostname of the system to query (or blank)
#    prefix  -- leading characters (or empty for all processes) of process
#               names we're interested in.  The first one to match will
#               be returned.
#    portnum -- port number that the RMD daemon is listening to at the
#               targetted system
# Results:
#    On failure of any kind, or a no process by that name on the targetted
#    system, returns "0".  On success, returns a two element list with
#    the process name as the first element, and the PID of the process
#    as the second element.
#
#=============================================================================
proc ftdfindproc {sysname prefix {portnum 575}} {
    if {$sysname == "" && $sysip == ""} { return }
    if {$sysname != ""} {
	set h $sysname
    } else {
	set h $ip
    }
    if {[catch "socket $h $portnum" sock]} {
	return -1
    }
    puts $sock "ftd get process info $prefix"
    flush $sock
    set procs [read $sock]
    set trash [read $sock]
    close $sock
    if {[llength $procs] <= 1} {
	return -2
    }
    return [lindex $procs 1]
}
