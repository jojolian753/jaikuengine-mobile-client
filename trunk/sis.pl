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

use Cwd;

open(BLDINF, "<bld.inf") || die "cannot open bld.inf";
$rootdir=getcwd;

my $SDK=$ENV{'EPOCROOT'};
$SDK=~s/.*\\([^\\]*)\\$/$1/;
my $v3=0;
$v3=1 if ($SDK=~/3rd/i);

while(<BLDINF>) {
	next if (m!^//!);
	next unless (/mmp$/i);
	if ($#ARGV>-1) {
		next unless (/$ARGV[0]/i);
	}
	
	chop; s///;
	$mmp=$_;

	chdir($rootdir) || die "cannot chdir to $rootdir";
	/^(.*)\\([^\\]*).mmp$/i;
	($path, $file)=($1, $2);
	$file .= "_" . $SDK . ".pkg";
	if ($v3) {
		my $gv3=$path;
		$gv3=~s/group/gv3/i;
		$gv3=~s/gv2/gv3/i;
		my $f=$gv3 . "\\" . $file;
		$path=$gv3 if (-f $f);
		#print "GV3 $gv3 $path $f\n";
	}

	chdir($path) || die "cannot chdir to $path";

	$buildsis=$file;
	$buildsis=~s/pkg/SIS/;
	$diffsis=$buildsis;
	$diffsis=~s/SIS/OLD/;
	if (-f $buildsis ) {
		system("copy $buildsis $diffsis");
		$ret=$? >> 8;
		if ($ret!=0) {
			exit $ret;
		}
	}

	$cmd="makesis $file";
	open(RES, "$cmd 2>&1 |");
	while(<RES>) {
		print $_;
		$error=1 if (/error/i);
	}
	close(RES);
	exit $error if ($error);

	system("diff --binary $buildsis $diffsis");
	if ($? == -1) {
		print "cannot run DIFF!\n";
		exit 1;
	}
	$ret=$? >> 8;
	if ($ret!=0) {
		print "\n";
		chdir($rootdir) || die "cannot chdir to $rootdir";
		chdir("..");
		system("perl update_version.pl $rootdir\\$path\\$file version.txt");
		if ($? == -1 || (($? >> 8) != 0) ) {
			print "failed to update version!\n",
			exit 1;
		}
		chdir($rootdir) || die "cannot chdir to $rootdir";
		chdir($path) || die "cannot chdir to $path";
	}

	$cmd="makesis $file";
	open(RES, "$cmd 2>&1 |");
	while(<RES>) {
		print $_;
		$error=1 if (/error/i);
	}
	close(RES);
	exit $error if ($error);
}

close(BLDINF);

exit 0;
