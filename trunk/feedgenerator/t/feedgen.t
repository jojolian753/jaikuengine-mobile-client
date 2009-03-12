#!/usr/bin/perl

use Test::More tests=>8;
use Jaiku::FeedGenerator;

my @items;
{
	package FeedTest;

	sub new {
		return bless {}, shift;
	}

	sub fetch_last_known {
		return "";
	}

	sub new_item {
		my ($self, $nick, $type, $xml)=@_;
		push(@items, [ $nick, $type, $xml ]);
	}

	1;
}

my $cb=new FeedTest;
my $feedgen=new Jaiku::FeedGenerator($cb);
ok(1, "created");
my $xml1="<presence><status>20060101T120000&lt;presencev2>&lt;/presencev2></status></presence>";
$feedgen->handle_next("mikie", $xml1);
ok(1, "handle didn't crash");
ok($#items==-1, "no items yet");

my $xml2="<presence><status>20060101T120000&lt;presencev2>&lt;a>a&lt;/a>&lt;/presencev2></status></presence>";
$feedgen->handle_next("mikie", $xml2);
ok($#items==0, "one item");
ok($items[0]->[0] eq "mikie", "right nick");
ok($items[0]->[1] eq "a", "right type");
ok($items[0]->[2] eq "<a>a</a>", "right content");

$feedgen->handle_next("mikie", $xml2);
ok($#items==0, "no items from no change");
