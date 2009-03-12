#!/usr/bin/perl

use Jaiku::XMLSplit;
use Test::More tests=>7;
use strict;

my $sp=new Jaiku::XMLSplit("<foo><bar/></foo>");
ok(1, "creation");
my @elements=$sp->elements;
ok($#elements==0, "element count");
my $el=$elements[0];
ok($el->[0] eq "bar", "element named bar");
like($el->[1], qr/<bar\s*\/>/, "element $el->[1] is <bar\\s*/>");

$sp=new Jaiku::XMLSplit("<foo><bar>sdf</bar></foo>");
@elements=$sp->elements;
$el=$elements[0];
like($el->[1], qr/<bar>sdf<\/bar>/, "element $el->[1] is <bar>sdf<\/bar>");

$sp=new Jaiku::XMLSplit("<foo><bar>sdf</bar><baz><child/></baz></foo>");
@elements=$sp->elements;
ok($#elements==1, "element count");
$el=$elements[1];
like($el->[1], qr!<baz><child\s*/></baz>!, "element $el->[1] is <baz><child/></baz>");
