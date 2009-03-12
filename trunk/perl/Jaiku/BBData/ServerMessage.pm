package Jaiku::BBData::ServerMessage;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( url body title );
use Jaiku::BBData::LongString;
use Jaiku::BBData::String;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'title' => 'title', 'body' => 'body', 'url' => 'url',  };
    $xml_to_field = { 'title' => 'title', 'body' => 'body', 'url' => 'url',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'servermessage';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 52, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub title {
    my $self=shift;
    return $self->{title} if ($self->{title});
    my $ret=new Jaiku::BBData::LongString('title');
    $self->{title}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_title {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{title}, $c );
    $self->{title}=$c;
}

sub body {
    my $self=shift;
    return $self->{body} if ($self->{body});
    my $ret=new Jaiku::BBData::String('body');
    $self->{body}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_body {
    my $self=shift;
    my $c=Jaiku::BBData::String->downbless(shift());
    $self->replace_child( $self->{body}, $c );
    $self->{body}=$c;
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


Jaiku::BBData::Factory::add_class();

1;
