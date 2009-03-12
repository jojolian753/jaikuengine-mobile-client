#!/usr/bin/perl

use strict;
use Carp;
use DBI;

package Jaiku;

my $dsn = "DBI:mysql:database=djabberd_test;host=127.0.0.1";
our $dbh = DBI->connect($dsn, 'djabberd', 'djabberd', { RaiseError => 1 });

1;
