#
# Copyright (c) %COPYRIGHTYEAR 2001% %COMPANYNAME%. All rights reserved.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
# Government is subject to restrictions as set forth in
# subparagraph (c)(1)(ii) of the Rights in Technical Data and
# Computer Software clause at DFARS 52.227-7013 and in similar
# clauses in the FAR and NASA FAR Supplement.
#
#
source select.tcl
source qdswizhelp

global cfg

#-------------------------------------------------------------------------
#
# QdsConfigWiz Namespace
#
# This namespace contains the procedures needed to define the Datastar 
# configuration wizard.
#
#-------------------------------------------------------------------------
namespace eval QdsConfigWiz {

 
    variable cfg
    variable curmirror
    variable curqds
    variable cursec
    variable curlg
    variable selectwidget
    variable availwidget
    global WIZ
    namespace import ::QdsNetwork::*
   
}

#-------------------------------------------------------------------------
#
# FormatList
#
# Formats a list of {path size} pairs (in which path is a string path, and
# size is an integer size in kilobytes) into a nice format for the listboxes
#
#-------------------------------------------------------------------------
proc QdsConfigWiz::FormatList {list} {

    set fslist ""

    foreach i $list { 
	set fsname [lindex $i 0]
	set fssize [lindex $i 1]
	if {$fssize > 1000.0} {
	    lappend fslist [format "%-30s(%.1f MB)"\
		    $fsname [expr $fssize / 1000.0]]
	} else {
	    lappend fslist [format "%-30s(%.1f KB)"\
		    $fsname $fssize]
	}
    }
    return $fslist
}

#-------------------------------------------------------------------------
#
# frame00
#
# This frame provides a simple introduction.
#
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame00 {} {

    global ::Wizard::WIZ 

    set WIZ(maintitle) "Qualix DataStar"
    set WIZ(subtitle) "Configuration Assistant"
    set WIZ(titlebar) "Qualix DataStar Configuration"

    set WIZ(text) "The following dialogs will guide you step-by-step through\
	    the process of configuring Datastar.  Before\
	    you begin you must read Chapter 1, \"Planning for DataStar\
	    Installation\".  This information will aid you in configuring\
	    DataStar properly for your particular environment."
   
    set WIZ(backProc) ""
    set WIZ(commit) {
	set curlg 0
    }
    set WIZ(nextProc) {frame01}
    set WIZ(exitProc) "exit"

}; # end frame00


