#!/usr/bin/perl
#############################################
#############################################
#  rsyncSrcpool.pl
#
#    rync local /work/srcpool/tdmf/bldenv/s390/
#    with anaconda:/srcpool/tdmf/bldenv/s390
#
#    local /srcpool is ln -s to /work/srcpool
#
#############################################
#############################################

open (STDOUT,">rsyncsrcpool.txt");
open (STDERR,">&STDOUT");
our $remotePath="/srcpool/tdmf/bldenv/s390x/"; #end of line /  :  need it
our $localPath="/work/srcpool/tdmf/bldenv/s390x";
system qq(sudo rsync -avz -e ssh root\@9.29.94.43:$remotePath $localPath);

close (STDOUT);
close (STDERR);


