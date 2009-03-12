package Jaiku::BBData::Tuple;
use strict;
use base qw(Jaiku::BBData);
use fields qw( tupleid tuplemeta data expires );
use Jaiku::BBData::Simple;
use Jaiku::BBData::TupleMeta;

sub new {
    my $class=shift;
    my $name='tuple';
    my $self=new Jaiku::BBData($name);

    return $class->downbless($self);
}

sub set_type_attributes {
    my $self=shift;
    $self->set_attr('{}module', '0x20006E4E');
    $self->set_attr('{}id', '7');
    $self->set_attr('{}major_version', '1');
    $self->set_attr('{}minor_version', '0');
}
sub tupleid {
    my $self=shift;
    return $self->{tupleid} if ($self->{tupleid});
    my $ret=new Jaiku::BBData::Simple('id');
    $self->{tupleid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_tupleid {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{tupleid} );
    $self->{tupleid}=$c;
    $self->push_child($c);
}

sub tuplemeta {
    my $self=shift;
    return $self->{tuplemeta} if ($self->{tuplemeta});
    my $ret=new Jaiku::BBData::TupleMeta;
    $self->{tuplemeta}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_tuplemeta {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{tuplemeta} );
    $self->{tuplemeta}=$c;
    $self->push_child($c);
}

sub expires {
    my $self=shift;
    return $self->{expires} if ($self->{expires});
    my $ret=new Jaiku::BBData::Simple('expires');
    $self->{expires}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_expires {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{expires} );
    $self->{expires}=$c;
    $self->push_child($c);
}

sub data {
    my $self=shift;
    return $self->{data} if ($self->{data});
}

sub set_data {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{data} );
    $self->{data}=$c;
    $self->push_child($c);
    $c->set_type_attributes;
    $c->{element}='tuplevalue';
}


1;
