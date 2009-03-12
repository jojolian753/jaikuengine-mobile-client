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

use Time::Local;

my $out;
my $PERSON=$ARGV[0];

&main;

sub usage
{
	print "Usage:\n";
	print "tosql.pl PERSON DEFAULT_NETWORK [date limit]\n";
	exit 0;
}

sub makesqlname($)
{
	my $n=shift;
	if ($n eq "number") { return "phoneno"; }
	return $n;
}

sub insert($$)
{
	my %fields=%{$_[0]};
	my $tab=$_[1];
	$fields{$tab}{"seqno"}++;

	return unless ($tab eq "activity" or $tab eq "call" or $tab eq "profile" or $tab eq "bt" or
			$tab eq "cell" or $tab eq "activeapp");

	my @names=keys %{$fields{$tab}};
	print $out "INSERT INTO $tab ";
	my $sep="( person, ";
	foreach my $n (@names) {
		my $field_n=makesqlname($n);
		print $out $sep, $field_n;
		$sep=", ";
	}
	print $out " ) VALUES ";
	my $sep="( $PERSON, ";
	foreach my $n (@names) {
		print $out $sep, $fields{$tab}{$n};
		$sep=", ";
	}
	print $out " );\n";
}

sub get_comm_offset($)
{
	my $for_time=shift;

	if ($PERSON==1) { return 3*60*60; }
	if ($PERSON==2) { return 0; }
	if ($PERSON==3) { return 3*60*60; }
	if ($PERSON==4) { return 2*60*60; }

	if ($PERSON==11) { return 2*60*60; }
	if ($PERSON==12) { return 0; }
	if ($PERSON==13) { return 0; }
	if ($PERSON==14) { 
		return 0 if ($for_time < "20041211T171302");
		return 2*60*60; 
	}
	if ($PERSON==15) {
		return 2*60*60 if ($for_time < "20050221T100153");
		return 1*60*60;
	}

	if ($PERSON==21) { return 0; }
	if ($PERSON==22) { return 2*60*60; }
	if ($PERSON==23) { return 0; }
	if ($PERSON==24) {
		return 0 if ($for_time < "20050411T201904");
		return 2*60*60;
	}
	if ($PERSON==25) {
		return 0 if ($for_time < "20050413T091307");
		return 3*60*60;
	}
	if ($PERSON==26) { return 0; }

	return 0;
}

sub format_datetime($)
{
        (my $sec,my $min,my $hour,my $mday,my $mon,my $year,my $wday,my $yday) = gmtime($_[0]);

        return sprintf("%04d%02d%02dT%02d%02d%02d", $year+1900, $mon+1, $mday,
                $hour, $min, $sec);
}

sub formatted_to_datetime($)
{
	my $datetime=shift;
	$datetime =~ /(....)(..)(..)T(..)(..)(..)/;
	(my $year, my $mon, my $d, my $h, my $min, my $s)=($1, $2, $3, $4, $5, $6);
	my $time=timegm($s, $min, $h, $d, $mon-1, $year);
	return $time;
}

sub sqldt($$)
{
	my $dt=shift;
	my $offset=shift;
	my $calc=formatted_to_datetime($dt) + $offset;
	my $str=format_datetime($calc);
	$str=~s/T/ /;
	return "TO_DATE('" . $str ."', 'YYYYMMDD HH24MISS')";
}

