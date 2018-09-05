#!/%FTDLIBEXECDIR%/%Q%wish
#-----------------------------------------------------------------------------
#
# %Q%configtool -- %PRODUCTNAME% Configuration Tool
#
# Copyright (c) 2001 %COMPANYNAME%
# All Rights Reserved.
#
#-----------------------------------------------------------------------------

# -- define necessary enviroment variables
global env
if {![info exists env(DISPLAY)]} {
    puts stderr "DISPLAY environment variable is not set"
    exit
}
set env(PROD_LIBRARY) /%FTDLIBDIR%/%Q%%REV%

set SYS(platform) $tcl_platform(os)
set SYS(osVersion) $tcl_platform(osVersion)

# -- dynamic loadable kernel for HPUX 11.xx, currently set to no
# -- HPUX 10.20 does not support DLKM
global dlkm
set dlkm 0

# DTurrin - With all the updates happening in HP-UX, we now have
#           to make sure that we support the OS Version where
#           the software is being installed. (August 16th, 2001)
if { $SYS(platform) == "HP-UX"} {
    set hpOsVers [list "B.10.20" "B.10.30" "B.11.00" "B.11.11"]
    if {![lsearch -exact hpOsVers $SYS(osVersion)]} {
        puts stderr "Cannot initialize BAB. %CAPQ% driver is not supported on HP-UX $SYS(osVersion)."
        exit
    }
}

#
# Set some platform specific variables
#
if {$SYS(platform) == "HP-UX"} {
    set SYS(rebootcmd) "shutdown -y -r"
    set SYS(conffile) "/etc/opt/%PKGNM%/%Q%.conf"
    set SYS(whoami) "/usr/bin/whoami"
    set SYS(hostname) "/usr/bin/hostname"
    set SYS(services) "/etc/services"
    set SYS(defjournal) ""
    #option add *disabledForeground grey50
    if {($SYS(osVersion) == "B.10.20") || ($SYS(osVersion) == "B.10.30")} {
        set dlkm  0
    }

} elseif {$SYS(platform) == "SunOS"} {
    set SYS(conffile) "/usr/kernel/drv/%Q%.conf"
    set SYS(whoami) "/usr/ucb/whoami"
    set SYS(hostname) "/bin/hostname"
    set SYS(services) "/etc/inet/services"
    set SYS(defjournal) ""
} elseif {$SYS(platform) == "AIX"} {
    set SYS(conffile) "/usr/lib/drivers/%Q%.conf"
    set SYS(whoami) "/bin/whoami"
    set SYS(hostname) "/bin/hostname"    
    set SYS(services) "/etc/services"
    set SYS(defjournal) ""
}

#
# Source in tcl utilities for managing device and process info
#
catch "source $env(PROD_LIBRARY)/%Q%confutil.tcl"
catch "source $env(PROD_LIBRARY)/%Q%migratecfg.tcl"

global defaulttunable
global defaultsys

#
# Min and Maximum sizes for BAB in bytes
#
set SYS(minbab) 32
set SYS(maxbab) 1536

#
# Set some system-wide (not LG specific) defaults
# 
set defaultsys(bab) 512
set defaultsys(chunksize) 256
set defaultsys(tcpport) 575
set defaultsys(tcpbufsize) 262144

#
# Set default tunable parameters
#
set defaulttunable(CHUNKSIZE:) 1024
set defaulttunable(CHUNKDELAY:) 0 
set defaulttunable(SYNCMODE:) off
set defaulttunable(SYNCMODEDEPTH:) 1
set defaulttunable(SYNCMODETIMEOUT:) 30
set defaulttunable(STATINTERVAL:) 10
set defaulttunable(MAXSTATFILESIZE:) 64
set defaulttunable(TRACETHROTTLE:) off
set defaulttunable(NETMAXKBPS:) -1
set defaulttunable(COMPRESSION:) off
set tunabletype(SYNCMODE:) ONOFF
set tunabletype(TRACETHROTTLE:) ONOFF
set tunabletype(COMPRESSION:) ONOFF

global %Q%devnewflag
set %Q%devnewflag 1
global PRE
global argv argc RMDtimeout CFGfilecheck BALOONhelpflag
set CFGfilecheck 1
set RMDtimeout 120000
set BALOONhelpflag 1
set SYS(onprimary) 1

#
# Process arguments
#
if {$argc > 0} {
    set i 0
    while {$i < $argc} {
	switch -glob -- [string tolower [lindex $argv $i]] {
	    -rmdtimeout {
		if {$i < [expr $argc - 1]} {
		    incr i
		    set t [lindex $argv $i]
		    incr i
		    if {![catch "expr $t * 1"]} {
			if {$t < 5} {set t 5}
			if {$t > 600} {set t 600}
			set RMDtimeout [expr $t * 1000] 
		    }
		}
		break
	    }
	    -noconfigchk {
		set CFGfilecheck 0
		incr i
	    }
            -notprimary {
                set SYS(onprimary) 0
                incr i
            }
	    -nohelp {
		set BALOONhelpflag 0
		incr i
	    }
	    default {
		puts stderr "usage:"
		puts stderr "\$ %Q%configtool \[-rmdtimeout <secs>\] \[-noconfigchk\] \[-nohelp\]"
		puts stderr " "
		exit
	    }
	}
    }
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
#-------------------------------------------------------------------------
#
#  %Q%_dialog {w title text msgwidth bitmap default args}
#
#  Creates a dialog with the specified message, bitmap, and buttons.
#  
#  Returns:  Index of button clicked ( O being leftmost button )
#-------------------------------------------------------------------------
proc %Q%_dialog {w title text msgwidth bitmap default args} {
    global tkPriv tcl_platform

    # 1. Create New top-level window and divide it into top
    # and bottom parts.

    catch {destroy $w}
    toplevel $w -class Dialog
    
    wm title $w $title
 
    wm iconname $w Dialog
    wm protocol $w WM_DELETE_WINDOW { }

    # The following command means that the dialog won't be posted if
    # [winfo parent $w] is iconified, but it's really needed;  otherwise
    # the dialog can become obscured by other windows in the application,
    # even though its grab keeps the rest of the application from being used.

    wm transient $w [winfo toplevel [winfo parent $w]]
    if {$tcl_platform(platform) == "macintosh"} {
	unsupported1 style $w dBoxProc
    }

    frame $w.bot
    frame $w.top
    if {$tcl_platform(platform) == "unix"} {
	$w.bot configure -relief raised -bd 1
	$w.top configure -relief raised -bd 1
    }
    pack $w.bot -side bottom -fill both
    pack $w.top -side top -fill both -expand 1

    # 2. Fill the top part with bitmap and message (use the option
    # database for -wraplength so that it can be overridden by
    # the caller).
    label $w.msg -justify left -text $text -wraplength $msgwidth
    option add *Dialog.msg.wrapLength 3i widgetDefault
    pack $w.msg -in $w.top -side right -expand 1 -fill both -padx 3m -pady 3m
    if {$bitmap != ""} {
	if {($tcl_platform(platform) == "macintosh") && ($bitmap == "error")} {
	    set bitmap "stop"
	}
	label $w.bitmap -bitmap $bitmap
	pack $w.bitmap -in $w.top -side left -padx 3m -pady 3m
    }

    # 3. Create a row of buttons at the bottom of the dialog.

    set i 0
    if {$args != -1} {
	foreach but $args {
	    button $w.button$i -text $but -command "set tkPriv(button) $i"
	    if {$i == $default} {
	    #$w.button$i configure -default active
	    } else {
		#$w.button$i configure -default normal
	    }
	    grid $w.button$i -in $w.bot -column $i -row 0 -sticky ew \
		    -padx 10 -pady .25c
	    grid columnconfigure $w.bot $i
	    # We boost the size of some Mac buttons for l&f
	    if {$tcl_platform(platform) == "macintosh"} {
		set tmp [string tolower $but]
		if {($tmp == "ok") || ($tmp == "cancel")} {
		    grid columnconfigure $w.bot $i -minsize [expr 59 + 20]
		}
	    }
	    incr i
	}
    }

    # 4. Create a binding for <Return> on the dialog if there is a
    # default button.

    if {$args != -1} {
	if {$default >= 0} {
	    bind $w <Return> "
	    $w.button$default configure -state active -relief sunken
	    update idletasks
	    after 100
	    set tkPriv(button) $default
	    "
	}

	# 5. Create a <Destroy> binding for the window that sets the
	# button variable to -1;  this is needed in case something happens
	# that destroys the window, such as its parent window being destroyed.
    
	bind $w <Destroy> {set tkPriv(button) -1}

    }

    wm withdraw $w
    update idletasks
    set x [expr [winfo screenwidth $w]/2 - [winfo reqwidth $w]/2 \
	    - [winfo vrootx [winfo parent $w]]]
    set y [expr [winfo screenheight $w]/2 - [winfo reqheight $w]/2 \
	    - [winfo vrooty [winfo parent $w]]]
    wm geom $w +$x+$y
    wm deiconify $w
    
    # 7. Set a grab and claim the focus too.
    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab $w"
    if {$default >= 0} {
	focus $w.button$default
    } else {
	focus $w
    }

   
    update 
    update idletasks
    if {$args != -1} {
	# 8. Wait for the user to respond, then restore the focus and
	# return the index of the selected button.  Restore the focus
	# before deleting the window, since otherwise the window manager
	# may take the focus away so we can't redirect it.  Finally,
	# restore any grab that was in effect.
	
	tkwait variable tkPriv(button)
	
	catch {focus $oldFocus}
	catch {
	    # It's possible that the window has already been destroyed,
	    # hence this "catch".  Delete the Destroy handler so that
	    # tkPriv(button) doesn't get reset by it.

	    bind $w <Destroy> {}
	    destroy $w
	}
	if {$oldGrab != ""} {
	    if {$grabStatus == "global"} {
		catch "grab -global $oldGrab"
	    } else {
                catch "grab $oldGrab"
	    }
	}
	return $tkPriv(button)
    } 
   
 
    update 
    update idletasks
}

#-------------------------------------------------------------------------
#
#  displayError {{msg ""} {msgwidth 4i}}
#
#  Displays an error message of the specified width with an Ok button
#  
#  Returns:  Nothing
#-------------------------------------------------------------------------
proc displayError {{msg ""} {msgwidth 4i}} {
    if {[winfo exists .errorD]} {
	catch {destroy .errorD}
    } 
    %Q%_dialog .errorD Error $msg $msgwidth warning 0 Ok
}

#-------------------------------------------------------------------------
#
#  displayInfo {{msg ""} {onoff ""} {msgwidth 4i}}
#
#  Displays or destroys a message dialog with no buttons. 
#  
#  Returns:  Nothing
#-------------------------------------------------------------------------
proc displayInfo {{msg ""} {onoff ""} {msgwidth 4i}} {
    global SYS savecursor In_Intro
   
    if {!$In_Intro} {
	if {$onoff == "off" || $onoff == "OFF"} {
	    catch {destroy .infoD}
	    . configure -cursor {}
	    update
	    return
	} elseif {$onoff == "on" || $onoff == "ON"} {
	    . configure -cursor watch
	    %Q%_dialog .infoD "Please Wait" $msg $msgwidth hourglass -1 -1
	    update 
            update idletasks
	    return
	} else {
	    %Q%_dialog .infoD "Information" $msg $msgwidth info 0 Ok
	}
	update
	update idletasks
    } else {
	if {$onoff == "off" || $onoff == "OFF"} {
	    .introT.f0.infoM configure -text ""
	    . configure -cursor {}
	    update 
	    update idletasks
	    return
	} elseif {$onoff == "on" || $onoff == "ON"} {
	    . configure -cursor watch
	   
	    .introT.f0.infoM configure -text $msg
	    update idletasks
	    update
	    after 1000
	    return
	} else {
	    %Q%_dialog .infoD "Information" $msg $msgwidth info 0 Ok
            update 
            update idletasks
	}
    }
   
}

#-------------------------------------------------------------------------
# 
#  readconfigs {{interferflag 0}}
#
#  Reads configuration files
#  
#  Returns:  Nothing
#-------------------------------------------------------------------------
proc readconfigs {{interferflag 0}} {
    global SYS defaultsys PRE MIG_INFO

    set SYS(primaryport) $defaultsys(tcpport)
    set SYS(tcpbufsize) $defaultsys(tcpbufsize)
    set SYS(cfgfiles) ""
    set SYS(loggrps) ""
    set SYS(numprofile) 0
    set SYS(errors) ""
    set SYS(interferflag) $interferflag
    set SYS(candidatecfg) ""
    set SYS(candidatelg) ""
    set SYS(rbab) [expr $defaultsys(bab) * 1024 * 1024]
    set SYS(bab) $defaultsys(bab)
    set SYS(needinit) 0
    if {$SYS(onprimary) } {
        set SYS(needinit) 1
        if {![catch {open $SYS(conffile) r} fd ]} {
            if {![catch {read $fd} %Q%conf ]} {
	        set chunksizeexist [ regexp {chunk_size=([0-9]*)}\
                  $%Q%conf x SYS(chunksize)]
                set numchunksexist [regexp {num_chunks=([0-9]*)}\
                  $%Q%conf x SYS(numchunk)]
                if {$chunksizeexist && $numchunksexist} {
                    # if we find the chunksize and number of chunks, multiply
                    # to get bab size
                    set SYS(needinit) 0
                    if { $SYS(numchunk) > 2048 } {
                        set SYS(rbab) [expr  $SYS(numchunk) / 2 * $SYS(chunksize) ]
                    } else {
                        set SYS(rbab) [expr $SYS(numchunk) *\
                                    $SYS(chunksize) ]
                    }
                } else {
                    set SYS(needinit) 1
                    #
                    # didn't find those, set it to default
                    # 
                    set SYS(chunksize) $defaultsys(chunksize)
                    # -- see if .cfg/.conf were cached from a previous 
                    # -- product installation
                    set x [migratecfgs]
                    if {$MIG_INFO(babsize) > 0} {
                        set SYS(numchunk) $MIG_INFO(num_chunks)
                        set SYS(chunksize) $MIG_INFO(chunk_size)
                        set SYS(bab) $MIG_INFO(babsize)
                        set SYS(rbab) [expr $MIG_INFO(babsize) * 1024 * 1024]
                        set PRE(bab) $SYS(bab)
                        set SYS(needinit) 1
                    }
                    if {$MIG_INFO(FTSWftdflag) && "$MIG_INFO(pstoredev)" != ""} {
                        set SYS(ps) $MIG_INFO(pstoredev)
                        set PRE(ps) $MIG_INFO(pstoredev)
                    }
                }
                
                if {[info exists SYS(numchunk)] } {
                    if {$SYS(numchunk) > 2048} {
                        set SYS(bab) [expr round ( $SYS(rbab) / (1024 * 1024 / 2) )] 
                    } else {
                        set SYS(bab) [expr round ( $SYS(rbab) / (1024 * 1024) )]                  
                    }
                } else {
                    set SYS(bab) [expr round ( $SYS(rbab) / (1024 * 1024) )]
                }
            
                if {[regexp {tcp_window_size=([0-9]*)} $%Q%conf x\
                        bufsize ]} {
                    set SYS(tcpbufsize) $bufsize
            }
            
        } else {
            # -- see if .cfg/.conf files were cached from a previous 
            # -- product installation
            set x [migratecfgs]
            if {$MIG_INFO(babsize) > 0} {
                set SYS(numchunk) $MIG_INFO(num_chunks)
                set SYS(chunksize) $MIG_INFO(chunk_size)
                set SYS(bab) $MIG_INFO(babsize)
                set SYS(rbab) [expr $MIG_INFO(babsize) * 1024 * 1024]
                set PRE(bab) $SYS(bab)
                set SYS(needinit) 1
            }
            if {$MIG_INFO(FTSWftdflag) && "$MIG_INFO(pstoredev)" != ""} {
                set SYS(ps) $MIG_INFO(pstoredev)
                set PRE(ps) $MIG_INFO(pstoredev)
            }
        }
    }
    # -- read primary port # from /etc/services
    if {![ catch {set servfd [open $SYS(services) r]}]} {
        set lines [split [read $servfd] \n]
        if {[set i [lsearch -regexp $lines "in\.%Q%"]]!=-1} {
            set portstr [lindex [lindex $lines $i] 1]
            if {[regexp {[0-9]*} $portstr portnum]} {
                set SYS(primaryport) $portnum
            }
        }
    }
    
}
    # -- parse the configuration file names
    foreach fname [lsort [glob -nocomplain "/%OURCFGDIR%/p\[0-9\]\[0-9\]\[0-9\].cfg"]] {
        
	set cfgfile [file rootname [lindex [file split $fname] end]]
	if {-1 == [lsearch -exact $SYS(cfgfiles) $cfgfile]} {
	    lappend SYS(cfgfiles) $cfgfile
	}
	scan [string range $cfgfile 1 4 ] "%d" loggrpno
	if {-1 == [lsearch -exact $SYS(loggrps) $loggrpno]} {
	    lappend SYS(loggrps) $loggrpno
	}
	#set result [readconfig $loggrpno]
    }
   
    if {$SYS(errors) != ""} {
	reportconfigerrors
    }
    set SYS(candidatelg) ""
    set SYS(candidatecfg) ""
  
}

#-------------------------------------------------------------------------
# 
#  readconfig {lgnumber} 
#  Reads the configuration file of a particular logical group
#  
#  Returns:  Nothing
#-------------------------------------------------------------------------
proc readconfig {lgnumber} {
    global SYS defaulttunable CFGfilecheck tunabletype defaultsys
    set havesystems 0 
   
    set SYS(candidatelg) $lgnumber
    set cfgfile [format "p%03d" $lgnumber]
    set SYS(candidatecfg) $cfgfile
    displayInfo "Processing Configuration File:  /%OURCFGDIR%/$cfgfile" ON
    # -- remove all global device variables
    foreach g [info globals "%Q%*"] {
        global $g
        unset $g
    }
    global $cfgfile curtag
    catch "unset $cfgfile"
    set ${cfgfile}(NOTES:) ""
    upvar #0 ${cfgfile} cfg
 
    if {-1 == [lsearch -exact $SYS(cfgfiles) $cfgfile]} {
	lappend SYS(cfgfiles) $cfgfile
    }
    if {-1 == [lsearch -exact $SYS(loggrps) $lgnumber]} {
	lappend SYS(loggrps) $lgnumber
    }
    # -- initialize internal use variables
    # -- FIXME: redundant with code in newLogGroup
    set cfg(ptag) "SYSTEM-A"
    set cfg(rtag) "SYSTEM-B"
    set cfg(curtag) "SYSTEM-A"
    set curtag "SYSTEM-A"
    set cfg(isprimary) 1
    set cfg(SYSTEM-A,HOST:) ""
    set cfg(SYSTEM-A,HOSTID:) ""
    set cfg(SYSTEM-A,PSTORE:) ""
    set cfg(SYSTEM-A,%Q%devs) ""
    set cfg(SYSTEM-B,HOST:) ""
    set cfg(SYSTEM-B,JOURNAL:) "$SYS(defjournal)"
    set cfg(SYSTEM-B,ON_JOURNAL_EXEC:) ""
    set cfg(SYSTEM-B,%Q%devs) ""
    set cfg(secport) $defaultsys(tcpport)
    set cfg(CHAINING:) off
    set cfg(%Q%devs) ""
    # -- set tunables to their defaults
    array set cfg [array get defaulttunable]
    # -- parse the configuration file
    set fd [open "/%OURCFGDIR%/$cfgfile.cfg" r]
    set buf [read $fd]
    close $fd
    set lineno 0
    set getthrottle 0
    set cfg(throttletext) ""
    foreach line [split $buf "\n"] {
	incr lineno
        if {$getthrottle} {
            set cfg(throttletext) "$cfg(throttletext)$line\n"
            if {[string match "ENDACTIONLIST*" $line ]} {
                set getthrottle 0
            } 
            continue;
        }
	# -- discard comments and empty lines
	if {[string index $line 0] == "\#"} {continue}
	if {[regexp "^\[ \t]*$" $line]} {continue}
       
	switch -glob -- [lindex $line 0] {

	    "NOTES:" { set cfg([lindex $line 0]) "[lrange $line 1 end]" }

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
            "PSTORE:" {set cfg(SYSTEM-A,PSTORE:) [lindex $line 1]}
	    "JOURNAL:" {set cfg(SYSTEM-B,JOURNAL:) [lindex $line 1]}
	    "PROFILE:" {
		
		if {$CFGfilecheck && (!$havesystems)} {
		    global RMDtimeout
		    # -- check if we have a devices list for this system
		    set sysaname $cfg(SYSTEM-A,HOST:)
		    set sysbname $cfg(SYSTEM-B,HOST:)
		    		 
		    if {$sysaname == ""} {
			lappend SYS(errors) "$cfgfile - Primary System definition missing"
		    }
		    if {$sysbname == ""} {
			lappend SYS(errors) "$cfgfile - Secondary System definition missing"
		    }

		    if {[info globals devlist-$sysaname] == "" && $sysaname != ""} {
			displayInfo "Retrieving device information from $sysaname" ON
			ftdmkdevlist $sysaname $SYS(primaryport) $RMDtimeout
			upvar #0 devlist-$sysaname sysadev
			displayInfo "" OFF
			if {$sysadev(status) != 0} {
                            displayError $sysadev(errmsg)
			    lappend SYS(errors) "$cfgfile - Primary System: $sysadev(errmsg)"
			}
		    }
		    if {[info globals devlist-$sysbname] == "" && $sysbname != ""} {
			displayInfo "fetching device information from $sysbname" ON
			ftdmkdevlist $sysbname $cfg(secport) $RMDtimeout
			upvar #0 devlist-$sysbname sysbdev
			displayInfo "" OFF
			if {$sysbdev(status) != 0} {

                            displayError $sysbdev(errmsg)
			    lappend SYS(errors) "$cfgfile - Secondary System: $sysbdev(errmsg)"
			}
		    }
		    set havesystems 1
		    upvar #0 devlist-$sysaname sysadev
		    upvar #0 devlist-$sysbname sysbdev
		}
		incr SYS(numprofile)
		global "profile$SYS(numprofile)"
		upvar #0 "profile$SYS(numprofile)" profile
		set profile(REMARK:) ""
		set profile(PRIMARY:) "SYSTEM-A"
		set profile(%CAPPRODUCTNAME_TOKEN%-DEVICE:) ""
		set profile(DATA-DISK:) ""
		
		set profile(SECONDARY:) "SYSTEM-B"
		set profile(MIRROR-DISK:) ""
	    }
	    "REMARK:" {set profile(REMARK:) "[lrange $line 1 end]"}
	    "PRIMARY:" {set profile(PRIMARY:) [lindex $line 1]}
	    "SECONDARY:" {set profile(SECONDARY:) [lindex $line 1]}
	    "SECONDARY-PORT:" {set cfg(secport) [lindex $line 1]}
            "CHAINING:" { set cfg(CHAINING:) [lindex $line 1]}
	    "%CAPPRODUCTNAME_TOKEN%-DEVICE:" {
                set %Q%dev [lindex [file split [lindex $line 1]] end]
                set profile(%CAPPRODUCTNAME_TOKEN%-DEVICE:) $%Q%dev
		if {[info exists cfg(%Q%devs)]} {
		    if {-1 == [lsearch $cfg(%Q%devs) $%Q%dev]} {
			lappend cfg(%Q%devs) $%Q%dev
			set cfg(%Q%devs) [lsort -command %Q%devsort $cfg(%Q%devs)]
		    } 
		} else {
		    lappend cfg(%Q%devs) $%Q%dev
		}
	    }
            "DATA-DISK:" { set profile(DATA-DISK:) [lindex $line 1]}
	    "MIRROR-DISK:" {set profile(MIRROR-DISK:) [lindex $line 1]}
	    "THROTTLE*" {
		set getthrottle 1
                set cfg(throttletext) "$cfg(throttletext)$line\n"
	    }
        }
    }
   
    # read tunable parameters from pstore if possible
    if {![catch "exec /%FTDBINDIR%/%Q%set -q -g $lgnumber" setbuf]} {
        foreach line [split $setbuf "\n"] {
            set element [lindex $line 0]
            if  {[info exists tunabletype($element)]} {
                if {$tunabletype($element) == "ONOFF"} {
                    set val [lindex $line 1]
                    if {($val == "on") || ($val=="ON") || ($val == "1")} {
                        set cfg($element) on
                    } elseif {($val == "off") || ($val=="OFF") || ($val == "0")} {
                        set cfg($element) off
                    } 
                } else {
                    set cfg($element) [lindex $line 1]
                }
            } else {
                set cfg($element) [lindex $line 1]
            }
        }
    } else {
        # attempt to read them from the settunables temp file
        if {![catch {open "/%OURCFGDIR%/settunables$lgnumber.tmp" r} tunefd]} {
            set tunebuffer [read $tunefd]
            foreach tuneline [split $tunebuffer "\n"] {
                if { [lindex $tuneline 0] == "#!/bin/sh" } {
                    continue
                }
                if { [lindex $tuneline 3] == $lgnumber } {
                    set tunestr [lindex $tuneline 4]
                    regexp "(.*)=(.*)" $tunestr xx element value
                    set cfg(${element}:) $value
                }
            }
        }
    }

    
    # ===== post-parsing validity checking =====
    if {![info exists sysaname]} {set sysaname ""}
    if {![info exists sysbname]} {set sysbname ""}
    upvar #0 devlist-$sysaname sysadev
    upvar #0 devlist-$sysbname sysbdev

    # -- %Q% Device Definitions
    foreach p [info globals profile*] {
	set q ""
	if {![catch {lindex [file split [set ${p}(%CAPPRODUCTNAME_TOKEN%-DEVICE:)]] end} q]} {
	    global $q
	    if {[info exists $q]} {
		upvar #0 $q %Q%
		lappend SYS(errors) "$cfgfile - \[$q\] device multiply defined - discarding earlier instance"
		ftddevdel $sysaname $%Q%(DATA-DISK:) $SYS(candidatelg) \
		    $q LOCATADEV
		ftddevdel $sysbname $%Q%(MIRROR-DISK:) $SYS(candidatelg) $q MIRRORDEV
		catch "unset $q"
	    }
	    array set $q [array get $p]
	    unset $p
	}
     
	# -- skip detailed error checking if told to on the command line
	if {!$CFGfilecheck} {continue}
	displayInfo "Verifying Configuration for Logical Group $SYS(candidatelg)" ON
        after 1000
	global $q
	upvar #0 $q %Q%
	if {[info exists %Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:)]} {
	    if {[string range $%Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:) 0 2] != "%Q%" } {
		lappend SYS(errors) "$cfgfile - Invalid %Q% Device path specification \[$%Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:)\]"
	    }
	}
	set %Q%dev [lindex [file split [lindex $%Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:) 0]] end]
	if {(![info exists %Q%(DATA-DISK:)]) || ($%Q%(DATA-DISK:) == "")} {
	    lappend SYS(errors) "$cfgfile - \[$%Q%dev\] missing Local Data Device definition"
	}
	
	if {(![info exists %Q%(MIRROR-DISK:)]) || ($%Q%(MIRROR-DISK:) == "")} {
	    lappend SYS(errors) "$cfgfile - \[$%Q%dev\] missing a Mirror Device definition"
	}
	# -- check for conflicts with itself
	
	
	# -- check for conflicts with other %Q% device definitions
	### if the device has been previously defined for this logical group,
	### this is an attempted replacement, first remove the prior stuff
	### from the interference detection array
        if {0 != [ftddevchk $sysaname $%Q%(DATA-DISK:) $SYS(candidatelg) $%Q%dev LOCDATADEV]} {
            lappend SYS(errors) "$cfgfile - \[$%Q%dev\] $sysadev(errmsg)"
        }
	
        if {0 != [ftddevchk $sysbname $%Q%(MIRROR-DISK:) $SYS(candidatelg) $%Q%dev MIRRORDEV]} {
            lappend SYS(errors) "$cfgfile - \[$%Q%dev\] $sysbdev(errmsg)"
        }
	# -- check for size issues
	if {[info exists sysadev($%Q%(DATA-DISK:)..size)]} {
	    set s1 [lindex $sysadev($%Q%(DATA-DISK:)..size) 3]
	} else {
	    set s1 -1
	}
	if {[info exists sysbdev($%Q%(MIRROR-DISK:)..size)]} {
	    set s2 [lindex $sysbdev($%Q%(MIRROR-DISK:)..size) 3]
	} else {
	    set s2 -1
	}
	if {$s1 != -1 && $s2 != -1} {
	    if {$s2 < $s1} {
		lappend SYS(errors) "$cfgfile - \[$%Q%dev\] Mirror Device \[$s2 sectors\] < Local Data Device \[$s1 sectors\]" 
	    }
	}
        
   
	# -- it passed the audition (or was forced) add to device definitions
	ftddevadd $sysaname $%Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:) $SYS(candidatelg) $%Q%dev FTDDEV 1
	ftddevadd $sysaname $%Q%(DATA-DISK:) $SYS(candidatelg) $%Q%dev LOCDATADEV 1
	
	ftddevadd $sysbname $%Q%(MIRROR-DISK:) $SYS(candidatelg) $%Q%dev MIRRORDEV 1
 
    }
    

    displayInfo "" OFF
}

