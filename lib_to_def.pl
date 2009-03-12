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

$epocroot=$ENV{'EPOCROOT'};
die "EPOCROOT must be set" if ($epocroot eq "");

$lib=$ARGV[0];
die "Usage:\nlib_to_def.pl FILE" if ($lib eq "");

if ($lib=~/thumb/i) {
	$nm=$epocroot . "epoc32\\gcc\\bin\\nm.exe";
	$objdump=$epocroot . "epoc32\\gcc\\bin\\objdump.exe";

	open(NM, "$nm ${epocroot}epoc32\\release\\$lib|");
	while (<NM>) {
		chop;
		s/\r//;
		#C:/DOCUME~1/mraento/LOCALS~1/Temp/d1000s_00001.o:
		if (/[^\d](\d+)\.o:$/) {
			$ordinal=int($1);
			next;
		}
		if (/^0+ T (.*)/) {
			$export=$1;
			$exports{$ordinal}=$export;
		}
	}
	close(NM) || die "failed to run $nm";

	open(OBJDUMP, "$objdump --section=.idata\$7 --full-contents ${epocroot}epoc32\\release\\$lib|");
	$seen_contents=0;
	$dllname="";
	while (<OBJDUMP>) {
		chop;
		s/\r//;
		if ($seen_contents) {
			last if (/^\s*$/);
			print STDERR "failed to parse line $_\n" 
				unless(/^.*\s+([^\s]+)\s*$/);
			$dllname .= $1;
		}
		if (/Contents of section/) {
			$seen_contents=1;
		}
	}
	close(OBJDUMP) || die "failed to run objdump.exe";
	die "failed to read name and uid of dll" if ($dllname eq "");
	$dllname=~s/\]\.DLL.*/].DLL/i;

	open(OBJDUMP2, "$objdump --full-contents ${epocroot}epoc32\\release\\$lib|");
	$seen_contents=0;
	while (<OBJDUMP2>) {
		chop;
		s/\r//;
		if (/[^\d](\d+)\.o:\s*file format/) {
			$ordinal=int($1);
			$seen_contents=0;
			next;
		}
		if ($seen_contents) {
			if (/\.K\.h\.G\.F/) {
				$r3unused{$ordinal}=" R3UNUSED";
			}
			$seen_contents=0;
			next;
		}
		if (/Contents of section \.text/) {
			$seen_contents=1;
		}
	}
	close(OBJDUMP2) || die "failed to run objdump.exe";

} else {
	$cmd="dumpbin /exports ${epocroot}epoc32\\release\\$lib|";
	open(NM, $cmd);
	$seen_ordinal=0;
	while (<NM>) {
		chop;
		s/\r//;
		if ($seen_ordinal) {
			next unless (/^\s*(\d+)\s+([^\s]+)\s/);
			$exports{$1}=$2;
			next;
		}
		$seen_ordinal=1 if (/ordinal/i);
	}
	close(NM) || die "failed to run $cmd";
	
	$lib=~s/\.LIB/.DLL/i;
	$cmd="dumpbin /section:.E32_UID /rawdata:8 ${epocroot}epoc32\\release\\$lib|";
	open(OBJDUMP, $cmd);
	$seen_rawdata=0;
	while (<OBJDUMP>) {
		chop;
		s/\r//;
		if ($seen_rawdata) {
			die "cannot parse line '$_'" unless(/^\s*[\da-f]+:\s+[\da-f]+\s+([\da-f]+)/i);
			$raw=$1;
			#print "RAW $raw";
			$raw=substr($raw, length($raw)-8);
			last;
		}
		$seen_rawdata=1 if (/RAW DATA/);
	}
	$dllname=$lib;
	$dllname=~s/^.*\\([^\\]+)$/$1/;
	$dllname=~s/\.DLL//i;
	$dllname .= "[" . $raw . "].DLL";
	close(OBJDUMP) || die "$cmd";
}

print ";DLLNAME $dllname\n";
print "EXPORTS\n";
foreach my $ordinal (sort { $a <=> $b } keys %exports) {
	$export=$exports{$ordinal};
	if ($lib=~/thumb/i) {
		print "\t\"$export\" @ $ordinal NONAME" . $r3unused{$ordinal} . "\n";
	} else {
		print "\t$export @ $ordinal NONAME\n";
	}
}
