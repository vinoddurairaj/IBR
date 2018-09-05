#
#-----------------------------------------------------------------------------
#
# migratecfg.tcl -- convert .cfg and .conf files from previous 
#                   installations to current expectations of %PRODUCTNAME%
#                   %VERSION%.
#
# Copyright (c) 1997, 1998, 1999 %COMPANYNAME% - All Rights Reserved.
#
#-----------------------------------------------------------------------------

global MIG_INFO
set MIG_INFO(migratedoneflag) 0
set MIG_INFO(migrateflag) 0
set MIG_INFO(FTSWftdflag) 0
set MIG_INFO(%PKGNM%flag) 0
set MIG_INFO(babsize) -1
set MIG_INFO(num_chunks) -1
set MIG_INFO(chunk_size) -1
set MIG_INFO(pstoredev) ""
set MIG_INFO(fromdir) ""
set MIG_INFO(domigrate) 0

#-----------------------------------------------------------------------------
# migratecfgs -- main config file migration entry point
#                returns 1 if migration of cfgs and bab/pstore was done
#                returns 0 otherwise
#-----------------------------------------------------------------------------
proc migratecfgs {} {

    global MIG_INFO
    if {![chkmigrate%PKGNM%]} {
        chkmigrateFTSWftd
    }
    if {!$MIG_INFO(migrateflag)} {
        return 0
    }
    if {$MIG_INFO(%PKGNM%flag)} {
        parseconffile $MIG_INFO(fromdir) %Q%
        if {0 == [promptformigrate]} {
            return 0
        }
    } else {
        parseconffile $MIG_INFO(fromdir) ftd
        if {$MIG_INFO(pstoredev) != "" && $MIG_INFO(FTSWftdflag)} {
            if {0 == [promptformigrate]} {
                return 0
            }
        }
    }
    if {$MIG_INFO(domigrate) && $MIG_INFO(%PKGNM%flag)} {
        return [domigrate%PKGNM%]
    } elseif {$MIG_INFO(domigrate) && $MIG_INFO(FTSWftdflag)} {
        if {$MIG_INFO(pstoredev) != ""} {
            return [domigrateFTSWftd]
        }
    }
    return 0
}

#-----------------------------------------------------------------------------
# promptformigrate -- display the pop-up form to migrate .cfg files
#-----------------------------------------------------------------------------
proc promptformigrate {} {
    global MIG_INFO

    set w .migraT
    if {[winfo exists $w ]} {
        destroy $w
    }
    toplevel $w
    wm title $w "Migrate Configuration Files"
    wm transient $w [winfo toplevel .]
    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab $w"
    
    if {$MIG_INFO(%PKGNM%flag)} {
        set rap "A previous set of configuration files
for %PRODUCTNAME% has been detected on this system.
Would you like to migrate them into the current 
%PRODUCTNAME% environment?"
    } elseif {$MIG_INFO(FTSWftdflag)} {
        set rap "A previous set of configuration files
for FullTime Data has been detected on this system.  
Would you like to have them converted and migrated into 
the current %PRODUCTNAME% environment?"
    } else {
        destroy $w
        update
        return 0
    }
    label $w.label -padx 20 -pady 10 -border 1 -relief raised -anchor c \
            -text "$rap"
    tixButtonBox $w.box -orientation horizontal
    $w.box add yes -text "Yes" -underline 0 -width 5 -command {
        global MIG_INFO
        set MIG_INFO(domigrate) 1
        destroy .migraT
    }
    $w.box add no -text "No" -underline 0 -width 5 -command {
        global MIG_INFO
        set MIG_INFO(domigrate) 0
        destroy .migraT
    }        
    pack $w.box -side bottom -fill x
    pack $w.label -side top -fill both -expand yes
    
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
    tkwait window $w
    return $MIG_INFO(domigrate)
}

#-----------------------------------------------------------------------------
# warnmigratepopup -- display a pop-up warning about coverted cfg files
#-----------------------------------------------------------------------------
proc warnmigratepopup {} {
    global MIG_INFO

    if {$MIG_INFO(%PKGNM%flag) || $MIG_INFO(migratedoneflag) == 0} {
        # -- dont warn about same product name conversions
        return
    }
    set w .migwarn
    if {[winfo exists $w ]} {
        destroy $w
    }
    toplevel $w
    wm title $w "Conversion Warning"
    wm transient $w [winfo toplevel .]
    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab $w"
    
    set rap "WARNING!!!\n\nAutomatic conversion of
FullTime Data configuration files to %PRODUCTNAME% format
has occurred.  Please examine and correct any unwanted 
effects of this conversion, such as unwanted changes in
directory path specifications or in volume names in 
each of these configuration files."
    label $w.label -padx 20 -pady 10 -border 1 -relief raised -anchor c \
            -text "$rap"
    tixButtonBox $w.box -orientation horizontal
    $w.box add continue -text "Continue" -underline 0 -width 10 -command {
        destroy .migwarn
    }
    pack $w.box -side bottom -fill x
    pack $w.label -side top -fill both -expand yes
    
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
    tkwait window $w
    return 
}