#-------------------------------------------------------------------------
#
# frame01
#
# Inputs:  Primary Host Name or IP Address
#
# Outputs: 
#         QdsConfigWiz::cfg(primary) -  hostname or IP of primary
#         QdsConfigWIz::curmirror - initialized to 0 on "Next"
#
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame01 {} {

    global ::Wizard::WIZ
    variable cfg 
    variable curmirror

    if {![info exists cfg(primary)]} {
	set cfg(primary) ""
    }

    set WIZ(step) "Step 1: "
    set WIZ(title) "Define the Primary System"

    set p $WIZ(parent); # abbreviation

    set WIZ(text) "The Primary System is the system that provides primary\
	    application and data storage services to your users.   You\
	    will configure this system to mirror to one or more secondary\
	    systems."
    
    frame $p.containF
    frame $p.leftF
    frame $p.rightF
    label $p.hostL -text "Host Name or IP Address:" 
    entry $p.hostE -width 45  \
	    -textvariable ::QdsConfigWiz::cfg(primary) \
	    -background white
    pack $p.hostL -in $p.leftF -anchor e
    pack $p.hostE -in $p.rightF -anchor w
    pack $p.leftF -pady 5 -side left -in $p.containF
    pack $p.rightF -pady 5 -side left -in $p.containF
   
    label $p.passL -text "DataStar Password:" 
    entry $p.passE -width 45  \
	    -textvariable ::QdsConfigWiz::cfg(primarypass) \
	    -background white -show *
    pack $p.passL -pady 5 -in $p.leftF -anchor e
    pack $p.passE -pady 5 -in $p.rightF -anchor w 
    pack $p.containF
  

    
    #---------------------------------------------
    # Make sure that we can resolve primary
    #---------------------------------------------   
    proc verify {} { 
	global  ::Wizard::WIZ ::QdsConfigWiz::cfg
	
	if { !$::Wizard::WIZ(online)} { 
	    return 0
	}
	qds_dialog .waitD "Please Wait"\
		"Connecting to $cfg(primary)..." hourglass -1 -1
	
	$WIZ(toplevel) config -cursor watch
	update
	after 1000
	set s [::QdsNetwork::Open_connection $cfg(primary)\
		$cfg(primarypass) 2540]
	    
	destroy .waitD
	$WIZ(toplevel) config -cursor left_ptr

	if {$s == -1} {  
	    if {[string compare $::QdsNetwork::error "PASSFAIL"] == 0} {
		qds_dialog .w Error "Authorization Failed." {} 0 Ok
		return -1 
	    } else {
		set sel [qds_dialog .w Error "Could not connect to\
			$::QdsConfigWiz::cfg(primary)" {} 0 Ok Help]
		if {$sel==1} {
		    ::Wizard::showHelp $::HELP(connect_error)
		}
		return -1
	    }
	} else { 
	    set cfg(sock) $s
	    return 0
	}
    }
	
    set WIZ(verify) { 
	verify
    }
    set WIZ(commit)  {
	set curmirror 0
    }

    set WIZ(nextProc) "frame01a"
    set WIZ(backProc) "frame00"
    set WIZ(exitProc) "exit"

}; # end frame01 
#-------------------------------------------------------------------------
#
# frame01
#
# Inputs: Secondary System Names and passwords
#
# Outputs: 
#         QdsConfigWiz::cfg(primary) -  hostname or IP of primary
#         QdsConfigWIz::curmirror - initialized to 0 on "Next"
#
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame01a {} {

    global ::Wizard::WIZ
    variable cfg 
    variable curmirror

    if {![info exists cfg(primary)]} {
	set cfg(primary) ""
    }

    set WIZ(step) "Step 2: "
    set WIZ(title) "Define Secondary Systems"

    set p $WIZ(parent); # abbreviation

    set WIZ(text) "You will now define the Secondary system(s) that will\
	    mirror the Primary system's data."


 
    frame $p.containF 
 
    label $p.hostL -text "Host Name or IP Address:" 
    entry $p.hostE -width 60  \
	    -textvariable ::QdsConfigWiz::cfg(secondary) \
	    -background white
    pack $p.hostL -anchor w -in $p.containF
    pack $p.hostE -anchor w -in $p.containF
   
  
    label $p.passL -text "DataStar Password:" 
    entry $p.passE -width 60  \
	    -textvariable ::QdsConfigWiz::cfg(secondarypass) \
	    -background white -show *
    pack $p.passL -anchor w -in $p.containF
    pack $p.passE  -anchor w -in $p.containF
    pack $p.containF -pady .5c
    frame $p.buttF 
    button $p.addB -text "Add" -command [namespace code {
	if {$::Wizard::WIZ(online)} {
	    qds_dialog .waitD "Please Wait"\
		    "Testing connection between $cfg(primary) and\
		    $cfg(secondary)..." hourglass -1 -1
	    
	    $::Wizard::WIZ(toplevel) config -cursor watch
	    
	    update
	    after 2000
	    set response [::QdsNetwork::Send_command $cfg(sock)\
		    10000 "Assert_host $cfg(secondary)" ]
	    
	    destroy .waitD
	    
	    $::Wizard::WIZ(toplevel) config -cursor left_ptr
	    
	    if {$response==-1} {
		if {[string compare $::QdsNetwork::error "TIMEOUT"] == 0} {
		    qds_dialog .w Error "Connection Timeout" {} 0 Ok
		} else {
		    set sel [qds_dialog .w Error "$cfg(primary) could not connect\
			    to $cfg(secondary)."\
			    {} 0 Ok Help]
		    if {$sel==1} { 
			::Wizard::showHelp $HELP(resolve_sec_error)
		    }
		    
		}
		
	    } else {
		qds_dialog .okD "Test Successful"\
			"Connection between $cfg(primary) and\
			$cfg(secondary) ok." info  0 Ok
		update
		
		qds_dialog .waitD "Please Wait"\
			"Connecting to $cfg(secondary)..."\
			hourglass -1 -1
		update
		
		$::Wizard::WIZ(toplevel) config -cursor watch
		update
		
		after 2000
		set s [::QdsNetwork::Open_connection $cfg(secondary)\
			$cfg(secondarypass) 2540]
		
		destroy .waitD
		$::Wizard::WIZ(toplevel) config -cursor left_ptr
		
		if {$s == -1} { 
		    if {[string compare $::QdsNetwork::error "PASSFAIL"] == 0} {
			qds_dialog .w Error "Authorization failed."\
				{} 0 Ok Help
			
		    } else {
			set sel [qds_dialog .w Error "Could not connect to\
				$::QdsConfigWiz::cfg(secondary)"\
				{} 0 Ok Help]
			if {$sel==1} { 
			    ::Wizard::showHelp $HELP(connect_error)
			}
		    }
			
		} else { 
		    set cfg(secondary,sock) $s
		    $::Wizard::WIZ(parent).selectLB insert end $cfg(secondary)
		}
	    }
	
	} else {
	    $::Wizard::WIZ(parent).selectLB insert end $cfg(secondary)
	}
    }]
    
    button $p.remB -text "Remove" -command   {
	foreach i [ lsort -decreasing -integer [ \
		$::Wizard::WIZ(parent).selectLB\
		curselection ]] {
	    $::Wizard::WIZ(parent).selectLB delete $i
	}
    }
     
    pack $p.addB -in $p.buttF -side left -padx .5c 
    pack $p.remB -in $p.buttF -side left  -padx .5c
    pack $p.buttF -anchor c -in $p.containF -pady 5

    frame $p.selectF
    label $p.selectL -text "Secondary Systems(s):"

    listbox $p.selectLB -yscrollcommand "$p.selectSB set" \
	    -height 10  -selectmode extended\
	    -width 60\
	    -selectbackground black  -selectborderwidth 2\
	    -selectforeground grey \
	    -background white
   
    scrollbar $p.selectSB -orient v -command "$p.selectLB yview"
    pack $p.selectL -in $p.selectF -side top -anchor w 
    pack $p.selectLB -side left -expand 1 -fill x -in $p.selectF
    pack $p.selectSB -side right -fill y -in $p.selectF
    pack $p.selectF  -anchor w -in $p.containF

    if {[info exists cfg(secondarylist)]} {
	foreach item $cfg(secondarylist) {
	    $p.selectLB insert end $item
	}
    }
    set WIZ(commit)  {
	set cfg(secondarylist) [$::Wizard::WIZ(parent).selectLB get 0 end]
	set curmirror 0
    }

    set WIZ(nextProc) "frame02"
    set WIZ(backProc) "frame00"
    set WIZ(exitProc) "exit"

}; # end frame01 



