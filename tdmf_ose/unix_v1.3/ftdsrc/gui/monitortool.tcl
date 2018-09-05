#!/%FTDLIBEXECDIR%/%Q%wish
#-----------------------------------------------------------------------------
#
# monitortool -- Health / Info Monitoring tool
#
# Copyright (c) 2001 %COMPANYNAME%
# All Rights Reserved.
#
#-----------------------------------------------------------------------------

global env
if {![info exists env(DISPLAY)]} {
    puts stderr "DISPLAY environment variable is not set"
    exit
}
set env(FTD_LIBRARY) /%FTDLIBDIR%/%Q%%REV%
set env(FTD_PROGRAMS) /%BINDIR%
set monitor(debugon) 0
set monitor(chklock) 0
set monitor(timeout) 30000
set monitor(msgs_retained) 200.0
set monitor(lastsecs) -1
if {$tcl_platform(os) == "HP-UX"} {
    set services "/etc/services"
} elseif {$tcl_platform(os) == "SunOS"} {
    set services "/etc/inet/services"
} elseif {$tcl_platform(os) == "AIX"} {
    set services "/etc/services"
}
set services /etc/services
global argv argc argv0
if {$argc > 0} {
    set i 0
    while {$i < $argc} {
	switch -glob -- [lindex $argv $i] {
	    "-ti*" {
		incr i
		catch "expr [lindex $argv $i] * 1000" monitor(timeout)
		incr i
    }
	    "-mes*" {
		incr i
		catch "expr [lindex $argv $i] * 1.0" monitor(msgs_retained)
		incr i
	    }
	    default {
		puts stderr "Usage:"
		puts stderr "  $argv0 \[-timeout <seconds>\] \[-messages <count>\]"
		puts stderr "  $argv0 -help"
		puts stderr "default values:"
		puts stderr "  -timeout [expr $monitor(timeout) / 1000]"
		puts stderr "  -messages [expr int($monitor(msgs_retained))]"
		puts stderr ""
		exit
	    }
	}
    }
    unset i
}

global monitorode

set monitorode(PASSTHRU) 2
set monitorode(BACKFRESH) 4
set monitorode(NORMAL) 0
set monitorode(TRACKING) 1 
set monitorode(REFRESH) 3
# -----------------------------------------------------------------------
proc dev_compare {dev1 dev2} {
    regexp {%Q%([0-9]*)} $dev1 matchvar dev1num
    regexp {%Q%([0-9]*)} $dev2 matchvar dev2num
    if { $dev1num < $dev2num} {
        return -1
    } elseif { $dev1num == $dev2num } {
        return 0
    } else {
      return 1
    }
}
#------------------------------------------------------------------------
proc dev_sort {devlist} {    
    return [lsort -integer -command dev_compare $devlist]
}
# -----------------------------------------------------------------------
proc do_exit {} {
    exit
}
# -----------------------------------------------------------------------
proc dputs {txt} {
    global monitor
    if {$monitor(debugon)} {
        puts stdout $txt
        flush stdout
    }
}

