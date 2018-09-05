#
# Copyright (c) 2001 Fujitsu Software Technology Corporation. All rights reserved.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
# Government is subject to restrictions as set forth in
# subparagraph (c)(1)(ii) of the Rights in Technical Data and
# Computer Software clause at DFARS 52.227-7013 and in similar
# clauses in the FAR and NASA FAR Supplement.
#
#
#-------------------------------------------------------------------------
#
# Wizard Namespace
#
# This namespace contains the procedures needed to create Wizards using
# simple template "frame" procedures.
#
#-------------------------------------------------------------------------
namespace eval Wizard {

    #
    # the WIZ array contains all of the variables that can be defined
    # by each frame to determine the appearance and behavior of the
    # frame
    #
    variable WIZ 
    
}

#-----------------------------------------------------------------------
#
# Wizard::create <parent> <nextproc>
# 
# This procedure creates a single frame, and a binding which calls 
# itself upon activation of the "Next" or "Back" buttons.  "nextproc" is
# the name of the frame procedure to use to define this frame.  
#
#-----------------------------------------------------------------------
proc Wizard::create { parent wizname nextproc } {  
    
    variable WIZ  
    variable par
    global HELP
    #
    #     HACK - Debug
    #
    #   puts "--------------------------------------------------------"
#   puts "--------------------------------------------------------"
    #   foreach i [array names ::QdsConfigWiz::cfg] {
	#	if {[string compare $i "devs" ] !=0 && [string compare $i "fs"] !=0} {
	    #    puts "cfg($i): $::QdsConfigWiz::cfg($i)"
	    #	}
	    #   }
	    
	    
	    
	    #
	    # set up some defaults for attributes that each frame can define
	    #
	    set WIZ(namespace) $wizname
	    set WIZ(backCmd) ""
	    set WIZ(commit) ""
	    set WIZ(verify) ""
	    set WIZ(step) ""
	    set WIZ(title) ""
	    set WIZ(maintitle) ""
	    set WIZ(subtitle) ""
	    set WIZ(stepfont) \
		    "-*-application-*-r-*-sans-17-*-*-*-*-*-*-*"
	    set WIZ(titlefont) \
		    "-*-application-*-i-*-sans-17-*-*-*-*-*-*-*"
	    set WIZ(bannerfont) \
		    "-*-palatino-medium-i-*-*-34-*-*-*-*-*-*-*"
	    set WIZ(subfont) \
		    "-*-palatino-medium-r-*-*-14-*-*-*-*-*-*-*"
    set WIZ(textfont) "-*-palatino-medium-r-*-*-14-*-*-*-*-*-*-*"
    set WIZ(inputfont) "-*-palatino-bold-r-*-*-14-*-*-*-*-*-*-*"
    option add *foreground black
    option add *background grey
    option add *font $WIZ(textfont)
    option add *Entry*Font "-*-courier-medium-r-*-*-12-*-*-*-*-*-*-*"
    option add *Listbox*Font "-*-courier-medium-r-*-*-12-*-*-*-*-*-*-*"
    option add *Label*Font "-*-palatino-medium-r-*-*-14-*-*-*-*-*-*-*"
    if { [string compare $parent "." ] == 0  } { 
	set parent ""
    }
 
    if { [winfo exists $parent.wiz ] } { 	
	destroy $parent.wiz
    }

  
    #
    # content frame for procedures to use
    # 
    set WIZ(parent) ${parent}.wiz.content

    #
    # abbreviation
    #
    set p $parent.wiz

    #
    # create the dialog 
    #
    toplevel $p
    set WIZ(toplevel) $p
    wm withdraw $p

    frame $p.topF
    pack $p.topF -padx 1c -pady .5c
    frame $p.content  

    if {[info exists WIZ(thisframe)]} {
	set WIZ(lastframe) $WIZ(thisframe)

    } else {
	set WIZ(lastframe) $nextproc
    }

    #
    # Remember what frame this is, so we can go back
    #
    set WIZ(thisframe) $nextproc
 
    #
    # Call the frame procedure that defines this wizard frame
    #
    namespace inscope $wizname $nextproc	

#
#  HACK
#
    wm title $p "$WIZ(titlebar) - $nextproc"
    frame $p.wiztitleF -background #000066
    
    # if this frame is a 'step' in a process, label it that way and
    # put the title to  the left 
    # 
    if {[string compare $WIZ(step) "" ] != 0  } {
	label $p.wiztitleF.stepL -text $WIZ(step)  \
		-font  $WIZ(stepfont) -foreground yellow  -background #000066
	pack $p.wiztitleF -pady .5c  -anchor w -in $p.topF -expand 1 -fill x
	pack $p.wiztitleF.stepL -side left
	
	label $p.wiztitleF.titleL -text $WIZ(title) \
		-font  $WIZ(titlefont) -foreground #ddFFFF -background #000066
	pack $p.wiztitleF.titleL -side left 
    } else {
	
	label $p.wizicon -image [image create photo -file new_qg_logo.gif]
	pack $p.wizicon -anchor e -in $p.wiztitleF  -side right
	set side top
	pack $p.wiztitleF  -pady .5c -in  $p.topF -anchor c   -expand 1 -fill x
    
	#
	# title this frame
	#
	frame $p.wiztitleF2
	label $p.wiztitleF2.titleL -text $WIZ(maintitle) \
		-font  $WIZ(bannerfont) -foreground #ddFFFF -background #000066
	label $p.wiztitleF2.title2L -text $WIZ(subtitle) \
		-font  $WIZ(subfont) -foreground #dddddd -background #000066
	pack $p.wiztitleF2.titleL $p.wiztitleF2.title2L -in $p.wiztitleF2 \
		-expand 1 -fill x
	pack $p.wiztitleF2 -in $p.wiztitleF -side left -expand 1 -fill x -anchor c
    }

  

    if {[string compare $WIZ(text) ""] != 0 } {
	#
	# pack the intro text
	#
	label $p.introT -bd 0  -wraplength 10c \
		-text $WIZ(text)\
		-font $WIZ(textfont) \
		-justify left \
			
	pack $p.introT -in $p.topF -anchor w
    }
    #
    # pack the rest of the widgets that were defined by the frame proc
    #
    pack $p.content -in $p.topF -anchor c -expand 1 -fill both
  
     
    # 
    # Conditionally create the Next, Back, Exit, and Help buttons
    #
    frame $p.bottomF -relief groove -bd 2
    frame $p.wizbuttonsF 
    pack $p.wizbuttonsF -in $p.bottomF -anchor c -pady .5c
    pack $p.bottomF -expand 1 -fill both 
 
    if { [ string compare $WIZ(backProc) "" ] != 0 } {
	button $p.wizbuttonsF.backB \
		-width 1.5c \
		-image [image create photo -file l_arrow.gif] \
		-command [namespace code "doBackCmd $parent" ] \
		-font $WIZ(textfont)
	pack $p.wizbuttonsF.backB -side left -padx 1c
    }
    if { [ string compare $WIZ(nextProc)  "" ] != 0 } {
	#
	# When the 'Next' button is activated, execute the 
	# frame-defined 'commit' command before going on to the next frame 
	#
	button  $p.wizbuttonsF.nextB -width 1.5c -default active \
		-image [image create photo -file r_arrow.gif] \
		-command [namespace code "doNextCmd $parent"]
	pack $p.wizbuttonsF.nextB -side left -padx 1c
	
    }
   
    if { [ string compare $WIZ(exitProc) "" ] != 0 } { 
	button $p.wizbuttonsF.exitB  \
		-width 1.5c \
		-image [image create photo -file door_exit.gif] \
		-command [namespace code {$WIZ(exitProc)}] \
		-font $WIZ(textfont)
	pack $p.wizbuttonsF.exitB -side left -padx 1c
    }
    if { [info exists HELP($WIZ(thisframe))]} {
	button $p.wizbuttonsF.helpB  \
		-image [image create photo -file help.gif ]\
		-command [namespace code {showHelp  $HELP($WIZ(thisframe))}] \
		-font $WIZ(textfont)
	pack $p.wizbuttonsF.helpB -side left -padx 1c
   }	
   
   bind $p <Return>  [namespace code "doNextCmd $parent"]
   #
   # center the frame on the screen
   #
   update
   set sw [ winfo screenwidth $p ]
   set sh [ winfo screenheight $p ]
   set w [winfo reqwidth $p ]
   set h [winfo reqheight $p ]
   set x [ expr ($sw - $w) / 2 ]
   set y [ expr ($sh - $h) / 2 ] 
   if {[string compare $WIZ(text) ""] != 0 } {
       $p.introT configure -wraplength  [winfo reqwidth $p ]
   }
   wm geometry $p +$x+$y
   wm deiconify $p
}