#-------------------------------------------------------------------------
#
# frame02
#
# User Input: Choice of Filesystem Mirror or Driver-level Mirror
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,type) - type of mirror (qdsfs or qds dev)
#       QdsConfigWIz::curmirror - decremented on "Back" (unless this is 
#                                                        Mirror 0)
#
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame02 {} {
   
    global ::Wizard::WIZ
    variable cfg
    variable curmirror
    variable curqds
 
    set WIZ(step) "Step 3: "
    set WIZ(title) "Specify Mirror Type"             

    set p $WIZ(parent); # abbreviation
    
    set par $p
    set WIZ(text) "You can mirror your data at the filesystem level\
	    or the device level.  If you choose to mirror at the filesystem\
	    level you will create a qdsfs filesystem that will be layered\
	    on top of a currently existing filesystem.  If you choose to\
	    mirror at the device level, a qds device will be created that\
	    will be layered on top of the device that you want to mirror."
    
    if {![ info exists cfg($curmirror,type)]} {
	set cfg($curmirror,type) "fs"
    } 
 
    set cfg($curmirror,oldtype) $cfg($curmirror,type)
    frame $p.noteF
    label $p.noteL -text "Description/notes for this mirror: " 
	    
    if {![info exists cfg($curmirror,notes)]} {
	set cfg($curmirror,notes) ""
    }

    entry $p.noteE -width 60 \
	    -textvariable ::QdsConfigWiz::cfg($curmirror,notes) \
	    -background white

    pack $p.noteL $p.noteE -side top -in $p.noteF -anchor w
    pack $p.noteF -pady .5c -anchor w
    frame $p.select
    pack $p.select -expand 1 -fill x
    
    radiobutton $p.select.yes -variable \
	    ::QdsConfigWiz::cfg($curmirror,type) \
	    -value "fs" -text "Create a Filesystem Mirror" \
	     -highlightthickness 0

    radiobutton $p.select.no -variable\
	    ::QdsConfigWiz::cfg($curmirror,type) \
	    -value "dev" \
	    -text "Create a Device Mirror" \
	    -highlightthickness 0

    pack  $p.select.yes $p.select.no -anchor w
 
    set WIZ(backCmd) {
	if { $curmirror > 0 } {
	    set ::Wizard::WIZ(backProc) "frame10"
	    set curmirror [expr $curmirror - 1]
	} else {
	    set ::Wizard::WIZ(backProc) "frame01a"
	}
    }
    
    set WIZ(commit) {
	if {[string compare $cfg($curmirror,type) "dev"] == 0 } {
	    if {![info exists curqds]} {
		set curqds 0
	    } else {
		incr curqds
	    }
	    set ::Wizard::WIZ(nextProc) "frame03"
	    set cfg($curmirror,qdsdev) $curqds
	} else {
	    set ::Wizard::WIZ(nextProc) "frame03b"
	}
	if {[string compare $cfg($curmirror,oldtype) $cfg($curmirror,type)]\
		!=0} {
	    catch {unset cfg($curmirror,localdata)}
	    catch {unset cfg($curmirror,localdatasize)}
	}
    }
	  
    set WIZ(nextProc) "frame03"   
    set WIZ(exitProc) "exit" 
}; # end frame01b

#-------------------------------------------------------------------------
#
# frame02
#
# User Input: Description for a Mirror 
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,notes) - notes for mirror 
#       QdsConfigWIz::curmirror - decremented on "Back" (unless this is 
#                                                        Mirror 0)
# Notes:
#       Next frame depends on mirror type - frame 3 is for qds devices
#       frame 3b for qdsfs
#
#-------------------------------------------------------------------------
set x { proc QdsConfigWiz::frame02 {} {
#
#    global ::Wizard::WIZ 
#    variable cfg
#    variable curmirror   
#    variable curqds

#    set WIZ(step) "Step 2.2: " 
#    set WIZ(title) "Create Mirror:  Describe Mirror Usage"

#    set p $WIZ(parent); # abbreviation

#    set WIZ(text) "You should now give a description of the data that will\
	    be mirrored by this mirror.  This will allow you to easily\
	    identify the mirror during Datastar administration."

#    frame $p.noteF
#    label $p.noteL -text "Description/notes for this mirror: " 
	    
#    if {![info exists cfg($curmirror,notes)]} {
	set cfg($curmirror,notes) ""
    }

    entry $p.noteE -width 60 \
	    -textvariable ::QdsConfigWiz::cfg($curmirror,notes) \
	    -background white

    pack $p.noteL $p.noteE -side top -in $p.noteF -anchor w
    pack $p.noteF -pady .25c -anchor w

    set WIZ(backCmd) {
	if {[string compare $cfg($curmirror,type) "dev" ] == 0} {
	    if { $curqds == 0 } { 
		unset curqds
	    } else {
		set curqds [expr $curqds - 1]
		
	    }
	}
    }

    set WIZ(commit) {
	if {[string compare $cfg($curmirror,type) "dev" ] == 0} {
	    set ::Wizard::WIZ(nextProc) "frame03"
	    set cfg($curmirror,qdsdev) $curqds
	} else {
	    set ::Wizard::WIZ(nextProc) "frame03b"
	}
    }

    set WIZ(backProc) "frame01b"
    set WIZ(exitProc) "exit" 

}; # end frame02
}
#-------------------------------------------------------------------------
#
# frame03
#
# User Input: Source Device (Local Data Device)
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,localdata) - source device selection
#   
#
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame03 {} {

    global ::Wizard::WIZ p
    variable cfg
    variable curmirror
    variable curqds

    set WIZ(step) "Step 4: " 
    set WIZ(title) "Specify Source Device"
    set p $WIZ(parent)

    set WIZ(text) "You will now define a source device on the primary system\
	    for qds$curqds."

    if {![info exists cfg($curmirror,localdata)]} {
	set cfg($curmirror,localdata) ""
	set cfg($curmirror,localdatasize) ""
    }
    if {![info exists cfg(devs)]} {
	if {$WIZ(online)} {
	    set cfg(devs) [Send_command $cfg(sock) 10000 "Get_avail_devs"]
	  
	} else {
	    set cfg(devs) ""
	}
    }
    set devlist [FormatList $cfg(devs)]

    frame $p.select
    pack $p.select -anchor w
    Select::makeSelect $p.select 5 50 $devlist\
	    ::QdsConfigWiz::cfg($curmirror,localdata)\
	    ::QdsConfigWiz::cfg($curmirror,localdatasize)\
	    "Available Devices:" "Selected Local Data Device:"
 
   
    set WIZ(commit) {}
    set WIZ(backCmd) {
	if { $curqds == 0 } { 
	 unset curqds
	} else {
	    set curqds [expr $curqds - 1]
	    
	}
    }

    set WIZ(nextProc) "frame04"
    set WIZ(backProc) "frame02"
    set WIZ(exitProc) "exit" 

}; # end frame03

