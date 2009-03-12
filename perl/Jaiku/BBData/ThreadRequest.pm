package Jaiku::BBData::ThreadRequest;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( postuuid streamdataid threadowner );
use Jaiku::BBData::ShortString;
use Jaiku::BBData::Int;
use Jaiku::BBData::UUID;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'threadowner' => 'threadowner', 'postuuid' => 'postuuid', 'streamdataid' => 'streamdataid',  };
    $xml_to_field = { 'threadowner' => 'threadowner', 'postuuid' => 'postuuid', 'streamdataid' => 'streamdataid',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'threadrequest';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 59, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub threadowner {
    my $self=shift;
    return $self->{threadowner} if ($self->{threadowner});
    my $ret=new Jaiku::BBData::ShortString('threadowner');
    $self->{threadowner}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_threadowner {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{threadowner}, $c );
    $self->{threadowner}=$c;
}

sub postuuid {
    my $self=shift;
    return $self->{postuuid} if ($self->{postuuid});
    my $ret=new Jaiku::BBData::UUID('postuuid');
    $self->{postuuid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_postuuid {
    my $self=shift;
    my $c=Jaiku::BBData::UUID->downbless(shift());
    $self->replace_child( $self->{postuuid}, $c );
    $self->{postuuid}=$c;
}

sub streamdataid {
    my $self=shift;
    return $self->{streamdataid} if ($self->{streamdataid});
    my $ret=new Jaiku::BBData::Int('streamdataid');
    $self->{streamdataid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_streamdataid {
    my $self=shift;
    my $c=Jaiku::BBData::Int->downbless(shift());
    $self->replace_child( $self->{streamdataid}, $c );
    $self->{streamdataid}=$c;
}


Jaiku::BBData::Factory::add_class();

1;