#-------------------------------------------------------------------------
# 
#  saveorigcfg {cfgname} 
#
#  save off any of the user-configurable vars so
#  we will know later if we need to save changes
#  
#  Returns:  Nothing
#-------------------------------------------------------------------------
proc saveorigcfg {cfgname} {
    global $cfgname OLD_$cfgname SYS OLD_SYS
    upvar #0 $cfgname cfg

    # - loop through %Q% device settings and save them off
    foreach g [info globals "%Q%*"] {
        global $g OLD_$g        
        array set OLD_$g [array get $g]
    }
    
    # - save off logical group array
    array set OLD_$cfgname [array get $cfgname]

    set OLD_SYS(primaryport) $SYS(primaryport)
    set OLD_SYS(tcpbufsize) $SYS(tcpbufsize)
    set OLD_SYS(bab) $SYS(bab)
}

#-------------------------------------------------------------------------
# 
#  checkvarschange {cfgname}
#
#  see if any of the user-configurable vars have changed so
#  we know if we need to save changes
#  
#  Returns: 1 if something has changed, 0 otherwise
#-------------------------------------------------------------------------
proc checkvarschange {cfgname} {
    global $cfgname OLD_$cfgname OLD_SYS SYS THROTCHANGE netkbpson netmax
    upvar #0 $cfgname cfg
    upvar #0 OLD_$cfgname oldcfg

    set changed 0

    if {$netkbpson} {
        set cfg(NETMAXKBPS:) $netmax
    } else {
        set cfg(NETMAXKBPS:) -1
    }

    set throttext [.f1.nb.nbframe.throttles.throtF.border.frame.throtT get 0.0 end]
    # - remove trailing /n
    set cfg(throttletext) [string range $throttext 0 [expr [string length $throttext] - 2 ]]
    
    # - loop through %Q% device settings and see if they've changed
    foreach g [info globals "%Q%*"] {
        global $g OLD_$g
        upvar #0 $g %Q%
        upvar #0 OLD_$g old%Q%
      
        foreach element [array names %Q%] { 
            if { [info exists old%Q%($element) ]} {
                if { $%Q%($element) != $old%Q%($element) } {
                    set changed 1
                }
            } else {
                set changed 1
            }
        }
    }
    
    foreach element [array names cfg] { 
        if { $cfg($element) != $oldcfg($element) } {
            set changed 1
        }
    }
  
    if { ($OLD_SYS(primaryport) != $SYS(primaryport)) || \
            ($OLD_SYS(tcpbufsize) != $SYS(tcpbufsize)) } { 
        set changed 1
    }

    return $changed
}

#-------------------------------------------------------------------------
# 
#  %Q%devsort {a b}
#  
#  Returns: -1, 0, 1 for %Q% device name ordering
#-------------------------------------------------------------------------
proc %Q%devsort {a b} {
    set aa [string range $a 3 end]
    set bb [string range $b 3 end]
    if {$aa == $bb} {
	return 0
    } else {
	if {[%Q%isint $aa] && [%Q%isint $bb]} {
	    return [expr (($aa > $bb) ? 1 : -1)]
	} else {
	    if {"$aa" > "$bb"} {return 1}
	    return -1
	}
    }
}

#-------------------------------------------------------------------------
# 
#  writeconfig {lgnumber}
#
#  Writes a config file from the contents of a Tcl global array
#  
#  Returns: Nothing 
#-------------------------------------------------------------------------
proc writeconfig {lgnumber} {
    global SYS defaulttunable netmax netkbpson
    menu_off
    displayInfo "Saving Logical Group $lgnumber...." ON
    if {-1 == [lsearch -exact $SYS(loggrps) $lgnumber]} {
	lappend SYS(loggrps) $lgnumber
	set SYS(loggrps) [lsort $SYS(loggrps)]
	lappend SYS(cfgfiles) [format "p%03d" $lgnumber]
	set SYS(cfgfiles) [lsort $SYS(cfgfiles)]
    }
    set cfgfile [format "p%03d" $lgnumber]
    set configpath "/%OURCFGDIR%/${cfgfile}.cfg"
    if {[file exists "$configpath"]} {
        catch "exec /bin/mv $configpath ${configpath}.1"
    }
    global $cfgfile
    upvar #0 $cfgfile cfg
    if {[info globals $cfgfile] == ""} {
	menu_on
	return 2
    }
    # -- create the config file
    if {[catch "open $configpath w" fd]} {
	displayError "Could not open $configpath for writing"
	menu_on
	return
    }
    puts $fd "#==============================================================="
    puts $fd "#  Softek %PRODUCTNAME% Configuration File:  $configpath"
    puts $fd "#  Softek %PRODUCTNAME% Version %VERSION%"
    puts $fd "#\n#  Last Updated:  [clock format [clock seconds]]"
    puts $fd "#==============================================================="
    if {[string length $cfg(NOTES:)] > 255} {
	set cfg(NOTES:) [string range $cfg(NOTES:) 0 255]
    }
    if {0 < [string length $cfg(NOTES:)]} {
	puts $fd "#\nNOTES:  $cfg(NOTES:)\n#"
    }
   
    # -- print system definitions
    puts $fd "#\n# Primary System Definition:\n#"
    puts $fd [format "SYSTEM-TAG:          %-15s           %-15s" $cfg(ptag) PRIMARY]
    set p $cfg(ptag)
    set ptag $cfg(ptag)
  
    if {[llength $cfg($p,HOST:)] > 0} {
	puts $fd "  HOST:                $cfg($p,HOST:)"
    }
    puts $fd "  PSTORE:              $cfg($p,PSTORE:)"
    puts $fd "#\n# Secondary System Definition:\n#"
    puts $fd [format "SYSTEM-TAG:          %-15s           %-15s" $cfg(rtag) SECONDARY]
    set p $cfg(rtag)
    if {[llength $cfg($p,HOST:)] > 0} {
	puts $fd "  HOST:                $cfg($p,HOST:)"
    }
    if {[llength $cfg($p,JOURNAL:)] > 0} {
	puts $fd "  JOURNAL:             $cfg($p,JOURNAL:)"
    }
    if { $cfg(secport) != 575 } {
        puts $fd "  SECONDARY-PORT:      $cfg(secport)"
    }
    if { $cfg(CHAINING:) == "on" } {
        puts $fd "  CHAINING:            on"
    }

    puts $fd "#"
    set throttext [.f1.nb.nbframe.throttles.throtF.border.frame.throtT get 0.0 end]
    # - remove trailing /n
     set cfg(throttletext) [string range $throttext 0 [expr [string length $throttext] - 2 ]]
    if {$cfg(throttletext) != ""} {
        puts $fd $cfg(throttletext) 
    }
    
    # -- print product device profiles
    if {[info exists ${cfgfile}(%Q%devs)] && [llength $cfg(%Q%devs)] > 0} {
	puts $fd "#\n# Device Definitions:\n#"
	set p 0
	foreach q [lsort -command %Q%devsort $cfg(%Q%devs)] {	  
            upvar #0 $q %Q%
	    incr p
	    puts $fd "PROFILE:            $p"
	    if {255 < [string length $%Q%(REMARK:)]} {
		set %Q%(REMARK:) [string range $%Q%(REMARK:) 0 255]
	    }
	    if {0 < [string length $%Q%(REMARK:)]} {
		puts $fd "  REMARK:  $%Q%(REMARK:)"
	    }
	    puts $fd "  PRIMARY:          $%Q%(PRIMARY:)"

	    puts $fd "  %CAPPRODUCTNAME_TOKEN%-DEVICE:  /dev/%Q%/lg$lgnumber/rdsk/$%Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:)"
            puts $fd "  DATA-DISK:        $%Q%(DATA-DISK:)"
	   
	    puts $fd "  SECONDARY:        $%Q%(SECONDARY:)"
	    puts $fd "  MIRROR-DISK:      $%Q%(MIRROR-DISK:)"
	    puts $fd "#"
	}
    }
    puts $fd "#\n# -- End of %Q% Configuration File:  $configpath\n"
    flush $fd
    close $fd
    catch "exec /bin/rm ${configpath}.1"
    # catch "write%Q%conf"
    catch "exec /bin/rm /%OURCFGDIR%/settunables$lgnumber.tmp"
    if {[catch "open  /%OURCFGDIR%/settunables$lgnumber.tmp w" fd]} {
	displayError "Could not open /%OURCFGDIR%/settunables$lgnumber.tmp for writing"
	return
    } else {
        puts $fd "#!/bin/sh"
    }

    foreach element [array names defaulttunable] {
        # -- remove trailing : from tunable name for %Q%set
        set tunable [string range $element 0 \
                [expr [string length $element] - 2]]
        if {!$SYS(onprimary) || ![file exists "/%OURCFGDIR%/${cfgfile}.cur"]} {
            puts $fd "/%FTDBINDIR%/%Q%set -q -g $lgnumber ${tunable}=$cfg($element)"
        } elseif {[catch "exec %Q%set -q -g $lgnumber ${tunable}=$cfg($element) >& /dev/null" ret]}  { 
            puts $fd "/%FTDBINDIR%/%Q%set -q -g $lgnumber ${tunable}=$cfg($element)"
        }
    }
    catch "close $fd"
    catch "exec chmod 744 /%OURCFGDIR%/settunables$lgnumber.tmp"
    menu_on
    displayInfo "" OFF
    return 
}

#-------------------------------------------------------------------------
# 
#  getloggrps {}
#
#  Returns: The logical group numbers currently defined
#
#-------------------------------------------------------------------------
proc getloggrps {} {
    global SYS
    set result ""
    foreach fname [lsort [glob -nocomplain \
            "/%OURCFGDIR%/p\[0-9\]\[0-9\]\[0-9\].cfg"]] { 
	set cfgfile [file rootname [lindex [file split $fname] end]]
	scan [string range $cfgfile 1 3 ] "%d" loggrpno
	lappend result $loggrpno
    }
    return $result
}

#-------------------------------------------------------------------------
# 
#  makeMainMenu {}
#   
#  Creates the main menu bar and status bar for configtool
#
#  Returns: Nothing
#
#-------------------------------------------------------------------------
proc makeMainMenu {} {
    global SYS
    global env
    set SYS(cur%Q%) ""
    global BALOONhelpflag
    catch "destroy [winfo children .]"
    toplevel .a
    set t [image create photo -file $env(PROD_LIBRARY)/%CAPQ%logo47.gif]
    label .a.a -image $t
    pack .a.a
    wm overrideredirect .a 1
    wm withdraw .a
    wm geometry .a 47x47
    wm deiconify .a
    wm iconwindow . .a

   
    set xloc [expr [winfo screenwidth .]/2 - 320]
    set yloc [expr [winfo screenheight .]/2 - 240]
    wm geometry . +$xloc+$yloc
    if {[winfo exists .bottomF]} {
        destroy .bottomF
    }
    frame .bottomF -relief ridge -bd 3
    pack .bottomF -side bottom -expand 1 -fill x
    label .bottomF.lgL -fg black \
            -font -Adobe-Helvetica-Bold-R-Normal--*-100-*-*-*-*-*-*
    pack .bottomF.lgL -side left 
    set w [frame .f0 -bd 2 -relief groove]
    pack $w -expand y -fill x -side top -anchor nw
    menubutton $w.file -menu $w.file.m -text File -takefocus 0
    pack $w.file -side left 
    menubutton $w.system -menu $w.system.m -text System -takefocus 0
    pack $w.system -side left 
    menubutton $w.view -menu $w.view.m -text View -takefocus 0
    pack $w.view -side left
   
    set SYS(help) [tixBalloon $w.help -initwait 3000]
    if {!$BALOONhelpflag} {$SYS(help) configure -state none}
    
    menu $w.file.m
    menu $w.view.m
    menu $w.system.m
 
    $w.file.m add command -label "New Logical Group" \
            -command "SaveQuery newLogGroupSetNum"
    $w.file.m add command -label "Select Logical Group" \
            -command "SaveQuery selLogGroup"
    $w.file.m add command -label "Reset Logical Group" \
            -command "resetLogGroup"
    $w.file.m add command -label "Save Changes" \
            -command "saveLogGroup" 
    $w.file.m add separator  
    $w.file.m add command -label "Exit" \
            -command "exitConfigTool"
    $w.view.m add command -label "Systems" \
            -command ".f1.nb raise systems"
    $w.view.m add command -label "%CAPQ% Devices" \
            -command {%Q%raise%Q%devpage\
	    .f1  $SYS(candidatecfg)}
    $w.view.m add command -label "Throttles" \
            -command ".f1.nb raise throttles"
    $w.view.m add command -label "Tunable Parameters" \
            -command ".f1.nb raise tunables"
    $w.view.m add separator
    $w.view.m add command -label "Configuration Errors" \
            -command "reportconfigerrors"
    $w.view.m add command -label "About Softek %PRODUCTNAME%..." \
            -command "Introduction 1"
    $w.system.m add command -label "BAB..." \
            -command "init_bab"
    $w.system.m add command -label "TCP Settings..." \
            -command "makeTCPWindow"

    if {[llength $SYS(cfgfiles)] == 0} {
	newLogGroup 
    } else {        
	editLogGroup [lindex $SYS(loggrps) 0]
    }

    wm title . "Softek %PRODUCTNAME% Configuration Tool"
    wm protocol . WM_DELETE_WINDOW exitConfigTool
    update idletasks
    wm deiconify .
    return
}
#-------------------------------------------------------------------------
# 
#  menu_off {}
#   
#  Disables main menu bar (greys)
#
#  Returns: Nothing
#
#-------------------------------------------------------------------------
proc menu_off {} {
    .f0.file configure -state disabled
}