#-------------------------------------------------------------------------
#
# frame03b
#
# User Input: Source Path for qdsfs 
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,localdata) - source path selection
#   
#
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame03b {} {

    global ::Wizard::WIZ p
    variable cfg
    variable curmirror

    set WIZ(step) "Step 4: " 
    set WIZ(title) "Specify Source Filesystem"
    set p $WIZ(parent)

    set WIZ(text) "You will now specify which filesystem you would like\
	    to mirror."

    if {![info exists cfg($curmirror,localdata)]} {
	set cfg($curmirror,localdata) ""
	set cfg($curmirror,localdatasize) ""
    } 

    if {![info exists cfg(fs) ]} {
	if {$WIZ(online)} {
	    set cfg(fs) [Send_command $cfg(sock) 10000 "Get_mounts"]
	} else {
	    set cfg(fs) ""
	}
    }

    set fslist [FormatList $cfg(fs)]
    frame $p.select  
 
    pack $p.select -anchor w

    Select::makeSelect $p.select 5 50 $fslist\
	    ::QdsConfigWiz::cfg($curmirror,localdata)\
	    ::QdsConfigWiz::cfg($curmirror,localdatasize)\
	    "Mounted Filesystems:" "Selected Filesystem:"
  
    set WIZ(commit) {}

    set WIZ(nextProc) "frame04"
    set WIZ(backProc) "frame02"
    set WIZ(exitProc) "exit" 

}; # end frame03b

#-------------------------------------------------------------------------
#
# frame04
#
# User Input: Primary  Devices
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,wlavail) - wl devices in available list
#       QdsConfigWiz::cfg($curmirror,wllist) - wl devices selected
#   
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame04 {} {

    global ::Wizard::WIZ
    variable cfg 
    variable mirrorname
    variable selectwidget
    variable availwidget
    variable curmirror
    variable curqds


    set WIZ(step) "Step 2.4: " 
    set WIZ(title) "Create Mirror: Specify "
    set p $WIZ(parent)
    
    if {[string compare $cfg($curmirror,type) "dev"] == 0 } { 
	set mirrorname qds$curqds
    } else {
	set mirrorname "mirrorring $cfg($curmirror,localdata)"
    }

    set WIZ(text) "Specify one or more writelog devices to be used for\
	    $mirrorname."

    if {![ info exists cfg($curmirror,wllist)]} {
	set cfg($curmirror,wllist) ""
    }
    if {![info exists cfg($curmirror,wlavail)]} {
	if {$WIZ(online)} {
	    set wlavail                 \
		    [Send_command $cfg(sock) 10000 "Get_avail_devs"]
	    set cfg($curmirror,wlavail) [FormatList $wlavail]
	} else {
	    set cfg($curmirror,wlavail) ""
	}
    }
    
    frame $p.select        
    pack $p.select
    if {$WIZ(online)} {
	set widgets [Select::makeAvailSelect $p.select 10 40 \
	    $cfg($curmirror,wlavail) $cfg($curmirror,wllist) \
	    "Available Devices:" "Selected Writelog Device(s):" ]
	set availwidget [lindex $widgets 0]
	set selectwidget [lindex $widgets 1]
    }  else { 
	set selectwidget [Select::makeEnterSelect $p.select 10 60 \
		$::QdsConfigWiz::cfg($curmirror,wllist) \
		"Writelog Device:" "Selected Writelog Device(s):"]
    }


    #---------------------------------------------
    # Make sure that some Writelogs were selected
    #---------------------------------------------   
    proc verify {} { 
	variable selectwidget
	variable availwidget
	variable cfg
	variable curmirror
	global ::Wizard::WIZ

	if {$WIZ(online) } {
	    set cfg($curmirror,wllist) [ $selectwidget get 0 end ]
	    set cfg($curmirror,wlavail) [ $availwidget get 0 end ]
	} else {
	    set cfg($curmirror,wllist) [$selectwidget get 0 end]
	}

	if {[string compare $cfg($curmirror,wllist) "" ] == 0 } { 
	    qds_dialog .w Error "You must specify writelog devices." {} 0 Ok
	    return -1
	} else {
	    return 0
	}
    }
	
    set WIZ(verify) "verify"
    set WIZ(commit) {
	    set cursec 0
    }

    set WIZ(nextProc) "frame06"

    if {[string compare $cfg($curmirror,type) "fs" ] == 0} { 
	set WIZ(backProc) "frame03b"
    } else {
	set WIZ(backProc) "frame03"
    }

    set WIZ(exitProc) "exit" 
}; # end frame04

