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

$PKG=$ARGV[0];
die "usage: single_pkg.pl PKGFILE" if ($PKG eq "");
	
open(MAIN, "<$PKG") || die "cannot open $PKG";

$out=$PKG;
$out=~s/\.pkg/_all.pkg/;
$out_dbg=$out;
$out_dbg=~s/_all/_dbg/;
$out_armv5=$out;
$out_armv5=~s/_all/_armv5/;

open(OUT, ">$out") || die "cannot open $out";
open(OUTDBG, ">" . $out_dbg) || die "cannot open $out_dbg";
open(ARMV5, ">" . $out_armv5) || die "cannot open $out_armv5";
while(<MAIN>) {
	unless(/^@/) {
		if (/EN, *FI/) {
			s/, *FI//;
		}
		s/\{"([^"]+)", "[^"]*"\}/{"\1"}/;
		s/^[{}]//g;
		s/!:/c:/gi;
		print OUT $_;
		s/urel/udeb/gi;
		print OUTDBG $_;
		s/udeb/urel/gi;
		s/gcce/armv5/gi;
		print ARMV5 $_;
		next;
	}
	if (/mobinfo/i) {
		print OUT $_;
		s/urel/udeb/gi;
		print OUTDBG $_;
		s/udeb/urel/gi;
		s/gcce/armv5/gi;
		print ARMV5 $_;
		next;
	}
	/^@"([^"]*)"/;
	$pkg=$1;
	$pkg=~s/SIS/pkg/i;
	if (-f $pkg) {
		my $gcce=0;
		$gcce=1 if ($pkg=~/pys60/i);
		$gcce=1 if ($pkg=~/python/i);
		$dir=$pkg;
		$dir=~s/\\[^\\]+$/\\/;
		open(IN, "<$pkg") || die "cannot open $pkg";
		while(<IN>) {
			s/\r//;
			s/\n//;
			next if (/^\[/);
			if (/EN, *FI/) {
				s/, *FI//;
			}
			s/\{"([^"]+)", "[^"]*"\}/{"\1"}/;
			s/^[{}]//g;
			next if (/\.r09"/);
			next if (/^\s*$/);
			next if (/^\&/);
			next if (/^#/);
			next if (/^;/);
			next if (/^\s*$/);
			next if (/^\(/);
			next if (/^:/);
			next if (/^%/);

			if (/^\s*"[^\\]/) {
				s/"/"$dir/;
			}
			s/!:/c:/gi;
			print OUT $_, "\n";
			s/urel/udeb/gi;
			print OUTDBG $_, "\n";
			s/udeb/urel/gi;
			s/gcce/armv5/gi unless($gcce);
			print ARMV5 $_, "\n";
		}
		close(IN);
	} else {
		print OUT $_;
		s/urel/udeb/gi;
		print OUTDBG $_, "\n";
		s/udeb/urel/gi;
		s/gcce/armv5/gi;
		print ARMV5 $_, "\n";
	}
}
close(OUT);
close(OUTDBG);