#-----------------------------------------------------------------------------
# chkmigrate%PKGNM% -- returns 1 if we need to migrate a %PRODUCTNAME%
#                      set of configuration files
#-----------------------------------------------------------------------------
proc chkmigrate%PKGNM% {} {
    global MIG_INFO
    global tcl_platform

    set newestmtime 0
    set newestdir ""
    set tname ""
    # -- see if there is an environment saved for %PRODUCTNAME%
    set dirs [glob -nocomplain /%FTDVAROPTDIR%/%PKGNM%*]
    foreach tname $dirs {
        if {[file isdirectory $tname]} {
            set mt [file mtime $tname]
            if {$mt > $newestmtime} {
                set newestmtime $mt
                set newestdir $tname
            }
        }
    }
    if {$newestmtime == 0} {
        return 0
    }
    set MIG_INFO(fromdir) $tname
    set MIG_INFO(migrateflag) 1
    set MIG_INFO(%PKGNM%flag) 1
    return 1
}

#-----------------------------------------------------------------------------
# chkmigrateFTSWftd -- returns 1 if we need to migrate a FullTime Data
#                      set of configuration files
#-----------------------------------------------------------------------------
proc chkmigrateFTSWftd {} {
    global MIG_INFO
    global tcl_platform

    if {$tcl_platform(os) == "AIX"} {
        set pkgnm "ftd"
        set dirpath "/var/ftd"
    } else {
        set pkgnm "FTSWftd"
        set dirpath "/var/opt/FTSWftd"
    }
    set newestmtime 0
    set newestdir ""
    set tname ""
    # -- see if there is an environment saved for FullTime Data
    set dirs [glob -nocomplain ${dirpath}/${pkgnm}*]
    foreach tname $dirs {
        if {[file isdirectory $tname]} {
            set mt [file mtime $tname]
            if {$mt > $newestmtime} {
                set newestmtime $mt
                set newestdir $tname
            }
        }
    }
    if {$newestmtime == 0} {
        return 0
    }
    set MIG_INFO(fromdir) $tname
    set MIG_INFO(migrateflag) 1
    set MIG_INFO(FTSWftdflag) 1
    return 1
}

#-----------------------------------------------------------------------------
# parseconffile -- parses the driver .conf file 
#-----------------------------------------------------------------------------
proc parseconffile {dirname prefix} {

    global MIG_INFO
    global tcl_platform
    set pstoredev ""
    set confpath ${dirname}/${prefix}.conf
    if {[file exists $confpath]} {
        if {[catch {open $confpath r} fd]} {
            return
        }
        set line ""
        while {![eof $fd]} {
            set line [gets $fd]
            if {[string length "$line"] > 1} {
                if {[string range "$line" 0 0] != "#"} {
                    regsub -all {=} $line { } oline
                    regsub -all {;} $oline { } oline
                    switch -exact [lindex $oline 0] {
                        "num_chunks" {set MIG_INFO(num_chunks) [lindex $oline 1]}
                        "chunk_size" {set MIG_INFO(chunk_size) [lindex $oline 1]}
                        "pstore" {set pstoredev [lindex $oline 1]}
                    }
                }
            }
        }
        catch {close $fd}
        set MIG_INFO(babsize) [expr ($MIG_INFO(num_chunks) * $MIG_INFO(chunk_size)) / 1048576]
        set MIG_INFO(pstoredev) "$pstoredev"
    }
}