# -----------------------------------------------------------------------
proc monitor_check_info {{autoreschedule 1}} {
    global monitor env prev_access last_access
    set lgname ""
    monitor_display_info "" OFF
    set monitor(alertflag) 0
    #print_call_stack
    while {$monitor(chklock)} {
        after 200 "monitor_check_info $autoreschedule"
        return
    }
      
    set monitor(chklock) 1
    # -- find out which groups have been started 
    set monitor(startedgroups) [glob -nocomplain /%OURCFGDIR%/p*.cur]
    set monitor(stoppedgroups) {}
    set allcfgs [glob -nocomplain /%OURCFGDIR%/p*.cfg]
    foreach cfgf $allcfgs {
        # -- get corresponding .cur file for this .cfg 
        regsub {/%OURCFGDIR%/p([0-9][0-9][0-9])\.cfg}\
                $cfgf {/%OURCFGDIR%/p\1.cur} curfile
        # -- does this file exist?
        if {![file exists $curfile]} {
            set len [string length $curfile]
            set lgname [string range $curfile [expr $len - 8] [expr $len - 5]]
            remove_group $monitor(w) $lgname
        }
    }
    # -- find corresponding .prf files for each .cur file 
    regsub -all {/%OURCFGDIR%/p([0-9][0-9][0-9])\.cur} $monitor(startedgroups) {/%FTDVAROPTDIR%/p\1.prf} monitor(prffiles)
 
   
    set groups_to_update {}
    foreach prf_file $monitor(prffiles) {
        set len [string length $prf_file]
        set grpname [string range $prf_file [expr $len - 8] \
            [expr $len - 5]]
        if {![info exists last_access($grpname)]} {
            set last_access($grpname) 0
        }
        if {![info exists prev_access($grpname)]} {
            set prev_access($grpname) -1
        }
        set count_mtime 0
        while {[catch "file mtime $prf_file" accesstime]} {
            incr count_mtime
            if {$count_mtime > 5} {
                set monitor(chklock) 0
                after 20 "monitor_check_info $autoreschedule"
                return
            }
            after 20
        } 
        set prev_access($grpname) $last_access($grpname)
        set last_access($grpname) $accesstime
        if {$last_access($grpname) != $prev_access($grpname)} {
            #
            # .prf file updated, update this group
            # 
            lappend groups_to_update $grpname
        }
    }
    if {[info exists groups_to_update]} {
        if {$groups_to_update == ""} {
            set monitor(chklock) 0
            after 1000 "monitor_check_info $autoreschedule"
            return
        }
    }
    
    monitor_display_info "Updating..." ON
  
    if {![info exists monitor(tsnow)]} {
	set monitor(tsnow) [expr [clock seconds] - 1]
    }
    set monitor(tsold) $monitor(tsnow)
    set monitor(tsnow) [clock seconds]
    if {$monitor(tsnow) == $monitor(tsold)} {
	set monitor(tsold) [expr $monitor(tsnow) - 1]
    }
    set monitor(delta_ts) [expr $monitor(tsnow) - $monitor(tsold)]
    set monitor(lgfound) {}

    foreach lgname $groups_to_update {
        # -- look at each logical group
        set lgnum [string range $lgname 1 end]
    
        set lowinterval 1
        global $lgname
        upvar #0 $lgname cfg
        
        if {[info global $lgname] != ""} {
            #
            # the newprf flag will be set if we happen to read
            # the .prf file when it is empty.  This avoids a problem
            # in which devices momentarily disappear from the group
            # frame
            #
            if {![info exists cfg(newprf)]} {
                set cfg(newprf) 0
            }
            set cfg(missingcount) 0
       
            if {!$cfg(newprf)} {
                foreach dev $cfg(ftddevs) {
                    if {-1 == [lsearch $cfg(devsfound) $dev]} {
                        global $dev
                        upvar #0 $dev ftd
                        catch "destroy $ftd(w)"
                        foreach name [array names $dev] {
                            unset ${dev}($name)
                        }
                        unset $dev
                    }
                }
                set cfg(ftddevs) [dev_sort $cfg(devsfound)]
                set cfg(devsfound) {}
            } else {
                set cfg(newprf) 0
            }
        }
	lappend monitor(lgfound) $lgname
        set monitor(lgfound) [lsort $monitor(lgfound)]
        if {![info exists $lgname]} {
            array set $lgname "
            ftddevs        {}
            drvmode        {1}
            entries         0
            pctbab          0.0
            devsfound      {}
            pmdalive        0
            rmdalive        0
            eKBps_out       0.0
            KBps_out        0.0
            w              {}
            name           $lgname
            "
            lappend monitor(loggrps) $lgname
            set monitor(loggrps) [lsort $monitor(loggrps)]

            if { [llength $monitor(loggrps) ] == 1 } {
                monitor_create_group_area $monitor(w) $lgname
            } else {
                set i [lsearch $monitor(loggrps) $lgname]
                if {$i == 0} {
                    set algname [lindex $monitor(loggrps) 1]
                    global $algname
                    upvar #0 $algname acfg
                    monitor_create_group_area $monitor(w) $lgname \
                            BEFORE $acfg(w)
                } else {
                    set algname [lindex $monitor(loggrps) \
                            [expr $i - 1]]
                            global $algname
                    upvar #0 $algname acfg
                    monitor_create_group_area $monitor(w) $lgname \
                            AFTER $acfg(w)
                }
            } 
            
            monitor_readconfig $lgname
           
            global $cfg($cfg(ptag),HOST:)
            upvar #0 $cfg($cfg(ptag),HOST:) system
            if {![info exists $cfg($cfg(ptag),HOST:)]} {
                set system(isprimary) 1
                set system(issecondary) 0
                lappend system(pmdloggrps) $lgname
                set system(rmdloggrps) {}
                set system(pmdloggrpsfound) {}
                set system(rmdloggrpsfound) {}
                set system(errmsgs) {}
                set system(erroffset) -1
                set system(host) $cfg($cfg(ptag),HOST:)
                set system(port) $cfg(LOCRMDPORT:)
                lappend monitor(systems) $cfg($cfg(ptag),HOST:)
            } else {
                set system(isprimary) 1
                lappend system(pmdloggrps) $lgname
            }
            global $cfg($cfg(rtag),HOST:)
            upvar #0 $cfg($cfg(rtag),HOST:) system
            if {![info exists $cfg($cfg(rtag),HOST:)]} {
                set system(isprimary) 0
                set system(issecondary) 1
                set system(pmdloggrps) {}
                lappend system(rmdloggrps) $lgname
                set system(pmdloggrpsfound) {}
                set system(rmdloggrpsfound) {}
                set system(errmsgs) {}
                set system(erroffset) -1
                set system(host) $cfg($cfg(rtag),HOST:)
                set system(port) $cfg(RMTRMDPORT:)
                lappend monitor(systems) $cfg($cfg(rtag),HOST:)
            } else {
                set system(issecondary) 1
                lappend system(rmdloggrps) $lgname
            }
	    monitor_change_transferring [set ${lgname}(w)].f.daemon \
            $lgname PMD
            monitor_change_transferring [set ${lgname}(w)].f.daemon \
                    $lgname RMD
            monitor_change_kbpsout [set ${lgname}(w)].f.kbpsout $lgname
            monitor_change_drvmode [set ${lgname}(w)].f.drvmode $lgname
            monitor_change_ekbpsout [set ${lgname}(w)].f.ekbpsout $lgname
            monitor_change_entries [set ${lgname}(w)].f.entries $lgname
            monitor_change_pctbab [set ${lgname}(w)].f.pctbab $lgname
        } else {
            # -- logical group has been seen before
            monitor_readconfig $lgname
        }   

        # - look at the prf file for this group for dev info
        set prfpath "/%FTDVAROPTDIR%/$lgname.prf"
        if {![catch "open $prfpath r" prffile]} {
            catch "file size $prfpath" size
            if {$size > 0} {
		catch "read $prffile" devprfbuf
                if {$devprfbuf == ""} {
                    set cfg(newprf) 1
                    close $prffile
                    continue
                }
                close $prffile
                
                # -- get last line
                set ftddevlines [split $devprfbuf "\n"]
                set i [expr [llength $ftddevlines ] - 2 ]
                set alldevsline [lindex $ftddevlines $i ]
                set lgdevlist [ split $alldevsline "||"]
                set lgdevlist [ lrange $lgdevlist 2  end]
                
                # 
                # The devsum array will be used to sum up all of the device
                # value and then it will be copied to the lg array.  
                # Since we will have a trace on the lg array we only 
                # want to set it once, so we use this array for the sum.
                #
                set devsum(entries) 0
                set devsum(pctdone) 0
                set devsum(pctbab) 0
                set devsum(eKBps_out) 0
                set devsum(KBps_out) 0
                set devsum(read_kbps) 0
                set devsum(write_kbps) 0
                
                for {set devindex 0 } { $devindex < [llength $lgdevlist] }\
                        {incr devindex} {
                    
                    set devline [lindex $lgdevlist $devindex]
                    if {$devline == "" } {
                        continue
                    }
                    
                    set ftddevicepath [lindex $devline 0]
                    
                    set ftddev [lindex [file split $ftddevicepath] end]_$lgname
                    lappend cfg(devsfound) $ftddev
                    if {-1 == [lsearch $cfg(ftddevs) $ftddev]} {
                        lappend cfg(ftddevs) $ftddev
                        set cfg(ftddevs) [dev_sort $cfg(ftddevs)]
                    }
                    global $ftddev
                    upvar #0 $ftddev ftd
                    
                    set ftd(devpath) $ftddevicepath
                    set ftd(KBps_out) [lindex $devline 1]
                    set ftd(eKBps_out) [lindex $devline 2]
                    set ftd(entries) [lindex $devline 3]
                    set ftd(pctdone) [lindex $devline 5]
                    set ftd(pctbab) [lindex $devline 6]
                    set ftd(drvmode) [lindex $devline 7]
                    set ftd(read_kbps) [lindex $devline 8]
                    set ftd(write_kbps) [lindex $devline 9]
                    
                    # 
                    # create the status bar for this device if needed
                    #
                    if {![info exists ftd(created_status_bar)]} {
                        if {[llength $cfg(ftddevs)] == 1} {
                            monitor_create_ftd_status_bar $cfg(w) \
                                    $lgname $ftddev
                        } else {
                            set t [lsearch $cfg(ftddevs) $ftddev]
                            if {$t > 0} {
                                set prevdev [lindex $cfg(ftddevs) \
                                        [expr $t - 1 ]]
                                global $prevdev
                                upvar #0 $prevdev pftd
                                monitor_create_ftd_status_bar $cfg(w) \
                                        $lgname $ftddev AFTER $pftd(w)
                            } else {		    
                                set afterdev [lindex $cfg(ftddevs) \
                                        [expr $t  + 1 ]]
                                global $afterdev
                                upvar #0 $afterdev aftd
                                monitor_create_ftd_status_bar $cfg(w) \
                                        $lgname $ftddev BEFORE $aftd(w)
                            }
                        }
                    }
                  
                    if {$ftd(pctdone) > $devsum(pctdone)} {
                        set devsum(pctdone) $ftd(pctdone)
                    }
                    set devsum(entries) [expr $devsum(entries) + \
                            $ftd(entries)]
                    set devsum(eKBps_out) [expr $devsum(eKBps_out) + \
                            $ftd(eKBps_out)]
                    set devsum(KBps_out) [expr $devsum(KBps_out) + \
                            $ftd(KBps_out)]
                    set devsum(read_kbps) [expr $devsum(read_kbps) + \
                            $ftd(read_kbps)]
                    set devsum(write_kbps) [expr $devsum(write_kbps) + \
                            $ftd(write_kbps)]
		    set devsum(pctbab) [expr $devsum(pctbab) + \
                            $ftd(pctbab) ]
                    
                    set devsum(drvmode) $ftd(drvmode) 
                    
                }
            } else {
                set cfg(newprf) 1
                close $prffile
                continue
            }   
        } else {
            set cfg(newprf) 1
            continue
        }   
        
        array set cfg [array get devsum]
    }
    if {[info exists lowinterval]} {
        set monitor(updatesec) $lowinterval
    }
    monitor_query_daemons
    monitor_display_info "" OFF
    if {$monitor(alertflag)} {
	monitor_alert
	set monitor(alertflag) 0
    }
    if {$autoreschedule} {
        set monitor(chklock) 0
	after [expr $monitor(updatesec) * 1000] monitor_check_info
    }
    set monitor(chklock) 0
}

