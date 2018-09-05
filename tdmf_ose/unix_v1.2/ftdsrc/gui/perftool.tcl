#!/bin/sh
# next line will restart with wish but the exec line will be ignored by wish because of \
LC_ALL=C exec /%FTDTIXBINPATH%/%FTDTIXWISH% "$0" "$@"
#-----------------------------------------------------------------------------
#
# %Q%perftool -- %PRODUCTNAME% Performance Monitor
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
#-----------------------------------------------------------------------------

global ftdpt_exit_called
set ftdpt_exit_called 0
global afterid
set afterid ""

if {$tcl_platform(os) == "HP-UX"} {
    set whoami "/usr/bin/whoami"
} elseif {$tcl_platform(os) == "SunOS"} {
    if {[file executable "/usr/ucb/whoami"]} {
        set whoami "/usr/ucb/whoami"
        } else {
        set whoami "/usr/bin/whoami"
        }
} elseif {$tcl_platform(os) == "AIX"} {
    set whoami "/bin/whoami"
} elseif {$tcl_platform(os) == "Linux"} {
    set whoami "/usr/bin/whoami"
}
catch "exec $whoami" user
if {$user != "root" } {
   puts stderr "You must be root to run this process...aborted"
   exit
}

package require Tk
package require Tix

global env
set env(LANG) C
if {![info exists env(DISPLAY)]} {
    puts stderr "DISPLAY environment variable is not set"
    exit
}
set env(FTD_LIBRARY) /%FTDLIBDIR%/%Q%%REV%

# -- bring BLT namespace into forefront
if {$tcl_platform(os) == "Linux"} {
    package require BLT
}
if { [info commands "namespace"] == "namespace" } {
    if { $tcl_version >= 8.0 } {
        namespace import blt::*
    } else {
        catch { import add blt }
    }
    if { $tcl_version >= 8.0 } {
        namespace import -force blt::tile::*
    } else {
        import add blt::tile
    }
} else {
    foreach cmd { button checkbutton radiobutton frame label
        scrollbar toplevel menubutton listbox } {
        if { [info command tile${cmd}] == "tile${cmd}" } {
            rename ${cmd} ""
            rename tile${cmd} ${cmd}
        }
    }
}

global pors
set pors "both"
global vecseed
set vecseed 0

#--------------------------------------------------------------
# initilize global variables to default values
array set ftd {
    lock           0
    top            {}
    chartcnt       0
    curchart      -1
    curchartw      {}
    charts         {}
    loggrps        {}
    curloggrp     -1
    helpflag       1
    helpwidget     {}
    stdstats       { 
	{"ts" x}
	{"dummy1" 0}
	{"Device" n}
	{"Total KBps" y}
	{"Data KBps" y}
	{"Entries per sec" y}
	{"WL entries" y}
	{"WL sects used" y}
	{"WL pct full" y}
	{"Entry age" y}
	{"Refr sects done" y}
	{"Refr sects to go" y}
	{"Pct refreshed" y}
    }	
    color,0        "forestgreen"
    color,1        "goldenrod"
    color,2        "slateblue"
    color,3        "tomato"
    color,4        "magenta"
    color,5        "blue"
    color,6        "green"
    color,7        "pink"
    color,8        "violet"
    color,9        "skyblue"
    formwidgets    {}
}

#-------------------------------------------------------------------------
# 
#  replaceRTM {}
#
#  Returns: replace UNIX* registered trademark with a graphical version
#
#-------------------------------------------------------------------------

proc replaceRTM {oldString} {
    regsub -all {UNIX\*} $oldString "UNIX\xAE" result 
    return $result
}

