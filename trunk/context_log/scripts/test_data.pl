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

@files=<log*.txt>;
$DEFAULT_NW="RADIOLINJA";

sub twonum($)
{
	my $n=$_[0];
	if ($n<10) { return "0" . $n; }
	return $n;
}

sub read_data
{
	my $count=0;
	foreach $file (@files) {
		open(IN, "<$file");
		while (<IN>) {
			next unless (/location.value/);
			next if (/STOPPING/);
			chop; chop;
			s/ STARTING//;
			/^([^ ]*) location.value: *(.*)/;
			($datetime, $id)=($1, $2);
			($mcc, $mnc, $nw, $lac, $cell)=split(/, /, $id);
			$id="$lac, $cell, RADIOLINJA";
			$datetime=~/(....)(..)(..)T(..)(..)(..)/;
			$datetime=$1 . twonum($2-1) . twonum($3-1) 
				. ":" . $4 . $5 . $6;
			push(@data, [ $datetime, $id] );
			$count++;
		}
		push(@data, [ $datetime, "SWITCH" ]);
	}
}

&read_data;

$real_count=$#data+1;
$compile_count=$#data;
if ($#data>9999) {
	$compile_count=9999;
}

open(OUT, ">bases_test_data.h") || die "cannot open bases_test_data.h";
print OUT "#define TEST_DATA_COUNT ", $compile_count+1, "\n";
print OUT "#define AVAILABLE_DATA_COUNT ", $real_count, "\n";
close(OUT);

open(OUT, ">bases_test_data.cpp") || die "cannot open bases_test_data.cpp";
open(OUT2, ">bases_test_data.txt") || die "cannot open bases_test_data.txt";
print OUT "#include \"bases.h\"\n";
print OUT "const unsigned short* bases::test_data[", $#data+1, "] [2] = {\n";
$i=0;
foreach $d (@data) {
	($datetime, $id) = @{$d};
	if ($i<=$compile_count) {
		print OUT "\t{ L\"", $datetime, "\", L\"", $id, "\" },\n";
	}
	print OUT2 $datetime, "\t", $id, "\n";
	$i++;
}

print OUT "};\n";
close(OUT);
close(OUT2);