#-------------------------------------------------------------------------
# 
#  menu_off {}
#   
#  Enables main menu bar
#
#  Returns: Nothing
#
#-------------------------------------------------------------------------
proc menu_on {} {
    .f0.file configure -state normal
}

#-------------------------------------------------------------------------
# 
#  isntNumeric {line {min -1} {max -1}}
#  
#  Returns: 0 if $line is numeric, 1 otherwise
#
#-------------------------------------------------------------------------
proc isntNumeric {line {min -1} {max -1}} {
    if {$line == ""} {return 1}
    foreach d [split $line ""] {
	if {-1 == [string first $d 0123456789]} {
	    return 1
	}
    }
    if {$min != -1} {
	if {$line < $min} {return 1}
    }
    if {$max != -1} {
	if {$line > $max} {return 1}
    }
    return 0
}

#-------------------------------------------------------------------------
# 
#  init_bab_and_ps {} 
#  
#  Creates introduction sequence for initializing BAB and pstore from
#  within configtool. This routine is only called when %Q%init has not
#  been run successfully. 
#
#  - HP first displays the license agreement.  
#  - Displays welcome informational message
#  - Asks for BAB size
#  - Asks for Pstore size
#  - For HP attempts to rebuild kernel and prompts for reboot
#
#  Returns: Nothing
#
#--------------------------------------------------------------------------
proc init_bab_and_ps {} {
    global SYS done In_Intro errorCode PRE env
    global dlkm

#PB: Remove Copyright statement ++
#    if {($SYS(platform) == "HP-UX") || ($SYS(platform) == "AIX")} { 
#        toplevel .top	
#        wm title .top "Softek %PRODUCTNAME% License Agreement"
#        wm transient .top [winfo toplevel .]
#        set t .top
#        frame $t.topF
#        frame $t.botF -relief groove -bd 2
#        pack $t.topF -side top
#        pack $t.botF -side bottom -fill x -expand 1
#        
#        set f $t.topF
#        frame $f.licenseF
#        pack $f.licenseF  -padx 1c -pady 1c
#        label $f.labelL -text "Software License Agreement" 
#        pack $f.labelL -in $f.licenseF -side top 
#        text $f.licenseT -width 80 -height 14 -yscrollcommand [list $f.licenseSB set]
#        scrollbar $f.licenseSB -orient vertical -command [list $f.licenseT yview]
#        
#        pack $f.licenseT -in $f.licenseF -side left
#        pack $f.licenseSB -side left -expand 1 -fill y -in $f.licenseF 
#        
#        if {![catch {open "$env(PROD_LIBRARY)/copyright.txt" r} lic_fd]} {
#            if {[catch {read $lic_fd} lic_buf]} {
#                %Q%_dialog .errorT "Error"\
#                        "Couldn't read $env(PROD_LIBRARY)/copyright.txt"\
#                        4i warning  0 Exit
#                exit
#            }
#         } else {
#             %Q%_dialog .errorT "Error"\
#                     "Couldn't read $env(PROD_LIBRARY)/copyright.txt."\
#                     4i warning  0 Exit
#             exit
#         }
# 	
#         $f.licenseT insert 0.0 $lic_buf
#         $f.licenseT configure -state disabled 
#         frame $t.botF.buttonF 
#         pack $t.botF.buttonF -pady .5c
#         button $t.botF.acceptB -text Accept -command "set done 1"
#         button $t.botF.rejectB -text Reject -command "exit"
#         pack $t.botF.acceptB -in $t.botF.buttonF -side left -padx .25c
#         pack $t.botF.rejectB -in $t.botF.buttonF -side left -padx .25c
#         
#         center_window .top
#         tkwait variable done
#         
#         destroy .top
#     }
#     
#PB: Remove Copyright statement ++

    toplevel .top
    wm title .top "Softek %PRODUCTNAME% Configuration Tool"
    wm transient .top [winfo toplevel .]
    set t .top
    frame $t.topF
    frame $t.botF -relief groove -bd 2
    pack $t.topF -side top
    pack $t.botF -side bottom -fill x -expand 1

    set f $t.topF
    set In_Intro 0
    label $f.explainL -text "Welcome to the Softek %PRODUCTNAME% Configuration Tool.  As a first time configuration step you will be asked to specify the amount of RAM that should be reserved for use by the BAB.  After this initialization is successful, you will not be prompted for this information the next time you invoke %Q%configtool." -justify left -wraplength 5i  
    
    pack $f.explainL -padx 1c -pady 1c
    button $t.botF.okB -text Ok -command "set done 1"
    pack $t.botF.okB -anchor c -pady .5c 
    
    center_window .top
    tkwait variable done
    
    destroy .top
    
    makeMemWindow 1
    set SYS(bab) $PRE(bab)
    
    set rebootflag 0
    
    set In_Intro 0
    
    #
    # For HP we'll need to rebuild the kernel with our driver
    #
    if { $SYS(platform) == "HP-UX" && !$dlkm } {
	if [file exists /stand/vmunix] {
	    set backmsg  "  /stand/vmunix will be renamed to /stand/vmunix.pre%CAPQ% before the rebuild occurs so that you may restore it at a later time if necessary."
	} else {
	    set backmsg ""
	} 
        %Q%_dialog .warnT "Kernel Rebuild Necessary" "A kernel rebuild will be necessary after the pstore and BAB are initialized.$backmsg" 4i info 0 Ok
        displayInfo "Building new kernel... (takes 1-2 minutes)" on
    } elseif { $SYS(platform) == "AIX" } {
	#
	# Make sure that odmadd has happened
	#
	if {[catch "exec /usr/sbin/lsdev -P | grep %Q%" retstr]} {
	    set ret [%Q%_dialog .errorT "Error" "Cannot initialize BAB; %CAPQ% driver is not included in the Predefined Devices object class.  This may be a result of an incomplete or failed installation.  You should remove and re-install Softek %PRODUCTNAME% to resolve this problem." 4i warning 0 Exit]
	    exit
	}
	#
	# Make sure that def%Q% has happened
	#
	if {[catch "exec /usr/sbin/lsdev -C | grep %Q%0" retstr]} {
	    set ret [%Q%_dialog .errorT "Error" "Cannot initialize BAB; %CAPQ% driver is not included in the Customized Devices object class.  This may be a result of an incomplete or failed installation.  You should remove and re-install Softek %PRODUCTNAME% to resolve this problem." 4i warning 0 Exit]
	    exit
	} elseif {[lindex $retstr 1] == "Available"} { 
	    #
	    # Driver has already attached
	    #
	    set ret [%Q%_dialog .errorT "Error" "The %CAPQ% Driver has already been attached, you must first stop all logical groups, then unattach the driver using the following command:\n     /usr/lib/methods/ucfg%Q% -l %Q%0" 4i warning 0 Exit]
	    exit
	}
    }
    set failed 0
    if {[catch "exec /%FTDBINDIR%/%Q%init -b $SYS(bab) >& /dev/null" result]} {
        puts stderr $result
        displayInfo ""  off 
        set failed 1
        if {$failed} {
            set ret [%Q%_dialog .errorT "Error" "BAB initialization failed." 4i warning 0 Exit]
            exit
        }
    } else {
        displayInfo "" off
    }

    if { $SYS(platform) == "HP-UX"} {
        # DTurrin - Merged the 2 HP-UX "if" statements. Also modified the "if"
        #           statement for HP-UX 11.00 so that it takes all 11.xx OS
        #           versions into account. (August 16th, 2001)
        if { $SYS(osVersion) == "B.10.20" } {
            set ret [%Q%_dialog .workedT "Kernel Build Successful" "The kernel was rebuilt sucessfully with the %CAPQ% driver included.  You must reboot the system for the changes to take effect." 4i info 0 Ok ]
            exit
        } else {
	    # HPUX 11.00 or 11.11
            set failed 0
	    displayInfo "Loading Driver..." on
	    displayInfo "kmtune Driver..." on
	    if {[catch "exec /usr/sbin/kmtune -s ftd_num_chunk=$SYS(bab)" result ] } {
	        puts stderr $result
	        displayError\
	            "kmtune failed.  You will have to kmtune the %Q% module manually."
                set failed 1
	        exit
	    }
            if {$dlkm} {
	        displayInfo "update kernel..." on
	        if {[catch "exec /usr/sbin/kmadmin -L %Q% " result ] } { 
	            puts stderr $result
	            displayError\
	                "kmadmin failed.  You will have to reload the %Q% module manually."
                    set failed 1
	            exit
	        }
                displayInfo "The new %Q% module loaded"
            } else {
                displayInfo ""  off 
                set ret [%Q%_dialog .workedT "Initialization Successful" "BAB size successfully initialized.  A reboot will be required before configuration\ncan continue.  The following command will be used for the reboot:\n     $SYS(rebootcmd)\nIs it ok to reboot the system now?" 4i question 0 Yes "No, Exit" ]
                if {$ret == 1} {
                    displayInfo "You will need to reboot the system manually for these settings to take effect."
                    return
                } else {
                    cd /
                    if {[catch "exec $SYS(rebootcmd) &" ] } {
                        displayError "Reboot failed.  You will have to reboot the system manually."
                        exit
                    } 
                    exit
                }
            }  
            return
        }
    } elseif { $SYS(platform) == "AIX" } {
	#
	# Now attach driver
	# 
	displayInfo "Attaching Driver..." on
	set failed 0
	if [catch "exec /usr/lib/methods/cfg%Q% -l %Q%0 -2" out] {
            displayInfo ""  off 
            displayError "Driver load failed: $out" 
            set failed 1
        } else { 
	    displayInfo "" off 
	}
    } elseif { $SYS(platform) == "SunOS" } {
	#
	# Now add driver
	#
	displayInfo "Loading Driver..." on
	set failed 0
        if [catch "exec /usr/sbin/add_drv %Q%" out] {
            displayInfo ""  off 
            displayError "Driver load failed: $out" 
            set failed 1
        } else { 
	    displayInfo ""  off 
	}
    }
    if {$failed} {
	displayError "An error occurred during an attempt to load the driver.  You may try a manual configuration of the BAB using the method documented in the Softek %PRODUCTNAME% Administrator's Guide."
	exit
    } else {
	catch "exec /%FTDBINDIR%/%Q%info -a" %Q%infoout
	foreach line [split $%Q%infoout "\n"] {
	    if {[lindex $line 0] == "Requested"} {
		set reqamt [lindex $line 6]
	    } elseif {[lindex $line 0] == "Actual"} {
		set actualamt [lindex $line 6]
	    }
	}
	if {$reqamt != $actualamt} {
	    displayInfo "The driver was loaded successfully but was only able to obtain $actualamt MB of RAM for use by the BAB ($reqamt MB requested).  You should reboot the system following configuration to acquire the requested amount of memory."
	} else {
	    displayInfo "Driver loaded successfully."
	}
    }
}

#-------------------------------------------------------------------------
# 
#  init_bab {} 
#  
#  Creates a window which prompts for BAB size, then either reloads driver 
#  or builds kernel depending on OS.
#
#  Returns: Nothing
#
#-------------------------------------------------------------------------- 
proc init_bab {} {
    global SYS done In_Intro PRE
    global dlkm

    if {![makeMemWindow 0]} {
        return
    }
    
    # DTurrin - Removed un-necessary OS Version check on August 16th, 2001
    if { $SYS(platform) == "HP-UX" } {
	#
	# Warn about the driver reload
	#
	if {$dlkm} {
          set ret [%Q%_dialog .workedT "Warning" "For this change to take effect, the driver will be reloaded.   Before proceeding, shut down any processes that may be accessing %CAPQ% devices, unmount any file systems that reside on %CAPQ% devices, and stop all logical groups using %Q%stop.  All %CAPQ% daemons will be temporarily shut down while the reload is taking place." 4i warning 0 Ok Cancel]
	 if {$ret == 1} { return }
	}
    }

    if { $SYS(platform) == "AIX" } {
	#
	# Make sure that odmadd has happened
	#
	if {[catch "exec /usr/sbin/lsdev -P | grep %Q%" retstr]} {
	    set ret [%Q%_dialog .errorT "Error" "Cannot initialize BAB; %CAPQ% driver is not included in the Predefined Devices object class.  This may be a result of an incomplete or failed installation.  You should remove and re-install Softek %PRODUCTNAME% to resolve this problem." 4i warning 0 Exit]
	    exit
	}
	#
	# Make sure that def%Q% has happened
	#
	if {[catch "exec /usr/sbin/lsdev -C | grep %Q%0" cfgstr]} {
	    set ret [%Q%_dialog .errorT "Error" "Cannot initialize BAB; %CAPQ% driver is not included in the Customized Devices object class.  This may be a result of an incomplete or failed installation.  You should remove and re-install Softek %PRODUCTNAME% to resolve this problem." 4i warning 0 Exit]
	    exit
	}
    } elseif {$SYS(platform) == "HP-UX" && !$dlkm} {
          if [file exists /stand/vmunix] {
            set backmsg  "  /stand/vmunix will be renamed to /stand/vmunix.%CAPQ%bak before the rebuild occurs so that you may restore it at a later time if necessary."
            exec mv /stand/vmunix /stand/vmunix.DTCbak > /dev/null
          } else {
              set backmsg ""
          }
	
          %Q%_dialog .warnT "Kernel Rebuild Necessary" "A kernel rebuild will be necessary after the pstore and BAB are initialized.$backmsg" 4i info 0 Ok
          displayInfo "Building new kernel... (takes 1-2 minutes)" on
    }
    
    set In_Intro 0
    
    if [catch "exec /%FTDBINDIR%/%Q%init -b $PRE(bab) >& /dev/null" out] {
        if { $SYS(platform) == "HP-UX" } {
            displayInfo ""  off 
        }
        displayError "BAB Initialization failed.  You may execute the /%FTDBINDIR%/%Q%init -b $SYS(bab) command manually to configure the BAB."
        exit
    } 
    #
    # - set the bab size since init was successful
    #
    set SYS(bab) $PRE(bab)
  
    if { $SYS(platform) == "HP-UX"} {
      if { $SYS(osVersion) == "B.10.20" || !$dlkm } {
	displayInfo ""  off 
	set ret [%Q%_dialog .workedT "Initialization Successful" "BAB size successfully initialized.  A reboot will be required before configuration\ncan continue.  The following command will be used for the reboot:\n     $SYS(rebootcmd)\nIs it ok to reboot the system now?" 4i question 0 Yes "No, Exit" ]
        if {$ret == 1} {
            displayInfo "You will need to reboot the system manually for these settings to take effect."
            return
        } else {
	    cd /
            if {[catch "exec $SYS(rebootcmd) &" ] } {
                displayError "Reboot failed.  You will have to reboot the system manually."
                exit
            } 
            exit
        }
      }
    } 
    #
    # Reload driver for AIX or SUN
    #
    displayInfo "Reloading Driver..." on
    set failed 0
    catch "exec /%FTDBINDIR%/kill%Q%master >& /dev/null"
    if {$SYS(platform) == "AIX"} {
	if { [lindex $cfgstr 1] == "Available"} { 
	    #
	    # Driver has already attached, remove it
	    #
	    if {[catch "exec /usr/lib/methods/ucfg%Q% -l %Q%0" out]} {
		displayInfo "" off
		displayError "Driver unconfigure failed: $out"
		set failed 1
	    }
	}
	if {!$failed && [catch "exec /usr/lib/methods/cfg%Q% -l %Q%0 -2 >& /dev/null" out]} {
	    displayInfo ""  off 
	    displayError "Driver configure failed: $out" 
	    set failed 1
	} 
    } elseif {$SYS(platform) == "SunOS"} {
	if {[catch "exec /usr/sbin/rem_drv %Q%" out]} {
		displayInfo ""  off 
	    displayError "Driver remove failed. $out"
	    set failed 1
	} else {
	    if [catch "exec /usr/sbin/add_drv %Q% >& /dev/null" out] {
		displayInfo ""  off 
		displayError "Driver load failed: $out" 
		set failed 1
	    } 
	}
    }
    catch "exec /%FTDBINDIR%/in.%Q% >& /dev/null"
    displayInfo ""  off 
    if {$failed} {
        if {$SYS(platform) == "AIX"} {
            displayError "Errors occurred during an attempt to reload the driver.  This is most likely because some %CAPQ% devices are in use.  You will need to rectify the problem, then manually re-attach the driver by issueing /usr/lib/methods/ucfg%Q% -l %Q%0 then /usr/lib/methods/cfg%Q% -l %Q0 -2"
        } elseif {$SYS(platform) == "SunOS"} {
            displayError "Errors occurred during an attempt to reload the driver.  This is most likely because some %CAPQ% devices are in use.  You will need to rectify the problem, then manually reload the driver using the rem_drv and add_drv commands"
        }
    } else {
	catch "exec /%FTDBINDIR%/%Q%info -a" %Q%infoout
	foreach line [split $%Q%infoout "\n"] {
                if {[lindex $line 0] == "Requested"} {
                    set reqamt [lindex $line 6]
                } elseif {[lindex $line 0] == "Actual"} {
                    set actualamt [lindex $line 6]
                }
	}
        if { [info exists reqamt] } {
            if {$reqamt != $actualamt} {
                displayInfo "The driver was reloaded successfully but was only able to obtain $actualamt MB of RAM for use by the BAB ($reqamt MB requested). You should reboot the system following configuration to aquire the requested amount of memory."
            } else {
                displayInfo "Driver reloaded succesfully."
            }
        } 
    }
}     


#-------------------------------------------------------------------------
# 
#  Introduction {{aboutFlag 0}}
#  
#  Creates %PRODUCTNAME% Data Logo window with Version information.
#
#  Returns: Nothing
#
#-------------------------------------------------------------------------- 
proc Introduction {{aboutFlag 0}} {
    global SYS env done In_Intro introdone

    set introdone 0

    if {!$aboutFlag} {
        catch "exec $SYS(whoami)" user
        if {$user != "root" } {
            %Q%_dialog .errorD Error \
		"You must be superuser to run %Q%configtool." 4i \
		warning 0 Ok
            exit 1;
	}
    }
    
    set In_Intro 1
    
    toplevel .introT
    
    if {!$aboutFlag} {
	wm overrideredirect .introT 1
	set bdwidth 6
    } else {
	wm title .introT "About"
	set bdwidth 0
    }

    frame .introT.borderF -relief ridge -bd $bdwidth -bg white 
    pack .introT.borderF -expand yes -fill both 

    set w .introT.borderF

    catch {image create photo -file $env(PROD_LIBRARY)/%CAPQ%logo202.gif} t
   
    label $w.%Q%image -image $t -bg white 
    pack $w.%Q%image -side top -pady .5c -padx .5c

    label $w.title -text "Softek %PRODUCTNAME%, Open Systems Edition" -fg black -bg white
    pack $w.title -side top
    
    message $w.vermsg -text "Configuration Tool - Version %VERSION%" -aspect 10000 -justify center -font -Adobe-Helvetica-Bold-R-Normal--*-100-*-*-*-*-*-* -bg white
    pack $w.vermsg -side top

    set msg "Copyright (c) 2001 %COMPANYNAME%. All Rights Reserved."
    message $w.msg2 -text $msg                  -aspect 10000 -justify center -font -Adobe-Helvetica-Bold-R-Normal--*-100-*-*-*-*-*-* -bg white
    pack $w.msg2 -side top

    if {$aboutFlag} {
	button .introT.okB -text "Cancel" -command {set done 1}
	pack .introT.okB -pady .25c 
    }
    

    center_window .introT
    update
    if {!$aboutFlag} {
        after 5000 { set introdone 1}
	    readconfigs
        if {!$introdone} {
            tkwait variable introdone
        }

        destroy .introT

        if {$SYS(onprimary) } {
            if {$SYS(needinit) } {
                init_bab_and_ps
            }
        }
    } else {
	tkwait variable done
        destroy .introT
    }
    set In_Intro 0
}

