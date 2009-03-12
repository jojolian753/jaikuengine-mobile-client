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

use vis_test_data;
use DBI;
$database="loca_vis";
$hostname="127.0.0.1";
$port=3308;
$user="mikie";
$password="raento1";

$dsn = "DBI:mysql:database=$database;host=$hostname;port=$port";

$|=1;
print "Connecting to database...";
$dbh = DBI->connect($dsn, $user, $password);
print "\n";

$sth = $dbh->prepare("DELETE FROM device");
$sth->execute();
$sth = $dbh->prepare("DELETE FROM logged_data");
$sth->execute();

foreach my $d (@visdev) {
	my $id=$d->[0];
	$id=~s/[^\d]*//;

	$dbh->do("INSERT INTO device (id, imei, name) VALUES (?, ?, ?)", undef,
		$id, $d->[1], $d->[0]);
}
foreach my $d (@visdata) {
	my $unixtime=$d->[1]*60;
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday)=gmtime($unixtime);
	if ($year < 1900) {
		$year+=1900;
	}
	$mon+=1;
	my $timestring=sprintf("%04d-%02d-%02d %02d:%02d:%02d",
		$year, $mon, $mday, $hour, $min, $sec);
	#print "time $timestring\n";
	my $dev=$d->[0];
	$dev=~s/[^\d]*//;
	$dbh->do("INSERT INTO logged_data (
		date, mac, major_class, minor_class,
		name, device_id, expires
		) VALUES (
		?, ?, 2, 2,
		NULL, ?, NOW())", undef,
		$timestring, $d->[2], $dev);
}
		
