package Jaiku::BBData::StreamComment;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( authordisplayname postauthornick uuid posttitle postuuid created channelname streamdataid content authornick );
use Jaiku::BBData::ShortString;
use Jaiku::BBData::Int;
use Jaiku::BBData::LongString;
use Jaiku::BBData::UUID;
use Jaiku::BBData::String;
use Jaiku::BBData::Time;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'uuid' => 'uuid', 'postuuid' => 'postuuid', 'authornick' => 'authornick', 'authordisplayname' => 'authordisplayname', 'content' => 'content', 'created' => 'created', 'channelname' => 'channelname', 'postauthornick' => 'postauthornick', 'posttitle' => 'posttitle', 'streamdataid' => 'streamdataid',  };
    $xml_to_field = { 'uuid' => 'uuid', 'postuuid' => 'postuuid', 'authornick' => 'authornick', 'authordisplayname' => 'authordisplayname', 'content' => 'content', 'created' => 'created', 'channelname' => 'channelname', 'postauthornick' => 'postauthornick', 'posttitle' => 'posttitle', 'streamdataid' => 'streamdataid',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'stream_comment';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 57, 1, 0 ];
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

sub authornick {
    my $self=shift;
    return $self->{authornick} if ($self->{authornick});
    my $ret=new Jaiku::BBData::ShortString('authornick');
    $self->{authornick}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_authornick {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{authornick}, $c );
    $self->{authornick}=$c;
}

sub authordisplayname {
    my $self=shift;
    return $self->{authordisplayname} if ($self->{authordisplayname});
    my $ret=new Jaiku::BBData::LongString('authordisplayname');
    $self->{authordisplayname}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_authordisplayname {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{authordisplayname}, $c );
    $self->{authordisplayname}=$c;
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

sub channelname {
    my $self=shift;
    return $self->{channelname} if ($self->{channelname});
    my $ret=new Jaiku::BBData::ShortString('channelname');
    $self->{channelname}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_channelname {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{channelname}, $c );
    $self->{channelname}=$c;
}

sub postauthornick {
    my $self=shift;
    return $self->{postauthornick} if ($self->{postauthornick});
    my $ret=new Jaiku::BBData::ShortString('postauthornick');
    $self->{postauthornick}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_postauthornick {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{postauthornick}, $c );
    $self->{postauthornick}=$c;
}

sub posttitle {
    my $self=shift;
    return $self->{posttitle} if ($self->{posttitle});
    my $ret=new Jaiku::BBData::LongString('posttitle');
    $self->{posttitle}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_posttitle {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{posttitle}, $c );
    $self->{posttitle}=$c;
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
