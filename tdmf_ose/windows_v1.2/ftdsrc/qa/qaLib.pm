package qaLib;
require Exporter;

=head1 NAME

DataStar Project QA Perl Library:

     GetStdDeviceGeometry - return geometry info for a standard disk partition
     DetermineqdsLogicalGroup - return the logical group number that contains a qds device

=head1 SYNOPSIS

     use qaLib;
     ($sliceStartSector, $sliceEndSector) = GetStdDeviceGeometry("/dev/rdsk/c0tt1d0s4");
     $logicalGroupNum = DetermineqdsLogicalGroup("/dev/rdsk/qds3");

=head1 DESCRIPTION


=cut

@ISA = qw(Exporter);
@EXPORT = qw(GetStdDeviceGeometry
			 GetqdsDeviceGeometry
			 CheckForDeviceMounted
			 DetermineqdsLogicalGroup
			 ParseCfgFile
			 GatherqdsStats
			 uSleep
			 GetMaxPhys);


########################################
# Package constants
#
$MOUNT				= '/usr/sbin/mount';
$ADB				= '/usr/bin/adb';
$DEVINFO			= '/usr/sbin/devinfo';
$QDSDEVINFO			= '/opt/QLIXds/bin/qdsdevinfo';
$QDSConfFileTmplt	= '/etc/opt/QLIXds/dsgrp%03d.cfg';
$QDSStatFileTmplt	= '/var/opt/QLIXds/dsgrp%03d.prf';
$statisticsRegExp	= "%s\\s+" . ('([0-9.]+)\s+' x 3) . ('([0-9]+)\s+' x 3) . '([0-9]+)(\s|$)';


# Name a list of the names of the qds stats we collect
@qdsStatNames = ('Kbps',
			  'Data Kbps',
			  'Xfers/Sec',
			  'WL Entries',
			  'WL Sectors',
			  'WL %',
			  'Oldest Entry');

# Get the first and last sectors of a disk device, in an array context.
# Just its size in a scalar context.
#
sub GetStdDeviceGeometry {
	my($device) = @_;
	my($ERRFILE, $stdDeviceStart, $stdDeviceSize) = ("/tmp/qaLib$$", 0, 0);

	open(DEVINFO, "$DEVINFO -p $device 2>$ERRFILE |") || die "$DEVINFO: $!";
	while (<DEVINFO>) {
		($stdDeviceStart, $stdDeviceSize) = m|^/dev/\S+\s+[0-9a-fA-F]+\s+[0-9a-fA-F]+\s+([0-9]+)\s+([0-9]+)\s|;
	}
	close DEVINFO;


	if ($? != 0) {
		open(ERR, $ERRFILE);
		while (<ERR>) {
			print;
		}
		close ERR;
		unlink($ERRFILE);
		return ();
	}
	unlink($ERRFILE);
	wantarray ? ($stdDeviceStart, $stdDeviceSize + $stdDeviceStart - 1) : $stdDeviceSize;
}

# Get the first and last sectors of a qds device, in an array context.
# Just its size in a scalar context.
#
sub GetqdsDeviceGeometry {
	my($device) = @_;

	my $lg = ParseCfgFile(sprintf($QDSConfFileTmplt, DetermineqdsLogicalGroup($device)));
	for $profileRef ( @{$lg->{PROFILES}} ) {
		next unless $profileRef;
		return GetStdDeviceGeometry($profileRef->{DATADISK})
			if $profileRef->{DATASTARDEVICE} eq $device;
	}
}

sub CheckForDeviceMounted {
	my($device) = @_;

	open(MOUNT, "$MOUNT |") || die "$MOUNT: $!";
	while (<MOUNT>) {
		next if (! m/$device/o);
		my($mountPoint) = m/(^\S+)\s/;
		print STDERR "$device appears to be mounted on $mountPoint.\n";
		die "Can't operate on a mounted device.  Stopping"
	}
	close MOUNT;

	if ($? != 0) {
		printf "mount returned %d\n", $?;
		die "$MOUNT failed";
	}
}

