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
use Date::Calc qw(Delta_DHMS Add_Delta_DHMS);
use Time::Local;

####################################################

sub time_diff($$)
{
	my $later = shift;
	my $earlier = shift;


	if ($earlier eq "" || $later eq "")
	{
		return "";
	}

	my $year1 = substr($earlier, 0, 4);
	my $month1 = substr($earlier, 4, 2);
	my $day1 = substr($earlier, 6, 2);
	my $hour1 = substr($earlier, 9, 2);
	my $min1  = substr($earlier, 11, 2);
	my $sec1  = substr($earlier, 13, 2);
	my $year2 = substr($later, 0, 4);
	my $month2 = substr($later, 4, 2);
	my $day2 = substr($later, 6, 2);
	my $hour2 = substr($later, 9, 2);
	my $min2  = substr($later, 11, 2);
	my $sec2  = substr($later, 13, 2);
	my $days = 0;
	my $hours =0;
	my $minutes = 0;
	my $seconds = 0;
	($days, $hours, $minutes, $seconds) = Delta_DHMS( $year1, $month1, $day1, $hour1, $min1, $sec1,  # earlier
              							  $year2, $month2, $day2, $hour2, $min2, $sec2); # later

	#print "later $later earlier $earlier, diff $days d $minutes min $seconds s\n";
	if ( ($days==0) && ($minutes==0) && ($seconds<4) && ($seconds>=-2) )
	{
		return $hours;
	}
	else
	{
		return "";
	}
}

my @logs=sort <log*.txt>;
my @comms=sort <comm-2*.txt>;
my %seen;
foreach my $comm (@comms) {
	open(COMM, "<$comm") || die "cannot open $comm";
	#print $comm, "\n";

	while ( my $line = <COMM>)
	{
		#print $line;
		if (0) {
			my $intra=0;
			$intra=1 if ($line =~ /Juha/);
			$intra=1 if ($line =~ /Kayla/);
			$intra=1 if ($line =~ /Olli-Pekka/);
			$intra=1 if ($line =~ /Timo/);
			next unless($intra);
		}

		$line =~ /(.+)  EVENT:? ID:? (.*) ?CONTACT:? (.*) ?DESCR[^: ]*:? (.*) ?DIRECTION:? (.+) ?DURATION:? (.*) ?NUMBER:? (.*) ?STATUS:? (.*) ?(SUBJECT:? .*)?(REMOTE:? .*)?/;

		my $date=$1;
		my $desc=$4;
		if (! $desc || ! $date) {
			print STDERR $comm, "\n";
			print STDERR $line;
			print STDERR "unmatched re\n";
			next;
		}
		next if ($seen{$date});
		$seen{$date}=1;
		#print $line;

		if ($desc =~ /Packet Data/)
		{
			next;
		}
		my $best_candidate="no_candidate";
		foreach my $log (@logs)
		{
			$log =~ /log-(.+).txt/;
			my $logtime = $1;
			if ($logtime le $date)
			{
				$best_candidate = $log;
			}
			else
			{
				last;
			}
		}

	#	print "$date -> $best_candidate\n";
		if ($best_candidate ne "no_candidate")
		{
			#print $best_candidate, "\n";
			open(LOG,"<$best_candidate") || die "cannot open $best_candidate";
			while (my $line2 = <LOG> )
			{
				if ($desc =~ /Short message/)
				{

					if ( $line2 =~ /(.+) SMS :/)
					{
						my $date2=$1;
						#print "comparing ->$date<- and ->$date2<-\n";
						if ( ! (time_diff($date2,$date) eq "") )
						{
							print &time_diff($date2,$date) ." comm=$date -", $desc, " log=" .substr($line2, 0, 40) ."\n";
							last;
						}
					}
				}
				else
				{
					if ( $line2 =~ /(.+) app event: STATUS: call/)
					{
						my $date2=$1;
						#print "comparing ->$date<- and ->$date2<-\n";
						if ( ! (time_diff($date2,$date) eq "") )
						{
							print &time_diff($date2,$date) ." comm=$date -", $desc, " log=" .substr($line2, 0, 40) ."\n";
							last;
						}
					}
				}
			}

			close(LOG);
		}
	}
	close(COMM);
}

exit;


