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


$stats=stream_to_stat(\@testdata);
%s=();
foreach my $d (@$stats) {
	my $node=$d->[3];
	my $min=$d->[0];
	my $bt=$d->[1];
	my $avg=$d->[2]->{sum} / $d->[2]->{count};
	my $var=($d->[2]->{ssum} / $d->[2]->{count}) - $avg*$avg;
	my $prev_end=$d->[2]->{prev_begin} + $d->[2]->{prev_span};
	if ($prev_end > 0) { $prev_end-=1; }
	$s{$node}{$bt}{$min*60}=[
		$d->[2]->{count},
		int($avg),
		int($var),
		$d->[2]->{begin} * 60,
		$d->[2]->{prev_begin} * 60,
		$prev_end * 60 ];
}

open(IN, "</win/symbian/7.0s/Series60_v21/Epoc32/wins/c/python-locatest.txt");
while(<IN>) {
	@got=split;
	$got[5]=int($got[5]);
	$exp=$s{$got[0]}{$got[2]}{$got[1]};
	my $err="";
	for ($i=0; $i<6; $i++) {
		if ( $got[$i+3] != $exp->[$i] ) {
			$err .= "mismatch in $i, got " .  $got[$i+3] . " expected " . $exp->[$i] . "\n";
		}
	}
	unless ($err eq "") {
		$err++;
		print "ERR for ",
			$got[0], " ", $got[2], " ", $got[1], " ", $err;
	} else {
		$ok++;
	}
}
 
print "OK: $ok/", $err+$ok, "\n";