#-------------------------------------------------------------------------
# 
#  doubleBAB {val}
#  
#  Doubles a value then rounds to the nearest power of 2, without exceeding 
#  the maximum or minimum allowable BAB sizes.
#
#  Returns: result of above calc
#
#-------------------------------------------------------------------------- 
proc doubleBAB {val} {
    global SYS

    if {$val < $SYS(minbab)} {
        set val $SYS(minbab)
    } elseif {$val >= $SYS(maxbab)} {
        return $SYS(maxbab)
    }
        
    #
    # first double the value
    #
    set x [expr $val * 2]

    #
    # now strip off all but the most significant bit
    #
    for {set i 1} {$i <= 1536} { set i [ expr $i * 2 ] } {
        if { [expr $i & $x] } {
            set highval $i
        }
    }
    
    return $highval
}

#-------------------------------------------------------------------------
# 
#  halveBAB {val}
#  
#  Divides a value by 2 then rounds to the nearest power of 2, without 
#  exceeding the maximum or minimum allowable BAB sizes.
#
#  Returns: result of above calc
#
#--------------------------------------------------------------------------   
proc halveBAB {val} {

    global SYS

    if {$val <= $SYS(minbab)} {
        return $SYS(minbab)
    } elseif {$val > $SYS(maxbab)} {
        set val $SYS(maxbab)
    }
        
    #
    # first halve the value
    #
    set x [expr $val / 2]

    #
    # now strip off all but the most significant bit
    #
    for {set i 1} {$i <= 1536} { set i [ expr $i * 2 ] } {
        if {[expr $i & $x] } {
            set highval $i
        }
    }
    return $highval
}

#-------------------------------------------------------------------------
# 
#  makeMemWindow {{initflag 0}}
#  
#  Creates window for specifying BAB size
#
#  Returns: 1 for OK, 0 for cancel
#
#--------------------------------------------------------------------------  
proc makeMemWindow {{initflag 0}} {
    
    global done SYS PRE

    set PRE(bab) $SYS(bab)
 
    if {[winfo exists .memT ]} {
        deiconify .memT
        return 0
    }
    toplevel .memT
    wm title .memT "Set BAB Size"
    wm transient .memT [winfo toplevel .]
    set w .memT
    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab $w"
   
    frame $w.topF
    pack $w.topF
    frame $w.botF -relief groove -bd 2
    pack $w.botF -expand 1 -fill x 

    frame $w.topF.padF
    pack $w.topF.padF -padx .5c -pady .5c

    set f $w.topF.padF
    
   # label $f.cacheL -text "BAB Size:"
    frame $f.cacheF 
    tixControl $f.cacheE -label "BAB Size: " -integer true \
	    -allowempty false -autorepeat true -variable PRE(bab) -min 32 \
	    -max 1536 -incrcmd doubleBAB -decrcmd halveBAB \
	    -options {
	entry.width 5
	label.width 10
	label.anchor c
    }
    
   # entry $f.cacheE -width 4 -bg white  -textvariable PRE(bab)
    label $f.mbL -text "MB"

    pack $f.cacheE -side left -in $f.cacheF
    pack $f.mbL -side right -in $f.cacheF

    grid $f.cacheF
  #  grid $f.cacheL -sticky w
    grid $f.cacheF -sticky w 

    frame $w.botF.padF
    pack $w.botF.padF -padx .5c -pady .5c

    set f $w.botF.padF
    
    button $f.okB -text "Ok" -command [list set done 1]
    pack $f.okB -side left -padx .25c
    bind $f.okB <Enter> [list focus $f.okB]

    button $f.cancelB -text "Cancel" -command "set done -1"
    pack $f.cancelB -side left -padx .25c

    center_window .memT
    while {1} {
        tkwait variable done
        if { $done == -1 } {
            destroy .memT
            if {$initflag} {
                exit
            } else {
                return 0
            }
        } else {
            destroy .memT
            return 1
        }
    }
}

#-------------------------------------------------------------------------
# 
#  valdevpath {devpath}
#  
#  Lops off the avail/inuse and size from a pstore list selection
#
#  Returns: resulting string
#
#--------------------------------------------------------------------------  
proc valdevpath {devpath} {
    global SYS
    set devpath [lindex $devpath 0]
    return $devpath
}

#-------------------------------------------------------------------------
# 
#  makePSWindow {{initflag 0}}
#  
#  Creates window for specifying pstore location
#
#  Returns: 1 for OK, 0 for cancel
#
#--------------------------------------------------------------------------  
proc makePSWindow {{initflag 0}} {
    
    global done RMDtimeout SYS PRE

    set PRE(ps) $SYS(ps)

    if {[catch "exec $SYS(hostname)" hostname]} {
        set hostname ""
    }
   
    displayInfo "Retrieving device information..." ON

    ftdmkdevlist $hostname $SYS(primaryport) $RMDtimeout
    after 2000
    upvar #0 devlist-$hostname sysadev
    displayInfo "" OFF
    if {$sysadev(status) != 0} {
        displayError $sysadev(errmsg)
    }
    set cfg(SYSTEM-A,HOSTID:) ""
    if {[winfo exists .ps ]} {
        deiconify .ps
        return 0
    }
    toplevel .ps
    wm title .ps "Persistent Storage"
    wm transient .ps [winfo toplevel .]
    set w .ps
    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab $w"

    frame $w.topF
    pack $w.topF
    frame $w.botF -relief groove -bd 2
    pack $w.botF -expand 1 -fill x 

    frame $w.topF.padF
    pack $w.topF.padF -padx .5c -pady .5c

    set f $w.topF.padF
  
    tixComboBox $f.recstor -label "Persistent Store Device:" \
	    -dropdown true \
	    -listcmd "%Q%filldevcombobox $f.recstor 1 $hostname dsk 1" \
	    -editable true -variable PRE(ps) \
            -validatecmd "valdevpath" \
	    -options {
	entry.width 30
	
	label.anchor w
	listbox.height 8
	listbox.width 30
	listbox.anchor e
    }
    pack $f.recstor
    frame $w.botF.padF
    pack $w.botF.padF -padx .5c -pady .5c

    set f $w.botF.padF
    
    button $f.okB -text "Ok" -command {set done 1}
    pack $f.okB -side left -padx .25c

    if {$initflag} {
        set canceltext "Exit"
    } else {
        set canceltext "Cancel"
    }

    button $f.cancelB -text $canceltext -command "set done -1"
    pack $f.cancelB -side left -padx .25c

    center_window .ps
 
    set notdone 1

    while {$notdone} {        
        tkwait variable done
        if { $done == -1} {
            destroy .ps
            if {$initflag} {
                exit
            } else {
                return 0
            }
        } else {
            if {[%Q%_dialog .verifyT "Warning" \
                    "You have selected $PRE(ps) for use as a pstore device.  Any data that currently exists on this device will be destroyed when the pstore is initialized." 4i\
                    warning 0 Continue "Cancel"] == 1 } {
            } else {
                destroy .ps
                return 1
            }
        }
    }
}

proc writetcpinfo {} {
    global SYS OLD_SYS

    if {![ catch {set servfd [open $SYS(services) r]}]} {
        # - we opened the services file successfully, read it in and make
        #   changes to our entry
        set lines [split [read $servfd] \n]
        if {[set i [lsearch -regexp $lines "in\.%Q%"]]!=-1} {
            set oldstr [lindex $lines $i]
            regsub {[0-9][0-9]*} $oldstr $SYS(primaryport) newstr
            set lines [lreplace $lines $i $i $newstr]
        }
        close $servfd
        
        if {![ catch {
            # - open up a new file and write the new contents
            set outfile [open $SYS(services).%Q%new w]
            set i 0
            foreach line $lines {
                incr i
                if {$i != [llength $lines]} {
                    puts $outfile $line
                } else {
                    puts -nonewline $outfile $line
                }
                
            }
            close $outfile
        }]} {
            # - if we did that successfully, copy it over the old /etc/services
            # 
            if {![catch {exec cp $SYS(services) $SYS(services).pre%Q%}]} {
                catch {exec mv $SYS(services).%Q%new $SYS(services)}
            }
        }
    }
    if {![ catch {set conffd [open $SYS(conffile) r]}]} {
        set lines [split [read $conffd] \n]
        if {[set i [lsearch -regexp $lines "tcp_window_size"]]!=-1} {
            set oldstr [lindex $lines $i]
            regsub {[0-9][0-9]*} $oldstr $SYS(tcpbufsize) newstr
            set lines [lreplace $lines $i $i $newstr]
        }
        close $conffd
        set outfile [open "$SYS(conffile).tmp" w]
        
        set i 0
        foreach line $lines {
            incr i
            if {$i != [llength $lines]} {
                puts $outfile $line
            } else {
                puts -nonewline $outfile $line
            }
        }
        close $outfile
        catch {exec mv $SYS(conffile).tmp $SYS(conffile)}
    }
}

proc makeTCPWindow {} {

    global SYS PRE
  
    if {[winfo exists .tcpT ]} {
        deiconify .tcpT
        return
    }
    toplevel .tcpT
    wm title .tcpT "TCP Settings"
    wm transient .tcpT [winfo toplevel .]
    set w .tcpT
    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab $w"
    frame $w.topF
    pack $w.topF
    frame $w.botF -relief groove -bd 2
    pack $w.botF -expand 1 -fill x 

    frame $w.topF.padF
    pack $w.topF.padF -padx .5c -pady .5c

    set f $w.topF.padF

    set PRE(primaryport) $SYS(primaryport)
    set PRE(tcpbufsize) $SYS(tcpbufsize)

    entry $f.pportE -width 3 -textvariable  PRE(primaryport)

    entry $f.bufE -width 6 -textvariable PRE(tcpbufsize)
    label $f.pportL -text "Listen Port:"
    label $f.bufL -text "Socket Buffer Size:"
  
    grid $f.pportL $f.pportE -padx .25c -sticky w
    grid $f.bufL $f.bufE -padx .25c -sticky w

    frame $w.botF.padF
    pack $w.botF.padF -padx .5c -pady .5c

    set f $w.botF.padF
    
    button $f.okB -text "Ok" -command {set SYS(tcpbufsize) $PRE(tcpbufsize); set SYS(primaryport) $PRE(primaryport); destroy .tcpT}
    pack $f.okB -side left -padx .25c
    
    button $f.cancelB -text "Cancel" -command "destroy .tcpT"
    pack $f.cancelB -side left -padx .25c

    center_window .tcpT

}
#-----------------------------------------------------------------------------
proc reportconfigerrors {} {
    global SYS env waitvar
    toplevel .configerrT
    
    frame .configerrT.f1
    pack .configerrT.f1 -padx .5c

    set w .configerrT.f1
   
    wm title .configerrT "Softek %PRODUCTNAME% Configuration Errors"  
    frame $w.fb -relief groove -borderwidth -2
    pack $w.fb -side bottom
    button $w.fb.dismiss -text "Cancel" -command {set waitvar 1 }
    pack $w.fb.dismiss
    label $w.title -text "%CAPQ% Configuration File Errors:"
    pack $w.title -side top
    tixScrolledListBox $w.sl -scrollbar auto \
	-scrollbars auto -options {
	    listbox.width 60
	    listbox.height 15
	}
    pack $w.sl -side top -anchor nw -expand yes -fill both
    set lb [$w.sl subwidget listbox]
    $lb configure -fg red
    $lb delete 0 end
    foreach line $SYS(errors) {
	$lb insert end $line
    }
    center_window .configerrT
    tkwait variable waitvar
    if [winfo exists .configerrT] {
        destroy .configerrT
     }
}    
    
#-----------------------------------------------------------------------------
proc newLogGroup {{lgnum ""}} {
    global SYS defaulttunable defaultsys
    menu_off
  
    set w .f1
    set lgrps [getloggrps]
    set lastlg [lindex $lgrps end]
    if {$lastlg == ""} {set lastlg -1}
    set SYS(candidatelg) [expr 1 + $lastlg]
    if {$lgnum != ""} {
	set SYS(candidatelg) $lgnum
    }
    set SYS(cur%Q%) ""
    set SYS(candidatecfg) [format "p%03d" $SYS(candidatelg)]
    global $SYS(candidatecfg)
    catch "unset $SYS(candidatecfg)"
    upvar #0 $SYS(candidatecfg) cfg
    # -- initialize internal use variables
    # -- FIXME: This code is redundant with code in readconfig
    set cfg(NOTES:) ""
    set cfg(%Q%devs) ""
    set cfg(ptag) "SYSTEM-A"
    set cfg(rtag) "SYSTEM-B"
    set cfg(curtag) "SYSTEM-A"
    set curtag "SYSTEM-A"
    set cfg(isprimary) 1
    set hostname ""
    catch "exec $SYS(hostname)" hostname
    set cfg(SYSTEM-A,HOST:) $hostname
    set cfg(SYSTEM-A,HOSTID:) ""
    set cfg(SYSTEM-A,PSTORE:) ""
    set cfg(SYSTEM-A,%Q%devs) ""
    set cfg(SYSTEM-B,HOST:) ""
    set cfg(SYSTEM-B,HOSTID:) ""
    set cfg(SYSTEM-B,JOURNAL:) "$SYS(defjournal)"
    set cfg(SYSTEM-B,%Q%devs) ""
    set cfg(secport) $defaultsys(tcpport)
    set cfg(CHAINING:) off
    set xxxcur%Q% ""
    # -- set tunables to their defaults
    array set cfg [array get defaulttunable]

    # -- remove all global device variables
    foreach g [info globals "%Q%*"] {
        global $g
        unset $g
    }
    wm title . "%CAPQ% Configuration Tool"
    update
    topNotebook $SYS(candidatecfg)
    
    update%Q%devs [$w.nb subwidget %Q%_Devices] $SYS(candidatecfg)
    updatedevicefields $w $SYS(candidatecfg) none
    menu_on
    .bottomF.lgL configure -text " Currently editing logical group $SYS(candidatelg)" -fg black
    .f1.nb raise systems
    # - save off cfg variables so we know whether we need to save changes
    #   later
    saveorigcfg $SYS(candidatecfg)
}

#-----------------------------------------------------------------------------
proc newLogGroupSetNum {} {
    global SYS defaulttunable xxxloggrp
    
    toplevel .newLogT
    wm transient .newLogT [winfo toplevel .]

    wm title .newLogT "New %CAPQ% Logical Group"
    set w .newLogT
    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab $w"
    frame .newLogT.padF
    pack .newLogT.padF  -pady .5c -padx .5c
    set w2 .newLogT
    set w .newLogT.padF
    
    set lastlg [lindex [getloggrps] end]
    if {$lastlg == ""} {set lastlg -1}
    incr lastlg
    
    tixControl $w.lgnum -label "\nLogical Group Number: " -integer true \
	    -allowempty false -autorepeat true -variable xxxloggrp -min 0 \
	    -max 999 -value $lastlg \
	    -options {
	entry.width 10
	label.width 25
	label.anchor c
    }
    pack $w.lgnum
    $SYS(help) bind $w.lgnum -balloonmsg \
	    "enter new Logical Group's number to be defined" 
    frame $w2.f -relief groove -bd 2 
    pack $w2.f -side top  -expand 1 -fill x
    frame $w2.butF
    pack $w2.butF -in $w2.f
    
    button $w2.butF.but1 -text "Ok" -command "destroy .newLogT; setLogGroupNum" 
    pack $w2.butF.but1 -side left -padx .25c
    bind $w2.butF.but1 <Enter> [list focus $w2.butF.but1]
 
    button $w2.butF.but2 -text "Cancel" -command "destroy .newLogT"
    pack $w2.butF.but2 -side left -pady .25c

    center_window .newLogT
    
}

#-----------------------------------------------------------------------------
proc setLogGroupNum {} {
    global xxxloggrp
    if {[info exists xxxloggrp]} {
	set lgnum $xxxloggrp
	catch "unset xxxloggrp"
    } else {
	set lgnum 0
    }
    if {-1 != [lsearch [getloggrps] $lgnum]} {
	displayError "Logical Group $lgnum exists, select another"
    } else {
	newLogGroup $lgnum
    }
}

#-----------------------------------------------------------------------------
proc selLogGroup {} {
    global SYS  xxxloggrp
  
    #
    # make sure some groups are defined
    #
    set loggrps [getloggrps]
    if {[llength $loggrps] == 0} {
	displayError "No Logical Groups are currently defined"
	return
    }

    toplevel .sel 
    wm transient .sel [winfo toplevel .]
    wm title .sel "Select Group"
    wm geometry .sel \
	    +[expr 100 + [winfo rootx . ]]+[expr 100 \
	    +[ winfo rooty .]]
    set w .sel
    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab $w"

    focus .sel
   
    #
    # make group selection dialog
    #
    frame .sel.topF
    pack .sel.topF -pady .5c -padx .5c  -fill y 
    
    label .sel.msgL -text "Select a Logical Group:"
    pack .sel.msgL -in .sel.topF 

    catch "unset xxxloggrp"

    set w .sel.topF
   
    set height 7
    if {[llength $loggrps] < 7} {
	set height [llength $loggrps]
	if {$height == 0} {set height 1}
    }
    if {$height < 7} {
	set width 3
    } else {
	set width 6
    }

    tixScrolledListBox $w.sl -scrollbar auto \
        -scrollbars auto -options "
            listbox.width $width
            listbox.height $height
        "
    
    $w.sl subwidget listbox delete 0 end
    foreach i [getloggrps] {
        $w.sl subwidget listbox insert end $i
    }
    $w.sl subwidget listbox selection set 0
    pack $w.sl -side top -fill x

    frame .sel.bottomF -relief groove -bd 2
    pack .sel.bottomF -expand 1 -fill both

    frame .sel.buttonF
    pack .sel.buttonF -in .sel.bottomF -expand 1 -fill y
    button .sel.okB -text "Ok" -command {
	set w .sel.topF
	set sl [$w.sl subwidget listbox]
	set selection [lindex [$sl get [$sl curselection]] 0]
	catch "grab release .sel"
	destroy .sel
	editLogGroup $selection
    }

    button .sel.cancelB -text "Cancel" -command {
	catch  "grab release .sel"
	destroy .sel
    }
    
    
    pack .sel.okB .sel.cancelB -in .sel.buttonF -side left\
	    -padx .25c -pady .25c
    update idletasks

}

#-----------------------------------------------------------------------------
proc editLogGroup {{lgnum ""}} {
    global SYS
    menu_off
  
    if {[llength $SYS(cfgfiles)] == 0} {return}

    if {$lgnum == ""} {
	set SYS(candidatelg) 0
    } else {
	set SYS(candidatelg) $lgnum
    }
    readconfig $lgnum
    .bottomF.lgL configure -text " Currently editing logical group $SYS(candidatelg)" -fg black
    set w .f1
    set SYS(candidatecfg) [format "p%03d" $SYS(candidatelg)]
    wm title . "Softek %PRODUCTNAME% Configuration Tool -- Logical Group $SYS(candidatelg)" 
    topNotebook $SYS(candidatecfg)
    set SYS(cur%Q%) ""
    update%Q%devs [$w.nb subwidget %Q%_Devices] $SYS(candidatecfg)
    # - save off cfg variables so we know whether we need to save changes
    #   later
    saveorigcfg $SYS(candidatecfg)
    menu_on
}

#-----------------------------------------------------------------------------
proc saveLogGroup {} {
    global SYS
    menu_off
  
    set w .f1
    wm title . "Softek %PRODUCTNAME% Configuration Tool -- Saving Changes"

    if {[DoSave] == 1 } { 
        # Save successful, so we need to update our 'original' variables so
        # we can detect changes on an exit
        saveorigcfg $SYS(candidatecfg)
    }
    menu_off
    update
    after 1000 
    wm title . "Softek %PRODUCTNAME% Configuration Tool -- Logical Group $SYS(candidatelg)"
 
   # update%Q%devs [$w.nb subwidget %Q%_Devices] $SYS(candidatecfg)
   
    update
    menu_on
}