#-------------------------------------------------------------------------
#
# frame05
#
# User Input: Secondary System Name
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,$cursec,host) - Secondary hostname
#   
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame05 {} {
    
    global ::Wizard::WIZ 
    variable cfg 
    variable curmirror  
    variable cursec
    variable mirrorname

    set WIZ(step) "Step 2.3: "
    set WIZ(title) "Create Mirror: Define Secondary Systems"
    set p $WIZ(parent)
    
    if { $cursec == 0 } {
	set WIZ(text) "You will now specify a secondary system for \
		$mirrorname. \
		When you are finished entering the information for this\
		secondary\
		system, you will have the opportunity to define additional\
		secondary systems."
    } else {
	set WIZ(text) "Specify an additional secondary system for $mirrorname."
    }

    frame $p.containF
    frame $p.leftF
    frame $p.rightF
    label $p.hostL -text "Host Name or IP Address:" 
    entry $p.hostE -width 45 \
	    -textvariable ::QdsConfigWiz::cfg($curmirror,$cursec,host) \
	    -background white
    pack $p.hostL -in $p.leftF -anchor e
    pack $p.hostE -in $p.rightF -anchor w
    pack $p.leftF -pady 5 -side left -in $p.containF
    pack $p.rightF -pady 5 -side left -in $p.containF
   
   
    label $p.passL -text "DataStar Password:"
    entry $p.passE -width 45  \
	    -textvariable ::QdsConfigWiz::cfg($curmirror,$cursec,pass) \
	    -background white -show *
    pack $p.passL -pady 5 -in $p.leftF -anchor e
    pack $p.passE -pady 5 -in $p.rightF -anchor w 
    pack $p.containF
   
    if {![info exists cfg($curmirror,$cursec,host)]} {
	set cfg($curmirror,$cursec,host) ""
    }
   
    set WIZ(commit) ""
    proc verify {} {
	global HELP ::Wizard::WIZ
	variable cfg 
	variable cursec
	variable curmirror
	if {$WIZ(online)} {
	    qds_dialog .waitD "Please Wait"\
		    "Testing connection between $cfg(primary) and\
		    $cfg($curmirror,$cursec,host)..." hourglass -1 -1
	    
	    $WIZ(toplevel) config -cursor watch
	    
	    update
	    after 2000
	    set response [::QdsNetwork::Send_command $cfg(sock) 10000 "Assert_host\
		    $cfg($curmirror,$cursec,host)" ]
	    
	    destroy .waitD
	    
	    $WIZ(toplevel) config -cursor left_ptr
	    
	    if {$response==-1} {
		if {[string compare $::QdsNetwork::error "TIMEOUT"] == 0} {
		    qds_dialog .w Error "Connection Timeout" {} 0 Ok
		} else {
		    set sel [qds_dialog .w Error "$cfg(primary) could not connect\
			    to $cfg($curmirror,$cursec,host)."\
			    {} 0 Ok Help]
		    if {$sel==1} { 
			::Wizard::showHelp $HELP(resolve_sec_error)
		    }
		    
		}
		return -1  
	    }
	    qds_dialog .okD "Test Successful"\
		    "Connection between $cfg(primary) and\
		    $cfg($curmirror,$cursec,host) ok." info  0 Ok
	    update
	    
	    qds_dialog .waitD "Please Wait"\
		    "Connecting to $cfg($curmirror,$cursec,host)..."\
		    hourglass -1 -1
	    update
	    
	    $WIZ(toplevel) config -cursor watch
	    update
	    
	    after 2000
	    set s [::QdsNetwork::Open_connection $cfg($curmirror,$cursec,host)\
		    $cfg($curmirror,$cursec,pass) 2540]
	    
	    destroy .waitD
	    $WIZ(toplevel) config -cursor left_ptr

	    if {$s == -1} { 
		if {[string compare $::QdsNetwork::error "PASSFAIL"] == 0} {
		    qds_dialog .w Error "Authorization failed."\
			    {} 0 Ok Help
		    return -1
		} else {
		    set sel [qds_dialog .w Error "Could not connect to\
			    $::QdsConfigWiz::cfg($curmirror,$cursec,host)"\
			    {} 0 Ok Help]
		    if {$sel==1} { 
			::Wizard::showHelp $HELP(connect_error)
		    }
		    return -1
		}
	    } else { 
		set cfg($curmirror,$cursec,sock) $s
		return 0
	    }
	} else {
	    return 0
	}
	
		
    }
	
	
    set WIZ(verify) "verify"
    if { [string compare $cfg($curmirror,type) "dev" ] == 0 } {
	set WIZ(nextProc) "frame06"
    } else {
	set WIZ(nextProc) "frame06b"
    }
    set WIZ(backCmd) {
	if { $cursec > 0} {
	    set ::Wizard::WIZ(backProc) "frame09"
	    set cursec [expr $cursec - 1]
	} else {
	    set ::Wizard::WIZ(backProc) "frame04"
	}
    }

    set WIZ(exitProc) "exit"
}; # end frame 05

#-------------------------------------------------------------------------
#
# frame06
#
# User Input: Mirror Device on Secondary (for qds device)
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,$cursec,mirror) - device path
#   
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame06 {} {
    
    global ::Wizard::WIZ 
    variable cfg 
    variable cursec 
    variable curmirror
    variable mirrorname

    set WIZ(step) "Step 2.3.1: "
    set WIZ(title) "Create Mirror: Secondary Systems: Mirror Device"

    set p $WIZ(parent); # abbreviation

    set WIZ(text) "You will now specify a mirror device on\
	    $cfg($curmirror,$cursec,host) for\
	    $mirrorname"

    if {![info exists cfg($curmirror,$cursec,mirror)]} {
	set cfg($curmirror,$cursec,mirror) ""
    }

    frame $p.select        
    pack $p.select -anchor w

    set devlist [FormatList $cfg(devs)]
    Select::makeSelect $p.select 5 50 $devlist\
	    ::QdsConfigWiz::cfg($curmirror,$cursec,mirror)\
	    ::QdsConfigWiz::cfg($curmirror,$cursec,mirrorsize)\
	    "Available Devices:" "Selected Mirror Device:"
    proc verify {} {
	global ::Wizard::WIZ
	variable cfg
	variable cursec
	variable curmirror 
	if {$WIZ(online)} {
	    if {$cfg($curmirror,$cursec,mirrorsize)\
		    < $cfg($curmirror,localdatasize)} {
		qds_dialog .w Error "$cfg($curmirror,$cursec,mirror) on \
			$cfg($curmirror,$cursec,host) does not\
		    have enough available space to mirror\
		    $cfg($curmirror,localdata) from $cfg(primary)." {} 0 Ok
		return -1 
	    } else {
		return 0
	    }
	} else {
	    return 0
	}

	
    }   
    
    set WIZ(verify) {verify}
    set WIZ(commit) "" 
    set WIZ(nextProc) "frame07"
    set WIZ(backProc) "frame05"
    set WIZ(exitProc) "exit" 

}; # end frame06

