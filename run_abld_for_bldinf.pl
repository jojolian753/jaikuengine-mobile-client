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

sub usage() {
	print "Usage:\n\n";
	print "run_abld_for_bldinf.pl PLATFORM RELEASE\n";
	exit 1;
}

if ($#ARGV != 1) {
	usage();
}

my $release=$ARGV[$#ARGV];
my $plat=$ARGV[$#ARGV-1];

if ($ENV{'EPOCROOT'}=~/3rd/i && lc($plat) eq "thumb") {
	$plat="gcce";
}

use Cwd;

use lib '..';
require 'building.pl';
$platform=$plat;

my $fhnum=0;

sub make_sure_deffile_exists($) {
	my $def=defname(shift);
	print "DEFFILE: $def\n";
	unless ( -f $def ) { 
		open(DEF, ">$def") || die "cannot open $def";
		print DEF "EXPORTS\n";
		close(DEF);
	}
}

sub defname($) {

	my $mmp=mmp(shift);
	$mmp =~ s/\.mmp//;
	my $pl=$platform;
	if ($pl=~/gcce/i) {
		$pl="THUMB";
	}
	$mmp .= "-" . $pl . "u.def";

	return $mmp;
}

open(BLDINF, "<bld.inf") || die "cannot open bld.inf";
while(<BLDINF>) {
	next if (m!^//!);
	next unless (/mmp$/i || /mk$/i);

	next if ( $platform=~/wins/i && /autostart/i);
	
	chop; s///;
	my $mmp=$_;
	$|=1;

	my ($path, $file);
	my $mmpchanged; 
	my $dependschanged;
	my ($depends, $type, $has_resource, $sourcefiles, $dummy, $includedirs, $dummy2);
	my $ismk = 0;
	if (/mk$/i) {
	  $ismk = 1;
	  /^gnumakefile (.*)\\([^\\]*).mk$/i;
	  ($path, $file)=($1, $2);
	  ($mmpchanged, $dependschanged, $has_resource) = (1,1,1);
	}
	else {
	  /^(.*)\\([^\\]*).mmp$/i;
	  ($path, $file)=($1, $2);
	  ($depends, $type, $has_resource, $sourcefiles, $dummy, $includedirs, $dummy2)=
	    getdepends(mmp($file), mmp($file), "", 0);
	  $?=0;
	  $mmpchanged=haschanged($file, $depends);
	  if ($mmpchanged) { print "MMP file (or file included from MMP) changed\n"; }
	  $dependschanged=depchanged($file, $sourcefiles, $includedirs);
	  if ( lc($type) eq dll ) {
	    make_sure_deffile_exists($file);
	  }
	}

	if ($?==0 && $ismk ) {
	  system("perl ..\\run_abld.pl build $platform $file");
	  if ($?!=0) { die "building gnumakefile $file failed\n"; }
	  else { next; }
	}

	if ($?==0 && ($mmpchanged || $dependschanged) ) {
	  print "$file\n";
	  open(TOUCH, ">>" . mmp_or_mk($file)) || die "cannot open mmp for touching";
	  close(TOUCH);
	  system("perl ..\\run_abld.pl makefile $platform $file");
	}
	if ($?==0 && 0 && $has_resource) {
		# this doesn't seem to be necessary? gets created with 'target'
		system("perl ..\\run_abld.pl resource $file");
	}

	if ($? ==0 ) { system("perl ..\\run_abld.pl target $platform $release $file $type"); }
	if ($? == -1) {
		print "failed to execute: $!\n";
		exit 1;
	} else {
		my $ret= $? >> 8;
		if ($ret != 0) {
			printf "child exited with value %d\n", $ret;
			exit $ret;
		}
	}
}

close(BLDINF);

exit 0;
