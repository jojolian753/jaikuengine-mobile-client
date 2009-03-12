package Jaiku::BBData::SensorEvent;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( stamp priority data name );
use Jaiku::BBData::ShortString;
use Jaiku::BBData::Int;
use Jaiku::BBData::Time;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'stamp' => 'datetime', 'priority' => 'priority', 'name' => 'eventname', 'data' => 'eventdata',  };
    $xml_to_field = { 'datetime' => 'stamp', 'priority' => 'priority', 'eventname' => 'name', 'eventdata' => 'data',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'event';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 10, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub stamp {
    my $self=shift;
    return $self->{stamp} if ($self->{stamp});
    my $ret=new Jaiku::BBData::Time('datetime');
    $self->{stamp}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_stamp {
    my $self=shift;
    my $c=Jaiku::BBData::Time->downbless(shift());
    $self->replace_child( $self->{stamp}, $c );
    $self->{stamp}=$c;
}

sub priority {
    my $self=shift;
    return $self->{priority} if ($self->{priority});
    my $ret=new Jaiku::BBData::Int('priority');
    $self->{priority}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_priority {
    my $self=shift;
    my $c=Jaiku::BBData::Int->downbless(shift());
    $self->replace_child( $self->{priority}, $c );
    $self->{priority}=$c;
}

sub name {
    my $self=shift;
    return $self->{name} if ($self->{name});
    my $ret=new Jaiku::BBData::ShortString('eventname');
    $self->{name}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_name {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{name}, $c );
    $self->{name}=$c;
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
    $c->{element}='eventdata';
}


Jaiku::BBData::Factory::add_class();

1;