#-------------------------------------------------------------------------
#
# frame06b
#
# User Input: Mirror path on Secondary (for qdsfs mirror)
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,$cursec,mirror) -  path
#   
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame06b {} {
    
    global ::Wizard::WIZ 
    variable cfg 
    variable cursec 
    variable curmirror
    variable mirrorname

    set WIZ(step) "Step 2.3.1: "
    set WIZ(title) "Create Mirror: Secondary Systems: Mirror Path"
    set p $WIZ(parent)

    set WIZ(text) "You must now specify a target path on\
	    $cfg($curmirror,$cursec,host) for $mirrorname from $cfg(primary)."

    if {![info exists cfg($curmirror,$cursec,mirror)]} {
	set cfg($curmirror,$cursec,mirror) ""
    }

    frame $p.pathF        
    pack $p.pathF -anchor w

    label $p.pathF.pathL \
	    -text "Target Path on $cfg($curmirror,$cursec,host):" \
	   

    entry $p.pathF.pathE -width 50  \
	    -textvariable QdsConfigWiz::cfg($curmirror,$cursec,mirror)\
	    -background white

    pack $p.pathF.pathL $p.pathF.pathE -side top -anchor w
    proc verify {} {
	global ::Wizard::WIZ
	variable cfg
	variable cursec
	variable curmirror 
	if {$WIZ(online)} {
	    set response [::QdsNetwork::Send_command $cfg($curmirror,$cursec,sock)\
		    10000 "Assert_dir\
		    $cfg($curmirror,$cursec,mirror)\
		    $cfg($curmirror,localdatasize)" ]
	    
	    if {[string compare $response "OK"]==0} {
	    return 0
	    } elseif {[string compare $response "NOSUCHDIR"]==0} {
		qds_dialog .w Error "$cfg($curmirror,$cursec,mirror) does not\
			exist on $cfg($curmirror,$cursec,host)." {} 0 Ok
		return -1
	    } elseif {[string compare $response "TOSMALL" ]==0} {
		qds_dialog .w Error "$cfg($curmirror,$cursec,mirror) on \
			$cfg($curmirror,$cursec,host) does not\
			have enough available space to mirror\
			$cfg($curmirror,localdata) from $cfg(primary)." {} 0 Ok
		return -1
	    }
	} else {
	    return 0
	}
	
    }
    set WIZ(verify) "verify"
    set WIZ(commit) "" 
    set WIZ(nextProc) "frame07"
    set WIZ(backProc) "frame05"
    set WIZ(exitProc) "exit" 

}; # end frame 06b

#-------------------------------------------------------------------------
#
# frame07
#
# User Input: Secondary Writelog Devices
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,$cursec,secwlavail) -  avail wl devices
#       QdsConfigWiz::cfg($curmirror,$cursec,secwllist) -  selected wl devices
#   
#------------------------------------------------------------------------- 
proc QdsConfigWiz::frame07 {} {

    global ::Wizard::WIZ 
    variable cfg 
    variable cursec 
    variable curmirror
    variable selectwidget
    variable availwidget

    set WIZ(step) "Step 2.3.2: "
    set WIZ(title) "Create Mirror: Secondary Systems: Writelog (optional)"

    set p $WIZ(parent); # abbreviation
    
    set WIZ(text) "You have the option to us a writelog on the secondary\
	    system in conjunction with the mirror device.  This option will\
	    allow you to \"offline\" the mirror device while data is being\
	    sent from the primary system, therby maximizing the efficiency\
	    of your network usage during back-ups of the mirror device."
    
    if {![ info exists cfg($curmirror,$cursec,secwllist)]} {
	set cfg($curmirror,$cursec,secwllist) ""
    }

    if {![info exists cfg($curmirror,$cursec,secwlavail)]} {
	if {$WIZ(online)} {
	    set x [Send_command\
		    $cfg($curmirror,$cursec,sock) 10000 "Get_avail_devs"]
	    set cfg($curmirror,$cursec,secwlavail) [FormatList $x]
	} 
    }
    
    frame $p.select        
    pack $p.select
    if {$WIZ(online)} {
	set widgets [Select::makeAvailSelect $p.select 10 40 \
		$cfg($curmirror,$cursec,secwlavail)\
		$cfg($curmirror,$cursec,secwllist) "Available Devices:"\
		"Selected Writelog Device(s):" ]    
	set availwidget [lindex $widgets 0]
	set selectwidget [lindex $widgets 1]
    } else {
	set selectwidget [Select::makeEnterSelect $p.select 10 60 \
		$cfg($curmirror,$cursec,secwllist) \
		"Writelog Device:" "Selected Writelog Device(s):"]
    }
    
    set WIZ(commit) {
	set QdsConfigWiz::cfg($curmirror,$cursec,secwllist)\
		[ $selectwidget get 0 end ]
	if {$::Wizard::WIZ(online)} {
	    set QdsConfigWiz::cfg($curmirror,$cursec,secwlavail)\
		    [ $availwidget get 0 end ]
	} 
    }
    if { [string compare $cfg($curmirror,type) "dev" ] == 0 } {
	set WIZ(backProc) "frame06"
    } else {
	set WIZ(backProc) "frame06b"
    }
    set WIZ(nextProc) "frame09"
    set WIZ(exitProc) "exit" 
}

