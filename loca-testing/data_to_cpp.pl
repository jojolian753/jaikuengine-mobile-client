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

use testdata;
use stream_to_stat;

print "struct TBTTestItem { const TUint16* const node; TInt minute; const TUint8* const bt; TInt aMsgSuccess; };\n";
print "static const TBTTestItem KBTTest[] = {\n";
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

print "struct TBTTestStats { 
	const TUint16* const node; 
	TInt minute; 
	const TUint8* const bt; 
	TInt count;
	TInt sum;
	TInt ssum;
	TInt last_seen;
	TInt last_seen_begin;
	TInt prev_seen;
	TInt prev_seen_begin;
	TInt first_seen;
	TInt max_span;
	TInt success_count;
	TInt failure_count;
	TInt local_success_count;
	TInt local_failure_count;
	TInt previous_local_success;
	TInt previous_local_failure;
	TInt previous_remote_success;
	TInt previous_remote_failure;
	};
";

print "static const TBTTestStats KBTTestStats[] = {\n";
$stats=stream_to_stat(\@testdata);
foreach my $d (@$stats) {
	my $node=$d->[3];
	my $min=$d->[0];
	my $bt=$d->[1];
	$line = "\t{\tL\"$node\", $min,
		(const TUint8*)\"";
	for ($i=0; $i<12; $i+=2) {
		$line .= "\\x";
		$line .= substr($bt, $i, 2);
	}
	$line .= "\",\n";
	$line .= "\t\t" . $d->[2]->{count} .  ",";
	$line .= " " . $d->[2]->{sum} . ",";
	$line .= " " . $d->[2]->{ssum} . ",";
	$line .= " " . $d->[2]->{last} . ",";
	$line .= " " . $d->[2]->{begin} . ",";
	my $prev_end=$d->[2]->{prev_begin} + $d->[2]->{prev_span};
	if ($prev_end > 0) { $prev_end-=1; }
	$line .= " " . $prev_end . ",";
	$line .= " " . $d->[2]->{prev_begin} . ",";
	$line .= " " . $d->[2]->{first_seen} . ",";
	$line .= " " . $d->[2]->{maxspan} . ",";
	$line .= " " . int($d->[2]->{success_count}) . ",";
	$line .= " " . int($d->[2]->{failure_count}) . ",";
	$line .= " " . int($d->[2]->{local_success_count}) . ",";
	$line .= " " . int($d->[2]->{local_failure_count}) . ",";
	$line .= " " . int($d->[2]->{previous_local_success}) . ",";
	$line .= " " . int($d->[2]->{previous_local_failure}) . ",";
	$line .= " " . int($d->[2]->{previous_remote_success}) . ",";
	$line .= " " . int($d->[2]->{previous_remote_failure});
	$line .= "\t},\n";
	$last_stat{$node}->{$bt}=$line;
	print $line;
}
print "\t{ 0, 0, 0 }\n";
print "};\n\n";

print "static const TBTTestStats KBTFinalStats[] = {\n";
foreach my $node (keys %last_stat) {
	foreach my $bt (keys %{$last_stat{$node}}) {
		print $last_stat{$node}->{$bt};
	}
}
print "\t{ 0, 0, 0 }\n";
print "};\n\n";
