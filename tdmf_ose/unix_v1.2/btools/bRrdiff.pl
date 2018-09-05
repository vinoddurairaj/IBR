#!/usr/bin/perl -Ipms
################################
##############################
# 
#  perl -w ./bRrdiff.pl
#
#    determine source differences between last build tag and
#        tip of trunk or branch
#
#    exit 0  :  source mods REPORTED
#    exit 1  :  source mods NOT reported
#
###############################
################################

################################
##############################
# vars  /  pms  /  use
##############################
################################

eval 'require bRglobalvars';
eval 'require bRcvstag';
eval 'require bRcvsprojects';

our $Bbnum;
our $CVSTAG;
our $branch_rdiff;
our $projects;

our @postFormat;
our @btoolsSegregate;

########################################
######################################
# STDERR  /  STDOUT
######################################
########################################

open (STDOUT, ">logs/rdiff.txt");
open (STDERR, ">\&STDOUT");

########################################
######################################
#  exec statements 
######################################
########################################

print "\n\nSTART  :   bRrdiff.pl\n\n";

########################################
######################################
#  log values 
######################################
########################################

system qq(pwd);
print "\n";
print "\nprojects  :\n$projects\n\n";
print "\ncurrent CVSTAG  =  $CVSTAG\n";
print "\nbranch_rdiff    =  $branch_rdiff\n";

########################################
######################################
#  clean 
######################################
########################################

unlink "sourcechangelist.txt";
unlink "btoolschangelist.txt";

########################################
######################################
#  root of workarea  : required directory
######################################
########################################

chdir "../../..";

system qq(pwd);

########################################
######################################
#  log then exec cvs rdiff 
######################################
########################################

print "\n\n\@list= qx ! cvs -Q rdiff -s $branch_rdiff -r $CVSTAG $projects ! \n\n";

@list= qx ! cvs -Q rdiff -s $branch_rdiff -r $CVSTAG $projects !;

########################################
######################################
#  log @list 
#  process @list to proper single line format
######################################
########################################

print "\n\nlist  =\n@list\n";

foreach $e0 (@list) {
   #print "\nFOREACH : e0 :  $e0\n";
   if ( "$e0" =~ m/^File / || "$e0" =~ m/^\ File/ ) {
      $e0 =~ s/^File\ //g || $e0 =~ s/^\ File\ //g; 
      $e0=~ s/changed\ from\ revision(.*)$//g;  #match string and everything to end of line
      $e0=~ s/is\ removed(.*)$//g;		  #match string and everything to end of line
      $e0=~ s/is\ new(.*)$//g;		  #match string and everything to end of line
      #print "\nIF : $e0\n";
      #$e0 =~ s/^\ //g; 
      push (@postFormat, "$e0");
      }
}

print "\n\npostFormat  =  \n@postFormat\n\n"; 

########################################
######################################
# create list @btoolsSegregate 
######################################
########################################

$i="0";

for ($i=0 ; $postFormat[$i] ; $i++) {
      print "\nFOREACH : index : postFormat[$i] : $postFormat[$i]\n";
      if ( $postFormat[$i] =~ m/btools\// ) {                     
         print "\nIF :  btools match : $postFormat[$i]\n";
         push (@btoolsSegregate,"$postFormat[$i]"); 
         splice ( @postFormat, $i, 1);
         $i=$i - 1;
         print "\n\ni after subtract - 1 = :  $i\n";   
      } 

}

print "\n\npostFormat array  =\n@postFormat\n\n"; 

print "\n\nbtoolsSegregate array  =\n@btoolsSegregate\n\n"; 

print "\n\npostFormat length  =  $#postFormat\n\n";

chdir "tdmf_ose/unix_v1.2/btools";

system qq(pwd);

if ( $#postFormat > -1 ) {
    print "\npostFormat array length = : $#postFormat\n";
    print "\nexit code 0, run the build ....source mods reported\n\n";
    exit(0);
}
else {
    print "\npostFormat array length = : $#postFormat\n";
    print "\nexit code 1, NO build .... NO source mods reported\n\n";
    exit(1);
}

print "\n\nEND  :  bRrdiff.pl\n\n";
