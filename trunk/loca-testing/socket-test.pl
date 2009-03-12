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
use Socket;
use Compress::Zlib;

my $setting="
  <tuple module='0x1020811e' id='7' major_version='1'
  minor_version='0'>
    <id>13</id>

    <tuplename>
      <module_uid>0x10208120</module_uid>

      <module_id>1</module_id>

      <subname>58</subname>
    </tuplename>

    <tuplevalue module='0x1020811a' id='10' major_version='1'
    minor_version='0'>testproject</tuplevalue>
    <expires>20070101T000000</expires>
  </tuple>
";

# use port 2000 as default
my $port = shift || 2000;
my $proto = getprotobyname('tcp');

# create a socket, make it reusable
socket(SERVER, PF_INET, SOCK_STREAM, $proto) or die "socket: $!";
setsockopt(SERVER, SOL_SOCKET, SO_REUSEADDR, 1) or die "setsock: $!";

# grab a port on this machine
my $paddr = sockaddr_in($port, INADDR_ANY);

# bind to a port, then listen
bind(SERVER, $paddr) or die "bind: $!";
listen(SERVER, SOMAXCONN) or die "listen: $!";
print "SERVER started on port $port\n";

$|=1;
my %ids;
# accepting a connection
my $client_addr;
my $sent_setting=0;
while ($client_addr = accept(CLIENT, SERVER)) {
	my ($iStr, $istatus) = inflateInit();
	my ($oStr, $ostatus) = deflateInit();

	# find out who connected
	my ($client_port, $client_ip) = sockaddr_in($client_addr);
	my $client_ipnum = inet_ntoa($client_ip);
	my $client_host = gethostbyaddr($client_ip, AF_INET);
	# print who has connected
	print "got a connection from: $client_host", "[$client_ipnum]\n";
	# send them a message, close connection
	select(CLIENT); $| = 1; select(STDOUT);
	my $in_buf;
	my $line;
	my $count=0;
	my $recv=0; my $uncompr=0;
	while( read(CLIENT, $in_buf, 1) > 0) {
		$recv++;
		my $st; my $buf;
		($buf, $st) = $iStr->inflate($in_buf);
		for (my $i=0; $i<length($buf); $i++) {
			$uncompr++;
			my $c=substr($buf, $i, 1);
			$line.=$c;
			#print "recv $line\n";
			if ($c eq "\n") {
			
				$_=$line;	
				print "RECV:", $line;
				$line="";
				my $resp="";
				if (/\<stream\>/) {
					$resp="<?xml version='1.0'?>\n<stream>\n";
				}
				next if (/\<ack>/);
				if (/\<id\>(\d+)\<\/id\>/) {
					print "id $1 count ", $ids{$1}++, "\n";
					$count++;
					$resp="<ack><id>$1</id></ack>\n";
					#$resp="<ack><id>$1</id></ack>\n" if ($1 == "0");
				}
				if ($sent_setting==0 && /\<id\>(\d+)\<\/id\>/) {
					#$resp .= $setting;
					$sent_setting=1;
				}
				if (/\<\/stream\>/) {
					$resp="</stream>\n";
				}
				unless ($resp eq "") {
					print "SEND:", $resp;
					my ($r, $status) = $oStr->deflate($resp);
					print CLIENT $r;
					($r, $status) = $oStr->flush(Z_SYNC_FLUSH);
					print CLIENT $r;
				}
			}
		}
	}
			
	close CLIENT;
	$count--; # ident
	print "connection closed, received $recv bytes, uncompressed $uncompr, $count tuples\n";
}
