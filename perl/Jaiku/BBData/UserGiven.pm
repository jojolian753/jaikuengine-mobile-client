package Jaiku::BBData::UserGiven;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( since description );
use Jaiku::BBData::LongString;
use Jaiku::BBData::Time;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'description' => 'description', 'since' => 'since',  };
    $xml_to_field = { 'description' => 'description', 'since' => 'since',  };
}

sub new {
    my $class=shift;
    my $name=shift;
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 33, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub description {
    my $self=shift;
    return $self->{description} if ($self->{description});
    my $ret=new Jaiku::BBData::LongString('description');
    $self->{description}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_description {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{description}, $c );
    $self->{description}=$c;
}

sub since {
    my $self=shift;
    return $self->{since} if ($self->{since});
    my $ret=new Jaiku::BBData::Time('since');
    $self->{since}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_since {
    my $self=shift;
    my $c=Jaiku::BBData::Time->downbless(shift());
    $self->replace_child( $self->{since}, $c );
    $self->{since}=$c;
}


Jaiku::BBData::Factory::add_class();

1;