#----------------------------------------------------------------------------
proc monitor_create_light_widget {w text width color {title ""}} {
    
    frame $w 
    pack $w -side left -padx 4
    if {$title != ""} {
        label $w.titleL -text $title -font fixed
        pack $w.titleL 
    }
    if {$color == "flat"} { 
        set reliefval "flat"
    }  else { 
        set reliefval "ridge"
    }
    frame $w.subF -borderwidth 3 -relief $reliefval
    pack $w.subF 
    label $w.subF.l  -font fixed -text [format "%-${width}s" $text]
    pack $w.subF.l 
    if {$color != "flat"} {
        monitor_change_light_widget_color $w $color
    }
}

#----------------------------------------------------------------------------
proc monitor_change_light_widget_color {w color} {

    set w $w.subF
    switch -exact -- $color {
	aliceblue {
	    $w.l configure -bg #F0F8FF
	    $w.l configure -fg #000000
	}
	gray70 {
	    $w.l configure -bg #BBBBBB
	    $w.l configure -fg #000000
	}
	green {
	    $w.l configure -bg #00FF00
	    $w.l configure -fg #000000
	}
	forestgreen {
	    $w.l configure -bg #228B22
	    $w.l configure -fg #FFFFFF
	}
	yellow {
	    $w.l configure -bg #FFFF00
	    $w.l configure -fg #000000
	}
	red {
	    $w.l configure -bg #FF0000
	    $w.l configure -fg #FFFFFF
	}
	default {
	    $w.l configure -bg $color
	    $w.l configure -fg #000000
	}
    }
   
}

#----------------------------------------------------------------------------
proc monitor_change_light_widget_text {w text width {color ""}} {
  
    if {$color != ""} {
	monitor_change_light_widget_color $w $color
    }

    set w $w.subF
  
    if {$width < 0 } {
	set width [expr 0 - $width]
	$w.l configure -text [format "%${width}s" $text]
    } else {
	$w.l configure -text [format "%-${width}s" $text]
    }
}

#----------------------------------------------------------------------------
proc monitor_create_ftd_status_bar {w loggrp ftddev {beforeafter ""} {afterw ""}} {
    global monitor $ftddev $loggrp
    upvar #0 $ftddev ftd

    if {![info exists $ftddev]} {return}
    set f "${w}.${ftddev}"
   
    set ftd(w) [frame $f]
    if {$beforeafter != ""} {
	if {$beforeafter == "BEFORE"} {
	    pack $f -side top -before $afterw -fill x -pady 0
	} else {
	    pack $f -side top -after $afterw -fill x -pady 0
	}
    } else {
	pack $f -side top -expand y -fill x -pady 0
    }
    set devlabel [string range $ftddev 0 [expr [string length $ftddev] - 6]]

    label $f.name -font fixed -text [format "%6s" $devlabel] 
    pack $f.name -side left
    monitor_create_light_widget $f.devcmt     "          " 10 flat
    monitor_create_light_widget $f.pctdone    "          " 10 gray70
    monitor_create_light_widget $f.read_kbps  "          " 10 gray70
    monitor_create_light_widget $f.write_kbps "          " 10 gray70
    monitor_create_light_widget $f.kbpsout    "          " 10 gray70
    monitor_create_light_widget $f.ekbpsout   "          " 10 gray70
    monitor_create_light_widget $f.entries    "          " 10 gray70
    monitor_create_light_widget $f.pctbab     "          " 10 gray70
    
    # -- create traces on the variables
    set cmd "monitor_change_devcmt $f.devcmt $ftddev"
    monitor_create_trace $ftddev devcmt $cmd 
    set cmd "monitor_change_kbpsout $f.kbpsout $ftddev"
    monitor_create_trace $ftddev KBps_out $cmd
    set cmd "monitor_change_ekbpsout $f.ekbpsout $ftddev"
    monitor_create_trace $ftddev eKBps_out $cmd   
    set cmd "monitor_change_pctbab $f.pctbab $ftddev"
    monitor_create_trace $ftddev pctbab $cmd
    set cmd "monitor_change_entries $f.entries $ftddev"
    monitor_create_trace $ftddev entries $cmd
    set cmd "monitor_change_pctdone $f.pctdone $ftddev"
    monitor_create_trace $ftddev pctdone $cmd
    set cmd "monitor_change_readkbps $f.read_kbps $ftddev"
    monitor_create_trace $ftddev read_kbps $cmd
    set cmd "monitor_change_writekbps $f.write_kbps $ftddev"
    monitor_create_trace $ftddev write_kbps $cmd

    monitor_change_devcmt $ftd(w).devcmt $ftddev
    monitor_change_ekbpsout $ftd(w).ekbpsout $ftddev
    monitor_change_kbpsout $ftd(w).kbpsout $ftddev
    monitor_change_entries $ftd(w).entries $ftddev
    monitor_change_pctdone $ftd(w).pctdone $ftddev
    monitor_change_pctbab  $ftd(w).pctbab $ftddev
    monitor_change_readkbps $ftd(w).read_kbps $ftddev
    monitor_change_writekbps $ftd(w).write_kbps $ftddev
    #
    # set a flag so we don't try to create this bar again
    #
    set ftd(created_status_bar) 1
}


proc monitor_create_trace {var element cmd args} {
    global monitor $var
    trace variable ${var}($element) w $cmd
    trace variable ${var}($element) u "monitor_delete_trace $var $element $cmd"
}

proc monitor_delete_trace {var element cmd args} {
    global monitor $var
    trace vdelete ${var}($element) w $cmd
}

