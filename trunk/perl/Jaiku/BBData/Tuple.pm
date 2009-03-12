package Jaiku::BBData::Tuple;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( tupleid tuplemeta data tupleuuid expires );
use Jaiku::BBData::Uint;
use Jaiku::BBData::UUID;
use Jaiku::BBData::Time;
use Jaiku::BBData::TupleMeta;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'tupleid' => 'id', 'tuplemeta' => 'tuplename', 'expires' => 'expires', 'data' => 'tuplevalue', 'tupleuuid' => 'uuid',  };
    $xml_to_field = { 'id' => 'tupleid', 'tuplename' => 'tuplemeta', 'expires' => 'expires', 'tuplevalue' => 'data', 'uuid' => 'tupleuuid',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'tuple';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 7, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub tupleid {
    my $self=shift;
    return $self->{tupleid} if ($self->{tupleid});
    my $ret=new Jaiku::BBData::Uint('id');
    $self->{tupleid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_tupleid {
    my $self=shift;
    my $c=Jaiku::BBData::Uint->downbless(shift());
    $self->replace_child( $self->{tupleid}, $c );
    $self->{tupleid}=$c;
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
    my $c=Jaiku::BBData::TupleMeta->downbless(shift());
    $self->replace_child( $self->{tuplemeta}, $c );
    $self->{tuplemeta}=$c;
}

sub expires {
    my $self=shift;
    return $self->{expires} if ($self->{expires});
    my $ret=new Jaiku::BBData::Time('expires');
    $self->{expires}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_expires {
    my $self=shift;
    my $c=Jaiku::BBData::Time->downbless(shift());
    $self->replace_child( $self->{expires}, $c );
    $self->{expires}=$c;
}

sub data {
    my $self=shift;
    return $self->{data} if ($self->{data});
}

sub set_data {
    my $self=shift;
    my $c=Jaiku::BBData::Factory::cast_xml(shift());
    $self->replace_child( $self->{data}, $c );
    $self->{data}=$c;
    $c->set_type_attributes;
    $c->{element}='tuplevalue';
}

sub tupleuuid {
    my $self=shift;
    return $self->{tupleuuid} if ($self->{tupleuuid});
    my $ret=new Jaiku::BBData::UUID('uuid');
    $self->{tupleuuid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_tupleuuid {
    my $self=shift;
    my $c=Jaiku::BBData::UUID->downbless(shift());
    $self->replace_child( $self->{tupleuuid}, $c );
    $self->{tupleuuid}=$c;
}


Jaiku::BBData::Factory::add_class();

1;
