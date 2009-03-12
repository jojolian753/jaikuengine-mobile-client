#!/usr/bin/perl
use DBI;

$database="loca_vis";
$hostname="127.0.0.1";
$port=3308;
$user="mikie";
$password="raento1";

$dsn = "DBI:mysql:database=$database;host=$hostname;port=$port";

$dbh = DBI->connect($dsn, $user, $password);

1;