#----------------------------------------------------------------------------
proc remove_group {w loggrp} {
    global monitor $loggrp last_access prev_access
    upvar #0 $loggrp cfg
    if {![info exists $loggrp]} {return}

    foreach dev $cfg(ftddevs) {
        global $dev
        set f $w.$dev
        if {![info exists $dev]} {continue}
        set cmd "monitor_change_kbpsout $f.kbpsout $dev"
        monitor_delete_trace $dev KBps_out $cmd
        set cmd "monitor_change_ekbpsout $f.ekbpsout $dev"
        monitor_delete_trace $dev eKBps_out $cmd   
        set cmd "monitor_change_pctbab $f.pctbab $dev"
        monitor_delete_trace $dev pctbab $cmd
        set cmd "monitor_change_entries $f.entries $dev"
        monitor_delete_trace $dev entries $cmd
        set cmd "monitor_change_pctdone $f.pctdone $dev"
        monitor_delete_trace $dev pctdone $cmd
        set cmd "monitor_change_readkbps $f.read_kbps $dev"
        monitor_delete_trace $dev read_kbps $cmd
        set cmd "monitor_change_writekbps $f.write_kbps $dev"
        monitor_delete_trace $dev write_kbps $cmd
        foreach name [array names $dev] {
            unset ${dev}($name)
        }
        unset $dev
    }
    set cmd "monitor_change_transferring [set ${loggrp}(w)].f.daemon $loggrp PMD"
    monitor_delete_trace $loggrp pmdalive $cmd
    set cmd "monitor_change_transferring [set ${loggrp}(w)].f.daemon $loggrp RMD"
    monitor_delete_trace $loggrp rmdalive $cmd
    set cmd "monitor_change_kbpsout [set ${loggrp}(w)].f.kbpsout  $loggrp"
    monitor_delete_trace $loggrp KBps_out $cmd
    set cmd "monitor_change_drvmode [set ${loggrp}(w)].f.drvmode $loggrp"
    monitor_delete_trace $loggrp drvmode $cmd
    set cmd "monitor_change_ekbpsout [set ${loggrp}(w)].f.ekbpsout $loggrp"
    monitor_delete_trace $loggrp eKBps_out $cmd
    set cmd "monitor_change_entries [set ${loggrp}(w)].f.entries $loggrp"
    monitor_delete_trace $loggrp entries $cmd
    set cmd "monitor_change_pctbab [set ${loggrp}(w)].f.pctbab $loggrp"
    monitor_delete_trace $loggrp pctbab $cmd
    set cmd "monitor_change_readkbps [set ${loggrp}(w)].f.read_kbps $loggrp"
    monitor_delete_trace $loggrp read_kbps $cmd
    set cmd "monitor_change_writekbps [set ${loggrp}(w)].f.write_kbps $loggrp"
    monitor_delete_trace $loggrp write_kbps $cmd
    unset last_access($loggrp)
    unset prev_access($loggrp)
    set i [lsearch $monitor(loggrps) $loggrp]
    set monitor(loggrps) [lreplace $monitor(loggrps) $i $i]
    foreach systemname $monitor(systems) {
	global $systemname
	upvar #0 $systemname system
        set i [lsearch -exact $system(pmdloggrps) $loggrp]
        if {$i != -1} {
            set system(pmdloggrps) \
                    [lreplace $system(pmdloggrps) $i $i]
        }
        set j [lsearch -exact $system(rmdloggrps) $loggrp]
        if {$j != -1} {
            set system(rmdloggrps) \
                    [lreplace $system(rmdloggrps) $j $j]
        }
    }
    global prev_cfg_access last_cfg_access
    unset prev_cfg_access($loggrp)
    unset last_cfg_access($loggrp)
    catch {destroy $w.$loggrp} dummy
    foreach name [array names $loggrp] {
        unset ${loggrp}($name)
    }
    unset ${loggrp}
    
    set lgnum [scan "%d" [string range $loggrp 1 3]] 
    monitor_display_error "Logical Group $lgnum has been stopped" 6
}

#----------------------------------------------------------------------------
proc monitor_create_group_area {w loggrp {beforeafter ""} {afterw ""}} {
    global monitor $loggrp
    upvar #0 $loggrp cfg
    if {![info exists $loggrp]} {return}
    monitor_readconfig $loggrp
    set f [frame $w.$loggrp -borderwidth 3 -relief groove]
    scan [string range $loggrp 1 3] "%d" grpno
    set atitle "Group [format %3d $grpno]: $cfg(comment)"
    label $f.titlebar -text $atitle -background black -foreground lightgrey
    pack $f.titlebar -side top -expand 1 -fill x
    if {$beforeafter != ""} {
	if {$beforeafter == "BEFORE"} {
	    pack $f -side top -before $afterw -fill x  \
		-pady 2 -anchor nw  -ipady 2
	} else {
	    pack $f -side top -after $afterw -fill x  \
		-pady 2 -anchor nw  -ipady 2
	}
    } else {
	pack $f -side top -fill x  -pady 2 -anchor nw \
	     -ipady 2
    }
    set ff [frame $f.f]
    pack $ff -side top -expand y -fill x  -pady 2

    label $ff.groupname -font fixed \
	-text "Group " 
    pack $ff.groupname -side left -anchor s
    monitor_create_light_widget $ff.daemon     "          " 10 gray70 "Connection"
    monitor_create_light_widget $ff.drvmode    "          " 10 gray70 "Mode/%Done"
    monitor_create_light_widget $ff.read_kbps  "          " 10 gray70 "Local Read"
    monitor_create_light_widget $ff.write_kbps "          " 10 gray70 "Loc. Write"
    monitor_create_light_widget $ff.kbpsout    "          " 10 gray70 "Net Actual"
    monitor_create_light_widget $ff.ekbpsout   "          " 10 gray70 "Net Effect"
    monitor_create_light_widget $ff.entries    "          " 10 gray70 "Entries"
    monitor_create_light_widget $ff.pctbab     "          " 10 gray70 "% BAB used"

    set cfg(w) $f
    set ptag $cfg(ptag)
    # -- create traces on the variables
    set cmd "monitor_change_transferring [set ${loggrp}(w)].f.daemon $loggrp PMD"
    monitor_create_trace $loggrp pmdalive $cmd
    set cmd "monitor_change_transferring [set ${loggrp}(w)].f.daemon $loggrp RMD"
    monitor_create_trace $loggrp rmdalive $cmd
    set cmd "monitor_change_kbpsout [set ${loggrp}(w)].f.kbpsout  $loggrp"
    monitor_create_trace $loggrp KBps_out $cmd
    set cmd "monitor_change_drvmode [set ${loggrp}(w)].f.drvmode $loggrp"
    monitor_create_trace $loggrp drvmode $cmd
    set cmd "monitor_change_ekbpsout [set ${loggrp}(w)].f.ekbpsout $loggrp"
    monitor_create_trace $loggrp eKBps_out $cmd
    set cmd "monitor_change_entries [set ${loggrp}(w)].f.entries $loggrp"
    monitor_create_trace $loggrp entries $cmd
    set cmd "monitor_change_pctbab [set ${loggrp}(w)].f.pctbab $loggrp"
    monitor_create_trace $loggrp pctbab $cmd
    set cmd "monitor_change_readkbps [set ${loggrp}(w)].f.read_kbps $loggrp"
    monitor_create_trace $loggrp read_kbps $cmd
    set cmd "monitor_change_writekbps [set ${loggrp}(w)].f.write_kbps $loggrp"
    monitor_create_trace $loggrp write_kbps $cmd
    return $f
}
#----------------------------------------------------------------------------
proc monitor_change_drvmode {w loggrp args} {
    global $loggrp monitorode
    upvar #0 $loggrp cfg
   
    switch $cfg(drvmode) \
        $monitorode(PASSTHRU) {
            monitor_change_light_widget_text $w "PASSTHRU " 10 aliceblue
        } \
        $monitorode(BACKFRESH) {
            monitor_change_light_widget_text $w "BACKFRESH" 10 aliceblue
        } \
        $monitorode(NORMAL) {
            monitor_change_light_widget_text $w " NORMAL  " 10 aliceblue
        } \
        $monitorode(TRACKING) { 
            monitor_change_light_widget_text $w "TRACKING " 10 aliceblue
        } \
        $monitorode(REFRESH) {
            monitor_change_light_widget_text $w " REFRESH " 10 aliceblue
        } 
        
   set ftddevlist [info globals *_$loggrp] 
   
   set w $cfg(w)
   foreach ftddev $ftddevlist {    
       set f "${w}.${ftddev}"
       if {[winfo exists $f.pctdone]} {
           monitor_change_pctdone $f.pctdone $ftddev
       }
   }
    
}
#----------------------------------------------------------------------------
proc monitor_change_devcmt {w item args} {
    global $item
    upvar #0 $item var
    if {![info exists var(devcmt)]} {
        return
    }
    set val [string range $var(devcmt) 0 10]
    monitor_change_light_widget_text $w $val 10
}
#---------------------------------------------------------------------------- 
proc monitor_change_ekbpsout {w loggrp args} {      
    global $loggrp
    upvar #0 $loggrp cfg 

    set val $cfg(eKBps_out)
    if {$val > 1000.0} {
	set str "[format "%5.1f MBps" [expr $val / 1000.0]]"
    } else {
	set str "[format "%5.1f KBps" $val]"
    }
    if {$val <= 0.05} {
	monitor_change_light_widget_text $w $str -10 gray70
    } else {
        monitor_change_light_widget_text $w $str -10 green
    }
}

