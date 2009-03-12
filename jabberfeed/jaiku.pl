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

$|=1;
print "Initializing: 'use'...";
use Net::Jabber qw(Client);
print "\n";
sub test1 {
	print "Initializing: 'new'...";
	#my $connection = new Net::Jabber::Client(debuglevel=>1, debugfile=>"stdout");
	my $connection = new Net::Jabber::Client(myhostname=>'ld5-3', debuglevel=>1, debugfile=>"dbg");
	my $connection2 = new Net::Jabber::Client(myhostname=>'ld5-3', debuglevel=>1, debugfile=>"dbg2");
	print "\n";
	$connection2->PresenceDB();

	#my $SERVER    = 'jabber.org';
	my $SERVER    = 'jaiku.com';
	#my $SERVER    = 'localhost';
	my $CONNECT ='localhost';
	my $PORT      = 9223;
	my $USER      = '_mikat1';
	my $USER2     = '_mikat2';
	my $PASSWORD  = 'mikat1';
	my $RESOURCE  = 'Context';
	my $RESOURCE2  = 'script';

	my $conn=\$connection;
	my $user=$USER;
	my $resource=$RESOURCE;

	for ($i=0; $i<2; $i++) {
		print "Connecting...";
		$$conn->Connect( "hostname" => $SERVER,
				 "connect_hostname" => $CONNECT,
				      "port"     => $PORT )
		   or die "Cannot connect ($!)\n";
		print "\n";
		my @result = $$conn->AuthSend( "username" => $user,
						    "password" => $PASSWORD,
						    "resource" => $resource );
		if ($result[0] ne "ok") {
		  die "Ident/Auth with server failed: $result[0] - $result[1]\n";
		}
		print "Connected\n";
		$conn=\$connection2;
		$user=$USER2;
		$resource=$RESOURCE2;
	}

	{
		my $message = Net::Jabber::Presence->new();
		$message->SetPresence(
			from=> $USER2 . "@" . $SERVER . "/" . $RESOURCE2,
			status=>"online");
		print "Sending presence for _mika\n";
		$connection2->Send($message);
	}

	$pre="20060311T164250<presencev2><usergiven><since>20060310T150000</since><description>from_client</description></usergiven></presencev2>";

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

	$cb=sub {
	  my $sid = shift;
	  my $presence = shift;
	  #print "PRESENCECB\n";

	  return unless($presence && $connection2);
	  $connection2->PresenceDBParse($presence);
	};
	$connection2->SetCallBacks(presence=>$cb);
	$suspended=0;
	$j=0;

	while(1) {
		my $message = Net::Jabber::Presence->new();
	#	my $presence = sprintf($pre, $i, $profiles[$i],
	#		$ringtype[$i], $ringvolume[$i], $vibra[$i]);
		my $presence=$pre;
		$message->SetPresence(
			from=> $USER . "@" . $SERVER . "/" . $RESOURCE,
			status=>$presence);
			#status=>substr($presence,0, ($i+1)*20));

		if ($j % 4 == 0) {
			print "Sending presence\n";
			$connection->Send($message);
		} else {
			print "Sending keepalive\n";
			$connection->Send("<c:keepalive xmlns:c='http://www.cs.helsinki.fi/group/context' />");
		}

		$i++;
		if ($i>4) {
			$i=0;
			$j++;
		}

		while ($connection->Process(0) || $connection2->Process(0)) {
			print "receiving\n";
			sleep 1;
		}
		my $recv=$connection2->PresenceDBQuery($USER . "@" . $SERVER);
		if ($recv) {
			print "received: " . $recv->GetStatus() . "\n";
		}
		if ($i==0) {
			if ($suspended==0) {
				$suspended=1;
				$connection2->Send("<c:suspend xmlns:c='http://www.cs.helsinki.fi/group/context' />");
				print "suspended\n";
			} else {
				$suspended=0;
				$connection2->Send("<c:resume xmlns:c='http://www.cs.helsinki.fi/group/context' />");
				print "resumed\n";
			}
		}
		sleep 3;
		#last;
	}

}

while(1) {
	test1();
}


