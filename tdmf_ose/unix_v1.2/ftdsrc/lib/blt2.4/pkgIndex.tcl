# Tcl package index file, version 1.0

proc Blt_MakePkgIndex { dir } {
    global tcl_platform

    set suffix .so.2
    set soext [lindex [split $suffix "."] 1]
    set library libBLT$suffix
    foreach lib {  } {
	catch { load $lib.$soext BLT }
    }
    if { $tcl_platform(os) == "Linux" } {
       set path /usr/lib
    } else {
       set path [file dirname $dir]
    }
    package ifneeded BLT 2.4 [list load [file join $path $library] BLT]
}

Blt_MakePkgIndex $dir
rename Blt_MakePkgIndex ""
