#
# Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
# Government is subject to restrictions as set forth in
# subparagraph (c)(1)(ii) of the Rights in Technical Data and
# Computer Software clause at DFARS 52.227-7013 and in similar
# clauses in the FAR and NASA FAR Supplement.
#
#
#!/%OPTDIR%/%PKGNM%/libexec/perl
#
# This front end program has the following requirements (as decided by me 
# on the spur of the moment)
#
# 1)  It must call some external program with the correct number, type, etc. of
#     arguments.
#
# 2)  All arguments must be correct before any are processed.
#
# 3)  All arguments are processed in the order given (left to right)
#

sub usage 
{
   print "Usage: %Q% start n [-f]\n";
   print "       %Q% stop n [-f]\n";
   print "       %Q% mirror n on | off\n";
   print "       %Q% checkpoint n [-c cmd]\n";
   print "       %Q% chart\n";
   print "       %Q% config\n";
   print "       %Q% monitor\n";
   print "       %Q% update\n";
   print "       %Q% backfresh\n";
   print "       %Q% refresh\n";
   print "       %Q% set group n keyword=value [keyword=value...]\n";
   print "       %Q% set device n keyword=value [keyword=value...]\n";
   print "       %Q% info group n \n";
   print "Where n is either \"-a\" or a list of logical group numbers\n";
   print "No action performed\n";
   exit 1;
}

sub get_expr_args
{
   my @params = @_;
   $expr = $params[0];

   print "\nget_expr_args, expr: \"$expr\"\n";
   print "Trying to match against \"$cmdline\"\n";

   if ($cmdline =~ /$expr/)
   {
       print "Get_expr_args, Made a match\n";
       $Grps    = $1;
       $Action  = $2;
       $Val_list= $3;
   }
   else
   {
       usage ();
   }
}

sub stopstartem
{
    my $force_param;
    my @cmd_args;
    my @fn;

    if ($Cmd eq "start")
    {
	$fn = "Start function";
    }
    else
    {
	$fn = "Stop function";
    } 
    if ($Grps eq "-a")
    {
	print "Do something to get all the groups.\n";
	print "Faking it by setting all groups to be 1 2 3\n";
	$Grps = "1 2 3";
    }
	
    for (split ' ', $Grps)
    {
	if ($Action eq "-f")
	{ 
	    @cmd_args = ("generic", $fn, $_, $force_param);
	}
	else
	{
	    @cmd_args = ("generic", "Start function", $_);
	}
	print "cmd_args: @cmd_args\n";
	system @cmd_args;
    }
}

sub mirrorem
{
    my @groups;
    my $force_param;
    my $fn;
    my @cmd_args;

    if ($Action eq "off")
    {
	$fn = "Mirror off function";
    }
    else
    {
	$fn = "Mirror on function";
    }
    if ($Grps eq "-a")
    {
	print "Do something to get all the groups.\n";
	print "Faking it by setting all groups to be 1 2 3\n";
	$Grps = "1 2 3";
    }
	
    for (split ' ', $Grps)
    {
	@cmd_args = ("generic", $fn, $_);
	system @cmd_args;
    }
}

sub checkpointem
{
    my @params = @_;
}
sub process_simple_cmds
{
    my @params = @_;
}
sub setgroupvals
{
    my @params = @_;
}
sub setdevvals
{
    my @params = @_;
}
sub getgroupvals
{
    my @params = @_;
}
sub getdevvals
{
    my @params = @_;
}

#
# A little variable initialization
#
$simple_cmds = "(chart|config|monitor|update|backfresh|refresh)";
$word_args = 0;
$_ = $ARGV[0];
$Cmd = $_;
shift; 

#
# Put the command line into one string
#
$cmdline = "";
while ($ARGV[0])
{
    $cmdline = $cmdline . " " . $ARGV[0];
    shift @ARGV;
}

#
# Perform pattern matches on the various parts of the string
# (as opposed to the individual "words" in it.
#
$_ = $Cmd;
SWITCH: {
    /mirror/o && do {
	get_expr_args ("([\\d+\\s+]+|-a)\\s*(on|off)\$");
	mirrorem ();
	last SWITCH;
    };
    /start|stop/o && do {
	get_expr_args ("([\\d+\\s+]+|-a)(?:\\s+(-f))?\$");
	stopstartem ();
	last SWITCH;
    };

    /checkpoint/o && do {
	get_expr_args ("([\\d+\\s+]+|-a)\\s+-c");
	checkpointem ();
	last SWITCH;
    };

    /$simple_cmds/o && do {
	print "Simple cmd, match: $MATCH\n";
	process_simple_cmds();
	last SWITCH;
    };
    /set/o && do {
	get_expr_args ("(group|device)\\s*([\\d+\\s+]+|-a)\\s*((?:\\w+\\s*=\\s*\\w+\\s*)+)");
	setgroupvals ();
	last SWITCH;
    };

    /info/o && do {
	print "cmdline: \"$cmdline\"\n";
	if ($cmdline =~ /^\s*control\s+.*/)
	{
	    $cmdline =~ s/^\s*control\s+//;
	    get_expr_args ("(.*)");
	}
	else
	{
	    get_expr_args ("(group|device)\\s*([\\d+\\s+]+|-a)\\s*(.*)");
	}
	last SWITCH;
    };
    #
    # No match found. Print error and bail out.
    #
    usage;
}

#
# Now handle the various commands and make sure that they are
# handled properly.
#


print "Command: $Cmd\n";
print "Groups:  $Grps\n";
print "Action:  $Action\n";
print "Val_list: $Val_list\n";

exit;
