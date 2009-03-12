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

use lib '../perl';
use Jaiku::BBData::BuildInfo;
use Getopt::Long;
use Cwd;

GetOptions("upminor"=>\$upminor,
	   "upmajor"=>\$upmajor,
	   "upinternal",\$upinternal,
	   "tstamp=s"=>\$tstamp,);

# Check tstamp
if ($tstamp eq "")
  {
    # get current time in correct format
    ($s,$min,$h,$d,$mon,$y) = localtime(time);
    $tstamp= sprintf("%04d%02d%02dT%02d%02d%02d", 
		     $y + 1900, $mon + 1, $d, $h, $min, $s);
  }
unless ($tstamp =~ m/\d{8}T\d{6}/) 
{
  die "Error: timestamp has to be in format yyyymmddThhmmss \n";
}

# Read version 
$workdir = getcwd;
open(IN, "<jaiku.pkg") || die "cannot open jaiku.pkg";
@lines=<IN>;
close(IN);

@matches=grep { $_=~m/(\d+),(\d+),(\d+)/ } @lines;
die "did not find match" unless($matches[0]);
die "several matches" if ($#matches>0);
$matches[0] =~ m/(\d+),(\d+),(\d+)\r?$/ || die "failed to parse version";
$majorV = $1;
$minorV = $2;
$internalV = $3;

print "Major $majorV minor $minorV internal $internalV\n";

# uupdate version
if ($upmajor) { $majorV += 1; $minorV = 0; $internalV = 0; }
if ($upminor) { $minorV += 1; $internalV = 0; }
if ($upinternal) { $internalV += 1; }
if ($upmajor || $upminor || $upinternal) {
  open(IN, "+<jaiku.pkg") || die "cannot open jaiku.pkg for writing";
  @lines = <IN>;
  seek IN,0,0;
  foreach $line (@lines) {
   $line =~ s/\d+,\d+,\d+$/$majorV,$minorV,$internalV/;
   print IN $line;
  }
  close(IN);
}


# Sdk and build by
$ENV{'EPOCROOT'} =~ m/([^\\]*)\\?$/;
$sdk = $1;
$buildBy = $ENV{'USERNAME'};

my $ug=new Jaiku::BBData::BuildInfo("buildinfo");
$ug->set_type_attributes();
$ug->when->set_value($tstamp);
$ug->buildby->set_value($buildBy);
$ug->sdk->set_value($sdk);
$ug->majorversion->set_value($majorV);
$ug->minorversion->set_value($minorV);
$ug->internalversion->set_value($internalV);

open(OUT, ">buildinfo.xml") || die "can't open buildinfo.xml";
print OUT $ug->as_xml;
close(OUT);


