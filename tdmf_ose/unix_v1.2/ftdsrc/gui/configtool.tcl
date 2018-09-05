#!/bin/sh
# next line will restart with wish but the exec line will be ignored by wish because of \
LC_ALL=C exec /%FTDTIXBINPATH%/%FTDTIXWISH% "$0" "$@"
#-----------------------------------------------------------------------------
# %Q%configtool -- %PRODUCTNAME% Configuration Tool
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
#-----------------------------------------------------------------------------
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

# -- define necessary enviroment variables
global env
set env(LANG) C
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

#
# Martin Proulx - I'm wondering if this is really useful or even needed at all.  If we have succesfully installed
#                 the product on a given machine, is it worth checking at this level if the os's version
#                 is supported or not?
#           
#
if { $SYS(platform) == "HP-UX"} {
    set hpOsVers [list "B.10.20" "B.10.30" "B.11.00" "B.11.11" "B.11.23" "B.11.31"]
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
    set SYS(confpath) "/etc/opt/%PKGNM%/"
    set SYS(conffile) "/etc/opt/%PKGNM%/%Q%.conf"
    set SYS(hostname) "/usr/bin/hostname"
    set SYS(services) "/etc/services"
    set SYS(defjournal) ""
    set SYS(mount) "/sbin/mount"
    set SYS(mount_opt) "-v"
    set SYS(mount_fsopt) "-F"
    set SYS(umount) "/sbin/umount"
    set SYS(modfs) "/opt/%PKGNM%/bin/%Q%modfs"
    set SYS(%Q%start) "/opt/%PKGNM%/bin/%Q%start"
    set SYS(fstab) "/etc/fstab"
    #option add *disabledForeground grey50

    # Enable the usage of the dynamically linked kernel module for the releases supporting it.
    # Tcl 8.0.4 didn't support the shorthand regexp syntaxex such as '\d{2}' to denote '[0-9][0-9]'.
    regexp {^B\.([0-9][0-9])\.([0-9][0-9])$} $SYS(osVersion) match SYS(osVersionMajor) SYS(osVersionMinor)

    if {$SYS(osVersionMajor) >= 11} {
        # Warning: Loadable modules supported since 11.11 according to http://en.wikipedia.org/wiki/HP-UX
        # Leaving the support enabled for all versions greater than 11 in order to remain compatible with the previous behavior.
        set dlkm  1
    }

} elseif {$SYS(platform) == "SunOS"} {
    set SYS(confpath) "/etc/opt/%PKGNM%/"
    set SYS(conffile) "/usr/kernel/drv/%Q%.conf"
    set SYS(hostname) "/bin/hostname"
    set SYS(services) "/etc/inet/services"
    set SYS(defjournal) ""
    set SYS(mount) "/sbin/mount"
    set SYS(mount_opt) "-p"
    set SYS(mount_fsopt) "-F"
    set SYS(umount) "/sbin/umount"
    set SYS(modfs) "/opt/%PKGNM%/bin/%Q%modfs"
    set SYS(%Q%start) "/opt/%PKGNM%/bin/%Q%start"
    set SYS(fstab) "/etc/vfstab"
} elseif {$SYS(platform) == "AIX"} {
    set SYS(confpath) "/etc/%Q%/lib/"
    set SYS(conffile) "/usr/lib/drivers/%Q%.conf"
    set SYS(hostname) "/bin/hostname"    
    set SYS(services) "/etc/services"
    set SYS(defjournal) ""
    set SYS(mount) "/usr/sbin/mount"
    set SYS(mount_opt) ""
    set SYS(mount_fsopt) "-v"
    set SYS(umount) "/usr/sbin/umount"
    set SYS(modfs) "/usr/%Q%/bin/%Q%modfs"
    set SYS(%Q%start) "/usr/%Q%/bin/%Q%start"
    set SYS(fstab) "/etc/filesystems"
} elseif {$SYS(platform) == "Linux"} {
    set SYS(confpath) "/etc/opt/%PKGNM%/"
    set SYS(conffile) "/%ETCOPTDIR%/%PKGNM%/driver/%Q%.conf"
    set SYS(hostname) "/bin/hostname"
    set SYS(services) "/etc/services"
    set SYS(defjournal) ""
    set SYS(mount) "/bin/mount"
    set SYS(mount_opt) "-v"
    set SYS(mount_fsopt) "-t"
    set SYS(umount) "/bin/umount"
    set SYS(modfs) "/opt/%PKGNM%/bin/%Q%modfs"
    set SYS(%Q%start) "/opt/%PKGNM%/bin/%Q%start"
    set SYS(fstab) "/etc/fstab"
}
    set SYS(maxlg) 999

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
set SYS(minbab) 1 
set SYS(maxbab) 1547

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
set defaulttunable(CHUNKSIZE:) 2048
set defaulttunable(CHUNKDELAY:) 0 
set defaulttunable(SYNCMODE:) off
set defaulttunable(SYNCMODEDEPTH:) 1
set defaulttunable(SYNCMODETIMEOUT:) 30
set defaulttunable(STATINTERVAL:) 10
# set default value of maxstatfilesize to 1024 Kb
set defaulttunable(MAXSTATFILESIZE:) 1024
set defaulttunable(TRACETHROTTLE:) off
set defaulttunable(NETMAXKBPS:) -1
set defaulttunable(COMPRESSION:) off
%ifbrand == tdmf
set defaulttunable(JOURNAL:) off
%endif
%ifbrand != tdmf
set defaulttunable(JOURNAL:) on
%endif
set defaulttunable(LRT:) on
set tunabletype(SYNCMODE:) ONOFF
set tunabletype(TRACETHROTTLE:) ONOFF
set tunabletype(COMPRESSION:) ONOFF
set tunabletype(JOURNAL:) ONOFF
set tunabletype(LRT:) ONOFF

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
	    --rmdtimeout -
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
	    --noconfigchk -
	    -noconfigchk {
		set CFGfilecheck 0
		incr i
	    }
            --notprimary -
            -notprimary {
                set SYS(onprimary) 0
                incr i
            }
	    --nohelp -
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
	    update
	    after 50 destroy $w
	    after 50
	}
	if {$oldGrab != ""} {
	    if {$grabStatus == "global"} {
		catch "grab -global $oldGrab"
	    } else {
                catch "grab $oldGrab"
	    }
	}
	update 
	update idletasks
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
    global SYS defaulttunable CFGfilecheck tunabletype defaultsys ipflag
    global proflag hostflag
    set havesystems 0 
    set proflag 0
     
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
    set cfg(SYSTEM-A,IP:) ""
    set cfg(SYSTEM-A,HOSTID:) ""
    set cfg(SYSTEM-A,PSTORE:) ""
	set cfg(DYNAMIC-ACTIVATION:) on
    set cfg(INITIAL-DYN-ACTIVATION:) $cfg(DYNAMIC-ACTIVATION:)
    set cfg(DYN-ACT-TURNING-OFF:) no
    set cfg(AUTOSTART:) on
    set cfg(SYSTEM-A,%Q%devs) ""
    set cfg(SYSTEM-B,HOST:) ""
    set cfg(SYSTEM-B,IP:) ""
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
            "AUTOSTART:" {set cfg(AUTOSTART:) [lindex $line 1]}
            "DYNAMIC-ACTIVATION:" {set cfg(DYNAMIC-ACTIVATION:) [lindex $line 1]}
	    "JOURNAL:" {set cfg(SYSTEM-B,JOURNAL:) [lindex $line 1]}
	    "PROFILE:" {
		if {$CFGfilecheck && (!$havesystems)} {
		    global RMDtimeout
		    set proflag 1

		    #ipflag is set to 4 if IPv4 and 6 if IPv6 
		    set ipflag 4
                    set tag "SYSTEM-A SYSTEM-B"
                    foreach cur $tag {
                        if {![regexp {\.} $cfg($cur,HOST:)]} {
	 		    if {[regexp {:} $cfg($cur,HOST:)]} {
				set hostip($cur) ""
				set hostflag 1
                            } elseif {$SYS(platform) == "SunOS"} {
				if {$cfg($cur,HOST:) != ""} {
                                    set hostip($cur) [get_host $cfg($cur,HOST:)]
                                    #-- checks the file /etc/inet/ipnodes if IPv4 not found 
				    #   in /etc/hosts
                                    if {$hostip($cur) == ""} {
                                        set hostip($cur) [get_host $cfg($cur,HOST:) "/etc/inet/ipnodes"]
                                    } 
				} else {
				    set hostip($cur) ""
				    set hostflag 0
				}
                            } elseif {$SYS(platform) == "HP-UX" || $SYS(platform) == "AIX" || $SYS(platform) == "Linux"} {
				if {$cfg($cur,HOST:) != ""} {
                                    set hostip($cur) [get_host $cfg($cur,HOST:)]
				} else {
				    set hostip($cur) ""
				    set hostflag 1
				}
                            }
			    set cfg($cur,IP:) $hostip($cur)
                            if {$hostip($cur) == ""} {
				set ipflag 6
                            }
			} else { 
			    set cfg($cur,IP:) $cfg($cur,HOST:)
			}
                    }

		    
		    # -- check if we have a devices list for this system
		    set sysaname $cfg(SYSTEM-A,IP:)
		    set sysbname $cfg(SYSTEM-B,IP:)
		    		 
		    if {$cfg(SYSTEM-A,HOST:) == ""} {
			lappend SYS(errors) "$cfgfile - Primary System definition missing"
		    }
		    if {$cfg(SYSTEM-B,HOST:) == ""} {
			lappend SYS(errors) "$cfgfile - Secondary System definition missing"
		    }

                    regsub -all {::} $sysaname :0: modsysaname
                    regsub -all {::} $sysbname :0: modsysbname


		    if {$ipflag != 4} {
			displayInfo "Processing Configuration File:  /%OURCFGDIR%/$cfgfile" OFF
			if {$hostflag == 0} {
			    displayError "Host Name Resolution Failed"
			} else {
	         	    displayError "Configuration Tool does not support IPv6 addressing."
                            set cfg(SYSTEM-A,PSTORE:) ""
                            set cfg(SYSTEM-B,JOURNAL:) "$SYS(defjournal)"
			}
		    } else {
		        if {[info globals devlist-$modsysaname] == "" && $sysaname != ""} {
			    displayInfo "Retrieving device information from $cfg(SYSTEM-A,HOST:)" ON
                            set isprim 1	
                            ftdmkdevlist $sysaname $SYS(primaryport) $RMDtimeout $isprim
                	    upvar #0 devlist-$modsysaname sysadev
			    displayInfo "" OFF
		            if {$sysadev(status) != 0} {
                                displayError $sysadev(errmsg)
	        	        lappend SYS(errors) "$cfgfile - Primary System: $sysadev(errmsg)"
		    	    }
		    	}
		        if {[info globals devlist-$modsysbname] == "" && $sysbname != ""} {
		  	    displayInfo "fetching device information from $cfg(SYSTEM-B,HOST:)" ON
                            set isprim 0 
			    ftdmkdevlist $sysbname $cfg(secport) $RMDtimeout $isprim
                            upvar #0 devlist-$modsysbname sysbdev
			    displayInfo "" OFF
			    if {$sysbdev(status) != 0} {
                                displayError $sysbdev(errmsg)
			        lappend SYS(errors) "$cfgfile - Secondary System: $sysbdev(errmsg)"
			    }
		    	}
		    }
		    set havesystems 1
		    upvar #0 devlist-$modsysaname sysadev
		    upvar #0 devlist-$modsysbname sysbdev
		}
		incr SYS(numprofile)
		global "profile$SYS(numprofile)"
		upvar #0 "profile$SYS(numprofile)" profile
		set profile(REMARK:) ""
		set profile(PRIMARY:) "SYSTEM-A"
		set profile(%CAPQ%-DEVICE:) ""
		set profile(DATA-DISK:) ""
		set profile(DATA-DEVNO:) ""
		
		set profile(SECONDARY:) "SYSTEM-B"
		set profile(MIRROR-DISK:) ""
		set profile(MIRROR-DEVNO:) ""
	    }
	    "REMARK:" {set profile(REMARK:) "[lrange $line 1 end]"}
	    "PRIMARY:" {set profile(PRIMARY:) [lindex $line 1]}
	    "SECONDARY:" {set profile(SECONDARY:) [lindex $line 1]}
	    "SECONDARY-PORT:" {set cfg(secport) [lindex $line 1]}
            "CHAINING:" { set cfg(CHAINING:) [lindex $line 1]}
	    "%CAPQ%-DEVICE:" {
                set %Q%dev [lindex [file split [lindex $line 1]] end]
                set profile(%CAPQ%-DEVICE:) $%Q%dev
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
            "DATA-DEVNO:" { set profile(DATA-DEVNO:) [lrange $line 1 end]}
	    "MIRROR-DISK:" {set profile(MIRROR-DISK:) [lindex $line 1]}
            "MIRROR-DEVNO:" { set profile(MIRROR-DEVNO:) [lrange $line 1 end]}
	    "THROTTLE*" {
		set getthrottle 1
                set cfg(throttletext) "$cfg(throttletext)$line\n"
	    }
        }
    }
    set cfg(INITIAL-DYN-ACTIVATION:) $cfg(DYNAMIC-ACTIVATION:)
   
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
    } else {
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
        }
    }

    if {$proflag != 0} {
    # ===== post-parsing validity checking =====
    if {![info exists sysaname]} {set sysaname ""}
    if {![info exists sysbname]} {set sysbname ""}
    upvar #0 devlist-$modsysaname sysadev
    upvar #0 devlist-$modsysbname sysbdev

    if {$CFGfilecheck} {
	displayInfo "Verifying Configuration for %CAPGROUPNAME% Group $SYS(candidatelg)" ON
    }
    # -- %Q% Device Definitions
    foreach p [info globals profile*] {
	set q ""
	if {![catch {lindex [file split [set ${p}(%CAPQ%-DEVICE:)]] end} q]} {
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
	global $q
	upvar #0 $q %Q%
	if {[info exists %Q%(%CAPQ%-DEVICE:)]} {
	    if {[string range $%Q%(%CAPQ%-DEVICE:) 0 2] != "%Q%" } {
		lappend SYS(errors) "$cfgfile - Invalid %Q% Device path specification \[$%Q%(%CAPQ%-DEVICE:)\]"
	    }
	}
	set %Q%dev [lindex [file split [lindex $%Q%(%CAPQ%-DEVICE:) 0]] end]
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
	ftddevadd $sysaname $%Q%(%CAPQ%-DEVICE:) $SYS(candidatelg) $%Q%dev FTDDEV 1
	ftddevadd $sysaname $%Q%(DATA-DISK:) $SYS(candidatelg) $%Q%dev LOCDATADEV 1
	
	ftddevadd $sysbname $%Q%(MIRROR-DISK:) $SYS(candidatelg) $%Q%dev MIRRORDEV 1
 
    }
    # Delay alittle for dialog box to show to the user 
    # TODO -- Should we avoid delaying if device processing is long enough.
    if {$CFGfilecheck} {
	after 1000
    }
    
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

    foreach g_old [info globals "OLD_%Q%*"] {
        if {$g_old != "OLD_%Q%devnewflag"} {
            global $g_old
            unset $g_old
        }
    }

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
#  get_host (hostname file}
#
#  gets the ip address or name for the hostname
#  The default value for file is /etc/hosts
#
#  Returns:   IPv4 address if present else blank
#-------------------------------------------------------------------------

proc get_host {hostname {file "/etc/hosts"}} {
    global hostflag
    set fd [open $file r]
    set buffer [read $fd]
    close $fd
    set lineno 0
    set hostflag 0
    foreach line [split $buffer "\n"] {
        incr lineno
        # -- discard comments and empty lines
        if {[string index $line 0] == "\#"} {continue}
        if {[regexp "^\[ \t]*$" $line]} {continue}
        if {[regexp "$hostname" $line]} {
	    set hostflag 1
            set host_ip_name [lindex $line 0]
            #--checks whether hostip contains IPv4 address
            if {[regexp {\.} $host_ip_name] && $host_ip_name != ""} {
                return $host_ip_name
            }
        }
    }
    #--if IPv6 returns blank
    return ""
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
    # variable old_t%Q%devmatch retains the previous value of t%Q%devmatch 
    # to determine the change in t%Q%devmatch	
    global t%Q%devmatch old_t%Q%devmatch flag
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
   
    if {$flag == 0} {
    	if {$old_t%Q%devmatch != $t%Q%devmatch} {
   	    set changed 1
    	}
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
#  updatemirdevno {cfgname}
#
#  Updates the MIRROR-DEVNO when options "MINOR NUMBER MATCH" is turned from
#  ON to OFF or vice versa
#
#  Returns:Nothing
#-------------------------------------------------------------------------
proc updatemirdevno { cfgname } {
    global t%Q%devmatch t%Q%mirdevno t%Q%datadevno t%Q%mirdev
    global SYS $cfgname
    upvar #0 $cfgname cfg

    if {[string length $SYS(cur%Q%)] <= 0} {
	set SYS(cur%Q%) [lindex $cfg(%Q%devs) 0]
    }
   
    upvar #0 $SYS(cur%Q%) %Q%
    set sysaname $cfg(SYSTEM-A,IP:)
    set sysbname $cfg(SYSTEM-B,IP:)
    regsub -all {::} $sysaname :0: modsysaname
    regsub -all {::} $sysbname :0: modsysbname
    global devlist-$modsysaname devlist-$modsysbname
    upvar #0 devlist-$modsysaname sysadev
    upvar #0 devlist-$modsysbname sysbdev

    if { $t%Q%devmatch } {
	if {[string length $t%Q%mirdev] > 0} {
            set minor [lindex $sysbdev($t%Q%mirdev..size) 6]
            set major [lindex $sysbdev($t%Q%mirdev..size) 5]
            set t%Q%mirdevno "$minor $major"
        }
    } else {
        set t%Q%datadevno ""
        set t%Q%mirdevno ""
    }
    set %Q%(MIRROR-DEVNO:) $t%Q%mirdevno
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
    set tmpcfgfile [format "p%03d" $lgnumber]
    global $tmpcfgfile
    upvar #0 $tmpcfgfile tmpcfg
    menu_off
    displayInfo "Saving %CAPGROUPNAME% Group $lgnumber...." ON
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
    # -- set GUI marker
    # -- to make sure dtcset will not mess up with p###.cfg & settunables#.tmp
    if {[catch "open /%OURCFGDIR%/settunables$lgnumber.gui w" fd]} {
	displayError "Could not open /%OURCFGDIR%/settunables$lgnumber.gui for writing"
	return
    }

    # -- create the config file
    if {[catch "open $configpath w" fd]} {
	displayError "Could not open $configpath for writing"
	menu_on
	return
    }
    puts $fd "#==============================================================="
    puts $fd "#  %PRODUCTNAME% Configuration File:  $configpath"
    puts $fd "#  %PRODUCTNAME% Version %VERSION%"
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
    if { $cfg(DYNAMIC-ACTIVATION:) == "on" } {
        puts $fd "  DYNAMIC-ACTIVATION:  on"
    } else {
        puts $fd "  DYNAMIC-ACTIVATION:  off"
    }
    if { $cfg(AUTOSTART:) == "off" } {
        puts $fd "  AUTOSTART:           off"
    } else {
        puts $fd "  AUTOSTART:           on"
    }
#   Upon writing to config file, new value replaces initial value
#   in case of subsequent saves of same config after another change
    set cfg(INITIAL-DYN-ACTIVATION:) $cfg(DYNAMIC-ACTIVATION:)

    puts $fd "#\n# Secondary System Definition:\n#"
    puts $fd [format "SYSTEM-TAG:          %-15s           %-15s" $cfg(rtag) SECONDARY]
    set p $cfg(rtag)
    if {[llength $cfg($p,HOST:)] > 0} {
	puts $fd "  HOST:                $cfg($p,HOST:)"
    }

    if {[llength $cfg($p,JOURNAL:)] > 0} {
	puts $fd "  JOURNAL:             $cfg($p,JOURNAL:)"

        foreach element [array names defaulttunable] {
            set tunable [string range $element 0 \
                    [expr [string length $element] - 2]]

#ifdef UPDATE_P_FILE
#            if {${tunable} == "JOURNAL"} {
#                puts $fd "# ${tunable} $cfg($element)"
#            }
#endif
        }

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
	    if {$SYS(platform) == "Linux"} {
	        puts $fd "  %CAPQ%-DEVICE:  /dev/%Q%/lg$lgnumber/dsk/$%Q%(%CAPQ%-DEVICE:)"
	    } else {
	        puts $fd "  %CAPQ%-DEVICE:  /dev/%Q%/lg$lgnumber/rdsk/$%Q%(%CAPQ%-DEVICE:)"
	    }
            puts $fd "  DATA-DISK:        $%Q%(DATA-DISK:)"
	    if {$SYS(platform) == "AIX"} {
		if {0 < [string length $%Q%(DATA-DEVNO:)]} {
		    puts $fd "  DATA-DEVNO:       $%Q%(DATA-DEVNO:)"
		}
	    }
	   
	    puts $fd "  SECONDARY:        $%Q%(SECONDARY:)"
	    puts $fd "  MIRROR-DISK:      $%Q%(MIRROR-DISK:)"
	    if {$SYS(platform) == "AIX"} {
		if {0 < [string length $%Q%(MIRROR-DEVNO:)]} {
		    puts $fd "  MIRROR-DEVNO:     $%Q%(MIRROR-DEVNO:)"
		}
	    }
	    puts $fd "#"
	}
    }
    puts $fd "#\n# -- End of %Q% Configuration File:  $configpath\n"
    flush $fd
    close $fd
    catch "exec /bin/rm ${configpath}.1"
    # catch "write%Q%conf"
    
    catch "exec /bin/rm /%OURCFGDIR%/settunables$lgnumber.tmp"
    set fileopen 0
    foreach element [array names defaulttunable] {
        # -- remove trailing : from tunable name for %Q%set
        set tunable [string range $element 0 \
                [expr [string length $element] - 2]]
        if {!$SYS(onprimary) || [catch "exec /opt/SFTKdtc/bin/dtcset -q -g $lgnumber ${tunable}=$cfg($element) >& /dev/null
" ret]} {
            if {$fileopen == 0} {
                if {[catch "open  /%OURCFGDIR%/settunables$lgnumber.tmp w" fd]} {
                    displayError "Could not open /%OURCFGDIR%/settunables$lgnumber.tmp for writing"
                    return
                } else {
                    puts $fd "#!/bin/sh"
                }
                set fileopen 1
            }
            puts $fd "/%FTDBINDIR%/%Q%set -q -g $lgnumber ${tunable}=$cfg($element)"
        } else {
            # -- add error message for JOURNAL setting
            if {${tunable} == "JOURNAL"} {
	        displayError \
                    "${tunable}=$cfg($element) will take effect after group restart"
            }
        }
    }
    if {$fd > 0} {
    	catch "close $fd"
    	catch "exec chmod 744 /%OURCFGDIR%/settunables$lgnumber.tmp"
    }
    catch "exec /bin/rm /%OURCFGDIR%/settunables$lgnumber.gui"
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
#  replaceRTM {}
#
#  Returns: replace UNIX* registered trademark with a graphical version
#
#-------------------------------------------------------------------------

proc replaceRTM {oldString} {
    regsub -all {UNIX\*} $oldString "UNIX\xAE" result 
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
 
    $w.file.m add command -label "New %CAPGROUPNAME% Group" \
            -command "SaveQuery newLogGroupSetNum"
    $w.file.m add command -label "Select %CAPGROUPNAME% Group" \
            -command "SaveQuery selLogGroup"
    $w.file.m add command -label "Reset %CAPGROUPNAME% Group" \
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
    $w.view.m add command -label [replaceRTM "About %PRODUCTNAME%..."] \
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

    wm title . [replaceRTM "%PRODUCTNAME% Configuration Tool"]
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
#        wm title .top [replaceRTM "%PRODUCTNAME% License Agreement"]
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
    wm title .top [replaceRTM "%PRODUCTNAME% Configuration Tool"]
    wm transient .top [winfo toplevel .]
    set t .top
    frame $t.topF
    frame $t.botF -relief groove -bd 2
    pack $t.topF -side top
    pack $t.botF -side bottom -fill x -expand 1

    set f $t.topF
    set In_Intro 0
    label $f.explainL -text [replaceRTM "Welcome to the %PRODUCTNAME% Configuration Tool.  As a first time configuration step you will be asked to specify the amount of RAM that should be reserved for use by the BAB.  After this initialization is successful, you will not be prompted for this information the next time you invoke %Q%configtool."] -justify left -wraplength 5i  
    
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
        if { $backmsg != "" } {
            exec mv /stand/vmunix /stand/vmunix.pre%CAPQ% > /dev/null
        }
        displayInfo "Building new kernel... (takes 1-2 minutes)" on
    } elseif { $SYS(platform) == "AIX" } {
	#
	# Make sure that odmadd has happened
	#
	if {[catch "exec /usr/sbin/lsdev -P -c %Q%_class 2>/dev/null" retstr]} {
	    set ret [%Q%_dialog .errorT "Error" [replaceRTM "Cannot initialize BAB; %CAPQ% driver is not included in the Predefined Devices object class.  This may be a result of an incomplete or failed installation.  You should remove and re-install %PRODUCTNAME% to resolve this problem."] 4i warning 0 Exit]
	    exit
	}
	#
	# Make sure that def%Q% has happened
	#
	if {[catch "exec /usr/sbin/lsdev -C -c %Q%_class 2>/dev/null" retstr]} {
	    set ret [%Q%_dialog .errorT "Error" [replaceRTM "Cannot initialize BAB; %CAPQ% driver is not included in the Customized Devices object class.  This may be a result of an incomplete or failed installation.  You should remove and re-install %PRODUCTNAME% to resolve this problem."] 4i warning 0 Exit]
	    exit
	} elseif {[lindex $retstr 1] == "Available"} { 
	    #
	    # Driver has already attached
	    #
	    set ret [%Q%_dialog .errorT "Error" "The %CAPQ% Driver has already been attached, you must first stop all %GROUPNAME% groups, then unattach the driver using the following command:\n     /usr/lib/methods/ucfg%Q% -l %Q%0" 4i warning 0 Exit]
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
        
        if { !$dlkm } {
            # We cannot dynamically load the kernel module.  We can only ask the user to reboot and terminate here.
            set ret [%Q%_dialog .workedT "Kernel Build Successful" "The kernel was rebuilt sucessfully with the %CAPQ% driver included.  You must reboot the system for the changes to take effect." 4i info 0 Ok ]
            exit
        } else {
            # Dynamically load the module, using the proper tools for the HP-UX version in use.
            set failed 0
	    displayInfo "Loading Driver..." on
	    displayInfo "Configuring Driver..." on

            # Make sure that the module is loaded.
            # 11.23 needs to load the driver prior to setting driver parameters.
            # What about other versions between 11.11 and 11.23?
            if { $dlkm && $SYS(osVersionMajor) >= 11 && $SYS(osVersionMinor) >= 23 } {

                #/usr/sbin/kcmodule %Q%=unused ## not needed ?
		# kcmodule return 2 on ERROR case.

                # Should we also take care of handling return codes of 1 in order to detect that we need to reboot
                # in order to apply the changes?

                if {[catch "exec /usr/sbin/kcmodule %Q%=best " result ] == 2} {
                        puts stderr $result
                    displayError\
                        "kcmodule failed.  You will have to reload the %Q% module manually."

                    set failed 1
                    exit
                }
            }

            # Set module parameters.
            if { $SYS(osVersionMajor) >= 11 && $SYS(osVersionMinor) >= 23 } {
                # Rely on kctune.

                # Should we also take care of handling return codes of 1 in order to detect that we need to reboot
                # in order to apply the changes?
                if {[catch "exec /usr/sbin/kctune ftd_num_chunk=$SYS(bab)" result ] == 2 } {
	        puts stderr $result
	        displayError\
	            "kctune failed.  You will have to kctune the %Q% module manually."
                set failed 1
	        exit

              } 
            } else {
                # Rely on kmtune.
	      if {[catch "exec /usr/sbin/kmtune -s ftd_num_chunk=$SYS(bab)" result ] } {
	        puts stderr $result
	        displayError\
	            "kmtune failed.  You will have to kmtune the %Q% module manually."
                set failed 1
	        exit
              }
            }

            if { $dlkm && $SYS(osVersionMajor) >= 11 && $SYS(osVersionMinor) >= 23 } {
                # <<< mksf should work on 11.31; need to debug <<<
                # For some reason /etc/mksf does not work in 11.23 ?? NOTE: it could work but requires the HP patch PHCO_34780 (pc,071222)
                # For 11.23: use mknod instead !

                # Is it possible that mksf works, but that the problem is the missing kmadmin in 11.23?

		catch "exec /usr/bin/mkdir -p /dev/%Q% "
#		catch "exec /usr/sbin/mknod /dev/%Q%/ctl c `/usr/sbin/lsdev -h -d %Q% | /usr/bin/cut -c 1-15 | /usr/bin/head -n 1` 0xffffff" 

                catch "exec /usr/sbin/lsdev -h -d %Q% > /tmp/%Q%1.tmp"
                catch "exec /usr/bin/cut -c 1-15 /tmp/%Q%1.tmp > /tmp/%Q%2.tmp"
                set major [exec /usr/bin/head -n 1 /tmp/%Q%2.tmp ]

                catch "exec /usr/sbin/mknod /dev/%Q%/ctl c $major 0xffffff"
		catch "exec /usr/bin/chmod 644 /dev/%Q%/ctl"
		catch "exec /usr/bin/chown bin /dev/%Q%/ctl"
		catch "exec /usr/bin/chgrp bin /dev/%Q%/ctl"
                displayInfo "The new %Q% module loaded with Dev major=$major"

            } elseif {$dlkm} {
		displayInfo "update kernel..." on
		catch "exec /etc/mksf -d %Q%  -m 0xffffff -r /dev/%Q%/ctl"

	        if {[catch "exec /usr/sbin/kmadmin -L %Q% " result ] } { 
	             puts stderr $result
	             displayError\
	                 "kmadmin failed.  You will have to reload the %Q% module manually."
                     set failed 1
	             exit
	        }
                displayInfo "The new %Q% module loaded"
		if [ file exists /etc/loadmods ] {
	             # add to /etc/loadmods
		     catch "exec echo \"%Q%\" >> /etc/loadmods"
		} else {
		     # create /etc/loadmods file
		     catch "exec /usr/bin/echo \"%Q%\" > /etc/loadmods"
		}

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
    } elseif { $SYS(platform) == "Linux" } {
        #
        # Now add driver
        #
        displayInfo "Loading Driver..." on
        set failed 0
        catch "exec /%ETCINITDDIR%/%PKGNM%-scan start reconfig"
        displayInfo ""  off
        catch "exec /%FTDBINDIR%/launch%Q%master >& /dev/null"
    }
    if {$failed} {
	displayError [replaceRTM "An error occurred during an attempt to load the driver.  You may try a manual configuration of the BAB using the method documented in the %PRODUCTNAME% Administrator's Guide."]
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
    if { [info exists reqamt] } {
	    if {$reqamt != $actualamt} {
		displayInfo "The driver was loaded successfully but was only able to obtain $actualamt MB of RAM for use by the BAB ($reqamt MB requested).  You should reboot the system following configuration to acquire the requested amount of memory."
	    } else {
		displayInfo "Driver loaded successfully."
	    }
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
    if {$SYS(bab) == $PRE(bab)} {
        return
    }

    # WR16973
    if {[glob -nocomplain "$SYS(confpath)p\[0-9\]\[0-9\]\[0-9\].cur"] != ""} {
	    set ret [%Q%_dialog .warnT "Warning" "Cannot initialize BAB; Some %GROUPNAME% groups are running now. You should retry after stopping all %GROUPNAME% groups." 4i warning 0 Ok]
        return
    }
    
    # DTurrin - Removed un-necessary OS Version check on August 16th, 2001
    if { $SYS(platform) == "HP-UX" } {
	#
	# Warn about the driver reload
	#
	if {$dlkm} {
          set ret [%Q%_dialog .workedT "Warning" "For this change to take effect, the driver will be reloaded.   Before proceeding, shut down any processes that may be accessing %CAPQ% devices, unmount any file systems that reside on %CAPQ% devices, and stop all %GROUPNAME% groups using %Q%stop.  All %CAPQ% daemons will be temporarily shut down while the reload is taking place." 4i warning 0 Ok Cancel]
	 if {$ret == 1} { return }
         catch "exec /%FTDBINDIR%/kill%Q%master >& /dev/null"
	}
    }

    if { $SYS(platform) == "AIX" } {
	#
	# Make sure that odmadd has happened
	#
	if {[catch "exec /usr/sbin/lsdev -P -c %Q%_class 2>/dev/null" retstr]} {
	    set ret [%Q%_dialog .errorT "Error" [replaceRTM "Cannot initialize BAB; %CAPQ% driver is not included in the Predefined Devices object class.  This may be a result of an incomplete or failed installation.  You should remove and re-install %PRODUCTNAME% to resolve this problem."] 4i warning 0 Exit]
	    exit
	}
	#
	# Make sure that def%Q% has happened
	#
	if {[catch "exec /usr/sbin/lsdev -C -c %Q%_class 2>/dev/null" cfgstr]} {
	    set ret [%Q%_dialog .errorT "Error" [replaceRTM "Cannot initialize BAB; %CAPQ% driver is not included in the Customized Devices object class.  This may be a result of an incomplete or failed installation.  You should remove and re-install %PRODUCTNAME% to resolve this problem."] 4i warning 0 Exit]
	    exit
	}
    } elseif {$SYS(platform) == "HP-UX" && !$dlkm} {
          if [file exists /stand/vmunix] {
            set backmsg  "  /stand/vmunix will be renamed to /stand/vmunix.%CAPQ%bak before the rebuild occurs so that you may restore it at a later time if necessary."
            exec mv /stand/vmunix /stand/vmunix.%CAPQ%bak > /dev/null
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
    if { !$dlkm } {
        catch "exec /%FTDBINDIR%/kill%Q%master >& /dev/null"
    }
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
    } elseif {$SYS(platform) == "Linux"} {
        if {[catch "exec /sbin/rmmod  %MODULENAME%" out]} {
                displayInfo ""  off
            displayError "Driver remove failed. $out"
            set failed 1
        } else {
           if [catch "exec /%ETCINITDDIR%/%PKGNM%-scan start reconfig >& /dev/null" out] {
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
            displayError "Errors occurred during an attempt to reload the driver.  This is most likely because some %CAPQ% devices are in use.  You will need to rectify the problem, then manually detach the driver by issueing /usr/lib/methods/ucfg%Q% -l %Q%0 after stopping master daemon.  And, manually attach the driver by issueing /usr/lib/methods/cfg%Q% -l %Q%0 -2, then restart master daemon."
        } elseif {$SYS(platform) == "SunOS"} {
            displayError "Errors occurred during an attempt to reload the driver.  This is most likely because some %CAPQ% devices are in use.  You will need to rectify the problem, then manually reload the driver using the rem_drv and add_drv commands"
        } elseif { $SYS(platform) == "Linux" } {
        displayError "Errors  occured during an attempt to reload the driver. This is most likely because some %CAPQ% devices are in use. You will need to rectify the problem, then manually reload the driver using rmmod and insmod commands."
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

    set In_Intro 1
    
    toplevel .introT
    
    if {!$aboutFlag} {
	wm overrideredirect .introT 1
	set bdwidth 6
    } else {
	wm title .introT "About"
	set bdwidth 0
    }

    .introT configure -bg white

    frame .introT.borderF -relief ridge -bd $bdwidth -bg white 
    pack .introT.borderF -expand yes -fill both 

    set w .introT.borderF

    catch {image create photo -file $env(PROD_LIBRARY)/%CAPQ%logo202.gif} t
   
    label $w.%Q%image -image $t -bg white 
    pack $w.%Q%image -side top -pady 0 -padx 0
    
    set msg [replaceRTM "%PRODUCTNAME% V%VERSION%\nLicensed Materials / Property of IBM, \xA9 Copyright %COMPANYNAME%\n%COPYRIGHTYEAR 2001%. All rights reserved.  IBM, Softek, and %PRODUCTNAME%\nare registered or common law trademarks of %COMPANYNAME% in the\nUnited States, other countries, or both."]
    message $w.msg2 -text $msg                  -aspect 10000 -justify left -font -Adobe-Helvetica-Bold-R-Normal--*-100-*-*-*-*-*-* -bg white
    pack $w.msg2 -side left -padx 3

    if {$aboutFlag} {
	button .introT.okB -text "Cancel" -command {set done 1}
	pack .introT.okB -pady .25c 
    }
    

    center_window .introT
    if {$aboutFlag} {
        set intro_w [winfo reqwidth .introT]
        set intro_h [winfo reqheight .introT]
        wm maxsize .introT $intro_w $intro_h
        wm geom .introT "${intro_w}x${intro_h}"
    }
    update
    if {!$aboutFlag} {
        after 5000 { set introdone 1}
        if {!$introdone} {
            tkwait variable introdone
            destroy .introT
	    readconfigs
        }
        if {$SYS(onprimary) } {
            if {$SYS(needinit) } {
                init_bab_and_ps
            }
        }
    } else {
	tkwait variable done
        update
        after 50 destroy .introT
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
    set highval $SYS(maxbab)
    for {set i 1} {$i <= $SYS(maxbab)} { set i [ expr $i * 2 ] } {
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
    for {set i 1} {$i <= 1547} { set i [ expr $i * 2 ] } {
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
	    -allowempty false -autorepeat false -variable PRE(bab) -min 1 \
	    -max 1547 -incrcmd doubleBAB -decrcmd halveBAB \
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
    set isprim 1 
    ftdmkdevlist $hostname $SYS(primaryport) $RMDtimeout $isprim
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
	    -listcmd "%Q%filldevcombobox $f.recstor 1 dsk 1" \
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

            # cp -p $SYS(services) to $SYS(services).%Q%new
            # allow the ownership and protection to be preseved
            catch {exec cp -p $SYS(services) $SYS(services).%Q%new}

            # - open up a new file and write the new contents
            # "w" option should retain the ownership and protection
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
            if {![catch {exec cp -p $SYS(services) $SYS(services).pre%Q%}]} {
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
   
    wm title .configerrT [replaceRTM "%PRODUCTNAME% Configuration Errors"]
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
        update
        after 50 destroy .configerrT
     }
}    
    
#-----------------------------------------------------------------------------
proc newLogGroup {{lgnum ""}} {
    global SYS defaulttunable defaultsys ipflag proflag hostflag
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
    if {$SYS(platform) != "Linux"} {
    	set cfg(SYSTEM-A,IP:) [get_host $hostname]
    	if {$SYS(platform) == "SunOS" && $cfg(SYSTEM-A,IP:) == ""} {
	    set cfg(SYSTEM-A,IP:) [get_host $hostname "/etc/inet/ipnodes"]
    	}
    } else {
	set cfg(SYSTEM-A,IP:) $hostname
    }
    set cfg(SYSTEM-A,HOSTID:) ""
    set cfg(SYSTEM-A,PSTORE:) ""
    # -- Dynamic Activation is ON by default at group creation
	set cfg(DYNAMIC-ACTIVATION:) on
    set cfg(INITIAL-DYN-ACTIVATION:) $cfg(DYNAMIC-ACTIVATION:)
    set cfg(DYN-ACT-TURNING-OFF:) no
    set cfg(AUTOSTART:) on
    set cfg(SYSTEM-A,%Q%devs) ""
    set cfg(SYSTEM-B,HOST:) ""
    set cfg(SYSTEM-B,IP:) ""
    set cfg(SYSTEM-B,HOSTID:) ""
    set cfg(SYSTEM-B,JOURNAL:) "$SYS(defjournal)"
    set cfg(SYSTEM-B,%Q%devs) ""
    set cfg(secport) $defaultsys(tcpport)
    set cfg(CHAINING:) off
    set xxxcur%Q% ""
    set proflag 1
    #setting ipflag as 4 by default
    set ipflag 4
    #checking for IPv6 or non existence of hostname in /etc/hosts
    if {$cfg(SYSTEM-A,IP:) == "" && $SYS(platform) != "Linux"} {
	if {$hostflag == 0} {
	    displayError "Host Name Resolution Failed"
	} else {
            #if yes displaying following msg and exiting
            displayError "Configuration Tool does not support IPv6 addressing."
        }
    }
    # -- set tunables to their defaults
    array set cfg [array get defaulttunable]

    # -- remove all global device variables
    foreach g [info globals "%Q%*"] {
        global $g
        unset $g
    }
    wm title . [replaceRTM "%PRODUCTNAME% Configuration Tool"]
    update
    topNotebook $SYS(candidatecfg)
    
    update%Q%devs [$w.nb subwidget %Q%_Devices] $SYS(candidatecfg)
    updatedevicefields $w $SYS(candidatecfg) none
    menu_on
    .bottomF.lgL configure -text " Currently editing %GROUPNAME% group $SYS(candidatelg)" -fg black
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

    wm title .newLogT "New %CAPQ% %CAPGROUPNAME% Group"
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
    
    tixControl $w.lgnum -label "\n%CAPGROUPNAME% Group Number: " -integer true \
	    -allowempty false -autorepeat false -variable xxxloggrp -min 0 \
	    -max $SYS(maxlg) -value $lastlg \
	    -options {
	entry.width 10
	label.width 25
	label.anchor c
    }
    pack $w.lgnum
    $SYS(help) bind $w.lgnum -balloonmsg \
	    "enter new %CAPGROUPNAME% Group's number to be defined" 
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
	displayError "%CAPGROUPNAME% Group $lgnum exists, select another"
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
	displayError "No %CAPGROUPNAME% Groups are currently defined"
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
    
    label .sel.msgL -text "Select a %CAPGROUPNAME% Group:"
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
    .bottomF.lgL configure -text " Currently editing %GROUPNAME% group $SYS(candidatelg)" -fg black
    set w .f1
    set SYS(candidatecfg) [format "p%03d" $SYS(candidatelg)]
    wm title . [replaceRTM "%PRODUCTNAME% Configuration Tool -- %CAPGROUPNAME% Group $SYS(candidatelg)"]
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
    global SYS flag
    menu_off
  
    set w .f1
    wm title . [replaceRTM "%PRODUCTNAME% Configuration Tool -- Saving Changes"]

    if {[DoSave] == 1 } { 
        # Save successful, so we need to update our 'original' variables so
        # we can detect changes on an exit
        saveorigcfg $SYS(candidatecfg)
	set flag 1
    }
    menu_off
    update
    after 1000 
    wm title . [replaceRTM "%PRODUCTNAME% Configuration Tool -- %CAPGROUPNAME% Group $SYS(candidatelg)"]
 
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
	displayError "No %CAPGROUPNAME% Groups currently defined"
	menu_on
	return
    }   
    if {$SYS(candidatelg) == ""} {
	displayError "You have not selected a %CAPGROUPNAME% Group to reset"
	menu_on
	return
    }

   
    set w .f1
    
    if {$SYS(candidatelg) < 0} {
	displayError "You have not selected a %CAPGROUPNAME% Group to reset"
	menu_on
	return
    }
    displayInfo "Resetting %CAPGROUPNAME% Group $SYS(candidatelg) from file"
    if {-1 != [lsearch [getloggrps] $SYS(candidatelg)]} {
	readconfig $SYS(candidatelg)
        # - save off cfg variables so we know whether we need to save changes
        #   later
        saveorigcfg $SYS(candidatecfg)
    } else {
	newLogGroup
    }
    wm title . [replaceRTM "%PRODUCTNAME% Configuration Tool"]
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
	   #"%CAPGROUPNAME% Group $SYS(candidatelg) is actively being edited"
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
proc isrunninglg {cfgfile} {
    global SYS
    if {$SYS(platform) == "HP-UX" || $SYS(platform) == "SunOS" || $SYS(platform) == "Linux"} {
        set curpath "/etc/opt/%PKGNM%/${cfgfile}.cur"
    } elseif {$SYS(platform) == "AIX"} {
        set curpath "/etc/%Q%/lib/${cfgfile}.cur"
    }
    if {[file exists "$curpath"]} {
        return 1
    }
    return 0
}


proc getlogbyopt {opt} {
    set opts [split $opt ","]
    set logdev ""
    set optstr ""
    set optlist {}
    foreach op $opts {
        if {[regsub "^log=" $op "" logdev]} {
            set logdev $logdev
        } else {
            lappend optlist $op
        }
    }
    if {[llength $optlist] != 0} {
        set optstr [join $optlist ","]
    }
    return [list $logdev $optstr]
}

#
# Returned list: {{mount point, device, log-device}, ...}
#
proc getmountinfo {} {
    global SYS
    set badfs {"nfs" "proc" "swap" "tmpfs" "mntfs"}
    
    set mtab {}
    if {[catch "exec $SYS(mount) $SYS(mount_opt)" mdata]} {
        # error
        return $mtab
    }
    set mntlines [split $mdata "\n"]
    
    foreach ln $mntlines {
        set mntlist [split $ln]
        if {$SYS(platform) == "AIX"} {
            if { [regexp \
                  {^ +(\/[^ ]+) +(\/[^ ]*) +([^ ]+) +[^ ]+ +[^ ]+ +[^ ]+ +(.*)$} \
                  $ln dummy device mpoint fs opt]} {
                set opt [string trim $opt]
                set aixopt [getlogbyopt $opt]
                set log [lindex $aixopt 0]
                set opt [lindex $aixopt 1]
                lappend mtab [list $mpoint $device $log $fs $opt]
            }
        } elseif {$SYS(platform) == "SunOS"} {
            set mpoint [lindex $mntlist 2]
            set device [lindex $mntlist 0]
            set log ""
            set fs [lindex $mntlist 3]
            set opt [lindex $mntlist 6]
            if {[regexp "^\/" $device] &&
                [lsearch -exact $badfs $fs] == -1} {
                set opt_index [string first ",dev=" $opt]
                if {$opt_index != -1} {
                    set opt_index [expr $opt_index - 1]
                    set opt [string range $opt 0 $opt_index]
                }
                lappend mtab [list $mpoint $device $log $fs $opt]
            }
        } elseif {$SYS(platform) == "HP-UX"} {
            # HP-UX
            if {[lindex $mntlist 1] == "on"} {
                set mpoint [lindex $mntlist 2]
                set device [lindex $mntlist 0]
                set log ""
                set fs [lindex $mntlist 4]
                set opt [lindex $mntlist 5]
                if {![regexp "^.+:\/.*$" $device] &&
                    [regexp "^\/.*$" $device] &&
                    $fs != "nfs"} {
                    lappend mtab [list $mpoint $device $log $fs $opt]
                }
            }
        } else {
            # Linux
            if {[lindex $mntlist 1] == "on"} {
                set mpoint [lindex $mntlist 2]
                set device [lindex $mntlist 0]
                set log ""
                set fs [lindex $mntlist 4]
                set opt [lindex $mntlist 5]
                regsub -all {^\(} $opt {} tmpopt
                set opt $tmpopt
                regsub -all {\)$} $opt {} tmpopt
                set opt $tmpopt
                lappend mtab [list $mpoint $device $log $fs $opt]
            }
        }
    }
    return $mtab
}


proc rawdev2block {raw} {
    global SYS
    if {$SYS(platform) == "Linux"} {
        return $raw
    }
    if {[regsub "/rdsk/" $raw "/dsk/" bdev]} {
        return $bdev
    }
    set dirs [split $raw "/"]
    set nameindex [expr [llength $dirs] - 1]
    set rawname [lindex $dirs $nameindex]
    if {! [regsub "^r" $rawname "" bname]} {
        set bname $rawname
    }
    set dirs [lreplace $dirs $nameindex $nameindex $bname]
    return [join $dirs "/"]
}

#
# Returned list: {{mnt-point, device, logdev, %Q%}, ...}
# NOTE: device and data-dev is same value
#
proc checkdatamounted {%Q%s} {
    set devlist [getmountinfo]
    set mounted_datadev_list {}
    set ma(DUMMY) ""
    foreach g $%Q%s {
        upvar #0 $g %Q%
        set rdev $%Q%(DATA-DISK:)
        set bdev [rawdev2block $rdev]
        set i 0
        foreach mnt $devlist {
            set mntdev [lindex $mnt 1]
            set logdev [lindex $mnt 2]
            if {$mntdev == $bdev} {
                set ma($i.data) $g
            }
            if {$logdev == $bdev} {
                set ma($i.log) $g
            }
            incr i
        }
    }

    foreach g_old [info globals "OLD_%Q%*"] {
        if {$g_old != "OLD_%Q%devnewflag"} {
            %Q%_dialog .workedT "Infomation" "Because some %Q% devices have already existed in this %GROUPNAME% group, the %Q%configtool did not execute neither \"umount data devices\", \"mount %Q% devices\", \"start %GROUPNAME% group\", nor \"change automatic mount definition\".\nPlease operate these by yourself." 4i info 0 Ok

            set mounted_datadev_list {}
            return $mounted_datadev_list
        }
    }

    set i 0
    foreach mnt $devlist {
        set dkey "$i.data"
        set lkey "$i.log"
        set datadev [array get ma $dkey]
        set logdev [array get ma $lkey]
        if {[llength $datadev] != 0} {
            lappend mnt [lindex $datadev 1]
            if {[llength $logdev] != 0} {
                lappend mnt [lindex $logdev 1]
            } else {
                # When only the data device exists
                if {[lindex $mnt 3] == "jfs" || \
                   ([lindex $mnt 3] == "jfs2" && [lindex $mnt 1] != [lindex $mnt 2])} {
                    # File system is jfs or
                    # jfs2 File system and data device and log device are different
                    set ssw 0
                    foreach g [info globals "%Q%*"] {
                        upvar #0 $g g%Q%
                        if {[info exists g%Q%(DATA-DISK:)] == 0} {
                            continue
                        }
                        set bdev [rawdev2block $g%Q%(DATA-DISK:)]
                        if {[lindex $mnt 2] == $bdev} {
                            set ssw 1
                            lappend mnt $g
                            break
                        }
                    }
                    if {$ssw == 0} {

                        %Q%_dialog .workedT "Warning" "You should add both of \"MetaData\" and \"JournalLog\" which composesfilesystem to a %GROUPNAME% group. \nPlease add insufficient \"MetaData\" or \"JournalLog\" to a %GROUPNAME% groupbefore starting a %GROUPNAME% group." 4i warning 0 Ok

                        set mounted_datadev_list {}
                        return $mounted_datadev_list
                    }
                } else {
                    lappend mnt [lindex $mnt 2]
                }
            }
            lappend mounted_datadev_list $mnt
        } elseif {[llength $logdev] != 0} {
            # When only the log device exists
            if {[lindex $mnt 3] == "jfs" || \
               ([lindex $mnt 3] == "jfs2" && [lindex $mnt 1] != [lindex $mnt 2])} {
                # File system is jfs or
                # jfs2 File system and data device and log device are different
                set ssw 0
                foreach g [info globals "%Q%*"] {
                    upvar #0 $g g%Q%
                    if {[info exists g%Q%(DATA-DISK:)] == 0} {
                        continue
                    }
                    set bdev [rawdev2block $g%Q%(DATA-DISK:)]
                    if {[lindex $mnt 1] == $bdev} {
                        set ssw 1
                        lappend mnt $g
                        break
                    }
                }
                if {$ssw == 0} {

                    %Q%_dialog .workedT "Warning" "You should add both of \"MetaData\" and \"JournalLog\" which composesfilesystem to a %GROUPNAME% group. \nPlease add insufficient \"MetaData\" or \"JournalLog\" to a %GROUPNAME% groupbefore starting a %GROUPNAME% group." 4i warning 0 Ok

                    set mounted_datadev_list {}
                    return $mounted_datadev_list
                }
            } else {
                lappend mnt [lindex $mnt 1]
            }
            lappend mnt [lindex $logdev 1]
            lappend mounted_datadev_list $mnt
        }
        incr i
    }
    return $mounted_datadev_list
}


proc check%Q%added {cfgname} {

    global $cfgname OLD_$cfgname OLD_SYS SYS THROTCHANGE netkbpson netmax
    upvar #0 $cfgname cfg
    upvar #0 OLD_$cfgname oldcfg

    array set %Q%array {}
    # - loop through %Q% device settings and see if they've changed
    # Note: if we just transitionned from Dynamic Activation ON to OFF, set all dtc devs
    #       flags to changed so that their mounting status be verifed.
    foreach g [info globals "%Q%*"] {
        global $g OLD_$g
        upvar #0 $g %Q%
        upvar #0 OLD_$g old%Q%
        foreach element [array names %Q%] { 
            if {! [info exists old%Q%($element) ] || ($cfg(DYN-ACT-TURNING-OFF:) == "yes")} {
                set %Q%array($g) 1
            }
        }
    }
    return [array names %Q%array]
}


proc checkmounted {lgnumber} {
    global SYS
    set cfgfile [format "p%03d" $lgnumber]
    set mntlist {}
    # check logical group running
    if {[isrunninglg $cfgfile]} {
        return $mntlist
    }

    # check added %Q%s
    set added_%Q% [check%Q%added $cfgfile]
    if {[llength $added_%Q%] == 0} {
        return $mntlist
    }

    # check mounted data devices    
    set mntlist [checkdatamounted $added_%Q%]
    if {[llength $mntlist] == 0} {
        return $mntlist
    }

    # Confirm unmount
    set umounts {}
    foreach m $mntlist {
        set um [lindex $m 0]
        lappend umounts $um;
    }
    set umount_msg [join $umounts "\n"]
    set ret [%Q%_dialog .workedT "Unmount/mount required" "Do you want to unmount now?\n$umount_msg" 4i question 0 Yes "No"]
    if {$ret == 1} {
        # No auto mount
        return $mntlist
    }

    # Do modfs (umount)
    set newmntlist {}
    set umount_fail {}
    set wcnt 0
    while 1 {
        foreach m $umounts {
            set um [lindex $m 0]
            if {[catch "exec $SYS(umount) $um" msg]} {
                # Unable to unmount
                lappend umount_fail $um
            }
        }
        if {[llength $umount_fail]} {
            set dialogName ".warnT_$wcnt"
            set umount_msg [join $umount_fail "\n"]
            set ret [%Q%_dialog $dialogName "Unmount failed" "Unable unmount\n$umount_msg Retry ?" 4i question 0 Yes No]
            update
            if {$ret != 0} {
                # No. Not retry
                break
            }
            set umounts $umount_fail
            set umount_fail {}
            incr wcnt
        } else {
            break;
        }
    }
    # Check umounted directories
    foreach m $mntlist {
        set um [lindex $m 0]
        set unmounted 1
        foreach uf $umount_fail {
            if {$um == $uf} {
                set unmounted 0
                break
            }
        }
        lappend m $unmounted
        lappend newmntlist $m
    }
    return $newmntlist
}


proc updatefstab {lgnumber} {
    global SYS

    if {[catch "exec $SYS(modfs) -c -g $lgnumber" amntbuf]} {
        # %Q%modfs failed
        displayError "Unable to execute $SYS(modfs).\n$amntbuf"
        return
    }
    if {![regexp "needs to be updated" $amntbuf]} {
        # Not need updating
        return
    }
    # Need updating. confirm it
    set ret [%Q%_dialog .workedT "$SYS(fstab) update" "$SYS(fstab) file should be updated. Do you want to update it now?" 4i question 0 "Yes" "No"]
    if {$ret == 1} {
        # No
        return
    }
    # Yes
    if {[catch "exec $SYS(modfs) -g $lgnumber" amntbuf]} {
        # %Q%modfs failed
        displayError "Unable to execute $SYS(modfs).\n$amntbuf"
        return
    }
    return
}


proc start_and_mount {lgnumber arg_mntlist} {
    global SYS
    # Check modfs point
    set mntlist {}
    foreach m $arg_mntlist {
        if {[llength $m] >= 8} {
            if {[lindex $m 7] == 1} {            
                lappend mntlist $m
            }
        }
    }
    if {[llength $mntlist] == 0} {
        # Not need auto mount
        return
    }
    # Start logical group
    if {[catch "exec $SYS(%Q%start) -g $lgnumber >& /dev/null" ret]} {
        displayError "$SYS(%Q%start) -g $lgnumber failed.\n$ret"
        return
    }
    # Mount data devices
    set errors {}
    foreach m $mntlist {
        set mntpoint [lindex $m 0]
        set fs [lindex $m 3]
        set opt [lindex $m 4]
        set g [lindex $m 5]
        upvar #0 $g %Q%
        if {[regexp "^%Q%" $g]} {
            set bdev "/dev/%Q%/lg$lgnumber/dsk/$%Q%(%CAPQ%-DEVICE:)"
        } else {
            set bdev $g
        }
        set log [lindex $m 6]
        if {$log != ""} {
            upvar #0 $log log%Q%
            if {[regexp "^%Q%" $log]} {
                set blog "/dev/%Q%/lg$lgnumber/dsk/$log%Q%(%CAPQ%-DEVICE:)"
            } else {
                set blog $log
            }
            set logopt "log=$blog"
            if {$opt == ""} {
                set opt $logopt
            } else {
                set opt "$opt,$logopt"
            }
        }
        if {$fs == "vxfs"} {
		if { $SYS(platform) == "Linux" } {
	            set mntcmd "$SYS(mount) -t vxfs -o $opt $bdev $mntpoint"
		} elseif { $SYS(platform) == "AIX" } {
                    set mntcmd "$SYS(mount) $mntpoint"
                } else {
		    set mntcmd "$SYS(mount) $SYS(mount_fsopt) vxfs -o $opt $bdev $mntpoint"
		}
	    } else {
	        set mntcmd "$SYS(mount) $SYS(mount_fsopt) $fs -o $opt $bdev $mntpoint"
	    }
        if {[catch "exec $mntcmd" ret]} {
            lappend errors $ret
        }
    }
    if {[llength $errors] != 0} {
        set errmsg [join $errors "\n"]
        displayError "Mount failed.\n$errmsg"
    }
    return
}


proc domodfs {lgnumber mntlist} {
    global SYS
    updatefstab $lgnumber
    start_and_mount $lgnumber $mntlist
}


proc saveconfig {lgnumber} {
    set mntlist [checkmounted $lgnumber]
    writeconfig $lgnumber
    if {[llength $mntlist] != 0} {
        domodfs $lgnumber $mntlist
    }
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
	
	label $w.save.msgL -text "You have not saved your changes\nto this %GROUPNAME% group.\n\nWould you like to save them now?" -justify left
	pack $w.save.msgL -in $w.save.topF -side right -padx 1c -pady 1c
	label $w.save.bitmapL -bitmap warning 
	pack $w.save.bitmapL -in $w.save.topF -side left -padx .5c 
	
	# message $w.msg -aspect 10000 -justify center -text \
		#"%CAPGROUPNAME% Group $SYS(candidatelg) is actively being edited"
	#pack $w.msg -side top 
	
	frame $w.save.botF -relief groove -bd 2
	pack $w.save.botF -expand 1 -fill both
	
	frame $w.save.buttonF 
	pack $w.save.buttonF -in $w.save.botF -expand 1 -fill y
	button $w.save.saveB -text "Yes" -command "destroy $w.save; updatemirdevno $SYS(candidatecfg); writeconfig $SYS(candidatelg); eval $args"
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

#   Set flag to indicate if we are turning off Dynamic Activation; in this case we must perform
#   the verifications and eventual manipulations on mounted devices and mount table files.
    if { ($cfg(DYNAMIC-ACTIVATION:) == "off") && ($cfg(INITIAL-DYN-ACTIVATION:) == "on") } {
       set cfg(DYN-ACT-TURNING-OFF:) yes
    } else {
       set cfg(DYN-ACT-TURNING-OFF:) no
    }
    if {$SYS(onprimary)} {
        writetcpinfo
    }
    if {$cfg(SYSTEM-A,HOST:) == ""} {
        displayError "You must specify a Primary Host area."
        return 0
    }
    if {$cfg(SYSTEM-B,JOURNAL:) == ""} {
        displayError "You must specify a Secondary Journal area."
        return 0
    }

    # -- Check for mounted native devices only if Dynamic Action is OFF
    if { $cfg(DYNAMIC-ACTIVATION:) == "off" } {
       set mntlist [checkmounted $SYS(candidatelg)]
    } else {
       set mntlist {}
    }

    updatemirdevno $SYS(candidatecfg)
    writeconfig $SYS(candidatelg)
    if {[llength $mntlist] != 0} {
        domodfs $SYS(candidatelg) $mntlist
    }
    return 1
}
#-----------------------------------------------------------------------------
proc %Q%raise%Q%devpage {w cfgname} {
    global RMDtimeout SYS xxxcur%Q% ipflag hostflag
    upvar #0 $cfgname cfg

    #ipflag is set to 4 if IPv4 and 6 if IPv6
    set ipflag 4
    set tag "SYSTEM-A SYSTEM-B"
    foreach cur $tag {
	if {![regexp {\.} $cfg($cur,HOST:)]} {
            if {[regexp {:} $cfg($cur,HOST:)]} {
                set hostip($cur) ""
		set hostflag 1
            } elseif {$SYS(platform) == "SunOS"} {
		if {$cfg($cur,HOST:) != ""} {
                    set hostip($cur) [get_host $cfg($cur,HOST:)]
                    #-- checks the file /etc/inet/ipnodes if IPv4 not found
                    #   in /etc/hosts
                    if {$hostip($cur) == ""} {
                        set hostip($cur) [get_host $cfg($cur,HOST:) "/etc/inet/ipnodes"]
                    }
		} else {
		    set hostip($cur) ""
		    set hostflag 0
		}
            } elseif {$SYS(platform) == "HP-UX" || $SYS(platform) == "AIX"  || $SYS(platform) == "Linux"} {
		if {$cfg($cur,HOST:) != ""} {
                    set hostip($cur) [get_host $cfg($cur,HOST:)]
		} else {
		    set hostip($cur) ""
                    set hostflag 0
		}
            }
            set cfg($cur,IP:) $hostip($cur)
            if {$hostip($cur) == ""} {
                set ipflag 6
            }
        } else {
            set cfg($cur,IP:) $cfg($cur,HOST:)
        }
    }

    # -- check if we have a devices list for this system
    set sysaname $cfg(SYSTEM-A,IP:)
    set sysbname $cfg(SYSTEM-B,IP:)

    if {$cfg(SYSTEM-A,HOST:) == ""} {
	displayError "You must define a Primary System first."
	$w.nb raise systems
	return
	
    }
    if {$cfg(SYSTEM-B,HOST:) == ""} {
	displayError "You must define a Secondary System first."
	$w.nb raise systems
	return
    }

    if {$ipflag != 4} {
	if {$hostflag == 0} {
	    displayError "Host Name Resolution Failed"
	} else {
            displayError "Configuration Tool does not support IPv6 addressing."
	}
        $w.nb raise systems
        return
   }
regsub -all {::} $sysaname :0: modsysaname
regsub -all {::} $sysbname :0: modsysbname
    if {[info globals devlist-$modsysaname] == ""} {
	displayInfo "Retrieving device information from $cfg(SYSTEM-A,HOST:)" ON
        set isprim 1  
	ftdmkdevlist $sysaname $SYS(primaryport) $RMDtimeout $isprim
        after 2000
        upvar #0 devlist-$modsysaname sysadev
        displayInfo "" OFF
        if {$sysadev(status) != 0} {
            displayError $sysadev(errmsg)
        }
    }
    if {[info globals devlist-$modsysbname] == ""} {
	displayInfo "Retrieving device information from $cfg(SYSTEM-B,HOST:)" ON
        set isprim 0 
	ftdmkdevlist $sysbname $SYS(primaryport) $RMDtimeout $isprim
	after 2000
        upvar #0 devlist-$modsysbname sysbdev
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
    set f $f.padF
    $f.autoboot_yesB configure -text Yes -variable ${cfgname}(AUTOSTART:) -value on
    $f.autoboot_noB  configure -text No -variable ${cfgname}(AUTOSTART:) -value off
    $f.dyn_actyesB configure -text Yes -variable ${cfgname}(DYNAMIC-ACTIVATION:)\
            -value on
    $f.dyn_actnoB  configure -text No -variable ${cfgname}(DYNAMIC-ACTIVATION:) -value off

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
    global SYS curband cfg RMDtimeout proflag hostflag
    upvar #0 $cfgname cfg
   
    if {$proflag == 0} {
	if {$SYS(platform) != "Linux" && ![regexp {\.} $cfg(SYSTEM-A,HOST:)]} {
            if {[regexp {:} $cfg(SYSTEM-A,HOST:)]} {
                set hostip ""
		set hostflag 1
            } elseif {$SYS(platform) == "SunOS"} {
		if {$cfg(SYSTEM-A,HOST:) != ""} {
                    set hostip [get_host $cfg(SYSTEM-A,HOST:)]
                    #-- checks the file /etc/inet/ipnodes if IPv4 not found
                    #   in /etc/hosts
                    if {$hostip == ""} {
                        set hostip [get_host $cfg(SYSTEM-A,HOST:) "/etc/inet/ipnodes"]
                    }
		} else {
		    set hostip ""
		    set hostflag 0
		}
            } elseif {$SYS(platform) == "HP-UX" || $SYS(platform) == "AIX"} {
		if {$cfg(SYSTEM-A,HOST:) != ""} {
                      set hostip [get_host $cfg($cur,HOST:)]
		} else {
		    set hostip ""
	  	    set hostflag 0
		}
            }
            set cfg(SYSTEM-A,IP:) $hostip
            if {$hostip == ""} {
		if {$hostflag == 0} {
		    displayError "Host Name Resolution Failed"
		} else {
                    displayError "Configuration Tool does not support IPv6 addressing."
		}
            }
        } else {
            set cfg(SYSTEM-A,IP:) $cfg(SYSTEM-A,HOST:)
        }
    }
	
    set hostname $cfg(SYSTEM-A,IP:)
regsub -all {::} $hostname :0: modhostname
    if {[info globals devlist-$modhostname] == "" && $hostname != ""} {
	displayInfo "Retrieving device information from $hostname" ON
        set isprim 1	
        ftdmkdevlist $hostname $SYS(primaryport) $RMDtimeout $isprim 
	after 2000
        upvar #0 devlist-$modhostname sysadev
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
    entry $f.hostE -width 30 -bg white -textvariable ${cfgname}(SYSTEM-A,HOST:)
 
    tixComboBox $f.pstoreCombo -label "Persistent Store Device:  " \
	    -dropdown true \
	    -listcmd "%Q%filldevcombobox $f.pstoreCombo 1 dsk 1" \
	    -editable true -variable ${cfgname}(SYSTEM-A,PSTORE:)\
            -validatecmd "valdevpath" \
	    -options {
	entry.width 27
	entry.background white
	
	label.anchor w
	listbox.height 8
	listbox.width 27
	listbox.anchor e
    }

    grid $f.hostL $f.hostE - -
    grid $f.pstoreCombo
    grid $f.hostL $f.pstoreCombo -sticky w
    grid $f.hostE -sticky w -columnspan 3
    grid $f.pstoreCombo -sticky w -columnspan 3

    # Buttons for Autostart on Boot
    label $f.autobootL -text "Autostart:  "
    radiobutton $f.autoboot_yesB -text Yes -variable ${cfgname}(AUTOSTART:) -value on
    radiobutton $f.autoboot_noB -text No -variable ${cfgname}(AUTOSTART:) -value off
    grid $f.autobootL $f.autoboot_yesB $f.autoboot_noB
    grid $f.autobootL -sticky e
    grid $f.autoboot_yesB -sticky w
    grid $f.autoboot_noB -sticky w
    
    # Buttons for Dynamic Activation:
    label $f.dyn_actL -text "Dynamic Activation:  "
    radiobutton $f.dyn_actyesB -text Yes -variable ${cfgname}(DYNAMIC-ACTIVATION:) -value on
    radiobutton $f.dyn_actnoB -text No -variable ${cfgname}(DYNAMIC-ACTIVATION:) -value off
    grid $f.dyn_actL $f.dyn_actyesB $f.dyn_actnoB
    grid $f.dyn_actL -sticky e
    grid $f.dyn_actyesB -sticky w
    grid $f.dyn_actnoB -sticky w

    tixLabelFrame $w.secF -label "Secondary System" -labelside acrosstop
    pack $w.secF -anchor n  -fill x -expand 1
  
    set f  [ $w.secF subwidget frame]
   
    frame $f.padF
    pack $f.padF -pady .25c -padx .5c -anchor w
    
    set f $f.padF
    
    label $f.hostL -text "Hostname or IP Address: " 
    entry $f.hostE -width 30 -bg white -textvariable ${cfgname}(SYSTEM-B,HOST:)

    grid $f.hostL $f.hostE - -
    grid $f.hostL -sticky e
    grid $f.hostE -sticky w -columnspan 3
   
    label $f.journalL -text "Journal Directory: "
    entry $f.journalE -width 30 -bg white -textvariable ${cfgname}(SYSTEM-B,JOURNAL:)
 
    grid $f.journalL $f.journalE - -
    grid $f.journalL -sticky e
    grid $f.journalE -sticky w 

    label $f.portL -text "Port: "
    entry $f.portE -width 3 -bg white -textvariable ${cfgname}(secport)

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
    entry $f.noteE -width 50 -bg white -textvariable ${cfgname}(NOTES:)
    pack $f.noteE -pady .5c -padx .5c

}

    
#-----------------------------------------------------------------------------
proc %Q%fetchdevlist {cfgname} {
    global SYS RMDtimeout
    upvar #0 $cfgname cfg 
   
    foreach tag {SYSTEM-A SYSTEM-B} {
	# -- check if we have a devices list for this system
	set sysname $cfg($tag,IP:)
	if {$sysname == ""} {
	    displayError "You must specify Host Name or IP Address first."
	    continue
	}
regsub -all {::} $sysname :0: modsysname
	displayInfo "Retrieving device information from $cfg($tag,HOST:)" ON
	if {$tag == "SYSTEM-A"} {
        set isprim 1  	    
        ftdmkdevlist $sysname $SYS(primaryport) $RMDtimeout $isprim 
	} else  { 
        set isprim 0 
        ftdmkdevlist $sysname $cfg(secport) $RMDtimeout $isprim 
        }
	displayInfo "" OFF
	upvar #0 devlist-$modsysname devs
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
    global t%Q%devmatch t%Q%mirdevno t%Q%datadevno

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
    set t%Q%devmatch on
    set t%Q%datadevno ""
    set t%Q%mirdevno ""
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
    global t%Q%devmatch t%Q%mirdevno t%Q%datadevno

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
    set t%Q%devmatch on
    set t%Q%datadevno ""
    set t%Q%mirdevno ""
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
        entry.background white
        label.width 11
        label.anchor w
    }
    pack $f.%Q%f.f -side top -anchor w -padx 7 -pady 3 -fill x -expand 1
    pack $f.%Q%f.a  -in $f.%Q%f.f -side left
    button $f.%Q%f.refreshB -text "Refresh Device Lists" -command "%Q%fetchdevlist $cfgname" 
    pack $f.%Q%f.refreshB -pady 3 -side right -in $f.%Q%f.f
    $SYS(help) bind $f.%Q%f.a -balloonmsg \
	    "The device name for the %CAPQ% device"
    
    if {$SYS(platform) == "AIX"} {
	label $f.%Q%f.devmatchL -text "Match Minor Numbers:   "
	radiobutton $f.%Q%f.devmatchonRB -text "On" -variable t%Q%devmatch -value on
	radiobutton $f.%Q%f.devmatchoffRB -text "Off" -variable t%Q%devmatch -value off 
	pack $f.%Q%f.devmatchL $f.%Q%f.devmatchonRB $f.%Q%f.devmatchoffRB -side left -anchor w 
	$SYS(help) bind $f.%Q%f.devmatchonRB  -balloonmsg \
	    "OPTIONAL:  Enable matching the %CAPQ% minor device number with the target device"
	$SYS(help) bind $f.%Q%f.devmatchoffRB  -balloonmsg \
	    "OPTIONAL:  Disable matching the %CAPQ% minor device number with the target device"
    }
    tixLabelEntry $f.remark -label "Remark:  " \
	    -options "
    entry.width 40
    entry.background white
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
	entry.background white
	label.anchor w
	listbox.height 8
	listbox.width 30
	listbox.anchor e
    }
    pack $pframe.datadev -side top
    
    if {$SYS(platform) == "HP-UX"} {
        $SYS(help) bind $pframe.datadev -balloonmsg \
		"special character device path (e.g.  /dev/rdsk/c1t2d0) or select"
    } elseif {$SYS(platform) == "AIX"} {
        $SYS(help) bind $pframe.datadev -balloonmsg \
		"special character device path (e.g.  /dev/hd0) or select"
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
	entry.background white
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
	"Define a new %CAPQ% device for this %CAPGROUPNAME% Group"
 
    button $f.f.b1 -text "Commit Device" -command "commit%Q%dev $w $cfgname 0" 
    pack $f.f.b1 -side left -padx .25c 
    $SYS(help) bind $f.f.b1 -balloonmsg \
	"Commit the definition of this %CAPQ% device"
  
    button $f.f.b2 -text "Delete Device" -command "del%Q%dev $w $cfgname"  
    pack $f.f.b2 -side left -padx .25c 
    $SYS(help) bind $f.f.b2 -balloonmsg \
	"Delete the displayed %CAPQ% device from the %CAPGROUPNAME% Group"
  
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
    set sysname $cfg($tag,IP:)
   
regsub -all {::} $sysname :0: modsysname
    if {$sysname != ""} {
	upvar #0 devlist-$modsysname devs
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
		    (![info exists %Q%(%CAPQ%-DEVICE:)]) || \
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
            if {[info exists %Q%(%CAPQ%-DEVICE:) ] } {
                catch "$f insert end $%Q%(%CAPQ%-DEVICE:)"
            }
	}
    } 
       
   # update
}
#-----------------------------------------------------------------------------
proc updatedevicefields {w cfgname %Q%name} {
    global SYS $%Q%name $cfgname 
    global t%Q%remark t%Q%path t%Q%datadev t%Q%mirdev 
    global t%Q%devmatch t%Q%datadevno t%Q%mirdevn old_t%Q%devmatch flag
    global t%Q%start t%Q%end 
    set flag 0


    set t%Q%remark ""
    set t%Q%datadev ""
    set t%Q%devmatch on    
    set t%Q%datadevno ""
    set t%Q%mirdevno ""
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
    if {[info exists ${%Q%name}(%CAPQ%-DEVICE:)]} { set t%Q%dev $%Q%(%CAPQ%-DEVICE:)}
    if {[info exists ${%Q%name}(%CAPQ%-DEVICE:)]} { set t%Q%dev $%Q%(%CAPQ%-DEVICE:)}

    if {[info exists ${%Q%name}(REMARK:)]} {set t%Q%remark $%Q%(REMARK:)}
    if {[info exists ${%Q%name}(DATA-DISK:)]} {set t%Q%datadev $%Q%(DATA-DISK:)}
    if {[info exists ${%Q%name}(DATA-DEVNO:)] && [string length $%Q%(DATA-DEVNO:)] == 0} {
	set t%Q%devmatch off
    } 
    if {[info exists ${%Q%name}(DATA-DEVNO:)] && [string length $%Q%(DATA-DEVNO:)] > 0} {
        set t%Q%datadevno $%Q%(DATA-DEVNO:)
	set t%Q%devmatch on
	}
    if {[info exists ${%Q%name}(MIRROR-DISK:)]} {set t%Q%mirdev $%Q%(MIRROR-DISK:)}
    if {[info exists ${%Q%name}(MIRROR-DEVNO:)] && [string length $%Q%(MIRROR-DEVNO:)] == 0} {
	set t%Q%devmatch off
    } 
    if {[info exists ${%Q%name}(MIRROR-DEVNO:)] && [string length $%Q%(MIRROR-DEVNO:)] > 0} {
	set t%Q%mirdevno $%Q%(MIRROR-DEVNO:)
	set t%Q%devmatch on
    }

    if {$t%Q%devmatch} {
        set old_t%Q%devmatch on
    } else {
	set old_t%Q%devmatch off
    }
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
    global t%Q%devmatch t%Q%datadevno t%Q%mirdevno
    global t%Q%start t%Q%end 
    global prev%Q%devwl %Q%devnewflag %Q%devprev xxxcur%Q%
    upvar #0 $cfgname cfg 
   
    set SYS(cur%Q%) $xxxcur%Q%
    set %Q%dev $xxxcur%Q%
 
    # -- check if we have a devices list for this system
    set sysaname $cfg(SYSTEM-A,IP:)
    set sysbname $cfg(SYSTEM-B,IP:)
   
regsub -all {::} $sysaname :0: modsysaname
regsub -all {::} $sysbname :0: modsysbname
    global devlist-$modsysaname devlist-$modsysbname
    upvar #0 devlist-$modsysaname sysadev
    upvar #0 devlist-$modsysbname sysbdev
  
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

    if {$t%Q%devmatch} {
	if {[string length $t%Q%datadevno] == 0} {
	    set minor [lindex $sysbdev($t%Q%mirdev..size) 6]
	    set major [lindex $sysbdev($t%Q%mirdev..size) 5]
	    set t%Q%mirdevno "$minor $major"
	}
    } else {
	set t%Q%datadevno ""
	set t%Q%mirdevno ""
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
	
        if { ( ($sysadev(status) == -4) && ($cfg(DYNAMIC-ACTIVATION:) == "on") ) } {
           # -- Case: source device is mounted but Dynamic Activation is ON: accept and continue.
        } else {
	   if {[%Q%_dialog .errorD Error  $sysadev(errmsg) 4i warning 0\
	   	Cancel "Force Commit"] == 0 } {
	       return
	   }
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

    set %Q%(%CAPQ%-DEVICE:) $SYS(cur%Q%)
    set %Q%(PRIMARY:) $cfg(ptag)
    set %Q%(SECONDARY:) $cfg(rtag)
    set %Q%(REMARK:) $t%Q%remark
    set %Q%(DATA-DISK:) $t%Q%datadev
    set %Q%(DATA-DEVNO:) $t%Q%datadevno
    set %Q%(MIRROR-DISK:) $t%Q%mirdev
    set %Q%(MIRROR-DEVNO:) $t%Q%mirdevno
  
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
    set sysaname $cfg(SYSTEM-A,IP:)
    set sysbname $cfg(SYSTEM-B,IP:)
regsub -all {::} $sysaname :0: modsysaname
regsub -all {::} $sysbname :0: modsysbname
    global devlist-$modsysaname devlist-$modsysbname
    upvar #0 devlist-$modsysaname sysadev
    upvar #0 devlist-$modsysbname sysbdev
    catch "unset $tovar"
    array set %Q%to [array get %Q%from]
    global $%Q%dev
    upvar #0 $%Q%dev %Q%

    if {[info exists %Q%from(%CAPQ%-DEVICE:)]} {
	ftddevdel $sysaname $%Q%from(%CAPQ%-DEVICE:) \
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
    set sysaname $cfg(SYSTEM-A,IP:)
    set sysbname $cfg(SYSTEM-B,IP:)
regsub -all {::} $sysaname :0: modsysaname
regsub -all {::} $sysbname :0: modsysbname
    upvar #0 devlist-$modsysaname sysadev
    upvar #0 devlist-$modsysbname sysbdev
    catch "unset $tovar"
    array set %Q%to [array get %Q%from]
    if {[info exists %Q%from(%CAPQ%-DEVICE:)]} {
	ftddevadd $sysaname $%Q%from(%CAPQ%-DEVICE:) \
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

###########################################################################
#
# %Q%filldevcombobox --
#
#    Fills a given combobox according tp devices currently held in one of the
#    devlist-xxx global arrays.
#
# Arguments:
#
#    f            A tixComboBox widget which will be filled with the available devices.
#
#    pflag        Primary flag. Set to 1 if we are filling devices for the primary system
#                 0 for the secondary system.
#                 Optionnal, defaults to 1.
#
#    type         dsk/rsk  Exact usage TBD.
#                 Optionnal, defaults to rdsk. 
#
#    pstoreflag   Persistent store flag.  Set to 1 if we are filling devices for the persistent store,
#                 0 otherwise
#                 Optionnal, defaults to 0.
#
#    ahost        Used to determine from which system the available devices are obtained.
#                 "" has a special meaning that, probably means to rely on the current configs to obtain the system name.
#                 Optionnal, defaults to "".
#
# See also:
#
#    ftdmkdevlist in confutil.tcl which builds the devlist-xxx array.
#
###########################################################################

proc %Q%filldevcombobox {f {pflag 1} {type rdsk} {pstoreflag 0} {ahost ""} } {
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
        set sysname $cfg($tag,IP:)
    } else {
        set sysname $ahost
    }
    regsub -all {::} $sysname :0: modsysname
    global devlist-$modsysname
    upvar #0 devlist-$modsysname devs
    set lb [$f subwidget listbox]
    $lb delete 0 end
    $f configure -state disabled
    if {[info exists devs(names)]} {
	foreach n $devs(names) {
	    if {[info exists devs($n..listentry)]} {
                set tempentry $devs($n..listentry)
                if {(!$pstoreflag) && $pflag && \
                        $cfg(SYSTEM-A,PSTORE:) != "" } {
                    regsub "/dsk/" $cfg(SYSTEM-A,PSTORE:) "/rdsk/" pstorerdsk
                    if { [string match "$pstorerdsk*" $devs($n..listentry)] == 1 } {
                        if { [string length $pstorerdsk] == [string first \  $devs($n..listentry) ] } {
                           regsub "AVAIL"  $tempentry "INUSE" tempentry
                           regsub "SECT]" $tempentry "SECT (pstore)]" tempentry
                        }
                    }
                }
                if {($SYS(platform) == "Linux") || ($type=="rdsk")} {
                    $lb insert end $tempentry
                } else {
		    regsub "/rdsk/" $tempentry "/dsk/" x
		    if { $tempentry == $x } {
			set devpath [lindex $tempentry 0]
			set dir [file dirname $devpath]
			set file [file tail $devpath]
                        set len [string length $devpath]
		        regsub r(.*) $file {\1} file
			set x "[file join $dir $file][string range $tempentry $len end]"
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
    set sysaname $cfg(SYSTEM-A,IP:)
    set sysbname $cfg(SYSTEM-B,IP:)
   
    if {[info exists %Q%(%CAPQ%-DEVICE:)] && $%Q%(%CAPQ%-DEVICE:) != ""} {
	ftddevdel $sysaname $%Q%(%CAPQ%-DEVICE:) $SYS(candidatelg) $SYS(cur%Q%) FTDDEV
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
    global SYS syncon staton compon netkbpson netmax jrnon
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
#PB++
    enableStatParams $f
#    if { $cfg(LOGSTATS:) == "on" } {
#        enableStatParams $f
#    } else {
#        disableStatParams $f
#    }
#PB--
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

    set f [ $w.jrnF subwidget frame ]
    set f $f.padF
%ifbrand == tdmf
    set jrnon 0
%endif
%ifbrand != tdmf
    set jrnon 1
%endif
    $f.jrnonRB configure -variable ${cfgname}(JOURNAL:)
    $f.jrnoffRB configure -variable ${cfgname}(JOURNAL:)
    set f [ $w1.lrtF subwidget frame ]
    set f $f.padF
    set lrtoff 0
    set lrton 0
    $f.lrtonRB configure -variable ${cfgname}(LRT:)
    $f.lrtoffRB configure -variable ${cfgname}(LRT:)

}
# -- tunable parameters
proc makeTunableMenu {w cfgname} {
    global SYS syncon staton compon netkbpson netmax jrnon
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

    label $f.maxsizeL -text "Maximum Stat File Size: " 
    label $f.kbL -text "KB" 

    entry $f.intervalE -width 6 \
	    -textvariable ${cfgname}(STATINTERVAL:) -bg white 
    entry $f.maxsizeE -width 6 \
	    -textvariable ${cfgname}(MAXSTATFILESIZE:) -bg white 
    
    grid $f.intervalL $f.intervalE $f.secsL
    grid $f.intervalL -sticky e
    grid $f.intervalE -sticky w
    grid $f.secsL -sticky w
    grid $f.maxsizeL $f.maxsizeE $f.kbL
    grid $f.maxsizeL -sticky e
    grid $f.maxsizeE -sticky w
    grid $f.kbL -sticky w

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

    tixLabelFrame $w.jrnF -label "Journal" -labelside acrosstop
    pack $w.jrnF -anchor nw -expand 1 -fill x

    set f [ $w.jrnF subwidget frame ]
    
    frame $f.padF 
    pack $f.padF -pady .25c -padx .25c
    set f $f.padF

%ifbrand == tdmf
    set jrnon 0
%endif
%ifbrand != tdmf
    set jrnon 1
%endif
    radiobutton $f.jrnonRB -text "On" -variable ${cfgname}(JOURNAL:) -value on
    radiobutton $f.jrnoffRB -text "Off" -variable ${cfgname}(JOURNAL:) -value off

    grid $f.jrnonRB $f.jrnoffRB -sticky w

    tixLabelFrame $w1.lrtF -label "LRT Mode" -labelside acrosstop
    pack $w1.lrtF -side right -anchor nw
    set f [ $w1.lrtF subwidget frame ]

    frame $f.padF
    pack $f.padF -pady .25c -padx .25c
    set f $f.padF   

    set lrton 1
    radiobutton $f.lrtonRB -text "On" -variable ${cfgname}(LRT:) -value on
    radiobutton $f.lrtoffRB -text "Off" -variable ${cfgname}(LRT:) -value off

    grid $f.lrtonRB -sticky w
    grid $f.lrtoffRB -sticky w
  

}

#-----------------------------------------------------------------------------
# end of procedures
#-----------------------------------------------------------------------------

wm withdraw .

Introduction 0
makeMainMenu
return