#-----------------------------------------------------------------------------
proc resetLogGroup {} {
    global SYS
    menu_off
    set loggrps [getloggrps]
    if {[llength $loggrps] == 0} {
	displayError "No Logical Groups currently defined"
	menu_on
	return
    }   
    if {$SYS(candidatelg) == ""} {
	displayError "You have not selected a Logical Group to reset"
	menu_on
	return
    }

   
    set w .f1
    
    if {$SYS(candidatelg) < 0} {
	displayError "You have not selected a Logical Group to reset"
	menu_on
	return
    }
    displayInfo "Resetting Logical Group $SYS(candidatelg) from file"
    if {-1 != [lsearch [getloggrps] $SYS(candidatelg)]} {
	readconfig $SYS(candidatelg)
        # - save off cfg variables so we know whether we need to save changes
        #   later
        saveorigcfg $SYS(candidatecfg)
    } else {
	newLogGroup
    }
    wm title . "Softek %PRODUCTNAME% Configuration Tool"
    topNotebook $SYS(candidatecfg)
     
    update
    menu_on
}

#-----------------------------------------------------------------------------
proc exitConfigTool {} {
    global SYS 
   
    if {$SYS(candidatecfg) != "" || $SYS(candidatelg) != ""} {
	finalSaveQuery 
    } else {
	exit
    }
}

#-----------------------------------------------------------------------------
proc finalSaveQuery {} {
    global SYS
    update idletasks
    
    set cfgfile [format "p%03d" $SYS(candidatelg) ]
    if {! [checkvarschange $cfgfile] } {
        exit
    }

    set w .f1
    toplevel $w.save
    wm title $w.save  Warning
    wm geometry $w.save +[expr 100 + [winfo rootx . ]]+[expr 100 +[ winfo rooty . ]]
    frame $w.save.topF
    pack $w.save.topF 
   
    label $w.save.msgL -text "Do you want to save your changes before exiting?" -justify left
    pack $w.save.msgL -in $w.save.topF -side right -padx .5c -pady 1c
    label $w.save.bitmapL -bitmap warning 
    pack $w.save.bitmapL -in $w.save.topF -side left -padx .5c 

   # message $w.msg -aspect 10000 -justify center -text \
	   #"Logical Group $SYS(candidatelg) is actively being edited"
    #pack $w.msg -side top 

    frame $w.save.botF -relief groove -bd 2
    pack $w.save.botF -expand 1 -fill both

    frame $w.save.buttonF 
    pack $w.save.buttonF -in $w.save.botF -expand 1 -fill y
    
    button $w.save.saveB -text "Yes" -command "SaveYes $w.save"
    button $w.save.exitB -text "No" -command "exit"
    button $w.save.cancelB -text "Cancel" -command "destroy $w.save"
    pack $w.save.saveB $w.save.exitB $w.save.cancelB -in $w.save.buttonF -side left -padx .25c -pady .5c
    update
 
}
proc SaveYes {w} {
    catch {destroy $w}
    if {[DoSave] == 1} {exit}
}
#-----------------------------------------------------------------------------
proc SaveQuery {args} {
    global SYS 
    set cfgfile [format "p%03d" $SYS(candidatelg)]

    if {! [checkvarschange $cfgfile] } {
        eval $args
        return
    }
    if {$SYS(candidatelg) != ""} {
	
	set w .f1
	toplevel $w.save
        wm transient $w.save [winfo toplevel .]  
        set oldFocus [focus]
        set oldGrab [grab current $w.save]
        if {$oldGrab != ""} {
            set grabStatus [grab status $oldGrab]
        }
        catch "grab $w.save"
	wm title $w.save  Warning 
	wm geometry $w.save +[expr 100 + [winfo rootx . ]]+[expr 100 +[ winfo rooty . ]]
	frame $w.save.topF
	pack $w.save.topF 
	
	label $w.save.msgL -text "You have not saved your changes\nto this logical group.\n\nWould you like to save them now?" -justify left
	pack $w.save.msgL -in $w.save.topF -side right -padx 1c -pady 1c
	label $w.save.bitmapL -bitmap warning 
	pack $w.save.bitmapL -in $w.save.topF -side left -padx .5c 
	
	# message $w.msg -aspect 10000 -justify center -text \
		#"Logical Group $SYS(candidatelg) is actively being edited"
	#pack $w.msg -side top 
	
	frame $w.save.botF -relief groove -bd 2
	pack $w.save.botF -expand 1 -fill both
	
	frame $w.save.buttonF 
	pack $w.save.buttonF -in $w.save.botF -expand 1 -fill y
	
	button $w.save.saveB -text "Yes" -command "destroy $w.save; writeconfig $SYS(candidatelg); eval $args"
	button $w.save.exitB -text "No" -command "destroy $w.save; eval $args"
	button $w.save.cancelB -text "Cancel" -command "destroy $w.save"
	pack $w.save.saveB $w.save.exitB $w.save.cancelB -in $w.save.buttonF -side left -padx .25c -pady .5c
	update
	
    } else {
	eval $args
    }
}

proc DoSave {} {
    global SYS
    upvar #0 $SYS(candidatecfg) cfg

    if {$SYS(onprimary)} {
        writetcpinfo
    }
    if {$cfg(SYSTEM-B,JOURNAL:) != ""} {
        writeconfig $SYS(candidatelg)
        return 1
    } else {
        displayError "You must specify a Secondary Journal area."
        return 0
    }
}
#-----------------------------------------------------------------------------
proc %Q%raise%Q%devpage {w cfgname} {
    global RMDtimeout SYS xxxcur%Q%
    upvar #0 $cfgname cfg 
    # -- check if we have a devices list for this system
   
    set sysaname $cfg(SYSTEM-A,HOST:)
    set sysbname $cfg(SYSTEM-B,HOST:)
   
    if {$sysaname == ""} {
	displayError "You must define a Primary System first."
	$w.nb raise systems
	return
	
    }
    if {$sysbname == ""} {
	displayError "You must define a Secondary System first."
	$w.nb raise systems
	return
    }
    if {[info globals devlist-$sysaname] == ""} {
	displayInfo "Retrieving device information from $sysaname" ON
       	ftdmkdevlist $sysaname $SYS(primaryport) $RMDtimeout
       	after 2000
        upvar #0 devlist-$sysaname sysadev
        displayInfo "" OFF
        if {$sysadev(status) != 0} {
            displayError $sysadev(errmsg)
        }
    }
    if {[info globals devlist-$sysbname] == ""} {
	displayInfo "Retrieving device information from $sysbname" ON
	ftdmkdevlist $sysbname $SYS(primaryport) $RMDtimeout
	after 2000
        upvar #0 devlist-$sysbname sysbdev
        displayInfo "" OFF
        if {$sysbdev(status) != 0} {
            displayError $sysbdev(errmsg)
        }
    }
    $w.nb pageconfigure %Q%_Devices -state normal
    $w.nb raise %Q%_Devices
    set f [$w.nb subwidget %Q%_Devices]
    raise $f
    set xxxcur%Q% %Q%0
   # update
   # update idletasks
}

#-----------------------------------------------------------------------------
proc configTopNotebook {configname} {
    global SYS defaulttunable
    upvar #0 $configname cfg
    
    # -- abbreviation
    set w .f1
    # -- create the systems notebook subwidget
    configSystemForm [$w.nb subwidget systems] $configname $cfg(rtag) 
    # -- %Q% devices
    set t [$w.nb subwidget %Q%_Devices]
    set SYS(cur%Q%) ""
    config%Q%devpage $t $configname
    # -- Throttles
    set t [$w.nb subwidget throttles]
    configThrottlePage $t $configname
    # -- tunable parameter form
    set t [$w.nb subwidget tunables]
    configTunableMenu $t $configname

    $w.nb pageconfigure %Q%_Devices -raisecmd\
            "%Q%raise%Q%devpage $w $configname"       
    $w.nb pageconfigure systems -state normal 
    $w.nb pageconfigure %Q%_Devices -state normal 
    $w.nb pageconfigure throttles -state normal
    $w.nb pageconfigure tunables -state normal
}

#-----------------------------------------------------------------------------
proc topNotebook {configname} {
    if {[winfo exists .f1 ]} {
        configTopNotebook $configname
    } else {
        makeTopNotebook $configname
    }
}

#-----------------------------------------------------------------------------
proc makeTopNotebook {configname} {
    global SYS defaulttunable
    upvar #0 $configname cfg

    # -- abbreviation
    set w .f1

    # -- create the notebook for the first time
    frame $w 
    pack $w -expand y -fill both -side top -anchor nw        
    tixNoteBook $w.nb 
    $w.nb add systems -label "Systems" -state disabled
    $w.nb add %Q%_Devices -label "%Q% Devices" -state disabled
    $w.nb add throttles -label "Throttles" -state disabled
    $w.nb add tunables -label "Tunable Parameters" -state disabled
    pack $w.nb -expand yes -fill both  -side top
    # -- create the systems notebook subwidget
    makeSystemForm [$w.nb subwidget systems] $configname $cfg(rtag) 
    # -- %Q% devices
    set t [$w.nb subwidget %Q%_Devices]
    set SYS(cur%Q%) ""
    make%Q%devpage $t $configname
    # -- Throttles
    set t [$w.nb subwidget throttles]
    makeThrottlePage $t $configname
    # -- tunable parameter form
    set t [$w.nb subwidget tunables]
    makeTunableMenu $t $configname
    
    $w.nb pageconfigure %Q%_Devices -raisecmd\
            "%Q%raise%Q%devpage $w $configname"       
    $w.nb pageconfigure systems -state normal 
    $w.nb pageconfigure %Q%_Devices -state normal 
    $w.nb pageconfigure throttles -state normal
    $w.nb pageconfigure tunables -state normal
}

#-----------------------------------------------------------------------------
proc configSystemForm {w cfgname tag} {
    global SYS curband cfg
    upvar #0 $cfgname cfg

    set f [ $w.mainF.fp subwidget frame ]

    $f.padF.hostE configure -textvariable ${cfgname}(SYSTEM-A,HOST:)
    $f.padF.pstoreCombo configure -variable ${cfgname}(SYSTEM-A,PSTORE:)

    set f [ $w.mainF.secF subwidget frame ]

    $f.padF.hostE configure -textvariable ${cfgname}(SYSTEM-B,HOST:)
    $f.padF.journalE configure -textvariable ${cfgname}(SYSTEM-B,JOURNAL:)
    $f.padF.portE configure -textvariable ${cfgname}(secport)
    set f $f.padF
    $f.chainyesB configure -text Yes -variable ${cfgname}(CHAINING:)\
            -value on
    $f.chainnoB  configure -text No -variable ${cfgname}(CHAINING:) -value off
    set f [ $w.mainF.noteF subwidget frame ]
    $f.noteE configure -textvariable ${cfgname}(NOTES:)
}
#-----------------------------------------------------------------------------
proc makeSystemForm {w cfgname tag} {
    global SYS curband cfg RMDtimeout
    upvar #0 $cfgname cfg

    set hostname $cfg(SYSTEM-A,HOST:)
    if {[info globals devlist-$hostname] == ""} {
	displayInfo "Retrieving device information from $hostname" ON
       	ftdmkdevlist $hostname $SYS(primaryport) $RMDtimeout
       	after 2000
        upvar #0 devlist-$hostname sysadev
        displayInfo "" OFF
        if {$sysadev(status) != 0} {
            displayError $sysadev(errmsg)
        }
    }

    frame $w.mainF
    pack $w.mainF

    set w $w.mainF

    tixLabelFrame $w.fp -label "Primary System" -labelside acrosstop
    pack $w.fp -anchor n -fill x -expand 1

    set f  [ $w.fp subwidget frame]
   
    frame $f.padF
    pack $f.padF -pady .25c -padx .5c -anchor w
    
    set f $f.padF
    
    label $f.hostL -text "Hostname or IP Address: " 
    entry $f.hostE -width 30 -textvariable ${cfgname}(SYSTEM-A,HOST:)
 
    tixComboBox $f.pstoreCombo -label "Persistent Store Device:  " \
	    -dropdown true \
	    -listcmd "%Q%filldevcombobox $f.pstoreCombo 1 $hostname dsk 1" \
	    -editable true -variable ${cfgname}(SYSTEM-A,PSTORE:)\
            -validatecmd "valdevpath" \
	    -options {
	entry.width 27
	
	label.anchor w
	listbox.height 8
	listbox.width 30
	listbox.anchor e
    }

    grid $f.hostL $f.hostE - -
    grid $f.pstoreCombo
    grid $f.hostL $f.pstoreCombo -sticky w
    grid $f.hostE -sticky w -columnspan 3
    grid $f.pstoreCombo -sticky w -columnspan 3
    
    tixLabelFrame $w.secF -label "Secondary System" -labelside acrosstop
    pack $w.secF -anchor n  -fill x -expand 1
  
    set f  [ $w.secF subwidget frame]
   
    frame $f.padF
    pack $f.padF -pady .25c -padx .5c -anchor w
    
    set f $f.padF
    
    label $f.hostL -text "Hostname or IP Address: " 
    entry $f.hostE -width 30 -textvariable ${cfgname}(SYSTEM-B,HOST:)

    grid $f.hostL $f.hostE - -
    grid $f.hostL -sticky e
    grid $f.hostE -sticky w -columnspan 3
   
    label $f.journalL -text "Journal Directory: "
    entry $f.journalE -width 30 -textvariable ${cfgname}(SYSTEM-B,JOURNAL:)
 
    grid $f.journalL $f.journalE - -
    grid $f.journalL -sticky e
    grid $f.journalE -sticky w 

    label $f.portL -text "Port: "
    entry $f.portE -width 3 -textvariable ${cfgname}(secport)

    label $f.chainL -text "Allow Chaining: "
    radiobutton $f.chainyesB -text Yes -variable ${cfgname}(CHAINING:)\
            -value on
    radiobutton $f.chainnoB -text No -variable ${cfgname}(CHAINING:) -value off
    grid $f.portL $f.portE $f.chainL $f.chainyesB
    grid x x x $f.chainnoB
    grid $f.portL -sticky e
    grid $f.portE -sticky w
    grid $f.chainyesB -sticky w
    grid $f.chainnoB -sticky w
    

    tixLabelFrame $w.noteF -label "Notes" -labelside acrosstop
    pack $w.noteF -anchor n  -fill x -expand 1
    
    set f [ $w.noteF subwidget frame ]
    entry $f.noteE -width 50 -textvariable ${cfgname}(NOTES:)
    pack $f.noteE -pady .5c -padx .5c

}

    
#-----------------------------------------------------------------------------
proc %Q%fetchdevlist {cfgname} {
    global SYS RMDtimeout
    upvar #0 $cfgname cfg 
   
    foreach tag {SYSTEM-A SYSTEM-B} {
	# -- check if we have a devices list for this system
	set sysname $cfg($tag,HOST:)
	if {$sysname == ""} {
	    displayError "You must specify Host Name or IP Address first."
	    continue
	}
	displayInfo "Retrieving device information from $sysname" ON
	if {$tag == "SYSTEM-A"} {
	    ftdmkdevlist $sysname $SYS(primaryport) $RMDtimeout
	} else {
	    ftdmkdevlist $sysname $cfg(secport) $RMDtimeout
	}
	displayInfo "" OFF
	upvar #0 devlist-$sysname devs
	if {$devs(status) != 0} {
	    displayError $devs(errmsg)
	    continue
	}
    }
}

#----------------------------------------------------------------------------
proc config%Q%devpage {w cfgname} { 
    global SYS info t%Q%datadev t%Q%remark t%Q%curdev t%Q%num 
    global t%Q%mirdev

    upvar #0 $cfgname cfg 

    set sf [ $w.topF subwidget frame ]
    set f $w.l
    $f.sl configure -browsecmd "sel%Q%dev $w $cfgname"
    set f $w.r
    
    # - if any devices exist, set SYS(cur%Q%) to be first in the list
    if {[info exists cfg(%Q%devs) ]} {
        if {[llength $cfg(%Q%devs)] != 0} {
            set SYS(cur%Q%) [lindex $cfg(%Q%devs) 0]
        } else {
            set SYS(cur%Q%) "%Q%0"
        } 
        # - set the device in the %Q% Device field to be the one that
        # - is currently selected
        set xxxcur%Q% $SYS(cur%Q%)
    } else {
        # - if no devices exists, then no device will be selected
        set SYS(cur%Q%) ""
        # - but we'll start the user out with dev 0 in the %Q% Device field
        set xxxcur%Q% "%Q%0"
    }
    
    
    set %Q%name  $SYS(cur%Q%)
    set %Q%devnewflag 1
    set t%Q%remark ""
    set t%Q%datadev ""
    set t%Q%mirdev ""
    set t%Q%start ""
    set t%Q%end ""
    
    $f.%Q%f.a configure -value $xxxcur%Q%
    $f.%Q%f.refreshB  configure -command "%Q%fetchdevlist $cfgname"
    set pframe [$f.p subwidget frame]
    $pframe.datadev configure -validatecmd "val%Q%subdev $cfgname SYSTEM-A"

    
    set sframe [$f.s subwidget frame]
    $sframe.mirdev configure -validatecmd "val%Q%subdev $cfgname SYSTEM-A"
    set f $w
    $f.f.but configure -command "new%Q%dev $cfgname"
    $f.f.b1 configure  -command "commit%Q%dev $w $cfgname 0"
    $f.f.b2 configure  -command "del%Q%dev $w $cfgname"
}
#----------------------------------------------------------------------------
# %Q% device management
#----------------------------------------------------------------------------
proc make%Q%devpage {w cfgname} {

    global SYS info t%Q%datadev t%Q%remark t%Q%curdev t%Q%num 
    global xxxcur%Q% 
    global t%Q%mirdev

    upvar #0 $cfgname cfg 
      
    set lblfont  {-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*}
  
    tixLabelFrame $w.topF -label "Define %CAPQ% Devices" -labelside acrosstop

    set sf [ $w.topF subwidget frame ]
    frame $w.l 
    pack $w.topF -expand 1 -fill both

    pack $w.l -side left -expand n -fill y  -in $sf -padx .25c -pady .25c
    frame $w.r 
    pack $w.r -side right -expand y -fill both -in $sf -padx .25c -pady .25c
    frame $w.b -bd 2 -relief groove 
    pack $w.b -side bottom -fill both -expand 1  
    set f $w.l
 
    label $w.l.devlistL -text "%CAPQ% Devices" 
    pack $w.l.devlistL -anchor w  
   
    tixScrolledListBox $f.sl  -browsecmd "sel%Q%dev $w $cfgname"\
	    -scrollbar "auto +y" -width 80
    pack $f.sl -side top -fill both -expand 1
   
    $SYS(help) bind $f.sl -balloonmsg \
        "Select existing %CAPQ% device for modification or deletion"
    set f $w.r

    frame $f.infoF 
    pack $f.infoF -fill both -expand 1 
  
    set xxxcur%Q% %Q%0
    set %Q%name  $xxxcur%Q%
    set %Q%devnewflag 1
    set t%Q%remark ""
    set t%Q%datadev ""
    set t%Q%mirdev ""
    set t%Q%start ""
    set t%Q%end ""

    frame $f.%Q%f
    pack $f.%Q%f -side top -fill x -anchor w -in $f.infoF
  
    frame $f.%Q%f.f 
    tixControl $f.%Q%f.a -label "%Q% Device:  " -integer false \
            -allowempty false -autorepeat false -variable xxxcur%Q% \
            -value $xxxcur%Q% -incrcmd incrFTDdev -validatecmd validateFTDdev\
            -decrcmd decrFTDdev \
            -options {
        entry.width 5
        label.width 11
        label.anchor w
    }
    pack $f.%Q%f.f -side top -anchor w -padx 7 -pady 3 -fill x -expand 1
    pack $f.%Q%f.a  -in $f.%Q%f.f -side left
    button $f.%Q%f.refreshB -text "Refresh Device Lists" -command "%Q%fetchdevlist $cfgname" 
    pack $f.%Q%f.refreshB -pady 3 -side right -in $f.%Q%f.f
    $SYS(help) bind $f.%Q%f.a -balloonmsg \
	    "The device name for the %CAPQ% device"
    
    tixLabelEntry $f.remark -label "Remark:  " \
	    -options "
    entry.width 40
    label.width 8
    label.anchor w
    entry.textVariable t%Q%remark
    "
    pack $f.remark -side top  -anchor w -padx 7
    $SYS(help) bind $f.remark -balloonmsg \
	    "OPTIONAL:  Enter a remark describing the intended use of this %CAPQ% device"
    
   tixLabelFrame $f.p -label "On Primary System" -labelside acrosstop
   pack $f.p -side top -padx 4 -ipady 6 -anchor w -pady 4 -fill x
    
   set pframe [$f.p subwidget frame]
   
    
    tixComboBox $pframe.datadev -label "Data Device:  " \
	    -dropdown true \
	    -listcmd "%Q%filldevcombobox $pframe.datadev 1" \
	    -editable true -variable t%Q%datadev \
	    -validatecmd "val%Q%subdev $cfgname SYSTEM-A" \
	    -options {
	entry.width 30
	label.anchor w
	listbox.height 8
	listbox.width 30
	listbox.anchor e
    }
    pack $pframe.datadev -side top
    
    if {$SYS(platform) == "HP-UX"} {
        $SYS(help) bind $pframe.datadev -balloonmsg \
		"special character device path (e.g.  /dev/rdsk/c1t2d0) or select"
    } else {
        $SYS(help) bind $pframe.datadev -balloonmsg \
		"special character device path (e.g.  /dev/rdsk/c1t2d0s4) or select"
    }
    
    frame $pframe.wls1
    pack $pframe.wls1 -side top 
   
    tixLabelFrame $f.s -label "On Secondary System" -labelside acrosstop
    pack $f.s -side top -padx 4 -ipady 6 -anchor w  -pady 4 -fill x
    
    set sframe [$f.s subwidget frame]
   
    tixComboBox $sframe.mirdev -label "Mirror Device:  " \
	    -dropdown true \
	    -listcmd "%Q%filldevcombobox $sframe.mirdev 0" \
	    -editable true -variable t%Q%mirdev \
	    -validatecmd "val%Q%subdev $cfgname SYSTEM-B" \
	    -options {
	entry.width 30
	
	label.anchor w
	listbox.height 8
	listbox.width 30
	listbox.anchor e
    }

    pack $sframe.mirdev -side top
	$SYS(help) bind $sframe.mirdev -balloonmsg \
		"Define the Mirror device on Secondary System for this %CAPQ% device"

   
    set f $w
 
    frame $f.f 
    pack $f.f -expand 1 -fill y -pady .25c 
    button $f.f.but -text "Create New Device" -command "new%Q%dev $cfgname"
    pack $f.f.but -side left -padx .25c
    $SYS(help) bind $f.f.but -balloonmsg \
	"Define a new %CAPQ% device for this Logical Group"
 
    button $f.f.b1 -text "Commit Device" -command "commit%Q%dev $w $cfgname 0" 
    pack $f.f.b1 -side left -padx .25c 
    $SYS(help) bind $f.f.b1 -balloonmsg \
	"Commit the definition of this %CAPQ% device"
  
    button $f.f.b2 -text "Delete Device" -command "del%Q%dev $w $cfgname"  
    pack $f.f.b2 -side left -padx .25c 
    $SYS(help) bind $f.f.b2 -balloonmsg \
	"Delete the displayed %CAPQ% device from the Logical Group"
  
}