#----------------------------------------------------------------------------
proc monitor_change_kbpsout {w item args} {
    global $item
    upvar #0 $item var
    set val $var(KBps_out)
    if {$val > 1000.0} {
	set str "[format "%5.1f MBps" [expr $val / 1000.0]]"
    } else {
	set str "[format "%5.1f KBps" $val]"
    }
    if {$val <= 0.05} {
	monitor_change_light_widget_text $w $str -10 gray70
    } else {
        monitor_change_light_widget_text $w $str -10 green
    }
  
}
#---------------------------------------------------------------------------- 
proc monitor_change_readkbps {w loggrp args} {      
    global $loggrp
    upvar #0 $loggrp cfg 

    set val $cfg(read_kbps)
    if {$val > 1000000.0} {
	set str "[format "%5.1f GBps" [expr $val / 1000000.0]]"
    } elseif {$val > 1000.0} {
	set str "[format "%5.1f MBps" [expr $val / 1000.0]]"
    } else {
	set str "[format "%5.1f KBps" $val]"
    }
    if {$val <= 0.05} {
	monitor_change_light_widget_text $w $str -10 gray70
    } else {
        monitor_change_light_widget_text $w $str -10 green
    }
}

#---------------------------------------------------------------------------- 
proc monitor_change_writekbps {w loggrp args} {      
    global $loggrp
    upvar #0 $loggrp cfg 

    set val $cfg(write_kbps)
    if {$val > 1000000.0} {
	set str "[format "%5.1f GBps" [expr $val / 1000000.0]]"
    } elseif {$val > 1000.0} {
	set str "[format "%5.1f MBps" [expr $val / 1000.0]]"
    } else {
	set str "[format "%5.1f KBps" $val]"
    }
    if {$val <= 0.05} {
	monitor_change_light_widget_text $w $str -10 gray70
    } else {
        monitor_change_light_widget_text $w $str -10 green
    }
}

#---------------------------------------------------------------------------- 
proc monitor_change_pctbab {w loggrp args} {      
    global $loggrp 
    upvar #0 $loggrp cfg 
    
    if { $cfg(pctbab) > 100 } {
        set ltcolor "red" 
        set cfg(pctbab) 100
    } elseif { $cfg(pctbab) > 80 } {
        set ltcolor "red"
    } elseif {$cfg(pctbab) > 50} {
        set ltcolor "yellow"
    } elseif {$cfg(pctbab) == 0}  {
        set ltcolor "gray70"
    } else {
        set ltcolor "green"
    }

    monitor_change_light_widget_text $w [format "%5.0f" $cfg(pctbab)] -10 $ltcolor
}   

#---------------------------------------------------------------------------- 
proc monitor_change_pctdone {w ftddev args} {      
    global $ftddev monitorode
    upvar #0 $ftddev cfg 
    set lg [string range $ftddev [expr [string length $ftddev] - 4] end]
    upvar #0 $lg lgcfg
    set needpct 0
    switch $lgcfg(drvmode) $monitorode(BACKFRESH) {
        set needpct 1
    } $monitorode(REFRESH) {
        set needpct 1
    } $monitorode(NORMAL) {
        set drvmode " NORMAL"
    } $monitorode(TRACKING) {
        set drvmode TRACKING 
    } $monitorode(PASSTHRU) {
        set drvmode PASSTHRU
    }
    set ltcolor "aliceblue"

    if { $needpct } {
        set donestr  "  [format "%5.0f" $cfg(pctdone)]%" 
        monitor_change_light_widget_text $w $donestr -10 $ltcolor
    } else {
        monitor_change_light_widget_text $w $drvmode 10 $ltcolor
    }
}      

#----------------------------------------------------------------------------
proc monitor_change_transferring {w loggrp which args} {
    global $loggrp monitor
    upvar #0 $loggrp cfg
    if {$cfg(pmdalive) && $cfg(rmdalive)} {
	monitor_change_light_widget_text $w "CONNECTED " 10 green
    } elseif {(!$cfg(pmdalive)) && (!$cfg(rmdalive))} {	
	monitor_change_light_widget_text $w "ACCUMULATE" 10 red
	set monitor(alertflag) 1
    } elseif {$cfg(pmdalive)} {
	monitor_change_light_widget_text $w " PMD ONLY " 10 yellow
    } else {
	monitor_change_light_widget_text $w " RMD ONLY " 10 yellow
    }
}


