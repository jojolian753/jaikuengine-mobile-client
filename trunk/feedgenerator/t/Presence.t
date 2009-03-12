#!/usr/bin/perl

use strict;
use Test::More tests => 7;

use Jaiku::Presence;

my $pr = new Jaiku::Presence("<presence/>");
ok(! $pr->isContext(), "empty stanza");
eval { $pr->timestamp() };
ok($@, "timestamp on non-context");
eval { $pr->presencev2() };
ok($@, "presencev2 on non-context");

eval { $pr = new Jaiku::Presence("<pre"); };
ok($@, "broken xml");

$pr = new Jaiku::Presence("<presence><status>20060101T120000&lt;presencev2>&lt;/presencev2></status></presence>");
ok($pr->isContext(), "is context");
ok($pr->timestamp eq "20060101T120000", "timestamp");
ok($pr->presencev2 eq "<presencev2></presencev2>", "presencev2");