sub main {
	if ($#ARGV<0) { usage(); }

	my $DEFAULT_NW=$ARGV[1];
	my $LIMIT=$ARGV[2];
	if ($LIMIT eq "") { $LIMIT="99999999T999999"; }
	$|=1;

	print "$PERSON\n";

	my %started; my %prev;
	my @tables=( "activity", "cell", "call", "profile", "activeapp", "bt" );
	my %fields;

	#open $out, ">context-$PERSON.sql" || die "cannot open context-$PERSON.sql for writing";
	open $out, ">../context-$PERSON.sql" || die "cannot open ../context-$PERSON.sql for writing";
	print $out "SPOOL context-$PERSON.out\n";
	foreach my $tab (@tables) {
		print $out "DELETE FROM $tab WHERE person=$PERSON;\n";
	}
	print $out "\n";

	my @files=<log*.txt>;
	my @files=sort @files;
	my $stop=0; my $do_stop=0;
	my $prev_cell="";
	my %profile_names=();
	$profile_names{'Yleinen'}=0;
	if ($PERSON==24) {
		$profile_names{'[ressu]'}=1;
		$profile_names{'Yö'}=2;
	}
	foreach my $file (@files) {
		print ".";
		my $last_dt;
		open(IN, "<$file");
		while (<IN>) {
			chop;
			s///;
			$_.="  ";
			my $start; my $stop;
			$start=$stop=0;
			if (/STARTING/) {
				$start=1;
				s/ STARTING:/: /;
			} 
			elsif (/STOPPING/) {
				$stop=1;
				s/ STOPPING/: s/;
			}
			/^([^ ]+) ([^:]+): (.*)/;
			(my $dt, my $var, my $val)=($1, $2, $3);
			if ($var eq "location.value") {
				if ( (! $stop) && (! ($val=~/NotifyChange/)) ) {
					my ($ncc, $nwc, $nw, $lac, $cell)=split(/, */, $val);
					if (! $cell ) { die "unmatched re for $_"; }
					unless ($cell =~ /^[0-9 ]+$/) { die "wrong re for $_"; }
					unless ($lac =~ /^[0-9 ]+$/) { die "wrong re for $_"; }
					$val="$lac, $cell, $nw";
					$val=~s/ *,/,/g;
					#print $val, "\n";
				}
				$var="area, cell, nw";
			}

			if ($dt gt $LIMIT) {
				$do_stop=1;
				last;
			}
			$last_dt=$dt;
			if ($var=~/area, cell/) {
				if ($stop) {
					$started{"cell"}=0;
					$fields{"cell"}{"valid_until"}=sqldt($dt,-1);
					insert(\%fields, "cell");
					$fields{"cell"}{"valid_from"}="NULL";
					$prev_cell="";
				} else {
					if ($start) {
						$started{"cell"}=1;
					}
					if ($val =~ /^[^,]*,[^,]*$/) {
						$val.=", " . $DEFAULT_NW;
					}
						
					(my $lac, my $cell, my $nw)=split(/, */, $val);
					$fields{"cell"}{"valid_until"}=sqldt($dt,-1);
					unless($prev_cell eq $val) {
						insert(\%fields, "cell") unless ($prev_cell eq "");
						$fields{"cell"}{"valid_from"}=sqldt($dt,0);
					}
					$prev_cell=$val;
					if ($cell==0 && $lac==0) {
						$started{"cell"}=0;
						$fields{"cell"}{"valid_from"}="NULL";
						$prev_cell="";
					} else {
						$started{"cell"}=1;
						$nw=~s/^ *//;
						$nw=~s/ *$//;
						$fields{"cell"}{"nw"}="'" . $nw . "'";
						$fields{"cell"}{"lac"}=$lac;
						$fields{"cell"}{"cellid"}=$cell;
					}
				}
			} elsif ($var=~/Active_?App/i) {
				if ($stop) {
					$started{"activeapp"}=0;
					$fields{"activeapp"}{"valid_until"}=sqldt($dt,-1);
					insert(\%fields, "activeapp");
					$fields{"activeapp"}{"valid_from"}="NULL";
				} else {
					$started{"activeapp"}=1;
					$fields{"activeapp"}{"valid_until"}=sqldt($dt,-1);
					insert(\%fields, "activeapp");
					$fields{"activeapp"}{"valid_from"}=sqldt($dt,0);
					$val=~s/\[0x/\[/;
					$val=~/\[([0-9a-f]+)\] ([^ ]*)/;
					if (! defined($1)) {
						print STDERR "unmatched re for activeapp on $_", "\n";
						$fields{"activeapp"}{"appuid"}="'0'";
						$fields{"activeapp"}{"appname"}="''";
					} else {
						$fields{"activeapp"}{"appuid"}="'" . $1 . "'";
						$fields{"activeapp"}{"appname"}="'" . $2 . "'";
					}
				}
			} elsif ($var=~/profile/) {
				if ($stop) {
					$started{"profile"}=0;
					$fields{"profile"}{"valid_until"}=sqldt($dt,-1);
					insert(\%fields, "profile");
					$fields{"profile"}{"valid_from"}="NULL";
				} else {
					$started{"profile"}=1;
					$fields{"profile"}{"valid_until"}=sqldt($dt,-1);
					#insert(\%fields, "profile")
						#unless($fields{"profile"}{"valid_from"} eq "NULL");
					$fields{"profile"}{"valid_from"}=sqldt($dt,0);
					$val=~/([0-9]+) ([^ ]+) \(([0-9]+) ([0-9]+) (\w+)\)/;
					$fields{"profile"}{"profilename"}="'" . $2 . "'";
					$fields{"profile"}{"ringtype"}=$3;
					$fields{"profile"}{"volume"}=$4;
					my $id=$1;
					if ($start && $dt gt "20050410T000000") {
						$id=$profile_names{$2};
						die "no id for $2 in $_" unless (defined($id));
					} else {
						$profile_names{$2}=$1;
					}
					$fields{"profile"}{"id"}=$id;
					if ($5 eq "Off") {
						$fields{"profile"}{"vibra"}=0;
					} else {
						$fields{"profile"}{"vibra"}=1;
					}
				}
			} elsif ($var=~/devices/) {
				#print $val, "\n";
				$fields{"bt"}{"datetime"}=sqldt($dt, 0);
				$val=~s/^\s*//;
				$val=~s/\s*$//;
				my $dev=$val;
				if ($dev eq "") {
					$fields{"bt"}{"btaddr"}="NULL";
					$fields{"bt"}{"nick"}="NULL";
					insert(\%fields, "bt");
				}
				while( length($dev) > 1 && (! $dev=~/error/) ) {
					$dev=~/([0-9a-f]+)( \[)([^\]]*)(\] ?)/;
					my $len=length($1)+length($2)+length($3)+length($4);
					$fields{"bt"}{"btaddr"}="'" . $1 . "'";
					$fields{"bt"}{"nick"}="'" . $3 . "'";
					$dev=substr($dev, $len);
					insert(\%fields, "bt");
				}
			} elsif ($var=~/UserActivity/i) {
				if ($stop) {
					$started{"activity"}=0;
					$fields{"activity"}{"valid_until"}=sqldt($dt,-1);
					insert(\%fields, "activity");
					$fields{"activity"}{"valid_from"}="NULL";
				} else {
					$started{"activity"}=1;
					$fields{"activity"}{"valid_until"}=sqldt($dt,-1);
					insert(\%fields, "activity");
					$fields{"activity"}{"valid_from"}=sqldt($dt,0);
					$val=~s/^ *//;
					$val=~s/ *$//;
					$fields{"activity"}{"state"}="'" . $val . "'";
				}
			}
		}
		if ($last_dt) {
			foreach my $st (keys %started) {
				if ($started{$st}) {
					$started{$st}=0;
					
					if ($fields{$st}{"valid_from"} eq $last_dt) {
						$fields{$st}{"valid_from"}="NULL";
						next;
					}
					$fields{$st}{"valid_until"}=sqldt($last_dt,-1);
					insert(\%fields, $st);
					$fields{$st}{"valid_from"}="NULL";
				}
			}
		}
		last if ($do_stop);
	}
	print "\n";
	my @files=<comm*.txt>;
	my @files=sort @files;
	my %comm;
	foreach my $file (@files) {
		print ".";
		open(IN, "<$file");
		while (<IN>) {
			chop;
			s///;
			s/EVENT ID/EVENTID/;
			/EVENTID: ([0-9]+)/;
			my $id=$1;
			/^([0-9]+)T/;
			my $t=$1;
			next unless( defined($id) );
			$id=$t . " " . $id;
			$comm{$id}=$_;
		}
	}
	my $prev_id=-1;
	my @comm_fields=qw(EVENTID CONTACT DESCRIPTION DIRECTION DURATION NUMBER STATUS REMOTE);
	my %fields;
	foreach my $id (sort keys %comm) {
		$_=$comm{$id};
		/^([^ ]+) (.*)/;
		(my $dt, my $vars)=($1, $2);
		my %vars; 
		for (my $i=0; $i<=$#comm_fields; $i++) {
			my $f=$comm_fields[$i];
			my $nextf=$comm_fields[$i+1];
			my $val;
			if ( defined($nextf) ) {
				$vars=~/\Q$f\E: (.*) ?\Q$nextf\E:/;
				$val=$1;
				$val=~s/^\s*//;
				$val=~s/\s*$//;
				$vars{$f}=$val;
			} else {
				$vars=~/\Q$f\E: (.*)/;
				$val=$1;
				$val=~s/^\s*//;
				$val=~s/\s*$//;
				$vars{$f}=$val;
			}
		}
		next if ( $vars{"NUMBER"} eq "16507" );
		my $type;
		if ($vars{"DESCRIPTION"} eq "Voice call") {
			$type="comm.call";
		} else {
			$prev_id=$id;
			next;
		}
		$prev_id=$id;
		foreach my $f (keys %vars) {
			$vars{$f}=~s/'//g;
			if ($f eq "EVENTID" || $f eq "DURATION" ) {
				$fields{"call"}{lc($f)}=$vars{$f};
			} else {
				$fields{"call"}{lc($f)}="'" . $vars{$f} . "'";
			}
		}
		$fields{"call"}{"datetime"}=sqldt($dt, get_comm_offset($dt));
		insert(\%fields, "call");
	}
	print "\n";

	print $out "\nCOMMIT;\n";
	close($out);
}
