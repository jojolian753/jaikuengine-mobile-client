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

use test_scripts;

print "struct TBTScriptItem { const TUint16* const node; TInt minute; const TUint8* const bt; TInt aMsgSuccess; const TUint16* const node2; const TUint16* const scriptfile; };\n";
print "static const TBTScriptItem KBTScriptTestCalls[] = {\n";
foreach my $d (@msgtest) {
	my $node=$d->[0];
	if ($node eq  "") {
		print "\t{ L\"\", 0, 0, 0, 0, 0 }, \n";
		next;
	}
	my $min=$d->[1];
	my $bt=$d->[2];
	print "\t{ L\"$node\", $min, (const TUint8*)\"";
	for ($i=0; $i<12; $i+=2) {
		print "\\x";
		print substr($bt, $i, 2);
	}
	print "\", $d->[3] ";
	print ", L\"", $d->[4], "\"";
	print ", L\"", $d->[5], "\"";
	print "}, \n";
}
print "\t{ 0, 0, 0, 0, 0, 0 }\n";
print "};\n\n";

print "static const TBTTestItem KBTScriptTestBt[] = {\n";

foreach my $d (@testdata) {
	my $node=$d->[0];
	my $min=$d->[1];
	foreach my $bt ( @{$d->[2]} ) {
		my $success=0;
		if ($bt eq $d->[3]) {
			$success=$d->[4];
		}
		print "\t{ L\"$node\", $min, (const TUint8*)\"";
		for ($i=0; $i<12; $i+=2) {
			print "\\x";
			print substr($bt, $i, 2);
		}
		print "\", $success ";
		print "}, \n";
	}
}
print "\t{ 0, 0, 0 }\n";
print "};\n\n";
