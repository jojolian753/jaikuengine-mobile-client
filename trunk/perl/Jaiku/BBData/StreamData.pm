package Jaiku::BBData::StreamData;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( streamurl authordisplayname uuid correlation kind title iconid channelname created streamdataid content url streamtitle authornick );
use Jaiku::BBData::ShortString;
use Jaiku::BBData::Int;
use Jaiku::BBData::LongString;
use Jaiku::BBData::UUID;
use Jaiku::BBData::String;
use Jaiku::BBData::Time;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'uuid' => 'uuid', 'correlation' => 'correlation', 'authornick' => 'authornick', 'authordisplayname' => 'authordisplayname', 'title' => 'title', 'content' => 'content', 'url' => 'url', 'iconid' => 'iconid', 'created' => 'created', 'kind' => 'kind', 'streamtitle' => 'streamtitle', 'streamurl' => 'streamurl', 'channelname' => 'channelname', 'streamdataid' => 'streamdataid',  };
    $xml_to_field = { 'uuid' => 'uuid', 'correlation' => 'correlation', 'authornick' => 'authornick', 'authordisplayname' => 'authordisplayname', 'title' => 'title', 'content' => 'content', 'url' => 'url', 'iconid' => 'iconid', 'created' => 'created', 'kind' => 'kind', 'streamtitle' => 'streamtitle', 'streamurl' => 'streamurl', 'channelname' => 'channelname', 'streamdataid' => 'streamdataid',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'stream_data';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 56, 1, 0 ];
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

sub correlation {
    my $self=shift;
    return $self->{correlation} if ($self->{correlation});
    my $ret=new Jaiku::BBData::UUID('correlation');
    $self->{correlation}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_correlation {
    my $self=shift;
    my $c=Jaiku::BBData::UUID->downbless(shift());
    $self->replace_child( $self->{correlation}, $c );
    $self->{correlation}=$c;
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

sub url {
    my $self=shift;
    return $self->{url} if ($self->{url});
    my $ret=new Jaiku::BBData::String('url');
    $self->{url}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_url {
    my $self=shift;
    my $c=Jaiku::BBData::String->downbless(shift());
    $self->replace_child( $self->{url}, $c );
    $self->{url}=$c;
}

sub iconid {
    my $self=shift;
    return $self->{iconid} if ($self->{iconid});
    my $ret=new Jaiku::BBData::Int('iconid');
    $self->{iconid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_iconid {
    my $self=shift;
    my $c=Jaiku::BBData::Int->downbless(shift());
    $self->replace_child( $self->{iconid}, $c );
    $self->{iconid}=$c;
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

sub kind {
    my $self=shift;
    return $self->{kind} if ($self->{kind});
    my $ret=new Jaiku::BBData::ShortString('kind');
    $self->{kind}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_kind {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{kind}, $c );
    $self->{kind}=$c;
}

sub streamtitle {
    my $self=shift;
    return $self->{streamtitle} if ($self->{streamtitle});
    my $ret=new Jaiku::BBData::ShortString('streamtitle');
    $self->{streamtitle}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_streamtitle {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{streamtitle}, $c );
    $self->{streamtitle}=$c;
}

sub streamurl {
    my $self=shift;
    return $self->{streamurl} if ($self->{streamurl});
    my $ret=new Jaiku::BBData::String('streamurl');
    $self->{streamurl}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_streamurl {
    my $self=shift;
    my $c=Jaiku::BBData::String->downbless(shift());
    $self->replace_child( $self->{streamurl}, $c );
    $self->{streamurl}=$c;
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
