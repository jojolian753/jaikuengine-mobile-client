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

$from_date="20060806T090000";
$to_date="20060814T030933";
$interval=60;

if ($interval==0) {
	$sql=
	"SELECT d.name, COUNT(DISTINCT l.mac), '" . to_mysql($from_date) . "' FROM device d, logged_data l
	WHERE l.device_id=d.id
	  AND l.date > '" . to_mysql($from_date) . "'
	  AND l.date <= '" . to_mysql($to_date) . "'
	GROUP BY d.name
	ORDER BY d.name";
} else {
	$fromu=to_unix($from_date)-30*$interval;
	$tou=to_unix($to_date);

	$dbh->do("DELETE FROM intervals");
	while($fromu < $tou-60*$interval) {
		$dbh->do("INSERT INTO intervals (int_begin, int_end) VALUES (?, ?)",
			undef, to_mysql(from_unix($fromu)), 
			to_mysql(from_unix($fromu+60*$interval)));
		$fromu+=30*$interval;
	}
	$sql1=
	"SELECT d.name nodename, COUNT(DISTINCT l.mac) bt_count, i.int_begin int_begin
	   FROM device d, logged_data l, intervals i
	  WHERE l.device_id=d.id
	    AND l.date > i.int_begin
	    AND l.date <= i.int_end
	GROUP BY i.int_begin, d.name";
	$sql2=
	"SELECT d.name nodename, 0 bt_count, i.int_begin int_begin
	FROM intervals i, device d";

	$sql=
	"SELECT nodename, bt_count, int_begin FROM (
		$sql1
		UNION
		$sql2
	) u
	GROUP BY int_begin, nodename
	ORDER BY int_begin, nodename";

}

#print "$sql\n";
#exit 0;
my $sth = $dbh->prepare( $sql );
$sth->execute();

my $rows=$sth->fetchall_arrayref();

print_text($rows);

$sth->finish();
$dbh->disconnect();
