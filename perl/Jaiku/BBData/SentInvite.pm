package Jaiku::BBData::SentInvite;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( stamp url tonumberhash from );
use Jaiku::BBData::ShortString;
use Jaiku::BBData::LongString;
use Jaiku::BBData::MD5Hash;
use Jaiku::BBData::Time;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'stamp' => 'stamp', 'url' => 'url', 'from' => 'from', 'tonumberhash' => 'tonumberhash',  };
    $xml_to_field = { 'stamp' => 'stamp', 'url' => 'url', 'from' => 'from', 'tonumberhash' => 'tonumberhash',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'invited';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 51, 1, 0 ];
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
    my $ret=new Jaiku::BBData::Time('stamp');
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

sub url {
    my $self=shift;
    return $self->{url} if ($self->{url});
    my $ret=new Jaiku::BBData::LongString('url');
    $self->{url}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_url {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{url}, $c );
    $self->{url}=$c;
}

sub from {
    my $self=shift;
    return $self->{from} if ($self->{from});
    my $ret=new Jaiku::BBData::ShortString('from');
    $self->{from}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_from {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{from}, $c );
    $self->{from}=$c;
}

sub tonumberhash {
    my $self=shift;
    return $self->{tonumberhash} if ($self->{tonumberhash});
    my $ret=new Jaiku::BBData::MD5Hash('tonumberhash');
    $self->{tonumberhash}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_tonumberhash {
    my $self=shift;
    my $c=Jaiku::BBData::MD5Hash->downbless(shift());
    $self->replace_child( $self->{tonumberhash}, $c );
    $self->{tonumberhash}=$c;
}


Jaiku::BBData::Factory::add_class();

1;
