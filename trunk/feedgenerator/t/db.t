#!/usr/bin/perl

use strict;
use DBI;
use Jaiku::SplitFromDB;
use Test::More tests=>1;

my $dsn = "DBI:mysql:database=jaikudev;host=127.0.0.1";
my $dbh = DBI->connect($dsn, 'root', '', { RaiseError => 1 });

my $dbsplitter=new Jaiku::SplitFromDB($dbh);

my $processed=$dbsplitter->process_from_db(10);
ok($processed==10, "processed all");
