package Jaiku::BBData::UserPic;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( phonenumberisverified mbm nick phonenumberhash );
use Jaiku::BBData::String8;
use Jaiku::BBData::ShortString;
use Jaiku::BBData::Bool;
use Jaiku::BBData::MD5Hash;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'nick' => 'nick', 'phonenumberhash' => 'phonenumberhash', 'mbm' => 'mbm', 'phonenumberisverified' => 'phonenumberisverified',  };
    $xml_to_field = { 'nick' => 'nick', 'phonenumberhash' => 'phonenumberhash', 'mbm' => 'mbm', 'phonenumberisverified' => 'phonenumberisverified',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'userpicture';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 50, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub nick {
    my $self=shift;
    return $self->{nick} if ($self->{nick});
    my $ret=new Jaiku::BBData::ShortString('nick');
    $self->{nick}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_nick {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{nick}, $c );
    $self->{nick}=$c;
}

sub phonenumberhash {
    my $self=shift;
    return $self->{phonenumberhash} if ($self->{phonenumberhash});
    my $ret=new Jaiku::BBData::MD5Hash('phonenumberhash');
    $self->{phonenumberhash}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_phonenumberhash {
    my $self=shift;
    my $c=Jaiku::BBData::MD5Hash->downbless(shift());
    $self->replace_child( $self->{phonenumberhash}, $c );
    $self->{phonenumberhash}=$c;
}

sub mbm {
    my $self=shift;
    return $self->{mbm} if ($self->{mbm});
    my $ret=new Jaiku::BBData::String8('mbm');
    $self->{mbm}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_mbm {
    my $self=shift;
    my $c=Jaiku::BBData::String8->downbless(shift());
    $self->replace_child( $self->{mbm}, $c );
    $self->{mbm}=$c;
}

sub phonenumberisverified {
    my $self=shift;
    return $self->{phonenumberisverified} if ($self->{phonenumberisverified});
    my $ret=new Jaiku::BBData::Bool('phonenumberisverified');
    $self->{phonenumberisverified}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_phonenumberisverified {
    my $self=shift;
    my $c=Jaiku::BBData::Bool->downbless(shift());
    $self->replace_child( $self->{phonenumberisverified}, $c );
    $self->{phonenumberisverified}=$c;
}


Jaiku::BBData::Factory::add_class();

1;
