package Jaiku::BBData::ChannelPost;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( nick created displayname uuid content channel );
use Jaiku::BBData::ShortString;
use Jaiku::BBData::LongString;
use Jaiku::BBData::UUID;
use Jaiku::BBData::String;
use Jaiku::BBData::Time;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'uuid' => 'uuid', 'channel' => 'channel', 'nick' => 'nick', 'displayname' => 'displayname', 'content' => 'content', 'created' => 'created',  };
    $xml_to_field = { 'uuid' => 'uuid', 'channel' => 'channel', 'nick' => 'nick', 'displayname' => 'displayname', 'content' => 'content', 'created' => 'created',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'channelpost';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 58, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub uuid {
    my $self=shift;
    return $self->{uuid} if ($self->{uuid});
    my $ret=new Jaiku::BBData::UUID('uuid');
    $self->{uuid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_uuid {
    my $self=shift;
    my $c=Jaiku::BBData::UUID->downbless(shift());
    $self->replace_child( $self->{uuid}, $c );
    $self->{uuid}=$c;
}

sub channel {
    my $self=shift;
    return $self->{channel} if ($self->{channel});
    my $ret=new Jaiku::BBData::ShortString('channel');
    $self->{channel}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_channel {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{channel}, $c );
    $self->{channel}=$c;
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

sub displayname {
    my $self=shift;
    return $self->{displayname} if ($self->{displayname});
    my $ret=new Jaiku::BBData::LongString('displayname');
    $self->{displayname}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_displayname {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{displayname}, $c );
    $self->{displayname}=$c;
}

sub content {
    my $self=shift;
    return $self->{content} if ($self->{content});
    my $ret=new Jaiku::BBData::String('content');
    $self->{content}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_content {
    my $self=shift;
    my $c=Jaiku::BBData::String->downbless(shift());
    $self->replace_child( $self->{content}, $c );
    $self->{content}=$c;
}

sub created {
    my $self=shift;
    return $self->{created} if ($self->{created});
    my $ret=new Jaiku::BBData::Time('created');
    $self->{created}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_created {
    my $self=shift;
    my $c=Jaiku::BBData::Time->downbless(shift());
    $self->replace_child( $self->{created}, $c );
    $self->{created}=$c;
}


Jaiku::BBData::Factory::add_class();

1;
