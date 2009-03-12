#!/usr/bin/perl

use Time::Local;

@tests=(
	[ "Sainte Claire", "7 8:00", "7 8:05", 4, "n1", 0 ],
	[ "Sainte Claire", "7 8:11", "7 8:15", 4, "n1", 0 ],
	[ "Sainte Claire", "7 8:21", "7 8:25", 4, "n1", 2 ],
	[ "Ryoji", "7 10:27", "7 13:17", 1, "n1", 1 ],
	[ "n2", "7 10:27", "7 11:27", 2, "n1", 0 ],
	[ "Montgomery", "8 8:00", "8 8:05", 3, "n1", 0 ],
	[ "Montgomery", "8 8:11", "8 8:15", 3, "n1", 0 ],
	[ "Montgomery", "8 8:21", "8 8:25", 3, "n1", 3 ],
	[ "PRNMS", "8 9:21", "8 9:55", 5, "n1", 4 ],
	[ "PRNMS", "8 9:21", "8 9:45", 6, "n1", 0 ],
	[ "n1", "8 10:00", "8 10:05", 7, "n2", -4],
	[ "n2", "8 10:10", "8 10:15", 7, "n2", 19 ],
	[ "n1", "8 10:20", "8 10:25", 7, "n2", -4],
	[ "n2", "8 10:30", "8 10:35", 7, "n2", 0 ],
	[ "" ],
	[ "n1", "8 10:00", "8 10:05", 7, "n2", -4],
	[ "n2", "8 10:10", "8 10:15", 7, "n2", 18 ],
	[ "n1", "8 10:20", "8 10:25", 7, "n2", -4],
	[ "n2", "8 10:30", "8 10:35", 7, "n2", 0 ],
	[ "South Hall entrance 1", "8 11:13", "8 11:15", 7, "n2", 0 ],
	[ "South Hall entrance 1", "8 11:13", "8 11:15", 8, "n2", 18 ],
	[ "Bottom of Cesar Chavez (LHS)", "8 11:20", "8 11:25", 7, "n2", 0 ],
	[ "Museum", "8 12:00", "8 12:00", 7, "Museum", 13 ],
	[ "Museum", "8 14:00", "8 14:00", 7, "Museum", 0 ],
	[ "Museum", "8 14:00", "8 14:00", 8, "Museum", 18 ],
	[ "n1", "8 15:00", "8 15:00", 7, "n1", 14 ],
	[ "n2", "8 15:30", "8 15:30", 9, "n2", 0 ],
	[ "n1", "8 16:00", "8 16:00", 9, "n1", 18 ],
	[ "" ],
	[ "n1", "9 9:00", "9 9:00", 10, "n1", -18 ],
	[ "n1", "9 9:05", "9 9:05", 10, "n1", -18 ],
	[ "n1", "9 10:05", "9 10:05", 10, "n1", 15 ],
	[ "" ],
	[ "n1", "9 12:03", "9 12:55", 11, "n1", 24 ],
	[ "" ],
	[ "Other", "9 13:00", "9 13:30", 12, "Other", 0 ],
	[ "Way", "9 13:40", "9 13:40", 12, "Way", 21 ],
	[ "Other", "9 14:00", "9 14:30", 13, "Other", 0 ],
	[ "Sainte Claire", "9 14:40", "9 14:40", 13, "Sainte Claire", 0 ],
	[ "" ],
	[ "Loca stand", "9 15:01", "9 15:05", 14, "Loca stand", 7 ],
	[ "Palm Circle", "9 15:30", "9 15:32", 15, "Palm Circle", 28 ],
	[ "Bottom of Cesar Chavez (RHS)", "9: 15:30", "9: 15:35", 16, "n1", 0 ],
	[ "Museum", "9: 15:40", "9: 15:45", 16, "Museum", 29 ],
);

sub tounix($)
{
	my $s=shift;
	$s=~m/(\d+) (\d+):(\d+)/;
	my $day=$1;
	my $hour=$2;
	my $min=$3;
	my $year=2006;
	my $month=7;
	my $u=timelocal(0, $min, $hour, $day, $month, $year);
	$u+=3*60*60;
	#print "$u $year-$month-$day $hour:$min:00\n";
	return $u;
}

sub tobt($)
{
	return sprintf("%012x", $_[0]);
}	

@testdata=();

foreach my $t (@tests) {
	if ($t->[0] eq "") {
		push(@msgtest, [ "" ]);
		next;
	}
	$t1=tounix($t->[1]);
	$t2=tounix($t->[2]);
	#print "U $t1 $t2\n";
	for ($m=$t1; $m<$t2+2*60; $m+=2*60) {
		push(@testdata, [ $t->[0], $m/60, [ tobt($t->[3]) ] ]);
	}
	$file="";
	if ( $t->[5] > 0 ) {
		$filespec=sprintf("scripts/%03d*py", $t->[5]);
		#print "$filespec\n";
		@files=< $filespec >;
		foreach my $f (@files) {
			next if ($f=~/test/);
			$file=$f;
		}
		$file=~s/scripts.//;
		#print "FILE $file\n";
	}
	push(@msgtest, [ $t->[4], $m/60, tobt($t->[3]), $t->[5], $t->[0], $file ] );
}

1;
