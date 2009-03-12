#!/usr/bin/perl

use Time::Local;

sub to_mysql($)
{
	my $iso=shift;
	$iso=~s/(....)(..)(..)T(..)(..)(..)/$1-$2-$3 $4:$5:$6/;
	return $iso;
}

sub to_unix($)
{
        my $datetime=shift;
        $datetime =~ /(....)(..)(..)T(..)(..)(..)/;
        (my $year, my $mon, my $d, my $h, my $min, my $s)=($1, $2, $3, $4, $5, $6);
        my $time=timegm($s, $min, $h, $d, $mon-1, $year);
        return $time;
}

sub from_unix($)
{
        (my $sec,my $min,my $hour,my $mday,my $mon,my $year,my $wday,my $yday) = gmtime($_[0]);

        return sprintf("%04d%02d%02dT%02d%02d%02d", $year+1900, $mon+1, $mday,
                $hour, $min, $sec);
}

1;
