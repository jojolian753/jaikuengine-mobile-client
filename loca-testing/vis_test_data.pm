#!/usr/bin/perl

$n1="n1";
$n2="n2";
$n3="n3";
$n4="n4";
$n5="n5";

$bt1="000000000001";
$bt2="000000000002";
$bt3="000000000003";
$bt4="000000000004";
$bt5="000000000005";
$bt6="000000000006";

@visdev=(
	[ $n1, "vis01" ],
	[ $n2, "vis02" ],
	[ $n3, "vis03" ],
	[ $n4, "vis04" ],
	[ $n5, "vis05" ],
);

@visdata=(
	[ $n1, 10, $bt1 ],
	[ $n2, 20, $bt2 ],
	[ $n3, 30, $bt3 ],
	[ $n4, 40, $bt4 ],
	[ $n5, 50, $bt5 ],

	[ $n1, 110, $bt1 ],
	[ $n2, 110, $bt2 ],
	[ $n3, 110, $bt3 ],
	[ $n4, 110, $bt4 ],
	[ $n5, 110, $bt5 ],
);

foreach my $d (@visdata) {
	push (@testdata,
		[ $d->[0], $d->[1], [ $d->[2] ] ]
	);
}

my $min=200;
my $prev_min=$min;
my %temp=();
for($i=0; $i<1000; $i++) {
	$bt=sprintf("%012x", $i+1);
	$min=200+int($i/10);
	if ($prev_min>200 && $prev_min!=$min) {
		foreach my $node (sort keys %temp) {
			push(@testdata, [ $node, $prev_min, [ @{$temp{$node}} ] ]);
		}
		%temp=();
	}
	$prev_min=$min;
	for ($n=1; $n<=5; $n++) {
		$node="n" . $n;
		push(@visdata, [ $node, $min, $bt ] );
		push( @{$temp{$node}}, $bt )
	}
}
foreach my $node (sort keys %temp) {
	push(@testdata, [ $node, $prev_min, [ @{$temp{$node}} ] ]);
}

my %move={};
for($i=1; $i<= 5*50*10; $i++) {
	if ($i<=50) {
		$move{$i}->{node}=1;
		$move{$i}->{stay}=0;
	}
	my %temp=();
	foreach $bt (keys %move) {
		if (++$move{$bt}->{stay} > 10) {;
			$move{$bt}->{node}++;
			$move{$bt}->{stay}=0;
		}
		if ($move{$bt}->{node} > 5) {
			delete $move{$bt};
			next;
		}
		$mac=sprintf("%012x", $bt);
		push(@visdata, [ "n" . $move{$bt}->{node}, 400+$i, $mac ] );
		push( @{ $temp{ "n" . $move{$bt}->{node} } }, $mac )
	}
	foreach my $node (sort keys %temp) {
		push(@testdata, [ $node, 400+$i, [ @{$temp{$node}} ] ]);
	}
}
1;
