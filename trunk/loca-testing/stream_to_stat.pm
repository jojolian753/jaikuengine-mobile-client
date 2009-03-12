#!/usr/bin/perl

my $STAY_IN_SAME=3;

use Data::Dumper;

sub max_t($$$) {
	my $p=shift;
	my %previous_t=%$p;
	my $this_node=shift;
	my $bt=shift;
	my $max=0;
	#print STDERR "max_t for node $this_node bt $bt ";
	foreach my $node (keys %previous_t) {
		next if ($node eq $this_node);
		my $t=int($previous_t{$node}->{$bt});
		$max=$t if ($t > $max);
	}
	#print STDERR "max_t is $max\n";
	return $max;
}

sub stream_to_stat($) {
	my $ref=shift;
	my @data=@{$ref};
	my %s=();
	my %stats=();
	my $t=0;
	my @result;
	my %all_msg=();
	my %previous_failure={};
	my %previous_success={};
	foreach my $d (@data) {
		$now=$d->[1];
		%s=%{$stats->{$d->[0]}};
		foreach my $bt (@{$d->[2]}) {
			my $span=0;
			$s{$bt}->{failure_count}=$all_msg{$bt}->{failure_count};
			$s{$bt}->{success_count}=$all_msg{$bt}->{success_count};
			if ($d->[3] eq $bt) {
				if ($d->[4]<0) {
					$s{$bt}->{local_failure_count}++;
					$s{$bt}->{failure_count}=++$all_msg{$bt}->{failure_count};
					$previous_failure{$d->[0]}->{$bt}=$now;
					$s{$bt}->{previous_local_failure}=$now;
				} elsif ($d->[4]>0) {
					#$s{$bt}->{local_failure_count}=0;
					#$s{$bt}->{failure_count}=$all_msg{$bt}->{failure_count}=0;
					$s{$bt}->{local_success_count}++;
					$s{$bt}->{success_count}=++$all_msg{$bt}->{success_count};
					$previous_success{$d->[0]}->{$bt}=$now;
					$s{$bt}->{previous_local_success}=$now;
				}
			}

			$s{$bt}->{previous_remote_failure}=
				max_t(\%previous_failure, $d->[0], $bt);
			$s{$bt}->{previous_remote_success}=
				max_t(\%previous_success, $d->[0], $bt);
			if ( !defined($s{$bt}->{first_seen}) ) {
				$s{$bt}->{first_seen}=$now;
			}
			if ( !defined($s{$bt}->{last}) || ($s{$bt}->{last} < $now - $STAY_IN_SAME)) {
				$s{$bt}->{prev_begin}=int($s{$bt}->{begin});
				$s{$bt}->{prev_span}=int($s{$bt}->{span});
				$span=1;
				$s{$bt}->{begin}=$now;
				$s{$bt}->{count}+=1;
			} else {
				$span=$s{$bt}->{span};
				$s{$bt}->{sum}-=$span;
				$s{$bt}->{ssum}-=$span*$span;
				$span=$now - $s{$bt}->{begin} + 1;
			}
			$s{$bt}->{span}=$span;
			$s{$bt}->{sum}+=$span;
			$s{$bt}->{ssum}+=$span*$span;
			$s{$bt}->{last}=$now;
			if ($span > $s{$bt}->{maxspan}) {
				$s{$bt}->{maxspan}=$span;
			}
			my $nows={};
			foreach my $key2 (keys %{$s{$bt}}) {
				$nows->{$key2}=$s{$bt}->{$key2};
			}
			push(@result, [ $now, $bt, $nows, $d->[0] ]);
		}
		$stats->{$d->[0]}={ %s };
	}
	#print Dumper(@result);
	return \@result;
}