sub DetermineqdsLogicalGroup {
	my($deviceName) = @_;
	my $lg = -1;
	my $i;

	return $devLG{$deviceName}
		if defined($devLG{$deviceName});

	for ($i = 0; $lg == -1 && $i < 999; $i++) {
		my $CONFFILE = sprintf($QDSConfFileTmplt, $i);
		next unless stat($CONFFILE);
		open(CONFFILE, $CONFFILE) || do {
			print STDERR "Warning: Could not open $CONFFILE: $!\n";
			next;
		};
		while (<CONFFILE>) {
			if (m|DATASTAR-DEVICE:\s*$deviceName\s*$|o) {
				$lg = $i;
				last;
			}
		}
		close CONFFILE;
	}
	die "Failed to determine logical group for device $deviceName. Stopped"
		unless ($lg != -1);
	$devLG{$deviceName} = $lg;
	$lg;
}


# The following is an example of a parsed logical group structure:
#
# $lg = {
# 	NOTES			=> "Logical Group Notes"
# 	CHUNKSIZE		=> 1024
# 	NODATASLEEP		=> 500
# 	SYNCINTERVAL	=> 10
# 	SYNCCOUNT		=> 100
# 	CHUNKDELAY		=> 0
# 	DEFAULTRMDPORT	=> 525
# 	STATINTERVAL	=> 10
# 	MAXSTATFILESIZE	=> 64
# 	LOGSTATS		=> "Y"
# 	TRACETHROTTLE	=> "N"
# 	PRIMARY			=> {
# 		SYSTEMTAG		=> "SYSTEM-A",
# 		HOST			=> "ultra2",
# 		HOSTID			=> 0x80825e93,
# 		IP				=> "208.206.246.9",
#		WLEXTENSIONS	=> [
#			{
#               DEVICE      => "/dev/rdsk/c3t1d0s0",
# 				STARTSECTOR => 0,
# 				ENDSECTOR   => 125678,
# 			},
# 			{
#               DEVICE      => "/dev/rdsk/c3t1d0s1",
# 				STARTSECTOR => 0,
# 				ENDSECTOR   => 125678,
# 			}
# 		]
# 	},
# 	SECONDARY		=> {
# 		SYSTEMTAG		=> "SYSTEM-B",
# 		HOST			=> "safedata-b",
# 		HOSTID			=> 0x807a98ab,
# 		IP				=> "208.206.246.5",
# 	},
#
# 	PROFILES		=> [
# 		{
# 			REMARK			=> "",
# 			PRIMARY			=> \*primary,
# 			DATASTARDEVICE	=> "/dev/rdsk/qds0",
# 			DATADISK		=> "/dev/rdsk/c1t1d0s1",
# 			WRITELOG		=> [
# 				"/dev/rdsk/c1t1d0s6"
# 			],
# 			SECONDARY		=> \*secondary,
# 			MIRRORDISK		=> "/dev/rdsk/c0t2d0s4",
# 		},
# 	],
#
#	THROTTLES		=> [
#		{
#			STARTTIME		=> "-",
#			ENDTIME			=> "-",
#			PARAM			=> "pctwl",
#			OPER			=> "T>=",
#			VALUE			=> 80,
#			ACTIONS => [
#				"do addwl",
#				"do console Added writelog."
#			]
#		}
#	]
# };
#