#--------------------------------------------------------------
# ftdpt_build_form -- create the top level form for chart creation /
#                     modify / delete
proc ftdpt_build_form {} {
    global ftd env argc argv
    global ftdpt_title ftdpt_charttype ftdpt_loggrp ftdpt_device ftdpt_stat
    global ftdpt_samples pors
    set ftdpt_title ""
    set ftdpt_charttype "Line"
    set ftdpt_loggrp ""
    set ftdpt_device ""
    set ftdpt_stat ""
    set ftdpt_samples 21
    wm withdraw .
    catch "destroy .qf"
    toplevel .qf
    toplevel .qfi
    set t [image create photo -file $env(FTD_LIBRARY)/%CAPQ%logo47.gif]
    label .qfi.i -image $t
    pack .qfi.i
    wm overrideredirect .qfi 1
    wm withdraw .qfi
    wm geometry .qfi 47x47
    wm deiconify .qfi
    wm withdraw .qf
    wm iconwindow .qf .qfi
    wm deiconify .qf
    wm withdraw .qf
    wm title .qf [replaceRTM "%PRODUCTNAME% Performance Monitor"]
    wm protocol .qf WM_DELETE_WINDOW {ftdpt_exit 1}
    wm geometry .qf +200+200
    set ftd(helpw) [tixBalloon .qf.help -initwait 4000]
    if {-1 != [lsearch -glob "$argv" "-no*"]} {$ftd(helpw) configure -state none}
    if {-1 != [lsearch -glob "$argv" "-p*"]} {set pors "primary"} 
    if {-1 != [lsearch -glob "$argv" "-s*"]} {set pors "secondary"} 
    frame .qf.f -borderwidth 2 -relief groove
    pack .qf.f -padx 4 -pady 4 -fill x
    set lgheight 6
    # -- build initial data structures
    ftdpt_build_data
    #
    # -- define Step I of form
    frame .qf.f.f0
    pack .qf.f.f0 -fill x -side top
    set ftd(errormsg) ""
    set ftd(emsgw) [label .qf.f.f0.l0 -fg red -text $ftd(errormsg) -width 60]
    pack $ftd(emsgw) -side left
    frame .qf.f.f1
    pack .qf.f.f1 -fill x -side top
    label .qf.f.f1.l1 -text "Step I:  Chart Setup"
    pack .qf.f.f1.l1 -side left
    frame .qf.f.f2 -borderwidth 2 -relief groove
    pack .qf.f.f2 -padx 4 -pady 4 -fill x
    frame .qf.f.f2.f1
    pack .qf.f.f2.f1 -side top
    set ftd(chartneww) [button .qf.f.f2.f1.b1 -text "Define New Chart" \
			    -command ftdpt_create_chart]
    pack $ftd(chartneww) -side left -padx 5 -pady 3
    set ftd(titlew) [tixLabelEntry .qf.f.f2.e1 -label "Title:  " \
			 -options {
			     entry.width 50
			     label.width 20
			     label.anchor e
			     entry.textVariable ftdpt_title
			 }]
    pack $ftd(titlew) -side top -anchor nw
    set ftd(charttypew) [tixComboBox .qf.f.f2.cb2 -label "Chart Type:  " \
			     -editable false -dropdown true \
			     -variable ftdpt_charttype \
			     -options {
				 listbox.height 2
				 entry.width 12
				 label.width 20
				 label.anchor e
			     }]
    $ftd(charttypew) insert end "Line"
    $ftd(charttypew) insert end "Bar"
    $ftd(charttypew) insert end "Stacked Bar"
    set ftdpt_charttype "Line"
    pack $ftd(charttypew) -side top -anchor nw
    set ftd(samplesw) [tixControl .qf.f.f2.co3 -label "Number of Samples:  " \
			   -integer true -variable ftdpt_samples -min 1 \
			   -max 1000 \
			   -options {
			       entry.width 4
			       label.width 20
			       label.anchor e
			   }]
    set ftdpt_samples 21
    pack $ftd(samplesw) -side top -anchor nw
    # -- define Step II of form
    frame .qf.f.f3
    pack .qf.f.f3 -fill x -side top
    label .qf.f.f3.l1 -text "\nStep II:  Measurements Shown in Chart"
    pack .qf.f.f3.l1 -side left
    frame .qf.f.f4 -borderwidth 2 -relief groove
    pack .qf.f.f4 -padx 4 -pady 4 -fill x -side top
    if {[llength $ftd(loggrps)] < 7} {set lgheight [expr [llength $ftd(loggrps)] - 1]}
    set ftd(loggrpw) [tixComboBox .qf.f.f4.cb1 -label "%CAPGROUPNAME% Group:  " \
			  -dropdown true -editable false \
			  -variable ftdpt_loggrp \
			  -command ftdpt_update_stats_form \
			  -options "
			      listbox.height $lgheight
			      label.width 20
                              label.anchor e
                              entry.width 3
                          "]
    pack $ftd(loggrpw) -side top -anchor nw
    set ftd(devw) [tixComboBox .qf.f.f4.cb2 -label "%CAPQ% Device:  " \
		       -dropdown true -editable false \
		       -variable ftdpt_device \
		       -options {
			   listbox.height 8 
			   label.width 20
			   label.anchor e
			   entry.width 25
			   listbox.width 25
		       }]
    pack $ftd(devw) -side top -anchor nw
    set ftdpt_device ""
    
    set ftd(statw) [tixComboBox .qf.f.f4.cb3 -label "Measurement:  " \
			-dropdown true -editable false \
			-variable ftdpt_stat \
			-options {
			    listbox.height 8 
			    label.width 20
			    label.anchor e
			    entry.width 25
			    listbox.width 25
			}]
    pack $ftd(statw) -side top -anchor nw
    set ftdpt_stat ""
    
    frame .qf.f.f4.f4
    pack .qf.f.f4.f4 -side top
    set ftd(addmeasw) [button .qf.f.f4.f4.b1 -text "Add To Chart" \
			   -command ftdpt_add_measure_to_chart]
    pack $ftd(addmeasw) -side left -padx 5 -pady 3
    set ftd(updatemeasw) [button .qf.f.f4.f4.b2 -text "Modify Measurement" \
			      -command ftdpt_mod_measure_in_chart]
    pack $ftd(updatemeasw) -side left -padx 5 -pady 3
    set ftd(delmeasw) [button .qf.f.f4.f4.b3 -text "Remove from Chart" \
			   -command ftdpt_del_measure_from_chart]
    pack $ftd(delmeasw) -side left -padx 5 -pady 3
    set sl [tixScrolledListBox .qf.f.f4.sl -scrollbar auto \
		-scrollbars auto -browsecmd ftdpt_sel_measure -options {
		    listbox.width 50
		    listbox.height 4
		}]
    pack $sl -side top -anchor nw -expand yes -fill both
    set ftd(measuresw) [$sl subwidget listbox]
    $ftd(measuresw) delete 0 end
    frame .qf.f.f5
    pack .qf.f.f5 -side top -fill x
    label .qf.f.f5.l1 -text "\nStep III:  Chart Display"
    pack .qf.f.f5.l1 -side left
    
    
    frame .qf.f.f6 -borderwidth 2 -relief groove
    pack .qf.f.f6 -padx 4 -pady 4 -fill x -side top
    frame .qf.f.f6.f1 
    pack .qf.f.f6.f1 -side top
    set ftd(displaychartw) [button .qf.f.f6.f1.b1 \
				-text "Display Chart" \
				-command ftdpt_show_chart]
    pack $ftd(displaychartw) -side left -padx 5 -pady 3
    set ftd(deletechartw) [button .qf.f.f6.f1.b2 -text "Delete Chart" \
			    -command ftdpt_delete_chart]
    pack $ftd(deletechartw) -side left -padx 5 -pady 3
    set ftd(exitw) [button .qf.f.b3 -text "Exit" \
			    -command {ftdpt_exit 2}]
    pack $ftd(exitw) -side top -padx 5 -pady 3
 			    
    scan [string range [lindex $ftd(loggrps) 0] 1 3] "%d" ftdpt_loggrp
    wm deiconify .qf
    set tlgnums ""
    set ftd(measureslist) ""
    set ftd(curmeasuresidx) -1
    foreach lg [lsort $ftd(loggrps)] {
	scan [string range $lg 1 3] "%d" lgnum
	if {-1 == [lsearch -exact $tlgnums $lgnum]} {
	    lappend tlgnums $lgnum
	    $ftd(loggrpw) insert end $lgnum
	}
    }
    if {[llength $tlgnums] > 0} {
	set ftdpt_loggrp [lindex $tlgnums 0]
    } else {
	set ftdpt_loggrp ""
    }
    ftdpt_create_chart
}

#--------------------------------------------------------------
proc ftdpt_display_error {msg} {
    global ftd
    raise .qf
    $ftd(emsgw) configure -fg red -text $msg
    update
    bell
    set x [after 4000 "$ftd(emsgw) configure -text {}"]
}

#--------------------------------------------------------------
proc ftdpt_display_info {msg} {
    global ftd
    raise .qf
    $ftd(emsgw) configure -fg blue -text $msg
    update
    set x [after 4000 "$ftd(emsgw) configure -text {}"]
}

#--------------------------------------------------------------
proc ftdpt_update_stats_form {args} {
    global ftd ftdpt_loggrp ftdpt_stat ftdpt_device
    if {![info exists ftdpt_loggrp]} {return}
    if {$ftdpt_loggrp == ""} {return}
    set lgname $ftd(lgname,$ftdpt_loggrp)
    upvar #0 $lgname lg
    [$ftd(statw) subwidget listbox] delete 0 end
    set i -1
    set slist ""
    foreach item $lg(statinfo) {
	set name [lindex $item 0]
	set type [lindex $item 1]
	if {$type == "y" && -1 == [lsearch $slist $name]} {
	    incr i
	    $ftd(statw) insert end $name
	    lappend slist $name
	}
    }
    [$ftd(statw) subwidget listbox] configure -height $i
    set ftdpt_stat [lindex $slist 0]
    [$ftd(devw) subwidget listbox] delete 0 end
    foreach device $lg(devices) {
	$ftd(devw) insert end $device
    }
    set ftdpt_device [lindex $lg(devices) 0]
}

#--------------------------------------------------------------
proc ftdpt_add_measure_to_chart {} {
    global ftd ftdpt_loggrp ftdpt_device ftdpt_stat
    set lgname $ftd(lgname,$ftdpt_loggrp)
    set item [format "%s--%s--%s" $lgname $ftdpt_device $ftdpt_stat]
    regsub -all { } $item {_} item2
    if {-1 != [lsearch $ftd(measureslist) $item2]} {
	ftdpt_display_error "This measurement is already shown in this chart"
	return
    }
    $ftd(measuresw) insert end $item2
    set ftd(measureslist) [$ftd(measuresw) get 0 end]
    set ftd(curmeasuresidx) [expr [llength $ftd(measureslist)] - 1]
}

#--------------------------------------------------------------
proc ftdpt_mod_measure_in_chart {} {
    global ftd ftdpt_loggrp ftdpt_device ftdpt_stat
    set ftd(measureslist) [$ftd(measuresw) get 0 end]
    if {$ftd(curmeasuresidx) == -1} {
	ftdpt_add_measure_to_chart
	return
    }
    set lastidx [expr [llength $ftd(measureslist)] - 1]
    if {$lastidx < $ftd(curmeasuresidx)} {
	ftdpt_add_measure_to_chart
	return
    }
    set lgname $ftd(lgname,$ftdpt_loggrp)
    set item [format "%s--%s--%s" $lgname $ftdpt_device $ftdpt_stat]
    regsub -all { } $item {_} item2
    if {-1 != [lsearch $ftd(measureslist) $item2]} {
	ftdpt_display_error "This measurement is already shown in this chart"
	return
    }
    if {$ftd(curmeasuresidx) == 0} {
	set l1 $item2
	set l2 [lrange $ftd(measureslist) 1 end]
    } elseif {$ftd(curmeasuresidx) == $lastidx} {
	set l1 [lrange $ftd(measureslist) 0 [expr $lastidx - 1]]
	set l2 $item2
    } else {
	set l1 [lrange $ftd(measureslist) 0 [expr $ftd(curmeasuresidx) - 1]]
	lappend l1 $item2
	set l2 [lrange $ftd(measureslist) [expr $ftd(curmeasuresidx) + 1] end]
    }
    set ftd(measureslist) [concat $l1 $l2]
    $ftd(measuresw) delete $ftd(curmeasuresidx) $ftd(curmeasuresidx)
    $ftd(measuresw) insert $ftd(curmeasuresidx) $item2
}

#--------------------------------------------------------------
proc ftdpt_del_measure_from_chart {} {
    global ftd ftdpt_loggrp ftdpt_device ftdpt_stat
    set ftd(measureslist) [$ftd(measuresw) get 0 end]
    if {$ftd(curmeasuresidx) == -1} {
	ftdpt_display_error "No measurement selected for removal"
	return
    }
    set lastidx [expr [llength $ftd(measureslist)] - 1]
    if {$lastidx < 0} {
	ftdpt_display_error "Empty list - no measurements to remove"
	return
    }
    if {$lastidx < $ftd(curmeasuresidx)} {
	set ftd(curmeasuresidx $lastidx
    }
    if {$ftd(curmeasuresidx) == 0} {
	$ftd(measuresw) delete 0 0
	set ftd(measureslist) [lrange $ftd(measureslist) 1 end]
	return
    } elseif {$ftd(curmeasuresidx) == $lastidx} {
	set ftd(measureslist) [lrange $ftd(measureslist) 0 [expr $lastidx - 1]]
	$ftd(measuresw) delete $lastidx $lastidx
	return
    } else {
	set l1 [lrange $ftd(measureslist) 0 [expr $ftd(curmeasuresidx) - 1]]
	set l2 [lrange $ftd(measureslist) [expr $ftd(curmeasuresidx) + 1] end]
    }
    set ftd(measureslist) [concat $l1 $l2]
    $ftd(measuresw) delete $ftd(curmeasuresidx) $ftd(curmeasuresidx)
}

#--------------------------------------------------------------
proc ftdpt_sel_measure {args} {
    global ftd ftdpt_loggrp ftdpt_device ftdpt_stat
    set sel [$ftd(measuresw) curselection]
    if {$sel == ""} {
	set ftdcurmeasureidx -1
	return
    }
    set ftd(curmeasuresidx) $sel
    set line [$ftd(measuresw) get $sel $sel]
    if {$line == ""} {return}
    regsub -all {_} $line { } items
    set itemlist [split $items "--"]
    scan [string range [lindex $itemlist 0] 1 3] "%d" ftdpt_loggrp
    set ftdpt_device [lindex $itemlist 2]
    set ftdpt_stat [lindex $itemlist 4]
}

#--------------------------------------------------------------
proc ftdpt_setstate_form {} {
}

#--------------------------------------------------------------
proc ftdpt_create_chart {} {
    global ftd
    global ftdpt_title ftdpt_charttype ftdpt_loggrp ftdpt_device ftdpt_stat
    global ftdpt_samples

    #
    # Charts are created, and should be deleted. The first crerated is chart0,
    # then chart1 and so on.  Charts may be deleted in any order.  When
    # this routine is called, the lowest number chart, not already existing
    # will be created.
    #
    for {set t 0} {$t < $ftd(chartcnt)} {incr t} {
	if {[info globals chart$t] != ""} {
	    upvar #0 chart$t chart
	    if {!$chart(inuseflag)} {
		break
	    }
	} else {
	    # first reference to a new chart, make it global
	    global chart$t
	    upvar #0 chart$t chart
	    break;
	}
    }
    if {$t == $ftd(chartcnt)} {
	# first reference to a new chart, make it global
	global chart$t
	upvar #0 chart$t chart
	incr ftd(chartcnt)
    }
    set ftd(curchart) $t
    array set chart {
	title ""
	type "Line"
	samples 21
	stats ""
	vectors ""
	chartw ""
	inuseflag 0
    }
    set ftdpt_title $chart(title)
    set ftdpt_charttype $chart(type)
    set ftdpt_device ""
    set ftdpt_stat ""
    set ftdpt_samples 21
    scan [string range [lindex $ftd(loggrps) 0] 1 3] "%d" ftdpt_loggrp
    wm deiconify .qf
    set tlgnums ""
    set ftd(measureslist) ""
    set ftd(curmeasuresidx) -1
    scan [string range [lindex $ftd(loggrps) 0] 1 3] "%d" ftdpt_loggrp
    $ftd(displaychartw) configure -text "Display Chart"
    $ftd(measuresw) delete 0 end
    set ftd(measureslist) ""
    set $ftd(curmeasuresidx) -1
    $ftd(displaychartw) configure -text "Display Chart"
}

#--------------------------------------------------------------
proc ftdpt_show_chart {} {
    global ftd ftdpt_charttype ftdpt_samples
    append chartname "chart" $ftd(curchart)
    upvar #0 $chartname chart
    if {0 == [llength $ftd(measureslist)]} {
	ftdpt_display_error "There are no measurements to display in chart"
	return
    }
    if {$chart(inuseflag)} {
	ftdpt_modify_existing_chart
    } else {
	if {$ftdpt_charttype == "Line" && $ftdpt_samples < 2} {
	    ftdpt_display_error "Line Graphs require at least 2 samples"
	    return
	}
	ftdpt_create_new_chart
	ftdpt_create_chart
    }
}

#--------------------------------------------------------------
# update a chart to new values set by the GUI
proc ftdpt_modify_existing_chart {} {
    global ftd
    global ftdpt_title ftdpt_charttype ftdpt_loggrp ftdpt_device ftdpt_stat
    global ftdpt_samples
    append chartname "chart" $ftd(curchart)
    upvar #0 $chartname chart
    # -- capture previous state of the chart
    set btitle $chart(title)
    set btype $chart(type)
    set bsamples $chart(samples)
    set bstats $chart(stats)
    set bvectors $chart(vectors)
    if {0 == [llength $ftd(measureslist)]} {
	ftdpt_display_error "No measurements selected for updating chart"
	return
    }
    if {$ftdpt_charttype == "Line" && $ftdpt_samples < 2} {
	ftdpt_display_error "Line Graphs require at least 2 samples"
	return
    }
    set redoticsflag 0
    # -- set the lock
    # -- set the lock
#    if {$ftd(lock) == 1} {
#	puts stderr "GAAAAAAAAAACK lock already set:1"
#	set level [info level]
#	for {set i 1} {$i < $level} {incr i} {
#	    puts stderr "Level $i: [info level $i]"
#	}
#    }
    set ftd(lock) 1

    #
    # Various calls below generate <Destroy> events.
    # Don't know why this is... but unhook the destroy binding for
    # now, re-establish it down below
    # The manual says these events wont be process unless
    # "update idletasks" is called, or we return to the
    # event loop, but its happening....... :-(
    #
    set destroywbind [bind $chart(chartw) <Destroy>]
    set destroyiwbind [bind $chart(iw) <Destroy>]
    bind $chart(chartw) <Destroy> {}
    bind $chart(iw) <Destroy> {}
    bind $chart(gw) <Destroy> {}
    # == deal with a title change
    if {$ftdpt_title != $btitle} {
	set chart(title) $ftdpt_title
	wm title $chart(chartw) $chart(title)
    }
    # == deal with new / deleted statistics
    set dellist ""
    set newlist ""
    foreach ostat $bstats {
	if {-1 == [lsearch $ftd(measureslist) $ostat]} {
	    lappend dellist $ostat
	}
    }
    foreach nstat $ftd(measureslist) {
	if {-1 == [lsearch $bstats $nstat]} {
	    lappend newlist $nstat
	}
	# -- create new vectors
	set name "$chartname--$nstat"
        set vname [new_vec_name]
        lappend chart(vecnames) $vname
        set chart(TOV,$name) $vname
        set chart(TON,$vname) $name
	if {[info globals $vname] == ""} {
            set vnamestring [concat "$vname" "($chart(samples)"]
	    uplevel #0 vector create ${vname}($chart(samples))
	}
    }
    # -- if any changes, delete the elements and re-add them to the chart
    if {0 != [llength $dellist] || 0 != [llength $newlist]} {
	foreach d $bstats {
	    $chart(gw) element delete $d
	}
	set i -1
	foreach s $ftd(measureslist) {
	    incr i
	    if {$i > 9} {set i 0}
            set name "$chartname--$s"
#            set vname [new_vec_name]
#            lappend chart(vecnames) $vname
#            set chart(TOV,$name) $vname
#            set chart(TON,$vname) $name
	    if {$chart(type) != "Line"} {
		$chart(gw) element create "$s" \
		    -xdata $chart(XLISTVNAME) \
		    -ydata $chart(TOV,$name) \
		    -foreground $ftd(color,$i) \
		    -background $ftd(color,$i)
	    } else {
		$chart(gw) element create "$s" \
		    -xdata $chart(XLISTVNAME) \
		    -ydata $chart(TOV,$name) \
		    -color $ftd(color,$i) \
		    -linewidth 2 \
                    -symbol ""
	    }		
	}
	# -- delete the deleted stats from tracking in the logical group
	foreach d $dellist {
	    set t [split $d "--"]
	    set lgname [lindex $t 0]
	    set devname [lindex $t 2]
	    set statname [lindex $t 4]
	    upvar #0 $lgname group
	    set cname "CHARTS--${devname}--${statname}"
	    set clist ""
	    foreach c $group($cname) {
		if {$c != $chartname} {lappend clist $c}
	    }
	    set group($cname) $clist
	    set useflag 0
	    foreach n [array names $lgname CHARTS--* ] {
		if {[llength $group($n)] > 0} {set useflag 1}
	    }
	    set group(useflag) $useflag
	}
	# -- add the new stats to tracking in the logical group
	foreach s $newlist {
	    set t [split $s "--"]
	    set lgname [lindex $t 0]
	    set devname [lindex $t 2]
	    set statname [lindex $t 4]
	    upvar #0 $lgname group
	    set cname "CHARTS--${devname}--${statname}"
	    lappend group($cname) $chartname
	    set group(useflag) 1
	}
	# -- replace the chart stats list and vector list
	set chart(stats) $ftd(measureslist)
	set chart(vectors) ""
	foreach c $chart(stats) {
	    lappend chart(vectors) "$chart(TOV,${chartname}--$c)"
	}    
    }
    # -- deal with sample size changes
    if {$bsamples != $ftdpt_samples} {

	set redoticsflag 1
	catch "destroy $chart(mw)"
	set samplesdiff [expr $ftdpt_samples - $bsamples]
	foreach v $chart(vectors) {
	    if {$ftdpt_samples < $chart(samples)} {
		set ei [expr $chart(samples) - 1]
		set si [expr $chart(samples) - $ftdpt_samples]
		set xxx [list $v set \[ set ${v}($si:$ei) \] ]
		uplevel #0 "eval $xxx"
	    } else {
		uplevel #0 "vector create tempvector($samplesdiff)"
		uplevel #0 "vector create tempvector2"
		uplevel #0 "tempvector2 append tempvector $v"
		set xxx [list $v set \[ set tempvector2(:) \] ]
		uplevel #0 "eval $xxx"
		uplevel #0 "vector destroy tempvector"
		uplevel #0 "vector destroy tempvector2" 
	    }
	}
	set v "$chart(XLISTVNAME)"
	if {$ftdpt_samples < $chart(samples)} {
	    set ei [expr $chart(samples) - 1]
	    set si [expr $chart(samples) - $ftdpt_samples]
	    set xxx [list $v set \[ set ${v}($si:$ei) \] ]
	    uplevel #0 "eval $xxx"
	} else {
	    uplevel #0 "vector create tempvector($samplesdiff)"
	    uplevel #0 "vector create tempvector2"
	    uplevel #0 "tempvector2 append tempvector $v"
	    set xxx [list $v set \[ set tempvector2(:) \] ]
	    uplevel #0 "eval $xxx"
	    # uplevel #0 [list $v set \[set tempvector2(:)\]]
	    uplevel #0 "vector destroy tempvector"
	    uplevel #0 "vector destroy tempvector2" 
	}
	for {set i 0} {$i < $ftdpt_samples} {incr i} {
	    uplevel #0 "set ${v}($i) $i"
	}
	if {$ftdpt_samples < $chart(samples)} {
	    set si [expr ($samplesdiff * -1) - 1]
	    set chart(xlist) [lrange $chart(xlist) $si end]
	} else {
	    for {set i 0} {$i < $samplesdiff} {incr i} {
		set chart(xlist) [concat [list {}] $chart(xlist)]
	    }
	}
	set chart(samples) $ftdpt_samples
	if {$ftdpt_charttype == "Line"} {
	    set min 0
	    set max [expr $chart(samples) - 1]
	} else {
	    set min -0.5
	    set max [expr $chart(samples) - 0.5]
	}
    }
    # == mutate chart types here
    if {$ftdpt_charttype != $btype} {
	set chart(type) $ftdpt_charttype
	if {$ftdpt_charttype == "Bar" && $btype == "Stacked Bar"} {
	    $chart(gw) configure -barmode aligned
	    set min -0.5
	    set max [expr $chart(samples) - 0.5]
	} elseif {$ftdpt_charttype == "Stacked Bar" && $btype == "Bar"} {
	    $chart(gw) configure -barmode stacked
	    set min -0.5
	    set max [expr $chart(samples) - 0.5]
	} else {
	    set chart(type) $ftdpt_charttype
	    # -- delete, then recreate the chart widget
	    set geom [winfo geometry $chart(chartw)]
	    if {[info commands $chart(mw)] != ""} {
		catch "destroy $chart(mw)"
	    }
	    catch "pack forget $chart(gw)"
	    set bt [bindtags $chart(gw)]
	    bindtags $chart(gw) $chart(gw)
	    catch "destroy $chart(gw)"
	    switch -exact -- $chart(type) {
		"Line" {
		    graph $chart(gw) \
			-plotbackground aliceblue \
			-height 4i \
			-bufferelements 0
                    $chart(gw) marker configure -hide 1
		    set i -1
		    foreach stat $chart(stats) {
			incr i
			if {$i > 9} {set i 0}
			$chart(gw) element create "$stat" \
			    -xdata $chart(XLISTVNAME) \
			    -ydata $chart(TOV,$chartname--$stat) \
			    -color $ftd(color,$i) \
			    -linewidth 2 \
                            -symbol ""

		    }
		    set min 0
		    set max [expr $chart(samples) - 1]
		}
		"Bar" {
		    barchart $chart(gw) \
			-plotbackground aliceblue \
			-barwidth .9 \
			-barmode aligned \
			-height 4i \
			-bufferelements 0
		    set i -1
		    foreach stat $chart(stats) {
			incr i
			if {$i > 9} {set i 0}
			$chart(gw) element create "$stat" \
			    -xdata $chart(XLISTVNAME) \
			    -ydata $chart(TOV,$chartname--$stat) \
			    -foreground $ftd(color,$i) \
			    -background $ftd(color,$i)
		    }
		    set min -0.5
		    set max [expr $chart(samples) - 0.5]
		}
		"Stacked Bar" {
		    barchart $chart(gw) \
			-plotbackground aliceblue \
			-barwidth .9 \
			-barmode stacked \
			-height 4i \
			-bufferelements 0
		    set i -1
		    foreach stat $chart(stats) {
			incr i
			if {$i > 9} {set i 0}
			$chart(gw) element create "$stat" \
			    -xdata $chart(XLISTVNAME) \
			    -ydata $chart(TOV,$chartname--$stat) \
			    -foreground $ftd(color,$i) \
			    -background $ftd(color,$i)
		    }
		    set min -0.5
		    set max [expr $chart(samples) - 0.5]
		}
	    }
	    pack $chart(gw) -expand y -fill both
	    bindtags $chart(gw) $bt
	    $chart(gw) configure -title ""
	    $chart(gw) yaxis configure -title "" \
		-min 0 \
		-logscale no
	    set redoticsflag 1
	}
    }
    # == do we need to redo x-axis tic annotation? 
    if {$redoticsflag} {
	set majtic 0.0
	set lasttic [expr $chart(samples) - 1]
	if {$chart(samples) > 1} {lappend majtic [expr $lasttic * 1.0]}
	if {[expr $lasttic%3] == 0} {
	    lappend majtic [expr int($lasttic/3) * 1.0]
	    lappend majtic [expr int(($lasttic/3)*2) * 1.0]
	} elseif {[expr $lasttic%4] == 0} {
	    lappend majtic [expr int($lasttic/4) * 1.0]
	    lappend majtic [expr int(($lasttic/4)*3) * 1.0]
	}
	if {[expr $lasttic%2] == 0 || $chart(samples) > 9} {
	    lappend majtic [expr int($lasttic/2) * 1.0]
	}
	set majtic [lsort -real $majtic]
	$chart(gw) xaxis configure \
	    -command "ftdpt_fmtx $chartname" \
	    -min $min \
	    -max $max \
	    -title "" \
	    -logscale no \
	    -majorticks $majtic
	$chart(gw) grid on
	$chart(gw) legend configure -position bottom
	$chart(gw) xaxis configure -title ""
	set chart(mw) $chart(gw).p
	tixPopupMenu $chart(mw) -title "Chart Options:" 
	$chart(mw) bind $chart(gw)
	set menu [$chart(mw) subwidget menu]
	$menu add command -label "Edit Chart        " \
	    -command "ftdpt_modify_chart $chartname"
	$menu add command -label "Delete Chart        " \
	    -command "ftdpt_delete_chart $chartname"
	$menu add command -label "Print Chart         " \
	    -command "ftdpt_print_chart $chartname"
	$menu add command -label "Print Chart to File " \
	    -command "ftdpt_print_file_chart $chartname"
    }
    catch "wm deiconify $chart(chartw)"
    # Re-establish the <Destroy> binding that was unhooked above
    bind $chart(chartw) <Destroy> $destroywbind
    bind $chart(iw) <Destroy> $destroyiwbind

    bind $chart(gw) <Button-1> [list +ftdpt_modify_chart $chartname ; ]
    set ftd(lock) 0
}

#--------------------------------------------------------------
proc ftdpt_create_new_chart {} {
    global ftd
    global ftdpt_title ftdpt_charttype ftdpt_loggrp ftdpt_device ftdpt_stat
    global ftdpt_samples
#    if {$ftd(lock) == 1} {
#	puts stderr "GAAAAAAAAAACK lock already set:2"
#	set level [info level]
#	for {set i 1} {$i < $level} {incr i} {
#	    puts stderr "Level $i: [info level $i]"
#	}
#    }
    set ftd(lock) 1
    append chartname "chart" $ftd(curchart)
    lappend ftd(charts) $chartname
    upvar #0 $chartname chart
    set chart(updateflag) 0
    set chart(inuseflag) 1
    set chart(chartw) ".$chartname"
    set chart(title) $ftdpt_title
    set chart(type) $ftdpt_charttype
    set chart(samples) $ftdpt_samples
    set chart(stats) $ftd(measureslist)
    set chart(vecnames) ""
    foreach stat $chart(stats) {
	set t [split $stat "--"]
	set lgname [lindex $t 0]
	set devname [lindex $t 2]
	set statname [lindex $t 4]
	upvar #0 $lgname lg
	set lg(useflag) 1
	set lg(DATA--$devname--$statname) 0.0
	lappend lg(CHARTS--$devname--$statname) $chartname
    }
    ftdpt_create_chart_widget
    set ftd(lock) 0
}
#--------------------------------------------------------------
proc ftdpt_create_chart_widget {} {
    global ftd env 
    global ftdpt_title ftdpt_charttype ftdpt_loggrp ftdpt_device ftdpt_stat
    global ftdpt_samples
    append chartname "chart" $ftd(curchart)
    upvar #0 $chartname chart
    set chart(iw) ".icon-$chartname"
    lappend ftd(chartlist) $chartname
    set ftd(curchartw) $chart(chartw)
    set ftd(curchartiw) $chart(iw)
    toplevel $chart(chartw)
    toplevel $chart(iw)
    set icon [image create photo -file $env(FTD_LIBRARY)/%CAPQ%logo47.gif]
    label $chart(iw).a -image $icon
    pack $chart(iw).a
    wm overrideredirect $chart(iw) 1
    wm withdraw $chart(iw)
    wm geometry $chart(iw) 47x47
    wm deiconify $chart(iw)
    wm withdraw $chart(chartw)
    wm iconwindow $chart(chartw) $chart(iw)
 
    #
    # The original code bound ftdpt_delete_chart to the <Destroy>
    # event at this point.  But graph and barchart below generate
    # <Destroy> events for some unknown reason.  So do the
    # binding later
    #
##    bind $chart(chartw) <Destroy> {after cancel $afterid}
##    bind $chart(iw) <Destroy> {after cancel $afterid}
#    bind $chart(chartw) <Unmap> "ftdpt_delete_chart $chartname"
#    bind $chart(iw) <Unmap> "ftdpt_delete_chart $chartname"
    wm withdraw $chart(chartw)
    wm title $chart(chartw) $chart(title)
    set offset [expr 200 + ($ftd(curchart) * 20)]
    wm geometry $chart(chartw) "+${offset}+${offset}"
    wm minsize $chart(chartw) 500 300
    set name "$chartname--xlist"
    set vname "[new_vec_name]xlist"
    set chart(XLISTVNAME) $vname
    global $vname
    uplevel #0 vector create ${vname}($chart(samples))
    for {set i 0} {$i < $chart(samples)} {incr i} {
	lappend chart(xlist) {}
	set ${vname}($i) $i
    }
    foreach stat $chart(stats) {
        set name "${chartname}--${stat}"
        set vname [new_vec_name]
        lappend chart(vecnames) $vname
        set chart(TOV,$name) $vname
        set chart(TON,$vname) $name
	global $name
	uplevel #0 vector create ${vname}($chart(samples))
	lappend chart(vectors) $vname
    }
    set chart(gw) $chart(chartw).g
    switch -exact -- $chart(type) {
	"Line" {
	    graph $chart(gw) \
		-plotbackground aliceblue \
		-height 4i \
		-bufferelements 0
            $chart(gw) marker configure -hide 1
	    set i -1
	    foreach stat $chart(stats) {
		incr i
		if {$i > 9} {set i 0}
		$chart(gw) element create "$stat" \
                        -xdata $chart(XLISTVNAME) \
                        -ydata $chart(TOV,${chartname}--${stat}) \
                        -color $ftd(color,$i) \
                        -linewidth 2 \
                        -symbol ""
	    }
	    set min 0
	    set max [expr $chart(samples) - 1]
	}
	"Bar" {
	    barchart $chart(gw) \
                    -plotbackground aliceblue \
                    -barwidth .9 \
                    -barmode aligned \
                    -height 4i \
                    -bufferelements 0
	    set i -1
	    foreach stat $chart(stats) {
		incr i
		if {$i > 9} {set i 0}
		$chart(gw) element create "$stat" \
                        -xdata $chart(XLISTVNAME) \
                        -ydata $chart(TOV,${chartname}--${stat}) \
                        -foreground $ftd(color,$i) \
                        -background $ftd(color,$i)
	    }
	    set min -0.5
	    set max [expr $chart(samples) - 0.5]
	}
	"Stacked Bar" {
	    barchart $chart(gw) \
                    -plotbackground aliceblue \
                    -barwidth .9 \
                    -barmode stacked \
                    -height 4i \
                    -bufferelements 0
	    set i -1
	    foreach stat $chart(stats) {
		incr i
		if {$i > 9} {set i 0}
		$chart(gw) element create "$stat" \
                        -xdata $chart(XLISTVNAME) \
                        -ydata $chart(TOV,${chartname}--${stat}) \
                        -foreground $ftd(color,$i) \
                        -background $ftd(color,$i)
	    }
	    set min -0.5
	    set max [expr $chart(samples) - 0.5]
	}
    }
    pack $chart(gw) -expand y -fill both
    #
    # Since graph and barchart above generate <Destroy> events, 
    # the Destroy bindings cannot be set before now
    #
    bind $chart(chartw) <Destroy> "ftdpt_delete_chart $chartname"
    bind $chart(iw) <Destroy> "ftdpt_delete_chart $chartname"
    $chart(gw) configure -title ""
    $chart(gw) yaxis configure -title "" \
            -min 0 \
            -logscale no
    set majtic 0.0
    set lasttic [expr $chart(samples) - 1]
    if {$chart(samples) > 1} {lappend majtic [expr $lasttic * 1.0]}
    if {[expr $lasttic%3] == 0} {
	lappend majtic [expr int($lasttic/3) * 1.0]
	lappend majtic [expr int(($lasttic/3)*2) * 1.0]
    } elseif {[expr $lasttic%4] == 0} {
	lappend majtic [expr int($lasttic/4) * 1.0]
	lappend majtic [expr int(($lasttic/4)*3) * 1.0]
    }
    if {[expr $lasttic%2] == 0 || $chart(samples) > 9} {
	lappend majtic [expr int($lasttic/2) * 1.0]
    }
    set majtic [lsort -real $majtic]
    $chart(gw) xaxis configure \
            -command "ftdpt_fmtx $chartname" \
            -min $min \
            -max $max \
            -title "" \
            -logscale no \
            -majorticks $majtic
    $chart(gw) grid on
    $chart(gw) legend configure -position bottom
    $chart(gw) xaxis configure -title ""
    set chart(mw) $chart(gw).p
    tixPopupMenu $chart(mw) -title "Chart Options:" 
    $chart(mw) bind $chart(gw)
    set menu [$chart(mw) subwidget menu]
    $menu add command -label "Edit Chart        " \
            -command "ftdpt_modify_chart $chartname"
    $menu add command -label "Delete Chart        " \
            -command "ftdpt_delete_chart $chartname"
    $menu add command -label "Print Chart         " \
            -command "ftdpt_print_chart $chartname"
    $menu add command -label "Print Chart to File " \
            -command "ftdpt_print_file_chart $chartname"
    wm deiconify $chart(chartw)
    bind $chart(gw) <Button-1> [list +ftdpt_modify_chart $chartname ; ]
}

#--------------------------------------------------------------
proc ftdpt_fmtx {chartname W i args} {
    upvar #0 $chartname chart
    return [lindex $chart(xlist) $i]
}

#--------------------------------------------------------------
proc ftdpt_modify_chart {chartname} {
    global ftd
    global ftdpt_title ftdpt_charttype ftdpt_loggrp ftdpt_device ftdpt_stat
    global ftdpt_samples
    upvar #0 $chartname chart
    $chart(chartw).g configure -plotbackground yellow
    set x [after 250 "$chart(chartw).g configure -plotbackground aliceblue"] 
    set chartno [string range $chartname 5 end]
    set ftd(curchart) $chartno
    set ftdpt_title $chart(title)
    set ftdpt_charttype $chart(type)
    set ftdpt_samples $chart(samples)
    $ftd(measuresw) delete 0 end
    foreach stat $chart(stats) {
	$ftd(measuresw) insert end $stat
    }
    set ftd(measureslist) $chart(stats)
    $ftd(displaychartw) configure -text "Update Chart"
}

#--------------------------------------------------------------
proc ftdpt_delete_chart {{chartname ""} {newflag 1}} {
    global ftd
    if {$chartname == ""} {
	set chartname "chart$ftd(curchart)"
    }
    if {[info globals $chartname] == ""} {return}
    upvar #0 $chartname chart
    if {[info exists chart(beingdeleted)]} {return}
    set chart(beingdeleted) 1
    if {[info exists chart(chartw)]} {
	bind $chart(chartw) <Destroy> {}
    }
    if {[info exists chart(iw)]} {
	bind $chart(iw) <Destroy> {}
    }
    # -- set the lock variable
#    if {$ftd(lock) == 1} {
#	puts stderr "GAAAAAAAAAACK lock already set:3"
#	set level [info level]
#	for {set i 1} {$i < $level} {incr i} {
#	    puts stderr "Level $i: [info level $i]"
#	}
#    }
    set ftd(lock) 1
    # -- walk through the stats in the chart to get to the logical groups
    # -- and remove references to this chart from the logical group array
    foreach stat $chart(stats) {
	set t [split $stat "--"]
	set lg [lindex $t 0]
	set dev [lindex $t 2]
	set statname [lindex $t 4]
	upvar #0 $lg group
	set item "CHARTS--${dev}--${statname}"
	set t ""
	foreach cname $group($item) {
	    if {$cname != $chartname} {lappend t $cname}
	}
	set group($item) $t
	# -- see if the group has fallen out of scrutiny as no charts displayed
	# -- from it
	set useflag 0
	foreach n [array names group CHARTS--*] {
	    if {[llength $group($n)] > 0} {set useflag 1}
	}
	set group(useflag) $useflag
    }
    # -- remove the vectors associated with this chart
    foreach v $chart(vectors) {
	uplevel #0 "vector destroy $v"
    }
    if {[info exists chart(XLISTVNAME)]} {
	uplevel #0 "vector destroy $chart(XLISTVNAME)"
    }
    # -- delete the toplevel widgets (chart and icon)
    if {[info exists chart(chartw)]} {catch "destroy $chart(chartw)"}
    if {[info exists chart(iw)]} {catch "destroy $chart(iw)"}
    # -- get rid of the chart array
#    catch "unset $chartname"
    catch "unset chart"
    # -- remove chart from masterlist
    set clist ""
    foreach c $ftd(charts) {
	if {$c != $chartname} {lappend clist $c}
    }
    set ftd(charts) $clist
    set ftd(chartlist) $clist
    # -- unlock things
    set ftd(lock) 0
    if {$newflag} {ftdpt_create_chart}
}

#--------------------------------------------------------------
proc ftdpt_print_chart {chartname} {
    upvar #0 $chartname chart
    if {[catch "$chart(gw) postscript output /tmp/${chartname}.ps -maxpect no -decorations no" msg]} {
	ftdpt_display_error "Error Printing Chart:  $msg"
	return
    }
    if {[catch "exec /bin/lp -c /tmp/${chartname}.ps" msg]} {
	ftdpt_display_error "Error Printing Chart:  $msg"
	return
    }
    catch "exec /bin/rm /tmp/${chartname}.ps"
}

#--------------------------------------------------------------
proc ftdpt_print_file_chart {chartname} {
    global psfilename
    upvar #0 $chartname chart
    set types {
	{{Postscript Files} {.ps}}
	{{All Files} *}
    }
    set psfilename [tk_getSaveFile -defaultextension .ps \
	-title "Save Postscript File" \
	-filetypes $types]
    if {"$psfilename" == ""} {return}
    if {[catch "$chart(gw) postscript output $psfilename -maxpect no -decorations no"]} {
	ftdpt_display_error "Error Printing Chart to File:  $psfilename"
    }
}

#--------------------------------------------------------------
proc ftdpt_build_data {} {
    global ftd
    global tcl_platform
    
    # -- have conflicting primary and secondary perf files win to primary
    set pvals ""
    set svals ""
    set pripaths "[lsort [glob -nocomplain -- /%FTDVAROPTDIR%/p???.prf]]"
    foreach p $pripaths {
        set l [string length $p]
        set p [string range $p [expr $l - 7] [expr $l - 5]]
        lappend pvals $p
    }
    set secpaths "[lsort [glob -nocomplain -- /%FTDVAROPTDIR%/s???.prf]]"
    foreach s $secpaths {
        set l [string length $s]
        set s [string range $s [expr $l - 7] [expr $l - 5]]
        lappend svals $s
    }
    foreach p $pvals {
        if {-1 != [set idx [lsearch -exact $svals $p]]} {
            set svals [lreplace $svals $idx $idx]
        }
    }
    set ppaths ""
    foreach p $pvals {lappend ppaths "p$p"}
    foreach s $svals {lappend ppaths "s$s"}
    foreach path "$ppaths" {
	set lgdigits [string range $path 1 3]
        set lgprefix [string range $path 0 0]
	scan $lgdigits "%d" lgnum
	set lgname "${lgprefix}${lgdigits}"
        set ftd(lgname,$lgnum) $lgname
	upvar #0 $lgname lg
	set lg(useflag) 0
	set lg(size) -1
	# -- figure out performance file layout
	set gotstatnames 0
	set lg(lastts) ""
	if {[file exists "/%FTDVAROPTDIR%/$lgname.phd"]} {
	    set fd [open /%FTDVAROPTDIR%/$lgname.phd r]
	    while {![eof $fd]} {
		set line [gets $fd]
		if {[llength $line] <= 1} {continue}
		if {[string index $line 0] == "#"} {continue}
		lappend lg(statinfo) $line
		set gotstatnames 1
	    }
	    close $fd
	}
	if {!$gotstatnames} {
	    foreach line $ftd(stdstats) {lappend lg(statinfo) $line}
	}
	# -- figure out devices for the logical group and set
	# -- current values to initial values
        global waitdone
        set waitdone 0
        while {0 == [file exists /%FTDVAROPTDIR%/$lgname.prf] && \
                $waitdone < 10} {
            after 1000 {global waitdone; incr waitdone}
        } 
        unset waitdone
        if {$tcl_platform(os) == "Linux"} {
            set line [exec /usr/bin/tail -1 /%FTDVAROPTDIR%/$lgname.prf]
        } else {
            set line [exec /bin/tail -1 /%FTDVAROPTDIR%/$lgname.prf]
        }
        set i 1
        set linelen [llength $line]
        if {$linelen == 0} {
            catch "unset lg"
            continue
        }
        lappend ftd(loggrps) $lgname
        set names [lrange $lg(statinfo) 1 end]
        while {$linelen > [llength $lg(statinfo)]} {
            set lg(statinfo) [concat $lg(statinfo) $names]
        }
	set lg(statinfo) [lrange $lg(statinfo) 0 [expr $linelen - 1]]
	set lg(devices) ""
	set i 0
	set devname ""
	foreach item $lg(statinfo) {
	    regsub -all { } [lindex $item 0] {_} name
	    set type [lindex $item 1]
	    if {$name == "Device"} {
		set devname [lindex $line $i]
		lappend lg(devices) $devname
	    }
	    if {$devname != "" && $type == "y"} {
		set lg(DATA--$devname--$name) "0.0"
		set lg(CHARTS--$devname--$name) ""
	    }
	    incr i
	}
    }
}

#--------------------------------------------------------------
proc ftdpt_watch_for_data_updates {} {
    global ftd afterid
    global tcl_platform
    if {!$ftd(lock)} {
	set ftd(lock) 1
	foreach grp $ftd(loggrps) {
	    upvar #0 $grp lg
	    if {!$lg(useflag)} {continue}
	    if {[catch [list file size "/%FTDVAROPTDIR%/${grp}.prf"] size]} {
		set size 0
	    }
	    if {$lg(size) != $size && $size != 0} {
		set lg(size) $size
                if {$tcl_platform(os) == "Linux"} {
		    set line [exec /usr/bin/tail -1 /%FTDVAROPTDIR%/${grp}.prf]
                } else { 
                    set line [exec /bin/tail -1 /%FTDVAROPTDIR%/${grp}.prf]
                }
		set device ""
		foreach item $lg(statinfo) data $line {
		    regsub -all { } [lindex $item 0] {_} name
		    set type [lindex $item 1]
		    switch -exact -- $type {
			"x" {
			    set lg(lastts) $data
			}
			"n" {set device $data}
			"0" { }
			"y" {
			    if {$device != ""} {
				set lg(DATA--$device--$name) $data
				foreach chartname $lg(CHARTS--$device--$name) {
				    upvar #0 $chartname chart
				    set chart(updateflag) 1
				    set chart(lastts) $lg(lastts)
				}
			    }
			}
		    }
		}
	    }
	}
	foreach chartname $ftd(charts) {
	    upvar #0 $chartname chart
	    if {![info exists chart(updateflag)]} {
		continue
		#set chart(updateflag) 0
	    }
	    if {$chart(updateflag)} {
		set chart(updateflag) 0
		foreach v $chart(vectors) {
                    set t [split $chart(TON,$v) "--"]
		    set cn [lindex $t 0]
		    set grp [lindex $t 2]
		    set dev [lindex $t 4]
		    set stat [lindex $t 6]

		    upvar #0 $grp group
		    set s "DATA--$dev--$stat"
                    #set v $chart(TOV,$v)
		    global $v
		    set k [expr $chart(samples) - 1]
		    for {set i 0} {$i < $k} {incr i} {
			set j [expr $i + 1]
			set ${v}($i) [set ${v}($j)]
		    }
		    set ${v}($k) $group($s)
		}
		set chart(xlist) [lrange $chart(xlist) 1 end]
		lappend chart(xlist) $chart(lastts)
		$chart(chartw).g xaxis configure -title ""
	    }
	}
	set ftd(lock) 0
    }
    set afterid [after 1000 {ftdpt_watch_for_data_updates}]
}

#--------------------------------------------------------------
proc ftdpt_exit {{exit_called 0}} {
    global ftdpt_exit_called
    global afterid
    # avoid calling exit multiple times within exit context
    set ftdpt_exit_called [expr $ftdpt_exit_called + 1]
    if {$afterid != ""} {
	catch {after cancel $afterid}
	set afterid ""
    }
    if {$ftdpt_exit_called == 1} {
        exit
    }
}
#--------------------------------------------------------------
proc new_vec_name {} {
    global vecseed
    incr vecseed
    return "VECTOR${vecseed}"
}

#--------------------------------------------------------------
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
    pack $w.%Q%image -side top -pady 0 -padx 0 
    set msg [replaceRTM "%PRODUCTNAME% V%VERSION%\nLicensed Materials / Property of IBM, \xA9 Copyright %COMPANYNAME%\n%COPYRIGHTYEAR 2001%. All rights reserved.  IBM, Softek, and %PRODUCTNAME% registered or common law trademarks\nof %COMPANYNAME% in the United States, other countries, or both."]
    message $w.msg2 -text $msg                  -aspect 10000 -justify center -font -Adobe-Helvetica-Bold-R-Normal--*-100-*-*-*-*-*-* -bg white
    pack $w.msg2 -side left -padx 3

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


# -- start 'er up
wm withdraw .
Introduction
ftdpt_build_form
ftdpt_watch_for_data_updates
