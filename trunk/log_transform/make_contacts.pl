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

use lib '/home/group/context/perl';
use lib '/home/group/context/perl/i386-linux-thread-multi';

push(@INC, "/home/group/context/perl");
push(@INC, "/home/group/context/perl/i386-linux-thread-multi");

my $DBNAME="context.cs.helsinki.fi";
my $DBUSER="context";
my $DBPASS="nSrXt8p";

use DBD::Oracle;

my $dbh=DBI->connect("dbi:Oracle:$DBNAME", $DBUSER, $DBPASS);

$dbh->{RaiseError} = 1;

my $st0=$dbh->prepare("update call set merged_contact_id=NULL");
$st0->execute();

my $sql2="select person_id, phoneno
from persons";

my $vals=$dbh->selectall_arrayref($sql2);
my $j=0;
my %us_map;
while (my $r=$vals->[$j++]) {
	my $person=$r->[0];
	my $phoneno=$r->[1];

	my $p=$person - ( $person % 10 );
	$us_map{$p}{$phoneno}=$person;
}


my $sql=
"select 
person, phoneno, contact, remote
from call
where length(phoneno) > 3";
#group by
#person, phoneno, contact, remote";

my $lastid=0;

my (%by_phoneno, %by_contact, %by_remote);
sub get_contactid($$$$)
{
	my ($person, $phoneno, $contact, $remote)=@_;
	my $p=$person - ($person % 10);
	my $us=$us_map{$p}{ $phoneno };
	$phoneno=$us if ( defined($us) );
	$remote=~s/ *Kotilinja//;
	my $id;
	$id=$by_phoneno{$person}{$phoneno} unless($id);
	if ($contact > 0) {
		$id=$by_contact{$person}{$contact} unless($id);
	}
	$id=$by_remote{$person}{$remote} unless($id);

	return $id;
}

sub add_contactid($$$$)
{
	my ($person, $phoneno, $contact, $remote)=@_;
	my $p=$person - ($person % 10);
	my $us=$us_map{$p}{ $phoneno };
	$phoneno=$us if ( defined($us) );
	$remote=~s/ *Kotilinja//;
	my $id=get_contactid($person, $phoneno, $contact, $remote);
	$id=$lastid++ unless($id);

	$by_phoneno{$person}{$phoneno}=$id;
	if ($contact > 0) {
		$by_contact{$person}{$contact}=$id;
	}
	$by_remote{$person}{$remote}=$id;

	#print $id, "\n";
}

$vals=$dbh->selectall_arrayref($sql);
$j=0;
while (my $r=$vals->[$j++]) {
	my $person=$r->[0];
	my $phoneno=$r->[1];
	my $contact=$r->[2];
	my $remote=$r->[3];

	add_contactid($person, $phoneno, $contact, $remote);
}

my $update_sql=
"select 
person, phoneno, contact, remote, rowid
from call
where length(phoneno) > 3";
#and merged_contact_id is null
#and rownum<20";

$vals=$dbh->selectall_arrayref($update_sql);
$j=0;
my @rowids;
my @ids;
while (my $r=$vals->[$j++]) {
	my $person=$r->[0];
	my $phoneno=$r->[1];
	my $contact=$r->[2];
	my $remote=$r->[3];
	my $rowid=$r->[4];
	my $id=get_contactid($person, $phoneno, $contact, $remote);

	$id || die "no id for $person, $phoneno, $contact, $remote ($j)\n";

	push(@rowids, $rowid);
	push(@ids, $id);
}

$st=$dbh->prepare("update call set merged_contact_id=? where rowid=?");
$st->bind_param_array(1, \@ids);
$st->bind_param_array(2, \@rowids);
my @tuple_status;
$st->execute_array({ ArrayTupleStatus => \@tuple_status });
#print join("\n", @rowids);
#print join("\n", @ids);
#print join("\n", @tuple_status);

#$dbh->close();
