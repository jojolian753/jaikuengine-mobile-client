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

open(NAMES, "<names.txt");
while(<NAMES>) {
	chop;
	($cell, $name)=split(/\t/);
	$oname=$name;
	if ($seen{$name}) {
		$name.="_" . $seen{$name};
	}
	$seen{$oname}++;
	$names{$cell}=$name;
}
close(NAMES);

while(<>) {
	last if (/^MAPP/);
	chop;
	if (/merging/) {
		/^([^ ]+) merging:/;
		$dt=$1;
		/merging: (.*) into/;
		$cells=$1;
		/into ([0-9]+)/;
		$into=$1;
		push(@merging, [ $dt, $cells, $into ]);
		next;
	}
	/^([0-9]+):/;
	$id=$1;
	$m=undef;
	next unless (/merged to ([0-9]+)$/);
	$m=$1;
	next if ($id eq $m); 
	push(@{$merged{$m}}, $id);
	$into{$id}=$m;
}

foreach $m (keys %merged) {
	$r=$m;
	while ( $into{$m} ) {
		$m=$into{$m};
	}
	next if ($r==$m);
	push(@{$merged{$m}}, @{$merged{$r}});
	delete $merged{$r}
}

while(<>) {
	chop;
	($id, $cell)=split(/\t/);
	$cells{$id}=$cell;
}

foreach $m (@merging) {
	($dt, $cells, $into)=@{$m};
	@cells=split(/ +/, $cells);
	print $dt, " merging: ";
	$sep="";
	foreach $c (@cells) {
		$cell=$cells{$c};
		$cell=$names{$cell} if ($names{$cell}); 
		print $sep, $cell;
		$sep="\t";
	}
	$cell=$cells{$into};
	$cell=$names{$cell} if ($names{$cell}); 
	print " into ", $cell, "\n";
}

foreach $m (keys %merged) {
	@ids=@{$merged{$m}};
	push(@ids, $m);
	@ids=grep { ++$count{$_} < 2 } @ids;
	foreach $id (@ids) {
		$cell=$cells{$id};
		$cell=$names{$cell} if ($names{$cells{$id}}); 
		print $cell, "\t";
	}
	print "\n";
}