#----------------------------------------------------------------------------
proc monitor_change_entries {w ftddev args} {
    global $ftddev
    upvar #0 $ftddev ftd
    if {$ftd(entries) < 0 } {
        set ftd(entries) 0
    }
    set t "[format "%7d" $ftd(entries)]"
    if {$ftd(entries) == 0} {
	monitor_change_light_widget_text $w $t -10 gray70
    } else {
	monitor_change_light_widget_text $w $t -10 aliceblue
    }
}

#----------------------------------------------------------------------------
proc monitor_change_pctused {w ftddev args} {
    global $ftddev monitor
    upvar #0 $ftddev ftd
    set t "WL IN USE: [format "%3d" $ftd(pctused)]%"
    if {$ftd(pctused) == 0} {
	monitor_change_light_widget_text $w $t -10 gray70
    } elseif {$ftd(pctused) > 0 && $ftd(pctused) < 51} {
	monitor_change_light_widget_text $w $t -10 green
    } elseif {$ftd(pctused) > 50 && $ftd(pctused) < 80} {
	monitor_change_light_widget_text $w $t -10 yellow
    } elseif {$ftd(pctused) >= 80} {
	monitor_change_light_widget_text $w $t -10 red
	set monitor(alertflag) 1
    }
}

#----------------------------------------------------------------------------
proc monitor_create_form {{top "."}} {
    global monitor env
    set monitor(bell) 0
    set monitor(flash) 0
    set monitor(deiconify) 0
    set monitor(top) $top
    set monitor(updatesec) 1
    set monitor(loggrps) {}
    set monitor(lgfound) {}
    set monitor(systems) {}
    wm withdraw .
    catch "destroy .qm"
    toplevel .qm
    set monitor(top) .qm
    set top .qm
    catch "destroy [winfo children $top]"
    toplevel .a 
    set monitor(iconimage) [image create photo -file $env(FTD_LIBRARY)/%CAPQ%logo47.gif]
    set monitor(iconimage-r) [image create photo -file $env(FTD_LIBRARY)/%CAPQ%logo47r.gif]
    label .a.a -image $monitor(iconimage)
    pack .a.a
    wm overrideredirect .a 1
    wm withdraw .a
    wm geometry .a 47x47
    wm deiconify .a
    wm withdraw $top
    wm geometry $top +100+100
    wm geometry $top 680x400
    wm minsize $top 680 400
    wm iconwindow $top .a
    wm deiconify $top
    wm title $top "Softek %PRODUCTNAME% Monitor Tool"
    if {$top == "."} {set top ""}
    # -- status line
    set w [frame $top.f0 -bd 2 -relief groove]
    pack $w -fill x -side top -anchor nw
    label $w.status -text "" -foreground red
    pack $w.status -side left -anchor nw -padx .5i -fill y
    # -- error message journal
    set w [frame $top.f1 -bd 2 -relief groove]
    pack $w -fill x -side top -anchor nw
    tixScrolledText $w.error -scrollbar auto
    pack $w.error -side top -expand yes -fill both
    set monitor(errorw) [$w.error subwidget text]
    $monitor(errorw) tag configure warning -font fixed -foreground black
    $monitor(errorw) tag configure fatal -font fixed -foreground red
    $monitor(errorw) configure -font fixed -fg red -bg aliceblue \
	-height 8 -wrap none
    # -- create the bottom buttons
    set w [frame $top.f3]
    pack $w -side bottom -anchor nw -fill x
    button $w.exit  -text "Exit" -command {do_exit}   -font fixed
    pack $w.exit -side left -padx 4
    frame $w.rightF -relief ridge -bd 2
    pack $w.rightF -side right -padx 4
    set w $w.rightF
    label $w.spacer -text "Signal critical event with..." -font fixed
    pack $w.spacer -side top -anchor w
    checkbutton $w.bell -variable monitor(bell)  -text "Bell" -font fixed
    pack $w.bell -side left
    checkbutton $w.flash -variable monitor(flash)  -text "Flash" -font fixed
    pack $w.flash -side left
    checkbutton $w.deiconify -variable monitor(deiconify)  -text "Deiconify" -font fixed
    pack $w.deiconify -side left
    
    # -- create group and ftd device area
    set w [tixScrolledWindow $top.f2 -scrollbar auto]
    pack $w -expand yes -fill both -side top -anchor nw
    set w [$w subwidget window]
    set monitor(w) $w
    bind [$monitor(top) cget -class] <Destroy> {do_exit}
    set monitor(bell) 1
    set monitor(flash) 1
    set monitor(deiconify) 1
    monitor_check_info 1
}


#-----------------------------------------------------------------------------
proc monitor_update_now {} {
    monitor_check_info 0
}

#-----------------------------------------------------------------------------
proc monitor_add_error {system msg} {
    global monitor
    append msg "\n"
    set fatalflag 0
    $monitor(errorw) configure -state normal
    set size [$monitor(errorw) index end]
    set max_allowed_size [expr $monitor(msgs_retained) * 2.0]
    if {$size > $max_allowed_size} {
	$monitor(errorw) delete 1.0 [expr $size - $max_allowed_size]
    }
    set i [string first "\]" $msg]
    set ts {}
    if {$i > -1} {
	set ts "[string range "$msg" 0 $i]"
        set premsg "[string range "$msg" [expr $i + 2] end]"
        regsub {(\[proc: .*\] )(\[pid.*\] \[src.*\])(.*: \[.*\]:.*)} $premsg {\3} msg
    }
    if {$ts != ""} {
	if {-1 != [string first FATAL $msg]} {
	    $monitor(errorw) insert end "----- $ts ----- $system -----\n" \
		fatal
	    $monitor(errorw) insert end $msg fatal
	    set fatalflag 1
	} else {
	    $monitor(errorw) insert end "----- $ts ----- $system -----\n" \
		warning
	    $monitor(errorw) insert end $msg warning
	}
	$monitor(errorw) yview end
	$monitor(errorw) xview moveto 0
	if {$fatalflag} {set monitor(alertflag) 1}
	update
    }
    $monitor(errorw) configure -state disabled
}
	
#-----------------------------------------------------------------------------
proc monitor_alert {} {
    global monitor
    if {$monitor(deiconify) && "normal" != [wm state $monitor(top)]} {
	wm deiconify $monitor(top)
	raise $monitor(top)
    }
    if {$monitor(bell)} {
	bell
	bell
    }
    if {$monitor(flash)} {
	if {[wm state $monitor(top)] == "normal"} {
	    for {set i 0} {$i < 3} {incr i} {
		$monitor(errorw) configure -bg blue
		update
		after 50
		$monitor(errorw) configure -bg aliceblue
		update
		after 50
	    }
	} else {
	    for {set i 0} {$i < 5} {incr i} {
		.a.a configure -image $monitor(iconimage-r)
		update
		after 100
		.a.a configure -image $monitor(iconimage)
		update
		after 100
	    }
	}
    }
}

#-----------------------------------------------------------------------------
proc monitor_display_error {{msg ""} {wait 0}} {
    global monitor
    if {$monitor(top) == "."} {
	set w ".f0.status"
    } else {
	set w "$monitor(top).f0.status"
    }
    $w configure -foreground red -text $msg
    update
    bell
    if {!$wait} {
	after 4000 "$w configure -text {}; update "
    } else {
	after [expr $wait * 1000] "$w configure -text {}; update; update idletasks"
    }
    update
    update idletasks
}

