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

$ENV{'NLS_LANG'}  = "american_america.we8iso8859p1";

my $DBNAME="context.cs.helsinki.fi";
my $DBUSER="context";
my $DBPASS="nSrXt8p";

use DBD::Oracle;

my $dbh=DBI->connect("dbi:Oracle:$DBNAME", $DBUSER, $DBPASS);

$dbh->{RaiseError} = 1;

my $sql=
"select rowid,
(select count(*) from contact_rank c2 where c.person=c2.person) total,
(select count(*) from contact_rank c2 where c2.rank_count<=c.rank_count and 
	c.person=c2.person) lesst_count,
(select count(*) from contact_rank c2 where c2.rank_count_in<=c.rank_count_in and 
	c.person=c2.person) lesst_count_in,
(select count(*) from contact_rank c2 where c2.rank_count_out<=c.rank_count_out and 
	c.person=c2.person) lesst_count_out,
(select count(*) from contact_rank c2 where c2.rank_duration<=c.rank_duration and 
	c.person=c2.person) lesst_duration,
(select count(*) from contact_rank c2 where c2.rank_duration_in<=c.rank_duration_in and 
	c.person=c2.person) lesst_duration_in,
(select count(*) from contact_rank c2 where c2.rank_duration_out<=c.rank_duration_out and 
	c.person=c2.person) lesst_duration_out
from contact_rank c";


$vals=$dbh->selectall_arrayref($sql);
$j=0;
my (@rowids, 
@call_count_prop, @call_duration_prop,
@call_count_prop_in, @call_duration_prop_in,
@call_count_prop_out, @call_duration_prop_out);

while (my $r=$vals->[$j++]) {
	my $rowid=$r->[0];
	my $total=$r->[1];
	my $lesst_count=$r->[2];
	my $lesst_count_in=$r->[3];
	my $lesst_count_out=$r->[4];
	my $lesst_duration=$r->[2];
	my $lesst_duration_in=$r->[3];
	my $lesst_duration_out=$r->[4];

	push(@rowids, $rowid);
	push(@call_count_prop, $lesst_count/$total);
	push(@call_count_prop_in, $lesst_count_in/$total);
	push(@call_count_prop_out, $lesst_count_out/$total);
	push(@call_duration_prop, $lesst_duration/$total);
	push(@call_duration_prop_in, $lesst_duration_in/$total);
	push(@call_duration_prop_out, $lesst_duration_out/$total);
}

$st=$dbh->prepare("update contact_rank set 
call_count_prop=?,
call_count_prop_in=?,
call_count_prop_out=?,
duration_prop=?,
duration_prop_in=?,
duration_prop_out=?
where rowid=?");

$st->bind_param_array(1, \@call_count_prop);
$st->bind_param_array(2, \@call_count_prop_in);
$st->bind_param_array(3, \@call_count_prop_out);
$st->bind_param_array(4, \@call_duration_prop);
$st->bind_param_array(5, \@call_duration_prop_in);
$st->bind_param_array(6, \@call_duration_prop_out);
$st->bind_param_array(7, \@rowids);

my @tuple_status;
$st->execute_array({ ArrayTupleStatus => \@tuple_status });
