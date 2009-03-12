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
use Net::Jabber qw(Client);
use Date::Calc qw(Add_Delta_DHMS);

my $nick ="";
my $pass = "";
my $server = "";
my $port = 5222;
my $resource = "Context";
my $profile = "no-name";
my $vibrator = "true";
my $ring_type = 0;
my $friends = 0;
my $others = 0;
my $speaker_volume = 0;
my $previous_location = "";
my $previous_location_since = "";
my $current_location = "";
my $current_location_since = "";
my $user_activity = "";
my $user_activity_since = "";
my $old = "";
my $profile_id = 0;


if ($#ARGV != 0) 
{
	print "Usage: $0 <presence-text-file>\n";
	exit;
}

open (INPUT, "$ARGV[0]") || die "can't open $ARGV[0]\n";
while (my $line = <INPUT>)
{
	chomp $line;
	if ($line =~ /^#/) {next;}
	if ($line =~ /^nick:(.+)/)
	{
		$nick = $1;	
	}
	elsif ($line =~/^pass:(.+)/)
	{
		$pass = $1;
	}
	elsif ($line =~/^profile:(.+)/)
	{
		$profile = $1;
		print "$profile\n";
	}
	elsif ($line =~/^prev:(.+)/)
        {
                $previous_location = $1;
        }
	elsif ($line =~/^curr:(.+)/)
        {
                $current_location = $1;
        }
	elsif ($line =~/vib:(.+)/)
        {
		$vibrator = $1;
        }
	elsif ($line =~/^sp_vol:(\d+)/)
        {
                $speaker_volume = $1;
        }
	elsif ($line =~/^ringtype:(\d+)/)
        {
                $ring_type = $1;
        }
	elsif ($line =~/^friends:(\d+)/)
        {
                $friends = $1;
        }
	elsif ($line =~/^others:(\d+)/)
        {
                $others = $1;
        }
	elsif ($line =~/^serv:(.+)/)
        {
                $server = $1;
        }
        elsif ($line =~/^port:(\d+)/)
        {
                $port = $1;
        }
	elsif ($line =~/^old:(\d+)/)
	{
		$old = $1;
	}
	elsif ($line =~/^prev_since:(\d+)/)
        {
                $previous_location_since = $1;
        	print "prev_since:$previous_location_since\n";
	}
	elsif ($line =~/^curr_since:(\d+)/)
        {
                $current_location_since = $1;
        	print "curr_since:$current_location_since\n";
	}
	elsif ($line =~/^user:(.+)/)
        {
                $user_activity = $1;
        }
	elsif ($line =~/^user_since:(\d+)/)
        {
                $user_activity_since = $1;
        }
	elsif ($line =~/^profile_id:(\d+)/)
        {
                $profile_id = $1;
        }
}
close INPUT;

my $connection = new Net::Jabber::Client();
$connection->Connect( "hostname" => $server,
                      "port"     => $port )
   or die "Cannot connect ($!)\n";
my @result = $connection->AuthSend( "username" => $nick,
                                    "password" => $pass,
                                    "resource" => $resource );
if ($result[0] ne "ok") 
{
	die "Ident/Auth with server failed: $result[0] - $result[1]\n";
}

while(1) {

	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
	my $now_time = sprintf("%4d%02d%02dT%02d%02d%02d", $year+1900, $mon+1, $mday, $hour, $min, $sec);
	my $presence = "";

	##########################################
	# begin generating presence
	##########################################


	# send time stamp
	my ($y,$mo,$d, $h,$m,$s) = Add_Delta_DHMS($year+1900,$mon+1,$mday, $hour,$min,$sec, 0, 0, -($old), 0);
	my $send_time = sprintf("%4d%02d%02dT%02d%02d%02d",$y,$mo,$d,$h,$m,$s);
	$presence .= "$send_time<presence><event><datetime>$send_time</datetime><location><location.value><location.network>RADIOLINJA</location.network><location.lac>9006</location.lac><location.cellid>18</location.cellid></location.value></location></event>";

	# begin base location
	$presence .= "<event><datetime>$send_time</datetime><base>";

	if ($previous_location && ($previous_location ne ""))
	{
		my ($y,$mo,$d, $h,$m,$s) = Add_Delta_DHMS($year+1900,$mon+1,$mday, $hour,$min,$sec, 0, 0, -($previous_location_since), 0);
		my $before_time = sprintf("%4d%02d%02dT%02d%02d%02d", $y, $mo, $d, $h, $m, $s); 
		$presence .= "<base.previous>$previous_location</base.previous><base.previous.left>$before_time</base.previous.left>";	
	}

	if ($current_location && ($current_location ne ""))
	{
		my ($y,$mo,$d, $h,$m,$s) = Add_Delta_DHMS($year+1900,$mon+1,$mday, $hour,$min,$sec, 0, 0, -($current_location_since), 0);
		my $before_time = sprintf("%4d%02d%02dT%02d%02d%02d", $y, $mo, $d, $h, $m, $s);
		$presence .= "<base.current.arrived>$before_time</base.current.arrived><base.current>$current_location</base.current>";
	}

	$presence .= "</base></event>";
	#end base location

	# neighbourhood
	$presence .= "<event><datetime>$send_time</datetime><bt.presence><buddies>$friends</buddies><other_phones>$others</other_phones></bt.presence></event>";

	#user activity
	if ($user_activity eq "active")
	{
		$presence .= "<event><datetime>$send_time</datetime><useractivity><useractivity.value>active</useractivity.value></useractivity></event>";
	}

	if ($user_activity eq "idle")
	{
		my ($y,$mo,$d, $h,$m,$s) = Add_Delta_DHMS($year+1900,$mon+1,$mday, $hour,$min,$sec, 0, 0, -($user_activity_since), 0);
		my $before_time = sprintf("%4d%02d%02dT%02d%02d%02d", $y, $mo, $d, $h, $m, $s);

		$presence .= "<event><datetime>$send_time</datetime><useractivity><useractivity.value>idle</useractivity.value></useractivity></event>";
	}

	# profile
	$presence .= "<event><datetime>$send_time</datetime><profile><profile.value><profile.id>$profile_id</profile.id><profile.name>$profile</profile.name><profile.ringtype>$ring_type</profile.ringtype><profile.ringvolume>$speaker_volume</profile.ringvolume><profile.vibrate>$vibrator</profile.vibrate></profile.value></profile></event>";

	$presence .= "</presence>";

	#debug
	#print "$presence\n";



	#########################################
	# end of generating presence
	#########################################
		my $message = Net::Jabber::Presence->new();
        $message->SetPresence(
                from=> $nick . "@" . $server . "/" . $resource,
                status=>$presence);
        $connection->Send($message);
        sleep 60;
}



exit;

