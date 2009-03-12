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

use Time::Local;

my @mappings=qw(bt nw lac cellid number contact);
my %map; my %mapfile; my %max;
my $out;

&main;

my @offsets; my $offset_i=0;

sub read_offsets()
{
	open(IN, "<offsets") || die "cannot read offsets";
	while(<IN>) {
		#2 comm=20040209T125952, log=20040209T145952 app event: STATUS: call
		/^(\d+) comm=(\w+), log=(\w+) .*/;
		my ($offset, $comm_time)=($1, $2);
		push @offsets, [ $offset, $comm_time ];
	}
}

sub usage
{
	print "Usage:\n";
	print "toxml.pl PERSON DEFAULT_NETWORK [date limit]\n";
	exit 0;
}

sub read_mapping($)
{
	my $m=shift;
	my %map;
	my $max=0;
	open(IN, "<../${m}.map");
	while(<IN>) {
		chop;
		s///;
		(my $name, my $mapped)=split(/\t/);
		$map{$name}=$mapped;
		$max=$mapped if ($mapped>$max);
	}
	close(IN);
	open(my $fh, ">>../${m}.map") || die "cannot open ${m}.map for writing";
	return ($fh, \%map, $max);
}
		
sub add_mapping($$)
{
	my $map=shift;
	my $name=shift;
	$name=~s/^\s*//;
	$name=~s/\s*$//;
	if ($map eq "number") {
		$name=substr($name, -8);
	}
	unless ( defined($map{$map}{$name}) ) {
		my $max=++$max{$map};
		$map{$map}{$name}=$max;
		print { $mapfile{$map} } "$name\t$max\n";
	}
	return $map{$map}{$name};
}

my $indent; my $leaf;

sub begin_document
{
	print $out "<?xml version='1.0' encoding='iso-8859-1'?>\n";
	print $out "<!DOCTYPE events SYSTEM 'log.dtd'>\n";
	print $out "<events>";
	$indent=2;
}

sub end_document 
{
	print $out "\n</events>\n";
}

sub begin_element($)
{
	$leaf=0;
	my $e=shift;
	print $out "\n", " " x $indent, "<$e>";
	$indent+=2;
}

sub characters($)
{
	$leaf=1;
	print $out shift;
}

sub end_element($)
{
	my $e=shift;
	$indent-=2;
	if ($leaf) {
		print $out "</$e>";
	} else {
		print $out "\n", " " x $indent, "</$e>";
	}
	$leaf=0;
}

sub begin_event($$)
{
	my $dt=shift;
	begin_element("event");
	begin_element("datetime");
	characters($dt);
	end_element("datetime");
	begin_element(shift);
}
sub end_event($)
{
	end_element(shift);
	end_element("event");
}

sub leaf($$)
{
	my $e=shift;
	begin_element($e);
	my $c=shift;
	$c=~s/^\s*//;
	$c=~s/\s*$//;
	characters($c);
	end_element($e);
}

