#!/usr/bin/perl

package Jaiku::Presence;
use strict;
use fields (
	'xs',
	'parsed'
);
use XML::Simple;

sub new {
	my ($class, $xml) = @_;
	my $self= fields::new($class);
	$self->{xs} = new XML::Simple;
	$self->{parsed} = $self->{xs}->XMLin($xml);
	return $self;
}

sub status {
	my $self=shift;
	return $self->{parsed}->{status};
}

sub isContext {
	my $self=shift;
	my $status=$self->status();
	return 0 unless $status;
	return ($status =~ /\d{8}T\d{6}<presencev2/);
}

sub timestamp {
	my $self=shift;
	die "not a Context presence" if (! $self->isContext());
	return substr($self->status(), 0, 15);
}

sub presencev2 {
	my $self=shift;
	die "not a Context presence" if (! $self->isContext());
	return substr($self->status(), 15);
}

1;
