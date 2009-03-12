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

use XML::Parser;
use Time::Local;

$ENV{'PATH'}="/bin:/usr/bin";
use CGI;
use CGI::Carp;
use Fcntl ':flock'; # import LOCK_* constants

sub get_fields {
	my $content=shift;
	unless ( substr($content, 0, 5) eq "<?xml" ) {
		$content="<?xml version='1.0' encoding='iso-8859-1'?>" . $content;
	}
	my $xp = new XML::Parser(Handlers => { 
		Char => \&char_handler ,
		Start => \&start_handler,
		End => \&end_handler,
	});

	eval { $xp->parse($content, ProtocolEncoding => 'ISO-8859-1') };
	if ($@) { $desc .=  " error: " . $@; }
	$desc=~s/\r//g;
	$desc=~s/\n//g;
	my %fields=(
		tag	=>	$tag,
		desc	=>	$desc,
		prev	=>	$prev,
		visualcode	=>	$vc,
		current	=>	$current);
	return \%fields;
}


sub char_handler {
  my($xp, $data) = @_;
  if ($in_tag) { $tag .= $data; }
  elsif ($in_desc) { $desc .= $data; }
  elsif ($in_prev) { $prev .= $data; }
  elsif ($in_current) { $current .= $data; }
  elsif ($in_vc) { $vc .= $data; }
}
sub start_handler {
  my($xp, $data, @rest) = @_;
  $in_tag=1 if ($data =~ /tag/);
  $in_desc=1 if ($data =~ /desc/);
  $in_prev=1 if ($data =~ /^base.previous$/);
  $in_current=1 if ($data =~ /^base.current$/);
  $in_vc=1 if ($data =~ /^threadid$/);
}
sub end_handler {
  my($xp, $data) = @_;
  $in_tag=$in_desc=$in_prev=$in_current=$in_vc=0;
}



sub response
{
	(my $status, my $msg)=@_;
	print "Content-Type: text/plain\n";
	#print "Status: $status\n";
	print "\n";
	print $msg;
	print "\n";
	exit 0;
}


my $cgi=new CGI;
my $DIR="/home/mraento/public_html/vc/";

my $filen=$cgi->param('photo');
if ($filen) {
	my $meta=$cgi->param('packet');
	my $packet_data;
	while(<$meta>) {
		$packet_data .= $_;
	}
	my $f=get_fields($packet_data);
	my $thread=$f->{'visualcode'};
	if (!$thread) { $thread=1; }

	open(IN, "+<$DIR/num.txt") || &response(201, "cannot open index file");
	flock(IN,LOCK_EX)  || &response(201, "cannot lock index file");
	seek(IN, 0, 0) || &response(201, "cannot seek in index");
	
	my $num=0;
	while(<IN>) {
		chop;
		$num=$_;
	}
	$num++;
	seek(IN, 0, 0) || &response(201, "cannot seek in index");
	print IN $num, "\n";
	flock(IN,LOCK_UN);
	close(IN);

	my $ext=$filen;
	$ext=~s/^.*\.([a-z0-9]+)$/.$1/;
	$store_filen=$DIR . $thread . "/" . $num . $ext;

	my $path=$store_filen;
	$path=~s/\/[^\/]+$/\//;
	mkdir($path);
	system("chmod 0755 $path");

	open(OUT, ">" . $store_filen) || &response(201, "Error: Cannot open output file $store_filen");
	while (<$filen>) {
		print OUT $_;
	}
	close(OUT);
	system("chmod 0644 $store_filen");
	if ($store_filen=~/jpg$/i) {
		my $full_filen=$store_filen;
		$full_filen=~s/\.jpg$/_full.jpg/i;
		system("mv $store_filen $full_filen");
		system("jpegtopnm $full_filen | pnmscale -xsize 172 | pnmtojpeg -quality 90 > $store_filen");
		system("chmod 0644 $store_filen");
	}
	if ($meta) {
		$store_filen=~s/\.[^.]*$//g;
		$store_filen .= ".xml";
		open(OUT2, ">" . $store_filen) || &response(201, "Error: Cannot open output file $store_filen");
		print OUT2 $packet_data;
		close(OUT2);
		system("chmod 0644 $store_filen");
	}

	&response(200, "OK");
} else {
	&response(201, "Error: no file given");
}
