package Jaiku::BBData::UserGiven;
use strict;
use base qw(Jaiku::BBData);
use fields qw( since description );
use Jaiku::BBData::Simple;

sub new {
    my $class=shift;
    my $name=shift;
    my $self=new Jaiku::BBData($name);

    return $class->downbless($self);
}

sub set_type_attributes {
    my $self=shift;
    $self->set_attr('{}module', '0x20006E4E');
    $self->set_attr('{}id', '33');
    $self->set_attr('{}major_version', '1');
    $self->set_attr('{}minor_version', '0');
}
sub description {
    my $self=shift;
    return $self->{description} if ($self->{description});
    my $ret=new Jaiku::BBData::Simple('description');
    $self->{description}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_description {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{description} );
    $self->{description}=$c;
    $self->push_child($c);
}

sub since {
    my $self=shift;
    return $self->{since} if ($self->{since});
    my $ret=new Jaiku::BBData::Simple('since');
    $self->{since}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_since {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{since} );
    $self->{since}=$c;
    $self->push_child($c);
}


1;
