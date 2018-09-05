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
namespace eval Select {
    #
    # parent widget name 
    #
    variable par 
    variable selectlist 
    variable availlist
    variable selentry
}
proc Select::makeAvailSelect { p {lines 10} {width 20} availList selectedList \
	{availTitle "Available"} {selTitle "Selected"}} {
	
    variable par 
    variable selectlist
    variable availlist

    set par $p
    set availlist $availList
    set selectlist $selectedList
    
    frame $p.moveF
    frame $p.selectF

    
    frame $p.availF
    listbox $p.availLB -yscrollcommand "$p.availSB set" \
	    -height $lines -selectmode extended\
	    -width $width\
	    -selectbackground black -selectborderwidth 2\
	    -selectforeground grey\
	    -background white
    scrollbar $p.availSB -orient v -command "$p.availLB yview"
    label $p.availL -text $availTitle
    

    listbox $p.selectLB -yscrollcommand "$p.selectSB set" \
	  -height $lines  -selectmode extended\
	    -width $width\
	    -selectbackground black  -selectborderwidth 2\
	    -selectforeground grey \
	    -background white
		
   
    scrollbar $p.selectSB -orient v -command "$p.selectLB yview"

  
    label $p.selectL -text $selTitle 
   
    pack $p.availL -in $p.availF -side top
    pack $p.availLB -in $p.availF -side left
    pack $p.availSB -in $p.availF -side left -expand 1 -fill y
   

    pack $p.selectL -in $p.selectF -side top
    pack $p.selectLB -in $p.selectF -side left
    pack $p.selectSB -in $p.selectF -side left -expand 1 -fill y


    button $p.selectB -image [image create photo -file "r_arrow_sm.gif"]\
	    -state disabled \
	    -command \
	    [namespace code { 
	#
	# Add elements to selected list
	# 
	foreach item [$par.availLB curselection] {
	    lappend selectlist [$par.availLB get $item]
	}
	    
	# 
	# Sort selected list
	# 
	set selectlist [lsort $selectlist]
	
	#
	# remove elements from available list
	#
	foreach item [lsort -decreasing -integer [$par.availLB curselection]] {
	    set availlist [lreplace $availlist $item $item ]
	}
	
	#
	# sort avail list
	#
	set availlist [lsort $availlist]
	
	#
	# clear listboxes, replenish with new lists
	#
	$par.availLB delete 0 end
	foreach item $availlist {
	    $par.availLB insert end $item
	    
	}
	$par.selectLB delete 0 end
	foreach item $selectlist {
	    $par.selectLB insert end $item
	    
	}
	
	# eval "$par.selectLB insert end [$par.availLB get anchor active]"
	# $par.availLB delete anchor active
	$par.selectB configure -state disabled
    }
    ]
    
    button $p.unselectB -image [image create photo -file "l_arrow_sm.gif"]\
	    -state disabled -command\
	    [ namespace code { 
	#
	# Add elements to avail list
	#
	foreach item  [$par.selectLB curselection] {
	    lappend availlist [$par.selectLB get $item]
	}
	
	# 
	# Sort avail list
	# 
	set availlist [lsort $availlist]
	
	#
	# remove elements from selected list
	#
	foreach item [lsort -decreasing -integer \
		[$par.selectLB curselection]] {
	    set selectlist [lreplace $selectlist $item $item ]
	}
	
	#
	# sort avail list
	#
	set availlist [lsort $availlist]
	
	#
	# clear listboxes, replenish with new lists
	#
	$par.availLB delete 0 end
	foreach item $availlist {
	    $par.availLB insert end $item
	    
	}
	$par.selectLB delete 0 end
	foreach item $selectlist {
	    $par.selectLB insert end $item
	    
	}
	
	
	$par.unselectB configure -state disabled
    }
    ]
    
    pack $p.selectB -in $p.moveF -side top -pady 1c -padx 1c
    pack $p.unselectB -in $p.moveF -side top -pady 1c -padx 1c
    
    pack $p.availF $p.moveF $p.selectF -side left
    
    bind $par.availLB <Button-1> [namespace code {
	
	update
	if {[$par.availLB size ] == 0} {
	    $par.selectB configure -state disabled
	} else {
	    $par.selectB configure -state normal
	}
	$par.unselectB configure -state disabled
    }    
    ]
    
    bind $par.selectLB <Button-1> [namespace code {
	
	update	
	if {[$par.selectLB size ] == 0} {
	    $par.unselectB configure -state disabled
	} else {
	    $par.unselectB configure -state normal
	}
	$par.selectB configure -state disabled
    } 
    ]
    
    foreach item $availList {
	$p.availLB insert end $item
    }

	
    foreach item $selectedList { 
	$p.selectLB insert end $item
    }


	return "$par.availLB $par.selectLB"

} 