#-----------------------------------------------------------------------------
proc incrFTDdev {devname} {
    global SYS xxxcur%Q%
    global %Q%devnewflag
    set devs [gleenFTDdevs]
   
    if {[llength $devs] > 0} {
	set last%Q%num [string range [lindex $devs end] 3 end]
    } else {
	set last%Q%num -1
    }
    set i [string first "%Q%" $devname]
    if {$i == -1} {
	set num $last%Q%num
    } else {
	set num [string range $devname [expr $i + 3] end]
	if {[catch {expr $num * 1}]} {
	    set num $last%Q%num
	}
    }
    incr num
  
    set %Q%dev "%Q%$num"
 
    set w .f1
    set f [$w.nb subwidget %Q%_Devices]
    set f $f.r.%Q%f.a
    set f [$f subwidget incr]
    if {-1 == [lsearch -exact [info globals %Q%*] $%Q%dev]} {
        catch "unset %Q%devprev"
	set %Q%devnewflag 1
    } else {
	set %Q%devnewflag 0
    }
    set xxxcur%Q% $%Q%dev
#    after idle "$f configure -relief raised; update"
    return "$%Q%dev"
}

#-----------------------------------------------------------------------------
proc decrFTDdev {devname} {
    global SYS xxxcur%Q%
    set devs [gleenFTDdevs]
    global %Q%devnewflag
    if {[llength $devs] > 0} {
	set last%Q%num [string range [lindex $devs end] 3 end]
    } else {
	set last%Q%num -1
    }
    set i [string first "%Q%" $devname]
    if {$i == -1} {
	set num $last%Q%num
    } else {
	set num [string range $devname [expr $i + 3] end]
       
	if {[catch {expr $num * 1}]} {
	    set num $last%Q%num
	}
    }
    
    set num [expr $num - 1]
  
    if {$num == -1} {
	set num 0
    }
   
    set %Q%dev "%Q%$num"
   
    set w .f1
    set f [$w.nb subwidget %Q%_Devices]
    set f $f.r.%Q%f.a
    set f [$f subwidget decr]
    if {-1 == [lsearch -exact [info globals %Q%*] $%Q%dev]} {
        catch "unset %Q%devprev"
	set %Q%devnewflag 1
    } else {
	set %Q%devnewflag 0
    }
    set xxxcur%Q% $%Q%dev
    return "$%Q%dev"
}
#-----------------------------------------------------------------------------
proc validateFTDdev {devname} {
    global SYS 
    set devs [gleenFTDdevs]
    if {[llength $devs] > 0} {
	set last%Q%num [string range [lindex $devs end] 3 end]
    } else {
	set last%Q%num -1
    }
    set dev [lindex [file split $devname] end]
    if {[string range $dev 0 2] != "%Q%"} {
	set %Q%dev "%Q%[expr $last%Q%num + 1]"
        sel%Q%dev .f1 $SYS(candidatecfg) $%Q%dev 
	return "$%Q%dev"
    }	
    set num [string range $dev 3 end]
    if {[isntNumeric $num]} {
	set %Q%dev "%Q%[expr $last%Q%num + 1]"
        sel%Q%dev .f1 $SYS(candidatecfg) $%Q%dev 
	return "$%Q%dev"
    }
    set %Q%dev $dev
    global $SYS(candidatecfg)
    sel%Q%dev .f1 $SYS(candidatecfg)  $devname 
    return "$%Q%dev"
}
#-----------------------------------------------------------------------------
proc val%Q%subdev {cfgname tag devpath} {
    global SYS
    upvar #0 $cfgname cfg
    # -- see what happens if we do nothing
    return [lindex $devpath 0]
    if {$devpath == ""} {return}
    set devpath [lindex $devpath 0]
   
    set sysname $cfg($tag,HOST:)
    
    if {$sysname != ""} {
	upvar #0 devlist-$sysname devs
	if {![info exists devs(names)]} {return}
	if {-1 == [lsearch $devs(names) $devpath]} {
	    displayError "Device $devpath may not exist" 
	    return $devpath
	}
	if {$devs($devpath..inuse) != 0} {
	    if {[llength $devs($devpath..size)] > 5} {
		displayError "Device $devpath in use \[mounted as [lindex $devs($devpath..size) 5]\]" 1
	    } else {
		displayError "Device $devpath already in use" 
	    }
	}
    }
    return $devpath
}

#-----------------------------------------------------------------------------
proc gleenLogGrps {} {
    # -- remove incomplete logical group definitions
    global SYS
    set lglist ""
    gleenFTDdevs
    foreach lgname [lsort [info globals dsgrp*]] {
	set delflag 0
	global $lgname
	upvar #0 $lgname group
	if {![info exists group(ptag)]} {set delflag 1}
	if {![info exists group(rtag)]} {set delflag 1}
	if {![info exists group(%Q%devs)]} {
	    set delflag 1
	} elseif {$group(%Q%devs) == ""} {
	    set delflag 1
	}
	set taglist ""
	if {[info exists group(ptag)]} {lappend taglist $group(ptag)}
	if {[info exists group(rtag)]} {lappend taglist $group(rtag)}
	foreach tag [list $group(ptag) $taglist] {
	    if {(![info exists group($tag,HOST:)]) } {
		set delflag 1
	    }
	   
	    
	    if {[info exists group($tag,HOST:)]   } {
		if {$group($tag,HOST:) == ""} {
		    set delflag 1
		}
	    }
	}
	if {!$delflag} {
	    lappend lglist $lgname
	} else {
	    unset $lgname
	}
    }
    set SYS(cfgfiles) $lglist
}

#-----------------------------------------------------------------------------
proc gleenFTDdevs {} {
    # -- remove incomplete FTD device definitions
    global SYS     
    set devs [lsort -command %Q%devsort [info globals %Q%*]]
    set outdevs ""
    foreach dev $devs {
	if {[string first [string index $dev 3] "0123456789"] != -1} {
	    global $dev
	    upvar #0 $dev %Q%
	    if {(![info exists %Q%(DATA-DISK:)]) || \
		    (![info exists %Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:)]) || \
		    (![info exists %Q%(MIRROR-DISK:)]) ||  \
		    (![info exists %Q%(PRIMARY:)]) || \
		    (![info exists %Q%(SECONDARY:)])} {
		unset $dev
	    } else {
		lappend outdevs $dev
	    }
	}
    }
    return [lsort -command %Q%devsort $outdevs]
    set groups [info globals dsgrp*]
    foreach groupname $groups {
	global $groupname
	upvar #0 $groupname group
	set %Q%list ""
	foreach %Q%dev $group(%Q%devs) {
	    if {[info globals $%Q%dev] != ""} {
		lappend %Q%list $%Q%dev
	    }
	}
	set group(%Q%devs) $%Q%list
    }
}


#-----------------------------------------------------------------------------
proc update%Q%devs {w cfgname} {
    global SYS $cfgname
    global t%Q%remark t%Q%path t%Q%datadev t%Q%mirdev
    global t%Q%start t%Q%end
    upvar #0 $cfgname cfg 
    
    set f [$w.l.sl subwidget listbox]
    $f delete 0 end
  
    if {[info exists cfg(%Q%devs)] && [llength $cfg(%Q%devs)] > 0} {
	foreach d $cfg(%Q%devs) {
	    upvar #0 $d %Q%
            if {[info exists %Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:) ] } {
                catch "$f insert end $%Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:)"
            }
	}
    } 
       
   # update
}
    
#-----------------------------------------------------------------------------
proc updatedevicefields {w cfgname %Q%name} {
    global SYS $%Q%name $cfgname 
    global t%Q%remark t%Q%path t%Q%datadev t%Q%mirdev 
    global t%Q%start t%Q%end 

    set t%Q%remark ""
    set t%Q%datadev ""
    set t%Q%mirdev ""  
    set t%Q%start ""
    set t%Q%end ""
    set xxxcur%Q% ""

    if {$%Q%name == "none" } {
        return
    }
    upvar #0 $%Q%name %Q%
    
    set xxxcur%Q% $%Q%name
    if {[info exists ${%Q%name}]} {
        set xxxcur%Q% ${%Q%name}
    }   
    if {[info exists ${%Q%name}(%CAPPRODUCTNAME_TOKEN%-DEVICE:)]} { set t%Q%dev $%Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:)}
    if {[info exists ${%Q%name}(%CAPPRODUCTNAME_TOKEN%-DEVICE:)]} { set t%Q%dev $%Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:)}

    if {[info exists ${%Q%name}(REMARK:)]} {set t%Q%remark $%Q%(REMARK:)}
    if {[info exists ${%Q%name}(DATA-DISK:)]} {set t%Q%datadev $%Q%(DATA-DISK:)}
    if {[info exists ${%Q%name}(MIRROR-DISK:)]} {set t%Q%mirdev $%Q%(MIRROR-DISK:)}
  
}

#-----------------------------------------------------------------------------
proc sel%Q%dev {w cfgname {%Q%dev "-1"}} {
    global SYS $cfgname info xxxcur%Q%
    global t%Q%remark t%Q%path t%Q%datadev t%Q%mirdev 
    global t%Q%start t%Q%en t%Q%num
    global prev%Q%devwl %Q%devnewflag %Q%devprev
    upvar #0 $cfgname cfg 

    set w .f1
    set w [$w.nb subwidget %Q%_Devices] 
    set f [$w.l.sl subwidget listbox]
   
    set %Q%devnewflag 0
    if {$%Q%dev == -1} {   
        # - we're being called as a result of a click on a listbox item or
        #   on initialization.  So all we have to do is set the right value
        #   in the Devices field.  This setting will cause the validatecmd
        #   to run which will call this proc again with the current device
        #   passed as an arg
        if {[info exists cfg(%Q%devs)] == 0 || [llength $cfg(%Q%devs)] == 0} {
            # - no devices are defined so this is a new device
            set %Q%devnewflag 1
            set xxxcur%Q% %Q%0
        } else {
            # - figure out what the item is 
            set i [$f curselection]
            # - set the Device: field to the selected item.  
           
            if {[catch {set xxxcur%Q% [$f get $i]}]} {
                # - nothing selected, default to first item in list
                set xxxcur%Q% [lindex $cfg(%Q%devs) 0]
                set %Q%devnewflag 1
            }
        }
    } else {   
        # - we were called by the validate cmd of the %Q% Device field
        # - clear the current selection
        $f selection clear 0 end
        if {[info exists cfg(%Q%devs)] == 0 || [llength $cfg(%Q%devs)] == 0} {
            # - no devices are defined set current device to null
            set $SYS(cur%Q%) ""
            set %Q%devnewflag 1
        } else {
            # - find out index of item in list to select
            set index [lsearch -exact $cfg(%Q%devs) $%Q%dev]
            if {$index == -1} {
                # - if not found then just set it to null and leave selection
                #   clear
                set SYS(cur%Q%) ""
                set %Q%devnewflag 1
            } else {
                $f selection set $index
                set SYS(cur%Q%) $%Q%dev 
            }
        }
        # - update fields for device
        updatedevicefields $w $cfgname $SYS(cur%Q%)
    }
}
#-----------------------------------------------------------------------------
proc commit%Q%dev {w cfgname force} {
    global SYS $cfgname 
    global t%Q%remark t%Q%path t%Q%datadev t%Q%mirdev
    global t%Q%start t%Q%end 
    global prev%Q%devwl %Q%devnewflag %Q%devprev xxxcur%Q%
    upvar #0 $cfgname cfg 
   
    set SYS(cur%Q%) $xxxcur%Q%
    set %Q%dev $xxxcur%Q%
 
    # -- check if we have a devices list for this system
  
    set sysaname $cfg(SYSTEM-A,HOST:)
    set sysbname $cfg(SYSTEM-B,HOST:)
    
    global devlist-$sysaname devlist-$sysbname
    upvar #0 devlist-$sysaname sysadev
    upvar #0 devlist-$sysbname sysbdev
  
    set f $w.r.%Q%f.a
      
    if {$t%Q%datadev == ""} {
        if {[winfo exists .errorD]} {
	    destroy .errorD
	} 
	if {[%Q%_dialog .errorD Error \
		"You must define a Data Device." 4i\
		warning 0 Cancel "Force Commit"] == 0 } {
	    return
	}
    }
    
    if {$t%Q%mirdev == ""} {
	if {[winfo exists .errorD]} {
	    destroy .errorD
	} 
	if {[%Q%_dialog .errorD Error \
		"$SYS(cur%Q%) must have a Secondary Mirror Device" 4i\
		warning 0 Cancel "Force Commit"] == 0 } {
	    return
	}
    }

   
    if {[llength [array names sysadev ${t%Q%datadev}*]] == 0} {

        if {[catch "open ${t%Q%datadev} {RDWR EXCL} " filed]} {
            if {[winfo exists .errorD]} {
                destroy .errorD
            } 
            
            if {[%Q%_dialog .errorD Error \
                    "$t%Q%datadev does not exist or is in use" 4i\
                    warning 0 Cancel "Force Commit"] == 0 } {
                return
            }
        } else {
            close $filed
        }
    }

    if {[llength [array names sysbdev ${t%Q%mirdev}*]] == 0} {
	if {[winfo exists .errorD]} {
	    destroy .errorD
	} 
	
	if {[%Q%_dialog .errorD Error \
		"$t%Q%mirdev does not appear to exist" 4i\
		warning 0 Cancel "Force Commit"] == 0 } {
	    return
	}
    }
    # -- check for size issues
    if {[info exists sysadev($t%Q%datadev..size)]} {
	set s1 [lindex $sysadev($t%Q%datadev..size) 3]
    } else {
	set s1 -1
    }
    
    if {[info exists sysbdev($t%Q%mirdev..size)]} {
	set s2 [lindex $sysbdev($t%Q%mirdev..size) 3]
    } else {
	set s2 -1
    }
    if {$s1 != -1 && $s2 != -1} {
	if {$s2 < $s1} {
	    if {[winfo exists .errorD]} {
		destroy .errorD
	    } 
	    
	    if {[%Q%_dialog .errorD Error \
		    "Mirror Device \[$s2 sectors\] < Local Data Device\[$s1 sectors\]" 4i warning 0 Cancel "Force Commit"] == 0 } {
		return
	    }
	    
	}
    }
	
    # -- if this is a replacement, remove previous values from the
    # -- interference checking data structure
    #  
    if {!$%Q%devnewflag} {
	stashdevinfo $%Q%dev $%Q%dev %Q%devprev
    }
    if {0 != [ftddevchk $sysaname $t%Q%datadev $SYS(candidatelg) \
	    $SYS(cur%Q%) "LOCDATADEV" $force]} {
	
	if {[winfo exists .errorD]} {
	    destroy .errorD
	} 
	
	if {[%Q%_dialog .errorD Error  $sysadev(errmsg) 4i warning 0\
		Cancel "Force Commit"] == 0 } {
	    return
	}
    }
    if {0 != [ftddevchk $sysbname $t%Q%mirdev $SYS(candidatelg) \
	    "MIRRORDEV" $force]} {
	
	if {[winfo exists .errorD]} {
	    destroy .errorD
	} 
	
	if {[%Q%_dialog .errorD Error  $sysbdev(errmsg) 4i warning 0\
		Cancel "Force Commit"] == 0 } {
	    return
	}
	
    }
    # -- check for conflicts with other %Q% device definitions
    set %Q%dev [lindex [file split [lindex $SYS(cur%Q%) 0]] end]
    
    # -- it passed the audition (or was forced) add to device definitions
    ftddevadd $sysaname $SYS(cur%Q%) $SYS(candidatelg) $%Q%dev FTDDEV $force
    ftddevadd $sysaname $t%Q%datadev $SYS(candidatelg) $%Q%dev \
	    LOCDATADEV $force
    ftddevadd $sysaname $SYS(candidatelg) $%Q%dev WLDEV $force
    ftddevadd $sysbname $t%Q%mirdev $SYS(candidatelg) $%Q%dev \
	    MIRRORDEV $force
    # -- it's passed the tests, add / update the %Q% device definition
    upvar #0 $SYS(cur%Q%) %Q%

    set %Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:) $SYS(cur%Q%)
    set %Q%(PRIMARY:) $cfg(ptag)
    set %Q%(SECONDARY:) $cfg(rtag)
    set %Q%(REMARK:) $t%Q%remark
    set %Q%(DATA-DISK:) $t%Q%datadev
    set %Q%(MIRROR-DISK:) $t%Q%mirdev
  
    if {![info exists cfg(%Q%devs)]} {
	lappend cfg(%Q%devs) $SYS(cur%Q%)
	set cfg(%Q%devs) [lsort -command %Q%devsort $cfg(%Q%devs)]
    } elseif {-1 == [lsearch -exact $cfg(%Q%devs) $SYS(cur%Q%)]} {
	lappend cfg(%Q%devs) $SYS(cur%Q%)
	set cfg(%Q%devs) [lsort -command %Q%devsort $cfg(%Q%devs)]
    } 
    
    set xxx $SYS(cur%Q%)

    update%Q%devs $w $cfgname

    if {$%Q%devnewflag} {
	displayInfo "Committed the definition for $xxx"
    } else {
	displayInfo "Committed the modification of $xxx"
    }
    set xxxcur%Q% $xxx 
}

#-----------------------------------------------------------------------------
proc stashdevinfo {%Q%dev fromvar tovar} {
    global SYS
    global t%Q%remark t%Q%path t%Q%datadev t%Q%mirdev 
    global t%Q%start t%Q%end 
    global prev%Q%devwl %Q%devnewflag %Q%devprev
    global $SYS(candidatecfg)
    upvar #0 $SYS(candidatecfg) cfg
    global $fromvar $tovar
    upvar #0 $fromvar %Q%from
    upvar #0 $tovar %Q%to
 
    set sysaname $cfg(SYSTEM-A,HOST:)
    set sysbname $cfg(SYSTEM-B,HOST:)
   
    global devlist-$sysaname devlist-$sysbname
    upvar #0 devlist-$sysaname sysadev
    upvar #0 devlist-$sysbname sysbdev
    catch "unset $tovar"
    array set %Q%to [array get %Q%from]
    global $%Q%dev
    upvar #0 $%Q%dev %Q%

    if {[info exists %Q%from(%CAPPRODUCTNAME_TOKEN%-DEVICE:)]} {
	ftddevdel $sysaname $%Q%from(%CAPPRODUCTNAME_TOKEN%-DEVICE:) \
	    $SYS(candidatelg) $%Q%dev FTDDEV
    }
    if {[info exists %Q%from(DATA-DISK:)]} {
	ftddevdel $sysaname $%Q%from(DATA-DISK:) \
	    $SYS(candidatelg) $%Q%dev LOCDATADEV
    }
    if {[info exists %Q%from(MIRROR-DISK:)]} {
	ftddevdel $sysbname $%Q%from(MIRROR-DISK:) \
	    $SYS(candidatelg) $%Q%dev MIRRORDEV
    }

}

