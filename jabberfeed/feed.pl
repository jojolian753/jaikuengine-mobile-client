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

use Net::Jabber qw(Client);
my $connection = new Net::Jabber::Client(debuglevel=>1, debugfile=>"stdout");

#my $SERVER    = 'jabber.org';
my $SERVER    = 'armi.hiit.fi';
my $PORT      = 5222;
my $USER      = 'mikie';
my $PASSWORD  = 'raento1';
my $RESOURCE  = 'Context';

$connection->Connect( "hostname" => $SERVER,
                      "port"     => $PORT )
   or die "Cannot connect ($!)\n";
my @result = $connection->AuthSend( "username" => $USER,
                                    "password" => $PASSWORD,
                                    "resource" => $RESOURCE );
if ($result[0] ne "ok") {
  die "Ident/Auth with server failed: $result[0] - $result[1]\n";
}
print "Connected\n";

$pre="20040311T104250#<presence><event><datetime>20040428T195102</datetime><location><location.value><location.network>RADIOLINJA</location.network><location.lac>9006</location.lac><location.cellid>18</location.cellid></location.value></location></event><event><datetime>20040428T195102</datetime><base><base.previous.left>20040311T102250</base.previous.left><base.previous>ESTI1</base.previous></base></event><event><datetime>20040428T195057</datetime><useractivity><useractivity.value>active</useractivity.value></useractivity></event><event><datetime>20040428T195057</datetime><profile><profile.value><profile.id>%d</profile.id><profile.name>%s</profile.name><profile.ringtype>%d</profile.ringtype><profile.ringvolume>%d</profile.ringvolume><profile.vibrate>%s</profile.vibrate></profile.value></profile></event></presence>";

my @profiles=(
	"General",
	"Silent",
	"Meeting",
	"Outdoor",
	"Pager");
my @ringtype=(
	1, 1, 2, 2, 3 );
my @ringvolume=(
	1, 1, 2, 2, 3 );
my @vibra=qw(true true false false true);

my $i=0;

while(1) {
	my $message = Net::Jabber::Presence->new();
	my $presence = sprintf($pre, $i, $profiles[$i],
		$ringtype[$i], $ringvolume[$i], $vibra[$i]);
	$message->SetPresence(
		from=> $USER . "@" . $SERVER . "/" . $RESOURCE,
		status=>$presence);
	print "Sending presence\n";
	$connection->Send($message);

	$i++;
	$i=0 if ($i>4);

	sleep 15;
}
