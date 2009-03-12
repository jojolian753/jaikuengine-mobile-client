# Copyright (c) 2007-2009 Google Inc.
# Copyright (c) 2006-2007 Jaiku Ltd.
# Copyright (c) 2002-2006 Mika Raento and Renaud Petit
#
# This software is licensed at your choice under either 1 or 2 below.
#
# 1. MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# 2. Gnu General Public license 2.0
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
# This file is part of the JaikuEngine mobile client.

$prev_error=-1;
$run_i=0;

if ($#ARGV==4) {
	$type=pop(@ARGV);
}

$project=pop(@ARGV);
$build=shift @ARGV;
$platform=join(" ", @ARGV);

$done_makefile=0;
$force_rerun=0;
@cmds=();
$|=1;
$v3=0;

if ($ENV{'EPOCROOT'} =~ /3rd/i) {
	$v3=1;
	$type="dll" if (lc($type) eq "epocexe");
}

# don't meddle with factory exports
$type="factorydll" if ($project=~/factory/i || $project=~/plugin/i);

if (lc($type) eq "dll" && $platform=~/wins/i) {
	# don't freeze on wins
	$type="unfrozen";
}

sub defname() {
	open(IN, "<bld.inf") || die "cannot open bld.inf";
	my $mmp="";
	while(<IN>) {
		chop;
		s/\r//;
		$proj=$_;
		$proj=~s/.*\\([^.]+)\.mmp/$1/;
		if (lc($proj) eq lc($project)) {
			$mmp=$_;
			last;
		}
	}
	close(IN);
	if ($mmp eq "") {
		die "cannot find $project in bld.inf";
	}

	$mmp =~ s/\.mmp//;
	my $pl=$platform;
	if ($pl=~/gcce/i) {
		$pl="THUMB";
	}
	$def = $mmp . "-" . $pl . "u.def";

	if (-f $def) { return $def };
	my $sdk=$ENV{'EPOCROOT'};
	$sdk=~s/.*\\([^\\]*)\\$/$1/;

	$mmp=~/(.*\\)(\w+)/;
	$def=$1 . $sdk . "\\" . $2 . "-" . $pl . "u.def";
	return $def;
}


while($run_i < 10) {
	$run_i++;
	if ( $#cmds == -1) {
		$cmd="abld.bat $build $platform $project";
		$force_rerun=0;
	} else {
		$c=shift @cmds;
		$cmd="abld.bat $c $platform $project";
		unless ($c eq "target") {
			$cmd =~ s/U(DEB|REL)//i;
		}
		if ($c eq "freeze") {
			print "***SLEEPING to ensure correct DEF file timestamp\n";
			sleep 2;
		}
	}
	if ($type eq "") { print "***RUNNING $cmd\n\n";}
	else { print "***RUNNING $cmd WITH PROJECTTYPE $type\n\n"; }

	open(RES, "$cmd 2>&1 |");
	$error=0;
	$def="";
	while(<RES>) {
		print $_;
		if ( / file.*(WINS|THUMB|WINSCW)' not found/ ) {
			$error=17;
		}
		if ( / No rule to.*(THUMB'|WINSCW')/ ) {
			$error=17;
		}
		if ( lc($type) eq "dll" && $build=~/target/i ) {
			if ($def eq "" && ( $error==4 || $error==2 || $error==14) ) {
				$def=$_;
				$def=~s/^ *([^ (]+)\(.*/$1/;
				$def=~s/^.*\\SYMBIAN/\\SYMBIAN/i;
				push(@defs, $def);
			}
			$error=15 if (/don't know how to.*DEF/);
			if (! $error ) {
				$error=2 if (/export.*not yet Froze/);
				$def="";
			}
			if (! $error ) {
				$error=3 if (/Not attempting to create/);
			}
			if ( /Ordinal not specified in sequence/) {
				$def=$_;
				$def=~s/^EFREEZE ERROR: //;
				$def=~s/^([^ (]+)\([0-9]+\) *: .*/$1/;
				$def=~s/^([^ ]+): .*/$1/;
				$def=~s/^.*\\SYMBIAN/\\SYMBIAN/i;
				push(@defs, $def);
				$error=4;
			}
			if (/The system cannot find the file specified/ && (! -f defname()) ) {
				$error=19;
			}
			if (/missing from object files/) {
				$def="";
				$error=14;
			}
			if (/Missing from ELF File/ ) {
				$def=defname();
				push(@defs, $def);
				$error=18;
			}
			if ( /When exports are frozen in/) {
				$def=$_;
				$def=~s/^[^"]*"//;
				$def=~s/".*$//;
				push(@defs, $def);
				$error=16;
			}
		}
		$error=1 if ($error<10 && $error != 4 && $error!=2 && /fatal error/i);
		$error=1 if ($error<10 && $error != 4 && $error!=2 && /^make.*Error/);
		$error=1 if ( /This project does not support/);
	}

	close(RES);
	if (!$force_rerun && $prev_error && $prev_error==$error) {
		print "LOGIC ERROR IN run_abld.pl!\n";
		exit 5;
	}
	$cmd="";
		
	$prev_error=$error;

	if ( ($#cmds==-1 && $error==0) || $error==1) {
		#print "***EXITING with status $error\n";
		exit $error;
	}

	if ($error==4 || $error==14) {
		foreach my $def (@defs) {
			print "***RESETING old DEFFILE $def\n";
			open(EXP, ">$def") || die "cannot open $def";
			print EXP "EXPORTS\n";
			close(EXP);
		}
		@cmds=();
		push(@cmds, "freeze");
		push(@cmds, "target");
		push(@cmds, "library");
	}
	if ($error==3 || $error==2) {
		@cmds=();
		push(@cmds, "freeze");
		push(@cmds, "target");
		push(@cmds, "library");
	}
	if ($error==17) {
		@cmds=();
		push(@cmds, "makefile");
		push(@cmds, "target");
	}
	if ($error==16) {
		@cmds=();
		if (! $v3 ) {
			print "***SLEEPING to ensure correct DEF file timestamp\n";
			sleep 2;
			foreach my $def (@defs) {
				print "***CREATING DEFFILE $def\n";
				my $dir=$def;
				$dir=~s/\\[^\\]*$//;
				system("mkdir $dir") unless (-d $dir );
				open(EXP, ">$def") || die "cannot open $def";
				print EXP "EXPORTS\n";
				close(EXP);
			}
			push(@cmds, "makefile");
			push(@cmds, "target");
		} else {
			push(@cmds, "freeze");
			push(@cmds, "makefile");
			push(@cmds, "target");
		}
	}
	if ($error==19) {
		push(@cmds, "makefile");
		push(@cmds, "target");
	}
	if ($error==18) {
		foreach my $def (@defs) {
			print "del $def\n";
			system("del $def");
		}
		push(@cmds, "makefile");
		push(@cmds, "target");
		push(@cmds, "freeze");
		push(@cmds, "makefile");
		push(@cmds, "target");
	}
}

print "***LOGIC ERROR in run_abld.pl: more than 10 iterations\n";
exit 2;
