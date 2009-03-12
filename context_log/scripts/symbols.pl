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

$dllname=uc($ARGV[0]);
$libname=$dllname;
$libname=~s/\.dll/.lib/i;
$mapname=$dllname;
$mapname=~s/\.dll/.map/i;
$base=$dllname;
$base=~s/\.dll//i;

system("DUMPBIN /all $dllname > fl") && die "cannot DUMPBIN dll $dllname";
system("DUMPBIN /all $libname > fl2") && die "cannot DUMPBIN lib $libname";

open(DLL, "<fl") || die "cannot open fl";
open(LIB, "<fl2") || die "cannot open fl2";
open(OUT, ">$mapname") || die "cannot open $mapname";
open(TEXT, "<$dllname") || die "cannot open $dllname";
binmode TEXT;

@s=stat(TEXT);

#print "len $s[7]\n";
read TEXT, $img, $s[7];
#sysread TEXT, $img, $s[7];
#print "read len ", length($img), "\n";

while(<DLL>) {
	last if (/ordinal hint/);
}
while(<DLL>) {
	next if (/^\s*$/);
	last if (/SECTION/);

	($ord, $addr, $name)=split;
	$addr=hex($addr);
	$addr[$ord]=$addr;
}

while(<LIB>) {
	last if (/ordinal\s*name/);
}

while(<LIB>) {
	next if (/^\s*$/);
	last if (/Summary/);
	/^\s+([0-9]+)\s+[^ ]+ \([^(]* ([^ (]+)\(/;
	($ord, $name)=($1, $2);
	$name=$base . "::" . $name;
	$addr=$addr[$ord];
	next if ($addr<=0 || $name=~/^\s*$/);
	$code=substr($img, $addr, 5);
	#print "addr $addr code $code\n";
	($op, $jaddr)=unpack("Cl", $code);
	#printf "%x\n", $op;
	if ($op==0xe9) {
		print OUT 1, "\t"; 
		$jaddr+=$addr+5;
		$jaddr-=hex("1000");
		printf OUT "%x", $jaddr; 
		print OUT "\t", $name, "\n";
		$name.="JMP";
	}
	$addr-=hex("1000");
	print OUT 1, "\t"; 
	printf OUT "%x", $addr; 
	print OUT "\t", $name, "\n";
}

close(DLL);
close(LIB);
close(OUT);