#-------------------------------------------------------------------------
#
# frame09
#
# User Input: Choice of whether to continue with additional secondaries,
#             or to continue on to next mirror.
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,$cursec,another) - 
#                       0:  continue with additional secondaries
#                       1:  go on to next mirror
#   
#------------------------------------------------------------------------- 
proc QdsConfigWiz::frame09 {} {

    global ::Wizard::WIZ 
    variable cfg 
    variable cursec 
    variable another
   
    variable curmirror
    
    set WIZ(step) "" 

    set WIZ(title) "Done with definition of secondary system\
	    $cfg($curmirror,$cursec,host)."

    set p $WIZ(parent); # abbreviation

   
    set WIZ(text) "Would you like to specify another secondary system for\
	    this mirror?" 

    frame $p.select        
    pack $p.select
    
    if {![info exists cfg($curmirror,$cursec,another)]} {
	set cfg($curmirror,$cursec,another) 1
    }

    radiobutton $p.select.yes \
	    -variable ::QdsConfigWiz::cfg($curmirror,$cursec,another)\
	    -value 0 -text "Yes"  -highlightthickness 0

    radiobutton $p.select.no\
	    -variable ::QdsConfigWiz::cfg($curmirror,$cursec,another)\
	    -value 1 -text "No" -highlightthickness 0

    pack  $p.select.yes $p.select.no -anchor w
    
    set WIZ(commit) {
	if { $cfg($curmirror,$cursec,another) == 0 } { 
	    set ::Wizard::WIZ(nextProc) "frame05" 
	    incr cursec 
	} else { 
	    set ::Wizard::WIZ(nextProc) "frame10" 
	}
    }
    set WIZ(backProc) "frame07"
    set WIZ(exitProc) "exit" 
}; # end frame09

#-------------------------------------------------------------------------
#
# frame10
#
#      Displays information input about previous mirror for verification.
#
# User Input: Choice of whether to continue with additional mirrors.  
#
# Outputs: 
#       QdsConfigWiz::cfg($curmirror,another) - 
#                       0:  continue with additional mirrors
#                       1:  go on to logical groups
#   
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame10 {} {
    global ::Wizard::WIZ 
    variable cfg 
    variable cursec 
    variable curmirror

    set WIZ(step) "" 
    set WIZ(title) "Done with mirror definition"
    set p $WIZ(parent)

    if { [string compare $cfg($curmirror,type) "fs" ] == 0 } {
	set mtype "Filesystem"
    } else {
	set mtype "Device"
    }

    set WIZ(text) "Please verify the following mirror information before\
	    proceeding:" 
    

    frame $p.infoF 
    text $p.infoF.infoT -width 65 -height 10 \
	    -yscrollcommand "$p.infoF.infoSB set" 
    scrollbar $p.infoF.infoSB -orient v -command "$p.infoF.infoT yview"
    pack $p.infoF.infoT -side left -fill both -expand 1
    pack $p.infoF.infoSB -side left -fill y 
    pack $p.infoF

    $p.infoF.infoT insert end "Description: $cfg($curmirror,notes)\nType:\
	    $mtype\nSource: $cfg($curmirror,localdata) on\
	    $cfg(primary)\nWritelog:"
   
    foreach item $cfg($curmirror,wllist) {
	$p.infoF.infoT insert end "\n\t$item"
    }
 
    set sec 0
 
    while {[info exists cfg($curmirror,$sec,host)]} {
	$p.infoF.infoT insert end \
		"\nTarget:\n\t$cfg($curmirror,$sec,mirror)\
		on $cfg($curmirror,$sec,host)\n\tWritelog:"

	if {[string compare $cfg($curmirror,$sec,secwllist) "" ] != 0  } {
	    foreach item $cfg($curmirror,$sec,secwllist) {
		$p.infoF.infoT insert end "\n\t\t\t$item"
	    } 
	} else {
	    $p.infoF.infoT insert end " None."
	}

	incr sec
    }

    $p.infoF.infoT configure -state disabled

    label $p.anotherL -text "Do you want to create another mirror for\
	    $cfg(primary)?"
    pack $p.anotherL -pady .5c -anchor w
    frame $p.select        
    pack $p.select

    if {![info exists cfg($curmirror,another)]} {
	set cfg($curmirror,another) 1
    }
    radiobutton $p.select.yes\
	    -variable ::QdsConfigWiz::cfg($curmirror,another)\
	    -value 0\
	    -text "Yes" \
	    -highlightthickness 0

    radiobutton $p.select.no -variable\
	    ::QdsConfigWiz::cfg($curmirror,another)\
	    -value 1 \
	    -text "No"\
	    -highlightthickness 0

    pack  $p.select.yes $p.select.no -anchor w
    
    set WIZ(commit) {
	if { $cfg($curmirror,another) == 0 } { 
	    set ::Wizard::WIZ(nextProc) "frame01b"
	    incr curmirror
	} else {
	    set ::Wizard::WIZ(nextProc) "frame11"
	}
    }
    
    set WIZ(backProc) "frame09"
    set WIZ(exitProc) "exit" 
}; # end frame10

