package Jaiku::BBData::FeedItem;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( mediadownloadstate streamurl thumbnailmbm authordisplayname correlation location streamtitle authornick parenttitle uuid mediafilename channel parentuuid kind isgroupchild isunread iconid fromserver thumbnailurl created streamdataid parentauthornick content linkedurl );
use Jaiku::BBData::String8;
use Jaiku::BBData::ShortString;
use Jaiku::BBData::Bool;
use Jaiku::BBData::Int;
use Jaiku::BBData::LongString;
use Jaiku::BBData::UUID;
use Jaiku::BBData::String;
use Jaiku::BBData::Time;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'parentuuid' => 'parentuuid', 'uuid' => 'uuid', 'authornick' => 'authornick', 'authordisplayname' => 'authordisplayname', 'thumbnailmbm' => 'thumbnailmbm', 'thumbnailurl' => 'thumbnailurl', 'iconid' => 'iconid', 'created' => 'created', 'linkedurl' => 'linkedurl', 'content' => 'content', 'location' => 'location', 'fromserver' => 'fromserver', 'kind' => 'kind', 'isunread' => 'isunread', 'isgroupchild' => 'isgroupchild', 'correlation' => 'correlation', 'streamtitle' => 'streamtitle', 'streamurl' => 'streamurl', 'channel' => 'channel', 'parentauthornick' => 'parentauthornick', 'parenttitle' => 'parenttitle', 'streamdataid' => 'streamdataid', 'mediafilename' => 'mediafilename', 'mediadownloadstate' => 'mediadownloadstate',  };
    $xml_to_field = { 'parentuuid' => 'parentuuid', 'uuid' => 'uuid', 'authornick' => 'authornick', 'authordisplayname' => 'authordisplayname', 'thumbnailmbm' => 'thumbnailmbm', 'thumbnailurl' => 'thumbnailurl', 'iconid' => 'iconid', 'created' => 'created', 'linkedurl' => 'linkedurl', 'content' => 'content', 'location' => 'location', 'fromserver' => 'fromserver', 'kind' => 'kind', 'isunread' => 'isunread', 'isgroupchild' => 'isgroupchild', 'correlation' => 'correlation', 'streamtitle' => 'streamtitle', 'streamurl' => 'streamurl', 'channel' => 'channel', 'parentauthornick' => 'parentauthornick', 'parenttitle' => 'parenttitle', 'streamdataid' => 'streamdataid', 'mediafilename' => 'mediafilename', 'mediadownloadstate' => 'mediadownloadstate',  };
}

sub new {
    my $class=shift;
    my $name=shift || 'feeditem';
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 54, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub parentuuid {
    my $self=shift;
    return $self->{parentuuid} if ($self->{parentuuid});
    my $ret=new Jaiku::BBData::UUID('parentuuid');
    $self->{parentuuid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_parentuuid {
    my $self=shift;
    my $c=Jaiku::BBData::UUID->downbless(shift());
    $self->replace_child( $self->{parentuuid}, $c );
    $self->{parentuuid}=$c;
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

sub thumbnailmbm {
    my $self=shift;
    return $self->{thumbnailmbm} if ($self->{thumbnailmbm});
    my $ret=new Jaiku::BBData::String8('thumbnailmbm');
    $self->{thumbnailmbm}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_thumbnailmbm {
    my $self=shift;
    my $c=Jaiku::BBData::String8->downbless(shift());
    $self->replace_child( $self->{thumbnailmbm}, $c );
    $self->{thumbnailmbm}=$c;
}

sub thumbnailurl {
    my $self=shift;
    return $self->{thumbnailurl} if ($self->{thumbnailurl});
    my $ret=new Jaiku::BBData::String('thumbnailurl');
    $self->{thumbnailurl}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_thumbnailurl {
    my $self=shift;
    my $c=Jaiku::BBData::String->downbless(shift());
    $self->replace_child( $self->{thumbnailurl}, $c );
    $self->{thumbnailurl}=$c;
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

sub linkedurl {
    my $self=shift;
    return $self->{linkedurl} if ($self->{linkedurl});
    my $ret=new Jaiku::BBData::String('linkedurl');
    $self->{linkedurl}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_linkedurl {
    my $self=shift;
    my $c=Jaiku::BBData::String->downbless(shift());
    $self->replace_child( $self->{linkedurl}, $c );
    $self->{linkedurl}=$c;
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

sub location {
    my $self=shift;
    return $self->{location} if ($self->{location});
    my $ret=new Jaiku::BBData::LongString('location');
    $self->{location}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_location {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{location}, $c );
    $self->{location}=$c;
}

sub fromserver {
    my $self=shift;
    return $self->{fromserver} if ($self->{fromserver});
    my $ret=new Jaiku::BBData::Bool('fromserver');
    $self->{fromserver}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_fromserver {
    my $self=shift;
    my $c=Jaiku::BBData::Bool->downbless(shift());
    $self->replace_child( $self->{fromserver}, $c );
    $self->{fromserver}=$c;
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

sub isunread {
    my $self=shift;
    return $self->{isunread} if ($self->{isunread});
    my $ret=new Jaiku::BBData::Bool('isunread');
    $self->{isunread}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_isunread {
    my $self=shift;
    my $c=Jaiku::BBData::Bool->downbless(shift());
    $self->replace_child( $self->{isunread}, $c );
    $self->{isunread}=$c;
}

sub isgroupchild {
    my $self=shift;
    return $self->{isgroupchild} if ($self->{isgroupchild});
    my $ret=new Jaiku::BBData::Bool('isgroupchild');
    $self->{isgroupchild}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_isgroupchild {
    my $self=shift;
    my $c=Jaiku::BBData::Bool->downbless(shift());
    $self->replace_child( $self->{isgroupchild}, $c );
    $self->{isgroupchild}=$c;
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

sub parentauthornick {
    my $self=shift;
    return $self->{parentauthornick} if ($self->{parentauthornick});
    my $ret=new Jaiku::BBData::ShortString('parentauthornick');
    $self->{parentauthornick}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_parentauthornick {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{parentauthornick}, $c );
    $self->{parentauthornick}=$c;
}

sub parenttitle {
    my $self=shift;
    return $self->{parenttitle} if ($self->{parenttitle});
    my $ret=new Jaiku::BBData::LongString('parenttitle');
    $self->{parenttitle}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_parenttitle {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{parenttitle}, $c );
    $self->{parenttitle}=$c;
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

sub mediafilename {
    my $self=shift;
    return $self->{mediafilename} if ($self->{mediafilename});
    my $ret=new Jaiku::BBData::LongString('mediafilename');
    $self->{mediafilename}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_mediafilename {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{mediafilename}, $c );
    $self->{mediafilename}=$c;
}

sub mediadownloadstate {
    my $self=shift;
    return $self->{mediadownloadstate} if ($self->{mediadownloadstate});
    my $ret=new Jaiku::BBData::Int('mediadownloadstate');
    $self->{mediadownloadstate}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_mediadownloadstate {
    my $self=shift;
    my $c=Jaiku::BBData::Int->downbless(shift());
    $self->replace_child( $self->{mediadownloadstate}, $c );
    $self->{mediadownloadstate}=$c;
}


Jaiku::BBData::Factory::add_class();

1;
