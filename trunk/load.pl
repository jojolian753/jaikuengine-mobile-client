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

require LWP;
require LWP::Debug;

my $DO_SEND=1;

use Image::Info qw(image_info);

use URI;
use URI::Heuristic qw(uf_uri);

use HTTP::Status qw(status_message);
use HTTP::Date qw(time2str str2time);

my $URL="http://aware.uiah.fi/upload/put5.pl?";
my $IMSI="353798002870220";

my $SENDER='<sender><jabbernick>mikie@armi.hiit.fi</jabbernick><authorname>Mika</authorname></sender>';

sub xmlenc($) {
 my $i;
 my $output; my $input=shift;
 for ($i=0; $i<length($input); $i++) {
   my $char=substr($input, $i, 1);
   if ($char eq "<") { $char="&lt;"; }
   if ($char eq "&") { $char="&amp;"; }
   $output .= $char;
 }
 return $output;
}
 
 use Image::IPTCInfo;

  # Create new info object
  my $info = new Image::IPTCInfo($ARGV[0]);
  my $exif_info=image_info($ARGV[0]);

  # Check if file had IPTC data
  unless (defined($info)) { die Image::IPTCInfo::Error(); }

  # Get list of keywords, supplemental categories, or contacts
  #my $dt=$info->Attribute('date created') . "T"  . 
	#substr($info->Attribute('time created'), 0, 6);
  my $dt=$exif_info->{DateTime};
  $dt=~s/://g;
  $dt=~s/ /T/;
    
  $|=1;
  my $put="C;$dt%20;0;$IMSI/" . $ARGV[0];
  my $cmd="|POST '" . $URL . $put . "'";
  print $cmd, ": ";
  my $keywordsRef = $info->Keywords();
  my @kw=@$keywordsRef;
  if ( grep(/private/, @kw) ) {
	print "not sending private image $ARGV[0]\n";
	$DO_SEND=0;
  }

  if ($DO_SEND) {
	  open(CMD, $cmd);
	  open(INP, "<$ARGV[0]");
	  binmode(INP);
	  my $buf;
	  while(sysread(INP, $buf, 1024*1024)) { 
	    print ".";
	    print CMD $buf;
	  }
	  close(CMD);
  }
  print "\n";

  my $put="P;$dt%20;0;$IMSI/" . $ARGV[0];

  my $packet="<packet><tag>";
  push(@kw, $loc);

  my @locs=("sub-location", "city", "country/primary location name");
  foreach my $l (@locs) {
	if ($info->Attribute($l)) {
		push(@kw, $info->Attribute($l));
	}
  }

  my $keyw=join(":", @kw);
  $keyw=~s/:+/:/g;
  $keyw=~s/^://;
  $keyw=~s/:$//;
  $packet .= xmlenc($keyw);
  $packet .= "</tag><description>";
  $packet .= xmlenc($info->Attribute('caption/abstract'));
  $packet .= "</description>";
  $packet .= $SENDER;
  $packet .= "</packet>";

  my $cmd="|POST '" . $URL . $put . "'";
  print $cmd, "\n";
  if ($DO_SEND) {
	  open(CMD, $cmd);
	  print CMD $packet;
	  close(CMD);
  } else {
	print $packet;
  }