#-------------------------------------------------------------------------
#
# frame11
#
# User Input: Choice of -  1) all mirrors in one logical group (default)
#                          2) each mirror in a separate logical group
#                          3) Custom configuration 
#
# Outputs: 
#       QdsConfigWiz::cfg(grouping) - 0,1, or 2 
#   
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame11 {} {

    global ::Wizard::WIZ 
    variable cfg 
    variable cursec  
    variable curlg

    variable curqds

    set WIZ(step) "Step 3:" 
    set WIZ(title) "Create Replication Groups"
    set p $WIZ(parent)

    set WIZ(text) "Datastar groups qds devices together into a collection of\
	    devices called a replication group.  All devices in a replication group\
	    will maintain chronological coherency, that is that no device in\
	    the group will be more up to date than any other device in the\
	    group at any given time.  It is essential that you define all\
	    devices used by a particular application (a database, of example)\
	    into one replication group to maintain this coherency.  You may want\
	    to define several replication groups to obtain a greater aggregate\
	    network throughput if you have several devices that you want to\
	    mirror that are used for different purposes." 

    frame $p.select        
    pack $p.select

    if {![info exists cfg(grouping)]} {
	set cfg(grouping) 0
    }

    radiobutton $p.select.onelg -variable ::QdsConfigWiz::cfg(grouping)\
	    -value 0 \
	    -text "Put all qDS devices in one replication group" \
	    -highlightthickness 0
    radiobutton $p.select.separatelg -variable ::QdsConfigWiz::cfg(grouping)\
	    -value 1 \
	    -text "Put each qDS device in its own replication group" \
	    -highlightthickness 0
    radiobutton $p.select.chooselg -variable ::QdsConfigWiz::cfg(grouping)\
	    -value 2\
	    -text "Let me group the qDS devices." \
	    -highlightthickness 0

    pack  $p.select.onelg $p.select.separatelg $p.select.chooselg -anchor w
    
    set WIZ(commit) {
	if {$cfg(grouping) == 2} {
	    set curlg 0
	    set ::Wizard::WIZ(nextProc) "frame12"
	} elseif {$cfg(grouping) == 1} {
	    set mirrornum 0
	    while {[info exists cfg($mirrornum,localdata)]} {
		set cfg($mirrornum,lgnum) $mirrornum
		incr mirrornum
	    }
	    set ::Wizard::WIZ(nextProc) "frame13"
	} else {
	    set mirrornum 0
	    while {[info exists cfg($mirrornum,localdata)]} {
		set cfg($mirrornum,lgnum) 0
		incr mirrornum
	    }
	}
    }
    set WIZ(backProc) "frame10"
    set WIZ(exitProc) "exit" 

}; # end frame11

#-------------------------------------------------------------------------
#
# frame12
#
# User Input: Custom Grouping of Mirrors into logical groups
#
# Outputs: 
#       QdsConfigWiz::cfg(availmirrors) - available mirrors
#       QdsConfigWiz::cfg($curlg,selmirrors)  - selected for current group
#   
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame12 {} {

    global ::Wizard::WIZ 
    variable cfg 
    variable curlg
    variable availwidget
    variable selectwidget

    set WIZ(step) "Step 3.1: " 
    set WIZ(title) "Create Replication Groups: Group Mirrors"
    set p $WIZ(parent)

    set WIZ(text) ""
    
    frame $p.notesF
    label $p.notesF.notesL \
	    -text "Description/Notes for Replication Group $curlg:"
    text $p.notesF.notesT -width 100 -height 3 -bd 2  -wrap word \
	    -highlightthickness 2 \
	    -state normal \
	    -background white

    pack $p.notesF.notesL -anchor w
    pack $p.notesF.notesT -anchor w
    pack $p.notesF -anchor w -pady .25c

    #
    #  create list of available mirrors
    #  we know each device must have a writelog, so
    #  we check if a wl has been defined for each qds #
    #
    if {![info exists cfg(availmirrorlist)]} {
	set cfg(availmirrorlist) ""
	set mirrornum 0 
	while {[info exists cfg($mirrornum,wllist)]} {
	    if {[string compare $cfg($mirrornum,type) "dev"]==0} {
		lappend cfg(availmirrorlist)\
			"/dev/rdsk/qds$cfg($mirrornum,qdsdev)"
	    } else {
		lappend cfg(availmirrorlist)\
			"$cfg($mirrornum,localdata)"
	    }
	    incr mirrornum
	}
    }

    if {![info exists cfg($curlg,selmirrorlist)]} {
	set cfg($curlg,selmirrorlist) ""
    }

    frame $p.selectF        

    label $p.selectF.selL \
	    -text "Select mirrors for Replication Group $curlg:"
    pack $p.selectF.selL -anchor w
    pack $p.selectF

    set widgets [Select::makeAvailSelect $p.selectF 10 40\
	    $cfg(availmirrorlist) $cfg($curlg,selmirrorlist)\
	    "Available Mirror(s):"\
	    "Selected Mirror(s):" ]

    set availwidget [lindex $widgets 0]
    set selectwidget [lindex $widgets 1]
   

    set WIZ(commit) {
	set cfg($curlg,selmirrorlist) [ $selectwidget get 0 end ]
	set cfg(availmirrorlist) [ $availwidget get 0 end ]
	if {[string compare $cfg(availmirrorlist) ""]!=0} {
	    set WIZ(nextProc) "frame12"
	    incr curlg
	} else {
	    set WIZ(nextProc) "frame13"
	}
    }

    set WIZ(backProc) "frame11"
    set WIZ(exitProc) "exit" 
}; # end frame12

#-------------------------------------------------------------------------
#
# frame13
#
# User Input: Creation of Writelog Pools
#
# Outputs: 
#       
#   
#-------------------------------------------------------------------------
proc QdsConfigWiz::frame13 {} {

    global ::Wizard::WIZ

    set WIZ(step) "Step 5:"
    set WIZ(title) "Create a Writelog Pool (optional)"
    set WIZ(titlebar) "Qualix DataStar Configuration"
    set p $WIZ(parent)

    
    set WIZ(text) "You will now have the opportunity to define a\
	    writelog pool. A writelog pool is a group of devices on the\
	    primary that can be used for additional writelog space in the\
	    event that a qDS device's writelog becomes full." 

    set WIZ(nextProc) ""
    set WIZ(backProc) ""
    set WIZ(exitProc) "exit"
    
}; # end frame 13



