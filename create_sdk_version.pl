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

$EPOC=$ENV{'EPOCROOT'};
if ($EPOC=~/^\s*$/) {
	die "EPOCROOT must be set"
}
my $SDK=$ENV{'EPOCROOT'};
$SDK=~s/.*\\([^\\]*)\\$/$1/;

@defs=();

if ($EPOC =~ /6\.1\\series60/i) {
	push(@defs, "__S60V1__");
} elsif ($EPOC =~ /7.0s\\series60_v20/i) {
	push(@defs, "__S60V2__");
	push(@defs, "__S60V20__");
} elsif ($EPOC =~ /7.0s\\series60_v21/i) {
	push(@defs, "__S60V2__");
	push(@defs, "__S60V21__");
} elsif ($EPOC =~ /8.0a.*FP2/i) {
	push(@defs, "__S60V2__");
	push(@defs, "__S60V2FP2__");
} elsif ($EPOC =~ /8\.1a.*FP3/i) {
	push(@defs, "__S60V2__");
	push(@defs, "__S60V2FP3__");
} elsif ($EPOC =~ /9\.1/i) {
	push(@defs, "__S60V2__");
	push(@defs, "__S60V3__");
} elsif ($EPOC =~ /9\.2/i) {
	push(@defs, "__S60V2__");
	push(@defs, "__S60V3__");
	push(@defs, "__S60V3FP1__");
} elsif ($EPOC =~ /3rd_FP2/i) {
	push(@defs, "__S60V2__");
	push(@defs, "__S60V3__");
	push(@defs, "__S60V3FP1__");
}

open(MMP, "<$EPOC\\epoc32\\include\\sdk_version.mmp") || print STDERR "No previous sdk_version.mmp\n";
while(<MMP>) { $MMP.=$_; }
close(MMP);

open(H, "<$EPOC\\epoc32\\include\\sdk_version.h") || print STDERR "No previous sdk_version.h\n";
while(<H>) { $H.=$_; }
close(H);

open(MK, "<$EPOC\\epoc32\\include\\sdk_version.mk") || print STDERR "No previous sdk_version.mk\n";
while(<MK>) { $MK.=$_; }
close(MK);


$wanted_mmp .= "#ifndef SDK_VERSION_MMP\n";
$wanted_mmp .= "#define SDK_VERSION_MMP\n";
$wanted_mmp .= "MACRO __SDK_VERSION_MMP__\n";

$wanted_h .= "#ifndef SDK_VERSION_H\n";
$wanted_h .= "#define SDK_VERSION_H\n";

$wanted_mk .= "";

foreach $d (@defs) {
	$wanted_mmp .= "MACRO $d\n";
	$wanted_mmp .= "#define $d\n";
	$wanted_h .= "#define $d\n";
	$wanted_mk .= "$d=1\n";
}

$wanted_mmp .= "#define __SDK__ $SDK\n";

if ($EPOC=~/9\./) {
    $wanted_mmp .= "#define MINIMAL_HEAP EPOCHEAPSIZE 0x2000 0x8000\n"
}

$wanted_mmp .= "#ifdef WINS\n";
$wanted_mmp .= "#define DEFFILENAME(x) EXPORTUNFROZEN\n";
$wanted_mmp .= "#define FACTORYDEFFILENAME(x) DEFFILE .\\ ## $SDK ## \\ ## x ## -wins.DEF\n";
$wanted_mmp .= "#else\n";
#$wanted_mmp .= "#define DEFFILENAME(x) DEFFILE .\\ ## $SDK ## \\ ## x ## -thumb.DEF\n";
$wanted_mmp .= "#define DEFFILENAME(x) EXPORTUNFROZEN\n";
$wanted_mmp .= "#define FACTORYDEFFILENAME(x) DEFFILE .\\ ## $SDK ## \\ ## x ## -thumb.DEF\n";
$wanted_mmp .= "#endif\n";
$wanted_mmp .= "\n";

$wanted_mmp .= "#endif // SDK_VERSION_MMP\n";
$wanted_h .= "#endif // SDK_VERSION_H\n";

unless ($MMP eq $wanted_mmp) {
	open(MMP, ">$EPOC\\epoc32\\include\\sdk_version.mmp") || die "cannot open $EPOC\\epoc32\\include\\sdk_version.mmp for writing";
	print MMP $wanted_mmp;
	close(MMP);
}

unless ($H eq $wanted_h) {
	open(H, ">$EPOC\\epoc32\\include\\sdk_version.h");
	print H $wanted_h;
	close(H);
}

unless ($MK eq $wanted_mk) {
	open(MK, ">$EPOC\\epoc32\\include\\sdk_version.mk");
	print MK $wanted_mk;
	close(MK);
}