#-----------------------------------------------------------------------------
proc monitor_display_info {{msg ""} {onoff ""}} {
    global monitor
    if {$monitor(top) == "."} {
	set w ".f0.status"
    } else {
	set w "$monitor(top).f0.status"
    }
    if {$onoff == "off" || $onoff == "OFF"} {
	$w configure -foreground blue 
	$w configure -text {}
	update
	return
    } elseif {$onoff == "on" || $onoff == "ON"} {
	$w configure -foreground blue 
	$w configure -text $msg
	update
	return
    } else {
 	$w configure -foreground blue -text "$msg"
	update
	update idletask
        after 4000 {
	    monitor_display_info {} OFF
	}
    }
}

#-----------------------------------------------------------------------------
# monitor_readconfig -- read a specific configuration file
#-----------------------------------------------------------------------------
proc monitor_readconfig {lgname} {
    global monitor last_cfg_access prev_cfg_access services
    if {![info exists last_cfg_access($lgname)]} {
        set last_cfg_access($lgname) 0
    }
    if {![info exists prev_cfg_access($lgname)]} {
        set prev_cfg_access($lgname) -1
    }
    set cfg_file "/%OURCFGDIR%/${lgname}.cur"
     if {[catch "file mtime $cfg_file" accesstime]} {
        return
    } 
    set prev_cfg_access($lgname) $last_cfg_access($lgname)
    set last_cfg_access($lgname) $accesstime
    if {[info exists $lgname] && ($last_cfg_access($lgname) == $prev_cfg_access($lgname))} {
        #
        # .cfg file not updated, don't update this group
        # 
        return
    }
    set havesystems 0
    set cfgfile $lgname
    set monitor(candidatecfg) $cfgfile
    global $cfgfile curtag
    upvar #0 ${cfgfile} cfg
    # -- read primary port # from /etc/services
    if {![ catch {set servfd [open $services r]}]} {
        set lines [split [read $servfd] \n]
        if {[set i [lsearch -regexp $lines "in\.%Q%"]]!=-1} {
            set portstr [lindex [lindex $lines $i] 1]
            if {[regexp {[0-9]*} $portstr portnum]} {
                set cfg(LOCRMDPORT:) $portnum
                set cfg(RMTRMDPORT:) $portnum
            }
        } 
        catch {close $servfd}
    }      
    if  {![info exists cfg(LOCRMDPORT:)]} { 
	set cfg(LOCRMDPORT:) 575 
        set cfg(RMTRMDPORT:) 575
    }
    # -- initialize internal use variables
    set cfg(comment) ""
    set cfg(ptag) "SYSTEM-A"
    set cfg(rtag) "SYSTEM-B"
    set cfg(curtag) "SYSTEM-A"
    set curtag "SYSTEM-A"
    set cfg(isprimary) 1
    set cfg(SYSTEM-A,HOST:) ""
    set cfg(SYSTEM-A,IP:) ""
    set cfg(SYSTEM-A,wlpoolcnt) 0
    set cfg(SYSTEM-B,HOST:) ""
    set cfg(SYSTEM-B,IP:) ""
    set cfg(SYSTEM-B,wlpoolcnt) 0

    # -- parse the configuration file
    set fd [open "/%OURCFGDIR%/$cfgfile.cur" r]
    set buf [read $fd]
    close $fd
    set lineno 0
    foreach line [split $buf "\n"] {
	incr lineno
	# -- discard comments and empty lines
	if {[string index $line 0] == "\#"} {continue}
	if {[regexp "^\[ \t]*$" $line]} {continue}
        # FIXME - get LOC from /etc/services
	switch -glob -- [lindex $line 0] {
            "NOTES:" {set cfg(comment) [lrange $line 1 end]}
	    "SECONDARYPORT:" {set cfg(RMTRMDPORT:) [lindex $line 1]}
	    "SYSTEM-TAG:" {
  		set cfg(curtag) [lindex $line 1]
		set curtag $cfg(curtag)
		if {[lindex $line 2] == "PRIMARY"} {
		    set cfg(isprimary) 1
		    set cfg(ptag) $cfg(curtag)
		} else {
		    set cfg(isprimary) 0
		    set cfg(rtag) $cfg(curtag)
		}
	    }
	    "HOST:" {set cfg($curtag,HOST:) [lindex $line 1]}
	    "THROTTLE:" {break}
	    "PROFILE:" {set remark ""}
            "REMARK:" {set remark [lrange $line 1 end]}
            "%PRODUCTNAME_TOKEN%-DEVICE:" { 
                regexp {%Q%[0-9][0-9]*} [lindex $line 1] devbasename xxx
                set ftdvarname "${devbasename}_${lgname}"
                global $ftdvarname
                upvar #0 $ftdvarname var
                set var(devcmt) $remark
            }
            
	}
    }
    set ptag $cfg(ptag)
    set rtag $cfg(rtag)
}

#=============================================================================
# monitor_query_daemons -- iterate through all systems and query daemon 
#                       health and new errors
#=============================================================================
proc monitor_query_daemons {} {
    global monitor
    foreach systemname $monitor(systems) {
	global $systemname
	upvar #0 $systemname system
	set system(pmdloggrpsfound) {}
	set system(rmdloggrpsfound) {}
	if {$system(isprimary)} {
	    set monitor(done) 0
	    monitor_rmd_query $systemname 1 1
	    vwait monitor(done)
	}
	set monitor(done) 0
	foreach plg $system(pmdloggrps) {
	    global $plg
	    upvar #0 $plg cfg
	    if {-1 == [lsearch $system(pmdloggrpsfound) $plg]} {
		if {$cfg(pmdalive) == 1} {set cfg(pmdalive) 0}
	    } else {
		if {$cfg(pmdalive) == 0} {set cfg(pmdalive) 1}
	    }
	}
	set errflag 1
	if {$system(issecondary)} {
	    if {$system(isprimary)} {
		# -- already got the errors on the PMD query
		set errflag 0
	    }
	    set monitor(done) 0

	    monitor_rmd_query $systemname 0 $errflag
            vwait monitor(done)
	}
	foreach rlg $system(rmdloggrps) {
	    global $rlg
	    upvar #0 $rlg cfg
	    if {-1 == [lsearch $system(rmdloggrpsfound) $rlg]} {
		if {$cfg(rmdalive) == 1} {set cfg(rmdalive) 0}
	    } else {
		if {$cfg(rmdalive) == 0} {set cfg(rmdalive) 1}
	    }
	}
    }
}

