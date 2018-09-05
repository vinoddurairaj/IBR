#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRmakeVARS.pl
#
#	run "make clean" and "make" in  :
#
#	../ftdsrc/mk  :  to produce vars.pm  
#
#	vars.pm  :  perl pm with DEV derived variables / values
#
#####################################
#######################################
 
#######################################
#####################################
# vars  /  pms   /  use
#####################################
#######################################

eval 'require bRglobalvars';
eval 'require bRstripbuildnumber';

our $COMver;
our $Bbnum;
our $PATCHver;
our $stripbuildnum;

#######################################
#####################################
# STDOUT  /  STDERR
#####################################
#######################################

open (STDOUT, ">logs/makeVARS.txt");
open (STDERR, ">&STDOUT");

#######################################
#####################################
#  exec statements 
#####################################
#######################################

print "\n\nSTART  :  bRmakeVARS.pl\n\n";

#######################################
#####################################
#  log values 
#####################################
#######################################

print "\nCOMver         =  $COMver\n";
print "\nBbnum          =  $Bbnum\n";
print "\nstripbuildnum  =  $stripbuildnum\n";

#######################################
#####################################
#  unlink, chdir, run makes and cp vars.pm ./
#####################################
#######################################

if (-e "pms/bRdevvars.pm") {
   unlink "pms/bRdevvars.pm";
   print "\n\nunlink pms/bRdevvars.pm rc =$?\n\n";
}

chdir "../ftdsrc"  || die "chdir death ../ftdsrc  :  $!\n";;

system qq(pwd);

system qq(cvs update);

print "\n\ncvs update ../ftdsrc rc  =  $?\n\n";

system qq(./bldcmd exitat=brand brand=rep build=$stripbuildnum fix=$PATCHver);

print "\n\nmake vars.pm rc  =  $?\n\n";

#######################################
#####################################
#  grab / format vars we need from vars.pm 
#
#	dump to bRdevvars.pm  :  issue  $VERSION is used
#	via %INC  ftp module  :  causing major issue....
#####################################
#######################################

open (FHv,"<mk/vars.pm");
@vars=<FHv>;
close (FHv);

# routine to gather each variable string we require from vars.pm
# currently, hopefully, we only require $VERSION = $PRODver (in build scripts)
chomp (@vars);
foreach $eV (@vars) {
   if ( "$eV" =~ /VERSION=/ ) {    #match VERSION=
      print "\nIF VERSION = $eV\n";
      $eV=~s/\$VERSION=//;
      print "\nIF0 VERSION = $eV\n";
      $PRODver="$eV";
      print "\nPRODver=$PRODver\n";
      push (@devvars,"\$PRODver=$PRODver\n");

   }
}

print "\ndevvars =\n@devvars\n";

chdir "../btools";

system qq(pwd);

open (FHdv,">pms/bRdevvars.pm");
print FHdv @devvars;
close (FHdv);

system qq(cvs commit -m "$Bbnum  :  auto build commit" pms/bRdevvars.pm);

print "\n\ncvs commit bRdevvars.pm rc  =  $?\n\n";

#######################################
#######################################

print "\n\nEND  :  bRmakeVARS.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);
