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

@dirs=qw( autostart starter ContextNotify ContextNotifyClient ContextCommon
ContextClient BlackBoard BlackBoardClient ContextSensors ContextNetwork
BlackBoardServer ContextCommon2 ContextMedia ContextSensors ContextUI ContextMedia
Recognizer ContextServer ContextMediaApp Context_log ContextContacts 
ContextCallLog );

$hs{"context_uids.h"}=1;
$hs{"context_uids.rh"}=1;

$|=1;
print "processing directories for includes";
foreach $dir (@dirs) {
	print ".";
	unless (chdir("$dir/inc")) {
		print STDERR "cannot chdir to $dir/inc";
		next;
	}
	foreach my $h (<*.h *.H>) {
		$hs{$h}=1;
	}
	chdir("../..");
}
print "\n";

print "processing directories for code\n";
#@dirs=qw(starter);
foreach $dir (@dirs) {
	foreach my $subdir (qw(src inc)) {
		print "  $dir/$subdir";
		unless (chdir("$dir/$subdir")) {
			print STDERR "cannot chdir to $dir/$subdir";
			next;
		}
		my @files=();
		if ($subdir eq "src") { @files=<*.cpp>; }
		else { @files=<*.h *.H>; }
		foreach my $cpp (@files) {
			print ".";
			unless (open(IN, "<$cpp")) {
				print STDERR "cannot open $dir/$subdir/$cpp for reading";
				next;
			}
			unless (open(OUT, ">${cpp}.bak")) {
				close(IN);
				print STDERR "cannot open $dir/$subdir/${cpp}.bar for writing";
				next;
			}
			while(<IN>) {
				if (/^\s*#include\s+<([^>]+)>\s+$/) {
					$f=$1;
					if ($hs{$f}) {
						print OUT '#include "', $f, '"', "\n";
						next;
					}
				}
				print OUT $_;
			}
			close(IN); close(OUT);
			system("cp ${cpp}.bak $cpp");
		}
		print "\n";
		chdir("../..");
	}
}