sub ParseCfgFile {
	my $CFGFILE = shift;
	my $lg = { };

	open(CFGFILE, $CFGFILE)
		|| die "Could not open $CFGFILE: $!\n";

	while (<CFGFILE>) {
		chomp;				# preprocess the line. Chop off the newline, ...
		s/#.*$//;			# ...remove comments, ...
		s/[ \t]+/ /g;		# ...compress all strings of spaces and/or tabs to a single space, ...
		/^ *$/ && next;		# ...and skip blank lines.

		m|NOTES:( (.+))?$|					&& do { $lg->{NOTES} = "$2";         next; };
		m|CHUNKSIZE: ([0-9]+)$|				&& do { $lg->{CHUNKSIZE} = $1;     	 next; };
		m|NODATASLEEP: ([0-9]+)$|			&& do { $lg->{NODATASLEEP} = $1;   	 next; };
		m|SYNCINTERVAL: ([0-9]+)$|			&& do { $lg->{SYNCINTERVAL} = $1;  	 next; };
		m|SYNCCOUNT: ([0-9]+)$|				&& do { $lg->{SYNCCOUNT} = $1;     	 next; };
		m|CHUNKDELAY: ([0-9]+)$|			&& do { $lg->{CHUNKDELAY} = $1;    	 next; };
		m|DEFAULTRMDPORT: ([0-9]+)$|		&& do { $lg->{DEFAULTRMDPORT} = $1;	 next; };
		m|STATINTERVAL: ([0-9]+)$|			&& do { $lg->{STATINTERVAL} = $1;  	 next; };
		m|MAXSTATFILESIZE: ([0-9]+)$|		&& do { $lg->{MAXSTATFILESIZE} = $1; next; };
		m|LOGSTATS: ([YN])$|				&& do { $lg->{LOGSTATS} = $1;        next; };
		m|TRACETHROTTLE: ([YN])$|			&& do { $lg->{TRACETHROTTLE} = $1;   next; };

		m|SYSTEM-TAG: (\S+) PRIMARY| && do {
			$lg->{PRIMARY} = { SYSTEMTAG => $1, HOST => '', HOSTID => 0, IP => '', WLEXTENSIONS => [] };
			while (<CFGFILE>) {
				chomp;   s/#.*$//;   s/[ \t]+/ /g;   /^ *$/ && next;
				m|HOST: (\S+)|    && do { $lg->{PRIMARY}{HOST}   = $1; next; };
				m|HOSTID: (\S+)$| && do { $lg->{PRIMARY}{HOSTID} = $1; next; };
			    m|IP: (\S+)$|     && do { $lg->{PRIMARY}{IP}     = $1; next; };
				m|WRITELOG-EXTENSION: (\S+)( ([0-9]+) ([0-9]+))?$| && do {
					push(@{$lg->{PRIMARY}{WLEXTENSIONS}}, { DEVICE => $1, STARTSECTOR => $3, ENDSECTOR => $4 });
					next;
				};
				last;
			}
			last if eof(CFGFILE);
			redo;
		};

		m|SYSTEM-TAG: (\S+) SECONDARY| && do {
			$lg->{SECONDARY} = { SYSTEMTAG => $1, HOST => '', HOSTID => 0, IP => '' };
			while (<CFGFILE>) {
				chomp;   s/#.*$//;   s/[ \t]+/ /g;   /^ *$/ && next;
				m|HOST: (\S+)|    && do { $lg->{SECONDARY}{HOST}   = $1; next; };
				m|HOSTID: (\S+)$| && do { $lg->{SECONDARY}{HOSTID} = $1; next; };
			    m|IP: (\S+)$|     && do { $lg->{SECONDARY}{IP}     = $1; next; };
				last;
			}
			last if eof(CFGFILE);
			redo;
		};

		m|PROFILE: ([0-9]+)| && do {
			my $profile = $1;
			$lg->{PROFILES}[$profile] = { REMARK => '', PRIMARY => '', DATASTARDEVICE => '', 
										  DATADISK => '', WRITELOG => [], SECONDARY => '',
										  MIRRORDISK => ''};
			while (<CFGFILE>) {
				chomp;   s/#.*$//;   s/[ \t]+/ /g;   s/ $//;	/^ *$/ && next;
				m|REMARK:( (.*))?$|			&& do {
					$lg->{PROFILES}[$profile]{REMARK} = (defined($2) ? $2 : '');
					next;
				};
				m|PRIMARY: (\S+)$|			&& do {
					$lg->{PROFILES}[$profile]{PRIMARY} = $lg->{PRIMARY};
					next;
				};
				m|DATASTAR-DEVICE: (\S+)$|	&& do {
					$lg->{PROFILES}[$profile]{DATASTARDEVICE} = $1;
					next;
				};
				m|DATADISK: (\S+)$|			&& do {
					$lg->{PROFILES}[$profile]{DATADISK} = $1;
					next;
				};
				m|SECONDARY: (\S+)$|		&& do {
					$lg->{PROFILES}[$profile]{SECONDARY} = $lg->{SECONDARY};
					next;
				};
				m|MIRROR-DISK: (\S+)$|		&& do {
					$lg->{PROFILES}[$profile]{MIRRORDISK} = $1;
					next;
				};
				m|WRITELOG: (\S+)( ([0-9]+) ([0-9]+))?$| && do {
					push(@{$lg->{PROFILES}[$profile]{WRITELOG}}, { DEVICE => $1, STARTSECTOR => $3, ENDSECTOR => $4 });
					next;
				};
				m|WRITELOG: (\S+)$|			&& do {
					push(@{$lg->{PROFILES}[$profile]{WRITELOG}}, { DEVICE => $1, STARTSECTOR => '', ENDSECTOR => '' });
					next;
				};
				last;
			}
			last if eof(CFGFILE);
			redo;
		};

		m/THROTTLE ([0-9:]+|-) ([0-9:]+|-) ([a-z]+) ?([T=<>]+) ?([0-9.]+)$/ && do {
			my $throttref = { STARTTIME => $1, ENDTIME => $2, PARAM => $3, OPER => $4, VALUE => $5, ACTIONS => [] };
			if (<CFGFILE> !~ /ACTIONLIST/) {
				warn "Syntax error: ACTIONLIST expected after THROTTLE $1 $2 $3 $4 $5\n";
				return undef;
			}
			while (<CFGFILE>) {
				chomp;   s/#.*$//;   s/[ \t]+/ /g;   s/ $//;	/^ *$/ && next;
				m|ACTION: (.*)$| && do {
					push(@{$throttref->{ACTIONS}}, $1);
					next;
				};
				m|ENDACTIONLIST| && do { push(@{$lg->{THROTTLES}}, $throttref); $_ = <CFGFILE>; last; };
			}
			last if eof(CFGFILE);
			next;
		};

	}
	close CFGFILE;
	$lg;
}

