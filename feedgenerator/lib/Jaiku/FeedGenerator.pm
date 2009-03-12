#!/usr/bin/perl

package Jaiku::FeedGenerator;
use fields ( 'callback', 'known' );
use Jaiku::Presence;
use Jaiku::XMLSplit;

sub new {
	my ($class, $callback) = @_;
	die "must have a callback" unless $callback;
	my $self=fields::new($class);
	$self->{callback}=$callback;
	$self->{known}={};
	return $self;
}

sub handle_inner {
	my ($self, $nick, $stanza, $do_items)=@_;
	return if ($stanza eq "");
	my $pr=new Jaiku::Presence($stanza);
	return unless ($pr->isContext);
	my $status=$pr->presencev2;
	my $splitter=new Jaiku::XMLSplit($status);
	my $callback=$self->{callback};
	my $known=$self->{known};
	foreach my $element ( $splitter->elements ) {
		my $previous=$known->{$nick}->{$element->[0]};
		$previous="" unless defined($previous);
		next if ($previous eq $element->[1]);
		$callback->new_item($nick, $element->[0], $element->[1]) if ($do_items);
		#print "prev $previous new ", $element->[1], "\n";
		$known->{$nick}->{$element->[0]}=$element->[1];
		#print "known ", $known->{$nick}->{$element->[0]}, "\n";
	}
}

sub handle_next {
	my ($self, $nick, $stanza)=@_;
	my $callback=$self->{callback};

	my $known=$self->{known};
	if (! defined($known->{$nick})) {
		#print "NO PREVIOUS\n";
		$known->{$nick}={};
		$self->handle_inner($nick, $callback->fetch_last_known($nick), 0);
	}
	$self->handle_inner($nick, $stanza, 1);
}

1;
