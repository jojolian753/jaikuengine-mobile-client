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

use strict;
use Data::Dumper;

my $tracefile=$ARGV[0];
my $addr=$ARGV[1];

if (! $tracefile ) {
	print STDERR "Usage:\n\n";
	print STDERR "read_trace_file.pl NAME_OF_TRACEFILE\n";
	exit 1;
}
open(TRACE, "<$tracefile") || die "cannot open $tracefile";

binmode TRACE;


sub read_stack {
	my $stack=shift;
	my ($current)=unpack( "l", substr($stack, 0, 4) );
	$current+=4;
	my @stack;
	while ($current>4) {
		my $prev=$current-4;
		my ($p)=unpack("l", substr($stack, $prev, 4));
		$current = 4 + $p;
		my $len = $prev-$current;
		my $n=substr($stack, $current, $len);
		$n=~s/([\w])[^\w]+$/$1/;
		push(@stack, $n);
	}
	return \@stack;
}

my (%top_alloc, %top_own_alloc);
my (%alloc_count, %own_alloc_count);
my (%current_alloc, %current_own_alloc);
my (%allocator, %own_allocator, %length);
my (%at_highest, %own_at_highest);

my $total_alloc=0;
my $highest_alloc=0;

sub handle_data {
	my ($op, $address, $length, $stack, $cell)=@_;
	return unless $address;
	if ($op==1) {
		$total_alloc+=$length;
		my %seen;
		my $prev_class="";
		foreach my $frame (reverse @$stack) {
			#next unless ($frame=~/(.*)::(.*)/);
			#my ($class, $func)=($1, $2);
			my $class=$frame;
			next if ($class eq $prev_class);
			next if ($class =~ /^\s*$/);
			next if ($seen{$class});
			$seen{$class}=1;
			$current_alloc{$class} += $length;
			if ($current_alloc{$class} > $top_alloc{$class}) {
				$top_alloc{$class}=$current_alloc{$class};
			}
			$alloc_count{$class}++;
			push(@{$allocator{$address}}, $class);
			$prev_class=$class;
		}
		$current_own_alloc{$prev_class} += $length;
		if ($current_own_alloc{$prev_class} > $top_own_alloc{$prev_class}) {
			$top_own_alloc{$prev_class}=$current_own_alloc{$prev_class};
		}
		$own_allocator{$address}=$prev_class;
		$own_alloc_count{$prev_class}++;
		$length{$address}=$length;
	} elsif ($op==2) {
		$total_alloc-=$length;
		foreach my $class ( @{$allocator{$address}} ) {
			$current_alloc{$class} -= $length{$address};
		}
		my $own_class=$own_allocator{$address};
		$current_own_alloc{$own_class} -= $length{$address};
		delete $allocator{$address};
		delete $own_allocator{$address};
	} elsif ($op==3) {
		$total_alloc-=$length{$cell};
		$total_alloc+=$length;
		foreach my $class ( @{$allocator{$cell}} ) {
			$current_alloc{$class} -= $length{$cell};
			$current_alloc{$class} += $length;
			$alloc_count{$class}++;
			if ($current_alloc{$class} > $top_alloc{$class}) {
				$top_alloc{$class}=$current_alloc{$class};
			}
		}
		my $own_class=$own_allocator{$cell};
		$current_own_alloc{$own_class} -= $length{$cell};
		$current_own_alloc{$own_class} += $length;
		if ($current_own_alloc{$own_class} > $top_own_alloc{$own_class}) {
			$top_own_alloc{$own_class}=$current_own_alloc{$own_class};
		}
		if ($cell != $address) {
			$allocator{$address}=$allocator{$cell};
			$own_allocator{$address}=$own_allocator{$cell};
			delete $allocator{$cell};
			delete $own_allocator{$cell};
		}
		$length{$address}=$length;
	}
	if ($total_alloc > $highest_alloc) {
		%at_highest=%current_alloc;
		%own_at_highest=%current_own_alloc;
		$highest_alloc=$total_alloc;
	}
}

my %allocated;
while(1) {
	my ($op, $stacklen, $stack, $length, $address, $cell);

	my $buf;
	read TRACE, $buf, 8;
	last if (length($buf)<8);
	($op, $stacklen)=unpack("ll", $buf);

	read TRACE, $buf, $stacklen;
	$stack=$buf;
	$stack=read_stack($stack);
	if ($op==1) {
		read TRACE, $buf, 8;
		($length, $address)=unpack("ll", $buf);
		if ($length) { $allocated{$address}=$stack; }
	} elsif ($op==2) {
		read TRACE, $buf, 4;
		($address)=unpack("l", $buf);
		$length=0;
		delete $allocated{$address};
	} else {
		read TRACE, $buf, 12;
		($length, $address, $cell)=unpack("lll", $buf);
		if ($address && $length && ($cell != $address)) {
			delete $allocated{$cell};
			$allocated{$address}=$stack;
		}
	}
	if ($address==oct($addr)) {
		printf "%02d: %08x %06d (%08x) ", $op, $address, $length, $cell;
	}
	handle_data($op, $address, $length, $stack, $cell);
	if ($address==oct($addr)) {
		print join("|", @$stack), "\n";
	}
}

foreach my $k (keys %allocated) {
	printf STDERR "Leaked at %08x by ", $k;
	my $stack=$allocated{$k};
	print STDERR join("|", @$stack), "\n";
}
exit 0;

exit 0 if ($addr);

print "class\ttop\ttop_own\tcount\town_count\tat_highest\town_at_highest\n";
foreach my $class (sort keys %top_alloc) {
	my $top_alloc=$top_alloc{$class};
	my $top_own_alloc=$top_own_alloc{$class};
	my $count=$alloc_count{$class};
	my $own_count=$own_alloc_count{$class};
	my $at_highest=$at_highest{$class};
	my $own_at_highest=$own_at_highest{$class};

	print "$class\t$top_alloc\t$top_own_alloc\t$count\t$own_count\t$at_highest\t$own_at_highest\n";
}
