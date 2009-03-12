package Jaiku::BBData::CellNaming;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( mappedid name );
use Jaiku::BBData::LongString;
use Jaiku::BBData::Int;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'mappedid' => 'mappedid', 'name' => 'name',  };
    $xml_to_field = { 'mappedid' => 'mappedid', 'name' => 'name',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'cellname';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 48, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub mappedid {
    my $self=shift;
    return $self->{mappedid} if ($self->{mappedid});
    my $ret=new Jaiku::BBData::Int('mappedid');
    $self->{mappedid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_mappedid {
    my $self=shift;
    my $c=Jaiku::BBData::Int->downbless(shift());
    $self->replace_child( $self->{mappedid}, $c );
    $self->{mappedid}=$c;
}

sub name {
    my $self=shift;
    return $self->{name} if ($self->{name});
    my $ret=new Jaiku::BBData::LongString('name');
    $self->{name}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_name {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{name}, $c );
    $self->{name}=$c;
}


Jaiku::BBData::Factory::add_class();

1;