#-----------------------------------------------------------------------
#
# Wizard::DoNextCmd
# 
# This procedure is executed when 'Next' is clicked.  It first runs 
# a verification procedure; if this succeeds it executes a commit procedure 
# and calls the next frame.  
#
#-----------------------------------------------------------------------
proc Wizard::doNextCmd { parent } {
    variable WIZ 
    if {[string compare $WIZ(verify) "" ] != 0 } {
	set ret [namespace inscope $WIZ(namespace) \
		{ eval $::Wizard::WIZ(verify)} ]
	if {$ret != 0} {
	    return $ret
	}
    }
   
    if {[string compare $WIZ(commit) "" ] != 0 } {
	namespace inscope $WIZ(namespace) {eval $::Wizard::WIZ(commit)}
    }
    create $parent $WIZ(namespace) $WIZ(nextProc)
}

#-----------------------------------------------------------------------
#
# Wizard::DoBackCmd
# 
# This procedure is executed when 'Back' is clicked.  It runs a back procedure
# to figure out where it should go back to. 
#
#-----------------------------------------------------------------------
proc Wizard::doBackCmd {parent } {
    variable WIZ
    if {[string compare $WIZ(backCmd) "" ] != 0} {
	namespace inscope $WIZ(namespace) {eval $::Wizard::WIZ(backCmd)}
    }
    create $parent $WIZ(namespace) $WIZ(backProc)
}

