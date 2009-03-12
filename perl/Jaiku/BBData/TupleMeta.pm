package Jaiku::BBData::TupleMeta;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( moduleuid subname moduleid );
use Jaiku::BBData::Int;
use Jaiku::BBData::Uid;
use Jaiku::BBData::TupleSubName;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'moduleuid' => 'module_uid', 'moduleid' => 'module_id', 'subname' => 'subname',  };
    $xml_to_field = { 'module_uid' => 'moduleuid', 'module_id' => 'moduleid', 'subname' => 'subname',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'tuplename';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 9, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub moduleuid {
    my $self=shift;
    return $self->{moduleuid} if ($self->{moduleuid});
    my $ret=new Jaiku::BBData::Uid('module_uid');
    $self->{moduleuid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_moduleuid {
    my $self=shift;
    my $c=Jaiku::BBData::Uid->downbless(shift());
    $self->replace_child( $self->{moduleuid}, $c );
    $self->{moduleuid}=$c;
}

sub moduleid {
    my $self=shift;
    return $self->{moduleid} if ($self->{moduleid});
    my $ret=new Jaiku::BBData::Int('module_id');
    $self->{moduleid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_moduleid {
    my $self=shift;
    my $c=Jaiku::BBData::Int->downbless(shift());
    $self->replace_child( $self->{moduleid}, $c );
    $self->{moduleid}=$c;
}

sub subname {
    my $self=shift;
    return $self->{subname} if ($self->{subname});
    my $ret=new Jaiku::BBData::TupleSubName('subname');
    $self->{subname}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_subname {
    my $self=shift;
    my $c=Jaiku::BBData::TupleSubName->downbless(shift());
    $self->replace_child( $self->{subname}, $c );
    $self->{subname}=$c;
}


Jaiku::BBData::Factory::add_class();

1;