#-----------------------------------------------------------------------------
# domigrateFTSWftd -- migrate FullTime Data .cfg files
#-----------------------------------------------------------------------------
proc domigrateFTSWftd {} {
    global MIG_INFO

    popmigratingmsg
    foreach fdirname "[glob -nocomplain $MIG_INFO(fromdir)/*.cfg]" {
        set fname [lindex [file split $fdirname] end]
        if {![catch {open $fdirname r} fd]} {
            set ofd [open "/%FTDCFGDIR%/$fname" w 0644]
            set pflag 0
            if {[string range $fname 0 0] == "p"} {
                set pflag 1
            }
            set linecount 0
            set primflag 0
            while {![eof $fd]} {
                set line [gets $fd]
                incr linecount
                regsub -all {FullTime Data} $line {%PRODUCTNAME%} oline
                regsub -all {FTSWftd} $oline {%PKGNM%} oline
                regsub -all {FULLTIME} $oline {%CAPPRODUCTNAME%} oline
                regsub -all {FTD} $oline {%CAPQ} oline
                regsub -all {ftd} $oline {%Q%} oline
                set line "$oline"
                puts $ofd "$line"
                if {$linecount == 2} {
                    puts $ofd "#  %PRODUCTNAME% Version %VERSION%"
                }
                if {[lindex $line 0] == "SYSTEM-TAG:"} {
                    if {[lindex $line 2] == "PRIMARY"} {
                        set primflag 1
                    } else {
                        set primflag 0
                    }
                }
                if {$pflag && $primflag && [lindex $line 0] == "HOST:"} {
                    puts $ofd "  PSTORE:              $MIG_INFO(pstoredev)"
                    set primflag 0
                }
            }
            catch {close $fd}
            catch {close $ofd}
        }
    }
    killpopmigratingmsg
    set MIG_INFO(migratedoneflag) 1
    warnmigratepopup
}


#-----------------------------------------------------------------------------
# domigrate%PKGNM% -- migrate %PRODUCTNAME% .cfg files
#-----------------------------------------------------------------------------
proc domigrate%PKGNM% {} {
    global MIG_INFO

    popmigratingmsg
    # -- simply copy the files
    foreach fname "[glob -nocomplain $MIG_INFO(fromdir)/*.cfg]" {
        set fn [lindex [file split $fname] end]
        exec /bin/cp $fname /%FTDCFGDIR%/$fn
    }
    killpopmigratingmsg
    set MIG_INFO(migratedoneflag) 1
}

#-----------------------------------------------------------------------------
# popmigratingmsg -- put a a popup indicating that we're migrating .cfg files
#-----------------------------------------------------------------------------
proc popmigratingmsg {} {

    set t .migratepop
    if {[winfo exists $t ]} {
        destroy $t
    }
    toplevel $t
    wm transient $t [winfo toplevel .]
    wm overrideredirect $t 1
    set oldFocus [focus]
    set oldGrab [grab current $t]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab $t"
    
    set rap "...Migrating Configuration Files..."
    label $t.label -padx 20 -pady 10 -border 1 -relief raised -anchor c \
            -text "$rap"
    pack $t.label -side top -fill both -expand yes
    
    wm withdraw $t
    update idletasks
    if { [string compare $t  "."] == 0 } {
	set vrootx 0
	set vrooty 0
    } else {
	set vrootx [winfo vrootx [winfo parent $t]]
	set vrooty [winfo vrooty [winfo parent $t]]
    }
    set x [expr [winfo screenwidth $t]/2 - [winfo reqwidth $t]/2 \
	    - $vrootx]

    set t .migratepop
    if {[winfo exists $t ]} {
        destroy $t
    }
    toplevel $t
    wm transient $t [winfo toplevel .]
    wm overrideredirect $t 1
    set oldFocus [focus]
    set oldGrab [grab current $t]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    catch "grab $t"
    
    set rap "...Migrating Configuration Files..."
    label $t.label -padx 20 -pady 10 -border 1 -relief raised -anchor c \
            -text "$rap"
    pack $t.label -side top -fill both -expand yes
    
    wm withdraw $t
    update idletasks
    if { [string compare $t  "."] == 0 } {
	set vrootx 0
	set vrooty 0
    } else {
	set vrootx [winfo vrootx [winfo parent $t]]
	set vrooty [winfo vrooty [winfo parent $t]]
    }
    set x [expr [winfo screenwidth $t]/2 - [winfo reqwidth $t]/2 \
	    - $vrootx]
    set y [expr [winfo screenheight $t]/2 - [winfo reqheight $t]/2 \
	    - $vrooty]
    wm geom $t +$x+$y
    wm deiconify $t
    
}

#-----------------------------------------------------------------------------
# killpopmigratingmsg -- take down migrating message
#-----------------------------------------------------------------------------
proc killpopmigratingmsg {} {
    set t .migratepop
    if {[info commands $t] != ""} {
        destroy $t
        update idletasks
    }
}

#=========================< END OF FILE >=====================================
