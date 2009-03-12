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

$num=$ARGV[0];

open(OUTH, ">routes_test_data.h");
open(OUTC, ">routes_test_data.cpp");

open(IN, "<data${num}.txt");
while(<IN>) {
	s///;
	chop;
	push(@data, $_);
}
close(IN);

open(IN, "<base${num}.txt");
while(<IN>) {
	s///;
	chop;
	push(@base, $_);
}
close(IN);

open(IN, "<name${num}.txt");
while(<IN>) {
	s///;
	chop;
	push(@name, "(unsigned short*)L\"$_\"");
}
close(IN);

open(IN, "<time${num}.txt");
while(<IN>) {
	s///;
	chop;
	push(@time, $_);
}
close(IN);
	
open(IN, "<area${num}.txt");
while(<IN>) {
	s///;
	chop;
	if (/level ([0-9])/) {
		$level=$1;
		$a=1;
	} else {
		foreach $c (split) {
			push(@area, "{ " . $level . ", " . $a . ", " . $c . "}");
		}
		$a++;
	}
}
close(IN);

print OUTH "#ifndef ROUTES_TEST_DATA_H_INCLUDED\n";
print OUTH "#define ROUTES_TEST_DATA_H_INCLUDED 1\n\n";
print OUTH "#define DATA_COUNT ", $#data+1, "\n";
print OUTH "#define BASE_COUNT ", $#base+1, "\n";
print OUTH "#define AREA_COUNT ", $#area+1, "\n";
print OUTH "\n#endif //ROUTES_TEST_DATA_H_INCLUDED\n";

close(OUTH);

print OUTC "#include \"routes_test.h\"\n\n";
print OUTC "const unsigned short routes_test::data[DATA_COUNT] = {\n";
print OUTC join(",\n", @data);
print OUTC "};\n\n";

print OUTC "const int routes_test::time[DATA_COUNT] = {\n";
print OUTC join(",\n", @time);
print OUTC "};\n\n";

print OUTC "const unsigned short routes_test::area[AREA_COUNT][3] = {\n";
print OUTC join(",\n", @area);
print OUTC "};\n\n";

print OUTC "const unsigned short routes_test::base[BASE_COUNT] = {\n";
print OUTC join(",\n", @base);
print OUTC "};\n\n";

print OUTC "const unsigned short * const routes_test::name[BASE_COUNT] = {\n";
print OUTC join(",\n", @name);
print OUTC "};\n\n";

close(OUTC);
