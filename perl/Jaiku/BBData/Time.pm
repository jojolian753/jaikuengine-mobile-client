package Jaiku::BBData::Time;
use strict;
use base qw(Jaiku::BBData::Simple);
use Jaiku::BBData::Simple;

sub set_type_attributes {
    my $self=shift;
    $self->set_attr('{}module', '0x20006E4C');
    $self->set_attr('{}id', '5');
    $self->set_attr('{}major_version', '1');
    $self->set_attr('{}minor_version', '0');
}

1;