#---------------------------------------------------------------------
#
# Wizard::showHelp
# 
# Help display window with cancel button.   
#
#---------------------------------------------------------------------
proc Wizard::showHelp {helptext}  {
    variable WIZ 
    global HELP
	
    set x [winfo rootx $WIZ(parent)]
    set y [winfo rooty $WIZ(parent)]
	
    set x [expr $x + 50 ]
    set y [expr $x + 50 ]
	
    set p $WIZ(parent).help
    toplevel $p 
    wm geometry $p +$x+$y 

    wm title $WIZ(parent).help "Help"
	
    frame $p.helpF
    text $p.helpT -bd 0 -height 7  -width 70  -wrap word\
	    -yscrollcommand "$p.helpSB set" \
	    -font $WIZ(textfont) \
	    -highlightthickness 0 -borderwidth 2
    $p.helpT insert end $helptext
    $p.helpT configure -state disabled -relief sunken 
    pack $p.helpT -in $p.helpF -side left
    pack $p.helpF -padx .5c -pady .5c

    scrollbar $p.helpSB -orient v -command "$p.helpT yview"
    pack $p.helpSB -side left -in $p.helpF -expand 1 -fill y
    
    
    frame $p.bottomF -relief groove -bd 2
    frame $p.buttonsF 
    pack $p.buttonsF -in $p.bottomF -anchor c -pady .5c
    pack $p.bottomF -expand 1 -fill both 

    button $p.buttonsF.exitB -text "Cancel" -command "destroy $p" \
	    -font $WIZ(textfont)
    pack $p.buttonsF.exitB -side left -padx 1c
} 


