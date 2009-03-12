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

$def=$ARGV[0];
die "Usage:\ndef_to_mmp.pl DEFFILE" if ($def eq "");

if ($def=~/([^\\]+)\\([^\\]+)\\(.*)/) {
	$platform=$1;
	$release=$2;
	$deffile=$3;
} else {
	die "DEFFILE should be platform\\release\\deffilename";
}

open(DEF, "<$def") || die "cannot open $def for reading";
$dllname=<DEF>;
if ($dllname=~/;DLLNAME\s+([^\[]+)\[([\da-f]+)\]\.DLL/i) {
	$dll=$1;
	$uid=$2;
} else {
	die "cannot parse dllname line $dllname";
}
close(DEF);
$project=$deffile;
#print $project;
$project=~s/.thumbu\.DEF//i;
$project=~s/.winsu\.DEF//i;

if ($platform eq "thumb") {
	$outputdir=$ENV{'EPOCROOT'} . "epoc32\\release\\"  . $platform . "\\" . $release . "\\";
	open(TOUCH, ">" . $outputdir . $project . ".LIB");
	close(TOUCH);
	$dllname=$dll . "[" . $uid . "].DLL";
	my $cwd=`echo %CD%`;
	my $drive=substr($cwd, 0, 2);
	$cmd="dlltool -m thumb --output-lib $drive$outputdir${project}.LIB --def $def --dllname $dllname";

	open(OUT, ">run-dlltool.bat") || die "cannot open dlltool.bat";
	print OUT "SET OLDPATH=%PATH%\r\n";
	print OUT "SET PATH=$drive%EPOCROOT%epoc32\\gcc\\bin\r\n";
	print OUT "$cmd\r\n";
	print OUT "SET PATH=%OLDPATH%\r\n";
	close(OUT);
	$cmd="run-dlltool.bat";
} else {
	$outputdir=$ENV{'EPOCROOT'} . "epoc32\\release\\"  . $platform . "\\" . $release . "\\";
	$dllname=$dll . ".DLL";
	$cmd="lib.exe /nologo /machine:i386 /nodefaultlib /name:\"$dllname\" /def:\"$def\" /out:\"$outputdir${project}.LIB\"";
}
print $cmd, "\n";
system($cmd)