#-----------------------------------------------------------------------------
proc restoredevinfo {%Q%dev fromvar tovar} {
    global SYS $cfgname 
    global t%Q%remark t%Q%path t%Q%datadev t%Q%mirdev 
    global t%Q%start t%Q%end 
    global $fromvar $tovar
    global prev%Q%devwl %Q%devnewflag %Q%devprev
    upvar #0 $fromvar %Q%from
    upvar #0 $tovar %Q%to
    upvar #0 $SYS(candidatecfg) cfg
    
    set sysaname $cfg(SYSTEM-A,HOST:)
    set sysbname $cfg(SYSTEM-B,HOST:)
    
    upvar #0 devlist-$sysaname sysadev
    upvar #0 devlist-$sysbname sysbdev
    catch "unset $tovar"
    array set %Q%to [array get %Q%from]
    if {[info exists %Q%from(%CAPPRODUCTNAME_TOKEN%-DEVICE:)]} {
	ftddevadd $sysaname $%Q%from(%CAPPRODUCTNAME_TOKEN%-DEVICE:) \
	    $SYS(candidatelg) $%Q%dev FTDDEV 1
    }
    if {[info exists %Q%from(DATA-DISK:)]} {
	ftddevadd $sysaname $%Q%from(DATA-DISK:) \
	    $SYS(candidatelg) $%Q%dev LOCDATADEV 1
    }
    if {[info exists %Q%from(MIRROR-DISK:)]} {
	ftddevadd $sysbname $%Q%from(MIRROR-DISK:) \
	    $SYS(candidatelg) $%Q%dev MIRRORDEV 1
    }

}

#-----------------------------------------------------------------------------
proc %Q%isint {s} {
    if {[llength $s] != 1} {return 0}
    if {[string length $s] == 0} {return 0}
    foreach c [split [lindex $s 0] ""] {
	if {$c < "0" || $c > "9"} {return 0}
    }
    return 1
}

#-----------------------------------------------------------------------------
proc %Q%filldevcombobox {f {pflag 1} {ahost ""} {type rdsk} {pstoreflag 0} } {
    global SYS

    if {$ahost==""} {
        set cfgname $SYS(candidatecfg)
        global $cfgname
        upvar #0 $cfgname cfg
        if {$pflag} {
            set tag $cfg(ptag)
        } else {
            set tag $cfg(rtag)
        }
        set sysname $cfg($tag,HOST:)
    } else {
        set sysname $ahost
    }
    global devlist-$sysname
    upvar #0 devlist-$sysname devs
    set lb [$f subwidget listbox]
    $lb delete 0 end
    if {![info exists devs(linewidth)]} {
	set devs(linewidth) 60
    }
    $lb configure -width $devs(linewidth)
    $f configure -state disabled
    if {[info exists devs(names)]} {
	foreach n $devs(names) {
	    if {[info exists devs($n..listentry)]} {
                set tempentry $devs($n..listentry)
                if {(!$pstoreflag) && $pflag && \
                        $cfg(SYSTEM-A,PSTORE:) != "" } {
                    regsub "/dsk/" $cfg(SYSTEM-A,PSTORE:) "/rdsk/" pstorerdsk
                    if {[string match "$pstorerdsk*" $devs($n..listentry)]} {
                        regsub "AVAIL"  $tempentry "INUSE" tempentry
                        regsub "SECT]" $tempentry "SECT (pstore)]" tempentry
                    }
                }
                if {$type=="rdsk"} {
                    $lb insert end $tempentry
                } else {
                    if  { $SYS(platform) == "HP-UX" } {
                        regsub (/dev/.*/)r(.*) $tempentry {\1\2} x
                    } elseif { $SYS(platform) == "AIX" } {
                        regsub {/dev/r} $tempentry {/dev/} x 
                    } else {
                        regsub "/rdsk/" $tempentry "/dsk/" x
                    }
                    $lb insert end $x
                }  
                    
	    }
	}
    } 
    $f configure -state normal
   # update
}

#-----------------------------------------------------------------------------
proc del%Q%dev {w cfgname} {
    global SYS $cfgname 
    global t%Q%remark t%Q%path t%Q%datadev t%Q%mirdev
    global t%Q%start t%Q%end 
 
    upvar #0 $cfgname cfg 
    if {![info exists SYS(cur%Q%)]} {
	displayError "No %CAPQ% device selected - cannot delete"
	return
    }
    if {$SYS(candidatecfg) == "" || $SYS(cur%Q%) == ""} {
	displayError "No %CAPQ% device selected - cannot delete"
	return
    }
    if {![info exists ${cfgname}(%Q%devs)]} {
	displayError "No %CAPQ% device selected - cannot delete"
	return
    }
    set i [lsearch -exact $cfg(%Q%devs) $SYS(cur%Q%)]
    if {-1 == $i} {
	displayError "Current %CAPQ% device not committed so cannot delete it"
	return
    }
    if {$i == 0} {
	set cfg(%Q%devs) [lrange $cfg(%Q%devs) 1 end]
    } elseif {$i == [expr [llength $cfg(%Q%devs)] - 1]} {
	incr i -1
	set cfg(%Q%devs) [lrange $cfg(%Q%devs) 0 $i]
    } else {
	set j [expr $i + 1]
	incr i -1
	set cfg(%Q%devs) [concat [lrange $cfg(%Q%devs) 0 $i] \
			      [lrange $cfg(%Q%devs) $j end]]
    }
    global $SYS(cur%Q%)
    upvar #0 $SYS(cur%Q%) %Q%
    set prev%Q%devwl ""
  
    set sysaname $cfg(SYSTEM-A,HOST:)
    set sysbname $cfg(SYSTEM-B,HOST:)
   
    if {[info exists %Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:)] && $%Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:) != ""} {
	ftddevdel $sysaname $%Q%(%CAPPRODUCTNAME_TOKEN%-DEVICE:) $SYS(candidatelg) $SYS(cur%Q%) FTDDEV
    }
    if {[info exists %Q%(DATA-DISK:)] && $%Q%(DATA-DISK:) != ""} {
	ftddevdel $sysaname $%Q%(DATA-DISK:) $SYS(candidatelg) $SYS(cur%Q%) LOCDATADEV
    }
   
    if {[info exists %Q%(MIRROR-DISK:)] && $%Q%(MIRROR-DISK:) != ""} {
	ftddevdel $sysbname $%Q%(MIRROR-DISK:) $SYS(candidatelg) \
                $SYS(cur%Q%) MIRRORDEV
    }
    set xxx $SYS(cur%Q%)
    catch "unset $SYS(cur%Q%)"
    update%Q%devs $w $cfgname
    updatedevicefields $w $cfgname none
    displayInfo "Deleted %CAPQ% device definition for:  $xxx"

}

proc new%Q%dev {cfgname} {
    global SYS  t%Q%datadev t%Q%remark t%Q%mirdev
    global $SYS(cur%Q%) %Q%devnewflag xxxcur%Q%
    upvar #0 $cfgname cfg

    set lastdevice [lindex $cfg(%Q%devs) end ]
    if {$lastdevice == ""} {
        set %Q%num 0
    } else {
        set %Q%num [ string range $lastdevice 3 end  ]
        incr %Q%num
    }

    set %Q%devnewflag 1
    set t%Q%datadev ""
    set t%Q%mirdev ""
    set t%Q%remark ""

    set xxxcur%Q% "%Q%${%Q%num}"
    set w [.f1.nb subwidget %Q%_Devices] 
    set listbox  [$w.l.sl subwidget listbox]
    $listbox selection clear 0 end
}

#----------------------------------------------------------------------------
# Throttle Page Configure
#----------------------------------------------------------------------------
proc configThrottlePage {w cfgname} {
    global SYS $cfgname always tfrom tto tdates throttext 
    
    upvar #0 $cfgname cfg

    set W $w  
    if {![info exists cfg(throttletext)]} {
        set cfg(throttletext) ""
    }
    
    set w [$w.throtF subwidget frame]
    set throttext $w.throtT
    $throttext delete 0.0 end
    $throttext insert 0.0 $cfg(throttletext)

    set f $W.rightF
    set f [ $f.tracF subwidget frame ]
    set f $f.padF
    
    $f.traconRB configure -variable ${cfgname}(TRACETHROTTLE:)
    $f.tracoffRB configure -variable ${cfgname}(TRACETHROTTLE:)
}
#----------------------------------------------------------------------------
# Throttle Management
#----------------------------------------------------------------------------
proc makeThrottlePage {w cfgname} {
    global SYS $cfgname always tfrom tto tdates throttext 
    
    upvar #0 $cfgname cfg

    set W $w
    set always 1
   
    if {![info exists cfg(throttletext)]} {
        set cfg(throttletext) ""
    }
    tixLabelFrame $w.throtF -label "Throttle Editor"\
	    -labelside acrosstop
    pack $w.throtF -anchor n -side left
    set w [$w.throtF subwidget frame]

    frame $w.f
    pack $w.f -padx .5c -pady .5c -anchor nw
    
    text $w.throtT -width 40 -height 15 -yscrollcommand [list $w.ySB set]
    scrollbar $w.ySB -orient vertical -command [list $w.throtT yview]

    grid $w.throtT $w.ySB -sticky news -in $w.f

    set throttext $w.throtT
   
    $throttext insert 0.0 $cfg(throttletext) 
    
    frame $W.rightF
    pack $W.rightF -side left -anchor n

    set f $W.rightF

    tixLabelFrame $f.tracF -label "Throttle Tracing" -labelside acrosstop 
    pack $f.tracF -anchor nw -expand 1 -fill x

    set f [ $f.tracF subwidget frame ]

    frame $f.padF 
    pack $f.padF -pady .25c -padx .25c
    set f $f.padF

    
    radiobutton $f.traconRB -text "On" -variable ${cfgname}(TRACETHROTTLE:) -value on
    radiobutton $f.tracoffRB -text "Off" -variable ${cfgname}(TRACETHROTTLE:) -value off

    grid $f.traconRB -sticky w
    grid $f.tracoffRB -sticky w

    button $W.rightF.buildB -text "Throttle Builder..." -command buildThrottle
    pack $W.rightF.buildB -pady .25c -anchor nw

}

#----------------------------------------------------------------------------
# Adds a definition built by the throttle builder to the throttle text area
#----------------------------------------------------------------------------
proc addthrot {} {
    global DAYS SYS tfrom tdays tto tdates always finalexpr tactions throttext
    global $SYS(candidatecfg)
    upvar #0 $SYS(candidatecfg) cfg

    if {$always == 1 } { 
	set tfrom "-"
	set tto   "-"
	set tdayfield "-"
    } else {
	if { $tdates == "-" || $tdates == ""} {
	    if { $tdays == "" } {
		set tdayfield "-"
	    } else {
		set tdayfield [string trimright $tdays ","]
	    }
	} else {
	    set tdayfield [concat $tdays $tdates]
	}
        set tdayfield [join $tdayfield ""]
    }
    set w $throttext
    set tactions [string trimright $tactions "\n"]
    regsub -all "ACTION:" $tactions "\tACTION:" tactions

    set thrtext "THROTTLE:  $tdayfield $tfrom $tto $finalexpr \nACTIONLIST\n$tactions\nENDACTIONLIST\n"
    $w insert insert $thrtext
}
proc checktime {} {
    global tfrom tto
    
    if {$tfrom == "" || $tto == "" } {
	displayError "Must specify FROM and TO times for throttle"
	return 0
    }
    if {$tfrom != "-" && ![string match \
            {[0-2][0-9]:[0-5][0-9]:[0-5][0-9]} $tfrom] } {
        displayError "FROM must be either \"-\" or \"HH:MM:SS\""
        return 0        
    }
    if {$tto != "-" && ![string match {[0-2][0-9]:[0-5][0-9]:[0-5][0-9]} $tto]} {
	displayError "TO must be either \"-\" or \"HH:MM:SS\""
	return 0
    }
    if {$tfrom == "-" && $tto != "-"} {
	displayError "FROM and TO must either both be \"-\" or \"HH:MM:SS\""
	return 0
    }
    if {$tfrom != "-" && $tto == "-"} {
	displayError "FROM and TO must either both be \"-\" or \"HH:MM:SS\""
	return 0
    }
    return 1
}
#----------------------------------------------------------------------------
# Checks valitidy of dates and day specifications
#----------------------------------------------------------------------------
proc checkall {} {
    if { [checktime] && [checkdate] && [checkdays] } {
	buildThrot2 
    } else { return }
  
}

#----------------------------------------------------------------------------
# Checks valitidy of dates string
#----------------------------------------------------------------------------
proc checkdate {} {
    global tdates DATES

    set datelist {}
    foreach d [lsort [array names DATES]] {
        if {$DATES($d)} {
            if {[llength $datelist] !=0} { 
                lappend datelist ","
            }
            lappend datelist $d
        }
    }
    set tdates [join $datelist ""]
    return 1
}

#----------------------------------------------------------------------------
# Builds list of days for throttle from DAYS array element settings
#----------------------------------------------------------------------------
proc checkdays {} {
    global tdays DAYS
    
    set tdays ""
   
    foreach day [array names DAYS] {
	if { $DAYS($day) } {
	    set prefix [string range $day 0 1] 
	    set tdays [concat $tdays "${prefix},"]
	}
    }
    set tdays [join $tdays]
    return 1
}