sub GatherqdsStats {
	my($dev,$ino,$mode,$nlnk,$uid,$gid,$rdev,$sz);
	my $device = shift;
	my $lg = DetermineqdsLogicalGroup($device);
	my $QDSSTATFILE = sprintf($QDSStatFileTmplt, $lg);
	my @sample;

	if (! defined($statFileInode[$lg])) {

		# The first time this subroutine is called, we do not gather
		# statistics. Instead, we initialize the statistics gathering
		# machinery.
		#
		open(QDSSTATFILE, $QDSSTATFILE) || do {
			# We might have found the window of time during which the daemon is shuffling
			# the statistics file around.  Wait half-a-second and try again.  Failure this
			# time means death.
			#
			uSleep(500000);
			open(QDSSTATFILE, $QDSSTATFILE) || die "$QDSSTATFILE: Open failed: $!,";
		};

		(($dev,$statFileInode[$lg]) = stat(QDSSTATFILE)) || die "$QDSSTATFILE: stat failed: $!,";
		seek(QDSSTATFILE, 0, 2); # seek to the end of the file
		$qdsStatSamples = 0;
		foreach $statName (@qdsStatNames) {
			$sum{$statName} = 0;
			$max{$statName} = 0;
		}
		wantarray ? () : 1;
	}
	else {
		# The daemon may have closed the statistics file we have been looking at, renamed it,
		# and opened a new one.  If so, then the inode number that we have for the file will
		# be different than the one now associated with the name $QDSSTATFILE.
		#
		(($dev,$ino) = stat($QDSSTATFILE)) || do {
			# We might have found the window of time during which the daemon is shuffling
			# the statistics file around.  Wait half-a-second and try again.  Failure this
			# time means death.
			#
			print STDERR "Oops! Lost the stat file -- trying again.\n";
			uSleep(500000);
			(($dev,$ino) = stat($QDSSTATFILE)) || die "$QDSSTATFILE: stat failed: $!,";
		};


		if ($ino != $statFileInode[$lg]) {
			# The statistics file changed on us: close the file we have and open the new one.
			#
			close(QDSSTATFILE) || die "What the...? Close of $QDSSTATFILE failed: $!, ";
			open(QDSSTATFILE, $QDSSTATFILE) || die "$QDSSTATFILE: Open failed: $!,";
			(($dev,$statFileInode[$lg]) = stat(QDSSTATFILE)) || die "$QDSSTATFILE: stat failed: $!,";
		}

		# While there are more lines in the file...
		#
		while (<QDSSTATFILE>) {
			chop;
			my $re = sprintf($statisticsRegExp, $device);

			# Parse the set of stats for $device and save them away.
			#
			$qdsStatSamples++;
			@sample{@qdsStatNames} = m/$re/;
			foreach $statName (@qdsStatNames) {
				$sum{$statName} += $sample{$statName};
				$max{$statName} = $sample{$statName} if ($max{$statName} < $sample{$statName});
			}
		}

		# Got to end-of-file. Do a seek to clear the EOF flag.
		#
		seek(QDSSTATFILE, 0, 1);
	}
	wantarray ? @sample{@qdsStatNames} : 1;
}

sub uSleep {
	my($uSecs) = @_;
	select(undef, undef, undef, $uSecs / 1000000.0);
}

sub GetMaxPhys {
	my($maxPhys);

	open(ADB, "echo 'maxphys?D' | $ADB -k |")
		|| die "Couldn't get maxphys using adb: $!,";

	while (<ADB>) {
		m/^maxphys:\s+([0-9]+)$/ && ($maxPhys = $1);
	}
	close(ADB);
	$maxPhys;
}
