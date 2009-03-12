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

$FILE=shift @ARGV;
$FILE="raii.txt" if (! defined($FILE) );
open(IN, "<$FILE") || die "cannot open $FILE";

my @indent=();
my $indent=0;

my $header;
my $guard;
my $class;
my @methods=();
my $init;
my $first=1;

sub print_class($$)
{
	if ($class eq "") { return; }

	my $out_h=shift;
	my $out_cpp=shift;

	my $c=$class;
	$c=~s/^R/RA/;
	print $out_h "class $c : public $class, public RABase {\n";
	print $out_h "public:\n";
	my $init_arg=$init;
	$init_arg=~s/\&//g;
	$init_arg=~s/^.*\s+(\w+)$/\1/;
	unless ($init eq "") {
		print $out_h "\t$c($init) : $class($init_arg) { }\n";
	}

	foreach my $m (@methods) {
		if ($m=~/^#/) {
			print $out_h $m, "\n";
			next;
		}
		$m=~s/;//;
		my $newm=$m;
		$newm=~s/TInt/void/;
		$newm=~s/L\(/\(/;
		$newm=~s/\(/LA(/;
		print $out_h "\tinline $newm {\n";
		$m=~s/=[^,)]+//g;
		$m=~s/\&//g;
		$m=~s/\*//g;
		$m=~s/[^(),]+\s+(\w+)(\s*[,)])/\1\2/g;
		$m=~s/TInt\s+//;
		$m=~s/void\s+//;
			
		print $out_h "\t\tRABase::CloseRA();\n";
		if ($m =~ /L\(/) {
			print $out_h "\t\t$m;\n";
		} else {
			print $out_h "\t\t{ TInt err=$m;\n";
			print $out_h "\t\tUser::LeaveIfError(err); } \n";
		}
		print $out_h "\t\tiOpen=ETrue;\n";
		print $out_h "#ifndef __LEAVE_EQUALS_THROW__\n";
		print $out_h "\t\tPutOnStackL();\n";
		print $out_h "#endif\n";
		print $out_h "\t}\n";
	}
	print $out_h "\t~$c() { RABase::CloseRA(); }\n";
	print $out_h "\tvoid CloseInner() { ${class}::Close(); }\n";
	print $out_h "private:\n";
	print $out_h "\tvoid Close() { }\n";
	#print $out_h "\tTBool IsOpen() { return iOpen; }\n";
	#print $out_h "\tTBool\tiOpen;\n";
	#print $out_h "\tTBool\tiOnStack;\n";
	print $out_h "};\n\n";

	$class="";
}

sub close_file($$)
{
	my $out_h=shift;
	my $out_cpp=shift;

	print $out_h "#endif // $guard\n";
	close($out_h);
	close($out_cpp);
}

while(<IN>) {
	chop;
	s/\r//;

	if (/^#/) {
		push(@methods, $_);
		next;
	}
	next if (/^\s*$/);
	
	/^(\s*)(.*)/;
	my ($this_indent, $this_arg) = ($1, $2);
	$this_arg=~s/^\s*//;
	$this_arg=~s/\s*$//;
	$this_indent=~s/\t/        /g; # tabs are 8 spaces
	while ( $first || length($this_indent) != length($indent[$#indent]) ) {
		$first=0;
		if ( length($this_indent) < length($indent[$#indent])) {
			$indent--;
			pop(@indent);
		} elsif ( length($this_indent) > length($indent[$#indent])) {
			$indent++;
			push(@indent, $this_indent);
		} 
		if ($indent==0) {
			print_class(\*OUT_H, \*OUT_CPP);
			close_file(\*OUT_H, \*OUT_CPP);

			$header=$this_arg;
			$guard="CC_RA" . uc($header) . "_INCLUDED";
			$guard=~s/\./_/g;
			
			my $out_h=$header;
			my $out_cpp=$header;
			$out_h="inc/raii_" . $out_h;
			$out_cpp="src/raii_" . $out_cpp;
			
			open(OUT_H, ">$out_h") || die "cannot open $out_h";
			#open(OUT_CPP, ">$out_cpp") || die "cannot open $out_cpp";

			print OUT_H "#ifndef $guard\n";
			print OUT_H "#define $guard 1\n\n";
			print OUT_H "#include <", $header, ">\n";
			print OUT_H "#include \"raii.h\"\n\n";

			$class="";
			@methods=();
		}
	}
	if ($indent==1) {
		print_class(\*OUT_H, \*OUT_CPP);
		$class="";
		@methods=();
		if ($this_arg=~/(\w+)\s+(.+)/) {
			$class=$1;
			$init=$2;
		} else {
			$init="";
			$class=$this_arg;
		}
	} elsif ($indent==2) {
		push(@methods, $this_arg);
	}
}

print_class(\*OUT_H, \*OUT_CPP);
close_file(\*OUT_H, \*OUT_CPP);