#=============================================================================
# monitor_rmd_query
#
#    Sends a query to a master RMD on a specific host and processes the
#    return value
# Results:
#    returns the name of the global array created.  This name has an
#    element named status, which may be 0=good, -1=bad host/port number, 
#    -2=operation failed in remote RMD.
#=============================================================================
proc monitor_rmd_query {systemname pmdflag errflag {doit 0}} { 
    global monitor $systemname
    upvar #0 $systemname system
    # -- our system variable has disappeared, simply return
    if {![info exists $systemname]} {return}
    if {![info exists system(lock)]} {set system(lock) 0} 
    #print_call_stack
    while {$system(lock) == 1 && $doit == 0} {
	# -- spin on lock
	after 200 "monitor_rmd_query $systemname $pmdflag $errflag $doit"
	return 
    }
    # -- our system variable has disappeared, simply return
    if {![info exists $systemname]} {
	set monitor(done) 1
	return
    }
    if {!$doit} {
	# -- submit the request to be fullfilled asynchronously
	after 5 "monitor_rmd_query $systemname $pmdflag $errflag 1"
       	return
    }
    
    # -- our system variable has disappeared, simply return
    if {![info exists $systemname]} {
	set monitor(done) 1
	set system(lock) 0
	return
    }
    # -- we're now asynchronous, one more test of the spin lock
    while {$system(lock)} {
	# -- spin on lock
	after 200 "monitor_rmd_query $systemname $pmdflag $errflag $doit"
	return 
    } 
    # -- our system variable has disappeared, simply return
    if {![info exists $systemname]} {
	set monitor(done) 1
	set system(lock) 0
	return
    }
    # -- set the lock -- guarantees one dialog with RMD at a time
    set system(lock) 1
    set system(timeoutid) ""
    set system(timeout) $monitor(timeout)
    set system(buffer) ""
    set system(pmdnamesfound) ""
    set system(rmdnamesfound) ""
    set system(done) 0
    if {[catch "socket $systemname $system(port)" system(sock)]} {
	monitor_display_info "could not connect to RMD server on $systemname/$system(port)" ON
	after 2000
	monitor_display_info "" OFF
	set system(lock) 0
	set monitor(done) 1
	return
    }
    fconfigure $system(sock) -translation {auto lf} -buffersize 10000
    fconfigure $system(sock) -blocking on
    if {$errflag} {
	if {$pmdflag} {
	    set cmd "ftd get all process and error info PMD_ $system(erroffset)"
	} else {
	    set cmd "ftd get all process and error info RMD_ $system(erroffset)"
	}
    } else {
	if {$pmdflag} {
	    set cmd "ftd get all process info PMD_"
	} else {
	    set cmd "ftd get all process info RMD_"
	}
    }
    puts $system(sock) $cmd
    flush $system(sock)
    fconfigure $system(sock) -blocking off
    set system(done) 0
    set system(timeoutid) [after $system(timeout) [list monitor_readtimeout \
						       $system(sock) \
						       $systemname]]
    fileevent $system(sock) readable [list monitor_readfromsock $system(sock) \
					  $systemname]
    vwait ${systemname}(done)
    if {[llength $system(buffer)] >= 2} {
	set plist [lindex $system(buffer) 1 ]
	foreach pitem $plist {
	    set pname [lindex $pitem 0]
	    set grpname [string range $pname 4 end]
	    if {$pmdflag} {
		lappend system(pmdloggrpsfound) "p$grpname"
	    } else {
		lappend system(rmdloggrpsfound) "p$grpname"
	    }
	}
    }
    if {[llength $system(buffer)] >= 4} {
	set errstatus [lindex $system(buffer) 2]
	set proceedflag 0
	switch -exact -- [lindex $errstatus 0] {
	    "***ERRORS***" {
		set newoffset [lindex $errstatus 1]
		set proceedflag 1
	    }
	    default {set proceedflag }
	}
	if {$proceedflag} {
	    foreach error [split [lindex $system(buffer) 3] "\n"] {
		if {$error == {} || $error == { } } {continue}
		monitor_add_error $systemname $error
	    }
	    set system(erroffset) $newoffset
	}
    }
    set system(buffer) ""
    set system(lock) 0
    set monitor(done) 1
}

proc monitor_readfromsock {sock var} {
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
	set d(timeoutid) [after $d(timeout) "monitor_readtimeout $sock $var"]
    }
}

proc monitor_readtimeout {sock var} {
    global $var
    upvar #0 $var d
    catch "after cancel $d(timeoutid)"
    fileevent $sock readable {}
    catch "close $sock"
    monitor_display_error "timeout occurred while querying $var master daemons" 6
    set d(done) 1
}


#-------------------------------------------------------------------------
#
#  center_window {w}
#
#  Given a top-level window widget, this procedure will center the window
#  according to screen size
#  
#-------------------------------------------------------------------------
proc center_window {w} {
    wm withdraw $w  
    update idletasks
    if { [string compare $w  "."] == 0 } {
	set vrootx 0
	set vrooty 0
    } else {
	set vrootx [winfo vrootx [winfo parent $w]]
	set vrooty [winfo vrooty [winfo parent $w]]
    }
    set x [expr [winfo screenwidth $w]/2 - [winfo reqwidth $w]/2 \
	    - $vrootx]
    set y [expr [winfo screenheight $w]/2 - [winfo reqheight $w]/2 \
	    - $vrooty]
    wm geom $w +$x+$y
    wm deiconify $w
}

#--------------------------------------------------------------------------
proc Introduction {} {
    global env waiting
    
    set waiting 0 
    
    toplevel .introT
   	wm overrideredirect .introT 1
   	set bdwidth 6
    
    frame .introT.borderF -relief ridge -bd $bdwidth -bg white 
    pack .introT.borderF -expand yes -fill both 
    set w .introT.borderF
    
    catch {image create photo -file $env(FTD_LIBRARY)/%CAPQ%logo202.gif} t
   									
    label $w.%Q%image -image $t -bg white 
    pack $w.%Q%image -side top -pady .5c -padx .5c
    label $w.title -text "Softek %PRODUCTNAME%, Open Systems Edition" -fg black -bg white
    pack $w.title -side top
    message $w.vermsg -text "Monitor Tool - Version %VERSION%" -aspect 10000 -justify center -font -Adobe-Helvetica-Bold-R-Normal--*-100-*-*-*-*-*-* -bg white
    pack $w.vermsg -side top
    set msg "Copyright (c) 2001 %COMPANYNAME%. All Rights Reserved."
    message $w.msg2 -text $msg                  -aspect 10000 -justify center -font -Adobe-Helvetica-Bold-R-Normal--*-100-*-*-*-*-*-* -bg white
    pack $w.msg2 -side top
    center_window .introT
    after 5000 { set waiting 1}
    update
    
    # should prevent MonitorTool to hang!    
    #puts "waiting = $waiting, ..."
    if {!$waiting} {   
        tkwait variable waiting
    }
    destroy .introT    
}

#--------------------------
#
# Utility to print calling stack for Tcl interpreter.
#

proc print_call_stack { } {
    set level [info level]

    set level [expr $level -1]

    puts "Tcl calling stack for [info script]"

    while { $level > 0 } {
        puts "\t [info level $level]"
        
        incr level -1
    }
}

# stack.tcl

#--------------------------
wm withdraw .
#print_call_stack
Introduction 
#print_call_stack
monitor_create_form