proc Select::makeSelect { p {lines 10} {width 20} {availList ""}\
	{selectv} {selects} {availTitle "Available"}\
	{selTitle "Selected"}} {

    variable par 
    variable selectvar
    variable selectsize
  
    set selectvar $selectv
    set selectsize $selects

    global $selectvar
    global $selectsize

    if {![info exists $selectvar]} {
	set $selectvar ""
    }
 
    set par $p

    label $p.availL -text $availTitle

    pack $p.availL -side top -anchor w

    frame $p.availF
    listbox $p.availLB -yscrollcommand "$p.availSB set" \
	    -selectbackground black \
	    -selectforeground grey \
	    -background white \
	    -height $lines -width $width
    scrollbar $p.availSB -orient v -command "$p.availLB yview"

    pack $p.availLB -in $p.availF -side left 
    pack $p.availSB -in $p.availF -side left -expand 1 -fill y
    pack $p.availF -side top -anchor w

    label $p.selectL -text $selTitle 
    pack $p.selectL -side top -anchor w
	
    entry $p.selectE  -width $width\
	    -textvariable $selectvar\
	    -background white
    pack $p.selectE -anchor w -side top

    foreach item $availList {
	$p.availLB insert end $item
    }

    bind $par.availLB <Button-1>  [namespace code {
	$par.availLB select clear 0 end
	$par.availLB select anchor [$par.availLB nearest %y]
	$par.availLB select set anchor [$par.availLB nearest %y]
	update
	set cur [$par.availLB curselection]
	if {[string compare $cur ""] != 0 } {
	   set $selectvar [lindex [$par.availLB get $cur] 0]
	    regexp {\((.*) MB\)} [$par.availLB get $cur] x $selectsize
	}
	
    }
    ]

    return $p.selectE

}  
proc Select::makeEnterSelect { p {lines 10} {width 20} \
	{selectlist}  {enterTitle "Enter Selection:"}\
	{selTitle "Selected"}} {

    variable par 
    variable selentry

    set selentry ""
 
    set par $p
   
    frame $p.enterF
    label $p.enterL -text $enterTitle

    pack $p.enterL -side top -in $p.enterF -anchor w

    entry $p.enterE -background white \
	   -width $width -textvariable ::Select::selentry
   
    button $p.addB -text "Add" -command [namespace code {
	$par.selectLB insert end $::Select::selentry
    }
    ]
    
    pack $p.enterE -in $p.enterF -anchor w  
    pack $p.enterF -anchor w

    pack $p.addB -in $p.enterF -pady 5
    
    frame $p.selectF

    label $p.selectL -text $selTitle  

    listbox $p.selectLB -yscrollcommand "$p.selectSB set" \
	    -height $lines  -selectmode extended\
	    -width $width\
	    -selectbackground black  -selectborderwidth 2\
	    -selectforeground grey \
	    -background white
   
    scrollbar $p.selectSB -orient v -command "$p.selectLB yview"
  
    button $p.removeB -text "Remove" -command [namespace code {
	set selectlist ""
	foreach item [$par.selectLB get 0 end] {
	    lappend selectlist $item
	} 
	foreach item [lsort -decreasing -integer \
		[$par.selectLB curselection]] {
	    set selectlist [lreplace $selectlist $item $item ]
	} 
	$par.selectLB delete 0 end
	foreach item $selectlist {
	    $par.selectLB insert end $item
	    
	}
    }
    ]
    
    foreach item $selectlist {
	$p.selectLB insert end $item
    }

    pack $p.selectL -in $p.selectF -side top -anchor w
    pack $p.selectLB -in $p.selectF -side left
    pack $p.selectSB -in $p.selectF -side left -expand 1 -fill y
    
    pack $p.selectF -in $p.enterF
    pack $p.removeB -pady 10 -in $p.enterF
    
    return $p.selectLB

}  