proc buildThrottle {} {
    global tfrom tto tdates always DAYS DATES finalexpr

    set tfrom "-"
    set tto   "-"
    set tdates "-"
    set finalexpr ""

    set always 1
   
    foreach day {Sun Mon Tues Weds Thurs Fri Sat} {
        set DAYS($day) 0
    }
    for {set i 1 } { $i < 32 } {incr i} {
        set DATES($i) 0
    }
    
    buildThrot1

}
#----------------------------------------------------------------------------
# Creates the "Throttle Builder" first window that sets values for evaluation
# times
#----------------------------------------------------------------------------
proc buildThrot1 {} {
    global SYS always DAYS DATES tfrom tto tdates tdowhatlist
   

    if {[winfo exists .buildT] } {
	destroy .buildT
    }

    if { $always == 1 } { 
	set whenstate disabled
        if {$SYS(platform) == "HP-UX"} {
            set lblclr grey50
        } else {
            set lblclr #a3a3a3
        }
    } else {
	set whenstate normal
	set lblclr black
    }
    toplevel .buildT
    wm transient .buildT [winfo toplevel .]
    wm title .buildT "Throttle Builder"
    set oldFocus [focus]
    set oldGrab [grab current .buildT]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab .buildT"
    
    frame .buildT.topF
    frame .buildT.botF
    
    pack .buildT.topF -side top
    pack .buildT.botF -side bottom

    set f .buildT.topF 
    
    tixLabelFrame $f.evalF -label "Evaluation Time"\
	    -labelside acrosstop
    pack $f.evalF 
    
    set f [ $f.evalF subwidget frame ]
    
    frame $f.alwaysF
    pack $f.alwaysF -side top -anchor nw  -padx 1c
    label $f.titleL -text "When should this throttle be evaluated?"
    pack $f.titleL -in $f.alwaysF -anchor nw
    radiobutton $f.alwaysCB -text "Always" -variable always -value 1
    radiobutton $f.onlywhenCB -text "Only on..." -variable always -value 0

    pack $f.alwaysCB $f.onlywhenCB -anchor w -in $f.alwaysF

    frame $f.whenF -relief ridge -bd 2
    pack $f.whenF -anchor nw -padx 1c

    frame $f.wkdayF 
    pack $f.wkdayF -side top -fill y -expand 1 -pady .25c -padx .25c\
	    -in $f.whenF

    set i 0
    foreach day {Sun Mon Tues Weds Thurs Fri Sat} {
	checkbutton $f.wkdayF.on$day -text $day -variable DAYS($day) -state $whenstate -indicatoron 0 -selectcolor grey
        
        grid $f.wkdayF.on$day -column $i -row 0 -sticky news
        grid columnconfigure $f.wkdayF $i -minsize 1c
        incr i
    }
    if {![catch {exec "/bin/date"} datestr]} {
        switch [lindex $datestr 0] {
            "Sun" {set dayoffset 0}
            "Mon" {set dayoffset 1}
            "Tue" {set dayoffset 2}
            "Wed" {set dayoffset 3}
            "Thu" {set dayoffset 4}
            "Fri" {set dayoffset 5}
            "Sat" {set dayoffset 6}
            default {set dayoffset 0}
        }
    } else {
        set dayoffset 0
    }
            
    for {set i $dayoffset} {$i < [expr 31 + $dayoffset]} { incr i} {
        set d [expr $i + 1 - $dayoffset]
        set k [expr $i / 7 + 1]
        set j [expr ( $i % 7)]
        checkbutton $f.wkdayF.ondate_$d -text $d -variable DATES($d) -indicatoron 0 -state $whenstate -selectcolor grey
        grid $f.wkdayF.ondate_$d -column $j -row $k -sticky news
        
    }

    checkbutton $f.wkdayF.ondate_last -text "Last Day of Month" -variable DATES(em) -indicatoron 0 -state $whenstate -selectcolor grey
    grid $f.wkdayF.ondate_last -column 2 -row [expr $k + 1] -columnspan 3
    frame $f.rightF 
   pack $f.rightF -side right -fill y -expand 1 -pady .25c -padx .25c \
	    -in $f.whenF

    set f $f.rightF 

   
    label $f.fromL -text "From (HH:MM:SS):"  -fg $lblclr
    entry $f.fromE -width 8 -state $whenstate -textvariable tfrom 
    $SYS(help) bind $f.fromE -balloonmsg \
	"Enter either \"-\" for always, or \"HH:MM:SS\" for ending effective time"
    

    grid $f.fromL $f.fromE  -pady 5
    grid $f.fromL -sticky e
    grid $f.fromE -sticky w

    label $f.toL -text "To (HH:MM:SS):"  -fg $lblclr
    entry $f.toE -width 8 -state $whenstate -textvariable tto
   
    $SYS(help) bind $f.toE -balloonmsg \
	"Enter either \"-\" for always, or \"HH:MM:SS\" for ending effective time"
    grid $f.toL $f.toE -pady 5
    grid $f.toL -sticky e 
    grid $f.toE -sticky w

   # trace variable tfrom w setThrotVar
   # trace variable tto w setThrotVar

    set f .buildT.botF

    frame $f.butF
    pack $f.butF -pady .25c 

    button $f.nextB -text "Next" -command "checkall"
    button $f.cancelB -text "Cancel" -command\
	    {destroy .buildT}
    pack $f.nextB -in $f.butF -side left -padx .25c
    pack $f.cancelB -in $f.butF -side left -padx .25c 

    center_window .buildT
    while {1} {
	tkwait variable always
	
	if {!$always} {
	    set f [ .buildT.topF.evalF subwidget frame ]
	    foreach day {Sun Mon Tues Weds Thurs\
		    Fri Sat} {
		 $f.wkdayF.on$day configure -state normal
	    } 
                        
            for {set i 0} {$i < 31} { incr i} {
                set d [expr $i + 1 ]
                $f.wkdayF.ondate_$d configure -state normal
            }
            $f.wkdayF.ondate_last configure -state normal
	    set f $f.rightF
	    
	   
	    $f.fromL configure -fg black
	    $f.toL configure -fg black
            $f.fromE configure -state normal
	    $f.toE configure -state normal 
            
	} else {
	    
	    set f [ .buildT.topF.evalF subwidget frame ]
	    foreach day {Sun Mon Tues Weds Thurs\
		    Fri Sat} {
		set DAYS($day) 0
		$f.wkdayF.on$day configure -state disabled 
	    } 
            for {set i 0} {$i < 31} { incr i} {
                set d [expr $i + 1 ]
                $f.wkdayF.ondate_$d configure -state disabled
            }
            $f.wkdayF.ondate_last configure -state disabled
	    set f $f.rightF
           
	    
	    $f.fromL configure -fg #a3a3a3
	    $f.toL configure -fg #a3a3a3
	  
            $f.fromE configure -state disabled 
	  
	    $f.toE configure -state disabled
	}
    }
}
#----------------------------------------------------------------------------
# Creates second throttle builder window which builds the throttle expression
#----------------------------------------------------------------------------
proc buildThrot2 {} {
    global SYS always DAYS testvar testval testop testand finalexpr
   
    set ttestlist "NETKBPS PCTCPU PCTBABINUSE NETCONNECTFLAG PID CHUNKSIZE STATINTERVAL MAXSTATFILESIZE TRACETHROTTLE SYNCMODE SYNCMODEDEPTH SYNCMODETIMEOUT COMPRESSION NETMAXKBPS CHUNKDELAY DRIVERMODE ACTUALKBPS EFFECTKBPS PERCENTDONE ENTRIES READKBPS WRITEKBPS"
    set ttestlist [lsort $ttestlist]

    set treloplist ">= > == != < <= T>= T> T== T!= T< T<="
    if {[winfo exists .buildT] } {
	destroy .buildT
    }

    toplevel .buildT

    wm title .buildT "Throttle Builder"
    
    frame .buildT.topF
    frame .buildT.botF
    
    pack .buildT.topF -side top
    pack .buildT.botF -side bottom

    set f .buildT.topF 
    
    tixLabelFrame $f.evalF -label "Build Expression"\
	    -labelside acrosstop
    pack $f.evalF 
    
    set f [ $f.evalF subwidget frame ]

    frame $f.pad0F 
    pack $f.pad0F -pady .5c -padx .5c

    frame $f.padF
    pack $f.padF -in $f.pad0F

   
    set f $f.padF
    frame $f.testF -relief groove -bd 1
    pack $f.testF -side left
    frame $f.testexpF 
    pack $f.testexpF -in $f.testF

    tixComboBox $f.testvar -label "Variable:  " \
	    -dropdown true  \
	    -editable false \
	    -variable testvar\
	    -options {
	entry.width 12
	listbox.height 4
	listbox.width 17
	label.anchor w
    }
    
    set testvarlist [$f.testvar subwidget listbox]
    $testvarlist delete 0 end
    foreach testvar $ttestlist {
	$testvarlist insert end $testvar
    }

    tixComboBox $f.testop -label "Operand:  " \
	    -dropdown true  \
	    -editable false \
	    -variable testop\
	    -options {
	entry.width 3
	listbox.height 4
	listbox.width 3
	label.anchor w
    }
    
    set testoplist [$f.testop subwidget listbox]
    $testoplist delete 0 end
    foreach testop $treloplist  {
	$testoplist insert end $testop
    }
    
    set testval 0
    label $f.testvalL -text "Value:"
    entry $f.testval -width 3 -textvariable testval
   
    button $f.testB -text "Add To Expression" -command {
	set f [ .buildT.topF.evalF subwidget frame ]
	$f.exprE insert end "$testvar $testop $testval "
    }

    pack  $f.testB -in $f.testF -pady 4
  
    pack $f.testvar  $f.testop $f.testvalL $f.testval -side left -in $f.testexpF

    set f [ .buildT.topF.evalF subwidget frame ]
    set f $f.padF
    frame $f.rightF  -relief groove -bd 1
    pack $f.rightF -side right

    tixComboBox $f.andor -label "Boolean Operator:  " \
	    -dropdown true  \
	    -editable false \
	    -variable testand \
	    -options {
	entry.width 4
	listbox.height 4
	listbox.width 4
	label.anchor w
    }
    
    set andorlist [$f.andor subwidget listbox]
    $andorlist delete 0 end
    foreach andor {AND OR} {
	$andorlist insert end $andor
    }
  
    pack $f.andor -in $f.rightF


    button $f.andorB -text "Add To Expression" -command {
	set f [ .buildT.topF.evalF subwidget frame ]
	$f.exprE insert end "$testand "
    }

    pack $f.andorB  -in $f.rightF  -pady 4


    set f [ .buildT.topF.evalF subwidget frame ]
    button $f.clearB -text "Clear Expression" -command {
        set f [ .buildT.topF.evalF subwidget frame ]
	$f.exprE delete 0 end
    }
    pack $f.clearB  -side bottom
    entry $f.exprE -width 70 -textvariable finalexpr
    pack $f.exprE -side bottom
  
    set f .buildT.botF

    frame $f.butF
    pack $f.butF -pady .25c 

   
    button $f.backB -text "Back" -command "buildThrot1"
    button $f.nextB -text "Next" -command "buildThrot3"

    button $f.cancelB -text "Cancel" -command\
	    {destroy .buildT}
    pack $f.backB -in $f.butF -side left -padx .25c
    pack $f.nextB -in $f.butF -side left -padx .25c
    pack $f.cancelB -in $f.butF -side left -padx .25c 
    

    center_window .buildT
}
#----------------------------------------------------------------------------
# Creates third throttle builder window which builds the actions list
#----------------------------------------------------------------------------
proc buildThrot3 {} {
    global SYS action throtarg tactions tvarlist

    set tdowhatlist {set incr decr}
    lappend tdowhatlist {do console} 
    lappend tdowhatlist {do log} 
    lappend tdowhatlist {do mail} 
    lappend tdowhatlist {do exec}

    set tvarlist { {} CHUNKSIZE STATINTERVAL MAXSTATFILESIZE TRACETHROTTLE SYNCMODE SYNCMODEDEPTH SYNCMODETIMEOUT COMPRESSION NETMAXKBPS CHUNKDELAY}

    if {[winfo exists .buildT] } {
	destroy .buildT
    }

    toplevel .buildT

    wm title .buildT "Throttle Builder"
    
    frame .buildT.topF
    frame .buildT.botF
    
    pack .buildT.topF -side top
    pack .buildT.botF -side bottom

    set f .buildT.topF 

    tixLabelFrame $f.evalF -label "Build Actions"\
	    -labelside acrosstop
    pack $f.evalF 
    
    set f [ $f.evalF subwidget frame ]

   
    frame $f.padF -relief groove -bd 1
    pack $f.padF -pady .5c -padx .5c

   
    set f $f.padF
   
    frame $f.actionF
    pack $f.actionF

    tixComboBox $f.action -label "Action:  " \
	    -dropdown true  \
	    -editable false \
	    -variable action\
	    -options {
	entry.width 8
	listbox.height 4
	listbox.width 8
	label.anchor w
    }
    
    set actionlist [$f.action subwidget listbox]
    $actionlist delete 0 end
    foreach action $tdowhatlist {
	$actionlist insert end $action
    }
   
    tixComboBox $f.vars -label "Variable:  " \
	    -dropdown true  \
	    -editable false \
	    -variable tvar \
	    -options {
	entry.width 12
	listbox.height 4
	listbox.width 17
	label.anchor w
    }
    
    set varlist [$f.vars subwidget listbox]
    $varlist delete 0 end
    foreach var $tvarlist {
	$varlist insert end $var
    }

    label $f.throtargL -text "Argument:"
    entry $f.throtarg -width 30 -textvariable throtarg


    button $f.testB -text "Add To Action List" -command {
	set f [ .buildT.topF.evalF subwidget frame ]
	set f $f.padF
	$f.actionT insert end "ACTION: $action $tvar $throtarg\n"
    }
    
    pack $f.action $f.vars $f.throtargL $f.throtarg -side left -in $f.actionF

    pack  $f.testB -pady 4

    text $f.actionT -width 80 -height 5
    
    pack $f.actionT  

    frame .buildT.botF.butF
    pack .buildT.botF.butF -pady .25c 
    set f2 .buildT.botF

    button $f2.backB -text "Back" -command "buildThrot2"
    button $f2.nextB -text "Done" -command "set tactions \[$f.actionT get 1.0 end\];destroy .buildT; addthrot"

    button $f2.cancelB -text "Cancel" -command\
	    {destroy .buildT}
    pack $f2.backB -in .buildT.botF.butF -side left -padx .25c
    pack $f2.nextB -in .buildT.botF.butF -side left -padx .25c
    pack $f2.cancelB -in .buildT.botF.butF -side left -padx .25c 
    
    center_window .buildT
}
    
    
#-----------------------------------------------------------------------------
proc setThrotVar {var elem op} {
    global SYS
    global tthrottlelist tthrottle tthrotidx
    global tfrom tto ttest ttestlist trelop treloplist tvalue
    global tactionlist taction tdowhat tdowhatlist tactargs tactidx
    switch -exact -- $var {
	tvalue {
	    set out ""
	    if {[string length $tvalue] > 0} {
		foreach x [split $tvalue ""] {
		    if {-1 == [lsearch -exact "0 1 2 3 4 5 6 7 8 9" $x]} {
			displayError "Only digits 0 through 9 allowed"
		    } else {
			append out $x
		    }
		}
	    }
	    set tvalue $out
	}
	tfrom -
	tto {
	    upvar #0 $var val
	    set len [string length $val]
	    if {$len == 0} {set val "-"}
	    if {$len == 1 && $val == "-"} {return}
	    set out ""
	    set xl [split $val ""]
	    if {$len >= 1} {
		set x [lindex $xl 0]
		if {-1 == [lsearch -exact "0 1 2 3 4 5 6 7 8 9" $x]} {
		    displayError "Only digits 0 through 9 allowed"
		} else {
		    append out $x
		}
	    }
	    if {$len >= 2} {
		set x [lindex $xl 1]
		if {-1 == [lsearch -exact "0 1 2 3 4 5 6 7 8 9" $x]} {
		    displayError "Only digits 0 through 9 allowed"
		} else {
		    append out $x
		}
	    }
	    if {$len >= 3} {
		set x [lindex $xl 2]
		if {$x != ":"} {
		    displayError "Character must be a colon \":\""
		} else {
		    append out $x
		}
	    }
	    if {$len >= 4} {
		set x [lindex $xl 3]
		if {-1 == [lsearch -exact "0 1 2 3 4 5 6 7 8 9" $x]} {
		    displayError "Only digits 0 through 9 allowed"
		} else {
		    append out $x
		}
	    }
	    if {$len >= 5} {
		set x [lindex $xl 4]
		if {-1 == [lsearch -exact "0 1 2 3 4 5 6 7 8 9" $x]} {
		    displayError "Only digits 0 through 9 allowed"
		} else {
		    append out $x
		}
	    }
	    if {$len >= 6} {
		set x [lindex $xl 5]
		if {$x != ":"} {
		    displayError "Character must be a colon \":\""
		} else {
		    append out $x
		}
	    }
	    if {$len >= 7} {
		set x [lindex $xl 6]
		if {-1 == [lsearch -exact "0 1 2 3 4 5 6 7 8 9" $x]} {
		    displayError "Only digits 0 through 9 allowed"
		} else {
		    append out $x
		}
	    }
	    if {$len >= 8} {
		set x [lindex $xl 7]
		if {-1 == [lsearch -exact "0 1 2 3 4 5 6 7 8 9" $x]} {
		    displayError "Only digits 0 through 9 allowed"
		} else {
		    append out $x
		}
	    }
	    if {$len > 8} {
		displayError "Time must be given as HH:MM:SS"
	    }
	    set val $out
	}
	tactargs {
	    set w .f1
	    if {[info commands $w.nb] == ""} {return}
	    set w [$w.nb subwidget throttles]
	    if {$tdowhat == "" && $tactargs == ""} {
		set taction ""
	    } 
	    if {$tdowhat == "" && $tactargs != ""} {
		displayError "Select \"Do What\" option first"
		set tactargs ""
		set taction ""
		$w.b.sa configure -selection $taction
		return
	    }
	    if {$tdowhat != ""} {
		set taction "ACTION: $tdowhat $tactargs"
	    }
	    $w.b.sa configure -selection $taction
	    return
	}
    }
    set w .f1
    if {[info commands $w.nb] == ""} {return}
    set w [$w.nb subwidget throttles] 
    set tthrottle "THROTTLE $tfrom $tto $ttest $trelop $tvalue"
    $w.t.t configure -selection $tthrottle
}

proc enableSyncParams {f} {
    $f.depthL configure -fg black
    $f.timeoutL configure -fg black
    $f.secsL configure -fg black
    $f.depthE configure -bg white -fg black -state normal
    $f.timeoutE configure -bg white -fg black -state normal
}
proc disableSyncParams {f} {
    $f.depthL configure -fg gray
    $f.timeoutL configure -fg gray
    $f.secsL configure -fg gray
    $f.depthE configure -bg lightgray  -fg gray -state disabled
    $f.timeoutE configure -bg lightgray -fg gray -state disabled
}
proc disableStatParams {f} {
    $f.intervalL configure -fg gray
    $f.maxsizeL configure -fg gray
    $f.secsL configure -fg gray
    $f.kbL configure -fg gray
    $f.intervalE configure -bg lightgray  -fg gray -state disabled
    $f.maxsizeE configure -bg lightgray -fg gray -state disabled
}
proc enableStatParams {f} {
    $f.intervalL configure -fg black
   $f.maxsizeL configure -fg black
    $f.secsL configure -fg black
    $f.kbL configure -fg black
   $f.intervalE  configure -bg white -fg black -state normal
      $f.maxsizeE configure -bg white -fg black -state normal
}
proc disableNetParams {f} {
    $f.netL configure -fg gray
    $f.kbpsL configure -fg gray
    $f.netE configure -bg lightgray  -fg gray  -state disabled 
  
    
}
proc enableNetParams {f} {
   $f.netL configure -fg black  
    $f.kbpsL configure -fg black
    $f.netE  configure -bg white -fg black -state normal

}
#-----------------------------------------------------------------------------
proc configTunableMenu {w cfgname} {
    global SYS syncon staton compon netkbpson netmax
    upvar #0 $cfgname cfg 

    set syncon 0
    set w $w.mainF

    set w1 $w.upperF
    set f [ $w1.syncF subwidget frame ]
    set f $f.padF
    $f.synconRB configure -variable ${cfgname}(SYNCMODE:) 
    $f.syncoffRB configure -variable ${cfgname}(SYNCMODE:) 

    $f.depthL configure -fg gray
    $f.timeoutL configure -fg gray
    $f.secsL configure -fg gray

    $f.depthE configure -bg lightgray -fg gray -state disabled \
	    -textvariable ${cfgname}(SYNCMODEDEPTH:)
    $f.timeoutE configure -fg gray  -bg lightgray -state disabled\
	    -textvariable ${cfgname}(SYNCMODETIMEOUT:)

    if { $cfg(SYNCMODE:) == "on" } {
        enableSyncParams $f
    } else {
        disableSyncParams $f
    }
    
    set f [ $w1.compF subwidget frame ]
    set f $f.padF
    set compon 1
    $f.componRB configure -variable ${cfgname}(COMPRESSION:)
    $f.compoffRB configure -variable ${cfgname}(COMPRESSION:)
    set f [ $w.statF subwidget frame ]
    set staton 1
    set f $f.padF
    $f.statonRB configure -variable ${cfgname}(LOGSTATS:)
    $f.statoffRB configure -variable ${cfgname}(LOGSTATS:)
    $f.intervalE configure -textvariable ${cfgname}(STATINTERVAL:)
    $f.maxsizeE configure -textvariable ${cfgname}(MAXSTATFILESIZE:)

    if { $cfg(LOGSTATS:) == "on" } {
        enableStatParams $f
    } else {
        disableStatParams $f
    }

    set f [ $w.netF subwidget frame ]
    set f $f.padF
    set netkbpson 0
  
    $f.netL configure -fg gray
    $f.kbpsL configure -fg gray
    $f.netE configure -fg gray -bg lightgrey -state disabled
    
    if {$cfg(NETMAXKBPS:) == -1} {
        set netmax 0
        set netkbpson 0
        disableNetParams $f
    } else {
        set netmax $cfg(NETMAXKBPS:)
        set netkbpson 1
        enableNetParams $f
    }

}
# -- tunable parameters
proc makeTunableMenu {w cfgname} {
    global SYS syncon staton compon netkbpson netmax
    upvar #0 $cfgname cfg 
    set syncon 0	
  
    frame $w.mainF
    pack $w.mainF 

    set w $w.mainF

    frame $w.upperF
    pack $w.upperF

    set w1 $w.upperF
    tixLabelFrame $w1.syncF -label "Synchronous Mode" -labelside acrosstop
    pack $w1.syncF -anchor nw  -side left

    set f [ $w1.syncF subwidget frame ]

    frame $f.padF 
    pack $f.padF -pady .25c -padx .25c

    set f $f.padF
    
    radiobutton $f.synconRB -text "On" -variable ${cfgname}(SYNCMODE:) -value on -command \
	    [list enableSyncParams $f]
    radiobutton $f.syncoffRB -text "Off" -variable ${cfgname}(SYNCMODE:) -value off  -command \
	    [list disableSyncParams $f]

    label $f.depthL -text "Depth: " -fg gray 
    label $f.timeoutL -text "Timeout:" -fg gray
    label $f.secsL -text "seconds" -fg gray

    entry $f.depthE -width 3 -bg lightgray -fg gray -state disabled \
	    -textvariable ${cfgname}(SYNCMODEDEPTH:)
    entry $f.timeoutE -width 3 -fg gray  -bg lightgray -state disabled\
	    -textvariable ${cfgname}(SYNCMODETIMEOUT:)
    
    grid $f.synconRB - $f.depthL $f.depthE -sticky w
    grid $f.syncoffRB - $f.timeoutL $f.timeoutE  $f.secsL -sticky w

    grid columnconfigure $f 1 -minsize 2c

    if { $cfg(SYNCMODE:) == "on" } {
        enableSyncParams $f
    } else {
        disableSyncParams $f
    }

    tixLabelFrame $w1.compF -label "Compression" -labelside acrosstop
    pack $w1.compF -side left -anchor nw

    set f [ $w1.compF subwidget frame ]
    
    frame $f.padF 
    pack $f.padF -pady .25c -padx .25c
    set f $f.padF

    set compon 1
    radiobutton $f.componRB -text "On" -variable ${cfgname}(COMPRESSION:) -value on
    radiobutton $f.compoffRB -text "Off" -variable ${cfgname}(COMPRESSION:) -value off

    grid $f.componRB -sticky w
    grid $f.compoffRB -sticky w

    tixLabelFrame $w.statF -label "Statistics Generation" -labelside acrosstop
    pack $w.statF -anchor nw -expand 1 -fill x
    set f [ $w.statF subwidget frame ]

    frame $f.padF 
    pack $f.padF -pady .25c -padx .25c
    
    set staton 1
    set f $f.padF
    radiobutton $f.statonRB -text "On" -variable ${cfgname}(LOGSTATS:) -value on -command \
	    [list enableStatParams $f]
    radiobutton $f.statoffRB -text "Off" -variable ${cfgname}(LOGSTATS:) -value off  -command \
	    [list disableStatParams $f]

    label $f.intervalL -text "Update Interval: " 
    label $f.secsL -text "seconds" 

    label $f.maxsizeL -text "Maximum Stat File Size:" 
    label $f.kbL -text "KB" 

    entry $f.intervalE -width 3 \
	    -textvariable ${cfgname}(STATINTERVAL:)
    entry $f.maxsizeE -width 3 \
	    -textvariable ${cfgname}(MAXSTATFILESIZE:)
    
    grid $f.intervalL $f.intervalE $f.secsL -sticky w
    grid $f.maxsizeL $f.maxsizeE  $f.kbL -sticky w

    grid columnconfigure $f 1 -minsize 2c


    enableStatParams $f
    
    tixLabelFrame $w.netF -label "Network Usage Threshold" -labelside acrosstop
    pack $w.netF -anchor nw -expand 1 -fill x
    set f [ $w.netF subwidget frame ]

    frame $f.padF 
    pack $f.padF -pady .25c -padx .25c
    
    set f $f.padF
    set netkbpson 0
    radiobutton $f.netkbpsonRB -text "On" -variable netkbpson -value 1\
            -command [list enableNetParams $f]
    radiobutton $f.netkbpsoffRB -text "Off" -variable netkbpson -value 0 \
            -command [list disableNetParams $f]

    label $f.netL -text "Maximum Transfer Rate:" -fg gray
    label $f.kbpsL -text "KBps" -fg grey
    entry $f.netE -width 5 -textvariable netmax -fg gray \
            -bg lightgrey -state disabled

    grid $f.netkbpsonRB - $f.netL $f.netE $f.kbpsL -sticky w
    grid $f.netkbpsoffRB - - - - -sticky w
    
    grid columnconfigure $f 1 -minsize 2c

    if {$cfg(NETMAXKBPS:) == -1} {
        set netmax 0
        set netkbpson 0
        disableNetParams $f
    } else {
        set netmax $cfg(NETMAXKBPS:)
        set netkbpson 1
        enableNetParams $f
    }
}

#-----------------------------------------------------------------------------
# end of procedures
#-----------------------------------------------------------------------------

wm withdraw .

Introduction 0
makeMainMenu
return