sub empty($)
{
	$e=shift;
	print $out "\n", " " x $indent, "<$e />";
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

sub get_comm_offset($)
{
	my $for_time=shift;
	if (1) {
		if ($PERSON==1) { return 2*60*60; }
		if ($PERSON==2) { return 2*60*60; }
		if ($PERSON==3) { return 2*60*60; }
		if ($PERSON==11) { return 3*60*60; }
		if ($PERSON==12) { return 0; }
		if ($PERSON==13) { return 3*60*60; }
		if ($PERSON==14) { return 2*60*60; }
		if ($PERSON==21) { return 2*60*60; }
		if ($PERSON==22) { return 0; }
		if ($PERSON==23) { return 0; }
		if ($PERSON==24) { 
			return 0 if ($for_time < "20041211T171302");
			return 2*60*60; 
		}
		if ($PERSON==25) {
			return 2*60*60 if ($for_time < "20050221T100153");
			return 1*60*60;
		}
		return 0;
	} else {
		if ($#offsets==-1) { return 2*60*60; }
		#my @offsets; my $offset_i=0; my $prev_offset=0;
		my ($offset, $comm_time) = @{$offsets[$offset_i]};
		while ($comm_time < $for_time && $offset_i <= $#offsets) {
			$offset_i++;
			($offset, $comm_time) = @{$offsets[$offset_i]};
		}
		return $offset*60*60;
	}
}

sub do_comm_offset($)
{
        my $dt=shift;
        my $offset=get_comm_offset($dt);
        my $calc=formatted_to_datetime($dt) + $offset;
        my $str=format_datetime($calc);	
	return $str;
}

sub main {
	if ($#ARGV<0) { usage(); }

	my $DEFAULT_NW=$ARGV[1];
	my $PERSON=$ARGV[0];
	my $LIMIT=$ARGV[2];
	if ($LIMIT eq "") { $LIMIT="99999999T999999"; }

	my %started;
	foreach my $m (@mappings) {
		(my $mapfile, my $map, my $max)=read_mapping($m);
		$map{$m}=$map;
		$mapfile{$m}=$mapfile;
		$max{$m}=$max;
	}
	#read_offsets();

	$|=1;
	open $out, ">../context-$PERSON.xml" || die "cannot open context-$PERSON.xml for writing";

	my @files=<log-2*.txt>;
	@files=sort @files;
	#@files=();
	begin_document;
	my $stop=0;
	print "  doing logs";
	foreach $file (@files) {
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
			/^([^ ]+) ([^:]+): (.*)$/;
			(my $dt, my $var, my $val)=($1, $2, $3);
			if ($dt gt $LIMIT) {
				$stop=1;
				last;
			}
			$last_dt=$dt;
			if ($var=~/Network status/) {
				next if ($stop || $start);
				next if ($val=~/0/);
				begin_event($dt, "location");
				begin_element("location.value");
				empty("no_coverage");
				end_element("location.value");
				end_event("location");
			}
			elsif ($var=~/area, cell/ || $var=~/location\.value/) {
				begin_event($dt, "location");
				if ($stop) {
					empty("stop");
					$started{"location"}=0;
				} else {
					if ($start) {
						empty("start");
						$started{"location"}=1;
					}
					my $lac; my $cell;
					if ($var=~/area, cell/) {
						if ($val =~ /^[^,]*,[^,]*$/) {
							$val.=", " . $DEFAULT_NW;
						}
							
						($lac, $cell, $nw)=split(/, */, $val);
					} else {
						(my $mcc, my $mnc, $nw, $lac, $cell)=split(/, */);
						if ($nw eq "") {	
							$nw=$mcc . "." . $mnc;
						}
					}
					begin_element("location.value");
					if ($cell==0 && $lac==0) {
						empty("no_coverage");
					} else {
						leaf("location.network", add_mapping("nw", $nw));
						leaf("location.lac", add_mapping("lac", $lac));
						leaf("location.cellid", add_mapping("cellid", $cell));
					}
					end_element("location.value");
				}
				end_event("location");
			} elsif ($var=~/profile/) {
				if ( $stop || $val=~/-19/ ) {
					if ($started{"profile"}==1) {
						begin_event($dt, "profile");
						empty("stop");
						$started{"profile"}=0;
						end_event("profile");
					}
				} else {
					begin_event($dt, "profile");
					if ($start) {
						empty("start");
						$started{"profile"}=1;
					}
					$id=$val;
					$id=~s/^([0-9]).*$/$1/;
					if ($id eq $val) {
						# oldest style, no id
						$id=2 if ($id =~ /Kokous/ || $id =~ /Meeting/) ;
						$id=0 if ($id =~ /Yleinen/ || $id =~ /General/) ;
						$id=1 if ($id =~ /Äänetön/ || $id =~ /Silent/) ;
						if ($id eq $val) {
							die "no profile id for $id";
						}
					}
					begin_element("profile.value");
					leaf("profile.id", $id);
					end_element("profile.value");
					end_event("profile");
				}
			} elsif ($var=~/ActiveApp/) {
				if ($stop) {
					begin_event($dt, "active_app");
					empty("stop");
					$started{"active_app"}=0;
					end_event("active_app");
				} else {
					begin_event($dt, "active_app");
					if ($start) {
						empty("start");
						$started{"active_app"}=1;
					}
					begin_element("active_app.value");
					$val=~s/^.*\[([^]]+)\].*$/$1/;
					leaf("active_app.uid", $val);
					end_element("active_app.value");
					end_event("active_app");
				}
			} elsif ($var=~/UserActivity/i) {
				if ($stop) {
					begin_event($dt, "useractivity");
					empty("stop");
					$started{"useractivity"}=0;
					end_event("useractivity");
				} else {
					begin_event($dt, "useractivity");
					if ($start) {
						empty("start");
						$started{"useractivity"}=1;
					}
					begin_element("useractivity.value");
					$val=~s/^\s*//;
					$val=~s/\s*$//;
					empty($val);
					end_element("useractivity.value");
					end_event("useractivity");
				}
			} elsif ($var=~/Charger/) {
				if ($stop) {
					begin_event($dt, "charger");
					empty("stop");
					$started{"charger"}=0;
					end_event("charger");
				} else {
					begin_event($dt, "charger");
					if ($start) {
						empty("start");
						$started{"charger"}=1;
					}
					begin_element("charger.value");
					if ($val=~/0/) {
						empty("connected");
					} elsif ($val=~/1/) {
						empty("not_connected");
					} elsif ($val=~/2/) {
						empty("not_charging");
					}
					end_element("charger.value");
					end_event("charger");
				}

			} elsif ($var=~/devices/) {
				next if ($val =~ /Own address/i);
				if ($stop || $val=~/error/ || $val=~/in use/) {
					if ($started{"devices"}==1) {
						begin_event($dt, "devices");
						empty("stop");
						$started{"devices"}=0;
						end_event("devices");
					}
				} else {
					begin_event($dt, "devices");
					if ($start) {
						empty("start");
						$started{"devices"}=1;
					}
					begin_element("devices.value");
					my $dev=$val;
					$dev=~s/^\s*//;
					$dev=~s/\s*$//;
					if ($dev =~ /\[[^\]]*\]/) {
						# names and macs
						while( length($dev) > 1 ) {
							$dev=~/([0-9a-f]+)( \[)([^\]]*)(\] ?)/;
							my $len=length($1)+length($2)+length($3)+length($4);
							begin_element("bt.device");
							leaf("bt.mac", add_mapping("bt", $1));
							$dev=substr($dev, $len);
							end_element("bt.device");
						}
					} else {
						#names only
						foreach my $bt (split(/;/, $dev)) {
							$bt=~s/^\s*//;
							$bt=~s/\s*$//;
							leaf("bt.mac", add_mapping("bt", $bt));
						}
					}
					end_element("devices.value");
					end_event("devices");
				}

			}
		}
		foreach my $st (keys %started) {
			if ($started{$st}) {
				begin_event($last_dt, $st);
				empty("stop");
				end_event($st);
				$started{$st}=0;
			}
		}
		last if ($stop);
	}
	end_document;
	close($out);
	print "\n";

	open $out, ">../comm-$PERSON.xml" || die "cannot open comm-$PERSON.xml for writing";

	print "  doing comm";
	my @files=<comm*.txt>;
	my @files=sort @files;
	my %comm;
	foreach $file (@files) {
		print ".";
		my $prev_line="";
		open(IN, "<$file");
		while (<IN>) {
			chop;
			s///;
			if (/^\d+T\d+ /) {
				my $temp=$_;
				$_=$prev_line;
				$prev_line=$temp;
			} else {
				$prev_line .= $_;
				next;
			}
			s/EVENT ID/EVENTID/;
			/EVENTID: ([0-9]+)/;
			my $id=$1;
			/^([0-9]+)T/;
			my $t=$1;
			next unless( defined($id) );
			$id=$t . " " . $id;
			$comm{$id}=$_;
		}
		if (! ($prev_line eq "") ) {
			$_=$prev_line;
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
	begin_document;
	my $prev_id=-1;
	my @comm_fields=qw(EVENTID CONTACT DESCRIPTION DIRECTION DURATION NUMBER STATUS SUBJECT REMOTE);
	foreach my $id (sort keys %comm) {
		$_=$comm{$id};
		/^([^ ]+) (.*)/;
		(my $dt, my $vars)=($1, $2);
		$dt=do_comm_offset($dt);
		my %vars;
		for (my $i=0; $i<=$#comm_fields; $i++) {
			my $f=$comm_fields[$i];
			my $nextf=$comm_fields[$i+1];
			my $val;
			if ( defined($nextf) ) {
				$vars=~/\Q$f\E:? (.*) ?\Q$nextf\E:?/;
				$val=$1;
				$val=~s/^\s*//;
				$val=~s/\s*$//;
				$vars{$f}=$val;
			} else {
				$vars=~/\Q$f\E:? (.*)/;
				$val=$1;
				$val=~s/^\s*//;
				$val=~s/\s*$//;
				$vars{$f}=$val;
			}
		}
		if ( $vars{"NUMBER"} eq "16507" || $vars{"NUMBER"} eq "15400" ) {
			$prev_id=$id;
			next;
		}
		my $type;
		if ($vars{"DESCRIPTION"} eq "Voice call") {
			$type="comm.call";
		} elsif ($vars{"DESCRIPTION"} eq "Short message") {
			$type="comm.sms";
		} else {
			$prev_id=$id;
			next;
		}
		if ($id>$prev_id+1) {
			if ($prev_id!=-1) {
				begin_event($last_dt, "communication");
				empty("stop");
				end_event("communication");
			}
			begin_event($dt, "communication");
			empty("start");
		} else {
			begin_event($dt, "communication");
		}
		$prev_id=$id;
		empty($type);
		if ($vars{"DIRECTION"} eq "Outgoing") {
			empty("comm.outgoing");
		} else {
			empty("comm.incoming");
		}
		if ($vars{"DESCRIPTION"} eq "Voice call") {
			leaf("comm.duration", $vars{"DURATION"});
		}
		unless ($vars{"NUMBER"} eq "" ) {
		leaf("comm.number", add_mapping("number", $vars{"NUMBER"})); }

		unless ($vars{"REMOTE"} eq "" || $vars{"NUMBER"} eq "") {
			leaf("comm.contact_name", add_mapping("contact", $PERSON . ":" .  $vars{"REMOTE"}));
		}
		end_event("communication");
		$last_dt=$dt;
	}
	begin_event($last_dt, "communication");
	empty("stop");
	end_event("communication");
	end_document;
	print "\n";

	close($out);
}
