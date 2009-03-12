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

use loca_db;
use loca_vis_format;
use loca_dt;

$from_date="19700101T000000";
$to_date="19700101T101000";
$interval=60;

$sql="
SELECT d.name, l.mac, l.date 
  FROM device d, logged_data l
 WHERE d.id=l.device_id
   AND l.date > '" . to_mysql($from_date) . "'
   AND l.date <= '" . to_mysql($to_date) . "'
ORDER BY l.mac, l.date";


#print $sql, "\n";

my $sth = $dbh->prepare( $sql );
$sth->execute();

my $rows=$sth->fetchall_arrayref();

my $prev_node="";
my $prev_mac="";
my %paths;
foreach my $row (@$rows) {
	unless($row->[1] eq $prev_mac) {
		$prev_node="";
	}
	$node=$row->[0];
	unless($node eq $prev_node || $prev_node eq "") {
		$paths{$prev_node}->{$node}++;
	}
	$prev_node=$node;
	$prev_mac=$row->[1];
}

print_path_text(\%paths);

$sth->finish();
$dbh->disconnect();
