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

use lib '../../ContextSensors/types';
use Cwd;

my $cwd=cwd();
my $module=$cwd;
$module=~s!/[^/]+/?$!!;
$module=~s!^.*/([^/]+)$!$1!;

%types=();
%uids=();

require cpp_types;
require perl_types;
require python_types;

open(UIDS, "<../../context_uids.rh") || die  "cannot open ../../context_uids.rh";
while(<UIDS>) {
	next unless (/^\s*#define\s+(\w+)\s+(\w+)/);
	$uids{$1}=$2;
}
open(UIDS, "<../../old2_context_uids.rh") || die  "cannot open ../../old2_context_uids.rh";
while(<UIDS>) {
	next unless (/^\s*#define\s+(\w+)\s+(\w+)/);
	$uids{$1}=$2;
}


foreach my $f (<*.pl>) {
	next if ($f eq "create_types.pl");
	eval('require("$f")');
	die "*** Failed to eval() file $f:\n$@\n" if ($@);
}

sub replace_if_different($$) {
	my $file=shift;
	my $contents=shift;
	if ( open(IN, "<" . $file)) {
		my @lines=<IN>;
		my $existing=join(@lines);
		close(IN);
	}
	return if ($contents eq $existing);
	open(OUT, ">" . $file) || die "cannot open $file for writing";
	print OUT $contents;
	close(OUT);
}

$mmp="SOURCEPATH ..\\src\n";
$py_types="";
foreach my $typename (keys %types) {
	my $cpp="";
	my $type=$types{$typename};
	my %fields;
	for (my $field_idx=0; $field_idx <= $#{$type->{ordered_fields}}; $field_idx+=2) {
		my $fieldname=$type->{ordered_fields}->[$field_idx];
		my $fieldtype=$type->{ordered_fields}->[$field_idx+1];
		$fields{ $fieldname } = $fieldtype;
	}
	$type->{fields}=\%fields;
	$type->{uid_value}=$uids{$type->{uid}};
	$type->{old_uid_value}=$uids{"OLD2_" . $type->{uid}};
	my $h=type_to_h($typename, $type, \%types);
	my $cpp=type_to_cpp($typename, $type, \%types);

	my $pm=type_to_pm($typename, $type, \%types);
	my $py=type_to_py($typename, $type, \%types);


	print "typename: ", $typename, "\n";
	print "uid: ", $type->{uid}, "\n";
	print "id: ", $type->{id}, "\n";
	print "defaultname: ", $type->{defaultname}, "\n";
	print "fields: ", "\n";
	foreach my $fieldname ( keys %{$type->{fields}} ) {
		$fieldtype = $type->{fields}->{$fieldname};
		print "\t", $fieldname, ": ", $type->{fields}->{$fieldname}, "\n";
	}
	replace_if_different("../inc/csd_" . lc($typename) . ".h", $h);
	replace_if_different("../src/csd_" . lc($typename) . ".cpp", $cpp);
	replace_if_different("../../perl/Jaiku/BBData/" . $typename . ".pm", $pm);
	replace_if_different("../../python/" . lc($typename) . ".py", $py);
	$mmp .= "SOURCE csd_" . lc($typename) . ".cpp\n";
	$py_types .= "from bbdata." . lc($typename) . " import *\n" if ($py);

	print "\n\n";
}
replace_if_different("types.mmp", $mmp);
replace_if_different("../../python/types_" . lc($module) . ".py", $py_types);
