#!/usr/bin/perl

sub print_text($)
{
	my $arrayref=shift;
	foreach my $row (@$arrayref) {
		my $sep="";
		foreach my $col (@$row) {
			print $sep, $col;
			$sep=", ";
		}
		print "\n";
	}
}

sub print_path_text($)
{
	my $paths=shift;
	foreach my $n1 (sort keys %{$paths}) {
		foreach my $n2 (sort keys %{$paths->{$n1}}) {
			print "$n1, $n2, ", $paths->{$n1}->{$n2}, "\n";
		}
	}
}
1;
