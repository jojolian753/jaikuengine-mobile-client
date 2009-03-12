package Jaiku::BBData::CellNaming;
use strict;
use base qw(Jaiku::BBData);
use fields qw( mappedid name );
use Jaiku::BBData::Simple;

sub new {
    my $class=shift;
    my $name='cellname';
    my $self=new Jaiku::BBData($name);

    return $class->downbless($self);
}

sub set_type_attributes {
    my $self=shift;
    $self->set_attr('{}module', '0x20006E4E');
    $self->set_attr('{}id', '48');
    $self->set_attr('{}major_version', '1');
    $self->set_attr('{}minor_version', '0');
}
sub mappedid {
    my $self=shift;
    return $self->{mappedid} if ($self->{mappedid});
    my $ret=new Jaiku::BBData::Simple('mappedid');
    $self->{mappedid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_mappedid {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{mappedid} );
    $self->{mappedid}=$c;
    $self->push_child($c);
}

sub name {
    my $self=shift;
    return $self->{name} if ($self->{name});
    my $ret=new Jaiku::BBData::Simple('name');
    $self->{name}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_name {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{name} );
    $self->{name}=$c;
    $self->push_child($c);
}


1;
